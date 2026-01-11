#!/usr/bin/python3
#
# Hoover Chess Utilities / Elementary Bitboards submask/multiplier calculator
# Copyright (C) 2026  Sami Kiminki
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


def printBinaryAsBoard(val):
    binstr = "{:064b}".format(val)[::-1]
    print("+--------+ " + "{:016x}".format(val))
    for i in range(56,-8,-8):
        print("|" + binstr[i:i+8] + "|")
    print("+--------+")

def mul(a, b):
    return a * b

def findMultiplier(maskBit, targetBit, multiplier):
    assert(maskBit <= targetBit)

    return multiplier | (targetBit // maskBit)

def findMultiplierForMask(mask, targetMask, targetBits):
    origMask = mask
    remainder = 0
    consumedMask = 0
    consumedTargetBits = 0
    multiplier = 0

    while mask != 0:
        maskBit = mask & (-mask)
        targetBit = targetBits & (-targetBits)

        mask = mask ^ maskBit
        targetBits = targetBits ^ targetBit

        #print(f"finding multiplier for {maskBit:016x} to {targetBit:016x} for target mask {targetMask:016x}")

        tryMultiplier = findMultiplier(maskBit, targetBit, multiplier)

        # did we find a working multiplier?
        if (tryMultiplier != 0 and
            mul(consumedMask | maskBit, tryMultiplier) & targetMask == consumedTargetBits | targetBit):

            # ok, we found one
            multiplier = tryMultiplier
            consumedMask = consumedMask | maskBit
            consumedTargetBits = consumedTargetBits | targetBit

    return (consumedMask, multiplier)

def verifyMultiplier(mask, targetMask, consumedMask, multiplier):
    targetBit = targetMask & (-targetMask)

    while mask != 0:
        maskBit = mask & (-mask)

        if consumedMask & maskBit != 0:
            assert(mul(maskBit, multiplier) & targetMask == targetBit)

        targetBit = targetBit << 1;
        mask = mask ^ maskBit

def findAllMultipliersForMask(mask, targetMask):
    consumedMask = 0
    remainingTargetBits = targetMask

    maskMultipliers = []

    while (mask ^ consumedMask) != 0:
        #print(f"mask={mask:016x}  consumedMask={consumedMask:016x}  remainingTargetBits={remainingTargetBits:016x}")

        (cm, multiplier) = findMultiplierForMask(mask ^ consumedMask, targetMask, remainingTargetBits)
        #printBinaryAsBoard(cm)
        #print(f"- multiplier: {multiplier:016x}\n")
        verifyMultiplier(mask, targetMask , cm, multiplier)

        maskMultipliers.append((cm, multiplier))

        remainingTargetBits = remainingTargetBits ^ mul(cm, multiplier) & targetMask

        consumedMask = consumedMask | cm

    return maskMultipliers

def squareBit(col, row):
    return 1 << (8 * row + col)

def getRookMask(sq):
    col = sq & 7
    row = sq // 8

    mask = 0

    for c in range(1, 7):
        if c == col:
            continue
        mask = mask | squareBit(c, row)

    for r in range(1, 7):
        if r == row:
            continue
        mask = mask | squareBit(col, r)

    return mask

def getBishopMask(sq):
    col = sq & 7
    row = sq // 8

    mask = 0

    for i in range(1, 7):

        for dx in [-1, +1]:
            for dy in [-1, +1]:
                c = col + dx * i
                r = row - dy * i
                if (c in range(1, 7)) and (r in range(1, 7)):
                    mask = mask | squareBit(c, r)

    return mask


tableSize = 0

for sq in range(0,64):

    mask = getBishopMask(sq)
    #mask = getRookMask(sq)
    #printBinaryAsBoard(mask)

    tableSize = tableSize + (1 << mask.bit_count())

    targetMask = ((1 << (64 - mask.bit_count())) - 1) ^ 0xffffffffffffffff
    targetShift = 64 - mask.bit_count()
    #print(f"targetMask: {targetMask:016x}")

    maskMultipliers = findAllMultipliersForMask(mask, targetMask)

    while len(maskMultipliers) < 3:
        maskMultipliers.append((0,0))

    product = 0
    print("BitBoardTables::MasksAndMultipliers {")
    for maskMultiplier in maskMultipliers:
        mask = maskMultiplier[0]
        multiplier = maskMultiplier[1]
        product = product ^ mul(mask, multiplier)

    print("    {")
    for maskMultiplier in maskMultipliers:
        mask = maskMultiplier[0]
        print(f"        UINT64_C(0x{mask:016x}),")
    print("    },")

    print(f"    {targetShift}U, // target shift")

    print("    {")
    for maskMultiplier in maskMultipliers:
        multiplier = maskMultiplier[1]
        print(f"        UINT64_C(0x{multiplier:016x}),")
    print("    },")

    print(f"    0U  // zero padding")


    print(f"    // masked product: {product & targetMask:016x} masks={len(maskMultipliers)} / raw product: {product:016x}")
    assert(len(maskMultipliers) <= 4)

    print("},")

print(f"// PEXT table size: {tableSize}")
