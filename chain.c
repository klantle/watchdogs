#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif 

#if defined(_DBG_PRINT)
#define WD_DEBUGGING
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <unistd.h>
#define PATH_SEP "/"
#endif

#include <limits.h>
#include <ncursesw/curses.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <ftw.h>
#include <sys/file.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "utils.h"
#include "hardware.h"
#include "crypto.h"
#include "package.h"
#include "archive.h"
#include "curl.h"
#include "chain.h"
#include "server.h"

static int
    __watchdogs_os__;
static const char
    *ptr_samp = NULL;
static int
    find_for_samp = 0x0;
static const char
    *ptr_openmp = NULL;
static int
    find_for_omp = 0x0;
    
void __init_function(void) {
        watchdogs_toml_data();
        watchdogs_u_history();
        watchdogs_reset_var();
        reset_watchdogs_sef_dir();

        __watchdogs_os__ = signal_system_os();
        
        if (__watchdogs_os__ == 0x01) {
            ptr_samp="samp-server.exe"; ptr_openmp="omp-server.exe";
        }
        else if (__watchdogs_os__ == 0x00) {
            ptr_samp="samp03svr"; ptr_openmp="omp-server";
        }
        
        FILE *file_s = fopen(ptr_samp, "r");
        if (file_s) {
            find_for_samp = 0x1;
            fclose(file_s);
        }
        FILE *file_m = fopen(ptr_openmp, "r");
        if (file_m) {
            find_for_omp = 0x1;
            fclose(file_m);
        }
#ifdef WD_DEBUGGING
        printf_color(COL_YELLOW, "-DEBUGGING\n");
        printf("[__init_function]:\n\t__watchdogs_os__: 0x0%d\n\tptr_samp: %s\n\tptr_openmp: %s\n", __watchdogs_os__, ptr_samp, ptr_openmp);
#endif
}

int __init_wd(void)
{
        __init_function();
        char *ptr_command = readline("watchdogs > ");
        if (!ptr_command) return -1;
        watchdogs_a_history(ptr_command);

        int c_distance = INT_MAX;
        const char *_dist_command = find_cl_command(ptr_command,
                                                    __command,
                                                    __command_len,
                                                    &c_distance);

_reexecute_command:
        if (strncmp(ptr_command, "help", 4) == 0) {
            watchdogs_title("Watchdogs | @ help");

            static char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') arg++;

            if (strlen(arg) == 0) {
                println("Usage: help | help [<command>]");
                println("command:");
                println(" clear, exit, kill, title");
                println(" gamemode, pawncc");
                println(" compile, running, debug");
                println(" stop, restart, hardware");
            } else if (strcmp(arg, "exit") == 0) { println("exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "clear") == 0) { println("clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "kill") == 0) { println("kill: kill - restart terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "hardware") == 0) { println("hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "title") == 0) { println("title: set-title Terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "compile") == 0) { println("compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println("running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "debug") == 0) { println("debug: debugging your project. | Usage: \"debug\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println("stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println("restart: restart server task. | Usage: \"restart\"");
            } else println("help not found for: '%s'", arg);
            
            return 0;
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            watchdogs_title("Watchdogs | @ pawncc");
            static
                char platform = 0;
ret_pcc:
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
                goto ret_pcc;
            }
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            watchdogs_title("Watchdogs | @ gamemode");
            static
                char platform = 0;
ret_gm:
                println("Select platform:");
                println("[L/l] Linux");
                println("[W/w] Windows");
                printf(">> ");

            if (scanf(" %c", &platform) != 1)
                return 0;

            signal(SIGINT, __init);

            if (platform == 'L' || platform == 'l')
                watch_samp("linux");
            else if (platform == 'W' || platform == 'w')
                watch_samp("windows");
            else {
                printf("Invalid platform selection. use C^ to exit.\n");
                goto ret_gm;
            }
        } else if (strcmp(ptr_command, "clear") == 0) {
            watchdogs_title("Watchdogs | @ clear");
            watchdogs_sys("clear");
            return 0;
        } else if (strcmp(ptr_command, "exit") == 0) {
            exit(1);
        } else if (strcmp(ptr_command, "kill") == 0) {
            watchdogs_title("Watchdogs | @ kill");
            watchdogs_sys("clear");
            __init(0);
        } else if (strcmp(ptr_command, "hardware") == 0) {
            printf("=== System Hardware Information ===\n\n");
            get_system_info();
            printf("\n");

            get_cpu_info();
            printf("\n");

            get_memory_info();
            printf("\n");

            get_disk_info();
            printf("\n");

            get_network_info();
            printf("\n");

#ifdef _WIN32
            printf("Note: GPU/BIOS info not implemented for Windows version.\n");
#else
            get_bios_info();
            get_gpu_info();
#endif

            printf("\n===================================\n");
            __init(0);
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') arg++;
            if (*arg == '\0') {
                println("Usage: title [<title>]");
            } else {
                char title_set[128];
                snprintf(title_set, sizeof(title_set), arg);
                watchdogs_title(title_set);
            }
            return 0;
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
            watchdogs_title("Watchdogs | @ compile");
            static char *arg;
            arg = ptr_command + 7;
            while (*arg == ' ') arg++;
        
            static char *compile_args;
            compile_args = strtok(arg, " ");
        
            const char *ptr_pawncc;
            if (__watchdogs_os__ == 0x01) 
                ptr_pawncc = "pawncc.exe";
            else if (__watchdogs_os__ == 0x00)
                ptr_pawncc = "pawncc";

            if (access(".wd_compiler.log", F_OK) == 0) {
                remove(".wd_compiler.log");
            }
            
            FILE *procc_f = NULL;
            char error_compiler_check[100];
            char watchdogs_c_output_f_container[128];
            int format_size_c_f_container = sizeof(watchdogs_c_output_f_container);
            
            procc_f = fopen(".wd_compiler.log", "r");
            if (procc_f) {
                int ch;
                while ((ch = fgetc(procc_f)) != EOF) putchar(ch);
                rewind(procc_f);
                while (fscanf(procc_f, "%s", error_compiler_check) != EOF) {
                    if (strcmp(error_compiler_check, "error") == 0) {
                        if (access(watchdogs_c_output_f_container, F_OK) == 0)
                            remove(watchdogs_c_output_f_container);
                        break;
                    }
                }
                fclose(procc_f);
            }

            int find_pawncc = watchdogs_sef_fdir(".", ptr_pawncc);
            if (find_pawncc) {
                size_t format_size_compiler = 2048;
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
                
                toml_table_t *watchdogs_compiler = toml_table_in(config, "compiler");
                if (watchdogs_compiler) {
                    toml_datum_t option_val = toml_string_in(watchdogs_compiler, "option");
                    if (option_val.ok) {
                        wcfg.wd_compiler_opt = strdup(option_val.u.s);
                        free(option_val.u.s);
                    }
        
                    toml_array_t *include_paths = toml_array_in(watchdogs_compiler, "include_path");
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
                            }
                        }
        
                        static char wd_gamemode[56];
                        if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                            toml_datum_t watchdogs_gmodes = toml_string_in(watchdogs_compiler, "input");
                            if (watchdogs_gmodes.ok) {
                                wcfg.wd_gamemode_input = strdup(watchdogs_gmodes.u.s);
                                free(watchdogs_gmodes.u.s);
                            }
                            toml_datum_t watchdogs_gmodes_o = toml_string_in(watchdogs_compiler, "output");
                            if (watchdogs_gmodes_o.ok) {
                                wcfg.wd_gamemode_output = strdup(watchdogs_gmodes_o.u.s);
                                free(watchdogs_gmodes_o.u.s);
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
                                "%s \"%s\" -o\"%s\" %s -i\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                wcfg.watchdogs_sef_found[0],                   // compiler binary
                                wcfg.wd_gamemode_input,                        // input file
                                wcfg.wd_gamemode_output,                       // output file
                                include_aio_path,                              // include search path
                                path_include,                                  // include directory
                                wcfg.wd_compiler_opt                           // additional options
                            );

                            char title_compiler_info[128];
                            snprintf(title_compiler_info, sizeof(title_compiler_info), "Watchdogs | @ compile | %s | %s | %s", wcfg.watchdogs_sef_found[0], watchdogs_c_output_f_container, wcfg.wd_gamemode_output);
                            watchdogs_title(title_compiler_info);
                            
                            clock_gettime(CLOCK_MONOTONIC, &start);
                                watchdogs_sys(_compiler_);
                            clock_gettime(CLOCK_MONOTONIC, &end);

                            procc_f = fopen(".wd_compiler.log", "r");
                            if (procc_f) {
                                int ch;
                                while ((ch = fgetc(procc_f)) != EOF)
                                    putchar(ch);
                                rewind(procc_f);
                                while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                    if (strcmp(error_compiler_check, "error") == 0) {
                                        FILE *c_output;
                                        c_output = fopen(watchdogs_c_output_f_container, "r");
                                        if (c_output)
                                            remove(watchdogs_c_output_f_container);
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
                                char __direct_path[PATH_MAX] = {0},
                                     __file_name[PATH_MAX] = {0},
                                     __input_path[PATH_MAX] = {0},
                                     __tmp_arg[PATH_MAX] = {0};

                                strncpy(__tmp_arg, compile_args, sizeof(__tmp_arg) - 1);
                                __tmp_arg[sizeof(__tmp_arg) - 1] = '\0';

                                char *last_slash = strrchr(__tmp_arg, '/'),
                                     *last_back = strrchr(__tmp_arg, '\\');
                                if (last_back && (!last_slash || last_back > last_slash))
                                    last_slash = last_back;

                                if (last_slash) {
                                    size_t dir_len = (size_t)(last_slash - __tmp_arg);
                                    if (dir_len >= sizeof(__direct_path)) dir_len = sizeof(__direct_path) - 1;
                                    memcpy(__direct_path, __tmp_arg, dir_len);
                                    __direct_path[dir_len] = '\0';

                                    strncpy(__file_name, last_slash + 1, sizeof(__file_name) - 1);
                                    __file_name[sizeof(__file_name) - 1] = '\0';

                                    size_t need = strlen(__direct_path) + 1 + strlen(__file_name) + 1;
                                    if (need > sizeof(__input_path)) {
                                        size_t max_fn = sizeof(__input_path) - strlen(__direct_path) - 2;
                                        if (max_fn > 0)
                                            __file_name[max_fn] = '\0';
                                        else {
                                            __direct_path[0] = '\0';
                                            strncpy(__direct_path, "gamemodes", sizeof(__direct_path) - 1);
                                            __direct_path[sizeof(__direct_path) - 1] = '\0';
                                            strncpy(__file_name, __tmp_arg, sizeof(__file_name) - 1);
                                            __file_name[sizeof(__file_name) - 1] = '\0';
                                        }
                                    }

                                    snprintf(__input_path, sizeof(__input_path), "%s/%s", __direct_path, __file_name);
                                    __input_path[sizeof(__input_path) - 1] = '\0';
                                } else {
                                    strncpy(__file_name, __tmp_arg, sizeof(__file_name) - 1);
                                    __file_name[sizeof(__file_name) - 1] = '\0';

                                    if (__direct_path[0] == '\0') {
                                        strncpy(__direct_path, "gamemodes", sizeof(__direct_path) - 1);
                                        __direct_path[sizeof(__direct_path) - 1] = '\0';
                                    }
                                    size_t need = strlen("gamemodes") + 1 + strlen(__file_name) + 1;
                                    if (need > sizeof(__input_path)) {
                                        size_t max_fn = sizeof(__input_path) - strlen("gamemodes") - 2;
                                        if (max_fn > 0) __file_name[max_fn] = '\0';
                                    }

                                    snprintf(__input_path, sizeof(__input_path), "gamemodes/%s", __file_name);
                                    __input_path[sizeof(__input_path) - 1] = '\0';
                                    strncpy(__direct_path, "gamemodes", sizeof(__direct_path) - 1);
                                    __direct_path[sizeof(__direct_path) - 1] = '\0';
                                }

                                int find_gamemodes_arg1 = watchdogs_sef_fdir(__direct_path, __file_name);
                                if (find_gamemodes_arg1) {
                                char* container_output;
                                if (wcfg.watchdogs_sef_count > 0 &&
                                    wcfg.watchdogs_sef_found[1][0] != '\0') {
                                    container_output = strdup(wcfg.watchdogs_sef_found[1]);
                                }
                                
                                char i_path_rm[PATH_MAX];
                                snprintf(i_path_rm, sizeof(i_path_rm), "%s", compile_args);
                                char *f_EXT = strrchr(i_path_rm, '.');
                                if (f_EXT && strcmp(f_EXT, ".pwn") == 0)
                                    *f_EXT = '\0';
                                char* f_last_slash_container = strrchr(container_output, '/');
                                if (f_last_slash_container != NULL && *(f_last_slash_container + 1) != '\0')
                                    *(f_last_slash_container + 1) = '\0';

                                if (strncmp(i_path_rm, __direct_path, strlen(__direct_path)) == 0) {
                                    snprintf(watchdogs_c_output_f_container, format_size_c_f_container,
                                            "%s%s", "", i_path_rm);
                                } else {
                                    snprintf(watchdogs_c_output_f_container, format_size_c_f_container,
                                            "%s/%s", __direct_path, i_path_rm);
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
                                    "%s \"%s\" -o\"%s.amx\" %s -i\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                    wcfg.watchdogs_sef_found[0],                    // compiler binary
                                    compile_args,                                   // input file
                                    watchdogs_c_output_f_container,                 // output file
                                    include_aio_path,                               // include search path
                                    path_include,                                   // include directory
                                    wcfg.wd_compiler_opt                            // additional options
                                );

                                if (ret < 0 || (size_t)ret >= (size_t)format_size_compiler) {
                                    fprintf(stderr, "[Error] snprintf() failed or buffer too small (needed %d bytes)\n", ret);
                                }

                                char title_compiler_info[128];
                                snprintf(title_compiler_info, sizeof(title_compiler_info), "Watchdogs | @ compile | %s | %s | %s.amx", wcfg.watchdogs_sef_found[0], compile_args, watchdogs_c_output_f_container);
                                watchdogs_title(title_compiler_info);
                                
                                clock_gettime(CLOCK_MONOTONIC, &start);
                                    watchdogs_sys(_compiler_);
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                
                                procc_f = fopen(".wd_compiler.log", "r");
                                if (procc_f) {
                                    int ch;
                                    while ((ch = fgetc(procc_f)) != EOF)
                                        putchar(ch);
                                    rewind(procc_f);
                                    while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                        if (strcmp(error_compiler_check, "error") == 0) {
                                            FILE *c_output;
                                            c_output = fopen(watchdogs_c_output_f_container, "r");
                                            if (c_output)
                                                remove(watchdogs_c_output_f_container);
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
        } else if (strncmp(ptr_command, "running", 7) == 0 || strncmp(ptr_command, "debug", 7) == 0) {
_runners_:
                if (strcmp(ptr_command, "debug") == 0) {
                    wcfg.server_or_debug="debug";
                    watchdogs_title("Watchdogs | @ debug");    
                } else {
                    watchdogs_title("Watchdogs | @ running");
                }

                static char *arg;
                    arg = ptr_command + 7;
                while (*arg == ' ') arg++;
                char *arg1 = strtok(arg, " ");

                static char *format_prompt = NULL;
                static size_t format_size = 126;
                if (format_prompt == NULL) 
                    format_prompt = malloc(format_size);

                if (find_for_samp == 0x1) {
                    if (*arg == '\0') {
                        FILE *server_log = fopen("server_log.txt", "r");
                        if (server_log)
                            remove("server_log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(ptr_samp, 0777);
#endif
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", ptr_samp);
                        int running_FAIL = watchdogs_sys(snprintf_ptrS);
                        if (running_FAIL == 0) {
                            sleep(2);

                            printf_color(COL_YELLOW, "Press enter to print logs..");
                            getchar();

                            FILE *server_log = fopen("server_log.txt", "r");
                            if (!server_log)
                                printf_error("Can't found server_log.txt!");
                            else {
                                int ch;
                                while ((ch = fgetc(server_log)) != EOF) {
                                    putchar(ch);
                                }
                                fclose(server_log);
                            }
                        } else {
                            printf_color(COL_RED, "running failed! ");
                        }
                        if (format_prompt) { free(format_prompt); }
                        return 0;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        watchdogs_server_samp(arg1, ptr_samp);
                    }
                } else if (find_for_omp == 0x1) {
                    if (*arg == '\0') {
                        FILE *server_log = fopen("log.txt", "r");

                        if (server_log) 
                            remove("log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(ptr_openmp, 0777);
#endif
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", ptr_openmp);
                        int running_FAIL = watchdogs_sys(snprintf_ptrS);
                        if (running_FAIL == 0) {
                            sleep(2);

                            printf_color(COL_YELLOW, "Press enter to print logs..");
                            getchar();

                            FILE *server_log = fopen("log.txt", "r");
                            if (!server_log)
                                printf_error("Can't found log.txt!");
                            else {
                                int ch;
                                while ((ch = fgetc(server_log)) != EOF) {
                                    putchar(ch);
                                }
                                fclose(server_log);
                            }
                        } else {
                            printf_color(COL_RED, "running failed! ");
                        }
                        if (format_prompt) { free(format_prompt); }
                        return 0;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        watchdogs_server_openmp(arg1, ptr_openmp);
                    }
                } else if (!find_for_omp || !find_for_samp) {
                    printf_error("samp-server/open.mp server not found!");

                    char *ptr_sigA;
                    ptr_sigA = readline("install now? [Y/n]: ");

                    while (1) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            if (__watchdogs_os__ == 0x01) {
                                watch_samp("windows");
                            } else if (__watchdogs_os__ == 0x00) {
                                watch_samp("linux");
                            }
                            break;
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            break;
                        } else {
                            printf("Invalid input. Please type Y/y to install or N/n to cancel.\n");
                            ptr_sigA = readline("install now? [Y/n]: ");
                        }
                    }

                    if (ptr_sigA) { free(ptr_sigA); }
                }
                if (format_prompt) { free(format_prompt); }
        } else if (strcmp(ptr_command, "stop") == 0) {
            watchdogs_title("Watchdogs | @ stop");
            watchdogs_server_stop_tasks();
            return 0;
        } else if (strcmp(ptr_command, "restart") == 0) {
            watchdogs_server_stop_tasks();
            sleep(2);
            goto _runners_;
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            watchdogs_title("Watchdogs | @ undefined");
            printf("did you mean: '%s'?", _dist_command);
            char *confirm = readline(" [y/n]: ");
            if (confirm) {
                if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
                    ptr_command = strdup(_dist_command);
                    if (confirm) { free(confirm); }
                    goto _reexecute_command;
                } else {
                    printf("Canceled.\n");
                }
                if (confirm) { free(confirm); }
            }
        } else {
            if (strlen(ptr_command) > 0) {
                watchdogs_title("Watchdogs | @ not found");
                println("%s not found!", ptr_command);
            }
        }

        if (ptr_command) { free(ptr_command); }
}

void __init(int sig_unused) {
        (void)sig_unused;
        signal(SIGINT, handle_sigint);
        watchdogs_title(NULL);
        while (1) {
            __init_wd();
        }
}

int main(void) {
        __init(0);
        return 0;
}

