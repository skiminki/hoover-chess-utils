// Hoover Chess Utilities / PGN reader
// Copyright (C) 2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_X86_AVX512F_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__BITBOARD_ATTACKS_X86_AVX512F_H_INCLUDED

#include "chessboard-types-squareset.h"

#include <immintrin.h>
#include <array>
#include <cstdint>

namespace hoover_chess_utils::pgn_reader
{

class Attacks_AVX512F
{
public:

    alignas(__m128i) static constexpr std::array<std::uint64_t, 2U> ctAttackingPawnMasks {
        static_cast<std::uint64_t>(~SquareSet::column(0)), // captures to left
        static_cast<std::uint64_t>(~SquareSet::column(7)), // captures to right
    };

    alignas(__m128i) static constexpr std::array<std::int64_t, 4U> ctAttackingPawnRotateLefts {
        -9, // captures to left,  black pawn
        -7, // captures to right, black pawn
        +7, // captures to left,  white pawn
        +9, // captures to right, white pawn
    };

    // the order of knights:
    // -2, +1  (two left, one up)
    // -1, +2
    // +1, +2
    // +2, +1
    // -2, -1
    // -1, -2
    // +1, -2
    // +2, -1
    alignas(__m512i) static constexpr std::array<std::uint64_t, 8U> ctAttackingKnightMasks {
        static_cast<std::uint64_t>(    ~(SquareSet::column(0U) | SquareSet::column(1U) | SquareSet::row(7U)                      )),
            static_cast<std::uint64_t>(~(SquareSet::column(0U)                         | SquareSet::row(7U) | SquareSet::row(6U) )),
            static_cast<std::uint64_t>(~(SquareSet::column(7U)                         | SquareSet::row(7U) | SquareSet::row(6U) )),
            static_cast<std::uint64_t>(~(SquareSet::column(7U) | SquareSet::column(6U) | SquareSet::row(7U)                      )),
            static_cast<std::uint64_t>(~(SquareSet::column(0U) | SquareSet::column(1U) | SquareSet::row(0U)                      )),
            static_cast<std::uint64_t>(~(SquareSet::column(0U)                         | SquareSet::row(0U) | SquareSet::row(1U) )),
            static_cast<std::uint64_t>(~(SquareSet::column(7U)                         | SquareSet::row(0U) | SquareSet::row(1U) )),
            static_cast<std::uint64_t>(~(SquareSet::column(7U) | SquareSet::column(6U) | SquareSet::row(0U)                      )),
    };

    alignas(__m512i) static constexpr std::array<std::int64_t, 8U> ctAttackingKnightRotateLefts {
        -2 + 8 ,
        -1 + 16,
        +1 + 16,
        +2 + 8 ,
        -2 - 8 ,
        -1 - 16,
        +1 - 16,
        +2 - 8 ,
    };

    // the order of sliders:
    // - bishop advancing top-left
    // - bishop advancing top-right
    // - bishop advancing bottom-left
    // - bishop advancing bottom-right
    // - rook advancing top
    // - rook bishop advancing bottom
    // - rook advancing left
    // - rook advancing right
    alignas(__m512i) static constexpr std::array<std::uint64_t, 8U> ctAttackingSliderMasks {
        static_cast<std::uint64_t>(~(SquareSet::row(7U) | SquareSet::column(0U))),
        static_cast<std::uint64_t>(~(SquareSet::row(7U) | SquareSet::column(7U))),
        static_cast<std::uint64_t>(~(SquareSet::row(0U) | SquareSet::column(0U))),
        static_cast<std::uint64_t>(~(SquareSet::row(0U) | SquareSet::column(7U))),
        static_cast<std::uint64_t>(~SquareSet::row(7U)),
        static_cast<std::uint64_t>(~SquareSet::row(0U)),
        static_cast<std::uint64_t>(~SquareSet::column(0U)),
        static_cast<std::uint64_t>(~SquareSet::column(7U)),
    };

    alignas(__m512i) static constexpr std::array<std::int64_t, 8U> ctAttackingSliderRotateLefts {
        +7,
        +9,
        -9,
        -7,
        +8,
        -8,
        -1,
        +1
    };

#if HAVE_X86_AVX512VL
    static inline __m128i getAttackedSquaresByPawns128(const SquareSet pawns, Color oppPawnColor) noexcept
    {
        static_assert(static_cast<std::int8_t>(Color::WHITE) == 0);
        static_assert(static_cast<std::int8_t>(Color::BLACK) == 8);

        const std::uint8_t *rotateData { std::bit_cast<const std::uint8_t *>(ctAttackingPawnRotateLefts.data()) };

        const __m128i rotateLefts { _mm_load_epi64(rotateData + 2U * static_cast<std::size_t>(oppPawnColor)) };
        const __m128i attackingPawnMasks { _mm_load_epi64(ctAttackingPawnMasks.data()) };

        __m128i attackingPawns { _mm_set1_epi64x(static_cast<std::uint64_t>(pawns)) };
        attackingPawns &= attackingPawnMasks;
        return _mm_rolv_epi64(attackingPawns, rotateLefts);
    }
#else
    static inline __m512i getAttackedSquaresByPawns512(const SquareSet pawns, Color oppPawnColor) noexcept
    {
        static_assert(static_cast<std::int8_t>(Color::WHITE) == 0);
        static_assert(static_cast<std::int8_t>(Color::BLACK) == 8);

        const std::uint8_t *rotateData { std::bit_cast<const std::uint8_t *>(ctAttackingPawnRotateLefts.data()) };

        const __m128i rotateLefts { _mm_load_si128(std::bit_cast<const __m128i *>(rotateData + 2U * static_cast<std::size_t>(oppPawnColor))) };
        const __m128i attackingPawnMasks { _mm_load_si128(std::bit_cast<const __m128i *>(ctAttackingPawnMasks.data())) };

        __m128i attackingPawns { _mm_set1_epi64x(static_cast<std::uint64_t>(pawns)) };
        attackingPawns &= attackingPawnMasks;
        return _mm512_rolv_epi64(_mm512_zextsi128_si512(attackingPawns), _mm512_zextsi128_si512(rotateLefts));
    }
#endif

    static inline __m512i getAttackedSquaresByKnights(const SquareSet knights) noexcept
    {
        const __m512i rotateLefts = _mm512_load_epi64(ctAttackingKnightRotateLefts.data());
        const __m512i attackingKnightMasks =  _mm512_load_epi64(ctAttackingKnightMasks.data());

        __m512i attackingKnights { _mm512_set1_epi64(static_cast<std::uint64_t>(knights)) };
        attackingKnights &= attackingKnightMasks;
        return _mm512_rolv_epi64(attackingKnights, rotateLefts);
    }

    static inline __m512i getAttackedSquaresBySliders(__m512i attacks, const SquareSet bishops, const SquareSet rooks, const SquareSet occupancyMask) noexcept
    {
        if ((bishops | rooks) == SquareSet::none())
            return attacks;

        const __m512i rotateLefts = _mm512_load_epi64(ctAttackingSliderRotateLefts.data());
        const __m512i attackingSliderMasks =  _mm512_load_epi64(ctAttackingSliderMasks.data());

        __m512i attackingSliders { };

        attackingSliders =
            _mm512_mask_set1_epi64(
                _mm512_set1_epi64(static_cast<std::uint64_t>(bishops)),
                0xF0,
                static_cast<std::uint64_t>(rooks));

        const __m512i occupancyMasksNegated = _mm512_set1_epi64(static_cast<std::uint64_t>(~occupancyMask));

        attackingSliders &= attackingSliderMasks;

        __mmask8 exitCond;

        do
        {
            attackingSliders  = _mm512_rolv_epi64(attackingSliders, rotateLefts);
            attacks          |= attackingSliders;
            attackingSliders  = attackingSliders & occupancyMasksNegated;
            attackingSliders &= attackingSliderMasks;

            attackingSliders  = _mm512_rolv_epi64(attackingSliders, rotateLefts);
            attacks          |= attackingSliders;
            attackingSliders  = attackingSliders & occupancyMasksNegated;

            exitCond = _mm512_test_epi64_mask(attackingSliders, attackingSliderMasks);

            attackingSliders &= attackingSliderMasks;
        }
        while (exitCond != 0U);

        return attacks;
    }

    static SquareSet determineAttackedSquares(
        SquareSet occupancyMask,
        SquareSet pawns,
        SquareSet knights,
        SquareSet bishops,
        SquareSet rooks,
        Square king,
        Color turn) noexcept
    {
        // add knight attacks
        __m512i attacks {
#if !HAVE_X86_AVX512VL
            getAttackedSquaresByPawns512(pawns, turn) |
#endif
            getAttackedSquaresByKnights(knights) };

        // add slider attacks
        attacks = getAttackedSquaresBySliders(attacks, bishops, rooks, occupancyMask);

        // reduce 8x 64b attack tables to 4x 64b
        const __m256i attacks256 {
            _mm512_extracti64x4_epi64(attacks, 1U) |
            _mm512_castsi512_si256(attacks) };

        // reduce 4x 64b attack tables to 2x 64b & add pawn attacks
        const __m128i attacks128 {
#if HAVE_X86_AVX512VL
            getAttackedSquaresByPawns128(pawns, turn) |
#endif
            _mm256_extracti128_si256(attacks256, 1U) |
            _mm256_castsi256_si128(attacks256)  };

        // reduce 2x 64 to 64b
        const std::uint64_t attacks64 =
            _mm_cvtsi128_si64(
                _mm_bsrli_si128(attacks128, 8U) |
                attacks128);

        // ... and add king attacks
        return
            SquareSet { attacks64 } |
            Attacks_Portable::getKingAttackMask(king);
    }

#if 0 // note: this is quite a lot slower than the regular PEXT/PDEP implementation
    static inline void determineSliderCheckersAndPins(
        SquareSet occupancyMask,
        SquareSet turnColorMask,
        SquareSet bishops,
        SquareSet rooks,
        SquareSet epCapturable,
        Square kingSq,
        SquareSet &out_checkers,
        SquareSet &out_pinnedPieces)
    {
        // rays expanding outwards from king towards a checker or possibly pinned piece
        __m512i kingRays { _mm512_set1_epi64(static_cast<std::uint64_t>(SquareSet::square(kingSq))) };

        __m512i oppSliders {
            _mm512_set_epi64(
                static_cast<std::uint64_t>(rooks &~ turnColorMask),
                static_cast<std::uint64_t>(rooks &~ turnColorMask),
                static_cast<std::uint64_t>(rooks &~ turnColorMask),
                static_cast<std::uint64_t>(rooks &~ turnColorMask),
                static_cast<std::uint64_t>(bishops &~ turnColorMask),
                static_cast<std::uint64_t>(bishops &~ turnColorMask),
                static_cast<std::uint64_t>(bishops &~ turnColorMask),
                static_cast<std::uint64_t>(bishops &~ turnColorMask)) };

        // Traces from potentially pinned pieces outwards. Used to commit the pinned piece in
        // case the trace hits an x-ray attacker
        __m512i xrays { };

        // pinned pieces
        __m512i potentiallyPinnedPieces { };
        __m512i pinnedPieces { };

        const __m512i occupancyMasks { _mm512_set1_epi64(static_cast<std::uint64_t>(occupancyMask)) };
        __m512i checkers { };
        const __m512i pinnables {
            _mm512_set_epi64(
                static_cast<std::uint64_t>(turnColorMask),
                static_cast<std::uint64_t>(turnColorMask),
                static_cast<std::uint64_t>(turnColorMask),
                static_cast<std::uint64_t>(turnColorMask),
                static_cast<std::uint64_t>(turnColorMask | epCapturable),
                static_cast<std::uint64_t>(turnColorMask | epCapturable),
                static_cast<std::uint64_t>(turnColorMask | epCapturable),
                static_cast<std::uint64_t>(turnColorMask | epCapturable)) };

        const __m512i rotateLefts = _mm512_load_epi64(ctAttackingSliderRotateLefts.data());
        const __m512i attackingSliderMasks =  _mm512_load_epi64(ctAttackingSliderMasks.data());

        // Filter out king rays about to go out of board
        kingRays &= attackingSliderMasks;

        while (true)
        {
            // expand king rays by a square. These are guaranteed to be on the board after expansion
            kingRays = _mm512_rolv_epi64(kingRays, rotateLefts);

            // add checkers if king rays meet opponent's sliders
            checkers |= kingRays & oppSliders;

            // king ray becomes an x-ray if it meets a pinnable piece
            xrays |= kingRays & pinnables;

            // filter out king rays that cannot expand anymore
            kingRays &= attackingSliderMasks;

            // terminate king rays on any piece
            kingRays &= ~occupancyMasks;

            // viable king rays left?
            if (_mm512_test_epi64_mask(kingRays, kingRays) == 0U)
                break;
        }

        potentiallyPinnedPieces = xrays; // potentially pinned, will resolve later

        while (true)
        {
            // filter out x-rays that are about to go oob
            xrays &= attackingSliderMasks;

            // viable x-rays left?
            if (_mm512_test_epi64_mask(xrays, xrays) == 0U)
                break;

            // expand x-rays
            xrays = _mm512_rolv_epi64(xrays, rotateLefts);

            // if an x-ray hits an opponent slider, that slider is a pinner
            const __mmask8 commitPinnedPieces { _mm512_test_epi64_mask(xrays, oppSliders) };
            pinnedPieces |= _mm512_maskz_mov_epi64(commitPinnedPieces, potentiallyPinnedPieces);

            // x-rays terminate on any piece
            xrays &= ~occupancyMasks;
        }

        out_checkers = SquareSet { static_cast<std::uint64_t>(_mm512_reduce_or_epi64(checkers)) };
        out_pinnedPieces = SquareSet { static_cast<std::uint64_t>(_mm512_reduce_or_epi64(pinnedPieces)) };
    }
#endif

};

}

#endif
