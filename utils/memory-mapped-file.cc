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

#include "memory-mapped-file.h"

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <format>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace hoover_chess_utils::utils
{

void MemoryMappedFile::map(const char *filename, bool read, bool write)
{
    // make sure we're not holding a map
    unmap();

    int flags { };
    int mapProt { };

    if (read && write)
    {
        flags = O_RDWR;
        mapProt = PROT_READ | PROT_WRITE;
    }
    else if (read)
    {
        flags = O_RDONLY;
        mapProt = PROT_READ;
    }
    else if (write)
    {
        flags = O_WRONLY;
        mapProt = PROT_WRITE;
    }
    else
        throw std::invalid_argument { "Bad read/write arguments" };

    const int fd { open(filename, flags | O_CLOEXEC) };
    if (fd == -1)
        throw std::system_error(
            errno,
            std::generic_category(),
            std::format("Failed to open file '{}'", filename));

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

void MemoryMappedFile::unmap()
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

}
