#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

#include "wd_extra.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_hardware.h"
#include "wd_compiler.h"

#ifndef WD_WINDOWS
extern char **environ;
#define POSIX_TIMEOUT 30
#endif

static char
            container_output[WD_PATH_MAX] = { 0 },
            compiler_direct_path[WD_PATH_MAX] = { 0 },
            compiler_size_file_name[WD_PATH_MAX] = { 0 },
            compiler_size_input_path[WD_PATH_MAX] = { 0 },
            compiler_size_temp[WD_PATH_MAX] = { 0 };

int wd_run_compiler(const char *arg, const char *compile_args,
                    const char *second_arg, const char *four_arg,
                    const char *five_arg, const char *six_arg,
                    const char *seven_arg, const char *eight_arg,
                    const char *nine_arg)
{
        const int aio_extra_options = 7;
#if defined (_DBG_PRINT)
        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
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
        wcfg.wd_compiler_stat = 0;

        FILE *proc_file;
        char size_log[WD_MAX_PATH * 4];
        char run_cmd[WD_PATH_MAX + 258];
        char include_aio_path[WD_PATH_MAX * 2] = { 0 };
        char _compiler_input_[WD_MAX_PATH + WD_PATH_MAX] = { 0 };

        char *compiler_last_slash = NULL;
        char *compiler_back_slash = NULL;

        int compiler_debugging = 0;
        int compiler_has_watchdogs = 0, compiler_has_debug = 0;
        int compiler_has_clean = 0, compiler_has_assembler = 0;
        int compiler_has_recursion = 0, compiler_has_verbose = 0;
        int compiler_has_encoding = 0;

        const char* compiler_args[] = {
                                            second_arg, four_arg,
                                            five_arg, six_arg,
                                            seven_arg, eight_arg,
                                            nine_arg
                                      };

        const char *ptr_pawncc = NULL;
        if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS))
            ptr_pawncc = "pawncc.exe";
        else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX))
            ptr_pawncc = "pawncc";

        char *path_include = NULL;
        if (wd_server_env() == 1)
            path_include="pawno/include";
        else if (wd_server_env() == 2)
            path_include="qawno/include";

        int _wd_log_acces = path_access(".wd_compiler.log");
        if (_wd_log_acces)
            remove(".wd_compiler.log");

        FILE *wd_log_files = fopen(".wd_compiler.log", "w");
        if (wd_log_files != NULL)
            fclose(wd_log_files);

        compiler_memory_clean();

        int __find_pawncc = wd_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc)
        {
            FILE *proc_f;
            proc_f = fopen("watchdogs.toml", "r");
            if (!proc_f)
            {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                goto compiler_end;
            }

            char error_buffer[256];
            toml_table_t *_toml_config;
            _toml_config = toml_parse_file(proc_f,
                       error_buffer,
                       sizeof(error_buffer));

            if (proc_f) {
                fclose(proc_f);
            }

            if (!_toml_config)
            {
                pr_error(stdout, "parsing TOML: %s", error_buffer);
                goto compiler_end;
            }

            char wd_compiler_input_pawncc_path[WD_PATH_MAX],
                 wd_compiler_input_gamemode_path[WD_PATH_MAX];
            wd_snprintf(wd_compiler_input_pawncc_path,
                    sizeof(wd_compiler_input_pawncc_path), "%s", wcfg.wd_sef_found_list[0]);

            wd_snprintf(run_cmd, sizeof(run_cmd),
                "%s > .compiler_test.log",
                wd_compiler_input_pawncc_path);
            wd_run_command(run_cmd);

            proc_file = fopen(".compiler_test.log", "r");
            if (!proc_file) {
                pr_error(stdout, "Failed to open .compiler_test.log");
            }

            for (int i = 0; i < aio_extra_options; ++i) {
                if (compiler_args[i] != NULL) {
                    if (strfind(compiler_args[i], "--watchdogs")) ++compiler_has_watchdogs;
                    if (strfind(compiler_args[i], "--debug")) ++compiler_has_debug;
                    if (strfind(compiler_args[i], "--clean")) ++compiler_has_clean;
                    if (strfind(compiler_args[i], "--assembler")) ++compiler_has_assembler;
                    if (strfind(compiler_args[i], "--recursion")) ++compiler_has_recursion;
                    if (strfind(compiler_args[i], "--verbose")) ++compiler_has_verbose;
                    if (strfind(compiler_args[i], "--encoding")) ++compiler_has_encoding;
                }
            }

            toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
#if defined(_DBG_PRINT)
            if (!wd_compiler)
                printf("%s not exists in line:%d", "compiler", __LINE__);
#endif
            if (wd_compiler)
            {
                toml_array_t *option_arr = toml_array_in(wd_compiler, "option");
#if defined(_DBG_PRINT)
                if (!option_arr)
                    printf("%s not exists in line:%d", "option", __LINE__);
#endif
                if (option_arr)
                {
                    char *merged = NULL;
                    size_t toml_array_size;
                    toml_array_size = toml_array_nelem(option_arr);

                    for (size_t i = 0; i < toml_array_size; i++)
                    {
                        toml_datum_t toml_option_value;
                        toml_option_value = toml_string_at(option_arr, i);
                        if (!toml_option_value.ok)
                            continue;

                        int rate_valid_flag_options = 0;

                        char flag_to_search[3] = { 0 };
                        size_t size_flag_to_search = sizeof(flag_to_search);
                        if (strlen(toml_option_value.u.s) >= 2) {
                            wd_snprintf(flag_to_search,
                                     size_flag_to_search,
                                     "%.2s",
                                     toml_option_value.u.s);
                        } else {
                            wd_strncpy(flag_to_search, toml_option_value.u.s, size_flag_to_search - 1);
                        }

                        if (proc_file != NULL) {
                            rewind(proc_file);
                            while (fgets(size_log, sizeof(size_log), proc_file) != NULL) {
                                if (strfind(size_log, "error while loading shared libraries:")) {
                                    wd_printfile(".compiler_test.log");
                                    goto compiler_end;
                                }
                                if (strstr(size_log, flag_to_search)) {
                                    rate_valid_flag_options = 1;
                                    break;
                                }
                            }
                        }

                        if (rate_valid_flag_options == 0)
                            goto not_valid_flag_options;

                        if (toml_option_value.u.s[0] != '-') {
not_valid_flag_options:
                            printf("[WARN]: "
                                "compiler option ");
                            pr_color(stdout, FCOLOUR_GREEN, "\"%s\" ", toml_option_value.u.s);
                            println(stdout, "not valid flag options!..");

                            if (rate_valid_flag_options == 0)
                              goto compiler_end;
                        }

                        if (strfind(toml_option_value.u.s, "-d"))
                          ++compiler_debugging;

                        size_t old_len = merged  ? strlen(merged) : 0,
                               new_len = old_len + strlen(toml_option_value.u.s) + 2;

                        char *tmp = wd_realloc(merged, new_len);
                        if (!tmp) {
                            wd_free(merged);
                            wd_free(toml_option_value.u.s);
                            merged = NULL;
                            break;
                        }
                        merged = tmp;

                        if (!old_len)
                            wd_snprintf(merged, new_len, "%s", toml_option_value.u.s);
                        else
                            wd_snprintf(merged + old_len,
                                     new_len - old_len,
                                     " %s", toml_option_value.u.s);

                        wd_free(toml_option_value.u.s);
                        toml_option_value.u.s = NULL;
                    }

                    if (proc_file) {
                        fclose(proc_file);
                        if (path_access(".compiler_test.log"))
                            remove(".compiler_test.log");
                    }

                    if (merged) {
                        wcfg.wd_toml_aio_opt = merged;
                    }
                    else {
                        wcfg.wd_toml_aio_opt = strdup("");
                    }
                }

                char compiler_extra_options[WD_PATH_MAX] = "";
                if (compiler_has_debug && compiler_debugging) {
                    const char *debug_mmv[] = {"-d1", "-d3"};
                    size_t n_mem_target = sizeof(debug_mmv) / sizeof(debug_mmv[0]);
                    size_t i;
                    for (i = 0; i < n_mem_target; i++) {
                        const char
                            *debug_opt = debug_mmv[i];
                        size_t size_debug_option = strlen(debug_opt);
                        char *fetch_pos;
                        while ((fetch_pos = strstr(wcfg.wd_toml_aio_opt, debug_opt)) != NULL) {
                            memmove(fetch_pos,
                                    fetch_pos + size_debug_option,
                                    strlen(fetch_pos + size_debug_option) + 1);
                        }
                    }
                }

                if (compiler_has_debug) strcat(compiler_extra_options, " -d2 ");
                if (compiler_has_assembler) strcat(compiler_extra_options, " -a ");
                if (compiler_has_recursion) strcat(compiler_extra_options, " -R+ ");
                if (compiler_has_verbose) strcat(compiler_extra_options, " -v2 ");
                if (compiler_has_encoding) strcat(compiler_extra_options, " -C+ ");

                if (strlen(compiler_extra_options) > 0) {
                    size_t current_len, extra_opt_len;
                    current_len = strlen(wcfg.wd_toml_aio_opt);
                    extra_opt_len = strlen(compiler_extra_options);

                    if (current_len + extra_opt_len < sizeof(wcfg.wd_toml_aio_opt)) {
                        strcat(wcfg.wd_toml_aio_opt, compiler_extra_options);
                    } else {
                        strncat(wcfg.wd_toml_aio_opt, compiler_extra_options,
                                sizeof(wcfg.wd_toml_aio_opt) - current_len - 1);
                    }
                }

                toml_array_t *toml_include_path = toml_array_in(wd_compiler, "include_path");
#if defined(_DBG_PRINT)
                if (!toml_include_path)
                    printf("%s not exists in line:%d", "include_path", __LINE__);
#endif
                if (toml_include_path)
                {
                    int toml_array_size;
                    toml_array_size = toml_array_nelem(toml_include_path);

                    for (int i = 0; i < toml_array_size; i++)
                    {
                        toml_datum_t path_val = toml_string_at(toml_include_path, i);
                        if (path_val.ok)
                        {
                            char __procc[WD_PATH_MAX + 26];
                            wd_strip_dot_fns(__procc, sizeof(__procc), path_val.u.s);
                            if (__procc[0] == '\0')
                                continue;

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

                if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0'))
                {
                    struct timespec start = {0}, end = { 0 };
                    double compiler_dur;

#ifdef WD_WINDOWS
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
                            FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_SEQUENTIAL_SCAN,
                            NULL
                    );
                    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;

                    if (hFile != INVALID_HANDLE_VALUE) {
                        si.hStdOutput = hFile;
                        si.hStdError = hFile;
                    }

                    int ret_command = 0;
                    ret_command = wd_snprintf(_compiler_input_,
                              sizeof(_compiler_input_),
                                    "%s %s -o%s %s %s -i%s",
                                    wd_compiler_input_pawncc_path,
                                    wcfg.wd_toml_gm_input,
                                    wcfg.wd_toml_gm_output,
                                    wcfg.wd_toml_aio_opt,
                                    include_aio_path,
                                    path_include);
#if defined(_DBG_PRINT)
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                    if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                        BOOL win32_process_succes;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        win32_process_succes = CreateProcessA(
                                NULL,
                                _compiler_input_,
                                NULL,
                                NULL,
                                TRUE,
                                CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,
                                NULL,
                                NULL,
                                &si,
                                &pi);
                        if (win32_process_succes) {
                            WaitForSingleObject(pi.hProcess, INFINITE);

                            clock_gettime(CLOCK_MONOTONIC, &end);

                            DWORD proc_exit_code;
                            GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                        }
                    } else {
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        pr_error(stdout, "ret_compiler too long! %s (L: %d)", __func__, __LINE__);
                        goto compiler_end;
                    }
                    if (hFile != INVALID_HANDLE_VALUE) {
                        CloseHandle(hFile);
                    }
#else
                    int ret_command = 0;
                    ret_command = wd_snprintf(_compiler_input_, sizeof(_compiler_input_),
                        "%s %s %s%s %s%s %s%s",
                        wd_compiler_input_pawncc_path,
                        wcfg.wd_toml_gm_input,
                        "-o",
                        wcfg.wd_toml_gm_output,
                        wcfg.wd_toml_aio_opt,
                        include_aio_path,
                        "-i",
                        path_include);
                    if (ret_command > (int)sizeof(_compiler_input_)) {
                        pr_error(stdout, "ret_compiler too long! %s (L: %d)", __func__, __LINE__);
                        goto compiler_end;
                    }
#if defined(_DBG_PRINT)
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                    int i = 0;
                    char *wd_compiler_unix_args[WD_MAX_PATH + 256];
                    char *compiler_unix_token = strtok(_compiler_input_, " ");
                    while (compiler_unix_token != NULL) {
                        wd_compiler_unix_args[i++] = compiler_unix_token;
                        compiler_unix_token = strtok(NULL, " ");
                    }
                    wd_compiler_unix_args[i] = NULL;

                    posix_spawn_file_actions_t process_file_actions;
                    posix_spawn_file_actions_init(&process_file_actions);
                    int posix_logging_file = open(".wd_compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (posix_logging_file != -1) {
                        posix_spawn_file_actions_adddup2(&process_file_actions,
                                posix_logging_file,
                                STDOUT_FILENO);
                        posix_spawn_file_actions_adddup2(&process_file_actions,
                                posix_logging_file,
                                STDERR_FILENO);
                        posix_spawn_file_actions_addclose(&process_file_actions,
                                posix_logging_file);
                    }

                    posix_spawnattr_t spawn_attr;
                    posix_spawnattr_init(&spawn_attr);

                    posix_spawnattr_setflags(&spawn_attr,
                        POSIX_SPAWN_SETSIGDEF |
                        POSIX_SPAWN_SETSIGMASK
                    );

                    pid_t compiler_process_id;

                    int process_spawn_result = posix_spawnp(&compiler_process_id,
                                wd_compiler_unix_args[0],
                                &process_file_actions,
                                &spawn_attr,
                                wd_compiler_unix_args,
                                environ);

                    posix_spawnattr_destroy(&spawn_attr);
                    posix_spawn_file_actions_destroy(&process_file_actions);

                    if (process_spawn_result == 0) {
                        int process_status;
                        int process_timeout_occurred = 0;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        for (int i = 0; i < POSIX_TIMEOUT; i++) {
                            int p_result = -1;
                            p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                            if (p_result == 0)
                                usleep(0xC350);
                            else if (p_result == compiler_process_id) {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                break;
                            }
                            else {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                pr_error(stdout, "waitpid error");
                                break;
                            }
                            if (i == POSIX_TIMEOUT - 1) {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                kill(compiler_process_id, SIGTERM);
                                sleep(2);
                                kill(compiler_process_id, SIGKILL);
                                pr_error(stdout,
                                         "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                waitpid(compiler_process_id, &process_status, 0);
                                process_timeout_occurred = 1;
                            }
                        }
                        if (!process_timeout_occurred) {
                            if (WIFEXITED(process_status)) {
                                int proc_exit_code = 0;
                                proc_exit_code = WEXITSTATUS(process_status);
                                if (proc_exit_code != 0)
                                    pr_error(stdout,
                                             "watchdogs compiler exited with code (%d)", proc_exit_code);
                            } else if (WIFSIGNALED(process_status)) {
                                pr_error(stdout,
                                         "watchdogs compiler terminated by signal (%d)", WTERMSIG(process_status));
                            }
                        }
                    } else {
                        pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result));
                    }
#endif
                    char size_container_output[WD_PATH_MAX + 128];
                    wd_snprintf(size_container_output,
                        sizeof(size_container_output), "%s", wcfg.wd_toml_gm_output);
                    if (path_exists(".wd_compiler.log")) {
                        printf("\n");
                        char *ca = NULL;
                        ca = size_container_output;
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

                        char log_line[WD_MAX_PATH * 4];
                        proc_file = fopen(".wd_compiler.log", "r");

                        if (proc_file != NULL) {
                            while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
                                if (strfind(log_line, "backtrace"))
                                    pr_color(stdout, FCOLOUR_CYAN,
                                        "~ backtrace detected - "
                                        "make sure you are using a newer version of pawncc than the one currently in use.");
                            }
                            fclose(proc_file);
                        }
                    }
compiler_done:
                    proc_f = fopen(".wd_compiler.log", "r");
                    if (proc_f)
                    {
                        char compiler_line_buffer[WD_PATH_MAX];
                        int compiler_has_err = 0;
                        while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), proc_f))
                        {
                            if (strstr(compiler_line_buffer, "error")) {
                                compiler_has_err = 1;
                                break;
                            }
                        }
                        fclose(proc_f);
                        if (compiler_has_err)
                        {
                            if (size_container_output[0] != '\0' && path_access(size_container_output))
                                remove(size_container_output);
                            wcfg.wd_compiler_stat = 1;
                        }
                    }
                    else {
                        pr_error(stdout, "Failed to open .wd_compiler.log");
                    }

                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
                    if (compiler_dur > 0x64) {
                        printf("~ It appears to be taking quite some time.\n"
                               "  Ensure that all existing warnings have been resolved,\n"
                               "  use the latest compiler,\n"
                               "  and verify that the logic"
                               "  and pawn algorithm optimizations within the gamemode scripts are consistent.\n");
                        fflush(stdout);
                    }
                }
                else
                {
                    wd_strncpy(compiler_size_temp, compile_args, sizeof(compiler_size_temp) - 1);
                    compiler_size_temp[sizeof(compiler_size_temp) - 1] = '\0';

                    compiler_last_slash = strrchr(compiler_size_temp, '/');
                    compiler_back_slash = strrchr(compiler_size_temp, '\\');

                    if (compiler_back_slash && (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                        compiler_last_slash = compiler_back_slash;

                    if (compiler_last_slash)
                    {
                        size_t compiler_dir_len;
                        compiler_dir_len = (size_t)(compiler_last_slash - compiler_size_temp);

                        if (compiler_dir_len >= sizeof(compiler_direct_path))
                            compiler_dir_len = sizeof(compiler_direct_path) - 1;

                        memcpy(compiler_direct_path, compiler_size_temp, compiler_dir_len);
                        compiler_direct_path[compiler_dir_len] = '\0';

                        const char *compiler_filename_start = compiler_last_slash + 1;
                        size_t compiler_filename_len;
                        compiler_filename_len = strlen(compiler_filename_start);

                        if (compiler_filename_len >= sizeof(compiler_size_file_name))
                            compiler_filename_len = sizeof(compiler_size_file_name) - 1;

                        memcpy(compiler_size_file_name, compiler_filename_start, compiler_filename_len);
                        compiler_size_file_name[compiler_filename_len] = '\0';

                        size_t total_needed;
                        total_needed = strlen(compiler_direct_path) + 1 +
                                    strlen(compiler_size_file_name) + 1;

                        if (total_needed > sizeof(compiler_size_input_path))
                        {
                            wd_strncpy(compiler_direct_path, "gamemodes",
                                sizeof(compiler_direct_path) - 1);
                            compiler_direct_path[sizeof(compiler_direct_path) - 1] = '\0';

                            size_t compiler_max_size_file_name;
                            compiler_max_size_file_name = sizeof(compiler_size_file_name) - 1;

                            if (compiler_filename_len > compiler_max_size_file_name)
                            {
                                memcpy(compiler_size_file_name, compiler_filename_start,
                                    compiler_max_size_file_name);
                                compiler_size_file_name[compiler_max_size_file_name] = '\0';
                            }
                        }

                        if (wd_snprintf(compiler_size_input_path, sizeof(compiler_size_input_path),
                                "%s/%s", compiler_direct_path, compiler_size_file_name) >=
                            (int)sizeof(compiler_size_input_path))
                        {
                            compiler_size_input_path[sizeof(compiler_size_input_path) - 1] = '\0';
                        }
                     }  else {
                            wd_strncpy(compiler_size_file_name, compiler_size_temp,
                                sizeof(compiler_size_file_name) - 1);
                            compiler_size_file_name[sizeof(compiler_size_file_name) - 1] = '\0';

                            wd_strncpy(compiler_direct_path, ".", sizeof(compiler_direct_path) - 1);
                            compiler_direct_path[sizeof(compiler_direct_path) - 1] = '\0';

                            if (wd_snprintf(compiler_size_input_path, sizeof(compiler_size_input_path),
                                    "./%s", compiler_size_file_name) >=
                                (int)sizeof(compiler_size_input_path))
                            {
                                compiler_size_input_path[sizeof(compiler_size_input_path) - 1] = '\0';
                            }
                        }

                    int compiler_finding_compile_args = 0;
                    compiler_finding_compile_args = wd_sef_fdir(compiler_direct_path, compiler_size_file_name, NULL);

                    if (!compiler_finding_compile_args &&
                        strcmp(compiler_direct_path, "gamemodes") != 0)
                    {
                        compiler_finding_compile_args = wd_sef_fdir("gamemodes",
                                            compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            wd_strncpy(compiler_direct_path, "gamemodes",
                                sizeof(compiler_direct_path) - 1);
                            compiler_direct_path[sizeof(compiler_direct_path) - 1] = '\0';

                            if (wd_snprintf(compiler_size_input_path, sizeof(compiler_size_input_path),
                                    "gamemodes/%s", compiler_size_file_name) >=
                                (int)sizeof(compiler_size_input_path))
                            {
                                compiler_size_input_path[sizeof(compiler_size_input_path) - 1] = '\0';
                            }

                            if (wcfg.wd_sef_count > RATE_SEF_EMPTY)
                                wd_strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    if (!compiler_finding_compile_args && !strcmp(compiler_direct_path, "."))
                    {
                        compiler_finding_compile_args = wd_sef_fdir("gamemodes",
                                            compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            wd_strncpy(compiler_direct_path, "gamemodes",
                                sizeof(compiler_direct_path) - 1);
                            compiler_direct_path[sizeof(compiler_direct_path) - 1] = '\0';

                            if (wd_snprintf(compiler_size_input_path, sizeof(compiler_size_input_path),
                                    "gamemodes/%s", compiler_size_file_name) >=
                                (int)sizeof(compiler_size_input_path))
                            {
                                compiler_size_input_path[sizeof(compiler_size_input_path) - 1] = '\0';
                            }

                            if (wcfg.wd_sef_count > RATE_SEF_EMPTY)
                                wd_strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    wd_snprintf(wd_compiler_input_gamemode_path,
                            sizeof(wd_compiler_input_gamemode_path), "%s", wcfg.wd_sef_found_list[1]);

                    if (compiler_finding_compile_args)
                    {
                        char __sef_path_sz[WD_PATH_MAX];
                        wd_snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wd_compiler_input_gamemode_path);
                        char *f_EXT = strrchr(__sef_path_sz, '.');
                        if (f_EXT)
                            *f_EXT = '\0';

                        wd_snprintf(container_output, sizeof(container_output), "%s", __sef_path_sz);

                        char size_container_output[WD_PATH_MAX + 128];
                        wd_snprintf(size_container_output, sizeof(size_container_output), "%s.amx", container_output);

                        struct timespec start = {0}, end = { 0 };
                        double compiler_dur;
#ifdef WD_WINDOWS
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
                                FILE_ATTRIBUTE_NORMAL |
                                    FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL
                        );

                        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                        si.wShowWindow = SW_HIDE;

                        if (hFile != INVALID_HANDLE_VALUE) {
                            si.hStdOutput = hFile;
                            si.hStdError = hFile;
                        }

                        int ret_command = 0;
                        ret_command = wd_snprintf(_compiler_input_,
                                  sizeof(_compiler_input_),
                                        "%s %s -o%s %s %s -i%s",
                                        wd_compiler_input_pawncc_path,
                                        wd_compiler_input_gamemode_path,
                                        size_container_output,
                                        wcfg.wd_toml_aio_opt,
                                        include_aio_path,
                                        path_include);
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                        if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                            BOOL win32_process_succes;
                            clock_gettime(CLOCK_MONOTONIC, &start);
                            win32_process_succes = CreateProcessA(
                                    NULL,
                                    _compiler_input_,
                                    NULL,
                                    NULL,
                                    TRUE,
                                    CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,
                                    NULL,
                                    NULL,
                                    &si,
                                    &pi);
                            if (win32_process_succes) {
                                WaitForSingleObject(pi.hProcess, INFINITE);

                                clock_gettime(CLOCK_MONOTONIC, &end);

                                DWORD proc_exit_code;
                                GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                                CloseHandle(pi.hProcess);
                                CloseHandle(pi.hThread);
                            } else {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                            }
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            pr_error(stdout, "ret_compiler too long! %s (L: %d)", __func__, __LINE__);
                            goto compiler_end;
                        }
                        if (hFile != INVALID_HANDLE_VALUE) {
                            CloseHandle(hFile);
                        }
#else
                        int ret_command = 0;
                        ret_command = wd_snprintf(_compiler_input_, sizeof(_compiler_input_),
                            "%s %s %s%s %s%s %s%s",
                            wd_compiler_input_pawncc_path,
                            wd_compiler_input_gamemode_path,
                            "-o",
                            size_container_output,
                            wcfg.wd_toml_aio_opt,
                            include_aio_path,
                            "-i",
                            path_include);
                        if (ret_command > (int)sizeof(_compiler_input_)) {
                            pr_error(stdout, "ret_compiler too long! %s (L: %d)", __func__, __LINE__);
                            goto compiler_end;
                        }
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_input_);
#endif
                        int i = 0;
                        char *wd_compiler_unix_args[WD_MAX_PATH + 256];
                        char *compiler_unix_token = strtok(_compiler_input_, " ");
                        while (compiler_unix_token != NULL) {
                            wd_compiler_unix_args[i++] = compiler_unix_token;
                            compiler_unix_token = strtok(NULL, " ");
                        }
                        wd_compiler_unix_args[i] = NULL;

                        posix_spawn_file_actions_t process_file_actions;
                        posix_spawn_file_actions_init(&process_file_actions);
                        int posix_logging_file = open(".wd_compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (posix_logging_file != -1) {
                            posix_spawn_file_actions_adddup2(&process_file_actions,
                                    posix_logging_file,
                                    STDOUT_FILENO);
                            posix_spawn_file_actions_adddup2(&process_file_actions,
                                    posix_logging_file,
                                    STDERR_FILENO);
                            posix_spawn_file_actions_addclose(&process_file_actions,
                                    posix_logging_file);
                        }

                        posix_spawnattr_t spawn_attr;
                        posix_spawnattr_init(&spawn_attr);

                        posix_spawnattr_setflags(&spawn_attr,
                                POSIX_SPAWN_SETSIGDEF |
                                POSIX_SPAWN_SETSIGMASK
                        );

                        pid_t compiler_process_id;

                        int process_spawn_result = posix_spawnp(&compiler_process_id,
                                    wd_compiler_unix_args[0],
                                    &process_file_actions,
                                    &spawn_attr,
                                    wd_compiler_unix_args,
                                    environ);

                        posix_spawnattr_destroy(&spawn_attr);
                        posix_spawn_file_actions_destroy(&process_file_actions);

                        if (process_spawn_result == 0) {
                            int process_status;
                            int process_timeout_occurred = 0;
                            clock_gettime(CLOCK_MONOTONIC, &start);
                            for (int i = 0; i < POSIX_TIMEOUT; i++) {
                                int p_result = -1;
                                p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                                if (p_result == 0)
                                    usleep(0xC350);
                                else if (p_result == compiler_process_id) {
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    break;
                                }
                                else {
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    pr_error(stdout, "waitpid error");
                                    break;
                                }
                                if (i == POSIX_TIMEOUT - 1) {
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    kill(compiler_process_id, SIGTERM);
                                    sleep(2);
                                    kill(compiler_process_id, SIGKILL);
                                    pr_error(stdout,
                                             "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                    waitpid(compiler_process_id, &process_status, 0);
                                    process_timeout_occurred = 1;
                                }
                            }
                            if (!process_timeout_occurred) {
                                if (WIFEXITED(process_status)) {
                                    int proc_exit_code = 0;
                                    proc_exit_code = WEXITSTATUS(process_status);
                                    if (proc_exit_code != 0)
                                        pr_error(stdout,
                                                 "watchdogs compiler exited with code (%d)", proc_exit_code);
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
                            ca = size_container_output;
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

                            char log_line[WD_MAX_PATH * 4];
                            proc_file = fopen(".wd_compiler.log", "r");

                            if (proc_file != NULL) {
                                while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
                                    if (strfind(log_line, "backtrace"))
                                        pr_color(stdout, FCOLOUR_CYAN,
                                            "~ backtrace detected - "
                                            "make sure you are using a newer version of pawncc than the one currently in use.\n");
                                }
                                fclose(proc_file);
                            }
                        }

compiler_done2:
                        proc_f = fopen(".wd_compiler.log", "r");
                        if (proc_f)
                        {
                            char compiler_line_buffer[WD_PATH_MAX];
                            int compiler_has_err = 0;
                            while (fgets(compiler_line_buffer, sizeof(compiler_line_buffer), proc_f))
                            {
                                if (strstr(compiler_line_buffer, "error")) {
                                    compiler_has_err = 1;
                                    break;
                                }
                            }
                            fclose(proc_f);
                            if (compiler_has_err)
                            {
                                if (size_container_output[0] != '\0' && path_access(size_container_output))
                                    remove(size_container_output);
                                wcfg.wd_compiler_stat = 1;
                            }
                        }
                        else {
                            pr_error(stdout, "Failed to open .wd_compiler.log");
                        }

                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        printf("\n");
                        pr_color(stdout, FCOLOUR_CYAN,
                            " <P> Finished at %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
                        if (compiler_dur > 0x64) {
                            printf("~ It appears to be taking quite some time.\n"
                                   "  Ensure that all existing warnings have been resolved,\n"
                                   "  use the latest compiler,\n"
                                   "  and verify that the logic"
                                   "  and pawn algorithm optimizations within the gamemode scripts are consistent.\n");
                            fflush(stdout);
                        }
                    }
                    else
                    {
                        printf("Cannot locate input: ");
                        pr_color(stdout, FCOLOUR_CYAN, "%s\n", compile_args);
                        goto compiler_end;
                    }
                }

                toml_free(_toml_config);

                goto compiler_end;
            }
        } else {
            pr_crit(stdout, "pawncc (our compiler) not found!");

            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");

            while (true)
            {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0)
                {
                    wd_free(ptr_sigA);
ret_ptr:
                    println(stdout, "Select platform:");
                    println(stdout, "-> [L/l] Linux");
                    println(stdout, "-> [W/w] Windows");
                    pr_color(stdout, FCOLOUR_YELLOW, " ^ work's in WSL/MSYS2\n");
                    println(stdout, "-> [T/t] Termux");

                    wcfg.wd_sel_stat = 1;

                    char *platform = readline("==> ");

                    if (strfind(platform, "L"))
                    {
                        int ret = wd_install_pawncc("linux");
loop_ipcc:
                        wd_free(platform);
                        if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc;
                    }
                    if (strfind(platform, "W"))
                    {
                        int ret = wd_install_pawncc("windows");
loop_ipcc2:
                        wd_free(platform);
                        if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc2;
                    }
                    if (strfind(platform, "T"))
                    {
                        int ret = wd_install_pawncc("termux");
loop_ipcc3:
                        wd_free(platform);
                        if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc3;
                    }
                    if (strfind(platform, "E")) {
                        wd_free(platform);
                        goto done;
                    }
                    else {
                        pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                        wd_free(platform);
                        goto ret_ptr;
                    }
done:
                    start_chain(NULL);
                }
                else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0)
                {
                    wd_free(ptr_sigA);
                    break;
                }
                else
                {
                    pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                    wd_free(ptr_sigA);
                    goto ret_ptr;
                }
            }
        }

compiler_end:
        return WD_RETN;
}