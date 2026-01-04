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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_ALL_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_MOVEGEN_ALL_H_INCLUDED

#include "chessboard-movegen.h"


namespace hoover_chess_utils::pgn_reader
{

/// @brief Generates legal pawn moves, the actual implementation.
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @tparam     legalDestinations  Parameter type for legal destinations
/// @tparam     turn               Side to move
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  legalDestinations  Legal destinations
/// @return                        Move list iterator (end of generated moves)
template <typename IteratorType, MoveGenType type, typename ParamType, Color turn>
auto generateMovesForPawnsTempl(
    const ChessBoard &board,
    IteratorType i,
    ParamType legalDestinations) noexcept -> IteratorType
{
    using SideSpecifics = PawnLookups_SideSpecificsTempl<turn>;

    const SquareSet pawns { board.getPawns() & board.getPiecesInTurn() };

    const SquareSet unpinnedPawns { pawns & ~board.getPinnedPieces() };

    // destination squares for advancing pawns (must be empty)
    const SquareSet advanceMaskUnpinned {
        SideSpecifics::pawnAdvance(unpinnedPawns) & ~board.getOccupancyMask() };

    // destination squares for double-advancing pawns (must be empty)
    const SquareSet doubleAdvanceMaskUnpinned {
        SideSpecifics::pawnAdvance(advanceMaskUnpinned & SideSpecifics::rank3) & ~board.getOccupancyMask() };

    // pawn advances for unpinned pawns
    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            advanceMaskUnpinned & ~SideSpecifics::promoRank & legalDestinations(),
            {
                *i = Move { SideSpecifics::pawnRetreatSq(dst), dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        SQUARESET_ENUMERATE(
            dst,
            doubleAdvanceMaskUnpinned & legalDestinations(),
            {
                *i = Move { SideSpecifics::pawnDoubleRetreatSq(dst), dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        SQUARESET_ENUMERATE(
            dst,
            advanceMaskUnpinned & SideSpecifics::promoRank & legalDestinations(),
            {
                const Square src { SideSpecifics::pawnRetreatSq(dst) };

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::QUEEN) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::ROOK) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::BISHOP) };
                ++i;

                *i = Move { src, dst, pieceToTypeAndPromotion(Piece::KNIGHT) };
                ++i;
            });
    }
    else
    {
        i += ((advanceMaskUnpinned | doubleAdvanceMaskUnpinned) &~ SideSpecifics::promoRank & legalDestinations()).
            popcount();

        i += (advanceMaskUnpinned & SideSpecifics::promoRank & legalDestinations()).popcount() * 4U;
    }

    const SquareSet oppPieces { (board.getOccupancyMask() ^ board.getPiecesInTurn()) };
    const SquareSet leftCapturingPawns {
        Attacks::getPawnAttackersMask<turn, false>(oppPieces & legalDestinations()) & pawns };
    const SquareSet rightCapturingPawns {
        Attacks::getPawnAttackersMask<turn, true>(oppPieces & legalDestinations()) & pawns };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        // capture left
        SQUARESET_ENUMERATE(
            src,
            leftCapturingPawns & ~SideSpecifics::rank7 & ~board.getPinnedPieces(),
            {
                *i = Move { src, SideSpecifics::captureLeftSq(src), MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        // capture right
        SQUARESET_ENUMERATE(
            src,
            rightCapturingPawns & ~SideSpecifics::rank7 & ~board.getPinnedPieces(),
            {
                *i = Move { src, SideSpecifics::captureRightSq(src), MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        // capture left + promo
        SQUARESET_ENUMERATE(
            src,
            leftCapturingPawns & SideSpecifics::rank7 & ~board.getPinnedPieces(),
            {
                const Square dst { SideSpecifics::captureLeftSq(src) };

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                ++i;
            });

        // capture right + promo
        SQUARESET_ENUMERATE(
            src,
            rightCapturingPawns & SideSpecifics::rank7 & ~board.getPinnedPieces(),
            {
                const Square dst { SideSpecifics::captureRightSq(src) };

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                ++i;

                *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                ++i;
            });
    }
    else
    {
        i += (leftCapturingPawns & ~SideSpecifics::rank7 & ~board.getPinnedPieces()).popcount();
        i += (rightCapturingPawns & ~SideSpecifics::rank7 & ~board.getPinnedPieces()).popcount();
        i += (leftCapturingPawns & SideSpecifics::rank7 & ~board.getPinnedPieces()).popcount() * 4U;
        i += (rightCapturingPawns & SideSpecifics::rank7 & ~board.getPinnedPieces()).popcount() * 4U;
    }

    // pinned pawns
    // note: pinned pawns can never resolve a check
    if constexpr (type == MoveGenType::NO_CHECK)
    {
        // note: we need to filter out the EP square with board.getPiecesInTurn()
        const SquareSet pinnedPawns { board.getPawns() & board.getPinnedPieces() };

        SQUARESET_ENUMERATE(
            src,
            pinnedPawns,
            {
                SquareSet pawnBit { src };
                const SquareSet advanceMask { SideSpecifics::pawnAdvance(pawnBit) & ~board.getOccupancyMask() };
                const SquareSet doubleAdvanceMask { SideSpecifics::pawnAdvance(advanceMask & SideSpecifics::rank3) & ~board.getOccupancyMask() };

                // advances, non-promo
                SQUARESET_ENUMERATE(
                    dst,
                    (advanceMask | doubleAdvanceMask) & (~SideSpecifics::promoRank) &
                    Intercepts::getPinRestiction<true>(board.getKingInTurn(), src) &
                    legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                        ++i;
                    });

                // note: pinned pawn can never promote without capture

                // left-capture, non-promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureLeft(pawnBit) & (~SideSpecifics::promoRank) &
                    Intercepts::getPinRestiction<true>(board.getKingInTurn(), src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                        ++i;
                    });

                // left-capture, promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureLeft(pawnBit) & SideSpecifics::promoRank &
                    Intercepts::getPinRestiction<true>(board.getKingInTurn(), src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                        ++i;
                    });

                // right-capture, non-promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureRight(pawnBit) & (~SideSpecifics::promoRank) &
                    Intercepts::getPinRestiction<true>(board.getKingInTurn(), src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                        ++i;
                    });

                // right-capture, promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureRight(pawnBit) & SideSpecifics::promoRank &
                    Intercepts::getPinRestiction<true>(board.getKingInTurn(), src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_QUEEN };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_ROOK };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_BISHOP };
                        ++i;

                        *i = Move { src, dst, MoveTypeAndPromotion::PROMO_KNIGHT };
                        ++i;
                    });
            });
    }

    // EP captures
    if (board.getEpSquare() <= Square::H8)
    {
        SQUARESET_ENUMERATE(
            src,
            Attacks::getPawnAttackerMask(board.getEpSquare(), turn) & pawns,
            {
                // The only legality check we need is the capturer pin check
                if (Attacks::pinCheck(src, SquareSet { board.getEpSquare() }, board.getKingInTurn(), board.getPinnedPieces()))
                {
                    *i = Move { src, board.getEpSquare(), MoveTypeAndPromotion::EN_PASSANT };
                    ++i;
                }
            });
    }

    return i;
}

/// @brief Generates legal pawn moves, dispatch for side-to-move.
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @tparam     legalDestinations  Parameter type for legal destinations
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  legalDestinations  Legal destinations
/// @return                        Move list iterator (end of generated moves)
template <typename IteratorType, MoveGenType type, typename ParamType>
auto generateMovesForPawns(
    const ChessBoard &board,
    IteratorType i,
    ParamType legalDestinations) noexcept -> IteratorType
{
    if (board.getTurn() == Color::WHITE)
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::WHITE>(board, i, legalDestinations);
    else
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::BLACK>(board, i, legalDestinations);
}

/// @brief Generates legal knight moves
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @tparam     legalDestinations  Parameter type for legal destinations
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  sq                 Source square
/// @param[in]  legalDestinations  Legal destinations
/// @return                        Move list iterator (end of generated moves)
template <typename IteratorType, MoveGenType type, typename ParamType>
auto generateMovesForKnight(
    const ChessBoard &board,
    IteratorType i,
    Square sq,
    ParamType legalDestinations) noexcept -> IteratorType
{
    // occup  turnColor    valid
    //     0          0        1
    //     0          1        1
    //     1          0        1
    //     1          1        0
    const SquareSet emptyOrCapture { ~(board.getOccupancyMask() & board.getPiecesInTurn()) };
    const SquareSet dstSquares { Attacks::getKnightAttackMask(sq) & emptyOrCapture & legalDestinations() };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            dstSquares,
            {
                *i = Move { sq, dst, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE };
                ++i;
            });
    }
    else
    {
        i += dstSquares.popcount();
    }

    return i;
}

/// @brief Generates legal bishop moves for queen or bishop.
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @tparam     legalDestinations  Parameter type for legal destinations
/// @tparam     pinned             Whether the piece is pinned
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  sq                 Source square
/// @param[in]  legalDestinations  Legal destinations
/// @param[in]  typeAndPromo       @coderef{MoveTypeAndPromotion::REGULAR_BISHOP_MOVE} or
///                                @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE} depending
///                                on the piece.
/// @return                        Move list iterator (end of generated moves)
template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
auto generateMovesForBishop(
    const ChessBoard &board,
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(board.getOccupancyMask() & board.getPiecesInTurn()) };

    const SquareSet dstSquares {
        Attacks::getBishopAttackMask(sq, board.getOccupancyMask()) &
        emptyOrCapture &
        Intercepts::getPinRestiction<pinned>(board.getKingInTurn(), sq) &
        legalDestinations() };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            dstSquares,
            {
                *i = Move { sq, dst, typeAndPromo };
                ++i;
            });
    }
    else
    {
        i += dstSquares.popcount();
    }

    return i;
}

/// @brief Generates legal rook moves for queen or rook.
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @tparam     legalDestinations  Parameter type for legal destinations
/// @tparam     pinned             Whether the piece is pinned
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  sq                 Source square
/// @param[in]  legalDestinations  Legal destinations
/// @param[in]  typeAndPromo       @coderef{MoveTypeAndPromotion::REGULAR_ROOK_MOVE} or
///                                @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE} depending
///                                on the piece.
/// @return                        Move list iterator (end of generated moves)
template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
auto generateMovesForRook(
    const ChessBoard &board,
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(board.getOccupancyMask() & board.getPiecesInTurn()) };
    const SquareSet dstSquares {
        Attacks::getRookAttackMask(sq, board.getOccupancyMask()) &
        emptyOrCapture &
        Intercepts::getPinRestiction<pinned>(board.getKingInTurn(), sq) &
        legalDestinations() };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            dstSquares,
            {
                *i = Move { sq, dst, typeAndPromo };
                ++i;
            });
    }
    else
    {
        i += dstSquares.popcount();
    }

    return i;
}

/// @brief Generates legal king moves
///
/// @tparam     IteratorType       Move list iterator type
/// @tparam     type               Move generator type
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @param[in]  attackedSquares    Attacked squares
/// @return                        Move list iterator (end of generated moves)
///
/// @sa @coderef{Attacks::determineAttackedSquares()}
template <typename IteratorType>
auto generateMovesForKing(
    const ChessBoard &board,
    IteratorType i,
    SquareSet attackedSquares) noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(board.getOccupancyMask() & board.getPiecesInTurn()) };

    const SquareSet dstSquares {
        Attacks::getKingAttackMask(board.getKingInTurn()) & emptyOrCapture &~ attackedSquares };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            dstSquares,
            {
                *i = Move { board.getKingInTurn(), dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
                ++i;
            });
    }
    else
    {
        i += dstSquares.popcount();
    }

    return i;
}

/// @brief Generates all legal moves when not in check
///
/// @tparam     IteratorType       Move list iterator type
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @return                        Move list iterator (end of generated moves)
///
/// @sa @coderef{Attacks::determineAttackedSquares()}
template <typename IteratorType>
inline IteratorType generateAllLegalMovesTemplNoCheck(
    const ChessBoard &board,
    IteratorType i) noexcept
{
    using ParamType = AllLegalDestinationType;
    constexpr ParamType legalDestinations { };

    i = generateMovesForPawns<IteratorType, MoveGenType::NO_CHECK>(board, i, legalDestinations);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getKnights() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        i = generateMovesForKnight<IteratorType, MoveGenType::NO_CHECK>(board, i, sq, legalDestinations));

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getBishopsAndQueens() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getRooksAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::NO_CHECK, ParamType, false>(board, i, sq, legalDestinations, typeAndPromo);
        });

    SQUARESET_ENUMERATE(
        sq,
        board.getBishopsAndQueens() & board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getRooksAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::NO_CHECK, ParamType, true>(board, i, sq, legalDestinations, typeAndPromo);
        });

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getRooksAndQueens() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getBishopsAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::NO_CHECK, ParamType, false>(board, i, sq, legalDestinations, typeAndPromo);
        });

    SQUARESET_ENUMERATE(
        sq,
        board.getRooksAndQueens() & board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getBishopsAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::NO_CHECK, ParamType, true>(board, i, sq, legalDestinations, typeAndPromo);
        });

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    // In no check, we'll do the king and castling moves last. This helps the
    // LegalMoveDetectorIterator with early exit, since these are a bit more
    // expensive than the other moves,
    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    i = generateMovesForKing<IteratorType>(board, i, attackedSquares);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, IteratorStoreMoveFn<IteratorType>, false>(board, attackedSquares, i);
    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, IteratorStoreMoveFn<IteratorType>, true>(board, attackedSquares, i);

    return i;
}

/// @brief Generates all legal moves when in check (but not in double check)
///
/// @tparam     IteratorType       Move list iterator type
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @return                        Move list iterator (end of generated moves)
///
/// @sa @coderef{Attacks::determineAttackedSquares()}
template <typename IteratorType>
inline IteratorType generateAllLegalMovesTemplInCheck(
    const ChessBoard &board,
    IteratorType i) noexcept
{
    using ParamType = ParametrizedLegalDestinationType;
    ParamType legalDestinations {
        Intercepts::getInterceptSquares(board.getKingInTurn(), board.getCheckers().firstSquare()) };

    // if we're in check, we'll try the king moves first
    {
        const SquareSet attackedSquares {
            Attacks::determineAttackedSquares(
                board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
                board.getPawns() &~ board.getPiecesInTurn(),
                board.getKnights() &~ board.getPiecesInTurn(),
                board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
                board.getRooksAndQueens() &~ board.getPiecesInTurn(),
                board.getKingNotInTurn(),
                board.getTurn()) };

        i = generateMovesForKing<IteratorType>(board, i, attackedSquares);
        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;
    }

    i = generateMovesForPawns<IteratorType, MoveGenType::CHECK, ParamType>(board, i, legalDestinations);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getKnights() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        i = generateMovesForKnight<IteratorType, MoveGenType::CHECK, ParamType>(board, i, sq, legalDestinations));

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getBishopsAndQueens() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getRooksAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::CHECK, ParamType, false>(board, i, sq, legalDestinations, typeAndPromo);
        });

    // Note: pinned pieces cannot resolve checks

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        board.getRooksAndQueens() & board.getPiecesInTurn() & ~board.getPinnedPieces(),
        {
            const MoveTypeAndPromotion typeAndPromo {
                (board.getBishopsAndQueens() & SquareSet { sq }) != SquareSet { } ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::CHECK, ParamType, false>(board, i, sq, legalDestinations, typeAndPromo);
        });

    // Note: pinned pieces cannot resolve checks

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    return i;
}

/// @brief Generates all legal moves when in double check
///
/// @tparam     IteratorType       Move list iterator type
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @return                        Move list iterator (end of generated moves)
///
/// @sa (@coderef{Attacks::determineAttackedSquares()})
template <typename IteratorType>
inline IteratorType generateAllLegalMovesTemplInDoubleCheck(
    const ChessBoard &board,
    IteratorType i) noexcept
{
    // in double-check, king moves are the only legal moves
    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            board.getOccupancyMask() &~ (board.getKings() & board.getPiecesInTurn()), // remove potentially attacked king
            board.getPawns() &~ board.getPiecesInTurn(),
            board.getKnights() &~ board.getPiecesInTurn(),
            board.getBishopsAndQueens() &~ board.getPiecesInTurn(),
            board.getRooksAndQueens() &~ board.getPiecesInTurn(),
            board.getKingNotInTurn(),
            board.getTurn()) };

    i = generateMovesForKing<IteratorType>(board, i, attackedSquares);

    return i;
}

/// @brief Generates all legal moves for a specific iterator type.
///
/// @tparam     type               Move generator type
/// @tparam     IteratorType       Move list iterator type
/// @param[in]  board              Chess board
/// @param[in]  i                  Move list iterator (begin of list)
/// @return                        Move list iterator (end of generated moves)
///
/// Depending on the iterator type, we generate the following information
///
/// <table>
/// <tr>
///   <th>Iterator type</th>
///   <th>Generated return</th>
/// </tr>
/// <tr>
///   <td>@c MoveList::iterator</td>
///   <td>Proper list of legal moves</td>
/// </tr>
/// <tr>
///   <td>@coderef{LegalMoveCounterIterator}</td>
///   <td>Number of legal moves</td>
/// </tr>
/// <tr>
///   <td>@coderef{LegalMoveDetectorIterator}</td>
///   <td>Whether there are any legal moves</td>
/// </tr>
/// </table>
template <MoveGenType type, typename IteratorType>
inline IteratorType generateMovesIterTempl(
    const ChessBoard &board,
    IteratorType i) noexcept
{
    if constexpr (type == MoveGenType::NO_CHECK)
    {
        // all destinations are ok as long as the move is ok
        i = generateAllLegalMovesTemplNoCheck<IteratorType>(board, i);
    }
    else if constexpr (type == MoveGenType::CHECK)
    {
        // must capture the checker or block the check
        i = generateAllLegalMovesTemplInCheck<IteratorType>(board, i);
    }
    else
    {
        static_assert(type == MoveGenType::DOUBLE_CHECK);

        // king moves only
        i = generateAllLegalMovesTemplInDoubleCheck<IteratorType>(board, i);
    }

    return i;
}

/// @brief Generates all legal moves for a position.
///
/// @tparam     type               Move generator type
/// @param[in]  board              Chess board
/// @param[out] moves              Move list
/// @return                        Number of generated moves
template <MoveGenType type>
std::size_t generateMovesTempl(const ChessBoard &board, MoveList &moves) noexcept
{
    const MoveList::iterator i { generateMovesIterTempl<type>(board, moves.begin()) };
    return i - moves.begin();
}

/// @brief Returns the number of legal moves for a position.
///
/// @tparam     type               Move generator type
/// @param[in]  board              Chess board
/// @return                        Number of generated moves
template <MoveGenType type>
std::size_t getNumberOfLegalMovesTempl(const ChessBoard &board) noexcept
{
    return generateMovesIterTempl<type>(board, LegalMoveCounterIterator { }).getNumberOfLegalMoves();
}

/// @brief Determines whether there are any legal moves
///
/// @tparam     type               Move generator type
/// @param[in]  board              Chess board
/// @return                        Whether there are any legal moves
template <MoveGenType type>
bool hasLegalMovesTempl(const ChessBoard &board) noexcept
{
    const LegalMoveDetectorIterator legalMovesIterator {
        generateMovesIterTempl<type>(board, LegalMoveDetectorIterator { }) };

    return legalMovesIterator.hasLegalMoves();
}

}

#endif
