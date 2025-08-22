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

#include "pgnreader-error.h"
#include "src/pgnparser.h"

#include <string_view>
#include <iostream>

extern unsigned char parsertest_1_pgn_data[];
extern unsigned int parsertest_1_pgn_data_len;


class TestActions
{
public:
    void gameStart()
    {
        std::cout << "New game" << std::endl;
    }

    void gameTerminated(hoover_chess_utils::pgn_reader::PgnResult result)
    {
        const char *str;
        switch (result)
        {
            case hoover_chess_utils::pgn_reader::PgnResult::WHITE_WIN: str = "1-0"; break;
            case hoover_chess_utils::pgn_reader::PgnResult::BLACK_WIN: str = "0-1"; break;
            case hoover_chess_utils::pgn_reader::PgnResult::DRAW:      str = "1/2-1/2"; break;
            case hoover_chess_utils::pgn_reader::PgnResult::UNKNOWN:   str = "*"; break;
            default:
                FAIL();
                break;
        }
        std::cout << "Game terminated: " << str << std::endl;
    }

    void endOfPGN()
    {
        std::cout << "End of PGN" << std::endl;
    }

    void pgnTag(const std::string_view &key, const std::string_view &value)
    {
        std::cout << "PGN tag: '" << key << "' '" << value << "'" << std::endl;
    }

    void moveTextSection()
    {
        std::cout << "Move text section" << std::endl;
    }

    void comment(const std::string_view &str)
    {
        std::cout << "Comment: '" << str << "'" << std::endl;
    }

    void nag(std::uint8_t nag)
    {
        std::cout << "Numeric annotation glyph: " << std::uint32_t{nag} << std::endl;
    }

    void moveNum(std::uint32_t plyNum)
    {
        std::cout << "Move num: "
                  << hoover_chess_utils::pgn_reader::moveNumOfPly(plyNum)
                  << (hoover_chess_utils::pgn_reader::colorOfPly(plyNum) == hoover_chess_utils::pgn_reader::Color::WHITE ?
                      ". " : "... ");
    }

    constexpr static char colChar(hoover_chess_utils::pgn_reader::Square sq) noexcept
    {
        return 'a' + hoover_chess_utils::pgn_reader::columnOf(sq);
    }

    constexpr static char rowChar(hoover_chess_utils::pgn_reader::Square sq) noexcept
    {
        return '1' + hoover_chess_utils::pgn_reader::rowOf(sq);
    }

    static std::string srcMaskToStr(hoover_chess_utils::pgn_reader::SquareSet mask)
    {
        if (mask == hoover_chess_utils::pgn_reader::SquareSet::all())
            return "";

        if (mask.popcount() == 1U)
        {
            const hoover_chess_utils::pgn_reader::Square sq { mask.firstSquare() };
            return std::format("{}{}", colChar(sq), rowChar(sq));
        }

        if ((mask & hoover_chess_utils::pgn_reader::SquareSet::column(0U)).popcount() == 1U)
        {
            // row
            const hoover_chess_utils::pgn_reader::Square sq { mask.firstSquare() };
            return std::format("{}", rowChar(sq));
        }
        else
        {
            // column
            const hoover_chess_utils::pgn_reader::Square sq { mask.firstSquare() };
            return std::format("{}", colChar(sq));
        }
    }

    // moves
    void move(hoover_chess_utils::pgn_reader::Piece p,
              hoover_chess_utils::pgn_reader::SquareSet srcMask,
              bool capture,
              hoover_chess_utils::pgn_reader::Square dst,
              hoover_chess_utils::pgn_reader::Piece promo)
    {
        std::cout << "Move: ";

        const char *pieceCh;
        switch (p)
        {
            case hoover_chess_utils::pgn_reader::Piece::PAWN:   pieceCh = ""; break;
            case hoover_chess_utils::pgn_reader::Piece::KNIGHT: pieceCh = "N"; break;
            case hoover_chess_utils::pgn_reader::Piece::BISHOP: pieceCh = "B"; break;
            case hoover_chess_utils::pgn_reader::Piece::ROOK:   pieceCh = "R"; break;
            case hoover_chess_utils::pgn_reader::Piece::QUEEN:  pieceCh = "Q"; break;
            case hoover_chess_utils::pgn_reader::Piece::KING:   pieceCh = "K"; break;
            default:                                     pieceCh = "???"; break;
        }
        std::cout << pieceCh;

        std::cout << srcMaskToStr(srcMask);

        if (capture)
            std::cout << 'x';

        std::cout << char(hoover_chess_utils::pgn_reader::columnOf(dst) + 'a') << char(hoover_chess_utils::pgn_reader::rowOf(dst) + '1');

        switch (promo)
        {
            case hoover_chess_utils::pgn_reader::Piece::NONE:   pieceCh = ""; break;
            case hoover_chess_utils::pgn_reader::Piece::KNIGHT: pieceCh = "=N"; break;
            case hoover_chess_utils::pgn_reader::Piece::BISHOP: pieceCh = "=B"; break;
            case hoover_chess_utils::pgn_reader::Piece::ROOK:   pieceCh = "=R"; break;
            case hoover_chess_utils::pgn_reader::Piece::QUEEN:  pieceCh = "=Q"; break;
            default:                                     pieceCh = "???"; break;
        }

        std::cout << pieceCh;

        std::cout << std::endl;
    }

    void movePawn(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::PAWN,
            srcMask, false, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void movePawnCapture(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::PAWN,
            srcMask, false, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void movePawnPromo(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        hoover_chess_utils::pgn_reader::Piece promo)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::PAWN,
            srcMask, true, dst,
            promo);
    }

    void movePawnPromoCapture(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        hoover_chess_utils::pgn_reader::Piece promo)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::PAWN,
            srcMask, true, dst,
            promo);
    }

    void moveKnight(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        bool capture)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::KNIGHT,
            srcMask, capture, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void moveBishop(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        bool capture)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::BISHOP,
            srcMask, capture, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void moveRook(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        bool capture)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::ROOK,
            srcMask, capture, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void moveQueen(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        bool capture)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::QUEEN,
            srcMask, capture, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void moveKing(
        hoover_chess_utils::pgn_reader::SquareSet srcMask,
        hoover_chess_utils::pgn_reader::Square dst,
        bool capture)
    {
        move(
            hoover_chess_utils::pgn_reader::Piece::KING,
            srcMask, capture, dst,
            hoover_chess_utils::pgn_reader::Piece::NONE);
    }

    void moveShortCastle()
    {
        std::cout << "Move: ";
        std::cout << "O-O";
        std::cout << std::endl;
    }

    void moveLongCastle()
    {
        std::cout << "Move: ";
        std::cout << "O-O-O";
        std::cout << std::endl;
    }

    void variationStart()
    {
        std::cout << "Variation start" << std::endl;
    }

    void variationEnd()
    {
        std::cout << "Variation end" << std::endl;
    }
};



TEST(PgnParserTest, BasicParse)
{
    using hoover_chess_utils::pgn_reader::PgnResult;
    using hoover_chess_utils::pgn_reader::PgnScanner;
    using hoover_chess_utils::pgn_reader::PgnParser;

    PgnScanner pgnScanner {
        reinterpret_cast<const char *>(parsertest_1_pgn_data),
        parsertest_1_pgn_data_len};

    TestActions actions { };
    PgnParser parser { pgnScanner, actions };

    parser.parse();

}

TEST(PgnParserTest, Errors)
{
    using hoover_chess_utils::pgn_reader::PgnError;
    using hoover_chess_utils::pgn_reader::PgnErrorCode;
    using hoover_chess_utils::pgn_reader::PgnScanner;
    using hoover_chess_utils::pgn_reader::PgnParser;
    using hoover_chess_utils::pgn_reader::PgnParser_NullActions;

    struct InputAndExpectedError
    {
        const char *input;
        PgnErrorCode code;
    };

    InputAndExpectedError inputs[] =
    {
        {
            "<",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            ">",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "+",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "1. e4e5",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. e41...",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "1. e4+e5",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "1. O-O1...",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "1. O-OO-O-O",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "1. e4!!!",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "e4|",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "[]",
            PgnErrorCode::BAD_PGN_TAG
        },
        {
            "[abc]",
            PgnErrorCode::BAD_PGN_TAG
        },
        {
            "[abc \"123\"",
            PgnErrorCode::BAD_PGN_TAG
        },
        {
            "{",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. !",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1... !",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. a4 { abc }!",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. a4 (1. b4 )!",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. a4 (!",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. a4 ;\n!",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. a4 ;\n$1",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1.",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "(",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. e4 ((",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. (",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            ")",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            // 64 levels, but 63 is the max level
            "1.e4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            "(1.a4(1.b4(1.c4(1.d4(1.e4(1.f4(1.g4(1.h4"
            ,
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. e4 ( *",
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "[key \"bad escape\\a\"] 1. e4 ( *",
            PgnErrorCode::BAD_CHARACTER
        },
        {
            "* 1. e4 $1 (1. e3 Nab3 2. Nf3 Bf4 3. Rh6 Qg5)(1. O-O 1... O-O-O 2. Ke2) $2 *", // misplaced nag
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "1. exd3 b1=N 2. bxc8=Q a3a2 { } $2 *", // misplaced nag
            PgnErrorCode::UNEXPECTED_TOKEN
        },
        {
            "",
            PgnErrorCode::OK
        }
    };

    for (const InputAndExpectedError &i : inputs)
    {
        PgnScanner pgnScanner {
            i.input,
            strlen(i.input)
        };

        PgnErrorCode caughtCode { };
        std::string caughtMsg { };

        try
        {
            PgnParser_NullActions actions { };
            PgnParser parser { pgnScanner, actions };
            parser.parse();
        }
        catch (const PgnError &pgnError)
        {
            caughtCode = pgnError.getCode();
            caughtMsg = pgnError.what();
        }

        std::cout << "Caught code: " << (unsigned)caughtCode
                  << ", what: " << caughtMsg
                  << std::endl;

        if (i.code != caughtCode)
        {
            std::cout << "INPUT: '" << i.input << '\'' << std::endl;
            std::cout << "Exception string: '" << caughtMsg << '\'' << std::endl;
        }

        EXPECT_EQ(i.code, caughtCode);
    }
}
