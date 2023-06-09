/* gates.c: Basic gates for quantum register manipulation

   Copyright 2003, 2004 Bjoern Butscher, Hendrik Weimer

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>

#include "matrix.h"
#include "defs.h"
#include "qcomplex.h"
#include "qureg.h"
#include "decoherence.h"
#include "qec.h"
#include "objcode.h"
#include "error.h"

/* Apply a controlled-not gate */

void
quantum_cnot(int control, int target, quantum_reg *reg)
{
  int i;
  int qec;

  quantum_qec_get_status(&qec, NULL);

  if(qec)
    quantum_cnot_ft(control, target, reg);
  else
    {
      if(quantum_objcode_put(CNOT, control, target))
	return;

#ifdef _OPENMP
#pragma omp parallel for
#endif      
      for(i=0; i<reg->size; i++)
	{
	  /* Flip the target bit of a basis state if the control bit is set */
      
	  if((reg->state[i] & ((MAX_UNSIGNED) 1 << control)))
	    reg->state[i] ^= ((MAX_UNSIGNED) 1 << target);
	}
      quantum_decohere(reg);
    }
}

/* Apply a toffoli (or controlled-controlled-not) gate */

void
quantum_toffoli(int control1, int control2, int target, quantum_reg *reg)
{
  int i;
  int qec;

  quantum_qec_get_status(&qec, NULL);

  if(qec)
    quantum_toffoli_ft(control1, control2, target, reg);
  else
    {
      if(quantum_objcode_put(TOFFOLI, control1, control2, target))
	return;

#ifdef _OPENMP
#pragma omp parallel for
#endif
      for(i=0; i<reg->size; i++)
	{
	  /* Flip the target bit of a basis state if both control bits are
	     set */

	  if(reg->state[i] & ((MAX_UNSIGNED) 1 << control1))
	    {
	      if(reg->state[i] & ((MAX_UNSIGNED) 1 << control2))
		{
		  reg->state[i] ^= ((MAX_UNSIGNED) 1 << target);
		}
	    }
	}
      quantum_decohere(reg);
    }
}

/* Apply an unbounded toffoli gate. This gate is not considered
elementary and is not available on all physical realizations of a
quantum computer. Be sure to pass the function the correct number of
controlling qubits. The target is given in the last argument. */

void
quantum_unbounded_toffoli(int controlling, quantum_reg *reg, ...)
{
  va_list bits;
  int target;
  int *controls;
  int i, j;

  controls = malloc(controlling * sizeof(int));

  if(!controls)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(controlling * sizeof(int));

  va_start(bits, reg);
  
  for(i=0; i<controlling; i++)
    controls[i] = va_arg(bits, int);

  target = va_arg(bits, int);

  va_end(bits);

#ifdef _OPENMP
#pragma omp parallel for private (j)
#endif      
  for(i=0; i<reg->size; i++)
    {
      for(j=0; (j < controlling) && 
	    (reg->state[i] & (MAX_UNSIGNED) 1 << controls[j]); j++);
      
      if(j == controlling) /* all control bits are set */
	reg->state[i] ^= ((MAX_UNSIGNED) 1 << target);
    }

  free(controls);
  quantum_memman(-controlling * sizeof(int));

  quantum_decohere(reg);

}
  

/* Apply a sigma_x (or not) gate */

void
quantum_sigma_x(int target, quantum_reg *reg)
{
  int i;
  int qec;

  quantum_qec_get_status(&qec, NULL);

  if(qec)
    quantum_sigma_x_ft(target, reg);
  else
    {
      if(quantum_objcode_put(SIGMA_X, target))
	return;

#ifdef _OPENMP
#pragma omp parallel for
#endif      
      for(i=0; i<reg->size; i++)
	{
	  /* Flip the target bit of each basis state */

	  reg->state[i] ^= ((MAX_UNSIGNED) 1 << target);
	} 
      quantum_decohere(reg);
    }
}

/* Apply a sigma_y gate */

void
quantum_sigma_y(int target, quantum_reg *reg)
{
  int i;

  if(quantum_objcode_put(SIGMA_Y, target))
    return;

#ifdef _OPENMP
#pragma omp parallel for
#endif        
  for(i=0; i<reg->size;i++)
    {
      /* Flip the target bit of each basis state and multiply with 
	 +/- i */

      reg->state[i] ^= ((MAX_UNSIGNED) 1 << target);
      
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	reg->amplitude[i] *= IMAGINARY;
      else
	reg->amplitude[i] *= -IMAGINARY;
    }

  quantum_decohere(reg);
}

/* Apply a sigma_y gate */

void
quantum_sigma_z(int target, quantum_reg *reg)
{
  int i;

  if(quantum_objcode_put(SIGMA_Z, target))
    return;

#ifdef _OPENMP
#pragma omp parallel for
#endif      
  for(i=0; i<reg->size; i++)
    {
      /* Multiply with -1 if the target bit is set */

      if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	reg->amplitude[i] *= -1;
    }
  quantum_decohere(reg);
}

/* Swap the first WIDTH bits of the quantum register. This is done
   classically by renaming the bits, unless QEC is enabled. */

void
quantum_swaptheleads(int width, quantum_reg *reg)
{
  int i, j;
  int pat1, pat2;
  int qec;
  MAX_UNSIGNED l;

  quantum_qec_get_status(&qec, NULL);

  if(qec)
    {
      for(i=0; i<width; i++)
	{
	  quantum_cnot(i, width+i, reg);
	  quantum_cnot(width+i, i, reg);
	  quantum_cnot(i, width+i, reg);
	}
    }
  else
    {
      for(i=0; i<reg->size; i++)
	{

	  if(quantum_objcode_put(SWAPLEADS, width))
	    return;

	  /* calculate left bit pattern */
	  
	  pat1 = reg->state[i] % ((MAX_UNSIGNED) 1 << width);
	  
	  /*calculate right but pattern */
	  
	  pat2 = 0;

	  for(j=0; j<width; j++)
	    pat2 += reg->state[i] & ((MAX_UNSIGNED) 1 << (width + j));
	  
	  /* construct the new basis state */
	  
	  l = reg->state[i] - (pat1 + pat2);
	  l += (pat1 << width);
	  l += (pat2 >> width);
	  reg->state[i] = l;
	}
    }
}

/* Swap WIDTH bits starting at WIDTH and 2*WIDTH+2 controlled by
   CONTROL */

void
quantum_swaptheleads_omuln_controlled(int control, int width, quantum_reg *reg)
{
   int i;

  for(i=0; i<width; i++)
    {
      quantum_toffoli(control, width+i, 2*width+i+2, reg);
      quantum_toffoli(control, 2*width+i+2, width+i, reg);
      quantum_toffoli(control, width+i, 2*width+i+2, reg);
    }
}

/* Apply the 2x2 matrix M to the target bit. M should be unitary. */

void 
quantum_gate1(int target, quantum_matrix m, quantum_reg *reg)
{
  int i, j, k, iset;
  int addsize=0, decsize=0;
  COMPLEX_FLOAT t, tnot=0;
  float limit;
  char *done;

  if((m.cols != 2) || (m.rows != 2))
    quantum_error(QUANTUM_EMSIZE);

  if(reg->hashw)
    {
      quantum_reconstruct_hash(reg);

      /* calculate the number of basis states to be added */

      for(i=0; i<reg->size; i++)
	{
	  /* determine whether XORed basis state already exists */

	  if(quantum_get_state(reg->state[i] 
			       ^ ((MAX_UNSIGNED) 1 << target), *reg) == -1)
	    addsize++;
	}
      
      /* allocate memory for the new basis states */
  
      reg->state = realloc(reg->state, 
			       (reg->size + addsize) * sizeof(MAX_UNSIGNED));
      reg->amplitude = realloc(reg->amplitude, 
			       (reg->size + addsize) * sizeof(COMPLEX_FLOAT));
      
      if(reg->size && !(reg->state && reg->amplitude)) 
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman(addsize*(sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));
      
      for(i=0; i<addsize; i++)
	{
	  reg->state[i+reg->size] = 0;
	  reg->amplitude[i+reg->size] = 0;
	}
       
    }

  done = calloc(reg->size + addsize, sizeof(char));

  if(!done)
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(reg->size + addsize * sizeof(char));

  k = reg->size;

  limit = (1.0 / ((MAX_UNSIGNED) 1 << reg->width)) * epsilon;

  /* perform the actual matrix multiplication */

  for(i=0; i<reg->size; i++)
    {
      if(!done[i])
	{
	  /* determine if the target of the basis state is set */
	  
	  iset = reg->state[i] & ((MAX_UNSIGNED) 1 << target);

	  tnot = 0;
	  j = quantum_get_state(reg->state[i] 
				^ ((MAX_UNSIGNED) 1<<target), *reg);
	  if(j >= 0)
	    tnot = reg->amplitude[j];

	  t = reg->amplitude[i];

	  if(iset)
	    reg->amplitude[i] = m.t[2] * tnot + m.t[3] * t;

	  else
	    reg->amplitude[i] = m.t[0] * t + m.t[1] * tnot;

	  if(j >= 0)
	    {
	      if(iset)
		reg->amplitude[j] = m.t[0] * tnot + m.t[1] * t;

	      else
		reg->amplitude[j] = m.t[2] * t + m.t[3] * tnot;
	    }

	  
	  else /* new basis state will be created */
	    {
	      
	      if((m.t[1] == 0) && (iset))
		break;
	      if((m.t[2] == 0) && !(iset))
		 break; 

	      reg->state[k] = reg->state[i] 
		^ ((MAX_UNSIGNED) 1 << target);

	      if(iset)
		reg->amplitude[k] = m.t[1] * t;

	      else
		reg->amplitude[k] = m.t[2] * t;

	      k++;
	    }

	  if(j >= 0)
	    done[j] = 1;

	}
    }

  reg->size += addsize;

  free(done);
  quantum_memman(-reg->size * sizeof(char));

  /* remove basis states with extremely small amplitude */

  if(reg->hashw)
    {
      for(i=0, j=0; i<reg->size; i++)
	{
	  if(quantum_prob_inline(reg->amplitude[i]) < limit)
	    {
	      j++;
	      decsize++;
	    }
	  
	  else if(j)
	    {
	      reg->state[i-j] = reg->state[i];
	      reg->amplitude[i-j] = reg->amplitude[i];
	    }
	}
    
      if(decsize)
	{
	  reg->size -= decsize;
	  reg->amplitude = realloc(reg->amplitude, 
				   reg->size * sizeof(COMPLEX_FLOAT));
	  reg->state = realloc(reg->state, 
			       reg->size * sizeof(MAX_UNSIGNED));
	  
	  
	  if(reg->size && !(reg->state && reg->amplitude)) 
	    quantum_error(QUANTUM_ENOMEM);

	  quantum_memman(-decsize * (sizeof(MAX_UNSIGNED) 
				     + sizeof(COMPLEX_FLOAT)));
	}
    }

  if(reg->size > (1 << (reg->hashw-1)))
    fprintf(stderr, "Warning: inefficient hash table (size %i vs hash %i)\n", 
	    reg->size, 1<<reg->hashw);


  quantum_decohere(reg);
}

/* Apply the 4x4 matrix M to the bits TARGET1 and TARGET2. M should be
   unitary. 

   Warning: code is mostly untested.*/

void 
quantum_gate2(int target1, int target2, quantum_matrix m, quantum_reg *reg)
{
  int i, j, k, l;
  int addsize=0, decsize=0;
  COMPLEX_FLOAT psi_sub[4];
  int base[4];
  int bits[2];
  float limit;
  char *done;

  if((m.cols != 4) || (m.rows != 4))
    quantum_error(QUANTUM_EMSIZE);
  
  /* Build hash table */

  for(i=0; i<(1 << reg->hashw); i++)
    reg->hash[i] = 0;
      
  for(i=0; i<reg->size; i++)
    quantum_add_hash(reg->state[i], i, reg);

  /* calculate the number of basis states to be added */

  for(i=0; i<reg->size; i++)
    {
      if(quantum_get_state(reg->state[i] ^ ((MAX_UNSIGNED) 1 << target1),
			   *reg) == -1)
	addsize++;
      if(quantum_get_state(reg->state[i] ^ ((MAX_UNSIGNED) 1 << target2),
			   *reg) == -1)
	addsize++;
    }

  /* allocate memory for the new basis states */

  reg->state = realloc(reg->state, 
		       (reg->size + addsize) * sizeof(MAX_UNSIGNED));
  reg->amplitude = realloc(reg->amplitude, 
			   (reg->size + addsize) * sizeof(COMPLEX_FLOAT));
      
  if(reg->size && !(reg->state && reg->amplitude)) 
    quantum_error(QUANTUM_ENOMEM);

  quantum_memman(addsize*(sizeof(COMPLEX_FLOAT) + sizeof(MAX_UNSIGNED)));

  for(i=0; i<addsize; i++)
    {
      reg->state[i+reg->size] = 0;
      reg->amplitude[i+reg->size] = 0;
    }

  done = calloc(reg->size + addsize, sizeof(char));

  if(!done)
    quantum_error(QUANTUM_EMSIZE);

  quantum_memman(reg->size + addsize * sizeof(char));

  l = reg->size;

  limit = (1.0 / ((MAX_UNSIGNED) 1 << reg->width)) / 1000000;

  bits[0] = target1;
  bits[1] = target2;

  /* perform the actual matrix multiplication */

  for(i=0; i<reg->size; i++)
    {
      if(!done[i])
	{
	  j = quantum_bitmask(reg->state[i], 2, bits);
	  base[j] = i;
	  base[j ^ 1] = quantum_get_state(reg->state[i] 
					  ^ ((MAX_UNSIGNED) 1 << target2),
					  *reg);
	  base[j ^ 2] = quantum_get_state(reg->state[i]
					  ^ ((MAX_UNSIGNED) 1 << target1), 
					  *reg);
	  base[j ^ 3] = quantum_get_state(reg->state[i]
					  ^ ((MAX_UNSIGNED) 1 << target1)
					  ^ ((MAX_UNSIGNED) 1 << target2),
					  *reg);

	  for(j=0; j<4; j++)
	    {
	      if(base[j] == -1)
		{
		  base[j] = l;
		  //		  reg->node[l].state = reg->state[i]
		  l++;
		}
	      psi_sub[j] = reg->amplitude[base[j]];
	    }

	  for(j=0; j<4; j++)
	    {
	      reg->amplitude[base[j]] = 0;
	      for(k=0; k<4; k++)
		reg->amplitude[base[j]] += M(m, k, j) * psi_sub[k];

	      done[base[j]] = 1;
	    }

	}
    }

  reg->size += addsize;

  free(done);

  quantum_memman(-reg->size * sizeof(char));

  /* remove basis states with extremely small amplitude */

  for(i=0, j=0; i<reg->size; i++)
    {
      if(quantum_prob_inline(reg->amplitude[i]) < limit)
	{
	  j++;
	  decsize++;
	}
      
      else if(j)
	{
	  reg->state[i-j] = reg->state[i];
	  reg->amplitude[i-j] = reg->amplitude[i];
	}
    }

  if(decsize)
    {
      reg->size -= decsize;
      reg->amplitude = realloc(reg->amplitude, 
			       reg->size * sizeof(COMPLEX_FLOAT));
      reg->state = realloc(reg->state, 
			   reg->size * sizeof(MAX_UNSIGNED));
	  
	  
      if(reg->size && !(reg->state && reg->amplitude)) 
	quantum_error(QUANTUM_ENOMEM);

      quantum_memman(-decsize * (sizeof(MAX_UNSIGNED) 
				 + sizeof(COMPLEX_FLOAT)));
      
    }

  quantum_decohere(reg);
}

/* Apply a hadamard gate */

void
quantum_hadamard(int target, quantum_reg *reg)
{
  quantum_matrix m;
  
  if(quantum_objcode_put(HADAMARD, target))
    return;
  
  m = quantum_new_matrix(2, 2);

  m.t[0] = sqrt(1.0/2);  m.t[1] = sqrt(1.0/2);
  m.t[2] = sqrt(1.0/2);  m.t[3] = -sqrt(1.0/2);

  quantum_gate1(target, m, reg);
  
  quantum_delete_matrix(&m);

}

/* Apply a walsh-hadamard transform */

void
quantum_walsh(int width, quantum_reg *reg)
{
  int i;

  for(i=0; i<width; i++)
    quantum_hadamard(i, reg);
  
}

/* Apply a rotation about the x-axis by the angle GAMMA */

void
quantum_r_x(int target, float gamma, quantum_reg *reg)
{
  quantum_matrix m;
  
  if(quantum_objcode_put(ROT_X, target, (double) gamma))
    return;

  m = quantum_new_matrix(2, 2);

  m.t[0] = cos(gamma / 2);              m.t[1] = -IMAGINARY * sin(gamma / 2);
  m.t[2] = -IMAGINARY * sin(gamma / 2); m.t[3] = cos(gamma / 2);

  quantum_gate1(target, m, reg);

  quantum_delete_matrix(&m);

}

/* Apply a rotation about the y-axis by the angle GAMMA */

void
quantum_r_y(int target, float gamma, quantum_reg *reg)
{
  quantum_matrix m;

  if(quantum_objcode_put(ROT_Y, target, (double) gamma))
    return;

  m = quantum_new_matrix(2, 2);

  m.t[0] = cos(gamma / 2);  m.t[1] = -sin(gamma / 2);
  m.t[2] = sin(gamma / 2);  m.t[3] = cos(gamma / 2);

  quantum_gate1(target, m, reg);

  quantum_delete_matrix(&m);

}

/* Apply a rotation about the z-axis by the angle GAMMA */

void
quantum_r_z(int target, float gamma, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(ROT_Z, target, (double) gamma))
    return;

  z = quantum_cexp(gamma/2);
  
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	reg->amplitude[i] *= z;
      else
	reg->amplitude[i] /= z;
    }

  quantum_decohere(reg);
}

/* Scale the phase of qubit */

void
quantum_phase_scale(int target, float gamma, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(PHASE_SCALE, target, (double) gamma))
    return;

  z = quantum_cexp(gamma);

#ifdef _OPENMP
#pragma omp parallel for
#endif        
  for(i=0; i<reg->size; i++)
    {
      reg->amplitude[i] *= z;
    }

  quantum_decohere(reg);
}


/* Apply a phase kick by the angle GAMMA */

void
quantum_phase_kick(int target, float gamma, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(PHASE_KICK, target, (double) gamma))
    return;

  z = quantum_cexp(gamma);

#ifdef _OPENMP
#pragma omp parallel for
#endif        
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	reg->amplitude[i] *= z;
    }

  quantum_decohere(reg);
}

/* Apply a conditional phase shift by PI / 2^(CONTROL - TARGET) */

void
quantum_cond_phase(int control, int target, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(COND_PHASE, control, target))
    return;

  z = quantum_cexp(pi / ((MAX_UNSIGNED) 1 << (control - target)));

#ifdef _OPENMP
#pragma omp parallel for
#endif      
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << control))
	{
	  if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	    reg->amplitude[i] *= z;
	}
    }

  quantum_decohere(reg);
}


void
quantum_cond_phase_inv(int control, int target, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  z = quantum_cexp(-pi / ((MAX_UNSIGNED) 1 << (control - target)));

#ifdef _OPENMP
#pragma omp parallel for
#endif      
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << control))
	{
	  if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	    reg->amplitude[i] *= z;
	}
    }

  quantum_decohere(reg);
}


void
quantum_cond_phase_kick(int control, int target, float gamma, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(COND_PHASE, control, target, (double) gamma))
    return;  

  z = quantum_cexp(gamma);

#ifdef _OPENMP
#pragma omp parallel for
#endif      
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << control))
	{
	  if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	    reg->amplitude[i] *= z;
	}
     }
  quantum_decohere(reg);
}

void
quantum_cond_phase_shift(int control, int target, float gamma, quantum_reg *reg)
{
  int i;
  COMPLEX_FLOAT z;

  if(quantum_objcode_put(COND_PHASE, control, target, (double) gamma))
    return;  

  z = quantum_cexp(gamma/2);

#ifdef _OPENMP
#pragma omp parallel for
#endif      
  for(i=0; i<reg->size; i++)
    {
      if(reg->state[i] & ((MAX_UNSIGNED) 1 << control))
	{
	  if(reg->state[i] & ((MAX_UNSIGNED) 1 << target))
	    reg->amplitude[i] *= z;
	  else
	    reg->amplitude[i] /= z;
	}
     }
  quantum_decohere(reg);
}




/* Increase the gate counter by INC steps or reset it if INC < 0. The
   current value of the counter is returned. */

int
quantum_gate_counter(int inc)
{
  static int counter = 0;

  if(inc > 0)
    counter += inc;
  else if(inc < 0)
    counter = 0;

  return counter;
}
