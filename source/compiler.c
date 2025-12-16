#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#include "utils.h"

#ifdef WG_LINUX
    #include <fcntl.h>
    #include <spawn.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#include "units.h"
#include "extra.h"
#include "library.h"
#include "debug.h"
#include "crypto.h"
#include "compiler.h"

#ifndef WG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 0x1E
#endif
static io_compilers wg_compiler_sys = { 0 };

static void cause_compiler_expl(const char *log_file, const char *wgoutput, int debug);

/* Main compiler execution function that orchestrates the entire compilation process */
int wg_run_compiler(const char *args, const char *compile_args,
                    const char *second_arg, const char *four_arg,
                    const char *five_arg, const char *six_arg,
                    const char *seven_arg, const char *eight_arg,
                    const char *nine_arg) {

		/* Debug information section */
        __debug_function();

        const int aio_extra_options = 7; /* Number of extra compiler options to process */
        wgconfig.wg_compiler_stat = 0; /* Reset compiler status flag */
        io_compilers comp; /* Local compiler configuration structure */
        io_compilers *ptr_io = &comp; /* Pointer to compiler configuration for easy access */
#ifdef WG_WINDOWS
        /* Windows-specific process handling structures */
        PROCESS_INFORMATION pi;
        STARTUPINFO si = { sizeof(si) };
        SECURITY_ATTRIBUTES sa = { sizeof(sa) };
#endif
        /* Timing structures for measuring compilation duration */
        struct timespec start = { 0 },
                        end = { 0 };
        double compiler_dur; /* Compilation duration in seconds */

        /* Array of compiler arguments for easy iteration and processing */
        const char* compiler_args[] = {
            second_arg, four_arg,
            five_arg, six_arg,
            seven_arg, eight_arg,
            nine_arg
        };

        FILE *this_proc_fileile; /* File pointer for process output handling */
        char proj_parse[WG_PATH_MAX] = { 0 }; /* project parse char */
        char size_log[WG_MAX_PATH * 4]; /* Buffer for log file reading */
        char run_cmd[WG_PATH_MAX + 258]; /* Buffer for constructing system commands */
        char include_aio_path[WG_PATH_MAX * 2] = { 0 }; /* Buffer for include paths concatenation */
        char _compiler_input_[WG_MAX_PATH + WG_PATH_MAX] = { 0 }; /* Buffer for final compiler command */
        char size_path_include[WG_MAX_PATH] = { 0 }; /* include path new char */

        /* Pointers for path manipulation - extracting directory and filename */
        char *compiler_last_slash = NULL;
        char *compiler_back_slash = NULL;

        char *procure_string_pos = NULL; /* Pointer for string searching operations */
        char compiler_extra_options[WG_PATH_MAX] = { 0 }; /* Buffer for additional compiler flags */

        /* Array of debug memory flags that need special handling */
        const char *debug_options[] = {"-d1", "-d3"};
        size_t size_debug_options = sizeof(debug_options);
        size_t size_debug_options_zero = sizeof(debug_options[0]);
        size_t debug_flag_count = size_debug_options / size_debug_options_zero;

        /* Flag variables tracking specific compiler options */
        int compiler_debugging = 0; /* Debug mode flag */
        int compiler_has_watchdogs = 0, compiler_has_debug = 0; /* Feature flags */
        int compiler_has_clean = 0, compiler_has_assembler = 0; /* Compilation mode flags */
        int compiler_has_recursion = 0, compiler_has_verbose = 0; /* Output control flags */
        int compiler_has_compact = 0; /* Character encoding compact flag */

        /* Determine the compiler executable name based on operating system */
        const char *ptr_pawncc = NULL;
        if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_WINDOWS))
            ptr_pawncc = "pawncc.exe";
        else if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX))
            ptr_pawncc = "pawncc";

        /* Determine include path based on server environment configuration */
        char path_include[WG_PATH_MAX];
        if (wg_server_env() == 1)
            snprintf(path_include, WG_PATH_MAX, "pawno/include");
        else if (wg_server_env() == 2)
            snprintf(path_include, WG_PATH_MAX, "qawno/include");

        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        /* Check and clean up existing compiler log file */
        int _wg_log_acces = path_access(".watchdogs/compiler.log");
        if (_wg_log_acces)
            remove(".watchdogs/compiler.log");

        /* Create fresh compiler log file for this compilation session */
        FILE *wg_log_files = fopen(".watchdogs/compiler.log", "w");
        if (wg_log_files != NULL)
            fclose(wg_log_files);
        else {
            pr_error(stdout, "can't create .watchdogs/compiler.log!");
            __debug_function(); /* call debugger function */
        }

        compiler_memory_clean(); /* Perform memory cleanup before compilation */

        /* Search for pawncc compiler executable in current directory */
        int __find_pawncc = wg_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc) /* Compiler found - proceed with compilation */
        {
            FILE *this_proc_file;
            this_proc_file = fopen("watchdogs.toml", "r");
            if (!this_proc_file) {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                __debug_function(); /* call debugger function */
                goto compiler_end;
            }

            /* Parse TOML configuration file for compiler settings */
            char wg_buf_err[WG_PATH_MAX];
            toml_table_t *wg_toml_config;
            wg_toml_config = toml_parse_file(this_proc_file, wg_buf_err, sizeof(wg_buf_err));

            if (this_proc_file) {
                fclose(this_proc_file);
            }

            if (!wg_toml_config) {
                pr_error(stdout, "failed to parse the watchdogs.toml....: %s", wg_buf_err);
                __debug_function(); /* call debugger function */
                goto compiler_end;
            }

            char wg_compiler_input_pawncc_path[WG_PATH_MAX],
                 wg_compiler_input_proj_path[WG_PATH_MAX];
            snprintf(wg_compiler_input_pawncc_path,
                    sizeof(wg_compiler_input_pawncc_path), "%s", wgconfig.wg_sef_found_list[0]);

            /* Test compiler execution to verify it works correctly */
            snprintf(run_cmd, sizeof(run_cmd),
                "/bin/sh -c \"%s > .watchdogs/compiler_test.log 2>&1\"",
                wg_compiler_input_pawncc_path);
            wg_run_command(run_cmd);

            this_proc_fileile = fopen(".watchdogs/compiler_test.log", "r");
            if (!this_proc_fileile) {
                pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
                __debug_function(); /* call debugger function */
            }

            /* Process and categorize compiler arguments from command line */
            for (int i = 0; i < aio_extra_options; ++i) {
                if (compiler_args[i] != NULL) {
                    if (strfind(compiler_args[i], "--detailed", true) ||
                        strfind(compiler_args[i], "--watchdogs", true))
                        ++compiler_has_watchdogs; /* Enable detailed watchdogs analysis */
                    if (strfind(compiler_args[i], "--debug", true))
                        ++compiler_has_debug; /* Enable debug compilation mode */
                    if (strfind(compiler_args[i], "--clean", true))
                        ++compiler_has_clean; /* Enable clean compilation (remove output on error) */
                    if (strfind(compiler_args[i], "--assembler", true))
                        ++compiler_has_assembler; /* Enable assembler output */
                    if (strfind(compiler_args[i], "--recursion", true))
                        ++compiler_has_recursion; /* Enable recursion detection */
                    if (strfind(compiler_args[i], "--prolix", true))
                        ++compiler_has_verbose; /* Enable verbose output */
                    if (strfind(compiler_args[i], "--compact", true))
                        ++compiler_has_compact; /* Enable specific character encoding compact */
                }
            }

            /* Extract compiler-specific configuration from TOML */
            toml_table_t *wg_compiler = toml_table_in(wg_toml_config, "compiler");
#if defined(_DBG_PRINT)
            if (!wg_compiler)
                printf("%s not exists in line:%d", "compiler", __LINE__);
#endif
            if (wg_compiler) /* Process compiler configuration if present */
            {
                /* Read compiler options array from TOML configuration */
                toml_array_t *option_arr = toml_array_in(wg_compiler, "option");
#if defined(_DBG_PRINT)
                if (!option_arr)
                    printf("%s not exists in line:%d", "option", __LINE__);
#endif
                if (option_arr)
                {
                    char *merged = NULL; /* Dynamic string to accumulate compiler flags */
                    size_t toml_array_size;
                    toml_array_size = toml_array_nelem(option_arr);

                    /* Process each compiler option from TOML configuration */
                    for (size_t i = 0; i < toml_array_size; i++)
                    {
                        toml_datum_t toml_option_value;
                        toml_option_value = toml_string_at(option_arr, i);
                        if (!toml_option_value.ok)
                            continue;

                        /* Extract flag prefix for validation against compiler help */
                        char flag_to_search[3] = { 0 };
                        size_t size_flag_to_search = sizeof(flag_to_search);
                        if (strlen(toml_option_value.u.s) >= 2) {
                            snprintf(flag_to_search,
                                     size_flag_to_search,
                                     "%.2s",
                                     toml_option_value.u.s);
                        } else {
                            strncpy(flag_to_search, toml_option_value.u.s, size_flag_to_search - 1);
                        }

                        /* Validate flag by checking against compiler help output */
                        if (this_proc_fileile != NULL) {
                            rewind(this_proc_fileile);
                            while (fgets(size_log, sizeof(size_log), this_proc_fileile) != NULL) {
                                if (strfind(size_log, "error while loading shared libraries:", true) ||
                                    strfind(size_log, "library \"libpawnc.so\" not found", true)) {
                                    wg_printfile(".watchdogs/compiler_test.log");
                                    goto compiler_end;
                                }
                            }
                        }

                        /* Ensure option starts with dash indicating it's a flag */
                        char *opt = toml_option_value.u.s;
                        while (*opt && isspace(*opt)) ++opt;

                        if (*opt != '-') {
not_valid_flag_options:
                            printf("[WARN]: "
                                "compiler option ");
                            pr_color(stdout, FCOLOUR_GREEN, "\"%s\" ", toml_option_value.u.s);
                            println(stdout, "not valid flag options!..");
                            goto compiler_end;
                        }

                        /* Track if debug flag is present */
                        if (strfind(toml_option_value.u.s, "-d", true))
                          ++compiler_debugging;

                        /* Dynamically build concatenated compiler options string */
                        size_t old_len = merged  ? strlen(merged) : 0,
                               new_len = old_len + strlen(toml_option_value.u.s) + 2;

                        char *tmp = wg_realloc(merged, new_len);
                        if (!tmp) {
                            wg_free(merged);
                            wg_free(toml_option_value.u.s);
                            merged = NULL;
                            break;
                        }

                        merged = tmp;

                        if (!old_len)
                            snprintf(merged, new_len, "%s", toml_option_value.u.s);
                        else
                            snprintf(merged + old_len,
                                     new_len - old_len,
                                     " %s", toml_option_value.u.s);

                        wg_free(toml_option_value.u.s);
                        toml_option_value.u.s = NULL;
                    }

                    /* Clean up temporary test files */
                    if (this_proc_fileile) {
                        fclose(this_proc_fileile);
                        if (path_access(".watchdogs/compiler_test.log"))
                            remove(".watchdogs/compiler_test.log");
                    }

                    /* Store accumulated compiler options in global config */
                    if (merged) {
                        wgconfig.wg_toml_aio_opt = merged;
                    }
                    else {
                        wgconfig.wg_toml_aio_opt = strdup("");
                    }

                    /*
                    * Special handling: remove conflicting debug flags when debug mode is enabled
                    * Security-enhanced version with bounds checking and error handling
                    */
                    if (compiler_has_debug != 0 && compiler_debugging != 0) {
                        /* Validate array sizes to prevent division by zero */
                        if (size_debug_options == 0 ||
                            size_debug_options_zero == 0)
                        {
                            pr_error(stdout,
                                "Invalid debug flag array configuration");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }

                        /* Prevent integer overflow in division */
                        if (size_debug_options < size_debug_options_zero ||
                            size_debug_options % size_debug_options_zero != 0) {
                            pr_error(stdout,
                                "Debug flag array size mismatch");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }

                        /* Validate maximum reasonable flag count */
                        const size_t MAX_DEBUG_FLAGS = 256;
                        if (debug_flag_count > MAX_DEBUG_FLAGS) {
                            pr_error(stdout,
                                "Excessive debug flag count: %zu",
                                debug_flag_count);
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }

                        /* Ensure wgconfig.wg_toml_aio_opt is valid */
                        if (wgconfig.wg_toml_aio_opt == NULL) {
                            pr_error(stdout,
                                "Configuration string is NULL");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }

                        /* Track original buffer for potential rollback */
                        char *original_config = strdup(wgconfig.wg_toml_aio_opt);
                        if (original_config == NULL) {
                            pr_error(stdout,
                                "Memory allocation failed for config backup");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }

                        /* Calculate safe working buffer size */
                        size_t config_buffer_size = strlen(wgconfig.wg_toml_aio_opt) + 1;
                        size_t max_safe_shifts = config_buffer_size * 2;

                        for (size_t i = 0; i < debug_flag_count; i++) {
                            /* Validate array bounds */
                            if (i >= (size_debug_options / sizeof(debug_options[0]))) {
                                pr_warning(stdout,
                                    "Debug flag index %zu out of bounds", i);
                                continue;
                            }

                            const char
                                *debug_flag = debug_options[i];
                            if (debug_flag == NULL) {
                                pr_warning(stdout,
                                    "NULL debug flag at index %zu", i);
                                continue;
                            }

                            size_t flag_length = strlen(debug_flag);
                            if (flag_length == 0) {
                                continue;
                            }

                            /* Prevent overly long flags that could be malicious */
                            const size_t MAX_FLAG_LENGTH = WG_MAX_PATH;
                            if (flag_length > MAX_FLAG_LENGTH) {
                                pr_warning(stdout,
                                    "Suspiciously long debug flag at index %zu", i);
                                continue;
                            }

                            /* Safe in-place removal with loop prevention */
                            char *config_ptr = wgconfig.wg_toml_aio_opt;
                            size_t shift_count = 0;

                            while (shift_count < max_safe_shifts) {
                                char *flag_pos = strstr(config_ptr, debug_flag);

                                if (flag_pos == NULL) {
                                    break;
                                }

                                /* Verify flag is a complete token (not part of another flag) */
                                bool is_complete_token = true;

                                if (flag_pos > wgconfig.wg_toml_aio_opt) {
                                    char prev_char = *(flag_pos - 1);
                                    if (isalnum((unsigned char)prev_char) ||
                                        prev_char == '_') {
                                        is_complete_token = false;
                                    }
                                }

                                char next_char = *(flag_pos + flag_length);
                                if (isalnum((unsigned char)next_char) ||
                                    next_char == '_') {
                                    is_complete_token = false;
                                }

                                if (!is_complete_token) {
                                    config_ptr = flag_pos + 1;
                                    continue;
                                }

                                /* Calculate remaining length safely */
                                char *prev_flag = flag_pos + flag_length;

                                /* Perform safe memory move with bounds checking */
                                if ((flag_pos - wgconfig.wg_toml_aio_opt) + strlen(prev_flag) + 1 < config_buffer_size)
                                {
                                    memmove(flag_pos, prev_flag, strlen(prev_flag) + 1);
                                    /* Recalculate buffer size after modification */
                                    config_buffer_size = strlen(wgconfig.wg_toml_aio_opt) + 1;
                                } else {
                                    pr_error(stdout,
                                        "Buffer overflow detected during flag removal");
                                    __debug_function(); /* call debugger function */
                                    strncpy(wgconfig.wg_toml_aio_opt, original_config, config_buffer_size);
                                    wg_free(original_config);
                                    goto compiler_end;
                                }

                                ++shift_count;
                            }

                            if (shift_count >= max_safe_shifts) {
                                pr_warning(stdout,
                                    "Excessive flag removals for '%s', possible loop", debug_flag);
                            }
                        }

                        /* Sanity check: ensure string is still null-terminated */
                        wgconfig.wg_toml_aio_opt[config_buffer_size - 1] = '\0';

                        /* Log changes for audit trail */
                        if (strcmp(original_config, wgconfig.wg_toml_aio_opt) != 0) {
#if defined (_DBG_PRINT)
                            pr_info(stdout,"Debug flags removed. Original: '%s', Modified: '%s'",
                                    original_config, wgconfig.wg_toml_aio_opt);
#endif
                        }

                        wg_free(original_config);

                        /* Final validation */
                        if (strlen(wgconfig.wg_toml_aio_opt) >= config_buffer_size) {
                            pr_error(stdout,
                                "Configuration string corruption detected");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }
                    }
                }

                /* Append command-line specified options to TOML options */
                if (compiler_has_debug > 0)
                    strcat(compiler_extra_options, " -d2 ");
                if (compiler_has_assembler > 0)
                    strcat(compiler_extra_options, " -a ");
                if (compiler_has_recursion > 0)
                    strcat(compiler_extra_options, " -R+ ");
                if (compiler_has_verbose > 0)
                    strcat(compiler_extra_options, " -v2 ");
                if (compiler_has_compact > 0)
                    strcat(compiler_extra_options, " -C+ ");

                int rate_debugger = 0;
#if defined(_DBG_PRINT)
                ++rate_debugger;
#endif
                if (compiler_has_watchdogs)
                    ++rate_debugger;

                /* Safely concatenate extra options ensuring buffer doesn't overflow */
                if (strlen(compiler_extra_options) > 0) {
                    size_t current_len = strlen(wgconfig.wg_toml_aio_opt);
                    size_t extra_opt_len = strlen(compiler_extra_options);

                    if (current_len + extra_opt_len < sizeof(wgconfig.wg_toml_aio_opt))
                        strcat(wgconfig.wg_toml_aio_opt, compiler_extra_options);
                    else
                        strncat(wgconfig.wg_toml_aio_opt, compiler_extra_options,
                            sizeof(wgconfig.wg_toml_aio_opt) - current_len - 1);
                }

                /* Process include paths from TOML configuration */
                toml_array_t *toml_include_path = toml_array_in(wg_compiler, "include_path");
#if defined(_DBG_PRINT)
                if (!toml_include_path)
                    printf("%s not exists in line:%d", "include_path", __LINE__);
#endif
                if (toml_include_path)
                {
                    int toml_array_size;
                    toml_array_size = toml_array_nelem(toml_include_path);

                    /* Build compiler include path arguments from TOML array */
                    for (int i = 0; i < toml_array_size; i++)
                    {
                        toml_datum_t path_val = toml_string_at(toml_include_path, i);
                        if (path_val.ok)
                        {
                            char size_path_val[WG_PATH_MAX + 26];
                            wg_strip_dot_fns(size_path_val, sizeof(size_path_val), path_val.u.s);
                            if (size_path_val[0] == '\0')
                                continue;

                            /* Add space separator between multiple include paths */
                            if (i > 0)
                            {
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1)
                                {
                                    snprintf(include_aio_path + cur,
                                        sizeof(include_aio_path) - cur,
                                        " ");
                                }
                            }

                            /* Format include path in compiler-specific format (-i"path") */
                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1)
                            {
                                snprintf(include_aio_path + cur,
                                    sizeof(include_aio_path) - cur,
                                    "-i\"%s\" ",
                                    size_path_val);
                            }
                            wg_free(path_val.u.s);
                        }
                    }
                }

                if (strfind(compile_args, "../", true)) {
                    size_t w = 0;
                    size_t j;
                    bool found_path = false;

                    for (j = 0; compile_args[j] != '\0'; ) {
                        if (!found_path && strncmp(&compile_args[j], "../", 3) == 0) {
                            j += 3;

                            while (compile_args[j] != '/' &&
                                compile_args[j] != '"' &&
                                compile_args[j] != ' ' &&
                                compile_args[j] != '\0') {
                                proj_parse[w++] = compile_args[j++];
                            }

                            if (compile_args[j] == '/') {
                                proj_parse[w++] = compile_args[j++];
                            }

                            found_path = true;
                            break;
                        } else {
                            j++;
                        }
                    }

                    if (found_path && w > 0) {
                        memmove(proj_parse + 3, proj_parse, w);
                        memcpy(proj_parse, "../", 3);
                        w += 3;
                        proj_parse[w] = '\0';

                        if (proj_parse[w-1] != __PATH_CHR_SEP_LINUX) {
                            strcat(proj_parse, __PATH_STR_SEP_LINUX);
                        }
                    } else {
                        strcpy(proj_parse, "../");
                    }

                    snprintf(size_path_include, sizeof(size_path_include),
                            "\"%s\" "
                            "-i\"%spawno/include/\" "
                            "-i\"%sqawno/include/\" "
                            "-i\"%sgamemodes/\"",
                            path_include, proj_parse, proj_parse, proj_parse);

                    strcpy(path_include, size_path_include);

                    pr_info(stdout,
                        "Parent dir detected: %s - new include path created: %s",
                        compile_args, path_include);
                }

                /* Main compilation logic block - handles both default and specific file compilation */
                if (compile_args == NULL || *compile_args == '\0' || (compile_args[0] == '.' && compile_args[1] == '\0'))
                {
                    static int compiler_targets = 0;
                    if (compiler_targets != 1) {
                        pr_color(stdout, FCOLOUR_YELLOW,
                                "=== COMPILER TARGET ===\n");
                        printf("   ** This notification appears only once.\n"
                               "    * You can set the target using args in the command.\n");
                        printf("   * You run the command without any args.\n"
                            "   * Do you want to compile for " FCOLOUR_GREEN "%s " FCOLOUR_DEFAULT "(just enter), \n"
                            "   * or do you want to compile for something else?\n"
                            "   ** Enter your input here:", wgconfig.wg_toml_proj_input);
                        char *proj_targets = readline(" ");
                        if (strlen(proj_targets) > 0) {
                            wgconfig.wg_toml_proj_input = strdup(proj_targets);
                        }
                        wg_free(proj_targets);
                        compiler_targets = 1;
                    }

                    /* Default compilation: use gamemode from TOML configuration */
#ifdef WG_WINDOWS
                    sa.bInheritHandle = TRUE; /* Set the `bInheritHandle` flag in the SECURITY_ATTRIBUTES structure to TRUE. This allows any handle created with this security attribute to be inherited by child processes. Inheritance is crucial for redirecting standard handles. */
                    HANDLE hFile = CreateFileA( /* Create or open a file for logging. The 'A' suffix denotes the ANSI (char) version; there's also CreateFileW for Unicode. */
                            ".watchdogs/compiler.log", /* Path to the log file. Relative to the current directory of the calling process. */
                            GENERIC_WRITE, /* Desired access: write-only permission. The process can write to but not read from this file. */
                            FILE_SHARE_READ, /* Sharing mode: other processes can open the file for reading concurrently. This prevents locking conflicts. */
                            &sa, /* Pointer to SECURITY_ATTRIBUTES. Determines inheritance and optionally contains a security descriptor for access control. */
                            CREATE_ALWAYS, /* Creation disposition: always create a new file. If it exists, overwrite and truncate to 0 bytes. Equivalent to POSIX's O_CREAT | O_TRUNC. */
                            FILE_ATTRIBUTE_NORMAL | /* File attributes: normal file without special attributes (not hidden, system, etc.). Combined with: */
                                FILE_FLAG_SEQUENTIAL_SCAN, /* Optimization hint: the file will be read/written sequentially. Windows uses this for better caching and prefetching. */
                            NULL /* Template file: not used here, must be NULL for file creation. */
                    );
                    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; /* Startup flags: STARTF_USESTDHANDLES indicates that hStdInput, hStdOutput, and hStdError members are valid. STARTF_USESHOWWINDOW indicates wShowWindow is valid. */
                    si.wShowWindow = SW_HIDE; /* Window display state: hide the window completely. For console applications, this prevents a console window from appearing. */

                    if (hFile != INVALID_HANDLE_VALUE) { /* Make sure file creation succeeded. INVALID_HANDLE_VALUE is the error return (defined as (HANDLE)-1). */
                        si.hStdOutput = hFile; /* Redirect child's standard output to the log file. All printf/cout output will go to the file. */
                        si.hStdError = hFile; /* Redirect child's standard error to the same log file. Error messages and normal output are combined. */
                    }

                    /* Construct full compiler command string for Windows */
                    int ret_command = 0; /* Store snprintf return value. Positive = characters written (excluding null), negative = error. */
                    ret_command = snprintf(_compiler_input_, /* Build the command line as a single string. Windows expects a command line similar to what you'd type in cmd.exe. */
                            sizeof(_compiler_input_),
                                    "%s " "%s " "-o%s " "%s " "%s " "-i%s", /* Format: compiler executable, input file, output flag, output file, options, include paths, include flag, include directories. */
                                    wg_compiler_input_pawncc_path, /* Path to the pawncc compiler (e.g., "pawncc.exe" or full path). */
                                    wgconfig.wg_toml_proj_input, /* Input source file (e.g., "gamemodes/main.pwn"). */
                                    wgconfig.wg_toml_proj_output, /* Output compiled file (e.g., "main.amx"). */
                                    wgconfig.wg_toml_aio_opt, /* Compiler options from TOML config (e.g., "-d3 -O2"). */
                                    include_aio_path, /* Additional include paths from TOML (e.g., "-i\"libs\" -i\"include\""). */
                                    path_include); /* Default include path (e.g., "pawno/include"). */
                    if (rate_debugger > 0) {
                        printf("{\n");
                        printf("  \"mode\": \"%s\",\n", "non args");
                        printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                        printf("  \"project\": \"%s\",\n", wgconfig.wg_toml_proj_input);
                        printf("  \"output\": \"%s\",\n", wgconfig.wg_toml_proj_output);
                        printf("  \"include\": \"%s\"\n", path_include);
                        printf("  \"command\": \"%s\"\n", _compiler_input_);
                        printf("}\n");
                        fflush(stdout);
                    }
                    if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) { /* Validate command construction: positive means success, less than buffer size means no truncation. */
                        BOOL win32_process_succes; /* Windows boolean type (typedef int). TRUE = non-zero success, FALSE = zero failure. */
                        clock_gettime(CLOCK_MONOTONIC, &start); /* Record start time using monotonic clock for accurate duration measurement. */
                        /* Execute compiler as external process on Windows */
                        win32_process_succes = CreateProcessA( /* The core Windows process creation function. */
                                NULL, /* Application name (lpApplicationName). NULL means the executable name is in the command line. The system searches PATH if no directory specified. */
                                _compiler_input_, /* Command line (lpCommandLine). Full command with arguments. Note: This buffer may be modified by CreateProcess, so it shouldn't be constant. */
                                NULL, /* Process security attributes (lpProcessAttributes). NULL = default security, non-inheritable handle. */
                                NULL, /* Thread security attributes (lpThreadAttributes). NULL = default security, non-inheritable handle. */
                                TRUE, /* Handle inheritance (bInheritHandles). TRUE = child inherits inheritable handles (like our log file). */
                                CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS, /* Creation flags: CREATE_NO_WINDOW = no console window for console apps. ABOVE_NORMAL_PRIORITY_CLASS = slightly higher CPU priority. */
                                NULL, /* Environment block (lpEnvironment). NULL = inherit parent's environment. */
                                NULL, /* Current directory (lpCurrentDirectory). NULL = inherit parent's current directory. */
                                &si, /* STARTUPINFO structure. Contains std handles, window properties, etc. Must have cb (size) set (done earlier). */
                                &pi); /* PROCESS_INFORMATION structure. Receives handles and IDs for the new process and thread. */
                        if (win32_process_succes) { /* CreateProcess succeeded. */
                            WaitForSingleObject(pi.hProcess, INFINITE); /* Wait indefinitely for the child process to terminate. Blocks the calling thread until the process exits. */

                            clock_gettime(CLOCK_MONOTONIC, &end); /* Record completion time after waiting. */

                            DWORD proc_exit_code; /* 32-bit unsigned integer for the process exit code. Windows convention: 0 = success, non-zero = error. */
                            GetExitCodeProcess(pi.hProcess, &proc_exit_code); /* Retrieve the exit code of the terminated process. For active processes, returns STILL_ACTIVE (259). */

                            CloseHandle(pi.hProcess); /* Release the process handle. Essential to prevent handle leaks. Even after process termination, the handle consumes system resources. */
                            CloseHandle(pi.hThread); /* Release the primary thread handle. The thread terminates when the process does, but the handle remains. */
                        } else { /* CreateProcess failed. */
                            clock_gettime(CLOCK_MONOTONIC, &end); /* Still record end time for consistency. */
                            pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError()); /* Print error with Windows error code. Common errors: ERROR_FILE_NOT_FOUND, ERROR_ACCESS_DENIED, ERROR_PATH_NOT_FOUND. */
                            __debug_function(); /* call debugger function */
                        }
                    } else { /* Command string construction failed (buffer too small or snprintf error). */
                        clock_gettime(CLOCK_MONOTONIC, &end); /* Record time for consistent timing output. */
                        pr_error(stdout, "ret_compiler too long!"); /* Error indicating command was truncated or construction failed. Includes function name and line number. */
                        __debug_function(); /* call debugger function */
                        goto compiler_end; /* Jump to cleanup code. Using goto for error handling is common in C for centralized cleanup. */
                    }
                    if (hFile != INVALID_HANDLE_VALUE) { /* If log file was successfully opened... */
                        CloseHandle(hFile); /* Close the file handle. The file remains on disk with the compiler output. */
                    }
#else
                    /* POSIX/Linux compilation path */
                    int ret_command = 0;
                    ret_command = snprintf(_compiler_input_, sizeof(_compiler_input_),
                        "%s " "%s " "%s%s " "%s%s " "%s%s",
                        wg_compiler_input_pawncc_path,
                        wgconfig.wg_toml_proj_input,
                        "-o",
                        wgconfig.wg_toml_proj_output,
                        wgconfig.wg_toml_aio_opt,
                        include_aio_path,
                        "-i",
                        path_include);
                    if (ret_command > (int)sizeof(_compiler_input_)) {
                        pr_error(stdout, "ret_compiler too long!");
                        __debug_function(); /* call debugger function */
                        goto compiler_end;
                    }
                    if (rate_debugger > 0) {
                        printf("{\n");
                        printf("  \"mode\": \"%s\",\n", "non args");
                        printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                        printf("  \"project\": \"%s\",\n", wgconfig.wg_toml_proj_input);
                        printf("  \"output\": \"%s\",\n", wgconfig.wg_toml_proj_output);
                        printf("  \"include\": \"%s\"\n", path_include);
                        printf("  \"command\": \"%s\"\n", _compiler_input_);
                        printf("}\n");
                        fflush(stdout);
                    }
                    /* Tokenize command for posix_spawn argument array */
                    int i = 0;
                    char *wg_compiler_unix_args[WG_MAX_PATH + 256];
                    char *compiler_unix_token = strtok(_compiler_input_, " ");
                    while (compiler_unix_token != NULL) {
                        wg_compiler_unix_args[i++] = compiler_unix_token;
                        compiler_unix_token = strtok(NULL, " ");
                    }
                    wg_compiler_unix_args[i] = NULL;

                    /* Setup file actions to redirect output to log file */
                    posix_spawn_file_actions_t process_file_actions; /* Declare a file actions object to manipulate the child's file descriptors before execution. */
                    posix_spawn_file_actions_init(&process_file_actions); /* Initialize the file actions object. Must be done before any `add` operations. */
                    int posix_logging_file = open(".watchdogs/compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644); /* Open the log file for writing, creating/truncating it. */
                    if (posix_logging_file != -1) { /* Make sure the log file was opened successfully. */
                        posix_spawn_file_actions_adddup2(&process_file_actions,
                                posix_logging_file,
                                STDOUT_FILENO); /* Add an action to duplicate the log file descriptor onto the child's stdout (fd 1). */
                        posix_spawn_file_actions_adddup2(&process_file_actions,
                                posix_logging_file,
                                STDERR_FILENO); /* Add an action to duplicate the same log file descriptor onto the child's stderr (fd 2). */
                        posix_spawn_file_actions_addclose(&process_file_actions,
                                posix_logging_file); /* Add an action to close the original log file descriptor in the child, preventing leaks. */
                    }

                    /* Configure spawn attributes with signal handling */
                    posix_spawnattr_t spawn_attr; /* Declare a spawn attributes object to control process behavior like signals, scheduling, etc. */
                    posix_spawnattr_init(&spawn_attr); /* Initialize the spawn attributes object. */

                    posix_spawnattr_setflags(&spawn_attr,
                        POSIX_SPAWN_SETSIGDEF | /* This flag tells `posix_spawn` to reset the child's signal handlers to their default actions (ignored, terminate, etc.). */
                        POSIX_SPAWN_SETSIGMASK  /* This flag tells `posix_spawn` to apply a specified signal mask to the child process. Here, it blocks certain signals initially. */
                    );

                    pid_t compiler_process_id; /* Variable to store the PID of the newly spawned child process. */

                    /* Execute compiler using posix_spawn for better control */
                    int process_spawn_result = posix_spawnp(&compiler_process_id, /* `posix_spawnp` searches for the executable in the directories listed in the `PATH` environment variable. */
                                wg_compiler_unix_args[0], /* The program to execute (e.g., "gcc", "./script.sh"). */
                                &process_file_actions, /* The file actions object; can be NULL if no file manipulations are needed. */
                                &spawn_attr, /* The spawn attributes object; can be NULL to use default process attributes. */
                                wg_compiler_unix_args, /* The argument array (argv) for the new program, terminated by a NULL pointer. */
                                environ); /* The environment variable array (envp) for the new program. Often `environ` (the parent's environment) is passed. */

                    posix_spawnattr_destroy(&spawn_attr); /* Destroy the spawn attributes object to free its resources. */
                    posix_spawn_file_actions_destroy(&process_file_actions); /* Destroy the file actions object to free its resources. */

                    if (process_spawn_result == 0) { /* `posix_spawn` returns 0 on success, and the child's PID is stored in `compiler_process_id`. */
                        int process_status; /* Variable to store the child's termination status. */
                        int process_timeout_occurred = 0; /* Flag to track if the process was killed due to timeout. */
                        clock_gettime(CLOCK_MONOTONIC, &start); /* Get the start time using a monotonic clock (unaffected by system time changes). */
                        /* Monitor process with timeout to prevent hanging */
                        for (int i = 0; i < POSIX_TIMEOUT; i++) { /* Loop for a predefined number of intervals (e.g., 120 iterations). */
                            int p_result = -1;
                            p_result = waitpid(compiler_process_id, &process_status, WNOHANG); /* Check child's status non-blockingly. Returns PID if child changed state, 0 if still running, or -1 on error. */
                            if (p_result == 0)
                                usleep(0xC350); /* Sleep 50ms (0xC350 microseconds = 50,000 µs) if the process is still running, to avoid busy-waiting. */
                            else if (p_result == compiler_process_id) { /* Child process terminated (normally or by signal). */
                                clock_gettime(CLOCK_MONOTONIC, &end); /* Record the end time. */
                                break; /* Exit the monitoring loop. */
                            }
                            else { /* `waitpid` returned -1, indicating an error. */
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                pr_error(stdout, "waitpid error");
                                __debug_function(); /* call debugger function */
                                break;
                            }
                            /* Kill process if timeout exceeded */
                            if (i == POSIX_TIMEOUT - 1) { /* If the loop reaches its final iteration, the timeout period has elapsed. */
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                kill(compiler_process_id, SIGTERM); /* First, send SIGTERM to allow graceful termination. */
                                sleep(2); /* Wait 2 seconds for the process to exit gracefully. */
                                kill(compiler_process_id, SIGKILL); /* Forcefully kill with SIGKILL if still running. */
                                pr_error(stdout,
                                        "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                __debug_function(); /* call debugger function */
                                waitpid(compiler_process_id, &process_status, 0); /* Reap the terminated child to avoid a zombie process. */
                                process_timeout_occurred = 1; /* Set the timeout flag. */
                            }
                        }
                        if (!process_timeout_occurred) { /* If the process terminated before the timeout... */
                            /* Analyze process exit status */
                            if (WIFEXITED(process_status)) { /* Make sure the child terminated normally via `exit()` or `return`. */
                                int proc_exit_code = 0;
                                proc_exit_code = WEXITSTATUS(process_status); /* Extract the exit status code (0-255). */
                                if (proc_exit_code != 0) /* Non-zero exit codes typically indicate an error. */
                                    pr_error(stdout,
                                            "compiler process exited with code (%d)", proc_exit_code);
                                __debug_function(); /* call debugger function */
                            } else if (WIFSIGNALED(process_status)) { /* Make sure the child was terminated by a signal (e.g., SIGSEGV, SIGKILL). */
                                pr_error(stdout,
                                        "compiler process terminated by signal (%d)", WTERMSIG(process_status)); /* Extract the signal number that caused termination. */
                            }
                        }
                    } else { /* `posix_spawn` failed (returned an error number, not 0). */
                        pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result)); /* Print the human-readable error (e.g., "File not found", "Permission denied"). */
                        __debug_function(); /* call debugger function */
                    }
#endif
                    char size_container_output[WG_PATH_MAX * 2];
                    snprintf(size_container_output,
                        sizeof(size_container_output), "%s", wgconfig.wg_toml_proj_output);
                    /* Post-compilation processing based on output and flags */
                    if (path_exists(".watchdogs/compiler.log")) {
                        printf("\n");
                        char *ca = NULL;
                        ca = size_container_output;
                        int cb = 0;
                        cb = compiler_debugging;
                        /* Handle different output modes based on command-line flags */
                        if (compiler_has_watchdogs && compiler_has_clean) {
                            /* Detailed analysis with cleanup on error */
                            cause_compiler_expl(".watchdogs/compiler.log", ca, cb);
                            if (path_exists(ca)) {
                                    remove(ca); /* Remove output file on error */
                            }
                            goto compiler_done;
                        } else if (compiler_has_watchdogs) {
                            /* Detailed analysis without cleanup */
                            cause_compiler_expl(".watchdogs/compiler.log", ca, cb);
                            goto compiler_done;
                        } else if (compiler_has_clean) {
                            /* Simple output with cleanup on error */
                            wg_printfile(".watchdogs/compiler.log");
                            if (path_exists(ca)) {
                                    remove(ca);
                            }
                            goto compiler_done;
                        }

                        /* Default: just print compiler output */
                        wg_printfile(".watchdogs/compiler.log");

                        /* Check for specific compiler issues in output */
                        char log_line[WG_MAX_PATH * 4];
                        this_proc_fileile = fopen(".watchdogs/compiler.log", "r");

                        if (this_proc_fileile != NULL) {
                            while (fgets(log_line, sizeof(log_line), this_proc_fileile) != NULL) {
                                if (strfind(log_line, "backtrace", true))
                                    pr_color(stdout, FCOLOUR_CYAN,
                                        "~ backtrace detected - "
                                        "make sure you are using a newer version of pawncc than the one currently in use.");
                            }
                            fclose(this_proc_fileile);
                        }
                    }
compiler_done:
                    /* Make sure compilation had errors by scanning log file */
                    this_proc_file = fopen(".watchdogs/compiler.log", "r");
                    if (this_proc_file)
                    {
                        char compiler_line_buffer[WG_PATH_MAX];
                        int compiler_has_err = 0;
                        while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), this_proc_file))
                        {
                            if (strstr(compiler_line_buffer, "error")) {
                                compiler_has_err = 1;
                                break;
                            }
                        }
                        fclose(this_proc_file);
                        if (compiler_has_err)
                        {
                            /* Cleanup output file if compilation failed */
                            if (size_container_output[0] != '\0' && path_access(size_container_output))
                                remove(size_container_output);
                            wgconfig.wg_compiler_stat = 1; /* Set error status */
                        }
                    }
                    else {
                        pr_error(stdout, "Failed to open .watchdogs/compiler.log");
                        __debug_function(); /* call debugger function */
                    }

                    /* Calculate and display compilation duration */
                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
                    /* Provide helpful message for long compilations */
                    if (compiler_dur > 0x64) {
                        printf("~ This is taking a while, huh?\n"
                                "  Make sure you've cleared all the warnings,\n"
                                "  you're using the latest compiler,\n"
                                "  and double-check that your logic\n"
                                "  and pawn algorithm tweaks in the gamemode scripts line up.\n");
                        fflush(stdout);
                    }
                }
                else
                {
                    /* Specific file compilation: process provided file path */
                    strncpy(ptr_io->compiler_size_temp, compile_args, sizeof(ptr_io->compiler_size_temp) - 1);
                    ptr_io->compiler_size_temp[sizeof(ptr_io->compiler_size_temp) - 1] = '\0';

                    /* Extract directory and filename from provided path */
                    compiler_last_slash = strrchr(ptr_io->compiler_size_temp, __PATH_CHR_SEP_LINUX);
                    compiler_back_slash = strrchr(ptr_io->compiler_size_temp, __PATH_CHR_SEP_WIN32);

                    if (compiler_back_slash && (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                        compiler_last_slash = compiler_back_slash;

                    if (compiler_last_slash)
                    {
                        /* Path contains directory separator - split into components */
                        size_t compiler_dir_len;
                        compiler_dir_len = (size_t)(compiler_last_slash - ptr_io->compiler_size_temp);

                        if (compiler_dir_len >= sizeof(ptr_io->compiler_direct_path))
                            compiler_dir_len = sizeof(ptr_io->compiler_direct_path) - 1;

                        memcpy(ptr_io->compiler_direct_path, ptr_io->compiler_size_temp, compiler_dir_len);
                        ptr_io->compiler_direct_path[compiler_dir_len] = '\0';

                        const char *compiler_filename_start = compiler_last_slash + 1;
                        size_t compiler_filename_len;
                        compiler_filename_len = strlen(compiler_filename_start);

                        if (compiler_filename_len >= sizeof(ptr_io->compiler_size_file_name))
                            compiler_filename_len = sizeof(ptr_io->compiler_size_file_name) - 1;

                        memcpy(ptr_io->compiler_size_file_name, compiler_filename_start, compiler_filename_len);
                        ptr_io->compiler_size_file_name[compiler_filename_len] = '\0';

                        /* Reconstruct full path ensuring buffer safety */
                        size_t total_needed;
                        total_needed = strlen(ptr_io->compiler_direct_path) + 1 +
                                    strlen(ptr_io->compiler_size_file_name) + 1;

                        if (total_needed > sizeof(ptr_io->compiler_size_input_path))
                        {
                            /* Fallback to default gamemodes directory if path too long */
                            strncpy(ptr_io->compiler_direct_path, "gamemodes",
                                sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            size_t compiler_max_size_file_name;
                            compiler_max_size_file_name = sizeof(ptr_io->compiler_size_file_name) - 1;

                            if (compiler_filename_len > compiler_max_size_file_name)
                            {
                                memcpy(ptr_io->compiler_size_file_name, compiler_filename_start,
                                    compiler_max_size_file_name);
                                ptr_io->compiler_size_file_name[compiler_max_size_file_name] = '\0';
                            }
                        }

                        if (snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                "%s/%s", ptr_io->compiler_direct_path, ptr_io->compiler_size_file_name) >=
                            (int)sizeof(ptr_io->compiler_size_input_path))
                        {
                            ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                        }
                     }  else {
                            /* No directory in path - treat as filename in current directory */
                            strncpy(ptr_io->compiler_size_file_name, ptr_io->compiler_size_temp,
                                sizeof(ptr_io->compiler_size_file_name) - 1);
                            ptr_io->compiler_size_file_name[sizeof(ptr_io->compiler_size_file_name) - 1] = '\0';

                            strncpy(ptr_io->compiler_direct_path, ".", sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "./%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }
                        }

                    /* Search for the specified file in determined directory */
                    int compiler_finding_compile_args = 0;
                    compiler_finding_compile_args = wg_sef_fdir(ptr_io->compiler_direct_path, ptr_io->compiler_size_file_name, NULL);

                    /* Fallback search in gamemodes directory if not found initially */
                    if (!compiler_finding_compile_args &&
                        strcmp(ptr_io->compiler_direct_path, "gamemodes") != 0)
                    {
                        compiler_finding_compile_args = wg_sef_fdir("gamemodes",
                                            ptr_io->compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            strncpy(ptr_io->compiler_direct_path, "gamemodes",
                                sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "gamemodes/%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }

                            if (wgconfig.wg_sef_count > RATE_SEF_EMPTY)
                                strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count - 1],
                                    ptr_io->compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    /* Additional fallback for current directory search */
                    if (!compiler_finding_compile_args && !strcmp(ptr_io->compiler_direct_path, "."))
                    {
                        compiler_finding_compile_args = wg_sef_fdir("gamemodes",
                                            ptr_io->compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            strncpy(ptr_io->compiler_direct_path, "gamemodes",
                                sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "gamemodes/%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }

                            if (wgconfig.wg_sef_count > RATE_SEF_EMPTY)
                                strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count - 1],
                                    ptr_io->compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    snprintf(wg_compiler_input_proj_path,
                            sizeof(wg_compiler_input_proj_path), "%s", wgconfig.wg_sef_found_list[1]);

                    /* Execute compilation if file was found */
                    if (compiler_finding_compile_args)
                    {
                        char __sef_path_sz[WG_PATH_MAX];
                        snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wg_compiler_input_proj_path);
                        char *extension = strrchr(__sef_path_sz, '.');
                        if (extension)
                            *extension = '\0'; /* Remove extension for output filename */

                        snprintf(ptr_io->container_output, sizeof(ptr_io->container_output), "%s", __sef_path_sz);

                        char size_container_output[WG_MAX_PATH];
                        snprintf(size_container_output, sizeof(size_container_output), "%s.amx", ptr_io->container_output);

#ifdef WG_WINDOWS
                        /* Windows-specific compilation execution */
                        sa.bInheritHandle = TRUE; /* Set the `bInheritHandle` member of the SECURITY_ATTRIBUTES structure to TRUE. This allows the file handle created below to be inherited by child processes. When TRUE, the handle is inherited; when FALSE, it's not. */
                        HANDLE hFile = CreateFileA( /* Create or open a file/device. Returns a handle to the object or INVALID_HANDLE_VALUE on error. 'A' suffix indicates ANSI character version (vs 'W' for Unicode). */
                                ".watchdogs/compiler.log", /* The name of the file to create or open. Relative paths are relative to the current directory. */
                                GENERIC_WRITE, /* Requested access to the file: GENERIC_WRITE allows writing to the file. This corresponds to write-only access. */
                                FILE_SHARE_READ, /* Sharing mode: FILE_SHARE_READ allows other processes to open the file for reading while this handle is open. This prevents exclusive locking. */
                                &sa, /* Pointer to SECURITY_ATTRIBUTES structure. Determines whether the handle can be inherited by child processes and optionally specifies a security descriptor. */
                                CREATE_ALWAYS, /* Creation disposition: CREATE_ALWAYS creates a new file, always. If the file exists, it is overwritten and truncated to zero bytes. Similar to O_CREAT | O_TRUNC in POSIX. */
                                FILE_ATTRIBUTE_NORMAL | /* File attributes: FILE_ATTRIBUTE_NORMAL indicates a standard file with no special attributes. Combined with: */
                                    FILE_FLAG_SEQUENTIAL_SCAN, /* FILE_FLAG_SEQUENTIAL_SCAN hints to the system that the file will be accessed sequentially. This enables read-ahead optimization for better performance. */
                                NULL /* Template file handle: not used for ordinary file creation, should be NULL. */
                        );

                        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; /* Set startup information flags: STARTF_USESTDHANDLES indicates that the hStdInput, hStdOutput, and hStdError members contain valid handles. STARTF_USESHOWWINDOW indicates that the wShowWindow member contains valid information. */
                        si.wShowWindow = SW_HIDE; /* Set the window show state: SW_HIDE hides the window (minimizes and removes from taskbar). This prevents a console window from appearing when launching the compiler. */

                        if (hFile != INVALID_HANDLE_VALUE) { /* Make sure the file was created successfully. INVALID_HANDLE_VALUE (usually -1) indicates failure. */
                            si.hStdOutput = hFile; /* Redirect the child process's standard output to the log file. Any output the child writes to stdout will go to the log file. */
                            si.hStdError = hFile; /* Redirect the child process's standard error to the same log file. This captures both regular output and error messages. */
                        }

                        int ret_command = 0; /* Variable to store the return value of snprintf, which indicates the number of characters that would have been written (excluding null terminator) or negative on error. */
                        ret_command = snprintf(_compiler_input_, /* Construct the compiler command line string. Similar to building argv in POSIX but as a single command string for Windows. */
                                sizeof(_compiler_input_),
                                        "%s " "%s " "-o%s " "%s " "%s " "-i%s", /* Format string: compiler_path input_file -ooutput_file options include_paths -iinclude_path */
                                        wg_compiler_input_pawncc_path, /* Path to the pawncc compiler executable (e.g., "pawncc.exe"). */
                                        wg_compiler_input_proj_path, /* Input source file to compile (e.g., "gamemodes/test.pwn"). */
                                        size_container_output, /* Output file name for the compiled binary (e.g., "test.amx"). */
                                        wgconfig.wg_toml_aio_opt, /* Compiler options from TOML configuration (e.g., "-d2 -O2"). */
                                        include_aio_path, /* Include paths from TOML configuration (e.g., "-i\"include1\" -i\"include2\""). */
                                        path_include); /* Default include paths (e.g., "pawno/include"). */
                        if (rate_debugger > 0) {
                            printf("{\n");
                            printf("  \"mode\": \"%s\",\n", "with args");
                            printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                            printf("  \"project\": \"%s\",\n", wg_compiler_input_proj_path);
                            printf("  \"output\": \"%s\",\n", size_container_output);
                            printf("  \"include\": \"%s\"\n", path_include);
                            printf("  \"command\": \"%s\"\n", _compiler_input_);
                            printf("}\n");
                            fflush(stdout);
                        }
                        if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) { /* Make sure snprintf succeeded and the command fits in the buffer. ret_command > 0 means characters were written; < buffer_size means no truncation occurred. */
                            BOOL win32_process_succes; /* BOOL is Windows' boolean type (typedef int BOOL). TRUE = non-zero, FALSE = 0. */
                            clock_gettime(CLOCK_MONOTONIC, &start); /* Record start time for performance measurement. CLOCK_MONOTONIC is available in Windows 10+ via gettimeofday() compatibility. */
                            win32_process_succes = CreateProcessA( /* Create and execute the compiler process. Returns TRUE on success, FALSE on failure. */
                                    NULL, /* Application name (lpApplicationName). If NULL, the executable name must be the first whitespace-delimited token in lpCommandLine. This allows searching in PATH. */
                                    _compiler_input_, /* Command line string (lpCommandLine). Contains the full command with arguments. This string can be modified by CreateProcess, so it should be writable (not a string literal). */
                                    NULL, /* Process security attributes (lpProcessAttributes). NULL means the process gets a default security descriptor and the handle cannot be inherited. */
                                    NULL, /* Thread security attributes (lpThreadAttributes). NULL means the thread gets a default security descriptor and the handle cannot be inherited. */
                                    TRUE, /* Handle inheritance flag (bInheritHandles). TRUE allows the child to inherit inheritable handles (like our log file handle). FALSE prevents inheritance. */
                                    CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS, /* Creation flags: CREATE_NO_WINDOW creates a console application without a console window (silent). ABOVE_NORMAL_PRIORITY_CLASS gives the process slightly higher priority than normal (for faster compilation). */
                                    NULL, /* Environment block (lpEnvironment). NULL means the child inherits the parent's environment. Can specify a custom environment block if needed. */
                                    NULL, /* Current directory (lpCurrentDirectory). NULL means the child inherits the parent's current directory. Can specify a different working directory. */
                                    &si, /* Startup information (lpStartupInfo). Contains window, std handle, and other startup configurations. Must be properly initialized. */
                                    &pi); /* Process information (lpProcessInformation). Receives handles and IDs for the new process and its primary thread. Contains hProcess, hThread, dwProcessId, dwThreadId. */
                            if (win32_process_succes) { /* If CreateProcess succeeded (returned TRUE)... */
                                WaitForSingleObject(pi.hProcess, INFINITE); /* Wait indefinitely for the child process to terminate. Blocks the parent until the child exits. INFINITE means wait forever (no timeout). */

                                clock_gettime(CLOCK_MONOTONIC, &end); /* Record end time after the child process has completed. */

                                DWORD proc_exit_code; /* DWORD (32-bit unsigned integer) to store the child's exit code. Windows exit codes are also in the range 0-255, though DWORD can hold 0-4294967295. */
                                GetExitCodeProcess(pi.hProcess, &proc_exit_code); /* Retrieve the exit code of the terminated process. If the process hasn't terminated, returns STILL_ACTIVE (259). */

                                CloseHandle(pi.hProcess); /* Close the process handle to release system resources. Always close handles returned by CreateProcess to avoid leaks. */
                                CloseHandle(pi.hThread); /* Close the thread handle as well. Even though the thread has terminated, the handle still consumes resources. */
                            } else { /* CreateProcess failed (returned FALSE) */
                                clock_gettime(CLOCK_MONOTONIC, &end); /* Still record end time for consistent timing, even though process never started. */
                                pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError()); /* Print error with Windows error code. GetLastError() returns the last error code (DWORD). Can be converted to message with FormatMessage(). */
                                __debug_function(); /* call debugger function */
                            }
                        } else { /* Command construction failed (snprintf error or buffer overflow) */
                            clock_gettime(CLOCK_MONOTONIC, &end); /* Record end time for timing consistency. */
                            pr_error(stdout, "ret_compiler too long!"); /* Print error indicating the command string was truncated or construction failed. __func__ and __LINE__ help debugging. */
                            __debug_function(); /* call debugger function */
                            goto compiler_end; /* Jump to cleanup/exit section. */
                        }
                        if (hFile != INVALID_HANDLE_VALUE) { /* If the log file was successfully opened... */
                            CloseHandle(hFile); /* Close the log file handle. After the child process completes, we no longer need the handle. The file remains on disk. */
                        }
#else
                        /* POSIX-specific compilation execution */
                        int ret_command = 0;
                        ret_command = snprintf(_compiler_input_, sizeof(_compiler_input_),
                            "%s " "%s " "%s%s " "%s%s " "%s%s",
                            wg_compiler_input_pawncc_path,
                            wg_compiler_input_proj_path,
                            "-o",
                            size_container_output,
                            wgconfig.wg_toml_aio_opt,
                            include_aio_path,
                            "-i",
                            path_include);
                        if (ret_command > (int)sizeof(_compiler_input_)) {
                            pr_error(stdout, "ret_compiler too long!");
                            __debug_function(); /* call debugger function */
                            goto compiler_end;
                        }
                        if (rate_debugger > 0) {
                            printf("{\n");
                            printf("  \"mode\": \"%s\",\n", "with args");
                            printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                            printf("  \"project\": \"%s\",\n", wg_compiler_input_proj_path);
                            printf("  \"output\": \"%s\",\n", size_container_output);
                            printf("  \"include\": \"%s\"\n", path_include);
                            printf("  \"command\": \"%s\"\n", _compiler_input_);
                            printf("}\n");
                            fflush(stdout);
                        }
                        int i = 0;
                        char *wg_compiler_unix_args[WG_MAX_PATH + 256];
                        char *compiler_unix_token = strtok(_compiler_input_, " ");
                        while (compiler_unix_token != NULL) {
                            wg_compiler_unix_args[i++] = compiler_unix_token;
                            compiler_unix_token = strtok(NULL, " ");
                        }
                        wg_compiler_unix_args[i] = NULL;

                        /* Setup file actions to redirect output to log file */
                        posix_spawn_file_actions_t process_file_actions; /* Declare a file actions object to manipulate the child's file descriptors before execution. This structure holds a sequence of file operations to be performed on the child's file descriptors. */
                        posix_spawn_file_actions_init(&process_file_actions); /* Initialize the file actions object to an empty state. Must be called before any `add` operations. Failure to initialize results in undefined behavior. */
                        int posix_logging_file = open(".watchdogs/compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644); /* Open the log file for writing (O_WRONLY), create it if it doesn't exist (O_CREAT), and truncate it to zero length if it already exists (O_TRUNC). The permissions 0644 mean: owner can read/write (6), group and others can only read (4). */
                        if (posix_logging_file != -1) { /* Make sure the log file was opened successfully. The `open()` function returns -1 on error. */
                            posix_spawn_file_actions_adddup2(&process_file_actions,
                                    posix_logging_file,
                                    STDOUT_FILENO); /* Add an action to duplicate the log file descriptor onto the child's stdout (file descriptor 1). After this, anything the child writes to stdout will go to the log file instead of the terminal. */
                            posix_spawn_file_actions_adddup2(&process_file_actions,
                                    posix_logging_file,
                                    STDERR_FILENO); /* Add an action to duplicate the same log file descriptor onto the child's stderr (file descriptor 2). This ensures both standard output and error streams are captured in the same log file for comprehensive logging. */
                            posix_spawn_file_actions_addclose(&process_file_actions,
                                    posix_logging_file); /* Add an action to close the original log file descriptor in the child process. This prevents file descriptor leaks since the child inherits all open file descriptors from the parent. The file remains open because stdout/stderr now reference it. */
                        }

                        /* Configure spawn attributes with signal handling */
                        posix_spawnattr_t spawn_attr; /* Declare a spawn attributes object to control various aspects of the child process behavior: signal masks, scheduling policies, process group, user/group IDs, etc. */
                        posix_spawnattr_init(&spawn_attr); /* Initialize the spawn attributes object with default values. This must be done before setting any specific attributes. */

                        posix_spawnattr_setflags(&spawn_attr,
                                POSIX_SPAWN_SETSIGDEF | /* This flag tells `posix_spawn` to reset the child's signal handlers to their default actions (SIG_DFL). For example: SIGINT becomes terminate, SIGCHLD becomes ignore, etc. This provides a clean signal state. */
                                POSIX_SPAWN_SETSIGMASK  /* This flag tells `posix_spawn` to apply a specified signal mask to the child process. The mask determines which signals are blocked (prevented from being delivered) initially. Combined with SIGSETDEF, this gives complete control over the child's signal environment. */
                        );

                        pid_t compiler_process_id; /* Variable to store the Process ID (PID) of the newly spawned child process. This PID can be used later to monitor, signal, or wait for the child. */

                        /* Execute compiler using posix_spawnp for better control */
                        int process_spawn_result = posix_spawnp(&compiler_process_id, /* `posix_spawnp` searches for the executable in the directories listed in the `PATH` environment variable. If the first argument contains a __PATH_CHR_SEP_LINUX, it's treated as a pathname and PATH is not searched. Returns 0 on success, error number on failure. */
                                    wg_compiler_unix_args[0], /* The program to execute (e.g., "pawncc", "gcc", "./script.sh"). This is argv[0] - typically the program name, but can be any string. */
                                    &process_file_actions, /* The file actions object; can be NULL if no file manipulations are needed. If provided, all specified file operations are performed atomically before the new program starts. */
                                    &spawn_attr, /* The spawn attributes object; can be NULL to use default process attributes. This controls signal handling, process group, scheduling, etc. */
                                    wg_compiler_unix_args, /* The argument array (argv) for the new program. This is a NULL-terminated array of strings, where argv[0] is typically the program name, and subsequent elements are command-line arguments. */
                                    environ); /* The environment variable array (envp) for the new program. `environ` is a global variable containing the parent's environment. Passing it gives the child the same environment as the parent. */

                        posix_spawnattr_destroy(&spawn_attr); /* Destroy the spawn attributes object to free its internal resources. Always destroy objects after use to prevent memory leaks. */
                        posix_spawn_file_actions_destroy(&process_file_actions); /* Destroy the file actions object to free its internal resources. Even if `posix_spawn` fails, you must destroy initialized objects. */

                        if (process_spawn_result == 0) { /* `posix_spawn` returns 0 on success, and the child's PID is stored in `compiler_process_id`. At this point, the child process is running concurrently with the parent. */
                            int process_status; /* Variable to store the child's termination status, which will be filled by `waitpid`. This integer encodes both exit code and termination reason. */
                            int process_timeout_occurred = 0; /* Flag to track if the process was killed due to timeout. 0 = normal termination, 1 = killed by timeout. */
                            clock_gettime(CLOCK_MONOTONIC, &start); /* Get the start time using a monotonic clock (unaffected by system time changes). CLOCK_MONOTONIC counts from an arbitrary point and never jumps backward, ideal for measuring intervals. */
                            /* Monitor process with timeout to prevent hanging */
                            for (int i = 0; i < POSIX_TIMEOUT; i++) { /* Loop for a predefined number of intervals (e.g., 120 iterations). Each iteration represents one check of the child's status. */
                                int p_result = -1;
                                p_result = waitpid(compiler_process_id, &process_status, WNOHANG); /* Check child's status non-blockingly. WNOHANG means return immediately if child hasn't changed state. Returns: >0 (PID) if child terminated, 0 if still running, -1 on error (e.g., no such child). */
                                if (p_result == 0) /* Child is still running - no state change */
                                    usleep(0xC350); /* Sleep 50ms (0xC350 microseconds = 50,000 µs = 0.05 seconds) if the process is still running. This prevents busy-waiting (consuming 100% CPU while polling). The hex value 0xC350 is 50,000 in decimal. */
                                else if (p_result == compiler_process_id) { /* Child process terminated (normally or by signal). waitpid returned the child's PID, indicating a state change. */
                                    clock_gettime(CLOCK_MONOTONIC, &end); /* Record the end time to calculate total execution duration. */
                                    break; /* Exit the monitoring loop since child has terminated. */
                                }
                                else { /* `waitpid` returned -1, indicating an error (e.g., ECHILD = no such child, EINTR = interrupted by signal). */
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    pr_error(stdout, "waitpid error"); /* Print error message. In production, you might want to use strerror(errno) for details. */
                                    __debug_function(); /* call debugger function */
                                    break;
                                }
                                /* Kill process if timeout exceeded */
                                if (i == POSIX_TIMEOUT - 1) { /* If the loop reaches its final iteration, the timeout period has elapsed. For example, if POSIX_TIMEOUT=120 and sleep is 50ms, total timeout = 120 * 0.05 = 6 seconds. */
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    kill(compiler_process_id, SIGTERM); /* First, send SIGTERM (signal 15) to allow graceful termination. This gives the process a chance to clean up resources, flush buffers, and exit properly. */
                                    sleep(2); /* Wait 2 seconds for the process to exit gracefully after SIGTERM. This is a grace period for cleanup. */
                                    kill(compiler_process_id, SIGKILL); /* Forcefully kill with SIGKILL (signal 9) if still running. SIGKILL cannot be caught or ignored - the OS immediately terminates the process. */
                                    pr_error(stdout,
                                            "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                    waitpid(compiler_process_id, &process_status, 0); /* Reap the terminated child to avoid a zombie process. The 0 flag means block until child terminates. */
                                    process_timeout_occurred = 1; /* Set the timeout flag to indicate abnormal termination. */
                                }
                            }
                            if (!process_timeout_occurred) { /* If the process terminated before the timeout (normal case)... */
                                /* Analyze process exit status using POSIX macros */
                                if (WIFEXITED(process_status)) { /* Make sure the child terminated normally via `exit()` or returning from main(). */
                                    int proc_exit_code = 0;
                                    proc_exit_code = WEXITSTATUS(process_status); /* Extract the exit status code (0-255). Convention: 0 = success, non-zero = error. */
                                    if (proc_exit_code != 0) /* Non-zero exit codes typically indicate compilation errors or other failures. */
                                        pr_error(stdout,
                                                "compiler process exited with code (%d)", proc_exit_code);
                                        __debug_function(); /* call debugger function */
                                } else if (WIFSIGNALED(process_status)) { /* Make sure the child was terminated by a signal it didn't catch (e.g., SIGSEGV, SIGABRT, SIGKILL). */
                                    pr_error(stdout,
                                            "compiler process terminated by signal (%d)", WTERMSIG(process_status)); /* Extract the signal number that caused termination. Common signals: 6=SIGABRT, 11=SIGSEGV, 9=SIGKILL, 15=SIGTERM. */
                                    __debug_function(); /* call debugger function */
                                }
                                /* Note: WIFSTOPPED and WIFCONTINUED are not checked here - they're for processes stopped by signals like SIGSTOP. */
                            }
                        } else { /* `posix_spawn` failed (returned an error number, not 0). Common errors: ENOENT (file not found), EACCES (permission denied), E2BIG (argument list too long). */
                            pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result)); /* Print the human-readable error using strerror(). The error number is stored in process_spawn_result (not errno, as posix_spawn returns it directly). */
                            __debug_function(); /* call debugger function */
                        }
#endif
                        /* Post-compilation output handling for specific file compilation */
                        if (path_exists(".watchdogs/compiler.log")) {
                            printf("\n");
                            char *ca = NULL;
                            ca = size_container_output;
                            int cb = 0;
                            cb = compiler_debugging;
                            if (compiler_has_watchdogs && compiler_has_clean) {
                                cause_compiler_expl(".watchdogs/compiler.log", ca, cb);
                                if (path_exists(ca)) {
                                        remove(ca);
                                }
                                goto compiler_done2;
                            } else if (compiler_has_watchdogs) {
                                cause_compiler_expl(".watchdogs/compiler.log", ca, cb);
                                goto compiler_done2;
                            } else if (compiler_has_clean) {
                                wg_printfile(".watchdogs/compiler.log");
                                if (path_exists(ca)) {
                                        remove(ca);
                                }
                                goto compiler_done2;
                            }

                            wg_printfile(".watchdogs/compiler.log");

                            char log_line[WG_MAX_PATH * 4];
                            this_proc_fileile = fopen(".watchdogs/compiler.log", "r");

                            if (this_proc_fileile != NULL) {
                                while (fgets(log_line, sizeof(log_line), this_proc_fileile) != NULL) {
                                    if (strfind(log_line, "backtrace", true))
                                        pr_color(stdout, FCOLOUR_CYAN,
                                            "~ backtrace detected - "
                                            "make sure you are using a newer version of pawncc than the one currently in use.\n");
                                }
                                fclose(this_proc_fileile);
                            }
                        }

compiler_done2:
                        /* Error detection and cleanup for specific file compilation */
                        this_proc_file = fopen(".watchdogs/compiler.log", "r");
                        if (this_proc_file)
                        {
                            char compiler_line_buffer[WG_PATH_MAX];
                            int compiler_has_err = 0;
                            while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), this_proc_file))
                            {
                                if (strstr(compiler_line_buffer, "error")) {
                                    compiler_has_err = 1;
                                    break;
                                }
                            }
                            fclose(this_proc_file);
                            if (compiler_has_err)
                            {
                                if (size_container_output[0] != '\0' && path_access(size_container_output))
                                    remove(size_container_output);
                                wgconfig.wg_compiler_stat = 1;
                            }
                        }
                        else {
                            pr_error(stdout, "Failed to open .watchdogs/compiler.log");
                            __debug_function(); /* call debugger function */
                        }

                        /* Display compilation metrics */
                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        printf("\n");
                        pr_color(stdout, FCOLOUR_CYAN,
                            " <P> Finished at %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
                        if (compiler_dur > 0x64) {
                            printf("~ This is taking a while, huh?\n"
                                    "  Make sure you've cleared all the warnings,\n"
                                    "  you're using the latest compiler,\n"
                                    "  and double-check that your logic\n"
                                    "  and pawn algorithm tweaks in the gamemode scripts line up.\n");
                            fflush(stdout);
                        }
                    }
                    else
                    {
                        /* File not found - display error */
                        printf("Cannot locate input: ");
                        pr_color(stdout, FCOLOUR_CYAN, "%s\n", compile_args);
                        goto compiler_end;
                    }
                }

                toml_free(wg_toml_config); /* Free TOML parsing resources */

                goto compiler_end;
            }
        } else {
            /* Compiler not found - offer installation */
            pr_error(stdout,
                "\033[1;31merror:\033[0m pawncc (our compiler) not found\n"
                "  \033[2mhelp:\033[0m install it before continuing");
            printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

            char *pointer_signalA = readline("");

            while (true)
            {
                if (strcmp(pointer_signalA, "Y") == 0 || strcmp(pointer_signalA, "y") == 0)
                {
                    wg_free(pointer_signalA);
ret_ptr:
                    println(stdout, "Select platform:");
                    println(stdout, "-> [L/l] Linux");
                    println(stdout, "-> [W/w] Windows");
                    pr_color(stdout, FCOLOUR_YELLOW, " ^ work's in WSL/MSYS2\n");
                    println(stdout, "-> [T/t] Termux");

                    wgconfig.wg_sel_stat = 1;

                    char *platform = readline("==> ");

                    /* Platform-specific compiler installation */
                    if (strfind(platform, "L", true))
                    {
                        int ret = wg_install_pawncc("linux");
                        wg_free(platform);
loop_ipcc:
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc;
                    } else if (strfind(platform, "W", true)) {
                        int ret = wg_install_pawncc("windows");
                        wg_free(platform);
loop_ipcc2:
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc2;
                    } else if (strfind(platform, "T", true)) {
                        int ret = wg_install_pawncc("termux");
                        wg_free(platform);
loop_ipcc3:
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc3;
                    } else if (strfind(platform, "E", true)) {
                        wg_free(platform);
                        goto loop_end;
                    } else {
                        pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                        wg_free(platform);
                        goto ret_ptr;
                    }
loop_end:
                    chain_ret_main(NULL); /* Return to main menu after installation */
                }
                else if (strcmp(pointer_signalA, "N") == 0 || strcmp(pointer_signalA, "n") == 0)
                {
                    wg_free(pointer_signalA);
                    break; /* Exit without installation */
                }
                else
                {
                    pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                    wg_free(pointer_signalA);
                    goto ret_ptr;
                }
            }
        }

compiler_end:
        return 1; /* Return success status */
}

causeExplanation ccs[] =
{
/* === SYNTAX ERRORS (Invalid Code Structure) ===  */

/* A key language element (like ',' or ';') is missing. */
{"expected token", "A required token (e.g., ';', ',', ')') is missing from the code. Check the line indicated for typos or omissions."},

/* The 'case' label can only be followed by a single statement. */
{"only a single statement", "A `case` label can only be followed by a single statement. To use multiple statements, enclose them in a block with `{` and `}`."},

/* Variables declared inside case/default must be inside a block. */
{"declaration of a local variable must appear in a compound block", "Local variables declared inside a `case` or `default` label must be scoped within a compound block `{ ... }`."},

/* Function call or declaration is incorrect. */
{"invalid function call", "The symbol being called is not a function, or the syntax of the function call is incorrect."},

/* Statements like 'case' or 'default' are used outside a switch block. */
{"invalid statement; not in switch", "The `case` or `default` label is only valid inside a `switch` statement."},

/* The 'default' case is not the last one in the switch block. */
{"default case must be the last case", "The `default` case within a `switch` statement must appear after all other `case` labels."},

/* More than one 'default' case found in a single switch. */
{"multiple defaults in switch", "A `switch` statement can only have one `default` case."},

/* The target of a 'goto' statement is not a valid label. */
{"not a label", "The symbol used in the `goto` statement is not defined as a label."},

/* The symbol name does not follow the language's naming rules. */
{"invalid symbol name", "Symbol names must start with a letter, an underscore '_', or an 'at' sign '@'."},

/* An empty statement (just a semicolon) is not allowed in certain contexts. */
{"empty statement", "Pawn does not support a standalone semicolon as an empty statement. double `;;`? - Use an empty block `{}` instead if intentional."},

/* A required semicolon is missing. */
{"missing semicolon", "A semicolon ';' is expected at the end of this statement."},

/* The file ended before a structure was properly closed. */
{"unexpected end of file", "The source file ended unexpectedly. This is often caused by a missing closing brace '}', parenthesis ')', or quote."},

/* An invalid character was encountered in the source code. */
{"illegal character", "A character was found that is not valid in the current context (e.g., outside of a string or comment)."},

/* Brackets or braces are not properly matched. */
{"missing closing parenthesis", "An opening parenthesis '(' was not closed with a matching ')'."},
{"missing closing bracket", "An opening bracket '[' was not closed with a matching ']'."},
{"missing closing brace", "An opening brace '{' was not closed with a matching '}'."},



/* === SEMANTIC ERRORS (Invalid Code Meaning) ===  */

/* A function was declared but its body was never defined. */
{"is not implemented", "A function was declared but its implementation (body) was not found. This can be caused by a missing closing brace `}` in a previous function."},

/* The main function is defined with parameters, which is not allowed. */
{"function may not have arguments", "The `main()` function cannot accept any arguments."},

/* A string literal is being used incorrectly. */
{"must be assigned to an array", "String literals must be assigned to a character array variable; they cannot be assigned to non-array types."},

/* An attempt was made to overload an operator that cannot be overloaded. */
{"operator cannot be redefined", "Only specific operators can be redefined (overloaded) in Pawn. Check the language specification."},

/* A context requires a constant value, but a variable or non-constant expression was provided. */
{"must be a constant expression; assumed zero", "Array sizes and compiler directives (like `#if`) require constant expressions (literals or declared constants)."},

/* An array was declared with a size of zero or a negative number. */
{"invalid array size", "The size of an array must be 1 or greater."},

/* A symbol (variable, function, constant) is used but was never declared. */
{"undefined symbol", "A symbol is used in the code but it has not been declared. Check for typos or missing declarations."},

/* The initializer for an array has more elements than the array's declared size. */
{"initialization data exceeds declared size", "The data used to initialize the array contains more elements than the size specified for the array."},

/* The left-hand side of an assignment is not something that can be assigned to. */
{"must be lvalue", "The symbol on the left side of an assignment operator must be a modifiable variable, array element, or other 'lvalue'."},

/* Arrays cannot be used in compound assignments. */
{"array assignment must be simple assignment", "Arrays cannot be used with compound assignment operators (like `+=`). Only simple assignment `=` is allowed for arrays."},

/* 'break' or 'continue' is used outside of any loop or switch statement. */
{"break or continue is out of context", "The `break` statement is only valid inside loops (`for`, `while`, `do-while`) and `switch`. `continue` is only valid inside loops."},

/* A function's definition does not match its previous declaration. */
{"function heading differs from prototype", "The function's definition (return type, name, parameters) does not match a previous declaration of the same function."},

/* An invalid character is used inside single quotes. */
{"invalid character constant", "An unknown escape sequence (like `\\z`) was used, or multiple characters were placed inside single quotes (e.g., 'ab')."},

/* The compiler could not parse the expression. */
{"invalid expression, assumed zero", "The compiler could not understand the structure of the expression. This is often due to a syntax error or operator misuse."},

/* An array index is outside the valid range for the array. */
{"array index out of bounds", "The index used to access the array is either negative or greater than or equal to the array's size."},

/* A function call has more than the allowed number of arguments. */
{"too many function arguments", "A function call was made with more than 64 arguments, which is the limit in Pawn."},

/* A symbol is defined but never used in the code. */
{"symbol is never used", "A variable, constant, or function was defined but never referenced in the code. This may indicate dead code or a typo."},

/* A value is assigned to a variable, but that value is never read later. */
{"symbol is assigned a value that is never used", "A variable is assigned a value, but that value is never used in any subsequent operation. This might indicate unnecessary computation."},

/* A conditional expression is always false. */
{"redundant code: constant expression is zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to zero (false), making the code block unreachable."},

/* A non-void function does not return a value on all control paths. */
{"should return a value", "A function that is declared to return a value must return a value on all possible execution paths."},

/* An assignment is used in a boolean context, which might be a mistake for '=='. */
{"possibly unintended assignment", "An assignment operator `=` was used in a context where a comparison operator `==` is typically used (e.g., in an `if` condition)."},

/* nvalid function or declaration */
{"invalid function or ", "You need to make sure that the area around the error line follows the proper syntax, structure, and rules typical of Pawn code."},

/* There is a type mismatch between two expressions. */
{"tag mismatch", "The type (or 'tag') of the expression does not match the type expected by the context. Check variable types and function signatures."},

/* An expression is evaluated but its result is not used or stored. */
{"expression has no effect", "An expression is evaluated but does not change the program's state (e.g., `a + b;` on a line by itself). This is often a logical error."},

/* The indentation in the source code is inconsistent. */
{"loose indentation", "The indentation (spaces/tabs) is inconsistent. While this doesn't affect compilation, it harms code readability."},

/* A function is marked as deprecated and should not be used. */
{"Function is deprecated", "This function is outdated and may be removed in future versions. The compiler suggests using an alternative."},

/* No valid entry point (main function) was found for the program. */
{"no entry point", "The program must contain a `main` function or another designated public function to serve as the entry point."},

/* A symbol is defined more than once in the same scope. */
{"symbol already defined", "A symbol (variable, function, etc.) is being redefined in the same scope. You cannot have two symbols with the same name in the same scope."},

/* Array indexing is used incorrectly. */
{"invalid subscript", "The bracket operators `[` and `]` are being used incorrectly, likely with a variable that is not an array."},

/* An array name is used without an index. */
{"array must be indexed", "An array variable is used in an expression without an index. You must specify which element of the array you want to access."},

/* A function argument placeholder lacks a default value. */
{"argument does not have a default value", "In a function call with named arguments, a placeholder was used for an argument that does not have a default value specified."},

/* The type of an argument in a function call does not match the function's parameter type. */
{"argument type mismatch", "The type of an argument passed to a function does not match the expected parameter type defined by the function."},

/* A string literal is malformed. */
{"invalid string", "A string literal is not properly formed, often due to a missing closing quote or an invalid escape sequence."},

/* A symbolic constant is used with the sizeof operator. */
{"constant symbol has no size", "The `sizeof` operator cannot be applied to a symbolic constant. It is only for variables and types."},

/* Two 'case' labels in the same switch have the same value. */
{"duplicate case label", "Two `case` labels within the same `switch` statement have the same constant value. Each `case` must be unique."},

/* The ellipsis (...) is used in an invalid context for array sizing. */
{"invalid ellipsis", "The compiler cannot determine the array size from the `...` initializer syntax."},

/* Incompatible class specifiers are used together. */
{"invalid combination of class specifiers", "A combination of storage class specifiers (e.g., `public`, `static`) is used that is not allowed by the language."},

/* A character value exceeds the 8-bit range. */
{"character constant exceeds range", "A character constant has a value that is outside the valid 0-255 range for an 8-bit character."},

/* Named and positional parameters are mixed incorrectly in a function call. */
{"positional parameters must precede", "In a function call, all positional arguments must come before any named arguments."},

/* An array is declared without a specified size. */
{"unknown array size", "An array was declared without a specified size and without an initializer to infer the size. Array sizes must be explicit."},

/* Array sizes in an assignment do not match. */
{"array sizes do not match", "In an assignment, the source and destination arrays have different sizes."},

/* Array dimensions in an operation do not match. */
{"array dimensions do not match", "The dimensions of the arrays used in an operation (e.g., addition) do not match."},

/* A backslash is used at the end of a line incorrectly. */
{"invalid line continuation", "A backslash `\\` was used at the end of a line, but it is not being used to continue a preprocessor directive or string literal correctly."},

/* A numeric range expression is invalid. */
{"invalid range", "A range expression (e.g., in a state array) is syntactically or logically invalid."},

/* A function body is found without a corresponding function header. */
{"start of function body without function header", "A block of code `{ ... }` that looks like a function body was encountered, but there was no preceding function declaration."},



/* === FATAL ERRORS (Compiler Failure) === */

/* The compiler cannot open the source file or an included file. */
{"cannot read from file", "The specified source file or an included file could not be opened. It may not exist, or there may be permission issues."},

/* The compiler cannot write the output file (e.g., the compiled .amx). */
{"cannot write to file", "The compiler cannot write to the output file. The disk might be full, the file might be in use, or there may be permission issues."},

/* An internal compiler data structure has exceeded its capacity. */
{"table overflow", "An internal compiler table (for symbols, tokens, etc.) has exceeded its maximum size. The source code might be too complex."},

/* The system ran out of memory during compilation. */
{"insufficient memory", "The compiler ran out of available system memory (RAM) while processing the source code."},

/* An invalid opcode is used in an #emit directive. */
{"invalid assembler instruction", "The opcode specified in an `#emit` directive is not a valid Pawn assembly instruction."},

/* A numeric constant is too large for the compiler to handle. */
{"numeric overflow", "A numeric constant in the source code is too large to be represented."},

/* A single line of code produced too many errors. */
{"too many error messages on one line", "One line of source code generated a large number of errors. The compiler is stopping to avoid flooding the output."},

/* A codepage mapping file specified by the user was not found. */
{"codepage mapping file not found", "The file specified for character set conversion (codepage) could not be found."},

/* The provided file or directory path is invalid. */
{"invalid path", "A file or directory path provided to the compiler is syntactically invalid or does not exist."},

/* A compile-time assertion (#assert) failed. */
{"assertion failed", "A compile-time assertion check (using `#assert`) evaluated to false."},

/* The #error directive was encountered, forcing the compiler to stop. */
{"user error", "The `#error` preprocessor directive was encountered, explicitly halting the compilation process."},



/* === WARNINGS (Potentially Problematic Code) ===  */

/* literal array/string passed to a non-const parameter */
{"literal array/string passed to a non", "Did you forget that the parameter isn't a const parameter? Also, make sure you're using the latest version of the standard library."},

/* A symbol name is too long and is being truncated. */
{"is truncated to", "A symbol name (variable, function, etc.) exceeds the maximum allowed length and will be truncated, which may cause link errors."},

/* A constant or macro is redefined with a new value. */
{"redefinition of constant", "A constant or macro is being redefined. The new definition will override the previous one."},

/* A function is called with the wrong number of arguments. */
{"number of arguments does not match", "The number of arguments in a function call does not match the number of parameters in the function's declaration."},

/* A conditional expression is always true. */
{"redundant test: constant expression is non-zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to a non-zero value (true), making the test unnecessary."},

/* A non-const qualified array is passed to a function expecting a const array. */
{"array argument was intended", "A non-constant array is being passed to a function parameter that is declared as `const`. The function promises not to modify it, so this is safe but noted."},



/* ===  PREPROCESSOR ERRORS ===  */

/* The maximum allowed depth for #include directives has been exceeded. */
{"too many nested includes", "The level of nested `#include` directives has exceeded the compiler's limit. Check for circular or overly deep inclusion."},

/* A file attempts to include itself, directly or indirectly. */
{"recursive include", "A file is including itself, either directly or through a chain of other includes. This creates an infinite loop."},

/* Macro expansion has exceeded the recursion depth limit. */
{"macro recursion too deep", "The expansion of a recursive macro has exceeded the maximum allowed depth. Check for infinitely recursive macro definitions."},

/* A constant expression involves division by zero. */
{"division by zero", "A compile-time constant expression attempted to divide by zero."},

/* A constant expression calculation resulted in an overflow. */
{"overflow in constant expression", "A calculation in a constant expression (e.g., in an `#if` directive) resulted in an arithmetic overflow."},

/* A macro used in a conditional compilation directive is not defined. */
{"undefined macro", "A macro used in an `#if` or `#elif` directive has not been defined. Its value is assumed to be zero."},

/* A function-like macro is used without the required arguments. */
{"missing preprocessor argument", "A function-like macro was invoked without providing the required number of arguments."},

/* A function-like macro is given more arguments than it expects. */
{"too many macro arguments", "A function-like macro was invoked with more arguments than specified in its definition."},

/* Extra text found after a preprocessor directive. */
{"extra characters on line", "Unexpected characters were found on the same line after a preprocessor directive (e.g., `#include <file> junk`)."},

/* Sentinel value to mark the end of the array. */
{NULL, NULL}
};

/*
 * Searches for known compiler warning and error patterns in a line of compiler output.
 * Iterates through the global constant array 'ccs' (Compiler Code Strings) which contains
 * pattern strings and their corresponding human-readable descriptions. Returns the
 * description for the first matching pattern, or NULL if no known pattern is found.
 */
static const char *wg_find_warn_err(const char *line)
{
      int cindex;
      /* Iterate through the compiler code strings array until a NULL terminator is found */
      for (cindex = 0; ccs[cindex].cs_t; ++cindex) {
        /* Make sure the current pattern exists anywhere in the input line */
        if (strstr(line, ccs[cindex].cs_t))
            return ccs[cindex].cs_i; /* Return the human-readable description */
      }
      return NULL; /* No known pattern matched */
}

/*
 * Displays detailed compilation results including file statistics, memory usage breakdown,
 * and DJB2 hash of the compiled output file. Updates console title with compilation status.
 * Only shows detailed information when debug mode is enabled and the output file exists.
 */
void compiler_detailed(const char *wgoutput, int debug,
                       int wcnt, int ecnt, const char *compiler_ver,
                       int header_size, int code_size, int data_size,
                       int stack_size, int total_size)
{
      /* Compiler Info - [pass] : warning - [fail] : error */
      println(stdout,
        "Compile Complete! | " FCOLOUR_CYAN "%d pass (warning) | " FCOLOUR_RED "%d fail (error)" FCOLOUR_DEFAULT "",
        wcnt, ecnt);
      println(stdout, "-----------------------------");

      /* Make sure the compiled output file exists and debug information is requested */
      int amx_access = path_access(wgoutput);
      if (amx_access && debug != 0) {
                /* Calculate DJB2 hash of the compiled output file for verification */
                unsigned long hash = crypto_djb2_hash_file(wgoutput);

                /* Display memory usage breakdown of the compiled AMX file */
		        printf("Output path: %s\n", wgoutput);
                printf("Header : %dB  |  Total        : %dB\n"
                       "Code   : %dB  |  hash (djb2)  : %#lx\n"
                       "Data   : %dB\n"
                       "Stack  : %dB\n",
                       header_size, total_size, code_size,
                       hash, data_size, stack_size);
		         fflush(stdout);

                /* Retrieve and display detailed file system statistics */
                portable_stat_t st;
                if (portable_stat(wgoutput, &st) == 0) {
                        /* Display comprehensive file metadata including permissions, timestamps, and identifiers */
                        printf("ino    : %llu   |  File   : %lluB\n"
                               "dev    : %llu\n"
                               "read   : %s   |  write  : %s\n"
                               "execute: %s   |  mode   : %020o\n"
                               "atime  : %llu\n"
                               "mtime  : %llu\n"
                               "ctime  : %llu\n",
                               (unsigned long long)st.st_ino,      /* Inode number */
                               (unsigned long long)st.st_size,     /* File size in bytes */
                               (unsigned long long)st.st_dev,      /* Device ID */
                               (st.st_mode & S_IRUSR) ? "Y" : "N", /* Read permission for owner */
                               (st.st_mode & S_IWUSR) ? "Y" : "N", /* Write permission for owner */
                               (st.st_mode & S_IXUSR) ? "Y" : "N", /* Execute permission for owner */
                               st.st_mode,                         /* Full permission mode in octal */
                               (unsigned long long)st.st_latime,   /* Last access time */
                               (unsigned long long)st.st_lmtime,   /* Last modification time */
                               (unsigned long long)st.st_mctime    /* Last status change time */
                        );
                        fflush(stdout);
                }
      }

      fflush(stdout);  /* Ensure all output is written before continuing */
      printf("\n");    /* Add blank line for separation */

      /* Display Pawn compiler copyright information */
      printf("* Pawn Compiler %s - Copyright (c) 1997-2006, ITB CompuPhase\n", compiler_ver);

      return;
}

/*
 * Analyzes compiler log file to extract errors, warnings, and compilation statistics.
 * Parses the log line-by-line, categorizes issues, provides descriptive explanations
 * for known error patterns, and displays comprehensive compilation summary.
 * Includes detailed debugging information when compiled with debug flags.
 */
static void cause_compiler_expl(const char *log_file, const char *wgoutput, int debug)
{
        /* Debug information section */
        __debug_function();

      /* Open compiler log file for reading */
      FILE *plog = fopen(log_file, "r");
      if (!plog)
        return; /* Exit if log file cannot be opened */

      char line[WG_MAX_PATH];        /* Buffer for reading each log line */
      int wcnt = 0, ecnt = 0;        /* Warning and error counters */
      int header_size = 0, code_size = 0, data_size = 0; /* AMX memory section sizes */
      int stack_size = 0, total_size = 0;                /* Stack and total memory usage */
      char compiler_ver[64] = { 0 }; /* Buffer for compiler version string */

      /* Process each line of the compiler log file */
      while (fgets(line, sizeof(line), plog)) {
        /* Skip summary lines that don't contain actual error/warning information */
        if (wg_strcase(line, "Warnings.") ||
            wg_strcase(line, "Warning.") ||
            wg_strcase(line, "Errors.")  ||
            wg_strcase(line, "Error."))
            continue;

        /* Parse memory usage statistics from compiler output */
        if (wg_strcase(line, "Header size:")) {
            /* Extract header size value after colon and convert to integer */
            header_size = strtol(strchr(line, ':') + 1, NULL, 10);
            continue;
        } else if (wg_strcase(line, "Code size:")) {
            code_size = strtol(strchr(line, ':') + 1, NULL, 10);
            continue;
        } else if (wg_strcase(line, "Data size:")) {
            data_size = strtol(strchr(line, ':') + 1, NULL, 10);
            continue;
        } else if (wg_strcase(line, "Stack/heap size:")) {
            stack_size = strtol(strchr(line, ':') + 1, NULL, 10);
            continue;
        } else if (wg_strcase(line, "Total requirements:")) {
            total_size = strtol(strchr(line, ':') + 1, NULL, 10);
            continue;
        } else if (wg_strcase(line, "Pawn compiler ")) {
            /* Extract compiler version from version string */
            const char *p = strstr(line, "Pawn compiler ");
            if (p) sscanf(p, "Pawn compiler %63s", compiler_ver);
            continue;
        }

        /* Output the original compiler message line to stdout */
        fwrite(line, 1, strlen(line), stdout);

        /* Count warnings and errors based on keyword presence in line */
        if (wg_strcase(line, "warning")) ++wcnt;
        if (wg_strcase(line, "error")) ++ecnt;

        /* Look for known compiler error/warning patterns and provide explanations */
        const char *description = wg_find_warn_err(line);
        if (description) {
            const char *found = NULL;
            int mk_pos = 0;
            /* Find the exact position of the error pattern in the line */
            for (int i = 0; ccs[i].cs_t; ++i) {
                if ((found = strstr(line, ccs[i].cs_t))) {
                    mk_pos = found - line; /* Calculate column position */
                    break;
                }
            }
            /* Print caret indicator under the error location with colored explanation */
            for (int i = 0; i < mk_pos; i++)
                putchar(' '); /* Space to align caret with error location */
            pr_color(stdout, FCOLOUR_BLUE, "^ %s :(\n", description);
        }
      }

      fclose(plog); /* Close the log file */

      /* Display detailed compilation summary with all collected statistics */
      compiler_detailed(wgoutput, debug, wcnt, ecnt,
                        compiler_ver, header_size, code_size,
                        data_size, stack_size, total_size);
}
