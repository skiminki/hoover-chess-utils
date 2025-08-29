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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_H_INCLUDED

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderImpl
/// @brief Generic portable slider attacks implementation
class SliderAttacksGeneric
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
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for usage documentation
    ///
    /// @param[in]  sq             Bishop square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of attacked squares
    ///
    /// @coderef{getSliderAttackMaskHyperbola()} is used to implement this function.
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept;

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
    static SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief Returns horizontal rook attack mask
    ///
    /// @param[in]  sq             Rook square
    /// @param[in]  occupancyMask  Set of occupied squares
    /// @return                    Set of horizontally attacked squares
    static SquareSet getHorizRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;
};

/// @ingroup PgnReaderImpl
/// @brief Slider attacks implementation using PEXT/PDEP
class SliderAttacksPextPdep
{
public:
    /// @brief See @coderef{Attacks::getBishopAttackMask()} for documentation
    static SquareSet getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept;

    /// @brief See @coderef{Attacks::getRookAttackMask()} for documentation
    static SquareSet getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept;
};

#if HAVE_PDEP_PEXT

/// @ingroup PgnReaderImpl
/// @brief Type alias (PEXT/PDEP implementation)
using SliderAttacks = SliderAttacksPextPdep;

#else

/// @ingroup PgnReaderImpl
/// @brief Type alias (generic implementation)
using SliderAttacks = SliderAttacksGeneric;

#endif

/// @ingroup PgnReaderImpl
/// @brief Vectorizable slider attacks - generic non-vectorized implementation
class SliderAttacksSimdGeneric
{
public:
    static inline SquareSet getAttackedSquaresBySliders(const SquareSet bishops, const SquareSet rooks, const SquareSet occupancyMask) noexcept
    {
        SquareSet attacks { };

        SQUARESET_ENUMERATE(
            piece,
            bishops,
            attacks |= SliderAttacks::getBishopAttackMask(piece, occupancyMask));

        SQUARESET_ENUMERATE(
            piece,
            rooks,
            attacks |= SliderAttacks::getRookAttackMask(piece, occupancyMask));

        return attacks;
    }
};

/// @ingroup PgnReaderImpl
/// @brief Vectorizable slider attacks - generic non-vectorized implementation
class SliderAttacksSimdAvx512f
{
public:
    static inline SquareSet getAttackedSquaresBySliders(const SquareSet bishops, const SquareSet rooks, const SquareSet occupancyMask) noexcept;
};

#if HAVE_AVX512F

/// @ingroup PgnReaderImpl
/// @brief Type alias (AVX512F implementation)
using SliderAttacksSimd = SliderAttacksSimdAvx512f;

#else

/// @ingroup PgnReaderImpl
/// @brief Type alias (generic implementation)
using SliderAttacksSimd = SliderAttacksSimdGeneric;

#endif

}

#if HAVE_AVX512F
#include "slider-attacks-avx512f.h"
#endif

#endif
