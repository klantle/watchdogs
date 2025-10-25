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
#define __PATH_SYM "\\"
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
#define chmod _chmod
#else
#include <unistd.h>
#include <dirent.h>
#define __PATH_SYM "/"
#endif

#include "cJSON/cJSON.h"
#include "chain.h"
#include "color.h"
#include "extra.h"
#include "utils.h"
#include "server.h"

void wd_StopServerTasks(void) {
        kill_process_safe("samp-server.exe");
        kill_process_safe("samp03svr");
        kill_process_safe("omp-server.exe");
        kill_process_safe("omp-server");
}

void wd_RunSAMPServer(const char *gamemode_arg, const char *server_bin) {
        FILE *serv_in = NULL,
             *serv_out = NULL;
        int fo_gm = 0;
        char g_line[256];

        wd_sef_wcopy("server.cfg", ".server.cfg.bak");
        serv_in = fopen(".server.cfg.bak", "r");
        if (!serv_in) {
            printf_error("failed to open backup config");
            __main(0);
        }
        serv_out = fopen("server.cfg", "w");
        if (!serv_out) {
            printf_error("failed to write new config");
            fclose(serv_in);
            __main(0);
        }

        wd_sef_fdir_reset();
        
        int __fi_gm = wd_sef_fdir(".", gamemode_arg, NULL);
        if (__fi_gm != 1) {
            printf_error("Can't locate:");
            printf("\t%s\n", gamemode_arg);
            __main(0);
        }

        while (fgets(g_line, sizeof(g_line), serv_in)) {
            if (!strncmp(g_line, "gamemode0 ", 10)) {
                fprintf(serv_out, "gamemode0 %s\n", gamemode_arg);
                fo_gm = 1;
            } else fputs(g_line, serv_out);
        }

        if (!fo_gm)
            fprintf(serv_out, "gamemode0 %s\n", gamemode_arg);

        if (serv_in)
            fclose(serv_in);
        if (serv_out)
            fclose(serv_out);

#ifdef _WIN32
        _chmod(server_bin, _S_IREAD | _S_IWRITE);
#else
        chmod(server_bin, 0777);
#endif

        char snprintf_ptrS[256];
#ifdef _WIN32
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", server_bin);
#else
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", server_bin);
#endif

        int running_FAIL = wd_RunCommand(snprintf_ptrS);
        if (running_FAIL == 0) {
            sleep(2);
#ifndef _WIN32
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
#endif
        } else {
            printf_color(COL_RED, "running failed!\n");
        }

        remove("server.cfg");
        rename(".server.cfg.bak", "server.cfg");

        if (wcfg.serv_dbg != NULL &&
            !strcmp(wcfg.serv_dbg, "debug")) {
            if (wcfg.__os__ == 0x01) {
                kill_process("samp-server.exe");
            }
            else if (wcfg.__os__ == 0x00) {
                kill_process("samp03svr");
            }
        }

        __main(0);
}

void wd_RunOMPServer(const char *gamemode_arg, const char *server_bin) {
        cJSON
            *__main_scripts = NULL,
            *__root = NULL,
            *__pawn = NULL
        ;
        FILE *procc_f = NULL;
        char *json_data = NULL,
             cmd_buf[256];

        wd_sef_wcopy("config.json", ".config.json.bak");
        procc_f = fopen("config.json", "r");
        if (!procc_f) {
            printf_error("failed to open config.json");
            return;
        }

        wd_sef_fdir_reset();

        int __fi_gm = wd_sef_fdir(".", gamemode_arg, NULL);
        if (__fi_gm != 1) {
            printf_color(COL_RED, "Can't locate: ");
            printf("%s\n", gamemode_arg);
            __main(0);
        }

        fseek(procc_f, 0, SEEK_END);
        long file_len = ftell(procc_f);
        fseek(procc_f, 0, SEEK_SET);
        json_data = malloc(file_len + 1);
        size_t bytes_read = fread(json_data, 1, file_len, procc_f);
        if (bytes_read != file_len) {
#if defined(_DBG_PRINT)
            printf_error("failed to read file completely (%zu of %zu bytes)", bytes_read, file_len);
#endif
            fclose(procc_f);
            free(json_data);
            return;
        }

        json_data[file_len] = '\0';

        if (procc_f)
            fclose(procc_f);

        __root = cJSON_Parse(json_data);
        if (!__root) {
            printf_error("JSON parse error: [%s]", cJSON_GetErrorPtr());
            free(json_data);
            return;
        }

        __pawn = cJSON_GetObjectItem(__root, "__pawn");
        cJSON_DeleteItemFromObject(__pawn, "main_scripts");

        __main_scripts = cJSON_CreateArray();
        snprintf(cmd_buf, sizeof(cmd_buf), "%s", gamemode_arg);
        cJSON_AddItemToArray(__main_scripts, cJSON_CreateString(cmd_buf));
        cJSON_AddItemToObject(__pawn, "main_scripts", __main_scripts);

        procc_f = fopen("config.json", "w");
        char *main_njson = cJSON_Print(__root);
        fputs(main_njson, procc_f);
        free(main_njson);

        if (procc_f)
            fclose(procc_f);

#ifdef _WIN32
        _chmod(server_bin, _S_IREAD | _S_IWRITE);
#else
        chmod(server_bin, 0777);
#endif

        char snprintf_ptrS[256];
#ifdef _WIN32
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "./%s", server_bin);
#else
        snprintf(snprintf_ptrS, sizeof(snprintf_ptrS), "%s", server_bin);
#endif

        int running_FAIL = wd_RunCommand(snprintf_ptrS);
        if (running_FAIL == 0) {
            sleep(2);
#ifndef _WIN32
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
#endif
        } else {
            printf_color(COL_RED, "running failed!\n");
        }

        remove("config.json");
        rename(".config.json.bak", "config.json");

        if (wcfg.serv_dbg != NULL &&
            !strcmp(wcfg.serv_dbg, "debug")) {
            if (wcfg.__os__ == 0x01) {
                kill_process("omp-server.exe");
            }
            else if (wcfg.__os__ == 0x00) {
                kill_process("omp-server");
            }
        }

        cJSON_Delete(__root);
        free(json_data);
}
