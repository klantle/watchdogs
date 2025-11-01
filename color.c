#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "color.h"

// Borders Colors
char *BG = "\x1b[48;5;235m";
char *FG = "\x1b[97m";
char *BORD = "\x1b[33m";
char *RST = "\x1b[0m";

/**
 * println - Print formatted string with newline
 * @stream : Stream Output
 * @format: Format string
 * @...: Format arguments
 */
void println(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

/**
 * printf_color - Print colored formatted text
 * @stream : Stream Output
 * @color: Color
 * @format: Format string
 * @...: Format arguments
 */
void printf_color(FILE *stream, const char *color, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("%s", color);
		vprintf(format, args);
		printf("%s", FCOLOUR_DEFAULT);
		va_end(args);

		fflush(stream);
}


/**
 * printf_info - Print info message
 * @format: Format string
 * @...: Format arguments
 */
void printf_info(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		pr_color(stdout, FCOLOUR_YELLOW, "info: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

/**
 * printf_warning - Print warning message
 * @format: Format string
 * @...: Format arguments
 */
void printf_warning(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		pr_color(stdout, FCOLOUR_GREEN, "warning: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

/**
 * printf_error - Print error message
 * @format: Format string
 * @...: Format arguments
 */
void printf_error(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		pr_color(stdout, FCOLOUR_RED, "error: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

/**
 * printf_crit - Print critical error message
 * @format: Format string
 * @...: Format arguments
 */
void printf_crit(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		pr_color(stdout, FCOLOUR_RED, "crit: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}
