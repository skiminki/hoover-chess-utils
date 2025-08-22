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

#include "output-buffer.h"

#include <cstddef>
#include <cstdio>


namespace hoover_chess_utils::utils
{

void OutputBuffer::writeInternal(const char *str, std::size_t numChars)
{
    while (numChars > 0U)
    {
        const std::size_t writeSize { std::min(ctBufferSize - m_numChars, numChars) };

        std::memcpy(&m_buf[m_numChars], str, writeSize);

        m_numChars += writeSize;

        numChars -= writeSize;
        str += writeSize;

        if (m_numChars == ctBufferSize)
            flush();
    }
}

void OutputBuffer::flush()
{
    std::fwrite(m_buf.data(), m_numChars, 1U, stdout);
    m_numChars = 0U;
}

}
