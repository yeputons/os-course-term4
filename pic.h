#ifndef _PIC_H
#define _PIC_H

void init_pic(void);

void send_eoi(int irq);

void pic_mask(int irq);
void pic_unmask(int irq);

#define LPIC_MASTER_ICW 0x20
#define LPIC_SLAVE_ICW  0x28

#endif

