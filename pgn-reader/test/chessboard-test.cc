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
        SquareSet(Square::B1, Square::G1, Square::B8, Square::G8));
    EXPECT_EQ(
        board.getBishops(),
        SquareSet(Square::C1, Square::F1, Square::C8, Square::F8));
    EXPECT_EQ(
        board.getRooks(),
        SquareSet(Square::A1, Square::H1, Square::A8, Square::H8));
    EXPECT_EQ(
        board.getQueens(),
        SquareSet(Square::D1, Square::D8));
    EXPECT_EQ(
        board.getBishopsAndQueens(),
        SquareSet(Square::C1, Square::D1, Square::F1, Square::C8, Square::D8, Square::F8));
    EXPECT_EQ(
        board.getRooksAndQueens(),
        SquareSet(Square::A1, Square::D1, Square::H1, Square::A8, Square::D8, Square::H8));

    board.loadFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
    EXPECT_EQ(board.getPiecesInTurn(), SquareSet::row(6) | SquareSet::row(7));
    EXPECT_EQ(board.getWhitePieces(), SquareSet::row(0) | SquareSet::row(1));
    EXPECT_EQ(board.getBlackPieces(), SquareSet::row(6) | SquareSet::row(7));
}

TEST(ChessBoard, moveTypes)
{
    auto expectRegular = [] (Move m) -> void
    {
        EXPECT_TRUE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectEnPassant = [] (Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_TRUE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectPromotion = [] (Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_TRUE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectCastling = [] (Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_TRUE(m.isCastlingMove());
        EXPECT_FALSE(m.isIllegal());
    };

    auto expectIllegal = [] (Move m) -> void
    {
        EXPECT_FALSE(m.isRegularMove());
        EXPECT_FALSE(m.isEnPassantMove());
        EXPECT_FALSE(m.isPromotionMove());
        EXPECT_FALSE(m.isCastlingMove());
        EXPECT_TRUE(m.isIllegal());
    };

    expectRegular(Move { Square::B2, Square::B3, MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
    expectRegular(Move { Square::B2, Square::A3, MoveTypeAndPromotion::REGULAR_PAWN_MOVE });
    expectRegular(Move { Square::B1, Square::A3, MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE });
    expectRegular(Move { Square::B1, Square::A2, MoveTypeAndPromotion::REGULAR_BISHOP_MOVE });
    expectRegular(Move { Square::B1, Square::A1, MoveTypeAndPromotion::REGULAR_ROOK_MOVE });
    expectRegular(Move { Square::B1, Square::A1, MoveTypeAndPromotion::REGULAR_QUEEN_MOVE });
    expectRegular(Move { Square::B1, Square::A1, MoveTypeAndPromotion::REGULAR_KING_MOVE });

    expectEnPassant(Move { Square::E5, Square::D6, MoveTypeAndPromotion::EN_PASSANT });

    expectPromotion(Move { Square::D7, Square::D8, MoveTypeAndPromotion::PROMO_KNIGHT });
    expectPromotion(Move { Square::D7, Square::D8, MoveTypeAndPromotion::PROMO_BISHOP });
    expectPromotion(Move { Square::D7, Square::D8, MoveTypeAndPromotion::PROMO_ROOK });
    expectPromotion(Move { Square::D7, Square::D8, MoveTypeAndPromotion::PROMO_QUEEN });

    expectCastling(Move { Square::B1, Square::A1, MoveTypeAndPromotion::CASTLING_LONG });
    expectCastling(Move { Square::B1, Square::C1, MoveTypeAndPromotion::CASTLING_SHORT });

    expectIllegal(Move::illegalNoMove());
    expectIllegal(Move::illegalAmbiguousMove());
}


TEST(MoveGenIteratorTraits, basics)
{
    // MoveList::iterator: no early completion; stores moves (in the list)
    {
        using IteratorType = MoveList::iterator;
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
}

}
