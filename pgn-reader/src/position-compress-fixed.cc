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

#include "position-compress-fixed.h"

#include "chessboard.h"
#include "chessboard-types-squareset.h"

#include <bit>
#include <format>
#include <iostream>
#include <stdexcept>

namespace hoover_chess_utils::pgn_reader
{

void PositionCompressor_FixedLength::compress(const pgn_reader::ChessBoard &board, CompressedPosition_FixedLength &out_compressedPosition)
{
    // needed for fast queen encoding
    static_assert(
        static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::WHITE_QUEEN) ==
        (static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::WHITE_BISHOP) | static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::WHITE_ROOK_CANNOT_CASTLE)));
    static_assert(
        static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::BLACK_QUEEN) ==
        (static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::BLACK_BISHOP) | static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::BLACK_ROOK_CANNOT_CASTLE)));

    // fast rook no/yes castling encoding
    static_assert(
        (static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::WHITE_ROOK_CANNOT_CASTLE) | 4U) ==
        static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::WHITE_ROOK_CAN_CASTLE));
    static_assert(
        (static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::BLACK_ROOK_CANNOT_CASTLE) | 4U) ==
        static_cast<std::uint8_t>(CompressedPosition_PieceEncoding::BLACK_ROOK_CAN_CASTLE));

    // 32 pieces or less? More cannot be compressed
    const SquareSet occupancyMask { board.getOccupancyMask() };
    if (occupancyMask.popcount() > 32U)
        throw std::out_of_range(
            std::format(
                "PositionCompressor_FixedLength::compress(): Cannot compress a position with more than 32 pieces (has {})",
                std::uint32_t { occupancyMask.popcount() }));

    out_compressedPosition.occupancy = static_cast<std::uint64_t>(occupancyMask);

    // specials
    SquareSet rooksCanCastle { };

    if (board.getWhiteLongCastleRook()  != Square::NONE)
        rooksCanCastle |= SquareSet::square(board.getWhiteLongCastleRook());
    if (board.getWhiteShortCastleRook() != Square::NONE)
        rooksCanCastle |= SquareSet::square(board.getWhiteShortCastleRook());
    if (board.getBlackLongCastleRook()  != Square::NONE)
        rooksCanCastle |= SquareSet::square(board.getBlackLongCastleRook());
    if (board.getBlackShortCastleRook() != Square::NONE)
        rooksCanCastle |= SquareSet::square(board.getBlackShortCastleRook());

    SquareSet epPawns { };
    SquareSet whiteKingInTurnAndBlackKing;

    if (board.getTurn() == Color::WHITE)
    {
        whiteKingInTurnAndBlackKing = board.getKings();
        epPawns = SquareSet::squareOrNone(board.getEpSquare()) >> 8U;
    }
    else
    {
        whiteKingInTurnAndBlackKing = board.getKings() & board.getPiecesInTurn();
        epPawns = SquareSet::squareOrNone(board.getEpSquare()) << 8U;
    }

    SquareSet nonEpPawns { board.getPawns() & ~epPawns };

    // non-compressed data planes
    std::array<SquareSet, 4U> planes { };
    planes[0] = board.getBishopsAndQueens() | nonEpPawns | whiteKingInTurnAndBlackKing;
    planes[1] = board.getRooksAndQueens()                | whiteKingInTurnAndBlackKing;
    planes[2] = board.getKnights()          | nonEpPawns | whiteKingInTurnAndBlackKing | rooksCanCastle;
    planes[3] = board.getBlackPieces()      | epPawns;

    // compress them by occupancy mask
    out_compressedPosition.dataPlanes[0] = static_cast<std::uint64_t>(planes[0].parallelExtract(occupancyMask));
    out_compressedPosition.dataPlanes[1] = static_cast<std::uint64_t>(planes[1].parallelExtract(occupancyMask));
    out_compressedPosition.dataPlanes[2] = static_cast<std::uint64_t>(planes[2].parallelExtract(occupancyMask));
    out_compressedPosition.dataPlanes[3] = static_cast<std::uint64_t>(planes[3].parallelExtract(occupancyMask));
}

void PositionCompressor_FixedLength::decompress(
    const CompressedPosition_FixedLength &compressedPosition, std::uint8_t halfMoveClock, std::uint32_t moveNum,
    pgn_reader::ChessBoard &out_board)
{
    std::array<SquareSet, 4U> planes;
    const SquareSet occupancyMask { compressedPosition.occupancy };

    if (occupancyMask.popcount() > 32U)
        throw std::out_of_range(
            std::format(
                "PositionCompressor_FixedLength::decompress(): Cannot decompress a position with more than 32 pieces (has {})",
                std::uint32_t { occupancyMask.popcount() }));

    planes[0] = SquareSet { compressedPosition.dataPlanes[0] }.parallelDeposit(occupancyMask);
    planes[1] = SquareSet { compressedPosition.dataPlanes[1] }.parallelDeposit(occupancyMask);
    planes[2] = SquareSet { compressedPosition.dataPlanes[2] }.parallelDeposit(occupancyMask);
    planes[3] = SquareSet { compressedPosition.dataPlanes[3] }.parallelDeposit(occupancyMask);

    // position description
    BitBoard bb { };
    std::array<Square, 2U> whiteCastlingRooks { Square::NONE, Square::NONE }; // long, short
    std::array<Square, 2U> blackCastlingRooks { Square::NONE, Square::NONE }; // long, short
    Square epSquare { Square::NONE };

    // specials
    SquareSet whiteKingNotInTurnOrEp { ~planes[0] & ~planes[1] & ~planes[2] & occupancyMask };
    SquareSet blackPieces { planes[3U] };
    SquareSet whiteKingNotInTurn { whiteKingNotInTurnOrEp & ~planes[3U] };
    SquareSet rooksCanCastle { ~planes[0U] & planes[1U] & planes[2U] };

    Color turn { whiteKingNotInTurn == SquareSet::none() ? Color::WHITE : Color::BLACK };

    SquareSet epPawn { whiteKingNotInTurnOrEp & planes[3U] };

    if (epPawn != SquareSet::none())
    {
        epSquare = epPawn.firstSquare();
        epSquare = addToSquareNoOverflowCheck(epSquare, 8);

        if (turn == Color::BLACK)
        {
            blackPieces &=~ epPawn;
            epSquare = addToSquareNoOverflowCheck(epSquare, -16);
        }

    }

    bb.pawns   =  planes[0] & ~planes[1] &  planes[2];  // 5
    bb.pawns  |=  epPawn;

    bb.knights = ~planes[0] & ~planes[1] &  planes[2];  // 4
    bb.bishops =  planes[0] & ~planes[1] & ~planes[2];  // 1
    bb.rooks   = ~planes[0] &  planes[1];               // 2 or 6
    bb.queens  =  planes[0] &  planes[1] & ~planes[2];  // 3

    bb.kings   =  planes[0] &  planes[1] &  planes[2];  // 7
    bb.kings  |=  whiteKingNotInTurn;

    bb.whitePieces = occupancyMask &~ blackPieces;

    SquareSet whiteRooksCanCastle { rooksCanCastle & bb.whitePieces };
    Square whiteKing { (bb.kings & bb.whitePieces).firstSquare() };
    SQUARESET_ENUMERATE(
        whiteCastlingRook,
        whiteRooksCanCastle,
        {
            std::size_t i { whiteCastlingRook > whiteKing };
            if (whiteCastlingRooks[i] != Square::NONE)
                throw std::out_of_range(
                    std::format(
                        "PositionCompressor_FixedLength::decompress(): Two white {} castling rooks",
                        i == 0U ? "long" : "short"));

            whiteCastlingRooks[i] = whiteCastlingRook;
        });

    SquareSet blackRooksCanCastle { rooksCanCastle &~ bb.whitePieces };
    Square blackKing { (bb.kings &~ bb.whitePieces).firstSquare() };
    SQUARESET_ENUMERATE(
        blackCastlingRook,
        blackRooksCanCastle,
        {
            std::size_t i { blackCastlingRook > blackKing };
            if (blackCastlingRooks[i] != Square::NONE)
                throw std::out_of_range(
                    std::format(
                        "PositionCompressor_FixedLength::decompress(): Two black {} castling rooks",
                        i == 0U ? "long" : "short"));

            blackCastlingRooks[i] = blackCastlingRook;
        });


    out_board.setBoard(
        bb,
        whiteCastlingRooks[0U], whiteCastlingRooks[1U],
        blackCastlingRooks[0U], blackCastlingRooks[1U],
        epSquare,
        halfMoveClock, makePlyNum(moveNum, turn));
}

} // namespace ultimate_kibitzer
