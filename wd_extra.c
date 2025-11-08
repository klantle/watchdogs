#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <sys/types.h>
  #include <sys/stat.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <errno.h>
#endif

#include "wd_crypto.h"
#include "wd_util.h"
#include "wd_extra.h"

char *BG = "\x1b[48;5;235m";
char *FG = "\x1b[97m";
char *BORD = "\x1b[33m";
char *RST = "\x1b[0m";

void println(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_color(FILE *stream, const char *color, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("%s", color);
		vprintf(format, args);
		printf("%s", FCOLOUR_DEFAULT);
		va_end(args);

		fflush(stream);
}

void printf_info(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[INFO]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_warning(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[WARNING]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_error(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[ERROR]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_crit(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[CRIT]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

/* Convert Windows FILETIME to time_t (seconds since UNIX epoch) */
#ifdef _WIN32
static time_t filetime_to_time_t(const FILETIME *ft) {
        /* FILETIME is in 100-nanosecond intervals since 1601-01-01.
           UNIX epoch is 1970-01-01: difference is 11644473600 seconds. */
        const uint64_t EPOCH_DIFF = 116444736000000000ULL; /* in 100-ns units */
        uint64_t v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
        if (v < EPOCH_DIFF) return (time_t)0;
        return (time_t)((v - EPOCH_DIFF) / 10000000ULL);
}
#endif

/* Portable stat function */
int portable_stat(const char *path, portable_stat_t *out) {
        if (!path || !out) return -1;
        memset(out, 0, sizeof(*out));

#ifdef _WIN32
        /* Use wide APIs for Unicode paths */
        wchar_t wpath[MAX_PATH];
        int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
        if (len == 0 || len > MAX_PATH) {
                /* fallback: try ANSI conversion */
                if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, MAX_PATH)) return -1;
        } else {
                MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_PATH);
        }

        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &fad)) {
                /* could be a long path, try CreateFile with \\?\ prefix? omitted for brevity */
                return -1;
        }

        /* size */
        uint64_t size = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        out->st_size = size;

        /* times */
        out->st_latime = filetime_to_time_t(&fad.ftLastAccessTime);
        out->st_lmtime = filetime_to_time_t(&fad.ftLastWriteTime);
        /* On Windows there's creation time and last metadata change isn't available like POSIX ctime.
           Use creation time as a proxy for st_ctime. */
        out->st_mctime = filetime_to_time_t(&fad.ftCreationTime);

        /* mode emulation: file/dir and simple permissions (read-only) */
        DWORD attrs = fad.dwFileAttributes;
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                out->st_mode |= S_IFDIR;
        } else {
                out->st_mode |= S_IFREG;
        }

        /* read-only bit -> emulate owner write bit */
        if (attrs & FILE_ATTRIBUTE_READONLY) {
                out->st_mode |= S_IRUSR;
                /* no write */
        } else {
                out->st_mode |= (S_IRUSR | S_IWUSR);
        }
        /* executable bit: heuristic by extension (common on Windows) */
        const char *ext = strrchr(path, '.');
        if (ext && (_stricmp(ext, ".exe") == 0 || _stricmp(ext, ".bat") == 0 || _stricmp(ext, ".com") == 0)) {
                out->st_mode |= S_IXUSR;
        }

        /* inode/device: not meaningful on Windows; you could use file index via GetFileInformationByHandle if needed */
        out->st_ino = 0;
        out->st_dev = 0;

        return 0;
#else
        struct stat st;
        if (stat(path, &st) != 0) return -1;
        out->st_size = (uint64_t)st.st_size;
        out->st_ino  = (uint64_t)st.st_ino;
        out->st_dev  = (uint64_t)st.st_dev;
        out->st_mode = (unsigned int)st.st_mode;
        out->st_latime = st.st_atime;
#ifdef __APPLE__
        out->st_lmtime = st.st_mtime;
        out->st_mctime = st.st_ctime;
#else
        out->st_lmtime = st.st_mtime;
        out->st_mctime = st.st_ctime;
#endif
        return 0;
#endif
}

causeExplanation ccs[] =
{
/* === SYNTAX ERRORS (Invalid Code Structure) === */

/* A key language element (like ',' or ';') is missing. */
{"expected token", "A required token (e.g., ';', ',', ')') is missing from the code. Check the line indicated for typos or omissions."},

/* The 'case' label can only be followed by a single statement. */
{"only a single statement", "A `case` label can only be followed by a single statement. To use multiple statements, enclose them in a block with `{` and `}`."},

/* Variables declared inside case/default must be inside a block. */
{"declaration of a local variable must appear in a compound block", "Local variables declared inside a `case` or `default` label must be scoped within a compound block `{ ... }`."},

/* Function call or declaration is incorrect. */
{"invalid function call", "The symbol being called is not a function, or the syntax of the function call is incorrect."},

/* Statements like 'case' or 'default' are used outside a switch block. */
{"invalid statement; not in switch", "The `case` or `default` label is only valid inside a `switch` statement."},

/* The 'default' case is not the last one in the switch block. */
{"default case must be the last case", "The `default` case within a `switch` statement must appear after all other `case` labels."},

/* More than one 'default' case found in a single switch. */
{"multiple defaults in switch", "A `switch` statement can only have one `default` case."},

/* The target of a 'goto' statement is not a valid label. */
{"not a label", "The symbol used in the `goto` statement is not defined as a label."},

/* The symbol name does not follow the language's naming rules. */
{"invalid symbol name", "Symbol names must start with a letter, an underscore '_', or an 'at' sign '@'."},

/* An empty statement (just a semicolon) is not allowed in certain contexts. */
{"empty statement", "Pawn does not support a standalone semicolon as an empty statement. double `;;`? - Use an empty block `{}` instead if intentional."},

/* A required semicolon is missing. */
{"missing semicolon", "A semicolon ';' is expected at the end of this statement."},

/* The file ended before a structure was properly closed. */
{"unexpected end of file", "The source file ended unexpectedly. This is often caused by a missing closing brace '}', parenthesis ')', or quote."},

/* An invalid character was encountered in the source code. */
{"illegal character", "A character was found that is not valid in the current context (e.g., outside of a string or comment)."},

/* Brackets or braces are not properly matched. */
{"missing closing parenthesis", "An opening parenthesis '(' was not closed with a matching ')'."},
{"missing closing bracket", "An opening bracket '[' was not closed with a matching ']'."},
{"missing closing brace", "An opening brace '{' was not closed with a matching '}'."},



/* === SEMANTIC ERRORS (Invalid Code Meaning) === */

/* A function was declared but its body was never defined. */
{"is not implemented", "A function was declared but its implementation (body) was not found. This can be caused by a missing closing brace `}` in a previous function."},

/* The main function is defined with parameters, which is not allowed. */
{"function may not have arguments", "The `main()` function cannot accept any arguments."},

/* A string literal is being used incorrectly. */
{"must be assigned to an array", "String literals must be assigned to a character array variable; they cannot be assigned to non-array types."},

/* An attempt was made to overload an operator that cannot be overloaded. */
{"operator cannot be redefined", "Only specific operators can be redefined (overloaded) in Pawn. Check the language specification."},

/* A context requires a constant value, but a variable or non-constant expression was provided. */
{"must be a constant expression; assumed zero", "Array sizes and compiler directives (like `#if`) require constant expressions (literals or declared constants)."},

/* An array was declared with a size of zero or a negative number. */
{"invalid array size", "The size of an array must be 1 or greater."},

/* A symbol (variable, function, constant) is used but was never declared. */
{"undefined symbol", "A symbol is used in the code but it has not been declared. Check for typos or missing declarations."},

/* The initializer for an array has more elements than the array's declared size. */
{"initialization data exceeds declared size", "The data used to initialize the array contains more elements than the size specified for the array."},

/* The left-hand side of an assignment is not something that can be assigned to. */
{"must be lvalue", "The symbol on the left side of an assignment operator must be a modifiable variable, array element, or other 'lvalue'."},

/* Arrays cannot be used in compound assignments. */
{"array assignment must be simple assignment", "Arrays cannot be used with compound assignment operators (like `+=`). Only simple assignment `=` is allowed for arrays."},

/* 'break' or 'continue' is used outside of any loop or switch statement. */
{"break or continue is out of context", "The `break` statement is only valid inside loops (`for`, `while`, `do-while`) and `switch`. `continue` is only valid inside loops."},

/* A function's definition does not match its previous declaration. */
{"function heading differs from prototype", "The function's definition (return type, name, parameters) does not match a previous declaration of the same function."},

/* An invalid character is used inside single quotes. */
{"invalid character constant", "An unknown escape sequence (like `\\z`) was used, or multiple characters were placed inside single quotes (e.g., 'ab')."},

/* The compiler could not parse the expression. */
{"invalid expression, assumed zero", "The compiler could not understand the structure of the expression. This is often due to a syntax error or operator misuse."},

/* An array index is outside the valid range for the array. */
{"array index out of bounds", "The index used to access the array is either negative or greater than or equal to the array's size."},

/* A function call has more than the allowed number of arguments. */
{"too many function arguments", "A function call was made with more than 64 arguments, which is the limit in Pawn."},

/* A symbol is defined but never used in the code. */
{"symbol is never used", "A variable, constant, or function was defined but never referenced in the code. This may indicate dead code or a typo."},

/* A value is assigned to a variable, but that value is never read later. */
{"symbol is assigned a value that is never used", "A variable is assigned a value, but that value is never used in any subsequent operation. This might indicate unnecessary computation."},

/* A conditional expression is always false. */
{"redundant code: constant expression is zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to zero (false), making the code block unreachable."},

/* A non-void function does not return a value on all control paths. */
{"should return a value", "A function that is declared to return a value must return a value on all possible execution paths."},

/* An assignment is used in a boolean context, which might be a mistake for '=='. */
{"possibly unintended assignment", "An assignment operator `=` was used in a context where a comparison operator `==` is typically used (e.g., in an `if` condition)."},

/* There is a type mismatch between two expressions. */
{"tag mismatch", "The type (or 'tag') of the expression does not match the type expected by the context. Check variable types and function signatures."},

/* An expression is evaluated but its result is not used or stored. */
{"expression has no effect", "An expression is evaluated but does not change the program's state (e.g., `a + b;` on a line by itself). This is often a logical error."},

/* The indentation in the source code is inconsistent. */
{"loose indentation", "The indentation (spaces/tabs) is inconsistent. While this doesn't affect compilation, it harms code readability."},

/* A function is marked as deprecated and should not be used. */
{"Function is deprecated", "This function is outdated and may be removed in future versions. The compiler suggests using an alternative."},

/* No valid entry point (main function) was found for the program. */
{"no entry point", "The program must contain a `main` function or another designated public function to serve as the entry point."},

/* A symbol is defined more than once in the same scope. */
{"symbol already defined", "A symbol (variable, function, etc.) is being redefined in the same scope. You cannot have two symbols with the same name in the same scope."},

/* Array indexing is used incorrectly. */
{"invalid subscript", "The bracket operators `[` and `]` are being used incorrectly, likely with a variable that is not an array."},

/* An array name is used without an index. */
{"array must be indexed", "An array variable is used in an expression without an index. You must specify which element of the array you want to access."},

/* A function argument placeholder lacks a default value. */
{"argument does not have a default value", "In a function call with named arguments, a placeholder was used for an argument that does not have a default value specified."},

/* The type of an argument in a function call does not match the function's parameter type. */
{"argument type mismatch", "The type of an argument passed to a function does not match the expected parameter type defined by the function."},

/* A string literal is malformed. */
{"invalid string", "A string literal is not properly formed, often due to a missing closing quote or an invalid escape sequence."},

/* A symbolic constant is used with the sizeof operator. */
{"constant symbol has no size", "The `sizeof` operator cannot be applied to a symbolic constant. It is only for variables and types."},

/* Two 'case' labels in the same switch have the same value. */
{"duplicate case label", "Two `case` labels within the same `switch` statement have the same constant value. Each `case` must be unique."},

/* The ellipsis (...) is used in an invalid context for array sizing. */
{"invalid ellipsis", "The compiler cannot determine the array size from the `...` initializer syntax."},

/* Incompatible class specifiers are used together. */
{"invalid combination of class specifiers", "A combination of storage class specifiers (e.g., `public`, `static`) is used that is not allowed by the language."},

/* A character value exceeds the 8-bit range. */
{"character constant exceeds range", "A character constant has a value that is outside the valid 0-255 range for an 8-bit character."},

/* Named and positional parameters are mixed incorrectly in a function call. */
{"positional parameters must precede", "In a function call, all positional arguments must come before any named arguments."},

/* An array is declared without a specified size. */
{"unknown array size", "An array was declared without a specified size and without an initializer to infer the size. Array sizes must be explicit."},

/* Array sizes in an assignment do not match. */
{"array sizes do not match", "In an assignment, the source and destination arrays have different sizes."},

/* Array dimensions in an operation do not match. */
{"array dimensions do not match", "The dimensions of the arrays used in an operation (e.g., addition) do not match."},

/* A backslash is used at the end of a line incorrectly. */
{"invalid line continuation", "A backslash `\\` was used at the end of a line, but it is not being used to continue a preprocessor directive or string literal correctly."},

/* A numeric range expression is invalid. */
{"invalid range", "A range expression (e.g., in a state array) is syntactically or logically invalid."},

/* A function body is found without a corresponding function header. */
{"start of function body without function header", "A block of code `{ ... }` that looks like a function body was encountered, but there was no preceding function declaration."},



/* === FATAL ERRORS (Compiler Failure) === */

/* The compiler cannot open the source file or an included file. */
{"cannot read from file", "The specified source file or an included file could not be opened. It may not exist, or there may be permission issues."},

/* The compiler cannot write the output file (e.g., the compiled .amx). */
{"cannot write to file", "The compiler cannot write to the output file. The disk might be full, the file might be in use, or there may be permission issues."},

/* An internal compiler data structure has exceeded its capacity. */
{"table overflow", "An internal compiler table (for symbols, tokens, etc.) has exceeded its maximum size. The source code might be too complex."},

/* The system ran out of memory during compilation. */
{"insufficient memory", "The compiler ran out of available system memory (RAM) while processing the source code."},

/* An invalid opcode is used in an #emit directive. */
{"invalid assembler instruction", "The opcode specified in an `#emit` directive is not a valid Pawn assembly instruction."},

/* A numeric constant is too large for the compiler to handle. */
{"numeric overflow", "A numeric constant in the source code is too large to be represented."},

/* A single line of code produced too many errors. */
{"too many error messages on one line", "One line of source code generated a large number of errors. The compiler is stopping to avoid flooding the output."},

/* A codepage mapping file specified by the user was not found. */
{"codepage mapping file not found", "The file specified for character set conversion (codepage) could not be found."},

/* The provided file or directory path is invalid. */
{"invalid path", "A file or directory path provided to the compiler is syntactically invalid or does not exist."},

/* A compile-time assertion (#assert) failed. */
{"assertion failed", "A compile-time assertion check (using `#assert`) evaluated to false."},

/* The #error directive was encountered, forcing the compiler to stop. */
{"user error", "The `#error` preprocessor directive was encountered, explicitly halting the compilation process."},



/* === WARNINGS (Potentially Problematic Code) === */

/* literal array/string passed to a non-const parameter */
{"literal array/string passed to a non", "Did you forget that the parameter isn’t a const parameter? Also, make sure you’re using the latest version of the standard library."},

/* A symbol name is too long and is being truncated. */
{"is truncated to", "A symbol name (variable, function, etc.) exceeds the maximum allowed length and will be truncated, which may cause link errors."},

/* A constant or macro is redefined with a new value. */
{"redefinition of constant", "A constant or macro is being redefined. The new definition will override the previous one."},

/* A function is called with the wrong number of arguments. */
{"number of arguments does not match", "The number of arguments in a function call does not match the number of parameters in the function's declaration."},

/* A conditional expression is always true. */
{"redundant test: constant expression is non-zero", "A conditional expression (e.g., in an `if` or `while`) always evaluates to a non-zero value (true), making the test unnecessary."},

/* A non-const qualified array is passed to a function expecting a const array. */
{"array argument was intended", "A non-constant array is being passed to a function parameter that is declared as `const`. The function promises not to modify it, so this is safe but noted."},



/* === PREPROCESSOR ERRORS === */

/* The maximum allowed depth for #include directives has been exceeded. */
{"too many nested includes", "The level of nested `#include` directives has exceeded the compiler's limit. Check for circular or overly deep inclusion."},

/* A file attempts to include itself, directly or indirectly. */
{"recursive include", "A file is including itself, either directly or through a chain of other includes. This creates an infinite loop."},

/* Macro expansion has exceeded the recursion depth limit. */
{"macro recursion too deep", "The expansion of a recursive macro has exceeded the maximum allowed depth. Check for infinitely recursive macro definitions."},

/* A constant expression involves division by zero. */
{"division by zero", "A compile-time constant expression attempted to divide by zero."},

/* A constant expression calculation resulted in an overflow. */
{"overflow in constant expression", "A calculation in a constant expression (e.g., in an `#if` directive) resulted in an arithmetic overflow."},

/* A macro used in a conditional compilation directive is not defined. */
{"undefined macro", "A macro used in an `#if` or `#elif` directive has not been defined. Its value is assumed to be zero."},

/* A function-like macro is used without the required arguments. */
{"missing preprocessor argument", "A function-like macro was invoked without providing the required number of arguments."},

/* A function-like macro is given more arguments than it expects. */
{"too many macro arguments", "A function-like macro was invoked with more arguments than specified in its definition."},

/* Extra text found after a preprocessor directive. */
{"extra characters on line", "Unexpected characters were found on the same line after a preprocessor directive (e.g., `#include <file> junk`)."},

/* Sentinel value to mark the end of the array. */
{NULL, NULL}
};

static const char
*find_warning_error(const char *line)
{
        int index;
        for (index = 0;
            ccs[index].cs_t;
            index++) {
                if (strstr(line,
                           ccs[index].cs_t))
                        return ccs[index].cs_i;
        }
        return NULL;
}

void
annotations_compiler(const char *log_file, const char *pawn_output, int debug)
{
	    FILE *pf;
	    char buffer[MAX_PATH];
	    int wcnt = 0, ecnt = 0;
	    int header_size = 0, code_size = 0, data_size = 0;
	    int stack_size = 0, total_size = 0;
			char compiler_ver[64] = {0};

	    pf = fopen(log_file, "r");
	    if (!pf) {
	        printf("Cannot open file: %s\n", log_file);
	        return;
	    }

	    while (fgets(buffer, sizeof(buffer), pf)) {
	        const char *description = NULL;
	        const char *t_pos = NULL;
	        int mk_pos = 0;
	        int i;

	        if (strstr(buffer, "Warnings.") || strstr(buffer, "Warning.") ||
	            strstr(buffer, "Errors.") || strstr(buffer, "Error."))
	            continue;

	        if (strstr(buffer, "Header size:")) {
	            sscanf(buffer,
										"Header size: %d bytes", &header_size);
	            continue;
	        } else if (strstr(buffer, "Code size:")) {
	            sscanf(buffer,
										"Code size: %d bytes", &code_size);
	            continue;
	        } else if (strstr(buffer, "Data size:")) {
	            sscanf(buffer,
										"Data size: %d bytes", &data_size);
	            continue;
	        } else if (strstr(buffer, "Stack/heap size:")) {
	            sscanf(buffer,
										"Stack/heap size: %d bytes", &stack_size);
	            continue;
	        } else if (strstr(buffer, "Total requirements:")) {
	            sscanf(buffer,
										"Total requirements: %d bytes", &total_size);
	            continue;
	        }
					if (strstr(buffer, "Pawn compiler ")) {
							sscanf(buffer,
										"Pawn compiler %63s", compiler_ver);
							continue;
					}

	        printf("%s", buffer);

	        if (strstr(buffer, "warning"))
	            wcnt++;
	        if (strstr(buffer, "error"))
	            ecnt++;

	        description = find_warning_error(buffer);
	        if (description) {
	            for (i = 0; ccs[i].cs_t; i++) {
	                t_pos = strstr(buffer, ccs[i].cs_t);
	                if (t_pos) {
	                    mk_pos = t_pos - buffer;
	                    break;
	                }
	            }

	            for (i = 0; i < mk_pos; i++)
	                printf(" ");

	            pr_color(stdout,
	                     FCOLOUR_BLUE,
	                     "^ %s :(\n", description);
	        }
	    }

	    fclose(pf);

	    char __sz_compiler[256];
	    snprintf(__sz_compiler, sizeof(__sz_compiler),
	             "COMPILE COMPLETE | WITH %d ERROR | %d WARNING",
	             ecnt, wcnt);
	    wd_set_title(__sz_compiler);

			int amx_access = path_acces(pawn_output);
			if (amx_access && debug != 0) {
				printf("%-18s: %d bytes\n", "Header size", header_size);
				printf("%-18s: %d bytes\n", "Code size", code_size);
				printf("%-18s: %d bytes\n", "Data size", data_size);
				printf("%-18s: %d bytes\n", "Stack/heap size", stack_size);
				printf("%-18s: %d bytes\n", "Total requirements", total_size);

				unsigned long hash = djb2_hash_file(pawn_output);
				printf("%-18s: %lu (0x%lx)\n", "djb2 hash", hash, hash);

				portable_stat_t st;
				if (portable_stat(pawn_output, &st) == 0) {
					printf("%-18s: %llu bytes\n", "size", (unsigned long long)st.st_size);
					printf("%-18s: %llu\n", "ino", (unsigned long long)st.st_ino);
					printf("%-18s: %llu\n", "dev", (unsigned long long)st.st_dev);
					printf("%-18s: %020o\n", "mode", st.st_mode);
					printf("%-18s: %lld\n", "atime", (long long)st.st_latime);
					printf("%-18s: %lld\n", "mtime", (long long)st.st_lmtime);
					printf("%-18s: %lld\n", "ctime", (long long)st.st_mctime);
					printf("%-18s: %s\n", "isdir", (st.st_mode & S_IFDIR) ? "yes" : "no");
					printf("%-18s: %s\n", "isreg", (st.st_mode & S_IFREG) ? "yes" : "no");
					printf("%-18s: %s\n", "readable", (st.st_mode & S_IRUSR) ? "yes" : "no");
					printf("%-18s: %s\n", "writable", (st.st_mode & S_IWUSR) ? "yes" : "no");
					printf("%-18s: %s\n", "executable", (st.st_mode & S_IXUSR) ? "yes" : "no");
				}
			}
			printf("* Pawn Compiler %s - Copyright (c) 1997-2006, ITB CompuPhase\n", compiler_ver);
}
