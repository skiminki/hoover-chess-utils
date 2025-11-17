// Hoover Chess Utilities / PGN reader
// Copyright (C) 2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITTRICKS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITTRICKS_H_INCLUDED

#include "pgnreader-config.h"

#include <bit>
#include <cassert>
#include <cinttypes>

#if (HAVE_BMI2)
#include <immintrin.h>
#endif


namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Collection of bit tricks for 64-bit words
struct BitTricks
{

    /// @brief Generates a bit mask with bits @c 0 to @c N set
    ///
    /// @param[in]  n   The highest bit to set. Range: [0, 63]
    /// @return         Bit mask with bits from @c 0 to @c N set
    static constexpr inline std::uint64_t bits0ToN(std::uint8_t n) noexcept
    {
        assert(n <= 63U);

        return (std::uint64_t { 2 } << n) - 1U;
    }

    /// @brief Generates a half-open bit mask (min(a,b), max(a,b)]
    ///
    /// @param[in]  a   Boundary of the bit mask. Range: [0, 63]
    /// @param[in]  b   Boundary of the bit mask. Range: [0, 63]
    /// @return         Half-open bit mask (min(a,b), max(a,b)]
    ///
    /// This function is intended to generate a half-open bit mask when it does
    /// not matter whether the boundaries are included. That is, the caller sets or
    /// resets the boundaries accordingly.
    ///
    /// Illustration:
    ///
    ///                       a       b
    ///            ----------------------------------------------------------------
    ///     ret =  0000000000011111111000000000000000000000000000000000000000000000
    static constexpr inline std::uint64_t rangeHalfOpen(std::uint8_t a, std::uint8_t b) noexcept
    {
        assert(a <= 63U);
        assert(b <= 63U);

        return bits0ToN(a) ^ bits0ToN(b);
    }

    /// @brief Isolates (extracts) the lowest bit set in bit mask.
    ///
    /// @param[in]  mask   Bit mask
    /// @return            Bit mask with lowest bit of @c mask set OR @c 0 if @c mask is @c 0.
    static constexpr inline std::uint64_t isolateLowestSetBit(std::uint64_t mask) noexcept
    {
        return mask & (-mask);
    }

    /// @brief Isolates (extracts) the highest bit set in bit mask.
    ///
    /// @param[in]  mask   Bit mask
    /// @return            Bit mask with highest bit of @c mask set OR @c 0 if @c mask is @c 0.
    static constexpr inline std::uint64_t isolateHighestSetBit(std::uint64_t mask) noexcept
    {
        // Note: we need to 'and' to ensure that the isolated bit actually exists (when n == 0)
        if (mask != 0U) [[likely]]
        {
            std::uint64_t topBit { UINT64_C(0x80'00'00'00'00'00'00'00) };
            return (topBit >> std::countl_zero(mask));
        }
        else [[unlikely]]
        {
            return 0U;
        }
    }

    /// @brief Extracts bits of @c data from bit locations specified by @c mask
    ///
    /// @param[in]  data   Data word
    /// @param[in]  mask   Mask
    /// @return            Extracted bits from data
    ///
    /// The following implementations are provided:
    /// <table>
    /// <tr>
    ///   <th>Build condition</th>
    ///   <th>Description</th>
    /// </tr>
    /// <tr>
    ///   <td>@ref HAVE_BMI2</td>
    ///   <td>Fast implementation using x86 PEXT instruction</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Generic portable implementation</td>
    /// </tr>
    /// </table>
    ///
    /// @sa https://www.chessprogramming.org/BMI2#PEXT
    static inline std::uint64_t parallelExtract(std::uint64_t data, std::uint64_t mask) noexcept
    {
#if (HAVE_BMI2)
        return _pext_u64(data, mask);
#else
        std::uint64_t ret { };
        std::uint64_t count { 1U };

        while (mask)
        {
            const std::uint64_t lowestSetBit { BitTricks::isolateLowestSetBit(mask) };
            ret |= (data & lowestSetBit) ? count : 0U;
            mask &= ~lowestSetBit;
            count *= 2U;
        }

        return ret;
#endif
    }

    /// @brief Deposits bits of @c data to bit locations specified by @c mask
    ///
    /// @param[in]  data   Data word
    /// @param[in]  mask   Mask
    /// @return            Deposited bits
    ///
    /// The following implementations are provided:
    /// <table>
    /// <tr>
    ///   <th>Build condition</th>
    ///   <th>Description</th>
    /// </tr>
    /// <tr>
    ///   <td>@ref HAVE_BMI2</td>
    ///   <td>Fast implementation using x86 PDEP instruction</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Generic portable implementation</td>
    /// </tr>
    /// </table>
    ///
    /// @sa https://www.chessprogramming.org/BMI2#PDEP
    static inline std::uint64_t parallelDeposit(std::uint64_t data, std::uint64_t mask) noexcept
    {
#if (HAVE_BMI2)
        return _pdep_u64(data, mask);
#else
        std::uint64_t ret { };

        while (mask)
        {
            std::uint64_t maskBit { BitTricks::isolateLowestSetBit(static_cast<std::uint64_t>(mask)) };

            ret |= maskBit * (data & 1U);
            data >>= 1U;

            mask ^= maskBit;
        }

        return ret;
#endif
    }

};

}

#endif
