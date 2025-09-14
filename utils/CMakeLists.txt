# Hoover Chess Utilities / PGN reader
# Copyright (C) 2025  Sami Kiminki
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.


project(HooverChessUtils_CliUtilities VERSION ${HOOVER_CHESS_UTILS_VERSION})

add_executable(hoover-tdb-query
  memory-mapped-file.cc
  tdb-query.cc)

target_include_directories(hoover-tdb-query PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/../pgn-reader/include"
  )

target_link_libraries(hoover-tdb-query
  hoover-pgn-reader
)

add_executable(hoover-compactify-tcec-pgn
  compactify-tcec-pgn.cc
  memory-mapped-file.cc
  output-buffer.cc)

target_include_directories(hoover-compactify-tcec-pgn PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/../pgn-reader/include"
  )

target_link_libraries(hoover-compactify-tcec-pgn
  hoover-pgn-reader
)

add_executable(hoover-process-full-tcec-pgn
  process-full-tcec-pgn.cc
  memory-mapped-file.cc
  output-buffer.cc)

target_include_directories(hoover-process-full-tcec-pgn PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/../pgn-reader/include"
  )

target_link_libraries(hoover-process-full-tcec-pgn
  hoover-pgn-reader
)
