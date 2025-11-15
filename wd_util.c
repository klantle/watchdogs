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
#include "wd_util.h"
#ifdef WD_LINUX
#include <spawn.h>
#include <termios.h>
#endif

#ifndef _NCURSES
#include <ncursesw/curses.h>
#else
#include <ncurses.h>
#endif
#include <readline/readline.h>
#include <readline/history.h>

#include <errno.h>

#include "wd_extra.h"
#include "wd_unit.h"
#include "wd_package.h"
#include "wd_crypto.h"

const char* __command[]={
				"help", "exit",
				"kill", "title",
				"sha256", "crc32",
				"djb2", "time",
				"config", "stopwatch",
				"install", "hardware",
				"gamemode", "pawncc",
				"compile",
				"running", "compiles",
				"stop", "restart",
				"ls", "ping",
				"clear", "nslookup",
				"netstat", "ipconfig",
				"uname", "hostname",
				"whoami", "arp",
				"route", "df",
				"du", "ps"
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
		.wd_sef_count = RATE_SEF_EMPTY,
		.wd_sef_found_list = { { RATE_SEF_EMPTY } },
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

		wcfg.wd_sef_count = RATE_SEF_EMPTY;
		memset(wcfg.wd_sef_found_list, RATE_SEF_EMPTY, sizeof(wcfg.wd_sef_found_list));
}

static char wd_work_dir[WD_PATH_MAX];
static void __wd_init_cwd(void) {
#ifdef WD_WINDOWS
	    DWORD size_len;
	    size_len = GetCurrentDirectoryA(sizeof(wd_work_dir), wd_work_dir);
	    if (size_len == 0 ||
	    	size_len >= sizeof(wd_work_dir))
	        wd_work_dir[0] = '\0';
#else
	    ssize_t size_len;
	    size_len = readlink("/proc/self/cwd",
	    		wd_work_dir,
	    		sizeof(wd_work_dir) - 1);
	    if (size_len < 0)
	        wd_work_dir[0] = '\0';
	    else
	        wd_work_dir[size_len] = '\0';
#endif
}

const char *wd_get_cwd(void) {
	if (wd_work_dir[0] == '\0')
		__wd_init_cwd();
	return wd_work_dir;
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

int wd_mkdir(const char *path) {
	    char temp[512];
	    char *p = NULL;
	    size_t len;

	    snprintf(temp, sizeof(temp), "%s", path);
	    len = strlen(temp);

	    if (len == 0) {
	        return -WD_RETN;
	    }

	    for (p = temp + 1; *p; p++) {
	        if (*p == '/') {
	            *p = '\0';
	            if (mkdir(temp, S_IRWXU) == -1) {
	                if (errno != EEXIST) {
	                    return -WD_RETN;
	                }
	            }
	            *p = '/';
	        }
	    }

	    if (mkdir(temp, S_IRWXU) == -1) {
	        if (errno != EEXIST) {
	            return -WD_RETN;
	        }
	    }

	    return WD_RETZ;
}

int wd_run_command(const char *cmd)
{
		char size_command[WD_MAX_PATH];
		wd_snprintf(size_command,
				sizeof(size_command), "%s", cmd);
		if (cmd[0] == '\0')
			return WD_RETZ;
		return system(size_command);
}

int wd_run_command_depends(const char *cmd)
{
		/* fastest command run */
		/* do not use for complex command|args */
	    if (cmd == NULL) {
	        return -WD_RETN;
	    }
#ifdef WD_WINDOWS
	    PROCESS_INFORMATION pi;
	    STARTUPINFO si;
	    DWORD exit_code;
	    char *cmd_copy = NULL;
	    int ret = -WD_RETN;

	    ZeroMemory(&si, sizeof(si));
	    si.cb = sizeof(si);
	    ZeroMemory(&pi, sizeof(pi));

	    cmd_copy = _strdup(cmd);
	    if (cmd_copy == NULL) {
	        return -WD_RETN;
	    }

	    BOOL rate_windows_proc_success = CreateProcess(
	        NULL,           // No module name (use command line)
	        cmd_copy,       // Command line
	        NULL,           // Process handle not inheritable
	        NULL,           // Thread handle not inheritable
	        FALSE,          // Set handle inheritance to FALSE
	        0,              // No creation flags
	        NULL,           // Use parent's environment block
	        NULL,           // Use parent's starting directory
	        &si,            // Pointer to STARTUPINFO structure
	        &pi             // Pointer to PROCESS_INFORMATION structure
	    );

	    if (!rate_windows_proc_success) {
	        wd_free(cmd_copy);
	        return -WD_RETN;
	    }

	    wd_free(cmd_copy);

	    WaitForSingleObject(pi.hProcess, INFINITE);

	    if (GetExitCodeProcess(pi.hProcess, &exit_code)) {
	        ret = (int)exit_code;
	    }

	    CloseHandle(pi.hProcess);
	    CloseHandle(pi.hThread);

	    return ret;
#elif defined(WD_LINUX)
	    pid_t pid;
	    int rate_posix_proc_status;
	    char *argv[] = { "sh", "-c", NULL, NULL };
	    posix_spawn_file_actions_t posix_f_actions;
	    extern char **environ;

	    argv[2] = (char *)cmd;

	    if (posix_spawn_file_actions_init(&posix_f_actions) != 0) {
	        return -WD_RETN;
	    }

	    int spawn_ret = -WD_RETN;
	    spawn_ret = posix_spawnp(&pid, "sh", &posix_f_actions, NULL, argv, environ);
	    
	    posix_spawn_file_actions_destroy(&posix_f_actions);

	    if (spawn_ret != WD_RETZ) {
	        return -spawn_ret;
	    }
	    if (waitpid(pid, &rate_posix_proc_status, 0) == -1) {
	        return -WD_RETN;
	    }

	    if (WIFEXITED(rate_posix_proc_status)) {
	        return WEXITSTATUS(rate_posix_proc_status);
	    } else if (WIFSIGNALED(rate_posix_proc_status)) {
	        return 128 + WTERMSIG(rate_posix_proc_status);
	    } else {
	        return -WD_RETN;
	    }
#endif
    	return -WD_RETN;
}

int is_termux_environment(void)
{
		struct stat st;
		int is_termux = WD_RETZ;
#if defined(WD_LINUX)
		is_termux = WD_RETN;
		return is_termux;
#endif
		if (stat("/data/data/com.termux/files/usr/local/lib/", &st) == 0 ||
			stat("/data/data/com.termux/files/usr/lib/", &st) == 0 ||
			stat("/data/data/com.termux/arm64/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/arm32/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/amd32/usr/lib", &st) == 0 ||
			stat("/data/data/com.termux/amd64/usr/lib", &st) == 0)
		{
			is_termux = WD_RETN;
		}
		return is_termux;
}

int is_native_windows(void)
{
#if defined WD_LINUX || WD_ANDROID
		return WD_RETZ;
#endif
		char* msys2_env;
		msys2_env = getenv("MSYSTEM");
		if (msys2_env)
			return WD_RETZ;
		else
			return WD_RETN;
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
#ifdef WD_WINDOWS
		SetConsoleTitleA(new_title);
#else
	    if (isatty(STDOUT_FILENO))
	        	printf("\033]0;%s\007", new_title);
#endif
		return WD_RETZ;
}

void wd_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		slash = strchr(src, __PATH_CHR_SEP_LINUX);
#ifdef WD_WINDOWS
		if (!slash)
				slash = strchr(src, __PATH_CHR_SEP_WIN32);
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

bool
wd_strcase(const char *text, const char *pattern) {
		const char *p;
		for (p = text; *p; p++) {
		    const char *a = p, *b = pattern;
		    while (*a && *b && (((*a | 32) == (*b | 32)))) {
		          a++;
		          b++;
	      	}
		    if (!*b) return true;
		}
		return false;
}

unsigned char wd_tolower(unsigned char c) {
    	return (unsigned char)(c + ((c - 'A') <= ('Z' - 'A') ? 32 : 0));
}
bool strfind(const char *text, const char *pattern) {
	    if (!text || !pattern)
	        return false;

	    size_t n = 0;
	    while (text[n]) n++;
	    size_t m = 0;
	    while (pattern[m]) m++;

	    if (m == 0) return true;
	    if (n < m) return false;

	    unsigned char pat[256];
	    uint8_t skip[256];

	    for (size_t i = 0; i < m; i++)
	        pat[i] = wd_tolower((unsigned char)pattern[i]);

	    for (size_t i = 0; i < 256; i++)
	        skip[i] = (uint8_t)m;
	    
	    for (size_t i = 0; i < m - 1; i++)
	        skip[pat[i]] = (uint8_t)(m - 1 - i);

	    const unsigned char *txt = (const unsigned char *)text;
	    size_t i = 0;

	    while (i <= n - m) {
	        unsigned char current_char = wd_tolower(txt[i + m - 1]);
	        
	        if (current_char == pat[m - 1]) {
	            int j = (int)m - 2;
	            while (j >= 0 && pat[j] == wd_tolower(txt[i + j])) {
	                j--;
	            }
	            if (j < 0) {
	                return true;
	            }
	        }
	        
	        i += skip[current_char];
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
						dest[j++] = __PATH_CHR_SEP_WIN32;
						dest[j++] = '"';
				} else
						dest[j++] = src[i];
		}
		dest[j] = '\0';
}

static void __set_path_sep(char *out, size_t out_sz,
						   const char *dir, const char *entry_name)
{
		size_t dir_len;
		int dir_has_sep, has_led_sep;

		if (!out || out_sz == 0)
				return;

		dir_len = strlen(dir);
		dir_has_sep = (dir_len > 0 &&
					   IS_PATH_SEP(dir[dir_len - 1]));
		has_led_sep = IS_PATH_SEP(entry_name[0]);

		if (dir_has_sep) {
				if (has_led_sep)
					wd_snprintf(out, out_sz, "%s%s", dir, entry_name + 1);
				else wd_snprintf(out, out_sz, "%s%s", dir, entry_name);
		} else {
				if (has_led_sep)
					wd_snprintf(out, out_sz, "%s%s", dir, entry_name);
				else wd_snprintf(out, out_sz, "%s%s%s", dir, __PATH_SEP, entry_name);
		}

		out[out_sz - 1] = '\0';
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
#ifdef WD_WINDOWS
    	wd_strncpy(os, "windows", sizeof(os));
#endif
#ifdef WD_LINUX
    	wd_strncpy(os, "linux", sizeof(os));
#endif
		if (access("/.dockerenv", F_OK) == 0)
			wd_strncpy(os, "linux", sizeof(os));
		else if (getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME"))
				wd_strncpy(os, "windows", sizeof(os));

		os[sizeof(os)-1] = '\0';
		return os;
}

int dir_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
	        return WD_RETN;
	    return WD_RETZ;
}

int path_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0)
	        return WD_RETN;
	    return WD_RETZ;
}

int dir_writable(const char *path)
{
	    if (access(path, W_OK) == 0)
	        return WD_RETN;
	    return WD_RETZ;
}

int path_acces(const char *path)
{
	    if (access(path, F_OK) == 0)
	        return WD_RETN;
	    return WD_RETZ;
}

int file_regular(const char *path)
{
		struct stat st;

		if (stat(path, &st) != 0)
				return WD_RETZ;

		return S_ISREG(st.st_mode);
}

int file_same_file(const char *a, const char *b)
{
		struct stat sa, sb;

		if (stat(a, &sa) != 0)
				return WD_RETZ;
		if (stat(b, &sb) != 0)
				return WD_RETZ;

		return (sa.st_ino == sb.st_ino &&
				sa.st_dev == sb.st_dev);
}

int
ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[WD_PATH_MAX];
		char *parent;

		if (strlen(dest) >= sizeof(tmp))
				return -WD_RETN;

		wd_strncpy(tmp, dest, sizeof(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		parent = dirname(tmp);
		if (!parent)
				return -WD_RETN;

		wd_strncpy(out_parent, parent, n);
		out_parent[n-1] = '\0';

		return WD_RETZ;
}

int kill_process(const char *entry_name)
{
		if (!entry_name)
			return -WD_RETN;
		char cmd[WD_PATH_MAX];
#ifndef WD_WINDOWS
		wd_snprintf(cmd, sizeof(cmd),
				"pkill -SIGTERM \"%s\" > /dev/null", entry_name);
#else
		wd_snprintf(cmd, sizeof(cmd),
				"C:\\Windows\\System32\\taskkill.exe "
				"/IM \"%s\" >nul", entry_name);
#endif
		return wd_run_command(cmd);
}

static int
wd_match_filename(const char *entry_name, const char *pattern)
{
		if (strchr(pattern, '*') || strchr(pattern, '?')) {
#ifdef WD_WINDOWS
				return PathMatchSpecA(entry_name, pattern);
#else
				return (fnmatch(pattern, entry_name, 0) == 0);
#endif
		} else {
				return (strcmp(entry_name, pattern) == 0);
		}
}

int wd_is_special_dir(const char *entry_name)
{
		return (entry_name[0] == '.' &&
		       (entry_name[1] == '\0' ||
			   (entry_name[1] == '.' && entry_name[2] == '\0')));
}

static int wd_should_ignore_dir(const char *entry_name,
								const char *ignore_dir)
{
		if (!ignore_dir)
				return WD_RETZ;
#ifdef WD_WINDOWS
		return (_stricmp(entry_name, ignore_dir) == 0);
#else
		return (strcmp(entry_name, ignore_dir) == 0);
#endif
}

static void wd_add_found_path(const char *path)
{
		if (wcfg.wd_sef_count < (sizeof(wcfg.wd_sef_found_list) /
								 sizeof(wcfg.wd_sef_found_list[0]))) {
				wd_strncpy(
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
#ifdef WD_WINDOWS
		WIN32_FIND_DATA find_data;
		HANDLE find_handle;
		char sp[WD_MAX_PATH * 2];
		const char *entry_name;

		if (sef_path[strlen(sef_path) - 1] == __PATH_CHR_SEP_WIN32)
				wd_snprintf(sp,
						    sizeof(sp), "%s*", sef_path);
		else
				wd_snprintf(sp,
						    sizeof(sp), "%s\\*", sef_path);
		find_handle = FindFirstFile(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return WD_RETZ;

		do {
				entry_name = find_data.cFileName;
				if (wd_is_special_dir(entry_name))
						continue;

				__set_path_sep(path_buff, sizeof(path_buff), sef_path, entry_name);

				if (find_data.dwFileAttributes &
						FILE_ATTRIBUTE_DIRECTORY) {
						if (wd_should_ignore_dir(entry_name, ignore_dir))
								continue;
						if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
								FindClose(find_handle);
								return WD_RETN;
						}
				} else {
						if (wd_match_filename(entry_name, sef_name)) {
								wd_add_found_path(path_buff);
								FindClose(find_handle);
								return WD_RETN;
						}
				}
		} while (FindNextFile(find_handle, &find_data));

		FindClose(find_handle);
#else
		DIR *dir;
		struct dirent *item;
		struct stat statbuf;
		const char *entry_name;

		dir = opendir(sef_path);
		if (!dir)
				return WD_RETZ;

		while ((item = readdir(dir)) != NULL) {
				entry_name = item->d_name;
				if (wd_is_special_dir(entry_name))
						continue;

				__set_path_sep(path_buff, sizeof(path_buff), sef_path, entry_name);

				if (item->d_type == DT_DIR) {
						if (wd_should_ignore_dir(entry_name, ignore_dir))
								continue;
						if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
								closedir(dir);
								return WD_RETN;
						}
				} else if (item->d_type == DT_REG) {
						if (wd_match_filename(entry_name, sef_name)) {
								wd_add_found_path(path_buff);
								closedir(dir);
								return WD_RETN;
						}
				} else {
						if (stat(path_buff, &statbuf) == -1)
								continue;

						if (S_ISDIR(statbuf.st_mode)) {
								if (wd_should_ignore_dir(entry_name, ignore_dir))
										continue;
								if (wd_sef_fdir(path_buff, sef_name, ignore_dir)) {
										closedir(dir);
										return WD_RETN;
								}
						} else if (S_ISREG(statbuf.st_mode)) {
								if (wd_match_filename(entry_name, sef_name)) {
										wd_add_found_path(path_buff);
										closedir(dir);
										return WD_RETN;
								}
						}
				}
		}

		closedir(dir);
#endif

		return WD_RETZ;
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

static void wd_check_compiler_options(int *compatibility, int *optimized_lt)
{
		char run_cmd[WD_PATH_MAX + 258];
		FILE *proc_file;
		char log_line[1024];

		if (path_acces(".compiler_test.log"))
			remove(".compiler_test.log");

		snprintf(run_cmd, sizeof(run_cmd),
					"%s -___DDDDDDDDDDDDDDDDD "
					"-___DDDDDDDDDDDDDDDDD"
					"-___DDDDDDDDDDDDDDDDD-"
					"___DDDDDDDDDDDDDDDDD > .compiler_test.log",
					wcfg.wd_sef_found_list[0]);
		wd_run_command(run_cmd);

		int found_Z = 0, found_ver = 0;
		proc_file = fopen(".compiler_test.log", "r");

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
			pr_error(stdout, "Failed to open .compiler_test.log");
		}

		if (path_acces(".compiler_test.log"))
				remove(".compiler_test.log");
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
				return WD_RETZ;
		}

		_toml_config = toml_parse_file(proc_file, error_buffer, sizeof(error_buffer));
		fclose(proc_file);

		if (!_toml_config) {
				pr_error(stdout, "Parsing TOML: %s", error_buffer);
				return WD_RETZ;
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
		return WD_RETN;
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
#ifdef WD_WINDOWS
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

					const char *last_slash = strrchr(base_path, __PATH_CHR_SEP_WIN32);
					if (last_slash &&
						strcmp(last_slash + 1,
							   find_data.cFileName) == 0)
						continue;

					wd_snprintf(fp, sizeof(fp), "%s/%s",
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

					const char *last_slash = strrchr(base_path, __PATH_CHR_SEP_LINUX);
					if (last_slash &&
						strcmp(last_slash + 1,
							   item->d_name) == 0)
						continue;

					wd_snprintf(fp, sizeof(fp), "%s/%s",
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
				return WD_RETZ;
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
						return WD_RETN;
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
				return WD_RETN;
		}

		char error_buffer[256];
		toml_table_t *_toml_config;
		FILE *proc_f = fopen("watchdogs.toml", "r");
		_toml_config = toml_parse_file(proc_f, error_buffer, sizeof(error_buffer));
		if (proc_f) fclose(proc_f);

		if (!_toml_config) {
			pr_error(stdout, "parsing TOML: %s", error_buffer);
			start_chain(NULL);
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
						else {
							wcfg.wd_is_samp = CRC32_TRUE;
							wcfg.wd_ptr_samp = strdup(bin_val.u.s);
						}
						wcfg.wd_toml_binary = strdup(bin_val.u.s);
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

		return WD_RETZ;
}

static int _try_mv_without_sudo(const char *src, const char *dest) {
	    char size_mv[WD_PATH_MAX];
	    if (is_native_windows())
	        wd_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wd_snprintf(size_mv, sizeof(size_mv), "mv -f %s %s", src, dest);
	    int ret = wd_run_command(size_mv);
	    return ret;
}

static int __mv_with_sudo(const char *src, const char *dest) {
	    char size_mv[WD_PATH_MAX];
	    if (is_native_windows())
	        wd_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wd_snprintf(size_mv, sizeof(size_mv), "sudo mv -f %s %s", src, dest);
	    int ret = wd_run_command(size_mv);
	    return ret;
}

static int _try_cp_without_sudo(const char *src, const char *dest) {
	    char size_cp[WD_PATH_MAX];
	    if (is_native_windows())
	        wd_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wd_snprintf(size_cp, sizeof(size_cp), "cp -f %s %s", src, dest);
	    int ret = wd_run_command(size_cp);
	    return ret;
}

static int __cp_with_sudo(const char *src, const char *dest) {
	    char size_cp[WD_PATH_MAX];
	    if (is_native_windows())
	        wd_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wd_snprintf(size_cp, sizeof(size_cp), "sudo cp -f %s %s", src, dest);
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

		return WD_RETN;
}

static void __wd_sef_set_permissions(const char *c_dest)
{
#ifdef WD_WINDOWS
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
		if (ret != WD_RETN)
				return WD_RETN;

		int is_not_sudo = 0;
#ifdef WD_WINDOWS
		is_not_sudo = 1;
#else
		is_not_sudo = wd_run_command("sudo echo > /dev/null");
#endif
		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved without sudo: '%s' -> '%s'", c_src, c_dest);
					return WD_RETZ;
			}
		} else {
			mv_ret = __mv_with_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved with sudo: '%s' -> '%s'", c_src, c_dest);
					return WD_RETZ;
			}
		}

		return WD_RETN;
}

int wd_sef_wcopy(const char *c_src, const char *c_dest)
{
		int ret, cp_ret;

		ret = __wd_sef_safety(c_src, c_dest);
		if (ret != WD_RETN)
				return WD_RETN;

		int is_not_sudo = 0;
#ifdef WD_WINDOWS
		is_not_sudo = 1;
#else
		is_not_sudo = wd_run_command("sudo echo > /dev/null");
#endif
		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying without sudo: '%s' -> '%s'", c_src, c_dest);
					return WD_RETZ;
			}
		} else {
			cp_ret = __cp_with_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wd_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying with sudo: '%s' -> '%s'", c_src, c_dest);
					return WD_RETZ;
			}
		}

		return WD_RETN;
}
