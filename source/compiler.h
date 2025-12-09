#ifndef COMPILER_H
#define COMPILER_H

#include "utils.h"

#define WATCHDOGS_COMPILER_ZERO 0

typedef struct {
    char container_output[WG_PATH_MAX];
    char compiler_direct_path[WG_PATH_MAX];
    char compiler_size_file_name[WG_PATH_MAX];
    char compiler_size_input_path[WG_PATH_MAX];
    char compiler_size_temp[WG_PATH_MAX];
} io_compilers;

#define compiler_memory_clean() \
do { \
    wg_sef_fdir_memset_to_null(); \
    memset(wg_compiler_sys.container_output, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.container_output)); \
    memset(wg_compiler_sys.compiler_direct_path, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_direct_path)); \
    memset(wg_compiler_sys.compiler_size_file_name, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_file_name)); \
    memset(wg_compiler_sys.compiler_size_input_path, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_input_path)); \
    memset(wg_compiler_sys.compiler_size_temp, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_temp)); \
} while(0)

int
wg_run_compiler(const char *arg,
				const char *compile_args,
				const char *second_arg,
				const char *four_arg,
				const char *five_arg,
				const char *six_arg,
				const char *seven_arg,
				const char *eight_arg,
				const char *nine_arg);

#endif