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

#include "chessboard.h"

#include "chessboard-movegen-all.h"
#include "chessboard-movegen-by-dest.h"

#include <array>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

template <typename... Args>
Move generateSingleIllegalNoMove(const ChessBoard &board, [[maybe_unused]] Args... args) noexcept
{
    static_cast<void>(board);

    return Move::illegalNoMove();
}

template <typename... Args>
std::size_t generateNoMoves(const ChessBoard &board, ShortMoveList &moves, [[maybe_unused]] Args... args) noexcept
{
    static_cast<void>(board);
    static_cast<void>(moves);

    return 0U;
}

}

std::array<MoveGenFunctions, 3U> MoveGenFunctionTables::m_fns
{
    // Move generator functions: MoveGenType::NO_CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestCapture = ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKnightAndDest = ChessBoard::generateSingleMoveForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForBishopAndDest = ChessBoard::generateSingleMoveForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForRookAndDest = ChessBoard::generateSingleMoveForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForQueenAndDest = ChessBoard::generateSingleMoveForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForShortCastling = ChessBoard::generateSingleMoveForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForLongCastling = ChessBoard::generateSingleMoveForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMovesForPawnAndDestNoCapture = ChessBoard::generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestCapture = ChessBoard::generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestPromoCapture = ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKnightAndDest = ChessBoard::generateMovesForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForBishopAndDest = ChessBoard::generateMovesForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForRookAndDest = ChessBoard::generateMovesForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForQueenAndDest = ChessBoard::generateMovesForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForShortCastling = ChessBoard::generateMovesForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateMovesForLongCastling = ChessBoard::generateMovesForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::NO_CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::NO_CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::NO_CHECK>,
    },

    // Move generator functions: MoveGenType::CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = ChessBoard::generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestCapture = ChessBoard::generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = ChessBoard::generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKnightAndDest = ChessBoard::generateSingleMoveForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForBishopAndDest = ChessBoard::generateSingleMoveForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForRookAndDest = ChessBoard::generateSingleMoveForRookAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForQueenAndDest = ChessBoard::generateSingleMoveForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForShortCastling = generateSingleIllegalNoMove,
        .generateSingleMoveForLongCastling = generateSingleIllegalNoMove,

        .generateMovesForPawnAndDestNoCapture = ChessBoard::generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestCapture = ChessBoard::generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = ChessBoard::generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoCapture = ChessBoard::generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForKnightAndDest = ChessBoard::generateMovesForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForBishopAndDest = ChessBoard::generateMovesForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForRookAndDest = ChessBoard::generateMovesForRookAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForQueenAndDest = ChessBoard::generateMovesForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForShortCastling = generateNoMoves,
        .generateMovesForLongCastling = generateNoMoves,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::CHECK>,
    },

    // Move generator functions: MoveGenType::DOUBLE_CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestCapture = generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestPromoNoCapture = generateSingleIllegalNoMove,
        .generateSingleMoveForPawnAndDestPromoCapture = generateSingleIllegalNoMove,
        .generateSingleMoveForKnightAndDest = generateSingleIllegalNoMove,
        .generateSingleMoveForBishopAndDest = generateSingleIllegalNoMove,
        .generateSingleMoveForRookAndDest = generateSingleIllegalNoMove,
        .generateSingleMoveForQueenAndDest = generateSingleIllegalNoMove,
        .generateSingleMoveForKingAndDest = ChessBoard::generateSingleMoveForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
        .generateSingleMoveForShortCastling = generateSingleIllegalNoMove,
        .generateSingleMoveForLongCastling = generateSingleIllegalNoMove,

        .generateMovesForPawnAndDestNoCapture = generateNoMoves,
        .generateMovesForPawnAndDestCapture = generateNoMoves,
        .generateMovesForPawnAndDestPromoNoCapture = generateNoMoves,
        .generateMovesForPawnAndDestPromoCapture = generateNoMoves,
        .generateMovesForKnightAndDest = generateNoMoves,
        .generateMovesForBishopAndDest = generateNoMoves,
        .generateMovesForRookAndDest = generateNoMoves,
        .generateMovesForQueenAndDest = generateNoMoves,
        .generateMovesForKingAndDest = ChessBoard::generateMovesForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
        .generateMovesForShortCastling = generateNoMoves,
        .generateMovesForLongCastling = generateNoMoves,

        .generateMoves = &ChessBoard::generateMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .getNumberOfLegalMoves = &ChessBoard::getNumberOfLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .hasLegalMoves = &ChessBoard::hasLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
    },
};

}
