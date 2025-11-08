#ifndef CURL_H
#define CURL_H

#include <curl/curl.h>

struct memory_struct {
		char *memory;
		size_t size;
};

int cacert_pem(CURL *curl);
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp);
int wd_download_file(const char *url, const char *fname);

#endif
