/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include  "utils.h"
#include  "crypto.h"
#include  "library.h"
#include  "archive.h"
#include  "curl.h"
#include  "endpoint.h"
#include  "compiler.h"
#include  "replicate.h"
#include  "debug.h"
#include  "units.h"

#if defined(__W_VERSION__)
#define WATCHDOGS_RELEASE __W_VERSION__
#else
#define WATCHDOGS_RELEASE "WATCHDOGS"
#endif

const char *  watchdogs_release = WATCHDOGS_RELEASE;
bool          unit_selection_stat = 0;
static struct timespec cmd_start = { 0 };
static struct timespec cmd_end = { 0 };
static double command_dur;

static void
cleanup_local_resources(char **ptr_prompt, char **ptr_command, 
                       char **title_running_info, char **command_ptr,
                       char **timestamp_ptr, char **size_command_ptr,
                       char **platform_ptr, char **signal_ptr,
                       char **debug_endpoint_ptr, char **args_ptr,
                       char **compile_target_ptr)
{
    if (ptr_prompt && *ptr_prompt) {
        free(*ptr_prompt);
        *ptr_prompt = NULL;
    }
    if (ptr_command && *ptr_command) {
        free(*ptr_command);
        *ptr_command = NULL;
    }
    if (title_running_info && *title_running_info) {
        free(*title_running_info);
        *title_running_info = NULL;
    }
    if (command_ptr && *command_ptr) {
        free(*command_ptr);
        *command_ptr = NULL;
    }
    if (timestamp_ptr && *timestamp_ptr) {
        free(*timestamp_ptr);
        *timestamp_ptr = NULL;
    }
    if (size_command_ptr && *size_command_ptr) {
        free(*size_command_ptr);
        *size_command_ptr = NULL;
    }
    if (platform_ptr && *platform_ptr) {
        free(*platform_ptr);
        *platform_ptr = NULL;
    }
    if (signal_ptr && *signal_ptr) {
        free(*signal_ptr);
        *signal_ptr = NULL;
    }
    if (debug_endpoint_ptr && *debug_endpoint_ptr) {
        free(*debug_endpoint_ptr);
        *debug_endpoint_ptr = NULL;
    }
    if (args_ptr && *args_ptr) {
        free(*args_ptr);
        *args_ptr = NULL;
    }
    if (compile_target_ptr && *compile_target_ptr) {
        free(*compile_target_ptr);
        *compile_target_ptr = NULL;
    }
}

static
int
__command__(char *unit_pre_command)
{
    unit_debugging(0);
    
    memset(&cmd_start, 0, sizeof(cmd_start));
    memset(&cmd_end, 0, sizeof(cmd_end));

    char *ptr_prompt = NULL;
    size_t size_ptr_command = DOG_MAX_PATH + DOG_PATH_MAX;
    char *ptr_command = NULL;
    const char *command_similar = NULL;
    int dist = INT_MAX;
    int ret_code = -1;
    
    char *title_running_info = NULL;
    char *command = NULL;
    char *timestamp = NULL;
    char *size_command = NULL;
    char *platform = NULL;
    char *pointer_signalA = NULL;
    char *debug_endpoint = NULL;
    char *__args = NULL;
    char *compile_target = NULL;
    
    if (compiling_gamemode == true) {
        compiling_gamemode = false;
        const char *argsc[] = { NULL, NULL, NULL,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL };
        dog_exec_compiler(argsc[0], argsc[1], argsc[2],
            argsc[3], argsc[4], argsc[5], argsc[6], argsc[7],
            argsc[8], argsc[9]);
    }
    static bool installing_stdlib_warn = false;
    if (compiler_installing_stdlib == true && installing_stdlib_warn == false) {
        installing_stdlib_warn = true;
        printf("\n");
        if (fetch_server_env()==1) {
            pr_info(stdout, "can't found sa-mp stdlib.. installing...");
            pr_info(stdout, "select version:\n\t1: 0.3.DL-R1 | 2: 0.3.7-R2-1-1");
            char *version = readline("> ");
            if (version[0] == '\0' || version[0] == '2') {
                dog_install_depends("gskeleton/samp-stdlib", "main", NULL);
            } else {
                dog_install_depends("gskeleton/samp-stdlib", "0.3.dl", NULL);
            }
            dog_free(version);
        } else {
            pr_info(stdout, "can't found open.mp stdlib.. installing..");
            dog_install_depends("openmultiplayer/omp-stdlib", "master", NULL);
        }
        ptr_command = strdup("compile");
        goto _reexecute_command;
    }

    ptr_prompt = dog_malloc(size_ptr_command);
    if (!ptr_prompt) {
        ret_code = -1;
        goto cleanup;
    }
    
    if (ptr_command) {
        free(ptr_command);
        ptr_command = NULL;
    }

    static bool unit_initial = false;
    if (!unit_initial) {
        unit_initial = true;
        using_history();
        unit_show_dog();
    }
    
    if (unit_pre_command && unit_pre_command[0] != '\0') {
        ptr_command = strdup(unit_pre_command);
        if (!ptr_command) {
            ret_code = -1;
            goto cleanup;
        }
        if (strfind(ptr_command, "812C397D", true) == 0) {
            printf("# %s\n", ptr_command);
        }
    } else {
        while (true) {
            snprintf(ptr_prompt, size_ptr_command, "# ");
            char *input = readline(ptr_prompt);
            if (!input) {
                ret_code = 2;
                goto cleanup;
            }
            
            if (input[0] == '\0') {
                free(input);
                continue;
            }
            
            ptr_command = strdup(input);
            free(input);
            
            if (!ptr_command) {
                ret_code = -1;
                goto cleanup;
            }
            break;
        }
    }

    fflush(stdout);
    if (ptr_command && ptr_command[0] != '\0' &&
        strfind(ptr_command, "812C397D", true) == false) {
        if (history_length > 0) {
            HIST_ENTRY *last = history_get(history_length - 1);
            if (!last || strcmp(last->line, ptr_command) != 0) {
                add_history(ptr_command);
            }
        } else {
            add_history(ptr_command);
        }
    }

    command_similar = dog_find_near_command(ptr_command,
        unit_command_list, unit_command_len, &dist);

_reexecute_command:
    unit_debugging(0);
    clock_gettime(CLOCK_MONOTONIC, &cmd_start);
    
    if (strncmp(ptr_command, "help", strlen("help")) == 0) {
        dog_console_title("Watchdogs | @ help");
        char *args = ptr_command + strlen("help");
        while (*args == ' ') ++args;
        unit_show_help(args);
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "exit") == 0) {
        ret_code = 2;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "sha1", strlen("sha1")) == 0) {
        char *args = ptr_command + strlen("sha1");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: sha1 [<words>]");
        } else {
            unsigned char digest[20];
            if (crypto_generate_sha1_hash(args, digest) == 1) {
                printf("        Crypto Input : " DOG_COL_YELLOW "%s\n", args);
                print_restore_color();
                printf("Crypto Output (sha1) : " DOG_COL_YELLOW);
                crypto_print_hex(digest, sizeof(digest), 1);
                print_restore_color();
            }
        }
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "sha256", strlen("sha256")) == 0) {
        char *args = ptr_command + strlen("sha256");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: sha256 [<words>]");
        } else {
            unsigned char digest[32];
            if (crypto_generate_sha256_hash(args, digest) == 1) {
                printf("          Crypto Input : " DOG_COL_YELLOW "%s\n", args);
                print_restore_color();
                printf("Crypto Output (SHA256) : " DOG_COL_YELLOW);
                crypto_print_hex(digest, sizeof(digest), 1);
                print_restore_color();
            }
        }
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "crc32", strlen("crc32")) == 0) {
        char *args = ptr_command + strlen("crc32");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: crc32 [<words>]");
        } else {
            uint32_t crc32_generate = crypto_generate_crc32(args, strlen(args));
            char crc_str[11];
            sprintf(crc_str, "%08X", crc32_generate);
            printf("         Crypto Input : " DOG_COL_YELLOW "%s\n", args);
            print_restore_color();
            printf("Crypto Output (CRC32) : " DOG_COL_YELLOW "%s\n", crc_str);
            print_restore_color();
        }
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "djb2", strlen("djb2")) == 0) {
        char *args = ptr_command + strlen("djb2");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: djb2 [<file>]");
        } else {
            if (path_exists(args) == 0) {
                pr_warning(stdout, "djb2: " DOG_COL_CYAN "%s - No such file or directory", args);
                ret_code = -1;
                goto cleanup;
            }
            
            unsigned long djb2_generate = crypto_djb2_hash_file(args);
            if (djb2_generate) {
                printf("        Crypto Input : " DOG_COL_YELLOW "%s\n", args);
                print_restore_color();
                printf("Crypto Output (DJB2) : " DOG_COL_YELLOW "%#lx\n", djb2_generate);
                print_restore_color();
            }
        }
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "pbkdf2", strlen("pbkdf2")) == 0) {
        char *args = ptr_command + strlen("pbkdf2");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: pbkdf2 [<password>]");
        } else {
            unsigned char pbkdf_generate[32];
            unsigned char stored_salt[16];

            crypto_simple_rand_bytes(stored_salt, 16);

            int ret = crypto_derive_key_pbkdf2(
                  args,
                  stored_salt,
                  16,
                  pbkdf_generate,
                  32
            );

            if (ret != 1) {
                printf("PBKDF2 Error\n");
                goto pbkdf_done;
            }

            printf("        Crypto Input : " DOG_COL_YELLOW "%s\n", args);
            char *hex_output = NULL;
            crypto_convert_to_hex(pbkdf_generate, 32, &hex_output);
            printf("Crypto Output (PBKDF2) : " DOG_COL_YELLOW "%s\n", hex_output);
            free(hex_output);
        }
        pbkdf_done:
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "config") == 0) {
        if (path_access("watchdogs.toml"))
            remove("watchdogs.toml");
        unit_debugging(1);
        printf(DOG_COL_B_BLUE "");
        dog_printfile("watchdogs.toml");
        printf(DOG_COL_DEFAULT "\n");
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "replicate", strlen("replicate")) == 0) {
        dog_console_title("Watchdogs | @ replicate depends");
        char *args = ptr_command + strlen("replicate");
        while (*args == ' ') ++args;
        
        int is_null_args = (args[0] == '\0' || strlen(args) < 1) ? 1 : -1;
        
        __args = strdup(args);
        if (!__args) {
            ret_code = -1;
            goto cleanup;
        }
        
        char *raw_branch = NULL;
        char *raw_save = NULL;
        char *args2 = strtok(__args, " ");
        if (!args2 || strcmp(args2, ".") == 0) is_null_args = 1;
        
        char *procure_args = strtok(args, " ");
        while (procure_args) {
            if (strcmp(procure_args, "--branch") == 0) {
                procure_args = strtok(NULL, " ");
                if (procure_args) raw_branch = procure_args;
            } else if (strcmp(procure_args, "--save") == 0) {
                procure_args = strtok(NULL, " ");
                if (procure_args) raw_save = procure_args;
            }
            procure_args = strtok(NULL, " ");
        }
        
        if (raw_save && strcmp(raw_save, ".") == 0) {
            static char *fetch_pwd = NULL;
            fetch_pwd = dog_procure_pwd();
            raw_save = strdup(fetch_pwd);
        }
        
        free(__args);
        __args = NULL;
        
        if (is_null_args != 1) {
            if (raw_branch && raw_save) dog_install_depends(args, raw_branch, raw_save);
            else if (raw_branch) dog_install_depends(args, raw_branch, NULL);
            else if (raw_save) dog_install_depends(args, "main", raw_save);
            else dog_install_depends(args, "main", NULL);
        } else {
            char errbuf[DOG_PATH_MAX];
            toml_table_t *dog_toml_server_config;
            FILE *this_proc_file = fopen("watchdogs.toml", "r");
            dog_toml_server_config = toml_parse_file(this_proc_file, errbuf, sizeof(errbuf));
            if (this_proc_file) fclose(this_proc_file);
            
            if (!dog_toml_server_config) {
                pr_error(stdout, "failed to parse the watchdogs.toml...: %s", errbuf);
                minimal_debugging();
                ret_code = 0;
                goto cleanup;
            }
            
            toml_table_t *dog_depends = toml_table_in(dog_toml_server_config, TOML_TABLE_DEPENDENCIES);
            char *expect = NULL;
            
            if (dog_depends) {
                toml_array_t *dog_toml_packages = toml_array_in(dog_depends, "packages");
                if (dog_toml_packages) {
                    int arr_sz = toml_array_nelem(dog_toml_packages);
                    for (int i = 0; i < arr_sz; i++) {
                        toml_datum_t val = toml_string_at(dog_toml_packages, i);
                        if (!val.ok) continue;
                        
                        if (!expect) {
                            expect = dog_realloc(NULL, strlen(val.u.s) + 1);
                            if (expect) snprintf(expect, strlen(val.u.s) + 1, "%s", val.u.s);
                        } else {
                            char *tmp;
                            size_t old_len = strlen(expect);
                            size_t new_len = old_len + strlen(val.u.s) + 2;
                            
                            tmp = dog_realloc(expect, new_len);
                            if (tmp) {
                                expect = tmp;
                                snprintf(expect + old_len, new_len - old_len, " %s", val.u.s);
                            }
                        }
                        
                        dog_free(val.u.s);
                        val.u.s = NULL;
                    }
                }
            }
            
            if (!expect) expect = strdup("");
            
            dog_free(dogconfig.dog_toml_packages);
            dogconfig.dog_toml_packages = expect;
            
            printf("Trying to installing:\n   %s",
                dogconfig.dog_toml_packages);
            
            if (raw_branch && raw_save) dog_install_depends(dogconfig.dog_toml_packages, raw_branch, raw_save);
            else if (raw_branch) dog_install_depends(dogconfig.dog_toml_packages, raw_branch, NULL);
            else if (raw_save) dog_install_depends(dogconfig.dog_toml_packages, "main", raw_save);
            else dog_install_depends(dogconfig.dog_toml_packages, "main", NULL);
            
            toml_free(dog_toml_server_config);
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "gamemode") == 0) {
        dog_console_title("Watchdogs | @ gamemode");
        
        unit_selection_stat = true;
        
        printf("\033[1;33m== Select a Platform ==\033[0m\n");
        printf("  \033[36m[l]\033[0m Linux\n");
        printf("  \033[36m[w]\033[0m Windows\n"
               "  ^ \033[90m(Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)\033[0m\n");
        printf("  \033[36m[t]\033[0m Termux\n");
        
        platform = readline("==> ");
        if (!platform) {
            ret_code = -1;
            goto cleanup;
        }
        
        int platform_ret = -1;
        if (strfind(platform, "L", true)) {
            platform_ret = dog_install_server("linux");
        } else if (strfind(platform, "W", true)) {
            platform_ret = dog_install_server("windows");
        } else if (strfind(platform, "T", true)) {
            platform_ret = dog_install_server("termux");
        } else if (strfind(platform, "E", true)) {
            ret_code = -1;
            goto cleanup;
        } else {
            pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
            ret_code = -1;
            goto cleanup;
        }
        
        free(platform);
        platform = NULL;
        
        if (platform_ret == 0) {
            ret_code = -1;
            goto cleanup;
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "pawncc") == 0) {
        dog_console_title("Watchdogs | @ pawncc");
        
        unit_selection_stat = true;
        
        printf("\033[1;33m== Select a Platform ==\033[0m\n");
        printf("  \033[36m[l]\033[0m Linux\n");
        printf("  \033[36m[w]\033[0m Windows\n"
               "  ^ \033[90m(Supported for: WSL/WSL2 ; not: Docker or Podman on WSL)\033[0m\n");
        printf("  \033[36m[t]\033[0m Termux\n");
        
        platform = readline("==> ");
        if (!platform) {
            ret_code = -1;
            goto cleanup;
        }
        
        int platform_ret = -1;
        if (strfind(platform, "L", true)) {
            platform_ret = dog_install_pawncc("linux");
        } else if (strfind(platform, "W", true)) {
            platform_ret = dog_install_pawncc("windows");
        } else if (strfind(platform, "T", true)) {
            platform_ret = dog_install_pawncc("termux");
        } else if (strfind(platform, "E", true)) {
            ret_code = -1;
            goto cleanup;
        } else {
            pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
            ret_code = -1;
            goto cleanup;
        }
        
        free(platform);
        platform = NULL;
        
        if (platform_ret == 0) {
            ret_code = -1;
            goto cleanup;
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "debug") == 0) {
        dog_console_title("Watchdogs | @ debug");
        dog_stop_server_tasks();
        unit_ret_main("812C397D");
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "812C397D") == 0) {
        dog_server_crash_check();
        ret_code = 3;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "compile", strlen("compile")) == 0 &&
               !isalpha((unsigned char)ptr_command[strlen("compile")])) {
        dog_console_title("Watchdogs | @ compile | logging file: .watchdogs/compiler.log");
        
        char *args = ptr_command + strlen("compile");
        while (*args == ' ') args++;
        
        char *compile_args = strtok(args, " ");
        char *second_arg = strtok(NULL, " ");
        char *four_arg = strtok(NULL, " ");
        char *five_arg = strtok(NULL, " ");
        char *six_arg = strtok(NULL, " ");
        char *seven_arg = strtok(NULL, " ");
        char *eight_arg = strtok(NULL, " ");
        char *nine_arg = strtok(NULL, " ");
        char *ten_arg = strtok(NULL, " ");
        
        dog_exec_compiler(args, compile_args, second_arg, four_arg,
                         five_arg, six_arg, seven_arg, eight_arg,
                         nine_arg, ten_arg);
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "decompile", strlen("decompile")) == 0) {
        dog_console_title("Watchdogs | @ decompile");

        char *args = ptr_command + strlen("decompile");
        while (*args == ' ') args++;
        if (*args == '\0') {
            println(stdout, "Usage: decompile [<file.amx>]");
            ret_code = -1;
            goto cleanup;
        }
        if (strfind(args, ".amx", true) == false) {
            println(stdout, "Usage: decompile [<file.amx>]");
            ret_code = -1;
            goto cleanup;
        }

        char *pawndisasm_ptr = NULL;
        int   ret_pawndisasm = 0;
        if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0) {
            pawndisasm_ptr = "pawndisasm.exe";
        } else if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_LINUX) == 0) {
            pawndisasm_ptr = "pawndisasm";
        }

        dog_sef_path_revert();

        if (dir_exists("pawno") != 0 && dir_exists("qawno") != 0) {
            ret_pawndisasm = dog_find_path("pawno", pawndisasm_ptr,
                NULL);
            if (ret_pawndisasm) {
                ;
            } else {
                ret_pawndisasm = dog_find_path("qawno",
                    pawndisasm_ptr, NULL);
                if (ret_pawndisasm < 1) {
                    ret_pawndisasm = dog_find_path(".",
                        pawndisasm_ptr, NULL);
                }
            }
        } else if (dir_exists("pawno") != 0) {
            ret_pawndisasm = dog_find_path("pawno", pawndisasm_ptr,
                NULL);
            if (ret_pawndisasm) {
                ;
            } else {
                ret_pawndisasm = dog_find_path(".",
                    pawndisasm_ptr, NULL);
            }
        } else if (dir_exists("qawno") != 0) {
            ret_pawndisasm = dog_find_path("qawno", pawndisasm_ptr,
                NULL);
            if (ret_pawndisasm) {
                ;
            } else {
                ret_pawndisasm = dog_find_path(".",
                    pawndisasm_ptr, NULL);
            }
        } else {
            ret_pawndisasm = dog_find_path(".", pawndisasm_ptr,
                NULL);
        }
        if (ret_pawndisasm) {

            if (condition_check(dogconfig.dog_sef_found_list[0]) == 1) {
                ret_code = -1;
                goto cleanup;
            }

            char *args2 = strdup(args);
            char *dot_amx = strstr(args2, ".amx");
            if (dot_amx)
                {
                    *dot_amx = '\0';
                }
            char s_args[DOG_PATH_MAX];
            snprintf(s_args, sizeof(s_args), "%s.asm", args2);
            dog_free(args2);
            char s_argv[DOG_PATH_MAX * 3];
            #ifdef DOG_LINUX
                char *executor = "sh -c";
            #else
                char *executor = "cmd.exe /C";
            #endif
            snprintf(s_argv, sizeof(s_argv),
                "%s '%s %s %s'", executor, dogconfig.dog_sef_found_list[0], args, s_args);
            char *argv[] = { s_argv, NULL };
            int ret = dog_exec_command(argv);
            if (!ret) println(stdout, "%s", s_args);
            dog_console_title(s_argv);
        } else {
            printf("\033[1;31merror:\033[0m pawndisasm/pawncc (our compiler) not found\n"
                "  \033[2mhelp:\033[0m install it before continuing\n");
        }
        ret_code = -1;
        goto cleanup;

} else if (strncmp(ptr_command, "running", strlen("running")) == 0) {
        dog_stop_server_tasks();
        
        if (!path_access(dogconfig.dog_toml_server_binary)) {
            pr_error(stdout, "can't locate sa-mp/open.mp binary file!");
            ret_code = -1;
            goto cleanup;
        }
        if (!path_access(dogconfig.dog_toml_server_config)) {
            pr_warning(stdout, "can't locate %s - config file!", dogconfig.dog_toml_server_config);
            ret_code = -1;
            goto cleanup;
        }
        
        if (dir_exists(".watchdogs") == 0) MKDIR(".watchdogs");
        
        size_t command_len = 7;
        char *args = ptr_command + command_len;
        while (*args == ' ') ++args;
        char *args2 = strtok(args, " ");
        
        char *size_arg1 = args2 ? args2 : dogconfig.dog_toml_proj_output;
        
        int needed = snprintf(NULL, 0, "Watchdogs | @ running | args: %s | config: %s | CTRL + C to stop. | \"debug\" for debugging",
                                size_arg1, dogconfig.dog_toml_server_config) + 1;
        title_running_info = dog_malloc(needed);
        if (!title_running_info) {
            ret_code = -1;
            goto cleanup;
        }
        snprintf(title_running_info, needed,
                "Watchdogs | @ running | args: %s | config: %s | CTRL + C to stop. | \"debug\" for debugging",
                size_arg1, dogconfig.dog_toml_server_config);
        
    #ifdef DOG_ANDROID
        println(stdout, "%s", title_running_info);
    #else
        dog_console_title(title_running_info);
    #endif
        
        free(title_running_info);
        title_running_info = NULL;
        
        if (!path_access(dogconfig.dog_toml_server_config)) {
            pr_error(stdout, "%s not found!", dogconfig.dog_toml_server_config);
            ret_code = -1;
            goto cleanup;
        }
        
        command = dog_malloc(DOG_PATH_MAX);
        if (!command) {
            ret_code = -1;
            goto cleanup;
        }
        
        struct sigaction sa;
        if (path_access("announce")) __set_default_access("announce");
        
        int rate_endpoint_failed = -1;

        if (fetch_server_env() == 1) {
            if (!args2 || (args2[0] == '.' && args2[1] == '\0')) {
                sa.sa_handler = unit_sigint_handler;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_RESTART;
                
                if (sigaction(SIGINT, &sa, NULL) == -1) {
                    perror("sigaction");
                    exit(EXIT_FAILURE);
                }
                
        #ifdef DOG_WINDOWS
                {
                    STARTUPINFOA        _STARTUPINFO;
                    PROCESS_INFORMATION _PROCESS_INFO;
                    
                    ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
                    ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));
                    
                    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
                    _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;
                    
                    _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
                    _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
                    _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
                    
                    snprintf(command, DOG_PATH_MAX, "%s%s", _PATH_STR_EXEC, dogconfig.dog_toml_server_binary);
                    
                    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
                                        &_STARTUPINFO, &_PROCESS_INFO)) {
                        rate_endpoint_failed = -1;
                    } else {
                        WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
                        CloseHandle(_PROCESS_INFO.hProcess);
                        CloseHandle(_PROCESS_INFO.hThread);
                        rate_endpoint_failed = 0;
                    }
                }
        #else
                {
                    pid_t pid;
                    __set_default_access(dogconfig.dog_toml_server_binary);
                    
                    snprintf(command, DOG_PATH_MAX, "%s/%s", dog_procure_pwd(), dogconfig.dog_toml_server_binary);
                    
                    if (condition_check(command) == 1) {
                        ret_code = -1;
                        goto cleanup;
                    }

                    int stdout_pipe[2];
                    int stderr_pipe[2];
                    
                    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
                        perror("pipe");
                        ret_code = -1;
                        goto cleanup;
                    }
                    
                    pid = fork();
                    if (pid == 0) {
                        close(stdout_pipe[0]);
                        close(stderr_pipe[0]);
                        
                        dup2(stdout_pipe[1], STDOUT_FILENO);
                        dup2(stderr_pipe[1], STDERR_FILENO);
                        
                        close(stdout_pipe[1]);
                        close(stderr_pipe[1]);
                        
                        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
                        _exit(127);
                    } else if (pid > 0) {
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
                        waitpid(pid, &status, 0);
                        
                        if (WIFEXITED(status)) {
                            int exit_code = WEXITSTATUS(status);
                            rate_endpoint_failed = (exit_code == 0) ? 0 : -1;
                        } else {
                            rate_endpoint_failed = -1;
                        }
                    }
                }
        #endif
                
                if (rate_endpoint_failed != 0) {
                    printf(DOG_COL_DEFAULT "\n");
                    pr_color(stdout, DOG_COL_RED, "Server startup failed!\n");
                } else {
                    printf(DOG_COL_DEFAULT "\n");
                }
                
                printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                debug_endpoint = readline("   answer (y/n): ");
                if (debug_endpoint && (debug_endpoint[0] == '\0' ||
                    strcmp(debug_endpoint, "Y") == 0 ||
                    strcmp(debug_endpoint, "y") == 0)) {
                    unit_ret_main("debug");
                }
                
            } else {
                dog_exec_samp_server(args2, dogconfig.dog_toml_server_binary);
                restore_server_config();
                
                printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                debug_endpoint = readline("   answer (y/n): ");
                if (debug_endpoint && (debug_endpoint[0] == '\0' ||
                    strcmp(debug_endpoint, "Y") == 0 ||
                    strcmp(debug_endpoint, "y") == 0)) {
                    unit_ret_main("debug");
                }
            }
        } else if (fetch_server_env() == 2) {
            if (!args2 || (args2[0] == '.' && args2[1] == '\0')) {
                sa.sa_handler = unit_sigint_handler;
                sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_RESTART;
                
                if (sigaction(SIGINT, &sa, NULL) == -1) {
                    perror("sigaction");
                    exit(EXIT_FAILURE);
                }
                
        #ifdef DOG_WINDOWS
                {
                    STARTUPINFOA        _STARTUPINFO;
                    PROCESS_INFORMATION _PROCESS_INFO;
                    
                    ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
                    ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));
                    
                    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
                    _STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;
                    
                    _STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
                    _STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
                    _STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
                    
                    snprintf(command, DOG_PATH_MAX, "%s%s", _PATH_STR_EXEC, dogconfig.dog_toml_server_binary);
                    
                    if (!CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
                                        &_STARTUPINFO, &_PROCESS_INFO)) {
                        rate_endpoint_failed = -1;
                    } else {
                        WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
                        CloseHandle(_PROCESS_INFO.hProcess);
                        CloseHandle(_PROCESS_INFO.hThread);
                        rate_endpoint_failed = 0;
                    }
                }
        #else
                {
                    pid_t pid;
                    __set_default_access(dogconfig.dog_toml_server_binary);
                    
                    snprintf(command, DOG_PATH_MAX, "%s/%s", dog_procure_pwd(), dogconfig.dog_toml_server_binary);\

                    if (condition_check(command) == 1) {
                        ret_code = -1;
                        goto cleanup;
                    }

                    int stdout_pipe[2];
                    int stderr_pipe[2];
                    
                    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
                        perror("pipe");
                        ret_code = -1;
                        goto cleanup;
                    }
                    
                    pid = fork();
                    if (pid == 0) {
                        close(stdout_pipe[0]);
                        close(stderr_pipe[0]);
                        
                        dup2(stdout_pipe[1], STDOUT_FILENO);
                        dup2(stderr_pipe[1], STDERR_FILENO);
                        
                        close(stdout_pipe[1]);
                        close(stderr_pipe[1]);
                        
                        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
                        _exit(127);
                    } else if (pid > 0) {
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
                        waitpid(pid, &status, 0);
                        
                        if (WIFEXITED(status)) {
                            int exit_code = WEXITSTATUS(status);
                            rate_endpoint_failed = (exit_code == 0) ? 0 : -1;
                        } else {
                            rate_endpoint_failed = -1;
                        }
                    }
                }
        #endif
                
                if (rate_endpoint_failed != 0) {
                    printf(DOG_COL_DEFAULT "\n");
                    pr_color(stdout, DOG_COL_RED, "Server startup failed!\n");
                } else {
                    printf(DOG_COL_DEFAULT "\n");
                }
                
                printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                debug_endpoint = readline("   answer (y/n): ");
                if (debug_endpoint && (debug_endpoint[0] == '\0' ||
                    strcmp(debug_endpoint, "Y") == 0 ||
                    strcmp(debug_endpoint, "y") == 0)) {
                    unit_ret_main("debug");
                }
                
            } else {
                dog_exec_omp_server(args2, dogconfig.dog_ptr_omp);
                restore_server_config();
                
                printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
                debug_endpoint = readline("   answer (y/n): ");
                if (debug_endpoint && (debug_endpoint[0] == '\0' ||
                    strcmp(debug_endpoint, "Y") == 0 ||
                    strcmp(debug_endpoint, "y") == 0)) {
                    unit_ret_main("debug");
                }
            }
        } else {
            pr_error(stdout, "\033[1;31merror:\033[0m sa-mp/open.mp server not found!\n"
                     "  \033[2mhelp:\033[0m install it before continuing\n");
            printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");
            
            unit_selection_stat = true;
            
            pointer_signalA = readline("");
            if (!pointer_signalA) {
                ret_code = -1;
                goto cleanup;
            }
            
            if (strcmp(pointer_signalA, "Y") == 0 || strcmp(pointer_signalA, "y") == 0) {
                if (!strcmp(dogconfig.dog_os_type, OS_SIGNAL_WINDOWS)) {
                    dog_install_server("windows");
                } else if (!strcmp(dogconfig.dog_os_type, OS_SIGNAL_LINUX)) {
                    dog_install_server("linux");
                }
            }
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "compiles", strlen("compiles")) == 0) {
        dog_console_title("Watchdogs | @ compiles");
        
        char *args = ptr_command + strlen("compiles");
        while (*args == ' ') ++args;
        char *args2 = strtok(args, " ");
        
        if (!args2 || args2[0] == '\0' || args2[0] == '.') {
            const char *argsc[] = { NULL, ".", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
            dog_exec_compiler(argsc[0], argsc[1], argsc[2], argsc[3], argsc[4], argsc[5], argsc[6], argsc[7], argsc[8], argsc[9]);
            dog_configure_toml();
            
            if (!compiler_is_err) unit_ret_main("running .");
        } else {
            const char *argsc[] = { NULL, args2, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
            dog_exec_compiler(argsc[0], argsc[1], argsc[2], argsc[3], argsc[4], argsc[5], argsc[6], argsc[7], argsc[8], argsc[6]);
            dog_configure_toml();
            
            if (!compiler_is_err) {
                size_t command_len = strlen(args) + 10;
                size_command = dog_malloc(command_len);
                if (size_command) {
                    snprintf(size_command, command_len, "running %s", args);
                    unit_ret_main(size_command);
                }
            }
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "stop") == 0) {
        dog_console_title("Watchdogs | @ stop");
        dog_stop_server_tasks();
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "restart") == 0) {
        dog_console_title("Watchdogs | @ restart");
        dog_stop_server_tasks();
        unit_ret_main("running .");
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "tracker", strlen("tracker")) == 0) {
        char *args = ptr_command + strlen("tracker");
        while (*args == ' ') ++args;
        
        if (*args == '\0') {
            println(stdout, "Usage: tracker [<name>]");
            ret_code = -1;
            goto cleanup;
        }
        
        CURL *curl;
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (!curl) {
            fprintf(stderr, "Curl initialization failed!\n");
            ret_code = -1;
            goto cleanup;
        }
        
        int variation_count = 0;
        char variations[MAX_VARIATIONS][MAX_USERNAME_LEN];
        tracker_discrepancy(args, variations, &variation_count);
        
        printf("[TRACKER] Search base: %s\n", args);
        printf("[TRACKER] Generated %d Variations\n\n", variation_count);
        
        for (int i = 0; i < variation_count; i++) {
            printf("=== TRACKING ACCOUNTS: %s ===\n", variations[i]);
            tracking_username(curl, variations[i]);
            print("\n");
        }
        
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "compress", strlen("compress")) == 0) {
        char *args = ptr_command + strlen("compress");
        while (*args == ' ') args++;
        
        if (*args == '\0') {
            printf("Usage: compress --file <input> --output <output> --type <format>\n");
            printf("Example:\n\tcompress --file myfile.txt --output myarchive.zip --type zip\n\t"
                   "compress --file myfolder/ --output myarchive.tar.gz --type gz\n");
            ret_code = -1;
            goto cleanup;
        }
        
        char *raw_input = NULL, *raw_output = NULL, *raw_type = NULL;
        char *procure_args = strtok(args, " ");
        while (procure_args) {
            if (strcmp(procure_args, "--file") == 0) {
                procure_args = strtok(NULL, " ");
                if (procure_args) raw_input = procure_args;
            } else if (strcmp(procure_args, "--output") == 0) {
                procure_args = strtok(NULL, " ");
                if (procure_args) raw_output = procure_args;
            } else if (strcmp(procure_args, "--type") == 0) {
                procure_args = strtok(NULL, " ");
                if (procure_args) raw_type = procure_args;
            }
            procure_args = strtok(NULL, " ");
        }
        
        if (!raw_input || !raw_output || !raw_type) {
            printf("Missing arguments!\n");
            printf("Usage: compress --file <input> --output <output> --type <zip|tar|gz|bz2|xz>\n");
            printf("Example:\n\tcompress --file myfile.txt --output myarchive.zip --type zip\n\t"
                   "compress --file myfolder/ --output myarchive.tar.gz --type gz\n");
            ret_code = -1;
            goto cleanup;
        }
        
        CompressionFormat fmt;
        if (strcmp(raw_type, "zip") == 0) fmt = COMPRESS_ZIP;
        else if (strcmp(raw_type, "tar") == 0) fmt = COMPRESS_TAR;
        else if (strcmp(raw_type, "gz") == 0) fmt = COMPRESS_TAR_GZ;
        else if (strcmp(raw_type, "bz2") == 0) fmt = COMPRESS_TAR_BZ2;
        else if (strcmp(raw_type, "xz") == 0) fmt = COMPRESS_TAR_XZ;
        else {
            printf("Unknown type: %s\n", raw_type);
            printf("Supported: zip, tar, gz, bz2, xz\n");
            ret_code = -1;
            goto cleanup;
        }
        
        const char *procure_items[] = { raw_input };
        int ret = compress_to_archive(raw_output, procure_items, 1, fmt);
        if (ret == 0) {
            pr_info(stdout, "Converter file/folder to archive (Compression) successfully: %s\n", raw_output);
        } else {
            pr_error(stdout, "Compression failed!\n");
            minimal_debugging();
        }
        
        ret_code = -1;
        goto cleanup;
        
    } else if (strncmp(ptr_command, "send", strlen("send")) == 0) {
        char *args = ptr_command + strlen("send");
        while (*args == ' ') ++args;
        
        timestamp = dog_malloc(64);
        
        if (*args == '\0') {
            println(stdout, "Usage: send [<file_path>]");
            ret_code = -1;
            goto cleanup;
        }
        
        if (path_access(args) == 0) {
            pr_error(stdout, "file not found: %s", args);
            ret_code = -1;
            goto cleanup;
        }
        
        if (!dogconfig.dog_toml_webhooks || strfind(dogconfig.dog_toml_webhooks, "DO_HERE", true) ||
            strlen(dogconfig.dog_toml_webhooks) < 1) {
            pr_color(stdout, DOG_COL_YELLOW, " ~ Discord webhooks not available");
            ret_code = -1;
            goto cleanup;
        }
        
        char *filename = args;
        if (strrchr(args, _PATH_CHR_SEP_POSIX) && strrchr(args, _PATH_CHR_SEP_WIN32)) {
            filename = (strrchr(args, _PATH_CHR_SEP_POSIX) > strrchr(args, _PATH_CHR_SEP_WIN32)) ?
                      strrchr(args, _PATH_CHR_SEP_POSIX) + 1 : strrchr(args, _PATH_CHR_SEP_WIN32) + 1;
        } else if (strrchr(args, _PATH_CHR_SEP_POSIX)) {
            filename = strrchr(args, _PATH_CHR_SEP_POSIX) + 1;
        } else if (strrchr(args, _PATH_CHR_SEP_WIN32)) {
            filename = strrchr(args, _PATH_CHR_SEP_WIN32) + 1;
        }
        
        CURL *curl = curl_easy_init();
        if (curl) {
            CURLcode res;
            curl_mime *mime;
            
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
            
            mime = curl_mime_init(curl);
            if (!mime) {
                fprintf(stderr, "Failed to create MIME handle\n");
                minimal_debugging();
                curl_easy_cleanup(curl);
                ret_code = -1;
                goto cleanup;
            }
            
            curl_mimepart *part;
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            if (timestamp) {
                snprintf(timestamp, 64, "%04d/%02d/%02d %02d:%02d:%02d",
                        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                        tm.tm_hour, tm.tm_min, tm.tm_sec);
            }
            
            dog_portable_stat_t st;
            if (dog_portable_stat(filename, &st) == 0) {
                char *content_data = dog_malloc(DOG_MAX_PATH);
                if (content_data) {
                    snprintf(content_data, DOG_MAX_PATH,
                            "### received send command - %s\n"
                            "> Metadata\n"
                            "- Name: %s\n- Size: %llu bytes\n- Last modified: %llu\n%s",
                            timestamp ? timestamp : "unknown",
                            filename,
                            (unsigned long long)st.st_size,
                            (unsigned long long)st.st_lmtime,
                            "-# Please note that if you are using webhooks with a public channel,"
                            "always convert the file into an archive with a password known only to you.");
                    
                    part = curl_mime_addpart(mime);
                    curl_mime_name(part, "content");
                    curl_mime_data(part, content_data, CURL_ZERO_TERMINATED);
                    dog_free(content_data);
                }
            }
            
            part = curl_mime_addpart(mime);
            curl_mime_name(part, "file");
            curl_mime_filedata(part, args);
            curl_mime_filename(part, filename);
            
            curl_easy_setopt(curl, CURLOPT_URL, dogconfig.dog_toml_webhooks);
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
            
            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
            
            curl_mime_free(mime);
            curl_easy_cleanup(curl);
        }
        
        curl_global_cleanup();
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, "watchdogs") == 0 || strcmp(ptr_command, "dog") == 0) {
        unit_show_dog();
        ret_code = -1;
        goto cleanup;
        
    } else if (strcmp(ptr_command, command_similar) != 0 && dist <= 2) {
        dog_console_title("Watchdogs | @ undefined");
        println(stdout, "watchdogs: '%s' is not valid watchdogs command. See 'help'.", ptr_command);
        println(stdout, "   but did you mean '%s'?", command_similar);
        ret_code = -1;
        goto cleanup;
        
    } else {
        size_t command_len = strlen(ptr_command) + DOG_PATH_MAX;
        command = dog_malloc(command_len);
        if (!command) {
            ret_code = -1;
            goto cleanup;
        }
        
        if (strfind(ptr_command, "clear", true) && is_running_in_wintive()) {
            free(ptr_command);
            ptr_command = strdup("cls");
        }
        
        if (path_access("/bin/sh") != 0) {
            snprintf(command, command_len, "/bin/sh -c '%s'", ptr_command);
        } else if (path_access("~/.bashrc") != 0) {
            snprintf(command, command_len, "bash -c '%s'", ptr_command);
        } else if (path_access("~/.zshrc") != 0) {
            snprintf(command, command_len, "zsh -c '%s'", ptr_command);
        } else {
            snprintf(command, command_len, "%s", ptr_command);
        }
        
        char *argv[32];
        int argc = 0;
        char *p = strtok(command, " ");
        while (p && argc < sizeof(argv) - 1) {
            argv[argc++] = p;
            p = strtok(NULL, " ");
        }
        argv[argc] = NULL;
        
        int ret = dog_exec_command(argv);
        if (ret) dog_console_title("Watchdogs | @ command not found");
        
        if (strcmp(ptr_command, "clear") == 0 || strcmp(ptr_command, "cls") == 0) {
            ret_code = -2;
        } else {
            ret_code = -1;
        }
        goto cleanup;
    }

cleanup:
    cleanup_local_resources(&ptr_prompt, &ptr_command, &title_running_info, &command,
                           &timestamp, &size_command, &platform, &pointer_signalA,
                           &debug_endpoint, &__args, &compile_target);
    return (ret_code);
}

void
unit_ret_main(void *unit_pre_command)
{
    int ret = -3;
    if (unit_pre_command != NULL) {
        char *procure_command_argv = strdup((char *)unit_pre_command);
        if (procure_command_argv) {
            ret = __command__(procure_command_argv);
            free(procure_command_argv);
        }
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        if (ret == -2 || ret == 3) {
            return;
        }
        return;
    }

loop_main:
    ret = __command__(NULL);
    if (ret == -1) {
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        command_dur = ((double)(cmd_end.tv_sec - cmd_start.tv_sec)) +
                     ((double)(cmd_end.tv_nsec - cmd_start.tv_nsec)) / 1e9;
        pr_color(stdout, DOG_COL_CYAN, " <I> (interactive) Finished at %.3fs\n", command_dur);
        goto loop_main;
    } else if (ret == 2) {
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        dog_console_title("Terminal.");
        
        clear_history();
        
        if (dogconfig.dog_ptr_samp) { free(dogconfig.dog_ptr_samp); dogconfig.dog_ptr_samp = NULL; }
        if (dogconfig.dog_ptr_omp) { free(dogconfig.dog_ptr_omp); dogconfig.dog_ptr_omp = NULL; }
        if (dogconfig.dog_toml_os_type) { free(dogconfig.dog_toml_os_type); dogconfig.dog_toml_os_type = NULL; }
        if (dogconfig.dog_toml_server_binary) { free(dogconfig.dog_toml_server_binary); dogconfig.dog_toml_server_binary = NULL; }
        if (dogconfig.dog_toml_server_config) { free(dogconfig.dog_toml_server_config); dogconfig.dog_toml_server_config = NULL; }
        if (dogconfig.dog_toml_server_logs) { free(dogconfig.dog_toml_server_logs); dogconfig.dog_toml_server_logs = NULL; }
        if (dogconfig.dog_toml_all_flags) { free(dogconfig.dog_toml_all_flags); dogconfig.dog_toml_all_flags = NULL; }
        if (dogconfig.dog_toml_root_patterns) { free(dogconfig.dog_toml_root_patterns); dogconfig.dog_toml_root_patterns = NULL; }
        if (dogconfig.dog_toml_packages) { free(dogconfig.dog_toml_packages); dogconfig.dog_toml_packages = NULL; }
        if (dogconfig.dog_toml_proj_input) { free(dogconfig.dog_toml_proj_input); dogconfig.dog_toml_proj_input = NULL; }
        if (dogconfig.dog_toml_proj_output) { free(dogconfig.dog_toml_proj_output); dogconfig.dog_toml_proj_output = NULL; }
        if (dogconfig.dog_toml_webhooks) { free(dogconfig.dog_toml_webhooks); dogconfig.dog_toml_webhooks = NULL; }
        if (compiler_full_includes) { free(compiler_full_includes); compiler_full_includes = NULL; }
        
        exit(0);
    } else if (ret == -2) {
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
        goto loop_main;
    } else if (ret == 3) {
        clock_gettime(CLOCK_MONOTONIC, &cmd_end);
    } else {
        goto basic_end;
    }

basic_end:
    clock_gettime(CLOCK_MONOTONIC, &cmd_end);
    command_dur = ((double)(cmd_end.tv_sec - cmd_start.tv_sec)) +
                 ((double)(cmd_end.tv_nsec - cmd_start.tv_nsec)) / 1e9;
    pr_color(stdout, DOG_COL_CYAN, " <I> (interactive) Finished at %.3fs\n", command_dur);
    goto loop_main;
}

int
main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc > 1) {
        int i;
        size_t unit_total_len = 0;

        for (i = 1; i < argc; ++i)
            unit_total_len += strlen(argv[i]) + 1;

        char *unit_size_prompt = dog_malloc(unit_total_len);
        if (!unit_size_prompt)
            return (0);

        char *ptr = unit_size_prompt;
        for (i = 1; i < argc; ++i) {
            if (i > 1)
                *ptr++ = ' ';
            size_t len = strlen(argv[i]);
            memcpy(ptr, argv[i], len);
            ptr += len;
        }
        *ptr = '\0';

        unit_ret_main(unit_size_prompt);

        dog_free(unit_size_prompt);
        unit_size_prompt = NULL;

        return (0);
    } else {
        unit_ret_main(NULL);
    }

    return (0);
}
