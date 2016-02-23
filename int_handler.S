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
    mov $\id, %rdi
    call int_handler
    iretq
.endm

    REP 0, 256, DEF_INT_HANDLER

    .data

.macro STORE_INT_HANDLER id
    .quad int_handler_asm_\id
.endm

int_handlers_asm:
    REP 0, 256, STORE_INT_HANDLER
