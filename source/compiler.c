/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <errno.h>
#include  <string.h>
#include  <unistd.h>
#include  <limits.h>
#include  <time.h>

#ifdef DOG_LINUX
	#include  <fcntl.h>
	#include  <sys/types.h>
	#include  <sys/wait.h>
#endif

#include  "utils.h"
#include  "units.h"
#include  "library.h"
#include  "debug.h"
#include  "crypto.h"
#include  "cause.h"
#include  "compiler.h"

const CompilerOption object_opt[] = {
	{ BIT_FLAG_DEBUG,     COMPILER_WHITESPACE "-d:2" COMPILER_WHITESPACE, 5 },
	{ BIT_FLAG_ASSEMBLER, COMPILER_WHITESPACE "-a"   COMPILER_WHITESPACE, 4 },
	{ BIT_FLAG_COMPAT,    COMPILER_WHITESPACE "-Z:+" COMPILER_WHITESPACE, 5 },
	{ BIT_FLAG_PROLIX,    COMPILER_WHITESPACE "-v:2" COMPILER_WHITESPACE, 5 },
	{ BIT_FLAG_COMPACT,   COMPILER_WHITESPACE "-C:+" COMPILER_WHITESPACE, 5 },
	{ BIT_FLAG_TIME,      COMPILER_WHITESPACE "-d:3" COMPILER_WHITESPACE, 5 },
	{ 0, NULL, 0 }
};

static struct
timespec pre_start = { compiler_rate_zero },
post_end           = { compiler_rate_zero };
static double  timer_rate_compile;
static         io_compilers
			    dog_compiler_sys = { compiler_rate_zero };
static FILE   *this_proc_file = NULL;
static int     compiler_retrying = 0;
bool           compilr_with_debugging = false;
bool           compiler_debugging = false;
bool           has_debug = false;
static bool    has_detailed = false;
static bool    has_clean = false;
static bool    has_assembler = false;
static bool    has_compat = false;
static bool    has_prolix = false;
static bool    has_compact = false;
static bool    has_time = false;
static bool    has_time_issue = false;
static bool    no_have_options = false;
static bool    rate_compiler_log_is_fail = false;

char          *compiler_full_includes = NULL;
static char    compiler_temp[DOG_PATH_MAX + 28] = { compiler_rate_zero };
static char    compiler_buf[DOG_MAX_PATH] = { compiler_rate_zero };
static char    compiler_mb[28] = { compiler_rate_zero };
static char    compiler_parsing[DOG_PATH_MAX] = { compiler_rate_zero };
static char    compiler_path_include_buf[DOG_PATH_MAX] = { compiler_rate_zero };
static char    compiler_input[DOG_MAX_PATH] = { compiler_rate_zero };
static char    compiler_extra_options[456] = { compiler_rate_zero };
static char   *compiler_proj_path = NULL;
static char   *compiler_size_last_slash = NULL;
static char   *compiler_back_slash = NULL;
static char   *size_include_extra = NULL;
static char   *procure_string_pos = NULL;
static char   *pointer_signalA = NULL;
static char   *compler_project = NULL;
static char   *compiler_unix_token = NULL;
static char   *dog_compiler_unix_args[DOG_MAX_PATH] = { NULL };
#ifdef DOG_WINDOWS
static         PROCESS_INFORMATION _PROCESS_INFO;
static         STARTUPINFO         _STARTUPINFO;
static         SECURITY_ATTRIBUTES _ATTRIBUTES;
#endif

static OptionMap compiler_all_flag_map[] = {
    {"--detailed",       "-w",   &has_detailed},
    {"--watchdogs",      "-w",   &has_detailed},
    {"--debug",          "-d",   &has_debug},
    {"--clean",          "-n",   &has_clean},
    {"--assembler",      "-a",   &has_assembler},
    {"--compat",         "-c",   &has_compat},
    {"--compact",        "-s",   &has_compact},
    {"--prolix",         "-p",   &has_prolix},
    {"--fast",           "-f",   &has_time},
    {NULL, NULL, NULL}
};

void
compiler_refresh_data ( void )  {

	if ( dir_exists( ".watchdogs" ) == 0 )
		MKDIR( ".watchdogs" );

	#ifdef DOG_LINUX
	static const char *usr_paths[] = {
		LINUX_LIB_PATH, LINUX_LIB32_PATH, TMUX_LIB_PATH,
		TMUX_LIB_LOC_PATH, TMUX_LIB_ARM64_PATH, TMUX_LIB_ARM32_PATH,
		TMUX_LIB_AMD64_PATH, TMUX_LIB_AMD32_PATH
	};
	static bool rate_export_path = false;

	if (rate_export_path == false) {
		size_t counts = 0;
		counts = sizeof(usr_paths) / sizeof(usr_paths[0]);

		char pnew[DOG_MAX_PATH], sopath[DOG_PATH_MAX];
		const char
			*pold = getenv("LD_LIBRARY_PATH");
		if (!pold)
			pold = "";

		snprintf(pnew, sizeof(pnew), "%s", pold);

		for (size_t i = 0; i < counts; i++) {
			snprintf(sopath, sizeof(sopath), "%s/libpawnc.so", usr_paths[i]);
			if (path_exists(sopath) != 0) {
				if ('\0' != pnew[0])
					strncat(pnew, ":", sizeof(pnew) - strlen(pnew) - 1);
				strncat(pnew, usr_paths[i], sizeof(pnew) - strlen(pnew) - 1);
			}
		}

		if ('\0' != pnew[0]) {
			setenv("LD_LIBRARY_PATH", pnew, 1);
			rate_export_path = true;
		} else {
			pr_warning(stdout,
				"libpawnc.so not found in any target path..");
		}
	}
	#endif
	
	/* sef reset */
	dog_sef_path_revert();

	/* time reset */
	memset(&pre_start, compiler_rate_zero, sizeof(pre_start));
	memset(&post_end, compiler_rate_zero, sizeof(post_end));

	/* bool reset */
	has_detailed = false, has_debug = false,
	has_clean = false, has_assembler = false,
	has_compat = false, has_prolix = false,
	has_compact = false, has_time_issue = false,
	no_have_options = false, rate_compiler_log_is_fail = false,
	compiler_retrying = 0;

	/* pointer reset */
	this_proc_file = NULL, compiler_size_last_slash = NULL,
	compiler_back_slash = NULL, size_include_extra = NULL,
	procure_string_pos = NULL, pointer_signalA = NULL,
	compler_project = NULL,
	compiler_unix_token = NULL;
	dog_compiler_sys.container_output = NULL;

	/* memory reset */
    memset(dog_compiler_sys.compiler_direct_path, compiler_rate_zero,
    	sizeof(dog_compiler_sys.compiler_direct_path));
    memset(dog_compiler_sys.compiler_size_file_name, compiler_rate_zero,
    	sizeof(dog_compiler_sys.compiler_size_file_name));
    memset(dog_compiler_sys.compiler_size_input_path, compiler_rate_zero,
    	sizeof(dog_compiler_sys.compiler_size_input_path));
    memset(dog_compiler_sys.compiler_size_temp, compiler_rate_zero,
    	sizeof(dog_compiler_sys.compiler_size_temp));
	memset(compiler_parsing, compiler_rate_zero, sizeof(compiler_parsing));
	memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));
	memset(compiler_mb, compiler_rate_zero, sizeof(compiler_mb));
	memset(compiler_path_include_buf, compiler_rate_zero,
	    sizeof(compiler_path_include_buf));
	memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
	memset(compiler_input, compiler_rate_zero, sizeof(compiler_input));
	memset(compiler_extra_options, compiler_rate_zero,
	    sizeof(compiler_extra_options));
	memset(dog_compiler_unix_args, compiler_rate_zero,
	    sizeof(dog_compiler_unix_args));
}

long compiler_get_milisec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void compiler_stage_trying(const char *stage, int ms) {
    long start = compiler_get_milisec();
    while (compiler_get_milisec() - start < ms) {
        printf("\r%s....", stage);
        fflush(stdout);
    }
	if (strcmp(stage, "AMX Output File??") == 0) {
		printf("\r\033[2K");
        fflush(stdout);
		print( "   |      A [=] Processing....................................\n");
		print( "   |  O|  C  '> Preparing the ? Process: vfork [right?].......\n");
		print( "   |  G|  D  '  *  CreateProcess/posix_spawn/fork.............\n");
		print( "   |      E  '> Preparing to compile [right?]................|\n");
		print( " 		  F  '> Preprocessing [right?]...............|\n");
		print( " 		  F  '> Parsing [right?].....................|\n");
		print( " 		  G  '> Semantic Analysis [right?]...........|\n");
		print( " 		  H  '> AMX Code Generation [right?].........|\n");
		print( " 		  I  '> AMX Output File Generation [right?]..|\n");
		print( "** Preparing all tasks..\n");
	}
}

static
int dog_exec_compiler_process(char *pawncc_path,
							  char *input_path,
							  char *output_path) {
	static bool rate_init_proc = false;
	if (rate_init_proc == false) {
		if ( path_exists("pawno") == 1 )
			{
				set_default_access("pawno");
				if (path_exists("pawno/include"))
					set_default_access("pawno/include");
			}
		if ( path_exists("qawno") == 1 )
			{
				set_default_access("qawno");
				if (path_exists("qawno/include"))
					set_default_access("qawno/include");
			}
		if ( path_exists("gamemodes") == 1 )
			{
				set_default_access("gamemodes");
			}
		set_default_access( pawncc_path );
		set_default_access( input_path );
		rate_init_proc = true;
	}

	memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));
	#ifdef DOG_WINDOWS
		snprintf(compiler_temp, sizeof(compiler_temp), "%s", ".watchdogs\\compiler.log");
	#else	
		snprintf(compiler_temp, sizeof(compiler_temp), "%s", ".watchdogs/compiler.log");
	#endif
	char *compiler_temp2 = strdup(compiler_temp);

    print(      "** Thinking all task..\n");
    compiler_stage_trying(
			"Processing Process??",                        16);
    compiler_stage_trying(
			"Compiling??",                                 16);
    compiler_stage_trying(
			"Preprocessing??",                             16);
    compiler_stage_trying(
			"Parsing??",                                   16);
    compiler_stage_trying(
			"Semantic Analysis?",                          16);
    compiler_stage_trying(
			"AMX Code Generation?",                        16);
    compiler_stage_trying(
			"AMX Output File??",                           16);

	#ifdef DOG_WINDOWS
		_ZERO_MEM_WIN32(&_STARTUPINFO,
			sizeof(_STARTUPINFO));
		_STARTUPINFO.cb = sizeof(_STARTUPINFO);

		_ZERO_MEM_WIN32(&_ATTRIBUTES, sizeof(_ATTRIBUTES));
		_ATTRIBUTES.nLength = sizeof(_ATTRIBUTES);
		_ATTRIBUTES.bInheritHandle = TRUE;
		_ATTRIBUTES.lpSecurityDescriptor = NULL;

		_ZERO_MEM_WIN32(&_PROCESS_INFO,
			sizeof(_PROCESS_INFO));

		HANDLE hFile = CreateFileA(
			compiler_temp2,
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
			pawncc_path,
			input_path,
			output_path,
			dogconfig.dog_toml_all_flags,
			compiler_full_includes,
			compiler_path_include_buf);

		if (compilr_with_debugging == true) {
		#ifdef DOG_ANDROID
			println(stdout, "%s", compiler_input);
		#else
			dog_console_title(compiler_input);
			println(stdout, "%s", compiler_input);
		#endif
		}

		if (ret_command < 0 ||
			ret_command >= sizeof(compiler_input)) {
			pr_error(stdout,
				"ret_compiler too long!");
			minimal_debugging();
			if (compiler_temp2)
				{
					free(compiler_temp2);
					compiler_temp2 = NULL;
				}
			return (-2);
		}

		BOOL win32_process_success;
		win32_process_success = CreateProcessA(
			NULL, compiler_input,
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
			pawncc_path,
			input_path,
			output_path,
			dogconfig.dog_toml_all_flags,
			compiler_full_includes,
			compiler_path_include_buf);

		if (ret_command < 0 ||
			ret_command >= sizeof(compiler_input)) {
			pr_error(stdout,
				"ret_compiler too long!");
			minimal_debugging();
			if (compiler_temp2)
				{
					free(compiler_temp2);
					compiler_temp2 = NULL;
				}
			return (-2);
		}

		if (compilr_with_debugging == true) {
		#ifdef DOG_ANDROID
			println(stdout, "%s", compiler_input);
		#else
			dog_console_title(compiler_input);
			println(stdout, "%s", compiler_input);
		#endif
		}

		int i = 0;
		char *token_pointer = NULL;
		compiler_unix_token = strtok_r(
			compiler_input, " ", &token_pointer);
		while (compiler_unix_token != NULL &&
			i < (DOG_MAX_PATH + 255)) {
			dog_compiler_unix_args[i++] =
				compiler_unix_token;
			compiler_unix_token = strtok_r(NULL,
				" ", &token_pointer);
		}
		dog_compiler_unix_args[i] = NULL;

		#ifdef DOG_ANDROID
			static bool child_method = false;
			if (has_time == 1) {
				child_method = true;
			}
			pid_t compiler_process_id;
			if (child_method == false) {
				compiler_process_id = fork();
			} else {
				compiler_process_id = vfork();
			}
			if (compiler_process_id == 0) {
				int logging_file = open(
					compiler_temp2,
					O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (logging_file != -1) {
					dup2(logging_file, STDOUT_FILENO);
					dup2(logging_file, STDERR_FILENO);
					close(logging_file);
				} else {
					rate_compiler_log_is_fail = true;
				}
				execv(dog_compiler_unix_args[0], dog_compiler_unix_args);
				fprintf(stderr, "execv failed: %s\n", strerror(errno));
				_exit(127);
			} else if (compiler_process_id > 0) {
				int process_status;
				int process_timeout_occurred = 0;
				clock_gettime(CLOCK_MONOTONIC, &pre_start);
				for (int k = 0; k < POSIX_TIMEOUT; k++) {
					int p_result = waitpid(
						compiler_process_id,
						&process_status,
						WNOHANG);
					if (p_result == 0) {
						usleep(100000);
					} else if (p_result == compiler_process_id) {
						break;
					} else {
						pr_error(stdout, "waitpid error");
						minimal_debugging();
						break;
					}
					if (k == POSIX_TIMEOUT - 1) {
						kill(compiler_process_id, SIGTERM);
						sleep(2);
						kill(compiler_process_id, SIGKILL);
						pr_error(stdout,
							"process execution timeout! (%d seconds)",
							POSIX_TIMEOUT);
						minimal_debugging();
						waitpid(
							compiler_process_id,
							&process_status,
							0);
						process_timeout_occurred = 1;
					}
				}
				clock_gettime(CLOCK_MONOTONIC, &post_end);
				if (!process_timeout_occurred) {
					if (WIFEXITED(process_status)) {
						int proc_exit_code =
							WEXITSTATUS(process_status);
						if (proc_exit_code != 0 &&
						proc_exit_code != 1) {
							pr_error(stdout,
								"compiler process exited with code (%d)",
								proc_exit_code);
							minimal_debugging();
						}
					} else if (WIFSIGNALED(process_status)) {
						pr_error(stdout,
							"compiler process terminated by signal (%d)",
							WTERMSIG(process_status));
					}
				}
			} else {
				pr_error(stdout,
					"process creation failed: %s",
					strerror(errno));
				minimal_debugging();
			}
		#else
			posix_spawn_file_actions_t process_file_actions;
			posix_spawn_file_actions_init(
				&process_file_actions);
			int posix_logging_file = open(
				compiler_temp2,
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
				posix_spawn_file_actions_addclose(&process_file_actions,
						posix_logging_file);
			} else {
				rate_compiler_log_is_fail = true;
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
				for (int k = 0; k < POSIX_TIMEOUT; k++) {
					int p_result = -1;
					p_result = waitpid(
						compiler_process_id,
						&process_status, WNOHANG);
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
					if (k == POSIX_TIMEOUT - 1) {
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
				clock_gettime(CLOCK_MONOTONIC, &post_end);
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

	if (compiler_temp2)
		{
			free(compiler_temp2);
			compiler_temp2 = NULL;
		}
	return (0);
}

int
dog_exec_compiler(const char *args, const char *compile_args_val,
    const char *second_arg, const char *four_arg, const char *five_arg,
    const char *six_arg, const char *seven_arg, const char *eight_arg,
    const char *nine_arg, const char *ten_arg)
{
	io_compilers   compiler_field;
	io_compilers  *kevlar_compiler = &compiler_field;
	int            rate_pawnc_is_exist = 0;
	char          *pawnc_pointer = NULL;
	size_t	       rate_sef_entries;
	char          *gamemodes_slash = "gamemodes/";
	char          *gamemodes_back_slash = "gamemodes\\";

	compiler_refresh_data();

	const char *argv_buf[] = {
		second_arg,four_arg,five_arg,six_arg,seven_arg,eight_arg,nine_arg,ten_arg
	};

	if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0) {
		/* windows : pawncc.exe - wsl/wsl2 - native / msys2 */
		pawnc_pointer = "pawncc.exe";
	} else if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_LINUX) == 0) {
		/* linux : pawncc */
		pawnc_pointer = "pawncc";
	}

	if (dir_exists("pawno") != 0 && dir_exists("qawno") != 0) {
		rate_pawnc_is_exist = dog_find_path("pawno", pawnc_pointer,
			NULL);
		if (rate_pawnc_is_exist) {
			;
		} else {
			rate_pawnc_is_exist = dog_find_path("qawno",
				pawnc_pointer, NULL);
			if (rate_pawnc_is_exist < 1) {
				rate_pawnc_is_exist = dog_find_path(".",
					pawnc_pointer, NULL);
			}
		}
	} else if (dir_exists("pawno") != 0) {
		rate_pawnc_is_exist = dog_find_path("pawno", pawnc_pointer,
			NULL);
		if (rate_pawnc_is_exist) {
			;
		} else {
			rate_pawnc_is_exist = dog_find_path(".",
				pawnc_pointer, NULL);
		}
	} else if (dir_exists("qawno") != 0) {
		rate_pawnc_is_exist = dog_find_path("qawno", pawnc_pointer,
			NULL);
		if (rate_pawnc_is_exist) {
			;
		} else {
			rate_pawnc_is_exist = dog_find_path(".",
				pawnc_pointer, NULL);
		}
	} else {
		rate_pawnc_is_exist = dog_find_path(".", pawnc_pointer,
			NULL);
	}

	if (rate_pawnc_is_exist != 0) {
		for (int i = 0; i < compiler_rate_all_args && argv_buf[i] != NULL; ++i) {
			const char *arg = argv_buf[i];
			
			if (arg[0] != '-') continue;
			
			for (OptionMap *opt = compiler_all_flag_map; opt->full_name; ++opt) {
				if (strcmp(arg, opt->full_name) == 0 || 
					strcmp(arg, opt->short_name) == 0) {
					*(opt->flag_ptr) = true;
					break;
				}
			}
		}

		if (!has_detailed && !has_debug && !has_clean &&
			!has_assembler && !has_compat && !has_prolix && !has_compact)
			{
				no_have_options = true;
			}

_compiler_retrying:
		if (compiler_retrying == 1) {
			has_compat = true, has_compact = true;
			has_time = true, has_detailed = true;
			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s MAX_PLAYERS=50 MAX_VEHICLES=50 MAX_ACTORS=50 MAX_OBJECTS=1000",
				dogconfig.dog_toml_all_flags);
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);
		} else if (compiler_retrying == 2) {
			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s MAX_PLAYERS=100 MAX_VEHICLES=1000 MAX_ACTORS=100 MAX_OBJECTS=2000",
				dogconfig.dog_toml_all_flags);
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);

			if (compiler_full_includes) {
				free(compiler_full_includes);
				compiler_full_includes = NULL;
				compiler_full_includes = strdup(" ");
			}
			goto next_;
		}
		if (has_time_issue) {
			has_compat = true, has_compact = true;
			has_time = true, has_detailed = true;
			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s MAX_PLAYERS=50 MAX_VEHICLES=50 MAX_ACTORS=50 MAX_OBJECTS=1000",
				dogconfig.dog_toml_all_flags);
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);
		}

		unsigned int flags_map = 0;

		if (has_debug)
			/* bit 0 = 1 (0x01) */
			flags_map |= BIT_FLAG_DEBUG;

		if (has_assembler)
			/* bit 1 = 1 (0x03) */
			flags_map |= BIT_FLAG_ASSEMBLER;

		if (has_compat)
			/* bit 2 = 1 (0x07) */
			flags_map |= BIT_FLAG_COMPAT;

		if (has_prolix)
			/* bit 3 = 1 (0x0F) */
			flags_map |= BIT_FLAG_PROLIX;

		if (has_compact)
			/* bit 4 = 1 (0x1F) */
			flags_map |= BIT_FLAG_COMPACT;

		if (has_time)
			/* bit 5 = 1 (0x20) */
			flags_map |= BIT_FLAG_TIME;

		char *p = compiler_extra_options;
		p += strlen(p);

		for (int i = 0; object_opt[i].option; i++) {
			if (!(flags_map & object_opt[i].flag))
				continue;

			memcpy(p, object_opt[i].option,
				object_opt[i].len);
			p += object_opt[i].len;
		}

		*p = '\0';
		
	next_:
		if (compiler_retrying == 2)
			{
				compiler_extra_options[0] = '\0';
			}
		if (has_detailed)
			{
				compilr_with_debugging = true;
			}
#if defined(_DBG_PRINT)
		{
			compilr_with_debugging = true;
		}
#endif
		if (strlen(compiler_extra_options) > 0) {
			size_t calcute_len_all_flags = 0;

			if (dogconfig.dog_toml_all_flags) {
				calcute_len_all_flags =
					strlen(dogconfig.dog_toml_all_flags);
			} else {
				dogconfig.dog_toml_all_flags = strdup("");
			}

			size_t extra_len =
				strlen(compiler_extra_options);
			char *new_ptr = dog_realloc(
				dogconfig.dog_toml_all_flags,
				calcute_len_all_flags + extra_len + 1);

			if (!new_ptr) {
				pr_error(stdout,
					"Memory allocation failed for extra options");
				return (-2);
			}

			dogconfig.dog_toml_all_flags = new_ptr;
			strcat(dogconfig.dog_toml_all_flags,
				compiler_extra_options);
		}

		if (strfind(compile_args_val, _SYM_PARENT_DIR, true) !=
			false) {
			size_t write_pos = 0, j;
			bool rate_parent_dir = false;
			for (j = 0; compile_args_val[j] != '\0';) {
				if (!rate_parent_dir &&
					strncmp(&compile_args_val[j],
					_SYM_PARENT_DIR, 3) == 0) {
					j += 3;
					while (compile_args_val[j] !=
						'\0' &&
						compile_args_val[j] != ' ' &&
						compile_args_val[j] !=
						'"') {
						compiler_parsing[write_pos++] =
							compile_args_val[j++];
					}
					size_t read_cur = 0, scan_idx;
					for (scan_idx = 0;
						scan_idx < write_pos;
						scan_idx++) {
						if (compiler_parsing[scan_idx] ==
							_PATH_CHR_SEP_POSIX ||
							compiler_parsing[scan_idx] ==
							_PATH_CHR_SEP_WIN32) {
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
				memmove(compiler_parsing + 3, compiler_parsing, write_pos);
				memcpy(compiler_parsing, _SYM_PARENT_DIR, 3);
				write_pos += 3;
				compiler_parsing[write_pos] = '\0';
				if (compiler_parsing[write_pos - 1] !=
					_PATH_CHR_SEP_POSIX &&
					compiler_parsing[write_pos - 1] !=
					_PATH_CHR_SEP_WIN32)
					strcat(compiler_parsing, "/");
			} else {
				strcpy(compiler_parsing, _SYM_PARENT_DIR);
			}

			memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));

			strcpy(compiler_temp, compiler_parsing);

			if (strstr(compiler_temp, gamemodes_slash) ||
				strstr(compiler_temp, gamemodes_back_slash)) {
				char *pos = strstr(compiler_temp,
					gamemodes_slash);
				if (!pos)
					pos = strstr(compiler_temp,
						gamemodes_back_slash);
				if (pos)
					*pos = '\0';
			}

			char *ret = strdup(dogconfig.dog_toml_all_flags);

			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));

			if (!strstr(ret, "gamemodes/") &&
				!strstr(ret, "pawno/include/") &&
				!strstr(ret, "qawno/include/")) {
					snprintf(compiler_buf, sizeof(compiler_buf),
						"-i" "=%s "
						"-i" "=%s" "gamemodes/ "
						"-i" "=%s" "pawno/include/ "
						"-i" "=%s" "qawno/include/ ",
						compiler_temp, compiler_temp, compiler_temp, compiler_temp);
			} else {
					snprintf(compiler_buf, sizeof(compiler_buf),
						"-i" "=%s ",
						compiler_temp);
			}

			if (ret) {
				free(ret);
				ret = NULL;
			}

			strncpy(compiler_path_include_buf, compiler_buf,
				sizeof(compiler_path_include_buf) - 1);
			compiler_path_include_buf[
				sizeof(compiler_path_include_buf) - 1] = '\0';
		}

		static bool rate_flag_notice = false;
		if (!rate_flag_notice && no_have_options) {
			printf("\n");
			compiler_show_tip();
			printf("\n");
			fflush(stdout);
			rate_flag_notice = true;
		}

		if (strfind(dogconfig.dog_toml_proj_compile, "localhost", true) == false)
		{
			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s LOCALHOST=0 MYSQL_LOCALHOST=0 SQL_LOCALHOST=0 LOCAL_SERVER=0",
				dogconfig.dog_toml_all_flags);
			dog_free(dogconfig.dog_toml_all_flags);
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);
		} else {
			memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s LOCALHOST=1 MYSQL_LOCALHOST=1 SQL_LOCALHOST=1 LOCAL_SERVER=1",
				dogconfig.dog_toml_all_flags);
			dog_free(dogconfig.dog_toml_all_flags);
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);
		}

		memset(compiler_buf, compiler_rate_zero, sizeof(compiler_buf));
#ifdef DOG_ANDROID
		/* disable warning 200 (truncated char) by default from compiler. */
		snprintf(compiler_buf, sizeof(compiler_buf), "%s -w:200-",
			dogconfig.dog_toml_all_flags);
		dog_free(dogconfig.dog_toml_all_flags);
		dogconfig.dog_toml_all_flags = strdup(compiler_buf);
#endif

		printf(DOG_COL_DEFAULT);

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
			bool _garbage = !dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILER_TARGET];
			if (_garbage && strlen(compile_args_val) < 1)
			{
				pr_color(stdout, DOG_COL_YELLOW,
					DOG_COL_BYELLOW
					"[== COMPILER TARGET ==]\n");
				printf(DOG_COL_BCYAN);
				printf(
					"   ** You run the compiler command "
					"without any args like: compile . compile mode.pwn\n"
					"   * Do you want to compile for "
					DOG_COL_GREEN "%s " DOG_COL_BCYAN
					"(enter), \n"
					"   * or do you want to compile for something else?\n",
					dogconfig.dog_toml_proj_input);
				print_restore_color();
				printf(DOG_COL_CYAN ">"
					DOG_COL_DEFAULT);
				fflush(stdout);
				compler_project = readline(" ");
				if (compler_project &&
					strlen(compler_project) > 0) {
					dog_free(
						dogconfig.dog_toml_proj_input);
					dogconfig.dog_toml_proj_input =
						strdup(compler_project);
					if (!dogconfig.dog_toml_proj_input) {
						pr_error(stdout,
							"Memory allocation failed");
						dog_free(compler_project);
						goto compiler_end;
					}
				}
				dog_free(compler_project);
				compler_project = NULL;
				dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILER_TARGET] = DOG_GARBAGE_TRUE;
			}

			int _process = dog_exec_compiler_process(
					dogconfig.dog_sef_found_list[0],
					dogconfig.dog_toml_proj_input,
					dogconfig.dog_toml_proj_output);
			if (_process != 0) {
				goto compiler_end;
			}

			if (path_exists(".watchdogs/compiler.log")) {
				printf("\n");
				char *ca = NULL;
				ca = dogconfig.dog_toml_proj_output;
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
					if (rate_compiler_log_is_fail == false)
						dog_printfile(
						".watchdogs/compiler.log");
					if (path_exists(ca)) {
						remove(ca);
					}
					goto compiler_done;
				}

				if (rate_compiler_log_is_fail == false)
					dog_printfile(
						".watchdogs/compiler.log");
			}
compiler_done:
			this_proc_file = fopen(".watchdogs/compiler.log",
				"r");
			if (this_proc_file) {
				bool has_err = false;
				while (fgets(compiler_mb,
					sizeof(compiler_mb),
					this_proc_file)) {
					if (strfind(compiler_mb,
						"error", true)) {
						has_err = false;
						break;
					}
				}
				fclose(this_proc_file);
				this_proc_file = NULL;
				if (has_err) {
					if (dogconfig.dog_toml_proj_output != NULL && path_access(dogconfig.dog_toml_proj_output))
						remove(dogconfig.dog_toml_proj_output);
					dogconfig.dog_compiler_stat = 1;
				} else {
					dogconfig.dog_compiler_stat = 0;
				}
			} else {
				pr_error(stdout,
					"Failed to open .watchdogs/compiler.log");
				minimal_debugging();
			}

			timer_rate_compile = ((double)(post_end.tv_sec - pre_start.tv_sec)) +
			                     ((double)(post_end.tv_nsec - pre_start.tv_nsec)) / 1e9;

			printf("\n");
			pr_color(stdout, DOG_COL_CYAN,
				" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
				timer_rate_compile,
				timer_rate_compile * 1000.0);
			if (timer_rate_compile > 300) {
				goto _print_time;
			}
		} else {

            if (strfind(compile_args_val, ".pwn", true) == false &&
                strfind(compile_args_val, ".p", true) == false)
            {
                pr_warning(stdout, "The compiler only accepts '.p' or '.pwn' files.");
                goto compiler_end;
            }

			strncpy(kevlar_compiler->compiler_size_temp,
				compile_args_val,
				sizeof(kevlar_compiler->compiler_size_temp) -
				1);
			kevlar_compiler->compiler_size_temp[
				sizeof(kevlar_compiler->compiler_size_temp) -
				1] = '\0';

			compiler_size_last_slash = strrchr(
				kevlar_compiler->compiler_size_temp,
				_PATH_CHR_SEP_POSIX);
			compiler_back_slash = strrchr(
				kevlar_compiler->compiler_size_temp,
				_PATH_CHR_SEP_WIN32);

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
			compiler_finding_compile_args = dog_find_path(
				kevlar_compiler->compiler_direct_path,
				kevlar_compiler->compiler_size_file_name,
				NULL);

			if (!compiler_finding_compile_args &&
				strcmp(kevlar_compiler->compiler_direct_path,
				"gamemodes") != 0) {
				compiler_finding_compile_args =
					dog_find_path("gamemodes",
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
					dog_find_path("gamemodes",
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

			rate_sef_entries = sizeof(dogconfig.dog_sef_found_list) /
							   sizeof(dogconfig.dog_sef_found_list[0]);

			for (int i = 0; i < rate_sef_entries; i++) {
				if (strfind(dogconfig.dog_sef_found_list[i], compile_args_val, true)) {
					memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));
					snprintf(compiler_temp,
						sizeof(compiler_temp), "%s",
						dogconfig.dog_sef_found_list[i]);
					if (compiler_proj_path)
						{
							free(compiler_proj_path);
							compiler_proj_path = NULL;
						}
					compiler_proj_path = strdup(compiler_temp);
				}
			}

			if (path_exists(compiler_proj_path) == 1) {
				if (compiler_proj_path) {
					memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));
					strncpy(compiler_temp, compiler_proj_path,
						sizeof(compiler_temp) - 1);
					compiler_temp[sizeof(compiler_temp) - 1] = '\0';
				} else {
					memset(compiler_temp, compiler_rate_zero, sizeof(compiler_temp));
				}

				char *extension = strrchr(compiler_temp,
					'.');
				if (extension)
					*extension = '\0';
				
				if (kevlar_compiler->container_output)
					{
						free(kevlar_compiler->container_output);
						kevlar_compiler->container_output = NULL;
					}
				kevlar_compiler->container_output = strdup(compiler_temp);

				snprintf(compiler_temp, sizeof(compiler_temp),
					"%s.amx", kevlar_compiler->container_output);
					
				char *compiler_temp2 = strdup(compiler_temp);
				
				int _process = dog_exec_compiler_process(
						dogconfig.dog_sef_found_list[0],
						compiler_proj_path,
						compiler_temp2);
				if (_process != 0) {
					goto compiler_end;
				}

				if (compiler_proj_path) {
					free(compiler_proj_path);
					compiler_proj_path = NULL;
				}

				if (path_exists(
					".watchdogs/compiler.log")) {
					printf("\n");
					char *ca = NULL;
					ca = compiler_temp2;
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
						if (rate_compiler_log_is_fail == false)
							dog_printfile(
							".watchdogs/compiler.log");
						if (path_exists(ca)) {
							remove(ca);
						}
						goto compiler_done2;
					}

					if (rate_compiler_log_is_fail == false)
						dog_printfile(
							".watchdogs/compiler.log");
				}

compiler_done2:
				this_proc_file = fopen(
					".watchdogs/compiler.log", "r");
				memset(compiler_mb, compiler_rate_zero, sizeof(compiler_mb));
				if (this_proc_file) {
					bool has_err = false;
					while (fgets(
						compiler_mb,
						sizeof(compiler_mb),
						this_proc_file)) {
						if (strfind(
							compiler_mb,
							"error", true)) {
							has_err = true;
							break;
						}
					}
					fclose(this_proc_file);
					this_proc_file = NULL;
					if (has_err) {
						if (compiler_temp2 && path_access(compiler_temp2))
							remove(compiler_temp2);
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

				if (compiler_temp2)
					{
						free(compiler_temp2);
						compiler_temp2 = NULL;
					}

				timer_rate_compile = ((double)(post_end.tv_sec - pre_start.tv_sec)) +
				                     ((double)(post_end.tv_nsec - pre_start.tv_nsec)) / 1e9;
				                     
				printf("\n");
				pr_color(stdout, DOG_COL_CYAN,
					" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
					timer_rate_compile,
					timer_rate_compile * 1000.0);
				if (timer_rate_compile > 300) {
					goto _print_time;
				}
			} else {
				printf(
					"Cannot locate input: " DOG_COL_CYAN
					"%s" DOG_COL_DEFAULT
					" - No such file or directory\n",
					compile_args_val);
				goto compiler_end;
			}
		}

		if (this_proc_file)
			fclose(this_proc_file);

		memset(compiler_mb, compiler_rate_zero, sizeof(compiler_mb));

		this_proc_file = fopen(".watchdogs/compiler.log", "rb");

		if (!this_proc_file)
			goto compiler_end;
		if (has_time_issue)
			goto compiler_end;

		while (fgets(
				compiler_mb,
				sizeof(compiler_mb),
			this_proc_file) != NULL)
			{
			if (strfind(compiler_mb, "error",
				true) != false) {
					if (compiler_retrying == 0) {
						compiler_retrying = 1;
						printf(DOG_COL_BCYAN
							"** Compilation Process Exit with Failed. recompile: 2/%d\n"
							BKG_DEFAULT, compiler_retrying);
						sleep(2);
						goto _compiler_retrying;
					}
					if (compiler_retrying == 1) {
						compiler_retrying = 2;
						printf(DOG_COL_BCYAN
							"** Compilation Process Exit with Failed. recompile: 2/%d\n"
							BKG_DEFAULT, compiler_retrying);
						sleep(2);
						goto _compiler_retrying;
					}
				}
			}

		if (this_proc_file)
			fclose(this_proc_file);

		goto compiler_end;

	} else {
		print_restore_color();

		printf("\033[1;31merror:\033[0m pawncc (our compiler) not found\n"
		    "  \033[2mhelp:\033[0m install it before continuing\n");
		if ((getenv("WSL_INTEROP") || getenv("WSL_DISTRO_NAME")) &&
			strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0 &&
			(path_exists("pawno/pawncc") == 1 || path_exists("qawno/pawncc") == 1))
			{
				pr_info(stdout, "Remember, if you run Watchdogs on Windows..");
			}
		printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

		print_restore_color();
		
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

	memset(compiler_mb, compiler_rate_zero, sizeof(compiler_mb));

	this_proc_file = fopen(".watchdogs/compiler.log", "r");
	if (this_proc_file) {
		bool has_err = false;
		while (fgets(
			compiler_mb,
			sizeof(compiler_mb),
			this_proc_file)) {
			if (strfind(
				compiler_mb,
				"error", true)) {
				has_err = true;
			}
		}
		if (has_err == false) {
			print("** Task Done!.\n");
		}
		fclose(this_proc_file);
	}
		
compiler_end:
	return (1);
_print_time:
	print(
		"** Process is taking a while..\n");
	if (has_time_issue == false) {
		pr_info(stdout, "Retrying..");
		sleep(2);
		has_time_issue = true;
		goto _compiler_retrying;
	}
	return (1);
}
