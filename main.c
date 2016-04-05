#include "serial.h"
#include "interrupt.h"
#include "printf.h"
#include "pic.h"
#include "pit.h"
#include "util.h"
#include "memory.h"
#include "threading.h"

void timer(struct interrupt_info *info) {
    send_eoi(info->interrupt_id);
    yield();
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

void work(void) {
    for (volatile int i = 0; i < (int)(1e5); i++);
}

volatile bool thread_1_stop;

void thread_1(void* arg) {
    for (int i = 0; !thread_1_stop; i++) {
        printf("thread_1(%d, %p)\n", i, arg);
        work();
    }
    thread_exit();
}

void thread_2(void *arg) {
    long long count = (long long)arg;
    for (int i = 0; i < count; i++) {
        printf("thread_2(%d, %p)\n", i, arg);
        work();
    }
    thread_exit();
}

void thread_chain(void *arg) {
    long long depth = (long long)arg;
    printf("chain(%lld)\n", depth);
    for (int i = 0; i < 50; i++) {
        work();
    }
    if (depth > 0) {
        wait(create_thread(thread_chain, (void*)(depth - 1)));
    }
    for (int i = 0; i < 50; i++) {
        work();
    }
    thread_exit();
}

bool sleeper_stop;
void thread_sleeper(void *arg) {
    (void)arg;
    printf("sleeper started\n");
    while (!sleeper_stop) {
        sleep();
        printf("Woken up!\n");
    }
    printf("sleeper stopped\n");
    thread_exit();
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

    printf("Initializing threading... \n");
    init_threading();
    printf("OK\n");

    printf("Starting PIT for preemption... ");
    set_int_handler(PIT_INTERRUPT, timer);
    init_pit(1000); // every 1ms
    pic_unmask(PIT_INTERRUPT);
    printf("OK\n");

    thread_t th1 = create_thread(thread_1, (void*)0x1234);
    thread_t th2 = create_thread(thread_2, (void*)100);
    for (int i = 0; get_thread_state(th2) == THREAD_RUNNING; i++) {
        printf("main thread %d\n", i);
        work();
    }
    wait(th2);
    printf("Thread2 is terminated\n");
    thread_1_stop = true;
    wait(th1);
    printf("Thread1 is terminated\n");

    printf("Starting chain\n");
    wait(create_thread(thread_chain, (void*)20));

    printf("Starting sleeper");
    thread_t sleeper = create_thread(thread_sleeper, NULL);
    for (int i = 0; i < 10; i++) {
        printf("sending to sleeper %d\n", i);
        wake(sleeper);
        yield();
    }
    sleeper_stop = true;
    wake(sleeper);
    wait(sleeper);
    thread_exit();
}
