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
#include "pgnreader-error.h"
#include "pgnreader-string-utils.h"

#include "gtest/gtest.h"

#include <array>


namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace
{

struct InputAndExpect
{
    const char *fen;
    PositionStatus status;
};

InputAndExpect expectStatus[] {
    // normal positions
    InputAndExpect { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", PositionStatus::NORMAL }, // no check, white to move
    InputAndExpect { "rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq - 0 1", PositionStatus::NORMAL }, // no check, black to move
    InputAndExpect { "3k4/8/8/8/B7/8/3K4/8 w - - 0 1", PositionStatus::NORMAL },

    InputAndExpect { "rnb1kbnr/pppp1ppp/4p3/8/5P1q/2N5/PPPPP1PP/R1BQKBNR w KQkq - 2 3", PositionStatus::CHECK }, // white in check
    InputAndExpect { "rnbqkbnr/ppppp1pp/5p2/7Q/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 1 2", PositionStatus::CHECK }, // black in check

    InputAndExpect { "8/8/8/2k5/8/8/3n4/1K6 w - - 0 1", PositionStatus::CHECK }, // white in check
    InputAndExpect { "8/8/8/2k5/4N3/8/8/1K6 b - - 0 1", PositionStatus::CHECK }, // black in check
    InputAndExpect { "3k4/8/8/B7/8/8/3K4/8 b - - 0 1", PositionStatus::CHECK },

    InputAndExpect { "rnb1kbnr/pppp1ppp/4p3/8/5PPq/8/PPPPP2P/RNBQKBNR w KQkq - 1 3", PositionStatus::MATE }, // white in mate
    InputAndExpect { "r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4", PositionStatus::MATE }, // black in mate
    InputAndExpect { "k7/8/NKB5/8/8/8/8/8 b - - 0 1", PositionStatus::MATE },

    InputAndExpect { "8/8/8/8/8/4k3/2q5/K7 w - - 0 1", PositionStatus::STALEMATE }, // white in stalemate
    InputAndExpect { "k7/8/1Q6/8/8/8/8/K7 b - - 0 1", PositionStatus::STALEMATE }, // black in stalemate
    InputAndExpect { "k7/8/NK6/8/8/8/8/8 b - - 0 1", PositionStatus::STALEMATE },
};

const char *expectIllegal[] {
    "rnb1kbnr/pppp1ppp/4p3/8/5P1q/2N5/PPPPP1PP/R1BQKBNR b KQkq - 2 3", // white in check, black to move (illegal)
    "rnbqkbnr/ppppp1pp/5p2/7Q/4P3/8/PPPP1PPP/RNB1KBNR w KQkq - 1 2", // black in check, white to move
    "8/8/8/2k5/8/8/3n4/1K6 b - - 0 1", // white in check, black to move
    "8/8/8/2k5/4N3/8/8/1K6 w - - 0 1", // black in check, white to move

    // wrong number of kings
    "k7/8/8/8/5Q2/8/8/8 b - - 0 1", // no white kings
    "k7/8/8/8/5Q2/8/8/K2K4 b - - 0 1", // two white kings
    "8/8/8/8/5Q2/8/8/1K6 b - - 0 1", // no black kings
    "8/1k5k/8/8/5Q2/8/8/1K6 b - - 0 1", // two black kings

    // illegal positions: pawns on 1st/8th
    "3k4/8/8/8/8/8/3K4/6P1 w - - 0 1", // pawns on 1st rank
    "3k2P1/8/8/8/8/8/3K4/8 w - - 0 1", // pawns on 8th rank

    // en passant: no pawn to capture
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e6 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq e3 0 1",

    // en passant: EP square not empty
    "rnbqkbnr/ppp2ppp/4p3/3Pp3/8/8/PPP1PPPP/RNBQKBNR w KQkq e6 0 1",

    // en passant: EP square wrong rank
    "rnbqkbnr/pppp1ppp/8/3Pp3/8/8/PPP1PPPP/RNBQKBNR b KQkq e6 0 1",

    // castling rights set but a rook is missing: KQkq notation
    "1nbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w KQkq - 0 1",

    // castling rights set but a rook is missing: FRC notation
    "1nbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w AHah - 0 1",
    "rnbqkbn1/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w AHah - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/1NBQKBNR w AHah - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w AHah - 0 1",

    // castling rights set but the king is not on 1st/8th
    "rnbqkbnr/pppppppp/8/8/8/4K3/PPPPPPPP/RNBQ1BNR w K - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/4K3/PPPPPPPP/RNBQ1BNR b Q - 0 1",
    "rnbq1bnr/pppppppp/4k3/8/8/8/PPPPPPPP/RNBQKBNR w k - 0 1",
    "rnbq1bnr/pppppppp/4k3/8/8/8/PPPPPPPP/RNBQKBNR b q - 0 1",

    // castling rights set multiple times: KQkq notation
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KK - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w QQ - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w kk - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w qq - 0 1",

    // castling rights set multiple times: FRC notation
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w AA - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w HH - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w aa - 0 1",
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w hh - 0 1",
};

}

TEST(ChessBoard, getPositionStatus)
{
    for (std::size_t i = 0U; i < std::size(expectStatus); ++i)
    {
        const InputAndExpect &t { expectStatus[i] };
        ChessBoard board;

        printf("fen[%zu]=\"%s\", expect status=%" PRIu8 "\n", i, t.fen, static_cast<std::uint8_t>(t.status));

        EXPECT_NO_THROW(board.loadFEN(t.fen));

        EXPECT_EQ(t.status, board.determineStatus());
    }
}

#define TEST_EXPECT_THROW_PGN_ERROR(st) \
    EXPECT_THROW(                       \
    try {                               \
        st;                             \
    }                                   \
    catch (const PgnError &e)           \
    {                                   \
        printf("- %s\n", e.what());     \
        throw;                          \
    }, PgnError)                        \


TEST(ChessBoard, validateBoard_illegalPosition)
{
    for (std::size_t i = 0U; i < std::size(expectIllegal); ++i)
    {
        const char *fen { expectIllegal[i] };

        ChessBoard board;

        printf("fen[%zu]=\"%s\", expect illegal\n", i, fen);

        TEST_EXPECT_THROW_PGN_ERROR(board.loadFEN(fen));
    }
}

TEST(ChessBoard, validateBoard_badCastlingRooks)
{
    ArrayBoard ab { };

    ab[getIndexOfSquare(Square::E1)] = PieceAndColor::WHITE_KING;
    ab[getIndexOfSquare(Square::A2)] = PieceAndColor::WHITE_ROOK;
    ab[getIndexOfSquare(Square::H2)] = PieceAndColor::WHITE_ROOK;

    ab[getIndexOfSquare(Square::E8)] = PieceAndColor::BLACK_KING;
    ab[getIndexOfSquare(Square::A7)] = PieceAndColor::BLACK_ROOK;
    ab[getIndexOfSquare(Square::H7)] = PieceAndColor::BLACK_ROOK;

    // wall of pawns to block checks
    for (std::size_t i { getIndexOfSquare(Square::A4) }; i < getIndexOfSquare(Square::A5); ++i)
    {
        ab[i] = PieceAndColor::WHITE_PAWN;
    }

    // castling rooks on wrong rank
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::A2, Square::NONE, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::H2, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::A7, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::NONE, Square::H7,
            Square::NONE, 0U, 0U));
    EXPECT_NO_THROW(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));

    // castling rooks, invalid squares
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square { 100 }, Square::NONE, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square { 100 }, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square { 100 }, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::NONE, Square { 100 },
            Square::NONE, 0U, 0U));
}

TEST(ChessBoard, validateBoard_badCastlingRookSide)
{
    ArrayBoard ab { };

    ab[getIndexOfSquare(Square::E1)] = PieceAndColor::WHITE_KING;
    ab[getIndexOfSquare(Square::A1)] = PieceAndColor::WHITE_ROOK;
    ab[getIndexOfSquare(Square::H1)] = PieceAndColor::WHITE_ROOK;

    ab[getIndexOfSquare(Square::E8)] = PieceAndColor::BLACK_KING;
    ab[getIndexOfSquare(Square::A8)] = PieceAndColor::BLACK_ROOK;
    ab[getIndexOfSquare(Square::H8)] = PieceAndColor::BLACK_ROOK;

    // wall of pawns to block checks
    for (std::size_t i { getIndexOfSquare(Square::A4) }; i < getIndexOfSquare(Square::A5); ++i)
    {
        ab[i] = PieceAndColor::WHITE_PAWN;
    }

    // castling rooks on wrong side of the king
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::H1, Square::NONE, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::A1, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::H8, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::NONE, Square::A8,
            Square::NONE, 0U, 0U));
}

TEST(ChessBoard, validateBoard_badCastlingKings)
{
    ArrayBoard ab { };

    ab[getIndexOfSquare(Square::E2)] = PieceAndColor::WHITE_KING;
    ab[getIndexOfSquare(Square::A1)] = PieceAndColor::WHITE_ROOK;
    ab[getIndexOfSquare(Square::H1)] = PieceAndColor::WHITE_ROOK;

    ab[getIndexOfSquare(Square::E7)] = PieceAndColor::BLACK_KING;
    ab[getIndexOfSquare(Square::A8)] = PieceAndColor::BLACK_ROOK;
    ab[getIndexOfSquare(Square::H8)] = PieceAndColor::BLACK_ROOK;

    // wall of pawns to block checks
    for (std::size_t i { getIndexOfSquare(Square::A4) }; i < getIndexOfSquare(Square::A5); ++i)
    {
        ab[i] = PieceAndColor::WHITE_PAWN;
    }

    // castling rooks on wrong rank
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::A1, Square::NONE, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::H1, Square::NONE, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::A8, Square::NONE,
            Square::NONE, 0U, 0U));
    TEST_EXPECT_THROW_PGN_ERROR(
        ChessBoard { }.setBoard(
            ab,
            Square::NONE, Square::NONE, Square::NONE, Square::H8,
            Square::NONE, 0U, 0U));
}


TEST(ChessBoard, validateBoard_BitBoard)
{
    ChessBoard referenceBoard { };
    referenceBoard.loadFEN("r2qkb1r/pppb1ppp/2np1n2/1B2p1B1/3PP3/2N2N2/PPP2PPP/R2QK2R b KQkq - 2 6");

    BitBoard bitBoard { };
    bitBoard.pawns = SquareSet {
        Square::A2, Square::B2, Square::C2, Square::D4, Square::E4, Square::F2, Square::G2, Square::H2,
        Square::A7, Square::B7, Square::C7, Square::D6, Square::E5, Square::F7, Square::G7, Square::H7 };
    bitBoard.knights = SquareSet { Square::C3, Square::C6, Square::F3, Square::F6 };
    bitBoard.bishops = SquareSet { Square::B5, Square::G5, Square::D7, Square::F8 };
    bitBoard.rooks   = SquareSet { Square::A1, Square::A8, Square::H1, Square::H8 };
    bitBoard.queens  = SquareSet { Square::D1, Square::D8 };
    bitBoard.kings   = SquareSet { Square::E1, Square::E8 };

    bitBoard.whitePieces =
        SquareSet::row(0) | SquareSet::row(1) | SquareSet::row(2) | SquareSet::row(3) | SquareSet { Square::B5, Square::G5 };

    ChessBoard board { };
    board.setBoard(
        bitBoard,
        Square::A1, Square::H1, Square::A8, Square::H8,
        Square::NONE, 2U, makePlyNum(6U, Color::BLACK));

    EXPECT_EQ(referenceBoard, board);
    if (referenceBoard != board)
    {
        std::cout << "Reference:" << std::endl;
        referenceBoard.printBoard();
        std::cout << "Got:" << std::endl;
        board.printBoard();
    }

    // flip turn
    referenceBoard.loadFEN("r2qkb1r/pppb1ppp/2np1n2/1B2p1B1/3PP3/2N2N2/PPP2PPP/R2QK2R w KQkq - 2 6");
    board.setBoard(
        bitBoard,
        Square::A1, Square::H1, Square::A8, Square::H8,
        Square::NONE, 2U, makePlyNum(6U, Color::WHITE));

    EXPECT_EQ(referenceBoard, board);

    // try with maximum move number
    EXPECT_NO_THROW(
        board.setBoard(
            bitBoard,
            Square::A1, Square::H1, Square::A8, Square::H8,
            Square::NONE, 2U, makePlyNum(9999U, Color::BLACK)));

    // try with overflowing move number
    TEST_EXPECT_THROW_PGN_ERROR(
        board.setBoard(
            bitBoard,
            Square::A1, Square::H1, Square::A8, Square::H8,
            Square::NONE, 2U, makePlyNum(100000U, Color::WHITE)));

    // intersecting bit boards
    bitBoard.rooks |= SquareSet { Square::C3 };
    TEST_EXPECT_THROW_PGN_ERROR(
        board.setBoard(
            bitBoard,
            Square::A1, Square::H1, Square::A8, Square::H8,
            Square::NONE, 2U, makePlyNum(6U, Color::BLACK)));


    std::cout << "Reference:" << std::endl;
    referenceBoard.printBoard();
    std::cout << "Got:" << std::endl;
    board.printBoard();
}

}
