# A deliberately rudimentary command-line option parser.
#
# Copyright (C) 2006-2018 James D. Lin <jamesdlin@berkeley.edu>
#
# The latest version of this file can be downloaded from:
# <http://www.taenarum.com/software/dropt/>
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source distribution.
#

cmake_minimum_required(VERSION 2.8.11)

project(dropt)

set(dropt_h_files
    ./include/dropt.h
    ./include/dropt_string.h
)

set(dropt_c_files
    ./src/dropt.c
    ./src/dropt_handlers.c
    ./src/dropt_string.c
)

add_library(dropt
    ${dropt_c_files}
    ${dropt_h_files}
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

target_include_directories(dropt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

set(dropt_example_h_files
    ./include/dropt.h
    ./include/dropt_string.h
)

set(dropt_example_c_files
    ./dropt_example.c
)

add_executable(dropt_example
    ${dropt_example_c_files}
    ${dropt_example_h_files}
)

target_link_libraries(dropt_example dropt)
