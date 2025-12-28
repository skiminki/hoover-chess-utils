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


macro(cpufeatures_detect_aarch64)
    include(CheckSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

    check_source_compiles(CXX "
int main(int argc, char **argv)
{
#if !defined(__aarch64__)
    static_assert(false);
#endif
    return 0;
}
" HAVE_AARCH64)
endmacro()

macro(cpufeatures_detect_aarch64_sve2_bitperm)
    include(CheckSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

    check_source_compiles(CXX "
#include <arm_sve.h>

int main(int argc, char **argv)
{
    svuint64_t op1 { };
    svuint64_t op2 { };
    return svbdep_u64(op1, op2)[0];
}
" HAVE_AARCH64_SVE2_BITPERM)
endmacro()
