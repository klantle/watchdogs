#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#ifdef __linux__
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_util.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_compiler.h"

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
        /* Reset all global compiler buffers to empty state */
        memset(container_output, 0, sizeof(container_output));
        memset(__wcp_direct_path, 0, sizeof(__wcp_direct_path));
        memset(__wcp_file_name, 0, sizeof(__wcp_file_name));
        memset(__wcp_input_path, 0, sizeof(__wcp_input_path));
        memset(__wcp_any_tmp, 0, sizeof(__wcp_any_tmp));
        return;
}

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
        char _compiler_[WD_PATH_MAX];             /* Buffer for compiler command construction */
        char size_log[1024];                      /* Buffer for log file reading */
        char run_cmd[WD_PATH_MAX + 258];          /* Buffer for system commands */
        char include_aio_path[WD_PATH_MAX] = { 0 }; /* Buffer for include paths */

        /* Path separator detection pointers */
        char *compiler_last_slash = NULL;         /* Last forward slash in path */
        char *compiler_back_slash = NULL;         /* Last backslash in path */

        /* Compiler option flags */
        int compiler_debugging = 0;               /* Debug level flag */
        int compiler_has_watchdogs, compiler_has_debug = 0;  /* Feature flags */
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
        wd_sef_fdir_reset();
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

            /* Test compiler capabilities by running with DDD flag */
            wd_snprintf(run_cmd, sizeof(run_cmd),
                "%s -DDD > .__CP.log 2>&1",
                  wcfg.wd_sef_found_list[0]);
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

                /* Apply feature-based compiler option modifications */
                char size_aio_option[PATH_MAX];
                if (compiler_has_debug && compiler_has_assembler && compiler_has_recursion && compiler_has_verbose) {
                    /* All features enabled - apply comprehensive options */
                    if (compiler_debugging) {
                        /* Remove existing debug options to avoid conflicts */
                        const char *mem_target[] =  {
                                                        "-d1",
                                                        "-d2",
                                                        "-d3"
                                                    };
                        size_t n_mem_target= sizeof(mem_target) / sizeof(mem_target[0]);
                        for (size_t i = 0; i < n_mem_target; i++) {
                            const char *debug_opt = mem_target[i];
                            size_t len = strlen(debug_opt);
                            char *pos;
                            while ((pos = strstr(wcfg.wd_toml_aio_opt, debug_opt)) != NULL) {
                                memmove(pos, pos + len, strlen(pos + len) + 1);
                            }
                        }
                    }
                    /* Apply debug level 2, assembler output, recursion, and verbose level 2 */
                    snprintf(size_aio_option, sizeof(size_aio_option), "%s -d2 -a -R+ -v2", wcfg.wd_toml_aio_opt);
                    strncpy(wcfg.wd_toml_aio_opt, size_aio_option, sizeof(wcfg.wd_toml_aio_opt) - 1);
                    wcfg.wd_toml_aio_opt[sizeof(wcfg.wd_toml_aio_opt) - 1] = '\0';
                }
                else if (compiler_has_debug) {
                    /* Debug-only mode */
                    if (compiler_debugging) {
                        /* Remove existing debug options */
                        const char *mem_target[] =  {
                                                        "-d1",
                                                        "-d2",
                                                        "-d3"
                                                    };
                        size_t n_mem_target= sizeof(mem_target) / sizeof(mem_target[0]);
                        for (size_t i = 0; i < n_mem_target; i++) {
                            const char *debug_opt = mem_target[i];
                            size_t len = strlen(debug_opt);
                            char *pos;
                            while ((pos = strstr(wcfg.wd_toml_aio_opt, debug_opt)) != NULL) {
                                memmove(pos, pos + len, strlen(pos + len) + 1);
                            }
                        }
                    }
                    /* Apply debug level 2 */
                    snprintf(size_aio_option, sizeof(size_aio_option), "%s -d2", wcfg.wd_toml_aio_opt);
                    strncpy(wcfg.wd_toml_aio_opt, size_aio_option, sizeof(wcfg.wd_toml_aio_opt) - 1);
                    wcfg.wd_toml_aio_opt[sizeof(wcfg.wd_toml_aio_opt) - 1] = '\0';
                }
                else if (compiler_has_assembler) {
                    /* Assembler output mode */
                    snprintf(size_aio_option, sizeof(size_aio_option), "%s -a", wcfg.wd_toml_aio_opt);
                    strncpy(wcfg.wd_toml_aio_opt, size_aio_option, sizeof(wcfg.wd_toml_aio_opt) - 1);
                    wcfg.wd_toml_aio_opt[sizeof(wcfg.wd_toml_aio_opt) - 1] = '\0';
                }
                else if (compiler_has_recursion) {
                    /* Recursion enabled */
                    snprintf(size_aio_option, sizeof(size_aio_option), "%s -R+", wcfg.wd_toml_aio_opt);
                    strncpy(wcfg.wd_toml_aio_opt, size_aio_option, sizeof(wcfg.wd_toml_aio_opt) - 1);
                    wcfg.wd_toml_aio_opt[sizeof(wcfg.wd_toml_aio_opt) - 1] = '\0';
                }
                else if (compiler_has_verbose) {
                    /* Verbose output mode */
                    snprintf(size_aio_option, sizeof(size_aio_option), "%s -v2", wcfg.wd_toml_aio_opt);
                    strncpy(wcfg.wd_toml_aio_opt, size_aio_option, sizeof(wcfg.wd_toml_aio_opt) - 1);
                    wcfg.wd_toml_aio_opt[sizeof(wcfg.wd_toml_aio_opt) - 1] = '\0';
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

                    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;

                    /* Redirect stdout and stderr to log file */
                    if (hFile != INVALID_HANDLE_VALUE) {
                        si.hStdOutput = hFile;
                        si.hStdError = hFile;
                    }

                    /* Build compiler command string */
                    int ret_cmd = wd_snprintf(
                                                _compiler_,
                                                sizeof(_compiler_),
                                                    "%s %s -o%s %s %s -i%s",
                                                    wcfg.wd_sef_found_list[0],
                                                    wcfg.wd_toml_gm_input,
                                                    wcfg.wd_toml_gm_output,
                                                    wcfg.wd_toml_aio_opt,
                                                    include_aio_path,
                                                    path_include
                                             );

                    /* Start timing and execute compiler */
                    clock_gettime(CLOCK_MONOTONIC, &start);
                    if (ret_cmd > 0 && ret_cmd < (int)sizeof(_compiler_)) {
                        BOOL success = CreateProcessA(
                            NULL,            // No module name
                            _compiler_,        // Command line
                            NULL,            // Process handle not inheritable
                            NULL,            // Thread handle not inheritable
                            TRUE,            // Set handle inheritance to TRUE
                            CREATE_NO_WINDOW,// Creation flags
                            NULL,            // Use parent's environment block
                            NULL,            // Use parent's starting directory
                            &si,             // Pointer to STARTUPINFO structure
                            &pi              // Pointer to PROCESS_INFORMATION structure
                        );
                        if (success) {
                            /* Optional: Set I/O priority for better performance */
                            HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
                            if (hKernel32) {
                                typedef BOOL (WINAPI *PSETPROCESSIOPRIORITY)(HANDLE, INT);
                                PSETPROCESSIOPRIORITY SetProcessIoPriority = 
                                    (PSETPROCESSIOPRIORITY)GetProcAddress(hKernel32, "SetProcessIoPriority");
                                
                                if (SetProcessIoPriority) {
                                    SetProcessIoPriority(pi.hProcess, 1);
                                }
                            }

                            clock_gettime(CLOCK_MONOTONIC, &end);
                            WaitForSingleObject(pi.hProcess, INFINITE);

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
                    pid_t pid = fork();
                    if (pid == 0) {
                        /* Child process: redirect output to log file */
                        int fd = open(".wd_compiler.log", O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
                        if (fd != -1) {
                            dup2(fd, STDOUT_FILENO);
                            dup2(fd, STDERR_FILENO);
                            close(fd);
                        }
                        
                        /* Prepare arguments for execvp */
                        char *args[] = {
                            wcfg.wd_sef_found_list[0],
                            wcfg.wd_toml_gm_input,
                            "-o",
                            wcfg.wd_toml_gm_output,
                            wcfg.wd_toml_aio_opt,
                            include_aio_path,
                            "-i",
                            path_include,
                            NULL
                       };

                        /* Build command string for debugging */
                        for (int i = 0; args[i] != NULL; i++) {
                            strcat(_compiler_, args[i]);
                            strcat(_compiler_, " ");
                        }
                        
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        execvp(args[0], args);  /* Execute compiler */
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        exit(127);  /* Exit if exec fails */
                    } else if (pid > 0) {
                        /* Parent process: wait for compilation to complete */
                        int status;
                        const int TIMEOUT = 30;  /* 30 second timeout */
                        for (int i = 0; i < TIMEOUT; i++) {
                            int result = waitpid(pid, &status, WNOHANG);
                            if (result == 0) {
                                sleep(1);  /* Still running, wait */
                            } else if (result == pid) {
                                break;  /* Process completed */
                            } else {
                                pr_error(stdout, "waitpid error");
                                break;
                            }
                            
                            /* Handle timeout by killing the process */
                            if (i == TIMEOUT - 1) {
                                kill(pid, SIGTERM);
                                sleep(2);
                                kill(pid, SIGKILL);
                                pr_error(stdout, "Process execution timeout (%d seconds)", TIMEOUT);
                                waitpid(pid, &status, 0);
                            }
                        }
                        
                        /* Process completion status */
                        if (WIFEXITED(status)) {
                            int exit_code = WEXITSTATUS(status);
                            if (exit_code != 0) {
                                pr_error(stdout, "Compiler exited with code %d", exit_code);
                            }
                        } else if (WIFSIGNALED(status)) {
                            pr_error(stdout, "Compiler terminated by signal %d", WTERMSIG(status));
                        }
                    } else {
                        pr_error(stdout, "fork() failed");
                    }
#endif
                    /* Prepare output file path for post-processing */
                    char _container_output[WD_PATH_MAX + 128];
                    wd_snprintf(_container_output, sizeof(_container_output), "%s", wcfg.wd_toml_gm_output);

                    /* Process compiler output based on feature flags */
                    if (path_exists(".wd_compiler.log")) {
                        printf("\n");
                        if (compiler_has_watchdogs && compiler_has_clean) {
                            /* Enhanced analysis with cleanup */
                            cause_compiler_expl(".wd_compiler.log", _container_output, compiler_debugging);
                            if (path_exists(_container_output)) {
                                    remove(_container_output);  /* Clean output file */
                            }
                            goto compiler_done;
                        } else if (compiler_has_watchdogs) {
                            /* Enhanced analysis without cleanup */
                            cause_compiler_expl(".wd_compiler.log", _container_output, compiler_debugging);
                            goto compiler_done;
                        } else if (compiler_has_clean) {
                            /* Simple output with cleanup */
                            wd_printfile(".wd_compiler.log");
                            if (path_exists(_container_output)) {
                                    remove(_container_output);  /* Clean output file */
                            }
                            goto compiler_done;
                        }

                        /* Default: simple log output */
                        wd_printfile(".wd_compiler.log");
                    }

/* Label for compiler completion handling */
compiler_done:
                    /* Check for compilation errors in log file */
                    procc_f = fopen(".wd_compiler.log", "r");
                    if (procc_f)
                    {
                        char line_buf[526];
                        int compiler_has_err = 0;
                        while (fgets(line_buf, sizeof(line_buf), procc_f))
                        {
                            if (strstr(line_buf, "error")) {
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

                    pr_color(stdout, FCOLOUR_CYAN,
                        " <T> [P]Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);

                    /* Debug output if enabled */
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
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
                    int find_compile_args = 0;
                    find_compile_args = wd_sef_fdir(__wcp_direct_path, __wcp_file_name, NULL);

                    /* Fallback search in gamemodes directory if not found */
                    if (!find_compile_args &&
                        strcmp(__wcp_direct_path, "gamemodes") != 0)
                    {
                        find_compile_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (find_compile_args)
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
                    if (!find_compile_args && !strcmp(__wcp_direct_path, "."))
                    {
                        find_compile_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (find_compile_args)
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

                    /* Proceed with compilation if file was found */
                    if (find_compile_args)
                    {
                        /* Prepare output filename by removing extension */
                        char __sef_path_sz[WD_PATH_MAX];
                        wd_snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.wd_sef_found_list[1]);
                        char *f_EXT = strrchr(__sef_path_sz, '.');
                        if (f_EXT)
                            *f_EXT = '\0';

                        wd_snprintf(container_output, sizeof(container_output), "%s", __sef_path_sz);

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

                        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                        si.wShowWindow = SW_HIDE;

                        if (hFile != INVALID_HANDLE_VALUE) {
                            si.hStdOutput = hFile;
                            si.hStdError = hFile;
                        }

                        /* Build compiler command for specific file */
                        int ret_cmd = wd_snprintf(
                                                    _compiler_,
                                                    sizeof(_compiler_),
                                                        "%s %s -o%s.amx %s %s -i%s",
                                                        wcfg.wd_sef_found_list[0],
                                                        wcfg.wd_sef_found_list[1],
                                                        container_output,
                                                        wcfg.wd_toml_aio_opt,
                                                        include_aio_path,
                                                        path_include
                                                 );

                        clock_gettime(CLOCK_MONOTONIC, &start);
                        if (ret_cmd > 0 && ret_cmd < (int)sizeof(_compiler_)) {
                            BOOL success = CreateProcessA(
                                NULL,            // No module name
                                _compiler_,        // Command line
                                NULL,            // Process handle not inheritable
                                NULL,            // Thread handle not inheritable
                                TRUE,            // Set handle inheritance to TRUE
                                CREATE_NO_WINDOW,// Creation flags
                                NULL,            // Use parent's environment block
                                NULL,            // Use parent's starting directory
                                &si,             // Pointer to STARTUPINFO structure
                                &pi              // Pointer to PROCESS_INFORMATION structure
                            );
                            if (success) {
                                HMODULE hKernel32 = GetModuleHandle("kernel32.dll");
                                if (hKernel32) {
                                    typedef BOOL (WINAPI *PSETPROCESSIOPRIORITY)(HANDLE, INT);
                                    PSETPROCESSIOPRIORITY SetProcessIoPriority = 
                                        (PSETPROCESSIOPRIORITY)GetProcAddress(hKernel32, "SetProcessIoPriority");
                                    
                                    if (SetProcessIoPriority) {
                                        SetProcessIoPriority(pi.hProcess, 1);
                                    }
                                }

                                clock_gettime(CLOCK_MONOTONIC, &end);
                                WaitForSingleObject(pi.hProcess, INFINITE);

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
                        /* Unix/Linux execution (similar to above) */
                        pid_t pid = fork();
                        if (pid == 0) {
                            int fd = open(".wd_compiler.log", O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
                            if (fd != -1) {
                                dup2(fd, STDOUT_FILENO);
                                dup2(fd, STDERR_FILENO);
                                close(fd);
                            }
                            
                            char *args[] = {
                                wcfg.wd_sef_found_list[0],
                                wcfg.wd_sef_found_list[1],
                                "-o",
                                container_output,
                                wcfg.wd_toml_aio_opt,
                                include_aio_path,
                                "-i",
                                path_include,
                                NULL
                            };

                            for (int i = 0; args[i] != NULL; i++) {
                                strcat(_compiler_, args[i]);
                                strcat(_compiler_, " ");
                            }
                            
                            clock_gettime(CLOCK_MONOTONIC, &start);
                            execvp(args[0], args);
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            exit(127);
                        } else if (pid > 0) {
                            int status;
                            const int TIMEOUT = 30;
                            for (int i = 0; i < TIMEOUT; i++) {
                                int result = waitpid(pid, &status, WNOHANG);
                                if (result == 0) {
                                    sleep(1);
                                } else if (result == pid) {
                                    break;
                                } else {
                                    pr_error(stdout, "waitpid error");
                                    break;
                                }
                                
                                if (i == TIMEOUT - 1) {
                                    kill(pid, SIGTERM);
                                    sleep(2);
                                    kill(pid, SIGKILL);
                                    pr_error(stdout, "Process execution timeout (%d seconds)", TIMEOUT);
                                    waitpid(pid, &status, 0);
                                }
                            }
                            
                            if (WIFEXITED(status)) {
                                int exit_code = WEXITSTATUS(status);
                                if (exit_code != 0) {
                                    pr_error(stdout, "Compiler exited with code %d", exit_code);
                                }
                            } else if (WIFSIGNALED(status)) {
                                pr_error(stdout, "Compiler terminated by signal %d", WTERMSIG(status));
                            }
                        } else {
                            pr_error(stdout, "fork() failed");
                        }
#endif
                        /* Post-compilation processing (similar to Branch 1) */
                        char _container_output[WD_PATH_MAX + 128];
                        wd_snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);

                        if (path_exists(".wd_compiler.log")) {
                            printf("\n");
                            if (compiler_has_watchdogs && compiler_has_clean) {
                                cause_compiler_expl(".wd_compiler.log", _container_output, compiler_debugging);
                                if (path_exists(_container_output)) {
                                        remove(_container_output);
                                }
                                goto compiler_done2;
                            } else if (compiler_has_watchdogs) {
                                cause_compiler_expl(".wd_compiler.log", _container_output, compiler_debugging);
                                goto compiler_done2;
                            } else if (compiler_has_clean) {
                                wd_printfile(".wd_compiler.log");
                                if (path_exists(_container_output)) {
                                        remove(_container_output);
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
                            char line_buf[526];
                            int compiler_has_err = 0;
                            while (fgets(line_buf, sizeof(line_buf), procc_f))
                            {
                                if (strstr(line_buf, "error")) {
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

                        pr_color(stdout, FCOLOUR_CYAN,
                            " <T> [P]Finished at %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
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