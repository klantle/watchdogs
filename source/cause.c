/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#include  "utils.h"
#include  "units.h"
#include  "crypto.h"
#include  "debug.h"
#include  "replicate.h"
#include  "cause.h"

extern causeExplanation ccs[];

static const char *dog_find_warn_err(const char *line)
{
    if (!line || !*line)
        return (NULL);

    size_t line_len = strlen(line);
    if (line_len == 0 || line_len > DOG_MAX_PATH)
        return (NULL);

    for (int cindex = 0; ccs[cindex].cs_t != NULL; ++cindex) {
        if (!ccs[cindex].cs_t || !ccs[cindex].cs_i)
            continue;

        const char *found = strstr(line, ccs[cindex].cs_t);
        if (found) {
            size_t pattern_len = strlen(ccs[cindex].cs_t);
            if ((size_t)(found - line) + pattern_len <= line_len)
                return (ccs[cindex].cs_i);
        }
    }
    return (NULL);
}

static void compiler_detailed(const char *dog_output, int debug,
                              int warning_count, int error_count, const char *compiler_ver,
                              int header_size, int code_size, int data_size,
                              int stack_size, int total_size)
{
    char outbuf[DOG_MAX_PATH];
    int len;

    if (error_count < 1) {
        len = snprintf(outbuf, sizeof(outbuf),
                       "Compilation Complete - OK! | " DOG_COL_CYAN "%d pass (warning) " DOG_COL_DEFAULT
                       "| " DOG_COL_BLUE "%d fail (error)\n",
                       warning_count, error_count);
    } else {
        len = snprintf(outbuf, sizeof(outbuf),
                       "Compilation Complete - Fail :( | " DOG_COL_CYAN "%d pass (warning) " DOG_COL_DEFAULT
                       "| " DOG_COL_BLUE "%d fail (error)\n",
                       warning_count, error_count);
    }

    if (len > 0)
        printf("%.*s", len, outbuf);

    printf("-----------------------------\n");

    if (path_exists(dog_output) && debug && error_count < 1 && header_size >= 1 && total_size >= 1) {
        __set_default_access(dog_output);
        unsigned long hash = crypto_djb2_hash_file(dog_output);

        len = snprintf(outbuf, sizeof(outbuf),
                       "Output: %s\nHeader : %dB  |  Total        : %dB\n"
                       "Code (static mem)   : %dB  |  hash (djb2)  : %#lx\n"
                       "Data (static mem)   : %dB\nStack (automatic)   : %dB\n",
                       dog_output,
                       header_size,
                       total_size,
                       code_size,
                       hash,
                       data_size,
                       stack_size);
        if (len > 0)
            printf("%.*s", len, outbuf);

        dog_portable_stat_t st;
        if (dog_portable_stat(dog_output, &st) == 0) {
            len = snprintf(outbuf, sizeof(outbuf),
                           "ino    : %llu   |  file   : %lluB\n"
                           "dev    : %llu\n"
                           "read   : %s   |  write  : %s\n"
                           "execute: %s   |  mode   : %020o\n"
                           "atime  : %llu\nmtime  : %llu\nctime  : %llu\n",
                           (unsigned long long)st.st_ino,
                           (unsigned long long)st.st_size,
                           (unsigned long long)st.st_dev,
                           (st.st_mode & S_IRUSR) ? "Y" : "N",
                           (st.st_mode & S_IWUSR) ? "Y" : "N",
                           (st.st_mode & S_IXUSR) ? "Y" : "N",
                           st.st_mode,
                           (unsigned long long)st.st_latime,
                           (unsigned long long)st.st_lmtime,
                           (unsigned long long)st.st_mctime);
            if (len > 0)
                printf("%.*s", len, outbuf);
        }
    }

    printf("\n");

    len = snprintf(outbuf, sizeof(outbuf),
                   "** Pawn Compiler %s - Copyright (c) 1997-2006, ITB CompuPhase\n",
                   compiler_ver);
    if (len > 0)
        printf("%.*s", len, outbuf);

    print_restore_color();
}

void cause_compiler_expl(const char *log_file, const char *dog_output, int debug)
{
    minimal_debugging();

    FILE *_log_file = fopen(log_file, "r");
    if (!_log_file)
        return;

    long warning_count = 0, error_count = 0;
    int header_size = 0, code_size = 0, data_size = 0, stack_size = 0, total_size = 0;
    char compiler_line[DOG_MORE_MAX_PATH] = {0}, compiler_ver[64] = {0};

    while (fgets(compiler_line, sizeof(compiler_line), _log_file)) {

        if (dog_strcase(compiler_line, "Warnings.") ||
            dog_strcase(compiler_line, "Warning.") ||
            dog_strcase(compiler_line, "Errors.") ||
            dog_strcase(compiler_line, "Error."))
            continue;

        if (dog_strcase(compiler_line, "Header size:")) {
            header_size = strtol(strchr(compiler_line, ':') + 1, NULL, 10);
            continue;
        } else if (dog_strcase(compiler_line, "Code size:")) {
            code_size = strtol(strchr(compiler_line, ':') + 1, NULL, 10);
            continue;
        } else if (dog_strcase(compiler_line, "Data size:")) {
            data_size = strtol(strchr(compiler_line, ':') + 1, NULL, 10);
            continue;
        } else if (dog_strcase(compiler_line, "Stack/heap size:")) {
            stack_size = strtol(strchr(compiler_line, ':') + 1, NULL, 10);
            continue;
        } else if (dog_strcase(compiler_line, "Total requirements:")) {
            total_size = strtol(strchr(compiler_line, ':') + 1, NULL, 10);
            continue;
        } else if (dog_strcase(compiler_line, "Pawn Compiler ")) {
            char *p = strstr(compiler_line, " ");
            while (*p && !isdigit(*p)) p++;
            if (*p)
                sscanf(p, "%63s", compiler_ver);
            continue;
        }

        printf(DOG_COL_BWHITE "%s" DOG_COL_DEFAULT, compiler_line);
        fflush(stdout);

        if (dog_strcase(compiler_line, "warning"))
            ++warning_count;
        if (dog_strcase(compiler_line, "error"))
            ++error_count;

        const char *description = dog_find_warn_err(compiler_line);
        if (description) {
            const char *found = NULL;
            int column = 0;
            for (int i = 0; ccs[i].cs_t; ++i) {
                if ((found = strstr(compiler_line, ccs[i].cs_t))) {
                    column = found - compiler_line;
                    break;
                }
            }
            for (int i = 0; i < column; ++i)
                putchar(' ');

#ifdef DOG_LINUX
            if (strfind(description, "file doesn't exist, insufficient permissions", true) == 1) {
                pr_color(stdout, DOG_COL_CYAN, "^ %s" DOG_COL_YELLOW " See " DOG_COL_CYAN
                                               ".watchdogs/help.txt | cat .watchdogs/help.txt\n",
                         description);
                if (path_exists(".watchdogs/help.txt") == 1)
                    remove(".watchdogs/help.txt");
                FILE *help = fopen(".watchdogs/help.txt", "w");
                if (help) {
                    fprintf(help, HELP_PICK1);
                    fprintf(help, HELP_PICK2);
                    fprintf(help, HELP_PICK3);
                    fprintf(help, HELP_PICK4);
                    fprintf(help, HELP_PICK5);
                    fprintf(help, HELP_PICK6);
                    fprintf(help, HELP_PICK8);
                    fprintf(help, HELP_PICK9);
                    fprintf(help, HELP_PICK01);
                    fprintf(help, HELP_PICK02);
                    fclose(help);
                }
                continue;
            }
#endif
            pr_color(stdout, DOG_COL_CYAN, "^ %s \n", description);
        }
    }

    fclose(_log_file);
    compiler_detailed(dog_output, debug, warning_count, error_count,
                      compiler_ver, header_size, code_size,
                      data_size, stack_size, total_size);
}

causeExplanation ccs[] =
{
/* 001 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000001,
    COMPILER_DT_SEL0000001
},

/* 002 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000002,
    COMPILER_DT_SEL0000002
},

/* 003 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000003,
    COMPILER_DT_SEL0000003
},

/* 012 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000012,
    COMPILER_DT_SEL0000012
},

/* 014 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000014,
    COMPILER_DT_SEL0000014
},

/* 015 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000015,
    COMPILER_DT_SEL0000015
},

/* 016 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000016,
    COMPILER_DT_SEL0000016
},

/* 019 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000019,
    COMPILER_DT_SEL0000019
},

/* 020 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000020,
    COMPILER_DT_SEL0000020
},

/* 036 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000036,
    COMPILER_DT_SEL0000036
},

/* 037 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000037,
    COMPILER_DT_SEL0000037
},

/* 030 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000030,
    COMPILER_DT_SEL0000030
},

/* 027 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000027,
    COMPILER_DT_SEL0000027
},

/* 026 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000026,
    COMPILER_DT_SEL0000026
},

/* 028 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000028,
    COMPILER_DT_SEL0000028
},

/* 054 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000054,
    COMPILER_DT_SEL0000054
},

/* 004 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000004,
    COMPILER_DT_SEL0000004
},

/* 005 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000005,
    COMPILER_DT_SEL0000005
},

/* 006 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000006,
    COMPILER_DT_SEL0000006
},

/* 007 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000007,
    COMPILER_DT_SEL0000007
},

/* 008 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000008,
    COMPILER_DT_SEL0000008
},

/* 009 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000009,
    COMPILER_DT_SEL0000009
},

/* 017 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000017,
    COMPILER_DT_SEL0000017
},

/* 018 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000018,
    COMPILER_DT_SEL0000018
},

/* 022 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000022,
    COMPILER_DT_SEL0000022
},

/* 023 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000023,
    COMPILER_DT_SEL0000023
},

/* 024 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000024,
    COMPILER_DT_SEL0000024
},

/* 025 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000025,
    COMPILER_DT_SEL0000025
},

/* 027 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK027,
    COMPILER_DT_SEL0000027_2
},

/* 029 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000029,
    COMPILER_DT_SEL0000029
},

/* 032 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000032,
    COMPILER_DT_SEL0000032
},

/* 045 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000045,
    COMPILER_DT_SEL0000045
},

/* 203 */  /* WARNING */
{
    COMPILER_DT_PICK000203,
    COMPILER_DT_SEL0000203
},

/* 204 */  /* WARNING */
{
    COMPILER_DT_PICK000204,
    COMPILER_DT_SEL0000204
},

/* 205 */  /* WARNING */
{
    COMPILER_DT_PICK000205,
    COMPILER_DT_SEL0000205
},

/* 209 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000209,
    COMPILER_DT_SEL0000209
},

/* 211 */  /* WARNING */
{
    COMPILER_DT_PICK000211,
    COMPILER_DT_SEL0000211
},

/* 010 */  /* SYNTAX ERROR */
{
    COMPILER_DT_PICK000010,
    COMPILER_DT_SEL0000010
},

/* 213 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000213,
    COMPILER_DT_SEL0000213
},

/* 215 */  /* WARNING */
{
    COMPILER_DT_PICK000215,
    COMPILER_DT_SEL0000215
},

/* 217 */  /* WARNING */
{
    COMPILER_DT_PICK000217,
    COMPILER_DT_SEL0000217
},

/* 234 */  /* WARNING */
{
    COMPILER_DT_PICK000234,
    COMPILER_DT_SEL0000234
},

/* 013 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000013,
    COMPILER_DT_SEL0000013
},

/* 021 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000021,
    COMPILER_DT_SEL0000021
},

/* 028 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK028,
    COMPILER_DT_SEL0000028_2
},

/* 033 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000033,
    COMPILER_DT_SEL0000033
},

/* 034 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000034,
    COMPILER_DT_SEL0000034
},

/* 035 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000035,
    COMPILER_DT_SEL0000035
},

/* 037 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK037,
    COMPILER_DT_SEL0000037_2
},

/* 039 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000039,
    COMPILER_DT_SEL0000039
},

/* 040 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000040,
    COMPILER_DT_SEL0000040
},

/* 041 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000041,
    COMPILER_DT_SEL0000041
},

/* 042 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000042,
    COMPILER_DT_SEL0000042
},

/* 043 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000043,
    COMPILER_DT_SEL0000043
},

/* 044 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000044,
    COMPILER_DT_SEL0000044
},

/* 046 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000046,
    COMPILER_DT_SEL0000046
},

/* 047 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000047,
    COMPILER_DT_SEL0000047
},

/* 048 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000048,
    COMPILER_DT_SEL0000048
},

/* 049 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000049,
    COMPILER_DT_SEL0000049
},

/* 050 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000050,
    COMPILER_DT_SEL0000050
},

/* 055 */  /* SEMANTIC ERROR */
{
    COMPILER_DT_PICK000055,
    COMPILER_DT_SEL0000055
},

/* 100 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000100,
    COMPILER_DT_SEL0000100
},

/* 101 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000101,
    COMPILER_DT_SEL0000101
},

/* 102 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000102,
    COMPILER_DT_SEL0000102
},

/* 103 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000103,
    COMPILER_DT_SEL0000103
},

/* 104 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000104,
    COMPILER_DT_SEL0000104
},

/* 105 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000105,
    COMPILER_DT_SEL0000105
},

/* 107 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000107,
    COMPILER_DT_SEL0000107
},

/* 108 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000108,
    COMPILER_DT_SEL0000108
},

/* 109 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000109,
    COMPILER_DT_SEL0000109
},

/* 110 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000110,
    COMPILER_DT_SEL0000110
},

/* 111 */  /* FATAL ERROR */
{
    COMPILER_DT_PICK000111,
    COMPILER_DT_SEL0000111
},

/* 214 */  /* WARNING */
{
    COMPILER_DT_PICK214,
    COMPILER_DT_SEL0000214
},

/* 200 */  /* WARNING */
{
    COMPILER_DT_PICK000200,
    COMPILER_DT_SEL0000200
},

/* 201 */  /* WARNING */
{
    COMPILER_DT_PICK000201,
    COMPILER_DT_SEL0000201
},

/* 202 */  /* WARNING */
{
    COMPILER_DT_PICK000202,
    COMPILER_DT_SEL0000202
},

/* 206 */  /* WARNING */
{
    COMPILER_DT_PICK000206,
    COMPILER_DT_SEL0000206
},

/* 214 */  /* WARNING */
{
    COMPILER_DT_PICK214_2,
    COMPILER_DT_SEL0000214_2
},

/* 060 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000060,
    COMPILER_DT_SEL0000060
},

/* 061 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000061,
    COMPILER_DT_SEL0000061
},

/* 062 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000062,
    COMPILER_DT_SEL0000062
},

/* 068 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000068,
    COMPILER_DT_SEL0000068
},

/* 069 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000069,
    COMPILER_DT_SEL0000069
},

/* 070 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000070,
    COMPILER_DT_SEL0000070
},

/* 071 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000071,
    COMPILER_DT_SEL0000071
},

/* 072 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK000072,
    COMPILER_DT_SEL0000072
},

/* 038 */  /* PREPROCESSOR ERROR */
{
    COMPILER_DT_PICK038,
    COMPILER_DT_SEL0000038
},

/* Sentinel value to mark the end of the array. */
{NULL, NULL}
};
