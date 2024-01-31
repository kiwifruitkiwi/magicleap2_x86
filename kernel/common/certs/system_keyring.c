// SPDX-License-Identifier: GPL-2.0-or-later
/* System trusted keyring for trusted public keys
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cred.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/verification.h>
#include <keys/asymmetric-type.h>
#include <keys/system_keyring.h>
#include <crypto/pkcs7.h>
#include <linux/fs.h>
#include <linux/slab.h>

static struct key *builtin_trusted_keys;
#ifdef CONFIG_SECONDARY_TRUSTED_KEYRING
static struct key *secondary_trusted_keys;
#endif
#ifdef CONFIG_INTEGRITY_PLATFORM_KEYRING
static struct key *platform_trusted_keys;
#endif

struct key *get_builtin_trusted_keys(void)
{
	return builtin_trusted_keys;
}

struct key *devcerts_keyring; /* developer certificates keyring */

extern __initconst const u8 system_certificate_list[];
extern __initconst const unsigned long system_certificate_list_size;

/**
 * restrict_link_to_builtin_trusted - Restrict keyring addition by built in CA
 *
 * Restrict the addition of keys into a keyring based on the key-to-be-added
 * being vouched for by a key in the built in system keyring.
 */
int restrict_link_by_builtin_trusted(struct key *dest_keyring,
				     const struct key_type *type,
				     const union key_payload *payload,
				     struct key *restriction_key)
{
	return restrict_link_by_signature(dest_keyring, type, payload,
					  builtin_trusted_keys);
}

#ifdef CONFIG_SECONDARY_TRUSTED_KEYRING
/**
 * restrict_link_by_builtin_and_secondary_trusted - Restrict keyring
 *   addition by both builtin and secondary keyrings
 *
 * Restrict the addition of keys into a keyring based on the key-to-be-added
 * being vouched for by a key in either the built-in or the secondary system
 * keyrings.
 */
int restrict_link_by_builtin_and_secondary_trusted(
	struct key *dest_keyring,
	const struct key_type *type,
	const union key_payload *payload,
	struct key *restrict_key)
{
	/* If we have a secondary trusted keyring, then that contains a link
	 * through to the builtin keyring and the search will follow that link.
	 */
	if (type == &key_type_keyring &&
	    dest_keyring == secondary_trusted_keys &&
	    payload == &builtin_trusted_keys->payload)
		/* Allow the builtin keyring to be added to the secondary */
		return 0;

	return restrict_link_by_signature(dest_keyring, type, payload,
					  secondary_trusted_keys);
}

/**
 * Allocate a struct key_restriction for the "builtin and secondary trust"
 * keyring. Only for use in system_trusted_keyring_init().
 */
static __init struct key_restriction *get_builtin_and_secondary_restriction(void)
{
	struct key_restriction *restriction;

	restriction = kzalloc(sizeof(struct key_restriction), GFP_KERNEL);

	if (!restriction)
		panic("Can't allocate secondary trusted keyring restriction\n");

	restriction->check = restrict_link_by_builtin_and_secondary_trusted;

	return restriction;
}
#endif

/*
 * Create the trusted keyrings
 */
static __init int system_trusted_keyring_init(void)
{
	struct key_restriction *devcert_restriction;
	pr_notice("Initialise system trusted keyrings\n");

	builtin_trusted_keys =
		keyring_alloc(".builtin_trusted_keys",
			      KUIDT_INIT(0), KGIDT_INIT(0), current_cred(),
			      ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
			      KEY_USR_VIEW | KEY_USR_READ | KEY_USR_SEARCH),
			      KEY_ALLOC_NOT_IN_QUOTA,
			      NULL, NULL);
	if (IS_ERR(builtin_trusted_keys))
		panic("Can't allocate builtin trusted keyring\n");

#ifdef CONFIG_SECONDARY_TRUSTED_KEYRING
	secondary_trusted_keys =
		keyring_alloc(".secondary_trusted_keys",
			      KUIDT_INIT(0), KGIDT_INIT(0), current_cred(),
			      ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
			       KEY_USR_VIEW | KEY_USR_READ | KEY_USR_SEARCH |
			       KEY_USR_WRITE),
			      KEY_ALLOC_NOT_IN_QUOTA,
			      get_builtin_and_secondary_restriction(),
			      NULL);
	if (IS_ERR(secondary_trusted_keys))
		panic("Can't allocate secondary trusted keyring\n");

	if (key_link(secondary_trusted_keys, builtin_trusted_keys) < 0)
		panic("Can't link trusted keyrings\n");
#endif

	pr_notice("Initializing developer certificates keyring\n");

	devcert_restriction = kzalloc(sizeof(struct key_restriction), GFP_KERNEL);
        if (!devcert_restriction)
            panic("Can't allocate devcert key restriction.");

	devcert_restriction->check = restrict_link_by_builtin_trusted;

	devcerts_keyring =
		keyring_alloc(".devcerts_keyring",
			      KUIDT_INIT(0), KGIDT_INIT(0), current_cred(),
			      ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
			      (KEY_USR_ALL & ~KEY_USR_SETATTR) |
			      (KEY_OTH_ALL & ~KEY_OTH_SETATTR)),
			      KEY_ALLOC_NOT_IN_QUOTA,
			      devcert_restriction,
			      NULL);
	if (IS_ERR(devcerts_keyring))
		panic("Can't allocate developers certificate keyring\n");

	return 0;
}

/*
 * Must be initialised before we try and load the keys into the keyring.
 */
device_initcall(system_trusted_keyring_init);


/*
 * Read the built-in list of X.509 certificates.
 * CALLER MUST FREE *out_list_buf
 */
static int read_system_certificate_list(u8 **out_list_buf, loff_t *out_list_size)
{
	struct file *list_file;
	u8 *list_buf;
	loff_t list_size;
	int error;
	loff_t offset = 0;

	list_file = filp_open("/system_certificate_list", O_RDONLY, 0);
	if (IS_ERR(list_file))
		return PTR_ERR(list_file);

	list_size = i_size_read(file_inode(list_file));

	list_buf = kmalloc(list_size, GFP_KERNEL);
	if (list_buf == NULL) {
		filp_close(list_file, NULL);
		return -ENOMEM;
	}

	error = kernel_read(list_file, (char *)list_buf, list_size, &offset);
	if (error < 0) {
		kfree(list_buf);
		filp_close(list_file, NULL);
		return error;
	}

	filp_close(list_file, NULL);
	*out_list_buf = list_buf;
	*out_list_size = list_size;
	return 0;
}

/*
 * Load the compiled-in list of X.509 certificates.
 */
static __init int load_system_certificate_list(void)
{
	key_ref_t key;
	const u8 *p, *end;
	size_t plen;
	int err;

	u8 *system_certificate_list;
	loff_t system_certificate_list_size;

	pr_notice("Loading compiled-in X.509 certificates\n");

	err = read_system_certificate_list(&system_certificate_list, &system_certificate_list_size);
	if (err < 0) {
		pr_err("Problem reading system certificate list file\n");
		return err;
	}

	p = system_certificate_list;
	end = p + system_certificate_list_size;
	while (p < end) {
		/* Each cert begins with an ASN.1 SEQUENCE tag and must be more
		 * than 256 bytes in size.
		 */
		if (end - p < 4)
			goto dodgy_cert;
		if (p[0] != 0x30 &&
		    p[1] != 0x82)
			goto dodgy_cert;
		plen = (p[2] << 8) | p[3];
		plen += 4;
		if (plen > end - p)
			goto dodgy_cert;

		key = key_create_or_update(make_key_ref(builtin_trusted_keys, 1),
					   "asymmetric",
					   NULL,
					   p,
					   plen,
					   ((KEY_POS_ALL & ~KEY_POS_SETATTR) |
					   KEY_USR_VIEW | KEY_USR_READ),
					   KEY_ALLOC_NOT_IN_QUOTA |
					   KEY_ALLOC_BUILT_IN |
					   KEY_ALLOC_BYPASS_RESTRICTION);
		if (IS_ERR(key)) {
			pr_err("Problem loading in-kernel X.509 certificate (%ld)\n",
			       PTR_ERR(key));
		} else {
			pr_notice("Loaded X.509 cert '%s'\n",
				  key_ref_to_ptr(key)->description);
			key_ref_put(key);
		}
		p += plen;
	}
	kfree(system_certificate_list);

	return 0;

dodgy_cert:
	kfree(system_certificate_list);
	pr_err("Problem parsing in-kernel X.509 certificate list\n");
	return 0;
}
late_initcall(load_system_certificate_list);

#ifdef CONFIG_SYSTEM_DATA_VERIFICATION

/**
 * verify_pkcs7_message_sig - Verify a PKCS#7-based signature on system data.
 * @data: The data to be verified (NULL if expecting internal data).
 * @len: Size of @data.
 * @pkcs7: The PKCS#7 message that is the signature.
 * @trusted_keys: Trusted keys to use (NULL for builtin trusted keys only,
 *					(void *)1UL for all trusted keys).
 * @usage: The use to which the key is being put.
 * @view_content: Callback to gain access to content.
 * @ctx: Context for callback.
 */
int verify_pkcs7_message_sig(const void *data, size_t len,
			     struct pkcs7_message *pkcs7,
			     struct key *trusted_keys,
			     enum key_being_used_for usage,
			     int (*view_content)(void *ctx,
						 const void *data, size_t len,
						 size_t asn1hdrlen),
			     void *ctx)
{
	int ret;

	/* The data should be detached - so we need to supply it. */
	if (data && pkcs7_supply_detached_data(pkcs7, data, len) < 0) {
		pr_err("PKCS#7 signature with non-detached data\n");
		ret = -EBADMSG;
		goto error;
	}

	ret = pkcs7_verify(pkcs7, usage);
	if (ret < 0)
		goto error;

	if (!trusted_keys) {
		trusted_keys = builtin_trusted_keys;
	} else if (trusted_keys == VERIFY_USE_SECONDARY_KEYRING) {
#ifdef CONFIG_SECONDARY_TRUSTED_KEYRING
		trusted_keys = secondary_trusted_keys;
#else
		trusted_keys = builtin_trusted_keys;
#endif
	} else if (trusted_keys == VERIFY_USE_PLATFORM_KEYRING) {
#ifdef CONFIG_INTEGRITY_PLATFORM_KEYRING
		trusted_keys = platform_trusted_keys;
#else
		trusted_keys = NULL;
#endif
		if (!trusted_keys) {
			ret = -ENOKEY;
			pr_devel("PKCS#7 platform keyring is not available\n");
			goto error;
		}
	}
	ret = pkcs7_validate_trust(pkcs7, trusted_keys);
	if (ret < 0) {
		/*
		 * If verification through the system_trusted_keyring fails,
		 * we try to verify through the devcerts_keyring whose keys
		 * verify binaries' embedded signature signed locally
		 * by external developers at dev time.
		 */
		ret = pkcs7_validate_trust(pkcs7, devcerts_keyring);
		if (ret < 0) {
			if (ret == -ENOKEY)
			pr_devel("PKCS#7 signature not signed with a trusted key\n");
			goto error;
		}
	}

	if (view_content) {
		size_t asn1hdrlen;

		ret = pkcs7_get_content_data(pkcs7, &data, &len, &asn1hdrlen);
		if (ret < 0) {
			if (ret == -ENODATA)
				pr_devel("PKCS#7 message does not contain data\n");
			goto error;
		}

		ret = view_content(ctx, data, len, asn1hdrlen);
	}

error:
	pr_devel("<==%s() = %d\n", __func__, ret);
	return ret;
}

/**
 * verify_pkcs7_signature - Verify a PKCS#7-based signature on system data.
 * @data: The data to be verified (NULL if expecting internal data).
 * @len: Size of @data.
 * @raw_pkcs7: The PKCS#7 message that is the signature.
 * @pkcs7_len: The size of @raw_pkcs7.
 * @trusted_keys: Trusted keys to use (NULL for builtin trusted keys only,
 *					(void *)1UL for all trusted keys).
 * @usage: The use to which the key is being put.
 * @view_content: Callback to gain access to content.
 * @ctx: Context for callback.
 */
int verify_pkcs7_signature(const void *data, size_t len,
			   const void *raw_pkcs7, size_t pkcs7_len,
			   struct key *trusted_keys,
			   enum key_being_used_for usage,
			   int (*view_content)(void *ctx,
					       const void *data, size_t len,
					       size_t asn1hdrlen),
			   void *ctx)
{
	struct pkcs7_message *pkcs7;
	int ret;

	pkcs7 = pkcs7_parse_message(raw_pkcs7, pkcs7_len);
	if (IS_ERR(pkcs7))
		return PTR_ERR(pkcs7);

	ret = verify_pkcs7_message_sig(data, len, pkcs7, trusted_keys, usage,
				       view_content, ctx);

	pkcs7_free_message(pkcs7);
	pr_devel("<==%s() = %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(verify_pkcs7_signature);

#endif /* CONFIG_SYSTEM_DATA_VERIFICATION */

#ifdef CONFIG_INTEGRITY_PLATFORM_KEYRING
void __init set_platform_trusted_keys(struct key *keyring)
{
	platform_trusted_keys = keyring;
}
#endif
