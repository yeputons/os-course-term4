#ifndef _PRINTF_H
#define _PRINTF_H

#include <stdarg.h>

int printf(const char *format, ...);
int vprintf(const char *format, va_list args);

#endif
