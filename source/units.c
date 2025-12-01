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

#ifndef _NCURSES
    #include <ncursesw/curses.h>
#else
    #include <ncurses.h>
#endif
#ifdef WG_LINUX
    #include <spawn.h>
#endif

#include "extra.h"
#include "utils.h"
#include "lowlevel.h"
#include "crypto.h"
#include "package.h"
#include "archive.h"
#include "curl.h"
#include "units.h"
#include "runner.h"
#include "compiler.h"
#include "depends.h"

#ifndef WATCHDOGS_RELEASE
    #define WATCHDOGS_RELEASE "WD-251101"
#endif
const char *watchdogs_release = WATCHDOGS_RELEASE;

struct timespec cmd_start, cmd_end;
double command_dur;

void chain_main_data(int debug_info) {
        signal(SIGINT, SIG_DFL);
        wg_sef_fdir_reset();
        wg_toml_configs();
        wg_stop_server_tasks();
        wg_u_history();
        wgconfig.wg_sel_stat = 0;
        handle_sigint = 0;
        if (debug_info == 1) {
#if defined(_DBG_PRINT)
            pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
            printf("[function: %s | "
                "pretty function: %s | "
                "line: %d | "
                "file: %s | "
                "date: %s | "
                "time: %s | "
                "timestamp: %s | "
                "C standard: %ld | "
                "C version: %s | "
                "compiler version: %d | "
                "architecture: %s | "
                "os_type: %s (CRC32) | "
                "pointer_samp: %s | "
                "pointer_openmp: %s | "
                "f_samp: %s (CRC32) | "
                "f_openmp: %s (CRC32) | "
                "toml gamemode input: %s | "
                "toml gamemode output: %s | "
                "toml binary: %s | "
                "toml configs: %s | "
                "toml logs: %s | "
                "toml github tokens: %s | "
                "toml chatbot: %s | "
                "toml ai models: %s | "
                "toml ai key: %s\n",
                    __func__, __PRETTY_FUNCTION__,
                    __LINE__, __FILE__,
                    __DATE__, __TIME__,
                    __TIMESTAMP__,
                    __STDC_VERSION__,
                    __VERSION__,
                    __GNUC__,
#ifdef __x86_64__
                    "x86_64",
#elif defined(__i386__)
                    "i386",
#elif defined(__arm__)
                    "ARM",
#elif defined(__aarch64__)
                    "ARM64",
#else
                    "Unknown",
#endif
                    wgconfig.wg_os_type, wgconfig.wg_ptr_samp,
                    wgconfig.wg_ptr_omp, wgconfig.wg_is_samp, wgconfig.wg_is_omp,
                    wgconfig.wg_toml_gm_input, wgconfig.wg_toml_gm_output,
                    wgconfig.wg_toml_binary, wgconfig.wg_toml_config, wgconfig.wg_toml_logs,
                    wgconfig.wg_toml_github_tokens,
                    wgconfig.wg_toml_chatbot_ai,
                    wgconfig.wg_toml_models_ai,
                    wgconfig.wg_toml_key_ai);
            printf("STDC: %d\n", __STDC__);
            printf("STDC_HOSTED: %d\n", __STDC_HOSTED__);
            printf("BYTE_ORDER: ");
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            printf("Little Endian\n");
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            printf("Big Endian\n");
#else
            printf("Unknown\n");
#endif
#else
            printf("Not defined\n");
#endif
            printf("SIZE_OF_PTR: %zu bytes\n", sizeof(void*));
            printf("SIZE_OF_INT: %zu bytes\n", sizeof(int));
            printf("SIZE_OF_LONG: %zu bytes\n", sizeof(long));
#ifdef __LP64__
            printf("DATA_MODEL: LP64\n");
#elif defined(__ILP32__)
            printf("DATA_MODEL: ILP32\n");
#endif
#ifdef __GNUC__
            printf("GNUC: %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif

#ifdef __clang__
            printf("CLANG: %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#endif
            printf("OS: ");
#ifdef __SSE__
            printf("SSE: Supported\n");
#endif
#ifdef __AVX__
            printf("AVX: Supported\n");
#endif
#ifdef __FMA__
            printf("FMA: Supported\n");
#endif
#endif
            }
}

int __command__(char *chain_pre_command)
{
        chain_main_data(1);

        setlocale(LC_ALL, "en_US.UTF-8");

        if (path_access(".watchdogs/crashdetect")) {
            remove(".watchdogs/crashdetect");
        }

        int wg_compile_running = 0;

        char ptr_prompt[WG_MAX_PATH * 2];
        size_t size_ptrp = sizeof(ptr_prompt);
        char *ptr_command;
        int c_distance = INT_MAX;
        const char *_dist_command;

_ptr_command:
        if (chain_pre_command && chain_pre_command[0] != '\0') {
            ptr_command = strdup(chain_pre_command);
            printf("[" FCOLOUR_CYAN
                "watchdogs" FCOLOUR_DEFAULT "] $ %s\n", ptr_command);
        } else {
            while (true) {
                wg_snprintf(ptr_prompt, size_ptrp,
                        "[" FCOLOUR_CYAN
                        "watchdogs" FCOLOUR_DEFAULT "] $ ");

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

        _dist_command = wg_find_near_command(ptr_command,
                                             __command,
                                             __command_len,
                                             &c_distance);

_reexecute_command:
        chain_main_data(1);
        clock_gettime(CLOCK_MONOTONIC, &cmd_start);
        if (strncmp(ptr_command, "help", 4) == 0) {
            wg_console_title("Watchdogs | @ help");

            char *args;
                args = ptr_command + strlen("help");
            while (*args == ' ') ++args;

            if (strlen(args) == 0) {
                printf("Usage: help | help [<command>]\n\n");
                printf("Commands:\n");
                for (size_t i = 0; i < __command_len; i++) {
                    if (strcmp(__command[i], "help") == 0 /* watchdogs */) {
                        printf("  - watchdogs:\n");
                        continue;
                    } if (strcmp(__command[i], "ls") == 0 /* system */) {
                        printf("  - system:\n");
                    }
                    printf("    @ [=| %-15s]\n", __command[i]);
                    if (strcmp(__command[i], "ps") == 0) {
                        printf("    * innumerable\n");
                    }
                }
            } else if (strcmp(args, "exit") == 0) { 
                println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"\n     Just type 'exit' and you're outta here!");
            } else if (strcmp(args, "kill") == 0) { 
                println(stdout, "kill: refresh terminal watchdogs. | Usage: \"kill\"\n     When things get stuck or buggy, this is your fix!");
            } else if (strcmp(args, "title") == 0) { 
                println(stdout, "title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]\n     Personalize your terminal window title.");
            } else if (strcmp(args, "sha256") == 0) { 
                println(stdout, "sha256: generate sha256. | Usage: \"sha256\" | [<args>]\n     Get that SHA256 hash for your files or text.");
            } else if (strcmp(args, "crc32") == 0) { 
                println(stdout, "crc32: generate crc32. | Usage: \"crc32\" | [<args>]\n     Quick CRC32 checksum generation.");
            } else if (strcmp(args, "djb2") == 0) { 
                println(stdout, "djb2: generate djb2 hash file. | Usage: \"djb2\" | [<args>]\n     DJB2 hashing algorithm at your service.");
            } else if (strcmp(args, "time") == 0) { 
                println(stdout, "time: print current time. | Usage: \"time\"\n     What time is it? Time to check!");
            } else if (strcmp(args, "config") == 0) { 
                println(stdout, "config: re-create watchdogs.toml. Usage: \"config\"\n     Reset your config file to default settings.");
            } else if (strcmp(args, "stopwatch") == 0) { 
                println(stdout, "stopwatch: calculating time. Usage: \"stopwatch\" | [<args>]\n     Need to time something? This is your stopwatch!");
            } else if (strcmp(args, "download") == 0) {
                println(stdout, "download: fetch file from URL | Usage: \"download\" | [<args>]\n     Downloads archives from direct links.");
            } else if (strcmp(args, "hardware") == 0) { 
                println(stdout, "hardware: hardware information. | Usage: \"hardware\"\n     Show off your PC specs!");
            } else if (strcmp(args, "gamemode") == 0) { 
                println(stdout, "gamemode: download SA-MP gamemode. | Usage: \"gamemode\"\n     Grab some SA-MP gamemodes quickly.");
            } else if (strcmp(args, "pawncc") == 0) { 
                println(stdout, "pawncc: download SA-MP pawncc. | Usage: \"pawncc\"\n     Get the Pawn compiler for SA-MP.");
            } else if (strcmp(args, "log") == 0) { 
                println(stdout, "log: debugging & logging server logs. | Usage: \"log\"\n     Keep an eye on your server logs.");
            } else if (strcmp(args, "compile") == 0) { 
                println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]\n     Turn your code into something runnable!");
            } else if (strcmp(args, "running") == 0) { 
                println(stdout, "running: running your project. | Usage: \"running\" | [<args>]\n     Fire up your project and see it in action.");
            } else if (strcmp(args, "compiles") == 0) { 
                println(stdout, "compiles: compile & running your project. | Usage: \"compiles\" | [<args>]\n     Two-in-one: compile then run immediately!");
            } else if (strcmp(args, "stop") == 0) { 
                println(stdout, "stop: stopped server task. | Usage: \"stop\"\n     Halt everything! Stop your server tasks.");
            } else if (strcmp(args, "restart") == 0) { 
                println(stdout, "restart: restart server task. | Usage: \"restart\"\n     Fresh start! Restart your server.");
            } else if (strcmp(args, "wanion") == 0) { 
                println(stdout, "wanion: ask to wanion. | Usage: \"wanion\" | [<args>] | gemini based\n     Got questions? Ask Wanion (Gemini AI powered).");
            } else if (strcmp(args, "tracker") == 0) { 
                println(stdout, "tracker: account tracking. | Usage: \"tracker\" | [<args>]\n     Track accounts across platforms.");
            } else {
                printf("wd-help can't found for: '");
                printf_colour(stdout, FCOLOUR_YELLOW, "%s", args);
                printf("'\n     Oops! That command doesn't exist. Try 'help' to see available commands.\n");
            }
            goto chain_done;
        } else if (strcmp(ptr_command, "exit") == 0) {
            return 2;
        } else if (strcmp(ptr_command, "kill") == 0) {
            wg_console_title("Watchdogs | @ kill");

            if (!is_native_windows()) {
                wg_run_command("clear");
            } else {
                wg_run_command("cls");
            }

            wgconfig.wg_sel_stat = 0;
            wg_compile_running = 0;

            chain_main_data(1);

           if (chain_pre_command && chain_pre_command[0] != '\0')
                goto chain_done;
            else
                goto _ptr_command;
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *args = ptr_command + strlen("title");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: title [<title>]");
            } else {
                char title_set[WG_PATH_MAX * 2];
                wg_snprintf(title_set, sizeof(title_set), "%s", args);
                wg_console_title(title_set);
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "sha256", 6) == 0) {
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
        } else if (strncmp(ptr_command, "crc32", 5) == 0) {
            char *args = ptr_command + strlen("crc32");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: crc32 [<words>]");
            } else {
                static int init_crc32 = 0;
                if (init_crc32 != 1) {
                    init_crc32 = 1;
                    crypto_crc32_init_table();
                }

                uint32_t crc32_generate;
                crc32_generate = crypto_generate_crc32(args, sizeof(args) - 1);

                char crc_str[11];
                wg_sprintf(crc_str, "%08X", crc32_generate);

                printf("         Crypto Input : %s\n", args);
                printf("Crypto Output (CRC32) : ");
                printf("%s\n", crc_str);
            }

            goto chain_done;
        } else if (strncmp(ptr_command, "djb2", 4) == 0) {
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

            chain_main_data(1);

            wg_printfile("watchdogs.toml");

            goto chain_done;
        } else if (strncmp(ptr_command, "stopwatch", 9) == 0) {
            struct timespec start, now;
            double stw_elp;

            char *args = ptr_command + strlen("stopwatch");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: stopwatch [<sec>]");
            } else {
                int ts, hh, mm, ss;
                ts = atoi(args);
                if ( ts <= 0 ) {
                    println(stdout, "Usage: stopwatch [<sec>]");
                    goto chain_done;
                }
                clock_gettime(CLOCK_MONOTONIC, &start);
                while (true) {
                    clock_gettime(CLOCK_MONOTONIC, &now);
                    stw_elp = (now.tv_sec - start.tv_sec)
                            + (now.tv_nsec - start.tv_nsec) / 1e9;
                    if (stw_elp >= ts) {
                        wg_console_title("S T O P W A T C H : DONE");
                        break;
                    }
                    hh = (int)(stw_elp / 3600),
                    mm = (int)((stw_elp - hh*3600)/60),
                    ss = (int)(stw_elp) % 60;
                    char title_set[128];
                    wg_snprintf(title_set, sizeof(title_set),
                        "S T O P W A T C H : "
                        "%02d:%02d:%02d / %d sec",
                        hh, mm, ss, ts);
                    wg_console_title(title_set);
                    struct timespec ts = {0, 10000000};
                    nanosleep(&ts, NULL);
                }
            }

            goto chain_done;
        } else if (strcmp(ptr_command, "hardware") == 0) {
            printf("Basic Summary:\n");
            hardware_show_summary();
            printf("\nSpecific Fields Query:\n");
            unsigned int specific_fields[] = {
                FIELD_CPU_NAME,
                FIELD_CPU_CORES,
                FIELD_MEM_TOTAL,
                FIELD_DISK_FREE
            };
            hardware_query_specific(specific_fields, 4);
            printf("\nDetailed Report:\n");
            hardware_show_detailed();

            goto chain_done;
        } else if (strncmp(ptr_command, "download", 8) == 0) {
            wg_console_title("Watchdogs | @ downloading files");

            char *args = ptr_command + strlen("download");
            while (*args == ' ') ++args;

            char links_name[WG_PATH_MAX];
            const char *size_last_slash;

            if (*args == '\0') {
                println(stdout, "Usage: download [<direct-link>]");
            } else {
                size_last_slash = strrchr(args, __PATH_CHR_SEP_LINUX);
                if (size_last_slash && *(size_last_slash + 1)) {
                        wg_snprintf(links_name, sizeof(links_name), "%s", size_last_slash + 1);
                        if (!strfind(links_name, ".zip") &&
                            !strfind(links_name, ".tar.gz") &&
                            !strfind(links_name, ".tar"))
                                wg_snprintf(links_name + strlen(links_name),
                                            sizeof(links_name) - strlen(links_name),
                                            ".zip");
                        wg_download_file(args, links_name);
                } else {
                        pr_color(stdout, FCOLOUR_RED, "");
                        printf("invalid link name: %s\t\t[Fail]\n", args);
                        goto chain_done;
                }
            }
            
            goto chain_done;
        } if (strncmp(ptr_command, "install", 7) == 0) {
            wg_console_title("Watchdogs | @ install depends");

            char *args = ptr_command + strlen("install");
            while (*args == ' ') ++args;

            if (*args) {
                wg_install_depends(args);
            } else {
                char errbuf[256];
                toml_table_t *wg_toml_config;
                FILE *proc_f = fopen("watchdogs.toml", "r");
                wg_toml_config = toml_parse_file(proc_f, errbuf, sizeof(errbuf));
                if (proc_f) fclose(proc_f);

                if (!wg_toml_config) {
                    pr_error(stdout, "parsing TOML: %s", errbuf);
                    return 0;
                }

                toml_table_t *wg_depends;
                size_t arr_sz, i;
                char *merged = NULL;

                wg_depends = toml_table_in(wg_toml_config, "depends");
                if (!wg_depends)
                    goto out;

                toml_array_t *wg_toml_aio_repo = toml_array_in(wg_depends, "aio_repo");
                if (!wg_toml_aio_repo)
                    goto out;

                arr_sz = toml_array_nelem(wg_toml_aio_repo);
                for (i = 0; i < arr_sz; i++) {
                    toml_datum_t val;

                    val = toml_string_at(wg_toml_aio_repo, i);
                    if (!val.ok)
                            continue;

                    if (!merged) {
                            merged = wg_realloc(NULL, strlen(val.u.s) + 1);
                            if (!merged)
                                    goto free_val;

                            wg_snprintf(merged, strlen(val.u.s) + 1, "%s", val.u.s);
                    } else {
                            char *tmp;
                            size_t old_len = strlen(merged);
                            size_t new_len = old_len + strlen(val.u.s) + 2;

                            tmp = wg_realloc(merged, new_len);
                            if (!tmp)
                                    goto free_val;

                            merged = tmp;
                            wg_snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
                    }

free_val:
                    wg_free(val.u.s);
                    val.u.s = NULL;
                }

                if (!merged)
                    merged = strdup("");

                wgconfig.wg_toml_aio_repo = merged;
                wg_install_depends(wgconfig.wg_toml_aio_repo);

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
            printf("  \033[36m[t]\033[0m Termux\n");

            wgconfig.wg_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L")) {
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
            } else if (strfind(platform, "W")) {
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
            } else if (strfind(platform, "E")) {
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

            if (strfind(platform, "L")) {
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
            } else if (strfind(platform, "W")) {
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
            } else if (strfind(platform, "T")) {
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
            } else if (strfind(platform, "E")) {
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
        } else if (strcmp(ptr_command, "log") == 0) {
            wg_console_title("Watchdogs | @ log");
            printf("running logs command: 0000WGDEBUGGINGSERVER...\n");
#ifdef __ANDROID__
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
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
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
            second_arg = strtok(NULL, " ");
            four_arg = strtok(NULL, " ");
            five_arg = strtok(NULL, " ");
            six_arg = strtok(NULL, " ");
            seven_arg = strtok(NULL, " ");
            eight_arg = strtok(NULL, " ");
            nine_arg = strtok(NULL, " ");

            wg_run_compiler(args, compile_args, second_arg, four_arg,
                            five_arg, six_arg, seven_arg, eight_arg,
                            nine_arg);

            goto chain_done;
        } if (strncmp(ptr_command, "running", 7) == 0) {
_runners_:
                wg_stop_server_tasks();

                if (!path_exists(wgconfig.wg_toml_binary)) {
                    pr_error(stdout, "can't locate sa-mp/open.mp binary file!");
                    goto chain_done;
                }
                if (!path_exists(wgconfig.wg_toml_config)) {
                    pr_warning(stdout, "can't locate %s - config file!", wgconfig.wg_toml_config);
                    goto chain_done;
                }

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
                    size_arg1 = wgconfig.wg_toml_gm_output;
                else
                    size_arg1 = args2;

                size_t needed = wg_snprintf(NULL, 0,
                                "Watchdogs | "
                                "@ running | "
                                "args: %s | "
                                "config: %s | "
                                "CTRL + C to stop. | \"log\" for debugging",
                                size_arg1,
                                wgconfig.wg_toml_config) + 1;
                char *title_running_info = wg_malloc(needed);
                if (!title_running_info) { return 1; }
                wg_snprintf(title_running_info, needed,
                                    "Watchdogs | "
                                    "@ running | "
                                    "args: %s | "
                                    "config: %s | "
                                    "CTRL + C to stop. | \"log\" for debugging",
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
                char size_run[WG_PATH_MAX];
                struct sigaction sa;
                if (wg_server_env() == 1) {
                    if (args2 == NULL || (args2[0] == '.' && args2[1] == '\0')) {
start_main:
                        sa.sa_handler = unit_handle_sigint;
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
                        wg_snprintf(size_run, sizeof(size_run), "%s", wgconfig.wg_toml_binary);
#else
                        chmod(wgconfig.wg_toml_binary, 0777);
                        wg_snprintf(size_run, sizeof(size_run), "./%s", wgconfig.wg_toml_binary);
#endif
                        int rate_runner_failed = wg_run_command(size_run);
                        if (rate_runner_failed == 0) {
                            if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX)) {
                                    printf("~ logging...\n");
                                    sleep(3);
                                    wg_display_server_logs(0);
                            }
                        } else {
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
                        if (handle_sigint == 0)
                            raise(SIGINT);

                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("log");
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
                            ptr_command = strdup("log");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);
                    }
                } else if (wg_server_env() == 2) {
                    if (args2 == NULL || (args2[0] == '.' && args2[1] == '\0')) {
start_main2:
                        sa.sa_handler = unit_handle_sigint;
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
                        wg_snprintf(size_run, sizeof(size_run), "%s", wgconfig.wg_toml_binary);
#else
                        chmod(wgconfig.wg_toml_binary, 0777);
                        wg_snprintf(size_run, sizeof(size_run), "./%s", wgconfig.wg_toml_binary);
#endif
                        int rate_runner_failed = wg_run_command(size_run);
                        if (rate_runner_failed != 0) {
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
                        if (handle_sigint == 0) {
                            raise(SIGINT);
                        }

                        printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                        char *debug_runner = readline("   answer (y/n): ");
                        if (strcmp(debug_runner, "Y") == 0 || strcmp(debug_runner, "y") == 0) {
                            ptr_command = strdup("log");
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
                            ptr_command = strdup("log");
                            wg_free(debug_runner);
                            goto _reexecute_command;
                        }
                        wg_free(debug_runner);                    }
                } else {
                    pr_crit(stdout,
"\033[1;31merror:\033[0m sa-mp/open.mp server not found!\n"
"  \033[2mhelp:\033[0m install it before continuing\n");
                    printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

                    char *ptr_sigA;
ret_ptr3:
                    ptr_sigA = readline("");

                    while (true) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            wg_free(ptr_sigA);
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
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            wg_free(ptr_sigA);
                            break;
                        } else {
                            pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                            wg_free(ptr_sigA);
                            goto ret_ptr3;
                        }
                    }
                }

                goto chain_done;
        } else if (strcmp(ptr_command, "compiles") == 0) {
            wg_console_title("Watchdogs | @ compiles");

            const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

            wg_compile_running = 1;

            wg_run_compiler(args[0], args[1], args[2], args[3],
                            args[4], args[5], args[6], args[7],
                            args[8]);

            if (wgconfig.wg_compiler_stat < 1) {
                goto _runners_;
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
        } else if (strncmp(ptr_command, "wanion", 6) == 0) {
            char *args = ptr_command + strlen("wanion");
            while (*args == ' ') ++args;

            if (*args == '\0') {
                println(stdout, "Usage: wanion [<text>]");
            } else {
                int retry = 0;
                char size_rest_api_perform[WG_PATH_MAX];
                int is_chatbot_groq_based = 0;
                if (strcmp(wgconfig.wg_toml_chatbot_ai, "gemini") == 0)
rest_def:
                    wg_snprintf(size_rest_api_perform, sizeof(size_rest_api_perform),
                                      "https://generativelanguage.googleapis.com/"
                                      "v1beta/models/%s:generateContent",
                                      wgconfig.wg_toml_models_ai);
                else if (strcmp(wgconfig.wg_toml_chatbot_ai, "groq") == 0) {
                    is_chatbot_groq_based = 1;
                    wg_snprintf(size_rest_api_perform, sizeof(size_rest_api_perform),
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
                    wg_snprintf(wanion_json_payload, sizeof(wanion_json_payload),
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
                    wg_snprintf(wanion_json_payload, sizeof(wanion_json_payload),
                             /* prompt here */
                             "{"
                             "\"contents\":[{\"role\":\"user\","
                             "\"parts\":[{\"text\":\"Your name in here Wanion (use it) made from Google my asking: %s\"}]}],"
                             "\"generationConfig\": {\"maxOutputTokens\": 1024}}",
                             wanion_escaped_argument);
                }

wanion_retrying:
#if defined(_DBG_PRINT)
                printf("json payload: %s\n", wanion_json_payload);
#endif
                struct timespec start = {0}, end = { 0 };
                double wanion_dur;

                clock_gettime(CLOCK_MONOTONIC, &start);
                struct buf b = {0};
                CURL *h = curl_easy_init();
                if (!h) {
                    clock_gettime(CLOCK_MONOTONIC, &end);
                    goto chain_done;
                }

                struct curl_slist *hdr = curl_slist_append(NULL, "Content-Type: application/json");
                char size_tokens[WG_PATH_MAX + 26];
                if (is_chatbot_groq_based == 1)
                    wg_snprintf(size_tokens, sizeof(size_tokens), "Authorization: Bearer %s", wgconfig.wg_toml_key_ai);
                else
                    wg_snprintf(size_tokens, sizeof(size_tokens), "x-goog-api-key: %s", wgconfig.wg_toml_key_ai);
                hdr = curl_slist_append(hdr, size_tokens);

                curl_easy_setopt(h, CURLOPT_URL, size_rest_api_perform);
                curl_easy_setopt(h, CURLOPT_HTTPHEADER, hdr);
                curl_easy_setopt(h, CURLOPT_TCP_KEEPALIVE, 1L);
                curl_easy_setopt(h, CURLOPT_POSTFIELDS, wanion_json_payload);
                curl_easy_setopt(h, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
                curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(h, CURLOPT_WRITEDATA, &b);
                curl_easy_setopt(h, CURLOPT_TIMEOUT, 30L);

                verify_cacert_pem(h);

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
                    if (strfind(b.data, "You exceeded your current quota, please check your plan and billing details") ||
                        strfind(b.data, "Too Many Requests"))
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
        } else if (strncmp(ptr_command, "tracker", 7) == 0) {
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
                char variations[100][100];

                account_tracker_discrepancy(args, variations, &variation_count);

                printf("[INFO] Search base: %s\n", args);
                printf("[INFO] Generated %d Variations\n\n", variation_count);

                for (int i = 0; i < variation_count; i++) {
                    printf("=== CHECKING ACCOUNTS: %s ===\n", variations[i]);
                    account_tracking_username(curl, variations[i]);
                    printf("\n");
                }

                curl_easy_cleanup(curl);
                curl_global_cleanup();
            }

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
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            wg_console_title("Watchdogs | @ undefined");
            printf("did you mean '" FCOLOUR_YELLOW "%s" FCOLOUR_DEFAULT "'", _dist_command);
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
                    ptr_command = strdup(_dist_command);
                    goto _reexecute_command;
                } else if (strcmp(confirm, "N") == 0 || strcmp(confirm, "n") == 0) {
                    wg_free(confirm);
                    goto chain_done;
                } else {
                    wg_free(confirm);
                    goto chain_done;
                }
            } else {
                wg_free(confirm);
                goto chain_done;
            }
        } else {
            char _p_command[WG_PATH_MAX * 2];
            wg_snprintf(_p_command, WG_PATH_MAX, "%s", ptr_command);
            if (!strcmp(_p_command, "clear")) {
                if (is_native_windows())
                    wg_snprintf(_p_command, WG_PATH_MAX, "cls");
            }
            int ret = -3;
            ret = wg_run_command(_p_command);
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

void chain_goto_main(void *chain_pre_command) {
        wg_console_title(NULL);
        int ret = -3;
        if (chain_pre_command != NULL ) {
            char *fetch_command_argv = (char*)chain_pre_command;
            ret = __command__(fetch_command_argv);
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
        chain_main_data(0);

        if (argc > 1) {
            int i;
            size_t chain_total_len = 0;

            for (i = 1; i < argc; ++i)
                chain_total_len += strlen(argv[i]) + 1;

            char *chain_size_prompt;
            chain_size_prompt = wg_malloc(chain_total_len);
            if (!chain_size_prompt) {
                pr_error(stdout, "Failed to allocate memory for command");
                return 1;
            }

            chain_size_prompt[0] = '\0';
            for (i = 1; i < argc; ++i) {
                if (i > 1)
                    strcat(chain_size_prompt, " ");
                strcat(chain_size_prompt, argv[i]);
            }

            chain_goto_main(chain_size_prompt);

            wg_free(chain_size_prompt);
            return 0;
        } else {
            chain_goto_main(NULL);
        }

        return 0;
}

