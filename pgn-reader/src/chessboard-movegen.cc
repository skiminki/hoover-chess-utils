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

#include "chessboard.h"

#include "bittricks.h"
#include "bitboard-attacks.h"
#include "bitboard-intercepts.h"
#include "chessboard-movegen.h"
#include "chessboard-priv.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cinttypes>
#include <utility>
#include <tuple>


namespace hoover_chess_utils::pgn_reader
{

bool ChessBoard::isLegalKingMove(const Square src, const Square dst, const Color turn) const noexcept
{
    const SquareSet kingSqBit { SquareSet::square(src) };

    return Attacks::determineAttackers(
        m_occupancyMask & ~kingSqBit,
        m_turnColorMask & ~kingSqBit,
        m_pawns,
        m_knights,
        m_bishops,
        m_rooks,
        m_kings & ~kingSqBit,
        dst,
        turn) == SquareSet::none();
}

std::size_t ChessBoard::generateMoves(MoveList &moves) const noexcept
{
    const MoveList::iterator i { generateMovesIteratorTempl(moves.begin()) };
    return i - moves.begin();
}

ChessBoard::Move ChessBoard::generateSingleMoveForPawnAndDestNoCapture(SquareSet srcSqMask, Square dst) const noexcept
{
    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~m_occupancyMask & blocksAllChecksMask(dst) };

    const SquareSet allowedDstSqMask {
        (SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U) | SquareSet::row(6U))
        >> static_cast<std::uint8_t>(turn) };

    const std::int8_t pawnAdvanceShift = 8 - 2 * static_cast<int8_t>(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (singleAdvancingPawnSquare &~ m_occupancyMask).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (pinCheck(src, dst)) [[likely]]
        {
            return Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
        }
    }

    return Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForPawnAndDestNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~m_occupancyMask & blocksAllChecksMask(dst) };

    const SquareSet allowedDstSqMask {
        (SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U) | SquareSet::row(6U))
        >> static_cast<std::uint8_t>(turn) };

    const std::int8_t pawnAdvanceShift = 8 - 2 * static_cast<int8_t>(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (singleAdvancingPawnSquare &~ m_occupancyMask).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (pinCheck(src, dst)) [[likely]]
        {
            moves[0U] = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
            return 1U;
        }
    }

    return 0U;
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForPawnAndDestCaptureTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    // ctPawnAttackerMasks, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };

    if (dst != m_epSquare) [[likely]]
    {
        const SquareSet dstSqBit { SquareSet::square(dst) };
        constexpr SquareSet dstSqMask { 0x00'FF'FF'FF'FF'FF'FF'00 };

        SquareSet dstOkMask {
            (blocksAllChecksMask(dst) & m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

        SQUARESET_ENUMERATE(
            src,
            dstOkMask &
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (pinCheck(src, dst))
                {
                    *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE };
                    ++i;
                }
            });
    }
    else
    {
        const int retractOneAdd { -8 + (2 * static_cast<int>(turn)) }; // pawn step backwards
        const Square epPawn { addToSquareNoOverflowCheck(dst, retractOneAdd) };

        SquareSet checkResolvedOk { (m_checkers &~ SquareSet::square(epPawn)).allIfNone() };

        SQUARESET_ENUMERATE(
            src,
            checkResolvedOk &
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            {
                const SquareSet exposedHorizLine {
                    SliderAttacksGeneric::getHorizRookAttackMask(
                        epPawn,
                        m_occupancyMask &~ SquareSet::square(src)) };

                const SquareSet kingBit { m_kings & m_turnColorMask };
                const SquareSet oppRooks { m_rooks & ~m_turnColorMask };

                if (pinCheck(src, m_epSquare) &&
                    ((kingBit & exposedHorizLine) == SquareSet::none() ||
                     (oppRooks & exposedHorizLine) == SquareSet::none()))
                {
                    *i = Move { src, m_epSquare, MoveTypeAndPromotion::EN_PASSANT };
                    ++i;
                }
            });
    }

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForPawnAndDestCapture(SquareSet srcSqMask, Square dst) const noexcept
{
    return generateMovesForPawnAndDestCaptureTempl(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

std::size_t ChessBoard::generateMovesForPawnAndDestCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    MoveList::iterator i { generateMovesForPawnAndDestCaptureTempl(moves.begin(), srcSqMask, dst) };

    return i - moves.begin();
}

ChessBoard::Move ChessBoard::generateSingleMoveForPawnAndDestPromoNoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~m_occupancyMask & blocksAllChecksMask(dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };
    const std::int8_t pawnAdvanceShift = 8 - 2 * static_cast<int8_t>(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (pinCheck(src, dst)) [[likely]]
        {
            return Move { src, dst, pieceToTypeAndPromotion(promo) };
        }
    }

    return Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForPawnAndDestPromoNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~m_occupancyMask & blocksAllChecksMask(dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };
    const std::int8_t pawnAdvanceShift = 8 - 2 * static_cast<int8_t>(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (pinCheck(src, dst)) [[likely]]
        {
            moves[0U] = Move { src, dst, pieceToTypeAndPromotion(promo) };
            return 1U;
        }
    }

    return 0U;
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl(IteratorType i, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    // ctPawnAttackerMasks, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };

    const SquareSet dstSqBit { SquareSet::square(dst) };
    const SquareSet dstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };

    SquareSet dstOkMask {
        (blocksAllChecksMask(dst) & m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

    SQUARESET_ENUMERATE(
        src,
        dstOkMask &
        srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
        {
            if (pinCheck(src, dst))
            {
                *i = Move { src, dst, pieceToTypeAndPromotion(promo) };
                ++i;
            }
        });

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForPawnAndDestPromoCapture(SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    return generateMovesForPawnAndDestPromoCaptureTempl(SingleMoveIterator { }, srcSqMask, dst, promo).getMove();
}

std::size_t ChessBoard::generateMovesForPawnAndDestPromoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    MoveList::iterator i { generateMovesForPawnAndDestPromoCaptureTempl(moves.begin(), srcSqMask, dst, promo) };
    return i - moves.begin();
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForKnightAndDestTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMask(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & m_turnColorMask & m_knights & srcSqMask & Attacks::getKnightAttackMask(dst) &~ m_pinnedPieces),
        {
            *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE };
            ++i;
        });

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForKnightAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return generateMovesForKnightAndDestTempl(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

std::size_t ChessBoard::generateMovesForKnightAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    MoveList::iterator i { generateMovesForKnightAndDestTempl(moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForBishopAndDestTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    const SquareSet pieces {
        Attacks::getBishopAttackMask(dst, m_occupancyMask)
        & srcSqMask & m_turnColorMask & m_bishops & (~m_rooks)
    };

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMask(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & pieces),
        {
            if (pinCheck(src, dst))
            {
                *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE };
                ++i;
            }
        });

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForBishopAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return generateMovesForBishopAndDestTempl(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

std::size_t ChessBoard::generateMovesForBishopAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    MoveList::iterator i { generateMovesForBishopAndDestTempl(moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForRookAndDestTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    const SquareSet pieces {
        Attacks::getRookAttackMask(dst, m_occupancyMask)
        & srcSqMask & m_turnColorMask & (~m_bishops) & m_rooks
    };

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMask(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & pieces),
        {
            if (pinCheck(src, dst))
            {
                *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_ROOK_MOVE };
                ++i;
            }
        });

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForRookAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return generateMovesForRookAndDestTempl(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

std::size_t ChessBoard::generateMovesForRookAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    MoveList::iterator i { generateMovesForRookAndDestTempl(moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForQueenAndDestTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    const SquareSet pieces {
        (Attacks::getBishopAttackMask(dst, m_occupancyMask) |
         Attacks::getRookAttackMask(dst, m_occupancyMask))
        & srcSqMask & m_turnColorMask & m_bishops & m_rooks
    };

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMask(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & pieces),
        {
            if (pinCheck(src, dst))
            {
                *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE };
                ++i;
            }
        });

    return i;
}

ChessBoard::Move ChessBoard::generateSingleMoveForQueenAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    return generateMovesForQueenAndDestTempl(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

std::size_t ChessBoard::generateMovesForQueenAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    MoveList::iterator i { generateMovesForQueenAndDestTempl(moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

ChessBoard::Move ChessBoard::generateSingleMoveForKingAndDest(SquareSet srcSqMask, Square dst) const noexcept
{
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if (((m_turnColorMask & dstSqBit) == SquareSet::none()) &&
        (srcSqMask & SquareSet::square(m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none() &&
        isLegalKingMove(m_kingSq, dst, getTurn()))
    {
        return Move { m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
    }

    return Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForKingAndDest(ShortMoveList &moves, SquareSet srcSqMask, Square dst) const noexcept
{
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if (((m_turnColorMask & dstSqBit).allIfNone() &
         srcSqMask & SquareSet::square(m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none() &&
        isLegalKingMove(m_kingSq, dst, getTurn()))
    {
        moves[0] = Move { m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
        return 1U;
    }

    return 0U;
}

ChessBoard::Move ChessBoard::generateSingleMoveForLongCastling() const noexcept
{
    // long castling requested
    if (m_checkers == SquareSet::none())
    {
        const SquareSet attackedSquares {
            Attacks::determineAttackedSquares(
                m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
                m_pawns &~ m_turnColorMask,
                m_knights &~ m_turnColorMask,
                m_bishops &~ m_turnColorMask,
                m_rooks &~ m_turnColorMask,
                m_oppKingSq,
                getTurn()) };

        return generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, false>(SingleMoveIterator { }, attackedSquares).getMove();
    }

    return ChessBoard::Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForLongCastling(ShortMoveList &moves) const noexcept
{
    // long castling requested
    ShortMoveList::iterator i { moves.begin() };

    if (m_checkers == SquareSet::none())
    {
        const SquareSet attackedSquares {
            Attacks::determineAttackedSquares(
                m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
                m_pawns &~ m_turnColorMask,
                m_knights &~ m_turnColorMask,
                m_bishops &~ m_turnColorMask,
                m_rooks &~ m_turnColorMask,
                m_oppKingSq,
                getTurn()) };

        i = generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, false>(i, attackedSquares);
    }

    return i - moves.begin();
}

ChessBoard::Move ChessBoard::generateSingleMoveForShortCastling() const noexcept
{
    // long castling requested
    if (m_checkers == SquareSet::none())
    {
        const SquareSet attackedSquares {
            Attacks::determineAttackedSquares(
                m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
                m_pawns &~ m_turnColorMask,
                m_knights &~ m_turnColorMask,
                m_bishops &~ m_turnColorMask,
                m_rooks &~ m_turnColorMask,
                m_oppKingSq,
                getTurn()) };

        return generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, true>(SingleMoveIterator { }, attackedSquares).getMove();
    }

    return ChessBoard::Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForShortCastling(ShortMoveList &moves) const noexcept
{
    // short castling requested
    ShortMoveList::iterator i { moves.begin() };

    if (m_checkers == SquareSet::none())
    {
        const SquareSet attackedSquares {
            Attacks::determineAttackedSquares(
                m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
                m_pawns &~ m_turnColorMask,
                m_knights &~ m_turnColorMask,
                m_bishops &~ m_turnColorMask,
                m_rooks &~ m_turnColorMask,
                m_oppKingSq,
                getTurn()) };

        i = generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, true>(i, attackedSquares);
    }

    return i - moves.begin();
}

}
