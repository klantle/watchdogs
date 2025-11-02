#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "color.h"
#include "utils.h"
#include "chain.h"
#include "package.h"
#include "compiler.h"

typedef struct {
        char *cause_trigger;
        char *cause_info;
} causeExplanation;
causeExplanation cause_list[] =
{
        {"expected token", "A required token is missing from the code."},
        {"only a single statement", "`case` can only contain one statement; use `{}` for multiple."},
        {"declaration of a local variable must appear in a compound block", "Local variables must be inside `{...}`."},
        {"is not implemented", "Function declared but not implemented; maybe missing `}`."},
        {"function may not have arguments", "`main()` cannot accept arguments."},
        {"must be assigned to an array", "String literals must be assigned to an array variable."},
        {"operator cannot be redefined", "Only specific operators can be redefined in Pawn."},
        {"must be a constant expression; assumed zero", "Array sizes and compiler directives require constants."},
        {"invalid array size", "Array size must be 1 or greater."},
        {"undefined symbol", "Symbol used but not declared."},
        {"initialization data exceeds declared size", "Array initialized with more elements than its size."},
        {"must be lvalue", "Symbol being modified must be assignable."},
        {"array assignment must be simple assignment", "Cannot combine array assignment with arithmetic."},
        {"break or continue is out of context", "`break`/`continue` only valid inside loops."},
        {"function heading differs from prototype", "Function definition does not match previous declaration."},
        {"invalid character constant", "Unknown escape sequence or multiple characters in single quotes."},
        {"invalid expression, assumed zero", "Compiler could not understand the expression."},
        {"array index out of bounds", "Index exceeds array size."},
        {"empty statement", "Pawn does not support empty statements; use `{}`."},
        {"too many function arguments", "Function call exceeds 64 arguments."},
        {"symbol is never used", "Variable/constant/function defined but never used."},
        {"symbol is assigned a value that is never used", "Assigned value is never read."},
        {"redundant code: constant expression is zero", "Conditional expression is always zero."},
        {"should return a value", "Function must return a value."},
        {"possibly unintended assignment", "Assignment may be unintended (`=` vs `==`)."},
        {"tag mismatch", "Type/tag of expression does not match expected."},
        {"expression has no effect", "Expression evaluated but result unused."},
        {"loose indentation", "Inconsistent indentation."},
        {"Function is deprecated", "Function is outdated; consider alternatives."},

        /* Additional errors/warnings (ringkas) */
        {"invalid function call", "Symbol is not a function or syntax is wrong."},
        {"no entry point", "No `main` or public function found."},
        {"invalid statement; not in switch", "`case`/`default` only valid in `switch`."},
        {"default case must be the last case", "`default` must be last in `switch`."},
        {"multiple defaults in switch", "Only one `default` allowed."},
        {"not a label", "`goto` targets non-label symbol."},
        {"invalid symbol name", "Symbol names must start with letter, `_`, or `@`."},
        {"symbol already defined", "Symbol redefined in same scope."},
        {"no matching #if", "`#else` or `#endif` without `#if`."},
        {"invalid subscript", "`[`/`]` used incorrectly with arrays."},
        {"compound statement not closed", "File ended before closing `}`."},
        {"unknown directive", "Line starting with `#` is invalid."},
        {"array must be indexed", "Array must reference specific element."},
        {"argument does not have a default value", "Function argument placeholder requires default value."},
        {"argument type mismatch", "Function argument type mismatch."},
        {"invalid string", "String not properly formed."},
        {"extra characters on line", "Extra characters after compiler directive."},
        {"constant symbol has no size", "Symbolic constant cannot use `sizeof`."},
        {"duplicate case label", "Duplicate `case` label in `switch`."},
        {"invalid ellipsis", "Cannot determine array size from `...`."},
        {"invalid combination of class specifiers", "Unsupported class specifier combination."},
        {"character constant exceeds range", "Character exceeds 8-bit limit."},
        {"positional parameters must precede", "Cannot mix named and positional params."},
        {"unknown array size", "Array sizes must be explicit."},
        {"array sizes do not match", "Source and destination arrays differ in size."},
        {"array dimensions do not match", "Array dimensions mismatch."},
        {"invalid line continuation", "Backslash used incorrectly."},
        {"invalid range", "Numeric range expression invalid."},
        {"start of function body without function header", "Function body starts without header."},
        
        /* Fatal errors */
        {"cannot read from file", "File not found or access denied."},
        {"cannot write to file", "Cannot write output file."},
        {"table overflow", "Internal compiler table exceeded."},
        {"insufficient memory", "Not enough memory."},
        {"invalid assembler instruction", "Invalid opcode in `#emit`."},
        {"numeric overflow", "Constant exceeds capacity."},
        {"too many error messages on one line", "Line generated too many errors."},
        {"codepage mapping file not found", "Codepage file missing."},
        {"invalid path", "Specified path invalid."},
        {"assertion failed", "Compile-time assertion failed."},
        {"user error", "`#error` directive encountered."},
        
        /* Warnings */
        {"is truncated to", "Symbol name truncated due to length."},
        {"redefinition of constant", "Constant/macro redefined."},
        {"number of arguments does not match", "Function call argument count mismatch."},
        {"redundant test: constant expression is non-zero", "Unnecessary test for non-zero constant."},
        {"array argument was intended", "Non-const array passed where `const` expected."},
        
        /* Syntax-specific */
        {"missing semicolon", "Expected semicolon."},
        {"unexpected end of file", "File ended unexpectedly."},
        {"illegal character", "Invalid character found."},
        {"too many nested includes", "Include nesting too deep."},
        {"recursive include", "File includes itself."},
        {"macro recursion too deep", "Macro expansion exceeded recursion depth."},
        {"division by zero", "Compile-time division by zero."},
        {"overflow in constant expression", "Constant expression overflow."},
        {"undefined macro", "Macro in #if/#elif undefined."},
        {"missing preprocessor argument", "Macro requires argument."},
        {"too many macro arguments", "Macro given too many arguments."},
        {"missing closing parenthesis", "Parenthesis not closed."},
        {"missing closing bracket", "Bracket not closed."},
        {"missing closing brace", "Brace not closed."},
        
        {NULL, NULL} // Sentinel
};

/**
 * find_error_description - Look up an error/warning explanation from compiler output
 * @line: A line of text from compiler output
 *
 * Return: Pointer to explanation string if found, NULL otherwise
 */
static const char *find_error_description(const char *line)
{
        int index;

        /* Iterate over the list of compiler causes */
        for (index = 0;
            cause_list[index].cause_trigger;
            index++) {
                if (strstr(line,
                           cause_list[index].cause_trigger))
                        return cause_list[index].cause_info;
        }

        /* No matching trigger found */
        return NULL;
}

/**
 * print_file_with_annotations - Print file contents with error/warning annotations
 * @filename: Compiler output file name
 *
 * Marks lines containing errors or warnings and prints explanations from cause_list
 */
void print_file_with_annotations(const char *filename)
{
        FILE *file;
        char buffer[1024];        /* Buffer to read each line */
        int warning_count = 0;
        int error_count = 0;

        file = fopen(filename, "r");
        if (!file) {
                printf("Cannot open file: %s\n", filename);
                return;
        }

        /* Read file line by line until EOF */
        while (fgets(buffer, sizeof(buffer), file)) {
                const char *description = NULL;
                const char *trigger_pos = NULL;
                int marker_pos = 0;
                int i;

                /* Print the original compiler line */
                printf("%s", buffer);

                /* Count warnings and errors */
                if (strstr(buffer, "warning"))
                        warning_count++;
                if (strstr(buffer, "error"))
                        error_count++;

                /* Look up explanation for this line */
                description = find_error_description(buffer);
                if (description) {

                        /* Find trigger position to place '^' marker */
                        for (i = 0;
                            cause_list[i].cause_trigger; i++)
                        {
                                trigger_pos = strstr(buffer, cause_list[i].cause_trigger);
                                if (trigger_pos) {
                                        marker_pos = trigger_pos - buffer;
                                        break;
                                }
                        }

                        /* Print spaces up to trigger position */
                        for (i = 0; i < marker_pos; i++)
                                printf(" ");

                        /* Print marker and explanation */
                        pr_color(stdout, FCOLOUR_BLUE, "^ %s :(", description);
                }
        }

        /* Print compile summary */
        printf("\nCOMPILE COMPLETE | WITH %d ERROR | %d WARNING\n",
               error_count, warning_count);

        fclose(file);
}

int wd_run_compiler(const char *arg, const char *compile_args)
{
        /* Determine the compiler binary name based on operating system */
        const char *ptr_pawncc = NULL;
        if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_WINDOWS)) 
        {
            /* Use Windows executable name */
            ptr_pawncc = "pawncc.exe";
        }
        else if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX))
        {
            /* Use Linux binary name */
            ptr_pawncc = "pawncc";
        }
        
        /* Set include path based on configuration flag */
        char *path_include = NULL;
        if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
        {
            /* Use pawno include directory */
            path_include="pawno/include";
        }
        else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
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
        char container_output[PATH_MAX] = { 0 };

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
                pr_error(stdout, "memory allocation failed for _compiler_!");
#endif
                return RETZ;
            }

            /* Open and parse the TOML configuration file */
            FILE *procc_f = fopen("watchdogs.toml", "r");
            if (!procc_f) 
            {
                /* Handle TOML file read error */
                pr_error(stdout, "Can't read file %s", "watchdogs.toml");
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
                pr_error(stdout, "parsing TOML: %s", errbuf);    
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
                        wcfg.wd_toml_aio_opt = merged;
                    }
                    else 
                    {
                        /* Set empty options if none were provided */
                        wcfg.wd_toml_aio_opt = strdup("");
                    }
                }
            
                /* Process include paths from TOML configuration */
                char include_aio_path[PATH_MAX] = { 0 };
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
                        wcfg.wd_toml_gm_input = strdup(toml_gm_i.u.s);
                        wdfree(toml_gm_i.u.s);
                        toml_gm_i.u.s = NULL;
                    }
                    
                    /* Get output file from TOML configuration */
                    toml_datum_t toml_gm_o = toml_string_in(wd_compiler, "output");
                    if (toml_gm_o.ok) 
                    {
                        wcfg.wd_toml_gm_output = strdup(toml_gm_o.u.s);
                        wdfree(toml_gm_o.u.s);
                        toml_gm_o.u.s = NULL;
                    }

                    /* Build compiler command for Windows */
#ifdef _WIN32
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "%s %s -o%s %s %s -i%s > .wd_compiler.log 2>&1",
                        wcfg.wd_sef_found_list[0],                      // compiler binary
                        wcfg.wd_toml_gm_input,                          // input file
                        wcfg.wd_toml_gm_output,                         // output file
                        wcfg.wd_toml_aio_opt,                           // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#else
                    /* Build compiler command for Unix-like systems */
                    int ret_compiler = snprintf(
                        _compiler_,
                        format_size_compiler,
                        "\"%s\" \"%s\" -o\"%s\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                        wcfg.wd_sef_found_list[0],                      // compiler binary
                        wcfg.wd_toml_gm_input,                          // input file
                        wcfg.wd_toml_gm_output,                         // output file
                        wcfg.wd_toml_aio_opt,                           // additional options
                        include_aio_path,                               // include search path
                        path_include                                    // include directory
                    );
#endif

                    /* Check for snprintf errors */
                    if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                    {
                        pr_error(stdout, "snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                    }
                    
                    /* Create window title with compilation info */
                    size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s",
                                                      wcfg.wd_sef_found_list[0],
                                                      wcfg.wd_toml_gm_input,
                                                      wcfg.wd_toml_gm_output) + 1;
                    char *title_compiler_info = wdmalloc(needed);
                    if (!title_compiler_info) 
                    { 
                        return RETN; 
                    }
                    snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s",
                                                          wcfg.wd_sef_found_list[0],
                                                          wcfg.wd_toml_gm_input,
                                                          wcfg.wd_toml_gm_output);
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
                        
                    /* Display compiler output with explanations if log file exists */
                    if (procc_f) 
                    {
                        print_file_with_annotations(".wd_compiler.log");
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
                            wcfg.wd_compiler_stat = 1;
                            } 
                            else 
                            {
                                /* Set error to Zero for successful compilation */
                                wcfg.wd_compiler_stat = 0;
                            }
                    } 
                    else 
                    {
                        pr_error(stdout, "Failed to open .wd_compiler.log");
                    }

                    /* Calculate and display compilation duration */
                    compiler_dur = (end.tv_sec - start.tv_sec)
                                        + (end.tv_nsec - start.tv_nsec) / 1e9;

                    pr_color(stdout, FCOLOUR_GREEN,
                        " ==> [P]Finished in %.3fs (%.0f ms)\n",
                        compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                    /* Debug output for compiler command */
                    pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                    printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                } 
                else 
                {
                    /* Handle compilation with specific file argument */
                    char __wcp_direct_path[PATH_MAX] = { 0 };
                    char __wcp_file_name[PATH_MAX] = { 0 };
                    char __wcp_input_path[PATH_MAX] = { 0 };
                    char __wcp_any_tmp[PATH_MAX] = { 0 };

                    /* Copy and parse the compile arguments */
                    strncpy(__wcp_any_tmp, compile_args, sizeof(__wcp_any_tmp) - 1);
                    __wcp_any_tmp[sizeof(__wcp_any_tmp) - 1] = '\0';

                    /* Extract directory and filename from path */
                    char *compiler_last_slash = NULL;
                    compiler_last_slash = strrchr(__wcp_any_tmp, '/');

                    char *compiler_back_slash = NULL;
                    compiler_back_slash = strrchr(__wcp_any_tmp, '\\');

                    /* Handle Windows backslashes */
                    if (compiler_back_slash &&
                        (!compiler_last_slash || compiler_back_slash > compiler_last_slash))
                    {
                        compiler_last_slash = compiler_back_slash;
                    }

                    /* Process path with directory separator */
                    if (compiler_last_slash) 
                    {
                        size_t __dir_len = (size_t)(compiler_last_slash - __wcp_any_tmp);

                        /* Ensure directory path doesn't exceed buffer */
                        if (__dir_len >= sizeof(__wcp_direct_path))
                        {
                            __dir_len = sizeof(__wcp_direct_path) - 1;
                        }

                        memcpy(__wcp_direct_path, __wcp_any_tmp, __dir_len);
                        __wcp_direct_path[__dir_len] = '\0';

                        /* Extract filename from path */
                        const char *__wcp_filename_start = compiler_last_slash + 1;
                        size_t __wcp_filename_len = strlen(__wcp_filename_start);

                        /* Ensure filename doesn't exceed buffer */
                        if (__wcp_filename_len >= sizeof(__wcp_file_name))
                        {
                            __wcp_filename_len = sizeof(__wcp_file_name) - 1;
                        }

                        memcpy(__wcp_file_name, __wcp_filename_start, __wcp_filename_len);
                        __wcp_file_name[__wcp_filename_len] = '\0';

                        /* Reconstruct full input path */
                        size_t total_needed = strlen(__wcp_direct_path) + 1 +
                                    strlen(__wcp_file_name) + 1;

                        if (total_needed > sizeof(__wcp_input_path)) 
                        {
                            /* Fallback to gamemodes directory if path is too long */
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            size_t __wcp_max_file_name = sizeof(__wcp_file_name) - 1;

                            if (__wcp_filename_len > __wcp_max_file_name) 
                            {
                                memcpy(__wcp_file_name, __wcp_filename_start,
                                    __wcp_max_file_name);
                                __wcp_file_name[__wcp_max_file_name] = '\0';
                            }
                        }

                        /* Build the full input path */
                        if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                "%s/%s", __wcp_direct_path, __wcp_file_name) >=
                            (int)sizeof(__wcp_input_path))
                        {
                            __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                        }

                    } 
                    else 
                    {
                        /* Handle filename without directory path */
                        strncpy(__wcp_file_name, __wcp_any_tmp,
                            sizeof(__wcp_file_name) - 1);
                        __wcp_file_name[sizeof(__wcp_file_name) - 1] = '\0';

                        strncpy(__wcp_direct_path, ".", sizeof(__wcp_direct_path) - 1);
                        __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                        if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                "./%s", __wcp_file_name) >=
                            (int)sizeof(__wcp_input_path))
                        {
                            __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                        }
                    }

                    /* Search for the file in specified directory */
                    int __find_gamemodes_args = 0;
                    __find_gamemodes_args = wd_sef_fdir(__wcp_direct_path,
                                        __wcp_file_name, NULL);

                    /* Fallback search in gamemodes directory */
                    if (!__find_gamemodes_args &&
                        strcmp(__wcp_direct_path, "gamemodes") != 0) 
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (__find_gamemodes_args) 
                        {
                            /* Update paths to use gamemodes directory */
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            /* Update search results */
                            if (wcfg.wd_sef_count > 0)
                            {
                                strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    /* Additional fallback for current directory files */
                    if (!__find_gamemodes_args &&
                        strcmp(__wcp_direct_path, ".") == 0) 
                    {
                        __find_gamemodes_args = wd_sef_fdir("gamemodes",
                                            __wcp_file_name, NULL);
                        if (__find_gamemodes_args) 
                        {
                            /* Update paths to use gamemodes directory */
                            strncpy(__wcp_direct_path, "gamemodes",
                                sizeof(__wcp_direct_path) - 1);
                            __wcp_direct_path[sizeof(__wcp_direct_path) - 1] = '\0';

                            if (snprintf(__wcp_input_path, sizeof(__wcp_input_path),
                                    "gamemodes/%s", __wcp_file_name) >=
                                (int)sizeof(__wcp_input_path))
                            {
                                __wcp_input_path[sizeof(__wcp_input_path) - 1] = '\0';
                            }

                            /* Update search results */
                            if (wcfg.wd_sef_count > 0)
                            {
                                strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count - 1],
                                    __wcp_input_path, MAX_SEF_PATH_SIZE);
                            }
                        }
                    }

                    /* Proceed if file was found */
                    if (__find_gamemodes_args) 
                    {
                        /* Prepare output filename by removing extension */
                        char __sef_path_sz[PATH_MAX];
                        snprintf(__sef_path_sz, sizeof(__sef_path_sz), "%s", wcfg.wd_sef_found_list[1]);
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
                            wcfg.wd_sef_found_list[0],                      // compiler binary
                            wcfg.wd_sef_found_list[1],                      // input file
                            container_output,                               // output file
                            wcfg.wd_toml_aio_opt,                           // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#else
                        /* Build compiler command for Unix-like systems */
                        int ret_compiler = snprintf(
                            _compiler_,
                            format_size_compiler,
                            "\"%s\" \"%s\" -o\"%s.amx\" \"%s\" %s -i\"%s\" > .wd_compiler.log 2>&1",
                            wcfg.wd_sef_found_list[0],                      // compiler binary
                            wcfg.wd_sef_found_list[1],                      // input file
                            container_output,                               // output file
                            wcfg.wd_toml_aio_opt,                           // additional options
                            include_aio_path,                               // include search path
                            path_include                                    // include directory
                        );
#endif

                        /* Check for snprintf errors */
                        if (ret_compiler < 0 || (size_t)ret_compiler >= (size_t)format_size_compiler)
                        {
                            pr_error(stdout, "snprintf() failed or buffer too small (needed %d bytes)", ret_compiler);
                        }

                        /* Create window title with compilation info */
                        size_t needed = snprintf(NULL, 0, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                          wcfg.wd_sef_found_list[0],
                                                          wcfg.wd_sef_found_list[1],
                                                          container_output) + 1;
                        char *title_compiler_info = wdmalloc(needed);
                        if (!title_compiler_info) 
                        { 
                            return RETN; 
                        }
                        snprintf(title_compiler_info, needed, "Watchdogs | @ compile | %s | %s | %s.amx",
                                                              wcfg.wd_sef_found_list[0],
                                                              wcfg.wd_sef_found_list[1],
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
                        
                        /* Display compiler output with explanations if log file exists */
                        if (procc_f) 
                        {
                            print_file_with_annotations(".wd_compiler.log");
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
                                wcfg.wd_compiler_stat = 1;
                            } 
                            else 
                            {
                                 /* Set error to Zero for successful compilation */
                                wcfg.wd_compiler_stat = 0;
                            }
                        } 
                        else 
                        {
                            pr_error(stdout, "Failed to open .wd_compiler.log");
                        }

                        /* Calculate and display compilation duration */
                        compiler_dur = (end.tv_sec - start.tv_sec)
                                            + (end.tv_nsec - start.tv_nsec) / 1e9;

                        pr_color(stdout, FCOLOUR_GREEN,
                            " ==> [P]Finished in %.3fs (%.0f ms)\n",
                            compiler_dur, compiler_dur * 1000.0);
#if defined(_DBG_PRINT)
                        /* Debug output for compiler command */
                        pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING\n");
                        printf("[COMPILER]:\n\t%s\n", _compiler_);
#endif
                    } 
                    else 
                    {
                        /* Handle case where file cannot be located */
                        pr_error(stdout, "Cannnot locate:");
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
            pr_crit(stdout, "pawncc not found!");
    
            /* Prompt user to install the compiler */
            char *ptr_sigA;
            ptr_sigA = readline("install now? [Y/n]: ");
    
            while (1) 
            {
                if (strcmp(ptr_sigA, "Y") == 0 || strcmp(ptr_sigA, "y") == 0) 
                {
                    wdfree(ptr_sigA);
                    /* User wants to install compiler - select platform */
ret_ptr:
                    println(stdout, "Select platform:");
                    println(stdout, "-> [L/l] Linux");
                    println(stdout, "-> [W/w] Windows");
                    pr_color(stdout, FCOLOUR_YELLOW, " ^ work's in WSL/MSYS2\n");
                    println(stdout, "-> [T/t] Termux");

                    wcfg.wd_sel_stat = 1;

                    char *platform = readline("==> ");

                    if (strfind(platform, "L"))
                    {
                        /* Install for Linux */
                        int ret = wd_install_pawncc("linux");
loop_ipcc:
                        wdfree(platform);
                        if (ret == -RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc;
                    }
                    if (strfind(platform, "W"))
                    {
                        /* Install for Windows */
                        int ret = wd_install_pawncc("windows");
loop_ipcc2:
                        wdfree(platform);
                        if (ret == -RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc2;
                    }
                    if (strfind(platform, "T"))
                    {
                        /* Install for Termux */
                        int ret = wd_install_pawncc("termux");
loop_ipcc3:
                        wdfree(platform);
                        if (ret == -RETN && wcfg.wd_sel_stat != 0)
                            goto loop_ipcc3;
                    }
                    if (strfind(platform, "E")) {
                        wdfree(platform);
                        goto done;
                    }
                    else
                    {
                        /* Invalid platform selection */
                        pr_error(stdout, "Invalid platform selection. Input 'E/e' to exit");
                        wdfree(platform);
                        goto ret_ptr;
                    }

                    /* goto main function after installation */
done:
                    __main(0);
                } 
                else if (strcmp(ptr_sigA, "N") == 0 || strcmp(ptr_sigA, "n") == 0) 
                {
                    /* User declined installation */
                    wdfree(ptr_sigA);
                    break;
                } 
                else 
                {
                    /* Invalid input - prompt again */
                    pr_error(stdout, "Invalid input. Please type Y/y to install or N/n to cancel.");
                    wdfree(ptr_sigA);
                    goto ret_ptr;
                }
            }
        }

        return RETN;
}