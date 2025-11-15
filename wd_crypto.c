#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "wd_util.h"
#include "wd_crypto.h"

static const char base64_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/";
#define hex_list \
        "0123456789" \
        "abcdef"

static uint32_t rotr(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
}

static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (~x & z);
}

static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sigma0(uint32_t x) {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static uint32_t sigma1(uint32_t x) {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static uint32_t gamma0(uint32_t x) {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static uint32_t gamma1(uint32_t x) {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t crc32_table[256];

void crypto_crc32_init_table() {
      uint32_t poly = 0xEDB88320;
      for (uint32_t i = 0; i < 256; i++) {
          uint32_t crc = i;
          for (int j = 0; j < 8; j++) {
              if (crc & 1)
                  crc = (crc >> 1) ^ poly;
              else
                  crc >>= 1;
          }
          crc32_table[i] = crc;
      }
}

uint32_t crypto_generate_crc32(const void *data, size_t length) {
      const uint8_t *bytes = (const uint8_t *)data;
      uint32_t crc = 0xFFFFFFFF;

      for (size_t i = 0; i < length; i++) {
          uint8_t index = (crc ^ bytes[i]) & 0xFF;
          crc = (crc >> 8) ^ crc32_table[index];
      }

      return crc ^ 0xFFFFFFFF;
}

uint32_t crypto_string_hash(const char *s)
{
        uint32_t h = 5381;
        int c;

        while ((c = *s++))
          h = ((h << 5) + h) + (uint32_t)c; /* h * 33 + c */

        return h;
}

unsigned long crypto_djb2_hash_file(const char *filename) {
      FILE *f = fopen(filename, "rb");
      if (!f) {
          perror("fopen");
          return 0;
      }

      unsigned long hash = 5381;
      int c;

      while ((c = fgetc(f)) != EOF) {
          hash = ((hash << 5) + hash) + (unsigned char)c; // hash * 33 + c
      }

      fclose(f);
      return hash;
}

void crypto_print_hex(const unsigned char *buf, size_t len)
{
        for (size_t i = 0; i < len; ++i) {
            printf("%02x", buf[i]);
        }
        putchar('\n');
}

static void crypto_sha256_transform(SHA256_CTX *ctx,
                                    const uint8_t data[SHA256_BLOCK_SIZE]) {
        uint32_t a, b, c, d, e, f, g, h;
        uint32_t w[64];
        int i;

        for (i = 0; i < 16; i++) {
                w[i] = ((uint32_t)data[i * 4] << 24) |
                           ((uint32_t)data[i * 4 + 1] << 16) |
                           ((uint32_t)data[i * 4 + 2] << 8) |
                           ((uint32_t)data[i * 4 + 3]);
        }

        for (i = 16; i < 64; i++) {
                w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        a = ctx->state[0];
        b = ctx->state[1];
        c = ctx->state[2];
        d = ctx->state[3];
        e = ctx->state[4];
        f = ctx->state[5];
        g = ctx->state[6];
        h = ctx->state[7];

        for (i = 0; i < 64; i++) {
                uint32_t t1 = h + sigma1(e) + ch(e, f, g) + k[i] + w[i];
                uint32_t t2 = sigma0(a) + maj(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + t1;
                d = c;
                c = b;
                b = a;
                a = t1 + t2;
        }

        ctx->state[0] += a;
        ctx->state[1] += b;
        ctx->state[2] += c;
        ctx->state[3] += d;
        ctx->state[4] += e;
        ctx->state[5] += f;
        ctx->state[6] += g;
        ctx->state[7] += h;
}

static void crypto_sha256_init(SHA256_CTX *ctx) {
        ctx->state[0] = 0x6a09e667;
        ctx->state[1] = 0xbb67ae85;
        ctx->state[2] = 0x3c6ef372;
        ctx->state[3] = 0xa54ff53a;
        ctx->state[4] = 0x510e527f;
        ctx->state[5] = 0x9b05688c;
        ctx->state[6] = 0x1f83d9ab;
        ctx->state[7] = 0x5be0cd19;
        ctx->count = 0;
}

static void crypto_sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
        size_t i;
        size_t index = ctx->count % SHA256_BLOCK_SIZE;

        ctx->count += len;

        for (i = 0; i + SHA256_BLOCK_SIZE <= len; i += SHA256_BLOCK_SIZE) {
                memcpy(ctx->buffer + index, data + i, SHA256_BLOCK_SIZE - index);
                crypto_sha256_transform(ctx, ctx->buffer);
                index = 0;
        }

        if (i < len) {
                memcpy(ctx->buffer + index, data + i, len - i);
        }
}

static void crypto_sha256_final(SHA256_CTX *ctx, uint8_t digest[SHA256_DIGEST_SIZE]) {
        uint64_t bit_count = ctx->count * 8;
        size_t index = ctx->count % SHA256_BLOCK_SIZE;
        int i;

        ctx->buffer[index++] = 0x80;

        if (index > 56) {
                memset(ctx->buffer + index, 0, SHA256_BLOCK_SIZE - index);
                crypto_sha256_transform(ctx, ctx->buffer);
                index = 0;
        }

        memset(ctx->buffer + index, 0, 56 - index);

        for (i = 0; i < 8; i++) {
                ctx->buffer[63 - i] = (bit_count >> (i * 8)) & 0xff;
        }

        crypto_sha256_transform(ctx, ctx->buffer);

        for (i = 0; i < 8; i++) {
                digest[i * 4] = (ctx->state[i] >> 24) & 0xff;
                digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
                digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
                digest[i * 4 + 3] = ctx->state[i] & 0xff;
        }
}

static uint32_t crypto_simple_rand(void) {
        static uint32_t seed = 0;
        seed = seed * 1103515245 + 12345;
        return seed;
}

static void crypto_simple_rand_bytes(uint8_t *buf, size_t len) {
        size_t i;
        for (i = 0; i < len; i++) {
                buf[i] = crypto_simple_rand() & 0xff;
        }
}

static void crypto_hmac_sha256(const uint8_t *key, size_t key_len,
                               const uint8_t *data, size_t data_len,
                               uint8_t digest[SHA256_DIGEST_SIZE]) {
        SHA256_CTX ctx;
        uint8_t k_ipad[SHA256_BLOCK_SIZE];
        uint8_t k_opad[SHA256_BLOCK_SIZE];
        uint8_t tmp[SHA256_DIGEST_SIZE];
        int i;

        memset(k_ipad, 0, sizeof(k_ipad));
        memset(k_opad, 0, sizeof(k_opad));

        if (key_len > SHA256_BLOCK_SIZE) {
                crypto_sha256_init(&ctx);
                crypto_sha256_update(&ctx, key, key_len);
                crypto_sha256_final(&ctx, tmp);
                memcpy(k_ipad, tmp, SHA256_DIGEST_SIZE);
                memcpy(k_opad, tmp, SHA256_DIGEST_SIZE);
        } else {
                memcpy(k_ipad, key, key_len);
                memcpy(k_opad, key, key_len);
        }

        for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
                k_ipad[i] ^= 0x36;
                k_opad[i] ^= 0x5c;
        }

        crypto_sha256_init(&ctx);
        crypto_sha256_update(&ctx, k_ipad, SHA256_BLOCK_SIZE);
        crypto_sha256_update(&ctx, data, data_len);
        crypto_sha256_final(&ctx, tmp);

        crypto_sha256_init(&ctx);
        crypto_sha256_update(&ctx, k_opad, SHA256_BLOCK_SIZE);
        crypto_sha256_update(&ctx, tmp, SHA256_DIGEST_SIZE);
        crypto_sha256_final(&ctx, digest);
}

void crypto_aes_encrypt(const uint8_t in[AES_BLOCK_SIZE],
                                           uint8_t out[AES_BLOCK_SIZE],
                                           const AES_KEY *key) {
        memcpy(out, in, AES_BLOCK_SIZE);
}

void crypto_aes_decrypt(const uint8_t in[AES_BLOCK_SIZE],
                                           uint8_t out[AES_BLOCK_SIZE],
                                           const AES_KEY *key) {
        memcpy(out, in, AES_BLOCK_SIZE);
}

int crypto_generate_sha256_hash(const char *input, unsigned char output[32])
{
        SHA256_CTX ctx;

        if (!input || !output)
                return WD_RETZ;

        crypto_sha256_init(&ctx);
        crypto_sha256_update(&ctx, (const uint8_t *)input, strlen(input));
        crypto_sha256_final(&ctx, output);

        return WD_RETN;
}

char *crypto_base64_encode(const unsigned char *input, int len)
{
        char *buffer;
        int i, j;
        uint32_t triple;
        int buffer_len;

        if (!input || len <= 0)
                return NULL;

        buffer_len = ((len + 2) / 3) * 4 + 1;
        buffer = wd_malloc(buffer_len);
        if (!buffer)
                return NULL;

        for (i = 0, j = 0; i < len; i += 3, j += 4) {
                triple = (i < len) ? input[i] << 16 : 0;
                triple |= (i + 1 < len) ? input[i + 1] << 8 : 0;
                triple |= (i + 2 < len) ? input[i + 2] : 0;

                buffer[j] = base64_table[(triple >> 18) & 0x3F];
                buffer[j + 1] = base64_table[(triple >> 12) & 0x3F];
                buffer[j + 2] = (i + 1 < len) ? base64_table[(triple >> 6) & 0x3F] : '=';
                buffer[j + 3] = (i + 2 < len) ? base64_table[triple & 0x3F] : '=';
        }
        buffer[j] = '\0';

        return buffer;
}

unsigned char *crypto_base64_decode(const char *input, int *out_len)
{
        unsigned char *buffer;
        int input_len, buffer_len;
        int i, j;
        uint32_t sextet;
        uint8_t val;

        if (!input || !out_len)
                return NULL;

        input_len = strlen(input);
        if (input_len % 4 != 0)
                return NULL;

        buffer_len = (input_len / 4) * 3;
        if (input[input_len - 1] == '=') buffer_len--;
        if (input[input_len - 2] == '=') buffer_len--;

        buffer = wd_malloc(buffer_len);
        if (!buffer)
                return NULL;

        for (i = 0, j = 0; i < input_len; i += 4, j += 3) {
                sextet = 0;

                for (int k = 0; k < 4; k++) {
                        char c = input[i + k];
                        val = 0;

                        if (c >= 'A' && c <= 'Z') val = c - 'A';
                        else if (c >= 'a' && c <= 'z') val = c - 'a' + 26;
                        else if (c >= '0' && c <= '9') val = c - '0' + 52;
                        else if (c == '+') val = 62;
                        else if (c == '/') val = 63;
                        else if (c == '=') val = 0;
                        else {
                                wd_free(buffer);
                                return NULL;
                        }

                        sextet = (sextet << 6) | val;
                }

                buffer[j] = (sextet >> 16) & 0xFF;
                if (input[i + 2] != '=')
                        buffer[j + 1] = (sextet >> 8) & 0xFF;
                if (input[i + 3] != '=')
                        buffer[j + 2] = sextet & 0xFF;
        }

        *out_len = buffer_len;
        return buffer;
}

int crypto_derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
					         int salt_len, unsigned char *key, int key_len)
{
        const int iterations = 100000;
        uint8_t u[SHA256_DIGEST_SIZE];
        uint8_t t[SHA256_DIGEST_SIZE];
        uint8_t salt_buffer[SHA256_BLOCK_SIZE];
        int blocks, i, j, k;
        uint32_t block_counter;

        if (!passphrase || !salt || !key || key_len <= 0)
                return WD_RETZ;

        blocks = (key_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE;

        for (i = 1; i <= blocks; i++) {
                memcpy(salt_buffer, salt, salt_len);
                block_counter = i;
                for (j = 0; j < 4; j++) {
                        salt_buffer[salt_len + 3 - j] = (block_counter >> (j * 8)) & 0xff;
                }

                crypto_hmac_sha256((const uint8_t *)passphrase, strlen(passphrase),
                                   salt_buffer, salt_len + 4, u);
                memcpy(t, u, SHA256_DIGEST_SIZE);

                for (j = 1; j < iterations; j++) {
                        crypto_hmac_sha256((const uint8_t *)passphrase, strlen(passphrase),
                                           u, SHA256_DIGEST_SIZE, u);
                        for (k = 0; k < SHA256_DIGEST_SIZE; k++) {
                                t[k] ^= u[k];
                        }
                }

                int copy_len = (i == blocks) ?
                        key_len - (i - 1) * SHA256_DIGEST_SIZE : SHA256_DIGEST_SIZE;
                memcpy(key + (i - 1) * SHA256_DIGEST_SIZE, t, copy_len);
        }

        return WD_RETN;
}

int crypto_encrypt_with_password(const unsigned char *plain, int plaintext_len,
						  const char *passphrase, unsigned char **out_blob,
						  int *out_blob_len)
{
        const int salt_len = 16;
        const int iv_len = 16;
        unsigned char salt[salt_len];
        unsigned char iv[iv_len];
        int total_len;

        if (!plain || plaintext_len <= 0 || !passphrase ||
                !out_blob || !out_blob_len)
                return WD_RETZ;

        crypto_simple_rand_bytes(salt, salt_len);
        crypto_simple_rand_bytes(iv, iv_len);

        total_len = salt_len + iv_len + plaintext_len;
        unsigned char *blob = wd_malloc(total_len);
        if (!blob)
                return WD_RETZ;

        memcpy(blob, salt, salt_len);
        memcpy(blob + salt_len, iv, iv_len);
        memcpy(blob + salt_len + iv_len, plain, plaintext_len);

        *out_blob = blob;
        *out_blob_len = total_len;

        return WD_RETN;
}

int crypto_decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
						  const char *passphrase, unsigned char **out_plain,
						  int *out_plain_len)
{
        const int salt_len = 16;
        const int iv_len = 16;

        if (!in_blob || in_blob_len <= 0 || !passphrase ||
                !out_plain || !out_plain_len)
                return WD_RETZ;

        if (in_blob_len <= salt_len + iv_len)
                return WD_RETZ;

        int plaintext_len = in_blob_len - salt_len - iv_len;
        unsigned char *plain = wd_malloc(plaintext_len);
        if (!plain)
                return WD_RETZ;

        memcpy(plain, in_blob + salt_len + iv_len, plaintext_len);

        *out_plain = plain;
        *out_plain_len = plaintext_len;

        return WD_RETN;
}

int crypto_convert_to_hex(const unsigned char *in, int in_len, char **out)
{
        char *buffer;
        int i;
        static const char hex[] = hex_list;

        if (!in || in_len < 0 || !out)
                return WD_RETZ;

        buffer = wd_malloc(in_len * 2 + 1);
        if (!buffer)
                return WD_RETZ;

        for (i = 0; i < in_len; i++) {
                buffer[i * 2] = hex[(in[i] >> 4) & 0xF];
                buffer[i * 2 + 1] = hex[in[i] & 0xF];
        }
        buffer[in_len * 2] = '\0';
        *out = buffer;

        return WD_RETN;
}
