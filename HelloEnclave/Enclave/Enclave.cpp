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
#include <string.h>
#include <string>
#include <climits>

#include "Enclave.h"
#include "Enclave_t.h"  /* print_string */

#include "sgx_trts.h"
#include "sgx_tcrypto.h"


long e = 65537;
void *private_key = NULL;
void *previous_public_key = NULL;
std::vector<std::string> buffer;

const char *MESSAGE_FALSE = "False";


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

    char content_compare[strlen(MESSAGE_FALSE) + 1];
    std::copy(decrypted_pout_data, decrypted_pout_data + strlen(MESSAGE_FALSE), content_compare);
    content_compare[strlen(MESSAGE_FALSE)] = '\0';

    if (strcmp(content_compare, MESSAGE_FALSE) != 0) {
        std::string decrypted_str = std::string((char *) decrypted_pout_data);
        
        int current_op_index = decrypted_str.find(':') + 1;
        char current_op_str[decrypted_str.length() - current_op_index + 1];

        decrypted_str.copy(current_op_str, decrypted_str.length() - current_op_index, current_op_index);
        current_op_str[decrypted_str.length() - current_op_index] = '\0';

        char message_without_op[current_op_index + 1];
        decrypted_str.copy(message_without_op, current_op_index);
        message_without_op[current_op_index] = '\0';

        int current_op = std::stoi(current_op_str) + 1;
        buffer.push_back(std::string(message_without_op) + std::to_string(current_op));
    } 

    printf("Current buffer size: %d\n", buffer.size());
}

// for (int i = 0; i < strlen(MESSAGE_FALSE) + 1; i++) {
    // 	printf("%d ", (int)content_compare[i]);
    // }
    // printf("\n%s\n", content_compare);

float generate_random_value() {
    unsigned int random_value;
    sgx_read_rand((unsigned char *) &random_value, sizeof(unsigned int));
    return (float)random_value / (float)UINT_MAX;
}

const int WATER_MARK = 15;
const float PROBABILITY_FAN_OUT = 0.01;

void dispatch(unsigned char *result, int *fan_out, size_t *buffer_size, int fan_all_out) {

    unsigned char *message;
    float probability_false = (WATER_MARK - (float)buffer.size()) / (float)WATER_MARK;
    *fan_out = 0;
    
    if (probability_false > 0 && generate_random_value() < probability_false && !fan_all_out) {             // create a false message
        message = (unsigned char *)"False";
        *buffer_size = buffer.size();
    } else {                                                                                                // obtain a message from the buffer
        int index = (int)(generate_random_value()*buffer.size());
        char *choosen_message = (char *) buffer.at(index).c_str();
        message = (unsigned char *)malloc(strlen(choosen_message));
        std::copy(choosen_message, choosen_message + strlen(choosen_message), message);
        message[strlen(choosen_message)] = '\0';

                                                                                              
        buffer.erase(buffer.begin() + index);

        *buffer_size = buffer.size();

        if (fan_all_out || generate_random_value() < PROBABILITY_FAN_OUT) {
            *fan_out = 1;
            std::copy(message, message + strlen((char *) message), result);
            return;
        }
    }

    size_t out_len = 0;

    sgx_status_t ret_get_output_len = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, NULL, &out_len, message, strlen((char *) message));

    if ( ret_get_output_len != SGX_SUCCESS) {
        printf("Determination of output length failed\n");
        return;
    }

    sgx_status_t ret_encrypt = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, result, &out_len, message, strlen((char *) message));

    if ( ret_encrypt != SGX_SUCCESS) {
        printf("Encryption failed\n");
        return;
    } else {
        printf("Encrypted message with success!\n");
    }
}
