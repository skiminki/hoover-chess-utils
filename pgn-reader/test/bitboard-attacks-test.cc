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

#include <gtest/gtest.h>

#include "pgnreader-string-utils.h"
#include "src/bitboard-attacks.h"
#include "src/slider-attacks.h"

#include <cmath>
#include <cstdio>
#include <format>
#include <iostream>
#include <string_view>

namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace {

SquareSet sqSetIfOnBoard(std::int8_t col, std::int8_t row)
{
    if (col >= 0 && col <= 7 && row >= 0 && row <= 7)
        return SquareSet::square(col, row);

    return SquareSet::none();
}

SquareSet shootRay(Square sq, SquareSet occupancy, int dx, int dy)
{
    int x = columnOf(sq);
    int y = rowOf(sq);
    SquareSet ret { };

    while (true)
    {
        x += dx;
        y += dy;

        if (x < 0 || y < 0 || x >= 8 || y >= 8)
            break;

        SquareSet tmp { SquareSet::square(x, y) };
        ret |= tmp;

        if ((tmp & occupancy) != SquareSet::none())
            break;
    }

    return ret;
}

}


TEST(Attacks, getPawnAttackMask_white)
{
    for (std::uint8_t row = 0U; row <= 7U; ++row)
        for (std::uint8_t col = 0U; col <= 7U; ++col)
        {
            // expectation
            SquareSet expect { };
            const Square pawn { makeSquare(col, row) };

            if (row <= 6U)
            {
                if (col >= 1U)
                    expect |= SquareSet::square(col - 1U, row + 1U);
                if (col <= 6U)
                    expect |= SquareSet::square(col + 1U, row + 1U);
            }

            EXPECT_EQ(expect, Attacks::getPawnAttackMask(pawn, Color::WHITE))
                << std::format("Pawn={}", StringUtils::squareToString(pawn, "??"));
        }
}

TEST(Attacks, getPawnAttackMask_black)
{
    for (std::uint8_t row = 0U; row <= 7U; ++row)
        for (std::uint8_t col = 0U; col <= 7; ++col)
        {
            // expectation
            SquareSet expect { };
            const Square pawn { makeSquare(col, row) };

            if (row >= 1U)
            {
                if (col >= 1U)
                    expect |= SquareSet::square(col - 1U, row - 1U);
                if (col <= 6U)
                    expect |= SquareSet::square(col + 1U, row - 1U);
            }

            EXPECT_EQ(expect, Attacks::getPawnAttackMask(pawn, Color::BLACK))
                << std::format("Pawn={}", StringUtils::squareToString(pawn, "??"));
        }
}

TEST(Attacks, getPawnAttackerMask_white)
{
    // white pawns can attack anything from 3rd rank to 8th rank
    for (std::uint8_t row = 2U; row <= 7U; ++row)
        for (std::uint8_t col = 0U; col <= 7U; ++col)
        {
            // expectation
            SquareSet expect { };
            const Square pawn { makeSquare(col, row) };

            if (col >= 1U)
                expect |= SquareSet::square(col - 1U, row - 1U);
            if (col <= 6U)
                expect |= SquareSet::square(col + 1U, row - 1U);

            EXPECT_EQ(expect, Attacks::getPawnAttackerMask(pawn, Color::WHITE))
                << std::format("Pawn={}", StringUtils::squareToString(pawn, "??"));
        }
}

TEST(Attacks, getPawnAttackerMask_black)
{
    // white pawns can attack anything from 1st rank to 6th rank
    for (std::uint8_t row = 0U; row <= 5U; ++row)
        for (std::uint8_t col = 0U; col <= 7U; ++col)
        {
            // expectation
            SquareSet expect { };
            const Square pawn { makeSquare(col, row) };

            if (col >= 1U)
                expect |= SquareSet::square(col - 1U, row + 1U);
            if (col <= 6U)
                expect |= SquareSet::square(col + 1U, row + 1U);

            EXPECT_EQ(expect, Attacks::getPawnAttackerMask(pawn, Color::BLACK))
                << std::format("Pawn={}", StringUtils::squareToString(pawn, "??"));
        }
}

TEST(Attacks, getPawnAttackersMask_whitePawn)
{
    // try all combinations of two capturable things
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    SquareSet expectLeftCapturingPawns { };
                    SquareSet expectRightCapturingPawns { };
                    Square piece1 { makeSquare(col1, row1) };
                    Square piece2 { makeSquare(col2, row2) };
                    const SquareSet capturablePieces { SquareSet::square(piece1) | SquareSet::square(piece2) };

                    if (row1 >= 1U)
                    {
                        if (col1 <= 6U)
                            expectLeftCapturingPawns |= SquareSet::square(col1 + 1U, row1 - 1U);
                        if (col1 >= 1U)
                            expectRightCapturingPawns |= SquareSet::square(col1 - 1U, row1 - 1U);
                    }

                    if (row2 >= 1U)
                    {
                        if (col2 <= 6U)
                            expectLeftCapturingPawns |= SquareSet::square(col2 + 1U, row2 - 1U);
                        if (col2 >= 1U)
                            expectRightCapturingPawns |= SquareSet::square(col2 - 1U, row2 - 1U);
                    }

                    EXPECT_EQ(
                        expectLeftCapturingPawns,
                        (Attacks::getPawnAttackersMask<Color::WHITE, false>(capturablePieces)))
                        << std::format("Piece1={} Piece2={}",
                                       StringUtils::squareToString(piece1, "??"),
                                       StringUtils::squareToString(piece2, "??"));
                    EXPECT_EQ(
                        expectRightCapturingPawns,
                        (Attacks::getPawnAttackersMask<Color::WHITE, true>(capturablePieces)))
                        << std::format("Piece1={} Piece2={}",
                                       StringUtils::squareToString(piece1, "??"),
                                       StringUtils::squareToString(piece2, "??"));
                }
}

TEST(Attacks, getPawnAttackersMask_blackPawn)
{
    // try all combinations of two capturable things
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    SquareSet expectLeftCapturingPawns { };
                    SquareSet expectRightCapturingPawns { };
                    Square piece1 { makeSquare(col1, row1) };
                    Square piece2 { makeSquare(col2, row2) };
                    const SquareSet capturablePieces { SquareSet::square(piece1) | SquareSet::square(piece2) };

                    if (row1 <= 6U)
                    {
                        if (col1 <= 6U)
                            expectLeftCapturingPawns |= SquareSet::square(col1 + 1U, row1 + 1U);
                        if (col1 >= 1U)
                            expectRightCapturingPawns |= SquareSet::square(col1 - 1U, row1 + 1U);
                    }

                    if (row2 <= 6U)
                    {
                        if (col2 <= 6U)
                            expectLeftCapturingPawns |= SquareSet::square(col2 + 1U, row2 + 1U);
                        if (col2 >= 1U)
                            expectRightCapturingPawns |= SquareSet::square(col2 - 1U, row2 + 1U);
                    }

                    EXPECT_EQ(
                        expectLeftCapturingPawns,
                        (Attacks::getPawnAttackersMask<Color::BLACK, false>(capturablePieces)))
                        << std::format("Piece1={} Piece2={}",
                                       StringUtils::squareToString(piece1, "??"),
                                       StringUtils::squareToString(piece2, "??"));
                    EXPECT_EQ(
                        expectRightCapturingPawns,
                        (Attacks::getPawnAttackersMask<Color::BLACK, true>(capturablePieces)))
                        << std::format("Piece1={} Piece2={}",
                                       StringUtils::squareToString(piece1, "??"),
                                       StringUtils::squareToString(piece2, "??"));
                }
}

TEST(Attacks, getKnightAttackMask)
{
    for (std::uint8_t row = 0U; row <= 7U; ++row)
        for (std::uint8_t col = 0U; col <= 7U; ++col)
        {
            SquareSet expect { };
            const Square knight { makeSquare(col, row) };

            expect |= sqSetIfOnBoard(col - 2, row - 1);
            expect |= sqSetIfOnBoard(col + 2, row - 1);
            expect |= sqSetIfOnBoard(col - 2, row + 1);
            expect |= sqSetIfOnBoard(col + 2, row + 1);
            expect |= sqSetIfOnBoard(col - 1, row - 2);
            expect |= sqSetIfOnBoard(col + 1, row - 2);
            expect |= sqSetIfOnBoard(col - 1, row + 2);
            expect |= sqSetIfOnBoard(col + 1, row + 2);

            EXPECT_EQ(expect, Attacks::getKnightAttackMask(knight))
                << std::format("Knight={}", StringUtils::squareToString(knight, "??"));
        }
}

TEST(Attacks, getKingAttackMask)
{
    for (std::uint8_t row = 0U; row <= 7U; ++row)
        for (std::uint8_t col = 0U; col <= 7U; ++col)
        {
            SquareSet expect { };
            const Square king { makeSquare(col, row) };

            expect |= sqSetIfOnBoard(col - 1, row - 1);
            expect |= sqSetIfOnBoard(col    , row - 1);
            expect |= sqSetIfOnBoard(col + 1, row - 1);
            expect |= sqSetIfOnBoard(col - 1, row    );
            expect |= sqSetIfOnBoard(col + 1, row    );
            expect |= sqSetIfOnBoard(col - 1, row + 1);
            expect |= sqSetIfOnBoard(col    , row + 1);
            expect |= sqSetIfOnBoard(col + 1, row + 1);

            EXPECT_EQ(expect, Attacks::getKingAttackMask(king))
                << std::format("King={}", StringUtils::squareToString(king, "??"));
        }
}

TEST(Attacks, getBishopAttackMask)
{
    // try all combinations of bishop + capturable piece
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    Square bishop { makeSquare(row1, col1) };
                    Square capturable { makeSquare(row2, col2) };

                    SquareSet occupancyMask { SquareSet::square(capturable) | SquareSet::square(bishop) };

                    SquareSet expect { };

                    expect |= shootRay(bishop, occupancyMask, -1, -1);
                    expect |= shootRay(bishop, occupancyMask, -1, +1);
                    expect |= shootRay(bishop, occupancyMask, +1, -1);
                    expect |= shootRay(bishop, occupancyMask, +1, +1);

                    EXPECT_EQ(expect, Attacks::getBishopAttackMask(bishop, occupancyMask))
                        << std::format("Bishop={} Capturable={}",
                                       StringUtils::squareToString(bishop, "??"),
                                       StringUtils::squareToString(capturable, "??"));
                }
}

TEST(Attacks, getBishopAttackMask_GenericImpl)
{
    // try all combinations of bishop + capturable piece
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    Square bishop { makeSquare(row1, col1) };
                    Square capturable { makeSquare(row2, col2) };

                    SquareSet occupancyMask { SquareSet::square(capturable) | SquareSet::square(bishop) };

                    SquareSet expect { };

                    expect |= shootRay(bishop, occupancyMask, -1, -1);
                    expect |= shootRay(bishop, occupancyMask, -1, +1);
                    expect |= shootRay(bishop, occupancyMask, +1, -1);
                    expect |= shootRay(bishop, occupancyMask, +1, +1);

                    EXPECT_EQ(expect, SliderAttacksGeneric::getBishopAttackMask(bishop, occupancyMask))
                        << std::format("Bishop={} Capturable={}",
                                       StringUtils::squareToString(bishop, "??"),
                                       StringUtils::squareToString(capturable, "??"));
                }
}

TEST(Attacks, getRookAttackMask)
{
    // try all combinations of bishop + capturable piece
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    Square rook { makeSquare(row1, col1) };
                    Square capturable { makeSquare(row2, col2) };

                    SquareSet occupancyMask { SquareSet::square(capturable) | SquareSet::square(rook) };

                    SquareSet expect { };

                    expect |= shootRay(rook, occupancyMask, -1, 0);
                    expect |= shootRay(rook, occupancyMask, +1, 0);
                    expect |= shootRay(rook, occupancyMask,  0, -1);
                    expect |= shootRay(rook, occupancyMask,  0, +1);

                    EXPECT_EQ(expect, Attacks::getRookAttackMask(rook, occupancyMask))
                        << std::format("Rook={} Capturable={}",
                                       StringUtils::squareToString(rook, "??"),
                                       StringUtils::squareToString(capturable, "??"));
                }
}

TEST(Attacks, getRookAttackMask_GenericImpl)
{
    // try all combinations of bishop + capturable piece
    for (std::uint8_t row1 = 0U; row1 <= 7U; ++row1)
        for (std::uint8_t col1 = 0U; col1 <= 7U; ++col1)
            for (std::uint8_t row2 = 0U; row2 <= 7U; ++row2)
                for (std::uint8_t col2 = 0U; col2 <= 7U; ++col2)
                {
                    Square rook { makeSquare(row1, col1) };
                    Square capturable { makeSquare(row2, col2) };

                    SquareSet occupancyMask { SquareSet::square(capturable) | SquareSet::square(rook) };

                    SquareSet expect { };

                    expect |= shootRay(rook, occupancyMask, -1, 0);
                    expect |= shootRay(rook, occupancyMask, +1, 0);
                    expect |= shootRay(rook, occupancyMask,  0, -1);
                    expect |= shootRay(rook, occupancyMask,  0, +1);

                    EXPECT_EQ(expect, SliderAttacksGeneric::getRookAttackMask(rook, occupancyMask))
                        << std::format("Rook={} Capturable={}",
                                       StringUtils::squareToString(rook, "??"),
                                       StringUtils::squareToString(capturable, "??"));
                }
}

}
