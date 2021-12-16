#!/bin/bash
# Uploads source and wheels to PyPI.
set -eux

IMAGES=(
  'quay.io/pypa/manylinux2014_aarch64'
  'quay.io/pypa/manylinux2014_x86_64'
  'quay.io/pypa/manylinux_2_24_aarch64'
  'quay.io/pypa/manylinux_2_24_x86_64'
)

TAGS=(
  'cp37-cp37m'
  'cp38-cp38'
  'cp39-cp39'
  'cp310-cp310'
)

SRCDIR=$(readlink --canonicalize $(dirname $0))
DSTDIR=$(mktemp --directory --tmpdir $(basename $0).XXXXXXXXXX)

if [[ ${UID} -ne 0 ]]; then
  cd ${DSTDIR}
  mkdir dist wheelhouse
  sudo docker run --pull always --rm --privileged multiarch/qemu-user-static --reset -p yes
  for IMAGE in ${IMAGES[@]}; do
    sudo docker run -i -t --pull always --rm -v ${SRCDIR}:/src -v ${DSTDIR}:/dst ${IMAGE} /src/$(basename $0)
  done
  ls -l dist wheelhouse
  twine upload dist/* wheelhouse/*
else
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
  for TAG in ${TAGS[@]}; do
    PYTHON3=/opt/python/${TAG}/bin/python3
    ${PYTHON3} -m pip install --upgrade pip
    ${PYTHON3} -m pip install pybind11
    ${PYTHON3} setup.py bdist_wheel
    ${PYTHON3} -m pip install auditwheel
    ${PYTHON3} -m auditwheel repair --strip dist/*
    rm dist/*
    mv wheelhouse/* /dst/wheelhouse
  done
  ${PYTHON3} setup.py sdist
  mv dist/* /dst/dist
fi

cd ${SRCDIR}
rm -rf ${DSTDIR}

exit 0
