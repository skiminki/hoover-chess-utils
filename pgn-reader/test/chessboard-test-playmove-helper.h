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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TEST_PLAYMOVE_HELPER_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TEST_PLAYMOVE_HELPER_H_INCLUDED

#include "chessboard.h"
#include "pgnreader-string-utils.h"

#include "gtest/gtest.h"


namespace hoover_chess_utils::pgn_reader::unit_test
{


template <typename ... Args>
void playMove(
    ChessBoard &board,
    std::size_t expectNumMoves,
    std::size_t (ChessBoard::*generateMovesFn)(ShortMoveList &moves, SquareSet, Square, Args ...) const noexcept,
    Move (ChessBoard::*generateSingleMoveFn)(SquareSet, Square, Args ...) const noexcept,
    SquareSet srcSqMask, Square dstSq, Args && ... args)
{
    ShortMoveList moves;

    std::size_t numMoves { (board.*generateMovesFn)(moves, srcSqMask, dstSq, std::forward<Args>(args) ...) };
    EXPECT_EQ(expectNumMoves, numMoves);

    Move singleMove { (board.*generateSingleMoveFn)(srcSqMask, dstSq, std::forward<Args>(args) ...) };

    if (expectNumMoves == 0U)
    {
        EXPECT_TRUE(singleMove.isIllegal());
        EXPECT_EQ(singleMove, Move::illegalNoMove());
    }
    else if (expectNumMoves == 1U)
    {
        EXPECT_FALSE(singleMove.isIllegal());
    }
    else
    {
        EXPECT_TRUE(singleMove.isIllegal());
        EXPECT_EQ(singleMove, Move::illegalAmbiguousMove());
    }

    if (numMoves == expectNumMoves)
    {
        if (numMoves == 1U)
        {
            EXPECT_EQ(singleMove, moves[0U]);

            // Single move found and it is expected

            // Verify that the source square is within the srcSqMask
            EXPECT_TRUE(srcSqMask.isMember(singleMove.getSrc()));

            // Verify that the destination square is as expected
            EXPECT_EQ(dstSq, singleMove.getDst());

            // Additional check: verify that allowing all other sources except
            // move source returns no moves
            playMove<Args ...>(
                board,
                0U,
                generateMovesFn,
                generateSingleMoveFn,
                ~SquareSet::square(singleMove.getSrc()),
                dstSq,
                std::forward<Args>(args) ...);

            board.doMove(singleMove);
        }
    }
    else
    {
        board.printBoard();
    }
}

void playMove(
    ChessBoard &board,
    std::size_t expectNumMoves,
    std::size_t (ChessBoard::*generateMovesFn)(ShortMoveList &moves) const noexcept,
    Move (ChessBoard::*generateSingleMoveFn)() const noexcept)
{
    ShortMoveList moves;

    std::size_t numMoves { (board.*generateMovesFn)(moves) };
    EXPECT_EQ(expectNumMoves, numMoves);

    Move singleMove { (board.*generateSingleMoveFn)() };

    if (expectNumMoves == 0U)
    {
        EXPECT_TRUE(singleMove.isIllegal());
        EXPECT_EQ(singleMove, Move::illegalNoMove());
    }
    else if (expectNumMoves == 1U)
    {
        EXPECT_FALSE(singleMove.isIllegal());
    }
    else
    {
        EXPECT_TRUE(singleMove.isIllegal());
        EXPECT_EQ(singleMove, Move::illegalAmbiguousMove());
    }

    if (numMoves == expectNumMoves)
    {
        if (numMoves == 1U)
        {
            EXPECT_EQ(singleMove, moves[0U]);
            board.doMove(singleMove);
        }
    }
    else
    {
        board.printBoard();
    }
}

}

#define PLAY_MOVE(board, MoveFunction, ...)                             \
    hoover_chess_utils::pgn_reader::unit_test::playMove(                \
        board,                                                          \
        1U,                                                             \
        &hoover_chess_utils::pgn_reader::ChessBoard::generateMovesFor ## MoveFunction, \
        &hoover_chess_utils::pgn_reader::ChessBoard::generateSingleMoveFor ## MoveFunction \
        __VA_OPT__(,) __VA_ARGS__)

#define PLAY_MOVE_EXPECT_NO_MOVES(board, MoveFunction, ...)     \
    hoover_chess_utils::pgn_reader::unit_test::playMove(       \
        board,                                                          \
        0U,                                                             \
        &hoover_chess_utils::pgn_reader::ChessBoard::generateMovesFor ## MoveFunction, \
        &hoover_chess_utils::pgn_reader::ChessBoard::generateSingleMoveFor ## MoveFunction \
        __VA_OPT__(,) __VA_ARGS__)

#endif
