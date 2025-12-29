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
    struct PextData
    {
        alignas(64) std::array<std::uint64_t, 128U> bishopRookMasks;
        alignas(64) std::array<std::uint64_t, 128U> bishopRookOffsets;
        alignas(64) std::array<std::uint64_t, 5248U + 102400U> bishopRookAttackData;
    };

    static const PextData ctPextData;

public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctPextData.bishopRookMasks[2U * static_cast<std::uint8_t>(sq)] };
        const std::uint64_t offset { ctPextData.bishopRookOffsets[2U * static_cast<std::uint8_t>(sq)] };

        svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };

        svuint64_t extractedV { svbext_n_u64(occupancyMaskV, static_cast<std::uint64_t>(pextMask)) };
        const std::uint64_t extracted { extractedV[0U] };

        return SquareSet { ctPextData.bishopRookAttackData[offset + extracted] };
    }

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        const std::uint64_t pextMask { ctPextData.bishopRookMasks[2U * static_cast<std::uint8_t>(sq) + 1U] };
        const std::uint64_t offset { ctPextData.bishopRookOffsets[2U * static_cast<std::uint8_t>(sq) + 1U] };

        svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };

        svuint64_t extractedV { svbext_n_u64(occupancyMaskV, static_cast<std::uint64_t>(pextMask)) };
        const std::uint64_t extracted { extractedV[0U] };

        return SquareSet { ctPextData.bishopRookAttackData[offset + extracted] };
    }

    /// @brief See @coderef{Attacks::getQueenAttackMask()} for documentation
    static inline SquareSet getQueenAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        // note: the vector elements are in order: bishop, rook

        // we'll use the first two elements of the vector when it matters
        const svbool_t firstTwo { svptrue_pat_b64(SV_VL2) };

        const svuint64_t pextMaskV { svld1_u64(firstTwo, &ctPextData.bishopRookMasks[2U * static_cast<std::uint8_t>(sq)]) };
        const svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };

        const svuint64_t extractedV { svbext_u64(occupancyMaskV, pextMaskV) };

        const svuint64_t baseOffsetV { svld1_u64(firstTwo, &ctPextData.bishopRookOffsets[2U * static_cast<std::uint8_t>(sq)]) };
        const svuint64_t retV { svld1_gather_u64index_u64(firstTwo, ctPextData.bishopRookAttackData.data(), baseOffsetV + extractedV) };

        return SquareSet { svorv_u64(firstTwo, retV) };
    }

    static void determineSliderCheckersAndPinners(
        Square kingSq,
        SquareSet occupancyMask,
        SquareSet rooks,
        SquareSet bishops,
        SquareSet opponentPieces,
        SquareSet epCapturable,
        SquareSet &inout_checkers,
        SquareSet &out_pinners) noexcept
    {
        const svbool_t firstTwo { svptrue_pat_b64(SV_VL2) };

        const svuint64_t pextMaskV { svld1_u64(firstTwo, &ctPextData.bishopRookMasks[2U * static_cast<std::uint8_t>(kingSq)]) };
        const svuint64_t occupancyMaskV { svdup_u64(static_cast<std::uint64_t>(occupancyMask)) };
        const svuint64_t opponentPiecesV { svdup_u64(static_cast<std::uint64_t>(opponentPieces)) };
        const svuint64_t bishopsRooksV { svdupq_u64(static_cast<std::uint64_t>(bishops), static_cast<std::uint64_t>(rooks)) };
        const svuint64_t baseOffsetV { svld1_u64(firstTwo, &ctPextData.bishopRookOffsets[2U * static_cast<std::uint8_t>(kingSq)]) };

        // Resolve checkers
        svuint64_t extractedV { svbext_u64(occupancyMaskV, pextMaskV) };
        const svuint64_t firstHitsV { svld1_gather_u64index_u64(firstTwo, ctPextData.bishopRookAttackData.data(), baseOffsetV + extractedV) };
        const svuint64_t checkersV { firstHitsV & bishopsRooksV };

        inout_checkers |= SquareSet { svorv_u64(firstTwo, checkersV & opponentPiecesV) };

        // Resolve pinned pieces. Notes:
        // - We'll remove only pinnable pieces from the first hits. The idea is to avoid
        //   x-rays over non-pinnable pieces in order to minimize the number of pinners
        // - In the second hit check, we'll remove pieces that are already determined to be checkers.
        //   The reason is the same as the above.
        extractedV = svbext_u64(((occupancyMaskV &~ firstHitsV) | opponentPiecesV) &~ svdupq_u64(static_cast<std::uint64_t>(epCapturable), 0U), pextMaskV);
        const svuint64_t secondHitsV { svld1_gather_u64index_u64(firstTwo, ctPextData.bishopRookAttackData.data(), baseOffsetV + extractedV) };
        const svuint64_t pinnersV { secondHitsV & bishopsRooksV & opponentPiecesV &~ checkersV };

        out_pinners = SquareSet { svorv_u64(firstTwo, pinnersV & opponentPiecesV) };
    }
};

}

#endif
