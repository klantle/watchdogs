#ifndef COMPILER_H
#define COMPILER_H

#define compiler_memory_clean() \
do { \
    wd_sef_fdir_reset(); \
    memset(container_output, WATCHDOGS_COMPILER_ZERO, sizeof(container_output)); \
    memset(compiler_direct_path, WATCHDOGS_COMPILER_ZERO, sizeof(compiler_direct_path)); \
    memset(compiler_size_file_name, WATCHDOGS_COMPILER_ZERO, sizeof(compiler_size_file_name)); \
    memset(compiler_size_input_path, WATCHDOGS_COMPILER_ZERO, sizeof(compiler_size_input_path)); \
    memset(compiler_size_temp, WATCHDOGS_COMPILER_ZERO, sizeof(compiler_size_temp)); \
} while(0)

int
wd_run_compiler(const char *arg,
				const char *compile_args,
				const char *second_arg,
				const char *four_arg,
				const char *five_arg,
				const char *six_arg,
				const char *seven_arg,
				const char *eight_arg,
				const char *nine_arg);

#endif