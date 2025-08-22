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

SquareSet ChessBoard::determineAttackers(
    const SquareSet occupancyMask,
    const SquareSet turnColorMask,
    const SquareSet pawns,
    const SquareSet knights,
    const SquareSet bishops,
    const SquareSet rooks,
    const SquareSet kings,
    const Square sq,
    const Color turn) noexcept
{
    const SquareSet opponentPieces { occupancyMask & ~turnColorMask };
    SquareSet attackers { };

    // pawn attackers
    attackers |=
        Attacks::getPawnAttackMask(sq, turn) &
        opponentPieces & pawns;

    // king
    attackers |= Attacks::getKingAttackMask(sq) & opponentPieces & kings;

    // knights
    attackers |= Attacks::getKnightAttackMask(sq) & opponentPieces & knights;

    // rooks and queens
    const SquareSet horizVertHits { Attacks::getRookAttackMask(sq, occupancyMask) };
    attackers |= horizVertHits & opponentPieces & rooks;

    // bishops and queens
    const SquareSet diagHits { Attacks::getBishopAttackMask(sq, occupancyMask) };
    attackers |= diagHits & opponentPieces & bishops;

    return attackers;
}

bool ChessBoard::isLegalKingMove(const Square src, const Square dst, const Color turn) const noexcept
{
    const SquareSet kingSqBit { SquareSet::square(src) };

    return determineAttackers(
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

bool ChessBoard::isLegalRegularMove(const Square src, const Square dst) const noexcept
{
    const SquareSet dstBit { SquareSet::square(dst) };

    // check if there are any checkers that weren't captured and can't be blocked (knights, pawns)
    SquareSet checkers { m_checkers & ~(dstBit | m_rooks | m_bishops) };
    if (checkers != SquareSet::none())
        return false;

    // update occupancy and turn color mask as per the move
    const SquareSet occupancyMask { (m_occupancyMask | dstBit) & (~SquareSet::square(src)) };
    const SquareSet turnColorMask { m_turnColorMask | dstBit };
    const SquareSet opponentPieces { occupancyMask & ~turnColorMask };

    // now check if any slider checkers remain (all blocked, none revealed)
    if ((((Attacks::getRookAttackMask(m_kingSq, occupancyMask) & m_rooks) |
          (Attacks::getBishopAttackMask(m_kingSq, occupancyMask) & m_bishops)) & opponentPieces)
        != SquareSet::none())
        return false;

    // ok, the move is legal
    return true;
}

bool ChessBoard::isLegalEpMove(const Square src, const Square dst, const Square ep) const noexcept
{
    const SquareSet dstBit { SquareSet::square(dst) };

    // NOTE: the only non-slider checker we can have is the EP pawn itself. The
    // last move was necessarily a pawn move, so only that pawn and exposed
    // sliders can check (but no other pawns or knights)
    if ((m_checkers & ~SquareSet::square(ep)) != SquareSet::none())
        return false;

    // update occupancy and turn color mask as per the move
    const SquareSet occupancyMask { (m_occupancyMask | dstBit) & (~(SquareSet::square(src) | SquareSet::square(ep))) };
    const SquareSet turnColorMask { m_turnColorMask | dstBit };
    const SquareSet opponentPieces { occupancyMask & ~turnColorMask };

    // now check if any slider checkers remain (all blocked, none revealed)
    if ((((Attacks::getRookAttackMask(m_kingSq, occupancyMask) & m_rooks) |
          (Attacks::getBishopAttackMask(m_kingSq, occupancyMask) & m_bishops)) & opponentPieces)
        != SquareSet::none())
        return false;

    // ok, the move is legal
    return true;
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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    const SquareSet allowedDstSqMask {
        (SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U) | SquareSet::row(6U))
        >> static_cast<std::uint8_t>(turn) };

    // is destination allowed and empty square
    if ((dstSqBit & allowedDstSqMask & ~m_occupancyMask) != SquareSet::none())
    {
        const int retractOneAdd { turn == Color::WHITE ? -8 : 8 }; // pawn step backwards
        const Square retractOne { addToSquareNoOverflowCheck(dst, retractOneAdd) };

        // is source allowed and our pawn?
        if ((srcSqMask & m_pawns & SquareSet::square(retractOne) & m_turnColorMask) != SquareSet::none())
        {
            if (isLegalRegularMove(retractOne, dst))
            {
                return Move { retractOne, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
            }

            return Move::illegalNoMove();
        }

        // ok, maybe it's a double pawn move
        const Square retractTwo { addToSquareNoOverflowCheck(retractOne, retractOneAdd) };
        const SquareSet powRow1 { turn == Color::WHITE ? SquareSet::row(1U) : SquareSet::row(6U) };

        if (
            // is source allowed, on 2nd rank (point of view), our pawn?
            ((srcSqMask & m_pawns & SquareSet::square(retractTwo) & powRow1 & m_turnColorMask) != SquareSet::none()) &&

            // is the square on 3rd rank empty?
            ((SquareSet::square(retractOne) & m_occupancyMask) == SquareSet::none())
            )
        {
            if (isLegalRegularMove(retractTwo, dst))
            {
                return Move { retractTwo, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
            }

            return Move::illegalNoMove();
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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    const SquareSet allowedDstSqMask {
        (SquareSet::row(2U) | SquareSet::row(3U) | SquareSet::row(4U) | SquareSet::row(5U) | SquareSet::row(6U))
        >> static_cast<std::uint8_t>(turn) };

    // is destination allowed and empty square
    if ((dstSqBit & allowedDstSqMask & ~m_occupancyMask) != SquareSet::none())
    {
        const int retractOneAdd { turn == Color::WHITE ? -8 : 8 }; // pawn step backwards
        const Square retractOne { addToSquareNoOverflowCheck(dst, retractOneAdd) };

        // is source allowed and our pawn?
        if ((srcSqMask & m_pawns & SquareSet::square(retractOne) & m_turnColorMask) != SquareSet::none())
        {
            if (isLegalRegularMove(retractOne, dst))
            {
                moves[0] = Move { retractOne, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                return 1U;
            }

            return 0U;
        }

        // ok, maybe it's a double pawn move
        const Square retractTwo { addToSquareNoOverflowCheck(retractOne, retractOneAdd) };
        const SquareSet powRow1 { turn == Color::WHITE ? SquareSet::row(1U) : SquareSet::row(6U) };

        if (
            // is source allowed, on 2nd rank (point of view), our pawn?
            ((srcSqMask & m_pawns & SquareSet::square(retractTwo) & powRow1 & m_turnColorMask) != SquareSet::none()) &&

            // is the square on 3rd rank empty?
            ((SquareSet::square(retractOne) & m_occupancyMask) == SquareSet::none())
            )
        {
            if (isLegalRegularMove(retractTwo, dst))
            {
                moves[0] = Move { retractTwo, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                return 1U;
            }

            return 0U;
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

    if (dst != m_epSquare)
    {
        const SquareSet dstSqBit { SquareSet::square(dst) };
        constexpr SquareSet dstSqMask { 0x00'FF'FF'FF'FF'FF'FF'00 };

        if ((m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask) != SquareSet::none())
        {
            SQUARESET_ENUMERATE(
                src,
                srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
                i = addMoveIfLegalRegular(i, src, dst, MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE));
        }
    }
    else
    {
        const int retractOneAdd { -8 + (2 * static_cast<int>(turn)) }; // pawn step backwards
        const Square retractOne { addToSquareNoOverflowCheck(dst, retractOneAdd) };

        SQUARESET_ENUMERATE(
            src,
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            i = addMoveIfLegalEp(i, src, dst, retractOne));
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
    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) };

    // allowedDstSqMask, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };

    const int retractOneAdd { -8 + (2 * static_cast<int>(turn)) }; // pawn step backwards
    const Square retractOne { addToSquareNoOverflowCheck(dst, retractOneAdd) };

    if (
        // destination allowed and empty square?
        ((dstSqBit & allowedDstSqMask & ~m_occupancyMask) != SquareSet::none()) &&

        // source allowed and has our pawn?
        ((SquareSet::square(retractOne) & srcSqMask & m_pawns & m_turnColorMask) != SquareSet::none())
        )
    {
        if (isLegalRegularMove(Square { retractOne }, dst))
        {
            return Move { retractOne, dst, pieceToTypeAndPromotion(promo) };
        }
    }

    return Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForPawnAndDestPromoNoCapture(ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) };

    // allowedDstSqMask, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };

    const int retractOneAdd { -8 + (2 * static_cast<int>(turn)) }; // pawn step backwards
    const Square retractOne { addToSquareNoOverflowCheck(dst, retractOneAdd) };

    if (
        // destination allowed and empty square?
        ((dstSqBit & allowedDstSqMask & ~m_occupancyMask) != SquareSet::none()) &&

        // source allowed and has our pawn?
        ((SquareSet::square(retractOne) & srcSqMask & m_pawns & m_turnColorMask) != SquareSet::none())
        )
    {
        if (isLegalRegularMove(Square { retractOne }, dst))
        {
            moves[0] = Move { retractOne, dst, pieceToTypeAndPromotion(promo) };
            return 1U;
        }
    }

    return 0U;
}

template <typename IteratorType>
IteratorType ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl(IteratorType i, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    const Color turn { getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { SquareSet::row(7U).rotl(static_cast<std::int8_t>(turn)) };

    // destination allowed and capture?
    if ((dstSqBit & allowedDstSqMask & m_occupancyMask & ~m_turnColorMask) != SquareSet::none())
    {
        SQUARESET_ENUMERATE(
            src,
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            i = addMoveIfLegalPromo(i, src, dst, promo));
    }

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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if ((m_turnColorMask & dstSqBit) == SquareSet::none())
    {
        SQUARESET_ENUMERATE(
            src,
            m_turnColorMask & m_knights & srcSqMask & Attacks::getKnightAttackMask(dst),
            i = addMoveIfLegalRegular(i, src, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE));
    }

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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if ((m_turnColorMask & dstSqBit) == SquareSet::none())
    {
        const SquareSet pieces {
            Attacks::getBishopAttackMask(dst, m_occupancyMask)
            & srcSqMask & m_turnColorMask & m_bishops & (~m_rooks)
        };

        SQUARESET_ENUMERATE(
            src,
            pieces,
            i = addMoveIfLegalRegular(i, src, dst, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE));
    }

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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if ((m_turnColorMask & dstSqBit) == SquareSet::none())
    {
        const SquareSet pieces {
            Attacks::getRookAttackMask(dst, m_occupancyMask)
            & srcSqMask & m_turnColorMask & (~m_bishops) & m_rooks
        };

        SQUARESET_ENUMERATE(
            src,
            pieces,
            i = addMoveIfLegalRegular(i, src, dst, MoveTypeAndPromotion::REGULAR_ROOK_MOVE));
    }

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
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if ((m_turnColorMask & dstSqBit) == SquareSet::none())
    {
        const SquareSet pieces {
            (Attacks::getBishopAttackMask(dst, m_occupancyMask) |
             Attacks::getRookAttackMask(dst, m_occupancyMask))
            & srcSqMask & m_turnColorMask & m_bishops & m_rooks
        };

        SQUARESET_ENUMERATE(
            src,
            pieces,
            i = addMoveIfLegalRegular(i, src, dst, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE));
    }

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

    if (((m_turnColorMask & dstSqBit) == SquareSet::none()) &&
        (srcSqMask & SquareSet::square(m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none() &&
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
        return generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, false>(SingleMoveIterator { }).getMove();

    return ChessBoard::Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForLongCastling(ShortMoveList &moves) const noexcept
{
    // long castling requested
    ShortMoveList::iterator i { moves.begin() };

    if (m_checkers == SquareSet::none())
        i = generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, false>(i);

    return i - moves.begin();
}

ChessBoard::Move ChessBoard::generateSingleMoveForShortCastling() const noexcept
{
    // long castling requested
    if (m_checkers == SquareSet::none())
        return generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, true>(SingleMoveIterator { }).getMove();

    return ChessBoard::Move::illegalNoMove();
}

std::size_t ChessBoard::generateMovesForShortCastling(ShortMoveList &moves) const noexcept
{
    // short castling requested
    ShortMoveList::iterator i { moves.begin() };

    if (m_checkers == SquareSet::none())
        i = generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, true>(i);

    return i - moves.begin();
}

}
