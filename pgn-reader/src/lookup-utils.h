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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__LOOKUP_UTILS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__LOOKUP_UTILS_H_INCLUDED

#include "chessboard-types.h"

#include <array>
#include <cstdint>


namespace hoover_chess_utils::pgn_reader
{


template <typename U64Type>
static inline U64Type turnSpecificLookup(const std::array<U64Type, 2U> &array, Color turn) noexcept
{
    static_assert(sizeof(U64Type) == 8U);
    static_assert(Color::WHITE == Color { 0U });
    static_assert(Color::BLACK == Color { 8U });

    assert(turn == Color::WHITE || turn == Color::BLACK);

    const std::uint8_t *ptr { std::bit_cast<const std::uint8_t *>(array.data()) };
    const U64Type *ptrAdjusted { std::bit_cast<const U64Type *>(ptr + static_cast<std::ptrdiff_t>(turn)) };
    return *ptrAdjusted;
}

}

#endif
