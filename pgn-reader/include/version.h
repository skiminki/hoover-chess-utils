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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__VERSION_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__VERSION_H_INCLUDED

#include <string_view>


#define HOOVER_CHESS_UTILS_VERSION_MAJOR 0
#define HOOVER_CHESS_UTILS_VERSION_MINOR 9
#define HOOVER_CHESS_UTILS_VERSION_PATCH 0
#define HOOVER_CHESS_UTILS_VERSION_SUFFIX "-dev"

namespace hoover_chess_utils::pgn_reader
{

constexpr std::string_view getVersionString() noexcept
{
    return std::string_view { "0.9.0-dev" };
}

}

#endif
