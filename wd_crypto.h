#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AES_BLOCK_SIZE 16
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

void OPENSSL_cleanse(void *ptr, size_t len);

uint32_t hash_str(const char *s);

int sha256_hash(const char *input, unsigned char output[SHA256_DIGEST_LENGTH]);

char *base64_encode(const unsigned char *input, int len);
unsigned char *base64_decode(const char *input, int *out_len);

int derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
                      int salt_len, unsigned char *key, int key_len);

int encrypt_with_password(const unsigned char *plaintext, int plaintext_len,
                         const char *passphrase, unsigned char **out_blob,
                         int *out_blob_len);
int decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
                         const char *passphrase, unsigned char **out_plain,
                         int *out_plain_len);

int to_hex(const unsigned char *in, int in_len, char **out);

void aes_encrypt(const uint8_t in[AES_BLOCK_SIZE],
                 uint8_t out[AES_BLOCK_SIZE],
                 const AES_KEY *key);
void aes_decrypt(const uint8_t in[AES_BLOCK_SIZE],
                 uint8_t out[AES_BLOCK_SIZE],
                 const AES_KEY *key);

#ifdef __cplusplus
}
#endif

#endif
