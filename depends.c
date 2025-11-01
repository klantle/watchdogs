#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "color.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "crypto.h"
#include "chain.h"
#include "depends.h"

/**
 * struct dep_curl_buffer - Buffer for CURL response data
 * @data: Pointer to allocated data
 * @size: Size of allocated data
 */
struct dep_curl_buffer {
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

/*
 * dep_implementation_samp_conf - Apply plugins name to SA-MP config
 * dep_implementation_omp_conf - Apply plugins name to Open.MP config
 */
typedef struct {
		const char *dep_config;
		const char *dep_target;
		const char *dep_added;
} depConfig;

/**
 * dep_curl_write_cb - CURL write callback function
 * @contents: Received data
 * @size: Size of each element
 * @nmemb: Number of elements
 * @userp: User pointer (dep_curl_buffer)
 *
 * Return: Number of bytes processed
 */
static size_t dep_curl_write_cb (void *contents, size_t size, size_t nmemb, void *userp)
{
		struct dep_curl_buffer *mem = (struct dep_curl_buffer *)userp;
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
 * dep_http_check_url- Check if URL is accessible
 * @url: URL to check
 *
 * Return: 1 if accessible, 0 if not
 */
int dep_check_url (const char *url, const char *github_token)
{
		CURL *curl = curl_easy_init();
		if (!curl) return 0;

		CURLcode res;
		long response_code = 0;
		struct curl_slist *headers = NULL;
		char error_buffer[CURL_ERROR_SIZE] = { 0 };

		printf("\tUsing URL: %s...\n", url);
		if (strfind(wcfg.wd_toml_github_tokens, "DO_HERE")) {
			pr_color(stdout, FCOLOUR_GREEN, "DEPENDS: Can't read Github token.. skipping\n");
			sleep(2);
		} else {
			if (github_token && strlen(github_token) > 0) {
				char auth_header[512];
				snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", github_token);
				headers = curl_slist_append(headers, auth_header);
				int reveal = 8;
				int len = strlen(github_token);
				char masked[len + 1]; 
				strncpy(masked, github_token, reveal);
				for(int i = reveal; i < len; i++) {
					masked[i] = '*';
				}
				masked[len] = '\0';
				printf("\tUsing token: %s...\n", masked);
			}
		}
		
		headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
		headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");
		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD request
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
		
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
		
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

		printf("cURL result: %s\n", curl_easy_strerror(res));
		printf("Response code: %ld\n", response_code);
		if (strlen(error_buffer) > 0) {
			printf("Error: %s\n", error_buffer);
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		return (res == CURLE_OK && response_code >= 200 && response_code < 300);
}

/**
 * dep_http_get_content - Fetch HTML content from URL
 * @url: URL to fetch
 * @out_html: Output pointer for HTML data
 *
 * Return: 1 on success, 0 on failure
 */
int dep_http_get_content (const char *url, char **out_html)
{
		CURL *curl;
		struct dep_curl_buffer buffer = { 0 };
		CURLcode res;

		curl = curl_easy_init();
		if (!curl)
				return RETZ;

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dep_curl_write_cb);
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
 * dep_get_assets - Select most appropriate asset for current platform
 * @deps_assets: Array of asset URLs
 * @count: Number of deps_assets
 * @preferred_os: Preferred OS name
 *
 * Return: Duplicated string of best asset URL
 */
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

		/* Prioritize OS-specific binaries */
		for (i = 0; i < num_patterns; i++) {
				for (j = 0; j < count; j++) {
						if (strstr(deps_assets[j], os_patterns[i]))
								return strdup(deps_assets[j]);
				}
		}

		/* Fallback to first asset */
		return strdup(deps_assets[0]);
}

/**
 * dep_parse_repo - Parse repository input string
 * @input: Input string (URL or short format)
 * @__deps_data: Output dep_repo_info structure
 *
 * Return: 1 on success, 0 on failure
 */
static int
dep_parse_repo(const char *input, struct dep_repo_info *__deps_data)
{
		char working[512];
		char *tag_ptr, *path, *first_slash;
		char *user, *repo_slash, *repo, *git_ext;

		memset(__deps_data, 0, sizeof(*__deps_data));

		/* Default host/domain values */
		strcpy(__deps_data->host, "github");
		strcpy(__deps_data->domain, "github.com");

		/* Create working copy of input */
		strncpy(working, input, sizeof(working) - 1);
		working[sizeof(working) - 1] = '\0';

		/* Parse tag if present */
		tag_ptr = strrchr(working, ':');
		if (tag_ptr) {
			*tag_ptr = '\0';
			strncpy(__deps_data->tag, tag_ptr + 1, sizeof(__deps_data->tag) - 1);

			/* Handle special tags */
			if (!strcmp(__deps_data->tag, "latest")) {
				pr_info(stdout, "Latest (:latest) tag detected, will use latest release");
			}
		}

		path = working;

		/* Skip protocol prefix */
		if (!strncmp(path, "https://", 8))
			path += 8;
		else if (!strncmp(path, "http://", 7))
			path += 7;

		/* Short deps_arch_format */
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
			/* Long format with explicit domain */
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

		/* Parse user/repo from path */
		user = path;
		repo_slash = strchr(path, '/');
		if (!repo_slash)
			return RETZ;

		*repo_slash = '\0';
		repo = repo_slash + 1;

		if (!strcmp(__deps_data->host, "sourceforge")) {
			strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);
			strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);

			if (strlen(__deps_data->user) == 0)
				strncpy(__deps_data->repo, user, sizeof(__deps_data->repo) - 1);
		} else {
			strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);

			/* Remove .git extension if present */
			git_ext = strstr(repo, ".git");
			if (git_ext)
				*git_ext = '\0';
			strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);
		}

		/* Validate required fields */
		if (strlen(__deps_data->user) == 0 || strlen(__deps_data->repo) == 0)
			return RETZ;

#if defined(_DBG_PRINT)
		pr_info(stdout, "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
			__deps_data->host,
			__deps_data->domain,
			__deps_data->user,
			__deps_data->repo,
			__deps_data->tag[0] ? __deps_data->tag : "(none)");
#endif

		return RETN;
}

/**
 * dep_gh_release_assets - Get release deps_assets from GitHub API
 * @user: Repository owner
 * @repo: Repository name
 * @tag: Release tag
 * @out_urls: Output array for URLs
 * @max_urls: Maximum number of URLs to fetch
 *
 * Return: Number of deps_assets ret
 */
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
				return RETZ;

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
 * dep_build_repo_url - Build repository URL based on host and type
 * @__deps_data: Repository information
 * @is_tag_page: Whether to build tag page URL
 * @deps_put_url: Output buffer for URL
 * @deps_put_size: Size of output buffer
 */
static void
dep_build_repo_url(const struct dep_repo_info *__deps_data, int is_tag_page,
		   char *deps_put_url, size_t deps_put_size)
{
		char deps_actual_tag[128] = { 0 };

		/* Copy tag for processing */
		if (__deps_data->tag[0]) {
			strncpy(deps_actual_tag, __deps_data->tag,
				sizeof(deps_actual_tag) - 1);

			/* For latest, we keep it as is for URL building
			* Actual conversion to latest tag happens in dep_handle_repo
			*/
			if (strcmp(deps_actual_tag, "latest") == 0) {
				/* For GitHub, latest in URL context typically means latest release */
				if (strcmp(__deps_data->host, "github") == 0 && !is_tag_page) {
					/* For download URLs, let dep_handle_repo handle conversion */
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
			/* Custom/unknown host */
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

/**
 * dep_gh_latest_tag - Get the latest tag from GitHub API
 * @user: Repository owner
 * @repo: Repository name
 * @out_tag: Output buffer for latest tag
 * @deps_put_size: Size of output buffer
 *
 * Return: 1 if found, 0 if not
 */
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
			return RETZ;

		/* Parse tag_name from JSON response */
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

		wdfree(json_data);
		return ret;
}

/**
 * dep_handle_repo - Helper function for handling repository dependencies
 */
static int dep_handle_repo (const struct dep_repo_info *dep_repo_info,
                           char *deps_put_url,
                           size_t deps_put_size)
{
		int ret = 0; 
		/// Initialize return value
		/// 0 indicates failure by default
		/// Will set to 1 if a valid source URL is found

		const char *deps_repo_branch[] = {
											"main",
											"master"
										 };
		/// List of common branch names
		/// Try "main" first, then "master" as fallback
		/// Used if tag is not specified or retrieval fails

		char deps_actual_tag[128] = { 0 };
		/// Buffer to store the resolved tag
		/// Initialized to zero
		/// Stores either the actual GitHub release tag or empty string

		if (dep_repo_info->tag[0] && strcmp(dep_repo_info->tag, "latest") == 0) {
			/// Check if the requested tag is "latest"
			/// Convert "latest" into the actual release tag
			/// Use GitHub API helper function dep_gh_latest_tag

			if (dep_gh_latest_tag(dep_repo_info->user, 
								dep_repo_info->repo, 
								deps_actual_tag, 
								sizeof(deps_actual_tag))) {
				/// Successfully retrieved latest tag from GitHub
				/// Store it in deps_actual_tag
				/// Log the tag being used for debugging
				pr_info(stdout, "Using latest tag: %s"
								"(instead of latest)", deps_actual_tag);
			} else {
				/// Failed to retrieve latest release tag
				/// Fallback to default branch approach
				/// Log an error message
				pr_error(stdout, "Failed to get latest tag for %s/%s,"
								"falling back to main branch",
								dep_repo_info->user, dep_repo_info->repo);
				strcpy(deps_actual_tag, ""); 
				/// Set to empty string to trigger branch fallback
				/// Ensures later logic will attempt main/master branches
			}
		} else {
			/// Copy the specified tag from dep_repo_info
			/// Only if it's not "latest"
			/// Ensure buffer is not overflowed
			strncpy(deps_actual_tag, dep_repo_info->tag, sizeof(deps_actual_tag) - 1);
		}

		if (deps_actual_tag[0]) {
			/// If a valid tag exists
			/// Attempt to retrieve release assets from GitHub
			/// Assets may contain pre-built binaries or archives

			char *deps_assets[10] = { 0 };
			/// Array to store asset URLs
			/// Maximum of 10 assets
			/// Initialized to NULL pointers

			int deps_asset_count = dep_gh_release_assets(dep_repo_info->user,
														 dep_repo_info->repo,
														 deps_actual_tag,
														 deps_assets,
														 10);
			/// Fetch release assets for the given tag
			/// Store in deps_assets array
			/// Returns number of assets found

			if (deps_asset_count > 0) {
				/// If assets are available
				/// Choose the best suitable asset
				char *deps_best_asset = NULL;
				if (deps_best_asset == NULL)
					deps_best_asset = dep_get_assets(deps_assets, deps_asset_count, NULL);
				/// Select the best asset based on platform/format
				/// NULL as third argument means default selection

				if (deps_best_asset) {
					/// If a suitable asset is found
					/// Copy URL to deps_put_url buffer
					/// Mark return value as success
					strncpy(deps_put_url, deps_best_asset, deps_put_size - 1);
					ret = 1;
					wdfree(deps_best_asset);
					/// Free memory allocated for best asset string
				}

				for (int j = 0; j < deps_asset_count; j++)
					wdfree(deps_assets[j]);
				/// Free memory for all other asset strings
				/// Prevent memory leaks
			}

			if (!ret) {
				/// If no asset was suitable or available
				/// Fallback to downloading tagged source archives
				const char *deps_arch_format[] = {
					"https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
					"https://github.com/%s/%s/archive/refs/tags/%s.zip"
				};
				/// Define possible archive deps_arch_format
				/// Try .tar.gz first, then .zip

				for (int j = 0; j < 2 && !ret; j++) {
					snprintf(deps_put_url, deps_put_size, deps_arch_format[j],
							 dep_repo_info->user,
							 dep_repo_info->repo,
							 deps_actual_tag);
					/// Format the URL for the archive
					/// Insert user, repo, and tag into URL

					if (dep_check_url(deps_put_url, wcfg.wd_toml_github_tokens))
						ret = 1;
					/// Check if the URL is reachable
					/// If yes, mark success
				}
			}
		} else {
			/// If no tag specified or 'latest' failed
			/// Attempt to use common branch names
			for (int j = 0; j < 2 && !ret; j++) {
				snprintf(deps_put_url, deps_put_size,
						"%s%s/%s/archive/refs/heads/%s.zip",
						"https://github.com/",
						dep_repo_info->user,
						dep_repo_info->repo,
						deps_repo_branch[j]);
				/// Format URL for branch archive
				/// Try main first, then master

				if (dep_check_url(deps_put_url, wcfg.wd_toml_github_tokens)) {
					ret = 1;
					/// Found valid branch archive
					if (j == 1)
						pr_info(stdout,
								"Using master branch (main not found)");
					/// Log if fallback to master branch occurs
				}
			}
		}

		return ret;
		/// Return 1 if a valid download URL was set
		/// Return 0 if no valid source found
		/// End of function
}

/**
 * dep_sym_convert - convert backslashes to forward slashes in path
 * @path: path string to convert
 */
void dep_sym_convert (char *path)
{
		char *p;

		for (p = path; *p; p++) {
				if (*p == '\\')
						*p = '/';
		}
}

/**
 * dep_add_ncheck_hash - add file hash to dependency list after checking duplicates
 * @_FP: full path to the file for hashing
 * @_JP: relative path for JSON storage
 */
void
dep_add_ncheck_hash(cJSON *depends, const char *_FP, const char *_JP)
{
		char convert_f_path[PATH_MAX];
		char convert_j_path[PATH_MAX];
		unsigned char hash[SHA256_DIGEST_LENGTH]; /* Buffer for SHA256 hash */
		char *hex;
		int h_exists = 0;
		int array_size;
		int j;

		char *pos;

		/* Remove "include/" from file paths */
		while ((pos = strstr(_FP, "include/")) != NULL) {
			memmove(pos, pos + strlen("include/"),
				strlen(pos + strlen("include/")) + 1);
		}

		while ((pos = strstr(_JP, "include/")) != NULL) {
			memmove(pos, pos + strlen("include/"),
				strlen(pos + strlen("include/")) + 1);
		}

		/* Convert paths to use forward slashes */
		strncpy(convert_f_path, _FP, sizeof(convert_f_path));
		convert_f_path[sizeof(convert_f_path) - 1] = '\0';
		dep_sym_convert(convert_f_path);

		strncpy(convert_j_path, _JP, sizeof(convert_j_path));
		convert_j_path[sizeof(convert_j_path) - 1] = '\0';
		dep_sym_convert(convert_j_path);

		pr_info(stdout, "\tHashing convert path: %s\n", convert_f_path);

		/* Calculate SHA256 hash of the file */
		if (sha256_hash(convert_f_path, hash) == RETN) {
			to_hex(hash, 32, &hex);

			/* Check if hash already exists in the JSON array */
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
			pr_error(stdout,
				"Failed to hash: %s (convert: %s)\n",
				convert_j_path,
				convert_f_path);
		}
}

void dep_implementation_samp_conf (depConfig config) {
		pr_info(stdout,
				"\tAdding Depends: %s",
				config.dep_added);
		
		FILE* file = fopen(config.dep_config, "r");
		if (file) {
			char line[256];
			int t_exist = 0;
			int tr_exist = 0;
			int tr_ln_has_tx = 0;
			
			while (fgets(line, sizeof(line), file)) {
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
			fclose(file);
			
			if (t_exist) {
				return;
			}
			
			if (tr_exist && !tr_ln_has_tx) {
				FILE* temp_file = fopen(".deps_tmp", "w");
				file = fopen(config.dep_config, "r");
				
				while (fgets(line, sizeof(line), file)) {
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
				
				fclose(file);
				fclose(temp_file);
				
				remove(config.dep_config);
				rename(".deps_tmp", config.dep_config);
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
#define S_ADD_PLUGIN(x, y, z) \
    dep_implementation_samp_conf((depConfig){x, y, z})

void dep_implementation_omp_conf (const char* config_name, const char* deps_name) {
		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Depends: Adding Depends '%s'",
				 deps_name);
		
		FILE* c_file = fopen(config_name, "r");
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
				!strcmp(item->valuestring, deps_name)) {
				p_exist = 1;
				break;
			}
		}
		
		if (!p_exist) {
			cJSON_AddItemToArray(legacy_plugins, cJSON_CreateString(deps_name));
		}
		
		char* __cJSON_Printed = cJSON_Print(root);
		c_file = fopen(config_name, "w");
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
void dep_add_include (const char *modes,
					  char *dep_name,
					  char *dep_after) {
		pr_color(stdout,
				 FCOLOUR_GREEN,
				 "Depends: Adding Include: '%s'",
				 dep_name);

		FILE *file = fopen(modes, "r");
		if (file == NULL) {
			return;
		}
		
		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		char *ct_modes = wdmalloc(fileSize + 1);
		fread(ct_modes, 1, fileSize, file);
		ct_modes[fileSize] = '\0';
		fclose(file);

		if (strstr(ct_modes, dep_name) != NULL &&
			strstr(ct_modes, dep_after) != NULL) {
			wdfree(ct_modes);
			return;
		}
		
		char *e_dep_after_pos = strstr(ct_modes,
					   				   dep_after);
		
		FILE *n_file = fopen(modes, "w");
		if (n_file == NULL) {
			wdfree(ct_modes);
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
		wdfree(ct_modes);
}
#define DEP_ADD_INCLUDES(x, y, z) dep_add_include(x, y, z)

/**
 * dep_get_fname - convert depends name
 */
const char *dep_get_fname (const char *path)
{
		const char *depends;

		/* Find last forward slash */
		depends = strrchr(path, '/');
		if (!depends) {
				/* If no forward slash, try backslash (Windows paths) */
				depends = strrchr(path, '\\');
		}

		return depends ? depends + 1 : path;
}

/**
 * dep_get_bname - Extract direct_bnames from file path
 * @path: Full file path
 * 
 * Return: Pointer to direct_bnames in path
 */
static const char *dep_get_bname (const char *path)
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
 * dep_pr_include_directive - Add include directive to main file
 * @deps_include: Include deps_include
 */
static void dep_pr_include_directive (const char *deps_include)
{
		char errbuf[256];
		toml_table_t *_toml_config;
		char depends_name[PATH_MAX];
		char idirective[MAX_PATH];
		
		/* Extract Basename */
		const char *dep_n = dep_get_fname(deps_include);
		snprintf(depends_name, sizeof(depends_name), "%s", dep_n);
		
		const char *direct_bnames = dep_get_bname(depends_name);
		
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
				"#include <%s>", direct_bnames);

		/* Add to main file */
		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		} else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <open.mp>");
		} else {
			DEP_ADD_INCLUDES(wcfg.wd_toml_gm_input,
							 idirective,
							 "#include <a_samp>");
		}
}

/**
 * dep_pr_include_files - process all `.inc` files recursively
 * @depends: cJSON dependency array
 * @bp: base path to start scanning
 * @db: destination base directory
 */
void dep_pr_include_files (cJSON *depends, const char *bp, const char *db)
{
		DIR *dir;
		struct dirent *item;
		struct stat st;
		char cmd[MAX_PATH * 3], fpath[PATH_MAX * 2], 
			 parent[PATH_MAX], dest[PATH_MAX];
		char *dname, *ext;

		dir = opendir(bp);
		if (!dir)
			return;

		while ((item = readdir(dir)) != NULL) {
			if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
				continue;

			snprintf(fpath, sizeof(fpath), "%s/%s", bp, item->d_name);

			if (stat(fpath, &st) != 0)
				continue;

			if (S_ISDIR(st.st_mode)) {
				dep_pr_include_files(depends, fpath, db);
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

			++dname; /* skip '/' */

			snprintf(dest, sizeof(dest), "%s/%s", db, dname);

			/* Try moving directory */
			if (rename(parent, dest)) {
				if (is_native_windows()) {
					snprintf(cmd, sizeof(cmd),
						"xcopy \"%s\" \"%s\" /E /I /H /Y && rmdir /S /Q \"%s\"",
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
static void dep_pr_file_type (const char *path, const char *pattern, 
                              const char *exclude, const char *cwd, 
                              cJSON *depends, const char *target_dir, int root)
{
		char cp_cmd[MAX_PATH * 2];
		char json_item[PATH_MAX];
		
		wd_sef_fdir_reset();
		int found = wd_sef_fdir(path, pattern, exclude);
		
		if (found) {
			for (int i = 0; i < wcfg.wd_sef_count; ++i) {
				const char *deps_names = dep_get_fname(wcfg.wd_sef_found_list[i]),
						   *deps_bnames = dep_get_bname(wcfg.wd_sef_found_list[i]);
				
				/* Move file */
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
				
				/* Add to hash list */
				snprintf(json_item, sizeof(json_item), "%s", deps_names);
				dep_add_ncheck_hash(depends, json_item, json_item);
				
				/* Skip adding if plugins on root */
				if (root != 1) goto done;

				/* Add to config */
				if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
					M_ADD_PLUGIN(wcfg.wd_toml_config,
								 deps_bnames);
				else
					S_ADD_PLUGIN(wcfg.wd_toml_config,
								"plugins", deps_bnames);
			}
		}
done:
		return;
}

/**
 * dep_move_files - Move dependency files and maintain cache
 * @dep_dir: Path to dependencies folder
 */
void dep_cjson_additem (cJSON *p1, int p2, cJSON *p3)
{
		cJSON *_e_item = cJSON_GetArrayItem(p1, p2);
		if (cJSON_IsString(_e_item))
			cJSON_AddItemToArray(p3,
				cJSON_CreateString(_e_item->valuestring));
}

void dep_move_files (const char *dep_dir)
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
		if (!wd_log_acces) {
			if (is_native_windows())
				system("type nul > wd_depends.json");
			else
				system("touch wd_depends.json");
		}

		/* Create JSON root object */
		root = cJSON_CreateObject();
		depends = cJSON_CreateArray();
		cJSON_AddStringToObject(root, "comment", " -- cache file");

		/* Load existing cache */
		e_file = fopen("wd_depends.json", "r");
		if (!e_file)
			return;

		fseek(e_file, 0, SEEK_END);
		fp_cache_sz = ftell(e_file);
		fseek(e_file, 0, SEEK_SET);

		if (fp_cache_sz <= 0)
			goto out_close;

		file_content = wdmalloc(fp_cache_sz + 1);
		if (!file_content)
			goto out_close;

		fread(file_content, 1, fp_cache_sz, e_file);
		file_content[fp_cache_sz] = '\0';

		_e_root = cJSON_Parse(file_content);
		if (!_e_root)
			goto out_free;

		cJSON *_e_depends;
		_e_depends = cJSON_GetObjectItem(_e_root, "depends");
		if (_e_depends && cJSON_IsArray(_e_depends)) {
			int cnt = cJSON_GetArraySize(_e_depends);
			int i;

			for (i = 0; i < cnt; i++)
				dep_cjson_additem(_e_depends, i, depends);

			pr_info(stdout, "Loaded %d existing hashes", cnt);
		}

out_free:
		wdfree(file_content);
out_close:
		fclose(e_file);

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

		dep_pr_include_files(depends, dep_dir, d_b);

		/* Process legacy include files */
		wd_sef_fdir_reset();
		__inc_legacyplug_r = wd_sef_fdir(dep_dir, "*.inc", dep_inc_path);
		if (__inc_legacyplug_r) {
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

				dep_add_ncheck_hash(depends, fi_depends_name, fi_depends_name);
				dep_pr_include_directive(fi_depends_name);
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

		if (is_native_windows())
			snprintf(rm_cmd, sizeof(rm_cmd),
				"rmdir /s /q \"%s\"",
				dep_dir);
		else
			snprintf(rm_cmd, sizeof(rm_cmd),
				"rm -rf %s",
				dep_dir);
		system(rm_cmd);
}

/**
 * wd_apply_depends - apply depends to gamemodes
 * @depends_name: name of the dependency to apply
 */
void wd_apply_depends (const char *depends_name)
{
		char _depends[PATH_MAX];
		/// Temporary buffer to store dependency name without extension
		/// Size limited by PATH_MAX
		/// Initialized later using snprintf

		char dep_dir[PATH_MAX];
		/// Buffer to store directory name derived from dependency
		/// Will be used for moving/extracting files
		/// Size limited by PATH_MAX

		struct stat st;
		/// Structure used for filesystem status checks
		/// Used with stat() to check existence of directories
		/// Helps decide if mkdir_recursive is needed

		char *__dot_ext;
		/// Pointer to locate the last dot in dependency name
		/// Used to strip file extension
		/// Will point to '.' in filenames like "lib.zip"

		snprintf(_depends, PATH_MAX, "%s", depends_name);
		/// Copy dependency name into temporary buffer
		/// Ensures safe null-terminated string
		/// Original depends_name remains unchanged

		__dot_ext = strrchr(_depends, '.');
		/// Find last occurrence of '.' in filename
		/// Helps identify file extension
		/// Returns NULL if no dot found

		if (__dot_ext)
			*__dot_ext = '\0';
		/// Remove file extension by replacing dot with null terminator
		/// Converts "lib.zip" -> "lib"
		/// Prepares directory name for extraction/move

		snprintf(dep_dir, sizeof(dep_dir), "%s", _depends);
		/// Copy stripped name into dep_dir buffer
		/// dep_dir will be used for file operations
		/// Safe string copy with size limit

		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
			/// If project type is SA-MP
			/// Perform SA-MP specific directory setup
			/// Check and create required folders

			if (stat("pawno/include", &st) != 0 && errno == ENOENT)
				mkdir_recursive("pawno/include");
			/// If "pawno/include" does not exist
			/// Create directory recursively
			/// Ensures header files can be placed

			if (stat("plugins", &st) != 0 && errno == ENOENT)
				mkdir_recursive("plugins");
			/// If "plugins" folder does not exist
			/// Create it recursively
			/// Ensures plugin files can be placed
		} else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
			/// If project type is OMP (OpenMP)
			/// Perform OMP specific directory setup
			/// Check and create required folders

			if (stat("qawno/include", &st) != 0 && errno == ENOENT)
				mkdir_recursive("qawno/include");
			/// If "qawno/include" does not exist
			/// Create recursively
			/// Ensures OMP headers can be placed

			if (stat("components", &st) != 0 && errno == ENOENT)
				mkdir_recursive("components");
			/// If "components" folder does not exist
			/// Create recursively
			/// Used for modular OMP components

			if (stat("plugins", &st) != 0 && errno == ENOENT)
				mkdir_recursive("plugins");
			/// If "plugins" folder does not exist
			/// Create recursively
			/// OMP plugins go here
		}

		dep_move_files(dep_dir);
		/// Move/extract files from dependency directory
		/// Handles copying headers, scripts, plugins, or assets
		/// Final step to integrate dependency into project
}

/**
 * wd_install_depends - install dependencies from string
 * @deps_str: string containing dependencies to install
 */
void wd_install_depends (const char *deps_str)
{
		char buffer[1024];
		/// Temporary buffer to copy the input dependency string
		/// Used for tokenization with strtok
		/// Maximum size 1024 bytes

		const char *depends[MAX_DEPENDS];
		/// Array of dependency tokens
		/// Stores pointers to each dependency name
		/// Maximum count defined by MAX_DEPENDS

		char *depends_files[MAX_DEPENDS];
		/// Array of downloaded dependency filenames
		/// Will store strdup copies of downloaded files
		/// To be used later in wd_apply_depends

		char *token;
		/// Pointer used for strtok tokenization
		/// Points to current dependency in loop
		/// Updated in while loop

		int DEP_CNT = 0;
		/// Counter for number of dependencies parsed
		/// Initially zero
		/// Incremented for each valid token

		int file_count = 0;
		/// Counter for successfully downloaded dependency files
		/// Used for applying dependencies later
		/// Incremented only when download succeeds

		if (!deps_str || !*deps_str) {
			/// Check if input string is NULL or empty
			/// No dependencies to process
			/// Log __deps_data and return early
			pr_info(stdout, "No valid dependencies to install");
			return;
		}

		wcfg.wd_idepends = 0;
		/// Mark installation in progress
		/// Used by configuration/state flags
		/// Prevents re-entry or race conditions

		snprintf(buffer, sizeof(buffer), "%s", deps_str);
		/// Copy input string into buffer safely
		/// Avoid modifying original deps_str
		/// Ensures null termination

		token = strtok(buffer, " ");
		/// Begin tokenizing the buffer by spaces
		/// Each token represents a dependency
		/// strtok modifies buffer in-place

		while (token && DEP_CNT < MAX_DEPENDS) {
			depends[DEP_CNT++] = token;
			/// Store pointer to token in depends array
			/// Increment DEP_CNT
			/// Stop when MAX_DEPENDS reached

			token = strtok(NULL, " ");
			/// Get next token
			/// Returns NULL if no more tokens
			/// Loop continues until all tokens processed
		}

		if (DEP_CNT == 0) {
			/// If no valid dependencies were parsed
			/// Log __deps_data message
			/// Return early
			pr_info(stdout, "No valid dependencies to install");
			return;
		}

		int i;
		/// Loop index for processing each dependency
		/// Declared outside loop for reuse

		for (i = 0; i < DEP_CNT; i++) {
			int dep_item_found = 0;
			/// Flag indicating if dependency source is found
			/// Initialized to 0 (not found)
			/// Set to 1 if valid URL or asset is located

			struct dep_repo_info dep_repo_info;
			/// Structure to hold parsed repository __deps_data
			/// Filled by dep_parse_repo
			/// Includes host, user, repo, tag, etc.

			char dep_url[1024] = { 0 };
			/// Buffer to store constructed dependency URL
			/// Initialized to zero
			/// Used in downloading dependency

			char dep_repo_name[256] = { 0 };
			/// Buffer to store local filename for dependency
			/// Derived from repo URL or tag
			/// Used in wd_download_file

			const char *chr_last_slash;
			/// Pointer used to find last '/' in URL
			/// Helps extract file name from URL
			/// NULL if '/' not found

			if (!dep_parse_repo(depends[i], &dep_repo_info)) {
				/// Parse dependency string into dep_repo_info
				/// Return 0 if format invalid
				/// Log error and skip this dependency
				pr_error(stdout, "Invalid repo format: %s", depends[i]);
				continue;
			}

			if (!strcmp(dep_repo_info.host, "github")) {
				/// If repository host is GitHub
				/// Use dep_handle_repo to resolve latest tag, release assets, or branch
				dep_item_found = dep_handle_repo(&dep_repo_info,
												dep_url,
												sizeof(dep_url));
			} else {
				/// Non-GitHub repositories
				/// Build URL manually
				/// Then check if reachable
				dep_build_repo_url(&dep_repo_info, 0, dep_url, sizeof(dep_url));
				dep_item_found = dep_check_url(dep_url, wcfg.wd_toml_github_tokens);
			}

			if (!dep_item_found) {
				/// Dependency not found or unreachable
				/// Log error and skip
				pr_error(stdout, "Repository not found: %s", depends[i]);
				continue;
			}

			chr_last_slash = strrchr(dep_url, '/');
			/// Find last '/' in URL
			/// Used to extract file name for download

			if (chr_last_slash && *(chr_last_slash + 1)) {
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s",
						chr_last_slash + 1);
				/// Use substring after last '/' as filename
				/// Typical for GitHub archives
			} else {
				snprintf(dep_repo_name, sizeof(dep_repo_name), "%s.zip",
						dep_repo_info.repo);
				/// Fallback: use repo name plus .zip
				/// Ensures a valid local filename
			}

			if (wd_download_file(dep_url, dep_repo_name) == RETZ)
				depends_files[file_count++] = strdup(dep_repo_name);
			/// Download dependency file
			/// If successful, store copy of filename in depends_files
			/// Increment file_count
		}

		wcfg.wd_idepends = 1;
		/// Mark dependency installation as completed
		/// Prevents further interference
		/// State flag updated for configuration

		for (i = 0; i < file_count; i++) {
			wd_apply_depends(depends_files[i]);
			/// Apply each downloaded dependency
			/// This may extract, compile, or copy files

			wdfree(depends_files[i]);
			/// Free memory allocated by strdup
			/// Avoid memory leaks
		}
}
