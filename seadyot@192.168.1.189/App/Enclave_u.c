#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_create_keys_t {
	unsigned char* ms_p_n;
} ms_create_keys_t;

typedef struct ms_import_message_t {
	unsigned char* ms_message;
} ms_import_message_t;

typedef struct ms_set_public_key_t {
	unsigned char* ms_module;
} ms_set_public_key_t;

typedef struct ms_dispatch_t {
	int ms_retval;
	unsigned char* ms_result;
} ms_dispatch_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_str;
} ms_ocall_print_string_t;

static sgx_status_t SGX_CDECL Enclave_ocall_print_string(void* pms)
{
	ms_ocall_print_string_t* ms = SGX_CAST(ms_ocall_print_string_t*, pms);
	ocall_print_string(ms->ms_str);

	return SGX_SUCCESS;
}

static const struct {
	size_t nr_ocall;
	void * table[1];
} ocall_table_Enclave = {
	1,
	{
		(void*)Enclave_ocall_print_string,
	}
};
sgx_status_t create_keys(sgx_enclave_id_t eid, unsigned char* p_n)
{
	sgx_status_t status;
	ms_create_keys_t ms;
	ms.ms_p_n = p_n;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t import_message(sgx_enclave_id_t eid, unsigned char* message)
{
	sgx_status_t status;
	ms_import_message_t ms;
	ms.ms_message = message;
	status = sgx_ecall(eid, 1, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t set_public_key(sgx_enclave_id_t eid, unsigned char* module)
{
	sgx_status_t status;
	ms_set_public_key_t ms;
	ms.ms_module = module;
	status = sgx_ecall(eid, 2, &ocall_table_Enclave, &ms);
	return status;
}

sgx_status_t dispatch(sgx_enclave_id_t eid, int* retval, unsigned char* result)
{
	sgx_status_t status;
	ms_dispatch_t ms;
	ms.ms_result = result;
	status = sgx_ecall(eid, 3, &ocall_table_Enclave, &ms);
	if (status == SGX_SUCCESS && retval) *retval = ms.ms_retval;
	return status;
}

