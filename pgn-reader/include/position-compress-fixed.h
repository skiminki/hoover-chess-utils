// Hoover Chess Utilities / PGN reader
// Copyright (C) 2021-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__POSITION_COMPRESS_FIXED_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__POSITION_COMPRESS_FIXED_H_INCLUDED

#include <array>
#include <bit>
#include <cstdint>


namespace hoover_chess_utils::pgn_reader
{

class ChessBoard;

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Compressed piece encoding for fixed-length compressed position
///
/// @sa @coderef{CompressedPosition_FixedLength}
/// @sa @coderef{PositionCompressor_FixedLength}
enum class CompressedPosition_PieceEncoding
{
    /// @brief White king, white to move
    WHITE_KING_NOT_IN_TURN     = 0U,

    /// @brief White bishop
    WHITE_BISHOP               = 1U,

    /// @brief White rook (no castling rights)
    WHITE_ROOK_CANNOT_CASTLE   = 2U,

    /// @brief White queen
    WHITE_QUEEN                = 3U, // note: BISHOP | ROOK_CANNOT_CASTLE

    /// @brief White knight
    WHITE_KNIGHT               = 4U,

    /// @brief White pawn
    WHITE_PAWN                 = 5U,

    /// @brief White rook, has castling rights
    WHITE_ROOK_CAN_CASTLE      = 6U, // note: ROOK_CANNOT_CASTLE | 4

    /// @brief White king, white to move
    WHITE_KING_IN_TURN         = 7U,

    /// @brief En-passant capturable pawn
    ///
    /// The color of the en-passant capturable pawn depends on the side to move.
    EP_PAWN                    = 8U,

    /// @brief Black bishop
    BLACK_BISHOP               = 9U,

    /// @brief Black rook, no castling rights
    BLACK_ROOK_CANNOT_CASTLE   = 10U,

    /// @brief Black queen
    BLACK_QUEEN                = 11U, // note: BISHOP | ROOK_CANNOT_CASTLE

    /// @brief Black knight
    BLACK_KNIGHT               = 12U,

    /// @brief Black pawn
    BLACK_PAWN                 = 13U,

    /// @brief Black rook, has castling rights
    BLACK_ROOK_CAN_CASTLE      = 14U, // note: ROOK_CANNOT_CASTLE | 4

    /// @brief Black king
    BLACK_KING                 = 15U,
};

/// @brief A position encoded in 192 bits
///
/// The encoding scheme is as follows:
/// -# Field @coderef{occupancy} determines the squares that are occupied, same
///    encoding as @coderef{SquareSet}. All other squares are empty.
/// -# For every occupied square, the data planes specify an encoded
///    4-bit piece/square value (@coderef{CompressedPosition_PieceEncoding}).
/// -# For the Nth occupied square, the Nth bits of the data planes encode
///    the value as follows:<br>
///    <tt>value = BIT_N(dataPlanes[0]) + 2*BIT_N(dataPlanes[1]) + 4*BIT_N(dataPlanes[2]) + 8*BIT_N(dataPlanes[3])</tt>
///
/// Properties of the position other than the piece configuration are encoded as
/// follows:
///
/// <table>
/// <tr>
///   <th>Property</th>
///   <th>Encoding</th>
/// </tr>
/// <tr>
///   <td>Side to move</td>
///   <td>Encoded in the white king:
///      @coderef{CompressedPosition_PieceEncoding::WHITE_KING_IN_TURN} or
///      @coderef{CompressedPosition_PieceEncoding::WHITE_KING_NOT_IN_TURN}.</td>
/// </tr>
/// <tr>
///   <td>Castling rights</td>
///   <td>Encoded in the rooks:
///      @coderef{CompressedPosition_PieceEncoding::WHITE_ROOK_CANNOT_CASTLE},
///      @coderef{CompressedPosition_PieceEncoding::WHITE_ROOK_CAN_CASTLE},
///      @coderef{CompressedPosition_PieceEncoding::BLACK_ROOK_CANNOT_CASTLE},
///      @coderef{CompressedPosition_PieceEncoding::BLACK_ROOK_CAN_CASTLE}.</td>
/// </tr>
/// <tr>
///   <td>En passant square</td>
///   <td>Encoded as the en passant capturable pawn: @coderef{CompressedPosition_PieceEncoding::EP_PAWN}.
///       The color of the en passant capturable pawn is the opposite of the
///       side to move. The en passant square is inferred from the en passant
///       capturable pawn position (retreat by 1 square).
///   </td>
/// </tr>
/// </table>
///
/// The usual chess position restrictions apply:
/// -# There must be exactly one white king, indicated by either
///    @coderef{CompressedPosition_PieceEncoding::WHITE_KING_IN_TURN} or
///    @coderef{CompressedPosition_PieceEncoding::WHITE_KING_NOT_IN_TURN}.
/// -# There must be exactly one black king
/// -# The en-passant capturable pawn must be on the 4th rank (white pawn) or
///    5th rank (black pawn).
/// -# Castling rights:
///    -# @coderef{CompressedPosition_PieceEncoding::WHITE_ROOK_CAN_CASTLE} may
///       be present only on the 1st rank.
///    -# @coderef{CompressedPosition_PieceEncoding::BLACK_ROOK_CAN_CASTLE} may
///       be present only on the 8th rank.
///    -# If a @coderef{CompressedPosition_PieceEncoding::WHITE_ROOK_CAN_CASTLE}
///       is present, then white king must be on the 1st rank
///    -# If a @coderef{CompressedPosition_PieceEncoding::BLACK_ROOK_CAN_CASTLE}
///       is present, then black king must be on the 8th rank
///    -# There can be at most one rook that can castle on either side
///       (left/right) of the corresponding king.
///
/// @remark The encoding scheme is designed for fast compression and decompression
/// of the position represented by @coderef{ChessBoard}.
struct CompressedPosition_FixedLength
{
    /// @brief Occupancy mask
    ///
    /// @sa @coderef{SquareSet}
    std::uint64_t occupancy;

    /// @brief Data planes
    std::array<std::uint32_t, 4U> dataPlanes;
};

/// @brief Three-way comparison operator (spaceship). The comparison provides
/// strong ordering.
///
/// @param[in] lhs      Left-hand side compressed position
/// @param[in] rhs      Right-hand side compressed position
/// @return             Three-way comparison result
/// @retval <0          <tt>lhs < rhs</tt>
/// @retval 0           <tt>lhs == rhs</tt>
/// @retval >0          <tt>lhs > rhs</tt>
///
/// @remark The exact way that the less-than relation of two compressed
/// positions is unspecified. This is for performance reasons. Currently, the
/// compressed positions are cast to std::array<std::uint64_t, 3U> and
/// comparison is performed between the arrays. This minimizes the number of
/// required comparisons. The implementation is subject to change in future
/// releases.
inline auto operator <=> (const CompressedPosition_FixedLength &lhs, const CompressedPosition_FixedLength &rhs) noexcept
{
    using RawCompareType = std::array<std::uint64_t, 3U>;
    static_assert(sizeof(RawCompareType) == sizeof(CompressedPosition_FixedLength));

    const RawCompareType *p1 { std::bit_cast<const RawCompareType *>(&lhs) };
    const RawCompareType *p2 { std::bit_cast<const RawCompareType *>(&rhs) };

    return *p1 <=> *p2;
}

/// @brief Equality comparison operator.
///
/// @param[in] lhs      Left-hand side compressed position
/// @param[in] rhs      Right-hand side compressed position
/// @return             Comparison result (true for equality)
inline bool operator == (const CompressedPosition_FixedLength &lhs, const CompressedPosition_FixedLength &rhs) noexcept
{
    using RawCompareType = std::array<std::uint64_t, 3U>;
    static_assert(sizeof(RawCompareType) == sizeof(CompressedPosition_FixedLength));

    const RawCompareType *p1 { std::bit_cast<const RawCompareType *>(&lhs) };
    const RawCompareType *p2 { std::bit_cast<const RawCompareType *>(&rhs) };

    return *p1 == *p2;
}

/// @brief Position compressor that produces fixed-length output (192 bits).
///
/// The position compressor can compress/decompress all legal positions with 32
/// pieces or less.
class PositionCompressor_FixedLength
{
private:
    PositionCompressor_FixedLength() = delete;
    ~PositionCompressor_FixedLength() = delete;

public:
    /// @brief Compresses a chess position with at most 32 pieces
    ///
    /// @param[in]  board                   Position
    /// @param[out] out_compressedPosition  Compressed position
    /// @throws std::out_of_range  The position has more than 32 pieces.
    ///
    /// See @coderef{CompressedPosition_FixedLength} for the specication of
    /// position encoding.
    static void compress(const ChessBoard &board, CompressedPosition_FixedLength &out_compressedPosition);

    /// @brief Compresses a chess position with at most 32 pieces
    ///
    /// @param[in]  compressedPosition      Compressed position
    /// @param[in]  halfMoveClock           Half-move clock
    /// @param[in]  moveNum                 Move number
    /// @param[out] out_board               @coderef{ChessBoard} to be set with
    ///                                     position represented by @p compressedPosition
    /// @throws std::out_of_range           Bad compressed format
    /// @throws PgnError                    Illegal position
    ///
    /// See @coderef{CompressedPosition_FixedLength} for the specication of
    /// position encoding.
    static void decompress(const CompressedPosition_FixedLength &compressedPosition, std::uint8_t halfMoveClock, std::uint32_t moveNum, pgn_reader::ChessBoard &out_board);
};

/// @}

}

#endif

