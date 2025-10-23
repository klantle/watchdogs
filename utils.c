#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define PATH_SEP "\\"
#define IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
static int w_chmo(const char *path) {
    int mode = _S_IREAD | _S_IWRITE;
    return chmod(path, mode);
}
#else
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fnmatch.h>
#define PATH_SEP "/"
#define IS_PATH_SEP(c) ((c) == '/')
chmod(c_dest);
#endif

#include <ncursesw/curses.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <ftw.h>
#include <sys/file.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/types.h>
#include <libgen.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "utils.h"
#include "chain.h"

const char* __command[] = {
        "help",
        "clear",
        "exit",
        "kill",
        "title",
        "toml",
        "hardware",
        "gamemode",
        "pawncc",
        "compile",
        "running",
        "crunn",
        "debug",
        "stop",
        "restart"
};
const size_t
    __command_len = sizeof(__command) / sizeof(__command[0]);

inline void watch_reset_var(void) {
        wd wcfg = {0};
}

inline int watch_sys(const char *cmd) {
        return system(cmd);
}

void __give_permissions(const char *src, const char *tmp_path) {
        struct stat st;
        if (stat(src, &st) == 0) {
#ifdef _WIN32
            DWORD attr = GetFileAttributesA(src);
            if (attr != INVALID_FILE_ATTRIBUTES) {
                if (attr & FILE_ATTRIBUTE_READONLY)
                    SetFileAttributesA(tmp_path, attr & ~FILE_ATTRIBUTE_READONLY);
                else
                    SetFileAttributesA(tmp_path, attr);
            }
#else
            chmod(tmp_path, st.st_mode & 07777);
#endif
        }
}

wd wcfg = {
        .ipcc = 0,
        .os = NULL,
        .sef_count = 0,
        .sef_found = { {0} },
        .serv_dbg = NULL,
        .ci_options = NULL,
        .gm_input = NULL,
        .g_output = NULL
};

void reset_watch_sef_dir()
{
        size_t MAX_ENTRIES = sizeof(wcfg.sef_found) /
                             sizeof(wcfg.sef_found[0]);

        for (size_t i = 0;
             i < MAX_ENTRIES;
             ++i)
             wcfg.sef_found[i][0] = '\0';

        memset(wcfg.sef_found, 0, sizeof(wcfg.sef_found));
        wcfg.sef_count = 0;
}

inline void handle_sigint(int sig)
{
        println("Exit?, You only exit with use a \"exit\"");
        __init(0);
}

inline int watch_title(const char *__title)
{
        const char
                *title = __title ? __title : "Watchdogs";
        printf("\033]0;%s\007", title);
        return 0;
}

void copy_strip_dotfns(char *dst, size_t dst_sz, const char *src) {
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
        int len1 = strlen(s1),
            len2 = strlen(s2);
        if (len2 > 128) return INT_MAX;
        int prev[129],
            curr[129];
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

const char* find_cl_command(
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

const char* watch_detect_os(void) {
        static char os[64] = "Unknown's";

        if ((getenv("OS") &&
             strstr(getenv("OS"), "Windows_NT")) ||
            getenv("WSL_INTEROP") ||
            getenv("WSL_DISTRO_NAME"))
            return "windows";

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
        if (strcmp(wcfg.os, "windows") == 0)
                return 0x01;
        else if (strcmp(wcfg.os, "linux") == 0)
                return 0x00;
        
        return 0;
}

int dir_exists(const char *path) {
        struct stat st;
        return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int path_exists(const char *path) {
        struct stat st;
        return (stat(path, &st) == 0);
}

int dir_writable(const char *path) {
        if (access(path, W_OK) == 0)
            return 1;
        return 0;
}

int is_regular_file(const char *path) {
        struct stat st;
        if (stat(path, &st) != 0) return 0;
        return S_ISREG(st.st_mode);
}

int is_same_file(const char *a, const char *b) {
        struct stat sa, sb;
        if (stat(a, &sa) != 0) return 0;
        if (stat(b, &sb) != 0) return 0;
        return (sa.st_ino == sb.st_ino &&
                sa.st_dev == sb.st_dev);
}

int ensure_parent_dir(char *out_parent, size_t n, const char *dest) {
        char tmp[PATH_MAX];
        if (strlen(dest) >= sizeof(tmp))
            return -1;
        strncpy(tmp, dest, sizeof(tmp));
        tmp[sizeof(tmp)-1] = '\0';
        char *parent = dirname(tmp);
        if (!parent)
            return -1;
        strncpy(out_parent, parent, n);
        out_parent[n-1] = '\0';

        return 0;
}

int cp_f_content(const char *src, const char *dst) {
        int infd = -1, outfd = -1;
        int ret = -1;
        char buf[8192];
        ssize_t r;
        infd = open(src, O_RDONLY);
        if (infd < 0) goto out;
        outfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (outfd < 0) goto out;
        while ((r = read(infd, buf, sizeof(buf))) > 0) {
            ssize_t w = 0;
            char *p = buf;
            while (r > 0) {
                w = write(outfd, p, r);
                if (w < 0) goto out;
                r -= w;
                p += w;
            }
        }
        if (r == 0) ret = 0;
out:
        if (infd >= 0) close(infd);
        if (outfd >= 0) close(outfd);
        return ret;
}

int kill_process(const char *name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "pkill -9 -f \"%s\" > /dev/null 2>&1", name);
        return system(cmd);
}

void kill_process_safe(const char *name) {
    if (name && strlen(name) > 0) kill_process(name);
}

void __toml_base_subdirs(const char *base_path, FILE *toml_files, int *is_first)
{
#ifdef _WIN32
        WIN32_FIND_DATAA ffd;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char search_path[MAX_PATH];

        snprintf(search_path, sizeof(search_path), "%s\\*", base_path);
        hFind = FindFirstFileA(search_path, &ffd);

        if (hFind == INVALID_HANDLE_VALUE)
            return;

        do {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
                    continue;

                char path[MAX_PATH];
                snprintf(path, sizeof(path), "%s/%s", base_path, ffd.cFileName);

                if (!*is_first)
                    fprintf(toml_files, ",\n        ");
                else {
                    fprintf(toml_files, "\n        ");
                    *is_first = 0;
                }

                fprintf(toml_files, "\"%s\"", path);
                __toml_base_subdirs(path, toml_files, is_first);
            }
        } while (FindNextFileA(hFind, &ffd) != 0);

        FindClose(hFind);
#else
        DIR *dir;
        struct dirent *entry;

        if (!(dir = opendir(base_path)))
            return;

        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                char path[PATH_MAX];
                snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

                if (!*is_first)
                    fprintf(toml_files, ",\n        ");
                else {
                    fprintf(toml_files, "\n        ");
                    *is_first = 0;
                }

                fprintf(toml_files, "\"%s\"", path);
                __toml_base_subdirs(path, toml_files, is_first);
            }
        }

        closedir(dir);
#endif
}

int watch_toml_data(void)
{
        const char *fname = "watchdogs.toml";
        FILE *toml_files;

        int find_gamemodes = watch_sef_fdir("gamemodes/", "*.pwn");

        char i_path_rm[PATH_MAX];
        snprintf(i_path_rm, sizeof(i_path_rm), "%s", wcfg.sef_found[0]);
        char *f_EXT = strrchr(i_path_rm, '.');
        if (f_EXT && strcmp(f_EXT, ".pwn") == 0)
            *f_EXT = '\0';
            
        toml_files = fopen(fname, "r");
        if (toml_files != NULL)
                fclose(toml_files);
        else {
                toml_files = fopen(fname, "w");
                if (toml_files != NULL) {
                    if (find_gamemodes) {
                        const char *os_type = watch_detect_os();
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "\tos = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        fprintf(toml_files, "\toption = [\"-d3\", \"-Z+\"]\n");
                        fprintf(toml_files, "\tinclude_path = [");
                        int __fm = 1;
                        if (access("gamemodes", F_OK) == 0) {
                            if (!__fm)
                                fprintf(toml_files, ",");
                            fprintf(toml_files, "\n        \"gamemodes\"");
                            __fm = 0;
                            __toml_base_subdirs("gamemodes", toml_files, &__fm);
                        }
                        if (find_for_samp == 0x01) {
                            if (access("pawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        else if (find_for_omp == 0x01) {
                            if (access("qawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"qawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("qawno/include", toml_files, &__fm);
                            }
                        }
                        else {
                            if (access("pawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        fprintf(toml_files, "\n    ]\n");
                        fprintf(toml_files, "\tinput = \"%s.pwn\"\n", i_path_rm);
                        fprintf(toml_files, "\toutput = \"%s.amx\"\n", i_path_rm);
                        fclose(toml_files);
                    }
                    else {
                        const char *os_type = watch_detect_os();
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "\tos = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        fprintf(toml_files, "\toption = [\"-d3\", \"-Z+\"]\n");
                        fprintf(toml_files, "\tinclude_path = [");
                        int __fm = 1;
                        if (access("gamemodes", F_OK) == 0) {
                            if (!__fm)
                                fprintf(toml_files, ",");
                            fprintf(toml_files, "\n        \"gamemodes\"");
                            __fm = 0;
                            __toml_base_subdirs("gamemodes", toml_files, &__fm);
                        }
                        if (find_for_samp == 0x01) {
                            if (access("pawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        else if (find_for_omp == 0x01) {
                            if (access("qawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"qawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("qawno/include", toml_files, &__fm);
                            }
                        }
                        else {
                            if (access("pawno/include", F_OK) == 0) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        fprintf(toml_files, "\n    ]\n");
                        fprintf(toml_files, "]\n");
                        fprintf(toml_files, "\tinput = \"main.pwn\"\n");
                        fprintf(toml_files, "\toutput = \"main.amx\"\n");
                        fclose(toml_files);
                    }
                }
        }
        FILE *procc_f = fopen(fname, "r");
        if (!procc_f) printf_error("can't a_read file %ss\n", fname);

        char errbuf[256];
        toml_table_t *config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
        fclose(procc_f);

        if (!config) printf_error("parsing TOML: %s\n", errbuf);

        toml_table_t *_watch_general = toml_table_in(config, "general");
        if (_watch_general) {
                toml_datum_t os_val = toml_string_in(_watch_general, "os");
                if (os_val.ok) wcfg.os = strdup(os_val.u.s);
                free(os_val.u.s);
        }

        toml_free(config);

        return 0;
}

static void __join_path(char *out,
                        size_t out_sz,
                        const char *dir,
                        const char *name) {
        if (!out || out_sz == 0)
            return;
        size_t dir_len = strlen(dir);
        int __dir_hsp = (dir_len > 0 && IS_PATH_SEP(dir[dir_len - 1])),
            __name_hlsp = IS_PATH_SEP(name[0]);

        if (__dir_hsp) {
            if (__name_hlsp) snprintf(out, out_sz, "%s%s", dir, name + 1);
            else snprintf(out, out_sz, "%s%s", dir, name);
        } else {
#ifdef _WIN32
            if (__name_hlsp) snprintf(out, out_sz, "%s%s", dir, name);
            else snprintf(out, out_sz, "%s%s%s", dir, PATH_SEP, name);
#else
            if (__name_hlsp) snprintf(out, out_sz, "%s%s", dir, name);
            else snprintf(out, out_sz, "%s%s%s", dir, PATH_SEP, name);
#endif
        }
        out[out_sz - 1] = '\0';
}

int watch_sef_fdir(const char *sef_path, const char *sef_name) {
        char path_buff[SEF_PATH_SIZE];

#ifdef _WIN32
        WIN32_FIND_DATA findFileData;
        HANDLE hFind;
        char searchPath[MAX_PATH];

        if (sef_path[strlen(sef_path)-1] == '\\') {
            snprintf(searchPath, sizeof(searchPath), "%s*", sef_path);
        } else {
            snprintf(searchPath, sizeof(searchPath), "%s%s*", sef_path, PATH_SEP);
        }

        hFind = FindFirstFile(searchPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            return 0;
        }
        do {
            const char *name = findFileData.cFileName;
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            __join_path(path_buff, sizeof(path_buff), sef_path, name);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (watch_sef_fdir(path_buff, sef_name)) {
                    FindClose(hFind);
                    return 1;
                }
            } else {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (PathMatchSpecA(name, sef_name)) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        FindClose(hFind);
                        return 1;
                    }
                } else {
                    if (strcmp(name, sef_name) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.sef_count++;
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

            __join_path(path_buff, sizeof(path_buff), sef_path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                if (watch_sef_fdir(path_buff, sef_name)) {
                    closedir(dir);
                    return 1;
                }
            } else if (entry->d_type == DT_REG) {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (fnmatch(sef_name, entry->d_name, 0) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        closedir(dir);
                        return 1;
                    }
                } else {
                    if (strcmp(entry->d_name, sef_name) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                path_buff, SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        closedir(dir);
                        return 1;
                    }
                }
            } else {
                if (stat(path_buff, &statbuf) == -1)
                    continue;

                if (S_ISDIR(statbuf.st_mode)) {
                    if (watch_sef_fdir(path_buff, sef_name)) {
                        closedir(dir);
                        return 1;
                    }
                } else if (S_ISREG(statbuf.st_mode)) {
                    if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                        if (fnmatch(sef_name, entry->d_name, 0) == 0) {
                            strncpy(wcfg.sef_found[wcfg.sef_count],
                                    path_buff, SEF_PATH_SIZE);
                            wcfg.sef_count++;
                            closedir(dir);
                            return 1;
                        }
                    } else {
                        if (strcmp(entry->d_name, sef_name) == 0) {
                            strncpy(wcfg.sef_found[wcfg.sef_count],
                                    path_buff, SEF_PATH_SIZE);
                            wcfg.sef_count++;
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

int __try_mv_wout_sudo(const char *src, const char *dest) {
        if (rename(src, dest) == 0) return 0;
        if (errno == EXDEV || errno == EACCES || errno == EPERM) {
                if (!is_regular_file(src)) return -1;
                char parent[PATH_MAX];
                if (ensure_parent_dir(parent, sizeof(parent), dest) != 0) return -1;
                if (!dir_writable(parent)) return -2;
                char tmp_path[PATH_MAX];
                int rv = snprintf(tmp_path, sizeof(tmp_path), "%s/.tmp_watch_move_XXXXXX", parent);
                if (rv < 0 || (size_t)rv >= sizeof(tmp_path)) return -1;
                int tmpfd = mkstemp(tmp_path);
                if (tmpfd < 0) return -1;
                close(tmpfd);
                if (cp_f_content(src, tmp_path) != 0) {
                        unlink(tmp_path);
                        return -1;
                }
                __give_permissions(src, tmp_path);
                if (rename(tmp_path, dest) != 0) {
                        unlink(tmp_path);
                        if (errno == EACCES || errno == EPERM) return -2;
                        return -1;
                }
                if (unlink(src) != 0) return -3;
                return 0;
        }
        return -1;
}

int __try_cp_wout_sudo(const char *src, const char *dest) {
        if (!is_regular_file(src)) return -1;
        char parent[PATH_MAX];
        if (ensure_parent_dir(parent, sizeof(parent), dest) != 0) return -1;
        if (!dir_writable(parent)) return -2;
        char tmp_path[PATH_MAX];
        int rv = snprintf(tmp_path, sizeof(tmp_path), "%s/.tmp_watch_copy_XXXXXX", parent);
        if (rv < 0 || (size_t)rv >= sizeof(tmp_path)) return -1;
        int tmpfd = mkstemp(tmp_path);
        if (tmpfd < 0) return -1;
        close(tmpfd);
        if (cp_f_content(src, tmp_path) != 0) {
                unlink(tmp_path);
                return -1;
        }
        __give_permissions(src, tmp_path);
        if (rename(tmp_path, dest) != 0) {
                unlink(tmp_path);
                if (errno == EACCES || errno == EPERM) return -2;
                return -1;
        }
        return 0;
}

#ifdef _WIN32

static int __mv_with_sudo(const char *src, const char *dest) {
        if (!MoveFileExA(src, dest, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
            DWORD err = GetLastError();
            fprintf(stderr, "MoveFileEx failed with error %lu\n", err);
            return -1;
        }
        return 0;
}

static int __cp_with_sudo(const char *src, const char *dest) {
        if (!CopyFileA(src, dest, FALSE /*overwrite*/)) {
            DWORD err = GetLastError();
            fprintf(stderr, "CopyFile failed with error %lu\n", err);
            return -1;
        }
        return 0;
}

#else

static int __mv_with_sudo(const char *src, const char *dest) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        } else if (pid == 0) {
            char *argv[5];
            argv[0] = "sudo";
            argv[1] = "mv";
            argv[2] = (char *)src;
            argv[3] = (char *)dest;
            argv[4] = NULL;
            execvp("sudo", argv);
            perror("execvp sudo");
            _exit(127);
        } else {
            int status;
            if (waitpid(pid, &status, 0) < 0) {
                perror("waitpid");
                return -1;
            }
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return -1;
            }
        }
}

static int __cp_with_sudo(const char *src, const char *dest) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -1;
        } else if (pid == 0) {
            char *argv[5];
            argv[0] = "sudo";
            argv[1] = "cp";
            argv[2] = (char *)src;
            argv[3] = (char *)dest;
            argv[4] = NULL;
            execvp("sudo", argv);
            perror("execvp sudo");
            _exit(127);
        } else {
            int status;
            if (waitpid(pid, &status, 0) < 0) {
                perror("waitpid");
                return -1;
            }
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            } else {
                return -1;
            }
        }
}

#endif

int __aio_sef_safety(const char *c_src, const char *c_dest)
{
        if (!c_src || !c_dest)
            printf_error("src or dest is null\n");
        if (strlen(c_src) == 0 || strlen(c_dest) == 0)
            printf_error("src or dest empty\n");
        if (strlen(c_src) >= PATH_MAX || strlen(c_dest) >= PATH_MAX)
            printf_error("path too long\n");
        if (!path_exists(c_src))
            printf_error("source does not exist: %s\n", c_src);
        if (!is_regular_file(c_src))
            printf_error("source is not a regular file: %s\n", c_src);
        if (path_exists(c_dest)) {
            if (is_same_file(c_src, c_dest)) {
                printf_info("source and dest are the same file: %s\n", c_src);
                return 0;
            }
        } else {
            char parent[PATH_MAX];
            if (ensure_parent_dir(parent, sizeof(parent), c_dest) != 0)
                printf_error("cannot determine parent directory of dest\n");
            struct stat st;
            if (stat(parent, &st) != 0)
                printf_error("destination directory does not exist: %s\n", parent);
            if (!S_ISDIR(st.st_mode))
                printf_error("destination parent is not a directory: %s\n", parent);
        }

        return 1;
}

int watch_sef_wmv(const char *c_src, const char *c_dest) {
        int ret = __aio_sef_safety(c_src, c_dest);
        if (ret == 1) { return 1; }

        int mv_ret = __try_mv_wout_sudo(c_src, c_dest);
        if (mv_ret == 0) {
#ifdef _WIN32
            if (w_chmo(c_dest) != 0)
#ifdef WD_DEBUGGING
                printf_warning("chmod failed: %s (errno=%d %s)\n", c_dest, errno, strerror(errno));
#endif
#else
            if (chmod(c_dest, 0755) != 0)
#ifdef WD_DEBUGGING
                printf_warning("chmod failed: %s (errno=%d %s)\n", c_dest, errno, strerror(errno));
#endif
#endif
            printf_info("moved without sudo: %s -> %s\n", c_src, c_dest);
            return 0;
        } else {
            if (mv_ret == -2 || errno == EACCES || errno == EPERM) {
                printf_info("attempting sudo move due to permission issue\n");
                int sudo_rc = __mv_with_sudo(c_src, c_dest);
                if (sudo_rc == 0) {
                    printf_info("moved with sudo: %s -> %s\n", c_src, c_dest);
                    return 0;
                } else {
                    printf_error("sudo mv failed with code %d\n", sudo_rc);
                    return 1;
                }
            } else if (mv_ret == -3) {
                printf_warning("move partially succeeded (copied but couldn't unlink source). dest=%s\n", c_dest);
                return 0;
            } else {
                printf_error("move without sudo failed (errno=%d %s)\n", errno, strerror(errno));
                printf_info("attempting sudo as last resort\n");
                int sudo_rc = __mv_with_sudo(c_src, c_dest);
                if (sudo_rc == 0) return 0;
                printf_error("sudo mv failed with code %d\n", sudo_rc);
                return 1;
            }
        }
}

int watch_sef_wcopy(const char *c_src,
                        const char *c_dest)
{
        int ret = __aio_sef_safety(c_src, c_dest);
        if (ret == 1) { return 1; }

        if (path_exists(c_dest)) {
            if (is_same_file(c_src, c_dest)) {
                printf_info("source and dest are the same file: %s\n", c_src);
                return 0;
            }
        } else {
            char parent[PATH_MAX];
            if (ensure_parent_dir(parent, sizeof(parent), c_dest) != 0) {
                printf_error("cannot determine parent directory of dest\n");
                return 1;
            }
            struct stat st;
            if (stat(parent, &st) != 0) {
                printf_error("destination directory does not exist: %s\n", parent);
                return 1;
            }
            if (!S_ISDIR(st.st_mode)) {
                printf_error("destination parent is not a directory: %s\n", parent);
                return 1;
            }
        }
	
        int cp_ret = __try_cp_wout_sudo(c_src, c_dest);
        if (cp_ret == 0) {
#ifdef _WIN32
            if (w_chmo(c_dest) != 0)
#ifdef WD_DEBUGGING
                printf_warning("chmod failed: %s (errno=%d %s)\n", c_dest, errno, strerror(errno));
#endif
#else
            if (chmod(c_dest, 0755) != 0)
#ifdef WD_DEBUGGING
                printf_warning("chmod failed: %s (errno=%d %s)\n", c_dest, errno, strerror(errno));
#endif
#endif
            printf_info("copied without sudo: %s -> %s\n", c_src, c_dest);
            return 0;
        } else {
            if (cp_ret == -2 || errno == EACCES || errno == EPERM) {
                printf_info("attempting sudo copy due to permission issue\n");
                int sudo_rc = __cp_with_sudo(c_src, c_dest);
                if (sudo_rc == 0) {
                    printf_info("copied with sudo: %s -> %s\n", c_src, c_dest);
                    return 0;
                } else {
                    printf_error("sudo cp failed with code %d\n", sudo_rc);
                    return 1;
                }
            } else {
                printf_error("copy without sudo failed (errno=%d %s)\n", errno, strerror(errno));
                printf_info("attempting sudo as last resort\n");
                int sudo_rc = __cp_with_sudo(c_src, c_dest);
                if (sudo_rc == 0) return 0;
                printf_error("sudo cp failed with code %d\n", sudo_rc);
                return 1;
            }
        }

        return 0;
}

void
install_pawncc_now(void) {
        int __os__ = signal_system_os();
        int find_pawncc_exe = watch_sef_fdir(".", "pawncc.exe"),
            find_pawncc = watch_sef_fdir(".", "pawncc"),
            find_pawndisasm_exe = watch_sef_fdir(".", "pawndisasm.exe"),
            find_pawndisasm = watch_sef_fdir(".", "pawndisasm");

        int dir_pawno=0,
            dir_qawno=0;
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

        char pawncc_dest_path[512] = {0},
             pawncc_exe_dest_path[512] = {0},
             pawndisasm_dest_path[512] = {0},
             pawndisasm_exe_dest_path[512] = {0};

        for (int i = 0; i < wcfg.sef_count; i++) {
            const char *entry = wcfg.sef_found[i];
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
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s",
            						   dest_path,
            						   PATH_SEP,
            						   "pawncc.exe");
            watch_sef_wmv(pawncc_exe_dest_path, str_dest_path);
        }
        if (find_pawncc) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s",
            						   dest_path,
            						   PATH_SEP,
            						   "pawncc");
            watch_sef_wmv(pawncc_dest_path, str_dest_path);
        }
        if (find_pawndisasm_exe) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s",
            						   dest_path,
            						   PATH_SEP,
            						   "pawndisasm.exe");
            watch_sef_wmv(pawndisasm_exe_dest_path, str_dest_path);
        }
        if (find_pawndisasm) {
            snprintf(str_dest_path, sizeof(str_dest_path), "%s%s%s",
            						   dest_path,
            						   PATH_SEP,
            						   "pawndisasm");
            watch_sef_wmv(pawndisasm_dest_path, str_dest_path);
        }

#ifndef _WIN32
        if (__os__ == 0x00) {
                char *str_lib_path = NULL,
                     str_full_dest_path[128];

                int find_libpawnc = watch_sef_fdir(".", "libpawnc.so");
                char libpawnc_dest_path[1024];
                for (int i = 0; i < wcfg.sef_count; i++) {
                        if (strstr(wcfg.sef_found[i], "libpawnc.so")) {
                                snprintf(libpawnc_dest_path, sizeof(libpawnc_dest_path), "%s",
                                                                                                wcfg.sef_found[i]);
                                break;
                        }
                }

                struct stat st;
                int lib_or_lib32 = 0;
                if (!stat("/usr/local/lib32", &st) && S_ISDIR(st.st_mode)) {
                        lib_or_lib32=2;
                        str_lib_path="/usr/local/lib32";
                } else if (!stat("/data/data/com.termux/files/usr/local/lib/", &st) && S_ISDIR(st.st_mode)) {
                        str_lib_path="/data/data/com.termux/files/usr/local/lib/";
                } else if (!stat("/data/data/com.termux/files/usr/lib/", &st) && S_ISDIR(st.st_mode)) {
                        str_lib_path="/data/data/com.termux/files/usr/lib/";
                } else if (!stat("/usr/local/lib", &st) && S_ISDIR(st.st_mode)) {
                        lib_or_lib32=1;
                        str_lib_path="/usr/local/lib";
                } else printf_error("Can't found ../usr/local/lib!");

                if (find_libpawnc == 1) {
                        snprintf(str_full_dest_path, sizeof(str_full_dest_path), "%s/libpawnc.so", str_lib_path);
                        watch_sef_wmv(libpawnc_dest_path, str_full_dest_path);
                }

                if (strcmp(str_lib_path, "/usr/local/lib") == 0) {
                        int sys_sudo = system("which sudo > /dev/null 2>&1");
                        if (sys_sudo == 0) watch_sys("sudo ldconfig");
                        else watch_sys("ldconfig");

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
                } else {
                        const char
                                *old_path_lib_tr = getenv("LD_LIBRARY_PATH");
                        char
                                new_path_lib_tr[256];
                        if (old_path_lib_tr) snprintf(new_path_lib_tr, sizeof(new_path_lib_tr), "%s:%s",
                                                                                                str_lib_path,
                                                                                                old_path_lib_tr);
                        else snprintf(new_path_lib_tr, sizeof(new_path_lib_tr), "%s", str_lib_path);
                        setenv("LD_LIBRARY_PATH", new_path_lib_tr, 1);
                }
        }
#endif

        printf_color(COL_YELLOW, "apply finished!\n");
        __init(0);
}

