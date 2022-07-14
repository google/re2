#!/bin/bash
set -eux

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

if [[ ${UID} -ne 0 ]]; then
  if [[ -d deploy ]]; then
    echo -e '\033[1;31m' "** The ${PWD}/deploy directory exists! Refusing to clobber it! **" '\033[0m'
    exit 1
  fi
  mkdir deploy
  sudo docker run -i -t --pull always --rm -v ${SRCDIR}:/src -v ${PWD}:/dst emscripten/emsdk /src/$(basename $0)
  ls -l deploy
else
  cd ${DSTDIR}
  git clone --depth 1 --branch main https://github.com/google/re2.git
  cd re2
  perl -i -lpe 's/-pthread //;' Makefile
  emmake make -j$(nproc) obj/libre2.a
  em++ --bind -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=loadModule -Wall -Wextra -Wno-unused-parameter -I. -O3 -g -DNDEBUG -Lobj -lre2 \
    ${SRCDIR}/_re2.cc -o ../_re2.js
  cd ..
  rm -rf re2

  cd ${SRCDIR}
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
