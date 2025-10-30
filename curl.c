#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "color.h"
#include "extra.h"
#include "crypto.h"
#include "utils.h"
#include "archive.h"
#include "chain.h"
#include "depends.h"
#include "curl.h"

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
								printf("\r\tDownloading... %3d%% ?-", percent);
						else
								printf("\r\tDownloading... %3d%% - ", percent);

						fflush(stdout);
						last_percent = percent;
				}
		}

		return RETZ;
}

/*
 * WriteMemoryCallback - Processing cURL to memory
 */
size_t WriteMemoryCallback(void *contents,
								  size_t size,
								  size_t nmemb,
								  void *userp) {
		size_t realsize = size * nmemb;
		struct MemoryStruct *mem = (struct MemoryStruct *)userp;

		char *ptr = realloc(mem->memory, mem->size + realsize + 1);
		if (ptr == NULL) {
			fprintf(stderr, "Out of memory!\n");
			return RETZ;
		}

		mem->memory = ptr;
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;

		return realsize;
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
				printf(" Extracting TAR archive: %s\n", filename);
				return wd_extract_tar(filename);
		} else if (strstr(filename, ".zip")) {
				printf(" Extracting ZIP archive: %s\n", filename);
				
				name_len = strlen(filename);
				if (name_len > 4 && !strncmp(filename + name_len - 4, ".zip", 4)) {
						strncpy(output_path, filename, name_len - 4);
						output_path[name_len - 4] = '\0';
				} else {
						strcpy(output_path, filename);
				}
				
				return wd_extract_zip(filename, output_path);
		} else {
				pr_info(stdout, "Unknown archive type, skipping extraction: %s\n", filename);
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
		wcfg.wd_ipackage = 0;

		pr_color(stdout, FCOLOUR_YELLOW, "Apply pawncc?");
		char *confirm = readline(" [y/n]: ");
		fflush(stdout);
		if (!confirm) {
			fprintf(stderr, "Error reading input\n");
			wdfree(confirm);
			goto done;
		}
		if (strlen(confirm) == 0) {
			wdfree(confirm);
			confirm = readline(" >>> [y/n]: ");
		}
		if (confirm) {
			if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
				wdfree(confirm);
				return RETN;
			}
		}

done:
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
 * is_archive_file - Checking Archive Type
 *
 * Return: 1 Succes, 0 Failed
 */
int is_archive_file(const char *filename)
{
		const char *ext = strrchr(filename, '.');
		if (!ext) return 0;
		
		const char *archive_exts[] = {
			".zip", ".tar", ".tar.gz", NULL
		};
		
		for (int i = 0; archive_exts[i] != NULL; i++) {
			if (strcasecmp(ext, archive_exts[i]) == 0) {
				return 1;
			}
		}
		
		return 0;
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
		struct stat file_stat;

		pr_color(stdout, FCOLOUR_GREEN, "Downloading: %s\n", filename);

		do {
			file = fopen(filename, "wb");
			if (!file) {
				pr_color(stdout, FCOLOUR_RED, "Failed to open file: %s\n", filename);
				return -RETN;
			}

			curl = curl_easy_init();
			if (!curl) {
				pr_color(stdout, FCOLOUR_RED, "Failed to initialize CURL\n");
				fclose(file);
				return -RETN;
			}
			
			struct curl_slist *headers = NULL;

			if (strfind(wcfg.wd_toml_github_tokens_table, "DO_HERE")) {
				printf_info(stdout, "Can't read Github token.. skipping");
			} else { 
				if (wcfg.wd_toml_github_tokens_table && strlen(wcfg.wd_toml_github_tokens_table) > 0) {
					char auth_header[512];
					snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", wcfg.wd_toml_github_tokens_table);
					headers = curl_slist_append(headers, auth_header);
				}
			}

			headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
			headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			
			if (progress_callback) {
				curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
				curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);
			}

			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);
			fclose(file);

			if (res == CURLE_OK && response_code == 200) {
				if (stat(filename, &file_stat) == 0 && file_stat.st_size > 0) {
					printf("Download successful: %ld bytes\n", file_stat.st_size);
					
					if (is_archive_file(filename)) {
						printf("Checking file type for extraction...\n");
						extract_archive(filename);

						pr_color(stdout, FCOLOUR_GREEN, "Remove the archive '%s'? ", filename);
						char *confirm = readline(" [y/n]: ");
						if (confirm) {
							if (confirm[0] == 'Y' || confirm[0] == 'y') {
								char rm_cmd[PATH_MAX];
								snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf %s", filename);
								system(rm_cmd);
							}
							wdfree(confirm);
						}
					} else {
						printf("File is not an archive, skipping extraction.\n");
					}

					if (wcfg.wd_ipackage && prompt_apply_pawncc())
						wd_apply_pawncc();

					if (wcfg.wd_idepends && prompt_apply_depends())
						wd_apply_depends(filename);

					return RETZ;
				} else {
					pr_color(stdout, FCOLOUR_RED, "Downloaded file is empty or missing\n");
				}
			} else {
				pr_color(stdout, FCOLOUR_RED, "Download failed - HTTP: %ld, CURL: %d, retrying...\n", response_code, res);
			}

			++retry_count;
			sleep(3);
		} while (retry_count < 5);

		pr_color(stdout, FCOLOUR_RED, "Download failed after 5 retries\n");
		return -RETN;
}