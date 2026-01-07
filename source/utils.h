#ifndef UTILS_H
#define UTILS_H

#define __WATCHDOGS__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <readline/history.h>
#include <readline/readline.h>
#define dog_history_init() using_history()
#define dog_history_add(cmd) add_history(cmd)
#define dog_history_clear() clear_history()
#if __has_include(<spawn.h>)
    #include <spawn.h>
#endif
#include "../include/cJSON/cJSON.h"
#include "../include/tomlc/toml.h"

/* Platform detection */
#if defined(__WINDOWS32__)
#define DOG_WINDOWS
#elif defined(__LINUX__)
#define DOG_LINUX
#elif defined(__ANDROID__)
#define DOG_LINUX
#define DOG_ANDROID
#endif

/* Path constants */
#define __PARENT_DIR "../"
#define __PATH_CHR_SEP_LINUX '/'
#define __PATH_CHR_SEP_WIN32 '\\'
#define __PATH_STR_SEP_LINUX "/"
#define __PATH_STR_SEP_WIN32 "\\"

/* Symbol to run programs */
#ifdef DOG_LINUX
/* Posix */
#define SYM_PROG "./" /* ./name */
#else
/* MS-DOS */
#define SYM_PROG ".\\" /* .\name */
#endif

/* Platform-specific includes & defines */
#ifdef DOG_WINDOWS
#include <io.h>
#include <windows.h>
#include <direct.h>
#include <strings.h>
#include <shellapi.h>
#ifdef ZeroMemory
#define _ZERO_MEM_WIN32 ZeroMemory
#endif
#define __PATH_SEP __PATH_STR_SEP_WIN32
#define IS_PATH_SEP(c) \
((c) == __PATH_CHR_SEP_LINUX || \
(c) == __PATH_CHR_SEP_WIN32)
#define mkdir(wx) _mkdir(wx)
#define MKDIR(wx) mkdir(wx)
#define Sleep(sec) Sleep((sec)*1000)
#define sleep(wx) Sleep(wx)
#define setenv(wx,wy,wz) _putenv_s(wx,wy)
#define SETENV(wx,wy,wz) setenv(wx,wy)
#define lstat(wx, wy) stat(wx, wy)
#define S_ISLNK(wx) ((wx & S_IFMT) == S_IFLNK)
#define CHMOD_OWNER_GROUP(wx) \
({ \
const char *_p = (wx); \
mode_t _m = S_IRUSR | S_IWUSR | S_IXUSR | \
            S_IRGRP | S_IWGRP | S_IXGRP | \
            S_IROTH | S_IXOTH; \
chmod(_p, _m); \
})
#define CHMOD_FULL(wx) \
({ \
const char *_p = (wx); \
mode_t _m =  S_IRUSR | S_IWUSR | S_IXUSR | \
             S_IRGRP | S_IWGRP | S_IXGRP | \
             S_IROTH | S_IWOTH | S_IXOTH; \
chmod(_p, _m); \
})
#define open _open
#define read _read
#define close _close
#define O_RDONLY _O_RDONLY
#define getcwd _getcwd
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fnmatch.h>
#define __PATH_SEP __PATH_STR_SEP_LINUX
#define IS_PATH_SEP(c) ((c) == __PATH_CHR_SEP_LINUX)
#define MKDIR(wx) mkdir(wx, 0755)
#define SETENV(wx,wy,wz) setenv(wx,wy,wz)
#define CHMOD_OWNER_GROUP(wx) chmod(wx, 0775)
#define CHMOD_FULL(wx) chmod(wx, 0777)
#endif

#define PATH_SEPARATOR(sep_path) ({ \
const char *_p = (sep_path); \
const char *_l = _p ? strrchr(_p, __PATH_CHR_SEP_LINUX) : NULL; \
const char *_w = _p ? strrchr(_p, __PATH_CHR_SEP_WIN32) : NULL; \
(_l && _w) ? ((_l > _w) ? _l : _w) : (_l ? _l : _w); \
})

#define __UNUSED__      __attribute__((unused))
#define __DEPRECATED__  __attribute__((deprecated))
#define __NORETURN__    __attribute__((noreturn))
#define __PACKED__      __attribute__((packed))
#define __ALIGN(N)__    __attribute__((aligned(N)))
#define __FORMAT(F,V)__ __attribute__((format(F,V,V)))
#define __CONSTRUCTOR__ __attribute__((constructor))
#define __DESTRUCTOR__  __attribute__((destructor))
#define __PURE__        __attribute__((pure))
#define __CONST__       __attribute__((const))

/* Dirent constants */
#if ! defined(DT_UNKNOWN) && ! defined(DT_FIFO)
#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK    10
#define DT_SOCK   12
#define DT_WHT    14
#endif

#define DOG_PATH_MAX  (260 + 126)
#define DOG_MAX_PATH  (4096)
#define DOG_MORE_MAX_PATH  (8192)

/* SEF constants */
enum {
RATE_SEF_EMPTY = 0,
MAX_SEF_ENTRIES = 28,
MAX_SEF_PATH_SIZE = DOG_PATH_MAX
};

/* Converter Array Number. */
enum { /* Garbage */
DOG_GARBAGE_MAX = 15 /* 14 */,
DOG_GARBAGE_ZERO = 0,
DOG_GARBAGE_TRUE = 1,
DOG_GARBAGE_IS_T = 2,
DOG_GARBAGE_IS_H = 3,
DOG_GARBAGE_UNIT = 1,
DOG_GARBAGE_INTRO = 2,
DOG_GARBAGE_CMD_WARN = 3,
DOG_GARBAGE_WSL_ENV = 4,
DOG_GARBAGE_COMPILER_HOST = 6,
DOG_GARBAGE_COMPILER_TARGET = 7,
DOG_GARBAGE_COMPILER_RETRY = 8,
DOG_GARBAGE_SELECTION_STAT = 9,
DOG_GARBAGE_IN_INSTALLING = 10,
DOG_GARBAGE_IN_INSTALLING_PACKAGE = 11,
DOG_GARBAGE_IN_INSTALLING_PAWNC = 12,
DOG_GARBAGE_CURL_COMPILER_TESTING = 13,
DOG_GARBAGE_COMPILE_N_RUNNING_STAT = 14
};

/* CRC32 signals */
#define CRC32_TRUE    "fdfc4c8d"
#define CRC32_FALSE   "2bcd6830"
#define CRC32_UNKNOWN "ad26a7c7"

#define OS_SIGNAL_WINDOWS "windows"
#define OS_SIGNAL_LINUX   "linux"
#define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

/* Watchdog config struct */
typedef struct {
int dog_garbage_access \
          [DOG_GARBAGE_MAX];
char * dog_os_type           ;
char * dog_is_samp           ;
char * dog_is_omp            ;
char * dog_ptr_samp          ;
char * dog_ptr_omp           ;
int    dog_compiler_stat     ;
size_t dog_sef_count         ;
char   dog_sef_found_list \
        [MAX_SEF_ENTRIES] \
        [MAX_SEF_PATH_SIZE]  ;
char * dog_toml_os_type      ;
char * dog_toml_binary       ;
char * dog_toml_config       ;
char * dog_toml_logs         ;
char * dog_toml_aio_opt      ;
char * dog_toml_root_patterns;
char * dog_toml_packages     ;
char * dog_toml_proj_input   ;
char * dog_toml_proj_output  ;
char * dog_toml_github_tokens;
char * dog_toml_webhooks     ;
} WatchdogConfig;

extern WatchdogConfig dogconfig;

/* Utility function declarations */
void dog_sef_restore(void);

#ifdef DOG_LINUX

#ifndef strlcpy
size_t strlcpy(char *dst, const char *src, size_t size);
#endif
#ifndef strlcat
size_t strlcat(char *dst, const char *src, size_t size);
#endif
#endif
#ifdef DOG_WINDOWS

#define strlcpy   win_strlcpy
#define strlcat   win_strlcat
#define ftruncate win_ftruncate

size_t win_strlcpy(char *dst, const char *src, size_t size);
size_t win_strlcat(char *dst, const char *src, size_t size);
int win_ftruncate(FILE *file, long length);

#endif

struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;

void* dog_malloc(size_t size);
void* dog_calloc(size_t count, size_t size);
void* dog_realloc(void* ptr, size_t size);
void  dog_free(void* ptr);

void path_sym_convert(char *path);
const char *try_get_basename(const char *path);
const char *try_get_filename(const char *path);

void unit_show_dog(void);
void compiler_show_tip(void);
void unit_show_help(const char* command);

char *dog_procure_pwd(void);
char* dog_masked_text(int reveal, const char *text);
int dog_mkdir(const char *path);
void dog_escaping_json(char *dest, const char *src, size_t dest_size);
int dog_exec_command(char *const argv[]);
void dog_clear_screen(void);

int dog_server_env(void);

int is_running_in_container(void);
int is_termux_env(void);
int is_native_windows(void);

void dog_printfile(const char *path);
int dog_console_title(const char *__title);
void dog_strip_dot_fns(char *dst, size_t dst_sz, const char *src);

bool dog_strcase(const char *text, const char *pattern);
bool strend(const char *str, const char *suffix, bool nocase);
bool strfind(const char *text, const char *pattern, bool nocase);
char* strreplace(const char *source, const char *old_sub, const char *new_sub);
void dog_escape_quotes(char *dest, size_t size, const char *src);

extern const char* dog_find_near_command(const char *ptr_command,
    const char *__commands[], size_t num_cmds, int *out_distance);

int dog_kill_process(const char *process);

int dog_match_wildcard(const char *str, const char *pat);
const char *dog_procure_os(void);

int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int path_access(const char *path);
int file_regular(const char *path);
int file_same_file(const char *a, const char *b);
int ensure_parent_dir(char *out_parent, size_t n, const char *dest);

int dog_dot_or_dotdot(const char *name);

int dog_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir);
int dog_toml_configs(void);
int dog_sef_wcopy(const char *c_src, const char *c_dest);
int dog_sef_wmv(const char *c_src, const char *c_dest);

#endif /* UTILS_H */
