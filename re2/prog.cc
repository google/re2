// Copyright 2007 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Compiled regular expression representation.
// Tested by compile_test.cc

#include "util/util.h"
#include "re2/prog.h"
#include "re2/stringpiece.h"

namespace re2 {

// Constructors per Inst opcode

void Prog::Inst::InitAlt(uint32 out, uint32 out1) {
  DCHECK_EQ(out_opcode_, 0);
  set_out_opcode(out, kInstAlt);
  out1_ = out1;
}

void Prog::Inst::InitByteRange(int lo, int hi, int foldcase, uint32 out) {
  DCHECK_EQ(out_opcode_, 0);
  set_out_opcode(out, kInstByteRange);
  lo_ = lo & 0xFF;
  hi_ = hi & 0xFF;
  foldcase_ = foldcase & 0xFF;
}

void Prog::Inst::InitCapture(int cap, uint32 out) {
  DCHECK_EQ(out_opcode_, 0);
  set_out_opcode(out, kInstCapture);
  cap_ = cap;
}

void Prog::Inst::InitEmptyWidth(EmptyOp empty, uint32 out) {
  DCHECK_EQ(out_opcode_, 0);
  set_out_opcode(out, kInstEmptyWidth);
  empty_ = empty;
}

void Prog::Inst::InitMatch(int32 id) {
  DCHECK_EQ(out_opcode_, 0);
  set_opcode(kInstMatch);
  match_id_ = id;
}

void Prog::Inst::InitNop(uint32 out) {
  DCHECK_EQ(out_opcode_, 0);
  set_opcode(kInstNop);
}

void Prog::Inst::InitFail() {
  DCHECK_EQ(out_opcode_, 0);
  set_opcode(kInstFail);
}

string Prog::Inst::Dump() {
  switch (opcode()) {
    default:
      return StringPrintf("opcode %d", static_cast<int>(opcode()));

    case kInstAlt:
      return StringPrintf("alt -> %d | %d", out(), out1_);

    case kInstAltMatch:
      return StringPrintf("altmatch -> %d | %d", out(), out1_);

    case kInstByteRange:
      return StringPrintf("byte%s [%02x-%02x] -> %d",
                          foldcase_ ? "/i" : "",
                          lo_, hi_, out());

    case kInstCapture:
      return StringPrintf("capture %d -> %d", cap_, out());

    case kInstEmptyWidth:
      return StringPrintf("emptywidth %#x -> %d",
                          static_cast<int>(empty_), out());

    case kInstMatch:
      return StringPrintf("match! %d", match_id());

    case kInstNop:
      return StringPrintf("nop -> %d", out());

    case kInstFail:
      return StringPrintf("fail");
  }
}

Prog::Prog()
  : anchor_start_(false),
    anchor_end_(false),
    reversed_(false),
    did_flatten_(false),
    did_onepass_(false),
    start_(0),
    start_unanchored_(0),
    size_(0),
    bytemap_range_(0),
    first_byte_(-1),
    flags_(0),
    onepass_statesize_(0),
    inst_(NULL),
    dfa_first_(NULL),
    dfa_longest_(NULL),
    dfa_mem_(0),
    onepass_nodes_(NULL),
    onepass_start_(NULL) {
}

Prog::~Prog() {
  DeleteDFA(&dfa_first_);
  DeleteDFA(&dfa_longest_);
  delete[] onepass_nodes_;
  delete[] inst_;
}

typedef SparseSet Workq;

static inline void AddToQueue(Workq* q, int id) {
  if (id != 0)
    q->insert(id);
}

static string ProgToString(Prog* prog, Workq* q) {
  string s;
  for (Workq::iterator i = q->begin(); i != q->end(); ++i) {
    int id = *i;
    Prog::Inst* ip = prog->inst(id);
    StringAppendF(&s, "%d. %s\n", id, ip->Dump().c_str());
    AddToQueue(q, ip->out());
    if (ip->opcode() == kInstAlt || ip->opcode() == kInstAltMatch)
      AddToQueue(q, ip->out1());
  }
  return s;
}

static string FlattenedProgToString(Prog* prog, int start) {
  string s;
  for (int id = start; id < prog->size(); id++) {
    Prog::Inst* ip = prog->inst(id);
    if (ip->last())
      StringAppendF(&s, "%d. %s\n", id, ip->Dump().c_str());
    else
      StringAppendF(&s, "%d+ %s\n", id, ip->Dump().c_str());
  }
  return s;
}

string Prog::Dump() {
  if (did_flatten_)
    return FlattenedProgToString(this, start_);

  Workq q(size_);
  AddToQueue(&q, start_);
  return ProgToString(this, &q);
}

string Prog::DumpUnanchored() {
  if (did_flatten_)
    return FlattenedProgToString(this, start_unanchored_);

  Workq q(size_);
  AddToQueue(&q, start_unanchored_);
  return ProgToString(this, &q);
}

string Prog::DumpByteMap() {
  string map;
  for (int c = 0; c < 256; c++) {
    int b = bytemap_[c];
    int lo = c;
    while (c < 256-1 && bytemap_[c+1] == b)
      c++;
    int hi = c;
    StringAppendF(&map, "[%02x-%02x] -> %d\n", lo, hi, b);
  }
  return map;
}

int Prog::first_byte() {
  std::call_once(first_byte_once_, [this]() {
    first_byte_ = ComputeFirstByte();
  });
  return first_byte_;
}

static bool IsMatch(Prog*, Prog::Inst*);

// Peep-hole optimizer.
void Prog::Optimize() {
  Workq q(size_);

  // Eliminate nops.  Most are taken out during compilation
  // but a few are hard to avoid.
  q.clear();
  AddToQueue(&q, start_);
  for (Workq::iterator i = q.begin(); i != q.end(); ++i) {
    int id = *i;

    Inst* ip = inst(id);
    int j = ip->out();
    Inst* jp;
    while (j != 0 && (jp=inst(j))->opcode() == kInstNop) {
      j = jp->out();
    }
    ip->set_out(j);
    AddToQueue(&q, ip->out());

    if (ip->opcode() == kInstAlt) {
      j = ip->out1();
      while (j != 0 && (jp=inst(j))->opcode() == kInstNop) {
        j = jp->out();
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
    int id = *i;
    Inst* ip = inst(id);
    AddToQueue(&q, ip->out());
    if (ip->opcode() == kInstAlt)
      AddToQueue(&q, ip->out1());

    if (ip->opcode() == kInstAlt) {
      Inst* j = inst(ip->out());
      Inst* k = inst(ip->out1());
      if (j->opcode() == kInstByteRange && j->out() == id &&
          j->lo() == 0x00 && j->hi() == 0xFF &&
          IsMatch(this, k)) {
        ip->set_opcode(kInstAltMatch);
        continue;
      }
      if (IsMatch(this, j) &&
          k->opcode() == kInstByteRange && k->out() == id &&
          k->lo() == 0x00 && k->hi() == 0xFF) {
        ip->set_opcode(kInstAltMatch);
      }
    }
  }
}

// Is ip a guaranteed match at end of text, perhaps after some capturing?
static bool IsMatch(Prog* prog, Prog::Inst* ip) {
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
        ip = prog->inst(ip->out());
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

void Prog::ComputeByteMap() {
  // Fill in byte map with byte classes for the program.
  // Ranges of bytes that are treated indistinguishably
  // are mapped to a single byte class.
  Bitmap<256> v;

  // Don't repeat the work for \b and \B.
  bool done_word_boundaries = false;

  for (int id = 0; id < static_cast<int>(size()); id++) {
    Inst* ip = inst(id);
    if (ip->opcode() == kInstByteRange) {
      int lo = ip->lo();
      int hi = ip->hi();
      if (0 < lo)
        v.Set(lo - 1);
      v.Set(hi);
      if (ip->foldcase() && lo <= 'z' && hi >= 'a') {
        if (lo < 'a')
          lo = 'a';
        if (hi > 'z')
          hi = 'z';
        if (lo <= hi) {
          v.Set(lo + 'A' - 'a' - 1);
          v.Set(hi + 'A' - 'a');
        }
      }
    } else if (ip->opcode() == kInstEmptyWidth) {
      if (ip->empty() & (kEmptyBeginLine|kEmptyEndLine)) {
        v.Set('\n' - 1);
        v.Set('\n');
      }
      if (ip->empty() & (kEmptyWordBoundary|kEmptyNonWordBoundary)) {
        if (done_word_boundaries)
          continue;
        int j;
        for (int i = 0; i < 256; i = j) {
          for (j = i + 1; j < 256 &&
                          Prog::IsWordChar(static_cast<uint8>(i)) ==
                              Prog::IsWordChar(static_cast<uint8>(j));
               j++)
            ;
          if (0 < i)
            v.Set(i - 1);
          v.Set(j - 1);
        }
        done_word_boundaries = true;
      }
    }
  }

  COMPILE_ASSERT(8*sizeof(v.Word(0)) == 32, wordsize);
  uint8 n = 0;
  uint32 bits = 0;
  for (int i = 0; i < 256; i++) {
    if ((i & 31) == 0)
      bits = v.Word(i >> 5);
    bytemap_[i] = n;
    n += bits & 1;
    bits >>= 1;
  }
  bytemap_range_ = bytemap_[255] + 1;

  if (0) {  // For debugging: use trivial byte map.
    for (int i = 0; i < 256; i++)
      bytemap_[i] = static_cast<uint8>(i);
    bytemap_range_ = 256;
    LOG(INFO) << "Using trivial bytemap.";
  }
}

void Prog::Flatten() {
  if (did_flatten_)
    return;
  did_flatten_ = true;

  // Scratch structures. It's important that these are reused by EmitList()
  // because we call it in a loop and it would thrash the heap otherwise.
  SparseSet q(size());
  vector<int> stk;
  stk.reserve(size());

  // First pass: Marks "roots".
  // Builds the mapping from inst-ids to root-ids.
  SparseArray<int> rootmap(size());
  MarkRoots(&rootmap, &q, &stk);

  // Second pass: Emits "lists". Remaps outs to root-ids.
  // Builds the mapping from root-ids to flat-ids.
  vector<int> flatmap(rootmap.size());
  vector<Inst> flat;
  flat.reserve(size());
  for (SparseArray<int>::const_iterator i = rootmap.begin();
       i != rootmap.end();
       ++i) {
    flatmap[i->value()] = static_cast<int>(flat.size());
    EmitList(i->index(), &rootmap, &flat, &q, &stk);
    flat.back().set_last();
  }

  list_count_ = static_cast<int>(flatmap.size());
  for (int i = 0; i < kNumInst; i++)
    inst_count_[i] = 0;

  // Third pass: Remaps outs to flat-ids.
  // Counts instructions by opcode.
  for (int id = 0; id < static_cast<int>(flat.size()); id++) {
    Inst* ip = &flat[id];
    if (ip->opcode() != kInstAltMatch)  // handled in EmitList()
      ip->set_out(flatmap[ip->out()]);
    inst_count_[ip->opcode()]++;
  }

  int total = 0;
  for (int i = 0; i < kNumInst; i++)
    total += inst_count_[i];
  DCHECK_EQ(total, static_cast<int>(flat.size()));

  // Remap start_unanchored and start.
  if (start_unanchored() == 0) {
    DCHECK_EQ(start(), 0);
  } else if (start_unanchored() == start()) {
    set_start_unanchored(flatmap[1]);
    set_start(flatmap[1]);
  } else {
    set_start_unanchored(flatmap[1]);
    set_start(flatmap[2]);
  }

  // Finally, replace the old instructions with the new instructions.
  size_ = static_cast<int>(flat.size());
  delete[] inst_;
  inst_ = new Inst[size_];
  memmove(inst_, flat.data(), size_ * sizeof *inst_);
}

void Prog::MarkRoots(SparseArray<int>* rootmap,
                     SparseSet* q, vector<int>* stk) {
  // Mark the kInstFail instruction.
  rootmap->set_new(0, rootmap->size());

  // Mark the start_unanchored and start instructions.
  if (!rootmap->has_index(start_unanchored()))
    rootmap->set_new(start_unanchored(), rootmap->size());
  if (!rootmap->has_index(start()))
    rootmap->set_new(start(), rootmap->size());

  q->clear();
  stk->clear();
  stk->push_back(start_unanchored());
  while (!stk->empty()) {
    int id = stk->back();
    stk->pop_back();
  Loop:
    if (q->contains(id))
      continue;
    q->insert_new(id);

    Inst* ip = inst(id);
    switch (ip->opcode()) {
      default:
        LOG(DFATAL) << "unhandled opcode: " << ip->opcode();
        break;

      case kInstAltMatch:
      case kInstAlt:
        stk->push_back(ip->out1());
        id = ip->out();
        goto Loop;

      case kInstByteRange:
      case kInstCapture:
      case kInstEmptyWidth:
        // Mark the out of this instruction.
        if (!rootmap->has_index(ip->out()))
          rootmap->set_new(ip->out(), rootmap->size());
        id = ip->out();
        goto Loop;

      case kInstNop:
        id = ip->out();
        goto Loop;

      case kInstMatch:
      case kInstFail:
        break;
    }
  }
}

void Prog::EmitList(int root, SparseArray<int>* rootmap, vector<Inst>* flat,
                    SparseSet* q, vector<int>* stk) {
  q->clear();
  stk->clear();
  stk->push_back(root);
  while (!stk->empty()) {
    int id = stk->back();
    stk->pop_back();
  Loop:
    if (q->contains(id))
      continue;
    q->insert_new(id);

    if (id != root && rootmap->has_index(id)) {
      // We reached another "tree" via epsilon transition. Emit a kInstNop
      // instruction so that the Prog does not become quadratically larger.
      flat->emplace_back();
      flat->back().set_opcode(kInstNop);
      flat->back().set_out(rootmap->get_existing(id));
      continue;
    }

    Inst* ip = inst(id);
    switch (ip->opcode()) {
      default:
        LOG(DFATAL) << "unhandled opcode: " << ip->opcode();
        break;

      case kInstAltMatch:
        flat->emplace_back();
        flat->back().set_opcode(kInstAltMatch);
        flat->back().set_out(static_cast<int>(flat->size()));
        flat->back().out1_ = static_cast<uint32>(flat->size())+1;
        // Fall through.

      case kInstAlt:
        stk->push_back(ip->out1());
        id = ip->out();
        goto Loop;

      case kInstByteRange:
      case kInstCapture:
      case kInstEmptyWidth:
        flat->emplace_back();
        memmove(&flat->back(), ip, sizeof *ip);
        flat->back().set_out(rootmap->get_existing(ip->out()));
        break;

      case kInstNop:
        id = ip->out();
        goto Loop;

      case kInstMatch:
      case kInstFail:
        flat->emplace_back();
        memmove(&flat->back(), ip, sizeof *ip);
        break;
    }
  }
}

}  // namespace re2
