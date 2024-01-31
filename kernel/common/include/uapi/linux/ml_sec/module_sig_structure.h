/*
 * Copyright (c) 2017, Magic Leap, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODULE_SIG_STRUCTURE_H__
#define __MODULE_SIG_STRUCTURE_H__

#include <linux/types.h>

/* Grabbed from module.h */
#ifndef MODULE_SIG_STRING
#define MODULE_SIG_STRING "~Module signature appended~\n"
#endif

/* Grabbed from kernel/module_signing.c */
enum pkey_id_type {
    PKEY_ID_PGP,		/* OpenPGP generated key ID */
    PKEY_ID_X509,		/* X.509 arbitrary subjectKeyIdentifier */
    PKEY_ID_PKCS7,		/* Signature in PKCS#7 message */
};

/* Grabbed from kernel/module_signing.c */
/*
 * Module signature information block.
 *
 * The constituents of the signature section are, in order:
 *
 * 	- Signer's name
 * 	- Key identifier
 * 	- Signature data
 * 	- Information block
 */
struct module_signature {
	__u8	algo;		/* Public-key crypto algorithm [0] */
	__u8	hash;		/* Digest algorithm [0] */
	__u8	id_type;	/* Key identifier type [PKEY_ID_PKCS7] */
	__u8	signer_len;	/* Length of signer's name [0] */
	__u8	key_id_len;	/* Length of key identifier [0] */
	__u8	__pad[3];
	__be32	sig_len;	/* Length of signature data */
};



#endif	/* __MODULE_SIG_STRUCTURE__ */
