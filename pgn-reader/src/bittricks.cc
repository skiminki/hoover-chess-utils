// Hoover Chess Utilities / PGN reader
// Copyright (C) 2026  Sami Kiminki
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

#include "bittricks.h"

#include <array>
#include <bit>
#include <cstdint>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

constexpr std::array<std::array<std::uint8_t, 256U>, 256U> pext8x8 {{
#include "bittricks-table-pext8x8.inc"
}};

constexpr std::array<std::array<std::uint8_t, 256U>, 256U> pdep8x8 {{
#include "bittricks-table-pdep8x8.inc"
}};

}

std::uint64_t BitTricks::parallelExtractPortable(std::uint64_t data, std::uint64_t mask) noexcept
{
    std::uint64_t ret { };

    for (unsigned maskShift = 0U; maskShift < 64U; maskShift += 8U)
    {
        const std::uint64_t prevMask { (UINT64_C(1) << maskShift) - 1U };
        const unsigned retShift = std::popcount(mask & prevMask);

        ret |= pext8x8[(mask >> maskShift) & 0xFFU][(data >> maskShift) & 0xFFU] << retShift;
    }

    return ret;
}

std::uint64_t BitTricks::parallelDepositPortable(std::uint64_t data, std::uint64_t mask) noexcept
{
    std::uint64_t ret { };

    for (unsigned maskShift = 0U; maskShift < 64U; maskShift += 8U)
    {
        const std::uint64_t prevMask { (UINT64_C(1) << maskShift) - 1U };
        const unsigned dataShift = std::popcount(mask & prevMask);

        ret |= std::uint64_t { pdep8x8[(mask >> maskShift) & 0xFFU][(data >> dataShift) & 0xFFU] } << maskShift;
    }

    return ret;
}

}
