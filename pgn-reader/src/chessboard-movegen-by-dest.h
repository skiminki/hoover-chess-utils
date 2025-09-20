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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_BY_DEST_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_BY_DEST_H_INCLUDED

#include "chessboard-movegen.h"


namespace hoover_chess_utils::pgn_reader
{

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForPawnAndDestNoCaptureStoreFnTempl(
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit {
        SquareSet::square(dst) & ~m_occupancyMask &
        blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) };

    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare {
        (PawnLookups::singleAdvanceNoPromoLegalDstMask(turn) & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (PawnLookups::rank3(turn) & singleAdvancingPawnSquare &~ m_occupancyMask).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces)) [[likely]]
        {
            MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
        }
    }
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForPawnAndDestNoCaptureStoreFnTempl<type, SingleMoveStoreMoveNoDupCheckFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    auto i = moves.begin();
    board.generateMovesForPawnAndDestNoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForPawnAndDestCaptureStoreFnTempl(
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept
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
            (blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) &
             m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

        SQUARESET_ENUMERATE(
            src,
            dstOkMask &
            srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
                {
                    MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE });
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
                    MoveStoreFn::storeMove(store, Move { src, m_epSquare, MoveTypeAndPromotion::EN_PASSANT });
                }
            });
    }
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForPawnAndDestCaptureStoreFnTempl<type, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    auto i = moves.begin();
    board.generateMovesForPawnAndDestCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl(
    SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { getTurn() };
    const SquareSet dstSqBit {
        SquareSet::square(dst) & ~m_occupancyMask &
        blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { PawnLookups::rank8(turn) };
    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        m_pawns & m_turnColorMask & srcSqMask };

    if (advancingPawn != SquareSet::none()) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces)) [[likely]]
        {
            MoveStoreFn::storeMove(store, Move { src, dst, pieceToTypeAndPromotion(promo) });
        }
    }
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl<type, SingleMoveStoreMoveNoDupCheckFn>(srcSqMask, dst, promo, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    auto i = moves.begin();
    board.generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        srcSqMask, dst, promo, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForPawnAndDestPromoCaptureStoreFnTempl(
    SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    const Color turn { getTurn() };

    const SquareSet dstSqBit { SquareSet::square(dst) };
    const SquareSet dstSqMask { PawnLookups::rank8(turn) };

    SquareSet dstOkMask {
        (blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) &
         m_occupancyMask & dstSqBit & dstSqMask & ~m_turnColorMask).allIfAny() };

    SQUARESET_ENUMERATE(
        src,
        dstOkMask &
        srcSqMask & m_pawns & m_turnColorMask & Attacks::getPawnAttackerMask(dst, turn),
        {
            if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
            {
                MoveStoreFn::storeMove(store, Move { src, dst, pieceToTypeAndPromotion(promo) });
            }
        });
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForPawnAndDestPromoCaptureStoreFnTempl<type, SingleMoveStoreMoveFn>(srcSqMask, dst, promo, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    auto i = moves.begin();
    board.generateMovesForPawnAndDestPromoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        srcSqMask, dst, promo, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForKnightAndDestStoreFnTempl(
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & m_turnColorMask & m_knights & srcSqMask & Attacks::getKnightAttackMask(dst) &~ m_pinnedPieces),
        {
            MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE });
        });
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForKnightAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForKnightAndDestStoreFnTempl<type, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForKnightAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { moves.begin() };
    board.generateMovesForKnightAndDestStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, MoveTypeAndPromotion moveType, typename MoveStoreFn>
void ChessBoard::generateMovesForSliderAndDestStoreFnTempl(
    SquareSet srcSqMask, Square dst, MoveStoreFn::Store &store) const noexcept
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
        blocksAllChecksMaskTempl<type>(m_kingSq, m_checkers, dst) &
        ((m_turnColorMask & SquareSet::square(dst)).allIfNone() // check for no self-capture
         & pieces),
        {
            if (Attacks::pinCheck(src, SquareSet::square(dst), m_kingSq, m_pinnedPieces))
            {
                MoveStoreFn::storeMove(store, Move { src, dst, moveType });
            }
        });
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForBishopAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForBishopAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { moves.begin() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForRookAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForRookAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { moves.begin() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForQueenAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForQueenAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { moves.begin() };
    board.generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
void ChessBoard::generateMovesForKingAndDestStoreFnTempl(
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) const noexcept
{
    const SquareSet dstSqBit { SquareSet::square(dst) };
    const SquareSet srcSqBit { SquareSet::square(m_kingSq) };

    const SquareSet kingAttackers {
        Attacks::determineAttackers(
            m_occupancyMask & ~srcSqBit,
            m_turnColorMask & ~srcSqBit,
            m_pawns,
            m_knights,
            m_bishops,
            m_rooks,
            m_kings & ~srcSqBit,
            dst,
            getTurn()) };


    if (((m_turnColorMask & dstSqBit).allIfNone() &
         kingAttackers.allIfNone() &
         srcSqMask & SquareSet::square(m_kingSq) & Attacks::getKingAttackMask(dst)) != SquareSet::none())
    {
        MoveStoreFn::storeMove(store, Move { m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE });
    }
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForKingAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    board.generateMovesForKingAndDestStoreFnTempl<type, SingleMoveStoreMoveFn>(srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForKingAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    MoveList::iterator i { moves.begin() };
    board.generateMovesForKingAndDestStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(srcSqMask, dst, i);
    return i - moves.begin();
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

    Move ret { Move::illegalNoMove() };
    board.generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, SingleMoveStoreMoveNoDupCheckFn, false>(
        attackedSquares, ret);
    return ret;
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

    board.generateMovesForCastlingStoreFnTempl<
        MoveGenType::NO_CHECK, IteratorStoreMoveFn<ShortMoveList::iterator>, false>(attackedSquares, i);

    return i - moves.begin();
}

template <MoveGenType type>
Move ChessBoard::generateSingleMoveForShortCastlingTempl(const ChessBoard &board) noexcept
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

    Move ret { Move::illegalNoMove() };
    board.generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, SingleMoveStoreMoveNoDupCheckFn, true>(
        attackedSquares, ret);
    return ret;
}

template <MoveGenType type>
std::size_t ChessBoard::generateMovesForShortCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept
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

    board.generateMovesForCastlingStoreFnTempl<
        MoveGenType::NO_CHECK, IteratorStoreMoveFn<ShortMoveList::iterator>, true>(attackedSquares, i);

    return i - moves.begin();
}

}

#endif
