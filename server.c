#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <io.h>
#define PATH_SYM "\\"
#define MKDIR(path) _mkdir(path)
#define SLEEP(sec) Sleep((sec) * 1000)
#define SETENV(name, val, overwrite) _putenv_s(name, val)
#define CHMOD(path, mode) _chmod(path, mode)
#define FILE_MODE _S_IREAD | _S_IWRITE
#else
#include <unistd.h>
#include <dirent.h>
#define PATH_SYM "/"
#define MKDIR(path) mkdir(path, 0755)
#define SLEEP(sec) sleep(sec)
#define SETENV(name, val, overwrite) setenv(name, val, overwrite)
#define CHMOD(path, mode) chmod(path, mode)
#define FILE_MODE 0777
#endif

#include "cJSON/cJSON.h"
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
		kill_process_safe("samp-server.exe");
		kill_process_safe("samp03svr");
		kill_process_safe("omp-server.exe");
		kill_process_safe("omp-server");
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
		char line[256];
		int gamemode_updated = 0;

		/* Backup original config */
		wd_sef_wcopy("server.cfg", ".server.cfg.bak");

		config_in = fopen(".server.cfg.bak", "r");
		if (!config_in) {
				printf_error("Failed to open backup config");
				return -RETN;
		}

		config_out = fopen("server.cfg", "w");
		if (!config_out) {
				printf_error("Failed to write new config");
				fclose(config_in);
				return -RETN;
		}

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
 * display_server_logs - Display server logs on Unix systems
 */
static void display_server_logs(void)
{
#ifndef _WIN32
		FILE *log_file;
		int ch;

		printf_color(COL_YELLOW, "Press enter to print logs..");
		getchar();

		log_file = fopen("server_log.txt", "r");
		if (!log_file) {
				printf_error("Cannot open server_log.txt");
				return;
		}

		while ((ch = fgetc(log_file)) != EOF)
				putchar(ch);

		fclose(log_file);
#endif
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

		/* Verify gamemode exists */
		wd_sef_fdir_reset();
		if (wd_sef_fdir(".", gamemode, NULL) != 1) {
				printf_error("Cannot locate gamemode: %s", gamemode);
				__main(0);
		}

		/* Update server configuration */
		if (update_server_config(gamemode) != 0)
				__main(0);

		/* Set executable permissions */
		CHMOD(server_bin, FILE_MODE);

		/* Build and execute command */
#ifdef _WIN32
		snprintf(command, sizeof(command), "./%s", server_bin);
#else
		snprintf(command, sizeof(command), "%s", server_bin);
#endif

		ret = wd_run_command(command);
		if (ret == 0) {
				SLEEP(2);
				display_server_logs();
		} else {
				printf_color(COL_RED, "Server startup failed!");
		}

		/* Restore original configuration */
		restore_server_config();

		/* Debug mode cleanup */
		if (wcfg.serv_dbg && strcmp(wcfg.serv_dbg, "debug") == 0) {
				if (wcfg.os_type == OS_SIGNAL_WINDOWS) {
						kill_process("samp-server.exe");
				} else if (wcfg.os_type == OS_SIGNAL_LINUX) {
						kill_process("samp03svr");
				}
		}

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
		FILE *config_file;
		cJSON *root, *pawn, *main_scripts;
		char *json_data, *printed_json;
		long file_size;
		size_t bytes_read;
		char gamemode_buf[256];

		/* Backup original config */
		wd_sef_wcopy("config.json", ".config.json.bak");

		config_file = fopen("config.json", "r");
		if (!config_file) {
				printf_error("Failed to open config.json");
				return -RETN;
		}

		/* Read entire file */
		fseek(config_file, 0, SEEK_END);
		file_size = ftell(config_file);
		fseek(config_file, 0, SEEK_SET);

		json_data = malloc(file_size + 1);
		if (!json_data) {
				printf_error("Memory allocation failed");
				fclose(config_file);
				return -RETN;
		}

		bytes_read = fread(json_data, 1, file_size, config_file);
		if (bytes_read != file_size) {
				printf_error("Incomplete file read (%zu of %ld bytes)", 
						     bytes_read, file_size);
				fclose(config_file);
				free(json_data);
				return -RETN;
		}
		json_data[file_size] = '\0';
		fclose(config_file);

		/* Parse and update JSON */
		root = cJSON_Parse(json_data);
		if (!root) {
				printf_error("JSON parse error: %s", cJSON_GetErrorPtr());
				free(json_data);
				return -RETN;
		}

		pawn = cJSON_GetObjectItem(root, "__pawn");
		if (!pawn) {
				printf_error("Missing '__pawn' section in config");
				cJSON_Delete(root);
				free(json_data);
				return -RETN;
		}

		/* Update main_scripts array */
		cJSON_DeleteItemFromObject(pawn, "main_scripts");
		
		main_scripts = cJSON_CreateArray();
		snprintf(gamemode_buf, sizeof(gamemode_buf), "%s", gamemode);
		cJSON_AddItemToArray(main_scripts, cJSON_CreateString(gamemode_buf));
		cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

		/* Write updated config */
		config_file = fopen("config.json", "w");
		if (!config_file) {
				printf_error("Failed to write config.json");
				cJSON_Delete(root);
				free(json_data);
				return -RETN;
		}

		printed_json = cJSON_Print(root);
		fputs(printed_json, config_file);
		
		fclose(config_file);
		free(printed_json);
		cJSON_Delete(root);
		free(json_data);

		return RETZ;
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

		/* Verify gamemode exists */
		wd_sef_fdir_reset();
		if (wd_sef_fdir(".", gamemode, NULL) != 1) {
				printf_error("Cannot locate gamemode: %s", gamemode);
				__main(0);
		}

		/* Update OpenMP configuration */
		if (update_omp_config(gamemode) != 0)
				return;

		/* Set executable permissions */
		CHMOD(server_bin, FILE_MODE);

		/* Build and execute command */
#ifdef _WIN32
		snprintf(command, sizeof(command), "./%s", server_bin);
#else
		snprintf(command, sizeof(command), "%s", server_bin);
#endif

		ret = wd_run_command(command);
		if (ret == 0) {
				SLEEP(2);
				display_server_logs();
		} else {
				printf_color(COL_RED, "Server startup failed!");
		}

		/* Restore original configuration */
		restore_omp_config();

		/* Debug mode cleanup */
		if (wcfg.serv_dbg && strcmp(wcfg.serv_dbg, "debug") == 0) {
				if (wcfg.os_type == OS_SIGNAL_WINDOWS) {
						kill_process("omp-server.exe");
				} else if (wcfg.os_type == OS_SIGNAL_LINUX) {
						kill_process("omp-server");
				}
		}
}