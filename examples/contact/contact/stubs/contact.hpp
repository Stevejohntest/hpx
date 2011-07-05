//  Copyright (c) 2011 Matt Anderson
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(HPX_COMPONENTS_STUBS_CONTACT_JUN_06_2011_1125AM)
#define HPX_COMPONENTS_STUBS_CONTACT_JUN_06_2011_1125AM

#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/applier/applier.hpp>
#include <hpx/runtime/components/stubs/runtime_support.hpp>
#include <hpx/runtime/components/stubs/stub_base.hpp>
#include <hpx/lcos/eager_future.hpp>

#include <examples/contact/contact/server/contact.hpp>

namespace hpx { namespace components { namespace stubs
{
    ///////////////////////////////////////////////////////////////////////////
    /// The \a stubs#simple_accumulator class is the client side representation 
    /// of all \a server#simple_accumulator components
    struct contact : stub_base<server::contact>
    {
        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. This is a non-blocking call. The caller 
        /// needs to call \a future_value#get on the return value of 
        /// this function to obtain the result as returned by the simple_accumulator.
        static lcos::future_value<int> query_async(naming::id_type gid) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized eager_future, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::contact::query_action action_type;
            return lcos::eager_future<action_type, int>(gid);
        }

        static lcos::future_value<void> contactsearch_async(naming::id_type gid) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized eager_future, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::contact::contactsearch_action action_type;
            return lcos::eager_future<action_type,void>(gid);
        }

        static lcos::future_value<void> contactenforce_async(naming::id_type gid) 
        {
            // Create an eager_future, execute the required action,
            // we simply return the initialized eager_future, the caller needs
            // to call get() on the return value to obtain the result
            typedef server::contact::contactenforce_action action_type;
            return lcos::eager_future<action_type,void>(gid);
        }

        /// Query the current value of the server#simple_accumulator instance 
        /// with the given \a gid. Block for the current simple_accumulator value to 
        /// be returned.
        static double query(naming::id_type gid) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the future_value
            return query_async(gid).get();
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Initialize the simple_accumulator value of the server#simple_accumulator instance 
        /// with the given \a gid
        static void init(naming::id_type gid,int i) 
        {
            applier::apply<server::contact::init_action>(gid,i);
        }

        static void contactsearch(naming::id_type gid) 
        {
            // The following get yields control while the action above 
            // is executed and the result is returned to the future_value
            contactsearch_async(gid).get();
        }

        /// Print the current value of the server#simple_accumulator instance 
        /// with the given \a gid
        static void contactenforce(naming::id_type gid) 
        {
            contactenforce_async(gid).get();
          //  applier::apply<server::contact::print_action>(gid);
        }
    };

}}}

#endif