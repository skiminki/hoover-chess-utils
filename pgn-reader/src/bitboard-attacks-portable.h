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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_PORTABLE_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_PORTABLE_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include "bitboard-intercepts.h"
#include "lookup-utils.h"

#include <array>
#include <cstdint>

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Portable implementations for attacks
class Attacks_Portable
{
private:
    /// @brief Calculate vertical, diagonal, or antidiagonal attack mask using
    /// the Hyperbola quintessence algorithm
    ///
    /// @param[in]  pieceBit       Square set containing only the attacking piece
    /// @param[in]  occupancyMask  Occupied squares
    /// @param[in]  rayMaskEx      Forward/backward attack ray excluding the attacking piece.
    ///                            For example, the file of the rook not including the rook.
    /// @return                    Attacked squares
    ///
    /// **Illustration**
    /// <table>
    /// <tr>
    ///   <th>Input</th>
    ///   <th>Return</th>
    /// </tr>
    /// <tr>
    ///   <td>
    /// <table class="bitboard">
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td> </td><td>o</td><td> </td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             >R</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td>o</td><td> </td><td>o</td><td> </td><td>o</td></tr>
    /// </table>
    //    </td>
    ///   <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             > </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///   </td>
    /// </tr>
    /// <tr>
    ///   <td>@c o = occupied square<br>
    ///       shaded squares = @c rayMaskEx<br>
    ///       @c R = attacker</td>
    ///   <td>Squares marked with @c x = return</td>
    /// </tr>
    /// </table>
    ///
    /// Design
    /// ------
    /// The implementation uses a variation of the "o^(o-2r)" trick. First, the occupancy mask is AND'd
    /// with the ray mask.
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             > </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// Then, two copies are taken: one as is (left side) and one mirrored vertically (right side):
    /// <table>
    /// <tr>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             > </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             > </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// </tr>
    /// </table>
    /// The attacking piece is subtracted from the boards. Note that the attacking piece bit is also flipped vertically in the right-side board.
    /// <table>
    /// <tr>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td             >o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td             >o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// </tr>
    /// </table>
    /// The right-side board is flipped vertically again.
    /// <table>
    /// <tr>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td             >o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// <td>
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask">o</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td             >o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">o</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    /// </td>
    /// </tr>
    /// </table>
    /// The left-side and right-side boards are then XOR'd together.
    /// <table class="bitboard">
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td> </td><td> </td><td class="mask"> </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">x</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td> </td><td> </td><td             > </td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">x</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">x</td><td>o</td><td>o</td><td>o</td><td>o</td><td>o</td></tr>
    /// <tr><td>o</td><td>o</td><td class="mask">x</td><td> </td><td> </td><td> </td><td> </td><td> </td></tr>
    /// </table>
    ///
    /// and the
    /// result is AND'd with the ray mask. This gives the final result.
    ///
    /// The XOR operation isolates the bits that change in the subtraction
    /// operation. This is the reason why this algorithm works.
    ///
    /// @sa https://www.chessprogramming.org/Hyperbola_Quintessence
    static inline SquareSet getSliderAttackMaskHyperbola(
        SquareSet pieceBit, SquareSet occupancyMask, SquareSet rayMaskEx) noexcept;

public:

    static inline SquareSet getPawnAttackMask(Square sq, Color pawnColor) noexcept
    {
        return SquareSet {
            turnSpecificArrayLookup(
                ctBitBoardTables.pawnAttackMasks,
                static_cast<std::size_t>(sq), pawnColor) };
    }

    static inline SquareSet getPawnAttackerMask(Square sq, Color pawnColor) noexcept
    {
        static_assert(static_cast<std::uint64_t>(Color::WHITE) == 0U);
        static_assert(static_cast<std::uint64_t>(Color::BLACK) == 8U);

        return SquareSet {
            turnSpecificArrayLookup(
                ctBitBoardTables.pawnAttackMasks,
                static_cast<std::size_t>(sq), oppositeColor(pawnColor)) };
    }

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

    static inline SquareSet getKnightAttackMask(Square sq) noexcept
    {
        return SquareSet { ctBitBoardTables.knightAttackMasks[getIndexOfSquare(sq)] };
    }

    static inline SquareSet getKingAttackMask(Square sq) noexcept
    {
        return SquareSet { ctBitBoardTables.kingAttackMasks[getIndexOfSquare(sq)] };
    }

    /// @brief See @coderef{Attacks::getBishopAttackMask()} for usage documentation
    ///
    /// @param[in]  sq             Bishop square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of attacked squares
    ///
    /// @coderef{getSliderAttackMaskHyperbola()} is used to implement this function.
    static inline SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief See @coderef{Attacks::getRookAttackMask()} for usage documentation
    ///
    /// @param[in]  sq             Rook square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of attacked squares
    ///
    /// @coderef{getSliderAttackMaskHyperbola()} is used to implement the
    /// vertical attack mask resolution.
    ///
    /// The horizontal attack mask is resolved as follows:
    /// -# Determine row and column of the attacking piece @c sq
    /// -# Determine shift for translating the row to the first rank (@c row * 8U)
    /// -# Shift the piece rank to first rank
    /// -# Use a precomputed table mapping the first rank occupancy and piece column
    ///    to attack squares
    /// -# Shift the attack squares back to piece row
    static inline SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief Returns horizontal rook attack mask
    ///
    /// @param[in]  sq             Rook square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of horizontally attacked squares
    static inline SquareSet getHorizRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
    {
        // horizontal attacks
        const SquareUnderlyingType rankShift = static_cast<SquareUnderlyingType>(sq) & 56U;
        const SquareUnderlyingType sqColumn = static_cast<SquareUnderlyingType>(sq) & 7U;
        const std::uint64_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift) & 0xFFU;

        return SquareSet { ctBitBoardTables.rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift;
    }
};

#if (BITBOARD_TABLES_HAVE_HYPERBOLA)
SquareSet Attacks_Portable::getSliderAttackMaskHyperbola(
    SquareSet pieceBit, SquareSet occupancyMask, SquareSet rayMaskEx) noexcept
{
    std::uint64_t forward { static_cast<std::uint64_t>(occupancyMask & rayMaskEx) };
    std::uint64_t reverse { static_cast<std::uint64_t>(SquareSet { forward }.flipVert()) };

    forward = forward - static_cast<std::uint64_t>(pieceBit);
    reverse = reverse - static_cast<std::uint64_t>(pieceBit.flipVert());

    return (SquareSet { forward } ^ SquareSet { reverse }.flipVert()) & rayMaskEx;
}

SquareSet Attacks_Portable::getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    const SquareSet pieceBit { ctBitBoardTables.hyperbolaAttackMasks[getIndexOfSquare(sq)].sqBit };

    return
        getSliderAttackMaskHyperbola(
            pieceBit, occupancyMask, SquareSet { ctBitBoardTables.hyperbolaAttackMasks[getIndexOfSquare(sq)].diagBLTREx }) |
        getSliderAttackMaskHyperbola(
            pieceBit, occupancyMask, SquareSet { ctBitBoardTables.hyperbolaAttackMasks[getIndexOfSquare(sq)].diagBRTLEx });
}

SquareSet Attacks_Portable::getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    // vertical attacks
    const SquareSet pieceBit { ctBitBoardTables.hyperbolaAttackMasks[getIndexOfSquare(sq)].sqBit };
    const SquareSet vertAttacks {
        getSliderAttackMaskHyperbola(
            pieceBit, occupancyMask, SquareSet { ctBitBoardTables.hyperbolaAttackMasks[getIndexOfSquare(sq)].vertMaskEx }) };

    // horizontal attacks
    const SquareUnderlyingType rankShift = static_cast<SquareUnderlyingType>(sq) & 56U;
    const SquareUnderlyingType sqColumn = static_cast<SquareUnderlyingType>(sq) & 7U;
    const std::uint64_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift) & 0xFFU;

    return vertAttacks | (SquareSet { ctBitBoardTables.rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift);
}
#endif

}

#endif
