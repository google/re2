// Copyright 2006 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Regular expression representation.
// Tested by parse_unittest.cc

#include "util/util.h"
#include "re2/regexp.h"
#include "re2/stringpiece.h"
#include "re2/walker-inl.h"

namespace re2 {

// Constructor.  Allocates vectors as appropriate for operator.
Regexp::Regexp(RegexpOp op, ParseFlags parse_flags)
  : op_(op),
    parse_flags_(parse_flags),
    ref_(1),
    simple_(false),
    num_captures_(-1),
    cap_(-1),
    max_(0),
    min_(0),
    nrunes_(0),
    nsub_(0),
    rune_(0),
    cc_(NULL),
    sub_(NULL),
    runes_(NULL),
    name_(NULL),
    down_(NULL) {

  switch (op_) {
    default:
      break;
    case kRegexpCharClass:
      cc_ = new CharClass;
      break;
  }
}

// Destructor.  Assumes already cleaned up children.
// Private: use Decref() instead of delete to destroy Regexps.
// Can't call Decref on the sub-Regexps here because
// that could cause arbitrarily deep recursion, so
// required Decref() to have handled them for us.
Regexp::~Regexp() {
  if (sub_)
    LOG(DFATAL) << "Regexp not destroyed.";

  delete cc_;
  delete name_;
  delete[] runes_;
}

// If it's possible to destroy this regexp without recurring,
// do so and return true.  Else return false.
bool Regexp::QuickDestroy() {
  if (sub_ == NULL) {
    delete this;
    return true;
  }
  return false;
}

// Deletes this object; ref count has count reached 0.
void Regexp::Destroy() {
  if (ref_ < 0)
    LOG(DFATAL) << "Bad reference count " << ref_;

  if (QuickDestroy())
    return;

  // Handle recursive Destroy with explicit stack
  // to avoid arbitrarily deep recursion on process stack [sigh].
  down_ = NULL;
  Regexp* stack = this;
  while (stack != NULL) {
    Regexp* re = stack;
    stack = re->down_;
    if (re->ref_ != 0)
      LOG(DFATAL) << "Bad reference count " << re->ref_;
    if (re->sub_ != NULL) {
      for (int i = 0; i < re->nsub_; i++) {
        Regexp* sub = re->sub_[i];
        if (--sub->ref_ == 0 && !sub->QuickDestroy()) {
          sub->down_ = stack;
          stack = sub;
        }
      }
      if (re->sub_ != re->smallsub_)
        delete[] re->sub_;
      re->sub_ = NULL;
    }
    delete re;
  }
}

void Regexp::AddRuneToString(Rune r) {
  DCHECK(op_ == kRegexpLiteralString);
  if (nrunes_ == 0) {
    // start with 8
    runes_ = new Rune[8];
  } else if (nrunes_ >= 8 && (nrunes_ & (nrunes_ - 1)) == 0) {
    // double on powers of two
    Rune *old = runes_;
    runes_ = new Rune[nrunes_ * 2];
    for (int i = 0; i < nrunes_; i++)
      runes_[i] = old[i];
    delete[] old;
  }

  runes_[nrunes_++] = r;
}


Regexp* Regexp::Plus(Regexp* sub, Regexp::ParseFlags flags) {
  if (sub->op() == kRegexpPlus && sub->parse_flags() == flags)
    return sub;
  Regexp* re = new Regexp(kRegexpPlus, flags);
  re->AllocSub(1);
  re->sub_[0] = sub;
  return re;
}

Regexp* Regexp::Star(Regexp* sub, Regexp::ParseFlags flags) {
  if (sub->op() == kRegexpStar && sub->parse_flags() == flags)
    return sub;
  Regexp* re = new Regexp(kRegexpStar, flags);
  re->AllocSub(1);
  re->sub_[0] = sub;
  return re;
}

Regexp* Regexp::Quest(Regexp* sub, Regexp::ParseFlags flags) {
  if (sub->op() == kRegexpQuest && sub->parse_flags() == flags)
    return sub;
  Regexp* re = new Regexp(kRegexpQuest, flags);
  re->AllocSub(1);
  re->sub_[0] = sub;
  return re;
}

Regexp* Regexp::Concat(Regexp** sub, int nsub, ParseFlags flags) {
  Regexp* re = new Regexp(kRegexpConcat, flags);
  re->AllocSub(nsub);
  for (int i = 0; i < nsub; i++)
    re->sub_[i] = sub[i];
  return re;
}

Regexp* Regexp::Alternate(Regexp** sub, int nsub, ParseFlags flags) {
  Regexp* re = new Regexp(kRegexpAlternate, flags);
  re->AllocSub(nsub);
  for (int i = 0; i < nsub; i++)
    re->sub_[i] = sub[i];
  return re;
}

Regexp* Regexp::Capture(Regexp* sub, ParseFlags flags, int cap) {
  Regexp* re = new Regexp(kRegexpCapture, flags);
  re->AllocSub(1);
  re->sub_[0] = sub;
  re->cap_ = cap;
  return re;
}

Regexp* Regexp::Repeat(Regexp* sub, ParseFlags flags, int min, int max) {
  Regexp* re = new Regexp(kRegexpRepeat, flags);
  re->AllocSub(1);
  re->sub_[0] = sub;
  re->min_ = min;
  re->max_ = max;
  return re;
}

Regexp* Regexp::NewLiteral(Rune rune, ParseFlags flags) {
  Regexp* re = new Regexp(kRegexpLiteral, flags);
  re->rune_ = rune;
  return re;
}

Regexp* Regexp::LiteralString(Rune* runes, int nrunes, ParseFlags flags) {
  if (nrunes <= 0)
    return new Regexp(kRegexpEmptyMatch, flags);
  if (nrunes == 1)
    return NewLiteral(runes[0], flags);
  Regexp* re = new Regexp(kRegexpLiteralString, flags);
  for (int i = 0; i < nrunes; i++)
    re->AddRuneToString(runes[i]);
  return re;
}

Regexp* Regexp::NewCharClass(CharClass* cc, ParseFlags flags) {
  Regexp* re = new Regexp(kRegexpCharClass, flags);
  delete re->cc_;
  re->cc_ = cc;
  return re;
}


// Keep in sync with enum RegexpStatusCode in regexp.h
static const string kErrorStrings[] = {
  "no error",
  "unexpected error",
  "invalid escape sequence",
  "invalid character class",
  "invalid character class range",
  "missing ]",
  "missing )",
  "trailing \\",
  "no argument for repetition operator",
  "invalid repetition size",
  "bad repetition operator",
  "invalid perl operator",
  "invalid UTF-8",
  "invalid named capture group",
};

const string& RegexpStatus::CodeText(enum RegexpStatusCode code) {
  if (code < 0 || code >= arraysize(kErrorStrings))
    code = kRegexpInternalError;
  return kErrorStrings[code];
}

string RegexpStatus::Text() const {
  if (error_arg_.empty())
    return CodeText(code_);
  string s;
  s.append(CodeText(code_));
  s.append(": ");
  s.append(error_arg_.data(), error_arg_.size());
  return s;
}

void RegexpStatus::Copy(const RegexpStatus& status) {
  code_ = status.code_;
  error_arg_ = status.error_arg_;
}

typedef int Ignored;  // Walker<void> doesn't exist

// Walker subclass to count capturing parens in regexp.
class NumCapturesWalker : public Regexp::Walker<Ignored> {
 public:
  NumCapturesWalker() : ncapture_(0) {}
  int ncapture() { return ncapture_; }

  virtual Ignored PreVisit(Regexp* re, Ignored ignored, bool* stop) {
    if (re->op() == kRegexpCapture)
      ncapture_++;
    return ignored;
  }
  virtual Ignored ShortVisit(Regexp* re, Ignored ignored) {
    // Should never be called: we use Walk not WalkExponential.
    LOG(DFATAL) << "NumCapturesWalker::ShortVisit called";
    return ignored;
  }

 private:
  int ncapture_;
  DISALLOW_EVIL_CONSTRUCTORS(NumCapturesWalker);
};

int Regexp::NumCaptures() {
  ANNOTATE_BENIGN_RACE(&num_captures_, "benign race: in the worst case"
    " multiple threads end up doing the same work in parallel.");
  if (num_captures_ == -1) {
    NumCapturesWalker w;
    w.Walk(this, 0);
    num_captures_ = w.ncapture();
  }
  return num_captures_;
}

// Walker class to build map of named capture groups and their indices.
class NamedCapturesWalker : public Regexp::Walker<Ignored> {
 public:
  NamedCapturesWalker() : map_(NULL) {}
  ~NamedCapturesWalker() { delete map_; }

  map<string, int>* TakeMap() {
    map<string, int>* m = map_;
    map_ = NULL;
    return m;
  }

  Ignored PreVisit(Regexp* re, Ignored ignored, bool* stop) {
    if (re->op() == kRegexpCapture && re->name() != NULL) {
      // Allocate map once we find a name.
      if (map_ == NULL)
        map_ = new map<string, int>;

      // Record first occurrence of each name.
      // (The rule is that if you have the same name
      // multiple times, only the leftmost one counts.)
      if (map_->find(*re->name()) == map_->end())
        (*map_)[*re->name()] = re->cap();
    }
    return ignored;
  }

  virtual Ignored ShortVisit(Regexp* re, Ignored ignored) {
    // Should never be called: we use Walk not WalkExponential.
    LOG(DFATAL) << "NamedCapturesWalker::ShortVisit called";
    return ignored;
  }

 private:
  map<string, int>* map_;
  DISALLOW_EVIL_CONSTRUCTORS(NamedCapturesWalker);
};

map<string, int>* Regexp::NamedCaptures() {
  NamedCapturesWalker w;
  w.Walk(this, 0);
  return w.TakeMap();
}

// Determines whether regexp matches must be anchored
// with a fixed string prefix.  If so, returns the prefix and
// the regexp that remains after the prefix.  The prefix might
// be ASCII case-insensitive.
bool Regexp::RequiredPrefix(string *prefix, bool *foldcase, Regexp** suffix) {
  // No need for a walker: the regexp must be of the form
  // 1. some number of ^ anchors
  // 2. a literal char or string
  // 3. the rest
  prefix->clear();
  *foldcase = false;
  *suffix = NULL;
  if (op_ != kRegexpConcat)
    return false;

  // Some number of anchors.
  int i = 0;
  while (i < nsub_ && sub_[i]->op_ == kRegexpBeginText)
    i++;
  if (i == 0)
    return false;

  // Then a literal or a concatenation.
  if (i < nsub_) {
    Regexp* re = sub_[i];
    switch (re->op_) {
      default:
        return false;

      case kRegexpLiteralString:
        // Convert to string in proper encoding.
        if (re->parse_flags_ & Latin1) {
          prefix->resize(re->nrunes_);
          for (int j = 0; j < re->nrunes_; j++)
            (*prefix)[j] = re->runes_[j];
        } else {
          // Convert to UTF-8 in place.
          // Assume worst-case space and then trim.
          prefix->resize(re->nrunes_ * UTFmax);
          char *p = &(*prefix)[0];
          for (int j = 0; j < re->nrunes_; j++) {
            Rune r = re->runes_[j];
            if (r < Runeself)
              *p++ = r;
            else
              p += runetochar(p, &r);
          }
          prefix->resize(p - &(*prefix)[0]);
        }
        break;

      case kRegexpLiteral:
        if ((re->parse_flags_ & Latin1) || re->rune_ < Runeself) {
          prefix->append(1, re->rune_);
        } else {
          char buf[UTFmax];
          prefix->append(buf, runetochar(buf, &re->rune_));
        }
        break;
    }
    *foldcase = (sub_[i]->parse_flags_ & FoldCase);
    i++;
  }

  // The rest.
  Regexp* re;
  if (i < nsub_) {
    for (int j = i; j < nsub_; j++)
      sub_[j]->Incref();
    re = Concat(sub_ + i, nsub_ - i, parse_flags_);
  } else {
    re = new Regexp(kRegexpEmptyMatch, parse_flags_);
  }
  *suffix = re;
  return true;
}

// Character class is a balanced binary tree (STL set)
// containing non-overlapping, non-abutting RuneRanges.
// The less-than operator used in the tree treats two
// ranges as equal if they overlap at all, so that
// lookups for a particular Rune are possible.

CharClass::CharClass() {
  nrunes_ = 0;
  upper_ = 0;
  lower_ = 0;
}

// Add lo-hi to the class; return whether class got bigger.
bool CharClass::AddRange(Rune lo, Rune hi) {
  if (hi < lo)
    return false;

  if (lo <= 'z' && hi >= 'A') {
    // Overlaps some alpha, maybe not all.
    // Update bitmaps telling which ASCII letters are in the set.
    Rune lo1 = max<Rune>(lo, 'A');
    Rune hi1 = min<Rune>(hi, 'Z');
    if (lo1 <= hi1)
      upper_ |= ((1 << (hi1 - lo1 + 1)) - 1) << (lo1 - 'A');

    lo1 = max<Rune>(lo, 'a');
    hi1 = min<Rune>(hi, 'z');
    if (lo1 <= hi1)
      lower_ |= ((1 << (hi1 - lo1 + 1)) - 1) << (lo1 - 'a');
  }

  {  // Check whether lo, hi is already in the class.
    iterator it = ranges_.find(RuneRange(lo, lo));
    if (it != end() && it->lo <= lo && hi <= it->hi)
      return false;
  }

  // Look for a range abutting lo on the left.
  // If it exists, take it out and increase our range.
  if (lo > 0) {
    iterator it = ranges_.find(RuneRange(lo-1, lo-1));
    if (it != end()) {
      lo = it->lo;
      if (it->hi > hi)
        hi = it->hi;
      nrunes_ -= it->hi - it->lo + 1;
      ranges_.erase(it);
    }
  }

  // Look for a range abutting hi on the right.
  // If it exists, take it out and increase our range.
  if (hi < Runemax) {
    iterator it = ranges_.find(RuneRange(hi+1, hi+1));
    if (it != end()) {
      hi = it->hi;
      nrunes_ -= it->hi - it->lo + 1;
      ranges_.erase(it);
    }
  }

  // Look for ranges between lo and hi.  Take them out.
  // This is only safe because the set has no overlapping ranges.
  // We've already removed any ranges abutting lo and hi, so
  // any that overlap [lo, hi] must be contained within it.
  for (;;) {
    iterator it = ranges_.find(RuneRange(lo, hi));
    if (it == end())
      break;
    nrunes_ -= it->hi - it->lo + 1;
    ranges_.erase(it);
  }

  // Finally, add [lo, hi].
  nrunes_ += hi - lo + 1;
  ranges_.insert(RuneRange(lo, hi));
  return true;
}

bool CharClass::AddCharClass(CharClass *cc) {
  bool added = false;
  for (iterator it = cc->begin(); it != cc->end(); ++it)
    added |= AddRange(it->lo, it->hi);
  return added;
}

bool CharClass::Contains(Rune r) {
  return ranges_.find(RuneRange(r, r)) != end();
}

// Does the character class behave the same on A-Z as on a-z?
bool CharClass::FoldsASCII() {
  return ((upper_ ^ lower_) & AlphaMask) == 0;
}

CharClass* CharClass::Copy() {
  CharClass* cc = new CharClass;
  for (iterator it = begin(); it != end(); ++it)
    cc->ranges_.insert(RuneRange(it->lo, it->hi));
  cc->upper_ = upper_;
  cc->lower_ = lower_;
  cc->nrunes_ = nrunes_;
  return cc;
}

void CharClass::RemoveAbove(Rune r) {
  if (r >= Runemax)
    return;

  if (r < 'z') {
    if (r < 'a')
      lower_ = 0;
    else
      lower_ &= AlphaMask >> ('z' - r);
  }

  if (r < 'Z') {
    if (r < 'A')
      upper_ = 0;
    else
      upper_ &= AlphaMask >> ('Z' - r);
  }

  for (;;) {
    iterator it = ranges_.find(RuneRange(r + 1, Runemax));
    if (it == end())
      break;
    RuneRange rr = *it;
    ranges_.erase(it);
    nrunes_ -= rr.hi - rr.lo + 1;
    if (rr.lo <= r) {
      rr.hi = r;
      ranges_.insert(rr);
      nrunes_ += rr.hi - rr.lo + 1;
    }
  }
}

void CharClass::Negate() {
  // Build up negation and then copy in.
  // Could edit ranges in place, but C++ won't let me.
  vector<RuneRange> v;
  v.reserve(ranges_.size() + 1);

  // In negation, first range begins at 0, unless
  // the current class begins at 0.
  iterator it = begin();
  if (it == end()) {
    v.push_back(RuneRange(0, Runemax));
  } else {
    int nextlo = 0;
    if (it->lo == 0) {
      nextlo = it->hi + 1;
      ++it;
    }
    for (; it != end(); ++it) {
      v.push_back(RuneRange(nextlo, it->lo - 1));
      nextlo = it->hi + 1;
    }
    if (nextlo <= Runemax)
      v.push_back(RuneRange(nextlo, Runemax));
  }

  ranges_.clear();
  for (int i = 0; i < v.size(); i++)
    ranges_.insert(v[i]);

  upper_ = AlphaMask & ~upper_;
  lower_ = AlphaMask & ~lower_;
  nrunes_ = Runemax+1 - nrunes_;
}

}  // namespace re2
