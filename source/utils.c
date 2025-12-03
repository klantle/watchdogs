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
		"crc32",      "djb2",      "time",      "config",    "download",
		"replicate",  "hardware",  "gamemode",  "pawncc",    "log",
		"compile",    "running",   "compiles",  "stop",      "restart",
		"wanion",     "tracker",   "send",      "ls",        "ping",
		"clear",      "nslookup",  "netstat",   "ipconfig",  "uname",
		"hostname",   "whoami",    "arp",       "route",     "df",
		"du",         "ps",        "top",       "free",      "uptime",
		"date",       "cal",       "bc",        "dd",        "telnet",
		"ssh",        "curl",      "wget",      "git",       "zip",
		"unzip",      "tar",       "7z",        "rar",       "md5sum",
		"sha1sum",    "sha256sum", "sha512sum", "djb2sum"
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
		.wg_downloading_file = 0,
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
		.wg_toml_models_ai = NULL,
		.wg_toml_webhooks = NULL
};

/*
 * Resets the SEF (Source File) directory search results by clearing the found list
 * and resetting the count to an empty state. Iterates through all entries in the
 * found list array and sets each first character to null terminator to empty strings,
 * then sets the global counter to empty value and fills the entire array with empty markers.
 */
void wg_sef_fdir_reset(void) {
		size_t i, sef_max_entries;
		/* Calculate maximum number of entries in the found list array */
		sef_max_entries = sizeof(wgconfig.wg_sef_found_list) /
						  sizeof(wgconfig.wg_sef_found_list[0]);

		/* Clear each entry in the found list by setting first character to null */
		for (i = 0; i < sef_max_entries; i++)
			wgconfig.wg_sef_found_list[i][0] = '\0';

		/* Reset the count of found items to empty state */
		wgconfig.wg_sef_count = RATE_SEF_EMPTY;
		/* Fill the entire found list array with empty markers */
		memset(wgconfig.wg_sef_found_list,
			RATE_SEF_EMPTY, sizeof(wgconfig.wg_sef_found_list));
}

#ifdef WG_WINDOWS
/*
 * Windows implementation of strlcpy - safely copies string from source to destination
 * with null termination guarantee. Calculates source length, copies up to size-1 bytes,
 * always null-terminates destination, and returns total source length.
 */
size_t win_strlcpy(char *dst, const char *src, size_t size)
{
	    size_t len = strlen(src);
	    /* Only copy if destination buffer has space */
	    if (size > 0) {
	        /* Determine how many characters can be copied (leave room for null terminator) */
	        size_t copy = (len >= size) ? size - 1 : len;
	        memcpy(dst, src, copy);
	        dst[copy] = 0; /* Always null-terminate */
	    }
	    return len; /* Return length of source string */
}

/*
 * Windows implementation of strlcat - safely concatenates source string to destination
 * with buffer size protection. Calculates current destination length and available space,
 * copies only what fits, null-terminates, and returns total length that would have been
 * created if buffer was large enough.
 */
size_t win_strlcat(char *dst, const char *src, size_t size)
{
	    size_t dlen = strlen(dst);
	    size_t slen = strlen(src);
	    /* Check if there's space in destination buffer */
	    if (dlen < size) {
	        size_t space = size - dlen - 1; /* Available space minus null terminator */
	        size_t copy = (slen > space) ? space : slen; /* Copy only what fits */
	        memcpy(dst + dlen, src, copy);
	        dst[dlen + copy] = 0; /* Null-terminate the concatenated string */
	        return dlen + slen; /* Return total length of combined strings */
	    }
	    /* If destination already fills buffer, return what length would have been */
	    return size + slen;
}

/*
 * Windows implementation of ftruncate - truncates a file to specified length
 * using Windows file handles. Converts FILE pointer to Windows HANDLE,
 * sets file pointer to specified length, and sets end of file at that position.
 */
int win_ftruncate(FILE *file, long length) {
	    /* Get Windows file handle from C file descriptor */
	    HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(file));
	    if (hFile == INVALID_HANDLE_VALUE)
	        return -1;

	    LARGE_INTEGER li;
	    li.QuadPart = length; /* Set 64-bit length value */

	    /* Move file pointer to specified position */
	    if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) == 0)
	        return -1;
	    /* Truncate file at current position */
	    if (SetEndOfFile(hFile) == 0)
	        return -1;

	    return 0; /* Success */
}
#endif

/*
 * Retrieves current working directory with caching mechanism. Returns a static buffer
 * containing the working directory, initializing it on first call. Uses OS-specific
 * methods: GetCurrentDirectoryA on Windows or readlink on /proc/self/cwd on Linux.
 * Result is cached for subsequent calls.
 */
char *wg_get_cwd(void) {
	static char wg_work_dir[WG_PATH_MAX]; /* Static buffer for caching working directory */
		/* Initialize buffer only if empty (first call) */
		if (wg_work_dir[0] == '\0') {
#ifdef WG_WINDOWS
			DWORD size_len;
			/* Windows: Get current directory using WinAPI */
			size_len = GetCurrentDirectoryA(sizeof(wg_work_dir), wg_work_dir);
			if (size_len == 0 || size_len >= sizeof(wg_work_dir))
				wg_work_dir[0] = '\0'; /* Error or buffer too small */
#else
			ssize_t size_len;
			/* Linux: Read symbolic link of current directory from proc filesystem */
			size_len = readlink("/proc/self/cwd", wg_work_dir, sizeof(wg_work_dir) - 1);
			if (size_len < 0)
				wg_work_dir[0] = '\0'; /* Readlink failed */
			else
				wg_work_dir[size_len] = '\0'; /* Null-terminate the result */
#endif
    	}
    	return wg_work_dir; /* Return cached working directory */
}

/*
 * Creates a masked version of text where characters beyond reveal count are replaced
 * with '?'. Allocates new string, copies first 'reveal' characters unchanged,
 * masks remaining characters with question marks. Handles edge cases like negative
 * reveal values and NULL input.
 */
char* wg_masked_text(int reveal, const char *text) {
	    if (!text) /* Handle NULL input */
	    	return NULL;

	    int len = (int)strlen(text);
	    /* Clamp reveal value between 0 and text length */
	    if (reveal < 0)
	    	reveal = 0;
	    if (reveal > len)
	    	reveal = len;

	    char *masked;
	    /* Allocate memory for masked string plus null terminator */
	    masked = wg_malloc((size_t)len + 1);
	    if (!masked)
	    	return NULL; /* Allocation failed */

	    /* Copy visible characters if any should be revealed */
	    if (reveal > 0)
	        memcpy(masked, text, (size_t)reveal);

	    /* Mask remaining characters with question marks */
	    for (int i = reveal; i < len; ++i)
	        masked[i] = '?';

	    masked[len] = '\0'; /* Null-terminate the masked string */
	    return masked;
}

/*
 * Creates directory and all necessary parent directories recursively.
 * Processes path character by character, creating each directory component
 * as it encounters path separators. Handles trailing slashes and ensures
 * all intermediate directories exist.
 */
int wg_mkdir(const char *path) {
		char tmp[PATH_MAX];
		char *p = NULL;
		size_t len;

		if (!path || !*path) return -1; /* Validate input */

		/* Copy path to temporary buffer for modification */
		snprintf(tmp, sizeof(tmp), "%s", path);
		len = strlen(tmp);

		/* Remove trailing slash if present */
		if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

		/* Iterate through path, creating directories at each separator */
		for (p = tmp + 1; *p; p++) {
			if (*p == '/') {
				*p = '\0'; /* Temporarily truncate at separator */
				/* Create directory component */
				if (MKDIR(tmp) != 0) {
					return -1; /* Failed to create directory */
				}
				*p = '/'; /* Restore separator */
			}
		}

		/* Create the final directory */
		if (MKDIR(tmp) != 0) {
			return -1;
		}

		return 0; /* Success */
}

/*
 * Escapes special JSON characters in source string for safe JSON embedding.
 * Processes source character by character, replacing control characters and
 * quotes with their JSON escape sequences. Ensures destination buffer doesn't overflow.
 */
void wg_escaping_json(char *dest, const char *src, size_t dest_size) {
	    if (dest_size == 0) return; /* No space in destination */
	    
	    char *ptr = dest;
	    size_t remaining = dest_size; /* Track remaining buffer space */
	    
	    while (*src && remaining > 1) {
	        size_t needed = 1; /* Default: one character needed */
	        const char *replacement = NULL; /* Escape sequence if needed */
	        
	        /* Check for characters that need escaping in JSON */
	        switch (*src) {
	            case '\"': replacement = "\\\""; needed = 2; break;
	            case '\\': replacement = "\\\\"; needed = 2; break;
	            case '\b': replacement = "\\b"; needed = 2; break;
	            case '\f': replacement = "\\f"; needed = 2; break;
	            case '\n': replacement = "\\n"; needed = 2; break;
	            case '\r': replacement = "\\r"; needed = 2; break;
	            case '\t': replacement = "\\t"; needed = 2; break;
	            default: 
	                /* Regular character, copy as-is */
	                *ptr++ = *src++;
	                remaining--;
	                continue;
	        }
	        
	        /* Check if escape sequence fits in remaining buffer */
	        if (needed >= remaining) break;
	        
	        if (replacement) {
	            /* Copy escape sequence to destination */
	            memcpy(ptr, replacement, needed);
	            ptr += needed;
	            remaining -= needed;
	        }
	        src++; /* Move to next source character */
	    }
	    
	    *ptr = '\0'; /* Null-terminate the escaped string */
}

/*
 * Executes a system command with platform-specific assembly preamble and safety checks.
 * Includes assembly instructions for Android and Linux to clear registers before execution.
 * Validates command length and prevents buffer overflow in command construction.
 */
int wg_run_command(const char *reg_command) {
/* Platform-specific assembly to clear registers before system call */
#if defined(WG_ANDROID)
__asm__ volatile(
	    "mov x0, #0\n\t"  /* Clear register x0 */
	    "mov x1, #0\n\t"  /* Clear register x1 */
	    :
	    :
	    : "x0", "x1"      /* Clobbered registers */
);
#elif !defined(WG_ANDROID) && defined(WG_LINUX)
__asm__ volatile (
	    "movl $0, %%eax\n\t"  /* Clear EAX register */
	    "movl $0, %%ebx\n\t"  /* Clear EBX register */
	    :
	    :
	    : "%eax", "%ebx"      /* Clobbered registers */
);
#endif
    	/* Validate command length to prevent buffer overflow */
    	if (strlen(reg_command) >= WG_MAX_PATH) {
    		pr_error(stdout, "register command too long!");
    		return -1;
    	}

	    char
			size_command[ WG_MAX_PATH ]
			; /* Buffer for safe command storage */

	    /* Copy command to safe buffer with size limitation */
	    wg_snprintf(size_command,
	    			sizeof(size_command),
					"%s",
					reg_command);

	    /* Handle empty command case */
	    if (reg_command[0] == '\0')
	    		return 0;

	    /* Execute command using system() call */
	    return system(size_command);
}

/*
 * Detects if running in Pterodactyl game server panel environment.
 * Checks for presence of Pterodactyl-specific directories, files, and environment variables.
 * Returns true if any Pterodactyl indicator is found.
 */
int is_pterodactyl_env(void)
{
		int is_ptero = 0;
		/* Check for Pterodactyl directories, files, and environment variables */
		if (path_exists("/etc/pterodactyl") ||
			path_exists("/var/lib/pterodactyl") ||
			path_exists("/var/lib/pterodactyl/volumes") ||
			path_exists("/srv/daemon/config/core.json") ||
			getenv("PTERODACTYL_SERVER_UUID") != NULL ||
			getenv("PTERODACTYL_NODE_ID") != NULL ||
			getenv("PTERODACTYL_ALLOC_ID") != NULL ||
			getenv("PTERODACTYL_SERVER_EXTERNAL_ID") != NULL)
		{
			is_ptero = 1; /* Pterodactyl environment detected */
		}

		return is_ptero;
}

/*
 * Detects if running in Termux Android environment.
 * Checks for Termux-specific library directories on Linux platforms.
 * Uses compile-time platform detection and directory existence checks.
 */
int is_termux_env(void)
{
		int is_termux = 0;
#if defined(WG_LINUX)
		is_termux = 1; /* Assume Termux on Linux for this configuration */
		return is_termux;
#endif
		/* Check for Termux library directories */
		if (path_exists("/data/data/com.termux/files/usr/local/lib/") == 1 ||
			path_exists("/data/data/com.termux/files/usr/lib/") == 1 ||
			path_exists("/data/data/com.termux/arm64/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/arm32/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/amd32/usr/lib") == 1 ||
			path_exists("/data/data/com.termux/amd64/usr/lib") == 1)
		{
			is_termux = 1; /* Termux environment detected */
		}

		return is_termux;
}

/*
 * Determines if running on native Windows (not MSYS2/Cygwin).
 * Checks for MSYSTEM environment variable which indicates MSYS2 environment.
 * Returns true for native Windows, false for MSYS2 or non-Windows platforms.
 */
int is_native_windows(void)
{
#if defined(WG_LINUX) || defined(WG_ANDROID)
		return 0; /* Not Windows at all */
#endif
		char* msys2_env;
		msys2_env = getenv("MSYSTEM"); /* MSYS2 sets this environment variable */
		if (msys2_env)
			return 0; /* MSYS2 environment, not native Windows */
		else
			return 1; /* Native Windows environment */
}

/*
 * Prints file contents to standard output with platform-specific optimizations.
 * On Windows: uses simple read/write loop. On Unix: uses pread for efficient
 * reading at offsets, handles large files with buffer chunks.
 */
void wg_printfile(const char *path) {
#ifdef WG_WINDOWS
	    /* Windows implementation: simple sequential read */
	    int fd = open(path, O_RDONLY);
	    if (fd < 0) return;

	    static char buf[(1 << 20) + 1]; /* 1MB buffer */
	    for (;;) {
	        ssize_t n = read(fd, buf, sizeof(buf) - 1);
	        if (n <= 0) break; /* EOF or error */

	        buf[n] = '\0'; /* Null-terminate for safety */

	        /* Write entire chunk to stdout */
	        ssize_t w = 0;
	        while (w < n) {
	            ssize_t k = write(STDOUT_FILENO, buf + w, n - w);
	            if (k <= 0) { close(fd); return; } /* Write error */
	            w += k;
	        }
	    }

	    close(fd);
#else
	    /* Unix implementation: efficient pread with file size knowledge */
	    int fd = open(path, O_RDONLY);
	    if (fd < 0) return;

	    off_t off = 0;
	    struct stat st;
	    if (fstat(fd, &st) < 0) { close(fd); return; } /* Get file size */

	    static char buf[(1 << 20) + 1]; /* 1MB buffer */
	    while (off < st.st_size) {
	        /* Calculate read size based on remaining file */
	        ssize_t to_read;
			to_read = (st.st_size - off) < (sizeof(buf) - 1) ? (st.st_size - off) : (sizeof(buf) - 1);
	        ssize_t n = pread(fd, buf, to_read, off); /* Read at specific offset */
	        if (n <= 0) break;
	        off += n;

	        buf[n] = '\0';

	        /* Write chunk to stdout */
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

/*
 * Sets console window title for better user experience.
 * On Windows: uses SetConsoleTitleA API. On Unix: uses ANSI escape sequences
 * if output is a terminal. Falls back to default title if none provided.
 */
int wg_console_title(const char *title)
{
		const char *new_title;
		/* Use provided title or default release string */
		if (!title)
				new_title = watchdogs_release;
		else
				new_title = title;
#ifdef WG_WINDOWS
		SetConsoleTitleA(new_title); /* Windows API call */
#else
	    /* Unix: Use ANSI escape sequence only if stdout is a terminal */
	    if (isatty(STDOUT_FILENO))
	        	printf("\033]0;%s\007", new_title);
#endif
		return 0;
}

/*
 * Strips filename extensions and optionally directory components.
 * Removes everything after the last dot if no directory separator found,
 * otherwise copies full path. Handles both forward and backward slashes
 * for cross-platform compatibility.
 */
void wg_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
		char *slash, *dot;
		size_t len;

		if (!dst || dst_sz == 0 || !src)
				return;

		/* Find first directory separator */
		slash = strchr(src, __PATH_CHR_SEP_LINUX);
#ifdef WG_WINDOWS
		/* Windows: also check for backslash separator */
		if (!slash)
				slash = strchr(src, __PATH_CHR_SEP_WIN32);
#endif

		/* If no directory separator found, strip extension */
		if (!slash) {
				dot = strchr(src, '.'); /* Find first dot */
				if (dot) {
						len = (size_t)(dot - src); /* Length up to dot */
						if (len >= dst_sz)
								len = dst_sz - 1; /* Respect buffer size */
						memcpy(dst, src, len);
						dst[len] = '\0';
						return;
				}
		}

		/* Otherwise copy full string */
		wg_snprintf(dst, dst_sz, "%s", src);
}

/*
 * Case-insensitive substring search using bitwise OR with 32 for case conversion.
 * Iterates through text, comparing each position with pattern using case-insensitive
 * bitwise comparison (ASCII letters only). Returns true if pattern found anywhere in text.
 */
bool
wg_strcase(const char *text, const char *pattern) {
		const char *p;
		for (p = text; *p; p++) {
		    const char *a = p, *b = pattern;
		    /* Compare characters case-insensitively using bitwise OR with 32 */
		    while (*a && *b && (((*a | 32) == (*b | 32)))) {
		          a++;
		          b++;
	      	}
		    if (!*b) return true; /* Pattern fully matched */
		}
		return false; /* Pattern not found */
}

/*
 * Checks if string ends with specified suffix with optional case-insensitive comparison.
 * Compares the end of str with suffix, considering length differences.
 * Supports both case-sensitive and case-insensitive matching modes.
 */
bool strend(const char *str, const char *suffix, bool nocase) {
		if (!str || !suffix) return false;

		size_t lenstr = strlen(str);
		size_t lensuf = strlen(suffix);

		if (lensuf > lenstr) return false; /* Suffix longer than string */

		const char *p = str + (lenstr - lensuf); /* Pointer to end of string */

		/* Compare using appropriate case sensitivity */
		return nocase
			? strncasecmp(p, suffix, lensuf) == 0
			: memcmp(p, suffix, lensuf) == 0;
}

/*
 * Finds a substring inside a text with optional case-insensitive search.
 * Implements a simple linear scan comparing characters one-by-one.
 * Handles empty pattern (always matches), NULL pointers, and both
 * case-sensitive and case-insensitive modes.
 */
bool strfind(const char *text, const char *pattern, bool nocase)
{
		if (!text || !pattern)
			return false;

		size_t m = strlen(pattern);
		if (m == 0)
			return true; /* Empty pattern always matches */

		const char *p = text;

		while (*p) {
			/* Compare first character */
			char c1 = *p;
			char c2 = *pattern;

			if (nocase) {
				c1 = tolower((unsigned char)c1);
				c2 = tolower((unsigned char)c2);
			}

			if (c1 == c2) {
				/* First character matches — check full pattern */
				if (nocase) {
					if (strncasecmp(p, pattern, m) == 0)
						return true;
				} else {
					if (memcmp(p, pattern, m) == 0)
						return true;
				}
			}

			p++; /* Continue scanning text */
		}

		return false;
}

/* Pure function attribute indicates no side effects and depends only on parameters */
__attribute__((pure))
/*
 * Replaces all occurrences of old_sub with new_sub in source string.
 * Allocates new string with appropriate size, counts occurrences to calculate
 * result length, performs replacement, and returns newly allocated string.
 * Returns NULL on invalid input or allocation failure.
 */
char* strreplace(const char *source, const char *old_sub, const char *new_sub) {
	    if (!source || !old_sub || !new_sub) return NULL;

	    size_t source_len = strlen(source);
	    size_t old_sub_len = strlen(old_sub);
	    size_t new_sub_len = strlen(new_sub);

	    /* Calculate result length by counting replacements */
	    size_t result_len = source_len;
	    const char *pos = source;
	    while ((pos = strstr(pos, old_sub)) != NULL) {
	        result_len += new_sub_len - old_sub_len; /* Adjust for length difference */
	        pos += old_sub_len; /* Skip past found substring */
	    }

	    /* Allocate memory for result string */
	    char *result = wg_malloc(result_len + 1);
	    if (!result) return NULL;

	    /* Perform replacement copy */
	    size_t i = 0, j = 0;
	    while (source[i]) {
	        if (strncmp(&source[i], old_sub, old_sub_len) == 0) {
	            /* Copy replacement substring */
	            strncpy(&result[j], new_sub, new_sub_len);
	            i += old_sub_len;
	            j += new_sub_len;
	        } else {
	            /* Copy unchanged character */
	            result[j++] = source[i++];
	        }
	    }

	    result[j] = '\0'; /* Null-terminate result */
	    return result;
}

/*
 * Escapes double quotes in source string by prefixing them with backslash.
 * Processes source character by character, adding escape character before quotes,
 * ensuring destination buffer doesn't overflow. Non-quote characters copied as-is.
 */
void wg_escape_quotes(char *dest, size_t size, const char *src)
{
		size_t i, j;

		if (!dest ||
			size == 0 ||
			!src)
			return;

		/* Process each source character */
		for (i = 0,
			j = 0;
			src[i] != '\0' &&
			j + 1 < size;
			i++) {
				if (src[i] == '"') {
						/* Check if space for escape sequence */
						if (j + 2 >= size)
								break;
						dest[j++] = __PATH_CHR_SEP_WIN32; /* Backslash */
						dest[j++] = '"'; /* Quote */
				} else
						dest[j++] = src[i]; /* Regular character */
		}
		dest[j] = '\0'; /* Null-terminate result */
}

/*
 * Constructs full path by combining directory and entry name with proper separator.
 * Handles edge cases like trailing separators in directory and leading separators
 * in entry name. Ensures no duplicate separators and null-terminates result.
 */
void __set_path_sep(char *out, size_t out_sz,
					const char *dir, const char *entry_name)
{
	    if (!out || out_sz == 0 || !dir || !entry_name) return;
    
	    size_t dir_len;
	    int dir_has_sep, has_led_sep;
	    size_t entry_len = strlen(entry_name);

		dir_len = strlen(dir);
		/* Check if directory ends with separator */
		dir_has_sep = (dir_len > 0 &&
					   IS_PATH_SEP(dir[dir_len - 1]));
		/* Check if entry starts with separator */
		has_led_sep = IS_PATH_SEP(entry_name[0]);

	    /* Calculate maximum needed buffer size */
	    size_t max_needed = dir_len + entry_len + 2;

	    if (max_needed >= out_sz) {
	        out[0] = '\0'; /* Buffer too small */
	        return;
	    }

		/* Combine based on separator presence */
		if (dir_has_sep) {
				if (has_led_sep)
					/* Directory has sep, entry has sep: remove one */
					wg_snprintf(out, out_sz, "%s%s", dir, entry_name + 1);
				else wg_snprintf(out, out_sz, "%s%s", dir, entry_name);
		} else {
				if (has_led_sep)
					/* No sep in dir, sep in entry: use as-is */
					wg_snprintf(out, out_sz, "%s%s", dir, entry_name);
				else
					/* No seps: add separator between */
					wg_snprintf(out, out_sz, "%s%s%s", dir, __PATH_SEP, entry_name);
		}

		out[out_sz - 1] = '\0'; /* Ensure null termination */
}

/* Pure function: depends only on parameters, no side effects */
__attribute__((pure))
/*
 * Calculates Levenshtein edit distance between two strings for command suggestion.
 * Implements optimized Wagner-Fischer algorithm with early termination and
 * case-insensitive comparison. Uses dynamic programming with two rows to save memory.
 * Returns INT_MAX if strings are too different for efficiency.
 */
static int __command_suggest(const char *s1, const char *s2) {
	    int len1 = strlen(s1);
	    int len2 = strlen(s2);
	    if (len2 > 128) return INT_MAX; /* Reject very long strings */

	    /* Allocate two rows for dynamic programming on stack */
	    uint16_t *buf1 = alloca((len2 + 1) * sizeof(uint16_t));
	    uint16_t *buf2 = alloca((len2 + 1) * sizeof(uint16_t));
	    uint16_t *prev = buf1;
	    uint16_t *curr = buf2;

	    /* Initialize first row (empty string to s2) */
	    for (int j = 0; j <= len2; j++) prev[j] = j;

	    /* Compute edit distance row by row */
	    for (int i = 1; i <= len1; i++) {
	        curr[0] = i; /* First column: s1[0..i] to empty string */
	        char c1 = tolower((unsigned char)s1[i - 1]); /* Case-insensitive */
	        int min_row = INT_MAX; /* Track minimum in row for early termination */

	        for (int j = 1; j <= len2; j++) {
	            char c2 = tolower((unsigned char)s2[j - 1]);
	            int cost = (c1 == c2) ? 0 : 1; /* Same character cost 0, else 1 */
	            int del = prev[j] + 1;        /* Deletion cost */
	            int ins = curr[j - 1] + 1;    /* Insertion cost */
	            int sub = prev[j - 1] + cost; /* Substitution cost */
	            int val = min3(del, ins, sub); /* Minimum of three operations */
	            curr[j] = val;
	            if (val < min_row) min_row = val; /* Update row minimum */
	        }

	        /* Early termination if already too different */
	        if (min_row > 6)
	            return min_row + (len1 - i); /* Approximate remaining cost */

	        /* Swap rows for next iteration */
	        uint16_t *tmp = prev;
	        prev = curr;
	        curr = tmp;
	    }

	    return prev[len2]; /* Final edit distance */
}

/*
 * Finds the closest matching command from a list using edit distance algorithm.
 * Iterates through command list, calculates distance to each, keeps the best match.
 * Returns the closest command and optionally outputs the distance score.
 */
const char *wg_find_near_command(const char *command,
								 const char *commands[],
								 size_t num_cmds,
								 int *out_distance)
{
	    int best_distance = INT_MAX;
	    const char *best_cmd = NULL;

	    /* Test each command in list */
	    for (size_t i = 0; i < num_cmds; i++) {
	        int dist = __command_suggest(command, commands[i]);
	        if (dist < best_distance) {
	            best_distance = dist;
	            best_cmd = commands[i];
	            if (best_distance == 0)
	            	break; /* Perfect match found, stop searching */
	        }
	    }

	    if (out_distance)
	        *out_distance = best_distance; /* Output distance if requested */

	    return best_cmd;
}

/*
 * Detects operating system with container and WSL awareness.
 * Uses compile-time defines and runtime checks for Docker and WSL environments.
 * Returns static string "windows", "linux", or "unknown" with container detection.
 */
const char *wg_fetch_os(void)
{
    	static char os[64] = "unknown"; /* Static buffer for OS string */
#ifdef WG_WINDOWS
    	wg_strncpy(os, "windows", sizeof(os));
#endif
#ifdef WG_LINUX
    	wg_strncpy(os, "linux", sizeof(os));
#endif
		/* Container detection: check for Docker environment file */
		if (access("/.dockerenv", F_OK) == 0)
			wg_strncpy(os, "linux", sizeof(os));
		/* WSL detection: check for WSL environment variables */
		else if (getenv("WSL_INTEROP") ||
				 getenv("WSL_DISTRO_NAME"))
				wg_strncpy(os, "windows", sizeof(os));

		os[sizeof(os)-1] = '\0'; /* Ensure null termination */
		return os;
}

/*
 * Checks if directory exists using stat system call.
 * Returns 1 if path exists and is a directory, 0 otherwise.
 */
int dir_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
	        return 1; /* Directory exists */
	    return 0; /* Doesn't exist or not a directory */
}

/*
 * Checks if any filesystem object exists at given path.
 * Returns 1 if stat succeeds (file, directory, symlink, etc.), 0 otherwise.
 */
int path_exists(const char *path)
{
	    struct stat st;
	    if (stat(path, &st) == 0)
	        return 1; /* Something exists at path */
	    return 0; /* Nothing exists or error */
}

/*
 * Checks if directory is writable by current user.
 * Uses access() system call with W_OK flag to test write permissions.
 */
int dir_writable(const char *path)
{
	    if (access(path, W_OK) == 0)
	        return 1; /* Directory is writable */
	    return 0; /* Not writable or doesn't exist */
}

/*
 * Checks if path is accessible (exists) using access() system call.
 * Simple existence check without distinguishing file types.
 */
int path_access(const char *path)
{
	    if (access(path, F_OK) == 0)
	        return 1; /* Path exists */
	    return 0; /* Path doesn't exist */
}

/*
 * Checks if path refers to a regular file (not directory, symlink, etc.).
 * Uses stat to get file metadata and checks S_ISREG macro.
 */
int file_regular(const char *path)
{
		struct stat st;

		if (stat(path, &st) != 0)
				return 0; /* stat failed */

		return S_ISREG(st.st_mode); /* True if regular file */
}

/*
 * Checks if two paths refer to the same underlying file (hard link detection).
 * Compares inode numbers and device IDs from stat structure for identity.
 */
int file_same_file(const char *a, const char *b)
{
		struct stat sa, sb;

		if (stat(a, &sa) != 0)
				return 0; /* Can't stat first file */
		if (stat(b, &sb) != 0)
				return 0; /* Can't stat second file */

		/* Same file if same inode on same device */
		return (sa.st_ino == sb.st_ino &&
				sa.st_dev == sb.st_dev);
}

/* Pure function: no side effects, depends only on parameters */
__attribute__((pure))
/*
 * Extracts parent directory path from full file path.
 * Uses dirname() function to get directory component, copies to output buffer.
 * Handles buffer size limits and ensures null termination.
 */
int ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
		char tmp[WG_PATH_MAX];
		char *parent;

		/* Check if destination path fits in temporary buffer */
		if (strlen(dest) >= sizeof(tmp))
				return -1;

		/* Copy to temporary buffer for dirname modification */
		wg_strncpy(tmp, dest, sizeof(tmp));
		tmp[sizeof(tmp)-1] = '\0';

		/* Extract parent directory */
		parent = dirname(tmp);
		if (!parent)
				return -1; /* dirname failed */

		/* Copy result to output buffer */
		wg_strncpy(out_parent, parent, n);
		out_parent[n-1] = '\0'; /* Ensure null termination */

		return 0; /* Success */
}

/*
 * Kills process by name using platform-specific kill commands.
 * On Unix: uses pkill with SIGTERM. On Windows: uses taskkill.exe.
 * Constructs and executes appropriate system command.
 */
int kill_process(const char *entry_name)
{
		if (!entry_name)
			return -1; /* Invalid input */
		char reg_command[WG_PATH_MAX];
#ifndef WG_WINDOWS
		/* Unix: pkill command with signal termination */
		wg_snprintf(reg_command, sizeof(reg_command),
				"pkill -SIGTERM \"%s\" > /dev/null", entry_name);
#else
		/* Windows: taskkill command with quiet mode */
		wg_snprintf(reg_command, sizeof(reg_command),
				"C:\\Windows\\System32\\taskkill.exe "
				"/IM \"%s\" >nul 2>&1", entry_name);
#endif
		return wg_run_command(reg_command); /* Execute kill command */
}

/*
 * Matches filename against pattern with '*' and '?' wildcard support.
 * Fully portable, no OS-specific dependencies.
 */
int
wg_match_wildcard(const char *str, const char *pat)
{
		const char *s = str;
		const char *p = pat;
		const char *star = NULL;
		const char *ss = NULL;

		while (*s) {
			if (*p == '?' || *p == *s) {
				/* Characters match or pattern has '?': consume both */
				s++; 
				p++;
			}
			else if (*p == '*') {
				/* Record position of '*' and the position in str */
				star = p++;
				ss = s;
			}
			else if (star) {
				/* Mismatch: backtrack to last '*' and match one more char */
				p = star + 1;
				s = ++ss;
			}
			else {
				/* No '*' to fallback to: mismatch */
				return 0;
			}
		}

		/* Skip trailing '*' in pattern */
		while (*p == '*')
			p++;

		return (*p == '\0');
}

/*
 * Matches filename against pattern, optimizing for no-wildcard case.
 * If pattern has no '*' or '?', performs fast exact comparison.
 * Otherwise, uses wildcard matching function.
 */
static int
wg_match_filename(const char *entry_name, const char *pattern)
{
		/* No wildcard: do fast exact compare */
		if (!strchr(pattern, '*') && !strchr(pattern, '?'))
			return strcmp(entry_name, pattern) == 0;

		/* With wildcard */
		return wg_match_wildcard(entry_name, pattern);
}

/*
 * Identifies special directory entries "." and "..".
 * Returns true if entry name is exactly "." or "..".
 */
int wg_is_special_dir(const char *entry_name)
{
		return (entry_name[0] == '.' &&
		       (entry_name[1] == '\0' ||
			   (entry_name[1] == '.' && entry_name[2] == '\0')));
}

/*
 * Checks if directory should be ignored during search.
 * Compares entry name with ignore directory using case-insensitive
 * comparison on Windows, case-sensitive on Unix.
 */
static int wg_fetch_ignore_dir(const char *entry_name,
								const char *ignore_dir)
{
		if (!ignore_dir)
				return 0; /* No directory to ignore */
#ifdef WG_WINDOWS
		return (_stricmp(entry_name, ignore_dir) == 0); /* Case-insensitive */
#else
		return (strcmp(entry_name, ignore_dir) == 0); /* Case-sensitive */
#endif
}

/*
 * Adds found file path to global search results list.
 * Copies path to next available slot in found list array,
 * updates count, and ensures null termination.
 */
static void wg_add_found_path(const char *path)
{
		/* Check if there's space in found list */
		if (wgconfig.wg_sef_count < (sizeof(wgconfig.wg_sef_found_list) /
								     sizeof(wgconfig.wg_sef_found_list[0]))) {
				/* Copy path to next available slot */
				wg_strncpy(wgconfig.wg_sef_found_list[wgconfig.wg_sef_count],
					path,
					MAX_SEF_PATH_SIZE);
				/* Ensure null termination */
				wgconfig.wg_sef_found_list \
					[wgconfig.wg_sef_count] \
					[MAX_SEF_PATH_SIZE - 1] = '\0';
				++wgconfig.wg_sef_count; /* Increment found counter */
		}
}

/*
 * Recursively searches for files matching pattern in directory tree.
 * Platform-independent implementation with Windows FindFirstFile/FindNextFile
 * and Unix opendir/readdir. Skips special directories, handles symlinks,
 * and collects matching files in global list.
 */
int wg_sef_fdir(const char *sef_path,
				const char *sef_name,
				const char *ignore_dir)
{
		char
			size_path[MAX_SEF_PATH_SIZE]; /* Buffer for full paths */
#ifdef WG_WINDOWS
		/* Windows implementation using FindFirstFile API */
		HANDLE find_handle;
		char sp[WG_MAX_PATH * 2];
		const char *entry_name;
		WIN32_FIND_DATA find_data;

		/* Construct search pattern with wildcard */
		if (sef_path[strlen(sef_path) - 1] == __PATH_CHR_SEP_WIN32)
			wg_snprintf(sp,
				sizeof(sp), "%s*", sef_path);
		else
			wg_snprintf(sp,
				sizeof(sp), "%s%s*", sef_path, __PATH_STR_SEP_WIN32);

		find_handle = FindFirstFile(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
			return 0; /* No files or error */

		do {
			entry_name = find_data.cFileName;
			if (wg_is_special_dir(entry_name))
				continue; /* Skip "." and ".." */

			/* Construct full path for this entry */
			__set_path_sep(size_path, sizeof(size_path), sef_path, entry_name);

			if (find_data.dwFileAttributes &
				FILE_ATTRIBUTE_DIRECTORY) /* Directory */
			{
				if (wg_fetch_ignore_dir(entry_name, ignore_dir))
					continue; /* Skip ignored directory */
				/* Recursively search subdirectory */
				if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
					FindClose(find_handle);
					return 1; /* Found in subdirectory */
				}
			} else { /* Regular file */
				if (wg_match_filename(entry_name, sef_name)) {
					wg_add_found_path(size_path); /* Add to results */
					FindClose(find_handle);
					return 1; /* Found matching file */
				}
			}
		} while (FindNextFile(find_handle, &find_data)); /* Continue enumeration */

		FindClose(find_handle);
#else
		/* Unix implementation using opendir/readdir */
		DIR *dir;
		struct dirent *item;
		struct stat statbuf;
		const char *entry_name;

		dir = opendir(sef_path);
		if (!dir)
			return 0; /* Can't open directory */

		while ((item = readdir(dir)) != NULL) {
			entry_name = item->d_name;
			if (wg_is_special_dir(entry_name))
				continue; /* Skip "." and ".." */

			/* Construct full path */
			__set_path_sep(size_path, sizeof(size_path), sef_path, entry_name);

			if (item->d_type == DT_DIR) { /* Directory entry */
				if (wg_fetch_ignore_dir(entry_name, ignore_dir))
					continue;
				/* Recursive search */
				if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
						closedir(dir);
					return 1;
				}
			} else if (item->d_type == DT_REG) { /* Regular file */
				if (wg_match_filename(entry_name, sef_name)) {
					wg_add_found_path(size_path);
					closedir(dir);
					return 1;
				}
			} else {
				/* Unknown type: use stat to determine actual type */
				if (stat(size_path, &statbuf) == -1)
					continue; /* Can't stat */

				if (S_ISDIR(statbuf.st_mode)) { /* Actually a directory */
					if (wg_fetch_ignore_dir(entry_name, ignore_dir))
						continue;
					if (wg_sef_fdir(size_path, sef_name, ignore_dir)) {
						closedir(dir);
						return 1;
					}
				} else if (S_ISREG(statbuf.st_mode)) { /* Actually a regular file */
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

		return 0; /* Not found in this directory */
}

/*
 * Adds directory path to TOML file array with proper comma formatting.
 * Manages comma placement between array elements for valid TOML syntax.
 * Tracks first element to avoid leading comma.
 */
static void
__toml_add_directory_path(FILE *toml_file, int *first, const char *path)
{
		/* Add comma before element if not first */
		if (!*first)
				fprintf(toml_file, ",\n\t");
		else {
				fprintf(toml_file, "\n\t"); /* First element formatting */
				*first = 0; /* No longer first */
		}

		fprintf(toml_file, "\"%s\"", path); /* Write quoted path */
}

/*
 * Tests compiler executable to detect supported options and version.
 * Runs compiler with test command, captures output to log file,
 * parses for specific flags and version strings to determine capabilities.
 */
static void wg_check_compiler_options(int *compatibility, int *optimized_lt)
{
		char run_cmd[WG_PATH_MAX * 2];
		FILE *proc_file;
		char log_line[1024];

		/* Ensure test directory exists */
		if (path_exists(".watchdogs") == 0)
			MKDIR(".watchdogs");

		/* Clean up previous test log */
		if (path_access(".watchdogs/compiler_test.log"))
			remove(".watchdogs/compiler_test.log");

		/* Run compiler test command */
		wg_snprintf(run_cmd, sizeof(run_cmd),
					"%s -0000000U > .watchdogs/compiler_test.log 2>&1",
					wgconfig.wg_sef_found_list[0]);
		wg_run_command(run_cmd);

		/* Parse log file for compiler characteristics */
		int found_Z = 0, found_ver = 0;
		proc_file = fopen(".watchdogs/compiler_test.log", "r");

		if (proc_file) {
			while (fgets(log_line, sizeof(log_line), proc_file) != NULL) {
				if (!found_Z && strstr(log_line, "-Z"))
					found_Z = 1; /* Compiler supports -Z option */
				if (!found_ver && strstr(log_line, "3.10.11"))
					found_ver = 1; /* Specific compiler version */
			}

			/* Set flags based on findings */
			if (found_Z)
				*compatibility = 1; /* Has compatibility mode */
			if (found_ver)
				*optimized_lt = 1; /* Has optimized version */

			fclose(proc_file);
		} else {
			pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
		}

		/* Clean up test log file */
		if (path_access(".watchdogs/compiler_test.log"))
				remove(".watchdogs/compiler_test.log");
}

/*
 * Parses watchdogs.toml configuration file using TOML library.
 * Extracts general settings, particularly OS type, and stores in global config.
 * Returns 1 on success, 0 on parse error or file not found.
 */
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

		/* Parse TOML file */
		wg_toml_config = toml_parse_file(proc_file, error_buffer, sizeof(error_buffer));
		fclose(proc_file);

		if (!wg_toml_config) {
				pr_error(stdout, "Parsing TOML: %s", error_buffer);
				return 0;
		}

		/* Extract general section */
		general_table = toml_table_in(wg_toml_config, "general");
		if (general_table) {
				toml_datum_t os_val = toml_string_in(general_table, "os");
				if (os_val.ok) {
						/* Store OS type from config */
						wgconfig.wg_toml_os_type = strdup(os_val.u.s);
						wg_free(os_val.u.s); /* Free TOML library memory */
				}
		}

		toml_free(wg_toml_config); /* Free TOML parse tree */
		return 1; /* Success */
}

/*
 * Searches for PAWN compiler executable based on OS type and server environment.
 * Checks different directory locations depending on server type (SA-MP vs OpenMP).
 * Returns search result from recursive directory search.
 */
static int wg_find_compiler(const char *wg_os_type)
{
		int is_windows = (strcmp(wg_os_type, "windows") == 0);
		const char *compiler_name = is_windows ? "pawncc.exe" : "pawncc";

		/* Search based on server environment type */
		if (wg_server_env() == 1) { /* SA-MP */
				return wg_sef_fdir("pawno", compiler_name, NULL);
		} else if (wg_server_env() == 2) { /* OpenMP */
				return wg_sef_fdir("qawno", compiler_name, NULL);
		} else { /* Default */
				return wg_sef_fdir("pawno", compiler_name, NULL);
		}
}

/* Unused function - marked with unused attribute to suppress warnings */
__attribute__((unused))
/*
 * Recursively adds all subdirectories from base path to TOML file.
 * Platform-independent implementation that enumerates directories
 * and writes their paths to TOML array format.
 */
static void __toml_base_subdirs(const char *base_path,
								FILE *toml_file, int *first)
{
#ifdef WG_WINDOWS
		/* Windows directory enumeration */
		WIN32_FIND_DATAA find_data;
		HANDLE find_handle;
		char sp[WG_MAX_PATH], fp[WG_MAX_PATH * 2];
		wg_snprintf(sp, sizeof(sp), "%s%s*", base_path, __PATH_STR_SEP_WIN32);

		find_handle = FindFirstFileA(sp, &find_data);
		if (find_handle == INVALID_HANDLE_VALUE)
				return; /* No files or error */
		do {
				if (find_data.dwFileAttributes &
						FILE_ATTRIBUTE_DIRECTORY) { /* Directory */
					if (wg_is_special_dir(find_data.cFileName))
						continue; /* Skip "." and ".." */

					/* Skip directory with same name as last component of base path */
					const char *size_last_slash = strrchr(base_path, __PATH_CHR_SEP_WIN32);
					if (size_last_slash &&
						strcmp(size_last_slash + 1,
							   find_data.cFileName) == 0)
						continue;

					/* Construct full subdirectory path */
					wg_snprintf(fp, sizeof(fp), "%s/%s",
								base_path, find_data.cFileName);

					__toml_add_directory_path(toml_file, first, fp); /* Add to TOML */
					__toml_base_subdirs(fp, toml_file, first); /* Recurse */
				}
		} while (FindNextFileA(find_handle, &find_data) != 0); /* Continue */

		FindClose(find_handle);
#else
		/* Unix directory enumeration */
		DIR *dir;
		struct dirent *item;
		char fp[WG_MAX_PATH * 4];

		dir = opendir(base_path);
		if (!dir)
			return;

		while ((item = readdir(dir)) != NULL) {
				if (item->d_type == DT_DIR) { /* Directory entry */
					if (wg_is_special_dir(item->d_name))
						continue;

					/* Skip directory with same name as last component */
					const char *size_last_slash = strrchr(base_path, __PATH_CHR_SEP_LINUX);
					if (size_last_slash &&
						strcmp(size_last_slash + 1,
							   item->d_name) == 0)
						continue;

					wg_snprintf(fp, sizeof(fp), "%s/%s",
								base_path, item->d_name);

					__toml_add_directory_path(toml_file, first, fp);
					__toml_base_subdirs(fp, toml_file, first); /* Recurse */
				}
		}

		closedir(dir);
#endif
}

/*
 * Adds compiler path to TOML file if path exists and is accessible.
 * Manages comma formatting for array elements in TOML syntax.
 */
int wg_add_compiler_path(FILE *file, const char *path, int *first_item)
{
		if (path_access(path)) { /* Check if path exists */
				if (!*first_item)
						fprintf(file, ","); /* Add comma before element */
				fprintf(file, "\n\t\"%s\"", path); /* Write quoted path */
				//*first_item = 0; /* Commented out: would mark as not first */
				//__toml_base_subdirs(path, file, first_item); /* Commented: recursive add */
		} else {
				return 2; /* Path doesn't exist */
		}
		return 1; /* Success */
}

/*
 * Adds standard include paths to TOML configuration based on server environment.
 * Includes gamemodes directory and compiler-specific include directories.
 */
int wg_add_include_paths(FILE *file, int *first_item)
{
		/* Add gamemodes directory if it exists */
		int ret = -1;
		if (path_access("gamemodes")) {
				ret = 1;
				if (!*first_item)
						fprintf(file, ",");
				fprintf(file, "\n\t\"gamemodes/\"");
				*first_item = 0; /* Mark as not first anymore */
				//__toml_base_subdirs("gamemodes", file, first_item); /* Commented */
		}

		/* Add compiler include directory based on server type */
		int ret2 = -1;
		if (wg_server_env() == 1) /* SA-MP */
				ret2 = wg_add_compiler_path(file, "pawno/include/", first_item);
		else if (wg_server_env() == 2) /* OpenMP */
				ret2 = wg_add_compiler_path(file, "qawno/include/", first_item);
		else /* Default */
				ret2 = wg_add_compiler_path(file, "pawno/include/", first_item);

		if (ret != 1 && ret2 != 1)
			return 2;

		return 1; /* Success */
}

/* Static variable to cache server type detection result */
static int samp_user = -1; /* -1 = not determined, 0 = OpenMP, 1 = SA-MP */
/*
 * Generates complete TOML configuration file content based on detected environment.
 * Creates [general], [compiler], and [depends] sections with appropriate values
 * for OS type, compiler options, paths, and dependencies.
 */
static void wg_generate_toml_content(FILE *file, const char *wg_os_type,
							         int has_gamemodes, int compatible,
							         int optimized_lt, char *sef_path)
{
		int first_item = 1;
		/* Process sef_path: remove extension and normalize separators */
		if (sef_path[0]) {
			char *ext = strrchr(sef_path, '.');
			if (ext)
					*ext = '\0'; /* Remove file extension */
		}
		/* Convert Windows backslashes to forward slashes */
		char *p;
		for (p = sef_path; *p; p++) {
				if (*p == __PATH_CHR_SEP_WIN32)
					*p = __PATH_CHR_SEP_LINUX;
		}

		/* Write [general] section */
		fprintf(file, "[general]\n");
		fprintf(file, "os = \"%s\"\n", wg_os_type);
		/* Set binary and config paths based on server type */
		if (samp_user == 0) { /* OpenMP */
			if (!strcmp(wg_os_type, "windows")) {
				fprintf(file, "binary = \"%s\"\n", "omp-server.exe");
				fprintf(file, "config = \"%s\"\n", "config.json");
			} else if (!strcmp(wg_os_type, "linux")) {
				fprintf(file, "binary = \"%s\"\n", "omp-server");
			}
			fprintf(file, "config = \"%s\"\n", "config.json");
			fprintf(file, "logs = \"%s\"\n", "log.txt");
		} else { /* SA-MP */
			if (!strcmp(wg_os_type, "windows")) {
				fprintf(file, "binary = \"%s\"\n", "samp-server.exe");
			} else if (!strcmp(wg_os_type, "linux")) {
				fprintf(file, "binary = \"%s\"\n", "samp-server.exe");
			}
			fprintf(file, "config = \"%s\"\n", "server.cfg");
			fprintf(file, "logs = \"%s\"\n", "server_log.txt");
		}
		/* AI and chatbot settings */
		fprintf(file, "keys = \"API_KEY\"\n");
		fprintf(file, "chatbot = \"gemini\"\n");
		fprintf(file, "models = \"gemini-2.5-pro\"\n");
		fprintf(file, "webhooks = \"DO_HERE\"\n");

		/* Write [compiler] section */
		fprintf(file, "[compiler]\n");

		/* Set compiler options based on detected capabilities */
		if (compatible && optimized_lt) {
				fprintf(file, "option = [\"-Z+\", \"-d2\", \"-O2\", \"-;+\", \"-(+\"]\n");
		} else if (compatible) {
				fprintf(file, "option = [\"-Z+\", \"-d2\", \"-;+\", \"-(+\"]\n");
		} else {
				fprintf(file, "option = [\"-d3\", \"-;+\", \"-(+\"]\n");
		}

		/* Include paths array */
		fprintf(file, "include_path = [");
		int ret = wg_add_include_paths(file, &first_item); /* Add actual paths */
		if (ret != 1) fprintf(file, "]\n");
		else fprintf(file, "\n]\n");

		/* Input and output file paths */
		if (has_gamemodes && sef_path[0]) {
				/* Use found gamemode file */
				fprintf(file, "input = \"%s.pwn\"\n", sef_path);
				fprintf(file, "output = \"%s.amx\"\n", sef_path);
		} else {
				/* Default bare gamemode */
				fprintf(file, "input = \"gamemodes/bare.pwn\"\n");
				fprintf(file, "output = \"gamemodes/bare.amx\"\n");
		}

		/* Write [depends] section */
		fprintf(file, "[depends]\n");
		fprintf(file, "github_tokens = \"DO_HERE\"\n");
		/* Dependency repositories */
		fprintf(file, "aio_repo = [\"Y-Less/sscanf:latest\", "
					  "\"samp-incognito/samp-streamer-plugin:latest\"]");
}

/*
 * Main TOML configuration management function orchestrates detection,
 * generation, and parsing of watchdogs.toml file.
 * Detects server type, finds compiler, checks capabilities, creates or updates
 * configuration, and loads settings into global configuration structure.
 */
int wg_toml_configs(void)
{
		int find_pawncc = 0, find_gamemodes = 0;
		int compatibility = 0, optimized_lt = 0;

		const char *wg_os_type;
		FILE *toml_file;

		wg_os_type = wg_fetch_os(); /* Detect OS */

		/* Determine server type by checking directories and files */
		if (dir_exists("qawno") &&
			dir_exists("components"))
			samp_user = 0; /* OpenMP */
		else if (dir_exists("pawno") &&
			path_exists("server.cfg"))
			samp_user = 1; /* SA-MP */
		else {
			static int crit_nf = 0;
			if (crit_nf == 0) {
				crit_nf = 1;
				pr_crit(stdout, "can't locate sa-mp/open.mp server!");
				return 0; /* Can't determine server type */
			}
		}

		/* Search for PAWN compiler */
		find_pawncc = wg_find_compiler(wg_os_type);
		if (!find_pawncc) {
			/* Fallback: search in current directory */
			if (strcmp(wg_os_type, "windows") == 0)
					find_pawncc = wg_sef_fdir(".", "pawncc.exe", NULL);
			else
					find_pawncc = wg_sef_fdir(".", "pawncc", NULL);
		}

		/* Search for gamemode source files */
		find_gamemodes = wg_sef_fdir("gamemodes/", "*.pwn", NULL);

		/* Test compiler capabilities if found */
		if (find_pawncc) {
			wg_check_compiler_options(&compatibility, &optimized_lt);
		}

		/* Check if TOML file already exists */
		toml_file = fopen("watchdogs.toml", "r");
		if (toml_file) {
			fclose(toml_file); /* File exists, will parse later */
		} else {
			/* Create new TOML file */
			toml_file = fopen("watchdogs.toml", "w");
			if (!toml_file) {
					pr_error(stdout, "Failed to create watchdogs.toml");
					return 1;
			}

			/* Generate content based on findings */
			if (find_pawncc)
				wg_generate_toml_content(toml_file, wg_os_type, find_gamemodes,
					compatibility, optimized_lt, wgconfig.wg_sef_found_list[1]);
			else
				wg_generate_toml_content(toml_file, wg_os_type, find_gamemodes,
					compatibility, optimized_lt, wgconfig.wg_sef_found_list[0]);
			fclose(toml_file);
		}

		/* Parse the TOML file */
		if (!wg_parsewg_toml_config()) {
				pr_error(stdout, "Failed to parse TOML configuration");
				return 1;
		}

		/* Re-parse for additional configuration extraction */
		char error_buffer[256];
		toml_table_t *wg_toml_config;
		FILE *proc_f = fopen("watchdogs.toml", "r");
		wg_toml_config = toml_parse_file(proc_f, error_buffer, sizeof(error_buffer));
		if (proc_f) fclose(proc_f);

		if (!wg_toml_config) {
			pr_error(stdout, "parsing TOML: %s", error_buffer);
			chain_goto_main(NULL); /* Error handling */
		}

		/* Extract dependencies section */
		toml_table_t *wg_toml_depends = toml_table_in(wg_toml_config, "depends");
		if (wg_toml_depends) {
				toml_datum_t toml_gh_tokens = toml_string_in(wg_toml_depends, "github_tokens");
				if (toml_gh_tokens.ok)
				{
					wgconfig.wg_toml_github_tokens = strdup(toml_gh_tokens.u.s);
					wg_free(toml_gh_tokens.u.s);
				}
		}

		/* Extract compiler section */
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

		/* Extract general section details */
		toml_table_t *general_table = toml_table_in(wg_toml_config, "general");
		if (general_table) {
				toml_datum_t bin_val = toml_string_in(general_table, "binary");
				if (bin_val.ok) {
					/* Store binary path based on server type */
					if (samp_user == 1) {
						wgconfig.wg_is_samp = CRC32_TRUE;
						wgconfig.wg_ptr_samp = strdup(bin_val.u.s);
					}
					else if (samp_user == 0) {
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
				toml_datum_t webhooks_val = toml_string_in(general_table, "webhooks");
				if (webhooks_val.ok) {
					wgconfig.wg_toml_webhooks = strdup(webhooks_val.u.s);
					wg_free(webhooks_val.u.s);
				}
		}

		toml_free(wg_toml_config); /* Free parse tree */

		/* Convert OS type string to enum value */
		if (strcmp(wgconfig.wg_toml_os_type, "windows") == 0) {
			wgconfig.wg_os_type = OS_SIGNAL_WINDOWS;
		} else if (strcmp(wgconfig.wg_toml_os_type, "linux") == 0) {
			wgconfig.wg_os_type = OS_SIGNAL_LINUX;
		}

		return 0; /* Success */
}

/*
 * Attempts to move file without sudo/administrator privileges.
 * Constructs platform-appropriate move command and executes it.
 * Returns command execution result.
 */
static int _try_mv_without_sudo(const char *src, const char *dest) {
	    char size_mv[WG_PATH_MAX];
	    /* Platform-specific move commands */
	    if (is_native_windows())
	        wg_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_mv, sizeof(size_mv), "mv -f %s %s", src, dest);
	    int ret = wg_run_command(size_mv);
	    return ret;
}

/*
 * Moves file with sudo/administrator privileges.
 * Constructs move command with sudo prefix on Unix, regular move on Windows.
 * Returns command execution result.
 */
static int __mv_with_sudo(const char *src, const char *dest) {
	    char size_mv[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_mv, sizeof(size_mv), "move /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_mv, sizeof(size_mv), "sudo mv -f %s %s", src, dest);
	    int ret = wg_run_command(size_mv);
	    return ret;
}

/*
 * Attempts to copy file without sudo/administrator privileges.
 * Uses xcopy on Windows, cp on Unix with force flag.
 * Returns command execution result.
 */
static int _try_cp_without_sudo(const char *src, const char *dest) {
	    char size_cp[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_cp, sizeof(size_cp), "cp -f %s %s", src, dest);
	    int ret = wg_run_command(size_cp);
	    return ret;
}

/*
 * Copies file with sudo/administrator privileges.
 * Adds sudo prefix to copy command on Unix systems.
 * Returns command execution result.
 */
static int __cp_with_sudo(const char *src, const char *dest) {
	    char size_cp[WG_PATH_MAX];
	    if (is_native_windows())
	        wg_snprintf(size_cp, sizeof(size_cp), "xcopy /-Y %s %s", src, dest);
	    else
	        wg_snprintf(size_cp, sizeof(size_cp), "sudo cp -f %s %s", src, dest);
	    int ret = wg_run_command(size_cp);
	    return ret;
}

/*
 * Validates file operation safety preconditions before move/copy.
 * Performs comprehensive checks: null pointers, path lengths, existence,
 * file types, same-file detection, parent directory validation.
 * Returns 1 if all checks pass, logs errors otherwise.
 */
static int __wg_sef_safety(const char *c_src, const char *c_dest)
{
		char parent[WG_PATH_MAX];
		struct stat st;

		/* Basic input validation */
		if (!c_src || !c_dest)
				pr_error(stdout, "src or dest is null");
		if (!*c_src || !*c_dest)
				pr_error(stdout, "src or dest empty");
		/* Path length validation */
		if (strlen(c_src) >= WG_PATH_MAX || strlen(c_dest) >= WG_PATH_MAX)
				pr_error(stdout, "path too long");
		/* Source existence and type */
		if (!path_exists(c_src))
				pr_error(stdout, "source does not exist: %s", c_src);
		if (!file_regular(c_src))
				pr_error(stdout, "source is not a regular file: %s", c_src);
		/* Same file detection */
		if (path_exists(c_dest) && file_same_file(c_src, c_dest))
				pr_info(stdout, "source and dest are the same file: %s", c_src);
		/* Parent directory validation */
		if (ensure_parent_dir(parent, sizeof(parent), c_dest))
				pr_error(stdout, "cannot determine parent dir of dest");
		if (stat(parent, &st))
				pr_error(stdout, "destination dir does not exist: %s", parent);
		if (!S_ISDIR(st.st_mode))
				pr_error(stdout, "destination parent is not a dir: %s", parent);

		return 1; /* All checks passed */
}

/*
 * Sets executable permissions on destination file after move/copy operation.
 * Uses platform-specific chmod functions: win32_chmod on Windows, chmod on Unix.
 * Logs warnings if permission setting fails (debug builds only).
 */
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
		if (chmod(c_dest, 0755)) { /* rwxr-xr-x permissions */
# if defined(_DBG_PRINT)
				pr_warning(stdout, "chmod failed: %s (errno=%d %s)",
						      c_dest, errno, strerror(errno));
# endif
		}
#endif
}

/*
 * Smart file moving with automatic privilege escalation fallback.
 * Performs safety checks, attempts move without sudo first,
 * falls back to sudo if needed and available. Sets permissions after move.
 */
int wg_sef_wmv(const char *c_src, const char *c_dest)
{
		int ret, mv_ret;

		/* Validate operation safety */
		ret = __wg_sef_safety(c_src, c_dest);
		if (ret != 1)
				return 1;

		/* Determine if sudo is available */
		int is_not_sudo = 0;
#ifdef WG_WINDOWS
		is_not_sudo = 1; /* Windows doesn't use sudo */
#else
		is_not_sudo = wg_run_command("sudo echo > /dev/null");
#endif
		/* Try without sudo first if available */
		if (is_not_sudo == 1) {
			mv_ret = _try_mv_without_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wg_sef_set_permissions(c_dest); /* Set permissions */
					pr_info(stdout, "* moved without sudo: '%s' -> '%s'", c_src, c_dest);
					return 0; /* Success without sudo */
			}
		} else {
			/* Use sudo for move operation */
			mv_ret = __mv_with_sudo(c_src, c_dest);

			if (!mv_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* moved with sudo: '%s' -> '%s'", c_src, c_dest);
					return 0; /* Success with sudo */
			}
		}

		return 1; /* Move failed */
}

/*
 * Smart file copying with automatic privilege escalation fallback.
 * Similar to wg_sef_wmv but for copy operations. Validates safety,
 * tries without sudo first, falls back to sudo if needed.
 */
int wg_sef_wcopy(const char *c_src, const char *c_dest)
{
		int ret, cp_ret;

		/* Validate operation safety */
		ret = __wg_sef_safety(c_src, c_dest);
		if (ret != 1)
				return 1;

		/* Check sudo availability */
		int is_not_sudo = 0;
#ifdef WG_WINDOWS
		is_not_sudo = 1;
#else
		is_not_sudo = wg_run_command("sudo echo > /dev/null");
#endif
		/* Try copy without sudo first */
		if (is_not_sudo == 1) {
			cp_ret = _try_cp_without_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying without sudo: '%s' -> '%s'", c_src, c_dest);
					return 0; /* Success without sudo */
			}
		} else {
			/* Use sudo for copy operation */
			cp_ret = __cp_with_sudo(c_src, c_dest);

			if (!cp_ret) {
					__wg_sef_set_permissions(c_dest);
					pr_info(stdout, "* copying with sudo: '%s' -> '%s'", c_src, c_dest);
					return 0; /* Success with sudo */
			}
		}

		return 1; /* Copy failed */
}