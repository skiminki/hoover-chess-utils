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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNSCANNERTOKENS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNSCANNERTOKENS_H_INCLUDED

#include "pgnreader-types.h"

#include "chessboard-types-squareset.h"

#include <cinttypes>
#include <variant>

namespace hoover_chess_utils::pgn_reader
{

enum PgnScannerToken : std::uint8_t
{
    NONE = 0U,

    END_OF_FILE,
    TAG_START,
    TAG_KEY,
    TAG_VALUE,
    TAG_END,
    VARIATION_START,
    VARIATION_END,
    MOVENUM,

    // various moves
    MOVE_PAWN,
    MOVE_PAWN_CAPTURE,
    MOVE_PAWN_PROMO,
    MOVE_PAWN_PROMO_CAPTURE,
    MOVE_PIECE,
    MOVE_SHORT_CASTLE,
    MOVE_LONG_CASTLE,

    NAG,

    COMMENT_START,
    COMMENT_TEXT,
    COMMENT_NEWLINE,
    COMMENT_END,

    RESULT,

    ERROR,
};

struct PgnScannerTokenInfo_PAWN_MOVE
{
    SquareSet srcMask;
    bool capture;
    Square dstSq;
    Piece promoPiece;
};

struct PgnScannerTokenInfo_PIECE_MOVE
{
    SquareSet srcMask;
    Piece piece;
    bool capture;
    Square dstSq;
};

struct PgnScannerTokenInfo_MOVENUM
{
    std::uint32_t num;
    Color color;
};

struct PgnScannerTokenInfo_NAG
{
    std::uint8_t nag;
};

struct PgnScannerTokenInfo_RESULT
{
    PgnResult result;
};

struct PgnScannerTokenInfo_ERROR
{
    const char *errorMessage;
};

union PgnScannerTokenInfo
{
    PgnScannerTokenInfo_PAWN_MOVE pawnMove;
    PgnScannerTokenInfo_PIECE_MOVE pieceMove;
    PgnScannerTokenInfo_MOVENUM moveNum;
    PgnScannerTokenInfo_NAG nag;
    PgnScannerTokenInfo_RESULT result;
    PgnScannerTokenInfo_ERROR error;
};

}

#endif
