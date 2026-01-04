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

#include "chessboard-types-squareset.h"

#include "gtest/gtest.h"

#include <atomic>
#include <algorithm>
#include <bit>
#include <cinttypes>

namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(SquareSet, constructor)
{
    // first, just ensure that the primary constructors can be evaluated compile-time
    static_assert(
        static_cast<std::uint64_t>(SquareSet { }) == UINT64_C(0x00'00'00'00'00'00'00'00));
    static_assert(
        static_cast<std::uint64_t>(SquareSet { Square::A1 }) == UINT64_C(0x00'00'00'00'00'00'00'01));
    static_assert(
        static_cast<std::uint64_t>(SquareSet { Square::A1, Square::A2 }) == UINT64_C(0x00'00'00'00'00'00'01'01));
    static_assert(
        static_cast<std::uint64_t>(SquareSet { Square::A1, Square::A2, Square::A3 }) == UINT64_C(0x00'00'00'00'00'01'01'01));
    static_assert(
        static_cast<std::uint64_t>(SquareSet { Square::A1, Square::A2, Square::A3, Square::H8 }) == UINT64_C(0x80'00'00'00'00'01'01'01));

    // make sure we can pass all sorts of references to the SquareSet constructor
    auto makeSquareSet1 = [] (Square sq1, Square sq2) noexcept
    {
        return SquareSet { sq1, sq2 };
    };

    auto makeSquareSet2 = [] (const Square sq1, const Square sq2) noexcept
    {
        return SquareSet { sq1, sq2 };
    };

    auto makeSquareSet3 = [] (const Square &sq1, const Square &sq2) noexcept
    {
        return SquareSet { sq1, sq2 };
    };

    auto makeSquareSet4 = [] (Square &&sq1, Square &&sq2) noexcept
    {
        return SquareSet { sq1, sq2 };
    };

    auto makeSquareSet5 = [] (Square &sq1, Square &sq2) noexcept
    {
        return SquareSet { sq1, sq2 };
    };

    EXPECT_EQ(static_cast<std::uint64_t>(makeSquareSet1(Square::A1, Square::B1)), UINT64_C(0x00'00'00'00'00'00'00'03));
    EXPECT_EQ(static_cast<std::uint64_t>(makeSquareSet2(Square::A1, Square::B1)), UINT64_C(0x00'00'00'00'00'00'00'03));
    EXPECT_EQ(static_cast<std::uint64_t>(makeSquareSet3(Square::A1, Square::B1)), UINT64_C(0x00'00'00'00'00'00'00'03));
    EXPECT_EQ(static_cast<std::uint64_t>(makeSquareSet4(Square::A1, Square::B1)), UINT64_C(0x00'00'00'00'00'00'00'03));

    {
        Square sq1 { Square::A1 };
        Square sq2 { Square::B1 };
        EXPECT_EQ(static_cast<std::uint64_t>(makeSquareSet5(sq1, sq2)), UINT64_C(0x00'00'00'00'00'00'00'03));
    }
}

TEST(SquareSet, popcount)
{
    // constexpr
    static_assert(SquareSet { std::uint64_t { 0xFF'FF'FF'F1'FF'FF'FF'FF } }.popcount() == 61U);

    // dynamic
    EXPECT_EQ(SquareSet { UINT64_C(0) }.popcount(), 0U);
    EXPECT_EQ(SquareSet { UINT64_C(0x01'00'00'00'00'00'00'00) }.popcount(), 1U);
    EXPECT_EQ(SquareSet { UINT64_C(0x80'00'00'00'00'00'00'00) }.popcount(), 1U);
    EXPECT_EQ(SquareSet { UINT64_C(0x00'00'00'00'00'00'04'00) }.popcount(), 1U);
    EXPECT_EQ(SquareSet { UINT64_C(0x00'00'0F'80'00'00'04'00) }.popcount(), 6U);
    EXPECT_EQ(SquareSet { UINT64_C(0xFF'FF'FF'F1'FF'FF'FF'FF) }.popcount(), 61U);
    EXPECT_EQ(SquareSet { UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF) }.popcount(), 64U);
}

TEST(SquareSet, firstSquare)
{
    // constexpr
    static_assert(SquareSet { UINT64_C(0x00'00'FF'00'00'00'00'00) }.firstSquare() == Square::A6);
    static_assert(SquareSet { UINT64_C(0x00'00'00'00'00'00'00'00) }.firstSquare() == Square::NONE);

    // dynamic
    EXPECT_EQ(SquareSet { UINT64_C(0x00'00'00'00'00'00'00'00) }.firstSquare(), Square::NONE);
    EXPECT_EQ(SquareSet { UINT64_C(0x01'10'20'03'04'50'60'71) }.firstSquare(), Square::A1);
    EXPECT_EQ(SquareSet { UINT64_C(0x01'10'20'03'04'50'60'00) }.firstSquare(), Square::F2);
    EXPECT_EQ(SquareSet { UINT64_C(0x80'00'00'00'00'00'00'00) }.firstSquare(), Square::H8);

}

TEST(SquareSet, square)
{
    // constexpr
    static_assert(SquareSet::square(0U, 0U) == SquareSet { UINT64_C(0x00'00'00'00'00'00'00'01) });
    static_assert(SquareSet::square(1U, 0U) == SquareSet { UINT64_C(0x00'00'00'00'00'00'00'02) });
    static_assert(SquareSet::square(1U, 1U) == SquareSet { UINT64_C(0x00'00'00'00'00'00'02'00) });
    static_assert(SquareSet::square(7U, 7U) == SquareSet { UINT64_C(0x80'00'00'00'00'00'00'00) });

    // dynamic
    EXPECT_EQ(SquareSet::square(7U, 7U), SquareSet { UINT64_C(0x80'00'00'00'00'00'00'00) });
}

TEST(SquareSet, SQUARESET_ENUMERATE)
{
    auto enumerateTest = [] (std::uint64_t mask) static -> void
    {
        const SquareSet sqSetToEnumerate { mask };
        SquareSet enumeratedSquares { };
        std::uint32_t numInvocations { };

        SQUARESET_ENUMERATE(
            sq, sqSetToEnumerate,
            enumeratedSquares |= SquareSet::square(sq);
            ++numInvocations);

        EXPECT_EQ(enumeratedSquares, sqSetToEnumerate);
        EXPECT_EQ(numInvocations, std::popcount(mask));
    };

    enumerateTest(0U);
    enumerateTest(UINT64_C(1));
    enumerateTest(UINT64_C(0x12'34'56'78'9A'BC'DE'F0));
    enumerateTest(UINT64_C(0xFF'00'FF'00'FF'00'FF'00));
    enumerateTest(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF));
}

TEST(SquareSet, SQUARESET_ENUMERATE_WITH_BREAK)
{
    auto enumerateTest = [] (std::uint64_t mask, std::uint8_t breakAfter) static -> void
    {
        const SquareSet sqSetToEnumerate { mask };
        SquareSet enumeratedSquares { };
        std::uint32_t numInvocations { };

        SQUARESET_ENUMERATE(
            sq, sqSetToEnumerate,
            if (numInvocations == breakAfter)
                break;
            enumeratedSquares |= SquareSet::square(sq);
            ++numInvocations);

        EXPECT_EQ(enumeratedSquares & sqSetToEnumerate, enumeratedSquares);
        EXPECT_EQ(numInvocations, std::min<std::uint8_t>(std::popcount(mask), breakAfter));

        if (numInvocations < breakAfter)
        {
            EXPECT_EQ(enumeratedSquares.popcount(), numInvocations);
        }
        else
        {
            EXPECT_EQ(enumeratedSquares.popcount(), breakAfter);
        }
    };

    enumerateTest(0U, 0U);
    enumerateTest(UINT64_C(1), 0U);
    enumerateTest(UINT64_C(0x12'34'56'78'9A'BC'DE'F0), 8U);
    enumerateTest(UINT64_C(0x12'34'56'78'9A'BC'DE'F0), 63U);
    enumerateTest(UINT64_C(0xFF'00'FF'00'FF'00'FF'00), 7U);
    enumerateTest(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF), 0U);
    enumerateTest(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF), 63U);
    enumerateTest(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF), 64U);
}

TEST(SquareSet, parallelDeposit)
{
    EXPECT_EQ(SquareSet { 0x12'34'56'78'90'AB'CD'EF }.parallelDeposit(SquareSet { 0xF0'F0'F0'F0'F0'F0'F0'F0 }),
              SquareSet { 0x90'00'A0'B0'C0'D0'E0'F0 });
    EXPECT_EQ(SquareSet { 0x12'34'56'78'90'AB'CD'EF }.parallelDeposit(SquareSet { 0x0F'0F'0F'0F'0F'0F'0F'0F }),
              SquareSet { 0x09'00'0A'0B'0C'0D'0E'0F });
}

TEST(SquareSet, parallelExtract)
{
    EXPECT_EQ(SquareSet { 0x12'34'56'78'90'AB'CD'EF }.parallelExtract(SquareSet { 0xF0'F0'F0'F0'F0'F0'F0'F0 }),
              SquareSet { 0x00'00'00'00'13'57'9A'CE });
    EXPECT_EQ(SquareSet { 0x12'34'56'78'90'AB'CD'EF }.parallelExtract(SquareSet { 0x0F'0F'0F'0F'0F'0F'0F'0F }),
              SquareSet { 0x00'00'00'00'24'68'0B'DF });
}

TEST(SquareSet, shiftsAndRotations)
{
    const SquareSet ss1 { 0x01'23'45'67'89'AB'CD'EF };

    EXPECT_EQ(ss1.rotl(4U), SquareSet { 0x12'34'56'78'9A'BC'DE'F0 });
    EXPECT_EQ(ss1.rotr(4U), SquareSet { 0xF0'12'34'56'78'9A'BC'DE });

    SquareSet ss2 { ss1 };
    ss2 <<= 8U;
    EXPECT_EQ(ss2, SquareSet { 0x23'45'67'89'AB'CD'EF'00 });

    ss2 = ss1;
    ss2 >>= 8U;
    EXPECT_EQ(ss2, SquareSet { 0x00'01'23'45'67'89'AB'CD });
}

TEST(SquareSet, flipVert)
{
    EXPECT_EQ(
        SquareSet { 0x12'34'56'78'9A'BC'DE'F0 },
        SquareSet { 0xF0'DE'BC'9A'78'56'34'12 }.flipVert());
}

}
