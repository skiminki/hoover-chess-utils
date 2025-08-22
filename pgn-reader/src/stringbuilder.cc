// Hoover Chess Utilities / PGN reader
// Copyright (C) 2023-2025  Sami Kiminki
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

#include "stringbuilder.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

namespace hoover_chess_utils::pgn_reader
{

void StringBuilder::growAndAppend(const char *str, std::size_t len)
{
    // first, assert that we actually need to grow
    assert(m_strEnd + len > m_bufEnd);

    // then, fill up the current buffer
    std::memcpy(m_strEnd, str, m_bufEnd - m_strEnd);
    len -= m_bufEnd - m_strEnd;
    str += m_bufEnd - m_strEnd;

    // then, figure out the new buffer size
    const std::size_t oldBufSize = m_bufEnd - m_buf;

    // figure out new buf size based on growth factor
    std::size_t newBufSize { oldBufSize };
    do
    {
        newBufSize *= ctDynamicGrowthFactor;
    }
    while (oldBufSize + len > newBufSize);

    // reallocate
    m_buf = static_cast<char *>(std::realloc(m_buf, newBufSize));
    m_bufEnd = m_buf + newBufSize;

    // copy the remaining segment
    std::memcpy(m_buf + oldBufSize, str, len);
    m_strEnd = m_buf + oldBufSize + len;
}

StringBuilder::StringBuilder()
{
    m_buf = static_cast<char *>(std::malloc(ctDynamicAllocBase));
    m_bufEnd = m_buf + ctDynamicAllocBase;
    m_strEnd = m_buf;
}

StringBuilder::~StringBuilder()
{
    std::free(m_buf);
}

void StringBuilder::pushBack(char c)
{
    if (hasRoom(1U)) [[likely]]
    {
        *m_strEnd++ = c;
    }
    else [[unlikely]]
    {
        growAndAppend(&c, 1U);
    }
}

void StringBuilder::appendString(const char *str, std::size_t len)
{
    if (hasRoom(len)) [[likely]]
    {
        std::memcpy(m_strEnd, str, len);
        m_strEnd += len;
    }
    else [[unlikely]]
    {
        growAndAppend(str, len);
    }
}



}
