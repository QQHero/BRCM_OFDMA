/*
 * Ultr-Low Bandwidth Mode (ULB)
 *
 * Required functions exported by wlc_ulb.c to common (os-independent) driver code.
 *
 * <<Broadcom-WL-IPTag/Proprietary:>>
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
 * $Id: wlc_ulb.h 570838 2015-07-13 18:44:44Z $
 */

/** Twiki: http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/UltraLowBandMode */

#ifndef _WLC_ULB_H_
#define _WLC_ULB_H_

#include <bcmutils.h>

#define ULB_CAP_BW_NONE				0
#define ULB_CAP_BW_10MHZ			BCM_BIT(0)
#define ULB_CAP_BW_5MHZ				BCM_BIT(1)
#define ULB_CAP_BW_2P5MHZ			BCM_BIT(2)

#ifndef WL11ULB
/* Default MACROs to avoid defining WL11ULB flag everywhere */
#define WLC_ULB_MODE_ENABLED(wlc)		0
#define CHSPEC_IS_ULB(wlc, chspec)		0
#define WLC_ULB_CHSPEC_ISLE20(wlc, chspec)		(CHSPEC_IS20(chspec))
#define WLC_2P5MHZ_ULB_SUPP_BAND(wlc, bunit)	0
#define WLC_5MHZ_ULB_SUPP_BAND(wlc, bunit)	0
#define WLC_10MHZ_ULB_SUPP_BAND(wlc, bunit)	0
#define WLC_2P5MHZ_ULB_SUPP_HW(wlc)		0
#define WLC_5MHZ_ULB_SUPP_HW(wlc)		0
#define WLC_10MHZ_ULB_SUPP_HW(wlc)		0
#define BSSCFG_MINBW_CHSPEC(wlc, cfg, chnum)	(CH20MHZ_CHSPEC(chnum))
#define WLC_ULB_GET_BSSCFG_MINBW(wlc, bw)	((bw) = WL_CHANSPEC_BW_20)
#define	WLC_LE20_VALID_CHANNEL(wlc, bw, chan)	(VALID_CHANNEL20(wlc, chan))
#define	WLC_LE20_VALID_CHANNEL_DB(wlc, bw, chan)	(VALID_CHANNEL20_DB(wlc, chan))
#define	WLC_LE20_VALID_CHANNEL_IN_BAND(wlc, bandunit, bw, chan) (\
			VALID_CHANNEL20_IN_BAND(SCAN_WLC(scan), (bu), (chan)))
#define WLC_LE20_VALID_SCAN_CHANNEL_IN_BAND(scan, bu, bw, val) (\
			SCAN_VALID_CHANNEL20_IN_BAND(scan, bu, val))
#else /* WL11ULB */
#define WLC_ULB_MODE_ENABLED(wlc)	(ULB_ENAB((wlc)->pub) && \
					wlc_ulb_mode_enabled((wlc)->ulb_info))
#define IS_ULB_DYN_MODE_ENABLED(wlc)	(ULB_ENAB((wlc)->pub) && \
					wlc_ulb_dyn_mode_enabled((wlc)->ulb_info))
#define CHSPEC_IS_ULB(wlc, chspec)	(WLC_ULB_MODE_ENABLED((wlc)) && \
					(CHSPEC_IS2P5((chspec)) || \
					CHSPEC_IS5((chspec)) || \
					CHSPEC_IS10((chspec))))
#define WLC_ULB_CHSPEC_ISLE20(wlc, chspec)	(CHSPEC_IS20(chspec) || \
						CHSPEC_IS_ULB(wlc, chspec))

/* Macro to substitue usage of CH20MHZ_CHSPEC() with ULB chanspecs */
#define BSSCFG_MINBW_CHSPEC(wlc, cfg, chnum) ((WLC_ULB_MODE_ENABLED(wlc) && ((cfg) != NULL))? \
		CHBW_CHSPEC(wlc_ulb_get_bss_min_bw((wlc)->ulb_info, (cfg)), (chnum)):\
		CH20MHZ_CHSPEC(chnum))
#define WLC_ULB_GET_BSSCFG_MINBW(wlc, bw) \
			{\
				int idx = 0; wlc_bsscfg_t *cfg = NULL; \
				FOREACH_ULB_ENAB_BSS(wlc, idx, cfg) {\
					(bw) = WLC_ULB_GET_BSS_MIN_BW(wlc, cfg); \
					break; \
				}\
			}

#define CHSPEC_IS_ULB_EITHER(wlc, chspec1, chspec2)	(\
		CHSPEC_IS_ULB(wlc, chspec1) || CHSPEC_IS_ULB(wlc, chspec2))

#define WLC_2P5MHZ_ULB_SUPP_BAND(wlc, bunit) \
			wlc_ulb_bw_supp_band((wlc)->ulb_info, (bunit), ULB_CAP_BW_2P5MHZ)
#define WLC_5MHZ_ULB_SUPP_BAND(wlc, bunit) \
			wlc_ulb_bw_supp_band((wlc)->ulb_info, (bunit), ULB_CAP_BW_5MHZ)
#define WLC_10MHZ_ULB_SUPP_BAND(wlc, bunit) \
			wlc_ulb_bw_supp_band((wlc)->ulb_info, (bunit), ULB_CAP_BW_10MHZ)
#define WLC_2P5MHZ_ULB_SUPP_HW(wlc) wlc_ulb_bw_supp_hw((wlc)->ulb_info, ULB_CAP_BW_2P5MHZ)
#define WLC_5MHZ_ULB_SUPP_HW(wlc) wlc_ulb_bw_supp_hw((wlc)->ulb_info, ULB_CAP_BW_5MHZ)
#define WLC_10MHZ_ULB_SUPP_HW(wlc) wlc_ulb_bw_supp_hw((wlc)->ulb_info, ULB_CAP_BW_10MHZ)
#define	WLC_LE20_VALID_CHANNEL(wlc, bw, chan) (\
			(WLC_ULB_MODE_ENABLED(wlc) ? CHSPEC_IS2P5(bw) ? \
			VALID_CHANNEL2P5(wlc, chan): CHSPEC_IS5(bw) ? \
			VALID_CHANNEL5(wlc, chan): VALID_CHANNEL10(wlc, chan) : \
			VALID_CHANNEL20(wlc, chan)))
#define	WLC_LE20_VALID_CHANNEL_DB(wlc, bw, chan) (\
				(WLC_ULB_MODE_ENABLED(wlc) ? CHSPEC_IS2P5(bw) ? \
				VALID_CHANNEL2P5_DB(wlc, chan): CHSPEC_IS5(bw) ? \
				VALID_CHANNEL5_DB(wlc, chan): VALID_CHANNEL10_DB(wlc, chan) : \
				VALID_CHANNEL20_DB(wlc, chan)))
#define	WLC_LE20_VALID_CHANNEL_IN_BAND(wlc, bu, bw, chan) (\
			(WLC_ULB_MODE_ENABLED(wlc) ? CHSPEC_IS2P5(bw) ? \
			VALID_CHANNEL2P5_IN_BAND(wlc, bu, chan): CHSPEC_IS5(bw) ? \
			VALID_CHANNEL5_IN_BAND(wlc, bu, chan): \
			VALID_CHANNEL10_IN_BAND(wlc, bu, chan) : \
			VALID_CHANNEL20_IN_BAND(wlc, bu, chan)))
#ifdef SCANOL
#define WLC_LE20_VALID_SCAN_CHANNEL_IN_BAND(scan, bu, bw, val) \
					SCAN_VALID_CHANNEL20_IN_BAND(scan, bu, val)
#else /* SCANOL */
#define WLC_LE20_VALID_SCAN_CHANNEL_IN_BAND(scan, bu, bw, val) \
				WLC_LE20_VALID_CHANNEL_IN_BAND(SCAN_WLC(scan), (bu), (bw), (val))
#endif /* SCANOL */

#define IS_BSS_ULB_CAP(bi)	(IS_BSS_ULB_2P5_CAP(bi) || IS_BSS_ULB_5_CAP(bi) || \
				IS_BSS_ULB_10_CAP(bi))
#define IS_BSS_ULB_2P5_CAP(bi)	(!!((bi)->flags2 & WLC_BSS_ULB_2P5_CAP))
#define IS_BSS_ULB_5_CAP(bi)	(!!((bi)->flags2 & WLC_BSS_ULB_5_CAP))
#define IS_BSS_ULB_10_CAP(bi)	(!!((bi)->flags2 & WLC_BSS_ULB_10_CAP))
#define IS_SCB_ULB_CAP(scb)	(IS_SCB_ULB_2P5_CAP(scb) || IS_SCB_ULB_5_CAP(scb) || \
				IS_SCB_ULB_10_CAP(scb))
#define IS_SCB_ULB_2P5_CAP(scb)	(!!((scb)->flags3 & SCB3_IS_2P5))
#define IS_SCB_ULB_5_CAP(scb)	(!!((scb)->flags3 & SCB3_IS_5))
#define IS_SCB_ULB_10_CAP(scb)	(!!((scb)->flags3 & SCB3_IS_10))
#define WLC_ULB_GET_BSS_MIN_BW(wlc, cfg) (WLC_ULB_MODE_ENABLED(wlc) ? \
	wlc_ulb_get_bss_min_bw((wlc)->ulb_info, (cfg)) : WL_CHANSPEC_BW_20)

extern wlc_ulb_info_t *wlc_ulb_attach(wlc_info_t *wlc);
extern void wlc_ulb_detach(wlc_ulb_info_t *ulb_info);
extern bool wlc_ulb_mode_enabled(wlc_ulb_info_t *ulb_info);
extern bool wlc_ulb_dyn_mode_enabled(wlc_ulb_info_t *ulb_info);
extern bool wlc_ulb_bw_supp_hw(wlc_ulb_info_t *ulb_info, uint16 ulb_bw);
extern bool wlc_ulb_bw_supp_band(wlc_ulb_info_t *ulb_info, uint bandunit, uint16 ulb_bw);
extern uint16 wlc_ulb_get_bss_min_bw(wlc_ulb_info_t *ulb_info, wlc_bsscfg_t *cfg);
extern int wlc_ulb_set_bss_min_bw(wlc_ulb_info_t *ulb_info, wlc_bsscfg_t *cfg, uint16 bw,
	bool update_enab);
extern int wlc_ulb_recv_action_frames(wlc_ulb_info_t *ulb_info, wlc_bsscfg_t *cfg,
	struct dot11_management_header *hdr, uint8 *body, int body_len);
extern int wlc_ulb_hdl_set_ulb_chspec(wlc_ulb_info_t *ulb_info, wlc_bsscfg_t *bsscfg,
	uint16 chspec);
#endif /* WL11ULB */
#endif /* _WLC_ULB_H_ */
