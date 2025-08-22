// Hoover Chess Utilities / PGN reader
// Copyright (C) 2024-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITRAYS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITRAYS_H_INCLUDED

#include "bittricks.h"
#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include <array>
#include <bit>
#include <cinttypes>
#include <cstdlib>
#include <cstdio>

namespace hoover_chess_utils::pgn_reader
{

enum class RayDirectionIndex : std::uint8_t
{
    // directions where position of hit is greater than origin
    LEFT_FORWARDS = 0U,
    FORWARDS,
    RIGHT_FORWARDS,
    RIGHT,

    // directions where position of hit is less than origin
    RIGHT_BACKWARDS,
    BACKWARDS,
    LEFT_BACKWARDS,
    LEFT,
};

struct RayData
{
    static const std::array<SquareSet, 64U * 8U> rays;

    template <RayDirectionIndex dirIndex>

    static constexpr SquareSet getRay(std::uint8_t from) noexcept
    {
        return rays[static_cast<std::size_t>(dirIndex) * 64U + from];
    }
};

constexpr inline SquareSet shootRaysPlusReturnTraces(Square from, SquareSet occupancyMask) noexcept
{
    std::uint64_t trace { };

    // from to T
    const std::uint64_t top { RayData::getRay<RayDirectionIndex::FORWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitTop {
        BitTricks::isolateLowestSetBit(top & static_cast<std::uint64_t>(occupancyMask)) };
    trace |= (2U * hitTop - 1U) & top;

    // from to R
    const std::uint64_t right { RayData::getRay<RayDirectionIndex::RIGHT>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitRight {
        BitTricks::isolateLowestSetBit(right & static_cast<std::uint64_t>(occupancyMask)) };
    trace |= (2U * hitRight - 1U) & right;

    // from to B
    const std::uint64_t bottom { RayData::getRay<RayDirectionIndex::BACKWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitBottom {
        BitTricks::isolateHighestSetBit(bottom & (static_cast<std::uint64_t>(occupancyMask) | UINT64_C(0x00'00'00'00'00'00'00'FF))) };
    trace |= (-hitBottom) & bottom;

    // from to L
    const std::uint64_t left { RayData::getRay<RayDirectionIndex::LEFT>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitLeft {
        BitTricks::isolateHighestSetBit(left & (static_cast<std::uint64_t>(occupancyMask) | UINT64_C(0x01'01'01'01'01'01'01'01))) };
    trace |= (-hitLeft) & left;

    return SquareSet { trace };
}

constexpr inline SquareSet shootRaysCrossReturnTraces(Square from, SquareSet occupancyMask) noexcept
{
    std::uint64_t trace { };

    // from to TL
    const std::uint64_t topLeft { RayData::getRay<RayDirectionIndex::LEFT_FORWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitTopLeft {
        BitTricks::isolateLowestSetBit(topLeft & static_cast<std::uint64_t>(occupancyMask)) };
    trace |= (2U * hitTopLeft - 1U) & topLeft;

    // from to TR
    const std::uint64_t topRight { RayData::getRay<RayDirectionIndex::RIGHT_FORWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitTopRight {
        BitTricks::isolateLowestSetBit(topRight & static_cast<std::uint64_t>(occupancyMask)) };
    trace |= (2U * hitTopRight - 1U) & topRight;

    // from to BL
    const std::uint64_t bottomLeft { RayData::getRay<RayDirectionIndex::LEFT_BACKWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitBottomLeft {
        BitTricks::isolateHighestSetBit(bottomLeft & (static_cast<std::uint64_t>(occupancyMask) | UINT64_C(0x01'01'01'01'01'01'01'FF))) };
    trace |= (-hitBottomLeft) & bottomLeft;

    // from to BR
    const std::uint64_t bottomRight { RayData::getRay<RayDirectionIndex::RIGHT_BACKWARDS>(static_cast<std::uint8_t>(from)) };
    const std::uint64_t hitBottomRight {
        BitTricks::isolateHighestSetBit(bottomRight & (static_cast<std::uint64_t>(occupancyMask) | UINT64_C(0x80'80'80'80'80'80'80'FF))) };
    trace |= (-hitBottomRight) & bottomRight;

    return SquareSet { trace };
}

}

#endif
