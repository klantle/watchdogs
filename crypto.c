#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

void sha256_hash(
                const char *input, unsigned char output[32])
{
        EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
        if (!mdctx) {
            fprintf(stderr, "Failed to create EVP_MD_CTX\n");
            return;
        }

        EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(mdctx, input, strlen(input));
        unsigned int outlen = 0;
        EVP_DigestFinal_ex(mdctx, output, &outlen);

        EVP_MD_CTX_free(mdctx);
}

void print_hash_hex(const unsigned char hash[32]) {
        for (int i = 0; i < 32; i++)
            printf("%02x", hash[i]);
        printf("\n");
}

void derive_key(const char *passphrase,
                uint8_t *key,
                size_t key_len)
{
        size_t pwd_len = strlen(passphrase);
        for (size_t i = 0; i < key_len; i++) {
            key[i] = (uint8_t)(passphrase[i % pwd_len] ^ (i * 31 + 17));
        }
}

void encrypt(const uint8_t *input,
             uint8_t *output,
             size_t len,
             const char *passphrase)
{
        uint8_t *key = malloc(len);
        derive_key(passphrase, key, len);

        for (size_t i = 0; i < len; i++) {
            uint8_t temp = input[i];
            temp ^= key[i];
            temp = (temp << 3) | (temp >> 5);
            temp = (temp << 4) | (temp >> 4);
            temp = (temp + (uint8_t)(i * 7)) ^ 0xAA;
            output[i] = temp;
        }
        free(key);
}

void decrypt(const uint8_t *input,
             uint8_t *output,
             size_t len,
             const char *passphrase)
{
        uint8_t *key = malloc(len);
        derive_key(passphrase, key, len);

        for (size_t i = 0; i < len; i++) {
            uint8_t temp = input[i];
            temp = (temp ^ 0xAA) - (uint8_t)(i * 7);
            temp = (temp << 4) | (temp >> 4);
            temp = (temp >> 3) | (temp << 5);
            temp ^= key[i];
            output[i] = temp;
        }
        free(key);
}

char* base64_encode(const unsigned char *input, int len) {
        BIO *bmem = NULL;
        BIO *b64 = NULL;
        BUF_MEM *bptr = NULL;

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bmem = BIO_new(BIO_s_mem());
        b64 = BIO_push(b64, bmem);

        BIO_write(b64, input, len);
        BIO_flush(b64);
        BIO_get_mem_ptr(b64, &bptr);

        char *buff = (char *)malloc(bptr->length + 1);
        memcpy(buff, bptr->data, bptr->length);
        buff[bptr->length] = 0;

        BIO_free_all(b64);

        return buff;
}

/**
int main() {
        const char *passphrase = "S3cureP@ss!";
        const char *plain_text = "Hello, World!";
        size_t len = strlen(plain_text);

        uint8_t *encrypted = malloc(len);
        uint8_t *decrypted = malloc(len);

        encrypt((const uint8_t *)plain_text, encrypted, len, passphrase);

        printf("Encrypted (hex): ");
        for (size_t i = 0; i < len; i++) {
            printf("%02X ", encrypted[i]);
        }
        printf("\n");

        decrypt(encrypted, decrypted, len, passphrase);
        printf("Decrypted: %.*s\n", (int)len, decrypted);

        free(encrypted);
        free(decrypted);

        unsigned char hex_data[] = {0xAF, 0x90, 0x84, 0x7A, 0xCE, 0xBF, 0xAD, 0x85, 0x68, 0x79, 0xE1, 0x5F, 0xB5};
        int hex_len = sizeof(hex_data);

        char *b64 = base64_encode(hex_data, hex_len);
        printf("Base64: %s\n", b64);
        free(b64);

        return 0;
}
*/
/**
int main() {
        const char *text = "Hello, World!";
        unsigned char hash[32];

        sha256_hash(text, hash);

        printf("SHA-256: ");
        print_hash_hex(hash);

        return 0;
}
*/