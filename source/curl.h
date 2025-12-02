#ifndef CURL_H
#define CURL_H

#include <curl/curl.h>

struct buf {
	    char *data;
	    size_t len;
		size_t allocated;
};

struct memory_struct {
		char *memory;
		size_t size;
		size_t allocated;
};

#define MAX_NUM_SITES (80)
#define WG_CURL_RESPONSE_OK (200)

void verify_cacert_pem(CURL *curl);
void buf_init(struct buf *b);
void buf_free(struct buf *b);
size_t write_callbacks(void *ptr, size_t size, size_t nmemb, void *userdata);
void memory_struct_init(struct memory_struct *mem);
void memory_struct_free(struct memory_struct *mem);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
void account_tracker_discrepancy(const char *base, char variations[][100], int *variation_count);
void account_tracking_username(CURL *curl, const char *username);
int wg_download_file(const char *url, const char *fname);

#endif
