#!/usr/bin/env python3
# Generates the seed corpus for re2_rewrite_extract_fuzzer.
#
# Seeds are structured (length-framed) inputs; see the harness header comment
# for the framing: [options:1][pattern_len:1][rewrite_len:1][text_len:2 BE]
# [pattern][rewrite][text]. Each seed targets a specific dark branch of the
# rewrite/extraction surface (rewrite tokens, invalid rewrites, empty-match
# skip-ahead, named groups, string-typed Arg extraction, ...).
#
# Usage: re2_rewrite_extract_fuzzer_seeds.py <output-dir>

import os
import struct
import sys

SEEDS = [
    # Basic capture + backref rewrites, both orders.
    (0x00, b"(a+)(b+)", b"\\2-\\1", b"aaabbb"),
    (0x00, b"(\\d+)-(\\d+)", b"[\\1/\\2]", b"123-456 and 7-89"),
    # Empty-match group: drives GlobalReplace's skip-ahead block.
    (0x00, b"(a*)*", b"<\\1>", b"aaaa"),
    # Escaped backslash token.
    (0x00, b"x", b"\\\\", b"yxyx"),
    # Backref beyond the number of capturing groups.
    (0x00, b"a", b"\\8", b"aaa"),
    # Invalid rewrite tokens (\\9 with no groups + trailing backslash).
    (0x00, b"a", b"\\9x\\\\", b"aa"),
    # Latin-1 encoding with high bytes.
    (0x01, b"\xff+", b"[\\0]", b"\xff\xffz"),
    # Alternating groups with empty captures.
    (0x00, b"(a)|(b)", b"\\2\\1", b"ab"),
    # Named group + rewrite.
    (0x00, b"(?P<n>x)(y)", b"\\1", b"xy"),
    # Many groups: large submatch vectors.
    (0x00, b"(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)(m)(n)", b"\\14",
     b"abcdefghijklmn"),
    # longest_match option with rewrite.
    (0x20, b"(word)", b"<\\1>", b"longer text with word inside"),
    # Case-insensitive pattern + optional group extraction.
    (0x00, b"(?i)(AB)+(C)?", b"\\1;\\2", b"ABabABC"),
]


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: %s <output-dir>" % sys.argv[0])
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    for idx, (opts, pattern, rewrite, text) in enumerate(SEEDS):
        blob = (struct.pack(">BBBH", opts, len(pattern), len(rewrite),
                            len(text)) + pattern + rewrite + text)
        with open(os.path.join(out_dir, "seed_%02d" % idx), "wb") as f:
            f.write(blob)
    print("wrote %d seeds to %s" % (len(SEEDS), out_dir))


if __name__ == "__main__":
    main()
