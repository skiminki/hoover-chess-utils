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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_PEXT_PDEP_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_PEXT_PDEP_H_INCLUDED

#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <array>
#include <cinttypes>
#include <immintrin.h>

static_assert(HAVE_PDEP_PEXT, "This file should be included only when PDEP/PEXT is available");

namespace hoover_chess_utils::pgn_reader
{

class CompileTimeInitializers
{
public:
};

/// @ingroup PgnReaderImpl
/// @brief Slider attacks implementation using PEXT/PDEP
class Attacks_PextPdep
{
private:
    static const std::array<SquareSet, 64U> ctPextRookMasks;
    static const std::array<std::uint32_t, 64U> ctPextRookOffsets;
    static const std::array<std::uint64_t, 102400U> ctPextRookAttackData;
    static const std::array<SquareSet, 64U> ctPextBishopMasks;
    static const std::array<std::uint32_t, 64U> ctPextBishopOffsets;
    static const std::array<std::uint64_t, 5248U> ctPextBishopAttackData;


public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static inline SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const SquareSet pextMask { ctPextBishopMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t offset { ctPextBishopOffsets[static_cast<std::uint8_t>(sq)] };

        return SquareSet {
            ctPextBishopAttackData[
                offset +
                _pext_u64(
                    static_cast<std::uint64_t>(occupancyMask),
                    static_cast<std::uint64_t>(pextMask))] };
    }

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const SquareSet pextMask { ctPextRookMasks[static_cast<std::uint8_t>(sq)] };
        const std::uint64_t offset { ctPextRookOffsets[static_cast<std::uint8_t>(sq)] };

        return SquareSet {
            ctPextRookAttackData[
                offset +
                _pext_u64(
                    static_cast<std::uint64_t>(occupancyMask),
                    static_cast<std::uint64_t>(pextMask))] };
    }
};

}

#endif
