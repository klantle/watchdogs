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
        struct timespec pre_start = { 0 }, post_end = { 0 };
        double timer_rate_compile;

        const char* compiler_args[] = {
           second_arg, four_arg,
           five_arg, six_arg,
           seven_arg, eight_arg,
           nine_arg
        };

        FILE *this_proc_file;
        char proj_parse[WG_PATH_MAX] = { 0 };
        char size_log[WG_MAX_PATH * 4] = { 0 };
        char run_cmd[WG_PATH_MAX + 258] = { 0 };
        char include_aio_path[WG_PATH_MAX * 2] = { 0 };
        char path_include[WG_PATH_MAX] = { 0 };
        char size_path_include[WG_MAX_PATH] = { 0 };
        char _compiler_input_[WG_MAX_PATH + WG_PATH_MAX] = { 0 };

        char flag_to_search[3] = { 0 };
        size_t size_flag_to_search = sizeof(flag_to_search);

        char *compiler_size_last_slash = NULL;
        char *compiler_back_slash = NULL;

        char *size_include_extra = NULL;
        char *procure_string_pos = NULL;
        char compiler_extra_options[WG_PATH_MAX] = { 0 };

        static const char *debug_options[] = {"-d1", "-d3"};
        size_t size_debug_options = sizeof(debug_options);
        size_t size_debug_options_zero = sizeof(debug_options[0]);
        size_t debug_flag_count = size_debug_options / size_debug_options_zero;

        int compiler_debugging = 0;
        int compiler_has_watchdogs = 0, compiler_has_debug = 0;
        int compiler_has_clean = 0, compiler_has_assembler = 0;
        int compiler_has_recursion = 0, compiler_has_verbose = 0;
        int compiler_has_compact = 0;

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
        static int rate_export_path = 0;
        if (rate_export_path < 1) {
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

          const char
            *_old = NULL;
          char _newpath[WG_MAX_PATH];
          char _so_path[WG_PATH_MAX];
          _old = getenv("LD_LIBRARY_PATH");
          if (!_old)
              _old = "";

          snprintf(_newpath, sizeof(_newpath),
                "%s", _old);

          for (size_t i = 0; i < counts; i++) {
              snprintf(_so_path, sizeof(_so_path),
                       "%s/libpawnc.so", library_paths_list[i]);
              if (path_exists(_so_path) != 0) {
                  if (_newpath[0] != '\0')
                      strncat(_newpath, ":", sizeof(_newpath) - strlen(_newpath) - 1);
                  strncat(_newpath, library_paths_list[i],
                          sizeof(_newpath) - strlen(_newpath) - 1);
              }
          }
          if ( _newpath[0] != '\0' ) {
              setenv("LD_LIBRARY_PATH", _newpath, 1);
              pr_info(stdout, "LD_LIBRARY_PATH set to: %s", _newpath);
          } else {
              pr_info(stdout, "libpawnc.so not found in any target path");
          }
          ++rate_export_path;
        }
#endif
        char *_pointer_pawncc = NULL;
        int __rate_pawncc_exists = 0;

        if (strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_WINDOWS) == 0)
          {
              _pointer_pawncc = "pawncc.exe";
          }
        else if (strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_LINUX) == 0)
          {
              _pointer_pawncc = "pawncc";
          }
        if (dir_exists("pawno") != 0 && dir_exists("qawno") != 0)
          {
              __rate_pawncc_exists = wg_sef_fdir("pawno", _pointer_pawncc, NULL);
              if (__rate_pawncc_exists) {
                  ;
              } else {
                  __rate_pawncc_exists = wg_sef_fdir("qawno", _pointer_pawncc, NULL);
                  if (__rate_pawncc_exists < 1) {
                    __rate_pawncc_exists = wg_sef_fdir(".", _pointer_pawncc, NULL);
                  }
              }
          }
        else if (dir_exists("pawno") != 0)
          {
              __rate_pawncc_exists = wg_sef_fdir("pawno", _pointer_pawncc, NULL);
              if (__rate_pawncc_exists) {
                  ;
              } else {
                  __rate_pawncc_exists = wg_sef_fdir(".", _pointer_pawncc, NULL);
              }
          }
        else if (dir_exists("qawno") != 0)
          {
              __rate_pawncc_exists = wg_sef_fdir("qawno", _pointer_pawncc, NULL);
              if (__rate_pawncc_exists) {
                  ;
              } else {
                  __rate_pawncc_exists = wg_sef_fdir(".", _pointer_pawncc, NULL);
              }
          }
        else {
            __rate_pawncc_exists = wg_sef_fdir(".", _pointer_pawncc, NULL);
        }

        if (__rate_pawncc_exists) {
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
                					"%s -0000000U > .watchdogs/compiler_test.log 2>&1",
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
                            continue
                            ;

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
                            break
                            ;
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
                                continue
                                ;
                            }

                            const char
                                *fetch_debug_flag = debug_options[i];

                            if (fetch_debug_flag == NULL) {
                                pr_warning(stdout,
                                    "NULL debug flag at index %zu", i);
                                continue
                                ;
                            }
                            size_t flag_length = strlen(fetch_debug_flag);
                            if (flag_length == 0) {
                                continue
                                ;
                            }
                            const size_t MAX_FLAG_LENGTH = WG_MAX_PATH;
                            if (flag_length > MAX_FLAG_LENGTH) {
                                pr_warning(stdout,
                                    "Suspiciously long debug flag at index %zu", i);
                                continue
                                ;
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

                toml_array_t *toml_include_path = toml_array_in(wg_compiler, "includes");
#if defined(_DBG_PRINT)
                if (!toml_include_path)
                    printf("%s not exists in line:%d", "includes", __LINE__);
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
                                continue
                                ;

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
                                break
                                ;
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

                if (compile_args == NULL)
                  {
                      compile_args = "";
                  }

                if (*compile_args == '\0' || (compile_args[0] == '.' && compile_args[1] == '\0'))
                {
                    static int compiler_targets = 0;
                    if (compiler_targets != 1 && strlen(compile_args) < 1) {
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
                        clock_gettime(CLOCK_MONOTONIC, &pre_start);
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

                            clock_gettime(CLOCK_MONOTONIC, &post_end);

                            DWORD proc_exit_code;
                            GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &post_end);
                            pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                            __debug_function();
                        }
                    } else {
                        clock_gettime(CLOCK_MONOTONIC, &post_end);
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
                        clock_gettime(CLOCK_MONOTONIC, &pre_start);
                        for (int i = 0; i < POSIX_TIMEOUT; i++) {
                            int p_result = -1;
                            p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                            if (p_result == 0)
                                usleep(0xC350);
                            else if (p_result == compiler_process_id) {
                                clock_gettime(CLOCK_MONOTONIC, &post_end);
                                break
                                ;
                            }
                            else {
                                clock_gettime(CLOCK_MONOTONIC, &post_end);
                                pr_error(stdout, "waitpid error");
                                __debug_function();
                                break
                                ;
                            }
                            if (i == POSIX_TIMEOUT - 1) {
                                clock_gettime(CLOCK_MONOTONIC, &post_end);
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
                                break
                                ;
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

                    timer_rate_compile = (post_end.tv_sec - pre_start.tv_sec)
                                        + (post_end.tv_nsec - pre_start.tv_nsec) / 1e9;

                    printf("\n");
                    pr_color(stdout, FCOLOUR_CYAN,
                        " <P> Finished at %.3fs (%.0f ms)\n",
                        timer_rate_compile, timer_rate_compile * 1000.0);
                    if (timer_rate_compile > 0x64) {
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
                            clock_gettime(CLOCK_MONOTONIC, &pre_start);
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

                                clock_gettime(CLOCK_MONOTONIC, &post_end);

                                DWORD proc_exit_code;
                                GetExitCodeProcess(pi.hProcess, &proc_exit_code);

                                CloseHandle(pi.hProcess);
                                CloseHandle(pi.hThread);
                            } else {
                                clock_gettime(CLOCK_MONOTONIC, &post_end);
                                pr_error(stdout, "CreateProcess failed! (%lu)", GetLastError());
                                __debug_function();
                            }
                        } else {
                            clock_gettime(CLOCK_MONOTONIC, &post_end);
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
                            clock_gettime(CLOCK_MONOTONIC, &pre_start);
                            for (int i = 0; i < POSIX_TIMEOUT; i++) {
                                int p_result = -1;
                                p_result = waitpid(compiler_process_id, &process_status, WNOHANG);
                                if (p_result == 0)
                                    usleep(0xC350);
                                else if (p_result == compiler_process_id) {
                                    clock_gettime(CLOCK_MONOTONIC, &post_end);
                                    break
                                    ;
                                }
                                else {
                                    clock_gettime(CLOCK_MONOTONIC, &post_end);
                                    pr_error(stdout, "waitpid error");
                                    __debug_function();
                                    break
                                    ;
                                }
                                if (i == POSIX_TIMEOUT - 1) {
                                    clock_gettime(CLOCK_MONOTONIC, &post_end);
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
                                    break
                                    ;
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

                        timer_rate_compile = (post_end.tv_sec - pre_start.tv_sec)
                                            + (post_end.tv_nsec - pre_start.tv_nsec) / 1e9;

                        printf("\n");
                        pr_color(stdout, FCOLOUR_CYAN,
                            " <P> Finished at %.3fs (%.0f ms)\n",
                            timer_rate_compile, timer_rate_compile * 1000.0);
                        if (timer_rate_compile > 0x64) {
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
                    break
                    ;
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
{
    "expected token",
    "A required syntactic element is missing from the parse tree. The parser expected one of: a semicolon ';', comma ',', closing parenthesis ')', bracket ']', or brace '}'. This typically indicates a malformed statement, improper expression termination, or incorrect nesting of control structures. Verify the statement's grammatical completeness according to Pawn's context-free grammar."
},

/* 002 */  /* SYNTAX ERROR */
{
    "only a single statement",
    "The `case` label syntactic production permits exactly one statement as its immediate successor. For multiple statements, you must encapsulate them within a compound statement delimited by braces `{ ... }`. This restriction stems from Pawn's simplified switch statement implementation which avoids implicit block creation."
},

/* 003 */  /* SYNTAX ERROR */
{
    "declaration of a local variable must appear in a compound block",
    "Variable declarations within `case` or `default` labels require explicit scoping via compound blocks due to potential jump-to-label issues in the control flow graph. Without explicit braces, the variable's lifetime and scope cannot be properly determined, violating the language's static single assignment analysis."
},

/* 012 */  /* SYNTAX ERROR */
{
    "invalid function call, not a valid address",
    "The identifier preceding the parenthesized argument list does not resolve to a function symbol in the current scope, or the call expression violates the function call syntax. Possible causes: attempting to call a non-function identifier, using incorrect call syntax for function pointers, or encountering a malformed expression that the parser misinterprets as a function call."
},

/* 014 */  /* SYNTAX ERROR */
{
    "invalid statement; not in switch",
    "The `case` or `default` labeled statement appears outside the lexical scope of any `switch` statement. These labels are context-sensitive productions that are only syntactically valid when nested within a switch statement's body according to Pawn's grammar specification section 6.8.1."
},

/* 015 */  /* SEMANTIC ERROR */
{
    "default case must be the last case",
    "The `default` label within a switch statement's case list must appear after all explicit `case` constant expressions. This ordering constraint is enforced by the Pawn compiler's semantic analysis phase to ensure predictable control flow and to simplify the generated jump table implementation."
},

/* 016 */  /* SEMANTIC ERROR */
{
    "multiple defaults in switch",
    "A `switch` statement contains more than one `default` label, which creates ambiguous control flow. According to the language specification (ISO/IEC TR 18037:2008), each switch statement may have at most one default case to serve as the catch-all branch in the decision tree."
},

/* 019 */  /* SEMANTIC ERROR */
{
    "not a label",
    "The identifier following the `goto` keyword does not correspond to any labeled statement in the current function's scope. Label resolution occurs during the semantic analysis phase, and the target must be a label defined earlier in the same function body (forward jumps are permitted)."
},

/* 020 */  /* SYNTAX ERROR */
{
    "invalid symbol name",
    "The identifier violates Pawn's lexical conventions for symbol names. Valid identifiers must match the regular expression: `[_@a-zA-Z][_@a-zA-Z0-9]*`. The '@' character has special significance for public/forward declarations and must be used consistently throughout the symbol's lifetime."
},

/* 036 */  /* SYNTAX ERROR */
{
    "empty statement",
    "A standalone semicolon constitutes a null statement, which Pawn's grammar specifically disallows in most contexts to prevent accidental emptiness. If intentional empty statement is needed, use an explicit empty block `{}`. Note that double semicolons `;;` often indicate a missing statement between them."
},

/* 037 */  /* SYNTAX ERROR */
{
    "missing semicolon",
    "A statement terminator (';') is required but absent. In Pawn's LL(1) grammar, semicolons terminate: expression statements, declarations, iteration statements, jump statements, and return statements. The parser's predictive parsing table expected this token to complete the current production."
},

/* 030 */  /* SYNTAX ERROR */
{
    "unexpected end of file",
    "The lexical analyzer reached EOF while the parser was still expecting tokens to complete one or more grammatical constructs. Common causes: unclosed block comment `/*`, string literal without terminating quote, unmatched braces/parentheses/brackets, or incomplete function/control structure."
},

/* 027 */  /* SYNTAX ERROR */
{
    "illegal character",
    "The source character (codepoint) is not valid in the current lexical context. Outside of string literals and comments, Pawn only accepts characters from its valid character set (typically ASCII or the active codepage). Control characters (0x00-0x1F) except whitespace are generally illegal."
},

/* 026 */  /* SYNTAX ERROR */
{
    "missing closing parenthesis",
    "An opening parenthesis '(' lacks its corresponding closing ')', creating an unbalanced delimiter sequence. This affects expression grouping, function call syntax, and condition specifications. The parser's delimiter stack detected this mismatch during syntax tree construction."
},

/* 028 */  /* SYNTAX ERROR */
{
    "missing closing bracket",
    "An opening bracket '[' was not matched with a closing ']'. This affects array subscripting, array declarations, and sizeof expressions. The bracket matching algorithm in the parser's shift-reduce automaton failed to find a closing bracket before the relevant scope ended."
},

/* 054 */  /* SYNTAX ERROR */
{
    "missing closing brace",
    "An opening brace '{' lacks its corresponding closing '}'. This affects compound statements, initializer lists, and function bodies. The brace nesting counter in the lexical analyzer reached EOF without returning to zero, indicating structural incompleteness."
},

/* 004 */  /* SEMANTIC ERROR */
{
    "is not implemented",
    "A function prototype was declared (forward reference) but no corresponding definition appears in the translation unit. This may also indicate that the compiler's symbol table contains an unresolved external reference, possibly due to a previous function's missing closing brace causing the parser to incorrectly associate following code."
},

/* 005 */  /* SEMANTIC ERROR */
{
    "function may not have arguments",
    "The `main()` function, serving as the program entry point, must have signature `main()` with zero parameters. This restriction ensures consistent program initialization across all Pawn implementations and prevents ambiguity in startup argument passing conventions."
},

/* 006 */  /* SEMANTIC ERROR */
{
    "must be assigned to an array",
    "String literals are rvalues of type 'array of char' and can only be assigned to compatible array types. The assignment operator's left operand must be an array lvalue with sufficient capacity (including null terminator). This is enforced during type checking of assignment expressions."
},

/* 007 */  /* SEMANTIC ERROR */
{
    "operator cannot be redefined",
    "Attempt to overload an operator that Pawn does not support for overloading. Only a specific subset of operators (typically arithmetic and comparison operators) can be overloaded via operator functions. Consult the language specification for the exhaustive list of overloadable operators."
},

/* 008 */  /* SEMANTIC ERROR */
{
    "must be a constant expression; assumed zero",
    "The context requires a compile-time evaluable constant expression but received a runtime expression. This affects: array dimension specifiers, case labels, bit-field widths, enumeration values, and preprocessor conditionals. The constant folder attempted evaluation but found variable references or non-constant operations."
},

/* 009 */  /* SEMANTIC ERROR */
{
    "invalid array size",
    "Array dimension specifier evaluates to a non-positive integer. Array sizes must be ≥1. For VLAs (Variable Length Arrays), the size expression must evaluate to positive at the point of declaration. This check occurs during array type construction in the type system."
},

/* 017 */  /* SEMANTIC ERROR */
{
    "undefined symbol",
    "Identifier lookup in the current scope chain failed to find any declaration for this symbol. The compiler traversed: local block scope → function scope → file scope → global scope. Possible causes: typographical error, missing include directive, symbol declared in excluded conditional compilation block, or incorrect namespace/visibility qualifiers."
},

/* 018 */  /* SEMANTIC ERROR */
{
    "initialization data exceeds declared size",
    "The initializer list contains more elements than the array's declared capacity. For aggregate initialization, the number of initializer-clauses must not exceed the array bound. For string literals, the literal length (including null terminator) must not exceed array size."
},

/* 022 */  /* SEMANTIC ERROR */
{
    "must be lvalue",
    "The left operand of an assignment operator (=, +=, etc.) does not designate a modifiable location in storage. Valid lvalues include: variables, array subscript expressions, dereferenced pointers, and structure/union members. Constants, literals, and rvalue expressions cannot appear on the left of assignment."
},

/* 023 */  /* SEMANTIC ERROR */
{
    "array assignment must be simple assignment",
    "Arrays cannot be used with compound assignment operators due to the semantic complexity of element-wise operations. Only simple assignment '=' is permitted for array types, which performs memcpy-like behavior. For element-wise operations, explicit loops or functions must be used."
},

/* 024 */  /* SEMANTIC ERROR */
{
    "break or continue is out of context",
    "A `break` statement appears outside any switch/loop construct, or `continue` appears outside any loop construct. These jump statements are context-sensitive and require specific enclosing syntactic structures. The control flow graph builder validates these constraints."
},

/* 025 */  /* SEMANTIC ERROR */
{
    "function heading differs from prototype",
    "Function definition signature does not match previous declaration in: return type, parameter count, parameter types (including qualifiers), or calling convention. This violates the one-definition rule and causes type incompatibility in the function type consistency check."
},

/* 027 */  /* SEMANTIC ERROR */
{
    "invalid character constant",
    "Character constant syntax error: multiple characters in single quotes, unknown escape sequence, or numeric escape sequence out of valid range (0-255). Valid escape sequences are: \\a, \\b, \\e, \\f, \\n, \\r, \\t, \\v, \\\\, \\', \\\", \\xHH, \\OOO (octal)."
},

/* 029 */  /* SEMANTIC ERROR */
{
    "invalid expression, assumed zero",
    "The expression parser encountered syntactically valid but semantically meaningless construct, such as mismatched operator operands, incorrect operator precedence binding, or type-incompatible operations. The expression evaluator defaults to zero to allow continued parsing for additional error detection."
},

/* 032 */  /* SEMANTIC ERROR */
{
    "array index out of bounds",
    "Subscript expression evaluates to value outside array bounds [0, size-1]. This is a compile-time check for constant indices; runtime bounds checking depends on implementation. For multidimensional arrays, each dimension is checked independently."
},

/* 045 */  /* SEMANTIC ERROR */
{
    "too many function arguments",
    "Function call contains more than 64 actual arguments, exceeding Pawn's implementation limit. This architectural constraint stems from the virtual machine's call frame design and register allocation scheme. Consider refactoring using structures or arrays for parameter groups."
},

/* 203 */  /* WARNING */
{
    "symbol is never used",
    "Variable, constant, or function declared but never referenced in any reachable code path. This may indicate: dead code, incomplete implementation, debugging remnants, or accidental omission. The compiler's data flow analysis determined no read operations on the symbol after its declaration."
},

/* 204 */  /* WARNING */
{
    "symbol is assigned a value that is never used",
    "Variable receives a value (via assignment or initialization) that is subsequently never read. This suggests: unnecessary computation, redundant initialization, or logical error where the variable should be used but isn't. The live variable analysis tracks definitions and uses."
},

/* 205 */  /* WARNING */
{
    "redundant code: constant expression is zero",
    "Conditional expression in if/while/for evaluates to compile-time constant false (0), making the controlled block dead code. This often results from: macro expansion errors, contradictory preprocessor conditions, or logical errors in constant expressions. The constant folder detected this during control flow analysis."
},

/* 209 */  /* SEMANTIC ERROR */
{
    "should return a value",
    "Non-void function reaches end of control flow without returning a value via return statement. All possible execution paths must return a value of compatible type. The control flow graph analyzer found at least one path terminating at function end without return."
},

/* 211 */  /* WARNING */
{
    "possibly unintended assignment",
    "Assignment expression appears in boolean context where equality comparison is typical (e.g., if condition). The expression `if (a = b)` assigns b to a, then tests a's truth value. If comparison was intended, use `if (a == b)`. This heuristic warning triggers on assignment in conditional context."
},

/* 010 */  /* SYNTAX ERROR */
{
    "invalid function or declaration",
    "The parser expected a function declarator or variable declaration but encountered tokens that don't conform to declaration syntax. This can indicate: misplaced storage class specifiers, incorrect type syntax, missing identifier, or malformed parameter list. Verify the declaration follows Pawn's declaration syntax exactly."
},

/* 213 */  /* SEMANTIC ERROR */
{
    "tag mismatch",
    "Type compatibility violation: expression type differs from expected type in assignment, argument passing, or return context. Pawn's tag system enforces type safety for numeric types. The type checker found incompatible tags between source and destination types."
},

/* 215 */  /* WARNING */
{
    "expression has no effect",
    "Expression statement computes a value but doesn't produce side effects or store result. Examples: `a + b;` or `func();` where func returns value ignored. This often indicates: missing assignment, incorrect function call, or leftover debug expression. The side-effect analyzer detected pure expression without observable effect."
},

/* 217 */  /* WARNING */
{
    "loose indentation",
    "Inconsistent whitespace usage (spaces vs tabs, or varying indentation levels) detected. While syntactically irrelevant, inconsistent indentation impairs readability and may indicate structural misunderstandings. The lexer tracks column positions and detects abrupt indentation changes."
},

/* 234 */  /* WARNING */
{
    "Function is deprecated",
    "Function marked with deprecated attribute via `forward deprecated:` or similar. Usage triggers warning but compiles. Deprecation suggests: API evolution, security concerns, performance issues, or planned removal. Consult documentation for replacement API."
},

/* 013 */  /* SEMANTIC ERROR */
{
    "no entry point",
    "Translation unit lacks valid program entry point. Required: `main()` function or designated public function based on target environment. The linker/loader cannot determine startup address. Some environments allow alternative entry points via compiler options or specific pragmas."
},

/* 021 */  /* SEMANTIC ERROR */
{
    "symbol already defined",
    "Redeclaration of identifier in same scope violates one-definition rule. Each identifier in a given namespace must have unique declaration (except for overloading, which Pawn doesn't support). The symbol table insertion failed due to duplicate key in current scope."
},

/* 028 */  /* SEMANTIC ERROR */
{
    "invalid subscript",
    "Bracket operator applied to non-array type, or subscript expression has wrong type. Left operand must have array or pointer type, subscript must be integer expression. The type checker validates subscript expressions during expression evaluation."
},

/* 033 */  /* SEMANTIC ERROR */
{
    "array must be indexed",
    "Array identifier used in value context without subscript. In most expressions, arrays decay to pointer to first element, but certain contexts require explicit element access. This prevents accidental pointer decay when element access was intended."
},

/* 034 */  /* SEMANTIC ERROR */
{
    "argument does not have a default value",
    "Named argument syntax used with parameter that lacks default value specification. When calling with named arguments (`func(.param=value)`), all parameters without defaults must be explicitly provided. The argument binder cannot resolve missing required parameter."
},

/* 035 */  /* SEMANTIC ERROR */
{
    "argument type mismatch",
    "Actual argument type incompatible with formal parameter type. This includes: tag mismatch, array vs non-array, dimension mismatch for arrays, or value range issues. Function call type checking ensures actual arguments can be converted to formal parameter types."
},

/* 037 */  /* SEMANTIC ERROR */
{
    "invalid string",
    "String literal malformed: unterminated (missing closing quote), contains invalid escape sequences, or includes illegal characters for current codepage. The lexer's string literal parsing state machine encountered unexpected input or premature EOF."
},

/* 039 */  /* SEMANTIC ERROR */
{
    "constant symbol has no size",
    "`sizeof` operator applied to symbolic constant (enumeration constant, #define macro). `sizeof` requires type name or object expression with storage. Constants exist only at compile-time and have no runtime representation with measurable size."
},

/* 040 */  /* SEMANTIC ERROR */
{
    "duplicate case label",
    "Multiple `case` labels in same switch statement have identical constant expression values. Each case must be distinct to ensure deterministic control flow. The switch statement semantic analyzer builds case value table and detects collisions."
},

/* 041 */  /* SEMANTIC ERROR */
{
    "invalid ellipsis",
    "Array initializer with `...` (ellipsis) cannot determine appropriate array size. Ellipsis initializer requires either explicit array size or preceding explicit elements from which to extrapolate. The array initializer resolver failed to compute array dimension."
},

/* 042 */  /* SEMANTIC ERROR */
{
    "invalid combination of class specifiers",
    "Storage class specifiers combined illegally (e.g., `public static`, `forward native`). Each storage class has compatibility rules. The declaration specifier parser validates specifier combinations according to language grammar."
},

/* 043 */  /* SEMANTIC ERROR */
{
    "character constant exceeds range",
    "Character constant numeric value outside valid 0-255 range (8-bit character set). Pawn uses unsigned 8-bit characters. Integer character constants, escape sequences, or multibyte characters must fit in 8 bits for portability across all Pawn implementations."
},

/* 044 */  /* SEMANTIC ERROR */
{
    "positional parameters must precede",
    "In mixed argument passing, positional arguments appear after named arguments. Syntax requires all positional arguments first, then named arguments (`func(1, 2, .param=3)`). The parser's argument list processing enforces this ordering for unambiguous binding."
},

/* 046 */  /* SEMANTIC ERROR */
{
    "unknown array size",
    "Array declaration lacks size specifier and initializer, making size indeterminate. Array types must have known size at declaration time (except for extern incomplete arrays). The type constructor cannot create array type with unspecified bound."
},

/* 047 */  /* SEMANTIC ERROR */
{
    "array sizes do not match",
    "Array assignment between arrays of different sizes. For array assignment, source and destination must have identical size (number of elements). The type compatibility checker for assignment verifies array dimension equality."
},

/* 048 */  /* SEMANTIC ERROR */
{
    "array dimensions do not match",
    "Array operation (arithmetic, comparison) between arrays of different dimensions. Element-wise operations require identical shape. The array operation validator checks dimension compatibility before generating element-wise code."
},

/* 049 */  /* SEMANTIC ERROR */
{
    "invalid line continuation",
    "Backslash-newline sequence appears outside valid context (preprocessor directive or string literal). Line continuation only permitted in: #define macros, #include paths, and string literals. The preprocessor's line splicing logic detected illegal continuation."
},

/* 050 */  /* SEMANTIC ERROR */
{
    "invalid range",
    "Range expression (e.g., in enum or array initializer) malformed: `start .. end` where start > end, or values non-integral, or exceeds implementation limits. Range validator ensures start ≤ end and both are integer constant expressions."
},

/* 055 */  /* SEMANTIC ERROR */
{
    "start of function body without function header",
    "Compound statement `{ ... }` appears where function body expected but no preceding function declarator. This often indicates: missing function header, extra brace, or incorrect nesting. The parser's function definition production expects declarator before body."
},

/* 100 */  /* FATAL ERROR */
{
    "cannot read from file",
    "File I/O error opening or reading source file. Possible causes: file doesn't exist, insufficient permissions, path too long, file locked by another process, or disk/media error. The compiler's file layer returns system error which gets mapped to this message."
},

/* 101 */  /* FATAL ERROR */
{
    "cannot write to file",
    "Output file creation/write failure. Check: disk full, write protection, insufficient permissions, output directory doesn't exist, or file in use. The code generator's output routines failed to write compiled output."
},

/* 102 */  /* FATAL ERROR */
{
    "table overflow",
    "Internal compiler data structure capacity exceeded. Limits may include: symbol table entries, hash table chains, parse tree nodes, or string table size. Source code too large/complex for compiler's fixed-size internal tables. Consider modularization or compiler with larger limits."
},

/* 103 */  /* FATAL ERROR */
{
    "insufficient memory",
    "Dynamic memory allocation failed during compilation. The compiler's memory manager cannot satisfy allocation request due to system memory exhaustion or fragmentation. Source code may be too large, or compiler has memory leak."
},

/* 104 */  /* FATAL ERROR */
{
    "invalid assembler instruction",
    "`#emit` directive contains unrecognized or illegal opcode/operand combination. The embedded assembler validates instruction against Pawn VM instruction set. Check opcode spelling, operand count, and operand types against VM specification."
},

/* 105 */  /* FATAL ERROR */
{
    "numeric overflow",
    "Numeric constant exceeds representable range for its type. Integer constants beyond 32-bit signed range, or floating-point beyond implementation limits. The constant parser's range checking detected value outside allowed bounds."
},

/* 107 */  /* FATAL ERROR */
{
    "too many error messages on one line",
    "Error reporting threshold exceeded for single source line. Prevents infinite error cascades from severely malformed code. Compiler stops processing this line after reporting maximum errors (typically 10-20)."
},

/* 108 */  /* FATAL ERROR */
{
    "codepage mapping file not found",
    "Character set conversion mapping file specified via `-C` option missing or unreadable. The compiler needs this file for non-ASCII source character processing. Verify file exists in compiler directory or specified path."
},

/* 109 */  /* FATAL ERROR */
{
    "invalid path",
    "File system path syntax error or inaccessible directory component. Path may contain illegal characters, be too long, or refer to non-existent network resource. The compiler's path normalization routine rejected the path."
},

/* 110 */  /* FATAL ERROR */
{
    "assertion failed",
    "Compile-time assertion via `#assert` directive evaluated false. Assertion condition must be constant expression; if false, compilation aborts. Used for validating compile-time assumptions about types, sizes, or configurations."
},

/* 111 */  /* FATAL ERROR */
{
    "user error",
    "`#error` directive encountered with diagnostic message. Intentionally halts compilation with custom error message. Used for conditional compilation checks, deprecated code paths, or unmet prerequisites."
},

/* 214 */  /* WARNING */
{
    "literal array/string passed to non-const parameter",
    "String literal or array initializer passed to function parameter not declared `const`. While syntactically valid, this risks modification of literal data which may be in read-only memory. Declare parameter as `const` if function doesn't modify data, or copy literal to mutable array first."
},

/* 200 */  /* WARNING */
{
    "is truncated to",
    "Identifier exceeds implementation length limit (typically 31-63 characters). Excess characters ignored for symbol table purposes. This may cause collisions if truncated names become identical. Consider shorter, distinct names."
},

/* 201 */  /* WARNING */
{
    "redefinition of constant",
    "Macro or `const` variable redefined with different value. The new definition overrides previous. This often occurs from conflicting header files or conditional compilation issues. If intentional, use `#undef` first or check inclusion order."
},

/* 202 */  /* WARNING */
{
    "number of arguments does not match",
    "Function call argument count differs from prototype parameter count. For variadic functions, minimum count must be satisfied. The argument count checker compares call site with function signature in symbol table."
},

/* 206 */  /* WARNING */
{
    "redundant test: constant expression is non-zero",
    "Condition always true at compile-time, making conditional redundant. Similar to warning 205 but for always-true conditions. May indicate: over-constrained condition, debug code left enabled, or macro expansion issue."
},

/* 214 */  /* WARNING */
{
    "array argument was intended as const",
    "Non-constant array argument passed to const-qualified parameter triggers this advisory. The function promises not to modify via const, but caller passes non-const. This is safe but suggests interface inconsistency."
},

/* 060 */  /* PREPROCESSOR ERROR */
{
    "too many nested includes",
    "Include file nesting depth exceeds implementation limit (typically 50-100). May indicate: recursive inclusion, deep header hierarchies, or include loop. Use include guards (#ifndef) and forward declarations to reduce nesting."
},

/* 061 */  /* PREPROCESSOR ERROR */
{
    "recursive include",
    "Circular include dependency detected. File A includes B which includes A (directly or indirectly). The include graph cycle detection prevents infinite recursion. Use include guards and forward declarations to break cycles."
},

/* 062 */  /* PREPROCESSOR ERROR */
{
    "macro recursion too deep",
    "Macro expansion recursion depth limit exceeded. Occurs with recursively defined macros without termination condition. The preprocessor's macro expansion stack has safety limit to prevent infinite recursion."
},

/* 068 */  /* PREPROCESSOR ERROR */
{
    "division by zero",
    "Constant expression evaluator encountered division/modulo by zero. Check: array sizes, case labels, enumeration values, or #if expressions. The constant folder performs arithmetic during preprocessing and catches this error."
},

/* 069 */  /* PREPROCESSOR ERROR */
{
    "overflow in constant expression",
    "Arithmetic overflow during constant expression evaluation in preprocessor. Integer operations exceed 32-bit signed range. The preprocessor uses signed 32-bit arithmetic for constant expressions."
},

/* 070 */  /* PREPROCESSOR ERROR */
{
    "undefined macro",
    "Macro identifier used in `#if` or `#elif` not defined. Treated as 0 per C standard. May indicate: missing header, typo, or conditional compilation branch for undefined configuration. Use `#ifdef` or `#if defined()` to test existence."
},

/* 071 */  /* PREPROCESSOR ERROR */
{
    "missing preprocessor argument",
    "Function-like macro invoked with insufficient arguments. Each parameter in macro definition must correspond to actual argument. Check macro invocation against its definition parameter count."
},

/* 072 */  /* PREPROCESSOR ERROR */
{
    "too many macro arguments",
    "Function-like macro invoked with excess arguments beyond parameter list. Extra arguments are ignored but indicate likely error. Verify macro definition matches usage pattern."
},

/* 038 */  /* PREPROCESSOR ERROR */
{
    "extra characters on line",
    "Trailing tokens after preprocessor directive. Preprocessor directives must occupy complete logical line (except line continuation). Common with stray semicolons or comments on `#include` lines."
},

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

      if (path_exists(log_file) == 0)
        return;

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
