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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_INTERCEPTS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_INTERCEPTS_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include <array>
#include <cinttypes>

namespace hoover_chess_utils::pgn_reader
{

class Intercepts
{
private:
    static const std::array<std::array<SquareSet, 64U>, 64U> ctInterceptsTable;

public:
    // Returns the set of squares that intercepts a check. That is:
    // - checkerSq itself; AND
    // - If checkerSq is horiz/vert/diagonal to kingSq, then squares in between
    //
    // 0 0 0 0 0 0 0 0
    // 0 K 0 0 0 0 0 0
    // 0 0 1 0 0 0 0 0
    // 0 0 0 1 0 0 0 0
    // 0 0 0 0 1 0 0 0
    // 0 0 0 0 0 C 0 0
    // 0 0 0 0 0 0 0 0
    // 0 0 0 0 0 0 0 0
    static SquareSet getInterceptSquares(Square kingSq, Square checkerSq) noexcept
    {
        return ctInterceptsTable[getIndexOfSquare(kingSq)][getIndexOfSquare(checkerSq)];
    }
};

}

#endif
