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

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <curl/curl.h>

#include "cJSON/cJSON.h"
#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "crypto.h"
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
 * write_callback - CURL write callback function
 * @contents: Received data
 * @size: Size of each element
 * @nmemb: Number of elements
 * @userp: User pointer (curl_buffer)
 *
 * Return: Number of bytes processed
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
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
 * curl_url_get_response - Check if URL is accessible
 * @url: URL to check
 *
 * Return: 1 if accessible, 0 if not
 */
int curl_url_get_response(const char *url)
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
 * curl_get_html - Fetch HTML content from URL
 * @url: URL to fetch
 * @out_html: Output pointer for HTML data
 *
 * Return: 1 on success, 0 on failure
 */
int curl_get_html(const char *url, char **out_html)
{
		CURL *curl;
		struct curl_buffer buffer = { 0 };
		CURLcode res;

		curl = curl_easy_init();
		if (!curl)
				return RETZ;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
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
 * find_archive_from_html - Find archive URL in HTML content
 * @html: HTML content to search
 * @out_url: Output buffer for URL
 * @out_size: Size of output buffer
 *
 * Return: 1 if found, 0 if not
 */
static int find_archive_from_html(const char *html, char *out_url, size_t out_size)
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
						href--;

				ulen = len + (ret - href);
				if (ulen < out_size) {
						strncpy(out_url, href, ulen);
						out_url[ulen] = 0;

						/* Convert relative URL to absolute */
						if (out_url[0] == '/') {
								char full_url[PATH_MAX];
								snprintf(full_url, sizeof(full_url), 
										 "https://github.com%s", out_url);
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
 * select_best_asset - Select most appropriate asset for current platform
 * @assets: Array of asset URLs
 * @count: Number of assets
 * @preferred_os: Preferred OS name
 *
 * Return: Duplicated string of best asset URL
 */
static char *select_best_asset(char **assets, int count, const char *preferred_os)
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
 * parse_repo_input - Parse repository input string
 * @input: Input string (URL or short format)
 * @info: Output dep_repo_info structure
 *
 * Return: 1 on success, 0 on failure
 */
static int parse_repo_input(const char *input, struct dep_repo_info *info)
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
 * get_github_release_assets - Get release assets from GitHub API
 * @user: Repository owner
 * @repo: Repository name
 * @tag: Release tag
 * @out_urls: Output array for URLs
 * @max_urls: Maximum number of URLs to fetch
 *
 * Return: Number of assets ret
 */
static int get_github_release_assets(const char *user, const char *repo, 
								         const char *tag, char **out_urls, int max_urls)
{
		char api_url[PATH_MAX];
		char *json_data = NULL;
		const char *p;
		int url_count = 0;

		snprintf(api_url, sizeof(api_url),
				 "https://api.github.com/repos/%s/%s/releases/tags/%s",
				 user, repo, tag);

		if (!curl_get_html(api_url, &json_data))
				return RETZ;

		p = json_data;
		while (url_count < max_urls && (p = strstr(p, "\"browser_download_url\"")) != NULL) {
				const char *url_end;
				size_t url_len;

				p += strlen("\"browser_download_url\"");
				p = strchr(p, '"');
				if (!p)
						break;
				p++;

				url_end = strchr(p, '"');
				if (!url_end)
						break;

				url_len = url_end - p;
				out_urls[url_count] = wdmalloc(url_len + 1);
				strncpy(out_urls[url_count], p, url_len);
				out_urls[url_count][url_len] = '\0';

				url_count++;
				p = url_end + 1;
		}

		wdfree(json_data);
		return url_count;
}

/**
 * wd_apply_depends - Apply Depends to Gamemodes
 */

/**
 * Extracts filename from a path string
 * @param path The full path to extract filename from
 * @return Pointer to the filename portion of the path
 */
const char* ddep_get_filename(const char *path) {
        // Find last forward slash
        const char *depends = strrchr(path, '/');
        if (!depends) {
                // If no forward slash, try backslash (Windows paths)
                depends = strrchr(path, '\\');
        }
        // Return filename portion (after slash) or original path if no slashes found
        return depends ? depends + 1 : path;
}

/**
 * Moves dependency files from depends folder to appropriate locations
 * and maintains a cache of file hashes to avoid duplicates
 * @param depends_folder Path to the folder containing dependencies
 */
void move_dependency_files(const char *depends_folder) {
        // Check if cache file exists, create if it doesn't
        int _wd_log_acces = path_acces("wd_depends.json");
        if (!_wd_log_acces) {
                system("touch wd_depends.json");
        }

        // Create new JSON object for storing dependency information
        cJSON *root = cJSON_CreateObject();
        cJSON *depends = cJSON_CreateArray();
        unsigned char hash[32]; // Buffer for SHA256 hash
        
        // Add comment to JSON
        cJSON_AddStringToObject(root, "comment", " -- this is for cache.");
        
        // Variables for reading existing cache
        cJSON *_e_root = NULL;
        cJSON *_e_depends = NULL;
        
        // Read existing cache file to preserve previous hashes
        FILE *e_file = fopen("wd_depends.json", "r");
        if (e_file) {
                fseek(e_file, 0, SEEK_END);
                long fp_cache_sz = ftell(e_file);
                fseek(e_file, 0, SEEK_SET);
                
                if (fp_cache_sz > 0) {
                        // Read entire file content
                        char *file_content = wdmalloc(fp_cache_sz + 1);
                        if (file_content) {
                                fread(file_content, 1, fp_cache_sz, e_file);
                                file_content[fp_cache_sz] = '\0';
                                
                                // Parse existing JSON
                                _e_root = cJSON_Parse(file_content);
                                if (_e_root) {
                                        _e_depends = cJSON_GetObjectItem(_e_root, "depends");
                                        if (_e_depends && cJSON_IsArray(_e_depends)) {
                                                // Copy existing hashes to new array
                                                int _e_cnt = cJSON_GetArraySize(_e_depends);
                                                for (int i = 0; i < _e_cnt; i++) {
                                                        cJSON *_e_item = cJSON_GetArrayItem(_e_depends, i);
                                                        if (cJSON_IsString(_e_item)) {
                                                                cJSON_AddItemToArray(depends, cJSON_CreateString(_e_item->valuestring));
                                                        }
                                                }
                                                printf_info("Loaded %d existing hashes from wd_depends.json", _e_cnt);
                                        }
                                }
                                wdfree(file_content);
                        }
                }
                fclose(e_file);
        }

        // Build paths for different dependency types
        char __sz_dp_fp[PATH_MAX];  // plugins folder path
        snprintf(__sz_dp_fp, sizeof(__sz_dp_fp), "%s/plugins", depends_folder);

        char __sz_dp_fc[PATH_MAX];  // components folder path
        snprintf(__sz_dp_fc, sizeof(__sz_dp_fc), "%s/components", depends_folder);

        char __sz_dp_inc[PATH_MAX]; // include folder path
        char *dep_inc_path = NULL;
        
        // Determine include path based on configuration
        if (wcfg.f_samp == VAL_TRUE) {
__default:
                dep_inc_path = "pawno/include";
                snprintf(__sz_dp_inc, sizeof(__sz_dp_inc), "%s/pawno/include", depends_folder);
        } else if (wcfg.f_openmp == VAL_TRUE) {
                dep_inc_path = "qawno/include";
                snprintf(__sz_dp_inc, sizeof(__sz_dp_inc), "%s/qawno/include", depends_folder);
        } else {
                goto __default; // Fallback to default
        }

        // Get current working directory
        char __cwd[PATH_MAX];
        size_t __sz_cwd = sizeof(__cwd);
        if (!getcwd(__cwd, __sz_cwd)) {
                perror("getcwd");
                cJSON_Delete(root);
                if (_e_root) cJSON_Delete(_e_root);
                return;
        }

/**
 * Converts backslashes to forward slashes in a path string
 * @param path Path string to convert
 */
void __convert_path__(char *path) {
        for (char *p = path;
				  *p;
				  p++) 
                if (*p == '\\')
					*p = '/';
}

/**
 * Adds file hash to dependency list after checking for duplicates
 * @param file_path Full path to the file for hashing
 * @param json_path Relative path for JSON storage
 */
void dep_add_ncheck_hash(const char *file_path, const char *json_path) {
        char convert_f_path[PATH_MAX], convert_j_path[PATH_MAX];
        
        // Convert paths to use forward slashes
        strncpy(convert_f_path, file_path, sizeof(convert_f_path));
        convert_f_path[sizeof(convert_f_path) - 1] = '\0';
        __convert_path__(convert_f_path);
        
        strncpy(convert_j_path, json_path, sizeof(convert_j_path));
        convert_j_path[sizeof(convert_j_path) - 1] = '\0';
        __convert_path__(convert_j_path);
        
        printf("\tHashing convert path: %s\n", convert_f_path);
        
        // Calculate SHA256 hash of the file
        if (sha256_hash(convert_f_path, hash) == RETN) {
                char *hex;
                to_hex(hash, 32, &hex); // Convert hash to hexadecimal string
                
                // Check if hash already exists in the list
                int hash_exists = 0,
                        array_size = cJSON_GetArraySize(depends);
                for (int j = 0; j < array_size; j++) {
                        cJSON *_e_item = cJSON_GetArrayItem(depends, j);
                        if (cJSON_IsString(_e_item) && 
                                strcmp(_e_item->valuestring, hex) == 0) {
                                hash_exists = 1;
                                break;
                        }
                }
                
                // Add hash if it doesn't exist
                if (!hash_exists) {
                        cJSON_AddItemToArray(depends, cJSON_CreateString(hex));
                        printf("\tAdded hash for: %s to wd_depends.json\n", convert_j_path);
                } else {
                        printf("\tHash already exists for: %s in wd_depends.json\n", convert_j_path);
                        printf("\t\tHash: %s\n", hex);
                }
                
                wdfree(hex);
        } else {
                printf_error("Failed to hash: %s (convert: %s)\n", convert_j_path, convert_f_path);
        }
}

        /// PROCESS PLUGINS ///
        
        /* Process DLL files in plugins subfolder */
        wd_sef_fdir_reset();
        int __dll_plugins_f = wd_sef_fdir(__sz_dp_fp, "*.dll", NULL);
        if (__dll_plugins_f) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        // Move file to plugins directory
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/plugins/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        // Add to hash list
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process SO files in plugins subfolder */
        wd_sef_fdir_reset();
        int __so_plugins_f = wd_sef_fdir(__sz_dp_fp, "*.so", NULL);
        if (__so_plugins_f) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/plugins/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process DLL files in root folder (excluding plugins subfolder) */
        wd_sef_fdir_reset();
        int __dll_plugins_r = wd_sef_fdir(depends_folder, "*.dll", "plugins");
        if (__dll_plugins_r) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/plugins/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "plugins/%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process SO files in root folder (excluding plugins subfolder) */
        wd_sef_fdir_reset();
        int __so_plugins_r = wd_sef_fdir(depends_folder, "*.so", "plugins");
        if (__so_plugins_r) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/plugins/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }

        /// PROCESS COMPONENTS ///
        
        /* Process DLL files in components subfolder */
        wd_sef_fdir_reset();
        int __dll_components_f = wd_sef_fdir(__sz_dp_fc, "*.dll", NULL);
        if (__dll_components_f) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        // Move file to components directory
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/components/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process SO files in components subfolder */
        wd_sef_fdir_reset();
        int __so_components_f = wd_sef_fdir(__sz_dp_fc, "*.so", NULL);
        if (__so_components_f) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/components/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process DLL files in root folder (excluding components subfolder) */
        wd_sef_fdir_reset();
        int __dll_components_r = wd_sef_fdir(depends_folder, "*.dll", "components");
        if (__dll_components_r) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/components/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }
        
        /* Process SO files in root folder (excluding components subfolder) */
        wd_sef_fdir_reset();
        int __so_components_r = wd_sef_fdir(depends_folder, "*.so", "components");
        if (__so_components_r) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/components/\"", wcfg.sef_found[i], __cwd);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        snprintf(__sz_json_item, sizeof(__sz_json_item), "%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }

        /// PROCESS INCLUDE FILES ///
        
        /* Process INC files in include subfolder */
        wd_sef_fdir_reset();
        int __inc_plugins_f = wd_sef_fdir(__sz_dp_inc, "*.inc", NULL);
        if (__inc_plugins_f) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        // Move file to appropriate include directory
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/%s/\"", wcfg.sef_found[i], __cwd, dep_inc_path);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        // Determine correct path prefix based on include type
                        if (strfind(__sz_dp_inc, "pawno"))
                                snprintf(__sz_json_item, sizeof(__sz_json_item), "pawno/%s", filename_only);
                        else if (strfind(__sz_dp_inc, "qawno"))
                                snprintf(__sz_json_item, sizeof(__sz_json_item), "qawno/%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }

        /* Process INC files in root folder (excluding include subfolder) */
        wd_sef_fdir_reset();
        int __inc_plugins_r = wd_sef_fdir(depends_folder, "*.inc", dep_inc_path);
        if (__inc_plugins_r) {
                char __sz_cp[PATH_MAX * 2];
                for (int i = 0; i < wcfg.sef_count; ++i) {
                        const char *filename_only = ddep_get_filename(wcfg.sef_found[i]);
                        
                        snprintf(__sz_cp, sizeof(__sz_cp), "mv -f \"%s\" \"%s/%s/\"", wcfg.sef_found[i], __cwd, dep_inc_path);
                        system(__sz_cp);
                        
                        char __sz_json_item[PATH_MAX];
                        if (strfind(__sz_dp_inc, "pawno"))
                                snprintf(__sz_json_item, sizeof(__sz_json_item), "pawno/%s", filename_only);
                        else if (strfind(__sz_dp_inc, "qawno"))
                                snprintf(__sz_json_item, sizeof(__sz_json_item), "qawno/%s", filename_only);
                        dep_add_ncheck_hash(__sz_json_item, __sz_json_item);
                }
        }

        /// SAVE UPDATED CACHE FILE ///
        
        // Add depends array to root JSON object
        cJSON_AddItemToObject(root, "depends", depends);
        
        // Write updated cache to file with formatted JSON
        FILE *fp_cache = fopen("wd_depends.json", "w");
        if (fp_cache) {
                fprintf(fp_cache, "{\n");
                fprintf(fp_cache, "\t\"comment\":\t\" -- this is for cache.\",\n");
                fprintf(fp_cache, "\t\"depends\":\t[");
                
                int array_size = cJSON_GetArraySize(depends);
                if (array_size > 0) {
                        fprintf(fp_cache, "\n");
                        // Write each hash string
                        for (int i = 0; i < array_size; i++) {
                                cJSON *item = cJSON_GetArrayItem(depends, i);
                                if (cJSON_IsString(item)) {
                                        fprintf(fp_cache, "\t\t\"%s\"", item->valuestring);
                                        if (i < array_size - 1) {
                                                fprintf(fp_cache, ",");
                                        }
                                        fprintf(fp_cache, "\n");
                                }
                        }
                        fprintf(fp_cache, "\t");
                }
                
                fprintf(fp_cache, "]\n");
                fprintf(fp_cache, "}\n");
                fclose(fp_cache);
                
                printf_info("Saved %d unique hashes to wd_depends.json\n", array_size);
        } else {
                printf_error("Failed to open wd_depends.json for writing\n");
        }
        
        // Clean up JSON objects
        cJSON_Delete(root);
        if (_e_root)
                cJSON_Delete(_e_root);
}

void wd_apply_depends(const char *depends_name) {
		char _depends[PATH_MAX];
		snprintf(_depends, PATH_MAX, "%s", depends_name);
		char *ext = strrchr(_depends, '.');
		if (ext) *ext = '\0';

		char depends_folder[PATH_MAX];
		snprintf(depends_folder, sizeof(depends_folder), "%s", _depends);

		struct stat st;
		if (wcfg.f_samp == VAL_TRUE) {
			if (stat("pawno/include", &st) != 0 && errno == ENOENT) mkdir_recursive("pawno/include");
			if (stat("plugins", &st) != 0 && errno == ENOENT) mkdir_recursive("plugins");
		} else if (wcfg.f_openmp == VAL_TRUE) {
			if (stat("qawno/include", &st) != 0 && errno == ENOENT) mkdir_recursive("qawno/include");
			if (stat("components", &st) != 0 && errno == ENOENT) mkdir_recursive("components");
			if (stat("plugins", &st) != 0 && errno == ENOENT) mkdir_recursive("plugins");
		}

		move_dependency_files(depends_folder);
}

/**
 * check_github_branch - Check if GitHub branch exists
 * @user: Repository owner
 * @repo: Repository name  
 * @branch: Branch name
 *
 * Return: 1 if exists, 0 if not
 */
static int check_github_branch(const char *user,
							   const char *repo,
							   const char *branch)
{
		char url[1024];

		snprintf(url, sizeof(url),
				 "https://github.com/%s/%s/archive/refs/heads/%s.zip",
				 user, repo, branch);

		return curl_url_get_response(url);
}

/**
 * build_repo_url - Build repository URL based on host and type
 * @info: Repository information
 * @is_tag_page: Whether to build tag page URL
 * @out_url: Output buffer for URL
 * @out_size: Size of output buffer
 */
static void build_repo_url(const struct dep_repo_info *info, int is_tag_page,
						   char *out_url, size_t out_size)
{
		if (strcmp(info->host, "github") == 0) {
				if (is_tag_page && info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/releases/tag/%s",
								 info->domain, info->user, info->repo, info->tag);
				} else if (info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
								 info->domain, info->user, info->repo, info->tag);
				} else {
						snprintf(out_url, out_size, "https://%s/%s/%s/archive/refs/heads/main.zip",
								 info->domain, info->user, info->repo);
				}
		} else if (strcmp(info->host, "gitlab") == 0) {
				if (is_tag_page && info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/-/tags/%s",
								 info->domain, info->user, info->repo, info->tag);
				} else if (info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/-/archive/%s/%s-%s.tar.gz",
								 info->domain, info->user, info->repo, info->tag,
								 info->repo, info->tag);
				} else {
						snprintf(out_url, out_size, "https://%s/%s/%s/-/archive/main/%s-main.zip",
								 info->domain, info->user, info->repo, info->repo);
				}
		} else if (strcmp(info->host, "gitea") == 0) {
				if (is_tag_page && info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/tags/%s",
								 info->domain, info->user, info->repo, info->tag);
				} else if (info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/%s/%s/archive/%s.tar.gz",
								 info->domain, info->user, info->repo, info->tag);
				} else {
						snprintf(out_url, out_size, "https://%s/%s/%s/archive/main.zip",
								 info->domain, info->user, info->repo);
				}
		} else if (strcmp(info->host, "sourceforge") == 0) {
				if (info->tag[0]) {
						snprintf(out_url, out_size, "https://%s/projects/%s/files/latest/download",
								 info->domain, info->repo);
				} else {
						snprintf(out_url, out_size, "https://%s/projects/%s/files/latest/download",
								 info->domain, info->repo);
				}
		}
}

/**
 * wd_install_dependss - Install depends from repositories
 * handle_base_dependency - Helper for Handling
 */

static int handle_base_dependency(const struct dep_repo_info *dep_repo_info,
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
			int asset_count = get_github_release_assets(dep_repo_info->user,
														dep_repo_info->repo,
														dep_repo_info->tag,
														assets,
														10);

			if (asset_count > 0) {
				char *best_asset = select_best_asset(assets, asset_count, NULL);
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
						if (curl_url_get_response(out_url))
							ret = 1;
					}
				}
		} else
		{
			for (int j = 0; j < 2 && !ret; j++) {
				snprintf(out_url, out_sz,
						"https://github.com/%s/%s/archive/refs/heads/%s.zip",
						dep_repo_info->user,
						dep_repo_info->repo,
						branches[j]);
				if (curl_url_get_response(out_url)) {
					ret = 1;
					if (j == 1)
						printf_info("Using master branch (main not ret)");
				}
			}
		}

		return ret;
}

void wd_install_depends(const char *dep_one, const char *dep_two)
{
		const char *depends[] = {
								  dep_one,
								  dep_two
								};
		for (int i = 0; i < MAX_ARR_DEPENDS; i++) {
			if (!depends[i] || !*depends[i])
				continue;

			int dep_item_found = 0;
			struct dep_repo_info dep_repo_info;
			char dep_url[1024] = {0}, dep_repo_name[256] = { 0 };
			size_t dep_url_size = sizeof(dep_url),
				   dep_repo_name_size = sizeof(dep_repo_name);

			/*  Parse input  */
			if (!parse_repo_input(depends[i], &dep_repo_info)) {
				printf_error("Invalid repo format:\n\t%s",
														depends[i]);
				continue;
			}

#if defined(_DBG_PRINT)
			printf_info("Parsed repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
					dep_repo_info.host,
					dep_repo_info.domain,
					dep_repo_info.user,
					dep_repo_info.repo,
					dep_repo_info.tag[0] ? dep_repo_info.tag : "main");
#endif

			/*  Handle based on host  */
			if (!strcmp(dep_repo_info.host, "github"))
				{
					dep_item_found = handle_base_dependency(&dep_repo_info,
															dep_url,
															dep_url_size);
				} 
			else if (!strcmp(dep_repo_info.host, "sourceforge"))
				{
					build_repo_url(&dep_repo_info,
								0,
								dep_url,
								dep_url_size);
					dep_item_found = curl_url_get_response(dep_url);
				} 
			else
				{
					/* GitLab / Gitea fallback */
					build_repo_url(&dep_repo_info,
								0,
								dep_url,
								dep_url_size);
					dep_item_found = curl_url_get_response(dep_url);
				}

			if (!dep_item_found) {
				printf_error("Repository not found or invalid:\n\t%s", depends[i]);
				continue;
			}

			/*  Extract filename from URL  */
			const char *chr_last_slash = strrchr(dep_url, '/');
			if (chr_last_slash && *(chr_last_slash + 1)) snprintf(dep_repo_name,
																  dep_repo_name_size,
																  "%s",
																  chr_last_slash + 1);
			else snprintf(dep_repo_name, dep_repo_name_size, "%s.zip", dep_repo_info.repo);

			/*  Download  */
			wcfg.idepends = 1;
			wd_download_file(dep_url, dep_repo_name);
		}
}
