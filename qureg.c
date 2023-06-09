/* qureg.c: Quantum register management

   Copyright 2003-2013 Bjoern Butscher, Hendrik Weimer

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "matrix.h"
#include "qureg.h"
#include "config.h"
#include "qcomplex.h"
#include "objcode.h"
#include "error.h"

/* Convert a vector to a quantum register */

quantum_reg
quantum_matrix2qureg(quantum_matrix *m, int width)
{
  quantum_reg reg;
  int i, j, size=0;

  if(m->cols != 1)
    quantum_error(QUANTUM_EMCMATRIX);

  reg.width = width;

  /* Determine the size of the quantum register */

  for(i=0; i<m->rows; i++)
    {
      if(m->t[i])
	size++;
    }

  /* Allocate the required memory */

  reg.size = size;
  reg.hashw = width + 2;

  reg.amplitude = calloc(size, sizeof(COMPLEX_FLOAT));
  reg.state = calloc(size, sizeof(MAX_UNSIGNED));

  if(!(reg.state && reg.amplitude))
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(size * (sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));

  /* Allocate the hash table */

  reg.hash = calloc(1 << reg.hashw, sizeof(int));

  if(!reg.hash)
    quantum_error(QUANTUM_ENOMEM);
        
  quantum_memman((1 << reg.hashw) * sizeof(int));

  /* Copy the nonzero amplitudes of the vector into the quantum
     register */

  for(i=0, j=0; i<m->rows; i++)
    {
      if(m->t[i])
	{
	  reg.state[j] = i;
	  reg.amplitude[j] = m->t[i];
	  j++;
	}
    }

  return reg;
}

/* Create a new quantum register from scratch */

quantum_reg
quantum_new_qureg(MAX_UNSIGNED initval, int width)
{
  quantum_reg reg;
  char *c;

  reg.width = width;
  reg.size = 1;
  reg.hashw = width + 2;

  /* Allocate memory for 1 base state */

  reg.state = calloc(1, sizeof(MAX_UNSIGNED));
  reg.amplitude = calloc(1, sizeof(COMPLEX_FLOAT));

  if(!(reg.state && reg.amplitude))
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(sizeof(MAX_UNSIGNED) + sizeof(COMPLEX_FLOAT));

  /* Allocate the hash table */

  reg.hash = calloc(1 << reg.hashw, sizeof(int));

  if(!reg.hash)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman((1 << reg.hashw) * sizeof(int));

  /* Initialize the quantum register */
  
  reg.state[0] = initval;
  reg.amplitude[0] = 1;

  /* Initialize the PRNG */

  /*  srandom(time(0)); */

  c = getenv("QUOBFILE");

  if(c)
    {
      quantum_objcode_start();
      quantum_objcode_file(c);
      atexit((void *) &quantum_objcode_exit);
    }

  quantum_objcode_put(INIT, initval);

  return reg;
}

/* Returns an empty quantum register of size N */

quantum_reg
quantum_new_qureg_size(int n, int width)
{
  quantum_reg reg;

  reg.width = width;
  reg.size = n;
  reg.hashw = 0;
  reg.hash = 0;

  /* Allocate memory for n basis states */

  reg.amplitude = calloc(n, sizeof(COMPLEX_FLOAT));
  reg.state = 0;

  if(!reg.amplitude)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(n*sizeof(COMPLEX_FLOAT));

  return reg;
}

/* Returns an empty sparse quantum register of size N */

quantum_reg
quantum_new_qureg_sparse(int n, int width)
{
  quantum_reg reg;

  reg.width = width;
  reg.size = n;
  reg.hashw = 0;
  reg.hash = 0;

  /* Allocate memory for n basis states */

  reg.amplitude = calloc(n, sizeof(COMPLEX_FLOAT));
  reg.state = calloc(n, sizeof(MAX_UNSIGNED));

  if(!(reg.amplitude && reg.state))
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(n*(sizeof(COMPLEX_FLOAT)+sizeof(MAX_UNSIGNED)));

  return reg;
}

/* Convert a quantum register to a vector */

quantum_matrix
quantum_qureg2matrix(quantum_reg reg)
{
  quantum_matrix m;
  int i;

  m = quantum_new_matrix(1, 1 << reg.width);
  
  for(i=0; i<reg.size; i++)
    m.t[reg.state[i]] = reg.amplitude[i];

  return m;
}

/* Destroys the entire hash table of a quantum register */

void
quantum_destroy_hash(quantum_reg *reg)
{
  free(reg->hash);
  quantum_memman(-(1 << reg->hashw) * sizeof(int));
  reg->hash = 0;
}

/* Delete a quantum register */

void
quantum_delete_qureg(quantum_reg *reg)
{
  if(reg->hashw && reg->hash)
    quantum_destroy_hash(reg);

  free(reg->amplitude);
  quantum_memman(-reg->size * sizeof(COMPLEX_FLOAT));
  reg->amplitude = 0;

  if(reg->state)
    {
      free(reg->state);
      quantum_memman(-reg->size * sizeof(MAX_UNSIGNED));
      reg->state = 0;
    }

}

/* Delete a quantum register but leave the hash table alive */

void
quantum_delete_qureg_hashpreserve(quantum_reg *reg)
{
  free(reg->amplitude);
  quantum_memman(-reg->size * sizeof(COMPLEX_FLOAT));
  reg->amplitude = 0;

  if(reg->state)
    {
      free(reg->state);
      quantum_memman(-reg->size * sizeof(MAX_UNSIGNED));
      reg->state = 0;
    }
}

/* Copy the contents of src to dst */

void
quantum_copy_qureg(quantum_reg *src, quantum_reg *dst)
{
  *dst = *src;
  
  /* Allocate memory for basis states */

  dst->amplitude = calloc(dst->size, sizeof(COMPLEX_FLOAT));

  if(!dst->amplitude)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(dst->size*sizeof(COMPLEX_FLOAT));

  memcpy(dst->amplitude, src->amplitude, src->size*sizeof(COMPLEX_FLOAT));

  if(src->state)
    {
      dst->state = calloc(dst->size, sizeof(MAX_UNSIGNED));

      if(!dst->state)
	quantum_error(QUANTUM_ENOMEM);
      
      quantum_memman(dst->size*sizeof(MAX_UNSIGNED));

      memcpy(dst->state, src->state, src->size*sizeof(MAX_UNSIGNED));

    }

  /* Allocate the hash table */

  if(dst->hashw)
    {
      dst->hash = calloc(1 << dst->hashw, sizeof(int));
      
      if(!dst->hash)
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman((1 << dst->hashw) * sizeof(int));
    }

}

/* Print the contents of a quantum register to stdout */

void
quantum_print_qureg(quantum_reg reg)
{
  int i,j;
  
  for(i=0; i<reg.size; i++)
    {
      printf("% f %+fi|%llu> (%e) (|", quantum_real(reg.amplitude[i]),
	     quantum_imag(reg.amplitude[i]), reg.state[i], 
	     quantum_prob_inline(reg.amplitude[i]));
      for(j=reg.width-1;j>=0;j--)
	{
	  if(j % 4 == 3)
	    printf(" ");
	  printf("%i", ((((MAX_UNSIGNED) 1 << j) & reg.state[i]) > 0));
	}

      printf(">)\n");
    }

  printf("\n");
}

/* Print the output of the modular exponentation algorithm */

void
quantum_print_expn(quantum_reg reg)
{
  int i;
  
  for(i=0; i<reg.size; i++)
    {
      printf("%i: %lli\n", i, reg.state[i] - i * (1 << (reg.width / 2)));
    }
}

/* Add additional space to a qureg. It is initialized to zero and can
   be used as scratch space. Note that the space gets added at the LSB */

void
quantum_addscratch(int bits, quantum_reg *reg)
{
  int i;
  MAX_UNSIGNED l;
  
  reg->width += bits;

  for(i=0; i<reg->size; i++)
    {
      l = reg->state[i] << bits;
      reg->state[i] = l;
    }
}

/* Print the hash table to stdout and test if the hash table is
   corrupted */

void
quantum_print_hash(quantum_reg reg)
{
  int i;

  for(i=0; i < (1 << reg.hashw); i++)
    {
      if(i)
	printf("%i: %i %llu\n", i, reg.hash[i]-1, 
	       reg.state[reg.hash[i]-1]);
    }

}

/* Compute the Kronecker product of two quantum registers */

quantum_reg
quantum_kronecker(quantum_reg *reg1, quantum_reg *reg2)
{
  int i,j;
  quantum_reg reg;
  
  reg.width = reg1->width+reg2->width;
  reg.size = reg1->size*reg2->size;
  reg.hashw = reg.width + 2;

  /* allocate memory for the new basis states */

  reg.amplitude = calloc(reg.size, sizeof(COMPLEX_FLOAT));
  reg.state = calloc(reg.size, sizeof(MAX_UNSIGNED));

  if(!(reg.state && reg.amplitude))
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(reg.size * (sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));

  /* Allocate the hash table */

  reg.hash = calloc(1 << reg.hashw, sizeof(int));
  if(!reg.hash)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman((1 << reg.hashw) * sizeof(int));

  for(i=0; i<reg1->size; i++)
    for(j=0; j<reg2->size; j++)
    {
      /* printf("processing |%lli> x |%lli>\n", reg1->state[i], 
	     reg2->state[j]);
         printf("%lli\n", (reg1->state[i]) << reg2->width); */

      reg.state[i*reg2->size+j] = ((reg1->state[i]) << reg2->width) 
	| reg2->state[j];
      reg.amplitude[i*reg2->size+j] = reg1->amplitude[i] * reg2->amplitude[j];
    }

  return reg;
}

/* Reduce the state vector after measurement or partial trace */

quantum_reg
quantum_state_collapse(int pos, int value, quantum_reg reg)
{
  int i, j, k;
  int size=0;
  double d=0;
  MAX_UNSIGNED lpat=0, rpat=0, pos2;
  quantum_reg out;

  pos2 = (MAX_UNSIGNED) 1 << pos;

  /* Eradicate all amplitudes of base states which have been ruled out
     by the measurement and get the norm of the new register */
  
  for(i=0;i<reg.size;i++)
    {
      if(((reg.state[i] & pos2) && value) 
	 || (!(reg.state[i] & pos2) && !value))
	{
	  d += quantum_prob_inline(reg.amplitude[i]);
	  size++;
	}
    }

  /* Build the new quantum register */

  out.width = reg.width-1;
  out.size = size;
  out.amplitude = calloc(size, sizeof(COMPLEX_FLOAT));
  out.state = calloc(size, sizeof(MAX_UNSIGNED));

  if(!(out.state && out.amplitude))
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(size * (sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));
  out.hashw = reg.hashw;
  out.hash = reg.hash;

  /* Determine the numbers of the new base states and norm the quantum
     register */

  for(i=0, j=0; i<reg.size; i++)
    {
      if(((reg.state[i] & pos2) && value) 
	 || (!(reg.state[i] & pos2) && !value))
	{
	  for(k=0, rpat=0; k<pos; k++)
	    rpat += (MAX_UNSIGNED) 1 << k;

	  rpat &= reg.state[i];

	  for(k=sizeof(MAX_UNSIGNED)*8-1, lpat=0; k>pos; k--)
	    lpat += (MAX_UNSIGNED) 1 << k;

	  lpat &= reg.state[i];

	  out.state[j] = (lpat >> 1) | rpat;
	  out.amplitude[j] = reg.amplitude[i] * 1 / (float) sqrt(d);
	
	  j++;
	}
    }

  return out;

}

/* Compute the dot product of two quantum registers */

COMPLEX_FLOAT
quantum_dot_product(quantum_reg *reg1, quantum_reg *reg2)
{
  int i, j;
  COMPLEX_FLOAT f = 0;

  /* Check whether quantum registers are sorted */
  
  if(reg2->hashw)
    quantum_reconstruct_hash(reg2);

  if(reg1->state)
    {
      for(i=0; i<reg1->size; i++)
	{
	  j = quantum_get_state(reg1->state[i], *reg2);

	  if(j > -1) /* state exists in reg2 */
	    f += quantum_conj(reg1->amplitude[i]) * reg2->amplitude[j];
	}
    }

  else
    {
      for(i=0; i<reg1->size; i++)
	{
	  j = quantum_get_state(i, *reg2);

	  if(j > -1) /* state exists in reg2 */
	    f += quantum_conj(reg1->amplitude[i]) * reg2->amplitude[j];
	}
    }
      
  return f;

}

/* Same as above, but without complex conjugation */

COMPLEX_FLOAT
quantum_dot_product_noconj(quantum_reg *reg1, quantum_reg *reg2)
{
  int i, j;
  COMPLEX_FLOAT f = 0;

  /* Check whether quantum registers are sorted */
  
  if(reg2->hashw)
    quantum_reconstruct_hash(reg2);

  if(!reg2->state)
    {
      for(i=0; i<reg1->size; i++)
	f += reg1->amplitude[i] * reg2->amplitude[reg1->state[i]];
    }

  else
    {
      for(i=0; i<reg1->size; i++)
	{
	  j = quantum_get_state(reg1->state[i], *reg2);

	  if(j > -1) /* state exists in reg2 */
	    f += reg1->amplitude[i] * reg2->amplitude[j];
	}
    }

  return f;

}

/* Vector addition of two quantum registers. This is a purely
   mathematical operation without any physical meaning, so only use it
   if you know what you are doing. */

quantum_reg
quantum_vectoradd(quantum_reg *reg1, quantum_reg *reg2)
{
  int i, j, k;
  int addsize = 0;
  quantum_reg reg;

  quantum_copy_qureg(reg1, &reg);
  
  if(reg1->hashw || reg2->hashw)
    {
      quantum_reconstruct_hash(reg1);
      quantum_copy_qureg(reg1, &reg);
      
      /* Calculate the number of additional basis states */

      for(i=0; i<reg2->size; i++)
	{
	  if(quantum_get_state(reg2->state[i], *reg1) == -1)
	    addsize++;
	}
    }

  if(addsize)
    {
      reg.size += addsize;

      reg.amplitude = realloc(reg.amplitude, reg.size*sizeof(COMPLEX_FLOAT));
      reg.state = realloc(reg.state, reg.size*sizeof(MAX_UNSIGNED));

      if(!(reg.state && reg.amplitude))
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman(addsize * (sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));
    }

  k = reg1->size;

  if(!reg2->state)
    {
      for(i=0; i<reg2->size; i++)
	reg.amplitude[i] += reg2->amplitude[i];
    }

  else
    {
      for(i=0; i<reg2->size; i++)
	{
	  j = quantum_get_state(reg2->state[i], *reg1);
	  
	  if(j >= 0)
	    reg.amplitude[j] += reg2->amplitude[i];

	  else
	    {
	      reg.state[k] = reg2->state[i];
	      reg.amplitude[k] = reg2->amplitude[i];
	      k++;
	    }
	}
    }
  
  return reg;
      
}

/* Same as above, but the result is stored in the first register */

void
quantum_vectoradd_inplace(quantum_reg *reg1, quantum_reg *reg2)
{
  int i, j, k;
  int addsize = 0;

  if(reg1->hashw || reg2->hashw)
    {
      quantum_reconstruct_hash(reg1);

      /* Calculate the number of additional basis states */

      for(i=0; i<reg2->size; i++)
	{
	  if(quantum_get_state(reg2->state[i], *reg1) == -1)
	    addsize++;
	}
    }

  if(addsize)
    {

      /* Allocate memory for basis states */

      reg1->amplitude = realloc(reg1->amplitude, 
				(reg1->size+addsize)*sizeof(COMPLEX_FLOAT));
      reg1->state = realloc(reg1->state, (reg1->size+addsize)
			    *sizeof(MAX_UNSIGNED));

      if(!(reg1->state && reg1->amplitude))
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman(addsize * (sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));

    }

  k = reg1->size;

  if(!reg2->state)
    {
      for(i=0; i<reg2->size; i++)
	reg1->amplitude[i] += reg2->amplitude[i];
    }

  else
    {
      for(i=0; i<reg2->size; i++)
	{
	  j = quantum_get_state(reg2->state[i], *reg1);

	  if(j >= 0)
	    reg1->amplitude[j] += reg2->amplitude[i];

	  else
	    {
	      reg1->state[k] = reg2->state[i];
	      reg1->amplitude[k] = reg2->amplitude[i];
	      k++;
	    }
	}

      reg1->size += addsize;
    }
      
}


/* Matrix-vector multiplication for a quantum register. A is a
   function returning a quantum register containing the row given in
   the first parameter. An additional parameter (e.g. time) may be
   supplied as well. */

quantum_reg
quantum_matrix_qureg(quantum_reg A(MAX_UNSIGNED, double), double t,
		     quantum_reg *reg, int flags)
{
  int i;
  quantum_reg reg2;
  quantum_reg tmp;

  reg2.width = reg->width;
  reg2.size = reg->size;
  reg2.hashw = 0;
  reg2.hash = 0;

  reg2.amplitude = calloc(reg2.size, sizeof(COMPLEX_FLOAT));
  reg2.state = 0;

  if(!reg2.amplitude)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(reg2.size * sizeof(COMPLEX_FLOAT));

  if(reg->state)
    {
      reg2.state = calloc(reg2.size, sizeof(MAX_UNSIGNED));

      if(!reg2.state)
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman(reg2.size * sizeof(MAX_UNSIGNED));
    }

#ifdef _OPENMP
  #pragma omp parallel for private (tmp)
#endif
  for(i=0; i<reg->size; i++)
    {
      if(reg2.state)
	reg2.state[i] = i;
      tmp = A(i, t);
      reg2.amplitude[i] = quantum_dot_product_noconj(&tmp, reg);
      if(!(flags & 1))
	quantum_delete_qureg(&tmp);
    }
 
  return reg2;

}

/* Matrix-vector multiplication using a quantum_matrix */

void 
quantum_mvmult(quantum_reg *y, quantum_matrix A, quantum_reg *x)
{
  int i, j;

  for(i=0; i<A.cols; i++)
    {
      y->amplitude[i] = 0;
      for(j=0; j<A.cols; j++)
	y->amplitude[i] += M(A, j, i)*x->amplitude[j];
    }
} 


/* Scalar multiplication of a quantum register. This is a purely
   mathematical operation without any physical meaning, so only use it
   if you know what you are doing. */

void
quantum_scalar_qureg(COMPLEX_FLOAT r, quantum_reg *reg)
{
  int i;
  
  for(i=0; i<reg->size; i++)
      reg->amplitude[i] *= r;
}

/* Print the time evolution matrix for a series of gates */

void 
quantum_print_timeop(int width, void f(quantum_reg *))
{
  int i, j;
  quantum_reg tmp;
  quantum_matrix m;
  
  m = quantum_new_matrix(1 << width, 1 << width);

  for(i=0;i<(1 << width); i++)
    {
      tmp = quantum_new_qureg(i, width);
      f(&tmp);
      for(j=0; j<tmp.size; j++)
	M(m, tmp.state[j], i) = tmp.amplitude[j];

      quantum_delete_qureg(&tmp);
	  
    }
  
  quantum_print_matrix(m);

  quantum_delete_matrix(&m);
  
}

/* Normalize a quantum register */

void
quantum_normalize(quantum_reg *reg)
{
  int i;
  double r = 0;

  for(i=0; i<reg->size; i++)
    r += quantum_prob(reg->amplitude[i]);

  quantum_scalar_qureg(1./sqrt(r), reg);

}
