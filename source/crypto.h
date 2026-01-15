/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AES_BLOCK_SIZE 16
#define SHA1_DIGEST_SIZE 20
#define SHA1_BLOCK_SIZE 64
#define CRYPTO_SALT_LEN 16
#define CRYPTO_IV_LEN 16
#define CRYPTO_KEY_LEN 32
#define SHA256_DIGEST_LENGTH 32
#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

typedef struct {
        uint32_t state[8];
        uint64_t count;
        uint8_t buffer[SHA256_BLOCK_SIZE];
} SHA256_CTX;

typedef struct {
        uint32_t rd_key[60];
        int rounds;
} AES_KEY;
typedef struct {
        uint32_t state[5];
        uint32_t count[2];
        uint8_t buffer[64];
} SHA1_CTX;

uint32_t rotr(uint32_t x, int n);
uint32_t ch(uint32_t x, uint32_t y, uint32_t z);
uint32_t maj(uint32_t x, uint32_t y, uint32_t z);
uint32_t sigma(uint32_t x);
uint32_t sigma2(uint32_t x);

void charset_random(char *buffer, size_t size);

void crypto_crc32_init_table(void);

uint32_t crypto_generate_crc32(const void *data, size_t length);

uint32_t crypto_string_hash(const char *s);

unsigned long crypto_djb2_hash_file(const char *filename);

void crypto_simple_rand_bytes(uint8_t *buf, size_t len);

void crypto_print_hex(const unsigned char *buf, size_t len, int null_terminator);

int crypto_convert_to_hex(const unsigned char *in, int in_len, char **out);

int crypto_generate_sha1_hash(const char *input, unsigned char output[20]);

int crypto_generate_sha256_hash(const char *input, unsigned char output[SHA256_DIGEST_LENGTH]);

char *crypto_base64_encode(const unsigned char *input, int len);
unsigned char *crypto_base64_decode(const char *input, int *out_len);

int crypto_derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
                      long salt_len, unsigned char *key, int key_len);

int crypto_encrypt_with_password(const unsigned char *plaintext, int plaintext_len,
                         const char *passphrase, unsigned char **out_blob,
                         int *out_blob_len);
int crypto_decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
                         const char *passphrase, unsigned char **out_plain,
                         long *out_plain_len);

void crypto_aes_encrypt(const uint8_t in[AES_BLOCK_SIZE],
                 uint8_t out[AES_BLOCK_SIZE],
                 const AES_KEY *key);
void crypto_aes_decrypt(const uint8_t in[AES_BLOCK_SIZE],
                 uint8_t out[AES_BLOCK_SIZE],
                 const AES_KEY *key);

#ifdef __cplusplus
}
#endif

#endif
