#!/usr/bin/python3
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

import os
import re
import sys


def help_and_exit():
    print('''Usage: check-source-preambles.py

Checks that all source, header, and other files have the expected preambles.''')
    sys.exit(1)

def fatal(str, exitcode):
    sys.stderr.write(f"Fatal: {str} (exit={exitcode})\n")
    sys.exit(exitcode)

def checkLicense(filename, lineCommentStart, skipLines):

    lineMatchers = [
        r'Hoover Chess Utilities / .*',
        r'Copyright \(C\) 20[1-9][0-9](-20[2-9][0-9])?  .*',
        r'',
        r'This program is free software: you can redistribute it and/or modify',
        r'it under the terms of the GNU General Public License as published by',
        r'the Free Software Foundation, either version 3 of the License, or',
        r'\(at your option\) any later version\.',
        r'',
        r'This program is distributed in the hope that it will be useful,',
        r'but WITHOUT ANY WARRANTY; without even the implied warranty of',
        r'MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\.  See the',
        r'GNU General Public License for more details\.',
        r'',
        r'You should have received a copy of the GNU General Public License',
        r'along with this program\.  If not, see <https://www\.gnu\.org/licenses/>\.'
    ]

    lineNum = 0
    with open(filename) as f:
        for line in f:
            if skipLines > 0:
                skipLines = skipLines - 1
                continue

            if lineNum >= len(lineMatchers):
                return

            if not line.startswith(lineCommentStart):
                print("Failed!")
                print(f"Expected line start   : '{lineCommentStart}'")
                print(f"Input                 : '{line}'")
                fatal("Header preamble check failed", 2)

            line = line[len(lineCommentStart):].strip()

            lineMatchRegex = lineMatchers[lineNum]
            if not re.fullmatch(lineMatchRegex, line):
                print("Failed!")
                print(f"Expected match against: '{lineMatchRegex}'")
                print(f"Input                 : '{line}'")
                fatal("Header preamble check failed", 2)

            lineNum = lineNum + 1

def checkCppHeaderGuardian(filename):
    guardianToken = (
        "HOOVER_CHESS_UTILS__" + filename.replace('/include/', '/').replace('/src/', '/').replace('/test/', '/').
        replace("-", "_").replace("/", "__").replace(".", "_").upper()) + "_INCLUDED"

    lineNum = 0
    with open(filename) as f:
        for line in f:
            lineNum = lineNum + 1

            if lineNum <= 16:
                continue
            if lineNum >= 19:
                break

            line = line.strip()

            if lineNum == 17:
                expectedLine = '#ifndef ' + guardianToken
            else:
                expectedLine = '#define ' + guardianToken

            if line != expectedLine:
                print(f"Expected line         : '{expectedLine}'")
                print(f"Input                 : '{line}'")
                fatal("Header guardian check failed", 3)


def checkFile(filename):
    print(f"Checking {filename}...")
    skipLines = 0
    if filename.endswith("CMakeLists.txt") or filename.endswith(".cmake"):
        lineCommentStart = '#'
    elif (filename.endswith(".bash") or filename.endswith(".sh") or
          filename.endswith(".py")):
        lineCommentStart = '#'
        skipLines = 2
    elif filename.endswith(".cc") or filename.endswith(".dox"):
        lineCommentStart = '//'
    elif filename.endswith(".l"):
        lineCommentStart = ' *'
        skipLines = 1
    elif filename.endswith(".h"):
        lineCommentStart = '//'
        checkCppHeaderGuardian(filename)
    elif (filename.endswith(".pgn") or filename.endswith(".epd") or filename.endswith(".css") or
          filename.endswith("pgn-reader/src/slider-attacks-elementary-bishop.inc") or
          filename.endswith("pgn-reader/src/slider-attacks-elementary-rook.inc") or
          filename.endswith("pgn-reader/src/slider-attacks-pext-pdep-bishop.inc") or
          filename.endswith("pgn-reader/src/slider-attacks-pext-pdep-rook.inc") or
          filename.endswith("config.h.in") or filename.endswith("README.txt") or
          filename.endswith("README.md") or filename.endswith(".html") or
          filename.endswith("LICENSE")):
        # ignore
        return
    else:
        fatal("Don't know how to handle this file", 1)

    checkLicense(filename, lineCommentStart, skipLines)

def main(argv):
    if len(argv) != 0:
        help_and_exit()

    with os.scandir(".") as it:
        for entry in it:
            if not entry.name.startswith('.') and entry.is_file():
                checkFile(entry.name)
    for path, subdirs, files in os.walk("pgn-reader"):
        for name in files:
            checkFile(os.path.join(path, name))
    for path, subdirs, files in os.walk("utils"):
        for name in files:
            checkFile(os.path.join(path, name))
    for path, subdirs, files in os.walk("scripts"):
        for name in files:
            checkFile(os.path.join(path, name))
    for path, subdirs, files in os.walk("html"):
        for name in files:
            checkFile(os.path.join(path, name))
    for path, subdirs, files in os.walk("doc"):
        for name in files:
            checkFile(os.path.join(path, name))

if __name__ == "__main__":
    main(sys.argv[1:])
