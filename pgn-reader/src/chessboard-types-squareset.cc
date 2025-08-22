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

#include "chessboard-types-squareset.h"
#include "bittricks.h"


namespace hoover_chess_utils::pgn_reader
{

SquareSet SquareSet::parallelExtract(SquareSet extractMask) noexcept
{
    return SquareSet { BitTricks::parallelExtract(m_bitmask, static_cast<std::uint64_t>(extractMask)) };
}

SquareSet SquareSet::parallelDeposit(SquareSet extractMask) noexcept
{
    return SquareSet { BitTricks::parallelDeposit(m_bitmask, static_cast<std::uint64_t>(extractMask)) };
}

}
