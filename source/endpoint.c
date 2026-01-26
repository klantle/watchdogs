/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

/* Project-specific headers organized by functionality */
#include "units.h"        /* Unit management functions */
#include "utils.h"        /* General-purpose utilities */
#include "crypto.h"       /* Cryptographic functions (CRC-32, etc.) */
#include "replicate.h"    /* Replication/backup functionality */
#include "compiler.h"     /* PAWN compiler integration */
#include "debug.h"        /* Debugging utilities */
#include "endpoint.h"     /* Network/endpoint communication */

/* Global variables for state management */

/* Signal handler flag - tracks SIGINT (Ctrl+C) state */
int                      sigint_handler = 0;

/* Buffer variables for various operations */
static char              sbuf[0x400];                  /* Line buffer for reading files */
static char              size_gamemode[DOG_PATH_MAX * 0x2]; /* Buffer for gamemode configuration */
static char              command[DOG_MAX_PATH + DOG_PATH_MAX * 2]; /* Command buffer for process execution */
static char              sputs[DOG_PATH_MAX + 0x1A];   /* Buffer for formatted gamemode name */
static char              size_config[DOG_PATH_MAX];    /* Backup file path buffer */

/* JSON/CJSON related pointers */
static char             *cJSON_Data = NULL;            /* Raw JSON data */
static char             *printed = NULL;               /* Formatted JSON output */
static char             *sampvoice_port = NULL;        /* SampVoice plugin port string */

/* Rate limiting and state tracking counters */
static int               rate_sampvoice_server = 0;    /* SampVoice rate limiter */
static int               rate_problem_stat = 0;        /* Problem detection rate limiter */
static int               server_crashdetect = 0;       /* Crash detection counter */
static int               server_rcon_pass = 0;         /* RCON password error counter */

/* File handles for configuration processing */
static FILE             *proc_conf_in = NULL;          /* Input configuration file */
static FILE             *proc_conf_out = NULL;         /* Output configuration file */

/* cJSON objects for JSON configuration parsing */
static cJSON            *root = NULL;                  /* Root JSON object */
static cJSON            *pawn = NULL;                  /* "pawn" section of JSON */
static cJSON            *msj = NULL;                   /* "msj" (gamemode script) array */

/*
 * try_cleanup_server:
 *     Clean up server state and resources to ensure clean restart.
 *     Resets all global variables, stops server tasks, restores config.
 */

void
try_cleanup_server(void)
{
        /* Clear all string buffers to prevent stale data */
        memset(sbuf, 0, sizeof(sbuf));
        memset(size_gamemode, 0, sizeof(size_gamemode));
        memset(command, 0, sizeof(command));
        memset(sputs, 0, sizeof(sputs));
        memset(size_config, 0, sizeof(size_config));

        /* Decrement rate-limiting counters if active */
        if (rate_sampvoice_server)
            --rate_sampvoice_server;

        if (rate_problem_stat)
            --rate_problem_stat;

        if (server_crashdetect)
            --server_crashdetect;

        if (server_rcon_pass)
            --server_rcon_pass;

        /* Reset all pointers to NULL to prevent dangling references */
        cJSON_Data = NULL;
        printed = NULL;
        sampvoice_port = NULL;
        proc_conf_in = NULL;
        proc_conf_out = NULL;
        root = NULL;
        pawn = NULL;
        msj = NULL;

        /* Set signal handler flag to indicate cleanup is in progress */
        sigint_handler = 1;

        /* Stop any running server tasks and restore config to default */
        dog_stop_server_tasks();
        restore_server_config();

        return;
}

/* Forward declaration for crash checking function */
void dog_server_crash_check(void);

/*
 * unit_sigint_handler:
 *     Handle SIGINT (Ctrl+C) signal gracefully.
 *     Cleans up server state, creates crash detection marker.
 *     Parameters:
 *         sig - Signal number (should be SIGINT)
 */

void
unit_sigint_handler(int sig __UNUSED__)
{
        /* Clean up server state first */
        try_cleanup_server();

        /* Record the time when signal was received */
        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);
}

/*
 * dog_stop_server_tasks:
 *     Stop running server process.
 */

void
dog_stop_server_tasks(void)
{
        int ret = 0;
        ret = dog_kill_process(dogconfig.dog_toml_server_binary);
        if (ret) {
            /* retrying */
            dog_kill_process(dogconfig.dog_toml_server_binary);
        }
}

/*
 * update_samp_config:
 *     Update SA-MP server configuration with new gamemode.
 *     Creates backup of existing config, opens backup for reading,
 *     writes new config with updated gamemode0 buffer, preserves all
 *     other configuration lines.
 *     Parameters:
 *         g - New gamemode name (without .amx extension)
 *     Returns:
 *         1 on success, -1 on failure
 */

static int
update_samp_config(const char *g)
{
        /* Create backup file path: .watchdogs/<config>.bak */
        snprintf(size_config, sizeof(size_config),
            ".watchdogs/%s.bak", dogconfig.dog_toml_server_config);

        /* Remove existing backup if it exists */
        if (path_access(size_config))
                remove(size_config);

        /* Platform-specific file moving for backup creation */
    #ifdef DOG_WINDOWS
        /* Windows: Use MoveFileExA with flags for atomic operation */
        if (!MoveFileExA(
                dogconfig.dog_toml_server_config,
                size_config,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            pr_error(stdout, "Failed to create backup file");
            minimal_debugging();
            return -1;
        }
    #else
        /* Unix/Linux: Use fork+exec for mv command */
        pid_t process_id = fork();
        if (process_id == 0) {
            execlp("mv", "mv", "-f",
                dogconfig.dog_toml_server_config,
                size_config,
                NULL);
            _exit(127);
        }
        if (process_id < 0 || waitpid(process_id, NULL, 0) < 0) {
            pr_error(stdout, "Failed to create backup file");
            minimal_debugging();
            return -1;
        }
    #endif

        /* Open backup file for reading with platform-specific flags */
    #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
    #else
        /* Unix: Use O_NOFOLLOW to prevent symlink attacks */
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
    #endif

        if (fd < 0) {
                pr_error(stdout, "cannot open backup");
                minimal_debugging();
                return -1;
        }

        /* Convert file descriptor to FILE* for buffer-based reading */
        proc_conf_in = fdopen(fd, "r");
        if (!proc_conf_in) {
                close(fd);
                return -1;
        }

        /* Open destination config file for writing */
        proc_conf_out = fopen(dogconfig.dog_toml_server_config, "w+");
        if (!proc_conf_out) {
                pr_error(stdout, "Failed to write new config");
                minimal_debugging();
                fclose(proc_conf_in);
                return -1;
        }

        /* Prepare gamemode name by removing extension if present */
        snprintf(sputs, sizeof(sputs), "%s", g);
        char *e = strrchr(sputs, '.');
        if (e) *e = '\0';  /* Remove file extension */
        g = sputs;

        /* Process config file buffer by buffer */
        while (fgets(sbuf, sizeof(sbuf), proc_conf_in)) {
            /* Look for gamemode0 buffer to replace */
            if (strfind(sbuf, "gamemode0", true)) {
                /* Create new gamemode0 buffer with updated gamemode */
                snprintf(size_gamemode, sizeof(size_gamemode),
                    "gamemode0 %s\n", sputs);
                fputs(size_gamemode, proc_conf_out);
                continue;
            }
            /* Copy all other lines unchanged */
            fputs(sbuf, proc_conf_out);
        }

        /* Cleanup file handles */
        fclose(proc_conf_in);
        fclose(proc_conf_out);

        return 1;  /* Success */
}

/*
 * restore_server_config:
 *     Restore server configuration from backup file.
 *     Checks if backup exists, prompts user for confirmation,
 *     replaces current config with backup, using platform-specific
 *     file operations.
 */

void
restore_server_config(void)
{
        snprintf(size_config, sizeof(size_config),
            ".watchdogs/%s.bak", dogconfig.dog_toml_server_config);

        /* Check if backup file exists */
        if (path_exists(size_config) == 0)
            goto restore_done;  /* No backup, nothing to restore */

        /* Prompt user for confirmation */
        printf(DOG_COL_GREEN "warning: " DOG_COL_DEFAULT
            "Continue to restore %s -> %s? y/n",
            size_config, dogconfig.dog_toml_server_config);

        char *restore_confirm = readline(" ");
        if (restore_confirm[0] != '\0' || strfind(restore_confirm, "Y", true)) {
                ;  /* User confirmed, continue with restore */
        } else {
                dog_free(restore_confirm);
                return;
        }

        dog_free(restore_confirm);

        /* Check if backup is accessible */
        if (path_access(size_config) == 0)
                goto restore_done;

        /* Platform-specific file deletion for current config */
    #ifdef DOG_WINDOWS
            DWORD attr = GetFileAttributesA(dogconfig.dog_toml_server_config);
            if (attr != INVALID_FILE_ATTRIBUTES &&
                !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                DeleteFileA(dogconfig.dog_toml_server_config);
            }
    #else
            unlink(dogconfig.dog_toml_server_config);
    #endif

        /* Platform-specific file moving to restore from backup */
    #ifdef DOG_WINDOWS
            if (!MoveFileExA(
                    size_config,
                    dogconfig.dog_toml_server_config,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                return;
            }
    #else
            if (rename(size_config, dogconfig.dog_toml_server_config) != 0) {
                return;
            }
    #endif

restore_done:
        return;
}

/*
 * dog_exec_samp_server:
 *     Execute SA-MP server with specified gamemode.
 *     Validates configuration and gamemode file, updates server
 *     configuration, sets up signal handlers, executes server binary
 *     with platform-specific methods, handles retry logic on failure.
 *     Parameters:
 *         g - Gamemode name
 *         server_bin - Server binary executable name
 */

void
dog_exec_samp_server(char *g, const char *server_bin)
{
        minimal_debugging();  /* Log minimal debug info */

        /* Validate configuration file type */
        if (strfind(dogconfig.dog_toml_server_config, ".json", true))
                return;  /* JSON configs not supported for SA-MP */

        int ret = -1;  /* Process execution result */

        /* Prepare gamemode name with .amx extension */
        char *e = strrchr(g, '.');
        if (e) {
                size_t len = e - g;
                snprintf(sputs, sizeof(sputs), "%.*s.amx", (int)len, g);
        } else {
                snprintf(sputs, sizeof(sputs), "%s.amx", g);
        }
        g = sputs;

        char *s_server_bin = strdup(server_bin);
        if (condition_check(s_server_bin) == 1) {
            dog_free(s_server_bin);
            return;
        }
        dog_free(s_server_bin);

        /* Restore system error handling */
        dog_sef_path_revert();

        /* Verify gamemode file exists */
        if (path_exists(g) == 0) {
                printf("Cannot locate g: ");
                pr_color(stdout, DOG_COL_CYAN, "%s\n", g);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);  /* Exit if gamemode not found */
        }

        /* Update server configuration with new gamemode */
        int ret_c = update_samp_config(g);
        if (ret_c == 0 || ret_c == -1)
                return;  /* Configuration update failed */

        /* Set up SIGINT handler for graceful shutdown */
        struct sigaction sa;
        sa.sa_handler = unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        /* Timing variables for performance tracking and retry logic */
        time_t start = 0, end = 0;
        double elp;
        bool rty = false;  /* Retry flag */
        int _access = -1;

    back_start:  /* Retry label for failed startup attempts */
        start = time(NULL);
        printf(DOG_COL_BLUE "");  /* Set output color */

        /* Platform-specific process execution */
    #ifdef DOG_WINDOWS
        /* Windows: CreateProcess API for process creation */
        STARTUPINFOA        _STARTUPINFO;
        PROCESS_INFORMATION _PROCESS_INFO;

        ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
        ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

        _STARTUPINFO.cb = sizeof(_STARTUPINFO);
        _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

        _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
        _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

        snprintf(command, sizeof(command), "%s%s", _PATH_STR_EXEC, server_bin);

        if (!CreateProcessA(
            NULL,command,NULL,NULL,TRUE,0,NULL,NULL,
            &_STARTUPINFO,
            &_PROCESS_INFO))
        {
            ret = 1;  /* Process creation failed */
        }
        else
        {
            WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
            CloseHandle(_PROCESS_INFO.hProcess);
            CloseHandle(_PROCESS_INFO.hThread);
            ret = 0;  /* Process executed successfully */
        }
    #else
        /* Unix/Linux: fork+exec with pipe redirection for output capture */
        pid_t process_id;
        __set_default_access(server_bin);  /* Ensure binary is executable */

        snprintf(command, sizeof(command), "%s/%s", dog_procure_pwd(), server_bin);

        /* Create pipes for stdout and stderr redirection */
        int stdout_pipe[2];
        int stderr_pipe[2];

        if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
            perror("pipe");
            return;
        }

        process_id = fork();
        if (process_id == 0) {  /* Child process */
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);

            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stderr_pipe[1], STDERR_FILENO);

            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            execl("/bin/sh", "sh", "-c", command, (char *)NULL);
            _exit(127);  /* execl failed */
        } else if (process_id > 0) {  /* Parent process */
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            int stdout_fd;
            int stderr_fd;
            int max_fd;
            char buffer[1024];
            ssize_t br;
            
            stdout_fd = stdout_pipe[0];
            stderr_fd = stderr_pipe[0];
            max_fd = (stdout_fd > stderr_fd ? stdout_fd : stderr_fd) + 1;
            
            fd_set readfds;

            while (1) {
                FD_ZERO(&readfds);
                if (stdout_fd >= 0) FD_SET(stdout_fd, &readfds);
                if (stderr_fd >= 0) FD_SET(stderr_fd, &readfds);

                if (select(max_fd, &readfds, NULL, NULL, NULL) < 0) {
                    perror("select failed");
                    break;
                }

                if (stdout_fd >= 0 && FD_ISSET(stdout_fd, &readfds)) {
                    br = read(stdout_fd, buffer, sizeof(buffer)-1);
                    if (br <= 0) stdout_fd = -1;
                    else {
                        buffer[br] = '\0';
                        printf("%s", buffer);
                    }
                }

                if (stderr_fd >= 0 && FD_ISSET(stderr_fd, &readfds)) {
                    br = read(stderr_fd, buffer, sizeof(buffer)-1);
                    if (br <= 0) stderr_fd = -1;
                    else {
                        buffer[br] = '\0';
                        fprintf(stderr, "%s", buffer);
                    }
                }

                if (stdout_fd < 0 && stderr_fd < 0) break;
            }

            close(stdout_pipe[0]);
            close(stderr_pipe[0]);

            /* Wait for child process to complete */
            int status;
            waitpid(process_id, &status, 0);

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code == 0) {
                    ret = 0;  /* Successful execution */
                } else {
                    ret = -1; /* Non-zero exit code */
                }
            } else {
                ret = -1;  /* Child terminated abnormally */
            }
        }
    #endif

        /* Handle execution result */
        if (ret == 0) {
                end = time(NULL);  /* Success - record end time */
        } else {
                printf(DOG_COL_DEFAULT "\n");
                pr_color(stdout, DOG_COL_RED, "Server startup failed!\n");

                /* Calculate elapsed time for retry logic */
                elp = difftime(end, start);
                if (elp <= 4.1 && rty == false) {
                    rty = true;  /* Mark that we're retrying */
                    printf("\ttry starting again..");

                    /* Clean up log files before retry */
                    _access = path_access(dogconfig.dog_toml_server_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_server_logs);

                    _access = path_access(dogconfig.dog_toml_server_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_server_logs);

                    end = time(NULL);
                    goto back_start;  /* Retry startup */
                }
        }
        printf(DOG_COL_DEFAULT "\n");

        end = time(NULL);

        /* Trigger SIGINT handler if not already triggered */
        if (sigint_handler == 0)
                raise(SIGINT);

        return;
}

/*
 * update_omp_config:
 *     Update open.mp JSON configuration with new gamemode.
 *     Creates backup of existing JSON config, parses JSON structure,
 *     updates "pawn"/"msj" array with new gamemode, writes updated
 *     JSON back to config file.
 *     Parameters:
 *         g - New gamemode name (without .amx extension)
 *     Returns:
 *         0 on success, -1 on failure
 */

static int
update_omp_config(const char *g)
{
        struct stat st;  /* File statistics */
        char   gf[DOG_PATH_MAX + 0x1A];  /* Buffer for gamemode name */
        int    ret = -1;

        /* Create backup file path */
        snprintf(size_config, sizeof(size_config),
            ".watchdogs/%s.bak", dogconfig.dog_toml_server_config);

        /* Remove existing backup */
        if (path_access(size_config))
                remove(size_config);

        /* Platform-specific backup creation */
    #ifdef DOG_WINDOWS
            if (!MoveFileExA(
                    dogconfig.dog_toml_server_config,
                    size_config,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                pr_error(stdout, "Failed to create backup file");
                minimal_debugging();
                return -1;
            }
    #else
            pid_t process_id = fork();
            if (process_id == 0) {
                execlp("mv", "mv", "-f",
                    dogconfig.dog_toml_server_config,
                    size_config,
                    NULL);
                _exit(127);
            }
            if (process_id < 0 || waitpid(process_id, NULL, 0) < 0) {
                pr_error(stdout, "Failed to create backup file");
                minimal_debugging();
                return -1;
            }
    #endif

        /* Open backup file with platform-specific security flags */
    #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
    #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
    #endif

        if (fd < 0) {
                pr_error(stdout, "Failed to open %s", size_config);
                minimal_debugging();
                goto endpoint_end;
        }

        /* Get file size for memory allocation */
        if (fstat(fd, &st) != 0) {
                pr_error(stdout, "Failed to stat %s", size_config);
                minimal_debugging();
                close(fd);
                goto endpoint_end;
        }

        /* Convert fd to FILE* for reading */
        proc_conf_in = fdopen(fd, "rb");
        if (!proc_conf_in) {
                pr_error(stdout, "fdopen failed");
                minimal_debugging();
                close(fd);
                goto endpoint_end;
        }

        /* Allocate memory for entire JSON file */
        cJSON_Data = dog_malloc(st.st_size + 1);
        if (!cJSON_Data) {
                goto endpoint_kill;  /* Memory allocation failed */
        }

        /* Read entire file into memory */
        size_t br;
        br = fread(cJSON_Data, 1, st.st_size, proc_conf_in);
        if (br != (size_t)st.st_size) {
                pr_error(stdout,
                    "Incomplete file read (%zu of %ld bytes)",
                    br, st.st_size);
                minimal_debugging();
                goto endpoint_cleanup;
        }

        cJSON_Data[st.st_size] = '\0';  /* Null-terminate string */
        fclose(proc_conf_in);
        proc_conf_in = NULL;

        /* Parse JSON data */
        root = cJSON_Parse(cJSON_Data);
        if (!root) {
                pr_error(stdout,
                    "JSON parse error: %s", cJSON_GetErrorPtr());
                minimal_debugging();
                goto endpoint_end;
        }

        /* Get "pawn" section from JSON */
        pawn = cJSON_GetObjectItem(root, "pawn");
        if (!pawn) {
                pr_error(stdout,
                    "Missing 'pawn' section in config!");
                minimal_debugging();
                goto endpoint_cleanup;
        }

        /* Prepare gamemode name (remove extension) */
        snprintf(sputs, sizeof(sputs), "%s", g);
        char *e = strrchr(sputs, '.');
        if (e) *e = '\0';

        /* Remove existing "msj" array and create new one */
        cJSON_DeleteItemFromObject(pawn, "msj");

        msj = cJSON_CreateArray();
        snprintf(gf, sizeof(gf), "%s", sputs);
        cJSON_AddItemToArray(msj, cJSON_CreateString(gf));
        cJSON_AddItemToObject(pawn, "msj", msj);

        /* Open output file for writing updated JSON */
        proc_conf_out = fopen(dogconfig.dog_toml_server_config, "w");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write %s", dogconfig.dog_toml_server_config);
                minimal_debugging();
                goto endpoint_end;
        }

        /* Convert JSON object to formatted string */
        printed = cJSON_Print(root);
        if (!printed) {
                pr_error(stdout,
                    "Failed to print JSON");
                minimal_debugging();
                goto endpoint_end;
        }

        /* Write JSON to output file */
        if (fputs(printed, proc_conf_out) == EOF) {
                pr_error(stdout,
                    "Failed to write to %s", dogconfig.dog_toml_server_config);
                minimal_debugging();
                goto endpoint_end;
        }

        ret = 0;  /* Success */

    endpoint_end:
        ;  /* Label for cleanup jump */

    endpoint_cleanup:
        /* Cleanup resources in reverse allocation order */
        if (proc_conf_out)
                fclose(proc_conf_out);
        if (proc_conf_in)
                fclose(proc_conf_in);
        if (printed) {
                free(printed);
                printed = NULL;
        }
        if (root) {
                cJSON_Delete(root);
                root = NULL;
        }
        if (cJSON_Data) {
                free(cJSON_Data);
                cJSON_Data = NULL;
        }

    endpoint_kill:
        return ret;
}

/*
 * restore_omp_config:
 *     Wrapper function for open.mp config restoration.
 */

void
restore_omp_config(void)
{
        restore_server_config();
}

/*
 * dog_exec_omp_server:
 *     Execute open.mp server with specified gamemode.
 *     Similar to SA-MP version but for open.mp JSON config format.
 *     Parameters:
 *         g - Gamemode name
 *         server_bin - Server binary executable name
 */

void
dog_exec_omp_server(char *g, const char *server_bin)
{
        minimal_debugging();

        /* Validate config file type */
        if (strfind(dogconfig.dog_toml_server_config, ".cfg", true))
                return;  /* .cfg files not supported for open.mp */

        int ret = -1;

        /* Prepare gamemode name with .amx extension */
        char *e = strrchr(g, '.');
        if (e) {
            size_t len = e - g;
            snprintf(sputs, sizeof(sputs), "%.*s.amx", (int)len, g);
        } else {
            snprintf(sputs, sizeof(sputs), "%s.amx", g);
        }
        g = sputs;

        char *s_server_bin = strdup(server_bin);
        if (condition_check(s_server_bin) == 1) {
            dog_free(s_server_bin);
            return;
        }
        dog_free(s_server_bin);

        dog_sef_path_revert();

        /* Verify gamemode exists */
        if (path_exists(g) == 0) {
                printf("Cannot locate g: ");
                pr_color(stdout, DOG_COL_CYAN, "%s\n", g);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);
        }

        /* Update open.mp JSON config */
        int ret_c = update_omp_config(g);
        if (ret_c == 0 || ret_c == -1)
                return;

        /* Set up signal handler */
        struct sigaction sa;
        sa.sa_handler = unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        /* Timing and retry variables */
        time_t start = 0, end = 0;
        double elp;
        bool rty = false;
        int _access = -1;

    back_start:  /* Retry label */
        start = time(NULL);
        printf(DOG_COL_BLUE "");

        /* Platform-specific process execution (same as SA-MP version) */
    #ifdef DOG_WINDOWS
            STARTUPINFOA        _STARTUPINFO;
            PROCESS_INFORMATION _PROCESS_INFO;

            ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
            ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

            _STARTUPINFO.cb = sizeof(_STARTUPINFO);
            _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

            _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
            _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

            snprintf(command, sizeof(command),
                "%s%s", _PATH_STR_EXEC, server_bin);

            if (!CreateProcessA(
                NULL,command,NULL,NULL,TRUE,0,NULL,NULL,
                &_STARTUPINFO,
                &_PROCESS_INFO))
            {
                ret = 1;
            }
            else
            {
                WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
                CloseHandle(_PROCESS_INFO.hProcess);
                CloseHandle(_PROCESS_INFO.hThread);
                ret = 0;
            }
    #else
            pid_t process_id;

            __set_default_access(server_bin);

            snprintf(command, sizeof(command), "%s/%s",
                dog_procure_pwd(), server_bin);

            int stdout_pipe[2];
            int stderr_pipe[2];

            if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
                perror("pipe");
                return;
            }

            process_id = fork();
            if (process_id == 0) {
                close(stdout_pipe[0]);
                close(stderr_pipe[0]);

                dup2(stdout_pipe[1], STDOUT_FILENO);
                dup2(stderr_pipe[1], STDERR_FILENO);

                close(stdout_pipe[1]);
                close(stderr_pipe[1]);

                char *argv[] = {
                    "/bin/sh",
                    "-c",
                    (char *)command,
                    NULL
                };
                execl("/bin/sh", "sh", "-c", command, (char *)NULL);
                _exit(127);
            } else if (process_id > 0) {
                close(stdout_pipe[1]);
                close(stderr_pipe[1]);

                int stdout_fd;
                int stderr_fd;
                int max_fd;
                char buffer[1024];
                ssize_t br;

                stdout_fd = stdout_pipe[0];
                stderr_fd = stderr_pipe[0];
                max_fd = (stdout_fd > stderr_fd ? stdout_fd : stderr_fd) + 1;

                fd_set readfds;

                while (1) {
                    FD_ZERO(&readfds);
                    if (stdout_fd >= 0) FD_SET(stdout_fd, &readfds);
                    if (stderr_fd >= 0) FD_SET(stderr_fd, &readfds);

                    if (select(max_fd, &readfds, NULL, NULL, NULL) < 0) {
                        perror("select failed");
                        break;
                    }

                    if (stdout_fd >= 0 && FD_ISSET(stdout_fd, &readfds)) {
                        br = read(stdout_fd, buffer, sizeof(buffer)-1);
                        if (br <= 0) stdout_fd = -1;
                        else {
                            buffer[br] = '\0';
                            printf("%s", buffer);
                        }
                    }

                    if (stderr_fd >= 0 && FD_ISSET(stderr_fd, &readfds)) {
                        br = read(stderr_fd, buffer, sizeof(buffer)-1);
                        if (br <= 0) stderr_fd = -1;
                        else {
                            buffer[br] = '\0';
                            fprintf(stderr, "%s", buffer);
                        }
                    }

                    if (stdout_fd < 0 && stderr_fd < 0) break;
                }

                close(stdout_pipe[0]);
                close(stderr_pipe[0]);

                int status;
                waitpid(process_id, &status, 0);

                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    if (exit_code == 0) {
                        ret = 0;
                    } else {
                        ret = -1;
                    }
                } else {
                    ret = -1;
                }
            }
    #endif

        /* Handle execution result */
        if (ret != 0) {
                printf(DOG_COL_DEFAULT "\n");
                pr_color(stdout, DOG_COL_RED, "Server startup failed!\n");

                elp = difftime(end, start);
                if (elp <= 4.1 && rty == false) {
                    rty = true;
                    printf("\ttry starting again..");
                    _access = path_access(dogconfig.dog_toml_server_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_server_logs);
                    _access = path_access(dogconfig.dog_toml_server_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_server_logs);
                    end = time(NULL);
                    goto back_start;
                }
        }
        printf(DOG_COL_DEFAULT "\n");

        end = time(NULL);
        if (sigint_handler)
                raise(SIGINT);

        return;
}

/*
 * dog_server_crash_check:
 *     Analyze server logs for errors, warnings, and crash patterns.
 *     Opens server log file, scans buffer by buffer for known error
 *     patterns, provides diagnostic information and auto-fix
 *     suggestions, handles specific issues like RCON password, plugin
 *     conflicts.
 */

void
dog_server_crash_check(void)
{
        int n;  /* snprintf return value */
        size_t  size_l;  /* Output size tracker */
        FILE *this_proc_file = NULL;  /* Log file handle */
        char  out[DOG_MAX_PATH + 26]; /* Output buffer */
        char  buf[DOG_MAX_PATH];  /* Line buffer for log reading */

        /* Open appropriate log file based on server environment */
        if (fetch_server_env() == 1)  /* SA-MP */
            this_proc_file = fopen(dogconfig.dog_toml_server_logs, "rb");
        else  /* open.mp */
            this_proc_file = fopen(dogconfig.dog_toml_server_logs, "rb");

        if (this_proc_file == NULL) {
            pr_error(stdout, "log file not found!.");
            minimal_debugging();
            return;
        }

        /* Check for crashinfo.txt file (crashdetect plugin output) */
        if (path_exists("crashinfo.txt") != 0) {
            pr_info(stdout, "crashinfo.txt detected..");
            char *confirm = readline("-> show? ");
            if (confirm && (confirm[0] == '\0' || confirm[0] == 'Y' || confirm[0] == 'y')) {
                dog_printfile("crashinfo.txt");  /* Display crash info */
            }
            dog_free(confirm);
        }

        /* Print separator for log analysis output */
        n = snprintf(out, sizeof(out),
            "====================================================================\n");
        size_l = (n < 0) ? 0 : (size_t)n;
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        /* Process log file buffer by buffer */
        while (fgets(buf, sizeof(buf), this_proc_file)) {
            /* Pattern 1: Filterscript loading errors */
            if (strfind(buf, "Unable to load filterscript", true)) {
                n = snprintf(out, sizeof(out),
                    "@ Unable to load filterscript detected - Please recompile our filterscripts.\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 2: Invalid index/entry point errors */
            if (strfind(buf, "Invalid index parameter (bad entry point)", true)) {
                n = snprintf(out, sizeof(out),
                    "@ Invalid index parameter (bad entry point) detected - You're forget " DOG_COL_CYAN "'main'" DOG_COL_DEFAULT "?\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 3: Runtime errors (most common crash cause) */
            if (strfind(buf, "run time error", true) || strfind(buf, "Run time error", true)) {
                rate_problem_stat = 1;  /* Flag that we found a problem */
                n = snprintf(out, sizeof(out), "@ Runtime error detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);

                /* Specific runtime error subtypes */
                if (strfind(buf, "division by zero", true)) {
                    n = snprintf(out, sizeof(out), "@ Division by zero error found\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "invalid index", true)) {
                    n = snprintf(out, sizeof(out), "@ Invalid index error found\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 4: Outdated include files requiring recompilation */
            if (strfind(buf, "The script might need to be recompiled with the latest include file.", true)) {
                n = snprintf(out, sizeof(out), "@ Needed for recompiled\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);

                /* Offer auto-fix: recompile script */
                char *recompiled = readline("Recompiled script now? (Auto-fix)");
                if (recompiled && (recompiled[0] == '\0' || !strcmp(recompiled, "Y") || !strcmp(recompiled, "y"))) {
                    dog_free(recompiled);
                    printf(DOG_COL_BCYAN "Please input the pawn file\n\t* (enter for %s - input E/e to exit):" DOG_COL_DEFAULT, dogconfig.dog_toml_proj_input);
                    char *gamemode_compile = readline(" ");
                    if (gamemode_compile && strlen(gamemode_compile) < 1) {
                        /* Use default project input */
                        const char *args[] = { NULL, ".", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        dog_exec_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
                        dog_free(gamemode_compile);
                    } else if (gamemode_compile) {
                        /* Use specified file */
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        dog_exec_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
                        dog_free(gamemode_compile);
                    }
                } else {
                    dog_free(recompiled);
                }
            }

            /* Pattern 5: Filesystem errors (common in WSL environments) */
            if (strfind(buf, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error", true)) {
                n = snprintf(out, sizeof(out), "@ Filesystem C++ Error Detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                n = snprintf(out, sizeof(out),
                    "\tAre you currently using the WSL ecosystem?\n"
                    "\tYou need to move the open.mp server folder from the /mnt area (your Windows directory) to \"~\" (your WSL HOME).\n"
                    "\tThis is because open.mp C++ filesystem cannot properly read directories inside the /mnt area,\n"
                    "\twhich isn't part of the directory model targeted by the Linux build.\n"
                    "\t* You must run it outside the /mnt area.\n");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 6: SampVoice plugin port detection */
            if (strfind(buf, "voice server running on port", true)) {
                int _sampvoice_port;
                if (scanf("%*[^v]voice server running on port %d", &_sampvoice_port) != 1)
                    continue;
                ++rate_sampvoice_server;
                dog_free(sampvoice_port);
                sampvoice_port = (char *)dog_malloc(16);
                if (sampvoice_port) {
                    snprintf(sampvoice_port, 16, "%d", _sampvoice_port);
                }
            }

            /* Pattern 7: Missing gamemode files */
            if (strfind(buf, "I couldn't load any gamemode scripts.", true)) {
                n = snprintf(out, sizeof(out), "@ Can't found gamemode detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                n = snprintf(out, sizeof(out),
                    "\tYou need to ensure that the name specified "
                    "in the configuration file matches the one in the gamemodes/ folder,\n"
                    "\tand that the .amx file exists. For example, "
                    "if server.cfg contains " DOG_COL_CYAN "gamemode0" DOG_COL_DEFAULT" main 1 or config.json" DOG_COL_CYAN " pawn.main_scripts [\"main 1\"].\n"
                    DOG_COL_DEFAULT
                    "\tthen main.amx must be present in the gamemodes/ directory\n");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 8: Memory address references (potential crashes) */
            if (strfind(buf, "0x", true)) {
                n = snprintf(out, sizeof(out), "@ Hexadecimal address found\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "address", true) || strfind(buf, "Address", true)) {
                n = snprintf(out, sizeof(out), "@ Memory address reference found\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 9: Crashdetect plugin output (detailed crash info) */
            if (rate_problem_stat) {
                if (strfind(buf, "[debug]", true) || strfind(buf, "crashdetect", true)) {
                    ++server_crashdetect;
                    n = snprintf(out, sizeof(out), "@ Crashdetect: Crashdetect debug information found\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);

                    /* Crashdetect-specific patterns */
                    if (strfind(buf, "AMX backtrace", true)) {
                        n = snprintf(out, sizeof(out), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        size_l = (n < 0) ? 0 : (size_t)n;
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "native stack trace", true)) {
                        n = snprintf(out, sizeof(out), "@ Crashdetect: Native stack trace detected\n\t");
                        size_l = (n < 0) ? 0 : (size_t)n;
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "heap", true)) {
                        n = snprintf(out, sizeof(out), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        size_l = (n < 0) ? 0 : (size_t)n;
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "[debug]", true)) {
                        n = snprintf(out, sizeof(out), "@ Crashdetect: Debug Detected\n\t");
                        size_l = (n < 0) ? 0 : (size_t)n;
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }

                    /* Native backtrace with plugin conflict detection */
                    if (strfind(buf, "Native backtrace", true)) {
                        n = snprintf(out, sizeof(out), "@ Crashdetect: Native backtrace detected\n\t");
                        size_l = (n < 0) ? 0 : (size_t)n;
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);

                        /* Detect SampVoice and Pawn.Raknet plugin conflicts */
                        if (strfind(buf, "sampvoice", true)) {
                            if(strfind(buf, "pawnraknet", true)) {
                                n = snprintf(out, sizeof(out), "@ Crashdetect: Crash potent detected\n\t");
                                size_l = (n < 0) ? 0 : (size_t)n;
                                fwrite(out, 1, size_l, stdout);
                                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                                fflush(stdout);
                                n = snprintf(out, sizeof(out),
                                    "\tWe have detected a crash and identified two plugins as potential causes,\n"
                                    "\tnamely SampVoice and Pawn.Raknet.\n"
                                    "\tAre you using SampVoice version 3.1?\n"
                                    "\tYou can downgrade to SampVoice version 3.0 if necessary,\n"
                                    "\tor you can remove either Sampvoice or Pawn.Raknet to avoid a potential crash.\n"
                                    "\tYou can review the changes between versions 3.0 and 3.1 to understand and analyze the possible reason for the crash\n"
                                    "\ton here: https://github.com/CyberMor/sampvoice/compare/v3.0-alpha...v3.1\n");
                                size_l = (n < 0) ? 0 : (size_t)n;
                                fwrite(out, 1, size_l, stdout);
                                fflush(stdout);

                                printf("\x1b[32m==> downgrading sampvoice? 3.1 -> 3.0? (Auto-fix)\x1b[0m\n");
                                fwrite(out, 1, size_l, stdout);
                                fflush(stdout);
                                char *downgrading = readline("   answer (y/n): ");
                                if (downgrading && (downgrading[0] == '\0' || strcmp(downgrading, "Y") == 0 || strcmp(downgrading, "y") == 0)) {
                                    dog_install_depends("CyberMor/sampvoice?v3.0-alpha", "master", NULL);
                                }
                                dog_free(downgrading);
                            }
                        }
                    }
                }

                /* Memory-related error patterns */
                if (strfind(buf, "stack", true)) {
                    n = snprintf(out, sizeof(out), "@ Stack-related issue detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "memory", true)) {
                    n = snprintf(out, sizeof(out), "@ Memory-related issue detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "access violation", true)) {
                    n = snprintf(out, sizeof(out), "@ Access violation detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "buffer overrun", true) || strfind(buf, "buffer overflow", true)) {
                    n = snprintf(out, sizeof(out), "@ Buffer overflow detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "null pointer", true)) {
                    n = snprintf(out, sizeof(out), "@ Null pointer exception detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 10: Array out-of-bounds errors with example fix */
            if (strfind(buf, "out of bounds", true) ||
                strfind(buf, "out-of-bounds", true)) {
                n = snprintf(out, sizeof(out), "@ out-of-bounds detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                n = snprintf(out, sizeof(out),
                    "\tnew array[3];\n"
                    "\tmain() {\n"
                    "\t  for (new i = 0; i < 4; i++) < potent 4 of 3\n"
                    "\t                      ^ sizeof(array)   for array[this] and array[this][]\n"
                    "\t                      ^ sizeof(array[]) for array[][this]\n"
                    "\t                      * instead of manual indexing..\n"
                    "\t     array[i] = 0;\n"
                    "\t}\n");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 11: RCON password security warning (SA-MP specific) */
            if (fetch_server_env() == 1) {
                if (strfind(buf, "Your password must be changed from the default password", true)) {
                    ++server_rcon_pass;
                }
            }

            /* Pattern 12: Missing gamemode0 configuration buffer */
            if (strfind(buf, "It needs a gamemode0 buffer", true)) {
                n = snprintf(out, sizeof(out), "@ Critical message found\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                n = snprintf(out, sizeof(out),
                    "\tYou need to ensure that the file name (.amx),\n"
                    "\tin your server.cfg under the parameter (gamemode0),\n"
                    "\tactually exists as a .amx file in the gamemodes/ folder.\n"
                    "\tIf there's only a file with the corresponding name but it's only a single .pwn file,\n"
                    "\tyou need to compile it.\n");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 13: Generic warning messages */
            if (strfind(buf, "warning", true) || strfind(buf, "Warning", true)) {
                n = snprintf(out, sizeof(out), "@ Warning message found\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 14: Failure messages */
            if (strfind(buf, "failed", true) || strfind(buf, "Failed", true)) {
                n = snprintf(out, sizeof(out), "@ Failure or Failed message detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 15: Timeout events */
            if (strfind(buf, "timeout", true) || strfind(buf, "Timeout", true)) {
                n = snprintf(out, sizeof(out), "@ Timeout event detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 16: Plugin-related issues */
            if (strfind(buf, "plugin", true) || strfind(buf, "Plugin", true)) {
                if (strfind(buf, "failed to load", true) || strfind(buf, "Failed.", true)) {
                    n = snprintf(out, sizeof(out), "@ Plugin load failure or failed detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                    n = snprintf(out, sizeof(out),
                        "\tIf you need to reinstall a plugin that failed, you can use the command:\n"
                        "\t\tinstall user/repo:tags\n"
                        "\tExample:\n"
                        "\t\tinstall Y-Less/sscanf?newer\n"
                        "\tYou can also recheck the username shown on the failed plugin using the command:\n"
                        "\t\ttracker name\n");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    fflush(stdout);
                }
                if (strfind(buf, "unloaded", true)) {
                    n = snprintf(out, sizeof(out), "@ Plugin unloaded detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                    n = snprintf(out, sizeof(out),
                        "\tLOADED (Active/In Use):\n"
                        "\t  - Plugin is running, all features are available.\n"
                        "\t  - Utilizing system memory and CPU (e.g., running background threads).\n"
                        "\tUNLOADED (Deactivated/Inactive):\n"
                        "\t  - Plugin has been shut down and removed from memory.\n"
                        "\t  - Features are no longer available; system resources (memory/CPU) are released.\n");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    fflush(stdout);
                }
            }

            /* Pattern 17: Database/MySQL connection issues */
            if (strfind(buf, "database", true) || strfind(buf, "mysql", true)) {
                if (strfind(buf, "connection failed", true) || strfind(buf, "can't connect", true)) {
                    n = snprintf(out, sizeof(out), "@ Database connection failure detected\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "error", true) || strfind(buf, "failed", true)) {
                    n = snprintf(out, sizeof(out), "@ Error or Failed database | mysql found\n\t");
                    size_l = (n < 0) ? 0 : (size_t)n;
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 18: Memory allocation failures */
            if (strfind(buf, "out of memory", true) || strfind(buf, "memory allocation", true)) {
                n = snprintf(out, sizeof(out), "@ Memory allocation failure detected\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 19: Memory management function references */
            if (strfind(buf, "malloc", true) || strfind(buf, "free", true) ||
                strfind(buf, "realloc", true) || strfind(buf, "calloc", true)) {
                n = snprintf(out, sizeof(out), "@ Memory management function referenced\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            fflush(stdout);
        }

        fclose(this_proc_file);

        /* SampVoice port mismatch detection */
        if (rate_sampvoice_server) {
            if (path_access("server.cfg") == 0)
                goto skip;  /* No server.cfg to check */

            this_proc_file = fopen("server.cfg", "rb");
            if (this_proc_file == NULL)
                goto skip;

            int _sampvoice_port = 0;
            char _p_sampvoice_port[16] = {0};

            /* Find sv_port setting in server.cfg */
            while (fgets(buf, sizeof(buf), this_proc_file)) {
                if (strfind(buf, "sv_port", true)) {
                    if (sscanf(buf, "sv_port %d", &_sampvoice_port) != 1)
                        break;
                    snprintf(_p_sampvoice_port, sizeof(_p_sampvoice_port), "%d", _sampvoice_port);
                    break;
                }
            }
            fclose(this_proc_file);

            /* Compare configured port with actual running port */
            if (sampvoice_port && strcmp(_p_sampvoice_port, sampvoice_port) != 0) {
                n = snprintf(out, sizeof(out), "@ SampVoice Port\n\t");
                size_l = (n < 0) ? 0 : (size_t)n;
                pr_color(stdout, DOG_COL_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
                fwrite(out, 1, size_l, stdout);
                n = snprintf(out, sizeof(out),
                    "\tWe have detected a mismatch between the sampvoice port in server.cfg\n"
                    "\tand the one loaded in the server log!\n"
                    "\t* Please make sure you have correctly set the port in server.cfg.\n");
                size_l = (n < 0) ? 0 : (size_t)n;
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }
        }

    skip:
        /* RCON password auto-fix for default password vulnerability */
        if (server_rcon_pass) {
            n = snprintf(out, sizeof(out),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            size_l = (n < 0) ? 0 : (size_t)n;
            fwrite(out, 1, size_l, stdout);
            fflush(stdout);

            /* Offer to auto-fix RCON password */
            char *fixed_now = readline("Auto-fix? (Y/n): ");

            if (fixed_now && (fixed_now[0] == '\0' || !strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y"))) {
              if (path_access("server.cfg")) {
                FILE *read_f = fopen("server.cfg", "rb");
                if (read_f) {
                  /* Read entire server.cfg into memory */
                  fseek(read_f, 0, SEEK_END);
                  long server_fle_size = ftell(read_f);
                  fseek(read_f, 0, SEEK_SET);

                  char *serv_f_cent = NULL;
                  serv_f_cent = dog_malloc(server_fle_size + 1);
                  if (!serv_f_cent) { goto skip_fixing; }

                  size_t br;
                  br = fread(serv_f_cent, 1, server_fle_size, read_f);
                  serv_f_cent[br] = '\0';
                  fclose(read_f);

                  /* Find and replace default RCON password */
                  char *server_n_content = NULL;
                  char *pos = strstr(serv_f_cent, "rcon_password changeme");

                  uint32_t crc32_generate;
                  if (pos) {
                    server_n_content = dog_malloc(server_fle_size + 10);
                    if (!server_n_content) {
                        dog_free(serv_f_cent);
                        goto skip_fixing;
                    }

                    /* Copy content before the password */
                    strncpy(server_n_content, serv_f_cent, pos - serv_f_cent);
                    server_n_content[pos - serv_f_cent] = '\0';

                    srand((unsigned int)time(NULL) ^ rand());
                    int rand7 = rand() % 10000000;
                    char size_rand7[DOG_PATH_MAX];
                    n = snprintf(size_rand7, sizeof(size_rand7), "%d", rand7);
                    crc32_generate = crypto_generate_crc32(size_rand7, sizeof(size_rand7) - 1);

                    /* Format new password buffer */
                    char crc_str[14 + 11 + 1];
                    sprintf(crc_str, "rcon_password %08X", crc32_generate);

                    /* Build new file content */
                    strcat(server_n_content, crc_str);
                    strcat(server_n_content, pos + strlen("rcon_password changeme"));
                  }

                  /* Write updated content back to file */
                  if (server_n_content) {
                      FILE *write_f = fopen("server.cfg", "wb");
                      if (write_f) {
                              fwrite(server_n_content, 1, strlen(server_n_content), write_f);
                              fclose(write_f);
                              n = snprintf(out, sizeof(out), "done! * server.cfg - rcon_password from changeme to %08X.\n", crc32_generate);
                              size_l = (n < 0) ? 0 : (size_t)n;
                              fwrite(out, 1, size_l, stdout);
                              fflush(stdout);
                      } else {
                              n = snprintf(out, sizeof(out), "Error: Cannot write to server.cfg\n");
                              size_l = (n < 0) ? 0 : (size_t)n;
                              fwrite(out, 1, size_l, stdout);
                              fflush(stdout);
                      }
                      dog_free(server_n_content);
                  } else {
                      n = snprintf(out, sizeof(out),
                              "-Replacement failed!\n"
                              " It is not known what the primary cause is."
                              " A reasonable explanation"
                              " is that it occurs when server.cfg does not contain the rcon_password parameter.\n");
                      size_l = (n < 0) ? 0 : (size_t)n;
                      fwrite(out, 1, size_l, stdout);
                      fflush(stdout);
                  }
                  dog_free(serv_f_cent);
                }
            }
          }
          dog_free(fixed_now);
        }

    skip_fixing:
        /* Print closing separator */
        n = snprintf(out, sizeof(out),
              "====================================================================\n");
        size_l = (n < 0) ? 0 : (size_t)n;
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        /* Cleanup SampVoice port memory */
        if (sampvoice_port) {
            free(sampvoice_port);
            sampvoice_port = NULL;
        }

        /* Offer to install crashdetect plugin if crashes detected but plugin missing */
        if (rate_problem_stat == 1 && server_crashdetect < 1) {
              n = snprintf(out, sizeof(out), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? (Auto-fix) ");
              size_l = (n < 0) ? 0 : (size_t)n;
              fwrite(out, 1, size_l, stdout);
              fflush(stdout);

              char *confirm;
              confirm = readline("Y/n ");
              if (confirm && (confirm[0] == '\0' || strfind(confirm, "y", true))) {
                  dog_free(confirm);
                  dog_install_depends("Y-Less/samp-plugin-crashdetect?newer", "master", NULL);
              } else {
                  dog_free(confirm);
                  return;
              }
        }

        return;
}