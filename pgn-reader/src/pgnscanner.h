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

#ifndef CHESS_UTILS__PGN_READER__PGN_SCANNER_H_INCLUDED
#define CHESS_UTILS__PGN_READER__PGN_SCANNER_H_INCLUDED

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

class PgnScanner : public ::yyFlexLexer
{
private:
    const char *m_inputData;
    std::size_t m_inputLeft;
    PgnScannerTokenInfo m_tokenInfo;

public:
    PgnScanner(const char *inputData, std::size_t inputLen) :
        yyFlexLexer { nullptr, nullptr },
        m_inputData { inputData },
        m_inputLeft { inputLen },
        m_tokenInfo { }
    {
    }

    inline PgnScannerToken nextToken()
    {
        return yylexex();
    }

    inline const PgnScannerTokenInfo &getTokenInfo() const noexcept
    {
        return m_tokenInfo;
    }

#define C(tok) case tok: return #tok
    static constexpr const char *scannerTokenToString(PgnScannerToken token) noexcept
    {
        switch (token)
        {
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
            C(MOVE_PIECE);
            C(MOVE_SHORT_CASTLE);
            C(MOVE_LONG_CASTLE);
            C(NAG);
            C(COMMENT_START);
            C(COMMENT_TEXT);
            C(COMMENT_NEWLINE);
            C(COMMENT_END);
            C(RESULT);
            default:
                assert(false);
                return "???";
        }
    }
#undef C

protected:
    int LexerInput(char *buf, int maxSize)
    {
        const std::size_t copySize { std::min(std::size_t { static_cast<unsigned int>(maxSize) }, m_inputLeft) };

        std::memcpy(buf, m_inputData, copySize);
        m_inputData += copySize;
        m_inputLeft -= copySize;

        return copySize;
    }

private:
    // extended yylex
    PgnScannerToken yylexex();

    // Rewrites string by processing back slash escapes
    // - buf is the start of the buffer (null-terminated)
    // - cursor should be the first character to get processed -- bufStart+1 to eliminate string '"'
    // - returns end of rewritten string (points always to '\0')
    char *processBackSlashEscapes(char *buf, const char *cursor);

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

    static constexpr inline Square charCoordToSq(char colChar, char rowChar) noexcept
    {
        const std::uint8_t col { static_cast<std::uint8_t>(colChar - 'a') };
        const std::uint8_t row { static_cast<std::uint8_t>(rowChar - '1') };

        return makeSquare(col, row);
    }

    static constexpr inline SquareSet colCharToMask(char colChar) noexcept
    {
        const std::uint8_t col { static_cast<std::uint8_t>(colChar - 'a') };

        return SquareSet::column(col);
    }

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
            Piece::KING,   // 11
            Piece::NONE,   // 12
            Piece::NONE,   // 13
            Piece::KNIGHT, // 14
            Piece::NONE,   // 15
            Piece::PAWN,   // 16
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

    // expect char to be one of: 'P', 'B', 'R', 'N', 'Q', 'K'
    static constexpr inline Piece getPieceForChar(char c) noexcept
    {
        return ctCharToPieceTable[static_cast<std::uint8_t>(c) % ctCharToPieceTable.size()];
    }

    inline void setTokenInfo_MOVENUM(const char *str, const char *end, Color color);
    inline void setTokenInfo_PAWN_MOVE(SquareSet srcMask, bool capture, Square dstSq, Piece promoPiece);
    inline void setTokenInfo_PIECE_MOVE(SquareSet srcMask, Piece piece, bool capture, Square dstSq);
    inline void setTokenInfo_NAG(std::uint8_t nag);
    inline void setTokenInfo_RESULT(PgnResult result);

    inline PgnScannerToken tokenizePieceMove(std::string_view str);
    inline PgnScannerToken tokenizeUnusualPawnMove(std::string_view str);
};

}

#endif
