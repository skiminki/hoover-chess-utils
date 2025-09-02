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

/// @addtogroup PgnReaderImpl
/// @{

/// @brief Piece attack tables


/// @brief Optimized lookup for arrays indexed by turn (side-to-move).
///
/// @tparam     U64Type    64-bit type, generally intended to be @coderef{SquareSet} or @c std::uint64_t
/// @param[in]  array      Array of U64Type elements, size 2 (element for @coderef{Color::WHITE} and @coderef{Color::BLACK})
/// @param[in]  turn       Side to move. Must be either @coderef{Color::WHITE} or @coderef{Color::BLACK}
///                        (asserted)
/// @return                Const reference to the element
///
/// The expected array layout is
/// <table>
/// <tr>
///   <th>Index</th>
///   <th>Content</th>
/// </tr>
/// <tr>
///   <td>0</td>
///   <td>Element for @coderef{Color::WHITE}</td>
/// </tr>
/// <tr>
///   <td>1</td>
///   <td>Element for @coderef{Color::BLACK}</td>
/// </tr>
/// </table>
///
/// The implementation makes a strong assumption that @p turn is one of { @coderef{Color::WHITE}, @coderef{Color::BLACK} }.
/// This can eliminate otherwise superfluous arithmetic operations when computing the memory address. In particular,
/// @p turn can be used as is as the array byte offset, instead of resetting the lowest 3 bits
/// (i.e., <tt>byte_offset = (turn / 8) * 8</tt>).
template <typename U64Type>
inline const U64Type &turnSpecificLookup(const std::array<U64Type, 2U> &array, Color turn) noexcept
{
    static_assert(sizeof(U64Type) == 8U);
    static_assert(Color::WHITE == Color { 0U });
    static_assert(Color::BLACK == Color { 8U });

    assert(turn == Color::WHITE || turn == Color::BLACK);

    const std::uint8_t *ptr { std::bit_cast<const std::uint8_t *>(array.data()) };
    const U64Type *ptrAdjusted { std::bit_cast<const U64Type *>(ptr + static_cast<std::ptrdiff_t>(turn)) };
    return *ptrAdjusted;
}

/// @brief Optimized lookup for arrays indexed by general index and turn (side-to-move).
///
/// @tparam     U64Type    64-bit type, generally intended to be @coderef{SquareSet} or @c std::uint64_t
/// @tparam     N          Array length in pairs of U64Type
/// @param[in]  array      Array of U64Type element pairs
/// @param[in]  i          Array index
/// @param[in]  turn       Side to move. Must be either @coderef{Color::WHITE} or @coderef{Color::BLACK}
///                        (asserted)
/// @return                Const reference to the element
///
/// The implementation makes a strong assumption that @p turn is one of { @coderef{Color::WHITE}, @coderef{Color::BLACK} }.
/// This can eliminate otherwise superfluous arithmetic operations when computing the memory address. In particular,
/// @p turn can be used as is as the array byte offset, instead of resetting the lowest 3 bits
/// (i.e., <tt>byte_offset = (turn / 8) * 8</tt>).
template <typename U64Type, std::size_t N>
inline const U64Type &turnSpecificArrayLookup(
    const std::array<std::array<U64Type, 2U>, N> &array, std::size_t i, Color turn) noexcept
{
    static_assert(sizeof(U64Type) == 8U);
    static_assert(Color::WHITE == Color { 0U });
    static_assert(Color::BLACK == Color { 8U });

    assert(turn == Color::WHITE || turn == Color::BLACK);
    assert(i < N);

    const std::uint8_t *ptr { std::bit_cast<const std::uint8_t *>(array.data()) };
    const U64Type *ptrAdjusted {
        std::bit_cast<const U64Type *>(
            ptr + static_cast<std::ptrdiff_t>(turn) + 16U * i) };
    return *ptrAdjusted;
}



/// @}

}

#endif
