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
#include "pgnreader-string-utils.h"

#include "gtest/gtest.h"

#include "chessboard-test-playmove-helper.h"


namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(MoveGen, CastlingInCheckNotAllowed)
{
    ChessBoard board { };

    // 1. e4 e5 2. Bc4 Nc6 3. Nf3 Nf6 4. d3 Bb4+ <try short castle> 5. c3 Ba5 6. O-O-O
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E4);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E5);
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::C4);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::C6);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F3);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F6);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::D3);
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::B4);

    PLAY_MOVE_EXPECT_NO_MOVES(board, ShortCastling);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C3);
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::A5);
    PLAY_MOVE(board, ShortCastling);

    board.loadStartPos();

    // 1. d4 d5 2. Bf4 e6 3. Na3 Nc6 4. Qd3 Bb4+ <try long castle> 5. c3 Nf6 6. O-O-O
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::D4);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::D5);
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::F4);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E6);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::A3);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::C6);
    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::D3);
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::B4);
    PLAY_MOVE_EXPECT_NO_MOVES(board, LongCastling);
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C3);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F6);
    PLAY_MOVE(board, LongCastling);
}

TEST(MoveGen, canEpCapture)
{
    ChessBoard board { };

    EXPECT_FALSE(board.canEpCapture());

    // 1. c4 c6 2. c5 Qb6 3. Nh3 h6 4. g3 Nf6 5. Bg2 a6 6. O-O g6 7. f4 d5 { cannot EP capture due to pin} 8. f5 e5 9. fxe6
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C4);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C5);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::B6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::H3);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::H6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::G3);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::G2);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::A6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, ShortCastling);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::G6);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::F4);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::D5);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::square(Square::C5), Square::D6);

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::F5);
    EXPECT_FALSE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E5);
    EXPECT_TRUE(board.canEpCapture());

    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::square(Square::F5), Square::E6);
    EXPECT_FALSE(board.canEpCapture());

    // special cases
    // in check but not by EP
    board.loadFEN("1r2k3/8/8/5Pp1/8/8/1K6/8 w - g6 0 1");
    EXPECT_FALSE(board.canEpCapture());
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // in check by EP
    board.loadFEN("4k3/8/8/5Pp1/7K/8/8/8 w - g6 0 1");
    EXPECT_TRUE(board.canEpCapture());
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP pawn is pinned (EP not possible)
    board.loadFEN("3bk3/8/8/5Pp1/7K/8/8/8 w - g6 0 1");
    EXPECT_FALSE(board.canEpCapture());
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP capture exposes king horizontally to rook
    board.loadFEN("3bk3/8/8/1r3PpK/8/8/8/8 w - g6 0 1");
    EXPECT_FALSE(board.canEpCapture());
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP capture, king on same rank, but not exposing to rook
    board.loadFEN("4k3/8/8/1n1Pp2K/8/8/8/8 w - e6 0 1");
    EXPECT_TRUE(board.canEpCapture());
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::E6);

    // EP capture, two rooks (no exposure)
    board.loadFEN("4k3/8/8/1r3Ppr/8/8/5K2/8 w - g6 0 1");
    EXPECT_TRUE(board.canEpCapture());
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP capture, capturing pawn pinned but ok
    board.loadFEN("4k3/7b/8/5Pp1/8/3K4/8/8 w - g6 0 1");
    EXPECT_TRUE(board.canEpCapture());
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP capture, capturing pawn pinned but not ok
    board.loadFEN("4k3/3b4/8/5Pp1/8/7K/8/8 w - - 0 1");
    EXPECT_FALSE(board.canEpCapture());
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::all(), Square::G6);

    // EP capture, two pawns: one pinned, one not
    board.loadFEN("4k3/3b4/8/5PpP/8/7K/8/8 w - g6 0 1");
    EXPECT_TRUE(board.canEpCapture());
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::G6);
}

TEST(MoveGen, halfMoveClock)
{
    ChessBoard board { };

    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    // 1. d4 c5 2. dxc5 Nf6 3. Nf3 e6 4. c6 Be7 5. cxb7 O-O 6. bxa8=Q Bb7 7. Qxb7
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::D4);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C5);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::C5);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F6);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 1U);

    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F3);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 2U);

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E6);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::C6);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::E7);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 1U);

    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::B7);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, ShortCastling);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 1U);

    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::A8, Piece::QUEEN);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);

    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::B7);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 1U);

    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::B7);
    EXPECT_EQ(unsigned { board.getHalfMoveClock() }, 0U);
}

TEST(MoveGen, castlingRights)
{
    ChessBoard board { };

    // white king move - reset both castling rooks
    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    PLAY_MOVE(board, KingAndDest, SquareSet::all(), Square::E2);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook move - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::A2);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::H2);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook capture - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/1b4b1/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::A1);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook capture - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/1b4b1/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::H1);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook capture by promo - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/1b4b1/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::A1);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook capture by promo - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/1p4p1/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::H1, Piece::KNIGHT);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // white rook capture by promo - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/1p4p1/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::A1, Piece::KNIGHT);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // black king move - reset both castling rooks
    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    PLAY_MOVE(board, KingAndDest, SquareSet::all(), Square::E7);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::NONE));

    // black rook move - reset individual castling right
    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::A7);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    board.loadFEN("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::H7);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::NONE));

    // black rook capture - reset individual castling right
    board.loadFEN("r3k2r/1B4B1/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::A8);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));

    // black rook capture - reset individual castling right
    board.loadFEN("r3k2r/1B4B1/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::H8);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::NONE));


    // black rook capture by promo - reset individual castling right
    board.loadFEN("r3k2r/1P4P1/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::H8, Piece::KNIGHT);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::A8));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::NONE));

    // black rook capture by promo - reset individual castling right
    board.loadFEN("r3k2r/1P4P1/8/8/8/8/8/R3K2R w KQkq - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::A8, Piece::KNIGHT);
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteLongCastleRook()), static_cast<unsigned>(Square::A1));
    EXPECT_EQ(static_cast<unsigned>(board.getWhiteShortCastleRook()), static_cast<unsigned>(Square::H1));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackLongCastleRook()), static_cast<unsigned>(Square::NONE));
    EXPECT_EQ(static_cast<unsigned>(board.getBlackShortCastleRook()), static_cast<unsigned>(Square::H8));
}

TEST(MoveGen, illegalMoves)
{
    ChessBoard board { };

    // checked by knight, rook cannot move (unless it would capture the knight)
    board.loadFEN("8/8/3k4/8/4n3/R7/3K4/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, RookAndDest, SquareSet::all(), Square::D3);

    // Pawn advance, but exposes a check
    board.loadFEN("4k3/8/8/8/8/8/r2PK3/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestNoCapture, SquareSet::all(), Square::D3);

    // Pawn advance, but exposes a check
    board.loadFEN("4k3/8/8/8/8/8/r2PK3/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestNoCapture, SquareSet::all(), Square::D4);

    // Pawn promo advance, but exposes a check
    board.loadFEN("8/r2PK3/8/7k/8/8/8/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestPromoNoCapture, SquareSet::all(), Square::D8, Piece::QUEEN);

    // Pawn promo advance with capture, but exposes a check
    board.loadFEN("2r5/r2PK3/8/7k/8/8/8/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestPromoCapture, SquareSet::all(), Square::C8, Piece::QUEEN);

    // Pawn advance, but blocked by a piece
    board.loadFEN("8/2b5/2P5/3K3k/8/8/8/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestNoCapture, SquareSet::all(), Square::C7);

    // Pawn promo advance, but blocked by a piece
    board.loadFEN("2b5/2P5/3K4/7k/8/8/8/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestPromoNoCapture, SquareSet::all(), Square::C8, Piece::KNIGHT);

    // EP capture but we're under a check
    board.loadFEN("1k2r3/8/8/2pP4/8/8/8/4K3 w - c6 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, PawnAndDestCapture, SquareSet::all(), Square::C6);

    // King walking into a check
    board.loadFEN("8/r7/3K4/7k/8/8/8/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, KingAndDest, SquareSet::all(), Square::D7);

    // King capturing own piece
    board.loadFEN("1k6/8/8/8/8/4P3/4K3/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, KingAndDest, SquareSet::all(), Square::E3);

    // Knight move in check, not blocking
    board.loadFEN("1k6/6b1/8/8/8/3N4/1K6/8 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, KnightAndDest, SquareSet::all(), Square::F4);

    // Double-check and one check is blocked
    board.loadFEN("k7/8/6b1/1r6/8/8/8/1KN5 w - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, KnightAndDest, SquareSet::all(), Square::B3);
    PLAY_MOVE_EXPECT_NO_MOVES(board, KnightAndDest, SquareSet::all(), Square::D3);

    // Pinned knight
    board.loadFEN("7k/8/6b1/8/4N3/3K4/8/8 b - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, BishopAndDest, SquareSet::all(), Square::G3);

    // Pinned bishop
    board.loadFEN("7k/8/6b1/8/4B3/3K4/8/8 b - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, BishopAndDest, SquareSet::all(), Square::F3);

    // Pinned rook
    board.loadFEN("7k/8/6b1/8/4R3/3K4/8/8 b - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, RookAndDest, SquareSet::all(), Square::E3);

    // Pinned queen
    board.loadFEN("7k/8/6b1/8/4Q3/3K4/8/8 b - - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, QueenAndDest, SquareSet::all(), Square::E3);

    // Short castling over checked squares
    board.loadFEN("2k5/8/8/8/2b5/8/8/4K2R w K - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, ShortCastling);

    // Long castling over checked squares
    board.loadFEN("2k5/8/8/8/6b1/8/8/R3K3 w Q - 0 1");
    PLAY_MOVE_EXPECT_NO_MOVES(board, LongCastling);
}

// cases not covered by other tests
TEST(MoveGen, specialCases)
{
    ChessBoard board { };

    // plain old pawn promotion
    board.loadFEN("8/2P5/3K4/7k/8/8/8/8 w - - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoNoCapture, SquareSet::all(), Square::C8, Piece::KNIGHT);

    // check intercepted by a knight, legal move detector
    board.loadFEN("8/1b5k/8/8/8/8/7P/6NK w - - 0 1");
    EXPECT_EQ(board.determineStatus(), PositionStatus::CHECK);

    // check intercepted by a bishop, legal move detector
    board.loadFEN("1b6/7k/8/8/8/7P/3B2BK/r7 w - - 0 1");
    EXPECT_EQ(board.determineStatus(), PositionStatus::CHECK);

    // check intercepted by a rook, legal move detector
    board.loadFEN("8/1b5k/8/8/8/8/7P/6RK w - - 0 1");
    EXPECT_EQ(board.determineStatus(), PositionStatus::CHECK);

    // only king can move, legal move detector
    board.loadFEN("8/8/8/7k/8/8/8/1q4BK w - - 0 1");
    EXPECT_EQ(board.determineStatus(), PositionStatus::NORMAL);

    // double check, legal move detector
    board.loadFEN("8/8/1k2q3/3P1P2/6P1/2b4P/PPP5/4K3 w - - 0 1");
    EXPECT_EQ(board.determineStatus(), PositionStatus::CHECK);

    // doubled pawns on 2nd/3rd ranks, pawn to 4th rank (point of view)
    board.loadFEN("rnbqkbnr/1pp1pppp/4p3/p7/8/P7/1PPP1PPP/RNBQKBNR b KQkq - 0 4");
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::E5);

    // promo capture right
    board.loadFEN("1k2r3/3PK3/8/8/8/8/8/8 w - - 0 1");
    PLAY_MOVE(board, PawnAndDestPromoCapture, SquareSet::all(), Square::E8, Piece::ROOK);

    // EP is the only legal move
    board.loadFEN("k4r2/8/8/6Pp/6K1/8/7q/8 w - h6 0 1");
    EXPECT_EQ(PositionStatus::CHECK, board.determineStatus());

    // EP is not a legal move, since it's pinned
    board.loadFEN("k4rr1/8/8/6Pp/6K1/8/7q/8 w - - 0 1");
    EXPECT_EQ(PositionStatus::MATE, board.determineStatus());

    // double-check mate
    board.loadFEN("7K/7r/5qkn/8/P7/8/8/8 w - - 0 1");
    EXPECT_EQ(PositionStatus::MATE, board.determineStatus());

    // pawn move interrupting check
    board.loadFEN("2k5/2b5/8/8/8/7P/r1P3PK/6NR w - - 0 1");
    EXPECT_EQ(PositionStatus::CHECK, board.determineStatus());
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::G3);

    // pawn move forced
    board.loadFEN("8/8/1b5k/8/8/8/6PP/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, PawnAndDestNoCapture, SquareSet::all(), Square::G3);

    // knight move forced
    board.loadFEN("8/8/1b5k/8/6p1/6Pp/2N4P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::D4);

    // bishop move forced
    board.loadFEN("8/8/1b5k/8/6p1/6Pp/2B4P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::D3);

    // rook move forced
    board.loadFEN("8/8/1b5k/8/6p1/6Pp/2R4P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::C3);

    // queen move forced
    board.loadFEN("8/8/1b5k/8/6p1/6Pp/2Q4P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::C3);

    // queen move forced
    board.loadFEN("8/8/1b5k/8/6p1/6Pp/2Q4P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::D3);

    // no moves, all piece types (except queen)
    board.loadFEN("8/8/8/8/1p2k1p1/pPb1PpPp/P1P2P1P/RB3KBN w - - 0 1");
    EXPECT_EQ(PositionStatus::STALEMATE, board.determineStatus());

    // only queen can move (bishop-like) (no check)
    board.loadFEN("8/8/8/1b6/4k1p1/4PpPp/5P1P/3Q2KN w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // only queen can move (rook-like) (no check)
    board.loadFEN("8/8/8/1p5k/1P6/1P6/QP6/KN5r w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // only a pinned pawn can advance
    board.loadFEN("k5rr/8/8/8/8/8/7P/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // only a pinned EP capture is possible
    board.loadFEN("k5r1/1b6/8/2pP4/8/8/5r2/7K w - c6 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // pinned EP capture, not possible
    board.loadFEN("k5r1/1b6/8/3Pp3/8/8/5r2/7K w - - 0 1");
    EXPECT_EQ(PositionStatus::STALEMATE, board.determineStatus());

    // white has castling rights (short) but is stalemated
    board.loadFEN("k7/8/8/8/5p1p/4pP1P/3nPn1P/4K1NR w K - 0 1");
    EXPECT_EQ(PositionStatus::STALEMATE, board.determineStatus());

    // white short castling is the only move
    board.loadFEN("2k5/8/8/1b6/8/5p1p/5P1P/6KR w K - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // white has castling rights (long) but is stalemated
    board.loadFEN("k7/8/8/8/p1p1p2p/P1PpP1pP/Pn1P1nb1/RN2K3 w Q - 0 1");
    EXPECT_EQ(PositionStatus::STALEMATE, board.determineStatus());

    // white has castling rights (long) but is stalemated 2
    board.loadFEN("2k5/8/8/6b1/8/p1p5/P1P5/RK6 w Q - 0 1");
    EXPECT_EQ(PositionStatus::STALEMATE, board.determineStatus());

    // white long castling is the only move
    board.loadFEN("2k5/4r3/8/8/8/p1p5/P1P5/1BRK4 w Q - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // Pinned bishop only moves
    board.loadFEN("2k5/8/8/3bbb2/8/8/1B6/K7 w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // Pinned queen only moves
    board.loadFEN("2k5/8/8/3bbb2/8/8/1Q6/K7 w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // Pinned rook only moves
    board.loadFEN("rrk5/8/8/8/8/8/R7/K7 w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());

    // Pinned queen only moves
    board.loadFEN("rrk5/8/8/8/8/8/Q7/K7 w - - 0 1");
    EXPECT_EQ(PositionStatus::NORMAL, board.determineStatus());
}

// successful moves by pinned pieces
TEST(MoveGen, pinnedPieceMoves)
{
    ChessBoard board { };

    // bishop pinned, moving along pin
    board.loadFEN("7k/8/6b1/8/4B3/3K4/8/8 w - - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::F5);

    // rook pinned, moving along pin
    board.loadFEN("4k3/4r3/8/8/4R3/4K3/8/8 w - - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::E7);

    // queen pinned, moving along pin
    board.loadFEN("7k/8/6b1/8/4Q3/3K4/8/8 w - - 0 1");
    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::F5);

    // two bishops, one pinned, only one can move to specified square
    board.loadFEN("4k3/b7/8/8/3B1B2/4K3/8/8 w - - 0 1");
    PLAY_MOVE(board, BishopAndDest, SquareSet::all(), Square::E5);

    // two rooks, one pinned, only one can move to specified square
    board.loadFEN("4k3/b7/8/8/3R1R2/4K3/8/8 w - - 0 1");
    PLAY_MOVE(board, RookAndDest, SquareSet::all(), Square::E4);

    // two queens, one pinned, only one can move to specified square
    board.loadFEN("4k3/b7/8/8/3Q1Q2/4K3/8/8 w - - 0 1");
    PLAY_MOVE(board, QueenAndDest, SquareSet::all(), Square::E5);

    // two pawns, one pinned, only one can move to specified square
    board.loadFEN("4k3/b7/8/4r3/3P1P2/4K3/8/8 w - - 0 1");
    PLAY_MOVE(board, PawnAndDestCapture, SquareSet::all(), Square::E5);
}

// cases not covered by other tests
TEST(MoveGen, halfMoveClockMax)
{
    ChessBoard board { };
    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 254 1");

    // check that half move clock doesn't overflow, but rather, is clamped at 255
    EXPECT_EQ(board.getHalfMoveClock(), 254U);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F3);
    EXPECT_EQ(board.getHalfMoveClock(), 255U);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::F6);
    EXPECT_EQ(board.getHalfMoveClock(), 255U);
    PLAY_MOVE(board, KnightAndDest, SquareSet::all(), Square::C3);
    EXPECT_EQ(board.getHalfMoveClock(), 255U);

}

}
