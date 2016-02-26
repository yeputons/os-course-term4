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

struct interrupt_info {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsp, rbp, rsi, rdi, rdx, rcx, rbx, rax;
    uint64_t interrupt_id, error_code;
    uint64_t rip, cs, rflags;
} __attribute__((packed));

typedef void (*t_int_handler)(struct interrupt_info *info);

void set_int_handler(int intererupt_id, t_int_handler handler);

#endif /*__INTERRUPT_H__*/
