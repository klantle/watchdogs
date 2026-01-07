/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>
#include  <errno.h>
#include  <inttypes.h>
#include  <sys/stat.h>

#ifdef DOG_LINUX
#include  <sys/wait.h>
#endif

#include  <curl/curl.h>

#include  "extra.h"
#include  "utils.h"
#include  "crypto.h"
#include  "archive.h"
#include  "replicate.h"
#include  "compiler.h"
#include  "units.h"
#include  "debug.h"
#include  "curl.h"

static char *pawncc_dir_source = NULL;
static char command[DOG_MAX_PATH];

void
curl_verify_cacert_pem(CURL *curl)
{
	int platform = 0;
#ifdef DOG_ANDROID
	platform = 1;
#elif defined(DOG_LINUX)
	platform = 2;
#elif defined(DOG_WINDOWS)
	platform = 3;
#endif
	static int cacert_notice = 0;

	if (platform == 3) {
		if (path_access("cacert.pem") != 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
		else if (access("C:/libdog/cacert.pem", F_OK) == 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO,
			    "C:/libdog/cacert.pem");
		else {
			if (cacert_notice != 1) {
				pr_color(stdout, DOG_COL_GREEN,
				    "cURL: can't locate cacert.pem - "
				    "SSL verification may fail.\n");
				cacert_notice = 1;
			}
		}
	} else if (platform == 1) {
		const char *prefix = getenv("PREFIX");
		if (!prefix || prefix[0] == '\0') {
			prefix = "/data/data/com.termux/files/usr";
		}

		char ca1[DOG_PATH_MAX], ca2[DOG_PATH_MAX];

		snprintf(ca1, sizeof(ca1),
		    "%s/etc/tls/cert.pem", prefix);
		snprintf(ca2, sizeof(ca2),
		    "%s/etc/ssl/certs/ca-certificates.crt", prefix);

		if (access(ca1, F_OK) == 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO, ca1);
		else if (access(ca2, F_OK) == 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO, ca2);
		else {
			pr_color(stdout, DOG_COL_GREEN,
			    "cURL: can't locate cacert.pem - "
			    "SSL verification may fail.\n");
		}
	} else if (platform == 2) {
		if (access("cacert.pem", F_OK) == 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
		else if (access("/etc/ssl/certs/cacert.pem", F_OK) == 0)
			curl_easy_setopt(curl, CURLOPT_CAINFO,
			    "/etc/ssl/certs/cacert.pem");
		else {
			if (cacert_notice != 1) {
				pr_color(stdout, DOG_COL_GREEN,
				    "cURL: can't locate cacert.pem - "
				    "SSL verification may fail.\n");
				cacert_notice = 1;
			}
		}
	}
}

void
buf_init(struct buf *b)
{
	b->data = dog_malloc(DOG_MAX_PATH);
	if (!b->data) {
		unit_ret_main(NULL);
	}
	b->len = 0;
	b->allocated = (b->data) ? DOG_MAX_PATH : 0;
}

void
buf_free(struct buf *b)
{
	if (b->data) {
		dog_free(b->data);
		b->data = NULL;
	}
	b->len = 0;
	b->allocated = 0;
}

size_t
write_callbacks(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct buf *b = userdata;

	if (b->data && ((uintptr_t)b->data & 0x7)) {
		fprintf(stderr,
		    " Buffer pointer corrupted: %p\n",
		    b->data);
		return 0;
	}

	size_t total = size * nmemb;

	if (total > 0xFFFFFFF)
		return 0;

	size_t required = b->len + total + 1;

	if (required > b->allocated) {
		size_t new_alloc = (b->allocated * 3) >> 1;
		new_alloc = (required > new_alloc) ? required : new_alloc;
		new_alloc = (new_alloc < 0x4000000) ? new_alloc : 0x4000000;

		char *p = realloc(b->data, new_alloc);
		if (!p) {
#if defined(_DBG_PRINT)
			fprintf(stderr,
			    " Realloc failed for %zu bytes\n",
			    new_alloc);
#endif
			return 0;
		}

		b->data = p;
		b->allocated = new_alloc;
	}

	memcpy(b->data + b->len, ptr, total);
	b->len += total;
	b->data[b->len] = 0;

	return total;
}

void
memory_struct_init(struct memory_struct *mem)
{
	mem->memory = dog_malloc(DOG_MAX_PATH);
	if (!mem->memory) {
		unit_ret_main(NULL);
	}
	mem->size = 0;
	mem->allocated = mem->memory ? DOG_MAX_PATH : 0;
}

void
memory_struct_free(struct memory_struct *mem)
{
	if (mem->memory) {
		free(mem->memory);
		mem->memory = NULL;
	}
	mem->size = 0;
	mem->allocated = 0;
}

size_t
write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	struct memory_struct *mem = userp;
	size_t realsize = size * nmemb;

	if (!contents || !mem || realsize > 0x10000000)
		return 0;

	size_t required = mem->size + realsize + 1;

	if (required > mem->allocated) {
		size_t new_alloc = mem->allocated ? (mem->allocated * 2) :
		    0x1000;
		if (new_alloc < required)
			new_alloc = required;
		if (new_alloc > 0x8000000)
			new_alloc = 0x8000000;

		char *ptr = realloc(mem->memory, new_alloc);
		if (!ptr) {
#if defined(_DBG_PRINT)
			fprintf(stdout,
			    " Memory exhausted at %zu bytes\n", new_alloc);
#endif
			return 0;
		}
		mem->memory = ptr;
		mem->allocated = new_alloc;
	}

	memcpy(mem->memory + mem->size, contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = '\0';

	return realsize;
}

int
package_url_checking(const char *url, const char *github_token)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return 0;

	CURLcode res;
	long response_code = 0;
	struct curl_slist *headers = NULL;
	char dog_buffer_error[CURL_ERROR_SIZE] = { 0 };

	printf("\tCreate & Checking URL: %s...\t\t[All good]\n", url);

	if (strfind(dogconfig.dog_toml_github_tokens, "DO_HERE", true) ||
	    dogconfig.dog_toml_github_tokens == NULL ||
	    strlen(dogconfig.dog_toml_github_tokens) < 1) {
		pr_color(stdout, DOG_COL_GREEN,
		    "Can't read Github token.. skipping\n");
	} else {
		char auth_header[512];
		snprintf(auth_header, sizeof(auth_header),
		    "Authorization: token %s", github_token);
		headers = curl_slist_append(headers, auth_header);
	}

	headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
	headers = curl_slist_append(headers,
	    "Accept: application/vnd.github.v3+json");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

	printf("   Try Connecting... ");
	fflush(stdout);

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, dog_buffer_error);

	curl_verify_cacert_pem(curl);

	fflush(stdout);

	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	if (response_code == DOG_CURL_RESPONSE_OK &&
	    strlen(dog_buffer_error) == 0) {
		printf("cURL result: %s\t\t[All good]\n",
		    curl_easy_strerror(res));
		printf("Response code: %ld\t\t[All good]\n", response_code);
	} else {
		if (strlen(dog_buffer_error) > 0) {
			printf("Error: %s\t\t[Fail]\n", dog_buffer_error);
			minimal_debugging();
		} else {
			printf("cURL result: %s\t\t[Fail]\n",
			    curl_easy_strerror(res));
			minimal_debugging();
		}
	}

	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);

	return (response_code >= 200 && response_code < 300);
}

int
package_http_get_content(const char *url, const char *github_token,
    char **out_html)
{
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers = NULL;
	struct memory_struct buffer = { 0 };

	curl = curl_easy_init();
	if (!curl)
		return 0;

	if (github_token && strlen(github_token) > 0 &&
	    !strfind(github_token, "DO_HERE", true)) {
		char auth_header[512];
		snprintf(auth_header, sizeof(auth_header),
		    "Authorization: token %s", github_token);
		headers = curl_slist_append(headers, auth_header);
	}

	headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	curl_easy_setopt(curl, CURLOPT_URL, url);

	memory_struct_init(&buffer);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

	curl_verify_cacert_pem(curl);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	curl_slist_free_all(headers);

	if (res != CURLE_OK || buffer.size == 0) {
		memory_struct_free(&buffer);
		return 0;
	}

	*out_html = buffer.memory;

	return 1;
}

const char *__track_suffixes[] = {
	"!", "@",
	"#", "$",
	"%", "^",
	"_", "-",
	".",
	NULL
};

void
tracker_discrepancy(const char *base,
    char discrepancy[][MAX_USERNAME_LEN],
    int *cnt)
{
	int i, j;
	size_t base_len;
	char temp[MAX_USERNAME_LEN];

	if (!base || !discrepancy || !cnt || *cnt >= MAX_VARIATIONS)
		return;

	base_len = strlen(base);
	if (base_len == 0 || base_len >= MAX_USERNAME_LEN)
		return;

	strlcpy(discrepancy[(*cnt)++], base, MAX_USERNAME_LEN);

	for (i = 0;
	    i < (int)base_len &&
	    *cnt < MAX_VARIATIONS &&
	    base_len + 1 < MAX_USERNAME_LEN;
	    i++) {
		memcpy(temp, base, i);

		temp[i] = base[i];
		temp[i + 1] = base[i];

		strlcpy(temp + i + 2,
		    base + i + 1,
		    sizeof(temp) - (i + 2));

		strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
	}

	for (i = 2;
	    i <= 5 &&
	    *cnt < MAX_VARIATIONS;
	    i++) {
		size_t len = base_len;

		if (len + i >= MAX_USERNAME_LEN)
			break;

		memcpy(temp, base, len);

		for (j = 0; j < i; j++)
			temp[len + j] = base[base_len - 1];

		temp[len + i] = '\0';

		strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
	}

	for (i = 0;
	    __track_suffixes[i] &&
	    *cnt < MAX_VARIATIONS;
	    i++) {
		snprintf(temp,
		    sizeof(temp),
		    "%s%s",
		    base,
		    __track_suffixes[i]);

		strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
	}
}

void
tracking_username(CURL *curl, const char *username)
{
	CURLcode res;
	struct memory_struct response;
	struct curl_slist *headers = NULL;

	headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");

	for (int i = 0; social_site_list[i].site_name != NULL; i++) {
		char url[512];

		snprintf(url, sizeof(url),
		    social_site_list[i].url_template,
		    username);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		memory_struct_init(&response);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		    write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

		curl_verify_cacert_pem(curl);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			printf("* [%s] %s -> ERROR %s\n",
			    social_site_list[i].site_name,
			    url,
			    curl_easy_strerror(res));
		} else {
			long status;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
			    &status);

			if (status == 200 || (status >= 300 && status < 400)) {
				println(stdout, "* [%s] %s -> FOUND (%ld)",
				    social_site_list[i].site_name, url,
				    status);
			} else {
				println(stdout, "* [%s] %s -> NOT FOUND (%ld)",
				    social_site_list[i].site_name, url,
				    status);
			}
		}

		memory_struct_free(&response);
	}

	curl_slist_free_all(headers);
}

static void
find_compiler_tools(int *found_pawncc_exe, int *found_pawncc,
    int *found_pawndisasm_exe, int *found_pawndisasm,
    int *found_pawnc_dll, int *found_PAWNC_DLL)
{
	const char *ignore_dir = NULL;

	*found_pawncc_exe = dog_sef_fdir(pawncc_dir_source, "pawncc.exe",
	    ignore_dir);
	*found_pawncc = dog_sef_fdir(pawncc_dir_source, "pawncc", ignore_dir);
	*found_pawndisasm_exe = dog_sef_fdir(pawncc_dir_source,
	    "pawndisasm.exe", ignore_dir);
	*found_pawndisasm = dog_sef_fdir(pawncc_dir_source, "pawndisasm",
	    ignore_dir);
	*found_PAWNC_DLL = dog_sef_fdir(pawncc_dir_source, "PAWNC.dll",
	    ignore_dir);
	*found_pawnc_dll = dog_sef_fdir(pawncc_dir_source, "pawnc.dll",
	    ignore_dir);

	if (*found_pawncc_exe < 1 && *found_pawncc < 1) {
		*found_pawncc_exe = dog_sef_fdir(".", "pawncc.exe",
		    ignore_dir);
		*found_pawncc = dog_sef_fdir(".", "pawncc", ignore_dir);
		*found_PAWNC_DLL = dog_sef_fdir(".", "PAWNC.dll", ignore_dir);
		*found_pawnc_dll = dog_sef_fdir(".", "pawnc.dll", ignore_dir);
		*found_pawndisasm_exe = dog_sef_fdir(".", "pawndisasm.exe",
		    ignore_dir);
		*found_pawndisasm = dog_sef_fdir(".", "pawndisasm", ignore_dir);
	}
	if (*found_pawncc_exe < 1 && *found_pawncc < 1) {
		*found_pawncc_exe = dog_sef_fdir("bin/", "pawncc.exe",
		    ignore_dir);
		*found_pawncc = dog_sef_fdir("bin/", "pawncc", ignore_dir);
		*found_PAWNC_DLL = dog_sef_fdir("bin/", "PAWNC.dll",
		    ignore_dir);
		*found_pawnc_dll = dog_sef_fdir("bin/", "pawnc.dll",
		    ignore_dir);
		*found_pawndisasm_exe = dog_sef_fdir("bin/", "pawndisasm.exe",
		    ignore_dir);
		*found_pawndisasm = dog_sef_fdir("bin/", "pawndisasm",
		    ignore_dir);
	}
}

static const char *
get_compiler_directory(void)
{
	const char *dir_path = NULL;

	if (path_exists("pawno")) {
		dir_path = "pawno";
	} else if (path_exists("qawno")) {
		dir_path = "qawno";
	} else {
		if (MKDIR("pawno") == 0)
			dir_path = "pawno";
	}

	return dir_path;
}

static void
copy_compiler_tool(const char *src_path, const char *tool_name,
    const char *dest_dir)
{
	char dest_path[DOG_PATH_MAX];

	CHMOD_FULL(src_path);

	snprintf(dest_path, sizeof(dest_path),
	    "%s" "%s" "%s", dest_dir, __PATH_STR_SEP_LINUX, tool_name);

	dog_sef_wmv(src_path, dest_path);
}

static void
update_library_environment(const char *lib_path)
{
	const char *home_dir = getenv("HOME");
	if (!home_dir) {
		fprintf(stderr, "Error: HOME environment variable not set\n");
		return;
	}

	const char *shell_rc = NULL;
	char shell_path[DOG_PATH_MAX];

	char *shell = getenv("SHELL");
	if (shell) {
		if (strfind(shell, "zsh", true)) {
			shell_rc = ".zshrc";
		} else if (strfind(shell, "bash", true)) {
			shell_rc = ".bashrc";
		}
	}

	if (!shell_rc) {
		snprintf(shell_path, sizeof(shell_path), "%s/.zshrc", home_dir);
		if (access(shell_path, F_OK) == 0) {
			shell_rc = ".zshrc";
		} else {
			shell_rc = ".bashrc";
		}
	}

	char shell_file[DOG_PATH_MAX * 2];
	snprintf(shell_file, sizeof(shell_file),
	    "%s" "%s" "%s", home_dir, __PATH_STR_SEP_LINUX, shell_rc);

	if (path_access(shell_file) == 0 && strfind(shell_file, "bash", true)) {
		FILE *fp = fopen(shell_file, "w");
		if (fp) {
			fclose(fp);
#ifdef DOG_WINDOWS
			chmod(shell_file, 0644);
#endif
		} else {
			;
		}
	}
}

static int
setup_linux_library(void)
{
#ifdef DOG_WINDOWS
	return 0;
#endif
	const char *selected_path = NULL;
	char libpawnc_src[DOG_PATH_MAX];
	char dest_path[DOG_PATH_MAX];
	int i, found_lib;

	const char *rate_each_any_path[] = {
    LINUX_LIB_PATH, LINUX_LIB32_PATH, TMUX_LIB_PATH,
    TMUX_LIB_LOC_PATH, TMUX_LIB_ARM64_PATH, TMUX_LIB_ARM32_PATH,
    TMUX_LIB_AMD64_PATH, TMUX_LIB_AMD32_PATH
	};
	size_t size_rate_each_any_path = sizeof(rate_each_any_path),
	    size_rate_each_any_path_zero = sizeof(rate_each_any_path[0]);

	if (!strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) ||
	    !strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_UNKNOWN))
		return 0;

	found_lib = dog_sef_fdir(pawncc_dir_source, "libpawnc.so", NULL);

	if (found_lib < 1) {
		found_lib = dog_sef_fdir(".", "libpawnc.so", NULL);
		if (found_lib < 1)
			found_lib = dog_sef_fdir("lib/", "libpawnc.so", NULL);
	}

	for (i = 0; i < dogconfig.dog_sef_count; i++) {
		if (strstr(
		    dogconfig.dog_sef_found_list[i],
		    "libpawnc.so")) {
			strncpy(libpawnc_src,
			    dogconfig.dog_sef_found_list[i],
			    sizeof(libpawnc_src));
			break;
		}
	}

	for (i = 0;
	    i < size_rate_each_any_path / size_rate_each_any_path_zero;
	    i++) if (path_exists(rate_each_any_path[i])) {
		selected_path = rate_each_any_path[i];
		break;
	}

	if (!selected_path) {
		fprintf(stderr, "No valid library path found!\n");
		return -1;
	}

	snprintf(dest_path, sizeof(dest_path),
	    "%s/libpawnc.so", selected_path);

	dog_sef_wmv(libpawnc_src, dest_path);

	update_library_environment(selected_path);

	return 0;
}

void
dog_apply_pawncc(void)
{
	int found_pawncc_exe, found_pawncc;
	int found_pawndisasm_exe, found_pawndisasm;
	int found_pawnc_dll, found_PAWNC_DLL;

	const char *dest_dir;

	char pawncc_src[DOG_PATH_MAX] = { 0 },
	     pawncc_exe_src[DOG_PATH_MAX] = { 0 },
	     pawndisasm_src[DOG_PATH_MAX] = { 0 },
	     pawndisasm_exe_src[DOG_PATH_MAX] = { 0 },
	     pawnc_dll_src[DOG_PATH_MAX] = { 0 },
	     PAWNC_DLL_src[DOG_PATH_MAX] = { 0 };

	int i;

	dog_sef_restore();

	find_compiler_tools(&found_pawncc_exe, &found_pawncc,
	    &found_pawndisasm_exe, &found_pawndisasm,
	    &found_pawnc_dll, &found_PAWNC_DLL);

	dest_dir = get_compiler_directory();
	if (!dest_dir) {
		pr_error(stdout, "Failed to create compiler directory");
		minimal_debugging();
		if (pawncc_dir_source) {
			free(pawncc_dir_source);
			pawncc_dir_source = NULL;
		}
		goto apply_done;
	}

	for (i = 0; i < dogconfig.dog_sef_count; i++) {
		const char *item = dogconfig.dog_sef_found_list[i];
		if (!item)
			continue;
		if (strstr(item, "pawncc.exe")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "pawncc.exe")) {
				strncpy(pawncc_exe_src, item,
				    sizeof(pawncc_exe_src));
			}
		}
		if (strstr(item, "pawncc")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "pawncc")) {
				strncpy(pawncc_src, item, sizeof(pawncc_src));
			}
		}
		if (strstr(item, "pawndisasm.exe")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "pawndisasm.exe")) {
				strncpy(pawndisasm_exe_src, item,
				    sizeof(pawndisasm_exe_src));
			}
		}
		if (strstr(item, "pawndisasm")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "pawndisasm")) {
				strncpy(pawndisasm_src, item,
				    sizeof(pawndisasm_src));
			}
		}
		if (strstr(item, "pawnc.dll")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "pawnc.dll")) {
				strncpy(pawnc_dll_src, item,
				    sizeof(pawnc_dll_src));
			}
		}
		if (strstr(item, "PAWNC.dll")) {
			char *size_last_slash = strrchr(item,
			    __PATH_CHR_SEP_LINUX);
			if (!size_last_slash)
				size_last_slash = strrchr(item,
				    __PATH_CHR_SEP_WIN32);
			if (size_last_slash &&
			    strstr(size_last_slash + 1, "PAWNC.dll")) {
				strncpy(PAWNC_DLL_src, item,
				    sizeof(PAWNC_DLL_src));
			}
		}
	}

	if (found_pawncc_exe && pawncc_exe_src[0])
		copy_compiler_tool(pawncc_exe_src, "pawncc.exe", dest_dir);

	if (found_pawncc && pawncc_src[0])
		copy_compiler_tool(pawncc_src, "pawncc", dest_dir);

	if (found_pawndisasm_exe && pawndisasm_exe_src[0])
		copy_compiler_tool(pawndisasm_exe_src, "pawndisasm.exe",
		    dest_dir);

	if (found_pawndisasm && pawndisasm_src[0])
		copy_compiler_tool(pawndisasm_src, "pawndisasm", dest_dir);

	if (found_PAWNC_DLL && PAWNC_DLL_src[0])
		copy_compiler_tool(PAWNC_DLL_src, "PAWNC.dll", dest_dir);

	if (found_pawnc_dll && pawnc_dll_src[0])
		copy_compiler_tool(pawnc_dll_src, "pawnc.dll", dest_dir);

	setup_linux_library();
	#ifdef DOG_WINDOWS
		DWORD attr = GetFileAttributesA(pawncc_dir_source);
		if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
			SHFILEOPSTRUCTA op;
			char path[DOG_PATH_MAX];

			_ZERO_MEM_WIN32(&op, sizeof(op));
			snprintf(path, sizeof(path), "%s%c%c", pawncc_dir_source, '\0', '\0');

			op.wFunc = FO_DELETE;
			op.pFrom = path;
			op.fFlags = FOF_NO_UI | FOF_SILENT | FOF_NOCONFIRMATION;
			SHFileOperationA(&op);
		}
	#else
		struct stat st;
		if (lstat(pawncc_dir_source, &st) == 0 && S_ISDIR(st.st_mode)) {
			pid_t pid = fork();
			if (pid == 0) {
				execlp("rm", "rm", "-rf", pawncc_dir_source, NULL);
				_exit(127);
			}
			waitpid(pid, NULL, 0);
		}
	#endif

	if (path_exists("pawno/pawnc.dll") == 1)
		rename("pawno/pawnc.dll", "pawno/PAWNC.dll");

	if (path_exists("qawno/pawnc.dll") == 1)
		rename("qawno/pawnc.dll", "qawno/PAWNC.dll");

	pr_info(stdout, "Congratulations! - Done.");

	if (pawncc_dir_source) {
		if (dir_exists(pawncc_dir_source))
			destroy_arch_dir(pawncc_dir_source);
		free(pawncc_dir_source);
		pawncc_dir_source = NULL;
	}

	sleep(1);
	
	printf(DOG_COL_BCYAN "Compile now? y/n: " DOG_COL_DEFAULT);
	char *compile_now = readline(" ");
	if (compile_now[0] == '\0' || compile_now[0] == 'Y' ||
	    compile_now[0] == 'y') {
		dog_free(compile_now);
		dogconfig.dog_garbage_access[DOG_GARBAGE_CURL_COMPILER_TESTING] = DOG_GARBAGE_TRUE;
	}

apply_done:
	unit_ret_main(NULL);
}

static int
prompt_apply_pawncc(void)
{
	dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PAWNC] = DOG_GARBAGE_ZERO;
	
	printf("\x1b[32m==> Apply pawncc?\x1b[0m\n");
	char *confirm = readline("   answer (y/n): ");

	fflush(stdout);

	if (confirm[0] == '\0' || strfind(confirm, "Y", true)) {
		dog_free(confirm);
		return 1;
	}

	dog_free(confirm);

done:
	return 0;
}

int
is_archive_file(const char *filename)
{
	if (strend(filename, ".zip", true) ||
	    strend(filename, ".tar", true) ||
	    strend(filename, ".tar.gz", true)) {
		return 1;
	}
	return 0;
}

static int
debug_callback(CURL *handle __UNUSED__, curl_infotype type,
    char *data, size_t size, void *userptr __UNUSED__)
{
	switch (type) {
	case CURLINFO_TEXT:
	case CURLINFO_HEADER_OUT:
	case CURLINFO_DATA_OUT:
	case CURLINFO_SSL_DATA_OUT:
		break;
	case CURLINFO_HEADER_IN:
		if (!data || (int)size < 1)
			break;
		if (strfind(data, "content-security-policy: ", true))
			break;
		printf("<= Recv header: %.*s", (int)size, data);
		fflush(stdout);
		break;
	case CURLINFO_DATA_IN:
	case CURLINFO_SSL_DATA_IN:
	default:
		break;
	}
	return 0;
}

static void
parsing_filename(char *filename)
{
	if (filename[0] == '\0')
		return;

	for (char *p = filename; *p; ++p) {
		if (*p == '?' || *p == '*' ||
		    *p == '<' || *p == '>' ||
		    *p == '|' || *p == ':' ||
		    *p == '"' || *p == __PATH_CHR_SEP_WIN32 ||
		    *p == __PATH_CHR_SEP_LINUX) {
			*p = '_';
		}
	}

	char *end = filename + strlen(filename) - 1;
	while (end > filename && isspace((unsigned char)*end)) {
		*end-- = '\0';
	}

	if (strlen(filename) == 0) {
		strcpy(filename, "downloaded_file");
	}
}

int
dog_download_file(const char *url, const char *output_filename)
{
	minimal_debugging();

	if (!url || !output_filename) {
		pr_color(stdout, DOG_COL_RED,
		    "Error: Invalid URL or filename\n");
		return -1;
	}

	CURLcode res;
	CURL *curl = NULL;
	long response_code = 0;
	int retry_count = 0;
	struct stat file_stat;

	char clean_filename[DOG_PATH_MAX];
	char *query_pos = strchr(output_filename, '?');
	if (query_pos) {
		size_t name_len = query_pos - output_filename;
		if (name_len >= sizeof(clean_filename)) {
			name_len = sizeof(clean_filename) - 1;
		}
		strncpy(clean_filename, output_filename, name_len);
		clean_filename[name_len] = '\0';
	} else {
		strncpy(clean_filename, output_filename,
		    sizeof(clean_filename) - 1);
		clean_filename[sizeof(clean_filename) - 1] = '\0';
	}

	char final_filename[DOG_PATH_MAX];
	if (strstr(clean_filename, "://") || strstr(clean_filename, "http")) {
		const char *url_filename = strrchr(url, __PATH_CHR_SEP_LINUX);
		if (url_filename) {
			char *url_query_pos = strchr(url_filename, '?');
			if (url_query_pos) {
				size_t url_name_len = url_query_pos -
				    url_filename;
				if (url_name_len >= sizeof(final_filename)) {
					url_name_len = sizeof(final_filename) -
					    1;
				}
				strncpy(final_filename, url_filename,
				    url_name_len);
				final_filename[url_name_len] = '\0';
				++url_filename;
			} else {
				strncpy(final_filename, url_filename,
				    sizeof(final_filename) - 1);
				final_filename[sizeof(final_filename) - 1] =
				    '\0';
			}
		} else {
			snprintf(final_filename, sizeof(final_filename),
			    "downloaded_file");
		}
	} else {
		strncpy(final_filename, clean_filename,
		    sizeof(final_filename) - 1);
		final_filename[sizeof(final_filename) - 1] = '\0';
	}

	parsing_filename(final_filename);

	pr_color(stdout, DOG_COL_GREEN, "* Try Downloading %s", final_filename);

	while (retry_count < 5) {
		curl = curl_easy_init();
		if (!curl) {
			pr_color(stdout, DOG_COL_RED,
			    "Failed to initialize CURL\n");
			return -1;
		}

		struct curl_slist *headers = NULL;

		if (dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PACKAGE] == DOG_GARBAGE_TRUE)
		{
			if (!dogconfig.dog_toml_github_tokens ||
			    strfind(dogconfig.dog_toml_github_tokens,
			    "DO_HERE", true) ||
			    strlen(dogconfig.dog_toml_github_tokens) < 1) {
				pr_color(stdout, DOG_COL_YELLOW,
				    " ~ GitHub token not available\n");
			} else {
				char auth_header[512];
				snprintf(auth_header, sizeof(auth_header),
				    "Authorization: token %s",
				    dogconfig.dog_toml_github_tokens);
				headers = curl_slist_append(headers,
				    auth_header);
				pr_color(stdout, DOG_COL_GREEN,
				    " ~ Using GitHub token: %s\n",
				    dog_masked_text(8,
				    dogconfig.dog_toml_github_tokens));
			}
		}

		headers = curl_slist_append(headers,
		    "User-Agent: watchdogs/1.0");
		headers = curl_slist_append(headers,
		    "Accept: application/vnd.github.v3.raw");

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		    write_memory_callback);

		struct buf download_buffer = { NULL, 0, 0 };
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download_buffer);

		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

		static int create_debugging = 0;
		static int always_create_debugging = 0;

		if (create_debugging == 0) {
			create_debugging = 1;
			pr_color(stdout, DOG_COL_CYAN,
			    " * Enable HTTP debugging? ");
			char *debug_http = readline("(y/n): ");
			if (debug_http[0] == '\0' || debug_http[0] == 'Y' ||
			    debug_http[0] == 'y') {
				always_create_debugging = 1;
			}
			dog_free(debug_http);
		}

		if (always_create_debugging) {
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION,
			    debug_callback);
			curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
		}

		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

		curl_verify_cacert_pem(curl);

		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		if (res == CURLE_OK &&
		    response_code == DOG_CURL_RESPONSE_OK &&
		    download_buffer.len > 0) {
			FILE *fp = fopen(final_filename, "wb");
			if (!fp) {
				pr_color(stdout, DOG_COL_RED,
				    "* Failed to open file for writing: %s "
				    "(errno: %d - %s)\n",
				    final_filename, errno, strerror(errno));
				dog_free(download_buffer.data);
				++retry_count;
				continue;
			}

			size_t written = fwrite(download_buffer.data, 1,
			    download_buffer.len, fp);
			fclose(fp);

			if (written != download_buffer.len) {
				pr_color(stdout, DOG_COL_RED,
				    "* Failed to write all data to file: %s "
				    "(written: %zu, expected: %zu)\n",
				    final_filename, written,
				    download_buffer.len);
				dog_free(download_buffer.data);
				unlink(final_filename);
				++retry_count;
				continue;
			}

			buf_free(&download_buffer);

			if (stat(final_filename, &file_stat) == 0 &&
			    file_stat.st_size > 0) {
				pr_color(stdout, DOG_COL_GREEN,
				    " %% successful: %" PRIdMAX " bytes to %s\n",
				    (intmax_t)file_stat.st_size,
				    final_filename);
				fflush(stdout);

				if (!is_archive_file(final_filename)) {
					pr_color(stdout, DOG_COL_YELLOW,
					    "Warning: File %s is not an archive\n",
					    final_filename);
					return 1;
				}

				char size_filename[DOG_PATH_MAX];
				snprintf(size_filename, sizeof(size_filename),
				    "%s", final_filename);

				char *extension = NULL;
				if ((extension = strstr(size_filename,
				    ".tar.gz")) != NULL) {
					*extension = '\0';
				} else if ((extension = strstr(size_filename,
				    ".tar")) != NULL) {
					*extension = '\0';
				} else if ((extension = strstr(size_filename,
				    ".zip")) != NULL) {
					*extension = '\0';
				}

				dog_extract_archive(final_filename,
				    size_filename);

				if (dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PACKAGE] == DOG_GARBAGE_TRUE)
				{
					static int remove_archive = 0;
					if (remove_archive == 0) {
						pr_color(stdout, DOG_COL_CYAN,
						    "==> Remove archive %s? ",
						    final_filename);
						char *confirm = readline(
						    "(y/n): ");

						if (confirm[0] == '\0' ||
						    confirm[0] == 'Y' ||
						    confirm[0] == 'y') {
							if (path_exists(
							    final_filename) ==
							    1) {
								destroy_arch_dir(
								    final_filename);
							}
							remove_archive = 1;
						}
						dog_free(confirm);
					} else {
						if (path_exists(
						    final_filename) == 1) {
							destroy_arch_dir(
							    final_filename);
						}
					}
				} else {
					pr_color(stdout, DOG_COL_CYAN,
					    "==> Remove archive %s? ",
					    final_filename);
					char *confirm = readline("(y/n): ");

					if (confirm[0] == '\0' ||
					    confirm[0] == 'Y' ||
					    confirm[0] == 'y') {
						if (path_exists(
						    final_filename) == 1) {
							destroy_arch_dir(
							    final_filename);
						}
					}
					dog_free(confirm);
				}

    			if (dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PAWNC] == DOG_GARBAGE_TRUE && prompt_apply_pawncc() == 1)
				{
					pawncc_dir_source = strdup(
					    size_filename);
					dog_apply_pawncc();
				}

				return 0;
			}
		}

		if (download_buffer.data) {
			dog_free(download_buffer.data);
		}

		pr_color(stdout, DOG_COL_YELLOW,
		    " Attempt %d/5 failed (HTTP: %ld). Retrying in 3s...\n",
		    retry_count + 1, response_code);
		++retry_count;
	}

	pr_color(stdout, DOG_COL_RED,
	    " Failed to download %s from %s after %d retries\n",
	    final_filename, url, retry_count);

	return 1;
}