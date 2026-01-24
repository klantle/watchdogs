/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "utils.h"

#ifndef DOG_WINDOWS
extern char **environ;
#endif

typedef struct {
    char *container_output                     ;
    char compiler_direct_path    [DOG_PATH_MAX];
    char compiler_size_file_name [DOG_PATH_MAX];
    char compiler_size_input_path[DOG_PATH_MAX];
    char compiler_size_temp      [DOG_PATH_MAX];
} io_compilers;

typedef enum {
    BIT_FLAG_DEBUG     = 1 << 0,
    BIT_FLAG_ASSEMBLER = 1 << 1,
    BIT_FLAG_COMPAT    = 1 << 2,
    BIT_FLAG_PROLIX    = 1 << 3,
    BIT_FLAG_COMPACT   = 1 << 4,
    BIT_FLAG_TIME      = 1 << 5
} CompilerFlags;

typedef struct {
    int flag;
    const char *option;
    size_t len;
} CompilerOption;

typedef struct {
    const char *full_name;
    const char *short_name;
    bool *flag_ptr;
} OptionMap;

extern const CompilerOption object_opt[];

#ifdef DOG_WINDOWS

/* Thread data structure for _beginthreadex */
typedef struct {
	char *compiler_input;
	STARTUPINFO *startup_info;
	PROCESS_INFORMATION *process_info;
	HANDLE hFile;
	struct timespec *pre_start;
	struct timespec *post_end;
	const char *windows_redist_err;
	const char *windows_redist_err2;
} compiler_thread_data_t;

#endif

extern bool compiler_is_err;
extern bool compiler_installing_stdlib;
extern char *compiler_full_includes;
extern bool compiler_have_debug_flag;
extern bool compiler_dog_flag_debug;

int
dog_exec_compiler(const char *arg,
				const char *compile_args,
				const char *second_arg,
				const char *four_arg,
				const char *five_arg,
				const char *six_arg,
				const char *seven_arg,
				const char *eight_arg,
				const char *nine_arg,
                const char *ten_arg);

#endif
