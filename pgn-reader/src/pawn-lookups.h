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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PAWN_LOOKUPS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PAWN_LOOKUPS_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"
#include "lookup-utils.h"

#include <array>
#include <cstdint>

namespace hoover_chess_utils::pgn_reader
{

template <Color side>
struct PawnLookups_SideSpecificsTempl;

template <>
struct PawnLookups_SideSpecificsTempl<Color::WHITE>
{
    static constexpr SquareSet rank1     { SquareSet::row(0U) };
    static constexpr SquareSet rank2     { SquareSet::row(1U) };
    static constexpr SquareSet rank3     { SquareSet::row(2U) };
    static constexpr SquareSet rank4     { SquareSet::row(3U) };
    static constexpr SquareSet rank5     { SquareSet::row(4U) };
    static constexpr SquareSet rank6     { SquareSet::row(5U) };
    static constexpr SquareSet rank7     { SquareSet::row(6U) };
    static constexpr SquareSet promoRank { SquareSet::row(7U) };

    static constexpr SquareSet pawnAdvance(SquareSet pawns) noexcept
    {
        return pawns << 8U;
    }

    static constexpr SquareSet captureLeft(SquareSet pawns) noexcept
    {
        return (pawns & ~SquareSet::column(0U)) << 7U;
    }

    static constexpr SquareSet captureRight(SquareSet pawns) noexcept
    {
        return (pawns & ~SquareSet::column(7U)) << 9U;
    }

    static constexpr Square captureLeftSq(Square sq) noexcept
    {
        assert(columnOf(sq) >= 1U);
        assert(rowOf(sq) <= 6U);
        return addToSquareNoOverflowCheck(sq, 7);
    }

    static constexpr Square captureRightSq(Square sq) noexcept
    {
        assert(columnOf(sq) <= 6U);
        assert(rowOf(sq) <= 6U);
        return addToSquareNoOverflowCheck(sq, 9);
    }

    static constexpr Square pawnRetreatSq(Square sq) noexcept
    {
        assert(sq >= Square::A3);
        return addToSquareNoOverflowCheck(sq, -8);
    }

    static constexpr Square pawnDoubleRetreatSq(Square sq) noexcept
    {
        assert(rowOf(sq) == 3U);
        return addToSquareNoOverflowCheck(sq, -16);
    }
};

template <>
struct PawnLookups_SideSpecificsTempl<Color::BLACK>
{
    static constexpr SquareSet rank1     { SquareSet::row(7U) };
    static constexpr SquareSet rank2     { SquareSet::row(6U) };
    static constexpr SquareSet rank3     { SquareSet::row(5U) };
    static constexpr SquareSet rank4     { SquareSet::row(4U) };
    static constexpr SquareSet rank5     { SquareSet::row(3U) };
    static constexpr SquareSet rank6     { SquareSet::row(2U) };
    static constexpr SquareSet rank7     { SquareSet::row(1U) };
    static constexpr SquareSet promoRank { SquareSet::row(0U) };

    static constexpr SquareSet pawnAdvance(SquareSet pawns) noexcept
    {
        return pawns >> 8U;
    }

    static constexpr SquareSet captureLeft(SquareSet pawns) noexcept
    {
        return (pawns & ~SquareSet::column(0U)) >> 9U;
    }

    static constexpr SquareSet captureRight(SquareSet pawns) noexcept
    {
        return (pawns & ~SquareSet::column(7U)) >> 7U;
    }

    static constexpr Square captureLeftSq(Square sq) noexcept
    {
        assert(columnOf(sq) >= 1U);
        assert(rowOf(sq) >= 1U);
        return addToSquareNoOverflowCheck(sq, -9);
    }

    static constexpr Square captureRightSq(Square sq) noexcept
    {
        assert(columnOf(sq) <= 6U);
        assert(rowOf(sq) >= 1U);
        return addToSquareNoOverflowCheck(sq, -7);
    }

    static constexpr Square pawnRetreatSq(Square sq) noexcept
    {
        assert(sq <= Square::H6);
        return addToSquareNoOverflowCheck(sq, +8);
    }

    static constexpr Square pawnDoubleRetreatSq(Square sq) noexcept
    {
        assert(rowOf(sq) == 4U);
        return addToSquareNoOverflowCheck(sq, +16);
    }
};

class PawnLookups
{
private:
    static constexpr std::array<SquareSet, 2U> ctRank8 {

        // white promotion dst square
        SquareSet::row(7U),

        // black promotion dst square
        SquareSet::row(0U),
    };

    static constexpr std::array<SquareSet, 2U> ctRank3 {
        // white double advancing pawns after single square retreat (rank 3)
        SquareSet::row(2U),

        // black double advancing pawns after single square retreat (rank 3)
        SquareSet::row(5U),
    };

    static constexpr std::array<std::int64_t, 2U> ctPawnAdvanceShift
    {
        8,
        -8
    };

    static constexpr std::array<SquareSet, 2U> ctAdvanceNoPromoLegalDstMask {
        // white single advancing pawns
        (SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U) | SquareSet::row(6U)),

        // black single advancing pawns
        (SquareSet::row(1U) | SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U)),
    };

public:
    static inline SquareSet singleAdvanceNoPromoLegalDstMask(Color turn) noexcept
    {
        return turnSpecificLookup(ctAdvanceNoPromoLegalDstMask, turn);
    }

    static inline SquareSet rank3(Color turn) noexcept
    {
        return turnSpecificLookup(ctRank3, turn);
    }

    static inline SquareSet rank8(Color turn) noexcept
    {
        return turnSpecificLookup(ctRank8, turn);
    }

    static inline std::int64_t pawnAdvanceShift(Color turn) noexcept
    {
        return turnSpecificLookup(ctPawnAdvanceShift, turn);
    }

};

}

#endif
