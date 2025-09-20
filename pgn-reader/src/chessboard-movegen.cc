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

namespace {

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
constexpr inline SquareSet blocksAllChecksMaskTempl<MoveGenType::CHECK>(Square kingSq, SquareSet checkers, Square dst) noexcept
{
    return
        (Intercepts::getInterceptSquares(kingSq, checkers.firstSquare()) & SquareSet::square(dst)).allIfAny();
}

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


template <typename IteratorType, MoveGenType type, typename ParamType, Color turn>
auto ChessBoard::generateMovesForPawnsTempl(
    IteratorType i,
    ParamType legalDestinations) const noexcept -> IteratorType
{
    using SideSpecifics = PawnLookups_SideSpecificsTempl<turn>;

    const SquareSet pawns { m_pawns & m_turnColorMask };

    const SquareSet unpinnedPawns { m_pawns & m_turnColorMask & ~m_pinnedPieces };
    const SquareSet pinnedPawns { m_pawns & m_turnColorMask & m_pinnedPieces };

    // destination squares for advancing pawns
    const SquareSet advanceMaskUnpinned { SideSpecifics::pawnAdvance(unpinnedPawns) & ~m_occupancyMask };

    // destination squares for double-advancing pawns
    const SquareSet doubleAdvanceMaskUnpinned { SideSpecifics::pawnAdvance(advanceMaskUnpinned & SideSpecifics::rank3) & ~m_occupancyMask };

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

    const SquareSet oppPieces { (m_occupancyMask ^ m_turnColorMask) };
    const SquareSet leftCapturingPawns {
        Attacks::getPawnAttackersMask<turn, false>(oppPieces & legalDestinations()) & pawns };
    const SquareSet rightCapturingPawns {
        Attacks::getPawnAttackersMask<turn, true>(oppPieces & legalDestinations()) & pawns };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        // capture left
        SQUARESET_ENUMERATE(
            src,
            leftCapturingPawns & ~SideSpecifics::rank7 & ~m_pinnedPieces,
            {
                *i = Move { src, SideSpecifics::captureLeftSq(src), MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        // capture right
        SQUARESET_ENUMERATE(
            src,
            rightCapturingPawns & ~SideSpecifics::rank7 & ~m_pinnedPieces,
            {
                *i = Move { src, SideSpecifics::captureRightSq(src), MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                ++i;
            });

        // capture left + promo
        SQUARESET_ENUMERATE(
            src,
            leftCapturingPawns & SideSpecifics::rank7 & ~m_pinnedPieces,
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
            rightCapturingPawns & SideSpecifics::rank7 & ~m_pinnedPieces,
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
        i += (leftCapturingPawns & ~SideSpecifics::rank7 & ~m_pinnedPieces).popcount();
        i += (rightCapturingPawns & ~SideSpecifics::rank7 & ~m_pinnedPieces).popcount();
        i += (leftCapturingPawns & SideSpecifics::rank7 & ~m_pinnedPieces).popcount() * 4U;
        i += (rightCapturingPawns & SideSpecifics::rank7 & ~m_pinnedPieces).popcount() * 4U;
    }

    // pinned pawns
    // note: pinned pawns can never resolve a check
    if constexpr (type == MoveGenType::NO_CHECK)
    {
        SQUARESET_ENUMERATE(
            src,
            pinnedPawns,
            {
                SquareSet pawnBit { SquareSet::square(src) };
                const SquareSet advanceMask { SideSpecifics::pawnAdvance(pawnBit) & ~m_occupancyMask };
                const SquareSet doubleAdvanceMask { SideSpecifics::pawnAdvance(advanceMask & SideSpecifics::rank3) & ~m_occupancyMask };

                // advances, non-promo
                SQUARESET_ENUMERATE(
                    dst,
                    (advanceMask | doubleAdvanceMask) & (~SideSpecifics::promoRank) &
                    Intercepts::getPinRestiction<true>(m_kingSq, src) &
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
                    Intercepts::getPinRestiction<true>(m_kingSq, src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                        ++i;
                    });

                // left-capture, promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureLeft(pawnBit) & SideSpecifics::promoRank &
                    Intercepts::getPinRestiction<true>(m_kingSq, src) & legalDestinations(),
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
                    Intercepts::getPinRestiction<true>(m_kingSq, src) & legalDestinations(),
                    {
                        *i = Move { src, dst, MoveTypeAndPromotion::REGULAR_PAWN_MOVE };
                        ++i;
                    });

                // right-capture, promo
                SQUARESET_ENUMERATE(
                    dst,
                    oppPieces & SideSpecifics::captureRight(pawnBit) & SideSpecifics::promoRank &
                    Intercepts::getPinRestiction<true>(m_kingSq, src) & legalDestinations(),
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
    if (m_epSquare <= Square::H8)
    {
        SQUARESET_ENUMERATE(
            src,
            Attacks::getPawnAttackerMask(m_epSquare, turn) & pawns,
            {
                // The only legality check we need is the capturer pin check
                if (Attacks::pinCheck(src, SquareSet::square(m_epSquare), m_kingSq, m_pinnedPieces))
                {
                    *i = Move { src, m_epSquare, MoveTypeAndPromotion::EN_PASSANT };
                    ++i;
                }
            });
    }

    return i;
}

template <typename IteratorType, MoveGenType type, typename ParamType>
auto ChessBoard::generateMovesForPawns(
    IteratorType i,
    ParamType legalDestinations) const noexcept -> IteratorType
{
    if (getTurn() == Color::WHITE)
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::WHITE>(i, legalDestinations);
    else
        return generateMovesForPawnsTempl<IteratorType, type, ParamType, Color::BLACK>(i, legalDestinations);
}

template <typename IteratorType, MoveGenType type, typename ParamType>
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

template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
auto ChessBoard::generateMovesForBishop(
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) const noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    const SquareSet dstSquares {
        Attacks::getBishopAttackMask(sq, m_occupancyMask) &
        emptyOrCapture &
        Intercepts::getPinRestiction<pinned>(m_kingSq, sq) &
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

template <typename IteratorType, MoveGenType type, typename ParamType, bool pinned>
auto ChessBoard::generateMovesForRook(
    IteratorType i,
    Square sq,
    ParamType legalDestinations,
    MoveTypeAndPromotion typeAndPromo) const noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };
    const SquareSet dstSquares {
        Attacks::getRookAttackMask(sq, m_occupancyMask) &
        emptyOrCapture &
        Intercepts::getPinRestiction<pinned>(m_kingSq, sq) &
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

template <typename IteratorType>
auto ChessBoard::generateMovesForKing(
    IteratorType i,
    SquareSet attackedSquares) const noexcept -> IteratorType
{
    const SquareSet emptyOrCapture { ~(m_occupancyMask & m_turnColorMask) };

    const SquareSet dstSquares {
        Attacks::getKingAttackMask(m_kingSq) & emptyOrCapture &~ attackedSquares };

    if constexpr (MoveGenIteratorTraits<IteratorType>::storesMoves())
    {
        SQUARESET_ENUMERATE(
            dst,
            dstSquares,
            {
                *i = Move { m_kingSq, dst, MoveTypeAndPromotion::REGULAR_KING_MOVE };
                ++i;
            });
    }
    else
    {
        i += dstSquares.popcount();
    }

    return i;
}

template <MoveGenType type, typename MoveStoreFn, bool shortCastling>
void ChessBoard::generateMovesForCastlingStoreFnTempl(
    SquareSet attackedSquares,
    typename MoveStoreFn::Store &store) const noexcept
{
    // When in check, castling is not legal. Since the caller is supposed to
    // classify the position before calling this function, we'll just ensure
    // here that we're not being called when in check.
    static_assert(type == MoveGenType::NO_CHECK);

    static_assert(static_cast<std::uint8_t>(Color::WHITE) == 0U);
    static_assert(static_cast<std::uint8_t>(Color::BLACK) == 8U);

    using CastlingSideSpecifics = CastlingSideSpecificsTempl<shortCastling>;

    const Color turn { getTurn() };
    const Square sqRook { getCastlingRook(turn, shortCastling) };

    // do we have rights to castle?
    if (sqRook == Square::NONE)
        return;

    const std::uint8_t targetRowAdd = (-static_cast<std::uint8_t>(turn)) & 63U;
    const Square sqKingTarget { static_cast<std::uint8_t>(CastlingSideSpecifics::kingTargetColumn + targetRowAdd) };
    const Square sqRookTarget { static_cast<std::uint8_t>(CastlingSideSpecifics::rookTargetColumn + targetRowAdd) };

    // note: (startSq, endSq]
    const SquareSet kingPathHalfOpen { Intercepts::getInterceptSquares(m_kingSq, sqKingTarget) };
    const SquareSet rookPathHalfOpen { Intercepts::getInterceptSquares(sqRook, sqRookTarget) };

    const SquareSet requiredEmptySquares {
        (kingPathHalfOpen | rookPathHalfOpen) &~ (SquareSet::square(m_kingSq) | SquareSet::square(sqRook)) };

    if (((requiredEmptySquares & m_occupancyMask) |     // all squares between king and rook empty?
         (SquareSet::square(sqRook) & m_pinnedPieces) | // castling rook not pinned?
         (kingPathHalfOpen & attackedSquares)           // king path is not attacked?
            ) != SquareSet::none())
        return;

    if constexpr (shortCastling)
        MoveStoreFn::storeMove(store, Move { m_kingSq, sqRook, MoveTypeAndPromotion::CASTLING_SHORT });
    else
        MoveStoreFn::storeMove(store, Move { m_kingSq, sqRook, MoveTypeAndPromotion::CASTLING_LONG });
}

template <typename IteratorType>
IteratorType ChessBoard::generateAllLegalMovesTemplNoCheck(
    IteratorType i) const noexcept
{
    using ParamType = AllLegalDestinationType;
    constexpr ParamType legalDestinations { };

    i = generateMovesForPawns<IteratorType, MoveGenType::NO_CHECK>(i, legalDestinations);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_knights & m_turnColorMask & ~m_pinnedPieces,
        i = generateMovesForKnight<IteratorType, MoveGenType::NO_CHECK>(i, sq, legalDestinations));

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_bishops & m_turnColorMask & ~m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_rooks & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::NO_CHECK, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
        });

    SQUARESET_ENUMERATE(
        sq,
        m_bishops & m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_rooks & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::NO_CHECK, ParamType, true>(i, sq, legalDestinations, typeAndPromo);
        });

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_rooks & m_turnColorMask & ~m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_bishops & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::NO_CHECK, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
        });

    SQUARESET_ENUMERATE(
        sq,
        m_rooks & m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_bishops & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::NO_CHECK, ParamType, true>(i, sq, legalDestinations, typeAndPromo);
        });

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    // In no check, we'll do the king and castling moves last. This helps the
    // LegalMoveDetectorIterator with early exit, since these are a bit more
    // expensive than the other moves,
    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
            m_pawns &~ m_turnColorMask,
            m_knights &~ m_turnColorMask,
            m_bishops &~ m_turnColorMask,
            m_rooks &~ m_turnColorMask,
            m_oppKingSq,
            getTurn()) };

    i = generateMovesForKing<IteratorType>(i, attackedSquares);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, IteratorStoreMoveFn<IteratorType>, false>(attackedSquares, i);
    generateMovesForCastlingStoreFnTempl<MoveGenType::NO_CHECK, IteratorStoreMoveFn<IteratorType>, true>(attackedSquares, i);

    return i;
}

template <typename IteratorType>
IteratorType ChessBoard::generateAllLegalMovesTemplInCheck(
    IteratorType i) const noexcept
{
    using ParamType = ParametrizedLegalDestinationType;
    ParamType legalDestinations {
        Intercepts::getInterceptSquares(m_kingSq, m_checkers.firstSquare()) };

    // if we're in check, we'll try the king moves first
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

        i = generateMovesForKing<IteratorType>(i, attackedSquares);
        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;
    }

    i = generateMovesForPawns<IteratorType, MoveGenType::CHECK, ParamType>(i, legalDestinations);

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_knights & m_turnColorMask & ~m_pinnedPieces,
        i = generateMovesForKnight<IteratorType, MoveGenType::CHECK, ParamType>(i, sq, legalDestinations));

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_bishops & m_turnColorMask & ~m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_rooks & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
            };
            i = generateMovesForBishop<IteratorType, MoveGenType::CHECK, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
        });

    // Note: pinned pieces cannot resolve checks

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    SQUARESET_ENUMERATE(
        sq,
        m_rooks & m_turnColorMask & ~m_pinnedPieces,
        {
            const MoveTypeAndPromotion typeAndPromo {
                (m_bishops & SquareSet::square(sq)) != SquareSet::none() ?
                MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                MoveTypeAndPromotion::REGULAR_ROOK_MOVE
            };
            i = generateMovesForRook<IteratorType, MoveGenType::CHECK, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
        });

    // Note: pinned pieces cannot resolve checks

    if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
        if (i.hasLegalMoves())
            return i;

    return i;
}

template <typename IteratorType>
IteratorType ChessBoard::generateAllLegalMovesTemplInDoubleCheck(
    IteratorType i) const noexcept
{
    // in double-check, king moves are the only legal moves
    const SquareSet attackedSquares {
        Attacks::determineAttackedSquares(
            m_occupancyMask &~ (m_kings & m_turnColorMask), // remove potentially attacked king
            m_pawns &~ m_turnColorMask,
            m_knights &~ m_turnColorMask,
            m_bishops &~ m_turnColorMask,
            m_rooks &~ m_turnColorMask,
            m_oppKingSq,
            getTurn()) };

    i = generateMovesForKing<IteratorType>(i, attackedSquares);

    return i;
}

template <MoveGenType type, typename IteratorType>
IteratorType ChessBoard::generateMovesIterTempl(
    IteratorType i) const noexcept
{
    if constexpr (type == MoveGenType::NO_CHECK)
    {
        // all destinations are ok as long as the move is ok
        i = generateAllLegalMovesTemplNoCheck<IteratorType>(i);
    }
    else if constexpr (type == MoveGenType::CHECK)
    {
        // must capture the checker or block the check
        i = generateAllLegalMovesTemplInCheck<IteratorType>(i);
    }
    else
    {
        static_assert(type == MoveGenType::DOUBLE_CHECK);

        // king moves only
        i = generateAllLegalMovesTemplInDoubleCheck<IteratorType>(i);
    }

    return i;
}

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
