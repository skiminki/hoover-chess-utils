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

#ifndef HOOVER_CHESS_UTILS__UTILS__MEMORY_MAPPED_FILE_H_INCLUDED
#define HOOVER_CHESS_UTILS__UTILS__MEMORY_MAPPED_FILE_H_INCLUDED

#include <cstddef>
#include <string_view>

namespace hoover_chess_utils::utils
{

class MemoryMappedFile
{
private:
    void *mapPtr { };
    std::size_t mapSize { };

public:
    MemoryMappedFile() = default;
    MemoryMappedFile(const MemoryMappedFile &) = delete;

    MemoryMappedFile(MemoryMappedFile &&other) :
        mapPtr { other.mapPtr },
        mapSize { other.mapSize }
    {
        other.mapPtr = nullptr;
        other.mapSize = 0U;
    }

    MemoryMappedFile &operator = (const MemoryMappedFile &) & = delete;
    MemoryMappedFile &operator = (MemoryMappedFile &&other) & = delete;

    ~MemoryMappedFile()
    {
        try
        {
            unmap();
        }
        catch (...)
        {
        }
    }

    inline std::string_view getStringView() const
    {
        return std::string_view { static_cast<const char *>(mapPtr), mapSize };
    }

    inline void *data()
    {
        return mapPtr;
    }

    inline std::size_t size() const
    {
        return mapSize;
    }

    void map(const char *filename, bool read, bool write);

    void unmap();
};

}

#endif
