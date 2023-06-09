/* complex.h: Declarations for complex.c

   Copyright 2003 Bjoern Butscher, Hendrik Weimer

   This file is part of libquantum

   libquantum is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License,
   or (at your option) any later version.

   libquantum is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with libquantum; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA

*/

#ifndef __QCOMPLEX_H
#define __QCOMPLEX_H

#include <complex.h>
#include "config.h"

#define quantum_conj(z) conj(z)
#define quantum_real(z) creal(z)
#define quantum_imag(z) cimag(z)

extern double quantum_prob(complex double a);
extern complex double quantum_cexp(double phi);

/* Calculate the square of a complex number (i.e. the probability) */

static inline double
quantum_prob_inline(complex double a)
{
  double r = quantum_real(a);
  double i = quantum_imag(a);
  return r * r + i * i;
}

#endif
