    .code64
    .global int_handler_asm
    .extern int_handler

.macro int_handler_asm name, id
    .global \name
    mov $\id, %rdi
    call int_handler
    iretq
.endm

#define h(id) int_handler_asm_##id: int_handler_asm int_handler_asm_##id id
h(0)
h(1)
h(2)
h(3)
h(4)
h(5)
h(6)
h(7)
h(8)
h(9)
h(10)
h(11)
h(12)
h(13)
h(14)
h(15)
h(16)
h(17)
h(18)
h(19)
h(20)
