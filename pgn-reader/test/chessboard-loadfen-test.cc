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
#include "pgnreader-error.h"
#include "pgnreader-string-utils.h"

#include "gtest/gtest.h"


namespace hoover_chess_utils::pgn_reader::unit_test
{

#define TEST_EXPECT_THROW_PGN_ERROR(st) \
    EXPECT_THROW(                       \
    try {                               \
        st;                             \
    }                                   \
    catch (const PgnError &e)           \
    {                                   \
        printf("- %s\n", e.what());     \
        throw;                          \
    }, PgnError)                        \



TEST(ChessBoard, loadFEN_illegalInputs)
{
    // something wrong with the input
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - "));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - A 99"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1 0"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1A 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 257 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 2"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 100000"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 4294967296"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/ppp\x01pppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/ppp\xFFpppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 A"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppEpppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkX - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/k7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/K7/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));

    // truncated input
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN(""));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR "));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w "));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR wx"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K-"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -K"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K "));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K 8"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K e"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K e9"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K e5"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K e53"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K e6 "));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 "));

    // castling issues
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w KQkq - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w KQkq - 0 1"));

    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KK - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w QQ - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w kk - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w qq - 0 1"));

    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w K - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w Q - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w k - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w q - 0 1"));

    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w H - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w A - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w h - 0 1"));
    TEST_EXPECT_THROW_PGN_ERROR(ChessBoard { }.loadFEN("1nbqqbn1/pppkpppp/8/8/8/8/PPPKPPPP/1NBQQBN1 w a - 0 1"));
}

TEST(ChessBoard, loadFEN_CastlingFRC)
{
    std::array<std::string_view, 24U> castlingRookFENs {
        // white short castling, all legal rook positions
        "1k6/8/8/8/8/8/8/1KR5 w C - 0 1",
        "1k6/8/8/8/8/8/8/1K1R4 w D - 0 1",
        "1k6/8/8/8/8/8/8/1K2R3 w E - 0 1",
        "1k6/8/8/8/8/8/8/1K3R2 w F - 0 1",
        "1k6/8/8/8/8/8/8/1K4R1 w G - 0 1",
        "1k6/8/8/8/8/8/8/1K5R w H - 0 1",

        // white long castling, all legal rook positions
        "6k1/8/8/8/8/8/8/R5K1 w A - 0 1",
        "6k1/8/8/8/8/8/8/1R4K1 w B - 0 1",
        "6k1/8/8/8/8/8/8/2R3K1 w C - 0 1",
        "6k1/8/8/8/8/8/8/3R2K1 w D - 0 1",
        "6k1/8/8/8/8/8/8/4R1K1 w E - 0 1",
        "6k1/8/8/8/8/8/8/5RK1 w F - 0 1",

        // black short castling, all legal rook positions
        "1kr5/8/8/8/8/8/8/1K6 w c - 0 1",
        "1k1r4/8/8/8/8/8/8/1K6 w d - 0 1",
        "1k2r3/8/8/8/8/8/8/1K6 w e - 0 1",
        "1k3r2/8/8/8/8/8/8/1K6 w f - 0 1",
        "1k4r1/8/8/8/8/8/8/1K6 w g - 0 1",
        "1k5r/8/8/8/8/8/8/1K6 w h - 0 1",

        // black long castling, all legal rook positions
        "r5k1/8/8/8/8/8/8/6K1 w a - 0 1",
        "1r4k1/8/8/8/8/8/8/6K1 w b - 0 1",
        "2r3k1/8/8/8/8/8/8/6K1 w c - 0 1",
        "3r2k1/8/8/8/8/8/8/6K1 w d - 0 1",
        "4r1k1/8/8/8/8/8/8/6K1 w e - 0 1",
        "5rk1/8/8/8/8/8/8/6K1 w f - 0 1",
    };

    for (std::string_view fen : castlingRookFENs)
    {
        EXPECT_NO_THROW(ChessBoard { }.loadFEN(fen));
    }
}

}
