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
#include "compiler.h"

int
    __os__;
const char
    *ptr_samp = NULL;
int
    find_for_samp = 0x0;
const char
    *ptr_openmp = NULL;
int
    find_for_omp = 0x0;
    
void __init_function(void) {
        watchdogs_toml_data();
        watchdogs_u_history();
        watchdogs_reset_var();
        reset_watchdogs_sef_dir();

        __os__ = signal_system_os();
        
        if (__os__ == 0x01) {
            ptr_samp="samp-server.exe"; ptr_openmp="omp-server.exe";
        }
        else if (__os__ == 0x00) {
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
        printf("[__init_function]:\n\t__os__: 0x0%d\n\tptr_samp: %s\n\tptr_openmp: %s\n", __os__, ptr_samp, ptr_openmp);
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
                println(" clear");
                println(" exit");
                println(" kill");
                println(" title");
                println(" toml");
                println(" gamemode");
                println(" pawncc");
                println(" compile");
                println(" running");
                println(" debug");
                println(" stop");
                println(" restart");
                println(" hardware");
            } else if (strcmp(arg, "clear") == 0) { println("clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "exit") == 0) { println("exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "kill") == 0) { println("kill: kill - refresh terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println("title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "toml") == 0) { println("toml: re-create toml - re-create & re-write watchdogs.toml\" | Usage: \"toml\"");
            } else if (strcmp(arg, "hardware") == 0) { println("hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println("gamemode: gamemode - download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println("pawncc: pawncc - download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println("compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println("running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "debug") == 0) { println("debug: debugging your project. | Usage: \"debug\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println("stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println("restart: restart server task. | Usage: \"restart\"");
            } else println("help not found for: '%s'", arg);
            
            return 0;
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
                println("Usage: title [<title>]");
            } else {
                char title_set[128];
                snprintf(title_set, sizeof(title_set), "%s", arg);
                watchdogs_title(title_set);
            }
            return 0;
        } else if (strcmp(ptr_command, "toml") == 0) {
            if (access("watchdogs.toml", F_OK) == 0) {
                remove("watchdogs.toml");
            }
            watchdogs_toml_data();
        } else if (strcmp(ptr_command, "hardware") == 0) {
            printf("=== System Hardware Information ===\n\n");
            hardware_system_info();
            printf("\n");

            hardware_cpu_info();
            printf("\n");

            hardware_memory_info();
            printf("\n");

            hardware_disk_info();
            printf("\n");

            hardware_network_info();
            printf("\n");

#ifdef _WIN32
            printf("Note: GPU/BIOS info not implemented for Windows version.\n");
#else
            hardware_bios_info();
            hardware_gpu_info();
#endif

            printf("\n===================================\n");
            __init(0);
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
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
            watchdogs_title("Watchdogs | @ compile");
            static char *arg;
            arg = ptr_command + 7;
            while (*arg == ' ') arg++;
        
            static char *compile_args;
            compile_args = strtok(arg, " ");

            watchdogs_compiler_sys(arg, compile_args);
        } else if (strncmp(ptr_command, "running", 7) == 0 || strncmp(ptr_command, "debug", 7) == 0) {
_runners_:
                if (strcmp(ptr_command, "debug") == 0) {
                    wcfg.serv_dbg="debug";
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
                            if (__os__ == 0x01) {
                                watch_samp("windows");
                            } else if (__os__ == 0x00) {
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

