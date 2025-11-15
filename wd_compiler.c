#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include "wd_util.h"
#ifdef WD_LINUX
#include <fcntl.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_compiler.h"

#define WATCHDOGS_COMPILER_ZERO 0

/* Global compiler state buffers for file paths and temporary data */
static char container_output[WD_PATH_MAX] = { 0 },            /* Output file path buffer */
            __wcp_direct_path[WD_PATH_MAX] = { 0 },           /* Directory path for compilation */
            __wcp_file_name[WD_PATH_MAX] = { 0 },             /* File name for compilation */
            __wcp_input_path[WD_PATH_MAX] = { 0 },            /* Full input file path */
            __wcp_any_tmp[WD_PATH_MAX] = { 0 };               /* Temporary buffer for path manipulation */

/**
 * Cleans up compiler memory buffers by resetting them to zero
 * Called before each compilation to ensure clean state
 */
void compiler_memory_clean(void) {
        /* Cleaning sef data */
        wd_sef_fdir_reset();
        /* Reset all global compiler buffers to empty state */
        memset(container_output, WATCHDOGS_COMPILER_ZERO, sizeof(container_output));
        memset(__wcp_direct_path, WATCHDOGS_COMPILER_ZERO, sizeof(__wcp_direct_path));
        memset(__wcp_file_name, WATCHDOGS_COMPILER_ZERO, sizeof(__wcp_file_name));
        memset(__wcp_input_path, WATCHDOGS_COMPILER_ZERO, sizeof(__wcp_input_path));
        memset(__wcp_any_tmp, WATCHDOGS_COMPILER_ZERO, sizeof(__wcp_any_tmp));
        return;
}

#ifdef WD_WINDOWS
void WD_COMPILER_WIN32_API_START(PROCESS_INFORMATION pi) {
        /** 
         * Get handles to core Windows DLLs
         * kernel32.dll contains core Windows API functions
         * ntdll.dll contains Native API functions (lower level)
         */
        HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
        HMODULE hNtdll = GetModuleHandle("ntdll.dll");
        
        if (hKernel32) {
            /**
             * SetProcessIoPriority - Controls I/O priority of a process
             * I/O priority affects how quickly the process's I/O requests are serviced
             * by the storage subsystem
             */
            typedef BOOL (WINAPI *PSETPROCESSIOPRIORITY)(HANDLE, INT);
            PSETPROCESSIOPRIORITY SetProcessIoPriority = 
                (PSETPROCESSIOPRIORITY)GetProcAddress(hKernel32, "SetProcessIoPriority");
            
            if (SetProcessIoPriority) {
                /** 
                 * Set I/O priority to 4 (HIGH priority)
                 * Windows I/O priority levels:
                 * 0 - Very Low, 1 - Low, 2 - Normal, 3 - High, 4 - High, 5 - Critical
                 * This gives the process faster access to disk I/O resources
                 */
                SetProcessIoPriority(pi.hProcess, 4);  // HIGH priority
            }

            /**
             * SetProcessInformation - Sets various process information types
             * PROCESS_INFORMATION_CLASS specifies what type of information to set
             */
            typedef BOOL (WINAPI *PSETPROCESSINFORMATION)(HANDLE, PROCESS_INFORMATION_CLASS, PVOID, ULONG);
            PSETPROCESSINFORMATION SetProcessInformation = 
                (PSETPROCESSINFORMATION)GetProcAddress(hKernel32, "SetProcessInformation");

            if (SetProcessInformation) {
                /**
                 * MEMORY_PRIORITY_INFORMATION - Controls memory management behavior
                 * MemoryPriority range: 0-5 (0 = lowest, 5 = highest)
                 * Higher priority means:
                 * - Pages are less likely to be paged out to disk
                 * - Better chance of keeping working set in physical memory
                 * - Potentially faster memory access
                 */
                MEMORY_PRIORITY_INFORMATION memPri = { 0 };
                memPri.MemoryPriority = 3;  // 0-5, 5 = highest
                SetProcessInformation(pi.hProcess, ProcessMemoryPriority, &memPri, sizeof(memPri));
            }

            /**
             * SetProcessWorkingSetSizeEx - Sets minimum and maximum working set sizes
             * Working set = portion of virtual address space that is resident in physical memory
             */
            typedef BOOL (WINAPI *PSETPROCESSWORKINGSETSIZEEX)(HANDLE, PSIZE_T, PSIZE_T, DWORD);
            PSETPROCESSWORKINGSETSIZEEX SetProcessWorkingSetSizeEx = 
                (PSETPROCESSWORKINGSETSIZEEX)GetProcAddress(hKernel32, "SetProcessWorkingSetSizeEx");

            if (SetProcessWorkingSetSizeEx) {
                /**
                 * Setting both min and max to -1 means:
                 * - No minimum working set limit
                 * - No maximum working set limit  
                 * - Process can use as much physical memory as needed
                 * - Windows memory manager won't artificially restrict memory usage
                 */
                SIZE_T minWs = (SIZE_T)-1;  // No limit
                SIZE_T maxWs = (SIZE_T)-1;  // No limit  
                SetProcessWorkingSetSizeEx(pi.hProcess, &minWs, &maxWs, 0);  // Flags = 0
            }
        }
        
        if (hNtdll) {
            /**
             * NtSetPowerRequest - Native API function for power management
             * Part of Windows Native API (undocumented/low-level)
             */
            typedef NTSTATUS (WINAPI *PNTSETPOWERREQUEST)(HANDLE, POWER_REQUEST_TYPE);
            PNTSETPOWERREQUEST NtSetPowerRequest = 
                (PNTSETPOWERREQUEST)GetProcAddress(hNtdll, "NtSetPowerRequest");

            if (NtSetPowerRequest) {
                /**
                 * PowerRequestExecutionRequired - Prevents system from entering sleep mode
                 * while the process is running
                 * Useful for:
                 * - Compilation processes that shouldn't be interrupted
                 * - Critical operations that require continuous execution
                 * - Preventing system sleep during long-running tasks
                 */
                HANDLE powerRequest = NULL;
                NtSetPowerRequest(powerRequest, PowerRequestExecutionRequired);
            }
        }
        
        return;
}
#endif

/**
 * Main compiler execution function with extensive configuration handling
 * Supports TOML-based configuration, cross-platform execution, and multiple compilation modes
 * 
 * @param arg Primary argument (can be NULL for TOML configuration)
 * @param compile_args Compilation arguments string
 * @param second_arg Secondary compiler argument
 * @param four_arg Fourth compiler argument  
 * @param five_arg Fifth compiler argument
 * @param six_arg Sixth compiler argument
 * @param seven_arg Seventh compiler argument
 * @param eight_arg Eighth compiler argument
 * @return __RETZ on success, __RETN on failure or user cancellation
 */
int wd_run_compiler(const char *arg,
                    const char *compile_args,
                    const char *second_arg,
                    const char *four_arg,
                    const char *five_arg,
                    const char *six_arg,
                    const char *seven_arg,
                    const char *eight_arg)
{
        /* Reset compiler status flag */
        wcfg.wd_compiler_stat = 0;

        /* Local variables for file handling and command execution */
        FILE *proc_file;                          /* File pointer for log processing */
        char _compiler_input_[WD_MAX_PATH + 128] = { 0 };     /* Buffer for compiler command construction */
        char size_log[1024];                      /* Buffer for log file reading */
        char run_cmd[WD_PATH_MAX + 258];          /* Buffer for system commands */
        char include_aio_path[WD_PATH_MAX] = { 0 }; /* Buffer for include paths */

        /* Path separator detection pointers */
        char *compiler_last_slash = NULL;         /* Last forward slash in path */
        char *compiler_back_slash = NULL;         /* Last backslash in path */

        /* Compiler option flags */
        int compiler_debugging = 0;               /* Debug level flag */
        int compiler_has_watchdogs = 0, compiler_has_debug = 0;  /* Feature flags */
        int compiler_has_clean = 0, compiler_has_assembler = 0;
        int compiler_has_recursion = 0, compiler_has_verbose = 0;

        /* Array of compiler arguments for easy processing */
        const char* compiler_args[] = {
                                            second_arg,
                                            four_arg,
                                            five_arg,
                                            six_arg,
                                            seven_arg,
                                            eight_arg
                                      };

        /* Determine compiler executable name based on operating system */
        const char *ptr_pawncc = NULL;
        if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS))
            ptr_pawncc = "pawncc.exe";  /* Windows executable */
        else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX))
            ptr_pawncc = "pawncc";      /* Linux executable */

        /* Determine include directory based on server environment */
        char *path_include = NULL;
        if (wd_server_env() == 1)
            path_include="pawno/include";  /* SA-MP include directory */
        else if (wd_server_env() == 2)
            path_include="qawno/include";  /* OpenMP include directory */

        /* Clean up previous compiler log if it exists */
        int _wd_log_acces = path_acces(".wd_compiler.log");
        if (_wd_log_acces)
            remove(".wd_compiler.log");

        /* Reset file search and clean compiler memory buffers */
        compiler_memory_clean();

        /* Search for pawn compiler executable in current directory */
        int __find_pawncc = wd_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc)  /* Compiler found - proceed with compilation */
        {
            /* Open and parse TOML configuration file */
            FILE *procc_f;
            procc_f = fopen("watchdogs.toml", "r");
            if (!procc_f)
            {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                return __RETZ;
            }

            /* Parse TOML configuration with error handling */
            char error_buffer[256];
            toml_table_t *_toml_config;
            _toml_config = toml_parse_file(procc_f,
                                           error_buffer,
                                           sizeof(error_buffer));

            if (procc_f) {
                fclose(procc_f);
            }

            /* Handle TOML parsing errors */
            if (!_toml_config)
            {
                pr_error(stdout, "parsing TOML: %s", error_buffer);
                return __RETZ;
            }

            /* Converter Data */
            char wd_compiler_input_pawncc_path[PATH_MAX], wd_compiler_input_gamemode_path[PATH_MAX];
            wd_snprintf(wd_compiler_input_pawncc_path,
                    sizeof(wd_compiler_input_pawncc_path), "%s", wcfg.wd_sef_found_list[0]);

            /* Test compiler capabilities by running with DDD flag */
            wd_snprintf(run_cmd, sizeof(run_cmd),
                "%s -DDD > .__CP.log 2>&1",
                  wd_compiler_input_pawncc_path);
            wd_run_command(run_cmd);

            /* Open compiler test output for analysis */
            proc_file = fopen(".__CP.log", "r");
            if (!proc_file) {
                pr_error(stdout, "Failed to open .__CP.log");
            }

            /* Analyze compiler arguments to detect feature flags */
            for (int i = 0; i < 5; i++) {
                if (compiler_args[i] != NULL) {
                    if (strfind(compiler_args[i], "--watchdogs"))
                        ++compiler_has_watchdogs;  /* Enable watchdogs analysis */
                    if(strfind(compiler_args[i], "--debug"))
                        ++compiler_has_debug;      /* Enable debug output */
                    if (strfind(compiler_args[i], "--clean"))
                        ++compiler_has_clean;      /* Clean output after compilation */
                    if (strfind(compiler_args[i], "--assembler"))
                        ++compiler_has_assembler;  /* Generate assembler output */
                    if (strfind(compiler_args[i], "--recursion"))
                        ++compiler_has_recursion;  /* Enable recursion */
                    if (strfind(compiler_args[i], "--verbose"))
                        ++compiler_has_verbose;    /* Enable verbose output */
                }
            }

            /* Process compiler configuration from TOML */
            toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
            if (wd_compiler)
            {
                /* Process compiler options array from TOML */
                toml_array_t *option_arr = toml_array_in(wd_compiler, "option");
                if (option_arr)
                {
                    size_t arr_sz = toml_array_nelem(option_arr);  /* Number of options */
                    char *merged = NULL;  /* Buffer for merged compiler options */

                    /* Process each compiler option */
                    for (size_t i = 0; i < arr_sz; i++)
                    {
                        toml_datum_t opt_val = toml_string_at(option_arr, i);
                        if (!opt_val.ok)
                            continue;

                        int rate_valid_flag = 0;  /* Flag for option validation */

                        /* Extract flag prefix for validation (first 2 characters) */
                        char flag_to_search[3] = {0};
                        size_t size_flag_to_search = sizeof(flag_to_search);
                        if (strlen(opt_val.u.s) >= 2) {
                            wd_snprintf(flag_to_search,
                                     size_flag_to_search,
                                     "%.2s",
                                     opt_val.u.s);
                        } else {
                            wd_strncpy(flag_to_search, opt_val.u.s, size_flag_to_search - 1);
                        }

                        /* Validate compiler option against compiler capabilities */
                        if (proc_file) {
                            rewind(proc_file);  /* Reset file pointer to beginning */
                            while (fgets(size_log, sizeof(size_log), proc_file) != NULL) {
                                if (strfind(size_log, "error while loading shared libraries:")) {
                                    wd_printfile(".wd_compiler.log");
                                    return __RETZ;
                                }
                                if (strstr(size_log, flag_to_search)) {
                                    rate_valid_flag = 1;  /* Option is supported */
                                    break;
                                }
                            }
                        }

                        /* Skip invalid options */
                        if (rate_valid_flag == 0)
                            goto n_valid_flag;

                        /* Check if option starts with dash (valid compiler flag) */
                        if (opt_val.u.s[0] != '-') {
n_valid_flag:
                            printf("[WARN]: compiler option ");
                            pr_color(stdout, FCOLOUR_GREEN, "\"%s\" ", opt_val.u.s);
                            println(stdout, "not valid flag!");
                            if (rate_valid_flag == 0)
                              return __RETZ;
                        }

                        /* Track debug options for later processing */
                        if (strfind(opt_val.u.s, "-d"))
                          ++compiler_debugging;

                        /* Merge option into combined options string */
                        size_t old_len = merged ? strlen(merged) : 0,
                               new_len = old_len + strlen(opt_val.u.s) + 2;

                        char *tmp = wd_realloc(merged, new_len);
                        if (!tmp) {
                            wd_free(merged);
                            wd_free(opt_val.u.s);
                            merged = NULL;
                            break;
                        }
                        merged = tmp;

                        /* Append option to merged string */
                        if (!old_len)
                            wd_snprintf(merged, new_len, "%s", opt_val.u.s);
                        else
                            wd_snprintf(merged + old_len,
                                     new_len - old_len,
                                     " %s", opt_val.u.s);

                        wd_free(opt_val.u.s);
                        opt_val.u.s = NULL;
                    }

                    /* Clean up temporary files */
                    if (proc_file) {
                        fclose(proc_file);
                        if (path_acces(".__CP.log"))
                            remove(".__CP.log");
                    }

                    /* Store merged options in global configuration */
                    if (merged) {
                        wcfg.wd_toml_aio_opt = merged;
                    }
                    else {
                        wcfg.wd_toml_aio_opt = strdup("");  /* Empty options */
                    }
                }

                /* Apply feature-based compiler option modifications - APPEND ONLY */
                char compiler_addt_opt[PATH_MAX] = "";
                if (compiler_has_debug && compiler_debugging) {
                    /* Remove only conflicting debug options */
                    const char *mem_target[] = {"-d1", "-d3"};
                    size_t n_mem_target = sizeof(mem_target) / sizeof(mem_target[0]);
                    for (size_t i = 0; i < n_mem_target; i++) {
                        const char *debug_opt = mem_target[i];
                        size_t len = strlen(debug_opt);
                        char *pos;
                        while ((pos = strstr(wcfg.wd_toml_aio_opt, debug_opt)) != NULL) {
                            memmove(pos, pos + len, strlen(pos + len) + 1);
                        }
                    }
                }

                /* Add debug options if compiler supports debugging */
                if (compiler_has_debug)
                    strcat(compiler_addt_opt, " -d2 ");

                /* Add assembler output option if supported */
                if (compiler_has_assembler)
                    strcat(compiler_addt_opt, " -a ");

                /* Add recursion support option if available */
                if (compiler_has_recursion)
                    strcat(compiler_addt_opt, " -R+ ");

                /* Add verbose output option if available */
                if (compiler_has_verbose)
                    strcat(compiler_addt_opt, " -v2 ");

                /* Append the additional options to main compiler options if any exist */
                if (strlen(compiler_addt_opt) > 0) {
                    size_t current_len, addt_len;
                    current_len = strlen(wcfg.wd_toml_aio_opt);
                    addt_len = strlen(compiler_addt_opt);
                    
                    /* Check buffer size to prevent overflow */
                    if (current_len + addt_len < sizeof(wcfg.wd_toml_aio_opt)) {
                        strcat(wcfg.wd_toml_aio_opt, compiler_addt_opt);
                    } else {
                        /* Use safe concatenation if buffer is nearly full */
                        strncat(wcfg.wd_toml_aio_opt, compiler_addt_opt, 
                                sizeof(wcfg.wd_toml_aio_opt) - current_len - 1);
                    }
                }

                /* Process include paths from TOML configuration */
                toml_array_t *include_paths = toml_array_in(wd_compiler, "include_path");
                if (include_paths)
                {
                    int array_size = toml_array_nelem(include_paths);  /* Number of include paths */

                    /* Process each include path */
                    for (int i = 0; i < array_size; i++)
                    {
                        toml_datum_t path_val = toml_string_at(include_paths, i);
                        if (path_val.ok)
                        {
                            /* Clean and validate the path */
                            char __procc[250];
                            wd_strip_dot_fns(__procc, sizeof(__procc), path_val.u.s);
                            if (__procc[0] == '\0')
                                continue;

                            /* Add space separator for multiple paths */
                            if (i > 0)
                            {
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1)
                                {
                                    wd_snprintf(include_aio_path + cur,
                                                sizeof(include_aio_path) - cur,
                                                " ");
                                }
                            }

                            /* Append include path with compiler flag */
                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1)
                            {
                                wd_snprintf(include_aio_path + cur,
                                            sizeof(include_aio_path) - cur,
                                            "-i\"%s\"",
                                            __procc);
                            }
                            wd_free(path_val.u.s);
                        }
                    }
                }

                /* Branch 1: TOML-based compilation (when no specific file is provided) */
                if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0'))
                {
                    /* Read input and output file paths from TOML configuration */
                    toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
                    if (toml_gm_i.ok)
                    {
                        wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
                        wd_free(toml_gm_i.u.s);
                        toml_gm_i.u.s = NULL;
                    }

                    toml_datum_t toml_gm_o = toml_string_in(wd_compiler, "output");
                    if (toml_gm_o.ok)
                    {
                        wcfg.wd_toml_gm_output = strdup(toml_gm_o.u.s);
                        wd_free(toml_gm_o.u.s);
                        toml_gm_o.u.s = NULL;
                    }

                    /* Timing variables for performance measurement */
                    struct timespec start = {0}, end = {0};
                    double compiler_dur;

                    /* Platform-specific compilation execution */
#ifdef WD_WINDOWS
                    /* Windows process creation structures */
                    PROCESS_INFORMATION pi;
                    STARTUPINFO si = { sizeof(si) };
                    SECURITY_ATTRIBUTES sa = { sizeof(sa) };

                    sa.bInheritHandle = TRUE;
                    /* Create log file for compiler output */
                    HANDLE hFile = CreateFileA(
                        ".wd_compiler.log",
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        &sa,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                    );
#if defined(_DBG_PRINT)
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;

                    /* Redirect stdout and stderr to log file */
                    if (hFile != INVALID_HANDLE_VALUE) {
                        si.hStdOutput = hFile;
                        si.hStdError = hFile;
                    }

                    /* Build compiler command string */
                    int ret_command = 0;
                    ret_command = wd_snprintf(_compiler_input_,
                                    sizeof(_compiler_input_),
                                        "%s %s -o%s %s %s -i%s",
                                        wd_compiler_input_pawncc_path, // Pawncc Path
                                        wcfg.wd_toml_gm_input,         // Gamemode Input
                                        wcfg.wd_toml_gm_output,        // Gamemode Output
                                        wcfg.wd_toml_aio_opt,          // All-in-One Options
                                        include_aio_path,              // All-in-One Path
                                        path_include);                 // More Include path

                    /* Start timing and execute compiler */
                    clock_gettime(CLOCK_MONOTONIC, &start);
                    if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                        BOOL win32_process_succes = CreateProcessA(
                            NULL,            // No module name
                            _compiler_input_,// Command line
                            NULL,            // Process handle not inheritable
                            NULL,            // Thread handle not inheritable
                            TRUE,            // Set handle inheritance to TRUE
                            CREATE_NO_WINDOW,// Creation flags
                            NULL,            // Use parent's environment block
                            NULL,            // Use parent's starting directory
                            &si,             // Pointer to STARTUPINFO structure
                            &pi              // Pointer to PROCESS_INFORMATION structure
                        );
                        if (win32_process_succes) {
                            WD_COMPILER_WIN32_API_START(pi);

                            WaitForSingleObject(pi.hProcess, INFINITE);

                            clock_gettime(CLOCK_MONOTONIC, &end);

                            DWORD exit_code;
                            GetExitCodeProcess(pi.hProcess, &exit_code);

                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            pr_error(stdout, "CreateProcess failed (%lu)", GetLastError());
                        }
                    } else {
                          clock_gettime(CLOCK_MONOTONIC, &end);
                    }
                    if (hFile != INVALID_HANDLE_VALUE) {
                        CloseHandle(hFile);
                    }
#else
                    /* Unix/Linux compilation using fork and exec */
                    wd_snprintf(_compiler_input_, sizeof(_compiler_input_), "%s %s %s%s %s%s %s%s",
                        wd_compiler_input_pawncc_path,   // Pawncc Path
                        wcfg.wd_toml_gm_input,           // Gamemode Input
                        "-o",                            // Output Options
                        wcfg.wd_toml_gm_output,          // Gamemode Output
                        wcfg.wd_toml_aio_opt,            // All-in-One Options
                        include_aio_path,                // All-in-One Path
                        "-i",                            // Include Path Options
                        path_include);                   // More Include Path
#if defined(_DBG_PRINT)
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                    /* Prepare arguments untuk posix_spawn */
                    int i = 0;
                    char *wd_compiler_unix_args[WD_MAX_PATH + 256] = { 0 };
                    char *compiler_unix_token = strtok(_compiler_input_, " ");
                    while (compiler_unix_token != NULL) {
                        wd_compiler_unix_args[i++] = compiler_unix_token;
                        compiler_unix_token = strtok(NULL, " ");
                    }
                    wd_compiler_unix_args[i] = NULL;

                    /* Setup file actions untuk redirect output */
                    posix_spawn_file_actions_t process_file_actions;
                    posix_spawn_file_actions_init(&process_file_actions);
                    int fd = open(".wd_compiler.log", O_WRONLY |
                                                      O_CREAT  |
                                                      O_TRUNC  |
                                                      O_SYNC,
                                                      0644);
                    if (fd != -1) {
                            posix_spawn_file_actions_adddup2(&process_file_actions, fd, STDOUT_FILENO);
                            posix_spawn_file_actions_adddup2(&process_file_actions, fd, STDERR_FILENO);
                            posix_spawn_file_actions_addclose(&process_file_actions, fd);
                    }

                    pid_t compiler_process_id;
                    clock_gettime(CLOCK_MONOTONIC, &start);
                    int process_spawn_result = posix_spawnp(&compiler_process_id,
                                wd_compiler_unix_args[0],
                                &process_file_actions,
                                NULL,
                                wd_compiler_unix_args,
                                NULL);
                    clock_gettime(CLOCK_MONOTONIC, &end);

                    /* Cleanup file actions */
                    posix_spawn_file_actions_destroy(&process_file_actions);

                    if (process_spawn_result == 0) {
                        const int PROC_TIMEOUT = 30;
                        int process_status;
                        int process_timeout_occurred = 0;
                        for (int i = 0; i < PROC_TIMEOUT; i++) {
                            int p_result = -1;
                            p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                            if (p_result == 0)
                                sleep(1);
                            else if (p_result == compiler_process_id)
                                break;
                            else {
                                pr_error(stdout, "waitpid error");
                                break;
                            }
                            if (i == PROC_TIMEOUT - 1) {
                                kill(compiler_process_id, SIGTERM);
                                sleep(2);
                                kill(compiler_process_id, SIGKILL);
                                pr_error(stdout, "Process execution timeout (%d seconds)", PROC_TIMEOUT);
                                waitpid(compiler_process_id, &process_status, 0);
                                process_timeout_occurred = 1;
                            }
                        }
                        if (!process_timeout_occurred) {
                            if (WIFEXITED(process_status)) {
                                int exit_code = WEXITSTATUS(process_status);
                                if (exit_code != 0)
                                    pr_error(stdout,
                                             "watchdogs compiler exited with code (%d)", exit_code);
                            } else if (WIFSIGNALED(process_status)) {
                                pr_error(stdout,
                                         "watchdogs compiler terminated by signal (%d)", WTERMSIG(process_status));
                            }
                        }
                    } else {
                        pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result));
                    }
#endif
                    /* Prepare output file path for post-processing */
                    char _container_output[WD_PATH_MAX + 128];
                    wd_snprintf(_container_output, sizeof(_container_output), "%s", wcfg.wd_toml_gm_output);
                    if (path_exists(".wd_compiler.log")) {
                        printf("\n");
                        char *ca = NULL;
                        ca = _container_output;
                        int cb = 0;
                        cb = compiler_debugging;
                        if (compiler_has_watchdogs && compiler_has_clean) {
                            cause_compiler_expl(".wd_compiler.log", ca, cb);
                            if (path_exists(ca)) {
                                    remove(ca);
                            }
                            goto compiler_done;
                        } else if (compiler_has_watchdogs) {
                            cause_compiler_expl(".wd_compiler.log", ca, cb);
                            goto compiler_done;
                        } else if (compiler_has_clean) {
                            wd_printfile(".wd_compiler.log");
                            if (path_exists(ca)) {
                                    remove(ca);
                            }
                            goto compiler_done;
                        }

                        wd_printfile(".wd_compiler.log");
                    }
/* Label for compiler completion handling */
compiler_done:
                    /* Check for compilation errors in log file */
                    procc_f = fopen(".wd_compiler.log", "r");
                    if (procc_f)
                    {
                        char compiler_line_buffer[526];
                        int compiler_has_err = 0;
                        while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), procc_f))
                        {
                            if (strstr(compiler_line_buffer, "error")) {
                                compiler_has_err = 1;
                                break;
                            }
                        }
                        fclose(procc_f);
                        if (compiler_has_err)
                        {
                            /* Clean up output file on compilation error */
                            if (_container_output[0] != '\0' && path_acces(_container_output))
                                remove(_container_output);
                            wcfg.wd_compiler_stat = 1;  /* Set error status */
                        }
                    }
                    else {
                        pr_error(stdout, "Failed to open .wd_compiler.log");
                    }

                    /* Calculate and display compilation duration */
                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
                }
                /* Branch 2: File-based compilation (when specific file is provided) */
                else
                {
                    /* Parse file path to extract directory and filename */
                    wd_strncpy(__wcp_any_tmp, compile_args, sizeof(__wcp_any_tmp) - 1);
                    __wcp_any_tmp[sizeof(__wcp_any_tmp) - 1] = '\0';

                    /* Find last path separator (supporting both / and \) */
                    compiler_last_slash = strrchr(__wcp_any_tmp, '/');
                    compiler_back_slash = strrchr(__wcp_any_tmp, '\\');

                    /* Prefer backslash on Windows, forward slash otherwise */
                    if (compiler_back_slash &&
                       (!compiler_last_slash ||
                        compiler_back_slash > compiler_last_slash))
                    {
                        compiler_last_slash = compiler_back_slash;
                    }

                    /* Extract directory and filename from path */
                    if (compiler_last_slash)
                    {
                        size_t __dir_len;
                        __dir_len = (size_t)(compiler_last_slash - __wcp_any_tmp);

                        /* Safely copy directory path */
                        if (__dir_len >= sizeof(__wcp_direct_path))
                            __dir_len = sizeof(__wcp_direct_path) - 1;

                        memcpy(__wcp_direct_path, __wcp_any_tmp, __dir_len);
                        __wcp_direct_path[__dir_len] = '\0';

                        /* Extract filename after last separator */
                        const char *__wcp_filename_start = compiler_last_slash + 1;
                        size_t __wcp_filename_len;
                        __wcp_filename_len = strlen(__wcp_filename_start);

                        /* Safely copy filename */
                        if (__wcp_filename_len >= sizeof(__wcp_file_name))
                            __wcp_filename_len = sizeof(__wcp_file_name) - 1;

                        memcpy(__wcp_file_name, __wcp_filename_start, __wcp_filename_len);
                        __wcp_file_name[__wcp_filename_len] = '\0';

                        /* Reconstruct full input path */
                        size_t total_needed;
                        total_needed = strlen(__wcp_direct_path) + 1 +
                                    strlen(__wcp_file_name) + 1;

                        /* Handle path length overflow */
                        if (total_needed > sizeof(__wcp_input_path))
                        {
                            /* Fallback to gamemodes directory */
                            wd_strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            /* Truncate filename if necessary */
                            size_t __wcp_max_file_name;
                            __wcp_max_file_name = sizeof(__wcp_file_name) - 1;

                            if (__wcp_filename_len > __wcp_max_file_name)
                            {
                                memcpy(__wcp_file_name, __wcp_filename_start,
                                    __wcp_max_file_name);
                                __wcp_file_name[__wcp_max_file_name] = '\0';
                            }
                        }

                        /* Build full input path */
                        if (wd_snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                "%s/%s", __wcp_direct_path, __wcp_file_name) >=
                            (int)sizeof(__wcp_input_path))
                        {
                            __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                        }
                     }  else {
                            /* No path separator - file is in current directory */
                            wd_strncpy(__wcp_file_name, __wcp_any_tmp,
                                sizeof(__wcp_file_name) - 1);
                            __wcp_file_name[sizeof(__wcp_file_name) - 1] = '\0';

                            wd_strncpy(__wcp_direct_path, ".", sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            /* Build path with ./ prefix */
                            if (wd_snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "./%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }
                        }

                    /* Search for input file in specified directory */
                    int compiler_finding_compile_args = 0;
                    compiler_finding_compile_args = wd_sef_fdir(__wcp_direct_path, __wcp_file_name, NULL);

                    /* Fallback search in gamemodes directory if not found */
                    if (!compiler_finding_compile_args &&
                        strcmp(__wcp_direct_path, "gamemodes") != 0)
                    {
                        compiler_finding_compile_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            /* Update paths to use gamemodes directory */
                            wd_strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (wd_snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            /* Update search results if space available */
                            if (wcfg.wd_sef_count > MAX_SEF_EMPTY)
                                wd_strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    /* Additional fallback for current directory search */
                    if (!compiler_finding_compile_args && !strcmp(__wcp_direct_path, "."))
                    {
                        compiler_finding_compile_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            wd_strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (wd_snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            if (wcfg.wd_sef_count > MAX_SEF_EMPTY)
                                wd_strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    /* Converter Data */
                    wd_snprintf(wd_compiler_input_gamemode_path,
                            sizeof(wd_compiler_input_gamemode_path), "%s", wcfg.wd_sef_found_list[1]);

                    /* Proceed with compilation if file was found */
                    if (compiler_finding_compile_args)
                    {
                        /* Prepare output filename by removing extension */
                        char __sef_path_sz[WD_PATH_MAX];
                        wd_snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wd_compiler_input_gamemode_path);
                        char *f_EXT = strrchr(__sef_path_sz, '.');
                        if (f_EXT)
                            *f_EXT = '\0';

                        wd_snprintf(container_output, sizeof(container_output), "%s", __sef_path_sz);

                        /* Post-compilation processing (similar to Branch 1) */
                        char _container_output[WD_PATH_MAX + 128];
                        wd_snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);

                        /* Timing and platform-specific execution (similar to Branch 1) */
                        struct timespec start = {0}, end = {0};
                        double compiler_dur;
#ifdef WD_WINDOWS
                        /* Windows process creation (similar to above) */
                        PROCESS_INFORMATION pi;
                        STARTUPINFO si = { sizeof(si) };
                        SECURITY_ATTRIBUTES sa = { sizeof(sa) };

                        sa.bInheritHandle = TRUE;
                        HANDLE hFile = CreateFileA(
                            ".wd_compiler.log",
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            &sa,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                        );
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                        si.wShowWindow = SW_HIDE;

                        if (hFile != INVALID_HANDLE_VALUE) {
                            si.hStdOutput = hFile;
                            si.hStdError = hFile;
                        }

                        /* Build compiler command for specific file */
                        int ret_command = 0;
                        ret_command = wd_snprintf(_compiler_input_,
                                        sizeof(_compiler_input_),
                                            "%s %s -o%s %s %s -i%s",
                                            wd_compiler_input_pawncc_path,   // Pawncc Path
                                            wd_compiler_input_gamemode_path, // Gamemode Input
                                            _container_output,               // Gamemode Output
                                            wcfg.wd_toml_aio_opt,            // All-in-One Options
                                            include_aio_path,                // All-in-One Path
                                            path_include);                   // More Include Path

                        clock_gettime(CLOCK_MONOTONIC, &start);
                        if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                            BOOL win32_process_succes = CreateProcessA(
                                NULL,            // No module name
                                _compiler_input_,// Command line
                                NULL,            // Process handle not inheritable
                                NULL,            // Thread handle not inheritable
                                TRUE,            // Set handle inheritance to TRUE
                                CREATE_NO_WINDOW,// Creation flags
                                NULL,            // Use parent's environment block
                                NULL,            // Use parent's starting directory
                                &si,             // Pointer to STARTUPINFO structure
                                &pi              // Pointer to PROCESS_INFORMATION structure
                            );
                            if (win32_process_succes) {
                                WD_COMPILER_WIN32_API_START(pi);

                                WaitForSingleObject(pi.hProcess, INFINITE);

                                clock_gettime(CLOCK_MONOTONIC, &end);

                                DWORD exit_code;
                                GetExitCodeProcess(pi.hProcess, &exit_code);

                                CloseHandle(pi.hProcess);
                                CloseHandle(pi.hThread);
                            } else {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                pr_error(stdout, "CreateProcess failed (%lu)", GetLastError());
                            }
                        } else {
                              clock_gettime(CLOCK_MONOTONIC, &end);
                        }
                        if (hFile != INVALID_HANDLE_VALUE) {
                            CloseHandle(hFile);
                        }
#else
                        /* Unix/Linux execution dengan posix_spawn */
                        wd_snprintf(_compiler_input_, sizeof(_compiler_input_), "%s %s %s%s %s%s %s%s",
                            wd_compiler_input_pawncc_path,          // Pawncc Path
                            wd_compiler_input_gamemode_path,        // Gamemode Input
                            "-o",                                   // Output Options
                            _container_output,                      // Gamemode Output
                            wcfg.wd_toml_aio_opt,                   // All-in-One Options
                            include_aio_path,                       // All-in-One Path
                            "-i",                                   // Include Path Options
                            path_include);                          // More Include Path
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                        /* Prepare arguments untuk posix_spawn */
                        int i = 0;
                        char *wd_compiler_unix_args[WD_MAX_PATH + 256] = { 0 };
                        char *compiler_unix_token = strtok(_compiler_input_, " ");
                        while (compiler_unix_token != NULL) {
                            wd_compiler_unix_args[i++] = compiler_unix_token;
                            compiler_unix_token = strtok(NULL, " ");
                        }
                        wd_compiler_unix_args[i] = NULL;

                        /* Setup file actions untuk redirect output */
                        posix_spawn_file_actions_t process_file_actions;
                        posix_spawn_file_actions_init(&process_file_actions);
                        int fd = open(".wd_compiler.log", O_WRONLY |
                                                          O_CREAT  |
                                                          O_TRUNC  |
                                                          O_SYNC,
                                                          0644);
                        if (fd != -1) {
                                posix_spawn_file_actions_adddup2(&process_file_actions, fd, STDOUT_FILENO);
                                posix_spawn_file_actions_adddup2(&process_file_actions, fd, STDERR_FILENO);
                                posix_spawn_file_actions_addclose(&process_file_actions, fd);
                        }

                        pid_t compiler_process_id;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        int process_spawn_result = posix_spawnp(&compiler_process_id,
                                    wd_compiler_unix_args[0],
                                    &process_file_actions,
                                    NULL,
                                    wd_compiler_unix_args,
                                    NULL);
                        clock_gettime(CLOCK_MONOTONIC, &end);

                        /* Cleanup file actions */
                        posix_spawn_file_actions_destroy(&process_file_actions);

                        if (process_spawn_result == 0) {
                            const int PROC_TIMEOUT = 30;
                            int process_status;
                            int process_timeout_occurred = 0;
                            for (int i = 0; i < PROC_TIMEOUT; i++) {
                                int p_result = -1;
                                p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                                if (p_result == 0)
                                    sleep(1);
                                else if (p_result == compiler_process_id)
                                    break;
                                else {
                                    pr_error(stdout, "waitpid error");
                                    break;
                                }
                                if (i == PROC_TIMEOUT - 1) {
                                    kill(compiler_process_id, SIGTERM);
                                    sleep(2);
                                    kill(compiler_process_id, SIGKILL);
                                    pr_error(stdout, "Process execution timeout (%d seconds)", PROC_TIMEOUT);
                                    waitpid(compiler_process_id, &process_status, 0);
                                    process_timeout_occurred = 1;
                                }
                            }
                            if (!process_timeout_occurred) {
                                if (WIFEXITED(process_status)) {
                                    int exit_code = WEXITSTATUS(process_status);
                                    if (exit_code != 0)
                                        pr_error(stdout,
                                                 "watchdogs compiler exited with code (%d)", exit_code);
                                } else if (WIFSIGNALED(process_status)) {
                                    pr_error(stdout,
                                             "watchdogs compiler terminated by signal (%d)", WTERMSIG(process_status));
                                }
                            }
                        } else {
                            pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result));
                        }
#endif
                        if (path_exists(".wd_compiler.log")) {
                            printf("\n");
                            char *ca = NULL;
                            ca = _container_output;
                            int cb = 0;
                            cb = compiler_debugging;
                            if (compiler_has_watchdogs && compiler_has_clean) {
                                cause_compiler_expl(".wd_compiler.log", ca, cb);
                                if (path_exists(ca)) {
                                        remove(ca);
                                }
                                goto compiler_done2;
                            } else if (compiler_has_watchdogs) {
                                cause_compiler_expl(".wd_compiler.log", ca, cb);
                                goto compiler_done2;
                            } else if (compiler_has_clean) {
                                wd_printfile(".wd_compiler.log");
                                if (path_exists(ca)) {
                                        remove(ca);
                                }
                                goto compiler_done2;
                            }

                            wd_printfile(".wd_compiler.log");
                        }

/* Label for file-based compilation completion */
compiler_done2:
                        /* Error checking and cleanup (similar to Branch 1) */
                        procc_f = fopen(".wd_compiler.log", "r");
                        if (procc_f)
                        {
                            char compiler_line_buffer[526];
                            int compiler_has_err = 0;
                            while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), procc_f))
                            {
                                if (strstr(compiler_line_buffer, "error")) {
                                    compiler_has_err = 1;
                                    break;
                                }
                            }
                            fclose(procc_f);
                            if (compiler_has_err)
                            {
                                if (_container_output[0] != '\0' && path_acces(_container_output))
                                    remove(_container_output);
                                wcfg.wd_compiler_stat = 1;
                            }
                        }
                        else {
                            pr_error(stdout, "Failed to open .wd_compiler.log");
                        }

                        /* Display compilation timing */
                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        printf("\n");
                        pr_color(stdout, FCOLOUR_CYAN,
                            " <P> Finished at %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
                    }
                    else
                    {
                        /* File not found error */
                        printf("Cannot locate input: ");
                        pr_color(stdout, FCOLOUR_CYAN, "%s\n", compile_args);
                        return __RETZ;
                    }
                }

                /* Clean up TOML configuration */
                toml_free(_toml_config);

                return __RETZ;
            }
        } else {  /* Compiler not found - installation required */
            pr_crit(stdout, "pawncc not found!");

            /* Prompt user to install compiler */
            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");

            /* Installation decision loop */
            while (1)
            {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0)
                {
                    wd_free(ptr_sigA);
ret_ptr:
                    /* Platform selection menu */
                    println(stdout, "Select platform:");
                    println(stdout, "-> [L/l] Linux");
                    println(stdout, "-> [W/w] Windows");
                    pr_color(stdout, FCOLOUR_YELLOW, " ^ work's in WSL/MSYS2\n");
                    println(stdout, "-> [T/t] Termux");

                    wcfg.wd_sel_stat = 1;  /* Set selection status */

                    char *platform = readline("==> ");

                    /* Platform-specific installation */
                    if (strfind(platform, "L"))
                    {
                        int ret = wd_install_pawncc("linux");
loop_ipcc:
                        wd_free(platform);
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc;  /* Retry on failure */
                    }
                    if (strfind(platform, "W"))
                    {
                        int ret = wd_install_pawncc("windows");
loop_ipcc2:
                        wd_free(platform);
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc2;  /* Retry on failure */
                    }
                    if (strfind(platform, "T"))
                    {
                        int ret = wd_install_pawncc("termux");
loop_ipcc3:
                        wd_free(platform);
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc3;  /* Retry on failure */
                    }
                    if (strfind(platform, "E")) {
                        wd_free(platform);
                        goto done;  /* Exit installation */
                    }
                    else {
                        pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                        wd_free(platform);
                        goto ret_ptr;  /* Retry platform selection */
                    }
done:
                    wd_main(NULL);  /* Return to main menu */
                }
                else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0)
                {
                    wd_free(ptr_sigA);
                    break;  /* Exit without installation */
                }
                else
                {
                    pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                    wd_free(ptr_sigA);
                    goto ret_ptr;  /* Retry installation prompt */
                }
            }
        }

        return __RETN;
}