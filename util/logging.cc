// Copyright 2015 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "util/logging.h"

DEFINE_int32(minloglevel, 0,  // INFO
             "Messages logged at a lower level than this don't actually get "
             "logged anywhere");
