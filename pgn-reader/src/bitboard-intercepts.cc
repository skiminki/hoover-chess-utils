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

#include "bitboard-intercepts.h"

#include "chessboard-types-squareset.h"

#include <algorithm>
#include <cstddef>

namespace hoover_chess_utils::pgn_reader
{

namespace
{

consteval bool isRay(std::int8_t dx, std::int8_t dy) noexcept
{
    return (dx == 0 || dy == 0 || dx == dy || dx == -dy);
}

consteval std::int8_t clampDifference(std::int8_t diff) noexcept
{
    if (diff < 0)
        return -1;
    if (diff > 0)
        return +1;

    return 0;
}

consteval auto computeIntercept(Square kingSq, Square checkerSq) noexcept
{
    if (kingSq == checkerSq)
        return SquareSet::none();

    const std::int8_t dx = columnOf(checkerSq) - columnOf(kingSq);
    const std::int8_t dy = rowOf(checkerSq) - rowOf(kingSq);

    if (isRay(dx, dy))
    {
        const std::int8_t stepx = clampDifference(dx);
        const std::int8_t stepy = clampDifference(dy);
        const std::int8_t shift = stepy * 8 + stepx;

        Square sq { kingSq };
        SquareSet intercept { };

        do
        {
            sq = addToSquareNoOverflowCheck(sq, shift);

            intercept |= SquareSet::square(sq);
        }
        while (sq != checkerSq);

        return intercept;
    }
    else
    {
        // knight jump or garbage
        return SquareSet::square(checkerSq);
    }
}

static_assert(computeIntercept(Square::A1, Square::A1) == SquareSet::none());
static_assert(computeIntercept(Square::A1, Square::A2) == SquareSet::square(Square::A2));
static_assert(computeIntercept(Square::A1, Square::A8) == SquareSet { 0x01'01'01'01'01'01'01'00U } );
static_assert(computeIntercept(Square::A1, Square::H8) == SquareSet { 0x80'40'20'10'08'04'02'00U } );
static_assert(computeIntercept(Square::A1, Square::B3) == SquareSet::square(Square::B3));

consteval auto computeInterceptsTable() noexcept
{
    std::array<std::array<SquareSet, 64U>, 64U> ret { };

    for (std::size_t kingSq { }; kingSq < 64U; ++kingSq)
        for (std::size_t checkerSq { }; checkerSq < 64U; ++checkerSq)
            ret[kingSq][checkerSq] = computeIntercept(getSquareForIndex(kingSq), getSquareForIndex(checkerSq));

    return ret;
}

}

const std::array<std::array<SquareSet, 64U>, 64U> Intercepts::ctInterceptsTable { computeInterceptsTable() };

}
