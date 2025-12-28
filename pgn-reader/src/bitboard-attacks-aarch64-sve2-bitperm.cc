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

#include "bitboard-attacks-aarch64-sve2-bitperm.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <array>
#include <bit>
#include <cinttypes>

static_assert(HAVE_AARCH64_SVE2_BITPERM, "This file should be included only when PDEP/PEXT is enabled");

namespace hoover_chess_utils::pgn_reader
{

namespace
{

consteval auto generatePextRookMasks() noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        SquareSet mask {
            SquareSet::column(col) ^
            SquareSet::row(row)
        };

        // remove tips of the attack rays
        mask &=
            ~(SquareSet::square(col, 0U) | SquareSet::square(col, 7U) |
              SquareSet::square(0U, row) | SquareSet::square(7U, row));

        ret[sq] = static_cast<std::uint64_t>(mask);
    }

    return ret;
}

consteval auto generatePextBishopMasks() noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        const SquareSet diagBLTR { 0x80'40'20'10'08'04'02'01 };
        const std::int8_t shiftBLTR { static_cast<std::int8_t>(col - row) };
        const SquareSet diagBLTRShifted { (shiftBLTR >= 0) ? diagBLTR >> (shiftBLTR * 8) : diagBLTR << (-shiftBLTR) * 8 };

        const SquareSet diagBRTL { 0x01'02'04'08'10'20'40'80 };
        const std::int8_t shiftBRTL { static_cast<std::int8_t>(7U - col - row) };
        const SquareSet diagBRTLShifted { (shiftBRTL >= 0) ? diagBRTL >> (shiftBRTL * 8) : diagBRTL << (-shiftBRTL) * 8 };

        SquareSet mask { diagBLTRShifted ^ diagBRTLShifted };

        // remove borders
        mask &= ~(SquareSet::column(0U) | SquareSet::column(7) | SquareSet::row(0U) | SquareSet::row(7U));

        ret[sq] = static_cast<std::uint64_t>(mask);
    }

    return ret;
}

consteval auto generatePextOffsets(const std::array<std::uint64_t, 64U> &masks) noexcept
{
    std::array<std::uint32_t, 64U> ret { };
    std::uint32_t offset { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        ret[sq] = offset;

        offset += std::uint32_t { 1 } << std::popcount(masks[sq]);
    }

    return ret;
}

consteval auto calculatePextDataSize(const std::array<std::uint64_t, 64U> &masks) noexcept
{
    std::uint32_t offset { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
        offset += std::uint32_t { 1 } << std::popcount(masks[sq]);

    return offset;
}

}

// masks for PEXT/PDEP. In general, these have the form:
//
// 0 0 0 0 0 0 0 0
// 0 0 0 1 0 0 0 0
// 0 0 0 1 0 0 0 0
// 0 1 1 R 1 1 1 0
// 0 0 0 1 0 0 0 0
// 0 0 0 1 0 0 0 0
// 0 0 0 1 0 0 0 0
// 0 0 0 0 0 0 0 0
//
// 'R' is 0
const std::array<std::uint64_t, 64U> Attacks_AArch64_SVE2_BitPerm::ctPextRookMasks { generatePextRookMasks() };
const std::array<std::uint64_t, 64U> Attacks_AArch64_SVE2_BitPerm::ctPextBishopMasks { generatePextBishopMasks() };


const std::array<std::uint32_t, 64U> Attacks_AArch64_SVE2_BitPerm::ctPextRookOffsets {
    generatePextOffsets(generatePextRookMasks()) };
const std::array<std::uint32_t, 64U> Attacks_AArch64_SVE2_BitPerm::ctPextBishopOffsets {
    generatePextOffsets(generatePextBishopMasks()) };

const std::array<std::uint64_t, calculatePextDataSize(generatePextRookMasks())> Attacks_AArch64_SVE2_BitPerm::ctPextRookAttackData {

#include "slider-attacks-pext-pdep-rook.inc"

};

const std::array<std::uint64_t, calculatePextDataSize(generatePextBishopMasks())> Attacks_AArch64_SVE2_BitPerm::ctPextBishopAttackData {

#include "slider-attacks-pext-pdep-bishop.inc"

};

}
