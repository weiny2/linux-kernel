/* Module internals
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */

/*
 * Module hash.
 */
struct module_hash {
	struct list_head list;  /* list of all hashs */
	u8 hash;                /* Hash algorithm [enum pkey_hash_algo] */
	char *hash_name;        /* nams string of hash */
	size_t size;            /* size of hash */
	u8 hash_data[];         /* Hash data */
};

extern struct list_head module_hash_blacklist;

extern int mod_verify_sig(const void *mod, unsigned long *_modlen);
