#ifndef UTILS_H
#define UTILS_H

#define __WATCHDOGS__

#if __has_include(<stddef.h>)
#include <stddef.h>
#endif
#if __has_include(<stdbool.h>)
#include <stdbool.h>
#endif
#if __has_include(<stdint.h>)
#include <stdint.h>
#endif
#if __has_include(<fcntl.h>)
#include <fcntl.h>
#endif
#if __has_include(<limits.h>)
#include <limits.h>
#endif
#if __has_include(<dirent.h>)
#include <dirent.h>
#endif
#if __has_include(<sys/stat.h>)
#include <sys/stat.h>
#endif
#if __has_include(<sys/types.h>)
#include <sys/types.h>
#endif
#if __has_include(<readline/history.h>)
#include <readline/history.h>
#include <readline/readline.h>
#define dog_u_history() using_history()
#define dog_a_history(cmd) add_history(cmd)
#endif
#if __has_include(<spawn.h>)
    #include <spawn.h>
#elif __has_include(<android-spawn.h>)
    #include <android-spawn.h>
#endif
#if __has_include("../include/cJSON/cJSON.h")
#include "../include/cJSON/cJSON.h"
#endif
#if __has_include("../include/tomlc/toml.h")
#include "../include/tomlc/toml.h"
#endif

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

/* Platform-specific includes & defines */
#ifdef DOG_WINDOWS
#if __has_include(<io.h>)
#include <io.h>
#endif
#if __has_include(<windows.h>)
#include <windows.h>
#endif
#if __has_include(<time.h>)
#include <time.h>
#endif
#if __has_include(<direct.h>)
#include <direct.h>
#endif
#if __has_include(<strings.h>)
#include <strings.h>
#endif
#define __PATH_SEP __PATH_STR_SEP_WIN32
#define IS_PATH_SEP(c) \
    ((c) == __PATH_CHR_SEP_LINUX || (c) == __PATH_CHR_SEP_WIN32)
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
#if __has_include(<sys/utsname.h>)
#include <sys/utsname.h>
#endif
#if __has_include(<sys/wait.h>)
#include <sys/wait.h>
#endif
#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#if __has_include(<fnmatch.h>)
#include <fnmatch.h>
#endif
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

#define __BIT_MASK_NONE   (0x00)  /* 0000 0000 0000 0000 */
#define __BIT_MASK_ZERO   (0x01)  /* 0000 0000 0000 0001 */
#define __BIT_MASK_ONE    (0x02)  /* 0000 0000 0000 0010 */
#define __BIT_MASK_TWO    (0x04)  /* 0000 0000 0000 0100 */
#define __BIT_MASK_THREE  (0x08)  /* 0000 0000 0000 1000 */
#define __BIT_MASK_FOUR   (0x10)  /* 0000 0000 0001 0000 */
#define __BIT_MASK_FIVE   (0x20)  /* 0000 0000 0010 0000 */
#define __BIT_MASK_SIX    (0x40)  /* 0000 0000 0100 0000 */
#define __BIT_MASK_SEVEN  (0x80)  /* 0000 0000 1000 0000 */
#define __BIT_MASK_EIGHT  (0x100) /* 0000 0001 0000 0000 */
#define __BIT_MASK_NINE   (0x200) /* 0000 0010 0000 0000 */
#define __BIT_MASK_TEN    (0x400) /* 0000 0100 0000 0000 */
#define __BIT_MASK_ELEVEN (0x800) /* 0000 1000 0000 0000 */

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
#define DOG_MAX_PATH  4096

/* SEF constants */
enum {
    /****/RATE_SEF_EMPTY = 0,
    /****/MAX_SEF_ENTRIES = 28,
    /****/MAX_SEF_PATH_SIZE = DOG_PATH_MAX
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
    int    dog_ipawncc;
    int    dog_idepends;
    int    dog_idownload;
    char * dog_os_type;
    int    dog_sel_stat;
    char * dog_is_samp;
    char * dog_is_omp;
    char * dog_ptr_samp;
    char * dog_ptr_omp;
    int    dog_compiler_stat;
    size_t dog_sef_count;
    char   dog_sef_found_list \
        [MAX_SEF_ENTRIES] [MAX_SEF_PATH_SIZE];
    char * dog_toml_os_type;
    char * dog_toml_binary;
    char * dog_toml_config;
    char * dog_toml_logs;
    char * dog_toml_aio_opt;
    char * dog_toml_root_patterns;
    char * dog_toml_packages;
    char * dog_toml_proj_input;
    char * dog_toml_proj_output;
    char * dog_toml_github_tokens;
    char * dog_toml_key_ai;
    char * dog_toml_chatbot_ai;
    char * dog_toml_models_ai;
    char * dog_toml_webhooks;
} WatchdogConfig;

extern WatchdogConfig wgconfig;

/* Utility function declarations */
void dog_sef_fdir_memset_to_null(void);

#ifdef DOG_LINUX /* Pop!_OS, etc. */
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

void unit_show_help(const char* command);

char *dog_procure_pwd(void);
char* dog_masked_text(int reveal, const char *text);
int dog_mkdir(const char *path);
void dog_escaping_json(char *dest, const char *src, size_t dest_size);
int dog_exec_command(const char *cmd);
void dog_clear_screen(void);

int dog_server_env(void);
int is_pterodactyl_env(void);
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
