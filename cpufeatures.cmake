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


macro(cpufeatures_detect_bmi2)
    include(CheckSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

    check_source_compiles(CXX "
#include <immintrin.h>
int main(int argc, char **argv)
{
    return _pext_u64(argc, argc);
}
" HAVE_BMI2)
endmacro()


macro(cpufeatures_detect_avx512f)
    include(CheckSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

    check_source_compiles(CXX "
#include <immintrin.h>
int main(int argc, char **argv)
{
    __m512i tmp { };
    tmp = _mm512_rolv_epi64(tmp, tmp);
    static_cast<void>(tmp);
    return 0;
}
" HAVE_AVX512F)
endmacro()


macro(cpufeatures_detect_avx512vl)
    include(CheckSourceCompiles)

    set(CMAKE_REQUIRED_FLAGS ${CMAKE_CXX_FLAGS})

    check_source_compiles(CXX "
#include <immintrin.h>
int main(int argc, char **argv)
{
    __m128i tmp { };
    tmp = _mm_rolv_epi64(tmp, tmp);
    static_cast<void>(tmp);
    return 0;
}
" HAVE_AVX512VL)
endmacro()
