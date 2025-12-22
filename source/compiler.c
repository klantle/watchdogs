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
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#if __has_include(<spawn.h>)
    #include <spawn.h>
#elif __has_include(<android-spawn.h>)
    #include <android-spawn.h>
#endif

#include "units.h"
#include "extra.h"
#include "library.h"
#include "debug.h"
#include "crypto.h"
#include "compiler.h"

#ifndef WG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 0x400
#endif
static io_compilers wg_compiler_sys = { 0 };

static void cause_compiler_expl(const char *log_file, const char *wgoutput, int debug);

int wg_run_compiler(const char *args, const char *compile_args,
                    const char *second_arg, const char *four_arg,
                    const char *five_arg, const char *six_arg,
                    const char *seven_arg, const char *eight_arg,
                    const char *nine_arg) {

        const int aio_extra_options = 7;
        wgconfig.wg_compiler_stat = 0;
        io_compilers comp;
        io_compilers *ptr_io = &comp;
#ifdef WG_WINDOWS
        PROCESS_INFORMATION pi;
        STARTUPINFO si = { sizeof(si) };
        SECURITY_ATTRIBUTES sa = { sizeof(sa) };
#endif
        struct timespec start = { 0 },
                          end = { 0 };
        double compiler_dur;

        const char* compiler_args[] = {
           second_arg, four_arg,
           five_arg, six_arg,
           seven_arg, eight_arg,
           nine_arg
        };

        FILE *this_proc_file;
        char proj_parse[WG_PATH_MAX] = { 0 };
        char size_log[WG_MAX_PATH * 4];
        char run_cmd[WG_PATH_MAX + 258];
        char include_aio_path[WG_PATH_MAX * 2] = { 0 };
        char _compiler_input_[WG_MAX_PATH + WG_PATH_MAX] = { 0 };
        char path_include[WG_PATH_MAX] = { 0 };
        char size_path_include[WG_MAX_PATH] = { 0 };

        char flag_to_search[3] = { 0 };
        size_t size_flag_to_search = sizeof(flag_to_search);

        char *compiler_size_last_slash = NULL;
        char *compiler_back_slash = NULL;

        char *size_include_extra = NULL;
        char *procure_string_pos = NULL;
        char compiler_extra_options[WG_PATH_MAX] = { 0 };

        const char *debug_options[] = {"-d1", "-d3"};
        size_t size_debug_options = sizeof(debug_options);
        size_t size_debug_options_zero = sizeof(debug_options[0]);
        size_t debug_flag_count = size_debug_options / size_debug_options_zero;

        int compiler_debugging = 0;
        int compiler_has_watchdogs = 0, compiler_has_debug = 0;
        int compiler_has_clean = 0, compiler_has_assembler = 0;
        int compiler_has_recursion = 0, compiler_has_verbose = 0;
        int compiler_has_compact = 0;

        const char *ptr_pawncc = NULL;
        if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_WINDOWS))
           ptr_pawncc = "pawncc.exe";
        else if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX))
           ptr_pawncc = "pawncc";

        if (dir_exists(".watchdogs") == 0)
           MKDIR(".watchdogs");

        int _wg_log_acces = path_access(".watchdogs/compiler.log");
        if (_wg_log_acces)
           remove(".watchdogs/compiler.log");

        FILE *wg_log_files = fopen(".watchdogs/compiler.log", "w");
        if (wg_log_files != NULL)
           fclose(wg_log_files);
        else {
           pr_error(stdout, "can't create .watchdogs/compiler.log!");
           __debug_function();
        }

        compiler_memory_clean();

#ifdef WG_LINUX
        const char *library_paths_list[] = {
            "/usr/local/lib", "/usr/local/lib32",
            "/data/data/com.termux/files/usr/lib",
            "/data/data/com.termux/files/usr/local/lib",
            "/data/data/com.termux/arm64/usr/lib",
            "/data/data/com.termux/arm32/usr/lib",
            "/data/data/com.termux/amd32/usr/lib",
            "/data/data/com.termux/amd64/usr/lib"
        };

        size_t counts =
              sizeof(library_paths_list) /
              sizeof(library_paths_list[0]);

        const char *old = NULL;
        char newpath[WG_MAX_PATH];
        char so_path[WG_PATH_MAX];
        old = getenv("LD_LIBRARY_PATH");
        if (!old)
            old = "";

        snprintf(newpath, sizeof(newpath),
              "%s", old);

        for (size_t i = 0; i < counts; i++) {
            snprintf(so_path, sizeof(so_path),
                     "%s/libpawnc.so", library_paths_list[i]);
            if (path_exists(so_path) != 0) {
                if (newpath[0] != '\0')
                    strncat(newpath, ":",
                      sizeof(newpath) - strlen(newpath) - 1);
                strncat(newpath, library_paths_list[i],
                        sizeof(newpath) - strlen(newpath) - 1);
            }
        }
        if (newpath[0] != '\0') {
            setenv("LD_LIBRARY_PATH", newpath, 1);
            pr_info(stdout, "LD_LIBRARY_PATH set to: %s", newpath);
        } else {
            pr_info(stdout, "libpawnc.so not found in any target path");
        }
#endif
        int __find_pawncc = wg_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc)
        {
            FILE *this_proc_file;
            this_proc_file = fopen("watchdogs.toml", "r");
            if (!this_proc_file) {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                __debug_function();
                goto compiler_end;
            }

            char wg_buf_err[WG_PATH_MAX];
            toml_table_t *wg_toml_config;
            wg_toml_config = toml_parse_file(this_proc_file, wg_buf_err, sizeof(wg_buf_err));

            if (this_proc_file) {
                fclose(this_proc_file);
            }

            if (!wg_toml_config) {
                pr_error(stdout, "failed to parse the watchdogs.toml....: %s", wg_buf_err);
                __debug_function();
                goto compiler_end;
            }

            char wg_compiler_input_pawncc_path[WG_PATH_MAX],
                 wg_compiler_input_proj_path[WG_PATH_MAX];
            snprintf(wg_compiler_input_pawncc_path,
                    sizeof(wg_compiler_input_pawncc_path), "%s", wgconfig.wg_sef_found_list[0]);

            if (path_exists(".watchdogs/compiler_test.log") == 0) {
                this_proc_file = fopen(".watchdogs/compiler_test.log", "w+");
                if (this_proc_file) {
                    fclose(this_proc_file);
                    snprintf(run_cmd, sizeof(run_cmd),
                        "sh -c \"%s\" >> .watchdogs/compiler_test.log 2>&1\"",
                        wg_compiler_input_pawncc_path);
                    wg_run_command(run_cmd);
                }
            }

            this_proc_file = fopen(".watchdogs/compiler_test.log", "r");
            if (!this_proc_file) {
                pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
                __debug_function();
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
                    if (strfind(compiler_args[i], "--compact", true))
                        ++compiler_has_compact;
                }
            }

            toml_table_t *wg_compiler = toml_table_in(wg_toml_config, "compiler");
#if defined(_DBG_PRINT)
            if (!wg_compiler)
                printf("%s notexists in line:%d", "compiler", __LINE__);
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

                        if (strlen(toml_option_value.u.s) >= 2) {
                            snprintf(flag_to_search,
                                     size_flag_to_search,
                                     "%.2s",
                                     toml_option_value.u.s);
                        } else {
                            strncpy(flag_to_search, toml_option_value.u.s, size_flag_to_search - 1);
                        }

                        if (this_proc_file != NULL) {
                            rewind(this_proc_file);
                            while (fgets(size_log, sizeof(size_log), this_proc_file) != NULL) {
                                if (strfind(size_log, "error while loading shared libraries:", true) ||
                                    strfind(size_log, "library \"libpawnc.so\" not found", true)) {
                                    wg_printfile(".watchdogs/compiler_test.log");
                                    goto compiler_end;
                                }
                            }
                        }

                        char *_compiler_options = toml_option_value.u.s;
                        while (*_compiler_options && isspace(*_compiler_options))
                            ++_compiler_options;

                        if (*_compiler_options != '-') {
not_valid_flag_options:
                            printf("[WARN]: "
                                "compiler option ");
                            pr_color(stdout, FCOLOUR_GREEN, "\"%s\" ", toml_option_value.u.s);
                            println(stdout, "not valid flag options!..");
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
                            snprintf(merged, new_len, "%s", toml_option_value.u.s);
                        else
                            snprintf(merged + old_len,
                                     new_len - old_len,
                                     " %s", toml_option_value.u.s);

                        wg_free(toml_option_value.u.s);
                        toml_option_value.u.s = NULL;
                    }

                    if (this_proc_file) {
                        fclose(this_proc_file);
                        if (path_access(".watchdogs/compiler_test.log"))
                            remove(".watchdogs/compiler_test.log");
                    }

                    if (merged) {
                        wgconfig.wg_toml_aio_opt = merged;
                    }
                    else {
                        wgconfig.wg_toml_aio_opt = strdup("");
                    }

                    char
                      *size_aio_option = wgconfig.wg_toml_aio_opt;
                    size_t
                      buffer_aio_options = strlen(wgconfig.wg_toml_aio_opt) + 1;
                    size_t
                      max_safe_shifts = buffer_aio_options * 2;

                    size_t rate_shift = 0;

                    if (compiler_has_debug != 0 && compiler_debugging != 0) {
                        if (size_debug_options == 0 ||
                            size_debug_options_zero == 0)
                        {
                            pr_error(stdout,
                                "Invalid debug flag array configuration");
                            __debug_function();
                            goto compiler_end;
                        }

                        if (size_debug_options < size_debug_options_zero ||
                            size_debug_options % size_debug_options_zero != 0) {
                            pr_error(stdout,
                                "Debug flag array size mismatch");
                            __debug_function();
                            goto compiler_end;
                        }

                        const size_t MAX_DEBUG_FLAGS = 256;

                        if (debug_flag_count > MAX_DEBUG_FLAGS) {
                            pr_error(stdout,
                                "Excessive debug flag count: %zu",
                                debug_flag_count);
                            __debug_function();
                            goto compiler_end;
                        }
                        if (wgconfig.wg_toml_aio_opt == NULL) {
                            pr_error(stdout,
                                "Configuration string is NULL");
                            __debug_function();
                            goto compiler_end;
                        }
                        char *original_config = strdup(wgconfig.wg_toml_aio_opt);
                        if (original_config == NULL) {
                            pr_error(stdout,
                                "Memory allocation failed for config backup");
                            __debug_function();
                            goto compiler_end;
                        }

                        for (size_t i = 0; i < debug_flag_count; i++) {
                            if (i >= (size_debug_options / sizeof(debug_options[0]))) {
                                pr_warning(stdout,
                                    "Debug flag index %zu out of bounds", i);
                                continue;
                            }

                            const char
                                *fetch_debug_flag = debug_options[i];

                            if (fetch_debug_flag == NULL) {
                                pr_warning(stdout,
                                    "NULL debug flag at index %zu", i);
                                continue;
                            }
                            size_t flag_length = strlen(fetch_debug_flag);
                            if (flag_length == 0) {
                                continue;
                            }
                            const size_t MAX_FLAG_LENGTH = WG_MAX_PATH;
                            if (flag_length > MAX_FLAG_LENGTH) {
                                pr_warning(stdout,
                                    "Suspiciously long debug flag at index %zu", i);
                                continue;
                            }

                            while (rate_shift < max_safe_shifts) {
                                char *fetch_flag_pos = strstr(size_aio_option, fetch_debug_flag);
                                if (fetch_flag_pos == NULL) {
                                    break
                                    ;
                                }
                                bool is_complete_token = true;
                                if (fetch_flag_pos > wgconfig.wg_toml_aio_opt) {
                                    char prev_char = *(fetch_flag_pos - 1);
                                    if (isalnum((unsigned char)prev_char) ||
                                        prev_char == '_') {
                                        is_complete_token = false;
                                    }
                                }
                                char next_char = *(fetch_flag_pos + flag_length);
                                if (isalnum((unsigned char)next_char) ||
                                    next_char == '_') {
                                    is_complete_token = false;
                                }
                                if (!is_complete_token) {
                                    size_aio_option = fetch_flag_pos + 1;
                                    continue
                                    ;
                                }
                                char *prev_flag = fetch_flag_pos + flag_length;
                                if ((fetch_flag_pos - wgconfig.wg_toml_aio_opt) + strlen(prev_flag) + 1 < buffer_aio_options)
                                {
                                    memmove(fetch_flag_pos, prev_flag, strlen(prev_flag) + 1);
                                    buffer_aio_options = strlen(wgconfig.wg_toml_aio_opt) + 1;
                                } else {
                                    pr_error(stdout,
                                        "Buffer overflow detected during flag removal");
                                    __debug_function();
                                    strncpy(wgconfig.wg_toml_aio_opt, original_config, buffer_aio_options);
                                    wg_free(original_config);
                                    goto compiler_end;
                                }

                                ++rate_shift;
                            }

                            if (rate_shift >= max_safe_shifts) {
                                pr_warning(stdout,
                                    "Excessive flag removals for '%s', possible loop", fetch_debug_flag);
                            }
                        }

                        wgconfig.wg_toml_aio_opt[buffer_aio_options - 1] = '\0';

                        if (strcmp(original_config, wgconfig.wg_toml_aio_opt) != 0) {
#if defined (_DBG_PRINT)
                            pr_info(stdout,"Debug flags removed. Original: '%s', Modified: '%s'",
                                    original_config, wgconfig.wg_toml_aio_opt);
#endif
                        }

                        wg_free(original_config);

                        if (strlen(wgconfig.wg_toml_aio_opt) >= buffer_aio_options) {
                            pr_error(stdout,
                                "Configuration string corruption detected");
                            __debug_function();
                            goto compiler_end;
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

                if (compiler_has_compact > 0)
                    strcat(compiler_extra_options, " -C+ ");

                int rate_debugger = 0;
#if defined(_DBG_PRINT)
                ++rate_debugger;
#endif
                if (compiler_has_watchdogs)
                    ++rate_debugger;

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
                                    snprintf(include_aio_path + cur,
                                        sizeof(include_aio_path) - cur,
                                        " ");
                                }
                            }

                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1)
                            {
                                snprintf(include_aio_path + cur,
                                    sizeof(include_aio_path) - cur,
                                    "-i%s ",
                                    size_path_val);
                            }
                            wg_free(path_val.u.s);
                        }
                    }
                }

                {
                    bool k = false;

                    if (strfind(compile_args, "../", true)) {
                        k = true;

                        size_t w = 0;
                        size_t j;
                        bool rate_path = false;

                        for (j = 0; compile_args[j] != '\0'; ) {
                            if (!rate_path && strncmp(&compile_args[j], "../", 3) == 0) {
                                j += 3;

                                while (compile_args[j] != '\0' &&
                                       compile_args[j] != ' ' &&
                                       compile_args[j] != '"') {
                                    proj_parse[w++] = compile_args[j++];
                                }

                                size_t size_last_slash = 0;
                                for (size_t idx = 0; idx < w; idx++) {
                                    if (proj_parse[idx] == '/' || proj_parse[idx] == '\\') {
                                        size_last_slash = idx + 1;
                                    }
                                }

                                if (size_last_slash > 0) {
                                    w = size_last_slash;
                                }

                                rate_path = true;
                                break;
                            } else {
                                j++;
                            }
                        }

                        if (rate_path && w > 0) {
                            memmove(proj_parse + 3, proj_parse, w + 1);
                            memcpy(proj_parse, "../", 3);
                            w += 3;
                            proj_parse[w] = '\0';

                            if (proj_parse[w-1] != '/' && proj_parse[w-1] != '\\') {
                                strcat(proj_parse, "/");
                            }
                        } else {
                            strcpy(proj_parse, "../");
                        }

                        char temp_path[256];
                        strcpy(temp_path, proj_parse);

                        if (strstr(temp_path, "gamemodes/") || strstr(temp_path, "gamemodes\\")) {
                            char *pos = strstr(temp_path, "gamemodes/");
                            if (!pos) pos = strstr(temp_path, "gamemodes\\");

                            if (pos) {
                                *pos = '\0';
                            }
                        }

                        snprintf(size_path_include, sizeof(size_path_include),
                                "-i%s "
                                "-i%spawno/include/ "
                                "-i%sqawno/include/ ",
                                temp_path, temp_path, temp_path);

                        strcpy(path_include, size_path_include);

                        pr_info(stdout,
                            "parent dir detected: %s - new include path created: %s",
                            compile_args, path_include);
                    } else {
                        if (dir_exists("pawno") && dir_exists("qawno"))
                          snprintf(path_include, sizeof(path_include), "-ipawno/include");
                        else if (dir_exists("pawno"))
                          snprintf(path_include, sizeof(path_include), "-ipawno/include");
                        else if (dir_exists("qawno"))
                          snprintf(path_include, sizeof(path_include), "-iqawno/include");
                        else
                          snprintf(path_include, sizeof(path_include), "-ipawno/include");
                    }
                }

                if (compile_args == NULL || *compile_args == '\0' || (compile_args[0] == '.' && compile_args[1] == '\0'))
                {
                    static int compiler_targets = 0;
                    if (compiler_targets != 1) {
                        pr_color(stdout, FCOLOUR_YELLOW,
                                "==== COMPILER TARGET ====\n");
                        printf("   ** This notification appears only once.\n"
                               "    * You can set the target using args in the command.\n");
                        printf("   * You run the command without any args.\n"
                            "   * Do you want to compile for " FCOLOUR_GREEN "%s " FCOLOUR_DEFAULT "(just enter), \n"
                            "   * or do you want to compile for something else?\n", wgconfig.wg_toml_proj_input);
                        printf("->");
                        char *proj_targets = readline(" ");
                        if (strlen(proj_targets) > 0) {
                            wgconfig.wg_toml_proj_input = strdup(proj_targets);
                        }
                        wg_free(proj_targets);
                        compiler_targets = 1;
                    }

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
                    ret_command = snprintf(_compiler_input_,
                            sizeof(_compiler_input_),
                                    "%s " "%s " "-o%s " "%s " "%s " "%s",
                                    wg_compiler_input_pawncc_path,
                                    wgconfig.wg_toml_proj_input,
                                    wgconfig.wg_toml_proj_output,
                                    wgconfig.wg_toml_aio_opt,
                                    include_aio_path,
                                    path_include);
                    if (rate_debugger > 0) {
                        printf("{\n");
                        printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                        printf("  \"project\": \"%s\",\n", wgconfig.wg_toml_proj_input);
                        printf("  \"output\": \"%s\",\n", wgconfig.wg_toml_proj_output);
                        printf("  \"include\": \"%s\",\n", path_include);
                        printf("  \"all-in-one options\": \"%s\",\n", wgconfig.wg_toml_aio_opt);
                        printf("  \"command\": \"%s\"\n", _compiler_input_);
                        printf("}\n");
                        fflush(stdout);
                    }
                    if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                        BOOL win32_process_success;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        win32_process_success = CreateProcessA(
                                /* lpApplicationName [in, optional]:
                                 * - NULL = use lpCommandLine to find executable
                                 * - Could specify full path to executable if needed
                                 * - If NULL, system searches in this order:
                                 *   1. Directory of calling process
                                 *   2. Windows system directory
                                 *   3. 16-bit system directory
                                 *   4. Windows directory
                                 *   5. Current directory
                                 *   6. PATH environment variable
                                 */
                                NULL,

                                /* lpCommandLine [in, out, optional]:
                                 * - Full command line to execute (including arguments)
                                 * - Must be writable memory (CreateProcess may modify it)
                                 * - First token should be executable name (unless lpApplicationName specified)
                                 * - If path contains spaces, must be quoted
                                 * - Maximum length: 32,768 characters including null terminator
                                 */
                                _compiler_input_,

                                /* lpProcessAttributes [in, optional]:
                                 * - NULL = Process handle cannot be inherited
                                 * - Security descriptor for the new process
                                 * - Typically NULL for normal process creation
                                 */
                                NULL,

                                /* lpThreadAttributes [in, optional]:
                                 * - NULL = Thread handle cannot be inherited
                                 * - Security descriptor for the main thread
                                 * - Typically NULL for normal process creation
                                 */
                                NULL,

                                /* bInheritHandles [in]:
                                 * - TRUE = Child inherits inheritable handles from parent
                                 * - Important for pipes, files, or other shared resources
                                 * - In this case, likely needed for I/O redirection
                                 *   (parent may need to read compiler output/error)
                                 */
                                TRUE,

                                /* dwCreationFlags [in]:
                                 * Combination of flags controlling process creation:
                                 *
                                 * CREATE_NO_WINDOW (0x08000000):
                                 * - Creates process without any visible console window
                                 * - Essential for background/silent operation
                                 * - Process output can still be captured via pipes
                                 * - Alternative: CREATE_NEW_CONSOLE would show window
                                 *
                                 * ABOVE_NORMAL_PRIORITY_CLASS (0x00008000):
                                 * - Gives compiler process slightly higher CPU priority
                                 * - Priority levels (lowest to highest):
                                 *   IDLE (0x40) -> BELOW_NORMAL (0x4000) -> NORMAL (0x20)
                                 *   -> ABOVE_NORMAL (0x8000) -> HIGH (0x80) -> REALTIME (0x100)
                                 * - Helps ensure compiler completes quickly
                                 * - Use cautiously: could affect system responsiveness
                                 */
                                CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,

                                /* lpEnvironment [in, optional]:
                                 * - NULL = Inherit parent's environment block
                                 * - Could specify custom environment variables
                                 * - Format: "Var1=Value1\0Var2=Value2\0\0" (double null-terminated)
                                 */
                                NULL,

                                /* lpCurrentDirectory [in, optional]:
                                 * - NULL = Child inherits parent's current directory
                                 * - Could specify working directory for child process
                                 * - Important for relative paths in compiler commands
                                 */
                                NULL,

                                /* lpStartupInfo [in]:
                                 * - Pointer to STARTUPINFOA structure
                                 * - Must set cb = sizeof(STARTUPINFOA) as first member
                                 * - Can specify stdio handles, window position, etc.
                                 * - Typically used for I/O redirection (hStdInput, hStdOutput, hStdError)
                                 * - If not using redirection, memset to 0 and set cb member
                                 */
                                &si,

                                /* lpProcessInformation [out]:
                                 * - Receives information about created process
                                 * - Contains:
                                 *   hProcess: Handle to the new process (must be closed with CloseHandle)
                                 *   hThread: Handle to the main thread (must be closed with CloseHandle)
                                 *   dwProcessId: Process ID (unique system-wide)
                                 *   dwThreadId: Thread ID
                                 * - IMPORTANT: Caller must close hProcess and hThread when done
                                 */
                                &pi
                        );
                        if (win32_process_success == TRUE) {
                            WaitForSingleObject(pi.hProcess, INFINITE);

                            clock_gettime(CLOCK_MONOTONIC, &end);

                            DWORD proc_exit_code;
                            GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                            __debug_function();
                        }
                    } else {
                        clock_gettime(CLOCK_MONOTONIC, &end);pr_error(stdout, "ret_compiler too long!");
                        __debug_function();
                        goto compiler_end;
                    }
                    if (hFile != INVALID_HANDLE_VALUE) {
                        CloseHandle(hFile);
                    }
#else
                    int ret_command = 0;
                    ret_command = snprintf(_compiler_input_, sizeof(_compiler_input_),
                        "%s " "%s " "%s%s " "%s %s " "%s",
                        wg_compiler_input_pawncc_path,
                        wgconfig.wg_toml_proj_input,
                        "-o",
                        wgconfig.wg_toml_proj_output,
                        wgconfig.wg_toml_aio_opt,
                        include_aio_path,
                        path_include);
                    if (ret_command > (int)sizeof(_compiler_input_)) {
                        pr_error(stdout, "ret_compiler too long!");
                        __debug_function();
                        goto compiler_end;
                    }
                    if (rate_debugger > 0) {
                        printf("{\n");
                        printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                        printf("  \"project\": \"%s\",\n", wgconfig.wg_toml_proj_input);
                        printf("  \"output\": \"%s\",\n", wgconfig.wg_toml_proj_output);
                        printf("  \"include\": \"%s\",\n", path_include);
                        printf("  \"all-in-one options\": \"%s\",\n", wgconfig.wg_toml_aio_opt);
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
                    int process_spawn_result = posix_spawnp(
                            /* pid_t *pid [out]:
                             * - Pointer to store the new process ID
                             * - On success, contains child process PID
                             * - Used later for waitpid() or process management
                             * - Similar to fork() return value in parent
                             */
                            &compiler_process_id,

                            /* const char *path [in]:
                             * - Name/path of executable to run (wg_compiler_unix_args[0])
                             * - The 'p' in posix_spawnp means PATH search:
                             *   @ Searches directories in PATH environment variable
                             *   @ If contains '/', treats as absolute or relative path
                             *   @ Otherwise searches PATH
                             * - Alternative: posix_spawn() requires absolute path
                             */
                            wg_compiler_unix_args[0],

                            /* const posix_spawn_file_actions_t *file_actions [in]:
                             * - Optional file descriptor manipulations before exec()
                             * - Can redirect stdin/stdout/stderr, open/close files
                             * - &process_file_actions suggests predefined actions like:
                             *   @ Redirect to/from pipes for I/O capture
                             *   @ Close unnecessary file descriptors
                             *   @ Open log files
                             * - NULL = no file actions, inherit parent's file descriptors
                             */
                            &process_file_actions,

                            /* const posix_spawnattr_t *attrp [in]:
                             * - Process attributes and execution flags
                             * - &spawn_attr suggests preconfigured attributes:
                             *   @ Signal mask/reset behavior
                             *   @ Process group/scheduling settings
                             *   @ Resource limits (RLIMIT_*)
                             *   @ User/group ID changes (setuid/setgid)
                             * - NULL = use default attributes
                             */
                            &spawn_attr,

                            /* char *const argv[] [in]:
                             * - Argument vector for new process (NULL-terminated)
                             * - wg_compiler_unix_args likely contains:
                             *   argv[0]: program name
                             *   argv[1...]: compiler flags, source files, output options
                             *   argv[last]: NULL terminator
                             * - Example: {"gcc", "-o", "program", "source.c", NULL}
                             * - Must be writable memory (some implementations modify)
                             */
                            wg_compiler_unix_args,

                            /* char *const envp[] [in]:
                             * - Environment variables for new process
                             * - 'environ' is global variable from unistd.h
                             *   @ Inherits parent's environment
                             *   @ Format: "VAR=value" strings, NULL-terminated array
                             * - Can provide custom environment:
                             *   @ Copy/modify environ array
                             *   @ Set specific variables (PATH, CC, CFLAGS, etc.)
                             * - NULL = use parent's environment
                             */
                            environ
                    );

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
                                __debug_function();
                                break;
                            }
                            if (i == POSIX_TIMEOUT - 1) {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                kill(compiler_process_id, SIGTERM);
                                sleep(2);
                                kill(compiler_process_id, SIGKILL);
                                pr_error(stdout,
                                        "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                __debug_function();
                                waitpid(compiler_process_id, &process_status, 0);
                                process_timeout_occurred = 1;
                            }
                        }
                        if (!process_timeout_occurred) {
                            if (WIFEXITED(process_status)) {
                                int proc_exit_code = 0;
                                proc_exit_code = WEXITSTATUS(process_status);
                                if (proc_exit_code != 0 && proc_exit_code != 1) {
                                    pr_error(stdout,
                                            "compiler process exited with code (%d)", proc_exit_code);
                                    __debug_function();
                                }
                            } else if (WIFSIGNALED(process_status)) {
                                pr_error(stdout,
                                        "compiler process terminated by signal (%d)", WTERMSIG(process_status));
                            }
                        }
                    } else {
                        pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result));
                        __debug_function();
                    }
#endif
                    char size_container_output[WG_PATH_MAX * 2];
                    snprintf(size_container_output,
                        sizeof(size_container_output), "%s", wgconfig.wg_toml_proj_output);
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
                        this_proc_file = fopen(".watchdogs/compiler.log", "r");

                        if (this_proc_file != NULL) {
                            while (fgets(log_line, sizeof(log_line), this_proc_file) != NULL) {
                                if (strfind(log_line, "backtrace", true))
                                    pr_color(stdout, FCOLOUR_CYAN,
                                        "~ backtrace detected - "
                                        "make sure you are using a newer version of pawncc than the one currently in use.");
                            }
                            fclose(this_proc_file);
                        }
                    }
compiler_done:
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
                        __debug_function();
                    }

                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
                    if (compiler_dur > 0x64) {
                        printf("~ This is taking a while, huh?\n"
                                "  Make sure you've cleared all the warnings,\n""  you're using the latest compiler,\n"
                                "  and double-check that your logic\n"
                                "  and pawn algorithm tweaks in the gamemode scripts line up.\n");
                        fflush(stdout);
                    }
                }
                else
                {
                    strncpy(ptr_io->compiler_size_temp, compile_args, sizeof(ptr_io->compiler_size_temp) - 1);
                    ptr_io->compiler_size_temp[sizeof(ptr_io->compiler_size_temp) - 1] = '\0';

                    compiler_size_last_slash = strrchr(ptr_io->compiler_size_temp, __PATH_CHR_SEP_LINUX);
                    compiler_back_slash = strrchr(ptr_io->compiler_size_temp, __PATH_CHR_SEP_WIN32);

                    if (compiler_back_slash && (!compiler_size_last_slash || compiler_back_slash > compiler_size_last_slash))
                        compiler_size_last_slash = compiler_back_slash;

                    if (compiler_size_last_slash)
                    {
                        size_t compiler_dir_len;
                        compiler_dir_len = (size_t)(compiler_size_last_slash - ptr_io->compiler_size_temp);

                        if (compiler_dir_len >= sizeof(ptr_io->compiler_direct_path))
                            compiler_dir_len = sizeof(ptr_io->compiler_direct_path) - 1;

                        memcpy(ptr_io->compiler_direct_path, ptr_io->compiler_size_temp, compiler_dir_len);
                        ptr_io->compiler_direct_path[compiler_dir_len] = '\0';

                        const char *compiler_filename_start = compiler_size_last_slash + 1;
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

                    int compiler_finding_compile_args = 0;
                    compiler_finding_compile_args = wg_sef_fdir(ptr_io->compiler_direct_path, ptr_io->compiler_size_file_name, NULL);

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

                    if (compiler_finding_compile_args)
                    {
                        char __sef_path_sz[WG_PATH_MAX];
                        snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wg_compiler_input_proj_path);
                        char *extension = strrchr(__sef_path_sz, '.');
                        if (extension)
                            *extension = '\0';

                        snprintf(ptr_io->container_output, sizeof(ptr_io->container_output), "%s", __sef_path_sz);

                        char size_container_output[WG_MAX_PATH];
                        snprintf(size_container_output, sizeof(size_container_output), "%s.amx", ptr_io->container_output);

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
                        ret_command = snprintf(_compiler_input_,
                                sizeof(_compiler_input_),
                                        "%s " "%s " "-o%s " "%s " "%s " "%s",
                                        wg_compiler_input_pawncc_path,
                                        wg_compiler_input_proj_path,
                                        size_container_output,
                                        wgconfig.wg_toml_aio_opt,
                                        include_aio_path,
                                        path_include);
                        if (rate_debugger > 0) {
                            printf("{\n");
                            printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                            printf("  \"project\": \"%s\",\n", wg_compiler_input_proj_path);
                            printf("  \"output\": \"%s\",\n", size_container_output);
                            printf("  \"include\": \"%s\",\n", path_include);
                            printf("  \"all-in-one options\": \"%s\",\n", wgconfig.wg_toml_aio_opt);
                            printf("  \"command\": \"%s\"\n", _compiler_input_);
                            printf("}\n");
                            fflush(stdout);
                        }
                        if (ret_command > 0 && ret_command < (int)sizeof(_compiler_input_)) {
                            BOOL win32_process_success;
                            clock_gettime(CLOCK_MONOTONIC, &start);
                            win32_process_success = CreateProcessA(
                                    /* lpApplicationName [in, optional]:
                                     * - NULL = use lpCommandLine to find executable
                                     * - Could specify full path to executable if needed
                                     * - If NULL, system searches in this order:
                                     *   1. Directory of calling process
                                     *   2. Windows system directory
                                     *   3. 16-bit system directory
                                     *   4. Windows directory
                                     *   5. Current directory
                                     *   6. PATH environment variable
                                     */
                                    NULL,

                                    /* lpCommandLine [in, out, optional]:
                                     * - Full command line to execute (including arguments)
                                     * - Must be writable memory (CreateProcess may modify it)
                                     * - First token should be executable name (unless lpApplicationName specified)
                                     * - If path contains spaces, must be quoted
                                     * - Maximum length: 32,768 characters including null terminator
                                     */
                                    _compiler_input_,

                                    /* lpProcessAttributes [in, optional]:
                                     * - NULL = Process handle cannot be inherited
                                     * - Security descriptor for the new process
                                     * - Typically NULL for normal process creation
                                     */
                                    NULL,

                                    /* lpThreadAttributes [in, optional]:
                                     * - NULL = Thread handle cannot be inherited
                                     * - Security descriptor for the main thread
                                     * - Typically NULL for normal process creation
                                     */
                                    NULL,

                                    /* bInheritHandles [in]:
                                     * - TRUE = Child inherits inheritable handles from parent
                                     * - Important for pipes, files, or other shared resources
                                     * - In this case, likely needed for I/O redirection
                                     *   (parent may need to read compiler output/error)
                                     */
                                    TRUE,

                                    /* dwCreationFlags [in]:
                                     * Combination of flags controlling process creation:
                                     *
                                     * CREATE_NO_WINDOW (0x08000000):
                                     * - Creates process without any visible console window
                                     * - Essential for background/silent operation
                                     * - Process output can still be captured via pipes
                                     * - Alternative: CREATE_NEW_CONSOLE would show window
                                     *
                                     * ABOVE_NORMAL_PRIORITY_CLASS (0x00008000):
                                     * - Gives compiler process slightly higher CPU priority
                                     * - Priority levels (lowest to highest):
                                     *   IDLE (0x40) -> BELOW_NORMAL (0x4000) -> NORMAL (0x20)
                                     *   -> ABOVE_NORMAL (0x8000) -> HIGH (0x80) -> REALTIME (0x100)
                                     * - Helps ensure compiler completes quickly
                                     * - Use cautiously: could affect system responsiveness
                                     */
                                    CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,

                                    /* lpEnvironment [in, optional]:
                                     * - NULL = Inherit parent's environment block
                                     * - Could specify custom environment variables
                                     * - Format: "Var1=Value1\0Var2=Value2\0\0" (double null-terminated)
                                     */
                                    NULL,

                                    /* lpCurrentDirectory [in, optional]:
                                     * - NULL = Child inherits parent's current directory
                                     * - Could specify working directory for child process
                                     * - Important for relative paths in compiler commands
                                     */
                                    NULL,

                                    /* lpStartupInfo [in]:
                                     * - Pointer to STARTUPINFOA structure
                                     * - Must set cb = sizeof(STARTUPINFOA) as first member
                                     * - Can specify stdio handles, window position, etc.
                                     * - Typically used for I/O redirection (hStdInput, hStdOutput, hStdError)
                                     * - If not using redirection, memset to 0 and set cb member
                                     */
                                    &si,

                                    /* lpProcessInformation [out]:
                                     * - Receives information about created process
                                     * - Contains:
                                     *   hProcess: Handle to the new process (must be closed with CloseHandle)
                                     *   hThread: Handle to the main thread (must be closed with CloseHandle)
                                     *   dwProcessId: Process ID (unique system-wide)
                                     *   dwThreadId: Thread ID
                                     * - IMPORTANT: Caller must close hProcess and hThread when done
                                     */
                                    &pi
                            );
                            if (win32_process_success == TRUE) {
                                WaitForSingleObject(pi.hProcess, INFINITE);

                                clock_gettime(CLOCK_MONOTONIC, &end);

                                DWORD proc_exit_code;
                                GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                                CloseHandle(pi.hProcess);
                                CloseHandle(pi.hThread);
                            } else {
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                                __debug_function();
                            }
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            pr_error(stdout, "ret_compiler too long!");
                            __debug_function();
                            goto compiler_end;
                        }
                        if (hFile != INVALID_HANDLE_VALUE) {
                            CloseHandle(hFile);
                        }
#else
                        int ret_command = 0;
                        ret_command = snprintf(_compiler_input_, sizeof(_compiler_input_),
                            "%s " "%s " "%s%s " "%s %s " "%s",
                            wg_compiler_input_pawncc_path,
                            wg_compiler_input_proj_path,
                            "-o",
                            size_container_output,
                            wgconfig.wg_toml_aio_opt,
                            include_aio_path,
                            path_include);
                        if (ret_command > (int)sizeof(_compiler_input_)) {
                            pr_error(stdout,"ret_compiler too long!");
                            __debug_function();
                            goto compiler_end;
                        }
                        if (rate_debugger > 0) {
                            printf("{\n");
                            printf("  \"compiler\": \"%s\",\n", wg_compiler_input_pawncc_path);
                            printf("  \"project\": \"%s\",\n", wg_compiler_input_proj_path);
                            printf("  \"output\": \"%s\",\n", size_container_output);
                            printf("  \"include\": \"%s\",\n", path_include);
                            printf("  \"all-in-one options\": \"%s\",\n", wgconfig.wg_toml_aio_opt);
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
                        int process_spawn_result = posix_spawnp(
                                /* pid_t *pid [out]:
                                 * - Pointer to store the new process ID
                                 * - On success, contains child process PID
                                 * - Used later for waitpid() or process management
                                 * - Similar to fork() return value in parent
                                 */
                                &compiler_process_id,

                                /* const char *path [in]:
                                 * - Name/path of executable to run (wg_compiler_unix_args[0])
                                 * - The 'p' in posix_spawnp means PATH search:
                                 *   @ Searches directories in PATH environment variable
                                 *   @ If contains '/', treats as absolute or relative path
                                 *   @ Otherwise searches PATH
                                 * - Alternative: posix_spawn() requires absolute path
                                 */
                                wg_compiler_unix_args[0],

                                /* const posix_spawn_file_actions_t *file_actions [in]:
                                 * - Optional file descriptor manipulations before exec()
                                 * - Can redirect stdin/stdout/stderr, open/close files
                                 * - &process_file_actions suggests predefined actions like:
                                 *   @ Redirect to/from pipes for I/O capture
                                 *   @ Close unnecessary file descriptors
                                 *   @ Open log files
                                 * - NULL = no file actions, inherit parent's file descriptors
                                 */
                                &process_file_actions,

                                /* const posix_spawnattr_t *attrp [in]:
                                 * - Process attributes and execution flags
                                 * - &spawn_attr suggests preconfigured attributes:
                                 *   @ Signal mask/reset behavior
                                 *   @ Process group/scheduling settings
                                 *   @ Resource limits (RLIMIT_*)
                                 *   @ User/group ID changes (setuid/setgid)
                                 * - NULL = use default attributes
                                 */
                                &spawn_attr,

                                /* char *const argv[] [in]:
                                 * - Argument vector for new process (NULL-terminated)
                                 * - wg_compiler_unix_args likely contains:
                                 *   argv[0]: program name
                                 *   argv[1...]: compiler flags, source files, output options
                                 *   argv[last]: NULL terminator
                                 * - Example: {"gcc", "-o", "program", "source.c", NULL}
                                 * - Must be writable memory (some implementations modify)
                                 */
                                wg_compiler_unix_args,

                                /* char *const envp[] [in]:
                                 * - Environment variables for new process
                                 * - 'environ' is global variable from unistd.h
                                 *   @ Inherits parent's environment
                                 *   @ Format: "VAR=value" strings, NULL-terminated array
                                 * - Can provide custom environment:
                                 *   @ Copy/modify environ array
                                 *   @ Set specific variables (PATH, CC, CFLAGS, etc.)
                                 * - NULL = use parent's environment
                                 */
                                environ
                        );

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
                                    __debug_function();
                                    break;
                                }
                                if (i == POSIX_TIMEOUT - 1) {
                                    clock_gettime(CLOCK_MONOTONIC, &end);
                                    kill(compiler_process_id, SIGTERM);
                                    sleep(2);
                                    kill(compiler_process_id, SIGKILL);
                                    pr_error(stdout,
                                            "posix_spawn process execution timeout! (%d seconds)", POSIX_TIMEOUT);
                                    __debug_function();
                                    waitpid(compiler_process_id, &process_status, 0);
                                    process_timeout_occurred = 1;
                                }
                            }
                            if (!process_timeout_occurred) {
                                if (WIFEXITED(process_status)) {
                                    int proc_exit_code = 0;
                                    proc_exit_code = WEXITSTATUS(process_status);
                                    if (proc_exit_code != 0 && proc_exit_code != 1) {
                                        pr_error(stdout,
                                                "compiler process exited with code (%d)", proc_exit_code);
                                        __debug_function();
                                    }
                                } else if (WIFSIGNALED(process_status)) {
                                    pr_error(stdout,
                                            "compiler process terminated by signal (%d)", WTERMSIG(process_status));
                                    __debug_function();
                                }
                            }
                        } else {
                            pr_error(stdout, "posix_spawn failed: %s", strerror(process_spawn_result));
                            __debug_function();
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
                            this_proc_file = fopen(".watchdogs/compiler.log", "r");

                            if (this_proc_file != NULL) {
                                while (fgets(log_line, sizeof(log_line), this_proc_file) != NULL) {
                                    if (strfind(log_line, "backtrace", true))
                                        pr_color(stdout, FCOLOUR_CYAN,
                                            "~ backtrace detected - "
                                            "make sure you are using a newer version of pawncc than the one currently in use.\n");
                                }
                                fclose(this_proc_file);
                            }
                        }

compiler_done2:
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
                            __debug_function();
                        }

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
                        printf("Cannot locate input: ");
                        pr_color(stdout, FCOLOUR_CYAN, "%s\n", compile_args);
                        goto compiler_end;
                    }
                }

                toml_free(wg_toml_config);

                goto compiler_end;
            }
        } else {
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
                    chain_ret_main(NULL);
                }
                else if (strcmp(pointer_signalA, "N") == 0 || strcmp(pointer_signalA, "n") == 0)
                {
                    wg_free(pointer_signalA);
                    break;
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
        return 1;
}


causeExplanation ccs[] =
{
/* 001 */  /* SYNTAX ERROR */
{"expected token", "A required token (e.g., ';', ',', ')') is missing from the code. Check the line indicated for typos or omissions."},
/* 002 */  /* SYNTAX ERROR */
{"only a single statement", "A `case` label can only be followed by a single statement. To use multiple statements, enclose them in a block with `{` and `}`."},
/* 003 */  /* SYNTAX ERROR */
{"declaration of a local variable must appear in a compound block", "Local variables declared inside a `case` or `default` label must be scoped within a compound block `{ ... }`."},
/* 012 */  /* SYNTAX ERROR */
{"invalid function call, not a valid address", "The symbol being called is not a function, or the syntax of the function call is incorrect."},
/* 014 */  /* SYNTAX ERROR */
{"invalid statement; not in switch", "The `case` or `default` label is only valid inside a `switch` statement."},
/* 015 */  /* SEMANTIC ERROR */
{"default case must be the last case", "The `default` case within a `switch` statement must appear after all other `case` labels."},
/* 016 */  /* SEMANTIC ERROR */
{"multiple defaults in switch", "A `switch` statement can only have one `default` case."},
/* 019 */  /* SEMANTIC ERROR */
{"not a label", "The symbol used in the `goto` statement is not defined as a label."},
/* 020 */  /* SYNTAX ERROR */
{"invalid symbol name", "Symbol names must start with a letter, an underscore '_', or an 'at' sign '@'."},
/* 036 */  /* SYNTAX ERROR */
{"empty statement", "Pawn does not support a standalone semicolon as an empty statement. double `;;`? - Use an empty block `{}` instead if intentional."},
/* 037 */  /* SYNTAX ERROR */
{"missing semicolon", "A semicolon ';' is expected at the end of this statement."},
/* 030 */  /* SYNTAX ERROR */
{"unexpected end of file", "The source file ended unexpectedly. This is often caused by a missing closing brace '}', parenthesis ')', or quote."},
/* 027 */  /* SYNTAX ERROR */
{"illegal character", "A character was found that is not valid in the current context (e.g., outside of a string or comment)."},
/* 026 */  /* SYNTAX ERROR */
{"missing closing parenthesis", "An opening parenthesis '(' was not closed with a matching ')'."},
/* 028 */  /* SYNTAX ERROR */
{"missing closing bracket", "An opening bracket '[' was not closed with a matching ']'."},
/* 054 */  /* SYNTAX ERROR */
{"missing closing brace", "An opening brace '{' was not closed with a matching '}'."},
/* 004 */  /* SEMANTIC ERROR */
{"is not implemented", "A function was declared but its implementation (body) was not found. This can be caused by a missing closing brace `}` in a previous function."},
/* 005 */  /* SEMANTIC ERROR */
{"function may not have arguments", "The `main()` function cannot accept any arguments."},
/* 006 */  /* SEMANTIC ERROR */
{"must be assigned to an array", "String literals must be assigned to a character array variable; they cannot be assigned to non-array types."},
/* 007 */  /* SEMANTIC ERROR */
{"operator cannot be redefined", "Only specific operators can be redefined (overloaded) in Pawn. Check the language specification."},
/* 008 */  /* SEMANTIC ERROR */
{"must be a constant expression; assumed zero", "Array sizes and compiler directives (like `#if`) require constant expressions (literals or declared constants)."},
/* 009 */  /* SEMANTIC ERROR */
{"invalid array size", "The size of an array must be 1 or greater."},
/* 017 */  /* SEMANTIC ERROR */
{"undefined symbol", "A symbol (variable, function, constant) is used but was never declared. Check for typos or missing declarations."},
/* 018 */  /* SEMANTIC ERROR */
{"initialization data exceeds declared size", "The data used to initialize the array contains more elements than the size specified for the array."},
/* 022 */  /* SEMANTIC ERROR */
{"must be lvalue", "The symbol on the left side of an assignment operator must be a modifiable variable, array element, or other 'lvalue'."},
/* 023 */  /* SEMANTIC ERROR */
{"array assignment must be simple assignment", "Arrays cannot be used with compound assignment operators (like `+=`). Only simple assignment `=` is allowed for arrays."},
/* 024 */  /* SEMANTIC ERROR */
{"break or continue is out of context", "The `break` statement is only valid inside loops (`for`, `while`, `do-while`) and `switch`. `continue` is only valid inside loops."},
/* 025 */  /* SEMANTIC ERROR */
{"function heading differs from prototype", "The function's definition (return type, name, parameters) does not match a previous declaration of the same function."},
/* 027 */  /* SEMANTIC ERROR */
{"invalid character constant", "An unknown escape sequence (like `\\z`) was used, or multiple characters were placed inside single quotes (e.g., 'ab')."},
/* 029 */  /* SEMANTIC ERROR */
{"invalid expression, assumed zero", "The compiler could not understand the structure of the expression. This is often due to a syntax error or operator misuse."},
/* 032 */  /* SEMANTIC ERROR */
{"array index out of bounds", "The index used to access the array is either negative or greater than or equal to the array's size."},
/* 045 */  /* SEMANTIC ERROR */
{"too many function arguments", "A function call was made with more than 64 arguments, which is the limit in Pawn."},
/* 203 */  /* WARNING */
{"symbol is never used", "A variable, constant, or function was defined but never referenced in the code.."},
/* 204 */  /* WARNING */
{"symbol is assigned a value that is never used", "A variable is assigned a value, but that value is never used in any subsequent operation. This might indicate unnecessary computation."},
/* 205 */  /* WARNING */
{"redundant code: constant expression is zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to zero (false), making the code block unreachable."},
/* 209 */  /* SEMANTIC ERROR */
{"should return a value", "A function that is declared to return a value must return a value on all possible execution paths."},
/* 211 */  /* WARNING */
{"possibly unintended assignment", "An assignment operator `=` was used in a context where a comparison operator `==` is typically used (e.g., in an `if` condition)."},
/* 010 */  /* SYNTAX ERROR */
{"invalid function or ", "You need to make sure that the area around the error line follows the proper syntax, structure, and rules typical of Pawn code."},
/* 213 */  /* SEMANTIC ERROR */
{"tag mismatch", "The type (or 'tag') of the expression does not match the type expected by the context. Check variable types and function signatures."},
/* 215 */  /* WARNING */
{"expression has no effect", "An expression is evaluated but does not change the program's state (e.g., `a + b;` on a line by itself). This is often a logical error."},
/* 217 */  /* WARNING */
{"loose indentation", "The indentation (spaces/tabs) is inconsistent. While this doesn't affect compilation, it harms code readability."},
/* 234 */  /* WARNING */
{"Function is deprecated", "This function is outdated and may be removed in future versions. The compiler suggests using an alternative."},
/* 013 */  /* SEMANTIC ERROR */
{"no entry point", "The program must contain a `main` function or another designated public function to serve as the entry point."},
/* 021 */  /* SEMANTIC ERROR */
{"symbol already defined", "A symbol (variable, function, etc.) is being redefined in the same scope. You cannot have two symbols with the same name in the same scope."},
/* 028 */  /* SEMANTIC ERROR */
{"invalid subscript", "The bracket operators `[` and `]` are being used incorrectly, likely with a variable that is not an array."},
/* 033 */  /* SEMANTIC ERROR */
{"array must be indexed", "An array variable is used in an expression without an index. You must specify which element of the array you want to access."},
/* 034 */  /* SEMANTIC ERROR */
{"argument does not have a default value", "In a function call with named arguments, a placeholder was used for an argument that does not have a default value specified."},
/* 035 */  /* SEMANTIC ERROR */
{"argument type mismatch", "The type of an argument passed to a function does not match the expected parameter type defined by the function."},
/* 037 */  /* SEMANTIC ERROR */
{"invalid string", "A string literal is not properly formed, often due to a missing closing quote or an invalid escape sequence."},
/* 039 */  /* SEMANTIC ERROR */
{"constant symbol has no size", "The `sizeof` operator cannot be applied to a symbolic constant. It is only for variables and types."},
/* 040 */  /* SEMANTIC ERROR */
{"duplicate case label", "Two `case` labels within the same `switch` statement have the same constant value. Each `case` must be unique."},
/* 041 */  /* SEMANTIC ERROR */
{"invalid ellipsis", "The compiler cannot determine the array size from the `...` initializer syntax."},
/* 042 */  /* SEMANTIC ERROR */
{"invalid combination of class specifiers", "A combination of storage class specifiers (e.g., `public`, `static`) is used that is not allowed by the language."},
/* 043 */  /* SEMANTIC ERROR */
{"character constant exceeds range", "A character constant has a value that is outside the valid 0-255 range for an 8-bit character."},
/* 044 */  /* SEMANTIC ERROR */
{"positional parameters must precede", "In a function call, all positional arguments must come before any named arguments."},
/* 046 */  /* SEMANTIC ERROR */
{"unknown array size", "An array was declared without a specified size and without an initializer to infer the size. Array sizes must be explicit."},
/* 047 */  /* SEMANTIC ERROR */
{"array sizes do not match", "In an assignment, the source and destination arrays have different sizes."},
/* 048 */  /* SEMANTIC ERROR */
{"array dimensions do not match", "The dimensions of the arrays used in an operation (e.g., addition) do not match."},
/* 049 */  /* SEMANTIC ERROR */
{"invalid line continuation", "A backslash `\\` was used at the end of a line, but it is not being used to continue a preprocessor directive or string literal correctly."},
/* 050 */  /* SEMANTIC ERROR */
{"invalid range", "A range expression (e.g., in a state array) is syntactically or logically invalid."},
/* 055 */  /* SEMANTIC ERROR */
{"start of function body without function header", "A block of code `{ ... }` that looks like a function body was encountered, but there was no preceding function declaration."},
/* 100 */  /* FATAL ERROR */
{"cannot read from file", "The specified source file or an included file could not be opened. It may not exist, or there may be permission issues."},
/* 101 */  /* FATAL ERROR */
{"cannot write to file", "The compiler cannot write to the output file. The disk might be full, the file might be in use, or there may be permission issues."},
/* 102 */  /* FATAL ERROR */
{"table overflow", "An internal compiler table (for symbols, tokens, etc.) has exceeded its maximum size. The source code might be too complex."},
/* 103 */  /* FATAL ERROR */
{"insufficient memory", "The compiler ran out of available system memory (RAM) while processing the source code."},
/* 104 */  /* FATAL ERROR */
{"invalid assembler instruction", "The opcode specified in an `#emit` directive is not a valid Pawn assembly instruction."},
/* 105 */  /* FATAL ERROR */
{"numeric overflow", "A numeric constant in the source code is too large to be represented."},
/* 107 */  /* FATAL ERROR */
{"too many error messages on one line", "One line of source code generated a large number of errors. The compiler is stopping to avoid flooding the output."},
/* 108 */  /* FATAL ERROR */
{"codepage mapping file not found", "The file specified for character set conversion (codepage) could not be found."},
/* 109 */  /* FATAL ERROR */
{"invalid path", "A file or directory path provided to the compiler is syntactically invalid or does not exist."},
/* 110 */  /* FATAL ERROR */
{"assertion failed", "A compile-time assertion check (using `#assert`) evaluated to false."},
/* 111 */  /* FATAL ERROR */
{"user error", "The `#error` preprocessor directive was encountered, explicitly halting the compilation process."},
/* 214 */  /* WARNING */
{"literal array/string passed to a non", "Did you forget that the parameter isn't a const parameter? Also, make sure you're using the latest version of the standard library."},
/* 200 */  /* WARNING */
{"is truncated to", "A symbol name (variable, function, etc.) exceeds the maximum allowed length and will be truncated, which may cause link errors."},
/* 201 */  /* WARNING */
{"redefinition of constant", "A constant or macro is being redefined. The new definition will override the previous one."},
/* 202 */  /* WARNING */
{"number of arguments does not match", "The number of arguments in a function call does not match the number of parameters in the function's declaration."},
/* 206 */  /* WARNING */
{"redundant test: constant expression is non-zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to a non-zero value (true), making the test unnecessary."},
/* 214 */  /* WARNING */
{"array argument was intended", "A non-constant array is being passed to a function parameter that is declared as `const`. The function promises not to modify it, so this is safe but noted."},
/* 060 */  /* PREPROCESSOR ERROR */
{"too many nested includes", "The level of nested `#include` directives has exceeded the compiler's limit. Check for circular or overly deep inclusion."},
/* 061 */  /* PREPROCESSOR ERROR */
{"recursive include", "A file is including itself, either directly or through a chain of other includes. This creates an infinite loop."},
/* 062 */  /* PREPROCESSOR ERROR */
{"macro recursion too deep", "The expansion of a recursive macro has exceeded the maximum allowed depth. Check for infinitely recursive macro definitions."},
/* 068 */  /* PREPROCESSOR ERROR */
{"division by zero", "A compile-time constant expression attempted to divide by zero."},
/* 069 */  /* PREPROCESSOR ERROR */
{"overflow in constant expression", "A calculation in a constant expression (e.g., in an `#if` directive) resulted in an arithmetic overflow."},
/* 070 */  /* PREPROCESSOR ERROR */
{"undefined macro", "A macro used in an `#if` or `#elif` directive has not been defined. Its value is assumed to be zero."},
/* 071 */  /* PREPROCESSOR ERROR */
{"missing preprocessor argument", "A function-like macro was invoked without providing the required number of arguments."},
/* 072 */  /* PREPROCESSOR ERROR */
{"too many macro arguments", "A function-like macro was invoked with more arguments than specified in its definition."},
/* 038 */  /* PREPROCESSOR ERROR */
{"extra characters on line", "Unexpected characters were found on the same line after a preprocessor directive (e.g., `#include <file> junk`)."},
/* Sentinel value to mark the end of the array. */
{NULL, NULL}
};

static const char *wg_find_warn_err(const char *line)
{
      int cindex;
      for (cindex = 0; ccs[cindex].cs_t; ++cindex) {
        if (strstr(line, ccs[cindex].cs_t))
            return ccs[cindex].cs_i;
      }
      return NULL;
}

void compiler_detailed(const char *wgoutput, int debug,
                       int wcnt, int ecnt, const char *compiler_ver,
                       int header_size, int code_size, int data_size,
                       int stack_size, int total_size)
{
      println(stdout,
        "Compile Complete! | " FCOLOUR_CYAN "%d pass (warning) | " FCOLOUR_RED "%d fail (error)" FCOLOUR_DEFAULT "",
        wcnt, ecnt);
      println(stdout, "-----------------------------");

      int amx_access = path_access(wgoutput);
      if (amx_access && debug != 0) {
                unsigned long hash = crypto_djb2_hash_file(wgoutput);

                printf("Output path: %s\n", wgoutput);
                printf("Header : %dB  |  Total        : %dB\n"
                       "Code   : %dB  |  hash (djb2)  : %#lx\n"
                       "Data   : %dB\n"
                       "Stack  : %dB\n",
                       header_size, total_size, code_size,
                       hash, data_size, stack_size);
                fflush(stdout);

                portable_stat_t st;
                if (portable_stat(wgoutput, &st) == 0) {
                        printf("ino    : %llu   |  File   : %lluB\n"
                               "dev    : %llu\n"
                               "read   : %s   |  write  : %s\n"
                               "execute: %s   |  mode   : %020o\n"
                               "atime  : %llu\n"
                               "mtime  : %llu\n"
                               "ctime  : %llu\n",
                               (unsigned long long)st.st_ino,
                               (unsigned long long)st.st_size,
                               (unsigned long long)st.st_dev,
                               (st.st_mode & S_IRUSR) ? "Y" : "N",
                               (st.st_mode & S_IWUSR) ? "Y" : "N",
                               (st.st_mode & S_IXUSR) ? "Y" : "N",
                               st.st_mode,
                               (unsigned long long)st.st_latime,
                               (unsigned long long)st.st_lmtime,
                               (unsigned long long)st.st_mctime
                        );
                        fflush(stdout);
                }
      }

      fflush(stdout);
      printf("\n");

      printf("* Pawn Compiler %s - Copyright (c) 1997-2006, ITB CompuPhase\n", compiler_ver);

      return;
}

static void cause_compiler_expl(const char *log_file, const char *wgoutput, int debug)
{
      __debug_function();

      FILE *plog = fopen(log_file, "r");
      if (!plog)
        return;

      char line[WG_MAX_PATH];
      int wcnt = 0, ecnt = 0;
      int header_size = 0, code_size = 0, data_size = 0;
      int stack_size = 0, total_size = 0;
      char compiler_ver[64] = { 0 };

      while (fgets(line, sizeof(line), plog)) {
        if (wg_strcase(line, "Warnings.") ||
            wg_strcase(line, "Warning.") ||
            wg_strcase(line, "Errors.")  ||
            wg_strcase(line, "Error."))
            continue;

        if (wg_strcase(line, "Header size:")) {
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
            const char *p = strstr(line, "Pawn compiler ");
            if (p) sscanf(p, "Pawn compiler %63s", compiler_ver);
            continue;
        }

        fwrite(line, 1, strlen(line), stdout);

        if (wg_strcase(line, "warning")) ++wcnt;
        if (wg_strcase(line, "error")) ++ecnt;

        const char *description = wg_find_warn_err(line);
        if (description) {
            const char *found = NULL;
            int mk_pos = 0;
            for (int i = 0; ccs[i].cs_t; ++i) {
                if ((found = strstr(line, ccs[i].cs_t))) {
                    mk_pos = found - line;
                    break;
                }
            }
            for (int i = 0; i < mk_pos; i++)
                putchar(' ');
            pr_color(stdout, FCOLOUR_BLUE, "^ %s :(\n", description);
        }
      }

      fclose(plog);

      compiler_detailed(wgoutput, debug, wcnt, ecnt,
                        compiler_ver, header_size, code_size,
                        data_size, stack_size, total_size);
}
