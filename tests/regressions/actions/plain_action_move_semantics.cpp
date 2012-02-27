////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2011-2012 Hartmut Kaiser
//  Copyright (c) 2012 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#define HPX_ACTION_ARGUMENT_LIMIT 6

#include <hpx/hpx_init.hpp>
#include <hpx/include/plain_actions.hpp>
#include <hpx/lcos/eager_future.hpp>
#include <hpx/lcos/async.hpp>
#include <hpx/components/dataflow/dataflow.hpp>
#include <hpx/util/lightweight_test.hpp>

#include <boost/foreach.hpp>

#include <tests/regressions/actions/components/movable_objects.hpp>

using boost::program_options::variables_map;
using boost::program_options::options_description;
using boost::program_options::value;

using hpx::naming::id_type;

using hpx::actions::plain_action1;
using hpx::actions::plain_result_action1;
using hpx::actions::plain_direct_action1;
using hpx::actions::plain_direct_result_action1;

using hpx::lcos::eager_future;
using hpx::lcos::async;
using hpx::lcos::dataflow;

using hpx::init;
using hpx::finalize;
using hpx::find_here;

using hpx::test::movable_object;
using hpx::test::non_movable_object;

///////////////////////////////////////////////////////////////////////////////
void pass_movable_object_void(movable_object const& obj) {}
std::size_t pass_movable_object(movable_object const& obj)
{
    return obj.get_count();
}

// 'normal' actions (execution is scheduled on a new thread)
typedef plain_action1<
    movable_object const&, pass_movable_object_void
> pass_movable_object_void_action;
typedef plain_result_action1<
    std::size_t, movable_object const&, pass_movable_object
> pass_movable_object_action;

HPX_REGISTER_PLAIN_ACTION(pass_movable_object_void_action);
HPX_REGISTER_PLAIN_ACTION(pass_movable_object_action);

// direct actions (execution happens in the calling thread)
typedef plain_direct_action1<
    movable_object const&, pass_movable_object_void
> pass_movable_object_void_direct_action;
typedef plain_direct_result_action1<
    std::size_t, movable_object const&, pass_movable_object
> pass_movable_object_direct_action;

HPX_REGISTER_PLAIN_ACTION(pass_movable_object_void_direct_action);
HPX_REGISTER_PLAIN_ACTION(pass_movable_object_direct_action);

///////////////////////////////////////////////////////////////////////////////
void pass_non_movable_object_void(non_movable_object const& obj) {}
std::size_t pass_non_movable_object(non_movable_object const& obj)
{
    return obj.get_count();
}

// 'normal' actions (execution is scheduled on a new thread
typedef plain_action1<
    non_movable_object const&, pass_non_movable_object_void
> pass_non_movable_object_void_action;
typedef plain_result_action1<
    std::size_t, non_movable_object const&, pass_non_movable_object
> pass_non_movable_object_action;

HPX_REGISTER_PLAIN_ACTION(pass_non_movable_object_void_action);
HPX_REGISTER_PLAIN_ACTION(pass_non_movable_object_action);

// direct actions (execution happens in the calling thread)
typedef plain_direct_action1<
    non_movable_object const&, pass_non_movable_object_void
> pass_non_movable_object_void_direct_action;
typedef plain_direct_result_action1<
    std::size_t, non_movable_object const&, pass_non_movable_object
> pass_non_movable_object_direct_action;

HPX_REGISTER_PLAIN_ACTION(pass_non_movable_object_void_direct_action);
HPX_REGISTER_PLAIN_ACTION(pass_non_movable_object_direct_action);

///////////////////////////////////////////////////////////////////////////////
template <typename Future, typename Object>
std::size_t pass_object_void()
{
    Object obj;
    Future(find_here(), obj).get();

    return obj.get_count();
}

template <typename Future, typename Object>
std::size_t pass_object(id_type id)
{
    Object obj;
    return Future(id, obj).get();
}

///////////////////////////////////////////////////////////////////////////////
template <typename Future, typename Object>
std::size_t move_object_void()
{
    Object obj;
    Future(find_here(), boost::move(obj)).get();

    return obj.get_count();
}

template <typename Future, typename Object>
std::size_t move_object(id_type id)
{
    Object obj;
    return Future(id, boost::move(obj)).get();
}

///////////////////////////////////////////////////////////////////////////////
template <typename Action, typename Object>
std::size_t async_pass_object_void()
{
    Object obj;
    async<Action>(find_here(), obj).get();

    return obj.get_count();
}

template <typename Action, typename Object>
std::size_t async_pass_object(id_type id)
{
    Object obj;
    return async<Action>(id, obj).get();
}

///////////////////////////////////////////////////////////////////////////////
template <typename Action, typename Object>
std::size_t async_move_object_void()
{
    Object obj;
    async<Action>(find_here(), boost::move(obj)).get();

    return obj.get_count();
}

template <typename Action, typename Object>
std::size_t async_move_object(id_type id)
{
    Object obj;
    return async<Action>(id, boost::move(obj)).get();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    std::vector<id_type> localities = hpx::find_all_localities();

    BOOST_FOREACH(id_type id, localities)
    {
        bool is_local = (id == find_here()) ? true : false;

        if (is_local) {
            // test the void actions locally only (there is no way to get the
            // overall copy count back)
            HPX_TEST_EQ((
                pass_object_void<
                    eager_future<pass_movable_object_void_action>, movable_object
                >()
            ), 1u);
            HPX_TEST_EQ((
                pass_object_void<
                    eager_future<pass_movable_object_void_direct_action>, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                pass_object_void<
                    eager_future<pass_non_movable_object_void_action>, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                pass_object_void<
                    eager_future<pass_non_movable_object_void_direct_action>, non_movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                move_object_void<
                    eager_future<pass_movable_object_void_action>, movable_object
                >()
            ), 0u);
            HPX_TEST_EQ((
                move_object_void<
                    eager_future<pass_movable_object_void_direct_action>, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                move_object_void<
                    eager_future<pass_non_movable_object_void_action>, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                move_object_void<
                    eager_future<pass_non_movable_object_void_direct_action>, non_movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                pass_object_void<
                    dataflow<pass_movable_object_void_action>, movable_object
                >()
            ), 1u);
            HPX_TEST_EQ((
                pass_object_void<
                    dataflow<pass_movable_object_void_direct_action>, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                pass_object_void<
                    dataflow<pass_non_movable_object_void_action>, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                pass_object_void<
                    dataflow<pass_non_movable_object_void_direct_action>, non_movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                move_object_void<
                    dataflow<pass_movable_object_void_action>, movable_object
                >()
            ), 1u);

            HPX_TEST_EQ((
                move_object_void<
                    dataflow<pass_movable_object_void_direct_action>, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                move_object_void<
                    dataflow<pass_non_movable_object_void_action>, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                move_object_void<
                    dataflow<pass_non_movable_object_void_direct_action>, non_movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                async_pass_object_void<
                    pass_movable_object_void_action, movable_object
                >()
            ), 1u);
            HPX_TEST_EQ((
                async_pass_object_void<
                    pass_movable_object_void_direct_action, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                async_pass_object_void<
                    pass_non_movable_object_void_action, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                async_pass_object_void<
                    pass_non_movable_object_void_direct_action, non_movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                async_move_object_void<
                    pass_movable_object_void_action, movable_object
                >()
            ), 0u);
            HPX_TEST_EQ((
                async_move_object_void<
                    pass_movable_object_void_direct_action, movable_object
                >()
            ), 0u);

            HPX_TEST_EQ((
                async_move_object_void<
                    pass_non_movable_object_void_action, non_movable_object
                >()
            ), 2u);
            HPX_TEST_EQ((
                async_move_object_void<
                    pass_non_movable_object_void_direct_action, non_movable_object
                >()
            ), 0u);
        }

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            pass_object<eager_future<pass_movable_object_action>, movable_object>(id)
        ), is_local ? 1u : 2u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            pass_object<eager_future<pass_movable_object_direct_action>, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            pass_object<eager_future<pass_non_movable_object_action>, non_movable_object>(id)
        ), is_local ? 2u : 3u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            pass_object<eager_future<pass_non_movable_object_direct_action>, non_movable_object>(id)
        ), is_local ? 0u : 3u);

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            move_object<eager_future<pass_movable_object_action>, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            move_object<eager_future<pass_movable_object_direct_action>, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            move_object<eager_future<pass_non_movable_object_action>, non_movable_object>(id)
        ), is_local ? 2u : 3u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            move_object<eager_future<pass_non_movable_object_direct_action>, non_movable_object>(id)
        ), is_local ? 0u : 3u);

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            pass_object<dataflow<pass_movable_object_action>, movable_object>(id)
        ), is_local ? 1u : 1u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            pass_object<dataflow<pass_movable_object_direct_action>, movable_object>(id)
        ), is_local ? 0u : 0u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            pass_object<dataflow<pass_non_movable_object_action>, non_movable_object>(id)
        ), is_local ? 2u : 2u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            pass_object<dataflow<pass_non_movable_object_direct_action>, non_movable_object>(id)
        ), is_local ? 0u : 0u);

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            move_object<dataflow<pass_movable_object_action>, movable_object>(id)
        ), is_local ? 1u : 1u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            move_object<dataflow<pass_movable_object_direct_action>, movable_object>(id)
        ), is_local ? 0u : 0u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            move_object<dataflow<pass_non_movable_object_action>, non_movable_object>(id)
        ), is_local ? 2u : 2u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            move_object<dataflow<pass_non_movable_object_direct_action>, non_movable_object>(id)
        ), is_local ? 0u : 0u);

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            async_pass_object<pass_movable_object_action, movable_object>(id)
        ), is_local ? 1u : 2u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            async_pass_object<pass_movable_object_direct_action, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            async_pass_object<pass_non_movable_object_action, non_movable_object>(id)
        ), is_local ? 2u : 3u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            async_pass_object<pass_non_movable_object_direct_action, non_movable_object>(id)
        ), is_local ? 0u : 3u);

        // test for movable object ('normal' actions)
        HPX_TEST_EQ((
            async_move_object<pass_movable_object_action, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for movable object (direct actions)
        HPX_TEST_EQ((
            async_move_object<pass_movable_object_direct_action, movable_object>(id)
        ), is_local ? 0u : 2u);

        // test for a non-movable object ('normal' actions)
        HPX_TEST_EQ((
            async_move_object<pass_non_movable_object_action, non_movable_object>(id)
        ), is_local ? 2u : 3u);

        // test for a non-movable object (direct actions)
        HPX_TEST_EQ((
            async_move_object<pass_non_movable_object_direct_action, non_movable_object>(id)
        ), is_local ? 0u : 3u);
    }

    finalize();

    return hpx::util::report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // Initialize and run HPX.
    return init(desc_commandline, argc, argv);
}
