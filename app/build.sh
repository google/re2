#!/bin/bash
set -eux

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

BAZEL=/tmp/bazel
BAZELISK_RELEASE=v1.17.0

if [[ ${UID} -ne 0 ]]; then
  if [[ -d deploy ]]; then
    echo -e '\033[1;31m' "** The ${PWD}/deploy directory exists! Refusing to clobber it! **" '\033[0m'
    exit 1
  fi
  mkdir deploy
  sudo docker run -i -t --pull always --rm -v ${SRCDIR}/..:/src -v ${PWD}:/dst emscripten/emsdk /src/app/$(basename $0)
  ls -l deploy
else
  wget -O ${BAZEL} https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_RELEASE}/bazelisk-linux-amd64
  chmod +x ${BAZEL}

  cd ${SRCDIR}
  # Emscripten doesn't support `-fstack-protector`.
  AR=emar CC=emcc \
    ${BAZEL} build --compilation_mode=opt \
    --copt=-fno-stack-protector \
    -- :all
  cp ../bazel-bin/app/_re2.js ${DSTDIR}
  # Clean up the sundry Bazel output directories.
  ${BAZEL} clean --expunge
  cp app.ts index.html _re2.d.ts ${DSTDIR}
  cp package.json rollup.config.js tsconfig.json ${DSTDIR}

  cd ${DSTDIR}
  npm install
  npx tsc
  npx rollup -c rollup.config.js -d deploy
  mv deploy/* /dst/deploy
fi

cd ${SRCDIR}
rm -rf ${DSTDIR}

exit 0
