#include "printf.h"
#include "serial.h"
#include "util.h"
#include "threading.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

int printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}

int vprintf(const char* format, va_list args) {
    static spin_lock_t printf_lock;

    int printed = 0;
    spin_lock(&printf_lock);
    #define addchar(c) { putchar(c); printed++; }
    #include "printf_impl.c"
    #undef addchar
    spin_unlock(&printf_lock);
    return printed;
}

int snprintf(char *outbuf, size_t outbufsize, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(outbuf, outbufsize, format, args);
    va_end(args);
    return result;
}

int vsnprintf(char *outbuf, size_t outbufsize, const char* format, va_list args) {
    if (outbufsize < 1) {
        return 0;
    }
    if (outbufsize == 1) {
        outbuf[0] = 0;
        return 0;
    }
    size_t printed = 0;
    #define addchar(c) { outbuf[printed++] = c; if (printed + 1 >= outbufsize) goto end; }
    #include "printf_impl.c"
    #undef addchar
    end:;
    outbuf[printed] = 0;
    return printed;
}
