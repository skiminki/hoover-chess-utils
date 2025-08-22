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

struct Frame
{

    // list of moves in the current stack
    ChessBoard::MoveList moves;
    std::size_t numMoves;
    std::size_t i; // move list iterator

    ChessBoard board; // position of this frame
};

void initializeFrame(Frame &frame)
{
    frame.numMoves = frame.board.generateMoves(frame.moves);
    frame.i = 0U;
}

void perft(const std::string &fen, const std::uint32_t maxDepth)
{
    using namespace hoover_chess_utils::pgn_reader;

    std::uint64_t numPositions { };
    std::uint64_t failedTryMoves { };
    std::int64_t usecs { };

    if (maxDepth > 0)
    {
        std::vector<Frame> stack { };
        stack.resize(maxDepth);

        if (fen.empty())
            stack[0].board.loadStartPos();
        else
            stack[0].board.loadFEN(fen.c_str());

        const std::chrono::steady_clock::time_point begin { std::chrono::steady_clock::now() };

        initializeFrame(stack[0]);

        if (maxDepth == 1U)
        {
            numPositions = stack[0].numMoves;
        }
        else
        {
            std::uint32_t curDepth { };

            while (true)
            {
                Frame &frame { stack[curDepth] };

                if (frame.i < frame.numMoves)
                {
                    // new depth
                    if (curDepth + 2U == maxDepth)
                    {
                        // leaf frame
                        for (std::size_t i = 0U; i < frame.numMoves; ++i)
                        {
                            ChessBoard board { frame.board };
                            board.doMove(frame.moves[i]);
                            numPositions += board.getNumberOfLegalMoves();
                        }
                    }
                    else
                    {
                        Frame &newFrame { stack[curDepth + 1U] };
                        newFrame.board = frame.board;
                        newFrame.board.doMove(frame.moves[frame.i++]);
                        initializeFrame(newFrame);
                        ++curDepth;
                        continue;
                    }
                }

                // this frame is done, enter previous if any
                if (curDepth > 0U)
                    --curDepth;
                else
                    break;
            }
        }

        const std::chrono::steady_clock::time_point end { std::chrono::steady_clock::now() };
        usecs = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    }


    printf("Searched %" PRIu64 " positions in %" PRId64 ".%06" PRId64 " seconds\n",
           numPositions, usecs / 1000000, std::abs(usecs % 1000000));
    if (usecs > 0U)
    {
        printf("%" PRIu64 " positions per second\n", (numPositions * UINT64_C(1000000)) / usecs);
    }
    printf("Failed doMoves: %" PRIu64 "\n", failedTryMoves);
}

}


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printHelp(argc == 1 ? argv[0] : "perft");
        return 1;
    }

    std::uint32_t depth { std::numeric_limits<std::uint32_t>::max() };
    std::string fen { };

    // parse arguments
    {
        const std::string_view depthArg { argv[1] };

        std::from_chars(
            depthArg.data(), depthArg.data() + depthArg.size(),
            depth);

        if (depth == std::numeric_limits<std::uint32_t>::max())
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
