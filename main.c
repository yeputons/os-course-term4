#include "serial.h"
#include "interrupt.h"
#include "printf.h"
#include "pic.h"
#include "pit.h"

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

    printf("Starting PIT... ");
    set_int_handler(0x20, timer);
    init_pit(0xFF00);
    printf("OK\n");

    printf("Enabling interruptions... ");
    __asm__("sti");
    printf("OK\n");

    printf("Unmasking PIT... ");
    pic_unmask(0x20);
    printf("OK\n");
    for(;;);
}
