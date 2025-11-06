#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "wd_extra.h"

char *BG = "\x1b[48;5;235m";
char *FG = "\x1b[97m";
char *BORD = "\x1b[33m";
char *RST = "\x1b[0m";

void println(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

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

void printf_info(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[INFO]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_warning(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[WARNING]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_error(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[ERROR]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}

void printf_crit(FILE *stream, const char *format, ...)
{
		va_list args;

		va_start(args, format);
		printf("[CRIT]: ");
		vprintf(format, args);
		printf("\n");
		va_end(args);

		fflush(stream);
}
