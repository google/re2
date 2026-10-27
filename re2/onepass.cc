// Copyright 2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Tested by search_test.cc.
//
// Prog::SearchOnePass is an efficient implementation of
// regular expression search with submatch tracking for
// what I call "one-pass regular expressions".  (An alternate
// name might be "backtracking-free regular expressions".)
//
// One-pass regular expressions have the property that
// at each input byte during an anchored match, there may be
// multiple alternatives but only one can proceed for any
// given input byte.
//
// For example, the regexp /x*yx*/ is one-pass: you read
// x's until a y, then you read the y, then you keep reading x's.
// At no point do you have to guess what to do or back up
// and try a different guess.
//
// On the other hand, /x*x/ is not one-pass: when you're
// looking at an input "x", it's not clear whether you should
// use it to extend the x* or as the final x.
//
// More examples: /([^ ]*) (.*)/ is one-pass; /(.*) (.*)/ is not.
// /(\d+)-(\d+)/ is one-pass; /(\d+).(\d+)/ is not.
//
// A simple intuition for identifying one-pass regular expressions
// is that it's always immediately obvious when a repetition ends.
// It must also be immediately obvious which branch of an | to take:
//
// /x(y|z)/ is one-pass, but /(xy|xz)/ is not.
//
// The NFA-based search in nfa.cc does some bookkeeping to
// avoid the need for backtracking and its associated exponential blowup.
// But if we have a one-pass regular expression, there is no
// possibility of backtracking, so there is no need for the
// extra bookkeeping.  Hence, this code.
//
// On a one-pass regular expression, the NFA code in nfa.cc
// runs at about 1/20 of the backtracking-based PCRE speed.
// In contrast, the code in this file runs at about the same
// speed as PCRE.
//
// One-pass regular expressions get used a lot when RE is
// used for parsing simple strings, so it pays off to
// notice them and handle them efficiently.
//
// See also Anne Brüggemann-Klein and Derick Wood,
// "One-unambiguous regular languages", Information and Computation 142(2).

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <limits>
#include <map>
#include <string>

#include "absl/base/call_once.h"
#include "absl/container/fixed_array.h"
#include "absl/container/inlined_vector.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "re2/pod_array.h"
#include "re2/prog.h"
#include "re2/sparse_set.h"
#include "util/utf.h"

// Silence "zero-sized array in struct/union" warning for OneState::action.
#ifdef _MSC_VER
#pragma warning(disable: 4200)
#endif

namespace re2 {

static const bool ExtraDebug = false;

// The key insight behind this implementation is that the
// non-determinism in an NFA for a one-pass regular expression
// is contained.  To explain what that means, first a
// refresher about what regular expression programs look like
// and how the usual NFA execution runs.
//
// In a regular expression program, only the kInstByteRange
// instruction processes an input byte c and moves on to the
// next byte in the string (it does so if c is in the given range).
// The kInstByteRange instructions correspond to literal characters
// and character classes in the regular expression.
//
// The kInstAlt instructions are used as wiring to connect the
// kInstByteRange instructions together in interesting ways when
// implementing | + and *.
// The kInstAlt instruction forks execution, like a goto that
// jumps to ip->out() and ip->out1() in parallel.  Each of the
// resulting computation paths is called a thread.
//
// The other instructions -- kInstEmptyWidth, kInstMatch, kInstCapture --
// are interesting in their own right but like kInstAlt they don't
// advance the input pointer.  Only kInstByteRange does.
//
// The automaton execution in nfa.cc runs all the possible
// threads of execution in lock-step over the input.  To process
// a particular byte, each thread gets run until it either dies
// or finds a kInstByteRange instruction matching the byte.
// If the latter happens, the thread stops just past the
// kInstByteRange instruction (at ip->out()) and waits for
// the other threads to finish processing the input byte.
// Then, once all the threads have processed that input byte,
// the whole process repeats.  The kInstAlt state instruction
// might create new threads during input processing, but no
// matter what, all the threads stop after a kInstByteRange
// and wait for the other threads to "catch up".
// Running in lock step like this ensures that the NFA reads
// the input string only once.
//
// Each thread maintains its own set of capture registers
// (the string positions at which it executed the kInstCapture
// instructions corresponding to capturing parentheses in the
// regular expression).  Repeated copying of the capture registers
// is the main performance bottleneck in the NFA implementation.
//
// A regular expression program is "one-pass" if, no matter what
// the input string, there is only one thread that makes it
// past a kInstByteRange instruction at each input byte.  This means
// that there is in some sense only one active thread throughout
// the execution.  Other threads might be created during the
// processing of an input byte, but they are ephemeral: only one
// thread is left to start processing the next input byte.
// This is what I meant above when I said the non-determinism
// was "contained".
//
// To execute a one-pass regular expression program, we can build
// a DFA (no non-determinism) that has at most as many states as
// the NFA (compare this to the possibly exponential number of states
// in the general case).  Each state records, for each possible
// input byte, the next state along with the conditions required
// before entering that state -- empty-width flags that must be true
// and capture operations that must be performed.  It also records
// whether a set of conditions required to finish a match at that
// point in the input rather than process the next byte.

// A state in the one-pass NFA - just an array of actions indexed
// by the bytemap_[] of the next input byte.  (The bytemap
// maps next input bytes into equivalence classes, to reduce
// the memory footprint.)
struct OneState {
  uint32_t matchcond;   // conditions to match right now.
  uint32_t action[];
};

struct OneState16 {
  uint16_t matchcond;   // conditions to match right now.
  uint16_t action[];
};

// The uint32_t conditions in the action are a combination of
// condition and capture bits and the next state.  The bottom 16 bits
// are the condition and capture bits, and the top 16 are the index of
// the next state.
//
// Bits 0-5 are the empty-width flags from prog.h.
// Bit 6 is kMatchWins, which means the match takes
// priority over moving to next in a first-match search.
// The remaining bits mark capture registers that should
// be set to the current input position.  The capture bits
// start at index 2, since the search loop can take care of
// cap[0], cap[1] (the overall match position).
// That means we can handle up to 5 capturing parens: $1 through $4, plus $0.
// No input position can satisfy both kEmptyWordBoundary
// and kEmptyNonWordBoundary, so we can use that as a sentinel
// instead of needing an extra bit.

static const int    kIndexShift   = 16;  // number of bits below index
static const int    kEmptyShift   = 6;   // number of empty flags in prog.h
static const int    kRealCapShift = kEmptyShift + 1;
static const int    kRealMaxCap   = (kIndexShift - kRealCapShift) / 2 * 2;

// Parameters used to skip over cap[0], cap[1].
static const int    kCapShift     = kRealCapShift - 2;
static const int    kMaxCap       = kRealMaxCap + 2;

static const uint32_t kMatchWins  = 1 << kEmptyShift;
static const uint32_t kCapMask    = ((1 << kRealMaxCap) - 1) << kRealCapShift;

static const uint32_t kImpossible = kEmptyWordBoundary | kEmptyNonWordBoundary;
static const uint16_t kInvalidAction16 = 0xFFFF;
static const uint8_t  kPaletteCondShift = 0xFF;

// Check, at compile time, that prog.h agrees with math above.
// This function is never called.
void OnePass_Checks() {
  static_assert((1<<kEmptyShift)-1 == kEmptyAllFlags,
                "kEmptyShift disagrees with kEmptyAllFlags");
  // kMaxCap counts pointers, kMaxOnePassCapture counts pairs.
  static_assert(kMaxCap == Prog::kMaxOnePassCapture*2,
                "kMaxCap disagrees with kMaxOnePassCapture");
  static_assert(sizeof(OneState16) == 2,
                "OneState16 must be 2 bytes");
  static_assert(sizeof(OneState) == 4,
                "OneState must be 4 bytes");
}

static bool Satisfy(uint32_t cond, absl::string_view context, const char* p) {
  uint32_t satisfied = Prog::EmptyFlags(context, p);
  if (cond & kEmptyAllFlags & ~satisfied)
    return false;
  return true;
}

// Apply the capture bits in cond, saving p to the appropriate
// locations in cap[].
static void ApplyCaptures(uint32_t cond, const char* p,
                          const char** cap, int ncap) {
  for (int i = 2; i < ncap; i++)
    if (cond & (1 << kCapShift << i))
      cap[i] = p;
}

// Computes the OneState* for the given nodeindex.
static inline OneState* IndexToNode(uint8_t* nodes, int statesize,
                                    int nodeindex) {
  return reinterpret_cast<OneState*>(nodes + statesize*nodeindex);
}

static inline OneState16* IndexToNode16(uint8_t* nodes, int statesize,
                                        int nodeindex) {
  return reinterpret_cast<OneState16*>(nodes + statesize*nodeindex);
}

// Copies a 32-bit one-pass NFA into 16-bit cells: the match conditions as
// they are, each action through encode().
template <typename Encode>
static void CopyTo16(uint8_t* src_nodes, int statesize, uint8_t* dst_nodes,
                     int statesize16, int nalloc, int bytemap_range,
                     Encode encode) {
  for (int i = 0; i < nalloc; i++) {
    const OneState* src = IndexToNode(src_nodes, statesize, i);
    OneState16* dst = IndexToNode16(dst_nodes, statesize16, i);
    dst->matchcond = static_cast<uint16_t>(src->matchcond);
    for (int b = 0; b < bytemap_range; b++)
      dst->action[b] = encode(src->action[b]);
  }
}

template <typename StateT, uint8_t kActionShift, uint8_t kCondShift,
          bool kUsePalette>
static bool SearchOnePassLoop(
    uint8_t* nodes, int bytemap_range, const uint8_t* bytemap,
    absl::string_view text, absl::string_view context,
    Prog::MatchKind kind, absl::string_view* match, int nmatch, int ncap,
    const char** cap, const char** matchcap) {
  const uint16_t* palette = NULL;
  if constexpr (kUsePalette) {
    palette = reinterpret_cast<const uint16_t*>(nodes);
    nodes += (sizeof(uint16_t) << kActionShift);
  }
  const int statesize =
      sizeof(StateT) + bytemap_range * sizeof(StateT::matchcond);
  constexpr uint32_t kActionMask = (uint32_t{1} << kActionShift) - 1;
  // In the shift encodings (no empty-width flags), keep the condition bits
  // as they sit in the 16-bit action (cond >> kEmptyShift) and test them with
  // shifted masks; decode only when applying captures.
  constexpr bool kShiftMode =
      sizeof(StateT) == sizeof(uint16_t) && kCondShift == kEmptyShift;
  constexpr int kCondDecodeShift = kShiftMode ? kEmptyShift : 0;
  constexpr uint32_t kMatchWinsC = kMatchWins >> kCondDecodeShift;
  constexpr uint32_t kCapMaskC = kCapMask >> kCondDecodeShift;
  // start() is always mapped to the zeroth OneState.
  StateT* state =
      reinterpret_cast<StateT*>(nodes);
  const char* bp = text.data();
  const char* ep = text.data() + text.size();
  const char* p;
  bool matched = false;
  matchcap[0] = bp;
  cap[0] = bp;
  uint32_t nextmatchcond = state->matchcond;
  for (p = bp; p < ep; p++) {
    int c = bytemap[*p & 0xFF];
    uint32_t matchcond = nextmatchcond;
    uint32_t cond;

    // Determine whether we can reach act->next.
    // If so, advance state and nextmatchcond.
    if constexpr (sizeof(StateT) == sizeof(uint16_t)) {
      uint32_t act = state->action[c];
      if constexpr (kCondShift == kEmptyShift) {
        if (act != kInvalidAction16) {
          cond = act & kActionMask;
          uint32_t nextindex = act >> kActionShift;
          state = IndexToNode16(nodes, statesize, nextindex);
          nextmatchcond = state->matchcond;
        } else {
          cond = 0;
          state = NULL;
          nextmatchcond = kImpossible;
        }
      } else {
        if constexpr (kUsePalette) {
          cond = palette[act & kActionMask];
        } else {
          cond = act & kActionMask;
        }
        if ((cond & kEmptyAllFlags) == 0 || Satisfy(cond, context, p)) {
          uint32_t nextindex = act >> kActionShift;
          state = IndexToNode16(nodes, statesize, nextindex);
          nextmatchcond = state->matchcond;
        } else {
          state = NULL;
          nextmatchcond = kImpossible;
        }
      }
    } else {
      cond = state->action[c];
      if ((cond & kEmptyAllFlags) == 0 || Satisfy(cond, context, p)) {
        uint32_t nextindex = cond >> kIndexShift;
        state = IndexToNode(nodes, statesize, nextindex);
        nextmatchcond = state->matchcond;
      } else {
        state = NULL;
        nextmatchcond = kImpossible;
      }
    }

    // This code section is carefully tuned.
    // The goto sequence is about 10% faster than the
    // obvious rewrite as a large if statement in the
    // ASCIIMatchRE2 and DotMatchRE2 benchmarks.

    // Saving the match capture registers is expensive.
    // Is this intermediate match worth thinking about?

    // Not if we want a full match.
    if (kind == Prog::kFullMatch)
      goto skipmatch;

    // Not if it's impossible.
    if (matchcond == kImpossible)
      goto skipmatch;

    // Not if the possible match is beaten by the certain
    // match at the next byte.  When this test is useless
    // (e.g., HTTPPartialMatchRE2) it slows the loop by
    // about 10%, but when it avoids work (e.g., DotMatchRE2),
    // it cuts the loop execution by about 45%.
    if ((cond & kMatchWinsC) == 0 && (nextmatchcond & kEmptyAllFlags) == 0)
      goto skipmatch;

    // Finally, the match conditions must be satisfied.
    if ((matchcond & kEmptyAllFlags) == 0 || Satisfy(matchcond, context, p)) {
      for (int i = 2; i < 2*nmatch; i++)
        matchcap[i] = cap[i];
      if (nmatch > 1 && (matchcond & kCapMask))
        ApplyCaptures(matchcond, p, matchcap, ncap);
      matchcap[1] = p;
      matched = true;

      // If we're in longest match mode, we have to keep
      // going and see if we find a longer match.
      // In first match mode, we can stop if the match
      // takes priority over the next state for this input byte.
      // That bit is per-input byte and thus in cond, not matchcond.
      if (kind == Prog::kFirstMatch && (cond & kMatchWinsC))
        goto done;
    }

  skipmatch:
    if (state == NULL)
      goto done;
    if ((cond & kCapMaskC) && nmatch > 1)
      ApplyCaptures(cond << kCondDecodeShift, p, cap, ncap);
  }

  // Look for match at end of input.
  {
    uint32_t matchcond = state->matchcond;
    if (matchcond != kImpossible &&
        ((matchcond & kEmptyAllFlags) == 0 || Satisfy(matchcond, context, p))) {
      if (nmatch > 1 && (matchcond & kCapMask))
        ApplyCaptures(matchcond, p, cap, ncap);
      for (int i = 2; i < ncap; i++)
        matchcap[i] = cap[i];
      matchcap[1] = p;
      matched = true;
    }
  }

done:
  if (!matched)
    return false;
  for (int i = 0; i < nmatch; i++)
    match[i] = absl::string_view(
        matchcap[2 * i],
        static_cast<size_t>(matchcap[2 * i + 1] - matchcap[2 * i]));
  return true;
}

bool Prog::SearchOnePass(absl::string_view text, absl::string_view context,
                         Anchor anchor, MatchKind kind,
                         absl::string_view* match, int nmatch) {
  if (anchor != kAnchored && kind != kFullMatch) {
    ABSL_LOG(DFATAL) << "Cannot use SearchOnePass for unanchored matches.";
    return false;
  }

  // Make sure we have at least cap[1],
  // because we use it to tell if we matched.
  int ncap = 2*nmatch;
  if (ncap < 2)
    ncap = 2;

  const char* cap[kMaxCap];
  for (int i = 0; i < ncap; i++)
    cap[i] = NULL;

  const char* matchcap[kMaxCap];
  for (int i = 0; i < ncap; i++)
    matchcap[i] = NULL;

  if (context.data() == NULL)
    context = text;
  if (anchor_start() && BeginPtr(context) != BeginPtr(text))
    return false;
  if (anchor_end() && EndPtr(context) != EndPtr(text))
    return false;
  if (anchor_end())
    kind = kFullMatch;

  // Build the one-pass NFA on first use, unless IsOnePass() kept it.
  // IsOnePass() has already run the analysis and cut the NFA's memory
  // from the DFA budget.
  ABSL_DCHECK(did_onepass_) << "SearchOnePass called before IsOnePass";
  absl::call_once(onepass_once_, [](Prog* prog) {
    if (prog->is_onepass_ && prog->onepass_nodes_.data() == NULL) {
      int nbytes = prog->ComputeOnePass(&prog->onepass_nodes_,
                                        std::numeric_limits<int>::max());
      ABSL_DCHECK_GE(nbytes, 0);
    }
  }, this);
  uint8_t* nodes = onepass_nodes_.data();
  if (nodes == NULL) {
    ABSL_LOG(DFATAL) << "SearchOnePass called on a non-one-pass program.";
    return false;
  }
  if (onepass_action_shift_ < kIndexShift) {
    if (onepass_cond_shift_ == kEmptyShift) {
      if (onepass_action_shift_ == 1) {
        return SearchOnePassLoop<OneState16, 1, kEmptyShift, false>(
            nodes, bytemap_range(), bytemap_, text, context, kind, match,
            nmatch, ncap, cap, matchcap);
      }
      if (onepass_action_shift_ == 7) {
        return SearchOnePassLoop<OneState16, 7, kEmptyShift, false>(
            nodes, bytemap_range(), bytemap_, text, context, kind, match,
            nmatch, ncap, cap, matchcap);
      }
      return SearchOnePassLoop<OneState16, 9, kEmptyShift, false>(
          nodes, bytemap_range(), bytemap_, text, context, kind, match,
          nmatch, ncap, cap, matchcap);
    }
    if (onepass_cond_shift_ == kPaletteCondShift) {
      if (onepass_action_shift_ == 4) {
        return SearchOnePassLoop<OneState16, 4, 0, true>(
            nodes, bytemap_range(), bytemap_, text, context, kind, match,
            nmatch, ncap, cap, matchcap);
      }
      return SearchOnePassLoop<OneState16, 6, 0, true>(
          nodes, bytemap_range(), bytemap_, text, context, kind, match,
          nmatch, ncap, cap, matchcap);
    }
    return SearchOnePassLoop<OneState16, 7, 0, false>(
        nodes, bytemap_range(), bytemap_, text, context, kind, match, nmatch,
        ncap, cap, matchcap);
  }
  return SearchOnePassLoop<OneState, kIndexShift, 0, false>(
      nodes, bytemap_range(), bytemap_, text, context, kind, match, nmatch,
      ncap, cap, matchcap);
}

// Analysis to determine whether a given regexp program is one-pass.

// If ip is not on workq, adds ip to work queue and returns true.
// If ip is already on work queue, does nothing and returns false.
// If ip is NULL, does nothing and returns true (pretends to add it).
typedef SparseSet Instq;
static bool AddQ(Instq *q, int id) {
  if (id == 0)
    return true;
  if (q->contains(id))
    return false;
  q->insert(id);
  return true;
}

struct InstCond {
  int id;
  uint32_t cond;
};

// Returns whether this is a one-pass program; that is,
// returns whether it is safe to use SearchOnePass on this program.
// These conditions must be true for any instruction ip:
//
//   (1) for any other Inst nip, there is at most one input-free
//       path from ip to nip.
//   (2) there is at most one kInstByte instruction reachable from
//       ip that matches any particular byte c.
//   (3) there is at most one input-free path from ip to a kInstMatch
//       instruction.
//
// This is actually just a conservative approximation: it might
// return false when the answer is true, when kInstEmptyWidth
// instructions are involved.
// On success, cuts the memory for the one-pass NFA from the DFA budget and
// keeps the NFA if it is small; SearchOnePass() builds a bigger one on first
// use.
bool Prog::IsOnePass() {
  if (did_onepass_)
    return is_onepass_;
  did_onepass_ = true;

  if (start() == 0)  // no match
    return false;

  // Steal memory for the one-pass NFA from the overall DFA budget.
  // Willing to use at most 1/4 of the DFA budget (heuristic).
  // Limit max node count to 65000 as a conservative estimate to
  // avoid overflowing 16-bit node index in encoding.
  int maxnodes = 2 + inst_count(kInstByteRange);
  int statesize = sizeof(OneState) + bytemap_range()*sizeof(uint32_t);
  if (maxnodes >= 65000 || dfa_mem_ / 4 / statesize < maxnodes)
    return false;

  // Keep a small one-pass NFA now, as before: building it again on first
  // use would cost more time than the memory is worth.
  int nbytes = ComputeOnePass(&onepass_nodes_, kMaxEagerOnePassBytes);
  if (nbytes < 0)
    return false;

  // A bigger one-pass NFA is not kept here: SearchOnePass() builds it again
  // on first use. RE2::Match calls SearchOnePass() only to extract
  // submatches (at most 4 groups), for anchored matches on texts of at most
  // 16 bytes, and when an anchored DFA search runs out of memory, so an RE2
  // that is used for none of these never builds it. Its memory is cut from
  // the DFA budget now, though, so that the budget does not depend on
  // whether (or when) that happens.
  dfa_mem_ -= nbytes;
  is_onepass_ = true;
  return true;
}

// Runs the one-pass analysis on a program that has passed the checks in
// IsOnePass(). Returns the size in bytes of the one-pass NFA, or -1 if the
// program is not one-pass. If out is not NULL and the NFA takes at most
// max_out_bytes bytes, saves the one-pass NFA there. Also picks the cell
// encoding (onepass_action_shift_, onepass_cond_shift_), which is the same
// on every call. The analysis only reads the program, which is immutable
// by now.
int Prog::ComputeOnePass(PODArray<uint8_t>* out, int max_out_bytes) {
  int maxnodes = 2 + inst_count(kInstByteRange);
  int statesize = sizeof(OneState) + bytemap_range()*sizeof(uint32_t);

  // Flood the graph starting at the start state, and check
  // that in each reachable state, each possible byte leads
  // to a unique next state.
  int stacksize = inst_count(kInstCapture) +
                  inst_count(kInstEmptyWidth) +
                  inst_count(kInstNop) + 1;  // + 1 for start inst
  absl::FixedArray<InstCond, 64> stack_storage(stacksize);
  InstCond* stack = stack_storage.data();

  int size = this->size();
  absl::FixedArray<int, 128> nodebyid_storage(size, -1);  // indexed by ip
  int* nodebyid = nodebyid_storage.data();

  // Originally, nodes was a uint8_t[maxnodes*statesize], but that was
  // unnecessarily optimistic: why allocate a large amount of memory
  // upfront for a large program when it is unlikely to be one-pass?
  absl::InlinedVector<uint8_t, 2048> nodes;
  Instq tovisit(size), workq(size);

  const int statesize16 =
      sizeof(OneState16) + bytemap_range() * sizeof(uint16_t);
  enum FastMode16 {
    kModeNone = 0,
    kModeShift1,
    kModeShift7,
    kModeShift9,
    kModeEmpty0,
    kModePalette4,
  };
  FastMode16 mode16 = kModeNone;
  const bool has_captures = inst_count(kInstCapture) > 0;
  if (!has_captures && inst_count(kInstEmptyWidth) == 0 &&
      maxnodes <= (kInvalidAction16 >> 1)) {
    mode16 = kModeShift1;
  } else if (!has_captures && maxnodes <= (1 << (16 - 7))) {
    mode16 = kModeEmpty0;
  } else if (inst_count(kInstEmptyWidth) == 0 &&
             maxnodes <= (kInvalidAction16 >> 7) &&
             inst_count(kInstCapture) <= 6) {
    mode16 = kModeShift7;
  } else if (inst_count(kInstEmptyWidth) == 0 &&
             maxnodes <= (kInvalidAction16 >> 9)) {
    mode16 = kModeShift9;
  } else if (maxnodes <= (1 << (16 - 4))) {
    mode16 = kModePalette4;
  }

  uint16_t palette[64];
  palette[0] = kImpossible;
  palette[1] = 0;
  int nconds = 2;
  uint32_t used_cond = 0;
  bool palette_ok = true;
  auto record_cond = [&](uint32_t c) {
    used_cond |= c;
    if (c != 0 && palette_ok) {
      uint16_t c16 = static_cast<uint16_t>(c);
      for (int idx = 2; idx < nconds; idx++) {
        if (palette[idx] == c16)
          return;
      }
      if (nconds < 64) {
        palette[nconds++] = c16;
      } else {
        palette_ok = false;
      }
    }
  };
  int nalloc = 1;

  if (mode16 != kModeNone && !ExtraDebug) {
    const uint16_t impossible16 =
        (mode16 == kModePalette4)
            ? 0
            : (mode16 == kModeEmpty0 ? kImpossible : kInvalidAction16);
    AddQ(&tovisit, start());
    nodebyid[start()] = 0;
    nalloc = 1;
    nodes.insert(nodes.end(), statesize16, 0);
    for (Instq::iterator it = tovisit.begin(); it != tovisit.end(); ++it) {
      int id = *it;
      int nodeindex = nodebyid[id];
      OneState16* node = IndexToNode16(nodes.data(), statesize16, nodeindex);

      if (mode16 == kModeEmpty0) {
        for (int b = 0; b < bytemap_range_; b++)
          node->action[b] = kImpossible;
      } else if (mode16 != kModePalette4) {
        memset(node->action, 0xFF, bytemap_range_ * sizeof(uint16_t));
      }
      node->matchcond = kImpossible;

      workq.clear();
      bool matched = false;
      int nstack = 0;
      stack[nstack].id = id;
      stack[nstack++].cond = 0;
      while (nstack > 0) {
        int id = stack[--nstack].id;
        uint32_t cond = stack[nstack].cond;

      Loop16:
        Prog::Inst* ip = inst(id);
        switch (ip->opcode()) {
          default:
            ABSL_LOG(DFATAL) << "unhandled opcode: " << ip->opcode();
            break;

          case kInstAltMatch:
            ABSL_DCHECK(!ip->last());
            if (!AddQ(&workq, id+1))
              goto fail;
            id = id+1;
            goto Loop16;

          case kInstByteRange: {
            int nextindex = nodebyid[ip->out()];
            if (nextindex == -1) {
              if (nalloc >= maxnodes)
                goto fail;
              nextindex = nalloc;
              AddQ(&tovisit, ip->out());
              nodebyid[ip->out()] = nalloc;
              nalloc++;
              nodes.insert(nodes.end(), statesize16, 0);
              node = IndexToNode16(nodes.data(), statesize16, nodeindex);
            }
            uint32_t step_cond = cond | (matched ? kMatchWins : 0);
            uint16_t newact;
            switch (mode16) {
              case kModeShift1:
                newact = static_cast<uint16_t>(
                    (nextindex << 1) | (step_cond >> kEmptyShift));
                break;
              case kModeShift7:
                if ((step_cond >> 13) != 0)
                  goto fallback32;
                newact = static_cast<uint16_t>(
                    (nextindex << 7) | (step_cond >> kEmptyShift));
                break;
              case kModeShift9:
                newact = static_cast<uint16_t>(
                    (nextindex << 9) | (step_cond >> kEmptyShift));
                break;
              case kModeEmpty0:
                newact = static_cast<uint16_t>((nextindex << 7) | step_cond);
                break;
              case kModePalette4:
              default: {
                uint16_t c16 = static_cast<uint16_t>(step_cond);
                int idx = 1;
                if (c16 != 0) {
                  idx = 2;
                  while (idx < nconds && palette[idx] != c16) {
                    idx++;
                  }
                  if (idx == nconds) {
                    if (nconds >= 16)
                      goto fallback32;
                    palette[nconds++] = c16;
                  }
                }
                newact = static_cast<uint16_t>((nextindex << 4) | idx);
                break;
              }
            }
            for (int c = ip->lo(); c <= ip->hi(); c++) {
              int b = bytemap_[c];
              while (c < 256-1 && bytemap_[c+1] == b)
                c++;
              uint16_t act = node->action[b];
              if (act == impossible16) {
                node->action[b] = newact;
              } else if (act != newact) {
                goto fail;
              }
            }
            if (ip->foldcase()) {
              Rune lo = std::max<Rune>(ip->lo(), 'a') + 'A' - 'a';
              Rune hi = std::min<Rune>(ip->hi(), 'z') + 'A' - 'a';
              for (int c = lo; c <= hi; c++) {
                int b = bytemap_[c];
                while (c < 256-1 && bytemap_[c+1] == b)
                  c++;
                uint16_t act = node->action[b];
                if (act == impossible16) {
                  node->action[b] = newact;
                } else if (act != newact) {
                  goto fail;
                }
              }
            }

            if (ip->last())
              break;
            if (!AddQ(&workq, id+1))
              goto fail;
            id = id+1;
            goto Loop16;
          }

          case kInstCapture:
          case kInstEmptyWidth:
          case kInstNop:
            if (!ip->last()) {
              if (!AddQ(&workq, id+1))
                goto fail;
              stack[nstack].id = id+1;
              stack[nstack++].cond = cond;
            }
            if (ip->opcode() == kInstCapture && ip->cap() < kMaxCap)
              cond |= (1 << kCapShift) << ip->cap();
            if (ip->opcode() == kInstEmptyWidth)
              cond |= ip->empty();
            if (!AddQ(&workq, ip->out()))
              goto fail;
            id = ip->out();
            goto Loop16;

          case kInstMatch:
            if (matched)
              goto fail;
            matched = true;
            node->matchcond = static_cast<uint16_t>(cond);
            if (ip->last())
              break;
            if (!AddQ(&workq, id+1))
              goto fail;
            id = id+1;
            goto Loop16;

          case kInstFail:
            break;
        }
      }
    }

    if (mode16 == kModePalette4) {
      onepass_action_shift_ = 4;
      onepass_cond_shift_ = kPaletteCondShift;
      const int palette_bytes = 16 * sizeof(uint16_t);
      const int total_bytes = palette_bytes + nalloc * statesize16;
      if (out != NULL && total_bytes <= max_out_bytes) {
        *out = PODArray<uint8_t>(total_bytes);
        memset(out->data(), 0, palette_bytes);
        memcpy(out->data(), palette, nconds * sizeof(uint16_t));
        memmove(out->data() + palette_bytes, nodes.data(),
                nalloc * statesize16);
      }
      return total_bytes;
    }

    if (mode16 == kModeEmpty0) {
      onepass_action_shift_ = 7;
      onepass_cond_shift_ = 0;
    } else {
      onepass_action_shift_ =
          (mode16 == kModeShift1) ? 1 : (mode16 == kModeShift7 ? 7 : 9);
      onepass_cond_shift_ = kEmptyShift;
    }
    const int total_bytes = nalloc * statesize16;
    if (out != NULL && total_bytes <= max_out_bytes) {
      *out = PODArray<uint8_t>(total_bytes);
      memmove(out->data(), nodes.data(), total_bytes);
    }
    return total_bytes;
  }

fallback32:
  nodes.clear();
  tovisit.clear();
  for (int i = 0; i < size; i++)
    nodebyid[i] = -1;
  nconds = 2;
  used_cond = 0;
  palette_ok = true;

  AddQ(&tovisit, start());
  nodebyid[start()] = 0;
  nalloc = 1;
  nodes.insert(nodes.end(), statesize, 0);
  for (Instq::iterator it = tovisit.begin(); it != tovisit.end(); ++it) {
    int id = *it;
    int nodeindex = nodebyid[id];
    OneState* node = IndexToNode(nodes.data(), statesize, nodeindex);

    // Flood graph using manual stack, filling in actions as found.
    // Default is none.
    for (int b = 0; b < bytemap_range_; b++)
      node->action[b] = kImpossible;
    node->matchcond = kImpossible;

    workq.clear();
    bool matched = false;
    int nstack = 0;
    stack[nstack].id = id;
    stack[nstack++].cond = 0;
    while (nstack > 0) {
      int id = stack[--nstack].id;
      uint32_t cond = stack[nstack].cond;

    Loop:
      Prog::Inst* ip = inst(id);
      switch (ip->opcode()) {
        default:
          ABSL_LOG(DFATAL) << "unhandled opcode: " << ip->opcode();
          break;

        case kInstAltMatch:
          // TODO(rsc): Ignoring kInstAltMatch optimization.
          // Should implement it in this engine, but it's subtle.
          ABSL_DCHECK(!ip->last());
          // If already on work queue, (1) is violated: bail out.
          if (!AddQ(&workq, id+1))
            goto fail;
          id = id+1;
          goto Loop;

        case kInstByteRange: {
          int nextindex = nodebyid[ip->out()];
          if (nextindex == -1) {
            if (nalloc >= maxnodes) {
              if (ExtraDebug)
                ABSL_LOG(ERROR) << absl::StrFormat(
                    "Not OnePass: hit node limit %d >= %d", nalloc, maxnodes);
              goto fail;
            }
            nextindex = nalloc;
            AddQ(&tovisit, ip->out());
            nodebyid[ip->out()] = nalloc;
            nalloc++;
            nodes.insert(nodes.end(), statesize, 0);
            // Update node because it might have been invalidated.
            node = IndexToNode(nodes.data(), statesize, nodeindex);
          }
          uint32_t step_cond = cond | (matched ? kMatchWins : 0);
          record_cond(step_cond);
          uint32_t newact = (nextindex << kIndexShift) | step_cond;
          for (int c = ip->lo(); c <= ip->hi(); c++) {
            int b = bytemap_[c];
            // Skip any bytes immediately after c that are also in b.
            while (c < 256-1 && bytemap_[c+1] == b)
              c++;
            uint32_t act = node->action[b];
            if ((act & kImpossible) == kImpossible) {
              node->action[b] = newact;
            } else if (act != newact) {
              if (ExtraDebug)
                ABSL_LOG(ERROR) << absl::StrFormat(
                    "Not OnePass: conflict on byte %#x at state %d", c, *it);
              goto fail;
            }
          }
          if (ip->foldcase()) {
            Rune lo = std::max<Rune>(ip->lo(), 'a') + 'A' - 'a';
            Rune hi = std::min<Rune>(ip->hi(), 'z') + 'A' - 'a';
            for (int c = lo; c <= hi; c++) {
              int b = bytemap_[c];
              // Skip any bytes immediately after c that are also in b.
              while (c < 256-1 && bytemap_[c+1] == b)
                c++;
              uint32_t act = node->action[b];
              if ((act & kImpossible) == kImpossible) {
                node->action[b] = newact;
              } else if (act != newact) {
                if (ExtraDebug)
                  ABSL_LOG(ERROR) << absl::StrFormat(
                      "Not OnePass: conflict on byte %#x at state %d", c, *it);
                goto fail;
              }
            }
          }

          if (ip->last())
            break;
          // If already on work queue, (1) is violated: bail out.
          if (!AddQ(&workq, id+1))
            goto fail;
          id = id+1;
          goto Loop;
        }

        case kInstCapture:
        case kInstEmptyWidth:
        case kInstNop:
          if (!ip->last()) {
            // If already on work queue, (1) is violated: bail out.
            if (!AddQ(&workq, id+1))
              goto fail;
            stack[nstack].id = id+1;
            stack[nstack++].cond = cond;
          }

          if (ip->opcode() == kInstCapture && ip->cap() < kMaxCap)
            cond |= (1 << kCapShift) << ip->cap();
          if (ip->opcode() == kInstEmptyWidth)
            cond |= ip->empty();

          // kInstCapture and kInstNop always proceed to ip->out().
          // kInstEmptyWidth only sometimes proceeds to ip->out(),
          // but as a conservative approximation we assume it always does.
          // We could be a little more precise by looking at what c
          // is, but that seems like overkill.

          // If already on work queue, (1) is violated: bail out.
          if (!AddQ(&workq, ip->out())) {
            if (ExtraDebug)
              ABSL_LOG(ERROR) << absl::StrFormat(
                  "Not OnePass: multiple paths %d -> %d", *it, ip->out());
            goto fail;
          }
          id = ip->out();
          goto Loop;

        case kInstMatch:
          if (matched) {
            // (3) is violated
            if (ExtraDebug)
              ABSL_LOG(ERROR) << absl::StrFormat(
                  "Not OnePass: multiple matches from %d", *it);
            goto fail;
          }
          matched = true;
          node->matchcond = cond;

          if (ip->last())
            break;
          // If already on work queue, (1) is violated: bail out.
          if (!AddQ(&workq, id+1))
            goto fail;
          id = id+1;
          goto Loop;

        case kInstFail:
          break;
      }
    }
  }

  if (ExtraDebug) {  // For debugging, dump one-pass NFA to ABSL_LOG(ERROR).
    ABSL_LOG(ERROR) << "bytemap:\n" << DumpByteMap();
    ABSL_LOG(ERROR) << "prog:\n" << Dump();

    std::map<int, int> idmap;
    for (int i = 0; i < size; i++)
      if (nodebyid[i] != -1)
        idmap[nodebyid[i]] = i;

    std::string dump;
    for (Instq::iterator it = tovisit.begin(); it != tovisit.end(); ++it) {
      int id = *it;
      int nodeindex = nodebyid[id];
      if (nodeindex == -1)
        continue;
      OneState* node = IndexToNode(nodes.data(), statesize, nodeindex);
      dump += absl::StrFormat("node %d id=%d: matchcond=%#x\n",
                              nodeindex, id, node->matchcond);
      for (int i = 0; i < bytemap_range_; i++) {
        if ((node->action[i] & kImpossible) == kImpossible)
          continue;
        dump += absl::StrFormat("  %d cond %#x -> %d id=%d\n",
                                i, node->action[i] & 0xFFFF,
                                node->action[i] >> kIndexShift,
                                idmap[node->action[i] >> kIndexShift]);
      }
    }
    ABSL_LOG(ERROR) << "nodes:\n" << dump;
  }

  {
    if ((used_cond & kEmptyAllFlags) == 0) {
      uint8_t action_shift = 9;
      if ((used_cond & kCapMask) == 0) {
        action_shift = 1;
      } else if ((used_cond >> 13) == 0) {
        action_shift = 7;
      }
      if (nalloc <= (kInvalidAction16 >> action_shift)) {
        onepass_action_shift_ = action_shift;
        onepass_cond_shift_ = kEmptyShift;
        const int total_bytes = nalloc * statesize16;
        if (out != NULL && total_bytes <= max_out_bytes) {
          *out = PODArray<uint8_t>(total_bytes);
          CopyTo16(nodes.data(), statesize, out->data(),
                   statesize16, nalloc, bytemap_range_,
                   [action_shift](uint32_t act) -> uint16_t {
                     if ((act & kImpossible) == kImpossible)
                       return kInvalidAction16;
                     uint32_t cond = (act & 0xFFFF) >> kEmptyShift;
                     return static_cast<uint16_t>(
                         ((act >> kIndexShift) << action_shift) | cond);
                   });
        }
        return total_bytes;
      }
    } else if ((used_cond & kCapMask) == 0 && nalloc <= (1 << (16 - 7))) {
      onepass_action_shift_ = 7;
      onepass_cond_shift_ = 0;
      const int total_bytes = nalloc * statesize16;
      if (out != NULL && total_bytes <= max_out_bytes) {
        *out = PODArray<uint8_t>(total_bytes);
        CopyTo16(nodes.data(), statesize, out->data(), statesize16,
                 nalloc, bytemap_range_, [](uint32_t act) -> uint16_t {
                   if ((act & kImpossible) == kImpossible)
                     return kImpossible;
                   return static_cast<uint16_t>(((act >> kIndexShift) << 7) |
                                                (act & 0x7F));
                 });
      }
      return total_bytes;
    }

    // Palette mode for large regexes with multiple captures or empty flags:
    // encode up to 64 distinct 16-bit condition masks in a compact palette.
    if (palette_ok) {
      uint8_t palette_shift = (nconds <= 16) ? 4 : 6;
      if (nalloc <= (1 << (16 - palette_shift))) {
        onepass_action_shift_ = palette_shift;
        onepass_cond_shift_ = kPaletteCondShift;
        const int palette_bytes = (1 << palette_shift) * sizeof(uint16_t);
        const int total_bytes = palette_bytes + nalloc * statesize16;
        if (out != NULL && total_bytes <= max_out_bytes) {
          *out = PODArray<uint8_t>(total_bytes);
          memset(out->data(), 0, palette_bytes);
          memcpy(out->data(), palette, nconds * sizeof(uint16_t));
          CopyTo16(nodes.data(), statesize,
                   out->data() + palette_bytes, statesize16, nalloc,
                   bytemap_range_,
                   [&palette, palette_shift](uint32_t act) -> uint16_t {
                     if ((act & kImpossible) == kImpossible)
                       return 0;
                     uint16_t c = static_cast<uint16_t>(act & 0xFFFF);
                     int idx = 1;
                     if (c != 0) {
                       idx = 2;
                       while (palette[idx] != c)
                         idx++;
                     }
                     return static_cast<uint16_t>(
                         ((act >> kIndexShift) << palette_shift) | idx);
                   });
        }
        return total_bytes;
      }
    }
  }

  onepass_action_shift_ = kIndexShift;
  onepass_cond_shift_ = 0;
  if (out != NULL && nalloc*statesize <= max_out_bytes) {
    *out = PODArray<uint8_t>(nalloc*statesize);
    memmove(out->data(), nodes.data(), nalloc*statesize);
  }
  return nalloc*statesize;

fail:
  return -1;
}

}  // namespace re2
