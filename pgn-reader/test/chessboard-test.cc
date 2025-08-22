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

#define STATIC_ASSERT_AND_TEST(x)               \
    do {                                        \
        static_assert(x);                       \
        EXPECT_TRUE(x);                         \
    } while (false)


TEST(ChessBoard, comparison)
{
    ChessBoard lhs;
    ChessBoard rhs;

    // equality
    lhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_EQ(lhs, rhs);

    // positions that differ by a single thing from the starting position

    // pawn
    rhs.loadFEN("rnbqkbnr/1ppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(lhs, rhs);

    // knight
    rhs.loadFEN("r1bqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(lhs, rhs);

    // bishop
    rhs.loadFEN("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(lhs, rhs);

    // queen
    rhs.loadFEN("rnb1kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(lhs, rhs);

    // piece color
    rhs.loadFEN("rNbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_NE(lhs, rhs);

    // castling rights
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qkq - 0 1");
    EXPECT_NE(lhs, rhs);
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Kkq - 0 1");
    EXPECT_NE(lhs, rhs);
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQq - 0 1");
    EXPECT_NE(lhs, rhs);
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQk - 0 1");
    EXPECT_NE(lhs, rhs);

    // turn and counters
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    EXPECT_NE(lhs, rhs);
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 1 1");
    EXPECT_NE(lhs, rhs);
    rhs.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 2");
    EXPECT_NE(lhs, rhs);

    // rooks
    lhs.loadFEN("2r5/8/3k4/8/8/3K4/8/1R6 w - - 0 1");
    rhs.loadFEN("1r6/8/3k4/8/8/3K4/8/1R6 w - - 0 1");
    EXPECT_NE(lhs, rhs);

    // EP square
    // these differ...
    lhs.loadFEN("8/8/3k4/6pP/8/3K4/8/8 w - g6 0 1");
    rhs.loadFEN("8/8/3k4/6pP/8/3K4/8/8 w - - 0 1");
    EXPECT_NE(lhs, rhs);
    EXPECT_NE(rhs, lhs);

    // ...but these don't, since the EP capture is not legal
    lhs.loadFEN("8/8/3k3P/6p1/8/3K4/8/8 w - g6 0 1");
    rhs.loadFEN("8/8/3k3P/6p1/8/3K4/8/8 w - - 0 1");
    EXPECT_EQ(lhs, rhs);
}

TEST(ChessBoard, bitmasks)
{
    ChessBoard board { };

    EXPECT_EQ(board.getPiecesInTurn(), SquareSet::row(0) | SquareSet::row(1));
    EXPECT_EQ(board.getWhitePieces(), SquareSet::row(0) | SquareSet::row(1));
    EXPECT_EQ(board.getBlackPieces(), SquareSet::row(6) | SquareSet::row(7));

    EXPECT_EQ(board.getPawns(), SquareSet::row(1) | SquareSet::row(6));
    EXPECT_EQ(
        board.getKnights(),
        SquareSet::square(Square::B1) | SquareSet::square(Square::G1) |
        SquareSet::square(Square::B8) | SquareSet::square(Square::G8));
    EXPECT_EQ(
        board.getBishops(),
        SquareSet::square(Square::C1) | SquareSet::square(Square::F1) |
        SquareSet::square(Square::C8) | SquareSet::square(Square::F8));
    EXPECT_EQ(
        board.getRooks(),
        SquareSet::square(Square::A1) | SquareSet::square(Square::H1) |
        SquareSet::square(Square::A8) | SquareSet::square(Square::H8));
    EXPECT_EQ(
        board.getQueens(),
        SquareSet::square(Square::D1) |
        SquareSet::square(Square::D8));
    EXPECT_EQ(
        board.getBishopsAndQueens(),
        SquareSet::square(Square::C1) | SquareSet::square(Square::D1) | SquareSet::square(Square::F1) |
        SquareSet::square(Square::C8) | SquareSet::square(Square::D8) | SquareSet::square(Square::F8));
    EXPECT_EQ(
        board.getRooksAndQueens(),
        SquareSet::square(Square::A1) | SquareSet::square(Square::D1) | SquareSet::square(Square::H1) |
        SquareSet::square(Square::A8) | SquareSet::square(Square::D8) | SquareSet::square(Square::H8));

    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    EXPECT_EQ(board.getPiecesInTurn(), SquareSet::row(6) | SquareSet::row(7));
    EXPECT_EQ(board.getWhitePieces(), SquareSet::row(0) | SquareSet::row(1));
    EXPECT_EQ(board.getBlackPieces(), SquareSet::row(6) | SquareSet::row(7));
}

TEST(ChessBoard, moveTypes)
{
    auto expectRegular = [] (ChessBoard::Move m) -> void
    {
        EXPECT_TRUE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectEnPassant = [] (ChessBoard::Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_TRUE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectPromotion = [] (ChessBoard::Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_TRUE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectCastling = [] (ChessBoard::Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_TRUE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectIllegal = [] (ChessBoard::Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_TRUE(m.isIllegal());
    };

    expectRegular(ChessBoard::Move { Square::B2, Square::B3, ChessBoard::MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
    expectRegular(ChessBoard::Move { Square::B2, Square::A3, ChessBoard::MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE });
    expectRegular(ChessBoard::Move { Square::B1, Square::A3, ChessBoard::MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE });
    expectRegular(ChessBoard::Move { Square::B1, Square::A2, ChessBoard::MoveTypeAndPromotion::REGULAR_BISHOP_MOVE });
    expectRegular(ChessBoard::Move { Square::B1, Square::A1, ChessBoard::MoveTypeAndPromotion::REGULAR_ROOK_MOVE });
    expectRegular(ChessBoard::Move { Square::B1, Square::A1, ChessBoard::MoveTypeAndPromotion::REGULAR_QUEEN_MOVE });
    expectRegular(ChessBoard::Move { Square::B1, Square::A1, ChessBoard::MoveTypeAndPromotion::REGULAR_KING_MOVE });

    expectEnPassant(ChessBoard::Move { Square::E5, Square::D6, ChessBoard::MoveTypeAndPromotion::EN_PASSANT });

    expectPromotion(ChessBoard::Move { Square::D7, Square::D8, ChessBoard::MoveTypeAndPromotion::PROMO_KNIGHT });
    expectPromotion(ChessBoard::Move { Square::D7, Square::D8, ChessBoard::MoveTypeAndPromotion::PROMO_BISHOP });
    expectPromotion(ChessBoard::Move { Square::D7, Square::D8, ChessBoard::MoveTypeAndPromotion::PROMO_ROOK });
    expectPromotion(ChessBoard::Move { Square::D7, Square::D8, ChessBoard::MoveTypeAndPromotion::PROMO_QUEEN });

    expectCastling(ChessBoard::Move { Square::B1, Square::A1, ChessBoard::MoveTypeAndPromotion::CASTLING_LONG });
    expectCastling(ChessBoard::Move { Square::B1, Square::C1, ChessBoard::MoveTypeAndPromotion::CASTLING_SHORT });

    expectIllegal(ChessBoard::Move::illegalNoMove());
    expectIllegal(ChessBoard::Move::illegalAmbiguousMove());
}


TEST(MoveGenIteratorTraits, basics)
{
    // MoveList::iterator: no early completion; stores moves (in the list)
    {
        using IteratorType = ChessBoard::MoveList::iterator;
        STATIC_ASSERT_AND_TEST(MoveGenIteratorTraits<IteratorType>::storesMoves());
    }

    // LegalMoveCounterIterator: no early completion; doesn't store moves (only counts them)
    {
        using IteratorType = LegalMoveCounterIterator;
        STATIC_ASSERT_AND_TEST(!MoveGenIteratorTraits<IteratorType>::storesMoves());
    }

    // LegalMoveDetectorIterator: completes as soon as the iterator has been
    // incremented; doesn't store moves (only counts the first one)
    {
        using IteratorType = LegalMoveDetectorIterator;
        STATIC_ASSERT_AND_TEST(!MoveGenIteratorTraits<IteratorType>::storesMoves());
    }

    // SingleMoveIterator: no early completion for ambiguous move detection (unlikely);
    // stores the single move
    {
        using IteratorType = SingleMoveIterator;
        STATIC_ASSERT_AND_TEST(MoveGenIteratorTraits<IteratorType>::storesMoves());
    }
}

}
