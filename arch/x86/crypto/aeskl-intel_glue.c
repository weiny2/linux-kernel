// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Support for Intel AES KeyLocker instructions. This file contains glue
 * code, the real AES implementation will be in aeskl-intel_asm.S.
 *
 * Most code is based on AES NI glue code, aesni-intel_glue.c
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/err.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <asm/keylocker.h>
#include <asm/cpu_device_id.h>
#include <asm/fpu/api.h>

#ifdef CONFIG_X86_64
#include <asm/crypto/glue_helper.h>
#endif

#define AESKL_ALIGN		16
#define AES_BLOCK_MASK		(~(AES_BLOCK_SIZE - 1))
#define AESKL_ALIGN_EXTRA	((AESKL_ALIGN - 1) & ~(CRYPTO_MINALIGN - 1))
#define CRYPTO_AES_CTX_SIZE \
	(sizeof(struct crypto_aes_ctx) + AESKL_ALIGN_EXTRA)

static const struct x86_cpu_id aes_keylocker_cpuid[] = {
	X86_MATCH_FEATURE(X86_FEATURE_AES, NULL),
	X86_MATCH_FEATURE(X86_FEATURE_KEYLOCKER, NULL),
	{}
};

asmlinkage int __aeskl_setkey(struct crypto_aes_ctx *ctx, const u8 *in_key,
			      unsigned int key_len);
asmlinkage int aesni_set_key(struct crypto_aes_ctx *ctx, const u8 *in_key,
			     unsigned int key_en);

asmlinkage int __aeskl_enc1(const void *ctx, u8 *out, const u8 *in);
asmlinkage int __aeskl_dec1(const void *ctx, u8 *out, const u8 *in);
asmlinkage void aesni_enc(const void *ctx, u8 *out, const u8 *in);
asmlinkage void aesni_dec(const void *ctx, u8 *out, const u8 *in);

static inline struct crypto_aes_ctx *aes_ctx(void *raw_ctx)
{
	unsigned long addr = (unsigned long)raw_ctx;
	unsigned long align = AESKL_ALIGN;

	if (align <= crypto_tfm_ctx_alignment())
		align = 1;
	return (struct crypto_aes_ctx *)ALIGN(addr, align);
}

static int aeskl_setkey_common(struct crypto_tfm *tfm, void *raw_ctx,
			       const u8 *in_key, unsigned int key_len)
{
	struct crypto_aes_ctx *ctx = aes_ctx(raw_ctx);
	int err;

	/*
	 * 192-bit key size is not supported by the KeyLocker instructions.
	 * So, falls back to either AES-NI or baseline implementation.
	 */
	if (unlikely(key_len == AES_KEYSIZE_192)) {
		if (!irq_fpu_usable())
			return aes_expandkey(ctx, in_key, key_len);

		kernel_fpu_begin();
		err = aesni_set_key(ctx, in_key, key_len);
		kernel_fpu_end();

		return err;
	}

	if ((key_len != AES_KEYSIZE_128) && (key_len != AES_KEYSIZE_256))
		return -EINVAL;

	if (!irq_fpu_usable())
		return -EBUSY;

	kernel_fpu_begin();
	/* Encode the key to a handle that is only usable at ring 0 */
	err = __aeskl_setkey(ctx, in_key, key_len);
	kernel_fpu_end();

	return err;
}

static int aeskl_setkey(struct crypto_tfm *tfm, const u8 *in_key,
			unsigned int key_len)
{
	return aeskl_setkey_common(tfm, crypto_tfm_ctx(tfm), in_key, key_len);
}

static void aeskl_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct crypto_aes_ctx *ctx = aes_ctx(crypto_tfm_ctx(tfm));
	int err = 0;

	kernel_fpu_begin();
	/* AESENC192 is not supported. So, falls back to AES-NI.*/
	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		aesni_enc(ctx, dst, src);
	else
		err = __aeskl_enc1(ctx, dst, src);
	kernel_fpu_end();

	if (err)
		pr_err("aes-keylocker (encrypt): invalid handle\n");
}

static void aeskl_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	struct crypto_aes_ctx *ctx = aes_ctx(crypto_tfm_ctx(tfm));
	int err = 0;

	kernel_fpu_begin();
	/* AESENC192 is not supported. So, falls back to AES-NI.*/
	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		aesni_dec(ctx, dst, src);
	else
		err = __aeskl_dec1(ctx, dst, src);
	kernel_fpu_end();

	if (err)
		pr_err("aes-keylocker (encrypt): invalid handle\n");
}

static struct crypto_alg aeskl_cipher_alg = {
	.cra_name		= "aes",
	.cra_driver_name	= "aes-aeskl",
	.cra_priority		= 301,
	.cra_flags		= CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		= AES_BLOCK_SIZE,
	.cra_ctxsize		= CRYPTO_AES_CTX_SIZE,
	.cra_module		= THIS_MODULE,
	.cra_u	= {
		.cipher	= {
			.cia_min_keysize	= AES_MIN_KEY_SIZE,
			.cia_max_keysize	= AES_MAX_KEY_SIZE,
			.cia_setkey		= aeskl_setkey,
			.cia_encrypt		= aeskl_encrypt,
			.cia_decrypt		= aeskl_decrypt
		}
	}
};

static int __init aeskl_init(void)
{
	u32 eax, ebx, ecx, edx;
	int err;

	if (!x86_match_cpu(aes_keylocker_cpuid))
		return -ENODEV;

	/* The CPUID leaf (0x19,0) enumerates detail capability */
	cpuid_count(KEYLOCKER_CPUID, 0, &eax, &ebx, &ecx, &edx);

	if (!(ebx & KEYLOCKER_CPUID_EBX_ENABLED))
		return -ENODEV;

	err = crypto_register_alg(&aeskl_cipher_alg);
	if (err)
		return err;

	return 0;
}

static void __exit aeskl_exit(void)
{
	crypto_unregister_alg(&aeskl_cipher_alg);
}

late_initcall(aeskl_init);
module_exit(aeskl_exit);

MODULE_DESCRIPTION("Rijndael (AES) Cipher Algorithm, Intel AES-KeyLocker-based implementation");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("aes");
