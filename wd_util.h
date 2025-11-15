#ifndef UTILS_H
#define UTILS_H

#define __WATCHDOGS__
#ifdef __WATCHDOGS__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "include/cJSON/cJSON.h"
#include "include/tomlc/toml.h"

#ifdef _WIN32
#define WD_WINDOWS
#elif defined(__ANDROID__)
#define WD_ANDROID
#elif defined(__linux__)
#define WD_LINUX
#endif

#ifdef WD_WINDOWS
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define __PATH_SEP "\\"
#define IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
#define mkdir(wx) _mkdir(wx)
#define MKDIR(wx) mkdir(wx)
#define Sleep(sec) Sleep((sec)*1000)
#define sleep(wx) Sleep(wx)
#define setenv(wx,wy,wz) _putenv_s(wx,wy)
#define SETEN(wx,wy,wz) setenv(wx,wy)
#define win32_chmod(path) \
({ \
    const char *_path = (path); \
    int _mode = _S_IREAD | _S_IWRITE; \
    chmod(_path, _mode); \
})
#define open _open
#define read _read
#define close _close
#define O_RDONLY _O_RDONLY
#define CHMOD(wx, wy) _chmod(wx, wy)
#define FILE_MODE _S_IREAD | _S_IWRITE
#define getcwd _getcwd
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fnmatch.h>
#define __PATH_SEP "/"
#define IS_PATH_SEP(c) ((c) == '/')
#define MKDIR(wx) mkdir(wx,0755)
#define SETENV(wx,wy,wz) setenv(wx,wy,wz)
#define CHMOD(wx,wy) chmod(wx,wy)
#define FILE_MODE 0777
#endif

#if __has_include(<readline/history.h>)
#include <readline/history.h>
#define wd_u_history() using_history()
#define wd_a_history(cmd) add_history(cmd)
#else
#define wd_u_history()
#define wd_a_history(cmd)
#endif

#define wd_malloc(wx) malloc(wx)
#define wd_calloc(wx, wy) calloc(wx, wy)
#define wd_realloc(wx, wy) realloc(wx, wy)
#define wd_free(wx) free(wx)

#define __PATH_CHR_SEP_LINUX '/'
#define __PATH_CHR_SEP_WIN32 '\\'

enum {
	WD_RETZ = 0,
	WD_RETN = 1,
	WD_RETW = 2,
	WD_RETH = 3
};

#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
#endif

#ifdef PATH_MAX
#define WD_PATH_MAX PATH_MAX
#else
#define WD_PATH_MAX 260
#endif
#define WD_MAX_PATH 4096

enum {
    RATE_SEF_EMPTY = 0,
    MAX_SEF_ENTRIES = 28,
    MAX_SEF_PATH_SIZE = WD_PATH_MAX
};

#define COMPILER_SAMP    0x01
#define COMPILER_OPENMP  0x02
#define COMPILER_DEFAULT 0x00

#define CRC32_TRUE              "fdfc4c8d"
#define CRC32_FALSE             "2bcd6830"
#define CRC32_UNKNOWN           "ad26a7c7"

#define OS_SIGNAL_WINDOWS CRC32_TRUE
#define OS_SIGNAL_LINUX   CRC32_FALSE
#define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

#define findstr strfind
#define wd_strcpy strcpy
#define wd_strncpy strncpy
#define wd_snprintf snprintf

#ifndef min3
#define min3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#endif

#define wd_server_env() \
({ \
    int ret = WD_RETZ; \
    if (ret == WD_RETZ) { \
        if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) \
            ret = WD_RETN; \
        else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) \
            ret = WD_RETW; \
    } \
    ret; \
})

typedef struct {
    char *wd_toml_os_type;
    char *wd_toml_binary;
    char *wd_toml_config;
    int wd_ipawncc;
    int wd_idepends;
    char *wd_os_type;
    int wd_sel_stat;
    char *wd_is_samp;
    char *wd_is_omp;
    char *wd_ptr_samp;
    char *wd_ptr_omp;
    int wd_compiler_stat;
    size_t wd_sef_count;
    char wd_sef_found_list \
    [MAX_SEF_ENTRIES][MAX_SEF_PATH_SIZE];
    char *wd_toml_aio_opt;
    char *wd_toml_aio_repo;
    char *wd_toml_gm_input;
    char *wd_toml_gm_output;
    char *wd_toml_github_tokens;
} WatchdogConfig;

extern WatchdogConfig wcfg;

void wd_sef_fdir_reset();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
const char *wd_get_cwd(void);
char* wd_masked_text(int reveal, const char *text);
int wd_mkdir(const char *path);
int wd_run_command(const char *cmd);
int wd_run_command_depends(const char *cmd);
int is_termux_environment(void);
int is_native_windows(void);
void wd_printfile(const char *path);
int wd_set_title(const char *__title);
void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src);
unsigned char wd_tolower(unsigned char c);
bool wd_strcase(const char *text, const char *pattern);
bool strfind(const char *text, const char *pattern);
void wd_escape_quotes(char *dest, size_t size, const char *src);
extern const char* wd_find_near_command(
    const char *ptr_command, const char *__commands[],
    size_t num_cmds, int *out_distance
);
const char* wd_detect_os(void);
int kill_process(const char *name);
int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int path_acces(const char *path);
int file_regular(const char *path);
int file_same_file(const char *a, const char *b);
int ensure_parent_dir(char *out_parent, size_t n, const char *dest);
int cp_f_content(const char *src, const char *dst);
int wd_is_special_dir(const char *name);
int wd_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir);
int wd_set_toml(void);
int wd_sef_wcopy(const char *c_src, const char *c_dest);
int wd_sef_wmv(const char *c_src, const char *c_dest);

#endif
#endif
