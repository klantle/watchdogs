#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#include "wd_unit.h"
#include "wd_extra.h"
#include "wd_util.h"
#include "wd_server.h"

FILE *config_in = NULL, *config_out = NULL;
cJSON *sroot = NULL, *pawn = NULL, *main_scripts = NULL;
char *__cJSON_Data = NULL, *__cJSON_Printed = NULL;

int server_mode = 0;
void unit_handle_sigint(int sig) {
        if (server_mode == 1) {
          restore_samp_config();
          restore_omp_config();
        }
        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);
#ifdef __ANDROID__
#ifndef _DBG_PRINT
    system("exit && ./watchdogs.tmux");
#else
    system("exit && ./watchdogs.debug.tmux");
#endif
#elif defined(__linux__)
#ifndef _DBG_PRINT
    system("exit && ./watchdogs");
#else
    system("exit && ./watchdogs.debug");
#endif
#elif defined(_WIN32)
#ifndef _DBG_PRINT
    system("exit && watchdogs.win");
#else
    system("exit && watchdogs.debug.win");
#endif
#endif
}

void wd_stop_server_tasks(void) {
        kill_process(wcfg.wd_ptr_samp);
        kill_process(wcfg.wd_ptr_omp);
}

static int update_server_config(const char *gamemode)
{
        FILE *config_in, *config_out;
        char line[1024];
        int gamemode_updated = 0;

        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);

        char __sz_mv[MAX_PATH];
        if (is_native_windows())
            snprintf(__sz_mv, sizeof(__sz_mv),
                    "ren %s %s",
                    wcfg.wd_toml_config,
                    __sz_config);
        else
            snprintf(__sz_mv, sizeof(__sz_mv),
                    "mv -f %s %s",
                    wcfg.wd_toml_config,
                    __sz_config);

        if (system(__sz_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -__RETN;
        }

        config_in = fopen(__sz_config, "r");
        if (!config_in) {
                pr_error(stdout, "Failed to open backup config");
                return -__RETN;
        }
        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write new config");
                fclose(config_in);
                return -__RETN;
        }

        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
        char *__dot_ext = strrchr(_gamemode, '.');
        if (__dot_ext) *__dot_ext = '\0';
        gamemode = _gamemode;

        while (fgets(line, sizeof(line), config_in)) {
                if (strncmp(line, "gamemode0 ", 10) == 0) {
                        fprintf(config_out, "gamemode0 %s\n", gamemode);
                        gamemode_updated = 1;
                } else {
                        fputs(line, config_out);
                }
        }

        if (!gamemode_updated)
                fprintf(config_out, "gamemode0 %s\n", gamemode);

        fclose(config_in);
        fclose(config_out);

        return __RETZ;
}

void wd_display_server_logs(int ret)
{
        FILE *log_file;
        int ch;

        if (!ret)
                log_file = fopen("server_log.txt", "r");  /* SA-MP log */
        else
                log_file = fopen("log.txt", "r");         /* OpenMP log */

        if (!log_file) {
                return;
        }

        /* Output log contents character by character */
        while ((ch = fgetc(log_file)) != EOF)
                putchar(ch);

        fclose(log_file);
}

void restore_samp_config(void) {
        char __sz_config[PATH_MAX * 2];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);
        remove(wcfg.wd_toml_config);
}

void wd_run_samp_server(const char *gamemode, const char *server_bin)
{
        char command[256];
        int ret;

        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
        char *__dot_ext = strrchr(gamemode, '.');
        if (!__dot_ext) gamemode = _gamemode;

        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
                wd_main(NULL);
        }

        if (update_server_config(gamemode) != 0)
                wd_main(NULL);

        CHMOD(server_bin, FILE_MODE);

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
#ifdef _WIN32
        snprintf(command, sizeof(command), "%s", server_bin);
#else
        snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);

        ret = system(command);
        if (ret == __RETZ) {
                if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                        sleep(2);
                        wd_display_server_logs(0);
                }
        } else {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);
                if (elapsed <= 5.0)
                {
                  if (ret_serv == 0) {
                    ret_serv = 1;
                    printf("\ttry starting again..");
                    goto back_start;
                  }
                }

        }
        restore_samp_config();

        wd_main(NULL);
}

static int update_omp_config(const char *gamemode)
{
        struct stat st;
        char gamemode_buf[256];
        char _gamemode[256];
        int ret = -__RETN;

        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);

        char __sz_mv[MAX_PATH];
        if (is_native_windows())
            snprintf(__sz_mv, sizeof(__sz_mv),
                    "ren %s %s",
                    wcfg.wd_toml_config,
                    __sz_config);
        else
            snprintf(__sz_mv, sizeof(__sz_mv),
                    "mv -f %s %s",
                    wcfg.wd_toml_config,
                    __sz_config);

        if (system(__sz_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -__RETN;
        }

        if (stat(__sz_config, &st) != 0) {
                pr_error(stdout, "Failed to get file status");
                return -__RETN;
        }

        config_in = fopen(__sz_config, "rb");
        if (!config_in) {
                pr_error(stdout, "Failed to open %s", __sz_config);
                return -__RETN;
        }

        __cJSON_Data = wd_malloc(st.st_size + 1);
        if (!__cJSON_Data) {
                pr_error(stdout, "Memory allocation failed");
                goto done;
        }

        size_t bytes_read = fread(__cJSON_Data, 1, st.st_size, config_in);
        if (bytes_read != (size_t)st.st_size) {
                pr_error(stdout, "Incomplete file read (%zu of %ld bytes)",
                        bytes_read,
                        st.st_size);
                goto done;
        }

        __cJSON_Data[st.st_size] = '\0';
        fclose(config_in);
        config_in = NULL;

        sroot = cJSON_Parse(__cJSON_Data);
        if (!sroot) {
                pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
                goto done;
        }

        pawn = cJSON_GetObjectItem(sroot, "pawn");
        if (!pawn) {
                pr_error(stdout, "Missing 'pawn' section in config");
                goto done;
        }

        snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
        char *__dot_ext = strrchr(_gamemode, '.');
        if (__dot_ext) *__dot_ext = '\0';

        cJSON_DeleteItemFromObject(pawn, "main_scripts");

        main_scripts = cJSON_CreateArray();
        snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", _gamemode);
        cJSON_AddItemToArray(main_scripts, cJSON_CreateString(gamemode_buf));
        cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write %s", wcfg.wd_toml_config);
                goto done;
        }

        __cJSON_Printed = cJSON_Print(sroot);
        if (!__cJSON_Printed) {
                pr_error(stdout, "Failed to print JSON");
                goto done;
        }

        if (fputs(__cJSON_Printed, config_out) == EOF) {
                pr_error(stdout, "Failed to write to %s", wcfg.wd_toml_config);
                goto done;
        }

        ret = __RETZ;

done:
        if (config_out)
                fclose(config_out);
        if (config_in)
                fclose(config_in);
        if (__cJSON_Printed)
                wd_free(__cJSON_Printed);
        if (sroot)
                cJSON_Delete(sroot);
        if (__cJSON_Data)
                wd_free(__cJSON_Data);

        return ret;
}

void restore_omp_config(void) {
        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);

        remove(wcfg.wd_toml_config);
        rename(__sz_config, wcfg.wd_toml_config);
}

void wd_run_omp_server(const char *gamemode, const char *server_bin)
{
        char command[256];
        int ret;

        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
        char *__dot_ext = strrchr(gamemode, '.');
        if (!__dot_ext) gamemode = _gamemode;

        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
                wd_main(NULL);
        }

        if (update_omp_config(gamemode) != 0)
                return;

        CHMOD(server_bin, FILE_MODE);

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
#ifdef _WIN32
        snprintf(command, sizeof(command), "%s", server_bin);
#else
        snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);

        ret = system(command);
        if (ret == __RETZ) {
                sleep(2);
                wd_display_server_logs(1);
        } else {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);
                if (elapsed <= 5.0)
                {
                  if (ret_serv == 0) {
                    ret_serv = 1;
                    printf("\ttry starting again..");
                    goto back_start;
                  }
                }
        }
        restore_omp_config();

        wd_main(NULL);
}
