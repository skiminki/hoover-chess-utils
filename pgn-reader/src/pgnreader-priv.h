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

#ifndef CHESS_UTILS__PGN_READER__PGNREADER_PRIV_H_INCLUDED
#define CHESS_UTILS__PGN_READER__PGNREADER_PRIV_H_INCLUDED

#include "pgnreader.h"

#include <cstdint>

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderAPI
/// @{

/// @brief PGN reader action filter (compile-time)
///
/// @tparam  enabled  Enabled actions
template <PgnReaderActionClass... args>
class PgnReaderActionCompileTimeFilter
{
public:
    static constexpr std::uint32_t actionBit(PgnReaderActionClass action) noexcept
    {
        const unsigned int shift { static_cast<unsigned int>(action) };
        if (shift < 32U) [[likely]]
        {
            return UINT32_C(1) << shift;
        }
        else [[unlikely]]
        {
            return 0U;
        }
    }

    static constexpr std::uint32_t actionsToBitmask() noexcept
    {
        return 0U;
    }

    template <typename... Args>
    static constexpr std::uint32_t actionsToBitmask(PgnReaderActionClass action, Args... rest) noexcept
    {
        return actionBit(action) | actionsToBitmask(rest...);
    }

private:
    static constexpr std::uint32_t s_filterBits { actionsToBitmask(args...) };

public:
    /// @brief Returns whether an action class is enabled
    ///
    /// @param[in]  action    Action class
    /// return                Status
    static constexpr inline bool isEnabled(PgnReaderActionClass action) noexcept
    {
        return (s_filterBits & actionBit(action)) != 0U;
    }

    /// @brief Returns a bit mask of enabled action classes. When bit N is set,
    /// action class with numeric value @c N is enabled.
    ///
    /// return                Bit mask
    static constexpr inline std::uint32_t getBitMask() noexcept
    {
        return s_filterBits;
    }
};

/// @}

}

#endif
