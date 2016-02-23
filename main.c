#include "serial.h"
#include "interrupt.h"

void main(void) {
    __asm__("cli");
    init_serial();
    puts("Initialized serial");

    puts("Loading IDT...");
    init_idt();
    puts("Done");

    puts("Testing int 2, 3, 4, 5...");
    __asm__("int $2");
    __asm__("int $3");
    __asm__("int $4");
    __asm__("int $5");
    puts("Done");
}
