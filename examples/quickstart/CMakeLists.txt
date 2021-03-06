# Copyright (c) 2007-2012 Hartmut Kaiser
# Copyright (c) 2011      Bryce Adelstein-Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(example_programs
    error_handling
    factorial
    fibonacci
    fibonacci_one
    fibonacci_futures
    fractals
    fractals_oclm
    interest_calculator
    non_atomic_rma
    quicksort
    simplest_hello_world
    timed_wake
   )

if(HPX_CXX11_LAMBDAS)
  set(example_programs ${example_programs}
      hello_world
      1d_wave_equation
     )
endif()

set(error_handling_FLAGS DEPENDENCIES iostreams_component)
set(hello_world_FLAGS DEPENDENCIES iostreams_component)
set(1d_wave_equation_FLAGS DEPENDENCIES iostreams_component)
set(simplest_hello_world_FLAGS DEPENDENCIES iostreams_component)
set(interest_calculator_FLAGS DEPENDENCIES dataflow_component)

foreach(example_program ${example_programs})
  set(sources
      ${example_program}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(${example_program}
                     SOURCES ${sources}
                     ${${example_program}_FLAGS}
                     FOLDER "Examples/Quickstart/${example_program}")

  # add a custom target for this example
  add_hpx_pseudo_target(examples.quickstart.${example_program})

  # make pseudo-targets depend on master pseudo-target
  add_hpx_pseudo_dependencies(examples.quickstart
                              examples.quickstart.${example_program})

  # add dependencies to pseudo-target
  add_hpx_pseudo_dependencies(examples.quickstart.${example_program}
                              ${example_program}_exe)
endforeach()

