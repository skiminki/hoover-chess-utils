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

function setup_build
{
    BUILD_PREFIX="$1"
    shift

    if [ $# -ne 1 ]
    then
        echo "Usage: $0 <build-arch>"
        echo
        echo 'Build arch is one of:'
        echo '  generic           Generic (default) architecture.'
        echo '  native            Native architecture. This adds `-march=native'\'' to compiler flags.'
        echo '  custom:<flags>    Custom architecture. This adds `<flags>'\'' to compiler flags. For example:'
        echo '                    "custom:-march=haswell -mtune=znver4"'
        exit 1
    fi

    case "$1" in

        generic)
            BUILD_ARCH=generic
            MARCH_FLAGS=""
            ;;

        native)
            BUILD_ARCH=native
            MARCH_FLAGS="-march=native"
            ;;

        custom:*)
            BUILD_ARCH=custom
            MARCH_FLAGS="${1:7}"
            ;;

        *)
            echo "Unknown build arch: $1"
            exit 1
            ;;
    esac

    BUILD_DIR="build/${BUILD_PREFIX}-${BUILD_ARCH}"
}


function do_build
{
    CMAKE_BUILD_TYPE="$1"
    CXXFLAGS="${MARCH_FLAGS} $2"

    echo "==============================================================================="
    echo "Build output:     ${BUILD_DIR}"
    echo "CMake build type: ${CMAKE_BUILD_TYPE}"
    echo "CXX:              ${CXX:-<unset>}"
    echo "CXXFLAGS:         ${CXXFLAGS}"
    echo "==============================================================================="

    CXX="${CXX}" CXXFLAGS="${CXXFLAGS}" \
       cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"

    cmake --build "${BUILD_DIR}" --parallel
}
