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

#ifndef CHESS_UTILS__UTILS__OUTPUT_BUFFER_H_INCLUDED
#define CHESS_UTILS__UTILS__OUTPUT_BUFFER_H_INCLUDED

#include "pgnreader-string-utils.h"

#include <cstddef>
#include <cstring>

namespace hoover_chess_utils::utils
{

class OutputBuffer
{
private:
    static constexpr std::size_t ctBufferSize { 65536U };

    std::size_t m_numChars { };
    std::array<char, ctBufferSize> m_buf;

    void writeInternal(const char *str, std::size_t numChars);

public:

    ~OutputBuffer() noexcept
    {
        try
        {
            flush();
        }
        catch (...)
        {
        }
    }

    inline void write(std::string_view sv)
    {
        writeInternal(sv.data(), sv.size());
    }

    template <std::size_t t_maxSize>
    inline void write(const pgn_reader::MiniString<t_maxSize> &s)
    {
        writeInternal(s.data(), s.size());
    }

    inline void write(char c)
    {
        writeInternal(&c, 1U);
    }

    void flush();
};

}

#endif
