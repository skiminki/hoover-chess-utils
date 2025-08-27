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

#include "slider-attacks.h"

#include "chessboard-types.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <cinttypes>


namespace hoover_chess_utils::pgn_reader
{

namespace
{

struct AttackMasks
{
    SquareSet sqBit;
    SquareSet vertMaskEx; // vertical column minus square
    SquareSet diagBLTREx; // diagonal minus square
    SquareSet diagBRTLEx; // anti-diagonal minus square
};

consteval auto generateAttackMasks() noexcept
{
    std::array<AttackMasks, 64U> ret { };

    for (std::uint8_t sqIndex { }; sqIndex < 64U; ++sqIndex)
    {
        const Square sq { sqIndex };
        const SquareSet sqBit { SquareSet::square(sq) };
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        ret[sqIndex].sqBit = sqBit;
        ret[sqIndex].vertMaskEx = SquareSet::column(col) ^ sqBit;

        const SquareSet diagBLTR { 0x80'40'20'10'08'04'02'01 };
        const std::int8_t shiftBLTR { static_cast<std::int8_t>(col - row) };
        const SquareSet diagBLTRShifted { (shiftBLTR >= 0) ? diagBLTR >> (shiftBLTR * 8) : diagBLTR << (-shiftBLTR) * 8 };

        const SquareSet diagBRTL { 0x01'02'04'08'10'20'40'80 };
        const std::int8_t shiftBRTL { static_cast<std::int8_t>(7U - col - row) };
        const SquareSet diagBRTLShifted { (shiftBRTL >= 0) ? diagBRTL >> (shiftBRTL * 8) : diagBRTL << (-shiftBRTL) * 8 };

        ret[sqIndex].diagBLTREx = diagBLTRShifted ^ sqBit;
        ret[sqIndex].diagBRTLEx = diagBRTLShifted ^ sqBit;
    }

    return ret;
}

consteval auto generateRookHorizAttackMasks() noexcept
{
    std::array<std::array<std::uint8_t, 8U>, 256U> ret { };

    for (std::uint8_t rookCol { }; rookCol < 8U; ++rookCol)
        for (std::uint16_t rankIndex { }; rankIndex < 256U; ++rankIndex)
        {
            std::uint8_t rankBits = rankIndex;

            std::uint8_t mask { };

            for (std::uint8_t c = rookCol + 1U; c < 8U; ++c)
            {
                const std::uint8_t maskBit = 1U << c;
                mask |= maskBit;

                if (rankBits & maskBit)
                    break;
            }

            for (std::uint8_t c = rookCol; c > 0U; )
            {
                --c;

                const std::uint8_t maskBit = 1U << c;
                mask |= maskBit;

                if (rankBits & maskBit)
                    break;
            }

            ret[rankIndex][rookCol] = mask;
        }

    return ret;
}

constexpr std::array<AttackMasks, 64U> attackMasks { generateAttackMasks() };
constexpr std::array<std::array<std::uint8_t, 8U>, 256U> rookHorizAttackMasks { generateRookHorizAttackMasks() };

}

SquareSet SliderAttacksGeneric::getSliderAttackMaskHyperbola(
    SquareSet pieceBit, SquareSet occupancyMask, SquareSet rayMaskEx) noexcept
{
    SquareSet forward { occupancyMask & rayMaskEx };
    SquareSet reverse { forward.flipVert() };

    forward  = forward - pieceBit;
    reverse  = reverse - pieceBit.flipVert();

    return (forward ^ reverse.flipVert()) & rayMaskEx;
}

// get mask of rook attacks/moves on populated board
SquareSet SliderAttacksGeneric::getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    const SquareSet pieceBit { attackMasks[getIndexOfSquare(sq)].sqBit };

    return
        getSliderAttackMaskHyperbola(pieceBit, occupancyMask, attackMasks[getIndexOfSquare(sq)].diagBLTREx) |
        getSliderAttackMaskHyperbola(pieceBit, occupancyMask, attackMasks[getIndexOfSquare(sq)].diagBRTLEx);
}

// get mask of rook attacks/moves on populated board
SquareSet SliderAttacksGeneric::getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    // vertical attacks
    const SquareSet pieceBit { attackMasks[getIndexOfSquare(sq)].sqBit };
    const SquareSet vertAttacks { getSliderAttackMaskHyperbola(pieceBit, occupancyMask, attackMasks[getIndexOfSquare(sq)].vertMaskEx) };

    // horizontal attacks
    const std::uint8_t rankShift = static_cast<std::uint8_t>(sq) & 56U;
    const std::uint8_t sqColumn = static_cast<std::uint8_t>(sq) & 7U;
    const std::uint8_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift);

    return vertAttacks | (SquareSet { rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift);
}

SquareSet SliderAttacksGeneric::getHorizRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    // horizontal attacks
    const std::uint8_t rankShift = static_cast<std::uint8_t>(sq) & 56U;
    const std::uint8_t sqColumn = static_cast<std::uint8_t>(sq) & 7U;
    const std::uint8_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift);

    return SquareSet { rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift;
}

}
