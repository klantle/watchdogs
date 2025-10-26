#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <dirent.h>

#define min3(a, b, c) \
        ((a) < (b) ? \
        ((a) < (c) ? \
        (a) : (c)) : ((b) < (c) ? \
        (b) : (c)))

#if __has_include(<readline/history.h>)
        #include <readline/history.h>
        #define wd_u_history() using_history()
        #define wd_a_history(cmd) add_history(cmd)
#else
        #define wd_u_history()
        #define wd_a_history(cmd)
#endif

#define wdmalloc(x) malloc(x)
#define wdcalloc(x, y) calloc(x, y)
#define wdrealloc(x, y) realloc(x, y)
#define wdfree(x) free(x)

#define RETZ 0
#define RETN 1
#define RETW 2
#define RETH 3

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

#define VAL_TRUE 0x01
#define VAL_FALSE 0x00

#define OS_SIGNAL_WINDOWS	0x01
#define OS_SIGNAL_LINUX		0x00  
#define OS_SIGNAL_UNKNOWN	0x02

#define MAX_SEF_ENTRIES 28
#define MAX_SEF_PATH_SIZE PATH_MAX

typedef struct {
        int ipackage;
        int idepends;
        const char* os;
        uint8_t os_type;
        uint8_t f_samp;
        uint8_t f_openmp;
        char* pointer_samp;
        char* pointer_openmp;
        int compiler_error;
        size_t sef_count;
        char sef_found[MAX_SEF_ENTRIES][MAX_SEF_PATH_SIZE];
        char* serv_dbg;
        char* ci_options;
        char* gm_input;
        char* gm_output;
} WatchdogConfig;

extern WatchdogConfig wcfg;

void wd_sef_fdir_reset();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
int wd_signal_os(void);
int wd_run_command(const char *cmd);
void print_file_to_terminal(const char *path);
int wd_set_permission(const char *src, const char *dst);
int wd_set_title(const char *__title);
char* readline_colored(const char* prompt);
void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src);
bool strfind(const char *text, const char *pattern);
void wd_escape_quotes(char *dest, size_t size, const char *src);
extern const char* wd_find_near_command(
        const char *ptr_command,
        const char *__commands[],
        size_t num_cmds,
        int *out_distance
);
const char* wd_detect_os(void);
int kill_process(const char *name);
void kill_process_safe(const char *name);
int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int path_acces(const char *path);
int file_regular(const char *path);
int file_same_file(const char *a, const char *b);
int ensure_parent_dir(char *out_parent, size_t n, const char *dest);
int cp_f_content(const char *src, const char *dst);
int wd_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir);
int wd_set_toml(void);
int wd_sef_wcopy(const char *c_src, const char *c_dest);
int wd_sef_wmv(const char *c_src, const char *c_dest);

#endif