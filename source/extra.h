#ifndef EXTRA_H
#define EXTRA_H

#include <stdint.h>
#include <signal.h>
#include <time.h>

// Foreground Colors (Standard)
#define DOG_COL_BLACK      "\033[0;30m"
#define DOG_COL_RED        "\033[0;31m"
#define DOG_COL_GREEN      "\033[0;32m"
#define DOG_COL_YELLOW     "\033[0;33m"
#define DOG_COL_BLUE       "\033[94m"
#define DOG_COL_MAGENTA    "\033[0;35m"
#define DOG_COL_CYAN       "\033[0;36m"
#define DOG_COL_WHITE      "\033[0;37m"

// Foreground Colors (Bright)
#define DOG_COL_BBLACK     "\033[1;30m"
#define DOG_COL_BRED       "\033[1;31m"
#define DOG_COL_BGREEN     "\033[1;32m"
#define DOG_COL_BYELLOW    "\033[1;33m"
#define DOG_COL_BBLUE      "\033[1;34m"
#define DOG_COL_BMAGENTA   "\033[1;35m"
#define DOG_COL_BCYAN      "\033[1;36m"
#define DOG_COL_BWHITE     "\033[1;37m"
#define DOG_COL_B_MAGENTA  "\033[1;35m"
#define DOG_COL_B_BLUE     "\033[1;94m"

// Background Colors
#define BKG_BLACK      "\033[40m"
#define BKG_RED        "\033[41m"
#define BKG_GREEN      "\033[42m"
#define BKG_YELLOW     "\033[43m"
#define BKG_BLUE       "\033[44m"
#define BKG_MAGENTA    "\033[45m"
#define BKG_CYAN       "\033[46m"
#define BKG_WHITE      "\033[47m"

// Background Colors (Bright)
#define BKG_BBLACK     "\033[100m"
#define BKG_BRED       "\033[101m"
#define BKG_BGREEN     "\033[102m"
#define BKG_BYELLOW    "\033[103m"
#define BKG_BBLUE      "\033[104m"
#define BKG_BMAGENTA   "\033[105m"
#define BKG_BCYAN      "\033[106m"
#define BKG_BWHITE     "\033[107m"

// Styles
#define DOG_COL_BOLD       "\033[1m"
#define DOG_COL_DIM        "\033[2m"
#define DOG_COL_UNDERLINE  "\033[4m"
#define DOG_COL_BLINK      "\033[5m"
#define DOG_COL_REVERSE    "\033[7m"
#define DOG_COL_HIDDEN     "\033[8m"

// Reset / Default
#define DOG_COL_RESET      "\033[0m"
#define DOG_COL_DEFAULT    "\033[39m"
#define BKG_DEFAULT    "\033[49m"

/* Mode bits we will populate (subset of POSIX) */
#ifndef S_IFREG
  #define S_IFREG 0100000
#endif
#ifndef S_IFDIR
  #define S_IFDIR 0040000
#endif
#ifndef S_IFLNK
  #define S_IFLNK 0120000
#endif

#ifndef S_IRUSR
  #define S_IRUSR 0400
  #define S_IWUSR 0200
  #define S_IXUSR 0100
#endif

/* Portable structure: fields commonly useful */
typedef struct {
        uint64_t st_size;     /* file size in bytes */
        uint64_t st_ino;      /* inode (0 if not available) */
        uint64_t st_dev;      /* device id (0 if not available) */
        unsigned int st_mode; /* file type & permission bits (emulated on Windows) */
        time_t st_latime;     /* last access time */
        time_t st_lmtime;     /* last modification time */
        time_t st_mctime;     /* metadata change time (POSIX) or creation/metadata time on Windows */
} portable_stat_t;

int portable_stat(const char *path, portable_stat_t *out);
int portable_stat(const char *path, portable_stat_t *out);

#define pr_color printf_colour
#define pr_info printf_info
#define pr_warning printf_warning
#define pr_error printf_error

void println(FILE *stream, const char* format, ...);
void printf_colour(FILE *stream, const char *color, const char *format, ...);
void printf_info(FILE *stream, const char *format, ...);
void printf_warning(FILE *stream, const char *format, ...);
void printf_error(FILE *stream, const char *format, ...);

#endif
