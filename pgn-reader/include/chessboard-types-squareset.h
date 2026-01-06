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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TYPES_SQUARESET_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__CHESSBOARD_TYPES_SQUARESET_H_INCLUDED

#include "chessboard-types.h"

#include <atomic>
#include <bit>
#include <cassert>
#include <cstdint>
#include <type_traits>


namespace hoover_chess_utils::pgn_reader
{

/// @ingroup PgnReaderAPI
/// @brief Set of squares. Implemented using a bit-mask.
class SquareSet
{
private:
    /// @brief The set of squares. Bit @c N set in the mask represents square @c
    /// N included in the set. See @coderef{Square} for the square values.
    std::uint64_t m_bitmask { };

public:
    /// @brief Default constructor (empty set)
    constexpr inline SquareSet() = default;

    /// @brief Constructor from a list of @coderef{Square}s
    ///
    /// @param[in]  squares    A list of squares to initially populate the square set
    constexpr inline SquareSet(std::same_as<Square> auto... squares) noexcept :
        m_bitmask {  }
    {
        std::uint64_t bitmask { };
        for (auto sq : std::initializer_list<Square>{ squares... })
        {
            const auto sqRaw { static_cast<SquareUnderlyingType>(sq) };

            assert(sqRaw < 64U);
            [[assume(sqRaw < 64U)]];

            bitmask |= UINT64_C(1) << sqRaw;
        }

        m_bitmask = bitmask;
    }

    /// @brief Default destructor
    constexpr ~SquareSet() = default;

    /// @brief Default copy constructor
    constexpr inline SquareSet(const SquareSet &) = default;

    /// @brief Default move constructor
    constexpr inline SquareSet(SquareSet &&) = default;

    /// @brief Default copy assignment
    constexpr inline SquareSet &operator = (const SquareSet &) = default;

    /// @brief Default move assignment
    constexpr inline SquareSet &operator = (SquareSet &&) = default;

    /// @brief Cast operator
    explicit constexpr inline SquareSet(std::uint64_t mask) noexcept :
        m_bitmask { mask }
    {
    }

    /// @brief Cast operator
    explicit constexpr inline operator std::uint64_t() const noexcept
    {
        return m_bitmask;
    }

    /// @brief Returns the number of squares in the set
    ///
    /// @return Number of squares in the set
    constexpr inline std::uint_fast8_t popcount() const noexcept
    {
        return std::popcount(m_bitmask);
    }

    /// @brief Returns the first (lowest-value) square in the set or
    /// @coderef{Square::NONE} if the set is empty.
    ///
    /// @return First (lowest-value) square in the set or @coderef{Square::NONE} for
    /// empty set.
    constexpr inline Square firstSquare() const noexcept
    {
        return Square(std::countr_zero(m_bitmask));
    }

    /// @brief Returns the last (highest-value) square in the set or @c
    /// #Square::NONE if the set is empty.
    ///
    /// @return First (lowest-value) square in the set or @coderef{Square::NONE} for
    /// empty set.
    constexpr inline Square lastSquare() const noexcept
    {
        SquareUnderlyingType tmp = 63U - std::countl_zero(m_bitmask);
        return tmp <= 63U ? Square(tmp) : Square::NONE;
    }

    /// @brief Returns a square set with the first square (if any) removed.
    ///
    /// @return The square set with the first square removed
    constexpr inline SquareSet removeFirstSquare() const noexcept
    {
        return SquareSet { m_bitmask & (m_bitmask - 1U) };
    }

    /// @brief Returns an intersection with another square set.
    ///
    /// @param[in] other   Another square set
    /// @return            Intersection of @c *this and @c other
    ///
    /// **Example**
    /// @code
    /// SquareSet a = SquareSet::column(1) & SquareSet::row(2);
    /// // a == SquareSet::square(Square::B3); -- column B, rank 3
    /// @endcode
    constexpr inline SquareSet operator & (SquareSet other) const noexcept
    {
        return SquareSet { m_bitmask & other.m_bitmask };
    }

    /// @brief Returns an union with another square set.
    ///
    /// @param[in] other   Another square set
    /// @return            Union of @c *this and @c other
    ///
    /// **Example**
    /// @code
    /// SquareSet a = SquareSet::column(1) | SquareSet::row(2);
    /// // a includes squares B1..B8 and A3..H3.
    /// @endcode
    constexpr inline SquareSet operator | (SquareSet other) const noexcept
    {
        return SquareSet { m_bitmask | other.m_bitmask };
    }

    /// @brief Returns an exclusive or with another square set. Formally, when
    /// square @c s is included in exactly one of @c *this or @c other, it is
    /// included in the return set.
    ///
    /// @param[in] other   Another square set
    /// @return            Exclusive or @c *this and @c other
    ///
    /// The set-theoretical definition is as follows: @c R = (@c A ∪ @c B) ∖ (@c A ∩ @c B)
    ///
    /// **Example**
    /// @code
    /// SquareSet a = SquareSet::column(1)  SquareSet::row(2);
    /// // a includes squares B1..B8 and A3..H3 except B3, since B3 is
    /// // included in both SquareSet::column(1) and SquareSet::row(2)
    /// @endcode
    constexpr inline SquareSet operator ^ (SquareSet other) const noexcept
    {
        return SquareSet { m_bitmask ^ other.m_bitmask };
    }

    /// @brief Assignment operator (union). Shorthand for @c x = @c x & @c other.
    ///
    /// @param[in] other   Another square set
    /// @return            @c *this after assignment
    constexpr inline SquareSet &operator &= (SquareSet other) noexcept
    {
        m_bitmask &= other.m_bitmask;
        return (*this);
    }

    /// @brief Assignment operator (union). Shorthand for @c x = @c x | @c other.
    ///
    /// @param[in] other   Another square set
    /// @return            @c *this after assignment
    constexpr inline SquareSet &operator |= (SquareSet other) noexcept
    {
        m_bitmask |= other.m_bitmask;
        return (*this);
    }

    /// @brief Assignment operator (union). Shorthand for @c x = @c x ^ @c other.
    ///
    /// @param[in] other   Another square set
    /// @return            @c *this after assignment
    constexpr inline SquareSet &operator ^= (SquareSet other) noexcept
    {
        m_bitmask ^= other.m_bitmask;
        return (*this);
    }

    /// @brief Returns the complement of @c *this.
    ///
    /// @return            Set of squares not included in @c *this.
    ///
    /// **Example**
    /// @code
    /// SquareSet a = ~SquareSet::none();
    /// // a == SquareSet::all()
    /// @endcode
    constexpr inline SquareSet operator ~ () const noexcept
    {
        return SquareSet { ~m_bitmask };
    }

    /// @brief Shifts left element-wise
    ///
    /// @param[in] shift      Shift amount. Range: [0, 63]
    /// @return               Shifted elements
    ///
    /// Every square in the set is added by @c shift. In case the square value
    /// overflows, it is removed from the set.
    ///
    /// @c shift amount outside range is undefined behavior.
    ///
    /// **Example**
    /// @code
    /// SquareSet a = SquareSet::column(4) << 2U;
    /// // a == SquareSet::column(6), i.e., the G file
    ///
    /// SquareSet b = SquareSet::column(0) << 8U;
    /// // b includes squares A2..A8, since A1 became A2, A2 become A3, and so
    /// // on. A8 would have overflown to A9.
    /// @endcode
    constexpr inline SquareSet operator << (std::uint_fast8_t shift) const noexcept
    {
        return SquareSet { m_bitmask << shift };
    }

    /// @brief Shifts right element-wise
    ///
    /// @param[in] shift      Shift amount. Range: [0, 63]
    /// @return               Shifted elements
    ///
    /// Every square in the set is subtracted by @c shift. In case the square value
    /// overflows, it is removed from the set.
    ///
    /// @c shift amount outside range is undefined behavior.
    ///
    /// **Example**
    /// @code
    /// SquareSet a = SquareSet::column(4) >> 2U;
    /// // a == SquareSet::column(2), i.e., the C file
    ///
    /// SquareSet b = SquareSet::column(0) >> 8U;
    /// // b includes squares A1..A7, since A2 became A1, A3 become A2, and so
    /// // on. A1 would have overflown to A0.
    /// @endcode
    constexpr inline SquareSet operator >> (std::uint_fast8_t shift) const noexcept
    {
        return SquareSet { m_bitmask >> shift };
    }

    /// @brief Rotates left element-wise (circular shift)
    ///
    /// @param[in] shift      Shift amount. No range restrictions.
    /// @return               Shifted elements
    ///
    /// Every square in the set is added by @c shift (modulo 64). That is, in
    /// case the square value overflows, it wraps around.
    ///
    /// All values of @c shift are allowed.
    ///
    /// **Example**
    /// @code
    /// SquareSet b = SquareSet::column(0).rotl(8);
    /// // b includes squares A1..A8, since A1 became A2, A2 become A3, and so
    /// // on. A8 wrap-arounds to A1 (equivalent to A9).
    /// @endcode
    constexpr inline SquareSet rotl(std::int_fast8_t shift) const noexcept
    {
        return SquareSet { std::rotl(m_bitmask, shift) };
    }

    /// @brief Rotates right element-wise (circular shift)
    ///
    /// @param[in] shift      Shift amount. No range restrictions.
    /// @return               Shifted elements
    ///
    /// Every square in the set is subtracted by @c shift (modulo 64). That is,
    /// in case the square value overflows, it wraps around.
    ///
    /// All values of @c shift are allowed.
    ///
    /// **Example**
    /// @code
    /// SquareSet b = SquareSet::column(0).rotr(8);
    /// // b includes squares A1..A8, since A2 became A1, A3 become A2, and so
    /// // on. A1 wrap-arounds to A8 (equivalent to A0).
    /// @endcode
    constexpr inline SquareSet rotr(std::int_fast8_t shift) const noexcept
    {
        return SquareSet { std::rotr(m_bitmask, shift) };
    }

    /// @brief Assignment operator (left shift). Shorthand for @c x = @c x << @c shift.
    ///
    /// @param[in] shift   Shift amount. Range: [0, 63]
    /// @return            @c *this after assignment
    constexpr inline SquareSet operator <<= (std::uint_fast8_t shift) noexcept
    {
        m_bitmask <<= shift;
        return (*this);
    }

    /// @brief Assignment operator (right shift). Shorthand for @c x = @c x >> @c shift.
    ///
    /// @param[in] shift   Shift amount. Range: [0, 63]
    /// @return            @c *this after assignment
    constexpr inline SquareSet operator >>= (std::uint_fast8_t shift) noexcept
    {
        m_bitmask >>= shift;
        return (*this);
    }

    /// @brief Flips the squares vertically (upside down)
    ///
    /// @return             Result
    constexpr inline SquareSet flipVert() const noexcept
    {
        return SquareSet { std::byteswap(m_bitmask) };
    }

    /// @brief Comparison operator (equality). Returns @true if @c *this equals
    /// to @c other.
    ///
    /// @param[in] other   Another square set
    /// @return            Comparison result
    constexpr inline bool operator == (SquareSet other) const noexcept
    {
        return m_bitmask == other.m_bitmask;
    }

    /// @brief Comparison operator (non-equality). Returns @true if @c *this
    /// does not equal to @c other.
    ///
    /// @param[in] other   Another square set
    /// @return            Comparison result
    constexpr inline bool operator != (SquareSet other) const noexcept
    {
        return m_bitmask != other.m_bitmask;
    }

    /// @brief Extracts squares from @c *this using extraction mask. The extracted squares are
    /// mapped as follows:
    /// - lowest square in the extraction mask &rarr; A1
    /// - second lowest square in the extraction mask &rarr; A2
    /// - ...
    ///
    /// @param[in] extractMask     Extraction mask
    /// @return                    Extracted squares
    ///
    /// **Example**
    ///
    /// Inputs: (@c *this as named squares; @c extractMask shaded)
    /// <table class="bitboard">
    /// <tr><td>A8</td><td>B8</td><td>C8</td><td>D8</td><td>E8</td><td>F8</td><td>G8</td><td>H8</td></tr>
    /// <tr><td>A7</td><td>B7</td><td class="mask">C7</td><td>D7</td><td>E7</td><td>F7</td><td>G7</td><td>H7</td></tr>
    /// <tr><td>A6</td><td>B6</td><td class="mask">C6</td><td>D6</td><td>E6</td><td>F6</td><td>G6</td><td>H6</td></tr>
    /// <tr><td>A5</td><td>B5</td><td class="mask">C5</td><td>D5</td><td>E5</td><td>F5</td><td>G5</td><td>H5</td></tr>
    /// <tr><td>A4</td><td>B4</td><td class="mask">C4</td><td>D4</td><td>E4</td><td>F4</td><td>G4</td><td>H4</td></tr>
    /// <tr><td>A3</td><td class="mask">B3</td><td>C3</td><td class="mask">D3</td><td class="mask">E3</td><td class="mask">F3</td><td class="mask">G3</td><td>H3</td></tr>
    /// <tr><td>A2</td><td>B2</td><td class="mask">C2</td><td>D2</td><td>E2</td><td>F2</td><td>G2</td><td>H2</td></tr>
    /// <tr><td>A1</td><td>B1</td><td>C1</td><td>D1</td><td>E1</td><td>F1</td><td>G1</td><td>H1</td></tr>
    /// </table>
    ///
    /// Output:
    /// <table class="bitboard">
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td>C6</td><td>C7</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td>C3</td><td>B3</td><td>D3</td><td>E3</td><td>F3</td><td>G3</td><td>C4</td><td>C5</td></tr>
    /// </table>
    ///
    /// @remark @ref HAVE_X86_BMI2 indicates fast implementation.
    SquareSet parallelExtract(SquareSet extractMask) noexcept;

    /// @brief Maps squares from @c *this using extraction mask. Squares are mapped as follows:
    /// mapped as follows:
    /// - lowest square in @c *this &rarr; lowest square in @c extractMask
    /// - second lowest square in @c *this &rarr; second lowest square in @c extractMask
    /// - ...
    ///
    /// @param[in] extractMask     Extract mask
    /// @return                    Mapped (deposited) squares
    ///
    /// **Example**
    ///
    /// @c *this:
    /// <table class="bitboard">
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td>C6</td><td>C7</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td>C3</td><td>B3</td><td>D3</td><td>E3</td><td>F3</td><td>G3</td><td>C4</td><td>C5</td></tr>
    /// </table>
    ///
    /// @c extractMask:
    /// <table class="bitboard">
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask"> 1</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask"> 1</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask"> 1</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask"> 1</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td class="mask"> 1</td><td> 0</td><td class="mask"> 1</td><td class="mask"> 1</td><td class="mask"> 1</td><td class="mask"> 1</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask"> 1</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// </table>
    ///
    /// Output:
    /// <table class="bitboard">
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask">C7</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask">C6</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask">C5</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask">C4</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td class="mask">B3</td><td> 0</td><td class="mask">D3</td><td class="mask">E3</td><td class="mask">F3</td><td class="mask">G3</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td class="mask">C2</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// <tr><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td><td> 0</td></tr>
    /// </table>
    ///
    /// @remark @ref HAVE_X86_BMI2 indicates fast implementation.
    SquareSet parallelDeposit(SquareSet extractMask) noexcept;

    /// @brief Returns an empty set
    ///
    /// @return Empty set
    static constexpr inline SquareSet none() noexcept
    {
        return SquareSet { };
    }

    /// @brief Returns a set of all squares
    ///
    /// @return Set of all squares
    static constexpr inline SquareSet all() noexcept
    {
        return SquareSet { 0xFF'FF'FF'FF'FF'FF'FF'FFU };
    }

    /// @brief Returns a set of squares in column number @c col.
    ///
    /// @param[in] col     Column number
    /// @return            Set of squares in column @c col. Range: [0, 7]
    ///
    /// @remark The A-file is column 0, the B file is column 1, and so on.
    static constexpr inline SquareSet column(RowColumn col) noexcept
    {
        assert(col <= 7U);
        [[assume(col <= 7U)]];

        return SquareSet { std::uint64_t { 0x01'01'01'01'01'01'01'01U } << col };
    }

    /// @brief Returns a set of squares in row number @c row.
    ///
    /// @param[in] row     Row number
    /// @return            Set of squares in row @c row. Range: [0, 7]
    ///
    /// @remark The 1st rank is row 0, the 2nd rank is row 1, and so on.
    static constexpr inline SquareSet row(RowColumn row) noexcept
    {
        assert(row <= 7U);
        [[assume(row <= 7U)]];

        return SquareSet { std::uint64_t { 0x00'00'00'00'00'00'00'FFU } << (row * 8U) };
    }

    /// @brief Returns a set of single square.
    ///
    /// @param[in] sq     The singleton square
    /// @return           The set containing @c sq
    static constexpr inline SquareSet square(Square sq) noexcept
    {
        assert(static_cast<SquareUnderlyingType>(sq) <= 63U);
        [[assume(static_cast<SquareUnderlyingType>(sq) <= 63U)]];

        return SquareSet { std::uint64_t { 1U } << static_cast<SquareUnderlyingType>(sq) };
    }

    /// @brief Returns a set of 0 or 1 squares
    ///
    /// @param[in] sq      Valid square or Square::NONE
    /// @return            The set containing @c sq, including empty set for Square::NONE
    static constexpr inline SquareSet squareOrNone(Square sq) noexcept
    {
        const std::uint64_t bitToShift { sq < Square::NONE };
        return SquareSet { std::rotl(bitToShift, static_cast<SquareUnderlyingType>(sq)) };
    }

    /// @brief Returns a set of single square specified by column and row numbers
    ///
    /// @param[in] col    Column number of the square
    /// @param[in] row    Row number of the square
    /// @return           The set containing square specified by @c col and @c row.
    static constexpr inline SquareSet square(RowColumn col, RowColumn row) noexcept
    {
        assert(col <= 7U);
        assert(row <= 7U);

        const auto shift { col + (row * 8U) };
        [[assume(shift <= 63U)]];

        return SquareSet { std::uint64_t { 1U } << shift };
    }

    /// @brief Checks whether a valid square is in the set
    constexpr inline bool isMember(Square sq) const noexcept
    {
        assert(static_cast<std::uint8_t>(sq) <= 63U);
        [[assume(static_cast<std::uint8_t>(sq) <= 63U)]];

        return (m_bitmask & (std::uint64_t { 1U } << static_cast<SquareUnderlyingType>(sq))) != 0U;
    }

    /// @brief Conditional
    constexpr inline SquareSet allIfNone() const noexcept
    {
        return SquareSet { m_bitmask ? SquareSet::none() : SquareSet::all() };
    }

    /// @brief Conditional
    constexpr inline SquareSet allIfAny() const noexcept
    {
        return SquareSet { m_bitmask ? SquareSet::all() : SquareSet::none() };
    }
};

}

/// @cond full
/// @ingroup PgnReaderImpl
/// @brief Implementation of @coderef{SQUARESET_ENUMERATE()}.
///
/// @param[in] sq         Variable name for enumerated squares
/// @param[in] squareSet  Square set
/// @param[in] tmpMask    Variable name for temporary square set
/// @param[in] ...        Statements to execute per enumerated square. Statements may
///                       include @c continue and @c break. The last statement
///                       does not have to end with semi-colon.
///
/// @remark Macro approach is used here for the following reasons:
/// - Lambdas are not quite optimal:
///   - CLANG has difficulties in optimizing lambda expressions
///     to be as efficient as expanded macros.
///   - We can't exit the enumeration loop by a returning from a lambda
/// - A regular iterator holding a square set mask is not as efficient.
///   The problem is that begin() != end() condition means comparing
///   masks. However, it's faster to do sq > Square::H8 comparison after
///   determining the lowest populated bit before calculating the mask
///   after the lowest bit has been extracted.
/// @endcond
/// @cond !full
/// @ingroup PgnReaderImpl
/// @brief Internal implementation of @coderef{SQUARESET_ENUMERATE()}.
/// **Do not invoke directly.**
/// @endcond
#define SQUARESET_ENUMERATE_INTERNAL(sq, squareSet, tmpMask, ...)       \
    do                                                                  \
    {                                                                   \
        SquareSet tmpMask { squareSet };                                \
                                                                        \
        while (true)                                                    \
        {                                                               \
            const Square sq { tmpMask.firstSquare() };                  \
                                                                        \
            if (sq > Square::H8)                                        \
                break;                                                  \
                                                                        \
            tmpMask &= ~SquareSet::square(sq);                          \
                                                                        \
            __VA_ARGS__;                                                \
        }                                                               \
    }                                                                   \
    while (false)

/// @ingroup PgnReaderAPI
/// @brief Enumerates all squares in a square set.
///
/// @param[in] sq         Variable name for enumerated squares
/// @param[in] squareSet  Square set
/// @param[in] ...        Statements to execute per enumerated square. Statements may
///                       include @c continue and @c break. The last statement
///                       does not have to end with semi-colon.
///
/// **Example**
/// @code
/// std::uint8_t inefficientPopcount(SquareSet sqSetToEnumerate)
/// {
///     std::uint8_t ret { };
///
///     SQUARESET_ENUMERATE(
///         sq, sqSetToEnumerate,
///         ++ret);
///
///     return ret;
/// }
/// @endcode
#define SQUARESET_ENUMERATE(sq, squareSet, ...)                         \
    SQUARESET_ENUMERATE_INTERNAL(sq, squareSet, squareset_enumerate_internal_mask, __VA_ARGS__)

#endif
