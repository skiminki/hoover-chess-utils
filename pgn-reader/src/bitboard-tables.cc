// Hoover Chess Utilities / PGN reader
// Copyright (C) 2026  Sami Kiminki
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

#include "bitboard-tables.h"
#include "chessboard-types.h"
#include "chessboard-types-squareset.h"

#include <bit>

namespace hoover_chess_utils::pgn_reader
{

namespace
{

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

template <typename T, std::size_t N>
consteval std::array<T, N * 2U> interleaveArrays2(const std::array<T, N> &lhs, const std::array<T, N> &rhs) noexcept
{
    std::array<T, N * 2U> ret { };

    for (std::size_t i { }; i < N; ++i)
    {
        ret[ 2U * i      ] = lhs[i];
        ret[(2U * i) + 1U] = rhs[i];
    }

    return ret;
}


consteval bool isRay(std::int8_t dx, std::int8_t dy) noexcept
{
    return (dx == 0 || dy == 0 || dx == dy || dx == -dy);
}

consteval std::int8_t clampDifference(std::int8_t diff) noexcept
{
    if (diff < 0)
        return -1;
    if (diff > 0)
        return +1;

    return 0;
}

consteval auto computeIntercept(Square kingSq, Square checkerSq) noexcept
{
    if (kingSq == checkerSq)
        return SquareSet { };

    const std::int8_t dx = columnOf(checkerSq) - columnOf(kingSq);
    const std::int8_t dy = rowOf(checkerSq) - rowOf(kingSq);

    if (isRay(dx, dy))
    {
        const std::int8_t stepx = clampDifference(dx);
        const std::int8_t stepy = clampDifference(dy);
        const std::int8_t shift = stepy * 8 + stepx;

        Square sq { kingSq };
        SquareSet intercept { };

        do
        {
            sq = addToSquareNoOverflowCheck(sq, shift);

            intercept |= SquareSet { sq };
        }
        while (sq != checkerSq);

        return intercept;
    }
    else
    {
        // knight jump or garbage
        return SquareSet { checkerSq };
    }
}

static_assert(computeIntercept(Square::A1, Square::A1) == SquareSet { });
static_assert(computeIntercept(Square::A1, Square::A2) == SquareSet { Square::A2 });
static_assert(computeIntercept(Square::A1, Square::A8) == SquareSet { 0x01'01'01'01'01'01'01'00U } );
static_assert(computeIntercept(Square::A1, Square::H8) == SquareSet { 0x80'40'20'10'08'04'02'00U } );
static_assert(computeIntercept(Square::A1, Square::B3) == SquareSet { Square::B3 });

consteval auto computeRay(Square kingSq, Square pinnedSq) noexcept
{
    if (kingSq == pinnedSq)
        return SquareSet { };

    const std::int8_t dx = columnOf(pinnedSq) - columnOf(kingSq);
    const std::int8_t dy = rowOf(pinnedSq) - rowOf(kingSq);

    if (isRay(dx, dy))
    {
        const std::int8_t stepx = clampDifference(dx);
        const std::int8_t stepy = clampDifference(dy);

        SquareSet ray { };

        std::int8_t x = columnOf(kingSq);
        std::int8_t y = rowOf(kingSq);

        while (true)
        {
            x += stepx;
            y += stepy;

            if (x < 0 || x >= 8 || y < 0 || y >= 8)
                return ray;

            ray |= SquareSet::square(x, y);
        }
    }
    else
    {
        // not on the same horiz/vert/diag axis. Return empty set (will be
        // asserted if used).
        return SquareSet { };
    }
}

consteval auto computeInterceptsTable() noexcept
{
    std::array<std::array<std::uint64_t, 64U>, 65U> ret { };

    for (std::size_t kingSq { }; kingSq < 64U; ++kingSq)
    {
        for (std::size_t checkerSq { }; checkerSq < 64U; ++checkerSq)
            ret[checkerSq][kingSq] =
                static_cast<std::uint64_t>(computeIntercept(getSquareForIndex(kingSq), getSquareForIndex(checkerSq)));

        ret[64U][kingSq] = static_cast<std::uint64_t>(SquareSet::all());
    }


    return ret;
}

consteval auto computeRaysFromKingTable() noexcept
{
    std::array<std::array<std::uint64_t, 64U>, 64U> ret { };

    for (std::size_t kingSq { }; kingSq < 64U; ++kingSq)
        for (std::size_t pinnedSq { }; pinnedSq < 64U; ++pinnedSq)
            ret[kingSq][pinnedSq] =
                static_cast<std::uint64_t>(computeRay(getSquareForIndex(kingSq), getSquareForIndex(pinnedSq)));

    return ret;
}

[[maybe_unused]]
consteval auto generatePextRookMasks() noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        SquareSet mask {
            SquareSet::column(col) ^
            SquareSet::row(row)
        };

        // remove tips of the attack rays
        mask &=
            ~(SquareSet::square(col, 0U) | SquareSet::square(col, 7U) |
              SquareSet::square(0U, row) | SquareSet::square(7U, row));

        ret[sq] = static_cast<std::uint64_t>(mask);
    }

    return ret;
}

[[maybe_unused]]
consteval auto generatePextBishopMasks() noexcept
{
    std::array<std::uint64_t, 64U> ret { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        const SquareSet diagBLTR { 0x80'40'20'10'08'04'02'01 };
        const std::int8_t shiftBLTR { static_cast<std::int8_t>(col - row) };
        const SquareSet diagBLTRShifted { (shiftBLTR >= 0) ? diagBLTR >> (shiftBLTR * 8) : diagBLTR << (-shiftBLTR) * 8 };

        const SquareSet diagBRTL { 0x01'02'04'08'10'20'40'80 };
        const std::int8_t shiftBRTL { static_cast<std::int8_t>(7U - col - row) };
        const SquareSet diagBRTLShifted { (shiftBRTL >= 0) ? diagBRTL >> (shiftBRTL * 8) : diagBRTL << (-shiftBRTL) * 8 };

        SquareSet mask { diagBLTRShifted ^ diagBRTLShifted };

        // remove borders
        mask &= ~(SquareSet::column(0U) | SquareSet::column(7) | SquareSet::row(0U) | SquareSet::row(7U));

        ret[sq] = static_cast<std::uint64_t>(mask);
    }

    return ret;
}

template <typename OffsetType>
[[maybe_unused]]
consteval auto generatePextOffsets(const std::array<std::uint64_t, 64U> &masks) noexcept
{
    std::array<OffsetType, 64U> ret { };
    std::uint32_t offset { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
    {
        ret[sq] = offset;

        offset += std::uint32_t { 1 } << std::popcount(masks[sq]);
    }

    return ret;
}

[[maybe_unused]]
consteval auto calculatePextDataSize(const std::array<std::uint64_t, 64U> &masks) noexcept
{
    std::uint32_t offset { };

    for (std::uint8_t sq { }; sq < 64U; ++sq)
        offset += std::uint32_t { 1 } << std::popcount(masks[sq]);

    return offset;
}

template <typename T, typename U, std::size_t N>
[[maybe_unused]]
consteval std::array<T, N> addConstant(const std::array<T, N> &lhs, const U rhs) noexcept
{
    std::array<T, N> ret { };

    for (std::size_t i { }; i < N; ++i)
    {
        ret[i] = lhs[i] + rhs;
    }

    return ret;
}

[[maybe_unused]]
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

#if (BITBOARD_TABLES_HAVE_HYPERBOLA)
consteval auto generateHyperbolaAttackMasks() noexcept
{
    std::array<BitBoardTables::HyperbolaAttackMasks, 64U> ret { };

    for (std::uint8_t sqIndex { }; sqIndex < 64U; ++sqIndex)
    {
        const Square sq { sqIndex };
        const SquareSet sqBit { sq };
        const std::uint8_t col { columnOf(Square { sq }) };
        const std::uint8_t row { rowOf(Square { sq }) };

        ret[sqIndex].sqBit = static_cast<std::uint64_t>(sqBit);
        ret[sqIndex].vertMaskEx = static_cast<std::uint64_t>(SquareSet::column(col) ^ sqBit);

        const SquareSet diagBLTR { 0x80'40'20'10'08'04'02'01 };
        const std::int8_t shiftBLTR { static_cast<std::int8_t>(col - row) };
        const SquareSet diagBLTRShifted { (shiftBLTR >= 0) ? diagBLTR >> (shiftBLTR * 8) : diagBLTR << (-shiftBLTR) * 8 };

        const SquareSet diagBRTL { 0x01'02'04'08'10'20'40'80 };
        const std::int8_t shiftBRTL { static_cast<std::int8_t>(7U - col - row) };
        const SquareSet diagBRTLShifted { (shiftBRTL >= 0) ? diagBRTL >> (shiftBRTL * 8) : diagBRTL << (-shiftBRTL) * 8 };

        ret[sqIndex].diagBLTREx = static_cast<std::uint64_t>(diagBLTRShifted ^ sqBit);
        ret[sqIndex].diagBRTLEx = static_cast<std::uint64_t>(diagBRTLShifted ^ sqBit);
    }

    return ret;
}
#endif

template <typename ValueType, std::size_t N>
consteval auto
offsetsToPointers(const ValueType *base, const std::array<ValueType, N> &offsets) noexcept
{
    std::array<const ValueType *, N> ret { };

    for (std::size_t i { }; i < N; ++i)
        ret[i] = &base[offsets[i]];

    return ret;
}

}

const BitBoardTables ctBitBoardTables
{
    // pawnAttackMasks
    interleaveArrays(generateWhitePawnAttackMaskTable(), generateBlackPawnAttackMaskTable()),

    // knightAttackMasks
    generateKnightAttackMaskTable(),

    // kingAttackMasks
    generateKingAttackMaskTable(),

    // rayIntercepts
    computeInterceptsTable(),

    // raysFromKing
    computeRaysFromKingTable(),

    // rookHorizAttackMasks
    generateRookHorizAttackMasks(),

#if (BITBOARD_TABLES_HAVE_X86_BMI2)
    // bmi2BishopMasks
    generatePextBishopMasks(),

    // bmi2BishopOffsets
    {
        offsetsToPointers(
            ctBitBoardTables.bmi2BishopRookAttackData,
            generatePextOffsets<std::uint64_t>(generatePextBishopMasks()))
    },

    // bmi2RookMasks
    generatePextRookMasks(),

    // bmi2RookOffsets
    {
        offsetsToPointers(
            ctBitBoardTables.bmi2BishopRookAttackData,
            addConstant(generatePextOffsets<std::uint64_t>(generatePextRookMasks()), static_cast<std::uint64_t>(5248U)))
    },

    // bmi2BishopRookAttackData
    {
#include "slider-attacks-pext-pdep-bishop.inc"
#include "slider-attacks-pext-pdep-rook.inc"
    },
#endif

#if (BITBOARD_TABLES_HAVE_ELEMENTARY)

    // elementaryBishopMaskMults
    {
        {
#include "slider-attacks-elementary-bishop.inc"
        },
    },

    // elementaryBishopOffsets
    generatePextOffsets<std::uint32_t>(generatePextBishopMasks()),

    // elementaryRookMaskMults
    {
        {
#include "slider-attacks-elementary-rook.inc"
        },
    },

    // elementaryRookOffsets
    addConstant(generatePextOffsets<std::uint32_t>(generatePextRookMasks()), static_cast<std::uint32_t>(5248U)),

    // elementaryBishopRookAttackData
    {
#include "slider-attacks-pext-pdep-bishop.inc"
#include "slider-attacks-pext-pdep-rook.inc"
    },
#endif

#if (BITBOARD_TABLES_HAVE_AARCH64_SVE2_BITPERM)

    // sve2BishopRookMasks
    interleaveArrays2(generatePextBishopMasks(), generatePextRookMasks()),

    // sve2BishopRookOffsets
    interleaveArrays2(
        generatePextOffsets<std::uint64_t>(generatePextBishopMasks()),
        addConstant(generatePextOffsets<std::uint64_t>(generatePextRookMasks()), static_cast<std::uint64_t>(5248U))),

    // sve2BishopRookAttackData
    {
#include "slider-attacks-pext-pdep-bishop.inc"
#include "slider-attacks-pext-pdep-rook.inc"
    },

#endif

#if (BITBOARD_TABLES_HAVE_HYPERBOLA)

    // hyperbolaAttackMasks
    generateHyperbolaAttackMasks(),
#endif

#if (BITBOARD_TABLES_HAVE_BLACK_MAGIC)
    // blackMagicAttacks
    {
#include "slider-attacks-black-magic-lookup.inc"
    },
    // blackMagicBishopMagics
    {{
#include "slider-attacks-black-magic-bishop-magics.inc"
    }},
    // blackMagicRookMagics
    {{
#include "slider-attacks-black-magic-rook-magics.inc"
    }},
#endif
};

#if (BITBOARD_TABLES_HAVE_BLACK_MAGIC)
BitBoardTables::BlackMagicData::BlackMagicData(
    std::uint64_t _attacksOffset,
    std::uint64_t _mask,
    std::uint64_t _hash) noexcept :
    attacksBase { &ctBitBoardTables.blackMagicAttacks[_attacksOffset] },
    mask { _mask },
    hash { _hash }
{
}
#endif

}
