// Hoover Chess Utilities / PGN reader
// Copyright (C) 2024-2026  Sami Kiminki
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
#include <limits>
#include <utility>
#include <tuple>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

std::uint_fast8_t saturatingIncrease(std::uint_fast8_t v)
{
    const std::uint_fast8_t inc { v < std::numeric_limits<std::uint8_t>::max() };
    return v + inc;
}

std::uint_fast8_t saturatingIncreaseOrReset(std::uint_fast8_t v, bool reset)
{
    const std::uint_fast8_t resetMask = (static_cast<uint_fast8_t>(reset) - 1U) & 0xFFU;
    const std::uint_fast8_t inc { v < std::numeric_limits<std::uint8_t>::max() };
    return (v + inc) & resetMask;
}

static constexpr inline RowColumn getKingColumnAfterCastling(MoveTypeAndPromotion typeAndPromo) noexcept
{
    static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_SHORT) & 1U) == 0U);
    static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_LONG) & 1U) == 1U);

    return 6U - (static_cast<RowColumn>(typeAndPromo) & 1U) * 4U;
}

static constexpr inline RowColumn getRookColumnAfterCastling(MoveTypeAndPromotion typeAndPromo) noexcept
{
    static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_SHORT) & 1U) == 0U);
    static_assert((static_cast<std::uint8_t>(MoveTypeAndPromotion::CASTLING_LONG) & 1U) == 1U);

    return 5U - (static_cast<RowColumn>(typeAndPromo) & 1U) * 2U;
}

}

void ChessBoard::updateCheckersAndPins() noexcept
{
    // Pawn that can be EP-captured may be considered pinned
    SquareSet epSquareBit { SquareSet::squareOrNone(m_epSquare) };
    SquareSet epCapturable { epSquareBit };

    static_assert(static_cast<int>(Color::WHITE) == 0);
    static_assert(static_cast<int>(Color::BLACK) == 8);
    epCapturable = epCapturable.rotl(static_cast<std::int8_t>(getTurn()) * 2 - 8);

    Attacks::determineCheckersAndPins(
        m_occupancyMask,
        m_turnColorMask,
        m_pawns,
        m_knights,
        m_bishops,
        m_rooks,
        m_epSquare,
        epCapturable,
        m_kingSq,
        getTurn(),
        m_checkers,
        m_pinnedPieces);

    std::uint8_t numCheckers { m_checkers.popcount() };
    if (numCheckers >= 2U)
        numCheckers = 2U;

    m_moveGenFns = &MoveGenFunctionTables::getFunctions(MoveGenType { numCheckers });
    // If EP pawn is pinned, it can never be captured. So, we'll reset it
    if ((m_pinnedPieces & epCapturable) != SquareSet::none())
    {
        m_epSquare = Square::NONE;
        m_pinnedPieces &=~ epCapturable;
    }
}

void ChessBoard::doMove(const Move m) noexcept
{
    m_epSquare = Square::NONE;

    if (m.isRegularMove()) [[likely]]
    {
        // regular move
        const SquareSet srcSqBit { m.getSrc() };
        const SquareSet dstSqBit { m.getDst() };

        // capture?
        if ((dstSqBit & m_occupancyMask) != SquareSet { })
        {
            Color turn { getTurn() };

            m_pawns   &=~ dstSqBit;
            m_knights &=~ dstSqBit;
            m_bishops &=~ dstSqBit;
            m_rooks   &=~ dstSqBit;

            Square &oppLongCastlingRook { getCastlingRookRef(oppositeColor(turn), false) };
            if (m.getDst() == oppLongCastlingRook)
                oppLongCastlingRook = Square::NONE;

            Square &oppShortCastlingRook { getCastlingRookRef(oppositeColor(turn), true) };
            if (m.getDst() == oppShortCastlingRook)
                oppShortCastlingRook = Square::NONE;
        }

        switch (m.getTypeAndPromotion())
        {
            case MoveTypeAndPromotion::REGULAR_PAWN_MOVE:
            {
                const unsigned int srcNum { static_cast<unsigned int>(m.getSrc()) };
                const unsigned int dstNum { static_cast<unsigned int>(m.getDst()) };

                m_pawns   &= ~srcSqBit;
                m_pawns   |=  dstSqBit;

                m_halfMoveClock = 0U;

                // if this was a double square advance, light the EP square if
                // there are potential capturing pawns
                if ((srcNum ^ dstNum) == 16U)
                {
                    const SquareSet adjacentPawns {
                        ((((m_pawns & ~m_turnColorMask) & ~SquareSet::column(0U)) >> 1U) |
                         (((m_pawns & ~m_turnColorMask) & ~SquareSet::column(7U)) << 1U))
                        & dstSqBit };

                    if (adjacentPawns != SquareSet::none())
                        m_epSquare = static_cast<Square>((srcNum + dstNum) / 2U);
                }
            }
            break;

            case MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE:
                m_knights &=~ srcSqBit;
                m_knights |=  dstSqBit;

                m_halfMoveClock = saturatingIncreaseOrReset(
                    m_halfMoveClock, (m_occupancyMask & dstSqBit) != SquareSet { });
                break;

            case MoveTypeAndPromotion::REGULAR_BISHOP_MOVE:
                m_bishops &=~ srcSqBit;
                m_bishops |=  dstSqBit;

                m_halfMoveClock = saturatingIncreaseOrReset(
                    m_halfMoveClock, (m_occupancyMask & dstSqBit) != SquareSet { });
                break;

            case MoveTypeAndPromotion::REGULAR_ROOK_MOVE:
                m_rooks   &=~ srcSqBit;
                m_rooks   |=  dstSqBit;

                {
                    Color turn { getTurn() };

                    Square &longCastlingRook { getCastlingRookRef(turn, false) };
                    if (m.getSrc() == longCastlingRook)
                        longCastlingRook = Square::NONE;

                    Square &shortCastlingRook { getCastlingRookRef(turn, true) };
                    if (m.getSrc() == shortCastlingRook)
                        shortCastlingRook = Square::NONE;
                }

                m_halfMoveClock = saturatingIncreaseOrReset(
                    m_halfMoveClock, (m_occupancyMask & dstSqBit) != SquareSet { });

                break;

            case MoveTypeAndPromotion::REGULAR_QUEEN_MOVE:
                m_bishops &=~ srcSqBit;
                m_rooks   &=~ srcSqBit;

                m_bishops |=  dstSqBit;
                m_rooks   |=  dstSqBit;

                m_halfMoveClock = saturatingIncreaseOrReset(
                    m_halfMoveClock, (m_occupancyMask & dstSqBit) != SquareSet { });
                break;

            default:
                assert(m.getTypeAndPromotion() == MoveTypeAndPromotion::REGULAR_KING_MOVE);

                // king move
                m_kings   &=~ srcSqBit;
                m_kings   |=  dstSqBit;

                // reset castling rights
                Color turn { getTurn() };
                setCastlingRook(turn, false, Square::NONE);
                setCastlingRook(turn, true, Square::NONE);
                m_kingSq = m.getDst();

                m_halfMoveClock = saturatingIncreaseOrReset(
                    m_halfMoveClock, (m_occupancyMask & dstSqBit) != SquareSet { });

                break;

        }

        // update occupancy mask
        m_occupancyMask &=~ srcSqBit;
        m_occupancyMask |=  dstSqBit;

        // update turn color mask
        m_turnColorMask |=  dstSqBit;
    }
    else if (m.isCastlingMove())
    {
        // castling move

        const RowColumn row { rowOf(m.getSrc()) };
        const Square kingSqAfterCastling { makeSquare(getKingColumnAfterCastling(m.getTypeAndPromotion()), row) };
        const Square rookSqAfterCastling { makeSquare(getRookColumnAfterCastling(m.getTypeAndPromotion()), row) };

        // update occupancy mask
        m_occupancyMask &=~ SquareSet { m.getSrc() };
        m_occupancyMask &=~ SquareSet { m.getDst() };
        m_occupancyMask |=  SquareSet { kingSqAfterCastling };
        m_occupancyMask |=  SquareSet { rookSqAfterCastling };

        // update turn color mask
        m_turnColorMask |= SquareSet { kingSqAfterCastling };
        m_turnColorMask |= SquareSet { rookSqAfterCastling };

        // update piece masks
        m_kings &=~ SquareSet { m.getSrc() };
        m_kings |=  SquareSet { kingSqAfterCastling };
        m_rooks &=~ SquareSet { m.getDst() };
        m_rooks |=  SquareSet { rookSqAfterCastling };

        m_halfMoveClock = saturatingIncrease(m_halfMoveClock);

        // reset castling rights
        Color turn { getTurn() };
        setCastlingRook(turn, false, Square::NONE);
        setCastlingRook(turn, true, Square::NONE);

        m_kingSq = kingSqAfterCastling;
    }
    else if (m.getTypeAndPromotion() == MoveTypeAndPromotion::EN_PASSANT)
    {
        // captured pawn location
        const Square capturedPawnSq { makeSquare(columnOf(m.getDst()), rowOf(m.getSrc())) };

        const SquareSet srcSqBit { m.getSrc() };
        const SquareSet dstSqBit { m.getDst() };
        const SquareSet epSqBit { capturedPawnSq };

        // update occupancy mask
        m_occupancyMask &=~ srcSqBit;
        m_occupancyMask &=~ epSqBit;
        m_occupancyMask |=  dstSqBit;

        // update turn color mask
        m_turnColorMask |= dstSqBit;

        // update pawn mask
        m_pawns  |=  SquareSet { m.getDst() };
        m_pawns  &=~ srcSqBit;
        m_pawns  &=~ epSqBit;

        m_halfMoveClock = 0U;
    }
    else
    {
        // promotion move
        assert(m.isPromotionMove());

        const SquareSet srcSqBit { m.getSrc() };
        const SquareSet dstSqBit { m.getDst() };

        // update occupancy mask
        m_occupancyMask &=~ srcSqBit;
        m_occupancyMask |= dstSqBit;

        // update turn color mask
        m_turnColorMask |= dstSqBit;

        // the moving piece
        SquareSet knightBit { };
        SquareSet bishopBit { };
        SquareSet rookBit   { };

        const Piece promoPiece { m.getPromotionPiece() };

        knightBit = SquareSet { static_cast<std::uint64_t>(promoPiece == Piece::KNIGHT) };
        bishopBit = SquareSet {
            static_cast<std::uint64_t>(promoPiece == Piece::BISHOP || promoPiece == Piece::QUEEN) };
        rookBit   = SquareSet {
            static_cast<std::uint64_t>(promoPiece == Piece::ROOK   || promoPiece == Piece::QUEEN) };

        // clear source square of piece masks
        m_pawns   ^=  srcSqBit;

        // clear dest square of piece masks
        m_pawns   &=~ dstSqBit;
        m_knights &=~ dstSqBit;
        m_bishops &=~ dstSqBit;
        m_rooks   &=~ dstSqBit;

        // set dest square of piece masks
        m_knights |= knightBit << getIndexOfSquare(m.getDst());
        m_bishops |= bishopBit << getIndexOfSquare(m.getDst());
        m_rooks   |= rookBit   << getIndexOfSquare(m.getDst());

        m_halfMoveClock = 0U;

        // reset castling rights
        Color turn { getTurn() };
        Square &oppLongCastlingRook { getCastlingRookRef(oppositeColor(turn), false) };
        if (m.getDst() == oppLongCastlingRook)
            oppLongCastlingRook = Square::NONE;

        Square &oppShortCastlingRook { getCastlingRookRef(oppositeColor(turn), true) };
        if (m.getDst() == oppShortCastlingRook)
            oppShortCastlingRook = Square::NONE;
    }

    // move applied, switch sides
    ++m_plyNum;
    std::swap(m_kingSq, m_oppKingSq);
    m_turnColorMask = (~m_turnColorMask) & m_occupancyMask;

    // update checkers and pinners for this turn
    updateCheckersAndPins();
}

}
