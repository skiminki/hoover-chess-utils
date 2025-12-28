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

#include "bitboard-attacks-portable.h"

#include <bit>

namespace hoover_chess_utils::pgn_reader
{

namespace {

// shift mask with overflow/wraparound prevention
// negative x/y = shift left/up
consteval std::uint64_t shiftMask(std::uint64_t mask, std::int8_t x, std::int8_t y) noexcept
{
    std::uint64_t nonWrappedMask { };

    // on rank 0
    std::uint64_t nonWrappedColumns { };

    // cull off wraparound bits
    if (x >= 0)
    {
        nonWrappedColumns = (UINT64_C(0xFF) << x) & 0xFFU;
    }
    else
    {
        nonWrappedColumns = UINT64_C(0xFF) >> (-x);
    }

    // broadcast row 0 to all rows
    nonWrappedMask =
        (nonWrappedColumns << 0U)  | (nonWrappedColumns << 8U)  | (nonWrappedColumns << 16U) | (nonWrappedColumns << 24U) |
        (nonWrappedColumns << 32U) | (nonWrappedColumns << 40U) | (nonWrappedColumns << 48U) | (nonWrappedColumns << 56U);

    if (y >= 0)
    {
        nonWrappedMask &= UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF) << (y * 8);
    }
    else
    {
        nonWrappedMask &= UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF) >> ((-y) * 8);
    }

    return std::rotl(mask, x + (8 * y)) & nonWrappedMask;
}

static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0,  0) == UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  1,  0) == UINT64_C(0xFE'FE'FE'FE'FE'FE'FE'FE));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF), -1,  0) == UINT64_C(0x7F'7F'7F'7F'7F'7F'7F'7F));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0,  1) == UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'00));
static_assert(shiftMask(UINT64_C(0xFF'FF'FF'FF'FF'FF'FF'FF),  0, -1) == UINT64_C(0x00'FF'FF'FF'FF'FF'FF'FF));

consteval auto generateAttackMaskTableFromPattern(
    std::uint64_t pattern,
    std::uint8_t origX,
    std::uint8_t origY) noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t y { }; y < 8U; ++y)
    {
        for (std::uint8_t x { }; x < 8U; ++x)
        {
            ret[(y * 8) + x] = shiftMask(pattern, x - origX, y - origY);
        }
    }

    return ret;
}

consteval auto generateWhitePawnAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 0U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY + 1U) |
        SquareSet::square(origX + 1U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateBlackPawnAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 1U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 1U) |
        SquareSet::square(origX + 1U, origY - 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateKnightAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 2U };
    constexpr std::uint8_t origY { 2U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 2U) |
        SquareSet::square(origX - 1U, origY + 2U) |
        SquareSet::square(origX - 2U, origY - 1U) |
        SquareSet::square(origX - 2U, origY + 1U) |
        SquareSet::square(origX + 1U, origY - 2U) |
        SquareSet::square(origX + 1U, origY + 2U) |
        SquareSet::square(origX + 2U, origY - 1U) |
        SquareSet::square(origX + 2U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

consteval auto generateKingAttackMaskTable() noexcept
{
    constexpr std::uint8_t origX { 1U };
    constexpr std::uint8_t origY { 1U };

    constexpr std::uint64_t attackPattern {
        SquareSet::square(origX - 1U, origY - 1U) |
        SquareSet::square(origX     , origY - 1U) |
        SquareSet::square(origX + 1U, origY - 1U) |
        SquareSet::square(origX - 1U, origY     ) |
        SquareSet::square(origX + 1U, origY     ) |
        SquareSet::square(origX - 1U, origY + 1U) |
        SquareSet::square(origX     , origY + 1U) |
        SquareSet::square(origX + 1U, origY + 1U)
    };

    return generateAttackMaskTableFromPattern(attackPattern, origX, origY);
}

template <std::size_t N, std::size_t M>
consteval auto concatenateArrays(const std::array<std::uint64_t, N> &first, const std::array<std::uint64_t, M> &second) noexcept
{
    std::array<std::uint64_t, N + M> ret { };

    for (std::size_t i { }; i < N; ++i)
        ret[i] = first[i];

    for (std::size_t i { }; i < M; ++i)
        ret[N + i] = second[i];

    return ret;
}

template <std::size_t N>
consteval auto interleaveArrays(const std::array<std::uint64_t, N> &first, const std::array<std::uint64_t, N> &second) noexcept
{
    std::array<std::array<std::uint64_t, 2U>, N> ret { };

    for (std::size_t i { }; i < N; ++i)
    {
        ret[i][0U] = first[i];
        ret[i][1U] = second[i];
    }

    return ret;
}


struct HyperbolaAttackMasks
{
    SquareSet sqBit;
    SquareSet vertMaskEx; // vertical column minus square
    SquareSet diagBLTREx; // diagonal minus square
    SquareSet diagBRTLEx; // anti-diagonal minus square
};

consteval auto generateAttackMasks() noexcept
{
    std::array<HyperbolaAttackMasks, 64U> ret { };

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

constexpr std::array<HyperbolaAttackMasks, 64U> hyperbolaAttackMasks { generateAttackMasks() };
constexpr std::array<std::array<std::uint8_t, 8U>, 256U> rookHorizAttackMasks { generateRookHorizAttackMasks() };

}

const std::array<std::array<std::uint64_t, 2U>, 64U > Attacks_Portable::ctPawnAttackMaskTable {
    interleaveArrays(generateWhitePawnAttackMaskTable(), generateBlackPawnAttackMaskTable())
};

const std::array<std::uint64_t, 64U> Attacks_Portable::ctKnightAttackMaskTable { generateKnightAttackMaskTable() };
const std::array<std::uint64_t, 64U> Attacks_Portable::ctKingAttackMaskTable { generateKingAttackMaskTable() };

SquareSet Attacks_Portable::getSliderAttackMaskHyperbola(
    SquareSet pieceBit, SquareSet occupancyMask, SquareSet rayMaskEx) noexcept
{
    SquareSet forward { occupancyMask & rayMaskEx };
    SquareSet reverse { forward.flipVert() };

    forward  = forward - pieceBit;
    reverse  = reverse - pieceBit.flipVert();

    return (forward ^ reverse.flipVert()) & rayMaskEx;
}

// get mask of rook attacks/moves on populated board
SquareSet Attacks_Portable::getBishopAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    const SquareSet pieceBit { hyperbolaAttackMasks[getIndexOfSquare(sq)].sqBit };

    return
        getSliderAttackMaskHyperbola(pieceBit, occupancyMask, hyperbolaAttackMasks[getIndexOfSquare(sq)].diagBLTREx) |
        getSliderAttackMaskHyperbola(pieceBit, occupancyMask, hyperbolaAttackMasks[getIndexOfSquare(sq)].diagBRTLEx);
}

// get mask of rook attacks/moves on populated board
SquareSet Attacks_Portable::getRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    // vertical attacks
    const SquareSet pieceBit { hyperbolaAttackMasks[getIndexOfSquare(sq)].sqBit };
    const SquareSet vertAttacks { getSliderAttackMaskHyperbola(pieceBit, occupancyMask, hyperbolaAttackMasks[getIndexOfSquare(sq)].vertMaskEx) };

    // horizontal attacks
    const std::uint8_t rankShift = static_cast<std::uint8_t>(sq) & 56U;
    const std::uint8_t sqColumn = static_cast<std::uint8_t>(sq) & 7U;
    const std::uint8_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift);

    return vertAttacks | (SquareSet { rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift);
}

SquareSet Attacks_Portable::getHorizRookAttackMask(Square sq, SquareSet occupancyMask) noexcept
{
    // horizontal attacks
    const std::uint8_t rankShift = static_cast<std::uint8_t>(sq) & 56U;
    const std::uint8_t sqColumn = static_cast<std::uint8_t>(sq) & 7U;
    const std::uint8_t occupancyShifted = static_cast<std::uint64_t>(occupancyMask >> rankShift);

    return SquareSet { rookHorizAttackMasks[occupancyShifted][sqColumn] } << rankShift;
}

}
