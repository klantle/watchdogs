#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define __PATH_SYM "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <unistd.h>
#define __PATH_SYM "/"
#endif

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "chain.h"

static int progress_callback(void *ptr,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t ultotal,
                             curl_off_t ulnow)
{
        static int lp = -1;
        if (dltotal > 0) {
                int percent = (int)((dlnow * 100) / dltotal);
                if (percent != lp) {
                        if (percent < 100)
                                printf("\rDownloading... %3d%% ?-", percent);
                        else
                                printf("\rDownloading... %3d%% - ", percent);

                        fflush(stdout);
                        lp = percent;
                }
        }

        return 0;
}

int watch_download_file(const char *url, const char *fname) {
        CURL *__curl;
        FILE *__fp;
        CURLcode __res;
        long __response_code = 0;
        int __retry = 0;
        const int __max_retry = 5;

        printf("Downloading %s\n", url);
        printf(" Output file: %s\n", fname);

        do {
                __fp = fopen(fname, "wb");
                if (!__fp) {
                        printf_error("fopen failed for %s\n", fname);
                        return -1;
                }

                __curl = curl_easy_init();
                if (!__curl) {
                        printf_error("failed to init curl\n");
                        fclose(__fp);
                        return -1;
                }

                curl_easy_setopt(__curl, CURLOPT_URL, url);
                curl_easy_setopt(__curl, CURLOPT_WRITEDATA, __fp);
                curl_easy_setopt(__curl, CURLOPT_FAILONERROR, 1L);
                curl_easy_setopt(__curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(__curl, CURLOPT_USERAGENT, "watchdogs/1.0");
                curl_easy_setopt(__curl, CURLOPT_CONNECTTIMEOUT, 15L);
                curl_easy_setopt(__curl, CURLOPT_TIMEOUT, 300L);
                curl_easy_setopt(__curl, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(__curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(__curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
                curl_easy_setopt(__curl, CURLOPT_XFERINFODATA, NULL);

                __res = curl_easy_perform(__curl);
                curl_easy_getinfo(__curl, CURLINFO_RESPONSE_CODE, &__response_code);
                curl_easy_cleanup(__curl);
                fclose(__fp);

                if (__res == CURLE_OK && __response_code == 200) {
                        struct stat __st;
                        if (stat(fname, &__st) == 0 && __st.st_size > 1024) {
                                printf("Download success %ld bytes\n", __st.st_size);
                                printf(" Checking file type for extraction...\n");

                                if (strstr(fname, ".tar") || strstr(fname, ".tar.gz")) {
                                        printf(" Extracting TAR archive %s\n", fname);
                                        watch_extract_archive(fname);
                                } else if (strstr(fname, ".zip")) {
                                        printf(" Extracting ZIP archive %s\n", fname);
                                        char
                                            __zip_ops[256]
                                        ;
                                        size_t
                                            len_fname = strlen(fname)
                                        ;
                                        if (len_fname > 4 && !strncmp(fname + len_fname - 4, ".zip", 4)) {
                                                strncpy(__zip_ops,
                                                        fname,
                                                        len_fname - 4);
                                                __zip_ops[len_fname - 4] = '\0';
                                        } else { strcpy(__zip_ops, fname); }
                                        watch_extract_zip(fname, __zip_ops);
                                } else {
                                        printf("Unknown archive type, skipping extraction\n");
                                }

                                if (wcfg.ipcc == 1) {
                                        wcfg.ipcc = 0;
                                        char __ptr_sigA[26];
                                        printf("apply pawncc now? [Y/n]: ");
                                        fflush(stdout);
                                        int c;
                                        while ((c = getchar()) != '\n' && c != EOF);
                                        if (fgets(__ptr_sigA, sizeof(__ptr_sigA), stdin) != NULL) {
                                        __ptr_sigA[strcspn(__ptr_sigA, "\n")] = 0;
                                        if (strcmp(__ptr_sigA, "Y") == 0 || strcmp(__ptr_sigA, "y") == 0)
                                                install_pawncc_now();
                                        }
                                }

                                return 0;
                        } else printf_error("downloaded file too small %ld bytes\n", __st.st_size);
                } else printf_error("download failed HTTP %ld CURLcode %d retrying...\n", __response_code, __res);

                __retry++;
                sleep(3);

        } while (__retry < __max_retry);

        printf_error("Download failed after %d retries\n", __max_retry);
        return -1;
}

