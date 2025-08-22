#!/bin/bash
#
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

set -e
set -o pipefail
cd "$(dirname "$0")"

source scripts/build-functions.bash

# profiling runs require clang
CXX="${CXX:-clang++}"

setup_build coverage "$@"
do_build Debug "-DNDEBUG -fprofile-instr-generate -fcoverage-mapping -Og"

rm -f cov_tmp/hoover-pgn-reader-tests.profraw
LLVM_PROFILE_FILE="${BUILD_DIR}"/cov_tmp/hoover-pgn-reader-tests.profraw "${BUILD_DIR}"/pgn-reader/hoover-pgn-reader-tests "$@"
llvm-profdata merge -o "${BUILD_DIR}"/cov_tmp/hoover-pgn-reader-tests.profdata --sparse "${BUILD_DIR}"/cov_tmp/hoover-pgn-reader-tests.profraw

llvm-cov show -format=html -output-dir="${BUILD_DIR}"/coverage_html \
         --instr-profile "${BUILD_DIR}"/cov_tmp/hoover-pgn-reader-tests.profdata \
         --ignore-filename-regex='.*/extern/.*' \
         --ignore-filename-regex='.*/test/.*' \
         --ignore-filename-regex="${BUILD_DIR}"'/pgn-reader/pgnscanner\.cc' \
         --Xdemangler c++filt -Xdemangler -n \
         --show-branches=count \
         --show-expansions \
         "${BUILD_DIR}"/pgn-reader/hoover-pgn-reader-tests

llvm-cov report \
         --instr-profile "${BUILD_DIR}"/cov_tmp/hoover-pgn-reader-tests.profdata \
         --ignore-filename-regex='.*/extern/.*' \
         --ignore-filename-regex='.*/test/.*' \
         --ignore-filename-regex="${BUILD_DIR}"'/pgn-reader/pgnscanner\.cc' \
         --Xdemangler c++filt -Xdemangler -n \
         "${BUILD_DIR}"/pgn-reader/hoover-pgn-reader-tests

echo
echo "See file://${PWD}/${BUILD_DIR}/coverage_html/index.html for the full report."
