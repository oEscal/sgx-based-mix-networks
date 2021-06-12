#ifndef ENCLAVE_U_H__
#define ENCLAVE_U_H__

#include <stdint.h>
#include <wchar.h>
#include <stddef.h>
#include <string.h>
#include "sgx_edger8r.h" /* for sgx_status_t etc. */


#include <stdlib.h> /* for size_t */

#define SGX_CAST(type, item) ((type)(item))

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OCALL_PRINT_STRING_DEFINED__
#define OCALL_PRINT_STRING_DEFINED__
void SGX_UBRIDGE(SGX_NOCONVENTION, ocall_print_string, (const char* str));
#endif

sgx_status_t create_keys(sgx_enclave_id_t eid, unsigned char* p_n);
sgx_status_t import_message(sgx_enclave_id_t eid, unsigned char* message);
sgx_status_t set_public_key(sgx_enclave_id_t eid, unsigned char* module);
sgx_status_t dispatch(sgx_enclave_id_t eid, int* retval, unsigned char* result, int* fan_out, size_t* buffer_size, int fan_all_out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
