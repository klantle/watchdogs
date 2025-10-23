#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <readline/readline.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "utils.h"
#include "chain.h"
#include "package.h"
#include "compiler.h"

int
    watch_compiler_error_level = 0;

int watch_compilers(const char *arg, const char *compile_args)
{
        const char *ptr_pawncc;
        if (__os__ == 0x01) 
            ptr_pawncc = "pawncc.exe";
        else if (__os__ == 0x00)
            ptr_pawncc = "pawncc";

        if (access(".wd_compiler.log", F_OK) == 0) {
            remove(".wd_compiler.log");
        }
        
        FILE *procc_f = NULL;
        char error_compiler_check[100];
        char container_output[1024 + 50] = {0};
        
        procc_f = fopen(".wd_compiler.log", "r");
        if (procc_f) {
            int ch;
            while ((ch = fgetc(procc_f)) != EOF) putchar(ch);
            rewind(procc_f);
            while (fscanf(procc_f, "%s", error_compiler_check) != EOF) {
                if (strcmp(error_compiler_check, "error") == 0) {
                    if (access(container_output, F_OK) == 0)
                        remove(container_output);
                    break;
                }
            }
            fclose(procc_f);
        }

        int find_pawncc = watch_sef_fdir(".", ptr_pawncc, NULL);
        if (find_pawncc) {
            size_t format_size_compiler = PATH_MAX;
            char *_compiler_ = malloc(format_size_compiler);
            if (!_compiler_) {
#ifdef WD_DEBUGGING
                printf_error("memory allocation failed for _compiler_!\n");
#endif
                return 0;
            }

            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (!procc_f) {
                printf_error("can't read file %s\n", "watchdogs.toml");
                if (_compiler_) { free(_compiler_); }
                return 0;
            }
            
            char errbuf[256];
            toml_table_t *config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
            fclose(procc_f);
    
            if (!config) {
                printf_error("parsing TOML: %s\n", errbuf);    
                if (_compiler_) { free(_compiler_); }
                return 0;
            }
            
            toml_table_t *watch_compiler = toml_table_in(config, "compiler");
            if (watch_compiler) {
                toml_array_t *option_arr = toml_array_in(watch_compiler, "option");
                if (option_arr) {
                    size_t arr_sz = toml_array_nelem(option_arr);
                    char *merged = NULL;

                    for (size_t i = 0; i < arr_sz; i++) {
                        toml_datum_t val = toml_string_at(option_arr, i);
                        if (!val.ok) continue;

                        size_t old_len = merged ? strlen(merged) : 0;
                        size_t new_len = old_len + strlen(val.u.s) + 2;

                        char *tmp = realloc(merged, new_len);
                        if (!tmp) {
                            free(merged);
                            free(val.u.s);
                            merged = NULL;
                            break;
                        }
                        merged = tmp;

                        if (old_len == 0)
                            snprintf(merged, new_len, "%s", val.u.s);
                        else
                            snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);

                        free(val.u.s);
                    }

                    if (merged) {
                        wcfg.ci_options = merged;
                    } else {
                        wcfg.ci_options = strdup("");
                    }
                }
            
                toml_array_t *include_paths = toml_array_in(watch_compiler, "include_path");
                if (include_paths) {
                    int array_size = toml_array_nelem(include_paths);
                    char include_aio_path[526] = {0};
    
                    for (int i = 0; i < array_size; i++) {
                        toml_datum_t path_val = toml_string_at(include_paths, i);
                        if (path_val.ok) {
                            char __procc[250];
                            cp_strip_dotfns(__procc, sizeof(__procc), path_val.u.s);
                            if (__procc[0] == '\0') continue;
                            if (i > 0) {
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1) {
                                    snprintf(include_aio_path + cur,
                                                sizeof(include_aio_path) - cur,
                                                " ");
                                }
                            }
                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1) {
                                snprintf(include_aio_path + cur,
                                            sizeof(include_aio_path) - cur,
                                            "-i\"%s\"",
                                            __procc);
                            }
                            free(path_val.u.s);
                        }
                    }
    
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        toml_datum_t toml_g_i = toml_string_in(watch_compiler, "input");
                        if (toml_g_i.ok) {
                            wcfg.gm_input = strdup(toml_g_i.u.s);
                            free(toml_g_i.u.s);
                        }
                        toml_datum_t toml_gm_o = toml_string_in(watch_compiler, "output");
                        if (toml_gm_o.ok) {
                            wcfg.g_output = strdup(toml_gm_o.u.s);
                            free(toml_gm_o.u.s);
                        }
                        
                        struct timespec start, end;
                        double compiler_dur;

                        char
                            *path_include = NULL;
                        if (find_for_samp == 0x1) path_include="pawno/include";
                        else if (find_for_omp == 0x1) path_include="qawno/include";

#ifdef _WIN32
                        int ret = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "%s %s -o%s %s %s -i%s > .wd_compiler.log 2>&1",
                            wcfg.sef_found[0],                              // compiler binary
                            wcfg.gm_input,                                  // input file
                            wcfg.g_output,                                  // output file
                            wcfg.ci_options,                                // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#else
                        int ret = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "\"%s\" \"%s\" -o\"%s\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                            wcfg.sef_found[0],                              // compiler binary
                            wcfg.gm_input,                                  // input file
                            wcfg.g_output,                                  // output file
                            wcfg.ci_options,                                // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#endif

                        if (ret < 0 || (size_t)ret >= (size_t)format_size_compiler) {
                            fprintf(stderr, "[Error] snprintf() failed or buffer too small (needed %d bytes)\n", ret);
                        }
                        
                        size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s", wcfg.sef_found[0], container_output, wcfg.g_output) + 1;
                        char *title_compiler_info = malloc(needed);
                        if (!title_compiler_info) { return 1; }
                        snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s", wcfg.sef_found[0], container_output, wcfg.g_output);
                        if (title_compiler_info) {
                            watch_title(title_compiler_info);
                            free(title_compiler_info);
                        }

                        clock_gettime(CLOCK_MONOTONIC, &start);
                            watch_sys(_compiler_);
                        clock_gettime(CLOCK_MONOTONIC, &end);

                        char size_err_check[1024];
                        snprintf(size_err_check, sizeof(size_err_check), "%s", wcfg.g_output);
                        if (access(size_err_check, F_OK) == 0) {
                            watch_compiler_error_level = 0;
                        } else {
                            watch_compiler_error_level = 1;
                        }

                        procc_f = fopen(".wd_compiler.log", "r");
                        if (procc_f) {
                            int ch;
                            while ((ch = fgetc(procc_f)) != EOF)
                                putchar(ch);
                            rewind(procc_f);
                            char line[512];
                            while (fgets(line, sizeof(line), procc_f) != NULL) {
                                if (strstr(line, "error")) {
                                    if (container_output[0] != '\0' &&
                                        access(container_output, F_OK) == 0)
                                        remove(container_output);
                                    break;
                                }
                            }
                            fclose(procc_f);
                        } else {
                            printf_error("failed to open .wd_compiler.log\n");
                        }

                        compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                        printf("[Finished in %.3fs]\n", compiler_dur);
#ifdef WD_DEBUGGING
                        printf_color(COL_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                    } else {
                        char
                            __cp_direct_path[PATH_MAX] = {0}, __cp_file_name[PATH_MAX] = {0},
                            __cp_input_path[PATH_MAX] = {0}, __cp_any_tmp[PATH_MAX] = {0};
                        strncpy(__cp_any_tmp, compile_args, sizeof(__cp_any_tmp) - 1);
                        __cp_any_tmp[sizeof(__cp_any_tmp) - 1] = '\0';

                        char *compiler_last_slash = strrchr(__cp_any_tmp, '/'),
                             *compiler_back_slash = strrchr(__cp_any_tmp, '\\');
                        if (compiler_back_slash && (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                            compiler_last_slash = compiler_back_slash;

                        if (compiler_last_slash) {
                            size_t dir_len = (size_t)(compiler_last_slash - __cp_any_tmp);
                            if (dir_len >= sizeof(__cp_direct_path)) 
                                dir_len = sizeof(__cp_direct_path) - 1;

                            memcpy(__cp_direct_path, __cp_any_tmp, dir_len);
                            __cp_direct_path[dir_len] = '\0';

                            const char *__cp_filename_start = compiler_last_slash + 1;
                            size_t __cp_filename_len = strlen(__cp_filename_start);
                            if (__cp_filename_len >= sizeof(__cp_file_name))
                                __cp_filename_len = sizeof(__cp_file_name) - 1;

                            memcpy(__cp_file_name, __cp_filename_start, __cp_filename_len);
                            __cp_file_name[__cp_filename_len] = '\0';

                            size_t total_needed = strlen(__cp_direct_path) + 1 + strlen(__cp_file_name) + 1;
                            if (total_needed > sizeof(__cp_input_path)) {
                                strncpy(__cp_direct_path, "gamemodes", sizeof(__cp_direct_path) - 1);
                                __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';
                                
                                size_t __cp_max_file_name = sizeof(__cp_file_name) - 1;
                                if (__cp_filename_len > __cp_max_file_name) {
                                    memcpy(__cp_file_name, __cp_filename_start, __cp_max_file_name);
                                    __cp_file_name[__cp_max_file_name] = '\0';
                                }
                            }
                            
                            if (snprintf(__cp_input_path, sizeof(__cp_input_path), "%s/%s", __cp_direct_path, __cp_file_name) >= (int)sizeof(__cp_input_path)) {
                                __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                            }
                        } else {
                            strncpy(__cp_file_name, __cp_any_tmp, sizeof(__cp_file_name) - 1);
                            __cp_file_name[sizeof(__cp_file_name) - 1] = '\0';

                            strncpy(__cp_direct_path, ".", sizeof(__cp_direct_path) - 1);
                            __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';

                            if (snprintf(__cp_input_path, sizeof(__cp_input_path), "./%s", __cp_file_name) >= (int)sizeof(__cp_input_path)) {
                                __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                            }
                        }

                        int find_gamemodes_arg1 = 0;
                        find_gamemodes_arg1 = watch_sef_fdir(__cp_direct_path, __cp_file_name, NULL);

                        if (!find_gamemodes_arg1 && strcmp(__cp_direct_path, "gamemodes") != 0) {
                            find_gamemodes_arg1 = watch_sef_fdir("gamemodes", __cp_file_name, NULL);
                            if (find_gamemodes_arg1) {
                                strncpy(__cp_direct_path, "gamemodes", sizeof(__cp_direct_path) - 1);
                                __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';
                                
                                if (snprintf(__cp_input_path, sizeof(__cp_input_path), "gamemodes/%s", __cp_file_name) >= (int)sizeof(__cp_input_path)) {
                                    __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                                }
                                
                                if (wcfg.sef_count > 0) {
                                    strncpy(wcfg.sef_found[wcfg.sef_count-1], __cp_input_path, SEF_PATH_SIZE);
                                }
                            }
                        }

                        if (!find_gamemodes_arg1 && strcmp(__cp_direct_path, ".") == 0) {
                            find_gamemodes_arg1 = watch_sef_fdir("gamemodes", __cp_file_name, NULL);
                            if (find_gamemodes_arg1) {
                                strncpy(__cp_direct_path, "gamemodes", sizeof(__cp_direct_path) - 1);
                                __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';
                                
                                if (snprintf(__cp_input_path, sizeof(__cp_input_path), "gamemodes/%s", __cp_file_name) >= (int)sizeof(__cp_input_path)) {
                                    __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                                }
                                
                                if (wcfg.sef_count > 0) {
                                    strncpy(wcfg.sef_found[wcfg.sef_count-1], __cp_input_path, SEF_PATH_SIZE);
                                }
                            }
                        }

                        if (find_gamemodes_arg1) {
                            char i_path_rm[1024];
                            snprintf(i_path_rm, sizeof(i_path_rm), "%s", wcfg.sef_found[1]);
                            
                            char *f_EXT = strrchr(i_path_rm, '.');
                            if (f_EXT && strcmp(f_EXT, ".pwn") == 0)
                                *f_EXT = '\0';

                            snprintf(container_output, sizeof(container_output), "%s", i_path_rm);

                            struct timespec start, end;
                            double compiler_dur;

                            char *path_include = NULL;
                            if (find_for_samp == 0x1) path_include="pawno/include";
                            else if (find_for_omp == 0x1) path_include="qawno/include";

#ifdef _WIN32
                            int ret = snprintf(
                                _compiler_,
                                format_size_compiler,
                                "%s %s -o%s.amx %s %s -i%s > .wd_compiler.log 2>&1",
                                wcfg.sef_found[0],                              // compiler binary
                                wcfg.sef_found[1],                              // input file
                                container_output,                               // output file
                                wcfg.ci_options,                                // additional options
                                include_aio_path,                               // include search path
                                path_include                                    // include directory
                            );
#else
                            int ret = snprintf(
                                _compiler_,
                                format_size_compiler,
                                "\"%s\" \"%s\" -o\"%s.amx\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                                wcfg.sef_found[0],                              // compiler binary
                                wcfg.sef_found[1],                              // input file
                                container_output,                               // output file
                                wcfg.ci_options,                                // additional options
                                include_aio_path,                               // include search path
                                path_include                                    // include directory
                            );
#endif

                            if (ret < 0 || (size_t)ret >= (size_t)format_size_compiler) {
                                fprintf(stderr, "[Error] snprintf() failed or buffer too small (needed %d bytes)\n", ret);
                            }

                            size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s.amx", wcfg.sef_found[0], wcfg.sef_found[1], container_output) + 1;
                            char *title_compiler_info = malloc(needed);
                            if (!title_compiler_info) { return 1; }
                            snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s.amx", wcfg.sef_found[0], wcfg.sef_found[1], container_output);
                            if (title_compiler_info) {
                                watch_title(title_compiler_info);
                                free(title_compiler_info);
                            }

                            clock_gettime(CLOCK_MONOTONIC, &start);
                                watch_sys(_compiler_);
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            
                            char size_err_check[1024 + 64];
                            snprintf(size_err_check, sizeof(size_err_check), "%s.amx", container_output);
                            if (access(size_err_check, F_OK) == 0) {
                                watch_compiler_error_level = 0;
                            } else {
                                watch_compiler_error_level = 1;
                            }
                            
                            procc_f = fopen(".wd_compiler.log", "r");
                            if (procc_f) {
                                int ch;
                                while ((ch = fgetc(procc_f)) != EOF)
                                    putchar(ch);
                                rewind(procc_f);
                                char line[512];
                                while (fgets(line, sizeof(line), procc_f) != NULL) {
                                    if (strstr(line, "error")) {
                                        if (container_output[0] != '\0' &&
                                            access(container_output, F_OK) == 0)
                                            remove(container_output);
                                        break;
                                    }
                                }
                                fclose(procc_f);
                            } else {
                                printf_error("failed to open .wd_compiler.log\n");
                            }

                            compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                            printf("[Finished in %.3fs]\n", compiler_dur);
#ifdef WD_DEBUGGING
                            printf_color(COL_YELLOW, "-DEBUGGING\n");
                            printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                        } else {
                            printf_color(COL_RED, "Can't locate: ");
                            printf("%s\n", compile_args);
                            return 0;
                        }
                    }
                }
                toml_free(config);
                if (_compiler_) { free(_compiler_); }
                return 0;
            }
        } else {
            printf_error("pawncc not found!");
    
            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");
    
            while (1) {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                    static
                        char platform = 0;
ret_pcc_2:
                        println("Select platform:");
                        println("[L/l] Linux");
                        println("[W/w] Windows");
                        println("[T/t] Termux");
                        printf(">> ");

                    if (scanf(" %c", &platform) != 1)
                        return 0;

                    signal(SIGINT, ___main___);

                    if (platform == 'L' || platform == 'l')
                        watch_pawncc("linux");
                    else if (platform == 'W' || platform == 'w')
                        watch_pawncc("windows");
                    else if (platform == 'T' || platform == 't')
                        watch_pawncc("termux");
                    else {
                        printf("Invalid platform selection. use C^ to exit.\n");
                        goto ret_pcc_2;
                    }
                    break;
                } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                    break;
                } else {
                    printf("Invalid input. Please type Y/y to install or N/n to cancel.\n");
                    if (ptr_sigA) { free(ptr_sigA); }
                    ptr_sigA = readline("install now? [Y/n]: ");
                }
            }

            if (ptr_sigA) { free(ptr_sigA); }
        }

        return 1;
}