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

#include "src/bittricks.h"

#include "gtest/gtest.h"


namespace hoover_chess_utils::pgn_reader::unit_test
{

#define STATIC_ASSERT_AND_TEST(x)               \
    do {                                        \
        static_assert(x);                       \
        EXPECT_TRUE(x);                         \
    } while (false)


TEST(BitTricks, bits0ToN)
{
    STATIC_ASSERT_AND_TEST(BitTricks::bits0ToN(0U) == 1U);
    STATIC_ASSERT_AND_TEST(BitTricks::bits0ToN(7U) == 255U);
    STATIC_ASSERT_AND_TEST(BitTricks::bits0ToN(63U) == 0xFF'FF'FF'FF'FF'FF'FF'FFU);
}

TEST(BitTricks, rangeHalfOpen)
{
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(0U, 0U) == 0U);
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(10U, 10U) == 0U);
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(63U, 63U) == 0U);

    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(0U, 1U) == 2U);
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(10U, 11U) == 0x08'00U);
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(63U, 62U) == 0x80'00'00'00'00'00'00'00U);
    STATIC_ASSERT_AND_TEST(BitTricks::rangeHalfOpen(3U, 8U) == 0x01'F0U);
}

TEST(BitTricks, isolateLowestSetBit)
{
    STATIC_ASSERT_AND_TEST(BitTricks::isolateLowestSetBit(0U) == 0U);
    STATIC_ASSERT_AND_TEST(BitTricks::isolateLowestSetBit(0xFF'FF'FF'FF'FF'FF'FF'FFU) == 1U);
    STATIC_ASSERT_AND_TEST(BitTricks::isolateLowestSetBit(0x82'3F'00'A4'00'00'00'00U) == 0x04'00'00'00'00U);
}

TEST(BitTricks, isolateHighestSetBit)
{
    STATIC_ASSERT_AND_TEST(BitTricks::isolateHighestSetBit(0U) == 0U);
    STATIC_ASSERT_AND_TEST(BitTricks::isolateHighestSetBit(1U) == 1U);
    STATIC_ASSERT_AND_TEST(BitTricks::isolateHighestSetBit(0xFF'FF'FF'FF'FF'FF'FF'FFU) == 0x80'00'00'00'00'00'00'00U);
    STATIC_ASSERT_AND_TEST(BitTricks::isolateHighestSetBit(0x30'3F'00'A4'00'00'00'00U) == 0x20'00'00'00'00'00'00'00U);
}


}
