#ifndef _PIC_H
#define _PIC_H

void init_pic(void);

void send_eoi(int intererupt_id);

void pic_mask(int intererupt_id);
void pic_unmask(int intererupt_id);

#define LPIC_MASTER_ICW 0x20
#define LPIC_SLAVE_ICW  0x28

#endif

