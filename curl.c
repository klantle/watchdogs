/*
 * What is this?:
 * ------------------------------------------------------------
 * This script provides a utility function for downloading files over HTTP/HTTPS 
 * using libcurl and automatically extracting them if they are recognized archive 
 * formats (.tar, .tar.gz, .zip). It is part of the "watchdogs" system, which 
 * appears to manage file deployments, updates, or automated installations.
 *
 *
 * Script Algorithm:
 * ------------------------------------------------------------
 * 1. Initialize download:
 *      - Prepare the target output file.
 *      - Initialize libcurl for the download.
 * 2. Configure libcurl options:
 *      - Set URL, write callback, timeouts, SSL verification, and user-agent.
 *      - Enable follow-location and fail-on-error.
 * 3. Perform download with retries:
 *      - Retry up to a maximum number of attempts if download fails.
 *      - Wait a few seconds between retries.
 * 4. Verify download:
 *      - Check HTTP response code and file size.
 *      - If successful and file size > 1024 bytes, proceed to extraction.
 * 5. Detect archive type:
 *      - `.tar` or `.tar.gz` → call `watchdogs_extract_archive()`.
 *      - `.zip` → call `watchdogs_extract_zip()` with destination path.
 *      - Otherwise, skip extraction.
 * 6. Optional post-download action:
 *      - If `init_ipcc` flag is set, prompt the user to install pawncc.
 *
 *
 * Script Logic:
 * ------------------------------------------------------------
 * Functions & Flow:
 *
 * > `write_file()`:
 *    - Callback function used by libcurl to write downloaded data to a file.
 *
 * > `progress_callback()`:
 *    - (Optional) Reports download progress; currently prints a simple "Downloading ..." message.
 *
 * > `watchdogs_download_file(const char *url, const char *fname)`:
 *    - Main function to handle file download and optional extraction.
 *    - Opens the output file for writing.
 *    - Initializes libcurl, sets options, and performs the download.
 *    - Verifies HTTP status and minimum file size.
 *    - Detects archive type based on file extension and extracts accordingly.
 *    - Handles retries and waits between attempts.
 *    - Prompts for post-download installation if required by configuration.
 *
 *
 * How to Use?:
 * ------------------------------------------------------------
 * 1. Include this source file in your project and link with libcurl, readline, and archive libraries.
 * 2. Call the download function:
 *      int res = watchdogs_download_file("https://example.com/file.tar.gz", "file.tar.gz");
 * 3. The function will:
 *      - Download the file to the specified path.
 *      - Automatically extract if it is a recognized archive.
 *      - Optionally prompt for post-download actions (e.g., install pawncc).
 * 4. Check the return value:
 *      - 0 → Success (download and extraction completed).
 *      - -1 → Failure after retries.
 *
 * Notes:
 * - Supports Windows and UNIX-like systems with proper path and sleep handling.
 * - Retry logic handles transient network errors up to 5 attempts.
 * - Extraction relies on the watchdogs archive utilities implemented elsewhere.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <unistd.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "watchdogs.h"

static size_t write_file(void *ptr,
                         size_t size,
                         size_t nmemb,
                         FILE *stream
) {
        size_t written = fwrite(ptr, size, nmemb, stream);
        return written;
}

static int progress_callback(void *ptr,
                             curl_off_t dltotal,
                             curl_off_t dlnow,
                             curl_off_t ultotal,
                             curl_off_t ulnow)
{
        if (dltotal > 0 && dlnow <= 1000) {
            printf("\rDownloading ...");
            fflush(stdout);
        }
        return 0;
}

int watchdogs_download_file(const char *url, const char *fname) {
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
                                        watchdogs_extract_archive(fname);
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
                                        watchdogs_extract_zip(fname, __zip_ops);
                                } else {
                                        printf("Unknown archive type, skipping extraction\n");
                                }

                                if (wcfg.init_ipcc == 1) {
                                        wcfg.init_ipcc = 0;
                                        char *__ptr_sigA = readline("apply pawncc now? [Y/n]: ");
                                        if (__ptr_sigA == NULL ||
                                            strlen(__ptr_sigA) == 0 ||
                                            strcmp(__ptr_sigA, "Y") == 0 ||
                                            strcmp(__ptr_sigA, "y") == 0) {
                                            install_pawncc_now();
                                        }
                                        if (__ptr_sigA) free(__ptr_sigA);
                                }

                                return 0;
                        } else printf_error("downloaded file too small %ld bytes\n", __st.st_size);
                } else printf_error("download failed HTTP %ld CURLcode %d retrying...\n", __response_code, __res);

                __retry++;
                sleep(3);

        } while (__retry < __max_retry);

        printf_error("download failed after %d retries\n", __max_retry);
        return -1;
}

