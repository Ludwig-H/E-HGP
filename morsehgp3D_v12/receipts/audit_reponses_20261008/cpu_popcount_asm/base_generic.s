	.file	"wrappers.cpp"
	.text
	.globl	__popcountdi2
	.p2align 4
	.globl	audit_popc32
	.type	audit_popc32, @function
audit_popc32:
.LFB7130:
	.cfi_startproc
	endbr64
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movl	%edi, %edi
	call	__popcountdi2@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
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
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	movq	%rdi, %rbx
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	movq	(%rdi), %rdi
	call	__popcountdi2@PLT
	movq	8(%rbx), %rdi
	movl	%eax, %ebp
	call	__popcountdi2@PLT
	movq	16(%rbx), %rdi
	addl	%eax, %ebp
	call	__popcountdi2@PLT
	movq	24(%rbx), %rdi
	addl	%eax, %ebp
	call	__popcountdi2@PLT
	addq	$8, %rsp
	.cfi_def_cfa_offset 24
	addl	%ebp, %eax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
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
