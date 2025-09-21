// Hoover Chess Utilities / PGN reader
// Copyright (C) 2024-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_H_INCLUDED


#include "chessboard.h"

#include "bittricks.h"
#include "bitboard-attacks.h"
#include "bitboard-intercepts.h"
#include "chessboard-priv.h"
#include "pawn-lookups.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cinttypes>
#include <utility>
#include <tuple>

namespace hoover_chess_utils::pgn_reader
{

class MoveGenFunctionTables
{
private:
    static std::array<MoveGenFunctions, 3U> m_fns;

public:
    static inline const MoveGenFunctions &getFunctions(MoveGenType type) noexcept
    {
        const std::size_t index { static_cast<std::size_t>(type) };

        assert(index < m_fns.size());
        return m_fns[index];
    }
};



template <bool shortCastling>
struct CastlingSideSpecificsTempl;

template <>
struct CastlingSideSpecificsTempl<false>
{
    static constexpr std::uint8_t kingTargetColumn { 2U };
    static constexpr std::uint8_t rookTargetColumn { 3U };
};

template <>
struct CastlingSideSpecificsTempl<true>
{
    static constexpr std::uint8_t kingTargetColumn { 6U };
    static constexpr std::uint8_t rookTargetColumn { 5U };
};

inline constexpr MoveTypeAndPromotion pieceToTypeAndPromotion(Piece promotion) noexcept
{
    static_assert(
        (static_cast<unsigned>(Piece::KNIGHT) | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_KNIGHT));
    static_assert(
        (static_cast<unsigned>(Piece::BISHOP) | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_BISHOP));
    static_assert(
        (static_cast<unsigned>(Piece::ROOK)   | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_ROOK));
    static_assert(
        (static_cast<unsigned>(Piece::QUEEN)  | 0x08U) == static_cast<unsigned>(MoveTypeAndPromotion::PROMO_QUEEN));

    assert(promotion >= Piece::KNIGHT && promotion <= Piece::QUEEN);
    [[assume(promotion >= Piece::KNIGHT && promotion <= Piece::QUEEN)]];

    return static_cast<MoveTypeAndPromotion>(static_cast<uint16_t>(promotion) | 0x08U);
}

// Note about legal destinations during generateMoves().
//
// Legal destinations for a non-king move. Rules:
// - No checkers:
//   - any destination is legal (as long as the move is otherwise legal)
// - One checker:
//   - Checker must be captured; OR
//   - Check must be intercepted (ray attacks only)
// - Two checkers or more: (>= 3 cannot be reached legally)
//   - No legal destinations (king move is forced when in double-check)

struct AllLegalDestinationType
{
    constexpr SquareSet operator () () const noexcept
    {
        return SquareSet::all();
    }
};

struct ParametrizedLegalDestinationType
{
    SquareSet m_legalDestinations;

    constexpr ParametrizedLegalDestinationType(SquareSet legalDestinations) noexcept :
        m_legalDestinations(legalDestinations)
    {
    }

    ParametrizedLegalDestinationType(const ParametrizedLegalDestinationType &) = default;
    ParametrizedLegalDestinationType(ParametrizedLegalDestinationType &&) = default;
    ParametrizedLegalDestinationType &operator = (const ParametrizedLegalDestinationType &) & = default;
    ParametrizedLegalDestinationType &operator = (ParametrizedLegalDestinationType &&) & = default;
    ~ParametrizedLegalDestinationType() = default;

    constexpr SquareSet operator () () const noexcept
    {
        return m_legalDestinations;
    }
};

template <MoveGenType type>
inline SquareSet blocksAllChecksMaskTempl(Square kingSq, SquareSet checkers, Square dst) noexcept;

template <>
constexpr inline SquareSet blocksAllChecksMaskTempl<MoveGenType::NO_CHECK>(Square kingSq, SquareSet checkers, Square dst) noexcept
{
    static_cast<void>(kingSq);
    static_cast<void>(checkers);
    static_cast<void>(dst);

    return SquareSet::all(); // no checks to block
}

template <>
inline SquareSet blocksAllChecksMaskTempl<MoveGenType::CHECK>(Square kingSq, SquareSet checkers, Square dst) noexcept
{
    return
        (Intercepts::getInterceptSquares(kingSq, checkers.firstSquare()) & SquareSet::square(dst)).allIfAny();
}

}

#endif
