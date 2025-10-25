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

#include "utils.h"

/**
 * sha256_hash - Compute SHA-256 hash of input string
 * @input: Input string to hash
 * @output: Output buffer for hash (32 bytes)
 *
 * Return: RETN on success, RETZ on failure
 */
int sha256_hash(const char *input, unsigned char output[32])
{
	EVP_MD_CTX *ctx;
	unsigned int outlen;

	if (!input || !output)
		return RETZ;

	ctx = EVP_MD_CTX_new();
	if (!ctx)
		return RETZ;

	if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1)
		goto error;

	if (EVP_DigestUpdate(ctx, input, strlen(input)) != 1)
		goto error;

	if (EVP_DigestFinal_ex(ctx, output, &outlen) != 1 || outlen != 32)
		goto error;

	EVP_MD_CTX_free(ctx);
	return RETN;

error:
	EVP_MD_CTX_free(ctx);
	return RETZ;
}

/**
 * base64_encode - Base64 encode binary data
 * @input: Input binary data
 * @len: Length of input data
 *
 * Return: Allocated string with base64 data, NULL on error
 */
char *base64_encode(const unsigned char *input, int len)
{
	BIO *b64, *bmem;
	BUF_MEM *bptr;
	char *buffer;

	if (!input || len <= 0)
		return NULL;

	b64 = BIO_new(BIO_f_base64());
	if (!b64)
		return NULL;

	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem = BIO_new(BIO_s_mem());
	if (!bmem) {
		BIO_free_all(b64);
		return NULL;
	}

	b64 = BIO_push(b64, bmem);

	if (BIO_write(b64, input, len) <= 0)
		goto error;

	if (BIO_flush(b64) != 1)
		goto error;

	BIO_get_mem_ptr(b64, &bptr);
	if (!bptr || bptr->length <= 0)
		goto error;

	buffer = malloc(bptr->length + 1);
	if (!buffer)
		goto error;

	memcpy(buffer, bptr->data, bptr->length);
	buffer[bptr->length] = '\0';

	BIO_free_all(b64);
	return buffer;

error:
	BIO_free_all(b64);
	return NULL;
}

/**
 * base64_decode - Base64 decode string to binary data
 * @input: Base64 encoded string
 * @out_len: Output length of decoded data
 *
 * Return: Allocated buffer with decoded data, NULL on error
 */
unsigned char *base64_decode(const char *input, int *out_len)
{
	BIO *b64, *bmem;
	unsigned char *buffer;
	size_t input_len;
	int len;

	if (!input || !out_len)
		return NULL;

	input_len = strlen(input);
	buffer = malloc(input_len);
	if (!buffer)
		return NULL;

	b64 = BIO_new(BIO_f_base64());
	if (!b64)
		goto error;

	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

	bmem = BIO_new_mem_buf((void *)input, input_len);
	if (!bmem)
		goto error_bio;

	bmem = BIO_push(b64, bmem);

	len = BIO_read(bmem, buffer, input_len);
	if (len <= 0)
		goto error_bmem;

	BIO_free_all(bmem);
	*out_len = len;

	return buffer;

error_bmem:
	BIO_free_all(bmem);
	return NULL;

error_bio:
	BIO_free_all(b64);
error:
	free(buffer);
	return NULL;
}

/**
 * derive_key_pbkdf2 - Derive key using PBKDF2 with SHA-256
 * @passphrase: Passphrase for key derivation
 * @salt: Salt for key derivation
 * @salt_len: Length of salt
 * @key: Output key buffer
 * @key_len: Desired key length
 *
 * Return: RETN on success, RETZ on failure
 */
int derive_key_pbkdf2(const char *passphrase, const unsigned char *salt,
		      int salt_len, unsigned char *key, int key_len)
{
	const int iterations = 100000;

	if (!passphrase || !salt || !key)
		return RETZ;

	if (PKCS5_PBKDF2_HMAC(passphrase, strlen(passphrase), salt, salt_len,
			      iterations, EVP_sha256(), key_len, key) != 1)
		return RETZ;

	return RETN;
}

/**
 * encrypt_with_password - Encrypt data with password using AES-256-CBC
 * @plaintext: Plaintext data to encrypt
 * @plaintext_len: Length of plaintext data
 * @passphrase: Encryption passphrase
 * @out_blob: Output encrypted blob
 * @out_blob_len: Output blob length
 *
 * Return: RETN on success, RETZ on failure
 */
int encrypt_with_password(const unsigned char *plaintext, int plaintext_len,
			  const char *passphrase, unsigned char **out_blob,
			  int *out_blob_len)
{
	const int salt_len = 16;
	const int iv_len = 16;
	const int key_len = 32;
	unsigned char salt[salt_len];
	unsigned char iv[iv_len];
	unsigned char key[key_len];
	EVP_CIPHER_CTX *ctx;
	unsigned char *ciphertext;
	unsigned char *blob;
	int max_ct_len, len, ciphertext_len;
	int total_len;

	if (!plaintext || plaintext_len <= 0 || !passphrase ||
	    !out_blob || !out_blob_len)
		return RETZ;

	/* Generate random salt and IV */
	if (RAND_bytes(salt, salt_len) != 1)
		return RETZ;
	if (RAND_bytes(iv, iv_len) != 1)
		return RETZ;

	/* Derive encryption key */
	if (!derive_key_pbkdf2(passphrase, salt, salt_len, key, key_len))
		return RETZ;

	ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		OPENSSL_cleanse(key, key_len);
		return RETZ;
	}

	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
		goto error_ctx;

	/* Allocate ciphertext buffer */
	max_ct_len = plaintext_len + EVP_CIPHER_block_size(EVP_aes_256_cbc());
	ciphertext = malloc(max_ct_len);
	if (!ciphertext)
		goto error_ctx;

	/* Perform encryption */
	if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
		goto error_ciphertext;

	ciphertext_len = len;

	if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1)
		goto error_ciphertext;

	ciphertext_len += len;
	EVP_CIPHER_CTX_free(ctx);

	/* Create output blob: salt + iv + ciphertext */
	total_len = salt_len + iv_len + ciphertext_len;
	blob = malloc(total_len);
	if (!blob)
		goto error_cleanup;

	memcpy(blob, salt, salt_len);
	memcpy(blob + salt_len, iv, iv_len);
	memcpy(blob + salt_len + iv_len, ciphertext, ciphertext_len);

	free(ciphertext);
	OPENSSL_cleanse(key, key_len);

	*out_blob = blob;
	*out_blob_len = total_len;

	return RETN;

error_ciphertext:
	free(ciphertext);
error_ctx:
	EVP_CIPHER_CTX_free(ctx);
error_cleanup:
	OPENSSL_cleanse(key, key_len);
	return RETZ;
}

/**
 * decrypt_with_password - Decrypt data with password using AES-256-CBC
 * @in_blob: Input encrypted blob
 * @in_blob_len: Length of input blob
 * @passphrase: Decryption passphrase
 * @out_plain: Output plaintext buffer
 * @out_plain_len: Output plaintext length
 *
 * Return: RETN on success, RETZ on failure
 */
int decrypt_with_password(const unsigned char *in_blob, int in_blob_len,
			  const char *passphrase, unsigned char **out_plain,
			  int *out_plain_len)
{
	const int salt_len = 16;
	const int iv_len = 16;
	const int key_len = 32;
	const unsigned char *salt, *iv, *ciphertext;
	unsigned char key[key_len];
	EVP_CIPHER_CTX *ctx;
	unsigned char *plaintext;
	int ciphertext_len, len, plaintext_len;

	if (!in_blob || in_blob_len <= 0 || !passphrase ||
	    !out_plain || !out_plain_len)
		return RETZ;

	if (in_blob_len <= salt_len + iv_len)
		return RETZ;

	/* Extract components from blob */
	salt = in_blob;
	iv = in_blob + salt_len;
	ciphertext = in_blob + salt_len + iv_len;
	ciphertext_len = in_blob_len - salt_len - iv_len;

	/* Derive decryption key */
	if (!derive_key_pbkdf2(passphrase, salt, salt_len, key, key_len))
		return RETZ;

	ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		OPENSSL_cleanse(key, key_len);
		return RETZ;
	}

	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
		goto error_ctx;

	/* Allocate plaintext buffer */
	plaintext = malloc(ciphertext_len + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
	if (!plaintext)
		goto error_ctx;

	/* Perform decryption */
	if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
		goto error_plaintext;

	plaintext_len = len;

	if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1)
		goto error_plaintext;

	plaintext_len += len;

	EVP_CIPHER_CTX_free(ctx);
	OPENSSL_cleanse(key, key_len);

	*out_plain = plaintext;
	*out_plain_len = plaintext_len;

	return RETN;

error_plaintext:
	free(plaintext);
error_ctx:
	EVP_CIPHER_CTX_free(ctx);
	OPENSSL_cleanse(key, key_len);
	return RETZ;
}

/**
 * to_hex - Convert binary data to hexadecimal string
 * @in: Input binary data
 * @in_len: Length of input data
 * @out: Output hexadecimal string
 *
 * Return: RETN on success, RETZ on failure
 */
int to_hex(const unsigned char *in, int in_len, char **out)
{
	char *buffer;
	int i;

	if (!in || in_len < 0 || !out)
		return RETZ;

	buffer = malloc(in_len * 2 + 1);
	if (!buffer)
		return RETZ;

	for (i = 0; i < in_len; i++)
		sprintf(buffer + i * 2, "%02x", in[i]);

	buffer[in_len * 2] = '\0';
	*out = buffer;

	return RETN;
}