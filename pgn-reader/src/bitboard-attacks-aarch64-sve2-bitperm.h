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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_AARCH64_SVE2_BITPERM_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_AARCH64_SVE2_BITPERM_H_INCLUDED

#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <array>
#include <cinttypes>
#include <arm_sve.h>

static_assert(HAVE_AARCH64_SVE2_BITPERM, "This file should be included only when AArch64 SVE2 BitPerm is available");

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Slider attacks implementation using PEXT/PDEP
class Attacks_AArch64_SVE2_BitPerm
{
private:
    static const std::array<std::uint64_t, 64U> ctPextRookMasks;
    static const std::array<std::uint32_t, 64U> ctPextRookOffsets;
    static const std::array<std::uint64_t, 102400U> ctPextRookAttackData;
    static const std::array<std::uint64_t, 64U> ctPextBishopMasks;
    static const std::array<std::uint32_t, 64U> ctPextBishopOffsets;
    static const std::array<std::uint64_t, 5248U> ctPextBishopAttackData;


public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctPextBishopMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t offset { ctPextBishopOffsets[static_cast<std::uint8_t>(sq)] };

        svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };

        svuint64_t extractedV { svbext_n_u64(occupancyMaskV, static_cast<std::uint64_t>(pextMask)) };
        const std::uint64_t extracted { extractedV[0U] };

        return SquareSet { ctPextBishopAttackData[offset + extracted] };
    }

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctPextRookMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t offset { ctPextRookOffsets[static_cast<std::uint8_t>(sq)] };

        svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };

        svuint64_t extractedV { svbext_n_u64(occupancyMaskV, static_cast<std::uint64_t>(pextMask)) };
        const std::uint64_t extracted { extractedV[0U] };

        return SquareSet { ctPextRookAttackData[offset + extracted] };
    }

    /// @brief See @coderef{Attacks::getQueenAttackMask()} for documentation
    static inline SquareSet getQueenAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t bishopPextMask { ctPextBishopMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t bishopOffset { ctPextBishopOffsets[static_cast<std::uint8_t>(sq)] };

        const std::uint64_t rookPextMask { ctPextRookMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t rookOffset { ctPextRookOffsets[static_cast<std::uint8_t>(sq)] };

        svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };
        svuint64_t pextMaskV { svdupq_u64(bishopPextMask, rookPextMask) };

        svuint64_t extractedV { svbext_u64(occupancyMaskV, pextMaskV) };

        return
            SquareSet { ctPextBishopAttackData[bishopOffset + extractedV[0U]] } |
            SquareSet { ctPextRookAttackData[rookOffset + extractedV[1U]] };
    }
};

}

#endif
