    .code64
    .altmacro
    .align 16
    .global int_handlers_asm
    .extern int_handler

.macro REP from, to, action
    .if \to-\from == 1
    action \from
    .else
    // Avoiding "macros nested too deeply" error
    REP \from, %((\from+\to)/2), action
    REP %((\from+\to)/2), \to, action
    .endif
.endm

.macro DEF_INT_HANDLER id
    .global int_handler_asm_\id
    int_handler_asm_\id:
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rdi
    push %rsi
    push %rbp
    push %rsp
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    mov $\id, %rdi
    call int_handler
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rsp
    pop %rbp
    pop %rsi
    pop %rdi
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    iretq
.endm

    REP 0, 256, DEF_INT_HANDLER

    .data

.macro STORE_INT_HANDLER id
    .quad int_handler_asm_\id
.endm

int_handlers_asm:
    REP 0, 256, STORE_INT_HANDLER
