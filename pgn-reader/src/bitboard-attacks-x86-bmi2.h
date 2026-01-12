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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_X86_BMI2_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_X86_BMI2_H_INCLUDED

#include "pgnreader-config.h"

#include "bitboard-tables.h"
#include "chessboard-types-squareset.h"

#include <array>
#include <cinttypes>
#include <immintrin.h>

static_assert(HAVE_X86_BMI2, "This file should be included only when PDEP/PEXT is available");

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Slider attacks implementation using PEXT/PDEP
class Attacks_BMI2
{
public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static inline SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctBitBoardTables.bmi2BishopMasks[static_cast<SquareUnderlyingType>(sq)] };
        const std::uint32_t offset { ctBitBoardTables.bmi2BishopOffsets[static_cast<SquareUnderlyingType>(sq)] };

        return SquareSet {
            ctBitBoardTables.bmi2BishopRookAttackData[
                offset +
                _pext_u64(
                    static_cast<std::uint64_t>(occupancyMask),
                    pextMask)] };
    }

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctBitBoardTables.bmi2RookMasks[static_cast<SquareUnderlyingType>(sq)] };
        const std::uint32_t offset { ctBitBoardTables.bmi2RookOffsets[static_cast<SquareUnderlyingType>(sq)] };

        return SquareSet {
            ctBitBoardTables.bmi2BishopRookAttackData[
                offset +
                _pext_u64(
                    static_cast<std::uint64_t>(occupancyMask),
                    pextMask)] };
    }
};

}

#endif
