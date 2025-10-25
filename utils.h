#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <limits.h>

#if __has_include(<readline/history.h>)
    #include <readline/history.h>
    #define wd_u_history() using_history()
    #define wd_a_history(cmd) add_history(cmd)
#else
    #define wd_u_history()
    #define wd_a_history(cmd)
#endif

#define RETZ 0
#define RETN 1
#define RETW 2

#define MAX_SEF_PATH_COUNT 28
#define MAX_SEF_PATH_SIZE PATH_MAX

typedef struct {
        int ipcc;
        char *os;
        int __os__;
        int f_samp;
        int f_openmp;
        char *pointer_samp;
        char *pointer_openmp;
        int compiler_error;
        int sef_count;
        char sef_found[MAX_SEF_PATH_COUNT][MAX_SEF_PATH_SIZE];
        char *serv_dbg;
        char *ci_options;
        char *gm_input;
        char *gm_output;
} __wd;
extern __wd wcfg;

void wd_sef_fdir_reset();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
int wd_SignalOS(void);
int wd_RunCommand(const char *cmd);
void wd_SetPermission(const char *src, const char *tmp);
int wd_SetTitle(const char *__title);
char* readline_colored(const char* prompt);
void wd_StripDotFns(char *dst, size_t dst_sz, const char *src);
bool strfind(const char *text, const char *pattern);
void wd_EscapeQuotes(char *dest, size_t size, const char *src);
extern const char* wd_FindNearCommand(
        const char *ptr_command,
        const char *__commands[],
        size_t num_cmds,
        int *out_distance
);
const char* wd_DetectOS(void);
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
int wd_SetToml(void);
int wd_sef_wcopy(const char *c_src, const char *c_dest);
int wd_sef_wmv(const char *c_src, const char *c_dest);

#endif

