#include "serial.h"
#include "interrupt.h"
#include "printf.h"
#include "pic.h"
#include "util.h"
#include "memory.h"

int timer_step;

void timer(struct interrupt_info *info) {
    send_eoi(info->interrupt_id);
    printf("Timer! step=%d\n", timer_step++);
}

void main(void) {
    __asm__("cli");
    init_serial();
    puts("Initialized serial");

    printf("Loading IDT... ");
    init_idt();
    printf("OK\n");

    printf("Starting PIC... ");
    init_pic();
    printf("OK\n");

    printf("Enabling interruptions... ");
    __asm__("sti");
    printf("OK\n");

    init_memory();
    for(;;);
}
