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
        char container_output[128];
        int format_size_c_f_container = sizeof(container_output);
        
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

        int find_pawncc = watch_sef_fdir(".", ptr_pawncc);
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
                    size_t total_len = 0;
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
                    char include_aio_path[250] = {0};
    
                    for (int i = 0; i < array_size; i++) {
                        toml_datum_t path_val = toml_string_at(include_paths, i);
                        if (path_val.ok) {
                            char __procc[250];
                            copy_strip_dotfns(__procc, sizeof(__procc), path_val.u.s);
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
    
                    static char wd_gamemode[56];
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

                        char title_compiler_info[1024 * 128];
                        snprintf(title_compiler_info, sizeof(title_compiler_info),
                            "Watchdogs | @ compile | %s | %s | %s",
                            wcfg.sef_found[0],
                            container_output, wcfg.g_output);
                        watch_title(title_compiler_info);
                        
                        clock_gettime(CLOCK_MONOTONIC, &start);
                            watch_sys(_compiler_);
                        clock_gettime(CLOCK_MONOTONIC, &end);

                        char size_err_check[128];
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
                            while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                if (strcmp(error_compiler_check, "error") == 0) {
                                    FILE *c_output;
                                    c_output = fopen(container_output, "r");
                                    if (c_output)
                                        remove(container_output);
                                    break;
                                }
                            }
                            fclose(procc_f);
                        } else {
#ifdef WD_DEBUGGING
                            printf_error("failed to open .wd_compiler.log\n");
#endif
                        }

                        compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                        printf("[Finished in %.3fs]\n", compiler_dur);
#ifdef WD_DEBUGGING
                        printf_color(COL_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                    } else {
                        char
                             __direct_p[PATH_MAX] = {0},
                             __file_n[PATH_MAX] = {0},
                             __input_p[PATH_MAX] = {0},
                             __tmp_a[PATH_MAX] = {0};
                        
                        strncpy(__tmp_a, compile_args, sizeof(__tmp_a) - 1);
                        __tmp_a[sizeof(__tmp_a) - 1] = '\0';

                        char *__l_slash = strrchr(__tmp_a, '/'),
                             *__l_back = strrchr(__tmp_a, '\\');
                        if (__l_back && (!__l_slash || __l_back > __l_slash))
                            __l_slash = __l_back;

                        if (__l_slash) {
                            size_t dir_len = (size_t)(__l_slash - __tmp_a);
                            if (dir_len >= sizeof(__direct_p)) 
                                dir_len = sizeof(__direct_p) - 1;
                            memcpy(__direct_p, __tmp_a, dir_len);
                            __direct_p[dir_len] = '\0';

                            const char *filename_start = __l_slash + 1;
                            size_t filename_len = strlen(filename_start);
                            if (filename_len >= sizeof(__file_n))
                                filename_len = sizeof(__file_n) - 1;
                            memcpy(__file_n, filename_start, filename_len);
                            __file_n[filename_len] = '\0';

                            size_t total_needed = strlen(__direct_p) + 1 + strlen(__file_n) + 1;
                            if (total_needed > sizeof(__input_p)) {
                                strncpy(__direct_p, "gamemodes", sizeof(__direct_p) - 1);
                                __direct_p[sizeof(__direct_p) - 1] = '\0';
                                
                                size_t max_filename = sizeof(__file_n) - 1;
                                if (filename_len > max_filename) {
                                    memcpy(__file_n, filename_start, max_filename);
                                    __file_n[max_filename] = '\0';
                                }
                            }
                            
                            if (snprintf(__input_p, sizeof(__input_p), "%s/%s", __direct_p, __file_n) >= (int)sizeof(__input_p)) {
                                __input_p[sizeof(__input_p) - 1] = '\0';
                            }
                        } else {
                            strncpy(__file_n, __tmp_a, sizeof(__file_n) - 1);
                            __file_n[sizeof(__file_n) - 1] = '\0';

                            strncpy(__direct_p, ".", sizeof(__direct_p) - 1);
                            __direct_p[sizeof(__direct_p) - 1] = '\0';

                            if (snprintf(__input_p, sizeof(__input_p), "./%s", __file_n) >= (int)sizeof(__input_p)) {
                                __input_p[sizeof(__input_p) - 1] = '\0';
                            }
                        }

                        int find_gamemodes_arg1 = 0;
                        find_gamemodes_arg1 = watch_sef_fdir(__direct_p, __file_n);

                        if (!find_gamemodes_arg1 && strcmp(__direct_p, "gamemodes") != 0) {
                            find_gamemodes_arg1 = watch_sef_fdir("gamemodes", __file_n);
                            if (find_gamemodes_arg1) {
                                strncpy(__direct_p, "gamemodes", sizeof(__direct_p) - 1);
                                __direct_p[sizeof(__direct_p) - 1] = '\0';
                                
                                if (snprintf(__input_p, sizeof(__input_p), "gamemodes/%s", __file_n) >= (int)sizeof(__input_p)) {
                                    __input_p[sizeof(__input_p) - 1] = '\0';
                                }
                                
                                if (wcfg.sef_count > 0) {
                                    strncpy(wcfg.sef_found[wcfg.sef_count-1], __input_p, SEF_PATH_SIZE);
                                }
                            }
                        }

                        if (!find_gamemodes_arg1 && strcmp(__direct_p, ".") == 0) {
                            find_gamemodes_arg1 = watch_sef_fdir("gamemodes", __file_n);
                            if (find_gamemodes_arg1) {
                                strncpy(__direct_p, "gamemodes", sizeof(__direct_p) - 1);
                                __direct_p[sizeof(__direct_p) - 1] = '\0';
                                
                                if (snprintf(__input_p, sizeof(__input_p), "gamemodes/%s", __file_n) >= (int)sizeof(__input_p)) {
                                    __input_p[sizeof(__input_p) - 1] = '\0';
                                }
                                
                                if (wcfg.sef_count > 0) {
                                    strncpy(wcfg.sef_found[wcfg.sef_count-1], __input_p, SEF_PATH_SIZE);
                                }
                            }
                        }

                        if (find_gamemodes_arg1) {
                            char container_output[PATH_MAX];
                            
                            char i_path_rm[PATH_MAX];
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

                            if (ret < 0 || (size_t)ret >= (size_t)format_size_compiler) {
                                fprintf(stderr, "[Error] snprintf() failed or buffer too small (needed %d bytes)\n", ret);
                            }

                            char title_compiler_info[1024 * 128];
                            snprintf(title_compiler_info, sizeof(title_compiler_info), 
                                    "Watchdogs | @ compile | %s | %s | %s.amx", 
                                    wcfg.sef_found[0],
                                    wcfg.sef_found[1],
                                    container_output);
                            watch_title(title_compiler_info);
                            
                            clock_gettime(CLOCK_MONOTONIC, &start);
                                watch_sys(_compiler_);
                            clock_gettime(CLOCK_MONOTONIC, &end);
                            
                            char size_err_check[128];
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
                                while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                    if (strcmp(error_compiler_check, "error") == 0) {
                                        FILE *c_output;
                                        c_output = fopen(container_output, "r");
                                        if (c_output)
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

                    signal(SIGINT, __init);

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