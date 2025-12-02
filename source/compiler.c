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
#include "package.h"
#include "lowlevel.h"
#include "compiler.h"

#ifndef WG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 30
#endif
static io_compilers wg_compiler_sys = {0};

int wg_run_compiler(const char *args, const char *compile_args,  const char *second_arg, const char *four_arg,
                    const char *five_arg, const char *six_arg, const char *seven_arg, const char *eight_arg,
                    const char *nine_arg)
{
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
        const int aio_extra_options = 7;
        wgconfig.wg_compiler_stat = 0;
        io_compilers comp;
        io_compilers *ptr_io = &comp;
#ifdef WG_WINDOWS
        PROCESS_INFORMATION pi;
        STARTUPINFO si = { sizeof(si) };
        SECURITY_ATTRIBUTES sa = { sizeof(sa) };
#endif
        struct timespec start = {0},
                        end = { 0 };
        double compiler_dur;

        const char* compiler_args[] = {
            second_arg, four_arg,
            five_arg, six_arg,
            seven_arg, eight_arg,
            nine_arg
        };

        FILE *proc_file;
        char size_log[WG_MAX_PATH * 4];
        char run_cmd[WG_PATH_MAX + 258];
        char include_aio_path[WG_PATH_MAX * 2] = { 0 };
        char _compiler_input_[WG_MAX_PATH + WG_PATH_MAX] = { 0 };

        char *compiler_last_slash = NULL;
        char *compiler_back_slash = NULL;

        char *fetch_string_pos = NULL;
        char compiler_extra_options[WG_PATH_MAX] = { 0 };
        const char *debug_memm[] = {"-d1", "-d3"};
        size_t size_debug_memm = sizeof(debug_memm);
        size_t size_debug_memm_zero = sizeof(debug_memm[0]);

        int compiler_debugging = 0;
        int compiler_has_watchdogs = 0, compiler_has_debug = 0;
        int compiler_has_clean = 0, compiler_has_assembler = 0;
        int compiler_has_recursion = 0, compiler_has_verbose = 0;
        int compiler_has_encoding = 0;

        const char *ptr_pawncc = NULL;
        if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_WINDOWS))
            ptr_pawncc = "pawncc.exe";
        else if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX))
            ptr_pawncc = "pawncc";

        char *path_include = NULL;
        if (wg_server_env() == 1)
            path_include="pawno/include";
        else if (wg_server_env() == 2)
            path_include="qawno/include";

        int _wg_log_acces = path_access(".watchdogs/compiler.log");
        if (_wg_log_acces)
            remove(".watchdogs/compiler.log");
        
        FILE *wg_log_files = fopen(".watchdogs/compiler.log", "w");
        if (wg_log_files != NULL)
            fclose(wg_log_files);
        else
            pr_error(stdout, "can't create .watchdogs/compiler.log!");

        compiler_memory_clean();

        int __find_pawncc = wg_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc)
        {
            FILE *proc_f;
            proc_f = fopen("watchdogs.toml", "r");
            if (!proc_f) {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                goto compiler_end;
            }

            char error_buffer[256];
            toml_table_t *wg_toml_config;
            wg_toml_config = toml_parse_file(proc_f,
                       error_buffer,
                       sizeof(error_buffer));

            if (proc_f) {
                fclose(proc_f);
            }

            if (!wg_toml_config) {
                pr_error(stdout, "parsing TOML: %s", error_buffer);
                goto compiler_end;
            }

            char wg_compiler_input_pawncc_path[WG_PATH_MAX],
                 wg_compiler_input_gamemode_path[WG_PATH_MAX];
            wg_snprintf(wg_compiler_input_pawncc_path,
                    sizeof(wg_compiler_input_pawncc_path), "%s", wgconfig.wg_sef_found_list[0]);

            wg_snprintf(run_cmd, sizeof(run_cmd),
                "%s > .watchdogs/compiler_test.log",
                wg_compiler_input_pawncc_path);
            wg_run_command(run_cmd);

            proc_file = fopen(".watchdogs/compiler_test.log", "r");
            if (!proc_file) {
                pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
            }

            for (int i = 0; i < aio_extra_options; ++i) {
                if (compiler_args[i] != NULL) {
                    if (strfind(compiler_args[i], "--detailed", true) ||
                        strfind(compiler_args[i], "--watchdogs", true))
                        ++compiler_has_watchdogs;
                    if (strfind(compiler_args[i], "--debug", true))
                        ++compiler_has_debug;
                    if (strfind(compiler_args[i], "--clean", true))
                        ++compiler_has_clean;
                    if (strfind(compiler_args[i], "--assembler", true))
                        ++compiler_has_assembler;
                    if (strfind(compiler_args[i], "--recursion", true))
                        ++compiler_has_recursion;
                    if (strfind(compiler_args[i], "--prolix", true))
                        ++compiler_has_verbose;
                    if (strfind(compiler_args[i], "--encoding", true))
                        ++compiler_has_encoding;
                }
            }

            toml_table_t *wg_compiler = toml_table_in(wg_toml_config, "compiler");
#if defined(_DBG_PRINT)
            if (!wg_compiler)
                printf("%s not exists in line:%d", "compiler", __LINE__);
#endif
            if (wg_compiler)
            {
                toml_array_t *option_arr = toml_array_in(wg_compiler, "option");
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
                            wg_snprintf(flag_to_search,
                                     size_flag_to_search,
                                     "%.2s",
                                     toml_option_value.u.s);
                        } else {
                            wg_strncpy(flag_to_search, toml_option_value.u.s, size_flag_to_search - 1);
                        }

                        if (proc_file != NULL) {
                            rewind(proc_file);
                            while (fgets(size_log, sizeof(size_log), proc_file) != NULL) {
                                if (strfind(size_log, "error while loading shared libraries:", false)) {
                                    wg_printfile(".watchdogs/compiler_test.log");
                                    goto compiler_end;
                                }
                                if (strfind(size_log, flag_to_search, true)) {
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

                        if (strfind(toml_option_value.u.s, "-d", true))
                          ++compiler_debugging;

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
                            wg_snprintf(merged, new_len, "%s", toml_option_value.u.s);
                        else
                            wg_snprintf(merged + old_len,
                                     new_len - old_len,
                                     " %s", toml_option_value.u.s);

                        wg_free(toml_option_value.u.s);
                        toml_option_value.u.s = NULL;
                    }

                    if (proc_file) {
                        fclose(proc_file);
                        if (path_access(".watchdogs/compiler_test.log"))
                            remove(".watchdogs/compiler_test.log");
                    }

                    if (merged) {
                        wgconfig.wg_toml_aio_opt = merged;
                    }
                    else {
                        wgconfig.wg_toml_aio_opt = strdup("");
                    }

                    if (compiler_has_debug != 0 && compiler_debugging != 0) {
                        size_t debug_flag_count = size_debug_memm / size_debug_memm_zero;
                        for (size_t i = 0; i < debug_flag_count; i++) {
                            const char *memm_saving_target = debug_memm[i];
                            size_t size_memm_saving_targetion = strlen(memm_saving_target);

                            while ((fetch_string_pos = strstr(wgconfig.wg_toml_aio_opt, memm_saving_target)) != NULL) {
                                memmove(fetch_string_pos, fetch_string_pos + size_memm_saving_targetion,
                                        strlen(fetch_string_pos + size_memm_saving_targetion) + 1);
                            }
                        }
                    }
                }

                if (compiler_has_debug > 0)
                    strcat(compiler_extra_options, " -d2 ");
                if (compiler_has_assembler > 0)
                    strcat(compiler_extra_options, " -a ");
                if (compiler_has_recursion > 0)
                    strcat(compiler_extra_options, " -R+ ");
                if (compiler_has_verbose > 0)
                    strcat(compiler_extra_options, " -v2 ");
                if (compiler_has_encoding > 0)
                    strcat(compiler_extra_options, " -C+ ");

                if (strlen(compiler_extra_options) > 0) {
                    size_t current_len = strlen(wgconfig.wg_toml_aio_opt);
                    size_t extra_opt_len = strlen(compiler_extra_options);

                    if (current_len + extra_opt_len < sizeof(wgconfig.wg_toml_aio_opt))
                        strcat(wgconfig.wg_toml_aio_opt, compiler_extra_options);
                    else
                        strncat(wgconfig.wg_toml_aio_opt, compiler_extra_options,
                            sizeof(wgconfig.wg_toml_aio_opt) - current_len - 1);
                }

                toml_array_t *toml_include_path = toml_array_in(wg_compiler, "include_path");
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
                            char size_path_val[WG_PATH_MAX + 26];
                            wg_strip_dot_fns(size_path_val, sizeof(size_path_val), path_val.u.s);
                            if (size_path_val[0] == '\0')
                                continue;

                            if (i > 0)
                            {
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1)
                                {
                                    wg_snprintf(include_aio_path + cur,
                                        sizeof(include_aio_path) - cur,
                                        " ");
                                }
                            }

                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1)
                            {
                                wg_snprintf(include_aio_path + cur,
                                    sizeof(include_aio_path) - cur,
                                    "-i\"%s\"",
                                    size_path_val);
                            }
                            wg_free(path_val.u.s);
                        }
                    }
                }

                if (args == NULL || *args == '\0' || (args[0] == '.' && args[1] == '\0'))
                {
#ifdef WG_WINDOWS
                    sa.bInheritHandle = TRUE;
                    HANDLE hFile = CreateFileA(
                            ".watchdogs/compiler.log",
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
                    ret_command = wg_snprintf(_compiler_input_,
                              sizeof(_compiler_input_),
                                    "%s %s -o%s %s %s -i%s",
                                    wg_compiler_input_pawncc_path,
                                    wgconfig.wg_toml_gm_input,
                                    wgconfig.wg_toml_gm_output,
                                    wgconfig.wg_toml_aio_opt,
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
                    ret_command = wg_snprintf(_compiler_input_, sizeof(_compiler_input_),
                        "%s %s %s%s %s%s %s%s",
                        wg_compiler_input_pawncc_path,
                        wgconfig.wg_toml_gm_input,
                        "-o",
                        wgconfig.wg_toml_gm_output,
                        wgconfig.wg_toml_aio_opt,
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
                    char *wg_compiler_unix_args[WG_MAX_PATH + 256];
                    char *compiler_unix_token = strtok(_compiler_input_, " ");
                    while (compiler_unix_token != NULL) {
                        wg_compiler_unix_args[i++] = compiler_unix_token;
                        compiler_unix_token = strtok(NULL, " ");
                    }
                    wg_compiler_unix_args[i] = NULL;

                    posix_spawn_file_actions_t process_file_actions;
                    posix_spawn_file_actions_init(&process_file_actions);
                    int posix_logging_file = open(".watchdogs/compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
                                wg_compiler_unix_args[0],
                                &process_file_actions,
                                &spawn_attr,
                                wg_compiler_unix_args,
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
                    char size_container_output[WG_PATH_MAX * 2];
                    wg_snprintf(size_container_output,
                        sizeof(size_container_output), "%s", wgconfig.wg_toml_gm_output);
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
                            goto compiler_done;
                        } else if (compiler_has_watchdogs) {
                            cause_compiler_expl(".watchdogs/compiler.log", ca, cb);
                            goto compiler_done;
                        } else if (compiler_has_clean) {
                            wg_printfile(".watchdogs/compiler.log");
                            if (path_exists(ca)) {
                                    remove(ca);
                            }
                            goto compiler_done;
                        }

                        wg_printfile(".watchdogs/compiler.log");

                        char log_line[WG_MAX_PATH * 4];
                        proc_file = fopen(".watchdogs/compiler.log", "r");

                        if (proc_file != NULL) {
                            while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
                                if (strfind(log_line, "backtrace", false))
                                    pr_color(stdout, FCOLOUR_CYAN,
                                        "~ backtrace detected - "
                                        "make sure you are using a newer version of pawncc than the one currently in use.");
                            }
                            fclose(proc_file);
                        }
                    }
compiler_done:
                    proc_f = fopen(".watchdogs/compiler.log", "r");
                    if (proc_f)
                    {
                        char compiler_line_buffer[WG_PATH_MAX];
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
                            wgconfig.wg_compiler_stat = 1;
                        }
                    }
                    else {
                        pr_error(stdout, "Failed to open .watchdogs/compiler.log");
                    }

                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
                    if (compiler_dur > 0x64) {
                        printf("~ This is taking a while, huh?\n"
                                "  Make sure you’ve cleared all the warnings,\n"
                                "  you're using the latest compiler,\n"
                                "  and double-check that your logic\n"
                                "  and pawn algorithm tweaks in the gamemode scripts line up.\n");
                        fflush(stdout);
                    }
                }
                else
                {
                    wg_strncpy(ptr_io->compiler_size_temp, compile_args, sizeof(ptr_io->compiler_size_temp) - 1);
                    ptr_io->compiler_size_temp[sizeof(ptr_io->compiler_size_temp) - 1] = '\0';

                    compiler_last_slash = strrchr(ptr_io->compiler_size_temp, '/');
                    compiler_back_slash = strrchr(ptr_io->compiler_size_temp, '\\');

                    if (compiler_back_slash && (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                        compiler_last_slash = compiler_back_slash;

                    if (compiler_last_slash)
                    {
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

                        size_t total_needed;
                        total_needed = strlen(ptr_io->compiler_direct_path) + 1 +
                                    strlen(ptr_io->compiler_size_file_name) + 1;

                        if (total_needed > sizeof(ptr_io->compiler_size_input_path))
                        {
                            wg_strncpy(ptr_io->compiler_direct_path, "gamemodes",
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

                        if (wg_snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                "%s/%s", ptr_io->compiler_direct_path, ptr_io->compiler_size_file_name) >=
                            (int)sizeof(ptr_io->compiler_size_input_path))
                        {
                            ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                        }
                     }  else {
                            wg_strncpy(ptr_io->compiler_size_file_name, ptr_io->compiler_size_temp,
                                sizeof(ptr_io->compiler_size_file_name) - 1);
                            ptr_io->compiler_size_file_name[sizeof(ptr_io->compiler_size_file_name) - 1] = '\0';

                            wg_strncpy(ptr_io->compiler_direct_path, ".", sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (wg_snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "./%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }
                        }

                    int compiler_finding_compile_args = 0;
                    compiler_finding_compile_args = wg_sef_fdir(ptr_io->compiler_direct_path, ptr_io->compiler_size_file_name, NULL);

                    if (!compiler_finding_compile_args &&
                        strcmp(ptr_io->compiler_direct_path, "gamemodes") != 0)
                    {
                        compiler_finding_compile_args = wg_sef_fdir("gamemodes",
                                            ptr_io->compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            wg_strncpy(ptr_io->compiler_direct_path, "gamemodes",
                                sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (wg_snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "gamemodes/%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }

                            if (wgconfig.wg_sef_count > RATE_SEF_EMPTY)
                                wg_strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count - 1],
                                    ptr_io->compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    if (!compiler_finding_compile_args && !strcmp(ptr_io->compiler_direct_path, "."))
                    {
                        compiler_finding_compile_args = wg_sef_fdir("gamemodes",
                                            ptr_io->compiler_size_file_name, NULL);
                        if (compiler_finding_compile_args)
                        {
                            wg_strncpy(ptr_io->compiler_direct_path, "gamemodes",
                                sizeof(ptr_io->compiler_direct_path) - 1);
                            ptr_io->compiler_direct_path[sizeof(ptr_io->compiler_direct_path) - 1] = '\0';

                            if (wg_snprintf(ptr_io->compiler_size_input_path, sizeof(ptr_io->compiler_size_input_path),
                                    "gamemodes/%s", ptr_io->compiler_size_file_name) >=
                                (int)sizeof(ptr_io->compiler_size_input_path))
                            {
                                ptr_io->compiler_size_input_path[sizeof(ptr_io->compiler_size_input_path) - 1] = '\0';
                            }

                            if (wgconfig.wg_sef_count > RATE_SEF_EMPTY)
                                wg_strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count - 1],
                                    ptr_io->compiler_size_input_path, MAX_SEF_PATH_SIZE);
                        }
                    }

                    wg_snprintf(wg_compiler_input_gamemode_path,
                            sizeof(wg_compiler_input_gamemode_path), "%s", wgconfig.wg_sef_found_list[1]);

                    if (compiler_finding_compile_args)
                    {
                        char __sef_path_sz[WG_PATH_MAX];
                        wg_snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wg_compiler_input_gamemode_path);
                        char *ext = strrchr(__sef_path_sz, '.');
                        if (ext)
                            *ext = '\0';

                        wg_snprintf(ptr_io->container_output, sizeof(ptr_io->container_output), "%s", __sef_path_sz);

                        char size_container_output[WG_MAX_PATH];
                        wg_snprintf(size_container_output, sizeof(size_container_output), "%s.amx", ptr_io->container_output);

#ifdef WG_WINDOWS
                        sa.bInheritHandle = TRUE;
                        HANDLE hFile = CreateFileA(
                                ".watchdogs/compiler.log",
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
                        ret_command = wg_snprintf(_compiler_input_,
                                  sizeof(_compiler_input_),
                                        "%s %s -o%s %s %s -i%s",
                                        wg_compiler_input_pawncc_path,
                                        wg_compiler_input_gamemode_path,
                                        size_container_output,
                                        wgconfig.wg_toml_aio_opt,
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
                        ret_command = wg_snprintf(_compiler_input_, sizeof(_compiler_input_),
                            "%s %s %s%s %s%s %s%s",
                            wg_compiler_input_pawncc_path,
                            wg_compiler_input_gamemode_path,
                            "-o",
                            size_container_output,
                            wgconfig.wg_toml_aio_opt,
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
                        char *wg_compiler_unix_args[WG_MAX_PATH + 256];
                        char *compiler_unix_token = strtok(_compiler_input_, " ");
                        while (compiler_unix_token != NULL) {
                            wg_compiler_unix_args[i++] = compiler_unix_token;
                            compiler_unix_token = strtok(NULL, " ");
                        }
                        wg_compiler_unix_args[i] = NULL;

                        posix_spawn_file_actions_t process_file_actions;
                        posix_spawn_file_actions_init(&process_file_actions);
                        int posix_logging_file = open(".watchdogs/compiler.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
                                    wg_compiler_unix_args[0],
                                    &process_file_actions,
                                    &spawn_attr,
                                    wg_compiler_unix_args,
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
                            proc_file = fopen(".watchdogs/compiler.log", "r");

                            if (proc_file != NULL) {
                                while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
                                    if (strfind(log_line, "backtrace", false))
                                        pr_color(stdout, FCOLOUR_CYAN,
                                            "~ backtrace detected - "
                                            "make sure you are using a newer version of pawncc than the one currently in use.\n");
                                }
                                fclose(proc_file);
                            }
                        }

compiler_done2:
                        proc_f = fopen(".watchdogs/compiler.log", "r");
                        if (proc_f)
                        {
                            char compiler_line_buffer[WG_PATH_MAX];
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
                                wgconfig.wg_compiler_stat = 1;
                            }
                        }
                        else {
                            pr_error(stdout, "Failed to open .watchdogs/compiler.log");
                        }

                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        printf("\n");
                        pr_color(stdout, FCOLOUR_CYAN,
                            " <P> Finished at %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
                        if (compiler_dur > 0x64) {
                            printf("~ This is taking a while, huh?\n"
                                    "  Make sure you’ve cleared all the warnings,\n"
                                    "  you're using the latest compiler,\n"
                                    "  and double-check that your logic\n"
                                    "  and pawn algorithm tweaks in the gamemode scripts line up.\n");
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

                toml_free(wg_toml_config);

                goto compiler_end;
            }
        } else {
            pr_crit(stdout,
                "\033[1;31merror:\033[0m pawncc (our compiler) not found\n"
                "  \033[2mhelp:\033[0m install it before continuing\n");
            printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");
            char *ptr_sigA = readline("");

            while (true)
            {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0)
                {
                    wg_free(ptr_sigA);
ret_ptr:
                    println(stdout, "Select platform:");
                    println(stdout, "-> [L/l] Linux");
                    println(stdout, "-> [W/w] Windows");
                    pr_color(stdout, FCOLOUR_YELLOW, " ^ work's in WSL/MSYS2\n");
                    println(stdout, "-> [T/t] Termux");

                    wgconfig.wg_sel_stat = 1;

                    char *platform = readline("==> ");

                    if (strfind(platform, "L", false))
                    {
                        int ret = wg_install_pawncc("linux");
loop_ipcc:
                        wg_free(platform);
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc;
                    }
                    if (strfind(platform, "W", false))
                    {
                        int ret = wg_install_pawncc("windows");
loop_ipcc2:
                        wg_free(platform);
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc2;
                    }
                    if (strfind(platform, "T", false))
                    {
                        int ret = wg_install_pawncc("termux");
loop_ipcc3:
                        wg_free(platform);
                        if (ret == -1 && wgconfig.wg_sel_stat != 0)
                            goto loop_ipcc3;
                    }
                    if (strfind(platform, "E", false)) {
                        wg_free(platform);
                        goto loop_end;
                    }
                    else {
                        pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                        wg_free(platform);
                        goto ret_ptr;
                    }
loop_end:
                    chain_goto_main(NULL);
                }
                else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0)
                {
                    wg_free(ptr_sigA);
                    break;
                }
                else
                {
                    pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                    wg_free(ptr_sigA);
                    goto ret_ptr;
                }
            }
        }

compiler_end:
        return 1;
}