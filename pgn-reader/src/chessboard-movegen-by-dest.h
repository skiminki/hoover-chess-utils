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
inline void generateMovesForPawnAndDestNoCaptureStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit {
        SquareSet { dst } & ~board.getOccupancyMask() &
        blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) };

    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare {
        (PawnLookups::singleAdvanceNoPromoLegalDstMask(turn) & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet doubleAdvancingPawnSquare {
        (PawnLookups::rank3(turn) & singleAdvancingPawnSquare &~ board.getOccupancyMask()).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        (singleAdvancingPawnSquare | doubleAdvancingPawnSquare) &
        board.getPawns() & board.getPiecesInTurn() & srcSqMask };

    if (advancingPawn != SquareSet { }) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces())) [[likely]]
        {
            MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
        }
    }
}

template <MoveGenType type>
Move generateSingleMoveForPawnAndDestNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForPawnAndDestNoCaptureStoreFnTempl<type, SingleMoveStoreMoveNoDupCheckFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForPawnAndDestNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    auto i = moves.begin();
    generateMovesForPawnAndDestNoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
inline void generateMovesForPawnAndDestCaptureStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // ctPawnAttackerMasks, retractOneAdd optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };

    if (dst != board.getEpSquare()) [[likely]]
    {
        const SquareSet dstSqBit { dst };
        constexpr SquareSet dstSqMask { 0x00'FF'FF'FF'FF'FF'FF'00 };

        SquareSet dstOkMask {
            (blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) &
             board.getOccupancyMask() & dstSqBit & dstSqMask & ~board.getPiecesInTurn()).allIfAny() };

        SQUARESET_ENUMERATE(
            src,
            dstOkMask &
            srcSqMask & board.getPawns() & board.getPiecesInTurn() & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces()))
                {
                    MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
                }
            });
    }
    else
    {
        SQUARESET_ENUMERATE(
            src,
            srcSqMask & board.getPawns() & board.getPiecesInTurn() & Attacks::getPawnAttackerMask(dst, turn),
            {
                if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces()))
                {
                    MoveStoreFn::storeMove(store, Move { src, board.getEpSquare(), MoveTypeAndPromotion::EN_PASSANT });
                }
            });
    }
}

template <MoveGenType type>
Move generateSingleMoveForPawnAndDestCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForPawnAndDestCaptureStoreFnTempl<type, SingleMoveStoreMoveFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForPawnAndDestCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    auto i = moves.begin();
    generateMovesForPawnAndDestCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
inline void generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    // allowedDstSqMask optimization
    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    const Color turn { board.getTurn() };
    const SquareSet dstSqBit {
        SquareSet { dst } & ~board.getOccupancyMask() &
        blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) };

    // 7th or 0th rank
    const SquareSet allowedDstSqMask { PawnLookups::rank8(turn) };
    const auto pawnAdvanceShift = PawnLookups::pawnAdvanceShift(turn);

    const SquareSet singleAdvancingPawnSquare { (allowedDstSqMask & dstSqBit).rotr(pawnAdvanceShift) };

    const SquareSet advancingPawn {
        singleAdvancingPawnSquare &
        board.getPawns() & board.getPiecesInTurn() & srcSqMask };

    if (advancingPawn != SquareSet { }) [[likely]]
    {
        Square src { advancingPawn.firstSquare() };
        if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces())) [[likely]]
        {
            MoveStoreFn::storeMove(store, Move { src, dst, pieceToTypeAndPromotion(promo) });
        }
    }
}

template <MoveGenType type>
Move generateSingleMoveForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl<type, SingleMoveStoreMoveNoDupCheckFn>(
        board, srcSqMask, dst, promo, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForPawnAndDestPromoNoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    auto i = moves.begin();
    generateMovesForPawnAndDestPromoNoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, promo, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
inline void generateMovesForPawnAndDestPromoCaptureStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, Piece promo, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    const Color turn { board.getTurn() };

    const SquareSet dstSqBit { dst };
    const SquareSet dstSqMask { PawnLookups::rank8(turn) };

    SquareSet dstOkMask {
        (blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) &
         board.getOccupancyMask() & dstSqBit & dstSqMask & ~board.getPiecesInTurn()).allIfAny() };

    SQUARESET_ENUMERATE(
        src,
        dstOkMask &
        srcSqMask & board.getPawns() & board.getPiecesInTurn() & Attacks::getPawnAttackerMask(dst, turn),
        {
            if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces()))
            {
                MoveStoreFn::storeMove(store, Move { src, dst, pieceToTypeAndPromotion(promo) });
            }
        });
}

template <MoveGenType type>
Move generateSingleMoveForPawnAndDestPromoCaptureTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForPawnAndDestPromoCaptureStoreFnTempl<type, SingleMoveStoreMoveFn>(
        board, srcSqMask, dst, promo, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForPawnAndDestPromoCaptureTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst, Piece promo) noexcept
{
    auto i = moves.begin();
    generateMovesForPawnAndDestPromoCaptureStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, promo, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
inline void generateMovesForKnightAndDestStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) &
        ((board.getPiecesInTurn() & SquareSet { dst }).allIfNone() // check for no self-capture
         & board.getPiecesInTurn() & board.getKnights() & srcSqMask & Attacks::getKnightAttackMask(dst) &~ board.getPinnedPieces()),
        {
            MoveStoreFn::storeMove(store, Move { src, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE });
        });
}

template <MoveGenType type>
Move generateSingleMoveForKnightAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForKnightAndDestStoreFnTempl<type, SingleMoveStoreMoveFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForKnightAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    ShortMoveList::iterator i { moves.begin() };
    generateMovesForKnightAndDestStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, MoveTypeAndPromotion moveType, typename MoveStoreFn>
inline void generateMovesForSliderAndDestStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK || type == MoveGenType::CHECK);

    static_assert(
        moveType == MoveTypeAndPromotion::REGULAR_BISHOP_MOVE ||
        moveType == MoveTypeAndPromotion::REGULAR_ROOK_MOVE ||
        moveType == MoveTypeAndPromotion::REGULAR_QUEEN_MOVE);

    SquareSet pieces;

    if constexpr (moveType == MoveTypeAndPromotion::REGULAR_BISHOP_MOVE)
    {
        pieces = Attacks::getBishopAttackMask(dst, board.getOccupancyMask())
            & srcSqMask & board.getPiecesInTurn() & board.getBishopsAndQueens() & (~board.getRooksAndQueens());
    }
    else if constexpr (moveType == MoveTypeAndPromotion::REGULAR_ROOK_MOVE)
    {
        pieces = Attacks::getRookAttackMask(dst, board.getOccupancyMask())
            & srcSqMask & board.getPiecesInTurn() & (~board.getBishopsAndQueens()) & board.getRooksAndQueens();
    }
    else
    {
        static_assert(moveType == MoveTypeAndPromotion::REGULAR_QUEEN_MOVE);

        pieces = Attacks::getQueenAttackMask(dst, board.getOccupancyMask())
            & srcSqMask & board.getPiecesInTurn() & board.getBishopsAndQueens() & board.getRooksAndQueens();
    }

    SQUARESET_ENUMERATE(
        src,
        blocksAllChecksMaskTempl<type>(board.getKingInTurn(), board.getCheckers(), dst) &
        ((board.getPiecesInTurn() & SquareSet { dst }).allIfNone() // check for no self-capture
         & pieces),
        {
            if (Attacks::pinCheck(src, SquareSet { dst }, board.getKingInTurn(), board.getPinnedPieces()))
            {
                MoveStoreFn::storeMove(store, Move { src, dst, moveType });
            }
        });
}

template <MoveGenType type>
Move generateSingleMoveForBishopAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE, SingleMoveStoreMoveFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForBishopAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    ShortMoveList::iterator i { moves.begin() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(
            board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type>
Move generateSingleMoveForRookAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE, SingleMoveStoreMoveFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForRookAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    ShortMoveList::iterator i { moves.begin() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_ROOK_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(
            board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type>
Move generateSingleMoveForQueenAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE, SingleMoveStoreMoveFn>(board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForQueenAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    ShortMoveList::iterator i { moves.begin() };
    generateMovesForSliderAndDestStoreFnTempl<
        type, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE, IteratorStoreMoveFn<ShortMoveList::iterator> >(
            board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type, typename MoveStoreFn>
inline void generateMovesForKingAndDestStoreFnTempl(
    const ChessBoard &board,
    SquareSet srcSqMask, Square dst, typename MoveStoreFn::Store &store) noexcept
{
    const SquareSet dstSqBit { dst };
    const SquareSet srcSqBit { board.getKingInTurn() };

    const SquareSet kingAttackers {
        Attacks::determineAttackers(
            board.getOccupancyMask() & ~srcSqBit,
            board.getPiecesInTurn() & ~srcSqBit,
            board.getPawns(),
            board.getKnights(),
            board.getBishopsAndQueens(),
            board.getRooksAndQueens(),
            board.getKings() & ~srcSqBit,
            dst,
            board.getTurn()) };


    if (((board.getPiecesInTurn() & dstSqBit).allIfNone() &
         kingAttackers.allIfNone() &
         srcSqMask & SquareSet { board.getKingInTurn() } & Attacks::getKingAttackMask(dst)) != SquareSet { })
    {
        MoveStoreFn::storeMove(store, Move { board.getKingInTurn(), dst, MoveTypeAndPromotion::REGULAR_KING_MOVE });
    }
}

template <MoveGenType type>
Move generateSingleMoveForKingAndDestTempl(const ChessBoard &board, SquareSet srcSqMask, Square dst) noexcept
{
    Move ret { Move::illegalNoMove() };
    generateMovesForKingAndDestStoreFnTempl<type, SingleMoveStoreMoveFn>(
        board, srcSqMask, dst, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForKingAndDestTempl(const ChessBoard &board, ShortMoveList &moves, SquareSet srcSqMask, Square dst) noexcept
{
    ShortMoveList::iterator i { moves.begin() };
    generateMovesForKingAndDestStoreFnTempl<type, IteratorStoreMoveFn<ShortMoveList::iterator> >(
        board, srcSqMask, dst, i);
    return i - moves.begin();
}

template <MoveGenType type>
Move generateSingleMoveForLongCastlingTempl(const ChessBoard &board) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    Move ret { Move::illegalNoMove() };
    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, SingleMoveStoreMoveNoDupCheckFn, false>(
        board, attackedSquares, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForLongCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    // long castling requested
    ShortMoveList::iterator i { moves.begin() };

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    generateMovesForCastlingStoreFnTempl<
        MoveGenType::NO_CHECK, IteratorStoreMoveFn<ShortMoveList::iterator>, false>(board, attackedSquares, i);

    return i - moves.begin();
}

template <MoveGenType type>
Move generateSingleMoveForShortCastlingTempl(const ChessBoard &board) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    Move ret { Move::illegalNoMove() };
    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, SingleMoveStoreMoveNoDupCheckFn, true>(
        board, attackedSquares, ret);
    return ret;
}

template <MoveGenType type>
std::size_t generateMovesForShortCastlingTempl(const ChessBoard &board, ShortMoveList &moves) noexcept
{
    static_assert(type == MoveGenType::NO_CHECK);

    // long castling requested
    ShortMoveList::iterator i { moves.begin() };

    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    generateMovesForCastlingStoreFnTempl<
        MoveGenType::NO_CHECK, IteratorStoreMoveFn<ShortMoveList::iterator>, true>(board, attackedSquares, i);

    return i - moves.begin();
}

}

#endif
