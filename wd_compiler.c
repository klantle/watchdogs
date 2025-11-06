#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_util.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_compiler.h"

int wd_run_compiler(const char *arg, const char *compile_args)
{
        wcfg.wd_compiler_stat = 0;

        const char *ptr_pawncc = NULL;
        if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS))
            ptr_pawncc = "pawncc.exe";
        else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX))
            ptr_pawncc = "pawncc";

        char *path_include = NULL;
        if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
            path_include="pawno/include";
        else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
            path_include="qawno/include";

        int _wd_log_acces = path_acces(".wd_compiler.log");
        if (_wd_log_acces)
            remove(".wd_compiler.log");

        char container_output[PATH_MAX] = { 0 };
        char __wcp_direct_path[PATH_MAX] = { 0 };
        char __wcp_file_name[PATH_MAX] = { 0 };
        char __wcp_input_path[PATH_MAX] = { 0 };
        char __wcp_any_tmp[PATH_MAX] = { 0 };

        wd_sef_fdir_reset();

        int __find_pawncc = wd_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc)
        {
            size_t format_size_compiler = 1024;
            char *_compiler_ = wd_malloc(format_size_compiler);
            if (!_compiler_)
            {
#if defined(_DBG_PRINT)
                pr_error(stdout, "memory allocation failed for _compiler_!");
#endif
                return __RETZ;
            }

            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (!procc_f)
            {
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
                if (_compiler_)
                {
                    wd_free(_compiler_);
                    _compiler_ = NULL;
                }
                return __RETZ;
            }

            char errbuf[256];
            toml_table_t *_toml_config;
            _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));

            if (procc_f) {
                fclose(procc_f);
            }

            if (!_toml_config)
            {
                pr_error(stdout, "parsing TOML: %s", errbuf);
                if (_compiler_)
                {
                    wd_free(_compiler_);
                    _compiler_ = NULL;
                }
                return __RETZ;
            }

            toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
            if (wd_compiler)
            {
                toml_array_t *option_arr = toml_array_in(wd_compiler, "option");
                if (option_arr)
                {
                    size_t arr_sz = toml_array_nelem(option_arr);
                    char *merged = NULL;

                    for (size_t i = 0; i < arr_sz; i++)
                    {
                        toml_datum_t val = toml_string_at(option_arr, i);
                        if (!val.ok)
                            continue;

                        size_t old_len = merged ? strlen(merged) : 0,
                               new_len = old_len + strlen(val.u.s) + 2;

                        char *tmp = wd_realloc(merged, new_len);
                        if (!tmp)
                        {
                            wd_free(merged);
                            wd_free(val.u.s);
                            merged = NULL;
                            break;
                        }
                        merged = tmp;

                        if (!old_len)
                            snprintf(merged, new_len, "%s", val.u.s);
                        else
                            snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);

                        wd_free(val.u.s);
                        val.u.s = NULL;
                    }

                    if (merged) {
                        wcfg.wd_toml_aio_opt = merged;
                    }
                    else {
                        wcfg.wd_toml_aio_opt = strdup("");
                    }
                }

                char include_aio_path[PATH_MAX] = { 0 };
                toml_array_t *include_paths = toml_array_in(wd_compiler, "include_path");
                if (include_paths)
                {
                    int array_size = toml_array_nelem(include_paths);

                    for (int i = 0; i < array_size; i++)
                    {
                        toml_datum_t path_val = toml_string_at(include_paths, i);
                        if (path_val.ok)
                        {
                            char __procc[250];
                            wd_strip_dot_fns(__procc, sizeof(__procc), path_val.u.s);
                            if (__procc[0] == '\0')
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
                                            "-i\"%s\"",
                                            __procc);
                            }
                            wd_free(path_val.u.s);
                        }
                    }
                }

                if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0'))
                {
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

#ifdef _WIN32
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "%s %s -o%s %s %s -i%s > .wd_compiler.log 2>&1",
                        wcfg.wd_sef_found_list[0],                      // compiler binary
                        wcfg.wd_toml_gm_input,                          // input file
                        wcfg.wd_toml_gm_output,                         // output file
                        wcfg.wd_toml_aio_opt,                           // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#else
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "\"%s\" \"%s\" -o\"%s\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                        wcfg.wd_sef_found_list[0],                      // compiler binary
                        wcfg.wd_toml_gm_input,                          // input file
                        wcfg.wd_toml_gm_output,                         // output file
                        wcfg.wd_toml_aio_opt,                           // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#endif

                    if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                    {
                        pr_error(stdout, "snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                    }

                    size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s",
                                                      wcfg.wd_sef_found_list[0],
                                                      wcfg.wd_toml_gm_input,
                                                      wcfg.wd_toml_gm_output) + 1;
                    char *title_compiler_info = wd_malloc(needed);
                    if (!title_compiler_info)
                    {
                        return __RETN;
                    }
                    snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s",
                                                          wcfg.wd_sef_found_list[0],
                                                          wcfg.wd_toml_gm_input,
                                                          wcfg.wd_toml_gm_output);
                    if (title_compiler_info)
                    {
                        wd_set_title(title_compiler_info);
                        wd_free(title_compiler_info);
                        title_compiler_info = NULL;
                    }

                    struct timespec start, end;
                    double compiler_dur;

                    clock_gettime(CLOCK_MONOTONIC, &start);
                        wd_run_command(_compiler_);
                    clock_gettime(CLOCK_MONOTONIC, &end);

                    if (procc_f) {
                        print_file_with_annotations(".wd_compiler.log");
                    }

                    char _container_output[PATH_MAX + 128];
                    snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);

                    procc_f = fopen(".wd_compiler.log", "r");
                    if (procc_f)
                    {
                        char line_buf[1024 + 1024];
                        int __has_error = 0;
                        while (fgets(line_buf, sizeof(line_buf), procc_f))
                        {
                            if (strstr(line_buf, "error") || strstr(line_buf, "Error"))
                                __has_error = 1;
                        }
                        fclose(procc_f);
                        if (__has_error)
                        {
                            if (_container_output[0] != '\0' && path_acces(_container_output))
                                remove(_container_output);
                            wcfg.wd_compiler_stat = 1;
                        }
                    }
                    else {
                        pr_error(stdout, "Failed to open .wd_compiler.log");
                    }

                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    pr_color(stdout, FCOLOUR_GREEN,
                        " ==> [P]Finished in %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                }
                else
                {
                    strncpy(__wcp_any_tmp, compile_args, sizeof(__wcp_any_tmp) - 1);
                    __wcp_any_tmp[sizeof(__wcp_any_tmp) - 1] = '\0';

                    char *compiler_last_slash = NULL;
                    compiler_last_slash = strrchr(__wcp_any_tmp, '/');

                    char *compiler_back_slash = NULL;
                    compiler_back_slash = strrchr(__wcp_any_tmp, '\\');

                    if (compiler_back_slash &&
                       (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                    {
                        compiler_last_slash = compiler_back_slash;
                    }

                    if (compiler_last_slash)
                    {
                        size_t __dir_len = (size_t)(compiler_last_slash - __wcp_any_tmp);

                        if (__dir_len >= sizeof(__wcp_direct_path))
                        {
                            __dir_len = sizeof(__wcp_direct_path) - 1;
                        }

                        memcpy(__wcp_direct_path, __wcp_any_tmp, __dir_len);
                        __wcp_direct_path[__dir_len] = '\0';

                        const char *__wcp_filename_start = compiler_last_slash + 1;
                        size_t __wcp_filename_len = strlen(__wcp_filename_start);

                        if (__wcp_filename_len >= sizeof(__wcp_file_name))
                        {
                            __wcp_filename_len = sizeof(__wcp_file_name) - 1;
                        }

                        memcpy(__wcp_file_name, __wcp_filename_start, __wcp_filename_len);
                        __wcp_file_name[__wcp_filename_len] = '\0';

                        size_t total_needed = strlen(__wcp_direct_path) + 1 +
                                    strlen(__wcp_file_name) + 1;

                        if (total_needed > sizeof(__wcp_input_path))
                        {
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            size_t __wcp_max_file_name = sizeof(__wcp_file_name) - 1;

                            if (__wcp_filename_len > __wcp_max_file_name)
                            {
                                memcpy(__wcp_file_name, __wcp_filename_start,
                                    __wcp_max_file_name);
                                __wcp_file_name[__wcp_max_file_name] = '\0';
                            }
                        }

                        if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                "%s/%s", __wcp_direct_path, __wcp_file_name) >=
                            (int)sizeof(__wcp_input_path))
                        {
                            __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                        }

                    }
                    else
                    {
                        strncpy(__wcp_file_name, __wcp_any_tmp,
                            sizeof(__wcp_file_name) - 1);
                        __wcp_file_name[sizeof(__wcp_file_name) - 1] = '\0';

                        strncpy(__wcp_direct_path, ".", sizeof(__wcp_direct_path) - 1);
                        __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                        if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                "./%s", __wcp_file_name) >=
                            (int)sizeof(__wcp_input_path))
                        {
                            __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                        }
                    }

                    int __find_gamemodes_args = 0;
                    __find_gamemodes_args = wd_sef_fdir(__wcp_direct_path,
                                        __wcp_file_name, NULL);

                    if (!__find_gamemodes_args &&
                        strcmp(__wcp_direct_path, "gamemodes") != 0)
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (__find_gamemodes_args)
                        {
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            if (wcfg.wd_sef_count > 0)
                            {
                                strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    if (!__find_gamemodes_args &&
                        strcmp(__wcp_direct_path, ".") == 0)
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (__find_gamemodes_args)
                        {
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            if (wcfg.wd_sef_count > 0)
                            {
                                strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    if (__find_gamemodes_args)
                    {
                        char __sef_path_sz[PATH_MAX];
                        snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.wd_sef_found_list[1]);
                        char *f_EXT = strrchr(__sef_path_sz, '.');
                        if (f_EXT)
                        {
                            *f_EXT = '\0';
                        }

                        snprintf(container_output, sizeof(container_output), "%s", __sef_path_sz);

#ifdef _WIN32
                        int ret_compiler = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "%s %s -o%s.amx %s %s -i%s > .wd_compiler.log 2>&1",
                            wcfg.wd_sef_found_list[0],                      // compiler binary
                            wcfg.wd_sef_found_list[1],                      // input file
                            container_output,                               // output file
                            wcfg.wd_toml_aio_opt,                           // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#else
                        int ret_compiler = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "\"%s\" \"%s\" -o\"%s.amx\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                            wcfg.wd_sef_found_list[0],                      // compiler binary
                            wcfg.wd_sef_found_list[1],                      // input file
                            container_output,                               // output file
                            wcfg.wd_toml_aio_opt,                           // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#endif

                        if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                        {
                            pr_error(stdout, "snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                        }

                        size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                          wcfg.wd_sef_found_list[0],
                                                          wcfg.wd_sef_found_list[1],
                                                          container_output) + 1;
                        char *title_compiler_info = wd_malloc(needed);
                        if (!title_compiler_info)
                        {
                            return __RETN;
                        }
                        snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                              wcfg.wd_sef_found_list[0],
                                                              wcfg.wd_sef_found_list[1],
                                                              container_output);
                        if (title_compiler_info)
                        {
                            wd_set_title(title_compiler_info);
                            wd_free(title_compiler_info);
                            title_compiler_info = NULL;
                        }

                        struct timespec start, end;
                        double compiler_dur;

                        clock_gettime(CLOCK_MONOTONIC, &start);
                            wd_run_command(_compiler_);
                        clock_gettime(CLOCK_MONOTONIC, &end);

                        if (procc_f) {
                            print_file_with_annotations(".wd_compiler.log");
                        }

                        char _container_output[PATH_MAX + 128];
                        snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);

                        procc_f = fopen(".wd_compiler.log", "r");
                        if (procc_f)
                        {
                            char line_buf[1024 + 1024];
                            int __has_error = 0;
                            while (fgets(line_buf, sizeof(line_buf), procc_f))
                            {
                                if (strstr(line_buf, "error") || strstr(line_buf, "Error"))
                                    __has_error = 1;
                            }
                            fclose(procc_f);
                            if (__has_error)
                            {
                                if (_container_output[0] != '\0' && path_acces(_container_output))
                                    remove(_container_output);
                                wcfg.wd_compiler_stat = 1;
                            }
                        }
                        else {
                            pr_error(stdout, "Failed to open .wd_compiler.log");
                        }

                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        pr_color(stdout, FCOLOUR_GREEN,
                            " ==> [P]Finished in %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                    }
                    else
                    {
                        pr_error(stdout, "Cannnot locate:");
                        printf("\t%s\n", compile_args);
                        return __RETZ;
                    }
                }

                toml_free(_toml_config);
                if (_compiler_)
                {
                    wd_free(_compiler_);
                    _compiler_ = NULL;
                }

                return __RETZ;
            }
        }
        else
        {
            pr_crit(stdout, "pawncc not found!");

            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");

            while (1)
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
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc;
                    }
                    if (strfind(platform, "W"))
                    {
                        int ret = wd_install_pawncc("windows");
loop_ipcc2:
                        wd_free(platform);
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc2;
                    }
                    if (strfind(platform, "T"))
                    {
                        int ret = wd_install_pawncc("termux");
loop_ipcc3:
                        wd_free(platform);
                        if (ret == -__RETN && wcfg.wd_sel_stat != 0)
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
                    wd_main(NULL);
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

        return __RETN;
}
