#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "wd_extra.h"
#include "wd_util.h"
#include "wd_curl.h"
#include "wd_archive.h"
#include "wd_crypto.h"
#include "wd_unit.h"
#include "wd_depends.h"

char working[512];
char *tag_ptr, *path, *first_slash;
char *user, *repo_slash, *repo, *git_ext;
char cmd[MAX_PATH * 3], fpath[PATH_MAX * 2],
		parent[PATH_MAX], dest[PATH_MAX];
char *dname, *ext;

static size_t dep_curl_write_cb (void *contents, size_t size, size_t nmemb, void *userp)
{
		struct dep_curl_buffer *mem = (struct dep_curl_buffer *)userp;
		size_t realsize = size * nmemb;
		char *ptr;

		ptr = wd_realloc(mem->data, mem->size + realsize + 1);
		if (!ptr)
				return __RETZ;

		mem->data = ptr;
		memcpy(&mem->data[mem->size], contents, realsize);
		mem->size += realsize;
		mem->data[mem->size] = 0;

		return realsize;
}

int dep_check_url (const char *url, const char *github_token)
{
		CURL *curl = curl_easy_init();
		if (!curl) return 0;

		CURLcode res;
		long response_code = 0;
		struct curl_slist *headers = NULL;
		char error_buffer[CURL_ERROR_SIZE] = { 0 };

		printf("\t[DEPS] Using URL: %s...\n", url);
		if (strstr(wcfg.wd_toml_github_tokens, "DO_HERE")) {
			pr_color(stdout, FCOLOUR_GREEN, "[DEPS] Can't read Github token.. skipping\n");
			sleep(2);
		} else {
				char auth_header[512];
				snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", github_token);
				headers = curl_slist_append(headers, auth_header);
		}

		headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
		headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

		printf("   Connecting... ");
		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		cacert_pem(curl);

		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		printf("cURL result: %s\n", curl_easy_strerror(res));
		printf("Response code: %ld\n", response_code);
		if (strlen(error_buffer) > 0) {
			printf("Error: %s\n", error_buffer);
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		return (res == CURLE_OK && response_code >= 200 && response_code < 300);
}

int dep_http_get_content (const char *url, char **out_html)
{
		CURL *curl;
		struct dep_curl_buffer buffer = { 0 };
		CURLcode res;

		curl = curl_easy_init();
		if (!curl)
				return __RETZ;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dep_curl_write_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "watchdogs/1.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

		cacert_pem(curl);

		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK || buffer.size == 0) {
				wd_free(buffer.data);
				return __RETZ;
		}

		*out_html = buffer.data;
		return __RETN;
}

static char *dep_get_assets (char **deps_assets, int count, const char *preferred_os)
{
		int i, j;
		const char *os_patterns[] = {
#ifdef _WIN32
				"windows", "win", ".exe", "msvc", "mingw",
#elif defined(__APPLE__)
				"macos", "mac", "darwin", "osx",
#else
				"linux", "ubuntu", "debian", "centos",
#endif
				"src", "source"
		};
		size_t num_patterns = sizeof(os_patterns) / sizeof(os_patterns[0]);

		if (count == 0)
				return NULL;
		if (count == 1)
				return strdup(deps_assets[0]);

		for (i = 0; i < num_patterns; i++) {
				for (j = 0; j < count; j++) {
						if (strstr(deps_assets[j], os_patterns[i]))
								return strdup(deps_assets[j]);
				}
		}

		return strdup(deps_assets[0]);
}

static int
dep_parse_repo(const char *input, struct dep_repo_info *__deps_data)
{
		memset(__deps_data, 0, sizeof(*__deps_data));

		strcpy(__deps_data->host, "github");
		strcpy(__deps_data->domain, "github.com");

		strncpy(working, input, sizeof(working) - 1);
		working[sizeof(working) - 1] = '\0';

		tag_ptr = strrchr(working, ':');
		if (tag_ptr) {
			*tag_ptr = '\0';
			strncpy(__deps_data->tag, tag_ptr + 1, sizeof(__deps_data->tag) - 1);

			if (!strcmp(__deps_data->tag, "latest")) {
				printf("[DEPS] Latest ");
				pr_color(stdout, FCOLOUR_GREEN, "(:latest) ");
				printf("tag detected, will use latest release\n");
			}
		}

		path = working;

		if (!strncmp(path, "https://", 8))
			path += 8;
		else if (!strncmp(path, "http://", 7))
			path += 7;

		if (!strncmp(path, "github/", 7)) {
			strcpy(__deps_data->host, "github");
			strcpy(__deps_data->domain, "github.com");
			path += 7;
		} else if (!strncmp(path, "gitlab/", 7)) {
			strcpy(__deps_data->host, "gitlab");
			strcpy(__deps_data->domain, "gitlab.com");
			path += 7;
		} else if (!strncmp(path, "gitea/", 6)) {
			strcpy(__deps_data->host, "gitea");
			strcpy(__deps_data->domain, "gitea.com");
			path += 6;
		} else if (!strncmp(path, "sourceforge/", 12)) {
			strcpy(__deps_data->host, "sourceforge");
			strcpy(__deps_data->domain, "sourceforge.net");
			path += 12;
		} else {
			first_slash = strchr(path, '/');
			if (first_slash && strchr(path, '.') && strchr(path, '.') < first_slash) {
				char domain[128];

				strncpy(domain, path, first_slash - path);
				domain[first_slash - path] = '\0';

				if (strstr(domain, "github")) {
					strcpy(__deps_data->host, "github");
					strcpy(__deps_data->domain, "github.com");
				} else if (strstr(domain, "gitlab")) {
					strcpy(__deps_data->host, "gitlab");
					strcpy(__deps_data->domain, "gitlab.com");
				} else if (strstr(domain, "gitea")) {
					strcpy(__deps_data->host, "gitea");
					strcpy(__deps_data->domain, "gitea.com");
				} else if (strstr(domain, "sourceforge")) {
					strcpy(__deps_data->host, "sourceforge");
					strcpy(__deps_data->domain, "sourceforge.net");
				} else {
					strncpy(__deps_data->domain, domain,
						sizeof(__deps_data->domain) - 1);
					strcpy(__deps_data->host, "custom");
				}

				path = first_slash + 1;
			}
		}

		user = path;
		repo_slash = strchr(path, '/');
		if (!repo_slash)
			return __RETZ;

		*repo_slash = '\0';
		repo = repo_slash + 1;

		if (!strcmp(__deps_data->host, "sourceforge")) {
			strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);
			strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);

			if (strlen(__deps_data->user) == 0)
				strncpy(__deps_data->repo, user, sizeof(__deps_data->repo) - 1);
		} else {
			strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);

			git_ext = strstr(repo, ".git");
			if (git_ext)
				*git_ext = '\0';
			strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);
		}

		if (strlen(__deps_data->user) == 0 || strlen(__deps_data->repo) == 0)
			return __RETZ;

#if defined(_DBG_PRINT)
		pr_info(stdout, "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
			__deps_data->host,
			__deps_data->domain,
			__deps_data->user,
			__deps_data->repo,
			__deps_data->tag[0] ? __deps_data->tag : "(none)");
#endif

		return __RETN;
}

static int dep_gh_release_assets (const char *user, const char *repo,
								  const char *tag, char **out_urls, int max_urls)
{
		char api_url[PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int url_count = 0;

		snprintf(api_url, sizeof(api_url),
				 "%sapi.github.com/repos/%s/%s/releases/tags/%s",
				 "https://", user, repo, tag);

		if (!dep_http_get_content(api_url, &json_data))
				return __RETZ;

		p = json_data;
		while (url_count < max_urls && (p = strstr(p, "\"browser_download_url\"")) != NULL) {
				const char *url_end;
				size_t url_len;

				p += strlen("\"browser_download_url\"");
				p = strchr(p, '"');
				if (!p)
						break;
				++p;

				url_end = strchr(p, '"');
				if (!url_end)
						break;

				url_len = url_end - p;
				out_urls[url_count] = wd_malloc(url_len + 1);
				strncpy(out_urls[url_count], p, url_len);
				out_urls[url_count][url_len] = '\0';

				++url_count;
				p = url_end + 1;
		}

		wd_free(json_data);
		return url_count;
}

static void
dep_build_repo_url(const struct dep_repo_info *__deps_data, int is_tag_page,
		   char *deps_put_url, size_t deps_put_size)
{
		char deps_actual_tag[128] = { 0 };

		if (__deps_data->tag[0]) {
			strncpy(deps_actual_tag, __deps_data->tag,
				sizeof(deps_actual_tag) - 1);

			if (strcmp(deps_actual_tag, "latest") == 0) {
				if (strcmp(__deps_data->host, "github") == 0 && !is_tag_page) {
					strcpy(deps_actual_tag, "latest");
				}
			}
		}

		if (strcmp(__deps_data->host, "github") == 0) {
			if (is_tag_page && deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/latest",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo);
				} else {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/tag/%s",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else if (deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/latest",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo);
				} else {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/archive/refs/heads/main.zip",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo);
			}
		} else if (strcmp(__deps_data->host, "gitlab") == 0) {
			if (is_tag_page && deps_actual_tag[0]) {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/-/tags/%s",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					deps_actual_tag);
			} else if (deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/-/archive/main/%s-main.zip",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						__deps_data->repo);
				} else {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/-/archive/%s/%s-%s.tar.gz",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						deps_actual_tag,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/-/archive/main/%s-main.zip",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					__deps_data->repo);
			}
		} else if (strcmp(__deps_data->host, "gitea") == 0) {
			if (is_tag_page && deps_actual_tag[0]) {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/%s/tags/%s",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					__deps_data->repo,
					deps_actual_tag);
			} else if (deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/%s/archive/main.zip",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						__deps_data->repo);
				} else {
					snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/%s/archive/%s.tar.gz",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/%s/archive/main.zip",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					__deps_data->repo);
			}
		} else if (strcmp(__deps_data->host, "sourceforge") == 0) {
			if (deps_actual_tag[0] && strcmp(deps_actual_tag, "latest")) {
				snprintf(deps_put_url, deps_put_size,
					"https://%s%s/projects/%s/files/%s/download",
					"https://",
					__deps_data->domain,
					__deps_data->repo,
					deps_actual_tag);
			} else {
				snprintf(deps_put_url, deps_put_size,
					"https://%s%s/projects/%s/files/latest/download",
					"https://",
					__deps_data->domain,
					__deps_data->repo);
			}
		} else {
			if (is_tag_page && deps_actual_tag[0]) {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/releases/tag/%s",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					deps_actual_tag);
			} else if (deps_actual_tag[0]) {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo,
					deps_actual_tag);
			} else {
				snprintf(deps_put_url, deps_put_size,
					"https://%s/%s/%s/archive/refs/heads/main.zip",
					__deps_data->domain,
					__deps_data->user,
					__deps_data->repo);
			}
		}

#if defined(_DBG_PRINT)
		pr_info(stdout, "Built URL: %s (is_tag_page=%d, tag=%s)",
			deps_put_url, is_tag_page,
			deps_actual_tag[0] ? deps_actual_tag : "(none)");
#endif
}

static int dep_gh_latest_tag (const char *user, const char *repo,
                              		  char *out_tag, size_t deps_put_size)
{
		char api_url[PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int ret = 0;

		snprintf(api_url, sizeof(api_url),
				"%sapi.github.com/repos/%s/%s/releases/latest",
				"https://", user, repo);

		if (!dep_http_get_content(api_url, &json_data))
			return __RETZ;

		p = strstr(json_data, "\"tag_name\"");
		if (p) {
			p = strchr(p, ':');
			if (p) {
				++p; // skip colon
				while (*p &&
					   (*p == ' ' ||
					   *p == '\t' ||
					   *p == '\n' ||
					   *p == '\r')
					  )
					  ++p;

				if (*p == '"') {
					++p; // skip opening quote
					const char *end = strchr(p, '"');
					if (end) {
						size_t tag_len = end - p;
						if (tag_len < deps_put_size) {
							strncpy(out_tag, p, tag_len);
							out_tag[tag_len] = '\0';
							ret = 1;
						}
					}
				}
			}
		}

		wd_free(json_data);
		return ret;
}

static int dep_handle_repo (const struct dep_repo_info *dep_repo_info,
                           char *deps_put_url,
                           size_t deps_put_size)
{
		int ret = 0;

		const char *deps_repo_branch[] = {
											"main",
											"master"
										 };

		char deps_actual_tag[128] = { 0 };

		if (dep_repo_info->tag[0] && strcmp(dep_repo_info->tag, "latest") == 0) {
			if (dep_gh_latest_tag(dep_repo_info->user,
								dep_repo_info->repo,
								deps_actual_tag,
								sizeof(deps_actual_tag))) {
									printf("[DEPS] Using latest tag: %s"
												 "(instead of latest)\n", deps_actual_tag);
			} else {
				pr_error(stdout, "Failed to get latest tag for %s/%s,"
								"falling back to main branch",
								dep_repo_info->user, dep_repo_info->repo);
				strcpy(deps_actual_tag, "");
			}
		} else {
			strncpy(deps_actual_tag, dep_repo_info->tag, sizeof(deps_actual_tag) - 1);
		}

		if (deps_actual_tag[0]) {
			char *deps_assets[10] = { 0 };

			int deps_asset_count = dep_gh_release_assets(dep_repo_info->user,
														 dep_repo_info->repo,
														 deps_actual_tag,
														 deps_assets,
														 10);

			if (deps_asset_count > 0) {
				char *deps_best_asset = NULL;
				if (deps_best_asset == NULL)
					deps_best_asset = dep_get_assets(deps_assets, deps_asset_count, NULL);

				if (deps_best_asset) {
					strncpy(deps_put_url, deps_best_asset, deps_put_size - 1);
					ret = 1;
					wd_free(deps_best_asset);
				}

				for (int j = 0; j < deps_asset_count; j++)
					wd_free(deps_assets[j]);
			}

			if (!ret) {
				const char *deps_arch_format[] = {
					"https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
					"https://github.com/%s/%s/archive/refs/tags/%s.zip"
				};

				for (int j = 0; j < 2 && !ret; j++) {
					snprintf(deps_put_url, deps_put_size, deps_arch_format[j],
							 dep_repo_info->user,
							 dep_repo_info->repo,
							 deps_actual_tag);

					if (dep_check_url(deps_put_url, wcfg.wd_toml_github_tokens))
						ret = 1;
				}
			}
		} else {
			for (int j = 0; j < 2 && !ret; j++) {
				snprintf(deps_put_url, deps_put_size,
						"%s%s/%s/archive/refs/heads/%s.zip",
						"https://github.com/",
						dep_repo_info->user,
						dep_repo_info->repo,
						deps_repo_branch[j]);

				if (dep_check_url(deps_put_url, wcfg.wd_toml_github_tokens)) {
					ret = 1;
					if (j == 1)
						printf("[DEPS] Using master branch (main branch not found)\n");
				}
			}
		}

		return ret;
}

void dep_sym_convert (char *path)
{
		char *p;

		for (p = path; *p; p++) {
				if (*p == '\\')
						*p = '/';
		}
}

int
dep_add_ncheck_hash(const char *_H_file_path, const char *_H_json_path)
{
		char convert_f_path[PATH_MAX];
		char convert_j_path[PATH_MAX];
		int array_size;
		int j;

		char *path_pos;
		while ((path_pos = strstr(_H_file_path, "include/")) != NULL)
		{
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include/")) + 1);
		}
		while ((path_pos = strstr(_H_json_path, "include/")) != NULL)
		{
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include/")) + 1);
		}
		strncpy(convert_f_path, _H_file_path, sizeof(convert_f_path));
			convert_f_path[sizeof(convert_f_path) - 1] = '\0';
		dep_sym_convert(convert_f_path);
		strncpy(convert_j_path, _H_json_path, sizeof(convert_j_path));
			convert_j_path[sizeof(convert_j_path) - 1] = '\0';
		dep_sym_convert(convert_j_path);

		static int init_crc32 = 0;
		if (init_crc32 != 1) {
			init_crc32 = 1;
			init_crc32_table();
		}

		uint32_t crc32_fpath;
		crc32_fpath = crc32(convert_f_path,
												sizeof(convert_f_path)-1);
		char crc_str[9];
		sprintf(crc_str, "%08X", crc32_fpath);

		if (crc32_fpath) {
				pr_color(stdout,
						 FCOLOUR_GREEN,
						 "[DEPS] Create hash (CRC32) for '%s': '%s'\n",
						 convert_j_path,
					     crc_str);
		} else {
				pr_error(stdout,
						"Failed to hash: %s (convert: %s)",
						convert_j_path,
						convert_f_path);
				return 0;
		}

		return 1;
}

void dep_implementation_samp_conf (depConfig config) {
		if (wd_server_env() != 1)
			return;

		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "[DEPS] Create Depends '%s' into '%s'\n",
				 config.dep_added, config.dep_config);

		FILE* c_file = fopen(config.dep_config, "r");
		if (c_file) {
			char line[256];
			int t_exist = 0;
			int tr_exist = 0;
			int tr_ln_has_tx = 0;

			while (fgets(line, sizeof(line), c_file)) {
				line[strcspn(line, "\n")] = 0;
				if (strstr(line, config.dep_added) != NULL) {
					t_exist = 1;
				}
				if (strstr(line, config.dep_target) != NULL) {
					tr_exist = 1;
					if (strstr(line, config.dep_added) != NULL) {
						tr_ln_has_tx = 1;
					}
				}
			}
			fclose(c_file);

			if (t_exist) {
				return;
			}

			if (tr_exist && !tr_ln_has_tx) {
				FILE* temp_file = fopen(".deps_tmp", "w");
				c_file = fopen(config.dep_config, "r");

				while (fgets(line, sizeof(line), c_file)) {
					char clean_line[256];
					strcpy(clean_line, line);
					clean_line[strcspn(clean_line, "\n")] = 0;

					if (strstr(clean_line, config.dep_target) != NULL &&
						strstr(clean_line, config.dep_added) == NULL) {
						fprintf(temp_file, "%s %s\n", clean_line, config.dep_added);
					} else {
						fputs(line, temp_file);
					}
				}

				fclose(c_file);
				fclose(temp_file);

				remove(config.dep_config);
				rename(".deps_tmp", config.dep_config);
			} else if (!tr_exist) {
				c_file = fopen(config.dep_config, "a");
				fprintf(c_file, "%s %s\n", config.dep_target, config.dep_added);
				fclose(c_file);
			}

		} else {
			c_file = fopen(config.dep_config, "w");
			fprintf(c_file, "%s %s\n",
					config.dep_target,
					config.dep_added);
			fclose(c_file);
		}

		return;
}
#define S_ADD_PLUGIN(x, y, z) \
    dep_implementation_samp_conf((depConfig){x, y, z})

void dep_implementation_omp_conf (const char* config_name, const char* deps_name) {
		if (wd_server_env() != 2)
			return;

		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "[DEPS] Create Depends '%s' into '%s'\n",
				 deps_name, config_name);

		FILE* c_file = fopen(config_name, "r");
		cJSON* s_root = NULL;

		if (!c_file) {
			s_root = cJSON_CreateObject();
		} else {
			fseek(c_file, 0, SEEK_END);
			long file_size = ftell(c_file);
			fseek(c_file, 0, SEEK_SET);

			char* buffer = (char*)wd_malloc(file_size + 1);
			if (!buffer) {
				pr_error(stdout, "Memory allocation failed!");
				fclose(c_file);
				return;
			}

			fread(buffer, 1, file_size, c_file);
			buffer[file_size] = '\0';
			fclose(c_file);

			s_root = cJSON_Parse(buffer);
			wd_free(buffer);

			if (!s_root) {
				s_root = cJSON_CreateObject();
			}
		}

		cJSON* pawn = cJSON_GetObjectItem(s_root, "pawn");
		if (!pawn) {
			pawn = cJSON_CreateObject();
			cJSON_AddItemToObject(s_root, "pawn", pawn);
		}

		cJSON* legacy_plugins;
		legacy_plugins = cJSON_GetObjectItem(pawn, "legacy_plugins");
		if (!legacy_plugins) {
			legacy_plugins = cJSON_CreateArray();
			cJSON_AddItemToObject(pawn,
								  "legacy_plugins",
								  legacy_plugins);
		}

		if (!cJSON_IsArray(legacy_plugins)) {
			cJSON_DeleteItemFromObject(pawn, "legacy_plugins");
			legacy_plugins = cJSON_CreateArray();
			cJSON_AddItemToObject(pawn,
								  "legacy_plugins",
								  legacy_plugins);
		}

		cJSON* item;
		int p_exist = 0;

		cJSON_ArrayForEach(item, legacy_plugins) {
			if (cJSON_IsString(item) &&
				!strcmp(item->valuestring, deps_name)) {
				p_exist = 1;
				break;
			}
		}

		if (!p_exist) {
			cJSON *size_deps_name = cJSON_CreateString(deps_name);
			cJSON_AddItemToArray(legacy_plugins, size_deps_name);
		}

		char* __cJSON_Printed = cJSON_Print(s_root);
		c_file = fopen(config_name, "w");
		if (c_file) {
			fputs(__cJSON_Printed, c_file);
			fclose(c_file);
		}

		cJSON_Delete(s_root);
		wd_free(__cJSON_Printed);

		return;
}
#define M_ADD_PLUGIN(x, y) dep_implementation_omp_conf(x, y)

void dep_add_include (const char *modes,
					  char *dep_name,
					  char *dep_after) {
		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "[DEPS] Create include '%s' into '%s' after '%s'\n",
				 dep_name, modes, dep_after);

		FILE *m_file = fopen(modes, "r");
		if (m_file == NULL) {
			return;
		}

		fseek(m_file, 0, SEEK_END);
		long fileSize = ftell(m_file);
		fseek(m_file, 0, SEEK_SET);

		char *ct_modes = wd_malloc(fileSize + 1);
		fread(ct_modes, 1, fileSize, m_file);
		ct_modes[fileSize] = '\0';
		fclose(m_file);

		if (strstr(ct_modes, dep_name) != NULL &&
			strstr(ct_modes, dep_after) != NULL) {
			wd_free(ct_modes);
			return;
		}

		char *e_dep_after_pos = strstr(ct_modes,
					   				   dep_after);

		FILE *n_file = fopen(modes, "w");
		if (n_file == NULL) {
			wd_free(ct_modes);
			return;
		}

		if (e_dep_after_pos != NULL) {
			fwrite(ct_modes,
				   1,
				   e_dep_after_pos - ct_modes + strlen(dep_after),
				   n_file);
			fprintf(n_file, "\n%s", dep_name);
			fputs(e_dep_after_pos + strlen(dep_after), n_file);
		} else {
			char *includeToAddPos = strstr(ct_modes, dep_name);

			if (includeToAddPos != NULL) {
				fwrite(ct_modes,
					   1,
					   includeToAddPos - ct_modes + strlen(dep_name),
					   n_file);
				fprintf(n_file, "\n%s", dep_after);
				fputs(includeToAddPos + strlen(dep_name), n_file);
			} else {
				fprintf(n_file, "%s\n%s\n%s", dep_after, dep_name, ct_modes);
			}
		}

		fclose(n_file);
		wd_free(ct_modes);

		return;
}
#define DEP_ADD_INCLUDES(x, y, z) dep_add_include(x, y, z)

const char *dep_get_fname (const char *path)
{
		/* file name */
		const char *depends = strrchr(path, '/');
		if (!depends)
				depends = strrchr(path, '\\');
		return depends ? depends + 1 : path;
}

static const char *dep_get_bname (const char *path)
{
		/* base name */
		const char *p1 = strrchr(path, '/'),
				   *p2 = strrchr(path, '\\'),
				   *p = NULL;

		if (p1 && p2) {
			p = (p1 > p2) ? p1 : p2;
		} else if (p1) {
			p = p1;
		} else if (p2) {
			p = p2;
		}

		return p ? p + 1 : path;
}

static void dep_pr_include_directive (const char *deps_include)
{
		char errbuf[256];
		toml_table_t *_toml_config;
		char depends_name[PATH_MAX];
		char idirective[MAX_PATH];

		const char *dep_n = dep_get_fname(deps_include);
		snprintf(depends_name, sizeof(depends_name), "%s", dep_n);

		const char
			*direct_bnames = dep_get_bname(depends_name);

		FILE *procc_f = fopen("watchdogs.toml", "r");
		_toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
		if (procc_f) fclose(procc_f);

		if (!_toml_config) {
			pr_error(stdout, "parsing TOML: %s", errbuf);
			return;
		}

		toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
		if (wd_compiler) {
			toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
			if (toml_gm_i.ok) {
				wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
				wd_free(toml_gm_i.u.s);
			}
		}
		toml_free(_toml_config);

		snprintf(idirective, sizeof(idirective),
				"#include <%s>", direct_bnames);

		if (wd_server_env() == 1) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		} else if (wd_server_env() == 2) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <open.mp>");
		} else {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		}
}

void deps_print_file_type (const char *path, const char *pattern,
						   const char *exclude, const char *cwd,
						   const char *target_dir, int root)
{
		char cp_cmd[MAX_PATH * 2];
		char json_item[PATH_MAX];

		wd_sef_fdir_reset();

		int found = wd_sef_fdir(path, pattern, exclude);

		if (found) {
			for (int i = 0; i < wcfg.wd_sef_count; ++i) {
				const char *deps_names = dep_get_fname(wcfg.wd_sef_found_list[i]),
						   *deps_bnames = dep_get_bname(wcfg.wd_sef_found_list[i]);

				if (target_dir[0] != '\0') {
					if (is_native_windows())
						snprintf(cp_cmd, sizeof(cp_cmd),
							"move /Y \"%s\" \"%s\\%s\\\"",
							wcfg.wd_sef_found_list[i], cwd, target_dir);
					else
						snprintf(cp_cmd, sizeof(cp_cmd), "mv -f \"%s\" \"%s/%s/\"",
							wcfg.wd_sef_found_list[i], cwd, target_dir);
				} else {
					if (is_native_windows())
						snprintf(cp_cmd, sizeof(cp_cmd),
							"move /Y \"%s\" \"%s\"",
							wcfg.wd_sef_found_list[i], cwd);
					else
						snprintf(cp_cmd, sizeof(cp_cmd),
							"mv -f \"%s\" \"%s\"",
							wcfg.wd_sef_found_list[i], cwd);
				}

				system(cp_cmd);

				snprintf(json_item, sizeof(json_item), "%s", deps_names);
				dep_add_ncheck_hash(json_item, json_item);

				if (root == 1)
					goto done;

				if (wd_server_env() == 1 &&
					  strfind(wcfg.wd_toml_config, "cfg"))
samp_label:
						S_ADD_PLUGIN(wcfg.wd_toml_config,
							"plugins", deps_bnames);
				else if (wd_server_env() == 2 &&
								strfind(wcfg.wd_toml_config, "json"))
								M_ADD_PLUGIN(wcfg.wd_toml_config,
											 deps_bnames);
				else
						goto samp_label;
			}
		}

done:
		return;
}

void dep_cjson_additem (cJSON *p1, int p2, cJSON *p3)
{
		cJSON *_e_item = cJSON_GetArrayItem(p1, p2);
		if (cJSON_IsString(_e_item))
			cJSON_AddItemToArray(p3,
				cJSON_CreateString(_e_item->valuestring));
}

void dep_move_files (const char *dep_dir)
{
		DIR *dir;
		struct dirent *item;
		struct stat st;
		FILE* jfile = NULL;
		FILE *e_file, *fp_cache;
		char dp_fp[PATH_MAX], dp_fc[PATH_MAX], dp_inc[PATH_MAX];
		char *dep_inc_path = NULL;
		char cp_cmd[MAX_PATH * 2];
		char d_b[MAX_PATH];
		char rm_cmd[PATH_MAX];
		int i, _include_search = 0;
		long fp_cache_sz;

		snprintf(dp_fp, sizeof(dp_fp), "%s/plugins", dep_dir);
		snprintf(dp_fc, sizeof(dp_fc), "%s/components", dep_dir);

		if (wd_server_env() == 1) {
			dep_inc_path = "pawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/pawno/include", dep_dir);
		} else if (wd_server_env() == 2) {
			dep_inc_path = "qawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/qawno/include", dep_dir);
		} else {
			dep_inc_path = "pawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/pawno/include", dep_dir);
		}

		char *cwd = wd_get_cwd();

		deps_print_file_type(dp_fp, "*.dll", NULL, cwd, "plugins", 0);
		deps_print_file_type(dp_fp, "*.so", NULL, cwd, "plugins", 0);
		deps_print_file_type(dep_dir, "*.dll", "plugins", cwd, "", 1);
		deps_print_file_type(dep_dir, "*.so", "plugins", cwd, "", 1);

		if (wd_server_env() == 2) {
			snprintf(d_b, sizeof(d_b), "qawno/include");
			deps_print_file_type(dp_fc, "*.dll", NULL, cwd, "components", 0);
			deps_print_file_type(dp_fc, "*.so", NULL, cwd, "components", 0);
		} else {
			snprintf(d_b, sizeof(d_b), "pawno/include");
		}

		int stack_size = 500;
		int stack_top = -1;
		char **dir_stack = wd_malloc(stack_size * sizeof(char*));

		for (int i = 0; i < stack_size; i++) {
		    dir_stack[i] = wd_malloc(PATH_MAX);
		}

		stack_top++;
		snprintf(dir_stack[stack_top], PATH_MAX, "%s", dep_dir);

		while (stack_top >= 0) {
		    char current_dir[PATH_MAX];
		    snprintf(current_dir, sizeof(current_dir), "%s",
						dir_stack[stack_top]);
		    stack_top--;

		    DIR *dir = opendir(current_dir);
		    if (!dir) continue;

		    struct dirent *item;
		    struct stat st;
		    char fpath[PATH_MAX];

			while ((item = readdir(dir)) != NULL) {
				if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
					continue;

					snprintf(fpath, sizeof(fpath), "%s/%s", current_dir,
													item->d_name);

					if (stat(fpath, &st) != 0) continue;

					if (S_ISDIR(st.st_mode)) {
						if (stack_top < stack_size - 1) {
							stack_top++;
							snprintf(dir_stack[stack_top], PATH_MAX, "%s", fpath);
						}
						continue;
					}

					ext = strrchr(item->d_name, '.');
					if (!ext || strcmp(ext, ".inc"))
							continue;

						snprintf(parent, sizeof(parent), "%s", current_dir);
						dname = strrchr(parent, '/');
						if (!dname)
								continue;

						++dname; /* skip '/' */

						snprintf(dest, sizeof(dest), "%s/%s", d_b, dname);

						if (rename(parent, dest)) {
								if (is_native_windows()) {
										snprintf(cmd, sizeof(cmd),
											    "xcopy \"%s\" \"%s\" /E /I /H /Y >nul 2>&1 && "
											    "rmdir /S /Q \"%s\" >nul 2>&1",
											    parent, dest, parent);
								} else {
										snprintf(cmd, sizeof(cmd),
												"cp -r \"%s\" \"%s\" && rm -rf \"%s\"",
												parent, dest, parent);
								}
								if (system(cmd)) {
										pr_error(stdout, "Failed to move folder: %s\n", parent);
										continue;
								}
						}

						dep_add_ncheck_hash(dest, dest);

						pr_info(stdout, "\tmoved folder: %s to %s/\n",
								dname, !strcmp(wcfg.wd_is_omp, CRC32_TRUE) ?
								"qawno/include" : "pawno/include");
				}

		    closedir(dir);
		}

		for (int i = 0; i < stack_size; i++) {
		    wd_free(dir_stack[i]);
		}
		wd_free(dir_stack);

		wd_sef_fdir_reset();

		_include_search = wd_sef_fdir(dp_inc, "*.inc", NULL);
		if (_include_search) {
			for (i = 0; i < wcfg.wd_sef_count; ++i) {
				const char *fi_depends_name;
				fi_depends_name = dep_get_fname(wcfg.wd_sef_found_list[i]);

				if (is_native_windows())
					snprintf(cp_cmd, sizeof(cp_cmd),
						"move /Y \"%s\" \"%s\\%s\\\"",
						wcfg.wd_sef_found_list[i], cwd, dep_inc_path);
				else
					snprintf(cp_cmd, sizeof(cp_cmd),
						"mv -f \"%s\" \"%s/%s/\"",
						wcfg.wd_sef_found_list[i], cwd, dep_inc_path);

				system(cp_cmd);

				dep_add_ncheck_hash(fi_depends_name, fi_depends_name);
				dep_pr_include_directive(fi_depends_name);
			}
		}

		if (is_native_windows())
			snprintf(rm_cmd, sizeof(rm_cmd),
				"rmdir /s /q \"%s\"",
				dep_dir);
		else
			snprintf(rm_cmd, sizeof(rm_cmd),
				"rm -rf %s",
				dep_dir);
		system(rm_cmd);

		wd_main(NULL);
}

void wd_apply_depends (const char *depends_name)
{
		char _depends[PATH_MAX];
		char dep_dir[PATH_MAX];
		struct stat st;
		char *f_EXT;

		snprintf(_depends, PATH_MAX, "%s", depends_name);
		f_EXT = strrchr(_depends, '.');
		if (f_EXT)
			*f_EXT = '\0';

		snprintf(dep_dir, sizeof(dep_dir), "%s", _depends);

		if (wd_server_env() == 1) {
			if (stat("pawno/include", &st) != 0 && errno == ENOENT)
				mkdir_recusrs("pawno/include");
			if (stat("plugins", &st) != 0 && errno == ENOENT)
				mkdir_recusrs("plugins");
		} else if (wd_server_env() == 2) {
			if (stat("qawno/include", &st) != 0 && errno == ENOENT)
				mkdir_recusrs("qawno/include");

			if (stat("components", &st) != 0 && errno == ENOENT)
				mkdir_recusrs("components");

			if (stat("plugins", &st) != 0 && errno == ENOENT)
				mkdir_recusrs("plugins");
		}

		dep_move_files(dep_dir);

		return;
}

void wd_install_depends (const char *depends_string)
{
		char buffer[1024];
		const char *depends[MAX_DEPENDS];
		char *depends_files[MAX_DEPENDS];
		int dep_item_found = 0;
		char dep_url[1024] = { 0 };
		char dep_repo_name[256] = { 0 };
		struct dep_repo_info dep_repo_info;
		const char *chr_last_slash;
		char *token;
		int DEPS_COUNT = 0;
		int file_count = 0;
		wcfg.wd_idepends = 0;

		if (!depends_string || !*depends_string) {
			pr_color(stdout, FCOLOUR_RED, "[DEPS] ");
			printf("no valid dependencies to install!");
			goto done;
		}

		snprintf(buffer, sizeof(buffer), "%s", depends_string);
		token = strtok(buffer, " ");

		while (token && DEPS_COUNT < MAX_DEPENDS) {
			depends[DEPS_COUNT++] = token;
			token = strtok(NULL, " ");
		}
		if (DEPS_COUNT == 0) {
			pr_color(stdout, FCOLOUR_RED, "[DEPS] ");
			printf("no valid depends to install!");
			goto done;
		}
		int i;
		for (i = 0; i < DEPS_COUNT; i++) {
			if (!dep_parse_repo(depends[i], &dep_repo_info)) {
				pr_color(stdout, FCOLOUR_RED, "[DEPS] ");
				printf("invalid repo format: %s\n", depends[i]);
				continue;
			}
			if (!strcmp(dep_repo_info.host, "github")) {
				dep_item_found = dep_handle_repo(&dep_repo_info,
												dep_url,
												sizeof(dep_url));
			} else {
				dep_build_repo_url(&dep_repo_info, 0, dep_url, sizeof(dep_url));
				dep_item_found = dep_check_url(dep_url, wcfg.wd_toml_github_tokens);
			}
			if (!dep_item_found) {
				pr_color(stdout, FCOLOUR_RED, "[DEPS] ");
				printf("repo not found: %s\n", depends[i]);
				continue;
			}

			chr_last_slash = strrchr(dep_url, '/');
			if (chr_last_slash &&
				*(chr_last_slash + 1))
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s",
						chr_last_slash + 1);
			else
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s.zip",
						dep_repo_info.repo);

			wcfg.wd_idepends = 1;

			if (wd_download_file(dep_url, dep_repo_name) == __RETZ)
				depends_files[file_count++] = strdup(dep_repo_name);
		}

		for (i = 0; i < file_count; i++) {
			wd_apply_depends(depends_files[i]);
			wd_free(depends_files[i]);
		}

done:
		wd_main(NULL);
}
