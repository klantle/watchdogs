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
#include <sys/file.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <archive.h>
#include <curl/curl.h>
#include <archive_entry.h>
#include <ncursesw/curses.h>
#include <readline/readline.h>
#include <readline/history.h>

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
        
		if (strcmp(wcfg.wd_toml_os_type, "windows") == 0) {
			wcfg.wd_os_type = OS_SIGNAL_WINDOWS;
		} else if (strcmp(wcfg.wd_toml_os_type, "linux") == 0) {
			wcfg.wd_os_type = OS_SIGNAL_LINUX;
		}
        if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS)) {
            wcfg.wd_ptr_samp="samp-server.exe";
            wcfg.wd_ptr_omp="omp-server.exe";
        } else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
            wcfg.wd_ptr_samp="samp03svr";
            wcfg.wd_ptr_omp="omp-server";
        }
        
        FILE *file_s = fopen(wcfg.wd_ptr_samp, "r");
        FILE *file_m = fopen(wcfg.wd_ptr_omp, "r");

        if (file_s && file_m) {
__default:
            wcfg.wd_is_omp = CRC32_FALSE;
            wcfg.wd_is_samp = CRC32_TRUE;
            fclose(file_s);
        } else if (file_s) {
            goto __default;
        } else if (file_m) {
            wcfg.wd_is_samp = CRC32_FALSE;
            wcfg.wd_is_omp = CRC32_TRUE;
            fclose(file_m);
        } else {
            goto __default;
        }

#if defined(_DBG_PRINT)
        printf_color(stdout,
                FCOLOUR_YELLOW,
                "-DEBUGGING\n");
        printf("[__function__]:\n"
               "\tos_type: %s\n"
               "\tpointer_samp: %s\n"
               "\tpointer_openmp: %s\n"
               "\tf_samp: %s\n"
               "\tf_openmp: %s\n",
                    wcfg.wd_os_type,
                    wcfg.wd_ptr_samp,
                    wcfg.wd_ptr_omp,
                    wcfg.wd_is_samp,
                    wcfg.wd_is_omp);
#endif
        return;
}

int __command__(void)
{
        char __cwd[PATH_MAX];
		size_t __sz_cwd = sizeof(__cwd);
        if (!getcwd(__cwd, __sz_cwd)) {
            perror("getcwd");
            return RETW;
        }

_ptr_command:
        char ptr_prompt[PATH_MAX + 56];
        if (strlen(__cwd) > 100)
            snprintf(ptr_prompt, sizeof(ptr_prompt), "[" FCOLOUR_YELLOW "watchdogs:%s" FCOLOUR_DEFAULT "]\n\t> $", __cwd);
        else
            snprintf(ptr_prompt, sizeof(ptr_prompt), "[" FCOLOUR_YELLOW "watchdogs:%s" FCOLOUR_DEFAULT "] > $", __cwd);
        char* ptr_command = readline(ptr_prompt);

        if (ptr_command == WD_ISNULL || ptr_command[0] == '\0')
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
                println(stdout, "Usage: help | help [<command>]");
                for (size_t i = 0; i < __command_len; i++)
                    printf(" -> %s\n", __command[i]);
            } else if (strcmp(arg, "clear") == 0) { println(stdout, "clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "exit") == 0) { println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "kill") == 0) { println(stdout, "kill: kill - refresh terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println(stdout, "title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "toml") == 0) { println(stdout, "toml: re-create toml - re-create & re-write watchdogs.toml\" | Usage: \"toml\"");
            } else if (strcmp(arg, "install") == 0) { println(stdout, "install: download & install depends | Usage: \"install\" | [<args>] - install github.com/github.com/gitea.com:user/repo:vtags");
            } else if (strcmp(arg, "upstream") == 0) { println(stdout, "upstream: get newer commits from upstream (gitlab). | Usage: \"upstream\"");
            } else if (strcmp(arg, "hardware") == 0) { println(stdout, "hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println(stdout, "gamemode: gamemode - download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println(stdout, "pawncc: pawncc - download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println(stdout, "running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "crunn") == 0) { println(stdout, "crunn: compile & running your project. | Usage: \"crunn\" | [<args>]");
            } else if (strcmp(arg, "debug") == 0) { println(stdout, "debug: debugging your project. | Usage: \"debug\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println(stdout, "stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println(stdout, "restart: restart server task. | Usage: \"restart\"");
            } else println(stdout, "help not found for: '%s'", arg);
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
            __function__();
            return RETN;
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') arg++;
            if (*arg == '\0') {
                println(stdout, "Usage: title [<title>]");
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

                // Borders Colors
                const char *BG = "\x1b[48;5;235m";
                const char *FG = "\x1b[97m";
                const char *BORD = "\x1b[33m";
                const char *RST = "\x1b[0m";
                                
                printf("%s%s+------------------------------------------+%s\n", BORD, FG, RST);

                char line[1024];
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
        } else if (strcmp(ptr_command, "upstream") == 0) {
            CURL *curl_handle;
            CURLcode res;
            struct MemoryStruct chunk;

            chunk.memory = malloc(1);
            chunk.size = 0;

            curl_global_init(CURL_GLOBAL_DEFAULT);
            curl_handle = curl_easy_init();

            if (!curl_handle) {
                fprintf(stderr, "Failed to initialize curl\n");
                return 1;
            }

            curl_easy_setopt(curl_handle, CURLOPT_URL,
                            "https://gitlab.com/api/v4/projects/75403219/repository/commits");
            curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
            curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
            curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

            res = curl_easy_perform(curl_handle);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
            } else {
                cJSON *root = cJSON_Parse(chunk.memory);
                if (!root) {
                    fprintf(stderr, "JSON parsing failed\n");
                } else {
                    cJSON *output_array = cJSON_CreateArray();
                    int array_size = cJSON_GetArraySize(root);

                    for (int i = 0; i < array_size; i++) {
                        cJSON *item = cJSON_GetArrayItem(root, i);
                        if (!item)
                            continue;

                        cJSON *id = cJSON_GetObjectItem(item, "id");
                        cJSON *title = cJSON_GetObjectItem(item, "title");
                        cJSON *author = cJSON_GetObjectItem(item, "author_name");
                        cJSON *date = cJSON_GetObjectItem(item, "created_at");

                        cJSON *commit_obj = cJSON_CreateObject();
                        cJSON_AddStringToObject(commit_obj, "id",
                                                id ? id->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "title",
                                                title ? title->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "author",
                                                author ? author->valuestring : "");
                        cJSON_AddStringToObject(commit_obj, "date",
                                                date ? date->valuestring : "");

                        cJSON_AddItemToArray(output_array, commit_obj);
                    }

                    char *pretty = cJSON_Print(output_array);

                    // Borders Colors
                    const char *BG = "\x1b[48;5;235m";
                    const char *FG = "\x1b[97m";
                    const char *BORD = "\x1b[33m";
                    const char *RST = "\x1b[0m";
                
                    printf("%s%s+------------------------------------------+%s\n", BORD, FG, RST);

                    char *line = strtok(pretty, "\n");
                    while (line) {
                        printf("%s%s|%s %-40.40s %s|%s\n", BORD, FG, BG, line, BORD, RST);
                        line = strtok(NULL, "\n");
                    }

                    printf("%s%s+------------------------------------------+%s\n", BORD, FG, RST);

                    free(pretty);
                    cJSON_Delete(output_array);
                    cJSON_Delete(root);
                }
            }

            curl_easy_cleanup(curl_handle);
            curl_global_cleanup();
            wdfree(chunk.memory);

            return RETN;
        } else if (strcmp(ptr_command, "hardware") == 0) {
            printf("Basic Summary:\n");
            hardware_show_summary();
            printf("\nSpecific Fields Query:\n");
            unsigned int specific_fields[] = {
                FIELD_CPU_NAME, FIELD_CPU_CORES, FIELD_MEM_TOTAL, FIELD_DISK_FREE
            };
            hardware_query_specific(specific_fields, 4);
            printf("\nDetailed Report:\n");
            hardware_show_detailed();
            return RETN;
        } else if (strncmp(ptr_command, "install", 7) == 0) {
            wd_set_title("Watchdogs | @ install depends");

            char *arg = ptr_command + 7;
            while (*arg == ' ') arg++;

            if (*arg) {
                wd_install_depends_str(arg);
            } else {
                char errbuf[256];
                toml_table_t *_toml_config;
                FILE *procc_f = fopen("watchdogs.toml", "r");
                _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                if (procc_f) fclose(procc_f);

                if (!_toml_config) {
                    printf_error(stdout, "parsing TOML: %s", errbuf);
                    return RETZ;
                }

                toml_table_t *wd_depends = toml_table_in(_toml_config, "depends");
                if (wd_depends) {
                    toml_array_t *wd_toml_aio_repo = toml_array_in(wd_depends, "wd_toml_aio_repo");
                    if (wd_toml_aio_repo) {
                        size_t arr_sz = toml_array_nelem(wd_toml_aio_repo);
                        char *merged = NULL;

                        for (size_t i = 0; i < arr_sz; i++) {
                            toml_datum_t val = toml_string_at(wd_toml_aio_repo, i);
                            if (!val.ok) continue;

                            size_t old_len = merged ? strlen(merged) : 0;
                            size_t new_len = old_len + strlen(val.u.s) + 2;

                            char *tmp = wdrealloc(merged, new_len);
                            if (!tmp) {
                                wdfree(merged);
                                wdfree(val.u.s);
                                merged = NULL;
                                break;
                            }
                            merged = tmp;

                            if (!old_len) {
                                snprintf(merged, new_len, "%s", val.u.s);
                            } else {
                                snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
                            }

                            wdfree(val.u.s);
                            val.u.s = NULL;
                        }

                        if (!merged) merged = strdup("");

                        wcfg.wd_toml_aio_repo = merged;
                        wd_install_depends_str(wcfg.wd_toml_aio_repo);
                    }
                }
                toml_free(_toml_config);
            }

            return RETN;
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            wd_set_title("Watchdogs | @ gamemode");
            char platform = 0;
ret_ptr1:
                println(stdout, "Select platform:");
                println(stdout, "-> [L/l] Linux");
                println(stdout, "-> [W/w] Windows");
                printf(" ^ work's in WSL/MSYS2\n");
                printf("==> ");

            if (scanf(" %c", &platform) != 1)
                return RETN;

            if (platform == 'L' || platform == 'l')
                wd_install_server("linux");
            else if (platform == 'W' || platform == 'w')
                wd_install_server("windows");
            else {
                printf_error(stdout, "Invalid platform selection. use C^ to exit.");
                goto ret_ptr1;
            }

            __main(0);
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            wd_set_title("Watchdogs | @ pawncc");
            char platform = 0;
ret_ptr2:
                println(stdout, "Select platform:");
                println(stdout, "-> [L/l] Linux");
                println(stdout, "-> [W/w] Windows");
                printf(" ^ work's in WSL/MSYS2\n");
                println(stdout, "-> [T/t] Termux");
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
                printf_error(stdout, "Invalid platform selection. use C^ to exit.");
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
        } if (strncmp(ptr_command, "debug", 5) == 0 || strncmp(ptr_command, "running", 7) == 0) {
_runners_:
                int is_debug = strncmp(ptr_command, "debug", 5) == 0;
                if (is_debug) wcfg.wd_runn_mode = "debug";

                wd_stop_server_tasks();

                int _wd_log_acces = path_acces("server_log.txt");
                if (_wd_log_acces)
                    remove("server_log.txt");
                _wd_log_acces = path_acces("log.txt");
                if (_wd_log_acces)
                    remove("log.txt");

                size_t cmd_len = is_debug ? 5 : 7;
                char *arg = ptr_command + cmd_len;
                while (*arg == ' ') arg++;
                char *arg1 = strtok(arg, " ");

		        size_t needed = snprintf(NULL, 0, "Watchdogs | @ running | args: %s | %s",
									              arg1,
									              "config.json") + 1;
		        char *title_running_info = wdmalloc(needed);
		        if (!title_running_info) { return RETN; }
		        snprintf(title_running_info, needed, "Watchdogs | @ running | args: %s | %s",
									                  arg1,
									                  "config.json");
		        if (title_running_info) {
			        wd_set_title(title_running_info);
			        wdfree(title_running_info);
			        title_running_info = NULL;
		        }

                printf_color(stdout, FCOLOUR_YELLOW, "running..\n");
#ifdef _WIN32

                system("C:\\msys64\\usr\\bin\\mintty.exe /bin/bash -c \"./watchdogs.win; pwd; exec bash\"");
#else
                if (is_termux_environment)
                    printf_error(stdout, "xterm not supported in termux!");
                else
                    system("xterm -hold -e bash -c 'echo \"here is your watchdogs!..\"; ./watchdogs' &");
#endif
                if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
                    if (arg == WD_ISNULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        char __sz_run[128];
#ifdef _WIN32
                        snprintf(__sz_run, sizeof(__sz_run), "%s", wcfg.wd_ptr_samp);
#else
                        chmod(wcfg.wd_ptr_samp, 0777);
                        snprintf(__sz_run, sizeof(__sz_run), "./%s", wcfg.wd_ptr_samp);
#endif
                        int running_FAIL = system(__sz_run);
                        if (running_FAIL == 0) {
                            if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                                sleep(2);
                                display_server_logs(0);
                            }
                        } else {
                            printf_color(stdout, FCOLOUR_RED, "running failed!\n");
                        }
                    } else {
                        wd_run_samp_server(arg1, wcfg.wd_ptr_samp);
                    }
                } else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
                    if (arg == WD_ISNULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        char __sz_run[128];
#ifdef _WIN32
                        snprintf(__sz_run, sizeof(__sz_run), "%s", wcfg.wd_ptr_omp);
#else
                        chmod(wcfg.wd_ptr_samp, 0777);
                        snprintf(__sz_run, sizeof(__sz_run), "./%s", wcfg.wd_ptr_omp);
#endif
                        int running_FAIL = system(__sz_run);
                        if (running_FAIL == 0) {
                            sleep(2);
                            display_server_logs(1);
                        } else {
                            printf_color(stdout, FCOLOUR_RED, "running failed!\n");
                        }
                    } else {
                        wd_run_omp_server(arg1, wcfg.wd_ptr_omp);
                    }
                } else if (!strcmp(wcfg.wd_is_samp, CRC32_FALSE) || !strcmp(wcfg.wd_is_omp, CRC32_FALSE)) {
                    printf_error(stdout, "samp-server/open.mp server not found!");

                    char *ptr_sigA;
ret_ptr3:
                    ptr_sigA = readline("install now? [Y/n]: ");

                    while (1) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS)) {
                                wd_install_server("windows");
                            } else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                                wd_install_server("linux");
                            }
                            break;
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            break;
                        } else {
                            printf_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
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
            if (wcfg.wd_compiler_stats < 1) {
                char errbuf[256];
                toml_table_t *_toml_config;
                FILE *procc_f = fopen("watchdogs.toml", "r");
                _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                if (procc_f) fclose(procc_f);

                if (!_toml_config) {
                    printf_error(stdout, "parsing TOML: %s", errbuf);
                    __main(0);
                }

                toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
                if (wd_compiler) {
                        toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
                        if (toml_gm_i.ok) 
                        {
                            wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
                            wdfree(toml_gm_i.u.s);
                            toml_gm_i.u.s = NULL;
                        }
                }
                toml_free(_toml_config);

                char __sz_gm_input[PATH_MAX];
                snprintf(__sz_gm_input, sizeof(__sz_gm_input), "%s", wcfg.wd_toml_gm_input);
                char *f_EXT = strrchr(__sz_gm_input, '.');
                if (f_EXT) 
                    *f_EXT = '\0';
                char *pos = strstr(__sz_gm_input, "gamemodes/");
                if (pos) {
                    memmove(pos, pos + strlen("gamemodes/"),
                                    strlen(pos + strlen("gamemodes/")) + 1);
                }
                if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
                    printf_color(stdout, FCOLOUR_YELLOW, "running..\n");
                    wd_run_samp_server(__sz_gm_input, wcfg.wd_ptr_samp);
                } else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
                    printf_color(stdout, FCOLOUR_YELLOW, "running..\n");
                    wd_run_samp_server(__sz_gm_input, wcfg.wd_ptr_omp);
                }
            }
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
            printf("did you mean '" FCOLOUR_YELLOW "%s" FCOLOUR_DEFAULT "'", _dist_command);
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
                strfind(ptr_command, "make") ||
                strfind(ptr_command, "cd")) {
                    printf_error(stdout, "you can't run it!");
                    goto _ptr_command;
                }
            char _p_command[256];
            snprintf(_p_command, 256, "%s", ptr_command);
            int ret = wd_run_command(_p_command);
            if (ret)
                wd_set_title("Watchdogs | @ command not found");
            return RETN;
        }

        if (ptr_command) {
            wdfree(ptr_command);
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
            printf_color(stdout, FCOLOUR_YELLOW, " ==> [C]Finished in %.3fs\n", command_dur);
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
