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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_PRIV_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_PRIV_H_INCLUDED

#include "chessboard.h"
#include "chessboard-types.h"


namespace hoover_chess_utils::pgn_reader
{

struct DummyMoveIteratorDereference
{
    DummyMoveIteratorDereference &operator = (Move) noexcept
    {
        return *this;
    }
};

class LegalMoveDetectorIterator
{
private:
    bool m_legalMoves { };

public:
    inline LegalMoveDetectorIterator &operator ++() noexcept
    {
        m_legalMoves = true;
        return *this;
    }

    inline LegalMoveDetectorIterator &operator += (std::uint8_t amount) noexcept
    {
        m_legalMoves = m_legalMoves || (amount != 0U);
        return *this;
    }

    DummyMoveIteratorDereference operator * () noexcept
    {
        return DummyMoveIteratorDereference { };
    }

    bool hasLegalMoves() const noexcept
    {
        return m_legalMoves;
    }
};

class LegalMoveCounterIterator
{
private:
    std::uint8_t m_numLegalMoves { };

public:
    inline LegalMoveCounterIterator &operator ++() noexcept
    {
        ++m_numLegalMoves;
        return *this;
    }

    inline LegalMoveCounterIterator &operator += (std::uint8_t amount) noexcept
    {
        m_numLegalMoves += amount;
        return *this;
    }

    DummyMoveIteratorDereference operator * () noexcept
    {
        return DummyMoveIteratorDereference { };
    }

    std::size_t getNumberOfLegalMoves() const noexcept
    {
        return m_numLegalMoves;
    }
};

template <typename IteratorType>
struct IteratorStoreMoveFn
{
    using Store = IteratorType;

    inline static void storeMove(IteratorType &i, Move m) noexcept
    {
        *i = m;
        ++i;
    }
};

struct SingleMoveStoreMoveFn
{
    using Store = Move;

    inline static void storeMove(Move &ret, Move m) noexcept
    {
        if (ret == Move::illegalNoMove()) [[likely]]
            ret = m;
        else
            ret = Move::illegalAmbiguousMove();
    }
};

struct SingleMoveStoreMoveNoDupCheckFn
{
    using Store = Move;

    inline static void storeMove(Move &ret, Move m) noexcept
    {
        ret = m;
    }
};


template <typename IteratorType>
struct MoveGenIteratorTraits
{
    using Iterator = IteratorType;

    // Returns true when further move generation can still add moves. This is
    // mainly intended to allow movegen with LegalMoveDetectorIterator to exit
    // early after a legal move has been found.
    static constexpr bool moveGenCompleted(Iterator) noexcept;

    // Whether this iterator stores moves (MoveList::iterator) or
    // whether it's just interested on whether there is a move
    // (LegalMoveCounterIterator, LegalMoveDetectorIterator). This is used to
    // skip creating moves is addMoveIfLegal* functions if the actual moves
    // don't matter.
    static constexpr bool storesMoves() noexcept;
};

template <>
struct MoveGenIteratorTraits<MoveList::iterator>
{
    using Iterator = MoveList::iterator;

    static constexpr bool canCompleteEarly { false };

    static constexpr bool storesMoves() noexcept
    {
        return true;
    }
};

template <>
struct MoveGenIteratorTraits<LegalMoveCounterIterator>
{
    using Iterator = LegalMoveCounterIterator;

    static constexpr bool canCompleteEarly { false };

    static constexpr bool storesMoves() noexcept
    {
        return false;
    }
};

template <>
struct MoveGenIteratorTraits<LegalMoveDetectorIterator>
{
    using Iterator = LegalMoveDetectorIterator;

    static constexpr bool canCompleteEarly { true };

    static constexpr bool storesMoves() noexcept
    {
        return false;
    }
};

}

#endif
