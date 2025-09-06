// Hoover Chess Utilities / TCEC PGN compactifier
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
#include "version.h"

#include "memory-mapped-file.h"
#include "output-buffer.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <iostream>
#include <string>
#include <vector>

namespace hoover_chess_utils::utils
{
namespace
{

static constexpr bool debugMode { false };


void printHelp()
{
    std::cout << "TCEC games PGN processing tool: full PGN file to compact PGN file (" << hoover_chess_utils::pgn_reader::getVersionString() << ')' << std::endl;
    std::cout << std::endl;
    std::puts("Usage: hoover-compactify-tcec-pgn <PGN-file>");
}

enum class BookDetectionMode : std::uint8_t
{
    NONE,      // Out of book or forced book exit
    NORMAL,    // Normal
    THORESEN2, // Thoresen tournament 2 style, also used in Thoresen match 1-3
    OVERRIDE,  // Manual override
};

class CompactifierActions : public pgn_reader::PgnReaderActions
{
private:
    const pgn_reader::ChessBoard *m_board { };

    pgn_reader::ChessBoard m_initialBoard { };

    std::uint32_t m_gameNo { };

    // PGN tags of the game
    std::vector<std::tuple<std::string, std::string>> m_pgnTags { };
    std::string m_pgnResultTag { };

    // moves of the game
    std::vector<pgn_reader::Move> m_moves { };
    BookDetectionMode m_bookDetectionMode { BookDetectionMode::NORMAL };
    std::size_t m_lastBookPly { };

    OutputBuffer out { };


    static constexpr std::string_view ctLiteralBookExitTrailingSpace { "{ Book exit } " };
    static constexpr std::string_view ctLiteralBookExitPrefixSpace { " { Book exit }" };
    static constexpr std::string_view ctLiteralResultWhiteWin { "1-0" };
    static constexpr std::string_view ctLiteralResultDraw { "1/2-1/2" };
    static constexpr std::string_view ctLiteralResultBlackWin { "0-1" };
    static constexpr std::string_view ctLiteralDoubleNewLine { "\n\n" };

    void writeEscapedPgnValue(const std::string_view value)
    {
        for (char c : value)
        {
            if (c == '\\' || c == '"')
                out.write('\\');

            out.write(c);
        }
    }

    void printMoves()
    {
        pgn_reader::ChessBoard board { m_initialBoard };
        bool forceMoveNum { true };

        if (board.getCurrentPlyNum() == m_lastBookPly)
        {
            out.write(ctLiteralBookExitTrailingSpace);
        }

        for (size_t moveIndex { }; moveIndex < m_moves.size(); ++moveIndex)
        {
            if (moveIndex != 0U)
                out.write(' ');

            const std::uint32_t plyNum { board.getCurrentPlyNum() };
            if (forceMoveNum || pgn_reader::colorOfPly(plyNum) == pgn_reader::Color::WHITE)
            {
                out.write(pgn_reader::StringUtils::moveNumToString(pgn_reader::moveNumOfPly(plyNum), pgn_reader::colorOfPly(plyNum)));
                out.write(' ');

                forceMoveNum = false;
            }

            out.write(pgn_reader::StringUtils::moveToSanAndPlay(board, m_moves[moveIndex]));

            if (board.getCurrentPlyNum() == m_lastBookPly)
            {
                out.write(ctLiteralBookExitPrefixSpace);
                forceMoveNum = true;
            }
        }
    }

    static bool pgnTagKeyLess(const std::string &lhs, const std::string &rhs)
    {
#define C(key)                                  \
        {                                       \
            const bool rhsIsKey { rhs == key }; \
            if (rhsIsKey)                       \
                return false;                   \
            const bool lhsIsKey { lhs == key }; \
            if (lhsIsKey)                       \
                return true;                    \
        }

        // fixed-order keys
        // TODO: might be faster to do perfect hashing
        C("Event");
        C("Site");
        C("Date");
        C("Round");
        C("White");
        C("Black");
        C("Result");
        C("WhiteElo");
        C("BlackElo");
        C("ECO");
        C("Opening");
        C("Variation");
        C("TimeControl");
        C("Termination");
        C("PlyCount");

        return lhs < rhs;
    }

public:
    void setBoardReferences(
        const pgn_reader::ChessBoard &curBoard,
        const pgn_reader::ChessBoard &prevBoard) override
    {
        m_board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void gameStart() override
    {
        ++m_gameNo;
        m_pgnTags.clear();
        m_pgnTags.reserve(32U);
        m_pgnResultTag.clear();
        m_moves.clear();
        m_moves.reserve(512U);
        m_bookDetectionMode = BookDetectionMode::NONE;
        m_lastBookPly = 0U;

        if constexpr(debugMode)
        {
            std::fputs(std::format("-- game {}\n", m_gameNo).c_str(), stderr);
        }
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        // determine book detection mode
        if (key == "Site")
        {
            m_bookDetectionMode = BookDetectionMode::NORMAL;

            if (value.starts_with("https://tcec-chess.com/#season=0&div=thort2&") ||
                value.starts_with("https://tcec-chess.com/#season=0&div=thorm1&") ||
                value.starts_with("https://tcec-chess.com/#season=0&div=thorm2&") ||
                value.starts_with("https://tcec-chess.com/#season=0&div=thorm3&"))
            {
                m_bookDetectionMode = BookDetectionMode::THORESEN2;
            }

            if (value == "https://tcec-chess.com/#season=0&div=thort1&game=29")
            {
                // override based on best guess
                m_bookDetectionMode = BookDetectionMode::OVERRIDE;
                m_lastBookPly = 16U;
            }
        }

        if (key == "Result")
            m_pgnResultTag = value;

        m_pgnTags.push_back(std::make_tuple(std::string { key }, std::string { value }));
    }

    void moveTextSection() override
    {
        m_initialBoard = *m_board;

        // Initial position is always book position unless we're overriding
        if (m_bookDetectionMode != BookDetectionMode::OVERRIDE)
        {
            m_lastBookPly = m_initialBoard.getCurrentPlyNum();
        }

        if (false)
        {
            std::sort(
                m_pgnTags.begin(), m_pgnTags.end(),
                [] (const auto &lhs, const auto &rhs) -> bool
                {
                    return pgnTagKeyLess(std::get<0>(lhs), std::get<0>(rhs));
                });
        }

        for (const auto &tagPair : m_pgnTags)
        {
            out.write('[');
            out.write(std::string_view(std::get<0>(tagPair)));
            out.write(std::string_view { " \"" });
            writeEscapedPgnValue(std::string_view(std::get<1>(tagPair)));
            out.write(std::string_view { "\"]\n" });
        }

        out.write('\n');
    }

    void afterMove(pgn_reader::Move m) override
    {
        m_moves.push_back(m);
    }

    void comment(std::string_view comment) override
    {
        if (m_bookDetectionMode == BookDetectionMode::OVERRIDE)
            return;

        // Ignore comment before first move
        if (m_board->getCurrentPlyNum() == m_initialBoard.getCurrentPlyNum())
        {
            return;
        }

        switch (m_bookDetectionMode)
        {
            case BookDetectionMode::NONE:
                break;

            case BookDetectionMode::NORMAL:
            {
                if (comment == "book" ||
                    comment.starts_with("book,") ||
                    comment == "B 0" ||
                    comment == "0 B/ 0" ||
                    comment == "ev=0,d=1,tl=02:00:00" ||
                    comment == "Eval = 0, Depth = 1, TimeLeft = 02:00:00" ||
                    comment == "Eval = 0.00, Depth = 1, TimeLeft = 02:00:00" ||
                    comment == "ev=0.00,d=1,tl=02:00:00" ||
                    comment.starts_with("Eval = 0.00, Depth = 1, MoveTime = 00:00:00") ||
                    comment.starts_with("ev=0.00, d=1, mt=00:00:00"))
                {
                    // book move
                    m_lastBookPly = m_board->getCurrentPlyNum();
                }
                else
                {
                    // not a book move, stop detecting
                    m_bookDetectionMode = BookDetectionMode::NONE;
                }

                break;
            }

            case BookDetectionMode::THORESEN2:
                // In Thoresen tournament 2 mode, all moves without a comment are book moves.
                m_lastBookPly = m_board->getCurrentPlyNum() - 1U;
                m_bookDetectionMode = BookDetectionMode::NONE;
                break;

            default:
                break;
        }
    }

    void gameTerminated(pgn_reader::PgnResult result) override
    {
        printMoves();

        if (!m_moves.empty())
            out.write(' ');

        if (!m_pgnResultTag.empty())
        {
            pgn_reader::PgnResult tagResult { };

            if (m_pgnResultTag == "1-0")
                tagResult = pgn_reader::PgnResult::WHITE_WIN;
            else if (m_pgnResultTag == "0-1")
                tagResult = pgn_reader::PgnResult::BLACK_WIN;
            else if (m_pgnResultTag == "1/2-1/2")
                tagResult = pgn_reader::PgnResult::DRAW;
            else if (m_pgnResultTag == "*")
                tagResult = pgn_reader::PgnResult::UNKNOWN;
            else
            {
                throw std::runtime_error(
                    std::format("Bad result tag: '{}'", m_pgnResultTag));
            }

            if (tagResult != result)
            {
                throw std::runtime_error(std::format("PGN tag mismatches with the game result."));
            }
        }

        switch (result)
        {
            case pgn_reader::PgnResult::WHITE_WIN:
                out.write(ctLiteralResultWhiteWin);
                break;

            case pgn_reader::PgnResult::BLACK_WIN:
                out.write(ctLiteralResultBlackWin);
                break;

            case pgn_reader::PgnResult::DRAW:
                out.write(ctLiteralResultDraw);
                break;

            case pgn_reader::PgnResult::UNKNOWN:
                out.write('*');
                break;
        }

        out.write(ctLiteralDoubleNewLine);
    }
};

int compactifyTcecPgnMain(int argc, char **argv) noexcept
{
    if (argc != 2)
    {
        printHelp();
        return 127;
    }

    try
    {
        using pgn_reader::PgnReader;
        using pgn_reader::PgnReaderActionClass;
        using pgn_reader::PgnReaderActionFilter;

        MemoryMappedFile pgnContents;
        CompactifierActions actions { };

        pgnContents.map(argv[1], true, false);
        pgn_reader::PgnReader::readFromMemory(
            pgnContents.getStringView(),
            actions,
            PgnReaderActionFilter { PgnReaderActionClass::Move, PgnReaderActionClass::PgnTag, PgnReaderActionClass::Comment });
        pgnContents.unmap();

        return 0;
    }
    catch (const pgn_reader::PgnError &pgnError)
    {
        std::fputs(pgnError.what(), stderr);
        std::fputs("\n", stderr);

        return 1;
    }
    catch (const std::exception &ex)
    {
        std::fputs(ex.what(), stderr);
        std::fputs("\n", stderr);

        return 2;
    }
}

}

}

int main(int argc, char **argv)
{
    return hoover_chess_utils::utils::compactifyTcecPgnMain(argc, argv);
}
