/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef ENDPOINT_H
#define ENDPOINT_H

#ifdef DOG_WINDOWS
/*
 * Windows compatibility layer for signal handling
 * DOG_WINDOWS macro is defined when compiling for Windows systems
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
static inline int _sigaction_stub(int sig, const struct sigaction *act, struct sigaction *oldac __UNUSED__) {
        signal(sig, act->sa_handler);
        return 0;
}
#endif
#endif

extern int sigint_handler;

void unit_sigint_handler(int sig);
void dog_stop_server_tasks(void);
void dog_server_crash_check(void);
void restore_server_config(void);

void dog_exec_samp_server(char *gamemode_arg, const char *server_bin);
void dog_exec_omp_server(char *gamemode_arg, const char *server_bin);

#endif
