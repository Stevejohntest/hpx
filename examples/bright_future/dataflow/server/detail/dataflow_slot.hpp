
//  Copyright (c) 2011 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_LCOS_DATAFLOW_SERVER_DETAIL_DATAFLOW_ARG_HPP
#define HPX_LCOS_DATAFLOW_SERVER_DETAIL_DATAFLOW_ARG_HPP

#include <examples/bright_future/dataflow/dataflow_fwd.hpp>

namespace hpx { namespace lcos { namespace server { namespace detail {

    template <typename T, int Slot, typename SinkAction>
    struct dataflow_slot
        : base_lco_with_value<T, T>
    {
        typedef hpx::lcos::server::detail::dataflow_slot<T, Slot, SinkAction> wrapped_type;
        typedef components::managed_component<wrapped_type> wrapping_type;
        
        typedef T result_type;
        typedef T remote_result;

        dataflow_slot(SinkAction * back, T const & t)
            : back_ptr_(0)
        {
            back->template set_arg<Slot>(t);
        }

        void set_result(remote_result const & r)
        {
            BOOST_ASSERT(false);
        }

        void connect()
        {
        }

        void set_event()
        {
        }
        
        T get_value()
        {
            BOOST_ASSERT(false);
            return T();
        }

        naming::id_type get_gid() const
        {
            return naming::id_type(get_base_gid(), naming::id_type::unmanaged);
        }

        naming::gid_type get_base_gid() const
        {
            BOOST_ASSERT(back_ptr_);
            return back_ptr_->get_base_gid();
        }

    private:
        template <typename, typename>
        friend class components::managed_component;

        void set_back_ptr(components::managed_component<dataflow_slot>* bp)
        {
            BOOST_ASSERT(0 == back_ptr_);
            BOOST_ASSERT(bp);
            back_ptr_ = bp;
        }
        
        components::managed_component<dataflow_slot>* back_ptr_;
    };

    template <typename Action, typename Result, int Slot, typename SinkAction>
    struct dataflow_slot<hpx::lcos::dataflow<Action, Result>, Slot, SinkAction>
        : hpx::lcos::base_lco_with_value<
            typename boost::mpl::if_<
                boost::is_void<Result>
              , hpx::util::unused_type
              , Result
            >::type
          , typename Action::result_type
        >
    {
        typedef
            typename boost::mpl::if_<
                boost::is_void<Result>
              , hpx::util::unused_type
              , Result
            >::type
            result_type;

        typedef hpx::lcos::dataflow<Action, Result> dataflow_type;
        typedef hpx::lcos::server::detail::dataflow_slot<dataflow_type, Slot, SinkAction> wrapped_type;
        typedef components::managed_component<wrapped_type> wrapping_type;
        
        typedef typename Action::result_type remote_result;

        dataflow_slot(SinkAction * back, dataflow_type const & flow)
            : back_ptr_(0)
            , dataflow_sink(back)
            , dataflow_source(flow)
        {
        }

        ~dataflow_slot()
        {
            LLCO_(info)
                << "~dataflow_slot<dataflow<"
                << util::type_id<Action>::typeid_.type_id()
                << ">, " << Slot
                << util::type_id<SinkAction>::typeid_.type_id()
                << ">::dataflow_slot(): "
                << get_gid();
        }

        void set_result(remote_result const & r)
        {
            LLCO_(info)
                << "dataflow_slot<dataflow<"
                << util::type_id<Action>::typeid_.type_id()
                << ">, " << Slot
                << util::type_id<SinkAction>::typeid_.type_id()
                << ">::set_result(): "
                << get_gid()
                << "\n";
            dataflow_sink
                ->template set_arg<Slot>(
                    traits::get_remote_result<result_type, remote_result>
                        ::call(r)
                );
        }

        void connect()
        {
            LLCO_(info)
                << "dataflow_slot<dataflow<"
                << util::type_id<Action>::typeid_.type_id()
                << ">, " << Slot
                << util::type_id<SinkAction>::typeid_.type_id()
                << ">::connect() from "
                << get_gid();
            dataflow_source.connect(get_gid());
        }

        void set_event()
        {
            if(boost::is_void<Result>::value)
            {
                set_result(remote_result());
            }
            else
            {
                BOOST_ASSERT(false);
            }
        }
        
        result_type get_value()
        {
            BOOST_ASSERT(false);
            return result_type();
        }

        naming::id_type get_gid() const
        {
            return naming::id_type(get_base_gid(), naming::id_type::unmanaged);
        }

        naming::gid_type get_base_gid() const
        {
            BOOST_ASSERT(back_ptr_);
            return back_ptr_->get_base_gid();
        }

    private:
        template <typename, typename>
        friend class components::managed_component;

        void set_back_ptr(components::managed_component<dataflow_slot>* bp)
        {
            BOOST_ASSERT(0 == back_ptr_);
            BOOST_ASSERT(bp);
            back_ptr_ = bp;
        }
        
        components::managed_component<dataflow_slot>* back_ptr_;

        SinkAction * dataflow_sink;
        dataflow_type dataflow_source;
    };

}}}}

namespace hpx { namespace traits
{
    template <
        typename T
      , int Slot
      , typename SinkAction
    >
    struct component_type_database<lcos::server::detail::dataflow_slot<T, Slot, SinkAction> >
    {
        typedef lcos::server::detail::dataflow_slot<T, Slot, SinkAction> dataflow_slot;
        typedef typename dataflow_slot::result_type result_type;
        typedef typename dataflow_slot::remote_result remote_result_type;

        static components::component_type get()
        {
            return
                component_type_database<
                    lcos::base_lco_with_value<result_type, remote_result_type>
                >::get();
        }

        static void set(components::component_type t)
        {
            component_type_database<
                lcos::base_lco_with_value<result_type, remote_result_type>
            >::set(t);
        }
    };
}}
#endif