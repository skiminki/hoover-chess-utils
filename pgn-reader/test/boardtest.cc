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
#include "pgnreader-error.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>


namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(BoardTest, LoadStartPos)
{
    ChessBoard board { };

    board.printBoard();

    // board setup
    const ArrayBoard expect {

            PieceAndColorCompact::WHITE_ROOK, PieceAndColorCompact::WHITE_KNIGHT, PieceAndColorCompact::WHITE_BISHOP, PieceAndColorCompact::WHITE_QUEEN, PieceAndColorCompact::WHITE_KING, PieceAndColorCompact::WHITE_BISHOP, PieceAndColorCompact::WHITE_KNIGHT, PieceAndColorCompact::WHITE_ROOK,
            PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN, PieceAndColorCompact::WHITE_PAWN,
            PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE,
            PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE,
            PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE,
            PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE, PieceAndColorCompact::NONE,
            PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN, PieceAndColorCompact::BLACK_PAWN,
            PieceAndColorCompact::BLACK_ROOK, PieceAndColorCompact::BLACK_KNIGHT, PieceAndColorCompact::BLACK_BISHOP, PieceAndColorCompact::BLACK_QUEEN, PieceAndColorCompact::BLACK_KING, PieceAndColorCompact::BLACK_BISHOP, PieceAndColorCompact::BLACK_KNIGHT, PieceAndColorCompact::BLACK_ROOK,

        };

    for (std::size_t i = 0U; i < expect.size(); ++i)
    {
        EXPECT_EQ(toFastType(expect[i]), board.getSquarePiece(getSquareForIndex(i)));
    }

    // move counters
    EXPECT_EQ(0U,                       board.getHalfMoveClock());
    EXPECT_EQ(0U,                       board.getCurrentPlyNum());

    // EP
    EXPECT_EQ(Square::NONE, board.getEpSquare());

    // castling
    EXPECT_EQ(Square::A1, board.getWhiteLongCastleRook());
    EXPECT_EQ(Square::H1, board.getWhiteShortCastleRook());
    EXPECT_EQ(Square::A8, board.getBlackLongCastleRook());
    EXPECT_EQ(Square::H8, board.getBlackShortCastleRook());

    // classification
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
}

// informal, non-verifying test
TEST(BoardTest, printBoard)
{
    ChessBoard board { };

    // display checkers
    board.loadFEN("4k2r/8/8/8/7b/4r3/8/R3K2R w KQk - 0 1");
    board.printBoard();

    // show EP
    board.loadFEN("rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
    board.printBoard();

    // black's turn
    board.loadFEN("rnbqkbnr/ppp2ppp/4p3/3pP3/3P4/8/PPP2PPP/RNBQKBNR b KQkq - 0 3");
    board.printBoard();
}

}
