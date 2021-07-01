#!/bin/bash
# Uploads source and wheels to PyPI.
# Expects to be used in a container:
#   docker run -i -t --pull always --rm -v $PWD:/src quay.io/pypa/manylinux2014_x86_64
set -eux

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

cd ${SRCDIR}
TAG=$(perl -lne 'if (/(\d{4})(\d{2})(\d{2})/) { print "$1-$2-$3"; }' setup.py)

cd ${DSTDIR}
git clone --depth 1 --branch ${TAG} https://github.com/google/re2.git
cd re2
perl -i -lpe 'if (/^CXXFLAGS/) { $_ .= " -fPIC"; }' Makefile
make -j$(nproc) static-install
cd ..
rm -rf re2

cd ${SRCDIR}
cp _re2.cc re2.py setup.py ${DSTDIR}

cd ${DSTDIR}
sed -i -E \
  -e '/absl\/memory\/memory\.h/d' \
  -e 's/absl\/strings\/string_view\.h/re2\/stringpiece.h/' \
  -e 's/absl::string_view/re2::StringPiece/' \
  -e 's/absl::make_unique<([^>]+)>\(([^)]+)\)/std::unique_ptr<\1>(new \1(\2))/' \
  _re2.cc
for TAGS in cp36-cp36m cp37-cp37m cp38-cp38 cp39-cp39; do
  PYTHON3=/opt/python/${TAGS}/bin/python3
  ${PYTHON3} -m pip install pybind11
  ${PYTHON3} setup.py bdist_wheel
  ${PYTHON3} -m pip install auditwheel
  ${PYTHON3} -m auditwheel repair --strip dist/*
  rm dist/*
done
${PYTHON3} setup.py sdist
${PYTHON3} -m pip install twine
${PYTHON3} -m twine upload dist/* wheelhouse/*

cd ${SRCDIR}
rm -rf ${DSTDIR}

exit 0
