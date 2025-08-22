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

#include <algorithm>
#include <array>
#include <bit>
#include <cinttypes>
#include <utility>
#include <tuple>

namespace hoover_chess_utils::pgn_reader
{

template <Color side>
struct SideSpecificsTempl;

template <>
struct SideSpecificsTempl<Color::WHITE>
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
struct SideSpecificsTempl<Color::BLACK>
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



template <typename IteratorType>
IteratorType ChessBoard::addMoveIfLegalKing(
    IteratorType i,
    Square src,
    Square dst) const noexcept
{
    if (isLegalKingMove(src, dst, getTurn()))
    {
        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
        ++i;
    }

    return i;
}

template <typename IteratorType>
IteratorType ChessBoard::addMoveIfLegalRegular(
    IteratorType i,
    Square src,
    Square dst,
    MoveTypeAndPromotion typeAndPromo) const noexcept
{
    if (isLegalRegularMove(src, dst))
    {
        *i = Move { src, dst, typeAndPromo };
        ++i;
    }

    return i;
}

template <typename IteratorType>
IteratorType ChessBoard::addMoveIfLegalPromo(
    IteratorType i,
    Square src,
    Square dst,
    Piece promo) const noexcept
{
    if (isLegalRegularMove(src, dst))
    {
        *i = Move { src, dst, pieceToTypeAndPromotion(promo) };
        ++i;
    }

    return i;
}

template <typename IteratorType>
IteratorType ChessBoard::addMoveIfLegalEp(
    IteratorType i,
    Square src,
    Square dst,
    Square ep) const noexcept
{
    if (isLegalEpMove(src, dst, ep))
    {
        *i = Move { src, dst, MoveTypeAndPromotion::EN_PASSANT };
        ++i;
    }

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType, Color turn>
auto ChessBoard::generateMovesForPawnsTempl(
    IteratorType i,
    ParamType legalDestinations) const noexcept -> IteratorType
{
    using SideSpecifics = SideSpecificsTempl<turn>;

    const SquareSet pawns { m_pawns & m_turnColorMask };

    // destination squares for advancing pawns
    const SquareSet advanceMask { SideSpecifics::pawnAdvance(pawns) & ~m_occupancyMask };

    // destination squares for double-advancing pawns
    const SquareSet doubleAdvanceMask { SideSpecifics::pawnAdvance(advanceMask & SideSpecifics::rank3) & ~m_occupancyMask };

    SQUARESET_ENUMERATE(
        dst,
        advanceMask & ~SideSpecifics::promoRank & legalDestinations(),
        i = addMoveIfLegalRegular(
            i,
            SideSpecifics::pawnRetreatSq(dst),
            dst,
            MoveTypeAndPromotion::REGULAR_PAWN_MOVE));

    SQUARESET_ENUMERATE(
        dst,
        doubleAdvanceMask & legalDestinations(),
        i = addMoveIfLegalRegular(
            i,
            SideSpecifics::pawnDoubleRetreatSq(dst),
            dst,
            MoveTypeAndPromotion::REGULAR_PAWN_MOVE));

    SQUARESET_ENUMERATE(
        dst,
        advanceMask & SideSpecifics::promoRank & legalDestinations(),
        {
            const Square src { SideSpecifics::pawnRetreatSq(dst) };

            if (isLegalRegularMove(src, dst))
            {
                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::QUEEN) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::ROOK) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::BISHOP) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::KNIGHT) };
                ++i;
            }
        });

    const SquareSet oppPieces { (m_occupancyMask ^ m_turnColorMask) };
    const SquareSet leftCapturingPawns {
        Attacks::getPawnAttackersMask<turn, false>(oppPieces & legalDestinations()) & pawns };
    const SquareSet rightCapturingPawns {
        Attacks::getPawnAttackersMask<turn, true>(oppPieces & legalDestinations()) & pawns };

    // capture left

    SQUARESET_ENUMERATE(
        src,
        leftCapturingPawns & ~SideSpecifics::rank7,
        {
            i = addMoveIfLegalRegular(
                i,
                src,
                SideSpecifics::captureLeftSq(src),
                MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE);
        });

    // capture right
    SQUARESET_ENUMERATE(
        src,
        rightCapturingPawns & ~SideSpecifics::rank7,
        {
            i = addMoveIfLegalRegular(
                i,
                src,
                SideSpecifics::captureRightSq(src),
                MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE);
        });

    // capture left + promo
    SQUARESET_ENUMERATE(
        src,
        leftCapturingPawns & SideSpecifics::rank7,
        {
            const Square dst { SideSpecifics::captureLeftSq(src) };

            if (isLegalRegularMove(src, dst))
            {
                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                ++i;
            }
        });

    // capture right + promo
    SQUARESET_ENUMERATE(
        src,
        rightCapturingPawns & SideSpecifics::rank7,
        {
            const Square dst { SideSpecifics::captureRightSq(src) };

            if (isLegalRegularMove(src, dst))
            {
                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                ++i;

            }
        });

    // EP captures
    if (m_epSquare <= Square::H8)
    {
        const Square epPawn { SideSpecifics::pawnRetreatSq(m_epSquare) };

        SQUARESET_ENUMERATE(
            src,
            Attacks::getPawnAttackerMask(m_epSquare, turn) & pawns,
            {
                i = addMoveIfLegalEp(i, Square { src }, m_epSquare, epPawn);
            });
    }

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType>
auto ChessBoard::generateMovesForPawns(
    IteratorType i,
    ParamType legalDestinations) const noexcept -> IteratorType
{
    if (getTurn() == Color::WHITE)
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::WHITE>(i, legalDestinations);
    else
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::BLACK>(i, legalDestinations);
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType>
auto ChessBoard::generateMovesForKnight(
    IteratorType i,
    Square sq,
    ParamType legalDestinations) const noexcept -> IteratorType
{
    // occup  turnColor    valid
    //     0          0        1
    //     0          1        1
    //     1          0        1
    //     1          1        0
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    SQUARESET_ENUMERATE(
        dst,
        Attacks::getKnightAttackMask(sq) & emptyOrCapture & legalDestinations(),
        i = addMoveIfLegalRegular(i, sq, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE));

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType>
auto ChessBoard::generateMovesForBishop(
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) const noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    SQUARESET_ENUMERATE(
        dst,
        Attacks::getBishopAttackMask(sq, m_occupancyMask) & emptyOrCapture & legalDestinations(),
        i = addMoveIfLegalRegular(i, sq, dst, typeAndPromo));

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType>
auto ChessBoard::generateMovesForRook(
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) const noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    SQUARESET_ENUMERATE(
        dst,
        Attacks::getRookAttackMask(sq, m_occupancyMask) & emptyOrCapture & legalDestinations(),
        i = addMoveIfLegalRegular(i, sq, dst, typeAndPromo));

    return i;
}

template <typename IteratorType>
auto ChessBoard::generateMovesForKing(
    IteratorType i,
    Square sq) const noexcept -> IteratorType
{
    // TODO: it may be faster to determine a "minefield" for the king before adding moves
    // instead of trying moves and determining attackers like addMoveIfLegalKing() does

    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    SQUARESET_ENUMERATE(
        dst,
        Attacks::getKingAttackMask(sq) & emptyOrCapture,
        i = addMoveIfLegalKing(i, sq, dst));

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, bool shortCastling>
auto ChessBoard::generateMovesForCastling(
    IteratorType i) const noexcept -> IteratorType
{
    // When in check, castling is not legal. Since the caller is supposed to
    // classify the position before calling this function, we'll just ensure
    // here that we're not being called when in check.
    static_assert(type == MoveGenType::NO_CHECK);

    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    using CastlingSideSpecifics = CastlingSideSpecificsTempl<shortCastling>;

    const Square sqKing { m_kingSq };
    Square sqKingTarget;
    Square sqRookTarget;

    const Color turn { getTurn() };
    const Square sqRook { getCastlingRook(turn, shortCastling) };

    // do we have rights to castle?
    if (sqRook == Square::NONE)
        return i;

    sqKingTarget = makeSquare(CastlingSideSpecifics::kingTargetColumn, (static_cast<std::uint8_t>(turn) / 8U) * 7U);
    sqRookTarget = makeSquare(CastlingSideSpecifics::rookTargetColumn, (static_cast<std::uint8_t>(turn) / 8U) * 7U);

    const std::uint8_t sqNumKing { static_cast<std::uint8_t>(sqKing) };
    const std::uint8_t sqNumRook { static_cast<std::uint8_t>(sqRook) };
    const std::uint8_t sqNumKingTarget { static_cast<std::uint8_t>(sqKingTarget) };
    const std::uint8_t sqNumRookTarget { static_cast<std::uint8_t>(sqRookTarget) };

    const SquareSet kingPathHalfOpen { BitTricks::rangeHalfOpen(sqNumKing, sqNumKingTarget) };
    const SquareSet rookPathHalfOpen { BitTricks::rangeHalfOpen(sqNumRook, sqNumRookTarget) };

    const SquareSet requiredEmptySquares {
        (kingPathHalfOpen | rookPathHalfOpen | SquareSet::square(sqKingTarget) | SquareSet::square(sqRookTarget)) &
        ~(SquareSet::square(sqKing) | SquareSet::square(sqRook)) };

    if ((requiredEmptySquares & m_occupancyMask) != SquareSet::none())
        return i;

    // The king's initial square does not need to be checked, since the king
    // cannot be checked when we enter this function. However, the final square
    // must be checked even when the king doesn't move (FRC), since it's
    // possible that the rook was pinned.
    SquareSet sqMask { (kingPathHalfOpen & ~SquareSet::square(sqKing)) | SquareSet::square(sqKingTarget) };

    SQUARESET_ENUMERATE(
        sq, sqMask,
        if (determineAttackers(
                m_occupancyMask & (~SquareSet::square(sqRook)),
                m_turnColorMask,
                m_pawns,
                m_knights,
                m_bishops,
                m_rooks & (~SquareSet::square(sqRook)),
                m_kings,
                sq,
                turn) != SquareSet::none())
            return i);

    if constexpr (shortCastling)
        *i = Move { sqKing, sqRook, MoveTypeAndPromotion::CASTLING_SHORT };
    else
        *i = Move { sqKing, sqRook, MoveTypeAndPromotion::CASTLING_LONG };
    ++i;

    return i;
}

template <typename IteratorType, ChessBoard::MoveGenType type, typename ParamType>
IteratorType ChessBoard::generateMovesTempl(
    IteratorType i,
    ParamType legalDestinations) const noexcept
{
    // if we're in check, we'll try the king moves first
    if constexpr (type != MoveGenType::NO_CHECK)
    {
        i = generateMovesForKing<IteratorType>(i, m_kingSq);
        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;
    }

    // in double-check, king moves are the only viable moves, so return here
    if constexpr (type == MoveGenType::DOUBLE_CHECK)
        return i;
    else
    {

        i = generateMovesForPawns<IteratorType, type, ParamType>(i, legalDestinations);

        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;

        SQUARESET_ENUMERATE(
            sq,
            (m_knights & m_turnColorMask),
            i = generateMovesForKnight<IteratorType, type, ParamType>(i, sq, legalDestinations));

        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;

        SQUARESET_ENUMERATE(
            sq,
            m_bishops & m_turnColorMask,
            {
                const MoveTypeAndPromotion typeAndPromo {
                    (m_rooks & SquareSet::square(sq)) != SquareSet::none() ?
                    MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                    MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
                };
                i = generateMovesForBishop<IteratorType, type, ParamType>(i, sq, legalDestinations, typeAndPromo);
            });

        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;

        SQUARESET_ENUMERATE(
            sq,
            m_rooks & m_turnColorMask,
            {
                const MoveTypeAndPromotion typeAndPromo {
                    (m_bishops & SquareSet::square(sq)) != SquareSet::none() ?
                    MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                    MoveTypeAndPromotion::REGULAR_ROOK_MOVE
                };
                i = generateMovesForRook<IteratorType, type, ParamType>(i, sq, legalDestinations, typeAndPromo);
            });

        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;

        // In no check, we'll do the king and castling moves last. This helps the
        // LegalMoveDetectorIterator with early exit, since these are a bit more
        // expensive than the other moves,
        if constexpr (type == MoveGenType::NO_CHECK)
        {
            i = generateMovesForKing<IteratorType>(i, m_kingSq);

            if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
                if (i.hasLegalMoves())
                    return i;

            i = generateMovesForCastling<IteratorType, type, false>(i);
            return generateMovesForCastling<IteratorType, type, true>(i);
        }
        else
            return i;
    }
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesIteratorTempl(
    IteratorType i) const noexcept
{
    const std::uint8_t numCheckers { m_checkers.popcount() };
    if (numCheckers == 0U) [[likely]]
    {
        // all destinations are ok as long as the move is ok
        i = generateMovesTempl<IteratorType, MoveGenType::NO_CHECK, AllLegalDestinationType>(
            i, AllLegalDestinationType { });
    }
    else [[unlikely]]
    {
        if (numCheckers == 1U) [[likely]]
        {
            // must capture the checker or block the check
            i = generateMovesTempl<IteratorType, MoveGenType::CHECK, ParametrizedLegalDestinationType>(
                i,
                ParametrizedLegalDestinationType {
                    Intercepts::getInterceptSquares(m_kingSq, m_checkers.firstSquare()) });
        }
        else [[unlikely]]
        {
            // king moves only
            i = generateMovesTempl<IteratorType, MoveGenType::DOUBLE_CHECK, NoLegalDestinationType>(
                i, NoLegalDestinationType { });
        }
    }

    return i;
}

}

#endif
