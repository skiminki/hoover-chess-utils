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

#include "bitboard-attacks-x86-bmi2.h"
#include "bittricks.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <array>
#include <bit>
#include <cinttypes>


static_assert(false, "This file should be included only if the PEXT data needs to be recreated");

namespace hoover_chess_utils::pgn_reader
{

namespace
{

[[maybe_unused]]
auto calculatePextRookAttackData(const std::array<SquareSet, 64U> &masks, const std::array<std::uint32_t, 64U> &offsets) noexcept
{
    std::array<std::uint64_t, 102400U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        SquareSet mask { masks[sq] };
        std::uint64_t offset { offsets[sq] };
        std::uint64_t numPositions { std::uint64_t { 1 } << mask.popcount() };

        for (std::uint64_t pos { }; pos < numPositions; ++pos)
        {
            ret[offset + pos] =
                static_cast<std::uint64_t>(
                    shootRaysPlusReturnTraces(Square { sq },
                                              SquareSet { BitTricks::parallelDeposit(pos, static_cast<std::uint64_t>(mask)) }));
        }
    }

    for (std::size_t i { }; i < ret.size(); ++i)
    {
        printf("0x%016lXU, ", static_cast<std::uint64_t>(ret[i]));
        if ((i & 15) == 15)
            printf("\n");
    }

    return ret;
}

[[maybe_unused]]
constexpr auto calculatePextBishopAttackData(const std::array<SquareSet, 64U> &masks, const std::array<std::uint32_t, 64U> &offsets) noexcept
{
    std::array<std::uint64_t, 5248U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        SquareSet mask { masks[sq] };
        std::uint64_t offset { offsets[sq] };
        std::uint64_t numPositions { std::uint64_t { 1 } << mask.popcount() };

        for (std::uint64_t pos { }; pos < numPositions; ++pos)
        {
            ret[offset + pos] =
                static_cast<std::uint64_t>(
                    shootRaysCrossReturnTraces(Square { sq }, SquareSet { BitTricks::parallelDeposit(pos, static_cast<std::uint64_t>(mask)) } ));
        }
    }

    for (std::size_t i { }; i < ret.size(); ++i)
    {
        printf("0x%016lXU, ", static_cast<std::uint64_t>(ret[i]));
        if ((i & 15) == 15)
            printf("\n");
    }

    return ret;
}

}

}
