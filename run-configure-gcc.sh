#!/bin/bash

export LLVMDIR="/usr"
export CC="${LLVMDIR}/bin/gcc"
export CPPFLAGS="-D_GNU_SOURCE -D_XOPEN_SOURCE=700"
export CFLAGS="-O3 -std=c99 -finline-functions -ftree-vectorize -ftree-slp-vectorize"
export CFLAGS="${CFLAGS} -funroll-loops -mtune=core-avx2"
export CFLAGS="${CFLAGS} -fopenmp"
export CFLAGS="${CFLAGS} -B${LLVMDIR}/bin"
export LDFLAGS="-O3 -fuse-ld=gold -lgomp"

./configure --prefix="/usr/local" --libdir="/usr/local/lib64" \
  --with-pic \
  --with-max-unsigned-type="uint64_t" \
  --with-complex-type='complex double' \
  --with-imaginary='_Complex_I'

