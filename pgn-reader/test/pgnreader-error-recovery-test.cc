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

#include "chessboard.h"
#include "pgnreader.h"
#include "pgnreader-error.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>


namespace hoover_chess_utils::pgn_reader::unit_test
{

namespace
{

class AbortingErrorHandlerActions : public PgnReaderActions
{
public:
    bool m_aborted { false };

    std::uint32_t m_gameStarts { };
    std::uint32_t m_moveTextSections { };
    std::uint32_t m_pgnTags { };
    std::uint32_t m_moves { };
    std::uint32_t m_comments { };
    std::uint32_t m_variationStarts { };
    std::uint32_t m_variationEnds { };
    std::uint32_t m_nags { };
    std::uint32_t m_gameEnds { };
    std::uint32_t m_errors { };

    void gameStart() override
    {
        EXPECT_FALSE(m_aborted);

        ++m_gameStarts;
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        static_cast<void>(key);
        static_cast<void>(value);

        EXPECT_FALSE(m_aborted);

        ++m_pgnTags;
    }

    void moveTextSection() override
    {
        EXPECT_FALSE(m_aborted);

        ++m_moveTextSections;
    }

    void comment(std::string_view comment) override
    {
        static_cast<void>(comment);

        EXPECT_FALSE(m_aborted);

        ++m_comments;
    }

    void gameTerminated(PgnResult result) override
    {
        static_cast<void>(result);

        EXPECT_FALSE(m_aborted);

        ++m_gameEnds;
    }

    void setBoardReferences(const ChessBoard &curBoard, const ChessBoard &prevBoard) override
    {
        static_cast<void>(curBoard);
        static_cast<void>(prevBoard);

        EXPECT_FALSE(m_aborted);
    }

    void afterMove(ChessBoard::Move move) override
    {
        static_cast<void>(move);

        EXPECT_FALSE(m_aborted);

        ++m_moves;
    }

    void nag(std::uint8_t nagNum) override
    {
        static_cast<void>(nagNum);

        EXPECT_FALSE(m_aborted);

        ++m_nags;
    }

    void variationStart() override
    {
        EXPECT_FALSE(m_aborted);

        ++m_variationStarts;
    }

    void variationEnd() override
    {
        EXPECT_FALSE(m_aborted);

        ++m_variationEnds;
    }

    PgnReaderOnErrorAction onError(const PgnError &error, const PgnErrorInfo &additionalInfo) override
    {
        static_cast<void>(error);
        static_cast<void>(additionalInfo);

        EXPECT_FALSE(m_aborted);

        m_aborted = true;

        ++m_errors;

        return PgnReaderOnErrorAction::Abort;
    }
};

}

TEST(PgnReader, ErrorRecoveryPolicy_Abort)
{
    // just a plain old PGN with one of everything before and after abort
    constexpr std::string_view testPgn {
        "[TestTag \"TestValue\"]\n"
        "1. e4? ({Missed opportunity: }1. d4!) 1... Nb6 2. d4 *\n"
    };

    AbortingErrorHandlerActions actions { };

    EXPECT_THROW(
        PgnReader::readFromMemory(
            testPgn,
            actions,
            PgnReaderActionFilter {
                PgnReaderActionClass::PgnTag,
                PgnReaderActionClass::Move,
                PgnReaderActionClass::NAG,
                PgnReaderActionClass::Variation,
                PgnReaderActionClass::Comment }),
        PgnError);

    EXPECT_TRUE(actions.m_aborted);
    EXPECT_EQ(actions.m_errors, 1U);

    EXPECT_EQ(actions.m_gameStarts, 1U);
    EXPECT_EQ(actions.m_moveTextSections, 1U);
    EXPECT_EQ(actions.m_pgnTags, 1U);
    EXPECT_EQ(actions.m_moves, 2U);
    EXPECT_EQ(actions.m_comments, 1U);
    EXPECT_EQ(actions.m_variationStarts, 1U);
    EXPECT_EQ(actions.m_variationEnds, 1U);
    EXPECT_EQ(actions.m_nags, 2U);
    EXPECT_EQ(actions.m_gameEnds, 0U);
}


namespace
{

class ContinuingErrorHandlerActions : public PgnReaderActions
{
public:
    std::uint32_t m_gameStarts { };
    std::uint32_t m_moveTextSections { };
    std::uint32_t m_pgnTags { };
    std::uint32_t m_moves { };
    std::uint32_t m_comments { };
    std::uint32_t m_variationStarts { };
    std::uint32_t m_variationEnds { };
    std::uint32_t m_nags { };
    std::uint32_t m_gameEnds { };
    std::uint32_t m_errors { };

    void gameStart() override
    {
        ++m_gameStarts;
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        static_cast<void>(key);
        static_cast<void>(value);

        ++m_pgnTags;
    }

    void moveTextSection() override
    {
        ++m_moveTextSections;
    }

    void comment(std::string_view comment) override
    {
        static_cast<void>(comment);

        ++m_comments;
    }

    void gameTerminated(PgnResult result) override
    {
        static_cast<void>(result);

        ++m_gameEnds;
    }

    void setBoardReferences(const ChessBoard &curBoard, const ChessBoard &prevBoard) override
    {
        static_cast<void>(curBoard);
        static_cast<void>(prevBoard);
    }

    void afterMove(ChessBoard::Move move) override
    {
        static_cast<void>(move);

        ++m_moves;
    }

    void nag(std::uint8_t nagNum) override
    {
        static_cast<void>(nagNum);

        ++m_nags;
    }

    void variationStart() override
    {
        ++m_variationStarts;
    }

    void variationEnd() override
    {
        ++m_variationEnds;
    }

    PgnReaderOnErrorAction onError(const PgnError &error, const PgnErrorInfo &additionalInfo) override
    {
        static_cast<void>(error);
        static_cast<void>(additionalInfo);

        std::cout << "ContinuingErrorHandlerActions::onError()" << std::endl;

        ++m_errors;

        return PgnReaderOnErrorAction::ContinueFromNextGame;
    }
};

}

TEST(PgnReader, ErrorRecoveryPolicy_ContinueFromNextGame)
{
    // just a plain old PGN with one of everything before and after abort
    constexpr std::string_view testPgn {
        "[TestTag \"TestValue\"]\n"
        "1. e4? ({Missed opportunity: }1. d4!) 1... Nb6 2. d4 *\n"
        "[TestTag \"TestValue\"]\n"
        "1. f4 ({Again, missed opportunity: }1. d4!) 1... garbage 2. d4 *\n"
        ";\n"
        "[TestTag \"TestValue\"]\n"
        "1. f4 ({Again, missed opportunity: }1. d4!) 1... Nh6 2. d4 *\n"
    };

    ContinuingErrorHandlerActions actions { };

    EXPECT_NO_THROW(
        PgnReader::readFromMemory(
            testPgn,
            actions,
            PgnReaderActionFilter {
                PgnReaderActionClass::PgnTag,
                PgnReaderActionClass::Move,
                PgnReaderActionClass::NAG,
                PgnReaderActionClass::Variation,
                PgnReaderActionClass::Comment }));

    EXPECT_EQ(actions.m_errors, 2U);

    EXPECT_EQ(actions.m_gameStarts, 3U);
    EXPECT_EQ(actions.m_moveTextSections, 3U);
    EXPECT_EQ(actions.m_pgnTags, 3U);
    EXPECT_EQ(actions.m_moves, 8U);
    EXPECT_EQ(actions.m_comments, 3U);
    EXPECT_EQ(actions.m_variationStarts, 3U);
    EXPECT_EQ(actions.m_variationEnds, 3U);
    EXPECT_EQ(actions.m_nags, 4U);
    EXPECT_EQ(actions.m_gameEnds, 1U);
}

TEST(PgnReader, ErrorRecoveryPolicy_ContinueFromNextGame_EOF)
{
    // just a plain old PGN with one of everything before and after abort
    constexpr std::string_view testPgn {
        "[TestTag \"TestValue\"]\n"
        "1. e4? ({Missed opportunity: }1. d4!) 1... Nb6 2. d4 *\n"
        "[TestTag \"TestValue\"]\n"
        "1. f4 ({Again, missed opportunity: }1. d4!) 1... garbage and some more garbage 2. d4 c5 { * }\n"
    };

    ContinuingErrorHandlerActions actions { };

    EXPECT_NO_THROW(
        PgnReader::readFromMemory(
            testPgn,
            actions,
            PgnReaderActionFilter {
                PgnReaderActionClass::PgnTag,
                PgnReaderActionClass::Move,
                PgnReaderActionClass::NAG,
                PgnReaderActionClass::Variation,
                PgnReaderActionClass::Comment }));

    EXPECT_EQ(actions.m_errors, 2U);

    EXPECT_EQ(actions.m_gameStarts, 2U);
    EXPECT_EQ(actions.m_moveTextSections, 2U);
    EXPECT_EQ(actions.m_pgnTags, 2U);
    EXPECT_EQ(actions.m_moves, 4U);
    EXPECT_EQ(actions.m_comments, 2U);
    EXPECT_EQ(actions.m_variationStarts, 2U);
    EXPECT_EQ(actions.m_variationEnds, 2U);
    EXPECT_EQ(actions.m_nags, 3U);
    EXPECT_EQ(actions.m_gameEnds, 0U);
}

namespace
{

class BadPolicyErrorHandlerActions : public PgnReaderActions
{
public:
    PgnReaderOnErrorAction onError(const PgnError &error, const PgnErrorInfo &additionalInfo) override
    {
        static_cast<void>(error);
        static_cast<void>(additionalInfo);

        return PgnReaderOnErrorAction { 255U };
    }
};

}

TEST(PgnReader, ErrorRecoveryPolicy_ContinueFromNextGame_BadPolicy)
{
    // trivially broken
    constexpr std::string_view testPgn { "garbage" };

    BadPolicyErrorHandlerActions actions { };

    EXPECT_THROW(
        PgnReader::readFromMemory(
            testPgn,
            actions,
            PgnReaderActionFilter {
                PgnReaderActionClass::PgnTag,
                PgnReaderActionClass::Move,
                PgnReaderActionClass::NAG,
                PgnReaderActionClass::Variation,
                PgnReaderActionClass::Comment }),
        PgnError);
}

}
