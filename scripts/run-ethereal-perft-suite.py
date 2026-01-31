#!/usr/bin/python3
#
# Hoover Chess Utilities / Ethereal perft test suite runner
# Copyright (C) 2018-2025  Andrew Grant, Sami Kiminki
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

# Note: The original script and EPD files can be found at:
# https://github.com/AndyGrant/Ethereal/tree/8f952ee13081fe5ef7d8d955b137efc122ea1a57/src/perft

import os
import re
import subprocess
import sys
import concurrent.futures

def run_single_perft(perft_path, fen, depth, perftMode):
    result = subprocess.run([ perft_path, perftMode, str(depth), fen ], capture_output=True, text=True)

    if result.returncode != 0 or len(result.stderr) > 0:
        return -1

    lines = result.stdout.split("\n")
    return int((lines[0].split(' ')[1]))

def run_perft_depths(perft_path, line, maxDepth, lineNum, numLines, perftMode):
    reDepthSpec = re.compile(r'D([0-9]+) ([0-9]+)')

    tokens = line.split(';')

    fen = tokens[0].strip()
    for depthSpec in tokens[1:]:
        depthMatch = reDepthSpec.fullmatch(depthSpec.strip())
        if depthMatch:
            depth = int(depthMatch.group(1))

            if depth <= maxDepth:
                expect_nodes = int(depthMatch.group(2))
                perft_nodes = run_single_perft(perft_path, fen, depth, perftMode)

                print(f"[{lineNum}/{numLines}] {fen} depth={depth} expect_nodes={expect_nodes} perft_nodes={perft_nodes}")

                if perft_nodes != expect_nodes:
                    raise Exception(f"[{lineNum}] Perft MISMATCH: FEN=\"{fen}\" depth={depth} expect={expect_nodes} perft={perft_nodes}")

    return True

def run_perfts(perftPath, suitePath, maxDepth, perftMode):
    lineList = [ ]

    with open(suitePath) as suite:
        for line in suite:
            lineList.append(line.strip())

    lineNum = 0

    futureList = [ ]
    exceptionList = [ ]

    with concurrent.futures.ThreadPoolExecutor(max_workers = os.cpu_count() - 1) as executor:
        for line in lineList:
            lineNum = lineNum + 1
            line = line.strip()
            futureList.append(
                executor.submit(run_perft_depths, perftPath, line, maxDepth, lineNum, len(lineList), perftMode))

        for future in futureList:
            try:
                result = future.result()
            except Exception as exc:
                exceptionList.append(exc)

    if len(exceptionList) > 0:
        print("Errors:")
        for exc in exceptionList:
            print(f"- {exc}")
        sys.exit(2)
    else:
        print("Success!")

def help_and_exit():
    print('Usage: run-ethereal-perft-suite.py [--bulk-moves|--leaf-moves|--play-moves] <path-to-Perft> <suite.epd> [max_depth]')
    sys.exit(1)

def main(argv):
    perft_mode = "--bulk-moves"
    if len(argv) >= 1:
        if argv[0] == "--bulk-moves":
            perft_mode = "--bulk-moves"
            argv = argv[1:]
        elif argv[0] == "--leaf-moves":
            perft_mode = "--leaf-moves"
            argv = argv[1:]
        elif argv[0] == "--play-moves":
            perft_mode = "--play-moves"
            argv = argv[1:]
        elif argv[0] == "--help":
            help_and_exit()

    if len(argv) < 1:
        help_and_exit()

    perft_path = argv[0]
    suite_path = argv[1]
    if len(argv) >= 3:
        max_depth = int(argv[2])
    else:
        max_depth = 255

    run_perfts(perft_path, suite_path, max_depth, perft_mode)

if __name__ == "__main__":
    main(sys.argv[1:])
