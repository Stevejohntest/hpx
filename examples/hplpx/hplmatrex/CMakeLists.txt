# Copyright (C) 2011 Dan Kogler
# Copyright (C) 2011 Bryce Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

add_hpx_component(hplmatrex
    SOURCE_GLOB "hplmatrex.c*"
    HEADER_GLOB "hplmatrex.h*"
    FOLDER "Examples/Applications/HplPx"
    AUTOGLOB)

add_hpx_component(lublock
    SOURCE_GLOB "lublock.c*"
    HEADER_GLOB "lublock.h*"
    FOLDER "Examples/Applications/HplPx"
    AUTOGLOB)

