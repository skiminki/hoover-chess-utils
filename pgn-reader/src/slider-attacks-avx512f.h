// Hoover Chess Utilities / PGN reader
// Copyright (C) 2025  Sami Kiminki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef HOOVER_CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_AVX512F_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_AVX512F_H_INCLUDED

#include "chessboard-types-squareset.h"

#include <immintrin.h>
#include <array>
#include <cstdint>

namespace hoover_chess_utils::pgn_reader
{

struct SliderAttacksSimdAvx512f_Constants
{
    // the order of sliders:
    // - advancing top-left
    // - advancing top-right
    // - advancing bottom-left
    // - advancing bottom-right
    // - advancing top
    // - advancing bottom
    // - advancing left
    // - advancing right

    alignas(64) static constexpr std::array<std::uint64_t, 8U> attackingSliderMasks {
        static_cast<std::uint64_t>(~(SquareSet::row(7U) | SquareSet::column(0U))),
        static_cast<std::uint64_t>(~(SquareSet::row(7U) | SquareSet::column(7U))),
        static_cast<std::uint64_t>(~(SquareSet::row(0U) | SquareSet::column(0U))),
        static_cast<std::uint64_t>(~(SquareSet::row(0U) | SquareSet::column(7U))),
        static_cast<std::uint64_t>(~SquareSet::row(7U)),
        static_cast<std::uint64_t>(~SquareSet::row(0U)),
        static_cast<std::uint64_t>(~SquareSet::column(0U)),
        static_cast<std::uint64_t>(~SquareSet::column(7U)),
    };

    alignas(64) static constexpr std::array<std::int64_t, 64U> rotateLefts {
        +7,
        +9,
        -9,
        -7,
        +8,
        -8,
        -1,
        +1
    };
};


SquareSet SliderAttacksSimdAvx512f::getAttackedSquaresBySliders(const SquareSet bishops, const SquareSet rooks, const SquareSet occupancyMask) noexcept
{
    if ((bishops | rooks) == SquareSet::none())
        return SquareSet::none();

    const __m512i rotateLefts = _mm512_load_epi64(SliderAttacksSimdAvx512f_Constants::rotateLefts.data());
    __m512i attackingSliderMasks =  _mm512_load_epi64(SliderAttacksSimdAvx512f_Constants::attackingSliderMasks.data());

    __m512i attackingSliders { };

    attackingSliders =
        _mm512_mask_set1_epi64(
            _mm512_set1_epi64(static_cast<std::uint64_t>(bishops)),
            0xF0,
            static_cast<std::uint64_t>(rooks));

    const __m512i occupancyMasks = _mm512_set1_epi64(static_cast<std::uint64_t>(occupancyMask));
    __m512i attacks { };

    attackingSliders &= attackingSliderMasks;

    __mmask8 exitCond;

    do
    {
        attackingSliders  = _mm512_rolv_epi64(attackingSliders, rotateLefts);
        attacks          |= attackingSliders;
        attackingSliders  = attackingSliders &~ occupancyMasks;
        attackingSliders &= attackingSliderMasks;

        attackingSliders  = _mm512_rolv_epi64(attackingSliders, rotateLefts);
        attacks          |= attackingSliders;
        attackingSliders  = attackingSliders &~ occupancyMasks;

        exitCond = _mm512_test_epi64_mask(attackingSliders, attackingSliderMasks);

        attackingSliders &= attackingSliderMasks;
    }
    while (exitCond != 0U);

    return SquareSet { static_cast<std::uint64_t>(_mm512_reduce_or_epi64(attacks)) };
}

}

#endif
