// Copyright 2017 The RE2 Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_DFA_H_
#define RE2_DFA_H_

// Interface for passing DFA details to caller.

namespace re2 {

class DFAWriter {
 public:
  DFAWriter();
  virtual ~DFAWriter();

  // Called to add a transition from src_state reading c into dst_state.
  virtual void AddTransition(int src_state, int c, int dst_state) = 0;
  
  // Called to record that state is a final state.
  virtual void AddFinal(int state) = 0;
  
  // Called to record that the DFA building is out of memory and stopping early.
  virtual void OutOfMemory() = 0;
};

}  // namespace re2


#endif  // RE2_DFA_H_
