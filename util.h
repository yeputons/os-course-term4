#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdint.h>

size_t strlen(const char *s);
void memset(void* buf, char c, size_t len);

void die(const char *format, ...);
#define assert(cond) if (!(cond)) { die("Assertion failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); }

uint64_t get_rflags();
void set_rflags(uint64_t rflags);

#endif
