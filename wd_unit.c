#define WATCHDOGS_RELEASE "WD-251101"
const char *watchdogs_release = WATCHDOGS_RELEASE;
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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
#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_util.h"
#include "wd_hardware.h"
#include "wd_crypto.h"
#include "wd_package.h"
#include "wd_archive.h"
#include "wd_curl.h"
#include "wd_unit.h"
#include "wd_server.h"
#include "wd_compiler.h"
#include "wd_depends.h"

struct timespec cmd_start, cmd_end;
double command_dur;

void __function__(void) {
        wd_set_toml();
        wd_sef_fdir_reset();
        wd_u_history();
#if defined(_DBG_PRINT)
        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING");
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
               "architecture: %s]:\n"
               "\tos_type: %s (CRC32)"
               "\tpointer_samp: %s\n"
               "\tpointer_openmp: %s"
               "\tf_samp: %s (CRC32)\n"
               "\tf_openmp: %s (CRC32)\n"
               "\tconfig: %s\n"
               "\tgh tokens: %s\n",
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
                wcfg.wd_os_type, wcfg.wd_ptr_samp,
                wcfg.wd_ptr_omp, wcfg.wd_is_samp,
                wcfg.wd_is_omp, wcfg.wd_toml_config,
                wcfg.wd_toml_github_tokens);
        printf("\n[ADDITIONAL DEBUG INFO]\n");
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
        return;
}

int __command__(char *chain_pre_command)
{
        setlocale(LC_ALL, "en_US.UTF-8");
        int _wd_crash_ck = path_acces(".wd_crashdetect");
        if (_wd_crash_ck) {
          remove(".wd_crashdetect");
          wd_server_crash_check();
        }
        
        wcfg.wd_sel_stat = 0;
        int wd_compile_running = 0;
        
        int wd_notice_logged = 0;
        if (wd_notice_logged == 0) {
			wd_notice_logged = 1;
			if (wd_server_env() == 1)
				printf("Welcome :) -Logged as SA-MP Server...\n");
			else if (wd_server_env() == 2)
				printf("Welcome :) -Logged as Open.MP Server...\n");
        }
        
        char ptr_prompt[WD_MAX_PATH];
        size_t size_ptrp;
        size_ptrp = sizeof(ptr_prompt);
        char *ptr_command;
        int c_distance = INT_MAX;
        const char *_dist_command;
        
_ptr_command:
        if (chain_pre_command && chain_pre_command[0] != '\0') {
            ptr_command = strdup(chain_pre_command);
            printf("[" FCOLOUR_CYAN
                   "watchdogs ~ %s" FCOLOUR_DEFAULT "]$ %s\n", wd_get_cwd(), ptr_command);
        } else {
            wd_snprintf(ptr_prompt, size_ptrp,
                        "[" FCOLOUR_CYAN
                        "watchdogs ~ %s" FCOLOUR_DEFAULT "]$ ", wd_get_cwd());
            ptr_command = readline(ptr_prompt);

            if (ptr_command == NULL || ptr_command[0] == '\0')
                goto _ptr_command;
        }

        fflush(stdout);

        wd_a_history(ptr_command);

        _dist_command = wd_find_near_command(ptr_command,
                                             __command,
                                             __command_len,
                                             &c_distance);

_reexecute_command:
        __function__();
        clock_gettime(CLOCK_MONOTONIC, &cmd_start);
        if (strncmp(ptr_command, "help", 4) == 0) {
            wd_set_title("Watchdogs | @ help");

            char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') ++arg;

            if (strlen(arg) == 0) {
                printf("Usage: help | help [<command>]\n\n");
                printf("Commands:\n");
                for (size_t i = 0; i < __command_len; i++) {
                    if (strcmp(__command[i], "help") == 0) {
                        printf("  - watchdogs:\n");
                        continue;
                    }
                    if (strcmp(__command[i], "ls") == 0) {
                        printf("  - system:\n");
                    }
                    printf("    @ [=| %-15s]\n", __command[i]);
                    if (strcmp(__command[i], "ps") == 0) {
                        printf("    * innumerable\n");
                    }
                }
            } else if (strcmp(arg, "exit") == 0) { println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "kill") == 0) { println(stdout, "kill: refresh terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println(stdout, "title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "sha256") == 0) { println(stdout, "sha256: generate sha256. | Usage: \"sha256\" | [<args>]");
            } else if (strcmp(arg, "crc32") == 0) { println(stdout, "crc32: generate crc32. | Usage: \"crc32\" | [<args>]");
            } else if (strcmp(arg, "djb2") == 0) { println(stdout, "djb2: generate djb2 hash file. | Usage: \"djb2\" | [<args>]");
            } else if (strcmp(arg, "time") == 0) { println(stdout, "time: print current time. | Usage: \"time\"");
            } else if (strcmp(arg, "config") == 0) { println(stdout, "config: re-create watchdogs.toml. Usage: \"config\"");
            } else if (strcmp(arg, "stopwatch") == 0) { println(stdout, "stopwatch: calculating time. Usage: \"stopwatch\" | [<args>]");
            } else if (strcmp(arg, "install") == 0) { println(stdout, "install: download & install depends | Usage: \"install\" |"
                                                                      "[<args>]\n\t- install user/repo:tag (github only)");
            } else if (strcmp(arg, "hardware") == 0) { println(stdout, "hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println(stdout, "gamemode: download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println(stdout, "pawncc: download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println(stdout, "running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "compiles") == 0) { println(stdout, "compiles: compile & running your project. | Usage: \"compiles\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println(stdout, "stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println(stdout, "restart: restart server task. | Usage: \"restart\"");
            } else {
                printf("wd-help can't found for: '");
                printf_color(stdout, FCOLOUR_YELLOW, "%s", arg);
                printf("'\n");
            }
            goto done;
        } else if (strcmp(ptr_command, "exit") == 0) {
            exit(0);
        } else if (strcmp(ptr_command, "kill") == 0) {
            wd_set_title("Watchdogs | @ kill");

            if (!is_native_windows()) {
                wd_run_command("clear");
            } else {
                wd_run_command("cls");
            }
            
            wd_sef_fdir_reset();
            wd_notice_logged = 0;
            wcfg.wd_sel_stat = 0;
            wd_compile_running = 0;
            

            __function__();

           if (chain_pre_command && chain_pre_command[0] != '\0')
                goto done;
            else
                goto _ptr_command;
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 5;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: title [<title>]");
            } else {
                char title_set[128];
                wd_snprintf(title_set, sizeof(title_set), "%s", arg);
                wd_set_title(title_set);
            }

            goto done;
        } else if (strncmp(ptr_command, "sha256", 6) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: sha256 [<words>]");
            } else {
                unsigned char digest[32];

                if (crypto_generate_sha256_hash(arg, digest) != WD_RETN) {
                    goto done;
                }

                printf("Crypto Input : %s\n", arg);
                printf("Crypto Output (SHA256) : ");
                crypto_print_hex(digest, sizeof(digest));
            }

            goto done;
        } else if (strncmp(ptr_command, "crc32", 5) == 0) {
            char *arg = ptr_command + 5;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: crc32 [<words>]");
            } else {
                static int init_crc32 = 0;
                if (init_crc32 != 1) {
                    init_crc32 = 1;
                    crypto_crc32_init_table();
                }

                uint32_t crc32_generate;
                crc32_generate = crypto_generate_crc32(arg, sizeof(arg) - 1);
                char crc_str[9];
                sprintf(crc_str, "%08X", crc32_generate);

                if (crc32_generate) {
                    printf("Crypto Input : %s\n", arg);
                    printf("Crypto Output (CRC32) : ");
                    printf("%s\n", crc_str);
                }
            }

            goto done;
        } else if (strncmp(ptr_command, "djb2", 4) == 0) {
            char *arg = ptr_command + 4;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: djb2 [<file>]");
            } else {
                unsigned long djb2_generate;
                djb2_generate = crypto_djb2_hash_file(arg);

                if (djb2_generate) {
                    printf("Crypto Input : %s\n", arg);
                    printf("Crypto Output (DJB2) : ");
                    printf("%#lx\n", djb2_generate);
                }
            }

            goto done;
        } else if (strcmp(ptr_command, "time") == 0) {
            time_t current_time;
            struct tm *time_info;
            char time_string[100];

            time(&current_time);
            time_info = localtime(&current_time);

            strftime(time_string, sizeof(time_string),
                    "%A, %d %B %Y %H:%M:%S", time_info);

            printf("Now: %s\n", time_string);

            goto done;
        } else if (strcmp(ptr_command, "config") == 0) {
            if (access("watchdogs.toml", F_OK) == 0)
                remove("watchdogs.toml");
                
            wd_notice_logged = 0;
            __function__();

            wd_printfile("watchdogs.toml");

            goto done;
        } else if (strncmp(ptr_command, "stopwatch", 9) == 0) {
            struct timespec start, now;
            double stw_elp;

            char *arg = ptr_command + 10;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: stopwatch [<sec>]");
            } else {
                int ts, hh, mm, ss;
                ts = atoi(arg);
                if ( ts <= 0 ) {
                    println(stdout, "Usage: stopwatch [<sec>]");
                    goto done;
                }
                clock_gettime(CLOCK_MONOTONIC, &start);
                while (true) {
                    clock_gettime(CLOCK_MONOTONIC, &now);
                    stw_elp = (now.tv_sec - start.tv_sec)
                            + (now.tv_nsec - start.tv_nsec) / 1e9;
                    if (stw_elp >= ts) {
                        wd_set_title("S T O P W A T C H : DONE");
                        break;
                    }
                    hh = (int)(stw_elp / 3600),
                    mm = (int)((stw_elp - hh*3600)/60),
                    ss = (int)(stw_elp) % 60;
                    char title_set[128];
                    wd_snprintf(title_set, sizeof(title_set),
                        "S T O P W A T C H : "
                        "%02d:%02d:%02d / %d sec",
                        hh, mm, ss, ts);
                    wd_set_title(title_set);
                    struct timespec ts = {0, 10000000};
                    nanosleep(&ts, NULL);
                }
            }

            goto done;
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

            goto done;
        } else if (strncmp(ptr_command, "install", 7) == 0) {
            wd_set_title("Watchdogs | @ install depends");

            char *arg = ptr_command + 7;
            while (*arg == ' ') ++arg;

            if (*arg) {
                wd_install_depends(arg);
            } else {
                char errbuf[256];
                toml_table_t *_toml_config;
                FILE *proc_f = fopen("watchdogs.toml", "r");
                _toml_config = toml_parse_file(proc_f, errbuf, sizeof(errbuf));
                if (proc_f) fclose(proc_f);

                if (!_toml_config) {
                    pr_error(stdout, "parsing TOML: %s", errbuf);
                    return WD_RETZ;
                }

                toml_table_t *wd_depends;
                size_t arr_sz, i;
                char *merged = NULL;

                wd_depends = toml_table_in(_toml_config, "depends");
                if (!wd_depends)
                    goto out;

                toml_array_t *wd_toml_aio_repo = toml_array_in(wd_depends, "aio_repo");
                if (!wd_toml_aio_repo)
                    goto out;

                arr_sz = toml_array_nelem(wd_toml_aio_repo);
                for (i = 0; i < arr_sz; i++) {
                    toml_datum_t val;

                    val = toml_string_at(wd_toml_aio_repo, i);
                    if (!val.ok)
                            continue;

                    if (!merged) {
                            merged = wd_realloc(NULL, strlen(val.u.s) + 1);
                            if (!merged)
                                    goto free_val;

                            wd_snprintf(merged, strlen(val.u.s) + 1, "%s", val.u.s);
                    } else {
                            char *tmp;
                            size_t old_len = strlen(merged);
                            size_t new_len = old_len + strlen(val.u.s) + 2;

                            tmp = wd_realloc(merged, new_len);
                            if (!tmp)
                                    goto free_val;

                            merged = tmp;
                            wd_snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
                    }

free_val:
                    wd_free(val.u.s);
                    val.u.s = NULL;
                }

                if (!merged)
                    merged = strdup("");

                wcfg.wd_toml_aio_repo = merged;
                wd_install_depends(wcfg.wd_toml_aio_repo);

out:
                toml_free(_toml_config);
            }

            goto done;
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            wd_set_title("Watchdogs | @ gamemode");
ret_ptr:
#ifdef WD_WINDOWS
            wchar_t *platform_art;
            platform_art = _wcsdup(
L"\033[1m"
L"╔════════════════════════════════════════════════╗\n"
L"║              Select Platform First             ║\n"
L"╠════════════════════════════════════════════════╣\n"
L"║  * 'L' or 'l' for Linux                        ║\n"
L"║  * 'W' or 'w' for Windows                      ║\n"
L"║    ^ Windows worked for WSL/MSYS (not for      ║\n"
L"║       docker in WSL)                           ║\n"
L"╚════════════════════════════════════════════════╝\n"
L"\033[0m");
            UINT originalOutputCP = GetConsoleOutputCP();
            UINT originalInputCP = GetConsoleCP();
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            _setmode(_fileno(stdout), _O_U8TEXT);
            wprintf(L"%ls", platform_art);
            fflush(stdout);
            _setmode(_fileno(stdout), _O_TEXT);
            SetConsoleOutputCP(originalOutputCP);
            SetConsoleCP(originalInputCP);
#else
printf("\033[1m");
printf("╔════════════════════════════════════════════════╗\n");
printf("║              Select Platform First             ║\n");
printf("╠════════════════════════════════════════════════╣\n");
printf("║  * 'L' or 'l' for Linux                        ║\n");
printf("║  * 'W' or 'w' for Windows                      ║\n");
printf("║    ^ Windows worked for WSL/MSYS (not for      ║\n");
printf("║       docker in WSL)                           ║\n");
printf("╚════════════════════════════════════════════════╝\n");
printf("\033[0m");
#endif
            wcfg.wd_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L")) {
                wd_free(ptr_command);
                wd_free(platform);
                int ret = wd_install_server("linux");
loop_igm:
                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                    goto loop_igm;
                else if (ret == WD_RETZ)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "W")) {
                wd_free(ptr_command);
                wd_free(platform);
                int ret = wd_install_server("windows");
loop_igm2:
                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                    goto loop_igm2;
                else if (ret == WD_RETZ)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "E")) {
                wd_free(ptr_command);
                wd_free(platform);
                if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
            } else {
                pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                wd_free(platform);
                platform = NULL;
                goto ret_ptr;
            }

            goto done;
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            wd_set_title("Watchdogs | @ pawncc");
ret_ptr2:
#ifdef WD_WINDOWS
            wchar_t *platform_art;
            platform_art = _wcsdup(
L"\033[1m"
L"╔════════════════════════════════════════════════╗\n"
L"║              Select Platform First             ║\n"
L"╠════════════════════════════════════════════════╣\n"
L"║  * 'L' or 'l' for Linux                        ║\n"
L"║  * 'W' or 'w' for Windows                      ║\n"
L"║    ^ Windows worked for WSL/MSYS (not for      ║\n"
L"║       docker in WSL)                           ║\n"
L"║  * 'T' or 't' for Termux                       ║\n"
L"╚════════════════════════════════════════════════╝\n"
L"\033[0m");
            UINT originalOutputCP = GetConsoleOutputCP();
            UINT originalInputCP = GetConsoleCP();
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
            _setmode(_fileno(stdout), _O_U8TEXT);
            wprintf(L"%ls", platform_art);
            fflush(stdout);
            _setmode(_fileno(stdout), _O_TEXT);
            SetConsoleOutputCP(originalOutputCP);
            SetConsoleCP(originalInputCP);
#else
printf("\033[1m");
printf("╔════════════════════════════════════════════════╗\n");
printf("║              Select Platform First             ║\n");
printf("╠════════════════════════════════════════════════╣\n");
printf("║  * 'L' or 'l' for Linux                        ║\n");
printf("║  * 'W' or 'w' for Windows                      ║\n");
printf("║    ^ Windows worked for WSL/MSYS (not for      ║\n");
printf("║       docker in WSL)                           ║\n");
printf("║  * 'T' or 't' for Termux                       ║\n");
printf("╚════════════════════════════════════════════════╝\n");
printf("\033[0m");
#endif
            wcfg.wd_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("linux");
loop_ipcc:
                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc;
                else if (ret == WD_RETZ)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "W")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("windows");
loop_ipcc2:
                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc2;
                else if (ret == WD_RETZ)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "T")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("termux");
loop_ipcc3:
                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc3;
                else if (ret == WD_RETZ)
                {
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
                }
            } else if (strfind(platform, "E")) {
                wd_free(ptr_command);
                wd_free(platform);
                if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
            } else {
                pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                wd_free(platform);
                platform = NULL;
                goto ret_ptr2;
            }

            goto done;
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
            wd_set_title("Watchdogs | @ compile");

            char *arg;
            char *compile_args;
            char *second_arg = NULL;
            char *four_arg = NULL;
            char *five_arg = NULL;
            char *six_arg = NULL;
            char *seven_arg = NULL;
            char *eight_arg = NULL;

            arg = ptr_command + 7;

            while (*arg == ' ') {
                arg++;
            }

            compile_args = strtok(arg, " ");
            second_arg = strtok(NULL, " ");
            four_arg = strtok(NULL, " ");
            five_arg = strtok(NULL, " ");
            six_arg = strtok(NULL, " ");
            seven_arg = strtok(NULL, " ");
            eight_arg = strtok(NULL, " ");

            wd_run_compiler(arg, compile_args, second_arg, four_arg, 
                            five_arg, six_arg, seven_arg, eight_arg);

            goto done;
        } if (strncmp(ptr_command, "running", 7) == 0) {
_runners_:
                wd_stop_server_tasks();

                if (!path_exists(wcfg.wd_toml_binary)) {
                    pr_error(stdout, "Can't locate sa-mp/open.mp binary file!");
                    goto done;
                }

                server_mode = 0;

                int _wd_log_acces = path_acces("server_log.txt");
                if (_wd_log_acces)
                  remove("server_log.txt");
                _wd_log_acces = path_acces("log.txt");
                if (_wd_log_acces)
                  remove("log.txt");

                size_t cmd_len = 7;
                char *arg = ptr_command + cmd_len;
                while (*arg == ' ') ++arg;
                char *arg1 = strtok(arg, " ");

                char *size_arg1 = NULL;
                if (arg1 == NULL || *arg1 == '\0')
                  size_arg1 = wcfg.wd_toml_gm_input;
                else
                  size_arg1 = arg1;
                char *gamemode = size_arg1;
                if (gamemode) {
                    char *f_EXT = strrchr(gamemode, '.');
                    if (f_EXT)
                            *f_EXT = '\0';
                } else {
                    gamemode = wcfg.wd_toml_gm_output;
                }

                size_t needed = wd_snprintf(NULL, 0,
                                "Watchdogs | "
                                "@ running | "
                                "args: %s | "
                                "gamemode: %s | "
                                "config: %s | "
                                "CTRL + C to stop.",
                                size_arg1,
                                gamemode,
                                wcfg.wd_toml_config) + 1;
                char *title_running_info = wd_malloc(needed);
                if (!title_running_info) { return WD_RETN; }
                wd_snprintf(title_running_info, needed,
                                    "Watchdogs | "
                                    "@ running | "
                                    "args: %s | "
                                    "gamemode: %s | "
                                    "config: %s | "
                                    "CTRL + C to stop.",
                                    size_arg1,
                                    gamemode,
                                    wcfg.wd_toml_config);
                if (title_running_info) {
                		wd_set_title(title_running_info);
                    wd_free(title_running_info);
                    title_running_info = NULL;
                }

                int _wd_config_acces = path_acces(wcfg.wd_toml_config);
                if (!_wd_config_acces)
                {
                    pr_error(stdout, "%s not found!", wcfg.wd_toml_config);
                    goto done;
                }

                pr_color(stdout, FCOLOUR_YELLOW, "running..\n");
                if (wd_server_env() == 1) {
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
start_main:
                        char size_run[128];

                        struct sigaction sa;

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
#ifdef WD_WINDOWS
                        wd_snprintf(size_run, sizeof(size_run), "%s", wcfg.wd_ptr_samp);
#else
                        chmod(wcfg.wd_ptr_samp, 0777);
                        wd_snprintf(size_run, sizeof(size_run), "./%s", wcfg.wd_ptr_samp);
#endif
                        end = time(NULL);

                        int running_FAIL = wd_run_command(size_run);
                        if (running_FAIL == 0) {
                            if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                                sleep(2);
                                wd_display_server_logs(0);
                            }
                        } else {
                            pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                            elapsed = difftime(end, start);
                            if (elapsed <= 5.0 && ret_serv == 0) {
                                ret_serv = 1;
                                printf("\ttry starting again..");
                                goto back_start;
                            }
                        }

                        wd_server_crash_check();
                    } else {
                        if (wd_compile_running == 1) {
                            wd_compile_running = 0;
                            goto start_main;
                        }
                        server_mode = 1;
                        wd_run_samp_server(arg1, wcfg.wd_ptr_samp);
                    }
                } else if (wd_server_env() == 2) {
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
start_main2:
                        char size_run[128];

                        struct sigaction sa;

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
#ifdef WD_WINDOWS
                        wd_snprintf(size_run, sizeof(size_run), "%s", wcfg.wd_ptr_omp);
#else
                        chmod(wcfg.wd_ptr_samp, 0777);
                        wd_snprintf(size_run, sizeof(size_run), "./%s", wcfg.wd_ptr_omp);
#endif
                        end = time(NULL);

                        int running_FAIL = wd_run_command(size_run);
                        if (running_FAIL == 0) {
                            sleep(2);
                            wd_display_server_logs(1);
                        } else {
                            pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                            elapsed = difftime(end, start);
                            if (elapsed <= 5.0 && ret_serv == 0) {
                                ret_serv = 1;
                                printf("\ttry starting again..");
                                goto back_start2;
                            }
                        }

                        wd_server_crash_check();
                    } else {
                        if (wd_compile_running == 1) {
                            wd_compile_running = 0;
                            goto start_main2;
                        }
                        server_mode = 1;
                        wd_run_omp_server(arg1, wcfg.wd_ptr_omp);
                    }
                } else {
                    pr_crit(stdout, "samp-server/open.mp server not found!");

                    char *ptr_sigA;
ret_ptr3:
                    ptr_sigA = readline("install now? [Y/n]: ");

                    while (1) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            wd_free(ptr_sigA);
                            if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS)) {
                                int ret = wd_install_server("windows");
n_loop_igm:
                                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                                    goto n_loop_igm;
                            } else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                                int ret = wd_install_server("linux");
n_loop_igm2:
                                if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
                                    goto n_loop_igm2;
                            }
                            break;
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            wd_free(ptr_sigA);
                            break;
                        } else {
                            pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                            wd_free(ptr_sigA);
                            goto ret_ptr3;
                        }
                    }
                }

                goto done;
        } else if (strcmp(ptr_command, "compiles") == 0) {
            wd_set_title("Watchdogs | @ compiles");

            const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

            wd_compile_running = 1;

            wd_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);

            if (wcfg.wd_compiler_stat < 1) {
                goto _runners_;
            }

            goto done;
        } else if (strcmp(ptr_command, "stop") == 0) {
            wd_set_title("Watchdogs | @ stop");
            wd_stop_server_tasks();

            goto done;
        } else if (strcmp(ptr_command, "restart") == 0) {
            wd_stop_server_tasks();
            sleep(2);
            goto _runners_;
        } else if (strcmp(ptr_command, "watchdogs") == 0) {
#ifndef WD_WINDOWS
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
                goto done;
            else
                goto _ptr_command;
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            wd_set_title("Watchdogs | @ undefined");
            printf("did you mean '" FCOLOUR_YELLOW "%s" FCOLOUR_DEFAULT "'", _dist_command);
            printf(" [y/n]:");
            fflush(stdout);
            char *confirm = readline(" ");
            if (!confirm) {
                fprintf(stderr, "Error reading input\n");
                wd_free(confirm);
                goto done;
            }
            if (strlen(confirm) == 0) {
                wd_free(confirm);
                confirm = readline(" >>> [y/n]: ");
            }
            if (confirm) {
                if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
                    wd_free(confirm);
                    ptr_command = strdup(_dist_command);
                    goto _reexecute_command;
                } else if (strcmp(confirm, "N") == 0 || strcmp(confirm, "n") == 0) {
                    wd_free(confirm);
                    goto done;
                } else {
                    wd_free(confirm);
                    goto done;
                }
            } else {
                wd_free(confirm);
                goto done;
            }
        } else {
            if (!strcmp(ptr_command, "sh") || !strcmp(ptr_command, "bash") ||
                !strcmp(ptr_command, "zsh") || !strcmp(ptr_command, "make") ||
                !strcmp(ptr_command, "cd")
                )
            {
                    pr_error(stdout, "You can't run it!");
                    if (chain_pre_command && chain_pre_command[0] != '\0')
                        goto done;
                    else
                        goto _ptr_command;
            }
            char _p_command[WD_PATH_MAX];
            wd_snprintf(_p_command, WD_PATH_MAX, "%s", ptr_command);
            int ret = wd_run_command(_p_command);
            if (ret)
                wd_set_title("Watchdogs | @ command not found");
            return -WD_RETW;
        }

done:
        fflush(stdout);
        if (ptr_command) {
            wd_free(ptr_command);
            ptr_command = NULL;
        }

        return -WD_RETN;
}

void start_chain(void *chain_pre_command) {
        wd_set_title(NULL);

        __function__();

        int ret = -WD_RETW;

        if (chain_pre_command != NULL) {
            char *fetch_command_argv = (char*)chain_pre_command;
            ret = __command__(fetch_command_argv);

            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
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
        if (ret == -WD_RETN || ret == -WD_RETN) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <C> Finished at %.3fs\n",
                         command_dur);
            goto loop_main;
        } else if (ret == WD_RETW) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            exit(1);
        } else if (ret == -WD_RETW) {
            goto basic_end;
        } else {
            goto basic_end;
        }

basic_end:
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        goto loop_main;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        int i;
        size_t chain_total_len = 0;
        for (i = 1; i < argc; ++i)
            chain_total_len += strlen(argv[i]) + 1;

        char *chain_size_prompt;
        chain_size_prompt = wd_malloc(chain_total_len);
        if (!chain_size_prompt) {
            pr_error(stdout, "Failed to allocate memory for command");
            return WD_RETN;
        }

        chain_size_prompt[0] = '\0';
        for (i = 1; i < argc; ++i) {
            if (i > 1)
                strcat(chain_size_prompt, " ");
            strcat(chain_size_prompt, argv[i]);
        }

        start_chain(chain_size_prompt);
        wd_free(chain_size_prompt);
        return WD_RETZ;
    } else {
        start_chain(NULL);
    }

    return WD_RETZ;
}
