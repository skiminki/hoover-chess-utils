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

#include "pgnreader-error.h"

#include "gtest/gtest.h"


namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(PgnError, getStringForCode)
{
    for (unsigned i { }; i <= 256U; ++i)
    {
        PgnErrorCode code { i };
        bool isKnown;

        switch (code)
        {
            case PgnErrorCode::OK:
            case PgnErrorCode::BAD_CHARACTER:
            case PgnErrorCode::BAD_PGN_TAG:
            case PgnErrorCode::UNEXPECTED_MOVE_NUM:
            case PgnErrorCode::UNEXPECTED_TOKEN:
            case PgnErrorCode::BAD_FEN:
            case PgnErrorCode::ILLEGAL_MOVE:
            case PgnErrorCode::AMBIGUOUS_MOVE:
            case PgnErrorCode::UNIMPLEMENTED:
            case PgnErrorCode::INTERNAL_ERROR:
                isKnown = true;
                break;

            default:
                isKnown = false;
                break;
        }

        // We'll just make sure all listed errors are mapped to something, and
        // unlisted errors are mapped to "Unknown error"

        if (isKnown)
            EXPECT_NE(PgnError::getStringForCode(code), "Unknown error");
        else
            EXPECT_EQ(PgnError::getStringForCode(code), "Unknown error");
    }
}

}
