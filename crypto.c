/*
 * What is this?
 * ------------------------------------------------------------
 * This C script is a cryptography utility library built on OpenSSL.
 * It provides functions for secure hashing (SHA-256), encoding/decoding (Base64),
 * key derivation from passphrases (PBKDF2-HMAC-SHA256), and symmetric encryption
 * and decryption using AES-256-CBC. It also includes a helper to convert binary data
 * to hexadecimal strings. The library can be used to securely store, transmit, or
 * verify sensitive data such as passwords, messages, or files.
 *
 *
 * Script Algorithm:
 * ------------------------------------------------------------
 * 1. Hashing:
 *    - Use `sha256_hash()` to compute the SHA-256 digest of a string.
 * 2. Base64 Encoding/Decoding:
 *    - Encode binary data into Base64 string using `base64_encode()`.
 *    - Decode Base64 strings back to binary using `base64_decode()`.
 * 3. Key Derivation:
 *    - Generate a cryptographic key from a passphrase and salt using `derive_key_pbkdf2()`.
 *    - Uses PBKDF2-HMAC-SHA256 with 100,000 iterations for security.
 * 4. Encryption:
 *    - `encrypt_with_password()` generates a random salt and IV.
 *    - Derives a 256-bit AES key from the passphrase.
 *    - Encrypts plaintext with AES-256-CBC.
 *    - Produces a blob combining salt + IV + ciphertext for storage/transmission.
 * 5. Decryption:
 *    - `decrypt_with_password()` extracts salt and IV from the blob.
 *    - Re-derives the AES key using the passphrase.
 *    - Decrypts the ciphertext to recover the original plaintext.
 * 6. Binary to Hex Conversion:
 *    - `to_hex()` converts arbitrary binary data into a human-readable hexadecimal string.
 *
 *
 * Script Logic:
 * ------------------------------------------------------------
 * - The library separates concerns: hashing, encoding, key derivation, encryption, decryption.
 * - All functions perform rigorous error checking and memory allocation validation.
 * - Sensitive data (keys, buffers) are cleansed from memory after use to reduce leakage risk.
 * - Encryption combines salt, IV, and ciphertext in a single blob, simplifying secure storage and transport.
 * - Base64 allows binary blobs to be stored or transmitted as text safely.
 *
 *
 * How to Use:
 * ------------------------------------------------------------
 * 1. Hash a string with SHA-256:
 *      unsigned char hash[32];
 *      sha256_hash("message", hash);
 *
 * 2. Encode/Decode Base64:
 *      char *b64 = base64_encode(data, data_len);
 *      int decoded_len;
 *      unsigned char *decoded = base64_decode(b64, &decoded_len);
 *
 * 3. Encrypt data with a passphrase:
 *      unsigned char *encrypted_blob;
 *      int blob_len;
 *      encrypt_with_password(plaintext, plaintext_len, "password", &encrypted_blob, &blob_len);
 *
 * 4. Decrypt data with the same passphrase:
 *      unsigned char *decrypted;
 *      int decrypted_len;
 *      decrypt_with_password(encrypted_blob, blob_len, "password", &decrypted, &decrypted_len);
 *
 * 5. Convert binary to hex string:
 *      char *hexstr;
 *      to_hex(binary_data, binary_len, &hexstr);
 *
 * 6. Always free allocated memory and cleanse sensitive buffers after use.
 *
 * Requirements:
 * - OpenSSL library installed.
 * - Include <openssl/evp.h>, <openssl/sha.h>, <openssl/rand.h>, <openssl/bio.h>, <openssl/buffer.h>, <openssl/err.h>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>

int sha256_hash(const char *input, unsigned char output[32]) {
        if (!input || !output) return 0;
        EVP_MD_CTX *ctx = EVP_MD_CTX_new();
        if (!ctx) return 0;
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) { EVP_MD_CTX_free(ctx); return 0; }
        if (EVP_DigestUpdate(ctx, input, strlen(input)) != 1) { EVP_MD_CTX_free(ctx); return 0; }
        unsigned int outlen = 0;
        if (EVP_DigestFinal_ex(ctx, output, &outlen) != 1 || outlen != 32) { EVP_MD_CTX_free(ctx); return 0; }
        EVP_MD_CTX_free(ctx);
        return 1;
}

char *base64_encode(const unsigned char *input, int len) {
        BIO *b64 = NULL;
        BIO *bmem = NULL;
        BUF_MEM *bptr = NULL;
        char *buf = NULL;
        b64 = BIO_new(BIO_f_base64());
        if (!b64) return NULL;
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bmem = BIO_new(BIO_s_mem());
        if (!bmem) { BIO_free_all(b64); return NULL; }
        b64 = BIO_push(b64, bmem);
        if (BIO_write(b64, input, len) <= 0) { BIO_free_all(b64); return NULL; }
        if (BIO_flush(b64) != 1) { BIO_free_all(b64); return NULL; }
        BIO_get_mem_ptr(b64, &bptr);
        if (!bptr || bptr->length <= 0) { BIO_free_all(b64); return NULL; }
        buf = malloc(bptr->length + 1);
        if (!buf) { BIO_free_all(b64); return NULL; }
        memcpy(buf, bptr->data, bptr->length);
        buf[bptr->length] = '\0';
        BIO_free_all(b64);
        return buf;
}

unsigned char *base64_decode(const char *input, int *out_len) {
        BIO *b64 = NULL;
        BIO *bmem = NULL;
        size_t input_len = strlen(input);
        unsigned char *buffer = malloc(input_len);
        if (!buffer) return NULL;
        b64 = BIO_new(BIO_f_base64());
        if (!b64) { free(buffer); return NULL; }
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bmem = BIO_new_mem_buf((void*)input, input_len);
        if (!bmem) { BIO_free_all(b64); free(buffer); return NULL; }
        bmem = BIO_push(b64, bmem);
        int len = BIO_read(bmem, buffer, input_len);
        if (len <= 0) { BIO_free_all(bmem); free(buffer); return NULL; }
        BIO_free_all(bmem);
        *out_len = len;
        return buffer;
}

int derive_key_pbkdf2(const char *passphrase,
                      const unsigned char *salt,
                      int salt_len,
                      unsigned char *key,
                      int key_len)
{
        if (!passphrase || !salt || !key) return 0;
        const int iterations = 100000;
        if (PKCS5_PBKDF2_HMAC(passphrase, strlen(passphrase), salt, salt_len, iterations, EVP_sha256(), key_len, key) != 1) return 0;
        return 1;
}

int encrypt_with_password(const unsigned char *plaintext,
                          int plaintext_len,
                          const char *passphrase,
                          unsigned char **out_blob,
                          int *out_blob_len)
{
        if (!plaintext || plaintext_len <= 0 || !passphrase || !out_blob || !out_blob_len) return 0;
        const int salt_len = 16;
        const int iv_len = 16;
        const int key_len = 32;
        unsigned char salt[salt_len];
        unsigned char iv[iv_len];
        unsigned char key[key_len];
        if (RAND_bytes(salt, salt_len) != 1) return 0;
        if (RAND_bytes(iv, iv_len) != 1) return 0;
        if (!derive_key_pbkdf2(passphrase, salt, salt_len, key, key_len)) return 0;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) { OPENSSL_cleanse(key, key_len); return 0; }
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) { EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        int block_size = EVP_CIPHER_block_size(EVP_aes_256_cbc());
        int max_ct_len = plaintext_len + block_size;
        unsigned char *ciphertext = malloc(max_ct_len);
        if (!ciphertext) { EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        int len = 0;
        int ciphertext_len = 0;
        if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) { free(ciphertext); EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        ciphertext_len = len;
        if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) { free(ciphertext); EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        ciphertext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        int total_len = salt_len + iv_len + ciphertext_len;
        unsigned char *blob = malloc(total_len);
        if (!blob) { free(ciphertext); OPENSSL_cleanse(key, key_len); return 0; }
        memcpy(blob, salt, salt_len);
        memcpy(blob + salt_len, iv, iv_len);
        memcpy(blob + salt_len + iv_len, ciphertext, ciphertext_len);
        free(ciphertext);
        OPENSSL_cleanse(key, key_len);
        *out_blob = blob;
        *out_blob_len = total_len;
        return 1;
}

int decrypt_with_password(const unsigned char *in_blob,
                          int in_blob_len,
                          const char *passphrase,
                          unsigned char **out_plain,
                          int *out_plain_len)
{
        if (!in_blob || in_blob_len <= 0 || !passphrase || !out_plain || !out_plain_len) return 0;
        const int salt_len = 16;
        const int iv_len = 16;
        const int key_len = 32;
        if (in_blob_len <= salt_len + iv_len) return 0;
        const unsigned char *salt = in_blob;
        const unsigned char *iv = in_blob + salt_len;
        const unsigned char *ciphertext = in_blob + salt_len + iv_len;
        int ciphertext_len = in_blob_len - salt_len - iv_len;
        unsigned char key[key_len];
        if (!derive_key_pbkdf2(passphrase, salt, salt_len, key, key_len)) return 0;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        if (!ctx) { OPENSSL_cleanse(key, key_len); return 0; }
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1) { EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        unsigned char *plaintext = malloc(ciphertext_len + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        if (!plaintext) { EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        int len = 0;
        int plaintext_len = 0;
        if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1) { free(plaintext); EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        plaintext_len = len;
        if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1) { free(plaintext); EVP_CIPHER_CTX_free(ctx); OPENSSL_cleanse(key, key_len); return 0; }
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        OPENSSL_cleanse(key, key_len);
        *out_plain = plaintext;
        *out_plain_len = plaintext_len;
        return 1;
}

int to_hex(const unsigned char *in, int in_len, char **out) {
        if (!in || in_len < 0 || !out) return 0;
        char *buf = malloc(in_len * 2 + 1);
        if (!buf) return 0;
        for (int i = 0; i < in_len; ++i) sprintf(buf + i * 2, "%02x", in[i]);
        buf[in_len * 2] = '\0';
        *out = buf;
        return 1;
}

/*
int main() {
        const char *passphrase = "S3cureP@ss!";
        const char *message = "Hello, secure world!";
        unsigned char hash[32];
        if (!sha256_hash(message, hash)) {
                fprintf(stderr, "sha256 failed\n");
                return 1;
        }
        char *hexhash = NULL;
        if (to_hex(hash, 32, &hexhash)) {
                printf("SHA256: %s\n", hexhash);
                free(hexhash);
        }
        unsigned char *blob = NULL;
        int blob_len = 0;
        if (!encrypt_with_password((const unsigned char *)message, strlen(message), passphrase, &blob, &blob_len)) {
                fprintf(stderr, "encryption failed\n");
                return 1;
        }
        char *b64 = base64_encode(blob, blob_len);
        if (b64) {
                printf("Encrypted (base64): %s\n", b64);
        } else {
                fprintf(stderr, "base64 encode failed\n");
                free(blob);
                return 1;
        }
        int decoded_len = 0;
        unsigned char *decoded = base64_decode(b64, &decoded_len);
        if (!decoded) {
                fprintf(stderr, "base64 decode failed\n");
                free(b64);
                free(blob);
                return 1;
        }
        unsigned char *decrypted = NULL;
        int decrypted_len = 0;
        if (!decrypt_with_password(decoded, decoded_len, passphrase, &decrypted, &decrypted_len)) {
                fprintf(stderr, "decryption failed\n");
                free(decoded);
                free(b64);
                free(blob);
                return 1;
        }
        printf("Decrypted: %.*s\n", decrypted_len, decrypted);
        OPENSSL_cleanse(decrypted, decrypted_len);
        free(decrypted);
        OPENSSL_cleanse(decoded, decoded_len);
        free(decoded);
        free(blob);
        free(b64);
        return 0;
}
*/

