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

#include "position-compress-fixed.h"
#include "chessboard.h"

#include "gtest/gtest.h"

#include <cstdio>
#include <stdexcept>
#include <string_view>


namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(PositionCompressor, compressDecompress)
{
    std::string_view fens[] {
        // starting position, white to move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",

        // starting position, black to move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1",

        // starting position, white long castling allowed
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Q - 0 1",

        // starting position, white short castling allowed
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 1",

        // starting position, black long castling allowed
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w q - 0 1",

        // starting position, black short castling allowed
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w k - 0 1",

        // white can EP-capture
        "rnbqkbnr/ppp1pp1p/6p1/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",

        // black can EP-capture
        "rnbqkbnr/pp1ppppp/8/8/2pP4/4PN2/PPP2PPP/RNBQKB1R b KQkq d3 0 3",

    };

    ChessBoard board { };
    CompressedPosition_FixedLength compressedPosition { };

    for (std::string_view fen : fens)
    {
        board.loadFEN(fen);

        PositionCompressor_FixedLength::compress(board, compressedPosition);

        ChessBoard tmp { };
        PositionCompressor_FixedLength::decompress(
            compressedPosition,
            board.getHalfMoveClock(),
            moveNumOfPly(board.getCurrentPlyNum()),
            tmp);

        const bool equal { board == tmp };

        if (!equal)
        {
            printf("Compression/decompression cycle failed\n\n");

            printf("Original:\n");
            board.printBoard();

            printf("\nBoard after compress/decompress:\n");
            tmp.printBoard();
        }

        EXPECT_TRUE(equal);
    }
}

TEST(PositionCompressor, basicComparison)
{
    ChessBoard board { };
    CompressedPosition_FixedLength compressedPosition1 { };
    CompressedPosition_FixedLength compressedPosition2 { };

    EXPECT_EQ(compressedPosition1, compressedPosition2);
    EXPECT_TRUE((compressedPosition1 <=> compressedPosition2) == 0);

    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    PositionCompressor_FixedLength::compress(board, compressedPosition1);

    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    PositionCompressor_FixedLength::compress(board, compressedPosition2);

    EXPECT_NE(compressedPosition1, compressedPosition2);
    EXPECT_FALSE((compressedPosition1 <=> compressedPosition2) == 0);
}

TEST(PositionCompressor, compress_tooManyPieces)
{
    ChessBoard board { };
    CompressedPosition_FixedLength compressedPosition { };
    board.loadFEN("rnbqkbnr/pppppppp/p7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_THROW(
        PositionCompressor_FixedLength::compress(board, compressedPosition),
        std::out_of_range);
}

TEST(PositionCompressor, decompress_badFormats)
{
    ChessBoard board { };

    {
        CompressedPosition_FixedLength compressedPosition { };

        // try to decompress a position with 33 pieces
        compressedPosition.occupancy = UINT64_C(0x00000001FFFFFFFF);

        EXPECT_THROW(
            PositionCompressor_FixedLength::decompress(compressedPosition, 0U, 1U, board),
            std::out_of_range);
    }

    {
        CompressedPosition_FixedLength compressedPosition { };

        // forged position with two white castling rooks on the left side of the king
        compressedPosition.occupancy = UINT64_C(0x8000000000000007);

        // white rook that can castle 1
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 0U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 0U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 0U;

        // white rook that can castle 2
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 1U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 1U;

        // white king in turn
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 2U;

        // black king
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 3U;

        EXPECT_THROW(
            PositionCompressor_FixedLength::decompress(compressedPosition, 0U, 1U, board),
            std::out_of_range);
    }

    {
        CompressedPosition_FixedLength compressedPosition { };

        // forged position with two white castling rooks on the right side of the king
        compressedPosition.occupancy = UINT64_C(0x8000000000000007);

        // white king in turn
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 0U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 0U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 0U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 0U;

        // white rook that can castle 1
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 1U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 1U;

        // white rook that can castle 2
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 2U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 2U;

        // black king
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 3U;

        EXPECT_THROW(
            PositionCompressor_FixedLength::decompress(compressedPosition, 0U, 1U, board),
            std::out_of_range);
    }

    {
        CompressedPosition_FixedLength compressedPosition { };

        // forged position with two black castling rooks on the left side of the king
        compressedPosition.occupancy = UINT64_C(0xE000000000000001);

        // white king not in turn
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[1] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[2] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 0U;

        // black rook that can castle 1
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 1U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 1U;

        // black rook that can castle 2
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 2U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 2U;

        // black king
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 3U;

        EXPECT_THROW(
            PositionCompressor_FixedLength::decompress(compressedPosition, 0U, 1U, board),
            std::out_of_range);
    }

    {
        CompressedPosition_FixedLength compressedPosition { };

        // forged position with two black castling rooks on the right side of the king
        compressedPosition.occupancy = UINT64_C(0xE000000000000001);

        // white king not in turn
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[1] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[2] |= UINT64_C(0) << 0U;
        compressedPosition.dataPlanes[3] |= UINT64_C(0) << 0U;

        // black king
        compressedPosition.dataPlanes[0] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 1U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 1U;

        // black rook that can castle 1
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 2U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 2U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 2U;

        // black rook that can castle 2
        compressedPosition.dataPlanes[0] |= UINT64_C(0) << 3U;
        compressedPosition.dataPlanes[1] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[2] |= UINT64_C(1) << 3U;
        compressedPosition.dataPlanes[3] |= UINT64_C(1) << 3U;

        EXPECT_THROW(
            PositionCompressor_FixedLength::decompress(compressedPosition, 0U, 1U, board),
            std::out_of_range);
    }
}

}
