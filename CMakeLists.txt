# Hoover Chess Utilities / PGN reader
# Copyright (C) 2025  Sami Kiminki
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 3.28)

project(HooverChessUtils VERSION 0.9.0)
set(HOOVER_CHESS_UTILS_VERSION_SUFFIX "-dev")

set(HOOVER_CHESS_UTILS_VERSION ${PROJECT_VERSION})
set(HOOVER_CHESS_UTILS_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(HOOVER_CHESS_UTILS_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(HOOVER_CHESS_UTILS_VERSION_PATCH ${PROJECT_VERSION_PATCH})

##### Basic compiler options
add_compile_options(-Wall -Wextra -pedantic -Werror)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

##### CPU features
include(cpufeatures.cmake)
cpufeatures_detect_bmi2()
cpufeatures_detect_avx512f()
cpufeatures_detect_avx512vl()

set(HAVE_PDEP_PEXT ${HAVE_BMI2})

##### Subdirectories
add_subdirectory(pgn-reader)
add_subdirectory(utils)


##### IPO/LTO support

include(CheckIPOSupported)
check_ipo_supported(RESULT isIPOsupported OUTPUT isIPOsupportedError)

if (NOT isIPOsupported)
    message(STATUS "IPO / LTO not supported: <${isIPOsupportedError}>")
endif()

if (isIPOsupported AND NOT (CMAKE_BUILD_TYPE STREQUAL Debug))
    message(STATUS "IPO / LTO enabled")
    set_property(TARGET hoover-pgn-reader PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-pgn-reader-tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-pgn-reader-perf-tests PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-perft PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-compactify-tcec-pgn PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-process-full-tcec-pgn PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET hoover-tdb-query PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO disabled")
endif()
