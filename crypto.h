#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRYPTO_SALT_LEN 16
#define CRYPTO_IV_LEN 16
#define CRYPTO_KEY_LEN 32
#define SHA256_DIGEST_LENGTH (32)

void OPENSSL_cleanse(void *ptr, size_t len);
int sha256_hash(const char *input, unsigned char output[32]);
char *base64_encode(const unsigned char *input, int len);
unsigned char *base64_decode(const char *input, int *out_len);
int encrypt_with_password(const unsigned char *plaintext, int plaintext_len, const char *passphrase, unsigned char **out_blob, int *out_blob_len);
int decrypt_with_password(const unsigned char *in_blob, int in_blob_len, const char *passphrase, unsigned char **out_plain, int *out_plain_len);
int to_hex(const unsigned char *in, int in_len, char **out);

#ifdef __cplusplus
}
#endif

#endif
