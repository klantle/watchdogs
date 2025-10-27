
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define __PATH_SYM "\\"
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
#define mkdir(path) _mkdir(path)
#define MKDIR(path) mkdir(path)
#define Sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
static int _w_chmod(const char *path) {
        int mode = _S_IREAD | _S_IWRITE;
        return chmod(path, mode);
}
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#define __PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
#define MKDIR(path) mkdir(path, 0755)
#endif

#include <sys/file.h>
#include <sys/types.h>
#include <ncursesw/curses.h>
#include <math.h>
#include <limits.h>
#ifndef _WIN32
#define MAX_PATH PATH_MAX + 764
#endif
#include <time.h>
#include <ftw.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stddef.h>
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <archive.h>
#include <archive_entry.h>
#include <errno.h>

#include "tomlc99/toml.h"
#include "color.h"
#include "extra.h"
#include "utils.h"
#include "chain.h"

const char* __command[] = {
        "help",
        "clear",
        "exit",
        "kill",
        "title",
        "toml",
        "hardware",
        "install",
        "gamemode",
        "pawncc",
        "compile",
        "running",
        "crunn",
        "debug",
        "stop",
        "restart"
};
const size_t
        __command_len =
            sizeof(__command) /
            sizeof(__command[0]);

WatchdogConfig wcfg = {
		.ipackage = 0,
		.idepends = 0,
		.os = NULL,
		.os_type = VAL_FALSE,
		.f_samp = VAL_FALSE,
		.f_openmp = VAL_FALSE,
		.pointer_samp = NULL,
		.pointer_openmp = NULL,
		.compiler_error = 0,
		.sef_count = 0,
		.sef_found = { {0} },
		.serv_dbg = NULL,
		.ci_options = NULL,
		.gm_input = NULL,
		.gm_output = NULL
};

/**
 * wd_sef_fdir_reset - Reset found directory entries
 * 
 * Clears all entries in the sef_found array and resets the count.
 * Uses both iterative clearing and memset for completeness.
 */
void wd_sef_fdir_reset(void)
{
		size_t max_entries = sizeof(wcfg.sef_found) / 
						     sizeof(wcfg.sef_found[0]);
		size_t i;

		/* Clear each string individually */
		for (i = 0; i < max_entries; i++)
				wcfg.sef_found[i][0] = '\0';

		/* Ensure complete memory clearance */
		memset(wcfg.sef_found, 0, sizeof(wcfg.sef_found));
		
		/* Reset counter */
		wcfg.sef_count = 0;
}

/**
 * wd_signal_os - Get operating system signal code
 *
 * Returns a signal code based on the detected operating system.
 * Used for platform-specific signal handling.
 *
 * Return: 0x01 for Windows, VAL_FALSE for Linux, 0x02 for unknown
 */
int wd_signal_os(void)
{
		if (strcmp(wcfg.os, "windows") == 0)
				return OS_SIGNAL_WINDOWS;
		else if (strcmp(wcfg.os, "linux") == 0)
				return OS_SIGNAL_LINUX;

		return OS_SIGNAL_UNKNOWN;
}

/*
 * mkdir_recursive
 *
 * mkdir with parent directory
 */
int mkdir_recursive(const char *path) {
		char tmp[PATH_MAX];
		char *p = NULL;
		size_t len;

		snprintf(tmp, sizeof(tmp), "%s", path);
		len = strlen(tmp);
		if (tmp[len - 1] == '/' || tmp[len - 1] == '\\') tmp[len - 1] = 0;

		for (p = tmp + 1; *p; p++) {
			if (*p == '/' || *p == '\\') {
				*p = 0;
				if (MKDIR(tmp) != 0 && errno != EEXIST) {
					perror("mkdir");
					return -1;
				}
				*p = '/';
			}
		}

		if (MKDIR(tmp) != 0 && errno != EEXIST) {
			perror("mkdir");
			return -1;
		}
		return RETZ;
}

/**
 * __regex_validate_char - Validate a single character against rules
 * @c: Character to validate
 * @forbidden: String of forbidden characters
 * @badch: Pointer to store invalid character found
 * @pos: Pointer to store position of invalid character
 * @i: Current position index
 *
 * Return: RETN if invalid, RETZ if valid
 */
static int __regex_validate_char(unsigned char c, const char *forbidden,
								 char *badch, size_t *pos, size_t i)
{
		/* Check for control characters */
		if (iscntrl(c)) {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return RETN;
		}

		/* Check for whitespace */
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return RETN;
		}

		/* Check against forbidden characters */
		if (strchr(forbidden, c)) {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return RETN;
		}

		return RETZ;
}

/**
 * __regex_v_unix - Validate string for Unix shell safety
 * @s: String to validate
 * @badch: Pointer to store invalid character found
 * @pos: Pointer to store position of invalid character
 *
 * Return: RETZ if valid, RETN if invalid character found
 */
static int __regex_v_unix(const char *s, char *badch, size_t *pos)
{
		const char *forbidden = "&;|$`\\\"'<>";
		size_t i = 0;
		unsigned char c;

		for (; *s; ++s, ++i) {
				c = (unsigned char)*s;

				/* Common character validation */
				if (__regex_validate_char(c, forbidden, badch, pos, i) == RETN)
						return RETN;

				/* Unix-specific additional checks */
				if (c == '(' || c == ')' || c == '{' || c == '}' ||
				    c == '[' || c == ']' || c == '*' || c == '?' || 
				    c == '~' || c == '#') {
						if (badch)
								*badch = c;
						if (pos)
								*pos = i;
						return RETN;
				}
		}

		return RETZ;
}

/**
 * __regex_v_win - Validate string for Windows shell safety
 * @s: String to validate
 * @badch: Pointer to store invalid character found
 * @pos: Pointer to store position of invalid character
 *
 * Return: RETZ if valid, RETN if invalid character found
 */
static int __regex_v_win(const char *s, char *badch, size_t *pos)
{
		const char *forbidden = "&|<>^\"'`\\";
		size_t i = 0;
		unsigned char c;

		for (; *s; ++s, ++i) {
				c = (unsigned char)*s;

				/* Common character validation */
				if (__regex_validate_char(c, forbidden, badch, pos, i) == RETN)
						return RETN;

				/* Windows-specific additional checks */
				if (c == '%' || c == '!' || c == ',' || c == ';' || 
				    c == '*' || c == '?' || c == '/') {
						if (badch)
								*badch = c;
						if (pos)
								*pos = i;
						return RETN;
				}
		}

		return RETZ;
}

static int __regex_check__(const char *cmd, char *badch, size_t *pos) {
#ifdef _WIN32
        return __regex_v_win(cmd, badch, pos);
#else
        return __regex_v_unix(cmd, badch, pos);
#endif
}

/**
 * wd_confirm_dangerous_command - Get user confirmation for suspicious command
 * @cmd: Command string
 * @badch: Dangerous character found
 * @pos: Position of dangerous character
 *
 * Return: 1 if user confirms, 0 if rejects
 */
static int wd_confirm_dangerous_command(const char *cmd, char badch, size_t pos)
{
		char *response;

		if (isprint((unsigned char)badch)) {
				printf_warning("Symbol detected in command - char='%c' (0x%02X) at pos=%zu; cmd=\"%s\"",
						       badch, (unsigned char)badch, pos, cmd);
		} else {
				printf_warning("Control symbol detected in command - char=0x%02X at pos=%zu; cmd=\"%s\"",
						       (unsigned char)badch, pos, cmd);
		}

		response = readline("Continue [y/n]: ");
		if (!response)
				return RETZ;

		if (strcmp(response, "Y") == 0 || strcmp(response, "y") == 0) {
				wdfree(response);
				return RETN;
		}

		wdfree(response);
		return RETZ;
}

/**
 * wd_run_command - Execute shell command with safety checks
 * @cmd: Command string to execute
 *
 * Validates command for dangerous characters and executes via system().
 * Includes debug mode with user confirmation for suspicious commands.
 *
 * Return: System call return value, or -RETN on error
 */
int wd_run_command(const char *cmd)
{
		char badch = 0;
		size_t pos = (size_t)-1;
		int ret;

		if (!cmd || !*cmd)
				return -RETN;

		/* Validate command for safety */
		if (__regex_check__(cmd, &badch, &pos)) {
#if defined(_DBG_PRINT)
				if (!wd_confirm_dangerous_command(cmd, badch, pos))
						return -RETN;
#endif
		}

		ret = system(cmd);
		return ret;
}

/**
 * print_file_to_terminal - Print file to Terminal
 * @path: path file}
 *
 * Return: void
 */
void print_file_to_terminal(const char *path) {
		int fd = open(path, O_RDONLY);
		if (fd < 0) {
			perror("open");
			return;
		}

		char buf[65536];
		ssize_t n;

		while ((n = read(fd, buf, sizeof buf)) > 0) {
			if (write(STDOUT_FILENO, buf, n) != n) {
				perror("write");
				close(fd);
				return;
			}
		}

		if (n < 0)
			perror("read");

		close(fd);
}

/**
 * wd_set_permission - Copy permissions from source to destination file
 * @src: Source file path to copy permissions from  
 * @dst: Destination file path to apply permissions to
 *
 * Return: RETZ on success, RETN on error
 */
int wd_set_permission(const char *src, const char *dst)
{
		struct stat st;

		if (!src || !dst)
				return RETN;

		/* Get source file permissions */
		if (stat(src, &st) != 0)
				return RETN;

#ifdef _WIN32
		/* Windows: set basic read/write permissions */
		if (_chmod(dst, _S_IREAD | _S_IWRITE) != 0)
				return RETN;
#else
		/* Unix: copy exact permission bits */
		if (chmod(dst, st.st_mode & 07777) != 0)
				return RETN;
#endif

		return RETZ;
}

/**
 * wd_set_title - Set terminal window title
 * @title: New title string (NULL for default)
 *
 * Sets the terminal window title using ANSI escape sequences.
 * If @title is NULL, uses default "Watchdogs" title.
 *
 * Return: 0 on success, -EINVAL on error
 */
int wd_set_title(const char *title)
{
		const char *new_title;
		
		if (!title)
				new_title = "Watchdogs";
		else
				new_title = title;

		/* Use ANSI escape sequence to set window title */
		printf("\033]0;%s\007", new_title);
		
		return RETZ;
}

/**
 * wd_strip_dot_fns - Remove file extension from filename
 * @dst: Destination buffer
 * @dst_sz: Destination buffer size
 * @src: Source filename
 *
 * Removes file extension if no path separators are present.
 * Preserves full path if path separators found.
 */
void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		/* Check for path separators */
		slash = strchr(src, '/');
#ifdef _WIN32
		if (!slash)
				slash = strchr(src, '\\');
#endif

		/* Only strip extension if no path separators */
		if (!slash) {
				dot = strchr(src, '.');
				if (dot) {
						len = (size_t)(dot - src);
						if (len >= dst_sz)
								len = dst_sz - 1;
						memcpy(dst, src, len);
						dst[len] = '\0';
						return;
				}
		}

		snprintf(dst, dst_sz, "%s", src);
}

/**
 * strfind - Case-insensitive substring search with word boundaries
 * @text: Text to search in
 * @pattern: Pattern to search for
 *
 * Return: true if pattern found with word boundaries, false otherwise
 */
bool strfind(const char *text, const char *pattern)
{
		size_t pat_len = strlen(pattern);
		size_t i, j;

		if (!text || !pattern)
				return false;

		for (i = 0; text[i]; i++) {
				j = 0;
				while (text[i + j] &&
				       tolower((unsigned char)text[i + j]) ==
				       tolower((unsigned char)pattern[j]))
						j++;

				if (j == pat_len) {
						bool left_ok = (i == 0 ||
										!isalnum((unsigned char)text[i - 1]));
						bool right_ok = !isalnum((unsigned char)text[i + j]);
						if (left_ok && right_ok)
								return true;
				}
		}

		return false;
}

/**
 * wd_escape_quotes - Escape double quotes in string
 * @dest: Destination buffer
 * @size: Destination buffer size
 * @src: Source string
 */
void wd_escape_quotes(char *dest, size_t size, const char *src)
{
		size_t i, j;

		if (!dest || size == 0 || !src)
				return;

		for (i = 0, j = 0; src[i] != '\0' && j + 1 < size; i++) {
				if (src[i] == '"') {
						if (j + 2 >= size)
								break;
						dest[j++] = '\\';
						dest[j++] = '"';
				} else {
						dest[j++] = src[i];
				}
		}
		dest[j] = '\0';
}

/**
 * __set_path_syms - Join directory and filename with proper separators
 * @out: Output buffer
 * @out_sz: Output buffer size
 * @dir: Directory path
 * @name: Filename
 */
static void __set_path_syms(char *out, size_t out_sz, const char *dir, const char *name)
{
		size_t dir_len;
		int dir_has_sep, name_has_leading_sep;

		if (!out || out_sz == 0)
				return;

		dir_len = strlen(dir);
		dir_has_sep = (dir_len > 0 && IS_PATH_SYM(dir[dir_len - 1]));
		name_has_leading_sep = IS_PATH_SYM(name[0]);

		if (dir_has_sep) {
				if (name_has_leading_sep)
						snprintf(out, out_sz, "%s%s", dir, name + 1);
				else
						snprintf(out, out_sz, "%s%s", dir, name);
		} else {
				if (name_has_leading_sep)
						snprintf(out, out_sz, "%s%s", dir, name);
				else
						snprintf(out, out_sz, "%s%s%s", dir, __PATH_SYM, name);
		}

		out[out_sz - 1] = '\0';
}

/**
 * __command_suggest - Calculate Levenshtein distance between two strings
 * @s1: First string
 * @s2: Second string
 *
 * Return: Edit distance, or INT_MAX if strings too long
 */
static int __command_suggest(const char *s1, const char *s2)
{
		int len1 = strlen(s1);
		int len2 = strlen(s2);
		int prev[129], curr[129];
		int i, j;

		if (len2 > 128)
				return INT_MAX;

		/* Initialize first row */
		for (j = 0; j <= len2; j++)
				prev[j] = j;

		/* Calculate distance matrix */
		for (i = 1; i <= len1; i++) {
				curr[0] = i;
				for (j = 1; j <= len2; j++) {
						int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
						int del = prev[j] + 1;
						int ins = curr[j-1] + 1;
						int sub = prev[j-1] + cost;

						curr[j] = min3(del, ins, sub);
				}
				memcpy(prev, curr, (len2 + 1) * sizeof(int));
		}

		return prev[len2];
}

/**
 * wd_find_near_command - Find closest matching command using edit distance
 * @command: Input command to match
 * @commands: Array of valid commands
 * @num_cmds: Number of commands in array
 * @out_distance: Optional output for distance value
 *
 * Return: Closest matching command, or NULL if none found
 */
const char *wd_find_near_command(const char *command, const char *commands[],
								 size_t num_cmds, int *out_distance)
{
		int best_distance = INT_MAX;
		const char *best_cmd = NULL;
		size_t i;

		for (i = 0; i < num_cmds; i++) {
				int dist = __command_suggest(command, commands[i]);
				if (dist < best_distance) {
						best_distance = dist;
						best_cmd = commands[i];
				}
		}

		if (out_distance)
				*out_distance = best_distance;

		return best_cmd;
}

/**
 * wd_detect_os - Detect operating system
 *
 * Return: String constant for detected OS
 */
const char *wd_detect_os(void)
{
		static char os[64] = "unknown";

#ifdef _WIN32
		strncpy(os, "windows", sizeof(os));
#else
		/* Check for WSL */
		if (getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME")) {
				strncpy(os, "windows", sizeof(os));
		} else {
				struct utsname sys_info;
				if (uname(&sys_info) == 0) {
						if (strstr(sys_info.sysname, "Linux"))
								strncpy(os, "linux", sizeof(os));
						else
								strncpy(os, sys_info.sysname, sizeof(os));
				}
		}
#endif

		os[sizeof(os)-1] = '\0';
		return os;
}

/**
 * dir_exists - Check if directory exists
 * @path: Directory path to check
 *
 * Return: RETN if exists, RETZ if not
 */
int dir_exists(const char *path)
{
		struct stat st;

		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
				return RETN;

		return RETZ;
}

/**
 * path_exists - Check if path exists
 * @path: Path to check
 *
 * Return: RETN if exists, RETZ if not
 */
int path_exists(const char *path)
{
		struct stat st;

		if (stat(path, &st) == 0)
				return RETN;

		return RETZ;
}

/**
 * dir_writable - Check if directory is writable
 * @path: Directory path to check
 *
 * Return: RETN if writable, RETZ if not
 */
int dir_writable(const char *path)
{
		if (access(path, W_OK) == 0)
				return RETN;

		return RETZ;
}

/**
 * path_acces - Check if path is accessible
 * @path: Path to check
 *
 * Return: RETN if accessible, RETZ if not
 */
int path_acces(const char *path)
{
		if (access(path, F_OK) == 0)
				return RETN;

		return RETZ;
}

/**
 * file_regular - Check if path is a regular file
 * @path: Path to check
 *
 * Return: RETN if regular file, RETZ if not
 */
int file_regular(const char *path)
{
		struct stat st;

		if (stat(path, &st) != 0)
				return RETZ;

		return S_ISREG(st.st_mode);
}

/**
 * file_same_file - Check if two paths point to same file
 * @a: First path
 * @b: Second path
 *
 * Return: RETN if same file, RETZ if not
 */
int file_same_file(const char *a, const char *b)
{
		struct stat sa, sb;

		if (stat(a, &sa) != 0)
				return RETZ;
		if (stat(b, &sb) != 0)
				return RETZ;

		return (sa.st_ino == sb.st_ino && sa.st_dev == sb.st_dev);
}

/**
 * ensure_parent_dir - Get parent directory of path
 * @out_parent: Output buffer for parent directory
 * @n: Output buffer size
 * @dest: Destination path
 *
 * Return: RETZ on success, -RETN on error
 */
int ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[PATH_MAX];
		char *parent;

		if (strlen(dest) >= sizeof(tmp))
				return -RETN;

		strncpy(tmp, dest, sizeof(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		parent = dirname(tmp);
		if (!parent)
				return -RETN;

		strncpy(out_parent, parent, n);
		out_parent[n-1] = '\0';

		return RETZ;
}

/**
 * kill_process - Kill process by name
 * @name: Process name
 *
 * Return: System command return value
 */
int kill_process(const char *name)
{
		char cmd[256];

		if (!name)
				return -RETN;

		snprintf(cmd, sizeof(cmd), "pkill -9 -f \"%s\" > /dev/null 2>&1", name);
		return system(cmd);
}

/**
 * kill_process_safe - Safely kill process by name
 * @name: Process name
 */
void kill_process_safe(const char *name)
{
		if (name && strlen(name) > 0)
				kill_process(name);
}

/**
 * wd_match_filename - Check if filename matches pattern
 * @name: Filename to check
 * @pattern: Pattern to match (may contain wildcards)
 *
 * Return: 1 if matches, 0 otherwise
 */
static int wd_match_filename(const char *name, const char *pattern)
{
		if (strchr(pattern, '*') || strchr(pattern, '?')) {
#ifdef _WIN32
				return PathMatchSpecA(name, pattern);
#else
				return (fnmatch(pattern, name, 0) == 0);
#endif
		} else {
				return (strcmp(name, pattern) == 0);
		}
}

/**
 * wd_is_special_dir - Check if directory name represents special entries
 * @name: Directory name to check
 * 
 * Identifies special directory entries like "." and ".." that should
 * typically be ignored during directory traversal.
 * 
 * Return: 1 if special directory, 0 otherwise
 */
static int wd_is_special_dir(const char *name);

/**
 * wd_should_ignore_dir - Determine if directory should be excluded from processing
 * @name: Directory name to check
 * @ignore_dir: Specific directory name to ignore (NULL for none)
 * 
 * Compares directory name against ignore pattern. Used to skip
 * specific directories during recursive file operations.
 * 
 * Return: 1 if directory should be ignored, 0 otherwise
 */
static int wd_should_ignore_dir(const char *name, const char *ignore_dir);

/**
 * wd_match_filename - Match filename against pattern with wildcard support
 * @name: Filename to check
 * @pattern: Pattern to match (supports * and ? wildcards)
 * 
 * Provides platform-specific pattern matching. On Windows uses
 * PathMatchSpecA, on Unix uses fnmatch().
 * 
 * Return: 1 if pattern matches, 0 otherwise
 */
static int wd_match_filename(const char *name, const char *pattern);

/**
 * wd_add_found_path - Add discovered path to configuration storage
 * @path: Full path to add to found paths list
 * 
 * Stores successfully found file paths in the global configuration
 * structure for later reference. Maintains count of found items.
 */
static void wd_add_found_path(const char *path);

/**
 * __toml_add_directory_path - Format and write directory path to TOML output
 * @toml_file: FILE handle for TOML configuration output
 * @first: Pointer to flag indicating first item in list
 * @path: Directory path to add to include_path array
 * 
 * Handles proper comma separation and formatting when adding
 * directory paths to TOML include_path arrays.
 */
static void __toml_add_directory_path(FILE *toml_file, int *first, const char *path);

/**
 * wd_find_compiler - Locate PAWN compiler executable based on environment
 * @os_type: Operating system type ("windows" or "linux")
 * 
 * Searches for pawncc compiler in platform-specific locations.
 * Consults configuration flags to determine search directory.
 * 
 * Return: 1 if compiler found, 0 if not found
 */
static int wd_find_compiler(const char *os_type);

/**
 * wd_generate_toml_content - Generate complete TOML configuration content
 * @file: FILE handle for output
 * @os_type: Target operating system
 * @find_gamemodes: Flag indicating if gamemodes were found
 * @find_pawncc: Flag indicating if compiler was found  
 * @find_plugins: Flag indicating if plugins should be included
 * @base_dir: Base directory path for file operations
 * 
 * Constructs complete watchdogs.toml configuration including
 * compiler options, include paths, and input/output file settings.
 */
static void wd_generate_toml_content(FILE *file, const char *os_type,
    int find_gamemodes, int find_pawncc, int find_plugins, char *base_dir);

/**
 * wd_add_include_paths - Populate TOML include_paths with discovered directories
 * @file: FILE handle for TOML output
 * @first_item: Pointer to flag tracking first list item for formatting
 * 
 * Recursively scans project directories and adds them to TOML
 * include_paths array. Handles gamemodes and compiler include directories.
 */
static void wd_add_include_paths(FILE *file, int *first_item);

/**
 * wd_add_compiler_path - Add compiler-specific include directory to TOML
 * @file: FILE handle for TOML output  
 * @path: Compiler include path to add
 * @first_item: Pointer to flag for comma formatting control
 * 
 * Adds pawno/include or qawno/include paths based on configuration
 * and recursively includes all subdirectories.
 */
static void wd_add_compiler_path(FILE *file, const char *path, int *first_item);

/**
 * wd_parse_toml_config - Parse and load TOML configuration file
 * 
 * Reads and parses watchdogs.toml configuration file using tomlc99 library.
 * Extracts general settings and OS configuration into global structure.
 * 
 * Return: 1 on successful parse, 0 on failure
 */
static int wd_parse_toml_config(void);

/**
 * wd_is_special_dir - Check if directory item is special (. or ..)
 * @name: Directory item name
 *
 * Return: 1 if special directory, 0 otherwise
 */
static int wd_is_special_dir(const char *name)
{
		return (name[0] == '.' && 
		       (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')));
}

/**
 * wd_should_ignore_dir - Check if directory should be ignored
 * @name: Directory name
 * @ignore_dir: Directory to ignore (NULL for none)
 *
 * Return: 1 if should ignore, 0 otherwise
 */
static int wd_should_ignore_dir(const char *name, const char *ignore_dir)
{
		if (!ignore_dir)
				return RETZ;

#ifdef _WIN32
		return (_stricmp(name, ignore_dir) == 0);
#else
		return (strcmp(name, ignore_dir) == 0);
#endif
}

/**
 * wd_add_found_path - Add found path to configuration
 * @path: Path to add
 */
static void wd_add_found_path(const char *path)
{
		if (wcfg.sef_count < (sizeof(wcfg.sef_found) / sizeof(wcfg.sef_found[0]))) {
				strncpy(wcfg.sef_found[wcfg.sef_count], path, MAX_SEF_PATH_SIZE);
				wcfg.sef_found[wcfg.sef_count][MAX_SEF_PATH_SIZE - 1] = '\0';
				wcfg.sef_count++;
		}
}

/**
 * wd_sef_fdir - Search for files recursively in directory
 * @sef_path: Directory path to search
 * @sef_name: File name or pattern to match
 * @ignore_dir: Directory name to ignore (optional)
 *
 * Recursively searches for files matching @sef_name pattern. Supports
 * wildcards (*, ?) and can ignore specific directories.
 *
 * Return: RETN if found, RETZ if not found
 */
int wd_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir)
{
		char path_buff[MAX_SEF_PATH_SIZE];

#ifdef _WIN32
		WIN32_FIND_DATA find_data;
		HANDLE find_handle;
		char search_path[MAX_PATH];
		const char *name;

		/* Build search path */
		if (sef_path[strlen(sef_path) - 1] == '\\')
				snprintf(search_path, sizeof(search_path), "%s*", sef_path);
		else
				snprintf(search_path, sizeof(search_path), "%s\\*", sef_path);

		find_handle = FindFirstFile(search_path, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return RETZ;

		do {
				name = find_data.cFileName;
				if (wd_is_special_dir(name))
						continue;

				__set_path_syms(path_buff, sizeof(path_buff), sef_path, name);

				if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (wd_should_ignore_dir(name, ignore_dir))
								continue;
						if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
								FindClose(find_handle);
								return RETN;
						}
				} else {
						if (wd_match_filename(name, sef_name)) {
								wd_add_found_path(path_buff);
								FindClose(find_handle);
								return RETN;
						}
				}
		} while (FindNextFile(find_handle, &find_data));

		FindClose(find_handle);
#else
		DIR *dir;
		struct dirent *item;
		struct stat statbuf;
		const char *name;

		dir = opendir(sef_path);
		if (!dir)
				return RETZ;

		while ((item = readdir(dir)) != NULL) {
				name = item->d_name;
				if (wd_is_special_dir(name))
						continue;

				__set_path_syms(path_buff, sizeof(path_buff), sef_path, name);

				if (item->d_type == DT_DIR) {
						if (wd_should_ignore_dir(name, ignore_dir))
								continue;
						if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
								closedir(dir);
								return RETN;
						}
				} else if (item->d_type == DT_REG) {
						if (wd_match_filename(name, sef_name)) {
								wd_add_found_path(path_buff);
								closedir(dir);
								return RETN;
						}
				} else {
						/* Handle file types that need stat */
						if (stat(path_buff, &statbuf) == -1)
								continue;

						if (S_ISDIR(statbuf.st_mode)) {
								if (wd_should_ignore_dir(name, ignore_dir))
										continue;
								if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
										closedir(dir);
										return RETN;
								}
						} else if (S_ISREG(statbuf.st_mode)) {
								if (wd_match_filename(name, sef_name)) {
										wd_add_found_path(path_buff);
										closedir(dir);
										return RETN;
								}
						}
				}
		}

		closedir(dir);
#endif

		return RETZ;
}

/**
 * __toml_add_directory_path - Add directory path to TOML with proper formatting
 * @toml_file: TOML file handle for output
 * @first: Pointer to first item flag
 * @path: Directory path to add
 */
static void __toml_add_directory_path(FILE *toml_file, int *first, const char *path)
{
		if (!*first)
				fprintf(toml_file, ",\n        ");
		else {
				fprintf(toml_file, "\n        ");
				*first = 0;
		}

		fprintf(toml_file, "\"%s\"", path);
}

/**
 * wd_check_compiler_options - Test compiler for specific options
 */
static void wd_check_compiler_options(int *compatibility, int *optimized_lt)
{
		char run_cmd[PATH_MAX + 258];
		FILE *proc_file;
		char log_line[1024];

		/* Run compiler with test command */
		snprintf(run_cmd, sizeof(run_cmd), 
				 "%s -___DDDDDDDDDDDDDDDDD-___DDDDDDDDDDDDDDDDD"
				 "-___DDDDDDDDDDDDDDDDD-___DDDDDDDDDDDDDDDDD > .__CP.log 2>&1", 
				 wcfg.sef_found[0]);
		
		system(run_cmd);

		int found_Z = 0, found_ver = 0;
		proc_file = fopen(".__CP.log", "r");
		if (proc_file) {
			while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
				if (!found_Z && strstr(log_line, "-Z"))
					found_Z = 1;
				if (!found_ver && strstr(log_line, "3.10.11"))
					found_ver = 1;
			}
			fclose(proc_file);
			if (found_Z)
				*compatibility = 1;
			if (found_ver)
				*optimized_lt = 1;
		} else {
			printf_error("Failed to open .__CP.log");
		}

		/* Cleanup temporary log file */
		if (path_acces(".__CP.log"))
				remove(".__CP.log");
}


/**
 * wd_parse_toml_config - Parse and load TOML configuration
 *
 * Return: 1 on success, 0 on failure
 */
static int wd_parse_toml_config(void)
{
		FILE *proc_file;
		char errbuf[256];
		toml_table_t *toml_config;
		toml_table_t *general_table;

		proc_file = fopen("watchdogs.toml", "r");
		if (!proc_file) {
				printf_error("Cannot read file %s", "watchdogs.toml");
				return RETZ;
		}

		toml_config = toml_parse_file(proc_file, errbuf, sizeof(errbuf));
		fclose(proc_file);

		if (!toml_config) {
				printf_error("Parsing TOML: %s", errbuf);
				return RETZ;
		}

		/* Extract OS configuration */
		general_table = toml_table_in(toml_config, "general");
		if (general_table) {
				toml_datum_t os_val = toml_string_in(general_table, "os");
				if (os_val.ok) {
						wcfg.os = strdup(os_val.u.s);
						wdfree(os_val.u.s);
				}
		}

		toml_free(toml_config);
		return RETN;
}

/**
 * wd_find_compiler - Locate pawn compiler based on OS and config
 * @os_type: Operating system type
 *
 * Return: 1 if found, 0 if not found
 */
static int wd_find_compiler(const char *os_type)
{
		int is_windows = (strcmp(os_type, "windows") == 0);
		const char *compiler_name = is_windows ? "pawncc.exe" : "pawncc";

		if (wcfg.f_samp == VAL_TRUE) {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		} else if (wcfg.f_openmp == VAL_TRUE) {
				return wd_sef_fdir("qawno", compiler_name, NULL);
		} else {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		}
}

/**
 * __toml_base_subdirs - Recursively add subdirectories to TOML include paths
 * @base_path: Base directory to scan
 * @toml_file: TOML file handle for output
 * @first: Pointer to first item flag (for comma formatting)
 *
 * Recursively traverses directories and adds them to TOML include_path array
 * in proper format with comma separation.
 */
static void __toml_base_subdirs(const char *base_path, FILE *toml_file, int *first)
{
#ifdef _WIN32
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;
		char search_path[MAX_PATH];
		char full_path[MAX_PATH + 50];

		snprintf(search_path, sizeof(search_path), "%s\\*", base_path);
		find_handle = FindFirstFileA(search_path, &find_data);

		if (find_handle == INVALID_HANDLE_VALUE)
				return;

		do {
				if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						if (strcmp(find_data.cFileName, ".") == 0 ||
						    strcmp(find_data.cFileName, "..") == 0)
								continue;

						snprintf(full_path, sizeof(full_path), "%s/%s", 
								 base_path, find_data.cFileName);

						__toml_add_directory_path(toml_file, first, full_path);
						__toml_base_subdirs(full_path, toml_file, first);
				}
		} while (FindNextFileA(find_handle, &find_data) != 0);

		FindClose(find_handle);
#else
		DIR *dir;
		struct dirent *item;
		char full_path[PATH_MAX + 50];

		dir = opendir(base_path);
		if (!dir)
				return;

		while ((item = readdir(dir)) != NULL) {
				if (item->d_type == DT_DIR) {
						if (strcmp(item->d_name, ".") == 0 ||
						    strcmp(item->d_name, "..") == 0)
								continue;

						snprintf(full_path, sizeof(full_path), "%s/%s", 
								 base_path, item->d_name);

						__toml_add_directory_path(toml_file, first, full_path);
						__toml_base_subdirs(full_path, toml_file, first);
				}
		}

		closedir(dir);
#endif
}

/**
 * wd_add_compiler_path - Add compiler-specific include path
 * @file: File handle to write to
 * @path: Path to add
 * @first_item: Pointer to first item flag
 */
static void wd_add_compiler_path(FILE *file, const char *path, int *first_item)
{
		if (path_acces(path)) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"%s\"", path);
				*first_item = 0;
				__toml_base_subdirs(path, file, first_item);
		}
}

/**
 * wd_add_include_paths - Add include paths to TOML configuration
 * @file: File handle to write to
 * @first_item: Pointer to first item flag
 */
static void wd_add_include_paths(FILE *file, int *first_item)
{
		if (path_acces("gamemodes")) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"gamemodes\"");
				*first_item = 0;
				__toml_base_subdirs("gamemodes", file, first_item);
		}

		/* Add compiler-specific include paths */
		if (wcfg.f_samp == VAL_TRUE) {
				wd_add_compiler_path(file, "pawno/include", first_item);
		} else if (wcfg.f_openmp == VAL_TRUE) {
				wd_add_compiler_path(file, "qawno/include", first_item);
		} else {
				wd_add_compiler_path(file, "pawno/include", first_item);
		}
}

/**
 * wd_generate_toml_content - Write TOML configuration to file
 * @file: File handle to write to
 * @os_type: Operating system type
 * @has_gamemodes: Whether gamemodes were found
 * @compatible: Compiler compatibility flag
 * @sef_path: Path for input/output files
 */
static void wd_generate_toml_content(FILE *file, const char *os_type, 
								    int has_gamemodes, int compatible,
								    int optimized_lt, char *sef_path)
{
		int first_item = 1;

		/* Build sef_path from found compiler */
		if (sef_path[0]) {
				char *ext = strrchr(sef_path, '.');
				if (ext)
						*ext = '\0';
		}

		fprintf(file, "[general]\n");
		fprintf(file, "\tos = \"%s\"\n", os_type);
		fprintf(file, "[compiler]\n");

		/* Compiler options */
		if (compatible && optimized_lt) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-O2\", \"-;+\", \"-(+\"]\n");
		} else if (compatible) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-;+\", \"-(+\"]\n");
		} else {
				fprintf(file, "\toption = [\"-d3\", \"-;+\", \"-(+\"]\n");
		}

		/* Include paths */
		fprintf(file, "\tinclude_path = [");
		wd_add_include_paths(file, &first_item);
		fprintf(file, "\n    ]\n");

		/* Input/output files */
		if (has_gamemodes && sef_path[0]) {
				fprintf(file, "\tinput = \"%s.pwn\"\n", sef_path);
				fprintf(file, "\toutput = \"%s.amx\"\n", sef_path);
		} else {
				fprintf(file, "\tinput = \"gamemodes/bare.pwn\"\n");
				fprintf(file, "\toutput = \"gamemodes/bare.amx\"\n");
		}
}

/**
 * wd_set_toml - Initialize and configure TOML settings
 *
 * Detects environment, finds compiler and gamemodes, generates
 * appropriate TOML configuration file.
 *
 * Return: RETZ on success, RETN on error
 */
int wd_set_toml(void)
{
		int find_pawncc = 0;
		int find_gamemodes = 0;
		int compatibility = 0;
		int optimized_lt = 0;
		const char *os_type;
		FILE *toml_file;

		os_type = wd_detect_os();

		/* Find compiler based on platform and configuration */
		find_pawncc = wd_find_compiler(os_type);
		if (!find_pawncc) {
				/* Fallback: search in current directory */
				if (strcmp(os_type, "windows") == 0)
						find_pawncc = wd_sef_fdir(".", "pawncc.exe", NULL);
				else
						find_pawncc = wd_sef_fdir(".", "pawncc", NULL);
		}

		/* Find gamemodes */
		find_gamemodes = wd_sef_fdir("gamemodes/", "*.pwn", NULL);

		/* Check compiler Options */
		if (find_pawncc) {
				wd_check_compiler_options(&compatibility, &optimized_lt);
		}

		/* Generate TOML configuration */
		toml_file = fopen("watchdogs.toml", "r");
		if (toml_file) {
				fclose(toml_file);
		} else {
				toml_file = fopen("watchdogs.toml", "w");
				if (!toml_file) {
						printf_error("Failed to create watchdogs.toml");
						return RETN;
				}

                if (find_pawncc)
				    wd_generate_toml_content(toml_file, os_type, find_gamemodes, 
										    compatibility, optimized_lt, wcfg.sef_found[1]);
                else
				    wd_generate_toml_content(toml_file, os_type, find_gamemodes, 
										    compatibility, optimized_lt, wcfg.sef_found[0]);
				fclose(toml_file);
		}

		/* Parse and load TOML configuration */
		if (!wd_parse_toml_config()) {
				printf_error("Failed to parse TOML configuration");
				return RETN;
		}

		return RETZ;
}

/**
 * _try_mv_without_sudo - Move file without sudo using copy-fallback
 * __mv_with_sudo - Move file with sudo
 * @src: Source file path
 * @dest: Destination file path
 */
 static int _try_mv_without_sudo(const char *src, const char *dest) {
		char clean_src[PATH_MAX];
		strncpy(clean_src, src, PATH_MAX);
		clean_src[PATH_MAX - 1] = '\0';

		char *p = clean_src;
		char *q = clean_src;
		while (*p) {
			if (*p != '/' && *p != '\\') {
				*q++ = *p;
			}
			p++;
		}
		*q = '\0';

		char __sz_mv[PATH_MAX * 2 + 100];
		snprintf(__sz_mv, sizeof(__sz_mv), "mv -i %s %s/%s", src, dest, clean_src);
		int ret = wd_run_command(__sz_mv);
		return ret;
}


static int __mv_with_sudo(const char *src, const char *dest) {
		char clean_src[PATH_MAX];
		strncpy(clean_src, src, PATH_MAX);
		clean_src[PATH_MAX - 1] = '\0';

		char *p = clean_src;
		char *q = clean_src;
		while (*p) {
			if (*p != '/' && *p != '\\') {
				*q++ = *p;
			}
			p++;
		}
		*q = '\0';

		char __sz_mv[PATH_MAX * 2 + 100];
		snprintf(__sz_mv, sizeof(__sz_mv), "sudo mv -i %s %s/%s", src, dest, clean_src);
		int ret = wd_run_command(__sz_mv);
		return ret;
}

/**
 * _try_cp_without_sudo - Copy file without sudo using temporary file
 * __cp_with_sudo - Copy file with sudo
 * @src: Source file path
 * @dest: Destination file path
 */
static int _try_cp_without_sudo(const char *src, const char *dest) {
		char clean_src[PATH_MAX];
		strncpy(clean_src, src, PATH_MAX);
		clean_src[PATH_MAX - 1] = '\0';

		char *p = clean_src;
		char *q = clean_src;
		while (*p) {
			if (*p != '/' && *p != '\\') {
				*q++ = *p;
			}
			p++;
		}
		*q = '\0';

		char __sz_cp[PATH_MAX * 2 + 100];
		snprintf(__sz_cp, sizeof(__sz_cp), "cp -i %s %s/%s", src, dest, clean_src);
		int ret = wd_run_command(__sz_cp);
		return ret;
}

static int __cp_with_sudo(const char *src, const char *dest) {
		char clean_src[PATH_MAX];
		strncpy(clean_src, src, PATH_MAX);
		clean_src[PATH_MAX - 1] = '\0';

		char *p = clean_src;
		char *q = clean_src;
		while (*p) {
			if (*p != '/' && *p != '\\') {
				*q++ = *p;
			}
			p++;
		}
		*q = '\0';

		char __sz_cp[PATH_MAX * 2 + 100];
		snprintf(__sz_cp, sizeof(__sz_cp), "sudo cp -i %s %s/%s", src, dest, clean_src);
		int ret = wd_run_command(__sz_cp);
		return ret;
}

/**
 * __wd_sef_safety - Validate source and destination for file operations
 * @c_src: Source file path
 * @c_dest: Destination file path
 *
 * Return: RETN if validation passed, RETZ if same file
 */
static int __wd_sef_safety(const char *c_src, const char *c_dest)
{
		char parent[PATH_MAX];
		struct stat st;

		if (!c_src || !c_dest)
				printf_error("src or dest is null");

		if (!*c_src || !*c_dest)
				printf_error("src or dest empty");

		if (strlen(c_src) >= PATH_MAX || strlen(c_dest) >= PATH_MAX)
				printf_error("path too long");

		if (!path_exists(c_src))
				printf_error("source does not exist: %s", c_src);

		if (!file_regular(c_src))
				printf_error("source is not a regular file: %s", c_src);

		if (path_exists(c_dest) && file_same_file(c_src, c_dest)) {
				printf_info("source and dest are the same file: %s", c_src);
		}

		if (ensure_parent_dir(parent, sizeof(parent), c_dest))
				printf_error("cannot determine parent dir of dest");

		if (stat(parent, &st))
				printf_error("destination dir does not exist: %s", parent);

		if (!S_ISDIR(st.st_mode))
				printf_error("destination parent is not a dir: %s", parent);

		return RETN;
}

/**
 * __wd_sef_set_permissions - Set permissions on destination file
 * @c_dest: Destination file path
 */
static void __wd_sef_set_permissions(const char *c_dest)
{
#ifdef _WIN32
		if (_w_chmod(c_dest)) {
# if defined(_DBG_PRINT)
				printf_warning("chmod failed: %s (errno=%d %s)",
						      c_dest, errno, strerror(errno));
# endif
		}
#else
		if (chmod(c_dest, 0755)) {
# if defined(_DBG_PRINT)
				printf_warning("chmod failed: %s (errno=%d %s)",
						      c_dest, errno, strerror(errno));
# endif
		}
#endif
}

/**
 * wd_sef_wmv - Move file with safety checks and sudo fallback
 * @c_src: Source file path
 * @c_dest: Destination file path
 *
 * Return: RETZ on success, RETN on failure
 */
 
int wd_sef_wmv(const char *c_src, const char *c_dest)
{
		int ret, mv_ret;

		ret = __wd_sef_safety(c_src, c_dest);
		if (ret != RETN)
				return RETN;

		int is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					printf_info("moved without sudo: %s -> %s", c_src, c_dest);
					return RETZ;
			}
		} else {
			mv_ret = __mv_with_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					printf_info("moved with sudo: %s -> %s", c_src, c_dest);
					return RETZ;
			}
		}

		return RETN;
}
/**
 * wd_sef_wcopy - Copy file with safety checks and sudo fallback
 * @c_src: Source file path
 * @c_dest: Destination file path
 *
 * Return: RETZ on success, RETN on failure
 */
int wd_sef_wcopy(const char *c_src, const char *c_dest)
{
		int ret, cp_ret;

		ret = __wd_sef_safety(c_src, c_dest);
		if (ret != RETN)
				return RETN;

		int is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					printf_info("copying without sudo: %s -> %s", c_src, c_dest);
					return RETZ;
			}
		} else {
			cp_ret = __cp_with_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					printf_info("copying with sudo: %s -> %s", c_src, c_dest);
					return RETZ;
			}
		}

		return RETN;
}
