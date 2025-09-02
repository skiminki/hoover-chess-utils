#!/bin/bash
#
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

set -e
cd "$(dirname "$0")"

mkdir -p build/
cp -r html build/github-pages
cp -r build/docs-hoover-chess-utils/html build/github-pages/hoover-chess-utils
cp -r build/docs-pgn-reader-docs/pgn-reader/html build/github-pages/pgn-reader
