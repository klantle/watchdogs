#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#if __has_include(<readline/history.h>)
    #include <readline/history.h>
    #define watch_u_history() using_history()
    #define watch_a_history(cmd) add_history(cmd)
#else
    #define watch_u_history()
    #define watch_a_history(cmd)
#endif

#define SEF_PATH_COUNT 28
#define SEF_PATH_SIZE 1024

typedef struct {
        int ipcc;
        int sef_count;
        char sef_found[SEF_PATH_COUNT][SEF_PATH_SIZE];
        const char *os;
        const char *serv_dbg;
        const char *ci_options;
        const char *gm_input;
        const char *g_output;
} wd;

extern wd wcfg;

void reset_watch_sef_dir();
struct struct_of { int (*title)(const char *); };
extern const char* __command[];
extern const size_t __command_len;
int watch_sys(const char *cmd);
void handle_sigint(int sig);
int watch_title(const char *__title);
char* readline_colored(const char* prompt);
void cp_strip_dotfns(char *dst, size_t dst_sz, const char *src);
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
const char* watch_detect_os(void);
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
int watch_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir);
int watch_toml_data(void);
int watch_sef_wcopy(const char *c_src, const char *c_dest);
int watch_sef_wmv(const char *c_src, const char *c_dest);
void install_pawncc_now(void);

#endif

