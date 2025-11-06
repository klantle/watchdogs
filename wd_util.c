#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/types.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <ftw.h>
#include <curl/curl.h>
#include <inttypes.h>
#include <stddef.h>
#include <libgen.h>

#include <ncursesw/curses.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <errno.h>

#include "wd_extra.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_util.h"

const char* __command[]={
				"help",
				"clear",
				"exit",
				"kill",
				"title",
				"time",
				"stopwatch",
				"toml",
				"install",
				"upstream",
				"hardware",
				"gamemode",
				"pawncc",
				"compile",
				"running",
				"crunn",
				"stop",
				"restart"
			};

const size_t
		__command_len =
sizeof(__command) / sizeof(__command[0]);

/**
 * WatchdogConfig wcfg - Default configuration for watchdog tool.
 *
 * Members:
 *   wd_toml_os_type       - OS type string from TOML config (default NULL)
 *   wd_toml_binary        - SA-MP/Open.MP Binary File
 *   wd_toml_config        - SA-MP/Open.MP Config File
 *   wd_ipawncc           - Package index counter
 *   wd_idepends           - Dependency counter
 *   wd_os_type            - OS type flag (CRC32_FALSE by default)
 *   wd_sel_stat           - Selection Status - anti redundancy
 *   wd_is_samp            - Flag if running on SAMP (CRC32_FALSE)
 *   wd_is_omp             - Flag if running on OMP (CRC32_FALSE)
 *   wd_ptr_samp           - Pointer to SAMP module (NULL)
 *   wd_ptr_omp            - Pointer to OMP module (NULL)
 *   wd_stopwatch_end	   - Stopwatch End Loop
 *   wd_sef_count          - Count of SEF modules (0)
 *   wd_sef_found_list     - List of found SEF modules (initialized to zero)
 *   wd_toml_aio_opt       - AIO options from TOML (NULL)
 *   wd_toml_aio_repo      - AIO repository string from TOML (NULL)
 *   wd_toml_gm_input      - Game mode input path (NULL)
 *   wd_toml_gm_output     - Game mode output path (NULL)
 *
 */
WatchdogConfig wcfg = {
		.wd_toml_os_type = NULL,
		.wd_toml_binary = NULL,
		.wd_toml_config = NULL,
		.wd_ipawncc = 0,
		.wd_idepends = 0,
		.wd_os_type = CRC32_FALSE,
		.wd_sel_stat = 0,
		.wd_is_samp = CRC32_FALSE,
		.wd_is_omp = CRC32_FALSE,
		.wd_ptr_samp = NULL,
		.wd_ptr_omp = NULL,
		.wd_compiler_stat = 0,
		.wd_sef_count = 0,
		.wd_sef_found_list = { { 0 } },
		.wd_toml_aio_opt = NULL,
		.wd_toml_aio_repo = NULL,
		.wd_toml_gm_input = NULL,
		.wd_toml_gm_output = NULL
};

void wd_sef_fdir_reset(void) {
		size_t max_entries;
		max_entries = sizeof(wcfg.wd_sef_found_list) /
			      sizeof(wcfg.wd_sef_found_list[0]);

		size_t i;

		for (i = 0; i < max_entries; i++)
			wcfg.wd_sef_found_list[i][0] = '\0';

		memset(wcfg.wd_sef_found_list, 0, sizeof(wcfg.wd_sef_found_list));

		wcfg.wd_sef_count = 0;
}

char* wd_get_cwd(void) {
		static
		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) == NULL) {
			perror("getcwd failed");
			return NULL;
		}
		return cwd;
}

char* get_masked_text(int reveal, const char *text) {
		int len = strlen(text);
		if (reveal > len) reveal = len;

		char *masked = malloc(len + 1);
		if (!masked) return NULL;

		strncpy(masked, text, reveal);

		for (int i = reveal; i < len; i++) {
			masked[i] = '?';
		}

		masked[len] = '\0';
		return masked;
}

int mkdir_recusrs(const char *path) {
		char tmp[PATH_MAX];
		size_t __sz_tmp = sizeof(tmp);
		char *p = NULL;
		size_t len;

		snprintf(tmp, __sz_tmp, "%s", path);
		len = strlen(tmp);
		if (tmp[len - 1] == '/' ||
			tmp[len - 1] == '\\')
			tmp[len - 1] = 0;

		for (p = tmp + 1; *p; p++) {
			if (*p == '/' || *p == '\\') {
				*p = 0;
				if (MKDIR(tmp) != 0 && errno != EEXIST) {
					perror("mkdir");
					return -__RETN;
				}
				*p = '/';
			}
		}

		if (MKDIR(tmp) != 0 && errno != EEXIST) {
			perror("mkdir");
			return -__RETN;
		}
		return __RETZ;
}

static int __regex_validate_char(unsigned char c, const char *forbidden,
								 char *badch, size_t *pos, size_t i)
{
		if (iscntrl(c)) {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return __RETN;
		}

		if (c == ' ' ||
				c == '\t' ||
				c == '\n' ||
				c == '\r') {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return __RETN;
		}

		if (strchr(forbidden, c)) {
				if (badch)
						*badch = c;
				if (pos)
						*pos = i;
				return __RETN;
		}

		return __RETZ;
}

#ifndef _WIN32
static int __regex_v_unix(const char *s, char *badch, size_t *pos)
{
		const char *forbidden = "&;|$`\\\"'<>";
		size_t i = 0;
		unsigned char c;

		for (; *s; ++s, ++i) {
				c = (unsigned char)*s;

				if (__regex_validate_char(c, forbidden, badch, pos, i) == __RETN)
						return __RETN;

				if (c == '(' || c == ')' ||
					c == '{' || c == '}' ||
				    c == '[' || c == ']' ||
					c == '*' || c == '?' ||
				    c == '~' || c == '#') {
						if (badch)
								*badch = c;
						if (pos)
								*pos = i;
						return __RETN;
				}
		}

		return __RETZ;
}
#endif

static int __regex_v_win(const char *s, char *badch, size_t *pos)
{
		const char *forbidden = "&|<>^\"'`\\";
		size_t i = 0;
		unsigned char c;

		for (; *s; ++s, ++i) {
				c = (unsigned char)*s;

				if (__regex_validate_char(c, forbidden, badch, pos, i) == __RETN)
						return __RETN;

				if (c == '%' || c == '!' || c == ',' || c == ';' ||
				    c == '*' || c == '?' || c == '/') {
						if (badch)
								*badch = c;
						if (pos)
								*pos = i;
						return __RETN;
				}
		}

		return __RETZ;
}

static int __regex_check__(const char *cmd, char *badch, size_t *pos) {
#ifdef _WIN32
        return __regex_v_win(cmd, badch, pos);
#else
        return __regex_v_unix(cmd, badch, pos);
#endif
}

static int wd_confirm_dangerous_command(const char *cmd, char badch, size_t pos)
{
		if (isprint((unsigned char)badch)) {
				pr_warning(stdout,
							"Symbol detected in command - "
							"char='%c' (0x%02X) at pos=%zu; cmd=\"%s\"",
							badch, (unsigned char)badch, pos, cmd);
		} else {
				pr_warning(stdout,
							"Control symbol detected in command - "
							"char=0x%02X at pos=%zu; cmd=\"%s\"",
							(unsigned char)badch, pos, cmd);
		}

		char *confirm;
		confirm = readline("Continue [y/n]: ");
		if (!confirm)
				goto done;
		if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
				wd_free(confirm);
				return __RETN;
		}

done:
		wd_free(confirm);
		return __RETZ;
}

int wd_server_env(void)
{
		int ret = 0;
		if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE))
			ret = SAMP_TRUE;
		else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
			ret = OMP_TRUE;
		return ret;
}

int wd_run_command(const char *cmd)
{
		char badch = 0;
		size_t pos = (size_t)-1;
		int ret;

		if (!cmd || !*cmd)
				return -__RETN;

		if (__regex_check__(cmd, &badch, &pos)) {
#if defined(_DBG_PRINT)
				if (!wd_confirm_dangerous_command(cmd, badch, pos))
						return -__RETN;
#endif
		}

		ret = system(cmd);
		return ret;
}

int is_termux_environment(void)
{
		struct stat st;
		int is_termux = __RETZ;
#if defined(__ANDROID__)
		is_termux = __RETN;
#endif
		if (stat("/data/data/com.termux/files/usr/local/lib/", &st) == 0 ||
			stat("/data/data/com.termux/files/usr/lib/", &st) == 0 ||
			stat("/data/data/com.termux/arm64/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/arm32/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/amd32/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/amd64/usr/lib", &st) == 0)
		{
			is_termux = __RETN;
		}
		return is_termux;
}

int is_native_windows(void)
{
		char* msys2_env = getenv("MSYSTEM");
		int is_win32 = __RETZ;
		int is_native = __RETZ;
#ifdef _WIN32
		is_win32 = __RETN;
#endif
		if (msys2_env == NULL && is_win32 == __RETN) {
			is_native = __RETN;
		}
		return is_native;
}

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

int wd_set_permission(const char *src, const char *dst)
{
		struct stat st;

		if (!src || !dst)
				return __RETN;

		if (stat(src, &st) != 0)
				return __RETN;

#ifdef _WIN32
		if (_chmod(dst, _S_IREAD | _S_IWRITE) != 0)
				return __RETN;
#else
		if (chmod(dst, st.st_mode & 07777) != 0)
				return __RETN;
#endif

		return __RETZ;
}

int wd_set_title(const char *title)
{
		const char *new_title;

		if (!title)
				new_title = "Watchdogs";
		else
				new_title = title;

		printf("\033]0;%s\007", new_title);

		return __RETZ;
}

void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		slash = strchr(src, '/');
#ifdef _WIN32
		if (!slash)
				slash = strchr(src, '\\');
#endif

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

bool strfind(const char *text, const char *pattern)
{
		size_t n = strlen(text);
		size_t m = strlen(pattern);
		if (m == 0 || n < m) return false;

		unsigned char skip[256];
		for (size_t i = 0; i < 256; i++)
			skip[i] = (unsigned char)m;
		for (size_t i = 0; i < m - 1; i++)
			skip[(unsigned char)tolower((unsigned char)pattern[i])] = (unsigned char)(m - 1 - i);

		size_t i = 0;
		while (i <= n - m) {
			size_t j = m;
			while (j > 0 && tolower((unsigned char)pattern[j - 1]) == tolower((unsigned char)text[i + j - 1]))
				j--;
			if (j == 0)
				return true;
			i += skip[(unsigned char)tolower((unsigned char)text[i + m - 1])];
		}
		return false;
}

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

static int __command_suggest(const char *s1, const char *s2)
{
		int len1 = strlen(s1);
		int len2 = strlen(s2);
		int prev[129], curr[129];
		int i, j;

		if (len2 > 128)
				return INT_MAX;

		for (j = 0; j <= len2; j++)
				prev[j] = j;

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

const char *wd_detect_os(void)
{
    	static char os[64] = "unknown";

#ifdef _WIN32
    	strncpy(os, "windows", sizeof(os));
#else
		if (access("/.dockerenv", F_OK) == 0) {
			strncpy(os, "linux", sizeof(os));
		}
		else if ((getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME")) &&
				system("which docker > /dev/null 2>&1") == 0) {
			if (getenv("WD_USE_DOCKER") || access("Dockerfile", F_OK) == 0) {
				strncpy(os, "linux", sizeof(os));
			} else {
				strncpy(os, "windows", sizeof(os));
			}
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

int dir_exists(const char *path)
{
		struct stat st;

		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
				return __RETN;

		return __RETZ;
}

int path_exists(const char *path)
{
		struct stat st;

		if (stat(path, &st) == 0)
				return __RETN;

		return __RETZ;
}

int dir_writable(const char *path)
{
		if (access(path, W_OK) == 0)
				return __RETN;

		return __RETZ;
}

int path_acces(const char *path)
{
		if (access(path, F_OK) == 0)
				return __RETN;

		return __RETZ;
}

int file_regular(const char *path)
{
		struct stat st;

		if (stat(path, &st) != 0)
				return __RETZ;

		return S_ISREG(st.st_mode);
}

int file_same_file(const char *a, const char *b)
{
		struct stat sa, sb;

		if (stat(a, &sa) != 0)
				return __RETZ;
		if (stat(b, &sb) != 0)
				return __RETZ;

		return (sa.st_ino == sb.st_ino && sa.st_dev == sb.st_dev);
}

int ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[PATH_MAX];
		char *parent;

		if (strlen(dest) >= sizeof(tmp))
				return -__RETN;

		strncpy(tmp, dest, sizeof(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		parent = dirname(tmp);
		if (!parent)
				return -__RETN;

		strncpy(out_parent, parent, n);
		out_parent[n-1] = '\0';

		return __RETZ;
}

int kill_process(const char *name)
{
		if (name && strlen(name) > 0) {
			char cmd[256];

			if (!name)
					return -__RETN;

#ifndef _WIN32
			snprintf(cmd, sizeof(cmd), "pkill -SIGTERM \"%s\" > /dev/null 2>&1", name);
#else
			snprintf(cmd, sizeof(cmd), "C:\\Windows\\System32\\taskkill.exe "
									   "/IM \"%s\" >nul 2>&1", name);
#endif
			return system(cmd);
		} else {
			return __RETZ;
		}
}

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

static int wd_is_special_dir(const char *name)
{
		return (name[0] == '.' &&
		       (name[1] == '\0' ||
					 (name[1] == '.' && name[2] == '\0')));
}

static int wd_should_ignore_dir(const char *name,
								const char *ignore_dir)
{
		if (!ignore_dir)
				return __RETZ;

#ifdef _WIN32
		return (_stricmp(name, ignore_dir) == 0);
#else
		return (strcmp(name, ignore_dir) == 0);
#endif
}

static void wd_add_found_path(const char *path)
{
		if (wcfg.wd_sef_count < (sizeof(wcfg.wd_sef_found_list) /
								 sizeof(wcfg.wd_sef_found_list[0]))) {
				strncpy(wcfg.wd_sef_found_list[wcfg.wd_sef_count], path, MAX_SEF_PATH_SIZE);
				wcfg.wd_sef_found_list[wcfg.wd_sef_count][MAX_SEF_PATH_SIZE - 1] = '\0';
				++wcfg.wd_sef_count;
		}
}

int wd_sef_fdir(const char *sef_path,
				const char *sef_name,
				const char *ignore_dir)
{
		char path_buff[MAX_SEF_PATH_SIZE];

#ifdef _WIN32
		WIN32_FIND_DATA find_data;
		HANDLE find_handle;
		char search_path[MAX_PATH];
		const char *name;

		if (sef_path[strlen(sef_path) - 1] == '\\')
				snprintf(search_path,
						 sizeof(search_path), "%s*", sef_path);
		else
				snprintf(search_path,
						 sizeof(search_path), "%s\\*", sef_path);

		find_handle = FindFirstFile(search_path, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return __RETZ;

		do {
				name = find_data.cFileName;
				if (wd_is_special_dir(name))
						continue;

				__set_path_syms(path_buff, sizeof(path_buff), sef_path, name);

				if (find_data.dwFileAttributes &
						FILE_ATTRIBUTE_DIRECTORY) {
						if (wd_should_ignore_dir(name, ignore_dir))
								continue;
						if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
								FindClose(find_handle);
								return __RETN;
						}
				} else {
						if (wd_match_filename(name, sef_name)) {
								wd_add_found_path(path_buff);
								FindClose(find_handle);
								return __RETN;
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
				return __RETZ;

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
								return __RETN;
						}
				} else if (item->d_type == DT_REG) {
						if (wd_match_filename(name, sef_name)) {
								wd_add_found_path(path_buff);
								closedir(dir);
								return __RETN;
						}
				} else {
						if (stat(path_buff, &statbuf) == -1)
								continue;

						if (S_ISDIR(statbuf.st_mode)) {
								if (wd_should_ignore_dir(name, ignore_dir))
										continue;
								if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
										closedir(dir);
										return __RETN;
								}
						} else if (S_ISREG(statbuf.st_mode)) {
								if (wd_match_filename(name, sef_name)) {
										wd_add_found_path(path_buff);
										closedir(dir);
										return __RETN;
								}
						}
				}
		}

		closedir(dir);
#endif

		return __RETZ;
}

static void
__toml_add_directory_path(FILE *toml_file, int *first, const char *path)
{
		if (!*first)
				fprintf(toml_file, ",\n        ");
		else {
				fprintf(toml_file, "\n        ");
				*first = 0;
		}

		fprintf(toml_file, "\"%s\"", path);
}

int rate_compiler_check = 0;
static void wd_check_compiler_options(int *compatibility, int *optimized_lt)
{
		if (rate_compiler_check == 0) {
			rate_compiler_check = 1;
			char run_cmd[PATH_MAX + 258];
			FILE *proc_file;
			char log_line[1024];

			snprintf(run_cmd, sizeof(run_cmd),
					"%s -___DDDDDDDDDDDDDDDDD "
					"-___DDDDDDDDDDDDDDDDD"
					"-___DDDDDDDDDDDDDDDDD-"
					"___DDDDDDDDDDDDDDDDD > .__CP.log 2>&1",
						wcfg.wd_sef_found_list[0]);
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
				if (found_Z)
					*compatibility = 1;
				if (found_ver)
					*optimized_lt = 1;
				fclose(proc_file);
			} else {
				pr_error(stdout, "Failed to open .__CP.log");
			}

			if (path_acces(".__CP.log"))
					remove(".__CP.log");
		}
}

static int wd_parse_toml_config(void)
{
		FILE *proc_file;
		char errbuf[256];
		toml_table_t *_toml_config;
		toml_table_t *general_table;

		proc_file = fopen("watchdogs.toml", "r");
		if (!proc_file) {
				pr_error(stdout, "Cannot read file %s", "watchdogs.toml");
				return __RETZ;
		}

		_toml_config = toml_parse_file(proc_file, errbuf, sizeof(errbuf));
		fclose(proc_file);

		if (!_toml_config) {
				pr_error(stdout, "Parsing TOML: %s", errbuf);
				return __RETZ;
		}

		general_table = toml_table_in(_toml_config, "general");
		if (general_table) {
				toml_datum_t os_val = toml_string_in(general_table, "os");
				if (os_val.ok) {
						wcfg.wd_toml_os_type = strdup(os_val.u.s);
						wd_free(os_val.u.s);
				}
		}

		toml_free(_toml_config);
		return __RETN;
}

static int wd_find_compiler(const char *wd_os_type)
{
		int is_windows = (strcmp(wd_os_type, "windows") == 0);
		const char *compiler_name = is_windows ? "pawncc.exe" : "pawncc";

		if (wd_server_env() == SAMP_TRUE) {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		} else if (wd_server_env() == OMP_TRUE) {
				return wd_sef_fdir("qawno", compiler_name, NULL);
		} else {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		}
}

static void __toml_base_subdirs(const char *base_path, FILE *toml_file, int *first)
{
#ifdef _WIN32
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;
		char search_path[MAX_PATH];
		char full_path[MAX_PATH * 4];

		snprintf(search_path, sizeof(search_path), "%s\\*", base_path);
		find_handle = FindFirstFileA(search_path, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
			return;

		do {
			if (find_data.dwFileAttributes &
					FILE_ATTRIBUTE_DIRECTORY) {
				if (strcmp(find_data.cFileName, ".") == 0 ||
					strcmp(find_data.cFileName, "..") == 0)
					continue;

				const char *last_slash = strrchr(base_path, '\\');
				if (last_slash && strcmp(last_slash + 1, find_data.cFileName) == 0)
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
		char full_path[MAX_PATH * 4];

		dir = opendir(base_path);
		if (!dir)
			return;

		while ((item = readdir(dir)) != NULL) {
			if (item->d_type == DT_DIR) {
				if (strcmp(item->d_name, ".") == 0 ||
					strcmp(item->d_name, "..") == 0)
					continue;

				const char *last_slash = strrchr(base_path, '/');
				if (last_slash && strcmp(last_slash + 1, item->d_name) == 0)
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

static void wd_add_compiler_path(FILE *file, const char *path, int *first_item)
{
		if (path_acces(path)) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"%s\"", path);
				//*first_item = 0;
				//__toml_base_subdirs(path, file, first_item);
		}
}

static void wd_add_include_paths(FILE *file, int *first_item)
{
		if (path_acces("gamemodes")) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"gamemodes/\"");
				*first_item = 0;
				//__toml_base_subdirs("gamemodes", file, first_item);
		}

		if (wd_server_env() == SAMP_TRUE)
				wd_add_compiler_path(file, "pawno/include/", first_item);
		else if (wd_server_env() == OMP_TRUE)
				wd_add_compiler_path(file, "qawno/include/", first_item);
		else
				wd_add_compiler_path(file, "pawno/include/", first_item);
}

static int _is_samp_ = 0;
static void wd_generate_toml_content(FILE *file, const char *wd_os_type,
								    int has_gamemodes, int compatible,
								    int optimized_lt, char *sef_path)
{
		int first_item = 1;

		if (sef_path[0]) {
				char *__dot_ext = strrchr(sef_path, '.');
				if (__dot_ext)
						*__dot_ext = '\0';
		}

		char *ptr_samp = NULL;
		char *ptr_omp = NULL;

		if (!strcmp(wd_os_type, "windows")) {
			ptr_samp = "samp-server.exe";
			ptr_omp = "omp-server.exe";
		} else if (!strcmp(wd_os_type, "linux")) {
			ptr_samp = "samp03svr";
			ptr_omp = "omp-server";
		}

		FILE *file_s = fopen(ptr_samp, "r");
		FILE *file_m = fopen(ptr_omp, "r");

		if (file_s != NULL && file_m != NULL) {
			_is_samp_ = 0;
		} else if (file_s != NULL) {
			_is_samp_ = 1;
		} else if (file_m != NULL) {
			_is_samp_ = 0;
		} else {
			_is_samp_ = 1;
		}

		if (file_s) fclose(file_s);
		if (file_m) fclose(file_m);

		fprintf(file, "[general]\n");
		fprintf(file, "\tos = \"%s\"\n", wd_os_type);
		if (_is_samp_ == 1) {
			fprintf(file, "\tbinary = \"%s\"\n", ptr_samp);
			fprintf(file, "\tconfig = \"%s\"\n", "server.cfg");
		} else {
			fprintf(file, "\tbinary = \"%s\"\n", ptr_omp);
			fprintf(file, "\tconfig = \"%s\"\n", "config.json");
		}

		fprintf(file, "[compiler]\n");

		if (compatible && optimized_lt) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-O2\", \"-;+\", \"-(+\"]\n");
		} else if (compatible) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-;+\", \"-(+\"]\n");
		} else {
				fprintf(file, "\toption = [\"-d3\", \"-;+\", \"-(+\"]\n");
		}

		fprintf(file, "\tinclude_path = [");
		wd_add_include_paths(file, &first_item);
		fprintf(file, "\n\t]\n");

		if (has_gamemodes && sef_path[0]) {
				fprintf(file, "\tinput = \"%s.pwn\"\n", sef_path);
				fprintf(file, "\toutput = \"%s.amx\"\n", sef_path);
		} else {
				fprintf(file, "\tinput = \"gamemodes/bare.pwn\"\n");
				fprintf(file, "\toutput = \"gamemodes/bare.amx\"\n");
		}

		fprintf(file, "[depends]\n");
		fprintf(file, "\tgithub_tokens = \"DO_HERE\"\n");
		fprintf(file, "\taio_repo = [\"Y-Less/sscanf:latest\", "
					  "\"samp-incognito/samp-streamer-plugin:latest\"]");
}

static int notice_not_found = 0;
int wd_set_toml(void)
{
		int find_pawncc = 0;
		int find_gamemodes = 0;
		int compatibility = 0;
		int optimized_lt = 0;
		const char *wd_os_type;
		FILE *toml_file;

		wd_os_type = wd_detect_os();

		find_pawncc = wd_find_compiler(wd_os_type);
		if (!find_pawncc) {
				if (strcmp(wd_os_type, "windows") == 0)
						find_pawncc = wd_sef_fdir(".", "pawncc.exe", NULL);
				else
						find_pawncc = wd_sef_fdir(".", "pawncc", NULL);
		}

		find_gamemodes = wd_sef_fdir("gamemodes/", "*.pwn", NULL);

		if (find_pawncc) {
				wd_check_compiler_options(&compatibility, &optimized_lt);
		}

		toml_file = fopen("watchdogs.toml", "r");
		if (toml_file) {
				fclose(toml_file);
		} else {
				toml_file = fopen("watchdogs.toml", "w");
				if (!toml_file) {
						pr_error(stdout, "Failed to create watchdogs.toml");
						return __RETN;
				}

				if (find_pawncc)
					wd_generate_toml_content(toml_file, wd_os_type, find_gamemodes,
						compatibility, optimized_lt, wcfg.wd_sef_found_list[1]);
				else
					wd_generate_toml_content(toml_file, wd_os_type, find_gamemodes,
						compatibility, optimized_lt, wcfg.wd_sef_found_list[0]);
				fclose(toml_file);
		}

		if (!wd_parse_toml_config()) {
				pr_error(stdout, "Failed to parse TOML configuration");
				return __RETN;
		}

		char errbuf[256];
		toml_table_t *_toml_config;
		FILE *procc_f = fopen("watchdogs.toml", "r");
		_toml_config = toml_parse_file(procc_f, errbuf, sizeof(errbuf));
		if (procc_f) fclose(procc_f);

		if (!_toml_config) {
			pr_error(stdout, "parsing TOML: %s", errbuf);
			wd_main(NULL);
		}

		toml_table_t *wd_toml_depends = toml_table_in(_toml_config, "depends");
		if (wd_toml_depends) {
				toml_datum_t toml_gh_tokens = toml_string_in(wd_toml_depends, "github_tokens");
				if (toml_gh_tokens.ok)
				{
					wcfg.wd_toml_github_tokens = strdup(toml_gh_tokens.u.s);
					wd_free(toml_gh_tokens.u.s);
					toml_gh_tokens.u.s = NULL;
				}
		}

		toml_table_t *general_table = toml_table_in(_toml_config, "general");
		if (general_table) {
				toml_datum_t bin_val = toml_string_in(general_table, "binary");
				if (bin_val.ok) {
						if (_is_samp_ == 1)
							wcfg.wd_ptr_samp = strdup(bin_val.u.s);
						else if (_is_samp_ == 0)
							wcfg.wd_ptr_omp = strdup(bin_val.u.s);
						else
							wcfg.wd_ptr_samp = strdup(bin_val.u.s);
						wd_free(bin_val.u.s);
				}
				toml_datum_t conf_val = toml_string_in(general_table, "config");
				if (conf_val.ok) {
						wcfg.wd_toml_config = strdup(conf_val.u.s);
						wd_free(conf_val.u.s);
				}
		}

		toml_free(_toml_config);

		if (strcmp(wcfg.wd_toml_os_type, "windows") == 0) {
			wcfg.wd_os_type = OS_SIGNAL_WINDOWS;
		} else if (strcmp(wcfg.wd_toml_os_type, "linux") == 0) {
			wcfg.wd_os_type = OS_SIGNAL_LINUX;
		}

		FILE *file_s = fopen(wcfg.wd_ptr_samp, "r");
		FILE *file_m = fopen(wcfg.wd_ptr_omp, "r");

		if (file_s != NULL && file_m != NULL) {
			wcfg.wd_is_samp = CRC32_TRUE;
			wcfg.wd_is_omp  = CRC32_FALSE;
		} else if (file_s != NULL) {
			wcfg.wd_is_samp = CRC32_TRUE;
			wcfg.wd_is_omp  = CRC32_FALSE;
		} else if (file_m != NULL) {
			wcfg.wd_is_samp = CRC32_FALSE;
			wcfg.wd_is_omp  = CRC32_TRUE;
		} else {
			wcfg.wd_is_samp = CRC32_FALSE;
			wcfg.wd_is_omp  = CRC32_FALSE;
			if (notice_not_found == 0) {
				notice_not_found = 1;
				pr_crit(stdout, "samp-server/open.mp server not found!");
			}
		}

		return __RETZ;
}

static int _try_mv_without_sudo(const char *src, const char *dest) {
		char __sz_mv[PATH_MAX];
		if (is_native_windows())
			snprintf(__sz_mv, sizeof(__sz_mv), "move /-Y %s %s", src, dest);
		else
			snprintf(__sz_mv, sizeof(__sz_mv), "mv -i %s %s", src, dest);
		int ret = wd_run_command(__sz_mv);
		return ret;
}

static int __mv_with_sudo(const char *src, const char *dest) {
		char __sz_mv[PATH_MAX];
		if (is_native_windows())
			snprintf(__sz_mv, sizeof(__sz_mv), "move /-Y %s %s", src, dest);
		else
			snprintf(__sz_mv, sizeof(__sz_mv), "sudo mv -i %s %s", src, dest);
		int ret = wd_run_command(__sz_mv);
		return ret;
}

static int _try_cp_without_sudo(const char *src, const char *dest) {
		char __sz_cp[PATH_MAX];
		if (is_native_windows())
			snprintf(__sz_cp, sizeof(__sz_cp), "xcopy /-Y %s %s", src, dest);
		else
			snprintf(__sz_cp, sizeof(__sz_cp), "cp -i %s %s", src, dest);
		int ret = wd_run_command(__sz_cp);
		return ret;
}

static int __cp_with_sudo(const char *src, const char *dest) {
		char __sz_cp[PATH_MAX];
		if (is_native_windows())
			snprintf(__sz_cp, sizeof(__sz_cp), "xcopy /-Y %s %s", src, dest);
		else
			snprintf(__sz_cp, sizeof(__sz_cp), "sudo cp -i %s %s", src, dest);
		int ret = wd_run_command(__sz_cp);
		return ret;
}

static int __wd_sef_safety(const char *c_src, const char *c_dest)
{
		char parent[PATH_MAX];
		struct stat st;

		if (!c_src || !c_dest)
				pr_error(stdout, "src or dest is null");
		if (!*c_src || !*c_dest)
				pr_error(stdout, "src or dest empty");
		if (strlen(c_src) >= PATH_MAX || strlen(c_dest) >= PATH_MAX)
				pr_error(stdout, "path too long");
		if (!path_exists(c_src))
				pr_error(stdout, "source does not exist: %s", c_src);
		if (!file_regular(c_src))
				pr_error(stdout, "source is not a regular file: %s", c_src);
		if (path_exists(c_dest) && file_same_file(c_src, c_dest))
				pr_info(stdout, "source and dest are the same file: %s", c_src);
		if (ensure_parent_dir(parent, sizeof(parent), c_dest))
				pr_error(stdout, "cannot determine parent dir of dest");
		if (stat(parent, &st))
				pr_error(stdout, "destination dir does not exist: %s", parent);
		if (!S_ISDIR(st.st_mode))
				pr_error(stdout, "destination parent is not a dir: %s", parent);

		return __RETN;
}

static void __wd_sef_set_permissions(const char *c_dest)
{
#ifdef _WIN32
		if (win32_chmod(c_dest)) {
# if defined(_DBG_PRINT)
				pr_warning(stdout, "chmod failed: %s (errno=%d %s)",
						      c_dest, errno, strerror(errno));
# endif
		}
#else
		if (chmod(c_dest, 0755)) {
# if defined(_DBG_PRINT)
				pr_warning(stdout, "chmod failed: %s (errno=%d %s)",
						      c_dest, errno, strerror(errno));
# endif
		}
#endif
}

int wd_sef_wmv(const char *c_src, const char *c_dest)
{
		int ret, mv_ret;

		ret = __wd_sef_safety(c_src, c_dest);
		if (ret != __RETN)
				return __RETN;

		int is_not_sudo = 0;
		if (is_native_windows())
			is_not_sudo = 1;
		else
			is_not_sudo = system("which sudo > /dev/null 2>&1");

		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "moved without sudo: %s -> %s", c_src, c_dest);
					return __RETZ;
			}
		} else {
			mv_ret = __mv_with_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "moved with sudo: %s -> %s", c_src, c_dest);
					return __RETZ;
			}
		}

		return __RETN;
}

int wd_sef_wcopy(const char *c_src, const char *c_dest)
{
		int ret, cp_ret;

		ret = __wd_sef_safety(c_src, c_dest);
		if (ret != __RETN)
				return __RETN;

		int is_not_sudo = 0;
		if (is_native_windows())
			is_not_sudo = 1;
		else
			is_not_sudo = system("which sudo > /dev/null 2>&1");

		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "copying without sudo: %s -> %s", c_src, c_dest);
					return __RETZ;
			}
		} else {
			cp_ret = __cp_with_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "copying with sudo: %s -> %s", c_src, c_dest);
					return __RETZ;
			}
		}

		return __RETN;
}

typedef struct {
        char *cs_t;
        char *cs_i;
} causeExplanation;

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
{"empty statement", "Pawn does not support a standalone semicolon as an empty statement. Use an empty block `{}` instead if intentional."},

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
print_file_with_annotations(const char *pawn_output)
{
        FILE *pf;
        char buffer[MAX_PATH];
        int wcnt = 0;
        int ecnt = 0;

        pf = fopen(pawn_output, "r");
        if (!pf) {
                printf("Cannot open file: %s\n", pawn_output);
                return;
        }

        while (fgets(buffer, sizeof(buffer), pf)) {
                const char *description = NULL;
                const char *t_pos = NULL;
                int mk_pos = 0;
                int i;

                printf("%s", buffer);

                if (strstr(buffer, "warning"))
                        wcnt++;
                if (strstr(buffer, "error"))
                        ecnt++;

                description = find_warning_error(buffer);
                if (description) {
                        for (i = 0;
                            ccs[i].cs_t; i++)
                        {
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

        char __sz_compiler[256];
        snprintf(__sz_compiler, 256, "COMPILE COMPLETE | "
									 "WITH %d ERROR | "
									 "%d WARNING", ecnt, wcnt);
        wd_set_title(__sz_compiler);

        fclose(pf);
}
