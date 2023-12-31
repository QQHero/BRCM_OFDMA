/**
 * @file
 * @brief
 * CCA ((Clear Channel Assessment, an 802.11 std term) stats module source file
 * Broadcom 802.11abgn Networking Device Driver
 *
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
 *
 * $Id: wlc_cca.c 799441 2021-05-31 03:43:41Z $
 */

/**
 * @file
 * @brief
 * Assists in channel (re)selection and interference mitigation
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>

#ifdef CCA_STATS

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <wlioctl.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_ap.h>
#include <wlc_cca.h>
#include <wlc_bmac.h>
#include <wlc_assoc.h>
#include <wl_export.h>
#include <wlc_lq.h>
#include <wlc_event_utils.h>
#include <wlc_pm.h>
#include <wlc_dump.h>

#ifdef WL_CCA_STATS_MESH
#include <wlc_addrmatch.h>
#include <wlc_keymgmt.h>

#define CCA_STATS_MESH_VER 1
#define GET_CCA_NBITVAL(wlc, amtinfo)	(D11REV_GE(wlc->pub->corerev, 128) ?\
		(amtinfo & NBITVAL(C_ADDR_MESHCCA_GE128_NBIT)) :\
		(amtinfo & NBITVAL(C_ADDR_MESHCCA_LT128_NBIT)))
#define SET_CCA_NBITVAL(wlc, amtinfo)	(D11REV_GE(wlc->pub->corerev, 128) ?\
		(amtinfo |= NBITVAL(C_ADDR_MESHCCA_GE128_NBIT)) :\
		(amtinfo |= NBITVAL(C_ADDR_MESHCCA_LT128_NBIT)))
#define CLR_CCA_NBITVAL(wlc, amtinfo)	(D11REV_GE(wlc->pub->corerev, 128) ?\
		(amtinfo &= ~NBITVAL(C_ADDR_MESHCCA_GE128_NBIT)) :\
		(amtinfo &= ~NBITVAL(C_ADDR_MESHCCA_LT128_NBIT)))

#endif /* WL_CCA_STATS_MESH */

/* IOVar table */
/* No ordering is imposed */
enum {
	IOV_CCA_STATS = 0,      /* Dump cca stats */
	IOV_CHAN_QUAL_EVENT = 1,
#ifdef WL_CCA_STATS_MESH
	IOV_CCA_STATS_MESH, /* Dump mesh cca stats */
#endif /* WL_CCA_STATS_MESH */
	IOV_LAST
};

static const bcm_iovar_t wlc_cca_iovars[] = {
	{"cca_get_stats", IOV_CCA_STATS,
	(0), 0, IOVT_BUFFER, sizeof(cca_congest_channel_req_t),
	},
	{"chq_event", IOV_CHAN_QUAL_EVENT,
	(0), 0, IOVT_BUFFER, sizeof(wl_chan_qual_event_t)
	},
#ifdef WL_CCA_STATS_MESH
	{"cca_stats_mesh", IOV_CCA_STATS_MESH,
	(0), 0, IOVT_BUFFER, sizeof(cca_mesh_req_t),
	},
#endif /* WL_CCA_STATS_MESH */
	{NULL, 0, 0, 0, 0, 0}
};

static int wlc_cca_doiovar(void *ctx, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif);

#ifndef DONGLEBUILD
static void cca_alloc_pool(cca_info_t *cca, int ch_idx, int second);
static void cca_free_pool(cca_info_t *cca, int ch_idx, int second);
#endif
static int cca_chanspec_to_index(cca_info_t *cca, chanspec_t chanspec);
static int cca_reset_stats(void *ctx);
static void cca_stats_watchdog(void *ctx);
static int cca_get_stats(cca_info_t *cca, void *input, int buf_len, void *output);
#ifdef WL_CCA_STATS_MESH
static int cca_stats_mesh(cca_info_t *cca, void *input);
#endif /* WL_CCA_STATS_MESH */

static void wlc_cca_chan_qual_event_timeout(void *arg);

#ifdef DONGLEBUILD
#define CCA_POOL_MAX(cca)	((cca)->n_chan_stats)	/* trimmed to fit dongle */
#else /* DONGLEBUILD */
#ifdef ISID_STATS
#define CCA_POOL_MAX(cca)	(450)	/* bigger pool for interference samples */
#else
#define CCA_POOL_MAX(cca)	(300)
#endif
#endif /* DONGLEBUILD */
#define CCA_FREE_BUF		0xffff

typedef uint16 cca_idx_t;
typedef struct {
	chanspec_t chanspec;
#ifndef DONGLEBUILD
	cca_idx_t  secs[MAX_CCA_SECS];
#endif /* DONGLEBUILD */
} cca_congest_channel_t;

struct cca_info {
	wlc_info_t *wlc;
	cca_ucode_counts_t last_cca_stats;	/* Previously read values, for computing deltas */
	cca_congest_channel_t *chan_stats;
	uint		n_chan_stats;		/* num of entries for chan_stats */
	int             cca_second;		/* which second bucket we are using */
	int             cca_second_max;		/* num of seconds to track */
	int		alloc_fail;
	wlc_congest_t	*cca_pool;
#ifdef WL_CCA_STATS_MESH
	uint8		mesh_count;
	bool		cca_mesh_enable;
#endif /* WL_CCA_STATS_MESH */
};

#ifndef DONGLEBUILD
#define CCA_POOL_DATA(cca, chanspec, second) \
	(&(cca->cca_pool[cca->chan_stats[chanspec].secs[second]]))
#define CCA_POOL_IDX(cca, chanspec, second) \
	(cca->chan_stats[chanspec].secs[second])
#else
#define CCA_POOL_DATA(cca, chanspec, second) (&(cca->cca_pool[chanspec]))
#define CCA_POOL_IDX(cca, chanspec, second) (chanspec)
#endif /* DONGLEBUILD */

#define CCA_MODULE_NAME "cca_stats"

struct cca_chan_qual {
	wl_chan_qual_event_t event;
	struct wl_timer *cca_event_timer;	/**< timer to limit event notifications */
	bool	is_cca_event_timer_active;	/**< flag to indicate timer active */
	uint8 level[WL_CHAN_QUAL_TOTAL];
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

#ifdef WL_CCA_STATS_MESH
/*
 * Function to reserve the required AMT indices for cca_stats_mesh
 * As one of the indices is fixed to 63, we have to reserve AMT idx
 * for 7 more. Starting from 51 to 57 if available are fixed for
 * CCA_STATS_MESH
 */
static int
wlc_cca_mesh_amt_reserve(wlc_info_t *wlc)
{
	wlc_keymgmt_t *km = wlc->keymgmt;
	uint8 amt_idx;
	//check if the required AMT indices are free.
	for (amt_idx = AMT_IDX_CCA_MESH_START; amt_idx <= AMT_IDX_CCA_MESH_END; amt_idx++) {
		if (wlc_keymgmt_amt_idx_isset(km, amt_idx)) {
			break;
		}
	}
	if (amt_idx < AMT_IDX_CCA_MESH_END) {
		WL_ERROR(("wl%d: amt indices already reserved\n",
			wlc->pub->unit));
		return BCME_NORESOURCE;
	}
	//reserve the indices in the AMT table.
	wlc_keymgmt_amt_reserve(km, AMT_IDX_CCA_MESH_START, (CCA_MESH_MAX - 1), TRUE);
	return BCME_OK;
}
/*
 * Function to release the amt indices occupied by cca_stats_mesh
 */
static int
wlc_cca_mesh_amt_release(wlc_info_t *wlc)
{
	wlc_keymgmt_t *km = wlc->keymgmt;
	uint8 amt_idx;
	//check if the required AMT indices are not free.
	for (amt_idx = AMT_IDX_CCA_MESH_START; amt_idx <= AMT_IDX_CCA_MESH_END; amt_idx++) {
		if (wlc_keymgmt_amt_idx_isset(km, amt_idx)) {
			wlc_clear_addrmatch(wlc, amt_idx);
		} else {
			break;
		}
	}
	if (amt_idx < AMT_IDX_CCA_MESH_END) {
		WL_ERROR(("wl%d: amt indices already released\n",
			wlc->pub->unit));
		return BCME_NORESOURCE;
	}
	wlc_keymgmt_amt_reserve(km, AMT_IDX_CCA_MESH_START, (CCA_MESH_MAX - 1), FALSE);

	return BCME_OK;
}
#endif /* WL_CCA_STATS_MESH */

static uint
BCMATTACHFN(wlc_cca_channels_num)(wlc_info_t *wlc)
{
	struct wlcband *band;
	enum wlc_bandunit bandunit;
	int i = 0, channel;
	FOREACH_WLC_BAND(wlc, bandunit) {
		band = wlc->bandstate[bandunit];
		FOREACH_WLC_BAND_CHANNEL20(band, channel) {
			i++;
		}
	}
	return i;
}

cca_info_t *
BCMATTACHFN(wlc_cca_attach)(wlc_info_t *wlc)
{
	int i;
	cca_info_t *cca = NULL;
	struct wlcband *band;
	enum wlc_bandunit bandunit;
	chanspec_t chanspec;
	uint8 chan;

	if ((cca = MALLOCZ(wlc->osh, sizeof(cca_info_t))) == NULL) {
		goto nomem;
	}
	if ((wlc->cca_chan_qual = MALLOCZ(wlc->osh, sizeof(cca_chan_qual_t))) == NULL) {
		goto nomem;
	}
	cca->wlc = wlc;
	cca->cca_second_max = MAX_CCA_SECS;
	cca->cca_second = 0;
	bzero(&cca->last_cca_stats, sizeof(cca->last_cca_stats));

	cca->n_chan_stats = wlc_cca_channels_num(wlc);
	cca->chan_stats = MALLOCZ(wlc->osh, cca->n_chan_stats * sizeof(cca_congest_channel_t));
	if (cca->chan_stats == NULL) {
		goto nomem;
	}
	cca->cca_pool = MALLOCZ(wlc->osh, CCA_POOL_MAX(cca) * sizeof(wlc_congest_t));
	if (cca->cca_pool == NULL) {
		goto nomem;
	}
	i = 0;
	FOREACH_WLC_BAND(wlc, bandunit) {
		band = wlc->bandstate[bandunit];

		FOREACH_WLC_BAND_CHANNEL20(band, chan) {
			chanspec = wf_create_20MHz_chspec(chan, wlc_bandunit2chspecband(bandunit));
			cca->chan_stats[i].chanspec = chanspec;
			i++;
		}
	}
	for (i = 0; i < CCA_POOL_MAX(cca); i++)
		cca->cca_pool[i].congest_ibss = CCA_FREE_BUF;

	if (wlc_module_register(wlc->pub, wlc_cca_iovars, CCA_MODULE_NAME,
	    (void *)cca, wlc_cca_doiovar, cca_stats_watchdog, cca_reset_stats,
	    NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
		wlc->pub->unit, __FUNCTION__));
		goto fail;
	};
	wlc->pub->_cca_stats = TRUE;
	return cca;
nomem:
	WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
		wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
fail:
	if (wlc->cca_chan_qual != NULL)
		MFREE(wlc->osh, wlc->cca_chan_qual, sizeof(cca_chan_qual_t));
	if (cca->cca_pool != NULL)
		MFREE(wlc->osh, cca->cca_pool, CCA_POOL_MAX(cca) * sizeof(wlc_congest_t));
	if (cca->chan_stats != NULL)
		MFREE(wlc->osh, cca->chan_stats, cca->n_chan_stats * sizeof(cca_congest_channel_t));
	if (cca != NULL)
		MFREE(wlc->osh, cca, sizeof(cca_info_t));
	return NULL;
}

void
BCMATTACHFN(wlc_cca_detach)(cca_info_t *cca)
{
	wlc_info_t *wlc = cca->wlc;
	cca_chan_qual_t *chq = wlc->cca_chan_qual;

	wlc_module_unregister(wlc->pub, CCA_MODULE_NAME, cca);

	if (chq && chq->cca_event_timer) {
		wl_del_timer(wlc->wl, chq->cca_event_timer);
		wl_free_timer(wlc->wl, chq->cca_event_timer);
		chq->cca_event_timer = NULL;
	}

	if (wlc->cca_chan_qual != NULL)
		MFREE(wlc->osh, wlc->cca_chan_qual, sizeof(cca_chan_qual_t));
	MFREE(wlc->osh, cca->cca_pool, CCA_POOL_MAX(cca) * sizeof(wlc_congest_t));
	MFREE(wlc->osh, cca->chan_stats, cca->n_chan_stats * sizeof(cca_congest_channel_t));
	MFREE(wlc->osh, cca, sizeof(cca_info_t));
}

static int
wlc_cca_doiovar(void *ctx, uint32 actionid,
        void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
	cca_info_t *cca = (cca_info_t *)ctx;
	wlc_info_t *wlc = cca->wlc;
	int err = 0;
	int32 int_val = 0;
	int32 *ret_int_ptr;
	bool bool_val;

	BCM_REFERENCE(val_size);
	BCM_REFERENCE(wlcif);

	/* convenience int and bool vals for first 4 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)arg;
	BCM_REFERENCE(ret_int_ptr);

	bool_val = (int_val != 0) ? TRUE : FALSE;
	BCM_REFERENCE(bool_val);

	switch (actionid) {
	case IOV_GVAL(IOV_CCA_STATS):
		if ((p_len < sizeof(cca_congest_channel_req_t)) ||
		    (len < (int)sizeof(cca_congest_channel_req_t)))
			err = BCME_BUFTOOSHORT;
		else
			err = cca_get_stats(cca, params, len, arg);
		break;

	case IOV_GVAL(IOV_CHAN_QUAL_EVENT):
		if (wlc->cca_chan_qual == NULL)
			return BCME_NOTREADY;
		memcpy(arg, &(wlc->cca_chan_qual->event), sizeof(wl_chan_qual_event_t));
		break;

	case IOV_SVAL(IOV_CHAN_QUAL_EVENT): {
		cca_chan_qual_t *chq = wlc->cca_chan_qual;
		if (chq == NULL)
			return BCME_NOTREADY;
		if (chq->cca_event_timer) {
			wl_del_timer(wlc->wl, chq->cca_event_timer);
			wl_free_timer(wlc->wl, chq->cca_event_timer);
			chq->cca_event_timer = NULL;
		}
		chq->is_cca_event_timer_active = FALSE;
		memset(chq->level, 0, sizeof(chq->level));	/* reset all current levels */
		memcpy(&(chq->event), arg, sizeof(wl_chan_qual_event_t));
		if (chq->event.rate_limit_msec) {
			chq->cca_event_timer = wl_init_timer(wlc->wl,
				wlc_cca_chan_qual_event_timeout, wlc, "chan_qual_event");
		}
		break;
	}
#ifdef WL_CCA_STATS_MESH
	case IOV_GVAL(IOV_CCA_STATS_MESH):
		if ((p_len < sizeof(cca_mesh_req_t)) ||
				(len < (int)sizeof(cca_mesh_req_t))) {
			err = BCME_BUFTOOSHORT;
		} else {
			cca_mesh_req_t *stats_req = (cca_mesh_req_t *) params;
			cca_mesh_req_t *stats_res = (cca_mesh_req_t *) arg;
			cca_congest_channel_req_t *input = &stats_req->data;
			cca_congest_channel_req_t *output = &stats_res->data;

			if (!cca->cca_mesh_enable) {
				WL_INFORM(("wl%d: %s: cca_stats_mesh disabled\n",
					cca->wlc->pub->unit, __FUNCTION__));
				err = BCME_DISABLED;
				break;
			}
			stats_res->count = cca->mesh_count;
			stats_res->ver = CCA_STATS_MESH_VER;
			if (stats_req->cmd_type == CCA_MESH_DUMP) {
				struct ether_addr tmp_ea;
				uint16 tmp_attr;
				uint16 prev_amtinfo;
				int i;
				struct ether_addr *ptr_ea = (struct ether_addr *)&stats_res->data;

				/* check special entry for self-MAC */
				wlc_get_addrmatch(cca->wlc, AMT_IDX_MAC, &tmp_ea, &tmp_attr);
				prev_amtinfo = wlc_read_amtinfo_by_idx(cca->wlc, AMT_IDX_MAC);
				if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
					ether_copy(&tmp_ea, ptr_ea);
				} else {
					ether_copy(&ether_null, ptr_ea);
				}
				ptr_ea++;
				/* check entries for generic MAC / (NOT self) */
				for (i = AMT_IDX_CCA_MESH_START; i <= AMT_IDX_CCA_MESH_END; i++) {
					wlc_get_addrmatch(cca->wlc, i, &tmp_ea, &tmp_attr);
					prev_amtinfo = wlc_read_amtinfo_by_idx(cca->wlc, i);
					if ((tmp_attr == ((AMT_ATTR_A2 | AMT_ATTR_A1 |
							AMT_ATTR_VALID))) &&
							GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
						ether_copy(&tmp_ea, ptr_ea);
						ptr_ea++;
					} else {
						ether_copy(&ether_null, ptr_ea);
						ptr_ea++;
					}
				}
			} else {
				err = cca_get_stats(cca, input, len, output);
			}
		}
		break;
	case IOV_SVAL(IOV_CCA_STATS_MESH):
		if ((p_len < sizeof(cca_congest_channel_req_t)) ||
		    (len < (int)sizeof(cca_congest_channel_req_t))) {
			err = BCME_BUFTOOSHORT;
		} else {
			err = cca_stats_mesh(cca, params);
		}
		break;
#endif /* WL_CCA_STATS_MESH */

	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
}

#ifndef DONGLEBUILD
/* Setup a new second for this chanspec_idx */
static void
cca_alloc_pool(cca_info_t *cca, int ch_idx, int second)
{
	int i;

	/* The zero'th entry is reserved, Its like a NULL pointer, give it out for failure */
	for (i = 1; i < CCA_POOL_MAX(cca) && cca->cca_pool[i].congest_ibss != CCA_FREE_BUF; i++)
		;
	if (i == CCA_POOL_MAX(cca)) {
		WL_INFORM(("%s: allocate an entry failed!\n", __FUNCTION__));
		/* Just leave the current bucket in place, nothing else we can do */
		/* Wait til watchdog ages out soem buckets */
		cca->alloc_fail++;
		return;
	}
#ifdef BCMDBG
	if (cca->cca_pool[i].congest_ibss != CCA_FREE_BUF)
		WL_ERROR(("%s:  NULL IDX but not CCA_FREE_BUF ch_idx = %d, dur = 0x%x\n",
			__FUNCTION__, i, cca->cca_pool[i].congest_ibss));
#endif
	bzero(&cca->cca_pool[i], sizeof(wlc_congest_t));
	CCA_POOL_IDX(cca, ch_idx, second) = (cca_idx_t)i & 0xffff;
	return;
}

/* Delete this second from given chanspec_idx */
static void
cca_free_pool(cca_info_t *cca, int ch_idx, int second)
{
	cca_idx_t pool_index = CCA_POOL_IDX(cca, ch_idx, second);
#ifdef BCMDBG
	if (cca->cca_pool[pool_index].congest_ibss == CCA_FREE_BUF)
		WL_ERROR(("%s: Freeing a free buffer\n", __FUNCTION__));
#endif
	cca->cca_pool[pool_index].congest_ibss = CCA_FREE_BUF;
	CCA_POOL_IDX(cca, ch_idx, second) = 0;
}
#endif /* DONGLEBUILD */

static int
cca_chanspec_to_index(cca_info_t *cca, chanspec_t chanspec)
{
	int i;

	for (i = 0; i < cca->n_chan_stats; i++) {
		if (cca->chan_stats[i].chanspec == chanspec)
			return (i);
	}
	return (-1);
}

uint
wlc_cca_get_channels_num(cca_info_t *cca)
{
	if (!cca)
		return 0;
	return cca->n_chan_stats;
}

chanspec_t
wlc_cca_get_chanspec(cca_info_t *cca, int index)
{
	if (!cca || index >= cca->n_chan_stats)
		return 0;
	return cca->chan_stats[index].chanspec;
}

void
cca_stats_tsf_upd(wlc_info_t *wlc)
{
	uint32 tsf_l, tsf_h;
	cca_info_t *cca = wlc->cca_info;

	if (!cca)
		return;

	wlc_read_tsf(wlc, &tsf_l, &tsf_h);
	cca->last_cca_stats.usecs = tsf_l;
}

static int
cca_reset_stats(void *ctx)
{
	int secs;
	chanspec_t chanspec;
	cca_info_t *cca = (cca_info_t *)ctx;

	if (!cca)
		return BCME_OK;
	for (secs = 0; secs < cca->cca_second_max; secs++) {
		for (chanspec = 0; chanspec < cca->n_chan_stats; chanspec++) {
#ifndef DONGLEBUILD
			if (CCA_POOL_IDX(cca, chanspec, secs) != 0)
				cca_free_pool(cca, chanspec, secs);
#else /* DONGLEBUILD */
			bzero(&cca->cca_pool[chanspec], sizeof(wlc_congest_t));
#endif /* DONGLEBUILD */
		}
	}
#ifdef WL_CCA_STATS_MESH
	if (cca->cca_mesh_enable) {
		wlc_cca_mesh_amt_release(cca->wlc);
		cca->cca_mesh_enable = FALSE;
	}
#endif /* WL_CCA_STATS_MESH */
	return BCME_OK;
}

static void
cca_stats_watchdog(void *ctx)
{
	cca_info_t *cca = (cca_info_t *)ctx;
	int ch_idx, i;
	chanspec_t chanspec;
	wlc_info_t *wlc;

	if (!cca)
		return;

	ASSERT(cca->wlc);
	wlc = cca->wlc;
	chanspec = wf_chspec_primary20_chspec(wlc->chanspec);

	/* Bump the global 'second' pointer */
	cca->cca_second = MODINC(cca->cca_second, cca->cca_second_max);

	if ((ch_idx = cca_chanspec_to_index(cca, chanspec)) < 0) {
		WL_ERROR(("%s: Bad chanspec; 0x%x!!\n", __FUNCTION__, chanspec));
		return;
	}

#ifndef DONGLEBUILD
	/* The 'seconds' buffer wraps, so if we are coming to this particular
	   second again, free the previous contents.  Essentially this frees
	   buffers that are cca->cca_second_max seconds old
	*/
	for (i = 0; i < cca->n_chan_stats; i++) {
		if (CCA_POOL_IDX(cca, i, cca->cca_second) != 0)
			cca_free_pool(cca, i, cca->cca_second);
	}
#else /* DONGLEBUILD */
#define CCA_ALPHA 96	/* 0.4 */
	/* Age statistics by ~90% per second */
	for (i = 0; i < cca->n_chan_stats; i++) {
		wlc_congest_t *stats = CCA_POOL_DATA(cca, i, cca->cca_second);
		int j;
		if (stats->duration == 0)
			continue;

		stats->duration = (stats->duration * CCA_ALPHA) >> 8;
		stats->congest_ibss = (stats->congest_ibss * CCA_ALPHA) >> 8;
		stats->congest_obss = (stats->congest_obss * CCA_ALPHA) >> 8;
		stats->interference = (stats->interference * CCA_ALPHA) >> 8;
#ifdef ISID_STATS
		stats->crsglitch = (stats->crsglitch * CCA_ALPHA) >> 8;
		stats->badplcp = (stats->badplcp * CCA_ALPHA) >> 8;
		stats->bphy_crsglitch = (stats->bphy_crsglitch * CCA_ALPHA) >> 8;
		stats->bphy_badplcp = (stats->bphy_badplcp * CCA_ALPHA) >> 8;
#endif /* ISID_STATS */
		for (j = 0; j < CCA_MESH_MAX; j++) {
			stats->txnode[j] = (stats->txnode[j] * CCA_ALPHA) >> 8;
			stats->rxnode[j] = (stats->rxnode[j] * CCA_ALPHA) >> 8;
		}
		stats->xxobss = (stats->xxobss * CCA_ALPHA) >> 8;
	}
#endif /* DONGLEBUILD */

	cca_stats_upd(wlc, 1);
	cca_send_event(wlc, 0);
}

int
cca_query_stats(wlc_info_t *wlc, chanspec_t chanspec, int nsecs,
	wlc_congest_channel_req_t *stats_results, int buflen)
{
	int secs_done, ch_idx, second;
	wlc_congest_t *congest;
	cca_info_t *cca;
	int i;

	cca = wlc->cca_info;
	if (!cca)
		return 0;

	second = cca->cca_second;
	nsecs = MIN(cca->cca_second_max, nsecs);

	if ((ch_idx = cca_chanspec_to_index(cca, chanspec)) < 0) {
		stats_results->num_secs = 0;
		stats_results->chanspec = 0;
		return 0;
	}

	stats_results->chanspec = chanspec;
	buflen -= OFFSETOF(cca_congest_channel_req_t, secs);

	/* Retreive the last x secs of measurements */
	for (secs_done = 0; (secs_done < nsecs) && (uint)buflen >= sizeof(wlc_congest_t);
			secs_done++) {
		second = MODDEC(second, cca->cca_second_max);

		/* If the entry for this second/channel is empty, CCA_POOL_IDX
		   will be zero, and CCA_POOL_DATA will be the zero'th entry
		   which we keep empty for this purpose
		*/
		congest = CCA_POOL_DATA(cca, ch_idx, second);

		stats_results->secs[secs_done].duration =
			(congest->duration + 500)/1000;
		stats_results->secs[secs_done].congest_ibss =
			(congest->congest_ibss + 500)/1000;
		stats_results->secs[secs_done].congest_obss =
			(congest->congest_obss + 500)/1000;
		stats_results->secs[secs_done].interference =
			(congest->interference + 500)/1000;
		stats_results->secs[secs_done].timestamp =
			(congest->timestamp + 500)/1000;
#ifdef ISID_STATS
		stats_results->secs[secs_done].crsglitch =
			congest->crsglitch;
		stats_results->secs[secs_done].badplcp =
			congest->badplcp;
		stats_results->secs[secs_done].bphy_crsglitch =
			congest->bphy_crsglitch;
		stats_results->secs[secs_done].bphy_badplcp =
			congest->bphy_badplcp;
#endif /* ISID_STATS */
		for (i = 0; i < 8; i++) {
			stats_results->secs[secs_done].txnode[i] = (congest->txnode[i] + 500)/1000;
			stats_results->secs[secs_done].rxnode[i] = (congest->rxnode[i] + 500)/1000;
		}
		stats_results->secs[secs_done].xxobss = (congest->xxobss + 500)/1000;

		buflen -= sizeof(wlc_congest_t);
	}
	stats_results->num_secs = (uint8)(secs_done & 0xff);
	return 0;
}

#ifdef WL_CCA_STATS_MESH
int
wlc_get_cca_mesh_enable(wlc_info_t* wlc)
{
	if (wlc->cca_info) {
		return wlc->cca_info->cca_mesh_enable;
	}
	return 0;
}
static int
cca_stats_mesh(cca_info_t *cca, void *input)
{
	wlc_info_t *wlc = cca->wlc;
	int i;
	int emp_idx = BCME_NOTFOUND;
	int amt_idx = BCME_NOTFOUND;
	uint16 prev_amtinfo;
	uint16 tmp_attr;
	struct ether_addr tmp_ea;
	int err = BCME_OK;
	cca_mesh_req_t *req = (cca_mesh_req_t *)input;

	/* Feature is only available for correv 42, 56, 61.5, 65,
	 * and revisions greater than 128 in KUDU
	 */
	if (!((D11REV_IS(wlc->pub->corerev, 61) && D11MINORREV_IS(wlc->pub->corerev_minor, 5)) ||
			D11REV_IS(wlc->pub->corerev, 42) || D11REV_IS(wlc->pub->corerev, 56) ||
			D11REV_IS(wlc->pub->corerev, 65) || D11REV_GE(wlc->pub->corerev, 128))) {
		WL_ERROR(("CCA_STATS_MESH is unsupported for corerev:%d\n",
			wlc->pub->corerev));
		return BCME_UNSUPPORTED;
	}

	if (req->cmd_type == CCA_MESH_ENABLE) {
		/* command handler to enable the feature and reserve entries */
		if (cca->cca_mesh_enable) {
			WL_INFORM(("wl%d: CCA_STATS_MESH already enabled\n",
				wlc->pub->unit));
			return BCME_OK;
		}
		if (wlc_cca_mesh_amt_reserve(wlc)) {
			WL_ERROR(("wl%d: %s: wlc_cca_mesh_amt_reserve() failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NORESOURCE;
		} else {
			cca->cca_mesh_enable = TRUE;
			cca->mesh_count = 0;
			return BCME_OK;
		}
	} else if (!cca->cca_mesh_enable) {
		/* Other operations are not supported till enabled */
		WL_INFORM(("wl%d: %s: disabled\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_DISABLED;
	}

	if (req->cmd_type == CCA_MESH_SET) {
		/* command handler for adding entry */
		if (cca->mesh_count >= CCA_MESH_MAX) {
			/* Limit mesh entries */
			WL_INFORM(("wl%d: Max CCA_CAP macs set\n",
				wlc->pub->unit));
			return BCME_NORESOURCE;
		}

		/* check special entry for self-MAC */
		wlc_get_addrmatch(wlc, AMT_IDX_MAC, &tmp_ea, &tmp_attr);
		prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, AMT_IDX_MAC);
		if (!ether_cmp(&tmp_ea, &req->mac)) {
			if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
			        WL_INFORM(("wl%d: MAC already present\n", wlc->pub->unit));
			} else {
				++cca->mesh_count;
				wlc_write_amtinfo_by_idx(wlc, AMT_IDX_MAC,
					SET_CCA_NBITVAL(wlc, prev_amtinfo));
			}
		} else {
			/* check entries for generic MAC / (NOT self) */
		        for (i = AMT_IDX_CCA_MESH_START; i <= AMT_IDX_CCA_MESH_END; i++) {
				wlc_get_addrmatch(wlc, i, &tmp_ea, &tmp_attr);
				if (tmp_attr == 0) {
					/* store index of first empty slot */
					if (emp_idx == BCME_NOTFOUND)
						emp_idx = i;
					continue;
				} else {
					/* check for duplicate entries for the MAC */
					prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, i);
					if (!ether_cmp(&tmp_ea, &req->mac) &&
						(tmp_attr == (AMT_ATTR_A2 |
							AMT_ATTR_A1 |
							AMT_ATTR_VALID))) {
						if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
							amt_idx = i;
							WL_INFORM(("wl%d: MAC already present\n ",
								wlc->pub->unit));
							break;
						}
					}
				}
			}
			if (amt_idx == BCME_NOTFOUND) {
				/* add new entry */
				amt_idx = emp_idx;
				if (emp_idx != BCME_NOTFOUND) {
					tmp_attr = AMT_ATTR_A2 | AMT_ATTR_A1 |
						AMT_ATTR_VALID;
					wlc_set_addrmatch(wlc, amt_idx, &req->mac, tmp_attr);
					++cca->mesh_count;
					SET_CCA_NBITVAL(wlc, prev_amtinfo);
					wlc_write_amtinfo_by_idx(wlc, amt_idx, prev_amtinfo);
				} else {
					WL_INFORM(("wl%d: WARNING: no available"
						"enrty in AMT table ...\n",
						wlc->pub->unit));
					return BCME_NORESOURCE;
				}
			}
		}
	} else if (req->cmd_type == CCA_MESH_DEL) {
		/* command handler for deleting entry */
		if (cca->mesh_count <= 0) {
			WL_INFORM(("wl%d: All CCA_CAP macs are deleted\n",
				wlc->pub->unit));
			return BCME_NOTFOUND;
		}
		/* check special entry for self-MAC */
		wlc_get_addrmatch(wlc, AMT_IDX_MAC, &tmp_ea, &tmp_attr);
		prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, AMT_IDX_MAC);
		if (!ether_cmp(&tmp_ea, &req->mac)) {
			if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
				--cca->mesh_count;
				wlc_write_amtinfo_by_idx(wlc, AMT_IDX_MAC,
					CLR_CCA_NBITVAL(wlc, prev_amtinfo));
			} else {
				WL_INFORM(("wl%d: CCA_CAP not set\n",
					wlc->pub->unit));
			}
		} else {
			/* check entries for generic MAC / (NOT self) */
		        for (i = AMT_IDX_CCA_MESH_START; i <= AMT_IDX_CCA_MESH_END; i++) {
				wlc_get_addrmatch(wlc, i, &tmp_ea, &tmp_attr);
				if (tmp_attr != 0) {
					prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, i);
					if (!ether_cmp(&tmp_ea, &req->mac) &&
						(tmp_attr == (AMT_ATTR_A2 |
						AMT_ATTR_A1 |
						AMT_ATTR_VALID))) {
						if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
							amt_idx = i;
							--cca->mesh_count;
							wlc_clear_addrmatch(wlc, amt_idx);
							break;
						}
					}
				}
			}
			if (amt_idx == BCME_NOTFOUND) {
				WL_INFORM(("wl%d: No enrty in AMT table ...\n",
					wlc->pub->unit));
			}
		}
	} else if (req->cmd_type == CCA_MESH_DEL_ALL) {
		/* command handler for deleting all entries */
		if (cca->mesh_count <= 0) {
			WL_INFORM(("wl%d: All CCA_CAP macs are deleted\n",
				wlc->pub->unit));
			return err;
		}
		/* check special entry for self-MAC */
		prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, AMT_IDX_MAC);
		if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
			--cca->mesh_count;
			wlc_write_amtinfo_by_idx(wlc, AMT_IDX_MAC,
				CLR_CCA_NBITVAL(wlc, prev_amtinfo));
		}
		/* check entries for generic MAC / (NOT self) */
		for (i = AMT_IDX_CCA_MESH_START; i <= AMT_IDX_CCA_MESH_END; i++) {
			wlc_get_addrmatch(wlc, i, &tmp_ea, &tmp_attr);
			if (tmp_attr != 0) {
				prev_amtinfo = wlc_read_amtinfo_by_idx(wlc, i);
				if (tmp_attr == (AMT_ATTR_A2 | AMT_ATTR_A1 |
						AMT_ATTR_VALID)) {
					if (GET_CCA_NBITVAL(wlc, prev_amtinfo)) {
						--cca->mesh_count;
						wlc_clear_addrmatch(wlc, i);
					}
				}
			}
		}
	} else if (req->cmd_type == CCA_MESH_DISABLE) {
		/* command handler to disable the feature and release entries */
		if (!wlc_cca_mesh_amt_release(wlc)) {
			cca->cca_mesh_enable = FALSE;
		}
	} else {
		err = BCME_USAGE_ERROR;
	}
	return err;
}
#endif /* WL_CCA_STATS_MESH */

static int
cca_get_stats(cca_info_t *cca, void *input, int buf_len, void *output)
{
	int nsecs;
	chanspec_t chanspec;
	cca_congest_channel_req_t *req = (cca_congest_channel_req_t *)input;
	cca_congest_channel_req_t *stats_results = (cca_congest_channel_req_t *)output;
	wlc_congest_channel_req_t *results;
	int result_len;
	int status;
	BCM_REFERENCE(buf_len);

	if (!cca)
		return BCME_UNSUPPORTED;

	if (wf_chspec_malformed(req->chanspec))
		return BCME_BADCHAN;

	chanspec = wf_chspec_ctlchspec(req->chanspec);
	nsecs = (req->num_secs > cca->cca_second_max) ? cca->cca_second_max : req->num_secs;

	result_len = sizeof(wlc_congest_channel_req_t) +
		((nsecs ? nsecs - 1 : nsecs) * sizeof(wlc_congest_t));
	if (!(results = (wlc_congest_channel_req_t*)MALLOCZ(cca->wlc->osh, result_len)))
		return BCME_NOMEM;

	status = cca_query_stats(cca->wlc, chanspec, nsecs, results, result_len);

	if (status == 0) {
		int i;
		wlc_congest_t *wlc_congest = results->secs;
		cca_congest_t *cca_congest = stats_results->secs;
		stats_results->chanspec = results->chanspec;
		stats_results->num_secs = results->num_secs;
		for (i = 0; i < nsecs; i++) {
			int j;
			cca_congest[i].duration = wlc_congest[i].duration;
			cca_congest[i].congest_ibss = wlc_congest[i].congest_ibss;
			cca_congest[i].congest_obss = wlc_congest[i].congest_obss;
			cca_congest[i].interference = wlc_congest[i].interference;
			cca_congest[i].timestamp = wlc_congest[i].timestamp;
			for (j = 0; j < CCA_MESH_MAX; j++) {
				cca_congest[i].txnode[j] = wlc_congest[i].txnode[j];
				cca_congest[i].rxnode[j] = wlc_congest[i].rxnode[j];
			}
			cca_congest[i].xxobss = wlc_congest[i].xxobss;
		}
	}

	MFREE(cca->wlc->osh, results, result_len);

	return status;
}

void
cca_stats_upd(wlc_info_t *wlc, int calculate)
{
	cca_ucode_counts_t tmp = {0};
	int chan;
#ifdef WL_CCA_STATS_MESH
	int i;
#endif /* WL_CCA_STATS_MESH */
	chanspec_t chanspec = wf_chspec_ctlchspec(wlc->chanspec);
	cca_info_t *cca = wlc->cca_info;

	if (!cca)
		return;

	if ((chan = cca_chanspec_to_index(cca, chanspec)) < 0) {
		WL_INFORM(("%s: Invalid chanspec 0x%x\n",
			__FUNCTION__, chanspec));
		return;
	}

	if (wlc_bmac_cca_stats_read(wlc->hw, &tmp))
		return;

	if (calculate) {
#ifndef DONGLEBUILD
		/* alloc a new second if needed. */
		if (CCA_POOL_IDX(cca, chan, cca->cca_second) == 0)
			cca_alloc_pool(cca, chan, cca->cca_second);

		if (CCA_POOL_IDX(cca, chan, cca->cca_second) != 0)
#endif /* DONGLEBUILD */
		{
			cca_ucode_counts_t delta;
			wlc_congest_t *stats = CCA_POOL_DATA(cca, chan, cca->cca_second);
			uint32 total_busy;

			/* Clear stat entry with invalid data */
			total_busy =
				stats->congest_ibss + stats->congest_obss + stats->interference;
			if (stats->duration < total_busy)
				bzero(stats, sizeof(wlc_congest_t));

			/* Calc delta */
			delta.txdur = tmp.txdur  - cca->last_cca_stats.txdur;
			delta.ibss  = tmp.ibss   - cca->last_cca_stats.ibss;
			delta.obss  = tmp.obss   - cca->last_cca_stats.obss;
			delta.noctg = tmp.noctg  - cca->last_cca_stats.noctg;
			delta.nopkt = tmp.nopkt  - cca->last_cca_stats.nopkt;
			delta.usecs = tmp.usecs  - cca->last_cca_stats.usecs;
			delta.PM    = tmp.PM     - cca->last_cca_stats.PM;
#ifdef ISID_STATS
			delta.crsglitch = tmp.crsglitch - cca->last_cca_stats.crsglitch;
			delta.badplcp = tmp.badplcp - cca->last_cca_stats.badplcp;
			delta.bphy_crsglitch = tmp.bphy_crsglitch -
			  cca->last_cca_stats.bphy_crsglitch;
			delta.bphy_badplcp = tmp.bphy_badplcp - cca->last_cca_stats.bphy_badplcp;
#endif /* ISID_STATS */
#ifdef WL_CCA_STATS_MESH
			for (i = 0; i < CCA_MESH_MAX; i++) {
				delta.txnode[i] = tmp.txnode[i] - cca->last_cca_stats.txnode[i];
				delta.rxnode[i] = tmp.rxnode[i] - cca->last_cca_stats.rxnode[i];
			}
			delta.xxobss = tmp.xxobss - cca->last_cca_stats.xxobss;
#endif /* WL_CCA_STATS_MESH */

			if (delta.usecs >= (delta.txdur + delta.ibss +
				delta.obss + delta.noctg + delta.nopkt + delta.PM)) {
				/* Update stats */
				stats->duration += delta.usecs;
				/* Factor in time MAC was powered down */
				if (BSSCFG_STA(wlc->primary_bsscfg) &&
					wlc->primary_bsscfg->pm->PMenabled)
					stats->duration -= delta.PM;
				stats->congest_ibss += delta.ibss + delta.txdur;
				stats->congest_obss += delta.obss + delta.noctg;
				stats->interference += delta.nopkt;
#ifdef ISID_STATS
				stats->crsglitch += delta.crsglitch;
				stats->badplcp += delta.badplcp;
				stats->bphy_crsglitch += delta.bphy_crsglitch;
				stats->bphy_badplcp += delta.bphy_badplcp;
#endif /* ISID_STATS */
				stats->timestamp = OSL_SYSUPTIME();
#ifdef WL_CCA_STATS_MESH
				for (i = 0; i < CCA_MESH_MAX; i++) {
					stats->txnode[i] += delta.txnode[i];
					stats->rxnode[i] += delta.rxnode[i];
				}
				stats->xxobss += delta.xxobss;
#endif /* WL_CCA_STATS_MESH */
			} else {
				WL_INFORM(("CCA sample ignored[ch=0x%04x]: "
					"busy = %d-%d-%d-%d-%d dur = %d-%d\n",
					chanspec, delta.txdur, delta.ibss, delta.obss,
					delta.noctg, delta.nopkt, delta.usecs, delta.PM));
				return;
			}
		}
	}
	/* Store raw values for next read */
	cca->last_cca_stats.txdur = tmp.txdur;
	cca->last_cca_stats.ibss  = tmp.ibss;
	cca->last_cca_stats.obss  = tmp.obss;
	cca->last_cca_stats.noctg = tmp.noctg;
	cca->last_cca_stats.nopkt = tmp.nopkt;
	cca->last_cca_stats.usecs = tmp.usecs;
	cca->last_cca_stats.PM = tmp.PM;
#ifdef ISID_STATS
	cca->last_cca_stats.crsglitch = tmp.crsglitch;
	cca->last_cca_stats.badplcp = tmp.badplcp;
	cca->last_cca_stats.bphy_crsglitch = tmp.bphy_crsglitch;
	cca->last_cca_stats.bphy_badplcp = tmp.bphy_badplcp;
#endif /* ISID_STATS */
#ifdef WL_CCA_STATS_MESH
	for (i = 0; i < CCA_MESH_MAX; i++) {
		cca->last_cca_stats.txnode[i] = tmp.txnode[i];
		cca->last_cca_stats.rxnode[i] = tmp.rxnode[i];
	}
	cca->last_cca_stats.xxobss  = tmp.xxobss;
#endif /* WL_CCA_STATS_MESH */
}

int
cca_send_event(wlc_info_t *wlc, bool forced)
{
	chanspec_t chanspec = wf_chspec_ctlchspec(wlc->chanspec);
	cca_info_t *cca = wlc->cca_info;
	wlc_congest_channel_req_t results;
	int status;

	if (!cca)
		return BCME_UNSUPPORTED;

	if (wf_chspec_malformed(chanspec))
		return BCME_BADCHAN;

	results.num_secs = 0;
	status = cca_query_stats(cca->wlc, chanspec, 1, &results, sizeof(results));

	if ((status == 0) && (results.num_secs == 1) &&
	    (results.secs[0].duration > 0)) {
		cca_chan_qual_event_t event_output;
		uint cca_busy;

		event_output.status = 0;
		event_output.id = WL_CHAN_QUAL_FULL_CCA;
		event_output.chanspec = results.chanspec;
		event_output.len = sizeof(event_output.cca_busy_ext);
		event_output.cca_busy_ext.duration = results.secs[0].duration;
		event_output.cca_busy_ext.congest_ibss = results.secs[0].congest_ibss;
		event_output.cca_busy_ext.congest_obss = results.secs[0].congest_obss;
		event_output.cca_busy_ext.interference = results.secs[0].interference;
		event_output.cca_busy_ext.timestamp = results.secs[0].timestamp;
		cca_busy = event_output.cca_busy_ext.congest_ibss +
				event_output.cca_busy_ext.congest_obss +
				event_output.cca_busy_ext.interference;
		if (event_output.cca_busy_ext.duration < cca_busy) {
			WL_ERROR(("CCA bad stat: dur=%d busy=%d\n",
				event_output.cca_busy_ext.duration,
				cca_busy));
		}

		cca_busy = (cca_busy * 100) / event_output.cca_busy_ext.duration;
		if (forced || wlc_cca_chan_qual_event_update(wlc, WL_CHAN_QUAL_CCA, cca_busy)) {
			wlc_bss_mac_event(cca->wlc, NULL, WLC_E_CCA_CHAN_QUAL, NULL,
				0, 0, 0, &event_output, sizeof(event_output));
		}
	}

	return status;
}

static void
wlc_cca_chan_qual_event_timeout(void *arg)
{
	wlc_info_t *wlc = (wlc_info_t*)arg;
	wlc->cca_chan_qual->is_cca_event_timer_active = FALSE;
}

/* return TRUE to indicate interested metric level changed */
bool
wlc_cca_chan_qual_event_update(wlc_info_t *wlc, uint8 id, int v)
{
	cca_chan_qual_t *chq = wlc->cca_chan_qual;
	uint8 level, prev_level;

	if (chq == NULL)
		return FALSE;

	/* no update if timer active */
	if (chq->is_cca_event_timer_active)
		return FALSE;

	/* check for supported metric */
	if ((id >= chq->event.num_metrics) || (id >= WL_CHAN_QUAL_TOTAL))
		return FALSE;

	/* find metric level */
	prev_level = chq->level[id];
	for (level = 0; level < chq->event.metric[id].num_levels; level++) {
		int thresh;
		if (level < prev_level)
			thresh = chq->event.metric[id].htol[level];
		else
			thresh = chq->event.metric[id].ltoh[level];

		if (v < thresh)
			break;
	}

	if (level == prev_level)
		return FALSE;

	chq->level[id] = level;

	if (chq->cca_event_timer && chq->event.rate_limit_msec) {
		/* rate limit events */
		chq->is_cca_event_timer_active = TRUE;
		wl_add_timer(wlc->wl, chq->cca_event_timer,
			chq->event.rate_limit_msec, FALSE);
	}
	return TRUE;
}
#endif /* CCA_STATS */
