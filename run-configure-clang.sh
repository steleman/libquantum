#!/bin/bash

export LLVMDIR="/usr"
export CC="${LLVMDIR}/bin/clang"
export CPPFLAGS="-D_GNU_SOURCE -D_XOPEN_SOURCE=700"
export CFLAGS="-O3 -std=c99 -finline-functions -fvectorize -fslp-vectorize"
export CFLAGS="${CFLAGS} -funroll-loops -mtune=core-avx2"
export CFLAGS="${CFLAGS} -fopenmp=libomp"
export CFLAGS="${CFLAGS} -B${LLVMDIR}/bin"
export LDFLAGS="-O3 -fuse-ld=lld"
export LDFLAGS="${LDFLAGS} -L${LLVMDIR}/lib64 -lomp -Wl,-rpath -Wl,${LLVMDIR}/lib64"

./configure --prefix="/usr/local" --libdir="/usr/local/lib64" \
  --with-pic \
  --with-max-unsigned-type="uint64_t" \
  --with-complex-type='complex double' \
  --with-imaginary='_Complex_I'

