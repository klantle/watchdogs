// Copyright (c) 2026 Watchdogs Team and contributors
// All rights reserved. under The 2-Clause BSD License See COPYING or https://opensource.org/license/bsd-2-clause
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
#include "replicate.h"
#include "compiler.h"
#include "debug.h"
#include "runner.h"

/*  source
    ├── archive.c
    ├── archive.h
    ├── cause.c
    ├── cause.h
    ├── compiler.c
    ├── compiler.h
    ├── crypto.c
    ├── crypto.h
    ├── curl.c
    ├── curl.h
    ├── debug.c
    ├── debug.h
    ├── extra.c
    ├── extra.h
    ├── library.c
    ├── library.h
    ├── replicate.c
    ├── replicate.h
    ├── runner.c [x]
    ├── runner.h
    ├── units.c
    ├── units.h
    ├── utils.c
    └── utils.h
*/

int sigint_handler   = 0;
static char command[DOG_MAX_PATH + DOG_PATH_MAX * 2];
static char *cJSON_Data = NULL;
static char *printed = NULL;
static char *sampvoice_port = NULL;
static int rate_sampvoice_server = 0;
static int rate_problem_stat = 0;
static int server_crashdetect = 0;
static int server_rcon_pass = 0;
static FILE *proc_conf_in = NULL;
static FILE *proc_conf_out = NULL;
static cJSON *root = NULL;
static cJSON *pawn = NULL;
static cJSON *msj = NULL;

void try_cleanup_server(void) {
        sigint_handler = 1;
        dog_stop_server_tasks();
        restore_server_config();
        return;
}

void unit_sigint_handler(int sig) {
        try_cleanup_server();

        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);

        FILE *crashdetect_file = NULL;
        crashdetect_file = fopen(".watchdogs/crashdetect", "w");
        if (crashdetect_file != NULL)
            fclose(crashdetect_file);

        #ifdef DOG_ANDROID
        #ifndef _DBG_PRINT
        dog_exec_command("exit && ./*.tmux");
        #else
        dog_exec_command("exit && ./*.debug.tmux");
        #endif
        #elif defined(DOG_LINUX)
        #ifndef _DBG_PRINT
        dog_exec_command("exit && ./watchdogs");
        #else
        dog_exec_command("exit && ./*.debug");
        #endif
        #elif defined(DOG_WINDOWS)
        #ifndef _DBG_PRINT
        dog_exec_command("exit && *.win");
        #else
        dog_exec_command("exit && *.debug.win");
        #endif
        #endif
}

void dog_stop_server_tasks(void) {
        if (dog_server_env() == 1)
          dog_kill_process(dogconfig.dog_toml_binary);
        else if (dog_server_env() == 2)
          dog_kill_process(dogconfig.dog_toml_binary);
}

void dog_display_server_logs(int ret) {
        char *log_file = NULL;
        if (dog_server_env() == 1)
            log_file = dogconfig.dog_toml_logs;
        else if (dog_server_env() == 2)
            log_file = dogconfig.dog_toml_logs;
        dog_printfile(log_file);
        return;
}

void dog_server_crash_check(void) {
        FILE *this_proc_file = NULL;
        if (dog_server_env() == 1)
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");
        else
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");

        if (this_proc_file == NULL) {
            pr_error(stdout, "log file not found!.");
            __create_logging();
            return;
        }

        int needed;
        char output_buf[DOG_MAX_PATH + 26];
        char runner_buffer[DOG_MAX_PATH * 4];

        if (path_exists("crashinfo.txt") != 0)
            {
                pr_info(stdout, "crashinfo.txt detected..");
                char *confirm = readline("-> show? ");
                if (confirm && (confirm[0] == '\0' || confirm[0] == 'Y' || confirm[0] == 'y')) {
                    dog_printfile("crashinfo.txt");
                }
                dog_free(confirm);
            }

        needed = snprintf(output_buf, sizeof(output_buf),
                "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);

        while (fgets(runner_buffer, sizeof(runner_buffer), this_proc_file))
        {
            if (strfind(runner_buffer, "Unable to load filterscript", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Unable to load filterscript detected - Please recompile our filterscripts.\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }

            if (strfind(runner_buffer, "Invalid index parameter (bad entry point)", true)) {
                needed = snprintf(output_buf, sizeof(output_buf),
                    "@ Invalid index parameter (bad entry point) detected - You're forget " FCOLOUR_CYAN "'main'" FCOLOUR_DEFAULT "?\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }

            if (strfind(runner_buffer, "run time error", true) || strfind(runner_buffer, "Run time error", true))
            {
                rate_problem_stat = 1;
                needed = snprintf(output_buf, sizeof(output_buf), "@ Runtime error detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);

                if (strfind(runner_buffer, "division by zero", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Division by zero error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "invalid index", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Invalid index error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
            }

            if (strfind(runner_buffer, "The script might need to be recompiled with the latest include file.", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Needed for recompiled\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);

                char *recompiled = readline("Recompiled scripts now? (Auto-fix)");
                if (recompiled && (recompiled[0] == '\0' || !strcmp(recompiled, "Y") || !strcmp(recompiled, "y"))) {
                    dog_free(recompiled);
                    printf(FCOLOUR_BCYAN "Please input the pawn file\n\t* (enter for %s - input E/e to exit):" FCOLOUR_DEFAULT, dogconfig.dog_toml_proj_input);
                    char *gamemode_compile = readline(" ");
                    if (gamemode_compile && strlen(gamemode_compile) < 1) {
                        const char *args[] = { NULL, ".", NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        dog_exec_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        dog_free(gamemode_compile);
                    } else if (gamemode_compile) {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        dog_exec_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        dog_free(gamemode_compile);
                    }
                } else {
                    dog_free(recompiled);
                }
            }

            if (strfind(runner_buffer, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Filesystem C++ Error Detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                        "\tAre you currently using the WSL ecosystem?\n"
                        "\tYou need to move the open.mp server folder from the /mnt area (your Windows directory) to \"~\" (your WSL HOME).\n"
                        "\tThis is because open.mp C++ filesystem cannot properly read directories inside the /mnt area,\n"
                        "\twhich isn't part of the directory model targeted by the Linux build.\n"
                        "\t* You must run it outside the /mnt area.\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            if (strfind(runner_buffer, "voice server running on port", true)) {
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

            if (strfind(runner_buffer, "I couldn't load any gamemode scripts.", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Can't found gamemode detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
                needed = snprintf(output_buf, sizeof(output_buf),
                        "\tYou need to ensure that the name specified "
                        "in the configuration file matches the one in the gamemodes/ folder,\n"
                        "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                        "\tthen main.amx must be present in the gamemodes/ directory\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }

            if (strfind(runner_buffer, "0x", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Hexadecimal address found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }
            if (strfind(runner_buffer, "address", true) || strfind(runner_buffer, "Address", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory address reference found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }

            if (rate_problem_stat) {
                if (strfind(runner_buffer, "[debug]", true) || strfind(runner_buffer, "crashdetect", true))
                {
                    ++server_crashdetect;
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crashdetect debug information found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);

                    if (strfind(runner_buffer, "AMX backtrace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                        fflush(stdout);
                    }
                    if (strfind(runner_buffer, "native stack trace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native stack trace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                        fflush(stdout);
                    }
                    if (strfind(runner_buffer, "heap", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                        fflush(stdout);
                    }
                    if (strfind(runner_buffer, "[debug]", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Debug Detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                        fflush(stdout);
                    }

                    if (strfind(runner_buffer, "Native backtrace", true)) {
                        needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native backtrace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                        fflush(stdout);

                        if (strfind(runner_buffer, "sampvoice", true)) {
                            if(strfind(runner_buffer, "pawnraknet", true)) {
                                needed = snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crash potent detected\n\t");
                                fwrite(output_buf, 1, needed, stdout);
                                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
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
                                if (downgrading && (downgrading[0] == '\0' || strcmp(downgrading, "Y") == 0 || strcmp(downgrading, "y") == 0)) {
                                    dog_install_depends("CyberMor/sampvoice?v3.0-alpha", "master", NULL);
                                }
                                dog_free(downgrading);
                            }
                        }
                    }
                }

                if (strfind(runner_buffer, "stack", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Stack-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "memory", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Memory-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "access violation", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Access violation detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "buffer overrun", true) || strfind(runner_buffer, "buffer overflow", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Buffer overflow detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "null pointer", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Null pointer exception detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
            }

            if (strfind(runner_buffer, "out of bounds", true) ||
                strfind(runner_buffer, "out-of-bounds", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ out-of-bounds detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
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

            if (dog_server_env() == 1) {
                if (strfind(runner_buffer, "Your password must be changed from the default password", true)) {
                    ++server_rcon_pass;
                }
            }
            if (strfind(runner_buffer, "It needs a gamemode0 line", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Critical message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
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

            if (strfind(runner_buffer, "warning", true) || strfind(runner_buffer, "Warning", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Warning message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }
            if (strfind(runner_buffer, "failed", true) || strfind(runner_buffer, "Failed", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Failure or Failed message detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }
            if (strfind(runner_buffer, "timeout", true) || strfind(runner_buffer, "Timeout", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Timeout event detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }

            if (strfind(runner_buffer, "plugin", true) || strfind(runner_buffer, "Plugin", true)) {
                if (strfind(runner_buffer, "failed to load", true) || strfind(runner_buffer, "Failed.", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Plugin load failure or failed detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
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
                if (strfind(runner_buffer, "unloaded", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Plugin unloaded detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
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

            if (strfind(runner_buffer, "database", true) || strfind(runner_buffer, "mysql", true)) {
                if (strfind(runner_buffer, "connection failed", true) || strfind(runner_buffer, "can't connect", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Database connection failure detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
                if (strfind(runner_buffer, "error", true) || strfind(runner_buffer, "failed", true)) {
                    needed = snprintf(output_buf, sizeof(output_buf), "@ Error or Failed database | mysql found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                    fflush(stdout);
                }
            }

            if (strfind(runner_buffer, "out of memory", true) || strfind(runner_buffer, "memory allocation", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory allocation failure detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }
            if (strfind(runner_buffer, "malloc", true) || strfind(runner_buffer, "free", true) || strfind(runner_buffer, "realloc", true) || strfind(runner_buffer, "calloc", true)) {
                needed = snprintf(output_buf, sizeof(output_buf), "@ Memory management function referenced\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", runner_buffer);
                fflush(stdout);
            }
            fflush(stdout);
        }

        fclose(this_proc_file);

        if (rate_sampvoice_server) {
            if (path_access("server.cfg") == 0)
                goto skip;
            this_proc_file = fopen("server.cfg", "rb");
            if (this_proc_file == NULL)
                goto skip;
            int _sampvoice_port = 0;
            char _p_sampvoice_port[16] = {0};
            while (fgets(runner_buffer, sizeof(runner_buffer), this_proc_file)) {
                if (strfind(runner_buffer, "sv_port", true)) {
                    if (sscanf(runner_buffer, "sv_port %d", &_sampvoice_port) != 1)
                        break;
                    snprintf(_p_sampvoice_port, sizeof(_p_sampvoice_port), "%d", _sampvoice_port);
                    break;
                }
            }
            fclose(this_proc_file);

            if (sampvoice_port && strcmp(_p_sampvoice_port, sampvoice_port) != 0) {
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
        }

skip:
        if (server_rcon_pass) {
            needed = snprintf(output_buf, sizeof(output_buf),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);

            char *fixed_now = readline("Auto-fix? (Y/n): ");

            if (fixed_now && (fixed_now[0] == '\0' || !strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y"))) {
                if (path_access("server.cfg")) {
                    FILE *read_f = fopen("server.cfg", "rb");
                    if (read_f) {
                        fseek(read_f, 0, SEEK_END);
                        long server_fle_size = ftell(read_f);
                        fseek(read_f, 0, SEEK_SET);

                        char *server_f_content = NULL;
                        server_f_content = dog_malloc(server_fle_size + 1);
                        if (!server_f_content) { goto dog_skip_fixed; }

                        size_t bytes_read;
                        bytes_read = fread(server_f_content, 1, server_fle_size, read_f);
                        server_f_content[bytes_read] = '\0';
                        fclose(read_f);

                        char *server_n_content = NULL;
                        char *pos = strstr(server_f_content, "rcon_password changeme");

                        uint32_t crc32_generate;
                        if (pos) {
                                server_n_content = dog_malloc(server_fle_size + 10);
                                if (!server_n_content) {
                                    dog_free(server_f_content);
                                    goto dog_skip_fixed;
                                }

                                strncpy(server_n_content, server_f_content, pos - server_f_content);
                                server_n_content[pos - server_f_content] = '\0';

                                static int init_crc32 = 0;
                                if (init_crc32 != 1) {
                                        crypto_crc32_init_table();
                                        init_crc32 = 1;
                                }

                                srand ( ( unsigned int )time(NULL) ^ rand());
                          			int rand7 = rand () % 10000000;
                                char size_rand7[DOG_PATH_MAX];
                                snprintf(size_rand7, sizeof(size_rand7), "%d", rand7);
                                crc32_generate = crypto_generate_crc32(size_rand7, sizeof(size_rand7) - 1);

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
                                        needed = snprintf(output_buf, sizeof(output_buf), "done! * server.cfg - rcon_password from changeme to %08X.\n", crc32_generate);
                                        fwrite(output_buf, 1, needed, stdout);
                                        fflush(stdout);
                                } else {
                                        needed = snprintf(output_buf, sizeof(output_buf), "Error: Cannot write to server.cfg\n");
                                        fwrite(output_buf, 1, needed, stdout);
                                        fflush(stdout);
                                }
                                dog_free(server_n_content);
                        } else {
                                needed = snprintf(output_buf, sizeof(output_buf),
                                        "-Replacement failed!\n"
                                        " It is not known what the primary cause is."
                                        " A reasonable explanation"
                                        " is that it occurs when server.cfg does not contain the rcon_password parameter.\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                        }
                        dog_free(server_f_content);
                    }
                }
            }
            dog_free(fixed_now);
        }

dog_skip_fixed:
        needed = snprintf(output_buf, sizeof(output_buf),
              "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);

        if (sampvoice_port) {
            free(sampvoice_port);
            sampvoice_port = NULL;
        }

        if (rate_problem_stat == 1 && server_crashdetect < 1) {
              needed = snprintf(output_buf, sizeof(output_buf), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? (Auto-fix) ");
              fwrite(output_buf, 1, needed, stdout);
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

static int update_samp_config(const char *gamemode)
{
        char line[0x400];

        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        if (path_access(size_config))
                remove(size_config);

        if (is_native_windows()) {
            snprintf(command, sizeof(command),
                    "ren %s %s",
                    dogconfig.dog_toml_config,
                    size_config);
        } else {
            snprintf(command, sizeof(command),
                    "mv -f %s %s",
                    dogconfig.dog_toml_config,
                    size_config);
        }

        if (dog_exec_command(command) != __BIT_MASK_NONE) {
                pr_error(stdout, "Failed to create backup file");
                __create_logging();
                return -1;
        }

        #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout, "cannot open backup");
                __create_logging();
                return -1;
        }

        proc_conf_in = fdopen(fd, "r");
        if (!proc_conf_in) {
                close(fd);
                return -1;
        }

        proc_conf_out = fopen(dogconfig.dog_toml_config, "w+");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write new config");
                __create_logging();
                fclose(proc_conf_in);
                return -1;
        }

        char put_gamemode[DOG_PATH_MAX + 0x1A];
        snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *extension = strrchr(put_gamemode, '.');
        if (extension) *extension = '\0';
        gamemode = put_gamemode;

        while (fgets(line, sizeof(line), proc_conf_in)) {
                  if (strfind(line, "gamemode0", true)) {
                      char size_gamemode[DOG_PATH_MAX * 0x2];
                      snprintf(size_gamemode, sizeof(size_gamemode),
                              "gamemode0 %s\n", put_gamemode);
                      fputs(size_gamemode, proc_conf_out);
                      continue;
                  }
                  fputs(line, proc_conf_out);
        }

        fclose(proc_conf_in);
        fclose(proc_conf_out);

        return 1;
}

void restore_server_config(void) {

        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        if (path_exists(size_config) == 0)
            goto restore_done;

        printf(FCOLOUR_GREEN "warning: " FCOLOUR_DEFAULT
                "Continue to restore %s -> %s? y/n",
                size_config, dogconfig.dog_toml_config);

        char *restore_confirm = readline(" ");
        if (restore_confirm && strfind(restore_confirm, "Y", true)) {
                ;
        } else {
                dog_free(restore_confirm);
                unit_ret_main(NULL);
        }

        dog_free(restore_confirm);

        if (path_access(size_config) == 0)
                goto restore_done;

        if (is_native_windows()) {
            snprintf(command, sizeof(command),
            "if exist \"%s\" (del /f /q \"%s\" 2>nul",
            dogconfig.dog_toml_config, dogconfig.dog_toml_config);
        } else {
            snprintf(command, sizeof(command),
            "rm -rf %s",
            dogconfig.dog_toml_config);
        }

        dog_exec_command(command);

        if (is_native_windows())
                snprintf(command, sizeof(command),
                        "ren %s %s",
                        size_config,
                        dogconfig.dog_toml_config);
        else
                snprintf(command, sizeof(command),
                        "mv -f %s %s",
                        size_config,
                        dogconfig.dog_toml_config);

        dog_exec_command(command);

restore_done:
        return;
}

void dog_exec_samp_server(const char *gamemode, const char *server_bin)
{
        __create_logging();

        if (strfind(dogconfig.dog_toml_config, ".json", true))
                return;

        int ret = -1;

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

        dog_sef_fdir_memset_to_null();
        if (dog_sef_fdir(".", gamemode, NULL) == 0) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);
        }

        int ret_c = update_samp_config(gamemode);
        if (ret_c == 0 ||
                ret_c == -1)
                return;

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

        bool runner_retry = false;

        int access_debugging_file = -1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        #ifdef DOG_WINDOWS
                snprintf(command, sizeof(command), "%s", server_bin);
        #else
                snprintf(command, sizeof(command), "./%s", server_bin);
        #endif
        ret = dog_exec_command(command);
        if (ret == 0) {
                end = time(NULL);

                if (!strcmp(dogconfig.dog_os_type, OS_SIGNAL_LINUX)) {
                        printf(FCOLOUR_DEFAULT "\n");
                        printf("~ logging...\n");
                        sleep(0x3);
                        printf(FCOLOUR_BLUE "");
                        dog_display_server_logs(__BIT_MASK_NONE);
                        printf(FCOLOUR_DEFAULT "\n");
                }
        } else {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                if (is_pterodactyl_env())
                        goto server_done;

                elapsed = difftime(end, start);
                if (elapsed <= 4.1 && runner_retry == false) {
                        runner_retry = true;
                        printf("\ttry starting again..");
                        access_debugging_file = path_access(dogconfig.dog_toml_logs);
                        if (access_debugging_file)
                                remove(dogconfig.dog_toml_logs);
                        access_debugging_file = path_access(dogconfig.dog_toml_logs);
                        if (access_debugging_file)
                                remove(dogconfig.dog_toml_logs);
                        end = time(NULL);
                        goto back_start;
                }
        }
        printf(FCOLOUR_DEFAULT "\n");

server_done:
        end = time(NULL);

        if (sigint_handler == 0)
                raise(SIGINT);

        return;
}

static int update_omp_config(const char *gamemode)
{
        struct stat st;
        char gf[DOG_PATH_MAX + 0x1A];
        char put_gamemode[DOG_PATH_MAX + 0x1A];
        int ret = -1;

        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        if (path_access(size_config))
                remove(size_config);

        if (is_native_windows()) {
            snprintf(command, sizeof(command),
                "ren %s %s",
                dogconfig.dog_toml_config,
                size_config);
        } else {
            snprintf(command, sizeof(command),
                "mv -f %s %s",
                dogconfig.dog_toml_config,
                size_config);
        }

        if (dog_exec_command(command) != __BIT_MASK_NONE) {
                pr_error(stdout,
                    "Failed to create backup file");
                __create_logging();
                goto runner_end;
        }

        #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout,
                    "Failed to open %s", size_config);
                __create_logging();
                goto runner_end;
        }

        if (fstat(fd, &st) != 0) {
                pr_error(stdout,
                    "Failed to stat %s", size_config);
                __create_logging();
                close(fd);
                goto runner_end;
        }

        proc_conf_in = fdopen(fd, "rb");
        if (!proc_conf_in) {
                pr_error(stdout, "fdopen failed");
                __create_logging();
                close(fd);
                goto runner_end;
        }

        cJSON_Data = dog_malloc(st.st_size + 1);
        if (!cJSON_Data) {
                goto runner_kill;
        }

        size_t bytes_read;
        bytes_read = fread(cJSON_Data, 1, st.st_size, proc_conf_in);
        if (bytes_read != (size_t)st.st_size) {
                pr_error(stdout,
                    "Incomplete file read (%zu of %ld bytes)",
                    bytes_read, st.st_size);
                __create_logging();
                goto runner_cleanup;
        }

        cJSON_Data[st.st_size] = '\0';
        fclose(proc_conf_in);
        proc_conf_in = NULL;

        root = cJSON_Parse(cJSON_Data);
        if (!root) {
                pr_error(stdout,
                    "JSON parse error: %s", cJSON_GetErrorPtr());
                __create_logging();
                goto runner_end;
        }

        pawn = cJSON_GetObjectItem(root, "pawn");
        if (!pawn) {
                pr_error(stdout,
                    "Missing 'pawn' section in config!");
                __create_logging();
                goto runner_cleanup;
        }

        snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *extension = strrchr(put_gamemode, '.');
        if (extension) *extension = '\0';

        cJSON_DeleteItemFromObject(pawn, "msj");

        msj = cJSON_CreateArray();
        snprintf(gf, sizeof(gf), "%s", put_gamemode);
        cJSON_AddItemToArray(msj, cJSON_CreateString(gf));
        cJSON_AddItemToObject(pawn, "msj", msj);

        proc_conf_out = fopen(dogconfig.dog_toml_config, "w");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write %s", dogconfig.dog_toml_config);
                __create_logging();
                goto runner_end;
        }

        printed = cJSON_Print(root);
        if (!printed) {
                pr_error(stdout,
                    "Failed to print JSON");
                __create_logging();
                goto runner_end;
        }

        if (fputs(printed, proc_conf_out) == EOF) {
                pr_error(stdout,
                    "Failed to write to %s", dogconfig.dog_toml_config);
                __create_logging();
                goto runner_end;
        }

        ret = 0;

runner_end:
        ;
runner_cleanup:
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
runner_kill:
        return ret;
}

void restore_omp_config(void) { restore_server_config(); }

void dog_exec_omp_server(const char *gamemode, const char *server_bin)
{
        __create_logging();

        if (strfind(dogconfig.dog_toml_config, ".cfg", true))
                return;

        int ret = -1;

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

        dog_sef_fdir_memset_to_null();
        if (dog_sef_fdir(".", gamemode, NULL) == 0) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);
        }

        int ret_c = update_omp_config(gamemode);
        if (ret_c == 0 ||
                ret_c == -1)
                return;

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

        bool runner_retry = false;

        int access_debugging_file = -1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        #ifdef DOG_WINDOWS
                snprintf(command, sizeof(command), "%s", server_bin);
        #else
                snprintf(command, sizeof(command), "./%s", server_bin);
        #endif
        ret = dog_exec_command(command);

        if (ret != 0) {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                if (is_pterodactyl_env())
                        goto server_done;

                elapsed = difftime(end, start);
                if (elapsed <= 4.1 && runner_retry == false) {
                        runner_retry = true;
                        printf("\ttry starting again..");
                        access_debugging_file = path_access(dogconfig.dog_toml_logs);
                        if (access_debugging_file)
                                remove(dogconfig.dog_toml_logs);
                        access_debugging_file = path_access(dogconfig.dog_toml_logs);
                        if (access_debugging_file)
                                remove(dogconfig.dog_toml_logs);
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
