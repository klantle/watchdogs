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
#include "units.h"
#include "debug.h"

///
/// Comparing
///
///-----without debug build
/// Thread 1 "watchdogs" received signal SIGSEGV, Segmentation fault.
/// Download failed: Invalid argument.  Continuing without source file ./string/../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S.
/// __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
/// warning: 660    ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S: No such file or directory
/// (gdb) bt
/// #0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
/// #1  0x000055555555c53c in write_callbacks ()
/// #2  0x00007ffff7f5741c in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #3  0x00007ffff7f557f8 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #4  0x00007ffff7f6c4b5 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #5  0x00007ffff7f50fb3 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #6  0x00007ffff7f540e5 in curl_multi_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #7  0x00007ffff7f243ab in curl_easy_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #8  0x0000555555564883 in dep_http_get_content ()
/// #9  0x00005555555677bb in wg_install_depends ()
/// #10 0x0000555555561061 in __command__ ()
/// #11 0x000055555556177c in chain_ret_main ()
/// #12 0x000055555555b67d in main ()
///-----with debug build
/// Thread 1 "watchdogs.debug" received signal SIGSEGV, Segmentation fault.
/// __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
/// warning: 660    ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S: No such file or directory
/// (gdb) bt
/// #0  __memcpy_avx_unaligned_erms () at ../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:660
/// #1  0x000055555555cd09 in write_callbacks (ptr=0x55555572e685, size=1, nmemb=8501, userdata=0x7fffffff5a80) at source/curl.c:210
/// #2  0x00007ffff7f5741c in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #3  0x00007ffff7f557f8 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #4  0x00007ffff7f6c4b5 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #5  0x00007ffff7f50fb3 in ?? () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #6  0x00007ffff7f540e5 in curl_multi_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #7  0x00007ffff7f243ab in curl_easy_perform () from /lib/x86_64-linux-gnu/libcurl.so.4
/// #8  0x00005555555698dd in dep_http_get_content (url=0x7fffffff5d00 "https://api.github.com/repos/Y-Less/sscanf/releases/latest", github_token=0x5555556cbf40 "DO_HERE", out_html=0x7fffffff5ce0)
///     at source/depend.c:176
/// #9  0x000055555556ac71 in dep_gh_latest_tag (user=0x7fffffff6230 "Y-Less", repo=0x7fffffff62b0 "sscanf", out_tag=0x7fffffff5fe0 "", deps_put_size=128) at source/depend.c:535
/// #10 0x000055555556af65 in dep_handle_repo (dep_repo_info=0x7fffffff61d0, deps_put_url=0x7fffffff66e0 "\001", deps_put_size=1024) at source/depend.c:588
/// #11 0x000055555556e317 in wg_install_depends (depends_string=0x5555557135f0 "Y-Less/sscanf?newer samp-incognito/samp-streamer-plugin?newer") at source/depend.c:1627
/// #12 0x00005555555611d3 in __command__ (chain_pre_command=0x0) at source/units.c:517
/// #13 0x0000555555564c64 in chain_ret_main (chain_pre_command=0x0) at source/units.c:1602
/// #14 0x0000555555564f67 in main (argc=1, argv=0x7fffffffcf48) at source/units.c:1666

void __reset_sys(void) {

        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        if (path_access(".watchdogs/crashdetect")) {
            remove(".watchdogs/crashdetect");
        }
        setlocale(LC_ALL, "en_US.UTF-8");
        signal(SIGINT, SIG_DFL);
        wg_sef_fdir_memset_to_null();
        wg_toml_configs();
        wg_stop_server_tasks();
        wg_u_history();
        wg_ptr_command_init = 0;
        wgconfig.wg_sel_stat = 0;
        sigint_handler = 0;
}

void __debug_main_chain_(int debug_hard,
            const char *function,
            const char *pretty_function,
            const char *file, int line) {

        __reset_sys();

#if ! defined(_DBG_PRINT)
        return;
#endif

        if (debug_hard == 1) {
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
                "toml ai key: %s\n]",
                    function, pretty_function,
                    line, file,
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
        } else if (debug_hard == 0) {
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
                "toml ai key: %s\n]",
                    function, pretty_function,
                    line, file,
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
        }

        fflush(stdout);

        return;
}

/*************************************************************************
//
**************************************************************************/

void __debug_function_(const char *function,
            const char *pretty_function,
            const char *file, int line) {
    
#if ! defined (_DBG_PRINT)
        return;
#endif

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
                function, pretty_function,
                line, file,
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

        fflush(stdout);

        return;
}