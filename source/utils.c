/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdarg.h>
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
#include  <sys/wait.h>
#endif

#include  "units.h"
#include  "library.h"
#include  "crypto.h"
#include  "debug.h"
#include  "compiler.h"
#include  "utils.h"

const char	*unit_command_list[] = {
	"help", "exit", "sha1", "sha256", "crc32", "djb2", "config",
	"replicate", "gamemode", "pawncc", "debug",
	"compile", "running", "compiles", "stop", "restart",
	"tracker", "compress", "send"
};

const size_t	 unit_command_len = sizeof(unit_command_list) /
									sizeof(unit_command_list[0]);

WatchdogConfig	 dogconfig = {
	.dog_garbage_access     = { DOG_GARBAGE_ZERO },
	.dog_os_type            = CRC32_FALSE,
	.dog_is_samp            = CRC32_FALSE,
	.dog_is_omp             = CRC32_FALSE,
	.dog_ptr_samp           = NULL,
	.dog_ptr_omp            = NULL,
	.dog_sef_count          = RATE_SEF_EMPTY,
	.dog_sef_found_list     = { { RATE_SEF_EMPTY } },
	.dog_toml_os_type       = NULL,
	.dog_toml_server_binary = NULL,
	.dog_toml_server_config = NULL,
	.dog_toml_server_logs   = NULL,
	.dog_toml_all_flags     = NULL,
	.dog_toml_root_patterns = NULL,
	.dog_toml_packages      = NULL,
	.dog_toml_proj_compile  = NULL,
	.dog_toml_proj_input    = NULL,
	.dog_toml_proj_output   = NULL,
	.dog_toml_webhooks      = NULL
};

const char	*toml_char_field[] = {
	"dog_toml_os_type", "dog_toml_server_binary",
	"dog_toml_server_config", "dog_toml_server_logs",
	"dog_toml_all_flags", "dog_toml_root_patterns",
	"dog_toml_packages", "dog_toml_proj_compile",
	"dog_toml_proj_input", "dog_toml_proj_output",
	"dog_toml_webhooks"
};

char		**toml_pointers[] = {
	&dogconfig.dog_toml_os_type,
	&dogconfig.dog_toml_server_binary,
	&dogconfig.dog_toml_server_config,
	&dogconfig.dog_toml_server_logs,
	&dogconfig.dog_toml_all_flags,
	&dogconfig.dog_toml_root_patterns,
	&dogconfig.dog_toml_packages,
	&dogconfig.dog_toml_proj_compile,
	&dogconfig.dog_toml_proj_input,
	&dogconfig.dog_toml_proj_output,
	&dogconfig.dog_toml_webhooks
};

void dog_sef_path_revert(void)
{
	size_t	 i, rate_sef_entries;

	rate_sef_entries = sizeof(dogconfig.dog_sef_found_list) /
	    sizeof(dogconfig.dog_sef_found_list[0]);

	for (i = 0; i < rate_sef_entries; i++)
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
#endif

void * dog_malloc(size_t size)
{
	void	*ptr = malloc(size);

	if (!ptr) {
		fprintf(stderr, "malloc failed: %zu bytes\n", size);
		minimal_debugging();
	}
	return (ptr);
}

void * dog_calloc(size_t count, size_t size)
{
	void	*ptr = calloc(count, size);

	if (!ptr) {
		fprintf(stderr,
		    "calloc failed: %zu elements x %zu bytes\n", count, size);
		return (NULL);
	}
	return (ptr);
}

void * dog_realloc(void *ptr, size_t size)
{
	void	*new_ptr = (ptr ? realloc(ptr, size) : malloc(size));

	if (!new_ptr) {
		fprintf(stderr, "realloc failed: %zu bytes\n", size);
		return (NULL);
	}
	return (new_ptr);
}

void dog_free(void *ptr)
{
	char      *ret = NULL;
	ret            = ptr;
	if (ret != NULL)
		{
			 free(ret);
		}
	else {
		pr_warning(stdout, "dog_free fail.. %s is null!", ret);
		minimal_debugging();
	}
}

void path_sep_to_posix(char *path)
{
	char	*pos;

	for (pos = path; *pos; pos++) {
		if (*pos == _PATH_CHR_SEP_WIN32)
			*pos = _PATH_CHR_SEP_POSIX;
	}
}

const char *look_up_sep(const char *sep_path) {
    if (!sep_path)
        return NULL;

    const char *_l = strrchr(sep_path, _PATH_CHR_SEP_POSIX);
    const char *_w = strrchr(sep_path, _PATH_CHR_SEP_WIN32);

    if (_l && _w)
        return (_l > _w) ? _l : _w;
    else
        return (_l ? _l : _w);
}

const char * fetch_filename(const char *path)
{
	const char	*p = look_up_sep(path);

	return (p ? p + 1 : path);
}

char * fetch_basename(const char *path)
{
    const char *filename = fetch_filename(path);

    char *base = strdup(filename);
    if (!base)
        return NULL;

    char *dot = strrchr(base, '.');

    if (dot) {
        *dot = '\0';
    }

    return base;
}

char * dog_procure_pwd(void)
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

int dog_mkdir_recursive(const char *path)
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

void print_restore_color(void) {

	print(BKG_DEFAULT)    ;

	print(DOG_COL_RESET)  ;

	print(DOG_COL_DEFAULT);

	return;
}

void println(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	print_restore_color();

	vfprintf(stream, format, args);
	
	print("\n");
	
	print_restore_color();

	va_end(args);
	fflush(stream);
}

void printf_colour(FILE *stream, const char *color, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	print_restore_color();

	printf("%s", color);
	
	vfprintf(stream, format, args);
	
	print_restore_color();

	va_end(args);
	fflush(stream);
}

void printf_info(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	print_restore_color();

	print(DOG_COL_YELLOW);
	print("@ Hey!");
	
	print_restore_color();

	print(": ");

	vfprintf(stream, format, args);
	
	print("\n");
	
	va_end(args);
	
	fflush(stream);
}

void printf_warning(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	print_restore_color();

	print(DOG_COL_GREEN);
	print("@ Uh-oh!");
	
	print_restore_color();

	print(": ");

	vfprintf(stream, format, args);
	
	print("\n");
	
	va_end(args);
	
	fflush(stream);
}

void printf_error(FILE *stream, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	print_restore_color();

	print(DOG_COL_RED);
	print("@ Oops!");
	
	print_restore_color();

	print(": ");

	vfprintf(stream, format, args);
	
	print("\n");
	
	va_end(args);
	
	fflush(stream);
}

#ifdef DOG_WINDOWS

static time_t filetime_to_time_t(const FILETIME *ft) {
	const uint64_t
		EPOCH_DIFF = 116444736000000000ULL;
	uint64_t
		v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
	if (v < EPOCH_DIFF) return (time_t)0;
	return (time_t)((v - EPOCH_DIFF) / 10000000ULL);
}
#endif

int dog_portable_stat(const char *path, dog_portable_stat_t *out) {
	if (!path || !out) return (-1); 
	memset(out, 0, sizeof(*out));

#ifdef DOG_WINDOWS
	wchar_t wpath[DOG_MAX_PATH];
	int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);

	if (len == 0 || len > DOG_MAX_PATH) {
			if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, DOG_MAX_PATH)) return (-1);
	} else {
			MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, DOG_MAX_PATH);
	}

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &fad)) {
			return (-1);
	}

	uint64_t size = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
	out->st_size = size;

	out->st_latime = filetime_to_time_t(&fad.ftLastAccessTime);
	out->st_lmtime = filetime_to_time_t(&fad.ftLastWriteTime);
	out->st_mctime = filetime_to_time_t(&fad.ftCreationTime);

	DWORD attrs = fad.dwFileAttributes;

	if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			out->st_mode |= S_IFDIR;
	} else {
			out->st_mode |= S_IFREG;
	}

	if (attrs & FILE_ATTRIBUTE_READONLY) {
			out->st_mode |= S_IRUSR;
	} else {
			out->st_mode |= (S_IRUSR | S_IWUSR);
	}

	const char *extension = strrchr(path, '.');
	if (extension && (_stricmp(extension, ".exe") == 0 ||
				_stricmp(extension, ".bat") == 0 ||
				_stricmp(extension, ".com") == 0)) {
			out->st_mode |= S_IXUSR;
	}

	out->st_ino = 0;
	out->st_dev = 0;

	return (0);
#else
	struct stat st;
	if (stat(path, &st) != 0) return (-1);

	out->st_size = (uint64_t)st.st_size;
	out->st_ino  = (uint64_t)st.st_ino;
	out->st_dev  = (uint64_t)st.st_dev;
	out->st_mode = (unsigned int)st.st_mode;
	out->st_latime = st.st_atime;
	out->st_lmtime = st.st_mtime;
	out->st_mctime = st.st_ctime;

	return (0);
#endif
}

void unit_show_dog(void) {

	print("\n                         \\/%%#z.     \\/.%%#z./   /,z#%%\\/\n");
	print("                         \\X##k      /X#####X\\   /d##X/\n");
	print("                         \\888\\   /888/ \\888\\   /888/\n");
	print("                        `v88;  ;88v'   `v88;  ;88v'\n");
	print("                         \\77xx77/       \\77xx77/\n");
	print("                        `::::'         `::::'\n\n");
	#ifndef DOG_ANDROID
	print("---------------------------------------------------------------------------------------------");
	print("\n");
	print("                                            -----------------------------------------        ");
	print("\n");
	print("      ;printf(\"Hello, World\")               |  plugin installer                     |        ");
	print("\n");
	print("                                            v          v                            |        ");
	print("\n");
	print("pawncc | compile | gamemode | running | compiles | replicate | restart | stop       |        ");
	print("\n");
	print("  ^        ^          ^          ^ -----------------------       ^         ^        v        ");
	print("\n");
	print("  -------  ---------   ------------------                |       |         | compile n run  ");
	print("\n");
	print("        v          |                    |                |       v         --------          ");
	print("\n");
	print(" install compiler  v                    v                v  restart server        |          ");
	print("\n");
	print("               compile gamemode  install gamemode  running server             stop server    ");
	print("\n");
	print("---------------------------------------------------------------------------------------------");
	print("\n");
	#endif
	print("Use \"help\" for more.");
	print("\n");
}

void unit_show_help(const char *command)
{
	if (strlen(command) == 0) {
	#ifndef DOG_ANDROID
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
	#else
	static const char *help_text =
	"Usage: help <command> | help sha1\n\n"
	"Commands:\n"
	"  exit - exit from watchdogs | "
	"Usage: \"exit\" " DOG_COL_CYAN "\n\t ^ Just type 'exit' and you're outta here!" DOG_COL_DEFAULT "\n"
	"  sha1 - generate sha1 hash | "
	"Usage: \"sha1\" | [<args>] " DOG_COL_CYAN "\n\t ^  Get that SHA1 hash for your text." DOG_COL_DEFAULT "\n"
	"  sha256 - generate sha256 hash | "
	"Usage: \"sha256\" | [<args>] " DOG_COL_CYAN "\n\t ^  Get that SHA256 hash for your text." DOG_COL_DEFAULT "\n"
	"  crc32 - generate crc32 checksum | "
	"Usage: \"crc32\" | [<args>] " DOG_COL_CYAN "\n\t ^  Quick CRC32 checksum generation." DOG_COL_DEFAULT "\n"
	"  djb2 - generate djb2 hash file | "
	"Usage: \"djb2\" | [<args>] " DOG_COL_CYAN "\n\t ^  djb2 hashing for your files." DOG_COL_DEFAULT "\n"
	"  config - re-write watchdogs.toml | "
	"Usage: \"config\" " DOG_COL_CYAN "\n\t ^  Reset your config file to default settings." DOG_COL_DEFAULT "\n"
	"  replicate - dependency installer | "
	"Usage: \"replicate\" " DOG_COL_CYAN "\n\t ^  Downloads & Install Our Dependencies." DOG_COL_DEFAULT "\n"
	"  gamemode - download SA-MP gamemode | "
	"Usage: \"gamemode\" " DOG_COL_CYAN "\n\t ^  Grab some SA-MP gamemodes quickly." DOG_COL_DEFAULT "\n"
	"  pawncc - download SA-MP pawncc | "
	"Usage: \"pawncc\" " DOG_COL_CYAN "\n\t ^  Get the Pawn Compiler for SA-MP/open.mp." DOG_COL_DEFAULT "\n"
	"  debug - debugging & logging server logs | "
	"Usage: \"debug\" " DOG_COL_CYAN "\n\t ^  Keep an eye on your server logs." DOG_COL_DEFAULT "\n"
	"  compile - compile your project | "
	"Usage: \"compile\" | [<args>] " DOG_COL_CYAN "\n\t ^  Turn your code into something runnable!" DOG_COL_DEFAULT "\n"
	"  running - running your project | "
	"Usage: \"running\" | [<args>] " DOG_COL_CYAN "\n\t ^  Fire up your project and see it in action." DOG_COL_DEFAULT "\n"
	"  compiles - compile and running your project | "
	"Usage: \"compiles\" | [<args>] " DOG_COL_CYAN "\n\t ^  Two-in-one: compile then run immediately!." DOG_COL_DEFAULT "\n"
	"  stop - stopped server tasks | "
	"Usage: \"stop\" " DOG_COL_CYAN "\n\t ^  Halt everything! Stop your server tasks." DOG_COL_DEFAULT "\n"
	"  restart - re-start server tasks | "
	"Usage: \"restart\" " DOG_COL_CYAN "\n\t ^  Fresh start! Restart your server." DOG_COL_DEFAULT "\n"
	"  tracker - account tracking | "
	"Usage: \"tracker\" | [<args>] " DOG_COL_CYAN "\n\t ^  Track accounts across platforms." DOG_COL_DEFAULT "\n"
	"  compress - create a compressed archive | "
	"Usage: \"compress <input> <output>\" " DOG_COL_CYAN "\n\t ^  Generates a compressed file (e.g., .zip/.tar.gz) from the specified source." DOG_COL_DEFAULT "\n"
	"  send - send file to Discord channel via webhook | "
	"Usage: \"send <files>\" " DOG_COL_CYAN "\n\t ^  Uploads a file directly to a Discord channel using a webhook." DOG_COL_DEFAULT "\n";
	#endif
	fwrite(help_text, 1, strlen(help_text), stdout);
	return;
	}

	if (strcmp(command, "exit") == 0) {
		print("exit: exit from watchdogs. | Usage: \"exit\"\n\tJust type 'exit' and you're outta here!");
		print("\n");
	} else if (strcmp(command, "sha1") == 0) {
		print("sha1: generate sha1. | Usage: \"sha1\" | [<args>]\n\tGet that SHA1 hash for your text.");
		print("\n");
	} else if (strcmp(command, "sha256") == 0) {
		print("sha256: generate sha256. | Usage: \"sha256\" | [<args>]\n\tGet that SHA256 hash for your text.");
		print("\n");
	} else if (strcmp(command, "crc32") == 0) {
		print("crc32: generate crc32. | Usage: \"crc32\" | [<args>]\n\tQuick CRC32 checksum generation.");
		print("\n");
	} else if (strcmp(command, "djb2") == 0) {
		print("djb2: generate djb2 hash file. | Usage: \"djb2\" | [<args>]\n\tdjb2 hashing for your files.");
		print("\n");
	} else if (strcmp(command, "config") == 0) {
		print("config: re-write watchdogs.toml. | Usage: \"config\"\n\tReset your config file to default settings.");
		print("\n");
	} else if (strcmp(command, "replicate") == 0) {
		print("replicate: dependency installer. | Usage: \"replicate\"\n\tDownloads & Install Our Dependencies.");
		print("\n");
	} else if (strcmp(command, "gamemode") == 0) {
		print("gamemode: download SA-MP gamemode. | Usage: \"gamemode\"\n\tGrab some SA-MP gamemodes quickly.");
		print("\n");
	} else if (strcmp(command, "pawncc") == 0) {
		print("pawncc: download SA-MP pawncc. | Usage: \"pawncc\"\n\tGet the Pawn Compiler for SA-MP/open.mp.");
		print("\n");
	} else if (strcmp(command, "debug") == 0) {
		print("debug: debugging & logging server debug. | Usage: \"debug\"\n\tKeep an eye on your server logs.");
		print("\n");
	} else if (strcmp(command, "compile") == 0) {
		print("compile: compile your project. | Usage: \"compile\" | [<args>]\n\tTurn your code into something runnable!");
		print("\n");
	} else if (strcmp(command, "running") == 0) {
		print("running: running your project. | Usage: \"running\" | [<args>]\n\tFire up your project and see it in action.");
		print("\n");
	} else if (strcmp(command, "compiles") == 0) {
		print("compiles: compile and running your project. | Usage: \"compiles\" | [<args>]\n\tTwo-in-one: compile then run immediately!");
		print("\n");
	} else if (strcmp(command, "stop") == 0) {
		print("stop: stopped server task. | Usage: \"stop\"\n\tHalt everything! Stop your server tasks.");
		print("\n");
	} else if (strcmp(command, "restart") == 0) {
		print("restart: re-start server task. | Usage: \"restart\"\n\tFresh start! Restart your server.");
		print("\n");
	} else if (strcmp(command, "tracker") == 0) {
		print("tracker: account tracking. | Usage: \"tracker\" | [<args>]\n\tTrack accounts across platforms.");
		print("\n");
	} else if (strcmp(command, "compress") == 0) {
		print("compress: create a compressed archive from a file or folder. | "
		    "Usage: \"compress <input> <output>\"\n\tGenerates a compressed file (e.g., .zip/.tar.gz) from the specified source.");
		print("\n");
	} else if (strcmp(command, "send") == 0) {
		print("send: send file to Discord channel via webhook. | "
		    "Usage: \"send <files>\"\n\tUploads a file directly to a Discord channel using a webhook.");
		print("\n");
	} else {
		print("help can't found for: '");
		pr_color(stdout, DOG_COL_YELLOW, "%s", command);
		print("'\n     Oops! That command doesn't exist. Try 'help' to see available commands.\n");
	}
	return;
}

void compiler_show_tip(void) {

	fputs(DOG_COL_YELLOW "Options:\n" DOG_COL_DEFAULT, stdout);

	#ifndef DOG_ANDROID
		fputs(
		"  " DOG_COL_CYAN "-w" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--watchdogs" DOG_COL_DEFAULT
		"   Show detailed output         - compile detail\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-d" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--debug" DOG_COL_DEFAULT
		"       Enable debug level 2         - full debugging\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-p" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--prolix" DOG_COL_DEFAULT
		"      Verbose mode                 - processing detail\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-a" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--assembler" DOG_COL_DEFAULT
		"   Generate assembler file      - assembler ouput\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-s" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--compact" DOG_COL_DEFAULT
		"     Compact encoding compression - resize output\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-c" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--compat" DOG_COL_DEFAULT
		"      Compatibility mode           - path sep compat\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-f" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--fast" DOG_COL_DEFAULT
		"        Fast compile-sime            - no optimize\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-n" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--clean" DOG_COL_DEFAULT
		"       Remove '.amx' after compile  - remover output\n", stdout);
	#else
		fputs(
		"  " DOG_COL_CYAN "-w" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--watchdogs" DOG_COL_DEFAULT
		"  Show detailed output        - compile detail\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-d" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--debug" DOG_COL_DEFAULT
		"      Enable debug level 2        - full debugging\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-p" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--prolix" DOG_COL_DEFAULT
		"     Verbose mode                - processing detail\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-a" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--assembler" DOG_COL_DEFAULT
		"  Generate assembler file     - assembler ouput\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-s" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--compact" DOG_COL_DEFAULT
		"    Compact encoding compression- resize output\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-c" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--compat" DOG_COL_DEFAULT
		"     Compatibility mode          - path sep compat\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-f" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--fast" DOG_COL_DEFAULT
		"       Fast compile-sime           - no optimize\n", stdout);

		fputs(
		"  " DOG_COL_CYAN "-n" DOG_COL_DEFAULT ", " DOG_COL_CYAN "--clean" DOG_COL_DEFAULT
		"      Remove '.amx' after compile - remover output\n", stdout);
	#endif
}

int dog_exec_command(char *const argv[])
{
    char size_command[DOG_MAX_PATH] = {0};
    char *p;
    int high_acces_command = 0, ret;

    if (argv == NULL || argv[0] == NULL)
        return (-1);

    for (int i = 0; argv[i] != NULL; i++) {
        for (p = argv[i]; *p; p++) {
            if (*p == ';' || *p == '`' || *p == '$' ||
				*p == '(' || *p == ')' || *p == '\n')
                return (-1);
        }
        if (i > 0) {
            strncat(size_command, " ",
				sizeof(size_command) - strlen(size_command) - 1);
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
            print("\n");
            print("\t=== HIGH ACCESS SECURITY WARNING ===\n");
            print("\tYou are about to run commands with ROOT privileges!\n");
            print("\tThis can DAMAGE your system if used incorrectly.\n");
            print("\t* Always verify commands before pressing Enter...\n");
            print("\t===========================================\n");
			printf("\t[ %s ]", size_command);
            print("\n\n");
			dogconfig.dog_garbage_access[DOG_GARBAGE_CMD_WARN] = DOG_GARBAGE_TRUE;
        }
    }

    ret = -1;
skip:
    ret = system(size_command);
    return (ret);
}

int
fetch_server_env(void)
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
			    strstr(line, "/containerd/")) {
				fclose(fp);
				return (1);
			}
		}
		fclose(fp);
	}

	return (0);
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

int dog_console_title(const char *title)
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

	slash = strchr(src, _PATH_CHR_SEP_POSIX);
#ifdef DOG_WINDOWS
	if (!slash)
		slash = strchr(src, _PATH_CHR_SEP_WIN32);
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

bool dog_strcase(const char *text, const char *pattern)
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

bool strend(const char *str, const char *suffix, bool nocase)
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

bool strfind(const char *text, const char *pattern, bool nocase)
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

int match_wildcard(const char *str, const char *pat)
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

static void configure_path_sep(char *out, size_t out_sz, const char *open_dir,
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
			snprintf(out, out_sz, "%s%s%s", open_dir, _PATH_SEP_SYSTEM,
			    entry_name);
	}

	out[out_sz - 1] = '\0';
}

__PURE__
static int __command_suggest(const char *s1, const char *s2)
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

const char * dog_find_near_command(const char *command, const char *commands[],
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

void set_default_access(const char *c_dest)
{
	if (_set_full_access(c_dest)) {
		pr_warning(stdout, "chmod failed: %s (errno=%d %s)",
		    c_dest, errno, strerror(errno));
	}
	return;
}

static const char * dog_procure_os(void)
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
static int
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

    snprintf(command, sizeof(command),
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

static int
dog_match_filename(const char *entry_name, const char *pattern)
{
	if (!strchr(pattern, '*') && !strchr(pattern, '?'))
		return (strcmp(entry_name, pattern) == 0);

	return (match_wildcard(entry_name, pattern));
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

static void dog_ensure_found_path(const char *path)
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

int dog_find_path(const char *sef_path, const char *sef_name, const char *ignore_dir)
{
	char		 size_path[MAX_SEF_PATH_SIZE];

#ifdef DOG_WINDOWS
	HANDLE		 find_handle;
	char		 sp[DOG_MAX_PATH * 2];
	const char	*entry_name;
	WIN32_FIND_DATA	 find_data;

	if (sef_path[strlen(sef_path) - 1] == _PATH_CHR_SEP_WIN32) {
		snprintf(sp, sizeof(sp), "%s*", sef_path);
	} else {
		snprintf(sp, sizeof(sp), "%s%s*", sef_path,
		    _PATH_STR_SEP_WIN32);
	}

	find_handle = FindFirstFile(sp, &find_data);
	if (find_handle == INVALID_HANDLE_VALUE)
		return (0);

	do {
		entry_name = find_data.cFileName;
		if (dog_dot_or_dotdot(entry_name))
			continue;

		configure_path_sep(size_path, sizeof(size_path), sef_path,
		    entry_name);

		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (dog_procure_ignore_dir(entry_name, ignore_dir))
				continue;

			if (dog_find_path(size_path, sef_name, ignore_dir)) {
				FindClose(find_handle);
				return (1);
			}
		} else {
			if (dog_match_filename(entry_name, sef_name)) {
				dog_ensure_found_path(size_path);
				FindClose(find_handle);
				return (1);
			}
		}
	} while (FindNextFile(find_handle, &find_data));

	FindClose(find_handle);
#else
	DIR *dir;
    struct dirent *entry;

    dir = opendir(sef_path);
    if (!dir) return 0;

    while ((entry = readdir(dir)) != NULL) {
        if (dog_dot_or_dotdot(entry->d_name)) continue;

        configure_path_sep(size_path,
			sizeof(size_path), sef_path, entry->d_name);

        #ifdef DT_DIR
			int is_dir = (entry->d_type == DT_DIR);
			int is_reg = (entry->d_type == DT_REG);
        #else
			struct stat st;
			if (stat(size_path, &st) == -1) continue;
			int is_dir = S_ISDIR(st.st_mode);
			int is_reg = S_ISREG(st.st_mode);
        #endif

        if (is_dir) {
            if (dog_procure_ignore_dir(entry->d_name, ignore_dir)) continue;
            if (dog_find_path(size_path, sef_name, ignore_dir)) {
                closedir(dir);
                return 1;
            }
        } else if (is_reg) {
            if (dog_match_filename(entry->d_name, sef_name)) {
                dog_ensure_found_path(size_path);
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
#endif

	return (0);
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

		snprintf(command, sizeof(command),
			"\"%s\" -N00000000:FF000000 -F000000=FF000000",
			dogconfig.dog_sef_found_list[0]
		);

		if (CreateProcessA(
		NULL, command, NULL, NULL, TRUE, compiler_rate_zero, NULL, NULL,
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
	toml_table_t	*dog_toml_server_config;
	toml_table_t	*general_table;

	this_proc_fileile = fopen("watchdogs.toml", "r");
	if (!this_proc_fileile) {
		pr_error(stdout, "Cannot read file %s", "watchdogs.toml");
		minimal_debugging();
		return (0);
	}

	dog_toml_server_config = toml_parse_file(this_proc_fileile, dog_buffer_error,
	    sizeof(dog_buffer_error));
	fclose(this_proc_fileile);

	if (!dog_toml_server_config) {
		pr_error(stdout, "Parsing TOML: %s", dog_buffer_error);
		minimal_debugging();
		return (0);
	}

	general_table = toml_table_in(dog_toml_server_config, "general");
	if (general_table) {
		toml_datum_t	 os_val = toml_string_in(general_table, "os");

		if (os_val.ok) {
			if (dogconfig.dog_toml_os_type == NULL ||
				strcmp(dogconfig.dog_toml_os_type, os_val.u.s) != 0) {
				if (dogconfig.dog_toml_os_type)
				{
					free(dogconfig.dog_toml_os_type);
					dogconfig.dog_toml_os_type = NULL;
				}
				dogconfig.dog_toml_os_type = strdup(os_val.u.s);
			}
			dog_free(os_val.u.s);
		}
	}

	toml_free(dog_toml_server_config);
	return (1);
}

static int
dog_find_compiler(const char *dog_os_type)
{
	int		 is_windows = (strcmp(dog_os_type, "windows") == 0);
	const char	*compiler_name = is_windows ? "pawncc.exe" : "pawncc";

	if (fetch_server_env() == 1)
		return (dog_find_path("pawno", compiler_name, NULL));
	else if (fetch_server_env() == 2)
		return (dog_find_path("qawno", compiler_name, NULL));
	else
		return (dog_find_path("pawno", compiler_name, NULL));
}

static int	samp_server_stat = -1;

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
		if (*p == _PATH_CHR_SEP_WIN32)
			*p = _PATH_CHR_SEP_POSIX;
	}

	if (is_running_in_container())
		is_container = 1;
	else if (getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME"))
		is_container = -1;

    fprintf(file, "# @general settings\n");
	fprintf(file, "[general]\n");
	fprintf(file, "   os = \"%s\" # os - windows (wsl/wsl2 supported) : linux\n",
	    dog_os_type);

	if (strcmp(dog_os_type, "windows") == 0 && is_container == -1) {
		if (dogconfig.dog_garbage_access[DOG_GARBAGE_WSL_ENV] == DOG_GARBAGE_ZERO) {
			pr_info(stdout,
			"We've detected that you are running Watchdogs in WSL without Docker/Podman - Container.\n"
			"\tTherefore, we have selected the Windows Ecosystem for Watchdogs,"
			"\n\tand you can change it in watchdogs.toml.");
			dogconfig.dog_garbage_access[DOG_GARBAGE_WSL_ENV] = DOG_GARBAGE_TRUE;
		}
	}

	if (samp_server_stat == 0) {
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
			    "samp03svr");
		}
		fprintf(file, "   config = \"%s\" # sa-mp config files\n",
		    "server.cfg");
		fprintf(file, "   logs = \"%s\" # sa-mp log files\n",
		    "server_log.txt");
	}
    fprintf(file, "   webhooks = \"DO_HERE\" # discord webhooks\n");

    fprintf(file, "# @compiler settings\n");
    fprintf(file, "[compiler]\n");

	fprintf(file, "   compile = \"localhost\" # compile mode - localhost : hosting\n");

	if (compatible && optimized_lt) {
		fprintf(file,
		    "   option = [\"-Z:+\", \"-d:2\", \"-O:2\"] # compiler options\n");
	} else if (compatible) {
		fprintf(file,
		    "   option = [\"-Z:+\", \"-d:2\"] # compiler options\n");
	} else {
		fprintf(file,
		    "   option = [\"-d:3\"] # compiler options\n");
	}

	fprintf(file, "   includes = [\"gamemodes/\"," \
		"\"pawno/include/\", \"qawno/include/\"] # compiler include path\n");

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

    fprintf(file, "# @dependencies settings\n");
	fprintf(file, "[dependencies]\n");
	fprintf(file, "   github_tokens = \"DO_HERE\" # github tokens\n");
	fprintf(file,
	    "   root_patterns = [\"lib\", \"log\", \"root\", " \
	    "\"amx\", \"static\", \"dynamic\", \"cfg\", \"config\", " \
		"\"json\", \"msvcrt\", \"msvcr\", \"msvcp\", \"ucrtbase\"] # root pattern\n");
	fprintf(file, "   packages = [\n"
	    "      \"Y-Less/sscanf?newer\",\n"
	    "      \"samp-incognito/samp-streamer-plugin?newer\"\n"
	    "   ] # package list");
	fprintf(file, "\n");
}

#ifndef DOG_WINDOWS

static int
_run_command_vfork(char *const argv[])
{
    pid_t pid;
    int status;

    pid = vfork();

    if (pid < 0)
        return -1;

    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0)
        return -1;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return -1;
}

#endif

#ifdef DOG_WINDOWS

static int
_run_windows_command(const char *command)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    DWORD exit_code = 0;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    memset(&pi, 0, sizeof(pi));

    if (!CreateProcess(
            NULL,
            (char *)command,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &si,
            &pi))
    {
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
}

#endif

static int
validate_src_dest(const char *c_src, const char *c_dest)
{
    char parent[DOG_PATH_MAX];
    struct stat st;

    if (!c_src || !c_dest)
        return 0;

    if (!*c_src || !*c_dest)
        return 0;

    if (strlen(c_src) >= DOG_PATH_MAX || strlen(c_dest) >= DOG_PATH_MAX)
        return 0;

    if (!path_exists(c_src))
        return 0;

    if (!file_regular(c_src))
        return 0;

    if (path_exists(c_dest) && file_same_file(c_src, c_dest))
        return 0;

    if (ensure_parent_dir(parent, sizeof(parent), c_dest))
        return 0;

    if (stat(parent, &st))
        return 0;

    if (!S_ISDIR(st.st_mode))
        return 0;

    return 1;
}

static int
detect_super_mode(void)
{
#ifdef DOG_LINUX

    char *sudo_check[] = {
        "sh", "-c",
        "'sudo",
		"echo",
		"superuser",
		">",
		"/dev/null",
		"2>&1'",
        NULL
    };

    if (dog_exec_command(sudo_check) == 0)
        return 1;

    char *run0_check[] = {
        "sh", "-c",
        "'run0",
		"echo",
		"superuser",
		">",
		"/dev/null",
		"2>&1'",
        NULL
    };

    if (dog_exec_command(run0_check) == 0)
        return 2;

#endif

    return 0;
}

static int
_run_file_operation(
    const char *operation,
    const char *src,
    const char *dest,
    int super_mode)
{
    if (!src || !dest)
        return -1;

#ifdef DOG_WINDOWS

    char command[DOG_PATH_MAX * 2];

    if (strcmp(operation, "mv") == 0) {
        snprintf(command, sizeof(command),
            "move /Y \"%s\" \"%s\"", src, dest);
    } else {
        snprintf(command, sizeof(command),
            "xcopy /Y \"%s\" \"%s\"", src, dest);
    }

    return _run_windows_command(command);

#else

    if (super_mode == 0) {
        char *argv[] = {
            (char *)operation,
            "-f",
            (char *)src,
            (char *)dest,
            NULL
        };

        return _run_command_vfork(argv);
    }

    if (super_mode == 1) {
        char *argv[] = {
            "sudo",
            (char *)operation,
            "-f",
            (char *)src,
            (char *)dest,
            NULL
        };

        return _run_command_vfork(argv);
    }

    if (super_mode == 2) {
        char *argv[] = {
            "run0",
            (char *)operation,
            "-f",
            (char *)src,
            (char *)dest,
            NULL
        };

        return _run_command_vfork(argv);
    }

    return -1;

#endif
}

int
dog_sef_wmv(const char *c_src, const char *c_dest)
{
    if (!validate_src_dest(c_src, c_dest))
        return 1;

    int super_mode = detect_super_mode();

    int ret = _run_file_operation("mv", c_src, c_dest, super_mode);

    if (ret == 0) {
        set_default_access(c_dest);
		if (super_mode == 1)
        	pr_info(stdout, "moved (with sudo): '%s' -> '%s'", c_src, c_dest);
		else if (super_mode == 2)
			pr_info(stdout, "moved (with run0): '%s' -> '%s'", c_src, c_dest);
		else
			pr_info(stdout, "moved: '%s' -> '%s'", c_src, c_dest);
        return 0;
    }

    pr_error(stdout, "failed to move: '%s' -> '%s'", c_src, c_dest);
    return 1;
}

int
dog_sef_wcopy(const char *c_src, const char *c_dest)
{
    if (!validate_src_dest(c_src, c_dest))
        return 1;

    int super_mode = detect_super_mode();

    int ret = _run_file_operation("cp", c_src, c_dest, super_mode);

    if (ret == 0) {
        set_default_access(c_dest);
		if (super_mode == 1)
        	pr_info(stdout, "copied (with sudo): '%s' -> '%s'", c_src, c_dest);
		else if (super_mode == 2)
			pr_info(stdout, "copied (with run0): '%s' -> '%s'", c_src, c_dest);
		else
			pr_info(stdout, "copied: '%s' -> '%s'", c_src, c_dest);
        return 0;
    }

    pr_error(stdout, "failed to copy: '%s' -> '%s'", c_src, c_dest);
    return 1;
}

int
dog_configure_toml(void)
{
	int		 find_pawncc = 0, find_gamemodes = 0;
	int		 compatibility = 0, optimized_lt = 0;
	const char	*dog_os_type;
	FILE		*toml_file;
	char		 dog_buffer_error[DOG_PATH_MAX];
	toml_table_t	*dog_toml_server_config;
	toml_table_t	*dog_toml_depends, *dog_toml_compiler, *general_table;
	toml_array_t	*dog_toml_root_patterns;
	toml_datum_t	 toml_compile_for, toml_gh_tokens, input_val, output_val;
	toml_datum_t	 bin_val, conf_val, logs_val, webhooks_val;
	size_t		 arr_sz;
	char		*expect = NULL;
	char        *buffer = NULL;
	size_t       buffer_size = 0;
	size_t       buffer_len = 0;
	char         multi_buf[DOG_MAX_PATH * 4] = { compiler_rate_zero };
	char iflag[3]          = { compiler_rate_zero };
	size_t siflag          = sizeof(iflag);
	compilr_with_debugging = false;
	compiler_debugging     = false;
	if (compiler_full_includes)
		{
			free(compiler_full_includes);
			compiler_full_includes      = NULL;
		}

	dog_os_type = dog_procure_os();

	if (dir_exists("qawno") && dir_exists("components"))
		samp_server_stat = 0;
	else if (dir_exists("pawno") && path_access("server.cfg"))
		samp_server_stat = 1;
	else {
		;
	}

	find_pawncc = dog_find_compiler(dog_os_type);
	if (!find_pawncc) {
		if (strcmp(dog_os_type, "windows") == 0)
			find_pawncc = dog_find_path(".", "pawncc.exe", NULL);
		else
			find_pawncc = dog_find_path(".", "pawncc", NULL);
	}

	find_gamemodes = dog_find_path("gamemodes/", "*.pwn", NULL);
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
	dog_toml_server_config = toml_parse_file(this_proc_file, dog_buffer_error,
	    sizeof(dog_buffer_error));
	if (this_proc_file)
		fclose(this_proc_file);

	if (!dog_toml_server_config) {
		pr_error(stdout, "failed to parse the watchdogs.toml...: %s",
		    dog_buffer_error);
		minimal_debugging();
		unit_ret_main(NULL);
	}

	dog_toml_depends = toml_table_in(dog_toml_server_config, "dependencies");
	if (dog_toml_depends) {
		toml_gh_tokens = toml_string_in(dog_toml_depends,
		    "github_tokens");
		if (toml_gh_tokens.ok) {
			if (dogconfig.dog_toml_github_tokens == NULL ||
				strcmp(dogconfig.dog_toml_github_tokens, toml_gh_tokens.u.s) != 0) {
				if (dogconfig.dog_toml_github_tokens)
					{
						dog_free(dogconfig.dog_toml_github_tokens);
						dogconfig.dog_toml_github_tokens = NULL;
					}
				dogconfig.dog_toml_github_tokens =
					strdup(toml_gh_tokens.u.s);
			}
			dog_free(toml_gh_tokens.u.s);
		}

		dog_toml_root_patterns = toml_array_in(dog_toml_depends,
		    "root_patterns");
		if (dog_toml_root_patterns) {
			arr_sz = toml_array_nelem(dog_toml_root_patterns);
			for (int i = 0; i < arr_sz; i++) {
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
					if (!tmp) {
						goto cleanup;
					}

					expect = tmp;
					snprintf(expect + old_len,
					    new_len - old_len, " %s",
					    val.u.s);
				}

free_val:
				if (dogconfig.dog_toml_root_patterns)
				{
					dog_free(dogconfig.dog_toml_root_patterns);
					dogconfig.dog_toml_root_patterns = NULL;
				}
				dogconfig.dog_toml_root_patterns =
					strdup(expect);
				dog_free(val.u.s);
				val.u.s = NULL;
				goto out;
cleanup:
			    dog_free(expect);
			    expect = NULL;
			}
		}
	}

out:
	dog_toml_compiler = toml_table_in(dog_toml_server_config, "compiler");
	if (dog_toml_compiler) {
		toml_compile_for = toml_string_in(dog_toml_compiler, "compile");
		if (toml_compile_for.ok) {
			if (dogconfig.dog_toml_proj_compile == NULL ||
				strcmp(dogconfig.dog_toml_proj_compile, toml_compile_for.u.s) != 0) {
				if (dogconfig.dog_toml_proj_compile)
					{
						dog_free(dogconfig.dog_toml_proj_compile);
						dogconfig.dog_toml_proj_compile = NULL;
					}
				dogconfig.dog_toml_proj_compile = strdup(toml_compile_for.u.s);
			}
			dog_free(toml_compile_for.u.s);
		}

		toml_array_t *toml_include_path = toml_array_in(dog_toml_compiler, "includes");
		if (toml_include_path) {
			int toml_array_size = toml_array_nelem(toml_include_path);
			
			for (int i = 0; i < toml_array_size; i++) {
				toml_datum_t path_val = toml_string_at(toml_include_path, i);
				if (!path_val.ok) continue;
				
				char clean_path[DOG_PATH_MAX];
				dog_strip_dot_fns(clean_path, sizeof(clean_path), path_val.u.s);
				
				if (clean_path[0] == '\0') {
					dog_free(path_val.u.s);
					continue;
				}
				
				char formatted[DOG_PATH_MAX + 10];
				int formatted_len = snprintf(formatted, sizeof(formatted), 
											"-i=%s ", clean_path);
				
				if (buffer_len + formatted_len + 1 > buffer_size) {
					size_t new_size = buffer_size ? buffer_size * 2 : 256;
					while (new_size < buffer_len + formatted_len + 1) new_size *= 2;
					
					char *new_buffer = realloc(buffer, new_size);
					if (!new_buffer) {
						pr_error(stdout, "Failed to allocate memory for include paths");
						dog_free(path_val.u.s);
						free(buffer);
						goto skip_;
					}
					buffer = new_buffer;
					buffer_size = new_size;
				}
				
				if (buffer_len > 0) {
					buffer[buffer_len] = ' ';
					buffer_len++;
				}
				
				memcpy(buffer + buffer_len, formatted, formatted_len);
				buffer_len += formatted_len;
				buffer[buffer_len] = '\0';
				
				dog_free(path_val.u.s);
			}
			
			compiler_full_includes = buffer;
		}

	skip_:
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
					static bool not_valid_flag = false;
					if (not_valid_flag == false) {
						not_valid_flag = true;
						pr_color(stdout, DOG_COL_GREEN,
							"[COMPILER]: " DOG_COL_CYAN
							"\"%s\" " DOG_COL_DEFAULT
							"is not valid compiler flag!\n",
							toml_option_value.u.s);
						sleep(2);
						printf("\n");
						dog_printfile(
							".watchdogs/compiler_test.log");
					}
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

				if (toml_option_value.u.s)
				{
					free(toml_option_value.u.s);
					toml_option_value = NULL;
				}
			}

			if (expect) {
				if (dogconfig.dog_toml_all_flags)
					{
						dog_free(dogconfig.dog_toml_all_flags);
						dogconfig.dog_toml_all_flags = NULL;
					}
				dogconfig.dog_toml_all_flags = expect;
				expect = NULL;
			} else {
				if (dogconfig.dog_toml_all_flags)
					{
						dog_free(dogconfig.dog_toml_all_flags);
						dogconfig.dog_toml_all_flags = NULL;
					}
				dogconfig.dog_toml_all_flags = strdup("");
				if (!dogconfig.dog_toml_all_flags) {
					pr_error(stdout,
						"Memory allocation failed");
				}
			}
		}

		input_val = toml_string_in(dog_toml_compiler, "input");
		if (input_val.ok) {
			if (dogconfig.dog_toml_proj_input == NULL ||
				strcmp(dogconfig.dog_toml_proj_input, input_val.u.s) != 0) {
				if (dogconfig.dog_toml_proj_input)
					{
						dog_free(dogconfig.dog_toml_proj_input);
						dogconfig.dog_toml_proj_input = NULL;
					}
				dogconfig.dog_toml_proj_input = strdup(input_val.u.s);
			}
			dog_free(input_val.u.s);
		}
		output_val = toml_string_in(dog_toml_compiler, "output");
		if (output_val.ok) {
			if (dogconfig.dog_toml_proj_output == NULL ||
				strcmp(dogconfig.dog_toml_proj_output, output_val.u.s) != 0) {
				if (dogconfig.dog_toml_proj_output)
					{
						dog_free(dogconfig.dog_toml_proj_output);
						dogconfig.dog_toml_proj_output = NULL;
					}
				dogconfig.dog_toml_proj_output = strdup(output_val.u.s);
			}
			dog_free(output_val.u.s);
		}
	}

	if (dogconfig.dog_toml_packages == NULL ||
		strcmp(dogconfig.dog_toml_packages, "none none none") != 0) {
		if (dogconfig.dog_toml_packages) {
			free(dogconfig.dog_toml_packages);
			dogconfig.dog_toml_packages = NULL;
		}
		dogconfig.dog_toml_packages = strdup("none none none");
	}

	general_table = toml_table_in(dog_toml_server_config, "general");
	if (general_table) {
		bin_val = toml_string_in(general_table, "binary");
		if (bin_val.ok) {
			if (dogconfig.dog_ptr_samp)
				{
					free(dogconfig.dog_ptr_samp);
					dogconfig.dog_ptr_samp = NULL;
				}
			if (dogconfig.dog_ptr_omp)
				{
					free(dogconfig.dog_ptr_omp);
					dogconfig.dog_ptr_omp = NULL;
				}
			if (samp_server_stat == 1) {
				if (dogconfig.dog_is_samp == NULL ||
					strcmp(dogconfig.dog_is_samp, CRC32_TRUE) != 0) {
					dogconfig.dog_is_samp = CRC32_TRUE;
				}
				if (dogconfig.dog_ptr_samp == NULL ||
					strcmp(dogconfig.dog_ptr_samp, bin_val.u.s) != 0) {
					dogconfig.dog_ptr_samp = strdup(bin_val.u.s);
				}
			} else if (samp_server_stat == 0) {
				if (dogconfig.dog_is_omp == NULL ||
					strcmp(dogconfig.dog_is_omp, CRC32_TRUE) != 0) {
					dogconfig.dog_is_omp = CRC32_TRUE;
				}
				if (dogconfig.dog_ptr_omp == NULL ||
					strcmp(dogconfig.dog_ptr_omp, bin_val.u.s) != 0) {
					dogconfig.dog_ptr_omp = strdup(bin_val.u.s);
				}
			} else {
				if (dogconfig.dog_is_samp == NULL ||
					strcmp(dogconfig.dog_is_samp, CRC32_TRUE) != 0) {
					dogconfig.dog_is_samp = CRC32_TRUE;
				}
				if (dogconfig.dog_ptr_samp == NULL ||
					strcmp(dogconfig.dog_ptr_samp, bin_val.u.s) != 0) {
					dogconfig.dog_ptr_samp = strdup(bin_val.u.s);
				}
			}
			if (dogconfig.dog_toml_server_binary == NULL ||
				strcmp(dogconfig.dog_toml_server_binary, bin_val.u.s) != 0) {
				if (dogconfig.dog_toml_server_binary)
					{
						dog_free(dogconfig.dog_toml_server_binary);
						dogconfig.dog_toml_server_binary = NULL;
					}
				dogconfig.dog_toml_server_binary = strdup(bin_val.u.s);
			}
			dog_free(bin_val.u.s);
		}
		conf_val = toml_string_in(general_table, "config");
		if (conf_val.ok) {
			if (dogconfig.dog_toml_server_config == NULL ||
				strcmp(dogconfig.dog_toml_server_config, conf_val.u.s) != 0) {
				if (dogconfig.dog_toml_server_config)
					{
						dog_free(dogconfig.dog_toml_server_config);
						dogconfig.dog_toml_server_config = NULL;
					}
				dogconfig.dog_toml_server_config = strdup(conf_val.u.s);
			}
			dog_free(conf_val.u.s);
		}
		logs_val = toml_string_in(general_table, "logs");
		if (logs_val.ok) {
			if (dogconfig.dog_toml_server_logs == NULL ||
				strcmp(dogconfig.dog_toml_server_logs, logs_val.u.s) != 0) {
				if (dogconfig.dog_toml_server_logs)
					{
						dog_free(dogconfig.dog_toml_server_logs);
						dogconfig.dog_toml_server_logs = NULL;
					}
				dogconfig.dog_toml_server_logs = strdup(logs_val.u.s);
			}
			dog_free(logs_val.u.s);
		}
		webhooks_val = toml_string_in(general_table, "webhooks");
		if (webhooks_val.ok) {
			if (dogconfig.dog_toml_webhooks == NULL ||
				strcmp(dogconfig.dog_toml_webhooks, webhooks_val.u.s) != 0) {
				if (dogconfig.dog_toml_webhooks)
					{
						dog_free(dogconfig.dog_toml_webhooks);
						dogconfig.dog_toml_webhooks = NULL;
					}
				dogconfig.dog_toml_webhooks = strdup(webhooks_val.u.s);
			}
			dog_free(webhooks_val.u.s);
		}
	}

	toml_free(dog_toml_server_config);

	for (size_t i = 0; i < sizeof(toml_char_field) / sizeof(toml_char_field[0]);
	    i++) {
		char		*field_value = *(toml_pointers[i]);
		const char	*field_name = toml_char_field[i];

		if (field_value == NULL ||
		    strcmp(field_value, CRC32_FALSE) == 0) {
			pr_warning(stdout,
			    "toml key null/crc32 false (%s) detected in key: %s * do not set to empty!.",
			    CRC32_FALSE, field_name);
		}
	}

	return (0);
}
