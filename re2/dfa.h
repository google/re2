// Copyright 2015 The RE2 Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_DFA_H
#define RE2_DFA_H

#include "re2/prog.h"
#include "re2/stringpiece.h"
#include "util/atomicops.h"
#include "util/flags.h"
#include "util/sparse_set.h"

namespace re2 {

// A DFA implementation of a regular expression program.
// Since this is entirely a forward declaration mandated by C++,
// some of the comments here are better understood after reading
// the comments in the sections that follow the DFA definition.

// This declaration exists in this header file only because the
// following declaration of StreamDFAContext must and because
// C++ requries this in order to declare DFA::StateSaver and 
// DFA::RWLocker, etc, members of StreamDFAContext.

class DFA {
 public:
  DFA(Prog* prog, Prog::MatchKind kind, int64 max_mem);
  ~DFA();
  bool ok() const { return !init_failed_; }
  Prog::MatchKind kind() { return kind_; }

  // Searches for the regular expression in text, which is considered
  // as a subsection of context for the purposes of interpreting flags
  // like ^ and $ and \A and \z.
  // Returns whether a match was found.
  // If a match is found, sets *ep to the end point of the best match in text.
  // If "anchored", the match must begin at the start of text.
  // If "want_earliest_match", the match that ends first is used, not
  //   necessarily the best one.
  // If "run_forward" is true, the DFA runs from text.begin() to text.end().
  //   If it is false, the DFA runs from text.end() to text.begin(),
  //   returning the leftmost end of the match instead of the rightmost one.
  // If the DFA cannot complete the search (for example, if it is out of
  //   memory), it sets *failed and returns false.
  bool Search(const StringPiece& text, const StringPiece& context,
              bool anchored, bool want_earliest_match, bool run_forward,
              bool* failed, const char** ep, vector<int>* matches);

  // Builds out all states for the entire DFA.  FOR TESTING ONLY
  // Returns number of states.
  int BuildAllStates();

  // Computes min and max for matching strings.  Won't return strings
  // bigger than maxlen.
  bool PossibleMatchRange(string* min, string* max, int maxlen);

  // These data structures are logically private, but C++ makes it too
  // difficult to mark them as such.
  class Workq;
  class RWLocker;
  class StateSaver;

  // A single DFA state.  The DFA is represented as a graph of these
  // States, linked by the next_ pointers.  If in state s and reading
  // byte c, the next state should be s->next_[c].
  struct State {
    inline bool IsMatch() const { return flag_ & kFlagMatch; }
    void SaveMatch(vector<int>* v);

    int* inst_;         // Instruction pointers in the state.
    int ninst_;         // # of inst_ pointers.
    uint flag_;         // Empty string bitfield flags in effect on the way
                        // into this state, along with kFlagMatch if this
                        // is a matching state.
    State** next_;      // Outgoing arrows from State,
                        // one per input byte class
  };

  enum {
    kByteEndText = 256,         // imaginary byte at end of text

    kFlagEmptyMask = 0xFFF,     // State.flag_: bits holding kEmptyXXX flags
    kFlagMatch = 0x1000,        // State.flag_: this is a matching state
    kFlagLastWord = 0x2000,     // State.flag_: last byte was a word char
    kFlagNeedShift = 16,        // needed kEmpty bits are or'ed in shifted left
  };

#ifndef STL_MSVC
  // STL function structures for use with unordered_set.
  struct StateEqual {
    bool operator()(const State* a, const State* b) const {
      if (a == b)
        return true;
      if (a == NULL || b == NULL)
        return false;
      if (a->ninst_ != b->ninst_)
        return false;
      if (a->flag_ != b->flag_)
        return false;
      for (int i = 0; i < a->ninst_; i++)
        if (a->inst_[i] != b->inst_[i])
          return false;
      return true;  // they're equal
    }
  };
#endif  // STL_MSVC
  struct StateHash {
    size_t operator()(const State* a) const {
      if (a == NULL)
        return 0;
      const char* s = reinterpret_cast<const char*>(a->inst_);
      int len = a->ninst_ * sizeof a->inst_[0];
      if (sizeof(size_t) == sizeof(uint32))
        return Hash32StringWithSeed(s, len, a->flag_);
      else
        return static_cast<size_t>(Hash64StringWithSeed(s, len, a->flag_));
    }
#ifdef STL_MSVC
    // Less than operator.
    bool operator()(const State* a, const State* b) const {
      if (a == b)
        return false;
      if (a == NULL || b == NULL)
        return a == NULL;
      if (a->ninst_ != b->ninst_)
        return a->ninst_ < b->ninst_;
      if (a->flag_ != b->flag_)
        return a->flag_ < b->flag_;
      for (int i = 0; i < a->ninst_; ++i)
        if (a->inst_[i] != b->inst_[i])
          return a->inst_[i] < b->inst_[i];
      return false;  // they're equal
    }
    // The two public members are required by msvc. 4 and 8 are default values.
    // Reference: http://msdn.microsoft.com/en-us/library/1s1byw77.aspx
    static const size_t bucket_size = 4;
    static const size_t min_buckets = 8;
#endif  // STL_MSVC
  };

#ifdef STL_MSVC
  typedef unordered_set<State*, StateHash> StateSet;
#else  // !STL_MSVC
  typedef unordered_set<State*, StateHash, StateEqual> StateSet;
#endif  // STL_MSVC


 private:
  // Special "firstbyte" values for a state.  (Values >= 0 denote actual bytes.)
  enum {
    kFbUnknown = -1,   // No analysis has been performed.
    kFbMany = -2,      // Many bytes will lead out of this state.
    kFbNone = -3,      // No bytes lead out of this state.
  };

  enum {
    // Indices into start_ for unanchored searches.
    // Add kStartAnchored for anchored searches.
    kStartBeginText = 0,          // text at beginning of context
    kStartBeginLine = 2,          // text at beginning of line
    kStartAfterWordChar = 4,      // text follows a word character
    kStartAfterNonWordChar = 6,   // text follows non-word character
    kMaxStart = 8,

    kStartAnchored = 1,
  };

  // Resets the DFA State cache, flushing all saved State* information.
  // Releases and reacquires cache_mutex_ via cache_lock, so any
  // State* existing before the call are not valid after the call.
  // Use a StateSaver to preserve important states across the call.
  // cache_mutex_.r <= L < mutex_
  // After: cache_mutex_.w <= L < mutex_
  void ResetCache(RWLocker* cache_lock);

  // Looks up and returns the State corresponding to a Workq.
  // L >= mutex_
  State* WorkqToCachedState(Workq* q, uint flag);

  // Looks up and returns a State matching the inst, ninst, and flag.
  // L >= mutex_
  State* CachedState(int* inst, int ninst, uint flag);

  // Clear the cache entirely.
  // Must hold cache_mutex_.w or be in destructor.
  void ClearCache();

  // Converts a State into a Workq: the opposite of WorkqToCachedState.
  // L >= mutex_
  static void StateToWorkq(State* s, Workq* q);

  // Runs a State on a given byte, returning the next state.
  State* RunStateOnByteUnlocked(State*, int);  // cache_mutex_.r <= L < mutex_
  State* RunStateOnByte(State*, int);          // L >= mutex_

  // Runs a Workq on a given byte followed by a set of empty-string flags,
  // producing a new Workq in nq.  If a match instruction is encountered,
  // sets *ismatch to true.
  // L >= mutex_
  void RunWorkqOnByte(Workq* q, Workq* nq,
                      int c, uint flag, bool* ismatch,
                      Prog::MatchKind kind);

  // Runs a Workq on a set of empty-string flags, producing a new Workq in nq.
  // L >= mutex_
  void RunWorkqOnEmptyString(Workq* q, Workq* nq, uint flag);

  // Adds the instruction id to the Workq, following empty arrows
  // according to flag.
  // L >= mutex_
  void AddToQueue(Workq* q, int id, uint flag);

  // For debugging, returns a text representation of State.
  static string DumpState(State* state);

  // For debugging, returns a text representation of a Workq.
  static string DumpWorkq(Workq* q);

  // Search parameters
  struct SearchParams {
    SearchParams(const StringPiece& text, const StringPiece& context,
                 RWLocker* cache_lock)
      : text(text), context(context),
        anchored(false),
        want_earliest_match(false),
        run_forward(false),
        start(NULL),
        firstbyte(kFbUnknown),
        cache_lock(cache_lock),
        failed(false),
        ep(NULL),
        matches(NULL) { }

    StringPiece text;
    StringPiece context;
    bool anchored;
    bool want_earliest_match;
    bool run_forward;
    State* start;
    int firstbyte;
    RWLocker *cache_lock;
    bool failed;     // "out" parameter: whether search gave up
    const char* ep;  // "out" parameter: end pointer for match
    vector<int>* matches;

   private:
    DISALLOW_COPY_AND_ASSIGN(SearchParams);
  };

  // Before each search, the parameters to Search are analyzed by
  // AnalyzeSearch to determine the state in which to start and the
  // "firstbyte" for that state, if any.
  struct StartInfo {
    StartInfo() : start(NULL), firstbyte(kFbUnknown) { }
    State* start;
    volatile int firstbyte;
  };

  // Fills in params->start and params->firstbyte using
  // the other search parameters.  Returns true on success,
  // false on failure.
  // cache_mutex_.r <= L < mutex_
  bool AnalyzeSearch(SearchParams* params);
  bool AnalyzeSearchHelper(SearchParams* params, StartInfo* info, uint flags);

  // The generic search loop, inlined to create specialized versions.
  // cache_mutex_.r <= L < mutex_
  // Might unlock and relock cache_mutex_ via params->cache_lock.
  inline bool InlinedSearchLoop(SearchParams* params,
                                bool have_firstbyte,
                                bool want_earliest_match,
                                bool run_forward);

  // The specialized versions of InlinedSearchLoop.  The three letters
  // at the ends of the name denote the true/false values used as the
  // last three parameters of InlinedSearchLoop.
  // cache_mutex_.r <= L < mutex_
  // Might unlock and relock cache_mutex_ via params->cache_lock.
  bool SearchFFF(SearchParams* params);
  bool SearchFFT(SearchParams* params);
  bool SearchFTF(SearchParams* params);
  bool SearchFTT(SearchParams* params);
  bool SearchTFF(SearchParams* params);
  bool SearchTFT(SearchParams* params);
  bool SearchTTF(SearchParams* params);
  bool SearchTTT(SearchParams* params);

  // The main search loop: calls an appropriate specialized version of
  // InlinedSearchLoop.
  // cache_mutex_.r <= L < mutex_
  // Might unlock and relock cache_mutex_ via params->cache_lock.
  bool FastSearchLoop(SearchParams* params);

  // For debugging, a slow search loop that calls InlinedSearchLoop
  // directly -- because the booleans passed are not constants, the
  // loop is not specialized like the SearchFFF etc. versions, so it
  // runs much more slowly.  Useful only for debugging.
  // cache_mutex_.r <= L < mutex_
  // Might unlock and relock cache_mutex_ via params->cache_lock.
  bool SlowSearchLoop(SearchParams* params);

  // Looks up bytes in bytemap_ but handles case c == kByteEndText too.
  int ByteMap(int c) {
    if (c == kByteEndText)
      return prog_->bytemap_range();
    return prog_->bytemap()[c];
  }

  // Constant after initialization.
  Prog* prog_;              // The regular expression program to run.
  Prog::MatchKind kind_;    // The kind of DFA.
  bool init_failed_;        // initialization failed (out of memory)

  Mutex mutex_;  // mutex_ >= cache_mutex_.r

  // Scratch areas, protected by mutex_.
  Workq* q0_;             // Two pre-allocated work queues.
  Workq* q1_;
  int* astack_;         // Pre-allocated stack for AddToQueue
  int nastack_;

  // State* cache.  Many threads use and add to the cache simultaneously,
  // holding cache_mutex_ for reading and mutex_ (above) when adding.
  // If the cache fills and needs to be discarded, the discarding is done
  // while holding cache_mutex_ for writing, to avoid interrupting other
  // readers.  Any State* pointers are only valid while cache_mutex_
  // is held.
  Mutex cache_mutex_;
  int64 mem_budget_;       // Total memory budget for all States.
  int64 state_budget_;     // Amount of memory remaining for new States.
  StateSet state_cache_;   // All States computed so far.
  StartInfo start_[kMaxStart];
  bool cache_warned_;      // have printed to LOG(INFO) about the cache

  friend class StreamDFAContext;
};

// Declaration of StreamDFAContext: should be created for every "search 
// context". Holds state of a search within a particular stream of input. 
// "master" DFA holds all cache state and determines state transitions.

class StreamDFAContext {
 public:
  StreamDFAContext(DFA* dfa, bool has_prefix, char end_prefix);
  ~StreamDFAContext();
  
  // Searches for the regular expression in text
  // Returns number of bytes of this chunk which cannot match, regardless of next chunk
  // Said another way, returns the number of bytes of the input which may be safely freed by the caller
  // If the DFA cannot complete the search (for example, if it is out of
  //   memory), it sets *failed and returns 0.
  // match_length is the length of the match, if matched
  int StreamSearch(const StringPiece& text, int len, bool* matched, int* match_length, bool* failed);

  // Same as StreamSearch but terminates trailing greedy operators such as * or +
  bool StreamSearchEOF(int* match_length, bool* failed);

  // Analagous to AnalyzeSearch functionality in DFA but with no concept of "context".
  bool AnalyzeSearch(char end_prefix);

  int BacklogByteCount() { return params->backlog_bytes; }
  void AddBacklogBytes(int b) { params->backlog_bytes += b; }
  bool CheckPrefixFailed() { return params->prefix_failed; }
  void SetPrefixFailed() { params->prefix_failed = true; }

  // Search parameters
  struct SearchParams {
    SearchParams()
      : matched(false),
        prefix_failed(false),
        start(NULL), 
        s0(NULL), 
        firstbyte(DFA::kFbUnknown),
        backlog_bytes(0),
        last_matched_offset(-1) { }

      ~SearchParams() { }

    bool matched;
    bool prefix_failed;
    DFA::State* start;
    DFA::StateSaver* s0;
    int firstbyte;
    int backlog_bytes; // partial match bytes
    int last_matched_offset;
   private:
    DISALLOW_COPY_AND_ASSIGN(SearchParams);
  };

 private: 

  // Read/Write the cache identically to DFA
  DFA::RWLocker *cache_lock;

  SearchParams *params;
  bool has_prefix_;
  DFA* master;
};

}

#endif

