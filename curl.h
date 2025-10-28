#ifndef CURL_H
#define CURL_H

struct MemoryStruct {
		char *memory;
		size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
int wd_download_file(const char *url, const char *fname);

#endif
