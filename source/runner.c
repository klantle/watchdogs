#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "units.h"
#include "extra.h"
#include "utils.h"
#include "crypto.h"
#include "depend.h"
#include "compiler.h"
#include "debug.h"
#include "runner.h"

/* Global variables for server management and configuration handling */
int sigint_handler   = 0;           /* Signal interrupt handling flag */
static char command[WG_MAX_PATH + WG_PATH_MAX * 2]; /* Command buffer */
static char *cJSON_Data = NULL;     /* Raw JSON data buffer */
static char *printed = NULL; /* Pretty-printed JSON string */
static char *sampvoice_port = NULL;  /* Detected SampVoice port */
static int rate_sampvoice_server = 0; /* SampVoice plugin status */
static int rate_problem_stat = 0;      /* General problem detection flag */
static int server_crashdetect = 0; /* CrashDetect plugin detection */
static int server_rcon_pass = 0;   /* RCON password issue detection */
static FILE *proc_conf_in = NULL;      /* Input configuration file pointer */
static FILE *proc_conf_out = NULL;     /* Output configuration file pointer */
static cJSON *root = NULL; /* Root JSON object for server config */
static cJSON *pawn = NULL;           /* Pawn-specific JSON configuration */
static cJSON *msj = NULL;   /* Main script object in JSON */

/*
 * Performs cleanup operations for server processes and configuration restoration.
 * Sets interrupt flag, stops server tasks, and restores original configuration files.
 * Used during graceful shutdown or error recovery scenarios.
 */
void try_cleanup_server(void) {

        sigint_handler = 1;            /* Mark that interrupt is being handled */
        wg_stop_server_tasks();       /* Terminate running server processes */
        restore_server_config();      /* Revert configuration files to original state */
        return;
}

/*
 * Signal handler for SIGINT (Ctrl+C) to gracefully terminate server processes.
 * Initiates cleanup, creates crash detection marker, and restarts the watchdogs
 * interface. Platform-specific restart commands ensure proper process management.
 */
void unit_sigint_handler(int sig) {

        try_cleanup_server();         /* Clean up server processes and configs */

        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer); /* Record termination time */

        /* Create crash detection marker file for debugging */
        FILE *crashdetect_file = NULL;
        crashdetect_file = fopen(".watchdogs/crashdetect", "w");
        if (crashdetect_file != NULL)
            fclose(crashdetect_file);

        /* Platform-specific restart of watchdogs interface */
        #ifdef WG_ANDROID
        #ifndef _DBG_PRINT
        wg_run_command("exit && ./watchdogs.tmux");
        #else
        wg_run_command("exit && ./watchdogs.debug.tmux");
        #endif
        #elif defined(WG_LINUX)
        #ifndef _DBG_PRINT
        wg_run_command("exit && ./watchdogs");
        #else
        wg_run_command("exit && ./watchdogs.debug");
        #endif
        #elif defined(WG_WINDOWS)
        #ifndef _DBG_PRINT
        wg_run_command("exit && watchdogs.win");
        #else
        wg_run_command("exit && watchdogs.debug.win");
        #endif
        #endif
}

/*
 * Stops running server tasks by sending termination signals.
 * Identifies server type (SA-MP or Open.MP) and kills corresponding process.
 * Uses platform-appropriate kill commands for process termination.
 */
void wg_stop_server_tasks(void) {

        if (wg_server_env() == 1)           /* SA-MP server environment */
          wg_kill_process(wgconfig.wg_toml_binary);
        else if (wg_server_env() == 2)      /* Open.MP server environment */
          wg_kill_process(wgconfig.wg_toml_binary);
}

/*
 * Displays server log files to stdout for real-time monitoring.
 * Determines appropriate log file based on server environment
 * and outputs its contents using file printing utility.
 */
void wg_display_server_logs(int ret) {

        char *log_file = NULL;
        if (wg_server_env() == 1)            /* SA-MP log file */
            log_file = wgconfig.wg_toml_logs;
        else if (wg_server_env() == 2)       /* Open.MP log file */
            log_file = wgconfig.wg_toml_logs;
        wg_printfile(log_file);              /* Output log contents */
        return;
}

/*
 * Analyzes server log files to detect crashes, errors, and configuration issues.
 * Performs pattern matching for common error types, provides diagnostics,
 * and offers automated fixes for detected problems. Includes specialized
 * detection for runtime errors, plugin failures, memory issues, and configuration mismatches.
 */
void wg_server_crash_check(void) {

        /* Open server log file for analysis */
        FILE *this_proc_file = NULL;
        if (wg_server_env() == 1)            /* SA-MP server logs */
            this_proc_file = fopen(wgconfig.wg_toml_logs, "rb");
        else                                  /* Open.MP server logs */
            this_proc_file = fopen(wgconfig.wg_toml_logs, "rb");

        if (this_proc_file == NULL) {
            pr_error(stdout, "log file not found!.");
            __debug_function(); /* call debugger function */
            return;
        }

        int needed;                          /* Buffer size calculation */
        char output_buf[WG_MAX_PATH + 26];   /* Formatted output buffer */
        char line_buf[WG_MAX_PATH * 4];      /* Line read buffer */

        /* Print analysis header */
        needed = snprintf(output_buf, sizeof(output_buf),
                "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);

        /* Process log file line by line for pattern matching */
        while (fgets(line_buf, sizeof(line_buf), this_proc_file))
        {
            /* Runtime error detection - common Pawn script errors */
            if (strfind(line_buf, "run time error", true) || strfind(line_buf, "Run time error", true))
            {
                rate_problem_stat = 1;            /* Mark general problem found */
                needed = snprintf(output_buf, sizeof(output_buf), "@ Runtime error detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);

                /* Specific runtime error subtypes */
                if (strfind(line_buf, "division by zero", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Division by zero error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "invalid index", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Invalid index error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
            }

            /* Outdated include file detection requiring recompilation */
            if (strfind(line_buf, "The script might need to be recompiled with the latest include file.", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Needed for recompiled\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);

                /* Interactive recompilation prompt */
                char *recompiled = readline("Recompiled scripts now? (Auto-fix)");
                if (!strcmp(recompiled, "Y") || !strcmp(recompiled, "y")) {
                    wg_free(recompiled);
                        pr_color(stdout, FCOLOUR_CYAN, "Please input the pawn file\n\t* (just enter for %s - input E/e to exit):", wgconfig.wg_toml_proj_input);
                    char *gamemode_compile = readline("Y/n: ");
                    if (strlen(gamemode_compile) < 1) {
                        /* Compile with default configuration */
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                    } else {
                        /* Compile with specified gamemode */
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                    }
                } else { wg_free(recompiled); }
            }

            /* Filesystem error specific to Open.MP on WSL */
            if (strfind(line_buf, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Filesystem C++ Error Detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                        "\tAre you currently using the WSL ecosystem?\n"
                        "\tYou need to move the Open.MP server folder from the /mnt area (your Windows directory) to \"~\" (your WSL HOME).\n"
                        "\tThis is because Open.MP C++ filesystem cannot properly read directories inside the /mnt area,\n"
                        "\twhich isn't part of the directory model targeted by the Linux build.\n"
                        "\t* You must run it outside the /mnt area.\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            /* SampVoice plugin port detection */
            if (strfind(line_buf, "voice server running on port", true)) {
                int _sampvoice_port;
                if (scanf("%*[^v]voice server running on port %d", &_sampvoice_port) != 1)
                    continue;
                ++rate_sampvoice_server;
                sampvoice_port = (char *)&sampvoice_port;
            }

            /* Missing gamemode file detection */
            if (strfind(line_buf, "I couldn't load any gamemode scripts.", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Can't found gamemode detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                        "\tYou need to ensure that the name specified "
                        "in the configuration file matches the one in the gamemodes/ folder,\n"
                        "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                        "\tthen main.amx must be present in the gamemodes/ directory\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            /* Memory address references in logs */
            if (strfind(line_buf, "0x", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Hexadecimal address found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "address", true) || strfind(line_buf, "Address", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory address reference found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }

            /* CrashDetect plugin specific diagnostics */
            if (rate_problem_stat) {
                if (strfind(line_buf, "[debug]", true) || strfind(line_buf, "crashdetect", true))
                {
                    ++server_crashdetect;    /* Count crashdetect references */
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crashdetect debug information found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);

                    /* CrashDetect backtrace analysis */
                    if (strfind(line_buf, "AMX backtrace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "native stack trace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native stack trace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "heap", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "[debug]", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Debug Detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                        fflush(stdout);
                    }

                    /* SampVoice and Pawn.Raknet conflict detection */
                    if (strfind(line_buf, "Native backtrace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native backtrace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                        fflush(stdout);

                        if (strfind(line_buf, "sampvoice", true)) {
                            if(strfind(line_buf, "pawnraknet", true)) {
                                needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crash potent detected\n\t");
                                fwrite(output_buf, 1, needed, stdout);
                                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                                fflush(stdout);
                                needed = snprintf(output_buf, sizeof(output_buf),
                                    "\tWe have detected a crash and identified two plugins as potential causes,\n"
                                    "\tnamely SampVoice and Pawn.Raknet.\n"
                                    "\tAre you using SampVoice version 3.1?\n"
                                    "\tYou can downgrade to SampVoice version 3.0 if necessary,\n"
                                    "\tor you can remove either Sampvoice or Pawn.Raknet to avoid a potential crash.\n"
                                    "\tYou can review the changes between versions 3.0 and 3.1 to understand and analyze the possible reason for the crash\n"
                                    "\ton here: https://github.com/CyberMor/sampvoice/compare/v3.0-alpha...v3.1\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);

                                printf("\x1b[32m==> downgrading sampvoice? 3.1 -> 3.0? (Auto-fix)\x1b[0m\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                                char *downgrading = readline("   answer (y/n): ");
                                if (strcmp(downgrading, "Y") == 0 || strcmp(downgrading, "y")) {
                                    wg_install_depends("CyberMor/sampvoice?v3.0-alpha", "master");
                                }
                            }
                        }
                    }
                }

                /* Memory and stack error patterns */
                if (strfind(line_buf, "stack", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Stack-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "memory", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Memory-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "access violation", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Access violation detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "buffer overrun", true) || strfind(line_buf, "buffer overflow", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Buffer overflow detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "null pointer", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Null pointer exception detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
            }

            /* out-of-bounds checking error */
            if (strfind(line_buf, "out of bounds", true) ||
                strfind(line_buf, "out-of-bounds", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ out-of-bounds detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                    "\tnew array[3];\n"
                    "\tmain() {\n"
                    "\t  for (new i = 0; i < 4; i++) < potent 4 of 3\n"
                    "\t                      ^ sizeof(array)   for array[this] and array[this][]\n"
                    "\t                      ^ sizeof(array[]) for array[][this]\n"
                    "\t                      * instead of manual indexing..\n"
                    "\t     array[i] = 0;\n"
                    "\t}\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            /* SA-MP specific RCON password warning */
            if (wg_server_env() == 1) {
                if (strfind(line_buf, "Your password must be changed from the default password", true)) {
                    ++server_rcon_pass;
                }
            }
            /* SA-MP specific gamemode0 error */
            if (strfind(line_buf, "It needs a gamemode0 line", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Critical message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                        "\tYou need to ensure that the file name (.amx),\n"
                        "\tin your server.cfg under the parameter (gamemode0),\n"
                        "\tactually exists as a .amx file in the gamemodes/ folder.\n"
                        "\tIf there's only a file with the corresponding name but it's only a single .pwn file,\n"
                        "\tyou need to compile it.\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            /* General warning and failure patterns */
            if (strfind(line_buf, "warning", true) || strfind(line_buf, "Warning", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Warning message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "failed", true) || strfind(line_buf, "Failed", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Failure or Failed message detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "timeout", true) || strfind(line_buf, "Timeout", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Timeout event detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }

            /* Plugin loading and management issues */
            if (strfind(line_buf, "plugin", true) || strfind(line_buf, "Plugin", true)) {
                if (strfind(line_buf, "failed to load", true) || strfind(line_buf, "Failed.", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Plugin load failure or failed detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                    needed = snprintf(output_buf, sizeof(output_buf),
                        "\tIf you need to reinstall a plugin that failed, you can use the command:\n"
                        "\t\tinstall user/repo:tags\n"
                        "\tExample:\n"
                        "\t\tinstall Y-Less/sscanf?newer\n"
                        "\tYou can also recheck the username shown on the failed plugin using the command:\n"
                        "\t\ttracker name\n");
                    fwrite(output_buf, 1, needed, stdout);
                    fflush(stdout);
                }
                if (strfind(line_buf, "unloaded", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Plugin unloaded detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                    needed = snprintf(output_buf, sizeof(output_buf),
                        "\tLOADED (Active/In Use):\n"
                        "\t  - Plugin is running, all features are available.\n"
                        "\t  - Utilizing system memory and CPU (e.g., running background threads).\n"
                        "\tUNLOADED (Deactivated/Inactive):\n"
                        "\t  - Plugin has been shut down and removed from memory.\n"
                        "\t  - Features are no longer available; system resources (memory/CPU) are released.\n");
                    fwrite(output_buf, 1, needed, stdout);
                    fflush(stdout);
                }
            }

            /* Database connection issues */
            if (strfind(line_buf, "database", true) || strfind(line_buf, "mysql", true)) {
                if (strfind(line_buf, "connection failed", true) || strfind(line_buf, "can't connect", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Database connection failure detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "error", true) || strfind(line_buf, "failed", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Error or Failed database | mysql found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                    fflush(stdout);
                }
            }

            /* Memory allocation failures */
            if (strfind(line_buf, "out of memory", true) || strfind(line_buf, "memory allocation", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory allocation failure detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "malloc", true) || strfind(line_buf, "free", true) || strfind(line_buf, "realloc", true) || strfind(line_buf, "calloc", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory management function referenced\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", line_buf);
                fflush(stdout);
            }
            fflush(stdout);
        }

        fclose(this_proc_file);  /* Close log file after analysis */

        /* SampVoice port mismatch detection between config and logs */
        if (rate_sampvoice_server) {
            if (path_access("server.cfg") == 0)
                goto skip;  /* Skip if server.cfg doesn't exist */
            this_proc_file = fopen("server.cfg", "rb");
            if (this_proc_file == NULL)
                goto skip;
            int _sampvoice_port = 0;
            char *_p_sampvoice_port = NULL;
            while (fgets(line_buf, sizeof(line_buf), this_proc_file)) {
                if (strfind(line_buf, "sv_port", true)) {
                    if (scanf("sv_port %d", &_sampvoice_port) != 1)
                        break;
                    _p_sampvoice_port = (char *)&_sampvoice_port;
                    break;
                }
            }
            if (strcmp(_p_sampvoice_port, sampvoice_port))
                goto skip;
            needed = snprintf(output_buf, sizeof(output_buf), "@ Sampvoice Port\n\t");
            pr_color(stdout, FCOLOUR_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
            fwrite(output_buf, 1, needed, stdout);
            needed = snprintf(output_buf, sizeof(output_buf),
                "\tWe have detected a mismatch between the sampvoice port in server.cfg\n"
                "\tand the one loaded in the server log!\n"
                "\t* Please make sure you have correctly set the port in server.cfg.\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);
        }

skip:
        /* RCON password fix for default password warning */
        if (server_rcon_pass) {
            needed = snprintf(output_buf, sizeof(output_buf),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);

            char *fixed_now = readline("Auto-fix? (Y/n): ");

            if (!strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y")) {
                if (path_access("server.cfg")) {
                    FILE *read_f = fopen("server.cfg", "rb");
                    if (read_f) {
                        fseek(read_f, 0, SEEK_END);
                        long server_fle_size = ftell(read_f);
                        fseek(read_f, 0, SEEK_SET);

                        char *server_f_content;
                        server_f_content = wg_malloc(server_fle_size + 1);
                        if (!server_f_content) { goto wg_skip_fixed; }

                        size_t bytes_read;
                        bytes_read = fread(server_f_content, 1, server_fle_size, read_f);
                        server_f_content[bytes_read] = '\0';
                        fclose(read_f);

                        char *server_n_content = NULL;
                        char *pos = strstr(server_f_content, "rcon_password changeme");
                        if (pos) {
                                server_n_content = wg_malloc(server_fle_size + 10);
                                if (!server_n_content) { goto wg_skip_fixed; }

                                strncpy(server_n_content, server_f_content, pos - server_f_content);
                                server_n_content[pos - server_f_content] = '\0';

                                static int init_crc32 = 0;
                                if (init_crc32 != 1) {
                                        crypto_crc32_init_table();
                                        init_crc32 = 1;
                                }

                                uint32_t crc32_generate;
                                crc32_generate = crypto_generate_crc32("000d8sK1abC0pqZ7Dd", sizeof("000d8sK1abC0pqZ7Dd") - 1);

                                char crc_str[14 + 11 + 1];
                                sprintf(crc_str, "rcon_password %08X", crc32_generate);

                                strcat(server_n_content, crc_str);
                                strcat(server_n_content, pos + strlen("rcon_password changeme"));
                        }

                        if (server_n_content) {
                                FILE *write_f = fopen("server.cfg", "wb");
                                if (write_f) {
                                        fwrite(server_n_content, 1, strlen(server_n_content), write_f);
                                        fclose(write_f);
                                        needed = snprintf(output_buf, sizeof(output_buf), "done! * server.cfg - rcon_password from changeme to changeme2.\n");
                                        fwrite(output_buf, 1, needed, stdout);
                                        fflush(stdout);
                                } else {
                                        needed = snprintf(output_buf, sizeof(output_buf), "Error: Cannot write to server.cfg\n");
                                        fwrite(output_buf, 1, needed, stdout);
                                        fflush(stdout);
                                }
                                wg_free(server_n_content);
                        } else {
                                needed = snprintf(output_buf, sizeof(output_buf),
                                        "-Replacement failed!\n"
                                        " It is not known what the primary cause is."
                                        " A reasonable explanation"
                                        " is that it occurs when server.cfg does not contain the rcon_password parameter.\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                        }
                        wg_free(server_f_content);
                    }
                }
            }
            wg_free(fixed_now);
        }

wg_skip_fixed:
        /* Analysis footer */
        needed = snprintf(output_buf, sizeof(output_buf),
              "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);

        /* CrashDetect installation prompt if crashes detected but plugin missing */
        if (rate_problem_stat == 1 && server_crashdetect < 1) {
              needed = snprintf(output_buf, sizeof(output_buf), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? (Auto-fix) ");
              fwrite(output_buf, 1, needed, stdout);
              fflush(stdout);

              char *confirm;
              confirm = readline("Y/n ");
              if (strfind(confirm, "y", true)) {
                  wg_free(confirm);
                  wg_install_depends("Y-Less/samp-plugin-crashdetect?newer", "master");
              } else {
                  wg_free(confirm);
                  int _wg_crash_ck = path_access(".watchdogs/crashdetect");
                  if (_wg_crash_ck)
                    remove(".watchdogs/crashdetect");
                  return;
              }
        }
}

/*
 * Updates SA-MP server configuration file (server.cfg) with specified gamemode.
 * Creates backup of original config, modifies gamemode0 line, and writes new config.
 * Handles file renaming and backup preservation for safe modification.
 */
static int update_samp_config(const char *gamemode)
{
        char line[0x400];  /* Line buffer for config processing */

        char size_config[WG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", wgconfig.wg_toml_config);

        /* Remove existing backup if present */
        if (path_access(size_config))
                remove(size_config);

        /* Create backup of original configuration file */
        if (is_native_windows()) {
            snprintf(command, sizeof(command),
                    "ren %s %s",
                    wgconfig.wg_toml_config,
                    size_config);
        } else {
            snprintf(command, sizeof(command),
                    "mv -f %s %s",
                    wgconfig.wg_toml_config,
                    size_config);
        }

        if (wg_run_command(command) != 0x0) {
                pr_error(stdout, "Failed to create backup file");
                __debug_function(); /* call debugger function */
                return -1;
        }

        /* Open bak config */
        #ifdef WG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout, "cannot open backup");
                __debug_function(); /* call debugger function */
                return -1;
        }

        proc_conf_in = fdopen(fd, "r");
        if (!proc_conf_in) {
                close(fd);
                return -1;
        }

        /* Create new configuration file */
        proc_conf_out = fopen(wgconfig.wg_toml_config, "w+");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write new config");
                __debug_function(); /* call debugger function */
                return -1;
        }

        /* Process gamemode filename - remove extension if present */
        char put_gamemode[WG_PATH_MAX + 0x1A];
        snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *extension = strrchr(put_gamemode, '.');
        if (extension) *extension = '\0';
        gamemode = put_gamemode;

        /* Read backup and write new config with modified gamemode */
        while (fgets(line, sizeof(line), proc_conf_in)) {
                  if (strfind(line, "gamemode0", true)) {
                      char size_gamemode[WG_PATH_MAX * 0x2];
                      snprintf(size_gamemode, sizeof(size_gamemode),
                              "gamemode0 %s\n", put_gamemode);
                      fputs(size_gamemode, proc_conf_out);
                      continue;
                  }
                  fputs(line, proc_conf_out);  /* Copy other lines unchanged */
        }

        fclose(proc_conf_in);
        fclose(proc_conf_out);

        return 1;
}

/*
 * Restores original server configuration from backup file.
 * Deletes modified configuration and restores backup with proper file operations.
 * Ensures clean state after temporary configuration changes.
 */
void restore_server_config(void) {

        char size_config[WG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", wgconfig.wg_toml_config);

        printf(FCOLOUR_GREEN "warning: " FCOLOUR_DEFAULT
                "Continue to restore %s -> %s? y/n",
                size_config, wgconfig.wg_toml_config);

        char *restore_confirm = readline(" ");
        if (strfind(restore_confirm, "Y", true)) {
                ;
        } else {
                wg_free(restore_confirm);
                chain_ret_main(NULL);
        }

        wg_free(restore_confirm);

        /* Skip if no backup exists */
        if (path_access(size_config) == 0)
                goto restore_done;

        /* Delete current configuration file */
        if (is_native_windows()) {
            snprintf(command, sizeof(command),
            "if exist \"%s\" (del /f /q \"%s\" 2>nul",
            wgconfig.wg_toml_config, wgconfig.wg_toml_config);
        } else {
            snprintf(command, sizeof(command),
            "rm -rf %s",
            wgconfig.wg_toml_config);
        }

        wg_run_command(command);

        /* Restore backup to original configuration file */
        if (is_native_windows())
                snprintf(command, sizeof(command),
                        "ren %s %s",
                        size_config,
                        wgconfig.wg_toml_config);
        else
                snprintf(command, sizeof(command),
                        "mv -f %s %s",
                        size_config,
                        wgconfig.wg_toml_config);

        wg_run_command(command);

restore_done:
        return;
}

/*
 * Executes SA-MP server with specified gamemode.
 * Updates configuration, sets up signal handling, and launches server process.
 * Includes error handling, gamemode validation, and cleanup routines.
 */
void wg_run_samp_server(const char *gamemode, const char *server_bin)
{
        /* Debug information section */
        __debug_function();

        /* Validate configuration file type */
        if (strfind(wgconfig.wg_toml_config, ".json", true))
                return;

        int ret = -1;

        /* Process gamemode filename to ensure .amx extension */
        char put_gamemode[0x100];
        char *extension = strrchr(gamemode, '.');
        if (extension) {
                size_t len = extension - gamemode;
                snprintf(put_gamemode,
                         sizeof(put_gamemode),
                         "%.*s.amx",
                         (int)len,
                         gamemode);
        } else {
                snprintf(put_gamemode,
                         sizeof(put_gamemode),
                         "%s.amx",
                         gamemode);
        }

        gamemode = put_gamemode;

        /* Search for gamemode file in current directory */
        wg_sef_fdir_memset_to_null();
        if (wg_sef_fdir(".", gamemode, NULL) == 0) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                chain_ret_main(NULL);
        }

        /* Update server configuration with new gamemode */
        int ret_c = update_samp_config(gamemode);
        if (ret_c == 0 ||
                ret_c == -1)
                return;

        /* Set up signal handler for graceful termination */
        struct sigaction sa;

        sa.sa_handler = unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        time_t start, end;
        double elapsed;

        int ret_serv = 0x0;

        int _wg_log_acces = -0x1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        /* Construct and execute server command */
        #ifdef WG_WINDOWS
                snprintf(command, sizeof(command), "%s", server_bin);
        #else
                snprintf(command, sizeof(command), "./%s", server_bin);
        #endif
        ret = wg_run_command(command);
        if (ret == 0) {
                end = time(NULL);

                /* Display server logs for Linux systems */
                if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX)) {
                        printf(FCOLOUR_DEFAULT "\n");
                        printf("~ logging...\n");
                        sleep(0x3);
                        printf(FCOLOUR_BLUE "");
                        wg_display_server_logs(0x0);
                        printf(FCOLOUR_DEFAULT "\n");
                }
        } else {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                /* Skip retry logic in Pterodactyl environments */
                /* so why? -- Pterodactyl hosting doesn't need this,
                *  because when your server restarts in Pterodactyl it will automatically retry
                */
                if (is_pterodactyl_env())
                        goto server_done;

                /* Retry logic with time-based condition */
                elapsed = difftime(end, start);
                if (elapsed <= 5.0 && ret_serv == 0) {
                        ret_serv = 0x1;
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
        printf(FCOLOUR_DEFAULT "\n");

server_done:
        end = time(NULL);

        /* Trigger cleanup if not already handled */
        if (sigint_handler == 0)
                raise(SIGINT);

        return;
}

/*
 * Updates Open.MP server configuration (JSON format) with specified gamemode.
 * Uses cJSON library for JSON manipulation, creates backup, modifies pawn
 * script array, and writes updated configuration. Handles memory allocation
 * and cleanup for JSON operations.
 */
static int update_omp_config(const char *gamemode)
{
        struct stat st;
        char gf[WG_PATH_MAX + 0x1A];
        char put_gamemode[WG_PATH_MAX + 0x1A];
        int ret = -1;

        char size_config[WG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", wgconfig.wg_toml_config);

        /* Remove existing backup */
        if (path_access(size_config))
                remove(size_config);

        /* Create backup of current configuration */
        if (is_native_windows()) {
            snprintf(command, sizeof(command),
                "ren %s %s",
                wgconfig.wg_toml_config,
                size_config);
        } else {
            snprintf(command, sizeof(command),
                "mv -f %s %s",
                wgconfig.wg_toml_config,
                size_config);
        }

        if (wg_run_command(command) != 0x0) {
                pr_error(stdout,
                    "Failed to create backup file");
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        /* Open bak config */
        #ifdef WG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout,
                    "Failed to open %s", size_config);
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        if (fstat(fd, &st) != 0) {
                pr_error(stdout,
                    "Failed to stat %s", size_config);
                __debug_function(); /* call debugger function */
                close(fd);
                goto runner_end;
        }

        proc_conf_in = fdopen(fd, "rb");
        if (!proc_conf_in) {
                pr_error(stdout, "fdopen failed");
                __debug_function(); /* call debugger function */
                close(fd);
                goto runner_end;
        }

        cJSON_Data = wg_malloc(st.st_size + 0x1);
        if (!cJSON_Data) {
                goto runner_kill;
        }

        size_t bytes_read;
        bytes_read = fread(cJSON_Data, 0x1, st.st_size, proc_conf_in);
        if (bytes_read != (size_t)st.st_size) {
                pr_error(stdout,
                    "Incomplete file read (%zu of %ld bytes)",
                    bytes_read, st.st_size);
                __debug_function(); /* call debugger function */
                goto runner_cleanup;
        }

        cJSON_Data[st.st_size] = '\0';
        fclose(proc_conf_in);
        proc_conf_in = NULL;

        /* Parse JSON configuration */
        root = cJSON_Parse(cJSON_Data);
        if (!root) {
                pr_error(stdout,
                    "JSON parse error: %s", cJSON_GetErrorPtr());
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        /* Access pawn section of configuration */
        pawn = cJSON_GetObjectItem(root, "pawn");
        if (!pawn) {
                pr_error(stdout,
                    "Missing 'pawn' section in config!");
                __debug_function(); /* call debugger function */
                goto runner_cleanup;
        }

        /* Process gamemode filename */
        snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *extension = strrchr(put_gamemode, '.');
        if (extension) *extension = '\0';

        /* Update main script array in JSON */
        cJSON_DeleteItemFromObject(pawn, "msj");

        msj = cJSON_CreateArray();
        snprintf(gf, sizeof(gf), "%s", put_gamemode);
        cJSON_AddItemToArray(msj, cJSON_CreateString(gf));
        cJSON_AddItemToObject(pawn, "msj", msj);

        /* Write updated configuration */
        proc_conf_out = fopen(wgconfig.wg_toml_config, "w");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write %s", wgconfig.wg_toml_config);
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        printed = cJSON_Print(root);
        if (!printed) {
                pr_error(stdout,
                    "Failed to print JSON");
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        if (fputs(printed, proc_conf_out) == EOF) {
                pr_error(stdout,
                    "Failed to write to %s", wgconfig.wg_toml_config);
                __debug_function(); /* call debugger function */
                goto runner_end;
        }

        ret = 0;

runner_end:
        ;
runner_cleanup:
        /* Comprehensive cleanup of allocated resources */
        if (proc_conf_out)
                fclose(proc_conf_out);
        if (proc_conf_in)
                fclose(proc_conf_in);
        if (printed)
                wg_free(printed);
        if (root)
                cJSON_Delete(root);
        if (cJSON_Data)
                wg_free(cJSON_Data);
runner_kill:
        return ret;
}

/*
 * Restores Open.MP configuration using generic server config restoration.
 * Alias function for consistency with SA-MP restoration interface.
 */
void restore_omp_config(void) { restore_server_config(); }

/*
 * Executes Open.MP server with specified gamemode.
 * Similar to SA-MP execution but with JSON configuration handling.
 * Includes gamemode validation, configuration updates, and process management.
 */
void wg_run_omp_server(const char *gamemode, const char *server_bin)
{
        /* Debug information section */
        __debug_function();

        /* Validate configuration file type */
        if (strfind(wgconfig.wg_toml_config, ".cfg", true))
                return;

        int ret = -1;

        /* Process gamemode filename */
        char put_gamemode[0x100];
        char *extension = strrchr(gamemode, '.');
        if (extension) {
            size_t len = extension - gamemode;
            snprintf(put_gamemode,
                    sizeof(put_gamemode),
                    "%.*s.amx",
                    (int)len,
                    gamemode);
        } else {
            snprintf(put_gamemode,
                    sizeof(put_gamemode),
                    "%s.amx",
                    gamemode);
        }

        gamemode = put_gamemode;

        /* Verify gamemode file exists */
        wg_sef_fdir_memset_to_null();
        if (wg_sef_fdir(".", gamemode, NULL) == 0) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                chain_ret_main(NULL);
        }

        /* Update Open.MP JSON configuration */
        int ret_c = update_omp_config(gamemode);
        if (ret_c == 0 ||
                ret_c == -1)
                return;

        /* Setup signal handling */
        struct sigaction sa;

        sa.sa_handler = unit_sigint_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
                perror("sigaction");
                exit(EXIT_FAILURE);
        }

        time_t start, end;
        double elapsed;

        int ret_serv = 0x0;

        int _wg_log_acces = -0x1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        /* Construct server execution command */
        #ifdef WG_WINDOWS
                snprintf(command, sizeof(command), "%s", server_bin);
        #else
                snprintf(command, sizeof(command), "./%s", server_bin);
        #endif
        ret = wg_run_command(command);

        if (ret != 0) {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                /* Skip retry logic in Pterodactyl environments */
                /* so why? -- Pterodactyl hosting doesn't need this,
                *  because when your server restarts in Pterodactyl it will automatically retry
                */
                if (is_pterodactyl_env())
                        goto server_done;

                elapsed = difftime(end, start);
                if (elapsed <= 5.0 && ret_serv == 0) {
                        ret_serv = 0x1;
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
        printf(FCOLOUR_DEFAULT "\n");

server_done:
        end = time(NULL);
        if (sigint_handler)
                raise(SIGINT);

        return;
}
