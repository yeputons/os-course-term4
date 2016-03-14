#include <stdarg.h>
#include "util.h"
#include "printf.h"

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
