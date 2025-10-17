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
#define PATH_SEP "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#define chmod _chmod
#else
#include <unistd.h>
#include <dirent.h>
#define PATH_SEP "/"
#endif

#include "cJSON/cJSON.h"
#include "watchdogs.h"
#include "color.h"
#include "utils.h"
#include "server.h"

void watchdogs_server_stop_tasks(void) {
        kill_process("samp-server.exe");
        kill_process("samp03svr");
        kill_process("omp-server.exe");
        kill_process("omp-server");
}

void watchdogs_server_samp(const char *gamemode_arg, const char *server_bin) {
        FILE *serv_in = NULL, *serv_out = NULL;
        int found_gamemode = 0;
        char g_line[256];

        watchdogs_sef_wcopy("server.cfg", ".server.cfg.bak");

        serv_in = fopen(".server.cfg.bak", "r");
        if (!serv_in) {
            printf_error("failed to open backup config");
            __init(0);
        }
        serv_out = fopen("server.cfg", "w");
        if (!serv_out) {
            printf_error("failed to write new config");
            fclose(serv_in);
            __init(0);
        }

        int find_gamemodes = watchdogs_sef_fdir(".", gamemode_arg);
        if (find_gamemodes != 1) {
            printf_color(COL_RED, "Can't locate: ");
            printf("%s\n", gamemode_arg);
            __init(0);
        }

        while (fgets(g_line, sizeof(g_line), serv_in)) {
            if (!strncmp(g_line, "gamemode0 ", 10)) {
                fprintf(serv_out, "gamemode0 %s\n", gamemode_arg);
                found_gamemode = 1;
            } else fputs(g_line, serv_out);
        }

        if (!found_gamemode)
            fprintf(serv_out, "gamemode0 %s\n", gamemode_arg);

        fclose(serv_in);
        fclose(serv_out);

#ifdef _WIN32
        _chmod(server_bin, _S_IREAD | _S_IWRITE);
#else
        chmod(server_bin, 0777);
#endif

        char snprintf_ptrS[256];
#ifdef _WIN32
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", server_bin);
#else
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", server_bin);
#endif

        int running_FAIL = watchdogs_sys(snprintf_ptrS);
        if (running_FAIL == 0) {
            sleep(2);
            printf_color(COL_YELLOW, "Press enter to print logs..");
            getchar();

            FILE *server_log = fopen("server_log.txt", "r");
            if (!server_log)
                printf_error("Can't found server_log.txt!");
            else {
                int ch;
                while ((ch = fgetc(server_log)) != EOF)
                    putchar(ch);
                fclose(server_log);
            }
        } else {
            printf_color(COL_RED, "running failed! ");
        }

        remove("server.cfg");
        rename(".server.cfg.bak", "server.cfg");

        if (watchdogs_config.server_or_debug &&
            !strcmp(watchdogs_config.server_or_debug, "debug"))
        {
#ifdef _WIN32
            kill_process("samp-server.exe");
            kill_process("samp03svr.exe");
#else
            kill_process("samp-server");
            kill_process("samp03svr");
#endif
        }

        __init(0);
}

void watchdogs_server_openmp(const char *gamemode_arg, const char *server_bin) {
        cJSON
            *root = NULL,
            *pawn = NULL,
            *main_scripts = NULL
        ;
        FILE *procc_f = NULL;
        char *json_data = NULL;
        char cmd_buf[256];

        watchdogs_sef_wcopy("config.json", ".config.json.bak");

        procc_f = fopen("config.json", "r");
        if (!procc_f) {
            printf_error("failed to open config.json");
            return;
        }

        int find_gamemodes = watchdogs_sef_fdir(".", gamemode_arg);
        if (find_gamemodes != 1) {
            printf_color(COL_RED, "Can't locate: ");
            printf("%s\n", gamemode_arg);
            __init(0);
        }

        fseek(procc_f, 0, SEEK_END);
        long file_len = ftell(procc_f);
        fseek(procc_f, 0, SEEK_SET);
        json_data = malloc(file_len + 1);
        size_t bytes_read = fread(json_data, 1, file_len, procc_f);
        if (bytes_read != file_len) {
            printf_error("failed to read file completely (%zu of %zu bytes)\n", bytes_read, file_len);
            fclose(procc_f);
            free(json_data);
            return;
        }

        json_data[file_len] = '\0';
        fclose(procc_f);

        root = cJSON_Parse(json_data);
        if (!root) {
            printf_error("JSON parse error: [%s]", cJSON_GetErrorPtr());
            free(json_data);
            return;
        }

        pawn = cJSON_GetObjectItem(root, "pawn");
        cJSON_DeleteItemFromObject(pawn, "main_scripts");

        main_scripts = cJSON_CreateArray();
        snprintf(cmd_buf, sizeof(cmd_buf), "%s", gamemode_arg);
        cJSON_AddItemToArray(main_scripts, cJSON_CreateString(cmd_buf));
        cJSON_AddItemToObject(pawn, "main_scripts", main_scripts);

        procc_f = fopen("config.json", "w");
        char *main_njson = cJSON_Print(root);
        fputs(main_njson, procc_f);
        free(main_njson);
        fclose(procc_f);

#ifdef _WIN32
        _chmod(server_bin, _S_IREAD | _S_IWRITE);
#else
        chmod(server_bin, 0777);
#endif

        char snprintf_ptrS[256];
#ifdef _WIN32
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", server_bin);
#else
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", server_bin);
#endif

        int running_FAIL = watchdogs_sys(snprintf_ptrS);
        if (running_FAIL == 0) {
            sleep(2);
            printf_color(COL_YELLOW, "Press enter to print logs..");
            getchar();

            FILE *server_log = fopen("log.txt", "r");
            if (!server_log)
                printf_error("Can't found log.txt!");
            else {
                int ch;
                while ((ch = fgetc(server_log)) != EOF)
                    putchar(ch);
                fclose(server_log);
            }
        } else {
            printf_color(COL_RED, "running failed! ");
        }

        remove("config.json");
        rename(".config.json.bak", "config.json");

        if (watchdogs_config.server_or_debug &&
            !strcmp(watchdogs_config.server_or_debug, "debug"))
        {
#ifdef _WIN32
            kill_process("omp-server.exe");
#else
            kill_process("omp-server");
#endif
        }

        cJSON_Delete(root);
        free(json_data);
}
