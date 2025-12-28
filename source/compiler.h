#ifndef COMPILER_H
#define COMPILER_H

#include "utils.h"

#define WATCHDOGS_COMPILER_ZERO 0
#define WATCHDOGS_COMPILER_AIO_OPTIONS 7

#ifndef WG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 900
#else
    #define WIN32_TIMEOUT 900000
#endif

typedef struct {
    char container_output[WG_PATH_MAX];
    char compiler_direct_path[WG_PATH_MAX];
    char compiler_size_file_name[WG_PATH_MAX];
    char compiler_size_input_path[WG_PATH_MAX];
    char compiler_size_temp[WG_PATH_MAX];
} io_compilers;

typedef enum {
    __FLAG_DEBUG     = 1 << 0,
    __FLAG_ASSEMBLER = 1 << 1,
    __FLAG_COMPAT    = 1 << 2,
    __FLAG_PROLIX    = 1 << 3,
    __FLAG_COMPACT   = 1 << 4
} CompilerFlags;

typedef struct {
    int flag;
    const char *option;
    size_t len;
} CompilerOption;

extern const CompilerOption object_opt[];

#define compiler_memory_clean() \
do { \
    wg_sef_fdir_memset_to_null(); \
    memset(wg_compiler_sys.container_output, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.container_output)); \
    memset(wg_compiler_sys.compiler_direct_path, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_direct_path)); \
    memset(wg_compiler_sys.compiler_size_file_name, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_file_name)); \
    memset(wg_compiler_sys.compiler_size_input_path, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_input_path)); \
    memset(wg_compiler_sys.compiler_size_temp, WATCHDOGS_COMPILER_ZERO, sizeof(wg_compiler_sys.compiler_size_temp)); \
} while(0)

/* Debug flag specific error/warning messages */
#define OPTION_FLAG_DEBUG_OUT_OF_BOUNDS    "Debug flag index %zu out of bounds"
#define OPTION_FLAG_DEBUG_NULL_FLAG        "NULL debug flag at index %zu"
#define OPTION_FLAG_DEBUG_LONG_FLAG        "Long debug flag at index %zu, length exceeds maximum"
#define OPTION_FLAG_DEBUG_EXCESSIVE_REMOVALS "Excessive flag removals for '%s', possible infinite loop"
#define OPTION_FLAG_DEBUG_BUFFER_OVERFLOW  "Buffer overflow detected during flag removal"

/* Optional: For general debug operations */
#define OPTION_FLAG_DEBUG_EMPTY_FLAG       "Empty debug flag encountered"
#define OPTION_FLAG_DEBUG_FLAG_NOT_FOUND   "Debug flag '%s' not found in configuration"

int
wg_exec_compiler(const char *arg,
				const char *compile_args,
				const char *second_arg,
				const char *four_arg,
				const char *five_arg,
				const char *six_arg,
				const char *seven_arg,
				const char *eight_arg,
				const char *nine_arg);

#endif
