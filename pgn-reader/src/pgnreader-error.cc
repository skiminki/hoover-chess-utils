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

#include "pgnreader-error.h"
#include "pgnscanner.h"

#include <format>
#include <string_view>

namespace hoover_chess_utils::pgn_reader
{

std::string_view PgnError::getStringForCode(PgnErrorCode code) noexcept
{
    switch (code)
    {
        case PgnErrorCode::OK:                        return "Success";
        case PgnErrorCode::BAD_CHARACTER:             return "Bad character";
        case PgnErrorCode::BAD_PGN_TAG:               return "Bad PGN tag";
        case PgnErrorCode::UNEXPECTED_MOVE_NUM:       return "Unexpected move number";
        case PgnErrorCode::UNEXPECTED_TOKEN:          return "Bad token";
        case PgnErrorCode::BAD_FEN:                   return "Bad FEN";
        case PgnErrorCode::ILLEGAL_MOVE:              return "Illegal move";
        case PgnErrorCode::AMBIGUOUS_MOVE:            return "Ambiguous move";
        case PgnErrorCode::UNIMPLEMENTED:             return "Unimplemented";
        case PgnErrorCode::INTERNAL_ERROR:            return "Internal error";

        default:                                      return "Unknown error";
    }
}

PgnError::PgnError(PgnErrorCode code, std::string_view details) :
    m_str { std::format("Error {} ({}): {}", static_cast<unsigned>(code), getStringForCode(code), details) },
    m_code { code }
{
}

PgnError::PgnError(const PgnScanner &scanner, const PgnError &ex) :
    m_str { std::format("Line {}: {}", scanner.lineno(), ex.m_str) },
    m_code { ex.m_code }
{
}

}
