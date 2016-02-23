#ifndef _PRINTF_H
#define _PRINTF_H

#include <stdarg.h>
#include <stddef.h>

int printf(const char *format, ...);
int vprintf(const char *format, va_list args);
int snprintf(char *outbuf, size_t outbufsize, const char* format, ...);
int vsnprintf(char *outbuf, size_t outbufsize, const char* format, va_list args);

#endif
