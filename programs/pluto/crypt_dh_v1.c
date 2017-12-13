/*
 * Cryptographic helper function - calculate DH
 *
 * Copyright (C) 2006-2008 Michael C. Richardson <mcr@xelerance.com>
 * Copyright (C) 2007-2009 Paul Wouters <paul@xelerance.com>
 * Copyright (C) 2009 Avesh Agarwal <avagarwa@redhat.com>
 * Copyright (C) 2009 David McCullough <david_mccullough@securecomputing.com>
 * Copyright (C) 2012-2013 Paul Wouters <paul@libreswan.org>
 * Copyright (C) 2015 Paul Wouters <pwouters@redaht.com>
 * Copyright (C) 2017 Antony Antony <antony@phenome.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * This code was developed with the support of IXIA communications.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <signal.h>

#include <libreswan.h>

#include "sysdep.h"
#include "constants.h"
#include "defs.h"
#include "packet.h"
#include "demux.h"
#include "crypto.h"
#include "rnd.h"
#include "state.h"
#include "pluto_crypt.h"
#include "lswlog.h"
#include "log.h"
#include "ike_alg.h"
#include "id.h"
#include "keys.h"
#include "crypt_symkey.h" /* to get free_any_symkey */
#include "crypt_dh.h"

/*
 * invoke helper to do DH work (IKEv1)
 *
 * Note: dh must be heap-allocated.
 */
stf_status start_dh_secretiv(struct pluto_crypto_req_cont *dh,
			     struct state *st,
			     enum crypto_importance importance,
			     enum original_role role,
			     const struct oakley_group_desc *oakley_group2)
{
	const chunk_t *pss = get_preshared_secret(st->st_connection);

	struct pcr_skeyid_q *const dhq = pcr_dh_init(dh, pcr_compute_dh_iv, importance);

	passert(st->st_sec_in_use);

	/* convert appropriate data to dhq */
	dhq->auth = st->st_oakley.auth;
	dhq->prf = st->st_oakley.ta_prf;
	dhq->oakley_group = oakley_group2;
	dhq->role = role;
	dhq->key_size = st->st_oakley.enckeylen / BITS_PER_BYTE;
	dhq->salt_size = st->st_oakley.ta_encrypt->salt_size;

	passert(dhq->oakley_group != OAKLEY_GROUP_invalid);
	if (pss != NULL)
		WIRE_CLONE_CHUNK(*dhq, pss, *pss);
	WIRE_CLONE_CHUNK(*dhq, ni, st->st_ni);
	WIRE_CLONE_CHUNK(*dhq, nr, st->st_nr);
	WIRE_CLONE_CHUNK(*dhq, gi, st->st_gi);
	WIRE_CLONE_CHUNK(*dhq, gr, st->st_gr);

	dhq->secret = st->st_sec_nss;

	dhq->encrypter = st->st_oakley.ta_encrypt;
	DBG(DBG_CRYPT,
	    DBG_log("Copying DH pub key pointer to be sent to a thread helper"));
	dhq->pubk = st->st_pubk_nss;

	ALLOC_WIRE_CHUNK(*dhq, icookie, COOKIE_SIZE);
	memcpy(WIRE_CHUNK_PTR(*dhq, icookie),
	       st->st_icookie, COOKIE_SIZE);

	ALLOC_WIRE_CHUNK(*dhq, rcookie, COOKIE_SIZE);
	memcpy(WIRE_CHUNK_PTR(*dhq, rcookie),
	       st->st_rcookie, COOKIE_SIZE);

	passert(dhq->oakley_group != OAKLEY_GROUP_invalid);
	return send_crypto_helper_request(st, dh);
}

bool finish_dh_secretiv(struct state *st,
			struct pluto_crypto_req *r)
{
	struct pcr_skeyid_r *dhr = &r->pcr_d.dhr;

	st->st_shared_nss = dhr->shared;
	st->st_skeyid_nss = dhr->skeyid;
	st->st_skeyid_d_nss = dhr->skeyid_d;
	st->st_skeyid_a_nss = dhr->skeyid_a;
	st->st_skeyid_e_nss = dhr->skeyid_e;
	st->st_enc_key_nss = dhr->enc_key;

	st->hidden_variables.st_skeyid_calculated = TRUE;

	if (st->st_shared_nss == NULL) {
		return FALSE;
	} else {
		passert(dhr->new_iv.len <= MAX_DIGEST_LEN);
		passert(dhr->new_iv.len > 0);
		memcpy(st->st_new_iv, WIRE_CHUNK_PTR(*dhr, new_iv), dhr->new_iv.len);
		st->st_new_iv_len = dhr->new_iv.len;
		return TRUE;
	}
}

stf_status start_dh_secret(struct pluto_crypto_req_cont *cn,
			   struct state *st,
			   enum crypto_importance importance,
			   enum original_role role,
			   const struct oakley_group_desc *oakley_group2)
{
	const chunk_t *pss = get_preshared_secret(st->st_connection);

	struct pcr_skeyid_q *const dhq = pcr_dh_init(cn, pcr_compute_dh, importance);

	passert(st->st_sec_in_use);

	/* convert appropriate data to dhq */
	dhq->auth = st->st_oakley.auth;
	dhq->prf = st->st_oakley.ta_prf;
	dhq->oakley_group = oakley_group2;
	dhq->role = role;
	dhq->key_size = st->st_oakley.enckeylen / BITS_PER_BYTE;
	dhq->salt_size = st->st_oakley.ta_encrypt->salt_size;

	if (pss != NULL)
		WIRE_CLONE_CHUNK(*dhq, pss, *pss);
	WIRE_CLONE_CHUNK(*dhq, ni, st->st_ni);
	WIRE_CLONE_CHUNK(*dhq, nr, st->st_nr);
	WIRE_CLONE_CHUNK(*dhq, gi, st->st_gi);
	WIRE_CLONE_CHUNK(*dhq, gr, st->st_gr);

	dhq->secret = st->st_sec_nss;

	/*copying required encryption algo*/
	/* XXX Avesh: you commented this out on purpose or by accident ?? */
	/*dhq->encrypter = st->st_oakley.ta_encrypt;*/
	DBG(DBG_CRYPT,
	    DBG_log("Copying DH pub key pointer to be sent to a thread helper"));
	dhq->pubk = st->st_pubk_nss;

	ALLOC_WIRE_CHUNK(*dhq, icookie, COOKIE_SIZE);
	memcpy(WIRE_CHUNK_PTR(*dhq, icookie),
	       st->st_icookie, COOKIE_SIZE);

	ALLOC_WIRE_CHUNK(*dhq, rcookie, COOKIE_SIZE);
	memcpy(WIRE_CHUNK_PTR(*dhq, rcookie),
	       st->st_rcookie, COOKIE_SIZE);

	return send_crypto_helper_request(st, cn);
}

/* NOTE: if NSS refuses to calculate DH, skr->shared == NULL */
/* MUST BE THREAD-SAFE */
void calc_dh(struct pluto_crypto_req *r)
{
	/* copy the request, since the reply will re-use the memory of the r->pcr_d.dhq */
	struct pcr_skeyid_q dhq;
	memcpy(&dhq, &r->pcr_d.dhq, sizeof(r->pcr_d.dhq));

	/* clear out the reply */
	struct pcr_skeyid_r *skr = &r->pcr_d.dhr;
	zero(skr);	/* ??? pointer fields might not be NULLed */
	INIT_WIRE_ARENA(*skr);

	const struct oakley_group_desc *group = dhq.oakley_group;
	passert(group != NULL);

	SECKEYPrivateKey *ltsecret = dhq.secret;
	SECKEYPublicKey *pubk = dhq.pubk;

	/* now calculate the (g^x)(g^y) */

	chunk_t g;

	setchunk_from_wire(g, &dhq, dhq.role == ORIGINAL_RESPONDER ? &dhq.gi : &dhq.gr);

	DBG(DBG_CRYPT, DBG_dump_chunk("peer's g: ", g));

	skr->shared = calc_dh_shared(g, ltsecret, group, pubk);
}

void finish_dh_secret(struct state *st,
		      struct pluto_crypto_req *r)
{
	struct pcr_skeyid_r *dhr = &r->pcr_d.dhr;

	st->st_shared_nss = dhr->shared;
}