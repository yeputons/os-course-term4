#include <stdarg.h>
#include "util.h"
#include "printf.h"

size_t strlen(const char *s) {
    size_t result = 0;
    while (*s) s++, result++;
    return result;
}

void memset(void* buf, char c, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ((char*)buf)[i] = c;
    }
}

void die(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    __asm__("cli");
    for (;;) {
        __asm__("hlt");
    }
}
