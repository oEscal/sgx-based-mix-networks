/*
 * Copyright (C) 2011-2018 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <cstring>

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

#include "sgx_tcrypto.h"


void encrypt() {
    /*
    unsigned char p_n[256];
    unsigned char p_d[256];
    unsigned char p_p[256];
    unsigned char p_q[256];
    unsigned char p_dmp1[256];
    unsigned char p_dmq1[256];
    unsigned char p_iqmp[256];

    int n_byte_size = 256;
    int e_byte_size = 4;
    long e = 65537;

    sgx_status_t se_ret = SGX_SUCCESS;

    void *private_key[256];

    se_ret = sgx_create_rsa_key_pair(
        n_byte_size, e_byte_size, p_n, p_d, (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp);

    if(se_ret != SGX_SUCCESS)
    {
        return;
    }

    se_ret = sgx_create_rsa_priv2_key(
        n_byte_size, e_byte_size, (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp, private_key);

        if(se_ret != SGX_SUCCESS)
    {
        return;
    }

	char *data = "ola";
    unsigned char *dst = NULL;

	void *public_key = NULL;

    // sgx_create_rsa_key_pair
	size_t pub_key_size = 256;

    // sgx_status_t ret_create_public_key = sgx_create_rsa_pub1_key(n_byte_size, e_byte_size, p_n, (unsigned char*)&e, pub_key);
	// se_ret = sgx_create_rsa_pub1_key(sizeof(module),
    //                              sizeof(exponent), module, exponent, pub_key);
    se_ret = sgx_create_rsa_pub1_key(n_byte_size, e_byte_size, p_n, (unsigned char*)&e, &public_key);

    if(se_ret != SGX_SUCCESS)
    {
        return;
    }

    // printf("%s\n", (char*)public_key);

	// se_ret = sgx_rsa_pub_encrypt_sha256(public_key, NULL, &pub_key_size, data, sizeof(data));
    // // printf("%zu\n", pub_key_size);
    // if(SGX_SUCCESS != se_ret)
    // {
    //     return;
    //     // return se_ret;
    // }

    se_ret = sgx_rsa_pub_encrypt_sha256(public_key, dst, &pub_key_size, (unsigned char *)data, sizeof(data));
    printf("%s", dst);
    if(SGX_SUCCESS != se_ret)
    {
        printf("%d\n", (int)se_ret);
        return;
        // return se_ret;
    }
    */

    unsigned char p_n[256];
    unsigned char p_d[256];
    unsigned char p_p[256];
    unsigned char p_q[256];
    unsigned char p_dmp1[256];
    unsigned char p_dmq1[256];
    unsigned char p_iqmp[256];

    int n_byte_size = 256;
    long e = 65537;


    sgx_status_t ret_create_key_params = sgx_create_rsa_key_pair(n_byte_size, sizeof(e), p_n, p_d, (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp);

    if (ret_create_key_params != SGX_SUCCESS) {
        printf("Key param generation failed");
    } else {
        // printf((char *) p_q);
    }

    void *private_key = NULL;

    sgx_status_t ret_create_private_key = sgx_create_rsa_priv2_key(n_byte_size, sizeof(e), (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp, &private_key);

    if ( ret_create_private_key != SGX_SUCCESS) {
        printf("Private key generation failed");
    }

    void *public_key = NULL;

    sgx_status_t ret_create_public_key = sgx_create_rsa_pub1_key(n_byte_size, sizeof(e), p_n, (unsigned char*)&e, &public_key);

    if ( ret_create_public_key != SGX_SUCCESS) {
        printf("Public key generation failed");
    }

    char * pin_data = "Hello World!";
    size_t out_len = 0;

    sgx_status_t ret_get_output_len = sgx_rsa_pub_encrypt_sha256(public_key, NULL, &out_len, (unsigned char *)pin_data, strlen(pin_data));

    if ( ret_get_output_len != SGX_SUCCESS) {
        printf("Determination of output length failed");
    }

    unsigned char pout_data[out_len];

    sgx_status_t ret_encrypt = sgx_rsa_pub_encrypt_sha256(public_key, pout_data, &out_len, (unsigned char *)pin_data, strlen(pin_data));

    if ( ret_encrypt != SGX_SUCCESS) {
        printf("Encryption failed");
    } else {
    }

    size_t decrypted_out_len = 0;

    sgx_status_t ret_determine_decrypt_len = sgx_rsa_priv_decrypt_sha256(private_key, NULL, &decrypted_out_len, pout_data, sizeof(pout_data));

    if ( ret_determine_decrypt_len != SGX_SUCCESS) {
        printf("Determination of decrypted output length failed");
    }

    unsigned char decrypted_pout_data[decrypted_out_len];

    sgx_status_t ret_decrypt = sgx_rsa_priv_decrypt_sha256(private_key, decrypted_pout_data, &decrypted_out_len, pout_data, sizeof(pout_data));

    if ( ret_decrypt != SGX_SUCCESS) {
        printf("Decryption failed");
    } else {
        printf("Decrypted MESSAGE:");
        printf("%s\n", (char *)decrypted_pout_data);
    }

}

/* 
 * printf: 
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void printf_helloworld()
{
    encrypt();
    // printf("Hello World\n");
}

