#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define __PATH_SYM "\\"
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
static int _w_chmod(const char *path) {
        int mode = _S_IREAD | _S_IWRITE;
        return chmod(path, mode);
}
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#define __PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
chmod(c_dest);
#endif

#include <sys/file.h>
#include <sys/types.h>
#include <ncursesw/curses.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <ftw.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "extra.h"
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
        __command_len =
            sizeof(__command) /
            sizeof(__command[0]);

__wd wcfg = {
        .ipcc = 0,
        .os = NULL,
        .__os__ = 0x00,
        .f_samp = 0x00,
        .f_openmp = 0x00,
        .pointer_samp = NULL,
        .pointer_openmp = NULL,
        .compiler_error = 0,
        .sef_count = 0,
        .sef_found = { {0} },
        .serv_dbg = NULL,
        .ci_options = NULL,
        .gm_input = NULL,
        .gm_output = NULL
};

void wd_sef_fdir_reset()
{
        size_t MAX_ENTRIES = sizeof(wcfg.sef_found) /
                             sizeof(wcfg.sef_found[0]);

        for (size_t i = 0;
             i < MAX_ENTRIES;
             ++i)
             wcfg.sef_found[i][0] = '\0';

        memset(wcfg.sef_found,
               0,
               sizeof(wcfg.sef_found));
        wcfg.sef_count = 0;
}

int wd_SignalOS(void) {
        if (strcmp(wcfg.os, "windows") == 0)
                return 0x01;
        else if (strcmp(wcfg.os, "linux") == 0)
                return 0x00;
        return 0x02;
}

static int __regex_v_unix(const char *s, char *badch, size_t *pos) {
        const char *forbidden = "&;|$`\\\"'<>";
        size_t i = 0;
        for (; *s; ++s, ++i) {
            unsigned char c = (unsigned char)*s;
            if (iscntrl(c)) {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (strchr(forbidden, c)) {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (c == '(' || c == ')' || c == '{' || c == '}' ||
                c == '[' || c == ']' || c == '*' || c == '?' || c == '~' ||
                c == '#'
            ) {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
        }
        return RETZ;
}

static int __regex_v_win(const char *s, char *badch, size_t *pos) {
        const char *forbidden = "&|<>^\"'`\\";
        size_t i = 0;
        for (; *s; ++s, ++i) {
            unsigned char c = (unsigned char)*s;
            if (iscntrl(c)) {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (strchr(forbidden, c)) {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
            if (c == '%' || c == '!' || c == ',' || c == ';' || c == '*'
                || c == '?' || c == '/') {
                if (badch) *badch = c;
                if (pos) *pos = i;
                return RETN;
            }
        }
        return RETZ;
}

static int __regex_check__(const char *cmd, char *badch, size_t *pos) {
#ifdef _WIN32
        return __regex_v_win(cmd, badch, pos);
#else
        return __regex_v_unix(cmd, badch, pos);
#endif
}

int wd_RunCommand(const char *cmd) {
        if (!cmd || !*cmd)
            return -RETN;

        char badch = 0;
        size_t pos = (size_t)-1;

        if (__regex_check__(cmd, &badch, &pos)) {
#if defined(_DBG_PRINT)
            if (isprint((unsigned char)badch)) {
                printf_warning("symbol detected in wd_RunCommand - char='%c' (0x%02X) at pos=%zu; cmd=\"%s\"",
                            badch, (unsigned char)badch, pos, cmd);
                char *continue_cmd;
                continue_cmd = readline("continue [y/n]: ");
                while (1) {
                    if (strcmp(continue_cmd, "Y") == 0 || strcmp(continue_cmd, "y") == 0) {
                        break;
                    } else if (strcmp(continue_cmd, "N") == 0 || strcmp(continue_cmd, "n") == 0) {
                        return -RETN;
                    } else {
                        return -RETN;
                    }
                }
            } else {
                printf_warning("control symbol detected in wd_RunCommand - char=0x%02X at pos=%zu; cmd=\"%s\"",
                            (unsigned char)badch, pos, cmd);
                char *continue_cmd;
                continue_cmd = readline("continue [y/n]: ");
                while (1) {
                    if (strcmp(continue_cmd, "Y") == 0 || strcmp(continue_cmd, "y") == 0) {
                        break;
                    } else if (strcmp(continue_cmd, "N") == 0 || strcmp(continue_cmd, "n") == 0) {
                        return -RETN;
                    } else {
                        return -RETN;
                    }
                }
            }
#endif
        }

        int ret = system(cmd);
        return ret;
}

void wd_SetPermission(const char *src, const char *tmp) {
        struct stat st;
        if (stat(src, &st) == 0) {
#ifdef _WIN32
            _w_chmod(tmp);
#else
            chmod(tmp, st.st_mode & 07777);
#endif
        }
}

inline void HANDLE_SIGINT(int sig)
{
        printf_color(COL_RED, "\n\tExit?, You only exit with use a \"exit\"\n");
        __main(0);
}

int wd_SetTitle(const char *__title) {
        const char
                *title = __title ? __title : "Watchdogs";
        printf("\033]0;%s\007", title);
        return RETZ;
}

void
wd_StripDotFns(char *dst, size_t dst_sz, const char *src) {
        if (!dst ||
            dst_sz == 0 ||
            !src) return;

        char *slash;
        slash = strchr(src, '/');
#ifdef _WIN32
        if (!slash)
            slash = strchr(src, '\\');
#endif
        if (slash == NULL) {
            char *dot;
            dot = strchr(src, '.');
            if (dot) {
                size_t len = (size_t)(dot - src);
                if (len >= dst_sz)
                    len = dst_sz - 1;
                memcpy(dst, src, len);
                dst[len] = '\0';
                return;
            }
        }

        snprintf(dst, dst_sz, "%s", src);
}

bool strfind(const char *text, const char *pattern) {
        size_t pat_len = strlen(pattern);
        for (size_t i = 0; text[i]; i++) {
            size_t j = 0;
            while (text[i + j] &&
                tolower((unsigned char)text[i + j]) ==
                    tolower((unsigned char)pattern[j]))
                j++;
            if (j == pat_len) {
                bool left_ok = (i == 0 ||
                    !isalnum((unsigned char)text[i - 1]));
                bool right_ok = !isalnum((unsigned char)text[i + j]);
                if (left_ok && right_ok)
                    return true;
            }
        }
        return false;
}

void
wd_EscapeQuotes(char *dest, size_t size, const char *src) {
        size_t j = 0;
        for (size_t i = 0;
            src[i] != '\0' &&
            j + 1 < size;
            i++) {
            if (src[i] == '"') {
                if (j + 2 >=
                    size)
                    break;
                dest[j++] = '\\';
                dest[j++] = '"';
            } else
                dest[j++] = src[i];
        }
        dest[j] = '\0';
}

static
void __SetPathSymS(char *out,
                   size_t out_sz,
                   const char *dir,
                   const char *name) {
        if (!out || out_sz == 0)
            return;
        size_t dir_len = strlen(dir);
        int __dir_hsp = (dir_len > 0 &&
                         IS_PATH_SYM(dir[dir_len - 1])),
            __name_hlsp = IS_PATH_SYM(name[0]);

        if (__dir_hsp) {
            if (__name_hlsp)
                snprintf(out, out_sz, "%s%s", dir, name + 1);
            else
                snprintf(out, out_sz, "%s%s", dir, name);
        } else {
#ifdef _WIN32
            if (__name_hlsp)
                snprintf(out, out_sz, "%s%s",
                    dir,
                    name);
            else snprintf(out, out_sz, "%s%s%s",
                    dir,
                    __PATH_SYM,
                    name);
#else
            if (__name_hlsp)
                snprintf(out, out_sz, "%s%s",
                    dir,
                    name);
            else snprintf(out, out_sz, "%s%s%s",
                    dir,
                    __PATH_SYM,
                    name);
#endif
        }
        out[out_sz - 1] = '\0';
}


int __CommandSuggest(const char *s1, const char *s2) {
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
                        curr[j] = (del < ins ? (del < sub ?
                                                del : sub)
                                             : (ins < sub ?
                                                ins : sub));
                }
                memcpy(prev, curr, (len2 + 1) * sizeof(int));
        }
        return prev[len2];
}

const char*
wd_FindNearCommand(
                    const char *ptr_command,
                    const char *__commands[],
                    size_t num_cmds,
    int *out_distance
) {
        int __b_distance = INT_MAX;
        const char *__b_cmd = NULL;

        for (size_t i = 0; i < num_cmds; i++) {
                int
                    dist = __CommandSuggest(ptr_command, __commands[i]);
                if (dist < __b_distance) {
                        __b_distance = dist;
                        __b_cmd = __commands[i];
                }
        }

        if (out_distance) *out_distance = __b_distance;
        return __b_cmd;
}

const char* wd_DetectOS(void) {
        static
            char os[64] = "Unknown's";

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
            else
                strncpy(os, sys_info.sysname, sizeof(os));
        }
#else
        strncpy(os, "windows", sizeof(os));
#endif

        os[sizeof(os)-1] = '\0';
        return os;
}

int dir_exists(const char *path) {
        struct stat st;
        return (stat(path, &st) == 0 &&
                S_ISDIR(st.st_mode));
}

int path_exists(const char *path) {
        struct stat st;
        return (stat(path, &st) == 0);
}

int dir_writable(const char *path) {
        if (access(path, W_OK) == 0)
            return RETN;
        return RETZ;
}

int path_acces(const char *path) {
        if (access(path, F_OK) == 0) {
            return RETN;
        }
        return RETZ;
}

int file_regular(const char *path) {
        struct stat st;
        if (stat(path, &st) != 0) return RETZ;
        return S_ISREG(st.st_mode);
}

int file_same_file(const char *a, const char *b) {
        struct stat sa, sb;
        if (stat(a, &sa) != 0) return RETZ;
        if (stat(b, &sb) != 0) return RETZ;
        return (sa.st_ino == sb.st_ino &&
                sa.st_dev == sb.st_dev);
}

int
ensure_parent_dir(char *out_parent,
                  size_t n,
                  const char *dest) {
        char tmp[PATH_MAX];
        if (strlen(dest) >= sizeof(tmp))
            return -RETN;
        strncpy(tmp, dest, sizeof(tmp));
        tmp[sizeof(tmp)-1] = '\0';
        char
            *parent = dirname(tmp);
        if (!parent)
            return -RETN;
        strncpy(out_parent, parent, n);
        out_parent[n-1] = '\0';

        return RETZ;
}

int cp_f_content(const char *src, const char *dst) {
        int infd = -1, outfd = -1;
        int ret = -1;
        char buf[PATH_MAX];
        ssize_t r;
        infd = open(src, O_RDONLY);
        if (infd < 0) goto out;
        outfd = open(dst, O_WRONLY |
                     O_CREAT |
                     O_TRUNC,
                     0600);
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
        if (r == 0)
            ret = 0;
out:
        if (infd >= 0)
            close(infd);
        if (outfd >= 0)
            close(outfd);

        return ret;
}

int kill_process(const char *name) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "pkill -9 -f \"%s\" > /dev/null 2>&1", name);
        return wd_RunCommand(cmd);
}

void kill_process_safe(const char *name) {
        if (name && strlen(name) > 0)
            kill_process(name);
}

int
wd_sef_fdir(const char *sef_path,
            const char *sef_name,
            const char *ignore_dir) {
        char _sef_path_buff[MAX_SEF_PATH_SIZE];

#ifdef _WIN32
        WIN32_FIND_DATA findFileData;
        HANDLE hFind;
        char _sef_search_path[MAX_PATH];

        if (sef_path[strlen(sef_path) - 1] == '\\')
            snprintf(_sef_search_path, sizeof(_sef_search_path), "%s*", sef_path);
        else
            snprintf(_sef_search_path, sizeof(_sef_search_path), "%s\\*", sef_path);

        hFind = FindFirstFile(_sef_search_path, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE)
            return RETZ;

        do {
            const char *name = findFileData.cFileName;
            if (name[0] == '.' &&
               (name[1] == '\0' ||
               (name[1] == '.' && 
               name[2] == '\0')))
               continue;

            __SetPathSymS(_sef_path_buff,
                         sizeof(_sef_path_buff),
                         sef_path,
                         name);

            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (ignore_dir && _stricmp(name, ignore_dir) == 0)
                    continue;
                if (wd_sef_fdir(_sef_path_buff, sef_name, ignore_dir)) {
                    FindClose(hFind);
                    return RETN;
                }
            } else {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (PathMatchSpecA(name, sef_name)) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                _sef_path_buff,
                                MAX_SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        FindClose(hFind);
                        return RETN;
                    }
                } else {
                    if (strcmp(name, sef_name) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                _sef_path_buff,
                                MAX_SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        FindClose(hFind);
                        return RETN;
                    }
                }
            }
        } while (FindNextFile(hFind, &findFileData));

        FindClose(hFind);
        return RETZ;
#else
        DIR *dir = opendir(sef_path);
        if (!dir)
            return RETZ;

        struct dirent *_entry;
        struct stat statbuf;

        while ((_entry = readdir(dir)) != NULL) {
            const char *name = _entry->d_name;
            if (name[0] == '.' &&
               (name[1] == '\0' ||
               (name[1] == '.' && 
               name[2] == '\0')))
               continue;

            __SetPathSymS(_sef_path_buff,
                         sizeof(_sef_path_buff),
                         sef_path,
                         name);

            if (_entry->d_type == DT_DIR) {
                if (ignore_dir && strcmp(name, ignore_dir) == 0)
                    continue;

                if (wd_sef_fdir(_sef_path_buff, sef_name, ignore_dir)) {
                    closedir(dir);
                    return RETN;
                }
            } else if (_entry->d_type == DT_REG) {
                if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                    if (fnmatch(sef_name, name, 0) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                _sef_path_buff,
                                MAX_SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        closedir(dir);
                        return RETN;
                    }
                } else {
                    if (strcmp(name, sef_name) == 0) {
                        strncpy(wcfg.sef_found[wcfg.sef_count],
                                _sef_path_buff,
                                MAX_SEF_PATH_SIZE);
                        wcfg.sef_count++;
                        closedir(dir);
                        return RETN;
                    }
                }
            } else {
                if (stat(_sef_path_buff, &statbuf) == -1)
                    continue;

                if (S_ISDIR(statbuf.st_mode)) {
                    if (ignore_dir && strcmp(name, ignore_dir) == 0)
                        continue;

                    if (wd_sef_fdir(_sef_path_buff, sef_name, ignore_dir)) {
                        closedir(dir);
                        return RETN;
                    }
                } else if (S_ISREG(statbuf.st_mode)) {
                    if (strchr(sef_name, '*') || strchr(sef_name, '?')) {
                        if (fnmatch(sef_name, name, 0) == 0) {
                            strncpy(wcfg.sef_found[wcfg.sef_count],
                                    _sef_path_buff,
                                    MAX_SEF_PATH_SIZE);
                            wcfg.sef_count++;
                            closedir(dir);
                            return RETN;
                        }
                    } else {
                        if (strcmp(name, sef_name) == 0) {
                            strncpy(wcfg.sef_found[wcfg.sef_count],
                                    _sef_path_buff,
                                    MAX_SEF_PATH_SIZE);
                            wcfg.sef_count++;
                            closedir(dir);
                            return RETN;
                        }
                    }
                }
            }
        }

        closedir(dir);
        return RETZ;
#endif
}

void __toml_base_subdirs(const char *base_path, FILE *toml_files, int *first)
{
#ifdef _WIN32
        WIN32_FIND_DATAA ffd;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        char __bs_search_path[MAX_PATH];

        snprintf(__bs_search_path, sizeof(__bs_search_path), "%s\\*", base_path);
        hFind = FindFirstFileA(__bs_search_path, &ffd);

        if (hFind == INVALID_HANDLE_VALUE)
            return;

        do {
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp(ffd.cFileName, ".") == 0 ||
                    strcmp(ffd.cFileName, "..") == 0)
                    continue;

                char path[MAX_PATH + 50];
                snprintf(path, sizeof(path), "%s/%s", base_path, ffd.cFileName);

                if (!*first)
                    fprintf(toml_files, ",\n        ");
                else {
                    fprintf(toml_files, "\n        ");
                    *first = 0;
                }

                fprintf(toml_files, "\"%s\"", path);
                __toml_base_subdirs(path, toml_files, first);
            }
        } while (FindNextFileA(hFind, &ffd) != 0);

        FindClose(hFind);
#else
        DIR *dir;
        struct dirent *_entry;

        if (!(dir = opendir(base_path)))
            return;

        while ((_entry = readdir(dir)) != NULL) {
            if (_entry->d_type == DT_DIR) {
                if (strcmp(_entry->d_name, ".") == 0 ||
                    strcmp(_entry->d_name, "..") == 0)
                    continue;

                char path[PATH_MAX + 50];
                snprintf(path, sizeof(path), "%s/%s", base_path, _entry->d_name);

                if (!*first)
                    fprintf(toml_files, ",\n        ");
                else {
                    fprintf(toml_files, "\n        ");
                    *first = 0;
                }

                fprintf(toml_files, "\"%s\"", path);
                __toml_base_subdirs(path, toml_files, first);
            }
        }

        closedir(dir);
#endif
}

int wd_SetToml(void)
{
        int __find_pawncc = 0;
        int __find_gamemodes = 0;
        int __pcc_is_compatible_opt = 0;

        const char *os_type = wd_DetectOS();
        int __os = 0x00;
        if (strcmp(os_type, "windows") == 0)
            __os = 0x01;
        else
            __os = 0x00;
        if (wcfg.f_samp == 0x01) {
__def:
            if (__os == 0x01)
                __find_pawncc = wd_sef_fdir("pawno", "pawncc.exe", NULL);
            else
                __find_pawncc = wd_sef_fdir("pawno", "pawncc", NULL);
        } else if (wcfg.f_openmp == 0x01) {
            if (__os == 0x01)
                __find_pawncc = wd_sef_fdir("qawno", "pawncc.exe", NULL);
            else
                __find_pawncc = wd_sef_fdir("qawno", "pawncc", NULL);
        } else {
            goto __def;
        }

        if (!__find_pawncc) {
__def2:
            if (__os == 0x01)
                __find_pawncc = wd_sef_fdir(".", "pawncc.exe", NULL);
            else
                __find_pawncc = wd_sef_fdir(".", "pawncc", NULL);

            if (!__find_pawncc)
                goto __def2;
        }

        __find_gamemodes = wd_sef_fdir("gamemodes/", "*.pwn", NULL);

        if (__find_pawncc)
        {
            char __run_pcc_sz[PATH_MAX + 258];
            snprintf(__run_pcc_sz, sizeof(__run_pcc_sz), "%s -___DDDDDDDDDDDDDDDDD" \
                                                            "-___DDDDDDDDDDDDDDDDD" \
                                                            "-___DDDDDDDDDDDDDDDDD" \
                                                            "-___DDDDDDDDDDDDDDDDD > .__CP.log 2>&1", wcfg.sef_found[0]);
            wd_RunCommand(__run_pcc_sz);

            FILE *procc_f;
            procc_f = fopen(".__CP.log", "r");
            if (procc_f) {
                char __cp_log_line[PATH_MAX];
                while (fgets(__cp_log_line, sizeof(__cp_log_line), procc_f) != NULL) {
                    if (strstr(__cp_log_line,
                        "         -Z[+/-]") || 
                        strfind(__cp_log_line,
                        "-Z[+/-]"))
                    {
                        __pcc_is_compatible_opt=1;
                        break;
                    } else {}
                }
                fclose(procc_f);
            } else {
                printf_error("failed to open .__CP.log");
            }
        }

        int _wd_log_acces = path_acces(".__CP.log");
        if (_wd_log_acces == 1)
            remove(".__CP.log");

        char __sef_path_sz[PATH_MAX];
        if (__find_pawncc)
            snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.sef_found[1]);
        else
            snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.sef_found[0]);
        char *f_EXT = strrchr(__sef_path_sz, '.');
        if (f_EXT) *f_EXT = '\0';
            
        FILE *toml_files = fopen("watchdogs.toml", "r");
        if (toml_files != NULL)
                fclose(toml_files);
        else {
                toml_files = fopen("watchdogs.toml", "w");
                if (toml_files != NULL) {
                    if (__find_gamemodes) {
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "\tos = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        if (__pcc_is_compatible_opt == 1)
                            fprintf(toml_files, "\toption = [\"-Z+\", \"-d3\", \"-;+\", \"-(+\"]\n");
                        else
                            fprintf(toml_files, "\toption = [\"-d3\", \"-;+\", \"-(+\"]\n");
                        fprintf(toml_files, "\tinclude_path = [");
                        int __fm = 1;
                        if (path_acces("gamemodes")) {
                            if (!__fm)
                                fprintf(toml_files, ",");
                            fprintf(toml_files, "\n        \"gamemodes\"");
                            __fm = 0;
                            __toml_base_subdirs("gamemodes", toml_files, &__fm);
                        }
                        if (wcfg.f_samp == 0x01) {
                            if (path_acces("pawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        else if (wcfg.f_openmp == 0x01) {
                            if (path_acces("qawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"qawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("qawno/include", toml_files, &__fm);
                            }
                        }
                        else {
                            if (path_acces("pawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        fprintf(toml_files, "\n    ]\n");
                        fprintf(toml_files, "\tinput = \"%s.pwn\"\n", __sef_path_sz);
                        fprintf(toml_files, "\toutput = \"%s.amx\"\n", __sef_path_sz);
                        fclose(toml_files);
                    }
                    else {
                        const char *os_type = wd_DetectOS();
                        fprintf(toml_files, "[general]\n");
                        fprintf(toml_files, "\tos = \"%s\"\n", os_type);
                        fprintf(toml_files, "[compiler]\n");
                        if (__pcc_is_compatible_opt == 1)
                            fprintf(toml_files, "\toption = [\"-Z+\", \"-d3\", \"-;+\", \"-(+\"]\n");
                        else
                            fprintf(toml_files, "\toption = [\"-d3\", \"-;+\", \"-(+\"]\n");
                        fprintf(toml_files, "\tinclude_path = [");
                        int __fm = 1;
                        if (path_acces("gamemodes")) {
                            if (!__fm)
                                fprintf(toml_files, ",");
                            fprintf(toml_files, "\n        \"gamemodes\"");
                            __fm = 0;
                            __toml_base_subdirs("gamemodes", toml_files, &__fm);
                        }
                        if (wcfg.f_samp == 0x01) {
                            if (path_acces("pawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        else if (wcfg.f_openmp == 0x01) {
                            if (path_acces("qawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"qawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("qawno/include", toml_files, &__fm);
                            }
                        }
                        else {
                            if (path_acces("pawno/include")) {
                                if (!__fm)
                                    fprintf(toml_files, ",");
                                fprintf(toml_files, "\n        \"pawno/include\"");
                                __fm = 0;
                                __toml_base_subdirs("pawno/include", toml_files, &__fm);
                            }
                        }
                        fprintf(toml_files, "\n    ]\n");
                        fprintf(toml_files, "\tinput = \"gamemodes/bare.pwn\"\n");
                        fprintf(toml_files, "\toutput = \"gamemodes/bare.amx\"\n");
                        fclose(toml_files);
                    }
                }
        }
        
        FILE *procc_f = fopen("watchdogs.toml", "r");
        if (!procc_f) printf_error("Can't a_read file %ss", "watchdogs.toml");

        char errbuf[256];
        toml_table_t *_toml_config;
        _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
        
        if (procc_f)
            fclose(procc_f);

        if (!_toml_config)
            printf_error("parsing TOML: %s", errbuf);

        toml_table_t *_wd_general = toml_table_in(_toml_config, "general");
        if (_wd_general) {
                toml_datum_t os_val = toml_string_in(_wd_general, "os");
                if (os_val.ok) wcfg.os = strdup(os_val.u.s);
                free(os_val.u.s);
                os_val.u.s = NULL;
        }

        toml_free(_toml_config);

        return RETZ;
}

int
__T_mv_with_o_sudo(const char *src, const char *dest) {
        if (rename(src, dest) == 0)
            return RETZ;
        
        if (errno == EXDEV ||
            errno == EACCES ||
            errno == EPERM)
        {
            if (!file_regular(src))
                return -RETN;

            char parent[PATH_MAX];
            if (ensure_parent_dir(parent, sizeof(parent), dest) != 0)
                return -RETN;
            if (!dir_writable(parent))
                return -RETW;

            char tmp_path[PATH_MAX];
            int rv = snprintf(tmp_path, sizeof(tmp_path),
                "%s/.tmp_wd_move_XXXXXX",
                parent);
            if (rv < 0 || (size_t)rv >= sizeof(tmp_path))
                return -RETN;
            int tmpfd = mkstemp(tmp_path);
            if (tmpfd < 0)
                return -RETN;
            
            close(tmpfd);
            
            if (cp_f_content(src, tmp_path) != 0) {
                    unlink(tmp_path);
                    return -RETN;
            }

            wd_SetPermission(src, tmp_path);
            
            if (rename(tmp_path, dest) != 0) {
                    unlink(tmp_path);
                    if (errno == EACCES || errno == EPERM)
                        return -RETW;
                    return -RETN;
            }
            if (unlink(src) != 0)
                return -3;
            
            return RETZ;
        }

        return -RETN;
}

int __T_cp_with_o_sudo(const char *src, const char *dest) {
        if (!file_regular(src))
            return -RETN;
        
        char parent[PATH_MAX];
        if (ensure_parent_dir(parent, sizeof(parent), dest) != 0)
            return -RETN;
        if (!dir_writable(parent))
            return -RETW;

        char tmp_path[PATH_MAX];
        int rv = snprintf(tmp_path, sizeof(tmp_path),
            "%s/.tmp_wd_copy_XXXXXX",
            parent);
        if (rv < 0 || (size_t)rv >= sizeof(tmp_path))
            return -RETN;
        int tmpfd = mkstemp(tmp_path);
        if (tmpfd < 0)
            return -RETN;
        
        close(tmpfd);
        
        if (cp_f_content(src, tmp_path) != 0) {
                unlink(tmp_path);
                return -RETN;
        }

        wd_SetPermission(src, tmp_path);
        
        if (rename(tmp_path, dest) != 0) {
                unlink(tmp_path);
                if (errno == EACCES || errno == EPERM)
                    return -RETW;
                return -RETN;
        }

        return RETZ;
}

#ifdef _WIN32

static int __mv_with_sudo(const char *src, const char *dest) {
        if (!MoveFileExA(src, dest, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
#if defined(_DBG_PRINT)
            printf_error("in __mv_with_sudo");
#endif
            return -RETN;
        }

        return RETZ;
}

static int __cp_with_sudo(const char *src, const char *dest) {
        if (!CopyFileA(src, dest, FALSE)) {
#if defined(_DBG_PRINT)
            printf_error("in __cp_with_sudo");
#endif
            return -RETN;
        }

        return RETZ;
}

#else

int __mv_with_sudo(const char *src, const char *dest) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -RETN;
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
                return -RETN;
            }
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                return RETN28 + WTERMSIG(status);
            else
                return -RETN;
        }
}

int __cp_with_sudo(const char *src, const char *dest) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return -RETN;
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
                return -RETN;
            }
            if (WIFEXITED(status))
                return WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                return RETN28 + WTERMSIG(status);
            else
                return -RETN;
        }
}

#endif

int __wd_sef_safety(const char *c_src, const char *c_dest)
{
	char parent[PATH_MAX];
	struct stat st;

	if (!c_src || !c_dest)
		printf_error("src or dest is null");

	if (!*c_src || !*c_dest)
		printf_error("src or dest empty");

	if (strlen(c_src) >= PATH_MAX || strlen(c_dest) >= PATH_MAX)
		printf_error("path too long");

	if (!path_exists(c_src))
		printf_error("source does not exist: %s", c_src);

	if (!file_regular(c_src))
		printf_error("source is not a regular file: %s", c_src);

	if (path_exists(c_dest) && file_same_file(c_src, c_dest)) {
		printf_info("source and dest are the same file: %s", c_src);
		return RETZ;
	}

	if (ensure_parent_dir(parent, sizeof(parent), c_dest))
		printf_error("cannot determine parent dir of dest");

	if (stat(parent, &st))
		printf_error("destination dir does not exist: %s", parent);

	if (!S_ISDIR(st.st_mode))
		printf_error("destination parent is not a dir: %s", parent);

	return RETN;
}

int wd_sef_wmv(const char *c_src, const char *c_dest)
{
	int ret, mv_ret, _mv_with_sudo;

	ret = __wd_sef_safety(c_src, c_dest);
	if (ret != 1)
		return RETN;

	mv_ret = __T_mv_with_o_sudo(c_src, c_dest);
	if (!mv_ret) {
#ifdef _WIN32
		if (_w_chmod(c_dest)) {
#if defined(_DBG_PRINT)
			printf_warning("chmod failed: %s (errno=%d %s)",
				       c_dest, errno, strerror(errno));
#endif
		}
#else
		if (chmod(c_dest, 0755)) {
#if defined(_DBG_PRINT)
			printf_warning("chmod failed: %s (errno=%d %s)",
				       c_dest, errno, strerror(errno));
#endif
		}
#endif
		printf_info("moved without sudo: %s -> %s", c_src, c_dest);
		return RETZ;
	}

	if (mv_ret == -2 || errno == EACCES || errno == EPERM) {
		printf_info("attempting sudo move due to permission issue");

		_mv_with_sudo = __mv_with_sudo(c_src, c_dest);
		if (!_mv_with_sudo) {
			printf_info("moved with sudo: %s -> %s", c_src, c_dest);
			return RETZ;
		}

		printf_error("sudo mv failed with code %d", _mv_with_sudo);
		return RETN;
	}

	if (mv_ret == -3) {
		printf_warning("move partially succeeded (copied but couldn't unlink source): %s", c_dest);
		return RETZ;
	}

	printf_error("move without sudo failed (errno=%d %s)", errno, strerror(errno));
	printf_info("attempting sudo as last resort");

	_mv_with_sudo = __mv_with_sudo(c_src, c_dest);
	if (!_mv_with_sudo)
		return RETZ;

	printf_error("sudo mv failed with code %d", _mv_with_sudo);
	return RETN;
}

int wd_sef_wcopy(const char *c_src, const char *c_dest)
{
	int ret, cp_ret, _cp_with_sudo;

	ret = __wd_sef_safety(c_src, c_dest);
	if (ret != 1)
		return RETN;

	cp_ret = __T_cp_with_o_sudo(c_src, c_dest);
	if (!cp_ret) {
#ifdef _WIN32
		if (_w_chmod(c_dest)) {
#if defined(_DBG_PRINT)
			printf_warning("chmod failed: %s (errno=%d %s)",
				       c_dest, errno, strerror(errno));
#endif
		}
#else
		if (chmod(c_dest, 0755)) {
#if defined(_DBG_PRINT)
			printf_warning("chmod failed: %s (errno=%d %s)",
				       c_dest, errno, strerror(errno));
#endif
		}
#endif
		printf_info("copied without sudo: %s -> %s", c_src, c_dest);
		return RETZ;
	}

	if (cp_ret == -2 || errno == EACCES || errno == EPERM) {
		printf_info("attempting sudo copy due to permission issue");

		_cp_with_sudo = __cp_with_sudo(c_src, c_dest);
		if (!_cp_with_sudo) {
			printf_info("copied with sudo: %s -> %s", c_src, c_dest);
			return RETZ;
		}

		printf_error("sudo cp failed with code %d", _cp_with_sudo);
		return RETN;
	}

	printf_error("copy without sudo failed (errno=%d %s)", errno, strerror(errno));
	printf_info("attempting sudo as last resort");

	_cp_with_sudo = __cp_with_sudo(c_src, c_dest);
	if (!_cp_with_sudo)
		return RETZ;

	printf_error("sudo cp failed with code %d", _cp_with_sudo);
	return RETN;
}
