#define WATCHDOGS_RELEASE "WD-251101"
const char *watchdogs_release = WATCHDOGS_RELEASE;
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
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
#include <ncursesw/curses.h>
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
        pr_color(stdout,
                FCOLOUR_YELLOW,
                "-DEBUGGING\n");
        printf("[function: %s | pretty function: %s | line: %d | file: %s]:\n"
               "\tos_type: %s (CRC32)"
               "\tpointer_samp: %s\n"
               "\tpointer_openmp: %s"
               "\tf_samp: %s (CRC32)\n"
               "\tf_openmp: %s (CRC32)\n"
               "\tconfig: %s\n"
               "\tgh tokens: %s\n",
                __func__, __PRETTY_FUNCTION__, __LINE__, __FILE__,
                wcfg.wd_os_type, wcfg.wd_ptr_samp,
                wcfg.wd_ptr_omp, wcfg.wd_is_samp,
                wcfg.wd_is_omp, wcfg.wd_toml_config,
                wcfg.wd_toml_github_tokens);
#endif
        return;
}

int __command__(char *pre_command)
{
        wcfg.wd_sel_stat = 0;
        setlocale(LC_ALL, "en_US.UTF-8");
        int _wd_crash_ck = path_acces(".crashdetect_ck");
        if (_wd_crash_ck) {
          remove(".crashdetect_ck");
          wd_server_crash_check();
        }
        int wd_compile_running = 0;
        char ptr_prompt[WD_PATH_MAX + 56];
        size_t size_ptrp = sizeof(ptr_prompt);
        char *ptr_command;
        int c_distance = INT_MAX;
        const char *_dist_command;
        pr_color(stdout, FCOLOUR_CYAN, "%s", "a");

_ptr_command:
        if (pre_command && pre_command[0] != '\0') {
            ptr_command = strdup(pre_command);
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
                println(stdout, "Usage: help | help [<command>]");

                for (size_t i = 0; i < __command_len; i++) {
                    if (strstr(__command[i], "help"))
                        continue;
                    printf("\t@ [=| %s\n", __command[i]);
                }
            } else if (strcmp(arg, "clear") == 0) { println(stdout, "clear: clear screen watchdogs. | Usage: \"clear\"");
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
            } else if (strcmp(arg, "upstream") == 0) { println(stdout, "upstream: print newer commits from upstream (gitlab). | Usage: \"upstream\"");
            } else if (strcmp(arg, "hardware") == 0) { println(stdout, "hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println(stdout, "gamemode: download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println(stdout, "pawncc: download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println(stdout, "running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "crunn") == 0) { println(stdout, "crunn: compile & running your project. | Usage: \"crunn\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println(stdout, "stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println(stdout, "restart: restart server task. | Usage: \"restart\"");
            } else {
              printf("help not found!");
            }
            goto done;
        } else if (strcmp(ptr_command, "clear") == 0) {
            wd_set_title("Watchdogs | @ clear");

            if (!is_native_windows()) {
                wd_run_command("clear");
            } else {
                wd_run_command("cls");
            }

            goto _ptr_command;
        } else if (strcmp(ptr_command, "exit") == 0) {
            exit(1);
        } else if (strcmp(ptr_command, "kill") == 0) {
            wd_set_title("Watchdogs | @ kill");

            if (!is_native_windows()) {
                wd_run_command("clear");
            } else {
                wd_run_command("cls");
            }

            __function__();

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

                if (crypto_generate_sha256_hash(arg, digest) != __RETN) {
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

            __function__();

            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (procc_f) {
#ifdef WD_WINDOWS
                HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
                if (hOut != INVALID_HANDLE_VALUE) {
                    DWORD dwMode = 0;
                    if (GetConsoleMode(hOut, &dwMode))
                        SetConsoleMode(hOut,
                                       dwMode |
                                       ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                }
#endif
                size_t len;
                printf("%s%s+", BORD, FG);
                    for (int i = 0; i < 42; ++i)
                        putchar('-');
                printf("+%s\n", RST);

                char line[WD_MAX_PATH * 4];

                while (fgets(line, sizeof(line), procc_f))
                {
                        len = strlen(line);
                        if (len && (line[len - 1] == '\n' ||
                                    line[len - 1] == '\r'))
                                line[--len] = '\0';

                        printf("%s%s|"
                               "%s "
                               "%-40s "
                               "%s|"
                               "%s\n",
                                BORD, FG, BG,
                                line, BORD, RST);
                }

                printf("%s%s+", BORD, FG);
                    for (int i = 0; i < 42; ++i)
                        putchar('-');
                printf("+%s\n", RST);

                fclose(procc_f);
            }

            goto done;
        } else if (strncmp(ptr_command, "stopwatch", 9) == 0) {
            struct timespec start, now;
            double stw_elp;

            char *arg = ptr_command + 10;
            while (*arg == ' ') ++arg;

            if (*arg == '\0') {
                println(stdout, "Usage: stopwatch [<sec>]");
            } else {
                int ts;
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
                        wd_set_title(
                            "S T O P W A T C H : DONE");
                        break;
                    }

                    int hh, mm, ss;
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
        } else if (strcmp(ptr_command, "upstream") == 0) {
            CURL *curl_handle;
            CURLcode res;
            struct memory_struct chunk = { 0 };

            chunk.memory = wd_malloc(1);
            chunk.size = 0;

            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl_handle = curl_easy_init();

            if (!curl_handle) {
                fprintf(stderr, "Failed to initialize curl\n");
                return __RETN;
            }

            curl_easy_setopt(curl_handle, CURLOPT_URL,
                            "https://gitlab.com/api/v4/projects/75403219/repository/commits");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            verify_cacert_pem(curl_handle);

            fflush(stdout);
            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            } else {
                cJSON *root = cJSON_Parse(chunk.memory);
                cJSON *output_array, *id,
                      *title, *author, *date, *commit_obj;
                if (!root) {
                    fprintf(stderr, "JSON parsing failed\n");
                } else {
                    output_array = cJSON_CreateArray();
                    int array_size = cJSON_GetArraySize(root);

                    for (int i = 0; i < array_size; i++) {
                        cJSON *item = cJSON_GetArrayItem(root, i);
                        if (!item)
                            continue;

                        id = cJSON_GetObjectItem(item, "id");
                        title = cJSON_GetObjectItem(item, "title");
                        author = cJSON_GetObjectItem(item, "author_name");
                        date = cJSON_GetObjectItem(item, "created_at");

                        commit_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(commit_obj, "id", id ? id->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "title", title ? title->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "author", author ? author->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "date", date ? date->valuestring : "");
                        cJSON_AddItemToArray(output_array, commit_obj);
                    }

                    printf("%s%s+", BORD, FG);
                        for (int i = 0; i < 42; ++i)
                            putchar('-');
                    printf("+%s\n", RST);

                    char *line, *pretty;
                    pretty = cJSON_Print(output_array);

                    line = strtok(pretty, "\n");
                    while (line)
                    {
                            printf("%s%s|"
                                   "%s "
                                   "%-40.40s "
                                   "%s|"
                                   "%s\n",
                                    BORD, FG, BG,
                                    line, BORD, RST);

                            line = strtok(NULL, "\n");
                    }

                    printf("%s%s+", BORD, FG);
                        for (int i = 0; i < 42; ++i)
                            putchar('-');
                    printf("+%s\n", RST);

                    wd_free(pretty);
                    cJSON_Delete(output_array);
                    cJSON_Delete(root);
                }
            }

            curl_easy_cleanup(curl_handle);
            curl_global_cleanup();
            wd_free(chunk.memory);

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
                FILE *procc_f = fopen("watchdogs.toml", "r");
                _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                if (procc_f) fclose(procc_f);

                if (!_toml_config) {
                    pr_error(stdout, "parsing TOML: %s", errbuf);
                    return __RETZ;
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
            println(stdout, "Select platform:");
            println(stdout, "-> [L/l] Linux");
            println(stdout, "-> [W/w] Windows");
            pr_color(stdout, FCOLOUR_YELLOW, " ^ ");
            println(stdout, "work's in WSL/MSYS2");

            wcfg.wd_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L")) {
                wd_free(ptr_command);
                wd_free(platform);
                int ret = wd_install_server("linux");
loop_igm:
                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                    goto loop_igm;
                else if (ret == __RETZ)
                    goto _ptr_command;
            } else if (strfind(platform, "W")) {
                wd_free(ptr_command);
                wd_free(platform);
                int ret = wd_install_server("windows");
loop_igm2:
                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                    goto loop_igm2;
                else if (ret == __RETZ)
                    goto _ptr_command;
            } else if (strfind(platform, "E")) {
                wd_free(ptr_command);
                wd_free(platform);
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
            println(stdout, "Select platform:");
            println(stdout, "-> [L/l] Linux");
            println(stdout, "-> [W/w] Windows");
            pr_color(stdout, FCOLOUR_YELLOW, " ^ ");
            println(stdout, "work's in WSL/MSYS2");
            println(stdout, "-> [T/t] Termux");

            wcfg.wd_sel_stat = 1;

            char *platform = readline("==> ");

            if (strfind(platform, "L")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("linux");
loop_ipcc:
                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc;
                else if (ret == __RETZ)
                    goto _ptr_command;
            } else if (strfind(platform, "W")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("windows");
loop_ipcc2:
                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc2;
                else if (ret == __RETZ)
                    goto _ptr_command;
            } else if (strfind(platform, "T")) {
                wd_free(ptr_command);
                wd_free(platform);
                ptr_command = NULL;
                int ret = wd_install_pawncc("termux");
loop_ipcc3:
                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                    goto loop_ipcc3;
                else if (ret == __RETZ)
                    goto _ptr_command;
            } else if (strfind(platform, "E")) {
                wd_free(ptr_command);
                wd_free(platform);
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
            arg = ptr_command + 7;
            while (*arg == ' ') ++arg;
            char *compile_args;
            compile_args = strtok(arg, " ");
            char *second_arg = NULL, *four_arg = NULL,
                 *five_arg = NULL, *six_arg = NULL,
                 *seven_arg = NULL, *eight_arg = NULL;
            second_arg = strtok(NULL, " ");
            four_arg = strtok(NULL, " ");
            five_arg = strtok(NULL, " ");
            six_arg = strtok(NULL, " ");
            seven_arg = strtok(NULL, " ");
            eight_arg = strtok(NULL, " ");

            wd_run_compiler(arg,
                            compile_args,
                            second_arg,
                            four_arg,
                            five_arg,
                            six_arg,
                            seven_arg,
                            eight_arg);

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

                size_t needed = wd_snprintf(NULL, 0, "Watchdogs | "
                                                      "@ running | "
                                                      "args: %s | "
                                                      "gamemode: %s | "
                                                      "config: %s | "
                                                      "CTRL + C to stop.",
                                                      size_arg1,
                                                      gamemode,
                                                      wcfg.wd_toml_config) + 1;
                char *title_running_info = wd_malloc(needed);
                if (!title_running_info) { return __RETN; }
                wd_snprintf(title_running_info, needed, "Watchdogs | "
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
                                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
                                    goto n_loop_igm;
                            } else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                                int ret = wd_install_server("linux");
n_loop_igm2:
                                if (ret == -__RETN && wcfg.wd_sel_stat != 0)
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
        } else if (strcmp(ptr_command, "crunn") == 0) {
            wd_set_title("Watchdogs | @ crunn");

            const char *arg = NULL;
            /* target */
            const char *compile_args = NULL;
            /* options */
            const char *second_arg = NULL;
            const char *four_arg = NULL;
            const char *five_arg = NULL;
            const char *six_arg = NULL;
            const char *seven_arg = NULL;
            const char *eight_arg = NULL;

            wd_compile_running = 1;

            wd_run_compiler(arg,
                            compile_args,
                            second_arg,
                            four_arg,
                            five_arg,
                            six_arg,
                            seven_arg,
                            eight_arg);

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
            goto _ptr_command;
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            wd_set_title("Watchdogs | @ undefined");
            printf("did you mean '" FCOLOUR_YELLOW "%s" FCOLOUR_DEFAULT "'", _dist_command);
            printf(" [y/n]: ");
            fflush(stdout);
            char *confirm = readline("");
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
            if (strstr(ptr_command, "sh") ||
                strstr(ptr_command, "bash") ||
                strstr(ptr_command, "zsh") ||
                strstr(ptr_command, "make") ||
                strstr(ptr_command, "cd")
                )
            {
                    pr_error(stdout, "You can't run it!");
                    goto _ptr_command;
            }
            char _p_command[256];
            wd_snprintf(_p_command, 256, "%s", ptr_command);
            int ret = wd_run_command(_p_command);
            if (ret)
                wd_set_title("Watchdogs | @ command not found");
            return -__RETW;
        }

done:
        fflush(stdout);
        if (ptr_command) {
            wd_free(ptr_command);
            ptr_command = NULL;
        }

        return -__RETN;
}

void wd_main(void *pre_command) {
        __function__();
        wd_set_title(NULL);
        int ret = -__RETW;

        if (pre_command != NULL) {
            char *cmd_from_argv = (char*)pre_command;
            ret = __command__(cmd_from_argv);

            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <T> [C]Finished at %.3fs\n",
                         command_dur);
            return;
        }

loop_main:
        ret = __command__(NULL);
        if (ret == -__RETN || ret == __RETZ || ret == __RETN) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_CYAN,
                         " <T> [C]Finished at %.3fs\n",
                         command_dur);
            goto loop_main;
        } else if (ret == __RETW) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            exit(1);
        } else if (ret == -__RETW) {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            goto loop_main;
        } else {
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            goto loop_main;
        }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        size_t total_len = 0;
        for (int i = 1; i < argc; i++) {
            total_len += strlen(argv[i]) + 1;
        }

        char *sz_command = wd_malloc(total_len);
        if (!sz_command) {
            pr_error(stdout, "Failed to allocate memory for command");
            return __RETN;
        }

        sz_command[0] = '\0';
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                strcat(sz_command, " ");
            strcat(sz_command, argv[i]);
        }

        wd_main(sz_command);
        wd_free(sz_command);
        return __RETZ;
    } else {
        wd_main(NULL);
    }

    return __RETZ;
}
