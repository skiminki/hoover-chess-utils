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


project(HooverChessUtils_PgnReader VERSION ${HOOVER_CHESS_UTILS_VERSION})

find_package(FLEX 2.6.0)
FLEX_TARGET(PgnScanner src/pgnscanner.l ${CMAKE_CURRENT_BINARY_DIR}/pgnscanner.cc)

configure_file(src/pgnreader-config.h.in pgnreader-config.h @ONLY)

##### Doxygen output
if(Doxygen_FOUND)
  set(DOXYGEN_ALIASES
      "coderef{1}=<tt>@ref \\1</tt>"
      "true=<tt>true</tt>"
      "false=<tt>false</tt>"
      "apionly{1}="
      "fullonly{1}=\\1"
      "pgnscannertoken{1}=<tt>&lt\;@link hoover_chess_utils::pgn_reader::PgnScannerToken::\\1 \\1@endlink&gt\;</tt>"
      )
  set(DOXYGEN_AUTOLINK_SUPPORT NO)
  set(DOXYGEN_ENABLED_SECTIONS full)
  set(DOXYGEN_EXTRACT_ALL YES)
  set(DOXYGEN_EXTRACT_PRIVATE YES)
  set(DOXYGEN_FILE_PATTERNS *.h *.dox)
  set(DOXYGEN_FULL_PATH_NAMES YES)
  set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/doc/chessboard-table.css)
  set(DOXYGEN_SHOW_ENUM_VALUES YES)
  set(DOXYGEN_STRIP_FROM_PATH ${CMAKE_SOURCE_DIR})
  set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)
  set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)

  doxygen_add_docs(
      pgn-reader-docs
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/doc/build-config.dox)


endif()

add_library(
  hoover-pgn-reader STATIC
  ${FLEX_PgnScanner_OUTPUTS}
  src/bitboard-attacks.cc
  src/bitboard-intercepts.cc
  src/chessboard-bitboard.cc
  src/chessboard-boardsetup.cc
  src/chessboard-loadfen.cc
  src/chessboard-movegen.cc
  src/chessboard-printboard.cc
  src/chessboard-types-squareset.cc
  src/chessboard.cc
  src/pawn-lookups.cc
  src/pgnparser.cc
  src/pgnreader-error.cc
  src/pgnreader-string-utils.cc
  src/pgnreader.cc
  src/position-compress-fixed.cc
  src/slider-attacks-generic.cc
  $<IF:$<BOOL:${HAVE_PDEP_PEXT}>,src/slider-attacks-pext-pdep.cc,>
  src/stringbuilder.cc
  )

target_include_directories(hoover-pgn-reader PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  )
target_include_directories(hoover-pgn-reader PRIVATE
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_BINARY_DIR}"
  )

##### PGN reader tests
enable_testing()
include(GoogleTest)
add_subdirectory("${PROJECT_SOURCE_DIR}/../extern/googletest" "extern/googletest")

add_custom_command(
  OUTPUT gen-scannertest-1.cc
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/test/scannertest-1.pgn scannertest-1.pgn.data
  COMMAND xxd -i scannertest-1.pgn.data gen-scannertest-1.cc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/scannertest-1.pgn VERBATIM)

add_custom_command(
  OUTPUT gen-parsertest-1.cc
  COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/test/parsertest-1.pgn parsertest-1.pgn.data
  COMMAND xxd -i parsertest-1.pgn.data gen-parsertest-1.cc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test/parsertest-1.pgn VERBATIM)

add_executable(hoover-pgn-reader-tests
  test/main.cc

  test/bitboard-attacks-test.cc
  test/bittricks-test.cc
  test/boardtest.cc
  test/chessboard-test-perft.cc
  test/chessboard-boardsetup-test.cc
  test/chessboard-loadfen-test.cc
  test/chessboard-movegen-test.cc
  test/chessboard-pinned-pieces-test.cc

  test/chessboard-test.cc
  test/chessboard-types-test.cc
  test/chessboard-types-squareset-test.cc
  test/pgnscannertest.cc
  test/pgnparsertest.cc
  test/pgnreader-error-test.cc
  test/pgnreader-error-recovery-test.cc
  test/pgnreader-test.cc
  test/pgnreader-string-utils-test.cc
  test/position-compress-fixed-test.cc
  test/stringbuilder-test.cc
  test/version-test.cc
  gen-scannertest-1.cc
  gen-parsertest-1.cc
  )

target_include_directories(hoover-pgn-reader-tests PRIVATE
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  )

target_link_libraries(hoover-pgn-reader-tests
  hoover-pgn-reader
  gtest_main)
include(GoogleTest)
gtest_discover_tests(hoover-pgn-reader-tests)

##### Board PERFT
add_executable(hoover-perft
  test/perft.cc
  )

target_link_libraries(hoover-perft
  hoover-pgn-reader)

##### PGN reader perf test suite
add_executable(hoover-pgn-reader-perf-tests
  test/pgnreaderperftest.cc
  )
target_include_directories(hoover-pgn-reader-perf-tests PUBLIC
  "${PROJECT_BINARY_DIR}"
  )
target_link_libraries(hoover-pgn-reader-perf-tests
  hoover-pgn-reader)
