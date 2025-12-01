#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <ftw.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <stddef.h>
#include <libgen.h>
#include <inttypes.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/types.h>

#include "utils.h"

#ifdef WG_LINUX
	#include <termios.h>
#endif
	#if defined(WG_ANDROID)
ssize_t sendfile(int out_fd,
			int in_fd, off_t *offset, size_t count) {
		char buf[8192];
		size_t left = count;
		while (left > 0) {
			ssize_t n = read(in_fd, buf, sizeof(buf));
			if (n <= 0) return n;
			write(out_fd, buf, n);
			left -= n;
		}
		return count;
}
#elif !defined(WG_ANDROID) && defined(WG_LINUX)
	#include <sys/sendfile.h>
#endif

#ifndef _NCURSES
	#include <ncursesw/curses.h>
#else
	#include <ncurses.h>
#endif
#include <readline/readline.h>
#include <readline/history.h>

#include "extra.h"
#include "units.h"
#include "package.h"
#include "crypto.h"

const char* __command[] = {
	    "help",       "exit",      "kill",      "title",     "sha256",
	    "crc32",      "djb2",      "time",      "config",    "stopwatch",
	    "install",    "hardware",  "gamemode",  "pawncc",    "log",
	    "compile",    "running",   "compiles",  "stop",      "restart",
	    "wanion",     "tracker",   "ls",        "ping",      "clear",
	    "nslookup",   "netstat",   "ipconfig",  "uname",     "hostname",
	    "whoami",     "arp",       "route",     "df",        "du",
	    "ps"
};

const size_t
		__command_len =
sizeof(__command) / sizeof(__command[0]);

WatchdogConfig wgconfig = {
	    .wg_toml_os_type = NULL,
	    .wg_toml_binary = NULL,
	    .wg_toml_config = NULL,
		.wg_toml_logs = NULL,
	    .wg_ipawncc = 0,
	    .wg_idepends = 0,
	    .wg_os_type = CRC32_FALSE,
	    .wg_sel_stat = 0,
	    .wg_is_samp = CRC32_FALSE,
	    .wg_is_omp = CRC32_FALSE,
	    .wg_ptr_samp = NULL,
	    .wg_ptr_omp = NULL,
	    .wg_compiler_stat = 0,
	    .wg_sef_count = RATE_SEF_EMPTY,
	    .wg_sef_found_list = { { RATE_SEF_EMPTY } },
	    .wg_toml_aio_opt = NULL,
	    .wg_toml_aio_repo = NULL,
	    .wg_toml_gm_input = NULL,
	    .wg_toml_gm_output = NULL,
		.wg_toml_key_ai = NULL,
		.wg_toml_chatbot_ai = NULL,
		.wg_toml_models_ai = NULL
};

void wg_sef_fdir_reset(void) {
		size_t i, sef_max_entries;
		sef_max_entries = sizeof(wgconfig.wg_sef_found_list) /
						  sizeof(wgconfig.wg_sef_found_list[0]);

		for (i = 0; i < sef_max_entries; i++)
			wgconfig.wg_sef_found_list[i][0] = '\0';

		wgconfig.wg_sef_count = RATE_SEF_EMPTY;
		memset(wgconfig.wg_sef_found_list,
			RATE_SEF_EMPTY, sizeof(wgconfig.wg_sef_found_list));
}

#ifdef WG_WINDOWS
size_t win_strlcpy(char *dst, const char *src, size_t size)
{
	    size_t len = strlen(src);
	    if (size > 0) {
	        size_t copy = (len >= size) ? size - 1 : len;
	        memcpy(dst, src, copy);
	        dst[copy] = 0;
	    }
	    return len;
}

size_t win_strlcat(char *dst, const char *src, size_t size)
{
	    size_t dlen = strlen(dst);
	    size_t slen = strlen(src);
	    if (dlen < size) {
	        size_t space = size - dlen - 1;
	        size_t copy = (slen > space) ? space : slen;
	        memcpy(dst + dlen, src, copy);
	        dst[dlen + copy] = 0;
	        return dlen + slen;
	    }
	    return size + slen;
}

int win_ftruncate(FILE *file, long length) {
	    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
	    if (hFile == INVALID_HANDLE_VALUE)
	        return -1;

	    LARGE_INTEGER li;
	    li.QuadPart = length;

	    if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) == 0)
	        return -1;
	    if (SetEndOfFile(hFile) == 0)
	        return -1;

	    return 0;
}
#endif

char *wg_get_cwd(void) {
	static char wg_work_dir[WG_PATH_MAX];
		if (wg_work_dir[0] == '\0') {
#ifdef WG_WINDOWS
			DWORD size_len;
			size_len = GetCurrentDirectoryA(sizeof(wg_work_dir), wg_work_dir);
			if (size_len == 0 || size_len >= sizeof(wg_work_dir))
				wg_work_dir[0] = '\0';
#else
			ssize_t size_len;
			size_len = readlink("/proc/self/cwd", wg_work_dir, sizeof(wg_work_dir) - 1);
			if (size_len < 0)
				wg_work_dir[0] = '\0';
			else
				wg_work_dir[size_len] = '\0';
#endif
    	}
    	return wg_work_dir;
}

char* wg_masked_text(int reveal, const char *text) {
	    if (!text)
	    	return NULL;

	    int len = (int)strlen(text);
	    if (reveal < 0)
	    	reveal = 0;
	    if (reveal > len)
	    	reveal = len;

	    char *masked;
	    masked = wg_malloc((size_t)len + 1);
	    if (!masked)
	    	return NULL;

	    if (reveal > 0)
	        memcpy(masked, text, (size_t)reveal);

	    for (int i = reveal; i < len; ++i)
	        masked[i] = '?';

	    masked[len] = '\0';
	    return masked;
}

int wg_mkdir(const char *path) {
		char tmp[PATH_MAX];
		char *p = NULL;
		size_t len;

		if (!path || !*path) return -1;

		snprintf(tmp, sizeof(tmp), "%s", path);
		len = strlen(tmp);

		if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

		for (p = tmp + 1; *p; p++) {
			if (*p == '/') {
				*p = '\0';
				if (MKDIR(tmp) != 0) {
					return -1;
				}
				*p = '/';
			}
		}

		if (MKDIR(tmp) != 0) {
			return -1;
		}

		return 0;
}

void wg_escaping_json(char *dest, const char *src, size_t dest_size) {
	    if (dest_size == 0) return;
	    
	    char *ptr = dest;
	    size_t remaining = dest_size;
	    
	    while (*src && remaining > 1) {
	        size_t needed = 1;
	        const char *replacement = NULL;
	        
	        switch (*src) {
	            case '\"': replacement = "\\\""; needed = 2; break;
	            case '\\': replacement = "\\\\"; needed = 2; break;
	            case '\b': replacement = "\\b"; needed = 2; break;
	            case '\f': replacement = "\\f"; needed = 2; break;
	            case '\n': replacement = "\\n"; needed = 2; break;
	            case '\r': replacement = "\\r"; needed = 2; break;
	            case '\t': replacement = "\\t"; needed = 2; break;
	            default: 
	                *ptr++ = *src++;
	                remaining--;
	                continue;
	        }
	        
	        if (needed >= remaining) break;
	        
	        if (replacement) {
	            memcpy(ptr, replacement, needed);
	            ptr += needed;
	            remaining -= needed;
	        }
	        src++;
	    }
	    
	    *ptr = '\0';
}

int wg_run_command(const char *reg_command) {
#if defined(WG_ANDROID)
__asm__ volatile(
	    "mov x0, #0\n\t"
	    "mov x1, #0\n\t"
	    :
	    :
	    : "x0", "x1"
);
#elif !defined(WG_ANDROID) && defined(WG_LINUX)
__asm__ volatile (
	    "movl $0, %%eax\n\t"
	    "movl $0, %%ebx\n\t"
	    :
	    :
	    : "%eax", "%ebx"
);
#endif
    	if (strlen(reg_command) >= WG_MAX_PATH) {
    		pr_error(stdout, "register command too long!");
    		return -1;
    	}

	    char
			size_command[ WG_MAX_PATH ]
			; /* 4096 */

	    wg_snprintf(size_command,
	    			sizeof(size_command),
					"%s",
					reg_command);

	    if (reg_command[0] == '\0')
	    		return 0;

	    return system(size_command);
}

int is_pterodactyl_env(void)
{
		int is_ptero = 0;
		if (path_exists("/etc/pterodactyl") ||
			path_exists("/var/lib/pterodactyl") ||
			path_exists("/var/lib/pterodactyl/volumes") ||
			path_exists("/srv/daemon/config/core.json") ||
			getenv("PTERODACTYL_SERVER_UUID") != NULL ||
			getenv("PTERODACTYL_NODE_ID") != NULL ||
			getenv("PTERODACTYL_ALLOC_ID") != NULL ||
			getenv("PTERODACTYL_SERVER_EXTERNAL_ID") != NULL)
		{
			is_ptero = 1;
		}

		return is_ptero;
}

int is_termux_env(void)
{
		int is_termux = 0;
#if defined(WG_LINUX)
		is_termux = 1;
		return is_termux;
#endif
		if (path_exists("/data/data/com.termux/files/usr/local/lib/") == 1 ||
			path_exists("/data/data/com.termux/files/usr/lib/") == 1 ||
			path_exists("/data/data/com.termux/arm64/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/arm32/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/amd32/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/amd64/usr/lib") == 1)
		{
			is_termux = 1;
		}

		return is_termux;
}

int is_native_windows(void)
{
#if defined(WG_LINUX) || defined(WG_ANDROID)
		return 0;
#endif
		char* msys2_env;
		msys2_env = getenv("MSYSTEM");
		if (msys2_env)
			return 0;
		else
			return 1;
}

void wg_printfile(const char *path) {
#ifdef WG_WINDOWS
	    int fd = open(path, O_RDONLY);
	    if (fd < 0) return;

	    static char buf[(1 << 20) + 1];
	    for (;;) {
	        ssize_t n = read(fd, buf, sizeof(buf) - 1);
	        if (n <= 0) break;

	        buf[n] = '\0';

	        ssize_t w = 0;
	        while (w < n) {
	            ssize_t k = write(STDOUT_FILENO, buf + w, n - w);
	            if (k <= 0) { close(fd); return; }
	            w += k;
	        }
	    }

	    close(fd);
#else
	    int fd = open(path, O_RDONLY);
	    if (fd < 0) return;

	    off_t off = 0;
	    struct stat st;
	    if (fstat(fd, &st) < 0) { close(fd); return; }

	    static char buf[(1 << 20) + 1];
	    while (off < st.st_size) {
	        ssize_t to_read;
			to_read = (st.st_size - off) < (sizeof(buf) - 1) ? (st.st_size - off) : (sizeof(buf) - 1);
	        ssize_t n = pread(fd, buf, to_read, off);
	        if (n <= 0) break;
	        off += n;

	        buf[n] = '\0';

	        ssize_t w = 0;
	        while (w < n) {
	            ssize_t k = write(STDOUT_FILENO, buf + w, n - w);
	            if (k <= 0) { close(fd); return; }
	            w += k;
	        }
	    }

	    close(fd);
#endif
}

int wg_console_title(const char *title)
{
		const char *new_title;
		if (!title)
				new_title = watchdogs_release;
		else
				new_title = title;
#ifdef WG_WINDOWS
		SetConsoleTitleA(new_title);
#else
	    if (isatty(STDOUT_FILENO))
	        	printf("\033]0;%s\007", new_title);
#endif
		return 0;
}

void wg_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		slash = strchr(src, __PATH_CHR_SEP_LINUX);
#ifdef WG_WINDOWS
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

		wg_snprintf(dst, dst_sz, "%s", src);
}

bool
wg_strcase(const char *text, const char *pattern) {
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

unsigned char wg_tolower(unsigned char c) {
    	return (unsigned char)(c + ((c - 'A') <= ('Z' - 'A') ? 32 : 0));
}

__attribute__((pure))
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
	        pat[i] = wg_tolower((unsigned char)pattern[i]);

	    for (size_t i = 0; i < 256; i++)
	        skip[i] = (uint8_t)m;

	    for (size_t i = 0; i < m - 1; i++)
	        skip[pat[i]] = (uint8_t)(m - 1 - i);

	    const unsigned char *txt = (const unsigned char *)text;
	    size_t i = 0;

	    while (i <= n - m) {
	        unsigned char current_char = wg_tolower(txt[i + m - 1]);

	        if (current_char == pat[m - 1]) {
	            int j = (int)m - 2;
	            while (j >= 0 && pat[j] == wg_tolower(txt[i + j])) {
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

__attribute__((pure))
char* strreplace(const char *source, const char *old_sub, const char *new_sub) {
	    if (!source || !old_sub || !new_sub) return NULL;

	    size_t source_len = strlen(source);
	    size_t old_sub_len = strlen(old_sub);
	    size_t new_sub_len = strlen(new_sub);

	    size_t result_len = source_len;
	    const char *pos = source;
	    while ((pos = strstr(pos, old_sub)) != NULL) {
	        result_len += new_sub_len - old_sub_len;
	        pos += old_sub_len;
	    }

	    char *result = wg_malloc(result_len + 1);
	    if (!result) return NULL;

	    size_t i = 0, j = 0;
	    while (source[i]) {
	        if (strncmp(&source[i], old_sub, old_sub_len) == 0) {
	            strncpy(&result[j], new_sub, new_sub_len);
	            i += old_sub_len;
	            j += new_sub_len;
	        } else {
	            result[j++] = source[i++];
	        }
	    }

	    result[j] = '\0';
	    return result;
}

void wg_escape_quotes(char *dest, size_t size, const char *src)
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

void __set_path_sep(char *out, size_t out_sz,
					const char *dir, const char *entry_name)
{
	    if (!out || out_sz == 0 || !dir || !entry_name) return;
    
	    size_t dir_len;
	    int dir_has_sep, has_led_sep;
	    size_t entry_len = strlen(entry_name);

		dir_len = strlen(dir);
		dir_has_sep = (dir_len > 0 &&
					   IS_PATH_SEP(dir[dir_len - 1]));
		has_led_sep = IS_PATH_SEP(entry_name[0]);

	    size_t max_needed = dir_len + entry_len + 2;

	    if (max_needed >= out_sz) {
	        out[0] = '\0';
	        return;
	    }

		if (dir_has_sep) {
				if (has_led_sep)
					wg_snprintf(out, out_sz, "%s%s", dir, entry_name + 1);
				else wg_snprintf(out, out_sz, "%s%s", dir, entry_name);
		} else {
				if (has_led_sep)
					wg_snprintf(out, out_sz, "%s%s", dir, entry_name);
				else wg_snprintf(out, out_sz, "%s%s%s", dir, __PATH_SEP, entry_name);
		}

		out[out_sz - 1] = '\0';
}

__attribute__((pure))
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

const char *wg_find_near_command(const char *command,
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

const char *wg_fetch_os(void)
{
    	static char os[64] = "unknown";
#ifdef WG_WINDOWS
    	wg_strncpy(os, "windows", sizeof(os));
#endif
#ifdef WG_LINUX
    	wg_strncpy(os, "linux", sizeof(os));
#endif
		if (access("/.dockerenv", F_OK) == 0)
			wg_strncpy(os, "linux", sizeof(os));
		else if (getenv("WSL_INTEROP") ||
				 getenv("WSL_DISTRO_NAME"))
				wg_strncpy(os, "windows", sizeof(os));

		os[sizeof(os)-1] = '\0';
		return os;
}

int dir_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
	        return 1;
	    return 0;
}

int path_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0)
	        return 1;
	    return 0;
}

int dir_writable(const char *path)
{
	    if (access(path, W_OK) == 0)
	        return 1;
	    return 0;
}

int path_access(const char *path)
{
	    if (access(path, F_OK) == 0)
	        return 1;
	    return 0;
}

int file_regular(const char *path)
{
		struct stat st;

		if (stat(path, &st) != 0)
				return 0;

		return S_ISREG(st.st_mode);
}

int file_same_file(const char *a, const char *b)
{
		struct stat sa, sb;

		if (stat(a, &sa) != 0)
				return 0;
		if (stat(b, &sb) != 0)
				return 0;

		return (sa.st_ino == sb.st_ino &&
				sa.st_dev == sb.st_dev);
}

__attribute__((pure))
int ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[WG_PATH_MAX];
		char *parent;

		if (strlen(dest) >= sizeof(tmp))
				return -1;

		wg_strncpy(tmp, dest, sizeof(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		parent = dirname(tmp);
		if (!parent)
				return -1;

		wg_strncpy(out_parent, parent, n);
		out_parent[n-1] = '\0';

		return 0;
}

int kill_process(const char *entry_name)
{
		if (!entry_name)
			return -1;
		char reg_command[WG_PATH_MAX];
#ifndef WG_WINDOWS
		wg_snprintf(reg_command, sizeof(reg_command),
				"pkill -SIGTERM \"%s\" > /dev/null", entry_name);
#else
		wg_snprintf(reg_command, sizeof(reg_command),
				"C:\\Windows\\System32\\taskkill.exe "
				"/IM \"%s\" >nul 2>&1", entry_name);
#endif
		return wg_run_command(reg_command);
}

static int
wg_match_filename(const char *entry_name, const char *pattern)
{
		if (strchr(pattern, '*') || strchr(pattern, '?')) {
#ifdef WG_WINDOWS
				return PathMatchSpecA(entry_name, pattern);
#else
				return (fnmatch(pattern, entry_name, 0) == 0);
#endif
		} else {
				return (strcmp(entry_name, pattern) == 0);
		}
}

int wg_is_special_dir(const char *entry_name)
{
		return (entry_name[0] == '.' &&
		       (entry_name[1] == '\0' ||
			   (entry_name[1] == '.' && entry_name[2] == '\0')));
}

static int wg_fetch_ignore_dir(const char *entry_name,
								const char *ignore_dir)
{
		if (!ignore_dir)
				return 0;
#ifdef WG_WINDOWS
		return (_stricmp(entry_name, ignore_dir) == 0);
#else
		return (strcmp(entry_name, ignore_dir) == 0);
#endif
}

static void wg_add_found_path(const char *path)
{
		if (wgconfig.wg_sef_count < (sizeof(wgconfig.wg_sef_found_list) /
								     sizeof(wgconfig.wg_sef_found_list[0]))) {
				wg_strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count],
					path,
					MAX_SEF_PATH_SIZE);
				wgconfig.wg_sef_found_list \
					[wgconfig.wg_sef_count] \
					[MAX_SEF_PATH_SIZE - 1] = '\0';
				++wgconfig.wg_sef_count;
		}
}

int wg_sef_fdir(const char *sef_path,
				const char *sef_name,
				const char *ignore_dir)
{
		char
			size_path[MAX_SEF_PATH_SIZE];
#ifdef WG_WINDOWS
		HANDLE find_handle;
		char sp[WG_MAX_PATH * 2];
		const char *entry_name;
		WIN32_FIND_DATA find_data;

		if (sef_path[strlen(sef_path) - 1] == __PATH_CHR_SEP_WIN32)
			wg_snprintf(sp,
				sizeof(sp), "%s*", sef_path);
		else
			wg_snprintf(sp,
				sizeof(sp), "%s%s*", sef_path, __PATH_STR_SEP_WIN32);

		find_handle = FindFirstFile(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
			return 0;

		do {
			entry_name = find_data.cFileName;
			if (wg_is_special_dir(entry_name))
				continue;

			__set_path_sep(size_path, sizeof(size_path), sef_path, entry_name);

			if (find_data.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY)
			{
				if (wg_fetch_ignore_dir(entry_name, ignore_dir))
					continue;
				if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
					FindClose(find_handle);
					return 1;
				}
			} else {
				if (wg_match_filename(entry_name, sef_name)) {
					wg_add_found_path(size_path);
					FindClose(find_handle);
					return 1;
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
			return 0;

		while ((item = readdir(dir)) != NULL) {
			entry_name = item->d_name;
			if (wg_is_special_dir(entry_name))
				continue;

			__set_path_sep(size_path, sizeof(size_path), sef_path, entry_name);

			if (item->d_type == DT_DIR) {
				if (wg_fetch_ignore_dir(entry_name, ignore_dir))
					continue;
				if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
						closedir(dir);
					return 1;
				}
			} else if (item->d_type == DT_REG) {
				if (wg_match_filename(entry_name, sef_name)) {
					wg_add_found_path(size_path);
					closedir(dir);
					return 1;
				}
			} else {
				if (stat(size_path, &statbuf) == -1)
					continue;

				if (S_ISDIR(statbuf.st_mode)) {
					if (wg_fetch_ignore_dir(entry_name, ignore_dir))
						continue;
					if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
						closedir(dir);
						return 1;
					}
				} else if (S_ISREG(statbuf.st_mode)) {
					if (wg_match_filename(entry_name, sef_name)) {
						wg_add_found_path(size_path);
						closedir(dir);
						return 1;
					}
				}
			}
		}

		closedir(dir);
#endif

		return 0;
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

static void wg_check_compiler_options(int *compatibility, int *optimized_lt)
{
		char run_cmd[WG_PATH_MAX * 2];
		FILE *proc_file;
		char log_line[1024];

		if (path_exists(".watchdogs") == 0)
			MKDIR(".watchdogs");

		if (path_access(".watchdogs/compiler_test.log"))
			remove(".watchdogs/compiler_test.log");

		wg_snprintf(run_cmd, sizeof(run_cmd),
					"%s -0000000U > .watchdogs/compiler_test.log",
					wgconfig.wg_sef_found_list[0]);
		wg_run_command(run_cmd);

		int found_Z = 0, found_ver = 0;
		proc_file = fopen(".watchdogs/compiler_test.log", "r");

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
			pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
		}

		if (path_access(".watchdogs/compiler_test.log"))
				remove(".watchdogs/compiler_test.log");
}

static int wg_parsewg_toml_config(void)
{
		FILE *proc_file;
		char error_buffer[256];
		toml_table_t *wg_toml_config;
		toml_table_t *general_table;

		proc_file = fopen("watchdogs.toml", "r");
		if (!proc_file) {
				pr_error(stdout, "Cannot read file %s", "watchdogs.toml");
				return 0;
		}

		wg_toml_config = toml_parse_file(proc_file, error_buffer, sizeof(error_buffer));
		fclose(proc_file);

		if (!wg_toml_config) {
				pr_error(stdout, "Parsing TOML: %s", error_buffer);
				return 0;
		}

		general_table = toml_table_in(wg_toml_config, "general");
		if (general_table) {
				toml_datum_t os_val = toml_string_in(general_table, "os");
				if (os_val.ok) {
						wgconfig.wg_toml_os_type = strdup(os_val.u.s);
						wg_free(os_val.u.s);
				}
		}

		toml_free(wg_toml_config);
		return 1;
}

static int wg_find_compiler(const char *wg_os_type)
{
		int is_windows = (strcmp(wg_os_type, "windows") == 0);
		const char *compiler_name = is_windows ? "pawncc.exe" : "pawncc";

		if (wg_server_env() == 1) {
				return wg_sef_fdir("pawno", compiler_name, NULL);
		} else if (wg_server_env() == 2) {
				return wg_sef_fdir("qawno", compiler_name, NULL);
		} else {
				return wg_sef_fdir("pawno", compiler_name, NULL);
		}
}

__attribute__((unused))
static void __toml_base_subdirs(const char *base_path,
								FILE *toml_file, int *first)
{
#ifdef WG_WINDOWS
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;
		char sp[WG_MAX_PATH], fp[WG_MAX_PATH * 2];
		wg_snprintf(sp, sizeof(sp), "%s%s*", base_path, __PATH_STR_SEP_WIN32);

		find_handle = FindFirstFileA(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return;
		do {
				if (find_data.dwFileAttributes &
						FILE_ATTRIBUTE_DIRECTORY) {
					if (wg_is_special_dir(find_data.cFileName))
						continue;

					const char *last_slash = strrchr(base_path, __PATH_CHR_SEP_WIN32);
					if (last_slash &&
						strcmp(last_slash + 1,
							   find_data.cFileName) == 0)
						continue;

					wg_snprintf(fp, sizeof(fp), "%s/%s",
								base_path, find_data.cFileName);

					__toml_add_directory_path(toml_file, first, fp);
					__toml_base_subdirs(fp, toml_file, first);
				}
		} while (FindNextFileA(find_handle, &find_data) != 0);

		FindClose(find_handle);
#else
		DIR *dir;
		struct dirent *item;
		char fp[WG_MAX_PATH * 4];

		dir = opendir(base_path);
		if (!dir)
			return;

		while ((item = readdir(dir)) != NULL) {
				if (item->d_type == DT_DIR) {
					if (wg_is_special_dir(item->d_name))
						continue;

					const char *last_slash = strrchr(base_path, __PATH_CHR_SEP_LINUX);
					if (last_slash &&
						strcmp(last_slash + 1,
							   item->d_name) == 0)
						continue;

					wg_snprintf(fp, sizeof(fp), "%s/%s",
								base_path, item->d_name);

					__toml_add_directory_path(toml_file, first, fp);
					__toml_base_subdirs(fp, toml_file, first);
				}
		}

		closedir(dir);
#endif
}

static void wg_add_compiler_path(FILE *file, const char *path, int *first_item)
{
		if (path_access(path)) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"%s\"", path);
				//*first_item = 0;
				//__toml_base_subdirs(path, file, first_item);
		}
}

static void wg_add_include_paths(FILE *file, int *first_item)
{
		if (path_access("gamemodes")) {
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n        \"gamemodes/\"");
				*first_item = 0;
				//__toml_base_subdirs("gamemodes", file, first_item);
		}

		if (wg_server_env() == 1)
				wg_add_compiler_path(file, "pawno/include/", first_item);
		else if (wg_server_env() == 2)
				wg_add_compiler_path(file, "qawno/include/", first_item);
		else
				wg_add_compiler_path(file, "pawno/include/", first_item);
}

static int _is_samp_ = -1;
static void wg_generate_toml_content(FILE *file, const char *wg_os_type,
							         int has_gamemodes, int compatible,
							         int optimized_lt, char *sef_path)
{
		int first_item = 1;
		if (sef_path[0]) {
			char *ext = strrchr(sef_path, '.');
			if (ext)
					*ext = '\0';
		}
		char *p;
		for (p = sef_path; *p; p++) {
				if (*p == __PATH_CHR_SEP_WIN32)
					*p = __PATH_CHR_SEP_LINUX;
		}

		fprintf(file, "[general]\n");
		fprintf(file, "\tos = \"%s\"\n", wg_os_type);
		if (_is_samp_ == 0) {
			if (!strcmp(wg_os_type, "windows")) {
				fprintf(file, "\tbinary = \"%s\"\n", "omp-server.exe");
				fprintf(file, "\tconfig = \"%s\"\n", "config.json");
			} else if (!strcmp(wg_os_type, "linux")) {
				fprintf(file, "\tbinary = \"%s\"\n", "omp-server");
			}
			fprintf(file, "\tconfig = \"%s\"\n", "config.json");
			fprintf(file, "\tlogs = \"%s\"\n", "log.txt");
		} else {
			if (!strcmp(wg_os_type, "windows")) {
				fprintf(file, "\tbinary = \"%s\"\n", "samp-server.exe");
			} else if (!strcmp(wg_os_type, "linux")) {
				fprintf(file, "\tbinary = \"%s\"\n", "samp-server.exe");
			}
			fprintf(file, "\tconfig = \"%s\"\n", "server.cfg");
			fprintf(file, "\tlogs = \"%s\"\n", "server_log.txt");
		}
		fprintf(file, "\tkeys = \"API_KEY\"\n");
		fprintf(file, "\tchatbot = \"gemini\"\n");
		fprintf(file, "\tmodels = \"gemini-2.5-pro\"\n");

		fprintf(file, "[compiler]\n");

		if (compatible && optimized_lt) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-O2\", \"-;+\", \"-(+\"]\n");
		} else if (compatible) {
				fprintf(file, "\toption = [\"-Z+\", \"-d2\", \"-;+\", \"-(+\"]\n");
		} else {
				fprintf(file, "\toption = [\"-d3\", \"-;+\", \"-(+\"]\n");
		}

		fprintf(file, "\tinclude_path = [");
		wg_add_include_paths(file, &first_item);
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

int wg_toml_configs(void)
{
		int find_pawncc = 0, find_gamemodes = 0;
		int compatibility = 0, optimized_lt = 0;

		const char *wg_os_type;
		FILE *toml_file;

		wg_os_type = wg_fetch_os();

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
				return 0;
			}
		}

		find_pawncc = wg_find_compiler(wg_os_type);
		if (!find_pawncc) {
			if (strcmp(wg_os_type, "windows") == 0)
					find_pawncc = wg_sef_fdir(".", "pawncc.exe", NULL);
			else
					find_pawncc = wg_sef_fdir(".", "pawncc", NULL);
		}

		find_gamemodes = wg_sef_fdir("gamemodes/", "*.pwn", NULL);

		if (find_pawncc) {
			wg_check_compiler_options(&compatibility, &optimized_lt);
		}

		toml_file = fopen("watchdogs.toml", "r");
		if (toml_file) {
			fclose(toml_file);
		} else {
			toml_file = fopen("watchdogs.toml", "w");
			if (!toml_file) {
					pr_error(stdout, "Failed to create watchdogs.toml");
					return 1;
			}

			if (find_pawncc)
				wg_generate_toml_content(toml_file, wg_os_type, find_gamemodes,
					compatibility, optimized_lt, wgconfig.wg_sef_found_list[1]);
			else
				wg_generate_toml_content(toml_file, wg_os_type, find_gamemodes,
					compatibility, optimized_lt, wgconfig.wg_sef_found_list[0]);
			fclose(toml_file);
		}

		if (!wg_parsewg_toml_config()) {
				pr_error(stdout, "Failed to parse TOML configuration");
				return 1;
		}

		char error_buffer[256];
		toml_table_t *wg_toml_config;
		FILE *proc_f = fopen("watchdogs.toml", "r");
		wg_toml_config = toml_parse_file(proc_f, error_buffer, sizeof(error_buffer));
		if (proc_f) fclose(proc_f);

		if (!wg_toml_config) {
			pr_error(stdout, "parsing TOML: %s", error_buffer);
			chain_goto_main(NULL);
		}

		toml_table_t *wg_toml_depends = toml_table_in(wg_toml_config, "depends");
		if (wg_toml_depends) {
				toml_datum_t toml_gh_tokens = toml_string_in(wg_toml_depends, "github_tokens");
				if (toml_gh_tokens.ok)
				{
					wgconfig.wg_toml_github_tokens = strdup(toml_gh_tokens.u.s);
					wg_free(toml_gh_tokens.u.s);
				}
		}

		toml_table_t *wg_toml_compiler = toml_table_in(wg_toml_config, "compiler");
		if (wg_toml_compiler) {
				toml_datum_t input_val = toml_string_in(wg_toml_compiler, "input");
				if (input_val.ok) {
					wgconfig.wg_toml_gm_input = strdup(input_val.u.s);
					wg_free(input_val.u.s);
				}
				toml_datum_t output_val = toml_string_in(wg_toml_compiler, "output");
				if (output_val.ok) {
					wgconfig.wg_toml_gm_output = strdup(output_val.u.s);
					wg_free(output_val.u.s);
				}
		}

		toml_table_t *general_table = toml_table_in(wg_toml_config, "general");
		if (general_table) {
				toml_datum_t bin_val = toml_string_in(general_table, "binary");
				if (bin_val.ok) {
					if (_is_samp_ == 1) {
						wgconfig.wg_is_samp = CRC32_TRUE;
						wgconfig.wg_ptr_samp = strdup(bin_val.u.s);
					}
					else if (_is_samp_ == 0) {
						wgconfig.wg_is_omp = CRC32_TRUE;
						wgconfig.wg_ptr_omp = strdup(bin_val.u.s);
					}
					else {
						wgconfig.wg_is_samp = CRC32_TRUE;
						wgconfig.wg_ptr_samp = strdup(bin_val.u.s);
					}
					wgconfig.wg_toml_binary = strdup(bin_val.u.s);
					wg_free(bin_val.u.s);
				}
				toml_datum_t conf_val = toml_string_in(general_table, "config");
				if (conf_val.ok) {
					wgconfig.wg_toml_config = strdup(conf_val.u.s);
					wg_free(conf_val.u.s);
				}
				toml_datum_t logs_val = toml_string_in(general_table, "logs");
				if (logs_val.ok) {
					wgconfig.wg_toml_logs = strdup(logs_val.u.s);
					wg_free(logs_val.u.s);
				}
				toml_datum_t keys_val = toml_string_in(general_table, "keys");
				if (keys_val.ok) {
					wgconfig.wg_toml_key_ai = strdup(keys_val.u.s);
					wg_free(keys_val.u.s);
				}
				toml_datum_t chatbot_val = toml_string_in(general_table, "chatbot");
				if (chatbot_val.ok) {
					wgconfig.wg_toml_chatbot_ai = strdup(chatbot_val.u.s);
					wg_free(chatbot_val.u.s);
				}
				toml_datum_t models_val = toml_string_in(general_table, "models");
				if (models_val.ok) {
					wgconfig.wg_toml_models_ai = strdup(models_val.u.s);
					wg_free(models_val.u.s);
				}
		}

		toml_free(wg_toml_config);

		if (strcmp(wgconfig.wg_toml_os_type, "windows") == 0) {
			wgconfig.wg_os_type = OS_SIGNAL_WINDOWS;
		} else if (strcmp(wgconfig.wg_toml_os_type, "linux") == 0) {
			wgconfig.wg_os_type = OS_SIGNAL_LINUX;
		}

		return 0;
}

static int _try_mv_without_sudo(const char *src, const char *dest) {
	    char size_mv[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_mv, sizeof(size_mv), "mv -f %s %s", src, dest);
	    int ret = wg_run_command(size_mv);
	    return ret;
}

static int __mv_with_sudo(const char *src, const char *dest) {
	    char size_mv[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_mv, sizeof(size_mv), "sudo mv -f %s %s", src, dest);
	    int ret = wg_run_command(size_mv);
	    return ret;
}

static int _try_cp_without_sudo(const char *src, const char *dest) {
	    char size_cp[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_cp, sizeof(size_cp), "cp -f %s %s", src, dest);
	    int ret = wg_run_command(size_cp);
	    return ret;
}

static int __cp_with_sudo(const char *src, const char *dest) {
	    char size_cp[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_cp, sizeof(size_cp), "sudo cp -f %s %s", src, dest);
	    int ret = wg_run_command(size_cp);
	    return ret;
}

static int __wg_sef_safety(const char *c_src, const char *c_dest)
{
		char parent[WG_PATH_MAX];
		struct stat st;

		if (!c_src || !c_dest)
				pr_error(stdout, "src or dest is null");
		if (!*c_src || !*c_dest)
				pr_error(stdout, "src or dest empty");
		if (strlen(c_src) >= WG_PATH_MAX || strlen(c_dest) >= WG_PATH_MAX)
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

		return 1;
}

static void __wg_sef_set_permissions(const char *c_dest)
{
#ifdef WG_WINDOWS
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

int wg_sef_wmv(const char *c_src, const char *c_dest)
{
		int ret, mv_ret;

		ret = __wg_sef_safety(c_src, c_dest);
		if (ret != 1)
				return 1;

		int is_not_sudo = 0;
#ifdef WG_WINDOWS
		is_not_sudo = 1;
#else
		is_not_sudo = wg_run_command("sudo echo > /dev/null");
#endif
		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved without sudo: '%s' -> '%s'", c_src, c_dest);
					return 0;
			}
		} else {
			mv_ret = __mv_with_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved with sudo: '%s' -> '%s'", c_src, c_dest);
					return 0;
			}
		}

		return 1;
}

int wg_sef_wcopy(const char *c_src, const char *c_dest)
{
		int ret, cp_ret;

		ret = __wg_sef_safety(c_src, c_dest);
		if (ret != 1)
				return 1;

		int is_not_sudo = 0;
#ifdef WG_WINDOWS
		is_not_sudo = 1;
#else
		is_not_sudo = wg_run_command("sudo echo > /dev/null");
#endif
		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying without sudo: '%s' -> '%s'", c_src, c_dest);
					return 0;
			}
		} else {
			cp_ret = __cp_with_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying with sudo: '%s' -> '%s'", c_src, c_dest);
					return 0;
			}
		}

		return 1;
}
