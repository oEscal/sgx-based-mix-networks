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
#include <vector>
#include <climits>

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

#include "sgx_trts.h"
#include "sgx_tcrypto.h"


long e = 65537;
void *private_key = NULL;
void *previous_public_key = NULL;
std::vector<unsigned char *> buffer;


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


int n_byte_size = 256;
void create_keys(unsigned char *p_n) {
    unsigned char p_d[256];
    unsigned char p_p[256];
    unsigned char p_q[256];
    unsigned char p_dmp1[256];
    unsigned char p_dmq1[256];
    unsigned char p_iqmp[256];

    sgx_status_t ret_create_key_params = sgx_create_rsa_key_pair(
        n_byte_size, sizeof(e), p_n, p_d, (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp);

    if (ret_create_key_params != SGX_SUCCESS) {
        printf("Key param generation failed");
    } else {
        // printf((char *) p_q);
    }

    sgx_status_t ret_create_private_key = sgx_create_rsa_priv2_key(
        n_byte_size, sizeof(e), (unsigned char*)&e, p_p, p_q, p_dmp1, p_dmq1, p_iqmp, &private_key);

    if ( ret_create_private_key != SGX_SUCCESS) {
        printf("Private key generation failed");
    }

    // void *public_key = NULL;
// 
    // sgx_status_t ret_create_public_key = sgx_create_rsa_pub1_key(n_byte_size, sizeof(e), p_n, (unsigned char*)&e, &public_key);
// 
    // if ( ret_create_public_key != SGX_SUCCESS) {
    //     printf("Public key generation failed");
    // }
}

void set_public_key(unsigned char *module) {
    sgx_status_t ret_create_public_key = sgx_create_rsa_pub1_key(
        n_byte_size, sizeof(e), module, (unsigned char*)&e, &previous_public_key);
    if ( ret_create_public_key != SGX_SUCCESS) {
        printf("Public key generation failed");
    }
}

void import_message(unsigned char *message) {
    size_t decrypted_out_len = 0;

    sgx_status_t ret_determine_decrypt_len = sgx_rsa_priv_decrypt_sha256(
        private_key, NULL, &decrypted_out_len, message, 256);

    if ( ret_determine_decrypt_len != SGX_SUCCESS) {
        printf("Determination of decrypted output length failed\n");
        return;
    }

    unsigned char decrypted_pout_data[decrypted_out_len];

    sgx_status_t ret_decrypt = sgx_rsa_priv_decrypt_sha256(
        private_key, decrypted_pout_data, &decrypted_out_len, message, 256);

    if (ret_decrypt != SGX_SUCCESS) {
        printf("Decryption failed\n");
        return;
    } else {
        printf("Decrypted message with success!\n"); 
    }

    buffer.push_back(decrypted_pout_data);

    printf("Current buffer size: %d\n", buffer.size());
}

float generate_random_value() {
    unsigned int random_value;
    sgx_read_rand((unsigned char *) &random_value, sizeof(unsigned int));
    return (float)random_value / (float)UINT_MAX;
}

const int WATER_MARK = 100;

void dispatch(unsigned char *result) {

    unsigned char *message;
    float probability_false = (float)(WATER_MARK - buffer.size()) / (float)WATER_MARK;
    
    // create a false message
    if (probability_false > 0 && generate_random_value() < probability_false) {
        message = (unsigned char *) "False";
    } else {
        int index = (int)(generate_random_value()*buffer.size());
        message = buffer.at(index);
        buffer.erase(buffer.begin() + index);
    }

    size_t out_len = 0;

    sgx_status_t ret_get_output_len = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, NULL, &out_len, message, strlen((char *) message));

    if ( ret_get_output_len != SGX_SUCCESS) {
        printf("Determination of output length failed\n");
    }

    sgx_status_t ret_encrypt = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, result, &out_len, message, strlen((char *) message));

    if ( ret_encrypt != SGX_SUCCESS) {
        printf("Encryption failed\n");
    } else {
        printf("Encrypted message with success!\n");
    }
}


/*
void encrypt(unsigned char *p_n) {
    // unsigned char p_n[256];
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
*/