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

/// @addtogroup PgnReaderImpl
/// @{

/// @brief PGN scanner token
///
/// @sa @coderef{PgnScanner::nextToken()}
enum PgnScannerToken : std::uint8_t
{
    /// @brief Null token (placeholder only)
    NONE = 0U,

    /// @brief End of file
    END_OF_FILE,

    /// @brief PGN tag start ('[')
    TAG_START,

    /// @brief PGN tag key
    TAG_KEY,

    /// @brief PGN tag value
    TAG_VALUE,

    /// @brief PGN tag end (']')
    TAG_END,

    /// @brief Variation start ('(')
    VARIATION_START,

    /// @brief Variation end ('(')
    VARIATION_END,

    /// @brief Move number
    ///
    /// @sa @coderef{PgnScannerTokenInfo_MOVENUM}
    MOVENUM,

    /// @brief Pawn advance (non-promoting)
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PAWN_MOVE}
    MOVE_PAWN,

    /// @brief Pawn capture (non-promoting)
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PAWN_MOVE}
    MOVE_PAWN_CAPTURE,

    /// @brief Pawn capture (promoting)
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PAWN_MOVE}
    MOVE_PAWN_PROMO,

    /// @brief Pawn capture (promoting)
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PAWN_MOVE}
    MOVE_PAWN_PROMO_CAPTURE,

    /// @brief Knight move
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PIECE_MOVE}
    MOVE_PIECE_KNIGHT,

    /// @brief Bishop move
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PIECE_MOVE}
    MOVE_PIECE_BISHOP,

    /// @brief Rook move
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PIECE_MOVE}
    MOVE_PIECE_ROOK,

    /// @brief Queen move
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PIECE_MOVE}
    MOVE_PIECE_QUEEN,

    /// @brief King move
    ///
    /// @sa @coderef{PgnScannerTokenInfo_PIECE_MOVE}
    MOVE_PIECE_KING,

    /// @brief Short castling move
    MOVE_SHORT_CASTLE,

    /// @brief Long castling move
    MOVE_LONG_CASTLE,

    /// @brief Numeric annotation glyph
    ///
    /// @sa @coderef{PgnScannerTokenInfo_NAG}
    NAG,

    /// @brief Block comment start
    COMMENT_START,

    /// @brief Block comment text line OR single line comment
    COMMENT_TEXT,

    /// @brief New line within a block comment
    COMMENT_NEWLINE,

    /// @brief Block comment end
    COMMENT_END,

    /// @brief PGN game result (terminator)
    ///
    /// @sa @coderef{PgnScannerTokenInfo_RESULT}
    RESULT,

    /// @brief Tokenizer error
    ///
    /// The error token spans at least a single character. Hence, error recovery may be attempted by reading
    /// error tokens until a regular tokens is encountered.
    ///
    /// @sa @coderef{PgnScannerTokenInfo_ERROR}
    ERROR,
};

constexpr inline std::uint32_t pgnScannerTokenToMaskBit(PgnScannerToken token) noexcept
{
    return std::uint32_t { 1U << static_cast<unsigned>(token) };
};

/// @brief Additional token info for pawn move
struct PgnScannerTokenInfo_PAWN_MOVE
{
    /// @brief Allowed source squares per the move specification.
    ///
    /// - When source rank and file are specified, this is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, this is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, this is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, this is all squares of the board (@coderef{SquareSet::all()}).
    SquareSet srcMask;

    /// @brief Destination square of the move
    Square dstSq;

    /// @brief Promotion piece for @coderef{PgnScannerToken::MOVE_PAWN_PROMO} and @coderef{PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE}.
    Piece promoPiece;
};

/// @brief Additional token info for piece move
struct PgnScannerTokenInfo_PIECE_MOVE
{
    /// @brief Allowed source squares per the move specification.
    ///
    /// - When source rank and file are specified, this is a single square (@coderef{SquareSet::square()}).
    /// - When source rank is specified, this is all squares of the rank (@coderef{SquareSet::row()}).
    /// - When source file is specified, this is all squares of the file (@coderef{SquareSet::column()}).
    /// - Otherwise, this is all squares of the board (@coderef{SquareSet::all()}).
    SquareSet srcMask;

    /// @brief Whether move is a capture move
    bool capture;

    /// @brief Destination square of the move
    Square dstSq;
};

/// @brief Additional token info for move number token
struct PgnScannerTokenInfo_MOVENUM
{
    /// @brief Move number
    std::uint32_t num;
};

/// @brief Additional token info for numeric annotation glyph
struct PgnScannerTokenInfo_NAG
{
    /// @brief Glyph number
    ///
    /// @sa https://www.saremba.de/chessgml/standards/pgn/pgn-complete.htm#c10
    std::uint8_t nag;
};

/// @brief Additional token info for game result
struct PgnScannerTokenInfo_RESULT
{
    /// @brief Game result
    PgnResult result;
};

/// @brief Additional token info for error token
struct PgnScannerTokenInfo_ERROR
{
    /// @brief Error message provided by the scanner
    const char *errorMessage;
};

/// @brief Union of all additional scanner information. The applicable union
/// member depends the PGN scanner token (see @coderef{PgnScannerToken}).
///
/// @sa @coderef{PgnScanner::getTokenInfo()}
union PgnScannerTokenInfo
{
    PgnScannerTokenInfo_PAWN_MOVE pawnMove;
    PgnScannerTokenInfo_PIECE_MOVE pieceMove;
    PgnScannerTokenInfo_MOVENUM moveNum;
    PgnScannerTokenInfo_NAG nag;
    PgnScannerTokenInfo_RESULT result;
    PgnScannerTokenInfo_ERROR error;
};

/// @}

}

#endif
