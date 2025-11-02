/*
 * Server Management Module
 * 
 * This module handles SA-MP and OpenMP server operations including:
 * - Configuration file management
 * - Signal handling for graceful shutdown
 * - Server process execution and monitoring
 * - Backup and restoration of configuration files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>

/* Local module headers */
#include "chain.h"
#include "color.h"
#include "utils.h"
#include "server.h"

/*
 * Signal handler for SIGINT (Ctrl+C)
 * Performs cleanup operations and exits gracefully
 */
void __handle_sigint(int sig) {
        restore_samp_config();    /* Restore original SA-MP config */
        restore_omp_config();     /* Restore original OpenMP config */
        _exit(0);                 /* Force exit without cleanup */
}

/*
 * Stop server tasks by killing running processes
 * Called during shutdown or error conditions
 */
void wd_stop_server_tasks(void)
{
        kill_process(wcfg.wd_ptr_samp);  /* Kill SA-MP process */
        kill_process(wcfg.wd_ptr_omp);   /* Kill OpenMP process */
}

/*
 * Update SA-MP server configuration with new gamemode
 * Creates backup, modifies config, and handles errors
 */
static int update_server_config(const char *gamemode)
{
        FILE *config_in, *config_out;
        char line[1024];
        int gamemode_updated = 0;

        /* Create backup filename */
        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);
        
        /* Platform-specific move command for backup */
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

        /* Open backup config for reading */
        config_in = fopen(__sz_config, "r");
        if (!config_in) {
                pr_error(stdout, "Failed to open backup config");
                return -__RETN;
        }
        
        /* Create new config file */
        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write new config");
                fclose(config_in);
                return -__RETN;
        }

        /* Process gamemode filename - remove extension if present */
        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
        char *__dot_ext = strrchr(_gamemode, '.');
        if (__dot_ext) *__dot_ext = '\0';
        gamemode = _gamemode;

        /* Read backup config line by line and update gamemode */
        while (fgets(line, sizeof(line), config_in)) {
                if (strncmp(line, "gamemode0 ", 10) == 0) {
                        fprintf(config_out, "gamemode0 %s\n", gamemode);
                        gamemode_updated = 1;
                } else {
                        fputs(line, config_out);
                }
        }

        /* Add gamemode line if not found in original config */
        if (!gamemode_updated)
                fprintf(config_out, "gamemode0 %s\n", gamemode);

        /* Cleanup file handles */
        fclose(config_in);
        fclose(config_out);

        return __RETZ;
}

/*
 * Display server logs based on return status
 * Shows different log files for different server types
 */
void wd_display_server_logs(int ret)
{
        FILE *log_file;
        int ch;

        /* Select appropriate log file based on server type */
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

/*
 * Restore original SA-MP configuration from backup
 * Called during cleanup operations
 */
void restore_samp_config(void)
{
        char __sz_config[PATH_MAX * 2];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);
        remove(wcfg.wd_toml_config);
}

/*
 * Execute SA-MP server with specified gamemode
 * Handles configuration, signal setup, and execution
 */
void wd_run_samp_server(const char *gamemode, const char *server_bin)
{
        char command[256];
        int ret;

        /* Ensure gamemode has .amx extension */
        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
        char *__dot_ext = strrchr(gamemode, '.');
        if (!__dot_ext) gamemode = _gamemode;

        /* Verify gamemode file exists */
        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
                __main(0);
        }

        /* Update server configuration */
        if (update_server_config(gamemode) != 0)
                __main(0);

        /* Set executable permissions */
        CHMOD(server_bin, FILE_MODE);

        /* Setup signal handler for graceful shutdown */
        struct sigaction sa;

        sa.sa_handler = __handle_sigint;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

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
        
        /* Build execution command */
#ifdef _WIN32
        snprintf(command, sizeof(command), "%s", server_bin);
#else
        snprintf(command, sizeof(command), "./%s", server_bin);
#endif

        /* Execute server binary */
        ret = system(command);
        
        /* Handle execution results */
        if (ret == __RETZ) {
                if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                        sleep(2);
                        wd_display_server_logs(0);
                }
        } else {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
        }

        /* Restore original configuration */
        restore_samp_config();

        __main(0);
}

/*
 * Update OpenMP server configuration with new gamemode
 * Uses JSON parsing for OpenMP config format
 */
static int update_omp_config(const char *gamemode)
{
        FILE *config_in = NULL, *config_out = NULL;
        cJSON *root = NULL, *pawn = NULL, *main_scripts = NULL;
        char *__cJSON_Data = NULL, *__cJSON_Printed = NULL;
        struct stat st;
        char gamemode_buf[256];
        char _gamemode[256];
        int ret = -__RETN;

        /* Create backup filename */
        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);

        /* Platform-specific move command for backup */
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

        /* Get file statistics for memory allocation */
        if (stat(__sz_config, &st) != 0) {
                pr_error(stdout, "Failed to get file status");
                return -__RETN;
        }

        /* Open backup config for reading */
        config_in = fopen(__sz_config, "rb");
        if (!config_in) {
                pr_error(stdout, "Failed to open %s", __sz_config);
                return -__RETN;
        }

        /* Allocate memory for JSON data */
        __cJSON_Data = wd_malloc(st.st_size + 1);
        if (!__cJSON_Data) {
                pr_error(stdout, "Memory allocation failed");
                goto done;
        }

        /* Read entire file into memory */
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
        
        /* Parse JSON configuration */
        root = cJSON_Parse(__cJSON_Data);
        if (!root) {
                pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
                goto done;
        }

        /* Navigate to pawn section */
        pawn = cJSON_GetObjectItem(root, "pawn");
        if (!pawn) {
                pr_error(stdout, "Missing 'pawn' section in config");
                goto done;
        }

        /* Process gamemode filename */
        snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
        char *__dot_ext = strrchr(_gamemode, '.');
        if (__dot_ext) *__dot_ext = '\0';

        /* Update main_scripts array in JSON */
        cJSON_DeleteItemFromObject(pawn, "main_scripts");
        
        main_scripts = cJSON_CreateArray();
        snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", _gamemode);
        cJSON_AddItemToArray(main_scripts, cJSON_CreateString(gamemode_buf));
        cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

        /* Write updated configuration */
        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write %s", wcfg.wd_toml_config);
                goto done;
        }

        __cJSON_Printed = cJSON_Print(root);
        if (!__cJSON_Printed) {
                pr_error(stdout, "Failed to print JSON");
                goto done;
        }

        if (fputs(__cJSON_Printed, config_out) == EOF) {
                pr_error(stdout, "Failed to write to %s", wcfg.wd_toml_config);
                goto done;
        }

        ret = __RETZ;

/* Cleanup section with goto labels for error handling */
done:
        if (config_out)
                fclose(config_out);
        if (config_in)
                fclose(config_in);
        if (__cJSON_Printed)
                wd_free(__cJSON_Printed);
        if (root)
                cJSON_Delete(root);
        if (__cJSON_Data)
                wd_free(__cJSON_Data);

        return ret;
}

/*
 * Restore original OpenMP configuration from backup
 */
void restore_omp_config(void)
{
        char __sz_config[PATH_MAX];
        snprintf(__sz_config, sizeof(__sz_config), ".%s.bak", wcfg.wd_toml_config);

        remove(wcfg.wd_toml_config);
        rename(__sz_config, wcfg.wd_toml_config);
}

/*
 * Execute OpenMP server with specified gamemode
 * Similar to SA-MP version but with OpenMP-specific handling
 */
void wd_run_omp_server(const char *gamemode, const char *server_bin)
{
        char command[256];
        int ret;

        /* Ensure gamemode has .amx extension */
        char _gamemode[256];
        snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
        char *__dot_ext = strrchr(gamemode, '.');
        if (!__dot_ext) gamemode = _gamemode; 

        /* Verify gamemode file exists */
        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
                __main(0);
        }

        /* Update OpenMP configuration */
        if (update_omp_config(gamemode) != 0)
                return;

        /* Set executable permissions */
        CHMOD(server_bin, FILE_MODE);
        
        /* Setup signal handler */
        struct sigaction sa;

        sa.sa_handler = __handle_sigint;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

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

        /* Build execution command */
#ifdef _WIN32
        snprintf(command, sizeof(command), "%s", server_bin);
#else
        snprintf(command, sizeof(command), "./%s", server_bin);
#endif

        /* Execute server binary */
        ret = system(command);
        if (ret == __RETZ) {
                sleep(2);
                wd_display_server_logs(1);
        } else {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
        }

        /* Restore original configuration */
        restore_omp_config();

        __main(0);
}