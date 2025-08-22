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

#include "version.h"

#include "pgnreader-config.h"

#include "gtest/gtest.h"

#include <format>
#include <string>
#include <string_view>

namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(Version, apiVersionDefines)
{
    EXPECT_EQ(HOOVER_CHESS_UTILS_VERSION_MAJOR, PROJECT_VERSION_MAJOR);
    EXPECT_EQ(HOOVER_CHESS_UTILS_VERSION_MINOR, PROJECT_VERSION_MINOR);
    EXPECT_EQ(HOOVER_CHESS_UTILS_VERSION_PATCH, PROJECT_VERSION_PATCH);
    EXPECT_EQ(HOOVER_CHESS_UTILS_VERSION_SUFFIX, PROJECT_VERSION_SUFFIX);
}

TEST(Version, apiVersionString)
{
    std::cout << "Hoover Chess Utilities version: " << getVersionString() << std::endl;

    const std::string expectedVersionString {
        std::format("{}.{}.{}{}",
                    PROJECT_VERSION_MAJOR,
                    PROJECT_VERSION_MINOR,
                    PROJECT_VERSION_PATCH,
                    PROJECT_VERSION_SUFFIX,
                    1)
    };

    EXPECT_EQ(static_cast<std::string_view>(expectedVersionString), getVersionString());
}


}
