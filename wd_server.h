#ifndef SERVER_H
#define SERVER_H

#ifdef WD_WINDOWS
/*
 * Windows compatibility layer for signal handling
 * WD_WINDOWS macro is defined when compiling for Windows systems
 */
/* Stub implementation of sigaction for Windows */
#define sigaction _sigaction_stub
/* Windows doesn't have sigemptyset, define as no-op */
#define sigemptyset(a) ((void)0)
/* SA_RESTART flag not supported on Windows */
#define SA_RESTART 0

/* Simplified sigaction structure for Windows compatibility */
struct sigaction {
        void (*sa_handler)(int);  /* Signal handler function pointer */
        int sa_flags;             /* Signal flags */
};

/**
 * sigaction stub implementation for Windows
 * Maps sigaction to Windows signal() function
 */
#ifndef _sigaction_stub
static inline int _sigaction_stub(int sig, const struct sigaction *act, struct sigaction *oldact) {
        signal(sig, act->sa_handler);
        return 0;
}
#endif
#endif
extern int handle_sigint_status;
extern int server_mode;
void unit_handle_sigint(int sig);
void wd_stop_server_tasks(void);
void wd_display_server_logs(int ret);
void wd_server_crash_check(void);
void restore_samp_config(void);
void restore_omp_config(void);
void wd_run_samp_server(const char *gamemode_arg, const char *server_bin);
void wd_run_omp_server(const char *gamemode_arg, const char *server_bin);

#endif
