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

#ifndef HOOVER_CHESS_UTILS__PGN_READER__STRINGBUILDER_H_INCLUDED
#define HOOVER_CHESS_UTILS__PGN_READER__STRINGBUILDER_H_INCLUDED

#include <cstddef>
#include <string_view>


namespace hoover_chess_utils::pgn_reader
{

class StringBuilder
{
private:
    char *m_buf { };
    char *m_strEnd { };
    char *m_bufEnd { };

    void growAndAppend(const char *str, std::size_t len);

    bool hasRoom(std::size_t newChars) const
    {
        return m_strEnd + newChars <= m_bufEnd;
    }

public:
    static constexpr std::size_t ctDynamicAllocBase { 4096 };
    static constexpr std::size_t ctDynamicGrowthFactor { 2 };

    StringBuilder();

    ~StringBuilder();

    bool isEmpty() const noexcept
    {
        return m_buf == m_strEnd;
    }

    std::string_view getStringView() const noexcept
    {
        return std::string_view { m_buf, m_strEnd };
    }

    void clear() noexcept
    {
        m_strEnd = m_buf;
    }

    void pushBack(char c);

    void appendString(const char *str, std::size_t len);

};

}
#endif
