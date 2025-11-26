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
#include "wd_depends.h"
#include "wd_compiler.h"
#include "wd_server.h"

int unit_handle_sigint_status = 0;
FILE *config_in = NULL, *config_out = NULL;
cJSON *cJSON_server_root = NULL, *pawn = NULL, *cJSON_MS_Obj = NULL;
char *cJSON_Data = NULL, *cjsON_PrInted_data = NULL;

int server_mode = 0;
void unit_handle_sigint(int sig) {
        unit_handle_sigint_status = 1;
        wd_stop_server_tasks();
        restore_server_config();
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

void wd_stop_server_tasks(void) {
        if (wd_server_env() == 1)
          kill_process(wcfg.wd_toml_binary);
        else if (wd_server_env() == 2)
          kill_process(wcfg.wd_toml_binary);
}

void wd_display_server_logs(int ret)
{
        char *log_file = NULL;
        if (wd_server_env() == 1)
            log_file = "server_log.txt";
        else if (wd_server_env() == 2)
            log_file = "log.txt";
        wd_printfile(log_file);
        return;
}

void wd_server_crash_check(void) {
        int problem_stat = 0;
        int server_crashdetect = 0;
        int server_rcon_pass = 0;
        int sampvoice_server_check = 0;
        char *sampvoice_port = NULL;
        
        FILE *proc_f = NULL;
        if (wd_server_env() == 1)
            proc_f = fopen("server_log.txt", "rb");
        else
            proc_f = fopen("log.txt", "rb");

        if (proc_f == NULL) {
            pr_error(stdout, "log file not found!. %s (L: %d)", __func__, __LINE__);
            return;
        }

        int needed;
        char output_buf[WD_MAX_PATH + 26];
        char line_buf[WD_MAX_PATH * 4];
        
        needed = wd_snprintf(output_buf, sizeof(output_buf),
                "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);
        
        while (fgets(line_buf, sizeof(line_buf), proc_f))
        {
            if (strfind(line_buf, "run time error") || strfind(line_buf, "Run time error"))
            {
                problem_stat = 1;
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Runtime error detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);

                if (strfind(line_buf, "division by zero")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Division by zero error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "invalid index")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Invalid index error found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "The script might need to be recompiled with the latest include file.")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Needed for recompiled\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                char *recompiled = readline("recompiled now?");
                if (!strcmp(recompiled, "Y") || !strcmp(recompiled, "y")) {
                    wd_free(recompiled);
                    pr_color(stdout, FCOLOUR_CYAN, "~ pawn file name (press enter for from config toml - enter E/e to exit):");
                    char *gamemode_compile = readline(" ");
                    if (strlen(gamemode_compile) < 1) {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wd_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wd_free(gamemode_compile);
                    } else {
                        const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
                        wd_run_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
                        wd_free(gamemode_compile);
                    }
                } else { wd_free(recompiled); }
            }
            if (strfind(line_buf, "terminate called after throwing an instance of 'ghc::filesystem::filesystem_error")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Filesystem C++ Error Detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wd_snprintf(output_buf, sizeof(output_buf),
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
                scanf("%*[^v]voice server running on port %d", &_sampvoice_port);
                ++sampvoice_server_check;
                sampvoice_port = (char *)&sampvoice_port;
            }
            if (strfind(line_buf, "I couldn't load any gamemode scripts.")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Can't found gamemode detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wd_snprintf(output_buf, sizeof(output_buf), 
                        "\tYou need to ensure that the name specified "
                        "in the configuration file matches the one in the gamemodes/ folder,\n"
                        "\tand that the .amx file exists. For example, if server.cfg contains gamemode0 main,\n"
                        "\tthen main.amx must be present in the gamemodes/ directory\n");
                fwrite(output_buf, 1, needed, stdout);
                fflush(stdout);
            }
            if (strfind(line_buf, "0x")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Hexadecimal address found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "address") || strfind(line_buf, "Address")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Memory address reference found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (problem_stat) {
                if (strfind(line_buf, "[debug]") || strfind(line_buf, "crashdetect"))
                {
                    ++server_crashdetect;
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crashdetect debug information found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);

                    if (strfind(line_buf, "AMX backtrace")) {
                        needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: AMX backtrace detected in crash log\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "native stack trace")) {
                        needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native stack trace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "heap")) {
                        needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Heap-related issue mentioned\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "[debug]")) {
                        needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Debug Detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);
                    }
                    if (strfind(line_buf, "Native backtrace")) {
                        needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Native backtrace detected\n\t");
                        fwrite(output_buf, 1, needed, stdout);
                        pr_color(stdout, FCOLOUR_BLUE, line_buf);
                        fflush(stdout);

                        if (strfind(line_buf, "sampvoice")) {
                            if(strfind(line_buf, "pawnraknet")) {
                                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Crashdetect: Crash potent detected\n\t");
                                fwrite(output_buf, 1, needed, stdout);
                                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                                fflush(stdout);
                                needed = wd_snprintf(output_buf, sizeof(output_buf),
                                    "\tWe have detected a crash and identified two plugins as potential causes,\n"
                                    "\tnamely SampVoice and Pawn.Raknet.\n"
                                    "\tAre you using SampVoice version 3.1?\n"
                                    "\tYou can downgrade to SampVoice version 3.0 if necessary,\n"
                                    "\tor you can remove either Sampvoice or Pawn.Raknet to avoid a potential crash.\n"
                                    "\tYou can review the changes between versions 3.0 and 3.1 to understand and analyze the possible reason for the crash\n"
                                    "\ton here: https://github.com/CyberMor/sampvoice/compare/v3.0-alpha...v3.1\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);

                                needed = wd_snprintf(output_buf, sizeof(output_buf), "* downgrading sampvoice now? 3.1 -> 3.0 [Y/n]\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                                char *downgrading = readline(" ");
                                if (strcmp(downgrading, "Y") == 0 || strcmp(downgrading, "y")) {
                                    wd_install_depends("CyberMor/sampvoice:v3.0-alpha");
                                }
                            }
                        }
                    }
                }
                if (strfind(line_buf, "stack")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Stack-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "memory")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Memory-related issue detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "access violation")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Access violation detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "buffer overrun") || strfind(line_buf, "buffer overflow")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Buffer overflow detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "null pointer")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Null pointer exception detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "out of bounds")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ out of bounds detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
                needed = wd_snprintf(output_buf, sizeof(output_buf),
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
            if (wd_server_env() == 1) {
                if (strfind(line_buf, "Your password must be changed from the default password")) {
                    ++server_rcon_pass;
                }
            }
            if (strfind(line_buf, "warning") || strfind(line_buf, "Warning")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Warning message found\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "failed") || strfind(line_buf, "Failed")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Failure or Failed message detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "timeout") || strfind(line_buf, "Timeout")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Timeout event detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "plugin") || strfind(line_buf, "Plugin")) {
                if (strfind(line_buf, "failed to load") || strfind(line_buf, "Failed.")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Plugin load failure or failed detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                    needed = wd_snprintf(output_buf, sizeof(output_buf),
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
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Plugin unloaded detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                    needed = wd_snprintf(output_buf, sizeof(output_buf),
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
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Database connection failure detected\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
                if (strfind(line_buf, "error") || strfind(line_buf, "failed")) {
                    needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Error or Failed database | mysql found\n\t");
                    fwrite(output_buf, 1, needed, stdout);
                    pr_color(stdout, FCOLOUR_BLUE, line_buf);
                    fflush(stdout);
                }
            }
            if (strfind(line_buf, "out of memory") || strfind(line_buf, "memory allocation")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Memory allocation failure detected\n\t");
                fwrite(output_buf, 1, needed, stdout);
                pr_color(stdout, FCOLOUR_BLUE, line_buf);
                fflush(stdout);
            }
            if (strfind(line_buf, "malloc") || strfind(line_buf, "free") || strfind(line_buf, "realloc") || strfind(line_buf, "calloc")) {
                needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Memory management function referenced\n\t");
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
                    scanf("sv_port %d", &_sampvoice_port);
                    _p_sampvoice_port = (char *)&_sampvoice_port;
                    break;
                }
            }
            if (strcmp(_p_sampvoice_port, sampvoice_port))
                goto skip;
            needed = wd_snprintf(output_buf, sizeof(output_buf), "@ Sampvoice Port\n\t");
            pr_color(stdout, FCOLOUR_BLUE, "in server.cfg: %s in server logs: %s", _p_sampvoice_port, sampvoice_port);
            fwrite(output_buf, 1, needed, stdout);
            needed = wd_snprintf(output_buf, sizeof(output_buf),
                "\tWe have detected a mismatch between the sampvoice port in server.cfg\n"
                "\tand the one loaded in the server log!\n"
                "\t* Please make sure you have correctly set the port in server.cfg.\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);
        }

skip:
        if (server_rcon_pass) {
            needed = wd_snprintf(output_buf, sizeof(output_buf),
              "@ Rcon Pass Error found\n\t* Error: Your password must be changed from the default password..\n");
            fwrite(output_buf, 1, needed, stdout);
            fflush(stdout);
            
            char *fixed_now = readline("fixed now? [Y/n] ");

            if (!strcmp(fixed_now, "Y") || !strcmp(fixed_now, "y")) {
                if (path_access("server.cfg")) {
                    FILE *read_f = fopen("server.cfg", "rb");
                    if (read_f) {
                        fseek(read_f, 0, SEEK_END);
                        long server_file_size = ftell(read_f);
                        fseek(read_f, 0, SEEK_SET);

                        char *server_f_content;
                        server_f_content = wd_malloc(server_file_size + 1);
                        if (server_f_content) {
                            size_t bytes_read;
                            bytes_read = fread(server_f_content, 1, server_file_size, read_f);
                            server_f_content[bytes_read] = '\0';
                            fclose(read_f);

                            char *server_n_content = NULL;
                            char *pos = strstr(server_f_content, "rcon_password changeme");
                            if (pos) {
                                server_n_content = wd_malloc(server_file_size + 10);
                                if (server_n_content) {
                                    wd_strncpy(server_n_content, server_f_content, pos - server_f_content);
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
                                    needed = wd_snprintf(output_buf, sizeof(output_buf), "done! * server.cfg - rcon_password from changeme to changeme2.\n");
                                    fwrite(output_buf, 1, needed, stdout);
                                    fflush(stdout);
                                } else {
                                    needed = wd_snprintf(output_buf, sizeof(output_buf), "Error: Cannot write to server.cfg\n");
                                    fwrite(output_buf, 1, needed, stdout);
                                    fflush(stdout);
                                }
                                wd_free(server_n_content);
                            } else {
                                needed = wd_snprintf(output_buf, sizeof(output_buf), 
                                       "-Replacement failed!\n"
                                       " It is not known what the primary cause is."
                                       " A reasonable explanation"
                                       " is that it occurs when server.cfg does not contain the rcon_password parameter.\n");
                                fwrite(output_buf, 1, needed, stdout);
                                fflush(stdout);
                            }
                            wd_free(server_f_content);
                        }
                    }
                }
            }
            wd_free(fixed_now);
        }
        
        needed = wd_snprintf(output_buf, sizeof(output_buf),
              "====================================================================\n");
        fwrite(output_buf, 1, needed, stdout);
        fflush(stdout);
        
        if (problem_stat == 1 && server_crashdetect < 1) {
              needed = wd_snprintf(output_buf, sizeof(output_buf), "INFO: crash found! "
                     "and crashdetect not found.. "
                     "install crashdetect now? ");
              fwrite(output_buf, 1, needed, stdout);
              fflush(stdout);
              
              char *confirm;
              confirm = readline("Y/n ");
              if (strfind(confirm, "y")) {
                  wd_free(confirm);
                  wd_install_depends("Y-Less/samp-plugin-crashdetect:latest");
              } else {
                  wd_free(confirm);
                  int _wd_crash_ck = path_access(".wd_crashdetect");
                  if (_wd_crash_ck)
                    remove(".wd_crashdetect");
                  return;
              }
        }
}

static int update_samp_config(const char *gamemode)
{
        FILE *config_in, *config_out;
        char line[1024];

        char size_config[WD_PATH_MAX];
        wd_snprintf(size_config, sizeof(size_config), ".%s.bak", wcfg.wd_toml_config);

        if (path_access(size_config))
            remove(size_config);

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

        if (wd_run_command(size_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -WD_RETN;
        }

        config_in = fopen(size_config, "r");
        if (!config_in) {
                pr_error(stdout, "Failed to open backup config");
                return -WD_RETN;
        }
        config_out = fopen(wcfg.wd_toml_config, "w+");
        if (!config_out) {
                pr_error(stdout, "Failed to write new config");
                fclose(config_out);
                return -WD_RETN;
        }

        char put_gamemode[WD_PATH_MAX + 26];
        wd_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *f_EXT = strrchr(put_gamemode, '.');
        if (f_EXT) *f_EXT = '\0';
        gamemode = put_gamemode;

        while (fgets(line, sizeof(line), config_in)) {
              if (strfind(line, "gamemode0")) {
                  char size_gamemode[WD_PATH_MAX * 2];
                  wd_snprintf(size_gamemode, sizeof(size_gamemode),
                      "gamemode0 %s\n", put_gamemode);
                  fputs(size_gamemode, config_out);
                  continue;
              }
              fputs(line, config_out);
        }

        fclose(config_in);
        fclose(config_out);

        return WD_RETN;
}

void restore_server_config(void) {
        char size_config[WD_PATH_MAX];
        wd_snprintf(size_config, sizeof(size_config), ".%s.bak", wcfg.wd_toml_config);

        if (path_access(size_config) == 0)
            goto done;

        char size_command[WD_PATH_MAX + WD_PATH_MAX + 26];

        if (is_native_windows())
            wd_snprintf(size_command, sizeof(size_command),
                "if exist \"%s\" (del /f /q \"%s\" 2>nul || "
                "rmdir /s /q \"%s\" 2>nul)",
                wcfg.wd_toml_config, wcfg.wd_toml_config, wcfg.wd_toml_config);
        else
            wd_snprintf(size_command, sizeof(size_command),
                "rm -rf %s",
                wcfg.wd_toml_config);

        wd_run_command(size_command);

        if (is_native_windows())
            wd_snprintf(size_command, sizeof(size_command),
                        "ren %s %s",
                        size_config,
                        wcfg.wd_toml_config);
        else
            wd_snprintf(size_command, sizeof(size_command),
                        "mv -f %s %s",
                        size_config,
                        wcfg.wd_toml_config);

        wd_run_command(size_command);

done:
        return;
}

void wd_run_samp_server(const char *gamemode, const char *server_bin)
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
        if (strfind(wcfg.wd_toml_config, ".json"))
                return;

        int ret = -WD_RETN;
        char command[WD_PATH_MAX];

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

        gamemode = put_gamemode;

        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) == WD_RETZ) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                start_chain(NULL);
        }

        int ret_c = update_samp_config(gamemode);
        if (ret_c == WD_RETZ ||
            ret_c == -WD_RETN)
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

        int _wd_log_acces = -1;
back_start:
        start = time(NULL);
#ifdef WD_WINDOWS
        wd_snprintf(command, sizeof(command), "%s", server_bin);
#else
        wd_snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);

        ret = wd_run_command(command);
        if (ret == WD_RETZ) {
                if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
                        printf("~ logging...\n");
                        sleep(3);
                        wd_display_server_logs(0);
                }
        } else {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);
                if (elapsed <= 5.0 && ret_serv == 0) {
                    ret_serv = 1;
                    printf("\ttry starting again..");
                    _wd_log_acces = path_access("server_log.txt");
                    if (_wd_log_acces)
                      remove("server_log.txt");
                    _wd_log_acces = path_access("log.txt");
                    if (_wd_log_acces)
                      remove("log.txt");
                    goto back_start;
                }
        }

        if (unit_handle_sigint_status == 0)
            raise(SIGINT);

        return;
}

static int update_omp_config(const char *gamemode)
{
        struct stat st;
        char gamemode_buf[WD_PATH_MAX + 26];
        char put_gamemode[WD_PATH_MAX + 26];
        int ret = -WD_RETN;

        char size_config[WD_PATH_MAX];
        wd_snprintf(size_config, sizeof(size_config), ".%s.bak", wcfg.wd_toml_config);

        if (path_access(size_config))
            remove(size_config);

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

        if (wd_run_command(size_mv) != 0) {
                pr_error(stdout, "Failed to create backup file");
                return -WD_RETN;
        }

        if (stat(size_config, &st) != 0) {
                pr_error(stdout, "Failed to get file status");
                return -WD_RETN;
        }

        config_in = fopen(size_config, "rb");
        if (!config_in) {
                pr_error(stdout, "Failed to open %s", size_config);
                return -WD_RETN;
        }

        cJSON_Data = wd_malloc(st.st_size + 1);
        if (!cJSON_Data) {
                pr_error(stdout, "Memory allocation failed");
                goto done;
        }

        size_t bytes_read;
        bytes_read = fread(cJSON_Data, 1, st.st_size, config_in);
        if (bytes_read != (size_t)st.st_size) {
                pr_error(stdout, "Incomplete file read (%zu of %ld bytes)",
                        bytes_read,
                        st.st_size);
                goto done;
        }

        cJSON_Data[st.st_size] = '\0';
        fclose(config_in);
        config_in = NULL;

        cJSON_server_root = cJSON_Parse(cJSON_Data);
        if (!cJSON_server_root) {
                pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
                goto done;
        }

        pawn = cJSON_GetObjectItem(cJSON_server_root, "pawn");
        if (!pawn) {
                pr_error(stdout, "Missing 'pawn' section in config!");
                goto done;
        }

        wd_snprintf(put_gamemode, sizeof(put_gamemode), "%s", gamemode);
        char *f_EXT = strrchr(put_gamemode, '.');
        if (f_EXT) *f_EXT = '\0';

        cJSON_DeleteItemFromObject(pawn, "cJSON_MS_Obj");

        cJSON_MS_Obj = cJSON_CreateArray();
        wd_snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", put_gamemode);
        cJSON_AddItemToArray(cJSON_MS_Obj, cJSON_CreateString(gamemode_buf));
        cJSON_AddItemToObject(pawn, "cJSON_MS_Obj", cJSON_MS_Obj);

        config_out = fopen(wcfg.wd_toml_config, "w");
        if (!config_out) {
                pr_error(stdout, "Failed to write %s", wcfg.wd_toml_config);
                goto done;
        }

        cjsON_PrInted_data = cJSON_Print(cJSON_server_root);
        if (!cjsON_PrInted_data) {
                pr_error(stdout, "Failed to print JSON");
                goto done;
        }

        if (fputs(cjsON_PrInted_data, config_out) == EOF) {
                pr_error(stdout, "Failed to write to %s", wcfg.wd_toml_config);
                goto done;
        }

        ret = WD_RETZ;

done:
        if (config_out)
                fclose(config_out);
        if (config_in)
                fclose(config_in);
        if (cjsON_PrInted_data)
                wd_free(cjsON_PrInted_data);
        if (cJSON_server_root)
                cJSON_Delete(cJSON_server_root);
        if (cJSON_Data)
                wd_free(cJSON_Data);

        return ret;
}

void restore_omp_config(void) {
        restore_server_config();
}

void wd_run_omp_server(const char *gamemode, const char *server_bin)
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
        if (strfind(wcfg.wd_toml_config, ".cfg"))
                return;

        int ret = -WD_RETN;
        char command[WD_PATH_MAX];

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

        gamemode = put_gamemode;

        wd_sef_fdir_reset();
        if (wd_sef_fdir(".", gamemode, NULL) == WD_RETZ) {
                printf("Cannot locate gamemode: ");
                pr_color(stdout, FCOLOUR_CYAN, "%s\n", gamemode);
                start_chain(NULL);
        }

        int ret_c = update_omp_config(gamemode);
        if (ret_c == WD_RETZ ||
            ret_c == -WD_RETN)
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

        int _wd_log_acces = -1;
back_start:
        start = time(NULL);
#ifdef WD_WINDOWS
        wd_snprintf(command, sizeof(command), "%s", server_bin);
#else
        wd_snprintf(command, sizeof(command), "./%s", server_bin);
#endif
        end = time(NULL);

        ret = wd_run_command(command);
        if (ret != WD_RETZ) {
                pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
                elapsed = difftime(end, start);
                if (elapsed <= 5.0 && ret_serv == 0) {
                    ret_serv = 1;
                    printf("\ttry starting again..");
                    _wd_log_acces = path_access("server_log.txt");
                    if (_wd_log_acces)
                      remove("server_log.txt");
                    _wd_log_acces = path_access("log.txt");
                    if (_wd_log_acces)
                      remove("log.txt");
                    goto back_start;
                }
        }

        if (unit_handle_sigint_status == 0)
            raise(SIGINT);

        return;
}