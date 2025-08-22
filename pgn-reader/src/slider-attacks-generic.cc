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

#include "slider-attacks.h"

#include "bitrays.h"
#include "chessboard-types.h"
#include "pgnreader-config.h"

#include <cinttypes>


namespace hoover_chess_utils::pgn_reader
{

// get mask of rook attacks/moves on populated board
SquareSet SliderAttacksImpl<SliderAttacksImplType::Generic>::getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    return shootRaysCrossReturnTraces(sq, occupancyMask);

}

// get mask of rook attacks/moves on populated board
SquareSet SliderAttacksImpl<SliderAttacksImplType::Generic>::getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    return shootRaysPlusReturnTraces(sq, occupancyMask);
}

}
