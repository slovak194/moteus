#!/usr/bin/python3

"""Generate G-Code to perform the "lathe-like" operations on the outer
housing.
"""

import argparse

PREFIX = """
%
(AXIS,stop)
(MK2_OUTER_HOUSING_OP0_PART)
N10 G21
N15 G90 G94 G40 G17 G91.1
N20 G53 G0 Z0.
(OD FINISH)
N25 G49
N30 M5
N35 G53 G0 X63.5 Y63.5
N40 M0
N45 T10 M6
N50 S47000 M3
N55 G54 G0
N60 G43 Z66.799 H10

N100 G0 A0. B{bstart}
N200 G1 X0 Y{y:.3f} F10000.

N400 X0 Z53 F10000.

N500 Z{z:.3f} F750.
N550 G93
"""

POSTFIX = """
N5000 G94
N5010 F10000.
N5020 Z53

N5820 X46.788 Z68.284 F10000.
N5825 G49
N5830 G53 G0 Z0.
N5835 G49
N5840 G53 G0 X63.5 Y63.5
N5845 M5
N5855 M30
(AXIS,stop)
%
"""

MOVE = """
N{line1} B{b1} Y{y:.3f} F20
N{line2} B{b2} F2
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '-t', '--top', action='store_true',
        help='Initial top facing')
    group.add_argument(
        '-o', '--od', action='store_true',
        help='Outer diameter')

    args = parser.parse_args()


    REF_TO_STOCK_BOTTOM = 24.717
    STOCK_HEIGHT = 29.4
    MODEL_TOP = 26.8
    TOOL_DIAMETER = 4

    if args.top:
        # This gives about 1mm inside the ID of the stock
        Z_POSITION = 43

        YSTART = REF_TO_STOCK_BOTTOM + STOCK_HEIGHT + 0.5 * TOOL_DIAMETER
        YEND = REF_TO_STOCK_BOTTOM + MODEL_TOP + 0.5 * TOOL_DIAMETER
        YSTEP = 0.15
        BSTART = 0
    elif args.od:
        # This is the final radius of the outer housing.
        Z_POSITION = 50

        MARGIN = 0.65
        YSTART = REF_TO_STOCK_BOTTOM + MODEL_TOP + 0.5 * TOOL_DIAMETER + MARGIN
        YEND = REF_TO_STOCK_BOTTOM
        YSTEP = 0.65
        BSTART = 8000
    else:
        raise RuntimeError("No mode specified")

    y = YSTART
    b = BSTART
    line = 1000

    print(PREFIX.format(z=Z_POSITION, y=y, bstart=b))

    while True:
        y = max(y - YSTEP, YEND)

        print(MOVE.format(y=y, b1=b-20, b2=b-360, line1=line, line2 = line +5))

        b -= 360
        line += 10

        if y == YEND:
            break

    print("N{line} B{b} F2".format(line=line, b=b-360))

    print(POSTFIX)

if __name__ == '__main__':
    main()
