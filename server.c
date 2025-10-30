#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include "chain.h"
#include "color.h"
#include "extra.h"
#include "utils.h"
#include "server.h"

/**
 * wd_stop_server_tasks - Stop all running server processes
 */
void wd_stop_server_tasks(void)
{
		kill_process("samp-server.exe");
		kill_process("samp03svr");
		kill_process("omp-server.exe");
		kill_process("omp-server");
}

/**
 * update_server_config - Update server.cfg with new gamemode
 * @gamemode: Gamemode filename
 *
 * Return: 0 on success, -1 on failure
 */
static int update_server_config(const char *gamemode)
{
		FILE *config_in, *config_out;
		char line[1024];
		int gamemode_updated = 0;

		char __sz_mv[MAX_PATH];
		snprintf(__sz_mv, sizeof(__sz_mv), "mv -f %s %s",
										   "server.cfg",
										   ".server.cfg.bak");
		system(__sz_mv);

		config_in = fopen(".server.cfg.bak", "r");
		if (!config_in) {
				pr_error(stdout, "Failed to open backup config");
				return -RETN;
		}
		config_out = fopen("server.cfg", "w");
		if (!config_out) {
				pr_error(stdout, "Failed to write new config");
				fclose(config_in);
				return -RETN;
		}

		char _gamemode[256];
		snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
		char *__dot_ext = strrchr(_gamemode, '.');
		if (__dot_ext) *__dot_ext = '\0';
		gamemode = _gamemode; 

		/* Update gamemode line */
		while (fgets(line, sizeof(line), config_in)) {
				if (strncmp(line, "gamemode0 ", 10) == 0) {
						fprintf(config_out, "gamemode0 %s\n", gamemode);
						gamemode_updated = 1;
				} else {
						fputs(line, config_out);
				}
		}

		/* Add gamemode if not found */
		if (!gamemode_updated)
				fprintf(config_out, "gamemode0 %s\n", gamemode);

		fclose(config_in);
		fclose(config_out);

		return RETZ;
}

/**
 * wd_display_server_logs - Display server logs on Unix systems
 */
void wd_display_server_logs(int ret)
{
		FILE *log_file;
		int ch;

		if (!ret)
			log_file = fopen("server_log.txt", "r");
		else
			log_file = fopen("log.txt", "r");

next:
		if (!log_file) {
			return;
		}

		while ((ch = fgetc(log_file)) != EOF)
				putchar(ch);

		fclose(log_file);
}

/**
 * restore_server_config - Restore original server configuration
 */
static void restore_server_config(void)
{
		remove("server.cfg");
		rename(".server.cfg.bak", "server.cfg");
}

/**
 * wd_run_samp_server - Run SA-MP server with specified gamemode
 * @gamemode: Gamemode filename
 * @server_bin: Server binary name
 */
void wd_run_samp_server(const char *gamemode, const char *server_bin)
{
		char command[256];
		int ret;

		char _gamemode[256];
		snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
		char *__dot_ext = strrchr(gamemode, '.');
		if (!__dot_ext) gamemode = _gamemode; 

		/* Verify gamemode exists */
		wd_sef_fdir_reset();
		if (wd_sef_fdir(".", gamemode, NULL) != 1) {
				pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
				__main(0);
		}

		/* Update server configuration */
		if (update_server_config(gamemode) != 0)
				__main(0);

		/* Set executable permissions */
		CHMOD(server_bin, FILE_MODE);

		/* Build and execute command */
#ifdef _WIN32
		snprintf(command, sizeof(command), "%s", server_bin);
#else
		snprintf(command, sizeof(command), "./%s", server_bin);
#endif

		ret = system(command);
		if (ret == RETZ) {
			if (!strcmp(wcfg.wd_os_type, OS_SIGNAL_LINUX)) {
				sleep(2);
				wd_display_server_logs(0);
			}
		} else {
				pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
		}

		/* Restore original configuration */
		restore_server_config();

		__main(0);
}

/**
 * update_omp_config - Update OpenMP config.json with new gamemode
 * @gamemode: Gamemode filename
 *
 * Return: 0 on success, -1 on failure
 */
static int update_omp_config(const char *gamemode)
{
		FILE *config_in = NULL, *config_out = NULL;
		cJSON *root = NULL, *pawn = NULL, *main_scripts = NULL;
		char *__cJSON_Data = NULL, *__cJSON_Printed = NULL;
		struct stat st;
		char gamemode_buf[256];
		char _gamemode[256];
		int ret = -RETN;

		/* Create backup */
		char __sz_mv[MAX_PATH];
		snprintf(__sz_mv, sizeof(__sz_mv), "mv -f %s %s",
										   "config.json",
										   ".config.json.bak");
		if (system(__sz_mv) != 0) {
			pr_error(stdout, "Failed to create backup file");
			return -RETN;
		}

		/* Use stat to get file size */
		if (stat(".config.json.bak", &st) != 0) {
			pr_error(stdout, "Failed to get file status");
			return -RETN;
		}

		/* Open input file */
		config_in = fopen(".config.json.bak", "rb");
		if (!config_in) {
			pr_error(stdout, "Failed to open .config.json.bak");
			return -RETN;
		}

		/* Allocate memory */
		__cJSON_Data = wdmalloc(st.st_size + 1);
		if (!__cJSON_Data) {
			pr_error(stdout, "Memory allocation failed");
			goto done;
		}

		/* Read entire file at once */
		size_t bytes_read = fread(__cJSON_Data, 1, st.st_size, config_in);
		if (bytes_read != (size_t)st.st_size) {
			pr_error(stdout, "Incomplete file read (%zu of %ld bytes)",
					bytes_read,
					st.st_size);
			goto done;
		}

		__cJSON_Data[st.st_size] = '\0';
		fclose(config_in);
		config_in = NULL;
		
		/* Parse JSON */
		root = cJSON_Parse(__cJSON_Data);
		if (!root) {
			pr_error(stdout, "JSON parse error: %s", cJSON_GetErrorPtr());
			goto done;
		}

		/* Get pawn section */
		pawn = cJSON_GetObjectItem(root, "pawn");
		if (!pawn) {
			pr_error(stdout, "Missing 'pawn' section in config");
			goto done;
		}

		/* Process gamemode name */
		snprintf(_gamemode, sizeof(_gamemode), "%s", gamemode);
		char *__dot_ext = strrchr(_gamemode, '.');
		if (__dot_ext) *__dot_ext = '\0';

		/* Update main_scripts array */
		cJSON_DeleteItemFromObject(pawn, "main_scripts");
		
		main_scripts = cJSON_CreateArray();
		snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", _gamemode);
		cJSON_AddItemToArray(main_scripts, cJSON_CreateString(gamemode_buf));
		cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

		/* Write updated config */
		config_out = fopen("config.json", "w");
		if (!config_out) {
			pr_error(stdout, "Failed to write config.json");
			goto done;
		}

		__cJSON_Printed = cJSON_Print(root);
		if (!__cJSON_Printed) {
			pr_error(stdout, "Failed to print JSON");
			goto done;
		}

		if (fputs(__cJSON_Printed, config_out) == EOF) {
			pr_error(stdout, "Failed to write to config.json");
			goto done;
		}

		ret = RETZ;

done:
		/* Cleanup resources in reverse allocation order */
		if (config_out)
			fclose(config_out);
		if (config_in)
			fclose(config_in);
		if (__cJSON_Printed)
			wdfree(__cJSON_Printed);
		if (root)
			cJSON_Delete(root);
		if (__cJSON_Data)
			wdfree(__cJSON_Data);

		return ret;
}

/**
 * restore_omp_config - Restore original OpenMP configuration
 */
static void restore_omp_config(void)
{
		remove("config.json");
		rename(".config.json.bak", "config.json");
}

/**
 * wd_run_omp_server - Run OpenMP server with specified gamemode
 * @gamemode: Gamemode filename
 * @server_bin: Server binary name
 */
void wd_run_omp_server(const char *gamemode, const char *server_bin)
{
		char command[256];
		int ret;

		char _gamemode[256];
		snprintf(_gamemode, sizeof(_gamemode), "%s.amx", gamemode);
		char *__dot_ext = strrchr(gamemode, '.');
		if (!__dot_ext) gamemode = _gamemode; 

		/* Verify gamemode exists */
		wd_sef_fdir_reset();
		if (wd_sef_fdir(".", gamemode, NULL) != 1) {
				pr_error(stdout, "Cannot locate gamemode: %s", gamemode);
				__main(0);
		}

		/* Update OpenMP configuration */
		if (update_omp_config(gamemode) != 0)
				return;

		/* Set executable permissions */
		CHMOD(server_bin, FILE_MODE);

		/* Build and execute command */
#ifdef _WIN32
		snprintf(command, sizeof(command), "%s", server_bin);
#else
		snprintf(command, sizeof(command), "./%s", server_bin);
#endif

		ret = system(command);
		if (ret == RETZ) {
			sleep(2);
			wd_display_server_logs(1);
		} else {
			pr_color(stdout, FCOLOUR_RED, "Server startup failed!\n");
		}

		/* Restore original configuration */
		restore_omp_config();

		__main(0);
}
