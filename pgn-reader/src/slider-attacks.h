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

#ifndef CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_H_INCLUDED
#define CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

namespace hoover_chess_utils::pgn_reader
{

/// @brief Slider attacks implementation type
enum class SliderAttacksImplType
{
    /// @brief Portable implementation
    Generic,

    /// @brief Implementation using PEXT/PDEP instructions
    PextPdep,
};

/// @brief Compile-time implementation dispatch for slider attacks
///
/// @tparam impl    Implementation type
template <SliderAttacksImplType impl>
class SliderAttacksImpl;

template <>
class SliderAttacksImpl<SliderAttacksImplType::Generic>
{
public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;
};

template <>
class SliderAttacksImpl<SliderAttacksImplType::PextPdep>
{
public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;
};

#if HAVE_PDEP_PEXT

using SliderAttacks = SliderAttacksImpl<SliderAttacksImplType::PextPdep>;

#else

using SliderAttacks = SliderAttacksImpl<SliderAttacksImplType::Generic>;

#endif

}

#endif
