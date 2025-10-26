#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <readline/readline.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "extra.h"
#include "utils.h"
#include "chain.h"
#include "package.h"
#include "compiler.h"

int wd_RunCompiler(const char *arg, const char *compile_args)
{
        /* Determine the compiler binary name based on operating system */
        const char *ptr_pawncc = NULL;
        if (wcfg.os_type == OS_SIGNAL_WINDOWS) 
        {
            /* Use Windows executable name */
            ptr_pawncc = "pawncc.exe";
        }
        else if (wcfg.os_type == OS_SIGNAL_LINUX)
        {
            /* Use Linux binary name */
            ptr_pawncc = "pawncc";
        }
        
        /* Set include path based on configuration flag */
        char *path_include = NULL;
        if (wcfg.f_openmp == VAL_TRUE)
        {
            /* Use pawno include directory */
            path_include="pawno/include";
        }
        else if (wcfg.f_openmp == VAL_TRUE)
        {
            /* Use qawno include directory */
            path_include="qawno/include";
        }

        /* Clean up previous compiler log if it exists */
        int _wd_log_acces = path_acces(".wd_compiler.log");
        if (_wd_log_acces)
        {
            /* Remove existing compiler log file */
            remove(".wd_compiler.log");
        }

        /* Initialize output container buffer */
        char container_output[PATH_MAX] = {0};

        /* Reset file directory search */
        wd_sef_fdir_reset();

        /* Search for the pawncc compiler in current directory */
        int __find_pawncc = wd_sef_fdir(".", ptr_pawncc, NULL);
        if (__find_pawncc) 
        {
            /* Allocate memory for compiler command string */
            size_t format_size_compiler = 1024;
            char *_compiler_ = wdmalloc(format_size_compiler);
            if (!_compiler_) 
            {
#if defined(_DBG_PRINT)
                /* Debug print for memory allocation failure */
                printf_error("memory allocation failed for _compiler_!");
#endif
                return RETZ;
            }

            /* Open and parse the TOML configuration file */
            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (!procc_f) 
            {
                /* Handle TOML file read error */
                printf_error("Can't read file %s", "watchdogs.toml");
                if (_compiler_) 
                {
                    /* Clean up allocated memory */
                    wdfree(_compiler_);
                    _compiler_ = NULL;
                }
                return RETZ;
            }
            
            /* Parse TOML configuration */
            char errbuf[256];
            toml_table_t *_toml_config;
            _toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));

            /* Close file after parsing */
            if (procc_f)
            {
                fclose(procc_f);
            }
    
            /* Check if TOML parsing was successful */
            if (!_toml_config) 
            {
                /* Handle TOML parsing error */
                printf_error("parsing TOML: %s", errbuf);    
                if (_compiler_) 
                {
                    /* Clean up allocated memory */
                    wdfree(_compiler_);
                    _compiler_ = NULL;
                }
                return RETZ;
            }
            
            /* Extract compiler configuration from TOML */
            toml_table_t *wd_compiler = toml_table_in(_toml_config, "compiler");
            if (wd_compiler) 
            {
                /* Process compiler options array from TOML */
                toml_array_t *option_arr = toml_array_in(wd_compiler, "option");
                if (option_arr) 
                {
                    /* Merge all compiler options into a single string */
                    size_t arr_sz = toml_array_nelem(option_arr);
                    char *merged = NULL;

                    for (size_t i = 0; i < arr_sz; i++) 
                    {
                        toml_datum_t val = toml_string_at(option_arr, i);
                        if (!val.ok) 
                        {
                            /* Skip invalid option entries */
                            continue;
                        }

                        /* Calculate new buffer size and reallocate */
                        size_t old_len = merged ? strlen(merged) : 0,
                               new_len = old_len + strlen(val.u.s) + 2;

                        char *tmp = wdrealloc(merged, new_len);
                        if (!tmp) 
                        {
                            /* Handle memory reallocation failure */
                            wdfree(merged);
                            wdfree(val.u.s);
                            merged = NULL;
                            break;
                        }
                        merged = tmp;

                        /* Append option to merged string */
                        if (!old_len) 
                        {
                            snprintf(merged, new_len, "%s", val.u.s);
                        }
                        else 
                        {
                            snprintf(merged + old_len, new_len - old_len, " %s", val.u.s);
                        }

                        /* Free temporary string */
                        wdfree(val.u.s);
                        val.u.s = NULL;
                    }

                    /* Store merged options in global configuration */
                    if (merged) 
                    {
                        wcfg.ci_options = merged;
                    }
                    else 
                    {
                        /* Set empty options if none were provided */
                        wcfg.ci_options = strdup("");
                    }
                }
            
                /* Process include paths from TOML configuration */
                char include_aio_path[PATH_MAX] = {0};
                toml_array_t *include_paths = toml_array_in(wd_compiler, "include_path");
                if (include_paths) 
                {
                    /* Build include path arguments for compiler */
                    int array_size = toml_array_nelem(include_paths);
    
                    for (int i = 0; i < array_size; i++) 
                    {
                        toml_datum_t path_val = toml_string_at(include_paths, i);
                        if (path_val.ok) 
                        {
                            /* Process and sanitize include path */
                            char __procc[250];
                            wd_strip_dot_fns(__procc, sizeof(__procc), path_val.u.s);
                            if (__procc[0] == '\0') 
                            {
                                /* Skip empty paths */
                                continue;
                            }
                            
                            /* Add space separator for multiple paths */
                            if (i > 0) 
                            {
                                size_t cur = strlen(include_aio_path);
                                if (cur < sizeof(include_aio_path) - 1) 
                                {
                                    snprintf(include_aio_path + cur,
                                                sizeof(include_aio_path) - cur,
                                                " ");
                                }
                            }
                            
                            /* Append include path argument */
                            size_t cur = strlen(include_aio_path);
                            if (cur < sizeof(include_aio_path) - 1) 
                            {
                                snprintf(include_aio_path + cur,
                                            sizeof(include_aio_path) - cur,
                                            "-i\"%s\"",
                                            __procc);
                            }
                            wdfree(path_val.u.s);
                        }
                    }
                }
    
                /* Handle compilation without specific file argument */
                if (arg == NULL || *arg == '\0' || (arg[0] == '.' && arg[1] == '\0')) 
                {
                    /* Get input file from TOML configuration */
                    toml_datum_t toml_gm_i = toml_string_in(wd_compiler, "input");
                    if (toml_gm_i.ok) 
                    {
                        wcfg.gm_input = strdup(toml_gm_i.u.s);
                        wdfree(toml_gm_i.u.s);
                        toml_gm_i.u.s = NULL;
                    }
                    
                    /* Get output file from TOML configuration */
                    toml_datum_t toml_gm_o = toml_string_in(wd_compiler, "output");
                    if (toml_gm_o.ok) 
                    {
                        wcfg.gm_output = strdup(toml_gm_o.u.s);
                        wdfree(toml_gm_o.u.s);
                        toml_gm_o.u.s = NULL;
                    }

                    /* Build compiler command for Windows */
#ifdef _WIN32
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "%s %s -o%s %s %s -i%s > .wd_compiler.log 2>&1",
                        wcfg.sef_found[0],                              // compiler binary
                        wcfg.gm_input,                                  // input file
                        wcfg.gm_output,                                 // output file
                        wcfg.ci_options,                                // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#else
                    /* Build compiler command for Unix-like systems */
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "\"%s\" \"%s\" -o\"%s\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                        wcfg.sef_found[0],                              // compiler binary
                        wcfg.gm_input,                                  // input file
                        wcfg.gm_output,                                 // output file
                        wcfg.ci_options,                                // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#endif

                    /* Check for snprintf errors */
                    if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                    {
                        printf_error("snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                    }
                    
                    /* Create window title with compilation info */
                    size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s",
                                                      wcfg.sef_found[0],
                                                      wcfg.gm_input,
                                                      wcfg.gm_output) + 1;
                    char *title_compiler_info = wdmalloc(needed);
                    if (!title_compiler_info) 
                    { 
                        return RETN; 
                    }
                    snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s",
                                                          wcfg.sef_found[0],
                                                          wcfg.gm_input,
                                                          wcfg.gm_output);
                    if (title_compiler_info) 
                    {
                        /* Set window/tab title */
                        wd_set_title(title_compiler_info);
                        wdfree(title_compiler_info);
                        title_compiler_info = NULL;
                    }

                    /* Measure compilation time */
                    struct timespec start, end;
                    double compiler_dur;

                    clock_gettime(CLOCK_MONOTONIC, &start);
                        /* Execute the compiler command */
                        wd_run_command(_compiler_);
                    clock_gettime(CLOCK_MONOTONIC, &end);
                        
                    /* Display compiler output if log file exists */
                    if (procc_f) 
                    {
                        print_file_to_terminal(".wd_compiler.log");
                    }

                    /* Construct output file path */
                    char _container_output[PATH_MAX + 128];
                    snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);
                    
                    /* Analyze compiler log for errors */
                    procc_f = fopen(".wd_compiler.log", "r");
                    if (procc_f) 
                    {
                        char line_buf[1024 + 1024];
                        int __has_error = 0;
                        while (fgets(line_buf, sizeof(line_buf), procc_f)) 
                        {
                            /* Check for error indicators in compiler output */
                            if (strstr(line_buf, "error") || strstr(line_buf, "Error")) 
                            {
                                __has_error = 1;
                            }
                        }
                        fclose(procc_f);
                        if (__has_error) 
                        {
                            /* Remove output file on compilation error */
                            if (_container_output[0] != '\0' && path_acces(_container_output)) 
                            {
                                remove(_container_output);
                            }
                            wcfg.compiler_error++;
                        } 
                        else 
                        {
                            /* Decrement error counter on successful compilation */
                            if (wcfg.compiler_error) 
                            {
                                wcfg.compiler_error--;
                            }
                        }
                    } 
                    else 
                    {
                        printf_error("Failed to open .wd_compiler.log");
                    }

                    /* Calculate and display compilation duration */
                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    printf_color(COL_YELLOW,
                        " ==> [P]Finished in %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                    /* Debug output for compiler command */
                    printf_color(COL_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                } 
                else 
                {
                    /* Handle compilation with specific file argument */
                    char __cp_direct_path[PATH_MAX] = {0};
                    char __cp_file_name[PATH_MAX] = {0};
                    char __cp_input_path[PATH_MAX] = {0};
                    char __cp_any_tmp[PATH_MAX] = {0};

                    /* Copy and parse the compile arguments */
                    strncpy(__cp_any_tmp, compile_args, sizeof(__cp_any_tmp) - 1);
                    __cp_any_tmp[sizeof(__cp_any_tmp) - 1] = '\0';

                    /* Extract directory and filename from path */
                    char *compiler_last_slash = NULL;
                    compiler_last_slash = strrchr(__cp_any_tmp, '/');

                    char *compiler_back_slash = NULL;
                    compiler_back_slash = strrchr(__cp_any_tmp, '\\');

                    /* Handle Windows backslashes */
                    if (compiler_back_slash &&
                        (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                    {
                        compiler_last_slash = compiler_back_slash;
                    }

                    /* Process path with directory separator */
                    if (compiler_last_slash) 
                    {
                        size_t __dir_len = (size_t)(compiler_last_slash - __cp_any_tmp);

                        /* Ensure directory path doesn't exceed buffer */
                        if (__dir_len >= sizeof(__cp_direct_path))
                        {
                            __dir_len = sizeof(__cp_direct_path) - 1;
                        }

                        memcpy(__cp_direct_path, __cp_any_tmp, __dir_len);
                        __cp_direct_path[__dir_len] = '\0';

                        /* Extract filename from path */
                        const char *__cp_filename_start = compiler_last_slash + 1;
                        size_t __cp_filename_len = strlen(__cp_filename_start);

                        /* Ensure filename doesn't exceed buffer */
                        if (__cp_filename_len >= sizeof(__cp_file_name))
                        {
                            __cp_filename_len = sizeof(__cp_file_name) - 1;
                        }

                        memcpy(__cp_file_name, __cp_filename_start, __cp_filename_len);
                        __cp_file_name[__cp_filename_len] = '\0';

                        /* Reconstruct full input path */
                        size_t total_needed = strlen(__cp_direct_path) + 1 +
                                    strlen(__cp_file_name) + 1;

                        if (total_needed > sizeof(__cp_input_path)) 
                        {
                            /* Fallback to gamemodes directory if path is too long */
                            strncpy(__cp_direct_path, "gamemodes",
                                sizeof(__cp_direct_path) - 1);
                            __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';

                            size_t __cp_max_file_name = sizeof(__cp_file_name) - 1;

                            if (__cp_filename_len > __cp_max_file_name) 
                            {
                                memcpy(__cp_file_name, __cp_filename_start,
                                    __cp_max_file_name);
                                __cp_file_name[__cp_max_file_name] = '\0';
                            }
                        }

                        /* Build the full input path */
                        if (snprintf(__cp_input_path, sizeof(__cp_input_path),
                                "%s/%s", __cp_direct_path, __cp_file_name) >=
                            (int)sizeof(__cp_input_path))
                        {
                            __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                        }

                    } 
                    else 
                    {
                        /* Handle filename without directory path */
                        strncpy(__cp_file_name, __cp_any_tmp,
                            sizeof(__cp_file_name) - 1);
                        __cp_file_name[sizeof(__cp_file_name) - 1] = '\0';

                        strncpy(__cp_direct_path, ".", sizeof(__cp_direct_path) - 1);
                        __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';

                        if (snprintf(__cp_input_path, sizeof(__cp_input_path),
                                "./%s", __cp_file_name) >=
                            (int)sizeof(__cp_input_path))
                        {
                            __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                        }
                    }

                    /* Search for the file in specified directory */
                    int __find_gamemodes_args = 0;
                    __find_gamemodes_args = wd_sef_fdir(__cp_direct_path,
                                        __cp_file_name, NULL);

                    /* Fallback search in gamemodes directory */
                    if (!__find_gamemodes_args &&
                        strcmp(__cp_direct_path, "gamemodes") != 0) 
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __cp_file_name, NULL);
                        if (__find_gamemodes_args) 
                        {
                            /* Update paths to use gamemodes directory */
                            strncpy(__cp_direct_path, "gamemodes",
                                sizeof(__cp_direct_path) - 1);
                            __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';

                            if (snprintf(__cp_input_path, sizeof(__cp_input_path),
                                    "gamemodes/%s", __cp_file_name) >=
                                (int)sizeof(__cp_input_path))
                            {
                                __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                            }

                            /* Update search results */
                            if (wcfg.sef_count > 0)
                            {
                                strncpy(wcfg.sef_found[wcfg.sef_count - 1],
                                    __cp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    /* Additional fallback for current directory files */
                    if (!__find_gamemodes_args &&
                        strcmp(__cp_direct_path, ".") == 0) 
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __cp_file_name, NULL);
                        if (__find_gamemodes_args) 
                        {
                            /* Update paths to use gamemodes directory */
                            strncpy(__cp_direct_path, "gamemodes",
                                sizeof(__cp_direct_path) - 1);
                            __cp_direct_path[sizeof(__cp_direct_path) - 1] = '\0';

                            if (snprintf(__cp_input_path, sizeof(__cp_input_path),
                                    "gamemodes/%s", __cp_file_name) >=
                                (int)sizeof(__cp_input_path))
                            {
                                __cp_input_path[sizeof(__cp_input_path) - 1] = '\0';
                            }

                            /* Update search results */
                            if (wcfg.sef_count > 0)
                            {
                                strncpy(wcfg.sef_found[wcfg.sef_count - 1],
                                    __cp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    /* Proceed if file was found */
                    if (__find_gamemodes_args) 
                    {
                        /* Prepare output filename by removing extension */
                        char __sef_path_sz[PATH_MAX];
                        snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.sef_found[1]);
                        char *f_EXT = strrchr(__sef_path_sz, '.');
                        if (f_EXT) 
                        {
                            *f_EXT = '\0';
                        }

                        snprintf(container_output, sizeof(container_output), "%s", __sef_path_sz);

                        /* Build compiler command for Windows */
#ifdef _WIN32
                        int ret_compiler = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "%s %s -o%s.amx %s %s -i%s > .wd_compiler.log 2>&1",
                            wcfg.sef_found[0],                              // compiler binary
                            wcfg.sef_found[1],                              // input file
                            container_output,                               // output file
                            wcfg.ci_options,                                // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#else
                        /* Build compiler command for Unix-like systems */
                        int ret_compiler = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "\"%s\" \"%s\" -o\"%s.amx\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                            wcfg.sef_found[0],                              // compiler binary
                            wcfg.sef_found[1],                              // input file
                            container_output,                               // output file
                            wcfg.ci_options,                                // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#endif

                        /* Check for snprintf errors */
                        if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                        {
                            printf_error("snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                        }

                        /* Create window title with compilation info */
                        size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                          wcfg.sef_found[0],
                                                          wcfg.sef_found[1],
                                                          container_output) + 1;
                        char *title_compiler_info = wdmalloc(needed);
                        if (!title_compiler_info) 
                        { 
                            return RETN; 
                        }
                        snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                              wcfg.sef_found[0],
                                                              wcfg.sef_found[1],
                                                              container_output);
                        if (title_compiler_info) 
                        {
                            /* Set window/tab title */
                            wd_set_title(title_compiler_info);
                            wdfree(title_compiler_info);
                            title_compiler_info = NULL;
                        }

                        /* Measure compilation time */
                        struct timespec start, end;
                        double compiler_dur;
                        
                        clock_gettime(CLOCK_MONOTONIC, &start);
                            /* Execute the compiler command */
                            wd_run_command(_compiler_);
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        
                        /* Display compiler output if log file exists */
                        if (procc_f) 
                        {
                            print_file_to_terminal(".wd_compiler.log");
                        }

                        /* Construct output file path */
                        char _container_output[PATH_MAX + 128];
                        snprintf(_container_output, sizeof(_container_output), "%s.amx", container_output);

                        /* Analyze compiler log for errors */
                        procc_f = fopen(".wd_compiler.log", "r");
                        if (procc_f) 
                        {
                            char line_buf[1024 + 1024];
                            int __has_error = 0;
                            while (fgets(line_buf, sizeof(line_buf), procc_f)) 
                            {
                                /* Check for error indicators in compiler output */
                                if (strstr(line_buf, "error") || strstr(line_buf, "Error")) 
                                {
                                    __has_error = 1;
                                }
                            }
                            fclose(procc_f);
                            if (__has_error) 
                            {
                                /* Remove output file on compilation error */
                                if (_container_output[0] != '\0' && path_acces(_container_output)) 
                                {
                                    remove(_container_output);
                                }
                                wcfg.compiler_error++;
                            } 
                            else 
                            {
                                /* Decrement error counter on successful compilation */
                                if (wcfg.compiler_error) 
                                {
                                    wcfg.compiler_error--;
                                }
                            }
                        } 
                        else 
                        {
                            printf_error("Failed to open .wd_compiler.log");
                        }

                        /* Calculate and display compilation duration */
                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        printf_color(COL_YELLOW,
                            " ==> [P]Finished in %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                        /* Debug output for compiler command */
                        printf_color(COL_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif

                    } 
                    else 
                    {
                        /* Handle case where file cannot be located */
                        printf_error("Can't locate:");
                        printf("\t%s\n", compile_args);
                        return RETZ;
                    }
                }

                /* Clean up TOML configuration */
                toml_free(_toml_config);
                if (_compiler_) 
                {
                    /* Free compiler command buffer */
                    wdfree(_compiler_);
                    _compiler_ = NULL;
                }
                
                return RETZ;
            }
        } 
        else 
        {
            /* Handle case where pawncc compiler is not found */
            printf_error("pawncc not found!");
    
            /* Prompt user to install the compiler */
            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");
    
            while (1) 
            {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) 
                {
                    /* User wants to install compiler - select platform */
                    char platform = 0;
ret_ptr:
                        println("Select platform:");
                        println("-> [L/l] Linux");
                        println("-> [W/w] Windows");
                        printf(" ^ work's in WSL/MSYS2\n");
                        println("-> [T/t] Termux");
                        printf("==> ");

                    if (scanf(" %c", &platform) != 1)
                    {
                        return RETZ;
                    }

                    if (platform == 'L' || platform == 'l')
                    {
                        /* Install for Linux */
                        wd_install_pawncc("linux");
                    }
                    else if (platform == 'W' || platform == 'w')
                    {
                        /* Install for Windows */
                        wd_install_pawncc("windows");
                    }
                    else if (platform == 'T' || platform == 't')
                    {
                        /* Install for Termux */
                        wd_install_pawncc("termux");
                    }
                    else 
                    {
                        /* Invalid platform selection */
                        printf_error("Invalid platform selection. use C^ to exit.");
                        goto ret_ptr;
                    }

                    /* Restart main function after installation */
                    __main(0);
                } 
                else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) 
                {
                    /* User declined installation */
                    break;
                } 
                else 
                {
                    /* Invalid input - prompt again */
                    printf_error("Invalid input. Please type Y/y to install or N/n to cancel.");
                    ptr_sigA = readline("install now? [Y/n]: ");
                }
            }
        }

        return RETN;
}