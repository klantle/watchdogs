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
#include <locale.h>
#include <sys/file.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <archive.h>
#include <signal.h>
#include <curl/curl.h>
#include <archive_entry.h>
#include <ncursesw/curses.h>
#include <readline/readline.h>
#include <readline/history.h>

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
#include "depends.h"

struct timespec cmd_start, cmd_end;
double command_dur;

void __function__(void) {
        wd_sef_fdir_reset();
        wd_u_history();
        wd_set_toml();
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
               "\tconfig: %s\n",
                __func__, __PRETTY_FUNCTION__, __LINE__, __FILE__,
                wcfg.wd_os_type, wcfg.wd_ptr_samp,
                wcfg.wd_ptr_omp, wcfg.wd_is_samp,
                wcfg.wd_is_omp, wcfg.wd_toml_config);
#endif
        return;
}
int __command__(char *pre_command)
{
        wcfg.wd_sel_stat = 0;
        setlocale(LC_ALL, "en_US.UTF-8");
        char __cwd[PATH_MAX];
        size_t __sz_cwd = sizeof(__cwd);
        if (!getcwd(__cwd, __sz_cwd)) {
            perror("getcwd");
            return __RETW;
        }
        char ptr_prompt[PATH_MAX + 56];
        size_t __sz_ptrp = sizeof(ptr_prompt);
        char *ptr_command;
        int c_distance = INT_MAX;
        const char *_dist_command;

_ptr_command:
        __function__();
        
        if (pre_command && pre_command[0] != '\0') {
            ptr_command = strdup(pre_command);
            printf("[" FCOLOUR_CYAN "watchdogs ~ %s" FCOLOUR_DEFAULT "]$ %s\n", __cwd, ptr_command);
        } else {
            snprintf(ptr_prompt, __sz_ptrp,
                     "[" FCOLOUR_CYAN "watchdogs ~ %s" FCOLOUR_DEFAULT "]$ ", __cwd);
            ptr_command = readline(ptr_prompt);

            if (ptr_command == NULL || ptr_command[0] == '\0')
                goto _ptr_command;
        }
        
        wd_a_history(ptr_command);
        
        _dist_command = wd_find_near_command(ptr_command,
                                             __command,
                                             __command_len,
                                             &c_distance);

_reexecute_command:
        clock_gettime(CLOCK_MONOTONIC, &cmd_start);
        if (strncmp(ptr_command, "help", 4) == 0) {
            wd_set_title("Watchdogs | @ help");

            char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') ++arg;

            if (strlen(arg) == 0) {
                println(stdout, "Usage: help | help [<command>]");

                for (size_t i = 0; i < __command_len; i++)
                    printf(" --|> %s\n", __command[i]);
            } else if (strcmp(arg, "clear") == 0) { println(stdout, "clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "exit") == 0) { println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "kill") == 0) { println(stdout, "kill: refresh terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println(stdout, "title: set-title terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "time") == 0) { println(stdout, "time: print current time. | Usage: \"time\"");
            } else if (strcmp(arg, "stopwatch") == 0) { println(stdout, "stopwatch: calculating time. Usage: \"stopwatch\" | [<args>]");
            } else if (strcmp(arg, "install") == 0) { println(stdout, "install: download & install depends | Usage: \"install\" |"
                                                                      "[<args>]\n\t- install github.com/github.com/gitea.com:user/repo:tags");
            } else if (strcmp(arg, "upstream") == 0) { println(stdout, "upstream: print newer commits from upstream (gitlab). | Usage: \"upstream\"");
            } else if (strcmp(arg, "hardware") == 0) { println(stdout, "hardware: hardware information. | Usage: \"hardware\"");
            } else if (strcmp(arg, "gamemode") == 0) { println(stdout, "gamemode: download sa-mp gamemode. | Usage: \"gamemode\"");
            } else if (strcmp(arg, "pawncc") == 0) { println(stdout, "pawncc: download sa-mp pawncc. | Usage: \"pawncc\"");
            } else if (strcmp(arg, "compile") == 0) { println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println(stdout, "running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "crunn") == 0) { println(stdout, "crunn: compile & running your project. | Usage: \"crunn\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println(stdout, "stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println(stdout, "restart: restart server task. | Usage: \"restart\"");
            } else println(stdout, "help not found for: '%s'", arg);

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

            _already_compiler_check = 0;

            __function__();
            
            goto _ptr_command;
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') ++arg;
            
            if (*arg == '\0') {
                println(stdout, "Usage: title [<title>]");
            } else {
                char title_set[128];
                snprintf(title_set, sizeof(title_set), "%s", arg);
                wd_set_title(title_set);
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
        } else if (strncmp(ptr_command, "stopwatch", 9) == 0) {
            struct timespec start, now;
            double stw_elp;

            char *arg = ptr_command + 10;
            while (*arg == ' ') ++arg;
            
            if (*arg == '\0') {
                println(stdout, "Usage: stopwatch [<sec>]");
            } else {
                int ts = atoi(arg);
                if (ts <= 0) {
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

                    int hh = (int)(stw_elp / 3600),
                        mm = (int)((stw_elp - hh*3600)/60),
                        ss = (int)(stw_elp) % 60;

                    char title_set[128];
                    snprintf(title_set, sizeof(title_set),
                            "S T O P W A T C H : %02d:%02d:%02d / %d sec",
                            hh, mm, ss, ts);
                    wd_set_title(title_set);

                    struct timespec ts = {0, 10000000};
                    nanosleep(&ts, NULL);
                }
            }

            goto done;
        } else if (strcmp(ptr_command, "toml") == 0) {
            if (access("watchdogs.toml", F_OK) == 0) {
                remove("watchdogs.toml");
            }
            
            _already_compiler_check = 0;

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
                size_t len;
                printf("%s%s+", BORD, FG);
                    for (int i = 0; i < 42; ++i)
                        putchar('-'); 
                printf("+%s\n", RST);

                char line[MAX_PATH * 4];

                while (fgets(line, sizeof(line), procc_f)) 
                {
                        len = strlen(line);
                        if (len && (line[len - 1] == '\n' ||
                                    line[len - 1] == '\r'))
                                line[--len] = '\0';

                        printf("%s%s|%s %-40s %s|%s\n",
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

			if (is_native_windows()) {
				if (access("cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "cacert.pem");
				else if (access("C:/libwatchdogs/cacert.pem", F_OK) == 0)
        			curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
				else
					pr_color(stdout, FCOLOUR_YELLOW, "Warning: No CA file found. SSL verification may fail.\n");
			}

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

                    printf("%s%s+", BORD, FG);
                        for (int i = 0; i < 42; ++i)
                            putchar('-'); 
                    printf("+%s\n", RST);

                    char *line, *pretty = cJSON_Print(output_array);

                    line = strtok(pretty, "\n");
                    while (line) 
                    {
                            printf("%s%s|%s %-40.40s %s|%s\n",
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

                            snprintf(merged, strlen(val.u.s) + 1, "%s", val.u.s);
                    } else {
                            char *tmp;
                            size_t old_len = strlen(merged);
                            size_t new_len = old_len + strlen(val.u.s) + 2;

                            tmp = wd_realloc(merged, new_len);
                            if (!tmp)
                                    goto free_val;

                            merged = tmp;
                            snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
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

            wd_run_compiler(arg, compile_args);
            
            goto done;
        } if (strncmp(ptr_command, "running", 7) == 0) {
_runners_:
                wd_stop_server_tasks();

                int _wd_log_acces = path_acces(".server_log.txt");
                if (_wd_log_acces)
                    remove(".server_log.txt");
                _wd_log_acces = path_acces(".log.txt");
                if (_wd_log_acces)
                    remove(".log.txt");

                size_t cmd_len = 7;
                char *arg = ptr_command + cmd_len;
                while (*arg == ' ') ++arg;
                char *arg1 = strtok(arg, " ");

		        size_t needed = snprintf(NULL, 0, "Watchdogs | @ running | args: %s | %s",
									              arg1,
									              wcfg.wd_toml_config) + 1;
		        char *title_running_info = wd_malloc(needed);
		        if (!title_running_info) { return __RETN; }
		        snprintf(title_running_info, needed, "Watchdogs | @ running | args: %s | %s",
									                  arg1,
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
                char* msys2_env = getenv("MSYSTEM");
		        int is_native = __RETZ;
		        int is_win32 = __RETZ;
                int is_linux = __RETZ;
#ifdef _WIN32
		        is_win32 = __RETN;
#endif
#ifdef __linux__
                is_linux = __RETN;
#endif
                printf(" kkkkkkkkkkkkkkkkkkkk %d %d %d", is_native, is_win32, is_linux);
                if (msys2_env == NULL && is_win32 == __RETN) {
                    is_native = __RETN;
                } else if (msys2_env == NULL && is_linux == 1) {
                    is_native = __RETW;
                }
                if (is_native == __RETZ) {
#ifdef _WIN32
                    FILE *fp;
                    fp = popen("powershell \"Get-Process mintty -ErrorAction SilentlyContinue | "
                               "Where-Object {$_.MainWindowTitle -like '*watchdogs*'} | "
                               "Measure-Object | Select-Object -Expand Count\"", "r");
                    if (fp) {
                        char buffer[128];
                        if (fgets(buffer, sizeof(buffer), fp)) {
                            int count = atoi(buffer);
                            if (count < 2) {
                                system("start \"\" \"C:\\\\msys64\\\\usr\\\\bin\\\\mintty.exe\" "
                                    "-e /bin/bash -lc \"echo here is your watchdogs!...; pwd; ./watchdogs.win; bash &\"");
                            }
                        }
                        pclose(fp);
                    }
#endif
                } else if (is_native == __RETW) {
                    if (is_termux_environment())
                        pr_error(stdout, "xterm not supported in termux!");
                    else {
#ifndef _WIN32
                        int result = system("pgrep -c watchdogs > /dev/null");
                        if (result != -1 && WEXITSTATUS(result) < 2)
                            system("xterm -hold -e bash -c 'echo \"here is your watchdogs!..\"; ./watchdogs; bash' &");
#endif
                    }
                }

                if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE)) {
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        char __sz_run[128];
    
                        struct timespec start, now;
                        double stw_elp;
                        
                        clock_gettime(CLOCK_MONOTONIC, &start);

                        while (true) {
                                clock_gettime(CLOCK_MONOTONIC, &now);

                                stw_elp = (now.tv_sec - start.tv_sec)
                                        + (now.tv_nsec - start.tv_nsec) / 1e9;

                                int hh = (int)(stw_elp / 3600),
                                    mm = (int)((stw_elp - hh*3600)/60),
                                    ss = (int)(stw_elp) % 60;

                                char title_set[128];
                                snprintf(title_set, sizeof(title_set),
                                        "S T O P W A T C H : %02d:%02d:%02d | CTRL + C to stop.",
                                        hh, mm, ss);
                                wd_set_title(title_set);

                                struct timespec ts = {0, 10000000};
                                nanosleep(&ts, NULL);
                        }

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
                                wd_display_server_logs(0);
                            }
                        } else {
                            pr_color(stdout, FCOLOUR_RED, "running failed!\n");
                        }
                    } else {
                        wd_run_samp_server(arg1, wcfg.wd_ptr_samp);
                    }
                } else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
                    if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) {
                        char __sz_run[128];
                        
                        struct timespec start, now;
                        double stw_elp;
                        
                        clock_gettime(CLOCK_MONOTONIC, &start);

                        while (true) {
                                clock_gettime(CLOCK_MONOTONIC, &now);

                                stw_elp = (now.tv_sec - start.tv_sec)
                                        + (now.tv_nsec - start.tv_nsec) / 1e9;

                                int hh = (int)(stw_elp / 3600),
                                    mm = (int)((stw_elp - hh*3600)/60),
                                    ss = (int)(stw_elp) % 60;

                                char title_set[128];
                                snprintf(title_set, sizeof(title_set),
                                        "S T O P W A T C H : %02d:%02d:%02d | CTRL + C to stop.",
                                        hh, mm, ss);
                                wd_set_title(title_set);

                                struct timespec ts = {0, 10000000};
                                nanosleep(&ts, NULL);
                        }

#ifdef _WIN32
                        snprintf(__sz_run, sizeof(__sz_run), "%s", wcfg.wd_ptr_omp);
#else
                        chmod(wcfg.wd_ptr_samp, 0777);
                        snprintf(__sz_run, sizeof(__sz_run), "./%s", wcfg.wd_ptr_omp);
#endif
                        int running_FAIL = system(__sz_run);
                        if (running_FAIL == 0) {
                            sleep(2);
                            wd_display_server_logs(1);
                        } else {
                            pr_color(stdout, FCOLOUR_RED, "running failed!\n");
                        }
                    } else {
                        wd_run_omp_server(arg1, wcfg.wd_ptr_omp);
                    }
                } else if (!strcmp(wcfg.wd_is_samp, CRC32_FALSE) || !strcmp(wcfg.wd_is_omp, CRC32_FALSE)) {
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
        } else if (strncmp(ptr_command, "crunn", 7) == 0) {
            wd_set_title("Watchdogs | @ crunn");
            const char *arg = NULL;
            const char *compile_args = NULL;
            wd_run_compiler(arg, compile_args);
            if (wcfg.wd_compiler_stat < 1) {
                char errbuf[256];
                toml_table_t *_toml_config;
                FILE *procc_f = fopen("watchdogs.toml", "r");
                _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                if (procc_f) fclose(procc_f);

                if (!_toml_config) {
                    pr_error(stdout, "parsing TOML: %s", errbuf);
                    wd_main(NULL);
                }

                toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
                if (wd_compiler) {
                        toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
                        if (toml_gm_i.ok) 
                        {
                            wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
                            wd_free(toml_gm_i.u.s);
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
                    pr_color(stdout, FCOLOUR_YELLOW, "running..\n");
                    wd_run_samp_server(__sz_gm_input, wcfg.wd_ptr_samp);
                } else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE)) {
                    pr_color(stdout, FCOLOUR_YELLOW, "running..\n");
                    wd_run_samp_server(__sz_gm_input, wcfg.wd_ptr_omp);
                }
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
#ifndef _WIN32
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
            if (
                strfind(ptr_command, "sh") ||
                strfind(ptr_command, "bash") ||
                strfind(ptr_command, "zsh") ||
                strfind(ptr_command, "make") ||
                strfind(ptr_command, "cd")
                )
            {
                    pr_error(stdout, "You can't run it!");
                    goto _ptr_command;
            }
            char _p_command[256];
            snprintf(_p_command, 256, "%s", ptr_command);
            int ret = wd_run_command(_p_command);
            if (ret)
                wd_set_title("Watchdogs | @ command not found");
            return -__RETW;
        }

done:
        if (ptr_command) {
            wd_free(ptr_command);
            ptr_command = NULL;
        }

        return -__RETN;
}

void wd_main(void *pre_command) {
        wd_set_title(NULL);
        int ret = -__RETW;
        
        if (pre_command != NULL) {
            char *cmd_from_argv = (char*)pre_command;
            ret = __command__(cmd_from_argv);
            
            clock_gettime(CLOCK_MONOTONIC, &cmd_end);
            command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
                          (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
            pr_color(stdout,
                         FCOLOUR_GREEN,
                         " ==> [C]Finished in %.3fs\n",
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
                         FCOLOUR_GREEN,
                         " ==> [C]Finished in %.3fs\n",
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