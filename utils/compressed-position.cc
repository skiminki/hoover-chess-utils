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

#include "compressed-position.h"

#include "chessboard.h"

#include <bit>
#include <iostream>
#include <stdexcept>

namespace hoover_chess_utils::utils
{

namespace {
constexpr bool debugMode { false };
}

void PositionCompressor::compress(const pgn_reader::ChessBoard &board, CompressedPosition &out_compressedPosition)
{
    using pgn_reader::Color;
    using pgn_reader::Square;
    using pgn_reader::SquareSet;

    static_cast<void>(board);
    static_cast<void>(out_compressedPosition);

    // needed for fast queen encoding
    static_assert(
        static_cast<std::uint8_t>(CompressedPieceEncoding::WHITE_QUEEN) ==
        (static_cast<std::uint8_t>(CompressedPieceEncoding::WHITE_BISHOP) | static_cast<std::uint8_t>(CompressedPieceEncoding::WHITE_ROOK_CANNOT_CASTLE)));
    static_assert(
        static_cast<std::uint8_t>(CompressedPieceEncoding::BLACK_QUEEN) ==
        (static_cast<std::uint8_t>(CompressedPieceEncoding::BLACK_BISHOP) | static_cast<std::uint8_t>(CompressedPieceEncoding::BLACK_ROOK_CANNOT_CASTLE)));

    // fast rook no/yes castling encoding
    static_assert(
        (static_cast<std::uint8_t>(CompressedPieceEncoding::WHITE_ROOK_CANNOT_CASTLE) | 4U) ==
        static_cast<std::uint8_t>(CompressedPieceEncoding::WHITE_ROOK_CAN_CASTLE));
    static_assert(
        (static_cast<std::uint8_t>(CompressedPieceEncoding::BLACK_ROOK_CANNOT_CASTLE) | 4U) ==
        static_cast<std::uint8_t>(CompressedPieceEncoding::BLACK_ROOK_CAN_CASTLE));

    out_compressedPosition.occupancy = static_cast<std::uint64_t>(board.getOccupancyMask());

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

        if (board.canEpCapture())
            epPawns = SquareSet::square(board.getEpSquare()) >> 8U;
    }
    else
    {
        whiteKingInTurnAndBlackKing = board.getKings() & board.getPiecesInTurn();

        if (board.canEpCapture())
            epPawns = SquareSet::square(board.getEpSquare()) << 8U;
    }

    SquareSet nonEpPawns { board.getPawns() & ~epPawns };

    std::array<SquareSet, 4U> planes { };
    planes[0] = board.getBishopsAndQueens() | nonEpPawns | whiteKingInTurnAndBlackKing;
    planes[1] = board.getRooksAndQueens()                | whiteKingInTurnAndBlackKing;
    planes[2] = board.getKnights()          | nonEpPawns | whiteKingInTurnAndBlackKing | rooksCanCastle;
    planes[3] = board.getBlackPieces()      | epPawns;

    out_compressedPosition.bitPlanes[0] = static_cast<std::uint64_t>(planes[0].parallelExtract(board.getOccupancyMask()));
    out_compressedPosition.bitPlanes[1] = static_cast<std::uint64_t>(planes[1].parallelExtract(board.getOccupancyMask()));
    out_compressedPosition.bitPlanes[2] = static_cast<std::uint64_t>(planes[2].parallelExtract(board.getOccupancyMask()));
    out_compressedPosition.bitPlanes[3] = static_cast<std::uint64_t>(planes[3].parallelExtract(board.getOccupancyMask()));

    if constexpr (debugMode) {
        pgn_reader::ChessBoard b1 { };

        decompress(
            out_compressedPosition,
            board.getHalfMoveClock(), pgn_reader::moveNumOfPly(board.getCurrentPlyNum()),
            b1);

        if (b1 != board)
        {
            std::cout << "MISMATCH!!!" << std::endl;
            board.printBoard();
            b1.printBoard();
            throw std::logic_error("Compress-decompress lost information");
        }
    }
}

void PositionCompressor::decompress(
    const CompressedPosition &compressedPosition, std::uint8_t halfMoveClock, std::uint32_t moveNum,
    pgn_reader::ChessBoard &out_board)
{
    using pgn_reader::Color;
    using pgn_reader::PieceAndColor;
    using pgn_reader::Square;
    using pgn_reader::SquareSet;
    using pgn_reader::makePlyNum;

    std::array<PieceAndColor, 64U> pieces { };
    Color turn { Color::WHITE };

    std::array<SquareSet, 4U> planes;
    planes[0] = SquareSet { compressedPosition.bitPlanes[0] }.parallelDeposit(SquareSet { compressedPosition.occupancy });
    planes[1] = SquareSet { compressedPosition.bitPlanes[1] }.parallelDeposit(SquareSet { compressedPosition.occupancy });
    planes[2] = SquareSet { compressedPosition.bitPlanes[2] }.parallelDeposit(SquareSet { compressedPosition.occupancy });
    planes[3] = SquareSet { compressedPosition.bitPlanes[3] }.parallelDeposit(SquareSet { compressedPosition.occupancy });

    std::array<CompressedPieceEncoding, 64U> cpe;
    std::array<Square, 2U> whiteCastlingRooks { Square::NONE, Square::NONE };
    std::array<Square, 2U> blackCastlingRooks { Square::NONE, Square::NONE };
    Square whiteKing { Square::NONE };
    Square blackKing { Square::NONE };
    Square epPawn { Square::NONE };

    for (std::uint8_t i { }; i < cpe.size(); ++i)
    {
        std::uint8_t value { };
        value  =        static_cast<std::uint64_t>((planes[0] >> i) & SquareSet { 1U });
        value |= 2U *   static_cast<std::uint64_t>((planes[1] >> i) & SquareSet { 1U });
        value |= 4U *   static_cast<std::uint64_t>((planes[2] >> i) & SquareSet { 1U });
        value |= 8U *   static_cast<std::uint64_t>((planes[3] >> i) & SquareSet { 1U });
        value |= 255U * (1U ^ ((compressedPosition.occupancy >> i) & 1U));

        switch (CompressedPieceEncoding { value })
        {
            case CompressedPieceEncoding::WHITE_KING_NOT_IN_TURN:
                pieces[i] = PieceAndColor::WHITE_KING;
                turn = Color::BLACK;
                whiteKing = Square { i };
                break;

            case CompressedPieceEncoding::WHITE_BISHOP:
                pieces[i] = PieceAndColor::WHITE_BISHOP;
                break;

            case CompressedPieceEncoding::WHITE_ROOK_CANNOT_CASTLE:
                pieces[i] = PieceAndColor::WHITE_ROOK;
                break;

            case CompressedPieceEncoding::WHITE_QUEEN:
                pieces[i] = PieceAndColor::WHITE_QUEEN;
                break;

            case CompressedPieceEncoding::WHITE_KNIGHT:
                pieces[i] = PieceAndColor::WHITE_KNIGHT;
                break;

            case CompressedPieceEncoding::WHITE_PAWN:
                pieces[i] = PieceAndColor::WHITE_PAWN;
                break;

            case CompressedPieceEncoding::WHITE_ROOK_CAN_CASTLE:
                pieces[i] = PieceAndColor::WHITE_ROOK;
                if (whiteCastlingRooks[0] == Square::NONE)
                    whiteCastlingRooks[0] = Square { i };
                else
                    whiteCastlingRooks[1] = Square { i };
                break;

            case CompressedPieceEncoding::WHITE_KING_IN_TURN:
                pieces[i] = PieceAndColor::WHITE_KING;
                whiteKing = Square { i };
                break;

            case CompressedPieceEncoding::EP_PAWN:
                epPawn = Square { i };
                break;

            case CompressedPieceEncoding::BLACK_BISHOP:
                pieces[i] = PieceAndColor::BLACK_BISHOP;
                break;

            case CompressedPieceEncoding::BLACK_ROOK_CANNOT_CASTLE:
                pieces[i] = PieceAndColor::BLACK_ROOK;
                break;

            case CompressedPieceEncoding::BLACK_QUEEN:
                pieces[i] = PieceAndColor::BLACK_QUEEN;
                break;

            case CompressedPieceEncoding::BLACK_KNIGHT:
                pieces[i] = PieceAndColor::BLACK_KNIGHT;
                break;

            case CompressedPieceEncoding::BLACK_PAWN:
                pieces[i] = PieceAndColor::BLACK_PAWN;
                break;

            case CompressedPieceEncoding::BLACK_ROOK_CAN_CASTLE:
                pieces[i] = PieceAndColor::BLACK_ROOK;
                if (blackCastlingRooks[0] == Square::NONE)
                    blackCastlingRooks[0] = Square { i };
                else
                    blackCastlingRooks[1] = Square { i };
                break;

            case CompressedPieceEncoding::BLACK_KING:
                pieces[i] = PieceAndColor::BLACK_KING;
                blackKing = Square { i };
                break;

            default:
                break;
        }
    }

    Square epSquare { Square::NONE };
    if (turn == Color::WHITE)
    {
        if (epPawn != Square::NONE)
        {
            pieces[static_cast<std::uint8_t>(epPawn)] = PieceAndColor::BLACK_PAWN;
            epSquare = addToSquareNoOverflowCheck(epPawn, +8);
        }
    }
    else
    {
        if (epPawn != Square::NONE)
        {
            pieces[static_cast<std::uint8_t>(epPawn)] = PieceAndColor::WHITE_PAWN;
            epSquare = addToSquareNoOverflowCheck(epPawn, -8);
        }
    }

    Square whiteLongCastleRook { Square::NONE };
    Square whiteShortCastleRook { Square::NONE };
    Square blackLongCastleRook { Square::NONE };
    Square blackShortCastleRook { Square::NONE };

    if (whiteCastlingRooks[0] != Square::NONE)
    {
        if (whiteCastlingRooks[0] < whiteKing)
            whiteLongCastleRook = whiteCastlingRooks[0];
        else
            whiteShortCastleRook = whiteCastlingRooks[0];

        if (whiteCastlingRooks[1] != Square::NONE)
        {
            if (whiteCastlingRooks[1] < whiteKing)
                whiteLongCastleRook = whiteCastlingRooks[1];
            else
                whiteShortCastleRook = whiteCastlingRooks[1];
        }
    }

    if (blackCastlingRooks[0] != Square::NONE)
    {
        if (blackCastlingRooks[0] < blackKing)
            blackLongCastleRook = blackCastlingRooks[0];
        else
            blackShortCastleRook = blackCastlingRooks[0];

        if (blackCastlingRooks[1] != Square::NONE)
        {
            if (blackCastlingRooks[1] < blackKing)
                blackLongCastleRook = blackCastlingRooks[1];
            else
                blackShortCastleRook = blackCastlingRooks[1];
        }
    }

    out_board.setBoard(
        pieces, whiteLongCastleRook, whiteShortCastleRook, blackLongCastleRook, blackShortCastleRook, epSquare,
        halfMoveClock, makePlyNum(moveNum, turn));
}

} // namespace ultimate_kibitzer
