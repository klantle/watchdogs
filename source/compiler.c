/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#include  "utils.h"
#include  "units.h"
#include  "library.h"
#include  "debug.h"
#include  "crypto.h"
#include  "cause.h"
#include  "compiler.h"

/*
 * Compiler option flags mapping table.
 * Maps bit flags to their corresponding command-line option strings
 * and lengths for the pawncc compiler.
 */
const CompilerOption object_opt[] = {
	{ BIT_FLAG_DEBUG,     " " "-d:2" " ", 5 },
	{ BIT_FLAG_ASSEMBLER, " " "-a"   " ", 4 },
	{ BIT_FLAG_COMPAT,    " " "-Z:+" " ", 5 },
	{ BIT_FLAG_PROLIX,    " " "-v:2" " ", 5 },
	{ BIT_FLAG_COMPACT,   " " "-C:+" " ", 5 },
	{ BIT_FLAG_TIME,      " " "-d:3" " ", 5 },
	{ 0, NULL, 0 }
};

/*
 * Command-line flag mapping table.
 * Maps long and short option names to their corresponding flag variables.
 */
static bool    compiler_dog_flag_detailed = false;	/* Detailed output flag */
bool           compiler_have_debug_flag = false;	/* Debug flag presence indicator */
static bool    compiler_dog_flag_clean = false;	/* Clean compilation flag */
static bool    compiler_dog_flag_asm = false;	/* Assembler output flag */
static bool    compiler_dog_flag_compat = false;	/* Compatibility mode flag */
static bool    compiler_dog_flag_prolix = false;	/* Verbose output flag */
static bool    compiler_dog_flag_compact = false;	/* Compact output flag */
static bool    compiler_dog_flag_fast = false;	/* Fast compilation flag */

static OptionMap compiler_all_flag_map[] = {
    {"--detailed",       "-w",   &compiler_dog_flag_detailed},
    {"--watchdogs",      "-w",   &compiler_dog_flag_detailed},
    {"--debug",          "-d",   &compiler_have_debug_flag},
    {"--clean",          "-n",   &compiler_dog_flag_clean},
    {"--assembler",      "-a",   &compiler_dog_flag_asm},
    {"--compat",         "-c",   &compiler_dog_flag_compat},
    {"--compact",        "-m",   &compiler_dog_flag_compact},
    {"--prolix",         "-p",   &compiler_dog_flag_prolix},
    {"--fast",           "-f",   &compiler_dog_flag_fast},
    {NULL, NULL, NULL}
};

/*
 * Timing structures for measuring compilation duration.
 * pre_start and post_end record the start and end times
 * of the compilation process.
 */
static struct
timespec pre_start = { 0 },
post_end           = { 0 };
static double  compiler_calculate_time;	/* Calculated compilation time in seconds */
static         io_compilers
			    dog_compiler_sys;		/* Compiler I/O context structure */
static FILE   *this_proc_file = NULL;	/* File handle for compiler log */
bool           compiler_installing_stdlib = NULL;	/* Flag indicating stdlib installation status */
bool           compiler_is_err = false;	/* Global error state flag */
static int     compiler_retry_stat = 0;	/* Retry attempt counter */
bool           compiler_input_debug = false;	/* Enable debug output for compiler input */
static bool    compiler_long_time = false;	/* Flag for long-running compilations */
bool           compiler_dog_flag_debug = false;	/* Debug mode flag */
static bool    compiler_empty_dog_flag = false;	/* No flags specified flag */
static bool    compiler_unix_file_fail = false;	/* Unix file operation failure flag */
char          *compiler_full_includes = NULL;	/* Full include path string */
static char    compiler_temp[DOG_PATH_MAX + 28] = { 0 };	/* Temporary path buffer */
static char    compiler_buf[DOG_MAX_PATH] = { 0 };	/* General purpose buffer */
static char    compiler_mb[128] = { 0 };	/* Small message buffer */
static char    compiler_parsing[DOG_PATH_MAX] = { 0 };	/* Path parsing buffer */
static char    compiler_path_include_buf[DOG_PATH_MAX] = { 0 };	/* Include path buffer */
static char    compiler_input[DOG_MAX_PATH] = { 0 };	/* Compiler command input buffer */
static char    compiler_dog_flag_list[456] = { 0 };	/* Flag list string buffer */
static char   *compiler_proj_path = NULL;	/* Project path pointer */
static char   *compiler_size_last_slash = NULL;	/* Last path separator position */
static char   *compiler_back_slash = NULL;	/* Backslash position pointer */
static char   *size_include_extra = NULL;	/* Extra include size pointer */
static char   *procure_string_pos = NULL;	/* String position pointer */
static char   *pointer_signalA = NULL;	/* Signal pointer for user input */
static char   *compiler_project = NULL;	/* Project name pointer */
static char   *compiler_unix_token = NULL;	/* Unix token pointer for strtok */
static char   *dog_compiler_unix_args[DOG_MAX_PATH] = { NULL };	/* Argument array for exec */
#ifdef DOG_WINDOWS
static         PROCESS_INFORMATION _PROCESS_INFO;	/* Windows process information */
static         STARTUPINFO         _STARTUPINFO;	/* Windows startup information */
static         SECURITY_ATTRIBUTES _ATTRIBUTES;	/* Windows security attributes */
#endif

/*
 * compiler_refresh_data
 * Reset all compiler state variables and memory buffers to their initial state.
 * Creates the .watchdogs directory if it doesn't exist and resets all
 * timing, boolean flags, pointers, and memory buffers.
 */
void
compiler_refresh_data ( void )  {

	if ( dir_exists( ".watchdogs" ) == 0 )
		MKDIR( ".watchdogs" );
	
	/* sef reset */
	dog_sef_path_revert();

	/* time reset */
	memset(&pre_start, 0, sizeof(pre_start));
	memset(&post_end, 0, sizeof(post_end));

	/* bool reset */
	compiler_dog_flag_detailed = false, compiler_dog_flag_debug = false,
	compiler_dog_flag_clean = false, compiler_dog_flag_asm = false,
	compiler_dog_flag_compat = false, compiler_dog_flag_prolix = false,
	compiler_dog_flag_compact = false, compiler_long_time = false,
	compiler_empty_dog_flag = false, compiler_unix_file_fail = false,
	compiler_retry_stat = 0;

	/* pointer reset */
	this_proc_file = NULL, compiler_size_last_slash = NULL,
	compiler_back_slash = NULL, size_include_extra = NULL,
	procure_string_pos = NULL, pointer_signalA = NULL,
	compiler_project = NULL,
	compiler_unix_token = NULL;

	/* memory reset */
	memset(&dog_compiler_sys, 0, sizeof(io_compilers));
	memset(compiler_parsing, 0, sizeof(compiler_parsing));
	memset(compiler_temp, 0, sizeof(compiler_temp));
	memset(compiler_mb, 0, sizeof(compiler_mb));
	memset(compiler_path_include_buf, 0,
	    sizeof(compiler_path_include_buf));
	memset(compiler_buf, 0, sizeof(compiler_buf));
	memset(compiler_input, 0, sizeof(compiler_input));
	memset(compiler_dog_flag_list, 0,
	    sizeof(compiler_dog_flag_list));
	memset(dog_compiler_unix_args, 0,
	    sizeof(dog_compiler_unix_args));
}

/*
 * compiler_configure_libpath
 * Configure the LD_LIBRARY_PATH environment variable on Linux systems
 * to include paths where libpawnc.so may be located. This function
 * searches through a predefined list of library paths and adds any
 * that contain the library to the environment variable.
 */
static void
compiler_configure_libpath(void)
{
#ifdef DOG_LINUX
	static const char *paths[] = {
		LINUX_LIB_PATH, LINUX_LIB32_PATH,
		TMUX_LIB_PATH, TMUX_LIB_LOC_PATH,
		TMUX_LIB_ARM64_PATH, TMUX_LIB_ARM32_PATH,
		TMUX_LIB_AMD64_PATH, TMUX_LIB_AMD32_PATH
	};
	static int done = 0;

	char buf[DOG_PATH_MAX * 2];
	char so[DOG_PATH_MAX];
	const char *old;
	size_t len = 0;
	size_t i;
	int n;

	if (done)
		return;

	buf[0] = '\0';
	old = getenv("LD_LIBRARY_PATH");

	if (old && *old) {
		len = strlcpy(buf, old, sizeof(buf));
		if (len >= sizeof(buf))
			len = sizeof(buf) - 1;
	}

	for (i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
		n = snprintf(so, sizeof(so), "%s/libpawnc.so", paths[i]);
		if (n < 0 || (size_t)n >= sizeof(so))
			continue;

		if (path_exists(so)) {
			if (len > 0 && len + 1 < sizeof(buf))
				buf[len++] = ':';

			len += strlcpy(buf + len, paths[i],
			    sizeof(buf) - len);
		}
	}

	if (len > 0) {
		setenv("LD_LIBRARY_PATH", buf, 1);
		done = 1;
	} else {
		pr_warning(stdout,
		    "libpawnc.so not found in any target path..");
	}
#endif
}

long compiler_get_milisec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return
    	ts.tv_sec * 1000 +
    	ts.tv_nsec / 1000000;
}

/*
 * compiler_stage_trying
 * Display a compilation stage message for a specified duration.
 * The function prints a stage name with dots and optionally displays
 * a detailed compilation progress tree when the final stage is reached.
 *
 * Parameters:
 *   stage: The name of the compilation stage to display
 *   ms: Duration in milliseconds to display the stage message
 */
void compiler_stage_trying(const char *stage, int ms) {
    long start = compiler_get_milisec();
    while (compiler_get_milisec() - start < ms) {
        printf("\r%s....", stage);
        fflush(stdout);
    }
	if (strcmp(stage, "AMX Output File??..") == 0) {
		printf("\r\033[2K");
        fflush(stdout);
		static const char *amx_stage_lines[] = {
			"   |  D) |  A [$] Processing............................................\n",
			"   |  O) |  B  '> Preparing the ? Process: vfork [right?]...............\n",
			"   |  G) |  C  '  *  CreateProcess/_beginthreadex/posix_spawn/fork......\n",
			"   |        D  '> Preparing to compile [right?].......................|\n",
			" 0  ?   E   '> Preprocessing [right?].................................|\n",
			" 0  X   F   '> Parsing [right?].......................................|\n",
			" 0  V   G   '> Semantic Analysis [right?].............................|\n",
			" 0  %   H   '> AMX Code Generation [right?]...........................|\n",
			" 1  $   I   '> AMX Output File Generation [right?]....................|\n",
			"** Preparing all tasks..\n",
			NULL
		};
		
		for (int i = 0; amx_stage_lines[i]; ++i) {
			print(amx_stage_lines[i]);
		}
	}
}

/*
 * dog_proj_init
 * Initialize the project compilation environment. This function
 * configures library paths, sets up default access paths for
 * gamemodes, pawno, and qawno directories, and displays compilation
 * stage progress messages.
 *
 * Parameters:
 *   input_path: Path to the input source file
 *   pawncc_path: Path to the pawncc compiler executable
 */
static
void dog_proj_init(char *input_path, char *pawncc_path) {

	compiler_configure_libpath();
	
	static bool rate_init_proc = false;
	if (rate_init_proc == false) {
		if ( path_exists("gamemodes") == 1 )
			__set_default_access("gamemodes");
		if ( path_exists("pawno") == 1 )
		{
			__set_default_access("pawno");
			if (path_exists("pawno/include"))
				__set_default_access("pawno/include");
		}
		if ( path_exists("qawno") == 1 )
		{
			__set_default_access("qawno");
			if (path_exists("qawno/include"))
				__set_default_access("qawno/include");
		}
		__set_default_access( pawncc_path );
		__set_default_access( input_path );
		rate_init_proc = true;
	}

    print(      "** Thinking all task..\n");
    compiler_stage_trying(
			"Processing Process??..",                         16);
    compiler_stage_trying(
			"Compiling??..",                                  16);
    compiler_stage_trying(
			"Preprocessing??..",                              16);
    compiler_stage_trying(
			"Parsing??..",                                    16);
    compiler_stage_trying(
			"Semantic Analysis??..",                          16);
    compiler_stage_trying(
			"AMX Code Generation??..",                        16);
    compiler_stage_trying(
			"AMX Output File??..",                            16);

    fflush(stdout);
}

#ifdef DOG_WINDOWS
/* Thread function for fast compilation using _beginthreadex */
static unsigned __stdcall
compiler_thread_func(void *arg) {
	compiler_thread_data_t *data = (compiler_thread_data_t *)arg;
	BOOL win32_process_success;

	/* Create Windows process for compiler execution */
	win32_process_success = CreateProcessA(
		NULL, data->compiler_input,
		NULL, NULL,
		TRUE,
		CREATE_NO_WINDOW |
		ABOVE_NORMAL_PRIORITY_CLASS |
		CREATE_BREAKAWAY_FROM_JOB,
		NULL, NULL,
		data->startup_info, data->process_info);

	if (data->hFile != INVALID_HANDLE_VALUE) {
		SetHandleInformation(data->hFile,
			HANDLE_FLAG_INHERIT, 0);
	}

	if (win32_process_success == TRUE) {
		SetThreadPriority(
			data->process_info->hThread,
			THREAD_PRIORITY_ABOVE_NORMAL);

		DWORD_PTR procMask, sysMask;
		GetProcessAffinityMask(
			GetCurrentProcess(),
			&procMask, &sysMask);
		SetProcessAffinityMask(
			data->process_info->hProcess,
			procMask & ~1);

		clock_gettime(CLOCK_MONOTONIC,
			data->pre_start);
		DWORD waitResult =
			WaitForSingleObject(
			data->process_info->hProcess,
			0x3E8000);
		if (waitResult == WAIT_TIMEOUT) {
			TerminateProcess(
				data->process_info->hProcess, 1);
			WaitForSingleObject(
				data->process_info->hProcess,
				5000);
		}
		clock_gettime(CLOCK_MONOTONIC,
			data->post_end);

		DWORD proc_exit_code;
		/* Retrieve process exit code for error reporting */
		GetExitCodeProcess(
			data->process_info->hProcess,
			&proc_exit_code);
#if defined(_DBG_PRINT)
		pr_info(stdout,
			"windows process exit with code: %lu",
			proc_exit_code);
		if (
			proc_exit_code ==
			3221225781)
		{
			pr_info(stdout,
				data->windows_redist_err);
			printf("%s", data->windows_redist_err2);
			fflush(stdout);
		}
#endif
		CloseHandle(data->process_info->hThread);
		CloseHandle(data->process_info->hProcess);

		if (data->startup_info->hStdOutput != NULL &&
			data->startup_info->hStdOutput != data->hFile)
			CloseHandle(
				data->startup_info->hStdOutput);
		if (data->startup_info->hStdError != NULL &&
			data->startup_info->hStdError != data->hFile)
			CloseHandle(
				data->startup_info->hStdError);
	} else {
		DWORD err = GetLastError();
		pr_error(stdout,
			"CreateProcess failed! (%lu)",
			err);
		if (strfind(strerror(err), "The system cannot find the file specified", true))
			pr_error(stdout, "^ The compiler executable does not exist.");
		if (strfind(strerror(err), "Access is denied", true))
			pr_error(stdout, "^ You do not have permission to execute the compiler executable.");
		if (strfind(strerror(err), "The directory name is invalid", true))
			pr_error(stdout, "^ The compiler executable is not a directory.");
		if (strfind(strerror(err), "The system cannot find the path specified", true))
			pr_error(stdout, "^ The compiler executable does not exist.");
		minimal_debugging();
	}

	return 0;
}
#endif

static
int dog_exec_compiler_process(char *pawncc_path,
							  char *input_path,
							  char *output_path) {

	int         result_configure = 0;
	int         i = 0;
	char       *unix_pointer_token = NULL;
	const char *windows_redist_err = "Have you made sure to install "
									"the Visual CPP (C++) "
									"Redist All-in-One?";
	const char *windows_redist_err2 = "   - install first: "
									"https://www.techpowerup.com/"
									"download/"
									"visual-c-redistributable-"
									"runtime-package-all-in-one"
									"/"
									"\n";

	if (compiler_full_includes == NULL)
		compiler_full_includes = strdup("-ipawno/include -iqawno/include -igamemodes");

	dog_proj_init(pawncc_path, input_path);

	/* Initialize log file path based on platform */
	memset(compiler_temp, 0, sizeof(compiler_temp));
	#ifdef DOG_WINDOWS
		snprintf(compiler_temp, sizeof(compiler_temp),
			"%s", ".watchdogs\\compiler.log");
	#else	
		snprintf(compiler_temp, sizeof(compiler_temp),
			"%s", ".watchdogs/compiler.log");
	#endif
	char *compiler_temp2 = strdup(compiler_temp);

	#ifdef DOG_WINDOWS
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

		/* Build compiler command line string */
		result_configure = snprintf(compiler_input,
			sizeof(compiler_input),
			"%s %s -o%s %s %s %s",
			pawncc_path,
			input_path,
			output_path,
			dogconfig.dog_toml_all_flags,
			compiler_full_includes,
			compiler_path_include_buf);

		if (compiler_input_debug == true) {
		#ifdef DOG_ANDROID
			println(stdout, "** %s", compiler_input);
		#else
			dog_console_title(compiler_input);
			println(stdout, "** %s", compiler_input);
		#endif
		}

		if (result_configure < 0 ||
			result_configure >= sizeof(compiler_input)) {
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

		/* Create Windows process for compiler execution */
		if (compiler_dog_flag_fast == true) {
			/* Use _beginthreadex for fast compilation */
			compiler_thread_data_t thread_data;
			HANDLE thread_handle;
			unsigned thread_id;

			thread_data.compiler_input = compiler_input;
			thread_data.startup_info = &_STARTUPINFO;
			thread_data.process_info = &_PROCESS_INFO;
			thread_data.hFile = hFile;
			thread_data.pre_start = &pre_start;
			thread_data.post_end = &post_end;
			thread_data.windows_redist_err = windows_redist_err;
			thread_data.windows_redist_err2 = windows_redist_err2;

			thread_handle = (HANDLE)_beginthreadex(
				NULL,
				0,
				compiler_thread_func,
				&thread_data,
				0,
				&thread_id);

			if (thread_handle == NULL) {
				pr_error(stdout,
					"_beginthreadex failed!");
				minimal_debugging();
			} else {
				/* Wait for thread to complete */
				WaitForSingleObject(thread_handle, INFINITE);
				CloseHandle(thread_handle);
			}
		} else {
			/* Standard CreateProcess approach */
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

			DWORD err = GetLastError();

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
					0x3E8000);
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
				/* Retrieve process exit code for error reporting */
				GetExitCodeProcess(
					_PROCESS_INFO.hProcess,
					&proc_exit_code);
#if defined(_DBG_PRINT)
				pr_info(stdout,
					"windows process exit with code: %lu",
					proc_exit_code);
				if (
					proc_exit_code ==
					3221225781)
				{
					pr_info(stdout,
						windows_redist_err);
					printf("%s", windows_redist_err2);
					fflush(stdout);
				}
#endif
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
					err);
				if (strfind(strerror(err), "The system cannot find the file specified", true))
					pr_error(stdout, "^ The compiler executable does not exist.");
				if (strfind(strerror(err), "Access is denied", true))
					pr_error(stdout, "^ You do not have permission to execute the compiler executable.");
				if (strfind(strerror(err), "The directory name is invalid", true))
					pr_error(stdout, "^ The compiler executable is not a directory.");
				if (strfind(strerror(err), "The system cannot find the path specified", true))
					pr_error(stdout, "^ The compiler executable does not exist.");
				minimal_debugging();
			}
		}
		if (hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile);
		}
	#else
		result_configure = snprintf(compiler_input,
			sizeof(compiler_input),
			"%s %s -o%s %s %s %s",
			pawncc_path,
			input_path,
			output_path,
			dogconfig.dog_toml_all_flags,
			compiler_full_includes,
			compiler_path_include_buf);

		if (result_configure < 0 ||
			result_configure >= sizeof(compiler_input)) {
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

		if (compiler_input_debug == true) {
		#ifdef DOG_ANDROID
			println(stdout, "@ %s", compiler_input);
		#else
			dog_console_title(compiler_input);
			if (strlen(compiler_input) > 120)
				println(stdout, "@ %s", compiler_input);
		#endif
		}

		/* Tokenize command line into argument array for exec */
		compiler_unix_token = strtok_r(
			compiler_input, " ", &unix_pointer_token);
		while (compiler_unix_token != NULL &&
			i < (sizeof(dog_compiler_unix_args) + 128)) {
			dog_compiler_unix_args[i++] =
				compiler_unix_token;
			compiler_unix_token = strtok_r(NULL,
				" ", &unix_pointer_token);
		}
		dog_compiler_unix_args[i] = NULL;

		#ifdef DOG_ANDROID
		/* Android-specific process creation using fork/vfork */
			static bool child_method = false;
			if (compiler_dog_flag_fast == 1) {
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
					compiler_unix_file_fail = true;
				}
				execv(dog_compiler_unix_args[0], dog_compiler_unix_args);
				fprintf(stderr, "execv failed: %s\n", strerror(errno));
				_exit(127);
			} else if (compiler_process_id > 0) {
				int process_status;
				int process_timeout_occurred = 0;
				clock_gettime(CLOCK_MONOTONIC, &pre_start);
				/* Poll for process completion with timeout (4096 iterations) */
				for (int k = 0; k < 0x1000; k++) {
					int proc_result = waitpid(
						compiler_process_id,
						&process_status,
						WNOHANG);
					if (proc_result == 0) {
						usleep(100000);	/* 100ms sleep between polls */
					} else if (proc_result == compiler_process_id) {
						break;
					} else {
						pr_error(stdout, "waitpid error");
						minimal_debugging();
						break;
					}
					/* Terminate process if timeout exceeded */
					if (k == 0x1000 - 1) {
						kill(compiler_process_id, SIGTERM);
						sleep(2);
						kill(compiler_process_id, SIGKILL);
						pr_error(stdout,
							"process execution timeout! (%d seconds)",
							0x1000);
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
			/* Initialize file actions for output redirection */
			posix_spawn_file_actions_init(
				&process_file_actions);
			int posix_logging_file = open(
				compiler_temp2,
				O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (posix_logging_file != -1) {
				/* Redirect stdout and stderr to log file */
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
				compiler_unix_file_fail = true;
			}

			/* Configure signal handling for spawned process */
			posix_spawnattr_t spawn_attr;
			posix_spawnattr_init(&spawn_attr);

			/* Set signal mask to block SIGCHLD */
			sigset_t sigmask;
			sigemptyset(&sigmask);
			sigaddset(&sigmask, SIGCHLD);

			posix_spawnattr_setsigmask(&spawn_attr,
				&sigmask);

			/* Set default signal actions */
			sigset_t sigdefault;
			sigemptyset(&sigdefault);
			sigaddset(&sigdefault, SIGPIPE);
			sigaddset(&sigdefault, SIGINT);
			sigaddset(&sigdefault, SIGTERM);

			posix_spawnattr_setsigdefault(&spawn_attr,
				&sigdefault);

			/* Apply signal mask and default settings */
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
				for (int k = 0; k < 0x1000; k++) {
					int proc_result = -1;
					proc_result = waitpid(
						compiler_process_id,
						&process_status, WNOHANG);
					if (proc_result == 0)
						usleep(50000);	/* 50ms sleep between polls */
					else if (proc_result ==
						compiler_process_id) {
						break;
					} else {
						pr_error(stdout,
							"waitpid error");
						minimal_debugging();
						break;
					}
					/* Terminate on timeout */
					if (k == 0x1000 - 1) {
						kill(compiler_process_id,
							SIGTERM);
						sleep(2);
						kill(compiler_process_id,
							SIGKILL);
						pr_error(stdout,
							"posix_spawn process execution timeout! (%d seconds)",
							0x1000);
						minimal_debugging();
						waitpid(
							compiler_process_id,
							&process_status, 0);
						process_timeout_occurred =
							1;
					}
				}
				clock_gettime(CLOCK_MONOTONIC, &post_end);
				/* Check exit status if process completed normally */
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
							if (getenv("WSL_DISTRO_NAME") &&
								strcmp(dogconfig.dog_toml_os_type,
									OS_SIGNAL_WINDOWS) == 0 && proc_exit_code == 53)
							{
								pr_info(stdout,
									windows_redist_err);
								printf("%s", windows_redist_err2);
								fflush(stdout);
							}
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
				if (strfind(strerror(process_spawn_result), "Exec format error", true)) 
					pr_error(stdout, "^ The compiler executable is not compatible with your system.");
				if (strfind(strerror(process_spawn_result), "Permission denied", true))
					pr_error(stdout, "^ You do not have permission to execute the compiler executable.");
				if (strfind(strerror(process_spawn_result), "No such file or directory", true))
					pr_error(stdout, "^ The compiler executable does not exist.");
				if (strfind(strerror(process_spawn_result), "Not a directory", true))
					pr_error(stdout, "^ The compiler executable is not a directory.");
				if (strfind(strerror(process_spawn_result), "Is a directory", true))
					pr_error(stdout, "^ The compiler executable is a directory.");
				minimal_debugging();
			}
		#endif
	#endif

	/* Free temporary log path string */
	if (compiler_temp2)
		{
			free(compiler_temp2);
			compiler_temp2 = NULL;
		}
	return (0);
}

/*
 * dog_exec_compiler
 * Main compiler execution function. This function orchestrates the
 * entire compilation process including finding the compiler executable,
 * parsing command-line arguments, setting up compiler flags, handling
 * input file paths, executing the compilation, and processing results.
 * Supports both default project compilation and explicit file compilation.
 *
 * Parameters:
 *   args: Unused parameter (legacy)
 *   compile_args_val: Input file path or "." for default project
 *   second_arg through ten_arg: Additional command-line arguments
 *
 * Returns:
 *   1 on completion (success or failure), -2 on memory allocation failure
 */
int
dog_exec_compiler(const char *args, const char *compile_args_val,
    const char *second_arg, const char *four_arg, const char *five_arg,
    const char *six_arg, const char *seven_arg, const char *eight_arg,
    const char *nine_arg, const char *ten_arg)
{
	io_compilers   all_compiler_field;
	io_compilers  *ctx = &all_compiler_field;
	int            ret_pawncc = 0;
	char          *_pawncc_ptr = NULL;
	size_t	       rate_sef_entries;
	char          *gamemodes_slash = "gamemodes/";
	char          *gamemodes_back_slash = "gamemodes\\";

	/* Reset all compiler state */
	compiler_refresh_data();

	/* Build argument array from variadic parameters */
	const char *argv_buf[] = {
		second_arg,four_arg,five_arg,six_arg,seven_arg,eight_arg,nine_arg,ten_arg
	};

	/* Determine compiler executable name based on OS */
	if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_WINDOWS) == 0) {
		/* windows : pawncc.exe - wsl/wsl2 - native / msys2 */
		_pawncc_ptr = "pawncc.exe";
	} else if (strcmp(dogconfig.dog_toml_os_type, OS_SIGNAL_LINUX) == 0) {
		/* linux : pawncc */
		_pawncc_ptr = "pawncc";
	}

	/* Search for compiler executable in standard locations */
	if (dir_exists("pawno") != 0 && dir_exists("qawno") != 0) {
		ret_pawncc = dog_find_path("pawno", _pawncc_ptr,
			NULL);
		if (ret_pawncc) {
			;
		} else {
			ret_pawncc = dog_find_path("qawno",
				_pawncc_ptr, NULL);
			if (ret_pawncc < 1) {
				ret_pawncc = dog_find_path(".",
					_pawncc_ptr, NULL);
			}
		}
	} else if (dir_exists("pawno") != 0) {
		ret_pawncc = dog_find_path("pawno", _pawncc_ptr,
			NULL);
		if (ret_pawncc) {
			;
		} else {
			ret_pawncc = dog_find_path(".",
				_pawncc_ptr, NULL);
		}
	} else if (dir_exists("qawno") != 0) {
		ret_pawncc = dog_find_path("qawno", _pawncc_ptr,
			NULL);
		if (ret_pawncc) {
			;
		} else {
			ret_pawncc = dog_find_path(".",
				_pawncc_ptr, NULL);
		}
	} else {
		ret_pawncc = dog_find_path(".", _pawncc_ptr,
			NULL);
	}

	/* Process command-line flags if compiler was found */
	if (ret_pawncc != 0) {
		/* Parse command-line arguments and set corresponding flags */
		for (int i = 0; i < 8 && argv_buf[i] != NULL; ++i) {
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

		/* Check if no flags were specified */
		if (!compiler_dog_flag_detailed &&
			!compiler_dog_flag_debug &&
			!compiler_dog_flag_clean &&
			!compiler_dog_flag_asm &&
			!compiler_dog_flag_compat &&
			!compiler_dog_flag_prolix &&
			!compiler_dog_flag_compact)
			{
				compiler_empty_dog_flag = true;
			}

		/* Handle clean flag: reset all compiler flags */
			if (compiler_dog_flag_clean)
		{
			memset(compiler_buf, 0, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				" ");
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);

			goto next_;
		}

	/* Retry logic for failed compilations with adjusted parameters */
	_compiler_retry_stat:
		if (compiler_retry_stat == 1) {
			compiler_dog_flag_compat = true, compiler_dog_flag_compact = true;
			compiler_dog_flag_fast = true, compiler_dog_flag_detailed = true;
			memset(compiler_buf, 0, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"%s MAX_PLAYERS=50 MAX_VEHICLES=50 MAX_ACTORS=50 MAX_OBJECTS=1000",
				dogconfig.dog_toml_all_flags);
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);
		} else if (compiler_retry_stat == 2) {
			memset(compiler_buf, 0, sizeof(compiler_buf));
			snprintf(compiler_buf, sizeof(compiler_buf),
				"MAX_PLAYERS=100 MAX_VEHICLES=1000 MAX_ACTORS=100 MAX_OBJECTS=2000");
			if (dogconfig.dog_toml_all_flags)
				{
					dog_free(dogconfig.dog_toml_all_flags);
					dogconfig.dog_toml_all_flags = NULL;
				}
			dogconfig.dog_toml_all_flags = strdup(compiler_buf);

			goto next_;
		}
		if (compiler_long_time) {
			compiler_dog_flag_compat = true, compiler_dog_flag_compact = true;
			compiler_dog_flag_fast = true, compiler_dog_flag_detailed = true;
			memset(compiler_buf, 0, sizeof(compiler_buf));
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

		/* Build bitmask of enabled compiler flags */
		unsigned int __set_bit = 0;

		if (compiler_dog_flag_debug)
			/* bit 0 = 1 (0x01) */
			__set_bit |= BIT_FLAG_DEBUG;

		if (compiler_dog_flag_asm)
			/* bit 1 = 1 (0x03) */
			__set_bit |= BIT_FLAG_ASSEMBLER;

		if (compiler_dog_flag_compat)
			/* bit 2 = 1 (0x07) */
			__set_bit |= BIT_FLAG_COMPAT;

		if (compiler_dog_flag_prolix)
			/* bit 3 = 1 (0x0F) */
			__set_bit |= BIT_FLAG_PROLIX;

		if (compiler_dog_flag_compact)
			/* bit 4 = 1 (0x1F) */
			__set_bit |= BIT_FLAG_COMPACT;

		if (compiler_dog_flag_fast)
			/* bit 5 = 1 (0x20) */
			__set_bit |= BIT_FLAG_TIME;

		/* Build flag string from enabled options */
		char *p = compiler_dog_flag_list;
		p += strlen(p);

		for (int i = 0; object_opt[i].option; i++) {
			if (!(__set_bit & object_opt[i].flag))
				continue;

			memcpy(p, object_opt[i].option,
				object_opt[i].len);
			p += object_opt[i].len;
		}

		*p = '\0';
		
	/* Merge flag list with existing compiler flags */
	next_:
		if (compiler_retry_stat == 2)
			{
				compiler_dog_flag_list[0] = '\0';
			}
		if (compiler_dog_flag_detailed)
			{
				compiler_input_debug = true;
			}
#if defined(_DBG_PRINT)
		{
			compiler_input_debug = true;
		}
#endif
		/* Append flag list to existing compiler flags */
		if (strlen(compiler_dog_flag_list) > 0) {
			size_t calcute_len_all_flags = 0;

			if (dogconfig.dog_toml_all_flags) {
				calcute_len_all_flags =
					strlen(dogconfig.dog_toml_all_flags);
			} else {
				dogconfig.dog_toml_all_flags = strdup("");
			}

			size_t extra_len =
				strlen(compiler_dog_flag_list);
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
				compiler_dog_flag_list);
		}

		/* Handle parent directory references in compile arguments */
		if (strfind(compile_args_val, "../", true) !=
			false) {
			size_t write_pos = 0, j;
			bool rate_parent_dir = false;
			for (j = 0; compile_args_val[j] != '\0';) {
				if (!rate_parent_dir &&
					strncmp(&compile_args_val[j],
					"../", 3) == 0) {
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
				memcpy(compiler_parsing, "../", 3);
				write_pos += 3;
				compiler_parsing[write_pos] = '\0';
				if (compiler_parsing[write_pos - 1] !=
					_PATH_CHR_SEP_POSIX &&
					compiler_parsing[write_pos - 1] !=
					_PATH_CHR_SEP_WIN32)
					strcat(compiler_parsing, "/");
			} else {
				strcpy(compiler_parsing, "../");
			}

			memset(compiler_temp, 0, sizeof(compiler_temp));

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

			memset(compiler_buf, 0, sizeof(compiler_buf));

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
		} else {
			snprintf(compiler_path_include_buf, sizeof(compiler_path_include_buf),
				"-i=none");
		}

		/* Show tip message if no flags were specified */
		static bool rate_flag_notice = false;
		if (!rate_flag_notice && compiler_empty_dog_flag) {
			print("\n");
			compiler_show_tip();
			print("\n");
			fflush(stdout);
			rate_flag_notice = true;
		}

		#ifdef DOG_ANDROID
			/* Disable warning 200 (truncated char) by default on Android */
			memset(compiler_buf, 0, sizeof(compiler_buf));
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
			/* Prompt user for compilation target if not specified */
			static bool compiler_target = false;
			if (compile_args_val[0] != '.' && compiler_target == false)
			{
				pr_color(stdout, DOG_COL_YELLOW,
					DOG_COL_BYELLOW
					"** COMPILER TARGET\n");
				int tree_ret = -1;
				{
					char *tree[] = { "tree", ">", "/dev/null 2>&1", NULL };
					tree_ret = dog_exec_command(tree);
				}
				if (!tree_ret) {
					if (path_exists("../storage/downloads") == 1) {
						char *tree[] = {
							"tree", "-P", "\"*.p\"", "-P", "\"*.pwn\"", "../storage/downloads", NULL };
						dog_exec_command(tree);
					} else {
						char *tree[] = { "tree", "-P", "\"*.p\"", "-P", "\"*.pwn\"", ".", NULL };
						dog_exec_command(tree);
					}
				} else {
					#ifdef DOG_LINUX
					if (path_exists("../storage/downloads") == 1) {
						char *argv[] = { "ls", "../storage/downloads", "-R", NULL };
						dog_exec_command(argv);
					} else {
						char *argv[] = { "ls", ".", "-R", NULL };
						dog_exec_command(argv);
					}
					#else
					char *argv[] = { "dir", ".", "-s", NULL };
					dog_exec_command(argv);
					#endif
				}
				printf(DOG_COL_BCYAN);
				printf(
					" * You run the compiler command "
					"without any args | compile %s | compile mode.pwn\n"
					" * Do you want to compile for "
					DOG_COL_GREEN "%s " DOG_COL_BCYAN
					"(enter), \n"
					" * or do you want to compile for something else?\n",
					compile_args_val, dogconfig.dog_toml_proj_input);
				printf(
					" * input likely:\n"
					"   bare.pwn | grandlarc.pwn | main.pwn | server.p\n"
					"   ../storage/downloads/dog/gamemodes/main.pwn\n"
					"   ../storage/downloads/osint/gamemodes/gm.pwn\n"
				);
				fflush(stdout);
				print_restore_color();
				printf(DOG_COL_CYAN ">"
					DOG_COL_DEFAULT);
				fflush(stdout);
				compiler_project = readline(" ");
				if (compiler_project &&
					strlen(compiler_project) > 0) {
					dog_free(
						dogconfig.dog_toml_proj_input);
					dogconfig.dog_toml_proj_input =
						strdup(compiler_project);
					if (!dogconfig.dog_toml_proj_input) {
						pr_error(stdout,
							"Memory allocation failed");
						dog_free(compiler_project);
						goto compiler_end;
					}
				}
				dog_free(compiler_project);
				compiler_project = NULL;
				compiler_target = true;
			}

			/* Execute compilation process */
			int _process = dog_exec_compiler_process(
					dogconfig.dog_sef_found_list[0],
					dogconfig.dog_toml_proj_input,
					dogconfig.dog_toml_proj_output);
			if (_process != 0) {
				goto compiler_end;
			}

			/* Process compiler log output */
			if (path_exists(".watchdogs/compiler.log")) {
				print("\n");
				char *ca = NULL;
				ca = dogconfig.dog_toml_proj_output;
				bool cb = 0;
				if (compiler_have_debug_flag)
					cb = 1;
				if (compiler_dog_flag_detailed) {
					cause_compiler_expl(
						".watchdogs/compiler.log",
						ca, cb);
					goto compiler_done;
				}

				if (compiler_unix_file_fail == false)
					dog_printfile(
						".watchdogs/compiler.log");
			}
		compiler_done:
			/* Check log file for compilation errors */
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
					if (dogconfig.dog_toml_proj_output != NULL &&
						path_access(dogconfig.dog_toml_proj_output))
						remove(dogconfig.dog_toml_proj_output);
					compiler_is_err = true;
				} else {
					compiler_is_err = false;
				}
			} else {
				pr_error(stdout,
					"Failed to open .watchdogs/compiler.log");
				minimal_debugging();
			}

			/* Calculate and display compilation time */
			compiler_calculate_time = ((double)(post_end.tv_sec - pre_start.tv_sec)) +
			                     ((double)(post_end.tv_nsec - pre_start.tv_nsec)) / 1e9;

			print("\n");

			if (!compiler_is_err)
				print("** Completed Tasks.\n");

			pr_color(stdout, DOG_COL_CYAN,
				" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
				compiler_calculate_time,
				compiler_calculate_time * 1000.0);
			if (compiler_calculate_time > 300) {
				goto _print_time;
			}
		} else {
		/* Handle explicit file compilation (not default project) */

            /* Validate file extension */
            if (strfind(compile_args_val, ".pwn", true) == false &&
                strfind(compile_args_val, ".p", true) == false)
            {
                pr_warning(stdout, "The compiler only accepts '.p' or '.pwn' files.");
                goto compiler_end;
            }

			/* Parse input file path to extract directory and filename */
			strncpy(ctx->compiler_size_temp,
				compile_args_val,
				sizeof(ctx->compiler_size_temp) -
				1);
			ctx->compiler_size_temp[
				sizeof(ctx->compiler_size_temp) -
				1] = '\0';

			/* Find path separators (handle both Unix and Windows) */
			compiler_size_last_slash = strrchr(
				ctx->compiler_size_temp,
				_PATH_CHR_SEP_POSIX);
			compiler_back_slash = strrchr(
				ctx->compiler_size_temp,
				_PATH_CHR_SEP_WIN32);

			if (compiler_back_slash && (!compiler_size_last_slash ||
				compiler_back_slash > compiler_size_last_slash))
				compiler_size_last_slash =
					compiler_back_slash;

			/* Extract directory and filename components */
			if (compiler_size_last_slash) {
				size_t compiler_dir_len;
				compiler_dir_len = (size_t)
					(compiler_size_last_slash -
					ctx->compiler_size_temp);

				if (compiler_dir_len >=
					sizeof(ctx->compiler_direct_path))
					compiler_dir_len =
						sizeof(ctx->compiler_direct_path) -
						1;

				memcpy(ctx->compiler_direct_path,
					ctx->compiler_size_temp,
					compiler_dir_len);
				ctx->compiler_direct_path[
					compiler_dir_len] = '\0';

				const char *compiler_filename_start =
					compiler_size_last_slash + 1;
				size_t compiler_filename_len;
				compiler_filename_len = strlen(
					compiler_filename_start);

				if (compiler_filename_len >=
					sizeof(ctx->compiler_size_file_name))
					compiler_filename_len =
						sizeof(ctx->compiler_size_file_name) -
						1;

				memcpy(
					ctx->compiler_size_file_name,
					compiler_filename_start,
					compiler_filename_len);
				ctx->compiler_size_file_name[
					compiler_filename_len] = '\0';

				size_t total_needed;
				total_needed =
					strlen(ctx->compiler_direct_path) +
					1 +
					strlen(ctx->compiler_size_file_name) +
					1;

				if (total_needed >
					sizeof(ctx->compiler_size_input_path)) {
					strncpy(ctx->compiler_direct_path,
						"gamemodes",
						sizeof(ctx->compiler_direct_path) -
						1);
					ctx->compiler_direct_path[
						sizeof(ctx->compiler_direct_path) -
						1] = '\0';

					size_t compiler_max_size_file_name;
					compiler_max_size_file_name =
						sizeof(ctx->compiler_size_file_name) -
						1;

					if (compiler_filename_len >
						compiler_max_size_file_name) {
						memcpy(
							ctx->compiler_size_file_name,
							compiler_filename_start,
							compiler_max_size_file_name);
						ctx->compiler_size_file_name[
							compiler_max_size_file_name] =
							'\0';
					}
				}

				if (snprintf(
					ctx->compiler_size_input_path,
					sizeof(ctx->compiler_size_input_path),
					"%s/%s",
					ctx->compiler_direct_path,
					ctx->compiler_size_file_name) >=
					(int)sizeof(
					ctx->compiler_size_input_path)) {
					ctx->compiler_size_input_path[
						sizeof(ctx->compiler_size_input_path) -
						1] = '\0';
				}
			} else {
				strncpy(
					ctx->compiler_size_file_name,
					ctx->compiler_size_temp,
					sizeof(ctx->compiler_size_file_name) -
					1);
				ctx->compiler_size_file_name[
					sizeof(ctx->compiler_size_file_name) -
					1] = '\0';

				strncpy(
					ctx->compiler_direct_path,
					".",
					sizeof(ctx->compiler_direct_path) -
					1);
				ctx->compiler_direct_path[
					sizeof(ctx->compiler_direct_path) -
					1] = '\0';

				if (snprintf(
					ctx->compiler_size_input_path,
					sizeof(ctx->compiler_size_input_path),
					"./%s",
					ctx->compiler_size_file_name) >=
					(int)sizeof(
					ctx->compiler_size_input_path)) {
					ctx->compiler_size_input_path[
						sizeof(ctx->compiler_size_input_path) -
						1] = '\0';
				}
			}

			/* Search for input file in specified directory */
			int compiler_finding_compile_args = 0;
			compiler_finding_compile_args = dog_find_path(
				ctx->compiler_direct_path,
				ctx->compiler_size_file_name,
				NULL);

			/* Fallback to gamemodes directory if not found */
			if (!compiler_finding_compile_args &&
				strcmp(ctx->compiler_direct_path,
				"gamemodes") != 0) {
				compiler_finding_compile_args =
					dog_find_path("gamemodes",
					ctx->compiler_size_file_name,
					NULL);
				if (compiler_finding_compile_args) {
					strncpy(
						ctx->compiler_direct_path,
						"gamemodes",
						sizeof(ctx->compiler_direct_path) -
						1);
					ctx->compiler_direct_path[
						sizeof(ctx->compiler_direct_path) -
						1] = '\0';

					if (snprintf(
						ctx->compiler_size_input_path,
						sizeof(ctx->compiler_size_input_path),
						"gamemodes/%s",
						ctx->compiler_size_file_name) >=
						(int)sizeof(
						ctx->compiler_size_input_path)) {
						ctx->compiler_size_input_path[
							sizeof(ctx->compiler_size_input_path) -
							1] = '\0';
					}

					if (dogconfig.dog_sef_count >
						RATE_SEF_EMPTY)
						strncpy(
							dogconfig.dog_sef_found_list[
							dogconfig.dog_sef_count -
							1],
							ctx->compiler_size_input_path,
							MAX_SEF_PATH_SIZE);
				}
			}

			if (!compiler_finding_compile_args &&
				!strcmp(ctx->compiler_direct_path,
				".")) {
				compiler_finding_compile_args =
					dog_find_path("gamemodes",
					ctx->compiler_size_file_name,
					NULL);
				if (compiler_finding_compile_args) {
					strncpy(
						ctx->compiler_direct_path,
						"gamemodes",
						sizeof(ctx->compiler_direct_path) -
						1);
					ctx->compiler_direct_path[
						sizeof(ctx->compiler_direct_path) -
						1] = '\0';

					if (snprintf(
						ctx->compiler_size_input_path,
						sizeof(ctx->compiler_size_input_path),
						"gamemodes/%s",
						ctx->compiler_size_file_name) >=
						(int)sizeof(
						ctx->compiler_size_input_path)) {
						ctx->compiler_size_input_path[
							sizeof(ctx->compiler_size_input_path) -
							1] = '\0';
					}

					if (dogconfig.dog_sef_count >
						RATE_SEF_EMPTY)
						strncpy(
							dogconfig.dog_sef_found_list[
							dogconfig.dog_sef_count -
							1],
							ctx->compiler_size_input_path,
							MAX_SEF_PATH_SIZE);
				}
			}

			/* Match input file against found file list */
			rate_sef_entries = sizeof(dogconfig.dog_sef_found_list) /
							   sizeof(dogconfig.dog_sef_found_list[0]);

			for (int i = 0; i < rate_sef_entries; i++) {
				if (strfind(dogconfig.dog_sef_found_list[i], compile_args_val, true)) {
					memset(compiler_temp, 0, sizeof(compiler_temp));
					snprintf(compiler_temp,
						sizeof(compiler_temp), "%s",
						dogconfig.dog_sef_found_list[i]);
					memset(compiler_buf, 0, sizeof(compiler_buf));
					snprintf(compiler_buf, sizeof(compiler_buf),
						"%s", compiler_temp);
						if (compiler_proj_path)
							{
								free(compiler_proj_path);
								compiler_proj_path = NULL;
							}
					compiler_proj_path = strdup(compiler_buf);
				}
			}

#if defined(_DBG_PRINT)
			if (compiler_proj_path != NULL)
				pr_info(stdout, "compiler_proj_path: %s", compiler_proj_path);
#endif
			/* Generate output filename and execute compilation */
			if (path_exists(compiler_proj_path) == 1) {
				if (compiler_proj_path) {
					memset(compiler_temp, 0, sizeof(compiler_temp));
					strncpy(compiler_temp, compiler_proj_path,
						sizeof(compiler_temp) - 1);
					compiler_temp[sizeof(compiler_temp) - 1] = '\0';
				} else {
					memset(compiler_temp, 0, sizeof(compiler_temp));
				}

				/* Remove file extension to generate .amx output name */
				char *extension = strrchr(compiler_temp,
					'.');
				if (extension)
					*extension = '\0';
				
				ctx->container_output = strdup(compiler_temp);

				snprintf(compiler_temp, sizeof(compiler_temp),
					"%s.amx", ctx->container_output);
					
				char *compiler_temp2 = strdup(compiler_temp);
				
				/* Execute compilation process */
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
					print("\n");
					char *ca = NULL;
					ca = compiler_temp2;
					bool cb = 0;
					if (compiler_have_debug_flag)
						cb = 1;
					if (compiler_dog_flag_detailed) {
						cause_compiler_expl(
							".watchdogs/compiler.log",
							ca, cb);
						goto compiler_done2;
					}

					if (compiler_unix_file_fail == false)
						dog_printfile(
							".watchdogs/compiler.log");
				}

		compiler_done2:
				this_proc_file = fopen(
					".watchdogs/compiler.log", "r");
				memset(compiler_mb, 0, sizeof(compiler_mb));
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
						if (compiler_temp2 &&
							path_access(compiler_temp2))
							remove(compiler_temp2);
						compiler_is_err = true;
					} else {
						compiler_is_err = false;
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

				compiler_calculate_time = ((double)(post_end.tv_sec - pre_start.tv_sec)) +
				                     ((double)(post_end.tv_nsec - pre_start.tv_nsec)) / 1e9;

				print("\n");

				if (!compiler_is_err)
					print("** Completed Tasks.\n");

				pr_color(stdout, DOG_COL_CYAN,
					" <C> (compile-time) Finished at %.3fs (%.0f ms)\n",
					compiler_calculate_time,
					compiler_calculate_time * 1000.0);
				if (compiler_calculate_time > 300) {
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

		/* Check compiler log for errors and retry logic */
		if (this_proc_file)
			fclose(this_proc_file);

		memset(compiler_mb, 0, sizeof(compiler_mb));

		this_proc_file = fopen(".watchdogs/compiler.log", "rb");

		if (!this_proc_file)
			goto compiler_end;
		if (compiler_long_time)
			goto compiler_end;

		/* Scan log file for errors and standard library issues */
		bool
		  samp_stdlib_fail=false;

		while (fgets(
				compiler_mb,
				sizeof(compiler_mb),
			this_proc_file) != NULL)
			{
			/* Check for compilation errors and trigger retry if needed */
			if (strfind(compiler_mb, "error",
				true) != false) {
					if (
						compiler_retry_stat == 0)
					{
						compiler_retry_stat = 1;
						printf(DOG_COL_BCYAN
							"** Compilation Process Exit with Failed. "
							"recompiling: "
							"%d/2\n"
							BKG_DEFAULT, compiler_retry_stat);
        				fflush(stdout);
						goto _compiler_retry_stat;
					}
					if (
						compiler_retry_stat == 1)
					{
						compiler_retry_stat = 2;
						printf(DOG_COL_BCYAN
							"** Compilation Process Exit with Failed. "
							"recompiling: "
							"%d/2\n"
							BKG_DEFAULT, compiler_retry_stat);
        				fflush(stdout);
						goto _compiler_retry_stat;
					}
				}
		    if((strfind(compiler_mb, "a_samp" ,true)
		    	== 1 &&
		    	strfind(compiler_mb, "cannot read from file", true)
		    	== 1) ||
		        (strfind(compiler_mb, "open.mp", true)
		        == 1 &&
		        strfind(compiler_mb, "cannot read from file", true)
		        == 1))
		    {
	        	samp_stdlib_fail = true;
		    }
			}

		if (this_proc_file)
			fclose(this_proc_file);

		if (samp_stdlib_fail == true) {
			compiler_installing_stdlib = true;
		}

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
        fflush(stdout);

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

/* Cleanup and return */
compiler_end:
    fflush(stdout);
	return (1);
/* Handle long-running compilation with retry */
_print_time:
    fflush(stdout);
	print(
		"** Process is taking a while..\n");
	if (compiler_long_time == false) {
		pr_info(stdout, "Retrying..");
		compiler_long_time = true;
		goto _compiler_retry_stat;
	}
	return (1);
}
