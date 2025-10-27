#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SYM "\\"
#define MKDIR(path) _mkdir(path)
#define SETENV(name, val, overwrite) _putenv_s(name, val)
#else
#include <unistd.h>
#define PATH_SYM "/"
#define MKDIR(path) mkdir(path, 0755)
#define SETENV(name, val, overwrite) setenv(name, val, overwrite)
#endif

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "color.h"
#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "chain.h"
#include "depends.h"

/**
 * progress_callback - CURL progress callback function
 * @ptr: User pointer (unused)
 * @dltotal: Total bytes to download
 * @dlnow: Bytes downloaded so far
 * @ultotal: Total bytes to upload (unused)
 * @ulnow: Bytes uploaded so far (unused)
 *
 * Return: 0 to continue, non-zero to abort
 */
static int progress_callback(void *ptr, curl_off_t dltotal,
						     curl_off_t dlnow, curl_off_t ultotal,
						     curl_off_t ulnow)
{
		static int last_percent = -1;
		int percent;

		if (dltotal > 0) {
				percent = (int)((dlnow * 100) / dltotal);
				if (percent != last_percent) {
						if (percent < 100)
								printf("\rDownloading... %3d%% ?-", percent);
						else
								printf("\rDownloading... %3d%% - ", percent);

						fflush(stdout);
						last_percent = percent;
				}
		}

		return RETZ;
}

/**
 * extract_archive - Extract downloaded archive based on file type
 * @filename: Archive filename
 *
 * Return: 0 on success, -1 on failure
 */
static int extract_archive(const char *filename)
{
		char output_path[PATH_MAX];
		size_t name_len;

		if (strstr(filename, ".tar") || strstr(filename, ".tar.gz")) {
				printf("Extracting TAR archive: %s\n", filename);
				return wd_extract_tar(filename);
		} else if (strstr(filename, ".zip")) {
				printf("Extracting ZIP archive: %s\n", filename);
				
				name_len = strlen(filename);
				if (name_len > 4 && !strncmp(filename + name_len - 4, ".zip", 4)) {
						strncpy(output_path, filename, name_len - 4);
						output_path[name_len - 4] = '\0';
				} else {
						strcpy(output_path, filename);
				}
				
				return wd_extract_zip(filename, output_path);
		} else {
				printf("Unknown archive type, skipping extraction: %s\n", filename);
				return -RETN;
		}
}

/**
 * prompt_apply_pawncc - Prompt user to apply pawncc after download
 *
 * Return: 1 if user wants to apply, 0 if not
 */
static int prompt_apply_pawncc(void)
{
		wcfg.ipackage = 0;

		char *ptr_sigA;
ret_ptr:
		printf_color(COL_YELLOW, "Apply pawncc now? ");
		ptr_sigA = readline("[Y/n]: ");

		while (1) {
			if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
				return RETN;
			} else
				return RETZ;
		}
		return RETZ;
}

/**
 * prompt_apply_depends - Prompt user to apply depends after download
 *
 * Return: 1 Always Apply.
 */
static int prompt_apply_depends(void)
{
		return RETN;
}

/**
 * wd_download_file - Download file from URL with retry logic
 * @url: URL to download from
 * @filename: Local filename to save as
 *
 * Return: 0 on success, -RETN on failure
 */
int wd_download_file(const char *url, const char *filename)
{
		CURL *curl;
		FILE *file;
		CURLcode res;
		long response_code = 0;
		int retry_count = 0;
		const int max_retries = 5;
		struct stat file_stat;

		printf_info("Downloading: %s", filename);

		do {
				file = fopen(filename, "wb");
				if (!file) {
#if defined(_DBG_PRINT)
						printf_error("Failed to open file: %s", filename);
#endif
						return -RETN;
				}

				curl = curl_easy_init();
				if (!curl) {
#if defined(_DBG_PRINT)
						printf_error("Failed to initialize CURL");
#endif
						fclose(file);
						return -RETN;
				}

				/* Configure CURL options */
				curl_easy_setopt(curl, CURLOPT_URL, url);
				curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
				curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_USERAGENT, "watchdogs/1.0");
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
				curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
				curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

				res = curl_easy_perform(curl);
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
				
				curl_easy_cleanup(curl);
				fclose(file);

				if (res == CURLE_OK && response_code == 200) {
						if (stat(filename, &file_stat) == 0 && file_stat.st_size > 1024) {
								printf("Download successful: %ld bytes\n", file_stat.st_size);
								printf("Checking file type for extraction...\n");

								/* Extract archive if applicable */
								extract_archive(filename);

								char __sz_rm[PATH_MAX];
								snprintf(__sz_rm, sizeof(__sz_rm), "rm -rf %s", filename);
								system(__sz_rm);

								/* Prompt for pawncc if needed */
								if (wcfg.ipackage) {
									if (prompt_apply_pawncc())
											wd_apply_pawncc();
								}
								if (wcfg.idepends) {
									if (prompt_apply_depends())
											wd_apply_depends(filename);
								}

								return RETZ;
						} else {
								printf_error("Downloaded file too small: %ld bytes", 
										     file_stat.st_size);
						}
				} else {
						printf_error("Download failed - HTTP: %ld, CURL: %d, retrying...",
								     response_code, res);
				}

				retry_count++;
#ifdef _WIN32
				Sleep(3000);
#else
				sleep(3);
#endif
		} while (retry_count < max_retries);

		printf_error("Download failed after %d retries", max_retries);
		return -RETN;
}