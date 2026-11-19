#!/usr/bin/env python3
# Generates the seed corpus for re2_filtered_set_fuzzer.
#
# Framing: [options:1][npatterns:1][text_len:2 BE][plen_0..plen_{n-1}]
# [pattern_0..pattern_{n-1}][text]. Patterns are chosen to produce prefilter
# atoms (required literal substrings) so that Compile/AllMatches/
# AllPotentials/PropagateMatch have real trees to prune, and texts are chosen
# so that only SOME atoms match (partial feedback, the dark direction).
#
# Usage: re2_filtered_set_fuzzer_seeds.py <output-dir>

import os
import struct
import sys

SEEDS = [
    # Two patterns, both partially matching (foo matches, gamma doesn't).
    (0x00, [b"foo.*bar", b"alpha.+gamma"], b"the foo bar alpha corpus"),
    # Numeric atoms + literal phrase, only some match.
    (0x00, [b"ID-[0-9]{4}", b"disk error", b"notice"],
     b"disk error ID-1234 seen"),
    # Case-insensitive pattern plus a phone-number class.
    (0x02, [b"(?i)warning", b"[0-9]{3}-[0-9]{4}"], b"call 555-0123 WARNING"),
    # Alternation producing nested prefilter nodes.
    (0x00, [b"(alpha|beta)gamma", b"delta"], b"beta gamma delta"),
    # Tiny atoms, only z matches.
    (0x00, [b"x(a|b)y", b"z"], b"xay bz"),
    # Class + literal prefix atoms.
    (0x00, [b"\\d+error", b"log.*fail"], b"2026 error log failure"),
    # Repeated group + plain literal.
    (0x00, [b"(test)+ing", b"abc"], b"testing testing abc"),
    # ANCHOR_START (opts>>6 = 1 -> 0x40) with ^ atoms.
    (0x40, [b"start", b"mid"], b"start of the middle"),
    # POSIX classes + two-word literal.
    (0x00, [b"[[:digit:]]{5}", b"hello world"], b"hello 12345 world"),
    # Deeply nested captures: atom extraction plus submatch-heavy compile.
    (0x00, [b"a(b(c(d)))", b"zzz"], b"abcd zzz"),
    # Single pattern (degenerate, keeps one-pattern trees covered too).
    (0x00, [b"needle"], b"a haystack with a needle inside"),
    # Many (8) patterns at once, mixed matching.
    (0x00, [b"aa", b"bb", b"cc", b"dd", b"ee", b"ff", b"gg", b"hh"],
     b"abcdefgh"),
]


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: %s <output-dir>" % sys.argv[0])
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    for idx, (opts, patterns, text) in enumerate(SEEDS):
        blob = struct.pack(">BBH", opts, len(patterns), len(text))
        blob += bytes(len(p) for p in patterns)
        for p in patterns:
            blob += p
        blob += text
        with open(os.path.join(out_dir, "seed_%02d" % idx), "wb") as f:
            f.write(blob)
    print("wrote %d seeds to %s" % (len(SEEDS), out_dir))


if __name__ == "__main__":
    main()
