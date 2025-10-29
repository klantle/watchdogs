#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "color.h"
#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "crypto.h"
#include "chain.h"
#include "depends.h"

/**
 * struct curl_buffer - Buffer for CURL response data
 * @data: Pointer to allocated data
 * @size: Size of allocated data
 */
struct curl_buffer {
		char *data;
		size_t size;
};

/**
 * struct dep_repo_info - Repository information structure
 * @host: Host name (github, gitlab, gitea, sourceforge)
 * @domain: Domain name
 * @user: Repository owner
 * @repo: Repository name  
 * @tag: Tag or branch name
 */
struct dep_repo_info {
		char host[32];
		char domain[64];
		char user[128];
		char repo[128];
		char tag[128];
};

/**
 * dep_write_callback - CURL write callback function
 * @contents: Received data
 * @size: Size of each element
 * @nmemb: Number of elements
 * @userp: User pointer (curl_buffer)
 *
 * Return: Number of bytes processed
 */
static size_t dep_write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
		struct curl_buffer *mem = (struct curl_buffer *)userp;
		size_t realsize = size * nmemb;
		char *ptr;

		ptr = wdrealloc(mem->data, mem->size + realsize + 1);
		if (!ptr)
				return RETZ;

		mem->data = ptr;
		memcpy(&mem->data[mem->size], contents, realsize);
		mem->size += realsize;
		mem->data[mem->size] = 0;

		return realsize;
}

/**
 * dep_curl_url_get_response - Check if URL is accessible
 * @url: URL to check
 *
 * Return: 1 if accessible, 0 if not
 */
int dep_curl_url_get_response(const char *url)
{
		CURL *curl;
		CURLcode res;
		long response_code = 0;
		int ret = 0;

		curl = curl_easy_init();
		if (!curl)
				return RETZ;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

		res = curl_easy_perform(curl);
		if (res == CURLE_OK)
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

		curl_easy_cleanup(curl);

		return (res == CURLE_OK && response_code == 200);
}

/**
 * dep_curl_get_html - Fetch HTML content from URL
 * @url: URL to fetch
 * @out_html: Output pointer for HTML data
 *
 * Return: 1 on success, 0 on failure
 */
int dep_curl_get_html(const char *url, char **out_html)
{
		CURL *curl;
		struct curl_buffer buffer = { 0 };
		CURLcode res;

		curl = curl_easy_init();
		if (!curl)
				return RETZ;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dep_write_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "watchdogs/1.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK || buffer.size == 0) {
				wdfree(buffer.data);
				return RETZ;
		}

		*out_html = buffer.data;
		return RETN;
}

/**
 * dep_find_archive_f_html - Find archive URL in HTML content
 * @html: HTML content to search
 * @out_url: Output buffer for URL
 * @out_size: Size of output buffer
 *
 * Return: 1 if found, 0 if not
 */
static int dep_find_archive_f_html(const char *html, char *out_url, size_t out_size)
{
		const char *p = html;
		const char *zip, *tar, *ret, *href;
		size_t len, ulen;

		while (p) {
				zip = strstr(p, ".zip");
				tar = strstr(p, ".tar.gz");
				ret = NULL;
				len = 0;

				if (zip && (!tar || zip < tar)) {
						ret = zip;
						len = zip - p + 4;
				} else if (tar) {
						ret = tar;
						len = tar - p + 7;
				} else {
						break;
				}

				/* Find href=" */
				href = ret;
				while (href > p && *(href - 1) != '"')
						--href;

				ulen = len + (ret - href);
				if (ulen < out_size) {
						strncpy(out_url, href, ulen);
						out_url[ulen] = 0;

						/* Convert relative URL to absolute */
						if (out_url[0] == '/') {
								char full_url[PATH_MAX];
								snprintf(full_url, sizeof(full_url), 
										 "%sgithub.com%s", "https://", out_url);
								strncpy(out_url, full_url, out_size - 1);
								out_url[out_size - 1] = '\0';
						}
						return RETN;
				}
				p = ret + len;
		}

		return RETZ;
}

/**
 * dep_gh_select_best_asset - Select most appropriate asset for current platform
 * @assets: Array of asset URLs
 * @count: Number of assets
 * @preferred_os: Preferred OS name
 *
 * Return: Duplicated string of best asset URL
 */
static char *dep_gh_select_best_asset(char **assets, int count, const char *preferred_os)
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
				return strdup(assets[0]);

		/* Prioritize OS-specific binaries */
		for (i = 0; i < num_patterns; i++) {
				for (j = 0; j < count; j++) {
						if (strstr(assets[j], os_patterns[i]))
								return strdup(assets[j]);
				}
		}

		/* Fallback to first asset */
		return strdup(assets[0]);
}

/**
 * dep_parse_repo_input - Parse repository input string
 * @input: Input string (URL or short format)
 * @info: Output dep_repo_info structure
 *
 * Return: 1 on success, 0 on failure
 */
static int dep_parse_repo_input(const char *input, struct dep_repo_info *info)
{
		char working[512];
		char *tag_ptr, *path, *first_slash, *user, *repo_slash, *repo, *git_ext;

		memset(info, 0, sizeof(*info));

		/* Default values */
		strcpy(info->host, "github");
		strcpy(info->domain, "github.com");

		/* Create working copy */
		strncpy(working, input, sizeof(working) - 1);
		working[sizeof(working) - 1] = '\0';

		/* Parse tag if present */
		tag_ptr = strrchr(working, ':');
		if (tag_ptr) {
				*tag_ptr = '\0';
				strncpy(info->tag, tag_ptr + 1, sizeof(info->tag) - 1);
		}

		path = working;

		/* Skip protocol if present */
		if (strncmp(path, "https://", 8) == 0) {
				path += 8;
		} else if (strncmp(path, "http://", 7) == 0) {
				path += 7;
		}

		/* Handle short formats */
		if (strncmp(path, "github/", 7) == 0) {
				strcpy(info->host, "github");
				strcpy(info->domain, "github.com");
				path += 7;
		} else if (strncmp(path, "gitlab/", 7) == 0) {
				strcpy(info->host, "gitlab");
				strcpy(info->domain, "gitlab.com");
				path += 7;
		} else if (strncmp(path, "gitea/", 6) == 0) {
				strcpy(info->host, "gitea");
				strcpy(info->domain, "gitea.com");
				path += 6;
		} else if (strncmp(path, "sourceforge/", 12) == 0) {
				strcpy(info->host, "sourceforge");
				strcpy(info->domain, "sourceforge.net");
				path += 12;
		} else {
				/* Long format with explicit domain */
				first_slash = strchr(path, '/');
				if (first_slash && strchr(path, '.') && strchr(path, '.') < first_slash) {
						char domain[128];
						strncpy(domain, path, first_slash - path);
						domain[first_slash - path] = '\0';

						if (strstr(domain, "github")) {
								strcpy(info->host, "github");
								strcpy(info->domain, "github.com");
						} else if (strstr(domain, "gitlab")) {
								strcpy(info->host, "gitlab");
								strcpy(info->domain, "gitlab.com");
						} else if (strstr(domain, "gitea")) {
								strcpy(info->host, "gitea");
								strcpy(info->domain, "gitea.com");
						} else if (strstr(domain, "sourceforge")) {
								strcpy(info->host, "sourceforge");
								strcpy(info->domain, "sourceforge.net");
						}
						path = first_slash + 1;
				}
		}

		/* Parse user/repo from path */
		user = path;
		repo_slash = strchr(path, '/');
		if (!repo_slash)
				return RETZ;

		*repo_slash = '\0';
		repo = repo_slash + 1;

		/* SourceForge has different format */
		if (strcmp(info->host, "sourceforge") == 0) {
				strncpy(info->user, user, sizeof(info->user) - 1);
				strncpy(info->repo, repo, sizeof(info->repo) - 1);
		} else {
				strncpy(info->user, user, sizeof(info->user) - 1);

				/* Remove .git extension if present */
				git_ext = strstr(repo, ".git");
				if (git_ext)
						*git_ext = '\0';
				strncpy(info->repo, repo, sizeof(info->repo) - 1);
		}

		return RETN;
}

/**
 * dep_gh_release_assets - Get release assets from GitHub API
 * @user: Repository owner
 * @repo: Repository name
 * @tag: Release tag
 * @out_urls: Output array for URLs
 * @max_urls: Maximum number of URLs to fetch
 *
 * Return: Number of assets ret
 */
static int dep_gh_release_assets(const char *user, const char *repo, 
									 const char *tag, char **out_urls, int max_urls)
{
		char api_url[PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int url_count = 0;

		snprintf(api_url, sizeof(api_url),
				 "%sapi.github.com/repos/%s/%s/releases/tags/%s",
				 "https://", user, repo, tag);

		if (!dep_curl_get_html(api_url, &json_data))
				return RETZ;

		p = json_data;
		while (url_count < max_urls && (p = strstr(p, "\"browser_download_url\"")) != WD_ISNULL) {
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
				out_urls[url_count] = wdmalloc(url_len + 1);
				strncpy(out_urls[url_count], p, url_len);
				out_urls[url_count][url_len] = '\0';

				++url_count;
				p = url_end + 1;
		}

		wdfree(json_data);
		return url_count;
}

/**
 * dep_check_github_branch - Check if GitHub branch exists
 * @user: Repository owner
 * @repo: Repository name  
 * @branch: Branch name
 *
 * Return: 1 if exists, 0 if not
 */
static int dep_check_github_branch(const char *user,
								   const char *repo,
								   const char *branch)
{
		char url[1024];

		snprintf(url, sizeof(url),
				 "github.com/%s/%s/archive/refs/heads/%s.zip",
				 "https://", user, repo, branch);

		return dep_curl_url_get_response(url);
}

/**
 * dep_build_repo_url - Build repository URL based on host and type
 * @info: Repository information
 * @is_tag_page: Whether to build tag page URL
 * @out_url: Output buffer for URL
 * @out_size: Size of output buffer
 */
static void dep_build_repo_url(const struct dep_repo_info *info, int is_tag_page,
						   	   char *out_url, size_t out_size)
{
		if (strcmp(info->host, "github") == 0)
			{
					if (is_tag_page && info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/releases/tag/%s",
									"https://", info->domain, info->user, info->repo, info->tag);
					} else if (info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/archive/refs/tags/%s.tar.gz",
									"https://", info->domain, info->user, info->repo, info->tag);
					} else {
							snprintf(out_url, out_size, "%s%s/%s/%s/archive/refs/heads/main.zip",
									"https://", info->domain, info->user, info->repo);
					}
			}
		else if (strcmp(info->host, "gitlab") == 0)
			{
					if (is_tag_page && info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/-/tags/%s",
									"https://", info->domain, info->user, info->repo, info->tag);
					} else if (info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/-/archive/%s/%s-%s.tar.gz",
									"https://", info->domain, info->user, info->repo, info->tag,
									info->repo, info->tag);
					} else {
							snprintf(out_url, out_size, "%s%s/%s/%s/-/archive/main/%s-main.zip",
									"https://", info->domain, info->user, info->repo, info->repo);
					}
			}
		else if (strcmp(info->host, "gitea") == 0)
			{
					if (is_tag_page && info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/tags/%s",
									"https://", info->domain, info->user, info->repo, info->tag);
					} else if (info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/%s/%s/archive/%s.tar.gz",
									"https://", info->domain, info->user, info->repo, info->tag);
					} else {
							snprintf(out_url, out_size, "%s%s/%s/%s/archive/main.zip",
									"https://", info->domain, info->user, info->repo);
					}
			}
		else if (strcmp(info->host, "sourceforge") == 0)
			{
					if (info->tag[0]) {
							snprintf(out_url, out_size, "%s%s/projects/%s/files/latest/download",
									"https://", info->domain, info->repo);
					} else {
							snprintf(out_url, out_size, "%s%s/projects/%s/files/latest/download",
									"https://", info->domain, info->repo);
					}
			}
}

/**
 * wd_install_dependss - Install depends from repositories
 * dep_handle_repo - Helper for Handling
 */

static int dep_handle_repo(const struct dep_repo_info *dep_repo_info,
								  char *out_url,
								  size_t out_sz)
{
		int ret = 0;
		/* Try common branch names */
		const char *branches[] = {
									"main",
									"master"
								};

		if (dep_repo_info->tag[0]) {
			/* Try GitHub release assets */
			char *assets[10] = { 0 };
			int asset_count = dep_gh_release_assets(dep_repo_info->user,
													dep_repo_info->repo,
													dep_repo_info->tag,
													assets,
													10);

			if (asset_count > 0) {
				char *best_asset;
				best_asset = dep_gh_select_best_asset(assets, asset_count, NULL);
				if (best_asset) {
					strncpy(out_url, best_asset, out_sz - 1);
					ret = 1;
					wdfree(best_asset);
				}
				for (int j = 0; j < asset_count; j++)
					wdfree(assets[j]);
			}

			/* Fallback to tagged source archives */
			if (!ret)
				{
					const char *formats[] = {
												"https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
												"https://github.com/%s/%s/archive/refs/tags/%s.zip"
											};
					for (int j = 0; j < 2 && !ret; j++) {
						snprintf(out_url, out_sz, formats[j],
								dep_repo_info->user,
								dep_repo_info->repo,
								dep_repo_info->tag);
						if (dep_curl_url_get_response(out_url))
							ret = 1;
					}
				}
		} else
		{
			for (int j = 0; j < 2 && !ret; j++) {
				snprintf(out_url, out_sz,
						"%s%s/%s/archive/refs/heads/%s.zip",
						"https://",
						dep_repo_info->user,
						dep_repo_info->repo,
						branches[j]);
				if (dep_curl_url_get_response(out_url)) {
					ret = 1;
					if (j == 1)
						pr_info(stdout, "Using master branch (main not master)");
				}
			}
		}

		return ret;
}

/**
 * __convert_path - convert backslashes to forward slashes in path
 * @path: path string to convert
 */
void __convert_path(char *path)
{
		char *p;

		for (p = path; *p; p++) {
				if (*p == '\\')
						*p = '/';
		}
}

/**
 * dep_add_ncheck_hash - add file hash to dependency list after checking duplicates
 * @file_path: full path to the file for hashing
 * @json_path: relative path for JSON storage
 */
void dep_add_ncheck_hash(cJSON *depends, const char *file_path, const char *json_path)
{
		char convert_f_path[PATH_MAX], convert_j_path[PATH_MAX];
		unsigned char hash[SHA256_DIGEST_LENGTH]; /* Buffer for SHA256 hash */
		char *hex;
		int h_exists = 0;
		int array_size;
		int j;

		char *pos;
		while ((pos = strstr(file_path, "include/")) != NULL) {
			memmove(pos, pos + strlen("include/"), strlen(pos + strlen("include/")) + 1);
		}

		/* Convert paths to use forward slashes */
		strncpy(convert_f_path, file_path, sizeof(convert_f_path));
		convert_f_path[sizeof(convert_f_path) - 1] = '\0';
		__convert_path(convert_f_path);

		strncpy(convert_j_path, json_path, sizeof(convert_j_path));
		convert_j_path[sizeof(convert_j_path) - 1] = '\0';
		__convert_path(convert_j_path);

		pr_info(stdout, "\tHashing convert path: %s\n", convert_f_path);

		/* Calculate SHA256 hash of the file */
		if (sha256_hash(convert_f_path, hash) == RETN) {
				to_hex(hash, 32, &hex);

				/* Check if hash already exists in the list */
				array_size = cJSON_GetArraySize(depends);
				for (j = 0; j < array_size; j++) {
						cJSON *_e_item = cJSON_GetArrayItem(depends, j);
						if (cJSON_IsString(_e_item) && 
						    strcmp(_e_item->valuestring, hex) == 0) {
								h_exists = 1;
								break;
						}
				}

				/* Add hash if it doesn't exist */
				if (!h_exists) {
						cJSON_AddItemToArray(depends, cJSON_CreateString(hex));
						pr_info(stdout,
								"\tAdded hash for: %s to wd_depends.json\n",
								convert_j_path);
				} else {
						pr_info(stdout,
								"\tHash already exists for: %s in wd_depends.json\n",
								convert_j_path);
						pr_info(stdout, "\tHash:");
						pr_color(stdout, FCOLOUR_GREEN, "\t\t%s\n", hex);
				}

				wdfree(hex);
		} else {
				pr_error(stdout, "Failed to hash: %s (convert: %s)\n", convert_j_path, convert_f_path);
		}
}

/*
 * dep_implementation_samp_conf - Apply plugins name to SA-MP config
 * dep_implementation_omp_conf - Apply plugins name to Open.MP config
 */
typedef struct {
    const char *dep_config;
    const char *dep_target;
    const char *dep_added;
} depConfig;

void dep_implementation_samp_conf(depConfig config) {
		pr_info(stdout, "\tAdding Depends: %s", config.dep_added);
		
		FILE* file = fopen(config.dep_config, "r");
		if (file) {
			char line[256];
			int t_exist = 0;
			int tr_exist = 0;
			int tr_ln_has_tx = 0;
			
			while (fgets(line, sizeof(line), file)) {
				line[strcspn(line, "\n")] = 0;
				
				if (strstr(line, config.dep_added) != WD_ISNULL) {
					t_exist = 1;
				}
				
				if (strstr(line, config.dep_target) != WD_ISNULL) {
					tr_exist = 1;
					if (strstr(line, config.dep_added) != WD_ISNULL) {
						tr_ln_has_tx = 1;
					}
				}
			}
			fclose(file);
			
			if (t_exist) {
				return;
			}
			
			if (tr_exist && !tr_ln_has_tx) {
				FILE* temp_file = fopen("temp.txt", "w");
				file = fopen(config.dep_config, "r");
				
				while (fgets(line, sizeof(line), file)) {
					char clean_line[256];
					strcpy(clean_line, line);
					clean_line[strcspn(clean_line, "\n")] = 0;
					
					if (strstr(clean_line, config.dep_target) != WD_ISNULL && 
						strstr(clean_line, config.dep_added) == WD_ISNULL) {
						fprintf(temp_file, "%s %s\n", clean_line, config.dep_added);
					} else {
						fputs(line, temp_file);
					}
				}
				
				fclose(file);
				fclose(temp_file);
				
				remove(config.dep_config);
				rename("temp.txt", config.dep_config);
			} else if (!tr_exist) {
				file = fopen(config.dep_config, "a");
				fprintf(file, "%s %s\n", config.dep_target, config.dep_added);
				fclose(file);
			}
			
		} else {
			file = fopen(config.dep_config, "w");
			fprintf(file, "%s %s\n",
					config.dep_target,
					config.dep_added);
			fclose(file);
		}
}
#define S_ADD_DEP_AFTER(x, y, z) \
    dep_implementation_samp_conf((depConfig){x, y, z})

void dep_implementation_omp_conf(const char* filename, const char* plugin_name) {
		pr_color(stdout, FCOLOUR_GREEN, "Depends: Adding Depends '%s'", plugin_name);
		
		FILE* c_file = fopen(filename, "r");
		cJSON* root = NULL;
		
		if (!c_file) {
			root = cJSON_CreateObject();
		} else {
			fseek(c_file, 0, SEEK_END);
			long file_size = ftell(c_file);
			fseek(c_file, 0, SEEK_SET);
			
			char* buffer = (char*)malloc(file_size + 1);
			if (!buffer) {
				pr_error(stdout, "Memory allocation failed!");
				fclose(c_file);
				return;
			}
			
			fread(buffer, 1, file_size, c_file);
			buffer[file_size] = '\0';
			fclose(c_file);
			
			root = cJSON_Parse(buffer);
			free(buffer);
			
			if (!root) {
				root = cJSON_CreateObject();
			}
		}
		
		cJSON* pawn = cJSON_GetObjectItem(root, "pawn");
		if (!pawn) {
			pawn = cJSON_CreateObject();
			cJSON_AddItemToObject(root, "pawn", pawn);
		}
		
		cJSON* legacy_plugins = cJSON_GetObjectItem(pawn, "legacy_plugins");
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
				!strcmp(item->valuestring, plugin_name)) {
				p_exist = 1;
				break;
			}
		}
		
		if (!p_exist) {
			cJSON_AddItemToArray(legacy_plugins, cJSON_CreateString(plugin_name));
		}
		
		char* __cJSON_Printed = cJSON_Print(root);
		c_file = fopen(filename, "w");
		if (c_file) {
			fputs(__cJSON_Printed, c_file);
			fclose(c_file);
		}
		
		cJSON_Delete(root);
		free(__cJSON_Printed);
}
#define M_ADD_PLUGIN(x, y) dep_implementation_omp_conf(x, y)

/**
 * dep_add_include - Added Include to Gamemode
 */
void dep_add_include(const char *modes,
					 char *dep_name,
					 char *dep_after) {
		FILE *file = fopen(modes, "r");
		if (file == WD_ISNULL) {
			return;
		}
		
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		char *ct_modes = wdmalloc(fileSize + 1);
		fread(ct_modes, 1, fileSize, file);
		ct_modes[fileSize] = '\0';
		fclose(file);

		pr_color(stdout, FCOLOUR_GREEN, "Depends: Adding Include: '%s'", dep_name);

		if (strstr(ct_modes, dep_name) != WD_ISNULL &&
			strstr(ct_modes, dep_after) != WD_ISNULL) {
			wdfree(ct_modes);
			return;
		}
		
		char *e_dep_after_pos = strstr(ct_modes,
					   				   dep_after);
		
		FILE *n_file = fopen(modes, "w");
		if (n_file == WD_ISNULL) {
			wdfree(ct_modes);
			return;
		}
		
		if (e_dep_after_pos != WD_ISNULL) {
			fwrite(ct_modes,
				   1,
				   e_dep_after_pos - ct_modes + strlen(dep_after),
				   n_file);
			fprintf(n_file, "\n%s", dep_name);
			fputs(e_dep_after_pos + strlen(dep_after), n_file);
		} else {
			char *includeToAddPos = strstr(ct_modes, dep_name);
			
			if (includeToAddPos != WD_ISNULL) {
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
		wdfree(ct_modes);
}
#define DEP_ADD_INCLUDES(x, y, z) dep_add_include(x, y, z)

/**
 * dep_get_fname - extract filename from a path string
 * @path: the full path to extract filename from
 *
 * Return: pointer to the filename portion of the path
 */
const char *dep_get_fname(const char *path)
{
		const char *depends;

		/* Find last forward slash */
		depends = strrchr(path, '/');
		if (!depends) {
				/* If no forward slash, try backslash (Windows paths) */
				depends = strrchr(path, '\\');
		}

		/* Return filename portion or original path if no slashes found */
		return depends ? depends + 1 : path;
}

/**
 * dep_get_bname - Extract basename from file path
 * @path: Full file path
 * 
 * Return: Pointer to basename in path
 */
static const char *dep_get_bname(const char *path)
{
		const char *p1 = strrchr(path, '/');
		const char *p2 = strrchr(path, '\\');
		const char *p = NULL;

		if (p1 && p2) {
			p = (p1 > p2) ? p1 : p2;
		} else if (p1) {
			p = p1;
		} else if (p2) {
			p = p2;
		}

		return p ? p + 1 : path;
}

/**
 * dep_pr_include_addition - Add include directive to main file
 * @filename: Include filename
 */
static void dep_pr_include_addition(const char *filename)
{
		char errbuf[256];
		toml_table_t *_toml_config;
		char depends_name[PATH_MAX];
		char idirective[MAX_PATH];
		
		/* Extract basename */
		const char *dep_n = dep_get_fname(filename);
		snprintf(depends_name, sizeof(depends_name), "%s", dep_n);
		
		const char *basename = dep_get_bname(depends_name);
		
		/* Parse TOML config */
		FILE *procc_f = fopen("watchdogs.toml", "r");
		_toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
		if (procc_f) fclose(procc_f);

		if (!_toml_config) {
			pr_error(stdout, "parsing TOML: %s", errbuf);
			return;
		}

		/* Get compiler input */
		toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
		if (wd_compiler) {
			toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
			if (toml_gm_i.ok) {
				wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
				wdfree(toml_gm_i.u.s);
			}
		}
		toml_free(_toml_config);

		/* Create include directive */
		snprintf(idirective, sizeof(idirective), 
				"#include <%s>", basename);

		/* Add to main file */
		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input, idirective, "#include <a_samp>");
		} else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input, idirective, "#include <open.mp>");
		} else {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input, idirective, "#include <a_samp>");
		}
}

/**
 * dep_pr_inc_files - process all .inc files recursively
 * @depends: cJSON dependency array
 * @bp: base path to start scanning
 * @db: destination base directory
 */
void dep_pr_inc_files(cJSON *depends, const char *bp, const char *db)
{
		DIR *dir;
		struct dirent *item;
		struct stat st;
		char cmd[MAX_PATH * 3];
		char fpath[PATH_MAX];
		char parent[PATH_MAX];
		char dest[PATH_MAX];
		char *dname, *ext;

		dir = opendir(bp);
		if (!dir)
			return;

		while ((item = readdir(dir)) != WD_ISNULL) {
			if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
				continue;

			snprintf(fpath, sizeof(fpath), "%s/%s", bp, item->d_name);

			if (stat(fpath, &st) != 0)
				continue;

			if (S_ISDIR(st.st_mode)) {
				dep_pr_inc_files(depends, fpath, db);
				continue;
			}

			if (!S_ISREG(st.st_mode))
				continue;

			/* Check file extension */
			ext = strrchr(item->d_name, '.');
			if (!ext || strcmp(ext, ".inc"))
				continue;

			/* Copy parent directory name */
			snprintf(parent, sizeof(parent), "%s", bp);
			dname = strrchr(parent, '/');
			if (!dname)
				continue;

			dname++; /* skip '/' */

			snprintf(dest, sizeof(dest), "%s/%s", db, dname);

			/* Try moving directory */
			if (rename(parent, dest)) {
				snprintf(cmd, sizeof(cmd),
					"cp -r \"%s\" \"%s\" && rm -rf \"%s\"",
					parent, dest, parent);
				if (system(cmd)) {
					pr_error(stdout, "Failed to move folder: %s\n", parent);
					continue;
				}
			}

			/* Add to dependency JSON */
			dep_add_ncheck_hash(depends, dest, dest);

			pr_info(stdout, "\tmoved folder: %s to %s/\n",
				dname, !strcmp(wcfg.wd_is_omp, CRC32_TRUE) ?
				"qawno/include" : "pawno/include");

			break; /* Stop after moving this folder */
		}

		closedir(dir);
}

/**
 * dep_pr_file_type - Process files of specific type
 */
static void dep_pr_file_type(const char *path, const char *pattern, 
                             const char *exclude, const char *cwd, 
                             cJSON *depends, const char *target_dir, int root)
{
		char cp_cmd[MAX_PATH * 2];
		char json_item[PATH_MAX];
		
		wd_sef_fdir_reset();
		int found = wd_sef_fdir(path, pattern, exclude);
		
		if (found) {
			for (int i = 0; i < wcfg.wd_sef_count; ++i) {
				const char *dep_names = dep_get_fname(wcfg.wd_sef_found_list[i]),
						   *dep_base_names = dep_get_bname(wcfg.wd_sef_found_list[i]);
				
				/* Move file */
				if (target_dir[0] != '\0')
					snprintf(cp_cmd, sizeof(cp_cmd), "mv -f \"%s\" \"%s/%s/\"", 
							wcfg.wd_sef_found_list[i], cwd, target_dir);
				else
					snprintf(cp_cmd, sizeof(cp_cmd), "mv -f \"%s\" \"%s\"", 
							wcfg.wd_sef_found_list[i], cwd);
				system(cp_cmd);
				
				/* Add to hash list */
				snprintf(json_item, sizeof(json_item), "%s", dep_names);
				dep_add_ncheck_hash(depends, json_item, json_item);
				
				/* Skip adding if plugins on root */
				if (root != 1) goto done;

				/* Add to config */
				if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
					M_ADD_PLUGIN("config.json", dep_base_names);
				else
					S_ADD_DEP_AFTER("server.cfg", "plugins", dep_base_names);
			}
		}
done:
		return;
}

/**
 * dep_move_files - Move dependency files and maintain cache
 * @dep_dir: Path to dependencies folder
 */
void dep_cjson_additem(cJSON *p1, int p2, cJSON *p3)
{
		cJSON *_e_item = cJSON_GetArrayItem(p1, p2);
		if (cJSON_IsString(_e_item)) cJSON_AddItemToArray(p3, cJSON_CreateString(_e_item->valuestring));
}

void dep_move_files(const char *dep_dir)
{
		cJSON *root, *depends, *_e_root = NULL;
		FILE *e_file, *fp_cache;
		char dp_fp[PATH_MAX], dp_fc[PATH_MAX], dp_inc[PATH_MAX];
		char cwd[PATH_MAX], *dep_inc_path = NULL;
		char cp_cmd[MAX_PATH * 2];
		char d_b[MAX_PATH];
		char rm_cmd[PATH_MAX];
		int wd_log_acces, i, __inc_legacyplug_r;
		long fp_cache_sz;
		char *file_content;

		/* Check or create cache file */
		wd_log_acces = path_acces("wd_depends.json");
		if (!wd_log_acces)
			system("touch wd_depends.json");

		/* Create JSON root object */
		root = cJSON_CreateObject();
		depends = cJSON_CreateArray();
		cJSON_AddStringToObject(root, "comment", " -- cache file");

		/* Load existing cache */
		e_file = fopen("wd_depends.json", "r");
		if (e_file) {
			fseek(e_file, 0, SEEK_END);
			fp_cache_sz = ftell(e_file);
			fseek(e_file, 0, SEEK_SET);

			if (fp_cache_sz > 0) {
				file_content = wdmalloc(fp_cache_sz + 1);
				if (file_content) {
					fread(file_content, 1, fp_cache_sz, e_file);
					file_content[fp_cache_sz] = '\0';

					_e_root = cJSON_Parse(file_content);
					if (_e_root) {
						cJSON *_e_depends;

						_e_depends = cJSON_GetObjectItem(_e_root, "depends");
						if (_e_depends && cJSON_IsArray(_e_depends)) {
							int _e_cnt = cJSON_GetArraySize(_e_depends);

							for (i = 0; i < _e_cnt; i++)
								dep_cjson_additem(_e_depends, i, depends);

							pr_info(stdout, "Loaded %d existing hashes", _e_cnt);
						}
					}
					wdfree(file_content);
				}
			}
			fclose(e_file);
		}

		/* Build paths */
		snprintf(dp_fp, sizeof(dp_fp), "%s/plugins", dep_dir);
		snprintf(dp_fc, sizeof(dp_fc), "%s/components", dep_dir);

		/* Determine include path */
		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
			dep_inc_path = "pawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/pawno/include", dep_dir);
		} else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
			dep_inc_path = "qawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/qawno/include", dep_dir);
		} else {
			dep_inc_path = "pawno/include";
			snprintf(dp_inc, sizeof(dp_inc), "%s/pawno/include", dep_dir);
		}

		/* Get current working directory */
		if (!getcwd(cwd, sizeof(cwd))) {
			perror("getcwd");
			goto cleanup;
		}

		/* Process plugin files */
		dep_pr_file_type(dp_fp, "*.dll", NULL, cwd, depends, "plugins", 1);
		dep_pr_file_type(dp_fp, "*.so", NULL, cwd, depends, "plugins", 1);
		dep_pr_file_type(dep_dir, "*.dll", "plugins", cwd, depends, "", 0);
		dep_pr_file_type(dep_dir, "*.so", "plugins", cwd, depends, "", 0);

		/* Process components (OMP only) */
		if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
			dep_pr_file_type(dp_fc, "*.dll", NULL, cwd, depends, "components", 1);
			dep_pr_file_type(dp_fc, "*.so", NULL, cwd, depends, "components", 1);
			dep_pr_file_type(dep_dir, "*.dll", "components", cwd, depends, "", 0);
			dep_pr_file_type(dep_dir, "*.so", "components", cwd, depends, "", 0);
		}

		/* Process include files */
		snprintf(d_b, sizeof(d_b), "%s/include",
			!strcmp(wcfg.wd_is_omp, CRC32_TRUE) ? "qawno" : "pawno");

		dep_pr_inc_files(depends, dep_dir, d_b);

		/* Process legacy include files */
		wd_sef_fdir_reset();
		__inc_legacyplug_r = wd_sef_fdir(dep_dir, "*.inc", dep_inc_path);
		if (__inc_legacyplug_r) {
			for (i = 0; i < wcfg.wd_sef_count; ++i) {
				const char *fi_depends_name;

				fi_depends_name = dep_get_fname(wcfg.wd_sef_found_list[i]);
				snprintf(cp_cmd, sizeof(cp_cmd),
					"mv -f \"%s\" \"%s/%s/\"",
					wcfg.wd_sef_found_list[i], cwd, dep_inc_path);
				system(cp_cmd);

				dep_add_ncheck_hash(depends, fi_depends_name, fi_depends_name);
				dep_pr_include_addition(fi_depends_name);
			}
		}

		/* Save updated cache */
		cJSON_AddItemToObject(root, "depends", depends);

		fp_cache = fopen("wd_depends.json", "w");
		if (fp_cache) {
			int array_size = cJSON_GetArraySize(depends);

			fprintf(fp_cache, "{\n");
			fprintf(fp_cache, "\t\"comment\":\t\" -- this is for cache.\",\n");
			fprintf(fp_cache, "\t\"depends\":\t[");

			if (array_size > 0) {
				fprintf(fp_cache, "\n");
				for (i = 0; i < array_size; i++) {
					cJSON *item = cJSON_GetArrayItem(depends, i);

					if (cJSON_IsString(item)) {
						fprintf(fp_cache, "\t\t\"%s\"", item->valuestring);
						if (i < array_size - 1)
							fprintf(fp_cache, ",");
						fprintf(fp_cache, "\n");
					}
				}
				fprintf(fp_cache, "\t");
			}

			fprintf(fp_cache, "]\n}\n");
			fclose(fp_cache);

			pr_info(stdout, "Saved %d unique hashes", array_size);
		}

cleanup:
		cJSON_Delete(root);
		if (_e_root)
			cJSON_Delete(_e_root);

		snprintf(rm_cmd, sizeof(rm_cmd), "rm -rf %s", dep_dir);
		system(rm_cmd);
}

/**
 * wd_apply_depends - apply depends to gamemodes
 * @depends_name: name of the dependency to apply
 */
void wd_apply_depends(const char *depends_name)
{
		char _depends[PATH_MAX];
		char dep_dir[PATH_MAX];
		struct stat st;
		char *__dot_ext;

		snprintf(_depends, PATH_MAX, "%s", depends_name);
		__dot_ext = strrchr(_depends, '.');
		if (__dot_ext)
				*__dot_ext = '\0';

		snprintf(dep_dir, sizeof(dep_dir), "%s", _depends);

		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
				if (stat("pawno/include", &st) != 0 && errno == ENOENT)
						mkdir_recursive("pawno/include");
				if (stat("plugins", &st) != 0 && errno == ENOENT)
						mkdir_recursive("plugins");
		} else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
				if (stat("qawno/include", &st) != 0 && errno == ENOENT)
						mkdir_recursive("qawno/include");
				if (stat("components", &st) != 0 && errno == ENOENT)
						mkdir_recursive("components");
				if (stat("plugins", &st) != 0 && errno == ENOENT)
						mkdir_recursive("plugins");
		}

		dep_move_files(dep_dir);
}

/**
 * wd_install_depends_str - install dependencies from string
 * @deps_str: string containing dependencies to install
 */
void wd_install_depends_str(const char *deps_str)
{
		char buffer[1024];
		const char *depends[MAX_ARR_DEPENDS];
		char *downloaded_files[MAX_ARR_DEPENDS];
		char *token;
		int dep_count = 0;
		int file_count = 0;
		int i;

		if (!deps_str || !*deps_str) {
			pr_info(stdout, "No valid dependencies to install");
			return;
		}

		wcfg.wd_idepends = 0;

		snprintf(buffer, sizeof(buffer), "%s", deps_str);

		token = strtok(buffer, " ");
		while (token && dep_count < MAX_ARR_DEPENDS) {
			depends[dep_count++] = token;
			token = strtok(NULL, " ");
		}

		if (dep_count == 0) {
			pr_info(stdout, "No valid dependencies to install");
			return;
		}

		for (i = 0; i < dep_count; i++) {
			int dep_item_found = 0;
			struct dep_repo_info dep_repo_info;
			char dep_url[1024] = { 0 };
			char dep_repo_name[256] = { 0 };
			const char *chr_last_slash;

			if (!dep_parse_repo_input(depends[i], &dep_repo_info)) {
				pr_error(stdout, "Invalid repo format: %s", depends[i]);
				continue;
			}

#if defined(_DBG_PRINT)
			pr_info(stdout, "Parsed repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
						dep_repo_info.host, dep_repo_info.domain, dep_repo_info.user,
						dep_repo_info.repo,
						dep_repo_info.tag[0] ? dep_repo_info.tag : "main");
#endif

			if (!strcmp(dep_repo_info.host, "github"))
				{
					dep_item_found = dep_handle_repo(&dep_repo_info,
													 dep_url,
													 sizeof(dep_url));
				}
			else
				{
					dep_build_repo_url(&dep_repo_info, 0, dep_url, sizeof(dep_url));
					dep_item_found = dep_curl_url_get_response(dep_url);
				}

			if (!dep_item_found) {
				pr_error(stdout, "Repository not found: %s", depends[i]);
				continue;
			}

			chr_last_slash = strrchr(dep_url, '/');
			if (chr_last_slash && *(chr_last_slash + 1))
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s",
						chr_last_slash + 1);
			else
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s.zip",
						dep_repo_info.repo);

			if (wd_download_file(dep_url, dep_repo_name) == RETZ)
				downloaded_files[file_count++] = strdup(dep_repo_name);
		}

		wcfg.wd_idepends = 1;
		for (i = 0; i < file_count; i++) {
			wd_apply_depends(downloaded_files[i]);
			wdfree(downloaded_files[i]);
		}
}
