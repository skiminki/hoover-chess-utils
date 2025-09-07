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

template <typename... Args>
Move ChessBoard::generateSingleIllegalNoMove(const ChessBoard &board, [[maybe_unused]] Args... args) noexcept
{
    static_cast<void>(board);

    return Move::illegalNoMove();
}

template <typename... Args>
std::size_t ChessBoard::generateNoMoves(const ChessBoard &board, ShortMoveList &moves, [[maybe_unused]] Args... args) noexcept
{
    static_cast<void>(board);
    static_cast<void>(moves);

    return 0U;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~board.m_occupancyMask & board.blocksAllChecksMaskTempl<type>(dst) };

    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare {
        (PawnLookups::singleAdvanceNoPromoLegalDstMask(turn) & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (PawnLookups::rank3(turn) & singleAdvancingPawnSquare &~ board.m_occupancyMask).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        board.m_pawns & board.m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), board.m_kingSq, board.m_pinnedPieces)) [[likely]]
        {
            return Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
        }
    }

    return Move::illegalNoMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~board.m_occupancyMask & board.blocksAllChecksMaskTempl<type>(dst) };

    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare {
        (PawnLookups::singleAdvanceNoPromoLegalDstMask(turn) & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (PawnLookups::rank3(turn) & singleAdvancingPawnSquare &~ board.m_occupancyMask).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        board.m_pawns & board.m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), board.m_kingSq, board.m_pinnedPieces)) [[likely]]
        {
            moves[0U] = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
            return 1U;
        }
    }

    return 0U;
}

template <MoveGenType type, typename IteratorType>
IteratorType ChessBoard::generateMovesForPawnAndDestCaptureIterTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // ctPawnAttackerMasks, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };

    if (dst != m_epSquare) [[likely]]
    {
        const SquareSet dstSqBit { SquareSet::square(dst) };
        constexpr SquareSet dstSqMask { 0x00'FF'FF'FF'FF'FF'FF'00 };

        SquareSet dstOkMask {
            (blocksAllChecksMaskTempl<type>(dst) & m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

        SQUARESET_ENUMERATE(
            src,
            dstOkMask &
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
                {
                    *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE };
                    ++i;
                }
            });
    }
    else
    {
        SQUARESET_ENUMERATE(
            src,
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
                {
                    *i = Move { src, m_epSquare, MoveTypeAndPromotion::EN_PASSANT };
                    ++i;
                }
            });
    }

    return i;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    return board.generateMovesForPawnAndDestCaptureIterTempl<type>(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { board.generateMovesForPawnAndDestCaptureIterTempl<type>(moves.begin(), srcSqMask, dst) };

    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~board.m_occupancyMask & board.blocksAllChecksMaskTempl<type>(dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { PawnLookups::rank8(turn) };
    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        board.m_pawns & board.m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), board.m_kingSq, board.m_pinnedPieces)) [[likely]]
        {
            return Move { src, dst, pieceToTypeAndPromotion(promo) };
        }
    }

    return Move::illegalNoMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit { SquareSet::square(dst) & ~board.m_occupancyMask & board.blocksAllChecksMaskTempl<type>(dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { PawnLookups::rank8(turn) };
    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        board.m_pawns & board.m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), board.m_kingSq, board.m_pinnedPieces)) [[likely]]
        {
            moves[0U] = Move { src, dst, pieceToTypeAndPromotion(promo) };
            return 1U;
        }
    }

    return 0U;
}

template <MoveGenType type, typename IteratorType>
IteratorType ChessBoard::generateMovesForPawnAndDestPromoCaptureIterTempl(IteratorType i, SquareSet srcSqMask, Square dst, Piece promo) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // ctPawnAttackerMasks, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };

    const SquareSet dstSqBit { SquareSet::square(dst) };
    const SquareSet dstSqMask { PawnLookups::rank8(turn) };

    SquareSet dstOkMask {
        (blocksAllChecksMaskTempl<type>(dst) & m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

    SQUARESET_ENUMERATE(
        src,
        dstOkMask &
        srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
        {
            if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
            {
                *i = Move { src, dst, pieceToTypeAndPromotion(promo) };
                ++i;
            }
        });

    return i;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    return board.generateMovesForPawnAndDestPromoCaptureIterTempl<type>(SingleMoveIterator { }, srcSqMask, dst, promo).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    MoveList::iterator i { board.generateMovesForPawnAndDestPromoCaptureIterTempl<type>(moves.begin(), srcSqMask, dst, promo) };
    return i - moves.begin();
}

template <MoveGenType type, typename IteratorType>
IteratorType ChessBoard::generateMovesForKnightAndDestIterTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMaskTempl<type>(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & m_turnColorMask & m_knights & srcSqMask & Attacks::getKnightAttackMask(dst) &~ m_pinnedPieces),
        {
            *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE };
            ++i;
        });

    return i;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForKnightAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    return board.generateMovesForKnightAndDestIterTempl<type>(SingleMoveIterator { }, srcSqMask, dst).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForKnightAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { board.generateMovesForKnightAndDestIterTempl<type>(moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <MoveGenType type, MoveTypeAndPromotion moveType, typename IteratorType>
IteratorType ChessBoard::generateMovesForSliderAndDestIterTempl(IteratorType i, SquareSet srcSqMask, Square dst) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    static_assert(
        moveType == MoveTypeAndPromotion::REGULAR_BISHOP_MOVE ||
        moveType == MoveTypeAndPromotion::REGULAR_ROOK_MOVE ||
        moveType == MoveTypeAndPromotion::REGULAR_QUEEN_MOVE);

    SquareSet pieces;

    if constexpr (moveType == MoveTypeAndPromotion::REGULAR_BISHOP_MOVE)
    {
        pieces = Attacks::getBishopAttackMask(dst, m_occupancyMask)
            & srcSqMask & m_turnColorMask & m_bishops & (~m_rooks);
    }
    else if constexpr (moveType == MoveTypeAndPromotion::REGULAR_ROOK_MOVE)
    {
        pieces = Attacks::getRookAttackMask(dst, m_occupancyMask)
            & srcSqMask & m_turnColorMask & (~m_bishops) & m_rooks;
    }
    else
    {
        static_assert(moveType == MoveTypeAndPromotion::REGULAR_QUEEN_MOVE);

        pieces = (Attacks::getBishopAttackMask(dst, m_occupancyMask) |
                  Attacks::getRookAttackMask(dst, m_occupancyMask))
            & srcSqMask & m_turnColorMask & m_bishops & m_rooks;
    }

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMaskTempl<type>(dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & pieces),
        {
            if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
            {
                *i = Move { src, dst, moveType };
                ++i;
            }
        });

    return i;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForBishopAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    return board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE>(
        SingleMoveIterator { }, srcSqMask, dst).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForBishopAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE>(
            moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForRookAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    return board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE>(
        SingleMoveIterator { }, srcSqMask, dst).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForRookAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE>(
            moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForQueenAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    return board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE>(
        SingleMoveIterator { }, srcSqMask, dst).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForQueenAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { board.generateMovesForSliderAndDestIterTempl<type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE>(
            moves.begin(), srcSqMask, dst) };
    return i - moves.begin();
}


template <MoveGenType type>
Move ChessBoard::generateSingleMoveForKingAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if (((board.m_turnColorMask & dstSqBit) == SquareSet::none()) &&
        (srcSqMask & SquareSet::square(board.m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none() &&
        board.isLegalKingMove(board.m_kingSq, dst, board.getTurn()))
    {
        return Move { board.m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
    }

    return Move::illegalNoMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForKingAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    const SquareSet dstSqBit { SquareSet::square(dst) };

    if (((board.m_turnColorMask & dstSqBit).allIfNone() &
         srcSqMask & SquareSet::square(board.m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none() &&
        board.isLegalKingMove(board.m_kingSq, dst, board.getTurn()))
    {
        moves[0] = Move { board.m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
        return 1U;
    }

    return 0U;
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForLongCastlingTempl(const ChessBoard &board) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.m_occupancyMask &~ (board.m_kings & board.m_turnColorMask), // remove potentially attacked king
            board.m_pawns &~ board.m_turnColorMask,
            board.m_knights &~ board.m_turnColorMask,
            board.m_bishops &~ board.m_turnColorMask,
            board.m_rooks &~ board.m_turnColorMask,
            board.m_oppKingSq,
            board.getTurn()) };

    return board.generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, false>(SingleMoveIterator { }, attackedSquares).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForLongCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    // long castling requested
    ShortMoveList::iterator i { moves.begin() };

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.m_occupancyMask &~ (board.m_kings & board.m_turnColorMask), // remove potentially attacked king
            board.m_pawns &~ board.m_turnColorMask,
            board.m_knights &~ board.m_turnColorMask,
            board.m_bishops &~ board.m_turnColorMask,
            board.m_rooks &~ board.m_turnColorMask,
            board.m_oppKingSq,
            board.getTurn()) };

    i = board.generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, false>(i, attackedSquares);

    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForShortCastlingTempl(const ChessBoard &board) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    // long castling requested
    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.m_occupancyMask &~ (board.m_kings & board.m_turnColorMask), // remove potentially attacked king
            board.m_pawns &~ board.m_turnColorMask,
            board.m_knights &~ board.m_turnColorMask,
            board.m_bishops &~ board.m_turnColorMask,
            board.m_rooks &~ board.m_turnColorMask,
            board.m_oppKingSq,
            board.getTurn()) };

    return board.generateMovesForCastling<SingleMoveIterator, MoveGenType::NO_CHECK, true>(SingleMoveIterator { }, attackedSquares).getMove();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForShortCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    // short castling requested
    ShortMoveList::iterator i { moves.begin() };

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.m_occupancyMask &~ (board.m_kings & board.m_turnColorMask), // remove potentially attacked king
            board.m_pawns &~ board.m_turnColorMask,
            board.m_knights &~ board.m_turnColorMask,
            board.m_bishops &~ board.m_turnColorMask,
            board.m_rooks &~ board.m_turnColorMask,
            board.m_oppKingSq,
            board.getTurn()) };

    i = board.generateMovesForCastling<ShortMoveList::iterator, MoveGenType::NO_CHECK, true>(i, attackedSquares);

    return i - moves.begin();
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesTempl(const ChessBoard &board, MoveList &moves) noexcept
{
    const MoveList::iterator i { board.generateMovesIterTempl<type>(moves.begin()) };
    return i - moves.begin();
}

template <MoveGenType type>
std::size_t ChessBoard::getNumberOfLegalMovesTempl(const ChessBoard &board) noexcept
{
    return board.generateMovesIterTempl<type>(LegalMoveCounterIterator { }).getNumberOfLegalMoves();
}

template <MoveGenType type>
bool ChessBoard::hasLegalMovesTempl(const ChessBoard &board) noexcept
{
    const LegalMoveDetectorIterator legalMovesIterator {
        board.generateMovesIterTempl<type>(LegalMoveDetectorIterator { }) };

    return legalMovesIterator.hasLegalMoves();
}

std::array<MoveGenFunctions, 3U> MoveGenFunctionTables::m_fns
{
    // Move generator functions: MoveGenType::NO_CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestCapture = ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKnightAndDest = ChessBoard::generateSingleMoveForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForBishopAndDest = ChessBoard::generateSingleMoveForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForRookAndDest = ChessBoard::generateSingleMoveForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForQueenAndDest = ChessBoard::generateSingleMoveForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForShortCastling = ChessBoard::generateSingleMoveForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForLongCastling = ChessBoard::generateSingleMoveForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMovesForPawnAndDestNoCapture = ChessBoard::generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestCapture = ChessBoard::generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestPromoCapture = ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKnightAndDest = ChessBoard::generateMovesForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForBishopAndDest = ChessBoard::generateMovesForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForRookAndDest = ChessBoard::generateMovesForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForQueenAndDest = ChessBoard::generateMovesForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForShortCastling = ChessBoard::generateMovesForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateMovesForLongCastling = ChessBoard::generateMovesForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::NO_CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::NO_CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::NO_CHECK>,
    },

    // Move generator functions: MoveGenType::CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestCapture = ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKnightAndDest = ChessBoard::generateSingleMoveForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForBishopAndDest = ChessBoard::generateSingleMoveForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForRookAndDest = ChessBoard::generateSingleMoveForRookAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForQueenAndDest = ChessBoard::generateSingleMoveForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForShortCastling = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForLongCastling = ChessBoard::generateSingleIllegalNoMove,

        .generateMovesForPawnAndDestNoCapture = ChessBoard::generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestCapture = ChessBoard::generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoCapture = ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForKnightAndDest = ChessBoard::generateMovesForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForBishopAndDest = ChessBoard::generateMovesForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForRookAndDest = ChessBoard::generateMovesForRookAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForQueenAndDest = ChessBoard::generateMovesForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForShortCastling = ChessBoard::generateNoMoves,
        .generateMovesForLongCastling = ChessBoard::generateNoMoves,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::CHECK>,
    },

    // Move generator functions: MoveGenType::DOUBLE_CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestCapture = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestPromoNoCapture = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestPromoCapture = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForKnightAndDest = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForBishopAndDest = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForRookAndDest = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForQueenAndDest = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
        .generateSingleMoveForShortCastling = ChessBoard::generateSingleIllegalNoMove,
        .generateSingleMoveForLongCastling = ChessBoard::generateSingleIllegalNoMove,

        .generateMovesForPawnAndDestNoCapture = ChessBoard::generateNoMoves,
        .generateMovesForPawnAndDestCapture = ChessBoard::generateNoMoves,
        .generateMovesForPawnAndDestPromoNoCapture = ChessBoard::generateNoMoves,
        .generateMovesForPawnAndDestPromoCapture = ChessBoard::generateNoMoves,
        .generateMovesForKnightAndDest = ChessBoard::generateNoMoves,
        .generateMovesForBishopAndDest = ChessBoard::generateNoMoves,
        .generateMovesForRookAndDest = ChessBoard::generateNoMoves,
        .generateMovesForQueenAndDest = ChessBoard::generateNoMoves,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
        .generateMovesForShortCastling = ChessBoard::generateNoMoves,
        .generateMovesForLongCastling = ChessBoard::generateNoMoves,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
    },
};

}
