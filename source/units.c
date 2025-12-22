#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <ftw.h>
#include <fcntl.h>
#include <math.h>
#include <locale.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <curl/curl.h>

#if __has_include(<spawn.h>)
    #include <spawn.h>
#elif __has_include(<android-spawn.h>)
    #include <android-spawn.h>
#endif

#include "extra.h"
#include "utils.h"
#include "crypto.h"
#include "library.h"
#include "archive.h"
#include "curl.h"
#include "runner.h"
#include "compiler.h"
#include "depend.h"
#include "debug.h"
#include "units.h"

#ifndef WATCHDOGS_RELEASE
    #define WATCHDOGS_RELEASE "WATCHDOGS-251214"
#endif
const char *watchdogs_release = WATCHDOGS_RELEASE;

static struct timespec cmd_start, cmd_end;
static double command_dur;

int wg_ptr_command_init = 0;

int __command__(char *chain_pre_command)
{
        __debug_main_chain(1);

        int wg_compile_running = 0;
        char ptr_prompt[WG_MAX_PATH * 2];
        size_t size_ptr_command = sizeof(ptr_prompt);
        char *ptr_command;
        const char *command_distance;
        int dist = INT_MAX;

_ptr_command:
        if (chain_pre_command && chain_pre_command[0] != '\0') {
            ptr_command = strdup(chain_pre_command);
            printf(FCOLOUR_BLUE ">>>" FCOLOUR_DEFAULT " %s\n", ptr_command);
        } else {
            static int k = 0;
            if (k != 1) {
                ++k;
                println(stdout, "Type \"help\" for more information.");
            }
            while (true) {
                snprintf(ptr_prompt, size_ptr_command,
                        FCOLOUR_BLUE ">>>" FCOLOUR_DEFAULT " ");

                ptr_command = readline(ptr_prompt);

                if (ptr_command == NULL || ptr_command[0] == '\0') {
                    wg_free(ptr_command);
                    continue;
                }
                break;
            }
        }

        fflush(stdout);

        wg_a_history(ptr_command);

        command_distance = wg_find_near_command(ptr_command, __command, __command_len, &dist);

_reexecute_command:
        ++wg_ptr_command_init;
        __debug_main_chain(0);
        clock_gettime(CLOCK_MONOTONIC, &cmd_start);
        if (strncmp(ptr_command, "help", strlen("help")) == 0) {
            wg_console_title("Watchdogs | @ help");

            char *args;
                args = ptr_command + strlen("help");
            while (*args == ' ') ++args;

            if (strlen(args) == 0) {
                printf("Usage: help <command> | help title\n\n");
                printf("Commands:\n");
                printf("  exit             exit from watchdogs\n");
                printf("  kill             refresh terminal watchdogs\n");
                printf("  title            set-title terminal watchdogs\n");
                printf("  sha256           generate sha256 hash\n");
                printf("  crc32            generate crc32 checksum\n");
                printf("  djb2             generate djb2 hash file\n");
                printf("  time             print current time\n");
                printf("  config           re-create watchdogs.toml\n");
                printf("  replicate        installer dependencies\n");
                printf("  gamemode         download SA-MP gamemode\n");
                printf("  pawncc           download SA-MP pawncc\n");
                printf("  debug            debugging & logging server logs\n");
                printf("  compile          compile your project\n");
                printf("  running          running your project\n");
                printf("  compiles         compile & running your project\n");
                printf("  stop             stopped server task\n");
                printf("  restart          restart server task\n");
                printf("  wanion           ask to wanion (gemini based)\n");
                printf("  tracker          account tracking\n");
                printf("  compress         create a compressed archive\n");
                printf("  send             send file to Discord channel via webhook\n");
            } else if (strcmp(args, "exit") == 0) {
                println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"\n\tJust type 'exit' and you're outta here!");
            } else if (strcmp(args, "kill") == 0) {
                println(stdout, "kill: refresh terminal watchdogs. | Usage: \"kill\"\n\tWhen things get stuck or buggy, this is your fix!");
            } else if (strcmp(args, "title") == 0) {
                println(stdout, "title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]\n\tPersonalize your terminal window title.");
            } else if (strcmp(args, "sha256") == 0) {
                println(stdout, "sha256: generate sha256. | Usage: \"sha256\" | [<args>]\n\tGet that SHA256 hash for your files or text.");
            } else if (strcmp(args, "crc32") == 0) {
                println(stdout, "crc32: generate crc32. | Usage: \"crc32\" | [<args>]\n\tQuick CRC32 checksum generation.");
            } else if (strcmp(args, "djb2") == 0) {
                println(stdout, "djb2: generate djb2 hash file. | Usage: \"djb2\" | [<args>]\n\tDJB2 hashing algorithm at your service.");
            } else if (strcmp(args, "time") == 0) {
                println(stdout, "time: print current time. | Usage: \"time\"\n\tWhat time is it? Time to check!");
            } else if (strcmp(args, "config") == 0) {
                println(stdout, "config: re-create watchdogs.toml. Usage: \"config\"\n\tReset your config file to default settings.");
            } else if (strcmp(args, "replicate") == 0) {
                println(stdout, "replicate: installer dependencies. | Usage: \"replicate\"\n\tDownloads & Install Our Dependencies.");
            } else if (strcmp(args, "gamemode") == 0) {
                println(stdout, "gamemode: download SA-MP gamemode. | Usage: \"gamemode\"\n\tGrab some SA-MP gamemodes quickly.");
            } else if (strcmp(args, "pawncc") == 0) {
                println(stdout, "pawncc: download SA-MP pawncc. | Usage: \"pawncc\"\n\tGet the Pawn compiler for SA-MP.");
            } else if (strcmp(args, "debug") == 0) {
                println(stdout, "debug: debugging & logging server debug. | Usage: \"debug\"\n\tKeep an eye on your server logs.");
            } else if (strcmp(args, "compile") == 0) {
                println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]\n\tTurn your code into something runnable!");
            } else if (strcmp(args, "running") == 0) {
                println(stdout, "running: running your project. | Usage: \"running\" | [<args>]\n\tFire up your project and see it in action.");
            } else if (strcmp(args, "compiles") == 0) {
                println(stdout, "compiles: compile & running your project. | Usage: \"compiles\" | [<args>]\n\tTwo-in-one: compile then run immediately!");
            } else if (strcmp(args, "stop") == 0) {
                println(stdout, "stop: stopped server task. | Usage: \"stop\"\n\tHalt everything! Stop your server tasks.");
            } else if (strcmp(args, "restart") == 0) {
                println(stdout, "restart: restart server task. | Usage: \"restart\"\n\tFresh start! Restart your server.");
            } else if (strcmp(args, "wanion") == 0) {
                println(stdout, "wanion: ask to wanion. | Usage: \"wanion\" | [<args>] | gemini based\n\tGot questions? Ask Wanion (Gemini AI powered).");
            } else if (strcmp(args, "tracker") == 0) {
                println(stdout, "tracker: account tracking. | Usage: \"tracker\" | [<args>]\n\tTrack accounts across platforms.");
            } else if (strcmp(args, "compress") == 0) {
                println(stdout, "compress: create a compressed archive from a file or folder. | "
                    "Usage: \"compress <input> <output>\"\n\tGenerates a compressed file (e.g., .zip/.tar.gz) from the specified source.");
            } else if (strcmp(args, "send") == 0) {
                println(stdout, "send: send file to Discord channel via webhook. | "
                    "Usage: \"send <file> <webhook_url>\"\n\tUploads a file directly to a Discord channel using a webhook.");
            } else {
                printf("help can't found for: '");
                pr_color(stdout, FCOLOUR_YELLOW, "%s", args);
                printf("'\n     Oops! That command doesn't exist. Try 'help' to see available commands.\n");
            }
            goto chain_done;
        } else if (strcmp(ptr_command, "exit") == 0) {
            return 2;
        } else if (strcmp(ptr_command, "kill") == 0) {
            wg_console_title("Watchdogs | @ kill");

            wg_clear_screen();

            wgconfig.wg_sel_stat = 0;
            wg_compile_running = 0;

            __debug_main_chain(1);

           if (chain_pre_command && chain_pre_command[0] != '\0')
                goto chain_done;
            else
                goto _ptr_command;
        } else if (strncmp(ptr_command, "title", strlen("title")) == 0) {
            char *args = ptr_command + strlen("title");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: title [<title>]");
            } else {
                char title_set[WG_PATH_MAX * 2];
                snprintf(title_set, sizeof(title_set), "%s", args);
                wg_console_title(title_set);
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "sha256", strlen("sha256")) == 0) {
            char *args = ptr_command + strlen("sha256");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: sha256 [<words>]");
            } else {
                unsigned char digest[32];

                if (crypto_generate_sha256_hash(args, digest) != 1) {
                    goto chain_done;
                }

                printf("          Crypto Input : %s\n", args);
                printf("Crypto Output (SHA256) : ");
                crypto_print_hex(digest, sizeof(digest), 1);
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "crc32", strlen("crc32")) == 0) {
            char *args = ptr_command + strlen("crc32");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: crc32 [<words>]");
            } else {
                static int init_crc32 = 0;
                if (init_crc32 != 1) {
                    crypto_crc32_init_table();
                    init_crc32 = 1;
                }

                uint32_t crc32_generate;
                crc32_generate = crypto_generate_crc32(args, sizeof(args) - 1);

                char crc_str[11];
                sprintf(crc_str, "%08X", crc32_generate);

                printf("         Crypto Input : %s\n", args);
                printf("Crypto Output (CRC32) : ");
                printf("%s\n", crc_str);
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "djb2", strlen("djb2")) == 0) {
            char *args = ptr_command + strlen("djb2");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: djb2 [<file>]");
            } else {
                unsigned long djb2_generate;
                djb2_generate = crypto_djb2_hash_file(args);

                if (djb2_generate) {
                    printf("        Crypto Input : %s\n", args);
                    printf("Crypto Output (DJB2) : ");
                    printf("%#lx\n", djb2_generate);
                }
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "time") == 0) {
            time_t current_time;
            struct tm *time_info;
            char time_string[100];

            time(&current_time);
            time_info = localtime(&current_time);

            strftime(time_string, sizeof(time_string),
                    "%A, %d %B %Y %H:%M:%S", time_info);

            printf("Now: %s\n", time_string);

            goto chain_done;
        } else if (strcmp(ptr_command, "config") == 0) {
            if (access("watchdogs.toml", F_OK) == 0)
                remove("watchdogs.toml");

            __debug_main_chain(1);

            printf(FCOLOUR_BRED "");
            wg_printfile("watchdogs.toml");
            printf(FCOLOUR_DEFAULT "\n");

            goto chain_done;
        } else if (strncmp(ptr_command, "replicate", strlen("replicate")) == 0) {
            wg_console_title("Watchdogs | @ replicate depends");

            char *args = ptr_command + strlen("replicate");
            while (*args == ' ') ++args;

            int is_null_args = -1;
            char *args2 = NULL;
            args2 = strtok(args, " ");
            if (args2 == NULL || args[0] == '\0' || strlen(args2) < 1 || strcmp(args2, ".") == 0)
              is_null_args = 1;

            char *raw_branch = NULL;
            char *procure_args = strtok(args, " ");
            while (procure_args) {
                if (strcmp(procure_args, "--branch") == 0) {
                    procure_args = strtok(NULL, " ");
                    if (procure_args) raw_branch = procure_args;
                }
                procure_args = strtok(NULL, " ");
            }

            if (is_null_args != 1) {
                if (raw_branch)
                    wg_install_depends(args, raw_branch);
                else
                    wg_install_depends(args, "main");
            } else {
                char errbuf[WG_PATH_MAX];
                toml_table_t *wg_toml_config;
                FILE *this_proc_file = fopen("watchdogs.toml", "r");
                wg_toml_config = toml_parse_file(this_proc_file, errbuf, sizeof(errbuf));
                if (this_proc_file) fclose(this_proc_file);

                if (!wg_toml_config) {
                    pr_error(stdout, "failed to parse the watchdogs.toml....: %s", errbuf);
                    __debug_function(); /* call debugger function */
                    return 0;
                }

                toml_table_t *wg_depends;
                size_t arr_sz, i;
                char *merged = NULL;

                wg_depends = toml_table_in(wg_toml_config, "dependencies");
                if (!wg_depends)
                    goto out;

                toml_array_t *wg_toml_packages = toml_array_in(wg_depends, "packages");
                if (!wg_toml_packages)
                    goto out;

                arr_sz = toml_array_nelem(wg_toml_packages);
                for (i = 0; i < arr_sz; i++) {
                    toml_datum_t val;

                    val = toml_string_at(wg_toml_packages, i);
                    if (!val.ok)
                            continue;

                    if (!merged) {
                            merged = wg_realloc(NULL, strlen(val.u.s) + 1);
                            if (!merged)
                                    goto free_val;

                            snprintf(merged, strlen(val.u.s) + 1, "%s", val.u.s);
                    } else {
                            char *tmp;
                            size_t old_len = strlen(merged);
                            size_t new_len = old_len + strlen(val.u.s) + 2;

                            tmp = wg_realloc(merged, new_len);
                            if (!tmp)
                                    goto free_val;

                            merged = tmp;
                            snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
                    }

free_val:
                    wg_free(val.u.s);
                    val.u.s = NULL;
                }

                if (!merged)
                    merged = strdup("");

                wgconfig.wg_toml_packages = merged;

                pr_info(stdout, "Loaded packages: %s", wgconfig.wg_toml_packages);

                if (raw_branch)
                    wg_install_depends(wgconfig.wg_toml_packages, raw_branch);
                else
                    wg_install_depends(wgconfig.wg_toml_packages, "main");

out:
                toml_free(wg_toml_config);
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            wg_console_title("Watchdogs | @ gamemode");
ret_ptr:
            printf("\033[1;33m== Select a Platform ==\033[0m\n");
            printf("  \033[36m[l]\033[0m Linux\n");
            printf("  \033[36m[w]\033[0m Windows  \033[90m(WSL/WSL2/MSYS2 supported — not for WSL Docker)\033[0m\n");

            wgconfig.wg_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                int ret = wg_install_server("linux");
loop_igm:
                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                    goto loop_igm;
                else if (ret == 0)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "W", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                int ret = wg_install_server("windows");
loop_igm2:
                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                    goto loop_igm2;
                else if (ret == 0)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "E", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
            } else {
                pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                wg_free(platform);
                platform = NULL;
                goto ret_ptr;
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            wg_console_title("Watchdogs | @ pawncc");
ret_ptr2:
            printf("\033[1;33m== Select a Platform ==\033[0m\n");
            printf("  \033[36m[l]\033[0m Linux\n");
            printf("  \033[36m[w]\033[0m Windows  \033[90m(WSL/WSL2/MSYS2 supported — not for WSL Docker)\033[0m\n");
            printf("  \033[36m[t]\033[0m Termux\n");

            wgconfig.wg_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                ptr_command = NULL;
                int ret = wg_install_pawncc("linux");
loop_ipcc:
                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                    goto loop_ipcc;
                else if (ret == 0)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "W", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                ptr_command = NULL;
                int ret = wg_install_pawncc("windows");
loop_ipcc2:
                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                    goto loop_ipcc2;
                else if (ret == 0)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "T", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                ptr_command = NULL;
                int ret = wg_install_pawncc("termux");
loop_ipcc3:
                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                    goto loop_ipcc3;
                else if (ret == 0)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "E", true)) {
                wg_free(ptr_command);
                wg_free(platform);
                if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto chain_done;
                    else
                        goto _ptr_command;
            } else {
                pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                wg_free(platform);
                platform = NULL;
                goto ret_ptr2;
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "debug") == 0) {
            wg_console_title("Watchdogs | @ debug");
            printf("running logs command: 0000WGDEBUGGINGSERVER...\n");
            #ifdef WG_ANDROID
            #ifndef _DBG_PRINT
                        wg_run_command("./watchdogs.tmux 0000WGDEBUGGINGSERVER");
            #else
                        wg_run_command("./watchdogs.debug.tmux 0000WGDEBUGGINGSERVER");
            #endif
            #elif defined(WG_LINUX)
            #ifndef _DBG_PRINT
                        wg_run_command("./watchdogs 0000WGDEBUGGINGSERVER");
            #else
                        wg_run_command("./watchdogs.debug 0000WGDEBUGGINGSERVER");
            #endif
            #elif defined(WG_WINDOWS)
            #ifndef _DBG_PRINT
                        wg_run_command("watchdogs.win 0000WGDEBUGGINGSERVER");
            #else
                        wg_run_command("watchdogs.debug.win 0000WGDEBUGGINGSERVER");
            #endif
            #endif
            goto chain_done;
        } else if (strcmp(ptr_command, "0000WGDEBUGGINGSERVER") == 0) {
            wg_server_crash_check();
            return 3;
        } else if (strncmp(ptr_command, "compile", strlen("compile")) == 0 &&
                   !isalpha((unsigned char)ptr_command[strlen("compile")])) {

            wg_console_title("Watchdogs | @ compile | logging file: .watchdogs/compiler.log");

            char *args;
            char *compile_args;
            char *second_arg = NULL;
            char *four_arg = NULL;
            char *five_arg = NULL;
            char *six_arg = NULL;
            char *seven_arg = NULL;
            char *eight_arg = NULL;
            char *nine_arg = NULL;

            args = ptr_command + strlen("compile");

            while (*args == ' ') {
                args++;
            }

            compile_args = strtok(args, " ");
            second_arg   = strtok(NULL, " ");
            four_arg     = strtok(NULL, " ");
            five_arg     = strtok(NULL, " ");
            six_arg      = strtok(NULL, " ");
            seven_arg    = strtok(NULL, " ");
            eight_arg    = strtok(NULL, " ");
            nine_arg     = strtok(NULL, " ");

            wg_run_compiler(args, compile_args, second_arg, four_arg,
                            five_arg, six_arg, seven_arg, eight_arg,
                            nine_arg);

            goto chain_done;
        } else if (strncmp(ptr_command, "running", strlen("running")) == 0) {
_runners_:
                wg_stop_server_tasks();

                if (!path_access(wgconfig.wg_toml_binary)) {
                    pr_error(stdout, "can't locate sa-mp/open.mp binary file!");
                    goto chain_done;
                }
                if (!path_access(wgconfig.wg_toml_config)) {
                    pr_warning(stdout, "can't locate %s - config file!", wgconfig.wg_toml_config);
                    goto chain_done;
                }

                if (dir_exists(".watchdogs") == 0)
                    MKDIR(".watchdogs");

                int _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                if (_wg_log_acces)
                  remove(wgconfig.wg_toml_logs);
                _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                if (_wg_log_acces)
                  remove(wgconfig.wg_toml_logs);

                size_t cmd_len = 7;
                char *args = ptr_command + cmd_len;
                while (*args == ' ') ++args;
                char *args2 = NULL;
                args2 = strtok(args, " ");

                char *size_arg1 = NULL;
                if (args2 == NULL || args2[0] == '\0')
                    size_arg1 = wgconfig.wg_toml_proj_output;
                else
                    size_arg1 = args2;

                size_t needed = snprintf(NULL, 0,
                                "Watchdogs | "
                                "@ running | "
                                "args: %s | "
                                "config: %s | "
                                "CTRL + C to stop. | \"debug\" for debugging",
                                size_arg1,
                                wgconfig.wg_toml_config) + 1;
                char *title_running_info = wg_malloc(needed);
                if (!title_running_info) { goto chain_done; }
                snprintf(title_running_info, needed,
                                    "Watchdogs | "
                                    "@ running | "
                                    "args: %s | "
                                    "config: %s | "
                                    "CTRL + C to stop. | \"debug\" for debugging",
                                    size_arg1,
                                    wgconfig.wg_toml_config);
                if (title_running_info) {
                		wg_console_title(title_running_info);
                    wg_free(title_running_info);
                    title_running_info = NULL;
                }

                int _wg_config_acces = path_access(wgconfig.wg_toml_config);
                if (!_wg_config_acces) {
                    pr_error(stdout, "%s not found!", wgconfig.wg_toml_config);
                    goto chain_done;
                }

                pr_color(stdout, FCOLOUR_YELLOW, "running..\n");
                println(stdout, "\toperating system: %s", wgconfig.wg_toml_os_type);
                println(stdout, "\tbinary file: %s", wgconfig.wg_toml_binary);
                println(stdout, "\tconfig file: %s", wgconfig.wg_toml_config);

                char size_run[WG_PATH_MAX];
                struct sigaction sa;
                if (wg_server_env() == 1) {
                    if (args2 == NULL || (args2[0] == '.' && args2[1] == '\0')) {
start_main:
                        sa.sa_handler = unit_sigint_handler;
                        sigemptyset(&sa.sa_mask);
                        sa.sa_flags = SA_RESTART;

                        if (sigaction(SIGINT, &sa, NULL) == -1) {
                                perror("sigaction");
                                exit(EXIT_FAILURE);
                        }

                        time_t start, end;
                        double elapsed;

                        int ret_serv = 0;
back_start:
                        start = time(NULL);
                        #ifdef WG_WINDOWS
                        snprintf(size_run, sizeof(size_run), "%s", wgconfig.wg_toml_binary);
                        #else
                        CHMOD_FULL(wgconfig.wg_toml_binary);
                        if (path_access("announce"))
                            CHMOD_FULL("announce");
                        snprintf(size_run, sizeof(size_run), "./%s", wgconfig.wg_toml_binary);
                        #endif
                        int rate_runner_failed = wg_run_command(size_run);
                        if (rate_runner_failed == 0) {
                            if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX)) {
                                    printf("~ logging...\n");
                                    sleep(3);
                                    wg_display_server_logs(0);
                            }
                        } else {
                            /* Skip retry logic in Pterodactyl environments */
                            /* so why? -- Pterodactyl hosting doesn't need this,
                            *  because when your server restarts in Pterodactyl it will automatically retry
                            */
                            pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                            if (is_pterodactyl_env()) {
                                goto server_done;
                            }
                            elapsed = difftime(end, start);
                            if (elapsed <= 5.0 && ret_serv == 0) {
                                ret_serv = 1;
                                printf("\ttry starting again..");
                                _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                                if (_wg_log_acces)
                                  remove(wgconfig.wg_toml_logs);
                                _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                                if (_wg_log_acces)
                                  remove(wgconfig.wg_toml_logs);
                                end = time(NULL);
                                goto back_start;
                            }
                        }

server_done:
                        end = time(NULL);
                        if (sigint_handler == 0)
                            raise(SIGINT);

                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("debug");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);
                    } else {
                        if (wg_compile_running == 1) {
                            wg_compile_running = 0;
                            goto start_main;
                        }
                        wg_run_samp_server(args2, wgconfig.wg_toml_binary);
                        restore_server_config();
                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("debug");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);
                    }
                } else if (wg_server_env() == 2) {
                    if (args2 == NULL || (args2[0] == '.' && args2[1] == '\0')) {
start_main2:
                        sa.sa_handler = unit_sigint_handler;
                        sigemptyset(&sa.sa_mask);
                        sa.sa_flags = SA_RESTART;

                        if (sigaction(SIGINT, &sa, NULL) == -1) {
                                perror("sigaction");
                                exit(EXIT_FAILURE);
                        }

                        time_t start, end;
                        double elapsed;

                        int ret_serv = 0;
back_start2:
                        start = time(NULL);
                        #ifdef WG_WINDOWS
                        snprintf(size_run, sizeof(size_run), "%s", wgconfig.wg_toml_binary);
                        #else
                        CHMOD_FULL(wgconfig.wg_toml_binary);
                        if (path_access("announce"))
                            CHMOD_FULL("announce");
                        snprintf(size_run, sizeof(size_run), "./%s", wgconfig.wg_toml_binary);
                        #endif
                        int rate_runner_failed = wg_run_command(size_run);
                        if (rate_runner_failed != 0) {
                            /* Skip retry logic in Pterodactyl environments */
                            /* so why? -- Pterodactyl hosting doesn't need this,
                            *  because when your server restarts in Pterodactyl it will automatically retry
                            */
                            pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                            if (is_pterodactyl_env()) {
                                goto server_done2;
                            }
                            elapsed = difftime(end, start);
                            if (elapsed <= 5.0 && ret_serv == 0) {
                                ret_serv = 1;
                                printf("\ttry starting again..");
                                _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                                if (_wg_log_acces)
                                  remove(wgconfig.wg_toml_logs);
                                _wg_log_acces = path_access(wgconfig.wg_toml_logs);
                                if (_wg_log_acces)
                                  remove(wgconfig.wg_toml_logs);
                                end = time(NULL);
                                goto back_start2;
                            }
                        }

server_done2:
                        end = time(NULL);
                        if (sigint_handler == 0) {
                            raise(SIGINT);
                        }

                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("debug");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);
                    } else {
                        if (wg_compile_running == 1) {
                            wg_compile_running = 0;
                            goto start_main2;
                        }
                        wg_run_omp_server(args2, wgconfig.wg_ptr_omp);
                        restore_server_config();
                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("debug");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);                    }
                } else {
                    pr_error(stdout,
                        "\033[1;31merror:\033[0m sa-mp/open.mp server not found!\n"
                        "  \033[2mhelp:\033[0m install it before continuing\n");
                    printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

                    char *pointer_signalA;
ret_ptr3:
                    pointer_signalA = readline("");

                    while (true) {
                        if (strcmp(pointer_signalA, "Y") == 0 || strcmp(pointer_signalA, "y") == 0) {
                            wg_free(pointer_signalA);
                            if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_WINDOWS)) {
                                int ret = wg_install_server("windows");
n_loop_igm:
                                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                                    goto n_loop_igm;
                            } else if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX)) {
                                int ret = wg_install_server("linux");
n_loop_igm2:
                                if (ret == -1 && wgconfig.wg_sel_stat != 0)
                                    goto n_loop_igm2;
                            }
                            break;
                        } else if (strcmp(pointer_signalA, "N") == 0 || strcmp(pointer_signalA, "n") == 0) {
                            wg_free(pointer_signalA);
                            break;
                        } else {
                            pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                            wg_free(pointer_signalA);
                            goto ret_ptr3;
                        }
                    }
                }

                goto chain_done;
        } else if (strncmp(ptr_command, "compiles", strlen("compiles")) == 0) {
            wg_console_title("Watchdogs | @ compiles");

            char *args = ptr_command + strlen("compiles");
            while (*args == ' ') ++args;
            char *args2 = NULL;
            args2 = strtok(args, " ");

            if (args2 == NULL || args2[0] == '\0') {
                const char *argsc[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

                wg_compile_running = 1;

                wg_run_compiler(argsc[0], argsc[1], argsc[2], argsc[3],
                                argsc[4], argsc[5], argsc[6], argsc[7],
                                argsc[8]);

                if (wgconfig.wg_compiler_stat < 1) {
                    goto _runners_;
                }
            } else {
                const char *argsc[] = { NULL, args2, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

                wg_compile_running = 1;

                wg_run_compiler(argsc[0], argsc[1], argsc[2], argsc[3],
                                argsc[4], argsc[5], argsc[6], argsc[7],
                                argsc[8]);

                if (wgconfig.wg_compiler_stat < 1) {

                    char size_command[WG_PATH_MAX];
                    snprintf(size_command,
                        sizeof(size_command), "running %s", args);
                    ptr_command = strdup(size_command);

                    goto _runners_;
                }
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "stop") == 0) {
            wg_console_title("Watchdogs | @ stop");

            wg_stop_server_tasks();

            goto chain_done;
        } else if (strcmp(ptr_command, "restart") == 0) {
            wg_console_title("Watchdogs | @ restart");

            wg_stop_server_tasks();
            sleep(2);
            goto _runners_;
        } else if (strncmp(ptr_command, "wanion", strlen("wanion")) == 0) {
            char *args = ptr_command + strlen("wanion");
            while (*args == ' ') ++args;

            char *raw_file = NULL;
            char *procure_args = strtok(args, " ");
            while (procure_args) {
                if (strcmp(procure_args, "--file") == 0) {
                    procure_args = strtok(NULL, " ");
                    if (procure_args) raw_file = procure_args;
                }
                procure_args = strtok(NULL, " ");
            }

            if (*args == '\0') {
                println(stdout, "Usage: wanion [<text>]");
            } else {
                int retry = 0;
                char size_rest_api_perform[WG_PATH_MAX];
                int is_chatbot_groq_based = 0;
                if (strcmp(wgconfig.wg_toml_chatbot_ai, "gemini") == 0)
rest_def:
                    snprintf(size_rest_api_perform, sizeof(size_rest_api_perform),
                                      "https://generativelanguage.googleapis.com/"
                                      "v1beta/models/%s:generateContent",
                                      wgconfig.wg_toml_models_ai);
                else if (strcmp(wgconfig.wg_toml_chatbot_ai, "groq") == 0) {
                    is_chatbot_groq_based = 1;
                    snprintf(size_rest_api_perform, sizeof(size_rest_api_perform),
                                      "https://api.groq.com/"
                                      "openai/v1/chat/completions");
                } else { goto rest_def; }

#if defined(_DBG_PRINT)
                printf("rest perform: %s\n", size_rest_api_perform);
#endif
                char wanion_escaped_argument[WG_MAX_PATH];
                wg_escaping_json(wanion_escaped_argument, args, sizeof(wanion_escaped_argument));

                char wanion_json_payload[WG_MAX_PATH * 2];
                if (is_chatbot_groq_based == 1) {
                    snprintf(wanion_json_payload, sizeof(wanion_json_payload),
                             /* prompt here */
                             "{"
                             "\"model\":\"%s\","
                             "\"messages\":[{"
                             "\"role\":\"user\","
                             "\"content\":\"Your name in here Wanion (use it) made from Groq my asking: %s\""
                             "}],"
                             "\"max_tokens\":1024}",
                             wgconfig.wg_toml_models_ai,
                             wanion_escaped_argument);
                } else {
                    snprintf(wanion_json_payload, sizeof(wanion_json_payload),
                             /* prompt here */
                             "{"
                             "\"contents\":[{\"role\":\"user\","
                             "\"parts\":[{\"text\":\"Your name in here Wanion (use it) made from Google my asking: %s\"}]}],"
                             "\"generationConfig\": {\"maxOutputTokens\": 1024}}",
                             wanion_escaped_argument);
                }

                struct timespec start = { 0 }, end = { 0 };
                double wanion_dur;
                struct buf b = { 0 };
                struct curl_slist *hdr;
                char size_tokens[WG_PATH_MAX + 26];
wanion_retrying:
#if defined(_DBG_PRINT)
                printf("json payload: %s\n", wanion_json_payload);
#endif
                clock_gettime(CLOCK_MONOTONIC, &start);
                CURL *h = curl_easy_init();
                if (!h) {
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto chain_done;
                }

                hdr = curl_slist_append(NULL, "Content-Type: application/json");
                if (is_chatbot_groq_based == 1)
                    snprintf(size_tokens, sizeof(size_tokens), "Authorization: Bearer %s", wgconfig.wg_toml_key_ai);
                else
                    snprintf(size_tokens, sizeof(size_tokens), "x-goog-api-key: %s", wgconfig.wg_toml_key_ai);
                hdr = curl_slist_append(hdr, size_tokens);

                curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "gzip");
                curl_easy_setopt(h, CURLOPT_URL, size_rest_api_perform);
                curl_easy_setopt(h, CURLOPT_HTTPHEADER, hdr);
                curl_easy_setopt(h, CURLOPT_TCP_KEEPALIVE, 1L);
                curl_easy_setopt(h, CURLOPT_POSTFIELDS, wanion_json_payload);
                curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
                buf_init(&b);
                curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_callbacks);
                curl_easy_setopt(h, CURLOPT_WRITEDATA, &b);
                buf_free(&b);
                curl_easy_setopt(h, CURLOPT_TIMEOUT, 30L);

                curl_verify_cacert_pem(h);

                CURLcode res = curl_easy_perform(h);
                if (res != CURLE_OK) {
                    fprintf(stderr, "HTTP request failed: %s\n", curl_easy_strerror(res));
                    if (retry != 3) {
                        ++retry;
                        printf("~ try retrying...\n");
                        goto wanion_retrying;
                    }
                    curl_slist_free_all(hdr);
                    curl_easy_cleanup(h);
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto chain_done;
                }

                long http_code = 0;
                curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &http_code);
                if (http_code != 200) {
                    fprintf(stderr, "API returned HTTP %ld:\n%s\n", http_code, b.data);
                    int rate_api_limit = 0;
                    if (strfind(b.data, "You exceeded your current quota, please check your plan and billing details", true) ||
                        strfind(b.data, "Too Many Requests", true))
                        ++rate_api_limit;
                    if (rate_api_limit)
                        printf("~ limit detected!\n");
                    else {
                        if (retry != 3) {
                            ++retry;
                            printf("~ try retrying...\n");
                            goto wanion_retrying;
                        }
                    }
                    curl_slist_free_all(hdr);
                    curl_easy_cleanup(h);
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto chain_done;
                }

                cJSON *root = cJSON_Parse(b.data);
                if (!root) {
                    fprintf(stderr, "JSON parse error\n");
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto wanion_curl_end;
                }

                cJSON *error = cJSON_GetObjectItem(root, "error");
                if (error) {
                    cJSON *message = cJSON_GetObjectItem(error, "message");
                    if (message && cJSON_IsString(message)) {
                        fprintf(stderr, "API Error: %s\n", message->valuestring);
                    }
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto wanion_json_end;
                }

#if defined(_DBG_PRINT)
                char *response_str = cJSON_Print(root);
                printf("response: %s\n", response_str);
                cJSON_free(response_str);
#endif
                if (is_chatbot_groq_based == 1) {
                    char *response_text = NULL;
                    cJSON *choices = cJSON_GetObjectItem(root, "choices");
                    if (choices && cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                        cJSON *first_choice = cJSON_GetArrayItem(choices, 0);
                        cJSON *message = cJSON_GetObjectItem(first_choice, "message");
                        if (message) {
                            cJSON *content = cJSON_GetObjectItem(message, "content");
                            if (cJSON_IsString(content)) {
                                response_text = content->valuestring;
                            }
                        }
                    }
                    if (!response_text) {
                        cJSON *data = cJSON_GetObjectItem(root, "data");
                        if (data && cJSON_IsArray(data) && cJSON_GetArraySize(data) > 0) {
                            cJSON *text_item = cJSON_GetArrayItem(data, 0);
                            cJSON *text = cJSON_GetObjectItem(text_item, "text");
                            if (cJSON_IsString(text)) {
                                response_text = text->valuestring;
                            }
                        }
                    }
                    if (!response_text) {
                        cJSON *text = cJSON_GetObjectItem(root, "text");
                        if (cJSON_IsString(text)) {
                            response_text = text->valuestring;
                        }
                    }
                    if (response_text) {
                        const char *p = response_text;
                        int in_bold = 0;
                        int in_italic = 0;
                        int in_code = 0;
                        while (*p) {
                            if (!in_code && strncmp(p, "```", 3) == 0) {
                                in_code = 1;
                                printf("\033[38;5;244m");
                                p += 3;
                                continue;
                            }
                            if (in_code && strncmp(p, "```", 3) == 0) {
                                in_code = 0;
                                printf("\033[0m");
                                p += 3;
                                continue;
                            }
                            if (in_code) {
                                putchar(*p++);
                                continue;
                            }
                            if (strncmp(p, "**", 2) == 0) {
                                in_bold = !in_bold;
                                printf(in_bold ? "\033[1m" : "\033[0m");
                                p += 2;
                                continue;
                            }
                            if (*p == '_') {
                                in_italic = !in_italic;
                                printf(in_italic ? "\033[3m" : "\033[0m");
                                p++;
                                continue;
                            }
                            if (strncmp(p, "~~", 2) == 0) {
                                printf("\033[9m");
                                p += 2;
                                continue;
                            }
                            if (strncmp(p, "==", 2) == 0) {
                                printf("\033[43m");
                                p += 2;
                                continue;
                            }
                            if (p == response_text || *(p - 1) == '\n') {
                                if (*p == '#' || memcmp(p, "##", 2) == 0 ||
                                    memcmp(p, "###", 3) == 0) {
                                    printf("\033[1m");
                                    p++;
                                    continue;
                                }
                            }
                            putchar(*p++);
                        }
                        printf("\033[0m");
                    } else {
                        fprintf(stderr, "No response text found in Groq response\n");
                        if (retry != 3) {
                            ++retry;
                            printf("~ try retrying...\n");
                            goto wanion_retrying;
                        }
#if defined(_DBG_PRINT)
                        printf("Raw Groq response: %s\n", b.data);
#endif
                    }
                } else {
                    cJSON *candidates = cJSON_GetObjectItem(root, "candidates");
                    if (candidates &&
                        cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates) > 0) {
                        cJSON *candidate = cJSON_GetArrayItem(candidates, 0);
                        cJSON *content = cJSON_GetObjectItem(candidate, "content");
                        if (content) {
                            cJSON *parts = cJSON_GetObjectItem(content, "parts");
                            if (parts && cJSON_IsArray(parts) && cJSON_GetArraySize(parts) > 0) {
                                cJSON *part = cJSON_GetArrayItem(parts, 0);
                                cJSON *text = cJSON_GetObjectItem(part, "text");
                                if (cJSON_IsString(text) && text->valuestring) {
                                    const char *p = text->valuestring;
                                    int in_bold = 0;
                                    int in_italic = 0;
                                    int in_code = 0;
                                    while (*p) {
                                        if (!in_code && strncmp(p, "```", 3) == 0) {
                                            in_code = 1;
                                            printf("\033[38;5;244m");
                                            p += 3;
                                            continue;
                                        }
                                        if (in_code && strncmp(p, "```", 3) == 0) {
                                            in_code = 0;
                                            printf("\033[0m");
                                            p += 3;
                                            continue;
                                        }
                                        if (in_code) {
                                            putchar(*p++);
                                            continue;
                                        }
                                        if (strncmp(p, "**", 2) == 0) {
                                            in_bold = !in_bold;
                                            printf(in_bold ? "\033[1m" : "\033[0m");
                                            p += 2;
                                            continue;
                                        }
                                        if (*p == '_') {
                                            in_italic = !in_italic;
                                            printf(in_italic ? "\033[3m" : "\033[0m");
                                            p++;
                                            continue;
                                        }
                                        if (strncmp(p, "~~", 2) == 0) {
                                            printf("\033[9m");
                                            p += 2;
                                            continue;
                                        }
                                        if (strncmp(p, "==", 2) == 0) {
                                            printf("\033[43m");
                                            p += 2;
                                            continue;
                                        }
                                        if (p == text->valuestring || *(p - 1) == '\n') {
                                            if (*p == '#' || memcmp(p, "##", 2) == 0 ||
                                                memcmp(p, "###", 3) == 0) {
                                                printf("\033[1m");
                                                p++;
                                                continue;
                                            }
                                        }
                                        putchar(*p++);
                                    }
                                    printf("\033[0m");
                                } else fprintf(stderr, "No response text found\n");
                            } else {
                                fprintf(stderr, "No parts found in content\n");
                                if (retry != 3) {
                                    ++retry;
                                    printf("~ try retrying...\n");
                                    goto wanion_retrying;
                                }
                            }
                        } else fprintf(stderr, "No content found in candidate\n");
                    } else {
                        fprintf(stderr, "No candidates found in response\n");
                    }
                }

                clock_gettime(CLOCK_MONOTONIC, &end);

                wanion_dur = (end.tv_sec - start.tv_sec)
                           + (end.tv_nsec - start.tv_nsec) / 1e9;

                printf("\n");
                pr_color(stdout, FCOLOUR_CYAN,
                    " <W> Finished at %.3fs (%.0f ms)\n",
                    wanion_dur, wanion_dur * 1000.0);

wanion_json_end:
                cJSON_Delete(root);
wanion_curl_end:
                wg_free(b.data);
                curl_slist_free_all(hdr);
                curl_easy_cleanup(h);
                goto chain_done;
            }
        } else if (strncmp(ptr_command, "tracker", strlen("tracker")) == 0) {
            char *args = ptr_command + strlen("tracker");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: tracker [<name>]");
            } else {
                CURL *curl;
                curl_global_init(CURL_GLOBAL_DEFAULT);
                curl = curl_easy_init();
                if (!curl) {
                    fprintf(stderr, "Curl initialization failed!\n");
                    goto chain_done;
                }

                int variation_count = 0;
                char variations[MAX_VARIATIONS][MAX_USERNAME_LEN];

                tracker_discrepancy(args, variations, &variation_count);

                printf("[TRACKER] Search base: %s\n", args);
                printf("[TRACKER] Generated %d Variations\n\n", variation_count);

                for (int i = 0; i < variation_count; i++) {
                    printf("=== TRACKING ACCOUNTS: %s ===\n", variations[i]);
                    tracking_username(curl, variations[i]);
                    printf("\n");
                }

                curl_easy_cleanup(curl);
                curl_global_cleanup();
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "compress", strlen("compress")) == 0) {
            char *args = ptr_command + strlen("compress");
            while (*args == ' ') args++;

            if (*args == '\0') {
                printf("Usage: compress --file <input> --output <output> --type <format>\n");
                printf("Example:\n\tcompress --file myfile.txt "
                    "--output myarchive.zip --type zip\n\t"
                    "compress --file myfolder/ "
                    "--output myarchive.tar.gz --type gz\n");
                goto chain_done;
            }

            char *raw_input = NULL, *raw_output = NULL, *raw_type = NULL;

            char *procure_args = strtok(args, " ");
            while (procure_args) {
                if (strcmp(procure_args, "--file") == 0) {
                    procure_args = strtok(NULL, " ");
                    if (procure_args) raw_input = procure_args;
                }
                else if (strcmp(procure_args, "--output") == 0) {
                    procure_args = strtok(NULL, " ");
                    if (procure_args) raw_output = procure_args;
                }
                else if (strcmp(procure_args, "--type") == 0) {
                    procure_args = strtok(NULL, " ");
                    if (procure_args) raw_type = procure_args;
                }
                procure_args = strtok(NULL, " ");
            }

            if (!raw_input || !raw_output || !raw_type) {
                printf("Missing arguments!\n");
                printf("Usage: compress "
                    "--file <input> --output <output> --type <zip|tar|gz|bz2|xz>\n");
                printf("Example:\n\tcompress --file myfile.txt "
                    "--output myarchive.zip --type zip\n\t"
                    "compress --file myfolder/ "
                    "--output myarchive.tar.gz --type gz\n");
                goto chain_done;
            }

            CompressionFormat fmt;

            if (strcmp(raw_type, "zip") == 0)
                fmt = COMPRESS_ZIP;
            else if (strcmp(raw_type, "tar") == 0)
                fmt = COMPRESS_TAR;
            else if (strcmp(raw_type, "gz") == 0)
                fmt = COMPRESS_TAR_GZ;
            else if (strcmp(raw_type, "bz2") == 0)
                fmt = COMPRESS_TAR_BZ2;
            else if (strcmp(raw_type, "xz") == 0)
                fmt = COMPRESS_TAR_XZ;
            else {
                printf("Unknown type: %s\n", raw_type);
                printf("Supported: zip, tar, gz, bz2, xz\n");
                goto chain_done;
            }

            const char
                *procure_items[] = { raw_input };

            int ret = compress_to_archive(raw_output, procure_items, 1, fmt);
            if (ret == 0)
                pr_info(stdout, "Converter file/folder "
                    "to archive (Compression) successfully: %s\n", raw_output);
            else {
                pr_error(stdout, "Compression failed!\n");
                __debug_function(); /* call debugger function */
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "send", strlen("send")) == 0) {
            char *args = ptr_command + strlen("send");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: send [<file_path>]");
            } else {
                if (path_access(args) == 0) {
                    pr_error(stdout, "file not found: %s", args);
                    goto send_done;
                }
                if (!wgconfig.wg_toml_webhooks ||
                        strfind(wgconfig.wg_toml_webhooks, "DO_HERE", true) ||
                        strlen(wgconfig.wg_toml_webhooks) < 1)
                {
                    pr_color(stdout, FCOLOUR_YELLOW, " ~ Discord webhooks not available");
                    goto send_done;
                }

                char *filename = args;
                if (strrchr(args, __PATH_CHR_SEP_LINUX) &&
                    strrchr(args, __PATH_CHR_SEP_WIN32))
                {
                    filename = (strrchr(args, __PATH_CHR_SEP_LINUX) > strrchr(args, __PATH_CHR_SEP_WIN32)) ? \
                                strrchr(args, __PATH_CHR_SEP_LINUX) + 1 : strrchr(args, __PATH_CHR_SEP_WIN32) + 1;
                } else if (strrchr(args, __PATH_CHR_SEP_LINUX)) {
                        filename = strrchr(args, __PATH_CHR_SEP_LINUX) + 1;
                } else if (strrchr(args, __PATH_CHR_SEP_WIN32)) {
                        filename = strrchr(args, __PATH_CHR_SEP_WIN32) + 1;
                } else {
                    ;
                }

                CURL *curl = curl_easy_init();
                if (curl) {
                    CURLcode res;
                    curl_mime *mime;

                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
                    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");

                    mime = curl_mime_init(curl);
                    if (!mime) {
                        fprintf(stderr, "Failed to create MIME handle\n");
                        curl_easy_cleanup(curl);
                        goto send_done;
                    }

                    curl_mimepart *part;

                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);
                    char timestamp[64];
                    snprintf(timestamp, sizeof(timestamp), "%04d/%02d/%02d %02d:%02d:%02d",
                            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                            tm.tm_hour, tm.tm_min, tm.tm_sec);

                    portable_stat_t st;
                    if (portable_stat(filename, &st) == 0) {
                        char content_data[WG_MAX_PATH];
                        snprintf(content_data, sizeof(content_data),
                                "### received send command - %s\n"
                                "> Metadata\n"
                                "- Name: %s\n- Size: %llu bytes\n- Last modified: %llu\n%s",
                                timestamp,
                                filename,
                                (unsigned long long)st.st_size,
                                (unsigned long long)st.st_lmtime,
                                "-# Please note that if you are using webhooks with a public channel,"
                                "always convert the file into an archive with a password known only to you.");

                        part = curl_mime_addpart(mime);
                        curl_mime_name(part, "content");
                        curl_mime_data(part, content_data, CURL_ZERO_TERMINATED);
                    }

                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "file");
                    curl_mime_filedata(part, args);
                    curl_mime_filename(part, filename);

                    curl_easy_setopt(curl, CURLOPT_URL, wgconfig.wg_toml_webhooks);
                    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

                    res = curl_easy_perform(curl);
                    if (res != CURLE_OK)
                        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

                    curl_mime_free(mime);
                    curl_easy_cleanup(curl);
                }

                curl_global_cleanup();
            }

send_done:
            goto chain_done;
        } else if (strcmp(ptr_command, "watchdogs") == 0) {
#ifndef WG_WINDOWS
            char *art;
            art = strdup(
"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⣤⣶⣶⣶⣶⣶⣶⣶⣦⣀⠀⠀⠀⠀⢀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠀⠀⢠⢤⣠⣶⣿⣿⡿⠿⠛⠛⠛⠛⠉⠛⠛⠛⠛⠿⣷⡦⠞⣩⣶⣸⡆⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠀⣠⣾⡤⣌⠙⠻⣅⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⠔⠋⢀⣾⣿⣿⠃⣇⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⣠⣾⣿⡟⢇⢻⣧⠄⠀⠈⢓⡢⠴⠒⠒⠒⠒⡲⠚⠁⠀⠐⣪⣿⣿⡿⡄⣿⣷⡄⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⣠⣿⣿⠟⠁⠸⡼⣿⡂⠀⠀⠈⠁⠀⠀⠀⠀⠀⠁⠀⠀⠀⠀⠉⠹⣿⣧⢳⡏⠹⣷⡄⠀⠀⠀⠀\n"
"\t\t⠀⠀⣰⣿⡿⠃⠀⠀⠀⢧⠑⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⠇⡸⠀⠀⠘⢿⣦⣄⠀⠀\n"
"\t\t⠀⢰⣿⣿⠃⠀⠀⠀⠀⡼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡠⠀⠀⠀⠀⠀⠀⠰⡇⠀⠀⠀⠈⣿⣿⣆⠀\n"
"\t\t⠀⣿⣿⡇⠀⠀⠀⠀⢰⠇⠀⢺⡇⣄⠀⠀⠀⠀⣤⣶⣀⣿⠃⠀⠀⠀⠀⠀⠀⠀⣇⠀⠀⠀⠀⠸⣿⣿⡀\n"
"\t\t⢸⣿⣿⠀⠀⠀⠀⠀⢽⠀⢀⡈⠉⢁⣀⣀⠀⠀⠀⠉⣉⠁⠀⠀⠀⣀⠀⠀⠀⠀⡇⠀⠀⠀⠀⠀⣿⣿⡇\n"
"\t\t⢸⣿⡟⠀⠀⠀⠠⠀⠈⢧⡀⠀⠀⠀⠹⠁⠀⠀⠀⠀⠀⠀⠠⢀⠀⠀⠀⠀⠀⢼⠁⠀⠀⠀⠀⠀⢹⣿⡇\n"
"\t\t⢸⣿⣿⠀⠀⠀⠀⠀⠠⠀⠙⢦⣀⠠⠊⠉⠂⠄⠀⠀⠀⠈⠀⠀⠀⣀⣤⣤⡾⠘⡆⠀⠀⠀⠀⠀⣾⣿⡇\n"
"\t\t⠘⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⢠⠜⠳⣤⡀⠀⠀⣀⣤⡤⣶⣾⣿⣿⣿⠟⠁⠀⠀⡸⢦⣄⠀⠀⢀⣿⣿⠇\n"
"\t\t⠀⢿⣿⣧⠀⠀⠀⠀⠀⣠⣤⠞⠀⠀⠀⠙⠁⠙⠉⠀⠀⠸⣛⡿⠉⠀⠀⠀⢀⡜⠀⠀⠈⠙⠢⣼⣿⡿⠀\n"
"\t\t⠀⠈⣿⣿⣆⠀⠀⢰⠋⠡⡇⠀⡀⣀⣤⢢⣤⣤⣀⠀⠀⣾⠟⠀⠀⠀⠀⢀⠎⠀⠀⠀⠀⠀⣰⣿⣿⠁⠀\n"
"\t\t⠀⠀⠈⢿⣿⣧⣀⡇⠀⡖⠁⢠⣿⣿⢣⠛⣿⣿⣿⣷⠞⠁⠀⠀⠈⠫⡉⠁⠀⠀⠀⠀⢀⣼⣿⠿⠃⠀⠀\n"
"\t\t⠀⠀⠀⠈⠻⣿⣿⣇⡀⡇⠀⢸⣿⡟⣾⣿⣿⣿⣿⠋⠀⠀⠀⢀⡠⠊⠁⠀⠀⠀⢀⣠⣿⠏⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠈⠻⣿⣿⣦⣀⢸⣿⢻⠛⣿⣿⡿⠁⠀⠀⣀⠔⠉⠀⠀⠀⠀⣀⣴⡿⠟⠁⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠀⠀⠈⠙⠿⣿⣿⣿⣼⣿⣿⣟⠀⠀⡠⠊⠀⣀⣀⣠⣴⣶⠿⠟⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠛⠿⣿⣿⣿⣿⣶⣶⣷⣶⣶⡿⠿⠛⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠛⠛⠛⠛⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
"\t\t   W   A   T   C   H   D   O   G   S\n");
            printf("%s", art);
            fflush(stdout);
#else
            wchar_t *art;
            art = _wcsdup(
L"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⣤⣶⣶⣶⣶⣶⣶⣶⣦⣀⠀⠀⠀⠀⢀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠀⠀⢠⢤⣠⣶⣿⣿⡿⠿⠛⠛⠛⠛⠉⠛⠛⠛⠛⠿⣷⡦⠞⣩⣶⣸⡆⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠀⣠⣾⡤⣌⠙⠻⣅⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⠔⠋⢀⣾⣿⣿⠃⣇⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⣠⣾⣿⡟⢇⢻⣧⠄⠀⠈⢓⡢⠴⠒⠒⠒⠒⡲⠚⠁⠀⠐⣪⣿⣿⡿⡄⣿⣷⡄⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⣠⣿⣿⠟⠁⠸⡼⣿⡂⠀⠀⠈⠁⠀⠀⠀⠀⠀⠁⠀⠀⠀⠀⠉⠹⣿⣧⢳⡏⠹⣷⡄⠀⠀⠀⠀\n"
L"\t\t⠀⠀⣰⣿⡿⠃⠀⠀⠀⢧⠑⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠻⠇⡸⠀⠀⠘⢿⣦⣄⠀⠀\n"
L"\t\t⠀⢰⣿⣿⠃⠀⠀⠀⠀⡼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡠⠀⠀⠀⠀⠀⠀⠰⡇⠀⠀⠀⠈⣿⣿⣆⠀\n"
L"\t\t⠀⣿⣿⡇⠀⠀⠀⠀⢰⠇⠀⢺⡇⣄⠀⠀⠀⠀⣤⣶⣀⣿⠃⠀⠀⠀⠀⠀⠀⠀⣇⠀⠀⠀⠀⠸⣿⣿⡀\n"
L"\t\t⢸⣿⣿⠀⠀⠀⠀⠀⢽⠀⢀⡈⠉⢁⣀⣀⠀⠀⠀⠉⣉⠁⠀⠀⠀⣀⠀⠀⠀⠀⡇⠀⠀⠀⠀⠀⣿⣿⡇\n"
L"\t\t⢸⣿⡟⠀⠀⠀⠠⠀⠈⢧⡀⠀⠀⠀⠹⠁⠀⠀⠀⠀⠀⠀⠠⢀⠀⠀⠀⠀⠀⢼⠁⠀⠀⠀⠀⠀⢹⣿⡇\n"
L"\t\t⢸⣿⣿⠀⠀⠀⠀⠀⠠⠀⠙⢦⣀⠠⠊⠉⠂⠄⠀⠀⠀⠈⠀⠀⠀⣀⣤⣤⡾⠘⡆⠀⠀⠀⠀⠀⣾⣿⡇\n"
L"\t\t⠘⣿⣿⡀⠀⠀⠀⠀⠀⠀⠀⢠⠜⠳⣤⡀⠀⠀⣀⣤⡤⣶⣾⣿⣿⣿⠟⠁⠀⠀⡸⢦⣄⠀⠀⢀⣿⣿⠇\n"
L"\t\t⠀⢿⣿⣧⠀⠀⠀⠀⠀⣠⣤⠞⠀⠀⠀⠙⠁⠙⠉⠀⠀⠸⣛⡿⠉⠀⠀⠀⢀⡜⠀⠀⠈⠙⠢⣼⣿⡿⠀\n"
L"\t\t⠀⠈⣿⣿⣆⠀⠀⢰⠋⠡⡇⠀⡀⣀⣤⢢⣤⣤⣀⠀⠀⣾⠟⠀⠀⠀⠀⢀⠎⠀⠀⠀⠀⠀⣰⣿⣿⠁⠀\n"
L"\t\t⠀⠀⠈⢿⣿⣧⣀⡇⠀⡖⠁⢠⣿⣿⢣⠛⣿⣿⣿⣷⠞⠁⠀⠀⠈⠫⡉⠁⠀⠀⠀⠀⢀⣼⣿⠿⠃⠀⠀\n"
L"\t\t⠀⠀⠀⠈⠻⣿⣿⣇⡀⡇⠀⢸⣿⡟⣾⣿⣿⣿⣿⠋⠀⠀⠀⢀⡠⠊⠁⠀⠀⠀⢀⣠⣿⠏⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠈⠻⣿⣿⣦⣀⢸⣿⢻⠛⣿⣿⡿⠁⠀⠀⣀⠔⠉⠀⠀⠀⠀⣀⣴⡿⠟⠁⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠀⠀⠈⠙⠿⣿⣿⣿⣼⣿⣿⣟⠀⠀⡠⠊⠀⣀⣀⣠⣴⣶⠿⠟⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠛⠿⣿⣿⣿⣿⣶⣶⣷⣶⣶⡿⠿⠛⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠉⠛⠛⠛⠛⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n"
L"\t\t   W   A   T   C   H   D   O   G   S\n");
            UINT originalOutputCP = GetConsoleOutputCP();
            UINT originalInputCP = GetConsoleCP();
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            _setmode(_fileno(stdout), _O_U8TEXT);
            wprintf(L"%ls", art);
            fflush(stdout);
            _setmode(_fileno(stdout), _O_TEXT);
            SetConsoleOutputCP(originalOutputCP);
            SetConsoleCP(originalInputCP);
#endif
            if (chain_pre_command && chain_pre_command[0] != '\0')
                goto chain_done;
            else
                goto _ptr_command;
        } else if (strcmp(ptr_command, command_distance) != 0 && dist <= 2) {
            wg_console_title("Watchdogs | @ undefined");
            printf("did you mean '" FCOLOUR_YELLOW "%s" FCOLOUR_DEFAULT "'", command_distance);
            printf(" (y/n):");
            fflush(stdout);
            char *confirm = readline(" ");
            if (!confirm) {
                fprintf(stderr, "Error reading input\n");
                wg_free(confirm);
                goto chain_done;
            }
            if (strlen(confirm) == 0) {
                wg_free(confirm);
                confirm = readline(" >>> (y/n): ");
            }
            if (confirm) {
                if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
                    wg_free(confirm);
                    ptr_command = strdup(command_distance);
                    goto _reexecute_command;
                } else if (strcmp(confirm, "N") == 0 || strcmp(confirm, "n") == 0) {
                    wg_free(confirm);
                    goto chain_try_command;
                } else {
                    wg_free(confirm);
                    goto chain_try_command;
                }
            } else {
                wg_free(confirm);
                goto chain_try_command;
            }
        } else {
chain_try_command:
            if (!strcmp(ptr_command, "clear")) {
                if (is_native_windows())
                    snprintf(ptr_command, WG_PATH_MAX, "cls");
            }
            int ret = -3;
            char size_run[WG_MAX_PATH];
            if (path_access("/bin/sh") != 0)
                snprintf(size_run, sizeof(size_run), "/bin/sh -c \"%s\"", ptr_command);
            else if (path_access("~/.bashrc") != 0)
                snprintf(size_run, sizeof(size_run), "bash -c \"%s\"", ptr_command);
            else if (path_access("~/.zshrc") != 0)
                snprintf(size_run, sizeof(size_run), "zsh -c \"%s\"", ptr_command);
            else
                snprintf(size_run, sizeof(size_run), "%s", ptr_command);
            ret = wg_run_command(size_run);
            if (ret)
                wg_console_title("Watchdogs | @ command not found");
            if (!(strcmp(ptr_command, "clear")))
                return -2;
            else
                return -1;
        }

chain_done:
        fflush(stdout);
        if (ptr_command) {
            wg_free(ptr_command);
            ptr_command = NULL;
        }

        return -1;
}

void chain_ret_main(void *chain_pre_command) {

        wg_console_title(NULL);
        int ret = -3;
        if (chain_pre_command != NULL ) {
            char *procure_command_argv = (char*)chain_pre_command;
            ret = __command__(procure_command_argv);
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            if (ret == -2) { return; }
            if (ret == 3) { return; }
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <C> Finished at %.3fs\n",
                         command_dur);
            return;
        }

loop_main:
        ret = __command__(NULL);
        if (ret == -1) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <C> Finished at %.3fs\n",
                         command_dur);
            goto loop_main;
        } else if (ret == 2) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            exit(0);
        } else if (ret == -2) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            goto loop_main;
        } else if (ret == 3) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        } else {
            goto basic_end;
        }

basic_end:

        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                      (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;

        pr_color(stdout,
                     FCOLOUR_CYAN,
                     " <C> Finished at %.3fs\n",
                     command_dur);
        goto loop_main;
}

int main(int argc, char *argv[]) {

        __debug_main_chain(0);

        if (argc > 1) {
            int i;
            size_t chain_total_len = 0;

            for (i = 1; i < argc; ++i)
                chain_total_len += strlen(argv[i]) + 1;

            char *chain_size_prompt;
            chain_size_prompt = wg_malloc(chain_total_len);
            if (!chain_size_prompt) { return 0; }

            chain_size_prompt[0] = '\0';
            for (i = 1; i < argc; ++i) {
                if (i > 1)
                    strcat(chain_size_prompt, " ");
                strcat(chain_size_prompt, argv[i]);
            }

            chain_ret_main(chain_size_prompt);

            wg_free(chain_size_prompt);
            return 0;
        } else {
            chain_ret_main(NULL);
        }

        return 0;
}
