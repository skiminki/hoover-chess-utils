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

#ifndef CHESS_UTILS__UTILS__COMPRESSED_POSITION_H_INCLUDED
#define CHESS_UTILS__UTILS__COMPRESSED_POSITION_H_INCLUDED

#include <array>
#include <cinttypes>


namespace hoover_chess_utils::pgn_reader
{
class ChessBoard;
}

namespace hoover_chess_utils::utils
{

// 4 bits per piece
enum class CompressedPieceEncoding : std::uint8_t
{
    WHITE_KING_NOT_IN_TURN     = 0,
    WHITE_BISHOP               = 1,
    WHITE_ROOK_CANNOT_CASTLE   = 2,
    WHITE_QUEEN                = 3, // note: BISHOP | ROOK_CANNOT_CASTLE
    WHITE_KNIGHT               = 4,
    WHITE_PAWN                 = 5,
    WHITE_ROOK_CAN_CASTLE      = 6, // note: ROOK_CANNOT_CASTLE | 4
    WHITE_KING_IN_TURN         = 7,

    EP_PAWN                    = 8, // black or white pawn, captureable with EP (color depends on turn <=> rank)

    BLACK_BISHOP               = 9,
    BLACK_ROOK_CANNOT_CASTLE   = 10,
    BLACK_QUEEN                = 11, // note: BISHOP | ROOK_CANNOT_CASTLE
    BLACK_KNIGHT               = 12,
    BLACK_PAWN                 = 13,
    BLACK_ROOK_CAN_CASTLE      = 14, // note: ROOK_CANNOT_CASTLE | 4
    BLACK_KING                 = 15,

    NUM_ENCODINGS,
};

static_assert(static_cast<std::uint8_t>(CompressedPieceEncoding::NUM_ENCODINGS) == 16,
              "4 bits per compressed square");

struct CompressedPosition
{
    static constexpr std::size_t ctMaxPieces { 32 };

    std::uint64_t occupancy;
    std::array<std::uint32_t, 4U> bitPlanes;
};

static_assert(sizeof(CompressedPosition) == 24, "Compressed position is supposed to be 192 bits == 24 bytes");

inline auto operator <=> (const CompressedPosition &lhs, const CompressedPosition &rhs)
{
    using RawType = std::array<std::uint64_t, 3U>;

    static_assert(sizeof(RawType) == sizeof(CompressedPosition));

    const RawType &lhs_raw { reinterpret_cast<const RawType &>(lhs) };
    const RawType &rhs_raw { reinterpret_cast<const RawType &>(rhs) };

    return lhs_raw <=> rhs_raw;
}

inline bool operator == (const CompressedPosition &lhs, const CompressedPosition &rhs)
{
    using RawType = std::array<std::uint64_t, 3U>;

    static_assert(sizeof(RawType) == sizeof(CompressedPosition));

    const RawType &lhs_raw { reinterpret_cast<const RawType &>(lhs) };
    const RawType &rhs_raw { reinterpret_cast<const RawType &>(rhs) };

    return lhs_raw == rhs_raw;
}

class PositionCompressor
{
private:
    PositionCompressor() = delete;
    ~PositionCompressor() = delete;

public:
    // throw out_of_range if the position has more than 32 pieces or cannot be
    // compressed for some other reason
    static void compress(const pgn_reader::ChessBoard &board, CompressedPosition &out_compressedPosition);

    // throws PgnError in case the position is invalid
    static void decompress(const CompressedPosition &compressedPosition, std::uint8_t halfMoveClock, std::uint32_t moveNum, pgn_reader::ChessBoard &out_board);
};

}

#endif
