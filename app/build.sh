#!/bin/bash
set -eux

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

cd ${SRCDIR}
# Emscripten doesn't support `-fstack-protector`.
AR=emar CC=emcc \
  bazel build --compilation_mode=opt \
  --copt=-fno-stack-protector \
  -- :all
cp ../bazel-bin/app/_re2.js ${DSTDIR}
bazel clean --expunge
cp app.ts index.html _re2.d.ts ${DSTDIR}
cp package.json rollup.config.js tsconfig.json ${DSTDIR}

cd ${DSTDIR}
npm install
npx tsc
npx rollup -c rollup.config.js -d deploy

cd ${SRCDIR}
mkdir deploy
cat >deploy/index.html <<EOF
<html><head><meta http-equiv="refresh" content="0; url=https://github.com/google/re2"></head><body></body></html>
EOF
mkdir deploy/app
cp ${DSTDIR}/deploy/* deploy/app
ls -lR deploy

exit 0
