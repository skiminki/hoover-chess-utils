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

#include "pgnreader.h"
#include "pgnreader-string-utils.h"
#include "pgnreader-error.h"
#include "src/pgnreader-priv.h"

#include "gtest/gtest.h"

#include <array>
#include <cstring>
#include <regex>
#include <string_view>

namespace hoover_chess_utils::pgn_reader::unit_test
{

#define STATIC_ASSERT_AND_TEST(x)               \
    do {                                        \
        static_assert(x);                       \
        EXPECT_TRUE(x);                         \
    } while (false)

namespace
{

// note: expectedError may be PgnErrorCode::OK for no error
void checkForExpectedError(std::string_view pgn, PgnErrorCode expectedError)
{
    std::string errorString { };
    PgnErrorCode errorCode { };
    bool errorSet { };

    try
    {
        PgnReaderActions nullActions { };
        PgnReader::readFromMemory(
            pgn, nullActions,
            PgnReaderActionFilter {
                PgnReaderActionClass::PgnTag,
                PgnReaderActionClass::Move,
                PgnReaderActionClass::NAG,
                PgnReaderActionClass::Variation,
                PgnReaderActionClass::Comment });
    }
    catch (const PgnError &ex)
    {
        errorString = ex.what();
        errorCode = ex.getCode();
    }

    EXPECT_EQ(static_cast<std::uint32_t>(errorCode), static_cast<std::uint32_t>(expectedError))
        << std::format("PGN input:\n------\n{}\n------\n", pgn);

    if (errorCode == PgnErrorCode::OK)
    {
        EXPECT_FALSE(errorSet);
    }
    else
    {
        std::cout << "- " << errorString << std::endl;

        // expect that the error string has exactly one 'Line <number>: ' at its
        // beginning
        const std::basic_regex<char> lineNumberMatcher {
            std::regex("Line [[:digit:]]+: Error [[:digit:]]+ \\(.*\\): .*",
                       std::regex::extended) };

        std::cmatch match { };
        EXPECT_TRUE(std::regex_match(errorString.c_str(), match, lineNumberMatcher))
                    << std::format("Bad error string format: {}", errorString);
    }
}

}

TEST(PgnReader, errors)
{
    checkForExpectedError(
        "",
        PgnErrorCode::OK);

    // PGN tag errors
    checkForExpectedError(
        "^",
        PgnErrorCode::BAD_CHARACTER);
    checkForExpectedError(
        "[",
        PgnErrorCode::BAD_PGN_TAG);
    checkForExpectedError(
        "[\"key\" \"value\"]",
        PgnErrorCode::BAD_PGN_TAG);
    checkForExpectedError(
        "[key value]",
        PgnErrorCode::BAD_PGN_TAG);
    checkForExpectedError(
        "[key \"value\" \"value\"]",
        PgnErrorCode::BAD_PGN_TAG);

    // header without movetext
    checkForExpectedError(
        "[key \"value\"]",
        PgnErrorCode::UNEXPECTED_TOKEN);

    // unterminated comment
    checkForExpectedError(
        "{",
        PgnErrorCode::UNEXPECTED_TOKEN);

    // bad/unexpected move nums
    checkForExpectedError(
        "1.. ..",
        PgnErrorCode::BAD_CHARACTER);
    checkForExpectedError(
        ". a4",
        PgnErrorCode::BAD_CHARACTER);
    checkForExpectedError(
        "2. a4",
        PgnErrorCode::UNEXPECTED_MOVE_NUM);

    // bad moves
    checkForExpectedError(
        "1. a5 *",
        PgnErrorCode::ILLEGAL_MOVE);
    checkForExpectedError(
        "1. Ta5 *",
        PgnErrorCode::BAD_CHARACTER);
    checkForExpectedError(
        "1. d4 d5 2. Nf3 Nf6 3. Nd2 *",
        PgnErrorCode::AMBIGUOUS_MOVE);
    checkForExpectedError(
        "1. d4 (1. e4 *",
        PgnErrorCode::UNEXPECTED_TOKEN);
    checkForExpectedError(
        "(1. e4 *",
        PgnErrorCode::UNEXPECTED_TOKEN);
    checkForExpectedError(
        "1. e9 *",
        PgnErrorCode::BAD_CHARACTER);
    checkForExpectedError(
        "1. e3??? *",
        PgnErrorCode::BAD_CHARACTER);

    // bad nag
    checkForExpectedError(
        "1. e3 $256 *",
        PgnErrorCode::BAD_CHARACTER);

    // bad move number
    checkForExpectedError(
        "0. e3 *",
        PgnErrorCode::BAD_CHARACTER);

    checkForExpectedError(
        "4294967296. e3 *",
        PgnErrorCode::BAD_CHARACTER);

    // bad fen
    checkForExpectedError(
        "[FEN \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNRR w KQkq - 0 1\"]",
        PgnErrorCode::BAD_FEN);

    checkForExpectedError(
        "[FEN \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 2\"] *",
        PgnErrorCode::BAD_FEN);
}

namespace
{

struct CallBackStats
{
    std::uint32_t count_gameStart;
    std::uint32_t count_pgnTag;
    std::uint32_t count_moveTextSection;
    std::uint32_t count_comment;
    std::uint32_t count_gameTerminated;
    std::uint32_t count_setBoardReferences;
    std::uint32_t count_afterMove;
    std::uint32_t count_nag;
    std::uint32_t count_variationStart;
    std::uint32_t count_variationEnd;
};

struct CallBackCollectorActions : public PgnReaderActions
{
    CallBackStats stats { };

    void gameStart() override
    {
        ++stats.count_gameStart;
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        static_cast<void>(key);
        static_cast<void>(value);

        ++stats.count_pgnTag;
    }

    void moveTextSection() override
    {
        ++stats.count_moveTextSection;
    }

    void comment(std::string_view comment) override
    {
        static_cast<void>(comment);

        ++stats.count_comment;
    }

    void gameTerminated(PgnResult result) override
    {
        static_cast<void>(result);

        ++stats.count_gameTerminated;
    }

    void setBoardReferences(const ChessBoard &curBoard, const ChessBoard &prevBoard) override
    {
        static_cast<void>(curBoard);
        static_cast<void>(prevBoard);

        ++stats.count_setBoardReferences;
    }

    void afterMove(Move move) override
    {
        static_cast<void>(move);

        ++stats.count_afterMove;
    }

    void nag(std::uint8_t nagNum) override
    {
        static_cast<void>(nagNum);

        ++stats.count_nag;
    }

    void variationStart() override
    {
        ++stats.count_variationStart;
    }

    void variationEnd() override
    {
        ++stats.count_variationEnd;
    }
};

CallBackStats collectCallBackStats(std::string_view pgn, PgnReaderActionFilter filter)
{
    CallBackCollectorActions actions { };

    PgnReader::readFromMemory(pgn, actions, filter);

    return actions.stats;
}

}

TEST(PgnReader, filters)
{
    std::string_view pgn {
        ";\n" // this doesn't trigger an action, since it's an empty single line comment
        "[PgnTag1 \"PgnValue1\"] ; comment 1\n"
        "[PgnTag2 \"PgnValue2\"] { comment 2 }\n"
        "\n"
        "{ Starting comment }\n"
        "1. d4! { Move comment}\n"
        "({ Variation comment } 1. e4!? d5?!\n"
        "({ Subvariation starting comment }\n"
        "1... c5 $1 { Subvariation comment 2}))(1. c4 c5)(1. Nf3)(1. d4 g6 2. Bf4 Nf6 3. Nc3 Bg7 4. Qd2 O-O 5. O-O-O) 1... d5 2. c4 c6 3. Nf3 e6 4. cxd5 { End comment }\n"
        "*\n"
    };

    for (bool enablePgnTag : { false, true })
        for (bool enableMove : { false, true })
            for (bool enableNAG : { false, true })
                for (bool enableVariation : { false, true })
                    for (bool enableComment : { false, true })
                    {
                        PgnReaderActionFilter filter { };
                        filter.set(PgnReaderActionClass::PgnTag, enablePgnTag);
                        filter.set(PgnReaderActionClass::Move, enableMove);
                        filter.set(PgnReaderActionClass::NAG, enableNAG);
                        filter.set(PgnReaderActionClass::Variation, enableVariation);
                        filter.set(PgnReaderActionClass::Comment, enableComment);

                        const CallBackStats stats { collectCallBackStats(pgn, filter) };

                        EXPECT_EQ(stats.count_gameStart, 1U);
                        EXPECT_EQ(stats.count_pgnTag, enablePgnTag ? 2U : 0U);
                        EXPECT_EQ(stats.count_moveTextSection, 1U);
                        EXPECT_EQ(stats.count_comment, enableComment ? (enableVariation ? 8U : 5U) : 0U);
                        EXPECT_EQ(stats.count_gameTerminated, 1U);
                        EXPECT_EQ(stats.count_setBoardReferences, enableMove ? 1U : 0U);
                        EXPECT_EQ(stats.count_afterMove, enableMove ? (enableVariation ? 22U : 7U) : 0U);
                        EXPECT_EQ(stats.count_nag, (enableMove && enableNAG) ? (enableVariation ? 4U : 1U) : 0U);
                        EXPECT_EQ(stats.count_variationStart, enableVariation ? 5U : 0U);
                        EXPECT_EQ(stats.count_variationEnd, enableVariation ? 5U : 0U);
                    }
}

TEST(PgnReader, filters_cornerCases1)
{
    std::string_view pgn {
        "[FEN \"r2k4/1P6/8/8/8/3K4/8/8 w - - 0 1\"]\n"
        "1. b8=Q (1. bxa8=Q) *\n"
    };

    for (bool enableMove : { false, true })
        for (bool enableVariation : { false, true })
        {
            PgnReaderActionFilter filter { };
            filter.set(PgnReaderActionClass::Move, enableMove);
            filter.set(PgnReaderActionClass::Variation, enableVariation);

            const CallBackStats stats { collectCallBackStats(pgn, filter) };

            EXPECT_EQ(stats.count_afterMove, enableMove ? (enableVariation ? 2U : 1U) : 0U);
            EXPECT_EQ(stats.count_variationStart, enableVariation ? 1U : 0U);
            EXPECT_EQ(stats.count_variationEnd, enableVariation ? 1U : 0U);
        }
}

TEST(PgnReader, filters_cornerCases2)
{
    std::string_view pgn {
        "[FEN \"r2k4/1P6/8/8/8/3K4/8/8 w - - 0 1\"]\n"
        "1. bxa8=Q (1. b8=Q) *\n"
    };

    for (bool enableMove : { false, true })
        for (bool enableVariation : { false, true })
        {
            PgnReaderActionFilter filter { };
            filter.set(PgnReaderActionClass::Move, enableMove);
            filter.set(PgnReaderActionClass::Variation, enableVariation);

            const CallBackStats stats { collectCallBackStats(pgn, filter) };

            EXPECT_EQ(stats.count_afterMove, enableMove ? (enableVariation ? 2U : 1U) : 0U);
            EXPECT_EQ(stats.count_variationStart, enableVariation ? 1U : 0U);
            EXPECT_EQ(stats.count_variationEnd, enableVariation ? 1U : 0U);
        }
}

TEST(PgnReader, filters_cornerCases3)
{
    std::string_view pgn {
        "[FEN \"3k4/r7/1P6/8/8/3K4/8/8 w - - 0 1\"]\n"
        "1. b7 (1. bxa7) *\n"
    };

    for (bool enableMove : { false, true })
        for (bool enableVariation : { false, true })
        {
            PgnReaderActionFilter filter { };
            filter.set(PgnReaderActionClass::Move, enableMove);
            filter.set(PgnReaderActionClass::Variation, enableVariation);

            const CallBackStats stats { collectCallBackStats(pgn, filter) };

            EXPECT_EQ(stats.count_afterMove, enableMove ? (enableVariation ? 2U : 1U) : 0U);
            EXPECT_EQ(stats.count_variationStart, enableVariation ? 1U : 0U);
            EXPECT_EQ(stats.count_variationEnd, enableVariation ? 1U : 0U);
        }
}

TEST(PgnReader, commentsAtTheBeginning)
{
    std::string_view pgn {
        "; comment\n{ comment } *"
    };

    const CallBackStats stats { collectCallBackStats(pgn, PgnReaderActionFilter { PgnReaderActionClass::Comment }) };

    EXPECT_EQ(stats.count_gameTerminated, 1U);
    EXPECT_EQ(stats.count_comment, 2U);

}

TEST(PgnReader, commentsAtTheEnd)
{
    std::string_view pgn {
        "* ; comment\n{ comment }"
    };

    const CallBackStats stats { collectCallBackStats(pgn, PgnReaderActionFilter { PgnReaderActionClass::Comment }) };

    EXPECT_EQ(stats.count_gameTerminated, 1U);
    EXPECT_EQ(stats.count_comment, 2U);
}

TEST(PgnReader, commentsBetweenTags)
{
    std::string_view pgn {
        "[tag1 \"value1\"] ;comment 1\n[tag2 \"value2\"] { comment2 } [tag3 \"value3\"] *"
    };

    const CallBackStats stats {
        collectCallBackStats(
            pgn,
            PgnReaderActionFilter { PgnReaderActionClass::Comment, PgnReaderActionClass::PgnTag }) };

    EXPECT_EQ(stats.count_gameTerminated, 1U);
    EXPECT_EQ(stats.count_pgnTag, 3U);
    EXPECT_EQ(stats.count_comment, 2U);
}

TEST(PgnReader, emptyCommentsDontCount)
{
    std::string_view pgn {
        "; \n[tag1 \"value1\"] 1. e4 ; \n*"
    };

    const CallBackStats stats {
        collectCallBackStats(
            pgn,
            PgnReaderActionFilter { PgnReaderActionClass::Comment, PgnReaderActionClass::Move, PgnReaderActionClass::PgnTag }) };

    EXPECT_EQ(stats.count_gameTerminated, 1U);
    EXPECT_EQ(stats.count_afterMove, 1U);
    EXPECT_EQ(stats.count_pgnTag, 1U);
    EXPECT_EQ(stats.count_comment, 0U);

}

TEST(PgnReader, singleLineComments)
{
    std::string_view pgn {
        "; 123\n[tag1 \"value1\"] 1. e4 ;345 \n;def\n*"
    };

    const CallBackStats stats {
        collectCallBackStats(
            pgn,
            PgnReaderActionFilter { PgnReaderActionClass::Comment, PgnReaderActionClass::Move, PgnReaderActionClass::PgnTag }) };

    EXPECT_EQ(stats.count_gameTerminated, 1U);
    EXPECT_EQ(stats.count_afterMove, 1U);
    EXPECT_EQ(stats.count_pgnTag, 1U);
    EXPECT_EQ(stats.count_comment, 3U);

}

namespace
{
struct PgnTagCollectorActions : public PgnReaderActions
{
    std::vector<std::string> tagKeys;
    std::vector<std::string> tagValues;

    void pgnTag(std::string_view key, std::string_view value) override
    {
        tagKeys.push_back(std::string { key });
        tagValues.push_back(std::string { value });
    }
};


}

TEST(PgnReader, pgnTagPairEscape)
{
    std::string_view pgn {
        "[TestTag \"TestTagValue\\\"\"]\n"
        "*\n"
    };

    PgnTagCollectorActions actions { };
    PgnReader::readFromMemory(pgn, actions, PgnReaderActionFilter { PgnReaderActionClass::PgnTag });

    EXPECT_EQ(actions.tagKeys.size(), 1U);
    EXPECT_EQ(actions.tagValues.size(), 1U);
    EXPECT_EQ(actions.tagKeys.at(0), "TestTag");
    EXPECT_EQ(actions.tagValues.at(0), "TestTagValue\"");
}

TEST(PgnReader, filters_unsupportedActionClass)
{
    std::string_view pgn { "*\n" };

    PgnReaderActions actions { };

    EXPECT_THROW(
        PgnReader::readFromMemory(pgn, actions, PgnReaderActionFilter { PgnReaderActionClass { 31U } }),
        PgnError);
}

namespace
{

enum class MoveTypeClass
{
    REGULAR,
    EN_PASSANT,
    PROMOTION,
    CASTLING,
};

class PositionVerifierActions : public PgnReaderActions
{
private:
    const ChessBoard *m_curBoard { };
    const ChessBoard *m_prevBoard { };

    std::string_view m_expectedFenAfterMove { };

    MoveTypeClass m_expectedMoveTypeClass { };

public:
    PositionVerifierActions(std::string_view expectedFenAfterMove, MoveTypeClass expectedMoveTypeClass) :
        m_expectedFenAfterMove { expectedFenAfterMove },
        m_expectedMoveTypeClass { expectedMoveTypeClass }
    {
    }

    void setBoardReferences(const ChessBoard &curBoard, const ChessBoard &prevBoard) override
    {
        m_curBoard = &curBoard;
        m_prevBoard = &prevBoard;
    }

    void afterMove(Move move) override
    {
        EXPECT_EQ(move.isRegularMove(),   (m_expectedMoveTypeClass == MoveTypeClass::REGULAR));
        EXPECT_EQ(move.isEnPassantMove(), (m_expectedMoveTypeClass == MoveTypeClass::EN_PASSANT));
        EXPECT_EQ(move.isPromotionMove(), (m_expectedMoveTypeClass == MoveTypeClass::PROMOTION));
        EXPECT_EQ(move.isCastlingMove(),  (m_expectedMoveTypeClass == MoveTypeClass::CASTLING));

        ChessBoard expectedBoard { };
        expectedBoard.loadFEN(m_expectedFenAfterMove);

        const bool equals { expectedBoard == *m_curBoard };
        EXPECT_TRUE(equals);

        if (!equals)
        {
            std::cout << "Expected:" << std::endl;
            expectedBoard.printBoard();

            std::cout << std::endl << "Got:" << std::endl;
            m_curBoard->printBoard();
        }
    }
};

void testMove(
    std::string_view fen,
    std::string_view move,
    MoveTypeClass expectedClass,
    std::string_view fenAfterMove)
{
    std::string pgn { };

    pgn = std::format("[FEN \"{}\"]\n{}\n*", fen, move);

    PositionVerifierActions actions { fenAfterMove, expectedClass };
    PgnReader::readFromMemory(
        pgn, actions,
        PgnReaderActionFilter { PgnReaderActionClass::Move });
}

}

TEST(PgnReader, moveFormats)
{
    // pawn advance moves
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "e3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/8/4P3/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "e4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "ee4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "2e4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "e2e4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Pe4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Pee4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "P2e4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Pe2e4",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1");

    // pawn capture moves
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "exd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "4xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "e4xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "Pe4xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");

    // en-passant capture
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "xc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "Pxc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "dxc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "Pdxc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "P5xc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3",
        "Pd5xc6",
        MoveTypeClass::EN_PASSANT,
        "rnbqkbnr/pp2pppp/2P5/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");

    // non-capturing promos
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "c8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "c8=R",
        MoveTypeClass::PROMOTION,
        "1bR5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "c8=B",
        MoveTypeClass::PROMOTION,
        "1bB5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "c8=N",
        MoveTypeClass::PROMOTION,
        "1bN5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pc8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pc7c8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "P7c8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pcc8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "7c8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "cc8=Q",
        MoveTypeClass::PROMOTION,
        "1bQ5/8/6k1/8/8/8/4K3/8 b - - 0 1");

    // capturing promos
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "cxb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "cxb8=R",
        MoveTypeClass::PROMOTION,
        "1R6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "cxb8=B",
        MoveTypeClass::PROMOTION,
        "1B6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "cxb8=N",
        MoveTypeClass::PROMOTION,
        "1N6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pxb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "P7xb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pcxb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "Pc7xb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "7xb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "xb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");
    testMove(
        "1b6/2P5/6k1/8/8/8/4K3/8 w - - 0 1",
        "c7xb8=Q",
        MoveTypeClass::PROMOTION,
        "1Q6/8/6k1/8/8/8/4K3/8 b - - 0 1");

    // knight moves, non-capturing
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Nc3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Nbc3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "N1c3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1");
    testMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Nb1c3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1");

    // knight moves, capturing
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2",
        "Nxd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3N4/8/8/PPPPPPPP/R1BQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2",
        "Ncxd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3N4/8/8/PPPPPPPP/R1BQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2",
        "N3xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3N4/8/8/PPPPPPPP/R1BQKBNR b KQkq - 0 2");
    testMove(
        "rnbqkbnr/ppp1pppp/8/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2",
        "Nc3xd5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pppp/8/3N4/8/8/PPPPPPPP/R1BQKBNR b KQkq - 0 2");

    // bishop moves, non-capturing
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Be7",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/ppppbppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR w KQkq - 2 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Bfe7",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/ppppbppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR w KQkq - 2 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "B8e7",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/ppppbppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR w KQkq - 2 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Bf8e7",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/ppppbppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR w KQkq - 2 3");

    // bishop moves, capturing
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Bxa3",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/pppp1ppp/8/4p3/4P3/b7/PPPP1PPP/R1BQKBNR w KQkq - 0 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "B8xa3",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/pppp1ppp/8/4p3/4P3/b7/PPPP1PPP/R1BQKBNR w KQkq - 0 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Bfxa3",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/pppp1ppp/8/4p3/4P3/b7/PPPP1PPP/R1BQKBNR w KQkq - 0 3");
    testMove(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/N7/PPPP1PPP/R1BQKBNR b KQkq - 1 2",
        "Bf8xa3",
        MoveTypeClass::REGULAR,
        "rnbqk1nr/pppp1ppp/8/4p3/4P3/b7/PPPP1PPP/R1BQKBNR w KQkq - 0 3");

    // rook moves, non-capturing
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rg4",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/6r1/8/6N1/3K4 w - - 1 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rgg4",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/6r1/8/6N1/3K4 w - - 1 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "R5g4",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/6r1/8/6N1/3K4 w - - 1 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rg5g4",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/6r1/8/6N1/3K4 w - - 1 2");

    // rook moves, capturing
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rxg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/3K4 w - - 0 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rgxg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/3K4 w - - 0 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "R5xg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/3K4 w - - 0 2");
    testMove(
        "8/8/4k3/6r1/8/8/6N1/3K4 b - - 0 1",
        "Rg5xg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/3K4 w - - 0 2");

    // queen moves, non-capturing
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qg3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P3/6Q1/PPPP1PPP/RNB1KBNR b KQkq - 1 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qgg3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P3/6Q1/PPPP1PPP/RNB1KBNR b KQkq - 1 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Q4g3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P3/6Q1/PPPP1PPP/RNB1KBNR b KQkq - 1 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qg4g3",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P3/6Q1/PPPP1PPP/RNB1KBNR b KQkq - 1 3");

    // queen moves, capturing
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qxg5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2Q1/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qgxg5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2Q1/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Q4xg5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2Q1/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");
    testMove(
        "rnbqkbnr/ppp1pp1p/8/3p2p1/4P1Q1/8/PPPP1PPP/RNB1KBNR w KQkq - 0 3",
        "Qg4xg5",
        MoveTypeClass::REGULAR,
        "rnbqkbnr/ppp1pp1p/8/3p2Q1/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3");


    // king moves, non-capturing
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Ke1",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/4K3 b - - 1 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Kfe1",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/4K3 b - - 1 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "K1e1",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/4K3 b - - 1 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Kf1e1",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6r1/4K3 b - - 1 1");

    // king moves, capturing
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Kxg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6K1/8 b - - 0 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Kfxg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6K1/8 b - - 0 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "K1xg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6K1/8 b - - 0 1");
    testMove(
        "8/8/4k3/8/8/8/6r1/5K2 w - - 0 1",
        "Kf1xg2",
        MoveTypeClass::REGULAR,
        "8/8/4k3/8/8/8/6K1/8 b - - 0 1");

    // castling white
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1",
        "O-O",
        MoveTypeClass::CASTLING,
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 b kq - 1 1");
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1",
        "O-O-O",
        MoveTypeClass::CASTLING,
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/2KR3R b kq - 1 1");

    // castling black
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 1",
        "O-O",
        MoveTypeClass::CASTLING,
        "r4rk1/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQ - 1 2");
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 1",
        "O-O-O",
        MoveTypeClass::CASTLING,
        "2kr3r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQ - 1 2");

    // check/mate marks should be ignored
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1",
        "O-O+",
        MoveTypeClass::CASTLING,
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 b kq - 1 1");
    testMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1",
        "O-O#",
        MoveTypeClass::CASTLING,
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R4RK1 b kq - 1 1");

}

namespace
{

class NagVerifierActions : public PgnReaderActions
{
private:
    std::uint8_t m_expectedNagNum;

public:
    NagVerifierActions(std::uint8_t expectedNagNum) :
        m_expectedNagNum { expectedNagNum }
    {
    }

    void nag(std::uint8_t nagNum) override
    {
        EXPECT_EQ(m_expectedNagNum, nagNum);
    }
};

void testNag(
    std::string_view nag,
    std::uint8_t expectedNagNum)
{
    std::string pgn { };

    pgn = std::format("1. e4{}\n*", nag);

    NagVerifierActions actions { expectedNagNum };
    PgnReader::readFromMemory(
        pgn, actions,
        PgnReaderActionFilter { PgnReaderActionClass::Move, PgnReaderActionClass::NAG });
}

}

TEST(PgnReader, nags)
{
    // traditional annotations
    testNag("!", 1U);
    testNag("?", 2U);
    testNag("!!", 3U);
    testNag("??", 4U);
    testNag("!?", 5U);
    testNag("?!", 6U);

    // nags
    testNag("$0", 0U);
    testNag("$1", 1U);
    testNag("$100", 100U);
    testNag("$255", 255U);

    testNag(" !", 1U);
    testNag(" $100", 100U);
}

namespace
{

class CommentVerifierActions : public PgnReaderActions
{
private:
    std::string_view m_expectedComment;

public:
    CommentVerifierActions(std::string_view expectedComment) :
        m_expectedComment { expectedComment }
    {
    }

    void comment(std::string_view comment) override
    {
        EXPECT_EQ(m_expectedComment, comment);
    }
};

void testComment(
    std::string_view pgnComment,
    std::string_view expectedComment)
{
    std::string pgn { };

    pgn = std::format("{}\n*", pgnComment);

    CommentVerifierActions actions { expectedComment };
    PgnReader::readFromMemory(
        pgn, actions,
        PgnReaderActionFilter { PgnReaderActionClass::Comment });
}

}

TEST(PgnReader, comments)
{
    // single line comments
    testComment("; 123", "123");
    testComment(";     12    3 \t  ", "12    3");
    testComment(";\t\v  12\t\v  ", "12");

    // brace comments
    testComment("{123}", "123");
    testComment("{     12  3      }", "12  3");
    testComment("{     12   \n     3      }", "12\n     3");
    testComment("{     12   \n\n 1\t\n     3      }", "12\n\n 1\n     3");
    testComment("{\v\t     12   \n\n 1\t\n     3      }", "12\n\n 1\n     3");
}

namespace
{

void testBadCapture(
    std::string_view fen,
    std::string_view src,
    std::string_view capture,
    std::string_view dst)
{

    // first, check that the move fails
    {
        std::string pgn { std::format("[FEN \"{}\"]\n{}{}{}\n*", fen, src, capture, dst) };
        checkForExpectedError(pgn, PgnErrorCode::ILLEGAL_MOVE);
    }

    // second, flip capture mark and check that it passes (i.e., test is good)
    {
        std::string pgn { std::format("[FEN \"{}\"]\n{}{}{}\n*", fen, src, capture == "x" ? "" : "x", dst) };
        checkForExpectedError(pgn, PgnErrorCode::OK);
    }
}

}

TEST(PgnReader, captureMarks_negative)
{
    // test that capture moves are actually capture moves, and non-capture moves
    // are non-capture moves

    // incorrectly flagged captures
    testBadCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "e", "x", "e4");
    testBadCapture(
        "r1bqkb1r/pPpppppp/n6n/8/8/8/PP1PPPPP/RNBQKBNR w KQkq - 1 5",
        "b", "x", "b8=Q");
    testBadCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "N", "x", "f3");
    testBadCapture(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "B", "x", "c4");
    testBadCapture(
        "rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
        "R", "x", "g1");
    testBadCapture(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "Q", "x", "f3");
    testBadCapture(
        "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "K", "x", "e2");

    // incorrectly missing capture marks
    testBadCapture(
        "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "e", "", "d5");
    testBadCapture(
        "rnbqkbnr/ppp2ppp/4p3/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
        "e", "", "d6");
    testBadCapture(
        "r1bqkb1r/pPpppppp/n6n/8/8/8/PP1PPPPP/RNBQKBNR w KQkq - 1 5",
        "b", "", "a8=Q");
    testBadCapture(
        "rnbqkbnr/pppp1ppp/8/4p3/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 2",
        "N", "", "e5");
    testBadCapture(
        "rnbqkbnr/p1pppppp/8/1p6/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "B", "", "b5");
    testBadCapture(
        "rn1qkbnr/ppp1pppp/8/3p4/3P3P/7b/PPP1PPP1/RNBQKBNR w KQkq - 1 3",
        "R", "", "h3");
    testBadCapture(
        "rnbqkbnr/ppppppp1/8/7p/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "Q", "", "h5");
    testBadCapture(
        "rnbqkbnr/ppp2Qpp/3p4/4p3/4P3/8/PPPP1PPP/RNB1KBNR b KQkq - 0 3",
        "K", "", "f7");
}

namespace
{

void testSelfCapture(std::string_view fen, std::string_view move)
{
    std::string pgn { std::format("[FEN \"{}\"]\n{}\n*", fen, move) };
    checkForExpectedError(pgn, PgnErrorCode::ILLEGAL_MOVE);
}

}

TEST(PgnReader, selfCaptures_negative)
{
    testSelfCapture(
        "rnbqkbnr/pppp1ppp/4p3/8/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "dxe3");
    testSelfCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Nxd2");
    testSelfCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Bxd2");
    testSelfCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Rxa2");
    testSelfCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Qxd2");
    testSelfCapture(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "Kxd2");
}

namespace
{

void testAmbiguousMove(std::string_view fen, std::string_view move)
{
    std::string pgn { std::format("[FEN \"{}\"]\n{}\n*", fen, move) };
    checkForExpectedError(pgn, PgnErrorCode::AMBIGUOUS_MOVE);
}

}

TEST(PgnReader, ambiguousMoves)
{
    testAmbiguousMove(
        "rnbqkbnr/ppp2ppp/3p4/4p3/3P1P2/8/PPP1P1PP/RNBQKBNR w KQkq - 0 3",
        "Pxe5");
    testAmbiguousMove(
        "rnbqkbnr/1pp3pp/p2p1p2/3PpP2/8/8/PPP1P1PP/RNBQKBNR w KQkq e6 0 5",
        "Pxe6");
    testAmbiguousMove(
        "rnbqk1nr/pppp1ppp/4p3/8/8/5N2/PPPbPPPP/RNBQKB1R w KQkq - 0 4",
        "Nxd2");
    testAmbiguousMove(
        "1k6/8/3r4/8/1B3B2/8/8/2K5 w - - 0 1",
        "Bxd6");
    testAmbiguousMove(
        "1k6/8/3r1R2/8/3R4/8/8/2K5 w - - 0 1",
        "Rxd6");
    testAmbiguousMove(
        "1k6/8/3r1Q2/8/3Q4/8/8/2K5 w - - 0 1",
        "Qxd6");
    testAmbiguousMove(
        "3k4/8/8/8/3pPp2/8/8/3K4 b - e3 0 1",
        "Pxe3");
    testAmbiguousMove(
        "8/8/8/3k4/3pPp2/8/8/3K4 b - e3 0 1",
        "Pxe3");
}

TEST(PgnReader, coverPgnReaderNullActions)
{
    // PGN with one of everything
    std::string_view pgn {
        "[Tag \"Key\"]\n"
        "{ Comment }\n"
        "1. e4! (1. d4!!)\n"
        "1/2-1/2"
    };


    checkForExpectedError(pgn, PgnErrorCode::OK);
}

namespace
{

void testIllegalMove(std::string_view fen, std::string_view move)
{
    std::string pgn { std::format("[FEN \"{}\"]\n{}\n*", fen, move) };
    checkForExpectedError(pgn, PgnErrorCode::ILLEGAL_MOVE);
}

}

TEST(PgnReader, illegalCastlingMoves)
{
    // castling but the board doesn't allow it
    testIllegalMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "O-O");
    testIllegalMove(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "O-O-O");

    // castling but no rights
    testIllegalMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQq - 0 1",
        "O-O");
    testIllegalMove(
        "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQk - 0 1",
        "O-O-O");
}

namespace
{
class CommentMoveTextOrderVerifier : public PgnReaderActions
{
private:
    bool m_inMoveText { };

public:
    CommentMoveTextOrderVerifier()
    {
    }

    void moveTextSection() override
    {
        m_inMoveText = true;
    }

    void comment([[maybe_unused]] std::string_view comment) override
    {
        EXPECT_TRUE(m_inMoveText);
    }
};
}

TEST(PgnReader, startMoveCommentAndMoveTextOrdering)
{
    std::string_view pgn {
        "[Tag \"Key\"]\n"
        "{ Comment }\n"
        "1. e4\n"
        "1/2-1/2"
    };

    CommentMoveTextOrderVerifier actions { };
    PgnReader::readFromMemory(
        pgn, actions,
        PgnReaderActionFilter { PgnReaderActionClass::Comment });
}

TEST(PgnReaderActionFilter, sanity)
{
    PgnReaderActionFilter filter { };

    // check that we're handling nonsense values gracefully (i.e., ignoring)
    filter.set(PgnReaderActionClass { 64U }, true);
    EXPECT_EQ(filter.getBitMask(), 0U);
}

TEST(PgnReaderActionCompileTimeFilter, sanity)
{
    using Filter = PgnReaderActionCompileTimeFilter<PgnReaderActionClass::Move, PgnReaderActionClass::NAG>;

    STATIC_ASSERT_AND_TEST(Filter::isEnabled(PgnReaderActionClass::Move));
    STATIC_ASSERT_AND_TEST(Filter::isEnabled(PgnReaderActionClass::NAG));
    STATIC_ASSERT_AND_TEST(!Filter::isEnabled(PgnReaderActionClass::Comment));
    STATIC_ASSERT_AND_TEST(!Filter::isEnabled(PgnReaderActionClass { 64U }));

    STATIC_ASSERT_AND_TEST(
        Filter::actionsToBitmask(PgnReaderActionClass { 0U },
                                 PgnReaderActionClass { 2U },
                                 PgnReaderActionClass { 4U }) == 1U + 4U + 16U);
}

TEST(PgnReader, moveNumbers)
{
    // all these are ok
    constexpr std::string_view validPgns[] {
        "1 e4 *",
        "1. e4 *",
        "1.. e4 *",
        "1... e4 *",
        "1 . e4 *",
        "1 .. e4 *",
        "1 ... e4 *",
        "1 1. 1.. 1... 1.... e4 *",
    };

    PgnReaderActionFilter filter { PgnReaderActionClass::Move };
    PgnReaderActions nullActions { };

    for (const std::string_view &pgn : validPgns)
    {
        EXPECT_NO_THROW(PgnReader::readFromMemory(pgn, nullActions, filter));
    }

}

}
