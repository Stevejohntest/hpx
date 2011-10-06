////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_5ABE62AC_CDBC_4EAE_B01B_693CB5F2C0E6)
#define HPX_5ABE62AC_CDBC_4EAE_B01B_693CB5F2C0E6

#include <hpx/runtime/components/client_base.hpp>
#include <hpx/runtime/agas/stubs/component_namespace.hpp>

namespace hpx { namespace agas 
{

struct component_namespace :
    components::client_base<component_namespace, stubs::component_namespace>
{
    // {{{ nested types 
    typedef components::client_base<
        component_namespace, stubs::component_namespace
    > base_type; 

    typedef server::component_namespace server_type;
    
    typedef server_type::component_id_type component_id_type;
    typedef server_type::prefix_type prefix_type;
    typedef server_type::prefixes_type prefixes_type;
    // }}}

    component_namespace()
      : base_type(bootstrap_component_namespace_id())
    {}

    explicit component_namespace(naming::id_type const& id)
      : base_type(id)
    {}

    ///////////////////////////////////////////////////////////////////////////
    // bind interface 
    lcos::promise<response>
    bind_async(std::string const& key, prefix_type prefix)
    { return this->base_type::bind_prefix_async(this->gid_, key, prefix); }

    response bind(std::string const& key, prefix_type prefix,
                       error_code& ec = throws)
    { return this->base_type::bind_prefix(this->gid_, key, prefix, ec); }

    lcos::promise<response>
    bind_async(std::string const& key, naming::gid_type const& prefix)
    {
        return this->base_type::bind_prefix_async
            (this->gid_, key, naming::get_prefix_from_gid(prefix));
    }

    response bind(std::string const& key,
                       naming::gid_type const& prefix,
                       error_code& ec = throws)
    {
        return this->base_type::bind_prefix
            (this->gid_, key, naming::get_prefix_from_gid(prefix), ec);
    }
    
    lcos::promise<response> bind_async(std::string const& key)
    { return this->base_type::bind_name_async(this->gid_, key); }
    
    response bind(std::string const& key, error_code& ec = throws)
    { return this->base_type::bind_name(this->gid_, key, ec); }

    ///////////////////////////////////////////////////////////////////////////
    // resolve_id and resolve_name interface 
    lcos::promise<response> resolve_async(component_id_type key)
    { return this->base_type::resolve_id_async(this->gid_, key); }
    
    response resolve(component_id_type key, error_code& ec = throws)
    { return this->base_type::resolve_id(this->gid_, key, ec); }

    lcos::promise<response>
    resolve_async(components::component_type key)
    {
        return this->base_type::resolve_id_async
            (this->gid_, component_id_type(key));
    }
    
    response resolve(components::component_type key,
                          error_code& ec = throws)
    {
        return this->base_type::resolve_id
            (this->gid_, component_id_type(key), ec);
    }

    lcos::promise<response>
    resolve_async(std::string const& key)
    { return this->base_type::resolve_name_async(this->gid_, key); }
    
    response resolve(std::string const& key,
                          error_code& ec = throws)
    { return this->base_type::resolve_name(this->gid_, key, ec); }
 
    ///////////////////////////////////////////////////////////////////////////
    // unbind interface 
    lcos::promise<response>
    unbind_async(std::string const& key)
    { return this->base_type::unbind_async(this->gid_, key); }
    
    response unbind(std::string const& key,
                         error_code& ec = throws)
    { return this->base_type::unbind(this->gid_, key, ec); }
};            

}}

#endif // HPX_5ABE62AC_CDBC_4EAE_B01B_693CB5F2C0E6
