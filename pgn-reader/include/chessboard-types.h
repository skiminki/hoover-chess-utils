// Hoover Chess Utilities / PGN reader
// Copyright (C) 2024-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TYPES_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TYPES_H_INCLUDED

#include <array>
#include <bit>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <string_view>
#include <type_traits>


namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Type definition helper
///
/// @param Type       Enumeration type name
/// @param Bits       Minimum number of bits required by the enum (8/16/32/64)
#define HOOVER_CHESS_UTILS__TYPEDEF_HELPER(Type, Bits)  \
    using Type ## UnderlyingType = std::uint_fast ## Bits ## _t; \
    using Type ## CompactType = std::uint ## Bits ## _t

HOOVER_CHESS_UTILS__TYPEDEF_HELPER(Square, 8);
HOOVER_CHESS_UTILS__TYPEDEF_HELPER(Color, 8);
HOOVER_CHESS_UTILS__TYPEDEF_HELPER(Piece, 8);
HOOVER_CHESS_UTILS__TYPEDEF_HELPER(PieceAndColor, 8);
HOOVER_CHESS_UTILS__TYPEDEF_HELPER(PositionStatus, 8);
HOOVER_CHESS_UTILS__TYPEDEF_HELPER(RowColumn, 8);

/// @brief Row/colum coordinate type
using RowColumn = RowColumnUnderlyingType;


/// @brief Named square
///
/// Squares are ordered by rank (ascending) and file (ascending). @c
/// #Square::NONE comes after all valid squares. That is:
///
/// @c Square::A1 < @c Square::B1 < ... < @c Square::H1 < @c Square::A2 < ... < @c Square::H8 < @c Square::NONE
///
/// The enumeration value of a square is the same as its index on an array
/// board.
///
/// In bit-board illustrations, the squares are represented as follows:
///
/// <table class="bitboard">
/// <tr><td>A8</td><td>B8</td><td>C8</td><td>D8</td><td>E8</td><td>F8</td><td>G8</td><td>H8</td></tr>
/// <tr><td>A7</td><td>B7</td><td>C7</td><td>D7</td><td>E7</td><td>F7</td><td>G7</td><td>H7</td></tr>
/// <tr><td>A6</td><td>B6</td><td>C6</td><td>D6</td><td>E6</td><td>F6</td><td>G6</td><td>H6</td></tr>
/// <tr><td>A5</td><td>B5</td><td>C5</td><td>D5</td><td>E5</td><td>F5</td><td>G5</td><td>H5</td></tr>
/// <tr><td>A4</td><td>B4</td><td>C4</td><td>D4</td><td>E4</td><td>F4</td><td>G4</td><td>H4</td></tr>
/// <tr><td>A3</td><td>B3</td><td>C3</td><td>D3</td><td>E3</td><td>F3</td><td>G3</td><td>H3</td></tr>
/// <tr><td>A2</td><td>B2</td><td>C2</td><td>D2</td><td>E2</td><td>F2</td><td>G2</td><td>H2</td></tr>
/// <tr><td>A1</td><td>B1</td><td>C1</td><td>D1</td><td>E1</td><td>F1</td><td>G1</td><td>H1</td></tr>
/// </table>
///
/// @sa @coderef{isValidValue(Square)}, @coderef{isValidSquare()}
/// @sa @coderef{makeSquare()}, @coderef{rowOf(Square)}, @coderef{columnOf(Square)}
/// @sa @coderef{getSquareForIndex()}, @coderef{getIndexOfSquare()}
enum class Square : SquareUnderlyingType
{
    A1 = 0U, ///< row 0, column 0
    B1 = 1U, ///< row 0, column 1
    C1 = 2U, ///< row 0, column 2
    D1 = 3U, ///< row 0, column 3
    E1 = 4U, ///< row 0, column 4
    F1 = 5U, ///< row 0, column 5
    G1 = 6U, ///< row 0, column 6
    H1 = 7U, ///< row 0, column 7
    A2 = 8U, ///< row 1, column 0
    B2 = 9U, ///< row 1, column 1
    C2 = 10U, ///< row 1, column 2
    D2 = 11U, ///< row 1, column 3
    E2 = 12U, ///< row 1, column 4
    F2 = 13U, ///< row 1, column 5
    G2 = 14U, ///< row 1, column 6
    H2 = 15U, ///< row 1, column 7
    A3 = 16U, ///< row 2, column 0
    B3 = 17U, ///< row 2, column 1
    C3 = 18U, ///< row 2, column 2
    D3 = 19U, ///< row 2, column 3
    E3 = 20U, ///< row 2, column 4
    F3 = 21U, ///< row 2, column 5
    G3 = 22U, ///< row 2, column 6
    H3 = 23U, ///< row 2, column 7
    A4 = 24U, ///< row 3, column 0
    B4 = 25U, ///< row 3, column 1
    C4 = 26U, ///< row 3, column 2
    D4 = 27U, ///< row 3, column 3
    E4 = 28U, ///< row 3, column 4
    F4 = 29U, ///< row 3, column 5
    G4 = 30U, ///< row 3, column 6
    H4 = 31U, ///< row 3, column 7
    A5 = 32U, ///< row 4, column 0
    B5 = 33U, ///< row 4, column 1
    C5 = 34U, ///< row 4, column 2
    D5 = 35U, ///< row 4, column 3
    E5 = 36U, ///< row 4, column 4
    F5 = 37U, ///< row 4, column 5
    G5 = 38U, ///< row 4, column 6
    H5 = 39U, ///< row 4, column 7
    A6 = 40U, ///< row 5, column 0
    B6 = 41U, ///< row 5, column 1
    C6 = 42U, ///< row 5, column 2
    D6 = 43U, ///< row 5, column 3
    E6 = 44U, ///< row 5, column 4
    F6 = 45U, ///< row 5, column 5
    G6 = 46U, ///< row 5, column 6
    H6 = 47U, ///< row 5, column 7
    A7 = 48U, ///< row 6, column 0
    B7 = 49U, ///< row 6, column 1
    C7 = 50U, ///< row 6, column 2
    D7 = 51U, ///< row 6, column 3
    E7 = 52U, ///< row 6, column 4
    F7 = 53U, ///< row 6, column 5
    G7 = 54U, ///< row 6, column 6
    H7 = 55U, ///< row 6, column 7
    A8 = 56U, ///< row 7, column 0
    B8 = 57U, ///< row 7, column 1
    C8 = 58U, ///< row 7, column 2
    D8 = 59U, ///< row 7, column 3
    E8 = 60U, ///< row 7, column 4
    F8 = 61U, ///< row 7, column 5
    G8 = 62U, ///< row 7, column 6
    H8 = 63U, ///< row 7, column 7

    /// @brief Token for 'no' square
    NONE = 64U,
};

/// @brief Color of a piece or side to move
enum class Color : ColorUnderlyingType
{
    /// @brief White piece or white side to move
    WHITE = 0U,

    /// @brief Black piece or black side to move
    BLACK = 8U,
};

/// @brief Named piece
enum class Piece : PieceUnderlyingType
{
    NONE = 0U, ///< Value representing no piece
    PAWN,      ///< Pawn
    KNIGHT,    ///< Knight
    BISHOP,    ///< Bishop
    ROOK,      ///< Rook
    QUEEN,     ///< Queen
    KING,      ///< King
};

/// @brief Named piece and color
///
/// @remark Values are organized such that extracting @coderef{Piece} and @coderef{Color} is a matter
/// of bitwise and.
enum class PieceAndColor : PieceAndColorUnderlyingType
{
    NONE = 0U,       ///< Value representing no piece and color. This is synonym to @coderef{WHITE_NONE}.

    WHITE_PAWN = 1U, ///< White pawn
    WHITE_KNIGHT,    ///< White knight
    WHITE_BISHOP,    ///< White bishop
    WHITE_ROOK,      ///< White rook
    WHITE_QUEEN,     ///< White queen
    WHITE_KING,      ///< White king

    BLACK_PAWN = 9U, ///< Black pawn
    BLACK_KNIGHT,    ///< Black knight
    BLACK_BISHOP,    ///< Black bishop
    BLACK_ROOK,      ///< Black rook
    BLACK_QUEEN,     ///< Black queen
    BLACK_KING,      ///< Black king

    WHITE_NONE = 0U, ///< Special value to make <tt>@coderef{makePieceAndColor}</tt><tt>(@coderef{Piece::NONE}, @coderef{Color::WHITE})</tt> well-defined
    BLACK_NONE = 8U, ///< Special value to make <tt>@coderef{makePieceAndColor}</tt><tt>(@coderef{Piece::NONE}, @coderef{Color::BLACK})</tt> well-defined
};

/// @brief Named piece and color (compact representation)
///
/// The compact representation is generally better suited to store in memory
/// than the fast type.
///
/// Use @coderef{toCompactType()} and @coderef{toFastType()} to switch between
/// representations.
enum class PieceAndColorCompact : PieceAndColorCompactType
{
    NONE = 0U,       ///< Value representing no piece and color. This is synonym to @coderef{WHITE_NONE}.

    WHITE_PAWN = 1U, ///< White pawn
    WHITE_KNIGHT,    ///< White knight
    WHITE_BISHOP,    ///< White bishop
    WHITE_ROOK,      ///< White rook
    WHITE_QUEEN,     ///< White queen
    WHITE_KING,      ///< White king

    BLACK_PAWN = 9U, ///< Black pawn
    BLACK_KNIGHT,    ///< Black knight
    BLACK_BISHOP,    ///< Black bishop
    BLACK_ROOK,      ///< Black rook
    BLACK_QUEEN,     ///< Black queen
    BLACK_KING,      ///< Black king

    WHITE_NONE = 0U, ///< Special value to make <tt>@coderef{makePieceAndColor}</tt><tt>(@coderef{Piece::NONE}, @coderef{Color::WHITE})</tt> well-defined
    BLACK_NONE = 8U, ///< Special value to make <tt>@coderef{makePieceAndColor}</tt><tt>(@coderef{Piece::NONE}, @coderef{Color::BLACK})</tt> well-defined
};

/// @brief Status of a position
///
/// @remark Note that with correct API usage, a @coderef{ChessBoard} can never hold an
/// illegal position. Hence, there is no enumeration for an illegal position.
///
/// @sa @coderef{ChessBoard::determineStatus()}
enum class PositionStatus : PositionStatusUnderlyingType
{
    /// @brief Regular position (not in check, mate, or stalemate)
    NORMAL,

    /// @brief King is checked (but not mated)
    CHECK,

    /// @brief Stalemate
    STALEMATE,

    /// @brief Mate
    MATE
};

/// @brief Checks whether a value is a valid enumeration value.
///
/// @param[in]  sq     Square
/// @return            Whether @c sq is a valid enumeration value. That is, a
///                    square or @coderef{Square::NONE}.
constexpr inline bool isValidValue(Square sq) noexcept
{
    return (sq <= Square::NONE);
}

/// @brief Checks whether a value is a square
///
/// @param[in]  sq     Square
/// @return            Whether @c sq is a named square
///
/// @remark Value @coderef{Square::NONE} is not a square.
constexpr inline bool isValidSquare(Square sq) noexcept
{
    return (sq <= Square::H8);
}

/// @brief Constructs a square from column and row
///
/// @param[in] col     Column number (0 for A-file). Range: [0, 7]
/// @param[in] row     Row number (0 for 1st rank). Range: [0, 7]
/// @return            Constructed @coderef{Square}
constexpr inline Square makeSquare(RowColumn col, RowColumn row) noexcept
{
    assert(col <= 7U);
    assert(row <= 7U);

    [[assume(row <= 7U)]];
    [[assume(col <= 7U)]];

    return Square((row * 8U) + col);
}

/// @brief Returns column number of square
///
/// @param[in] sq      Square. Range: [Square::A1, Square::H8]
/// @return            Column number (0 for A-file)
constexpr inline RowColumn columnOf(Square sq) noexcept
{
    assert(sq <= Square::H8);
    [[assume(sq <= Square::H8)]];

    return static_cast<SquareUnderlyingType>(sq) & 7U;
}

/// @brief Returns row number of square
///
/// @param[in] sq      Square. Range: [Square::A1, Square::H8]
/// @return            Row number (0 for 1st rank)
constexpr inline RowColumn rowOf(Square sq) noexcept
{
    assert(sq <= Square::H8);
    [[assume(sq <= Square::H8)]];

    return static_cast<SquareUnderlyingType>(sq) / 8U;
}

/// @brief Returns a square for an index. In essence, this is the ordinal of the
/// square.
///
/// @param[in] index      Index on array board. Range: [0, 63]
/// @return               Corresponding @coderef{Square}
///
/// @sa @coderef{ArrayBoard}
constexpr inline Square getSquareForIndex(std::size_t index) noexcept
{
    assert(index <= 63U);
    [[assume(index <= 63U)]];

    return Square(index);
}

/// @brief Returns an index for a square
///
/// @param[in]  sq        Square. Range: [Square::A1, Square::H8]
/// @return               Corresponding index on an array board
///
/// @sa @coderef{ArrayBoard}
constexpr inline std::size_t getIndexOfSquare(Square sq) noexcept
{
    assert(sq <= Square::H8);
    [[assume(sq <= Square::H8)]];

    return static_cast<std::size_t>(sq);
}

/// @brief Adds to square. This function performs no overflow checking.
///
/// @param[in]  sq       Square
/// @param[in]  shift    Amount to add (or subtract for negative shifts)
/// @return              Square value added by @c shift
///
/// This function is equivalent to:
/// @code
/// Square newSq = getSquareForIndex(getIndexOfSquare(sq) + shift)
/// @endcode
constexpr inline Square addToSquareNoOverflowCheck(Square sq, std::int_fast8_t shift) noexcept
{
    return Square(static_cast<SquareUnderlyingType>(sq) + shift);
}

/// @brief Checks whether a value is a valid enumeration value.
///
/// @param[in]  c    Color enumeration value
/// @return          Whether @c is a valid value. That is, either
///                  @coderef{Color::WHITE} or @coderef{Color::BLACK}.
constexpr inline bool isValidValue(Color c) noexcept
{
    return c == Color::WHITE || c == Color::BLACK;
}

/// @brief Flips the color
///
/// @param[in]  c    Color enumeration value
/// @return          Opposite color
constexpr inline Color oppositeColor(Color c) noexcept
{
    return Color(static_cast<ColorUnderlyingType>(c) ^ 0x08U);
}

/// @brief Returns color of a piece
///
/// @param[in]  pc   Piece and color enumeration value
/// @return          Color of @c pc
constexpr inline Color colorOf(PieceAndColor pc) noexcept
{
    assert((pc >= PieceAndColor::WHITE_NONE && pc <= PieceAndColor::WHITE_KING) ||
           (pc >= PieceAndColor::BLACK_NONE && pc <= PieceAndColor::BLACK_KING));

    return Color(static_cast<PieceAndColorUnderlyingType>(pc) & 0x8U);
}


/// @brief Checks whether a value is a valid enumeration value.
///
/// @param[in]  p    Piece enumeration value
/// @return          Whether @c p is a valid value.
constexpr inline bool isValidValue(Piece p) noexcept
{
    return p <= Piece::KING;
}

/// @brief Checks whether a value is a valid enumeration value.
///
/// @param[in]  pc   Piece and color enumeration value
/// @return          Whether @c pc is a valid value.
constexpr inline bool isValidValue(PieceAndColor pc) noexcept
{
    return (pc <= PieceAndColor::WHITE_KING) || (pc >= PieceAndColor::BLACK_NONE && pc <= PieceAndColor::BLACK_KING);
}

/// @brief Returns piece of a piece and color enumeration value.
///
/// @param[in]  pc   Piece and color enumeration value
/// @return          Piece of @c pc
constexpr inline Piece pieceOf(PieceAndColor pc) noexcept
{
    assert(isValidValue(pc));
    [[assume(pc <= PieceAndColor::BLACK_KING)]];

    return Piece(static_cast<PieceAndColorUnderlyingType>(pc) & 0x7U);
}

/// @brief Constructs a PieceAndColor enumeration value from Piece and Color
///
/// @param[in] p     Valid piece enumeration value. May be @coderef{Piece::NONE}.
/// @param[in] c     Valid color enumeration value.
/// @return          Constructed @coderef{PieceAndColor}.
constexpr inline PieceAndColor makePieceAndColor(Piece p, Color c) noexcept
{
    assert(isValidValue(p));
    assert(isValidValue(c));

    return PieceAndColor(
        static_cast<PieceUnderlyingType>(p) |
        static_cast<ColorUnderlyingType>(c));
}


constexpr inline PieceAndColorCompact toCompactType(PieceAndColor pc) noexcept
{
    return PieceAndColorCompact(static_cast<PieceAndColorUnderlyingType>(pc));
}

constexpr inline PieceAndColor toFastType(PieceAndColorCompact pc) noexcept
{
    return PieceAndColor(static_cast<PieceAndColorCompactType>(pc));
}


/// @}

}

#endif
