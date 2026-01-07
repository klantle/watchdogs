/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#ifdef DOG_LINUX
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "utils.h"
#include "units.h"
#include "extra.h"
#include "library.h"
#include "debug.h"
#include "crypto.h"
#include "cause.h"
#include "compiler.h"


#ifndef DOG_WINDOWS
const char *usr_paths[] = {
	"/usr/local/lib",
	"/usr/local/lib32",
	"/data/data/com.termux/files/usr/lib",
	"/data/data/com.termux/files/usr/local/lib",
	"/data/data/com.termux/arm64/usr/lib",
	"/data/data/com.termux/arm32/usr/lib",
	"/data/data/com.termux/amd32/usr/lib",
	"/data/data/com.termux/amd64/usr/lib"
};
#endif

const CompilerOption object_opt[] = {
	{ __FLAG_DEBUG,		COMPILER_WHITESPACE "-d:2" COMPILER_WHITESPACE, 5 },
	{ __FLAG_ASSEMBLER,	COMPILER_WHITESPACE "-a"   COMPILER_WHITESPACE, 4 },
	{ __FLAG_COMPAT,	COMPILER_WHITESPACE "-Z:+" COMPILER_WHITESPACE, 5 },
	{ __FLAG_PROLIX,	COMPILER_WHITESPACE "-v:2" COMPILER_WHITESPACE, 5 },
	{ __FLAG_COMPACT,	COMPILER_WHITESPACE "-C:+" COMPILER_WHITESPACE, 5 },
	{ __FLAG_TIME,		COMPILER_WHITESPACE "-d:3" COMPILER_WHITESPACE, 5 },
	{ 0,			NULL, 0 }
};

static struct timespec pre_start = { __compiler_rate_zero };
static struct timespec post_end = { __compiler_rate_zero };
static double timer_rate_compile;

static io_compilers dog_compiler_sys = { __compiler_rate_zero };

static FILE *this_proc_file = NULL;

static bool compiler_retrying = false;

bool compilr_with_debugging = false;
bool compiler_debugging = false;

static bool has_detailed = false;
bool has_debug = false;
static bool has_clean = false;
static bool has_assembler = false;
static bool has_compat = false;
static bool has_prolix = false;
static bool has_compact = false;
static bool has_time = false;

static char command[DOG_PATH_MAX + 258] = { __compiler_rate_zero };
static char parsing[DOG_PATH_MAX] = { __compiler_rate_zero };
static char temp[DOG_PATH_MAX] = { __compiler_rate_zero };
static char multi_buf[DOG_MAX_PATH * 4] = { __compiler_rate_zero };
static char this_path_include[DOG_PATH_MAX] = { __compiler_rate_zero };
static char buf[DOG_MAX_PATH] = { __compiler_rate_zero };
static char compiler_input[DOG_MAX_PATH * 2] = { __compiler_rate_zero };
static char compiler_extra_options[DOG_PATH_MAX] = { __compiler_rate_zero };
static char compiler_pawncc_path[DOG_PATH_MAX] = { __compiler_rate_zero };
static char compiler_proj_path[DOG_PATH_MAX] = { __compiler_rate_zero };
static char dog_buffer_error[DOG_PATH_MAX] = { __compiler_rate_zero };

static char *compiler_size_last_slash = NULL;
static char *compiler_back_slash = NULL;
static char *size_include_extra = NULL;
static char *procure_string_pos = NULL;
static char *pointer_signalA = NULL;
static char *platform = NULL;
static char *proj_targets = NULL;
static char *compiler_unix_token = NULL;
static char *dog_compiler_unix_args[DOG_MAX_PATH + 256] = { NULL };

#ifdef DOG_WINDOWS
static PROCESS_INFORMATION _PROCESS_INFO;
static STARTUPINFO _STARTUPINFO;
static SECURITY_ATTRIBUTES _ATTRIBUTES;
#endif

char include_aio_path[DOG_PATH_MAX * 2] = { __compiler_rate_zero };

void exec_compiler(void) {

	if (dir_exists(".watchdogs") == 0)
		MKDIR(".watchdogs");

	/* variable reset */
	/* boolean */
	has_detailed = false;
	has_debug = false;
	has_clean = false;
	has_assembler = false;
	has_compat = false;
	has_prolix = false;
	has_compact = false;
	compiler_retrying = false;

	/* variable of processing file */
	this_proc_file = NULL;

	/* memory reset */
	memset(parsing, __compiler_rate_zero, sizeof(parsing));
	memset(temp, __compiler_rate_zero, sizeof(temp));
	memset(multi_buf, __compiler_rate_zero, sizeof(multi_buf));
	memset(command, __compiler_rate_zero, sizeof(command));
	memset(this_path_include, __compiler_rate_zero,
	    sizeof(this_path_include));
	memset(buf, __compiler_rate_zero, sizeof(buf));
	memset(compiler_input, __compiler_rate_zero, sizeof(compiler_input));
	memset(compiler_extra_options, __compiler_rate_zero,
	    sizeof(compiler_extra_options));
	memset(compiler_pawncc_path, __compiler_rate_zero,
	    sizeof(compiler_pawncc_path));

	/* pointer */
	compiler_size_last_slash = NULL;
	compiler_back_slash = NULL;
	size_include_extra = NULL;
	procure_string_pos = NULL;
	pointer_signalA = NULL;
	proj_targets = NULL;
	platform = NULL;
	
	if (proj_targets) dog_free(proj_targets);
	if (pointer_signalA) dog_free(pointer_signalA);

	/* pointer array */
	memset(dog_compiler_unix_args, __compiler_rate_zero,
	    sizeof(dog_compiler_unix_args));

	/* unix compiler token */
	compiler_unix_token = NULL;

	/* compiler cleaning - loc: tree/compiler.h */
	compiler_memory_clean();

	/* Verify */
	dog_toml_configs();
}

int
dog_exec_compiler(const char *args, const char *compile_args_val,
    const char *second_arg, const char *four_arg, const char *five_arg,
    const char *six_arg, const char *seven_arg, const char *eight_arg,
    const char *nine_arg, const char *ten_arg)
{
	io_compilers compiler_field;
	io_compilers *kevlar_compiler = &compiler_field;

	const char *argv_buf[] = {second_arg,four_arg,five_arg,six_arg,seven_arg,eight_arg,nine_arg,ten_arg
	};

#ifdef DOG_LINUX
	/* exporting - setenv session */
	if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0) { /* WSL/WSL -> Skip */
		goto _skip_path;
	}

	static int rate_export_path = 0;

	if (rate_export_path < 1) {
		size_t counts = sizeof(usr_paths) / sizeof(usr_paths[0]);

		char _newpath[DOG_MAX_PATH], _so_path[DOG_PATH_MAX];
		const char *_old = getenv("LD_LIBRARY_PATH");
		if (!_old)
			_old = "";

		snprintf(_newpath, sizeof(_newpath), "%s", _old);

		for (size_t i = 0; i < counts; i++) {
			snprintf(_so_path, sizeof(_so_path), "%s/libpawnc.so",
			    usr_paths[i]);
			if (path_exists(_so_path) != 0) {
				if (_newpath[0] != '\0')
					strncat(_newpath, ":",
					    sizeof(_newpath) -
					    strlen(_newpath) - 1);
				strncat(_newpath, usr_paths[i],
				    sizeof(_newpath) - strlen(_newpath) - 1);
			}
		}

		if (_newpath[0] != '\0') {
			setenv("LD_LIBRARY_PATH", _newpath, 1);
			pr_info(stdout, "LD_LIBRARY_PATH set to: %s",
			    _newpath);
			++rate_export_path;
		} else {
			pr_warning(stdout,
			    "libpawnc.so not found in any target path..");
		}
	}
#endif

	/* pawncc pointer */
	char *_pointer_pawncc = NULL;
	/* rate if exists */
	int __rate_pawncc_exists = 0;
_skip_path:
	if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0) {
		/* windows : pawncc.exe - wsl/wsl2 - native / msys2 */
		_pointer_pawncc = "pawncc.exe";
	} else if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_LINUX) == 0) {
		/* linux : pawncc */
		_pointer_pawncc = "pawncc";
	}

	if (dir_exists("pawno") != 0 && dir_exists("qawno") != 0) {
		__rate_pawncc_exists = dog_sef_fdir("pawno", _pointer_pawncc,
		    NULL);
		if (__rate_pawncc_exists) {
			;
		} else {
			__rate_pawncc_exists = dog_sef_fdir("qawno",
			    _pointer_pawncc, NULL);
			if (__rate_pawncc_exists < 1) {
				__rate_pawncc_exists = dog_sef_fdir(".",
				    _pointer_pawncc, NULL);
			}
		}
	} else if (dir_exists("pawno") != 0) {
		__rate_pawncc_exists = dog_sef_fdir("pawno", _pointer_pawncc,
		    NULL);
		if (__rate_pawncc_exists) {
			;
		} else {
			__rate_pawncc_exists = dog_sef_fdir(".",
			    _pointer_pawncc, NULL);
		}
	} else if (dir_exists("qawno") != 0) {
		__rate_pawncc_exists = dog_sef_fdir("qawno", _pointer_pawncc,
		    NULL);
		if (__rate_pawncc_exists) {
			;
		} else {
			__rate_pawncc_exists = dog_sef_fdir(".",
			    _pointer_pawncc, NULL);
		}
	} else {
		__rate_pawncc_exists = dog_sef_fdir(".", _pointer_pawncc,
		    NULL);
	}

	if (__rate_pawncc_exists) {
		if (dogconfig.dog_sef_found_list[0]) {
			snprintf(compiler_pawncc_path,
			    sizeof(compiler_pawncc_path), "%s",
			    dogconfig.dog_sef_found_list[0]);
		} else {
			pr_error(stdout, "Compiler path not found");
			goto compiler_end;
		}

		if (path_exists(".watchdogs/compiler_test.log") == 1) {
			remove(".watchdogs/compiler_test.log");
		}

		#ifdef DOG_WINDOWS
		HANDLE hFile;
		ZeroMemory(&_STARTUPINFO, sizeof(_STARTUPINFO));
		ZeroMemory(&_PROCESS_INFO, sizeof(_PROCESS_INFO));
		ZeroMemory(&_ATTRIBUTES, sizeof(_ATTRIBUTES));

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
				compiler_pawncc_path
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
					&_PROCESS_INFO)) {
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
					compiler_pawncc_path,
					"-N00000000:FF000000",
					"-F000000=FF000000",
					NULL
				};

				execv(compiler_pawncc_path, argv);
				_exit(127);
			}

			close(fd);
			waitpid(pid, NULL, 0);
		}
		#endif

		this_proc_file = fopen(".watchdogs/compiler_test.log", "r");
		if (!this_proc_file) {
			pr_error(stdout,
			    "Failed to open .watchdogs/compiler_test.log");
			minimal_debugging();
		}

		/* checking options */
		for (int i = 0; i < __compiler_rate_aio_repo; ++i) {
			if (argv_buf[i] != NULL) {
				/*
				 * compile . -w
				 * compile . --detailed
				 * compile . --watchdogs
				 */
				if (strfind(argv_buf[i], "--detailed", true) ||
				    strfind(argv_buf[i], "--watchdogs", true) ||
				    strfind(argv_buf[i], "-w", true))
					has_detailed = true;
				/*
				 * compile . -d
				 * compile . --debug
				 */
				if (strfind(argv_buf[i], "--debug", true) ||
				    strfind(argv_buf[i], "-d", true))
					has_debug = true;
				/*
				 * compile . -c
				 * compile . --clean
				 */
				if (strfind(argv_buf[i], "--clean", true) ||
				    strfind(argv_buf[i], "-n", true))
					has_clean = true;
				/*
				 * compile . -a
				 * compile . --assembler
				 */
				if (strfind(argv_buf[i], "--assembler", true) ||
				    strfind(argv_buf[i], "-a", true))
					has_assembler = true;
				/*
				 * compile . -c
				 * compile . --compat
				 */
				if (strfind(argv_buf[i], "--compat", true) ||
				    strfind(argv_buf[i], "-c", true))
					has_compat = true;
				/*
				 * compile . -p
				 * compile . --prolix
				 */
				if (strfind(argv_buf[i], "--prolix", true) ||
				    strfind(argv_buf[i], "-p", true))
					has_prolix = true;
				/*
				 * compile . -s
				 * compile . --compact
				 */
				if (strfind(argv_buf[i], "--compact", true) ||
				    strfind(argv_buf[i], "-s", true))
					has_compact = true;
			}
			if (strfind(argv_buf[i], "--fast", true) ||
			    strfind(argv_buf[i], "-f", true))
				has_time = true;
		}

_compiler_retrying:
		if (compiler_retrying) {
			has_compat = true;
			has_compact = true;
			has_time = true;
			has_detailed = true;
		}

		{
			/*
				* __FLAG_DEBUG	 = 0x01  (1 << 0) = 00000001
				* __FLAG_ASSEMBLER = 0x02  (1 << 1) = 00000010
				* __FLAG_COMPAT    = 0x04  (1 << 2) = 00000100
				* __FLAG_PROLIX    = 0x08  (1 << 3) = 00001000
				* __FLAG_COMPACT   = 0x10  (1 << 4) = 00010000
				* __FLAG_TIME      = 0x20  (1 << 5) = 00100000
				*/
			unsigned int flags_map = 0;

			if (has_debug)
				/* bit 0 = 1 (0x01) */
				flags_map |= __FLAG_DEBUG;

			if (has_assembler)
				/* bit 1 = 1 (0x03) */
				flags_map |= __FLAG_ASSEMBLER;

			if (has_compat)
				/* bit 2 = 1 (0x07) */
				flags_map |= __FLAG_COMPAT;

			if (has_prolix)
				/* bit 3 = 1 (0x0F) */
				flags_map |= __FLAG_PROLIX;

			if (has_compact)
				/* bit 4 = 1 (0x1F) */
				flags_map |= __FLAG_COMPACT;

			if (has_time)
				/* bit 5 = 1 (0x20) */
				flags_map |= __FLAG_TIME;

			char *p = compiler_extra_options;
			p += strlen(p);

			int i;
			for (i = 0; object_opt[i].option; i++) {
				if (!(flags_map & object_opt[i].flag))
					continue;

				memcpy(p, object_opt[i].option,
					object_opt[i].len);
				p += object_opt[i].len;
			}

			*p = '\0';
		}
#if defined(_DBG_PRINT)
		compilr_with_debugging = true;
#endif
		if (has_detailed)
			compilr_with_debugging = true;

		if (strlen(compiler_extra_options) > 0) {
			size_t current_aio_opt_len = 0;

			if (dogconfig.dog_toml_aio_opt) {
				current_aio_opt_len =
					strlen(dogconfig.dog_toml_aio_opt);
			} else {
				dogconfig.dog_toml_aio_opt = strdup("");
			}

			size_t extra_len =
				strlen(compiler_extra_options);
			char *new_ptr = dog_realloc(
				dogconfig.dog_toml_aio_opt,
				current_aio_opt_len + extra_len + 1);

			if (!new_ptr) {
				pr_error(stdout,
					"Memory allocation failed for extra options");
				goto compiler_end;
			}

			dogconfig.dog_toml_aio_opt = new_ptr;
			strcat(dogconfig.dog_toml_aio_opt,
				compiler_extra_options);
		}

		if (strfind(compile_args_val, __PARENT_DIR, true) !=
			false) {
			size_t write_pos = 0, j;
			bool rate_parent_dir = false;
			for (j = 0; compile_args_val[j] != '\0';) {
				if (!rate_parent_dir &&
					strncmp(&compile_args_val[j],
					__PARENT_DIR, 3) == 0) {
					j += 3;
					while (compile_args_val[j] !=
						'\0' &&
						compile_args_val[j] != ' ' &&
						compile_args_val[j] !=
						'"') {
						parsing[write_pos++] =
							compile_args_val[j++];
					}
					size_t read_cur = 0, scan_idx;
					for (scan_idx = 0;
						scan_idx < write_pos;
						scan_idx++) {
						if (parsing[scan_idx] ==
							__PATH_CHR_SEP_LINUX ||
							parsing[scan_idx] ==
							__PATH_CHR_SEP_WIN32) {
							read_cur =
								scan_idx + 1;
						}
					}
					if (read_cur > 0) {
						write_pos = read_cur;
					}
					rate_parent_dir = true;
					break;
				} else
					j++;
			}

			if (rate_parent_dir && write_pos > 0) {
				memmove(parsing + 3, parsing,
					write_pos + 1);
				memcpy(parsing, __PARENT_DIR, 3);
				write_pos += 3;
				parsing[write_pos] = '\0';
				if (parsing[write_pos - 1] !=
					__PATH_CHR_SEP_LINUX &&
					parsing[write_pos - 1] !=
					__PATH_CHR_SEP_WIN32)
					strcat(parsing, "/");
			} else {
				strcpy(parsing, __PARENT_DIR);
			}

			strcpy(temp, parsing);

			char *gamemodes_slash = "gamemodes/";
			char *gamemodes_back_slash = "gamemodes\\";

			if (strstr(temp, gamemodes_slash) ||
				strstr(temp, gamemodes_back_slash)) {
				char *pos = strstr(temp,
					gamemodes_slash);
				if (!pos)
					pos = strstr(temp,
						gamemodes_back_slash);
				if (pos)
					*pos = '\0';
			}

			snprintf(buf, sizeof(buf),
				"-i" "=%s "
				"-i" "=%s" "gamemodes/ "
				"-i" "=%s" "pawno/include/ "
				"-i" "=%s" "qawno/include/ ",
				temp, temp, temp, temp);

			strncpy(this_path_include, buf,
				sizeof(this_path_include) - 1);
			this_path_include[
				sizeof(this_path_include) - 1] = '\0';
		} else {
			snprintf(buf, sizeof(buf),
				"-i" "=gamemodes/ "
				"-i" "=pawno/include/ "
				"-i" "=qawno/include");
			strncpy(this_path_include, buf,
				sizeof(this_path_include) - 1);
			this_path_include[
				sizeof(this_path_include) - 1] = '\0';
		}

		static int rate_flag_notice = 0;
		if (!rate_flag_notice) {
			printf("\n");
			printf(FCOLOUR_YELLOW "Options:\n" FCOLOUR_DEFAULT);

			printf(
			"  %s-w%s, %s--watchdogs%s   Show detailed output         - compile detail\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-d%s, %s--debug%s       Enable debug level 2         - full debugging\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-p%s, %s--prolix%s      Verbose mode                 - processing detail\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-a%s, %s--assembler%s   Generate assembler file      - assembler ouput\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-s%s, %s--compact%s     Compact encoding compression - resize output\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-c%s, %s--compat%s      Compatibility mode           - path sep compat\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-f%s, %s--fast%s        Fast compile-sime            - no optimize\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf(
			"  %s-n%s, %s--clean%s       Remove '.amx' after compile  - remover output\n",
			FCOLOUR_CYAN, FCOLOUR_DEFAULT,
			FCOLOUR_CYAN, FCOLOUR_DEFAULT);

			printf("\n");
			fflush(stdout);
			rate_flag_notice = 1;
		}

		if (!is_pterodactyl_env()) {
			/* localhost detecting
				#include <a_samp>

				#if LOCALHOST==1 || MYSQL_LOCALHOST==1 || SQL_LOCALHOST==1 || LOCAL_SERVER==1
				// also #if defined LOCALHOST
					... YES
				#else
					... NO
				#endif
			*/
			/* options https://github.com/gskeleton/watchdogs/blob/main/options.txt
				... ..
				sym=val  define constant "sym" with value "val"
				sym=     define constant "sym" with value 0
			*/
			/* one-time check */
			static int ot_val = -1; /* one-time check */
			if (ot_val == -1) {
				printf(FCOLOUR_BCYAN
					"you're compiling in localhost??.\n"
					"   Do you want to target the server for localhost or non-localhost (hosting)?\n"
					"   1/enter = localhost | 2/rand = hosting server.\n"
					FCOLOUR_DEFAULT);
				printf(FCOLOUR_CYAN ">" FCOLOUR_DEFAULT);
				fflush(stdout);
				char *otime = readline(" ");
				if (otime[0] != '\0') {
					ot_val = (strfind(otime, "1",
						true) == 1) ? 1 : 2;
					dog_free(otime);
				} else {
					ot_val = 1;
				}
			}
			if (ot_val == 1) {
				static int not_val = 0;
				snprintf(buf, sizeof(buf),
					"%s LOCALHOST" "=1 "
					"MYSQL_LOCALHOST" "=1 "
					"SQL_LOCALHOST" "=1 "
					"LOCAL_SERVER" "=1",
					dogconfig.dog_toml_aio_opt);
			} else {
				snprintf(buf, sizeof(buf), "%s "
					"LOCALHOST" "=0 "
					"MYSQL_LOCALHOST" "=0 "
					"SQL_LOCALHOST" "=0 "
					"LOCAL_SERVER" "=0",
					dogconfig.dog_toml_aio_opt);
			}
			dog_free(dogconfig.dog_toml_aio_opt);
			dogconfig.dog_toml_aio_opt = strdup(buf);
		} else {
			snprintf(buf, sizeof(buf), "%s "
				"LOCALHOST" "=0 "
				"MYSQL_LOCALHOST" "=0 "
				"SQL_LOCALHOST" "=0 "
				"LOCAL_SERVER" "=0",
				dogconfig.dog_toml_aio_opt);
			dog_free(dogconfig.dog_toml_aio_opt);
			dogconfig.dog_toml_aio_opt = strdup(buf);
		}

#ifdef DOG_ANDROID
		/* disable warning 200 (truncated char) from compiler. */
		snprintf(buf, sizeof(buf), "%s -w:200-",
			dogconfig.dog_toml_aio_opt);
		dog_free(dogconfig.dog_toml_aio_opt);
		dogconfig.dog_toml_aio_opt = strdup(buf);
#endif

		printf(FCOLOUR_DEFAULT);

		if (compile_args_val == NULL) {
			compile_args_val = "";
		}

		if (*compile_args_val == '\0' ||
			(compile_args_val[0] == '.' &&
			compile_args_val[1] == '\0')) {
			if (path_exists(dogconfig.dog_toml_proj_input) ==
				0) {
				pr_error(stdout, "%s not found!.",
					dogconfig.dog_toml_proj_input);
				goto compiler_end;
			}
			static int compiler_targets = 0;
			if (compiler_targets != 1 &&
				strlen(compile_args_val) < 1) {
				pr_color(stdout, FCOLOUR_YELLOW,
					FCOLOUR_BYELLOW
					"[====== COMPILER TARGET ======]\n");
				printf(FCOLOUR_BCYAN);
				printf(
					"   ** You run the command without any args like: compile . compile mode.pwn\n"
					"   * Do you want to compile for "
					FCOLOUR_GREEN "%s " FCOLOUR_BCYAN
					"(enter), \n"
					"   * or do you want to compile for something else?\n",
					dogconfig.dog_toml_proj_input);
				printf(FCOLOUR_DEFAULT);
				printf(FCOLOUR_CYAN ">"
					FCOLOUR_DEFAULT);
				fflush(stdout);
				proj_targets = readline(" ");
				if (proj_targets &&
					strlen(proj_targets) > 0) {
					dog_free(
						dogconfig.dog_toml_proj_input);
					dogconfig.dog_toml_proj_input =
						strdup(proj_targets);
					if (!dogconfig.dog_toml_proj_input) {
						pr_error(stdout,
							"Memory allocation failed");
						dog_free(proj_targets);
						goto compiler_end;
					}
				}
				dog_free(proj_targets);
				proj_targets = NULL;
				compiler_targets = 1;
			}
#ifdef DOG_WINDOWS
			/* typedef struct _STARTUPINFO {
				DWORD   cb;              // Size of structure
				LPTSTR  lpReserved;      // Reserved
				LPTSTR  lpDesktop;       // Desktop name
				LPTSTR  lpTitle;         // Console window title
				DWORD   dwX;             // Window X position
				DWORD   dwY;             // Window Y position
				DWORD   dwXSize;         // Window width
				DWORD   dwYSize;         // Window height
				DWORD   dwXCountChars;   // Console buffer width
				DWORD   dwYCountChars;   // Console buffer height
				DWORD   dwFillAttribute; // Console text/background color
				DWORD   dwFlags;         // Flags
				WORD    wShowWindow;     // Window show state
				WORD    cbReserved2;     // Reserved
				LPBYTE  lpReserved2;     // Reserved
				HANDLE  hStdInput;       // Standard input handle
				HANDLE  hStdOutput;      // Standard output handle
				HANDLE  hStdError;       // Standard error handle
			} STARTUPINFO, *LPSTARTUPINFO; */
			ZeroMemory(&_STARTUPINFO,
				sizeof(_STARTUPINFO));
			_STARTUPINFO.cb = sizeof(_STARTUPINFO);

			ZeroMemory(&_ATTRIBUTES, sizeof(_ATTRIBUTES));
			_ATTRIBUTES.nLength = sizeof(_ATTRIBUTES);
			_ATTRIBUTES.bInheritHandle = TRUE;
			_ATTRIBUTES.lpSecurityDescriptor = NULL;

			ZeroMemory(&_PROCESS_INFO,
				sizeof(_PROCESS_INFO));

			HANDLE hFile = CreateFileA(
				".watchdogs/compiler.log",
				GENERIC_WRITE,
				FILE_SHARE_READ,
				&_ATTRIBUTES,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_SEQUENTIAL_SCAN |
				FILE_ATTRIBUTE_TEMPORARY,
				NULL);

			_STARTUPINFO.dwFlags = STARTF_USESTDHANDLES |
				STARTF_USESHOWWINDOW;
			_STARTUPINFO.wShowWindow = SW_HIDE;

			if (hFile != INVALID_HANDLE_VALUE) {
				_STARTUPINFO.hStdOutput = hFile;
				_STARTUPINFO.hStdError = hFile;
			}
			_STARTUPINFO.hStdInput = GetStdHandle(
				STD_INPUT_HANDLE);

			char *size_include_aio_path = strdup(include_aio_path);

			int ret_command = 0;
			ret_command = snprintf(compiler_input,
				sizeof(compiler_input),
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				"-o" COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING,
				compiler_pawncc_path,
				dogconfig.dog_toml_proj_input,
				dogconfig.dog_toml_proj_output,
				dogconfig.dog_toml_aio_opt,
				size_include_aio_path,
				this_path_include);

			if (compilr_with_debugging == true) {
#ifdef DOG_ANDROID
				println(stdout, "%s", compiler_input);
#else
				dog_console_title(compiler_input);
#endif
			}

			char multi_compiler_input[sizeof(compiler_input)];
			memcpy(multi_compiler_input, compiler_input,
				sizeof(multi_compiler_input));

			if (ret_command < 0 ||
				ret_command >= sizeof(compiler_input)) {
				pr_error(stdout,
					"ret_compiler too long!");
				minimal_debugging();
				goto compiler_end;
			}

			BOOL win32_process_success;
			win32_process_success = CreateProcessA(
				NULL, multi_compiler_input,
				NULL, NULL,
				TRUE,
				CREATE_NO_WINDOW |
				ABOVE_NORMAL_PRIORITY_CLASS |
				CREATE_BREAKAWAY_FROM_JOB,
				NULL, NULL,
				&_STARTUPINFO, &_PROCESS_INFO);

			if (hFile != INVALID_HANDLE_VALUE) {
				SetHandleInformation(hFile,
					HANDLE_FLAG_INHERIT, 0);
			}

			if (win32_process_success == TRUE) {
				SetThreadPriority(
					_PROCESS_INFO.hThread,
					THREAD_PRIORITY_ABOVE_NORMAL);

				DWORD_PTR procMask, sysMask;
				GetProcessAffinityMask(
					GetCurrentProcess(),
					&procMask, &sysMask);
				SetProcessAffinityMask(
					_PROCESS_INFO.hProcess,
					procMask & ~1);

				clock_gettime(CLOCK_MONOTONIC,
					&pre_start);
				DWORD waitResult =
					WaitForSingleObject(
					_PROCESS_INFO.hProcess,
					WIN32_TIMEOUT);
				if (waitResult == WAIT_TIMEOUT) {
					TerminateProcess(
						_PROCESS_INFO.hProcess, 1);
					WaitForSingleObject(
						_PROCESS_INFO.hProcess,
						5000);
				}
				clock_gettime(CLOCK_MONOTONIC,
					&post_end);

				DWORD proc_exit_code;
				GetExitCodeProcess(
					_PROCESS_INFO.hProcess,
					&proc_exit_code);

				CloseHandle(_PROCESS_INFO.hThread);
				CloseHandle(_PROCESS_INFO.hProcess);

				if (_STARTUPINFO.hStdOutput != NULL &&
					_STARTUPINFO.hStdOutput != hFile)
					CloseHandle(
						_STARTUPINFO.hStdOutput);
				if (_STARTUPINFO.hStdError != NULL &&
					_STARTUPINFO.hStdError != hFile)
					CloseHandle(
						_STARTUPINFO.hStdError);
			} else {
				pr_error(stdout,
					"CreateProcess failed! (%lu)",
					GetLastError());
				minimal_debugging();
			}
			if (hFile != INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
			}
#else
			char *size_include_aio_path = strdup(include_aio_path);

			int ret_command = 0;
			ret_command = snprintf(compiler_input,
				sizeof(compiler_input),
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				"-o" COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING
				COMPILER_WHITESPACE
				COMPILER_PLACEHOLDER_STRING,
				compiler_pawncc_path,
				dogconfig.dog_toml_proj_input,
				dogconfig.dog_toml_proj_output,
				dogconfig.dog_toml_aio_opt,
				size_include_aio_path,
				this_path_include);
				
			if (ret_command < 0 ||
				ret_command >= sizeof(compiler_input)) {
				pr_error(stdout,
					"ret_compiler too long!");
				minimal_debugging();
				goto compiler_end;
			}

			if (compilr_with_debugging == true) {
#ifdef DOG_ANDROID
				println(stdout, "%s", compiler_input);
#else
				dog_console_title(compiler_input);
#endif
			}

			char multi_compiler_input[sizeof(compiler_input)];
			strncpy(multi_compiler_input, compiler_input,
				sizeof(multi_compiler_input) - 1);
			multi_compiler_input[
				sizeof(multi_compiler_input) - 1] = '\0';

			int i = 0;
			char *saveptr = NULL;
			compiler_unix_token = strtok_r(
				multi_compiler_input, " ", &saveptr);
			while (compiler_unix_token != NULL &&
				i < (DOG_MAX_PATH + 255)) {
				dog_compiler_unix_args[i++] =
					compiler_unix_token;
				compiler_unix_token = strtok_r(NULL,
					" ", &saveptr);
			}
			dog_compiler_unix_args[i] = NULL;

#ifdef DOG_ANDROID
			static int child_method = 0;
			if (has_time == 1) {
				child_method = 1;
			}

			if (child_method == 0) {
				pid_t compiler_process_id = fork();

				if (compiler_process_id == 0) {
					int logging_file = open(
						".watchdogs/compiler.log",
						O_WRONLY | O_CREAT |
						O_TRUNC, 0644);
					if (logging_file != -1) {
						dup2(logging_file,
							STDOUT_FILENO);
						dup2(logging_file,
							STDERR_FILENO);
						close(logging_file);
					}

					execv(dog_compiler_unix_args[0],
						dog_compiler_unix_args);

					fprintf(stderr,
						"execv failed: %s\n",
						strerror(errno));
					_exit(127);
				} else if (compiler_process_id > 0) {
					int process_status;
					int process_timeout_occurred = 0;
					clock_gettime(CLOCK_MONOTONIC,
						&pre_start);

					for (int i = 0;
						i < POSIX_TIMEOUT; i++) {
						int p_result = waitpid(
							compiler_process_id,
							&process_status,
							WNOHANG);
						clock_gettime(
							CLOCK_MONOTONIC,
							&post_end);

						if (p_result == 0) {
							usleep(100000);
						} else if (p_result ==
							compiler_process_id) {
							break;
						} else {
							pr_error(stdout,
								"waitpid error");
							minimal_debugging();
							break;
						}

						if (i ==
							POSIX_TIMEOUT - 1) {
							kill(
								compiler_process_id,
								SIGTERM);
							sleep(2);
							kill(
								compiler_process_id,
								SIGKILL);
							pr_error(stdout,
								"process execution timeout! (%d seconds)",
								POSIX_TIMEOUT);
							minimal_debugging();
							waitpid(
								compiler_process_id,
								&process_status,
								0);
							process_timeout_occurred =
								1;
						}
					}

					if (!process_timeout_occurred) {
						if (WIFEXITED(
							process_status)) {
							int proc_exit_code =
								WEXITSTATUS(
								process_status);
							if (proc_exit_code !=
								0 &&
								proc_exit_code !=
								1) {
								pr_error(stdout,
									"compiler process exited with code (%d)",
									proc_exit_code);
								minimal_debugging();
							}
						} else if (WIFSIGNALED(
							process_status)) {
							pr_error(stdout,
								"compiler process terminated by signal (%d)",
								WTERMSIG(
								process_status));
						}
					}
				} else {
					pr_error(stdout,
						"fork failed: %s",
						strerror(errno));
					minimal_debugging();
				}
			} else {
				pid_t compiler_process_id = vfork();

				if (compiler_process_id == 0) {
					int logging_file = open(
						".watchdogs/compiler.log",
						O_WRONLY | O_CREAT |
						O_TRUNC, 0644);
					if (logging_file != -1) {
						dup2(logging_file,
							STDOUT_FILENO);
						dup2(logging_file,
							STDERR_FILENO);
						close(logging_file);
					}

					execv(dog_compiler_unix_args[0],
						dog_compiler_unix_args);

					_exit(127);
				} else if (compiler_process_id > 0) {
					int process_status;
					int process_timeout_occurred = 0;
					clock_gettime(CLOCK_MONOTONIC,
						&pre_start);

					for (int i = 0;
						i < POSIX_TIMEOUT; i++) {
						int p_result = waitpid(
							compiler_process_id,
							&process_status,
							WNOHANG);
						clock_gettime(
							CLOCK_MONOTONIC,
							&post_end);

						if (p_result == 0) {
							usleep(100000);
						} else if (p_result ==
							compiler_process_id) {
							break;
						} else {
							pr_error(stdout,
								"waitpid error");
							minimal_debugging();
							break;
						}

						if (i ==
							POSIX_TIMEOUT - 1) {
							kill(
								compiler_process_id,
								SIGTERM);
							sleep(2);
							kill(
								compiler_process_id,
								SIGKILL);
							pr_error(stdout,
								"process execution timeout! (%d seconds)",
								POSIX_TIMEOUT);
							minimal_debugging();
							waitpid(
								compiler_process_id,
								&process_status,
								0);
							process_timeout_occurred =
								1;
						}
					}

					if (!process_timeout_occurred) {
						if (WIFEXITED(
							process_status)) {
							int proc_exit_code =
								WEXITSTATUS(
								process_status);
							if (proc_exit_code !=
								0 &&
								proc_exit_code !=
								1) {
								pr_error(stdout,
									"compiler process exited with code (%d)",
									proc_exit_code);
								minimal_debugging();
							}
						} else if (WIFSIGNALED(
							process_status)) {
							pr_error(stdout,
								"compiler process terminated by signal (%d)",
								WTERMSIG(
								process_status));
						}
					}
				} else {
					pr_error(stdout,
						"vfork failed: %s",
						strerror(errno));
					minimal_debugging();
				}
			}
#else
			posix_spawn_file_actions_t process_file_actions;
			posix_spawn_file_actions_init(
				&process_file_actions);
			int posix_logging_file = open(
				".watchdogs/compiler.log",
				O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (posix_logging_file != -1) {
				posix_spawn_file_actions_adddup2(
					&process_file_actions,
					posix_logging_file,
					STDOUT_FILENO);
				posix_spawn_file_actions_adddup2(
					&process_file_actions,
					posix_logging_file,
					STDERR_FILENO);
			}

			posix_spawnattr_t spawn_attr;
			posix_spawnattr_init(&spawn_attr);

			sigset_t sigmask;
			sigemptyset(&sigmask);
			sigaddset(&sigmask, SIGCHLD);

			posix_spawnattr_setsigmask(&spawn_attr,
				&sigmask);

			sigset_t sigdefault;
			sigemptyset(&sigdefault);
			sigaddset(&sigdefault, SIGPIPE);
			sigaddset(&sigdefault, SIGINT);
			sigaddset(&sigdefault, SIGTERM);

			posix_spawnattr_setsigdefault(&spawn_attr,
				&sigdefault);

			short flags = POSIX_SPAWN_SETSIGMASK |
				POSIX_SPAWN_SETSIGDEF;

			posix_spawnattr_setflags(&spawn_attr, flags);

			pid_t compiler_process_id;
			int process_spawn_result = posix_spawn(
				&compiler_process_id,
				dog_compiler_unix_args[0],
				&process_file_actions,
				&spawn_attr,
				dog_compiler_unix_args,
				environ);

			if (posix_logging_file != -1) {
				close(posix_logging_file);
			}

			posix_spawnattr_destroy(&spawn_attr);
			posix_spawn_file_actions_destroy(
				&process_file_actions);

			if (process_spawn_result == 0) {
				int process_status;
				int process_timeout_occurred = 0;
				clock_gettime(CLOCK_MONOTONIC,
					&pre_start);
				for (int i = 0; i < POSIX_TIMEOUT; i++) {
					int p_result = -1;
					p_result = waitpid(
						compiler_process_id,
						&process_status, WNOHANG);
					clock_gettime(CLOCK_MONOTONIC,
						&post_end);
					if (p_result == 0)
						usleep(50000);
					else if (p_result ==
						compiler_process_id) {
						break;
					} else {
						pr_error(stdout,
							"waitpid error");
						minimal_debugging();
						break;
					}
					if (i == POSIX_TIMEOUT - 1) {
						kill(compiler_process_id,
							SIGTERM);
						sleep(2);
						kill(compiler_process_id,
							SIGKILL);
						pr_error(stdout,
							"posix_spawn process execution timeout! (%d seconds)",
							POSIX_TIMEOUT);
						minimal_debugging();
						waitpid(
							compiler_process_id,
							&process_status, 0);
						process_timeout_occurred =
							1;
					}
				}
				if (!process_timeout_occurred) {
					if (WIFEXITED(process_status)) {
						int proc_exit_code = 0;
						proc_exit_code =
							WEXITSTATUS(
							process_status);
						if (proc_exit_code != 0 &&
							proc_exit_code != 1) {
							pr_error(stdout,
								"compiler process exited with code (%d)",
								proc_exit_code);
							minimal_debugging();
						}
					} else if (WIFSIGNALED(
						process_status)) {
						pr_error(stdout,
							"compiler process terminated by signal (%d)",
							WTERMSIG(
							process_status));
					}
				}
			} else {
				pr_error(stdout,
					"posix_spawn failed: %s",
					strerror(process_spawn_result));
				minimal_debugging();
			}
#endif
#endif
			char size_container_output[DOG_PATH_MAX * 2];
			snprintf(size_container_output,
				sizeof(size_container_output), "%s",
				dogconfig.dog_toml_proj_output);
			if (path_exists(".watchdogs/compiler.log")) {
				printf("\n");
				char *ca = NULL;
				ca = size_container_output;
				bool cb = 0;
				if (compiler_debugging)
					cb = 1;
				if (has_detailed && has_clean) {
					cause_compiler_expl(
						".watchdogs/compiler.log",
						ca, cb);
					if (path_exists(ca)) {
						remove(ca);
					}
					goto compiler_done;
				} else if (has_detailed) {
					cause_compiler_expl(
						".watchdogs/compiler.log",
						ca, cb);
					goto compiler_done;
				} else if (has_clean) {
					dog_printfile(
						".watchdogs/compiler.log");
					if (path_exists(ca)) {
						remove(ca);
					}
					goto compiler_done;
				}

				dog_printfile(".watchdogs/compiler.log");

				char log_line[DOG_MAX_PATH * 4];
				this_proc_file = fopen(
					".watchdogs/compiler.log", "r");

				if (this_proc_file != NULL) {
					while (fgets(log_line,
						sizeof(log_line),
						this_proc_file) != NULL) {
						if (strfind(log_line,
							"backtrace", true))
							pr_color(stdout,
								FCOLOUR_CYAN,
								"~ backtrace detected - "
								"make sure you are using a newer version of pawncc than the one currently in use.");
					}
					fclose(this_proc_file);
					this_proc_file = NULL;
				}
			}
compiler_done:
			this_proc_file = fopen(".watchdogs/compiler.log",
				"r");
			if (this_proc_file) {
				char compiler_line_buffer[DOG_PATH_MAX];
				int has_err = 0;
				while (fgets(compiler_line_buffer,
					sizeof(compiler_line_buffer),
					this_proc_file)) {
					if (strstr(compiler_line_buffer,
						"error")) {
						has_err = 1;
						break;
					}
				}
				fclose(this_proc_file);
				this_proc_file = NULL;
				if (has_err) {
					if (size_container_output[0] !=
						'\0' &&
						path_access(size_container_output))
						remove(size_container_output);
					dogconfig.dog_compiler_stat = 1;
				} else {
					dogconfig.dog_compiler_stat = 0;
				}
			} else {
				pr_error(stdout,
					"Failed to open .watchdogs/compiler.log");
				minimal_debugging();
			}

			timer_rate_compile = (post_end.tv_sec -
				pre_start.tv_sec) +
				(post_end.tv_nsec - pre_start.tv_nsec) /
				1e9;

			printf("\n");
			pr_color(stdout, FCOLOUR_CYAN,
				" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
				timer_rate_compile,
				timer_rate_compile * 1000.0);
			if (timer_rate_compile > 36) {
				printf(
					"~ This is taking a while, huh?\n"
					"  Make sure you've cleared all the warnings,\n"
					"  you're using the latest compiler,\n"
					"  and double-check that your logic\n"
					"  and pawn algorithm tweaks in the gamemode scripts line up.\n"
					"  make sure " FCOLOUR_CYAN
					"'MAX_PLAYERS'" FCOLOUR_DEFAULT " and"
					FCOLOUR_CYAN "'MAX_VEHICLES'" FCOLOUR_DEFAULT "is normal macro value\n");
				fflush(stdout);
			}
		} else {
			strncpy(kevlar_compiler->compiler_size_temp,
				compile_args_val,
				sizeof(kevlar_compiler->compiler_size_temp) -
				1);
			kevlar_compiler->compiler_size_temp[
				sizeof(kevlar_compiler->compiler_size_temp) -
				1] = '\0';

			compiler_size_last_slash = strrchr(
				kevlar_compiler->compiler_size_temp,
				__PATH_CHR_SEP_LINUX);
			compiler_back_slash = strrchr(
				kevlar_compiler->compiler_size_temp,
				__PATH_CHR_SEP_WIN32);

			if (compiler_back_slash && (!compiler_size_last_slash ||
				compiler_back_slash > compiler_size_last_slash))
				compiler_size_last_slash =
					compiler_back_slash;

			if (compiler_size_last_slash) {
				size_t compiler_dir_len;
				compiler_dir_len = (size_t)
					(compiler_size_last_slash -
					kevlar_compiler->compiler_size_temp);

				if (compiler_dir_len >=
					sizeof(kevlar_compiler->compiler_direct_path))
					compiler_dir_len =
						sizeof(kevlar_compiler->compiler_direct_path) -
						1;

				memcpy(kevlar_compiler->compiler_direct_path,
					kevlar_compiler->compiler_size_temp,
					compiler_dir_len);
				kevlar_compiler->compiler_direct_path[
					compiler_dir_len] = '\0';

				const char *compiler_filename_start =
					compiler_size_last_slash + 1;
				size_t compiler_filename_len;
				compiler_filename_len = strlen(
					compiler_filename_start);

				if (compiler_filename_len >=
					sizeof(kevlar_compiler->compiler_size_file_name))
					compiler_filename_len =
						sizeof(kevlar_compiler->compiler_size_file_name) -
						1;

				memcpy(
					kevlar_compiler->compiler_size_file_name,
					compiler_filename_start,
					compiler_filename_len);
				kevlar_compiler->compiler_size_file_name[
					compiler_filename_len] = '\0';

				size_t total_needed;
				total_needed =
					strlen(kevlar_compiler->compiler_direct_path) +
					1 +
					strlen(kevlar_compiler->compiler_size_file_name) +
					1;

				if (total_needed >
					sizeof(kevlar_compiler->compiler_size_input_path)) {
					strncpy(kevlar_compiler->compiler_direct_path,
						"gamemodes",
						sizeof(kevlar_compiler->compiler_direct_path) -
						1);
					kevlar_compiler->compiler_direct_path[
						sizeof(kevlar_compiler->compiler_direct_path) -
						1] = '\0';

					size_t compiler_max_size_file_name;
					compiler_max_size_file_name =
						sizeof(kevlar_compiler->compiler_size_file_name) -
						1;

					if (compiler_filename_len >
						compiler_max_size_file_name) {
						memcpy(
							kevlar_compiler->compiler_size_file_name,
							compiler_filename_start,
							compiler_max_size_file_name);
						kevlar_compiler->compiler_size_file_name[
							compiler_max_size_file_name] =
							'\0';
					}
				}

				if (snprintf(
					kevlar_compiler->compiler_size_input_path,
					sizeof(kevlar_compiler->compiler_size_input_path),
					"%s/%s",
					kevlar_compiler->compiler_direct_path,
					kevlar_compiler->compiler_size_file_name) >=
					(int)sizeof(
					kevlar_compiler->compiler_size_input_path)) {
					kevlar_compiler->compiler_size_input_path[
						sizeof(kevlar_compiler->compiler_size_input_path) -
						1] = '\0';
				}
			} else {
				strncpy(
					kevlar_compiler->compiler_size_file_name,
					kevlar_compiler->compiler_size_temp,
					sizeof(kevlar_compiler->compiler_size_file_name) -
					1);
				kevlar_compiler->compiler_size_file_name[
					sizeof(kevlar_compiler->compiler_size_file_name) -
					1] = '\0';

				strncpy(
					kevlar_compiler->compiler_direct_path,
					".",
					sizeof(kevlar_compiler->compiler_direct_path) -
					1);
				kevlar_compiler->compiler_direct_path[
					sizeof(kevlar_compiler->compiler_direct_path) -
					1] = '\0';

				if (snprintf(
					kevlar_compiler->compiler_size_input_path,
					sizeof(kevlar_compiler->compiler_size_input_path),
					"./%s",
					kevlar_compiler->compiler_size_file_name) >=
					(int)sizeof(
					kevlar_compiler->compiler_size_input_path)) {
					kevlar_compiler->compiler_size_input_path[
						sizeof(kevlar_compiler->compiler_size_input_path) -
						1] = '\0';
				}
			}

			int compiler_finding_compile_args = 0;
			compiler_finding_compile_args = dog_sef_fdir(
				kevlar_compiler->compiler_direct_path,
				kevlar_compiler->compiler_size_file_name,
				NULL);

			if (!compiler_finding_compile_args &&
				strcmp(kevlar_compiler->compiler_direct_path,
				"gamemodes") != 0) {
				compiler_finding_compile_args =
					dog_sef_fdir("gamemodes",
					kevlar_compiler->compiler_size_file_name,
					NULL);
				if (compiler_finding_compile_args) {
					strncpy(
						kevlar_compiler->compiler_direct_path,
						"gamemodes",
						sizeof(kevlar_compiler->compiler_direct_path) -
						1);
					kevlar_compiler->compiler_direct_path[
						sizeof(kevlar_compiler->compiler_direct_path) -
						1] = '\0';

					if (snprintf(
						kevlar_compiler->compiler_size_input_path,
						sizeof(kevlar_compiler->compiler_size_input_path),
						"gamemodes/%s",
						kevlar_compiler->compiler_size_file_name) >=
						(int)sizeof(
						kevlar_compiler->compiler_size_input_path)) {
						kevlar_compiler->compiler_size_input_path[
							sizeof(kevlar_compiler->compiler_size_input_path) -
							1] = '\0';
					}

					if (dogconfig.dog_sef_count >
						RATE_SEF_EMPTY)
						strncpy(
							dogconfig.dog_sef_found_list[
							dogconfig.dog_sef_count -
							1],
							kevlar_compiler->compiler_size_input_path,
							MAX_SEF_PATH_SIZE);
				}
			}

			if (!compiler_finding_compile_args &&
				!strcmp(kevlar_compiler->compiler_direct_path,
				".")) {
				compiler_finding_compile_args =
					dog_sef_fdir("gamemodes",
					kevlar_compiler->compiler_size_file_name,
					NULL);
				if (compiler_finding_compile_args) {
					strncpy(
						kevlar_compiler->compiler_direct_path,
						"gamemodes",
						sizeof(kevlar_compiler->compiler_direct_path) -
						1);
					kevlar_compiler->compiler_direct_path[
						sizeof(kevlar_compiler->compiler_direct_path) -
						1] = '\0';

					if (snprintf(
						kevlar_compiler->compiler_size_input_path,
						sizeof(kevlar_compiler->compiler_size_input_path),
						"gamemodes/%s",
						kevlar_compiler->compiler_size_file_name) >=
						(int)sizeof(
						kevlar_compiler->compiler_size_input_path)) {
						kevlar_compiler->compiler_size_input_path[
							sizeof(kevlar_compiler->compiler_size_input_path) -
							1] = '\0';
					}

					if (dogconfig.dog_sef_count >
						RATE_SEF_EMPTY)
						strncpy(
							dogconfig.dog_sef_found_list[
							dogconfig.dog_sef_count -
							1],
							kevlar_compiler->compiler_size_input_path,
							MAX_SEF_PATH_SIZE);
				}
			}

			size_t	 i, sef_max_entries;

			sef_max_entries = sizeof(dogconfig.dog_sef_found_list) /
				sizeof(dogconfig.dog_sef_found_list[0]);

			for (i = 0; i < sef_max_entries; i++) {
				if (strfind(dogconfig.dog_sef_found_list[i], compile_args_val, true)) {
					snprintf(compiler_proj_path,
					sizeof(compiler_proj_path), "%s",
					dogconfig.dog_sef_found_list[i]);
				}
			}

			if (compiler_finding_compile_args) {
				char size_sef_path[DOG_PATH_MAX];
				snprintf(size_sef_path,
					sizeof(size_sef_path), "%s",
					compiler_proj_path);
				char *extension = strrchr(size_sef_path,
					'.');
				if (extension)
					*extension = '\0';

				snprintf(
					kevlar_compiler->container_output,
					sizeof(kevlar_compiler->container_output),
					"%s", size_sef_path);

				char size_container_output[DOG_MAX_PATH];
				snprintf(size_container_output,
					sizeof(size_container_output),
					"%s.amx",
					kevlar_compiler->container_output);

#ifdef DOG_WINDOWS
				/* typedef struct _STARTUPINFO {
					DWORD   cb;              // Size of structure
					LPTSTR  lpReserved;      // Reserved
					LPTSTR  lpDesktop;       // Desktop name
					LPTSTR  lpTitle;         // Console window title
					DWORD   dwX;             // Window X position
					DWORD   dwY;             // Window Y position
					DWORD   dwXSize;         // Window width
					DWORD   dwYSize;         // Window height
					DWORD   dwXCountChars;   // Console buffer width
					DWORD   dwYCountChars;   // Console buffer height
					DWORD   dwFillAttribute; // Console text/background color
					DWORD   dwFlags;         // Flags
					WORD    wShowWindow;     // Window show state
					WORD    cbReserved2;     // Reserved
					LPBYTE  lpReserved2;     // Reserved
					HANDLE  hStdInput;       // Standard input handle
					HANDLE  hStdOutput;      // Standard output handle
					HANDLE  hStdError;       // Standard error handle
				} STARTUPINFO, *LPSTARTUPINFO; */
				ZeroMemory(&_STARTUPINFO,
					sizeof(_STARTUPINFO));
				_STARTUPINFO.cb = sizeof(_STARTUPINFO);

				ZeroMemory(&_ATTRIBUTES,
					sizeof(_ATTRIBUTES));
				_ATTRIBUTES.nLength = sizeof(_ATTRIBUTES);
				_ATTRIBUTES.bInheritHandle = TRUE;
				_ATTRIBUTES.lpSecurityDescriptor = NULL;

				ZeroMemory(&_PROCESS_INFO,
					sizeof(_PROCESS_INFO));

				HANDLE hFile = CreateFileA(
					".watchdogs/compiler.log",
					GENERIC_WRITE,
					FILE_SHARE_READ,
					&_ATTRIBUTES,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL |
					FILE_FLAG_SEQUENTIAL_SCAN |
					FILE_ATTRIBUTE_TEMPORARY,
					NULL);

				_STARTUPINFO.dwFlags =
					STARTF_USESTDHANDLES |
					STARTF_USESHOWWINDOW;
				_STARTUPINFO.wShowWindow = SW_HIDE;

				if (hFile != INVALID_HANDLE_VALUE) {
					_STARTUPINFO.hStdOutput = hFile;
					_STARTUPINFO.hStdError = hFile;
				}
				_STARTUPINFO.hStdInput =
					GetStdHandle(STD_INPUT_HANDLE);

				char *size_include_aio_path = strdup(include_aio_path);

				int ret_command = 0;
				ret_command = snprintf(compiler_input,
					sizeof(compiler_input),
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					"-o" COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING,
					compiler_pawncc_path,
					compiler_proj_path,
					size_container_output,
					dogconfig.dog_toml_aio_opt,
					size_include_aio_path,
					this_path_include);

				if (compilr_with_debugging == true) {
#ifdef DOG_ANDROID
					println(stdout, "%s",
						compiler_input);
#else
					dog_console_title(compiler_input);
#endif
				}

				char multi_compiler_input[
					sizeof(compiler_input)];
				memcpy(multi_compiler_input,
					compiler_input,
					sizeof(multi_compiler_input));

				if (ret_command < 0 ||
					ret_command >= sizeof(compiler_input)) {
					pr_error(stdout,
						"ret_compiler too long!");
					minimal_debugging();
					goto compiler_end;
				}

				BOOL win32_process_success;
				win32_process_success = CreateProcessA(
					NULL, multi_compiler_input,
					NULL, NULL,
					TRUE,
					CREATE_NO_WINDOW |
					ABOVE_NORMAL_PRIORITY_CLASS |
					CREATE_BREAKAWAY_FROM_JOB,
					NULL, NULL,
					&_STARTUPINFO, &_PROCESS_INFO);

				if (hFile != INVALID_HANDLE_VALUE) {
					SetHandleInformation(hFile,
						HANDLE_FLAG_INHERIT, 0);
				}

				if (win32_process_success == TRUE) {
					SetThreadPriority(
						_PROCESS_INFO.hThread,
						THREAD_PRIORITY_ABOVE_NORMAL);

					DWORD_PTR procMask, sysMask;
					GetProcessAffinityMask(
						GetCurrentProcess(),
						&procMask, &sysMask);
					SetProcessAffinityMask(
						_PROCESS_INFO.hProcess,
						procMask & ~1);

					clock_gettime(CLOCK_MONOTONIC,
						&pre_start);
					DWORD waitResult =
						WaitForSingleObject(
						_PROCESS_INFO.hProcess,
						WIN32_TIMEOUT);
					if (waitResult == WAIT_TIMEOUT) {
						TerminateProcess(
							_PROCESS_INFO.hProcess,
							1);
						WaitForSingleObject(
							_PROCESS_INFO.hProcess,
							5000);
					}
					clock_gettime(CLOCK_MONOTONIC,
						&post_end);

					DWORD proc_exit_code;
					GetExitCodeProcess(
						_PROCESS_INFO.hProcess,
						&proc_exit_code);

					CloseHandle(_PROCESS_INFO.hThread);
					CloseHandle(_PROCESS_INFO.hProcess);

					if (_STARTUPINFO.hStdOutput !=
						NULL &&
						_STARTUPINFO.hStdOutput != hFile)
						CloseHandle(
							_STARTUPINFO.hStdOutput);
					if (_STARTUPINFO.hStdError !=
						NULL &&
						_STARTUPINFO.hStdError != hFile)
						CloseHandle(
							_STARTUPINFO.hStdError);
				} else {
					pr_error(stdout,
						"CreateProcess failed! (%lu)",
						GetLastError());
					minimal_debugging();
				}
				if (hFile != INVALID_HANDLE_VALUE) {
					CloseHandle(hFile);
				}
#else
				char *size_include_aio_path = strdup(include_aio_path);

				int ret_command = 0;
				ret_command = snprintf(compiler_input,
					sizeof(compiler_input),
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					"-o" COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING
					COMPILER_WHITESPACE
					COMPILER_PLACEHOLDER_STRING,
					compiler_pawncc_path,
					compiler_proj_path,
					size_container_output,
					dogconfig.dog_toml_aio_opt,
					size_include_aio_path,
					this_path_include);

				if (ret_command < 0 ||
					ret_command >= sizeof(compiler_input)) {
					pr_error(stdout,
						"ret_compiler too long!");
					minimal_debugging();
					goto compiler_end;
				}

				if (compilr_with_debugging == true) {
#ifdef DOG_ANDROID
					println(stdout, "%s",
						compiler_input);
#else
					dog_console_title(compiler_input);
#endif
				}

				char multi_compiler_input[
					sizeof(compiler_input)];
				strncpy(multi_compiler_input,
					compiler_input,
					sizeof(multi_compiler_input) - 1);
				multi_compiler_input[
					sizeof(multi_compiler_input) - 1] =
					'\0';

				int i = 0;
				char *saveptr = NULL;
				compiler_unix_token = strtok_r(
					multi_compiler_input, " ",
					&saveptr);
				while (compiler_unix_token != NULL &&
					i < (DOG_MAX_PATH + 255)) {
					dog_compiler_unix_args[i++] =
						compiler_unix_token;
					compiler_unix_token =
						strtok_r(NULL, " ",
						&saveptr);
				}
				dog_compiler_unix_args[i] = NULL;

#ifdef DOG_ANDROID
				static int child_method = 0;
				if (has_time == 1) {
					child_method = 1;
				}

				if (child_method == 0) {
					pid_t compiler_process_id = fork();

					if (compiler_process_id == 0) {
						int logging_file = open(
							".watchdogs/compiler.log",
							O_WRONLY | O_CREAT |
							O_TRUNC, 0644);
						if (logging_file != -1) {
							dup2(logging_file,
								STDOUT_FILENO);
							dup2(logging_file,
								STDERR_FILENO);
							close(logging_file);
						}

						execv(dog_compiler_unix_args[0],
							dog_compiler_unix_args);

						fprintf(stderr,
							"execv failed: %s\n",
							strerror(errno));
						_exit(127);
					} else if (compiler_process_id > 0) {
						int process_status;
						int process_timeout_occurred = 0;
						clock_gettime(CLOCK_MONOTONIC,
							&pre_start);

						for (int i = 0;
							i < POSIX_TIMEOUT; i++) {
							int p_result = waitpid(
								compiler_process_id,
								&process_status,
								WNOHANG);
							clock_gettime(
								CLOCK_MONOTONIC,
								&post_end);

							if (p_result == 0) {
								usleep(100000);
							} else if (p_result ==
								compiler_process_id) {
								break;
							} else {
								pr_error(stdout,
									"waitpid error");
								minimal_debugging();
								break;
							}

							if (i ==
								POSIX_TIMEOUT - 1) {
								kill(
									compiler_process_id,
									SIGTERM);
								sleep(2);
								kill(
									compiler_process_id,
									SIGKILL);
								pr_error(stdout,
									"process execution timeout! (%d seconds)",
									POSIX_TIMEOUT);
								minimal_debugging();
								waitpid(
									compiler_process_id,
									&process_status,
									0);
								process_timeout_occurred =
									1;
							}
						}

						if (!process_timeout_occurred) {
							if (WIFEXITED(
								process_status)) {
								int proc_exit_code =
									WEXITSTATUS(
									process_status);
								if (proc_exit_code !=
									0 &&
									proc_exit_code !=
									1) {
									pr_error(stdout,
										"compiler process exited with code (%d)",
										proc_exit_code);
									minimal_debugging();
								}
							} else if (WIFSIGNALED(
								process_status)) {
								pr_error(stdout,
									"compiler process terminated by signal (%d)",
									WTERMSIG(
									process_status));
							}
						}
					} else {
						pr_error(stdout,
							"fork failed: %s",
							strerror(errno));
						minimal_debugging();
					}
				} else {
					pid_t compiler_process_id = vfork();

					if (compiler_process_id == 0) {
						int logging_file = open(
							".watchdogs/compiler.log",
							O_WRONLY | O_CREAT |
							O_TRUNC, 0644);
						if (logging_file != -1) {
							dup2(logging_file,
								STDOUT_FILENO);
							dup2(logging_file,
								STDERR_FILENO);
							close(logging_file);
						}

						execv(dog_compiler_unix_args[0],
							dog_compiler_unix_args);

						_exit(127);
					} else if (compiler_process_id > 0) {
						int process_status;
						int process_timeout_occurred = 0;
						clock_gettime(CLOCK_MONOTONIC,
							&pre_start);

						for (int i = 0;
							i < POSIX_TIMEOUT; i++) {
							int p_result = waitpid(
								compiler_process_id,
								&process_status,
								WNOHANG);
							clock_gettime(
								CLOCK_MONOTONIC,
								&post_end);

							if (p_result == 0) {
								usleep(100000);
							} else if (p_result ==
								compiler_process_id) {
								break;
							} else {
								pr_error(stdout,
									"waitpid error");
								minimal_debugging();
								break;
							}

							if (i ==
								POSIX_TIMEOUT - 1) {
								kill(
									compiler_process_id,
									SIGTERM);
								sleep(2);
								kill(
									compiler_process_id,
									SIGKILL);
								pr_error(stdout,
									"process execution timeout! (%d seconds)",
									POSIX_TIMEOUT);
								minimal_debugging();
								waitpid(
									compiler_process_id,
									&process_status,
									0);
								process_timeout_occurred =
									1;
							}
						}

						if (!process_timeout_occurred) {
							if (WIFEXITED(
								process_status)) {
								int proc_exit_code =
									WEXITSTATUS(
									process_status);
								if (proc_exit_code !=
									0 &&
									proc_exit_code !=
									1) {
									pr_error(stdout,
										"compiler process exited with code (%d)",
										proc_exit_code);
									minimal_debugging();
								}
							} else if (WIFSIGNALED(
								process_status)) {
								pr_error(stdout,
									"compiler process terminated by signal (%d)",
									WTERMSIG(
									process_status));
							}
						}
					} else {
						pr_error(stdout,
							"vfork failed: %s",
							strerror(errno));
						minimal_debugging();
					}
				}
#else
				posix_spawn_file_actions_t
					process_file_actions;
				posix_spawn_file_actions_init(
					&process_file_actions);
				int posix_logging_file = open(
					".watchdogs/compiler.log",
					O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (posix_logging_file != -1) {
					posix_spawn_file_actions_adddup2(
						&process_file_actions,
						posix_logging_file,
						STDOUT_FILENO);
					posix_spawn_file_actions_adddup2(
						&process_file_actions,
						posix_logging_file,
						STDERR_FILENO);
				}

				posix_spawnattr_t spawn_attr;
				posix_spawnattr_init(&spawn_attr);

				sigset_t sigmask;
				sigemptyset(&sigmask);
				sigaddset(&sigmask, SIGCHLD);

				posix_spawnattr_setsigmask(&spawn_attr,
					&sigmask);

				sigset_t sigdefault;
				sigemptyset(&sigdefault);
				sigaddset(&sigdefault, SIGPIPE);
				sigaddset(&sigdefault, SIGINT);
				sigaddset(&sigdefault, SIGTERM);

				posix_spawnattr_setsigdefault(&spawn_attr,
					&sigdefault);

				short flags = POSIX_SPAWN_SETSIGMASK |
					POSIX_SPAWN_SETSIGDEF;

				posix_spawnattr_setflags(&spawn_attr,
					flags);

				pid_t compiler_process_id;
				int process_spawn_result = posix_spawn(
					&compiler_process_id,
					dog_compiler_unix_args[0],
					&process_file_actions,
					&spawn_attr,
					dog_compiler_unix_args,
					environ);

				if (posix_logging_file != -1) {
					close(posix_logging_file);
				}

				posix_spawnattr_destroy(&spawn_attr);
				posix_spawn_file_actions_destroy(
					&process_file_actions);

				if (process_spawn_result == 0) {
					int process_status;
					int process_timeout_occurred = 0;
					clock_gettime(CLOCK_MONOTONIC,
						&pre_start);
					for (int i = 0;
						i < POSIX_TIMEOUT; i++) {
						int p_result = -1;
						p_result = waitpid(
							compiler_process_id,
							&process_status,
							WNOHANG);
						clock_gettime(
							CLOCK_MONOTONIC,
							&post_end);
						if (p_result == 0)
							usleep(50000);
						else if (p_result ==
							compiler_process_id) {
							break;
						} else {
							pr_error(stdout,
								"waitpid error");
							minimal_debugging();
							break;
						}
						if (i ==
							POSIX_TIMEOUT - 1) {
							kill(
								compiler_process_id,
								SIGTERM);
							sleep(2);
							kill(
								compiler_process_id,
								SIGKILL);
							pr_error(stdout,
								"posix_spawn process execution timeout! (%d seconds)",
								POSIX_TIMEOUT);
							minimal_debugging();
							waitpid(
								compiler_process_id,
								&process_status,
								0);
							process_timeout_occurred =
								1;
						}
					}
					if (!process_timeout_occurred) {
						if (WIFEXITED(
							process_status)) {
							int proc_exit_code =
								0;
							proc_exit_code =
								WEXITSTATUS(
								process_status);
							if (proc_exit_code !=
								0 &&
								proc_exit_code !=
								1) {
								pr_error(stdout,
									"compiler process exited with code (%d)",
									proc_exit_code);
								minimal_debugging();
							}
						} else if (WIFSIGNALED(
							process_status)) {
							pr_error(stdout,
								"compiler process terminated by signal (%d)",
								WTERMSIG(
								process_status));
						}
					}
				} else {
					pr_error(stdout,
						"posix_spawn failed: %s",
						strerror(process_spawn_result));
					minimal_debugging();
				}
#endif
#endif
				if (path_exists(
					".watchdogs/compiler.log")) {
					printf("\n");
					char *ca = NULL;
					ca = size_container_output;
					bool cb = 0;
					if (compiler_debugging)
						cb = 1;
					if (has_detailed && has_clean) {
						cause_compiler_expl(
							".watchdogs/compiler.log",
							ca, cb);
						if (path_exists(ca)) {
							remove(ca);
						}
						goto compiler_done2;
					} else if (has_detailed) {
						cause_compiler_expl(
							".watchdogs/compiler.log",
							ca, cb);
						goto compiler_done2;
					} else if (has_clean) {
						dog_printfile(
							".watchdogs/compiler.log");
						if (path_exists(ca)) {
							remove(ca);
						}
						goto compiler_done2;
					}

					dog_printfile(
						".watchdogs/compiler.log");

					char log_line[DOG_MAX_PATH * 4];
					this_proc_file = fopen(
						".watchdogs/compiler.log",
						"r");

					if (this_proc_file != NULL) {
						while (fgets(log_line,
							sizeof(log_line),
							this_proc_file) !=
							NULL) {
							if (strfind(
								log_line,
								"backtrace",
								true))
								pr_color(stdout,
									FCOLOUR_CYAN,
									"~ backtrace detected - "
									"make sure you are using a newer version "
									"of pawncc than the one currently in use.\n");
						}
						fclose(this_proc_file);
						this_proc_file = NULL;
					}
				}

compiler_done2:
				this_proc_file = fopen(
					".watchdogs/compiler.log", "r");
				if (this_proc_file) {
					char compiler_line_buffer[
						DOG_PATH_MAX];
					int has_err = 0;
					while (fgets(
						compiler_line_buffer,
						sizeof(compiler_line_buffer),
						this_proc_file)) {
						if (strstr(
							compiler_line_buffer,
							"error")) {
							has_err = 1;
							break;
						}
					}
					fclose(this_proc_file);
					this_proc_file = NULL;
					if (has_err) {
						if (size_container_output[
							0] != '\0' &&
							path_access(size_container_output))
							remove(
								size_container_output);
						dogconfig.dog_compiler_stat =
							1;
					} else {
						dogconfig.dog_compiler_stat =
							0;
					}
				} else {
					pr_error(stdout,
						"Failed to open .watchdogs/compiler.log");
					minimal_debugging();
				}

				timer_rate_compile =
					(post_end.tv_sec - pre_start.tv_sec) +
					(post_end.tv_nsec - pre_start.tv_nsec) /
					1e9;

				printf("\n");
				pr_color(stdout, FCOLOUR_CYAN,
					" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
					timer_rate_compile,
					timer_rate_compile * 1000.0);
				if (timer_rate_compile > 36) {
					printf(
						"~ This is taking a while, huh?\n"
						"  Make sure you've cleared all the warnings,\n"
						"  you're using the latest compiler,\n"
						"  and double-check that your logic\n"
						"  and pawn algorithm tweaks in the gamemode scripts line up.\n"
						"  make sure " FCOLOUR_CYAN
						"'MAX_PLAYERS'" FCOLOUR_DEFAULT " and"
						FCOLOUR_CYAN "'MAX_VEHICLES'" FCOLOUR_DEFAULT "is normal macro value\n");
					fflush(stdout);
				}
			} else {
				printf(
					"Cannot locate input: " FCOLOUR_CYAN
					"%s" FCOLOUR_DEFAULT
					" - No such file or directory\n",
					compile_args_val);
				goto compiler_end;
			}
		}

		if (this_proc_file)
			fclose(this_proc_file);

		memset(multi_buf, __compiler_rate_zero, sizeof(multi_buf));

		this_proc_file = fopen(".watchdogs/compiler.log", "rb");

		{
			static int k = 0;
			if (k == 0 && this_proc_file != NULL &&
				compiler_retrying != 1 && has_compat != 1) {
				rewind(this_proc_file);
				while (fgets(multi_buf, sizeof(multi_buf),
					this_proc_file) != NULL) {
					if (strfind(multi_buf, "error",
						true) != false) {
						printf(FCOLOUR_BCYAN
							"*** compile exit with failed. retrying.."
							BKG_DEFAULT);
						sleep(2);
						compiler_retrying = true;
						++k;
						goto _compiler_retrying;
					}
				}
			}
		}

		if (this_proc_file)
			fclose(this_proc_file);

		goto compiler_end;
		
	} else {
		printf("\033[1;31merror:\033[0m pawncc (our compiler) not found\n"
		    "  \033[2mhelp:\033[0m install it before continuing");
		printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

		pointer_signalA = readline("");

		if (pointer_signalA && (pointer_signalA[0] == '\0' ||
		    strcmp(pointer_signalA, "Y") == 0 ||
		    strcmp(pointer_signalA, "y") == 0)) {
			dog_free(pointer_signalA);
			if (path_exists(".watchdogs/compiler.log") != 0) {
				remove(".watchdogs/compiler.log");
			}
			unit_ret_main("pawncc");
		}
		dog_free(pointer_signalA);
	}

compiler_end:
	return 1;
}
