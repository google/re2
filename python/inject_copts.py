# Copyright 2021 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.
"""Injects copts into a compile command."""

import subprocess
import sys
import sysconfig

c = sys.argv.index('-c')
sys.argv.insert(c, '-I' + sysconfig.get_path('include'))
sys.argv.insert(c, '-fvisibility=hidden')
sys.exit(subprocess.call(sys.argv[1:]))
