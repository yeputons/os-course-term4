#include "ioport.h"
#include "pic.h"
#include "printf.h"

#define MASTER_COMMAND_REG  0x20
#define MASTER_STATUS_REG   0x20
#define MASTER_INT_MASK_REG 0x21
#define MASTER_DATA_REG     0x21

#define SLAVE_COMMAND_REG   0xA0
#define SLAVE_STATUS_REG    0xA0
#define SLAVE_INT_MASK_REG  0xA1
#define SLAVE_DATA_REG      0xA1

void init_pic(void) {
    out8(MASTER_COMMAND_REG, 0x11); // 0b00010001
    out8(MASTER_DATA_REG, LPIC_MASTER_ICW);
    out8(MASTER_DATA_REG, 1 << 2); // Slave is connected to 2nd input, set 2nd bit
    out8(MASTER_DATA_REG, 0x01);

    out8(SLAVE_COMMAND_REG, 0x11);
    out8(SLAVE_DATA_REG, LPIC_SLAVE_ICW);
    out8(SLAVE_DATA_REG, 2); // Slave is connected to 2nd input, send number
    out8(SLAVE_DATA_REG, 0x01);

    out8(MASTER_INT_MASK_REG, -1);
    out8(SLAVE_INT_MASK_REG, -1);
}

#define ON_MASTER(interrupt_id) ((interrupt_id) >= LPIC_MASTER_ICW && (interrupt_id) < LPIC_MASTER_ICW + 8)
#define ON_SLAVE(interrupt_id)  ((interrupt_id) >= LPIC_SLAVE_ICW  && (interrupt_id) < LPIC_SLAVE_ICW + 8)

void pic_mask(int interrupt_id) {
    if (ON_MASTER(interrupt_id)) {
        interrupt_id -= LPIC_MASTER_ICW;
        out8(MASTER_INT_MASK_REG, in8(MASTER_INT_MASK_REG) | (1 << interrupt_id));
    } else if (ON_SLAVE(interrupt_id)) {
        interrupt_id -= LPIC_SLAVE_ICW;
        out8(SLAVE_INT_MASK_REG, in8(SLAVE_INT_MASK_REG) | (1 << interrupt_id));
    } else {
        printf("Invalid call: pic_mask(%d)", interrupt_id);
    }
}

void pic_unmask(int interrupt_id) {
    if (ON_MASTER(interrupt_id)) {
        interrupt_id -= LPIC_MASTER_ICW;
        out8(MASTER_INT_MASK_REG, in8(MASTER_INT_MASK_REG) & ~(1 << interrupt_id));
    } else if (ON_SLAVE(interrupt_id)) {
        interrupt_id -= LPIC_SLAVE_ICW;
        out8(SLAVE_INT_MASK_REG, in8(SLAVE_INT_MASK_REG) & ~(1 << interrupt_id));
    } else {
        printf("Invalid call: pic_unmask(%d)", interrupt_id);
    }
}

void send_eoi(int interrupt_id) {
    if (ON_MASTER(interrupt_id)) {
        out8(MASTER_COMMAND_REG, 0x20);
    } else if (ON_SLAVE(interrupt_id)) {
        out8(MASTER_COMMAND_REG, 0x20);
        out8(SLAVE_COMMAND_REG, 0x20);
    } else {
        printf("Invalid call: send_eoi(%d)", interrupt_id);
    }
}
