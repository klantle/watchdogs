#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "wg_extra.h"
#include "wg_util.h"
#include "wg_curl.h"
#include "wg_archive.h"
#include "wg_crypto.h"
#include "wg_unit.h"
#include "wg_depends.h"

char working[WG_PATH_MAX * 2];
char *tag_pointer, *path, *first_slash;
char *user, *repo_slash, *repo, *git_ext;
char cmd[WG_MAX_PATH * 4], rm_cmd[WG_PATH_MAX];
char *depends_filename, *extension;

void dep_sym_convert (char *path)
{
		char *p;
		for (p = path; *p; p++) {
				if (*p == __PATH_CHR_SEP_WIN32)
					*p = __PATH_CHR_SEP_LINUX;
		}
}

const char *dep_get_fname (const char *path) {
	    const char *depends = PATH_SEPARATOR(path);
	    return depends ? depends + 1 : path;
}

static const char *dep_get_bname (const char *path) {
	    const char *p = PATH_SEPARATOR(path);
	    return p ? p + 1 : path;
}

static size_t dep_curl_write_cb (void *contents, size_t size, size_t nmemb, void *userp){
		struct dep_curl_buffer *mem = (struct dep_curl_buffer *)userp;
		size_t realsize = size * nmemb;
		char *ptr;

		ptr = wg_realloc(mem->data, mem->size + realsize + 1);
		if (!ptr)
				return WG_RETZ;

		mem->data = ptr;
		memcpy(&mem->data[mem->size], contents, realsize);
		mem->size += realsize;
		mem->data[mem->size] = 0;

		return realsize;
}

int dep_url_checking (const char *url, const char *github_token)
{
		CURL *curl = curl_easy_init();
		if (!curl) return WG_RETZ;

		CURLcode res;
		long response_code = 0;
		struct curl_slist *headers = NULL;
		char error_buffer[CURL_ERROR_SIZE] = { 0 };

		printf("\tCreate & Checking URL: %s...\t\t[V]\n", url);
		if (strstr(wgconfig.wg_toml_github_tokens, "DO_HERE") ||
				   wgconfig.wg_toml_github_tokens == NULL ||
				   strlen(wgconfig.wg_toml_github_tokens) < 1) {
			pr_color(stdout, FCOLOUR_GREEN, "Can't read Github token.. skipping\n");
			sleep(2);
		} else {
				char auth_header[512];
				wg_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", github_token);
				headers = curl_slist_append(headers, auth_header);
		}

		headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
		headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

		printf("   Connecting... ");
		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

		verify_cacert_pem(curl);

		fflush(stdout);
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		if (response_code == WG_CURL_RESPONSE_OK && strlen(error_buffer) == WG_RETZ) {
		    printf("cURL result: %s\t\t[V]\n", curl_easy_strerror(res));
		    printf("Response code: %ld\t\t[V]\n", response_code);
		} else {
		    if (strlen(error_buffer) > 0) {
		        printf("Error: %s\t\t[X]\n", error_buffer);
		    } else {
		        printf("cURL result: %s\t\t[X]\n", curl_easy_strerror(res));
		    }
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		return (response_code >= 200 && response_code < 300);
}

int dep_http_get_content (const char *url, const char *github_token, char **out_html)
{
        CURL *curl;
        struct dep_curl_buffer buffer = { 0 };
        CURLcode res;
        struct curl_slist *headers = NULL;

        curl = curl_easy_init();
        if (!curl)
            return WG_RETZ;

        if (github_token && strlen(github_token) > 0 && !strstr(github_token, "DO_HERE")) {
            char auth_header[512];
            wg_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", github_token);
            headers = curl_slist_append(headers, auth_header);
        }

        headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dep_curl_write_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        verify_cacert_pem(curl);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK || buffer.size == WG_RETZ) {
            wg_free(buffer.data);
            return WG_RETZ;
        }

        *out_html = buffer.data;
        return WG_RETN;
}

static int is_os_specific_archive (const char *filename)
{
	    const char *os_patterns[] = {
"windows", "win", ".exe", "msvc", "mingw",
"macos", "mac", "darwin", "osx", "linux",
"ubuntu", "debian", "cent", "centos", "cent_os", "fedora", "arch", "archlinux",
"alphine", "redhat", "redhatenterprise", "redhatenterpriselinux", "linuxmint", "mint",
"x86", "x64", "x86_64", "amd64", "arm", "arm64", "aarch64",
NULL
	    };
	    
	    for (int i = 0; os_patterns[i] != NULL; i++) {
	        if (strstr(filename, os_patterns[i])) {
	            return WG_RETN;
	        }
	    }
	    return WG_RETZ;
}

static int is_gamemode_archive (const char *filename)
{
	    const char *gamemode_patterns[] = {
"server", "_server",
NULL
	    };
	    
	    for (int i = 0; gamemode_patterns[i] != NULL; i++) {
	        if (strstr(filename, gamemode_patterns[i])) {
	            return WG_RETN;
	        }
	    }
	    return WG_RETZ;
}

static char *dep_get_assets (char **deps_assets, int cnt, const char *preferred_os)
{
	    int i, j;
	    const char *os_patterns[] = {
#ifdef WG_WINDOWS
"windows", "win", ".exe", "msvc", "mingw",
#elif defined(__APPLE__)
"macos", "mac", "darwin", "osx", 
#else
"ubuntu", "debian", "cent", "centos", "cent_os", "fedora", "arch", "archlinux",
"alphine", "redhat", "redhatenterprise", "redhatenterpriselinux", "linuxmint", "mint",
#endif
"src", "source"
	    };
	    size_t num_patterns = sizeof(os_patterns) / sizeof(os_patterns[0]);

	    if (cnt == WG_RETZ)
	        return NULL;
	    if (cnt == WG_RETN)
	        return strdup(deps_assets[0]);

	    for (i = 0; i < cnt; i++) {
	        if (strstr(deps_assets[i], "server") || 
	        	strstr(deps_assets[i], "_server")) {
	            return strdup(deps_assets[i]);
	        }
	    }

	    for (i = 0; i < num_patterns; i++) {
	        for (j = 0; j < cnt; j++) {
	            if (strstr(deps_assets[j], os_patterns[i]))
	                return strdup(deps_assets[j]);
	        }
	    }

	    for (i = 0; i < cnt; i++) {
	        int has_os_pattern = 0;
	        for (j = 0; j < num_patterns; j++) {
	            if (strstr(deps_assets[i], os_patterns[j])) {
	                has_os_pattern = 1;
	                break;
	            }
	        }
	        if (!has_os_pattern) {
	            return strdup(deps_assets[i]);
	        }
	    }

	    return strdup(deps_assets[0]);
}

static int
dep_parse_repo (const char *input, struct dep_repo_info *__deps_data)
{
		memset(__deps_data, 0, sizeof(*__deps_data));

		wg_strcpy(__deps_data->host, "github");
		wg_strcpy(__deps_data->domain, "github.com");

		wg_strncpy(working, input, sizeof(working) - 1);
		working[sizeof(working) - 1] = '\0';

		tag_pointer = strrchr(working, ':');
		if (tag_pointer) {
			*tag_pointer = '\0';
			wg_strncpy(__deps_data->tag, tag_pointer + 1, sizeof(__deps_data->tag) - 1);

			if (!strcmp(__deps_data->tag, "latest")) {
				printf("latest tags ");
				pr_color(stdout, FCOLOUR_CYAN, ":latest ");
				printf("detected, will use latest release..\t\t[V]\n");
			}
		}

		path = working;

		if (!strncmp(path, "https://", 8))
			path += 8;
		else if (!strncmp(path, "http://", 7))
			path += 7;

		if (!strncmp(path, "github/", 7)) {
			wg_strcpy(__deps_data->host, "github");
			wg_strcpy(__deps_data->domain, "github.com");
			path += 7;
		} else {
			first_slash = strchr(path, __PATH_CHR_SEP_LINUX);
			if (first_slash && strchr(path, '.') && strchr(path, '.') < first_slash) {
				char domain[128];

				wg_strncpy(domain, path, first_slash - path);
				domain[first_slash - path] = '\0';

				if (strstr(domain, "github")) {
					wg_strcpy(__deps_data->host, "github");
					wg_strcpy(__deps_data->domain, "github.com");
				} else {
					wg_strncpy(__deps_data->domain, domain,
						sizeof(__deps_data->domain) - 1);
					wg_strcpy(__deps_data->host, "custom");
				}

				path = first_slash + 1;
			}
		}

		user = path;
		repo_slash = strchr(path, __PATH_CHR_SEP_LINUX);
		if (!repo_slash)
			return WG_RETZ;

		*repo_slash = '\0';
		repo = repo_slash + 1;

		wg_strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);

		git_ext = strstr(repo, ".git");
		if (git_ext)
			*git_ext = '\0';
		wg_strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);

		if (strlen(__deps_data->user) == WG_RETZ || strlen(__deps_data->repo) == WG_RETZ)
			return WG_RETZ;

#if defined(_DBG_PRINT)
		char size_title[WG_PATH_MAX * 3];
		wg_snprintf(size_title, sizeof(size_title), "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
			__deps_data->host,
			__deps_data->domain,
			__deps_data->user,
			__deps_data->repo,
			__deps_data->tag[0] ? __deps_data->tag : "(none)");
		wg_console_title(size_title);
#endif

		return WG_RETN;
}

static int dep_gh_release_assets (const char *user, const char *repo,
								  const char *tag, char **out_urls, int max_urls)
{
		char api_url[WG_PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int url_count = 0;

		wg_snprintf(api_url, sizeof(api_url),
				 "%sapi.github.com/repos/%s/%s/releases/tags/%s",
				 "https://", user, repo, tag);

		if (!dep_http_get_content(api_url, wgconfig.wg_toml_github_tokens, &json_data))
				return WG_RETZ;

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
				out_urls[url_count] = wg_malloc(url_len + 1);
				wg_strncpy(out_urls[url_count], p, url_len);
				out_urls[url_count][url_len] = '\0';

				++url_count;
				p = url_end + 1;
		}

		wg_free(json_data);
		return url_count;
}

static void
dep_build_repo_url (const struct dep_repo_info *__deps_data, int is_tag_page,
		   			char *deps_put_url, size_t deps_put_size)
{
		char deps_actual_tag[128] = { 0 };

		if (__deps_data->tag[0]) {
			wg_strncpy(deps_actual_tag, __deps_data->tag,
				sizeof(deps_actual_tag) - 1);

			if (strcmp(deps_actual_tag, "latest") == WG_RETZ) {
				if (strcmp(__deps_data->host, "github") == WG_RETZ && !is_tag_page) {
					wg_strcpy(deps_actual_tag, "latest");
				}
			}
		}

		if (strcmp(__deps_data->host, "github") == WG_RETZ) {
			if (is_tag_page && deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					wg_snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/latest",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo);
				} else {
					wg_snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/tag/%s",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else if (deps_actual_tag[0]) {
				if (!strcmp(deps_actual_tag, "latest")) {
					wg_snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/releases/latest",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo);
				} else {
					wg_snprintf(deps_put_url, deps_put_size,
						"https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
						__deps_data->domain,
						__deps_data->user,
						__deps_data->repo,
						deps_actual_tag);
				}
			} else {
				wg_snprintf(deps_put_url, deps_put_size,
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
		char api_url[WG_PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int ret = 0;

		wg_snprintf(api_url, sizeof(api_url),
				"%sapi.github.com/repos/%s/%s/releases/latest",
				"https://", user, repo);

		if (!dep_http_get_content(api_url, wgconfig.wg_toml_github_tokens, &json_data))
			return WG_RETZ;

		p = strstr(json_data,
				   "\"tag_name\"");
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
							wg_strncpy(out_tag, p, tag_len);
							out_tag[tag_len] = '\0';
							ret = 1;
						}
					}
				}
			}
		}

		wg_free(json_data);
		return ret;
}

static int dep_handle_repo(const struct dep_repo_info *dep_repo_info,
                          char *deps_put_url,
                          size_t deps_put_size)
{
	    int ret = 0;
	    const char *deps_repo_branch[] = {"main", "master"};
	    char deps_actual_tag[128] = {0};
	    int use_fallback_branch = 0;

	    if (dep_repo_info->tag[0] && strcmp(dep_repo_info->tag, "latest") == WG_RETZ) {
	        if (dep_gh_latest_tag(dep_repo_info->user,
	                            dep_repo_info->repo,
	                            deps_actual_tag,
	                            sizeof(deps_actual_tag))) {
	            printf("Create latest tag: %s (~instead of latest)\t\t[V]\n", deps_actual_tag);
	        } else {
	            pr_error(stdout, "Failed to get latest tag for %s/%s,"
	                             "Falling back to main branch\t\t[X]",
	                             dep_repo_info->user, dep_repo_info->repo);
	            use_fallback_branch = 1;
	        }
	    } else {
	        wg_strncpy(deps_actual_tag, dep_repo_info->tag, sizeof(deps_actual_tag) - 1);
	    }

	    if (use_fallback_branch) {
	        for (int j = 0; j < 2 && !ret; j++) {
	            wg_snprintf(deps_put_url, deps_put_size,
	                    "https://github.com/%s/%s/archive/refs/heads/%s.zip",
	                    dep_repo_info->user,
	                    dep_repo_info->repo,
	                    deps_repo_branch[j]);

	            if (dep_url_checking(deps_put_url, wgconfig.wg_toml_github_tokens)) {
	                ret = 1;
	                if (j == WG_RETN)
	                    printf("Create master branch (main branch not found)\t\t[V]\n");
	            }
	        }
	        return ret;
	    }

	    if (deps_actual_tag[0]) {
	        char *deps_assets[10] = {0};
	        int deps_asset_count = dep_gh_release_assets(dep_repo_info->user,
	                                                     dep_repo_info->repo,
	                                                     deps_actual_tag,
	                                                     deps_assets,
	                                                     10);

	        if (deps_asset_count > 0) {
	            char *deps_best_asset = NULL;
	            deps_best_asset = dep_get_assets(deps_assets, deps_asset_count, NULL);

	            if (deps_best_asset) {
	                wg_strncpy(deps_put_url, deps_best_asset, deps_put_size - 1);
	                ret = 1;
	                
	                if (is_gamemode_archive(deps_best_asset)) {
	                    printf("Selected gamemode archive: %s\t\t[V]\n", deps_best_asset);
	                } else if (is_os_specific_archive(deps_best_asset)) {
	                    printf("Selected OS-specific archive: %s\t\t[V]\n", deps_best_asset);
	                } else {
	                    printf("Selected neutral archive: %s\t\t[V]\n", deps_best_asset);
	                }
	                
	                wg_free(deps_best_asset);
	            }

	            for (int j = 0; j < deps_asset_count; j++)
	                wg_free(deps_assets[j]);
	        }

	        if (!ret) {
	            const char *deps_arch_format[] = {
	                "https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
	                "https://github.com/%s/%s/archive/refs/tags/%s.zip"
	            };

	            for (int j = 0; j < 2 && !ret; j++) {
	                wg_snprintf(deps_put_url, deps_put_size, deps_arch_format[j],
	                         dep_repo_info->user,
	                         dep_repo_info->repo,
	                         deps_actual_tag);

	                if (dep_url_checking(deps_put_url, wgconfig.wg_toml_github_tokens))
	                    ret = 1;
	            }
	        }
	    } else {
	        for (int j = 0; j < 2 && !ret; j++) {
	            wg_snprintf(deps_put_url, deps_put_size,
	                    "%s%s/%s/archive/refs/heads/%s.zip",
	                    "https://github.com/",
	                    dep_repo_info->user,
	                    dep_repo_info->repo,
	                    deps_repo_branch[j]);

	            if (dep_url_checking(deps_put_url, wgconfig.wg_toml_github_tokens)) {
	                ret = 1;
	                if (j == WG_RETN)
	                    printf("Create master branch (main branch not found)\t\t[V]\n");
	            }
	        }
	    }

	    return ret;
}

int dep_add_ncheck_hash (const char *raw_file_path, const char *raw_json_path)
{
		char res_convert_f_path[WG_PATH_MAX];
		char res_convert_json_path[WG_PATH_MAX];

		wg_strncpy(res_convert_f_path, raw_file_path, sizeof(res_convert_f_path));
			res_convert_f_path[sizeof(res_convert_f_path) - 1] = '\0';
		dep_sym_convert(res_convert_f_path);
		wg_strncpy(res_convert_json_path, raw_json_path, sizeof(res_convert_json_path));
			res_convert_json_path[sizeof(res_convert_json_path) - 1] = '\0';
		dep_sym_convert(res_convert_json_path);


		if (strfind(res_convert_json_path, "pawno") ||
			strfind(res_convert_json_path, "qawno"))
			goto done;

        unsigned char sha1_hash[20];
        int k = crypto_generate_sha1_hash(res_convert_json_path, sha1_hash);
        if (k != WG_RETN)
        	goto done;

		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Create hash (CRC32) for '%s': ",
				 res_convert_json_path);
		crypto_print_hex(sha1_hash, sizeof(sha1_hash), 0);
		printf("\t\t[V]\n");


done:
		return WG_RETN;
}

void dep_implementation_samp_conf (depConfig config) {
		if (wg_server_env() != 1)
			return;

		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Create Depends '%s' into '%s'\t\t[V]\n",
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
				FILE* temp_file = fopen(".watchdogs/depends_tmp", "w");
				c_file = fopen(config.dep_config, "r");

				while (fgets(line, sizeof(line), c_file)) {
					char clean_line[256];
					wg_strcpy(clean_line, line);
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
				rename(".watchdogs/depends_tmp", config.dep_config);
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
		if (wg_server_env() != 2)
			return;

		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Create Depends '%s' into '%s'\t\t[V]\n",
				 deps_name, config_name);

		FILE* c_file = fopen(config_name, "r");
		cJSON* s_root = NULL;

		if (!c_file) {
			s_root = cJSON_CreateObject();
		} else {
			fseek(c_file, 0, SEEK_END);
			long file_size = ftell(c_file);
			fseek(c_file, 0, SEEK_SET);

			char* buffer = (char*)wg_malloc(file_size + 1);
			if (!buffer) {
				pr_error(stdout, "Memory allocation failed!");
				fclose(c_file);
				return;
			}

			fread(buffer, 1, file_size, c_file);
			buffer[file_size] = '\0';
			fclose(c_file);

			s_root = cJSON_Parse(buffer);
			wg_free(buffer);

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
		wg_free(__cJSON_Printed);

		return;
}
#define M_ADD_PLUGIN(x, y) dep_implementation_omp_conf(x, y)

void dep_add_include (const char *modes,
					  char *dep_name,
					  char *dep_after) {
		if (path_exists(modes) == WG_RETZ) {
			pr_color(stdout, FCOLOUR_GREEN,
				"~ can't found %s.. skipping adding include name\t\t[X]\n", modes);
			return;
		}
		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Create include '%s' into '%s' after '%s'\t\t[V]\n",
				 dep_name, modes, dep_after);

		FILE *m_file = fopen(modes, "r");
		if (m_file == NULL) {
			return;
		}

		fseek(m_file, 0, SEEK_END);
		long fileSize = ftell(m_file);
		fseek(m_file, 0, SEEK_SET);

		char *ct_modes = wg_malloc(fileSize + 1);
		fread(ct_modes, 1, fileSize, m_file);
		ct_modes[fileSize] = '\0';
		fclose(m_file);

		if (strstr(ct_modes, dep_name) != NULL &&
			strstr(ct_modes, dep_after) != NULL) {
			wg_free(ct_modes);
			return;
		}

		char *e_dep_after_pos = strstr(ct_modes,
					   				   dep_after);

		FILE *n_file = fopen(modes, "w");
		if (n_file == NULL) {
			wg_free(ct_modes);
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
		wg_free(ct_modes);

		return;
}
#define DEP_ADD_INCLUDES(x, y, z) dep_add_include(x, y, z)

static void dep_pr_include_directive (const char *deps_include)
{
		char error_buffer[256];
		toml_table_t *wg_toml_config;
		char depends_name[WG_PATH_MAX];
		char idirective[WG_MAX_PATH];

		const char *dep_n = dep_get_fname(deps_include);
		wg_snprintf(depends_name, sizeof(depends_name), "%s", dep_n);

		const char
			*direct_bnames = dep_get_bname(depends_name);

		FILE *proc_f = fopen("watchdogs.toml", "r");
		wg_toml_config = toml_parse_file(proc_f, error_buffer, sizeof(error_buffer));
		if (proc_f) fclose(proc_f);

		if (!wg_toml_config) {
			pr_error(stdout, "parsing TOML: %s", error_buffer);
			return;
		}

		toml_table_t *wg_compiler = toml_table_in(wg_toml_config, "compiler");
		if (wg_compiler) {
			toml_datum_t toml_gm_i = toml_string_in(wg_compiler, "input");
			if (toml_gm_i.ok) {
				wgconfig.wg_toml_gm_input = strdup(toml_gm_i.u.s);
				wg_free(toml_gm_i.u.s);
			}
		}
		toml_free(wg_toml_config);

		wg_snprintf(idirective, sizeof(idirective),
				    "#include <%s>",
				    direct_bnames);

		if (wg_server_env() == WG_RETN) {
			DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		} else if (wg_server_env() == WG_RETW) {
			DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
							 idirective,
							 "#include <open.mp>");
		} else {
			DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		}
}

void dump_file_type (const char *path,
					 char *pattern,
				     char *exclude, char *cwd,
				     char *target_dir, int root)
{
		char cp_cmd[WG_MAX_PATH * 2];
		char json_item[WG_PATH_MAX];

		wg_sef_fdir_reset();

		int found = wg_sef_fdir(path, pattern, exclude);

		if (found) {
			for (int i = 0; i < wgconfig.wg_sef_count; ++i) {
				const char *deps_names  = dep_get_fname(wgconfig.wg_sef_found_list[i]),
						   *deps_bnames = dep_get_bname(wgconfig.wg_sef_found_list[i]);

				if (target_dir[0] != '\0') {
					int _is_win32 = 0;
#ifdef WG_WINDOWS
					_is_win32 = 1;
#endif
					if (_is_win32) {
						wg_snprintf(cp_cmd, sizeof(cp_cmd),
							"move /Y \"%s\" \"%s\\%s\\\"",
							wgconfig.wg_sef_found_list[i], cwd, target_dir);
					} else
						wg_snprintf(cp_cmd, sizeof(cp_cmd), "mv -f \"%s\" \"%s/%s/\"",
							wgconfig.wg_sef_found_list[i], cwd, target_dir);

	                struct timespec start = {0}, end = { 0 };
	                double moving_dur;

	                clock_gettime(CLOCK_MONOTONIC, &start);
					wg_run_command(cp_cmd);
	                clock_gettime(CLOCK_MONOTONIC, &end);

	                moving_dur = (end.tv_sec - start.tv_sec)
	                           + (end.tv_nsec - start.tv_nsec) / 1e9;

	            	pr_color(stdout,
	                         FCOLOUR_CYAN,
	                         " [MOVING] Plugins %s -> %s - %s [Finished at %.3fs]\n",
	                         wgconfig.wg_sef_found_list[i], cwd, target_dir, moving_dur);
				} else {
					int _is_win32 = 0;
#ifdef WG_WINDOWS
					_is_win32 = 1;
#endif
					if (_is_win32) {
							wg_snprintf(cp_cmd, sizeof(cp_cmd),
								"move /Y \"%s\" \"%s\"",
								wgconfig.wg_sef_found_list[i], cwd);
					} else
						wg_snprintf(cp_cmd, sizeof(cp_cmd),
							"mv -f \"%s\" \"%s\"",
							wgconfig.wg_sef_found_list[i], cwd);

	                struct timespec start = {0}, end = { 0 };
	                double moving_dur;

	                clock_gettime(CLOCK_MONOTONIC, &start);
					wg_run_command(cp_cmd);
	                clock_gettime(CLOCK_MONOTONIC, &end);

	                moving_dur = (end.tv_sec - start.tv_sec)
	                           + (end.tv_nsec - start.tv_nsec) / 1e9;

	            	pr_color(stdout,
	                         FCOLOUR_CYAN,
	                         " [MOVING] Plugins %s -> %s [Finished at %.3fs]\n",
	                         wgconfig.wg_sef_found_list[i], cwd, moving_dur);

					wg_snprintf(json_item, sizeof(json_item), "%s", deps_names);
					dep_add_ncheck_hash(json_item, json_item);

					if (root == WG_RETN)
						goto done;

					if (wg_server_env() == WG_RETN &&
						  strfind(wgconfig.wg_toml_config, "cfg"))
samp_label:
							S_ADD_PLUGIN(wgconfig.wg_toml_config,
								"plugins", deps_bnames);
					else if (wg_server_env() == WG_RETW &&
									strfind(wgconfig.wg_toml_config, "json"))
									M_ADD_PLUGIN(wgconfig.wg_toml_config,
												 deps_bnames);
					else
							goto samp_label;
				}
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
		char *deps_include_path = NULL;
		char size_deps_copy[WG_MAX_PATH * 2];
		char include_path[WG_MAX_PATH];
		int i, deps_include_search = 0;
		char plug_path[WG_PATH_MAX],
			 comp_path[WG_PATH_MAX],
			 deps_include_full_path[WG_PATH_MAX],
			 depends_ipath[WG_PATH_MAX * 2],
			 deps_parent_dir[WG_PATH_MAX],
			 dest[WG_MAX_PATH * 2];
#ifdef WG_WINDOWS
		wg_snprintf(plug_path,
				    sizeof(plug_path),
				    "%s\\plugins",
				    dep_dir);
		wg_snprintf(comp_path,
				    sizeof(comp_path),
				    "%s\\components",
				    dep_dir);
#else
		wg_snprintf(plug_path,
				    sizeof(plug_path),
				    "%s\\plugins",
				    dep_dir);
		wg_snprintf(comp_path,
				    sizeof(comp_path),
				    "%s\\components",
				    dep_dir);
#endif

		if (wg_server_env() == WG_RETN) {
#ifdef WG_WINDOWS
			deps_include_path = "pawno\\include";
			wg_snprintf(deps_include_full_path,
					sizeof(deps_include_full_path), "%s\\pawno\\include", dep_dir);
#else
			deps_include_path = "pawno/include";
			wg_snprintf(deps_include_full_path,
					sizeof(deps_include_full_path), "%s/pawno/include", dep_dir);
#endif
		} else if (wg_server_env() == WG_RETW) {
#ifdef WG_WINDOWS
			deps_include_path = "qawno\\include";
			wg_snprintf(deps_include_full_path,
					sizeof(deps_include_full_path), "%s\\qawno\\include", dep_dir);
#else
			deps_include_path = "qawno/include";
			wg_snprintf(deps_include_full_path,
					sizeof(deps_include_full_path), "%s/qawno/include", dep_dir);
#endif
		}

		char *path_pos;
		while ((path_pos = strstr(deps_include_path, "include\\")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include\\")) + 1);
		while ((path_pos = strstr(deps_include_path, "include/")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include/")) + 1);
		while ((path_pos = strstr(deps_include_full_path, "include\\")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include\\")) + 1);
		while ((path_pos = strstr(deps_include_full_path, "include/")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include/")) + 1);

		char *cwd = wg_get_cwd();

#ifndef WG_WINDOWS
		dump_file_type(plug_path, "*.dll", NULL, cwd, "/plugins", 0);
		dump_file_type(plug_path, "*.so", NULL, cwd, "/plugins", 0);
#else
		dump_file_type(plug_path, "*.dll", NULL, cwd, "\\plugins", 0);
		dump_file_type(plug_path, "*.so", NULL, cwd, "\\plugins", 0);
#endif
		dump_file_type(dep_dir, "*.dll", "plugins", cwd, "", 1);
		dump_file_type(dep_dir, "*.so", "plugins", cwd, "", 1);

		if (wg_server_env() == WG_RETW) {
#ifdef WG_WINDOWS
			wg_snprintf(include_path, sizeof(include_path), "qawno\\include");
#else
			wg_snprintf(include_path, sizeof(include_path), "qawno/include");
#endif
			dump_file_type(comp_path, "*.dll", NULL, cwd, "components", 0);
			dump_file_type(comp_path, "*.so", NULL, cwd, "components", 0);
		} else {
#ifdef WG_WINDOWS
			wg_snprintf(include_path, sizeof(include_path), "pawno\\include");
#else
			wg_snprintf(include_path, sizeof(include_path), "pawno/include");
#endif
		}

		while ((path_pos = strstr(include_path, "include\\")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include\\")) + 1);
		while ((path_pos = strstr(include_path, "include/")) != NULL)
			memmove(path_pos, path_pos + strlen("include/"),
				strlen(path_pos + strlen("include/")) + 1);

		wg_sef_fdir_reset();

		deps_include_search = wg_sef_fdir(deps_include_full_path, "*.inc", NULL);
		if (deps_include_search) {
			for (i = 0; i < wgconfig.wg_sef_count; ++i) {
				const char *fi_depends_name;
				fi_depends_name = dep_get_fname(wgconfig.wg_sef_found_list[i]);

				int _is_win32 = 0;
#ifdef WG_WINDOWS
				_is_win32 = 1;
#endif
				if (_is_win32) {
					wg_snprintf(size_deps_copy, sizeof(size_deps_copy),
						"move /Y \"%s\" \"%s\\%s\\\"",
						wgconfig.wg_sef_found_list[i], cwd, deps_include_path);
				} else
					wg_snprintf(size_deps_copy, sizeof(size_deps_copy),
						"mv -f \"%s\" \"%s/%s/\"",
						wgconfig.wg_sef_found_list[i], cwd, deps_include_path);

                struct timespec start = {0}, end = { 0 };
                double moving_dur;

                clock_gettime(CLOCK_MONOTONIC, &start);
				wg_run_command(size_deps_copy);
                clock_gettime(CLOCK_MONOTONIC, &end);

                moving_dur = (end.tv_sec - start.tv_sec)
                           + (end.tv_nsec - start.tv_nsec) / 1e9;

	        	pr_color(stdout,
	                     FCOLOUR_CYAN,
	                     " [MOVING] Include %s -> %s - %s [Finished at %.3fs]\n",
	                     wgconfig.wg_sef_found_list[i], cwd, deps_include_path, moving_dur);

				dep_add_ncheck_hash(fi_depends_name, fi_depends_name);
				dep_pr_include_directive(fi_depends_name);
			}
		}

		int stack_size = WG_MAX_PATH;
		int stack_index = -1;
		char **dir_stack = wg_malloc(stack_size * sizeof(char*));

		for (int i = 0; i < stack_size; i++) {
		    dir_stack[i] = wg_malloc(WG_PATH_MAX);
		}

		stack_index++;
		wg_snprintf(dir_stack[stack_index], WG_PATH_MAX, "%s", dep_dir);

		while (stack_index >= 0) {
		    char working_directory[WG_PATH_MAX];
		    wg_strlcpy(working_directory,
		    	dir_stack[stack_index],
		    	sizeof(working_directory));
		    stack_index--;

		    DIR *dir = opendir(working_directory);
		    if (!dir)
		        continue;

		    struct dirent *item;
		    struct stat st;

		    while ((item = readdir(dir)) != NULL) {
		        if (wg_is_special_dir(item->d_name))
		            continue;

		        wg_strlcpy(depends_ipath, working_directory, sizeof(depends_ipath));
		        strlcat(depends_ipath, __PATH_STR_SEP_LINUX, sizeof(depends_ipath));
		        strlcat(depends_ipath, item->d_name, sizeof(depends_ipath));

		        if (stat(depends_ipath, &st) != 0)
		            continue;

		        if (S_ISDIR(st.st_mode)) {
		            if (strcmp(item->d_name, "pawno") == WG_RETZ ||
						strcmp(item->d_name, "qawno") == WG_RETZ ||
						strcmp(item->d_name, "include") == WG_RETZ) {
		                continue;
		            }
		            if (stack_index < stack_size - 1) {
		                stack_index++;
		                wg_strlcpy(dir_stack[stack_index], depends_ipath, WG_MAX_PATH);
		            }
		            continue;
		        }

		        const char *extension = strrchr(item->d_name, '.');
		        if (!extension || strcmp(extension, ".inc"))
		            continue;

		        wg_strlcpy(deps_parent_dir, working_directory, sizeof(deps_parent_dir));
				char *depends_filename = NULL;
#ifdef WG_WIDOWS
				depends_filename = strrchr(deps_parent_dir, __PATH_CHR_SEP_WIN32);
#else
				depends_filename = strrchr(deps_parent_dir, __PATH_CHR_SEP_LINUX);
#endif
		        if (!depends_filename)
		            continue;

		        ++depends_filename;

		        wg_strlcpy(dest, include_path, sizeof(dest));
#ifdef WG_WINDOWS
				strlcat(dest, __PATH_STR_SEP_WIN32, sizeof(dest));
#else
		        strlcat(dest, __PATH_STR_SEP_LINUX, sizeof(dest));
#endif
		        strlcat(dest, depends_filename, sizeof(dest));

		        if (rename(deps_parent_dir, dest)) {
		            int _is_win32 = 0;
#ifdef WG_WINDOWS
		            _is_win32 = 1;
#endif
		            if (_is_win32) {
		                wg_strlcpy(cmd, "xcopy ", sizeof(cmd));
		                strlcat(cmd, deps_parent_dir, sizeof(cmd));
		                strlcat(cmd, " ", sizeof(cmd));
		                strlcat(cmd, dest, sizeof(cmd));
		                strlcat(cmd, " /E /I /H /Y >nul 2>&1 && rmdir /S /Q ", sizeof(cmd));
		                strlcat(cmd, deps_parent_dir, sizeof(cmd));
		                strlcat(cmd, " >nul 2>&1", sizeof(cmd));
		            } else {
		                wg_strlcpy(cmd, "cp -r ", sizeof(cmd));
		                strlcat(cmd, deps_parent_dir, sizeof(cmd));
		                strlcat(cmd, " ", sizeof(cmd));
		                strlcat(cmd, dest, sizeof(cmd));
		                strlcat(cmd, " && rm -rf ", sizeof(cmd));
		                strlcat(cmd, deps_parent_dir, sizeof(cmd));
		            }

		            struct timespec start = {0}, end = { 0 };
		            double move_duration;

		            clock_gettime(CLOCK_MONOTONIC, &start);
		            wg_run_command(cmd);
		            clock_gettime(CLOCK_MONOTONIC, &end);

		            move_duration = (end.tv_sec - start.tv_sec) + 
									(end.tv_nsec - start.tv_nsec) / 1e9;

		            pr_color(stdout,
							 FCOLOUR_CYAN,
							 " [MOVING] Include %s -> %s [Finished at %.3fs]\n",
							 deps_parent_dir, dest, move_duration);
		        }

		        dep_add_ncheck_hash(dest, dest);

				printf("\n");
		    }

		    closedir(dir);
		}

		for (int i = 0; i < stack_size; i++) {
		    wg_free(dir_stack[i]);
		}
		wg_free(dir_stack);

		int _is_win32 = 0;
#ifdef WG_WINDOWS
		_is_win32 = 1;
#endif
		if (_is_win32)
			wg_snprintf(rm_cmd, sizeof(rm_cmd),
				"rmdir /s /q \"%s\"",
				dep_dir);
		else
			wg_snprintf(rm_cmd, sizeof(rm_cmd),
				"rm -rf %s",
				dep_dir);
		wg_run_command(rm_cmd);

		return;
}

void wg_apply_depends (const char *depends_name)
{
		char _depends[WG_PATH_MAX];
		char dep_dir[WG_PATH_MAX];

		wg_snprintf(_depends, sizeof(_depends), "%s", depends_name);

		if (strstr(_depends, ".tar.gz")) {
			char *f_EXT = strstr(_depends, ".tar.gz");
			if (f_EXT)
				*f_EXT = '\0';
		} if (strstr(_depends, ".zip")) {
			char *f_EXT = strstr(_depends, ".zip");
			if (f_EXT)
				*f_EXT = '\0';
		}

		wg_snprintf(dep_dir, sizeof(dep_dir), "%s", _depends);

		if (wg_server_env() == WG_RETN) {
			if (dir_exists("pawno/include") == WG_RETZ) {
				wg_mkdir("pawno/include");
			}
			if (dir_exists("plugins") == WG_RETZ) {
				MKDIR("plugins");
			}
		} else if (wg_server_env() == WG_RETW) {
			if (dir_exists("qawno/include") == WG_RETZ) {
				wg_mkdir("qawno/include");
			}
			if (dir_exists("plugins") == WG_RETZ) {
				MKDIR("plugins");
			}
			if (dir_exists("components") == WG_RETZ) {
				MKDIR("components");
			}
		}

		dep_move_files(dep_dir);
}

void wg_install_depends (const char *depends_string)
{
        char buffer[1024];
        const char *depends[MAX_DEPENDS];
        struct dep_repo_info repo;
        char dep_url[1024];
        char dep_name[256];
        const char *last_slash;
        char *token;
        int deps_count = 0;
        int i;

        memset(depends, 0, sizeof(depends));
        wgconfig.wg_idepends = 0;

        if (!depends_string || !*depends_string) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("no valid dependencies to install!\t\t[X]\n");
                goto done;
        }

        if (strlen(depends_string) >= sizeof(buffer)) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("depends_string too long!\t\t[X]\n");
                goto done;
        }

        wg_snprintf(buffer, sizeof(buffer), "%s", depends_string);

        token = strtok(buffer, " ");
        while (token && deps_count < MAX_DEPENDS) {
                depends[deps_count++] = token;
                token = strtok(NULL, " ");
        }

        if (!deps_count) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("no valid depends to install!\t\t[X]\n");
                goto done;
        }

        for (i = 0; i < deps_count; i++) {
                if (!dep_parse_repo(depends[i], &repo)) {
                        pr_color(stdout, FCOLOUR_RED, "");
                        printf("invalid repo format: %s\t\t[X]\n", depends[i]);
                        continue;
                }
                if (!strcmp(repo.host, "github")) {
                        if (!dep_handle_repo(&repo, dep_url, sizeof(dep_url))) {
                                pr_color(stdout, FCOLOUR_RED, "");
                                printf("repo not found: %s\t\t[X]\n", depends[i]);
                                continue;
                        }
                } else {
                        dep_build_repo_url(&repo, 0, dep_url, sizeof(dep_url));
                        if (!dep_url_checking(dep_url, wgconfig.wg_toml_github_tokens)) {
                                pr_color(stdout, FCOLOUR_RED, "");
                                printf("repo not found: %s\t\t[X]\n", depends[i]);
                                continue;
                        }
                }

                last_slash = strrchr(dep_url, __PATH_CHR_SEP_LINUX);
                if (last_slash && *(last_slash + 1)) {
                        wg_snprintf(dep_name, sizeof(dep_name), "%s", last_slash + 1);
                        if (!strstr(dep_name, ".zip") && !strstr(dep_name, ".tar.gz"))
                                wg_snprintf(dep_name + strlen(dep_name),
                                            sizeof(dep_name) - strlen(dep_name),
                                            ".zip");
                } else {
                        wg_snprintf(dep_name, sizeof(dep_name), "%s.tar.gz", repo.repo);
                }

                if (!*dep_name) {
                        pr_color(stdout, FCOLOUR_RED, "");
                        printf("invalid repo name: %s\t\t[X]\n", dep_url);
                        continue;
                }

                wgconfig.wg_idepends = 1;

                struct timespec start = {0}, end = { 0 };
                double depends_dur;

                clock_gettime(CLOCK_MONOTONIC, &start);
                wg_download_file(dep_url, dep_name);
                wg_apply_depends(dep_name);
                clock_gettime(CLOCK_MONOTONIC, &end);

                depends_dur = (end.tv_sec - start.tv_sec)
                            + (end.tv_nsec - start.tv_nsec) / 1e9;

            	pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <D> Finished at %.3fs (%.0f ms)\n",
	                     depends_dur, depends_dur * 1000.0);
        }

done:
        return;
}
