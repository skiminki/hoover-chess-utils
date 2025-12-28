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
include(cpufeatures-x86.cmake)
cpufeatures_detect_x86()
if (HAVE_X86)
  cpufeatures_detect_x86_bmi2()
  cpufeatures_detect_x86_avx512f()
  cpufeatures_detect_x86_avx512vl()
endif()

include(cpufeatures-aarch64.cmake)
cpufeatures_detect_aarch64()
if (HAVE_AARCH64)
  cpufeatures_detect_aarch64_sve2_bitperm()
endif()


##### The main Doxygen build
find_package(Doxygen COMPONENTS doxygen dot)


if(Doxygen_FOUND)
  set(DOXYGEN_ALIASES
      "coderef{1}=<tt>@ref \\1</tt>"
      "true=<tt>true</tt>"
      "false=<tt>false</tt>"
      "apionly{1}=\\1"
      "fullonly{1}="
      )

  set(DOXYGEN_AUTOLINK_SUPPORT NO)
  set(DOXYGEN_ENABLED_SECTIONS full)
  set(DOXYGEN_ENABLED_SECTIONS)
  set(DOXYGEN_EXTRACT_ALL NO)
  set(DOXYGEN_EXTRACT_PRIVATE NO)
  set(DOXYGEN_FILE_PATTERNS *.h *.dox)
  set(DOXYGEN_FULL_PATH_NAMES YES)
  set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_SOURCE_DIR}/pgn-reader/doc/chessboard-table.css)
  set(DOXYGEN_SHOW_ENUM_VALUES NO)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
  set(DOXYGEN_HTML_DYNAMIC_SECTIONS NO)

  doxygen_add_docs(
      hoover-chess-utils-docs
      ${CMAKE_SOURCE_DIR}/doc
      ${CMAKE_SOURCE_DIR}/pgn-reader/doc
      ${CMAKE_SOURCE_DIR}/pgn-reader/include)
endif()


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
