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
#define getcwd _getcwd
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define __PATH_SYM "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <unistd.h>
#define __PATH_SYM "/"
#endif

#include <ncursesw/curses.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
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
#include "extra.h"
#include "utils.h"
#include "hardware.h"
#include "crypto.h"
#include "package.h"
#include "archive.h"
#include "curl.h"
#include "chain.h"
#include "server.h"
#include "compiler.h"
#include "depends.h"

struct timespec cmd_start, cmd_end;
double command_dur;

void __function__(void) {
        wd_sef_fdir_reset();
        wd_set_toml();
        wd_u_history();
        
        wcfg.os_type = wd_signal_os();
        
        if (wcfg.os_type == OS_SIGNAL_WINDOWS) {
            wcfg.pointer_samp="samp-server.exe";
            wcfg.pointer_openmp="omp-server.exe";
        } else if (wcfg.os_type == OS_SIGNAL_LINUX) {
            wcfg.pointer_samp="samp03svr";
            wcfg.pointer_openmp="omp-server";
        }
        
        FILE *file_s = fopen(wcfg.pointer_samp, "r");
        if (file_s) {
            wcfg.f_samp = 0x01;
            fclose(file_s);
        }
        FILE *file_m = fopen(wcfg.pointer_openmp, "r");
        if (file_m) {
            wcfg.f_openmp = 0x01;
            fclose(file_m);
        }
#if defined(_DBG_PRINT)
        printf_color(COL_YELLOW, "-DEBUGGING\n");
        printf("[__function__]:\n\tos_type: 0x0%d\n\tpointer_samp: %s\n\tpointer_openmp: %s\n",
                wcfg.os_type,
                wcfg.pointer_samp,
                wcfg.pointer_openmp);
#endif
        return;
}

int __command__(void)
{
        char
            __cwd[PATH_MAX]
        ;
        if (!getcwd(__cwd, sizeof(__cwd))) {
            perror("getcwd");
            return RETW;
        }

_ptr_command:
        char ptr_prompt[PATH_MAX + 56];
        if (strlen(__cwd) > 100)
            snprintf(ptr_prompt, sizeof(ptr_prompt), "[" COL_YELLOW "watchdogs:%s" COL_DEFAULT "]\n\t> $", __cwd);
        else
            snprintf(ptr_prompt, sizeof(ptr_prompt), "[" COL_YELLOW "watchdogs:%s" COL_DEFAULT "] > $", __cwd);
        char* ptr_command = readline(ptr_prompt);

        if (ptr_command == NULL || ptr_command[0] == '\0')
            goto _ptr_command;
        
        wd_a_history(ptr_command);

        int c_distance = INT_MAX;
        const char *_dist_command = wd_find_near_command(ptr_command,
                                                       __command,
                                                       __command_len,
                                                       &c_distance);

_reexecute_command:
        clock_gettime(CLOCK_MONOTONIC, &cmd_start);
        if (strncmp(ptr_command, "help", 4) == 0) {
            wd_set_title("Watchdogs | @ help");

            char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') arg++;

            if (strlen(arg) == 0) {
                println("Usage: help | help [<command>]");
                println("command:");
                println(" -> clear");
                println(" -> exit");
                println(" -> kill");
                println(" -> title");
                println(" -> toml");
                println(" -> install");
                println(" -> gamemode");
                println(" -> pawncc");
                println(" -> compile");
                println(" -> running");
                println(" -> crunn");
                println(" -> debug");
                println(" -> stop");
                println(" -> restart");
                println(" -> hardware");
            } else if (strcmp(arg, "clear") == 0) { println("clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "exit") == 0) { println("exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "kill") == 0) { println("kill: kill - refresh terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println("title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "toml") == 0) { println("toml: re-create toml - re-create & re-write watchdogs.toml\" | Usage: \"toml\"");
            } else if (strcmp(arg, "install") == 0) { println("install: download & install depends | Usage: \"install\" | [<args>] - install github.com/github.com/gitea.com:user/repo:vtags");
            } else if (strcmp(arg, "hardware") == 0) { println("hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println("gamemode: gamemode - download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println("pawncc: pawncc - download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println("compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println("running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "crunn") == 0) { println("crunn: compile & running your project. | Usage: \"crunn\" | [<args>]");
            } else if (strcmp(arg, "debug") == 0) { println("debug: debugging your project. | Usage: \"debug\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println("stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println("restart: restart server task. | Usage: \"restart\"");
            } else println("help not found for: '%s'", arg);
            return RETN;
        } else if (strcmp(ptr_command, "clear") == 0) {
            wd_set_title("Watchdogs | @ clear");
            wd_run_command("clear");
            goto _ptr_command;
        } else if (strcmp(ptr_command, "exit") == 0) {
            exit(1);
        } else if (strcmp(ptr_command, "kill") == 0) {
            wd_set_title("Watchdogs | @ kill");
            wd_run_command("clear");
            sleep(1);
            __main(0);
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') arg++;
            if (*arg == '\0') {
                println("Usage: title [<title>]");
            } else {
                char title_set[128];
                snprintf(title_set, sizeof(title_set), "%s", arg);
                wd_set_title(title_set);
            }
            return RETN;
        } else if (strcmp(ptr_command, "toml") == 0) {
            if (access("watchdogs.toml", F_OK) == 0) {
                remove("watchdogs.toml");
            }
            __function__();
            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (procc_f) {
#ifdef _WIN32
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hOut != INVALID_HANDLE_VALUE) {
                    DWORD dwMode = 0;
                    if (GetConsoleMode(hOut, &dwMode))
                        SetConsoleMode(hOut,
                                       dwMode |
                                       ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
#endif

                const char *BG  = "\x1b[48;5;235m";
                const char *FG  = "\x1b[97m";
                const char *BORD= "\x1b[33m";
                const char *RST = "\x1b[0m";

                printf("%s%s+------------------------------------------+%s\n", BORD, FG, RST);

                char line[256];
                while (fgets(line, sizeof(line), procc_f)) {
                    size_t len = strlen(line);
                    if (len && (line[len-1] == '\n' ||
                        line[len-1] == '\r'))
                        line[--len] = '\0';

                    printf("%s%s|%s %-40s %s|%s\n", BORD, FG, BG, line, BORD, RST);
                }

                printf("%s%s+------------------------------------------+%s\n", BORD, FG, RST);
                fclose(procc_f);
            }
            return RETN;

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
            
            return RETN;
        } else if (strncmp(ptr_command, "install", 7) == 0) {
            wd_set_title("Watchdogs | @ install depends");

            char *arg = ptr_command + 7;
            while (*arg == ' ') arg++;

            char *pkg_one = strtok(arg, " ");
            char *pkg_two = strtok(NULL, " ");

            if (!pkg_one && !pkg_two) {
                printf_info("install [repo] [repo]");
            } else {
                wd_install_packages(pkg_one, pkg_two);
            }

            return RETN;
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            wd_set_title("Watchdogs | @ gamemode");
            char platform = 0;
ret_ptr1:
                println("Select platform:");
                println("-> [L/l] Linux");
                println("-> [W/w] Windows");
                printf("==> ");

            if (scanf(" %c", &platform) != 1)
                return RETN;

            if (platform == 'L' || platform == 'l')
                wd_install_server("linux");
            else if (platform == 'W' || platform == 'w')
                wd_install_server("windows");
            else {
                printf_error("Invalid platform selection. use C^ to exit.");
                goto ret_ptr1;
            }

            __main(0);
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            wd_set_title("Watchdogs | @ pawncc");
            char platform = 0;
ret_ptr2:
                println("Select platform:");
                println("-> [L/l] Linux");
                println("-> [W/w] Windows");
                println("-> [T/t] Termux");
                printf("==> ");

            if (scanf(" %c", &platform) != 1)
                return RETN;

            if (platform == 'L' || platform == 'l')
                wd_install_pawncc("linux");
            else if (platform == 'W' || platform == 'w')
                wd_install_pawncc("windows");
            else if (platform == 'T' || platform == 't')
                wd_install_pawncc("termux");
            else {
                printf_error("Invalid platform selection. use C^ to exit.");
                goto ret_ptr2;
            }
            
            __main(0);
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
            wd_set_title("Watchdogs | @ compile");
            char *arg;
            arg = ptr_command + 7;
            while (*arg == ' ') arg++;
            char *compile_args;
            compile_args = strtok(arg, " ");

            wd_RunCompiler(arg, compile_args);
            return RETN;
        } else if (strncmp(ptr_command, "running", 7) == 0 || strncmp(ptr_command, "debug", 7) == 0) {
_runners_:
                if (strcmp(ptr_command, "debug") == 0) {
                    wcfg.serv_dbg="debug";
                    wd_set_title("Watchdogs | @ debug");    
                } else {
                    wd_set_title("Watchdogs | @ running");
                }

                char *arg;
                    arg = ptr_command + 7;
                while (*arg == ' ') arg++;
                char *arg1 = strtok(arg, " ");

                if (wcfg.f_samp == 0x01) {
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        FILE *server_log = fopen("server_log.txt", "r");
                        if (server_log)
                            remove("server_log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(wcfg.pointer_samp, 0777);
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", wcfg.pointer_samp);
#else
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", wcfg.pointer_samp);
#endif
                        int running_FAIL = wd_run_command(snprintf_ptrS);
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
                            printf_color(COL_RED, "running failed!\n");
                        }
                        return RETN;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        wd_run_samp_server(arg1, wcfg.pointer_samp);
                    }
                } else if (wcfg.f_openmp == 0x01) {
                    if (*arg == '\0') {
                        FILE *server_log = fopen("log.txt", "r");

                        if (server_log) 
                            remove("log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(wcfg.pointer_openmp, 0777);
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", wcfg.pointer_openmp);
#else
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", wcfg.pointer_openmp);
#endif
                        int running_FAIL = wd_run_command(snprintf_ptrS);
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
                            printf_color(COL_RED, "running failed!\n");
                        }
                        return RETN;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        wd_run_omp_server(arg1, wcfg.pointer_openmp);
                    }
                } else if (wcfg.f_samp == 0x00 || wcfg.f_openmp == 0x00) {
                    printf_error("samp-server/open.mp server not found!");

                    char *ptr_sigA;
ret_ptr3:
                    ptr_sigA = readline("install now? [Y/n]: ");

                    while (1) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            if (wcfg.os_type == OS_SIGNAL_WINDOWS) {
                                wd_install_server("windows");
                            } else if (wcfg.os_type == OS_SIGNAL_LINUX) {
                                wd_install_server("linux");
                            }
                            break;
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            break;
                        } else {
                            printf_error("Invalid input. Please type Y/y to install or N/n to cancel.");
                            goto ret_ptr3;
                        }
                    }
                }
                
                return RETN;
        } else if (strncmp(ptr_command, "crunn", 7) == 0) {
            wd_set_title("Watchdogs | @ crunn");
            const char *arg = NULL;
            const char *compile_args = NULL;
            wd_RunCompiler(arg, compile_args);
            if (wcfg.compiler_error < 1)
                goto _runners_;
        } else if (strcmp(ptr_command, "stop") == 0) {
            wd_set_title("Watchdogs | @ stop");
            wd_stop_server_tasks();
            return RETN;
        } else if (strcmp(ptr_command, "restart") == 0) {
            wd_stop_server_tasks();
            sleep(2);
            goto _runners_;
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            wd_set_title("Watchdogs | @ undefined");
            printf("did you mean '" COL_YELLOW "%s" COL_DEFAULT "'", _dist_command);
            char *confirm = readline(" [y/n]: ");
            if (confirm) {
                if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
                    ptr_command = strdup(_dist_command);
                    goto _reexecute_command;
                } else if (strcmp(confirm, "N") == 0 || strcmp(confirm, "n") == 0) {
                    return RETN;
                } else { return RETN; }
            } else { return RETN; }
        } else {
            if (strfind(ptr_command, "bash") ||
                strfind(ptr_command, "sh") ||
                strfind(ptr_command, "zsh") ||
                strfind(ptr_command, "make"))
                    goto _ptr_command;
            char _p_command[256];
            snprintf(_p_command, 256, "%s", ptr_command);
            int __T_ptr_command = wd_run_command(_p_command);
            if (__T_ptr_command == 0) {}
            else {
                wd_set_title("Watchdogs | @ command not found");
            }
            return RETN;
        }

        if (ptr_command) {
            free(ptr_command);
            ptr_command = NULL;
        }

        return -RETN;
}

void __main(int sig_unused) {
        (void)sig_unused;
        wd_set_title(NULL);
        __function__();
loop_main:
        int ret = -RETW;
            ret = __command__();
        if (ret == -RETN || ret == RETZ || ret == RETN) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            printf_color(COL_YELLOW, " ==> [C]Finished in %.3fs\n", command_dur);
            goto loop_main;
        } else if (ret == RETW) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            exit(1);
        } else {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            goto loop_main;
        }
}

int main(void) {
        __main(0);
        return RETZ;
}