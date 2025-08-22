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


#include <gtest/gtest.h>

#include "include/chessboard.h"
#include "include/pgnreader.h"
#include "include/pgnreader-string-utils.h"
#include "src/pgnscanner.h"

#include <format>
#include <iostream>
#include <string_view>

extern unsigned char scannertest_1_pgn_data[];
extern unsigned int scannertest_1_pgn_data_len;

namespace hoover_chess_utils::pgn_reader::unit_test
{

TEST(PgnScannerTest, BasicTokenize)
{
    PgnScanner pgnScanner {
        reinterpret_cast<const char *>(scannertest_1_pgn_data),
        scannertest_1_pgn_data_len};
    bool spaceBeforeToken { };


    while (true)
    {
        const PgnScannerToken token { pgnScanner.nextToken() };

        if (spaceBeforeToken)
            std::cout << ' ';
        else
            spaceBeforeToken = true; // reset on tokens were we add newline

        std::cout << PgnScanner::scannerTokenToString(token);

        switch (token)
        {
        case PgnScannerToken::END_OF_FILE:
            std::cout << std::endl;
            goto exitLoop;

        case PgnScannerToken::TAG_START:
            break;

        case PgnScannerToken::TAG_KEY:
            std::cout << '(' << pgnScanner.YYText() << ')';
            break;

        case PgnScannerToken::TAG_VALUE:
            std::cout << '(' << pgnScanner.YYText() << ')';
            break;

        case PgnScannerToken::TAG_END:
            std::cout << std::endl;
            spaceBeforeToken = false;
            break;

        case PgnScannerToken::VARIATION_START:
            break;

        case PgnScannerToken::VARIATION_END:
            break;

        case PgnScannerToken::COMMENT_START:
            break;

        case PgnScannerToken::COMMENT_TEXT:
            std::cout << '(' << pgnScanner.YYText() << ')';
            break;

        case PgnScannerToken::COMMENT_NEWLINE:
            break;

        case PgnScannerToken::COMMENT_END:
            std::cout << std::endl;
            spaceBeforeToken = false;
            break;

        case PgnScannerToken::MOVENUM:
        {
            const PgnScannerTokenInfo_MOVENUM &moveNum { pgnScanner.getTokenInfo().moveNum };
            std::cout
                << '('
                << StringUtils::moveNumToString(moveNum.num, moveNum.color).getStringView()
                << ')';
            break;
        }

        case PgnScannerToken::MOVE_PAWN:
        {
            const PgnScannerTokenInfo_PAWN_MOVE &pawnMove { pgnScanner.getTokenInfo().pawnMove };
            std::cout
                << std::format(
                    "({}{})",
                    StringUtils::sourceMaskToString(pawnMove.srcMask).getStringView(),
                    StringUtils::squareToString(pawnMove.dstSq, "??"));
            break;
        }

        case PgnScannerToken::MOVE_PAWN_CAPTURE:
        {
            const PgnScannerTokenInfo_PAWN_MOVE &pawnMove { pgnScanner.getTokenInfo().pawnMove };
            std::cout << std::format(
                "({}x{})",
                StringUtils::sourceMaskToString(pawnMove.srcMask).getStringView(),
                StringUtils::squareToString(pawnMove.dstSq, "??"));
            break;
        }

        case PgnScannerToken::MOVE_PAWN_PROMO:
        {
            const PgnScannerTokenInfo_PAWN_MOVE &pawnMove { pgnScanner.getTokenInfo().pawnMove };
            std::cout << std::format(
                "({}{}={})",
                StringUtils::sourceMaskToString(pawnMove.srcMask).getStringView(),
                StringUtils::squareToString(pawnMove.dstSq, "??"),
                StringUtils::pieceToSanStr(pawnMove.promoPiece).getStringView());
            break;
        }

        case PgnScannerToken::MOVE_PAWN_PROMO_CAPTURE:
        {
            const PgnScannerTokenInfo_PAWN_MOVE &pawnMove { pgnScanner.getTokenInfo().pawnMove };
            std::cout << std::format(
                "({}x{}={})",
                StringUtils::sourceMaskToString(pawnMove.srcMask).getStringView(),
                StringUtils::squareToString(pawnMove.dstSq, "??"),
                StringUtils::pieceToSanStr(pawnMove.promoPiece).getStringView());
            break;
        }

        case PgnScannerToken::MOVE_PIECE:
        {
            const PgnScannerTokenInfo_PIECE_MOVE &pieceMove { pgnScanner.getTokenInfo().pieceMove };
            std::cout << std::format(
                "({}{}{}{})",
                StringUtils::pieceToSanStr(pieceMove.piece).getStringView(),
                StringUtils::sourceMaskToString(pieceMove.srcMask).getStringView(),
                pieceMove.capture ? "x" : "",
                StringUtils::squareToString(pieceMove.dstSq, "??"));
            break;
        }

        case PgnScannerToken::MOVE_SHORT_CASTLE:
            break;

        case PgnScannerToken::MOVE_LONG_CASTLE:
            break;

        case PgnScannerToken::NAG:
        {
            const PgnScannerTokenInfo_NAG &nag { pgnScanner.getTokenInfo().nag };

            std::cout << "($" << std::uint16_t { nag.nag } << ')';
            break;
        }

        case PgnScannerToken::RESULT:
        {
            const PgnScannerTokenInfo_RESULT &result { pgnScanner.getTokenInfo().result };

            switch (result.result)
            {
            case PgnResult::WHITE_WIN:
                std::cout << "(1-0)";
                break;
            case PgnResult::BLACK_WIN:
                std::cout << "(0-1)";
                break;
            case PgnResult::DRAW:
                std::cout << "(1/2-1/2)";
                break;
            default:
                std::cout << "(*)";
                break;
            }

            std::cout << std::endl << std::endl;
            spaceBeforeToken = false;
            break;
        }

        default:
            std::cout << "Token " << token << " '" << pgnScanner.YYText() << "'" << pgnScanner.YYLeng() << std::endl;
            FAIL();
            break;
        }
    }
  exitLoop:
    ;
}

#ifdef NDEBUG
// scnnerTokenToString asserts for bad token
TEST(PgnScannerTest, scannerTokenToString_badToken)
{
    EXPECT_EQ(std::string_view { "???" }, PgnScanner::scannerTokenToString(PgnScannerToken { 255U }));
}
#endif

}
