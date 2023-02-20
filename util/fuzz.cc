// Copyright 2016 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Entry point for libFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char** argv) {
  uint8_t data[4096];
  for (int i = 0; i < 4096; i++) {
    for (int j = 0; j < 4096; j++) {
      data[j] = random() & 0xFF;
    }
    LLVMFuzzerTestOneInput(data, 4096);
  }
  return 0;
}
