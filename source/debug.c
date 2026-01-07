/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

/* 
 * System header includes - essential C library headers for:
 * - I/O operations (stdio.h)
 * - Memory management and utility functions (stdlib.h)
 * - Time functions (time.h)
 * - File system status operations (sys/stat.h)
 * - Data types (sys/types.h)
 * - Signal handling (signal.h)
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <time.h>
#include  <sys/stat.h>
#include  <sys/types.h>
#include  <signal.h>

/* Project-specific header includes for modular functionality */
#include  "extra.h"      /* Additional utility functions */
#include  "endpoint.h"   /* Network/communication endpoint handling */
#include  "utils.h"      /* General utility functions */
#include  "units.h"      /* Unit-related functionality */
#include  "debug.h"      /* Debugging utilities and macros */

/*
 * Function: unit_restore
 * Purpose: Restore system to a clean state by:
 *  1. Ensuring the .watchdogs directory exists
 *  2. Cleaning up crash detection files
 *  3. Resetting signal handlers to default
 *  4. Initializing various system components
 *  5. Clearing garbage collection flags
 * Parameters: None
 * Returns: void
 */
void unit_restore(void) {
        /* Create .watchdogs directory if it doesn't exist */
        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        /* Remove crash detection file if it exists */
        if (path_access(".watchdogs/crashdetect"))
            remove(".watchdogs/crashdetect");

        /* Reset SIGINT (Ctrl+C) to default behavior */
        signal(SIGINT, SIG_DFL);

        /* Initialize system components in specific order */
        dog_sef_restore();           /* Restore system error handling */
        dog_toml_configs();          /* Load TOML configuration files */
        dog_stop_server_tasks();     /* Stop any running server tasks */
        dog_history_init();          /* Initialize command history */
        
        /* Reset garbage */
        sigint_handler = 0;
        dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT] = DOG_GARBAGE_ZERO;
}

/*
 * Function: _unit_debugger
 * Purpose: Comprehensive debug information logger with two verbosity levels
 * Parameters:
 *   debug_hard - Verbosity level: 0 (normal) or 1 (detailed)
 *   function - Name of the calling function
 *   pretty_function - Demangled/pretty function name
 *   file - Source file name
 *   line - Line number in source file
 * Returns: void
 */
void _unit_debugger(int debug_hard,
            const char *function,
            const char *pretty_function,
            const char *file, int line) {

        /*
         * First-time initialization section
         */
        if (dogconfig.dog_garbage_access[DOG_GARBAGE_UNIT] == DOG_GARBAGE_ZERO)
        {
            /* Initialize console and clear history */
            dog_console_title(NULL);
            dog_history_clear();
            
            /* Set garbage to true|1 */
            dogconfig.dog_garbage_access[DOG_GARBAGE_UNIT] = DOG_GARBAGE_TRUE;
                
#ifdef DOG_WINDOWS
            /* Windows-specific console initialization for ANSI escape codes */
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE) return;

            DWORD dwMode = 0;
            if (!GetConsoleMode(hOut, &dwMode)) {
                return;
            }

            /* Enable virtual terminal processing for ANSI escape codes */
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            dwMode |= ENABLE_PROCESSED_OUTPUT;

            if (!SetConsoleMode(hOut, dwMode)) {
                return;
            }
#endif
        }

        /* Always restore system to clean state before debugging */
        unit_restore();

#if ! defined(_DBG_PRINT)
        /* Early return if debug printing is disabled at compile time */
        return;
#endif

        /* Detailed debug output (debug_hard == 1) */
        if (debug_hard == 1) {
            pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
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
                "toml aio opt: %s | "
                "toml aio packages: %s]\n",
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
                    dogconfig.dog_os_type, dogconfig.dog_ptr_samp,
                    dogconfig.dog_ptr_omp, dogconfig.dog_is_samp, dogconfig.dog_is_omp,
                    dogconfig.dog_toml_proj_input, dogconfig.dog_toml_proj_output,
                    dogconfig.dog_toml_binary, dogconfig.dog_toml_config, dogconfig.dog_toml_logs,
                    dogconfig.dog_toml_github_tokens, dogconfig.dog_toml_aio_opt, dogconfig.dog_toml_packages);
                    
            /* Additional system information for detailed debugging */
            printf("STDC: %d\n", __STDC__);                     /* C standard compliance */
            printf("STDC_HOSTED: %d\n", __STDC_HOSTED__);       /* Hosted vs freestanding */
            
            /* Endianness detection */
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
            
            /* Data type sizes for platform portability debugging */
            printf("SIZE_OF_PTR: %zu bytes\n", sizeof(void*));
            printf("SIZE_OF_INT: %zu bytes\n", sizeof(int));
            printf("SIZE_OF_LONG: %zu bytes\n", sizeof(long));
            
            /* Data model detection (32-bit vs 64-bit) */
#ifdef __LP64__
            printf("DATA_MODEL: LP64\n");
#elif defined(__ILP32__)
            printf("DATA_MODEL: ILP32\n");
#endif
            
            /* Compiler version information */
#ifdef __GNUC__
            printf("GNUC: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif

#ifdef __clang__
            printf("CLANG: %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif
            
            /* CPU instruction set support detection */
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
            /* Normal debug output - less verbose than detailed mode */
            pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
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
                "toml github tokens: %s]\n",
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
                    dogconfig.dog_os_type, dogconfig.dog_ptr_samp,
                    dogconfig.dog_ptr_omp, dogconfig.dog_is_samp, dogconfig.dog_is_omp,
                    dogconfig.dog_toml_proj_input, dogconfig.dog_toml_proj_output,
                    dogconfig.dog_toml_binary, dogconfig.dog_toml_config, dogconfig.dog_toml_logs,
                    dogconfig.dog_toml_github_tokens);
        }

        /* Ensure all output is flushed to the console immediately */
        fflush(stdout);

        return;
}

/*
 * Function: _minimal_debugger
 * Purpose: Lightweight debug information logger with basic system info
 * Parameters:
 *   function - Name of the calling function
 *   pretty_function - Demangled/pretty function name
 *   file - Source file name
 *   line - Line number in source file
 * Returns: void
 */
void _minimal_debugger(const char *function,
            const char *pretty_function,
            const char *file, int line) {

#if ! defined (_DBG_PRINT)
        /* Early return if debug printing is disabled at compile time */
        return;
#endif

        /* Print minimal debug information with colored output */
        pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
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
                   "architecture: %s]\n",
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

        /* Ensure output is flushed to console */
        fflush(stdout);

        return;
}
