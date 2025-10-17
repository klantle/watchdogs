#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

void encrypt(const uint8_t *input, uint8_t *output, size_t len, const char *passphrase);
void decrypt(const uint8_t *input, uint8_t *output, size_t len, const char *passphrase);
void sha256_hash(const char *input, unsigned char output[32]);
void print_hash_hex(const unsigned char hash[32]);
char* base64_encode(const unsigned char *input, int len);

#endif
