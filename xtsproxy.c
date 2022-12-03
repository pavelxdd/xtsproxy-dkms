/* https://github.com/cloudflare/linux/blob/master/patches/0024-Add-xtsproxy-Crypto-API-module.patch */

#include <linux/module.h>
#include <linux/crypto.h>
#include <crypto/internal/skcipher.h>
#include <crypto/aes.h>
#include <asm/fpu/api.h>

struct xtsproxy_ctx {
	struct crypto_skcipher *xts_aesni;
	struct crypto_skcipher *xts_generic;
};

static int xtsproxy_init_tfm(struct crypto_skcipher *tfm)
{
	struct xtsproxy_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct crypto_skcipher *xts_aesni;
	struct crypto_skcipher *xts_generic;

	/* AESNI based XTS implementation, requires FPU to be available */
	xts_aesni = crypto_alloc_skcipher("__xts-aes-aesni", CRYPTO_ALG_INTERNAL, 0);
	if (IS_ERR(xts_aesni))
		return PTR_ERR(xts_aesni);

	/* generic XTS implementation based on generic FPU-less AES */
	/* there is also aes-aesni implementation, which falls back to aes-generic */
	/* but we're doing FPU checks in our code, so no need to repeat those */
	/* as we will always fallback to aes-generic in this case */
	xts_generic = crypto_alloc_skcipher("xts(ecb(aes-generic))", 0, 0);
	if (IS_ERR(xts_generic)) {
		crypto_free_skcipher(xts_aesni);
		return PTR_ERR(xts_generic);
	}

	/* make sure we allocate enough request memory for both implementations */
	crypto_skcipher_set_reqsize(tfm, max(crypto_skcipher_reqsize(xts_aesni),
										 crypto_skcipher_reqsize(xts_generic)));
	ctx->xts_aesni = xts_aesni;
	ctx->xts_generic = xts_generic;
	return 0;
}

static void xtsproxy_exit_tfm(struct crypto_skcipher *tfm)
{
	struct xtsproxy_ctx *ctx = crypto_skcipher_ctx(tfm);
	crypto_free_skcipher(ctx->xts_aesni);
	crypto_free_skcipher(ctx->xts_generic);
}

static int xtsproxy_setkey(struct crypto_skcipher *tfm,
						   const u8 *key, unsigned int keylen)
{
	struct xtsproxy_ctx *ctx = crypto_skcipher_ctx(tfm);

	int err = crypto_skcipher_setkey(ctx->xts_aesni, key, keylen);
	if (err)
		return err;

	return crypto_skcipher_setkey(ctx->xts_generic, key, keylen);
}

static __always_inline
int xtsproxy_crypt(struct skcipher_request *req, int enc)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct xtsproxy_ctx *ctx = crypto_skcipher_ctx(tfm);

	skcipher_request_set_tfm(req, irq_fpu_usable()
								  ? ctx->xts_aesni
								  : ctx->xts_generic);

	/* underlying implementations should not try to sleep */
	req->base.flags &= ~(CRYPTO_TFM_REQ_MAY_SLEEP |
						 CRYPTO_TFM_REQ_MAY_BACKLOG);

	return enc ? crypto_skcipher_encrypt(req)
			   : crypto_skcipher_decrypt(req);
}

static int xtsproxy_encrypt(struct skcipher_request *req)
{
	return xtsproxy_crypt(req, 1);
}

static int xtsproxy_decrypt(struct skcipher_request *req)
{
	return xtsproxy_crypt(req, 0);
}

static struct skcipher_alg xtsproxy_skcipher = {
	.base.cra_name			= "xts(aes)",
	.base.cra_driver_name	= "xts-aes-xtsproxy",
	.base.cra_module		= THIS_MODULE,
	/* make sure we don't use it unless requested explicitly */
	.base.cra_priority		= 0,
	.base.cra_flags			= 0,
	.base.cra_blocksize	 	= AES_BLOCK_SIZE,
	.base.cra_ctxsize		= sizeof(struct xtsproxy_ctx),
	.setkey					= xtsproxy_setkey,
	.encrypt				= xtsproxy_encrypt,
	.decrypt				= xtsproxy_decrypt,
	.init					= xtsproxy_init_tfm,
	.exit					= xtsproxy_exit_tfm,
	.min_keysize			= 2 * AES_MIN_KEY_SIZE,
	.max_keysize			= 2 * AES_MAX_KEY_SIZE,
	.ivsize					= AES_BLOCK_SIZE,
};

static int __init xtsproxy_init(void)
{
	return crypto_register_skcipher(&xtsproxy_skcipher);
}

static void __exit xtsproxy_fini(void)
{
	crypto_unregister_skcipher(&xtsproxy_skcipher);
}

module_init(xtsproxy_init);
module_exit(xtsproxy_fini);

MODULE_DESCRIPTION("XTS-AES using AESNI implementation with generic AES fallback");
MODULE_AUTHOR("Ignat Korchagin <ignat@cloudflare.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("xts(aes)");
