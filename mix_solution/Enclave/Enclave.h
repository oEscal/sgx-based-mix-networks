#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

void encrypt(unsigned char *p_n);
void import_message(unsigned char *message);
void set_public_key(unsigned char *module);
int dispatch(unsigned char *result, int *fan_out, size_t *buffer_size, int fan_all_out);
void printf(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* !_ENCLAVE_H_ */
