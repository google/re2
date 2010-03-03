// Copyright 2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Compiled regular expression representation.
// Tested by compile_unittest.cc

#include "util/util.h"
#include "util/sparse_array.h"
#include "re2/prog.h"
#include "re2/stringpiece.h"

namespace re2 {

// Constructors per Inst opcode

void Inst::InitAlt(Inst* out, Inst* out1) {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstAlt;
  out_ = out;
  out1_ = out1;
}

void Inst::InitByteRange(int lo, int hi, int foldcase, Inst* out) {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstByteRange;
  lo_ = lo & 0xFF;
  hi_ = hi & 0xFF;
  foldcase_ = foldcase;
  out_ = out;
}

void Inst::InitCapture(int cap, Inst* out) {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstCapture;
  cap_ = cap;
  out_ = out;
}

void Inst::InitEmptyWidth(EmptyOp empty, Inst* out) {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstEmptyWidth;
  empty_ = empty;
  out_ = out;
}

void Inst::InitMatch() {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstMatch;
}

void Inst::InitNop(Inst* out) {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstNop;
  out_ = out;
}

void Inst::InitFail() {
  DCHECK_EQ(opcode_, 0);
  opcode_ = kInstFail;
}

string Inst::Dump() {
  switch (opcode_) {
    default:
      return StringPrintf("opcode %d", static_cast<int>(opcode_));

    case kInstAlt:
      return StringPrintf("alt -> %d | %d", out_->id(), out1_->id());

    case kInstAltMatch:
      return StringPrintf("altmatch -> %d | %d", out_->id(), out1_->id());

    case kInstByteRange:
      return StringPrintf("byte%s [%02x-%02x] -> %d",
                          foldcase_ ? "/i" : "",
                          lo_, hi_, out_->id());

    case kInstCapture:
      return StringPrintf("capture %d -> %d", cap_, out_->id());

    case kInstEmptyWidth:
      return StringPrintf("emptywidth %#x -> %d",
                          static_cast<int>(empty_), out_->id());

    case kInstMatch:
      return StringPrintf("match!");

    case kInstNop:
      return StringPrintf("nop -> %d", out_->id());

    case kInstFail:
      return StringPrintf("fail");
  }
}

Prog::Prog()
  : anchor_start_(false),
    anchor_end_(false),
    start_(NULL),
    size_(0),
    byte_inst_count_(0),
    arena_(40*sizeof(Inst)),        // about 1kB on 64-bit
    dfa_first_(NULL),
    dfa_longest_(NULL),
    dfa_mem_(0),
    delete_dfa_(NULL),
    bytemap_range_(0),
    flags_(0),
    did_onepass_(false),
    onepass_nodes_(NULL),
    onepass_start_(NULL) {
}

Prog::~Prog() {
  if (delete_dfa_) {
    if (dfa_first_)
      delete_dfa_(dfa_first_);
    if (dfa_longest_)
      delete_dfa_(dfa_longest_);
  }
  delete[] onepass_nodes_;
}

Inst* Prog::AllocInst() {
  return new (AllocateInArena, &arena_) Inst(size_++);
}

typedef SparseArray<Inst*> Workq;

static inline void AddToQueue(Workq* q, Inst* ip) {
  if (ip != NULL)
    q->set(ip->id(), ip);
}

static string ProgToString(Workq *q) {
  string s;

  for (Workq::iterator i = q->begin(); i != q->end(); ++i) {
    Inst* ip = i->value();
    StringAppendF(&s, "%d. %s\n", ip->id(), ip->Dump().c_str());
    AddToQueue(q, ip->out());
    if (ip->opcode() == kInstAlt || ip->opcode() == kInstAltMatch)
      AddToQueue(q, ip->out1());
  }
  return s;
}

string Prog::Dump() {
  string map;
  if (false) {  // Debugging
    int lo = 0;
    StringAppendF(&map, "byte map:\n");
    for (int i = 0; i < bytemap_range_; i++) {
      StringAppendF(&map, "\t%d. [%02x-%02x]\n", i, lo, unbytemap_[i]);
      lo = unbytemap_[i] + 1;
    }
    StringAppendF(&map, "\n");
  }

  Workq q(size_);
  AddToQueue(&q, start_);
  return map + ProgToString(&q);
}

string Prog::DumpUnanchored() {
  Workq q(size_);
  AddToQueue(&q, start_unanchored_);
  return ProgToString(&q);
}

static bool IsMatch(Inst*);

// Peep-hole optimizer.
void Prog::Optimize() {
  Workq q(size_);

  // Eliminate nops.  Most are taken out during compilation
  // but a few are hard to avoid.
  q.clear();
  AddToQueue(&q, start_);
  for (Workq::iterator i = q.begin(); i != q.end(); ++i) {
    Inst* ip = i->value();

    Inst* j = ip->out();
    while (j != NULL && j->opcode() == kInstNop) {
      j = j->out();
    }
    ip->out_ = j;
    AddToQueue(&q, ip->out());

    if (ip->opcode() == kInstAlt) {
      j = ip->out1();
      while (j != NULL && j->opcode() == kInstNop) {
        j = j->out();
      }
      ip->out1_ = j;
      AddToQueue(&q, ip->out1());
    }
  }

  // Insert kInstAltMatch instructions
  // Look for
  //   ip: Alt -> j | k
  //	  j: ByteRange [00-FF] -> ip
  //    k: Match
  // or the reverse (the above is the greedy one).
  // Rewrite Alt to AltMatch.
  q.clear();
  AddToQueue(&q, start_);
  for (Workq::iterator i = q.begin(); i != q.end(); ++i) {
    Inst* ip = i->value();
    AddToQueue(&q, ip->out());
    if (ip->opcode() == kInstAlt)
      AddToQueue(&q, ip->out1());

    if (ip->opcode() == kInstAlt) {
      Inst* j = ip->out();
      Inst* k = ip->out1();
      if (j->opcode() == kInstByteRange && j->out() == ip &&
          j->lo() == 0x00 && j->hi() == 0xFF &&
          IsMatch(k)) {
        ip->opcode_ = kInstAltMatch;
        continue;
      }
      if (IsMatch(j) &&
          k->opcode() == kInstByteRange && k->out() == ip &&
          k->lo() == 0x00 && k->hi() == 0xFF) {
        ip->opcode_ = kInstAltMatch;
      }
    }
  }
}

// Is ip a guaranteed match at end of text, perhaps after some capturing?
static bool IsMatch(Inst *ip) {
  for (;;) {
    switch (ip->opcode()) {
      default:
        LOG(DFATAL) << "Unexpected opcode in IsMatch: " << ip->opcode();
        return false;

      case kInstAlt:
      case kInstAltMatch:
      case kInstByteRange:
      case kInstFail:
      case kInstEmptyWidth:
        return false;

      case kInstCapture:
      case kInstNop:
        ip = ip->out();
        break;

      case kInstMatch:
        return true;
    }
  }
}

uint32 Prog::EmptyFlags(const StringPiece& text, const char* p) {
  int flags = 0;

  // ^ and \A
  if (p == text.begin())
    flags |= kEmptyBeginText | kEmptyBeginLine;
  else if (p[-1] == '\n')
    flags |= kEmptyBeginLine;

  // $ and \z
  if (p == text.end())
    flags |= kEmptyEndText | kEmptyEndLine;
  else if (p < text.end() && p[0] == '\n')
    flags |= kEmptyEndLine;

  // \b and \B
  if (p == text.begin() && p == text.end()) {
    // no word boundary here
  } else if (p == text.begin()) {
    if (IsWordChar(p[0]))
      flags |= kEmptyWordBoundary;
  } else if (p == text.end()) {
    if (IsWordChar(p[-1]))
      flags |= kEmptyWordBoundary;
  } else {
    if (IsWordChar(p[-1]) != IsWordChar(p[0]))
      flags |= kEmptyWordBoundary;
  }
  if (!(flags & kEmptyWordBoundary))
    flags |= kEmptyNonWordBoundary;

  return flags;
}

void Prog::MarkByteRange(int lo, int hi) {
  CHECK_GE(lo, 0);
  CHECK_GE(hi, 0);
  CHECK_LE(lo, 255);
  CHECK_LE(hi, 255);
  if (lo > 0)
    byterange_.Set(lo - 1);
  byterange_.Set(hi);
}

void Prog::ComputeByteMap() {
  // Fill in bytemap with byte classes for prog_.
  // Ranges of bytes that are treated as indistinguishable
  // by the regexp program are mapped to a single byte class.
  // The vector prog_->byterange() marks the end of each
  // such range.
  const Bitmap<256>& v = byterange();

  COMPILE_ASSERT(8*sizeof(v.Word(0)) == 32, wordsize);
  uint8 n = 0;
  uint32 bits = 0;
  for (int i = 0; i < 256; i++) {
    if ((i&31) == 0)
      bits = v.Word(i >> 5);
    bytemap_[i] = n;
    unbytemap_[n] = i;
    n += bits & 1;
    bits >>= 1;
  }
  bytemap_range_ = bytemap_[255] + 1;

  if (0) {  // For debugging: use trivial byte map.
    for (int i = 0; i < 256; i++) {
      bytemap_[i] = i;
      unbytemap_[i] = i;
    }
    bytemap_range_ = 256;
    LOG(INFO) << "Using trivial bytemap.";
  }
}

}  // namespace re2

