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

int_handler_common:
	// assumes that error code and interrupt id are already pushed
	// interrupt id is on the top
	push %rsp
	push %rax
	push %rbx
	push %rcx
	push %rdx
	push %rdi
	push %rsi
	push %rbp
	push %r8
	push %r9
	push %r10
	push %r11
	push %r12
	push %r13
	push %r14
	push %r15
	mov %rsp, %rdi
	cld
	call int_handler
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %r11
	pop %r10
	pop %r9
	pop %r8
	pop %rbp
	pop %rsi
	pop %rdi
	pop %rdx
	pop %rcx
	pop %rbx
	pop %rax
	pop %rsp
	add $16, %rsp // get rid of error code and interrupt it
	iretq

.macro DEF_INT_HANDLER_NOERR id
	int_handler_asm_\id:
	pushq $0 // fake error code
	pushq $\id
	jmp int_handler_common
.endm
.macro DEF_INT_HANDLER_ERR id
	int_handler_asm_\id:
	pushq $\id
	jmp int_handler_common
.endm

	REP  0,   8, DEF_INT_HANDLER_NOERR
	DEF_INT_HANDLER_ERR   8
	DEF_INT_HANDLER_NOERR 9
	REP 10,  15, DEF_INT_HANDLER_ERR
	DEF_INT_HANDLER_NOERR 15
	DEF_INT_HANDLER_NOERR 16
	DEF_INT_HANDLER_ERR   17
	REP 18, 256, DEF_INT_HANDLER_NOERR

	.data

.macro STORE_INT_HANDLER id
	.quad int_handler_asm_\id
.endm

int_handlers_asm:
	REP 0, 256, STORE_INT_HANDLER
