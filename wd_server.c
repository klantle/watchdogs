#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <signal.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "wd_unit.h"
#include "wd_extra.h"
#include "wd_util.h"
#include "wd_depends.h"
#include "wd_server.h"

FILE *config_in = NULL, *config_out = NULL;
cJSON *sroot = NULL, *pawn = NULL, *main_scripts = NULL;
char *__cJSON_Data = NULL, *__cJSON_Printed = NULL;

int server_mode = 0;
void unit_handle_sigint(int sig) {
/* Ensure SA-MP sends the "--- Server Shutting Down." message.
* This is unnecessary in Watchdogs since the process is replaced
* by the executed binary from the start. If CTRL+C is pressed,
* it sends a SIGINT to the server process, stopping the server immediately.
*/
/* wd_stop_server_tasks(); */
        if (server_mode == 1) {
          restore_samp_config();
          restore_omp_config();
        }
        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);

        FILE *crashdetect_file = fopen(".wd_crashdetect", "w");
        if (crashdetect_file != NULL)
            fclose(crashdetect_file);

#ifdef __ANDROID__
#ifndef _DBG_PRINT
        wd_run_command("exit && ./watchdogs.tmux");
#else
        wd_run_command("exit && ./watchdogs.debug.tmux");
#endif
#elif defined(WD_LINUX)
#ifndef _DBG_PRINT
        wd_run_command("exit && ./watchdogs");
#else
        wd_run_command("exit && ./watchdogs.debug");
#endif
#elif defined(WD_WINDOWS)
#ifndef _DBG_PRINT
        wd_run_command("exit && watchdogs.win");
#else
        wd_run_command("exit && watchdogs.debug.win");
#endif
#endif
}

/**
 * Stops running server tasks based on the current server environment
 * Kills the appropriate server process (SA-MP or OpenMP)
 */
void wd_stop_server_tasks(void) {
        /* Check server environment and kill the corresponding process */
        if (wd_server_env() == 1)
          kill_process(wcfg.wd_ptr_samp);  /* Kill SA-MP server process */
        else if (wd_server_env() == 2)
          kill_process(wcfg.wd_ptr_omp);   /* Kill OpenMP server process */
}

/**
 * Updates server configuration file with new gamemode
 * Creates backup, modifies gamemode0 line, and handles file operations
 * 
 * @param gamemode The new gamemode to set in the configuration
 * @return WD_RETZ on success, -WD_RETN on failure
 */
static int update_server_config(const char *gamemode)
{
        FILE *config_in, *config_out;  /* File handles for input and output */
        char line[1024];               /* Buffer for reading configuration lines */
        int gamemode_updated = 0;      /* Flag indicating if gamemode was found and updated */

        /* Create backup filename by adding .bak prefix */
        char size_config[WD_PATH_MAX];
        wd_snprintf(size_config, sizeof(size_config), ".%s.bak", wcfg.wd_toml_config);

        /* Build move command for creating backup (platform-specific) */
        char size_mv[WD_MAX_PATH];
        if (is_native_windows())
            /* Windows rename command */
            wd_snprintf(size_mv, sizeof(size_mv),
                        "ren %s %s",
                        wcfg.wd_toml_config,
                        size_config);
        else
            /* Unix/Linux move command with force flag */
            wd_snprintf(size_mv, sizeof(size_mv),
                        "mv -f %s %s",
                        wcfg.wd_toml_config,
                        size_config);

        /* Execute backup creation command */
        if (wd_run_command(size_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -WD_RETN;
        }

        /* Open backup file for reading */
        config_in = fopen(size_config, "r");
        if (!config_in) {
                pr_error(stdout, "Failed to open backup config");
                return -WD_RETN;
        }
        /* Open original config file for writing (creates new file) */
        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write new config");
                fclose(config_in);
                return -WD_RETN;
        }

        /* Process gamemode parameter - remove file extension if present */
        char put_gamemode[256];
        wd_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *f_EXT = strrchr(put_gamemode, '.');  /* Find last dot in filename */
        if (f_EXT) *f_EXT = '\0';  /* Remove file extension by null-terminating */
        gamemode = put_gamemode;   /* Update gamemode pointer to processed string */

        /* Read and process each line from backup configuration */
        while (fgets(line, sizeof(line), config_in)) {
                /* Check if this line is the gamemode0 configuration line */
                if (strncmp(line, "gamemode0 ", 10) == 0) {
                        /* Replace with new gamemode value */
                        fprintf(config_out, "gamemode0 %s\n", gamemode);
                        gamemode_updated = 1;  /* Mark that we found and updated the gamemode */
                } else {
                        /* Copy line as-is without modification */
                        fputs(line, config_out);
                }
        }

        /* If gamemode0 line wasn't found, append it to the end */
        if (!gamemode_updated)
                fprintf(config_out, "gamemode0 %s\n", gamemode);

        /* Close both file handles */
        fclose(config_in);
        fclose(config_out);

        return WD_RETZ;  /* Return success */
}

/**
 * Displays server log files based on server type
 * Shows either SA-MP server_log.txt or OpenMP log.txt
 * 
 * @param ret Server type indicator (0 = SA-MP, non-zero = OpenMP)
 */
void wd_display_server_logs(int ret)
{
        FILE *log_file;  /* File handle for log file */
        int ch;          /* Character buffer for reading file */

        /* Determine which log file to open based on server type */
        if (!ret)
                log_file = fopen("server_log.txt", "r");  /* SA-MP server log */
        else
                log_file = fopen("log.txt", "r");         /* OpenMP server log */

        /* Silently return if log file cannot be opened */
        if (!log_file) {
                return;
        }

        /* Output log contents character by character to stdout */
        while ((ch = fgetc(log_file)) != EOF)
                putchar(ch);

        fclose(log_file);  /* Close log file */
}

/**
 * Checks for server crashes and prompts to install crash detection
 * Scans log files for runtime errors and checks if crashdetect is present
 */
void wd_server_crash_check(void) {
        int __has_error = 0;           /* Flag indicating runtime error was found */
        int server_crashdetect = 0;    /* Counter for crashdetect debug lines */

        FILE *proc_f;  /* File handle for log file processing */
        /* Open appropriate log file based on server mode */
        if (server_mode == 0)
          proc_f = fopen("server_log.txt", "r");  /* SA-MP log */
        else
          proc_f = fopen("log.txt", "r");         /* OpenMP log */

        /* Process log file if successfully opened */
        if (proc_f)
        {
            char line_buf[1024 + 1024];  /* Buffer for reading log lines */
            /* Read each line from the log file */
            while (fgets(line_buf, sizeof(line_buf), proc_f))
            {
                /* Check for runtime error indicators (case insensitive) */
                if (strfind(line_buf, "run time error") || strfind(line_buf, "Run time error"))
                    __has_error = 1;  /* Mark that a runtime error occurred */
                /* If error was found, look for crashdetect debug information */
                if (__has_error) {
                  if (strfind(line_buf, "[debug]") || strfind(line_buf, "crashdetect"))
                    ++server_crashdetect;  /* Count crashdetect related lines */
                }
            }
            fclose(proc_f);  /* Close log file */
        }
        /* If runtime error occurred but no crashdetect was found, offer to install it */
        if (__has_error == 1 && server_crashdetect < 1) {
              printf("INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? ");
              /* Prompt user for installation confirmation */
              char *confirm;
              confirm = readline("Y/n ");
              if (strfind(confirm, "y")) {
                  wd_free(confirm);  /* Free confirmation string */
                  /* Install crashdetect plugin from repository */
                  wd_install_depends("Y-Less/samp-plugin-crashdetect:latest");
              } else {
                  wd_free(confirm);  /* Free confirmation string */
                  /* Check for and remove crashdetect check file if it exists */
                  int _wd_crash_ck = path_acces(".wd_crashdetect");
                  if (_wd_crash_ck)
                    remove(".wd_crashdetect");
                  return;
              }
        }
}

/**
 * Placeholder function for restoring SA-MP configuration
 * Currently does nothing but provides framework for future implementation
 */
void restore_samp_config(void) {
        return;  /* No restoration logic implemented yet */
}

/**
 * Runs SA-MP server with specified gamemode
 * Handles configuration updates, process execution, and error recovery
 * 
 * @param gamemode The gamemode to run (without .amx extension)
 * @param server_bin The server binary executable filename
 */
void wd_run_samp_server(const char *gamemode, const char *server_bin)
{
        /* Debugging SA-MP Function */
#if defined (_DBG_PRINT)
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
               "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
        /* Validate configuration file type - must be TOML, not JSON */
        if (strfind(wcfg.wd_toml_config, ".json"))
                return;

        char command[256];  /* Buffer for building execution command */
        int ret;            /* Return status from command execution */

        /* Process gamemode parameter - ensure it has .amx extension */
        char put_gamemode[256];
        char *f_EXT = strrchr(gamemode, '.');  /* Find file extension */
        if (f_EXT) {
            /* Calculate length without extension and append .amx */
            size_t len = f_EXT - gamemode;
            wd_snprintf(put_gamemode,
                     sizeof(put_gamemode),
                     "%.*s.amx",
                     (int)len,
                     gamemode);
        } else {
            /* No extension found, simply append .amx */
            wd_snprintf(put_gamemode,
                     sizeof(put_gamemode),
                     "%s.amx",
                     gamemode);
        }

        gamemode = put_gamemode;  /* Update gamemode pointer */

        /* Search for gamemode file in current directory */
        wd_sef_fdir_reset();  /* Reset file directory search */
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                /* Gamemode file not found - display error and return to main menu */
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                start_chain(NULL);
        }

        /* Update server configuration with new gamemode */
        if (update_server_config(gamemode) != 0)
                start_chain(NULL);  /* Return to main menu on configuration failure */

        /* Set execute permissions on server binary (Unix/Linux systems) */
        CHMOD(server_bin, FILE_MODE);

        /* Set up signal handler for graceful shutdown on Ctrl+C */
        struct sigaction sa;

        sa.sa_handler = unit_handle_sigint;  /* Signal handler function */
        sigemptyset(&sa.sa_mask);            /* Initialize empty signal mask */
        sa.sa_flags = SA_RESTART;            /* Restart system calls if interrupted */

        /* Register SIGINT (Ctrl+C) handler */
        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        /* Variables for tracking server runtime and restart attempts */
        time_t start, end;
        double elapsed;

        int ret_serv = 0;  /* Restart attempt counter */

/* Label for restarting server after quick failure */
back_start:
        start = time(NULL);  /* Record start time */
        /* Build execution command (platform-specific) */
#ifdef WD_WINDOWS
        /* Windows: execute binary directly */
        wd_snprintf(command, sizeof(command), "%s", server_bin);
#else
        /* Unix/Linux: execute with ./ prefix */
        wd_snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);  /* Record end time (minimal time elapsed for command building) */

        /* Execute the server command */
        ret = wd_run_command(command);
        if (ret == WD_RETZ) {
                /* Server started successfully */
                if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                        /* On Linux, wait a moment then display server logs */
                        sleep(2);
                        wd_display_server_logs(0);  /* Display SA-MP logs */
                }
        } else {
                /* Server startup failed */
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);  /* Calculate how long server ran */
                /* If server failed quickly (within 5 seconds) and we haven't retried yet */
                if (elapsed <= 5.0 && ret_serv == 0) {
                    ret_serv = 1;  /* Mark that we've attempted restart */
                    printf("\ttry starting again..");
                    goto back_start;  /* Restart the server */
                }
        }
        /* Restore original configuration and check for crashes */
        restore_samp_config();
        wd_server_crash_check();

        /* Return to main menu after server stops */
        start_chain(NULL);
}

/**
 * Updates OpenMP server configuration with new gamemode using JSON format
 * Handles JSON parsing, modification, and file operations
 * 
 * @param gamemode The new gamemode to set in the configuration
 * @return WD_RETZ on success, -WD_RETN on failure
 */
static int update_omp_config(const char *gamemode)
{
        struct stat st;              /* File status structure */
        char gamemode_buf[256];      /* Buffer for processed gamemode name */
        char put_gamemode[256];      /* Buffer for gamemode processing */
        int ret = -WD_RETN;           /* Return status, default to error */

        /* Create backup filename */
        char size_config[WD_PATH_MAX];
        wd_snprintf(size_config, sizeof(size_config), ".%s.bak", wcfg.wd_toml_config);

        /* Build platform-specific move command for backup */
        char size_mv[WD_MAX_PATH];
        if (is_native_windows())
            wd_snprintf(size_mv, sizeof(size_mv),
                        "ren %s %s",
                        wcfg.wd_toml_config,
                        size_config);
        else
            wd_snprintf(size_mv, sizeof(size_mv),
                        "mv -f %s %s",
                        wcfg.wd_toml_config,
                        size_config);

        /* Execute backup creation */
        if (wd_run_command(size_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -WD_RETN;
        }

        /* Get file size for memory allocation */
        if (stat(size_config, &st) != 0) {
                pr_error(stdout, "Failed to get file status");
                return -WD_RETN;
        }

        /* Open backup file for reading */
        config_in = fopen(size_config, "rb");
        if (!config_in) {
                pr_error(stdout, "Failed to open %s", size_config);
                return -WD_RETN;
        }

        /* Allocate memory for entire file content plus null terminator */
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

        /* Null-terminate the file data for string processing */
        __cJSON_Data[st.st_size] = '\0';
        fclose(config_in);
        config_in = NULL;

        /* Parse JSON data from file */
        sroot = cJSON_Parse(__cJSON_Data);
        if (!sroot) {
                pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
                goto done;
        }

        /* Get the "pawn" section from JSON configuration */
        pawn = cJSON_GetObjectItem(sroot, "pawn");
        if (!pawn) {
                pr_error(stdout, "Missing 'pawn' section in config");
                goto done;
        }

        /* Process gamemode name - remove file extension */
        wd_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *f_EXT = strrchr(put_gamemode, '.');
        if (f_EXT) *f_EXT = '\0';

        /* Remove existing main_scripts array */
        cJSON_DeleteItemFromObject(pawn, "main_scripts");

        /* Create new main_scripts array with the specified gamemode */
        main_scripts = cJSON_CreateArray();
        wd_snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", put_gamemode);
        cJSON_AddItemToArray(main_scripts, cJSON_CreateString(gamemode_buf));
        cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

        /* Open original config file for writing updated JSON */
        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write %s", wcfg.wd_toml_config);
                goto done;
        }

        /* Convert JSON object back to string representation */
        __cJSON_Printed = cJSON_Print(sroot);
        if (!__cJSON_Printed) {
                pr_error(stdout, "Failed to print JSON");
                goto done;
        }

        /* Write JSON string to configuration file */
        if (fputs(__cJSON_Printed, config_out) == EOF) {
                pr_error(stdout, "Failed to write to %s", wcfg.wd_toml_config);
                goto done;
        }

        ret = WD_RETZ;  /* Mark operation as successful */

/* Cleanup section for error handling and resource management */
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

/**
 * Placeholder function for restoring OpenMP configuration
 * Currently does nothing but provides framework for future implementation
 */
void restore_omp_config(void) {
        return;  /* No restoration logic implemented yet */
}

/**
 * Runs OpenMP server with specified gamemode
 * Handles JSON configuration updates, process execution, and error recovery
 * 
 * @param gamemode The gamemode to run (without .amx extension)
 * @param server_bin The server binary executable filename
 */
void wd_run_omp_server(const char *gamemode, const char *server_bin)
{
        /* Debugging OMP Function */
#if defined (_DBG_PRINT)
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
               "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
        /* Validate configuration file type - must be JSON, not CFG */
        if (strfind(wcfg.wd_toml_config, ".cfg"))
                return;

        char command[256];  /* Buffer for execution command */
        int ret;            /* Return status from command execution */

        /* Process gamemode parameter - ensure it has .amx extension */
        char put_gamemode[256];
        char *f_EXT = strrchr(gamemode, '.');
        if (f_EXT) {
            size_t len = f_EXT - gamemode;
            wd_snprintf(put_gamemode,
                        sizeof(put_gamemode),
                        "%.*s.amx",
                        (int)len,
                        gamemode);
        } else {
            wd_snprintf(put_gamemode,
                        sizeof(put_gamemode),
                        "%s.amx",
                        gamemode);
        }

        gamemode = put_gamemode;  /* Update gamemode pointer */

        /* Search for gamemode file in current directory */
        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) != 1) {
                /* Gamemode file not found - display error and return to main menu */
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                start_chain(NULL);
        }

        /* Update OpenMP JSON configuration with new gamemode */
        if (update_omp_config(gamemode) != 0)
                return;  /* Return on configuration failure */

        /* Set execute permissions on server binary */
        CHMOD(server_bin, FILE_MODE);

        /* Set up signal handler for graceful shutdown */
        struct sigaction sa;

        sa.sa_handler = unit_handle_sigint;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        /* Variables for tracking server runtime and restart attempts */
        time_t start, end;
        double elapsed;

        int ret_serv = 0;  /* Restart attempt counter */

/* Label for restarting server after quick failure */
back_start:
        start = time(NULL);  /* Record start time */
        /* Build platform-specific execution command */
#ifdef WD_WINDOWS
        wd_snprintf(command, sizeof(command), "%s", server_bin);
#else
        wd_snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);  /* Record end time */

        /* Execute the server command */
        ret = wd_run_command(command);
        if (ret == WD_RETZ) {
                /* Server started successfully - display logs after brief delay */
                sleep(2);
                wd_display_server_logs(1);  /* Display OpenMP logs */
        } else {
                /* Server startup failed */
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);  /* Calculate runtime */
                /* If quick failure and no restart attempted yet, try again */
                if (elapsed <= 5.0 && ret_serv == 0) {
                    ret_serv = 1;  /* Mark restart attempt */
                    printf("\ttry starting again..");
                    goto back_start;  /* Restart server */
                }
        }
        /* Restore configuration and check for crashes */
        restore_omp_config();
        wd_server_crash_check();

        /* Return to main menu */
        start_chain(NULL);
}
