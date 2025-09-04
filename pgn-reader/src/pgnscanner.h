// Hoover Chess Utilities / PGN reader
// Copyright (C) 2022-2025  Sami Kiminki
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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNSCANNER_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNSCANNER_H_INCLUDED

#include "pgnreader-error.h"
#include "pgnscannertokens.h"

#include <FlexLexer.h>

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstring>
#include <format>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{

/// @addtogroup PgnReaderImpl
/// @{

/// @brief The PGN scanner (lexer)
class PgnScanner : public ::yyFlexLexer
{
private:
    const char *m_inputData;
    std::size_t m_inputLeft;
    PgnScannerToken m_curToken { };
    PgnScannerTokenInfo m_tokenInfo;

public:
    /// @brief Constructor
    ///
    /// @param[in]  inputData     Input data
    /// @param[in]  inputLen      Input length
    PgnScanner(const char *inputData, std::size_t inputLen) noexcept :
        yyFlexLexer { nullptr, nullptr },
        m_inputData { inputData },
        m_inputLeft { inputLen },
        m_tokenInfo { }
    {
    }

    PgnScanner(const PgnScanner &) = delete;
    PgnScanner(PgnScanner &&) = delete;
    PgnScanner & operator = (const PgnScanner &) & = delete;
    PgnScanner & operator = (PgnScanner &&) & = delete;

    /// @brief Scans input and returns the next token. On bad input, error token
    /// is returned instead of throwing an exception.
    ///
    /// @return Token
    ///
    /// This function is mainly intended for error recovery.
    inline PgnScannerToken nextTokenNoThrowOnErrorToken()
    {
        m_curToken = yylexex();
        return m_curToken;
    }

    /// @brief Scans input and returns the next token
    ///
    /// @return Token
    /// @throws PgnError(PgnErrorCode::BAD_CHARACTER)  Erroneous scanner input
    ///
    /// The token in textual format can be obtained with calls to
    /// @c YYText() and @c YYLeng() provided by the parent class.
    ///
    /// Certain tokens are decoded by the scanner. The decoded information
    /// is obtained with a call to @coderef{getTokenInfo()}.
    ///
    /// @sa https://westes.github.io/flex/manual/Cxx.html
    inline PgnScannerToken nextToken()
    {
        m_curToken = yylexex();

        if (m_curToken == PgnScannerToken::ERROR) [[unlikely]]
        {
            throw PgnError(
                PgnErrorCode::BAD_CHARACTER,
                std::format(
                    "{}: '{}'",
                    getTokenInfo().error.errorMessage,
                    std::string_view { YYText(), static_cast<std::size_t>(YYLeng()) }));
        }

        return m_curToken;
    }

    /// @brief Returns the previously scanned token
    ///
    /// @return Token
    ///
    /// The token in textual format can be obtained with calls to
    /// @c YYText() and @c YYLeng() provided by the parent class.
    ///
    /// Certain tokens are decoded by the scanner. The decoded information
    /// is obtained with a call to @coderef{getTokenInfo()}.
    ///
    /// @sa https://westes.github.io/flex/manual/Cxx.html
    inline PgnScannerToken getCurrentToken() const noexcept
    {
        return m_curToken;
    }

    /// @brief Returns additional information on the token.
    ///
    /// @return Additional inforation on the current token
    inline const PgnScannerTokenInfo &getTokenInfo() const noexcept
    {
        return m_tokenInfo;
    }

#define C(tok) case tok: return #tok

    /// @brief Returns a string for a scanner token
    ///
    /// @param[in] token  PGN scanner token
    /// @return           Corresponding string
    static constexpr const char *scannerTokenToString(PgnScannerToken token) noexcept
    {
        switch (token)
        {
            C(NONE);
            C(END_OF_FILE);
            C(TAG_START);
            C(TAG_KEY);
            C(TAG_VALUE);
            C(TAG_END);
            C(VARIATION_START);
            C(VARIATION_END);
            C(MOVENUM);
            C(MOVE_PAWN);
            C(MOVE_PAWN_CAPTURE);
            C(MOVE_PAWN_PROMO);
            C(MOVE_PAWN_PROMO_CAPTURE);
            C(MOVE_PIECE_KNIGHT);
            C(MOVE_PIECE_BISHOP);
            C(MOVE_PIECE_ROOK);
            C(MOVE_PIECE_QUEEN);
            C(MOVE_PIECE_KING);
            C(MOVE_SHORT_CASTLE);
            C(MOVE_LONG_CASTLE);
            C(NAG);
            C(COMMENT_START);
            C(COMMENT_TEXT);
            C(COMMENT_NEWLINE);
            C(COMMENT_END);
            C(RESULT);
            C(ERROR);
            default:
                assert(false);
                return "???";
        }
    }
#undef C

protected:

    /// @brief Provides more input to the parent class
    ///
    /// @param[in] buf       Buffer for input data
    /// @param[in] maxSize   Maximum size of data to be provided
    /// @return              Number of bytes provided
    ///
    /// @sa https://westes.github.io/flex/manual/Cxx.html
    int LexerInput(char *buf, int maxSize)
    {
        const std::size_t copySize { std::min(std::size_t { static_cast<unsigned int>(maxSize) }, m_inputLeft) };

        std::memcpy(buf, m_inputData, copySize);
        m_inputData += copySize;
        m_inputLeft -= copySize;

        return copySize;
    }

private:
    /// @brief The generated lexer
    ///
    /// @return Token
    PgnScannerToken yylexex();

    /// @brief Ascii to unsigned integer conversion
    ///
    /// @tparam    RetType    Unsigned integer type
    /// @param[in] str        Start of string to convert (inclusive)
    /// @param[in] end        End of string to convert (exclusive)
    /// @param[in] tokenType  Name of the token type, e.g., @c "NAG". Used for error reporting
    /// @return               Unsigned integer
    /// @throws PgnError(PgnErrorCode::BAD_CHARACTER)  Integer overflow
    ///
    /// @pre This function assumes that the string consists only of numbers
    template <typename RetType>
    RetType asciiToUnsigned(const char *str, const char *end, const char *tokenType)
    {
        RetType ret { };
        RetType prev { };

        while (str != end)
        {
            char c = *str;
            ret *= 10U;
            ret += static_cast<RetType>(c - '0');

            if (ret < prev)
                throw PgnError(
                    PgnErrorCode::BAD_CHARACTER,
                    std::format("Integer overflow when tokenizing {}", tokenType));

            prev = ret;
            ++str;
        }

        return ret;
    }

    /// @brief Translates character coordinates to a square
    ///
    /// @param[in]   colChar      Column character (file), range: ['a', 'h']
    /// @param[in]   rowChar      Row character (rank), range: ['1', '8']
    /// @return                   Corresponding square
    static constexpr inline Square charCoordToSq(char colChar, char rowChar) noexcept
    {
        const std::uint8_t col { static_cast<std::uint8_t>(colChar - 'a') };
        const std::uint8_t row { static_cast<std::uint8_t>(rowChar - '1') };

        return makeSquare(col, row);
    }

    /// @brief Translates column coordinate (file) to a square set
    ///
    /// @param[in]   colChar      Column character (file), range: ['a', 'h']
    /// @return                   Corresponding set of squares
    static constexpr inline SquareSet colCharToMask(char colChar) noexcept
    {
        const std::uint8_t col { static_cast<std::uint8_t>(colChar - 'a') };

        return SquareSet::column(col);
    }

    /// @brief Translates row coordinate (rank) to a square set
    ///
    /// @param[in]   rowChar      Row character (rank), range: ['1', '8']
    /// @return                   Corresponding set of squares
    static constexpr inline SquareSet rowCharToMask(char rowChar) noexcept
    {
        const std::uint8_t row { static_cast<std::uint8_t>(rowChar - '1') };

        return SquareSet::row(row);
    }

    static constexpr std::array<Piece, 32> ctCharToPieceTable {{
            Piece::NONE,   // 0
            Piece::NONE,   // 1
            Piece::BISHOP, // 2
            Piece::NONE,   // 3
            Piece::NONE,   // 4
            Piece::NONE,   // 5
            Piece::NONE,   // 6
            Piece::NONE,   // 7
            Piece::NONE,   // 8
            Piece::NONE,   // 9
            Piece::NONE,   // 10
            Piece::NONE,   // 11
            Piece::NONE,   // 12
            Piece::NONE,   // 13
            Piece::KNIGHT, // 14
            Piece::NONE,   // 15
            Piece::NONE,   // 16
            Piece::QUEEN,  // 17
            Piece::ROOK,   // 18
            Piece::NONE,   // 19
            Piece::NONE,   // 21
            Piece::NONE,   // 22
            Piece::NONE,   // 23
            Piece::NONE,   // 24
            Piece::NONE,   // 25
            Piece::NONE,   // 26
            Piece::NONE,   // 27
            Piece::NONE,   // 28
            Piece::NONE,   // 29
            Piece::NONE,   // 30
            Piece::NONE,   // 31
    }};

    static constexpr std::array<PgnScannerToken, 32> ctCharToMovePieceTable {{
            PgnScannerToken::NONE,   // 0
            PgnScannerToken::NONE,   // 1
            PgnScannerToken::MOVE_PIECE_BISHOP, // 2
            PgnScannerToken::NONE,   // 3
            PgnScannerToken::NONE,   // 4
            PgnScannerToken::NONE,   // 5
            PgnScannerToken::NONE,   // 6
            PgnScannerToken::NONE,   // 7
            PgnScannerToken::NONE,   // 8
            PgnScannerToken::NONE,   // 9
            PgnScannerToken::NONE,   // 10
            PgnScannerToken::MOVE_PIECE_KING,   // 11
            PgnScannerToken::NONE,   // 12
            PgnScannerToken::NONE,   // 13
            PgnScannerToken::MOVE_PIECE_KNIGHT, // 14
            PgnScannerToken::NONE,   // 15
            PgnScannerToken::NONE,   // 16
            PgnScannerToken::MOVE_PIECE_QUEEN,  // 17
            PgnScannerToken::MOVE_PIECE_ROOK,   // 18
            PgnScannerToken::NONE,   // 19
            PgnScannerToken::NONE,   // 21
            PgnScannerToken::NONE,   // 22
            PgnScannerToken::NONE,   // 23
            PgnScannerToken::NONE,   // 24
            PgnScannerToken::NONE,   // 25
            PgnScannerToken::NONE,   // 26
            PgnScannerToken::NONE,   // 27
            PgnScannerToken::NONE,   // 28
            PgnScannerToken::NONE,   // 29
            PgnScannerToken::NONE,   // 30
            PgnScannerToken::NONE,   // 31
    }};

    // expect char to be one of: 'B', 'R', 'N', 'Q'
    static constexpr inline Piece getPieceForChar(char c) noexcept
    {
        return ctCharToPieceTable[static_cast<std::uint8_t>(c) % ctCharToPieceTable.size()];
    }

    // expect char to be one of: 'B', 'R', 'N', 'Q', 'K'
    static constexpr inline PgnScannerToken getMovePieceScannerTokenForChar(char c) noexcept
    {
        return ctCharToMovePieceTable[static_cast<std::uint8_t>(c) % ctCharToMovePieceTable.size()];
    }

    inline void setTokenInfo_MOVENUM(const char *str, const char *end, Color color);
    inline void setTokenInfo_PAWN_MOVE(SquareSet srcMask, Square dstSq, Piece promoPiece);
    inline void setTokenInfo_PIECE_MOVE(SquareSet srcMask, bool capture, Square dstSq);
    inline void setTokenInfo_NAG(std::uint8_t nag);
    inline void setTokenInfo_RESULT(PgnResult result);
    inline void setTokenInfo_ERROR(const char *errorMessage);

    inline PgnScannerToken tokenizePieceMove(std::string_view str);
    inline PgnScannerToken tokenizeUnusualPawnMove(std::string_view str);
};

/// @}

}

#endif
