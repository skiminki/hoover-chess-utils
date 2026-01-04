// Hoover Chess Utilities / PGN reader
// Copyright (C) 2022-2025  Sami Kiminki
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

#include "bitboard-attacks.h"
#include "chessboard-movegen.h"
#include "chessboard-priv.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-error.h"

#include <algorithm>
#include <bit>
#include <format>

namespace hoover_chess_utils::pgn_reader
{

namespace
{

void setBoardFirstLineInputValidation(
    Square whiteLongCastleRook, Square whiteShortCastleRook,
    Square blackLongCastleRook, Square blackShortCastleRook,
    Square epSquare,
    std::uint32_t plyNum)
{
    if (!isValidValue(whiteLongCastleRook))
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Illegal whiteLongCastleRook value: {}",
                        static_cast<std::uint8_t>(whiteLongCastleRook)));

    if ((whiteLongCastleRook != Square::NONE) && rowOf(whiteLongCastleRook) != 0U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White long castling rook not on 1st rank");

    if (!isValidValue(whiteShortCastleRook))
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Illegal whiteShortCastleRook value: {}",
                        static_cast<std::uint8_t>(whiteShortCastleRook)));

    if ((whiteShortCastleRook != Square::NONE) && rowOf(whiteShortCastleRook) != 0U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White short castling rook not on 1st rank");

    if (!isValidValue(blackLongCastleRook))
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Illegal blackLongCastleRook value: {}",
                        static_cast<std::uint8_t>(blackLongCastleRook)));

    if ((blackLongCastleRook != Square::NONE) && rowOf(blackLongCastleRook) != 7U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black long castling rook not on 8th rank");

    if (!isValidValue(blackShortCastleRook))
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format("Illegal EP square value: {}",
                        static_cast<std::uint8_t>(epSquare)));

    if ((blackShortCastleRook != Square::NONE) && rowOf(blackShortCastleRook) != 7U)
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black short castling rook not on 8th rank");

    if (plyNum >= makePlyNum(10000U, Color::WHITE))
        throw PgnError(PgnErrorCode::BAD_FEN, "Move number overflow");
}

}

Piece ChessBoard::getSquarePieceNoColor(Square sq) const noexcept
{
    if ((m_occupancyMask & SquareSet { sq }) == SquareSet { })
        return Piece::NONE;

    if ((m_pawns & SquareSet { sq }) != SquareSet { })
        return Piece::PAWN;

    if ((m_knights & SquareSet { sq }) != SquareSet { })
        return Piece::KNIGHT;

    if ((m_kings & SquareSet { sq }) != SquareSet { })
        return Piece::KING;

    if ((m_bishops & m_rooks & SquareSet { sq }) != SquareSet { })
        return Piece::QUEEN;

    if ((m_bishops & SquareSet { sq }) != SquareSet { })
        return Piece::BISHOP;

    return Piece::ROOK;
}

PieceAndColor ChessBoard::getSquarePiece(Square sq) const noexcept
{
    const Piece p { getSquarePieceNoColor(sq) };

    if (p == Piece::NONE)
        return PieceAndColor::NONE;

    SquareSet whitePieces { getTurn() == Color::WHITE ? m_turnColorMask : ~m_turnColorMask };

    return makePieceAndColor(
        p,
        (whitePieces & SquareSet { sq }) != SquareSet { } ? Color::WHITE : Color::BLACK);
}

void ChessBoard::setBoard(
    const ArrayBoard &board,
    Square whiteLongCastleRook, Square whiteShortCastleRook,
    Square blackLongCastleRook, Square blackShortCastleRook,
    Square epSquare,
    std::uint8_t halfMoveClock, std::uint32_t plyNum)
{
    setBoardFirstLineInputValidation(
        whiteLongCastleRook, whiteShortCastleRook,
        blackLongCastleRook, blackShortCastleRook,
        epSquare, plyNum);

    m_plyNum = plyNum;
    m_halfMoveClock = halfMoveClock;
    m_epSquare = epSquare;
    setCastlingRook(Color::WHITE, false, whiteLongCastleRook);
    setCastlingRook(Color::WHITE, true, whiteShortCastleRook);
    setCastlingRook(Color::BLACK, false, blackLongCastleRook);
    setCastlingRook(Color::BLACK, true, blackShortCastleRook);

    calculateMasks(board);
    validateBoard();
}

void ChessBoard::setBoard(
    const BitBoard &board,
    Square whiteLongCastleRook, Square whiteShortCastleRook,
    Square blackLongCastleRook, Square blackShortCastleRook,
    Square epSquare,
    std::uint8_t halfMoveClock, std::uint32_t plyNum)
{
    setBoardFirstLineInputValidation(
        whiteLongCastleRook, whiteShortCastleRook,
        blackLongCastleRook, blackShortCastleRook,
        epSquare, plyNum);

    m_plyNum = plyNum;
    m_halfMoveClock = halfMoveClock;
    m_epSquare = epSquare;
    setCastlingRook(Color::WHITE, false, whiteLongCastleRook);
    setCastlingRook(Color::WHITE, true, whiteShortCastleRook);
    setCastlingRook(Color::BLACK, false, blackLongCastleRook);
    setCastlingRook(Color::BLACK, true, blackShortCastleRook);

    SquareSet occupancyMask { board.pawns };
    SquareSet intersectionMask { };
    m_pawns           = board.pawns;

    intersectionMask |= occupancyMask & board.knights;
    occupancyMask    |= board.knights;
    m_knights         = board.knights;

    intersectionMask |= occupancyMask & board.bishops;
    occupancyMask    |= board.bishops;

    intersectionMask |= occupancyMask & board.rooks;
    occupancyMask    |= board.rooks;

    intersectionMask |= occupancyMask & board.queens;
    occupancyMask    |= board.queens;
    m_bishops         = board.bishops | board.queens;
    m_rooks           = board.rooks   | board.queens;

    intersectionMask |= occupancyMask & board.kings;
    occupancyMask    |= board.kings;
    m_kings           = board.kings;

    if (intersectionMask != SquareSet { })
        throw PgnError(PgnErrorCode::BAD_FEN, "Two pieces occupy the same square");

    m_occupancyMask = occupancyMask;


    m_turnColorMask =
        (getTurn() == Color::WHITE ?
         m_occupancyMask & board.whitePieces : m_occupancyMask & ~board.whitePieces);

    // kings
    m_kingSq    = (m_kings &  m_turnColorMask).firstSquare();
    m_oppKingSq = (m_kings & ~m_turnColorMask).firstSquare();

    validateBoard();
}

void ChessBoard::validateBoard()
{
    // basic checks
    // - single king for both sides
    if ((m_kings & m_turnColorMask).popcount() != 1U)
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format(
                "Illegal position: incorrect number of {} kings",
                getTurn() == Color::WHITE ? "white" : "black"));
    }

    if ((m_kings & ~m_turnColorMask).popcount() != 1U)
    {
        throw PgnError(
            PgnErrorCode::BAD_FEN,
            std::format(
                "Illegal position: incorrect number of {} kings",
                getTurn() == Color::BLACK ? "white" : "black"));
    }

    if (isValidSquare(getWhiteLongCastleRook()) || isValidSquare(getWhiteShortCastleRook()))
    {
        const Square kingSq { getTurn() == Color::WHITE ? m_kingSq : m_oppKingSq };

        if (rowOf(kingSq) != 0U)
            throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White castling rights set but king not on 1st rank");

        const Square longCastleRook { getWhiteLongCastleRook() };
        if (longCastleRook != Square::NONE)
        {
            if ((SquareSet { longCastleRook } & getWhitePieces() & getRooks()) == SquareSet { })
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White long castling rook not found");

            if (longCastleRook >= kingSq)
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White long castling rook is on the short side");
        }

        const Square shortCastleRook { getWhiteShortCastleRook() };
        if (shortCastleRook != Square::NONE)
        {
            if ((SquareSet { shortCastleRook } & getWhitePieces() & getRooks()) == SquareSet { })
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White short castling rook not found");

            if (shortCastleRook <= kingSq)
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: White short castling rook is on the long side");
        }
    }

    if (isValidSquare(getBlackLongCastleRook()) || isValidSquare(getBlackShortCastleRook()))
    {
        const Square kingSq { getTurn() == Color::BLACK ? m_kingSq : m_oppKingSq };

        if (rowOf(kingSq) != 7U)
            throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black castling rights set but king not on 1st rank");

        const Square longCastleRook { getBlackLongCastleRook() };
        if (longCastleRook != Square::NONE)
        {
            if ((SquareSet { longCastleRook } & getBlackPieces() & getRooks()) == SquareSet { })
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black long castling rook not found");

            if (longCastleRook >= kingSq)
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black long castling rook is on the short side");
        }

        const Square shortCastleRook { getBlackShortCastleRook() };
        if (shortCastleRook != Square::NONE)
        {
            if ((SquareSet { shortCastleRook } & getBlackPieces() & getRooks()) == SquareSet { })
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black short castling rook not found");

            if (shortCastleRook <= kingSq)
                throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: Black short castling rook is on the long side");
        }
    }

    // pawns on 1st row?
    if ((m_pawns & SquareSet::row(0U)) != SquareSet { })
    {
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: pawns on 1st rank");
    }

    // pawns on 8th row?
    if ((m_pawns & SquareSet::row(7U)) != SquareSet { })
    {
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: pawns on 8th rank");
    }

    const Color turn { getTurn() };
    const Color oppositeTurn { oppositeColor(turn) };

    // ep square validation
    if (m_epSquare != Square::NONE)
    {
        const std::uint8_t epRow { static_cast<std::uint8_t>(turn == Color::WHITE ? 5U : 2U) };
        const std::uint8_t pawnRow { static_cast<std::uint8_t>(turn == Color::WHITE ? 4U : 3U) };

        // EP square on correct row?
        if (rowOf(m_epSquare) != epRow)
        {
            throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: EP square on wrong rank");
        }

        // There is a corresponding pawn for the EP square?
        const Square pawnSquare { makeSquare(columnOf(m_epSquare), pawnRow) };

        if ((m_pawns & (~m_turnColorMask) & SquareSet { pawnSquare }) == SquareSet { })
        {
            throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: EP square set but no pawn to capture");
        }

        // EP square is empty?
        if ((m_occupancyMask & SquareSet { m_epSquare }) != SquareSet { })
        {
            throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: EP square not empty");
        }

        // Filter out EP pawn if no pawn can capture it. However, this we don't
        // consider an error.
        const SquareSet adjacentPawns {
            ((((m_pawns & m_turnColorMask) & ~SquareSet::column(0U)) >> 1U) |
             (((m_pawns & m_turnColorMask) & ~SquareSet::column(7U)) << 1U))
            & SquareSet { pawnSquare }
        };

        // No adjacent pawns?
        if (adjacentPawns == SquareSet { })
            m_epSquare = Square::NONE;
    }

    updateCheckersAndPins();

    if (Attacks::determineAttackers(
            m_occupancyMask,
            ~m_turnColorMask,
            m_pawns,
            m_knights,
            m_bishops,
            m_rooks,
            m_kings,
            m_oppKingSq,
            oppositeTurn) != SquareSet { })
    {
        throw PgnError(PgnErrorCode::BAD_FEN, "Illegal position: opponent's king in check");
    }
}

}
