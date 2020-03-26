/* SPDX-License-Identifier: GPL-2.0 */

/* Encrypted keys of format mktme */

/*
 * Payload size is based on the only currently supported algorithm,
 * AES-XTS 128. It requires 128 bits for each user supplied data
 * key and tweak key.
 */
#define MKTME_AES_XTS_SIZE	16	/* 16 bytes, 128 bits */

struct mktme_payload {
	u32	keyid_ctrl;	/* Command & Encryption Algorithm */
	u8	data_key[MKTME_AES_XTS_SIZE];
	u8	tweak_key[MKTME_AES_XTS_SIZE];
};

#define KEY_MKTME_PAYLOAD_LEN (sizeof(struct mktme_payload))

#ifdef CONFIG_X86_INTEL_MKTME
int mktme_request_key(struct key *key, struct mktme_payload *payload,
		      int encrypted_key_cmd);
void mktme_remove_key(struct key *key);

#else
static inline int mktme_request_key(struct key *key,
				    struct mktme_payload *payload,
				    int encrypted_key_cmd) { return -EINVAL; }
static inline void mktme_remove_key(struct key *key) {}
#endif /* CONFIG_X86_INTEL_MKTME */
