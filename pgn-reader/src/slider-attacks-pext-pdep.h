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

#ifndef CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_PEXT_PDEP_H_INCLUDED
#define CHESS_UTILS__PGN_READER__SLIDER_ATTACKS_PEXT_PDEP_H_INCLUDED

#include "slider-attacks.h"
#include "chessboard-types-squareset.h"
#include "pgnreader-config.h"

#include <array>
#include <cinttypes>

static_assert(HAVE_PDEP_PEXT, "This file should be included only when PDEP/PEXT is available");

namespace hoover_chess_utils::pgn_reader
{

class CompileTimeInitializers
{
public:
};


}

#endif
