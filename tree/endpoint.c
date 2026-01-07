/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>

#include "units.h"
#include "extra.h"
#include "utils.h"
#include "crypto.h"
#include "replicate.h"
#include "compiler.h"
#include "debug.h"
#include "endpoint.h"

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
}

void dog_stop_server_tasks(void) {
        if (dog_server_env() == 1)
          dog_kill_process(dogconfig.dog_toml_binary);
        else if (dog_server_env() == 2)
          dog_kill_process(dogconfig.dog_toml_binary);
}

void dog_server_crash_check(void) {
        FILE *this_proc_file = NULL;
        if (dog_server_env() == 1)
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");
        else
            this_proc_file = fopen(dogconfig.dog_toml_logs, "rb");

        if (this_proc_file == NULL) {
            pr_error(stdout, "log file not found!.");
            minimal_debugging();
            return;
        }

        int size_l;
        char out[DOG_MAX_PATH + 26];
        char buf[DOG_MAX_PATH * 4];

        if (path_exists("crashinfo.txt") != 0)
            {
                pr_info(stdout, "crashinfo.txt detected..");
                char *confirm = readline("-> show? ");
                if (confirm && (confirm[0] == '\0' || confirm[0] == 'Y' || confirm[0] == 'y')) {
                    dog_printfile("crashinfo.txt");
                }
                dog_free(confirm);
            }

        size_l = snprintf(out, sizeof(out),
                "====================================================================\n");
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        while (fgets(buf, sizeof(buf), this_proc_file))
        {
        if (strfind(buf, "Unable to load filterscript", true)) {
            size_l = snprintf(out, sizeof(out), "@ Unable to load filterscript detected - Please recompile our filterscripts.\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }

        if (strfind(buf, "Invalid index parameter (bad entry point)", true)) {
            size_l = snprintf(out, sizeof(out),
                "@ Invalid index parameter (bad entry point) detected - You're forget " FCOLOUR_CYAN "'main'" FCOLOUR_DEFAULT "?\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }

        if (strfind(buf, "run time error", true) || strfind(buf, "Run time error", true))
        {
            rate_problem_stat = 1;
            size_l = snprintf(out, sizeof(out), "@ Runtime error detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);

            if (strfind(buf, "division by zero", true)) {
                size_l = snprintf(out, sizeof(out), "@ Division by zero error found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "invalid index", true)) {
                size_l = snprintf(out, sizeof(out), "@ Invalid index error found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
        }

        if (strfind(buf, "The script might need to be recompiled with the latest include file.", true)) {
            size_l = snprintf(out, sizeof(out), "@ Needed for recompiled\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);

            char *recompiled = readline("Recompiled script now? (Auto-fix)");
            if (recompiled && (recompiled[0] == '\0' || !strcmp(recompiled, "Y") || !strcmp(recompiled, "y"))) {
                dog_free(recompiled);
                printf(FCOLOUR_BCYAN "Please input the pawn file\n\t* (enter for %s - input E/e to exit):" FCOLOUR_DEFAULT, dogconfig.dog_toml_proj_input);
                char *gamemode_compile = readline(" ");
                if (gamemode_compile && strlen(gamemode_compile) < 1) {
                    const char *args[] = { NULL, ".", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                    dog_exec_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
                    dog_free(gamemode_compile);
                } else if (gamemode_compile) {
                    const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                    dog_exec_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
                    dog_free(gamemode_compile);
                }
            } else {
                dog_free(recompiled);
            }
        }

        if (strfind(buf, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error", true)) {
            size_l = snprintf(out, sizeof(out), "@ Filesystem C++ Error Detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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

        if (strfind(buf, "I couldn't load any gamemode scripts.", true)) {
            size_l = snprintf(out, sizeof(out), "@ Can't found gamemode detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
            size_l = snprintf(out, sizeof(out),
                    "\tYou need to ensure that the name specified "
                    "in the configuration file matches the one in the gamemodes/ folder,\n"
                    "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                    "\tthen main.amx must be present in the gamemodes/ directory\n");
            fwrite(out, 1, size_l, stdout);
            fflush(stdout);
        }

        if (strfind(buf, "0x", true)) {
            size_l = snprintf(out, sizeof(out), "@ Hexadecimal address found\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }
        if (strfind(buf, "address", true) || strfind(buf, "Address", true)) {
            size_l = snprintf(out, sizeof(out), "@ Memory address reference found\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }

        if (rate_problem_stat) {
            if (strfind(buf, "[debug]", true) || strfind(buf, "crashdetect", true))
            {
                ++server_crashdetect;
                size_l = snprintf(out, sizeof(out), "@ Crashdetect: Crashdetect debug information found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);

                if (strfind(buf, "AMX backtrace", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "native stack trace", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: Native stack trace detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "heap", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: Heap-related issue mentioned\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                    fflush(stdout);
                }
                if (strfind(buf, "[debug]", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: Debug Detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                    fflush(stdout);
                }

                if (strfind(buf, "Native backtrace", true)) {
                    size_l = snprintf(out, sizeof(out), "@ Crashdetect: Native backtrace detected\n\t");
                    fwrite(out, 1, size_l, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                    fflush(stdout);

                    if (strfind(buf, "sampvoice", true)) {
                        if(strfind(buf, "pawnraknet", true)) {
                            size_l = snprintf(out, sizeof(out), "@ Crashdetect: Crash potent detected\n\t");
                            fwrite(out, 1, size_l, stdout);
                            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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

            if (strfind(buf, "stack", true)) {
                size_l = snprintf(out, sizeof(out), "@ Stack-related issue detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "memory", true)) {
                size_l = snprintf(out, sizeof(out), "@ Memory-related issue detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "access violation", true)) {
                size_l = snprintf(out, sizeof(out), "@ Access violation detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "buffer overrun", true) || strfind(buf, "buffer overflow", true)) {
                size_l = snprintf(out, sizeof(out), "@ Buffer overflow detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "null pointer", true)) {
                size_l = snprintf(out, sizeof(out), "@ Null pointer exception detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
        }

        if (strfind(buf, "out of bounds", true) ||
            strfind(buf, "out-of-bounds", true)) {
            size_l = snprintf(out, sizeof(out), "@ out-of-bounds detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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

        if (dog_server_env() == 1) {
            if (strfind(buf, "Your password must be changed from the default password", true)) {
                ++server_rcon_pass;
            }
        }
        if (strfind(buf, "It needs a gamemode0 line", true)) {
            size_l = snprintf(out, sizeof(out), "@ Critical message found\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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

        if (strfind(buf, "warning", true) || strfind(buf, "Warning", true)) {
            size_l = snprintf(out, sizeof(out), "@ Warning message found\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }
        if (strfind(buf, "failed", true) || strfind(buf, "Failed", true)) {
            size_l = snprintf(out, sizeof(out), "@ Failure or Failed message detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }
        if (strfind(buf, "timeout", true) || strfind(buf, "Timeout", true)) {
            size_l = snprintf(out, sizeof(out), "@ Timeout event detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }

        if (strfind(buf, "plugin", true) || strfind(buf, "Plugin", true)) {
            if (strfind(buf, "failed to load", true) || strfind(buf, "Failed.", true)) {
                size_l = snprintf(out, sizeof(out), "@ Plugin load failure or failed detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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

        if (strfind(buf, "database", true) || strfind(buf, "mysql", true)) {
            if (strfind(buf, "connection failed", true) || strfind(buf, "can't connect", true)) {
                size_l = snprintf(out, sizeof(out), "@ Database connection failure detected\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
            if (strfind(buf, "error", true) || strfind(buf, "failed", true)) {
                size_l = snprintf(out, sizeof(out), "@ Error or Failed database | mysql found\n\t");
                fwrite(out, 1, size_l, stdout);
                pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
                fflush(stdout);
            }
        }

        if (strfind(buf, "out of memory", true) || strfind(buf, "memory allocation", true)) {
            size_l = snprintf(out, sizeof(out), "@ Memory allocation failure detected\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
            fflush(stdout);
        }
        if (strfind(buf, "malloc", true) || strfind(buf, "free", true) || strfind(buf, "realloc", true) || strfind(buf, "calloc", true)) {
            size_l = snprintf(out, sizeof(out), "@ Memory management function referenced\n\t");
            fwrite(out, 1, size_l, stdout);
            pr_color(stdout, FCOLOUR_BLUE, "%s", buf);
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
            while (fgets(buf, sizeof(buf), this_proc_file)) {
                if (strfind(buf, "sv_port", true)) {
                    if (sscanf(buf, "sv_port %d", &_sampvoice_port) != 1)
                        break;
                    snprintf(_p_sampvoice_port, sizeof(_p_sampvoice_port), "%d", _sampvoice_port);
                    break;
                }
            }
            fclose(this_proc_file);

            if (sampvoice_port && strcmp(_p_sampvoice_port, sampvoice_port) != 0) {
                size_l = snprintf(out, sizeof(out), "@ Sampvoice Port\n\t");
                pr_color(stdout, FCOLOUR_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
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
        if (server_rcon_pass) {
            size_l = snprintf(out, sizeof(out),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            fwrite(out, 1, size_l, stdout);
            fflush(stdout);

            char *fixed_now = readline("Auto-fix? (Y/n): ");

            if (fixed_now && (fixed_now[0] == '\0' || !strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y"))) {
              if (path_access("server.cfg")) {
                FILE *read_f = fopen("server.cfg", "rb");
                if (read_f) {
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

                  char *server_n_content = NULL;
                  char *pos = strstr(serv_f_cent, "rcon_password changeme");

                  uint32_t crc32_generate;
                  if (pos) {
                    server_n_content = dog_malloc(server_fle_size + 10);
                    if (!server_n_content) {
                        dog_free(serv_f_cent);
                        goto dog_skip_fixed;
                    }

                    strncpy(server_n_content, serv_f_cent, pos - serv_f_cent);
                    server_n_content[pos - serv_f_cent] = '\0';

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
        size_l = snprintf(out, sizeof(out),
              "====================================================================\n");
        fwrite(out, 1, size_l, stdout);
        fflush(stdout);

        if (sampvoice_port) {
            free(sampvoice_port);
            sampvoice_port = NULL;
        }

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

static int update_samp_config(const char *g)
{
        char line[0x400];

        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        if (path_access(size_config))
                remove(size_config);

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
        #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout, "cannot open backup");
                minimal_debugging();
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
                minimal_debugging();
                fclose(proc_conf_in);
                return -1;
        }

        char puts[DOG_PATH_MAX + 0x1A];
        snprintf(puts, sizeof(puts), "%s", g);
        char *e = strrchr(puts, '.');
        if (e) *e = '\0';
        g = puts;

        while (fgets(line, sizeof(line), proc_conf_in)) {
                  if (strfind(line, "gamemode0", true)) {
                      char size_gamemode[DOG_PATH_MAX * 0x2];
                      snprintf(size_gamemode, sizeof(size_gamemode),
                              "gamemode0 %s\n", puts);
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

        #ifdef DOG_WINDOWS
            DWORD attr = GetFileAttributesA(dogconfig.dog_toml_config);
            if (attr != INVALID_FILE_ATTRIBUTES &&
                !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                DeleteFileA(dogconfig.dog_toml_config);
            }
        #else
            unlink(dogconfig.dog_toml_config);
        #endif

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

void dog_exec_samp_server(const char *g, const char *server_bin)
{
        minimal_debugging();

        if (strfind(dogconfig.dog_toml_config, ".json", true))
                return;

        int ret = -1;

        char puts[0x100];
        char *e = strrchr(g, '.');
        if (e) {
                size_t len = e - g;
                snprintf(puts,
                         sizeof(puts),
                         "%.*s.amx",
                         (int)len,
                         g);
        } else {
                snprintf(puts,
                         sizeof(puts),
                         "%s.amx",
                         g);
        }

        g = puts;

        dog_sef_restore();

        if (dog_sef_fdir(".", g, NULL) == 0) {
                printf("Cannot locate g: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", g);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);
        }

        int ret_c = update_samp_config(g);
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
        double elp;

        bool rty = false;

        int _access = -1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        #ifdef DOG_WINDOWS
            STARTUPINFOA _STARTUPINFO;
            PROCESS_INFORMATION _PROCESS_INFO;
            
            char command[DOG_PATH_MAX * 2];

            ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
            ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

            _STARTUPINFO.cb = sizeof(_STARTUPINFO);
            _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

            _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
            _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

            if (is_pterodactyl_env())
                snprintf(command, sizeof(command),
                    "wine64 ./%s", server_bin);
            else
                snprintf(command, sizeof(command),
                    "%s%s", SYM_PROG, server_bin);

            if (!CreateProcessA(
                    NULL,
                    command,
                    NULL,
                    NULL,
                    TRUE,
                    0,
                    NULL,
                    NULL,
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
        if (ret == 0) {
                end = time(NULL);
        } else {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                if (is_pterodactyl_env())
                        goto server_done;

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
        printf(FCOLOUR_DEFAULT "\n");

server_done:
        end = time(NULL);

        if (sigint_handler == 0)
                raise(SIGINT);

        return;
}

static int update_omp_config(const char *g)
{
        struct stat st;
        char gf[DOG_PATH_MAX + 0x1A];
        char puts[DOG_PATH_MAX + 0x1A];
        int ret = -1;

        char size_config[DOG_PATH_MAX];
        snprintf(size_config, sizeof(size_config),
                ".watchdogs/%s.bak", dogconfig.dog_toml_config);

        if (path_access(size_config))
                remove(size_config);

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

        #ifdef DOG_WINDOWS
        int fd = open(size_config, O_RDONLY);
        #else
        int fd = open(size_config, O_RDONLY | O_NOFOLLOW);
        #endif
        if (fd < 0) {
                pr_error(stdout,
                    "Failed to open %s", size_config);
                minimal_debugging();
                goto endpoint_end;
        }

        if (fstat(fd, &st) != 0) {
                pr_error(stdout,
                    "Failed to stat %s", size_config);
                minimal_debugging();
                close(fd);
                goto endpoint_end;
        }

        proc_conf_in = fdopen(fd, "rb");
        if (!proc_conf_in) {
                pr_error(stdout, "fdopen failed");
                minimal_debugging();
                close(fd);
                goto endpoint_end;
        }

        cJSON_Data = dog_malloc(st.st_size + 1);
        if (!cJSON_Data) {
                goto endpoint_kill;
        }

        size_t br;
        br = fread(cJSON_Data, 1, st.st_size, proc_conf_in);
        if (br != (size_t)st.st_size) {
                pr_error(stdout,
                    "Incomplete file read (%zu of %ld bytes)",
                    br, st.st_size);
                minimal_debugging();
                goto endpoint_cleanup;
        }

        cJSON_Data[st.st_size] = '\0';
        fclose(proc_conf_in);
        proc_conf_in = NULL;

        root = cJSON_Parse(cJSON_Data);
        if (!root) {
                pr_error(stdout,
                    "JSON parse error: %s", cJSON_GetErrorPtr());
                minimal_debugging();
                goto endpoint_end;
        }

        pawn = cJSON_GetObjectItem(root, "pawn");
        if (!pawn) {
                pr_error(stdout,
                    "Missing 'pawn' section in config!");
                minimal_debugging();
                goto endpoint_cleanup;
        }

        snprintf(puts, sizeof(puts), "%s", g);
        char *e = strrchr(puts, '.');
        if (e) *e = '\0';

        cJSON_DeleteItemFromObject(pawn, "msj");

        msj = cJSON_CreateArray();
        snprintf(gf, sizeof(gf), "%s", puts);
        cJSON_AddItemToArray(msj, cJSON_CreateString(gf));
        cJSON_AddItemToObject(pawn, "msj", msj);

        proc_conf_out = fopen(dogconfig.dog_toml_config, "w");
        if (!proc_conf_out) {
                pr_error(stdout,
                    "Failed to write %s", dogconfig.dog_toml_config);
                minimal_debugging();
                goto endpoint_end;
        }

        printed = cJSON_Print(root);
        if (!printed) {
                pr_error(stdout,
                    "Failed to print JSON");
                minimal_debugging();
                goto endpoint_end;
        }

        if (fputs(printed, proc_conf_out) == EOF) {
                pr_error(stdout,
                    "Failed to write to %s", dogconfig.dog_toml_config);
                minimal_debugging();
                goto endpoint_end;
        }

        ret = 0;

endpoint_end:
        ;
endpoint_cleanup:
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

void restore_omp_config(void) { restore_server_config(); }

void dog_exec_omp_server(const char *g, const char *server_bin)
{
        minimal_debugging();

        if (strfind(dogconfig.dog_toml_config, ".cfg", true))
                return;

        int ret = -1;

        char puts[0x100];
        char *e = strrchr(g, '.');
        if (e) {
            size_t len = e - g;
            snprintf(puts,
                    sizeof(puts),
                    "%.*s.amx",
                    (int)len,
                    g);
        } else {
            snprintf(puts,
                    sizeof(puts),
                    "%s.amx",
                    g);
        }

        g = puts;

        dog_sef_restore();

        if (dog_sef_fdir(".", g, NULL) == 0) {
                printf("Cannot locate g: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", g);
                pr_info(stdout, "Check first, Compile first.");
                unit_ret_main(NULL);
        }

        int ret_c = update_omp_config(g);
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
        double elp;

        bool rty = false;

        int _access = -1;
back_start:
        start = time(NULL);
        printf(FCOLOUR_BLUE "");
        #ifdef DOG_WINDOWS
            STARTUPINFOA _STARTUPINFO;
            PROCESS_INFORMATION _PROCESS_INFO;
            
            char command[DOG_PATH_MAX * 2];

            ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
            ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

            _STARTUPINFO.cb = sizeof(_STARTUPINFO);
            _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

            _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
            _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

            if (is_pterodactyl_env())
                snprintf(command, sizeof(command),
                    "wine64 ./%s", server_bin);
            else
                snprintf(command, sizeof(command),
                    "%s%s", SYM_PROG, server_bin);

            if (!CreateProcessA(
                    NULL,
                    command,
                    NULL,
                    NULL,
                    TRUE,
                    0,
                    NULL,
                    NULL,
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
        if (ret != 0) {
                printf(FCOLOUR_DEFAULT "\n");
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");

                if (is_pterodactyl_env())
                        goto server_done;

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
        printf(FCOLOUR_DEFAULT "\n");

server_done:
        end = time(NULL);
        if (sigint_handler)
                raise(SIGINT);

        return;
}
