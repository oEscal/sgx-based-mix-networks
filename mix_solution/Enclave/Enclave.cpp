/***
 * Encryption related code adapted from the one found at:
 * https://community.intel.com/t5/Intel-Software-Guard-Extensions/Asymmetric-cryptography-in-Enclave/m-p/1196902
 ***/


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

const int WATER_MARK = 100;
const float PROBABILITY_FAN_OUT = 0.01;


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

float generate_random_value() {
    unsigned int random_value;
    sgx_read_rand((unsigned char *) &random_value, sizeof(unsigned int));
    return (float)random_value / (float)UINT_MAX;
}

int dispatch(unsigned char *result, int *fan_out, size_t *buffer_size, int fan_all_out) {

    unsigned char *message;
    float probability_false = (WATER_MARK - (float)buffer.size()) / (float)WATER_MARK;

    // calculate the probability of fan out
    if (fan_all_out || generate_random_value() < PROBABILITY_FAN_OUT)
        *fan_out = 1;

    
    if (!*fan_out && probability_false > 0 && generate_random_value() < probability_false) {             // create a false message
        message = (unsigned char *) MESSAGE_FALSE;
        *buffer_size = buffer.size();
    } else {                                                                                                            // obtain a message from the buffer
        *buffer_size = buffer.size();

        if (buffer.size() < 1)
            return -1;

        int index = (int)(generate_random_value()*buffer.size());
        char *choosen_message = (char *) buffer.at(index).c_str();
        message = (unsigned char *)malloc(strlen(choosen_message));
        std::copy(choosen_message, choosen_message + strlen(choosen_message), message);
        message[strlen(choosen_message)] = '\0';

                                                                                              
        buffer.erase(buffer.begin() + index);

        if (*fan_out) {
            std::copy(message, message + strlen((char *) message), result);
            return 1;
        }
    }

    size_t out_len = 0;

    sgx_status_t ret_get_output_len = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, NULL, &out_len, message, strlen((char *) message));

    if (ret_get_output_len != SGX_SUCCESS) {
        printf("Determination of output length failed\n");
        return 0;
    }

    sgx_status_t ret_encrypt = sgx_rsa_pub_encrypt_sha256(
        previous_public_key, result, &out_len, message, strlen((char *) message));

    if (ret_encrypt != SGX_SUCCESS) {
        printf("Encryption failed\n");
        return 0;
    } else {
        printf("Encrypted message with success!\n");
    }

    return 1;
}
