#!/usr/bin/env python3
# Generates the seed corpus for re2_program_props_fuzzer.
#
# Framing: [options:1][maxlen:1][pattern_len:1][text_len:2 BE][pattern][text].
# Patterns are chosen to produce interesting program shapes (alternations,
# repeats, classes, anchors) so fanout histograms and PossibleMatchRange bound
# emission have real structure to walk; maxlen sweeps 1..12.
#
# Usage: re2_program_props_fuzzer_seeds.py <output-dir>

import os
import struct
import sys

SEEDS = [
    # Simple alternation under repeats: fanout-rich programs.
    (0x00, 9, b"(a|b)+c", b"ababc"),
    (0x00, 9, b"a+b|c*", b"cccaaa"),
    # Bounded repeat + class: wider bytemap, bigger programs.
    (0x00, 12, b"[a-z]{2,5}\\d+", b"abc12"),
    # Case-insensitive literal: foldcase byte ranges.
    (0x04, 5, b"hello", b"HeLLo world"),
    # POSIX longest-match program split.
    (0x18, 4, b"ab|abc", b"abc"),
    # Anchors + dollar: EmptyWidth assertions in the program.
    (0x00, 8, b"^x.*y$", b"x any y"),
    # Never-capture: flat program shape.
    (0x20, 3, b"(a)(b)(c)", b"abc"),
    # Latin-1 encoding with a high byte.
    (0x01, 7, b"\xe9+", b"\xe9\xe9z"),
    # Deep nesting: bigger reverse program.
    (0x00, 11, b"(?:((?:a|b)c))*d", b"acbcd"),
    # Literal option: prefix-accelerated program.
    (0x02, 2, b"prefix", b"the prefix of it"),
    # Tricky PossibleMatchRange input: empty-able pattern.
    (0x00, 6, b"(a*)*", b""),
    # Questionable ranges: guard the bound-emission logic.
    (0x00, 12, b"x(y[0-9]{1,3}|z){0,4}", b"xy12z"),
]


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: %s <output-dir>" % sys.argv[0])
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    for idx, (opts, maxlen, pattern, text) in enumerate(SEEDS):
        blob = (struct.pack(">BBBH", opts, maxlen, len(pattern), len(text)) +
                pattern + text)
        with open(os.path.join(out_dir, "seed_%02d" % idx), "wb") as f:
            f.write(blob)
    print("wrote %d seeds to %s" % (len(SEEDS), out_dir))


if __name__ == "__main__":
    main()
