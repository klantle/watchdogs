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
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>

#define __PATH_SYM "\\"
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
#define mkdir(path) _mkdir(path)
#define MKDIR(path) mkdir(path)
#define Sleep(sec) Sleep((sec)*1000)
#define sleep(x) Sleep(x)
#define setenv(x,y,z) _putenv_s(x,y)
#define SETEN(x,y,z) setenv(x,y)
static inline int
win32_chmod(const char *path) {
    int mode = _S_IREAD | _S_IWRITE;
    return chmod(path, mode);
}
#define open _open
#define read _read
#define close _close
#define O_RDONLY _O_RDONLY
#define CHMOD(path, mode) _chmod(path, mode)
#define FILE_MODE _S_IREAD | _S_IWRITE
#define getcwd _getcwd

#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fnmatch.h>
#define __PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
#define MKDIR(x) mkdir(x,0755)
#define SETENV(x,y,z) setenv(x,y,z)
#define CHMOD(x,y) chmod(x,y)
#define FILE_MODE 0777
#endif

#define __PATH_CHR_SYM_LINUX '/'
#define __PATH_CHR_SYM_WIN32 '\\'

#define WD_PATH_MAX 260
#define WD_MAX_PATH 4096

#define findstr strfind

#if __has_include(<readline/history.h>)
#include <readline/history.h>
#define wd_u_history() using_history()
#define wd_a_history(cmd) add_history(cmd)
#else
#define wd_u_history()
#define wd_a_history(cmd)
#endif

#define wd_malloc(x) malloc(x)
#define wd_calloc(x, y) calloc(x, y)
#define wd_realloc(x, y) realloc(x, y)
#define wd_free(x) free(x)

#define __RETZ 0
#define __RETN 1
#define __RETW 2
#define __RETH 3

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

#define COMPILER_SAMP    0x01
#define COMPILER_OPENMP  0x02
#define COMPILER_DEFAULT 0x00

#define CRC32_TRUE              "fdfc4c8d"
#define CRC32_FALSE             "2bcd6830"
#define CRC32_UNKNOWN           "ad26a7c7"

#define OS_SIGNAL_WINDOWS CRC32_TRUE
#define OS_SIGNAL_LINUX   CRC32_FALSE
#define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

#define MAX_SEF_ENTRIES 28
#define MAX_SEF_PATH_SIZE WD_PATH_MAX

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
    char wd_sef_found_list[MAX_SEF_ENTRIES][MAX_SEF_PATH_SIZE];
    char *wd_toml_aio_opt;
    char *wd_toml_aio_repo;
    char *wd_toml_gm_input;
    char *wd_toml_gm_output;
    char *wd_toml_github_tokens;
} WatchdogConfig;

extern WatchdogConfig wcfg;

extern int rate_compiler_check;

void wd_sef_fdir_reset();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
const char *wd_get_cwd(void);
size_t wd_strcnpy(char *dest, const char *src, size_t size);
size_t wd_strcpy(char *dest, const char *src, size_t size);
int wd_snprintf(char *buf, size_t size, const char *fmt, ...);
char* wd_masked_text(int reveal, const char *text);
int mkdir_recusrs(const char *path);
int wd_server_env(void);
int wd_run_command(const char *cmd);
int is_termux_environment(void);
int is_native_windows(void);
void wd_printfile(const char *path);
int wd_set_title(const char *__title);
void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src);
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
