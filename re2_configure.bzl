# Copyright 2021 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

def _re2_configure_impl(repository_ctx):
    path = repository_ctx.which("python3-config")
    if not path:
        fail("Failed to find python3-config")

    result = repository_ctx.execute([path, "--includes"])
    if result.return_code:
        fail("Failed to execute python3-config:", result.stderr)

    copts = [copt for copt in result.stdout.split(" ") if copt]
    repository_ctx.file(
        "python/defs.bzl",
        content = "COPTS = %r\n" % copts,
        executable = False,
    )

    # Bazel requires a BUILD file. Yes, even if it's empty.
    repository_ctx.file(
        "python/BUILD",
        content = "",
        executable = False,
    )

re2_configure = repository_rule(
    implementation = _re2_configure_impl,
    attrs = {
        # Nothing yet...
    },
    configure = True,
)
