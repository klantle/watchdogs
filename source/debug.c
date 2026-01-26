/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

/* Project-specific header includes for modular functionality */
#include  "endpoint.h"   /* Network/communication endpoint handling */
#include  "utils.h"      /* General utility functions */
#include  "units.h"      /* Unit-related functionality */
#include  "crypto.h"     /* For CRC32 init */
#include  "compiler.h"   /* compiler_is_err */
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
static void unit_restore(void) {

        /* Create .watchdogs directory if it doesn't exist */
        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        /* Reset SIGINT (Ctrl+C) to default behavior */
        signal(SIGINT, SIG_DFL);

        /* Initialize system components in specific order */
        dog_sef_path_revert();           /* Restore system error handling */
        dog_configure_toml();          /* Load TOML configuration files */
        dog_stop_server_tasks();     /* Stop any running server tasks */
        
        /* Reset garbage */
        sigint_handler = 0;
        compiler_is_err
            = false;
        unit_selection_stat
            = false;
}

/*
 * Function: _unit_debugger
 * Purpose: Comprehensive debug information logger with two verbosity levels
 * Parameters:
 *   hard_debug - Verbosity level: 0 (normal) or 1 (detailed)
 *   function - Name of the calling function
 *   file - Source file name
 *   line - Line number in source file
 * Returns: void
 */
void _unit_debugger(int hard_debug,
            const char *function,
            const char *file, int line) {

        /*
         * First-time initialization section
         */
        static bool unit_initial = false;
        if (!unit_initial) {
            unit_initial = true;

            /* Initialize console and clear history */
            clear_history();
            dog_console_title(NULL);
            crypto_crc32_init_table();
                
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

        /* Detailed debug output (hard_debug == 1) */
        if (hard_debug == 1) {
            pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
            printf("[function: %s | "
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
                    function,
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
                    dogconfig.dog_toml_server_binary, dogconfig.dog_toml_server_config, dogconfig.dog_toml_server_logs,
                    dogconfig.dog_toml_github_tokens, dogconfig.dog_toml_all_flags, dogconfig.dog_toml_packages);
                    
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
            
        } else if (hard_debug == 0) {
            /* Normal debug output - less verbose than detailed mode */
            pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
            printf("[function: %s | "
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
                    function,
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
                    dogconfig.dog_toml_server_binary, dogconfig.dog_toml_server_config, dogconfig.dog_toml_server_logs,
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
 *   file - Source file name
 *   line - Line number in source file
 * Returns: void
 */
void _minimal_debugger(const char *function,
            const char *file, int line) {

#if ! defined (_DBG_PRINT)
        /* Early return if debug printing is disabled at compile time */
        return;
#endif

        /* Print minimal debug information with colored output */
        pr_color(stdout, DOG_COL_YELLOW, "-DEBUGGER ");
        printf("[function: %s | "
                   "line: %d | "
                   "file: %s | "
                   "date: %s | "
                   "time: %s | "
                   "timestamp: %s | "
                   "C standard: %ld | "
                   "C version: %s | "
                   "compiler version: %d | "
                   "architecture: %s]\n",
                function,
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
