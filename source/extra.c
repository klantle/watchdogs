static const char *description =
"Utility module providing compiler error analysis, cross-platform file statistics," "\n"
"portable I/O functions, and debugging output utilities with comprehensive"         "\n"
"error pattern matching and detailed compilation diagnostics."
;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>
#include "utils.h"

#ifdef WG_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <sys/types.h>
    #include <sys/stat.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#include "crypto.h"
#include "debug.h"
#include "extra.h"

char
        *BG = "\x1b[48;5;235m";
char
        *FG = "\x1b[97m";
char
        *BORD = "\x1b[33m";
char
        *RST = "\x1b[0m";

/*
 * Prints formatted output followed by a newline character to specified stream.
 * Wrapper around vprintf that automatically appends newline and flushes stream.
 * Uses variable argument lists to support printf-style formatting.
 */
void println(FILE *stream, const char *format, ...)
{
        va_list args;  /* Variable argument list handler */
        va_start(args, format);  /* Initialize argument list */
        vfprintf(stream, format, args);   /* Print formatted string with variable arguments */
        printf("\n");            /* Append newline character */
        va_end(args);            /* Clean up argument list */
        fflush(stream);          /* Ensure output is written immediately */
}

/*
 * Prints colored formatted output to specified stream, then resets to default color.
 * Applies ANSI color code before printing and resets to default color afterwards.
 * Supports variable arguments like printf for flexible formatting.
 */
void printf_colour(FILE *stream, const char *color, const char *format, ...)
{
        va_list args;
        va_start(args, format);
        printf("%s", color);     /* Apply ANSI color escape sequence */
        vfprintf(stream, format, args);   /* Print formatted content with variable arguments */
        printf("%s", FCOLOUR_DEFAULT);  /* Reset to default terminal color */
        va_end(args);
        fflush(stream);          /* Force immediate output */
}

/*
 * Prints informational message with "I" prefix and newline.
 * Standardized format for informational messages with automatic newline.
 */
void printf_info(FILE *stream, const char *format, ...)
{
        va_list args;
        va_start(args, format);
        printf("%s", FCOLOUR_YELLOW);
        printf(">> I");       /* Standard informational prefix */
        printf("%s", FCOLOUR_DEFAULT);
        printf(": ");
        vfprintf(stream, format, args);   /* Print message content */
        printf("\n");            /* Ensure line break */
        va_end(args);
        fflush(stream);
}

/*
 * Prints warning message with "W" prefix and newline.
 * Standardized format for warning messages with automatic newline.
 */
void printf_warning(FILE *stream, const char *format, ...)
{
        va_list args;
        va_start(args, format);
        printf("%s", FCOLOUR_GREEN);
        printf(">> W");       /* Standard informational prefix */
        printf("%s", FCOLOUR_DEFAULT);
        printf(": ");
        vfprintf(stream, format, args);   /* Print message content */
        printf("\n");            /* Ensure line break */
        va_end(args);
        fflush(stream);
}

/*
 * Prints error message with "E" prefix and newline.
 * Standardized format for error messages with automatic newline.
 */
void printf_error(FILE *stream, const char *format, ...)
{
        va_list args;
        va_start(args, format);
        printf("%s", FCOLOUR_RED);
        printf(">> E");       /* Standard informational prefix */
        printf("%s", FCOLOUR_DEFAULT);
        printf(": ");
        vfprintf(stream, format, args);   /* Print message content */
        printf("\n");            /* Ensure line break */
        va_end(args);
        fflush(stream);
}

#ifdef WG_WINDOWS
/*
 * Converts Windows FILETIME structure to Unix time_t (seconds since epoch).
 * FILETIME represents 100-nanosecond intervals since January 1, 1601 (UTC).
 * Adjusts for epoch difference and converts to seconds.
 */
static time_t filetime_to_time_t(const FILETIME *ft) {
        const uint64_t EPOCH_DIFF = 116444736000000000ULL;  /* Seconds between 1601 and 1970 epochs */
        uint64_t v = ((uint64_t)ft->dwHighDateTime << 32) | ft->dwLowDateTime;  /* Combine 64-bit value */
        if (v < EPOCH_DIFF) return (time_t)0;  /* Handle pre-epoch timestamps */
        return (time_t)((v - EPOCH_DIFF) / 10000000ULL);  /* Convert to seconds (100ns -> seconds) */
}
#endif

/*
 * Cross-platform file statistics function that works on both Windows and Unix-like systems.
 * Retrieves file metadata including size, timestamps, permissions, and identifiers.
 * Returns 0 on success, -1 on error with errno set appropriately.
 */
int portable_stat(const char *path, portable_stat_t *out) {
        if (!path || !out) return -1;  /* Validate input parameters */
        memset(out, 0, sizeof(*out));  /* Initialize output structure to zero */

#ifdef WG_WINDOWS
        /* Windows implementation using Win32 API */
        wchar_t wpath[WG_MAX_PATH];
        int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);  /* Calculate required buffer size */
        
        /* Convert path to wide characters, fall back to ANSI if UTF-8 conversion fails */
        if (len == 0 || len > WG_MAX_PATH) {
                if (!MultiByteToWideChar(CP_ACP, 0, path, -1, wpath, WG_MAX_PATH)) return -1;  /* ANSI fallback */
        } else {
                MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, WG_MAX_PATH);  /* UTF-8 conversion */
        }

        WIN32_FILE_ATTRIBUTE_DATA fad;
        if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &fad)) {
                return -1;  /* Failed to get file attributes */
        }

        /* Combine high and low parts to get 64-bit file size */
        uint64_t size = ((uint64_t)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        out->st_size = size;

        /* Convert Windows FILETIME timestamps to Unix time_t */
        out->st_latime = filetime_to_time_t(&fad.ftLastAccessTime);   /* Last access time */
        out->st_lmtime = filetime_to_time_t(&fad.ftLastWriteTime);    /* Last modification time */
        out->st_mctime = filetime_to_time_t(&fad.ftCreationTime);     /* Creation/change time */

        DWORD attrs = fad.dwFileAttributes;
        /* Set file type flags based on Windows attributes */
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                out->st_mode |= S_IFDIR;  /* Directory flag */
        } else {
                out->st_mode |= S_IFREG;  /* Regular file flag */
        }

        /* Set permission flags based on read-only attribute */
        if (attrs & FILE_ATTRIBUTE_READONLY) {
                out->st_mode |= S_IRUSR;  /* Read-only: only read permission */
        } else {
                out->st_mode |= (S_IRUSR | S_IWUSR);  /* Read-write: both permissions */
        }

        /* Set execute permission for known executable file extensions */
        const char *extension = strrchr(path, '.');
        if (extension && (_stricmp(extension, ".exe") == 0 ||
                    _stricmp(extension, ".bat") == 0 ||
                    _stricmp(extension, ".com") == 0)) {
                out->st_mode |= S_IXUSR;  /* Execute permission */
        }

        out->st_ino = 0;  /* Windows doesn't have inode numbers */
        out->st_dev = 0;  /* Windows doesn't have device IDs in the same way */

        return 0;  /* Success */
#else
        /* Unix/Linux/macOS implementation using standard stat() */
        struct stat st;
        if (stat(path, &st) != 0) return -1;  /* Standard stat call failed */
        
        /* Map standard stat structure to portable structure */
        out->st_size = (uint64_t)st.st_size;     /* File size in bytes */
        out->st_ino  = (uint64_t)st.st_ino;      /* Inode number */
        out->st_dev  = (uint64_t)st.st_dev;      /* Device ID */
        out->st_mode = (unsigned int)st.st_mode; /* Permission and type bits */
        out->st_latime = st.st_atime;            /* Last access time */
        out->st_lmtime = st.st_mtime;  /* Last modification time */
        out->st_mctime = st.st_ctime;  /* Last status change time */

        return 0;  /* Success */
#endif
}
