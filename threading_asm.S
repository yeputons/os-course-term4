	.code64
	.altmacro
	.align 16
	.global yield
	.global thread_entry
	.extern switch_thread_switch
	.extern exit_unclean

yield:
	pushfq
	cli
	cld
	push %rbx
	push %rbp
	push %r12
	push %r13
	push %r14
	push %r15

	mov %rsp, %rdi
	call switch_thread_switch
	mov %rax, %rsp

	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbp
	pop %rbx
	popfq
	ret

thread_entry:
	mov %rbx, %rdi
	call *%rbp
	call exit_unclean
