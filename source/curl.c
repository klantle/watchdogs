#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "extra.h"
#include "crypto.h"
#include "archive.h"
#include "depends.h"
#include "compiler.h"
#include "utils.h"
#include "units.h"
#include "curl.h"

/* Array of online platform names and their URL patterns for account tracking */
const char* __FORUM[MAX_NUM_SITES][2] = {
        {"github", "https://github.com/%s"},
        {"gitlab", "https://gitlab.com/%s"},
        {"gitea", "https://gitea.io/%s"},
        {"sourceforge", "https://sourceforge.net/u/%s"},
        {"bitbucket", "https://bitbucket.org/%s"},
        {"stackoverflow", "https://stackoverflow.com/users/%s"},
        {"devto", "https://dev.to/%s"},
        {"hackerrank", "https://www.hackerrank.com/%s"},
        {"leetcode", "https://leetcode.com/%s"},
        {"codewars", "https://www.codewars.com/users/%s"},
        {"codepen", "https://codepen.io/%s"},
        {"jsfiddle", "https://jsfiddle.net/user/%s"},
        {"replit", "https://replit.com/@%s"},
        {"medium", "https://medium.com/@%s"},
        {"substack", "https://%s.substack.com"},
        {"wordpress", "https://%s.wordpress.com"},
        {"blogger", "https://%s.blogspot.com"},
        {"tumblr", "https://%s.tumblr.com"},
        {"mastodon", "https://mastodon.social/@%s"},
        {"bluesky", "https://bsky.app/profile/%s"},
        {"threads", "https://www.threads.net/@%s"},
        {"slideshare", "https://www.slideshare.net/%s"},
        {"speakerdeck", "https://speakerdeck.com/%s"},
        {"reddit", "https://www.reddit.com/user/%s"},
        {"discord", "https://discord.com/users/%s"},
        {"keybase", "https://keybase.io/%s"},
        {"gravatar", "https://gravatar.com/%s"},
        {"letterboxd", "https://letterboxd.com/%s"},
        {"trello", "https://trello.com/%s"},
        {"linktree", "https://linktr.ee/%s"}
};

/* Static buffer for storing compiler source directory path */
static char pawncc_dir_src[WG_PATH_MAX];

/* 
 * Locate and configure CA certificate bundle for CURL SSL verification 
 * Searches for cacert.pem in platform-specific locations
 */
void verify_cacert_pem(CURL *curl) {
        int is_win32 = 0;
#ifdef WG_WINDOWS
        is_win32 = 1;
#endif

        int is_android = 0;
#ifdef WG_ANDROID
        is_android = 2;
#endif

        int is_linux = 0;
#ifdef WG_LINUX
        is_linux = 3;
#endif

        static int cacert_notice = 0; /* Track if warning already shown */

        /* Windows-specific certificate locations */
        if (is_win32) {
                if (access("cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                else if (access("C:/libwatchdogs/cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
                else {
                        if (cacert_notice != 1) {
                                cacert_notice = 1;
                                pr_color(stdout,
                                                 FCOLOUR_YELLOW,
                                                 "curl: i can't found cacert.pem. SSL verification may fail.\n");
                        }
                }
        }
        /* Android-specific certificate locations */
        else if (is_android) {
                const char *env_home = getenv("HOME");
                if (env_home == NULL || env_home[0] == '\0') {
                        env_home = "/data/data/com.termux/files/home";
                }
                char size_env_home[WG_PATH_MAX + 6];

                wg_snprintf(size_env_home, sizeof(size_env_home), "%s/cacert.pem", env_home);

                if (access("cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                else if (access(size_env_home, F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, size_env_home);
                else {
                        if (cacert_notice != 1) {
                                cacert_notice = 1;
                                pr_color(stdout,
                                                 FCOLOUR_YELLOW,
                                                 "curl: i can't found cacert.pem. SSL verification may fail.\n");
                        }
                }
        }
        /* Linux-specific certificate locations */
        else if (is_linux) {
                if (access("cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                else if (access("/etc/ssl/certs/cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/cacert.pem");
                else {
                        if (cacert_notice != 1) {
                                cacert_notice = 1;
                                pr_color(stdout,
                                                 FCOLOUR_GREEN,
                                                 "curl: i can't found cacert.pem. SSL verification may fail.\n");
                        }
                }
        }
}

/* 
 * CURL progress callback function for displaying download progress
 * Shows percentage and spinner animation during file transfer
 */
static int progress_callback(void *ptr, curl_off_t dltotal,
                                                         curl_off_t dlnow, curl_off_t ultotal,
                                                         curl_off_t ulnow)
{
        static int last_percent = -1; /* Track last displayed percentage */
        static int dot_index = 0;         /* Index for spinner animation */
        static const char term_spinner[] = "-\\|/"; /* Spinner characters */

        int percent;

        /* Only show progress if total download size is known */
        if (dltotal > 0) {
                percent = (int)((dlnow * 100) / dltotal);

                /* Update display only when percentage changes */
                if (percent != last_percent) {
                        char spin_char = term_spinner[dot_index % 4];
                        dot_index++;

                        if (percent < 100)
                                printf("\rDownloading... %3d%% [%c]", percent, spin_char);
                        else
                                printf("\rDownloading... %3d%% Done!\n", percent);

                        fflush(stdout);
                        last_percent = percent;
                }
        }

        return 0;
}

/* 
 * Write callback for CURL - handles incoming data during downloads
 * Dynamically allocates memory as data arrives
 */
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
        struct buf *b = userdata;
        
        /* Sanity check: ensure buffer pointer is properly aligned */
        if (b->data && ((uintptr_t)b->data & 0x7)) {
                pr_error(stdout, " Buffer pointer corrupted: %p\n", b->data);
                return 0;
        }
        
        size_t total = size * nmemb;
        
        /* Safety check: prevent excessively large chunks */
        if (total > 0xFFFFFFF) return 0;
        
        size_t required = b->len + total + 1;
        
        /* Reallocate buffer if more space needed */
        if (required > b->allocated) {
                /* Grow by 50% or to required size, whichever is larger */
                size_t new_alloc = (b->allocated * 3) >> 1;
                new_alloc = (required > new_alloc) ? required : new_alloc;
                /* Cap maximum allocation at 64MB */
                new_alloc = (new_alloc < 0x4000000) ? new_alloc : 0x4000000;
                
                char *p = wg_realloc(b->data, new_alloc);
                if (!p) {
                        pr_error(stdout, " Realloc failed for %zu bytes\n", new_alloc);
                        return 0;
                }
                
                b->data = p;
                b->allocated = new_alloc;
        }
        
        /* Copy incoming data to buffer */
        memcpy(b->data + b->len, ptr, total);
        b->len += total;
        b->data[b->len] = 0; /* Null-terminate */
        
        return total;
}

/* 
 * Write callback specifically for website content fetching
 * Similar to write_callback but with different growth strategy
 */
size_t write_callback_sites(void *data, size_t size, size_t nmemb, void *userp) 
{
        size_t total_size = size * nmemb;
        struct string *str = (struct string *)userp;
        
        /* Safety check: prevent oversized chunks */
        if (total_size > 0x8000000) {
#if defined(_DBG_PRINT)
                pr_error(stdout, "Oversized chunk: 0x%zX\n", total_size);
#endif
                return 0;
        }
        
        /* Calculate required capacity and reallocate if needed */
        size_t new_capacity = str->len + total_size + 1;
        if (new_capacity > str->allocated) {
                /* Grow by 50% or to required size */
                size_t growth = str->allocated + (str->allocated >> 1);
                new_capacity = (new_capacity > growth) ? new_capacity : growth;
                /* Cap at 32MB */
                new_capacity = (new_capacity < 0x2000000) ? new_capacity : 0x2000000;
                
                char *new_ptr = wg_realloc(str->ptr, new_capacity);
                if (!new_ptr) {
#if defined(_DBG_PRINT)
                        pr_error(stdout, "Allocation failed for 0x%zX bytes\n", new_capacity);
#endif
                        return 0;
                }
                str->ptr = new_ptr;
                str->allocated = new_capacity;
        }
        
        /* Append data to buffer */
        memcpy(str->ptr + str->len, data, total_size);
        str->len += total_size;
        str->ptr[str->len] = '\0';
        
        return total_size;
}

/* 
 * Generic memory write callback for CURL
 * Stores downloaded content in dynamically allocated memory
 */
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
        struct memory_struct *mem = userp;
        size_t realsize = size * nmemb;
        
        /* Input validation */
        if (!contents || !mem || realsize > 0x10000000)
                return 0;
        
        size_t required = mem->size + realsize + 1;
        if (required > mem->allocated) {
                /* Double allocation or start with 4KB */
                size_t new_alloc = mem->allocated ? (mem->allocated * 2) : 0x1000;
                new_alloc = (required > new_alloc) ? required : new_alloc;
                /* Cap at 128MB */
                new_alloc = (new_alloc < 0x8000000) ? new_alloc : 0x8000000;
                
                char *ptr = wg_realloc(mem->memory, new_alloc);
                if (!ptr) {
#if defined(_DBG_PRINT)
                        pr_error(stdout, "Memory exhausted at 0x%zX bytes\n", new_alloc);
#endif
                        return 0;
                }
                mem->memory = ptr;
                mem->allocated = new_alloc;
        }
                
        /* Copy data to memory buffer */
        memcpy(&mem->memory[mem->size], contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = '\0';
        
        return realsize;
}

/* 
 * Generate username variations for account tracking
 * Creates common username mutations to find accounts across platforms
 */
void account_tracker_discrepancy(const char *base,
                                        char discrepancy[][100],
                                        int *cnt) {
        int k;
        int dis_base_len = strlen(base);

        /* Add original username */
        wg_strcpy(discrepancy[(*cnt)++], base);

        /* Create variations with duplicate characters (e.g., "heello" from "hello") */
        for (k = 0; k < dis_base_len; k++) {
                char dis_size_temp[100];
                strncpy(dis_size_temp, base, k);
                dis_size_temp[k] = base[k];
                dis_size_temp[k+1] = base[k]; /* Duplicate character */
                wg_strcpy(dis_size_temp + k + 2, base + k + 1);
                wg_strcpy(discrepancy[(*cnt)++], dis_size_temp);
        }

        /* Add trailing character repetitions (e.g., "hello111") */
        for (k = 2; k <= 5; k++) {
                char dis_size_temp[100];
                wg_snprintf(dis_size_temp, sizeof(dis_size_temp), "%s", base);
                for (int j = 0; j < k; j++) {
                        dis_size_temp[strlen(dis_size_temp)] = base[dis_base_len - 1];
                }
                wg_strcpy(discrepancy[(*cnt)++], dis_size_temp);
        }

        /* Add common suffixes */
        const char *suffixes[] = {"1", "123", "007", "_", ".", "_dev"};
        for (int k = 0; k < sizeof(suffixes) / sizeof(suffixes[0]); k++) {
                char *temp = strdup(base);
                wg_strcpy(discrepancy[(*cnt)++], strcat(temp, suffixes[k]));
                wg_free(temp);
        }
}

/* 
 * Check if a username exists on multiple online platforms
 * Uses CURL to probe each platform's URL pattern
 */
void account_tracking_username(CURL *curl, const char *username) {
        CURLcode res;
        struct string response;
        response.ptr = wg_malloc(1);
        response.len = 0;

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");

        /* Test username against each platform in the list */
        for (int i = 0; i < MAX_NUM_SITES; i++) {
                char url[200];
                wg_snprintf(url, sizeof(url), __FORUM[i][1], username);

                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_sites);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

                verify_cacert_pem(curl);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                        printf("[%s] %s -> ERROR %s\n",
                                        __FORUM[i][0],
                                        url, curl_easy_strerror(res));
                } else {
                        long status_code;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
                        if (status_code == WG_CURL_RESPONSE_OK) {
                                printf("[%s] %s -> FOUND\n", __FORUM[i][0], url);
                                fflush(stdout);
                        } else {
                                printf("[%s] %s -> NOT FOUND (%ld)\n", __FORUM[i][0], url, status_code);
                                fflush(stdout);
                        }
                }
                response.len = 0;
        }
}

/* 
 * Search for compiler tools in the specified directory
 * Looks for various compiler executables and libraries
 */
static void find_compiler_tools(int compiler_type,
                                int *found_pawncc_exe, int *found_pawncc,
                                int *found_pawndisasm_exe, int *found_pawndisasm,
                                int *found_pawnc_dll, int *found_PAWNC_DLL)
{
        const char *ignore_dir = NULL;
        char size_pf[WG_PATH_MAX + 56];

        wg_snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);

        /* Search for each compiler tool */
        *found_pawncc_exe = wg_sef_fdir(size_pf, "pawncc.exe", ignore_dir);
        *found_pawncc = wg_sef_fdir(size_pf, "pawncc", ignore_dir);
        *found_pawndisasm_exe = wg_sef_fdir(size_pf, "pawndisasm.exe", ignore_dir);
        *found_pawndisasm = wg_sef_fdir(size_pf, "pawndisasm", ignore_dir);
        *found_pawnc_dll = wg_sef_fdir(size_pf, "pawnc.dll", ignore_dir);
        *found_PAWNC_DLL = wg_sef_fdir(size_pf, "PAWNC.dll", ignore_dir);
}

/* 
 * Determine the appropriate compiler directory (pawno or qawno)
 * Creates directory if it doesn't exist
 */
static const char *get_compiler_directory(void)
{
        const char *dir_path = NULL;

        /* Check existing directories */
        if (path_exists("pawno")) {
                dir_path = "pawno";
        } else if (path_exists("qawno")) {
                dir_path = "qawno";
        } else {
                /* Create pawno directory if neither exists */
                if (MKDIR("pawno") == 0)
                        dir_path = "pawno";
        }

        return dir_path;
}

/* 
 * Copy a compiler tool from source to destination directory
 */
static void copy_compiler_tool(const char *src_path, const char *tool_name,
                                                           const char *dest_dir)
{
        char dest_path[WG_PATH_MAX];

        wg_snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
        wg_sef_wmv(src_path, dest_path);
}

#ifndef WG_WINDOWS
/* 
 * Update shell configuration to add library path to LD_LIBRARY_PATH
 * Works with bash and zsh shells
 */
static void update_library_environment(const char *lib_path)
{
        const char *home_dir = getenv("HOME");
        if (!home_dir) {
                fprintf(stderr, "Error: HOME environment variable not set\n");
                return;
        }

        const char *shell_rc = NULL;
        char shell_path[256];

        /* Detect shell type from SHELL environment variable */
        char *shell = getenv("SHELL");
        if (shell) {
                if (strfind(shell, "zsh", false)) {
                        shell_rc = ".zshrc";
                } else if (strfind(shell, "bash", false)) {
                        shell_rc = ".bashrc";
                }
        }

        /* Fallback to checking for existing config files */
        if (!shell_rc) {
                wg_snprintf(shell_path, sizeof(shell_path), "%s/.zshrc", home_dir);
                if (access(shell_path, F_OK) == 0) {
                        shell_rc = ".zshrc";
                } else {
                        shell_rc = ".bashrc";
                }
        }

        char config_file[256];
        wg_snprintf(config_file, sizeof(config_file), "%s/%s", home_dir, shell_rc);

        /* Check if library path already exists in config file */
        char grep_cmd[512];
        wg_snprintf(grep_cmd, sizeof(grep_cmd),
                                "grep -q \"LD_LIBRARY_PATH.*%s\" %s", lib_path, config_file);

        int path_exists = wg_run_command(grep_cmd);

        /* Add library path if not already present */
        if (path_exists != 0) {
                char export_cmd[512];
                char backup_cmd[WG_PATH_MAX * 3];

                /* Backup config file before modification */
                wg_snprintf(backup_cmd, sizeof(backup_cmd), "cp %s %s.backup", config_file, config_file);
                wg_run_command(backup_cmd);

                wg_snprintf(export_cmd, sizeof(export_cmd),
                                   "echo 'export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH' >> %s",
                                   lib_path, config_file);

                int ret = wg_run_command(export_cmd);

                if (ret == 0) {
                        printf("Successfully updated %s. Please run 'source %s' to apply changes.\n",
                                   shell_rc, config_file);
                } else {
                        fprintf(stderr, "Error updating %s\n", shell_rc);
                }
        } else {
                printf("Library path already exists in %s\n", shell_rc);
        }

        /* Update system library cache if needed */
        if (strfind(lib_path, "/usr/", false)) {
                int is_not_sudo = wg_run_command("sudo echo > /dev/null");
                if (is_not_sudo == 0) {
                        wg_run_command("sudo ldconfig");
                } else {
                        wg_run_command("ldconfig");
                }
        }
}
#endif

/* 
 * Install library files to system library directories on Linux/Unix
 */
static int setup_linux_library(void)
{
#ifdef WG_WINDOWS
        return 0; /* Not needed on Windows */
#else
        const char *selected_path = NULL;
        char libpawnc_src[WG_PATH_MAX];
        char dest_path[WG_PATH_MAX];
        int i, found_lib;

        /* Possible library installation paths */
        const char *lib_paths[] = {
                "/usr/local/lib",
                "/usr/local/lib32",
                "/data/data/com.termux/files/usr/lib/",
                "/data/data/com.termux/files/usr/local/lib/",
                "/data/data/com.termux/arm64/usr/lib",
                "/data/data/com.termux/arm32/usr/lib",
                "/data/data/com.termux/amd32/usr/lib",
                "/data/data/com.termux/amd64/usr/lib"
        };

        /* Skip if on Windows or unknown OS */
        if (!strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_WINDOWS) ||
                !strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_UNKNOWN))
                        return 0;

        /* Search for library file */
        char size_pf[WG_PATH_MAX + 56];
        wg_snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);
        found_lib = wg_sef_fdir(size_pf, "libpawnc.so", NULL);
        if (!found_lib)
                        return 0;

        /* Find full path to library */
        for (i = 0; i < wgconfig.wg_sef_count; i++) {
                        if (strstr(wgconfig.wg_sef_found_list[i], "libpawnc.so")) {
                                        wg_strncpy(libpawnc_src, wgconfig.wg_sef_found_list[i], sizeof(libpawnc_src));
                                        break;
                        }
        }

        /* Select appropriate library directory */
        for (i = 0; i < sizeof(lib_paths) / sizeof(lib_paths[0]); i++) {
                        if (path_exists(lib_paths[i])) {
                                        selected_path = lib_paths[i];
                                        break;
                        }
        }

        if (!selected_path) {
                        fprintf(stderr, "No valid library path found!\n");
                        return -1;
        }

        /* Copy library to system directory */
        wg_snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
        wg_sef_wmv(libpawnc_src, dest_path);

        /* Update environment variables */
        update_library_environment(selected_path);

        return 0;
#endif
}

/* 
 * Main function to install compiler tools after download
 * Organizes compiler files into proper directory structure
 */
void wg_apply_pawncc(void)
{
        int found_pawncc_exe, found_pawncc;
        int found_pawndisasm_exe, found_pawndisasm;
        int found_pawnc_dll, found_PAWNC_DLL;

        const char *dest_dir;

        char pawncc_src[WG_PATH_MAX] = { 0 },
                 pawncc_exe_src[WG_PATH_MAX] = { 0 },
                 pawndisasm_src[WG_PATH_MAX] = { 0 },
                 pawndisasm_exe_src[WG_PATH_MAX] = { 0 },
                 pawnc_dll_src[WG_PATH_MAX] = { 0 },
                 PAWNC_DLL_src[WG_PATH_MAX] = { 0 };

        int i;

        wg_sef_fdir_reset(); /* Reset file search cache */

        /* Find all compiler tools in downloaded directory */
        find_compiler_tools(wgconfig.wg_is_samp ? COMPILER_SAMP : COMPILER_OPENMP,
                                                &found_pawncc_exe, &found_pawncc,
                                                &found_pawndisasm_exe, &found_pawndisasm,
                                                &found_pawnc_dll, &found_PAWNC_DLL);

        /* Get destination directory (pawno or qawno) */
        dest_dir = get_compiler_directory();
        if (!dest_dir) {
                        pr_error(stdout, "Failed to create compiler directory");
                        goto apply_done;
        }

        /* Store paths to found compiler tools */
        for (i = 0; i < wgconfig.wg_sef_count; i++) {
                        const char *item = wgconfig.wg_sef_found_list[i];
                        if (!item)
                                        continue;

                        if (strstr(item, "pawncc.exe")) {
                                        wg_strncpy(pawncc_exe_src, item, sizeof(pawncc_exe_src));
                        } else if (strstr(item, "pawncc") && !strstr(item, ".exe")) {
                                        wg_strncpy(pawncc_src, item, sizeof(pawncc_src));
                        } else if (strstr(item, "pawndisasm.exe")) {
                                        wg_strncpy(pawndisasm_exe_src, item, sizeof(pawndisasm_exe_src));
                        } else if (strstr(item, "pawndisasm") && !strstr(item, ".exe")) {
                                        wg_strncpy(pawndisasm_src, item, sizeof(pawndisasm_src));
                        } else if (strstr(item, "pawnc.dll"))  {
                                        wg_strncpy(pawnc_dll_src, item, sizeof(pawnc_dll_src));
                        } else if (strstr(item, "PAWNC.dll")) {
                                        wg_strncpy(PAWNC_DLL_src, item, sizeof(PAWNC_DLL_src));
                        }
        }

        /* Copy each found tool to destination directory */
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

        /* Setup libraries on Linux/Unix systems */
        setup_linux_library();

        /* Clean up downloaded source directory */
        char rm_cmd[WG_PATH_MAX + 56];
        if (is_native_windows())
                wg_snprintf(rm_cmd, sizeof(rm_cmd),
                        "rmdir /s /q \"%s\"",
                        pawncc_dir_src);
        else
                wg_snprintf(rm_cmd, sizeof(rm_cmd),
                        "rm -rf %s",
                        pawncc_dir_src);
        wg_run_command(rm_cmd);

        memset(pawncc_dir_src, 0, sizeof(pawncc_dir_src));

        pr_info(stdout, "Compiler installed successfully!");

        /* Prompt to run compiler immediately */
        pr_color(stdout, FCOLOUR_CYAN, "Run compiler now? (y/n):");
        char *compile_now = readline(" ");
        if (!strcmp(compile_now, "Y") || !strcmp(compile_now, "y")) {
                wg_free(compile_now);
                pr_color(stdout, FCOLOUR_CYAN, "~ input pawn files (press enter for from config toml - enter E/e to exit):");
                char *gamemode_compile = readline(" ");
                if (strlen(gamemode_compile) < 1) {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                } else {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                }
        } else { wg_free(compile_now); }

apply_done:
        chain_goto_main(NULL);
}

/* 
 * Prompt user whether to install the downloaded compiler
 */
static int prompt_apply_pawncc(void)
{
        wgconfig.wg_ipawncc = 0;

        printf("\x1b[32m==> Apply pawncc?\x1b[0m\n");
        char *confirm = readline("   answer (y/n): ");

        fflush(stdout);

        if (!confirm) {
                fprintf(stderr, "Error reading input\n");
                wg_free(confirm);
                goto done;
        }

        if (strlen(confirm) == 0) {
                wg_free(confirm);
                confirm = readline(" >>> (y/n): ");
        }

        if (confirm) {
                if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
                        wg_free(confirm);
                        return 1;
                }
        }

done:
        return 0;
}

/* 
 * Check if a filename has a known archive extension
 */
int is_archive_file(const char *filename)
{
        if (strend(filename, ".zip", true) ||
                strend(filename, ".tar", true) ||
                strend(filename, ".tar.gz", true)) {
                return 1;
        }
        return 0;
}

/* 
 * Debug callback for CURL to display HTTP communication details
 */
static int debug_callback(CURL *handle, curl_infotype type,
                                                  char *data, size_t size, void *userptr)
{
        (void)handle;
        (void)userptr;

        switch (type) {
        case CURLINFO_TEXT:
        case CURLINFO_HEADER_OUT:
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_OUT:
                break;
        case CURLINFO_HEADER_IN:
                /* Filter out common headers to reduce noise */
                if (strfind(data, "location: ", false) || strfind(data, "content-security-policy: ", false))
                        break;
                printf("<= Recv header: %.*s", (int)size, data);
                break;
        case CURLINFO_DATA_IN:
        case CURLINFO_SSL_DATA_IN:
        default:
                break;
        }
        return 0;
}

/* 
 * Main file download function with retry logic and progress display
 * Handles HTTP downloads, SSL verification, and archive extraction
 */
int wg_download_file(const char *url, const char *filename)
{
#if defined (_DBG_PRINT)
        /* Debug information block - prints compilation environment details */
        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
        printf("[function: %s | "
                        "pretty function: %s | "
                        "line: %d | "
                        "file: %s | "
                        "date: %s | "
                        "time: %s | "
                        "timestamp: %s | "
                        "C standard: %ld | "
                        "C version: %s | "
                        "compiler version: %d | "
                        "architecture: %s]:\n",
                                __func__, __PRETTY_FUNCTION__,
                                __LINE__, __FILE__,
                                __DATE__, __TIME__,
                                __TIMESTAMP__,
                                __STDC_VERSION__,
                                __VERSION__,
                                __GNUC__,
#ifdef __x86_64__
                                "x86_64");
#elif defined(__i386__)
                                "i386");
#elif defined(__arm__)
                                "ARM");
#elif defined(__aarch64__)
                                "ARM64");
#else
                                "Unknown");
#endif
#endif

        if (!url || !filename) {
                pr_color(stdout, FCOLOUR_RED, "Error: Invalid URL or filename\n");
                return -1;
        }

        CURLcode res;
        CURL *curl = NULL;
        long response_code = 0;
        int retry_count = 0;
        struct stat file_stat;

        pr_color(stdout, FCOLOUR_GREEN, "On it: downloading %s", filename);

        /* Retry loop with up to 5 attempts */
        while (retry_count < 5) {
                curl = curl_easy_init();
                if (!curl) {
                        pr_color(stdout, FCOLOUR_RED, "Failed to initialize CURL\n");
                        return -1;
                }

                struct curl_slist *headers = NULL;

                /* Add GitHub authentication header if available */
                if (wgconfig.wg_idepends == 1) {
                        if (!wgconfig.wg_toml_github_tokens || 
                                strstr(wgconfig.wg_toml_github_tokens, "DO_HERE") ||
                                strlen(wgconfig.wg_toml_github_tokens) < 1) {
                                pr_color(stdout, FCOLOUR_YELLOW, " ~ GitHub token not available");
                                sleep(1);
                        } else {
                                char auth_header[512];
                                wg_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", 
                                                wgconfig.wg_toml_github_tokens);
                                headers = curl_slist_append(headers, auth_header);
                                pr_color(stdout, FCOLOUR_GREEN, " ~ Using GitHub token: %s\n",
                                                wg_masked_text(8, wgconfig.wg_toml_github_tokens));
                        }
                }

                /* Set standard HTTP headers */
                headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
                headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

                /* Configure CURL options */
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
                                
                struct buf download_buffer = { 0 };
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &download_buffer);

                curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

                /* Prompt for HTTP debugging on first download */
                static int create_debugging = 0;
                static int always_create_debugging = 0;
                
                if (create_debugging == 0) {
                        create_debugging = 1;
                        pr_color(stdout, FCOLOUR_CYAN, " %% Enable HTTP debugging? ");
                        char *debug_http = readline("(y/n): ");
                        if (debug_http && (debug_http[0] == 'Y' || debug_http[0] == 'y')) {
                                always_create_debugging = 1;
                        }
                        wg_free(debug_http);
                }
                
                /* Enable verbose debugging if requested */
                if (always_create_debugging) {
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
                        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
                }

                /* Set progress callback for download tracking */
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

                /* Configure SSL certificate verification */
                verify_cacert_pem(curl);

                fflush(stdout);
                res = curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);

                /* Check if download succeeded */
                if (res == CURLE_OK && response_code == WG_CURL_RESPONSE_OK && 
                        download_buffer.len > 0) {
                        
                        /* Write downloaded data to file */
                        FILE *fp = fopen(filename, "wb");
                        if (!fp) {
                                pr_color(stdout, FCOLOUR_RED, "Failed to open file for writing: %s\n", filename);
                                wg_free(download_buffer.data);
                                ++retry_count;
                                sleep(3);
                                continue;
                        }
                        
                        fwrite(download_buffer.data, 1, download_buffer.len, fp);
                        fclose(fp);
                        
                        wg_free(download_buffer.data);
                        
                        /* Verify file was written successfully */
                        if (stat(filename, &file_stat) == 0 && file_stat.st_size > 0) {
                                pr_color(stdout, FCOLOUR_GREEN, " Well done! Download successful: %" PRIdMAX " bytes\n", 
                                                (intmax_t)file_stat.st_size);
                                fflush(stdout);

                                /* Check if file is an archive */
                                if (!is_archive_file(filename)) {
                                        pr_color(stdout, FCOLOUR_YELLOW, "Warning: File is not an archive\n");
                                        return 1;
                                }

                                /* Extract archive contents */
                                wg_extract_archive(filename);

                                /* Prompt to remove archive file */
                                if (wgconfig.wg_idepends == 1) {
                                        static int remove_archive = 0;
                                        if (remove_archive == 0) {
                                                pr_color(stdout, FCOLOUR_CYAN, "==> Remove archive? ");
                                                char *confirm = readline("(y/n): ");
                                                if (confirm && (confirm[0] == 'Y' || confirm[0] == 'y')) {
                                                        remove_archive = 1;
                                                        char rm_cmd[WG_PATH_MAX];
                                                        if (is_native_windows())
                                                                wg_snprintf(rm_cmd, sizeof(rm_cmd),
                                                                        "if exist \"%s\" (del /f /q \"%s\" 2>nul)", filename, filename);
                                                        else
                                                                wg_snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", filename);
                                                        wg_run_command(rm_cmd);
                                                }
                                                wg_free(confirm);
                                        } else {
                                                char rm_cmd[WG_PATH_MAX];
                                                if (is_native_windows())
                                                        wg_snprintf(rm_cmd, sizeof(rm_cmd),
                                                                "if exist \"%s\" (del /f /q \"%s\" 2>nul)", filename, filename);
                                                else
                                                        wg_snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", filename);
                                                wg_run_command(rm_cmd);
                                        }
                                } else {
                                        pr_color(stdout, FCOLOUR_CYAN, "==> Remove archive? ");
                                        char *confirm = readline("(y/n): ");
                                        if (confirm && (confirm[0] == 'Y' || confirm[0] == 'y')) {
                                                char rm_cmd[WG_PATH_MAX];
                                                if (is_native_windows())
                                                        wg_snprintf(rm_cmd, sizeof(rm_cmd),
                                                                "if exist \"%s\" (del /f /q \"%s\" 2>nul)", filename, filename);
                                                else
                                                        wg_snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", filename);
                                                wg_run_command(rm_cmd);
                                        }
                                        wg_free(confirm);
                                }

                                /* Prompt to install compiler if this is a compiler download */
                                if (wgconfig.wg_ipawncc && prompt_apply_pawncc()) {
                                        char size_filename[WG_PATH_MAX];
                                        wg_snprintf(size_filename, sizeof(size_filename), "%s", filename);

                                        /* Remove file extension to get directory name */
                                        char *ext = strstr(size_filename, ".tar.gz");
                                        if (ext) {
                                                *ext = '\0';
                                        } else {
                                                ext = strrchr(size_filename, '.');
                                                if (ext) *ext = '\0';
                                        }
                                        
                                        wg_snprintf(pawncc_dir_src, sizeof(pawncc_dir_src), "%s", size_filename);
                                        wg_apply_pawncc();
                                }

                                return 0;
                        }
                }
                
                /* Clean up and retry on failure */
                if (download_buffer.data) {
                        wg_free(download_buffer.data);
                }

                pr_color(stdout, FCOLOUR_YELLOW, " Attempt %d/5 failed (HTTP: %ld). Retrying in 3s...\n", 
                                retry_count + 1, response_code);
                ++retry_count;
                sleep(3);
        }

        /* Final failure message after all retries */
        pr_color(stdout,
                        FCOLOUR_RED, " Failed to download %s. after retrying: %d\n",
                        filename, retry_count);

        return 1;
}