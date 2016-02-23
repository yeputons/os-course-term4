#include "interrupt.h"
#include "serial.h"
#include "printf.h"
#include "memory.h"

typedef void (*t_handler_asm)(void);

struct idt_ptr idt;

#define INTS_COUNT 256
extern t_handler_asm int_handlers_asm[INTS_COUNT];
uint32_t int_descriptors[INTS_COUNT][4];

void set_int_descriptor(int id, t_handler_asm handler_asm) {
    uint64_t offset = (uint64_t)handler_asm;
    if (offset == 0) {
        int_descriptors[id][0] =
        int_descriptors[id][1] =
        int_descriptors[id][2] =
        int_descriptors[id][3] = 0;
        return;
    }

    int_descriptors[id][0]
        = (offset & 0xFFFF)    // offset[15:0]
        | (KERNEL_CODE << 16); // code segment selector
    int_descriptors[id][1]
        = (offset & 0xFFFF0000) // offset[31:16] << 16
        | (1 << 15)             // present flag
        | (0 << 13)             // DPL = 0
        | (14 << 8)             // type = 0b1110, 64-bit interrupt gate
        | (0);                  // ist = 0, we assume that stack is always good, no need in IST
    int_descriptors[id][2] = (offset >> 32); // offset[63:32]
    int_descriptors[id][3] = 0; // reserved
}

t_int_handler handlers[INTS_COUNT];

void int_handler(uint64_t value) {
    if (!handlers[value]) {
        printf("unexpected int_handler(%lld)\n", value);
    } else {
        handlers[value](value);
    }
}

void set_int_handler(int irq, t_int_handler handler) {
    handlers[irq] = handler;
}

void init_idt(void) {
    for (int i = 0; i < INTS_COUNT; i++) {
        set_int_descriptor(i, int_handlers_asm[i]);
    }
    idt.base = (uint64_t)int_descriptors;
    idt.size = sizeof(int_descriptors) - 1;
    set_idt(&idt);
}
