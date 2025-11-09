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
#ifdef __linux__
#include <termios.h>
#endif

#include <ncursesw/curses.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <errno.h>

#include "wd_extra.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_crypto.h"
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
		size_t i, sef_max_entries;
		sef_max_entries = sizeof(wcfg.wd_sef_found_list) /
			      	  	  sizeof(wcfg.wd_sef_found_list[0]);

		for (i = 0; i < sef_max_entries; i++)
			wcfg.wd_sef_found_list[i][0] = '\0';

		wcfg.wd_sef_count = 0;
		memset(wcfg.wd_sef_found_list, 0, sizeof(wcfg.wd_sef_found_list));
}

static char cwd_cache[WD_PATH_MAX] = { 0 };

static void __wd_init_cwd(void) {
#ifdef _WIN32
	    DWORD len;
	    len = GetCurrentDirectoryA(sizeof(cwd_cache), cwd_cache);
	    if (len == 0 ||
	    	len >= sizeof(cwd_cache))
	        cwd_cache[0] = '\0';
#else
	    ssize_t len;
	    len = readlink("/proc/self/cwd",
	    		cwd_cache,
	    		sizeof(cwd_cache) - 1);
	    if (len < 0)
	        cwd_cache[0] = '\0';
	    else
	        cwd_cache[len] = '\0';
#endif
}

const char *wd_get_cwd(void) {
	    if (cwd_cache[0] == '\0') {
	    	__wd_init_cwd();
	    }
	    return cwd_cache;
}

size_t wd_strcnpy(char *dest, const char *src, size_t size) {
	    if (size == 0)
	    	return 0;

	    size_t len = strlen(src);
	    if (len >= size)
	        len = size - 1;

	    memcpy(dest, src, len);
	    dest[len] = '\0';

	    return len;
}

size_t wd_strcpy(char *dest, const char *src, size_t size) {
	    if (size == 0)
	    	return 0;

	    size_t len = strlen(src);
	    if (len >= size)
	        len = size - 1;

	    memcpy(dest, src, len);
	    dest[len] = '\0';
	    return len;
}

int wd_snprintf(char *buf, size_t size, const char *fmt, ...) {
	    va_list args;
	    va_start(args, fmt);
	    int n;
	    n = vsnprintf(buf, size, fmt, args);
	    va_end(args);

	    if (n < 0 || (size_t)n >= size) {
	        if (size > 0)
	            buf[size - 1] = '\0';
	        return -1;
	    }
	    return n;
}

char* wd_masked_text(int reveal, const char *text) {
	    if (!text)
	    	return NULL;

	    int len = (int)strlen(text);
	    if (reveal < 0)
	    	reveal = 0;
	    if (reveal > len)
	    	reveal = len;

	    char *masked;
	    masked = wd_malloc((size_t)len + 1);
	    if (!masked)
	    	return NULL;

	    if (reveal > 0)
	        memcpy(masked, text, (size_t)reveal);

	    for (int i = reveal; i < len; ++i)
	        masked[i] = '?';

	    masked[len] = '\0';
	    return masked;
}

int mkdir_recusrs(const char *path) {
		char tmp[WD_PATH_MAX + 56];
		size_t size_tmp = sizeof(tmp);
		char *p = NULL;
		size_t len;

		wd_snprintf(tmp, size_tmp, "%s", path);
		len = strlen(tmp);
		if (tmp[len - 1] == __PATH_CHR_SYM_LINUX ||
			tmp[len - 1] == __PATH_CHR_SYM_WIN32)
			tmp[len - 1] = 0;

		for (p = tmp + 1; *p; p++) {
			if (*p == __PATH_CHR_SYM_LINUX ||
				*p == __PATH_CHR_SYM_WIN32)
			{
				*p = 0;
				if (MKDIR(tmp) != 0 && errno != EEXIST) {
					perror("mkdir");
					return -__RETN;
				}
				*p = __PATH_CHR_SYM_LINUX;
			}
		}

		if (MKDIR(tmp) != 0 && errno != EEXIST) {
			perror("mkdir");
			return -__RETN;
		}
		return __RETZ;
}

int wd_server_env(void)
{
		int ret = __RETZ;
		if (ret == __RETZ) {
			if (!strcmp(wcfg.wd_is_samp, CRC32_TRUE))
				ret = __RETN;
			else if (!strcmp(wcfg.wd_is_omp, CRC32_TRUE))
				ret = __RETW;
		}
		return ret;
}

int wd_run_command(const char *cmd)
{
		if (cmd == NULL) {
			return -__RETN;
		}
		static char size_command[WD_MAX_PATH]; /* 4096 */
		int s_ret = wd_snprintf(size_command,
						        sizeof(size_command),
						        "%s",
						        cmd);
		if (!s_ret)
			return -__RETN;
		int ret = -__RETN;
		if (ret == -__RETN)
			system(size_command);
		return ret;
}

int is_termux_environment(void)
{
		struct stat st;
		int is_termux = __RETZ;
#if defined(__ANDROID__)
		is_termux = __RETN;
		return is_termux;
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
#if defined __linux__ || __ANDROID__
		return __RETZ;
#endif
		char* msys2_env;
		msys2_env = getenv("MSYSTEM");
		if (msys2_env)
			return __RETZ;
		else
			return __RETN;
}

void wd_printfile(const char *path) {
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

int wd_set_title(const char *title)
{
		const char *new_title;
		if (!title)
				new_title = watchdogs_release;
		else
				new_title = title;
#ifdef _WIN32
		SetConsoleTitleA(new_title);
#else
	    if (isatty(STDOUT_FILENO))
	        	printf("\033]0;%s\007", new_title);
#endif
		return __RETZ;
}

void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		slash = strchr(src, __PATH_CHR_SYM_LINUX);
#ifdef _WIN32
		if (!slash)
				slash = strchr(src, __PATH_CHR_SYM_WIN32);
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

		wd_snprintf(dst, dst_sz, "%s", src);
}


bool strfind(const char *text, const char *pattern) {
	    const size_t n = strlen(text);
	    const size_t m = strlen(pattern);
	    if (m == 0 || n < m) return false;

	    unsigned char pat[256];
	    for (size_t i = 0; i < m; i++)
	        pat[i] = (unsigned char) \
	    			tolower((unsigned char)pattern[i]);

	    uint8_t skip[256];
	    memset(skip, m, sizeof(skip));
	    for (size_t i = 0; i < m - 1; i++)
	        skip[pat[i]] = (uint8_t)(m - 1 - i);

	    const unsigned char *txt;
	    txt = (const unsigned char *)text;
	    size_t i = 0;

	    while (i <= n - m) {
	        unsigned char last = tolower(txt[i + m - 1]);
	        if (pat[m - 1] == last) {
	            size_t j = m - 1;
	            while (j > 0 &&
	            	   pat[j - 1] == tolower(txt[i + j - 1]))
	                j--;
	            if (j == 0)
	                return true;
	        }
	        i += skip[last];
	    }
	    return false;
}

void wd_escape_quotes(char *dest, size_t size, const char *src)
{
		size_t i, j;

		if (!dest ||
			size == 0 ||
			!src)
			return;

		for (i = 0,
			j = 0;
			src[i] != '\0' &&
			j + 1 < size;
			i++) {
				if (src[i] == '"') {
						if (j + 2 >= size)
								break;
						dest[j++] = __PATH_CHR_SYM_WIN32;
						dest[j++] = '"';
				} else
						dest[j++] = src[i];
		}
		dest[j] = '\0';
}

static void __set_path_syms(char *out, size_t out_sz, const char *dir, const char *name)
{
		size_t dir_len;
		int dir_has_sep, has_led_sep;

		if (!out || out_sz == 0)
				return;

		dir_len = strlen(dir);
		dir_has_sep = (dir_len > 0 &&
					   IS_PATH_SYM(dir[dir_len - 1]));
		has_led_sep = IS_PATH_SYM(name[0]);

		if (dir_has_sep) {
				if (has_led_sep) snprintf(out, out_sz, "%s%s", dir, name + 1);
				else snprintf(out, out_sz, "%s%s", dir, name);
		} else {
				if (has_led_sep) snprintf(out, out_sz, "%s%s", dir, name);
				else snprintf(out, out_sz, "%s%s%s", dir, __PATH_SYM, name);
		}

		out[out_sz - 1] = '\0';
}

static inline int min3(int a, int b, int c) {
    	return (a < b) ? (a < c ? a : c) : (b < c ? b : c);
}

static int __command_suggest(const char *s1, const char *s2) {
	    int len1 = strlen(s1);
	    int len2 = strlen(s2);
	    if (len2 > 128) return INT_MAX;

	    uint16_t *buf1 = alloca((len2 + 1) * sizeof(uint16_t));
	    uint16_t *buf2 = alloca((len2 + 1) * sizeof(uint16_t));
	    uint16_t *prev = buf1;
	    uint16_t *curr = buf2;

	    for (int j = 0; j <= len2; j++) prev[j] = j;

	    for (int i = 1; i <= len1; i++) {
	        curr[0] = i;
	        char c1 = tolower((unsigned char)s1[i - 1]);
	        int min_row = INT_MAX;

	        for (int j = 1; j <= len2; j++) {
	            char c2 = tolower((unsigned char)s2[j - 1]);
	            int cost = (c1 == c2) ? 0 : 1;
	            int del = prev[j] + 1;
	            int ins = curr[j - 1] + 1;
	            int sub = prev[j - 1] + cost;
	            int val = min3(del, ins, sub);
	            curr[j] = val;
	            if (val < min_row) min_row = val;
	        }

	        if (min_row > 6)
	            return min_row + (len1 - i);

	        uint16_t *tmp = prev;
	        prev = curr;
	        curr = tmp;
	    }

	    return prev[len2];
}

const char *wd_find_near_command(const char *command,
                                 const char *commands[],
                                 size_t num_cmds,
                                 int *out_distance)
{
	    int best_distance = INT_MAX;
	    const char *best_cmd = NULL;

	    for (size_t i = 0; i < num_cmds; i++) {
	        int dist = __command_suggest(command, commands[i]);
	        if (dist < best_distance) {
	            best_distance = dist;
	            best_cmd = commands[i];
	            if (best_distance == 0)
	            	break;
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
				wd_run_command("which docker > /dev/null 2>&1") == 0) {
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

		if (stat(path, &st) == 0 &&
			S_ISDIR(st.st_mode))
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

		return (sa.st_ino == sb.st_ino &&
				sa.st_dev == sb.st_dev);
}

int
ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[WD_PATH_MAX];
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
		if (!name)
			return -__RETN;
		char cmd[PATH_MAX];
#ifndef _WIN32
		snprintf(cmd, sizeof(cmd), "pkill -SIGTERM \"%s\" > /dev/null 2>&1", name);
#else
		snprintf(cmd, sizeof(cmd), "C:\\Windows\\System32\\taskkill.exe "
									   "/IM \"%s\" >nul 2>&1", name);
#endif
		return wd_run_command(cmd);
}

static int
wd_match_filename(const char *name, const char *pattern)
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

int wd_is_special_dir(const char *name)
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
				strncpy(
							wcfg.wd_sef_found_list[wcfg.wd_sef_count],
							path,
							MAX_SEF_PATH_SIZE
						);
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
		char sp[WD_MAX_PATH * 2];
		const char *name;
		if (sef_path[strlen(sef_path) - 1] == __PATH_CHR_SYM_WIN32)
				wd_snprintf(sp,
						    sizeof(sp), "%s*", sef_path);
		else
				wd_snprintf(sp,
						    sizeof(sp), "%s\\*", sef_path);
		find_handle = FindFirstFile(sp, &find_data);
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
			char run_cmd[WD_PATH_MAX + 258];
			FILE *proc_file;
			char log_line[1024];

			snprintf(run_cmd, sizeof(run_cmd),
					"%s -___DDDDDDDDDDDDDDDDD "
					"-___DDDDDDDDDDDDDDDDD"
					"-___DDDDDDDDDDDDDDDDD-"
					"___DDDDDDDDDDDDDDDDD > .__CP.log 2>&1",
						wcfg.wd_sef_found_list[0]);
			wd_run_command(run_cmd);

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
		char error_buffer[256];
		toml_table_t *_toml_config;
		toml_table_t *general_table;

		proc_file = fopen("watchdogs.toml", "r");
		if (!proc_file) {
				pr_error(stdout, "Cannot read file %s", "watchdogs.toml");
				return __RETZ;
		}

		_toml_config = toml_parse_file(proc_file, error_buffer, sizeof(error_buffer));
		fclose(proc_file);

		if (!_toml_config) {
				pr_error(stdout, "Parsing TOML: %s", error_buffer);
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

		if (wd_server_env() == 1) {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		} else if (wd_server_env() == 2) {
				return wd_sef_fdir("qawno", compiler_name, NULL);
		} else {
				return wd_sef_fdir("pawno", compiler_name, NULL);
		}
}

static void __attribute__((unused)) __toml_base_subdirs(const char *base_path,
														FILE *toml_file, int *first)
{
#ifdef _WIN32
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;
		char sp[WD_MAX_PATH], fp[WD_MAX_PATH * 2];
		wd_snprintf(sp, sizeof(sp), "%s\\*", base_path);

		find_handle = FindFirstFileA(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return;
		do {
				if (find_data.dwFileAttributes &
						FILE_ATTRIBUTE_DIRECTORY) {
					if (wd_is_special_dir(find_data.cFileName))
						continue;

					const char *last_slash = strrchr(base_path, __PATH_CHR_SYM_WIN32);
					if (last_slash &&
						strcmp(last_slash + 1,
							   find_data.cFileName) == 0)
						continue;

					snprintf(fp, sizeof(fp), "%s/%s",
							base_path, find_data.cFileName);

					__toml_add_directory_path(toml_file, first, fp);
					__toml_base_subdirs(fp, toml_file, first);
				}
		} while (FindNextFileA(find_handle, &find_data) != 0);

		FindClose(find_handle);
#else
		DIR *dir;
		struct dirent *item;
		char fp[WD_MAX_PATH * 4];

		dir = opendir(base_path);
		if (!dir)
			return;

		while ((item = readdir(dir)) != NULL) {
				if (item->d_type == DT_DIR) {
					if (wd_is_special_dir(item->d_name))
						continue;

					const char *last_slash = strrchr(base_path, __PATH_CHR_SYM_LINUX);
					if (last_slash &&
						strcmp(last_slash + 1,
							   item->d_name) == 0)
						continue;

					snprintf(fp, sizeof(fp), "%s/%s",
							base_path, item->d_name);

					__toml_add_directory_path(toml_file, first, fp);
					__toml_base_subdirs(fp, toml_file, first);
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

		if (wd_server_env() == 1)
				wd_add_compiler_path(file, "pawno/include/", first_item);
		else if (wd_server_env() == 2)
				wd_add_compiler_path(file, "qawno/include/", first_item);
		else
				wd_add_compiler_path(file, "pawno/include/", first_item);
}

static int _is_samp_ = -1;
static void wd_generate_toml_content(FILE *file, const char *wd_os_type,
								    int has_gamemodes, int compatible,
								    int optimized_lt, char *sef_path)
{
		int first_item = 1;
		if (sef_path[0]) {
			char *f_EXT = strrchr(sef_path, '.');
			if (f_EXT)
					*f_EXT = '\0';
		}

		fprintf(file, "[general]\n");
		fprintf(file, "\tos = \"%s\"\n", wd_os_type);
		if (_is_samp_ == 0) {
			if (!strcmp(wd_os_type, "windows")) {
				fprintf(file, "\tbinary = \"%s\"\n", "omp-server.exe");
				fprintf(file, "\tconfig = \"%s\"\n", "config.json");
			} else if (!strcmp(wd_os_type, "linux")) {
				fprintf(file, "\tbinary = \"%s\"\n", "omp-server");
				fprintf(file, "\tconfig = \"%s\"\n", "config.json");
			}
		} else {
			if (!strcmp(wd_os_type, "windows")) {
				fprintf(file, "\tbinary = \"%s\"\n", "samp-server.exe");
				fprintf(file, "\tconfig = \"%s\"\n", "server.cfg");
			} else if (!strcmp(wd_os_type, "linux")) {
				fprintf(file, "\tbinary = \"%s\"\n", "samp-server.exe");
				fprintf(file, "\tconfig = \"%s\"\n", "server.cfg");
			}
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

int wd_set_toml(void)
{
		int find_pawncc = 0;
		int find_gamemodes = 0;
		int compatibility = 0;
		int optimized_lt = 0;

		const char *wd_os_type;
		FILE *toml_file;

		wd_os_type = wd_detect_os();
		
		if (dir_exists("qawno") &&
			dir_exists("components"))
			_is_samp_ = 0;
		else if (dir_exists("pawno") &&
			path_exists("server.cfg"))
			_is_samp_ = 1;
		else {
			static int crit_nf = 0;
			if (crit_nf == 0) {
				crit_nf = 1;
				pr_crit(stdout, "can't locate sa-mp/open.mp server!");	
				return __RETZ;
			}
		}

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

		char error_buffer[256];
		toml_table_t *_toml_config;
		FILE *procc_f = fopen("watchdogs.toml", "r");
		_toml_config = toml_parse_file(procc_f, error_buffer, sizeof(error_buffer));
		if (procc_f) fclose(procc_f);

		if (!_toml_config) {
			pr_error(stdout, "parsing TOML: %s", error_buffer);
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
						if (_is_samp_ == 1) {
							wcfg.wd_is_samp = CRC32_TRUE;
							wcfg.wd_ptr_samp = strdup(bin_val.u.s);
						}
						else if (_is_samp_ == 0) {
							wcfg.wd_is_omp = CRC32_TRUE;
							wcfg.wd_ptr_omp = strdup(bin_val.u.s);
						}
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

		if (!path_exists(wcfg.wd_toml_config)) {
			static int crit_nf = 0;
			if (crit_nf == 0) {
				crit_nf = 1;
				pr_warning(stdout, "can't locate %s!", wcfg.wd_toml_config);
			}
		}

		return __RETZ;
}

static int _try_mv_without_sudo(const char *src, const char *dest) {
		char size_mv[WD_PATH_MAX];
		if (is_native_windows())
			snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
		else
			snprintf(size_mv, sizeof(size_mv), "mv -i %s %s", src, dest);
		int ret = wd_run_command(size_mv);
		return ret;
}

static int __mv_with_sudo(const char *src, const char *dest) {
		char size_mv[WD_PATH_MAX];
		if (is_native_windows())
			snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
		else
			snprintf(size_mv, sizeof(size_mv), "sudo mv -i %s %s", src, dest);
		int ret = wd_run_command(size_mv);
		return ret;
}

static int _try_cp_without_sudo(const char *src, const char *dest) {
		char size_cp[WD_PATH_MAX];
		if (is_native_windows())
			snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
		else
			snprintf(size_cp, sizeof(size_cp), "cp -i %s %s", src, dest);
		int ret = wd_run_command(size_cp);
		return ret;
}

static int __cp_with_sudo(const char *src, const char *dest) {
		char size_cp[WD_PATH_MAX];
		if (is_native_windows())
			snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
		else
			snprintf(size_cp, sizeof(size_cp), "sudo cp -i %s %s", src, dest);
		int ret = wd_run_command(size_cp);
		return ret;
}

static int __wd_sef_safety(const char *c_src, const char *c_dest)
{
		char parent[WD_PATH_MAX];
		struct stat st;

		if (!c_src || !c_dest)
				pr_error(stdout, "src or dest is null");
		if (!*c_src || !*c_dest)
				pr_error(stdout, "src or dest empty");
		if (strlen(c_src) >= WD_PATH_MAX || strlen(c_dest) >= WD_PATH_MAX)
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
#ifdef _WIN32
		is_not_sudo = 1;
#else
		is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
#endif
		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved without sudo: '%s' -> '%s'", c_src, c_dest);
					return __RETZ;
			}
		} else {
			mv_ret = __mv_with_sudo(c_src, c_dest);
			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved with sudo: '%s' -> '%s'", c_src, c_dest);
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
#ifdef _WIN32
		is_not_sudo = 1;
#else
		is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
#endif
		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying without sudo: '%s' -> '%s'", c_src, c_dest);
					return __RETZ;
			}
		} else {
			cp_ret = __cp_with_sudo(c_src, c_dest);
			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying with sudo: '%s' -> '%s'", c_src, c_dest);
					return __RETZ;
			}
		}

		return __RETN;
}
