// Hoover Chess Utilities / PGN reader
// Copyright (C) 2023-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include "bitboard-intercepts.h"
#include "slider-attacks.h"

#if HAVE_AVX512F
#include "bitboard-attacks-avx512f.h"
#endif

#include <array>
#include <cinttypes>

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Piece attack tables
class Attacks
{
private:
    /// @brief Pawn attack masks, flattened for both colors
    ///
    /// Layout:
    ///
    /// - <tt>element[0..63]</tt>: White pawn attacks per square. Non-zero
    ///   for ranks 1 to 7 (rows 0..6).
    /// - <tt>element[64..127]</tt>: Black pawn attacks per square. Non-zero
    ///   for ranks 2 to 8 (rows 1..7).
    ///
    /// This table is used for both pawn-to-attacked-square and attacking-pawn-to-square
    /// tables. Hence the inclusion of ranks 1 and 8.
    ///
    /// @sa @c getPawnAttackMask()
    /// @sa @c getPawnAttackerMask()
    /// @sa @c getPawnAttackersMask()
    static const std::array<std::uint64_t, 128U> ctPawnAttackMaskTableFlattened;

    /// @brief Knight attack masks
    ///
    /// Table of knight square to attacked square set masks.
    static const std::array<std::uint64_t, 64U> ctKnightAttackMaskTable;

    /// @brief King attack masks
    ///
    /// Table of king square to attacked square set masks.
    static const std::array<std::uint64_t, 64U> ctKingAttackMaskTable;

public:
    /// @brief Returns a set of squares that a pawn can attack
    ///
    /// @param[in]  sq         Pawn square
    /// @param[in]  pawnColor  Color of the pawn
    /// @return                Set of squares the pawn can attack
    ///
    /// **Example**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td class="maskw"> </td><td> </td><td class="maskw"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td>wP</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///
    /// @remark Validity of pawn position is not assumed. Hence, attack squares
    /// are provided for white pawns on 1st rank and black pawns on 8th rank.
    static inline SquareSet getPawnAttackMask(Square sq, Color pawnColor) noexcept
    {
        static_assert(static_cast<std::uint64_t>(Color::WHITE) == 0U);
        static_assert(static_cast<std::uint64_t>(Color::BLACK) == 8U);

        return SquareSet {
            ctPawnAttackMaskTableFlattened[(static_cast<std::uint64_t>(pawnColor) << 3U) | getIndexOfSquare(sq)] };
    }

    /// @brief Returns a set of squares from which a pawn can attack
    ///
    /// @param[in]  sq         Attacked square
    /// @param[in]  pawnColor  Color of the attacking pawn
    /// @return                Set of squares from which a pawn can attack @c sq
    ///
    /// **Example**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td>sq </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td class="maskw">wP</td><td> </td><td class="maskw">wP</td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///
    /// @remark Validity of pawn position is not assumed. Hence, attacker
    /// squares are provided for white pawns on 2nd rank (pawns on 1st) and
    /// black pawns on 7th rank (pawns on 8th).
    static inline SquareSet getPawnAttackerMask(Square sq, Color pawnColor) noexcept
    {
        static_assert(static_cast<std::uint64_t>(Color::WHITE) == 0U);
        static_assert(static_cast<std::uint64_t>(Color::BLACK) == 8U);

        return SquareSet {
            ctPawnAttackMaskTableFlattened[((static_cast<std::uint64_t>(pawnColor) ^ 8U) << 3U) | getIndexOfSquare(sq)] };
    }

    /// @brief For a given set of squares, returns the squares where pawns can
    /// attack the given squares.
    ///
    /// @tparam     pawnColor       Color of the attacking pawns
    /// @tparam     captureToRight  Whether pawns are capturing towards right
    ///                             (increasing column number)
    /// @param[in]  capturable      Capturable squares. Typically all opponent's
    ///                             pawns and pieces
    /// @return                     Set of pawn attacker squares
    ///
    /// **Example** (white pawn, capture right)
    ///
    /// <table class="bitboard">
    /// <tr><td>c</td><td> </td><td>c</td><td>c</td><td>c</td><td> </td><td> </td><td>c</td></tr>
    /// <tr><td>c</td><td>c</td><td>c</td><td class="maskw">wP</td><td>c</td><td>c</td><td class="maskw">wP</td><td> </td></tr>
    /// <tr><td class="maskw">wP</td><td class="maskw">wP</td><td>c</td><td class="maskw">wP</td><td class="maskw">wP</td><td> </td><td> </td><td>c</td></tr>
    /// <tr><td> </td><td> </td><td> </td><td>c</td><td>c</td><td> </td><td>c</td><td> </td></tr>
    /// <tr><td> </td><td>c</td><td class="maskw">wP</td><td class="maskw">wP</td><td> </td><td class="maskw">wP</td><td> </td><td> </td></tr>
    /// <tr><td class="maskw">wP</td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///
    /// @remark Validity of pawn position is not assumed. Hence, attacker
    /// squares are provided for white pawns on 2nd rank (pawns on 1st) and
    /// black pawns on 7th rank (pawns on 8th).
    ///
    /// @sa @coderef{ChessBoard::generateMovesForPawnsTempl()}
    template <Color pawnColor, bool captureToRight>
    static constexpr inline SquareSet getPawnAttackersMask(SquareSet capturable) noexcept
    {
        if constexpr (pawnColor == Color::WHITE)
        {
            if constexpr (captureToRight)
            {
                SquareSet destMask { 0xFE'FE'FE'FE'FE'FE'FE'00 };
                return (capturable & destMask) >> 9U;
            }
            else
            {
                SquareSet destMask { 0x7F'7F'7F'7F'7F'7F'7F'00 };
                return (capturable & destMask) >> 7U;
            }
        }
        else
        {
            if constexpr (captureToRight)
            {
                SquareSet destMask { 0x00'FE'FE'FE'FE'FE'FE'FE };
                return (capturable & destMask) << 7U;
            }
            else
            {
                SquareSet destMask { 0x00'7F'7F'7F'7F'7F'7F'7F };
                return (capturable & destMask) << 9U;
            }
        }
    }

    /// @brief Returns the set of squares a knight can attack
    ///
    /// @param[in]  sq     Knight square
    /// @return            Set of attacked squares
    ///
    /// **Example**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td>N</td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    static inline SquareSet getKnightAttackMask(Square sq) noexcept
    {
        return SquareSet { ctKnightAttackMaskTable[getIndexOfSquare(sq)] };
    }

    /// @brief Returns the set of squares a bishop can attack, given also
    /// a set of occupied squares on board.
    ///
    /// @param[in]  sq             Bishop square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of attacked squares
    ///
    /// Per every diagonal direction from @c sq, the set of attacked squares is
    /// all squares between @c sq and the first occupied square (if any)
    /// including the occupied square. If no occupied squares are in that
    /// direction, then all squares in that direction are included.
    ///
    /// Occupancy of @c sq is ignored.
    ///
    /// The implementation used is as follows:
    /// <table>
    /// <tr>
    ///   <th>Build condition</th>
    ///   <th>Description</th>
    /// </tr>
    /// <tr>
    ///   <td>@ref HAVE_PDEP_PEXT</td>
    ///   <td>Implementation using @coderef{SliderAttacksPextPdep::getBishopAttackMask()}</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Implementation using @coderef{SliderAttacksGeneric::getBishopAttackMask()}</td>
    /// </tr>
    /// </table>
    ///
    /// **Example**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td>o</td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td class="mask"> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td>B</td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td class="mask"> </td><td> </td><td class="mask"> </td><td>o</td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td>o</td><td class="mask">o</td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td>o</td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///
    ///
    static inline SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        return SliderAttacks::getBishopAttackMask(sq, occupancyMask);
    }

    /// @brief Returns the set of squares a rook can attack, given also
    /// a set of occupied squares on board.
    ///
    /// @param[in]  sq             Rook square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of attacked squares
    ///
    /// Per horizontal/vertical direction from @c sq, the set of attacked
    /// squares is all squares between @c sq and the first occupied square (if
    /// any) including the occupied square. If no occupied squares are in that
    /// direction, then all squares in that direction are included.
    ///
    /// Occupancy of @c sq is ignored.
    ///
    /// The implementation used is as follows:
    /// <table>
    /// <tr>
    ///   <th>Build condition</th>
    ///   <th>Description</th>
    /// </tr>
    /// <tr>
    ///   <td>@ref HAVE_PDEP_PEXT</td>
    ///   <td>Implementation using @coderef{SliderAttacksPextPdep::getRookAttackMask()}</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Implementation using @coderef{SliderAttacksGeneric::getRookAttackMask()}</td>
    /// </tr>
    /// </table>
    ///
    /// **Example**
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td class="mask"> </td><td class="mask"> </td><td class="mask"> </td><td>R</td><td class="mask"> </td><td class="mask"> </td><td class="mask">o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td>o</td><td> </td><td class="mask"> </td><td> </td><td>o</td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        return SliderAttacks::getRookAttackMask(sq, occupancyMask);
    }

    /// @brief Returns the set of squares a king can attack
    ///
    /// @param[in]  sq         King square
    /// @return                Set of attacked squares
    ///
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td class="mask"> </td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td>K</td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td class="mask"> </td><td class="mask"> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    static inline SquareSet getKingAttackMask(Square sq) noexcept
    {
        return SquareSet { ctKingAttackMaskTable[getIndexOfSquare(sq)] };
    }

    static inline SquareSet determineAttackers(
        const SquareSet occupancyMask,
        const SquareSet turnColorMask,
        const SquareSet pawns,
        const SquareSet knights,
        const SquareSet bishops,
        const SquareSet rooks,
        const SquareSet kings,
        const Square sq,
        const Color turn) noexcept
    {
        const SquareSet opponentPieces { occupancyMask & ~turnColorMask };
        SquareSet attackers { };

        // rooks and queens
        const SquareSet horizVertHits { Attacks::getRookAttackMask(sq, occupancyMask) };
        SquareSet attackers4 { horizVertHits & rooks };

        // bishops and queens
        const SquareSet diagHits { Attacks::getBishopAttackMask(sq, occupancyMask) };
        const SquareSet attackers5 { diagHits & bishops };

        // pawn attackers
        const SquareSet attackers1 { Attacks::getPawnAttackMask(sq, turn) & pawns };

        // king
        const SquareSet attackers2 { Attacks::getKingAttackMask(sq) & kings };

        // knights
        const SquareSet attackers3 { Attacks::getKnightAttackMask(sq) & knights };

        attackers = (attackers1 | attackers2 | attackers3 | attackers4 | attackers5) & opponentPieces;

        return attackers;
    }

    /// @brief Determines all checkers and pinners
    ///
    /// @param[in]  occupancyMask    All occupied squares. These block slider attacks
    /// @param[in]  turnColorMask    Pieces of the side to move
    /// @param[in]  pawns            All pawns
    /// @param[in]  knights          All knights
    /// @param[in]  bishops          All bishops and queens
    /// @param[in]  rooks            All rooks and queens
    /// @param[in]  epSquare         En passant capturable pawn (if any)
    /// @param[in]  kingSq           King of side to move
    /// @param[in]  turn             Side to move
    /// @param[out] out_checkers     Set of checking pieces
    /// @param[out] out_pinnedPieces Set of pinned pieces. The capturable en passant pawn
    ///                              is included if it is pinned diagonally.
    ///
    /// @remark A diagonally pinned en passant capturable pawn cannot be
    /// captured due to the pin.
    static inline void determineCheckersAndPins(
        SquareSet occupancyMask,
        SquareSet turnColorMask,
        SquareSet pawns,
        SquareSet knights,
        SquareSet bishops,
        SquareSet rooks,
        SquareSet epCapturable,
        Square kingSq,
        Color turn,
        SquareSet &out_checkers,
        SquareSet &out_pinnedPieces) noexcept
    {
        const SquareSet opponentPieces { occupancyMask ^ turnColorMask };

        // pawn checkers
        out_checkers = Attacks::getPawnAttackMask(kingSq, turn) & pawns;

        // knights
        out_checkers |= Attacks::getKnightAttackMask(kingSq) & knights;

        // rooks and queens
        const SquareSet firstHVHits { Attacks::getRookAttackMask(kingSq, occupancyMask) };
        out_checkers |= firstHVHits & rooks;

        // bishops and queens
        const SquareSet firstDiagHits { Attacks::getBishopAttackMask(kingSq, occupancyMask) };
        out_checkers |= firstDiagHits & bishops;

        out_checkers &= opponentPieces;

        // potential pinners
        const SquareSet secondHVHits { Attacks::getRookAttackMask(kingSq, occupancyMask &~ firstHVHits) };
        const SquareSet secondDiagHits { Attacks::getBishopAttackMask(kingSq, occupancyMask &~ firstDiagHits) };

        SquareSet xrayHorizVertAttackers {
            rooks & (occupancyMask & secondHVHits & opponentPieces) };
        SquareSet xrayDiagAttackers {
            bishops & (occupancyMask & secondDiagHits & opponentPieces) };

        out_pinnedPieces = SquareSet { };
        SQUARESET_ENUMERATE(
            pinner,
            xrayHorizVertAttackers,
            {
                SquareSet inBetween { Intercepts::getInterceptSquares(kingSq, pinner) };
                out_pinnedPieces |= inBetween & turnColorMask;
            });
        SQUARESET_ENUMERATE(
            pinner,
            xrayDiagAttackers,
            {
                SquareSet inBetween { Intercepts::getInterceptSquares(kingSq, pinner) };
                out_pinnedPieces |= inBetween & (turnColorMask | epCapturable);
            });
    }

    /// @brief Determines all attacked squares
    ///
    /// @param[in] occupancyMask    All occupied squares. These block slider attacks
    /// @param[in] pawns            Attacking pawns
    /// @param[in] knights          Attacking knights
    /// @param[in] bishops          Attacking bishops and queens
    /// @param[in] rooks            Attacking rooks and queens
    /// @param[in] king             Attacking king
    /// @param[in] turn             Side to move (determines pawn attacking direction)
    /// @return                     All attacked squares
    static inline SquareSet determineAttackedSquares(
        SquareSet occupancyMask,
        SquareSet pawns,
        SquareSet knights,
        SquareSet bishops,
        SquareSet rooks,
        Square king,
        Color turn) noexcept
    {
#if HAVE_AVX512F
        return Attacks_AVX512F::determineAttackedSquares(
            occupancyMask,
            pawns,
            knights,
            bishops,
            rooks,
            king,
            turn);
#else
    SquareSet attacks { };

    // pawn attacks
    static_assert(static_cast<std::int8_t>(Color::WHITE) == 0);
    static_assert(static_cast<std::int8_t>(Color::BLACK) == 8);

    // for rotl -- pawn color is opposite to turn
    std::int8_t pawnAdvanceShiftLeft = -9 + 2 * static_cast<std::int8_t>(turn);

    // captures to left
    attacks |= (pawns &~ SquareSet::column(0)).rotl(pawnAdvanceShiftLeft);

    // captures to right
    attacks |= (pawns &~ SquareSet::column(7)).rotl(pawnAdvanceShiftLeft + 2);


    SQUARESET_ENUMERATE(
        piece,
        knights,
        attacks |= Attacks::getKnightAttackMask(piece));

    SQUARESET_ENUMERATE(
        piece,
        bishops,
        attacks |= SliderAttacks::getBishopAttackMask(piece, occupancyMask));

    SQUARESET_ENUMERATE(
        piece,
        rooks,
        attacks |= SliderAttacks::getRookAttackMask(piece, occupancyMask));

    attacks |= Attacks::getKingAttackMask(king);

    return attacks;
#endif
    }
};

#if HAVE_AVX512F
SquareSet Attacks_AVX512F::getKingAttackMask(Square sq) noexcept
{
    return Attacks::getKingAttackMask(sq);
}
#endif

}

#endif
