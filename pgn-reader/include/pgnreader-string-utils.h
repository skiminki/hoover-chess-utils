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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_STRING_UTILS_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_STRING_UTILS_H_INCLUDED

#include "chessboard.h"
#include "chessboard-types.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <string_view>
#include <type_traits>

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Tag for unitialized storage. This is a tiny optimization to save a
/// single write when constructing a @coderef{MiniString}.
///
/// @remark Constructing an uninitialized @coderef{MiniString} is made awkward
/// on purpose.
struct MiniString_Uninitialized { };

/// @brief A Pascal-style length-prefixed string with the specified maximum
/// length.
///
/// @tparam t_maxLen    Maximum length of the contained string. Range: [1, 255]
///
/// This object is intended as a fast, low-level container for small
/// strings. The implementation foregoes many common API usage correctness
/// checks to improve runtime performance.
///
/// @remark @coderef{MiniString} is not a @c '\0'-terminated string. A
/// @coderef{MiniString} can contain the character @c '\0'.
template <std::size_t t_maxLen>
class MiniString
{
    static_assert(t_maxLen > 0U);
    static_assert(t_maxLen <= 255U);

private:
    std::uint8_t m_length;
    std::array<char, t_maxLen> m_storage;

public:

    /// @brief Initializing constructor. Sets the length of the contained string
    /// to 0.
    constexpr MiniString() noexcept : m_length { }
    {
    }

    /// @brief Non-initializing constructor. The state of the object is
    /// undefined.
    constexpr MiniString(MiniString_Uninitialized) noexcept
    {
    }

    /// @brief Copy constructor (default)
    MiniString(const MiniString &) = default;

    /// @brief Move constructor (default)
    MiniString(MiniString &&) = default;

    /// @brief Copy assignment (default)
    MiniString &operator = (const MiniString &) & = default;

    /// @brief Move assignment (default)
    MiniString &operator = (MiniString &&) & = default;

    /// @brief Destructor (default)
    ~MiniString() = default;

    /// @brief Sets the length of the contained string without resetting the
    /// contents.
    ///
    /// @param[in] length    New string length. Range: [0, @c t_maxLen]
    ///
    /// @note It is the caller's responsibility to ensure that the string data
    /// for offsets [0, @c length-1] is fully initialized.
    constexpr void setLength(std::uint8_t length) noexcept
    {
        assert(length <= t_maxLen);

        m_length = length;
    }

    /// @brief Assigns a C-string
    ///
    /// @param[in]  str   C-string
    ///
    /// Assigns a C-string by copying up to @c t_maxLen characters and setting
    /// the length. The terminator (@c '\0') is not included.
    constexpr void assign(const char *str) noexcept
    {
        std::size_t len { };

        while (*str != '\0' && len < t_maxLen)
            m_storage[len++] = *str++;

        m_length = len;
    }

    /// @brief Assigns a string with length. In case the string is longer than
    /// the maximum capacity, the contained string is truncated.
    ///
    /// @param[in]  str     Pointer to string
    /// @param[in]  strLen  Input string length
    constexpr void assign(const char *str, std::size_t strLen) noexcept
    {
        const std::size_t len { std::min(capacity(), strLen) };

        for (std::size_t i { }; i < len; ++i)
            m_storage[i] = str[i];

        m_length = len;
    }

    /// @brief Returns a character at a specified offset. This function does not
    /// perform an index bounds check.
    ///
    /// @param[in]  index     String offset
    /// @return               Character at the specified offset
    constexpr inline char operator [] (std::size_t index) const noexcept
    {
        assert(index <= t_maxLen);
        return m_storage[index];
    }

    /// @brief Returns a reference to the character at a specified offset. This
    /// function does not perform an index bounds check.
    ///
    /// @param[in]  index     String offset
    /// @return               Reference to the character at the specified offset
    ///
    /// **Example**
    ///
    /// @code
    /// MiniString<10U> str;
    /// str.setLength(2U);
    /// str[0] = '4';
    /// str[1] = '2';
    /// @endcode
    constexpr inline char &operator [] (std::size_t index) noexcept
    {
        assert(index <= t_maxLen);
        return m_storage[index];
    }

    /// @brief Returns a pointer to the string data. The string may be
    /// non-terminated by (@c '\0').
    ///
    /// @return Pointer to string data
    constexpr inline char *data() noexcept
    {
        return m_storage.data();
    }

    /// @brief Returns a const pointer to the string data. The string may be
    /// non-terminated by (@c '\0').
    ///
    /// @return Pointer to string data
    constexpr inline const char *data() const noexcept
    {
        return m_storage.data();
    }

    /// @brief Returns the capacity (maximum size) of a string to hold
    ///
    /// @return Maximum size of the held string. Same as @c t_maxLen template
    /// parameter.
    static constexpr inline std::size_t capacity() noexcept
    {
        return t_maxLen;
    }

    /// @brief Returns the size of the string
    ///
    /// @return Size of the string
    constexpr inline std::size_t size() const noexcept
    {
        assert(m_length <= t_maxLen);
        return m_length;
    }

    /// @brief Returns a string view to the string
    ///
    /// @return String view
    ///
    /// @code
    /// MiniString<10U> str;
    /// str.assign("12345");
    /// std::cout << "MiniString contents: " << str.getStringView() << std::endl;
    /// @endcode
    constexpr std::string_view getStringView() const
    {
        return std::string_view { m_storage.data(), size() };
    }
};

/// @brief MiniString containing a FEN
///
/// For maximum size bound, the following template was used:
///
///     xxxxxxxx/xxxxxxxx/xxxxxxxx/xxxxxxxx/xxxxxxxx/xxxxxxxx/xxxxxxxx/xxxxxxxx w KQkq ep 255 4294967295
///     123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456
///     0        1         2         3         4         5         6         7         8         9
///
/// which is 96 bytes. Technically, this board could not have EP, but that
/// can be arranged with individual empty squares.
using FenString = MiniString<96U>;

/// @brief Miscellaneous string utilities
class StringUtils
{
public:
    /// @brief Returns a file (column) designation character for a square, lower-case
    ///
    /// @param[in]  sq    Valid square
    /// @return           Lower case file designation (<tt>'a'</tt>--<tt>'h'</tt>)
    static constexpr inline char colChar(Square sq) noexcept
    {
        return 'a' + columnOf(sq);
    }

    /// @brief Returns a rank (row) designation character for a square, lower-case
    ///
    /// @param[in]  sq    Valid square
    /// @return           Rank designation (<tt>'1'</tt>--<tt>'8'</tt>)
    static constexpr inline char rowChar(Square sq) noexcept
    {
        return '1' + rowOf(sq);
    }

    /// @brief Source mask to SAN notation
    ///
    /// @param[in] srcMask    Source mask derived from SAN notation
    /// @return               SAN notation for source square
    ///
    /// <table>
    /// <tr>
    ///   <th>Source mask</th>
    ///   <th>Returned string</th>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{SquareSet::all()}</td>
    ///   <td>Empty string (no source mask was specified)</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{SquareSet::column}<tt>(N)</tt></td>
    ///   <td>File designation character (source column was specified)</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{SquareSet::row}<tt>(N)</tt></td>
    ///   <td>Rank designation character (source row was specified)</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{SquareSet::square}<tt>(sq)</tt></td>
    ///   <td>File and rank designation characters (both source column and row were specified)</td>
    /// </tr>
    /// <tr>
    ///   <td>Other</td>
    ///   <td>Unspecified but not undefined behavior</td>
    /// </tr>
    /// </table>
    static MiniString<2U> sourceMaskToString(SquareSet srcMask) noexcept;

    /// @brief Ply number to move string (e.g., <tt>"5."</tt> or <tt>"11..."</tt>)
    ///
    /// @param[in] plyNum  Ply number
    /// @return            Move string in PGN move number format. E.g.,
    ///                    @c "1." for first white move and
    ///                    @c "2..." for second black move.
    static MiniString<13U> plyNumToString(std::uint32_t plyNum) noexcept
    {
        return moveNumToString(moveNumOfPly(plyNum), colorOfPly(plyNum));
    }

    /// @brief Generic unsigned integer to string converter
    ///
    /// @tparam bufSize    Reserved buffer size. Must be exactly the maximum string
    ///                    size for the integer type
    /// @tparam UintType   Unsigned integer type
    /// @param[out] s      Pointer to receive the string
    /// @param[in]  num    Number to convert
    /// @return            Pointer one past the string. Difference to @c s is the length
    ///                    of the written string.
    template <std::size_t bufSize, typename UintType>
    static char *genUnsignedToString(char *s, UintType num) noexcept
    {
        static_assert(std::is_unsigned_v<UintType>);
        static_assert(std::is_integral_v<UintType>);
        static_assert(std::numeric_limits<UintType>::digits10 + 1U == bufSize);

        char *i = s;

        do
        {
            *i++ = '0' + (num % 10U);
            num /= 10U;
        }
        while (num > 0U);

        // Workaround for GCC bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106757
        //
        // This should be just:
        //
        // std::reverse(s, i);

        const std::size_t len = i - s;
        [[assume(len <= bufSize)]];

        for (std::size_t c { }; c < len / 2U; ++c)
            std::swap(s[c], s[len - c - 1U]);

        return i;
    }

    /// @brief Move number and side-to-move to move string (e.g., <tt>"5."</tt> or <tt>"11..."</tt>)
    ///
    /// @param[in] moveNum Move number
    /// @param[in] turn    Side to move
    /// @return            Move string in PGN move number format. E.g.,
    ///                    @c "1." for first white move and
    ///                    @c "2..." for second black move.
    static MiniString<13U> moveNumToString(std::uint32_t moveNum, Color turn) noexcept
    {
        // max length: "4294967295..."
        //              1234567890123 = 13 chars
        MiniString<13U> ret { MiniString_Uninitialized() };

        char *i = ret.data();

        i = genUnsignedToString<10U>(i, moveNum);

        *i++ = '.';
        if (turn == Color::BLACK)
        {
            *i++ = '.';
            *i++ = '.';
        }

        ret.setLength(i - ret.data());
        return ret;
    }

    /// @brief Translates promotion piece to character (N/B/R/Q). Return value
    /// is unspecified for any other piece values.
    ///
    /// @param[in] promo  Promotion piece
    /// @return           Piece character, one of (N/B/R/Q)
    static constexpr char promoPieceChar(Piece promo) noexcept
    {
        static_assert(Piece::KNIGHT == Piece { 2U });
        static_assert(Piece::BISHOP == Piece { 3U });
        static_assert(Piece::ROOK   == Piece { 4U });
        static_assert(Piece::QUEEN  == Piece { 5U });

        std::uint32_t pieceCharactersWord {
            (static_cast<std::uint32_t>('R') << 0U)  |
            (static_cast<std::uint32_t>('Q') << 8U)  |
            (static_cast<std::uint32_t>('N') << 16U) |
            (static_cast<std::uint32_t>('B') << 24U) };

        unsigned int shift { (static_cast<std::uint8_t>(promo) & 3U) * 8U };
        return static_cast<char>((pieceCharactersWord >> shift) & 0xFFU);
    }

    /// @brief Translates piece to SAN string
    ///
    /// @param[in] p     Piece
    /// @return          Piece string for SAN
    ///
    /// <table>
    /// <tr>
    ///   <th>Piece</th>
    ///   <th>Returned string</th>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::PAWN}</td>
    ///   <td>Empty string</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::KNIGHT}</td>
    ///   <td>@c "N"</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::BISHOP}</td>
    ///   <td>@c "B"</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::ROOK}</td>
    ///   <td>@c "R"</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::QUEEN}</td>
    ///   <td>@c "Q"</td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{Piece::KING}</td>
    ///   <td>@c "K"</td>
    /// </tr>
    /// <tr>
    ///   <td>Other</td>
    ///   <td>@c "?"</td>
    /// </tr>
    /// </table>
    static constexpr MiniString<1U> pieceToSanStr(Piece p) noexcept
    {
        static_assert(Piece::NONE   == Piece { 0U });
        static_assert(Piece::PAWN   == Piece { 1U });
        static_assert(Piece::KNIGHT == Piece { 2U });
        static_assert(Piece::BISHOP == Piece { 3U });
        static_assert(Piece::ROOK   == Piece { 4U });
        static_assert(Piece::QUEEN  == Piece { 5U });
        static_assert(Piece::KING   == Piece { 6U });

        constexpr std::uint64_t pieceCharactersWord {
            (static_cast<std::uint64_t>('?') << 0U)   |
            (static_cast<std::uint64_t>('N') << 16U)  |
            (static_cast<std::uint64_t>('B') << 24U)  |
            (static_cast<std::uint64_t>('R') << 32U)  |
            (static_cast<std::uint64_t>('Q') << 40U)  |
            (static_cast<std::uint64_t>('K') << 48U) };

        if (p > Piece::KING)
            p = Piece::NONE;

        const unsigned int index { static_cast<unsigned int>(p) };

        char pieceCharacter { static_cast<char>((pieceCharactersWord >> (index * 8U)) & 0xFFU) };

        MiniString<1U> ret { MiniString_Uninitialized() };
        ret[0U] = pieceCharacter;
        ret.setLength(pieceCharacter != '\0' ? 1U : 0U);

        return ret;
    }

    /// @brief Generates minimal SAN for a move and plays it. The move is fully
    /// validated by this function.
    ///
    /// @param[in] board    Chess board
    /// @param[in] move     Move to play
    /// @throws PgnError(PgnErrorCode::ILLEGAL_MOVE)   Illegal move. @c board is
    ///                                                not modified.
    /// @throws std::logic_error                       Bad move type @c board is
    ///                                                not modified.
    ///
    /// Format of the produced move string is as follows:
    ///
    ///     SAN_MOVE = move_body ('+' | '#')?
    ///
    /// where @c move_body consists the piece identifier and source/destination
    /// identifiers, appended with check/checkmate indicators as necessary.
    ///
    /// @c move_body has the following format:
    ///
    /// <table>
    /// <tr>
    ///   <th>Type (@coderef{MoveTypeAndPromotion})</th>
    ///   <th>Condition (first match)</th>
    ///   <th>Format</th>
    ///   <th>Example</th>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{MoveTypeAndPromotion::REGULAR_PAWN_MOVE}</td>
    ///   <td>None</td>
    ///   <td><tt>&lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>e4</tt></td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{MoveTypeAndPromotion::REGULAR_PAWN_CAPTURE}<br>
    ///       @coderef{MoveTypeAndPromotion::EN_PASSANT}</td>
    ///   <td>None</td>
    ///   <td><tt>&lt;src_file&gt; 'x' &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>exc5</tt><br><tt>cxd6</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="8">@coderef{MoveTypeAndPromotion::REGULAR_KNIGHT_MOVE}<br>
    ///       @coderef{MoveTypeAndPromotion::REGULAR_BISHOP_MOVE}<br>
    ///       @coderef{MoveTypeAndPromotion::REGULAR_ROOK_MOVE}<br>
    ///       @coderef{MoveTypeAndPromotion::REGULAR_QUEEN_MOVE}</td>
    ///   <td rowspan="2">No disambiguation required</td>
    ///   <td><tt>&lt;piece&gt; &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Ne4</tt></td>
    /// </tr>
    /// <tr>
    ///   <td><tt>&lt;piece&gt; &lt;dest_file&gt; 'x' &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Nxf6</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="2">Source file disambiguation required</td>
    ///   <td><tt>&lt;piece&gt; &lt;src_file&gt; &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Nbd2</tt></td>
    /// </tr>
    /// <tr>
    ///   <td><tt>&lt;piece&gt; &lt;src_file&gt; 'x' &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Nexg5</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="2">Source rank disambiguation required</td>
    ///   <td><tt>&lt;piece&gt; &lt;src_rank&gt; &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>R1h3</tt></td>
    /// </tr>
    /// <tr>
    ///   <td><tt>&lt;piece&gt; &lt;src_rank&gt; 'x' &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>N5xd4</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="2">Source square required</td>
    ///   <td><tt>&lt;piece&gt; &lt;src_file&gt; &lt;src_rank&gt; &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Qd4d5</tt></td>
    /// </tr>
    /// <tr>
    ///   <td><tt>&lt;piece&gt; &lt;src_file&gt; 'x' &lt;src_rank&gt; &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Qc2xc5</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="2">@coderef{MoveTypeAndPromotion::REGULAR_KING_MOVE}</td>
    ///   <td>No capture</td>
    ///   <td><tt>'K' &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Kd2</tt></td>
    /// </tr>
    /// <tr>
    ///   <td>Capture</td>
    ///   <td><tt>"Kx" &lt;dest_file&gt; &lt;dest_rank&gt;</tt></td>
    ///   <td><tt>Kxf7</tt></td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{MoveTypeAndPromotion::CASTLING_SHORT}</td>
    ///   <td>None</td>
    ///   <td><tt>"O-O"</tt></td>
    ///   <td><tt>O-O</tt></td>
    /// </tr>
    /// <tr>
    ///   <td>@coderef{MoveTypeAndPromotion::CASTLING_LONG}</td>
    ///   <td>None</td>
    ///   <td><tt>"O-O-O"</tt></td>
    ///   <td><tt>O-O-O</tt></td>
    /// </tr>
    /// <tr>
    ///   <td rowspan="2">@coderef{MoveTypeAndPromotion::PROMO_KNIGHT}<br>
    ///       @coderef{MoveTypeAndPromotion::PROMO_BISHOP}<br>
    ///       @coderef{MoveTypeAndPromotion::PROMO_ROOK}<br>
    ///       @coderef{MoveTypeAndPromotion::PROMO_QUEEN}</td>
    ///   <td>No capture</td>
    ///   <td><tt>&lt;dest_file&gt; &lt;dest_rank&gt; '=' &lt;piece&gt;</tt></td>
    ///   <td><tt>a8=Q</tt></td>
    /// </tr>
    /// <tr>
    ///   <td>Capture</td>
    ///   <td><tt>&lt;src_file&gt; 'x' &lt;dest_file&gt; &lt;dest_rank&gt; '=' &lt;piece&gt;</tt></td>
    ///   <td><tt>fxg1=N</tt></td>
    /// </tr>
    /// </table>
    ///
    /// Disambiguation requirements:
    /// <table>
    /// <tr>
    ///   <th>Disambiguation requirement</th>
    ///   <th>Description</th>
    /// </tr>
    /// <tr>
    ///   <td>None</td>
    ///   <td>There is only a single legal move for the piece and destination.</td>
    /// </tr>
    /// <tr>
    ///   <td>Source file</td>
    ///   <td>Two or more legal moves match with the piece and destination
    ///       square. Specifying source file makes the move unambiguous.</td>
    /// </tr>
    /// <tr>
    ///   <td>Source rank</td>
    ///   <td>Two or more legal moves match with the piece and destination
    ///       square. Specifying source rank makes the move unambiguous.</td>
    /// </tr>
    /// <tr>
    ///   <td>Source square</td>
    ///   <td>More than one legal move matches with the piece and destination
    ///       square. Neither source file nor source rank is sufficient to make
    ///       the move unambiguous.</td>
    /// </tr>
    /// </table>
    /// Note that the potential check/checkmate indicator is not used for disambiguation.
    ///
    /// @remark The maximum length of the returned move is 7 characters.
    /// Example: @c "Na1xb3+"
    static MiniString<7U> moveToSanAndPlay(ChessBoard &board, Move move);

    /// @brief Generates minimal SAN for a move. The move is fully validated by
    /// this function.
    ///
    /// @param[in] board    Chess board
    /// @param[in] move     Move to play
    /// @throws PgnError(PgnErrorCode::ILLEGAL_MOVE)   Illegal move
    ///
    /// This function is implemented using @coderef{moveToSanAndPlay()}. In case
    /// the intention is to produce SAN notation while replaying the moves, it
    /// is faster to invoke @coderef{moveToSanAndPlay()} to combine the
    /// activities than calling this function and @coderef{ChessBoard::doMove()}
    /// separately.
    ///
    /// @sa @coderef{moveToSanAndPlay()} for full specification.
    static MiniString<7U> moveToSan(const ChessBoard &board, Move move)
    {
        ChessBoard tmpBoard { board };
        return moveToSanAndPlay(tmpBoard, move);
    }

    /// @brief Returns a name string for @coderef{PieceAndColor}.
    ///
    /// @param[in]  pc    PieceAndColor. May be valid or invalid
    /// @return           Name string
    ///
    /// For all concrete @coderef{PieceAndColor} values, the name is a two-letter color
    /// (lower-case 'w' or 'b') and piece (upper-case 'P', 'N', 'B', 'R', 'Q', 'K')
    /// string. For none values, the return string is two spaces. For any other
    /// values, the returned string is "??".
    static std::string_view pieceAndColorToString(PieceAndColor pc) noexcept;

    /// @brief Returns a name string for @coderef{Square}.
    ///
    /// @param[in]  sq                Square. May be valid or invalid
    /// @param[in]  emptySquareName   Name for @coderef{Square::NONE} and
    ///                               invalid squares.
    /// @return                       Name string
    ///
    /// For all valid squares, the name is a two-letter file and rank. For example: "a1", "h8".
    static std::string_view squareToString(Square sq, std::string_view emptySquareName) noexcept;

    /// @brief Move type and promotion code to string
    ///
    /// @param[in]  typeAndPromotion   Move type and promotion piece
    /// @return                        String matching the enum value label or @c "??"
    static std::string_view moveTypeAndPromotionToString(MoveTypeAndPromotion typeAndPromotion) noexcept;

    /// @brief Builds a FEN string for the position on @c board
    ///
    /// @param[in]  board         Board that holds the position
    /// @param[in]  fen           FEN for the position
    static void boardToFEN(const ChessBoard &board, FenString &fen) noexcept;

};

/// @}

}

#endif
