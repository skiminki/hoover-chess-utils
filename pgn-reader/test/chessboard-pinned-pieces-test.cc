// Hoover Chess Utilities / PGN reader
// Copyright (C) 2025  Sami Kiminki
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
#include "src/chessboard-priv.h"

#include "gtest/gtest.h"

namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(ChessBoard, pinnedPieces)
{
    ChessBoard board { };

    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet { });

    board.loadFEN("rnbqkbnr/pppp1ppp/8/4p2Q/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 1 2");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet(Square::F7));

    board.loadFEN("rnbqk1nr/ppppbppp/8/4Q3/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet(Square::E7));

    board.loadFEN("rnbqkb1r/pppp1ppp/5n2/4Q3/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet { });
    EXPECT_EQ(board.getCheckers(), SquareSet(Square::E5));

    board.loadFEN("r1bqkb1r/ppp2ppp/2np1n2/8/4P3/5N2/PPPPQPPP/RNB1KB1R b KQkq - 3 5");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet { });

    board.loadFEN("1k6/7b/8/4Pp2/8/8/2K5/8 w - f6 0 1");
    EXPECT_EQ(board.getPinnedPieces(), SquareSet { });
    EXPECT_EQ(board.getEpSquare(), Square::NONE);
}

}
