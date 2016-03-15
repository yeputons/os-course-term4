#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>

size_t strlen(const char *s);
void memset(void* buf, char c, size_t len);

void die(const char *format, ...);
#define assert(cond) if (!(cond)) { die("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); }

#endif
