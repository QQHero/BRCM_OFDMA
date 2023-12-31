/*
 * Implementation of wlc_key algo 'aes'
 * Copyright 2022 Broadcom
 *
 * This program is the proprietary software of Broadcom and/or
 * its licensors, and may only be used, duplicated, modified or distributed
 * pursuant to the terms and conditions of a separate, written license
 * agreement executed between you and Broadcom (an "Authorized License").
 * Except as set forth in an Authorized License, Broadcom grants no license
 * (express or implied), right to use, or waiver of any kind with respect to
 * the Software, and Broadcom expressly reserves all rights in and to the
 * Software and all intellectual property rights therein.  IF YOU HAVE NO
 * AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
 * WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
 * THE SOFTWARE.
 *
 * Except as expressly set forth in the Authorized License,
 *
 * 1. This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof, and to
 * use this information only in connection with your use of Broadcom
 * integrated circuit products.
 *
 * 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 * "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
 * OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 * 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
 * SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
 * IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
 * IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
 * ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
 * OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
 * NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
 * $Id: km_key_aes.c 803587 2021-09-30 07:59:23Z $
 */

#include "km_key_aes_pvt.h"
#ifdef WL_REUSE_KEY_SEQ
#include "wlfc_proto.h"
#endif
#include <wlc_frag.h>
/* internal interface */

/* BCMCCMP - s/w decr/encr for ccm */
#if !defined(BCMCCMP)
#define key_aes_rx_ccm(k, p, h, b, l, pkti) BCME_UNSUPPORTED
#define key_aes_tx_ccm(k, p, h, b, l) BCME_UNSUPPORTED
#endif /* !BCMGCMP */

/* BCMGCMP - s/w decr/encr for gcm */
#if !defined(BCMGCMP)
#define key_aes_rx_gcm(k, h, b, l, s, pkti) BCME_UNSUPPORTED
#define key_aes_tx_gcm(k, p, h, b, l, s) BCME_UNSUPPORTED
#endif /* !BCMGCMP */

#if defined(BCMDBG) && defined(BCMDBG_ASSERT)
/* check aes key for compatible algo vs (key, icv, iv) sizes
 * note: all gcm/gmac algorithms have longer icv
 */
static bool
key_aes_valid(const wlc_key_t *key)
{
	bool valid = FALSE;
	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
#ifdef MFP
	case CRYPTO_ALGO_BIP:
#endif /* MFP */
		valid = (key->info.key_len == AES_KEY_MIN_SIZE) &&
			(key->info.iv_len == AES_KEY_IV_SIZE) &&
			(key->info.icv_len == AES_KEY_MIN_ICV_SIZE);
		break;

	case CRYPTO_ALGO_AES_GCM:
	case CRYPTO_ALGO_BIP_GMAC:
		valid = (key->info.key_len == AES_KEY_MIN_SIZE) &&
			(key->info.iv_len == AES_KEY_IV_SIZE) &&
			(key->info.icv_len == AES_KEY_MAX_ICV_SIZE);
		break;

	case CRYPTO_ALGO_AES_CCM256:
#ifdef MFP
	case CRYPTO_ALGO_BIP_CMAC256:
#endif /* MFP */
	case CRYPTO_ALGO_AES_GCM256:
	case CRYPTO_ALGO_BIP_GMAC256:
		valid = (key->info.key_len == AES_KEY_MAX_SIZE) &&
			(key->info.iv_len == AES_KEY_IV_SIZE) &&
			(key->info.icv_len == AES_KEY_MAX_ICV_SIZE);
		break;

	case CRYPTO_ALGO_AES_OCB_MSDU:
	case CRYPTO_ALGO_AES_OCB_MPDU:
		valid = (key->info.key_len == AES_KEY_MIN_SIZE) &&
			(key->info.iv_len == DOT11_IV_AES_OCB_LEN) &&
			(key->info.icv_len == AES_KEY_MIN_ICV_SIZE);
		break;
	default:
		break;
	}
	return valid;
}
#define AES_KEY_VALID(_key) key_aes_valid(_key)
#else
#define AES_KEY_VALID(_key) AES_KEY_ALGO_VALID(_key)
#endif /* BCMDBG && BCMDBG_ASSERT */

static void
key_aes_seq_to_body(wlc_key_t *key, uint8 *seq, uint8 *body)
{
	if (!AES_KEY_ALGO_OCBXX(key)) {
		body[0] = seq[0];
		body[1] = seq[1];
		body[2] = 0;		/* reserved */
		body[3] = key->info.key_id << DOT11_KEY_INDEX_SHIFT | DOT11_EXT_IV_FLAG;
		body[4] = seq[2];
		body[5] = seq[3];
		body[6] = seq[4];
		body[7] = seq[5];
	} else {
		body[0] = seq[0];
		body[1] = seq[1];
		body[2] = seq[2];
		body[3] = key->info.key_id << DOT11_KEY_INDEX_SHIFT;
		body[3] |= seq[3] & 0xf;
	}
}

static void
key_aes_seq_from_body(wlc_key_t *key, uint8 *seq, const uint8 *body)
{
	if (!AES_KEY_ALGO_OCBXX(key)) {
		seq[0] = body[0];
		seq[1] = body[1];
		seq[2] = body[4];
		seq[3] = body[5];
		seq[4] = body[6];
		seq[5] = body[7];
	} else {
		seq[0] = body[0];
		seq[1] = body[1];
		seq[2] = body[2];
		seq[3] = body[3] & 0xf;	/* only 28 bits for ocb  */
	}
}

static int
key_aes_destroy(wlc_key_t *key)
{
	KM_DBG_ASSERT(AES_KEY_VALID(key));

	if (key->algo_impl.ctx != NULL) {
		MFREE(KEY_OSH(key), key->algo_impl.ctx, AES_KEY_STRUCT_SIZE(key));
	}

	key->algo_impl.cb = NULL;
	key->algo_impl.ctx = NULL;
	key->info.key_len = 0;
	key->info.iv_len = 0;
	key->info.icv_len = 0;
	return BCME_OK;
}

static int
key_aes_get_data(wlc_key_t *key, uint8 *data, size_t data_size,
	size_t *data_len, key_data_type_t data_type, int ins, bool tx)
{
	uint8 *key_data;
	int err = BCME_OK;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY:
		if (data_len != NULL) {
			*data_len = key->info.key_len;
		}

		if (data_size < key->info.key_len) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->key; /* MFP related */
		} else {
			key_data = ((aes_key_t *)key->algo_impl.ctx)->key;
		}
		memcpy(data, key_data, key->info.key_len);
		break;
	case WLC_KEY_DATA_TYPE_SEQ:
		if (data_len != NULL) {
			*data_len = AES_KEY_SEQ_SIZE;
		}

		if (data_size < AES_KEY_SEQ_SIZE) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->seq; /* MFP related */
		} else {
			aes_key_t *aes_key;
			if (!AES_KEY_VALID_INS(key, tx, ins)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			aes_key = (aes_key_t *)key->algo_impl.ctx;
			key_data = AES_KEY_SEQ(key, aes_key, tx, ins);
		}
		memcpy(data, key_data, AES_KEY_SEQ_SIZE);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

static int
key_aes_set_data(wlc_key_t *key, const uint8 *data,
    size_t data_len, key_data_type_t data_type, int seq_id, bool tx)
{
	uint8 *key_data;
	int err = BCME_OK;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	switch (data_type) {
	case WLC_KEY_DATA_TYPE_KEY:
		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->key; /* MFP related */
		} else {
			key_data = ((aes_key_t *)key->algo_impl.ctx)->key;
		}

		if (data_len != key->info.key_len) {
			if (!data_len) {
				memset(key_data, 0, key->info.key_len);
				break;
			}
			if (data_len < key->info.key_len) {
				err = BCME_BADLEN;
				break;
			}
			data_len = key->info.key_len;
		}

		if (!data) {
			err = BCME_BADARG;
			break;
		}

		if (!memcmp(key_data, data, key->info.key_len)) /* no change ? */ {
			err = BCME_REPLAY;
			break;
		}

		/* update the key. and re-init the tx/rx seq */
		memset(key->algo_impl.ctx, 0, AES_KEY_STRUCT_SIZE(key));

		/* aes legacy mode support */
		{
			scb_t *scb = wlc_keymgmt_get_scb(KEY_KM(key), key->info.key_idx);
			if (KM_SCB_LEGACY_AES(scb))
				key->info.flags |= WLC_KEY_FLAG_AES_MODE_LEGACY;
		}
		/* use ivtw (if configured), except for BIP and non WPA_NONE Group Key case */
#ifdef BRCMAPIVTW
		{
			wlc_bsscfg_t *cfg = wlc_keymgmt_get_bsscfg(KEY_KM(key), key->info.key_idx);
			if (!WLC_KEY_ALGO_IS_BIPXX(key->info.algo) &&
				(!WLC_KEY_IS_GROUP(&key->info) || (WLC_KEY_IS_IBSS(&key->info) &&
				(cfg->WPA_auth == WPA_AUTH_NONE))))
				key->info.flags |= WLC_KEY_FLAG_USE_IVTW; /* if enabled */
		}
#else
#endif /* BRCMAPIVTW */

		memcpy(key_data, data, key->info.key_len);
		break;
	case WLC_KEY_DATA_TYPE_SEQ:
		if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
			key_data = ((aes_igtk_t *)key->algo_impl.ctx)->seq; /* MFP */
		} else {
			aes_key_t *aes_key;
			if (!AES_KEY_VALID_INS(key, tx, seq_id)) {
				err = BCME_UNSUPPORTED;
				break;
			}
			aes_key = (aes_key_t *)key->algo_impl.ctx;
			key_data = AES_KEY_SEQ(key, aes_key, tx, seq_id);
		}

		if (!data_len) {
			memset(key_data, 0, AES_KEY_SEQ_SIZE);
			break;
		}

		if (!data) {
			err = BCME_BADARG;
			break;
		}

		if (data_len < AES_KEY_SEQ_SIZE) /* ocb modes */
			memset(key_data, 0, AES_KEY_SEQ_SIZE);
		else if (data_len > AES_KEY_SEQ_SIZE) /* truncate */
			data_len = AES_KEY_SEQ_SIZE;

		memcpy(key_data, data, data_len);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

#ifdef BCMCCMP
static uint8
key_aes_ccm_nonce_flags(wlc_key_t *key, void *pkt,
	const struct dot11_header *hdr, uint8 *body, bool htc, bool tx)
{
	uint16 fc;
	uint8 nonce_flags = 0;

	fc =  ltoh16(hdr->fc);

#if defined(MFP)
	if (FC_TYPE(fc) == FC_TYPE_MNG) {
		if (!tx) {
			scb_t *scb = WLPKTTAGSCBGET(pkt);
			KM_ASSERT(scb != NULL);
			if (KM_SCB_MFP(scb)) {
				nonce_flags = AES_CCMP_NF_MANAGEMENT;
			}
		} else if (WLPKTFLAG_MFP(WLPKTTAG(pkt))) {
			nonce_flags = AES_CCMP_NF_MANAGEMENT;
		}
	} else
#endif /* MFP */
	if (KM_KEY_FC_TYPE_QOS_DATA(fc) &&
		!(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY)) {
		uint16 qc;
		uint8 *qcp = body;
		if (htc)
			qcp -= DOT11_HTC_LEN;
		qcp -= DOT11_QOS_LEN;
		qc = ltoh16_ua(qcp);
		nonce_flags = (uint8)(QOS_TID(qc) & AES_CCMP_NF_PRIORITY);
	}

	return nonce_flags;
}

static int
key_aes_rx_ccm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, const key_pkt_info_t *pkt_info)
{
	int err = BCME_OK;
	size_t pkt_len;
	aes_key_t *aes_key;
	uint8 nonce_flags;
	uint32 rk[4*(AES_MAXROUNDS+1)];
	int st;

	KM_DBG_ASSERT(AES_KEY_ALGO_CCMXX(key));

	pkt_len = (body + body_len) - (uint8 *)hdr;
	nonce_flags = key_aes_ccm_nonce_flags(key, pkt, hdr, body,
		(pkt_info->flags & KEY_PKT_HTC_PRESENT) != 0, FALSE);
#ifdef WLFIPS
	if (FIPS_ENAB((KEY_WLC(key)))) {
		err = wl_fips_decrypt_pkt(KEY_WLC(key)->fips, key->info.key_id,
			hdr, pkt_len, nonce_flags);
	} else
#endif
	{
		aes_key = (aes_key_t *)key->algo_impl.ctx;
		if (!rijndaelKeySetupEnc(rk, aes_key->key, key->info.key_len << 3)) {
			return BCME_ENCERR;
		}
		st = aes_ccmp_decrypt(rk, key->info.key_len, pkt_len, (uint8 *)hdr,
			(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY), nonce_flags,
			key->info.icv_len);

		KM_DBG_ASSERT(st == AES_CCMP_DECRYPT_MIC_FAIL ||
			st == AES_CCMP_DECRYPT_SUCCESS);
		if (st != AES_CCMP_DECRYPT_SUCCESS) {
			err = BCME_DECERR;
		}
	}

	return err;
}
#endif /* BCMCCMP */

#ifdef BCMGCMP
static void
key_aes_gcm_calc_params(const struct dot11_header *hdr, uint8 *body,
	uint8 *nonce, size_t *nonce_len, uint8 *aad, size_t *aad_len, uint8 *seq, bool htc)
{
	uint8 *tmp;
	size_t i;
	uint16 fc;
	uint16 aad_fc, temp_seq;

	/* nonce - A2+PN. PN5 first 11adD9 */
	memcpy(nonce, hdr->a2.octet, ETHER_ADDR_LEN);
	tmp =  &nonce[ETHER_ADDR_LEN];
	for (i = AES_KEY_SEQ_SIZE; i > 0; --i)
		*(tmp++) = seq[i - 1];
	*nonce_len = tmp - nonce;

	/* process fc */
	fc =  ltoh16(hdr->fc);
	if (FC_TYPE(fc) == FC_TYPE_MNG)
		aad_fc = (fc & (FC_SUBTYPE_MASK | AES_CCMP_FC_MASK)); /* MFP */
	else
		aad_fc = (fc & AES_CCMP_FC_MASK); /* note: no legacy 11iD3 support */

	/* Mask out order bit in FC if HTC is present */
	if (htc) {
		aad_fc &= AES_CCMP_FC_HTC_MASK;
	}

	/* populate aad */
	tmp = aad;
	*tmp++ = aad_fc & 0xff;
	*tmp++ = aad_fc >> 8 & 0xff;
	memcpy(tmp, &hdr->a1, 3*ETHER_ADDR_LEN); /* copy A1, A2, A3 */
	tmp += 3*ETHER_ADDR_LEN;

	/* add masked seq control */
	temp_seq = ltoh16(hdr->seq) & AES_CCMP_SEQ_MASK; /* CCMP and GCMP have same mask */
	*tmp++ = (uint8)(temp_seq & 0xff);
	*tmp++ = (uint8)((temp_seq >> 8) & 0xff);

	if ((fc & (FC_TODS|FC_FROMDS)) == (FC_TODS|FC_FROMDS)) { /* wds */
		memcpy(tmp, &hdr->a4, ETHER_ADDR_LEN);
		tmp += ETHER_ADDR_LEN;
	}

	if (KM_KEY_FC_TYPE_QOS_DATA(fc)) {
		uint16 qc;
		uint8 *qcp = body;
		if (htc)
			qcp -= DOT11_HTC_LEN;
		qcp -= DOT11_QOS_LEN;
		qc = ltoh16_ua(qcp);
		qc &= AES_KEY_AAD_QOS_MASK;
		*tmp++ = qc & 0xff;
		*tmp++ = qc >> 8 & 0xff;
	}

	*aad_len = tmp - aad;
}

static int
key_aes_rx_gcm(wlc_key_t *key, struct dot11_header *hdr,
	uint8 *body, int body_len, uint8 *rx_seq, const key_pkt_info_t *pkt_info)
{
	aes_key_t *aes_key;
	uint8 nonce[AES_BLOCK_SZ];
	size_t nonce_len;
	uint8 aad[AES_KEY_AAD_MAX_SIZE];
	size_t aad_len;
	int st;
	STATIC_ASSERT((AES_KEY_SEQ_SIZE + ETHER_ADDR_LEN) <= AES_BLOCK_SZ);

	KM_DBG_ASSERT(AES_KEY_ALGO_GCMXX(key));
	KM_DBG_ASSERT(key->info.icv_len == AES_BLOCK_SZ);

	key_aes_gcm_calc_params(hdr, body, nonce, &nonce_len,
		aad, &aad_len, rx_seq, ((pkt_info->flags & KEY_PKT_HTC_PRESENT) != 0));
	aes_key = (aes_key_t *)key->algo_impl.ctx;
	st = aes_gcm_decrypt(aes_key->key, key->info.key_len,
		nonce, nonce_len, aad, aad_len,
		body + key->info.iv_len,
		body_len - (key->info.iv_len + key->info.icv_len),
		body + body_len - key->info.icv_len, key->info.icv_len);

	return st ? BCME_OK : BCME_DECERR;
}
#endif /* BCMGCMP */

static int
key_aes_rx_mpdu(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, const key_pkt_info_t *pkt_info)
{
	aes_key_t *aes_key;
	int err = BCME_OK;
	uint8 rx_seq[AES_KEY_SEQ_SIZE];
	key_seq_id_t ins = 0;
	uint16 fc;
	uint16 qc = 0;
	scb_t *scb;

	KM_DBG_ASSERT(AES_KEY_VALID(key) && pkt_info != NULL);

	aes_key = (aes_key_t *)key->algo_impl.ctx;
	scb = WLPKTTAGSCBGET(pkt);

	fc =  ltoh16(hdr->fc);
	if ((FC_TYPE(fc) == FC_TYPE_MNG) && ETHER_ISMULTI(&hdr->a1)) {
#if defined(MFP)
		if (KM_SCB_MFP(scb)) {
			err = km_key_aes_rx_mmpdu_mcmfp(key, pkt, hdr, body, body_len, pkt_info);
			goto done;
		}
#endif /* MFP */
	}

#ifdef BCMDBG
	/* uc mgmt or uc/mc data frames; no bip for these */
	if (AES_KEY_ALGO_BIPXX(key)) {
		err = BCME_UNSUPPORTED;
		goto done;
	}

	KM_ASSERT(!WLC_KEY_IS_MGMT_GROUP(&key->info));
#endif /* BCMDBG */

	/* ext iv bit must be set for all except ocb modes */
	if (!(body[KEY_ID_BODY_OFFSET] & DOT11_EXT_IV_FLAG) &&
		!AES_KEY_ALGO_OCBXX(key)) {
		KEY_LOG(("wl%d: %s: EXT IV is not set for pkt, key idx %d\n",
			KEY_WLUNIT(key), __FUNCTION__, key->info.key_idx));
		err = BCME_BADOPTION;
		goto done;
	}

	/* check replay with applicable counter */
	if (FC_TYPE(fc) == FC_TYPE_MNG) {
#ifdef MFP
		ins = KEY_NUM_RX_SEQ(key) - 1;
#else
		err = BCME_UNSUPPORTED;
		goto done;
#endif
	} else if (pkt_info->flags & KEY_PKT_QC_PRESENT) {
		qc = pkt_info->qc;
		ins = PRIO2IVIDX(QOS_PRIO(qc));
		KM_DBG_ASSERT(ins < KEY_NUM_RX_SEQ(key));
	}

	key_aes_seq_from_body(key, rx_seq, body);

	/* do replay check for first packet in chain (could be length of 1). if
	 * the decryption already failed, skip the replay check as we'll discard it
	 * later anyway
	 */
	if ((pkt_info->flags & KEY_PKT_PKTC_FIRST) && (pkt_info->status == BCME_OK)) {
		uint8 first_rx_seq[AES_KEY_SEQ_SIZE] = {0};
		uint8 *first_body = NULL;
		uint8 frag = ltoh16(hdr->seq) & FRAGNUM_MASK;
		uint64 first_seq, seq;

		if (km_is_replay(KEY_KM(key), &key->info, ins,
			AES_KEY_SEQ(key, aes_key, FALSE /* rx */, ins),
			rx_seq, AES_KEY_SEQ_SIZE)) {
			err = BCME_REPLAY;
			goto done;
		}

		if (frag) {
			first_body = wlc_defrag_first_frag_body_get(key->wlc, scb, QOS_PRIO(qc));
			if (first_body) {
				first_body -= key->info.iv_len;
				key_aes_seq_from_body(key, first_rx_seq, first_body);
				seq = KM_SEQ_TO_U64(rx_seq);
				first_seq = KM_SEQ_TO_U64(first_rx_seq);

				if (seq != (first_seq + frag)) {
					err = BCME_EPERM;
					goto done;
				}
			} else {
				err = BCME_EPERM;
				goto done;
			}
		}
	}

	/* nothing else for hw decrypt. caller checks hw decr status */
	if (pkt_info->flags & KEY_PKT_HWDEC) {
		err = pkt_info->status;
		goto done;
	}

	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
	case CRYPTO_ALGO_AES_CCM256:
		err = key_aes_rx_ccm(key, pkt, hdr, body, body_len, pkt_info);
		break;
	case CRYPTO_ALGO_AES_GCM:
	case CRYPTO_ALGO_AES_GCM256:
		err = key_aes_rx_gcm(key, hdr, body, body_len, rx_seq, pkt_info);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

done:
	if (err == BCME_OK) {
		/* on success, update rx iv in key for data & uc mgmt frames. for ivtw, the
		 * seq window is already updated.
		 */
		if (!WLC_KEY_USE_IVTW(&key->info)) {
			if ((FC_TYPE(fc) != FC_TYPE_MNG) || !ETHER_ISMULTI(&hdr->a1)) {
				memcpy(AES_KEY_SEQ(key, aes_key, FALSE /* rx */, ins),
					rx_seq, AES_KEY_SEQ_SIZE);
			}
		}
#ifdef BRCMAPIVTW
		else {
			bool chained;
			/* for intermediate frames in a chain, skip the iv update. for last, but
			 * not the first frame (chain of 1) do a chained update.
			 */
			chained = (pkt_info->flags & KEY_PKT_PKTC_LAST) &&
				!(pkt_info->flags & KEY_PKT_PKTC_FIRST);
			if (pkt_info->flags & (KEY_PKT_PKTC_FIRST | KEY_PKT_PKTC_LAST))
				KM_UPDATE_IVTW(KEY_KM(key), &key->info, ins, rx_seq,
					AES_KEY_SEQ_SIZE, chained);
		}
#endif
	} else { /* update counters */
		switch (err) {
		case BCME_REPLAY:
			WLCNTINCR(KEY_CNT(key)->ccmpreplay);
			if (ETHER_ISMULTI(&hdr->a1))
				WLCNTINCR(KEY_CNT(key)->ccmpreplay_mcst);

			break;

		case BCME_DECERR:
			WLCNTINCR(KEY_CNT(key)->ccmpfmterr);
			if (ETHER_ISMULTI(&hdr->a1))
				WLCNTINCR(KEY_CNT(key)->ccmpfmterr_mcst);

			/* fall through */

		default:
			WLCNTINCR(KEY_CNT(key)->ccmpundec);
			if (ETHER_ISMULTI(&hdr->a1))
				WLCNTINCR(KEY_CNT(key)->ccmpundec_mcst);
			break;
		}
	}

	return err;
}

#ifdef BCMCCMP
static int
key_aes_tx_ccm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len)
{
	int err = BCME_OK;
	size_t pkt_len;
	aes_key_t *aes_key;
	uint8 nonce_flags;
	uint16 fc, subtype;
	bool htc;
	uint32 rk[4 * (AES_MAXROUNDS + 1)];
	int st;

	pkt_len = (body + body_len) - (uint8 *)hdr;
	htc = FALSE;
	fc = ltoh16(hdr->fc);
	subtype = (fc & FC_SUBTYPE_MASK) >> FC_SUBTYPE_SHIFT;
	if ((FC_TYPE(fc) == FC_TYPE_DATA) && (subtype & FC_SUBTYPE_QOS_DATA)) {
		htc = ((fc & FC_ORDER) >> FC_ORDER_SHIFT);
	}
	nonce_flags = key_aes_ccm_nonce_flags(key, pkt, hdr, body, htc, TRUE);

#ifdef WLFIPS
	if (FIPS_ENAB((KEY_WLC(key)))) {
		err = wl_fips_encrypt_pkt(KEY_WLC(key)->fips, key->info.key_id,
			hdr, pkt_len, nonce_flags);
	} else
#endif
	{
		aes_key = (aes_key_t *)key->algo_impl.ctx;
		if (!rijndaelKeySetupEnc(rk, aes_key->key, key->info.key_len << 3)) {
			return BCME_ENCERR;
		}
		st = aes_ccmp_encrypt(rk, key->info.key_len, pkt_len, (uint8 *)hdr,
			(key->info.flags & WLC_KEY_FLAG_AES_MODE_LEGACY), nonce_flags,
			key->info.icv_len);

		if (st != AES_CCMP_ENCRYPT_SUCCESS) {
			err = BCME_ENCERR;
		}
	}

	return err;
}
#endif /* BCMCCMP */

#ifdef BCMGCMP
static int
key_aes_tx_gcm(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
    uint8 *body, int body_len, uint8 *tx_seq)
{
	aes_key_t *aes_key;
	uint8 nonce[AES_BLOCK_SZ];
	size_t nonce_len;
	uint8 aad[AES_KEY_AAD_MAX_SIZE];
	size_t aad_len;
	STATIC_ASSERT((AES_KEY_SEQ_SIZE + ETHER_ADDR_LEN) <= AES_BLOCK_SZ);

	KM_DBG_ASSERT(AES_KEY_ALGO_GCMXX(key));
	KM_DBG_ASSERT(key->info.icv_len == AES_BLOCK_SZ);

	key_aes_gcm_calc_params(hdr, body, nonce, &nonce_len,
		aad, &aad_len, tx_seq, FALSE);

	aes_key = (aes_key_t *)key->algo_impl.ctx;
	aes_gcm_encrypt(aes_key->key, key->info.key_len,
		nonce, nonce_len, aad, aad_len,
		body + key->info.iv_len,
		body_len - key->info.iv_len,
		body + body_len, key->info.icv_len);

	return BCME_OK;
}
#endif /* BCMGCMP */

#ifdef WL_REUSE_KEY_SEQ
static bool
key_aes_tx_mpdu_reuse_seq(wlc_key_t *key, void *pkt, uint8 *body)
{
	aes_key_t *aes_key;
	uint8 *key_seq;
#ifdef BCMDBG
	uint8 buf[8] = {0}, buf1[8] = {0};
	int j;
#endif
	key_seq = PKTFRAGFCTLV(KEY_OSH(key), pkt);
	if (!GET_DRV_HAS_ASSIGNED_KEY_SEQ(key_seq)) {
		return FALSE;
	}
	aes_key = (aes_key_t *)key->algo_impl.ctx;
	key_aes_seq_to_body(key, key_seq, body);
	RESET_DRV_HAS_ASSIGNED_KEY_SEQ(key_seq);
	SET_WL_HAS_ASSIGNED_KEY_SEQ(key_seq);
#ifdef BCMDBG
	WL_TRACE(("%s reuses key seq for d11 seq %02x => ",
		__FUNCTION__, WL_SEQ_GET_NUM(WLPKTTAG(pkt)->seq)));
	for (j = KEY_SEQ_SIZE; j > 0; --j) {
		WL_TRACE(("%02x", key_seq[j - 1]));
	}
	WL_TRACE((" tx_seq => "));
	key_aes_seq_to_body(key, aes_key->tx_seq, buf);
	key_aes_seq_from_body(key, buf1, buf);
	for (j = KEY_SEQ_SIZE; j > 0; --j) {
		WL_TRACE(("%02x", buf1[j - 1]));
	}
	WL_TRACE(("\n"));
#endif
	if (km_key_seq_less(key_seq, aes_key->tx_seq, KEY_SEQ_SIZE) ||
		!memcmp(key_seq, aes_key->tx_seq, KEY_SEQ_SIZE)) {
	} else {
		WL_TRACE(("%s invalid reused key seq. most likely due to rekeying\n",
			__FUNCTION__));
		return FALSE;
	}
	return TRUE;
}

static void
wlc_set_wl_has_assigned_key_seq(wlc_key_t *key, void *pkt, uint8 *body)
{
#ifdef BCMDBG
	int j;
#endif
	uint8 *key_seq;
	key_seq = PKTFRAGFCTLV(KEY_OSH(key), pkt);
	key_aes_seq_from_body(key, key_seq, body);
	SET_WL_HAS_ASSIGNED_KEY_SEQ(key_seq);
#ifdef BCMDBG
	WL_TRACE(("%s suppressed d11 seq %02x key seq => ", __FUNCTION__,
		WL_SEQ_GET_NUM(WLPKTTAG(pkt)->seq)));
	for (j = WL_KEY_SEQ_SIZE; j > 0; --j) {
		WL_TRACE(("%02x", key_seq[j - 1]));
	}
	WL_TRACE(("\n"));
#endif
	return;
}
#endif /* WL_REUSE_KEY_SEQ */

#ifdef WLCFP
/** Fast path code to do AES processing
 * 1. Increment sequence number
 * 2. update pkt body
 * 3. Update Tx descriptor cache info
 */
void
key_aes_tx_mpdu_fast(wlc_key_t *key, void *pkt, uint8 *body, d11txhdr_t *txd)
{
	aes_key_t	*aes_key;
	bool reusesseq = FALSE;

	aes_key = (aes_key_t *)key->algo_impl.ctx;
#ifdef WL_REUSE_KEY_SEQ
	reusesseq = key_aes_tx_mpdu_reuse_seq(key, pkt, body);
#endif
	if (!reusesseq) {
		/** Update the key sequence Number */
		if (!AES_KEY_ALGO_OCBXX(key)) {
			ASSERT(KEY_SEQ_IS_MAX(aes_key->tx_seq) == FALSE);
			KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
		} else {
			uint32 val = ltoh32_ua(aes_key->tx_seq);
			if (val >= ((1 << 28) - 3)) {
				memset(aes_key->tx_seq, 0, AES_KEY_SEQ_SIZE);
			} else {
				KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
			}
		}
		/* update iv in pkt - seq and key id  */
		key_aes_seq_to_body(key, aes_key->tx_seq, body);
#ifdef WL_REUSE_KEY_SEQ
		wlc_set_wl_has_assigned_key_seq(key, pkt, body);
#endif
	}

	if (KEY_COREREV_GE128(key)) {
		ASSERT(RATELINKMEM_ENAB(key->wlc->pub) == TRUE);
		/* nothing can be done here, link_entry should be ok already */
	} else {
		d11actxh_t *ac_txh = &txd->rev40;
		d11actxh_cache_t *cache_info;

		cache_info =
		        WLC_TXD_CACHE_INFO_GET(ac_txh, KEY_PUB(key)->corerev);

		/* Update TSCPn in tx descriptor */
		memcpy(cache_info->TSCPN, aes_key->tx_seq,
			MIN(sizeof(cache_info->TSCPN), AES_KEY_SEQ_SIZE));

		cache_info->common.BssIdEncAlg |= key->info.hw_algo << D11AC_ENCRYPT_ALG_SHIFT;
		cache_info->common.KeyIdx = (uint8)KM_KEY_HW_IDX_TO_SLOT(key, key->hw_idx);
	}
}
#endif /* WLCFP */
static int
key_aes_tx_mpdu(wlc_key_t *key, void *pkt, struct dot11_header *hdr,
	uint8 *body, int body_len, d11txhdr_t *txd)
{
	int err = BCME_OK;
	aes_key_t *aes_key;
	uint16 fc;
#if defined(BCMDBG) || defined(MFP_TEST)
	bool gen_err = FALSE;
#endif /* BCMDBG || MFP_TEST */
	bool reuse_seq = FALSE;

	KM_DBG_ASSERT(AES_KEY_VALID(key));

	fc =  ltoh16(hdr->fc);
	if ((FC_TYPE(fc) == FC_TYPE_MNG) && ETHER_ISMULTI(&hdr->a1)) {
		err = km_key_aes_tx_mmpdu_mcmfp(key, pkt, hdr, body, body_len, txd);
		goto done;
	}

	aes_key = (aes_key_t *)key->algo_impl.ctx;
#ifdef BCMDBG
	if (key->info.flags & WLC_KEY_FLAG_GEN_REPLAY) {
		key->info.flags &= ~WLC_KEY_FLAG_GEN_REPLAY;
		reuse_seq = FALSE;
	} else
#endif /* BCMDBG */
	 {
#ifdef WL_REUSE_KEY_SEQ
		reuse_seq = key_aes_tx_mpdu_reuse_seq(key, pkt, body);
#endif
		if (!reuse_seq) {
			if (!AES_KEY_ALGO_OCBXX(key)) {
				if (KEY_SEQ_IS_MAX(aes_key->tx_seq)) {
					err = BCME_BADKEYIDX;
					goto done;
				}
				KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
			} else {
				uint32 val = ltoh32_ua(aes_key->tx_seq);
				if (val >= ((1 << 28) - 3)) {
					memset(aes_key->tx_seq, 0, AES_KEY_SEQ_SIZE);
				} else {
					KEY_SEQ_INCR(aes_key->tx_seq, AES_KEY_SEQ_SIZE);
				}
			}
		}
	}

	if (!reuse_seq) {
		/* update iv in pkt - seq and key id  */
		key_aes_seq_to_body(key, aes_key->tx_seq, body);
#ifdef WL_REUSE_KEY_SEQ
		wlc_set_wl_has_assigned_key_seq(key, pkt, body);
#endif
	}

	if (WLC_KEY_IN_HW(&key->info) && txd != NULL) {
		if (KEY_COREREV_GE128(key)) {
			/* do nothing */
		} else if (KEY_COREREV_GE40(key)) {
			d11actxh_t * txh = &txd->rev40;
			d11actxh_cache_t	*cache_info;

			cache_info = WLC_TXD_CACHE_INFO_GET(txh, KEY_PUB(key)->corerev);
			memcpy(cache_info->TSCPN, aes_key->tx_seq,
				MIN(sizeof(cache_info->TSCPN), AES_KEY_SEQ_SIZE));
		} else {
			d11txh_pre40_t *txh = &txd->pre40;
			memcpy(txh->IV, body, key->info.iv_len);
		}

		goto done;
	}

#if defined(BCMDBG) || defined(MFP_TEST)
	if (key->info.flags & (WLC_KEY_FLAG_GEN_MFP_ACT_ERR|
		WLC_KEY_FLAG_GEN_MFP_DISASSOC_ERR |
		WLC_KEY_FLAG_GEN_MFP_DEAUTH_ERR)) {
		uint16 fk = (fc & FC_KIND_MASK);
		wlc_key_flags_t flags = key->info.flags;
		if ((flags & WLC_KEY_FLAG_GEN_MFP_ACT_ERR) && fk == FC_ACTION) {
			gen_err = TRUE;
			flags &= ~WLC_KEY_FLAG_GEN_MFP_ACT_ERR;
		}
		if ((flags & WLC_KEY_FLAG_GEN_MFP_DISASSOC_ERR) && fk == FC_DISASSOC) {
			gen_err = TRUE;
			flags &= ~WLC_KEY_FLAG_GEN_MFP_DISASSOC_ERR;
		}
		if ((flags & WLC_KEY_FLAG_GEN_MFP_DEAUTH_ERR) && fk == FC_DEAUTH) {
			gen_err = TRUE;
			flags &= ~WLC_KEY_FLAG_GEN_MFP_DEAUTH_ERR;
		}
		key->info.flags = flags;

		if (gen_err)
			aes_key->key[0] = ~aes_key->key[0];
	}

#endif /* BCMDBG || MFP_TEST */

	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
	case CRYPTO_ALGO_AES_CCM256:
		err = key_aes_tx_ccm(key, pkt, hdr, body, body_len);
		break;
	case CRYPTO_ALGO_AES_GCM:
	case CRYPTO_ALGO_AES_GCM256:
		err = key_aes_tx_gcm(key, pkt, hdr, body, body_len, aes_key->tx_seq);
		break;
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

#if defined(BCMDBG) || defined(MFP_TEST)
	if (gen_err)
		aes_key->key[0] = ~aes_key->key[0];
#endif /* BCMDBG || MFP_TEST */

done:
	return err;
}

#if defined(BCMDBG)
static int
key_aes_dump(const wlc_key_t *key, struct bcmstrbuf *b)
{
	size_t i;
	KM_DBG_ASSERT(AES_KEY_VALID(key));

	if (WLC_KEY_IS_MGMT_GROUP(&key->info)) {
		aes_igtk_t *aes_igtk;
		aes_igtk = (aes_igtk_t *)key->algo_impl.ctx;
		bcm_bprintf(b, "\taes igtk: ");
		for (i = 0; i < key->info.key_len; ++i)
			bcm_bprintf(b, "%02x", aes_igtk->key[i]);
		bcm_bprintf(b, "\n");

		bcm_bprintf(b, "\taes seq: 0x");
		for (i = AES_KEY_SEQ_SIZE; i > 0; --i)
			bcm_bprintf(b, "%02x", aes_igtk->seq[i-1]);
		bcm_bprintf(b, "\n");
	} else {
		aes_key_t *aes_key;
		aes_key = (aes_key_t *)key->algo_impl.ctx;

		bcm_bprintf(b, "\taes key: ");
		for (i = 0; i < key->info.key_len; ++i)
			bcm_bprintf(b, "%02x", aes_key->key[i]);
		bcm_bprintf(b, "\n");

		bcm_bprintf(b, "\taes tx_seq: 0x");
		for (i = AES_KEY_SEQ_SIZE; i > 0; --i)
			bcm_bprintf(b, "%02x", aes_key->tx_seq[i-1]);
		bcm_bprintf(b, "\n");

		bcm_bprintf(b, "\taes rx_seq:\n");
		for (i = 0; i < (size_t)KEY_NUM_RX_SEQ(key); ++i) {
			size_t j;
			uint8 *rx_seq;

			bcm_bprintf(b, "\t\t%02d: 0x", i);
			rx_seq = AES_KEY_RX_SEQ(key, aes_key, i);
			for (j = AES_KEY_SEQ_SIZE; j > 0; --j)
				bcm_bprintf(b, "%02x", rx_seq[j - 1]);

			bcm_bprintf(b, "\n");
		}
	}
	return BCME_OK;
}

#define KEY_AES_DUMP key_aes_dump
#else
#define KEY_AES_DUMP NULL
#endif

static const key_algo_callbacks_t
km_key_aes_callbacks = {
    key_aes_destroy,	/* destroy */
    key_aes_get_data,	/* get data */
    key_aes_set_data,	/* set data */
    key_aes_rx_mpdu,	/* rx mpdu */
    NULL,				/* rx msdu */
    key_aes_tx_mpdu,	/* tx mpdu */
    NULL,				/* tx msdu */
    KEY_AES_DUMP		/* dump */
};

/* public interface */
int
km_key_aes_init(wlc_key_t *key)
{
	int err = BCME_OK;

	KM_DBG_ASSERT(key != NULL && AES_KEY_ALGO_VALID(key));

	switch (key->info.algo) {
	case CRYPTO_ALGO_AES_CCM:
#ifdef MFP
	case CRYPTO_ALGO_BIP:
#endif /* MFP */
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MIN_ICV_SIZE;
		break;

	case CRYPTO_ALGO_AES_GCM: /* 802.11adD9 */
#ifdef MFP
	case CRYPTO_ALGO_BIP_GMAC:
#endif /* MFP */
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MAX_ICV_SIZE;
		break;

	case CRYPTO_ALGO_AES_CCM256:
#ifdef MFP
	case CRYPTO_ALGO_BIP_CMAC256:
#endif /* MFP */
	case CRYPTO_ALGO_AES_GCM256:
	case CRYPTO_ALGO_BIP_GMAC256:
		key->info.key_len = AES_KEY_MAX_SIZE;
		key->info.iv_len = AES_KEY_IV_SIZE;
		key->info.icv_len = AES_KEY_MAX_ICV_SIZE;
		break;
	case CRYPTO_ALGO_AES_OCB_MSDU:
	case CRYPTO_ALGO_AES_OCB_MPDU:
		key->info.key_len = AES_KEY_MIN_SIZE;
		key->info.iv_len = DOT11_IV_AES_OCB_LEN;
		key->info.icv_len = AES_KEY_MIN_ICV_SIZE;
		break;
	default:
		key->algo_impl.cb = NULL;
		key->algo_impl.ctx = NULL;
		err = BCME_UNSUPPORTED;
		break;
	}

	if (err != BCME_OK)
		goto done;

	key->algo_impl.cb = &km_key_aes_callbacks;
	key->algo_impl.ctx = MALLOCZ(KEY_OSH(key), AES_KEY_STRUCT_SIZE(key));
	err = (key->algo_impl.ctx != NULL) ? BCME_OK : BCME_NOMEM;
done:
	return err;
}
