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

#include "bitrays.h"

#include <array>
#include <bit>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

struct RayDir
{
    std::int8_t dx;
    std::int8_t dy;
};

consteval inline std::uint64_t posToBitMask(std::uint8_t pos) noexcept
{
    return std::uint64_t { 1 } << pos;
}

constexpr std::array<RayDir, 8U> ctRayDirections {{
        { -1, +1 }, // TOP LEFT
        {  0, +1 }, // UP
        { +1, +1 }, // TOP RIGHT
        { +1,  0 }, // RIGHT
        { +1, -1 }, // BOTTOM RIGHT
        {  0, -1 }, // BOTTOM
        { -1, -1 }, // BOTTOM LEFT
        { -1,  0 }, // LEFT
    }};

consteval auto initializeRays() noexcept
{
    std::array<SquareSet, 64U * 8U> ret { };

    for (std::size_t i { }; i < 64U * 8U; ++i)
    {
        std::int8_t x { static_cast<std::int8_t>(i % 8U) };
        std::int8_t y { static_cast<std::int8_t>((i / 8U) % 8U) };
        RayDir dir { ctRayDirections[i / 64U] };

        std::uint64_t ray { };

        while (true)
        {
            x += dir.dx;
            y += dir.dy;

            if (x < 0 || x >= 8)
                break;
            if (y < 0 || y >= 8)
                break;

            ray |= posToBitMask(y * 8 + x);
        }
        ret[i] = SquareSet { ray };
    }

    return ret;
}

}

const std::array<SquareSet, 64U * 8U> RayData::rays { initializeRays() };

}
