/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdbool.h>
#include  <ctype.h>
#include  <errno.h>
#include  <fcntl.h>
#include  <dirent.h>
#include  <limits.h>
#include  <libgen.h>
#include  <sys/stat.h>
#include  <sys/types.h>

#ifdef DOG_LINUX
#include  <termios.h>
#endif
#if defined(DOG_ANDROID)
ssize_t
sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
	char	 buf[DOG_MORE_MAX_PATH];
	size_t	 left = count;

	while (left > 0) {
		ssize_t	 n = read(in_fd, buf, sizeof(buf));

		if (n <= 0)
			return (n);
		int w = write(out_fd, buf, n);
		if (w) {;} else {;}
		left -= n;
	}
	return (count);
}
#elif !defined(DOG_ANDROID) && defined(DOG_LINUX)
#include  <sys/sendfile.h>
#include  <sys/wait.h>
#endif

#include  "extra.h"
#include  "units.h"
#include  "library.h"
#include  "crypto.h"
#include  "debug.h"
#include  "compiler.h"
#include  "utils.h"

const char	*__command[] = {
	"help", "exit", "sha1", "sha256", "crc32", "djb2", "config",
	"replicate", "gamemode", "pawncc", "debug",
	"compile", "running", "compiles", "stop", "restart",
	"tracker", "compress", "send"
};

const size_t	 __command_len = sizeof(__command) / sizeof(__command[0]);

WatchdogConfig	 dogconfig = {
	.dog_garbage_access = { DOG_GARBAGE_ZERO },
	.dog_os_type = CRC32_FALSE,
	.dog_is_samp = CRC32_FALSE,
	.dog_is_omp = CRC32_FALSE,
	.dog_ptr_samp = NULL,
	.dog_ptr_omp = NULL,
	.dog_sef_count = RATE_SEF_EMPTY,
	.dog_sef_found_list = { { RATE_SEF_EMPTY } },
	.dog_toml_os_type = NULL,
	.dog_toml_binary = NULL,
	.dog_toml_config = NULL,
	.dog_toml_logs = NULL,
	.dog_toml_aio_opt = NULL,
	.dog_toml_root_patterns = NULL,
	.dog_toml_packages = NULL,
	.dog_toml_proj_input = NULL,
	.dog_toml_proj_output = NULL,
	.dog_toml_webhooks = NULL
};

const char	*char_fields[] = {
	"dog_toml_os_type",
	"dog_toml_binary",
	"dog_toml_config",
	"dog_toml_logs",
	"dog_toml_aio_opt",
	"dog_toml_root_patterns",
	"dog_toml_packages",
	"dog_toml_proj_input",
	"dog_toml_proj_output",
	"dog_toml_webhooks"
};

char		**field_pointers[] = {
	&dogconfig.dog_toml_os_type,
	&dogconfig.dog_toml_binary,
	&dogconfig.dog_toml_config,
	&dogconfig.dog_toml_logs,
	&dogconfig.dog_toml_aio_opt,
	&dogconfig.dog_toml_root_patterns,
	&dogconfig.dog_toml_packages,
	&dogconfig.dog_toml_proj_input,
	&dogconfig.dog_toml_proj_output,
	&dogconfig.dog_toml_webhooks
};

void
dog_sef_restore(void)
{
	size_t	 i, sef_max_entries;

	sef_max_entries = sizeof(dogconfig.dog_sef_found_list) /
	    sizeof(dogconfig.dog_sef_found_list[0]);

	for (i = 0; i < sef_max_entries; i++)
		dogconfig.dog_sef_found_list[i][0] = '\0';

	dogconfig.dog_sef_count = RATE_SEF_EMPTY;
	memset(dogconfig.dog_sef_found_list, RATE_SEF_EMPTY,
	    sizeof(dogconfig.dog_sef_found_list));
}

#ifdef DOG_LINUX
#ifndef strlcpy
size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t	 src_len = strlen(src);

	if (size) {
		size_t	 copy_len = (src_len >= size) ? size - 1 : src_len;
		memcpy(dst, src, copy_len);
		dst[copy_len] = '\0';
	}
	return (src_len);
}
#endif
#ifndef strlcat
size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t	 dst_len = strlen(dst);
	size_t	 src_len = strlen(src);

	if (dst_len < size) {
		size_t	 copy_len = size - dst_len - 1;

		if (copy_len > src_len)
			copy_len = src_len;
		memcpy(dst + dst_len, src, copy_len);
		dst[dst_len + copy_len] = '\0';
	}
	return (dst_len + src_len);
}
#endif
#endif

#ifdef DOG_WINDOWS
size_t
win_strlcpy(char *dst, const char *src, size_t size)
{
	size_t	 len = strlen(src);

	if (size > 0) {
		size_t	 copy = (len >= size) ? size - 1 : len;

		memcpy(dst, src, copy);
		dst[copy] = 0;
	}
	return (len);
}

size_t
win_strlcat(char *dst, const char *src, size_t size)
{
	size_t	 dlen = strlen(dst);
	size_t	 slen = strlen(src);

	if (dlen < size) {
		size_t	 space = size - dlen - 1;
		size_t	 copy = (slen > space) ? space : slen;

		memcpy(dst + dlen, src, copy);
		dst[dlen + copy] = 0;
		return (dlen + slen);
	}
	return (size + slen);
}

int
win_ftruncate(FILE *file, long length)
{
	HANDLE		 hFile;
	LARGE_INTEGER	 li;

	hFile = (HANDLE)_get_osfhandle(_fileno(file));
	if (hFile == INVALID_HANDLE_VALUE)
		return (-1);

	li.QuadPart = length;
	if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) == 0)
		return (-1);
	if (SetEndOfFile(hFile) == 0)
		return (-1);

	return (0);
}
#endif

void *
dog_malloc(size_t size)
{
	void	*ptr = malloc(size);

	if (!ptr) {
		fprintf(stderr, "malloc failed: %zu bytes\n", size);
		minimal_debugging();
		return (NULL);
	}
	return (ptr);
}

void *
dog_calloc(size_t count, size_t size)
{
	void	*ptr = calloc(count, size);

	if (!ptr) {
		fprintf(stderr,
		    "calloc failed: %zu elements x %zu bytes\n", count, size);
		minimal_debugging();
		return (NULL);
	}
	return (ptr);
}

void *
dog_realloc(void *ptr, size_t size)
{
	void	*new_ptr = (ptr ? realloc(ptr, size) : malloc(size));

	if (!new_ptr) {
		fprintf(stderr, "realloc failed: %zu bytes\n", size);
		minimal_debugging();
		return (NULL);
	}
	return (new_ptr);
}

void
dog_free(void *ptr)
{
	if (ptr)
		free(ptr);
}

void
path_sym_convert(char *path)
{
	char	*pos;

	for (pos = path; *pos; pos++) {
		if (*pos == __PATH_CHR_SEP_WIN32)
			*pos = __PATH_CHR_SEP_LINUX;
	}
}

const char *
try_get_filename(const char *path)
{
	const char	*__name = PATH_SEPARATOR(path);

	return (__name ? __name + 1 : path);
}

const char *
try_get_basename(const char *path)
{
	const char	*p = PATH_SEPARATOR(path);

	return (p ? p + 1 : path);
}

char *
dog_procure_pwd(void)
{
	static char	 dog_work_dir[DOG_PATH_MAX];

	if (dog_work_dir[0] == '\0') {
#ifdef DOG_WINDOWS
		DWORD	 size_len;

		size_len = GetCurrentDirectoryA(sizeof(dog_work_dir),
		    dog_work_dir);
		if (size_len == 0 || size_len >= sizeof(dog_work_dir))
			dog_work_dir[0] = '\0';
#else
		if (getcwd(dog_work_dir, sizeof(dog_work_dir)) == NULL) {
				dog_work_dir[0] = '\0';
		}
#endif
	}
	return (dog_work_dir);
}

char *
dog_masked_text(int reveal, const char *text)
{
	char	*masked;
	int	 len, i;

	if (!text)
		return (NULL);

	len = (int)strlen(text);
	if (reveal < 0)
		reveal = 0;
	if (reveal > len)
		reveal = len;

	masked = dog_malloc((size_t)len + 1);
	if (!masked)
		unit_ret_main(NULL);

	if (reveal > 0)
		memcpy(masked, text, (size_t)reveal);

	for (i = reveal; i < len; ++i)
		masked[i] = '?';

	masked[len] = '\0';
	return (masked);
}

int
dog_mkdir(const char *path)
{
	char	 tmp[PATH_MAX];
	char	*p;
	size_t	 len;

	if (!path || !*path)
		return (-1);

	snprintf(tmp, sizeof(tmp), "%s", path);
	len = strlen(tmp);

	if (len > 1 && tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	for (p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			if (MKDIR(tmp) != 0 && errno != EEXIST) {
				perror("mkdir");
				return (-1);
			}
			*p = '/';
		}
	}

	if (MKDIR(tmp) != 0 && errno != EEXIST) {
		perror("mkdir");
		return (-1);
	}

	return (0);
}

void
unit_show_dog(void) {

	printf("\n                         \\/%%#z.      \\/.%%#z./    /,z#%%\\/\n");
	printf("                         \\X##k      /X#####X\\   /d##X/\n");
	printf("                         \\888\\   /888/ \\888\\   /888/\n");
	printf("                        `v88;  ;88v'   `v88;  ;88v'\n");
	printf("                         \\77xx77/       \\77xx77/\n");
	printf("                        `::::'         `::::'\n\n");
	fflush(stdout);
	println(stdout, "---------------------------------------------------------------------------------------------");
	println(stdout, "                                            -----------------------------------------        ");
	println(stdout, "      ;printf(\"Hello, World\")               |  plugin installer                     |        ");
	println(stdout, "                                            v          v                            |        ");
	println(stdout, "pawncc | compile | gamemode | running | compiles | replicate | restart | stop       |        ");
	println(stdout, "  ^        ^          ^          ^ -----------------------       ^         ^        v        ");
	println(stdout, "  -------  ---------   ------------------                |       |         | compile n run  ");
	println(stdout, "        v          |                    |                |       v         --------          ");
	println(stdout, " install compiler  v                    v                v  restart server        |          ");
	println(stdout, "               compile gamemode  install gamemode  running server             stop server    ");
	println(stdout, "---------------------------------------------------------------------------------------------");
	println(stdout, "Use \"help\" for more.");
}

void
compiler_show_tip(void) {

	printf(DOG_COL_YELLOW "Options:\n" DOG_COL_DEFAULT);

	printf(
	"  %s-w%s, %s--watchdogs%s   Show detailed output         - compile detail\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-d%s, %s--debug%s       Enable debug level 2         - full debugging\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-p%s, %s--prolix%s      Verbose mode                 - processing detail\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-a%s, %s--assembler%s   Generate assembler file      - assembler ouput\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-s%s, %s--compact%s     Compact encoding compression - resize output\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-c%s, %s--compat%s      Compatibility mode           - path sep compat\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-f%s, %s--fast%s        Fast compile-sime            - no optimize\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);

	printf(
	"  %s-n%s, %s--clean%s       Remove '.amx' after compile  - remover output\n",
	DOG_COL_CYAN, DOG_COL_DEFAULT,
	DOG_COL_CYAN, DOG_COL_DEFAULT);
}

void
unit_show_help(const char *command)
{
	if (strlen(command) == 0) {
	static const char *help_text =
	"Usage: help <command> | help sha1\n\n"
	"Commands:\n"
	"  exit             exit from watchdogs | "
	"Usage: \"exit\" " DOG_COL_CYAN "; Just type 'exit' and you're outta here!" DOG_COL_DEFAULT "\n"
	"  sha1             generate sha1 hash | "
	"Usage: \"sha1\" | [<args>] " DOG_COL_CYAN "; Get that SHA1 hash for your text." DOG_COL_DEFAULT "\n"
	"  sha256           generate sha256 hash | "
	"Usage: \"sha256\" | [<args>] " DOG_COL_CYAN "; Get that SHA256 hash for your text." DOG_COL_DEFAULT "\n"
	"  crc32            generate crc32 checksum | "
	"Usage: \"crc32\" | [<args>] " DOG_COL_CYAN "; Quick CRC32 checksum generation." DOG_COL_DEFAULT "\n"
	"  djb2             generate djb2 hash file | "
	"Usage: \"djb2\" | [<args>] " DOG_COL_CYAN "; djb2 hashing for your files." DOG_COL_DEFAULT "\n"
	"  config           re-write watchdogs.toml | "
	"Usage: \"config\" " DOG_COL_CYAN "; Reset your config file to default settings." DOG_COL_DEFAULT "\n"
	"  replicate        dependency installer | "
	"Usage: \"replicate\" " DOG_COL_CYAN "; Downloads & Install Our Dependencies." DOG_COL_DEFAULT "\n"
	"  gamemode         download SA-MP gamemode | "
	"Usage: \"gamemode\" " DOG_COL_CYAN "; Grab some SA-MP gamemodes quickly." DOG_COL_DEFAULT "\n"
	"  pawncc           download SA-MP pawncc | "
	"Usage: \"pawncc\" " DOG_COL_CYAN "; Get the Pawn Compiler for SA-MP/open.mp." DOG_COL_DEFAULT "\n"
	"  debug            debugging & logging server logs | "
	"Usage: \"debug\" " DOG_COL_CYAN "; Keep an eye on your server logs." DOG_COL_DEFAULT "\n"
	"  compile          compile your project | "
	"Usage: \"compile\" | [<args>] " DOG_COL_CYAN "; Turn your code into something runnable!" DOG_COL_DEFAULT "\n"
	"  running          running your project | "
	"Usage: \"running\" | [<args>] " DOG_COL_CYAN "; Fire up your project and see it in action." DOG_COL_DEFAULT "\n"
	"  compiles         compile and running your project | "
	"Usage: \"compiles\" | [<args>] " DOG_COL_CYAN "; Two-in-one: compile then run immediately!." DOG_COL_DEFAULT "\n"
	"  stop             stopped server tasks | "
	"Usage: \"stop\" " DOG_COL_CYAN "; Halt everything! Stop your server tasks." DOG_COL_DEFAULT "\n"
	"  restart          re-start server tasks | "
	"Usage: \"restart\" " DOG_COL_CYAN "; Fresh start! Restart your server." DOG_COL_DEFAULT "\n"
	"  tracker          account tracking | "
	"Usage: \"tracker\" | [<args>] " DOG_COL_CYAN "; Track accounts across platforms." DOG_COL_DEFAULT "\n"
	"  compress         create a compressed archive | "
	"Usage: \"compress <input> <output>\" " DOG_COL_CYAN "; Generates a compressed file (e.g., .zip/.tar.gz) from the specified source." DOG_COL_DEFAULT "\n"
	"  send             send file to Discord channel via webhook | "
	"Usage: \"send <files>\" " DOG_COL_CYAN "; Uploads a file directly to a Discord channel using a webhook." DOG_COL_DEFAULT "\n";

	fwrite(help_text, 1, strlen(help_text), stdout);
	return;
	}

	if (strcmp(command, "exit") == 0) {
		println(stdout, "exit: exit from watchdogs. | Usage: \"exit\"\n\tJust type 'exit' and you're outta here!");
	} else if (strcmp(command, "sha1") == 0) {
		println(stdout, "sha1: generate sha1. | Usage: \"sha1\" | [<args>]\n\tGet that SHA1 hash for your text.");
	} else if (strcmp(command, "sha256") == 0) {
		println(stdout, "sha256: generate sha256. | Usage: \"sha256\" | [<args>]\n\tGet that SHA256 hash for your text.");
	} else if (strcmp(command, "crc32") == 0) {
		println(stdout, "crc32: generate crc32. | Usage: \"crc32\" | [<args>]\n\tQuick CRC32 checksum generation.");
	} else if (strcmp(command, "djb2") == 0) {
		println(stdout, "djb2: generate djb2 hash file. | Usage: \"djb2\" | [<args>]\n\tdjb2 hashing for your files.");
	} else if (strcmp(command, "config") == 0) {
		println(stdout, "config: re-write watchdogs.toml. | Usage: \"config\"\n\tReset your config file to default settings.");
	} else if (strcmp(command, "replicate") == 0) {
		println(stdout, "replicate: dependency installer. | Usage: \"replicate\"\n\tDownloads & Install Our Dependencies.");
	} else if (strcmp(command, "gamemode") == 0) {
		println(stdout, "gamemode: download SA-MP gamemode. | Usage: \"gamemode\"\n\tGrab some SA-MP gamemodes quickly.");
	} else if (strcmp(command, "pawncc") == 0) {
		println(stdout, "pawncc: download SA-MP pawncc. | Usage: \"pawncc\"\n\tGet the Pawn Compiler for SA-MP/open.mp.");
	} else if (strcmp(command, "debug") == 0) {
		println(stdout, "debug: debugging & logging server debug. | Usage: \"debug\"\n\tKeep an eye on your server logs.");
	} else if (strcmp(command, "compile") == 0) {
		println(stdout, "compile: compile your project. | Usage: \"compile\" | [<args>]\n\tTurn your code into something runnable!");
	} else if (strcmp(command, "running") == 0) {
		println(stdout, "running: running your project. | Usage: \"running\" | [<args>]\n\tFire up your project and see it in action.");
	} else if (strcmp(command, "compiles") == 0) {
		println(stdout, "compiles: compile and running your project. | Usage: \"compiles\" | [<args>]\n\tTwo-in-one: compile then run immediately!");
	} else if (strcmp(command, "stop") == 0) {
		println(stdout, "stop: stopped server task. | Usage: \"stop\"\n\tHalt everything! Stop your server tasks.");
	} else if (strcmp(command, "restart") == 0) {
		println(stdout, "restart: re-start server task. | Usage: \"restart\"\n\tFresh start! Restart your server.");
	} else if (strcmp(command, "tracker") == 0) {
		println(stdout, "tracker: account tracking. | Usage: \"tracker\" | [<args>]\n\tTrack accounts across platforms.");
	} else if (strcmp(command, "compress") == 0) {
		println(stdout, "compress: create a compressed archive from a file or folder. | "
		    "Usage: \"compress <input> <output>\"\n\tGenerates a compressed file (e.g., .zip/.tar.gz) from the specified source.");
	} else if (strcmp(command, "send") == 0) {
		println(stdout, "send: send file to Discord channel via webhook. | "
		    "Usage: \"send <files>\"\n\tUploads a file directly to a Discord channel using a webhook.");
	} else {
		printf("help can't found for: '");
		pr_color(stdout, DOG_COL_YELLOW, "%s", command);
		printf("'\n     Oops! That command doesn't exist. Try 'help' to see available commands.\n");
	}
	return;
}

void
dog_escaping_json(char *dest, const char *src, size_t dest_size)
{
	char		*ptr = dest;
	const char	*replacement;
	size_t		 remaining = dest_size, needed;

	if (dest_size == 0)
		return;

	while (*src && remaining > 1) {
		needed = 1;
		replacement = NULL;

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

		if (needed >= remaining)
			break;

		if (replacement) {
			memcpy(ptr, replacement, needed);
			ptr += needed;
			remaining -= needed;
		}
		src++;
	}

	*ptr = '\0';
}

int dog_exec_command(char *const argv[])
{
    char size_command[DOG_MAX_PATH] = {0};
    char *p;
    int high_acces_command = 0, ret;

    if (argv == NULL || argv[0] == NULL)
        return (-1);

	size_command[0] = '\0';
    for (int i = 0; argv[i] != NULL; i++) {
        for (p = argv[i]; *p; p++) {
            if (*p == ';' || *p == '`' || *p == '$' ||
				*p == '(' || *p == ')' || *p == '\n')
                return (-1);
        }
        if (i > 0) {
            strncat(size_command, " ", sizeof(size_command) - strlen(size_command) - 1);
        }
        strncat(size_command, argv[i], sizeof(size_command) - strlen(size_command) - 1);
    }

    size_t cmd_len = strlen(size_command);
    if (cmd_len >= DOG_MAX_PATH - 1) {
        pr_warning(stdout,
            "length over size detected!");
		minimal_debugging();
        return (-1);
    }

    if (strfind(size_command, "sudo", true) ||
		strfind(size_command, "run0", true) ||
		strfind(size_command, "su", true) ||
		strfind(size_command, "pkexec", true) ||
		strfind(size_command, "chmod 777", true) ||
		strfind(size_command, "chown", true) ||
		strfind(size_command, "rm -rf /", true) ||
		strfind(size_command, "dd if=", true) ||
		strfind(size_command, "mount", true) ||
		strfind(size_command, "systemctl", true) ||
		strfind(size_command, "init", true) ||
		strfind(size_command, "reboot", true) ||
		strfind(size_command, "shutdown", true))
	{
		high_acces_command = 1;
	}

	if (strfind(size_command, "rm -rf/", true) ||
        strfind(size_command, "rm -rf /", true)) {
        return (-1);
    }

	if (high_acces_command == 1) {
        if (dogconfig.dog_garbage_access[DOG_GARBAGE_CMD_WARN] == DOG_GARBAGE_ZERO)
		{
            printf("\n");
            printf("\t=== HIGH ACCESS SECURITY WARNING ===\n");
            printf("\tYou are about to run commands with ROOT privileges!\n");
            printf("\tThis can DAMAGE your system if used incorrectly.\n");
            printf("\t* Always verify commands before pressing Enter...\n");
            printf("\t===========================================\n");
			printf("\t[ %s ]", size_command);
            printf("\n");
			dogconfig.dog_garbage_access[DOG_GARBAGE_CMD_WARN] = DOG_GARBAGE_TRUE;
        }
    }

    ret = -1;
skip:
    ret = system(size_command);
    return (ret);
}

void
dog_clear_screen(void)
{
#ifdef DOG_WINDOWS
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD home = {0, 0};

    if (hOut == INVALID_HANDLE_VALUE)
        return;

    if (!GetConsoleScreenBufferInfo(hOut, &csbi))
        return;

    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    FillConsoleOutputCharacterA(hOut, ' ', cellCount, home, &count);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, home, &count);
    SetConsoleCursorPosition(hOut, home);
#else
    const char seq[] = "\033[2J\033[H";
    int w = write(STDOUT_FILENO, seq, sizeof(seq) - 1);
	if (w) {;} else {;}
#endif
}

int
dog_server_env(void)
{
	if (strcmp(dogconfig.dog_is_samp, CRC32_TRUE) == 0) {
		return (1);
	} else if (strcmp(dogconfig.dog_is_omp, CRC32_TRUE) == 0) {
		return (2);
	} else {
		return (1);
	}
}

int
is_termux_env(void)
{
	int	 is_termux = 0;
#if defined(DOG_ANDROID)
	is_termux = 1;
	return (is_termux);
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

	return (is_termux);
}

int
is_native_windows(void)
{
#if defined(DOG_LINUX) || defined(DOG_ANDROID)
	return (0);
#endif
	char	*msys2_env;

	msys2_env = getenv("MSYSTEM");
	if (msys2_env)
		return (0);
	else
		return (1);
}

void
dog_printfile(const char *path)
{
#ifdef DOG_WINDOWS
	int	 fd;
	char	 buf[(1 << 20) + 1];
	ssize_t	 n, w;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return;

	for (;;) {
		n = read(fd, buf, sizeof(buf) - 1);
		if (n <= 0)
			break;

		buf[n] = '\0';
		w = 0;
		while (w < n) {
			ssize_t	 k = write(STDOUT_FILENO, buf + w, n - w);

			if (k <= 0) {
				close(fd);
				return;
			}
			w += k;
		}
	}

	close(fd);
#else
	int		 fd;
	struct stat	 st;
	off_t		 off = 0;
	char		 buf[(1 << 20) + 1];
	ssize_t		 to_read, n, w;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return;
	}

	while (off < st.st_size) {
		to_read = (st.st_size - off) < (sizeof(buf) - 1) ?
		    (st.st_size - off) : (sizeof(buf) - 1);
		n = pread(fd, buf, to_read, off);
		if (n <= 0)
			break;
		off += n;

		buf[n] = '\0';
		w = 0;
		while (w < n) {
			ssize_t	 k = write(STDOUT_FILENO, buf + w, n - w);

			if (k <= 0) {
				close(fd);
				return;
			}
			w += k;
		}
	}

	close(fd);
#endif
	return;
}

int
dog_console_title(const char *title)
{
	const char	*new_title;
#ifdef DOG_ANDROID
	return (0);
#endif

	if (!title)
		new_title = watchdogs_release;
	else
		new_title = title;

#ifdef DOG_WINDOWS
	SetConsoleTitleA(new_title);
#else
	if (isatty(STDOUT_FILENO))
		printf("\033]0;%s\007", new_title);
#endif
	return (0);
}

void
dog_strip_dot_fns(char *dst, size_t dst_sz, const char *src)
{
	char	*slash, *dot;
	size_t	 len;

	if (!dst || dst_sz == 0 || !src)
		return;

	slash = strchr(src, __PATH_CHR_SEP_LINUX);
#ifdef DOG_WINDOWS
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

	snprintf(dst, dst_sz, "%s", src);
}

bool
dog_strcase(const char *text, const char *pattern)
{
	const char	*p, *a, *b;

	for (p = text; *p; p++) {
		a = p;
		b = pattern;
		while (*a && *b && (((*a | 32) == (*b | 32)))) {
			a++;
			b++;
		}
		if (!*b)
			return (true);
	}
	return (false);
}

bool
strend(const char *str, const char *suffix, bool nocase)
{
	size_t	 lenstr, lensuf;
	const char *p;

	if (!str || !suffix)
		return (false);

	lenstr = strlen(str);
	lensuf = strlen(suffix);

	if (lensuf > lenstr)
		return (false);

	p = str + (lenstr - lensuf);
	return (nocase ?
	    strncasecmp(p, suffix, lensuf) == 0 :
	    memcmp(p, suffix, lensuf) == 0);
}

bool
strfind(const char *text, const char *pattern, bool nocase)
{
	size_t	 m;
	const char *p;
	char	 c1, c2;

	if (!text || !pattern)
		return (false);

	m = strlen(pattern);
	if (m == 0)
		return (true);

	p = text;
	while (*p) {
		c1 = *p;
		c2 = *pattern;

		if (nocase) {
			c1 = tolower((unsigned char)c1);
			c2 = tolower((unsigned char)c2);
		}

		if (c1 == c2) {
			if (nocase) {
				if (strncasecmp(p, pattern, m) == 0)
					return (true);
			} else {
				if (memcmp(p, pattern, m) == 0)
					return (true);
			}
		}
		p++;
	}

	return (false);
}

__PURE__
char *
strreplace(const char *source, const char *old_sub, const char *new_sub)
{
	char		*result;
	const char	*pos;
	size_t		 source_len, old_sub_len, new_sub_len, result_len;
	size_t		 i = 0, j = 0;

	if (!source || !old_sub || !new_sub)
		return (NULL);

	source_len = strlen(source);
	old_sub_len = strlen(old_sub);
	new_sub_len = strlen(new_sub);

	result_len = source_len;
	pos = source;
	while ((pos = strstr(pos, old_sub)) != NULL) {
		result_len += new_sub_len - old_sub_len;
		pos += old_sub_len;
	}

	result = dog_malloc(result_len + 1);
	if (!result)
		unit_ret_main(NULL);

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
	return (result);
}

void
dog_escape_quotes(char *dest, size_t size, const char *src)
{
	size_t	 i, j;

	if (!dest || size == 0 || !src)
		return;

	for (i = 0, j = 0; src[i] != '\0' && j + 1 < size; i++) {
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

void
__set_path_sep(char *out, size_t out_sz, const char *open_dir,
    const char *entry_name)
{
	size_t	 dir_len, entry_len, max_needed;
	int	 dir_has_sep, has_led_sep;

	if (!out || out_sz == 0 || !open_dir || !entry_name)
		return;

	dir_len = strlen(open_dir);
	dir_has_sep = (dir_len > 0 && IS_PATH_SEP(open_dir[dir_len - 1]));
	has_led_sep = IS_PATH_SEP(entry_name[0]);
	entry_len = strlen(entry_name);
	max_needed = dir_len + entry_len + 2;

	if (max_needed >= out_sz) {
		out[0] = '\0';
		return;
	}

	if (dir_has_sep) {
		if (has_led_sep)
			snprintf(out, out_sz, "%s%s", open_dir, entry_name + 1);
		else
			snprintf(out, out_sz, "%s%s", open_dir, entry_name);
	} else {
		if (has_led_sep)
			snprintf(out, out_sz, "%s%s", open_dir, entry_name);
		else
			snprintf(out, out_sz, "%s%s%s", open_dir, __PATH_SEP,
			    entry_name);
	}

	out[out_sz - 1] = '\0';
}

__PURE__
static int
__command_suggest(const char *s1, const char *s2)
{
	int	 len1, len2, i, j;
	uint16_t*buf1, *buf2, *prev, *curr, *tmp;
	char	 c1, c2;
	int	 cost, del, ins, sub, val, min_row;

	len1 = strlen(s1);
	len2 = strlen(s2);
	if (len2 > 128)
		return (INT_MAX);

	buf1 = alloca((len2 + 1) * sizeof(uint16_t));
	buf2 = alloca((len2 + 1) * sizeof(uint16_t));
	prev = buf1;
	curr = buf2;

	for (j = 0; j <= len2; j++)
		prev[j] = j;

	for (i = 1; i <= len1; i++) {
		curr[0] = i;
		c1 = tolower((unsigned char)s1[i - 1]);
		min_row = INT_MAX;

		for (j = 1; j <= len2; j++) {
			c2 = tolower((unsigned char)s2[j - 1]);
			cost = (c1 == c2) ? 0 : 1;
			del = prev[j] + 1;
			ins = curr[j - 1] + 1;
			sub = prev[j - 1] + cost;
			val = ((del) < (ins) ? ((del) < (sub) ? (del) :
			    (sub)) : ((ins) < (sub) ? (ins) : (sub)));
			curr[j] = val;
			if (val < min_row)
				min_row = val;
		}

		if (min_row > 6)
			return (min_row + (len1 - i));

		tmp = prev;
		prev = curr;
		curr = tmp;
	}

	return (prev[len2]);
}

const char *
dog_find_near_command(const char *command, const char *commands[],
    size_t num_cmds, int *out_distance)
{
	int		 best_distance = INT_MAX;
	const char	*best_cmd = NULL;
	size_t		 i;

	for (i = 0; i < num_cmds; i++) {
		int	 dist = __command_suggest(command, commands[i]);

		if (dist < best_distance) {
			best_distance = dist;
			best_cmd = commands[i];
			if (best_distance == 0)
				break;
		}
	}

	if (out_distance)
		*out_distance = best_distance;

	return (best_cmd);
}

int
is_running_in_container(void)
{
	FILE	*fp;
	char	 line[DOG_MAX_PATH];

	if (path_access("/.dockerenv"))
		return (1);
	if (path_access("/run/.containerenv"))
		return (1);

	fp = fopen("/proc/1/cgroup", "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp)) {
			if (strstr(line, "/docker/") ||
			    strstr(line, "/podman/") ||
			    strstr(line, "/containerd/") ||
			    strstr(line, "kubepods")) {
				fclose(fp);
				return (1);
			}
		}
		fclose(fp);
	}

	return (0);
}

const char *
dog_procure_os(void)
{
	static char	 os[64] = "unknown";

#ifdef DOG_WINDOWS
	strncpy(os, "windows", sizeof(os));
#endif
#ifdef DOG_LINUX
	strncpy(os, "linux", sizeof(os));
#endif

	if (is_running_in_container())
		strncpy(os, "linux", sizeof(os));
	else if (path_access("/proc/sys/fs/binfmt_misc/WSLInterop") == 1)
		strncpy(os, "windows", sizeof(os));

	os[sizeof(os)-1] = '\0';
	return (os);
}

int
dir_exists(const char *path)
{
	struct stat	 st;

	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
		return (1);
	return (0);
}

int
path_exists(const char *path)
{
	struct stat	 st;

	if (stat(path, &st) == 0)
		return (1);
	return (0);
}

int
dir_writable(const char *path)
{
	if (access(path, W_OK) == 0)
		return (1);
	return (0);
}

int
path_access(const char *path)
{
	if (access(path, F_OK) == 0)
		return (1);
	return (0);
}

int
file_regular(const char *path)
{
	struct stat	 st;

	if (stat(path, &st) != 0)
		return (0);
	return (S_ISREG(st.st_mode));
}

int
file_same_file(const char *a, const char *b)
{
	struct stat	 sa, sb;

	if (stat(a, &sa) != 0)
		return (0);
	if (stat(b, &sb) != 0)
		return (0);

	return (sa.st_ino == sb.st_ino && sa.st_dev == sb.st_dev);
}

__PURE__
int
ensure_parent_dir(char *out_parent, size_t n, const char *dest)
{
	char	 tmp[DOG_PATH_MAX];
	char	*parent;

	if (strlen(dest) >= sizeof(tmp))
		return (-1);

	strncpy(tmp, dest, sizeof(tmp));
	tmp[sizeof(tmp)-1] = '\0';
	parent = dirname(tmp);
	if (!parent)
		return (-1);

	strncpy(out_parent, parent, n);
	out_parent[n-1] = '\0';
	return (0);
}

int
dog_kill_process(const char *process)
{
    if (!process)
        return (-1);

#ifdef DOG_WINDOWS
    STARTUPINFOA _STARTUPINFO;
    PROCESS_INFORMATION _PROCESS_INFO;
    SECURITY_ATTRIBUTES _ATTRIBUTES;
    char command[DOG_PATH_MAX * 2];

    _ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
    _ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));
    _ZERO_MEM_WIN32(&_ATTRIBUTES, sizeof(_ATTRIBUTES));

    _STARTUPINFO.cb = sizeof(_STARTUPINFO);

    snprintf(
        command,
        sizeof(command),
        "C:\\Windows\\System32\\taskkill.exe /F /IM \"%s\"",
        process
    );

    if (!CreateProcessA(
		NULL,
		command,
		NULL,
		NULL,
		FALSE,
		CREATE_NO_WINDOW,
		NULL,
		NULL,
		&_STARTUPINFO,
		&_PROCESS_INFO))
        return (-1);

    WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
    CloseHandle(_PROCESS_INFO.hProcess);
    CloseHandle(_PROCESS_INFO.hThread);

    return (0);

#else

#if !defined(DOG_ANDROID) && defined(DOG_LINUX)
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        execlp("pkill", "pkill", "-SIGTERM", process, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#else
    pid_t pid;
    pid = fork();
    if (pid == 0) {
        execlp("pgrep", "pgrep", "-f", process, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#endif

#endif
}

int
dog_match_wildcard(const char *str, const char *pat)
{
	const char	*s = str;
	const char	*p = pat;
	const char	*star = NULL;
	const char	*ss = NULL;

	while (*s) {
		if (*p == '?' || *p == *s) {
			s++;
			p++;
		} else if (*p == '*') {
			star = p++;
			ss = s;
		} else if (star) {
			p = star + 1;
			s = ++ss;
		} else {
			return (0);
		}
	}

	while (*p == '*')
		p++;

	return (*p == '\0');
}

static int
dog_match_filename(const char *entry_name, const char *pattern)
{
	if (!strchr(pattern, '*') && !strchr(pattern, '?'))
		return (strcmp(entry_name, pattern) == 0);

	return (dog_match_wildcard(entry_name, pattern));
}

int
dog_dot_or_dotdot(const char *entry_name)
{
	return (entry_name[0] == '.' &&
	    (entry_name[1] == '\0' ||
	    (entry_name[1] == '.' && entry_name[2] == '\0')));
}

static int
dog_procure_ignore_dir(const char *entry_name, const char *ignore_dir)
{
	if (!ignore_dir)
		return (0);
#ifdef DOG_WINDOWS
	return (_stricmp(entry_name, ignore_dir) == 0);
#else
	return (strcmp(entry_name, ignore_dir) == 0);
#endif
}

static void
dog_add_found_path(const char *path)
{
	if (dogconfig.dog_sef_count < (sizeof(dogconfig.dog_sef_found_list) /
	    sizeof(dogconfig.dog_sef_found_list[0]))) {
		strncpy(dogconfig.dog_sef_found_list[dogconfig.dog_sef_count],
		    path, MAX_SEF_PATH_SIZE);
		dogconfig.dog_sef_found_list[dogconfig.dog_sef_count]
		    [MAX_SEF_PATH_SIZE - 1] = '\0';
		++dogconfig.dog_sef_count;
	}
}

int
dog_sef_fdir(const char *sef_path, const char *sef_name, const char *ignore_dir)
{
	char		 size_path[MAX_SEF_PATH_SIZE];

#ifdef DOG_WINDOWS
	HANDLE		 find_handle;
	char		 sp[DOG_MAX_PATH * 2];
	const char	*entry_name;
	WIN32_FIND_DATA	 find_data;

	if (sef_path[strlen(sef_path) - 1] == __PATH_CHR_SEP_WIN32) {
		snprintf(sp, sizeof(sp), "%s*", sef_path);
	} else {
		snprintf(sp, sizeof(sp), "%s%s*", sef_path,
		    __PATH_STR_SEP_WIN32);
	}

	find_handle = FindFirstFile(sp, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return (0);

	do {
		entry_name = find_data.cFileName;
		if (dog_dot_or_dotdot(entry_name))
			continue;

		__set_path_sep(size_path, sizeof(size_path), sef_path,
		    entry_name);

		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (dog_procure_ignore_dir(entry_name, ignore_dir))
				continue;

			if (dog_sef_fdir(size_path, sef_name, ignore_dir)) {
				FindClose(find_handle);
				return (1);
			}
		} else {
			if (dog_match_filename(entry_name, sef_name)) {
				dog_add_found_path(size_path);
				FindClose(find_handle);
				return (1);
			}
		}
	} while (FindNextFile(find_handle, &find_data));

	FindClose(find_handle);
#else
	DIR		*open_dir;
	struct dirent	*item;
	struct stat	 statbuf;
	const char	*entry_name;
	int		 is_dir, is_reg;

	open_dir = opendir(sef_path);
	if (!open_dir)
		return (0);

	while ((item = readdir(open_dir)) != NULL) {
		entry_name = item->d_name;

		if (dog_dot_or_dotdot(entry_name))
			continue;

		__set_path_sep(size_path, sizeof(size_path), sef_path,
		    entry_name);

		if (stat(size_path, &statbuf) == -1) {
			if (lstat(size_path, &statbuf) == -1)
				continue;
		}

		is_dir = S_ISDIR(statbuf.st_mode);
		is_reg = S_ISREG(statbuf.st_mode);

		if (is_dir) {
			if (dog_procure_ignore_dir(entry_name, ignore_dir))
				continue;

			if (S_ISLNK(statbuf.st_mode)) {
				/* Option: follow or skip symlink */
				/* continue; Skip symlink */
			}

			if (dog_sef_fdir(size_path, sef_name, ignore_dir)) {
				closedir(open_dir);
				return (1);
			}
		} else if (is_reg) {
			if (dog_match_filename(entry_name, sef_name)) {
				dog_add_found_path(size_path);
				closedir(open_dir);
				return (1);
			}
		}
	}

	closedir(open_dir);
#endif

	return (0);
}

static void
__toml_add_directory_path(FILE *toml_file, int *first, const char *path)
{
	if (!*first)
		fprintf(toml_file, ",\n   ");
	else {
		fprintf(toml_file, "\n   ");
		*first = 0;
	}

	fprintf(toml_file, "\"%s\"", path);
}

static void
dog_check_compiler_options(int *compatibility, int *optimized_lt)
{
	char	 command[DOG_PATH_MAX * 2];
	FILE	*this_proc_fileile;
	char	 log_line[1024];
	int	 found_Z = 0, found_ver = 0;

	if (dir_exists(".watchdogs") == 0)
		MKDIR(".watchdogs");

	if (path_access(".watchdogs/compiler_test.log"))
		remove(".watchdogs/compiler_test.log");

	#ifdef DOG_WINDOWS
	PROCESS_INFORMATION _PROCESS_INFO;
	STARTUPINFO _STARTUPINFO;
	SECURITY_ATTRIBUTES _ATTRIBUTES;
	HANDLE hFile;
	_ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
	_ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));
	_ZERO_MEM_WIN32(&_ATTRIBUTES, sizeof(_ATTRIBUTES));

	_ATTRIBUTES.nLength = sizeof(_ATTRIBUTES);
	_ATTRIBUTES.bInheritHandle = TRUE;

	hFile = CreateFileA(
		".watchdogs\\compiler_test.log",
		GENERIC_WRITE,
		FILE_SHARE_WRITE,
		&_ATTRIBUTES,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile != INVALID_HANDLE_VALUE) {
		_STARTUPINFO.cb = sizeof(_STARTUPINFO);
		_STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;
		_STARTUPINFO.hStdOutput = hFile;
		_STARTUPINFO.hStdError = hFile;

		snprintf(
			command,
			sizeof(command),
			"\"%s\" -N00000000:FF000000 -F000000=FF000000",
			dogconfig.dog_sef_found_list[0]
		);

		if (CreateProcessA(
		NULL,
		command,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&_STARTUPINFO,
		&_PROCESS_INFO))
		{
			WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
			CloseHandle(_PROCESS_INFO.hProcess);
			CloseHandle(_PROCESS_INFO.hThread);
		}

		CloseHandle(hFile);
	}
	#else
	pid_t pid;
	int fd;

	fd = open(".watchdogs/compiler_test.log",
			O_CREAT | O_WRONLY | O_TRUNC,
			0644);

	if (fd >= 0) {
		pid = fork();
		if (pid == 0) {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);

			char *argv[] = {
				dogconfig.dog_sef_found_list[0],
				"-N00000000:FF000000",
				"-F000000=FF000000",
				NULL
			};

			execv(dogconfig.dog_sef_found_list[0], argv);
			_exit(127);
		}

		close(fd);
		waitpid(pid, NULL, 0);
	}
	#endif

	this_proc_fileile = fopen(".watchdogs/compiler_test.log", "r");
	if (this_proc_fileile) {
		while (fgets(log_line, sizeof(log_line),
		    this_proc_fileile) != NULL) {
			if (!found_Z && strfind(log_line, "-Z", true))
				found_Z = 1;
			if (!found_ver && strfind(log_line, "3.10.11", true))
				found_ver = 1;
			if (strfind(log_line, "error while loading shared libraries:", true) ||
				strfind(log_line, "required file not found", true)) {
				dog_printfile(
					".watchdogs/compiler_test.log");
			}
		}

		if (found_Z)
			*compatibility = 1;
		if (found_ver)
			*optimized_lt = 1;

		fclose(this_proc_fileile);
	} else {
		pr_error(stdout, "Failed to open .watchdogs/compiler_test.log");
		minimal_debugging();
	}

	if (path_access(".watchdogs/compiler_test.log"))
		remove(".watchdogs/compiler_test.log");
}

static int
dog_parse_toml_config(void)
{
	FILE		*this_proc_fileile;
	char		 dog_buffer_error[DOG_PATH_MAX];
	toml_table_t	*dog_toml_config;
	toml_table_t	*general_table;

	this_proc_fileile = fopen("watchdogs.toml", "r");
	if (!this_proc_fileile) {
		pr_error(stdout, "Cannot read file %s", "watchdogs.toml");
		minimal_debugging();
		return (0);
	}

	dog_toml_config = toml_parse_file(this_proc_fileile, dog_buffer_error,
	    sizeof(dog_buffer_error));
	fclose(this_proc_fileile);

	if (!dog_toml_config) {
		pr_error(stdout, "Parsing TOML: %s", dog_buffer_error);
		minimal_debugging();
		return (0);
	}

	general_table = toml_table_in(dog_toml_config, "general");
	if (general_table) {
		toml_datum_t	 os_val = toml_string_in(general_table, "os");

		if (os_val.ok) {
			dogconfig.dog_toml_os_type = strdup(os_val.u.s);
			dog_free(os_val.u.s);
		}
	}

	toml_free(dog_toml_config);
	return (1);
}

static int
dog_find_compiler(const char *dog_os_type)
{
	int		 is_windows = (strcmp(dog_os_type, "windows") == 0);
	const char	*compiler_name = is_windows ? "pawncc.exe" : "pawncc";

	if (dog_server_env() == 1)
		return (dog_sef_fdir("pawno", compiler_name, NULL));
	else if (dog_server_env() == 2)
		return (dog_sef_fdir("qawno", compiler_name, NULL));
	else
		return (dog_sef_fdir("pawno", compiler_name, NULL));
}

__attribute__((unused))
static void
__toml_base_subdirs(const char *base_path, FILE *toml_file, int *first)
{
#ifdef DOG_WINDOWS
	WIN32_FIND_DATAA	 find_data;
	HANDLE			 find_handle;
	char			 sp[DOG_MAX_PATH], fp[DOG_MAX_PATH * 2];

	snprintf(sp, sizeof(sp), "%s%s*", base_path, __PATH_STR_SEP_WIN32);
	find_handle = FindFirstFileA(sp, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return;

	do {
		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (dog_dot_or_dotdot(find_data.cFileName))
				continue;

			if (strrchr(base_path, __PATH_CHR_SEP_WIN32) &&
			    strcmp(strrchr(base_path, __PATH_CHR_SEP_WIN32) + 1,
			    find_data.cFileName) == 0)
				continue;

			snprintf(fp, sizeof(fp), "%s%s%s",
			    base_path, __PATH_STR_SEP_LINUX,
			    find_data.cFileName);

			__toml_add_directory_path(toml_file, first, fp);
			__toml_base_subdirs(fp, toml_file, first);
		}
	} while (FindNextFileA(find_handle, &find_data) != 0);

	FindClose(find_handle);
#else
	DIR		*open_dir;
	struct dirent	*item;
	char		 fp[DOG_MAX_PATH * 4];

	open_dir = opendir(base_path);
	if (!open_dir)
		return;

	while ((item = readdir(open_dir)) != NULL) {
		if (item->d_type == DT_DIR) {
			if (dog_dot_or_dotdot(item->d_name))
				continue;

			if (strrchr(base_path, __PATH_CHR_SEP_LINUX) &&
			    strcmp(strrchr(base_path, __PATH_CHR_SEP_LINUX) + 1,
			    item->d_name) == 0)
				continue;

			snprintf(fp, sizeof(fp), "%s%s%s",
			    base_path, __PATH_STR_SEP_LINUX, item->d_name);

			__toml_add_directory_path(toml_file, first, fp);
			__toml_base_subdirs(fp, toml_file, first);
		}
	}

	closedir(open_dir);
#endif
}

int
dog_add_compiler_path(FILE *file, const char *path, int *first_item)
{
	if (path_access(path)) {
		if (!*first_item)
			fprintf(file, ",");
		fprintf(file, "\n      \"%s\"", path);
	} else {
		return (2);
	}
	return (1);
}

int
dog_add_include_paths(FILE *file, int *first_item)
{
	int	 ret = -1, ret2 = -1;

	if (path_access("gamemodes")) {
		ret = 1;
		if (!*first_item)
			fprintf(file, ",");
		fprintf(file, "\n      \"gamemodes/\"");
		*first_item = 0;
	}

	if (dog_server_env() == 1)
		ret2 = dog_add_compiler_path(file, "pawno/include/", first_item);
	else if (dog_server_env() == 2)
		ret2 = dog_add_compiler_path(file, "qawno/include/", first_item);
	else
		ret2 = dog_add_compiler_path(file, "pawno/include/", first_item);

	if (ret != 1 && ret2 != 1)
		return (2);

	return (1);
}

static int	samp_user = -1;

static void
dog_generate_toml_content(FILE *file, const char *dog_os_type,
    int has_gamemodes, int compatible, int optimized_lt, char *sef_path)
{
	char	*p;
	int	 first_item = 1, is_container = 0;

	if (sef_path[0]) {
		char	*extension = strrchr(sef_path, '.');

		if (extension)
			*extension = '\0';
	}

	for (p = sef_path; *p; p++) {
		if (*p == __PATH_CHR_SEP_WIN32)
			*p = __PATH_CHR_SEP_LINUX;
	}

	if (is_running_in_container())
		is_container = 1;
	else if (getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME"))
		is_container = -1;

	fprintf(file, "[general]\n");
	fprintf(file, "   os = \"%s\" # os - windows (wsl/wsl2 supported); linux\n",
	    dog_os_type);

	if (strcmp(dog_os_type, "windows") == 0 && is_container == -1) {
		if (dogconfig.dog_garbage_access[DOG_GARBAGE_WSL_ENV] == DOG_GARBAGE_ZERO)
		{
			pr_info(stdout,
			    "We've detected that you are running Watchdogs in WSL without Docker/Podman - Container.\n"
			    "\tTherefore, we have selected the Windows Ecosystem for Watchdogs,"
			    "\n\tand you can change it in watchdogs.toml.");
			dogconfig.dog_garbage_access[DOG_GARBAGE_WSL_ENV] = DOG_GARBAGE_TRUE;
		}
	}

	if (samp_user == 0) {
		if (!strcmp(dog_os_type, "windows")) {
			fprintf(file, "   binary = \"%s\" # open.mp binary files\n",
			    "omp-server.exe");
		} else if (!strcmp(dog_os_type, "linux")) {
			fprintf(file, "   binary = \"%s\" # open.mp binary files\n",
			    "omp-server");
		}
		fprintf(file, "   config = \"%s\" # open.mp config files\n",
		    "config.json");
		fprintf(file, "   logs = \"%s\" # open.mp log files\n",
		    "log.txt");
	} else {
		if (!strcmp(dog_os_type, "windows")) {
			fprintf(file, "   binary = \"%s\" # sa-mp binary files\n",
			    "samp-server.exe");
		} else if (!strcmp(dog_os_type, "linux")) {
			fprintf(file, "   binary = \"%s\" # sa-mp binary files\n",
			    "samp-server.exe");
		}
		fprintf(file, "   config = \"%s\" # sa-mp config files\n",
		    "server.cfg");
		fprintf(file, "   logs = \"%s\" # sa-mp log files\n",
		    "server_log.txt");
	}

	fprintf(file, "   webhooks = \"DO_HERE\" # discord webhooks\n");
	fprintf(file, "[compiler]\n");

	if (compatible && optimized_lt) {
		fprintf(file,
		    "   option = [\"-Z:+\", \"-d:2\", \"-O:2\", \"-;+\", \"-(+\"] # compiler options\n");
	} else if (compatible) {
		fprintf(file,
		    "   option = [\"-Z:+\", \"-d:2\", \"-;+\", \"-(+\"] # compiler options\n");
	} else {
		fprintf(file,
		    "   option = [\"-d:3\", \"-;+\", \"-(+\"] # compiler options\n");
	}

_tmux:
	fprintf(file, "   includes = [");
	int	 ret = dog_add_include_paths(file, &first_item);

	if (ret != 1)
		fprintf(file, "]\n");
	else
		fprintf(file, "\n   ] # include paths\n");

	if (has_gamemodes && sef_path[0]) {
		fprintf(file, "   input = \"%s.pwn\" # project input\n",
		    sef_path);
		fprintf(file, "   output = \"%s.amx\" # project output\n",
		    sef_path);
	} else {
		if (path_exists("cache/server.p") == 1) {
			fprintf(file,
			    "   input = \"cache/server.p\" # project input\n");
			fprintf(file,
			    "   output = \"cache/server.amx\" # project output\n");
		} else {
			fprintf(file,
			    "   input = \"gamemodes/bare.pwn\" # project input\n");
			fprintf(file,
			    "   output = \"gamemodes/bare.amx\" # project output\n");
		}
	}

	fprintf(file, "[dependencies]\n");
	fprintf(file, "   github_tokens = \"DO_HERE\" # github tokens\n");
	fprintf(file,
	    "   root_patterns = [\"lib\", \"log\", \"root\", "
	    "\"amx\", \"static\", \"dynamic\", \"cfg\", \"config\", \"json\", \"msvcrt\", \"msvcr\", \"msvcp\", \"ucrtbase\"] # root pattern\n");
	fprintf(file, "   packages = [\n"
	    "      \"Y-Less/sscanf?newer\",\n"
	    "      \"samp-incognito/samp-streamer-plugin?newer\"\n"
	    "   ] # package list");
	fprintf(file, "\n");
}

int
dog_toml_configs(void)
{
	int		 find_pawncc = 0, find_gamemodes = 0;
	int		 compatibility = 0, optimized_lt = 0;
	const char	*dog_os_type;
	FILE		*toml_file;
	char		 dog_buffer_error[DOG_PATH_MAX];
	toml_table_t	*dog_toml_config;
	toml_table_t	*dog_toml_depends, *dog_toml_compiler, *general_table;
	toml_array_t	*dog_toml_root_patterns;
	toml_datum_t	 toml_gh_tokens, input_val, output_val;
	toml_datum_t	 bin_val, conf_val, logs_val, webhooks_val;
	size_t		 arr_sz, i;
	char		*expect = NULL;
	char         multi_buf[DOG_MAX_PATH * 4] = { __compiler_rate_zero };
	char iflag[3]          = { __compiler_rate_zero };
	size_t siflag          = sizeof(iflag);
	compilr_with_debugging = false;
	compiler_debugging     = false;
	memset(all_include_paths,
	__compiler_rate_zero, sizeof(all_include_paths));
	dog_free(dogconfig.dog_toml_aio_opt);
	dogconfig.dog_toml_aio_opt = NULL;
	memset(all_include_paths,__compiler_rate_zero, sizeof(all_include_paths));

	dog_os_type = dog_procure_os();

	if (dir_exists("qawno") && dir_exists("components"))
		samp_user = 0;
	else if (dir_exists("pawno") && path_access("server.cfg"))
		samp_user = 1;
	else {
		;
	}

	find_pawncc = dog_find_compiler(dog_os_type);
	if (!find_pawncc) {
		if (strcmp(dog_os_type, "windows") == 0)
			find_pawncc = dog_sef_fdir(".", "pawncc.exe", NULL);
		else
			find_pawncc = dog_sef_fdir(".", "pawncc", NULL);
	}

	find_gamemodes = dog_sef_fdir("gamemodes/", "*.pwn", NULL);
	toml_file = fopen("watchdogs.toml", "r");
	if (toml_file) {
		fclose(toml_file);
	} else {
		if (find_pawncc)
			dog_check_compiler_options(&compatibility, &optimized_lt);

		toml_file = fopen("watchdogs.toml", "w");
		if (!toml_file) {
			pr_error(stdout, "Failed to create watchdogs.toml");
			println(stdout, "   Permission?? - verify first.");
			minimal_debugging();
			exit(1);
		}

		if (find_pawncc)
			dog_generate_toml_content(toml_file, dog_os_type,
			    find_gamemodes, compatibility, optimized_lt,
			    dogconfig.dog_sef_found_list[1]);
		else
			dog_generate_toml_content(toml_file, dog_os_type,
			    find_gamemodes, compatibility, optimized_lt,
			    dogconfig.dog_sef_found_list[0]);
		fclose(toml_file);
	}

	if (!dog_parse_toml_config()) {
		pr_error(stdout, "Failed to parse TOML configuration");
		minimal_debugging();
		return (1);
	}

	FILE	*this_proc_file = fopen("watchdogs.toml", "r");
	dog_toml_config = toml_parse_file(this_proc_file, dog_buffer_error,
	    sizeof(dog_buffer_error));
	if (this_proc_file)
		fclose(this_proc_file);

	if (!dog_toml_config) {
		pr_error(stdout, "failed to parse the watchdogs.toml...: %s",
		    dog_buffer_error);
		minimal_debugging();
		unit_ret_main(NULL);
	}

	dog_toml_depends = toml_table_in(dog_toml_config, "dependencies");
	if (dog_toml_depends) {
		toml_gh_tokens = toml_string_in(dog_toml_depends,
		    "github_tokens");
		if (toml_gh_tokens.ok) {
			dogconfig.dog_toml_github_tokens =
			    strdup(toml_gh_tokens.u.s);
			dog_free(toml_gh_tokens.u.s);
		}

		dog_toml_root_patterns = toml_array_in(dog_toml_depends,
		    "root_patterns");
		if (dog_toml_root_patterns) {
			arr_sz = toml_array_nelem(dog_toml_root_patterns);
			for (i = 0; i < arr_sz; i++) {
				toml_datum_t	 val;

				val = toml_string_at(dog_toml_root_patterns, i);
				if (!val.ok)
					continue;

				if (!expect) {
					expect = dog_realloc(NULL,
					    strlen(val.u.s) + 1);
					if (!expect)
						goto free_val;

					snprintf(expect, strlen(val.u.s) + 1,
					    "%s", val.u.s);
				} else {
					char	*tmp;
					size_t	 old_len = strlen(expect);
					size_t	 new_len = old_len +
					    strlen(val.u.s) + 2;

					tmp = dog_realloc(expect, new_len);
					if (!tmp)
						goto free_val;

					expect = tmp;
					snprintf(expect + old_len,
					    new_len - old_len, " %s",
					    val.u.s);
				}

free_val:
				dogconfig.dog_toml_root_patterns =
				    strdup(expect);
				dog_free(val.u.s);
				val.u.s = NULL;
			}
		}
	}

out:
	dog_toml_compiler = toml_table_in(dog_toml_config, "compiler");
	if (dog_toml_compiler) {
		toml_array_t *toml_include_path = toml_array_in(
			dog_toml_compiler, "includes");
		if (toml_include_path) {
			int toml_array_size;
			toml_array_size =
				toml_array_nelem(toml_include_path);

			for (int i = 0; i < toml_array_size; i++) {
				toml_datum_t path_val = toml_string_at(
					toml_include_path, i);
				if (path_val.ok) {
					char size_path_val[
						DOG_PATH_MAX + 26];
					dog_strip_dot_fns(size_path_val,
						sizeof(size_path_val),
						path_val.u.s);
					if (size_path_val[0] == '\0') {
						dog_free(path_val.u.s);
						continue;
					}
					if (i > 0) {
						size_t cur =
							strlen(
							all_include_paths);
						if (cur <
							sizeof(
							all_include_paths) -
							1) {
							snprintf(
								all_include_paths +
								cur,
								sizeof(
								all_include_paths) -
								cur,
								" ");
						}
					}
					size_t cur = strlen(
						all_include_paths);
					if (cur <
						sizeof(all_include_paths) -
						1) {
						snprintf(
							all_include_paths +
							cur,
							sizeof(
							all_include_paths) -
							cur,
							"-i=%s ",
							size_path_val);
					}
					dog_free(path_val.u.s);
				}
			}
		}

		toml_array_t *option_arr = toml_array_in(dog_toml_compiler,
			"option");
		if (option_arr) {
			expect = NULL;

			size_t toml_array_size;
			toml_array_size = toml_array_nelem(option_arr);

			for (size_t i = 0; i < toml_array_size; i++) {
				toml_datum_t toml_option_value;
				toml_option_value = toml_string_at(
					option_arr, i);
				if (!toml_option_value.ok)
					continue;

				if (strlen(toml_option_value.u.s) >= 2) {
					snprintf(iflag,
						siflag,
						"%.2s",
						toml_option_value.u.s);
				} else {
					strncpy(iflag,
						toml_option_value.u.s,
						siflag -
						1);
				}

				char *_compiler_options =
					toml_option_value.u.s;
				while (*_compiler_options &&
					isspace(*_compiler_options))
					++_compiler_options;

				if (*_compiler_options != '-') {
					pr_color(stdout, DOG_COL_GREEN,
						"[COMPILER]: " DOG_COL_CYAN
						"\"%s\" " DOG_COL_DEFAULT
						"is not valid compiler flag!",
						toml_option_value.u.s);
					sleep(2);
					printf("\n");
					dog_printfile(
						".watchdogs/compiler_test.log");
					dog_free(toml_option_value.u.s);
				}

				if (strfind(toml_option_value.u.s,
					"-d", true) || has_debug > 0)
					compiler_debugging = true;

				size_t old_len = expect ? strlen(expect) :
					0;
				size_t new_len = old_len +
					strlen(toml_option_value.u.s) + 2;

				char *tmp = dog_realloc(expect, new_len);
				if (!tmp) {
					dog_free(expect);
					dog_free(toml_option_value.u.s);
					expect = NULL;
					break;
				}

				expect = tmp;

				if (!old_len)
					snprintf(expect, new_len, "%s",
						toml_option_value.u.s);
				else
					snprintf(expect + old_len,
						new_len - old_len, " %s",
						toml_option_value.u.s);

				dog_free(toml_option_value.u.s);
				toml_option_value.u.s = NULL;
			}

			if (expect) {
				dog_free(dogconfig.dog_toml_aio_opt);
				dogconfig.dog_toml_aio_opt = expect;
				expect = NULL;
			} else {
				dog_free(dogconfig.dog_toml_aio_opt);
				dogconfig.dog_toml_aio_opt = strdup("");
				if (!dogconfig.dog_toml_aio_opt) {
					pr_error(stdout,
						"Memory allocation failed");
				}
			}
		}

		input_val = toml_string_in(dog_toml_compiler, "input");
		if (input_val.ok) {
			dogconfig.dog_toml_proj_input = strdup(input_val.u.s);
			dog_free(input_val.u.s);
		}
		output_val = toml_string_in(dog_toml_compiler, "output");
		if (output_val.ok) {
			dogconfig.dog_toml_proj_output = strdup(output_val.u.s);
			dog_free(output_val.u.s);
		}
	}

	dogconfig.dog_toml_packages = strdup("none none none");

	general_table = toml_table_in(dog_toml_config, "general");
	if (general_table) {
		bin_val = toml_string_in(general_table, "binary");
		if (bin_val.ok) {
			if (samp_user == 1) {
				dogconfig.dog_is_samp = CRC32_TRUE;
				dogconfig.dog_ptr_samp = strdup(bin_val.u.s);
			} else if (samp_user == 0) {
				dogconfig.dog_is_omp = CRC32_TRUE;
				dogconfig.dog_ptr_omp = strdup(bin_val.u.s);
			} else {
				dogconfig.dog_is_samp = CRC32_TRUE;
				dogconfig.dog_ptr_samp = strdup(bin_val.u.s);
			}
			dogconfig.dog_toml_binary = strdup(bin_val.u.s);
			dog_free(bin_val.u.s);
		}
		conf_val = toml_string_in(general_table, "config");
		if (conf_val.ok) {
			dogconfig.dog_toml_config = strdup(conf_val.u.s);
			dog_free(conf_val.u.s);
		}
		logs_val = toml_string_in(general_table, "logs");
		if (logs_val.ok) {
			dogconfig.dog_toml_logs = strdup(logs_val.u.s);
			dog_free(logs_val.u.s);
		}
		webhooks_val = toml_string_in(general_table, "webhooks");
		if (webhooks_val.ok) {
			dogconfig.dog_toml_webhooks = strdup(webhooks_val.u.s);
			dog_free(webhooks_val.u.s);
		}
	}

	toml_free(dog_toml_config);

	for (size_t i = 0; i < sizeof(char_fields) / sizeof(char_fields[0]);
	    i++) {
		char		*field_value = *(field_pointers[i]);
		const char	*field_name = char_fields[i];

		if (field_value == NULL ||
		    strcmp(field_value, CRC32_FALSE) == 0) {
			pr_warning(stdout,
			    "toml key null/crc32 false (%s) detected in key: %s",
			    CRC32_FALSE, field_name);
		}
	}

	return (0);
}

static int _try_mv_without_sudo(const char *src, const char *dest)
{
    if (!src || !dest)
        return (-1);

#ifdef DOG_WINDOWS
    char command[DOG_PATH_MAX * 2];
    PROCESS_INFORMATION _PROCESS_INFO;
    STARTUPINFO _STARTUPINFO;
    DWORD exit_code = 0;

    memset(&_STARTUPINFO, 0, sizeof(_STARTUPINFO));
    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
    memset(&_PROCESS_INFO, 0, sizeof(_PROCESS_INFO));

    snprintf(command, sizeof(command), "move /Y \"%s\" \"%s\"", src, dest);

    if (!
		CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &_STARTUPINFO, &_PROCESS_INFO)) {
        return (-1);
    }

    WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
    GetExitCodeProcess(_PROCESS_INFO.hProcess, &exit_code);

    CloseHandle(_PROCESS_INFO.hProcess);
    CloseHandle(_PROCESS_INFO.hThread);

    return (int)exit_code;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execlp("mv", "mv", "-f", src, dest, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#endif
}

static int __mv_with_sudo(const char *src, const char *dest)
{
    if (!src || !dest)
        return (-1);

#ifdef DOG_WINDOWS
    char command[DOG_PATH_MAX * 2];
    PROCESS_INFORMATION _PROCESS_INFO;
    STARTUPINFO _STARTUPINFO;
    DWORD exit_code = 0;

    memset(&_STARTUPINFO, 0, sizeof(_STARTUPINFO));
    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
    memset(&_PROCESS_INFO, 0, sizeof(_PROCESS_INFO));

    snprintf(command, sizeof(command), "move /Y \"%s\" \"%s\"", src, dest);

    if (!
		CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &_STARTUPINFO, &_PROCESS_INFO)) {
        return (-1);
    }

    WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
    GetExitCodeProcess(_PROCESS_INFO.hProcess, &exit_code);

    CloseHandle(_PROCESS_INFO.hProcess);
    CloseHandle(_PROCESS_INFO.hThread);

    return (int)exit_code;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sudo", "sudo", "mv", "-f", src, dest, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#endif
}

static int _try_cp_without_sudo(const char *src, const char *dest)
{
    if (!src || !dest)
        return (-1);

#ifdef DOG_WINDOWS
    char command[DOG_PATH_MAX * 2];
    PROCESS_INFORMATION _PROCESS_INFO;
    STARTUPINFO _STARTUPINFO;
    DWORD exit_code = 0;

    memset(&_STARTUPINFO, 0, sizeof(_STARTUPINFO));
    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
    memset(&_PROCESS_INFO, 0, sizeof(_PROCESS_INFO));

    snprintf(command, sizeof(command), "xcopy /Y \"%s\" \"%s\"", src, dest);

    if (!
		CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &_STARTUPINFO, &_PROCESS_INFO)) {
        return (-1);
    }

    WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
    GetExitCodeProcess(_PROCESS_INFO.hProcess, &exit_code);

    CloseHandle(_PROCESS_INFO.hProcess);
    CloseHandle(_PROCESS_INFO.hThread);

    return (int)exit_code;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execlp("cp", "cp", "-f", src, dest, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#endif
}

static int __cp_with_sudo(const char *src, const char *dest)
{
    if (!src || !dest)
        return (-1);

#ifdef DOG_WINDOWS
    char command[DOG_PATH_MAX * 2];
    PROCESS_INFORMATION _PROCESS_INFO;
    STARTUPINFO _STARTUPINFO;
    DWORD exit_code = 0;

    memset(&_STARTUPINFO, 0, sizeof(_STARTUPINFO));
    _STARTUPINFO.cb = sizeof(_STARTUPINFO);
    memset(&_PROCESS_INFO, 0, sizeof(_PROCESS_INFO));

    snprintf(command, sizeof(command), "xcopy /Y \"%s\" \"%s\"", src, dest);

    if (!
		CreateProcess(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &_STARTUPINFO, &_PROCESS_INFO)) {
        return (-1);
    }

    WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
    GetExitCodeProcess(_PROCESS_INFO.hProcess, &exit_code);

    CloseHandle(_PROCESS_INFO.hProcess);
    CloseHandle(_PROCESS_INFO.hThread);

    return (int)exit_code;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sudo", "sudo", "cp", "-f", src, dest, NULL);
        _exit(127);
    }
    if (pid < 0)
        return (-1);
    waitpid(pid, NULL, 0);
    return (0);
#endif
}

static int
__dog_sef_safety(const char *c_src, const char *c_dest)
{
	char		 parent[DOG_PATH_MAX];
	struct stat	 st;

#if defined(_DBG_PRINT)
	if (!c_src || !c_dest)
		pr_error(stdout, "src or dest is null");
	if (!*c_src || !*c_dest)
		pr_error(stdout, "src or dest empty");
	if (strlen(c_src) >= DOG_PATH_MAX || strlen(c_dest) >= DOG_PATH_MAX)
		pr_error(stdout, "path too long");
	if (!path_exists(c_src))
		pr_error(stdout, "source does not exist: %s", c_src);
	if (!file_regular(c_src))
		pr_error(stdout, "source is not a regular file: %s", c_src);
	if (path_exists(c_dest) && file_same_file(c_src, c_dest))
		pr_info(stdout, "source and dest are the same file: %s", c_src);
	if (ensure_parent_dir(parent, sizeof(parent), c_dest))
		pr_error(stdout, "cannot determine parent open_dir of dest");
	if (stat(parent, &st))
		pr_error(stdout, "destination open_dir does not exist: %s",
		    parent);
	if (!S_ISDIR(st.st_mode))
		pr_error(stdout, "destination parent is not a open_dir: %s",
		    parent);
#endif

	return (1);
}

static void
__dog_sef_set_permissions(const char *c_dest)
{
	if (CHMOD_FULL(c_dest)) {
		pr_warning(stdout, "chmod failed: %s (errno=%d %s)",
		    c_dest, errno, strerror(errno));
	}
	return;
}

int
dog_sef_wmv(const char *c_src, const char *c_dest)
{
	int	 ret, mv_ret;

	ret = __dog_sef_safety(c_src, c_dest);
	if (ret != 1)
		return (1);

	static int	 is_not_superuser = 1;
#ifdef DOG_LINUX
	static int	 su_check = 1;

	if (su_check != 1)
		goto skip;

	pid_t pid;
	int fd;

	fd = open("/dev/null", O_WRONLY);
	if (fd >= 0) {
		pid = fork();
		if (pid == 0) {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
			execlp("sudo", "sudo", "-n", "true", NULL);
			_exit(127);
		}
		close(fd);
		if (pid > 0 && waitpid(pid, &su_check, 0) > 0 && WIFEXITED(su_check))
			su_check = WEXITSTATUS(su_check);
		else
			su_check = -1;
	}

	if (su_check < 1)
		--is_not_superuser;
#endif
skip:
	if (is_not_superuser == 1) {
		mv_ret = _try_mv_without_sudo(c_src, c_dest);
		if (!mv_ret) {
			__dog_sef_set_permissions(c_dest);
			pr_info(stdout, "moved without sudo: '%s' -> '%s'",
			    c_src, c_dest);
			return (0);
		}
	} else {
		mv_ret = __mv_with_sudo(c_src, c_dest);
		if (!mv_ret) {
			__dog_sef_set_permissions(c_dest);
			pr_info(stdout, "moved with sudo: '%s' -> '%s'",
			    c_src, c_dest);
			return (0);
		}
	}

	return (1);
}

int
dog_sef_wcopy(const char *c_src, const char *c_dest)
{
	int	 ret, cp_ret;

	ret = __dog_sef_safety(c_src, c_dest);
	if (ret != 1)
		return (1);

	static int	 is_not_superuser = 1;
#ifdef DOG_LINUX
	static int	 su_check = 1;

	if (su_check != 1)
		goto skip;

	pid_t pid;
	int fd;

	fd = open("/dev/null", O_WRONLY);
	if (fd >= 0) {
		pid = fork();
		if (pid == 0) {
			dup2(fd, STDOUT_FILENO);
			dup2(fd, STDERR_FILENO);
			close(fd);
			execlp("sudo", "sudo", "-n", "true", NULL);
			_exit(127);
		}
		close(fd);
		if (pid > 0 && waitpid(pid, &su_check, 0) > 0 && WIFEXITED(su_check))
			su_check = WEXITSTATUS(su_check);
		else
			su_check = -1;
	}

	if (su_check < 1)
		--is_not_superuser;
#endif
skip:
	if (is_not_superuser == 1) {
		cp_ret = _try_cp_without_sudo(c_src, c_dest);
		if (!cp_ret) {
			__dog_sef_set_permissions(c_dest);
			pr_info(stdout, "copying without sudo: '%s' -> '%s'",
			    c_src, c_dest);
			return (0);
		}
	} else {
		cp_ret = __cp_with_sudo(c_src, c_dest);
		if (!cp_ret) {
			__dog_sef_set_permissions(c_dest);
			pr_info(stdout, "copying with sudo: '%s' -> '%s'",
			    c_src, c_dest);
			return (0);
		}
	}

	return (1);
}
