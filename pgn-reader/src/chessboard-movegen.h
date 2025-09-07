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

        assert(index < moveGenFunctions.size());
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

template <MoveGenType type>
SquareSet ChessBoard::blocksAllChecksMaskTempl(Square dst) const noexcept
{
    if constexpr (type == MoveGenType::NO_CHECK)
        return SquareSet::all(); // no checks to block
    else
    {
        static_assert(type == MoveGenType::CHECK);
        return
            (Intercepts::getInterceptSquares(m_kingSq, m_checkers.firstSquare()) & SquareSet::square(dst)).allIfAny();
    }
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

template <typename IteratorType, MoveGenType type, typename ParamType>
IteratorType ChessBoard::generateAllLegalMovesTempl(
    IteratorType i,
    ParamType legalDestinations) const noexcept
{
    // if we're in check, we'll try the king moves first
    if constexpr (type != MoveGenType::NO_CHECK)
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
            m_knights & m_turnColorMask & ~m_pinnedPieces,
            i = generateMovesForKnight<IteratorType, type, ParamType>(i, sq, legalDestinations));

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
                i = generateMovesForBishop<IteratorType, type, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
            });

        // pinned pieces cannot resolve checks
        if constexpr (type == MoveGenType::NO_CHECK)
        {
            SQUARESET_ENUMERATE(
                sq,
                m_bishops & m_turnColorMask & m_pinnedPieces,
                {
                    const MoveTypeAndPromotion typeAndPromo {
                        (m_rooks & SquareSet::square(sq)) != SquareSet::none() ?
                        MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                        MoveTypeAndPromotion::REGULAR_BISHOP_MOVE
                    };
                    i = generateMovesForBishop<IteratorType, type, ParamType, true>(i, sq, legalDestinations, typeAndPromo);
                });
        }

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
                i = generateMovesForRook<IteratorType, type, ParamType, false>(i, sq, legalDestinations, typeAndPromo);
            });

        // pinned pieces cannot resolve checks
        if constexpr (type == MoveGenType::NO_CHECK)
        {
            SQUARESET_ENUMERATE(
                sq,
                m_rooks & m_turnColorMask & m_pinnedPieces,
                {
                    const MoveTypeAndPromotion typeAndPromo {
                        (m_bishops & SquareSet::square(sq)) != SquareSet::none() ?
                        MoveTypeAndPromotion::REGULAR_QUEEN_MOVE :
                        MoveTypeAndPromotion::REGULAR_ROOK_MOVE
                    };
                    i = generateMovesForRook<IteratorType, type, ParamType, true>(i, sq, legalDestinations, typeAndPromo);
                });
        }

        if constexpr (MoveGenIteratorTraits<IteratorType>::canCompleteEarly)
            if (i.hasLegalMoves())
                return i;

        // In no check, we'll do the king and castling moves last. This helps the
        // LegalMoveDetectorIterator with early exit, since these are a bit more
        // expensive than the other moves,
        if constexpr (type == MoveGenType::NO_CHECK)
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

            generateMovesForCastlingStoreFnTempl<type, IteratorStoreMoveFn<IteratorType>, false>(attackedSquares, i);
            generateMovesForCastlingStoreFnTempl<type, IteratorStoreMoveFn<IteratorType>, true>(attackedSquares, i);

            return i;
        }
        else
            return i;
    }
}

template <MoveGenType type, typename IteratorType>
IteratorType ChessBoard::generateMovesIterTempl(
    IteratorType i) const noexcept
{
    if constexpr (type == MoveGenType::NO_CHECK)
    {
        // all destinations are ok as long as the move is ok
        i = generateAllLegalMovesTempl<IteratorType, MoveGenType::NO_CHECK, AllLegalDestinationType>(
            i, AllLegalDestinationType { });
    }
    else if constexpr (type == MoveGenType::CHECK)
    {
        // must capture the checker or block the check
        i = generateAllLegalMovesTempl<IteratorType, MoveGenType::CHECK, ParametrizedLegalDestinationType>(
            i,
            ParametrizedLegalDestinationType {
                Intercepts::getInterceptSquares(m_kingSq, m_checkers.firstSquare()) });
    }
    else
    {
        static_assert(type == MoveGenType::DOUBLE_CHECK);

        // king moves only
        i = generateAllLegalMovesTempl<IteratorType, MoveGenType::DOUBLE_CHECK, NoLegalDestinationType>(
            i, NoLegalDestinationType { });
    }

    return i;
}

}

#endif
