#include "serial.h"
#include "interrupt.h"
#include "printf.h"
#include "pic.h"
#include "util.h"
#include "memory.h"
#include "threading.h"

int timer_step;

void timer(struct interrupt_info *info) {
    send_eoi(info->interrupt_id);
    printf("Timer! step=%d\n", timer_step++);
}

void fault_handler(struct interrupt_info *info) {
	(void)info;
	__asm__("cli");
	printf("FAIL interrupt_id=%d, error_code=%d\n", info->interrupt_id, info->error_code);
	printf("rip=0x%p, cs=%d, rflags=0x%08llx\n", info->rip, info->cs, info->rflags);
	#define dump(reg) printf("  %3s=0x%016llx\n", #reg, info->reg)
	dump(rax); dump(rbx); dump(rcx); dump(rdx);
	dump(rdi); dump(rsi); dump(rbp); dump(rsp);
	dump( r8); dump( r9); dump(r10); dump(r11);
	dump(r12); dump(r13); dump(r14); dump(r15);
	die("");
}

void thread_1(void* arg) {
    for (int i = 0;; i++) {
        printf("thread_1(%d, %p)\n", i, arg);
        yield();
    }
}

void thread_2(void *arg) {
    long long count = (long long)arg;
    for (int i = 0; i < count; i++) {
        printf("thread_2(%d, %p)\n", i, arg);
        yield();
    }
}

void main(void) {
    __asm__("cli");
    init_serial();
    puts("Initialized serial");

    printf("Loading IDT... ");
    init_idt();
    printf("OK\n");

    set_int_handler(13, fault_handler); // General protection fault
    set_int_handler(14, fault_handler); // Page fault

    printf("Starting PIC... ");
    init_pic();
    printf("OK\n");

    printf("Enabling interruptions... ");
    __asm__("sti");
    printf("OK\n");

    init_memory();

    init_threading();
    create_thread(thread_1, (void*)0x1234);
    create_thread(thread_2, (void*)300);
    for (int i = 0;; i++) {
        printf("main thread %d\n", i);
        yield();
    }
}
