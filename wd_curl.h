#ifndef CURL_H
#define CURL_H

#include <curl/curl.h>

struct buf {
	    char *data;
	    size_t len;
};

struct memory_struct {
		char *memory;
		size_t size;
};

void verify_cacert_pem(CURL *curl);
size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata);
void json_escape_string(char *dest, const char *src, size_t dest_size);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
int wd_download_file(const char *url, const char *fname);

#endif
