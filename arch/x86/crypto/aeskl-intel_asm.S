/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Implement AES algorithm in Intel AES KeyLocker instructions.
 *
 * Most codes are based from AES NI implementation, aesni-intel_asm.S
 *
 */

#include <linux/linkage.h>
#include <asm/inst.h>
#include <asm/frame.h>

#define STATE1	%xmm0
#define STATE2	%xmm1
#define STATE3	%xmm2
#define STATE4	%xmm3
#define STATE5	%xmm4
#define STATE6	%xmm5
#define STATE7	%xmm6
#define STATE8	%xmm7
#define STATE	STATE1

#ifdef __x86_64__
#define IN1	%xmm8
#define IN2	%xmm9
#define IN3	%xmm10
#define IN4	%xmm11
#define IN5	%xmm12
#define IN6	%xmm13
#define IN7	%xmm14
#define IN8	%xmm15
#define IN	IN1
#else
#define IN	%xmm1
#endif

#ifdef __x86_64__
#define AREG	%rax
#define HANDLEP	%rdi
#define OUTP	%rsi
#define KLEN	%r9d
#define INP	%rdx
#define T1	%r10
#define LEN	%rcx
#define IVP	%r8
#else
#define AREG	%eax
#define HANDLEP	%edi
#define OUTP	AREG
#define KLEN	%ebx
#define INP	%edx
#define T1    %ecx
#define LEN %esi
#define IVP %ebp
#endif

#define UKEYP OUTP

/*
 * int __aeskl_setkey(struct crypto_aes_ctx *ctx,
 *		      const u8 *in_key,
 *		      unsigned int key_len)
 */
SYM_FUNC_START(__aeskl_setkey)
	FRAME_BEGIN
#ifndef __x86_64__
	push HANDLEP
	movl (FRAME_OFFSET+8)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+12)(%esp), UKEYP	# in_key
	movl (FRAME_OFFSET+16)(%esp), %edx	# key_len
#endif
	movl %edx, 480(HANDLEP)
	movdqu (UKEYP), STATE1
	mov $1, %eax
	cmp $16, %dl
	je .Lsetkey_128

	movdqu 0x10(UKEYP), STATE2
	ENCODEKEY256 %eax, %eax
	movdqu STATE4, 0x30(HANDLEP)
	jmp .Lsetkey_end
.Lsetkey_128:
	ENCODEKEY128 %eax, %eax

.Lsetkey_end:
	movdqu STATE1, (HANDLEP)
	movdqu STATE2, 0x10(HANDLEP)
	movdqu STATE3, 0x20(HANDLEP)

	xor AREG, AREG
#ifndef __x86_64__
	popl HANDLEP
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_setkey)

/*
 * int __aeskl_enc1(const void *ctx,
 *		    u8 *dst,
 *		    const u8 *src)
 */
SYM_FUNC_START(__aeskl_enc1)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+12)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+16)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+20)(%esp), INP	# src
#endif
	movdqu (INP), STATE
	movl 480(HANDLEP), KLEN

	cmp $16, KLEN
	je .Lenc_128
	AESENC256KL HANDLEP, STATE
	jz .Lenc_err
	jmp .Lenc_noerr
.Lenc_128:
	AESENC128KL HANDLEP, STATE
	jz .Lenc_err

.Lenc_noerr:
	xor AREG, AREG
	jmp .Lenc_end
.Lenc_err:
	mov $1, AREG
.Lenc_end:
	movdqu STATE, (OUTP)
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_enc1)


/*
 * int __aeskl_dec1(const void *ctx,
 *		    u8 *dst,
 *		    const u8 *src)
 */
SYM_FUNC_START(__aeskl_dec1)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+12)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+16)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+20)(%esp), INP	# src
#endif
	movdqu (INP), STATE
	mov 480(HANDLEP), KLEN

	cmp $16, KLEN
	je .Ldec_128
	AESDEC256KL HANDLEP, STATE
	jz .Ldec_err
	jmp .Ldec_noerr
.Ldec_128:
	AESDEC128KL HANDLEP, STATE
	jz .Ldec_err

.Ldec_noerr:
	xor AREG, AREG
	jmp .Ldec_end
.Ldec_err:
	mov $1, AREG
.Ldec_end:
	movdqu STATE, (OUTP)
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_dec1)


/*
 * int __aeskl_ecb_enc(struct crypto_aes_ctx *ctx,
 *		       const u8 *dst,
 *		       u8 *src,
 *		       size_t len)
 */
SYM_FUNC_START(__aeskl_ecb_enc)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl LEN
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+16)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+20)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+24)(%esp), INP	# src
	movl (FRAME_OFFSET+28)(%esp), LEN	# len
#endif
	test LEN, LEN
	jz .Lecb_enc_noerr
	mov 480(HANDLEP), KLEN
	cmp $16, LEN
	jb .Lecb_enc_noerr
	cmp $128, LEN
	jb .Lecb_enc1

.align 4
.Lecb_enc8:
	movdqu (INP), STATE1
	movdqu 0x10(INP), STATE2
	movdqu 0x20(INP), STATE3
	movdqu 0x30(INP), STATE4
	movdqu 0x40(INP), STATE5
	movdqu 0x50(INP), STATE6
	movdqu 0x60(INP), STATE7
	movdqu 0x70(INP), STATE8

	cmp $16, KLEN
	je .Lecb_enc8_128
	AESENCWIDE256KL HANDLEP
	jz .Lecb_enc_err
	jmp .Lecb_enc8_end
.Lecb_enc8_128:
	AESENCWIDE128KL HANDLEP
	jz .Lecb_enc_err

.Lecb_enc8_end:
	movdqu STATE1, (OUTP)
	movdqu STATE2, 0x10(OUTP)
	movdqu STATE3, 0x20(OUTP)
	movdqu STATE4, 0x30(OUTP)
	movdqu STATE5, 0x40(OUTP)
	movdqu STATE6, 0x50(OUTP)
	movdqu STATE7, 0x60(OUTP)
	movdqu STATE8, 0x70(OUTP)

	sub $128, LEN
	add $128, INP
	add $128, OUTP
	cmp $128, LEN
	jge .Lecb_enc8
	cmp $16, LEN
	jb .Lecb_enc_noerr

.align 4
.Lecb_enc1:
	movdqu (INP), STATE1
	cmp $16, KLEN
	je .Lecb_enc1_128
	AESENC256KL HANDLEP, STATE
	jz .Lecb_enc_err
	jmp .Lecb_enc1_end
.Lecb_enc1_128:
	AESENC128KL HANDLEP, STATE
	jz .Lecb_enc_err

.Lecb_enc1_end:
	movdqu STATE1, (OUTP)
	sub $16, LEN
	add $16, INP
	add $16, OUTP
	cmp $16, LEN
	jge .Lecb_enc1

.Lecb_enc_noerr:
	xor AREG, AREG
	jmp .Lecb_enc_end
.Lecb_enc_err:
	mov $1, AREG
.Lecb_enc_end:
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
	popl LEN
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_ecb_enc)

/*
 * int __aeskl_ecb_dec(struct crypto_aes_ctx *ctx,
 *		       const u8 *dst,
 *		       u8 *src,
 *		       size_t len);
 */
SYM_FUNC_START(__aeskl_ecb_dec)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl LEN
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+16)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+20)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+24)(%esp), INP	# src
	movl (FRAME_OFFSET+28)(%esp), LEN	# len
#endif

	test LEN, LEN
	jz .Lecb_dec_noerr
	mov 480(HANDLEP), KLEN
	cmp $16, LEN
	jb .Lecb_dec_noerr
	cmp $128, LEN
	jb .Lecb_dec1

.align 4
.Lecb_dec8:
	movdqu (INP), STATE1
	movdqu 0x10(INP), STATE2
	movdqu 0x20(INP), STATE3
	movdqu 0x30(INP), STATE4
	movdqu 0x40(INP), STATE5
	movdqu 0x50(INP), STATE6
	movdqu 0x60(INP), STATE7
	movdqu 0x70(INP), STATE8

	cmp $16, KLEN
	je .Lecb_dec8_128
	AESDECWIDE256KL HANDLEP
	jz .Lecb_dec_err
	jmp .Lecb_dec8_end
.Lecb_dec8_128:
	AESDECWIDE128KL HANDLEP
	jz .Lecb_dec_err

.Lecb_dec8_end:
	movdqu STATE1, (OUTP)
	movdqu STATE2, 0x10(OUTP)
	movdqu STATE3, 0x20(OUTP)
	movdqu STATE4, 0x30(OUTP)
	movdqu STATE5, 0x40(OUTP)
	movdqu STATE6, 0x50(OUTP)
	movdqu STATE7, 0x60(OUTP)
	movdqu STATE8, 0x70(OUTP)

	sub $128, LEN
	add $128, INP
	add $128, OUTP
	cmp $128, LEN
	jge .Lecb_dec8
	cmp $16, LEN
	jb .Lecb_dec_noerr

.align 4
.Lecb_dec1:
	movdqu (INP), STATE1
	cmp $16, KLEN
	je .Lecb_dec1_128
	AESDEC256KL HANDLEP, STATE
	jz .Lecb_dec_err
	jmp .Lecb_dec1_end
.Lecb_dec1_128:
	AESDEC128KL HANDLEP, STATE
	jz .Lecb_dec_err

.Lecb_dec1_end:
	movdqu STATE1, (OUTP)
	sub $16, LEN
	add $16, INP
	add $16, OUTP
	cmp $16, LEN
	jge .Lecb_dec1

.Lecb_dec_noerr:
	xor AREG, AREG
	jmp .Lecb_dec_end
.Lecb_dec_err:
	mov $1, AREG
.Lecb_dec_end:
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
	popl LEN
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_ecb_dec)



/*
 * int __aeskl_cbc_enc(struct crypto_aes_ctx *ctx,
 *		       const u8 *dst,
 *		       u8 *src,
 *		       size_t len,
 *		       u8 *iv)
 */
SYM_FUNC_START(__aeskl_cbc_enc)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl IVP
	pushl LEN
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+20)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+24)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+28)(%esp), INP	# src
	movl (FRAME_OFFSET+32)(%esp), LEN	# len
	movl (FRAME_OFFSET+36)(%esp), IVP	# iv
#endif

	cmp $16, LEN
	jb .Lcbc_enc_noerr
	mov 480(HANDLEP), KLEN
	movdqu (IVP), STATE

.align 4
.Lcbc_enc1:
	movdqu (INP), IN
	pxor IN, STATE

	cmp $16, KLEN
	je .Lcbc_enc1_128
	AESENC256KL HANDLEP, STATE
	jz .Lcbc_enc_err
	jmp .Lcbc_enc1_end
.Lcbc_enc1_128:
	AESENC128KL HANDLEP, STATE
	jz .Lcbc_enc_err

.Lcbc_enc1_end:
	movdqu STATE, (OUTP)
	sub $16, LEN
	add $16, INP
	add $16, OUTP
	cmp $16, LEN
	jge .Lcbc_enc1
	movdqu STATE, (IVP)

.Lcbc_enc_noerr:
	xor AREG, AREG
	jmp .Lcbc_enc_end
.Lcbc_enc_err:
	mov $1, AREG
.Lcbc_enc_end:
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
	popl LEN
	popl IVP
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_cbc_enc)

/*
 * int __aeskl_cbc_dec(struct crypto_aes_ctx *ctx,
 *		       const u8 *dst,
 *		       u8 *src,
 *		       size_t len,
 *		       u8 *iv)
 */
SYM_FUNC_START(__aeskl_cbc_dec)
	FRAME_BEGIN
#ifndef __x86_64__
	pushl IVP
	pushl LEN
	pushl HANDLEP
	pushl KLEN
	movl (FRAME_OFFSET+20)(%esp), HANDLEP	# ctx
	movl (FRAME_OFFSET+24)(%esp), OUTP	# dst
	movl (FRAME_OFFSET+28)(%esp), INP	# src
	movl (FRAME_OFFSET+32)(%esp), LEN	# len
	movl (FRAME_OFFSET+36)(%esp), IVP	# iv
#endif

	cmp $16, LEN
	jb .Lcbc_dec_noerr
	mov 480(HANDLEP), KLEN
#ifdef __x86_64__
	cmp $128, LEN
	jb .Lcbc_dec1_pre

.align 4
.Lcbc_dec8:
	movdqu 0x0(INP), STATE1
	movdqu 0x10(INP), STATE2
	movdqu 0x20(INP), STATE3
	movdqu 0x30(INP), STATE4
	movdqu 0x40(INP), STATE5
	movdqu 0x50(INP), STATE6
	movdqu 0x60(INP), STATE7
	movdqu 0x70(INP), STATE8

	movdqu (IVP), IN1
	movdqa STATE1, IN2
	movdqa STATE2, IN3
	movdqa STATE3, IN4
	movdqa STATE4, IN5
	movdqa STATE5, IN6
	movdqa STATE6, IN7
	movdqa STATE7, IN8
	movdqu STATE8, (IVP)

	cmp $16, KLEN
	je .Lcbc_dec8_128
	AESDECWIDE256KL HANDLEP
	jz .Lcbc_dec_err
	jmp .Lcbc_dec8_end
.Lcbc_dec8_128:
	AESDECWIDE128KL HANDLEP
	jz .Lcbc_dec_err

.Lcbc_dec8_end:
	pxor IN1, STATE1
	pxor IN2, STATE2
	pxor IN3, STATE3
	pxor IN4, STATE4
	pxor IN5, STATE5
	pxor IN6, STATE6
	pxor IN7, STATE7
	pxor IN8, STATE8

	movdqu STATE1, 0x0(OUTP)
	movdqu STATE2, 0x10(OUTP)
	movdqu STATE3, 0x20(OUTP)
	movdqu STATE4, 0x30(OUTP)
	movdqu STATE5, 0x40(OUTP)
	movdqu STATE6, 0x50(OUTP)
	movdqu STATE7, 0x60(OUTP)
	movdqu STATE8, 0x70(OUTP)

	sub $128, LEN
	add $128, INP
	add $128, OUTP
	cmp $128, LEN
	jge .Lcbc_dec8
	cmp $16, LEN
	jb .Lcbc_dec_noerr
#endif

.align 4
.Lcbc_dec1_pre:
	movdqu (IVP), STATE3
.Lcbc_dec1:
	movdqu (INP), STATE2
	movdqa STATE2, STATE1

	cmp $16, KLEN
	je .Lcbc_dec1_128
	AESDEC256KL HANDLEP, STATE1
	jz .Lcbc_dec_err
	jmp .Lcbc_dec1_end
.Lcbc_dec1_128:
	AESDEC128KL HANDLEP, STATE1
	jz .Lcbc_dec_err

.Lcbc_dec1_end:
	pxor STATE3, STATE1
	movdqu STATE1, (OUTP)
	movdqa STATE2, STATE3
	sub $16, LEN
	add $16, INP
	add $16, OUTP
	cmp $16, LEN
	jge .Lcbc_dec1
	movdqu STATE3, (IVP)

.Lcbc_dec_noerr:
	xor AREG, AREG
	jmp .Lcbc_dec_end
.Lcbc_dec_err:
	mov $1, AREG
.Lcbc_dec_end:
#ifndef __x86_64__
	popl KLEN
	popl HANDLEP
	popl LEN
	popl IVP
#endif
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_cbc_dec)


#ifdef __x86_64__

/*
 * CTR implementations
 */

.pushsection .rodata
.align 16
.Lbswap_mask:
	.byte 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
.popsection

.section	.rodata.cst16.gf128mul_x_ble_mask, "aM", @progbits, 16
.align 16
.Lgf128mul_x_ble_mask:
	.octa 0x00000000000000010000000000000087

#define BSWAP_MASK	%xmm10
#define CTR		%xmm11
#define INC		%xmm12
#define IV		%xmm13
#define TCTR_LOW	%r11

.text
.align 4
__aeskl_ctr_init:
	movdqa .Lbswap_mask, BSWAP_MASK
	movdqa IV, CTR
	PSHUFB_XMM BSWAP_MASK CTR
	mov $1, TCTR_LOW
	MOVQ_R64_XMM TCTR_LOW INC
	MOVQ_R64_XMM CTR TCTR_LOW
	ret
SYM_FUNC_END(__aeskl_ctr_init)

.align 4
__aeskl_ctr_inc:
	paddq INC, CTR
	add $1, TCTR_LOW
	jnc .Lctr_inc_low
	pslldq $8, INC
	paddq INC, CTR
	psrldq $8, INC
.Lctr_inc_low:
	movdqa CTR, IV
	PSHUFB_XMM BSWAP_MASK IV
	ret
SYM_FUNC_END(__aeskl_ctr_inc)

/*
 * int __aeskl_ctr_enc(struct crypto_aes_ctx *ctx,
 *		       const u8 *dst,
 *		       u8 *src,
 *		       size_t len,
 *		       u8 *iv)
 */
SYM_FUNC_START(__aeskl_ctr_enc)
	FRAME_BEGIN
	cmp $16, LEN
	jb .Lctr_enc_noerr
	mov 480(HANDLEP), KLEN
	movdqu (IVP), IV
	call __aeskl_ctr_init
	cmp $128, LEN
	jb .Lctr_enc1

.align 4
.Lctr_enc8:
	movdqa IV, STATE1
	call __aeskl_ctr_inc
	movdqa IV, STATE2
	call __aeskl_ctr_inc
	movdqa IV, STATE3
	call __aeskl_ctr_inc
	movdqa IV, STATE4
	call __aeskl_ctr_inc
	movdqa IV, STATE5
	call __aeskl_ctr_inc
	movdqa IV, STATE6
	call __aeskl_ctr_inc
	movdqa IV, STATE7
	call __aeskl_ctr_inc
	movdqa IV, STATE8
	call __aeskl_ctr_inc

	cmp $16, KLEN
	je .Lctr_enc8_128
	AESENCWIDE256KL %rdi
	jz .Lctr_enc_err
	jmp .Lctr_enc8_end
.Lctr_enc8_128:
	AESENCWIDE128KL %rdi
	jz .Lctr_enc_err
.Lctr_enc8_end:

	movdqu (INP), IN1
	pxor IN1, STATE1
	movdqu STATE1, (OUTP)

	movdqu 0x10(INP), IN1
	pxor IN1, STATE2
	movdqu STATE2, 0x10(OUTP)

	movdqu 0x20(INP), IN1
	pxor IN1, STATE3
	movdqu STATE3, 0x20(OUTP)

	movdqu 0x30(INP), IN1
	pxor IN1, STATE4
	movdqu STATE4, 0x30(OUTP)

	movdqu 0x40(INP), IN1
	pxor IN1, STATE5
	movdqu STATE5, 0x40(OUTP)

	movdqu 0x50(INP), IN1
	pxor IN1, STATE6
	movdqu STATE6, 0x50(OUTP)

	movdqu 0x60(INP), IN1
	pxor IN1, STATE7
	movdqu STATE7, 0x60(OUTP)

	movdqu 0x70(INP), IN1
	pxor IN1, STATE8
	movdqu STATE8, 0x70(OUTP)

	sub $128, LEN
	add $128, INP
	add $128, OUTP
	cmp $128, LEN
	jge .Lctr_enc8
	cmp $16, LEN
	jb .Lctr_enc_end

.align 4
.Lctr_enc1:
	movdqa IV, STATE
	call __aeskl_ctr_inc

	cmp $16, KLEN
	je .Lctr_enc1_128
	AESENC256KL HANDLEP, STATE
	jmp .Lctr_enc1_end
.Lctr_enc1_128:
	AESENC128KL HANDLEP, STATE

.Lctr_enc1_end:
	movdqu (INP), IN
	pxor IN, STATE
	movdqu STATE, (OUTP)
	sub $16, LEN
	add $16, INP
	add $16, OUTP
	cmp $16, LEN
	jge .Lctr_enc1

.Lctr_enc_end:
	movdqu IV, (IVP)
.Lctr_enc_noerr:
	xor AREG, AREG
	jmp .Lctr_enc_ret
.Lctr_enc_err:
	mov $1, AREG
.Lctr_enc_ret:
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_ctr_enc)

/*
 * XTS implementation
 */
#define GF128MUL_MASK %xmm10

#define __aeskl_gf128mul_x_ble() \
	pshufd $0x13, IV, CTR; \
	paddq IV, IV; \
	psrad $31, CTR; \
	pand GF128MUL_MASK, CTR; \
	pxor CTR, IV;

/*
 * int __aeskl_xts_crypt8(const struct crypto_aes_ctx *ctx,
 *			  const u8 *dst,
 *			  u8 *src,
 *			  bool enc,
 *			  u8 *iv)
 */
SYM_FUNC_START(__aeskl_xts_crypt8)
	FRAME_BEGIN

	movdqa .Lgf128mul_x_ble_mask, GF128MUL_MASK
	movdqu (IVP), IV

	mov 480(HANDLEP), KLEN

	movdqa IV, STATE1
	movdqu (INP), INC
	pxor INC, STATE1
	movdqu IV, (OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE2
	movdqu 0x10(INP), INC
	pxor INC, STATE2
	movdqu IV, 0x10(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE3
	movdqu 0x20(INP), INC
	pxor INC, STATE3
	movdqu IV, 0x20(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE4
	movdqu 0x30(INP), INC
	pxor INC, STATE4
	movdqu IV, 0x30(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE5
	movdqu 0x40(INP), INC
	pxor INC, STATE5
	movdqu IV, 0x40(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE6
	movdqu 0x50(INP), INC
	pxor INC, STATE6
	movdqu IV, 0x50(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE7
	movdqu 0x60(INP), INC
	pxor INC, STATE7
	movdqu IV, 0x60(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqa IV, STATE8
	movdqu 0x70(INP), INC
	pxor INC, STATE8
	movdqu IV, 0x70(OUTP)

	cmpb $0, %cl
	je  .Lxts_dec8
	cmp $16, KLEN
	je .Lxts_enc8_128
	AESENCWIDE256KL %rdi
	jz .Lxts_err
	jmp .Lxts_crypt8_end
.Lxts_enc8_128:
	AESENCWIDE128KL %rdi
	jz .Lxts_err
	jmp .Lxts_crypt8_end
.Lxts_dec8:
	cmp $16, KLEN
	je .Lxts_dec8_128
	AESDECWIDE256KL %rdi
	jz .Lxts_err
	jmp .Lxts_crypt8_end
.Lxts_dec8_128:
	AESDECWIDE128KL %rdi
	jz .Lxts_err

.Lxts_crypt8_end:
	movdqu 0x00(OUTP), INC
	pxor INC, STATE1
	movdqu STATE1, 0x00(OUTP)

	movdqu 0x10(OUTP), INC
	pxor INC, STATE2
	movdqu STATE2, 0x10(OUTP)

	movdqu 0x20(OUTP), INC
	pxor INC, STATE3
	movdqu STATE3, 0x20(OUTP)

	movdqu 0x30(OUTP), INC
	pxor INC, STATE4
	movdqu STATE4, 0x30(OUTP)

	movdqu 0x40(OUTP), INC
	pxor INC, STATE5
	movdqu STATE5, 0x40(OUTP)

	movdqu 0x50(OUTP), INC
	pxor INC, STATE6
	movdqu STATE6, 0x50(OUTP)

	movdqu 0x60(OUTP), INC
	pxor INC, STATE7
	movdqu STATE7, 0x60(OUTP)

	movdqu 0x70(OUTP), INC
	pxor INC, STATE8
	movdqu STATE8, 0x70(OUTP)

	__aeskl_gf128mul_x_ble()
	movdqu IV, (IVP)

	xor AREG, AREG
	jmp .Lxts_end
.Lxts_err:
	mov $1, AREG
.Lxts_end:
	FRAME_END
	ret
SYM_FUNC_END(__aeskl_xts_crypt8)

#endif

