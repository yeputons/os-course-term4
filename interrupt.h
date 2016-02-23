#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>

struct idt_ptr {
	uint16_t size;
	uint64_t base;
} __attribute__((packed));

static inline void set_idt(const struct idt_ptr *ptr)
{ __asm__ volatile ("lidt (%0)" : : "a"(ptr)); }

void init_idt(void);

typedef void (*t_int_handler)(int irq);

void set_int_handler(int irq, t_int_handler handler);

#endif /*__INTERRUPT_H__*/
