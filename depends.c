#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SYM "\\"
#define MKDIR(path) _mkdir(path)
#define SLEEP(sec) Sleep((sec) * 1000)
#define SETENV(name, val, overwrite) _putenv_s(name, val)
#else
#include <unistd.h>
#define PATH_SYM "/"
#define MKDIR(path) mkdir(path, 0755)
#define SLEEP(sec) sleep(sec)
#define SETENV(name, val, overwrite) setenv(name, val, overwrite)
#endif

#include <curl/curl.h>
#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"

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
 * struct repo_info - Repository information structure
 * @host: Host name (github, gitlab, gitea, sourceforge)
 * @domain: Domain name
 * @user: Repository owner
 * @repo: Repository name  
 * @tag: Tag or branch name
 */
struct repo_info {
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

		ptr = realloc(mem->data, mem->size + realsize + 1);
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
		struct curl_buffer buffer = {0};
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
				free(buffer.data);
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
		const char *zip, *tar, *found, *href;
		size_t len, ulen;

		while (p) {
				zip = strstr(p, ".zip");
				tar = strstr(p, ".tar.gz");
				found = NULL;
				len = 0;

				if (zip && (!tar || zip < tar)) {
						found = zip;
						len = zip - p + 4;
				} else if (tar) {
						found = tar;
						len = tar - p + 7;
				} else {
						break;
				}

				/* Find href=" */
				href = found;
				while (href > p && *(href - 1) != '"')
						href--;

				ulen = len + (found - href);
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
				p = found + len;
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
 * @info: Output repo_info structure
 *
 * Return: 1 on success, 0 on failure
 */
static int parse_repo_input(const char *input, struct repo_info *info)
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
 * Return: Number of assets found
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
				out_urls[url_count] = malloc(url_len + 1);
				strncpy(out_urls[url_count], p, url_len);
				out_urls[url_count][url_len] = '\0';

				url_count++;
				p = url_end + 1;
		}

		free(json_data);
		return url_count;
}

/**
 * check_github_branch - Check if GitHub branch exists
 * @user: Repository owner
 * @repo: Repository name  
 * @branch: Branch name
 *
 * Return: 1 if exists, 0 if not
 */
static int check_github_branch(const char *user, const char *repo, const char *branch)
{
		char url[PATH_MAX];

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
static void build_repo_url(const struct repo_info *info, int is_tag_page,
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
 * wd_install_packages - Install packages from repositories
 * @pkg_one: First package specification
 * @pkg_two: Second package specification
 */
void wd_install_packages(char *pkg_one, char *pkg_two)
{
		char *packages[] = { pkg_one, pkg_two };
		int num_packages = 2;
		int i;

		for (i = 0; i < num_packages; i++) {
				struct repo_info repo_info;
				char pkg_url[PATH_MAX];
				char repo_name[256];
				char *last_slash;
				int pkg_found = 0;

				if (!packages[i])
						continue;

				if (!parse_repo_input(packages[i], &repo_info)) {
						printf_error("Invalid repo format: %s", packages[i]);
						continue;
				}

				printf_info("Parsed: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
						   repo_info.host, repo_info.domain, repo_info.user,
						   repo_info.repo, repo_info.tag[0] ? repo_info.tag : "main");

				/* Handle based on host */
				if (strcmp(repo_info.host, "github") == 0) {
						if (repo_info.tag[0]) {
								char *assets[10];
								int asset_count;

								asset_count = get_github_release_assets(
										repo_info.user, repo_info.repo, repo_info.tag,
										assets, 10);

								if (asset_count > 0) {
										char *selected_asset = select_best_asset(assets, asset_count, NULL);
										if (selected_asset) {
												strncpy(pkg_url, selected_asset, sizeof(pkg_url) - 1);
												pkg_found = 1;
												free(selected_asset);
										}
										for (int j = 0; j < asset_count; j++)
												free(assets[j]);
								}

								/* Fallback to source archive */
								if (!pkg_found) {
										const char *source_formats[] = {
												"https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
												"https://github.com/%s/%s/archive/refs/tags/%s.zip",
										};
										for (int j = 0; j < 2; j++) {
												snprintf(pkg_url, sizeof(pkg_url), source_formats[j],
														 repo_info.user, repo_info.repo, repo_info.tag);
												if (curl_url_get_response(pkg_url)) {
														pkg_found = 1;
														break;
												}
										}
								}
						} else {
								/* No tag - find branch */
								const char *branches[] = {"main", "master"};
								for (int j = 0; j < 2; j++) {
										snprintf(pkg_url, sizeof(pkg_url),
												 "https://github.com/%s/%s/archive/refs/heads/%s.zip",
												 repo_info.user, repo_info.repo, branches[j]);
										if (curl_url_get_response(pkg_url)) {
												pkg_found = 1;
												if (j == 1)
														printf_info("Using master branch (main not found)");
												break;
										}
								}
						}
				} else if (strcmp(repo_info.host, "sourceforge") == 0) {
						build_repo_url(&repo_info, 0, pkg_url, sizeof(pkg_url));
						if (curl_url_get_response(pkg_url)) {
								pkg_found = 1;
						}
				} else {
						/* GitLab & Gitea */
						build_repo_url(&repo_info, 0, pkg_url, sizeof(pkg_url));
						if (curl_url_get_response(pkg_url)) {
								pkg_found = 1;
						}
				}

				if (!pkg_found) {
						printf_error("Repository not found: %s", packages[i]);
						continue;
				}

				printf_info("Repository found: %s", pkg_url);

				/* Extract filename */
				last_slash = strrchr(pkg_url, '/');
				if (last_slash) {
						snprintf(repo_name, sizeof(repo_name), "%s", last_slash + 1);
				} else {
						snprintf(repo_name, sizeof(repo_name), "%s.zip", repo_info.repo);
				}

				printf_info("Downloading %s from %s", repo_name, pkg_url);
				wd_download_file(pkg_url, repo_name);
		}
}