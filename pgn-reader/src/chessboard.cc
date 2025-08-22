// Hoover Chess Utilities / PGN reader
// Copyright (C) 2022-2025  Sami Kiminki
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

#include "chessboard.h"

#include <algorithm>
#include <array>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{

bool ChessBoard::operator == (const ChessBoard &o) const noexcept
{
    if ((m_turnColorMask) != (o.m_turnColorMask))
        return false;

    if (m_pawns != o.m_pawns)
        return false;

    if (m_knights != o.m_knights)
        return false;

    if (m_bishops != o.m_bishops)
        return false;

    if (m_rooks != o.m_rooks)
        return false;

    if (getWhiteShortCastleRook() != o.getWhiteShortCastleRook())
        return false;

    if (getWhiteLongCastleRook() != o.getWhiteLongCastleRook())
        return false;

    if (getBlackShortCastleRook() != o.getBlackShortCastleRook())
        return false;

    if (getBlackLongCastleRook() != o.getBlackLongCastleRook())
        return false;

    if (getEpSquare() != o.getEpSquare())
    {
        Square ep1 { canEpCapture() ? getEpSquare() : Square::NONE };
        Square ep2 { o.canEpCapture() ? o.getEpSquare() : Square::NONE };

        if (ep1 != ep2)
            return false;
    }

    if (getCurrentPlyNum() != o.getCurrentPlyNum())
        return false;

    if (getHalfMoveClock() != o.getHalfMoveClock())
        return false;

    return true;
}

}
