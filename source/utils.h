#ifndef UTILS_H
#define UTILS_H

#define __WATCHDOGS__

#include <stddef.h>
#include <stdint.h>

#include <signal.h>
#include <time.h>
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

#if defined(__WINDOWS_NT__)
#define DOG_WINDOWS
#elif defined(__LINUX__)
#define DOG_LINUX
#elif defined(__ANDROID__)
#define DOG_LINUX
#define DOG_ANDROID
#endif

#define _SYM_PARENT_DIR     "../"
#define _PATH_CHR_SEP_POSIX '/'
#define _PATH_CHR_SEP_WIN32 '\\'
#define _PATH_STR_SEP_POSIX "/"
#define _PATH_STR_SEP_WIN32 "\\"

#ifdef DOG_LINUX
#define _relative_path_exec "./"
#else
#define _relative_path_exec ".\\"
#endif

#ifdef DOG_WINDOWS
#include <io.h>
#include <windows.h>
#include <direct.h>
#include <strings.h>
#include <shellapi.h>
#ifdef ZeroMemory
#define _ZERO_MEM_WIN32 ZeroMemory
#endif
#define _PATH_SEP_SYSTEM _PATH_STR_SEP_WIN32
#define IS_PATH_SEP(c) \
((c) == _PATH_CHR_SEP_POSIX || \
(c) == _PATH_CHR_SEP_WIN32)
#define MKDIR(wx) _mkdir(wx)
#define Sleep(sec) Sleep((sec)*1000)
#define sleep(wx) Sleep(wx)
#define setenv(wx,wy,wz) _putenv_s(wx,wy)
#define SETENV(wx,wy,wz) setenv(wx,wy)
#define lstat(wx, wy) stat(wx, wy)
#define S_ISLNK(wx) ((wx & S_IFMT) == S_IFLNK)
#define _set_full_access(wx) \
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
#define _PATH_SEP_SYSTEM _PATH_STR_SEP_POSIX
#define IS_PATH_SEP(c) ((c) == _PATH_CHR_SEP_POSIX)
#define MKDIR(wx) mkdir(wx, 0755)
#define SETENV(wx,wy,wz) setenv(wx,wy,wz)
#define _set_full_access(wx) chmod(wx, 0777)
#endif

#ifndef DOG_WINDOWS
#ifndef bool
typedef _Bool bool;
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#else
#ifndef bool
#define bool _Bool
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

#ifndef print
  #define print(wx) \
    fputs(wx, stdout)
#endif

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

#define CRC32_TRUE    "fdfc4c8d"
#define CRC32_FALSE   "2bcd6830"
#define CRC32_UNKNOWN "ad26a7c7"

#define OS_SIGNAL_WINDOWS "windows"
#define OS_SIGNAL_LINUX   "linux"
#define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

#define DOG_PATH_MAX  (260 + 126)
#define DOG_MAX_PATH  (4096)
#define DOG_MORE_MAX_PATH  (8192)

#define RATE_SEF_EMPTY (0)
#define MAX_SEF_ENTRIES (28)
#define MAX_SEF_PATH_SIZE (DOG_PATH_MAX)

#define DOG_GARBAGE_MAX (13)
#define DOG_GARBAGE_ZERO (false)
#define DOG_GARBAGE_TRUE (true)
#define DOG_GARBAGE_UNIT (1)
#define DOG_GARBAGE_INTRO (2)
#define DOG_GARBAGE_CMD_WARN (3)
#define DOG_GARBAGE_WSL_ENV (4)
#define DOG_GARBAGE_COMPILER_TARGET (5)
#define DOG_GARBAGE_SELECTION_STAT (6)
#define DOG_GARBAGE_IN_INSTALLING (7)
#define DOG_GARBAGE_IN_INSTALLING_PACKAGE (8)
#define DOG_GARBAGE_IN_INSTALLING_PAWNC (9)
#define DOG_GARBAGE_CURL_COMPILER_TESTING (10)
#define DOG_GARBAGE_COMPILE_N_RUNNING_STAT (11)

#define TOML_TABLE_GENERAL "general"
#define TOML_TABLE_COMPILER "compiler"
#define TOML_TABLE_DEPENDENCIES "dependencies"
    
typedef struct {
    bool   dog_garbage_access \
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
    char * dog_toml_server_binary;
    char * dog_toml_server_config;
    char * dog_toml_server_logs  ;
    char * dog_toml_all_flags    ;
    char * dog_toml_root_patterns;
    char * dog_toml_packages     ;
    char * dog_toml_proj_compile ;
    char * dog_toml_proj_input   ;
    char * dog_toml_proj_output  ;
    char * dog_toml_github_tokens;
    char * dog_toml_webhooks     ;
} WatchdogConfig;

extern WatchdogConfig dogconfig;

#define DOG_COL_BLACK      "\033[0;30m"
#define DOG_COL_RED        "\033[0;31m"
#define DOG_COL_GREEN      "\033[0;32m"
#define DOG_COL_YELLOW     "\033[0;33m"
#define DOG_COL_BLUE       "\033[94m"
#define DOG_COL_MAGENTA    "\033[0;35m"
#define DOG_COL_CYAN       "\033[0;36m"
#define DOG_COL_WHITE      "\033[0;37m"

#define DOG_COL_BBLACK     "\033[1;30m"
#define DOG_COL_BRED       "\033[1;31m"
#define DOG_COL_BGREEN     "\033[1;32m"
#define DOG_COL_BYELLOW    "\033[1;33m"
#define DOG_COL_BBLUE      "\033[1;34m"
#define DOG_COL_BMAGENTA   "\033[1;35m"
#define DOG_COL_BCYAN      "\033[1;36m"
#define DOG_COL_BWHITE     "\033[1;37m"
#define DOG_COL_B_MAGENTA  "\033[1;35m"
#define DOG_COL_B_BLUE     "\033[1;94m"

#define BKG_BLACK      "\033[40m"
#define BKG_RED        "\033[41m"
#define BKG_GREEN      "\033[42m"
#define BKG_YELLOW     "\033[43m"
#define BKG_BLUE       "\033[44m"
#define BKG_MAGENTA    "\033[45m"
#define BKG_CYAN       "\033[46m"
#define BKG_WHITE      "\033[47m"

#define BKG_BBLACK     "\033[100m"
#define BKG_BRED       "\033[101m"
#define BKG_BGREEN     "\033[102m"
#define BKG_BYELLOW    "\033[103m"
#define BKG_BBLUE      "\033[104m"
#define BKG_BMAGENTA   "\033[105m"
#define BKG_BCYAN      "\033[106m"
#define BKG_BWHITE     "\033[107m"

#define DOG_COL_BOLD       "\033[1m"
#define DOG_COL_DIM        "\033[2m"
#define DOG_COL_UNDERLINE  "\033[4m"
#define DOG_COL_BLINK      "\033[5m"
#define DOG_COL_REVERSE    "\033[7m"
#define DOG_COL_HIDDEN     "\033[8m"

#define DOG_COL_RESET      "\033[0m"
#define DOG_COL_DEFAULT    "\033[39m"
#define BKG_DEFAULT    "\033[49m"

#ifndef S_IFREG
  #define S_IFREG 0100000
#endif
#ifndef S_IFDIR
  #define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
  #define S_IFLNK 0120000
#endif

#ifndef S_IRUSR
  #define S_IRUSR 0400
  #define S_IWUSR 0200
  #define S_IXUSR 0100
#endif

typedef struct {
        uint64_t st_size;     /* file size in bytes */
        uint64_t st_ino;      /* inode (0 if not available) */
        uint64_t st_dev;      /* device id (0 if not available) */
        unsigned int st_mode; /* file type & permission bits (emulated on Windows) */
        time_t st_latime;     /* last access time */
        time_t st_lmtime;     /* last modification time */
        time_t st_mctime;     /* metadata change time (POSIX) or creation/metadata time on Windows */
} dog_portable_stat_t;

int dog_portable_stat(const char *path, dog_portable_stat_t *out);
int dog_portable_stat(const char *path, dog_portable_stat_t *out);

#define pr_color printf_colour
#define pr_info printf_info
#define pr_warning printf_warning
#define pr_error printf_error

void print_restore_color(void);

void println(FILE *stream, const char* format, ...);
void printf_colour(FILE *stream, const char *color, const char *format, ...);
void printf_info(FILE *stream, const char *format, ...);
void printf_warning(FILE *stream, const char *format, ...);
void printf_error(FILE *stream, const char *format, ...);

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

size_t win_strlcpy(char *dst, const char *src, size_t size);
size_t win_strlcat(char *dst, const char *src, size_t size);

#endif

void dog_sef_path_revert(void);

extern const char* dog_find_near_command(const char *ptr_command,
    const char *__commands[], size_t num_cmds, int *out_distance);

struct struct_of { int (*title)(const char *); };
extern const char* unit_command_list[];
extern const size_t unit_command_len;

void* dog_malloc(size_t size);
void* dog_calloc(size_t count, size_t size);
void* dog_realloc(void* ptr, size_t size);
void  dog_free(void* ptr);

void path_sep_to_posix(char *path);
const char *look_up_sep(const char *sep_path);
const char *fetch_filename(const char *path);
char * fetch_basename(const char *path);

void unit_show_dog(void);
void compiler_show_tip(void);
void unit_show_help(const char* command);

char *dog_procure_pwd(void);
char* dog_masked_text(int reveal, const char *text);
int dog_mkdir_recursive(const char *path);
int dog_exec_command(char *const argv[]);

int fetch_server_env(void);
int is_running_in_container(void);
int is_termux_env(void);
int is_native_windows(void);

void dog_printfile(const char *path);
int dog_console_title(const char *__title);
void dog_strip_dot_fns(char *dst, size_t dst_sz, const char *src);

bool dog_strcase(const char *text, const char *pattern);
bool strend(const char *str, const char *suffix, bool nocase);
bool strfind(const char *text, const char *pattern, bool nocase);
int match_wildcard(const char *str, const char *pat);

void set_default_access(const char *c_dest);

int dog_kill_process(const char *process);

int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int path_access(const char *path);
int file_regular(const char *path);
int file_same_file(const char *a, const char *b);
int dog_dot_or_dotdot(const char *name);

int dog_find_path(const char *sef_path, const char *sef_name, const char *ignore_dir);
int dog_configure_toml(void);
int dog_sef_wcopy(const char *c_src, const char *c_dest);
int dog_sef_wmv(const char *c_src, const char *c_dest);

#endif /* UTILS_H */
