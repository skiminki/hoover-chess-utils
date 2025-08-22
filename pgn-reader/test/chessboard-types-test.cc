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

#include "chessboard-types.h"

#include "gtest/gtest.h"

namespace hoover_chess_utils::pgn_reader::unit_test
{

#define STATIC_ASSERT_AND_TEST(x)               \
    do {                                        \
        static_assert(x);                       \
        EXPECT_TRUE(x);                         \
    } while (false)


TEST(Square, values)
{
    // just make sure we didn't make any stupid typos when defining the enum values...

    STATIC_ASSERT_AND_TEST(Square::A1 < Square::B1);
    STATIC_ASSERT_AND_TEST(Square::B1 < Square::C1);
    STATIC_ASSERT_AND_TEST(Square::C1 < Square::D1);
    STATIC_ASSERT_AND_TEST(Square::D1 < Square::E1);
    STATIC_ASSERT_AND_TEST(Square::E1 < Square::F1);
    STATIC_ASSERT_AND_TEST(Square::F1 < Square::G1);
    STATIC_ASSERT_AND_TEST(Square::G1 < Square::H1);
    STATIC_ASSERT_AND_TEST(Square::H1 < Square::A2);

    STATIC_ASSERT_AND_TEST(Square::A2 < Square::B2);
    STATIC_ASSERT_AND_TEST(Square::B2 < Square::C2);
    STATIC_ASSERT_AND_TEST(Square::C2 < Square::D2);
    STATIC_ASSERT_AND_TEST(Square::D2 < Square::E2);
    STATIC_ASSERT_AND_TEST(Square::E2 < Square::F2);
    STATIC_ASSERT_AND_TEST(Square::F2 < Square::G2);
    STATIC_ASSERT_AND_TEST(Square::G2 < Square::H2);
    STATIC_ASSERT_AND_TEST(Square::H2 < Square::A3);

    STATIC_ASSERT_AND_TEST(Square::A3 < Square::B3);
    STATIC_ASSERT_AND_TEST(Square::B3 < Square::C3);
    STATIC_ASSERT_AND_TEST(Square::C3 < Square::D3);
    STATIC_ASSERT_AND_TEST(Square::D3 < Square::E3);
    STATIC_ASSERT_AND_TEST(Square::E3 < Square::F3);
    STATIC_ASSERT_AND_TEST(Square::F3 < Square::G3);
    STATIC_ASSERT_AND_TEST(Square::G3 < Square::H3);
    STATIC_ASSERT_AND_TEST(Square::H3 < Square::A4);

    STATIC_ASSERT_AND_TEST(Square::A4 < Square::B4);
    STATIC_ASSERT_AND_TEST(Square::B4 < Square::C4);
    STATIC_ASSERT_AND_TEST(Square::C4 < Square::D4);
    STATIC_ASSERT_AND_TEST(Square::D4 < Square::E4);
    STATIC_ASSERT_AND_TEST(Square::E4 < Square::F4);
    STATIC_ASSERT_AND_TEST(Square::F4 < Square::G4);
    STATIC_ASSERT_AND_TEST(Square::G4 < Square::H4);
    STATIC_ASSERT_AND_TEST(Square::H4 < Square::A5);

    STATIC_ASSERT_AND_TEST(Square::A5 < Square::B5);
    STATIC_ASSERT_AND_TEST(Square::B5 < Square::C5);
    STATIC_ASSERT_AND_TEST(Square::C5 < Square::D5);
    STATIC_ASSERT_AND_TEST(Square::D5 < Square::E5);
    STATIC_ASSERT_AND_TEST(Square::E5 < Square::F5);
    STATIC_ASSERT_AND_TEST(Square::F5 < Square::G5);
    STATIC_ASSERT_AND_TEST(Square::G5 < Square::H5);
    STATIC_ASSERT_AND_TEST(Square::H5 < Square::A6);

    STATIC_ASSERT_AND_TEST(Square::A6 < Square::B6);
    STATIC_ASSERT_AND_TEST(Square::B6 < Square::C6);
    STATIC_ASSERT_AND_TEST(Square::C6 < Square::D6);
    STATIC_ASSERT_AND_TEST(Square::D6 < Square::E6);
    STATIC_ASSERT_AND_TEST(Square::E6 < Square::F6);
    STATIC_ASSERT_AND_TEST(Square::F6 < Square::G6);
    STATIC_ASSERT_AND_TEST(Square::G6 < Square::H6);
    STATIC_ASSERT_AND_TEST(Square::H6 < Square::A7);

    STATIC_ASSERT_AND_TEST(Square::A7 < Square::B7);
    STATIC_ASSERT_AND_TEST(Square::B7 < Square::C7);
    STATIC_ASSERT_AND_TEST(Square::C7 < Square::D7);
    STATIC_ASSERT_AND_TEST(Square::D7 < Square::E7);
    STATIC_ASSERT_AND_TEST(Square::E7 < Square::F7);
    STATIC_ASSERT_AND_TEST(Square::F7 < Square::G7);
    STATIC_ASSERT_AND_TEST(Square::G7 < Square::H7);
    STATIC_ASSERT_AND_TEST(Square::H7 < Square::A8);

    STATIC_ASSERT_AND_TEST(Square::A8 < Square::B8);
    STATIC_ASSERT_AND_TEST(Square::B8 < Square::C8);
    STATIC_ASSERT_AND_TEST(Square::C8 < Square::D8);
    STATIC_ASSERT_AND_TEST(Square::D8 < Square::E8);
    STATIC_ASSERT_AND_TEST(Square::E8 < Square::F8);
    STATIC_ASSERT_AND_TEST(Square::F8 < Square::G8);
    STATIC_ASSERT_AND_TEST(Square::G8 < Square::H8);
    STATIC_ASSERT_AND_TEST(Square::H8 < Square::NONE);
}

TEST(Square, isValidValue)
{
    STATIC_ASSERT_AND_TEST(isValidValue(Square::A1));
    STATIC_ASSERT_AND_TEST(isValidValue(Square::H8));
    STATIC_ASSERT_AND_TEST(isValidValue(Square::NONE));
    STATIC_ASSERT_AND_TEST(!isValidValue(addToSquareNoOverflowCheck(Square::NONE, 1)));
}

TEST(Square, isValidSquare)
{
    STATIC_ASSERT_AND_TEST(isValidSquare(Square::A1));
    STATIC_ASSERT_AND_TEST(isValidSquare(Square::H8));
    STATIC_ASSERT_AND_TEST(!isValidSquare(Square::NONE));
}

TEST(Square, makeSquare)
{
    STATIC_ASSERT_AND_TEST(makeSquare(0U, 0U) == Square::A1);
    STATIC_ASSERT_AND_TEST(makeSquare(1U, 0U) == Square::B1);
    STATIC_ASSERT_AND_TEST(makeSquare(0U, 1U) == Square::A2);
    STATIC_ASSERT_AND_TEST(makeSquare(7U, 7U) == Square::H8);
}

TEST(Square, columnOf)
{
    for (std::uint8_t r { }; r <= 7U; ++r)
        for (std::uint8_t c { }; c <= 7U; ++c)
        {
            Square sq { makeSquare(c, r) };
            EXPECT_EQ(columnOf(sq), c);
        }
}

TEST(Square, rowOf)
{
    for (std::uint8_t r { }; r <= 7U; ++r)
        for (std::uint8_t c { }; c <= 7U; ++c)
        {
            Square sq { makeSquare(c, r) };
            EXPECT_EQ(rowOf(sq), r);
        }
}

TEST(Square, getIndexOfSquare)
{
    STATIC_ASSERT_AND_TEST(getIndexOfSquare(Square::A1) == 0U);
    STATIC_ASSERT_AND_TEST(getIndexOfSquare(Square::B1) == 1U);
    STATIC_ASSERT_AND_TEST(getIndexOfSquare(Square::H8) == 63U);
}

TEST(Square, getSquareForIndex)
{
    STATIC_ASSERT_AND_TEST(getSquareForIndex(0U) == Square::A1);
    STATIC_ASSERT_AND_TEST(getSquareForIndex(1U) == Square::B1);
    STATIC_ASSERT_AND_TEST(getSquareForIndex(63U) == Square::H8);
}

TEST(Color, isValidValue)
{
    STATIC_ASSERT_AND_TEST(isValidValue(Color::WHITE));
    STATIC_ASSERT_AND_TEST(isValidValue(Color::BLACK));
    STATIC_ASSERT_AND_TEST(!isValidValue(Color { 1U }));
}

TEST(Color, oppositeColor)
{
    STATIC_ASSERT_AND_TEST(oppositeColor(Color::WHITE) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(oppositeColor(Color::BLACK) == Color::WHITE);

    STATIC_ASSERT_AND_TEST(oppositeColor(oppositeColor(Color::WHITE)) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(oppositeColor(oppositeColor(Color::BLACK)) == Color::BLACK);
}

TEST(Color, colorOf)
{
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_PAWN) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_KNIGHT) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_BISHOP) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_ROOK) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_QUEEN) == Color::WHITE);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::WHITE_KING) == Color::WHITE);

    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_PAWN) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_KNIGHT) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_BISHOP) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_ROOK) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_QUEEN) == Color::BLACK);
    STATIC_ASSERT_AND_TEST(colorOf(PieceAndColor::BLACK_KING) == Color::BLACK);
}

TEST(Piece, isValidValue)
{
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::NONE));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::PAWN));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::KNIGHT));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::BISHOP));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::ROOK));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::QUEEN));
    STATIC_ASSERT_AND_TEST(isValidValue(Piece::KING));
    STATIC_ASSERT_AND_TEST(!isValidValue(Piece { 7 }));
}

TEST(Piece, pieceOf)
{
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_PAWN) == Piece::PAWN);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_KNIGHT) == Piece::KNIGHT);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_BISHOP) == Piece::BISHOP);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_ROOK) == Piece::ROOK);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_QUEEN) == Piece::QUEEN);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::WHITE_KING) == Piece::KING);

    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_PAWN) == Piece::PAWN);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_KNIGHT) == Piece::KNIGHT);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_BISHOP) == Piece::BISHOP);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_ROOK) == Piece::ROOK);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_QUEEN) == Piece::QUEEN);
    STATIC_ASSERT_AND_TEST(pieceOf(PieceAndColor::BLACK_KING) == Piece::KING);
}

TEST(PieceAndColor, makePieceAndColor)
{
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::PAWN, Color::WHITE) == PieceAndColor::WHITE_PAWN);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::KNIGHT, Color::WHITE) == PieceAndColor::WHITE_KNIGHT);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::BISHOP, Color::WHITE) == PieceAndColor::WHITE_BISHOP);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::ROOK, Color::WHITE) == PieceAndColor::WHITE_ROOK);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::QUEEN, Color::WHITE) == PieceAndColor::WHITE_QUEEN);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::KING, Color::WHITE) == PieceAndColor::WHITE_KING);

    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::PAWN, Color::BLACK) == PieceAndColor::BLACK_PAWN);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::KNIGHT, Color::BLACK) == PieceAndColor::BLACK_KNIGHT);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::BISHOP, Color::BLACK) == PieceAndColor::BLACK_BISHOP);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::ROOK, Color::BLACK) == PieceAndColor::BLACK_ROOK);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::QUEEN, Color::BLACK) == PieceAndColor::BLACK_QUEEN);
    STATIC_ASSERT_AND_TEST(makePieceAndColor(Piece::KING, Color::BLACK) == PieceAndColor::BLACK_KING);
}

TEST(PieceAndColor, isValidValue)
{
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::NONE));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_NONE));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_PAWN));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_KNIGHT));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_BISHOP));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_ROOK));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_QUEEN));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::WHITE_KING));
    STATIC_ASSERT_AND_TEST(!isValidValue(PieceAndColor { 7U }));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_NONE));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_PAWN));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_KNIGHT));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_BISHOP));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_ROOK));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_QUEEN));
    STATIC_ASSERT_AND_TEST(isValidValue(PieceAndColor::BLACK_KING));
    STATIC_ASSERT_AND_TEST(!isValidValue(PieceAndColor { 15U }));
}

}
