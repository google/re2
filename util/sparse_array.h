// Copyright 2006 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef UTIL_SPARSE_ARRAY_H_
#define UTIL_SPARSE_ARRAY_H_

// DESCRIPTION
//
// SparseArray<T>(m) is a map from integers in [0, m) to T values.
// It requires (sizeof(T)+sizeof(int))*m memory, but it provides
// fast iteration through the elements in the array and fast clearing
// of the array.  The array has a concept of certain elements being
// uninitialized (having no value).
//
// Insertion and deletion are constant time operations.
//
// Allocating the array is a constant time operation
// when memory allocation is a constant time operation.
//
// Clearing the array is a constant time operation (unusual!).
//
// Iterating through the array is an O(n) operation, where n
// is the number of items in the array (not O(m)).
//
// The array iterator visits entries in the order they were first
// inserted into the array.  It is safe to add items to the array while
// using an iterator: the iterator will visit indices added to the array
// during the iteration, but will not re-visit indices whose values
// change after visiting.  Thus SparseArray can be a convenient
// implementation of a work queue.
//
// The SparseArray implementation is NOT thread-safe.  It is up to the
// caller to make sure only one thread is accessing the array.  (Typically
// these arrays are temporary values and used in situations where speed is
// important.)
//
// The SparseArray interface does not present all the usual STL bells and
// whistles.
//
// Implemented with reference to Briggs & Torczon, An Efficient
// Representation for Sparse Sets, ACM Letters on Programming Languages
// and Systems, Volume 2, Issue 1-4 (March-Dec.  1993), pp.  59-69.
//
// Briggs & Torczon popularized this technique, but it had been known
// long before their paper.  They point out that Aho, Hopcroft, and
// Ullman's 1974 Design and Analysis of Computer Algorithms and Bentley's
// 1986 Programming Pearls both hint at the technique in exercises to the
// reader (in Aho & Hopcroft, exercise 2.12; in Bentley, column 1
// exercise 8).
//
// Briggs & Torczon describe a sparse set implementation.  I have
// trivially generalized it to create a sparse array (actually the original
// target of the AHU and Bentley exercises).

// IMPLEMENTATION
//
// SparseArray uses a vector dense_ and an array sparse_to_dense_, both of
// size max_size_. At any point, the number of elements in the sparse array is
// size_.
//
// The vector dense_ contains the size_ elements in the sparse array (with
// their indices),
// in the order that the elements were first inserted.  This array is dense:
// the size_ pairs are dense_[0] through dense_[size_-1].
//
// The array sparse_to_dense_ maps from indices in [0,m) to indices in
// [0,size_).
// For indices present in the array, dense_[sparse_to_dense_[i]].index_ == i.
// For indices not present in the array, sparse_to_dense_ can contain
// any value at all, perhaps outside the range [0, size_) but perhaps not.
//
// The lax requirement on sparse_to_dense_ values makes clearing
// the array very easy: set size_ to 0.  Lookups are slightly more
// complicated.  An index i has a value in the array if and only if:
//   sparse_to_dense_[i] is in [0, size_) AND
//   dense_[sparse_to_dense_[i]].index_ == i.
// If both these properties hold, only then it is safe to refer to
//   dense_[sparse_to_dense_[i]].value_
// as the value associated with index i.
//
// To insert a new entry, set sparse_to_dense_[i] to size_,
// initialize dense_[size_], and then increment size_.
//
// Deletion of specific values from the array is implemented by
// swapping dense_[size_-1] and the dense_ being deleted and then
// updating the appropriate sparse_to_dense_ entries.
//
// To make the sparse array as efficient as possible for non-primitive types,
// elements may or may not be destroyed when they are deleted from the sparse
// array through a call to erase(), erase_existing() or resize(). They
// immediately become inaccessible, but they are only guaranteed to be
// destroyed when the SparseArray destructor is called.
//
// A moved-from SparseArray will be empty.

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace re2 {

template<typename Value>
class SparseArray {
 public:
  SparseArray();
  explicit SparseArray(int max_size);
  ~SparseArray();

  // IndexValue pairs: exposed in SparseArray::iterator.
  class IndexValue;

  typedef IndexValue value_type;
  typedef typename std::vector<IndexValue>::iterator iterator;
  typedef typename std::vector<IndexValue>::const_iterator const_iterator;

  SparseArray(const SparseArray& src);
  SparseArray(SparseArray&& src) /*noexcept*/;

  SparseArray& operator=(const SparseArray& src);
  SparseArray& operator=(SparseArray&& src) /*noexcept*/;

  const IndexValue& iv(int i) const;

  // Return the number of entries in the array.
  int size() const {
    return size_;
  }

  // Indicate whether the array is empty.
  int empty() const {
    return size_ == 0;
  }

  // Iterate over the array.
  iterator begin() {
    return dense_.begin();
  }
  iterator end() {
    return dense_.begin() + size_;
  }

  const_iterator begin() const {
    return dense_.begin();
  }
  const_iterator end() const {
    return dense_.begin() + size_;
  }

  // Change the maximum size of the array.
  // Invalidates all iterators.
  void resize(int max_size);

  // Return the maximum size of the array.
  // Indices can be in the range [0, max_size).
  int max_size() const {
    return max_size_;
  }

  // Clear the array.
  void clear() {
    size_ = 0;
  }

  // Check whether index i is in the array.
  bool has_index(int i) const;

  // Comparison function for sorting.
  // Can sort the sparse array so that future iterations
  // will visit indices in increasing order using
  // std::sort(arr.begin(), arr.end(), arr.less);
  static bool less(const IndexValue& a, const IndexValue& b);

 public:
  // Set the value at index i to v.
  iterator set(int i, const Value& v) {
    return SetInternal(true, i, v);
  }
  iterator set(int i, Value&& v) {  // NOLINT
    return SetInternal(true, i, std::move(v));
  }

  std::pair<iterator, bool> insert(const value_type& v) {
    return InsertInternal(v);
  }
  std::pair<iterator, bool> insert(value_type&& v) {  // NOLINT
    return InsertInternal(std::move(v));
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {  // NOLINT
    return InsertInternal(value_type(std::forward<Args>(args)...));
  }

  iterator find(int i) {
    if (has_index(i))
      return dense_.begin() + sparse_to_dense_[i];
    return end();
  }

  const_iterator find(int i) const {
    if (has_index(i))
      return dense_.begin() + sparse_to_dense_[i];
    return end();
  }

  // Change the value at index i to v.
  // Fast but unsafe: only use if has_index(i) is true.
  iterator set_existing(int i, const Value& v) {
    return SetExistingInternal(i, v);
  }
  iterator set_existing(int i, Value&& v) {  // NOLINT
    return SetExistingInternal(i, std::move(v));
  }

  // Set the value at the new index i to v.
  // Fast but unsafe: only use if has_index(i) is false.
  iterator set_new(int i, const Value& v) {
    return SetInternal(false, i, v);
  }
  iterator set_new(int i, Value&& v) {  // NOLINT
    return SetInternal(false, i, std::move(v));
  }

  // Get the value at index i from the array..
  // Fast but unsafe: only use if has_index(i) is true.
  const Value& get_existing(int i) const;

  // Erasing items from the array during iteration is in general
  // NOT safe.  There is one special case, which is that the current
  // index-value pair can be erased as long as the iterator is then
  // checked for being at the end before being incremented.
  // For example:
  //
  //   for (i = m.begin(); i != m.end(); ++i) {
  //     if (ShouldErase(i->index(), i->value())) {
  //       m.erase(i->index());
  //       --i;
  //     }
  //   }
  //
  // Except in the specific case just described, elements must
  // not be erased from the array (including clearing the array)
  // while iterators are walking over the array.  Otherwise,
  // the iterators could walk past the end of the array.

  // Erases the element at index i from the array.
  void erase(int i);

  // Erases the element at index i from the array.
  // Fast but unsafe: only use if has_index(i) is true.
  void erase_existing(int i);

 private:
  template <typename U>
  std::pair<iterator, bool> InsertInternal(U&& v) {
    DebugCheckInvariants();
    std::pair<iterator, bool> p;
    if (has_index(v.index_)) {
      p = {dense_.begin() + sparse_to_dense_[v.index_], false};
    } else {
      p = {set_new(std::forward<U>(v).index_, std::forward<U>(v).second), true};
    }
    DebugCheckInvariants();
    return p;
  }

  template <typename U>
  iterator SetInternal(bool allow_overwrite, int i, U&& v) {  // NOLINT
    DebugCheckInvariants();
    if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(max_size_)) {
      assert(!"illegal index");
      // Semantically, end() would be better here, but we already know
      // the user did something stupid, so begin() insulates them from
      // dereferencing an invalid pointer.
      return begin();
    }
    if (!allow_overwrite) {
      assert(!has_index(i));
      create_index(i);
    } else {
      if (!has_index(i))
        create_index(i);
    }
    return set_existing(i, std::forward<U>(v));  // NOLINT
  }

  template <typename U>
  iterator SetExistingInternal(int i, U&& v) {  // NOLINT
    DebugCheckInvariants();
    assert(has_index(i));
    dense_[sparse_to_dense_[i]].value() = std::forward<U>(v);
    DebugCheckInvariants();
    return dense_.begin() + sparse_to_dense_[i];
  }

  // Add the index i to the array.
  // Only use if has_index(i) is known to be false.
  // Since it doesn't set the value associated with i,
  // this function is private, only intended as a helper
  // for other methods.
  void create_index(int i);

  // In debug mode, verify that some invariant properties of the class
  // are being maintained. This is called at the end of the constructor
  // and at the beginning and end of all public non-const member functions.
  void DebugCheckInvariants() const;

  int size_ = 0;
  int max_size_ = 0;
  std::unique_ptr<int[]> sparse_to_dense_;
  std::vector<IndexValue> dense_;
};

template<typename Value>
SparseArray<Value>::SparseArray() = default;

template<typename Value>
SparseArray<Value>::SparseArray(const SparseArray& src)
    : size_(src.size_),
      max_size_(src.max_size_),
      sparse_to_dense_(new int[max_size_]),
      dense_(src.dense_) {
  std::copy_n(src.sparse_to_dense_.get(), max_size_, sparse_to_dense_.get());
}

template<typename Value>
SparseArray<Value>::SparseArray(SparseArray&& src) /*noexcept*/  // NOLINT
    : size_(src.size_),
      max_size_(src.max_size_),
      sparse_to_dense_(std::move(src.sparse_to_dense_)),
      dense_(std::move(src.dense_)) {
  src.size_ = 0;
  src.max_size_ = 0;
  src.dense_.clear();
}

template<typename Value>
SparseArray<Value>& SparseArray<Value>::operator=(const SparseArray& src) {
  std::unique_ptr<int[]> a(new int[src.max_size_]);
  std::copy_n(src.sparse_to_dense_.get(), src.max_size_, a.get());
  sparse_to_dense_ = std::move(a);
  dense_ = src.dense_;
  max_size_ = src.max_size_;
  size_ = src.size_;
  return *this;
}

template<typename Value>
SparseArray<Value>& SparseArray<Value>::operator=(
    SparseArray&& src) /*noexcept*/ {  // NOLINT
  size_ = src.size_;
  max_size_ = src.max_size_;
  sparse_to_dense_ = std::move(src.sparse_to_dense_);
  dense_ = std::move(src.dense_);
  // clear out the source
  src.size_ = 0;
  src.max_size_ = 0;
  src.dense_.clear();
  return *this;
}

// IndexValue pairs: exposed in SparseArray::iterator.
template<typename Value>
class SparseArray<Value>::IndexValue {
  friend class SparseArray;
 public:
  typedef int first_type;
  typedef Value second_type;

  IndexValue() {}
  IndexValue(int i, const Value& v) : index_(i), second(v) {}
  IndexValue(int i, Value&& v) : index_(i), second(std::move(v)) {}

  int index() const { return index_; }

  Value& value() /*&*/ { return second; }
  const Value& value() const /*&*/ { return second; }
  //Value&& value() /*&&*/ { return std::move(second); }  // NOLINT

 private:
  int index_;

 public:
  // Provide the data in the 'second' member so that the utilities
  // in map-util work.
  // TODO(billydonahue): 'second' is public for short-term compatibility.
  // Users will be transitioned to using value() accessor.
  Value second;
};

template<typename Value>
const typename SparseArray<Value>::IndexValue&
SparseArray<Value>::iv(int i) const {
  assert(i >= 0);
  assert(i < size_);
  return dense_[i];
}

// Change the maximum size of the array.
// Invalidates all iterators.
template<typename Value>
void SparseArray<Value>::resize(int max_size) {
  DebugCheckInvariants();
  if (max_size > max_size_) {
    std::unique_ptr<int[]> a(new int[max_size]);
    if (sparse_to_dense_) {
      std::copy_n(sparse_to_dense_.get(), max_size_, a.get());
    }
    sparse_to_dense_ = std::move(a);

    dense_.resize(max_size);

#ifdef MEMORY_SANITIZER
    for (int i = max_size_; i < max_size; i++) {
      sparse_to_dense_[i] = 0xababababU;
      dense_[i].index_ = 0xababababU;
    }
#endif
  }
  max_size_ = max_size;
  if (size_ > max_size_)
    size_ = max_size_;
  DebugCheckInvariants();
}

// Check whether index i is in the array.
template<typename Value>
bool SparseArray<Value>::has_index(int i) const {
  assert(i >= 0);
  assert(i < max_size_);
  if (static_cast<uint32_t>(i) >= static_cast<uint32_t>(max_size_)) {
    return false;
  }
  // Unsigned comparison avoids checking sparse_to_dense_[i] < 0.
  return (uint32_t)sparse_to_dense_[i] < (uint32_t)size_ &&
         dense_[sparse_to_dense_[i]].index_ == i;
}

template<typename Value>
const Value& SparseArray<Value>::get_existing(int i) const {
  assert(has_index(i));
  return dense_[sparse_to_dense_[i]].second;
}

template<typename Value>
void SparseArray<Value>::erase(int i) {
  DebugCheckInvariants();
  if (has_index(i))
    erase_existing(i);
  DebugCheckInvariants();
}

template<typename Value>
void SparseArray<Value>::erase_existing(int i) {
  DebugCheckInvariants();
  assert(has_index(i));
  int di = sparse_to_dense_[i];
  if (di < size_ - 1) {
    dense_[di] = std::move(dense_[size_ - 1]);
    sparse_to_dense_[dense_[di].index_] = di;
  }
  size_--;
  DebugCheckInvariants();
}

template<typename Value>
void SparseArray<Value>::create_index(int i) {
  assert(!has_index(i));
  assert(size_ < max_size_);
  sparse_to_dense_[i] = size_;
  dense_[size_].index_ = i;
  size_++;
}

template<typename Value> SparseArray<Value>::SparseArray(int max_size) {
  max_size_ = max_size;
  sparse_to_dense_ = std::unique_ptr<int[]>(new int[max_size]);
  dense_.resize(max_size);
  size_ = 0;

#ifdef MEMORY_SANITIZER
  for (int i = 0; i < max_size; i++) {
    sparse_to_dense_[i] = 0xababababU;
    dense_[i].index_ = 0xababababU;
  }
#endif

  DebugCheckInvariants();
}

template<typename Value> SparseArray<Value>::~SparseArray() {
  DebugCheckInvariants();
}

template<typename Value> void SparseArray<Value>::DebugCheckInvariants() const {
  assert(0 <= size_);
  assert(size_ <= max_size_);
  assert(size_ == 0 || sparse_to_dense_ != NULL);
}

// Comparison function for sorting.
template<typename Value> bool SparseArray<Value>::less(const IndexValue& a,
                                                       const IndexValue& b) {
  return a.index_ < b.index_;
}

}  // namespace re2

#endif  // UTIL_SPARSE_ARRAY_H_
