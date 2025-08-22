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

#include "src/stringbuilder.h"

#include "gtest/gtest.h"

#include <functional>
#include <random>
#include <string>


namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace
{

std::mt19937 s_randomGenerator { };


std::string getRandomString(std::size_t len)
{
    std::uniform_int_distribution<> distrib('A', 'Z');

    std::string ret { };
    ret.resize(len);

    for (std::size_t i { }; i < len; ++i)
        ret[i] = distrib(s_randomGenerator);

    return ret;
}

void testSplicedAppend(std::size_t firstSegment, std::size_t secondSegment)
{
    std::string inputString { getRandomString(firstSegment + secondSegment) };

    StringBuilder sb { };
    sb.appendString(inputString.data(), firstSegment);
    sb.appendString(inputString.data() + firstSegment, secondSegment);

    EXPECT_EQ(std::string_view { inputString }, sb.getStringView());
}

}

TEST(StringBuilder, appendString)
{
    StringBuilder sb { };

    s_randomGenerator.seed(12345678U);

    // first the very basics with couple of sizes
    for (std::size_t firstSegment : { 0U, 1U, 16U })
        for (std::size_t secondSegment : { 0U, 1U, 16U })
            testSplicedAppend(firstSegment, secondSegment);

    // boundary value testing around the dynamic allocation base size
    static_assert(StringBuilder::ctDynamicAllocBase >= 100U); // just a test assumption
    for (std::size_t totalSize : {
            StringBuilder::ctDynamicAllocBase,
                StringBuilder::ctDynamicAllocBase + 1U,
                2U * StringBuilder::ctDynamicAllocBase - 1U,
                2U * StringBuilder::ctDynamicAllocBase,
                2U * StringBuilder::ctDynamicAllocBase + 1U })
        for (std::size_t segmentSize : { 0U, 1U, 2U, 100U })
        {
            testSplicedAppend(totalSize - segmentSize, segmentSize);
            testSplicedAppend(segmentSize, totalSize - segmentSize);
        }
}

TEST(StringBuilder, pushBack)
{
    StringBuilder sb { };

    s_randomGenerator.seed(42424242U);

    std::string input { getRandomString((4U * StringBuilder::ctDynamicAllocBase) + 1U) };

    for (std::size_t i { }; i < input.size(); ++i)
    {
        EXPECT_EQ(std::string_view(input.begin(), input.begin() + i), sb.getStringView());
        sb.pushBack(input[i]);
    }

    EXPECT_EQ(std::string_view(input), sb.getStringView());
}

}
