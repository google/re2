#!/usr/bin/env python3
# Generates the seed corpus for re2_engine_bias_fuzzer.
#
# Framing: [options:1][maxmem_exp:1][rounds:1][pattern_len:1][text_len:2 BE]
# [pattern][text]. Seeds bias the engine interiors: assertion-heavy anchored
# patterns, foldcase ranges, many captures, POSIX/longest-match toggles and
# small max_mem budgets (exponent up to 12 -> 16 KiB) over longer texts so
# DFA caches flush and fall back to the NFA.
#
# Usage: re2_engine_bias_fuzzer_seeds.py <output-dir>

import os
import struct
import sys

SEEDS = [
    # DFA first-match over a long alternating text, 4 rounds.
    (0x00, 3, 4, b"(a|b)*b", b"abababababababababab"),
    # POSIX + longest_match: reverse program and kLongestMatch DFA.
    (0x0C, 4, 2, b"(ab|a)(c|)", b"ababcabc"),
    # Foldcase byte ranges through the DFA byte map.
    (0x02, 5, 3, b"(abc|abd)+", b"ABCABDABc"),
    # Anchored start + word-boundary assertions.
    (0x40, 6, 2, b"\\b(foo|bar)\\b", b"foo bar foo"),
    # End assertions: \\B and \\z with shrinking endpos rounds.
    (0x00, 7, 4, b"\\Bx+\\z", b"aaaxxx"),
    # Eight captures: capture restore in NFA/bitstate bookkeeping.
    (0x00, 8, 3, b"(a)(b)(c)(d)(e)(f)(g)(h)", b"abcdefgh"),
    # Tiny budget (exponent 12 -> 16 KiB): DFA cache thrash -> NFA fallback.
    (0x00, 12, 4, b".*foo.*bar.*baz", b"xxfooyybarzzbazww"),
    # Latin-1 + case-insensitive high-byte ranges.
    (0x03, 5, 2, b"[\\xE0-\\xFF]{2,}", b"\xe0\xe1\xff\xe1"),
    # Multiline text with one_line off: $ and ^ EmptyWidth ops.
    (0x00, 6, 3, b"(?m)^x.*y$", b"x11y\nx22y\nx33y"),
    # Counted repeats: program size pressure on a small budget.
    (0x08, 11, 3, b"a{2,4}b{2,4}c", b"aabbaabbc"),
    # Empty-able pattern + \\B: EmptyWidth assertion paths in onepass/NFA.
    (0x00, 9, 2, b"(?:x)*y(z)?\\B", b"yyz"),
    # dot_nl over a text with newlines, ANCHOR_BOTH.
    (0x90, 10, 2, b"(?s)a.c", b"a\nc"),
]


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: %s <output-dir>" % sys.argv[0])
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    for idx, (opts, exp, rounds, pattern, text) in enumerate(SEEDS):
        blob = (struct.pack(">BBBBH", opts, exp, rounds, len(pattern),
                            len(text)) + pattern + text)
        with open(os.path.join(out_dir, "seed_%02d" % idx), "wb") as f:
            f.write(blob)
    print("wrote %d seeds to %s" % (len(SEEDS), out_dir))


if __name__ == "__main__":
    main()
