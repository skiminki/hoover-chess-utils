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

#include "chessboard.h"

#include <charconv>
#include <chrono>
#include <filesystem>
#include <limits>
#include <string_view>
#include <vector>

namespace
{

void printHelp(const char *exe)
{
    printf(
        "Performance test for the PGN reader move generator.\n"
        "\n"
        "Usage: %s <depth> [<FEN>]\n%s",
        std::filesystem::path(exe).filename().c_str(),
        "\n"
        "  <depth>    Search tree depth.\n"
        "  <FEN>      FEN of the search root. If omitted, standard starting position is\n"
        "             used. Shell quoting is not needed.\n"
        "\n");

}

using namespace hoover_chess_utils::pgn_reader;

std::tuple<std::uint64_t, std::chrono::steady_clock::duration>
perftDepth1(const std::string &fen)
{
    ChessBoard board { };

    if (!fen.empty())
        board.loadFEN(fen.c_str());

    const std::chrono::steady_clock::time_point begin { std::chrono::steady_clock::now() };
    const std::uint64_t numLegalMoves { board.getNumberOfLegalMoves() };
    const std::chrono::steady_clock::time_point end { std::chrono::steady_clock::now() };

    return std::make_tuple(numLegalMoves, end - begin);
}

struct Frame
{

    // list of moves in the current stack
    std::size_t i; // move list iterator
    std::size_t numMoves;

    ChessBoard board; // position of this frame
    MoveList moves;
};

void initializeFrame(Frame &frame)
{
    frame.numMoves = frame.board.generateMoves(frame.moves);
    frame.i = 0U;
}

std::tuple<std::uint64_t, std::chrono::steady_clock::duration>
perftDepth2(const std::string &fen)
{
    std::uint64_t numPositions { };

    ChessBoard board { };

    if (!fen.empty())
        board.loadFEN(fen.c_str());

    const std::chrono::steady_clock::time_point begin { std::chrono::steady_clock::now() };

    MoveList moves;
    std::size_t numMoves { };

    numMoves = board.generateMoves(moves);
    if (numMoves >= 1U)
    {
        std::size_t i;

        for (i = 0U; i < (numMoves - 1U); ++i)
        {
            ChessBoard tmpBoard { board };
            tmpBoard.doMove(moves[i]);
            numPositions += tmpBoard.getNumberOfLegalMoves();
        }

        board.doMove(moves[i]);
        numPositions += board.getNumberOfLegalMoves();
    }

    const std::chrono::steady_clock::time_point end { std::chrono::steady_clock::now() };

    return std::make_tuple(numPositions, end - begin);
}

std::tuple<std::uint64_t, std::chrono::steady_clock::duration>
perftDepth3Plus(const std::string &fen, const std::uint_fast8_t maxDepth)
{
    std::uint64_t numPositions { };
    std::vector<Frame> stack { };
    stack.resize(maxDepth - 2U);

    if (!fen.empty())
        stack[0U].board.loadFEN(fen.c_str());

    const std::chrono::steady_clock::time_point begin { std::chrono::steady_clock::now() };

    Frame *curDepth { stack.data() };
    Frame *const zeroDepth { curDepth };
    Frame *const maxNonLeafDepthMinus2 { curDepth + (maxDepth - 3U) };

    initializeFrame(*curDepth);

    while (true)
    {
        if (curDepth->i < curDepth->numMoves)
        {
            // new depth, depth <= leaf-2

            if (curDepth == maxNonLeafDepthMinus2)
            {
                Frame &frame { *curDepth };
                ChessBoard leafMinus1Board { frame.board };
                leafMinus1Board.doMove(frame.moves[frame.i++]);

                MoveList leafMinus1MoveList;
                std::size_t const leafMinus1NumMoves { leafMinus1Board.generateMoves(leafMinus1MoveList) };

                if (leafMinus1NumMoves >= 1U)
                {
                    // leaf-1 frame: just loop over the maxDepth-1 (leaf) positions
                    std::size_t i;

                    for (i = 0U; i < (leafMinus1NumMoves - 1U); ++i)
                    {
                        ChessBoard board { leafMinus1Board };
                        board.doMove(leafMinus1MoveList[i]);
                        numPositions += board.getNumberOfLegalMoves();
                    }

                    // avoid board copy for final move
                    leafMinus1Board.doMove(leafMinus1MoveList[i]);
                    numPositions += leafMinus1Board.getNumberOfLegalMoves();
                }
            }
            else
            {
                Frame &frame { *curDepth };

                ++curDepth;
                curDepth->board = frame.board;
                curDepth->board.doMove(frame.moves[frame.i++]);
                initializeFrame(*curDepth);
            }
        }
        else
        {
            // this frame is done, enter previous if any
            if (curDepth == zeroDepth) [[unlikely]]
                break;

            --curDepth;
        }
    }

    const std::chrono::steady_clock::time_point end { std::chrono::steady_clock::now() };

    return std::make_tuple(numPositions, end - begin);
}

void perft(const std::string &fen, const std::uint_fast8_t maxDepth)
{
    using namespace hoover_chess_utils::pgn_reader;

    std::uint64_t numPositions { };
    std::chrono::steady_clock::duration duration { };

    switch (maxDepth)
    {
        case 0U:
            break;

        case 1U:
            std::tie(numPositions, duration) = perftDepth1(fen);
            break;

        case 2U:
            std::tie(numPositions, duration) = perftDepth2(fen);
            break;

        default:
            std::tie(numPositions, duration) = perftDepth3Plus(fen, maxDepth);
            break;
    }

    const std::int64_t usecs { std::chrono::duration_cast<std::chrono::microseconds>(duration).count() };
    printf("Searched %" PRIu64 " positions in %" PRId64 ".%06" PRId64 " seconds\n",
           numPositions, usecs / 1000000, std::abs(usecs % 1000000));
    if (usecs > 0U)
    {
        printf("%" PRIu64 " positions per second\n", (numPositions * UINT64_C(1000000)) / usecs);
    }
}

}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printHelp(argc == 1 ? argv[0] : "perft");
        return 1;
    }

    std::uint8_t depth { std::numeric_limits<std::uint8_t>::max() };
    std::string fen { };

    // parse arguments
    {
        const std::string_view depthArg { argv[1] };

        std::from_chars(
            depthArg.data(), depthArg.data() + depthArg.size(),
            depth);

        if (depth == std::numeric_limits<std::uint8_t>::max())
        {
            fprintf(stderr, "Bad depth: '%s'\n", argv[1]);
            return 2;
        }
    }

    // collect FEN from the commandline arguments
    for (int i = 2; i < argc; ++i)
    {
        if (!fen.empty())
            fen += ' ';

        fen += argv[i];
    }

    perft(fen.c_str(), depth);

    return 0;
}
