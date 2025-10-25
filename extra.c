#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define PATH_SYM "\\"
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
#define MKDIR(path) _mkdir(path)
#define SLEEP(sec) Sleep((sec) * 1000)
#define SETENV(name, val, overwrite) _putenv_s(name, val)

static int wd_chmod(const char *path)
{
	int mode = _S_IREAD | _S_IWRITE;
	return _chmod(path, mode);
}
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#define PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
#define MKDIR(path) mkdir(path, 0755)
#define SLEEP(sec) sleep(sec)
#define SETENV(name, val, overwrite) setenv(name, val, overwrite)
#endif

#include "color.h"
#include "chain.h"
#include "utils.h"
#include "extra.h"

/**
 * println - Print formatted string with newline
 * @fmt: Format string
 * @...: Format arguments
 */
void println(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

/**
 * printf_color - Print colored formatted text
 * @color: ANSI color code
 * @format: Format string
 * @...: Format arguments
 */
void printf_color(const char *color, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf("%s", color);
	vprintf(format, args);
	printf("%s", COL_DEFAULT);
	va_end(args);
}

/**
 * printf_success - Print success message
 * @format: Format string
 * @...: Format arguments
 */
void printf_success(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf_color(COL_YELLOW, "success: ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

/**
 * printf_info - Print info message
 * @format: Format string
 * @...: Format arguments
 */
void printf_info(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf_color(COL_YELLOW, "info: ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

/**
 * printf_warning - Print warning message
 * @format: Format string
 * @...: Format arguments
 */
void printf_warning(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf_color(COL_GREEN, "warning: ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

/**
 * printf_error - Print error message
 * @format: Format string
 * @...: Format arguments
 */
void printf_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf_color(COL_RED, "error: ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

/**
 * printf_crit - Print critical error message
 * @format: Format string
 * @...: Format arguments
 */
void printf_crit(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf_color(COL_RED, "crit: ");
	vprintf(format, args);
	printf("\n");
	va_end(args);
}

/**
 * find_compiler_tools - Search for compiler tools in current directory
 * @compiler_type: Compiler type (SAMP or OPENMP)
 * @found_pawncc_exe: Output for pawncc.exe found status
 * @found_pawncc: Output for pawncc found status
 * @found_pawndisasm_exe: Output for pawndisasm.exe found status
 * @found_pawndisasm: Output for pawndisasm found status
 */
static void find_compiler_tools(int compiler_type,
				int *found_pawncc_exe, int *found_pawncc,
				int *found_pawndisasm_exe, int *found_pawndisasm)
{
	const char *ignore_dir = (compiler_type == COMPILER_SAMP) ? "pawno" : "qawno";

	*found_pawncc_exe = wd_sef_fdir(".", "pawncc.exe", ignore_dir);
	*found_pawncc = wd_sef_fdir(".", "pawncc", ignore_dir);
	*found_pawndisasm_exe = wd_sef_fdir(".", "pawndisasm.exe", ignore_dir);
	*found_pawndisasm = wd_sef_fdir(".", "pawndisasm", ignore_dir);
}

/**
 * get_compiler_directory - Get or create compiler directory
 *
 * Return: Directory path, NULL on failure
 */
static const char *get_compiler_directory(void)
{
	struct stat st;
	const char *dir_path = NULL;

	if (stat("pawno", &st) == 0 && S_ISDIR(st.st_mode)) {
		dir_path = "pawno";
	} else if (stat("qawno", &st) == 0 && S_ISDIR(st.st_mode)) {
		dir_path = "qawno";
	} else {
		/* Create default directory */
		if (MKDIR("pawno") == 0)
			dir_path = "pawno";
	}

	return dir_path;
}

/**
 * copy_compiler_tool - Copy compiler tool to destination
 * @src_path: Source file path
 * @tool_name: Tool filename
 * @dest_dir: Destination directory
 */
static void copy_compiler_tool(const char *src_path, const char *tool_name,
			       const char *dest_dir)
{
	char dest_path[PATH_MAX];

	snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
	wd_sef_wcopy(src_path, dest_path);
}

/**
 * setup_linux_library - Setup library on Linux systems
 *
 * Return: 0 on success, -1 on failure
 */
static int setup_linux_library(void)
{
#ifdef _WIN32
	return RETZ; /* Not applicable on Windows */
#else
	char libpawnc_src[PATH_MAX];
	char dest_path[PATH_MAX];
	const char *lib_paths[] = {
		"/data/data/com.termux/files/usr/lib/",
		"/data/data/com.termux/files/usr/local/lib/",
		"/usr/local/lib",
		"/usr/local/lib32"
	};
	const char *selected_path = NULL;
	struct stat st;
	int i, found_lib;

	if (wcfg.os_type != OS_SIGNAL_LINUX)
		return RETZ;

	/* Find libpawnc.so */
	found_lib = wd_sef_fdir(".", "libpawnc.so", NULL);
	if (!found_lib)
		return RETZ;

	/* Get source path */
	for (i = 0; i < wcfg.sef_count; i++) {
		if (strstr(wcfg.sef_found[i], "libpawnc.so")) {
			strncpy(libpawnc_src, wcfg.sef_found[i], sizeof(libpawnc_src));
			break;
		}
	}

	/* Find suitable library directory */
	for (i = 0; i < sizeof(lib_paths) / sizeof(lib_paths[0]); i++) {
		if (stat(lib_paths[i], &st) == 0 && S_ISDIR(st.st_mode)) {
			selected_path = lib_paths[i];
			break;
		}
	}

	if (!selected_path) {
		fprintf(stderr, "No valid library path found!\n");
		return -RETN;
	}

	/* Copy library */
	snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
	wd_sef_wmv(libpawnc_src, dest_path);

	/* Update library cache and environment */
	update_library_environment(selected_path);

	return RETZ;
#endif
}

/**
 * update_library_environment - Update library path environment
 * @lib_path: Library path that was used
 */
static void update_library_environment(const char *lib_path)
{
#ifndef _WIN32
	const char *old_path;
	char new_path[256];

	/* Run ldconfig if in system directory */
	if (strstr(lib_path, "/usr/local")) {
		int has_sudo = wd_run_command("which sudo > /dev/null 2>&1");
		if (has_sudo == 0)
			wd_run_command("sudo ldconfig");
		else
			wd_run_command("ldconfig");
	}

	/* Update LD_LIBRARY_PATH */
	old_path = getenv("LD_LIBRARY_PATH");
	if (old_path) {
		snprintf(new_path, sizeof(new_path), "%s:%s", lib_path, old_path);
	} else {
		snprintf(new_path, sizeof(new_path), "%s", lib_path);
	}

	SETENV("LD_LIBRARY_PATH", new_path, 1);
#endif
}

/**
 * wd_apply_pawncc - Install and setup pawn compiler tools
 */
void wd_apply_pawncc(void)
{
	int found_pawncc_exe, found_pawncc;
	int found_pawndisasm_exe, found_pawndisasm;
	const char *dest_dir;
	char pawncc_src[PATH_MAX] = {0};
	char pawncc_exe_src[PATH_MAX] = {0};
	char pawndisasm_src[PATH_MAX] = {0};
	char pawndisasm_exe_src[PATH_MAX] = {0};
	int i;

	wd_sef_fdir_reset();

	/* Find compiler tools */
	find_compiler_tools(wcfg.f_samp ? COMPILER_SAMP : COMPILER_OPENMP,
			    &found_pawncc_exe, &found_pawncc,
			    &found_pawndisasm_exe, &found_pawndisasm);

	/* Get or create destination directory */
	dest_dir = get_compiler_directory();
	if (!dest_dir) {
		printf_error("Failed to create compiler directory");
		goto error;
	}

	/* Collect source paths */
	for (i = 0; i < wcfg.sef_count; i++) {
		const char *entry = wcfg.sef_found[i];
		if (!entry)
			continue;

		if (strstr(entry, "pawncc.exe")) {
			strncpy(pawncc_exe_src, entry, sizeof(pawncc_exe_src));
		} else if (strstr(entry, "pawncc") && !strstr(entry, ".exe")) {
			strncpy(pawncc_src, entry, sizeof(pawncc_src));
		} else if (strstr(entry, "pawndisasm.exe")) {
			strncpy(pawndisasm_exe_src, entry, sizeof(pawndisasm_exe_src));
		} else if (strstr(entry, "pawndisasm") && !strstr(entry, ".exe")) {
			strncpy(pawndisasm_src, entry, sizeof(pawndisasm_src));
		}
	}

	/* Copy tools to destination */
	if (found_pawncc_exe && pawncc_exe_src[0])
		copy_compiler_tool(pawncc_exe_src, "pawncc.exe", dest_dir);

	if (found_pawncc && pawncc_src[0])
		copy_compiler_tool(pawncc_src, "pawncc", dest_dir);

	if (found_pawndisasm_exe && pawndisasm_exe_src[0])
		copy_compiler_tool(pawndisasm_exe_src, "pawndisasm.exe", dest_dir);

	if (found_pawndisasm && pawndisasm_src[0])
		copy_compiler_tool(pawndisasm_src, "pawndisasm", dest_dir);

	/* Setup library on Linux */
	setup_linux_library();

	printf_success("Compiler tools installed successfully");

error:
	printf_color(COL_YELLOW, "apply finished!\n");
	__main(0);
}