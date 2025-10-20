/*
 * What is this?:
 * ------------------------------------------------------------
 * This program serves as a comprehensive management tool for running,
 * compiling, and controlling multiplayer game servers, specifically
 * SA-MP (San Andreas Multiplayer) and Open.MP servers, along with
 * Pawn / PawnCC compilation for gamemodes. It provides an interactive
 * command-line interface to execute tasks such as starting,
 * stopping, compiling, debugging, and installing server projects
 * and compilers.
 *
 *
 * Script Algorithm:
 * ------------------------------------------------------------
 * 1. Initialize the program:
 *      - Load configuration from TOML files.
 *      - Set up user command history (via readline/history).
 *      - Reset internal state variables and directories.
 * 2. Enter main loop (`__init_wd()`):
 *      - Display prompt (`watchdogs > `).
 *      - Read user input, add to history, parse command.
 *      - Detect OS type (Windows, Linux, Termux) to adapt binary names & permissions.
 *      - Check presence of server binaries (samp-server / omp-server).
 * 3. Command handling for built-in commands:
 *      - `help`    : Show available commands and usage.
 *      - `exit`    : Quit the watchdogs program.
 *      - `clear`   : Clear the terminal screen.
 *      - `kill`    : Restart the session (clears screen, reinitialises).
 *      - `title`   : Set the terminal window title.
 *      - `compile` : Compile gamemodes using PawnCC with configured paths.
 *      - `running` : Start a SA-MP or Open.MP server (standard mode).
 *      - `debug`   : Start a server in debug mode.
 *      - `stop`    : Stop all running servers/tasks.
 *      - `restart` : Stop then restart servers/tasks.
 *      - `pawncc`  : Install or run the Pawn compiler for a selected platform.
 *      - `gamemode`: Install or run gamemodes for a selected platform.
 * 4. For compilation (`compile` command):
 *      - Read `watchdogs.toml` config to get compiler options, include paths,
 *        gamemode input/output filenames.
 *      - Build full compiler command (via `snprintf`), log output to
 *        `.wd_compiler.log`.
 *      - Measure compilation duration (using `clock_gettime(CLOCK_MONOTONIC,…)`).
 *      - After compile, inspect log and delete output binary if errors found.
 * 5. For server launch commands (`running` / `debug`):
 *      - Use OS-specific binary names for server (e.g., `samp03svr` vs `samp-server.exe`).
 *      - Backup config (e.g., `server.cfg` or `config.json`), adjust gamemode/script path,
 *        set execute permissions (`chmod` / `_chmod`).
 *      - Launch server (e.g., via `watchdogs_sys()` wrapper).
 *      - In interactive mode, after start wait for user to press Enter to display logs
 *        (e.g., `server_log.txt` or `log.txt`).
 *      - On completion or stop, restore original config backup.
 *
 *
 * Script Logic:
 * ------------------------------------------------------------
 * Key Functions:
 *
 * > `__init()`:
 *    - Set up `SIGINT` handler to `handle_sigint`.
 *    - Loop infinitely by calling `__init_wd()`.
 *
 * > `__init_wd()`:
 *    - Calls `__init_function()` which sets up title, loads config, user history,
 *      resets variables and directories.
 *    - Shows command prompt, reads input, adds to history.
 *    - Detects OS, determines server binary names.
 *    - Locates server binaries for SA-MP and Open.MP.
 *    - Parses user command and maps to handler:
 *         * `help`, `exit`, `clear`, `kill`, `title`
 *         * `pawncc`, `gamemode`
 *         * `compile` — invokes compilation logic
 *         * `running` / `debug` — invokes server launch logic
 *         * `stop`, `restart` — server control logic
 *    - If unknown command but close match detected (via `__find_c_command()`),
 *      suggests the closest command.
 *
 * > Compilation flow (`compile`):
 *    - Determine Pawn compiler binary based on OS.
 *    - Parse `watchdogs.toml`:
 *         * Compiler `option`
 *         * Compiler `output` filename
 *         * Include paths array
 *         * Input gamemode file
 *
 * > Server run / debug flow (`running` / `debug`):
 *    - If SA-MP binary available:
 *         * If no gamemode argument or dot (.), reset logs, set permissions, run `./samp-server`.
 *         * Else call `watchdogs_server_samp(gamemode, server_bin)`.
 *    - If Open.MP binary available:
 *         * Similar logic with `./omp-server` and `watchdogs_server_openmp()`.
 *    - If neither binary found, prompt user to install via `watch_samp()` or `watch_samp("windows"/"linux")`.
 *    - On debug mode, internal variable `server_or_debug` set to `"debug"`.
 *
 * > `watchdogs_server_samp(const char *gamemode, const char *server_bin)`:
 *    - Backup `server.cfg`.
 *    - Update gamemode line in `server.cfg` to selected gamemode file.
 *    - Set executable permission for server binary.
 *    - Launch SA-MP server, then wait for user interaction to view `server_log.txt`.
 *    - On exit restore `server.cfg` backup.
 *
 * > `watchdogs_server_openmp(const char *gamemode, const char *server_bin)`:
 *    - Backup `config.json`.
 *    - Modify `main_scripts` array to include selected gamemode.
 *    - Set executable permission for server binary.
 *    - Launch Open.MP server, then let user view `log.txt` after run.
 *    - Restore `config.json` backup.
 *
 * > `watch_pawncc(platform)`:
 *    - Install or run PawnCC compiler for the selected platform (linux/windows/termux).
 *    - Handles user prompt selection, calls `watch_pawncc("linux")`, etc.
 *
 * > `watch_samp(platform)`:
 *    - Install SA-MP server binaries for specified platform (linux/windows).
 *
 *
 * How to Use?:
 * ------------------------------------------------------------
 * 1. Compile the program along with dependencies (`watchdogs.h`, `color.h`, `utils.h`,
 *    `crypto.h`, `archive.h`, `curl.h`, etc.).
 * 2. Execute binary:
 *      - On Linux/Unix: `./watchdogs`
 *      - On Windows: `watchdogs.exe`
 * 3. Use the interactive prompt at `watchdogs > `:
 *      - `help` : list commands
 *      - `compile` : build gamemodes
 *      - `pawncc` / `gamemode` : install / select platform
 *      - `running` / `debug` : run or debug a server
 *      - `stop` / `restart` : manage servers
 * 4. Ensure that gamemodes, server binaries and config files exist in working directory.
 * 5. In debug or run mode, you will be prompted (e.g., “Press enter to print logs..”) to view the server log.
 * 6. Program handles OS differences automatically (permissions, binary names, signals).
 *
 * Notes:
 * ------------------------------------------------------------
 * - Compatible with Windows, Linux and Termux environments.
 * - Uses backups for config files to prevent overwriting originals.
 * - Sets executable permissions on server binaries when needed.
 * - Provides detailed log outputs for compile & runtime errors.
 * - Scanner searches for “error” keyword in compile log to detect failure.
 * - On unknown command, suggests the closest known command if within distance threshold.
 * - Uses `readline()` + `history` for better interactive user experience.
 */
#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif 

/* debugging */
#define DEBUGGING_COMPILER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path) // override POSIX mkdir
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#else
#include <unistd.h>
#define PATH_SEP "/"
#endif

#include <limits.h>
#include <ncursesw/curses.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <ftw.h>
#include <sys/file.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "utils.h"
#include "crypto.h"
#include "package.h"
#include "archive.h"
#include "curl.h"
#include "watchdogs.h"
#include "server.h"

void __init_function(void) {
        watchdogs_title(NULL);
        watchdogs_toml_data();
        watchdogs_u_history();
        watchdogs_reset_var();
        reset_watchdogs_sef_dir();
}

int __init_wd(void)
{
        __init_function();
        char *ptr_command = readline("watchdogs > ");
        if (!ptr_command) return -1;
        watchdogs_a_history(ptr_command);

        int c_distance = INT_MAX;
        const char *_dist_command = __find_c_command(ptr_command,
                                                     __command,
                                                     __command_len,
                                                     &c_distance);

        static const char
            *ptr_samp = NULL;
        static int
            find_for_samp = 0x0;
        static const char
            *ptr_openmp = NULL;
        static int
            find_for_omp = 0x0;

        static int __watchdogs_os__;
            __watchdogs_os__ = signal_system_os();
        
        if (__watchdogs_os__ == 0x01) {
            ptr_samp="samp-server.exe"; ptr_openmp="omp-server.exe";
        }
        else if (__watchdogs_os__ == 0x00) {
            ptr_samp="samp03svr"; ptr_openmp="omp-server";
        }
        
        FILE *file_s = fopen(ptr_samp, "r");
        if (file_s) {
            find_for_samp = 0x1;
            fclose(file_s);
        }

        FILE *file_m = fopen(ptr_openmp, "r");
        if (file_m) {
            find_for_omp = 0x1;
            fclose(file_m);
        }

        if (strncmp(ptr_command, "help", 4) == 0) {
            watchdogs_title("Watchdogs | @ help");

            static char *arg;
                arg = ptr_command + 4;
            while (*arg == ' ') arg++;

            if (strlen(arg) == 0) {
                println("usage: help | help [<cmds>]");
                println("cmds:");
                println(" clear, exit, kill, title");
                println(" gamemode, pawncc");
                println(" compile, running, debug");
                println(" stop, restart");
            } else if (strcmp(arg, "exit") == 0) { println("exit: exit from watchdogs. | Usage: \"exit\"");
            } else if (strcmp(arg, "clear") == 0) { println("clear: clear screen watchdogs. | Usage: \"clear\"");
            } else if (strcmp(arg, "kill") == 0) { println("kill: kill - restart terminal watchdogs. | Usage: \"kill\"");
            } else if (strcmp(arg, "title") == 0) { println("title: set-title Terminal watchdogs. | Usage: \"title\" | [<args>]");
            } else if (strcmp(arg, "compile") == 0) { println("compile: compile your project. | Usage: \"compile\" | [<args>]");
            } else if (strcmp(arg, "running") == 0) { println("running: running your project. | Usage: \"running\" | [<args>]");
            } else if (strcmp(arg, "debug") == 0) { println("debug: debugging your project. | Usage: \"debug\" | [<args>]");
            } else if (strcmp(arg, "stop") == 0) { println("stop: stopped server task. | Usage: \"stop\"");
            } else if (strcmp(arg, "restart") == 0) { println("restart: restart server task. | Usage: \"restart\"");
            } else println("help not found for: '%s'", arg);
            
            return 0;
        } else if (strcmp(ptr_command, "pawncc") == 0) {
            watchdogs_title("Watchdogs | @ pawncc");
            static
                char platform = 0;
            ret_pcc:
                println("Select platform:");
                println("[L/l] Linux");
                println("[W/w] Windows");
                println("[T/t] Termux");
                printf(">> ");

            if (scanf(" %c", &platform) != 1)
                return 0;

            signal(SIGINT, __init);

            if (platform == 'L' || platform == 'l')
                watch_pawncc("linux");
            else if (platform == 'W' || platform == 'w')
                watch_pawncc("windows");
            else if (platform == 'T' || platform == 't')
                watch_pawncc("termux");
            else {
                printf("Invalid platform selection. use C^ to exit.\n");
                goto ret_pcc;
            }
        } else if (strcmp(ptr_command, "gamemode") == 0) {
            watchdogs_title("Watchdogs | @ gamemode");
            static
                char platform = 0;
            ret_gm:
                println("Select platform:");
                println("[L/l] Linux");
                println("[W/w] Windows");
                printf(">> ");

            if (scanf(" %c", &platform) != 1)
                return 0;

            signal(SIGINT, __init);

            if (platform == 'L' || platform == 'l')
                watch_samp("linux");
            else if (platform == 'W' || platform == 'w')
                watch_samp("windows");
            else {
                printf("Invalid platform selection. use C^ to exit.\n");
                goto ret_gm;
            }
        } else if (strcmp(ptr_command, "clear") == 0) {
            watchdogs_title("Watchdogs | @ clear");
            watchdogs_sys("clear");
            return 0;
        } else if (strcmp(ptr_command, "exit") == 0) {
            exit(1);
        } else if (strcmp(ptr_command, "kill") == 0) {
            watchdogs_title("Watchdogs | @ kill");
            watchdogs_sys("clear");
            __init(0);
        } else if (strncmp(ptr_command, "title", 5) == 0) {
            char *arg = ptr_command + 6;
            while (*arg == ' ') arg++;
            if (*arg == '\0') {
                println("usage: title [<title>]");
            } else {
                printf("\033]0;%s\007", arg);
            }
            return 0;
        } else if (strncmp(ptr_command, "compile", 7) == 0) {
            watchdogs_title("Watchdogs | @ compile");
            static char *arg;
            arg = ptr_command + 7;
            while (*arg == ' ') arg++;
        
            static char *compile_args;
            compile_args = strtok(arg, " ");
        
            const char *ptr_pawncc;
            if (__watchdogs_os__ == 0x01) 
                ptr_pawncc = "pawncc.exe";
            else if (__watchdogs_os__ == 0x00)
                ptr_pawncc = "pawncc";

            if (access(".wd_compiler.log", F_OK) == 0) {
                remove(".wd_compiler.log");
            }
            
            FILE *procc_f = NULL;
            char error_compiler_check[100];
            char watchdogs_c_output_f_container[128];
            int format_size_c_f_container = sizeof(watchdogs_c_output_f_container);
            
            procc_f = fopen(".wd_compiler.log", "r");
            if (procc_f) {
                int ch;
                while ((ch = fgetc(procc_f)) != EOF) putchar(ch);
                rewind(procc_f);
                while (fscanf(procc_f, "%s", error_compiler_check) != EOF) {
                    if (strcmp(error_compiler_check, "error") == 0) {
                        if (access(watchdogs_c_output_f_container, F_OK) == 0)
                            remove(watchdogs_c_output_f_container);
                        break;
                    }
                }
                fclose(procc_f);
            }

            int find_pawncc = watchdogs_sef_fdir(".", ptr_pawncc);
            if (find_pawncc) {
                size_t format_size_compiler = 2048;
                char *_compiler_ = malloc(format_size_compiler);
                if (!_compiler_) {
                    printf_error("Memory allocation failed for _compiler_!\n");
                    return 0;
                }

                FILE *procc_f = fopen("watchdogs.toml", "r");
                if (!procc_f) {
                    printf_error("Can't read file %s\n", "watchdogs.toml");
                    if (_compiler_) {
                        free(_compiler_);
                    }
                    return 0;
                }
                
                char errbuf[256];
                toml_table_t *config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
                fclose(procc_f);
        
                if (!config) {
                    printf_error("error parsing TOML: %s\n", errbuf);    
                    if (_compiler_) {
                        free(_compiler_);
                    }
                    return 0;
                }
                
                toml_table_t *watchdogs_compiler = toml_table_in(config, "compiler");
                if (watchdogs_compiler) {
                    toml_datum_t option_val = toml_string_in(watchdogs_compiler, "option");
                    if (option_val.ok) {
                        wcfg.wd_compiler_opt = strdup(option_val.u.s);
                        free(option_val.u.s);
                    }
        
                    toml_datum_t output_val = toml_string_in(watchdogs_compiler, "output");
                    if (output_val.ok) {
                        wcfg.wd_gamemode_output = output_val.u.s;
                        free(output_val.u.s);
                    }
        
                    toml_array_t *include_paths = toml_array_in(watchdogs_compiler, "include_path");
                    if (include_paths) {
                        int array_size = toml_array_nelem(include_paths);
                        char include_aio_path[250] = {0};
        
                        for (int i = 0; i < array_size; i++) {
                            toml_datum_t path_val = toml_string_at(include_paths, i);
                            if (path_val.ok) {
                                char __procc[250];
                                __copy_strip_dotfns(__procc, sizeof(__procc), path_val.u.s);
                                if (__procc[0] == '\0') continue;
                                if (i > 0) {
                                    size_t cur = strlen(include_aio_path);
                                    if (cur < sizeof(include_aio_path) - 1) {
                                        snprintf(include_aio_path + cur,
                                                 sizeof(include_aio_path) - cur,
                                                 " ");
                                    }
                                }
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1) {
                                    snprintf(include_aio_path + cur,
                                             sizeof(include_aio_path) - cur,
                                             "-i\"%s\"",
                                             __procc);
                                }
                            }
                        }
        
                        static char wd_gamemode[56];
                        if (arg == NULL || *arg == '\0') {
                            toml_datum_t watchdogs_gmodes = toml_string_in(watchdogs_compiler, "input");
                            if (watchdogs_gmodes.ok) {
                                wcfg.wd_gamemode_input = watchdogs_gmodes.u.s;
                            }
                            
                            struct timespec start, end;
                            double compiler_dur;

                            char
                                *path_include = NULL;
                            if (find_for_samp == 0x1) path_include="pawno/include";
                            else if (find_for_omp == 0x1) path_include="qawno/include";
                            
                            int ret = snprintf(
                                _compiler_,
                                format_size_compiler,
                                "%s \"%s\" -o\"%s\" %s -i\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                wcfg.watchdogs_sef_found[0],                   // compiler binary
                                wcfg.wd_gamemode_input,                        // input file
                                wcfg.wd_gamemode_output,                       // output file
                                include_aio_path,                              // include search path
                                path_include,                                  // include directory
                                wcfg.wd_compiler_opt                           // additional options
                            );

                            clock_gettime(CLOCK_MONOTONIC, &start);
                                watchdogs_sys(_compiler_);
                            clock_gettime(CLOCK_MONOTONIC, &end);

                            procc_f = fopen(".wd_compiler.log", "r");
                            if (procc_f) {
                                int ch;
                                while ((ch = fgetc(procc_f)) != EOF)
                                    putchar(ch);
                                rewind(procc_f);
                                while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                    if (strcmp(error_compiler_check, "error") == 0) {
                                        FILE *c_output;
                                        c_output = fopen(watchdogs_c_output_f_container, "r");
                                        if (c_output)
                                            remove(watchdogs_c_output_f_container);
                                        break;
                                    }
                                }
                                fclose(procc_f);
                            } else {
                                printf_error("Failed to open .wd_compiler.log\n");
                            }

                            compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                            printf("[Finished in %.3fs]\n", compiler_dur);
#ifdef DEBUGGING_COMPILER
                            printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                        } else {
                            int find_gamemodes_arg1 = watchdogs_sef_fdir("gamemodes", compile_args);
                            if (find_gamemodes_arg1) {
                                char* container_output;
                                if (wcfg.watchdogs_sef_count > 0 &&
                                    wcfg.watchdogs_sef_found[1][0] != '\0') {
                                    container_output = strdup(wcfg.watchdogs_sef_found[1]);
                                }
                                
                                char i_path_rm[PATH_MAX];
                                snprintf(i_path_rm, sizeof(i_path_rm), "%s", compile_args);
                                char *f_EXT = strrchr(i_path_rm, '.');
                                if (f_EXT && strcmp(f_EXT, ".pwn") == 0)
                                    *f_EXT = '\0';
                                char* f_last_slash_container = strrchr(container_output, '/');
                                if (f_last_slash_container != NULL && *(f_last_slash_container + 1) != '\0')
                                    *(f_last_slash_container + 1) = '\0';

                                snprintf(watchdogs_c_output_f_container, format_size_c_f_container, "%s%s",
                                    container_output, i_path_rm);
                                
                                struct timespec start, end;
                                double compiler_dur;

                                char
                                    *path_include = NULL;
                                if (find_for_samp == 0x1) path_include="pawno/include";
                                else if (find_for_omp == 0x1) path_include="qawno/include";
                                
                                int ret = snprintf(
                                    _compiler_,
                                    format_size_compiler,
                                    "%s \"gamemodes/%s\" -o\"%s.amx\" %s -i\"%s\" \"%s\" > .wd_compiler.log 2>&1",
                                    wcfg.watchdogs_sef_found[0],                    // compiler binary
                                    compile_args,                                   // input file
                                    watchdogs_c_output_f_container,                 // output file
                                    include_aio_path,                               // include search path
                                    path_include,                                   // include directory
                                    wcfg.wd_compiler_opt                            // additional options
                                );

                                if (ret < 0 || (size_t)ret >= (size_t)format_size_compiler) {
                                    fprintf(stderr, "[Error] snprintf() failed or buffer too small (needed %d bytes)\n", ret);
                                }
                                
                                clock_gettime(CLOCK_MONOTONIC, &start);
                                    watchdogs_sys(_compiler_);
                                clock_gettime(CLOCK_MONOTONIC, &end);
                                
                                procc_f = fopen(".wd_compiler.log", "r");
                                if (procc_f) {
                                    int ch;
                                    while ((ch = fgetc(procc_f)) != EOF)
                                        putchar(ch);
                                    rewind(procc_f);
                                    while (fscanf(procc_f, "%99s", error_compiler_check) != EOF) {
                                        if (strcmp(error_compiler_check, "error") == 0) {
                                            FILE *c_output;
                                            c_output = fopen(watchdogs_c_output_f_container, "r");
                                            if (c_output)
                                                remove(watchdogs_c_output_f_container);
                                            break;
                                        }
                                    }
                                    fclose(procc_f);
                                } else {
                                    printf_error("Failed to open .wd_compiler.log\n");
                                }

                                compiler_dur = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                                printf("[Finished in %.3fs]\n", compiler_dur);
#ifdef DEBUGGING_COMPILER
                                printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                            } else {
                                printf_color(COL_RED, "Can't locate: ");
                                printf("%s\n", compile_args);
                                return 0;
                            }
                        }
                    }
                    toml_free(config);
                    if (_compiler_) {
                        free(_compiler_);
                    }
                    return 0;
                }
            } else {
                printf_error("pawncc not found!");
        
                char *ptr_sigA;
                ptr_sigA = readline("install now? [Y/n]: ");
        
                while (1) {
                    if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                        static
                            char platform = 0;
                        ret_pcc_2:
                            println("Select platform:");
                            println("[L/l] Linux");
                            println("[W/w] Windows");
                            println("[T/t] Termux");
                            printf(">> ");

                        if (scanf(" %c", &platform) != 1)
                            return 0;

                        signal(SIGINT, __init);

                        if (platform == 'L' || platform == 'l')
                            watch_pawncc("linux");
                        else if (platform == 'W' || platform == 'w')
                            watch_pawncc("windows");
                        else if (platform == 'T' || platform == 't')
                            watch_pawncc("termux");
                        else {
                            printf("Invalid platform selection. use C^ to exit.\n");
                            goto ret_pcc_2;
                        }
                        break;
                    } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                        break;
                    } else {
                        printf("Invalid input. Please type Y/y to install or N/n to cancel.\n");
                        if (ptr_sigA) { free(ptr_sigA); }
                        ptr_sigA = readline("install now? [Y/n]: ");
                    }
                }

                if (ptr_sigA) { free(ptr_sigA); }
            }
        } else if (strncmp(ptr_command, "running", 7) == 0 || strncmp(ptr_command, "debug", 7) == 0) {
            _runners_:
                if (strcmp(ptr_command, "debug") == 0) {
                    wcfg.server_or_debug="debug";
                    watchdogs_title("Watchdogs | @ debug");    
                } else {
                    watchdogs_title("Watchdogs | @ running");
                }

                static char *arg;
                    arg = ptr_command + 7;
                while (*arg == ' ') arg++;
                char *arg1 = strtok(arg, " ");

                static char *format_prompt = NULL;
                static size_t format_size = 126;
                if (format_prompt == NULL) 
                    format_prompt = malloc(format_size);

                if (find_for_samp == 0x1) {
                    if (*arg == '\0') {
                        FILE *server_log = fopen("server_log.txt", "r");
                        if (server_log)
                            remove("server_log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(ptr_samp, 0777);
#endif
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", ptr_samp);
                        int running_FAIL = watchdogs_sys(snprintf_ptrS);
                        if (running_FAIL == 0) {
                            sleep(2);

                            printf_color(COL_YELLOW, "Press enter to print logs..");
                            getchar();

                            FILE *server_log = fopen("server_log.txt", "r");
                            if (!server_log)
                                printf_error("Can't found server_log.txt!");
                            else {
                                int ch;
                                while ((ch = fgetc(server_log)) != EOF) {
                                    putchar(ch);
                                }
                                fclose(server_log);
                            }
                        } else {
                            printf_color(COL_RED, "running failed! ");
                        }
                        if (format_prompt) { free(format_prompt); }
                        return 0;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        watchdogs_server_samp(arg1, ptr_samp);
                    }
                } else if (find_for_omp == 0x1) {
                    if (*arg == '\0') {
                        FILE *server_log = fopen("log.txt", "r");

                        if (server_log) 
                            remove("log.txt");

                        printf_color(COL_YELLOW, "running..\n");

                        char snprintf_ptrS[128];
#ifndef _WIN32
                        chmod(ptr_openmp, 0777);
#endif
                        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", ptr_openmp);
                        int running_FAIL = watchdogs_sys(snprintf_ptrS);
                        if (running_FAIL == 0) {
                            sleep(2);

                            printf_color(COL_YELLOW, "Press enter to print logs..");
                            getchar();

                            FILE *server_log = fopen("log.txt", "r");
                            if (!server_log)
                                printf_error("Can't found log.txt!");
                            else {
                                int ch;
                                while ((ch = fgetc(server_log)) != EOF) {
                                    putchar(ch);
                                }
                                fclose(server_log);
                            }
                        } else {
                            printf_color(COL_RED, "running failed! ");
                        }
                        if (format_prompt) { free(format_prompt); }
                        return 0;
                    } else {
                        printf_color(COL_YELLOW, "running..\n");
                        watchdogs_server_openmp(arg1, ptr_openmp);
                    }
                } else if (!find_for_omp || !find_for_samp) {
                    printf_error("samp-server/open.mp server not found!");

                    char *ptr_sigA;
                    ptr_sigA = readline("install now? [Y/n]: ");

                    while (1) {
                        if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) {
                            if (__watchdogs_os__ == 0x01) {
                                watch_samp("windows");
                            } else if (__watchdogs_os__ == 0x00) {
                                watch_samp("linux");
                            }
                            break;
                        } else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) {
                            break;
                        } else {
                            printf("Invalid input. Please type Y/y to install or N/n to cancel.\n");
                            if (ptr_sigA) { free(ptr_sigA); }
                            ptr_sigA = readline("install now? [Y/n]: ");
                        }
                    }

                    if (ptr_sigA) { free(ptr_sigA); }
                }
                if (format_prompt) { free(format_prompt); }
        } else if (strcmp(ptr_command, "stop") == 0) {
            watchdogs_title("Watchdogs | @ stop");
            watchdogs_server_stop_tasks();
            return 0;
        } else if (strcmp(ptr_command, "restart") == 0) {
            watchdogs_server_stop_tasks();
            sleep(2);
            goto _runners_;
        } else if (strcmp(ptr_command, _dist_command) != 0 && c_distance <= 2) {
            watchdogs_title("Watchdogs | @ undefined");
            printf("did you mean: %s?\n", _dist_command);
        } else {
            if (strlen(ptr_command) > 0) {
                watchdogs_title("Watchdogs | @ not found");
                println("%s not found!", ptr_command);
            }
        }

        if (ptr_command) { free(ptr_command); }
}

void __init(int sig_unused) {
        (void)sig_unused;
        signal(SIGINT, handle_sigint);
        while (1) {
            __init_wd();
        }
}

int main(void) {
        __init(0);
        return 0;
}

