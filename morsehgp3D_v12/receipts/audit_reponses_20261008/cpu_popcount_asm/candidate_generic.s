	.file	"wrappers.cpp"
	.text
	.p2align 4
	.globl	audit_popc32
	.type	audit_popc32, @function
audit_popc32:
.LFB7130:
	.cfi_startproc
	endbr64
	movl	%edi, %eax
	shrl	%eax
	andl	$1431655765, %eax
	subl	%eax, %edi
	movl	%edi, %edx
	shrl	$2, %edi
	andl	$858993459, %edx
	andl	$858993459, %edi
	addl	%edi, %edx
	movl	%edx, %eax
	shrl	$4, %eax
	addl	%edx, %eax
	andl	$252645135, %eax
	imull	$16843009, %eax, %eax
	shrl	$24, %eax
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
	movdqu	(%rdi), %xmm1
	movdqa	.LC0(%rip), %xmm3
	movdqu	16(%rdi), %xmm0
	movdqa	%xmm1, %xmm2
	psrlq	$1, %xmm2
	pand	%xmm3, %xmm2
	psubq	%xmm2, %xmm1
	movdqa	%xmm0, %xmm2
	psrlq	$1, %xmm2
	pand	%xmm3, %xmm2
	movdqa	%xmm1, %xmm3
	psubq	%xmm2, %xmm0
	movdqa	.LC1(%rip), %xmm2
	psrlq	$2, %xmm3
	pand	%xmm2, %xmm1
	pand	%xmm2, %xmm3
	paddq	%xmm1, %xmm3
	movdqa	%xmm0, %xmm1
	pand	%xmm2, %xmm0
	psrlq	$2, %xmm1
	pand	%xmm2, %xmm1
	movdqa	%xmm3, %xmm2
	psrlq	$4, %xmm2
	paddq	%xmm0, %xmm1
	paddq	%xmm3, %xmm2
	movdqa	.LC2(%rip), %xmm3
	pand	%xmm3, %xmm2
	movdqa	%xmm2, %xmm0
	psllq	$8, %xmm0
	paddq	%xmm2, %xmm0
	movdqa	%xmm0, %xmm2
	psllq	$16, %xmm2
	paddq	%xmm2, %xmm0
	movdqa	%xmm0, %xmm2
	psllq	$32, %xmm2
	paddq	%xmm2, %xmm0
	movdqa	%xmm1, %xmm2
	psrlq	$4, %xmm2
	psrlq	$56, %xmm0
	paddq	%xmm1, %xmm2
	pand	%xmm3, %xmm2
	movdqa	%xmm2, %xmm1
	psllq	$8, %xmm1
	paddq	%xmm2, %xmm1
	movdqa	%xmm1, %xmm2
	psllq	$16, %xmm2
	paddq	%xmm2, %xmm1
	movdqa	%xmm1, %xmm2
	psllq	$32, %xmm2
	paddq	%xmm2, %xmm1
	psrlq	$56, %xmm1
	shufps	$136, %xmm1, %xmm0
	movdqa	%xmm0, %xmm1
	psrldq	$8, %xmm1
	paddd	%xmm1, %xmm0
	movdqa	%xmm0, %xmm1
	psrldq	$4, %xmm1
	paddd	%xmm1, %xmm0
	movd	%xmm0, %eax
	ret
	.cfi_endproc
.LFE7131:
	.size	audit_popc256, .-audit_popc256
	.section	.rodata.cst16,"aM",@progbits,16
	.align 16
.LC0:
	.quad	6148914691236517205
	.quad	6148914691236517205
	.align 16
.LC1:
	.quad	3689348814741910323
	.quad	3689348814741910323
	.align 16
.LC2:
	.quad	1085102592571150095
	.quad	1085102592571150095
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
