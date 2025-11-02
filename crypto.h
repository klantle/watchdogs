#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define CRYPTO_SALT_LEN 16
#define CRYPTO_IV_LEN 16
#define CRYPTO_KEY_LEN 32
#define SHA256_DIGEST_LENGTH 32
#define SHA256_BLOCK_SIZE 64
#define AES_BLOCK_SIZE 16

/* SHA-256 context structure */
typedef struct {
        uint32_t state[8];
        uint64_t count;
        uint8_t buffer[SHA256_BLOCK_SIZE];
} SHA256_CTX;

/* AES context structure (simplified) */
typedef struct {
        uint32_t rd_key[60];
        int rounds;
} AES_KEY;

/* Crypto functions */
void OPENSSL_cleanse(void *ptr, size_t len);

/* SHA-256 functions */
void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t digest[SHA256_DIGEST_LENGTH]);
int sha256_hash(const char *input, unsigned char output[SHA256_DIGEST_LENGTH]);

/* Base64 functions */
char *base64_encode(const unsigned char *input, int len);
unsigned char *base64_decode(const char *input, int *out_len);

/* PBKDF2 function */
int derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
                      int salt_len, unsigned char *key, int key_len);

/* Encryption/Decryption functions */
int encrypt_with_password(const unsigned char *plaintext, int plaintext_len,
                         const char *passphrase, unsigned char **out_blob,
                         int *out_blob_len);
int decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
                         const char *passphrase, unsigned char **out_plain,
                         int *out_plain_len);

/* Utility functions */
int to_hex(const unsigned char *in, int in_len, char **out);

/* Internal functions (for unit testing or advanced use) */
void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *data, size_t data_len,
                 uint8_t digest[SHA256_DIGEST_LENGTH]);

void sha256_transform(SHA256_CTX *ctx, const uint8_t data[SHA256_BLOCK_SIZE]);

/* Random number generation */
void simple_rand_bytes(uint8_t *buf, size_t len);

/* AES functions (simplified - need proper implementation) */
void aes_encrypt(const uint8_t in[AES_BLOCK_SIZE], 
                 uint8_t out[AES_BLOCK_SIZE], 
                 const AES_KEY *key);
void aes_decrypt(const uint8_t in[AES_BLOCK_SIZE], 
                 uint8_t out[AES_BLOCK_SIZE], 
                 const AES_KEY *key);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_H */