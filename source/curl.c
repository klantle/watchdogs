#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "extra.h"
#include "utils.h"
#include "crypto.h"
#include "archive.h"
#include "depend.h"
#include "compiler.h"
#include "units.h"
#include "debug.h"
#include "curl.h"

/* Array of online platform names and their URL patterns for account tracking */
const char* URL_FORUM_AIO[MAX_NUM_SITES][2] = {
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
static char
        pawncc_dir_src[WG_PATH_MAX]
;

/* 
 * Locate and configure CA certificate bundle for CURL SSL verification 
 * Searches for cacert.pem in platform-specific locations
 */
void verify_cacert_pem(CURL *curl) {
        int platform = 0;
#ifdef WG_ANDROID
        platform = 1;
#elif defined(WG_LINUX)
        platform = 2;
#elif defined(WG_WINDOWS)
        platform = 3;
#endif
        static int cacert_notice = 0; /* Track if warning already shown */

        /* Windows-specific certificate locations */
        if (platform == 3) {
                if (access("cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                else if (access("C:/libwatchdogs/cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
                else {
                        if (cacert_notice != 1) {
                                cacert_notice = 1;
                                pr_color(stdout, FCOLOUR_GREEN,
                                        "cURL: can't locate cacert.pem - SSL verification may fail.\n");
                        }
                }
        }
        /* Android-specific certificate locations */
        else if (platform == 1) {
                const char *prefix = getenv("PREFIX");
                if (!prefix || prefix[0] == '\0') {
                        prefix = "/data/data/com.termux/files/usr";
                }

                char ca1[WG_PATH_MAX];
                char ca2[WG_PATH_MAX];

                snprintf(ca1, sizeof(ca1), "%s/etc/tls/cert.pem", prefix);
                snprintf(ca2, sizeof(ca2), "%s/etc/ssl/certs/ca-certificates.crt", prefix);

                if (access(ca1, F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, ca1);
                else if (access(ca2, F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, ca2);
                else {
                        pr_color(stdout, FCOLOUR_GREEN,
                                "cURL: can't locate cacert.pem - SSL verification may fail.\n");
                }
        }
        /* Linux-specific certificate locations */
        else if (platform == 2) {
                if (access("cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
                else if (access("/etc/ssl/certs/cacert.pem", F_OK) == 0)
                        curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/cacert.pem");
                else {
                        if (cacert_notice != 1) {
                                cacert_notice = 1;
                                pr_color(stdout, FCOLOUR_GREEN,
                                        "cURL: can't locate cacert.pem - SSL verification may fail.\n");
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
        static const char term_spinner[] = /* Spinner characters */
                                           "-"
                                           "\\"
                                           "|"
                                           "/";

        int percent;

        /* Only show progress if total download size is known */
        if (dltotal > 0) {
                percent = (int)((dlnow * 100) / dltotal);

                /* Update display only when percentage changes */
                if (percent != last_percent) {
                        char spin_char = term_spinner[dot_index % 4];
                        dot_index++;

                        if (percent < 100)
                                printf("\r" FCOLOUR_CYAN "** Downloading... %3d%% [%c]", percent, spin_char);
                        else
                                printf("\r" FCOLOUR_CYAN "** Downloading... %3d%% Done!", percent);

                        fflush(stdout);
                        last_percent = percent;
                }
        }

        return 0;
}

/**
 * Initialize buffer structure with default capacity
 */
void buf_init(struct buf *b) {
        b->data = wg_malloc(WG_MAX_PATH);
        if (!b->data) {
                chain_ret_main(NULL);
        }
        b->len = 0;
        b->allocated = (b->data) ? WG_MAX_PATH : 0;
}

/**
 * Free buffer memory and reset structure
 */
void buf_free(struct buf *b) {
        wg_free(b->data);
        b->data = NULL;
        b->len = 0;
        b->allocated = 0;
}

/**
 * CURL write callback function for handling incoming data
 * Dynamically allocates memory as data arrives with safe boundary checks
 */
size_t write_callbacks(void *ptr, size_t size, size_t nmemb, void *userdata)
{
        struct buf *b = userdata;

        /* Validate buffer pointer alignment */
        if (b->data && ((uintptr_t)b->data & 0x7)) {
                fprintf(stderr, " Buffer pointer corrupted: %p\n", b->data);
                return 0;
        }

        size_t total = size * nmemb;

        /* Prevent processing excessively large chunks */
        if (total > 0xFFFFFFF) return 0;

        size_t required = b->len + total + 1;

        /* Reallocate buffer if additional space is needed */
        if (required > b->allocated) {
                /* Grow buffer by 50% or to required size, whichever is larger */
                size_t new_alloc = (b->allocated * 3) >> 1;
                new_alloc = (required > new_alloc) ? required : new_alloc;
                /* Cap maximum allocation at 64MB */
                new_alloc = (new_alloc < 0x4000000) ? new_alloc : 0x4000000;

                char *p = realloc(b->data, new_alloc);
                if (!p) {
#if defined(_DBG_PRINT)
                        fprintf(stderr, " Realloc failed for %zu bytes\n", new_alloc);
#endif
                        return 0;
                }

                b->data = p;
                b->allocated = new_alloc;
        }

        memcpy(b->data + b->len, ptr, total);
        b->len += total;
        b->data[b->len] = 0; /* Null-terminate string */

        return total;
}

/**
 * Initialize memory structure with default allocation
 */
void memory_struct_init(struct memory_struct *mem) {
        mem->memory = wg_malloc(WG_MAX_PATH);
        if (!mem->memory) {
                chain_ret_main(NULL);
        }
        mem->size = 0;
        mem->allocated = mem->memory ? WG_MAX_PATH : 0;
}

/**
 * Free memory structure and release allocated memory
 */
void memory_struct_free(struct memory_struct *mem) {
        wg_free(mem->memory);
        mem->memory = NULL;
        mem->size = 0;
        mem->allocated = 0;
}

/**
 * Generic memory write callback for CURL operations
 * Stores downloaded content in dynamically allocated memory with safety checks
 */
size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
        struct memory_struct *mem = userp;
        size_t realsize = size * nmemb;

        /* Input validation and size limit check */
        if (!contents || !mem || realsize > 0x10000000) /* Limit large chunks >256/MB */
                return 0;

        size_t required = mem->size + realsize + 1; /* +1 for null-terminator */

        /* Reallocate buffer if capacity is insufficient */
        if (required > mem->allocated) {
                size_t new_alloc = mem->allocated ? (mem->allocated * 2) : 0x1000; /* Start with 4/KB */
                if (new_alloc < required) new_alloc = required;
                if (new_alloc > 0x8000000) new_alloc = 0x8000000; /* Cap at 128/MB */

                char *ptr = realloc(mem->memory, new_alloc);
                if (!ptr) {
#if defined(_DBG_PRINT)
                        fprintf(stdout, "Memory exhausted at %zu bytes\n", new_alloc);
#endif
                        return 0;
                }
                mem->memory = ptr;
                mem->allocated = new_alloc;
        }

        /* Copy data to memory buffer */
        memcpy(mem->memory + mem->size, contents, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = '\0'; /* Null terminate string */

        return realsize;
}

/*
 * Generate username variations for account tracking.
 * Creates common username mutations to find accounts across platforms.
 */
void
account_tracker_discrepancy(const char *base, char discrepancy[][MAX_USERNAME_LEN], 
    int *cnt)
{
        int i, j, base_len;
        char temp[MAX_USERNAME_LEN];

        if (base == NULL || discrepancy == NULL || cnt == NULL ||
                *cnt >= MAX_VARIATIONS)
                return;

        base_len = strlen(base);
        if (base_len == 0 || base_len >= MAX_USERNAME_LEN)
                return;

        /* Add original username */
        strlcpy(discrepancy[(*cnt)++], base, MAX_USERNAME_LEN);

        /* Create variations with duplicate characters (e.g., "heello" from "hello") */
        for (i = 0; i < base_len && *cnt < MAX_VARIATIONS; i++) {
                /* Copy prefix */
                strncpy(temp, base, i);
                temp[i] = '\0';
                
                /* Duplicate current character */
                temp[i] = base[i];
                temp[i + 1] = base[i];
                
                /* Copy suffix */
                strlcpy(temp + i + 2, base + i + 1, 
                        sizeof(temp) - (i + 2));
                
                strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
        }

        /* Add trailing character repetitions (e.g., "hello111") */
        for (i = 2; i <= 5 && *cnt < MAX_VARIATIONS; i++) {
                strlcpy(temp, base, sizeof(temp));
                
                for (j = 0; j < i && strlen(temp) < MAX_USERNAME_LEN - 1; j++) {
                        temp[strlen(temp)] = base[base_len - 1];
                        temp[strlen(temp) + 1] = '\0';
                }
                
                strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
        }

        /* Add common suffixes */
        const char *suffixes[] = {
                "!", "@",
                "#", "$",
                "%", "^",
                "_", "-",
                ".",
                NULL	/* Sentinel */
        };

        for (i = 0; suffixes[i] != NULL && *cnt < MAX_VARIATIONS; i++) {
                snprintf(temp, sizeof(temp), "%s%s", base, suffixes[i]);
                strlcpy(discrepancy[(*cnt)++], temp, MAX_USERNAME_LEN);
        }
}

/* 
 * Check if a username exists on multiple online platforms
 * Uses CURL to probe each platform's URL pattern
 */
void account_tracking_username(CURL *curl, const char *username) {
        CURLcode res;
        
        struct memory_struct response;

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");

        /* Test username against each platform in the list */
        for (int i = 0; i < MAX_NUM_SITES; i++) {
                char url[200];
                snprintf(url, sizeof(url), URL_FORUM_AIO[i][1], username);

                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                memory_struct_init(&response);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

                verify_cacert_pem(curl);

                res = curl_easy_perform(curl);

                if (res != CURLE_OK) {
                        printf("[%s] %s -> ERROR %s\n",
                                URL_FORUM_AIO[i][0], url, curl_easy_strerror(res));
                } else {
                        long status_code;
                        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

                        if (status_code == WG_CURL_RESPONSE_OK) {
                                printf("[%s] %s -> FOUND\n", URL_FORUM_AIO[i][0], url);
                        } else {
                                printf("[%s] %s -> NOT FOUND (%ld)\n",
                                URL_FORUM_AIO[i][0], url, status_code);
                        }
                }

                memory_struct_free(&response);
        }

        curl_slist_free_all(headers);
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

        snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);

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

        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
        wg_sef_wmv(src_path, dest_path);
}

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
        char shell_path[WG_PATH_MAX];

        /* Detect shell type from SHELL environment variable */
        char *shell = getenv("SHELL");
        if (shell) {
                if (strfind(shell, "zsh", true)) {
                        shell_rc = ".zshrc";
                } else if (strfind(shell, "bash", true)) {
                        shell_rc = ".bashrc";
                }
        }

        /* Fallback to checking for existing config files */
        if (!shell_rc) {
                snprintf(shell_path, sizeof(shell_path), "%s/.zshrc", home_dir);
                if (access(shell_path, F_OK) == 0) {
                        shell_rc = ".zshrc";
                } else {
                        shell_rc = ".bashrc";
                }
        }

        char config_file[WG_PATH_MAX];
        snprintf(config_file, sizeof(config_file), "%s/%s", home_dir, shell_rc);

        /* Check if library path already exists in config file */
        char grep_cmd[512];
        snprintf(grep_cmd, sizeof(grep_cmd),
                "grep -q \"LD_LIBRARY_PATH.*%s\" %s", lib_path, config_file);

        int path_exists = wg_run_command(grep_cmd);

        /* Add library path if not already present */
        if (path_exists != 0) {
                char export_cmd[512];
                char backup_cmd[WG_PATH_MAX * 3];

                /* Backup config file before modification */
                snprintf(backup_cmd, sizeof(backup_cmd), "cp %s %s.backup", config_file, config_file);
                wg_run_command(backup_cmd);

                snprintf(export_cmd, sizeof(export_cmd),
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
        if (strfind(lib_path, "/usr/", true)) {
                int is_not_sudo = wg_run_command("sudo echo > /dev/null");
                if (is_not_sudo == 0) {
                        wg_run_command("sudo ldconfig");
                } else {
                        wg_run_command("ldconfig");
                }
        }
}

/* 
 * Install library files to system library directories on Linux/Unix
 */
static int setup_linux_library(void)
{
#ifdef WG_WINDOWS
        return 0; /* Not needed on Windows */
#endif
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
        snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);
        found_lib = wg_sef_fdir(size_pf, "libpawnc.so", NULL);
        if (!found_lib)
                return 0;

        /* Find full path to library */
        for (i = 0; i < wgconfig.wg_sef_count; i++) {
                if (strstr(wgconfig.wg_sef_found_list[i], "libpawnc.so")) {
                        strncpy(libpawnc_src, wgconfig.wg_sef_found_list[i], sizeof(libpawnc_src));
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
        snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
        wg_sef_wmv(libpawnc_src, dest_path);

        /* Update environment variables */
        update_library_environment(selected_path);

        return 0;
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
                snprintf(rm_cmd, sizeof(rm_cmd),
                        "rmdir /s /q \"%s\"",
                        pawncc_dir_src);
        else
                snprintf(rm_cmd, sizeof(rm_cmd),
                        "rm -rf %s",
                        pawncc_dir_src);
        wg_run_command(rm_cmd);

        memset(pawncc_dir_src, 0, sizeof(pawncc_dir_src));

        pr_info(stdout, "Congratulations! compiler installed successfully.");
        /* Prompt to run compiler immediately */
        pr_color(stdout, FCOLOUR_CYAN, "Run compiler now? (y/n):");
        char *compile_now = readline(" ");
        if (compile_now[0] == 'Y' || compile_now[0] == 'y') {
            wg_free(compile_now);
            pr_color(stdout, FCOLOUR_CYAN, "Please input the pawn file\n\t* "
                "(just enter for %s - input E/e to exit):", wgconfig.wg_toml_proj_input);
            char *gamemode_compile = readline(" ");
            
            /* Determine compilation target */
            const char *target = (strlen(gamemode_compile) < 1) ? 
                                wgconfig.wg_toml_proj_input : gamemode_compile;
            
            /* Create file if it doesn't exist */
            if (path_exists(target) == 0) {
                pr_info(stdout, "File: %s.pwn not found!.. creating...", target);

                if (dir_exists("gamemodes") == 0) MKDIR("gamemodes");
                
                char size_gamemode[WG_PATH_MAX];
                snprintf(size_gamemode, sizeof(size_gamemode), 
                        (target == wgconfig.wg_toml_proj_input) ? 
                        "%s" : "gamemodes/%s.pwn", target);
                
                FILE *test = fopen(size_gamemode, "w+");
                if (test) {
                    wgconfig.wg_toml_proj_input = strdup(size_gamemode);
                    const char *default_code = 
                        "native printf(const format[], {Float,_}:...);\n"
                        "main() {\n"
                        "\tprintf(\"Hello, World!\");\n"
                        "}";
                    fwrite(default_code, 1, strlen(default_code), test);
                    fclose(test);
                }
            }
            
            /* Run compiler with appropriate arguments */
            wg_run_compiler(
                NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
            );
            wg_free(gamemode_compile);
        } else { 
            wg_free(compile_now); 
        }

apply_done:
        chain_ret_main(NULL);
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
                if (strfind(data, "location: ", true) ||
                    strfind(data, "content-security-policy: ", true))
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

/* Helper function to sanitize filename */
static void persing_filename(char *filename) {
        if (filename[0] == '\0') return;
        
        /* Replace invalid characters with underscores */
        for (char *p = filename; *p; ++p) {
                if (*p == '?' || *p == '*' ||
                    *p == '<' || *p == '>' || 
                    *p == '|' || *p == ':' ||
                    *p == '"' || *p == '\\' || *p == '/') {
                        *p = '_';
                }
        }
        
        /* Trim whitespace */
        char *end = filename + strlen(filename) - 1;
        while (end > filename && isspace((unsigned char)*end)) {
                *end-- = '\0';
        }
        
        /* Ensure filename is not empty */
        if (strlen(filename) == 0) {
                strcpy(filename, "downloaded_file");
        }
}

/* Main file download function with retry logic and progress display */
int wg_download_file(const char *url, const char *output_filename)
{
        __debug_function();

        if (!url || !output_filename) {
                pr_color(stdout, FCOLOUR_RED, "Error: Invalid URL or filename\n");
                return -1;
        }

        CURLcode res;
        CURL *curl = NULL;
        long response_code = 0;
        int retry_count = 0;
        struct stat file_stat;
        
        /* Clean filename and remove query parameters */
        char clean_filename[WG_PATH_MAX];
        char *query_pos = strchr(output_filename, '?');
        if (query_pos) {
                size_t name_len = query_pos - output_filename;
                if (name_len >= sizeof(clean_filename)) {
                        name_len = sizeof(clean_filename) - 1;
                }
                strncpy(clean_filename, output_filename, name_len);
                clean_filename[name_len] = '\0';
        } else {
                strncpy(clean_filename, output_filename, sizeof(clean_filename) - 1);
                clean_filename[sizeof(clean_filename) - 1] = '\0';
        }
        
        /* Extract basename from URL if needed */
        char final_filename[WG_PATH_MAX];
        if (strstr(clean_filename, "://") || strstr(clean_filename, "http")) {
                const char *url_filename = strrchr(url, '/');
                if (url_filename) {
                        url_filename++;
                        
                        char *url_query_pos = strchr(url_filename, '?');
                        if (url_query_pos) {
                                size_t url_name_len = url_query_pos - url_filename;
                                if (url_name_len >= sizeof(final_filename)) {
                                        url_name_len = sizeof(final_filename) - 1;
                                }
                                strncpy(final_filename, url_filename, url_name_len);
                                final_filename[url_name_len] = '\0';
                        } else {
                                strncpy(final_filename, url_filename, sizeof(final_filename) - 1);
                                final_filename[sizeof(final_filename) - 1] = '\0';
                        }
                } else {
                        snprintf(final_filename, sizeof(final_filename), "downloaded_file");
                }
        } else {
                strncpy(final_filename, clean_filename, sizeof(final_filename) - 1);
                final_filename[sizeof(final_filename) - 1] = '\0';
        }
        
        persing_filename(final_filename);

        pr_color(stdout, FCOLOUR_GREEN, "* Try Downloading %s", final_filename);

        /* Retry loop with exponential backoff */
        while (retry_count < 5) {
                curl = curl_easy_init();
                if (!curl) {
                        pr_color(stdout, FCOLOUR_RED, "Failed to initialize CURL\n");
                        return -1;
                }

                /* Configure HTTP headers */
                struct curl_slist *headers = NULL;

                if (wgconfig.wg_idepends == 1) {
                        if (!wgconfig.wg_toml_github_tokens || 
                                strfind(wgconfig.wg_toml_github_tokens, "DO_HERE", true) ||
                                strlen(wgconfig.wg_toml_github_tokens) < 1) {
                                pr_color(stdout, FCOLOUR_YELLOW, " ~ GitHub token not available\n");
                                sleep(1);
                        } else {
                                char auth_header[512];
                                snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", 
                                                wgconfig.wg_toml_github_tokens);
                                headers = curl_slist_append(headers, auth_header);
                                pr_color(stdout, FCOLOUR_GREEN, " ~ Using GitHub token: %s\n",
                                                wg_masked_text(8, wgconfig.wg_toml_github_tokens));
                        }
                }

                headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
                headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

                /* Set CURL options */
                curl_easy_setopt(curl, CURLOPT_URL, url);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
                                
                struct buf download_buffer = { 0 };
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

                /* Optional HTTP debugging */
                static int create_debugging = 0;
                static int always_create_debugging = 0;
                
                if (create_debugging == 0) {
                        create_debugging = 1;
                        pr_color(stdout, FCOLOUR_CYAN, " * Enable HTTP debugging? ");
                        char *debug_http = readline("(y/n): ");
                        if (debug_http && (debug_http[0] == 'Y' || debug_http[0] == 'y')) {
                                always_create_debugging = 1;
                        }
                        wg_free(debug_http);
                }
                
                if (always_create_debugging) {
                        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
                        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
                }

                /* Progress display setup */
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

                /* SSL certificate configuration */
                verify_cacert_pem(curl);

                /* Execute download */
                fflush(stdout);
                res = curl_easy_perform(curl);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

                curl_easy_cleanup(curl);
                curl_slist_free_all(headers);

                /* Process successful download */
                if (res == CURLE_OK && response_code == WG_CURL_RESPONSE_OK && 
                        download_buffer.len > 0) {
                        
                        /* Write data to file */
                        FILE *fp = fopen(final_filename, "wb");
                        if (!fp) {
                                pr_color(stdout, FCOLOUR_RED, "* Failed to open file for writing: %s (errno: %d - %s)\n", 
                                        final_filename, errno, strerror(errno));
                                wg_free(download_buffer.data);
                                ++retry_count;
                                sleep(3);
                                continue;
                        }
                        
                        size_t written = fwrite(download_buffer.data, 1, download_buffer.len, fp);
                        fclose(fp);
                        
                        if (written != download_buffer.len) {
                                pr_color(stdout, FCOLOUR_RED, "* Failed to write all data to file: %s (written: %zu, expected: %zu)\n",
                                        final_filename, written, download_buffer.len);
                                wg_free(download_buffer.data);
                                unlink(final_filename); /* Remove partial file */
                                ++retry_count;
                                sleep(3);
                                continue;
                        }
                        
                        wg_free(download_buffer.data);
                        
                        /* Verify downloaded file */
                        if (stat(final_filename, &file_stat) == 0 && file_stat.st_size > 0) {
                                pr_color(stdout, FCOLOUR_GREEN, " %% successful: %" PRIdMAX " bytes to %s\n", 
                                                (intmax_t)file_stat.st_size, final_filename);
                                fflush(stdout);

                                /* Check if file is an archive */
                                if (!is_archive_file(final_filename)) {
                                        pr_color(stdout, FCOLOUR_YELLOW, "Warning: File %s is not an archive\n", final_filename);
                                        return 1;
                                }

                                /* Extract archive */
                                wg_extract_archive(final_filename);

                                /* Prompt for archive removal */
                                if (wgconfig.wg_idepends == 1) {
                                        static int remove_archive = 0;
                                        if (remove_archive == 0) {
                                                pr_color(stdout, FCOLOUR_CYAN, 
                                                        "==> Remove archive %s? ", final_filename);
                                                char *confirm = readline("(y/n): ");
                                                
                                                if (confirm && (confirm[0] == 'Y' || confirm[0] == 'y')) {
                                                        remove_archive = 1;
                                                        if (unlink(final_filename) != 0) {
                                                                pr_color(stdout, FCOLOUR_YELLOW,
                                                                        "Warning: Failed to remove %s: %s\n",
                                                                        final_filename, strerror(errno));
                                                        }
                                                }
                                                wg_free(confirm);
                                        } else {
                                                if (unlink(final_filename) != 0) {
                                                        pr_color(stdout, FCOLOUR_YELLOW,
                                                                "Warning: Failed to remove %s: %s\n",
                                                                final_filename, strerror(errno));
                                                }
                                        }
                                } else {
                                        pr_color(stdout, FCOLOUR_CYAN, 
                                                "==> Remove archive %s? ", final_filename);
                                        char *confirm = readline("(y/n): ");
                                        
                                        if (confirm && (confirm[0] == 'Y' || confirm[0] == 'y')) {
                                                if (unlink(final_filename) != 0) {
                                                        pr_color(stdout, FCOLOUR_YELLOW,
                                                                "Warning: Failed to remove %s: %s\n",
                                                                final_filename, strerror(errno));
                                                }
                                        }
                                        wg_free(confirm);
                                }

                                /* Special handling for compiler installation */
                                if (wgconfig.wg_ipawncc && prompt_apply_pawncc()) {
                                        char size_filename[WG_PATH_MAX];
                                        snprintf(size_filename, sizeof(size_filename), "%s", final_filename);

                                        char *extension = strstr(size_filename, ".tar.gz");
                                        if (extension) {
                                                *extension = '\0';
                                        } else {
                                                extension = strrchr(size_filename, '.');
                                                if (extension) *extension = '\0';
                                        }
                                        
                                        snprintf(pawncc_dir_src, sizeof(pawncc_dir_src), "%s", size_filename);
                                        wg_apply_pawncc();
                                }

                                return 0;
                        }
                }
                
                /* Cleanup failed attempt */
                if (download_buffer.data) {
                        wg_free(download_buffer.data);
                }

                pr_color(stdout, FCOLOUR_YELLOW, " Attempt %d/5 failed (HTTP: %ld). Retrying in 3s...\n", 
                                retry_count + 1, response_code);
                ++retry_count;
                sleep(3);
        }

        /* Final failure after all retries exhausted */
        pr_color(stdout, FCOLOUR_RED, " Failed to download %s from %s after %d retries\n",
                final_filename, url, retry_count);

        return 1;
}