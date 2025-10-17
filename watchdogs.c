#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path) // override POSIX mkdir
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
#include "crypto.h"
#include "package.h"
#include "watchdogs.h"
#include "server.h"

void __init_function(void) {
        watchdogs_title(NULL);
        watchdogs_toml_data();
        watchdogs_u_history();
        watchdogs_reset_var();
}

int __init_wd(void)
{
	char *ptr_command = readline("watchdogs > ");
	if (!ptr_command) return -1;
	watchdogs_a_history(ptr_command);

	int c_distance = INT_MAX;
	const char *_dist_command = __find_c_command(ptr_command,
                                                 __command,
                                                 __command_len,
                                                 &c_distance);

        if (strncmp(ptr_command, "help", 4) == 0) {
            watchdogs_title("Watchdogs | @ help");

            static char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') arg++;

            if (strlen(arg) == 0) {
                println("usage: help | help [<cmds>]");
                println("cmds:");
                println(" clear, exit, kill, title");
                println(" gamemode, pawncc");
                println(" compile, running, debug");
                println(" stop, restart");
            } else if (strcmp(arg, "exit") == 0) { println("exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "clear") == 0) { println("clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "kill") == 0) { println("kill: kill - restart terminal watchdogs. | Usage: \"kill\"");
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
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') arg++;
            if (*arg == '\0') {
                println("usage: title [<title>]");
            } else {
                printf("\033]0;%s\007", arg);
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
            int __watchdogs_os__ = signal_system_os();
            if (__watchdogs_os__ == 0x01) 
                ptr_pawncc = "pawncc.exe";
            else if (__watchdogs_os__ == 0x00)
                ptr_pawncc = "pawncc";

            FILE *procc_f = NULL;
            char error_compiler_check[100];
            char watchdogs_c_output_f_container[128];
            int format_size_c_f_container = sizeof(watchdogs_c_output_f_container);
        
            int find_pawncc = watchdogs_sef_fdir(".", ptr_pawncc);
            if (find_pawncc) {
                static char *_compiler_ = NULL;
                static size_t format_size_compiler = 2048;

                if (_compiler_ == NULL) {
                    _compiler_ = malloc(format_size_compiler);
                    if (!_compiler_) {
                        printf_error("Memory allocation failed for _compiler_!\n");
                        return 0;
                    }
                }
        
                FILE *procc_f = fopen("watchdogs.toml", "r");
                if (!procc_f)
                    printf_error("Can't read file %s\n", "watchdogs.toml");
        
                char errbuf[256];
                toml_table_t *config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                fclose(procc_f);
        
                if (!config)
                    printf_error("error parsing TOML: %s\n", errbuf);
        
                toml_table_t *watchdogs_compiler = toml_table_in(config, "compiler");
                if (watchdogs_compiler) {
                    toml_datum_t option_val = toml_string_in(watchdogs_compiler, "option");
                    if (option_val.ok) {
                        watchdogs_config.wd_compiler_opt = option_val.u.s;
                    }
        
                    toml_datum_t output_val = toml_string_in(watchdogs_compiler, "output");
                    if (output_val.ok) {
                        watchdogs_config.wd_gamemode_output = output_val.u.s;
                    }
        
                    toml_array_t *include_paths = toml_array_in(watchdogs_compiler, "include_path");
                    if (include_paths) {
                        int array_size = toml_array_nelem(include_paths);
                        char all_paths[250] = {0};
        
                        for (int i = 0; i < array_size; i++) {
                            toml_datum_t path_val = toml_string_at(include_paths, i);
                            if (path_val.ok) {
                                if (i > 0)
                                    strcat(all_paths, " ");
                                snprintf(all_paths + strlen(all_paths), sizeof(all_paths) - strlen(all_paths), "-i\"%s\"", path_val.u.s);
                            }
                        }
        
                        static char wd_gamemode[56];
                        if (*arg == '\0' || arg == ".") {
                            toml_datum_t watchdogs_gmodes = toml_string_in(watchdogs_compiler, "input");
                            if (watchdogs_gmodes.ok) {
                                watchdogs_config.wd_gamemode_input = watchdogs_gmodes.u.s;
                            }
                            
                            // Search for the specified gamemode file inside the "gamemodes/" directory.
                            // 'watchdogs_config.wd_gamemode_input' holds the name of the gamemode to find.
                            // The result is stored in 'find_gamemodes':
                            //     - If the file is found, 'find_gamemodes' will be set to 1.
                            //     - If the file is not found, 'find_gamemodes' will be set to 0.
                            // This is typically used to verify that the gamemode exists before proceeding.
                            int find_gamemodes = watchdogs_sef_fdir("gamemodes/", watchdogs_config.wd_gamemode_input);
                            if (find_gamemodes) {
                                char* container_output = strdup(watchdogs_config.watchdogs_sef_found[1]);
                                char* f_last_slash_container = strrchr(container_output, '/');
                                if (f_last_slash_container != NULL && *(f_last_slash_container + 1) != '\0')
                                    *(f_last_slash_container + 1) = '\0';

                                snprintf(watchdogs_c_output_f_container, format_size_c_f_container, "%s%s",
                                    container_output, watchdogs_config.wd_gamemode_output);

                                watchdogs_config.wd_gamemode_input=strdup(watchdogs_config.watchdogs_sef_found[1]);

                                struct timespec start, end;
                                double compiler_dur;

                                snprintf(_compiler_, format_size_compiler, "%s %s \"%s\" -o\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                    watchdogs_config.watchdogs_sef_found[0], /// %s 1
                                    all_paths,                              /// %s 2
                                    watchdogs_config.wd_gamemode_input,    /// %s 3
                                    watchdogs_c_output_f_container,       /// %s 4
                                    watchdogs_config.wd_compiler_opt);   // %s 5

                                clock_gettime(CLOCK_MONOTONIC, &start);
                                watchdogs_sys(_compiler_);
                                clock_gettime(CLOCK_MONOTONIC, &end);

                                procc_f = fopen(".wd_compiler.log", "r");
                                if (procc_f) {
                                    int ch;
                                    while ((ch = fgetc(procc_f)) != EOF) {
                                        putchar(ch);
                                    }
                                }
                                while (fscanf(procc_f, "%s", error_compiler_check) != EOF) {
                                    if (strcmp(error_compiler_check, "error") == 0) {
                                        FILE *c_output;
                                        c_output = fopen(watchdogs_c_output_f_container, "r");
                                        if (c_output)
                                            remove(watchdogs_c_output_f_container);
                                        break;
                                    }
                                }
                            
                                fclose(procc_f);

                                compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                                printf("[Finished in %.3fs]\n", compiler_dur);
                                if (_compiler_) { free(_compiler_); }
                            } else {
                                printf_color(COL_RED, "Can't locate: ");
                                printf("%s\n", watchdogs_config.wd_gamemode_input);
                                return 0;
                            }
                        } else {
                            // Search for the specified gamemode file inside the "gamemodes/" directory.
                            // 'watchdogs_config.wd_gamemode_input' holds the name of the gamemode to find.
                            // The result is stored in 'find_gamemodes_arg1':
                            //     - If the file is found, 'find_gamemodes_arg1' will be set to 1.
                            //     - If the file is not found, 'find_gamemodes_arg1' will be set to 0.
                            // This is typically used to verify that the gamemode exists before proceeding.
                            int find_gamemodes_arg1 = watchdogs_sef_fdir("gamemodes/", compile_args);
                            if (find_gamemodes_arg1) {
                                char* container_output = strdup(watchdogs_config.watchdogs_sef_found[1]);
                                char* f_last_slash_container = strrchr(container_output, '/');
                                if (f_last_slash_container != NULL && *(f_last_slash_container + 1) != '\0')
                                    *(f_last_slash_container + 1) = '\0';

                                snprintf(watchdogs_c_output_f_container, format_size_c_f_container, "%s%s",
                                    container_output, watchdogs_config.wd_gamemode_output);

                                compile_args=strdup(watchdogs_config.watchdogs_sef_found[1]);

                                struct timespec start, end;
                                double compiler_dur;

                                snprintf(_compiler_, format_size_compiler, "%s %s \"%s\" -o\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                    watchdogs_config.watchdogs_sef_found[0], /// %s 1
                                    all_paths,                              /// %s 2
                                    compile_args,                          /// %s 3
                                    watchdogs_c_output_f_container,       /// %s 4
                                    watchdogs_config.wd_compiler_opt);   /// %s 5
                                
                                clock_gettime(CLOCK_MONOTONIC, &start);
                                watchdogs_sys(_compiler_);
                                clock_gettime(CLOCK_MONOTONIC, &end);

                                procc_f = fopen(".wd_compiler.log", "r");
                                if (procc_f) {
                                    int ch;
                                    while ((ch = fgetc(procc_f)) != EOF) {
                                        putchar(ch);
                                    }
                                }
                                while (fscanf(procc_f, "%s", error_compiler_check) != EOF) {
                                    if (strcmp(error_compiler_check, "error") == 0) {
                                        FILE *c_output;
                                        c_output = fopen(watchdogs_c_output_f_container, "r");
                                        if (c_output)
                                            remove(watchdogs_c_output_f_container);
                                        break;
                                    }
                                }
                            
                                fclose(procc_f);

                                compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                                printf("[Finished in %.3fs]\n", compiler_dur);
                                
                                if (_compiler_) { free(_compiler_); }
                            } else {
                                printf_color(COL_RED, "Can't locate: ");
                                printf("%s\n", compile_args);
                                return 0;
                            }
                        }
                    }
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
                    watchdogs_config.server_or_debug="debug";
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

                static const char
                    *ptr_samp = NULL;
                static int
                    find_for_samp = 0x0;
                static const char
                    *ptr_openmp = NULL;
                static int
                    find_for_omp = 0x0;

                static int __watchdogs_os__;
                    __watchdogs_os__ = signal_system_os();
                
                if (__watchdogs_os__ == 0x01) {
                    ptr_samp="samp-server.exe"; ptr_openmp="omp-server.exe";
                }
                else if (__watchdogs_os__ == 0x00) {
                    ptr_samp="samp03svr"; ptr_openmp="omp-server";
                }
                
                FILE *file_s = fopen(ptr_samp, "r");
                if (file_s)
                    find_for_samp=0x1;
                FILE *file_m = fopen(ptr_openmp, "r");
                if (file_m)
                    find_for_omp=0x1;

                if (find_for_samp == 0x1) {
                    if (*arg == '\0' || arg == ".") {
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
                        return 0;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        watchdogs_server_samp(arg1, ptr_samp);
                    }
                } else if (find_for_omp == 0x1) {
                    if (*arg == '\0' || arg == ".") {
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
                            if (ptr_sigA) { free(ptr_sigA); }
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
            printf("did you mean: %s?\n", _dist_command);
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
        __init_function();
        while (1) {
            __init_wd();
        }
}

int main(void) {
        __init(0);
        return 0;
}