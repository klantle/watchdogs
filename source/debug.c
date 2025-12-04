#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>
#include <fcntl.h>
#include <math.h>
#include <locale.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <curl/curl.h>

#include "extra.h"
#include "runner.h"
#include "utils.h"

void __reset_sys(void) {
        signal(SIGINT, SIG_DFL);
        wg_sef_fdir_reset();
        wg_toml_configs();
        wg_stop_server_tasks();
        wg_u_history();
        wgconfig.wg_sel_stat = 0;
        sigint_handler = 0;
}

void __debug_main_chain(int debug_info) {
        __reset_sys();
        if (debug_info == 1) {
#if defined(_DBG_PRINT)
            pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGER ");
            printf("[function: %s | "
                "pretty function: %s | "
                "line: %d | "
                "file: %s | "
                "date: %s | "
                "time: %s | "
                "timestamp: %s | "
                "C standard: %ld | "
                "C version: %s | "
                "compiler version: %d | "
                "architecture: %s | "
                "os_type: %s (CRC32) | "
                "pointer_samp: %s | "
                "pointer_openmp: %s | "
                "f_samp: %s (CRC32) | "
                "f_openmp: %s (CRC32) | "
                "toml gamemode input: %s | "
                "toml gamemode output: %s | "
                "toml binary: %s | "
                "toml configs: %s | "
                "toml logs: %s | "
                "toml github tokens: %s | "
                "toml chatbot: %s | "
                "toml ai models: %s | "
                "toml ai key: %s\n",
                    __func__, __PRETTY_FUNCTION__,
                    __LINE__, __FILE__,
                    __DATE__, __TIME__,
                    __TIMESTAMP__,
                    __STDC_VERSION__,
                    __VERSION__,
                    __GNUC__,
#ifdef __x86_64__
                    "x86_64",
#elif defined(__i386__)
                    "i386",
#elif defined(__arm__)
                    "ARM",
#elif defined(__aarch64__)
                    "ARM64",
#else
                    "Unknown",
#endif
                    wgconfig.wg_os_type, wgconfig.wg_ptr_samp,
                    wgconfig.wg_ptr_omp, wgconfig.wg_is_samp, wgconfig.wg_is_omp,
                    wgconfig.wg_toml_proj_input, wgconfig.wg_toml_proj_output,
                    wgconfig.wg_toml_binary, wgconfig.wg_toml_config, wgconfig.wg_toml_logs,
                    wgconfig.wg_toml_github_tokens,
                    wgconfig.wg_toml_chatbot_ai,
                    wgconfig.wg_toml_models_ai,
                    wgconfig.wg_toml_key_ai);
            printf("STDC: %d\n", __STDC__);
            printf("STDC_HOSTED: %d\n", __STDC_HOSTED__);
            printf("BYTE_ORDER: ");
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            printf("Little Endian\n");
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            printf("Big Endian\n");
#else
            printf("Unknown\n");
#endif
#else
            printf("Not defined\n");
#endif
            printf("SIZE_OF_PTR: %zu bytes\n", sizeof(void*));
            printf("SIZE_OF_INT: %zu bytes\n", sizeof(int));
            printf("SIZE_OF_LONG: %zu bytes\n", sizeof(long));
#ifdef __LP64__
            printf("DATA_MODEL: LP64\n");
#elif defined(__ILP32__)
            printf("DATA_MODEL: ILP32\n");
#endif
#ifdef __GNUC__
            printf("GNUC: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif

#ifdef __clang__
            printf("CLANG: %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif
            printf("OS: ");
#ifdef __SSE__
            printf("SSE: Supported\n");
#endif
#ifdef __AVX__
            printf("AVX: Supported\n");
#endif
#ifdef __FMA__
            printf("FMA: Supported\n");
#endif
#endif
            }
}

void __debug_function(void) {
#if defined (_DBG_PRINT)
        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGER ");
        printf("[function: %s | "
                   "pretty function: %s | "
                   "line: %d | "
                   "file: %s | "
                   "date: %s | "
                   "time: %s | "
                   "timestamp: %s | "
                   "C standard: %ld | "
                   "C version: %s | "
                   "compiler version: %d | "
                   "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
        return;
}