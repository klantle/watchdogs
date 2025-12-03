#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "crypto.h"
#include "units.h"
#include "depends.h"

/* Global buffers for dependency management operations */
char working[WG_PATH_MAX * 2];          /* Working buffer for path manipulation */
char d_command[WG_MAX_PATH * 4];        /* Buffer for system commands */
char *tag;                              /* Version tag for dependencies */
char *path;                             /* Path component of dependency URL */
char *path_slash;                       /* Pointer to path separator */
char *user;                             /* Repository owner/username */
char *repo;                             /* Repository name */
char *repo_slash;                       /* Pointer to repository separator */
char *git_dir;                          /* Pointer to .git extension */
char *depends_filename;                 /* Extracted filename */
char *extension;                        /* File extension */

/* 
 * Convert Windows path separators to Unix/Linux style
 * Replaces backslashes with forward slashes for cross-platform compatibility
 */
void dep_sym_convert(char *path)
{
        char *pos;
        for (pos = path; *pos; ++pos) {
                if (*pos == __PATH_CHR_SEP_WIN32)
                        *pos = __PATH_CHR_SEP_LINUX;
        }
}

/* 
 * Extract filename from full path (excluding directory)
 * Returns pointer to filename component
 */
const char *dep_get_fname(const char *path) {
        const char *depends = PATH_SEPARATOR(path);
        return depends ? depends + 1 : path;
}

/* 
 * Extract basename from full path (similar to dep_get_fname)
 * Alternative implementation for filename extraction
 */
static const char *dep_get_bname(const char *path) {
        const char *p = PATH_SEPARATOR(path);
        return p ? p + 1 : path;
}

/* 
 * Validate URL accessibility using HTTP HEAD request
 * Checks if a given URL is reachable and returns appropriate status code
 */
int dep_url_checking(const char *url, const char *github_token)
{
        CURL *curl = curl_easy_init();
        if (!curl) return 0;

        CURLcode res;
        long response_code = 0;
        struct curl_slist *headers = NULL;
        char error_buffer[CURL_ERROR_SIZE] = { 0 };

        printf("\tCreate & Checking URL: %s...\t\t[All good]\n", url);
        
        /* Add GitHub authentication header if token is available */
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

        /* Set standard HTTP headers */
        headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3+json");

        /* Configure CURL for HEAD request (NOBODY = 1) */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); /* HEAD request */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        printf("   Connecting... ");
        fflush(stdout);
        
        /* First perform to establish connection */
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        /* Configure error buffer for detailed error reporting */
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);

        /* Configure SSL certificate verification */
        verify_cacert_pem(curl);

        fflush(stdout);
        
        /* Perform actual HEAD request */
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        /* Display results with success/failure indicators */
        if (response_code == WG_CURL_RESPONSE_OK && strlen(error_buffer) == 0) {
                printf("cURL result: %s\t\t[All good]\n", curl_easy_strerror(res));
                printf("Response code: %ld\t\t[All good]\n", response_code);
        } else {
                if (strlen(error_buffer) > 0) {
                        printf("Error: %s\t\t[Fail]\n", error_buffer);
                } else {
                        printf("cURL result: %s\t\t[Fail]\n", curl_easy_strerror(res));
                }
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        /* Return success if HTTP status is 2xx */
        return (response_code >= 200 && response_code < 300);
}

/* 
 * Download and retrieve content from a URL
 * Returns HTML/JSON content in dynamically allocated buffer
 */
int dep_http_get_content(const char *url, const char *github_token, char **out_html)
{
        CURL *curl;
        CURLcode res;
        struct curl_slist *headers = NULL;
        struct dep_curl_buffer buffer = { 0 }; /* Buffer for downloaded content */

        curl = curl_easy_init();
        if (!curl)
                return 0;

        /* Add GitHub authentication if token provided */
        if (github_token && strlen(github_token) > 0 && !strstr(github_token, "DO_HERE")) {
                char auth_header[512];
                wg_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", github_token);
                headers = curl_slist_append(headers, auth_header);
        }

        /* Set standard headers */
        headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        /* Configure download options */
        curl_easy_setopt(curl, CURLOPT_URL, url);
        memory_struct_init((void *)&buffer);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
        memory_struct_free((void *)&buffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        /* Configure SSL verification */
        verify_cacert_pem(curl);

        /* Perform download */
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);

        /* Check for errors */
        if (res != CURLE_OK || buffer.size == 0) {
                wg_free(buffer.data);
                return 0;
        }

        /* Return downloaded content */
        *out_html = buffer.data;
        return 1;
}

/* 
 * Check if filename indicates OS-specific archive
 * Identifies platform-specific files by name patterns
 */
static int is_os_specific_archive(const char *filename)
{
const char *os_patterns[] = {
        "windows", "win", ".exe", "msvc", "mingw",
        "macos", "mac", "darwin", "osx", "linux",
        "ubuntu", "debian", "cent", "centos", "cent_os", "fedora", "arch", "archlinux",
        "alphine", "redhat", "redhatenterprise", "redhatenterpriselinux", "linuxmint", "mint",
        "x86", "x64", "x86_64", "amd64", "arm", "arm64", "aarch64",
        NULL
};      
        /* Check if filename contains any OS pattern */
        for (int i = 0; os_patterns[i] != NULL; i++) {
                if (strstr(filename, os_patterns[i])) {
                        return 1;
                }
        }
        return 0;
}

/* 
 * Check if filename indicates server/gamemode archive
 * Identifies server-related files by name patterns
 */
static int is_gamemode_archive(const char *filename)
{
const char *gamemode_patterns[] = {
        "server", "_server",
        NULL
};        
        for (int i = 0; gamemode_patterns[i] != NULL; i++) {
                if (strstr(filename, gamemode_patterns[i])) {
                        return 1;
                }
        }
        return 0;
}

/* 
 * Select most appropriate asset from available options
 * Prioritizes server assets, then OS-specific assets, then neutral assets
 */
static char *dep_get_assets(char **deps_assets, int cnt, const char *preferred_os)
{
        int i, j;
        /* Platform-specific patterns for asset selection */
const char *os_patterns[] = {
#ifdef WG_WINDOWS
        "windows", "win", ".exe", "msvc", "mingw"
#else
        "ubuntu", "debian", "cent", "centos", "cent_os", "fedora", "arch", "archlinux",
        "alphine", "redhat", "redhatenterprise", "redhatenterpriselinux", "linuxmint", "mint"
#endif
        "src", "source"  /* Source code indicators */
};
        size_t num_patterns = sizeof(os_patterns) / sizeof(os_patterns[0]);

        if (cnt == 0)
                return NULL;
        if (cnt == 1)
                return strdup(deps_assets[0]);

        /* Priority 1: Server/gamemode assets */
        for (i = 0; i < cnt; i++) {
                if (strstr(deps_assets[i], "server") || 
                        strstr(deps_assets[i], "_server")) {
                        return strdup(deps_assets[i]);
                }
        }

        /* Priority 2: OS-specific assets */
        for (i = 0; i < num_patterns; i++) {
                for (j = 0; j < cnt; j++) {
                        if (strstr(deps_assets[j], os_patterns[i]))
                                return strdup(deps_assets[j]);
                }
        }

        /* Priority 3: Neutral assets (no OS pattern) */
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

        /* Fallback: First asset */
        return strdup(deps_assets[0]);
}

/* 
 * Parse repository URL/identifier into structured components
 * Extracts host, domain, user, repo, and tag from dependency string
 */
static int
dep_parse_repo(const char *input, struct dep_repo_info *__deps_data)
{
        memset(__deps_data, 0, sizeof(*__deps_data));

        /* Default to GitHub */
        wg_strcpy(__deps_data->host, "github");
        wg_strcpy(__deps_data->domain, "github.com");

        wg_strncpy(working, input, sizeof(working) - 1);
        working[sizeof(working) - 1] = '\0';

        /* Extract tag (version) if present (format: user/repo:tag) */
        tag = strrchr(working, ':');
        if (tag) {
                *tag = '\0';
                wg_strncpy(__deps_data->tag, tag + 1, sizeof(__deps_data->tag) - 1);

                if (!strcmp(__deps_data->tag, "latest")) {
                        printf("Spot ");
                        pr_color(stdout, FCOLOUR_CYAN, ":latest ");
                        printf("tag, rolling with the freshest release..\t\t[All good]\n");
                }
        }

        path = working;

        /* Strip protocol prefix */
        if (!strncmp(path, "https://", 8))
                path += 8;
        else if (!strncmp(path, "http://", 7))
                path += 7;

        /* Handle shorthand GitHub format */
        if (!strncmp(path, "github/", 7)) {
                wg_strcpy(__deps_data->host, "github");
                wg_strcpy(__deps_data->domain, "github.com");
                path += 7;
        } else {
                /* Parse custom domain format (e.g., gitlab.com/user/repo) */
                path_slash = strchr(path, __PATH_CHR_SEP_LINUX);
                if (path_slash && strchr(path, '.') && strchr(path, '.') < path_slash) {
                        char domain[128];

                        wg_strncpy(domain, path, path_slash - path);
                        domain[path_slash - path] = '\0';

                        if (strstr(domain, "github")) {
                                wg_strcpy(__deps_data->host, "github");
                                wg_strcpy(__deps_data->domain, "github.com");
                        } else {
                                wg_strncpy(__deps_data->domain, domain,
                                        sizeof(__deps_data->domain) - 1);
                                wg_strcpy(__deps_data->host, "custom");
                        }

                        path = path_slash + 1;
                }
        }

        /* Extract user and repository */
        user = path;
        repo_slash = strchr(path, __PATH_CHR_SEP_LINUX);
        if (!repo_slash)
                return 0;

        *repo_slash = '\0';
        repo = repo_slash + 1;

        wg_strncpy(__deps_data->user, user, sizeof(__deps_data->user) - 1);

        /* Remove .git extension if present */
        git_dir = strstr(repo, ".git");
        if (git_dir)
                *git_dir = '\0';
        wg_strncpy(__deps_data->repo, repo, sizeof(__deps_data->repo) - 1);

        if (strlen(__deps_data->user) == 0 || strlen(__deps_data->repo) == 0)
                return 0;

#if defined(_DBG_PRINT)
        /* Debug output: display parsed repository information */
        char size_title[WG_PATH_MAX * 3];
        wg_snprintf(size_title, sizeof(size_title), "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
                __deps_data->host,
                __deps_data->domain,
                __deps_data->user,
                __deps_data->repo,
                __deps_data->tag[0] ? __deps_data->tag : "(none)");
        wg_console_title(size_title);
#endif

        return 1;
}

/* 
 * Fetch release asset URLs from GitHub repository
 * Retrieves downloadable assets for a specific release tag
 */
static int dep_gh_release_assets(const char *user, const char *repo,
                                 const char *tag, char **out_urls, int max_urls)
{
        char api_url[WG_PATH_MAX * 2];
        char *json_data = NULL;
        const char *p;
        int url_count = 0;

        /* Build GitHub API URL for specific release tag */
        wg_snprintf(api_url, sizeof(api_url),
                         "%sapi.github.com/repos/%s/%s/releases/tags/%s",
                         "https://", user, repo, tag);

        /* Fetch release information JSON */
        if (!dep_http_get_content(api_url, wgconfig.wg_toml_github_tokens, &json_data))
                return 0;

        /* Parse JSON to extract browser_download_url entries */
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

/* 
 * Construct repository URL based on parsed information
 * Builds appropriate URL for GitHub releases, tags, or branches
 */
static void
dep_build_repo_url(const struct dep_repo_info *__deps_data, int is_tag_page,
                   char *deps_put_url, size_t deps_put_size)
{
        char deps_actual_tag[128] = { 0 };

        if (__deps_data->tag[0]) {
                wg_strncpy(deps_actual_tag, __deps_data->tag,
                        sizeof(deps_actual_tag) - 1);

                /* Handle "latest" tag special case */
                if (strcmp(deps_actual_tag, "latest") == 0) {
                        if (strcmp(__deps_data->host, "github") == 0 && !is_tag_page) {
                                wg_strcpy(deps_actual_tag, "latest");
                        }
                }
        }

        /* GitHub-specific URL construction */
        if (strcmp(__deps_data->host, "github") == 0) {
                if (is_tag_page && deps_actual_tag[0]) {
                        /* Tag page URL (for display/verification) */
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
                        /* Direct archive URL for specific tag */
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
                        /* Default to main branch ZIP */
                        wg_snprintf(deps_put_url, deps_put_size,
                                "https://%s/%s/%s/archive/refs/heads/main.zip",
                                __deps_data->domain,
                                __deps_data->user,
                                __deps_data->repo);
                }
        }

#if defined(_DBG_PRINT)
        /* Debug output: display constructed URL */
        pr_info(stdout, "Built URL: %s (is_tag_page=%d, tag=%s)",
                deps_put_url, is_tag_page,
                deps_actual_tag[0] ? deps_actual_tag : "(none)");
#endif
}

/* 
 * Fetch latest release tag from GitHub repository
 * Uses GitHub API to get the most recent release tag
 */
static int dep_gh_latest_tag(const char *user, const char *repo,
                             char *out_tag, size_t deps_put_size)
{
        char api_url[WG_PATH_MAX * 2];
        char *json_data = NULL;
        const char *p;
        int ret = 0;

        /* Build GitHub API URL for latest release */
        wg_snprintf(api_url, sizeof(api_url),
                        "%sapi.github.com/repos/%s/%s/releases/latest",
                        "https://", user, repo);

        /* Fetch latest release information */
        if (!dep_http_get_content(api_url, wgconfig.wg_toml_github_tokens, &json_data))
                return 0;

        /* Parse JSON to extract tag_name field */
        p = strstr(json_data,
                           "\"tag_name\"");
        if (p) {
                p = strchr(p, ':');
                if (p) {
                        ++p; // skip colon
                        /* Skip whitespace */
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

/* 
 * Process repository information to determine download URL
 * Handles latest tag resolution, asset selection, and fallback strategies
 */
static int dep_handle_repo(const struct dep_repo_info *dep_repo_info,
                        char *deps_put_url,
                        size_t deps_put_size)
{
        int ret = 0;
        const char *deps_repo_branch[] = {"main", "master"}; /* Common branch names */
        char deps_actual_tag[128] = {0};
        int use_fallback_branch = 0;

        /* Handle "latest" tag by resolving to actual tag name */
        if (dep_repo_info->tag[0] && strcmp(dep_repo_info->tag, "latest") == 0) {
                if (dep_gh_latest_tag(dep_repo_info->user,
                        dep_repo_info->repo,
                        deps_actual_tag,
                        sizeof(deps_actual_tag))) {
                        printf("Create latest tag: %s (~instead of latest)\t\t[All good]\n", deps_actual_tag);
                } else {
                        pr_error(stdout, "Failed to get latest tag for %s/%s,"
                                        "Falling back to main branch\t\t[Fail]",
                                        dep_repo_info->user, dep_repo_info->repo);
                        use_fallback_branch = 1;
                }
        } else {
                wg_strncpy(deps_actual_tag, dep_repo_info->tag, sizeof(deps_actual_tag) - 1);
        }

        /* Fallback to main/master branch if latest tag resolution failed */
        if (use_fallback_branch) {
                for (int j = 0; j < 2 && !ret; j++) {
                        wg_snprintf(deps_put_url, deps_put_size,
                                        "https://github.com/%s/%s/archive/refs/heads/%s.zip",
                                        dep_repo_info->user, dep_repo_info->repo,
                                        deps_repo_branch[j]);

                        if (dep_url_checking(deps_put_url, wgconfig.wg_toml_github_tokens)) {
                                ret = 1;
                                if (j == 1) printf("Create master branch (main branch not found)\t\t[All good]\n");
                        }
                }
                return ret;
        }

        /* Process tagged releases */
        if (deps_actual_tag[0]) {
                char *deps_assets[10] = {0};
                int deps_asset_count = dep_gh_release_assets(dep_repo_info->user,
                                                        dep_repo_info->repo, deps_actual_tag,
                                                        deps_assets, 10);

                /* If release has downloadable assets */
                if (deps_asset_count > 0) {
                        char *deps_best_asset = NULL;
                        deps_best_asset = dep_get_assets(deps_assets, deps_asset_count, NULL);

                        if (deps_best_asset) {
                                wg_strncpy(deps_put_url, deps_best_asset, deps_put_size - 1);
                                ret = 1;
                                
                                /* Log asset selection rationale */
                                if (is_gamemode_archive(deps_best_asset)) {
                                        printf("Selected gamemode archive: %s\t\t[All good]\n", deps_best_asset);
                                } else if (is_os_specific_archive(deps_best_asset)) {
                                        printf("Selected OS-specific archive: %s\t\t[All good]\n", deps_best_asset);
                                } else {
                                        printf("Selected neutral archive: %s\t\t[All good]\n", deps_best_asset);
                                }
                                
                                wg_free(deps_best_asset);
                        }

                        /* Clean up asset array */
                        for (int j = 0; j < deps_asset_count; j++)
                                wg_free(deps_assets[j]);
                }

                /* If no assets found, try standard archive formats */
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
                /* No tag specified - try main/master branches */
                for (int j = 0; j < 2 && !ret; j++) {
                        wg_snprintf(deps_put_url, deps_put_size,
                                        "%s%s/%s/archive/refs/heads/%s.zip",
                                        "https://github.com/",
                                        dep_repo_info->user,
                                        dep_repo_info->repo,
                                        deps_repo_branch[j]);

                        if (dep_url_checking(deps_put_url, wgconfig.wg_toml_github_tokens)) {
                                ret = 1;
                                if (j == 1)
                                        printf("Create master branch (main branch not found)\t\t[All good]\n");
                        }
                }
        }

        return ret;
}

/* 
 * Generate and store cryptographic hash for dependency files
 * Creates SHA-1 hash for verification and tracking
 */
int dep_add_ncheck_hash(const char *raw_file_path, const char *raw_json_path)
{
        char res_convert_f_path[WG_PATH_MAX];
        char res_convert_json_path[WG_PATH_MAX];

        /* Convert paths to consistent format */
        wg_strncpy(res_convert_f_path, raw_file_path, sizeof(res_convert_f_path));
        res_convert_f_path[sizeof(res_convert_f_path) - 1] = '\0';
        dep_sym_convert(res_convert_f_path);
        wg_strncpy(res_convert_json_path, raw_json_path, sizeof(res_convert_json_path));
        res_convert_json_path[sizeof(res_convert_json_path) - 1] = '\0';
        dep_sym_convert(res_convert_json_path);

        /* Skip hash generation for compiler directories */
        if (strfind(res_convert_json_path, "pawno", true) ||
                strfind(res_convert_json_path, "qawno", true))
                goto done;

        unsigned char sha1_hash[20];
        if (crypto_generate_sha1_hash(res_convert_json_path, sha1_hash) == 0) {
                goto done;
        }

        /* Display generated hash */
        pr_color(stdout, FCOLOUR_GREEN,
                "Create hash (CRC32) for '%s': ",
                res_convert_json_path);
        crypto_print_hex(sha1_hash, sizeof(sha1_hash), 0);
        printf("\t\t[All good]\n");

done:
        return 1;
}

/* 
 * Add plugin dependency to SA-MP server.cfg configuration
 * Updates plugins line (c_line) with new dependency
 */
void dep_implementation_samp_conf(depConfig config) {
        if (wg_server_env() != 1)
                return;

        pr_color(stdout, FCOLOUR_GREEN,
                "Create Depends '%s' into '%s'\t\t[All good]\n",
                config.dep_added, config.dep_config);

        FILE* c_file = fopen(config.dep_config, "r");
        if (c_file) {
                int t_exist = 0;
                int tr_exist = 0;
                int tr_ln_has_tx = 0;
                char c_line[WG_PATH_MAX];

                /* Read existing configuration */
                while (fgets(c_line, sizeof(c_line), c_file)) {
                        c_line[strcspn(c_line, "\n")] = 0;
                        if (strstr(c_line, config.dep_added) != NULL) {
                                t_exist = 1; /* Dependency already exists */
                        }
                        if (strstr(c_line, config.dep_target) != NULL) {
                                tr_exist = 1; /* Target c_line exists */
                                if (strstr(c_line, config.dep_added) != NULL) {
                                        tr_ln_has_tx = 1; /* Dependency already on target c_line */
                                }
                        }
                }
                fclose(c_file);

                if (t_exist) {
                        return; /* Dependency already configured */
                }

                if (tr_exist && !tr_ln_has_tx) {
                        /* Add to existing plugins c_line */
                        srand((unsigned int)time(NULL) ^ rand());
                        int rand7 = rand() % 10000000;

                        char temp_path[WG_PATH_MAX];
                        snprintf(temp_path, sizeof(temp_path),
                                ".watchdogs/000%07d_temp", rand7);

                        FILE* temp_file = fopen(temp_path, "w");
                        c_file = fopen(config.dep_config, "r");

                        while (fgets(c_line, sizeof(c_line), c_file)) {
                                char clean_line[WG_PATH_MAX];
                                wg_strcpy(clean_line, c_line);
                                clean_line[strcspn(clean_line, "\n")] = 0;

                                if (strstr(clean_line, config.dep_target) != NULL &&
                                        strstr(clean_line, config.dep_added) == NULL) {
                                        /* Append dependency to existing plugins c_line */
                                        fprintf(temp_file, "%s %s\n", clean_line, config.dep_added);
                                } else {
                                        fputs(c_line, temp_file);
                                }
                        }

                        fclose(c_file);
                        fclose(temp_file);

                        /* Replace original file with updated version */
                        remove(config.dep_config);
                        rename(".watchdogs/depends_tmp", config.dep_config);
                } else if (!tr_exist) {
                        /* Create new plugins c_line */
                        c_file = fopen(config.dep_config, "a");
                        fprintf(c_file, "%s %s\n",
                                        config.dep_target,
                                        config.dep_added);
                        fclose(c_file);
                }

        } else {
                /* Create new configuration file */
                c_file = fopen(config.dep_config, "w");
                fprintf(c_file, "%s %s\n",
                                config.dep_target,
                                config.dep_added);
                fclose(c_file);
        }

        return;
}
/* Macro for adding SA-MP plugins */
#define S_ADD_PLUGIN(x, y, z) \
        dep_implementation_samp_conf((depConfig){x, y, z})

/* 
 * Add plugin dependency to open.mp JSON configuration
 * Updates legacy_plugins array in server configuration
 */
void dep_implementation_omp_conf(const char* config_name, const char* deps_name) {
        if (wg_server_env() != 2)
                return;

        pr_color(stdout, FCOLOUR_GREEN,
                "Create Depends '%s' into '%s'\t\t[All good]\n",
                deps_name, config_name);

        FILE* c_file = fopen(config_name, "r");
        cJSON* s_root = NULL;

        /* Load or create JSON configuration */
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

                size_t file_read = fread(buffer, 1, file_size, c_file);
                if (file_read != file_size) {
                        pr_error(stdout, "Failed to read the entire file!");
                        wg_free(buffer);
                        fclose(c_file);
                        return;
                }

                buffer[file_size] = '\0';
                fclose(c_file);

                s_root = cJSON_Parse(buffer);
                wg_free(buffer);

                if (!s_root) {
                        s_root = cJSON_CreateObject();
                }
        }

        /* Navigate to pawn -> legacy_plugins structure */
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

        /* Ensure legacy_plugins is an array */
        if (!cJSON_IsArray(legacy_plugins)) {
                cJSON_DeleteItemFromObject(pawn, "legacy_plugins");
                legacy_plugins = cJSON_CreateArray();
                cJSON_AddItemToObject(pawn,
                                "legacy_plugins",
                                legacy_plugins);
        }

        /* Check if plugin already exists */
        cJSON* item;
        int p_exist = 0;

        cJSON_ArrayForEach(item, legacy_plugins) {
                if (cJSON_IsString(item) &&
                        !strcmp(item->valuestring, deps_name)) {
                        p_exist = 1;
                        break;
                }
        }

        /* Add plugin if not already present */
        if (!p_exist) {
                cJSON *size_deps_name = cJSON_CreateString(deps_name);
                cJSON_AddItemToArray(legacy_plugins, size_deps_name);
        }

        /* Write updated configuration */
        char* __cJSON_Printed = cJSON_Print(s_root);
        c_file = fopen(config_name, "w");
        if (c_file) {
                fputs(__cJSON_Printed, c_file);
                fclose(c_file);
        }

        /* Cleanup */
        cJSON_Delete(s_root);
        wg_free(__cJSON_Printed);

        return;
}
/* Macro for adding open.mp plugins */
#define M_ADD_PLUGIN(x, y) dep_implementation_omp_conf(x, y)

/* 
 * Add include directive to gamemode file
 * Inserts #include statement in appropriate location
 */
void dep_add_include(const char *modes,
                char *dep_name,
                char *dep_after) {
        if (path_exists(modes) == 0) {
                pr_color(stdout, FCOLOUR_GREEN,
                        "~ can't found %s.. skipping adding include name\t\t[Fail]\n", modes);
                return;
        }
        pr_color(stdout, FCOLOUR_GREEN,
                "Create include '%s' into '%s' after '%s'\t\t[All good]\n",
                dep_name, modes, dep_after);

        FILE *m_file = fopen(modes, "r");
        if (m_file == NULL) {
                return;
        }

        /* Read entire file */
        fseek(m_file, 0, SEEK_END);
        long fle_size = ftell(m_file);
        fseek(m_file, 0, SEEK_SET);

        char *ct_modes = wg_malloc(fle_size + 1);
        size_t file_read = fread(ct_modes, 1, fle_size, m_file);
        if (file_read != fle_size) {
                wg_free(ct_modes);
                fclose(m_file);
                return;
        }

        ct_modes[fle_size] = '\0';
        fclose(m_file);

        /* Check if include already exists */
        if (strstr(ct_modes, dep_name) != NULL &&
                strstr(ct_modes, dep_after) != NULL) {
                wg_free(ct_modes);
                return;
        }

        /* Find position to insert include */
        char *e_dep_after_pos = strstr(ct_modes, dep_after);

        FILE *n_file = fopen(modes, "w");
        if (n_file == NULL) {
                wg_free(ct_modes);
                return;
        }

        if (e_dep_after_pos != NULL) {
                /* Insert after specified include */
                fwrite(ct_modes,
                           1,
                           e_dep_after_pos - ct_modes + strlen(dep_after),
                           n_file);
                fprintf(n_file, "\n%s", dep_name);
                fputs(e_dep_after_pos + strlen(dep_after), n_file);
        } else {
                /* Alternative insertion strategy */
                char *includeToAddPos = strstr(ct_modes, dep_name);

                if (includeToAddPos != NULL) {
                        fwrite(ct_modes,
                                   1,
                                   includeToAddPos - ct_modes + strlen(dep_name),
                                   n_file);
                        fprintf(n_file, "\n%s", dep_after);
                        fputs(includeToAddPos + strlen(dep_name), n_file);
                } else {
                        /* Prepend at beginning */
                        fprintf(n_file, "%s\n%s\n%s", dep_after, dep_name, ct_modes);
                }
        }

        fclose(n_file);
        wg_free(ct_modes);

        return;
}
/* Macro for adding include directives */
#define DEP_ADD_INCLUDES(x, y, z) dep_add_include(x, y, z)

/* 
 * Process and add include directive based on dependency
 * Reads TOML configuration to determine gamemode file
 */
static void dep_pr_include_directive(const char *deps_include)
{
        char error_buffer[WG_PATH_MAX];
        toml_table_t *wg_toml_config;
        char depends_name[WG_PATH_MAX];
        char idirective[WG_MAX_PATH];

        /* Extract filename from path */
        const char *dep_n = dep_get_fname(deps_include);
        wg_snprintf(depends_name, sizeof(depends_name), "%s", dep_n);

        const char *direct_bnames = dep_get_bname(depends_name);

        /* Parse TOML configuration */
        FILE *proc_f = fopen("watchdogs.toml", "r");
        wg_toml_config = toml_parse_file(proc_f, error_buffer, sizeof(error_buffer));
        if (proc_f) fclose(proc_f);

        if (!wg_toml_config) {
                pr_error(stdout, "parsing TOML: %s", error_buffer);
                return;
        }

        /* Extract gamemode input file from TOML */
        toml_table_t *wg_compiler = toml_table_in(wg_toml_config, "compiler");
        if (wg_compiler) {
                toml_datum_t toml_gm_i = toml_string_in(wg_compiler, "input");
                if (toml_gm_i.ok) {
                        wgconfig.wg_toml_gm_input = strdup(toml_gm_i.u.s);
                        wg_free(toml_gm_i.u.s);
                }
        }
        toml_free(wg_toml_config);

        /* Construct include directive */
        wg_snprintf(idirective, sizeof(idirective),
                                "#include <%s>",
                                direct_bnames);

        /* Add include based on server environment */
        if (wg_server_env() == 1) {
                /* SA-MP: include after a_samp */
                DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
                                                 idirective,
                                                 "#include <a_samp>");
        } else if (wg_server_env() == 2) {
                /* open.mp: include after open.mp */
                DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
                                                 idirective,
                                                 "#include <open.mp>");
        } else {
                /* Default: include after a_samp */
                DEP_ADD_INCLUDES(wgconfig.wg_toml_gm_input,
                                                 idirective,
                                                 "#include <a_samp>");
        }
}

/* 
 * Search for and move files matching pattern
 * Handles file relocation and configuration updates
 */
void dump_file_type(const char *path,
                char *pattern,
                char *exclude, char *cwd,
                char *target_dir, int root)
{
        char json_item[WG_PATH_MAX];

        wg_sef_fdir_reset();

        /* Search for files matching pattern */
        int found = wg_sef_fdir(path, pattern, exclude);

        if (found) {
                for (int i = 0; i < wgconfig.wg_sef_count; ++i) {
                        const char *deps_names  = dep_get_fname(wgconfig.wg_sef_found_list[i]),
                                *deps_bnames = dep_get_bname(wgconfig.wg_sef_found_list[i]);

                        /* Move to target directory if specified */
                        if (target_dir[0] != '\0') {
                                int windows_userin32 = 0;
#ifdef WG_WINDOWS
                                windows_userin32 = 1;
#endif
                                if (windows_userin32) {
                                        wg_snprintf(d_command, sizeof(d_command),
                                                "move "
                                                "/Y "
                                                "\"%s\" \"%s\\%s\\\"",
                                                wgconfig.wg_sef_found_list[i], cwd, target_dir);
                                } else
                                        wg_snprintf(d_command, sizeof(d_command),
                                                "mv "
                                                "-f "
                                                "\"%s\" \"%s/%s/\"",
                                                wgconfig.wg_sef_found_list[i], cwd, target_dir);

                                struct timespec start = {0}, end = { 0 };
                                double calculate_moving_dur;

                                /* Time the move operation */
                                clock_gettime(CLOCK_MONOTONIC, &start);
                                wg_run_command(d_command);
                                clock_gettime(CLOCK_MONOTONIC, &end);

                                calculate_moving_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                                pr_color(stdout, FCOLOUR_CYAN, " [MOVING] Plugins %s -> %s - %s [Finished at %.3fs]\n",
                                                 wgconfig.wg_sef_found_list[i], cwd, target_dir, calculate_moving_dur);
                        } else {
                                /* Move to current directory */
                                int windows_userin32 = 0;
#ifdef WG_WINDOWS
                                windows_userin32 = 1;
#endif
                                if (windows_userin32) {
                                        wg_snprintf(d_command, sizeof(d_command),
                                                "move "
                                                "/Y "
                                                "\"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], cwd);
                                } else
                                        wg_snprintf(d_command, sizeof(d_command),
                                                "mv "
                                                "-f "
                                                "\"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], cwd);

                                struct timespec start = {0}, end = { 0 };
                                double calculate_moving_dur;

                                clock_gettime(CLOCK_MONOTONIC, &start);
                                wg_run_command(d_command);
                                clock_gettime(CLOCK_MONOTONIC, &end);

                                calculate_moving_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                                pr_color(stdout, FCOLOUR_CYAN, " [MOVING] Plugins %s -> %s [Finished at %.3fs]\n",
                                                 wgconfig.wg_sef_found_list[i], cwd, calculate_moving_dur);

                                wg_snprintf(json_item, sizeof(json_item), "%s", deps_names);
                                dep_add_ncheck_hash(json_item, json_item);

                                if (root == 1)
                                        goto done;

                                /* Update configuration based on server environment */
                                if (wg_server_env() == 1 &&
                                          strfind(wgconfig.wg_toml_config, ".cfg", true))
samp_label:
                                          S_ADD_PLUGIN(wgconfig.wg_toml_config,
                                                "plugins", deps_bnames);
                                else if (wg_server_env() == 2 &&
                                        strfind(wgconfig.wg_toml_config, ".json", true))
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

/* 
 * Utility function to add items between cJSON arrays
 * Copies string items from one array to another
 */
void dep_cjson_additem(cJSON *p1, int p2, cJSON *p3)
{
        cJSON *_e_item = cJSON_GetArrayItem(p1, p2);
        if (cJSON_IsString(_e_item))
                cJSON_AddItemToArray(p3,
                        cJSON_CreateString(_e_item->valuestring));
}

/* 
 * Organize dependency files into appropriate directories
 * Moves plugins, includes, and components to their proper locations
 */
void dep_move_files(const char *dep_dir)
{
        char *deps_include_path = NULL;
        char include_path[WG_MAX_PATH];
        int i, deps_include_search = 0;
        char plug_path[WG_PATH_MAX],
                 comp_path[WG_PATH_MAX],
                 deps_include_full_path[WG_PATH_MAX],
                 depends_ipath[WG_PATH_MAX * 2],
                 deps_parent_dir[WG_PATH_MAX],
                 dest[WG_MAX_PATH * 2];
        
        /* Construct platform-specific paths */
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

        /* Determine include path based on server environment */
        if (wg_server_env() == 1) {
#ifdef WG_WINDOWS
                deps_include_path = "pawno\\include";
                wg_snprintf(deps_include_full_path,
                        sizeof(deps_include_full_path), "%s\\pawno\\include", dep_dir);
#else
                deps_include_path = "pawno/include";
                wg_snprintf(deps_include_full_path,
                        sizeof(deps_include_full_path), "%s/pawno/include", dep_dir);
#endif
        } else if (wg_server_env() == 2) {
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

        /* Normalize path separators */
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

        /* Move plugin files */
#ifndef WG_WINDOWS
        dump_file_type(plug_path, "*.dll", NULL, cwd, "/plugins", 0);
        dump_file_type(plug_path, "*.so", NULL, cwd, "/plugins", 0);
#else
        dump_file_type(plug_path, "*.dll", NULL, cwd, "\\plugins", 0);
        dump_file_type(plug_path, "*.so", NULL, cwd, "\\plugins", 0);
#endif
        
        /* Move root-level plugin files */
        dump_file_type(dep_dir, "*.dll", "plugins", cwd, "", 1);
        dump_file_type(dep_dir, "*.so", "plugins", cwd, "", 1);

        /* Handle open.mp specific components */
        if (wg_server_env() == 2) {
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

        /* Normalize include path */
        while ((path_pos = strstr(include_path, "include\\")) != NULL)
                memmove(path_pos, path_pos + strlen("include/"),
                        strlen(path_pos + strlen("include\\")) + 1);
        while ((path_pos = strstr(include_path, "include/")) != NULL)
                memmove(path_pos, path_pos + strlen("include/"),
                        strlen(path_pos + strlen("include/")) + 1);

        wg_sef_fdir_reset();

        /* Move include files from standard include directory */
        deps_include_search = wg_sef_fdir(deps_include_full_path, "*.inc", NULL);
        if (deps_include_search) {
                for (i = 0; i < wgconfig.wg_sef_count; ++i) {
                        const char *fi_depends_name;
                        fi_depends_name = dep_get_fname(wgconfig.wg_sef_found_list[i]);

                        int windows_userin32 = 0;
#ifdef WG_WINDOWS
                        windows_userin32 = 1;
#endif
                        if (windows_userin32) {
                                wg_snprintf(d_command, sizeof(d_command),
                                        "move "
                                        "/Y "
                                        "\"%s\" \"%s\\%s\\\"",
                                        wgconfig.wg_sef_found_list[i], cwd, deps_include_path);
                        } else
                                wg_snprintf(d_command, sizeof(d_command),
                                        "mv "
                                        "-f "
                                        "\"%s\" \"%s/%s/\"",
                                        wgconfig.wg_sef_found_list[i], cwd, deps_include_path);

                        struct timespec start = {0}, end = { 0 };
                        double calculate_moving_dur;

                        clock_gettime(CLOCK_MONOTONIC, &start);
                        wg_run_command(d_command);
                        clock_gettime(CLOCK_MONOTONIC, &end);

                        calculate_moving_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                        pr_color(stdout, FCOLOUR_CYAN, " [MOVING] Include %s -> %s - %s [Finished at %.3fs]\n",
                                wgconfig.wg_sef_found_list[i], cwd, deps_include_path, calculate_moving_dur);

                        dep_add_ncheck_hash(fi_depends_name, fi_depends_name);
                        dep_pr_include_directive(fi_depends_name);
                }
        }

        /* Stack-based directory traversal for finding nested include files */
        int moving_stack_index = -1;
        int moving_stack_size = WG_MAX_PATH;
        char **moving_dir_stack = wg_malloc(moving_stack_size * sizeof(char*));

        for (int i = 0; i < moving_stack_size; i++) {
                moving_dir_stack[i] = wg_malloc(WG_PATH_MAX);
        }

        /* Start with dependency directory */
        moving_stack_index++;
        wg_snprintf(moving_dir_stack[moving_stack_index], WG_PATH_MAX, "%s", dep_dir);

        /* Depth-first search for include files */
        while (moving_stack_index >= 0) {
                char root_dir[WG_PATH_MAX];
                wg_strlcpy(root_dir,
                        moving_dir_stack \
                                [moving_stack_index],
                        sizeof(root_dir));
                moving_stack_index--;

                DIR *dir = opendir(root_dir);
                if (!dir)
                        continue;

                struct dirent *item;
                struct stat st;

                while ((item = readdir(dir)) != NULL) {
                        if (wg_is_special_dir(item->d_name))
                                continue;

                        /* Construct full path */
                        wg_strlcpy(depends_ipath, root_dir, sizeof(depends_ipath));
                        strlcat(depends_ipath, __PATH_STR_SEP_LINUX, sizeof(depends_ipath));
                        strlcat(depends_ipath, item->d_name, sizeof(depends_ipath));

                        if (stat(depends_ipath, &st) != 0)
                                continue;

                        if (S_ISDIR(st.st_mode)) {
                                /* Skip compiler directories */
                                if (strcmp(item->d_name, "pawno") == 0 ||
                                        strcmp(item->d_name, "qawno") == 0 ||
                                        strcmp(item->d_name, "include") == 0) {
                                        continue;
                                }
                                /* Push directory onto stack for later processing */
                                if (moving_stack_index < moving_stack_size - 1) {
                                        moving_stack_index++;
                                        wg_strlcpy(moving_dir_stack[moving_stack_index],
                                                depends_ipath, WG_MAX_PATH);
                                }
                                continue;
                        }

                        /* Process .inc files */
                        const char *extension = strrchr(item->d_name, '.');
                        if (!extension || strcmp(extension, ".inc"))
                                continue;

                        /* Extract parent directory name */
                        wg_strlcpy(deps_parent_dir, root_dir, sizeof(deps_parent_dir));
                        char *depends_filename = NULL;
#ifdef WG_WIDOWS
                        depends_filename = strrchr(deps_parent_dir, __PATH_CHR_SEP_WIN32);
#else
                        depends_filename = strrchr(deps_parent_dir, __PATH_CHR_SEP_LINUX);
#endif
                        if (!depends_filename)
                                continue;

                        ++depends_filename;

                        /* Construct destination path */
                        wg_strlcpy(dest, include_path, sizeof(dest));
#ifdef WG_WINDOWS
                        strlcat(dest, __PATH_STR_SEP_WIN32, sizeof(dest));
#else
                        strlcat(dest, __PATH_STR_SEP_LINUX, sizeof(dest));
#endif
                        strlcat(dest, depends_filename, sizeof(dest));

                        /* Move or copy directory containing include files */
                        if (rename(deps_parent_dir, dest)) {
                                int windows_userin32 = 0;
#ifdef WG_WINDOWS
                                windows_userin32 = 1;
#endif
                                if (windows_userin32) {
                                        /* Windows: copy directory tree and remove original */
                                        const char *win_parts[] = {
                                                "xcopy ",
                                                deps_parent_dir,
                                                " ",
                                                dest,
                                                " /E /I /H /Y >nul 2>&1 && rmdir /S /Q ",
                                                deps_parent_dir,
                                                " >nul 2>&1"
                                        };
                                        wg_strlcpy(d_command, "", sizeof(d_command));
                                        for (int j = 0; j < sizeof(win_parts) / sizeof(win_parts[0]); j++) {
                                                strlcat(d_command, win_parts[j], sizeof(d_command));
                                        }
                                } else {
                                        /* Unix: recursive copy and remove */
                                        const char *unix_parts[] = {
                                                "cp -r ",
                                                deps_parent_dir,
                                                " ",
                                                dest,
                                                " && rm -rf ",
                                                deps_parent_dir
                                        };
                                        wg_strlcpy(d_command, "", sizeof(d_command));
                                        for (int j = 0; j < sizeof(unix_parts) / sizeof(unix_parts[0]); j++) {
                                                strlcat(d_command, unix_parts[j], sizeof(d_command));
                                        }
                                }

                                struct timespec start = {0}, end = { 0 };
                                double move_duration;

                                clock_gettime(CLOCK_MONOTONIC, &start);
                                wg_run_command(d_command);
                                clock_gettime(CLOCK_MONOTONIC, &end);

                                move_duration = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                                pr_color(stdout, FCOLOUR_CYAN, " [MOVING] Include %s -> %s [Finished at %.3fs]\n",
                                                 deps_parent_dir, dest, move_duration);
                        }

                        dep_add_ncheck_hash(dest, dest);

                        printf("\n");
                }

                closedir(dir);
        }

        /* Clean up directory stack */
        for (int i = 0; i < moving_stack_size; i++) {
                wg_free(moving_dir_stack[i]);
        }
        wg_free(moving_dir_stack);

        /* Remove extracted dependency directory */
        int windows_userin32 = 0;
#ifdef WG_WINDOWS
        windows_userin32 = 1;
#endif
        if (windows_userin32)
                wg_snprintf(d_command, sizeof(d_command),
                        "rmdir "
                        "/s "
                        "/q "
                        "\"%s\"",
                        dep_dir);
        else
                wg_snprintf(d_command, sizeof(d_command),
                        "rm "
                        "-rf "
                        "%s",
                        dep_dir);
        wg_run_command(d_command);

        return;
}

/* 
 * Prepare and organize dependencies after download
 * Creates necessary directories and initiates file organization
 */
void wg_apply_depends(const char *depends_name)
{
        char _depends[WG_PATH_MAX];
        char dep_dir[WG_PATH_MAX];

        wg_snprintf(_depends, sizeof(_depends), "%s", depends_name);

        /* Remove archive extension to get directory name */
        if (strstr(_depends, ".tar.gz")) {
                char *ext = strstr(_depends, ".tar.gz");
                if (ext)
                        *ext = '\0';
        } if (strstr(_depends, ".zip")) {
                char *ext = strstr(_depends, ".zip");
                if (ext)
                        *ext = '\0';
        }

        wg_snprintf(dep_dir, sizeof(dep_dir), "%s", _depends);

        /* Create necessary directories based on server environment */
        if (wg_server_env() == 1) {
                if (dir_exists("pawno/include") == 0) {
                        wg_mkdir("pawno/include");
                }
                if (dir_exists("plugins") == 0) {
                        MKDIR("plugins");
                }
        } else if (wg_server_env() == 2) {
                if (dir_exists("qawno/include") == 0) {
                        wg_mkdir("qawno/include");
                }
                if (dir_exists("plugins") == 0) {
                        MKDIR("plugins");
                }
                if (dir_exists("components") == 0) {
                        MKDIR("components");
                }
        }

        /* Organize dependency files */
        dep_move_files(dep_dir);
}

/* 
 * Main dependency installation function
 * Parses dependency strings, downloads archives, and applies dependencies
 */
void wg_install_depends(const char *depends_string)
{
        char buffer[1024];
        const char *depends[MAX_DEPENDS];
        struct dep_repo_info repo;
        char dep_url[1024];
        char dep_name[WG_PATH_MAX];
        const char *size_last_slash;
        char *token;
        int deps_count = 0;
        int i;

        memset(depends, 0, sizeof(depends));
        wgconfig.wg_idepends = 0;

        /* Validate input */
        if (!depends_string || !*depends_string) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("no valid dependencies to install!\t\t[Fail]\n");
                goto done;
        }

        if (strlen(depends_string) >= sizeof(buffer)) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("depends_string too long!\t\t[Fail]\n");
                goto done;
        }

        /* Tokenize dependency string */
        wg_snprintf(buffer, sizeof(buffer), "%s", depends_string);

        token = strtok(buffer, " ");
        while (token && deps_count < MAX_DEPENDS) {
                depends[deps_count++] = token;
                token = strtok(NULL, " ");
        }

        if (!deps_count) {
                pr_color(stdout, FCOLOUR_RED, "");
                printf("no valid depends to install!\t\t[Fail]\n");
                goto done;
        }

        /* Process each dependency */
        for (i = 0; i < deps_count; i++) {
                /* Parse repository information */
                if (!dep_parse_repo(depends[i], &repo)) {
                        pr_color(stdout, FCOLOUR_RED, "");
                        printf("invalid repo format: %s\t\t[Fail]\n", depends[i]);
                        continue;
                }
                
                /* Handle GitHub repositories */
                if (!strcmp(repo.host, "github")) {
                        if (!dep_handle_repo(&repo, dep_url, sizeof(dep_url))) {
                                pr_color(stdout, FCOLOUR_RED, "");
                                printf("repo not found: %s\t\t[Fail]\n", depends[i]);
                                continue;
                        }
                } else {
                        /* Handle custom repositories */
                        dep_build_repo_url(&repo, 0, dep_url, sizeof(dep_url));
                        if (!dep_url_checking(dep_url, wgconfig.wg_toml_github_tokens)) {
                                pr_color(stdout, FCOLOUR_RED, "");
                                printf("repo not found: %s\t\t[Fail]\n", depends[i]);
                                continue;
                        }
                }

                /* Extract filename from URL */
                size_last_slash = strrchr(dep_url, __PATH_CHR_SEP_LINUX);
                if (size_last_slash && *(size_last_slash + 1)) {
                        wg_snprintf(dep_name, sizeof(dep_name), "%s", size_last_slash + 1);
                        /* Ensure archive extension */
                        if (!strend(dep_name, ".tar.gz", true) &&
                                !strend(dep_name, ".tar", true) &&
                                !strend(dep_name, ".zip", true))
                                wg_snprintf(dep_name + strlen(dep_name),
                                                        sizeof(dep_name) - strlen(dep_name),
                                                        ".zip");
                } else {
                        /* Default naming scheme */
                        wg_snprintf(dep_name, sizeof(dep_name), "%s.tar.gz", repo.repo);
                }

                if (!*dep_name) {
                        pr_color(stdout, FCOLOUR_RED, "");
                        printf("invalid repo name: %s\t\t[Fail]\n", dep_url);
                        continue;
                }

                wgconfig.wg_idepends = 1;

                struct timespec start = {0}, end = { 0 };
                double depends_dur;

                /* Time the dependency installation process */
                clock_gettime(CLOCK_MONOTONIC, &start);
                wg_download_file(dep_url, dep_name);
                wg_apply_depends(dep_name);
                clock_gettime(CLOCK_MONOTONIC, &end);

                depends_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

                pr_color(stdout, FCOLOUR_CYAN, " <D> Finished at %.3fs (%.0f ms)\n",
                        depends_dur, depends_dur * 1000.0);
        }

done:
        return;
}