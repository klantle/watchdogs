#ifndef EXTRA_H
#define EXTRA_H

void println(FILE *stream, const char* fmt, ...);
void printf_color(FILE *stream, const char *color, const char *format, ...);
void printf_succes(FILE *stream, const char *format, ...);
void printf_info(FILE *stream, const char *format, ...);
void printf_warning(FILE *stream, const char *format, ...);
void printf_error(FILE *stream, const char *format, ...);
void printf_crit(FILE *stream, const char *format, ...);

void wd_apply_pawncc(void);

#endif