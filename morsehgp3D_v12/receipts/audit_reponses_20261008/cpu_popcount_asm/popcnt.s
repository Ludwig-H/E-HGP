	.file	"wrappers.cpp"
	.text
	.p2align 4
	.globl	audit_popc32
	.type	audit_popc32, @function
audit_popc32:
.LFB7130:
	.cfi_startproc
	endbr64
	xorl	%eax, %eax
	popcntl	%edi, %eax
	ret
	.cfi_endproc
.LFE7130:
	.size	audit_popc32, .-audit_popc32
	.p2align 4
	.globl	audit_popc256
	.type	audit_popc256, @function
audit_popc256:
.LFB7131:
	.cfi_startproc
	endbr64
	xorl	%edx, %edx
	xorl	%eax, %eax
	popcntq	(%rdi), %rdx
	popcntq	8(%rdi), %rax
	addl	%edx, %eax
	xorl	%edx, %edx
	popcntq	16(%rdi), %rdx
	addl	%edx, %eax
	xorl	%edx, %edx
	popcntq	24(%rdi), %rdx
	addl	%edx, %eax
	ret
	.cfi_endproc
.LFE7131:
	.size	audit_popc256, .-audit_popc256
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
