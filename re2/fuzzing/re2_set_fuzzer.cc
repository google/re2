// re2_set_fuzzer.cc
//
// Fuzz target for RE2::Set – the multi-pattern matching API.
// The existing OSS-Fuzz re2_fuzzer covers only single-pattern RE2::FullMatch.
// RE2::Set exercises a different code path:
//   re2/set.cc      – Add/Compile/Match driver
//   re2/prog.cc     – NFA/DFA multi-pattern bytecode
//   re2/dfa.cc      – DFA cache + multi-pattern accept states
//   re2/re2.cc      – per-pattern compile
//
// A bug here (e.g. OOB in DFA accept-state mapping) could affect any
// application using RE2::Set for routing/filtering.

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

#include "re2/re2.h"
#include "re2/set.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 4) return 0;

    // Split fuzz bytes: first 2 bytes encode number of patterns (1-4)
    // and per-pattern length hint; rest is input string + pattern data.
    size_t num_patterns = (data[0] % 4) + 1;
    size_t pat_len_hint = (data[1] % 16) + 1;

    const uint8_t *p = data + 2;
    size_t rem       = size - 2;

    RE2::Options opts;
    opts.set_log_errors(false);
    opts.set_max_mem(1 << 20);   // 1 MiB limit per pattern

    RE2::Set s(opts, RE2::UNANCHORED);

    // Add up to num_patterns patterns from fuzz data
    for (size_t i = 0; i < num_patterns && rem > 0; i++) {
        size_t plen = (rem < pat_len_hint) ? rem : pat_len_hint;
        std::string pat(reinterpret_cast<const char *>(p), plen);
        p   += plen;
        rem -= plen;
        int ignored;
        s.Add(pat, &ignored);
    }

    if (!s.Compile()) return 0;

    // Match remaining bytes as input
    std::string input(reinterpret_cast<const char *>(p), rem);
    std::vector<int> matches;
    s.Match(input, &matches);

    return 0;
}
