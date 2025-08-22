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

#include "pgnreader.h"
#include "pgnreader-string-utils.h"
#include "chessboard.h"

#include "../src/pgnscanner.h"
#include "../src/pgnparser.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace hoover_chess_utils::pgn_reader::perf_test_suite
{

class MemoryMappedFile
{
private:
    void *mapPtr { };
    std::size_t mapSize { };

public:
    inline std::string_view getStringView() const
    {
        return std::string_view { static_cast<const char *>(mapPtr), mapSize };
    }

    void openAndMap(const char *filename)
    {
        // make sure we're not holding a map
        unmap();

        int flags { };
        int mapProt { };

        flags = O_RDONLY;
        mapProt = PROT_READ;

        const int fd { open(filename, flags | O_CLOEXEC) };
        if (fd == -1)
            throw std::system_error(errno, std::generic_category(), "Failed to open file");

        int err;

        try
        {
            off_t len;
            void *mmapRet;

            len = lseek(fd, 0, SEEK_END);
            if (len == (off_t)-1)
                throw std::system_error(errno, std::generic_category(), "Failed to read file length");

            mapSize = len;

            mmapRet = mmap(NULL, mapSize, mapProt, MAP_SHARED, fd, 0);
            if (mmapRet == MAP_FAILED)
                throw std::system_error(errno, std::generic_category(), "Failed to map the file");

            mapPtr = mmapRet;
        }
        catch (...)
        {
            close(fd);
            throw;
        }

        err = close(fd);
        if (err == -1)
            throw std::system_error(errno, std::generic_category(), "Failed to close file after map");
    }

    void unmap()
    {
        if (mapPtr)
        {
            const int err = munmap(mapPtr, mapSize);
            if (err != 0)
                throw std::system_error(errno, std::generic_category(), "Failed to unmap");

            mapPtr = nullptr;
            mapSize = 0;
        }
    }
};

class TestPgnReaderActions : public PgnReaderActions
{
public:
    std::uint64_t games { };
    std::uint64_t pgnTags { };
    std::uint64_t moves { };
    const hoover_chess_utils::pgn_reader::ChessBoard *board { };

    void setBoardReferences(
        const ChessBoard &curBoard,
        const ChessBoard &prevBoard) override
    {
        board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void gameStart() override
    {
        ++games;
    }

    void pgnTag(std::string_view, std::string_view) override
    {
        ++pgnTags;
    }

    void afterMove(ChessBoard::Move) override
    {
        ++moves;
    }
};

class TestPgnMoveWriterActions : public PgnReaderActions
{
public:
    const ChessBoard *m_prevBoard { };
    const ChessBoard *m_curBoard { };
    bool m_outputMoveNum { };
    std::size_t m_outputLength { };

    void gameStart() override
    {
        m_outputMoveNum = true;
    }

    void setBoardReferences(
        const ChessBoard &curBoard,
        const ChessBoard &prevBoard) override
    {
        m_prevBoard = &prevBoard;
        m_curBoard = &curBoard;
    }

    void collectOutput(std::string_view str)
    {
        if constexpr(false)
            std::cout << str;

        m_outputLength += str.size();
    }

    void afterMove(ChessBoard::Move m) override
    {
            if (m_outputMoveNum)
            {
                collectOutput(StringUtils::plyNumToString(m_prevBoard->getCurrentPlyNum()).getStringView());
                collectOutput(std::string_view { " " });
            }

            collectOutput(StringUtils::moveToSan(*m_prevBoard, m).getStringView());
            collectOutput(std::string_view { " " });

            m_outputMoveNum = (m_curBoard->getTurn() == Color::WHITE);
    }
};

std::uint64_t pgnScannerPerfTest(std::string_view pgn)
{
    std::uint64_t tokens { };
    PgnScanner pgnScanner { pgn.data(), pgn.size() };

    while (true)
    {
        const PgnScannerToken token { pgnScanner.nextToken() };
        ++tokens;
        if (token == PgnScannerToken::END_OF_FILE)
            break;
    }
    return tokens;
}

void pgnParserPerfTest(std::string_view pgn)
{
    PgnScanner pgnScanner { pgn.data(), pgn.size() };
    PgnParser_NullActions parserActions { };

    PgnParser parser { pgnScanner, parserActions };
    parser.parse();
}

}

int main(int argc, char **argv)
{
    using hoover_chess_utils::pgn_reader::PgnError;
    using hoover_chess_utils::pgn_reader::PgnReader;
    using hoover_chess_utils::pgn_reader::PgnReaderActionClass;
    using hoover_chess_utils::pgn_reader::PgnReaderActionFilter;

    using hoover_chess_utils::pgn_reader::perf_test_suite::MemoryMappedFile;
    using hoover_chess_utils::pgn_reader::perf_test_suite::TestPgnReaderActions;
    using hoover_chess_utils::pgn_reader::perf_test_suite::TestPgnMoveWriterActions;
    using hoover_chess_utils::pgn_reader::perf_test_suite::pgnScannerPerfTest;
    using hoover_chess_utils::pgn_reader::perf_test_suite::pgnParserPerfTest;

    if (argc != 2)
    {
        std::cerr << "Usage: pgnreaderperftest <pgn-file>" << std::endl;
        return 1;
    }

    MemoryMappedFile mmfile;

    mmfile.openAndMap(argv[1]);

    for (std::size_t i = 0; i < 3; ++i)
    {
        const std::size_t fileSize { mmfile.getStringView().size() };

        try
        {
            TestPgnReaderActions actions { };
            TestPgnMoveWriterActions moveWriterActions { };

            const auto startPgnTokenScan { std::chrono::steady_clock::now() };
            const std::uint64_t tokens { pgnScannerPerfTest(mmfile.getStringView()) };
            const auto endPgnTokenScan { std::chrono::steady_clock::now() };

            const auto &startPgnParser { endPgnTokenScan };
            pgnParserPerfTest(mmfile.getStringView());
            const auto endPgnParser { std::chrono::steady_clock::now() };

            const auto &startPgnReadMoves { endPgnParser };
            PgnReader::readFromMemory(
                mmfile.getStringView(), actions, PgnReaderActionFilter { PgnReaderActionClass::Move });
            const auto endPgnReadMoves = std::chrono::steady_clock::now();

            const auto &startPgnReadTags { endPgnReadMoves };
            PgnReader::readFromMemory(
                mmfile.getStringView(), actions, PgnReaderActionFilter { PgnReaderActionClass::PgnTag });
            const auto endPgnReadTags = std::chrono::steady_clock::now();

            const auto &startPgnWriteMoves { endPgnReadTags };
            PgnReader::readFromMemory(
                mmfile.getStringView(), moveWriterActions, PgnReaderActionFilter { PgnReaderActionClass::Move });
            const auto endPgnWriteMoves = std::chrono::steady_clock::now();

            const std::chrono::duration<double> pgnTokenScanDuration = endPgnTokenScan - startPgnTokenScan;
            const std::chrono::duration<double> pgnParserDuration = endPgnParser - startPgnParser;
            const std::chrono::duration<double> pgnReadMovesDuration = endPgnReadMoves - startPgnReadMoves;
            const std::chrono::duration<double> pgnReadTagsDuration = endPgnReadTags - startPgnReadTags;
            const std::chrono::duration<double> pgnMoveWriterDuration = endPgnWriteMoves - startPgnWriteMoves;

            std::cout << "Iteration " << (i + 1)
                      << (actions.games / 2U) << " games, "
                      << actions.pgnTags << " pgn tags, "
                      << actions.moves << " moves, "
                      << tokens << " tokens" << std::endl
                      << "- tokenizer pass:        "
                      << pgnTokenScanDuration.count() << " secs, "
                      << (fileSize / (1000000U * pgnTokenScanDuration.count())) << " MB/s" << std::endl
                      << "- tokenizer+parser pass: "
                      << pgnParserDuration.count() << " secs, "
                      << (fileSize / (1000000U * pgnParserDuration.count())) << " MB/s" << std::endl
                      << "- tag reader pass:       "
                      << pgnReadTagsDuration.count() << " secs, "
                      << (fileSize / (1000000U * pgnReadTagsDuration.count())) << " MB/s" << std::endl
                      << "- move reader pass:      "
                      << pgnReadMovesDuration.count() << " secs, "
                      << (fileSize / (1000000U * pgnReadMovesDuration.count())) << " MB/s" << std::endl
                      << "- SAN move writer pass:  "
                      << pgnMoveWriterDuration.count() << " secs, "
                      << (fileSize / (1000000U * pgnMoveWriterDuration.count())) << " MB/s" << std::endl
                      << std::endl;
        }
        catch (const PgnError &pgnError)
        {
            std::cout << pgnError.what() << std::endl;
        }
    }

    mmfile.unmap();
    return 0;
}
