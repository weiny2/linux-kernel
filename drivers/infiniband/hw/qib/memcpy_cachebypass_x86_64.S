/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
 * Copyright (c) 2006, 2007, 2008 QLogic Corporation. All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

	.text
	.p2align 4,,15
	/* rdi  destination, rsi source, rdx count */
	.globl	memcpy_cachebypass
	.type	memcpy_cachebypass, @function
# loads bypass the cache, stores fill the cache
memcpy_cachebypass:
	movq	%rdi, %rax
.L5:
	cmpq	$15, %rdx
	ja	.L34
.L3:
	cmpl	$8, %edx	/* rdx is 0..15 */
	jbe	.L9
.L6:
	testb	$8, %dxl	/* rdx is 3,5,6,7,9..15 */
	je	.L13
	movq	(%rsi), %rcx
	addq	$8, %rsi
	movq	%rcx, (%rdi)
	addq	$8, %rdi
.L13:
	testb	$4, %dxl
	je	.L15
	movl	(%rsi), %ecx
	addq	$4, %rsi
	movl	%ecx, (%rdi)
	addq	$4, %rdi
.L15:
	testb	$2, %dxl
	je	.L17
	movzwl	(%rsi), %ecx
	addq	$2, %rsi
	movw	%cx, (%rdi)
	addq	$2, %rdi
.L17:
	testb	$1, %dxl
	je	.L33
.L1:
	movzbl	(%rsi), %ecx
	movb	%cl, (%rdi)
.L33:
	ret
.L34:
	cmpq	$63, %rdx	/* rdx is > 15 */
	ja	.L64
	movl	$16, %ecx	/* rdx is 16..63 */
.L25:
	movq	8(%rsi), %r8
	movq	(%rsi), %r9
	addq	%rcx, %rsi
	movq	%r8, 8(%rdi)
	movq	%r9, (%rdi)
	addq	%rcx, %rdi
	subq	%rcx, %rdx
	cmpl	%edx, %ecx	/* is rdx >= 16? */
	jbe	.L25
	jmp	.L3		/* rdx is 0..15 */
	.p2align 4,,7
.L64:
	movl	$64, %ecx
.L42:
	prefetchnta	256(%rsi)
	movq	(%rsi), %r8
	movq	8(%rsi), %r9
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	subq	%rcx, %rdx
	movq	%r8, (%rdi)
	movq	32(%rsi), %r8
	movq	%r9, 8(%rdi)
	movq	40(%rsi), %r9
	movq	%r10, 16(%rdi)
	movq	48(%rsi), %r10
	movq	%r11, 24(%rdi)
	movq	56(%rsi), %r11
	addq	%rcx, %rsi
	movq	%r8, 32(%rdi)
	movq	%r9, 40(%rdi)
	movq	%r10, 48(%rdi)
	movq	%r11, 56(%rdi)
	addq	%rcx, %rdi
	cmpq	%rdx, %rcx	/* is rdx >= 64? */
	jbe	.L42
	/*sfence */
	orl	%edx, %edx
	je	.L33
	jmp	.L5
.L9:
	jmp	*.L12(,%rdx,8)	/* rdx is 0..8 */
	.section	.rodata
	.align 8
	.align 4
.L12:
	.quad	.L33
	.quad	.L1
	.quad	.L2
	.quad	.L6
	.quad	.L4
	.quad	.L6
	.quad	.L6
	.quad	.L6
	.quad	.L8
	.text
.L2:
	movzwl	(%rsi), %ecx
	movw	%cx, (%rdi)
	ret
.L4:
	movl	(%rsi), %ecx
	movl	%ecx, (%rdi)
	ret
.L8:
	movq	(%rsi), %rcx
	movq	%rcx, (%rdi)
	ret

		.text
	.p2align 4,,15
	/* rdi  destination, rsi source, rdx count */
	.globl	memcpy_cachebypass2
	.type	memcpy_cachebypass2, @function
# both loads and stores bypass the cache
memcpy_cachebypass2:
	movq	%rdi, %rax
.L2_5:
	cmpq	$15, %rdx
	ja	.L2_34
.L2_3:
	cmpl	$8, %edx	/* rdx is 0..15 */
	jbe	.L2_9
.L2_6:
	testb	$8, %dxl	/* rdx is 3,5,6,7,9..15 */
	je	.L2_13
	movq	(%rsi), %rcx
	addq	$8, %rsi
	movq	%rcx, (%rdi)
	addq	$8, %rdi
.L2_13:
	testb	$4, %dxl
	je	.L2_15
	movl	(%rsi), %ecx
	addq	$4, %rsi
	movl	%ecx, (%rdi)
	addq	$4, %rdi
.L2_15:
	testb	$2, %dxl
	je	.L2_17
	movzwl	(%rsi), %ecx
	addq	$2, %rsi
	movw	%cx, (%rdi)
	addq	$2, %rdi
.L2_17:
	testb	$1, %dxl
	je	.L2_33
.L2_1:
	movzbl	(%rsi), %ecx
	movb	%cl, (%rdi)
.L2_33:
	ret
.L2_34:
	cmpq	$63, %rdx	/* rdx is > 15 */
	ja	.L2_64
	movl	$16, %ecx	/* rdx is 16..63 */
.L2_25:
	movq	8(%rsi), %r8
	movq	(%rsi), %r9
	addq	%rcx, %rsi
	movq	%r8, 8(%rdi)
	movq	%r9, (%rdi)
	addq	%rcx, %rdi
	subq	%rcx, %rdx
	cmpl	%edx, %ecx	/* is rdx >= 16? */
	jbe	.L2_25
	jmp	.L2_3		/* rdx is 0..15 */
	.p2align 4,,7
.L2_64:
	movl	$64, %ecx
.L2_42:
	prefetchnta	256(%rsi)
	movq	(%rsi), %r8
	movq	8(%rsi), %r9
	movq	16(%rsi), %r10
	movq	24(%rsi), %r11
	subq	%rcx, %rdx
	movnti	%r8, (%rdi)
	movq	32(%rsi), %r8
	movnti	%r9, 8(%rdi)
	movq	40(%rsi), %r9
	movnti	%r10, 16(%rdi)
	movq	48(%rsi), %r10
	movnti	%r11, 24(%rdi)
	movq	56(%rsi), %r11
	addq	%rcx, %rsi
	movnti	%r8, 32(%rdi)
	movnti	%r9, 40(%rdi)
	movnti	%r10, 48(%rdi)
	movnti	%r11, 56(%rdi)
	addq	%rcx, %rdi
	cmpq	%rdx, %rcx	/* is rdx >= 64? */
	jbe	.L2_42
	sfence
	orl	%edx, %edx
	je	.L2_33
	jmp	.L2_5
.L2_9:
	jmp	*.L2_12(,%rdx,8)	/* rdx is 0..8 */
	.section	.rodata
	.align 8
	.align 4
.L2_12:
	.quad	.L2_33
	.quad	.L2_1
	.quad	.L2_2
	.quad	.L2_6
	.quad	.L2_4
	.quad	.L2_6
	.quad	.L2_6
	.quad	.L2_6
	.quad	.L2_8
	.text
.L2_2:
	movzwl	(%rsi), %ecx
	movw	%cx, (%rdi)
	ret
.L2_4:
	movl	(%rsi), %ecx
	movl	%ecx, (%rdi)
	ret
.L2_8:
	movq	(%rsi), %rcx
	movq	%rcx, (%rdi)
	ret
