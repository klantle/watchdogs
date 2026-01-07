#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>

#include "utils.h"

enum {
    __compiler_rate_zero = 0,
    __compiler_rate_aio_repo = 7
};

#define COMPILER_WHITESPACE " "
#define COMPILER_PLACEHOLDER_STRING "%s"

#ifndef DOG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 0x1000
#else
    #define WIN32_TIMEOUT 0x3E8000
#endif

typedef struct {
    char container_output[DOG_PATH_MAX];
    char compiler_direct_path[DOG_PATH_MAX];
    char compiler_size_file_name[DOG_PATH_MAX];
    char compiler_size_input_path[DOG_PATH_MAX];
    char compiler_size_temp[DOG_PATH_MAX];
} io_compilers;

typedef enum {
    __FLAG_DEBUG     = 1 << 0,
    __FLAG_ASSEMBLER = 1 << 1,
    __FLAG_COMPAT    = 1 << 2,
    __FLAG_PROLIX    = 1 << 3,
    __FLAG_COMPACT   = 1 << 4,
    __FLAG_TIME      = 1 << 5
} CompilerFlags;

typedef struct {
    int flag;
    const char *option;
    size_t len;
} CompilerOption;

extern const CompilerOption object_opt[];

#define compiler_memory_clean() \
do { \
    dog_sef_restore(); \
    memset(dog_compiler_sys.container_output, __compiler_rate_zero, sizeof(dog_compiler_sys.container_output)); \
    memset(dog_compiler_sys.compiler_direct_path, __compiler_rate_zero, sizeof(dog_compiler_sys.compiler_direct_path)); \
    memset(dog_compiler_sys.compiler_size_file_name, __compiler_rate_zero, sizeof(dog_compiler_sys.compiler_size_file_name)); \
    memset(dog_compiler_sys.compiler_size_input_path, __compiler_rate_zero, sizeof(dog_compiler_sys.compiler_size_input_path)); \
    memset(dog_compiler_sys.compiler_size_temp, __compiler_rate_zero, sizeof(dog_compiler_sys.compiler_size_temp)); \
} while(0)

extern char include_aio_path[DOG_PATH_MAX * 2];
extern bool compilr_with_debugging;
extern bool compiler_debugging;
extern bool has_debug;

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
