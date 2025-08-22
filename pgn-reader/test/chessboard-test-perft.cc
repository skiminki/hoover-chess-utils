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


namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace
{

struct FenPerft
{
    const char *fen;
    std::uint8_t depth;
    std::uint64_t movenum;
    bool print;
};

FenPerft fenPerfts[] = {

    // start pos
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20, false },
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400, false },
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902, false },
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281, false },
    // { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, false },
    // { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324, true },

    // pawns test with extra spaces between tokens
    { "    2k1rn2/4P1P1/8/2Pp4/1p1p2p1/P1PP3P/3P4/3K4    w     -       d6     0   1    ", 1, 25, false },

    { "rnbqkbnr/pp1ppppp/8/4P3/2pP4/8/PPP2PPP/RNBQKBNR b KQkq d3 0 3", 1, 22, false },
    // knights test
    { "nn1k2nn/nn4nn/8/8/8/8/NN4NN/NN1K2NN w - - 0 1", 1, 28, false },
    { "nn1k2nn/nn4nn/8/8/8/8/NN4NN/NN1K2NN b - - 0 1", 1, 28, false },

    // bishops test
    { "3k3B/6p1/1p2p3/8/2BB4/1B6/2K2b2/8 w - - 0 1", 1, 27, false},

    // rooks
    { "k7/8/8/2R5/4R3/2R4R/5K2/8 w - - 0 1", 1, 53, false },

    // queens
    { "7k/1Q6/6r1/2Q2Qrr/4r3/5r2/2Q5/K7 w - - 0 1", 1, 74, false },

    // castling normal
    { "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 1, 26, false },
    { "r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 1, 26, false },

    // checks
    { "3k4/6b1/8/3n4/3p4/3K4/7r/8 w - - 0 1", 1, 2, false },
    { "k7/8/1p3p2/8/3K4/8/1p3p2/8 w - - 0 1", 1, 6, false },
    { "2k5/8/8/8/8/8/rr5n/R3K2R w KQ - 0 1", 1, 9, false },
    { "2k5/8/8/8/8/8/p3r2p/R3K2R w KQ - 0 1", 1, 3, false },
    { "8/8/3k4/8/3K4/8/8/8 w - - 0 1", 1, 5, false },
    { "8/8/3k4/8/3K4/8/8/8 w - - 0 1", 2, 36, false },
    { "8/8/3k4/8/3K4/8/8/8 w - - 0 1", 3, 257, false },

    // castling + check
    { "r1bqkbnr/pp2pppp/2np4/1Bp5/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 2 4", 1, 33, false },
    { "r3kR2/8/8/8/8/8/8/1K6 b q - 0 1", 1, 3, false },
    { "r3k2r/8/8/8/8/8/8/R2K3R b kq - 1 1", 2, 492, false },

    // EP captures with check
    { "n7/4k3/8/2Pp4/8/8/6K1/8 w - d6 0 1", 2, 98, false },
    { "nn1r1krb/p1p1pppp/q1bp4/8/1pPP2P1/4P3/PP3P1P/NNQRBKRB b KQkq c3 0 10", 2, 1131, false },
    { "nrbnk1rb/ppp1pq1p/3p4/6p1/2P1PpP1/1N3K2/PP1P1P1P/1RBN1QRB b kq g3 0 10", 2, 967, false },

    // exposure checks
    { "n7/4k3/8/6N1/1P5Q/B3K3/4R3/8 w - - 0 1", 2, 241, false },

    // castling FRC + check
    { "rk3R2/8/8/8/8/8/8/1K6 b q - 0 1", 1, 3, false },

    // pinned promoting pawn
    { "8/r2PK3/8/7k/8/8/8/8 w - - 0 1", 1, 7, true },

    // more complex
    // { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48, false},
    // { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039, false },
    // { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862, false },
    // { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603, false },
    // { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690, true },

    // FRC startpos
    { "nqbrnkrb/pppppppp/8/8/8/8/PPPPPPPP/NQBRNKRB w KQkq - 0 1", 1, 20, false },

    // FRC castling with multiple rooks on 1st/8th
    { "rr2k1rr/2pppp2/1p4p1/p6p/P6P/1P4P1/2PPPP2/RR1K2RR w KQkq - 1 1", 1, 21, false},
    { "rr2k1rr/2pppp2/1p4p1/p6p/P6P/1P4P1/2PPPP2/RR1K2RR w AHgb - 1 1", 1, 21, false},
    { "rr2k1rr/2pppp2/1p4p1/p6p/P6P/1P4P1/2PPPP2/RR1K2RR b KQkq - 1 1", 1, 21, false},
    { "rr2k1rr/2pppp2/1p4p1/p6p/P6P/1P4P1/2PPPP2/RR1K2RR w GBgb - 1 1", 1, 23, false},
    { "rr2k1rr/2pppp2/1p4p1/p6p/P6P/1P4P1/2PPPP2/RR1K2RR b GBgb - 1 1", 1, 23, false},

    // regressions
    { "rnbqkbnr/pppppppp/8/8/8/P7/1PPPPPPP/RNBQKBNR b KQkq - 0 1", 1, 20, false },
    { "1R6/P1R2nq1/8/4p1k1/4P3/1P6/1KP1r1p1/6n1 w - - 0 109", 1, 31, false },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/P1N2Q2/1PPBBPpP/1R2K2R b Kkq - 0 2", 2, 2201, false },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2, 191, false },
    { "8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - e3 0 1", 1, 16, false },
    { "Qr2kqbr/2bpp1pp/pn3p2/2p5/6P1/P1PP4/1P2PP1P/NRNBK1BR b HBhb - 0 11", 1, 34, false },
    { "nrkb2nr/ppppp1p1/6bp/5p2/BPP1P1P1/P7/3P1P1P/qRK1BQNR w KQkq - 0 11", 1, 28, false },
    { "4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 2, 32, false },

    // movegen coverage
    { "r2k4/1P6/8/8/8/3K4/8/8 w - - 0 1", 1, 16, false },
    { "8/3k4/8/8/8/8/1p6/R2K4 b - - 0 1", 1, 16, false },
    { "8/3k4/8/8/3p4/2R1R3/8/3K4 b - - 0 1", 1, 5, false },
    { "k7/8/8/8/8/8/r2PKP2/8 w - - 0 1", 1, 8, false },
    { "4n3/r2PKP1r/8/8/4k3/8/8/8 w - - 0 1", 1, 4, false },
    { "2k5/2b5/8/8/8/7P/r1P3PK/6NR w - - 0 1", 1, 1, false }, // pawn move blocks a check
    { "2k5/2b5/8/8/8/7P/r1P2BBK/6NR w - - 0 1", 1, 1, false }, // bishop move blocks a check
    { "2k5/2b5/8/8/8/7P/r1P2RBK/6NR w - - 0 1", 1, 1, false }, // rook move blocks a check
    { "8/8/8/8/1p2k1p1/pPb1PpPp/P1P2P1P/RB3KBN w - - 0 1", 1, 0, false }, // stalemate, no moves
    { "8/8/8/8/4k1p1/4PpPp/4nP1P/3Q1NKN w - - 0 1", 1, 1, false }, // in check, queen must move
};

void printMove(ChessBoard::Move move)
{
    printf("%c%c%c%c",
           StringUtils::colChar(move.getSrc()),
           StringUtils::rowChar(move.getSrc()),
           StringUtils::colChar(move.getDst()),
           StringUtils::rowChar(move.getDst()));

    switch (move.getTypeAndPromotion())
    {
        case ChessBoard::MoveTypeAndPromotion::PROMO_KNIGHT: printf("N"); break;
        case ChessBoard::MoveTypeAndPromotion::PROMO_BISHOP: printf("B"); break;
        case ChessBoard::MoveTypeAndPromotion::PROMO_ROOK:   printf("R"); break;
        case ChessBoard::MoveTypeAndPromotion::PROMO_QUEEN:  printf("Q"); break;
        default:                                                          break;
    }
}

std::uint64_t perft(ChessBoard &board, std::uint8_t depth, bool print)
{
    ChessBoard::MoveList moves;

    const std::size_t numMoves { board.generateMoves(moves) };
    std::size_t numLeafMoves { };

    if (print)
        board.printBoard();

    for (std::size_t i = 0; i < numMoves; ++i)
    {
        if (print)
            printMove(moves[i]);

        const ChessBoard refBoard { board };

        board.doMove(moves[i]);

        std::size_t leafMoves { };
        if (depth > 1U)
        {
            numLeafMoves += perft(board, depth - 1U, false);
        }
        else
        {
            ++numLeafMoves;
        }
        if (print)
        {
            printf(": %zu\n", leafMoves);
            if (depth == 1U)
                board.printBoard();
        }

        board = refBoard;

        // additional per-position checks

        // check legal castling rights
        if (board.getWhiteLongCastleRook() != Square::NONE)
        {
            EXPECT_EQ(
                board.getSquarePiece(board.getWhiteLongCastleRook()),
                PieceAndColor::WHITE_ROOK);
        }

        if (board.getWhiteShortCastleRook() != Square::NONE)
        {
            EXPECT_EQ(
                board.getSquarePiece(board.getWhiteShortCastleRook()),
                PieceAndColor::WHITE_ROOK);
        }

        if (board.getBlackLongCastleRook() != Square::NONE)
        {
            EXPECT_EQ(
                board.getSquarePiece(board.getBlackLongCastleRook()),
                PieceAndColor::BLACK_ROOK);
        }

        if (board.getBlackShortCastleRook() != Square::NONE)
        {
            EXPECT_EQ(
                board.getSquarePiece(board.getBlackShortCastleRook()),
                PieceAndColor::BLACK_ROOK);
        }

        // EP
        if (board.getEpSquare() != Square::NONE)
        {
            if (board.getTurn() == Color::WHITE)
            {
                EXPECT_EQ(rowOf(board.getEpSquare()), 5U);
            }
            else
            {
                EXPECT_EQ(rowOf(board.getEpSquare()), 2U);
            }
        }
    }

    return numLeafMoves;
}

}

TEST(ChessBoard, MoveGenPerft)
{
    ChessBoard board { };

    for (const FenPerft &fp : fenPerfts)
    {
        std::cout << '"' << fp.fen << "\" perft(" << (unsigned)fp.depth << "), expect: " << fp.movenum << std::endl;
        board.loadFEN(fp.fen);

        EXPECT_EQ(fp.movenum, perft(board, fp.depth, fp.print));
        if (fp.depth == 1)
        {
            EXPECT_EQ(fp.movenum, board.getNumberOfLegalMoves());
        }
    }
}

TEST(StringUtils, boardToFEN_perftFens)
{
    ChessBoard board { };
    ChessBoard board2 { };

    for (const FenPerft &fp : fenPerfts)
    {
        FenString fenFromBoard { MiniString_Uninitialized { } };

        std::cout << '"' << fp.fen << "\" == " << std::flush;
        board.loadFEN(fp.fen);

        StringUtils::boardToFEN(board, fenFromBoard);
        std::cout << '"' << fenFromBoard.getStringView() << '"' << std::endl;

        board2.loadFEN(fenFromBoard.getStringView());
        EXPECT_EQ(board, board2);
    }
}

}
