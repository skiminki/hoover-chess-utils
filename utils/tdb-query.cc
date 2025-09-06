// Hoover Chess Utilities / TDB query tool
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

#include "compressed-position.h"
#include "memory-mapped-file.h"

#include "pgnreader.h"
#include "pgnreader-string-utils.h"
#include "chessboard.h"
#include "version.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <system_error>

namespace hoover_chess_utils::utils
{

namespace
{

static constexpr bool debugMode { false };

enum class PositionClassification : std::uint8_t
{
    // Results from non-book positions, i.e., engines are freely playing from
    // this position onward. Note: the book exit position is not considered a
    // book position, since the next move is decided by the engines.
    WHITE_WIN = 0U,
    DRAW,
    BLACK_WIN,

    // Results where the next move is still from the book. I.e., not necessarily
    // an engine choice.
    BOOK_WHITE_WIN,
    BOOK_DRAW,
    BOOK_BLACK_WIN,

    NUM_VALUES,
};

// stats for a specific result for a position
struct PositionResultStats
{
    std::size_t numGames;

    // most recent game
    std::string whitePlayer;
    std::string blackPlayer;
    std::string site;
};

// we gather the stats for each unique position given in the input FEN + moves
struct PositionStats
{
    std::array<PositionResultStats, static_cast<std::size_t>(PositionClassification::NUM_VALUES)> resultStats;
};

void printHelp()
{
    std::cout << "TCEC games database query tool for TCEC_hoover_bot (" << hoover_chess_utils::pgn_reader::getVersionString() << ')' << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: hoover-tdb-query <PGN-database> <PGN-query> [threads]" << std::endl;
    std::cout << std::endl;
    std::cout << "PGN-database  Compacted TCEC games PGN database file" << std::endl;
    std::cout << "PGN-query     PGN containing a single game. The positions in the PGN are queried" << std::endl;
    std::cout << "              in reverse order." << std::endl;
}

class CollectQueryFilePositionsActions : public pgn_reader::PgnReaderActions
{
private:
    // position to ply num
    std::map<CompressedPosition, std::uint32_t> m_positions;

    const hoover_chess_utils::pgn_reader::ChessBoard *board { };

    void addPosition()
    {
        CompressedPosition cp { };
        PositionCompressor::compress(*board, cp);

        m_positions[cp] = board->getCurrentPlyNum();

        if constexpr (debugMode)
        {
            std::cout << "Adding input position "
                      << pgn_reader::StringUtils::plyNumToString(board->getCurrentPlyNum()).getStringView()
                      << std::endl;

            board->printBoard();
        }
    }

public:
    // compressed position to ply num
    const auto &getPositions() const noexcept
    {
        return m_positions;
    }

    void setBoardReferences(
        const hoover_chess_utils::pgn_reader::ChessBoard &curBoard,
        const hoover_chess_utils::pgn_reader::ChessBoard &prevBoard) override
    {
        board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void moveTextSection() override
    {
        m_positions.clear();
        addPosition();
    }

    void afterMove(pgn_reader::Move) override
    {
        addPosition();
    }
};

class CollectStatisticsActions : public pgn_reader::PgnReaderActions
{
private:
    const std::vector<CompressedPosition> &m_inputPositions;
    std::vector<PositionStats> &m_inputPositionStats;

    // current database game
    const hoover_chess_utils::pgn_reader::ChessBoard *m_board { };
    std::string m_whitePlayer;
    std::string m_blackPlayer;
    std::string m_site;

    // helper used to check which input positions have been seen
    std::vector<PositionClassification> m_inputPositionsSeen;

    std::array<CompressedPosition, 1024U> m_currentGamePositions { };
    std::size_t m_numPositions { };
    std::size_t m_bookEnd { }; // index of first position where players are free to move (i.e., last book position)

    void addCurrentBoard()
    {
        if (m_numPositions < m_currentGamePositions.size())
        {
            PositionCompressor::compress(*m_board, m_currentGamePositions.at(m_numPositions));
            m_numPositions++;
        }
    }

public:
    CollectStatisticsActions(
        const std::vector<CompressedPosition> &inputPositions,
        std::vector<PositionStats> &inputPositionStats) noexcept :
        m_inputPositions { inputPositions },
        m_inputPositionStats { inputPositionStats }
    {
        m_inputPositionStats.resize(inputPositions.size());
        m_inputPositionsSeen.resize(inputPositions.size());
    }

    void setBoardReferences(
        const hoover_chess_utils::pgn_reader::ChessBoard &curBoard,
        const hoover_chess_utils::pgn_reader::ChessBoard &prevBoard) override
    {
        m_board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void gameStart() override
    {
        m_whitePlayer.clear();
        m_blackPlayer.clear();
        m_site.clear();
        m_numPositions = 0U;
        m_bookEnd = 0U;
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        if (key == "White")
            m_whitePlayer = value;
        else if (key == "Black")
            m_blackPlayer = value;
        else if (key == "Site")
            m_site = value;
    }

    void moveTextSection() override
    {
        addCurrentBoard();
    }

    void afterMove(pgn_reader::Move) override
    {
        addCurrentBoard();
    }

    void comment(std::string_view comment) override
    {
        if (comment == "Book exit")
        {
            if (m_numPositions == 0U)
                throw std::runtime_error("CollectStatisticsActions::comment: Book position marked but no positions exist");

            m_bookEnd = m_numPositions - 1U;
        }
    }

    void gameTerminated(const pgn_reader::PgnResult result) override
    {
        if (result == pgn_reader::PgnResult::UNKNOWN)
            // skip games with unknown result
            return;

        // mark all input positions as not seen
        std::fill(m_inputPositionsSeen.begin(), m_inputPositionsSeen.end(), PositionClassification::NUM_VALUES);

        // replay positions and mark results
        for (std::size_t i { }; i < m_numPositions; ++i)
        {
            const CompressedPosition &cp { m_currentGamePositions[i] };

            // find the position in input
            const auto j { std::lower_bound(m_inputPositions.begin(), m_inputPositions.end(), cp) };

            // did we find the element?
            if (j == m_inputPositions.end() || *j != cp)
                continue;

            const std::size_t inputPosNum = (j - m_inputPositions.begin());

            PositionClassification &inputPosResult { m_inputPositionsSeen.at(inputPosNum) };

            if (i < m_bookEnd)
            {
                switch (result)
                {
                    case pgn_reader::PgnResult::WHITE_WIN:
                        inputPosResult = PositionClassification::BOOK_WHITE_WIN;
                        break;

                    case pgn_reader::PgnResult::DRAW:
                        inputPosResult = PositionClassification::BOOK_DRAW;
                        break;

                    case pgn_reader::PgnResult::BLACK_WIN:
                        inputPosResult = PositionClassification::BOOK_BLACK_WIN;
                        break;

                    default:
                        throw std::logic_error("CollectStatisticsActions::gameTerminated: bad PgnResult (1)");
                }
            }
            else
            {
                switch (result)
                {
                    case pgn_reader::PgnResult::WHITE_WIN:
                        inputPosResult = PositionClassification::WHITE_WIN;
                        break;

                    case pgn_reader::PgnResult::DRAW:
                        inputPosResult = PositionClassification::DRAW;
                        break;

                    case pgn_reader::PgnResult::BLACK_WIN:
                        inputPosResult = PositionClassification::BLACK_WIN;
                        break;

                    default:
                        throw std::logic_error("CollectStatisticsActions::gameTerminated: bad PgnResult (2)");
                }
            }
        }

        // update input position results
        for (std::size_t i { }; i < m_inputPositions.size(); ++i)
        {
            std::size_t resultIndex { static_cast<std::size_t>(m_inputPositionsSeen.at(i)) };

            if (resultIndex < static_cast<std::size_t>(PositionClassification::NUM_VALUES))
            {
                PositionResultStats &resultStats { m_inputPositionStats.at(i).resultStats[resultIndex] };

                ++resultStats.numGames;
                resultStats.whitePlayer = m_whitePlayer;
                resultStats.blackPlayer = m_blackPlayer;
                resultStats.site = m_site;
            }
        }
    }
};

std::string_view::iterator findNextGameStart(std::string_view fullPgn, std::size_t b)
{
    if (b == 0U || b == fullPgn.size())
        return fullPgn.begin() + b;

    // find next game
    const char *gameStartTag = "\n\n["; // empty line + PGN header tag
    const std::size_t pos { fullPgn.find(gameStartTag, b) };

    if (pos == std::string_view::npos)
        return fullPgn.end();
    else
        return fullPgn.begin() + pos;
}

void collectStatisticsThreadMain(
    const std::vector<CompressedPosition> &positions,
    std::string_view databasePgn,
    std::size_t segmentNo,
    std::size_t numSegments,
    std::vector<PositionStats> &result)
{
    // get the segment of the full pgn for this thread
    std::string_view pgnSegment {
        findNextGameStart(databasePgn, databasePgn.size() * segmentNo / numSegments),
        findNextGameStart(databasePgn, databasePgn.size() * (segmentNo + 1U) / numSegments) };

    if constexpr (debugMode)
    {
        std::cout << std::format("Thread {}: target={} slice=[{}, {})",
                                 segmentNo,
                                 databasePgn.size() * segmentNo / numSegments,
                                 pgnSegment.begin() - databasePgn.begin(),
                                 pgnSegment.end() - databasePgn.begin())
                  << std::endl;
    }

    CollectStatisticsActions actions { positions, result };

    using pgn_reader::PgnReader;
    using pgn_reader::PgnReaderActionClass;
    using pgn_reader::PgnReaderActionFilter;

    PgnReader::readFromMemory(
        pgnSegment,
        actions,
        PgnReaderActionFilter { PgnReaderActionClass::PgnTag, PgnReaderActionClass::Move, PgnReaderActionClass::Comment });
}

std::vector<PositionStats> collectStatistics(
    const std::vector<CompressedPosition> &positions,
    const std::string &dbFileName,
    std::size_t numThreads)
{
    std::vector<std::thread> threads;
    std::vector<std::vector<PositionStats> > threadResults;

    threads.resize(numThreads);
    threadResults.resize(numThreads);

    MemoryMappedFile mmfile { };
    mmfile.map(dbFileName.c_str(), true, false);

    if constexpr (debugMode)
    {
        std::cout << "Launching " << numThreads << " to collect results..." << std::endl;
    }

    for (std::size_t i { }; i < threads.size(); ++i)
    {
        threads.at(i) =
            std::thread(
                collectStatisticsThreadMain,
                std::cref(positions),
                std::string_view(static_cast<const char *>(mmfile.data()), mmfile.size()),
                i,
                numThreads,
                std::ref(threadResults.at(i)));
    }

    for (std::size_t i { }; i < threads.size(); ++i)
    {
        threads.at(i).join();

        if (i > 0U)
        {
            std::vector<PositionStats> &firstThreadResults { threadResults.at(0U) };
            const std::vector<PositionStats> &curThreadResults { threadResults.at(i) };

            for (std::size_t inputPosNum { }; inputPosNum < positions.size(); ++inputPosNum)
            {
                for (std::size_t resultIndex { };
                     resultIndex < static_cast<std::size_t>(PositionClassification::NUM_VALUES);
                     ++resultIndex)
                {
                    const PositionResultStats &curThreadStats { curThreadResults.at(inputPosNum).resultStats.at(resultIndex) };

                    if (curThreadStats.numGames > 0U)
                    {
                        PositionResultStats &sumStats { firstThreadResults.at(inputPosNum).resultStats.at(resultIndex) };

                        sumStats.numGames    += curThreadStats.numGames;
                        sumStats.whitePlayer =  curThreadStats.whitePlayer;
                        sumStats.blackPlayer =  curThreadStats.blackPlayer;
                        sumStats.site        =  curThreadStats.site;
                    }
                }
            }
        }
    }

    if constexpr (debugMode)
    {
        std::cout << "Threads done" << std::endl;
    }

    mmfile.unmap();

    return threadResults.at(0);
}

std::pair<std::vector<CompressedPosition>, std::vector<std::uint32_t> > collectQueryFilePositions(const std::string &fileName)
{
    using pgn_reader::PgnReader;
    using pgn_reader::PgnReaderActionClass;
    using pgn_reader::PgnReaderActionFilter;

    std::array<char, 4096U> buf;
    std::string pgnContext { };

    FILE *f = std::fopen(fileName.c_str(), "rb");
    if (f == nullptr)
    {
        throw std::system_error(errno, std::generic_category(), std::format("Failed to open {}", fileName));
    }

    while (!std::feof(f))
    {
        std::size_t numChars { std::fread(buf.data(), 1U, sizeof buf, f) };

        if (numChars == 0U)
            break;

        pgnContext.append(buf.data(), numChars);
    }

    std::fclose(f);

    CollectQueryFilePositionsActions actions { };
    pgn_reader::PgnReader::readFromMemory(
        pgnContext,
        actions,
        PgnReaderActionFilter { PgnReaderActionClass::Move });

    // linearize the collected positions
    std::vector<CompressedPosition> positions { };
    std::vector<std::uint32_t> positionPlyNums { };

    const auto &collectedPositions { actions.getPositions() };

    positions.reserve(collectedPositions.size());
    positionPlyNums.reserve(collectedPositions.size());

    for (const auto &keyAndPos : collectedPositions)
    {
        positions.emplace(positions.end(), keyAndPos.first);
        positionPlyNums.emplace(positionPlyNums.end(), keyAndPos.second);
    }

    if constexpr(debugMode)
        std::cout << "Input PGN contains " << collectedPositions.size() << " unique positions" << std::endl;

    return std::make_pair(std::move(positions), std::move(positionPlyNums));
}

const char *positionClassificationToString(PositionClassification pc) noexcept
{
    switch (pc)
    {
        case PositionClassification::WHITE_WIN:
            return "1-0";

        case PositionClassification::DRAW:
            return "1/2-1/2";

        case PositionClassification::BLACK_WIN:
            return "0-1";

        case PositionClassification::BOOK_WHITE_WIN:
            return "in book: 1-0";

        case PositionClassification::BOOK_DRAW:
            return "in book: 1/2-1/2";

        case PositionClassification::BOOK_BLACK_WIN:
            return "in book: 0-1";

        default:
            return "???";
    }
}

void printStats(
    const std::vector<CompressedPosition> &positions,
    const std::vector<PositionStats> &stats,
    const std::vector<std::uint32_t> &positionPlyNums)
{
    std::uint32_t highestPlyNum { };
    std::uint32_t highestPlyNumFound { };
    std::size_t highestPlyNumPositionIndex { };
    bool foundInDb { false };

    // resolve highest ply num in the input positions
    for (std::uint32_t plyNum : positionPlyNums)
        highestPlyNum = std::max(highestPlyNum, plyNum);

    // find the highest ply num of found positions
    for (std::size_t i { }; i < stats.size(); ++i)
    {
        const auto &resultStats { stats.at(i).resultStats };
        const std::uint32_t thisPlyNum { positionPlyNums.at(i) };

        if constexpr (debugMode)
        {
            pgn_reader::ChessBoard board { };
            const auto &cp { positions.at(i) };

            std::cout << "Checking position " << i << ", ply " << thisPlyNum << std::endl;
            PositionCompressor::decompress(cp, 0U, 0U, board);
            board.printBoard();
        }

        for (const auto &posResultStats : resultStats)
        {
            if (posResultStats.numGames > 0U)
            {
                if (!foundInDb || thisPlyNum > highestPlyNumFound)
                {
                    highestPlyNumFound = thisPlyNum;
                    highestPlyNumPositionIndex = i;
                    foundInDb = true;

                    if constexpr (debugMode)
                    {
                        std::cout << "New highest position index " << highestPlyNumPositionIndex << std::endl;
                    }
                }

                break;
            }
        }
    }

    if (foundInDb)
    {
        if (highestPlyNumFound != highestPlyNum)
        {
            std::cout << "Not found, last known: ";
        }

        const auto &resultStats { stats.at(highestPlyNumPositionIndex).resultStats };

        const std::size_t whiteWin { resultStats.at(static_cast<std::size_t>(PositionClassification::WHITE_WIN)).numGames };
        const std::size_t draw { resultStats.at(static_cast<std::size_t>(PositionClassification::DRAW)).numGames };
        const std::size_t blackWin { resultStats.at(static_cast<std::size_t>(PositionClassification::BLACK_WIN)).numGames };

        const std::size_t bookWhiteWin { resultStats.at(static_cast<std::size_t>(PositionClassification::BOOK_WHITE_WIN)).numGames };
        const std::size_t bookDraw { resultStats.at(static_cast<std::size_t>(PositionClassification::BOOK_DRAW)).numGames };
        const std::size_t bookBlackWin { resultStats.at(static_cast<std::size_t>(PositionClassification::BOOK_BLACK_WIN)).numGames };

        if constexpr (debugMode)
        {
            const auto &cp { positions.at(highestPlyNumPositionIndex) };
            pgn_reader::ChessBoard board { };
            PositionCompressor::decompress(cp, 0U, 0U, board);

            board.printBoard();
        }

        std::cout
            << std::format("({}{}) +{}={}-{}",
                           pgn_reader::moveNumOfPly(highestPlyNum),
                           pgn_reader::colorOfPly(highestPlyNum) == pgn_reader::Color::WHITE ? "w" : "b",
                           whiteWin,
                           draw,
                           blackWin);

        if (bookWhiteWin + bookDraw + bookBlackWin > 0U)
        {
            std::cout
                << std::format(" (in book: +{}={}-{})",
                               bookWhiteWin, bookDraw, bookBlackWin);
        }

        // print recent games
        if (whiteWin + draw + blackWin > 0U)
        {
            for (PositionClassification pc :
                     { PositionClassification::WHITE_WIN,
                       PositionClassification::DRAW,
                       PositionClassification::BLACK_WIN })
            {
                const auto &stats { resultStats.at(static_cast<std::size_t>(pc)) };
                if (stats.numGames > 0U)
                {
                    std::cout
                        << std::format(" • {} - {} ({}) {}",
                                       stats.whitePlayer, stats.blackPlayer,
                                       positionClassificationToString(pc),
                                       stats.site);
                }
            }
        }
        else
        {
            for (PositionClassification pc :
                     { PositionClassification::BOOK_WHITE_WIN,
                       PositionClassification::BOOK_DRAW,
                       PositionClassification::BOOK_BLACK_WIN })
            {
                const auto &stats { resultStats.at(static_cast<std::size_t>(pc)) };
                if (stats.numGames > 0U)
                {
                    std::cout
                        << std::format(" • {} - {} ({}) {}",
                                       stats.whitePlayer, stats.blackPlayer,
                                       positionClassificationToString(pc),
                                       stats.site);
                }
            }
        }

        std::cout << std::endl;
    }
    else
    {
        std::cout << "Not found" << std::endl;
    }
}

int tdbQueryMain(int argc, char **argv) noexcept
{
    if (argc != 3 && argc != 4)
    {
        printHelp();
        return 127;
    }

    try
    {
        const std::string pgnDatabaseFile { argv[1] };
        const std::string pgnQueryFile { argv[2] };
        std::size_t threads { 1U };

        if (argc == 4)
        {
            std::string_view sv { argv[3] };
            std::from_chars(sv.begin(), sv.end(), threads);

            threads = std::clamp(threads, std::size_t { 1U }, std::size_t { 256U });
        }

        std::vector<CompressedPosition> positions;
        std::vector<std::uint32_t> positionPlyNums;

        std::tie(positions, positionPlyNums) = collectQueryFilePositions(pgnQueryFile);

        const std::vector<PositionStats> stats { collectStatistics(positions, pgnDatabaseFile, threads) };

        printStats(positions, stats, positionPlyNums);

        return 0;
    }
    catch (const pgn_reader::PgnError &pgnError)
    {
        std::cerr << pgnError.what() << std::endl;

        return 1;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;

        return 2;
    }
}

}

}

int main(int argc, char **argv)
{
    return hoover_chess_utils::utils::tdbQueryMain(argc, argv);
}
