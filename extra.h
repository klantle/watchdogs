#ifndef EXTRA_H
#define EXTRA_H

#define pr_color printf_color
#define pr_succes printf_succes
#define pr_info printf_info
#define pr_warning printf_warning
#define pr_error printf_error
#define pr_crit printf_crit

void wd_apply_pawncc(void);

void println(FILE *stream, const char* fmt, ...);
void printf_color(FILE *stream, const char *color, const char *format, ...);
void printf_succes(FILE *stream, const char *format, ...);
void printf_info(FILE *stream, const char *format, ...);
void printf_warning(FILE *stream, const char *format, ...);
void printf_error(FILE *stream, const char *format, ...);
void printf_crit(FILE *stream, const char *format, ...);

#endif