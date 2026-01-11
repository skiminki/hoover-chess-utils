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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_ELEMENTARY_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_ELEMENTARY_H_INCLUDED

#include "pgnreader-config.h"

#include "bitboard-tables.h"
#include "chessboard-types-squareset.h"

#include <array>
#include <cinttypes>


namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Slider attacks implementation using Elementary Bitboards
class Attacks_Elementary_Bitboards
{
public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static inline SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint32_t offset { ctBitBoardTables.elementaryBishopOffsets[static_cast<SquareUnderlyingType>(sq)] };
        const std::uint64_t occupMask { static_cast<std::uint64_t>(occupancyMask) };

        const BitBoardTables::MasksAndMultipliers &masksAndMults { ctBitBoardTables.elementaryBishopMaskMults[static_cast<std::uint8_t>(sq)] };
        std::uint64_t product { };
        for (std::size_t i { }; i < 3U; ++i)
            product |= (occupMask & masksAndMults.masks[i]) * masksAndMults.multipliers[i];

        std::uint64_t baseOccupancy { product >> masksAndMults.shift };
        return SquareSet { ctBitBoardTables.elementaryBishopRookAttackData[offset + baseOccupancy] };
    }

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint32_t offset { ctBitBoardTables.elementaryRookOffsets[static_cast<SquareUnderlyingType>(sq)] };
        const std::uint64_t occupMask { static_cast<std::uint64_t>(occupancyMask) };

        const BitBoardTables::MasksAndMultipliers &masksAndMults { ctBitBoardTables.elementaryRookMaskMults[static_cast<std::uint8_t>(sq)] };
        std::uint64_t product { };
        for (std::size_t i { }; i < 3U; ++i)
            product |= (occupMask & masksAndMults.masks[i]) * masksAndMults.multipliers[i];

        std::uint64_t baseOccupancy { product >> masksAndMults.shift };
        return SquareSet { ctBitBoardTables.elementaryBishopRookAttackData[offset + baseOccupancy] };
    }
};

}

#endif
