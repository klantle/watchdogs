# ifndef UTILS_H
# define UTILS_H

# define __WATCHDOGS__

# include <stddef.h>
# include <stdbool.h>
# include <stdint.h>
# include <limits.h>
# include <dirent.h>
# include <sys/stat.h>
# include <sys/types.h>

# if __has_include(<readline/history.h>)
# include <readline/history.h>
# include <readline/readline.h>
# define wg_u_history() using_history()
# define wg_a_history(cmd) add_history(cmd)
# endif

# include "../include/cJSON/cJSON.h"
# include "../include/tomlc/toml.h"

# if defined(__WINDOWS__)
# define WG_WINDOWS
# endif
# if defined(__LINUX__)
# define WG_LINUX
# endif
# if defined(__ANDROID__)
# define WG_ANDROID
# endif

# ifdef WG_WINDOWS
# include <io.h>
# include <time.h>
# include <direct.h>
# include <windows.h>
# include <shlwapi.h>
# include <strings.h>
# define __PATH_SEP "\\"
# define IS_PATH_SEP(c) ((c) == '/' || (c) == '\\')
# define mkdir(wx) _mkdir(wx)
# define MKDIR(wx) mkdir(wx)
# define Sleep(sec) Sleep((sec)*1000)
# define sleep(wx) Sleep(wx)
# define setenv(wx,wy,wz) _putenv_s(wx,wy)
# define SETEN(wx,wy,wz) setenv(wx,wy)
# define win32_chmod(path) \
({ \
    const char *_path = (path); \
    int _mode = _S_IREAD | _S_IWRITE; \
    chmod(_path, _mode); \
})
# define open _open
# define read _read
# define close _close
# define O_RDONLY _O_RDONLY
# define CHMOD(wx, wy) _chmod(wx, wy)
# define FILE_MODE _S_IREAD | _S_IWRITE
# define getcwd _getcwd
# else
# include <sys/utsname.h>
# include <sys/wait.h>
# include <unistd.h>
# include <fnmatch.h>
# define __PATH_SEP "/"
# define IS_PATH_SEP(c) ((c) == '/')
# define MKDIR(wx) mkdir(wx,0755)
# define SETENV(wx,wy,wz) setenv(wx,wy,wz)
# define CHMOD(wx,wy) chmod(wx,wy)
# define FILE_MODE 0777
# endif

#define wg_realloc(wx, wy) \
    ((wx) ? realloc((wx), (wy)) : malloc((wy)))
#define wg_malloc(wx) malloc(wx)
#define wg_calloc(wx, wy) calloc((wx), (wy))
#define wg_free(wx) do { if (wx) { free((wx)); (wx) = NULL; } } while(0)

# define __PATH_CHR_SEP_LINUX '/'
# define __PATH_CHR_SEP_WIN32 '\\'
# define __PATH_STR_SEP_LINUX "/"
# define __PATH_STR_SEP_WIN32 "\\"

# ifndef DT_UNKNOWN
# define DT_UNKNOWN 0
# define DT_FIFO 1
# define DT_CHR 2
# define DT_DIR 4
# define DT_BLK 6
# define DT_REG 8
# define DT_LNK 10
# define DT_SOCK 12
# define DT_WHT 14
# endif

# define WG_PATH_MAX 260 + 126
# define WG_MAX_PATH 4096

enum {
    RATE_SEF_EMPTY = 0,
    MAX_SEF_ENTRIES = 28,
    MAX_SEF_PATH_SIZE = WG_PATH_MAX
};

# define COMPILER_SAMP    0x01
# define COMPILER_OPENMP  0x02
# define COMPILER_DEFAULT 0x00

# define CRC32_TRUE              "fdfc4c8d"
# define CRC32_FALSE             "2bcd6830"
# define CRC32_UNKNOWN           "ad26a7c7"

# define OS_SIGNAL_WINDOWS CRC32_TRUE
# define OS_SIGNAL_LINUX   CRC32_FALSE
# define OS_SIGNAL_UNKNOWN CRC32_UNKNOWN

# define findstr strfind
# define wg_strcpy strcpy
# define wg_strncpy strncpy
# define wg_strlcpy strlcpy
# define wg_sprintf sprintf
# define wg_snprintf snprintf

# ifndef min3
# define min3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
# endif

# define wg_server_env() \
({ \
    int ret = 0; \
    if (ret == 0) { \
        if (!strcmp(wgconfig.wg_is_samp, CRC32_TRUE)) \
            ret = 1; \
        else if (!strcmp(wgconfig.wg_is_omp, CRC32_TRUE)) \
            ret = 2; \
    } \
    ret; \
})

typedef struct {
    char * wg_toml_os_type;
    char * wg_toml_binary;
    char * wg_toml_config;
    char * wg_toml_logs;
    int    wg_ipawncc;
    int    wg_idepends;
    char * wg_os_type;
    int    wg_sel_stat;
    char * wg_is_samp;
    char * wg_is_omp;
    char * wg_ptr_samp;
    char * wg_ptr_omp;
    int    wg_compiler_stat;
    size_t wg_sef_count;
    char   wg_sef_found_list \
            [MAX_SEF_ENTRIES] \
            [MAX_SEF_PATH_SIZE];
    char * wg_toml_aio_opt;
    char * wg_toml_aio_repo;
    char * wg_toml_gm_input;
    char * wg_toml_gm_output;
    char * wg_toml_github_tokens;
    char * wg_toml_key_ai;
    char * wg_toml_chatbot_ai;
    char * wg_toml_models_ai;
} WatchdogConfig;

extern WatchdogConfig wgconfig;

void wg_sef_fdir_reset(void);
# ifdef WG_WINDOWS
# define strlcpy   win_strlcpy
# define strlcat   win_strlcat
# define ftruncate win_ftruncate
size_t win_strlcpy(char *dst, const char *src, size_t size);
size_t win_strlcat(char *dst, const char *src, size_t size);
int win_ftruncate(FILE *file, long length);
# endif
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
char *wg_get_cwd(void);
char* wg_masked_text(int reveal, const char *text);
int wg_mkdir(const char *path);
void wg_escaping_json(char *dest, const char *src, size_t dest_size);
int wg_run_command(const char *cmd);
int is_termux_environment(void);
int is_native_windows(void);
void wg_printfile(const char *path);
int wg_console_title(const char *__title);
void wg_strip_dot_fns(char *dst, size_t dst_sz, const char *src);
unsigned char wg_tolower(unsigned char c);
bool wg_strcase(const char *text, const char *pattern);
bool strfind(const char *text, const char *pattern);
char* strreplace(const char *source, const char *old_sub, const char *new_sub);
void wg_escape_quotes(char *dest, size_t size, const char *src);
extern const char* wg_find_near_command(const char *ptr_command, const char *__commands[], size_t num_cmds, int *out_distance);
int kill_process(const char *name);
int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int path_access(const char *path);
int file_regular(const char *path);
int file_same_file(const char *a, const char *b);
int ensure_parent_dir(char *out_parent, size_t n, const char *dest);
int wg_is_special_dir(const char *name);
int wg_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir);
int wg_toml_configs(void);
int wg_sef_wcopy(const char *c_src, const char *c_dest);
int wg_sef_wmv(const char *c_src, const char *c_dest);

# endif