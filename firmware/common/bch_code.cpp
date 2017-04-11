/*
 * Copyright (C) 2015 Craig Shelley (craig@microtron.org.uk)
 * Copyright (C) 2016 Furrtek
 *
 * BCH Encoder/Decoder - Adapted from GNURadio
 * 
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <math.h>
#include <stdlib.h>
#include <ch.h>

#include "bch_code.hpp"

void BCHCode::generate_gf() {
	/*
	* generate GF(2**m) from the irreducible polynomial p(X) in p[0]..p[m]
	* lookup tables:  index->polynomial form   alpha_to[] contains j=alpha**i;
	* polynomial form -> index form  index_of[j=alpha**i] = i alpha=2 is the
	* primitive element of GF(2**m) 
	*/
	
	int i, mask;
	mask = 1;
	alpha_to[m] = 0;
	
	for (i = 0; i < m; i++) 
	{
		alpha_to[i] = mask;
		
		index_of[alpha_to[i]] = i;
		
		if (p[i] != 0)
			alpha_to[m] ^= mask;
		
		mask <<= 1;
	}

	index_of[alpha_to[m]] = m;
	
	mask >>= 1;
	
	for (i = m + 1; i < n; i++) 
	{
		if (alpha_to[i - 1] >= mask)
		  alpha_to[i] = alpha_to[m] ^ ((alpha_to[i - 1] ^ mask) << 1);
		else
		  alpha_to[i] = alpha_to[i - 1] << 1;
	
		index_of[alpha_to[i]] = i;
	}
	
	index_of[0] = -1;
}


void BCHCode::gen_poly() {
	/* 
	* Compute generator polynomial of BCH code of length = 31, redundancy = 10
	* (OK, this is not very efficient, but we only do it once, right? :)
	*/

	int ii, jj, ll, kaux;
	int test, aux, nocycles, root, noterms, rdncy;
	int cycle[15][6], size[15], min[11], zeros[11];
	
	// Generate cycle sets modulo 31 
	cycle[0][0] = 0; size[0] = 1;
	cycle[1][0] = 1; size[1] = 1;
	jj = 1;			// cycle set index 
	
	do 
	{
		// Generate the jj-th cycle set 
		ii = 0;
		do 
		{
			ii++;
			cycle[jj][ii] = (cycle[jj][ii - 1] * 2) % n;
			size[jj]++;
			aux = (cycle[jj][ii] * 2) % n;

		} while (aux != cycle[jj][0]);
		
		// Next cycle set representative 
		ll = 0;
		do 
		{
			ll++;
			test = 0;
			for (ii = 1; ((ii <= jj) && (!test)); ii++)	
			// Examine previous cycle sets 
			  for (kaux = 0; ((kaux < size[ii]) && (!test)); kaux++)
					if (ll == cycle[ii][kaux])
						test = 1;
		} 
		while ((test) && (ll < (n - 1)));
		
		if (!(test)) 
		{
			jj++;	// next cycle set index 
			cycle[jj][0] = ll;
			size[jj] = 1;
		}

	} while (ll < (n - 1));
	
	nocycles = jj;		// number of cycle sets modulo n 
	// Search for roots 1, 2, ..., d-1 in cycle sets 
	
	kaux = 0;
	rdncy = 0;
	
	for (ii = 1; ii <= nocycles; ii++) 
	{
		min[kaux] = 0;
		
		for (jj = 0; jj < size[ii]; jj++)
			for (root = 1; root < d; root++)
				if (root == cycle[ii][jj])
					min[kaux] = ii;
		
		if (min[kaux]) 
		{
			rdncy += size[min[kaux]];
			kaux++;
		}
	}

	noterms = kaux;
	kaux = 1;
	
	for (ii = 0; ii < noterms; ii++)
		for (jj = 0; jj < size[min[ii]]; jj++) 
		{
			zeros[kaux] = cycle[min[ii]][jj];
			kaux++;
		}
	
	// Compute generator polynomial 
	g[0] = alpha_to[zeros[1]];
	g[1] = 1;		// g(x) = (X + zeros[1]) initially 
	
	for (ii = 2; ii <= rdncy; ii++) 
	{
	  g[ii] = 1;
	  for (jj = ii - 1; jj > 0; jj--)
	    if (g[jj] != 0)
	      g[jj] = g[jj - 1] ^ alpha_to[(index_of[g[jj]] + zeros[ii]) % n];
	    else
	      g[jj] = g[jj - 1];
	  
		g[0] = alpha_to[(index_of[g[0]] + zeros[ii]) % n];
	}
}

int * BCHCode::encode(int data[]) {
	// Calculate redundant bits bb[]

	int    h, i, j=0, start=0, end=(n-k);	// 10
	int Mr[31];
	
	if (!valid) return nullptr;
	
	for (i = 0; i < n; i++) {
		Mr[i] = 0;
	}
	
	for (h = 0; h < k; ++h)
		Mr[h] = data[h];
	
	while (end < n)
	{
		for (i=end; i>start-2; --i)
		{
			if (Mr[start] != 0)
			{
				Mr[i]^=g[j];
				++j;
			}
			else
			{
				++start;
				j = 0;
				end = start+(n-k);
				break;
			}
		}
	}
	
	j = 0;
	for (i = start; i<end; ++i) {
		bb[j]=Mr[i];
		++j;
	}
	
	return bb;
};


int BCHCode::decode(int recd[]) {
	// We do not need the Berlekamp algorithm to decode.
	// We solve before hand two equations in two variables.

	int i, j, q;
	int elp[3], s[5], s3;
	int count = 0, syn_error = 0;
	int loc[3], reg[3];
	int	aux;
	int retval=0;
	
	if (!valid) return 0;
	
	for (i = 1; i <= 4; i++) {
		s[i] = 0;
		for (j = 0; j < n; j++) {
			if (recd[j] != 0) {
				s[i] ^= alpha_to[(i * j) % n];
			}
		}
		if (s[i] != 0) {
			syn_error = 1;		// set flag if non-zero syndrome
		}
		/* NOTE: If only error detection is needed,
		 * then exit the program here...
		 */
		// Convert syndrome from polynomial form to index form
		s[i] = index_of[s[i]];
	};
	
	if (syn_error) {			// If there are errors, try to correct them
		if (s[1] != -1) {
			s3 = (s[1] * 3) % n;
			if ( s[3] == s3 ) { 	// Was it a single error ?
				//printf("One error at %d\n", s[1]);
				recd[s[1]] ^= 1; 	// Yes: Correct it
			} else {
				/* Assume two errors occurred and solve
				 * for the coefficients of sigma(x), the
				 * error locator polynomial
				 */
				if (s[3] != -1) {
					aux = alpha_to[s3] ^ alpha_to[s[3]];
				} else {
					aux = alpha_to[s3];
				}
				elp[0] = 0;
				elp[1] = (s[2] - index_of[aux] + n) % n;
				elp[2] = (s[1] - index_of[aux] + n) % n;
				//printf("sigma(x) = ");
				//for (i = 0; i <= 2; i++) {
				//	printf("%3d ", elp[i]);
				//}
				//printf("\n");
				//printf("Roots: ");
				
				// Find roots of the error location polynomial
				for (i = 1; i <= 2; i++) {
					reg[i] = elp[i];
				}
				count = 0;
				for (i = 1; i <= n; i++) { // Chien search
					q = 1;
					for (j = 1; j <= 2; j++) {
						if (reg[j] != -1) {
							reg[j] = (reg[j] + j) % n;
							q ^= alpha_to[reg[j]];
						}
					}
					if (!q) {	// store error location number indices
						loc[count] = i % n;
						count++;
					}
				}
				
				if (count == 2)	{
					// no. roots = degree of elp hence 2 errors
					for (i = 0; i < 2; i++)
						recd[loc[i]] ^= 1;
				} else {	// Cannot solve: Error detection
					retval=1;
				}
			}
		} else if (s[2] != -1) {	// Error detection
			retval=1;
		}
	}

	return retval;
}

/*
 * Example usage BCH(31,21,5)
 *
 * p[] = coefficients of primitive polynomial used to generate GF(2**5)
 * m = order of the field GF(2**5) = 5
 * n = 2**5 - 1 = 31
 * t = 2 = error correcting capability
 * d = 2*t + 1 = 5 = designed minimum distance
 * k = n - deg(g(x)) = 21 = dimension
 * g[] = coefficients of generator polynomial, g(x) [n - k + 1]=[11]
 * alpha_to [] = log table of GF(2**5)
 * index_of[] = antilog table of GF(2**5)
 * data[] = coefficients of data polynomial, i(x)
 * bb[] = coefficients of redundancy polynomial ( x**(10) i(x) ) modulo g(x)
 */
BCHCode::BCHCode(
	std::vector<int> p_init, int m, int n, int k, int t
) : m { m },
	n { n },
	k { k },
	t { t } {
	size_t i;
	
	d = 5;
	
	alpha_to = (int *)chHeapAlloc(NULL, sizeof(int) * (n + 1));
	index_of = (int *)chHeapAlloc(0, sizeof(int) * (n + 1));
	p = (int *)chHeapAlloc(0, sizeof(int) * (m + 1));
	g = (int *)chHeapAlloc(0, sizeof(int) * (n - k + 1));
	bb = (int *)chHeapAlloc(0, sizeof(int) * (n - k + 1));
	
	if (alpha_to == NULL ||
		index_of == NULL ||
		p == NULL ||
		g == NULL ||
		bb == NULL)
		valid = false;
	else
		valid = true;
	
	if (valid) {
		for (i = 0; i < (size_t)(m + 1); i++) {
			p[i] = p_init[i];
		}

		generate_gf();			/* generate the Galois Field GF(2**m) */
		gen_poly();				/* Compute the generator polynomial of BCH code */
	}
}

BCHCode::~BCHCode() {
	if (alpha_to != NULL) chHeapFree(alpha_to);
	if (index_of != NULL) chHeapFree(index_of);
	if (p != NULL) chHeapFree(p);
	if (g != NULL) chHeapFree(g);
	if (bb != NULL) chHeapFree(bb);
}
