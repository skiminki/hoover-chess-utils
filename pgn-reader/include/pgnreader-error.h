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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_ERROR_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__PGNREADER_ERROR_H_INCLUDED

#include <exception>
#include <string>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{
    class PgnScanner;

/// @addtogroup PgnReaderAPI
/// @{

/// @brief Error code
///
/// @sa @coderef{PgnError}
enum class PgnErrorCode : unsigned
{
    /// @brief Code for success. Never set in @coderef{PgnError} by this code
    /// base.
    OK = 0U,

    /// @brief Unexpected character in PGN input (tokenizer error)
    BAD_CHARACTER = 1U,

    /// @brief PGN tag parsing failed (parser error)
    BAD_PGN_TAG,

    /// @brief Unexpected (bad) move number (PGN reader error)
    UNEXPECTED_MOVE_NUM,

    /// @brief Unexpected (bad) token (parser error)
    UNEXPECTED_TOKEN,

    /// @brief Bad or illegal FEN (position setup error)
    BAD_FEN,

    /// @brief Illegal move
    ILLEGAL_MOVE,

    /// @brief Ambiguous move (missing source file/rank specifiers)
    AMBIGUOUS_MOVE,

    /// @brief Unimplemented functionality
    UNIMPLEMENTED,

    /// @brief Internal logic error
    INTERNAL_ERROR,
};

/// @brief PGN error exception
class PgnError : public std::exception
{
private:
    /// @brief Error string
    std::string m_str;

    /// @brief Error code
    PgnErrorCode m_code;

public:
    /// @brief Constructor: error code and error message
    ///
    /// @param[in]  code         Error code
    /// @param[in]  details      Error message (additional details)
    PgnError(PgnErrorCode code, std::string_view details);

    /// @brief Constructor: adds position to a PGN error
    ///
    /// @param[in]  scanner      Tokenizer for error location
    /// @param[in]  ex           Exception
    PgnError(const PgnScanner &scanner, const PgnError &ex);

    /// @brief Returns the error message
    ///
    /// @return Error message
    const char *what() const noexcept override
    {
        return m_str.c_str();
    }

    /// @brief Returns the error code
    ///
    /// @return Error code
    PgnErrorCode getCode() const noexcept
    {
        return m_code;
    }

    /// @brief Returns error string for code
    ///
    /// @param[in]  code         Error code
    /// @return                  Error string
    static std::string_view getStringForCode(PgnErrorCode code) noexcept;

};

/// @}

}


#endif
