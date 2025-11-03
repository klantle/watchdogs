#ifndef UTILS_H
#define UTILS_H

/** Define WATCHDOGS compilation flag **/
#define __WATCHDOGS__
#ifdef __WATCHDOGS__

#include <stddef.h>  /** size_t, NULL **/
#include <stdbool.h> /** bool type **/
#include <stdint.h>  /** fixed width integers **/
#include <limits.h>  /** limits of types **/
#include <dirent.h>  /** directory traversal **/
#include <sys/types.h> /** POSIX types **/
#include <sys/stat.h>  /** file stat and chmod **/
#include "include/cJSON/cJSON.h" /** JSON parser **/
#include "include/tomlc/toml.h"  /** TOML parser **/

#ifdef _WIN32
#include <windows.h>  /** Windows API **/
#include <direct.h>   /** mkdir on Windows **/
#include <shlwapi.h>  /** path functions **/
#include <strings.h>  /** string utilities **/
#include <io.h>       /** low-level I/O **/

/** Windows-specific path separator **/
#define __PATH_SYM "\\"
/** Macro to check path separator **/
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
/** Windows mkdir wrapper **/
#define mkdir(path) _mkdir(path)
#define MKDIR(path) mkdir(path)
/** Sleep wrappers **/
#define Sleep(sec) Sleep((sec)*1000)
#define sleep(x) Sleep(x)
/** Environment variable setter **/
#define setenv(x,y,z) _putenv_s(x,y)
#define SETEN(x,y,z) setenv(x,y)
/** Windows chmod wrapper **/
static inline int
win32_chmod(const char *path) {
    int mode = _S_IREAD | _S_IWRITE;
    return chmod(path, mode);
}
#define CHMOD(path, mode) _chmod(path, mode)
#define FILE_MODE _S_IREAD | _S_IWRITE
#define getcwd _getcwd

#else  /** Linux / POSIX **/
#include <sys/utsname.h> /** uname **/
#include <sys/wait.h>    /** waitpid **/
#include <unistd.h>      /** POSIX functions **/
#include <fnmatch.h>     /** filename matching **/
#define __PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
#define MKDIR(x) mkdir(x,0755)
#define SETENV(x,y,z) setenv(x,y,z)
#define CHMOD(x,y) chmod(x,y)
#define FILE_MODE 0777
#endif

/** Max path constants **/
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX + 3836
#else
#undef MAX_PATH
#define MAX_PATH PATH_MAX + 3836
#endif

/** String find alias **/
#define findstr strfind

/** Minimum of three numbers macro **/
#ifndef min3
#define min3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#endif

/** Readline history support if available **/
#if __has_include(<readline/history.h>)
#include <readline/history.h>
#define wd_u_history() using_history()
#define wd_a_history(cmd) add_history(cmd)
#else
#define wd_u_history()
#define wd_a_history(cmd)
#endif

/** Memory allocation wrappers **/
#define wd_malloc(x) malloc(x)
#define wd_calloc(x, y) calloc(x, y)
#define wd_realloc(x, y) realloc(x, y)
#define wd_free(x) free(x)

/** Return codes **/
#define __RETZ 0
#define __RETN 1
#define __RETW 2
#define __RETH 3

/** Directory entry types fallback **/
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

/** Compiler type flags **/
#define COMPILER_SAMP    0x01
#define COMPILER_OPENMP  0x02
#define COMPILER_DEFAULT 0x00

/** CRC32 strings representing OS signals **/
#define CRC32_TRUE              "fdfc4c8d"
#define CRC32_FALSE             "2bcd6830"
#define CRC32_UNKNOWN           "ad26a7c7"

#define OS_SIGNAL_WINDOWS CRC32_TRUE
#define OS_SIGNAL_LINUX   CRC32_FALSE  
#define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

/** SEF config limits **/
#define MAX_SEF_ENTRIES 28
#define MAX_SEF_PATH_SIZE PATH_MAX

/** Watchdog configuration structure **/
typedef struct {
    char *wd_toml_os_type;       /** OS type from TOML **/
    char *wd_toml_binary;        /** Path to binary **/
    char *wd_toml_config;        /** Config file path **/
    int wd_ipackage;             /** Package index **/
    int wd_idepends;             /** Dependency count **/
    char *wd_os_type;            /** Detected OS type **/
    int wd_sel_stat;             /** Selection status **/
    char *wd_is_samp;            /** Samp compiler flag **/
    char *wd_is_omp;             /** OpenMP compiler flag **/
    char *wd_ptr_samp;           /** Pointer to Samp binary **/
    char *wd_ptr_omp;            /** Pointer to OpenMP binary **/
    int wd_compiler_stat;        /** Compiler check status **/
    size_t wd_sef_count;         /** SEF files found **/
    char wd_sef_found_list[MAX_SEF_ENTRIES][MAX_SEF_PATH_SIZE]; /** List of SEF paths **/
    char *wd_toml_aio_opt;       /** TOML aio options **/
    char *wd_toml_aio_repo;      /** TOML aio repository **/
    char *wd_toml_gm_input;      /** Game input path **/
    char *wd_toml_gm_output;     /** Game output path **/
    char *wd_toml_github_tokens; /** GitHub tokens **/
} WatchdogConfig;

extern WatchdogConfig wcfg; /** Global config instance **/

extern int _already_compiler_check; /** Compiler check status flag **/

/** Function declarations **/
void wd_sef_fdir_reset(); /** Reset SEF directory tracking **/
struct struct_of { int (*title)(const char *); }; /** Function pointer struct **/
extern const char* __command[]; /** Command array **/
extern const size_t __command_len; /** Command count **/
int mkdir_recusrs(const char *path); /** Recursive mkdir **/
int wd_run_command(const char *cmd); /** Run shell command **/
int is_termux_environment(void); /** Detect Termux **/
int is_native_windows(void); /** Detect Windows **/
void print_file_to_terminal(const char *path); /** Print file content **/
int wd_set_permission(const char *src, const char *dst); /** Set file permissions **/
int wd_set_title(const char *__title); /** Set terminal title **/
void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src); /** Strip dot files **/
bool strfind(const char *text, const char *pattern); /** String search **/
uint32_t hash_str(const char *s); /** Simple hash function **/
void wd_escape_quotes(char *dest, size_t size, const char *src); /** Escape quotes **/
extern const char* wd_find_near_command(
    const char *ptr_command, const char *__commands[],
    size_t num_cmds, int *out_distance
); /** Fuzzy command search **/
const char* wd_detect_os(void); /** Detect OS **/
int kill_process(const char *name); /** Kill process by name **/
int dir_exists(const char *path); /** Check if directory exists **/
int path_exists(const char *path); /** Check if path exists **/
int dir_writable(const char *path); /** Check if dir is writable **/
int path_acces(const char *path); /** Check path accessibility **/
int file_regular(const char *path); /** Check if file is regular **/
int file_same_file(const char *a, const char *b); /** Compare two files **/
int ensure_parent_dir(char *out_parent, size_t n, const char *dest); /** Ensure parent dir exists **/
int cp_f_content(const char *src, const char *dst); /** Copy file content **/
int wd_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir); /** Scan SEF dir **/
int wd_set_toml(void); /** Load TOML config **/
int wd_sef_wcopy(const char *c_src, const char *c_dest); /** Copy SEF file **/
int wd_sef_wmv(const char *c_src, const char *c_dest); /** Move SEF file **/
void print_file_with_annotations(const char *pawn_output); /** Compiler Cause **/

#endif
#endif
