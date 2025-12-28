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

// the baseline
#include "bitboard-attacks-portable.h"

#if HAVE_X86_BMI2
#include "bitboard-attacks-x86-bmi2.h"
#endif

#if HAVE_X86_AVX512F
#include "bitboard-attacks-x86-avx512f.h"
#endif

#include <array>
#include <cinttypes>

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Piece attack tables
class Attacks
{
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

        return Attacks_Portable::getPawnAttackMask(sq, pawnColor);
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

        return Attacks_Portable::getPawnAttackerMask(sq, pawnColor);
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
    /// @sa @coderef{generateMovesForPawnsTempl()}
    template <Color pawnColor, bool captureToRight>
    static constexpr inline SquareSet getPawnAttackersMask(SquareSet capturable) noexcept
    {
        return Attacks_Portable::getPawnAttackersMask<pawnColor, captureToRight>(capturable);
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
        return Attacks_Portable::getKnightAttackMask(sq);
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
    ///   <td>@ref HAVE_X86_BMI2</td>
    ///   <td>Implementation using @coderef{Attacks_BMI2::getBishopAttackMask()}</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Implementation using @coderef{Attacks_Portable::getBishopAttackMask()}</td>
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
#if HAVE_X86_BMI2
        return
            Attacks_BMI2::getBishopAttackMask(sq, occupancyMask);
#else
        return
            Attacks_Portable::getBishopAttackMask(sq, occupancyMask);
#endif
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
    ///   <td>@ref HAVE_X86_BMI2</td>
    ///   <td>Implementation using @coderef{Attacks_BMI2::getRookAttackMask()}</td>
    /// </tr>
    /// <tr>
    ///   <td>Otherwise</td>
    ///   <td>Implementation using @coderef{Attacks_Portable::getRookAttackMask()}</td>
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
#if HAVE_X86_BMI2
        return
            Attacks_BMI2::getRookAttackMask(sq, occupancyMask);
#else
        return
            Attacks_Portable::getRookAttackMask(sq, occupancyMask);
#endif
    }

    static inline SquareSet getQueenAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
#if HAVE_X86_BMI2
        return
            Attacks_BMI2::getBishopAttackMask(sq, occupancyMask) |
            Attacks_BMI2::getRookAttackMask(sq, occupancyMask);
#else
        return
            Attacks_Portable::getBishopAttackMask(sq, occupancyMask) |
            Attacks_Portable::getRookAttackMask(sq, occupancyMask);
#endif
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
        return Attacks_Portable::getKingAttackMask(sq);
    }

    /// @brief Checks whether a move by a possibly pinned piece does not expose
    /// a check.
    ///
    /// @param[in]   src           Source square
    /// @param[in]   dstBit        Destination square bit
    /// @param[in]   kingSq        King square
    /// @param[in]   pinnedPieces  Set of pinned pieces
    /// @return                    Move legality (from the point of pin)
    ///
    /// In case the piece on @p src is not pinned, this function will always
    /// return @true.
    ///
    /// In case the piece on @p src is pinned, this function checks whether the
    /// piece moves directly towards or away from the king.
    static inline bool pinCheck(Square src, SquareSet dstBit, Square kingSq, SquareSet pinnedPieces) noexcept
    {
        return ((pinnedPieces & SquareSet::square(src)) == SquareSet::none() ||
                (Intercepts::getPinRestiction<true>(kingSq, src) & dstBit) != SquareSet::none());
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
        SquareSet attackers { };

        // rooks and queens
        const SquareSet horizVertHits { Attacks::getRookAttackMask(sq, occupancyMask) };
        SquareSet attackers1 { horizVertHits & rooks };

        // bishops and queens
        const SquareSet diagHits { Attacks::getBishopAttackMask(sq, occupancyMask) };
        const SquareSet attackers2 { diagHits & bishops };

        // pawn attackers
        const SquareSet attackers3 { Attacks::getPawnAttackMask(sq, turn) & pawns };

        // king
        const SquareSet attackers4 { Attacks::getKingAttackMask(sq) & kings };

        // knights
        const SquareSet attackers5 { Attacks::getKnightAttackMask(sq) & knights };

        attackers = (attackers1 | attackers2 | attackers3 | attackers4 | attackers5) & ~turnColorMask;

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
    /// @param[in]  epSquare         En passant square or @coderef{Square::NONE}
    /// @param[in]  epCapturable     En passant capturable pawn or @coderef{SquareSet::none()}
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
        Square epSquare,
        SquareSet epCapturable,
        Square kingSq,
        Color turn,
        SquareSet &out_checkers,
        SquareSet &out_pinnedPieces) noexcept
    {

#if 0 && HAVE_X86_AVX512F // quite a lot slower than the PDEP/PEXT implementation on Zen4, so disable for now
        Attacks_AVX512F::determineSliderCheckersAndPins(
            occupancyMask,
            turnColorMask,
            bishops,
            rooks,
            epCapturable,
            kingSq,
            out_checkers,
            out_pinnedPieces);

        const SquareSet opponentPieces { occupancyMask &~ turnColorMask };

        // pawn checkers
        out_checkers |= Attacks::getPawnAttackMask(kingSq, turn) & pawns;

        // knights
        out_checkers |= Attacks::getKnightAttackMask(kingSq) & knights;

        out_checkers &= opponentPieces;

#else
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

        // Resolve pinned pieces. Notes:
        // - We'll remove only pinnable pieces from the first hits. The idea is to avoid
        //   x-rays over non-pinnable pieces in order to minimize the number of pinners
        // - In the second hit check, we'll remove pieces that are already determined to be checkers.
        //   The reason is the same as the above.
        const SquareSet secondHVHits {
            Attacks::getRookAttackMask(kingSq, (occupancyMask &~ firstHVHits) | opponentPieces) };
        const SquareSet secondDiagHits {
            Attacks::getBishopAttackMask(kingSq, (occupancyMask &~ firstDiagHits) | (opponentPieces &~ epCapturable)) };

        const SquareSet pinners {
            ((rooks & secondHVHits) | (bishops & secondDiagHits)) & opponentPieces &~ out_checkers };

        out_pinnedPieces = SquareSet { };
        SQUARESET_ENUMERATE(
            pinner,
            pinners,
            {
                SquareSet inBetween { Intercepts::getInterceptSquares(kingSq, pinner) };
                out_pinnedPieces |= inBetween & (turnColorMask | epCapturable);
            });
#endif
        // The rest of this function is EP capture legality checking. Short-circuit if there's no EP to capture
        if (epCapturable == SquareSet::none())
            return;

        // If we're in check and the only checker is not the EP pawn, EP capture
        // is not legal
        if ((out_checkers &~ epCapturable) != SquareSet::none())
            out_pinnedPieces |= epCapturable;

        // If not marked illegal yet, check whether the EP capture is legal.
        if ((epCapturable &~ out_pinnedPieces) != SquareSet::none())
        {
            // Adjacent pawns. This leaves one pawn in case there's
            // adjacent pawns both sides.
            const SquareSet adjacentPawns {
                ((((epCapturable & ~SquareSet::column(0U)) >> 1U) |
                  ((epCapturable & ~SquareSet::column(7U)) << 1U))

                 & pawns & turnColorMask)
            };

            // If all (usually 1) adjacent pawns are pinned and can't capture
            // along the pin direction, mark the EP pawn pinned, too. Capture is
            // not legal.
            SquareSet epCaptureLegalMask { };
            SQUARESET_ENUMERATE(
                epCapturer,
                adjacentPawns,
                {
                    if (pinCheck(epCapturer, SquareSet::square(epSquare), kingSq, out_pinnedPieces))
                        epCaptureLegalMask = SquareSet::all();
                });

            // add EP capturable to pinned if neither capture is legal
            out_pinnedPieces |= epCapturable &~ epCaptureLegalMask;

            // Check whether the EP capturable pawn is horizontally pinned. This can only happen
            // when the king is on the same row
            if ((epCapturable & (SquareSet::row(0U) << (static_cast<std::uint64_t>(kingSq) & 56U)))
                != SquareSet::none())
            {
                const SquareSet adjacentPawnsMinus1 { adjacentPawns.removeFirstSquare() };

                const SquareSet exposedHorizLine {
                    Attacks_Portable::getHorizRookAttackMask(
                        epCapturable.firstSquare(),
                        occupancyMask &~ (adjacentPawns &~ adjacentPawnsMinus1)) };

                const SquareSet kingBit { SquareSet::square(kingSq) };
                const SquareSet oppRooks { rooks & ~turnColorMask };

                const SquareSet pinnedEp { epCapturable &
                    (kingBit & exposedHorizLine).allIfAny() &
                    (oppRooks & exposedHorizLine).allIfAny() };

                // add the EP pawn in pinned pieces if capturing it would
                // horizontally expose the king to a rook
                out_pinnedPieces |= pinnedEp;
            }
        }
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
#if HAVE_X86_AVX512F
        return Attacks_AVX512F::determineAttackedSquares(
            occupancyMask,
            pawns,
            knights,
            bishops,
            rooks,
            king,
            turn);
#else
        return Attacks_Portable::determineAttackedSquares(
            occupancyMask,
            pawns,
            knights,
            bishops,
            rooks,
            king,
            turn);
#endif
    }
};

}

#endif
