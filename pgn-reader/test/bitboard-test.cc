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

#include "src/bitboard_rays.h"

#include <cstdio>
#include <string_view>
#include <iostream>

namespace
{
}

TEST(BBTest, betweenHorizSquares)
{
    using namespace hoover_chess_utils::pgn_reader;

    using namespace hoover_chess_utils::pgn_reader;

    for (std::uint8_t a = 0U; a < 64U; ++a)
    {
        for (std::uint8_t b = 0U; b < 64U; ++b)
        {
            std::uint64_t expect { };

            if (a <= b)
            {
                for (std::uint8_t c = a + 1; c < b; ++c)
                    expect |= UINT64_C(1) << c;
            }
            else
            {
                for (std::uint8_t c = b + 1; c < a; ++c)
                    expect |= UINT64_C(1) << c;
            }
            std::uint64_t v { BB::betweenHorizSquares(a, b) };
            EXPECT_EQ(expect, v);
            if (expect != v)
            {
                printf("expect: 0x%016" PRIx64 "  value: 0x%016" PRIx64 "\n",
                       expect, v);
                return;
            }
        }
    }
}

TEST(BBTest, horizLine)
{
    using namespace hoover_chess_utils::pgn_reader;

    for (std::uint8_t a = 0U; a < 64U; ++a)
    {
        for (std::uint8_t b = 0U; b < 64U; ++b)
        {
            std::uint64_t expect { };

            if (a <= b)
            {
                for (std::uint8_t c = a; c <= b; ++c)
                    expect |= UINT64_C(1) << c;
            }
            else
            {
                for (std::uint8_t c = b; c <= a; ++c)
                    expect |= UINT64_C(1) << c;
            }
            std::uint64_t v { BB::horizSquares(a, b) };
            EXPECT_EQ(expect, v);
            if (expect != v)
            {
                printf("%u..%u: expect: 0x%016" PRIx64 "  value: 0x%016" PRIx64 "\n",
                       a, b, expect, v);
                printf("mask1: 0x%016lx\n", (~std::uint64_t(0)) << a);
                printf("mask2: 0x%016lx\n", (UINT64_C(1) << b) - 1U);
                return;
            }
        }
    }
}

TEST(BBTest, semiOpenHorizSquares)
{
    using namespace hoover_chess_utils::pgn_reader;

    for (std::uint8_t a = 0U; a < 64U; ++a)
    {
        for (std::uint8_t b = 0U; b < 64U; ++b)
        {
            std::uint64_t expect { BB::horizSquares(a, b) ^ BB::coordToBit(b) };

            std::uint64_t v { BB::semiOpenHorizSquares(a, b) };
            EXPECT_EQ(expect, v);
            if (expect != v)
            {
                printf("%u..%u: expect: 0x%016" PRIx64 "  value: 0x%016" PRIx64 "\n",
                       a, b, expect, v);
                printf("mask1: 0x%016lx\n", (~std::uint64_t(0)) << a);
                printf("mask2: 0x%016lx\n", (UINT64_C(1) << b) - 1U);
                return;
            }
        }
    }
}
