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
#include <crypto/internal/skcipher.h>
#include <crypto/internal/simd.h>
#include <crypto/xts.h>
#include <asm/keylocker.h>
#include <asm/cpu_device_id.h>
#include <asm/fpu/api.h>
#include <asm/simd.h>

#ifdef CONFIG_X86_64
#include <asm/crypto/glue_helper.h>
#endif

#define AESKL_ALIGN		16
#define AESKL_ALIGN_ATTR	__aligned(AESKL_ALIGN)
#define AES_BLOCK_MASK		(~(AES_BLOCK_SIZE - 1))
#define RFC4106_HASH_SUBKEY_SZ	16
#define AESKL_ALIGN_EXTRA	((AESKL_ALIGN - 1) & ~(CRYPTO_MINALIGN - 1))
#define CRYPTO_AES_CTX_SIZE \
	(sizeof(struct crypto_aes_ctx) + AESKL_ALIGN_EXTRA)

struct aeskl_xts_ctx {
	u8 raw_tweak_ctx[sizeof(struct crypto_aes_ctx)] AESKL_ALIGN_ATTR;
	u8 raw_crypt_ctx[sizeof(struct crypto_aes_ctx)] AESKL_ALIGN_ATTR;
};

#define XTS_AES_CTX_SIZE \
	(sizeof(struct aeskl_xts_ctx) + AESKL_ALIGN_EXTRA)

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

asmlinkage int __aeskl_ecb_enc(struct crypto_aes_ctx *ctx, u8 *out,
				const u8 *in, unsigned int len);
asmlinkage int __aeskl_ecb_dec(struct crypto_aes_ctx *ctx, u8 *out,
				const u8 *in, unsigned int len);
asmlinkage int aesni_ecb_enc(struct crypto_aes_ctx *ctx, u8 *out,
			      const u8 *in, unsigned int len);
asmlinkage int aesni_ecb_dec(struct crypto_aes_ctx *ctx, u8 *out,
			      const u8 *in, unsigned int len);

asmlinkage int __aeskl_cbc_enc(struct crypto_aes_ctx *ctx, u8 *out,
				const u8 *in, unsigned int len, u8 *iv);
asmlinkage int __aeskl_cbc_dec(struct crypto_aes_ctx *ctx, u8 *out,
				const u8 *in, unsigned int len, u8 *iv);
asmlinkage int aesni_cbc_enc(struct crypto_aes_ctx *ctx, u8 *out,
			      const u8 *in, unsigned int len, u8 *iv);
asmlinkage int aesni_cbc_dec(struct crypto_aes_ctx *ctx, u8 *out,
			      const u8 *in, unsigned int len, u8 *iv);

#ifdef CONFIG_X86_64
asmlinkage int __aeskl_ctr_enc(struct crypto_aes_ctx *ctx, u8 *out,
				const u8 *in, unsigned int len, u8 *iv);
asmlinkage void aesni_ctr_enc(struct crypto_aes_ctx *ctx, u8 *out,
			      const u8 *in, unsigned int len, u8 *iv);

asmlinkage int __aeskl_xts_crypt8(const struct crypto_aes_ctx *ctx,
				   u8 *out, const u8 *in, bool enc,
				   u8 *iv);
asmlinkage void aesni_xts_crypt8(const struct crypto_aes_ctx *ctx,
				 u8 *out, const u8 *in, bool enc, u8 *iv);
#endif

static inline void aeskl_enc1(const void *ctx, u8 *out, const u8 *in)
{
	int err;

	err = __aeskl_enc1(ctx, out, in);
	if (err)
		pr_err("aes-kl: invalid handle\n");
}

static inline void aeskl_dec1(const void *ctx, u8 *out, const u8 *in)
{
	int err;

	err = __aeskl_dec1(ctx, out, in);
	if (err)
		pr_err("aes-kl: invalid handle\n");
}

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

static int aeskl_skcipher_setkey(struct crypto_skcipher *tfm, const u8 *key,
				unsigned int len)
{
	return aeskl_setkey_common(crypto_skcipher_tfm(tfm),
				   crypto_skcipher_ctx(tfm), key, len);
}

static int ecb_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm;
	struct crypto_aes_ctx *ctx;
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	tfm = crypto_skcipher_reqtfm(req);
	ctx = aes_ctx(crypto_skcipher_ctx(tfm));

	err = skcipher_walk_virt(&walk, req, true);
	if (err)
		return err;

	while ((nbytes = walk.nbytes)) {
		unsigned int len = nbytes & AES_BLOCK_MASK;
		const u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_ecb_enc(ctx, dst, src, len);
		else
			err = __aeskl_ecb_enc(ctx, dst, src, len);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (ECB encrypt): invalid handle\n");
			return -EINVAL;
		}

		nbytes &= AES_BLOCK_SIZE - 1;

		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			return err;
	}

	return err;
}

static int ecb_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm;
	struct crypto_aes_ctx *ctx;
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	tfm = crypto_skcipher_reqtfm(req);
	ctx = aes_ctx(crypto_skcipher_ctx(tfm));

	err = skcipher_walk_virt(&walk, req, true);
	if (err)
		return err;

	while ((nbytes = walk.nbytes)) {
		unsigned int len = nbytes & AES_BLOCK_MASK;
		const u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_ecb_dec(ctx, dst, src, len);
		else
			err = __aeskl_ecb_dec(ctx, dst, src, len);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (ECB decrypt): invalid handle\n");
			return -EINVAL;
		}

		nbytes &= AES_BLOCK_SIZE - 1;

		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			return err;
	}

	return err;
}

static int cbc_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm;
	struct crypto_aes_ctx *ctx;
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	tfm = crypto_skcipher_reqtfm(req);
	ctx = aes_ctx(crypto_skcipher_ctx(tfm));
	err = skcipher_walk_virt(&walk, req, true);
	if (err)
		return err;

	while ((nbytes = walk.nbytes)) {
		unsigned int len = nbytes & AES_BLOCK_MASK;
		const u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;
		u8 *iv = walk.iv;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_cbc_enc(ctx, dst, src, len, iv);
		else
			err = __aeskl_cbc_enc(ctx, dst, src, len, iv);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (CBC encrypt): invalid handle\n");
			return -EINVAL;
		}

		nbytes &= AES_BLOCK_SIZE - 1;

		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			return err;
	}

	return err;
}

static int cbc_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm;
	struct crypto_aes_ctx *ctx;
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	tfm = crypto_skcipher_reqtfm(req);
	ctx = aes_ctx(crypto_skcipher_ctx(tfm));
	err = skcipher_walk_virt(&walk, req, true);
	if (err)
		return err;

	while ((nbytes = walk.nbytes)) {
		unsigned int len = nbytes & AES_BLOCK_MASK;
		const u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;
		u8 *iv = walk.iv;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_cbc_dec(ctx, dst, src, len, iv);
		else
			err = __aeskl_cbc_dec(ctx, dst, src, len, iv);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (CBC decrypt): invalid handle\n");
			return -EINVAL;
		}

		nbytes &= AES_BLOCK_SIZE - 1;

		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			return err;
	}

	return err;
}

#ifdef CONFIG_X86_64
static int ctr_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm;
	struct crypto_aes_ctx *ctx;
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;

	tfm = crypto_skcipher_reqtfm(req);
	ctx = aes_ctx(crypto_skcipher_ctx(tfm));

	err = skcipher_walk_virt(&walk, req, true);
	if (err)
		return err;

	while ((nbytes = walk.nbytes) >= AES_BLOCK_SIZE) {
		unsigned int len = nbytes & AES_BLOCK_MASK;
		const u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;
		u8 *iv = walk.iv;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_ctr_enc(ctx, dst, src, len, iv);
		else
			err = __aeskl_ctr_enc(ctx, dst, src, len, iv);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (CTR): invalid handle\n");
			return -EINVAL;
		}

		nbytes &= AES_BLOCK_SIZE - 1;

		err = skcipher_walk_done(&walk, nbytes);
		if (err)
			return err;
	}

	if (nbytes) {
		u8 keystream[AES_BLOCK_SIZE];
		u8 *src = walk.src.virt.addr;
		u8 *dst = walk.dst.virt.addr;
		u8 *ctrblk = walk.iv;

		kernel_fpu_begin();
		if (unlikely(ctx->key_length == AES_KEYSIZE_192))
			aesni_enc(ctx, keystream, ctrblk);
		else
			err = __aeskl_enc1(ctx, keystream, ctrblk);
		kernel_fpu_end();

		if (err) {
			pr_err("aes-keylocker (CTR, a block): invalid handle\n");
			return -EINVAL;
		}

		crypto_xor(keystream, src, nbytes);
		memcpy(dst, keystream, nbytes);
		crypto_inc(ctrblk, AES_BLOCK_SIZE);

		err = skcipher_walk_done(&walk, 0);
	}

	return err;
}

static int aeskl_xts_setkey(struct crypto_skcipher *tfm, const u8 *key,
			    unsigned int keylen)
{
	struct aeskl_xts_ctx *ctx = crypto_skcipher_ctx(tfm);
	int err;

	err = xts_verify_key(tfm, key, keylen);
	if (err)
		return err;

	keylen /= 2;

	/* first half of xts-key is for crypt */
	err = aeskl_setkey_common(crypto_skcipher_tfm(tfm),
				  ctx->raw_crypt_ctx, key, keylen);
	if (err)
		return err;

	/* second half of xts-key is for tweak */
	return aeskl_setkey_common(crypto_skcipher_tfm(tfm),
				     ctx->raw_tweak_ctx,
				     key + keylen, keylen);
}


static void aeskl_xts_tweak(const void *raw_ctx, u8 *out, const u8 *in)
{
	struct crypto_aes_ctx *ctx = aes_ctx((void *)raw_ctx);

	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		aesni_enc(raw_ctx, out, in);
	else
		aeskl_enc1(raw_ctx, out, in);
}

static void
aeskl_xts_enc1(const void *raw_ctx, u8 *dst, const u8 *src, le128 *iv)
{
	struct crypto_aes_ctx *ctx = aes_ctx((void *)raw_ctx);
	common_glue_func_t fn = aeskl_enc1;

	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		fn = aesni_enc;

	glue_xts_crypt_128bit_one(raw_ctx, dst, src, iv, fn);
}

static void
aeskl_xts_dec1(const void *raw_ctx, u8 *dst, const u8 *src, le128 *iv)
{
	struct crypto_aes_ctx *ctx = aes_ctx((void *)raw_ctx);
	common_glue_func_t fn = aeskl_dec1;

	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		fn = aesni_dec;

	glue_xts_crypt_128bit_one(raw_ctx, dst, src, iv, fn);
}

static void
aeskl_xts_enc8(const void *raw_ctx, u8 *dst, const u8 *src, le128 *iv)
{
	struct crypto_aes_ctx *ctx = aes_ctx((void *)raw_ctx);
	int err = 0;

	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		aesni_xts_crypt8(raw_ctx, dst, src, true, (u8 *)iv);
	else
		err = __aeskl_xts_crypt8(raw_ctx, dst, src, true, (u8 *)iv);

	if (err)
		pr_err("aes-keylocker (XTS encrypt): invalid handle\n");
}

static void
aeskl_xts_dec8(const void *raw_ctx, u8 *dst, const u8 *src, le128 *iv)
{
	struct crypto_aes_ctx *ctx = aes_ctx((void *)raw_ctx);
	int err = 0;

	if (unlikely(ctx->key_length == AES_KEYSIZE_192))
		aesni_xts_crypt8(raw_ctx, dst, src, false, (u8 *)iv);
	else
		__aeskl_xts_crypt8(raw_ctx, dst, src, false, (u8 *)iv);

	if (err)
		pr_err("aes-keylocker (XTS decrypt): invalid handle\n");
}

static const struct common_glue_ctx aeskl_xts_enc = {
	.num_funcs = 2,
	.fpu_blocks_limit = 1,

	.funcs = { {
		.num_blocks = 8,
		.fn_u = { .xts = aeskl_xts_enc8 }
	}, {
		.num_blocks = 1,
		.fn_u = { .xts = aeskl_xts_enc1 }
	} }
};

static const struct common_glue_ctx aeskl_xts_dec = {
	.num_funcs = 2,
	.fpu_blocks_limit = 1,

	.funcs = { {
		.num_blocks = 8,
		.fn_u = { .xts = aeskl_xts_dec8 }
	}, {
		.num_blocks = 1,
		.fn_u = { .xts = aeskl_xts_dec1 }
	} }
};

static int xts_crypt(struct skcipher_request *req, bool decrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct aeskl_xts_ctx *ctx = crypto_skcipher_ctx(tfm);
	const struct common_glue_ctx *gctx;

	if (decrypt)
		gctx = &aeskl_xts_dec;
	else
		gctx = &aeskl_xts_enc;

	return glue_xts_req_128bit(gctx, req,
				   aeskl_xts_tweak,
				   aes_ctx(ctx->raw_tweak_ctx),
				   aes_ctx(ctx->raw_crypt_ctx),
				   decrypt);
}

static int xts_encrypt(struct skcipher_request *req)
{
	return xts_crypt(req, false);
}

static int xts_decrypt(struct skcipher_request *req)
{
	return xts_crypt(req, true);
}
#endif

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

static struct skcipher_alg aeskl_skciphers[] = {
	{
		.base = {
			.cra_name		= "__ecb(aes)",
			.cra_driver_name	= "__ecb-aes-aeskl",
			.cra_priority		= 401,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= AES_BLOCK_SIZE,
			.cra_ctxsize		= CRYPTO_AES_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.setkey		= aeskl_skcipher_setkey,
		.encrypt	= ecb_encrypt,
		.decrypt	= ecb_decrypt,
	}, {
		.base = {
			.cra_name		= "__cbc(aes)",
			.cra_driver_name	= "__cbc-aes-aeskl",
			.cra_priority		= 401,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= AES_BLOCK_SIZE,
			.cra_ctxsize		= CRYPTO_AES_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= aeskl_skcipher_setkey,
		.encrypt	= cbc_encrypt,
		.decrypt	= cbc_decrypt,
#ifdef CONFIG_X86_64
	}, {
		.base = {
			.cra_name		= "__ctr(aes)",
			.cra_driver_name	= "__ctr-aes-aeskl",
			.cra_priority		= 401,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_AES_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= AES_MIN_KEY_SIZE,
		.max_keysize	= AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.chunksize	= AES_BLOCK_SIZE,
		.setkey		= aeskl_skcipher_setkey,
		.encrypt	= ctr_crypt,
		.decrypt	= ctr_crypt,
	}, {
		.base = {
			.cra_name		= "__xts(aes)",
			.cra_driver_name	= "__xts-aes-aeskl",
			.cra_priority		= 402,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= AES_BLOCK_SIZE,
			.cra_ctxsize		= XTS_AES_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= 2 * AES_MIN_KEY_SIZE,
		.max_keysize	= 2 * AES_MAX_KEY_SIZE,
		.ivsize		= AES_BLOCK_SIZE,
		.setkey		= aeskl_xts_setkey,
		.encrypt	= xts_encrypt,
		.decrypt	= xts_decrypt,
#endif
	}
};

static
struct simd_skcipher_alg *aeskl_simd_skciphers[ARRAY_SIZE(aeskl_skciphers)];

static int __init aeskl_init(void)
{
	u32 eax, ebx, ecx, edx;
	int err;

	if (!x86_match_cpu(aes_keylocker_cpuid))
		return -ENODEV;

	/* The CPUID leaf (0x19,0) enumerates detail capability */
	cpuid_count(KEYLOCKER_CPUID, 0, &eax, &ebx, &ecx, &edx);

	if (!(ebx & KEYLOCKER_CPUID_EBX_ENABLED) ||
	    !(ebx & KEYLOCKER_CPUID_EBX_WIDE_AES))
		return -ENODEV;

	err = crypto_register_alg(&aeskl_cipher_alg);
	if (err)
		return err;

	err = simd_register_skciphers_compat(aeskl_skciphers,
					     ARRAY_SIZE(aeskl_skciphers),
					     aeskl_simd_skciphers);
	if (err)
		goto unregister_algs;

	return 0;

unregister_algs:
	crypto_unregister_alg(&aeskl_cipher_alg);

	return err;
}

static void __exit aeskl_exit(void)
{
	simd_unregister_skciphers(aeskl_skciphers, ARRAY_SIZE(aeskl_skciphers),
				  aeskl_simd_skciphers);
	crypto_unregister_alg(&aeskl_cipher_alg);
}

late_initcall(aeskl_init);
module_exit(aeskl_exit);

MODULE_DESCRIPTION("Rijndael (AES) Cipher Algorithm, Intel AES-KeyLocker-based implementation");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("aes");
