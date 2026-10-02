// Copyright 2026 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

module;

#include "re2/filtered_re2.h"
#include "re2/re2.h"
#include "re2/set.h"
#include "re2/stringpiece.h"

export module re2;

export namespace re2 {
    using re2::Prog;
    using re2::Regexp;
    using re2::RE2;
    using re2::PrefilterTree;
    using re2::FilteredRE2;
    using re2::StringPiece;

    namespace hooks {
        using re2::hooks::context;
        using re2::hooks::DFASearchFailure;
        using re2::hooks::DFAStateCacheReset;
        using re2::hooks::DFASearchFailureCallback;
        using re2::hooks::DFAStateCacheResetCallback;
        using re2::hooks::GetDFASearchFailureHook;
        using re2::hooks::GetDFAStateCacheResetHook;
        using re2::hooks::SetDFASearchFailureHook;
        using re2::hooks::SetDFAStateCacheResetHook;
    }
}
