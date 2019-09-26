#!/bin/bash
set -eux

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

cd ${SRCDIR}
cp _re2.cc re2.py setup.py ${DSTDIR}

cd ${DSTDIR}
sed -i -E \
  -e '/absl\/memory\/memory\.h/d' \
  -e 's/absl\/strings\/string_view\.h/re2\/stringpiece.h/' \
  -e 's/absl::string_view/re2::StringPiece/' \
  -e 's/absl::make_unique<([^>]+)>\(([^)]+)\)/std::unique_ptr<\1>(new \1(\2))/' \
  _re2.cc
python3 setup.py sdist
twine upload dist/*

cd ${SRCDIR}
rm -rf ${DSTDIR}

exit 0
