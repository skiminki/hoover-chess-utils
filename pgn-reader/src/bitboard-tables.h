// Hoover Chess Utilities / PGN reader
// Copyright (C) 2023-2026  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_TABLES_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_TABLES_H_INCLUDED

#include "pgnreader-config.h"

#include <array>
#include <cstdint>

#define BITBOARD_TABLES_HAVE_X86_BMI2             (HAVE_X86_BMI2)
#define BITBOARD_TABLES_HAVE_AARCH64_SVE2_BITPERM (HAVE_AARCH64_SVE2_BITPERM)
#define BITBOARD_TABLES_HAVE_HYPERBOLA            (!(HAVE_X86_BMI2 || HAVE_AARCH64_SVE2_BITPERM))
#define BITBOARD_TABLES_HAVE_ELEMENTARY           0

namespace hoover_chess_utils::pgn_reader
{

struct BitBoardTables
{
    /// @brief Pawn attack masks
    ///
    /// This table is used for both pawn-to-attacked-square and attacking-pawn-to-square
    /// tables. Hence the inclusion of ranks 1 and 8.
    ///
    /// @sa @coderef{Attacks_Portable::getPawnAttackMask()}
    /// @sa @coderef{Attacks_Portable::getPawnAttackerMask()}
    alignas(64)
    std::array<std::array<std::uint64_t, 2U>, 64U> pawnAttackMasks;

    /// @brief Knight attack masks
    ///
    /// Table of knight square to attacked square set masks.
    alignas(64)
    std::array<std::uint64_t, 64U> knightAttackMasks;

    /// @brief King attack masks
    ///
    /// Table of king square to attacked square set masks.
    alignas(64)
    std::array<std::uint64_t, 64U> kingAttackMasks;

    /// @brief Ray attack intercept squares from king to checker
    ///
    /// @sa @coderef{Intercepts::getInterceptSquares()}
    alignas(64)
    std::array<std::array<std::uint64_t, 64U>, 65U> rayIntercepts;

    /// @brief Rays from a king square to the direction of a pinned piece square
    ///
    /// @sa @coderef{Intercepts::getInterceptSquares()}
    alignas(64)
    std::array<std::array<std::uint64_t, 64U>, 64U> raysFromKing;

    /// @brief Horizontal rook attacks: column to squares on the column
    ///
    /// @sa @coderef{Attacks_Portable::getHorizRookAttackMask()}
    alignas(64)
    std::array<std::array<std::uint8_t, 8U>, 256U> rookHorizAttackMasks;

#if (BITBOARD_TABLES_HAVE_X86_BMI2)
    alignas(64) std::array<std::uint64_t, 64U> bmi2BishopMasks;
    alignas(64) std::array<std::uint32_t, 64U> bmi2BishopOffsets;
    alignas(64) std::array<std::uint64_t, 64U> bmi2RookMasks;
    alignas(64) std::array<std::uint32_t, 64U> bmi2RookOffsets;
    alignas(64) std::array<std::uint64_t, 5248U + 102400U> bmi2BishopRookAttackData;
#endif

#if (BITBOARD_TABLES_HAVE_ELEMENTARY)
    struct MasksAndMultipliers
    {
        std::uint64_t masks[3U];
        std::uint64_t shift;
        std::uint64_t multipliers[3U];
        std::uint64_t zeroPadding;
    };

    static_assert(sizeof(MasksAndMultipliers) == 64);

    alignas(64) std::array<MasksAndMultipliers, 64U> elementaryBishopMaskMults;
    alignas(64) std::array<std::uint32_t, 64U> elementaryBishopOffsets;
    alignas(64) std::array<MasksAndMultipliers, 64U> elementaryRookMaskMults;
    alignas(64) std::array<std::uint32_t, 64U> elementaryRookOffsets;
    alignas(64) std::array<std::uint64_t, 5248U + 102400U> elementaryBishopRookAttackData;
#endif

#if (BITBOARD_TABLES_HAVE_AARCH64_SVE2_BITPERM)
    alignas(64) std::array<std::uint64_t, 128U> sve2BishopRookMasks;
    alignas(64) std::array<std::uint64_t, 128U> sve2BishopRookOffsets;
    alignas(64) std::array<std::uint64_t, 5248U + 102400U> sve2BishopRookAttackData;
#endif

#if (BITBOARD_TABLES_HAVE_HYPERBOLA)
    struct HyperbolaAttackMasks
    {
        std::uint64_t sqBit;
        std::uint64_t vertMaskEx; // vertical column minus square
        std::uint64_t diagBLTREx; // diagonal minus square
        std::uint64_t diagBRTLEx; // anti-diagonal minus square
    };

    alignas(64) std::array<HyperbolaAttackMasks, 64U> hyperbolaAttackMasks;
#endif

};

/// @brief Various bitboard attack and other tables
extern const BitBoardTables ctBitBoardTables;

}

#endif
