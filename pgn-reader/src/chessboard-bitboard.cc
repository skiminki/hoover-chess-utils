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
#include <limits>
#include <utility>
#include <tuple>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

template <typename IntType>
IntType saturatingIncrease(IntType v)
{
    const IntType inc { v < std::numeric_limits<IntType>::max() };
    return v + inc;
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
        m_epSquare = Square::NONE;
}

void ChessBoard::doMove(const Move m) noexcept
{
    m_epSquare = Square::NONE;

    if (m.isRegularMove()) [[likely]]
    {
        // regular move
        const SquareSet srcSqBit { SquareSet::square(m.getSrc()) };
        const SquareSet dstSqBit { SquareSet::square(m.getDst()) };

        // capture?
        if ((dstSqBit & m_occupancyMask) != SquareSet::none()) [[unlikely]]
        {
            m_halfMoveClock = 0U;

            // clear the destination square. Note: kings cannot be captured
            m_pawns   &= ~dstSqBit;
            m_knights &= ~dstSqBit;
            m_bishops &= ~dstSqBit;
            m_rooks   &= ~dstSqBit;

            Color turn { getTurn() };

            Square &oppLongCastlingRook { getCastlingRookRef(oppositeColor(turn), false) };
            if (m.getDst() == oppLongCastlingRook)
                oppLongCastlingRook = Square::NONE;

            Square &oppShortCastlingRook { getCastlingRookRef(oppositeColor(turn), true) };
            if (m.getDst() == oppShortCastlingRook)
                oppShortCastlingRook = Square::NONE;
        }
        else
            m_halfMoveClock = saturatingIncrease(m_halfMoveClock);

        switch (m.getTypeAndPromotion())
        {
            case MoveTypeAndPromotion::REGULAR_PAWN_MOVE:
                m_pawns   &= ~srcSqBit;
                m_pawns   |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                m_halfMoveClock = 0U;

                if (isPawnDoubleSquareMove(m.getSrc(), m.getDst()))
                {
                    // we'll lit m_epSquare only if there are adjacent opponent pawns to capture it
                    const SquareSet adjacentPawns {
                        ((((m_pawns & ~m_turnColorMask) & ~SquareSet::column(0U)) >> 1U) |
                         (((m_pawns & ~m_turnColorMask) & ~SquareSet::column(7U)) << 1U))
                        & dstSqBit };

                    if (adjacentPawns != SquareSet::none())
                        m_epSquare = static_cast<Square>((getIndexOfSquare(m.getSrc()) + getIndexOfSquare(m.getDst())) / 2U);
                }
                break;

            case MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE:
                m_pawns   &= ~srcSqBit;
                m_pawns   |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                m_halfMoveClock = 0U;
                break;

            case MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE:
                m_knights &= ~srcSqBit;
                m_knights |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                break;

            case MoveTypeAndPromotion::REGULAR_BISHOP_MOVE:
                m_bishops &= ~srcSqBit;
                m_bishops |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                break;

            case MoveTypeAndPromotion::REGULAR_ROOK_MOVE:
                m_rooks &= ~srcSqBit;
                m_rooks |= SquareSet { 1U } << getIndexOfSquare(m.getDst());

                {
                    Color turn { getTurn() };

                    Square &longCastlingRook { getCastlingRookRef(turn, false) };
                    if (m.getSrc() == longCastlingRook)
                        longCastlingRook = Square::NONE;

                    Square &shortCastlingRook { getCastlingRookRef(turn, true) };
                    if (m.getSrc() == shortCastlingRook)
                        shortCastlingRook = Square::NONE;
                }
                break;

            case MoveTypeAndPromotion::REGULAR_QUEEN_MOVE:
                m_bishops &= ~srcSqBit;
                m_bishops |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                m_rooks &= ~srcSqBit;
                m_rooks |= SquareSet { 1U } << getIndexOfSquare(m.getDst());
                break;

            default:
                assert(m.getTypeAndPromotion() == MoveTypeAndPromotion::REGULAR_KING_MOVE);

                // king move
                m_kings   &= ~srcSqBit;
                m_kings   |= SquareSet { 1U } << getIndexOfSquare(m.getDst());

                // reset castling rights
                Color turn { getTurn() };
                setCastlingRook(turn, false, Square::NONE);
                setCastlingRook(turn, true, Square::NONE);
                m_kingSq = m.getDst();
                break;

        }

        // update occupancy mask
        m_occupancyMask ^= srcSqBit;
        m_occupancyMask |= dstSqBit;

        // update turn color mask
        m_turnColorMask |= dstSqBit;
    }
    else if (m.isCastlingMove())
    {
        // castling move

        const std::uint8_t row { rowOf(m.getSrc()) };
        const Square kingSqAfterCastling { makeSquare(getKingColumnAfterCastling(m.getTypeAndPromotion()), row) };
        const Square rookSqAfterCastling { makeSquare(getRookColumnAfterCastling(m.getTypeAndPromotion()), row) };

        // update occupancy mask
        m_occupancyMask ^= SquareSet::square(m.getSrc());
        m_occupancyMask ^= SquareSet::square(m.getDst());
        m_occupancyMask |= SquareSet::square(kingSqAfterCastling);
        m_occupancyMask |= SquareSet::square(rookSqAfterCastling);

        // update turn color mask
        m_turnColorMask |= SquareSet::square(kingSqAfterCastling);
        m_turnColorMask |= SquareSet::square(rookSqAfterCastling);

        // update piece masks
        m_kings ^= SquareSet::square(m.getSrc());
        m_kings |= SquareSet::square(kingSqAfterCastling);
        m_rooks ^= SquareSet::square(m.getDst());
        m_rooks |= SquareSet::square(rookSqAfterCastling);

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

        const SquareSet srcSqBit { SquareSet::square(m.getSrc()) };
        const SquareSet dstSqBit { SquareSet::square(m.getDst()) };
        const SquareSet epSqBit { SquareSet::square(capturedPawnSq) };

        // update occupancy mask
        m_occupancyMask ^= srcSqBit;
        m_occupancyMask ^= epSqBit;
        m_occupancyMask ^= dstSqBit;

        // update turn color mask
        m_turnColorMask |= dstSqBit;

        // update pawn mask
        m_pawns   |= SquareSet::square(m.getDst());
        m_pawns   ^= srcSqBit;
        m_pawns   ^= epSqBit;

        m_halfMoveClock = 0U;
    }
    else
    {
        // promotion move
        assert(m.isPromotionMove());

        const SquareSet srcSqBit { SquareSet::square(m.getSrc()) };
        const SquareSet dstSqBit { SquareSet::square(m.getDst()) };

        // update occupancy mask
        m_occupancyMask ^= srcSqBit;
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
        m_pawns   &= ~dstSqBit;
        m_knights &= ~dstSqBit;
        m_bishops &= ~dstSqBit;
        m_rooks   &= ~dstSqBit;

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

void ChessBoard::calculateMasks(const ArrayBoard &board) noexcept
{
    static_assert(static_cast<std::size_t>(Piece::PAWN)   <= 7U);
    static_assert(static_cast<std::size_t>(Piece::KNIGHT) <= 7U);
    static_assert(static_cast<std::size_t>(Piece::BISHOP) <= 7U);
    static_assert(static_cast<std::size_t>(Piece::ROOK)   <= 7U);
    static_assert(static_cast<std::size_t>(Piece::QUEEN)  <= 7U);
    static_assert(static_cast<std::size_t>(Piece::KING)   <= 7U);

    SquareSet turnColorMask { };
    std::array<SquareSet, 8U> squareSets { };

    const Color turn { getTurn() };

    for (std::uint8_t sqIndex { }; sqIndex < 64U; ++sqIndex)
    {
        const PieceAndColor pc { board[sqIndex] };
        if (pc == PieceAndColor::NONE)
            continue;

        const SquareSet sqBit { SquareSet::square(Square { sqIndex }) };

        if (colorOf(pc) == turn)
            turnColorMask |= sqBit;

        squareSets[static_cast<std::size_t>(pieceOf(pc))] |= sqBit;
    }

    m_turnColorMask = turnColorMask;

    m_pawns   = squareSets[static_cast<std::size_t>(Piece::PAWN)];
    m_knights = squareSets[static_cast<std::size_t>(Piece::KNIGHT)];
    m_bishops =
        squareSets[static_cast<std::size_t>(Piece::BISHOP)] |
        squareSets[static_cast<std::size_t>(Piece::QUEEN)];
    m_rooks   =
        squareSets[static_cast<std::size_t>(Piece::ROOK)] |
        squareSets[static_cast<std::size_t>(Piece::QUEEN)];
    m_kings   = squareSets[static_cast<std::size_t>(Piece::KING)];

    m_occupancyMask = m_pawns | m_knights | m_bishops | m_rooks | m_kings;

    // kings
    m_kingSq    = (m_kings &  m_turnColorMask).firstSquare();
    m_oppKingSq = (m_kings & ~m_turnColorMask).firstSquare();
}

}
