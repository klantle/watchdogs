#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#if __has_include(<readline/history.h>)
    #include <readline/history.h>
    #define watchdogs_u_history() using_history()
    #define watchdogs_a_history(cmd) add_history(cmd)
#else
    #define watchdogs_u_history()
    #define watchdogs_a_history(cmd)
#endif

#define SEF_PATH_COUNT 28
#define SEF_PATH_SIZE 1024

typedef struct {
        int init_ipcc;
        int watchdogs_sef_count;
        char watchdogs_sef_found[SEF_PATH_COUNT][SEF_PATH_SIZE];
        const char *watchdogs_os;
        const char *server_or_debug;
        const char *wd_compiler_opt;
        const char *wd_gamemode_input;
        const char *wd_gamemode_output;
} wd;

extern wd wcfg;

void reset_watchdogs_sef_dir();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
void watchdogs_reset_var(void);
int watchdogs_sys(const char *cmd);
void handle_sigint(int sig);
int watchdogs_title(const char *__title);
void copy_strip_dotfns(char *dst, size_t dst_sz, const char *src);
void escape_quotes(char *dest, size_t size, const char *src);
extern const char* find_cl_command(
    const char *ptr_command,
    const char *__commands[],
    size_t num_cmds,
    int *out_distance
);
void printf_color(const char *color, const char *format, ...);
void println(const char* fmt, ...);
void printf_succes(const char *format, ...);
void printf_info(const char *format, ...);
void printf_warning(const char *format, ...);
void printf_error(const char *format, ...);
void printf_crit(const char *format, ...);
const char* watchdogs_detect_os(void);
int signal_system_os(void);
int kill_process(const char *name);
void kill_process_safe(const char *name);
int dir_exists(const char *path);
int path_exists(const char *path);
int dir_writable(const char *path);
int is_regular_file(const char *path);
int is_same_file(const char *a, const char *b);
int ensure_parent_dir(char *out_parent, size_t n, const char *dest);
int cp_f_content(const char *src, const char *dst);
int watchdogs_toml_data(void);
int watchdogs_sef_fdir(const char *sef_path, const char *sef_name);
int watchdogs_sef_wcopy(const char *c_src, const char *c_dest);
int watchdogs_sef_wmv(const char *c_src, const char *c_dest);
void install_pawncc_now(void);

#endif

