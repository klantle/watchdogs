#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>

#include "utils.h"

enum {
    __compiler_rate_zero = 0,
    __compiler_rate_aio_repo = 7
};

#define LINUX_LIB_PATH "/usr/local/lib"
#define LINUX_LIB32_PATH "/usr/local/lib32"
#define TMUX_LIB_PATH "/data/data/com.termux/files/usr/lib"
#define TMUX_LIB_LOC_PATH "/data/data/com.termux/files/usr/local/lib"
#define TMUX_LIB_ARM64_PATH "/data/data/com.termux/arm64/usr/lib"
#define TMUX_LIB_ARM32_PATH "/data/data/com.termux/arm32/usr/lib"
#define TMUX_LIB_AMD64_PATH "/data/data/com.termux/amd64/usr/lib"
#define TMUX_LIB_AMD32_PATH "/data/data/com.termux/amd32/usr/lib"

#define COMPILER_WHITESPACE " "
#define COMPILER_PLACEHOLDER_STRING "%s"

#ifndef DOG_WINDOWS
    extern char **environ;
    #define POSIX_TIMEOUT 0x1000
#else
    #define WIN32_TIMEOUT 0x3E8000
#endif

typedef struct {
    char container_output        [DOG_PATH_MAX];
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

extern const CompilerOption object_opt[];

extern char all_include_paths[DOG_PATH_MAX * 2];
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
