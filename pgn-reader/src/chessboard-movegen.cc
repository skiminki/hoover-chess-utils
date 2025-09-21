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
        .generateSingleMoveForPawnAndDestNoCapture = generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestCapture = generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKnightAndDest = generateSingleMoveForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForBishopAndDest = generateSingleMoveForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForRookAndDest = generateSingleMoveForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForQueenAndDest = generateSingleMoveForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForKingAndDest = generateSingleMoveForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForShortCastling = generateSingleMoveForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateSingleMoveForLongCastling = generateSingleMoveForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMovesForPawnAndDestNoCapture = generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestCapture = generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForPawnAndDestPromoCapture = generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKnightAndDest = generateMovesForKnightAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForBishopAndDest = generateMovesForBishopAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForRookAndDest = generateMovesForRookAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForQueenAndDest = generateMovesForQueenAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForKingAndDest = generateMovesForKingAndDestTempl<MoveGenType::NO_CHECK>,
        .generateMovesForShortCastling = generateMovesForShortCastlingTempl<MoveGenType::NO_CHECK>,
        .generateMovesForLongCastling = generateMovesForLongCastlingTempl<MoveGenType::NO_CHECK>,

        .generateMoves = generateMovesTempl<MoveGenType::NO_CHECK>,
        .getNumberOfLegalMoves = getNumberOfLegalMovesTempl<MoveGenType::NO_CHECK>,
        .hasLegalMoves = hasLegalMovesTempl<MoveGenType::NO_CHECK>,
    },

    // Move generator functions: MoveGenType::CHECK
    MoveGenFunctions {
        .generateSingleMoveForPawnAndDestNoCapture = generateSingleMoveForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestCapture = generateSingleMoveForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoNoCapture = generateSingleMoveForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForPawnAndDestPromoCapture = generateSingleMoveForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKnightAndDest = generateSingleMoveForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForBishopAndDest = generateSingleMoveForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForRookAndDest = generateSingleMoveForRookAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForQueenAndDest = generateSingleMoveForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForKingAndDest = generateSingleMoveForKingAndDestTempl<MoveGenType::CHECK>,
        .generateSingleMoveForShortCastling = generateSingleIllegalNoMove,
        .generateSingleMoveForLongCastling = generateSingleIllegalNoMove,

        .generateMovesForPawnAndDestNoCapture = generateMovesForPawnAndDestNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestCapture = generateMovesForPawnAndDestCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoNoCapture = generateMovesForPawnAndDestPromoNoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForPawnAndDestPromoCapture = generateMovesForPawnAndDestPromoCaptureTempl<MoveGenType::CHECK>,
        .generateMovesForKnightAndDest = generateMovesForKnightAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForBishopAndDest = generateMovesForBishopAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForRookAndDest = generateMovesForRookAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForQueenAndDest = generateMovesForQueenAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForKingAndDest = generateMovesForKingAndDestTempl<MoveGenType::CHECK>,
        .generateMovesForShortCastling = generateNoMoves,
        .generateMovesForLongCastling = generateNoMoves,

        .generateMoves = generateMovesTempl<MoveGenType::CHECK>,
        .getNumberOfLegalMoves = getNumberOfLegalMovesTempl<MoveGenType::CHECK>,
        .hasLegalMoves = hasLegalMovesTempl<MoveGenType::CHECK>,
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
        .generateSingleMoveForKingAndDest = generateSingleMoveForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
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
        .generateMovesForKingAndDest = generateMovesForKingAndDestTempl<MoveGenType::DOUBLE_CHECK>,
        .generateMovesForShortCastling = generateNoMoves,
        .generateMovesForLongCastling = generateNoMoves,

        .generateMoves = generateMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .getNumberOfLegalMoves = getNumberOfLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
        .hasLegalMoves = hasLegalMovesTempl<MoveGenType::DOUBLE_CHECK>,
    },
};

}
