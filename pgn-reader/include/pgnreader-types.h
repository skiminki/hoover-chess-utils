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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_TYPES_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_TYPES_H_INCLUDED

#include <cinttypes>

#include "chessboard-types.h"

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Game result
enum class PgnResult : std::uint8_t
{
    /// @brief White win
    WHITE_WIN,

    /// @brief Black win
    BLACK_WIN,

    /// @brief Draw
    DRAW,

    /// @brief Unknown/undetermined
    UNKNOWN,
};

/// @brief Computes the ply number from full move number and side to move.
///
/// @param[in] moveNum     Full move number. Range: [1, 2147483648]
/// @param[in] color       Side to move
/// @return                Ply number
constexpr inline std::uint32_t makePlyNum(std::uint32_t moveNum, Color color) noexcept
{
    assert(moveNum >= 1U);
    [[assume(moveNum >= 1U)]];

    assert(moveNum <= UINT32_C(0x80'00'00'00));
    [[assume(moveNum <= UINT32_C(0x80'00'00'00))]];

    return (moveNum * 2U) - (color == Color::WHITE ? 2U : 1U);
}

/// @brief Returns side to move for a ply number
///
/// @param[in] plyNum      Ply number
/// @return                Side to move
constexpr inline Color colorOfPly(std::uint32_t plyNum) noexcept
{
    return (plyNum & 1U) ? Color::BLACK : Color::WHITE;
}

/// @brief Computes the full move for a ply number
///
/// @param[in] plyNum      Ply number
/// @return                Full move number
constexpr inline std::uint32_t moveNumOfPly(std::uint32_t plyNum) noexcept
{
    return 1U + (plyNum / 2U);
}

/// @}

}

#endif
