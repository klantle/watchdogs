/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

/*
 * System headers for core functionality:
 * - Standard I/O (stdio), memory allocation (stdlib), string operations (string)
 * - Boolean types (stdbool), system types (sys/types), file statistics (sys/stat)
 * - Time functions (time), signal handling (signal)
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdbool.h>
#include  <sys/types.h>
#include  <sys/stat.h>
#include  <time.h>
#include  <signal.h>

/* Project-specific headers organized by functionality */
#include  "units.h"        /* Unit management functions */
#include  "extra.h"        /* Additional utilities */
#include  "utils.h"        /* General-purpose utilities */
#include  "crypto.h"       /* Cryptographic functions (CRC-32, etc.) */
#include  "replicate.h"    /* Replication/backup functionality */
#include  "compiler.h"     /* PAWN compiler integration */
#include  "debug.h"        /* Debugging utilities */
#include  "endpoint.h"     /* Network/endpoint communication */

/* Global variables for state management */

/* Signal handler flag - tracks SIGINT (Ctrl+C) state */
int               sigint_handler   = 0;

/* Buffer variables for various operations */
static char       line[0x400];                  /* Line buffer for reading files */
static char       size_gamemode[DOG_PATH_MAX * 0x2]; /* Buffer for gamemode configuration */
static char       command[DOG_MAX_PATH + DOG_PATH_MAX * 2]; /* Command buffer for process execution */

/* JSON/CJSON related pointers */
static char      *cJSON_Data = NULL;           /* Raw JSON data */
static char      *printed = NULL;              /* Formatted JSON output */
static char      *sampvoice_port = NULL;       /* SampVoice plugin port string */

/* Rate limiting and state tracking counters */
static int        rate_sampvoice_server = 0;   /* SampVoice rate limiter */
static int        rate_problem_stat = 0;       /* Problem detection rate limiter */
static int        server_crashdetect = 0;      /* Crash detection counter */
static int        server_rcon_pass = 0;        /* RCON password error counter */

/* File handles for configuration processing */
static FILE      *proc_conf_in = NULL;         /* Input configuration file */
static FILE      *proc_conf_out = NULL;        /* Output configuration file */

/* cJSON objects for JSON configuration parsing */
static cJSON     *root = NULL;                 /* Root JSON object */
static cJSON     *pawn = NULL;                 /* "pawn" section of JSON */
static cJSON     *msj = NULL;                  /* "msj" (gamemode script) array */

/*
 * Function: try_cleanup_server
 * Purpose: Clean up server state and resources to ensure clean restart
 * Process: Resets all global variables, stops server tasks, restores config
 * Returns: void
 */
void try_cleanup_server(void) {
        /* Clear all string buffers to prevent stale data */
        memset(line, 0, sizeof(line));
        memset(size_gamemode, 0, sizeof(size_gamemode));
        memset(command, 0, sizeof(command));

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
 * Function: unit_sigint_handler
 * Purpose: Handle SIGINT (Ctrl+C) signal gracefully
 * Process: Cleans up server state, creates crash detection marker
 * Parameters:
 *   sig - Signal number (should be SIGINT)
 * Returns: void
 */
void unit_sigint_handler(int sig) {
        /* Clean up server state first */
        try_cleanup_server();

        /* Record the time when signal was received */
        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);

        /* Create crash detection marker file to indicate abnormal termination */
        FILE *crashdetect_file = NULL;
        crashdetect_file = fopen(".watchdogs/crashdetect", "w");

        if (crashdetect_file != NULL)
            fclose(crashdetect_file);
}

/*
 * Function: dog_stop_server_tasks
 * Purpose: Stop running server processes based on environment
 * Process: Checks server environment type and kills appropriate process
 * Returns: void
 */
void dog_stop_server_tasks(void) {
        /* Kill server binary based on environment type */
        if (dog_server_env() == 1)      /* SA-MP environment */
            dog_kill_process(dogconfig.dog_toml_binary);
        else if (dog_server_env() == 2) /* open.mp environment */
            dog_kill_process(dogconfig.dog_toml_binary);
}

/*
 * Function: update_samp_config (static)
 * Purpose: Update SA-MP server configuration with new gamemode
 * Process:
 *   1. Creates backup of existing config
 *   2. Opens backup for reading
 *   3. Writes new config with updated gamemode0 line
 *   4. Preserves all other configuration lines
 * Parameters:
 *   g - New gamemode name (without .amx extension)
 * Returns:
 *   1 on success, -1 on failure
 */
static int update_samp_config(const char *g) {
        char puts[DOG_PATH_MAX + 0x1A];        /* Buffer for formatted gamemode name */
        char size_config[DOG_PATH_MAX];        /* Backup file path buffer */

        /* Create backup file path: .watchdogs/<config>.bak */
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        /* Remove existing backup if it exists */
        if (path_access(size_config))
                remove(size_config);

        /* Platform-specific file moving for backup creation */
#ifdef DOG_WINDOWS
        /* Windows: Use MoveFileExA with flags for atomic operation */
        if (!MoveFileExA(
                dogconfig.dog_toml_config,
                size_config,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            pr_error(stdout, "Failed to create backup file");
            minimal_debugging();
            return -1;
        }
#else
        /* Unix/Linux: Use fork+exec for mv command */
        pid_t pid = fork();
        if (pid == 0) {
            execlp("mv", "mv", "-f",
                dogconfig.dog_toml_config,
                size_config,
                NULL);
            _exit(127);
        }
        if (pid < 0 || waitpid(pid, NULL, 0) < 0) {
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

        /* Convert file descriptor to FILE* for line-based reading */
        proc_conf_in = fdopen(fd, "r");
        if (!proc_conf_in) {
                close(fd);
                return -1;
        }

        /* Open destination config file for writing */
        proc_conf_out = fopen(dogconfig.dog_toml_config, "w+");
        if (!proc_conf_out) {
                pr_error(stdout, "Failed to write new config");
                minimal_debugging();
                fclose(proc_conf_in);
                return -1;
        }

        /* Prepare gamemode name by removing extension if present */
        snprintf(puts, sizeof(puts), "%s", g);
        char *e = strrchr(puts, '.');
        if (e) *e = '\0';  /* Remove file extension */
        g = puts;

        /* Process config file line by line */
        while (fgets(line, sizeof(line), proc_conf_in)) {
            /* Look for gamemode0 line to replace */
            if (strfind(line, "gamemode0", true)) {
                /* Create new gamemode0 line with updated gamemode */
                snprintf(size_gamemode, sizeof(size_gamemode),
                        "gamemode0 %s\n", puts);
                fputs(size_gamemode, proc_conf_out);
                continue;
            }
            /* Copy all other lines unchanged */
            fputs(line, proc_conf_out);
        }

        /* Cleanup file handles */
        fclose(proc_conf_in);
        fclose(proc_conf_out);

        return 1;  /* Success */
}

/*
 * Function: restore_server_config
 * Purpose: Restore server configuration from backup file
 * Process:
 *   1. Checks if backup exists
 *   2. Prompts user for confirmation
 *   3. Replaces current config with backup
 *   4. Platform-specific file operations
 * Returns: void
 */
void restore_server_config(void) {
        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        /* Check if backup file exists */
        if (path_exists(size_config) == 0)
            goto restore_done;  /* No backup, nothing to restore */

        /* Prompt user for confirmation */
        printf(DOG_COL_GREEN "warning: " DOG_COL_DEFAULT
                "Continue to restore %s -> %s? y/n",
                size_config, dogconfig.dog_toml_config);

        char *restore_confirm = readline(" ");
        if (restore_confirm && strfind(restore_confirm, "Y", true)) {
                ;  /* User confirmed, continue with restore */
        } else {
                dog_free(restore_confirm);
                unit_ret_main(NULL);  /* Exit if user declines */
        }

        dog_free(restore_confirm);

        /* Check if backup is accessible */
        if (path_access(size_config) == 0)
                goto restore_done;

        /* Platform-specific file deletion for current config */
#ifdef DOG_WINDOWS
            DWORD attr = GetFileAttributesA(dogconfig.dog_toml_config);
            if (attr != INVALID_FILE_ATTRIBUTES &&
                !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                DeleteFileA(dogconfig.dog_toml_config);
            }
#else
            unlink(dogconfig.dog_toml_config);
#endif

        /* Platform-specific file moving to restore from backup */
#ifdef DOG_WINDOWS
            if (!MoveFileExA(
                    size_config,
                    dogconfig.dog_toml_config,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                return;
            }
#else
            if (rename(size_config, dogconfig.dog_toml_config) != 0) {
                return;
            }
#endif

restore_done:
        return;
}

/*
 * Function: dog_exec_samp_server
 * Purpose: Execute SA-MP server with specified gamemode
 * Process:
 *   1. Validates configuration and gamemode file
 *   2. Updates server configuration
 *   3. Sets up signal handlers
 *   4. Executes server binary with platform-specific methods
 *   5. Handles retry logic on failure
 * Parameters:
 *   g - Gamemode name
 *   server_bin - Server binary executable name
 * Returns: void
 */
void dog_exec_samp_server(const char *g, const char *server_bin) {
        minimal_debugging();  /* Log minimal debug info */

        /* Validate configuration file type */
        if (strfind(dogconfig.dog_toml_config, ".json", true))
                return;  /* JSON configs not supported for SA-MP */

        int ret = -1;  /* Process execution result */

        /* Prepare gamemode name with .amx extension */
        char puts[0x100];
        char *e = strrchr(g, '.');
        if (e) {
                size_t len = e - g;
                snprintf(puts, sizeof(puts), "%.*s.amx", (int)len, g);
        } else {
                snprintf(puts, sizeof(puts), "%s.amx", g);
        }
        g = puts;

        /* Restore system error handling */
        dog_sef_restore();

        /* Verify gamemode file exists */
        if (dog_sef_fdir(".", g, NULL) == 0) {
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
        time_t start, end;
        double elp;
        bool rty = false;  /* Retry flag */
        int _access = -1;

back_start:  /* Retry label for failed startup attempts */
        start = time(NULL);
        printf(DOG_COL_BLUE "");  /* Set output color */

        /* Platform-specific process execution */
#ifdef DOG_WINDOWS
        /* Windows: CreateProcess API for process creation */
        STARTUPINFOA _STARTUPINFO;
        PROCESS_INFORMATION _PROCESS_INFO;

        _ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
        _ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

        _STARTUPINFO.cb = sizeof(_STARTUPINFO);
        _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

        _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
        _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

        snprintf(command, sizeof(command), "%s%s", SYM_PROG, server_bin);

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
        pid_t pid;
        CHMOD_FULL(server_bin);  /* Ensure binary is executable */

        char cmd[DOG_PATH_MAX + 26];
        snprintf(cmd, sizeof(cmd), "%s/%s", dog_procure_pwd(), server_bin);

        /* Create pipes for stdout and stderr redirection */
        int stdout_pipe[2];
        int stderr_pipe[2];

        if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
            perror("pipe");
            return;
        }

        pid = fork();
        if (pid == 0) {  /* Child process */
            close(stdout_pipe[0]);
            close(stderr_pipe[0]);

            dup2(stdout_pipe[1], STDOUT_FILENO);
            dup2(stderr_pipe[1], STDERR_FILENO);

            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            char *argv[] = { "/bin/sh", "-c", (char *)cmd, NULL };
            execv("/bin/sh", argv);
            _exit(127);  /* execv failed */
        } else if (pid > 0) {  /* Parent process */
            close(stdout_pipe[1]);
            close(stderr_pipe[1]);

            /* Read and display child process output */
            char buffer[DOG_MAX_PATH];
            ssize_t br;

            while ((br = read(stdout_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
                buffer[br] = '\0';
                printf("%s", buffer);
            }

            while ((br = read(stderr_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
                buffer[br] = '\0';
                printf("%s", buffer);
            }

            close(stdout_pipe[0]);
            close(stderr_pipe[0]);

            /* Wait for child process to complete */
            int status;
            waitpid(pid, &status, 0);

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
                    _access = path_access(dogconfig.dog_toml_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_logs);

                    _access = path_access(dogconfig.dog_toml_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_logs);

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
 * Function: update_omp_config (static)
 * Purpose: Update open.mp JSON configuration with new gamemode
 * Process:
 *   1. Creates backup of existing JSON config
 *   2. Parses JSON structure
 *   3. Updates "pawn"/"msj" array with new gamemode
 *   4. Writes updated JSON back to config file
 * Parameters:
 *   g - New gamemode name (without .amx extension)
 * Returns:
 *   0 on success, -1 on failure
 */
static int update_omp_config(const char *g) {
        struct stat st;  /* File statistics */
        char   gf[DOG_PATH_MAX + 0x1A];  /* Buffer for gamemode name */
        char   puts[DOG_PATH_MAX + 0x1A]; /* Formatted gamemode name */
        int    ret = -1;

        /* Create backup file path */
        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        /* Remove existing backup */
        if (path_access(size_config))
                remove(size_config);

        /* Platform-specific backup creation */
#ifdef DOG_WINDOWS
            if (!MoveFileExA(
                    dogconfig.dog_toml_config,
                    size_config,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                pr_error(stdout, "Failed to create backup file");
                minimal_debugging();
                return -1;
            }
#else
            pid_t pid = fork();
            if (pid == 0) {
                execlp("mv", "mv", "-f",
                    dogconfig.dog_toml_config,
                    size_config,
                    NULL);
                _exit(127);
            }
            if (pid < 0 || waitpid(pid, NULL, 0) < 0) {
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
        snprintf(puts, sizeof(puts), "%s", g);
        char *e = strrchr(puts, '.');
        if (e) *e = '\0';

        /* Remove existing "msj" array and create new one */
        cJSON_DeleteItemFromObject(pawn, "msj");

        msj = cJSON_CreateArray();
        snprintf(gf, sizeof(gf), "%s", puts);
        cJSON_AddItemToArray(msj, cJSON_CreateString(gf));
        cJSON_AddItemToObject(pawn, "msj", msj);

        /* Open output file for writing updated JSON */
        proc_conf_out = fopen(dogconfig.dog_toml_config, "w");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write %s", dogconfig.dog_toml_config);
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
                    "Failed to write to %s", dogconfig.dog_toml_config);
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

/* Wrapper function for open.mp config restoration */
void restore_omp_config(void) { restore_server_config(); }

/*
 * Function: dog_exec_omp_server
 * Purpose: Execute open.mp server with specified gamemode
 * Process: Similar to SA-MP version but for open.mp JSON config format
 * Parameters:
 *   g - Gamemode name
 *   server_bin - Server binary executable name
 * Returns: void
 */
void dog_exec_omp_server(const char *g, const char *server_bin) {
        minimal_debugging();

        /* Validate config file type */
        if (strfind(dogconfig.dog_toml_config, ".cfg", true))
                return;  /* .cfg files not supported for open.mp */

        int ret = -1;

        /* Prepare gamemode name with .amx extension */
        char puts[0x100];
        char *e = strrchr(g, '.');
        if (e) {
            size_t len = e - g;
            snprintf(puts, sizeof(puts), "%.*s.amx", (int)len, g);
        } else {
            snprintf(puts, sizeof(puts), "%s.amx", g);
        }
        g = puts;

        dog_sef_restore();

        /* Verify gamemode exists */
        if (dog_sef_fdir(".", g, NULL) == 0) {
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
        time_t start, end;
        double elp;
        bool rty = false;
        int _access = -1;

back_start:  /* Retry label */
        start = time(NULL);
        printf(DOG_COL_BLUE "");

        /* Platform-specific process execution (same as SA-MP version) */
#ifdef DOG_WINDOWS
            STARTUPINFOA _STARTUPINFO;
            PROCESS_INFORMATION _PROCESS_INFO;

            _ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
            _ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

            _STARTUPINFO.cb = sizeof(_STARTUPINFO);
            _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

            _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
            _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

            snprintf(command, sizeof(command),
                "%s%s", SYM_PROG, server_bin);

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
            pid_t pid;

            CHMOD_FULL(server_bin);

            char cmd[DOG_PATH_MAX + 26];
            snprintf(cmd, sizeof(cmd), "%s/%s",
                dog_procure_pwd(), server_bin);

            int stdout_pipe[2];
            int stderr_pipe[2];

            if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
                perror("pipe");
                return;
            }

            pid = fork();
            if (pid == 0) {
                close(stdout_pipe[0]);
                close(stderr_pipe[0]);

                dup2(stdout_pipe[1], STDOUT_FILENO);
                dup2(stderr_pipe[1], STDERR_FILENO);

                close(stdout_pipe[1]);
                close(stderr_pipe[1]);

                char *argv[] = {
                    "/bin/sh",
                    "-c",
                    (char *)cmd,
                    NULL
                };
                execv("/bin/sh", argv);
                _exit(127);
            } else if (pid > 0) {
                close(stdout_pipe[1]);
                close(stderr_pipe[1]);

                char buffer[DOG_MAX_PATH];
                ssize_t br;

                while ((br = read(stdout_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
                    buffer[br] = '\0';
                    printf("%s", buffer);
                }

                while ((br = read(stderr_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
                    buffer[br] = '\0';
                    printf("%s", buffer);
                }

                close(stdout_pipe[0]);
                close(stderr_pipe[0]);

                int status;
                waitpid(pid, &status, 0);

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
                    _access = path_access(dogconfig.dog_toml_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_logs);
                    _access = path_access(dogconfig.dog_toml_logs);
                    if (_access)
                            remove(dogconfig.dog_toml_logs);
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
 * Function: dog_server_crash_check
 * Purpose: Analyze server logs for errors, warnings, and crash patterns
 * Process:
 *   1. Opens server log file
 *   2. Scans line by line for known error patterns
 *   3. Provides diagnostic information and auto-fix suggestions
 *   4. Handles specific issues like RCON password, plugin conflicts
 * Returns: void
 */
void dog_server_crash_check(void) {
        int   size_l;  /* Output size tracker */
        FILE *this_proc_file = NULL;  /* Log file handle */
        char  out[DOG_MAX_PATH + 26]; /* Output buffer */
        char  buf[DOG_MAX_PATH * 4];  /* Line buffer for log reading */

        /* Open appropriate log file based on server environment */
        if (dog_server_env() == 1)  /* SA-MP */
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");
        else  /* open.mp */
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");

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
        size_l = snprintf(out, sizeof(out),
            "====================================================================\n");
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        /* Process log file line by line */
        while (fgets(buf, sizeof(buf), this_proc_file)) {
            /* Pattern 1: Filterscript loading errors */
            if (strfind(buf, "Unable to load filterscript", true)) {
                size_l = snprintf(out, sizeof(out),
                    "@ Unable to load filterscript detected - Please recompile our filterscripts.\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 2: Invalid index/entry point errors */
            if (strfind(buf, "Invalid index parameter (bad entry point)", true)) {
                size_l = snprintf(out, sizeof(out),
                    "@ Invalid index parameter (bad entry point) detected - You're forget " DOG_COL_CYAN "'main'" DOG_COL_DEFAULT "?\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 3: Runtime errors (most common crash cause) */
            if (strfind(buf, "run time error", true) || strfind(buf, "Run time error", true)) {
                rate_problem_stat = 1;  /* Flag that we found a problem */
                size_l = snprintf(out, sizeof(out), "@ Runtime error detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);

                /* Specific runtime error subtypes */
                if (strfind(buf, "division by zero", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Division by zero error found\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "invalid index", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Invalid index error found\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 4: Outdated include files requiring recompilation */
            if (strfind(buf, "The script might need to be recompiled with the latest include file.", true)) {
                size_l = snprintf(out, sizeof(out), "@ Needed for recompiled\n\t");
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
                size_l = snprintf(out, sizeof(out), "@ Filesystem C++ Error Detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                size_l = snprintf(out, sizeof(out),
                    "\tAre you currently using the WSL ecosystem?\n"
                    "\tYou need to move the open.mp server folder from the /mnt area (your Windows directory) to \"~\" (your WSL HOME).\n"
                    "\tThis is because open.mp C++ filesystem cannot properly read directories inside the /mnt area,\n"
                    "\twhich isn't part of the directory model targeted by the Linux build.\n"
                    "\t* You must run it outside the /mnt area.\n");
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
                size_l = snprintf(out, sizeof(out), "@ Can't found gamemode detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                size_l = snprintf(out, sizeof(out),
                    "\tYou need to ensure that the name specified "
                    "in the configuration file matches the one in the gamemodes/ folder,\n"
                    "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                    "\tthen main.amx must be present in the gamemodes/ directory\n");
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 8: Memory address references (potential crashes) */
            if (strfind(buf, "0x", true)) {
                size_l = snprintf(out, sizeof(out), "@ Hexadecimal address found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "address", true) || strfind(buf, "Address", true)) {
                size_l = snprintf(out, sizeof(out), "@ Memory address reference found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 9: Crashdetect plugin output (detailed crash info) */
            if (rate_problem_stat) {
                if (strfind(buf, "[debug]", true) || strfind(buf, "crashdetect", true)) {
                    ++server_crashdetect;
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: Crashdetect debug information found\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);

                    /* Crashdetect-specific patterns */
                    if (strfind(buf, "AMX backtrace", true)) {
                        size_l = snprintf(out, sizeof(out), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "native stack trace", true)) {
                        size_l = snprintf(out, sizeof(out), "@ Crashdetect: Native stack trace detected\n\t");
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "heap", true)) {
                        size_l = snprintf(out, sizeof(out), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }
                    if (strfind(buf, "[debug]", true)) {
                        size_l = snprintf(out, sizeof(out), "@ Crashdetect: Debug Detected\n\t");
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);
                    }

                    /* Native backtrace with plugin conflict detection */
                    if (strfind(buf, "Native backtrace", true)) {
                        size_l = snprintf(out, sizeof(out), "@ Crashdetect: Native backtrace detected\n\t");
                        fwrite(out, 1, size_l, stdout);
                        pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                        fflush(stdout);

                        /* Detect SampVoice and Pawn.Raknet plugin conflicts */
                        if (strfind(buf, "sampvoice", true)) {
                            if(strfind(buf, "pawnraknet", true)) {
                                size_l = snprintf(out, sizeof(out), "@ Crashdetect: Crash potent detected\n\t");
                                fwrite(out, 1, size_l, stdout);
                                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                                fflush(stdout);
                                size_l = snprintf(out, sizeof(out),
                                    "\tWe have detected a crash and identified two plugins as potential causes,\n"
                                    "\tnamely SampVoice and Pawn.Raknet.\n"
                                    "\tAre you using SampVoice version 3.1?\n"
                                    "\tYou can downgrade to SampVoice version 3.0 if necessary,\n"
                                    "\tor you can remove either Sampvoice or Pawn.Raknet to avoid a potential crash.\n"
                                    "\tYou can review the changes between versions 3.0 and 3.1 to understand and analyze the possible reason for the crash\n"
                                    "\ton here: https://github.com/CyberMor/sampvoice/compare/v3.0-alpha...v3.1\n");
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
                    size_l = snprintf(out, sizeof(out), "@ Stack-related issue detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "memory", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Memory-related issue detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "access violation", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Access violation detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "buffer overrun", true) || strfind(buf, "buffer overflow", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Buffer overflow detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "null pointer", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Null pointer exception detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 10: Array out-of-bounds errors with example fix */
            if (strfind(buf, "out of bounds", true) ||
                strfind(buf, "out-of-bounds", true)) {
                size_l = snprintf(out, sizeof(out), "@ out-of-bounds detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                size_l = snprintf(out, sizeof(out),
                    "\tnew array[3];\n"
                    "\tmain() {\n"
                    "\t  for (new i = 0; i < 4; i++) < potent 4 of 3\n"
                    "\t                      ^ sizeof(array)   for array[this] and array[this][]\n"
                    "\t                      ^ sizeof(array[]) for array[][this]\n"
                    "\t                      * instead of manual indexing..\n"
                    "\t     array[i] = 0;\n"
                    "\t}\n");
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 11: RCON password security warning (SA-MP specific) */
            if (dog_server_env() == 1) {
                if (strfind(buf, "Your password must be changed from the default password", true)) {
                    ++server_rcon_pass;
                }
            }

            /* Pattern 12: Missing gamemode0 configuration line */
            if (strfind(buf, "It needs a gamemode0 line", true)) {
                size_l = snprintf(out, sizeof(out), "@ Critical message found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
                size_l = snprintf(out, sizeof(out),
                    "\tYou need to ensure that the file name (.amx),\n"
                    "\tin your server.cfg under the parameter (gamemode0),\n"
                    "\tactually exists as a .amx file in the gamemodes/ folder.\n"
                    "\tIf there's only a file with the corresponding name but it's only a single .pwn file,\n"
                    "\tyou need to compile it.\n");
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }

            /* Pattern 13: Generic warning messages */
            if (strfind(buf, "warning", true) || strfind(buf, "Warning", true)) {
                size_l = snprintf(out, sizeof(out), "@ Warning message found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 14: Failure messages */
            if (strfind(buf, "failed", true) || strfind(buf, "Failed", true)) {
                size_l = snprintf(out, sizeof(out), "@ Failure or Failed message detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 15: Timeout events */
            if (strfind(buf, "timeout", true) || strfind(buf, "Timeout", true)) {
                size_l = snprintf(out, sizeof(out), "@ Timeout event detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 16: Plugin-related issues */
            if (strfind(buf, "plugin", true) || strfind(buf, "Plugin", true)) {
                if (strfind(buf, "failed to load", true) || strfind(buf, "Failed.", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Plugin load failure or failed detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                    size_l = snprintf(out, sizeof(out),
                        "\tIf you need to reinstall a plugin that failed, you can use the command:\n"
                        "\t\tinstall user/repo:tags\n"
                        "\tExample:\n"
                        "\t\tinstall Y-Less/sscanf?newer\n"
                        "\tYou can also recheck the username shown on the failed plugin using the command:\n"
                        "\t\ttracker name\n");
                    fwrite(out, 1, size_l, stdout);
                    fflush(stdout);
                }
                if (strfind(buf, "unloaded", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Plugin unloaded detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                    size_l = snprintf(out, sizeof(out),
                        "\tLOADED (Active/In Use):\n"
                        "\t  - Plugin is running, all features are available.\n"
                        "\t  - Utilizing system memory and CPU (e.g., running background threads).\n"
                        "\tUNLOADED (Deactivated/Inactive):\n"
                        "\t  - Plugin has been shut down and removed from memory.\n"
                        "\t  - Features are no longer available; system resources (memory/CPU) are released.\n");
                    fwrite(out, 1, size_l, stdout);
                    fflush(stdout);
                }
            }

            /* Pattern 17: Database/MySQL connection issues */
            if (strfind(buf, "database", true) || strfind(buf, "mysql", true)) {
                if (strfind(buf, "connection failed", true) || strfind(buf, "can't connect", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Database connection failure detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "error", true) || strfind(buf, "failed", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Error or Failed database | mysql found\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                    fflush(stdout);
                }
            }

            /* Pattern 18: Memory allocation failures */
            if (strfind(buf, "out of memory", true) || strfind(buf, "memory allocation", true)) {
                size_l = snprintf(out, sizeof(out), "@ Memory allocation failure detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, DOG_COL_BLUE, "%s", buf);
                fflush(stdout);
            }

            /* Pattern 19: Memory management function references */
            if (strfind(buf, "malloc", true) || strfind(buf, "free", true) ||
                strfind(buf, "realloc", true) || strfind(buf, "calloc", true)) {
                size_l = snprintf(out, sizeof(out), "@ Memory management function referenced\n\t");
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
                size_l = snprintf(out, sizeof(out), "@ SampVoice Port\n\t");
                pr_color(stdout, DOG_COL_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
                fwrite(out, 1, size_l, stdout);
                size_l = snprintf(out, sizeof(out),
                    "\tWe have detected a mismatch between the sampvoice port in server.cfg\n"
                    "\tand the one loaded in the server log!\n"
                    "\t* Please make sure you have correctly set the port in server.cfg.\n");
                fwrite(out, 1, size_l, stdout);
                fflush(stdout);
            }
        }

skip:
        /* RCON password auto-fix for default password vulnerability */
        if (server_rcon_pass) {
            size_l = snprintf(out, sizeof(out),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
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
                  if (!serv_f_cent) { goto dog_skip_fixed; }

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
                        goto dog_skip_fixed;
                    }

                    /* Copy content before the password */
                    strncpy(server_n_content, serv_f_cent, pos - serv_f_cent);
                    server_n_content[pos - serv_f_cent] = '\0';

                    /* Generate secure password using CRC-32 of random number */
                    static int init_crc32 = 0;
                    if (init_crc32 != 1) {
                            crypto_crc32_init_table();
                            init_crc32 = 1;
                    }

                    srand((unsigned int)time(NULL) ^ rand());
                    int rand7 = rand() % 10000000;
                    char size_rand7[DOG_PATH_MAX];
                    snprintf(size_rand7, sizeof(size_rand7), "%d", rand7);
                    crc32_generate = crypto_generate_crc32(size_rand7, sizeof(size_rand7) - 1);

                    /* Format new password line */
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
                              size_l = snprintf(out, sizeof(out), "done! * server.cfg - rcon_password from changeme to %08X.\n", crc32_generate);
                              fwrite(out, 1, size_l, stdout);
                              fflush(stdout);
                      } else {
                              size_l = snprintf(out, sizeof(out), "Error: Cannot write to server.cfg\n");
                              fwrite(out, 1, size_l, stdout);
                              fflush(stdout);
                      }
                      dog_free(server_n_content);
                  } else {
                      size_l = snprintf(out, sizeof(out),
                              "-Replacement failed!\n"
                              " It is not known what the primary cause is."
                              " A reasonable explanation"
                              " is that it occurs when server.cfg does not contain the rcon_password parameter.\n");
                      fwrite(out, 1, size_l, stdout);
                      fflush(stdout);
                  }
                  dog_free(serv_f_cent);
                }
            }
          }
          dog_free(fixed_now);
        }

dog_skip_fixed:
        /* Print closing separator */
        size_l = snprintf(out, sizeof(out),
              "====================================================================\n");
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        /* Cleanup SampVoice port memory */
        if (sampvoice_port) {
            free(sampvoice_port);
            sampvoice_port = NULL;
        }

        /* Offer to install crashdetect plugin if crashes detected but plugin missing */
        if (rate_problem_stat == 1 && server_crashdetect < 1) {
              size_l = snprintf(out, sizeof(out), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? (Auto-fix) ");
              fwrite(out, 1, size_l, stdout);
              fflush(stdout);

              char *confirm;
              confirm = readline("Y/n ");
              if (confirm && (confirm[0] == '\0' || strfind(confirm, "y", true))) {
                  dog_free(confirm);
                  dog_install_depends("Y-Less/samp-plugin-crashdetect?newer", "master", NULL);
              } else {
                  if (confirm) dog_free(confirm);
                  int _dog_crash_ck = path_access(".watchdogs/crashdetect");
                  if (_dog_crash_ck)
                    remove(".watchdogs/crashdetect");
                  return;
              }
        }
}
