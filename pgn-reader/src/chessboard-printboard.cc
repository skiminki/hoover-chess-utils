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
#include "pgnreader-string-utils.h"

#include <format>
#include <iostream>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{

namespace
{
constexpr std::string_view ctStringView_NONE { "  " };
}

void ChessBoard::printBoard() const
{
    for (std::uint8_t row = 7U; row < 8U; --row)
    {
        std::cout << '|';
        for (std::uint8_t col = 0U; col < 8U; ++col)
        {
            Square sq { makeSquare(col, row) };
            PieceAndColor pc { getSquarePiece(sq) };

            std::cout << StringUtils::pieceAndColorToString(pc) << '|';
        }
        std::cout << std::endl;
    }

    std::cout
        << std::format("Castling W:{} {}  B:{} {}  EP:{}  half-move={:03}  move={}{}",
                       StringUtils::squareToString(getWhiteLongCastleRook(), ctStringView_NONE),
                       StringUtils::squareToString(getWhiteShortCastleRook(), ctStringView_NONE),
                       StringUtils::squareToString(getBlackLongCastleRook(), ctStringView_NONE),
                       StringUtils::squareToString(getBlackShortCastleRook(), ctStringView_NONE),
                       StringUtils::squareToString(canEpCapture() ? getEpSquare() : Square::NONE, ctStringView_NONE),
                       getHalfMoveClock(),
                       moveNumOfPly(getCurrentPlyNum()),
                       getTurn() == Color::WHITE ? 'w' : 'b')
        << std::endl;

    std::cout  << "Checkers:";

    const SquareSet checkers { getCheckers() };
    if (checkers == SquareSet::none())
        std::cout << " (none)";
    else
    {
        SQUARESET_ENUMERATE(
            checker,
            checkers,
            std::cout << ' ' << StringUtils::squareToString(checker, ctStringView_NONE));
    }

    std::cout << std::endl;
}

}
