/*
 * TxPowerCap module implementation - iovar table
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
 * $Id: phy_txpwrcap_iov.c 785011 2020-03-10 22:27:57Z $
 */

#if defined(WLC_TXPWRCAP)

#include <phy_txpwrcap.h>
#include <phy_type_txpwrcap.h>
#include <phy_txpwrcap_iov.h>
#include <phy_txpwrcap_api.h>
#include <wlc_iocv_reg.h>
#ifndef ALL_NEW_PHY_MOD
#include <wlc_phy_int.h>
#endif

enum {
	IOV_PHY_CELLSTATUS,
	IOV_PHY_TX_PWRCAP,
	IOV_PHY_TXPWRCAP_TBL
};

/* iovar table */
static const bcm_iovar_t phy_txpwrcap_iovars[] = {
#if defined(WLTEST)
	{"phy_txpwrcap_tbl", IOV_PHY_TXPWRCAP_TBL, 0, 0, IOVT_BUFFER, sizeof(wl_txpwrcap_tbl_t)},
#endif
	{"phy_cellstatus", IOV_PHY_CELLSTATUS, IOVF_SET_UP | IOVF_GET_UP, 0, IOVT_INT8, 0},
	{"phy_txpwrcap", IOV_PHY_TX_PWRCAP, IOVF_GET_UP, 0, IOVT_UINT32, 0},
	{NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

static int
phy_txpwrcap_doiovar(void *ctx, uint32 aid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	int err = BCME_OK;
	phy_info_t *pi = (phy_info_t *)ctx;
	bool bool_val = FALSE;
	int int_val = 0;
	int32 *ret_int_ptr = (int32 *)a;

	if (plen >= (uint)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	/* bool conversion to avoid duplication below */
	bool_val = int_val != 0;

	(void)pi;
	(void)bool_val;

	switch (aid) {
	case IOV_GVAL(IOV_PHY_CELLSTATUS):
		if (PHY_TXPWRCAP_ENAB(pi->sh->physhim))
			err = wlc_phyhal_txpwrcap_get_cellstatus((wlc_phy_t*)pi,
					ret_int_ptr);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_PHY_CELLSTATUS):
		if (PHY_TXPWRCAP_ENAB(pi->sh->physhim))
			err = phy_txpwrcap_cellstatus_override_set(pi, int_val);
		else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_GVAL(IOV_PHY_TX_PWRCAP):
		if (PHY_TXPWRCAP_ENAB(pi->sh->physhim)) {
			if (!pi->sh->clk) {
				err = BCME_NOCLK;
				break;
			}
			*ret_int_ptr = phy_txpwrcap_get_caps_inuse(pi);
		} else
			err = BCME_UNSUPPORTED;
		break;

#if defined(WLTEST)
	case IOV_GVAL(IOV_PHY_TXPWRCAP_TBL):
		if (PHY_TXPWRCAP_ENAB(pi->sh->physhim)) {
			wl_txpwrcap_tbl_t txpwrcap_tbl;
			err = wlc_phy_txpwrcap_tbl_get((wlc_phy_t*)pi, &txpwrcap_tbl);
			if (err == BCME_OK)
				bcopy(&txpwrcap_tbl, a, sizeof(wl_txpwrcap_tbl_t));
		} else
			err = BCME_UNSUPPORTED;
		break;

	case IOV_SVAL(IOV_PHY_TXPWRCAP_TBL):
		if (PHY_TXPWRCAP_ENAB(pi->sh->physhim)) {
			wl_txpwrcap_tbl_t txpwrcap_tbl;
			bcopy(p, &txpwrcap_tbl, sizeof(wl_txpwrcap_tbl_t));
			err = wlc_phy_txpwrcap_tbl_set((wlc_phy_t*)pi, &txpwrcap_tbl);
		} else
			err = BCME_UNSUPPORTED;
		break;
#endif
	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	return err;
}

/* register iovar table to the system */
int
BCMATTACHFN(phy_txpwrcap_register_iovt)(phy_info_t *pi, wlc_iocv_info_t *ii)
{
	wlc_iovt_desc_t iovd;
#if defined(WLC_PATCH_IOCTL)
	wlc_iov_disp_fn_t disp_fn = IOV_PATCH_FN;
	const bcm_iovar_t *patch_table = IOV_PATCH_TBL;
#else
	wlc_iov_disp_fn_t disp_fn = NULL;
	const bcm_iovar_t* patch_table = NULL;
#endif /* WLC_PATCH_IOCTL */

	ASSERT(ii != NULL);

	wlc_iocv_init_iovd(phy_txpwrcap_iovars,
	                   NULL, NULL,
	                   phy_txpwrcap_doiovar, disp_fn, patch_table, pi,
	                   &iovd);

	return wlc_iocv_register_iovt(ii, &iovd);
}

#endif /* WLC_TXPWRCAP */
