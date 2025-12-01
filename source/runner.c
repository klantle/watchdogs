#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include "units.h"
#include "extra.h"
#include "utils.h"
#include "depends.h"
#include "compiler.h"
#include "runner.h"

int                 handle_sigint   = 0;
FILE                *               config_in = NULL;
FILE                *               config_out = NULL;
cJSON               *               cJSON_server_root = NULL;
cJSON               *               pawn = NULL;
cJSON               *               cJSON_MS_Obj = NULL;
char                *               cJSON_Data = NULL;
char                *               cjsON_PrInted_data = NULL;
char                                r_command[WG_MAX_PATH + WG_PATH_MAX * 2];

void try_cleanup_server(void) {
        handle_sigint = 1;
        wg_stop_server_tasks();
        restore_server_config();
        return;
}

void unit_handle_sigint(int sig) {
        try_cleanup_server();
        struct timespec stop_all_timer;
        clock_gettime(CLOCK_MONOTONIC, &stop_all_timer);
        FILE *crashdetect_file = NULL;
        crashdetect_file = fopen(".watchdogs/crashdetect", "w");
        if (crashdetect_file != NULL)
            fclose(crashdetect_file);
#ifdef __ANDROID__
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

void wg_stop_server_tasks(void) {
        if (wg_server_env() == 1)
          kill_process(wgconfig.wg_toml_binary);
        else if (wg_server_env() == 2)
          kill_process(wgconfig.wg_toml_binary);
}

void wg_display_server_logs(int ret)
{
        char *log_file = NULL;
        if (wg_server_env() == 1)
            log_file = wgconfig.wg_toml_logs;
        else if (wg_server_env() == 2)
            log_file = wgconfig.wg_toml_logs;
        wg_printfile(log_file);
        return;
}

void wg_server_crash_check(void) {
        int problem_stat = 0;
        int server_crashdetect = 0;
        int server_rcon_pass = 0;
        int sampvoice_server_check = 0;
        char *sampvoice_port = NULL;
        
        FILE *proc_f = NULL;
        if (wg_server_env() == 1)
            proc_f = fopen(wgconfig.wg_toml_logs, "rb");
        else
            proc_f = fopen(wgconfig.wg_toml_logs, "rb");

        if (proc_f == NULL) {
            pr_error(stdout, "log file not found!. %s (L: %d)", __func__, __LINE__);
            return;
        }

        int needed;
        char output_buf[WG_MAX_PATH + 26];
        char line_buf[WG_MAX_PATH * 4];
        
        needed = wg_snprintf(output_buf, sizeof(output_buf),
                "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);
        
        while (fgets(line_buf, sizeof(line_buf), proc_f))
        {
            if (strfind(line_buf, "run time error") || strfind(line_buf, "Run time error"))
            {
                problem_stat = 1;
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Runtime error detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);

                if (strfind(line_buf, "division by zero")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Division by zero error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "invalid index")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Invalid index error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "The script might need to be recompiled with the latest include file.")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Needed for recompiled\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                char *recompiled = readline("Recompiled scripts now?");
                if (!strcmp(recompiled, "Y") || !strcmp(recompiled, "y")) {
                    wg_free(recompiled);
                    pr_color(stdout, FCOLOUR_CYAN, "~ pawn file name (press enter for from config toml - enter E/e to exit):");
                    char *gamemode_compile = readline(" ");
                    if (strlen(gamemode_compile) < 1) {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                    } else {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wg_run_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wg_free(gamemode_compile);
                    }
                } else { wg_free(recompiled); }
            }
            if (strfind(line_buf, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Filesystem C++ Error Detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wg_snprintf(output_buf, sizeof(output_buf),
                        "\tAre you currently using the WSL ecosystem?\n"
                        "\tYou need to move the Open.MP server folder from the /mnt area (your Windows directory) to “~” (your WSL HOME).\n"
                        "\tThis is because Open.MP C++ filesystem cannot properly read directories inside the /mnt area,\n"
                        "\twhich isn't part of the directory model targeted by the Linux build.\n"
                        "\t* You must run it outside the /mnt area.\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }
            if (strfind(line_buf, "voice server running on port")) {
                int _sampvoice_port;
                if (scanf("%*[^v]voice server running on port %d", &_sampvoice_port) != 1)
                    continue;
                ++sampvoice_server_check;
                sampvoice_port = (char *)&sampvoice_port;
            }
            if (strfind(line_buf, "I couldn't load any gamemode scripts.")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Can't found gamemode detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wg_snprintf(output_buf, sizeof(output_buf), 
                        "\tYou need to ensure that the name specified "
                        "in the configuration file matches the one in the gamemodes/ folder,\n"
                        "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                        "\tthen main.amx must be present in the gamemodes/ directory\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }
            if (strfind(line_buf, "0x")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Hexadecimal address found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "address") || strfind(line_buf, "Address")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Memory address reference found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (problem_stat) {
                if (strfind(line_buf, "[debug]") || strfind(line_buf, "crashdetect"))
                {
                    ++server_crashdetect;
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crashdetect debug information found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);

                    if (strfind(line_buf, "AMX backtrace")) {
                        needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "native stack trace")) {
                        needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native stack trace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "heap")) {
                        needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "[debug]")) {
                        needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Debug Detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "Native backtrace")) {
                        needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native backtrace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);

                        if (strfind(line_buf, "sampvoice")) {
                            if(strfind(line_buf, "pawnraknet")) {
                                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crash potent detected\n\t");
                                fwrite(output_buf, 1, needed, stdout);
                                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                                fflush(stdout);
                                needed = wg_snprintf(output_buf, sizeof(output_buf),
                                    "\tWe have detected a crash and identified two plugins as potential causes,\n"
                                    "\tnamely SampVoice and Pawn.Raknet.\n"
                                    "\tAre you using SampVoice version 3.1?\n"
                                    "\tYou can downgrade to SampVoice version 3.0 if necessary,\n"
                                    "\tor you can remove either Sampvoice or Pawn.Raknet to avoid a potential crash.\n"
                                    "\tYou can review the changes between versions 3.0 and 3.1 to understand and analyze the possible reason for the crash\n"
                                    "\ton here: https://github.com/CyberMor/sampvoice/compare/v3.0-alpha...v3.1\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);

                                needed = wg_snprintf(output_buf, sizeof(output_buf), "* downgrading sampvoice now? 3.1 -> 3.0 [Y/n]\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                                char *downgrading = readline(" ");
                                if (strcmp(downgrading, "Y") == 0 || strcmp(downgrading, "y")) {
                                    wg_install_depends("CyberMor/sampvoice:v3.0-alpha");
                                }
                            }
                        }
                    }
                }
                if (strfind(line_buf, "stack")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Stack-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "memory")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Memory-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "access violation")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Access violation detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "buffer overrun") || strfind(line_buf, "buffer overflow")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Buffer overflow detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "null pointer")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Null pointer exception detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "out of bounds")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ out of bounds detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wg_snprintf(output_buf, sizeof(output_buf),
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
            if (wg_server_env() == 1) {
                if (strfind(line_buf, "Your password must be changed from the default password")) {
                    ++server_rcon_pass;
                }
            }
            if (strfind(line_buf, "warning") || strfind(line_buf, "Warning")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Warning message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "failed") || strfind(line_buf, "Failed")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Failure or Failed message detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "timeout") || strfind(line_buf, "Timeout")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Timeout event detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "plugin") || strfind(line_buf, "Plugin")) {
                if (strfind(line_buf, "failed to load") || strfind(line_buf, "Failed.")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Plugin load failure or failed detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                    needed = wg_snprintf(output_buf, sizeof(output_buf),
                        "\tIf you need to reinstall a plugin that failed, you can use the command:\n"
                        "\t\tinstall user/repo:tags\n"
                        "\tExample:\n"
                        "\t\tinstall Y-Less/sscanf:latest\n"
                        "\tYou can also recheck the username shown on the failed plugin using the command:\n"
                        "\t\ttracker name\n");
                    fwrite(output_buf, 1, needed, stdout);
                    fflush(stdout);
                }
                if (strfind(line_buf, "unloaded")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Plugin unloaded detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                    needed = wg_snprintf(output_buf, sizeof(output_buf),
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
            if (strfind(line_buf, "database") || strfind(line_buf, "mysql")) {
                if (strfind(line_buf, "connection failed") || strfind(line_buf, "can't connect")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Database connection failure detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "error") || strfind(line_buf, "failed")) {
                    needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Error or Failed database | mysql found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "out of memory") || strfind(line_buf, "memory allocation")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Memory allocation failure detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "malloc") || strfind(line_buf, "free") || strfind(line_buf, "realloc") || strfind(line_buf, "calloc")) {
                needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Memory management function referenced\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            fflush(stdout);
        }

        fclose(proc_f);

        if (sampvoice_server_check) {
            if (path_access("server.cfg") == 0)
                goto skip;
            proc_f = fopen("server.cfg", "rb");
            if (proc_f == NULL)
                goto skip;
            int _sampvoice_port = 0;
            char *_p_sampvoice_port = NULL;
            while (fgets(line_buf, sizeof(line_buf), proc_f)) {
                if (strfind(line_buf, "svport")) {
                    if (scanf("sv_port %d", &_sampvoice_port) != 1)
                        break;
                    _p_sampvoice_port = (char *)&_sampvoice_port;
                    break;
                }
            }
            if (strcmp(_p_sampvoice_port, sampvoice_port))
                goto skip;
            needed = wg_snprintf(output_buf, sizeof(output_buf), "@ Sampvoice Port\n\t");
            pr_color(stdout, FCOLOUR_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
            fwrite(output_buf, 1, needed, stdout);
            needed = wg_snprintf(output_buf, sizeof(output_buf),
                "\tWe have detected a mismatch between the sampvoice port in server.cfg\n"
                "\tand the one loaded in the server log!\n"
                "\t* Please make sure you have correctly set the port in server.cfg.\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);
        }

skip:
        if (server_rcon_pass) {
            needed = wg_snprintf(output_buf, sizeof(output_buf),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);
            
            char *fixed_now = readline("Fixed it now? [Y/n] ");

            if (!strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y")) {
                if (path_access("server.cfg")) {
                    FILE *read_f = fopen("server.cfg", "rb");
                    if (read_f) {
                        fseek(read_f, 0, SEEK_END);
                        long server_file_size = ftell(read_f);
                        fseek(read_f, 0, SEEK_SET);

                        char *server_f_content;
                        server_f_content = wg_malloc(server_file_size + 1);
                        if (server_f_content) {
                            size_t bytes_read;
                            bytes_read = fread(server_f_content, 1, server_file_size, read_f);
                            server_f_content[bytes_read] = '\0';
                            fclose(read_f);

                            char *server_n_content = NULL;
                            char *pos = strstr(server_f_content, "rcon_password changeme");
                            if (pos) {
                                server_n_content = wg_malloc(server_file_size + 10);
                                if (server_n_content) {
                                    wg_strncpy(server_n_content, server_f_content, pos - server_f_content);
                                    server_n_content[pos - server_f_content] = '\0';
                                    strcat(server_n_content, "rcon_password changeme2");
                                    strcat(server_n_content, pos + strlen("rcon_password changeme"));
                                }
                            }

                            if (server_n_content) {
                                FILE *write_f = fopen("server.cfg", "wb");
                                if (write_f) {
                                    fwrite(server_n_content, 1, strlen(server_n_content), write_f);
                                    fclose(write_f);
                                    needed = wg_snprintf(output_buf, sizeof(output_buf), "done! * server.cfg - rcon_password from changeme to changeme2.\n");
                                    fwrite(output_buf, 1, needed, stdout);
                                    fflush(stdout);
                                } else {
                                    needed = wg_snprintf(output_buf, sizeof(output_buf), "Error: Cannot write to server.cfg\n");
                                    fwrite(output_buf, 1, needed, stdout);
                                    fflush(stdout);
                                }
                                wg_free(server_n_content);
                            } else {
                                needed = wg_snprintf(output_buf, sizeof(output_buf), 
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
            }
            wg_free(fixed_now);
        }
        
        needed = wg_snprintf(output_buf, sizeof(output_buf),
              "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);
        
        if (problem_stat == 1 && server_crashdetect < 1) {
              needed = wg_snprintf(output_buf, sizeof(output_buf), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? ");
              fwrite(output_buf, 1, needed, stdout);
              fflush(stdout);
              
              char *confirm;
              confirm = readline("Y/n ");
              if (strfind(confirm, "y")) {
                  wg_free(confirm);
                  wg_install_depends("Y-Less/samp-plugin-crashdetect:latest");
              } else {
                  wg_free(confirm);
                  int _wg_crash_ck = path_access(".watchdogs/crashdetect");
                  if (_wg_crash_ck)
                    remove(".watchdogs/crashdetect");
                  return;
              }
        }
}

static int update_samp_config(const char *gamemode)
{
    FILE *config_in, *config_out;
    char line[0x400];

    char size_config[WG_PATH_MAX];
    wg_snprintf(size_config, sizeof(size_config), ".watchdogs/%s.bak", wgconfig.wg_toml_config);

    if (path_access(size_config))
        remove(size_config);

    if (is_native_windows())
        wg_snprintf(r_command, sizeof(r_command),
            "ren %s %s",
            wgconfig.wg_toml_config,
            size_config);
    else
        wg_snprintf(r_command, sizeof(r_command),
            "mv -f %s %s",
            wgconfig.wg_toml_config,
            size_config);

    if (wg_run_command(r_command) != 0x0) {
        pr_error(stdout, "Failed to create backup file");
        return -1;
    }

    config_in = fopen(size_config, "r");
    if (!config_in) {
        pr_error(stdout, "Failed to open backup config");
        return -1;
    }
    config_out = fopen(wgconfig.wg_toml_config, "w+");
    if (!config_out) {
        pr_error(stdout, "Failed to write new config");
        return -1;
    }

    char put_gamemode[WG_PATH_MAX + 0x1A];
    wg_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
    char *ext = strrchr(put_gamemode, '.');
    if (ext) *ext = '\0';
    gamemode = put_gamemode;

    while (fgets(line, sizeof(line), config_in)) {
          if (strfind(line, "gamemode0")) {
          char size_gamemode[WG_PATH_MAX * 0x2];
          wg_snprintf(size_gamemode, sizeof(size_gamemode),
              "gamemode0 %s\n", put_gamemode);
          fputs(size_gamemode, config_out);
          continue;
          }
          fputs(line, config_out);
    }

    fclose(config_in);
    fclose(config_out);

    return 1;
}

void restore_server_config(void) {
    char size_config[WG_PATH_MAX];
    wg_snprintf(size_config, sizeof(size_config), ".watchdogs/%s.bak", wgconfig.wg_toml_config);

    if (path_access(size_config) == 0)
        goto restore_done;

    if (is_native_windows())
        wg_snprintf(r_command, sizeof(r_command),
        "if exist \"%s\" (del /f /q \"%s\" 2>nul || "
        "rmdir /s /q \"%s\" 2>nul)",
        wgconfig.wg_toml_config, wgconfig.wg_toml_config, wgconfig.wg_toml_config);
    else
        wg_snprintf(r_command, sizeof(r_command),
        "rm -rf %s",
        wgconfig.wg_toml_config);

    wg_run_command(r_command);

    if (is_native_windows())
        wg_snprintf(r_command, sizeof(r_command),
            "ren %s %s",
            size_config,
            wgconfig.wg_toml_config);
    else
        wg_snprintf(r_command, sizeof(r_command),
            "mv -f %s %s",
            size_config,
            wgconfig.wg_toml_config);

    wg_run_command(r_command);

restore_done:
    return;
}
void wg_run_samp_server(const char *gamemode, const char *server_bin)
{
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
    if (strfind(wgconfig.wg_toml_config, ".json"))
        return;

    int ret = -1;

    char put_gamemode[0x100];
    char *ext = strrchr(gamemode, '.');
    if (ext) {
        size_t len = ext - gamemode;
        wg_snprintf(put_gamemode,
             sizeof(put_gamemode),
             "%.*s.amx",
             (int)len,
             gamemode);
    } else {
        wg_snprintf(put_gamemode,
             sizeof(put_gamemode),
             "%s.amx",
             gamemode);
    }

    gamemode = put_gamemode;

    wg_sef_fdir_reset();
    if (wg_sef_fdir(".", gamemode, NULL) == 0) {
        printf("Cannot locate gamemode: ");
        pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
        chain_goto_main(NULL);
    }

    int ret_c = update_samp_config(gamemode);
    if (ret_c == 0 ||
        ret_c == -1)
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

    int ret_serv = 0x0;

    int _wg_log_acces = -0x1;
back_start:
    start = time(NULL);
#ifdef WG_WINDOWS
    wg_snprintf(r_command, sizeof(r_command), "%s", server_bin);
#else
    wg_snprintf(r_command, sizeof(r_command), "./%s", server_bin);
#endif
    end = time(NULL);

    ret = wg_run_command(r_command);
    if (ret == 0) {
        if (!strcmp(wgconfig.wg_os_type, OS_SIGNAL_LINUX)) {
            printf("~ logging...\n");
            sleep(0x3);
            wg_display_server_logs(0x0);
        }
    } else {
        pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
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
            goto back_start;
        }
    }

    if (handle_sigint == 0)
        raise(SIGINT);

    return;
}

static int update_omp_config(const char *gamemode)
{
    struct stat st;
    char gamemode_buf[WG_PATH_MAX + 0x1A];
    char put_gamemode[WG_PATH_MAX + 0x1A];
    int ret = -1;

    char size_config[WG_PATH_MAX];
    wg_snprintf(size_config, sizeof(size_config), ".watchdogs/%s.bak", wgconfig.wg_toml_config);

    if (path_access(size_config))
        remove(size_config);

    if (is_native_windows())
        wg_snprintf(r_command, sizeof(r_command),
            "ren %s %s",
            wgconfig.wg_toml_config,
            size_config);
    else
        wg_snprintf(r_command, sizeof(r_command),
            "mv -f %s %s",
            wgconfig.wg_toml_config,
            size_config);

    if (wg_run_command(r_command) != 0x0) {
        pr_error(stdout, "Failed to create backup file");
        goto runner_end;
    }

    if (stat(size_config, &st) != 0x0) {
        pr_error(stdout, "Failed to get file status");
        goto runner_end;
    }

    config_in = fopen(size_config, "rb");
    if (!config_in) {
        pr_error(stdout, "Failed to open %s", size_config);
        goto runner_end;
    }

    cJSON_Data = wg_malloc(st.st_size + 0x1);
    if (!cJSON_Data) {
        pr_error(stdout, "Memory allocation failed");
        goto runner_cleanup;
    }

    size_t bytes_read;
    bytes_read = fread(cJSON_Data, 0x1, st.st_size, config_in);
    if (bytes_read != (size_t)st.st_size) {
        pr_error(stdout, "Incomplete file read (%zu of %ld bytes)",
            bytes_read,
            st.st_size);
        goto runner_cleanup;
    }

    cJSON_Data[st.st_size] = '\0';
    fclose(config_in);
    config_in = NULL;

    cJSON_server_root = cJSON_Parse(cJSON_Data);
    if (!cJSON_server_root) {
        pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
        goto runner_end;
    }

    pawn = cJSON_GetObjectItem(cJSON_server_root, "pawn");
    if (!pawn) {
        pr_error(stdout, "Missing 'pawn' section in config!");
        goto runner_cleanup;
    }

    wg_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
    char *ext = strrchr(put_gamemode, '.');
    if (ext) *ext = '\0';

    cJSON_DeleteItemFromObject(pawn, "cJSON_MS_Obj");

    cJSON_MS_Obj = cJSON_CreateArray();
    wg_snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", put_gamemode);
    cJSON_AddItemToArray(cJSON_MS_Obj, cJSON_CreateString(gamemode_buf));
    cJSON_AddItemToObject(pawn, "cJSON_MS_Obj", cJSON_MS_Obj);

    config_out = fopen(wgconfig.wg_toml_config, "w");
    if (!config_out) {
        pr_error(stdout, "Failed to write %s", wgconfig.wg_toml_config);
        goto runner_end;
    }

    cjsON_PrInted_data = cJSON_Print(cJSON_server_root);
    if (!cjsON_PrInted_data) {
        pr_error(stdout, "Failed to print JSON");
        goto runner_end;
    }

    if (fputs(cjsON_PrInted_data, config_out) == EOF) {
        pr_error(stdout, "Failed to write to %s", wgconfig.wg_toml_config);
        goto runner_end;
    }

    ret = 0;

runner_end:
    ;
runner_cleanup:
    if (config_out)
        fclose(config_out);
    if (config_in)
        fclose(config_in);
    if (cjsON_PrInted_data)
        wg_free(cjsON_PrInted_data);
    if (cJSON_server_root)
        cJSON_Delete(cJSON_server_root);
    if (cJSON_Data)
        wg_free(cJSON_Data);

    return ret;
}

void restore_omp_config(void) {
    restore_server_config();
}

void wg_run_omp_server(const char *gamemode, const char *server_bin)
{
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
    if (strfind(wgconfig.wg_toml_config, ".cfg"))
        return;

    int ret = -1;

    char put_gamemode[0x100];
    char *ext = strrchr(gamemode, '.');
    if (ext) {
        size_t len = ext - gamemode;
        wg_snprintf(put_gamemode,
            sizeof(put_gamemode),
            "%.*s.amx",
            (int)len,
            gamemode);
    } else {
        wg_snprintf(put_gamemode,
            sizeof(put_gamemode),
            "%s.amx",
            gamemode);
    }

    gamemode = put_gamemode;

    wg_sef_fdir_reset();
    if (wg_sef_fdir(".", gamemode, NULL) == 0) {
        printf("Cannot locate gamemode: ");
        pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
        chain_goto_main(NULL);
    }

    int ret_c = update_omp_config(gamemode);
    if (ret_c == 0 ||
        ret_c == -1)
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

    int ret_serv = 0x0;

    int _wg_log_acces = -0x1;
back_start:
    start = time(NULL);
#ifdef WG_WINDOWS
    wg_snprintf(r_command, sizeof(r_command), "%s", server_bin);
#else
    wg_snprintf(r_command, sizeof(r_command), "./%s", server_bin);
#endif
    end = time(NULL);

    ret = wg_run_command(r_command);
    if (ret != 0) {
        pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
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
            goto back_start;
        }
    }

    if (handle_sigint)
        raise(SIGINT);

    return;
}
