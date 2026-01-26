/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#include  "utils.h"
#include  "crypto.h"

/* Base64 encoding table used for converting binary data to ASCII representation */
static const char crypto_base64_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789"
      "+/";
/* Hexadecimal character lookup table for hex conversion operations */
#define crypto_hex_list \
      "0123456789" \
      "abcdef"

/* Rotate right operation - circular shift of bits to the right by n positions */
uint32_t rotr(uint32_t x, int n) {
      return (x >> n) | (x << (32 - n));
}

/* Choice function used in SHA-256 algorithm: selects bits from y or z based on x */
uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
      return (x & y) ^ (~x & z);
}

/* Majority function used in SHA-256: returns majority of three input bits */
uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
      return (x & y) ^ (x & z) ^ (y & z);
}

/* Sigma0 function for SHA-256: applies three rotate-right operations and XORs them */
uint32_t sigma(uint32_t x) {
      return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

/* Sigma1 function for SHA-256: applies three rotate-right operations and XORs them */
uint32_t sigma2(uint32_t x) {
      return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

/* Gamma0 function for SHA-256: used in message schedule expansion */
uint32_t crypto_gamma(uint32_t x) {
      return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

/* Gamma1 function for SHA-256: used in message schedule expansion */
uint32_t crypto_gamma2(uint32_t x) {
      return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

/* SHA-256 constant array K - precomputed constants derived from cube roots of first 64 primes */
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

/**
 * Generate random filename with pattern rand_<random>.tmp
 */
void charset_random(char *buffer, size_t size) {
      const char charset[] =  "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "0123456789";
      time_t t;
      srand((unsigned) time(&t));
      
      /** format: rand_XXXXXX.tmp (where XXXXXX is random) */
      snprintf(buffer, size, "rand_");
      
      int i;
      for (i = 0; i < 8; ++i) {
            int key;
            key = rand() % (int)(sizeof(charset) - 1);
            buffer[ 5 + i ] = charset[key];
      }

      buffer[13] = '\0';
      strncat(buffer,
            ".tmp",
            size - strlen(buffer) - 1);
}

/* Lookup table for CRC-32 computation - precomputed polynomial values */
static uint32_t crc32_table[DOG_PATH_MAX];

/* Initialize CRC-32 lookup table using polynomial 0xEDB88320 */
void crypto_crc32_init_table(void) {
      uint32_t poly = 0xEDB88320;
      /* Generate table for all possible byte values (0-255) */
      for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        /* Process each bit in the byte */
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
              /* If LSB is 1, XOR with polynomial after shifting */
              crc = (crc >> 1) ^ poly;
            else
              /* Just shift right if LSB is 0 */
              crc >>= 1;
        }
        crc32_table[i] = crc;
      }
}

/* Compute CRC-32 checksum for given data using lookup table method */
uint32_t crypto_generate_crc32(const void *data, size_t length) {
      const uint8_t *bytes = (const uint8_t *)data;
      uint32_t crc = 0xFFFFFFFF; /* Initial CRC value (all bits set) */

      /* Process each byte in the input data */
      for (size_t i = 0; i < length; i++) {
        /* XOR current byte with CRC and use result as table index */
        uint8_t index = (crc ^ bytes[i]) & 0xFF;
        /* Update CRC using precomputed table value */
        crc = (crc >> 8) ^ crc32_table[index];
      }

      /* Return final CRC after inverting all bits */
      return (crc ^ 0xFFFFFFFF);
}

/* DJB2 hash function - simple string hashing algorithm */
uint32_t crypto_string_hash(const char *s)
{
      uint32_t h = 5381; /* Initial hash value (magic constant) */
      int c;

      /* Process each character in the string */
      while ((c = *s++))
        /* hash * 33 + character */
        h = ((h << 5) + h) + (uint32_t)c;

      return (h);
}

/* DJB2 hash function applied to file contents */
unsigned long crypto_djb2_hash_file(const char *filename) {
      FILE *f = fopen(filename, "rb");
      if (!f) {
            perror("fopen");
            return (0);
      }

      unsigned long hash = 5381;
      int c;

      /* Read file byte by byte and update hash */
      while ((c = fgetc(f)) != EOF) {
            hash = ((hash << 5) + hash) + (unsigned char)c;
      }

      fclose(f);
      return (hash);
}

/* Print hexadecimal representation of binary data */
void crypto_print_hex(const unsigned char *buf, size_t len, int null_terminator)
{
      /* Print each byte as two hexadecimal characters */
      for (size_t i = 0; i < len; ++i) {
          printf("%02x", buf[i]);
      }
      if (null_terminator != 0)
          putchar('\n');
}

/* Convert binary data to hexadecimal string representation */
int crypto_convert_to_hex(const unsigned char *in, int in_len, char **out)
{
      char *buffer;
      int i;
      static const char hex[] = crypto_hex_list;

      if (!in || in_len < 0 || !out)
            return (0);

      /* Allocate buffer: 2 hex chars per byte + null terminator */
      buffer = dog_malloc(in_len * 2 + 1);
      if (!buffer)
            return (0);

      /* Convert each byte to two hex characters */
      for (i = 0; i < in_len; i++) {
            /* High nibble (4 bits) */
            buffer[i * 2] = hex[(in[i] >> 4) & 0xF];
            /* Low nibble (4 bits) */
            buffer[i * 2 + 1] = hex[in[i] & 0xF];
      }
      buffer[in_len * 2] = '\0';
      *out = buffer;

      return (1);
}

/* SHA-1 compression function - processes 512-bit block */
static void crypto_sha1_transform(SHA1_CTX *ctx, const uint8_t data[SHA1_BLOCK_SIZE]) {
      uint32_t a, b, c, d, e;
      uint32_t w[80]; /* Message schedule array */
      int i;

      /* Initialize working variables from current state */
      a = ctx->state[0];
      b = ctx->state[1];
      c = ctx->state[2];
      d = ctx->state[3];
      e = ctx->state[4];

      /* Convert 512-bit block into 16 32-bit words (big-endian) */
      for (i = 0; i < 16; i++) {
            w[i] = ((uint32_t)data[i * 4] << 24) |
                   ((uint32_t)data[i * 4 + 1] << 16) |
                   ((uint32_t)data[i * 4 + 2] << 8) |
                   ((uint32_t)data[i * 4 + 3]);
      }

      /* Expand 16 words into 80 words for the message schedule */
      for (i = 16; i < 80; i++) {
            w[i] = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
            w[i] = (w[i] << 1) | (w[i] >> 31); /* Left rotate by 1 */
      }

      /* Main SHA-1 compression loop - 80 rounds */
      for (i = 0; i < 80; i++) {
      uint32_t f, k;

      /* Determine function f and constant k based on round number */
      if (i < 20) {
          f = (b & c) | ((~b) & d);
          k = 0x5A827999;
      } else if (i < 40) {
          f = b ^ c ^ d;
          k = 0x6ED9EBA1;
      } else if (i < 60) {
          f = (b & c) | (b & d) | (c & d);
          k = 0x8F1BBCDC;
      } else {
          f = b ^ c ^ d;
          k = 0xCA62C1D6;
      }

      /* Compute temporary value and update working variables */
      uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2); /* Left rotate b by 30 */
            b = a;
            a = temp;
      }

      /* Add compressed chunk back to state */
      ctx->state[0] += a;
      ctx->state[1] += b;
      ctx->state[2] += c;
      ctx->state[3] += d;
      ctx->state[4] += e;
}

/* Initialize SHA-1 context with initial hash values */
static void crypto_sha1_init(SHA1_CTX *ctx) {
      ctx->state[0] = 0x67452301;
      ctx->state[1] = 0xEFCDAB89;
      ctx->state[2] = 0x98BADCFE;
      ctx->state[3] = 0x10325476;
      ctx->state[4] = 0xC3D2E1F0;
      ctx->count[0] = ctx->count[1] = 0; /* Reset bit count */
}

/* Update SHA-1 context with new data */
static void crypto_sha1_update(SHA1_CTX *ctx, const uint8_t *data, size_t len) {
      size_t i;
      size_t index = (ctx->count[0] >> 3) & 63; /* Current byte position in buffer */

      /* Update total bit count (handling 64-bit overflow) */
      if ((ctx->count[0] += (len << 3)) < (len << 3)) {
            ctx->count[1]++; /* Carry to high word */
      }
      ctx->count[1] += (len >> 29); /* Add high bits */

      /* Process input data */
      for (i = 0; i < len; i++) {
      ctx->buffer[index++] = data[i];
      /* Transform when buffer is full (64 bytes) */
      if (index == 64) {
          crypto_sha1_transform(ctx, ctx->buffer);
          index = 0;
      }
    }
}

/* Finalize SHA-1 computation and produce digest */
static void crypto_sha1_final(SHA1_CTX *ctx, uint8_t digest[SHA1_DIGEST_SIZE]) {
      uint8_t bits[8]; /* 64-bit bit count in big-endian */
      size_t index, pad_len;
      int i;

      /* Convert bit count to big-endian bytes */
      for (i = 0; i < 8; i++) {
            bits[i] = (ctx->count[(i >= 4) ? 0 : 1] >> ((3 - (i & 3)) * 8)) & 0xff;
      }

      /* Calculate padding length */
      index = (ctx->count[0] >> 3) & 0x3f; /* Current byte position */
      pad_len = (index < 56) ? (56 - index) : (120 - index); /* Padding needed to reach 448 mod 512 bits */

      /* Add padding: start with 0x80 then zeros */
      crypto_sha1_update(ctx, (uint8_t*)"\x80", 1);
      while (pad_len-- > 1) {
            crypto_sha1_update(ctx, (uint8_t*)"\0", 1);
      }

      /* Append bit count */
      crypto_sha1_update(ctx, bits, 8);

      /* Extract final hash state to digest (big-endian) */
      for (i = 0; i < 5; i++) {
            digest[i * 4] = (ctx->state[i] >> 24) & 0xff;
            digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
            digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
            digest[i * 4 + 3] = ctx->state[i] & 0xff;
      }
}

/* Public SHA-1 hash generation interface */
int crypto_generate_sha1_hash(const char *input, unsigned char output[20]) {
      SHA1_CTX ctx;

      if (!input || !output)
            return (0);

      crypto_sha1_init(&ctx);
      crypto_sha1_update(&ctx, (const uint8_t *)input, strlen(input));
      crypto_sha1_final(&ctx, output);

      return (1);
}

/* SHA-256 compression function - processes 512-bit block */
static void crypto_sha256_transform(SHA256_CTX *ctx,
                            const uint8_t data[SHA256_BLOCK_SIZE]) {
      uint32_t a, b, c, d, e, f, g, h;
      uint32_t w[64]; /* Message schedule array */
      int i;

      /* Convert 512-bit block into 16 32-bit words (big-endian) */
      for (i = 0; i < 16; i++) {
            w[i] = ((uint32_t)data[i * 4] << 24) |
                     ((uint32_t)data[i * 4 + 1] << 16) |
                     ((uint32_t)data[i * 4 + 2] << 8) |
                     ((uint32_t)data[i * 4 + 3]);
      }

      /* Expand 16 words into 64 words for the message schedule */
      for (i = 16; i < 64; i++) {
            w[i] = crypto_gamma2(w[i - 2]) + w[i - 7] + crypto_gamma(w[i - 15]) + w[i - 16];
      }

      /* Initialize working variables from current state */
      a = ctx->state[0];
      b = ctx->state[1];
      c = ctx->state[2];
      d = ctx->state[3];
      e = ctx->state[4];
      f = ctx->state[5];
      g = ctx->state[6];
      h = ctx->state[7];

      /* Main SHA-256 compression loop - 64 rounds */
      for (i = 0; i < 64; i++) {
            /* Compute temporary values using SHA-256 functions */
            uint32_t t1 = h + sigma2(e) + ch(e, f, g) + k[i] + w[i];
            uint32_t t2 = sigma(a) + maj(a, b, c);
            /* Update working variables */
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
      }

      /* Add compressed chunk back to state */
      ctx->state[0] += a;
      ctx->state[1] += b;
      ctx->state[2] += c;
      ctx->state[3] += d;
      ctx->state[4] += e;
      ctx->state[5] += f;
      ctx->state[6] += g;
      ctx->state[7] += h;
}

/* Initialize SHA-256 context with initial hash values (first 32 bits of fractional parts of square roots of first 8 primes) */
static void crypto_sha256_init(SHA256_CTX *ctx) {
      ctx->state[0] = 0x6a09e667;
      ctx->state[1] = 0xbb67ae85;
      ctx->state[2] = 0x3c6ef372;
      ctx->state[3] = 0xa54ff53a;
      ctx->state[4] = 0x510e527f;
      ctx->state[5] = 0x9b05688c;
      ctx->state[6] = 0x1f83d9ab;
      ctx->state[7] = 0x5be0cd19;
      ctx->count = 0; /* Reset byte count */
}

/* Update SHA-256 context with new data */
static void crypto_sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
      size_t i;
      size_t index = ctx->count % SHA256_BLOCK_SIZE; /* Current position in buffer */

      ctx->count += len; /* Update total byte count */

      /* Process complete blocks first */
      for (i = 0; i + SHA256_BLOCK_SIZE <= len; i += SHA256_BLOCK_SIZE) {
            memcpy(ctx->buffer + index, data + i, SHA256_BLOCK_SIZE - index);
            crypto_sha256_transform(ctx, ctx->buffer);
            index = 0;
      }

      /* Store remaining data in buffer */
      if (i < len) {
            memcpy(ctx->buffer + index, data + i, len - i);
      }
}

/* Finalize SHA-256 computation and produce digest */
static void crypto_sha256_final(SHA256_CTX *ctx, uint8_t digest[SHA256_DIGEST_SIZE]) {
      uint64_t bit_count = ctx->count * 8; /* Convert byte count to bit count */
      size_t index = ctx->count % SHA256_BLOCK_SIZE; /* Current position in buffer */
      int i;

      /* Append padding: start with 0x80 byte */
      ctx->buffer[index++] = 0x80;

      /* If not enough space for padding + length, process current block */
      if (index > 56) {
            memset(ctx->buffer + index, 0, SHA256_BLOCK_SIZE - index);
            crypto_sha256_transform(ctx, ctx->buffer);
            index = 0;
      }

      /* Pad with zeros */
      memset(ctx->buffer + index, 0, 56 - index);

      /* Append bit count as 64-bit big-endian integer */
      for (i = 0; i < 8; i++) {
            ctx->buffer[63 - i] = (bit_count >> (i * 8)) & 0xff;
      }

      /* Final transform */
      crypto_sha256_transform(ctx, ctx->buffer);

      /* Extract final hash state to digest (big-endian) */
      for (i = 0; i < 8; i++) {
            digest[i * 4] = (ctx->state[i] >> 24) & 0xff;
            digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
            digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
            digest[i * 4 + 3] = ctx->state[i] & 0xff;
      }
}

/* Simple linear congruential generator for random numbers */
static uint32_t crypto_simple_rand(void) {
      static uint32_t seed = 0;
      seed = seed * 1103515245 + 12345;
      return seed;
}

/* Generate random bytes using simple PRNG */
void crypto_simple_rand_bytes(uint8_t *buf, size_t len) {
      size_t i;
      for (i = 0; i < len; i++) {
            buf[i] = crypto_simple_rand() & 0xff;
      }
}

/* HMAC-SHA256 implementation: Hash-based Message Authentication Code using SHA-256 */
static void crypto_hmac_sha256(const uint8_t *key, size_t key_len,
                         const uint8_t *data, size_t data_len,
                         uint8_t digest[SHA256_DIGEST_SIZE]) {
      SHA256_CTX ctx;
      uint8_t k_ipad[SHA256_BLOCK_SIZE]; /* Inner padding key */
      uint8_t k_opad[SHA256_BLOCK_SIZE]; /* Outer padding key */
      uint8_t tmp[SHA256_DIGEST_SIZE];   /* Intermediate hash */
      int i;

      memset(k_ipad, 0, sizeof(k_ipad));
      memset(k_opad, 0, sizeof(k_opad));

      /* If key longer than block size, hash it first */
      if (key_len > SHA256_BLOCK_SIZE) {
            crypto_sha256_init(&ctx);
            crypto_sha256_update(&ctx, key, key_len);
            crypto_sha256_final(&ctx, tmp);
            memcpy(k_ipad, tmp, SHA256_DIGEST_SIZE);
            memcpy(k_opad, tmp, SHA256_DIGEST_SIZE);
      } else {
            /* Copy key directly if it fits */
            memcpy(k_ipad, key, key_len);
            memcpy(k_opad, key, key_len);
      }

      /* XOR keys with padding constants */
      for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
            k_ipad[i] ^= 0x36; /* Inner pad constant */
            k_opad[i] ^= 0x5c; /* Outer pad constant */
      }

      /* Inner hash: H((key ⊕ ipad) || message) */
      crypto_sha256_init(&ctx);
      crypto_sha256_update(&ctx, k_ipad, SHA256_BLOCK_SIZE);
      crypto_sha256_update(&ctx, data, data_len);
      crypto_sha256_final(&ctx, tmp);

      /* Outer hash: H((key ⊕ opad) || inner_hash) */
      crypto_sha256_init(&ctx);
      crypto_sha256_update(&ctx, k_opad, SHA256_BLOCK_SIZE);
      crypto_sha256_update(&ctx, tmp, SHA256_DIGEST_SIZE);
      crypto_sha256_final(&ctx, digest);
}

/* Placeholder AES encryption function - currently just copies input */
void crypto_aes_encrypt(const uint8_t in[AES_BLOCK_SIZE],
                  uint8_t out[AES_BLOCK_SIZE],
                  const AES_KEY *key __UNUSED__) {
      memcpy(out, in, AES_BLOCK_SIZE);
}

/* Placeholder AES decryption function - currently just copies input */
void crypto_aes_decrypt(const uint8_t in[AES_BLOCK_SIZE],
                                 uint8_t out[AES_BLOCK_SIZE],
                                 const AES_KEY *key) {
      memcpy(out, in, AES_BLOCK_SIZE);
}

/* Public SHA-256 hash generation interface */
int crypto_generate_sha256_hash(const char *input, unsigned char output[32])
{
      SHA256_CTX ctx;

      if (!input || !output)
            return (0);

      crypto_sha256_init(&ctx);
      crypto_sha256_update(&ctx, (const uint8_t *)input, strlen(input));
      crypto_sha256_final(&ctx, output);

      return (1);
}

/* Base64 encode binary data to ASCII string */
char *crypto_base64_encode(const unsigned char *input, int len)
{
      char *buffer;
      int i, j;
      uint32_t triple; /* Stores 3 bytes (24 bits) for encoding */
      int buffer_len;

      if (!input || len <= 0)
            return (NULL);

      /* Calculate output buffer size: ceil(len/3) * 4 + null terminator */
      buffer_len = ((len + 2) / 3) * 4 + 1;
      buffer = dog_malloc(buffer_len);
      if (!buffer)
            return (NULL);

      /* Process input in groups of 3 bytes */
      for (i = 0, j = 0; i < len; i += 3, j += 4) {
            /* Pack 3 bytes into 24-bit value */
            triple = (i < len) ? input[i] << 16 : 0;
            triple |= (i + 1 < len) ? input[i + 1] << 8 : 0;
            triple |= (i + 2 < len) ? input[i + 2] : 0;

            /* Convert 24 bits to 4 Base64 characters */
            buffer[j] = crypto_base64_table[(triple >> 18) & 0x3F];
            buffer[j + 1] = crypto_base64_table[(triple >> 12) & 0x3F];
            /* Add padding if input wasn't multiple of 3 */
            buffer[j + 2] = (i + 1 < len) ? crypto_base64_table[(triple >> 6) & 0x3F] : '=';
            buffer[j + 3] = (i + 2 < len) ? crypto_base64_table[triple & 0x3F] : '=';
      }
      buffer[j] = '\0';

      return (buffer);
}

/* Base64 decode ASCII string to binary data */
unsigned char *crypto_base64_decode(const char *input, int *out_len)
{
      unsigned char *buffer;
      int input_len, buffer_len;
      int i, j;
      uint32_t sextet; /* Stores 4 Base64 chars (24 bits) for decoding */
      uint8_t val;     /* Decoded value of single Base64 character */

      if (!input || !out_len)
            return (NULL);

      input_len = strlen(input);
      /* Base64 length must be multiple of 4 */
      if (input_len % 4 != 0)
            return (NULL);

      /* Calculate output buffer size */
      buffer_len = (input_len / 4) * 3;
      /* Adjust for padding characters */
      if (input[input_len - 1] == '=') buffer_len--;
      if (input[input_len - 2] == '=') buffer_len--;

      buffer = dog_malloc(buffer_len);
      if (!buffer)
            return (NULL);

      /* Process input in groups of 4 characters */
      for (i = 0, j = 0; i < input_len; i += 4, j += 3) {
            sextet = 0;

            /* Convert 4 Base64 chars to 24-bit value */
            for (int k = 0; k < 4; k++) {
                  char c = input[i + k];
                  val = 0;

                  /* Decode Base64 character to 6-bit value */
                  if (c >= 'A' && c <= 'Z') val = c - 'A';
                  else if (c >= 'a' && c <= 'z') val = c - 'a' + 26;
                  else if (c >= '0' && c <= '9') val = c - '0' + 52;
                  else if (c == '+') val = 62;
                  else if (c == '/') val = 63;
                  else if (c == '=') val = 0; /* Padding */
                  else {
                        /* Invalid Base64 character */
                        dog_free(buffer);
                        return (NULL);
                  }

                  sextet = (sextet << 6) | val;
            }

            /* Extract 3 bytes from 24-bit value */
            buffer[j] = (sextet >> 16) & 0xFF;
            if (input[i + 2] != '=')
                  buffer[j + 1] = (sextet >> 8) & 0xFF;
            if (input[i + 3] != '=')
                  buffer[j + 2] = sextet & 0xFF;
      }

      *out_len = buffer_len;
      return (buffer);
}

/* PBKDF2 key derivation function using HMAC-SHA256 */
int crypto_derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
                               long salt_len, unsigned char *key, int key_len)
{
      const int iterations = 100000; /* Number of iterations for key stretching */
      uint8_t u[SHA256_DIGEST_SIZE]; /* HMAC output */
      uint8_t t[SHA256_DIGEST_SIZE]; /* Accumulated hash */
      uint8_t salt_buffer[SHA256_BLOCK_SIZE]; /* Salt with block counter */
      int blocks, i, j, k;
      uint32_t block_counter;

      if (!passphrase || !salt || !key || key_len <= 0)
            return (0);

      /* Calculate number of SHA-256 blocks needed */
      blocks = (key_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE;

      /* Process each block */
      for (i = 1; i <= blocks; i++) {
            memcpy(salt_buffer, salt, salt_len);
            block_counter = i;
            /* Append block counter as big-endian to salt */
            for (j = 0; j < 4; j++) {
                  salt_buffer[salt_len + 3 - j] = (block_counter >> (j * 8)) & 0xff;
            }

            /* First iteration: U1 = HMAC(Passphrase, Salt || i) */
            crypto_hmac_sha256((const uint8_t *)passphrase, strlen(passphrase),
                           salt_buffer, salt_len + 4, u);
            memcpy(t, u, SHA256_DIGEST_SIZE);

            /* Subsequent iterations: XOR all intermediate hashes */
            for (j = 1; j < iterations; j++) {
                  crypto_hmac_sha256((const uint8_t *)passphrase, strlen(passphrase),
                                 u, SHA256_DIGEST_SIZE, u);
                  for (k = 0; k < SHA256_DIGEST_SIZE; k++) {
                        t[k] ^= u[k];
                  }
            }

            /* Copy derived bytes to output key */
            int copy_len = (i == blocks) ?
                  key_len - (i - 1) * SHA256_DIGEST_SIZE : SHA256_DIGEST_SIZE;
            memcpy(key + (i - 1) * SHA256_DIGEST_SIZE, t, copy_len);
      }

      return (1);
}

/* Password-based encryption (placeholder - currently just adds salt/IV without actual encryption) */
int crypto_encrypt_with_password(const unsigned char *plain, int plaintext_len,
                                     const char *passphrase, unsigned char **out_blob,
                                     int *out_blob_len)
{
      const int salt_len = 16; /* Salt length in bytes */
      const int iv_len = 16;   /* IV length in bytes */
      unsigned char salt[salt_len];
      unsigned char iv[iv_len];
      int total_len;

      if (!plain || plaintext_len <= 0 || !passphrase ||
            !out_blob || !out_blob_len)
            return (0);

      /* Generate random salt and IV */
      crypto_simple_rand_bytes(salt, salt_len);
      crypto_simple_rand_bytes(iv, iv_len);

      /* Create output blob: salt + IV + plaintext */
      total_len = salt_len + iv_len + plaintext_len;
      unsigned char *blob = dog_malloc(total_len);
      if (!blob)
            return (0);

      memcpy(blob, salt, salt_len);
      memcpy(blob + salt_len, iv, iv_len);
      memcpy(blob + salt_len + iv_len, plain, plaintext_len);

      *out_blob = blob;
      *out_blob_len = total_len;

      return (1);
}

/* Password-based decryption (placeholder - currently just extracts data without actual decryption) */
int crypto_decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
                                     const char *passphrase, unsigned char **out_plain,
                                     long *out_plain_len)
{
      const int salt_len = 16;
      const int iv_len = 16;

      if (!in_blob || in_blob_len <= 0 || !passphrase ||
            !out_plain || !out_plain_len)
            return (0);

      if (in_blob_len <= salt_len + iv_len)
            return (0);

      /* Extract plaintext from blob (assuming no actual encryption was applied) */
      int plaintext_len = in_blob_len - salt_len - iv_len;
      unsigned char *plain = dog_malloc(plaintext_len);
      if (!plain)
            return (0);

      memcpy(plain, in_blob + salt_len + iv_len, plaintext_len);

      *out_plain = plain;
      *out_plain_len = plaintext_len;

      return (1);
}
