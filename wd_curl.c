#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_crypto.h"
#include "wd_util.h"
#include "wd_archive.h"
#include "wd_unit.h"
#include "wd_depends.h"
#include "wd_curl.h"

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

		return __RETZ;
}

size_t write_memory_callback(void *contents, size_t size,
							size_t nmemb, void *userp)
{
		struct memory_struct *mem = userp;
		size_t realsize = size * nmemb;
		char *ptr;

		if (!contents || !mem)
			return __RETZ;

		ptr = wd_realloc(mem->memory, mem->size + realsize + 1);
		if (!ptr) {
#if defined(_DBG_PRINT)
			pr_error(stdout,
				   "Out of memory detected in %s\n", __func__);
#endif
			return __RETZ;
		}

		mem->memory = ptr;
		memcpy(&mem->memory[mem->size], contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = '\0';

		return realsize;
}

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
				if (name_len > 4 &&
					!strncmp(filename + name_len - 4, ".zip", 4))
				{
						strncpy(output_path, filename, name_len - 4);
						output_path[name_len - 4] = '\0';
				} else {
						strcpy(output_path, filename);
				}

				return wd_extract_zip(filename, output_path);
		} else {
				pr_info(stdout, "Unknown archive type, skipping extraction: %s\n", filename);
				return -__RETN;
		}
}

static void find_compiler_tools(int compiler_type,
								int *found_pawncc_exe, int *found_pawncc,
								int *found_pawndisasm_exe, int *found_pawndisasm,
								int *found_pawnc_dll, int *found_PAWNC_DLL)
{
		const char *ignore_dir = (compiler_type == COMPILER_SAMP) ? "pawno" : "qawno";

		*found_pawncc_exe = wd_sef_fdir(".", "pawncc.exe", ignore_dir);
		*found_pawncc = wd_sef_fdir(".", "pawncc", ignore_dir);
		*found_pawndisasm_exe = wd_sef_fdir(".", "pawndisasm.exe", ignore_dir);
		*found_pawndisasm = wd_sef_fdir(".", "pawndisasm", ignore_dir);
		*found_pawnc_dll = wd_sef_fdir(".", "pawnc.dll", ignore_dir);
		*found_PAWNC_DLL = wd_sef_fdir(".", "PAWNC.dll", ignore_dir);
}

static const char *get_compiler_directory(void)
{
		struct stat st;
		const char *dir_path = NULL;

		if (stat("pawno", &st) == 0 && S_ISDIR(st.st_mode)) {
				dir_path = "pawno";
		} else if (stat("qawno", &st) == 0 && S_ISDIR(st.st_mode)) {
				dir_path = "qawno";
		} else {
				if (MKDIR("pawno") == 0)
						dir_path = "pawno";
		}
		if (stat("pawno/include", &st) != 0 && errno == ENOENT) {
			mkdir_recusrs("pawno/include");
		}
		else if (stat("qawno/include", &st) != 0 && errno == ENOENT) {
			mkdir_recusrs("qawno/include");
		}

		return dir_path;
}

static void copy_compiler_tool(const char *src_path, const char *tool_name,
						       const char *dest_dir)
{
		char dest_path[PATH_MAX];

		snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
		wd_sef_wmv(src_path, dest_path);
}

#ifndef _WIN32
static void update_library_environment(const char *lib_path)
{
		const char *old_path;
		char new_path[256];

		if (strstr(lib_path, "/usr/local")) {
				int is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
				if (is_not_sudo == 0)
						wd_run_command("sudo ldconfig");
				else
						wd_run_command("ldconfig");
		}

		old_path = getenv("LD_LIBRARY_PATH");
		if (old_path) {
				snprintf(new_path, sizeof(new_path), "%s:%s", lib_path, old_path);
		} else {
				snprintf(new_path, sizeof(new_path), "%s", lib_path);
		}

		SETENV("LD_LIBRARY_PATH", new_path, 1);
}
#endif

static int setup_linux_library(void)
{
#ifdef _WIN32
		return __RETZ;
#else
		const char *selected_path = NULL;
		char libpawnc_src[PATH_MAX];
		char dest_path[PATH_MAX];
		struct stat st;
		int i, found_lib;
		const char *lib_paths[] =	{
										"/data/data/com.termux/files/usr/lib/",
										"/data/data/com.termux/files/usr/local/lib/",
										"/data/data/com.termux/arm64/usr/lib",
										"/data/data/com.termux/arm32/usr/lib",
										"/data/data/com.termux/amd32/usr/lib",
										"/data/data/com.termux/amd64/usr/lib",
										"/usr/local/lib",
										"/usr/local/lib32"
									};

		if (!strcmp(wcfg.wd_toml_os_type, OS_SIGNAL_WINDOWS) ||
			!strcmp(wcfg.wd_toml_os_type, OS_SIGNAL_UNKNOWN))
				return __RETZ;

		found_lib = wd_sef_fdir(".", "libpawnc.so", NULL);
		if (!found_lib)
				return __RETZ;

		for (i = 0; i < wcfg.wd_sef_count; i++) {
				if (strstr(wcfg.wd_sef_found_list[i], "libpawnc.so")) {
						strncpy(libpawnc_src, wcfg.wd_sef_found_list[i], sizeof(libpawnc_src));
						break;
				}
		}

		for (i = 0; i < sizeof(lib_paths) / sizeof(lib_paths[0]); i++) {
				if (stat(lib_paths[i], &st) == 0 && S_ISDIR(st.st_mode)) {
						selected_path = lib_paths[i];
						break;
				}
		}

		if (!selected_path) {
				fprintf(stderr, "No valid library path found!\n");
				return -__RETN;
		}

		snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
		wd_sef_wmv(libpawnc_src, dest_path);

		update_library_environment(selected_path);

		return __RETZ;
#endif
}

void wd_apply_pawncc(void)
{
		int found_pawncc_exe, found_pawncc;
		int found_pawndisasm_exe, found_pawndisasm;
		int found_pawnc_dll, found_PAWNC_DLL;
		const char *dest_dir;
		char pawncc_src[PATH_MAX] = { 0 },
			 pawncc_exe_src[PATH_MAX] = { 0 },
			 pawndisasm_src[PATH_MAX] = { 0 },
			 pawndisasm_exe_src[PATH_MAX] = { 0 },
			 pawnc_dll_src[PATH_MAX] = { 0 },
			 PAWNC_DLL_src[PATH_MAX] = { 0 };

		int i;

		wd_sef_fdir_reset();

		find_compiler_tools(wcfg.wd_is_samp ? COMPILER_SAMP : COMPILER_OPENMP,
						    &found_pawncc_exe, &found_pawncc,
						    &found_pawndisasm_exe, &found_pawndisasm,
							&found_pawnc_dll, &found_PAWNC_DLL);

		dest_dir = get_compiler_directory();
		if (!dest_dir) {
				pr_error(stdout, "Failed to create compiler directory");
				goto done;
		}

		for (i = 0; i < wcfg.wd_sef_count; i++) {
				const char *item = wcfg.wd_sef_found_list[i];
				if (!item)
						continue;

				if (strstr(item, "pawncc.exe")) {
						strncpy(pawncc_exe_src, item, sizeof(pawncc_exe_src));
				} else if (strstr(item, "pawncc") && !strstr(item, ".exe")) {
						strncpy(pawncc_src, item, sizeof(pawncc_src));
				} else if (strstr(item, "pawndisasm.exe")) {
						strncpy(pawndisasm_exe_src, item, sizeof(pawndisasm_exe_src));
				} else if (strstr(item, "pawndisasm") && !strstr(item, ".exe")) {
						strncpy(pawndisasm_src, item, sizeof(pawndisasm_src));
				} else if (strstr(item, "pawnc.dll"))  {
						strncpy(pawnc_dll_src, item, sizeof(pawnc_dll_src));
				} else if (strstr(item, "PAWNC.dll")) {
						strncpy(PAWNC_DLL_src, item, sizeof(PAWNC_DLL_src));
				}
		}

		if (found_pawncc_exe && pawncc_exe_src[0])
				copy_compiler_tool(pawncc_exe_src, "pawncc.exe", dest_dir);

		if (found_pawncc && pawncc_src[0])
				copy_compiler_tool(pawncc_src, "pawncc", dest_dir);

		if (found_pawndisasm_exe && pawndisasm_exe_src[0])
				copy_compiler_tool(pawndisasm_exe_src, "pawndisasm.exe", dest_dir);

		if (found_pawndisasm && pawndisasm_src[0])
				copy_compiler_tool(pawndisasm_src, "pawndisasm", dest_dir);

		if (found_pawnc_dll && pawnc_dll_src[0])
				copy_compiler_tool(pawnc_dll_src, "pawnc.dll", dest_dir);

		if (found_PAWNC_DLL && PAWNC_DLL_src[0])
				copy_compiler_tool(PAWNC_DLL_src, "PAWNC.dll", dest_dir);

		setup_linux_library();

		pr_info(stdout, "Compiler installed successfully!");

done:
		wd_main(NULL);
}

static int prompt_apply_pawncc(void)
{
		wcfg.wd_ipackage = 0;

		pr_color(stdout, FCOLOUR_YELLOW, "Apply pawncc?");
		char *confirm = readline(" [y/n]: ");
		fflush(stdout);
		if (!confirm) {
			fprintf(stderr, "Error reading input\n");
			wd_free(confirm);
			goto done;
		}
		if (strlen(confirm) == 0) {
			wd_free(confirm);
			confirm = readline(" >>> [y/n]: ");
		}
		if (confirm) {
			if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
				wd_free(confirm);
				return __RETN;
			}
		}

done:
		return __RETZ;
}

int is_archive_file(const char *filename)
{
		const char *ext = strrchr(filename, '.');
		if (!ext) return 0;

		const char *archive_exts[] = {
										".zip",
										".tar",
										".tar.gz",
										NULL
									 };

		for (int i = 0; archive_exts[i] != NULL; i++) {
			if (strcasecmp(ext, archive_exts[i]) == 0) {
				return 1;
			}
		}

		return 0;
}

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
				return -__RETN;
			}

			curl = curl_easy_init();
			if (!curl) {
				pr_color(stdout, FCOLOUR_RED, "Failed to initialize CURL\n");
				fclose(file);
				return -__RETN;
			}

			struct curl_slist *headers = NULL;

			if (wcfg.wd_idepends == 1) {
				if (strstr(wcfg.wd_toml_github_tokens, "DO_HERE")) {
					pr_color(stdout, FCOLOUR_GREEN, "\t[DEPS]: Can't read Github token.. skipping\n");
					sleep(2);
				} else {
						char auth_header[512];
						snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", wcfg.wd_toml_github_tokens);
						headers = curl_slist_append(headers, auth_header);
						pr_color(stdout, FCOLOUR_GREEN, "\t[DEPS]: Using token: %s...\n", get_masked_text(8, wcfg.wd_toml_github_tokens));
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

			printf("\n");
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

			if (is_native_windows()) {
				if (access("cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
				else if (access("C:/libwatchdogs/cacert.pem", F_OK) == 0)
        			curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
				else
					pr_color(stdout, FCOLOUR_YELLOW, "Warning: No CA file found. SSL verification may fail.\n");
			}

			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);

			if (file)
				fclose(file);

			if (res == CURLE_OK && response_code == 200) {
				if (stat(filename, &file_stat) == 0 && file_stat.st_size > 0) {
					printf("Download successful: %" PRIdMAX " bytes\n", (intmax_t)file_stat.st_size);

					if (is_archive_file(filename)) {
						printf("Checking file type for extraction...\n");
						extract_archive(filename);

						pr_color(stdout, FCOLOUR_GREEN, "Remove the archive '%s'? ", filename);
						char *confirm = readline(" [y/n]: ");
						if (confirm) {
							if (confirm[0] == 'Y' || confirm[0] == 'y') {
								char rm_cmd[PATH_MAX];
								if (is_native_windows())
									snprintf(rm_cmd, sizeof(rm_cmd),
										"if exist \"%s\" (del /f /q \"%s\" 2>nul || rmdir /s /q \"%s\" 2>nul)",
										filename, filename, filename);
								else
									snprintf(rm_cmd, sizeof(rm_cmd),
										"rm -rf %s",
										filename);
								system(rm_cmd);
							}
							wd_free(confirm);
						}
					} else {
						printf("File is not an archive, skipping extraction.\n");
					}

					if (wcfg.wd_ipackage && prompt_apply_pawncc())
						wd_apply_pawncc();

					return __RETZ;
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
		return -__RETN;
}
