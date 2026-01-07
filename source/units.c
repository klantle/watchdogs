/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <stdbool.h>
#include  <limits.h>
#include  <time.h>
#include  <sys/stat.h>
#include  <sys/types.h>
#include  <signal.h>
#include  <curl/curl.h>

#include  "extra.h"
#include  "utils.h"
#include  "crypto.h"
#include  "library.h"
#include  "archive.h"
#include  "curl.h"
#include  "endpoint.h"
#include  "compiler.h"
#include  "replicate.h"
#include  "debug.h"
#include  "units.h"

#if defined(__W_VERSION__)
#define WATCHDOGS_RELEASE __W_VERSION__
#else
#define WATCHDOGS_RELEASE "WATCHDOGS"
#endif

const char *watchdogs_release = WATCHDOGS_RELEASE;

static struct timespec cmd_start;
static struct timespec cmd_end = { 0 };
static double command_dur;

int
__command__(char *unit_pre_command)
{
	unit_debugging(1);
	if (dogconfig.dog_garbage_access[DOG_GARBAGE_CURL_COMPILER_TESTING] == DOG_GARBAGE_TRUE)
	{
		printf(DOG_COL_BCYAN
			"Please input the pawn file with dot type (.pwn/.p):\n"
			DOG_COL_DEFAULT);
		char *compile_target = readline(" ");
		if (compile_target) {
			const char *argsc[] = { NULL, compile_target, "-w",
				NULL, NULL, NULL, NULL, NULL, NULL, NULL };
			dog_exec_compiler(argsc[0], argsc[1], argsc[2],
				argsc[3],
				argsc[4], argsc[5], argsc[6], argsc[7],
				argsc[8], argsc[9]);
		}
		dog_free(compile_target);
		dogconfig.dog_garbage_access[DOG_GARBAGE_CURL_COMPILER_TESTING] = DOG_GARBAGE_ZERO;
	}

	char *ptr_prompt = NULL;
	size_t size_ptr_command = DOG_MAX_PATH + DOG_PATH_MAX;
	char *ptr_command = NULL;
	const char *command_similar;
	int dist = INT_MAX;

	ptr_prompt = dog_malloc(size_ptr_command);
	if (!ptr_prompt)
		return -1;

_ptr_command:
	if (ptr_command) {
		free(ptr_command);
		ptr_command = NULL;
	}
	if (unit_pre_command && unit_pre_command[0] != '\0') {
		ptr_command = strdup(unit_pre_command);
		if (strfind(ptr_command, "812C397D", true) == 0)
			{
				printf("# %s\n", ptr_command);
			}
	} else {
		if (dogconfig.dog_garbage_access[DOG_GARBAGE_INTRO] == DOG_GARBAGE_ZERO)
		{
			dogconfig.dog_garbage_access[DOG_GARBAGE_INTRO] = DOG_GARBAGE_TRUE;
			unit_show_dog();
			goto _ptr_command;
		}
		while (true) {
			snprintf(ptr_prompt, size_ptr_command, "# ");
			char *input = readline(ptr_prompt);
			if (input == NULL) {
				free(input);
				return 2;
			}
			if (input[0] == '\0') {
				free(input);
				continue;
			}
			ptr_command = strdup(input);
			free(input);
			break;
		}
	}

	fflush(stdout);
	if (ptr_command && ptr_command[0] != '\0' &&
		strfind(ptr_command, "812C397D", true) == false)
	{
		HIST_ENTRY *last = history_get(history_length);
		if (last == NULL ||
		    strcmp(last->line, ptr_command) != 0) {
			dog_history_add(ptr_command);
		}
	}

	command_similar = dog_find_near_command(ptr_command,
	    __command, __command_len, &dist);

_reexecute_command:
	unit_debugging(0);
	clock_gettime(CLOCK_MONOTONIC, &cmd_start);
	if (strncmp(ptr_command, "help", strlen("help")) == 0) {
		dog_console_title("Watchdogs | @ help");

		char *args;
		args = ptr_command + strlen("help");
		while (*args == ' ')
			++args;

		unit_show_help(args);

		goto unit_done;
	} else if (strcmp(ptr_command, "exit") == 0) {
		dog_free(ptr_command);
		ptr_command = NULL;
		dog_free(ptr_prompt);
		ptr_prompt = NULL;
		return 2;
	} else if (strncmp(ptr_command, "sha1", strlen("sha1")) == 0) {
		char *args = ptr_command + strlen("sha1");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: sha1 [<words>]");
		} else {
			unsigned char digest[20];

			if (crypto_generate_sha1_hash(args, digest) != 1) {
				goto unit_done;
			}

			printf("        Crypto Input : %s\n", args);
			printf("Crypto Output (sha1) : ");
			crypto_print_hex(digest, sizeof(digest), 1);
		}

		goto unit_done;
	} else if (strncmp(ptr_command, "sha256", strlen("sha256")) == 0) {
		char *args = ptr_command + strlen("sha256");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: sha256 [<words>]");
		} else {
			unsigned char digest[32];

			if (crypto_generate_sha256_hash(args, digest) != 1) {
				goto unit_done;
			}

			printf("          Crypto Input : %s\n", args);
			printf("Crypto Output (SHA256) : ");
			crypto_print_hex(digest, sizeof(digest), 1);
		}

		goto unit_done;
	} else if (strncmp(ptr_command, "crc32", strlen("crc32")) == 0) {
		char *args = ptr_command + strlen("crc32");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: crc32 [<words>]");
		} else {
			static int init_crc32 = 0;
			if (init_crc32 != 1) {
				crypto_crc32_init_table();
				init_crc32 = 1;
			}

			uint32_t crc32_generate;
			crc32_generate = crypto_generate_crc32(args,
			    strlen(args));

			char crc_str[11];
			sprintf(crc_str, "%08X", crc32_generate);

			printf("         Crypto Input : %s\n", args);
			printf("Crypto Output (CRC32) : ");
			printf("%s\n", crc_str);
		}

		goto unit_done;
	} else if (strncmp(ptr_command, "djb2", strlen("djb2")) == 0) {
		char *args = ptr_command + strlen("djb2");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: djb2 [<file>]");
		} else {
			if (path_exists(args) == 0) {
				pr_error(stdout,
				    "djb2: " DOG_COL_CYAN "%s"
				    " - No such file or directory", args);
				goto unit_done;
			}
			unsigned long djb2_generate;
			djb2_generate = crypto_djb2_hash_file(args);

			if (djb2_generate) {
				printf("        Crypto Input : %s\n", args);
				printf("Crypto Output (DJB2) : ");
				printf("%#lx\n", djb2_generate);
			}
		}

		goto unit_done;
	} else if (strcmp(ptr_command, "config") == 0) {
		if (access("watchdogs.toml", F_OK) == 0)
			remove("watchdogs.toml");

		unit_debugging(1);

		printf(DOG_COL_B_BLUE "");
		dog_printfile("watchdogs.toml");
		printf(DOG_COL_DEFAULT "\n");

		goto unit_done;
	} else if (strncmp(ptr_command, "replicate", strlen("replicate")) ==
	    0) {
		dog_console_title("Watchdogs | @ replicate depends");
		char *args = ptr_command + strlen("replicate");
		while (*args == ' ')
			++args;

		int is_null_args = -1;
		if (args[0] == '\0' || strlen(args) < 1)
			is_null_args = 1;

		char *__args = strdup(args);
		char *raw_branch = NULL;
		char *raw_save = NULL;

		char *args2 = strtok(__args, " ");
		if (args2 == NULL || strcmp(args2, ".") == 0)
			is_null_args = 1;

		char *procure_args = strtok(args, " ");
		while (procure_args) {
			if (strcmp(procure_args, "--branch") == 0) {
				procure_args = strtok(NULL, " ");
				if (procure_args)
					raw_branch = procure_args;
			} else if (strcmp(procure_args, "--save") == 0) {
				procure_args = strtok(NULL, " ");
				if (procure_args)
					raw_save = procure_args;
			}
			procure_args = strtok(NULL, " ");
		}

		if (raw_save && strcmp(raw_save, ".") == 0) {
			static char *fetch_pwd = NULL;
			fetch_pwd = dog_procure_pwd();
			raw_save = strdup(fetch_pwd);
		}

		dog_free(__args);

		if (is_null_args != 1) {
			if (raw_branch && raw_save)
				dog_install_depends(args, raw_branch, raw_save);
			else if (raw_branch)
				dog_install_depends(args, raw_branch, NULL);
			else if (raw_save)
				dog_install_depends(args, "main", raw_save);
			else
				dog_install_depends(args, "main", NULL);
		} else {
			char errbuf[DOG_PATH_MAX];
			toml_table_t *dog_toml_config;
			FILE *this_proc_file = fopen("watchdogs.toml", "r");
			dog_toml_config = toml_parse_file(this_proc_file,
			    errbuf, sizeof(errbuf));
			if (this_proc_file)
				fclose(this_proc_file);

			if (!dog_toml_config) {
				pr_error(stdout,
				    "failed to parse the watchdogs.toml...: %s",
				    errbuf);
				minimal_debugging();
				return 0;
			}

			toml_table_t *dog_depends;
			size_t arr_sz, i;
			char *expect = NULL;

			dog_depends = toml_table_in(dog_toml_config,
			    "dependencies");
			if (!dog_depends)
				goto out;

			toml_array_t *dog_toml_packages = toml_array_in(
			    dog_depends, "packages");
			if (!dog_toml_packages)
				goto out;

			arr_sz = toml_array_nelem(dog_toml_packages);
			for (i = 0; i < arr_sz; i++) {
				toml_datum_t val;

				val = toml_string_at(dog_toml_packages, i);
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
					char *tmp;
					size_t old_len = strlen(expect);
					size_t new_len = old_len +
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
				dog_free(val.u.s);
				val.u.s = NULL;
			}

			if (!expect)
				expect = strdup("");

			dog_free(dogconfig.dog_toml_packages);
			dogconfig.dog_toml_packages = expect;

			pr_info(stdout,
			    "Trying to installing: %s",
			    dogconfig.dog_toml_packages);

			if (raw_branch && raw_save)
				dog_install_depends(dogconfig.dog_toml_packages,
				    raw_branch, raw_save);

			else if (raw_branch)
				dog_install_depends(dogconfig.dog_toml_packages,
				    raw_branch, NULL);

			else if (raw_save)
				dog_install_depends(dogconfig.dog_toml_packages,
				    "main", raw_save);

			else
				dog_install_depends(dogconfig.dog_toml_packages,
				    "main", NULL);
out:
			toml_free(dog_toml_config);
		}

		goto unit_done;
	} else if (strcmp(ptr_command, "gamemode") == 0) {
		dog_console_title("Watchdogs | @ gamemode");
ret_ptr:
		printf("\033[1;33m== Select a Platform ==\033[0m\n");
		printf("  \033[36m[l]\033[0m Linux\n");
		printf("  \033[36m[w]\033[0m Windows\n"
		    "  ^ \033[90m(Supported for: WSL/WSL2 ; "
		    "not: Docker or Podman on WSL)\033[0m\n");
		printf("  \033[36m[t]\033[0m Termux\n");

		dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT] = DOG_GARBAGE_TRUE;
		int stat_true = dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT];

		char *platform = readline("==> ");

		if (strfind(platform, "L", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_server("linux");
loop_igm:
			if (ret == -1 && stat_true)
				goto loop_igm;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "W", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_server("windows");
loop_igm2:
			if (ret == -1 && stat_true)
				goto loop_igm2;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "T", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_server("termux");
loop_igm3:
			if (ret == -1 && stat_true)
				goto loop_igm3;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "E", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			if (unit_pre_command && unit_pre_command[0] != '\0')
				goto unit_done;
			else
				goto _ptr_command;
		} else {
			pr_error(stdout,
			    "Invalid platform selection. Input 'E/e' to exit");
			free(platform);
			platform = NULL;
			goto ret_ptr;
		}

		goto unit_done;
	} else if (strcmp(ptr_command, "pawncc") == 0) {
		dog_console_title("Watchdogs | @ pawncc");
ret_ptr2:
		printf("\033[1;33m== Select a Platform ==\033[0m\n");
		printf("  \033[36m[l]\033[0m Linux\n");
		printf("  \033[36m[w]\033[0m Windows\n"
		    "  ^ \033[90m(Supported for: WSL/WSL2 ; "
		    "not: Docker or Podman on WSL)\033[0m\n");
		printf("  \033[36m[t]\033[0m Termux\n");

		dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT] = DOG_GARBAGE_TRUE;
		int stat_true = dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT];

		char *platform = readline("==> ");

		if (strfind(platform, "L", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_pawncc("linux");
loop_ipcc:
			if (ret == -1 && stat_true)
				goto loop_ipcc;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "W", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_pawncc("windows");
loop_ipcc2:
			if (ret == -1 && stat_true)
				goto loop_ipcc2;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "T", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			int ret = dog_install_pawncc("termux");
loop_ipcc3:
			if (ret == -1 && stat_true)
				goto loop_ipcc3;
			else if (ret == 0) {
				if (unit_pre_command &&
				    unit_pre_command[0] != '\0')
					goto unit_done;
				else
					goto _ptr_command;
			}
		} else if (strfind(platform, "E", true)) {
			free(ptr_command);
			ptr_command = NULL;
			free(platform);
			if (unit_pre_command && unit_pre_command[0] != '\0')
				goto unit_done;
			else
				goto _ptr_command;
		} else {
			pr_error(stdout,
			    "Invalid platform selection. Input 'E/e' to exit");
			free(platform);
			platform = NULL;
			goto ret_ptr2;
		}

		goto unit_done;
	} else if (strcmp(ptr_command, "debug") == 0) {
		dog_console_title("Watchdogs | @ debug");

		unit_ret_main("812C397D");

		goto unit_done;
	} else if (strcmp(ptr_command, "812C397D") == 0) {
		dog_server_crash_check();
		dog_free(ptr_command);
		ptr_command = NULL;
		return 3;
	} else if (strncmp(ptr_command, "compile", strlen("compile")) == 0 &&
	    !isalpha((unsigned char)ptr_command[strlen("compile")])) {
		dog_console_title("Watchdogs | @ compile | "
		    "logging file: .watchdogs/compiler.log");

		char *args;
		char *compile_args;
		char *second_arg = NULL;
		char *four_arg = NULL;
		char *five_arg = NULL;
		char *six_arg = NULL;
		char *seven_arg = NULL;
		char *eight_arg = NULL;
		char *nine_arg = NULL;
		char *ten_arg = NULL;

		args = ptr_command + strlen("compile");

		while (*args == ' ') {
			args++;
		}

		compile_args = strtok(args, " ");
		second_arg = strtok(NULL, " ");
		four_arg = strtok(NULL, " ");
		five_arg = strtok(NULL, " ");
		six_arg = strtok(NULL, " ");
		seven_arg = strtok(NULL, " ");
		eight_arg = strtok(NULL, " ");
		nine_arg = strtok(NULL, " ");
		ten_arg = strtok(NULL, " ");

		dog_exec_compiler(args, compile_args, second_arg, four_arg,
		    five_arg, six_arg, seven_arg, eight_arg,
		    nine_arg, ten_arg);

		goto unit_done;
	} else if (strncmp(ptr_command, "running", strlen("running")) == 0) {
_endpoints_:
		dog_stop_server_tasks();

		if (!path_access(dogconfig.dog_toml_binary)) {
			pr_error(stdout,
			    "can't locate sa-mp/open.mp binary file!");
			goto unit_done;
		}
		if (!path_access(dogconfig.dog_toml_config)) {
			pr_warning(stdout,
			    "can't locate %s - config file!",
			    dogconfig.dog_toml_config);
			goto unit_done;
		}

		if (dir_exists(".watchdogs") == 0)
			MKDIR(".watchdogs");

		int access_debugging_file = path_access(dogconfig.dog_toml_logs);
		if (access_debugging_file)
			remove(dogconfig.dog_toml_logs);
		access_debugging_file = path_access(dogconfig.dog_toml_logs);
		if (access_debugging_file)
			remove(dogconfig.dog_toml_logs);

		size_t cmd_len = 7;
		char *args = ptr_command + cmd_len;
		while (*args == ' ')
			++args;
		char *args2 = NULL;
		args2 = strtok(args, " ");

		char *size_arg1 = NULL;
		if (args2 == NULL || args2[0] == '\0')
			size_arg1 = dogconfig.dog_toml_proj_output;
		else
			size_arg1 = args2;

		size_t needed = snprintf(NULL, 0, "Watchdogs | "
		    "@ running | " "args: %s | "
		    "config: %s | " "CTRL + C to stop. | \"debug\" for debugging",
		    size_arg1, dogconfig.dog_toml_config) + 1;
		char *title_running_info = dog_malloc(needed);
		if (!title_running_info) {
			goto unit_done;
		}
		snprintf(title_running_info, needed,
		    "Watchdogs | "
		    "@ running | "
		    "args: %s | "
		    "config: %s | "
		    "CTRL + C to stop. | \"debug\" for debugging",
		    size_arg1,
		    dogconfig.dog_toml_config);
		if (title_running_info) {
#ifdef DOG_ANDROID
			println(stdout, "%s", title_running_info);
#else
			dog_console_title(title_running_info);
#endif
			dog_free(title_running_info);
			title_running_info = NULL;
		}

		int _dog_config_acces = path_access(dogconfig.dog_toml_config);
		if (!_dog_config_acces) {
			pr_error(stdout, "%s not found!",
			    dogconfig.dog_toml_config);
			goto unit_done;
		}

		char *command = dog_malloc(DOG_PATH_MAX);
		if (!command)
			goto unit_done;
		struct sigaction sa;

		if (path_access("announce"))
			CHMOD_FULL("announce");

		int rate_endpoint_failed = -1;

		if (dog_server_env() == 1) {
			if (args2 == NULL ||
			    (args2[0] == '.' && args2[1] == '\0')) {
start_main:
				sa.sa_handler = unit_sigint_handler;
				sigemptyset(&sa.sa_mask);
				sa.sa_flags = SA_RESTART;

				if (sigaction(SIGINT, &sa, NULL) == -1) {
					perror("sigaction");
					exit(EXIT_FAILURE);
				}

				time_t start, end;
				double elapsed;
				int ret_serv = 0;
				back_start:
				start = time(NULL);
				printf(DOG_COL_B_BLUE "");

				#ifdef DOG_WINDOWS
				{
					STARTUPINFOA _STARTUPINFO;
					PROCESS_INFORMATION _PROCESS_INFO;

					char command[DOG_PATH_MAX * 2];

					_ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
					_ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

					_STARTUPINFO.cb = sizeof(_STARTUPINFO);
					_STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

					_STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
					_STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
					_STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

					snprintf(command, sizeof(command),
						"%s%s", SYM_PROG, dogconfig.dog_toml_binary);

					if (!CreateProcessA(
						NULL,command,NULL,NULL,TRUE,0,NULL,NULL,
						&_STARTUPINFO,
						&_PROCESS_INFO))
					{
						rate_endpoint_failed = -1;
					}
					else
					{
						WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
						CloseHandle(_PROCESS_INFO.hProcess);
						CloseHandle(_PROCESS_INFO.hThread);
						rate_endpoint_failed = 0;
					}
				}
				#else
				{
					pid_t pid;

					CHMOD_FULL(dogconfig.dog_toml_binary);

					char cmd[DOG_PATH_MAX + 26];
					snprintf(cmd, sizeof(cmd), "%s/%s",
						dog_procure_pwd(), dogconfig.dog_toml_binary);

					int stdout_pipe[2];
					int stderr_pipe[2];

					if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
						perror("pipe");
						goto unit_done;
					}

					pid = fork();
					if (pid == 0) {
						close(stdout_pipe[0]);
						close(stderr_pipe[0]);

						dup2(stdout_pipe[1], STDOUT_FILENO);
						dup2(stderr_pipe[1], STDERR_FILENO);

						close(stdout_pipe[1]);
						close(stderr_pipe[1]);

						char *argv[] = {
							"/bin/sh",
							"-c",
							(char *)cmd,
							NULL
						};
						execv("/bin/sh", argv);
						_exit(127);
					} else if (pid > 0) {
						close(stdout_pipe[1]);
						close(stderr_pipe[1]);

						char buffer[DOG_MAX_PATH];
						ssize_t br;

						while ((br = read(stdout_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
							buffer[br] = '\0';
							printf("%s", buffer);
						}

						while ((br = read(stderr_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
							buffer[br] = '\0';
							printf("%s", buffer);
						}

						close(stdout_pipe[0]);
						close(stderr_pipe[0]);

						int status;
						waitpid(pid, &status, 0);

						if (WIFEXITED(status)) {
							int exit_code = WEXITSTATUS(status);
							if (exit_code == 0) {
								rate_endpoint_failed = 0;
							} else {
								rate_endpoint_failed = -1;
							}
						} else {
							rate_endpoint_failed = -1;
						}
					}
				}
				#endif

				if (rate_endpoint_failed == 0) {
					;
				} else {
					printf(DOG_COL_DEFAULT "\n");
					pr_color(stdout, DOG_COL_RED,
					    "Server startup failed!\n");

					elapsed = difftime(end, start);
					if (elapsed <= 4.1 && ret_serv == 0) {
						ret_serv = 1;
						printf("\ttry starting again..");
						access_debugging_file =
						    path_access(
						    dogconfig.dog_toml_logs);
						if (access_debugging_file)
							remove(
							    dogconfig.dog_toml_logs);
						access_debugging_file =
						    path_access(
						    dogconfig.dog_toml_logs);
						if (access_debugging_file)
							remove(
							    dogconfig.dog_toml_logs);
						end = time(NULL);
						goto back_start;
					}
				}
				printf(DOG_COL_DEFAULT "\n");
server_done:
				end = time(NULL);
				if (sigint_handler == 0)
					raise(SIGINT);

				printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
				char *debug_endpoint = readline(
				    "   answer (y/n): ");
				if (debug_endpoint && (debug_endpoint[0] == '\0' ||
				    strcmp(debug_endpoint, "Y") == 0 ||
				    strcmp(debug_endpoint, "y") == 0)) {
					free(ptr_command);
					ptr_command = strdup("debug");
					free(debug_endpoint);
					dog_free(command);
					command = NULL;
					goto _reexecute_command;
				}
				if (debug_endpoint)
					free(debug_endpoint);
			} else {
				if (dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] == DOG_GARBAGE_TRUE)
				{
					dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] = DOG_GARBAGE_ZERO;
					goto start_main;
				}
				dog_exec_samp_server(args2,
				    dogconfig.dog_toml_binary);
				restore_server_config();
				printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
				char *debug_endpoint = readline(
				    "   answer (y/n): ");
				if (debug_endpoint && (debug_endpoint[0] == '\0' ||
				    strcmp(debug_endpoint, "Y") == 0 ||
				    strcmp(debug_endpoint, "y") == 0)) {
					free(ptr_command);
					ptr_command = strdup("debug");
					free(debug_endpoint);
					dog_free(command);
					command = NULL;
					goto _reexecute_command;
				}
				if (debug_endpoint)
					free(debug_endpoint);
			}
		} else if (dog_server_env() == 2) {
			if (args2 == NULL ||
			    (args2[0] == '.' && args2[1] == '\0')) {
start_main2:
				sa.sa_handler = unit_sigint_handler;
				sigemptyset(&sa.sa_mask);
				sa.sa_flags = SA_RESTART;

				if (sigaction(SIGINT, &sa, NULL) == -1) {
					perror("sigaction");
					exit(EXIT_FAILURE);
				}

				time_t start, end;
				double elapsed;
				int ret_serv = 0;
				back_start2:
				start = time(NULL);
				printf(DOG_COL_B_BLUE "");

				#ifdef DOG_WINDOWS
				{
					STARTUPINFOA _STARTUPINFO;
					PROCESS_INFORMATION _PROCESS_INFO;

					char command[DOG_PATH_MAX * 2];

					_ZERO_MEM_WIN32(&_STARTUPINFO, sizeof(_STARTUPINFO));
					_ZERO_MEM_WIN32(&_PROCESS_INFO, sizeof(_PROCESS_INFO));

					_STARTUPINFO.cb = sizeof(_STARTUPINFO);
					_STARTUPINFO.dwFlags = STARTF_USESTDHANDLES;

					_STARTUPINFO.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
					_STARTUPINFO.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
					_STARTUPINFO.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

					snprintf(command, sizeof(command),
						"%s%s", SYM_PROG, dogconfig.dog_toml_binary);

					if (!CreateProcessA(
						NULL,command,NULL,NULL,TRUE,0,NULL,NULL,
						&_STARTUPINFO,
						&_PROCESS_INFO))
					{
						rate_endpoint_failed = -1;
					}
					else
					{
						WaitForSingleObject(_PROCESS_INFO.hProcess, INFINITE);
						CloseHandle(_PROCESS_INFO.hProcess);
						CloseHandle(_PROCESS_INFO.hThread);
						rate_endpoint_failed = 0;
					}
				}
				#else
				{
					pid_t pid;

					CHMOD_FULL(dogconfig.dog_toml_binary);

					char cmd[DOG_PATH_MAX + 26];
					snprintf(cmd, sizeof(cmd), "%s/%s",
						dog_procure_pwd(), dogconfig.dog_toml_binary);

					int stdout_pipe[2];
					int stderr_pipe[2];

					if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
						perror("pipe");
						goto unit_done;
					}

					pid = fork();
					if (pid == 0) {
						close(stdout_pipe[0]);
						close(stderr_pipe[0]);

						dup2(stdout_pipe[1], STDOUT_FILENO);
						dup2(stderr_pipe[1], STDERR_FILENO);

						close(stdout_pipe[1]);
						close(stderr_pipe[1]);

						char *argv[] = {
							"/bin/sh",
							"-c",
							(char *)cmd,
							NULL
						};
						execv("/bin/sh", argv);
						_exit(127);
					} else if (pid > 0) {
						close(stdout_pipe[1]);
						close(stderr_pipe[1]);

						char buffer[DOG_MAX_PATH];
						ssize_t br;

						while ((br = read(stdout_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
							buffer[br] = '\0';
							printf("%s", buffer);
						}

						while ((br = read(stderr_pipe[0], buffer, sizeof(buffer)-1)) > 0) {
							buffer[br] = '\0';
							printf("%s", buffer);
						}

						close(stdout_pipe[0]);
						close(stderr_pipe[0]);

						int status;
						waitpid(pid, &status, 0);

						if (WIFEXITED(status)) {
							int exit_code = WEXITSTATUS(status);
							if (exit_code == 0) {
								rate_endpoint_failed = 0;
							} else {
								rate_endpoint_failed = -1;
							}
						} else {
							rate_endpoint_failed = -1;
						}
					}
				}
				#endif

				if (rate_endpoint_failed != 0) {
					printf(DOG_COL_DEFAULT "\n");
					pr_color(stdout, DOG_COL_RED,
					    "Server startup failed!\n");

					elapsed = difftime(end, start);
					if (elapsed <= 4.1 && ret_serv == 0) {
						ret_serv = 1;
						printf("\ttry starting again..");
						access_debugging_file =
						    path_access(
						    dogconfig.dog_toml_logs);
						if (access_debugging_file)
							remove(
							    dogconfig.dog_toml_logs);
						access_debugging_file =
						    path_access(
						    dogconfig.dog_toml_logs);
						if (access_debugging_file)
							remove(
							    dogconfig.dog_toml_logs);
						end = time(NULL);
						goto back_start2;
					}
				}
				printf(DOG_COL_DEFAULT "\n");
server_done2:
				end = time(NULL);
				if (sigint_handler == 0) {
					raise(SIGINT);
				}

				printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
				char *debug_endpoint = readline(
				    "   answer (y/n): ");
				if (debug_endpoint && (debug_endpoint[0] == '\0' ||
				    strcmp(debug_endpoint, "Y") == 0 ||
				    strcmp(debug_endpoint, "y") == 0)) {
					free(ptr_command);
					ptr_command = strdup("debug");
					free(debug_endpoint);
					dog_free(command);
					command = NULL;
					goto _reexecute_command;
				}
				if (debug_endpoint)
					free(debug_endpoint);
			} else {
				if (dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] == DOG_GARBAGE_TRUE)
				{
					dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] = DOG_GARBAGE_ZERO;
					goto start_main;
				}
				dog_exec_omp_server(args2,
				    dogconfig.dog_ptr_omp);
				restore_server_config();
				printf("\x1b[32m==> create debugging runner?\x1b[0m\n");
				char *debug_endpoint = readline(
				    "   answer (y/n): ");
				if (debug_endpoint && (debug_endpoint[0] == '\0' ||
				    strcmp(debug_endpoint, "Y") == 0 ||
				    strcmp(debug_endpoint, "y") == 0)) {
					free(ptr_command);
					ptr_command = strdup("debug");
					free(debug_endpoint);
					dog_free(command);
					command = NULL;
					goto _reexecute_command;
				}
				if (debug_endpoint)
					free(debug_endpoint);
			}
		} else {
			pr_error(stdout,
			    "\033[1;31merror:\033[0m sa-mp/open.mp server not found!\n"
			    "  \033[2mhelp:\033[0m install it before continuing\n");
			printf("\n  \033[1mInstall now?\033[0m  [\033[32mY\033[0m/\033[31mn\033[0m]: ");

			dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT] = DOG_GARBAGE_TRUE;
			int stat_true = dogconfig.dog_garbage_access[DOG_GARBAGE_SELECTION_STAT];

			char *pointer_signalA;
ret_ptr3:
			pointer_signalA = readline("");

			while (true) {
				if (pointer_signalA &&
				    (strcmp(pointer_signalA, "Y") == 0 ||
				    strcmp(pointer_signalA, "y") == 0)) {
					free(pointer_signalA);
					if (!strcmp(dogconfig.dog_os_type,
					    OS_SIGNAL_WINDOWS)) {
						int ret = dog_install_server(
						    "windows");
n_loop_igm:
						if (ret == -1 &&
						    stat_true)
							goto n_loop_igm;
					} else if (!strcmp(
					    dogconfig.dog_os_type,
					    OS_SIGNAL_LINUX)) {
						int ret = dog_install_server(
						    "linux");
n_loop_igm2:
						if (ret == -1 &&
						    stat_true)
							goto n_loop_igm2;
					}
					break;
				} else if (pointer_signalA &&
				    (strcmp(pointer_signalA, "N") == 0 ||
				    strcmp(pointer_signalA, "n") == 0)) {
					free(pointer_signalA);
					break;
				} else {
					pr_error(stdout,
					    "Invalid input. Please type Y/y to install or N/n to cancel.");
					free(pointer_signalA);
					pointer_signalA = NULL;
					goto ret_ptr3;
				}
			}
		}

		if (command) {
			free(command);
			command = NULL;
		}
		goto unit_done;
	} else if (strncmp(ptr_command, "compiles", strlen("compiles")) == 0) {
		dog_console_title("Watchdogs | @ compiles");

		char *args = ptr_command + strlen("compiles");
		while (*args == ' ')
			++args;
		char *args2 = NULL;
		args2 = strtok(args, " ");

		if (args2 == NULL || args2[0] == '\0') {
			const char *argsc[] = { NULL, ".", NULL, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL };

			dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] = DOG_GARBAGE_TRUE;

			dog_exec_compiler(argsc[0], argsc[1], argsc[2], argsc[3],
			    argsc[4], argsc[5], argsc[6], argsc[7],
			    argsc[8], argsc[9]);

			if (dogconfig.dog_compiler_stat < 1) {
				goto _endpoints_;
			}
		} else {
			const char *argsc[] = { NULL, args2, NULL, NULL, NULL,
			    NULL, NULL, NULL, NULL, NULL };

			dogconfig.dog_garbage_access[DOG_GARBAGE_COMPILE_N_RUNNING_STAT] = DOG_GARBAGE_TRUE;

			dog_exec_compiler(argsc[0], argsc[1], argsc[2], argsc[3],
			    argsc[4], argsc[5], argsc[6], argsc[7],
			    argsc[8], argsc[6]);

			if (dogconfig.dog_compiler_stat < 1) {
				size_t cmd_len = strlen(args) + 10;
				char *size_command = dog_malloc(cmd_len);
				if (!size_command)
					goto unit_done;
				snprintf(size_command, cmd_len, "running %s",
				    args);
				free(ptr_command);
				ptr_command = size_command;

				goto _endpoints_;
			}
		}

		goto unit_done;
	} else if (strcmp(ptr_command, "stop") == 0) {
		dog_console_title("Watchdogs | @ stop");

		dog_stop_server_tasks();

		goto unit_done;
	} else if (strcmp(ptr_command, "restart") == 0) {
		dog_console_title("Watchdogs | @ restart");

		dog_stop_server_tasks();
		sleep(1);
		goto _endpoints_;
	} else if (strncmp(ptr_command, "tracker", strlen("tracker")) == 0) {
		char *args = ptr_command + strlen("tracker");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: tracker [<name>]");
		} else {
			CURL *curl;
			curl_global_init(CURL_GLOBAL_DEFAULT);
			curl = curl_easy_init();
			if (!curl) {
				fprintf(stderr, "Curl initialization failed!\n");
				goto unit_done;
			}

			int variation_count = 0;
			char variations[MAX_VARIATIONS][MAX_USERNAME_LEN];

			tracker_discrepancy(args, variations, &variation_count);

			printf("[TRACKER] Search base: %s\n", args);
			printf("[TRACKER] Generated %d Variations\n\n",
			    variation_count);

			for (int i = 0; i < variation_count; i++) {
				printf("=== TRACKING ACCOUNTS: %s ===\n",
				    variations[i]);
				tracking_username(curl, variations[i]);
				printf("\n");
			}

			curl_easy_cleanup(curl);
			curl_global_cleanup();
		}

		goto unit_done;
	} else if (strncmp(ptr_command, "compress", strlen("compress")) == 0) {
		char *args = ptr_command + strlen("compress");
		while (*args == ' ')
			args++;

		if (*args == '\0') {
			printf("Usage: compress --file <input> "
			    "--output <output> --type <format>\n");
			printf("Example:\n\tcompress --file myfile.txt "
			    "--output myarchive.zip --type zip\n\t"
			    "compress --file myfolder/ "
			    "--output myarchive.tar.gz --type gz\n");
			goto unit_done;
		}

		char *raw_input = NULL, *raw_output = NULL, *raw_type = NULL;

		char *procure_args = strtok(args, " ");
		while (procure_args) {
			if (strcmp(procure_args, "--file") == 0) {
				procure_args = strtok(NULL, " ");
				if (procure_args)
					raw_input = procure_args;
			} else if (strcmp(procure_args, "--output") == 0) {
				procure_args = strtok(NULL, " ");
				if (procure_args)
					raw_output = procure_args;
			} else if (strcmp(procure_args, "--type") == 0) {
				procure_args = strtok(NULL, " ");
				if (procure_args)
					raw_type = procure_args;
			}
			procure_args = strtok(NULL, " ");
		}

		if (!raw_input || !raw_output || !raw_type) {
			printf("Missing arguments!\n");
			printf("Usage: compress "
			    "--file <input> --output <output> --type <zip|tar|gz|bz2|xz>\n");
			printf("Example:\n\tcompress --file myfile.txt "
			    "--output myarchive.zip --type zip\n\t"
			    "compress --file myfolder/ "
			    "--output myarchive.tar.gz --type gz\n");
			goto unit_done;
		}

		CompressionFormat fmt;

		if (strcmp(raw_type, "zip") == 0)
			fmt = COMPRESS_ZIP;
		else if (strcmp(raw_type, "tar") == 0)
			fmt = COMPRESS_TAR;
		else if (strcmp(raw_type, "gz") == 0)
			fmt = COMPRESS_TAR_GZ;
		else if (strcmp(raw_type, "bz2") == 0)
			fmt = COMPRESS_TAR_BZ2;
		else if (strcmp(raw_type, "xz") == 0)
			fmt = COMPRESS_TAR_XZ;
		else {
			printf("Unknown type: %s\n", raw_type);
			printf("Supported: zip, tar, gz, bz2, xz\n");
			goto unit_done;
		}

		const char *procure_items[] = { raw_input };

		int ret = compress_to_archive(raw_output, procure_items, 1, fmt);
		if (ret == 0)
			pr_info(stdout,
			    "Converter file/folder "
			    "to archive (Compression) successfully: %s\n",
			    raw_output);
		else {
			pr_error(stdout, "Compression failed!\n");
			minimal_debugging();
		}

		goto unit_done;
	} else if (strncmp(ptr_command, "send", strlen("send")) == 0) {
		char *args = ptr_command + strlen("send");
		while (*args == ' ')
			++args;

		if (*args == '\0') {
			println(stdout, "Usage: send [<file_path>]");
		} else {
			if (path_access(args) == 0) {
				pr_error(stdout, "file not found: %s", args);
				goto send_done;
			}
			if (!dogconfig.dog_toml_webhooks ||
			    strfind(dogconfig.dog_toml_webhooks,
			    "DO_HERE", true) ||
			    strlen(dogconfig.dog_toml_webhooks) < 1) {
				pr_color(stdout, DOG_COL_YELLOW,
				    " ~ Discord webhooks not available");
				goto send_done;
			}

			char *filename = args;
			if (strrchr(args, __PATH_CHR_SEP_LINUX) &&
			    strrchr(args, __PATH_CHR_SEP_WIN32)) {
				filename = (strrchr(args,
				    __PATH_CHR_SEP_LINUX) >
				    strrchr(args, __PATH_CHR_SEP_WIN32)) ?
				    strrchr(args, __PATH_CHR_SEP_LINUX) + 1 :
				    strrchr(args, __PATH_CHR_SEP_WIN32) + 1;
			} else if (strrchr(args, __PATH_CHR_SEP_LINUX)) {
				filename = strrchr(args,
				    __PATH_CHR_SEP_LINUX) + 1;
			} else if (strrchr(args, __PATH_CHR_SEP_WIN32)) {
				filename = strrchr(args,
				    __PATH_CHR_SEP_WIN32) + 1;
			} else {
				;
			}

			CURL *curl = curl_easy_init();
			if (curl) {
				CURLcode res;
				curl_mime *mime;

				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,
				    1L);
				curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,
				    2L);
				curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING,
				    "gzip");

				mime = curl_mime_init(curl);
				if (!mime) {
					fprintf(stderr,
					    "Failed to create MIME handle\n");
					minimal_debugging();
					curl_easy_cleanup(curl);
					goto send_done;
				}

				curl_mimepart *part;

				time_t t = time(NULL);
				struct tm tm    = *localtime(&t);
				char *timestamp = dog_malloc(64);
				if (timestamp) {
					snprintf(timestamp, 64,
					"%04d/%02d/%02d %02d:%02d:%02d",
					tm.tm_year + 1900, tm.tm_mon + 1,
					tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec);
				}

				portable_stat_t st;
				if (portable_stat(filename, &st) == 0) {
					char *content_data = dog_malloc(
					    DOG_MAX_PATH);
					if (content_data) {
						snprintf(content_data,
						DOG_MAX_PATH,
						"### received send command - %s\n"
						"> Metadata\n"
						"- Name: %s\n- Size: %llu bytes\n- Last modified: %llu\n%s",
						timestamp ? timestamp :
						"unknown",
						filename,
						(unsigned long long)st.st_size,
						(unsigned long long)st.st_lmtime,
						"-# Please note that if you are using webhooks with a public channel,"
						"always convert the file into an archive with a password known only to you.");

						part = curl_mime_addpart(mime);
						curl_mime_name(part, "content");
						curl_mime_data(part,
						    content_data,
						    CURL_ZERO_TERMINATED);
						dog_free(content_data);
						content_data = NULL;
					}
				}

				part = curl_mime_addpart(mime);
				curl_mime_name(part, "file");
				curl_mime_filedata(part, args);
				curl_mime_filename(part, filename);

				curl_easy_setopt(curl, CURLOPT_URL,
				    dogconfig.dog_toml_webhooks);
				curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

				res = curl_easy_perform(curl);
				if (res != CURLE_OK)
					fprintf(stderr,
					    "curl_easy_perform() failed: %s\n",
					    curl_easy_strerror(res));

				curl_mime_free(mime);
				curl_easy_cleanup(curl);
				if (timestamp) {
					free(timestamp);
					timestamp = NULL;
				}
			}

			curl_global_cleanup();
		}

send_done:
		goto unit_done;
	} else if (strcmp(ptr_command, "watchdogs") == 0 ||
	    strcmp(ptr_command, "dog") == 0) {

		unit_show_dog();

		goto unit_done;
	} else if (strcmp(ptr_command, command_similar) != 0 && dist <= 2) {
		dog_console_title("Watchdogs | @ undefined");
		println(stdout,
		    "watchdogs: '%s' is not valid watchdogs command. See 'help'.",
		    ptr_command);
		println(stdout, "   but did you mean '%s'?", command_similar);
		goto unit_done;
	} else {
		int ret;
		size_t cmd_len;
		char *command = NULL;
		ret = -3;
		cmd_len = strlen(ptr_command) + DOG_PATH_MAX;
		command = dog_malloc(cmd_len);
		if (!command)
			goto unit_done;
		if (strfind(ptr_command, "clear", true) && is_native_windows())
			ptr_command = strdup("cls");
		if (path_access("/bin/sh") != 0)
			snprintf(command, cmd_len, "/bin/sh -c '%s'",
			    ptr_command);
		else if (path_access("~/.bashrc") != 0)
			snprintf(command, cmd_len, "bash -c '%s'",
			    ptr_command);
		else if (path_access("~/.zshrc") != 0)
			snprintf(command, cmd_len, "zsh -c '%s'",
			    ptr_command);
		else
			snprintf(command, cmd_len, "%s", ptr_command);
		char *argv[] = { command, NULL };
powershell:
		ret = dog_exec_command(argv);
		if (ret)
			dog_console_title("Watchdogs | @ command not found");
		dog_free(command);
		command = NULL;
		if (strcmp(ptr_command, "clear") == 0 ||
		    strcmp(ptr_command, "cls") == 0)
			return -2;
		else
			return -1;
	}

unit_done:
	fflush(stdout);
	if (ptr_command) {
		free(ptr_command);
		ptr_command = NULL;
	}
	if (ptr_prompt) {
		free(ptr_prompt);
		ptr_prompt = NULL;
	}

	return -1;
}

void
unit_ret_main(void *unit_pre_command)
{
	int ret = -3;
	if (unit_pre_command != NULL) {
		char *procure_command_argv = (char *)unit_pre_command;
		ret = __command__(procure_command_argv);
		clock_gettime(CLOCK_MONOTONIC, &cmd_end);
		if (ret == -2) {
			return;
		}
		if (ret == 3) {
			return;
		}
		command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
		    (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
		pr_color(stdout,
		    DOG_COL_CYAN,
		    " <I> (interactive) Finished at %.3fs\n",
		    command_dur);
		return;
	}

loop_main:
	ret = __command__(NULL);
	if (ret == -1) {
		clock_gettime(CLOCK_MONOTONIC, &cmd_end);
		command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
		    (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;
		pr_color(stdout,
		    DOG_COL_CYAN,
		    " <I> (interactive) Finished at %.3fs\n",
		    command_dur);
		goto loop_main;
	} else if (ret == 2) {
		clock_gettime(CLOCK_MONOTONIC, &cmd_end);
		dog_console_title("Terminal.");
		exit(0);
	} else if (ret == -2) {
		clock_gettime(CLOCK_MONOTONIC, &cmd_end);
		goto loop_main;
	} else if (ret == 3) {
		clock_gettime(CLOCK_MONOTONIC, &cmd_end);
	} else {
		goto basic_end;
	}

basic_end:

	clock_gettime(CLOCK_MONOTONIC, &cmd_end);
	command_dur = (cmd_end.tv_sec - cmd_start.tv_sec) +
	    (cmd_end.tv_nsec - cmd_start.tv_nsec) / 1e9;

	pr_color(stdout,
	    DOG_COL_CYAN,
	    " <I> (interactive) Finished at %.3fs\n",
	    command_dur);
	goto loop_main;
}

int
main(int argc, char *argv[])
{
	/**
	 * ./watchdogs compile .
	   - ./watchdogs.tmux compile .
	   - ./watchdogs.win compile .

	 * ./watchdogs
	   - ./watchdogs.tmux
	   - ./watchdogs.win
	   & compile .
	 */
	setvbuf(stdout, NULL, _IONBF, 0);

	unit_debugging(0);

	if (argc > 1) {
		int i;
		size_t unit_total_len = 0;

		for (i = 1; i < argc; ++i)
			unit_total_len += strlen(argv[i]) + 1;

		char *unit_size_prompt = dog_malloc(unit_total_len);
		if (!unit_size_prompt)
			return 0;

		char *ptr = unit_size_prompt;
		for (i = 1; i < argc; ++i) {
			if (i > 1)
				*ptr++ = ' ';
			size_t len = strlen(argv[i]);
			memcpy(ptr, argv[i], len);
			ptr += len;
		}
		*ptr = '\0';

		unit_ret_main(unit_size_prompt);

		dog_free(unit_size_prompt);
		unit_size_prompt = NULL;

		return 0;
	} else {
		unit_ret_main(NULL);
	}

	return 0;
}
