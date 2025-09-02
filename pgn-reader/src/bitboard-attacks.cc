// Hoover Chess Utilities / PGN reader
// Copyright (C) 2023-2025  Sami Kiminki
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

#include "bitboard-attacks.h"

#include <bit>

namespace hoover_chess_utils::pgn_reader
{

namespace {

// shift mask with overflow/wraparound prevention
// negative x/y = shift left/up
consteval std::uint64_t shiftMask(std::uint64_t mask, std::int8_t x, std::int8_t y) noexcept
{
    std::uint64_t nonWrappedMask { };

    // on rank 0
    std::uint64_t nonWrappedColumns { };

    // cull off wraparound bits
    if (x >= 0)
    {
        nonWrappedColumns = (UINT64_C(0xFF) << x) & 0xFFU;
    }
    else
    {
        nonWrappedColumns = UINT64_C(0xFF) >> (-x);
    }

    // broadcast row 0 to all rows
    nonWrappedMask =
        (nonWrappedColumns << 0U)  | (nonWrappedColumns << 8U)  | (nonWrappedColumns << 16U) | (nonWrappedColumns << 24U) |
        (nonWrappedColumns << 32U) | (nonWrappedColumns << 40U) | (nonWrappedColumns << 48U) | (nonWrappedColumns << 56U);

    if (y >= 0)
    {
        nonWrappedMask &= UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF) << (y * 8);
    }
    else
    {
        nonWrappedMask &= UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF) >> ((-y) * 8);
    }

    return std::rotl(mask, x + (8 * y)) & nonWrappedMask;
}

static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0,  0) == UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  1,  0) == UINT64_C(0xFE'FE'FE'FE'FE'FE'FE'FE));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF), -1,  0) == UINT64_C(0x7F'7F'7F'7F'7F'7F'7F'7F));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0,  1) == UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'00));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0, -1) == UINT64_C(0x00'FF'FF'FF'FF'FF'FF'FF));

consteval auto generateAttackMaskTableFromPattern(
    std::uint64_t pattern,
    std::uint8_t origX,
    std::uint8_t origY) noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t y { }; y < 8U; ++y)
    {
        for (std::uint8_t x { }; x < 8U; ++x)
        {
            ret[(y * 8) + x] = shiftMask(pattern, x - origX, y - origY);
        }
    }

    return ret;
}

consteval auto generateWhitePawnAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 0U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY + 1U) |
        SquareSet::square(origX + 1U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateBlackPawnAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 1U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 1U) |
        SquareSet::square(origX + 1U, origY - 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateKnightAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 2U };
    constexpr std::uint8_t origY { 2U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 2U) |
        SquareSet::square(origX - 1U, origY + 2U) |
        SquareSet::square(origX - 2U, origY - 1U) |
        SquareSet::square(origX - 2U, origY + 1U) |
        SquareSet::square(origX + 1U, origY - 2U) |
        SquareSet::square(origX + 1U, origY + 2U) |
        SquareSet::square(origX + 2U, origY - 1U) |
        SquareSet::square(origX + 2U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateKingAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 1U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 1U) |
        SquareSet::square(origX     , origY - 1U) |
        SquareSet::square(origX + 1U, origY - 1U) |
        SquareSet::square(origX - 1U, origY     ) |
        SquareSet::square(origX + 1U, origY     ) |
        SquareSet::square(origX - 1U, origY + 1U) |
        SquareSet::square(origX     , origY + 1U) |
        SquareSet::square(origX + 1U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

template <std::size_t N, std::size_t M>
consteval auto concatenateArrays(const std::array<std::uint64_t, N> &first, const std::array<std::uint64_t, M> &second) noexcept
{
    std::array<std::uint64_t, N + M> ret { };

    for (std::size_t i { }; i < N; ++i)
        ret[i] = first[i];

    for (std::size_t i { }; i < M; ++i)
        ret[N + i] = second[i];

    return ret;
}

template <std::size_t N>
consteval auto interleaveArrays(const std::array<std::uint64_t, N> &first, const std::array<std::uint64_t, N> &second) noexcept
{
    std::array<std::array<std::uint64_t, 2U>, N> ret { };

    for (std::size_t i { }; i < N; ++i)
    {
        ret[i][0U] = first[i];
        ret[i][1U] = second[i];
    }

    return ret;
}

}

const std::array<std::array<std::uint64_t, 2U>, 64U > Attacks::ctPawnAttackMaskTable {
    interleaveArrays(generateWhitePawnAttackMaskTable(), generateBlackPawnAttackMaskTable())
};

const std::array<std::uint64_t, 64U> Attacks::ctKnightAttackMaskTable { generateKnightAttackMaskTable() };
const std::array<std::uint64_t, 64U> Attacks::ctKingAttackMaskTable { generateKingAttackMaskTable() };

}
