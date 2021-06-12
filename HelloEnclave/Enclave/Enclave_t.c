#include "Enclave_t.h"

#include "sgx_trts.h" /* for sgx_ocalloc, sgx_is_outside_enclave */
#include "sgx_lfence.h" /* for sgx_lfence */

#include <errno.h>
#include <mbusafecrt.h> /* for memcpy_s etc */
#include <stdlib.h> /* for malloc/free etc */

#define CHECK_REF_POINTER(ptr, siz) do {	\
	if (!(ptr) || ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_UNIQUE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_outside_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define CHECK_ENCLAVE_POINTER(ptr, siz) do {	\
	if ((ptr) && ! sgx_is_within_enclave((ptr), (siz)))	\
		return SGX_ERROR_INVALID_PARAMETER;\
} while (0)

#define ADD_ASSIGN_OVERFLOW(a, b) (	\
	((a) += (b)) < (b)	\
)


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
	int* ms_fan_out;
	size_t* ms_buffer_size;
	int ms_fan_all_out;
} ms_dispatch_t;

typedef struct ms_ocall_print_string_t {
	const char* ms_str;
} ms_ocall_print_string_t;

static sgx_status_t SGX_CDECL sgx_create_keys(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_create_keys_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_create_keys_t* ms = SGX_CAST(ms_create_keys_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	unsigned char* _tmp_p_n = ms->ms_p_n;
	size_t _len_p_n = 256;
	unsigned char* _in_p_n = NULL;

	CHECK_UNIQUE_POINTER(_tmp_p_n, _len_p_n);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_p_n != NULL && _len_p_n != 0) {
		if ( _len_p_n % sizeof(*_tmp_p_n) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_p_n = (unsigned char*)malloc(_len_p_n)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_p_n, 0, _len_p_n);
	}

	create_keys(_in_p_n);
	if (_in_p_n) {
		if (memcpy_s(_tmp_p_n, _len_p_n, _in_p_n, _len_p_n)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_p_n) free(_in_p_n);
	return status;
}

static sgx_status_t SGX_CDECL sgx_import_message(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_import_message_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_import_message_t* ms = SGX_CAST(ms_import_message_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	unsigned char* _tmp_message = ms->ms_message;
	size_t _len_message = 256;
	unsigned char* _in_message = NULL;

	CHECK_UNIQUE_POINTER(_tmp_message, _len_message);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_message != NULL && _len_message != 0) {
		if ( _len_message % sizeof(*_tmp_message) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_message = (unsigned char*)malloc(_len_message);
		if (_in_message == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_message, _len_message, _tmp_message, _len_message)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	import_message(_in_message);

err:
	if (_in_message) free(_in_message);
	return status;
}

static sgx_status_t SGX_CDECL sgx_set_public_key(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_set_public_key_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_set_public_key_t* ms = SGX_CAST(ms_set_public_key_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	unsigned char* _tmp_module = ms->ms_module;
	size_t _len_module = 256;
	unsigned char* _in_module = NULL;

	CHECK_UNIQUE_POINTER(_tmp_module, _len_module);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_module != NULL && _len_module != 0) {
		if ( _len_module % sizeof(*_tmp_module) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		_in_module = (unsigned char*)malloc(_len_module);
		if (_in_module == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		if (memcpy_s(_in_module, _len_module, _tmp_module, _len_module)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}

	}

	set_public_key(_in_module);

err:
	if (_in_module) free(_in_module);
	return status;
}

static sgx_status_t SGX_CDECL sgx_dispatch(void* pms)
{
	CHECK_REF_POINTER(pms, sizeof(ms_dispatch_t));
	//
	// fence after pointer checks
	//
	sgx_lfence();
	ms_dispatch_t* ms = SGX_CAST(ms_dispatch_t*, pms);
	sgx_status_t status = SGX_SUCCESS;
	unsigned char* _tmp_result = ms->ms_result;
	size_t _len_result = 256;
	unsigned char* _in_result = NULL;
	int* _tmp_fan_out = ms->ms_fan_out;
	size_t _len_fan_out = 4;
	int* _in_fan_out = NULL;
	size_t* _tmp_buffer_size = ms->ms_buffer_size;
	size_t _len_buffer_size = 8;
	size_t* _in_buffer_size = NULL;

	CHECK_UNIQUE_POINTER(_tmp_result, _len_result);
	CHECK_UNIQUE_POINTER(_tmp_fan_out, _len_fan_out);
	CHECK_UNIQUE_POINTER(_tmp_buffer_size, _len_buffer_size);

	//
	// fence after pointer checks
	//
	sgx_lfence();

	if (_tmp_result != NULL && _len_result != 0) {
		if ( _len_result % sizeof(*_tmp_result) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_result = (unsigned char*)malloc(_len_result)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_result, 0, _len_result);
	}
	if (_tmp_fan_out != NULL && _len_fan_out != 0) {
		if ( _len_fan_out % sizeof(*_tmp_fan_out) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_fan_out = (int*)malloc(_len_fan_out)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_fan_out, 0, _len_fan_out);
	}
	if (_tmp_buffer_size != NULL && _len_buffer_size != 0) {
		if ( _len_buffer_size % sizeof(*_tmp_buffer_size) != 0)
		{
			status = SGX_ERROR_INVALID_PARAMETER;
			goto err;
		}
		if ((_in_buffer_size = (size_t*)malloc(_len_buffer_size)) == NULL) {
			status = SGX_ERROR_OUT_OF_MEMORY;
			goto err;
		}

		memset((void*)_in_buffer_size, 0, _len_buffer_size);
	}

	ms->ms_retval = dispatch(_in_result, _in_fan_out, _in_buffer_size, ms->ms_fan_all_out);
	if (_in_result) {
		if (memcpy_s(_tmp_result, _len_result, _in_result, _len_result)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_fan_out) {
		if (memcpy_s(_tmp_fan_out, _len_fan_out, _in_fan_out, _len_fan_out)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}
	if (_in_buffer_size) {
		if (memcpy_s(_tmp_buffer_size, _len_buffer_size, _in_buffer_size, _len_buffer_size)) {
			status = SGX_ERROR_UNEXPECTED;
			goto err;
		}
	}

err:
	if (_in_result) free(_in_result);
	if (_in_fan_out) free(_in_fan_out);
	if (_in_buffer_size) free(_in_buffer_size);
	return status;
}

SGX_EXTERNC const struct {
	size_t nr_ecall;
	struct {void* ecall_addr; uint8_t is_priv; uint8_t is_switchless;} ecall_table[4];
} g_ecall_table = {
	4,
	{
		{(void*)(uintptr_t)sgx_create_keys, 0, 0},
		{(void*)(uintptr_t)sgx_import_message, 0, 0},
		{(void*)(uintptr_t)sgx_set_public_key, 0, 0},
		{(void*)(uintptr_t)sgx_dispatch, 0, 0},
	}
};

SGX_EXTERNC const struct {
	size_t nr_ocall;
	uint8_t entry_table[1][4];
} g_dyn_entry_table = {
	1,
	{
		{0, 0, 0, 0, },
	}
};


sgx_status_t SGX_CDECL ocall_print_string(const char* str)
{
	sgx_status_t status = SGX_SUCCESS;
	size_t _len_str = str ? strlen(str) + 1 : 0;

	ms_ocall_print_string_t* ms = NULL;
	size_t ocalloc_size = sizeof(ms_ocall_print_string_t);
	void *__tmp = NULL;


	CHECK_ENCLAVE_POINTER(str, _len_str);

	if (ADD_ASSIGN_OVERFLOW(ocalloc_size, (str != NULL) ? _len_str : 0))
		return SGX_ERROR_INVALID_PARAMETER;

	__tmp = sgx_ocalloc(ocalloc_size);
	if (__tmp == NULL) {
		sgx_ocfree();
		return SGX_ERROR_UNEXPECTED;
	}
	ms = (ms_ocall_print_string_t*)__tmp;
	__tmp = (void *)((size_t)__tmp + sizeof(ms_ocall_print_string_t));
	ocalloc_size -= sizeof(ms_ocall_print_string_t);

	if (str != NULL) {
		ms->ms_str = (const char*)__tmp;
		if (_len_str % sizeof(*str) != 0) {
			sgx_ocfree();
			return SGX_ERROR_INVALID_PARAMETER;
		}
		if (memcpy_s(__tmp, ocalloc_size, str, _len_str)) {
			sgx_ocfree();
			return SGX_ERROR_UNEXPECTED;
		}
		__tmp = (void *)((size_t)__tmp + _len_str);
		ocalloc_size -= _len_str;
	} else {
		ms->ms_str = NULL;
	}
	
	status = sgx_ocall(0, ms);

	if (status == SGX_SUCCESS) {
	}
	sgx_ocfree();
	return status;
}

