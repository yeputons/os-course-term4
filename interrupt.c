#include "interrupt.h"
#include "serial.h"

void int_handler_asm_0(void);
void int_handler_asm_1(void);
void int_handler_asm_2(void);
void int_handler_asm_3(void);
void int_handler_asm_4(void);
void int_handler_asm_5(void);
void int_handler_asm_6(void);
void int_handler_asm_7(void);
void int_handler_asm_8(void);
void int_handler_asm_9(void);
void int_handler_asm_10(void);
void int_handler_asm_11(void);
void int_handler_asm_12(void);
void int_handler_asm_13(void);
void int_handler_asm_14(void);
void int_handler_asm_15(void);
void int_handler_asm_16(void);
void int_handler_asm_17(void);
void int_handler_asm_18(void);
void int_handler_asm_19(void);
void int_handler_asm_20(void);

struct idt_ptr idt;

#define DESCIPTORS_COUNT 21
uint32_t int_descriptors[DESCIPTORS_COUNT][4];

typedef void (*t_handler_asm)(void);

void set_int_descriptor(int id, t_handler_asm handler_asm) {
    uint64_t offset = (uint64_t)handler_asm;
    int_descriptors[id][0] = (offset & 0xFFFF) // offset[15:0]
            | ((3 * 8) << 16);  // code segment selector
    int_descriptors[id][1] = (offset & 0xFFFF0000)   // offset[31:16] << 16
            | (1 << 15)               // present flag
            | (0 << 13)               // DPL = 0
            | (14 << 8)               // type = 0b1110, 64-bit interrupt gate
            | (0);                    // ist = 0, we assume that stack is always good, no need in IST
    int_descriptors[id][2] = (offset >> 32); // offset[63:32]
    int_descriptors[id][3] = 0; // reserved
}

void int_handler(uint64_t value) {
    putchar('i');
    putchar('n');
    putchar('t');
    putchar(' ');
    if (value >= 100) {
        putchar('0' + (value / 100) % 10);
    }
    if (value >= 10) {
        putchar('0' + (value / 10) % 10);
    }
    putchar('0' + (value / 1) % 10);
    putchar('\n');
}

void init_idt(void) {
    set_int_descriptor(0, int_handler_asm_0);
    set_int_descriptor(1, int_handler_asm_1);
    set_int_descriptor(2, int_handler_asm_2);
    set_int_descriptor(3, int_handler_asm_3);
    set_int_descriptor(4, int_handler_asm_4);
    set_int_descriptor(5, int_handler_asm_5);
    set_int_descriptor(6, int_handler_asm_6);
    set_int_descriptor(7, int_handler_asm_7);
    set_int_descriptor(8, int_handler_asm_8);
    set_int_descriptor(9, int_handler_asm_9);
    set_int_descriptor(10, int_handler_asm_10);
    set_int_descriptor(11, int_handler_asm_11);
    set_int_descriptor(12, int_handler_asm_12);
    set_int_descriptor(13, int_handler_asm_13);
    set_int_descriptor(14, int_handler_asm_14);
    set_int_descriptor(15, int_handler_asm_15);
    set_int_descriptor(16, int_handler_asm_16);
    set_int_descriptor(17, int_handler_asm_17);
    set_int_descriptor(18, int_handler_asm_18);
    set_int_descriptor(19, int_handler_asm_19);
    set_int_descriptor(20, int_handler_asm_20);
    idt.base = (uint64_t)int_descriptors;
    idt.size = sizeof(int_descriptors) - 1;
    set_idt(&idt);
}
