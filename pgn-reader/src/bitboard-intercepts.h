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

#include "bitboard-tables.h"

#include <array>
#include <cinttypes>

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderImpl
/// @{

/// @brief Check interceptions and pin checks
class Intercepts
{
public:
    /// @brief Returns the set of squares that intercepts a check.
    ///
    /// @param[in] kingSq       King square
    /// @param[in] checkerSq    Checker square. May be @coderef{Square::NONE}.
    /// @return                 The set of intercept squares or @coderef{SquareSet::all()} when
    ///                         the checker is @coderef{Square::NONE}.
    ///
    /// The set of squares that intercept a check are:
    /// - @p checkerSq (assume captured)
    /// - If @p checkerSq is along a ray direction from @p kingSq, then all squares in between
    ///   (but not including @p kingSq)
    ///
    /// **Illustration**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td>K</td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td class="mask">C</td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// Where:
    /// - @c K = the checked king
    /// - @c C = the checker
    /// - shaded = the intercept squares
    static inline SquareSet getInterceptSquares(Square kingSq, Square checkerSq) noexcept
    {
        assert(checkerSq <= Square::NONE);
        return SquareSet { ctBitBoardTables.rayIntercepts[static_cast<std::size_t>(checkerSq)][getIndexOfSquare(kingSq)] };
    }

    /// @brief Returns a ray from king square to the direction of pinned piece square.
    ///
    /// @param[in]  kingSq       King square
    /// @param[in]  pinnedSq     Square of the pinned piece
    ///
    /// @pre (Asserted) @c kingSq &rarr; @c pinnedSq must be on the same rank, file,
    /// diagonal, or anti-diagonal.
    ///
    /// The ray represents the squares that are valid for a pinned piece, as
    /// pinned piece can only be moved along the pin axis.
    static inline SquareSet getRay(Square kingSq, Square pinnedSq) noexcept
    {
        assert(isValidSquare(kingSq));
        assert(isValidSquare(pinnedSq));

        const SquareSet ray { ctBitBoardTables.raysFromKing[getIndexOfSquare(kingSq)][getIndexOfSquare(pinnedSq)] };
        assert(ray != SquareSet { });
        return ray;
    }

    template <bool pinned>
    static inline SquareSet getPinRestiction(Square kingSq, Square pinnedSq) noexcept
    {
        if constexpr (pinned)
            return getRay(kingSq, pinnedSq);
        else
            return SquareSet::all();
    }
};

/// @}

}

#endif
