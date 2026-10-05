#ifndef _INCLUDE_LDPC_H_
#define _INCLUDE_LDPC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// codeword is 174 log-likelihoods.
// plain is a return value, 174 ints, to be 0 or 1.
// iters is how hard to try.
// ok == 87 means success.
void ldpc_decode(float codeword[], int max_iters, uint8_t plain[], int* ok);

void bp_decode(float codeword[], int max_iters, uint8_t plain[], int* ok);

#ifdef __cplusplus
}
#endif

#endif // _INCLUDE_LDPC_H_
