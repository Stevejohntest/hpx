//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_fwd.hpp>
#include <hpx/exception.hpp>
#include <hpx/runtime/naming/name.hpp>
#include <hpx/runtime/agas/stubs/component_namespace.hpp>
#include <hpx/runtime/agas/stubs/primary_namespace.hpp>
#include <hpx/runtime/agas/stubs/symbol_namespace.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/counter_creators.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters
{
    ///////////////////////////////////////////////////////////////////////////
    // Creation functions to be registered with counter types

    /// Default discovery function for performance counters; to be registered
    /// with the counter types. It will pass the \a counter_info and the
    /// \a error_code to the supplied function.
    bool default_counter_discoverer(counter_info const& info,
        HPX_STD_FUNCTION<discover_counter_func> const& f, error_code& ec)
    {
        return f(info, ec);
    }

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>{locality#<locality_id>/total}/<instancename>
    ///
    bool locality_counter_discoverer(counter_info const& info,
        HPX_STD_FUNCTION<discover_counter_func> const& f, error_code& ec)
    {
        performance_counters::counter_info i = info;

        // compose the counter name templates
        performance_counters::counter_path_elements p;
        performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status)) return false;

        p.parentinstancename_ = "locality#<*>";
        p.parentinstanceindex_ = -1;
        p.instancename_ = "total";
        p.instanceindex_ = -1;

        status = get_counter_name(p, i.fullname_, ec);
        if (!status_is_valid(status) || !f(i, ec) || ec)
            return false;

//         boost::uint32_t last_locality = get_num_localities();
//         for (boost::uint32_t l = 0; l < last_locality; ++l)
//         {
//             p.parentinstanceindex_ = static_cast<boost::int32_t>(l);
//             status = get_counter_name(p, i.fullname_, ec);
//             if (!status_is_valid(status) || !f(i, ec) || ec)
//                 return false;
//         }

        if (&ec != &throws)
            ec = make_success_code();
        return true;
    }

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>{locality#<locality_id>/thread#<threadnum>}/<instancename>
    ///
    bool locality_thread_counter_discoverer(counter_info const& info,
        HPX_STD_FUNCTION<discover_counter_func> const& f, error_code& ec)
    {
        performance_counters::counter_info i = info;

        // compose the counter name templates
        performance_counters::counter_path_elements p;
        performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status)) return false;

        p.parentinstancename_ = "locality#<*>";
        p.parentinstanceindex_ = -1;
        p.instancename_ = "total";
        p.instanceindex_ = -1;

        status = get_counter_name(p, i.fullname_, ec);
        if (!status_is_valid(status) || !f(i, ec) || ec)
            return false;

        p.instancename_ = "worker-thread#<*>";
        p.instanceindex_ = -1;

        status = get_counter_name(p, i.fullname_, ec);
        if (!status_is_valid(status) || !f(i, ec) || ec)
            return false;

//         boost::uint32_t last_locality = get_num_localities();
//         std::size_t num_threads = get_os_thread_count();
//         for (boost::uint32_t l = 0; l <= last_locality; ++l)
//         {
//             p.parentinstanceindex_ = static_cast<boost::int32_t>(l);
//             p.instancename_ = "total";
//             p.instanceindex_ = -1;
//             status = get_counter_name(p, i.fullname_, ec);
//             if (!status_is_valid(status) || !f(i, ec) || ec)
//                 return false;
//
//             for (std::size_t t = 0; t < num_threads; ++t)
//             {
//                 p.instancename_ = "worker-thread";
//                 p.instanceindex_ = static_cast<boost::int32_t>(t);
//                 status = get_counter_name(p, i.fullname_, ec);
//                 if (!status_is_valid(status) || !f(i, ec) || ec)
//                     return false;
//             }
//         }

        if (&ec != &throws)
            ec = make_success_code();
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Creation function for raw counters. The passed function is encapsulating
    /// the actual value to monitor. This function checks the validity of the
    /// supplied counter name, it has to follow the scheme:
    ///
    ///   /<objectname>{locality#<locality_id>/total}/<instancename>
    ///
    naming::gid_type locality_raw_counter_creator(counter_info const& info,
        HPX_STD_FUNCTION<boost::int64_t()> const& f, error_code& ec)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec) return naming::invalid_gid;

        if (paths.parentinstance_is_basename_) {
            HPX_THROWS_IF(ec, bad_parameter, "locality_raw_counter_creator",
                "invalid counter instance parent name: " +
                    paths.parentinstancename_);
            return naming::invalid_gid;
        }

        if (paths.instancename_ == "total" && paths.instanceindex_ == -1)
            return detail::create_raw_counter(info, f, ec);   // overall counter

        HPX_THROWS_IF(ec, bad_parameter, "locality_raw_counter_creator",
            "invalid counter instance name: " + paths.instancename_);
        return naming::invalid_gid;
    }

    namespace detail
    {
        naming::gid_type retrieve_statistics_counter(std::string const& name,
            naming::id_type const& agas_id, error_code& ec)
        {
            naming::gid_type id;

            //  get action code from counter type
            agas::namespace_action_code service_code =
                agas::detail::retrieve_action_service_code(name, ec);
            if (agas::invalid_request == service_code) return id;

            // compose request
            agas::request req(service_code, name);
            agas::response rep;

            switch (service_code) {
            case agas::component_ns_statistics_counter:
                rep = agas::stubs::component_namespace::service(
                    agas_id, req, threads::thread_priority_default, ec);
                break;
            case agas::primary_ns_statistics_counter:
                rep = agas::stubs::primary_namespace::service(
                    agas_id, req, threads::thread_priority_default, ec);
                break;
            case agas::symbol_ns_service:
                rep = agas::stubs::symbol_namespace::service(
                    agas_id, req, threads::thread_priority_default, ec);
                break;
            default:
                HPX_THROWS_IF(ec, bad_parameter, "retrieve_statistics_counter",
                    "unknown counter agas counter name: " + name);
                break;
            }
            if (!ec)
                id = rep.get_statistics_counter();
            return id;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Creation function for raw AGAS counters. This function checks the
    /// validity of the supplied counter name, it has to follow the scheme:
    ///
    ///   /agas(<objectinstance>/total)/<instancename>
    ///
    naming::gid_type agas_raw_counter_creator(
        counter_info const& info, error_code& ec, char const* const service_name)
    {
        // verify the validity of the counter instance name
        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec) return naming::invalid_gid;

        if (paths.objectname_ != "agas") {
            HPX_THROWS_IF(ec, bad_parameter, "agas_raw_counter_creator",
                "unknown performance counter (unrelated to AGAS)");
            return naming::invalid_gid;
        }
        if (paths.parentinstance_is_basename_) {
            HPX_THROWS_IF(ec, bad_parameter, "agas_raw_counter_creator",
                "invalid counter instance parent name: " +
                    paths.parentinstancename_);
            return naming::invalid_gid;
        }

        if (service_name == paths.parentinstancename_ && paths.parentinstanceindex_ == -1)
        {
            // counter instance name: /<agas_service_name>/<agas_instance_name>/total
            // for instance: /component_namespace/root/total
            std::string::size_type p = paths.instancename_.find("total");
            if (p == paths.instancename_.size() - 5 && paths.instanceindex_ == -1)
            {
                // find the referenced AGAS instance and dispatch the request there
                std::string agas_instance;
                performance_counters::get_counter_instance_name(paths, agas_instance, ec);
                if (ec) return naming::invalid_gid;

                naming::id_type id;
                bool result = agas::resolve_name(
                    agas_instance.substr(0, agas_instance.size() - 6), id, ec);
                if (!result) {
                    HPX_THROWS_IF(ec, not_implemented,
                        "agas_raw_counter_creator",
                        "invalid counter instance name: " + agas_instance);
                    return naming::invalid_gid;
                }
                return detail::retrieve_statistics_counter(info.fullname_, id, ec);
            }
        }

        HPX_THROWS_IF(ec, not_implemented, "agas_raw_counter_creator",
            "invalid counter type name: " + paths.instancename_);
        return naming::invalid_gid;
    }

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>(<objectinstance>/total)/<instancename>
    ///
    bool agas_counter_discoverer(counter_info const& info,
        HPX_STD_FUNCTION<discover_counter_func> const& f, error_code& ec)
    {
        performance_counters::counter_info i_ = info;

        // compose the counter names
        performance_counters::counter_path_elements p;
        performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status)) return false;

        p.parentinstancename_ = "agas";
        p.parentinstanceindex_ = -1;
        p.instanceindex_ = -1;

        // list all counter related to component_ns
        for (std::size_t i = 0;
             i < agas::detail::num_component_namespace_services;
             ++i)
        {
            p.instancename_ = agas::detail::component_namespace_services[i].name_;
            status = get_counter_name(p, i_.fullname_, ec);
            if (!status_is_valid(status) || !f(i_, ec) || ec)
                return false;
        }

        // list all counter related to primary_ns
        for (std::size_t i = 0;
             i < agas::detail::num_primary_namespace_services;
             ++i)
        {
            p.instancename_ = agas::detail::primary_namespace_services[i].name_;
            status = get_counter_name(p, i_.fullname_, ec);
            if (!status_is_valid(status) || !f(i_, ec) || ec)
                return false;
        }

        return true;
    }
}}
