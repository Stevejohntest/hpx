////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_15D904C7_CD18_46E1_A54A_65059966A34F)
#define HPX_15D904C7_CD18_46E1_A54A_65059966A34F

#include <hpx/config.hpp>

#include <vector>

#include <boost/atomic.hpp>
#include <boost/lockfree/fifo.hpp>
#include <boost/make_shared.hpp>
#include <boost/cache/entries/lfu_entry.hpp>
#include <boost/cache/local_cache.hpp>
#include <boost/cstdint.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/noncopyable.hpp>

#include <hpx/exception.hpp>
#include <hpx/hpx_fwd.hpp>
#include <hpx/state.hpp>
#include <hpx/lcos/mutex.hpp>
#include <hpx/lcos/local_counting_semaphore.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/runtime/agas/big_boot_barrier.hpp>
#include <hpx/runtime/agas/component_namespace.hpp>
#include <hpx/runtime/agas/primary_namespace.hpp>
#include <hpx/runtime/agas/symbol_namespace.hpp>
#include <hpx/runtime/agas/gva.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/naming/address.hpp>
#include <hpx/runtime/naming/locality.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/util/runtime_configuration.hpp>

// TODO: split into a base class and two implementations (one for bootstrap,
// one for hosted).

namespace hpx { namespace agas
{

struct HPX_EXPORT addressing_service : boost::noncopyable
{
    // {{{ types 
    typedef primary_namespace::server_type
        primary_namespace_server_type; 

    typedef component_namespace::server_type
        component_namespace_server_type; 

    typedef symbol_namespace::server_type
        symbol_namespace_server_type; 

    typedef component_namespace::component_id_type component_id_type;

    typedef primary_namespace::gva_type gva_type;
    typedef primary_namespace::count_type count_type;
    typedef primary_namespace::offset_type offset_type;
    typedef primary_namespace::endpoint_type endpoint_type;
    typedef component_namespace::prefix_type prefix_type;

    typedef symbol_namespace::iterate_symbols_function_type
        iterateids_function_type;

    typedef hpx::lcos::mutex cache_mutex_type;

    typedef boost::atomic<boost::uint32_t> console_cache_type;
    // }}}

    // {{{ gva cache
    struct gva_cache_key
    { // {{{ gva_cache_key implementation
        gva_cache_key()
          : id()
          , count(0)
        {}

        explicit gva_cache_key(
            naming::gid_type const& id_
          , count_type count_ = 1
            )
          : id(id_)
          , count(count_)
        {}

        naming::gid_type id;
        count_type count;

        friend bool operator<(
            gva_cache_key const& lhs
          , gva_cache_key const& rhs
            )
        {
            // Make sure that the credit has been stripped.
            naming::gid_type raw_lgid = lhs.id
                           , raw_rgid = rhs.id;
            naming::strip_credit_from_gid(raw_lgid); 
            naming::strip_credit_from_gid(raw_rgid); 

            return (raw_lgid + (lhs.count - 1)) < raw_rgid;
        }

        friend bool operator==(
            gva_cache_key const& lhs
          , gva_cache_key const& rhs
            )
        {
            naming::gid_type raw_lgid = lhs.id
                           , raw_rgid = rhs.id;
            naming::strip_credit_from_gid(raw_lgid); 
            naming::strip_credit_from_gid(raw_rgid); 

            // Is lhs in rhs?
            if (1 == lhs.count && 1 != rhs.count)
                return (raw_lgid >= raw_rgid)
                    && (raw_lgid <= (raw_rgid + (lhs.count - 1)));
             
            // Is rhs in lhs?
            else if (1 != lhs.count && 1 == rhs.count)
                return (raw_rgid >= raw_lgid)
                    && (raw_rgid <= (raw_lgid + (lhs.count - 1)));

            // Direct hit 
            else
                return (raw_lgid == raw_rgid) && (lhs.count == rhs.count);
        }
    }; // }}}
    
    struct gva_erase_policy
    { // {{{ gva_erase_policy implementation
        gva_erase_policy(
            naming::gid_type const& id
          , count_type count
            )
          : entry(id, count)
        {}

        typedef std::pair<
            gva_cache_key, boost::cache::entries::lfu_entry<gva_type>
        > entry_type;

        bool operator()(
            entry_type const& p
            ) const
        {
            return p.first == entry;
        }

        gva_cache_key entry;
    }; // }}}

    typedef boost::cache::entries::lfu_entry<gva_type> gva_entry_type;

    typedef boost::cache::local_cache<
        gva_cache_key, gva_entry_type, 
        std::less<gva_entry_type>,
        boost::cache::policies::always<gva_entry_type>,
        std::map<gva_cache_key, gva_entry_type>
    > gva_cache_type;
    // }}}

    // {{{ future freelists
    typedef boost::lockfree::fifo<
        lcos::eager_future<
            primary_namespace_server_type::bind_locality_action,
            response
        >*
    > allocate_response_pool_type;

    typedef boost::lockfree::fifo<
        lcos::eager_future<
            primary_namespace_server_type::bind_gid_action,
            response
        >*
    > bind_response_pool_type;
    // }}}

    struct bootstrap_data_type
    { // {{{
        primary_namespace_server_type primary_ns_server;
        component_namespace_server_type component_ns_server;
        symbol_namespace_server_type symbol_ns_server;
    }; // }}}

    struct hosted_data_type
    { // {{{
        hosted_data_type()
          : console_cache_(0)
        {}

        primary_namespace primary_ns_;
        component_namespace component_ns_;
        symbol_namespace symbol_ns_;

        cache_mutex_type gva_cache_mtx_;
        gva_cache_type gva_cache_; // AKA, the TLB

        console_cache_type console_cache_;

        hpx::lcos::local_counting_semaphore allocate_response_sema_;
        allocate_response_pool_type allocate_response_pool_;

        hpx::lcos::local_counting_semaphore bind_response_sema_;
        bind_response_pool_type bind_response_pool_;

        naming::address primary_ns_addr_;
        naming::address component_ns_addr_;
        naming::address symbol_ns_addr_;
    }; // }}}

    const service_mode service_type;
    const runtime_mode runtime_type;

    boost::shared_ptr<bootstrap_data_type> bootstrap;
    boost::shared_ptr<hosted_data_type> hosted;

    atomic_state state_;
    naming::gid_type prefix_;

    addressing_service(
        parcelset::parcelport& pp 
      , util::runtime_configuration const& ini_
      , runtime_mode runtime_type_
        );

    ~addressing_service()
    {
        // TODO: Free the future pools?
        destroy_big_boot_barrier();
    }

    void launch_bootstrap(
        parcelset::parcelport& pp 
      , util::runtime_configuration const& ini_
        );

    void launch_hosted(
        parcelset::parcelport& pp 
      , util::runtime_configuration const& ini_
        );

    state status() const
    {
        if (!hosted && !bootstrap)
            return stopping;
        else
            return state_.load();
    }
    
    void status(state new_state) 
    {
        state_.store(new_state);
    }

    naming::gid_type const& local_prefix() const
    {
        BOOST_ASSERT(prefix_ != naming::invalid_gid);
        return prefix_;
    }

    void local_prefix(naming::gid_type const& g)
    {
        prefix_ = g;
    }

    bool is_bootstrap() const
    {
        return service_type == service_mode_bootstrap;
    } 

    /// \brief Returns whether this addressing_service represents the console 
    ///        locality.
    bool is_console() const
    {
        return runtime_type == runtime_mode_console;
    }
 
    /// \brief Add a locality to the runtime. 
    bool register_locality(
        naming::locality const& l
      , naming::gid_type& prefix
      , error_code& ec = throws
        );

    /// \brief Remove a locality from the runtime.
    bool unregister_locality(
        naming::locality const& l
      , error_code& ec = throws
        );

    /// \brief Get locality prefix of the console locality.
    ///
    /// \param prefix     [out] The prefix value uniquely identifying the
    ///                   console locality. This is valid only, if the 
    ///                   return value of this function is true.
    /// \param try_cache  [in] If this is set to true the console is first
    ///                   tried to be found in the local cache. Otherwise
    ///                   this function will always query AGAS, even if the 
    ///                   console prefix is already known locally.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if a console prefix 
    ///                   exists and returns \a false otherwise.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool get_console_prefix(
        naming::gid_type& prefix
      , bool try_cache = true, 
        error_code& ec = throws
        );

    bool get_console_prefix_cached(
        naming::gid_type& prefix
      , error_code& ec = throws
        )
    {
        return get_console_prefix(prefix, true, ec);
    }

    /// \brief Query for the prefixes of all known localities.
    ///
    /// This function returns the prefixes of all localities known to the 
    /// AGAS server or all localities having a registered factory for a 
    /// given component type.
    /// 
    /// \param prefixes   [out] The vector will contain the prefixes of all
    ///                   localities registered with the AGAS server. The
    ///                   returned vector holds the prefixes representing 
    ///                   the runtime_support components of these 
    ///                   localities.
    /// \param type       [in] The component type will be used to determine
    ///                   the set of prefixes having a registered factory
    ///                   for this component. The default value for this 
    ///                   parameter is \a components#component_invalid, 
    ///                   which will return prefixes of all localities.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool get_prefixes(
        std::vector<naming::gid_type>& prefixes
      , components::component_type type
      , error_code& ec = throws
        );

    bool get_prefixes(
        std::vector<naming::gid_type>& prefixes
      , error_code& ec = throws
        ) 
    {
        return get_prefixes(prefixes, components::component_invalid, ec);
    }

    /// \brief Return a unique id usable as a component type.
    /// 
    /// This function returns the component type id associated with the 
    /// given component name. If this is the first request for this 
    /// component name a new unique id will be created.
    ///
    /// \param name       [in] The component name (string) to get the 
    ///                   component type for.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The function returns the currently associated 
    ///                   component type. Any error results in an 
    ///                   exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    components::component_type get_component_id(
        std::string const& name
      , error_code& ec = throws
        );

    /// \brief Register a factory for a specific component type
    ///
    /// This function allows to register a component factory for a given
    /// locality and component type.
    ///
    /// \param prefix     [in] The prefix value uniquely identifying the 
    ///                   given locality the factory needs to be registered 
    ///                   for. 
    /// \param name       [in] The component name (string) to register a
    ///                   factory for the given component type for.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The function returns the currently associated 
    ///                   component type. Any error results in an 
    ///                   exception thrown from this function. The returned
    ///                   component type is the same as if the function
    ///                   \a get_component_id was called using the same 
    ///                   component name.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    components::component_type register_factory(
        naming::gid_type const& prefix
      , std::string const& name
      , error_code& ec = throws
        );

    /// \brief Get unique range of freely assignable global ids.
    ///
    /// Every locality needs to be able to assign global ids to different
    /// components without having to consult the AGAS server for every id 
    /// to generate. This function can be called to preallocate a range of
    /// ids usable for this purpose.
    /// 
    /// \param l          [in] The locality the locality id needs to be 
    ///                   generated for. Repeating calls using the same 
    ///                   locality results in identical prefix values.
    /// \param count      [in] The number of global ids to be generated.
    /// \param lower_bound 
    ///                   [out] The lower bound of the assigned id range.
    ///                   The returned value can be used as the first id
    ///                   to assign. This is valid only, if the return 
    ///                   value of this function is true.
    /// \param upper_bound
    ///                   [out] The upper bound of the assigned id range.
    ///                   The returned value can be used as the last id
    ///                   to assign. This is valid only, if the return 
    ///                   value of this function is true.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if a new range has 
    ///                   been generated (it has been called for the first 
    ///                   time for the given locality) and returns \a false 
    ///                   if this locality already got a range assigned in 
    ///                   an earlier call. Any error results in an exception 
    ///                   thrown from this function.
    ///
    /// \note             This function assigns a range of global ids usable
    ///                   by the given locality for newly created components.
    ///                   Any of the returned global ids still has to be 
    ///                   bound to a local address, either by calling 
    ///                   \a bind or \a bind_range.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool get_id_range(
        naming::locality const& l
      , count_type count
      , naming::gid_type& lower_bound
      , naming::gid_type& upper_bound
      , error_code& ec = throws
        );

    /// \brief Bind a global address to a local address.
    ///
    /// Every element in the HPX namespace has a unique global address
    /// (global id). This global address has to be associated with a concrete
    /// local address to be able to address an instance of a component using
    /// it's global address.
    ///
    /// \param id         [in] The global address which has to be bound to 
    ///                   the local address.
    /// \param addr       [in] The local address to be bound to the global 
    ///                   address.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          This function returns \a true, if this global id 
    ///                   got associated with an local address for the 
    ///                   first time. It returns \a false, if the global id 
    ///                   was associated with another local address earlier 
    ///                   and the given local address replaced the 
    ///                   previously associated local address. Any error 
    ///                   results in an exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             Binding a gid to a local address sets its global
    ///                   reference count to one.
    bool bind(
        naming::gid_type const& id
      , naming::address const& addr
      , error_code& ec = throws
        ) 
    {
        return bind_range(id, 1, addr, 0, ec);
    }

    /// \brief Bind unique range of global ids to given base address
    ///
    /// Every locality needs to be able to bind global ids to different
    /// components without having to consult the AGAS server for every id 
    /// to bind. This function can be called to bind a range of consecutive 
    /// global ids to a range of consecutive local addresses (separated by 
    /// a given \a offset).
    /// 
    /// \param lower_id   [in] The lower bound of the assigned id range.
    ///                   The value can be used as the first id to assign. 
    /// \param count      [in] The number of consecutive global ids to bind
    ///                   starting at \a lower_id.
    /// \param baseaddr   [in] The local address to bind to the global id
    ///                   given by \a lower_id. This is the base address 
    ///                   for all additional local addresses to bind to the
    ///                   remaining global ids.
    /// \param offset     [in] The offset to use to calculate the local
    ///                   addresses to be bound to the range of global ids.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if the given range 
    ///                   has been successfully bound and returns \a false 
    ///                   otherwise. Any error results in an exception 
    ///                   thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             Binding a gid to a local address sets its global
    ///                   reference count to one.
    bool bind_range(
        naming::gid_type const& lower_id
      , count_type count
      , naming::address const& baseaddr
      , offset_type offset
      , error_code& ec = throws
        );

    /// \brief Unbind a global address
    ///
    /// Remove the association of the given global address with any local 
    /// address, which was bound to this global address. Additionally it 
    /// returns the local address which was bound at the time of this call.
    /// 
    /// \param id         [in] The global address (id) for which the 
    ///                   association has to be removed.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          The function returns \a true if the association 
    ///                   has been removed, and it returns \a false if no 
    ///                   association existed. Any error results in an 
    ///                   exception thrown from this function.
    ///
    /// \note             You can unbind only global ids bound using the 
    ///                   function \a bind. Do not use this function to 
    ///                   unbind any of the global ids bound using 
    ///                   \a bind_range.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             This function will raise an error if the global 
    ///                   reference count of the given gid is not zero!
    ///                   TODO: confirm that this happens.
    bool unbind(
        naming::gid_type const& id
      , error_code& ec = throws
        ) 
    {
        return unbind_range(id, 1, ec);
    } 
        
    /// \brief Unbind a global address
    ///
    /// Remove the association of the given global address with any local 
    /// address, which was bound to this global address. Additionally it 
    /// returns the local address which was bound at the time of this call.
    /// 
    /// \param id         [in] The global address (id) for which the 
    ///                   association has to be removed.
    /// \param addr       [out] The local address which was associated with 
    ///                   the given global address (id).
    ///                   This is valid only if the return value of this 
    ///                   function is true.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          The function returns \a true if the association 
    ///                   has been removed, and it returns \a false if no 
    ///                   association existed. Any error results in an 
    ///                   exception thrown from this function.
    ///
    /// \note             You can unbind only global ids bound using the 
    ///                   function \a bind. Do not use this function to 
    ///                   unbind any of the global ids bound using 
    ///                   \a bind_range.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             This function will raise an error if the global 
    ///                   reference count of the given gid is not zero!
    ///                   TODO: confirm that this happens.
    bool unbind(
        naming::gid_type const& id
      , naming::address& addr
      , error_code& ec = throws
        ) 
    {
        return unbind_range(id, 1, addr, ec);
    }

    /// \brief Unbind the given range of global ids
    ///
    /// \param lower_id   [in] The lower bound of the assigned id range.
    ///                   The value must the first id of the range as 
    ///                   specified to the corresponding call to 
    ///                   \a bind_range. 
    /// \param count      [in] The number of consecutive global ids to unbind
    ///                   starting at \a lower_id. This number must be 
    ///                   identical to the number of global ids bound by 
    ///                   the corresponding call to \a bind_range
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if a new range has 
    ///                   been generated (it has been called for the first 
    ///                   time for the given locality) and returns \a false 
    ///                   if this locality already got a range assigned in 
    ///                   an earlier call. Any error results in an exception 
    ///                   thrown from this function.
    ///
    /// \note             You can unbind only global ids bound using the 
    ///                   function \a bind_range. Do not use this function 
    ///                   to unbind any of the global ids bound using 
    ///                   \a bind.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             This function will raise an error if the global 
    ///                   reference count of the given gid is not zero!
    ///                   TODO: confirm that this happens.
    bool unbind_range(
        naming::gid_type const& lower_id
      , count_type count
      , error_code& ec = throws
        ) 
    {
        naming::address addr; 
        return unbind_range(lower_id, count, addr, ec);
    } 

    /// \brief Unbind the given range of global ids
    ///
    /// \param lower_id   [in] The lower bound of the assigned id range.
    ///                   The value must the first id of the range as 
    ///                   specified to the corresponding call to 
    ///                   \a bind_range. 
    /// \param count      [in] The number of consecutive global ids to unbind
    ///                   starting at \a lower_id. This number must be 
    ///                   identical to the number of global ids bound by 
    ///                   the corresponding call to \a bind_range
    /// \param addr       [out] The local address which was associated with 
    ///                   the given global address (id).
    ///                   This is valid only if the return value of this 
    ///                   function is true.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if a new range has 
    ///                   been generated (it has been called for the first 
    ///                   time for the given locality) and returns \a false 
    ///                   if this locality already got a range assigned in 
    ///                   an earlier call. 
    ///
    /// \note             You can unbind only global ids bound using the 
    ///                   function \a bind_range. Do not use this function 
    ///                   to unbind any of the global ids bound using 
    ///                   \a bind.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    /// 
    /// \note             This function will raise an error if the global 
    ///                   reference count of the given gid is not zero!
    bool unbind_range(
        naming::gid_type const& lower_id
      , count_type count
      , naming::address& addr
      , error_code& ec = throws
        );

    /// \brief Resolve a given global address (\a id) to its associated local 
    ///        address.
    ///
    /// This function returns the local address which is currently associated 
    /// with the given global address (\a id).
    ///
    /// \param id         [in] The global address (\a id) for which the 
    ///                   associated local address should be returned.
    /// \param addr       [out] The local address which currently is 
    ///                   associated with the given global address (id), 
    ///                   this is valid only if the return value of this 
    ///                   function is true.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \returns          This function returns \a true if the global 
    ///                   address has been resolved successfully (there 
    ///                   exists an association to a local address) and the 
    ///                   associated local address has been returned. The 
    ///                   function returns \a false if no association exists 
    ///                   for the given global address. Any error results 
    ///                   in an exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool resolve(
        naming::gid_type const& id
      , naming::address& addr
      , bool try_cache = true
      , error_code& ec = throws
        );

    bool resolve(
        naming::id_type const& id
      , naming::address& addr
      , bool try_cache = true
      , error_code& ec = throws
        ) 
    {
        return resolve(id.get_gid(), addr, try_cache, ec);
    }

    bool resolve_cached(
        naming::gid_type const& id
      , naming::address& addr
      , error_code& ec = throws
        );

    /// \brief Increment the global reference count for the given id
    ///
    /// \param id         [in] The global address (id) for which the 
    ///                   global reference count has to be incremented.
    /// \param credits    [in] The number of reference counts to add for
    ///                   the given id.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The global reference count after the increment. 
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception. 
    count_type incref(
        naming::gid_type const& id
      , count_type credits = 1
      , error_code& ec = throws
        );

    /// \brief Decrement the global reference count for the given id
    ///
    /// \param id         [in] The global address (id) for which the 
    ///                   global reference count has to be decremented.
    /// \param t          [out] If this was the last outstanding global 
    ///                   reference for the given gid (the return value of 
    ///                   this function is zero), t will be set to the
    ///                   component type of the corresponding element.
    ///                   Otherwise t will not be modified.
    /// \param credits    [in] The number of reference counts to add for
    ///                   the given id.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The global reference count after the decrement. 
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception. 
    count_type decref(
        naming::gid_type const& id
      , components::component_type& t
      , count_type credits = 1
      , error_code& ec = throws
        );

    /// \brief Register a global name with a global address (id)
    /// 
    /// This function registers an association between a global name 
    /// (string) and a global address (id) usable with one of the functions 
    /// above (bind, unbind, and resolve).
    ///
    /// \param name       [in] The global name (string) to be associated
    ///                   with the global address.
    /// \param id         [in] The global address (id) to be associated 
    ///                   with the global address.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The function returns \a true if the global name 
    ///                   got an association with a global address for the 
    ///                   first time, and it returns \a false if this 
    ///                   function call replaced a previously registered 
    ///                   global address with the global address (id) 
    ///                   given as the parameter. Any error results in an 
    ///                   exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool registerid(
        std::string const& name
      , naming::gid_type const& id
      , error_code& ec = throws
        );

    /// \brief Unregister a global name (release any existing association)
    ///
    /// This function releases any existing association of the given global 
    /// name with a global address (id). 
    /// 
    /// \param name       [in] The global name (string) for which any 
    ///                   association with a global address (id) has to be 
    ///                   released.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// \returns          The function returns \a true if an association of 
    ///                   this global name has been released, and it returns 
    ///                   \a false, if no association existed. Any error 
    ///                   results in an exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool unregisterid(
        std::string const& name
      , error_code& ec = throws
        );

    /// \brief Query for the global address associated with a given global name.
    ///
    /// This function returns the global address associated with the given 
    /// global name.
    ///
    /// \param name       [in] The global name (string) for which the 
    ///                   currently associated global address has to be 
    ///                   retrieved.
    /// \param id         [out] The id currently associated with the given 
    ///                   global name (valid only if the return value is 
    ///                   true).
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    /// 
    /// This function returns true if it returned global address (id), 
    /// which is currently associated with the given global name, and it 
    /// returns false, if currently there is no association for this global 
    /// name. Any error results in an exception thrown from this function.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool queryid(
        std::string const& ns_name
      , naming::gid_type& id
      , error_code& ec = throws
        );

    /// \brief Invoke the supplied \a hpx#function for every registered global
    ///        name.
    ///
    /// This function iterates over all registered global ids and 
    /// unconditionally invokes the supplied hpx#function for ever found entry.
    /// Any error results in an exception thrown (or reported) from this 
    /// function.
    /// 
    /// \param f          [in] a \a hpx#function encapsulating an action to be
    ///                   invoked for every currently registered global name.
    /// \param ec         [in,out] this represents the error status on exit,
    ///                   if this is pre-initialized to \a hpx#throws
    ///                   the function will throw on error instead.
    ///
    /// \note             As long as \a ec is not pre-initialized to 
    ///                   \a hpx#throws this function doesn't 
    ///                   throw but returns the result code using the 
    ///                   parameter \a ec. Otherwise it throws an instance
    ///                   of hpx#exception.
    bool iterateids(
        iterateids_function_type const& f
      , error_code& ec = throws
        );
};

}}

#endif // HPX_15D904C7_CD18_46E1_A54A_65059966A34F
