/*
 * Watchdogs Core System Initialization & Utility Module
 * -----------------------------------------------------
 * This module implements the core infrastructure for the Watchdogs framework.
 * It provides essential low-level functionality required by higher-level
 * components such as the compiler manager, runtime controller, and build
 * automation logic.
 *
 * Key responsibilities:
 *  - Cross-platform filesystem and directory management (Windows / Linux / macOS)
 *  - String and console utilities (ANSI color, formatted output)
 *  - Configuration parsing and serialization using TOML (via tomlc99)
 *  - Pawn compiler (pawncc) and disassembler (pawndisasm) integration
 *  - Environment setup and OS detection
 *
 * Design notes:
 *  - The module avoids external dependencies except for standard C libraries,
 *    tomlc99, ncursesw, and libcurl.
 *  - Functions in this file are intended to be stateless and reusable.
 *  - All system-dependent behavior is isolated via #ifdef _WIN32 guards.
 *  - Error handling follows a consistent pattern using return codes and
 *    stderr logging for diagnostics.
 *
 * This file forms the foundation layer for all Watchdogs system features.
 * Higher-level modules (e.g. watchdogs_runtime.c, watchdogs_build.c) depend
 * on its utilities for reliable, platform-agnostic operation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <shlwapi.h>
#include <strings.h>
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path) // override POSIX mkdir
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#define PATH_SEP "/"
#endif

#include <math.h>
#include <limits.h>
#include <ncursesw/curses.h>
#include <time.h>
#include <ftw.h>
#include <sys/file.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "utils.h"
#include "watchdogs.h"

const char* __command[] = {
        "clear",
        "compile",
        "debug",
        "exit",
        "gamemode",
        "help",
        "kill",
        "pawncc",
        "restart",
        "running",
        "stop",
        "title"
};
const size_t
	__command_len = sizeof(__command) / sizeof(__command[0]);

inline void watchdogs_reset_var(void) {
        wd wcfg = {0};
}
inline int watchdogs_sys(const char *cmd) {
        return system(cmd);
}

wd wcfg = {
        .init_ipcc = 0,
        .watchdogs_sef_count = 0,
        .watchdogs_sef_found = { {0} },
        .watchdogs_os = NULL,
        .server_or_debug = NULL,
        .wd_compiler_opt = NULL,
        .wd_gamemode_input = NULL,
        .wd_gamemode_output = NULL
};

void reset_watchdogs_sef_dir()
{
        size_t MAX_ENTRIES = sizeof(wcfg.watchdogs_sef_found) /
                             sizeof(wcfg.watchdogs_sef_found[0]);

        for (size_t i = 0;
			 i < MAX_ENTRIES;
			 ++i)
            wcfg.watchdogs_sef_found[i][0] = '\0';

        memset(wcfg.watchdogs_sef_found, 0, sizeof(wcfg.watchdogs_sef_found));
        wcfg.watchdogs_sef_count = 0;
}

inline void handle_sigint(int sig)
{
        println("Exit?, You only exit with use a \"exit\"");
        __init(0);
}

inline int watchdogs_title(const char *__title)
{
        const char
                *title = __title ? __title : "Watchdogs";
        printf("\033]0;%s\007", title);
        return 0;
}

void copy_strip_dot_if_no_slash(char *dst, size_t dst_sz, const char *src) {
        if (!dst || dst_sz == 0 || !src) return;

        const char *slash = strchr(src, '/');
#ifdef _WIN32
        if (!slash) slash = strchr(src, '\\');
#endif

        if (slash == NULL) {
            const char *dot = strchr(src, '.');
            if (dot) {
                size_t len = (size_t)(dot - src);
                if (len >= dst_sz) len = dst_sz - 1;
                memcpy(dst, src, len);
                dst[len] = '\0';
                return;
            }
        }

        snprintf(dst, dst_sz, "%s", src);
    }

void __escape_quotes(char *dest, size_t size, const char *src) {
        size_t j = 0;
        for (size_t i = 0; src[i] != '\0' && j + 1 < size; i++) {
            if (src[i] == '"') {
                if (j + 2 >= size) break;
                dest[j++] = '\\';
                dest[j++] = '"';
            } else {
                dest[j++] = src[i];
            }
        }
        dest[j] = '\0';
}

int __command_suggest(const char *s1, const char *s2) {
        int len1 = strlen(s1);
        int len2 = strlen(s2);
        if (len2 > 128) return INT_MAX;
        int prev[129], curr[129];
        for (int j = 0; j <= len2; j++) prev[j] = j;

        for (int i = 1; i <= len1; i++) {
                curr[0] = i;
                for (int j = 1; j <= len2; j++) {
                        int cost = (s1[i-1] == s2[j-1]) ? 0 : 1,
                            del = prev[j] + 1,
                            ins = curr[j-1] + 1,
                            sub = prev[j-1] + cost;
                        curr[j] = (del < ins ? (del < sub ? del : sub)
                                                : (ins < sub ? ins : sub));
                }
                memcpy(prev, curr, (len2 + 1) * sizeof(int));
        }
        return prev[len2];
}

const char* __find_c_command(
    const char *ptr_command,
    const char *__commands[],
    size_t num_cmds,
    int *out_distance
) {
        int best_distance = INT_MAX;
        const char *best_cmd = NULL;

        for (size_t i = 0; i < num_cmds; i++) {
                int dist = __command_suggest(ptr_command, __commands[i]);
                if (dist < best_distance) {
                        best_distance = dist;
                        best_cmd = __commands[i];
                }
        }

        if (out_distance) *out_distance = best_distance;
        return best_cmd;
}

void printf_color(const char *color,
                  const char *format, ...)
{
        va_list args;
        va_start(args, format);

        printf("%s", color);
        vprintf(format, args);
        printf("%s", COL_DEFAULT);

        va_end(args);
}

void println(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
}

void printf_succes(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_YELLOW, "succes: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_info(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_YELLOW, "info: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_warning(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_GREEN, "warning: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_RED, "error: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_crit(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_RED, "crit: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

const char* watchdogs_detect_os(void) {
        static char os[64] = "unknown";

        if ((getenv("OS") && strstr(getenv("OS"), "Windows_NT")) ||
            getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME")) {
            return "windows";
        }

#ifndef _WIN32
        struct utsname sys_info;
        if (uname(&sys_info) == 0) {
            if (strstr(sys_info.sysname, "Linux"))
                strncpy(os, "linux", sizeof(os));
            else if (strstr(sys_info.sysname, "Darwin"))
                strncpy(os, "macos", sizeof(os));
            else
                strncpy(os, sys_info.sysname, sizeof(os));
        }
#else
        strncpy(os, "windows", sizeof(os));
#endif

        os[sizeof(os)-1] = '\0';
        return os;
}

int signal_system_os(void) {
        if (strcmp(wcfg.watchdogs_os, "windows") == 0)
                return 0x01;
        else if (strcmp(wcfg.watchdogs_os, "linux") == 0)
                return 0x00;
        
        return 0;
}

int dir_exists(const char *path) {
        struct stat st;
        return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int kill_process(const char *name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "pkill -9 -f \"%s\" > /dev/null 2>&1", name);
        return system(cmd);
}

int watchdogs_toml_data(void)
{
        const char *fname = "watchdogs.toml";
        FILE *toml_files;

        int find_gamemodes = watchdogs_sef_fdir("gamemodes/", "*.pwn");

        toml_files = fopen(fname, "r");
        if (toml_files != NULL)
                fclose(toml_files);
        else {
                toml_files = fopen(fname, "w");
                if (toml_files != NULL) {
                    if (find_gamemodes) {
                        const char *os_type = watchdogs_detect_os();
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "   os = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        fprintf(toml_files, "   option = \"-;+ -(+ -d3\"\n");
                        fprintf(toml_files, "   include_path = [\"sample1\", \"sample2\"]\n");
                        char i_path_rm[PATH_MAX];
                        snprintf(i_path_rm, sizeof(i_path_rm), "%s", wcfg.watchdogs_sef_found[0]);
                        char *f_EXT = strrchr(i_path_rm, '.');
                        if (f_EXT && strcmp(f_EXT, ".pwn") == 0)
                            *f_EXT = '\0';
                        fprintf(toml_files, "   input = \"%s.pwn\"\n", i_path_rm);
                        fprintf(toml_files, "   output = \"%s.amx\"\n", i_path_rm);
                        fclose(toml_files);
                    }
                    else {
                        const char *os_type = watchdogs_detect_os();
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "   os = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        fprintf(toml_files, "   option = \"-;+ -(+ -d3\"\n");
                        fprintf(toml_files, "   include_path = [\"sample1\", \"sample2\"]\n");
                        fprintf(toml_files, "   input = \"main.pwn\"\n");
                        fprintf(toml_files, "   output = \"main.amx\"\n");
                        fclose(toml_files);
                    }
                }
        }
        FILE *procc_f = fopen(fname, "r");
        if (!procc_f) printf_error("Can't a_read file %ss\n", fname);

        char errbuf[256];
        toml_table_t *config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
        fclose(procc_f);

        if (!config) printf_error("error parsing TOML: %s\n", errbuf);

        toml_table_t *_watchdogs_general = toml_table_in(config, "general");
        if (_watchdogs_general) {
                toml_datum_t os_val = toml_string_in(_watchdogs_general, "os");
                if (os_val.ok) wcfg.watchdogs_os = os_val.u.s;
        }

        return 0;
}

static void join_path(char *out, size_t out_sz, const char *dir, const char *name) {
        if (!out || out_sz == 0) return;
        size_t dir_len = strlen(dir);

#ifdef _WIN32
        #define IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#else
        #define IS_PATH_SEP(c) ((c) == '/')
#endif

        int dir_has_sep = (dir_len > 0 && IS_PATH_SEP(dir[dir_len - 1])),
            name_has_leading_sep = IS_PATH_SEP(name[0]);

        if (dir_has_sep) {
            if (name_has_leading_sep) {
                snprintf(out, out_sz, "%s%s", dir, name + 1);
            } else {
                snprintf(out, out_sz, "%s%s", dir, name);
            }
        } else {
#ifdef _WIN32
            if (name_has_leading_sep)
                snprintf(out, out_sz, "%s%s", dir, name);
            else
                snprintf(out, out_sz, "%s%s%s", dir, PATH_SEP, name);
#else
            if (name_has_leading_sep)
                snprintf(out, out_sz, "%s%s", dir, name);
            else
                snprintf(out, out_sz, "%s%s%s", dir, PATH_SEP, name);
#endif
        }
        out[out_sz - 1] = '\0';
}

int watchdogs_sef_fdir(const char *sef_path, const char *sef_name) {
        char path_buff[SEF_PATH_SIZE];

#ifdef _WIN32
        WIN32_FIND_DATA findFileData;
        HANDLE hFind;
        char searchPath[MAX_PATH];

        if (sef_path[strlen(sef_path)-1] == '\\') {
            snprintf(searchPath, sizeof(searchPath), "%s*", sef_path);
        } else {
            snprintf(searchPath, sizeof(searchPath), "%s\\*", sef_path);
        }

        hFind = FindFirstFile(searchPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            return 0;
        }
        do {
            const char *name = findFileData.cFileName;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            join_path(path_buff, sizeof(path_buff), sef_path, name);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (watchdogs_sef_fdir(path_buff, sef_name)) {
                    FindClose(hFind);
                    return 1;
                }
            } else {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (PathMatchSpecA(name, sef_name)) {
                        strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.watchdogs_sef_count++;
                        FindClose(hFind);
                        return 1;
                    }
                } else {
                    if (strcmp(name, sef_name) == 0) {
                        strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.watchdogs_sef_count++;
                        FindClose(hFind);
                        return 1;
                    }
                }
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
        return 0;
#else
        DIR *dir;
        struct dirent *entry;
        struct stat statbuf;

        dir = opendir(sef_path);
        if (!dir) {
            return 0;
        }

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.' &&
               (entry->d_name[1] == '\0' ||
               (entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
               continue;

            join_path(path_buff, sizeof(path_buff), sef_path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                if (watchdogs_sef_fdir(path_buff, sef_name)) {
                    closedir(dir);
                    return 1;
                }
            } else if (entry->d_type == DT_REG) {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (fnmatch(sef_name, entry->d_name, 0) == 0) {
                        strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.watchdogs_sef_count++;
                        closedir(dir);
                        return 1;
                    }
                } else {
                    if (strcmp(entry->d_name, sef_name) == 0) {
                        strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.watchdogs_sef_count++;
                        closedir(dir);
                        return 1;
                    }
                }
            } else {
                if (stat(path_buff, &statbuf) == -1)
                    continue;

                if (S_ISDIR(statbuf.st_mode)) {
                    if (watchdogs_sef_fdir(path_buff, sef_name)) {
                        closedir(dir);
                        return 1;
                    }
                } else if (S_ISREG(statbuf.st_mode)) {
                    if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                        if (fnmatch(sef_name, entry->d_name, 0) == 0) {
                            strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                    path_buff, SEF_PATH_SIZE);
                            wcfg.watchdogs_sef_count++;
                            closedir(dir);
                            return 1;
                        }
                    } else {
                        if (strcmp(entry->d_name, sef_name) == 0) {
                            strncpy(wcfg.watchdogs_sef_found[wcfg.watchdogs_sef_count],
                                    path_buff, SEF_PATH_SIZE);
                            wcfg.watchdogs_sef_count++;
                            closedir(dir);
                            return 1;
                        }
                    }
                }
            }
        }

        closedir(dir);
        return 0;
#endif
}

int watchdogs_sef_wmv(const char *c_src, const char *c_dest)
{
        FILE *src_FILE = fopen(c_src, "rb");
        if (!src_FILE) {
            perror("fopen src");
            return 1;
        }

        FILE *dest_FILE = fopen(c_dest, "wb");
        if (!dest_FILE) {
            if (errno == EACCES) {
                fclose(src_FILE);

                printf_color(COL_RED, "[E] ");
                println("Permission denied. try with sudo...");

                char esc_src[128],
                     esc_dest[128];
                __escape_quotes(esc_src, sizeof(esc_src), c_src);
                __escape_quotes(esc_dest, sizeof(esc_dest), c_dest);

                char cmd[526];
                snprintf(cmd, sizeof(cmd), "sudo mv \"%s\" \"%s\"",
                                            esc_src,
                                            esc_dest);
                int ret = system(cmd);
                if (ret == 0) {
                    printf(" Moving with sudo successfully: %s -> %s\n", c_src, c_dest);
                    return 0;
                } else {
                    fprintf(stderr, " Moving with sudo failed with code %d\n", ret);
                    return 1;
                }
            } else {
                perror("fopen dest");
                fclose(src_FILE);
                return 1;
            }
        }

        char buf[520];
        size_t bytes;
        while ((bytes = fread(buf, 1, sizeof(buf), src_FILE)) > 0)
            fwrite(buf, 1, bytes, dest_FILE);

        fclose(src_FILE);
        fclose(dest_FILE);

        char chmod_cmd[128];
        snprintf(chmod_cmd, sizeof(chmod_cmd), "chmod +x \"%s\"", c_dest);
        int ret = system(chmod_cmd);
        if (ret == 0) {
            return 0;
        } else {
            return 1;
        }

        return 0;
}

int watchdogs_sef_wcopy(const char *c_src,
                        const char *c_dest)
{
        FILE *src_FILE = fopen(c_src, "rb");
        if (src_FILE == NULL)
                return 1;
        FILE *dest_FILE = fopen(c_dest, "wb");
        if (dest_FILE == NULL)
                return 1;

        char src_buff[520];
        size_t bytes;
        while ((bytes = fread(src_buff, 1, sizeof(src_buff), src_FILE)) > 0)
                fwrite(src_buff, 1, bytes, dest_FILE);

        fclose(src_FILE);
        fclose(dest_FILE);

        char chmod_cmd[128];
        snprintf(chmod_cmd, sizeof(chmod_cmd), "chmod +x \"%s\"", c_dest);
        int ret = system(chmod_cmd);
        if (ret == 0) {
            return 0;
        } else {
            return 1;
        }

        return 0;
}

void
install_pawncc_now(void) {
        int __watchdogs_os__ = signal_system_os();
        int find_pawncc_exe = watchdogs_sef_fdir(".", "pawncc.exe"),
                find_pawncc = watchdogs_sef_fdir(".", "pawncc"),
                find_pawndisasm_exe = watchdogs_sef_fdir(".", "pawndisasm.exe"),
                find_pawndisasm = watchdogs_sef_fdir(".", "pawndisasm");

        int dir_pawno=0, dir_qawno=0;
        char *dest_path = NULL,
             str_dest_path[526];

        struct stat st;
        if (stat("pawno", &st) == 0 && S_ISDIR(st.st_mode)) {
                dir_pawno=1;
                dest_path="pawno";
        }
        if (stat("qawno", &st) == 0 && S_ISDIR(st.st_mode)) {
                dir_qawno=1;
                dest_path="qawno";
        }
        if (!dir_pawno && !dir_qawno) {
#ifdef _WIN32
            if (mkdir("pawno") == 0) dest_path = "pawno";
#else
            if (mkdir("pawno", 0755) == 0) dest_path = "pawno";
#endif
        }

        sleep(2);

        char pawncc_dest_path[512] = {0}, pawncc_exe_dest_path[512] = {0},
             pawndisasm_dest_path[512] = {0}, pawndisasm_exe_dest_path[512] = {0};

        for (int i = 0; i < wcfg.watchdogs_sef_count; i++) {
            const char *entry = wcfg.watchdogs_sef_found[i];
            if (!entry) continue;

            if (strstr(entry, "pawncc.exe")) {
                snprintf(pawncc_exe_dest_path, sizeof(pawncc_exe_dest_path), "%s", entry);
                find_pawncc_exe = 1;
            } else if (strstr(entry, "pawncc")) {
                snprintf(pawncc_dest_path, sizeof(pawncc_dest_path), "%s", entry);
                find_pawncc = 1;
            }
            else if (strstr(entry, "pawndisasm.exe")) {
                snprintf(pawndisasm_exe_dest_path, sizeof(pawndisasm_exe_dest_path), "%s", entry);
                find_pawndisasm_exe = 1;
            } else if (strstr(entry, "pawndisasm")) {
                snprintf(pawndisasm_dest_path, sizeof(pawndisasm_dest_path), "%s", entry);
                find_pawndisasm = 1;
            }
        }

        if (find_pawncc_exe) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s", dest_path, PATH_SEP, "pawncc.exe");
            watchdogs_sef_wmv(pawncc_exe_dest_path, str_dest_path);
        }
        if (find_pawncc) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s", dest_path, PATH_SEP, "pawncc");
            watchdogs_sef_wmv(pawncc_dest_path, str_dest_path);
        }
        if (find_pawndisasm_exe) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s", dest_path, PATH_SEP, "pawndisasm.exe");
            watchdogs_sef_wmv(pawndisasm_exe_dest_path, str_dest_path);
        }
        if (find_pawndisasm) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s", dest_path, PATH_SEP, "pawndisasm");
            watchdogs_sef_wmv(pawndisasm_dest_path, str_dest_path);
        }

#ifndef _WIN32
        if (__watchdogs_os__ == 0x00) {
                char *str_lib_path = NULL;
                char str_full_dest_path[128];

                int find_libpawnc = watchdogs_sef_fdir(".", "libpawnc.so");
                char libpawnc_dest_path[1024];
                for (int i = 0; i < wcfg.watchdogs_sef_count; i++) {
                        if (strstr(wcfg.watchdogs_sef_found[i], "libpawnc.so")) {
                                snprintf(libpawnc_dest_path, sizeof(libpawnc_dest_path), "%s",
                                                                                                wcfg.watchdogs_sef_found[i]);
                                break;
                        }
                }

                struct stat st;
                int lib_or_lib32 = 0;
                if (stat("/usr/local/lib32", &st) == 0 && S_ISDIR(st.st_mode)) {
                        lib_or_lib32=2;
                        str_lib_path="/usr/local/lib32";
                } else if (stat("/data/data/com.termux/files/usr/local/lib/", &st) == 0 && S_ISDIR(st.st_mode)) {
                        str_lib_path="/data/data/com.termux/files/usr/local/lib/";
                } else if (stat("/usr/local/lib", &st) == 0 && S_ISDIR(st.st_mode)) {
                        lib_or_lib32=1;
                        str_lib_path="/usr/local/lib";
                } else printf_error("Can't found ../usr/local/lib!");

                if (find_libpawnc == 1) {
                        snprintf(str_full_dest_path, sizeof(str_full_dest_path), "%s/libpawnc.so", str_lib_path);
                        watchdogs_sef_wmv(libpawnc_dest_path, str_full_dest_path);
                }

                if (strcmp(str_lib_path, "/usr/local/lib") == 0) {
                        int sys_sudo = system("which sudo > /dev/null 2>&1");
                        if (sys_sudo == 0) watchdogs_sys("sudo ldconfig");
                        else watchdogs_sys("ldconfig");

                        if (lib_or_lib32 == 1) {
                                const char
                                        *old_path_lib = getenv("LD_LIBRARY_PATH");
                                char
                                        new_path_lib[256];
                                if (old_path_lib) snprintf(new_path_lib, sizeof(new_path_lib), "/usr/local/lib:%s",
                                                                                                old_path_lib);
                                else snprintf(new_path_lib, sizeof(new_path_lib), "/usr/local/lib");
                                setenv("LD_LIBRARY_PATH", new_path_lib, 1);
                        } else if (lib_or_lib32 == 2) {
                                const char
                                        *old_path_lib32 = getenv("LD_LIBRARY_PATH");
                                char
                                        new_path_lib32[256];
                                if (old_path_lib32) snprintf(new_path_lib32, sizeof(new_path_lib32), "/usr/local/lib32:%s",
                                                                                                     old_path_lib32);
                                else snprintf(new_path_lib32, sizeof(new_path_lib32), "/usr/local/lib32");
                                setenv("LD_LIBRARY_PATH", new_path_lib32, 1);
                        }
                } else if (strcmp(str_lib_path, "/data/data/com.termux/files/usr/local/lib/") == 0) {
                        const char
                                *old_path_lib_tr = getenv("LD_LIBRARY_PATH");
                        char
                                new_path_lib_tr[256];
                        if (old_path_lib_tr) snprintf(new_path_lib_tr, sizeof(new_path_lib_tr), "/data/data/com.termux/files/usr/local/lib:%s",
                                                                                                old_path_lib_tr);
                        else snprintf(new_path_lib_tr, sizeof(new_path_lib_tr), "/data/data/com.termux/files/usr/local/lib");
                        setenv("LD_LIBRARY_PATH", new_path_lib_tr, 1);
                }
        }
#endif

        printf_color(COL_YELLOW, "apply finished!\n");
        __init(0);
}

