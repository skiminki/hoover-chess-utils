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

#include "pgnreader.h"
#include "pgnreader-string-utils.h"
#include "position-compress-fixed.h"
#include "version.h"

#include "memory-mapped-file.h"
#include "output-buffer.h"

#include <charconv>
#include <format>
#include <iostream>
#include <map>
#include <regex>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hoover_chess_utils::utils
{

namespace
{

constexpr bool debugMode { false };

constexpr std::array<std::string_view, 960U> frcTable {
    "BBQNNRKR", "BQNBNRKR", "BQNNRBKR", "BQNNRKRB", "QBBNNRKR", "QNBBNRKR", "QNBNRBKR", "QNBNRKRB", "QBNNBRKR", "QNNBBRKR",
    "QNNRBBKR", "QNNRBKRB", "QBNNRKBR", "QNNBRKBR", "QNNRKBBR", "QNNRKRBB", "BBNQNRKR", "BNQBNRKR", "BNQNRBKR", "BNQNRKRB",
    "NBBQNRKR", "NQBBNRKR", "NQBNRBKR", "NQBNRKRB", "NBQNBRKR", "NQNBBRKR", "NQNRBBKR", "NQNRBKRB", "NBQNRKBR", "NQNBRKBR",
    "NQNRKBBR", "NQNRKRBB", "BBNNQRKR", "BNNBQRKR", "BNNQRBKR", "BNNQRKRB", "NBBNQRKR", "NNBBQRKR", "NNBQRBKR", "NNBQRKRB",
    "NBNQBRKR", "NNQBBRKR", "NNQRBBKR", "NNQRBKRB", "NBNQRKBR", "NNQBRKBR", "NNQRKBBR", "NNQRKRBB", "BBNNRQKR", "BNNBRQKR",
    "BNNRQBKR", "BNNRQKRB", "NBBNRQKR", "NNBBRQKR", "NNBRQBKR", "NNBRQKRB", "NBNRBQKR", "NNRBBQKR", "NNRQBBKR", "NNRQBKRB",
    "NBNRQKBR", "NNRBQKBR", "NNRQKBBR", "NNRQKRBB", "BBNNRKQR", "BNNBRKQR", "BNNRKBQR", "BNNRKQRB", "NBBNRKQR", "NNBBRKQR",
    "NNBRKBQR", "NNBRKQRB", "NBNRBKQR", "NNRBBKQR", "NNRKBBQR", "NNRKBQRB", "NBNRKQBR", "NNRBKQBR", "NNRKQBBR", "NNRKQRBB",
    "BBNNRKRQ", "BNNBRKRQ", "BNNRKBRQ", "BNNRKRQB", "NBBNRKRQ", "NNBBRKRQ", "NNBRKBRQ", "NNBRKRQB", "NBNRBKRQ", "NNRBBKRQ",
    "NNRKBBRQ", "NNRKBRQB", "NBNRKRBQ", "NNRBKRBQ", "NNRKRBBQ", "NNRKRQBB", "BBQNRNKR", "BQNBRNKR", "BQNRNBKR", "BQNRNKRB",
    "QBBNRNKR", "QNBBRNKR", "QNBRNBKR", "QNBRNKRB", "QBNRBNKR", "QNRBBNKR", "QNRNBBKR", "QNRNBKRB", "QBNRNKBR", "QNRBNKBR",
    "QNRNKBBR", "QNRNKRBB", "BBNQRNKR", "BNQBRNKR", "BNQRNBKR", "BNQRNKRB", "NBBQRNKR", "NQBBRNKR", "NQBRNBKR", "NQBRNKRB",
    "NBQRBNKR", "NQRBBNKR", "NQRNBBKR", "NQRNBKRB", "NBQRNKBR", "NQRBNKBR", "NQRNKBBR", "NQRNKRBB", "BBNRQNKR", "BNRBQNKR",
    "BNRQNBKR", "BNRQNKRB", "NBBRQNKR", "NRBBQNKR", "NRBQNBKR", "NRBQNKRB", "NBRQBNKR", "NRQBBNKR", "NRQNBBKR", "NRQNBKRB",
    "NBRQNKBR", "NRQBNKBR", "NRQNKBBR", "NRQNKRBB", "BBNRNQKR", "BNRBNQKR", "BNRNQBKR", "BNRNQKRB", "NBBRNQKR", "NRBBNQKR",
    "NRBNQBKR", "NRBNQKRB", "NBRNBQKR", "NRNBBQKR", "NRNQBBKR", "NRNQBKRB", "NBRNQKBR", "NRNBQKBR", "NRNQKBBR", "NRNQKRBB",
    "BBNRNKQR", "BNRBNKQR", "BNRNKBQR", "BNRNKQRB", "NBBRNKQR", "NRBBNKQR", "NRBNKBQR", "NRBNKQRB", "NBRNBKQR", "NRNBBKQR",
    "NRNKBBQR", "NRNKBQRB", "NBRNKQBR", "NRNBKQBR", "NRNKQBBR", "NRNKQRBB", "BBNRNKRQ", "BNRBNKRQ", "BNRNKBRQ", "BNRNKRQB",
    "NBBRNKRQ", "NRBBNKRQ", "NRBNKBRQ", "NRBNKRQB", "NBRNBKRQ", "NRNBBKRQ", "NRNKBBRQ", "NRNKBRQB", "NBRNKRBQ", "NRNBKRBQ",
    "NRNKRBBQ", "NRNKRQBB", "BBQNRKNR", "BQNBRKNR", "BQNRKBNR", "BQNRKNRB", "QBBNRKNR", "QNBBRKNR", "QNBRKBNR", "QNBRKNRB",
    "QBNRBKNR", "QNRBBKNR", "QNRKBBNR", "QNRKBNRB", "QBNRKNBR", "QNRBKNBR", "QNRKNBBR", "QNRKNRBB", "BBNQRKNR", "BNQBRKNR",
    "BNQRKBNR", "BNQRKNRB", "NBBQRKNR", "NQBBRKNR", "NQBRKBNR", "NQBRKNRB", "NBQRBKNR", "NQRBBKNR", "NQRKBBNR", "NQRKBNRB",
    "NBQRKNBR", "NQRBKNBR", "NQRKNBBR", "NQRKNRBB", "BBNRQKNR", "BNRBQKNR", "BNRQKBNR", "BNRQKNRB", "NBBRQKNR", "NRBBQKNR",
    "NRBQKBNR", "NRBQKNRB", "NBRQBKNR", "NRQBBKNR", "NRQKBBNR", "NRQKBNRB", "NBRQKNBR", "NRQBKNBR", "NRQKNBBR", "NRQKNRBB",
    "BBNRKQNR", "BNRBKQNR", "BNRKQBNR", "BNRKQNRB", "NBBRKQNR", "NRBBKQNR", "NRBKQBNR", "NRBKQNRB", "NBRKBQNR", "NRKBBQNR",
    "NRKQBBNR", "NRKQBNRB", "NBRKQNBR", "NRKBQNBR", "NRKQNBBR", "NRKQNRBB", "BBNRKNQR", "BNRBKNQR", "BNRKNBQR", "BNRKNQRB",
    "NBBRKNQR", "NRBBKNQR", "NRBKNBQR", "NRBKNQRB", "NBRKBNQR", "NRKBBNQR", "NRKNBBQR", "NRKNBQRB", "NBRKNQBR", "NRKBNQBR",
    "NRKNQBBR", "NRKNQRBB", "BBNRKNRQ", "BNRBKNRQ", "BNRKNBRQ", "BNRKNRQB", "NBBRKNRQ", "NRBBKNRQ", "NRBKNBRQ", "NRBKNRQB",
    "NBRKBNRQ", "NRKBBNRQ", "NRKNBBRQ", "NRKNBRQB", "NBRKNRBQ", "NRKBNRBQ", "NRKNRBBQ", "NRKNRQBB", "BBQNRKRN", "BQNBRKRN",
    "BQNRKBRN", "BQNRKRNB", "QBBNRKRN", "QNBBRKRN", "QNBRKBRN", "QNBRKRNB", "QBNRBKRN", "QNRBBKRN", "QNRKBBRN", "QNRKBRNB",
    "QBNRKRBN", "QNRBKRBN", "QNRKRBBN", "QNRKRNBB", "BBNQRKRN", "BNQBRKRN", "BNQRKBRN", "BNQRKRNB", "NBBQRKRN", "NQBBRKRN",
    "NQBRKBRN", "NQBRKRNB", "NBQRBKRN", "NQRBBKRN", "NQRKBBRN", "NQRKBRNB", "NBQRKRBN", "NQRBKRBN", "NQRKRBBN", "NQRKRNBB",
    "BBNRQKRN", "BNRBQKRN", "BNRQKBRN", "BNRQKRNB", "NBBRQKRN", "NRBBQKRN", "NRBQKBRN", "NRBQKRNB", "NBRQBKRN", "NRQBBKRN",
    "NRQKBBRN", "NRQKBRNB", "NBRQKRBN", "NRQBKRBN", "NRQKRBBN", "NRQKRNBB", "BBNRKQRN", "BNRBKQRN", "BNRKQBRN", "BNRKQRNB",
    "NBBRKQRN", "NRBBKQRN", "NRBKQBRN", "NRBKQRNB", "NBRKBQRN", "NRKBBQRN", "NRKQBBRN", "NRKQBRNB", "NBRKQRBN", "NRKBQRBN",
    "NRKQRBBN", "NRKQRNBB", "BBNRKRQN", "BNRBKRQN", "BNRKRBQN", "BNRKRQNB", "NBBRKRQN", "NRBBKRQN", "NRBKRBQN", "NRBKRQNB",
    "NBRKBRQN", "NRKBBRQN", "NRKRBBQN", "NRKRBQNB", "NBRKRQBN", "NRKBRQBN", "NRKRQBBN", "NRKRQNBB", "BBNRKRNQ", "BNRBKRNQ",
    "BNRKRBNQ", "BNRKRNQB", "NBBRKRNQ", "NRBBKRNQ", "NRBKRBNQ", "NRBKRNQB", "NBRKBRNQ", "NRKBBRNQ", "NRKRBBNQ", "NRKRBNQB",
    "NBRKRNBQ", "NRKBRNBQ", "NRKRNBBQ", "NRKRNQBB", "BBQRNNKR", "BQRBNNKR", "BQRNNBKR", "BQRNNKRB", "QBBRNNKR", "QRBBNNKR",
    "QRBNNBKR", "QRBNNKRB", "QBRNBNKR", "QRNBBNKR", "QRNNBBKR", "QRNNBKRB", "QBRNNKBR", "QRNBNKBR", "QRNNKBBR", "QRNNKRBB",
    "BBRQNNKR", "BRQBNNKR", "BRQNNBKR", "BRQNNKRB", "RBBQNNKR", "RQBBNNKR", "RQBNNBKR", "RQBNNKRB", "RBQNBNKR", "RQNBBNKR",
    "RQNNBBKR", "RQNNBKRB", "RBQNNKBR", "RQNBNKBR", "RQNNKBBR", "RQNNKRBB", "BBRNQNKR", "BRNBQNKR", "BRNQNBKR", "BRNQNKRB",
    "RBBNQNKR", "RNBBQNKR", "RNBQNBKR", "RNBQNKRB", "RBNQBNKR", "RNQBBNKR", "RNQNBBKR", "RNQNBKRB", "RBNQNKBR", "RNQBNKBR",
    "RNQNKBBR", "RNQNKRBB", "BBRNNQKR", "BRNBNQKR", "BRNNQBKR", "BRNNQKRB", "RBBNNQKR", "RNBBNQKR", "RNBNQBKR", "RNBNQKRB",
    "RBNNBQKR", "RNNBBQKR", "RNNQBBKR", "RNNQBKRB", "RBNNQKBR", "RNNBQKBR", "RNNQKBBR", "RNNQKRBB", "BBRNNKQR", "BRNBNKQR",
    "BRNNKBQR", "BRNNKQRB", "RBBNNKQR", "RNBBNKQR", "RNBNKBQR", "RNBNKQRB", "RBNNBKQR", "RNNBBKQR", "RNNKBBQR", "RNNKBQRB",
    "RBNNKQBR", "RNNBKQBR", "RNNKQBBR", "RNNKQRBB", "BBRNNKRQ", "BRNBNKRQ", "BRNNKBRQ", "BRNNKRQB", "RBBNNKRQ", "RNBBNKRQ",
    "RNBNKBRQ", "RNBNKRQB", "RBNNBKRQ", "RNNBBKRQ", "RNNKBBRQ", "RNNKBRQB", "RBNNKRBQ", "RNNBKRBQ", "RNNKRBBQ", "RNNKRQBB",
    "BBQRNKNR", "BQRBNKNR", "BQRNKBNR", "BQRNKNRB", "QBBRNKNR", "QRBBNKNR", "QRBNKBNR", "QRBNKNRB", "QBRNBKNR", "QRNBBKNR",
    "QRNKBBNR", "QRNKBNRB", "QBRNKNBR", "QRNBKNBR", "QRNKNBBR", "QRNKNRBB", "BBRQNKNR", "BRQBNKNR", "BRQNKBNR", "BRQNKNRB",
    "RBBQNKNR", "RQBBNKNR", "RQBNKBNR", "RQBNKNRB", "RBQNBKNR", "RQNBBKNR", "RQNKBBNR", "RQNKBNRB", "RBQNKNBR", "RQNBKNBR",
    "RQNKNBBR", "RQNKNRBB", "BBRNQKNR", "BRNBQKNR", "BRNQKBNR", "BRNQKNRB", "RBBNQKNR", "RNBBQKNR", "RNBQKBNR", "RNBQKNRB",
    "RBNQBKNR", "RNQBBKNR", "RNQKBBNR", "RNQKBNRB", "RBNQKNBR", "RNQBKNBR", "RNQKNBBR", "RNQKNRBB", "BBRNKQNR", "BRNBKQNR",
    "BRNKQBNR", "BRNKQNRB", "RBBNKQNR", "RNBBKQNR", "RNBKQBNR", "RNBKQNRB", "RBNKBQNR", "RNKBBQNR", "RNKQBBNR", "RNKQBNRB",
    "RBNKQNBR", "RNKBQNBR", "RNKQNBBR", "RNKQNRBB", "BBRNKNQR", "BRNBKNQR", "BRNKNBQR", "BRNKNQRB", "RBBNKNQR", "RNBBKNQR",
    "RNBKNBQR", "RNBKNQRB", "RBNKBNQR", "RNKBBNQR", "RNKNBBQR", "RNKNBQRB", "RBNKNQBR", "RNKBNQBR", "RNKNQBBR", "RNKNQRBB",
    "BBRNKNRQ", "BRNBKNRQ", "BRNKNBRQ", "BRNKNRQB", "RBBNKNRQ", "RNBBKNRQ", "RNBKNBRQ", "RNBKNRQB", "RBNKBNRQ", "RNKBBNRQ",
    "RNKNBBRQ", "RNKNBRQB", "RBNKNRBQ", "RNKBNRBQ", "RNKNRBBQ", "RNKNRQBB", "BBQRNKRN", "BQRBNKRN", "BQRNKBRN", "BQRNKRNB",
    "QBBRNKRN", "QRBBNKRN", "QRBNKBRN", "QRBNKRNB", "QBRNBKRN", "QRNBBKRN", "QRNKBBRN", "QRNKBRNB", "QBRNKRBN", "QRNBKRBN",
    "QRNKRBBN", "QRNKRNBB", "BBRQNKRN", "BRQBNKRN", "BRQNKBRN", "BRQNKRNB", "RBBQNKRN", "RQBBNKRN", "RQBNKBRN", "RQBNKRNB",
    "RBQNBKRN", "RQNBBKRN", "RQNKBBRN", "RQNKBRNB", "RBQNKRBN", "RQNBKRBN", "RQNKRBBN", "RQNKRNBB", "BBRNQKRN", "BRNBQKRN",
    "BRNQKBRN", "BRNQKRNB", "RBBNQKRN", "RNBBQKRN", "RNBQKBRN", "RNBQKRNB", "RBNQBKRN", "RNQBBKRN", "RNQKBBRN", "RNQKBRNB",
    "RBNQKRBN", "RNQBKRBN", "RNQKRBBN", "RNQKRNBB", "BBRNKQRN", "BRNBKQRN", "BRNKQBRN", "BRNKQRNB", "RBBNKQRN", "RNBBKQRN",
    "RNBKQBRN", "RNBKQRNB", "RBNKBQRN", "RNKBBQRN", "RNKQBBRN", "RNKQBRNB", "RBNKQRBN", "RNKBQRBN", "RNKQRBBN", "RNKQRNBB",
    "BBRNKRQN", "BRNBKRQN", "BRNKRBQN", "BRNKRQNB", "RBBNKRQN", "RNBBKRQN", "RNBKRBQN", "RNBKRQNB", "RBNKBRQN", "RNKBBRQN",
    "RNKRBBQN", "RNKRBQNB", "RBNKRQBN", "RNKBRQBN", "RNKRQBBN", "RNKRQNBB", "BBRNKRNQ", "BRNBKRNQ", "BRNKRBNQ", "BRNKRNQB",
    "RBBNKRNQ", "RNBBKRNQ", "RNBKRBNQ", "RNBKRNQB", "RBNKBRNQ", "RNKBBRNQ", "RNKRBBNQ", "RNKRBNQB", "RBNKRNBQ", "RNKBRNBQ",
    "RNKRNBBQ", "RNKRNQBB", "BBQRKNNR", "BQRBKNNR", "BQRKNBNR", "BQRKNNRB", "QBBRKNNR", "QRBBKNNR", "QRBKNBNR", "QRBKNNRB",
    "QBRKBNNR", "QRKBBNNR", "QRKNBBNR", "QRKNBNRB", "QBRKNNBR", "QRKBNNBR", "QRKNNBBR", "QRKNNRBB", "BBRQKNNR", "BRQBKNNR",
    "BRQKNBNR", "BRQKNNRB", "RBBQKNNR", "RQBBKNNR", "RQBKNBNR", "RQBKNNRB", "RBQKBNNR", "RQKBBNNR", "RQKNBBNR", "RQKNBNRB",
    "RBQKNNBR", "RQKBNNBR", "RQKNNBBR", "RQKNNRBB", "BBRKQNNR", "BRKBQNNR", "BRKQNBNR", "BRKQNNRB", "RBBKQNNR", "RKBBQNNR",
    "RKBQNBNR", "RKBQNNRB", "RBKQBNNR", "RKQBBNNR", "RKQNBBNR", "RKQNBNRB", "RBKQNNBR", "RKQBNNBR", "RKQNNBBR", "RKQNNRBB",
    "BBRKNQNR", "BRKBNQNR", "BRKNQBNR", "BRKNQNRB", "RBBKNQNR", "RKBBNQNR", "RKBNQBNR", "RKBNQNRB", "RBKNBQNR", "RKNBBQNR",
    "RKNQBBNR", "RKNQBNRB", "RBKNQNBR", "RKNBQNBR", "RKNQNBBR", "RKNQNRBB", "BBRKNNQR", "BRKBNNQR", "BRKNNBQR", "BRKNNQRB",
    "RBBKNNQR", "RKBBNNQR", "RKBNNBQR", "RKBNNQRB", "RBKNBNQR", "RKNBBNQR", "RKNNBBQR", "RKNNBQRB", "RBKNNQBR", "RKNBNQBR",
    "RKNNQBBR", "RKNNQRBB", "BBRKNNRQ", "BRKBNNRQ", "BRKNNBRQ", "BRKNNRQB", "RBBKNNRQ", "RKBBNNRQ", "RKBNNBRQ", "RKBNNRQB",
    "RBKNBNRQ", "RKNBBNRQ", "RKNNBBRQ", "RKNNBRQB", "RBKNNRBQ", "RKNBNRBQ", "RKNNRBBQ", "RKNNRQBB", "BBQRKNRN", "BQRBKNRN",
    "BQRKNBRN", "BQRKNRNB", "QBBRKNRN", "QRBBKNRN", "QRBKNBRN", "QRBKNRNB", "QBRKBNRN", "QRKBBNRN", "QRKNBBRN", "QRKNBRNB",
    "QBRKNRBN", "QRKBNRBN", "QRKNRBBN", "QRKNRNBB", "BBRQKNRN", "BRQBKNRN", "BRQKNBRN", "BRQKNRNB", "RBBQKNRN", "RQBBKNRN",
    "RQBKNBRN", "RQBKNRNB", "RBQKBNRN", "RQKBBNRN", "RQKNBBRN", "RQKNBRNB", "RBQKNRBN", "RQKBNRBN", "RQKNRBBN", "RQKNRNBB",
    "BBRKQNRN", "BRKBQNRN", "BRKQNBRN", "BRKQNRNB", "RBBKQNRN", "RKBBQNRN", "RKBQNBRN", "RKBQNRNB", "RBKQBNRN", "RKQBBNRN",
    "RKQNBBRN", "RKQNBRNB", "RBKQNRBN", "RKQBNRBN", "RKQNRBBN", "RKQNRNBB", "BBRKNQRN", "BRKBNQRN", "BRKNQBRN", "BRKNQRNB",
    "RBBKNQRN", "RKBBNQRN", "RKBNQBRN", "RKBNQRNB", "RBKNBQRN", "RKNBBQRN", "RKNQBBRN", "RKNQBRNB", "RBKNQRBN", "RKNBQRBN",
    "RKNQRBBN", "RKNQRNBB", "BBRKNRQN", "BRKBNRQN", "BRKNRBQN", "BRKNRQNB", "RBBKNRQN", "RKBBNRQN", "RKBNRBQN", "RKBNRQNB",
    "RBKNBRQN", "RKNBBRQN", "RKNRBBQN", "RKNRBQNB", "RBKNRQBN", "RKNBRQBN", "RKNRQBBN", "RKNRQNBB", "BBRKNRNQ", "BRKBNRNQ",
    "BRKNRBNQ", "BRKNRNQB", "RBBKNRNQ", "RKBBNRNQ", "RKBNRBNQ", "RKBNRNQB", "RBKNBRNQ", "RKNBBRNQ", "RKNRBBNQ", "RKNRBNQB",
    "RBKNRNBQ", "RKNBRNBQ", "RKNRNBBQ", "RKNRNQBB", "BBQRKRNN", "BQRBKRNN", "BQRKRBNN", "BQRKRNNB", "QBBRKRNN", "QRBBKRNN",
    "QRBKRBNN", "QRBKRNNB", "QBRKBRNN", "QRKBBRNN", "QRKRBBNN", "QRKRBNNB", "QBRKRNBN", "QRKBRNBN", "QRKRNBBN", "QRKRNNBB",
    "BBRQKRNN", "BRQBKRNN", "BRQKRBNN", "BRQKRNNB", "RBBQKRNN", "RQBBKRNN", "RQBKRBNN", "RQBKRNNB", "RBQKBRNN", "RQKBBRNN",
    "RQKRBBNN", "RQKRBNNB", "RBQKRNBN", "RQKBRNBN", "RQKRNBBN", "RQKRNNBB", "BBRKQRNN", "BRKBQRNN", "BRKQRBNN", "BRKQRNNB",
    "RBBKQRNN", "RKBBQRNN", "RKBQRBNN", "RKBQRNNB", "RBKQBRNN", "RKQBBRNN", "RKQRBBNN", "RKQRBNNB", "RBKQRNBN", "RKQBRNBN",
    "RKQRNBBN", "RKQRNNBB", "BBRKRQNN", "BRKBRQNN", "BRKRQBNN", "BRKRQNNB", "RBBKRQNN", "RKBBRQNN", "RKBRQBNN", "RKBRQNNB",
    "RBKRBQNN", "RKRBBQNN", "RKRQBBNN", "RKRQBNNB", "RBKRQNBN", "RKRBQNBN", "RKRQNBBN", "RKRQNNBB", "BBRKRNQN", "BRKBRNQN",
    "BRKRNBQN", "BRKRNQNB", "RBBKRNQN", "RKBBRNQN", "RKBRNBQN", "RKBRNQNB", "RBKRBNQN", "RKRBBNQN", "RKRNBBQN", "RKRNBQNB",
    "RBKRNQBN", "RKRBNQBN", "RKRNQBBN", "RKRNQNBB", "BBRKRNNQ", "BRKBRNNQ", "BRKRNBNQ", "BRKRNNQB", "RBBKRNNQ", "RKBBRNNQ",
    "RKBRNBNQ", "RKBRNNQB", "RBKRBNNQ", "RKRBBNNQ", "RKRNBBNQ", "RKRNBNQB", "RBKRNNBQ", "RKRBNNBQ", "RKRNNBBQ", "RKRNNQBB"
};

static constexpr std::array<std::string_view, 23U> ctKnownTagsInOrder {
    "Event", "Site", "Date", "Round", "White", "Black", "Result", // seven tag roster
    "WhiteElo", "BlackElo", "WhiteType", "BlackType", "ECO", "Opening", "Variation",
    "TimeControl", "Annotator", "EventDate", "Time", "Variant", "SetUp", "FEN", "Termination", "PlyCount",
};

enum class KnownPgnTags : std::size_t
{
    // seven tag roster
    Event = 0U,
    Site,
    Date,
    Round,
    White,
    Black,
    Result,

    // rest in pgn-extract order
    WhiteElo,
    BlackElo,
    WhiteType,
    BlackType,
    ECO,
    Opening,
    Variation,
    TimeControl,
    Annotator,
    EventDate,
    Time,
    Variant,
    SetUp,
    FEN,
    Termination,
    PlyCount,
};

// make sure the count of enum values match with the array
static_assert(static_cast<std::size_t>(KnownPgnTags::PlyCount) + 1U ==
              std::tuple_size<decltype(ctKnownTagsInOrder)>::value);

auto buildFrcConfigurationMap(const std::array<std::string_view, 960U> &frcTable)
{
    std::uint16_t frcConfiguration { };
    std::map<std::string_view, std::uint16_t> ret { };

    for (const auto &frc : frcTable)
    {
        ret.emplace(std::make_pair(frc, frcConfiguration));
        ++frcConfiguration;
    }

    return ret;
}

const std::map<std::string_view, std::uint16_t> frcConfigurationMap { buildFrcConfigurationMap(frcTable) };

void printHelp()
{
    std::cout << "TCEC games PGN processing tool: master archive PGN file(s) to full PGN file (" << hoover_chess_utils::pgn_reader::getVersionString() << ')' << std::endl;
    std::cout << std::endl;
    std::puts("Usage: hoover-process-full-tcec-pgn <season_number> <event_number> <eco.pgn> (<PGN-file> <url_prefix>)+");
}

std::string classifyDfrc(std::string_view fen)
{
    std::string ret { };

    const std::basic_regex<char> fenMatcher {
        std::regex("([nbrqk]{8})/pppppppp/8/8/8/8/PPPPPPPP/([NBRQK]{8}) w (K?Q?k?q?|-) - 0 1",
                   std::regex::extended) };

    std::cmatch match;

    if (std::regex_match(fen.begin(), fen.end(), match, fenMatcher) && match.size() == 4U)
    {
        std::string_view blackPiecesView { match[1].first, match[1].second };
        std::string blackPieces { std::string{ blackPiecesView } };
        std::transform(blackPieces.begin(), blackPieces.end(), blackPieces.begin(), ::toupper);

        std::string_view whitePieces { match[2].first, match[2].second };
        std::string_view castling    { match[3].first, match[3].second };

        if (whitePieces == blackPieces)
        {
            // FRC
            const auto i { std::as_const(frcConfigurationMap).find(whitePieces) };

            ret = std::format("FRC {}", i->second);
        }
        else
        {
            // DFRC
            const auto i { std::as_const(frcConfigurationMap).find(whitePieces) };
            const auto j { std::as_const(frcConfigurationMap).find(blackPieces) };

            ret = std::format("DFRC {}:{}", i->second, j->second);
        }

        if (!ret.empty())
        {
            if (castling == "KQkq")
            {
                // nothing to do
            }
            else if (castling == "-")
            {
                ret += " no castling";
            }
            else
            {
                ret += " ";
                ret += castling;
            }
        }
    }

    return ret;
}

template <typename NumberType>
NumberType toNumber(const char *str)
{
    NumberType ret { };
    const std::string_view sv { str };
    const std::from_chars_result result { std::from_chars(sv.data(), sv.data() + sv.size(), ret) };

    if (result.ec == std::errc { })
    {
        return ret;
    }

    throw std::runtime_error { std::format("Error converting '{}' to number", sv) };
}

struct OpeningInfo
{
    std::string eco;
    std::string opening;
    std::string variation;
};

class EcoPgnReaderActions : public pgn_reader::PgnReaderActions
{
private:
    const pgn_reader::ChessBoard *m_board { };
    std::map<pgn_reader::CompressedPosition_FixedLength, OpeningInfo> m_openings { };


    std::string m_eco { };
    std::string m_opening { };
    std::string m_variation { };

public:
    void setBoardReferences(
        const pgn_reader::ChessBoard &curBoard,
        const pgn_reader::ChessBoard &prevBoard) override
    {
        m_board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void gameStart() override
    {
        m_eco.clear();
        m_opening.clear();
        m_variation.clear();
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        // filter out opening tags
        if (key == "ECO")
            m_eco = value;
        else if (key == "Opening")
            m_opening = value;
        else if (key == "Variation")
            m_variation = value;
    }

    void gameTerminated(pgn_reader::PgnResult) override
    {
        pgn_reader::CompressedPosition_FixedLength pos;
        pgn_reader::PositionCompressor_FixedLength::compress(*m_board, pos);

        if constexpr (debugMode)
        {
            std::cout << std::format("Adding eco={} opening={} variation={} for below position", m_eco, m_opening, m_variation) << std::endl;
            m_board->printBoard();
        }

        m_openings.emplace(std::make_pair(pos, OpeningInfo { std::move(m_eco), std::move(m_opening), std::move(m_variation) }));
    }

    const OpeningInfo *getOpeningForPosition(const pgn_reader::ChessBoard &board) const
    {
        pgn_reader::CompressedPosition_FixedLength pos;
        pgn_reader::PositionCompressor_FixedLength::compress(board, pos);

        auto i { std::as_const(m_openings).find(pos) };
        if (i != m_openings.end())
            return &(i->second);
        else
            return nullptr;
    }
};

class EventScannerActions : public pgn_reader::PgnReaderActions
{
private:
    std::string m_previousEvent { };
    std::uint32_t m_numSubEvents { };

public:
    void pgnTag(std::string_view key, std::string_view value) override
    {
        // filter out anything that is not the Event tag
        if (key != "Event")
            return;

        // do we already have this event?
        if (m_previousEvent != value)
        {
            m_previousEvent = value;
            ++m_numSubEvents;
        }
    }

    std::uint32_t getNumberOfSubEvents() const
    {
        return m_numSubEvents;
    }
};

class GameProcessorActions : public pgn_reader::PgnReaderActions
{
private:
    std::string_view m_urlPrefix { };
    const std::uint32_t m_seasonNumber;
    const std::uint32_t m_eventNumber;
    const std::uint32_t m_numSubEvents;
    const EcoPgnReaderActions &m_eco;

    const pgn_reader::ChessBoard *m_board { };
    std::uint32_t m_gameNo { };

    // PGN tags of the game
    std::array<std::string, std::tuple_size<decltype(ctKnownTagsInOrder)>::value> m_knownTagValues { };
    std::map<std::string_view, std::size_t> m_knownTagKeyToIndexMap { };

    // Values of "unknown" tags -- i.e., those that whose keys we don't know about.
    // We'll assume there are only ever a handful of these, so we'll just keep them in a linear vector
    std::vector<std::pair<std::string, std::string> > m_additionalPgnTags { };

    // Previous event key--used to detect the next subevent
    const std::basic_regex<char> m_eventNamePruneMatcher {
        std::regex("^(TCEC )?(Season [[:digit:]]+)?([[:space:]]*[-][[:space:]]*)?", std::regex::extended) };

    std::string m_previousEventValue { };
    std::string m_previousProcessedEventName { };
    std::uint32_t m_subEventNumber { };

    // previous game pending? This triggers printing out the moves and starting
    // a new game on pgnTag(), moveTextSection(), or endOfPGN()
    bool m_gamePending { };

    // moves of the game
    pgn_reader::ChessBoard m_initialBoard { };
    std::vector<pgn_reader::Move> m_moves { };

    // comments associated with moves. Note: these come just before the move
    std::vector<std::string> m_comments { };

    // result of the game
    pgn_reader::PgnResult m_result { };

    const OpeningInfo *m_openingInfo { };

    OutputBuffer out { };

    static constexpr std::string_view ctLiteralResultWhiteWin { "1-0" };
    static constexpr std::string_view ctLiteralResultDraw { "1/2-1/2" };
    static constexpr std::string_view ctLiteralResultBlackWin { "0-1" };
    static constexpr std::string_view ctLiteralResultUnknown { "*" };
    static constexpr std::string_view ctLiteralResultUnknown2 { "?" };
    static constexpr std::string_view ctLiteralDoubleNewLine { "\n\n" };
    static constexpr std::string_view ctLiteralBlockCommentStart { "{ " };
    static constexpr std::string_view ctLiteralBlockCommentEnd { " }" };
    static constexpr std::string_view ctLiteralPgnTagValueStart { " \"" };
    static constexpr std::string_view ctLiteralPgnTagValueEnd { "\"]\n" };


    void writeEscapedPgnValue(const std::string_view value)
    {
        for (char c : value)
        {
            if (c == '\\' || c == '"')
                out.write('\\');

            out.write(c);
        }
    }

    std::string &getValueRefForKnownPgnTag(KnownPgnTags tag)
    {
        const std::size_t i { static_cast<std::size_t>(tag) };
        return m_knownTagValues[i];
    }

    std::string getSubEventNameFromEventName(const std::string_view &eventName)
    {
        auto it = std::cregex_iterator(eventName.begin(), eventName.end(), m_eventNamePruneMatcher);
        auto end = std::cregex_iterator();

        if (it != end)
        {
            std::cmatch match = *it;
            return std::string { eventName.substr(match.length()) };
        }
        else
        {
            return std::string { eventName };
        }
    }

    void flushPreviousGameAndStartNew()
    {
        if (m_gamePending)
        {
            printTags();
            printMoves();
            m_gamePending = false;
        }

        m_moves.clear();
        m_moves.reserve(512U);
        m_comments.clear();
        m_comments.reserve(513U);
        m_openingInfo = nullptr;

        // clear all known tags
        for (auto &v : m_knownTagValues)
            v.clear();

        // clear all additional tags
        m_additionalPgnTags.clear();

        if constexpr(debugMode)
        {
            std::fputs(std::format("-- game {}\n", m_gameNo).c_str(), stderr);
        }
    }

    void printTags()
    {
        // tags with known keys and known order
        for (std::size_t i { }; i < m_knownTagValues.size(); ++i)
        {
            auto &value { m_knownTagValues[i] };
            if (value.empty())
                continue;

            out.write('[');
            out.write(ctKnownTagsInOrder[i]);
            out.write(ctLiteralPgnTagValueStart);
            writeEscapedPgnValue(value);
            out.write(ctLiteralPgnTagValueEnd);
        }

        // any remaining tag is written in the order we encountered it
        for (const auto &tag : m_additionalPgnTags)
        {
            out.write('[');
            out.write(std::string_view(tag.first));
            out.write(std::string_view(" \""));
            writeEscapedPgnValue(tag.second);
            out.write(std::string_view("\"]\n"));
        }

        // FRC/DFRC opening?
        if ((!getValueRefForKnownPgnTag(KnownPgnTags::FEN).empty()) &&
            getValueRefForKnownPgnTag(KnownPgnTags::ECO).empty() &&
            getValueRefForKnownPgnTag(KnownPgnTags::Opening).empty() &&
            getValueRefForKnownPgnTag(KnownPgnTags::Variation).empty())
        {
            std::string opening { classifyDfrc(getValueRefForKnownPgnTag(KnownPgnTags::FEN)) };
            if (!opening.empty())
            {
                out.write(std::string_view("[Opening \""));
                out.write(opening);
                out.write(std::string_view("\"]\n"));
            }
        }

        out.write('\n');
    }

    void printComment(std::string_view sv)
    {
        out.write(ctLiteralBlockCommentStart);
        bool eatWhiteSpace { true };
        for (char c : sv)
        {
            if (c <= ' ')
            {
                if (eatWhiteSpace)
                    continue;

                out.write(' ');
                eatWhiteSpace = true;
            }
            else
            {
                // map braces to parens, since braces can't appear in block comments
                if (c == '{')
                    c = '(';
                if (c == '}')
                    c = ')';

                out.write(c);
                eatWhiteSpace = false;
            }
        }

        out.write(ctLiteralBlockCommentEnd);
    }

    void printMoves()
    {
        bool spaceBeforeNextToken { };
        bool moveNumBeforeNextMove { true };
        pgn_reader::ChessBoard board { m_initialBoard };
        const std::size_t maxPrintPly { std::max(m_moves.size(), m_comments.size()) };

        for (size_t moveIndex { }; moveIndex < maxPrintPly; ++moveIndex)
        {
            if (moveIndex < m_comments.size() && !m_comments[moveIndex].empty())
            {
                if (spaceBeforeNextToken)
                {
                    out.write(' ');
                    spaceBeforeNextToken = false;
                }

                printComment(m_comments[moveIndex]);
                if (moveIndex == 0U)
                    out.write('\n');
                else
                    spaceBeforeNextToken = true;

                moveNumBeforeNextMove = true;
            }

            if (moveIndex < m_moves.size())
            {
                const std::uint_fast32_t plyNum { board.getCurrentPlyNum() };
                if (pgn_reader::colorOfPly(plyNum) == pgn_reader::Color::WHITE)
                    moveNumBeforeNextMove = true;

                if (spaceBeforeNextToken)
                {
                    out.write(' ');
                    spaceBeforeNextToken = false;
                }

                if (moveNumBeforeNextMove)
                {
                    out.write(pgn_reader::StringUtils::moveNumToString(pgn_reader::moveNumOfPly(plyNum), pgn_reader::colorOfPly(plyNum)));
                    out.write(' ');
                    moveNumBeforeNextMove = false;
                }

                out.write(pgn_reader::StringUtils::moveToSanAndPlay(board, m_moves[moveIndex]));
                spaceBeforeNextToken = true;
            }
        }

        if (spaceBeforeNextToken)
            out.write(' ');

        switch (m_result)
        {
            case pgn_reader::PgnResult::WHITE_WIN:
                out.write(ctLiteralResultWhiteWin);
                break;

            case pgn_reader::PgnResult::BLACK_WIN:
                out.write(ctLiteralResultBlackWin);
                break;

            case pgn_reader::PgnResult::DRAW:
                out.write(ctLiteralResultDraw);
                break;

            case pgn_reader::PgnResult::UNKNOWN:
                out.write('*');
                break;

            default:
                throw std::logic_error(std::format("Internal error: unknown result tag {}", static_cast<std::uint8_t>(m_result)));
        }

        out.write(ctLiteralDoubleNewLine);
    }

    void checkOpening()
    {
        const OpeningInfo *openingInfo { m_eco.getOpeningForPosition(*m_board) };

        if (openingInfo != nullptr)
            m_openingInfo = openingInfo;
    }

public:
    GameProcessorActions(std::uint32_t seasonNumber, std::uint32_t eventNumber,
                         std::uint32_t numSubEvents,
                         const EcoPgnReaderActions &eco) :
        m_seasonNumber { seasonNumber },
        m_eventNumber { eventNumber },
        m_numSubEvents { numSubEvents },
        m_eco { eco }
    {
        // build known tag key to index map
        for (std::size_t i { }; i < ctKnownTagsInOrder.size(); ++i)
        {
            const auto &key { ctKnownTagsInOrder[i] };
            m_knownTagKeyToIndexMap.emplace(key, i);
        }
    }

    void setUrlPrefix(std::string_view urlPrefix)
    {
        m_urlPrefix = urlPrefix;
        m_gameNo = 0U;
    }

    void setBoardReferences(
        const pgn_reader::ChessBoard &curBoard,
        const pgn_reader::ChessBoard &prevBoard) override
    {
        m_board = &curBoard;
        static_cast<void>(prevBoard);
    }

    void gameStart() override
    {
        ++m_gameNo;
    }

    void pgnTag(std::string_view key, std::string_view value) override
    {
        if (m_gamePending)
            flushPreviousGameAndStartNew();

        // we don't store these, since we're classifying openings ourselves
        if (key != "Opening" && key != "ECO" && key != "Variation" && key != "Site")
        {
            // do we know about this tag?
            const auto i { std::as_const(m_knownTagKeyToIndexMap).find(key) };
            if (i != m_knownTagKeyToIndexMap.end())
            {
                // ok, we do
                m_knownTagValues.at(i->second) = value;
            }
            else
            {
                // ok, we don't. Do we have this tag already?
                bool tagFound { };

                for (auto &tag : m_additionalPgnTags)
                {
                    if (tag.first == key)
                    {
                        tag.second = value;
                        tagFound = true;
                        break;
                    }
                }

                if (!tagFound)
                {
                    m_additionalPgnTags.emplace_back(std::string(key), std::string(value));
                }
            }
        }
    }

    void comment(std::string_view comment) override
    {
        m_comments.resize(m_moves.size() + 1U);

        if (m_comments.back().empty())
            m_comments.back() = comment;
        else
        {
            m_comments.back() += ' ';
            m_comments.back() += comment;
        }
    }

    void moveTextSection() override
    {
        if (m_gamePending)
            flushPreviousGameAndStartNew();

        m_initialBoard = *m_board;
        checkOpening();
    }

    void afterMove(pgn_reader::Move m) override
    {
        checkOpening();

        m_moves.push_back(m);
    }

    void gameTerminated(pgn_reader::PgnResult result) override
    {
        auto &event { getValueRefForKnownPgnTag(KnownPgnTags::Event) };
        if (!event.empty())
        {
            if (event != m_previousEventValue)
            {
                if (m_numSubEvents >= 2U)
                {
                    m_previousProcessedEventName = std::format(
                        "TCEC Season {:02} ({:02}{}) {}",
                        m_seasonNumber,
                        m_eventNumber,
                        static_cast<char>('a' + m_subEventNumber),
                        getSubEventNameFromEventName(event));
                }
                else
                {
                    m_previousProcessedEventName = std::format(
                        "TCEC Season {:02} ({:02}) {}",
                        m_seasonNumber,
                        m_eventNumber,
                        getSubEventNameFromEventName(event));
                }

                ++m_subEventNumber;
                m_previousEventValue = event;
            }

            event = m_previousProcessedEventName;
        }

        getValueRefForKnownPgnTag(KnownPgnTags::Site) = std::format("{}&game={}", m_urlPrefix, m_gameNo);

        if (!getValueRefForKnownPgnTag(KnownPgnTags::Result).empty())
        {
            const std::string &pgnResultTag { getValueRefForKnownPgnTag(KnownPgnTags::Result) };
            pgn_reader::PgnResult tagResult { };

            if (pgnResultTag == ctLiteralResultWhiteWin)
                tagResult = pgn_reader::PgnResult::WHITE_WIN;
            else if (pgnResultTag == ctLiteralResultBlackWin)
                tagResult = pgn_reader::PgnResult::BLACK_WIN;
            else if (pgnResultTag == ctLiteralResultDraw)
                tagResult = pgn_reader::PgnResult::DRAW;
            else if (pgnResultTag == ctLiteralResultUnknown)
                tagResult = pgn_reader::PgnResult::UNKNOWN;
            else if (pgnResultTag == ctLiteralResultUnknown2)
                tagResult = pgn_reader::PgnResult::UNKNOWN;
            else
            {
                throw std::runtime_error(
                    std::format("Bad result tag: '{}'", pgnResultTag));
            }

            if (tagResult != result)
            {
                throw std::runtime_error(
                    std::format("PGN tag '{}' mismatches with the game result.", pgnResultTag));
            }
        }
        else
        {
            std::string &pgnResultTag { getValueRefForKnownPgnTag(KnownPgnTags::Result) };
            switch (result)
            {
                case pgn_reader::PgnResult::WHITE_WIN:
                    pgnResultTag = ctLiteralResultWhiteWin;
                    break;

                case pgn_reader::PgnResult::BLACK_WIN:
                    pgnResultTag = ctLiteralResultBlackWin;
                    break;

                case pgn_reader::PgnResult::DRAW:
                    pgnResultTag = ctLiteralResultDraw;
                    break;

                case pgn_reader::PgnResult::UNKNOWN:
                    pgnResultTag = ctLiteralResultUnknown;
                    break;

                default:
                    throw std::runtime_error(
                        std::format("Bad result code: '{}'", static_cast<std::uint8_t>(result)));
            }
        }

        if (m_openingInfo != nullptr)
        {
            if (!m_openingInfo->eco.empty())
                getValueRefForKnownPgnTag(KnownPgnTags::ECO) = m_openingInfo->eco;
            if (!m_openingInfo->opening.empty())
                getValueRefForKnownPgnTag(KnownPgnTags::Opening) = m_openingInfo->opening;
            if (!m_openingInfo->variation.empty())
                getValueRefForKnownPgnTag(KnownPgnTags::Variation) = m_openingInfo->variation;
        }

        m_result = result;
        m_gamePending = true;
    }

    void endOfPGN() override
    {
        if (m_gamePending)
            flushPreviousGameAndStartNew();
    }
};

int processFullTcecPgnMain(int argc, char **argv) noexcept
{
    if ((argc < 6) || (argc % 2U) == 1U)
    {
        printHelp();
        return 127;
    }

    try
    {
        using pgn_reader::PgnReader;
        using pgn_reader::PgnReaderActionClass;
        using pgn_reader::PgnReaderActionFilter;

        std::vector<MemoryMappedFile> inputPgns;
        std::vector<std::string_view> urlPrefixes;

        const std::uint32_t seasonNumber { toNumber<std::uint32_t>(argv[1]) };
        const std::uint32_t eventNumber { toNumber<std::uint32_t>(argv[2]) };

        EcoPgnReaderActions ecoPgnActions { };
        EventScannerActions eventScannerActions { };

        {
            MemoryMappedFile ecoPgn;
            ecoPgn.map(argv[3], true, false);

            PgnReader::readFromMemory(
                ecoPgn.getStringView(),
                ecoPgnActions,
                PgnReaderActionFilter { PgnReaderActionClass::PgnTag, PgnReaderActionClass::Move });
            ecoPgn.unmap();
        }

        inputPgns.resize((argc - 4U) / 2U);
        urlPrefixes.reserve(inputPgns.size());
        for (std::size_t i { }; i < inputPgns.size(); ++i)
        {
            const char *inputPgnFile { argv[(i * 2U) + 4U] };

            if constexpr (debugMode)
                std::cout << std::format("Opening and mapping file {}...\n", inputPgnFile);

            inputPgns.at(i).map(inputPgnFile, true, false);

            urlPrefixes.push_back(argv[(i * 2U) + 5U]);
        }

        // go through the PGNs, collect unique event tags and assign sub-event-numbers if multiple
        {
            for (auto &inputPgn : inputPgns)
            {
                pgn_reader::PgnReader::readFromMemory(
                    inputPgn.getStringView(),
                    eventScannerActions,
                    PgnReaderActionFilter { PgnReaderActionClass::PgnTag });
            }
        }

        // go through the PGNs, collect moves and comments, normalize tags, and resolve opening tags
        {
            GameProcessorActions gameProcessorActions {
                seasonNumber, eventNumber, eventScannerActions.getNumberOfSubEvents(), ecoPgnActions };

            for (std::size_t i { }; i < inputPgns.size(); ++i)
            {
                auto &inputPgn { inputPgns.at(i) };
                gameProcessorActions.setUrlPrefix(urlPrefixes.at(i));
                pgn_reader::PgnReader::readFromMemory(
                    inputPgn.getStringView(),
                    gameProcessorActions,
                    PgnReaderActionFilter { PgnReaderActionClass::PgnTag, PgnReaderActionClass::Move, PgnReaderActionClass::Comment });
            }
        }

        for (auto &inputPgn : inputPgns)
            inputPgn.unmap();

        return 0;
    }
    catch (const pgn_reader::PgnError &pgnError)
    {
        std::fputs(pgnError.what(), stderr);
        std::fputs("\n", stderr);

        return 1;
    }
    catch (const std::exception &ex)
    {
        std::fputs(ex.what(), stderr);
        std::fputs("\n", stderr);

        return 2;
    }
}


}
}


int main(int argc, char **argv)
{
    return hoover_chess_utils::utils::processFullTcecPgnMain(argc, argv);
}
