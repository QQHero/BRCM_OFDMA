/*
 * ACPHY Calibration Manager module implementation
 *
 * Copyright 2021 Broadcom
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
 * $Id: phy_ac_calmgr.c 795604 2021-02-10 06:28:26Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_calmgr.h"
#include "phy_type_cache.h"
#include <phy_ac.h>
#include <phy_btcx.h>
#include <phy_papdcal.h>
#include <phy_calmgr.h>
#include <phy_ac_calmgr.h>
#include <phy_ac_chanmgr.h>
#include <phy_ac_misc.h>
#include <phy_ac_papdcal.h>
#include <phy_ac_radio.h>
#include <phy_ac_rxgcrs.h>
#include <phy_ac_rxiqcal.h>
#include <phy_ac_tssical.h>
#include <phy_ac_txiqlocal.h>
#include <phy_ac_vcocal.h>
#include <phy_cache_api.h>
#include <phy_ocl_api.h>
#include <phy_stf.h>
#include <phy_misc_api.h>
#include <phy_calmgr_api.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20693.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>
#include <wlc_phy_int.h>

#include <phy_utils_math.h>
#include <phy_utils_reg.h>
#include <phy_ac_info.h>

struct multiphase_caltimes {
	uint16 vco;
	uint16 dc;
	uint16 tssi;
	uint16 tx;
	uint16 rx;
};

/* module private states */
struct phy_ac_calmgr_info {
	phy_info_t *pi;
	phy_ac_info_t *aci;
	phy_calmgr_info_t *cmn_info;
	struct multiphase_caltimes cal_times;
	/* cals  - Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx;
	uint8 enTx;
	int16 idac_i;
	int16 idac_q;
	uint8 tx_pwr_ctrl_state;
	uint32 cal_phase_id;
	uint32 start_time;
	bool enIndxCap;
	bool fullcal;
};

/* local functions */
static int phy_ac_calmgr_init(phy_type_calmgr_ctx_t *ctx);
static int phy_ac_calmgr_prepare(phy_type_calmgr_ctx_t *ctx);
static void phy_ac_calmgr_cleanup(phy_type_calmgr_ctx_t *ctx);
static bool phy_ac_calmgr_wd(phy_type_calmgr_ctx_t *ctx);
static void phy_ac_calmgr_nvram_attach(phy_ac_calmgr_info_t *calmgri);
static void wlc_phy_iqlocal_state_check_acphy(phy_info_t *pi);
static void phy_ac_calmgr_caltimes(phy_info_t *pi, phy_ac_calmgr_info_t *ci);

/* register phy type specific implementation */
phy_ac_calmgr_info_t *
BCMATTACHFN(phy_ac_calmgr_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_calmgr_info_t *cmn_info)
{
	phy_ac_calmgr_info_t *ac_info;
	phy_type_calmgr_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage together */
	if ((ac_info = phy_malloc(pi, sizeof(phy_ac_calmgr_info_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}
	ac_info->pi = pi;
	ac_info->aci = aci;
	ac_info->cmn_info = cmn_info;

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		(ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev))) {
		/* phy_cal based on tempsense only */
		pi->cal_period = 0;
	}

	/* register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.init = phy_ac_calmgr_init;
	fns.wd = phy_ac_calmgr_wd;
	fns.prepare = phy_ac_calmgr_prepare;
	fns.cleanup = phy_ac_calmgr_cleanup;
	fns.cals = wlc_phy_cals_acphy;
	fns.ctx = ac_info;

	/* Read srom params from nvram */
	phy_ac_calmgr_nvram_attach(ac_info);

	/* Multiphase Calibration times for cts2self */
	phy_ac_calmgr_caltimes(pi, ac_info);

	if (phy_calmgr_register_impl(cmn_info, &fns) != BCME_OK) {
		PHY_ERROR(("%s: phy_calmgr_register_impl failed\n", __FUNCTION__));
		goto fail;
	}

	return ac_info;

	/* error handling */
fail:
	if (ac_info != NULL)
		phy_mfree(pi, ac_info, sizeof(phy_ac_calmgr_info_t));
	return NULL;
}

void
BCMATTACHFN(phy_ac_calmgr_unregister_impl)(phy_ac_calmgr_info_t *ac_info)
{
	phy_info_t *pi;
	phy_calmgr_info_t *cmn_info;

	ASSERT(ac_info);
	pi = ac_info->pi;
	cmn_info = ac_info->cmn_info;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* unregister from common */
	phy_calmgr_unregister_impl(cmn_info);

	phy_mfree(pi, ac_info, sizeof(phy_ac_calmgr_info_t));
}

static int
phy_ac_calmgr_init(phy_type_calmgr_ctx_t *ctx)
{
	phy_ac_calmgr_info_t *calmgri = (phy_ac_calmgr_info_t *) ctx;
	phy_info_t * pi = calmgri->pi;
	pi->interf->aci.ma_total = PHY_NOISE_MA_WINDOW_SZ * ACI_INIT_MA;
	pi->interf->badplcp_ma_total = PHY_NOISE_GLITCH_INIT_MA_BADPlCP *
		PHY_NOISE_MA_WINDOW_SZ;
	return BCME_OK;
}

static int
phy_ac_calmgr_prepare(phy_type_calmgr_ctx_t *ctx)
{
	BCM_REFERENCE(ctx);
	PHY_TRACE(("%s\n", __FUNCTION__));
	return BCME_OK;
}

static void
phy_ac_calmgr_cleanup(phy_type_calmgr_ctx_t *ctx)
{
	BCM_REFERENCE(ctx);
	PHY_TRACE(("%s\n", __FUNCTION__));
}

/* Check to see if a cal needs to be run */
static bool
phy_ac_wd_perical_schedule(phy_info_t *pi)
{
	if (pi->disable_percal) {
		return FALSE;
	}

	if (phy_papdcal_is_wfd_phy_ll_enable(pi->papdcali) && DCS_INPROG_PHY(pi)) {
		return FALSE;
	}

	if ((pi->phy_cal_mode == PHY_PERICAL_DISABLE) ||
		(pi->phy_cal_mode == PHY_PERICAL_MANUAL) ||
		(pi->cal_info->cal_suppress_count != 0)) {
		return FALSE;
	}

	if (GLACIAL_TIMEOUT(pi)) {
		return TRUE;
	} /* event : galcial timeout */
	else
		return FALSE;
}

/* watchdog callback */
static bool
phy_ac_calmgr_wd(phy_type_calmgr_ctx_t *ctx)
{
	phy_ac_calmgr_info_t *info = (phy_ac_calmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	/* Check to see if a cal needs to be run */
	if (phy_ac_wd_perical_schedule(pi)) {
		PHY_CAL(("wl%d: phy_ac_calmgr_wd: wd triggered cal\n"
			"(cal mode=%d, now=%d, prev_time=%d, gt=%d)\n",
			PI_INSTANCE(pi), pi->phy_cal_mode, PHYTIMER_NOW(pi),
			LAST_CAL_TIME(pi), GLACIAL_TIMER(pi)));
		wlc_phy_cal_perical(pi, PHY_PERICAL_WATCHDOG);
	}

	if (PHY_PAPDEN(pi) && ACMAJORREV_129(pi->pubpi->phy_rev) && !(pi->skip_wdpapd)) {
		wlc_phy_txpwr_papd_cal_acphy(pi);
	}
	return TRUE;
}

static void
BCMATTACHFN(phy_ac_calmgr_nvram_attach)(phy_ac_calmgr_info_t *calmgri)
{
	if (ACMAJORREV_GE40(calmgri->pi->pubpi->phy_rev)) {
		calmgri->pi->phy_cal_mode = PHY_PERICAL_SPHASE;
	}
	calmgri->enIndxCap = TRUE;
}

bool
phy_ac_calmgr_get_enIndxCap(phy_ac_calmgr_info_t *calmgri)
{
	return calmgri->enIndxCap;
}

static void
phy_ac_calmgr_init_cals(phy_info_t *pi, uint8 *searchmode, acphy_cal_result_t *accal,
	uint8 phase_id)
{
	phy_ac_calmgr_info_t *ci = pi->u.pi_acphy->calmgri;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	uint8 max_chains;

	/* Initializations */
	ci->enRx = 0;
	ci->enTx = 0;
	ci->idac_i = 0;
	ci->idac_q = 0;

	if (ACMAJORREV_4(pi->pubpi->phy_rev))
		ci->enIndxCap = FALSE;
	ci->fullcal = TRUE;

	phy_ac_chanmgr_cal_init(pi);
	phy_ac_radio_cal_init(pi);

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &(ci->enRx), &(ci->enTx),
	                        max_chains, max_chains);

#ifdef OCL
	if (PHY_OCL_ENAB(pi->sh->physhim)) {
		phy_ocl_disable_req_set(pi, OCL_DISABLED_CAL, TRUE, WLC_OCL_REQ_CAL);
	}
#endif

	/*
	 * Search-Mode Sanity Check for Tx-iqlo-Cal
	 *
	 * Notes: - "RESTART" means: start with 0-coeffs and use large search radius
	 *        - "REFINE"  means: start with latest coeffs and only search
	 *                    around that (faster)
	 *        - here, if channel has changed or no previous valid coefficients
	 *          are available, enforce RESTART search mode (this shouldn't happen
	 *          unless cal driver code is work-in-progress, so this is merely a safety net)
	 */
	if ((pi->radio_chanspec != accal->chanspec) ||
	    (accal->txiqlocal_coeffsvalid == 0)) {
		*searchmode = PHY_CAL_SEARCHMODE_RESTART;
	}

	/*
	 * If previous phase of multiphase cal was on different channel,
	 * then restart multiphase cal on current channel (again, safety net)
	 */
	if ((phase_id > PHY_CAL_PHASE_INIT)) {
		if (accal->chanspec != pi->radio_chanspec) {
			phy_calmgr_mphase_restart(pi->calmgri);
		}
	}

#ifdef WFD_PHY_LL_DEBUG
	ci->cal_phase_id = pi->cal_info->cal_phase_id;
	ci->start_time = hnd_time_us();
#endif

	/* Make the ucode send a CTS-to-self packet with duration set to 10ms. This
	 *  prevents packets from other STAs/AP from interfering with Rx IQcal
	 */
	/* XXX FIXME
	 *	if ((pi->mphase_cal_phase_id == PHY_CAL_PHASE_RXCAL)) {
	 *		wlapi_bmac_write_shm(pi->sh->physhim, M_CTS_DURATION, 10000);
	 *	}
	 */
	ci->tx_pwr_ctrl_state = pi->txpwrctrl;

	if (!ACMAJORREV_47(pi->pubpi->phy_rev)) {
		/* Send out cts2self */
		phy_ac_papdcal_cal_init(pi);
	}

	if ((ACMAJORREV_32(pi->pubpi->phy_rev) && ACMINORREV_2(pi)) ||
	    ACMAJORREV_33(pi->pubpi->phy_rev) ||
		(ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev))) {
	    /* Disable core2core sync */
		/* Martin thinks it is not logical that 6878 will keep c2c sync on during cal */
	    phy_ac_chanmgr_core2core_sync_setup(pi->u.pi_acphy->chanmgri, FALSE);
	}
	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		wlc_phy_set_txfgc_acphy(pi, 1, 0);
	}

	pi->u.pi_acphy->radar_cal_active = TRUE;
}

void
phy_ac_calmgr_clean(phy_info_t *pi)
{
	phy_ac_calmgr_info_t *ci = pi->u.pi_acphy->calmgri;
	uint8 phyrxchain;
	uint8 core;
	BCM_REFERENCE(phyrxchain);

	if ((PHY_IPA(pi)) && (ci->tx_pwr_ctrl_state == PHY_TPC_HW_ON) && (!TINY_RADIO(pi)) &&
		(!ACMAJORREV_128(pi->pubpi->phy_rev))) {
		phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
		FOREACH_ACTV_CORE(pi, phyrxchain, core) {
			MOD_PHYREGCEE(pi, EpsilonTableAdjust, core, epsilonOffset, 0);
		}
	}

	wlc_phy_txpwrctrl_enable_acphy(pi, ci->tx_pwr_ctrl_state);

	/* Restore Rx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, ci->enRx, ci->enTx);

	phy_ac_radio_cal_reset(pi, ci->idac_i, ci->idac_q);

	phy_ac_chanmgr_cal_reset(pi);

	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		wlc_phy_set_txfgc_acphy(pi, 0, 1);
	}

#ifdef ATE_BUILD
		PHY_INFORM(("===> Resuming MAC, after cal\n"));
#endif /* ATE_BUILD */

#ifdef OCL
	if (PHY_OCL_ENAB(pi->sh->physhim)) {
		phy_ocl_disable_req_set(pi, OCL_DISABLED_CAL, FALSE, WLC_OCL_REQ_CAL);
	}
#endif

#ifdef WFD_PHY_LL_DEBUG
	PHY_INFORM(("phase_id:%2d usec:%d\n", ci->cal_phase_id, hnd_time_us() - ci->start_time));
#endif
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		ci->enIndxCap = TRUE;
		wlc_phy_ac_gains_load(pi->u.pi_acphy->tbli);
	}
}

void
phy_ac_calmgr_singleshot(phy_info_t *pi, uint8 searchmode, acphy_cal_result_t *accal)
{
	/*
	 * SINGLE-SHOT Calibrations
	 *
	 *    Call all Cals one after another
	 *
	 *    Notes:
	 *    - if this proc is called with the phase state in IDLE,
	 *      we know that this proc was called directly rather
	 *      than via the mphase scheduler (the latter puts us into
	 *      INIT state); under those circumstances, perform immediate
	 *      execution over all cal tasks
	 *    - for better code structure, we would use the below mphase code for
	 *      sphase case, too, by utilizing an appropriate outer for-loop
	 */

	/* TO-DO: Ensure that all inits and cleanups happen here */
	/* carry out all phases "en bloc", for comments see the various phases below */
	uint8 multilo_cal_cnt;

#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif
	PHY_CAL(("phy_ac_calmgr_singleshot\n"));
	pi->cal_info->last_cal_time = pi->sh->now;
	accal->chanspec = pi->radio_chanspec;

	if (TINY_RADIO(pi) &&
	    READ_RADIO_REGFLD_20693(pi, PLL_CFGR1, 0, rfpll_monitor_need_refresh) == 1) {
		wlc_phy_radio_tiny_vcocal(pi);
	}

	if (ACMAJORREV_47_129_130_131_132(pi->pubpi->phy_rev)) {
		phy_ac_vcocal_singlephase(pi);
	}

	if ((ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) ||
		!ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
		wlc_phy_cal_txiqlo_acphy(pi, searchmode, FALSE, FALSE, 0);
		if (phy_txiqlocal_num_multilo(pi) != 0) {
			/* multi-point LO cal */
			/* phy_txiqlocal_num_multilo returns number of multi-point LO cals */
			for (multilo_cal_cnt = 1;
				multilo_cal_cnt <= phy_txiqlocal_num_multilo(pi);
				multilo_cal_cnt++) {
				/* for each multi-point LO cal, searchmode =
				 * PHY_CAL_SEARCHMODE_MULTILO
				 */
				wlc_phy_cal_txiqlo_acphy(pi, PHY_CAL_SEARCHMODE_MULTILO,
						FALSE, FALSE, multilo_cal_cnt);
			}
		}
	}

	/* XXX 4349BU XXX
	 * JIRA:SW4349-430 | Bypass Tx IQ Lo cal for now
	 */

	if (TINY_RADIO(pi)) {
		wlc_phy_tiny_static_dc_offset_cal(pi);
	} else if (ACPHY_TXCAL_PRERXCAL(pi)) {
		wlc_phy_cal_txiqlo_acphy(pi, searchmode, FALSE, TRUE, 0);
	}

	wlc_phy_txpwrctrl_idle_tssi_meas_acphy(pi);

	wlc_phy_cals_mac_susp_en_other_cr(pi, TRUE);

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
		phy_ac_dccal(pi);
	}

	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		phy_ac_dccal_init(pi);
		phy_ac_load_gmap_tbl(pi);
		phy_ac_dccal(pi);
		if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
			wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		}
		wlc_phy_cal_txiqlo_acphy(pi, searchmode, FALSE, FALSE, 0);
		wlc_phy_cal_rx_fdiqi_acphy(pi);

		/* 2G NB PAPD CAL */
		if (PHY_PAPDEN(pi)) {
			wlc_phy_do_papd_cal_acphy(pi, -1);
		}
		if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
			wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		}

	} else  {
		wlc_phy_cal_rx_fdiqi_acphy(pi);
	}

	wlc_phy_cals_mac_susp_en_other_cr(pi, FALSE);

	if (!ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
		if (!ACMAJORREV_32(pi->pubpi->phy_rev) &&
			!ACMAJORREV_33(pi->pubpi->phy_rev)) {
			phy_ac_dssf(pi->u.pi_acphy->rxspuri, TRUE);
		}
		if (PHY_PAPDEN(pi)) {
			wlc_phy_do_papd_cal_acphy(pi, -1);
		}
	}
	if (PHY_PAPDEN(pi) && ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev)) {
		if (ACMAJORREV_129(pi->pubpi->phy_rev) && (pi->u.pi_acphy->calmgri->fullcal)) {
			/* Used to indicate full cal */
			wlc_phy_do_papd_cal_acphy(pi, -2);
		} else {
			wlc_phy_do_papd_cal_acphy(pi, -1);
		}
		pi->skip_wdpapd = FALSE;
	}

#if defined(PHYCAL_CACHING)
	if (ctx) {
		ctx->valid = TRUE;  /* All Cache written by now */

		/* Don't call cache here as its now called from individual cals
		   This way it supports both single/multiphase cals
		   phy_cache_cal(pi);
		*/
	}
#else
	/* cache cals for restore on return to home channel */
	wlc_phy_scanroam_cache_txcal_acphy(pi->u.pi_acphy->txiqlocali, 1);
	wlc_phy_scanroam_cache_rxcal_acphy(pi->u.pi_acphy->rxiqcali, 1);
#endif /* PHYCAL_CACHING */

	pi->cal_info->fullphycalcntr++;
}

/* Times are in us for multiphase cals */
static void
phy_ac_calmgr_caltimes(phy_info_t *pi, phy_ac_calmgr_info_t *ci)
{
	multiphase_caltimes_t *cal_times = &ci->cal_times;

#ifdef WFD_PHY_LL
	/* Single-core on 20MHz channel */
	cal_times->vco = 600;
	cal_times->dc = 1000;
	cal_times->tssi = 7000;
	cal_times->tx = 3000;
	cal_times->rx = 300;
#else
	cal_times->vco = 300;
	cal_times->dc = 1000;

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		cal_times->tssi = 5000;
		cal_times->tx = 7000;
		cal_times->rx = 18000;
	} else {
		cal_times->tssi = 1550;
		cal_times->tx = 4400;
		cal_times->rx = 9500;
	}
#endif /* WFD_PHY_LL */
}

static void
phy_ac_calmgr_init_phase(phy_info_t *pi)
{
	/*
	 *   Housekeeping & txiqlo-cal init params
	 */

	/* remember time and channel of this cal event */
	pi->cal_info->last_cal_time     = pi->sh->now;
	pi->cal_info->u.accal.chanspec = pi->radio_chanspec;

	wlc_phy_cal_txiqlo_init_acphy(pi);

	/* move on */
	pi->cal_info->cal_phase_id++;
}

void
phy_ac_calmgr_multiphase(phy_info_t *pi, uint8 phase_id, uint8 searchmode)
{
/*
 * MULTI-PHASE CAL
 *
 *	 Carry out next step in multi-phase execution of cal tasks
 *   Issue the cals(switches) in order of enum as some of them don't have break
 *
 */

	phy_ac_calmgr_info_t *ci = pi->u.pi_acphy->calmgri;
	multiphase_caltimes_t *cal_times = &ci->cal_times;
	uint8 multilo_cal_cnt;

#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctx = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	PHY_CAL(("phy_ac_calmgr_multiphase\n"));

	switch (phase_id) {
	case PHY_CAL_PHASE_INIT:
		phy_ac_calmgr_init_phase(pi);
		// no break, continue with next cal

	case PHY_CAL_PHASE_VCOCAL:
		phy_ac_vcocal_multiphase(pi, cal_times->vco);
		break;

	case PHY_CAL_PHASE_DCCAL:
		if (phy_ac_dccal_multiphase_isen(pi)) {
			phy_ac_dccal_multiphase(pi, cal_times->dc);
			break;
		} else {
			pi->cal_info->cal_phase_id++;
			// no break, continue with next cal
		}

	case PHY_CAL_PHASE_IDLETSSI:
		phy_ac_tssical_idle_multiphase(pi, cal_times->tssi);
		break;

	case PHY_CAL_PHASE_TX:
		phy_ac_txiqlocal_multiphase(pi, searchmode, FALSE, cal_times->tx, 0);
		break;

	case PHY_CAL_PHASE_TX_MULTILO:
		if ((phy_txiqlocal_num_multilo(pi) != 0)) {
			/* phy_txiqlocal_num_multilo returns number of multi-point LO cals */
			for (multilo_cal_cnt = 1;
				multilo_cal_cnt <= phy_txiqlocal_num_multilo(pi);
				multilo_cal_cnt++) {
				wlc_phy_cal_txiqlo_acphy(pi, PHY_CAL_SEARCHMODE_MULTILO,
						FALSE, FALSE, multilo_cal_cnt);
			}
			pi->cal_info->cal_phase_id++;
			break;
		} else {
			pi->cal_info->cal_phase_id++;
			// no break
		}

	case PHY_CAL_PHASE_TXPRERX:
		phy_ac_txiqlocal_multiphase(pi, searchmode, 1, cal_times->tx, 0);
		break;

	case PHY_CAL_PHASE_RXCAL:
		phy_ac_rxiqcal_multiphase(pi, cal_times->rx);
		break;

	case PHY_CAL_PHASE_PAPDCAL:
		if (pi->cal_info->cal_core == -1 || !ACMAJORREV_129(pi->pubpi->phy_rev)) {
			/* do PAPD cal for all cores in one phase */
			phy_ac_papdcal_multiphase(pi, -1);
		} else if ((pi->cal_info->cal_core >= 0) &&
				(pi->cal_info->cal_core < PHYCORENUM((pi)->pubpi->phy_corenum))) {
			/* Per-core multiphase PAPD cal only supported for 6710 */
			phy_ac_papdcal_multiphase(pi, pi->cal_info->cal_core);
			if (pi->cal_info->cal_core == PHYCORENUM((pi)->pubpi->phy_corenum)-1) {
				pi->cal_info->cal_core = 0;
			} else {
				pi->cal_info->cal_core++;
			}
		}
		pi->skip_wdpapd = FALSE;
		break;

	default:
		ASSERT(0);
		phy_calmgr_mphase_reset(pi->calmgri);
		break;
	}

	if (pi->cal_info->cal_phase_id >= PHY_CAL_PHASE_DONE) {
		phy_calmgr_mphase_reset(pi->calmgri);

#if defined(PHYCAL_CACHING)
		if (ctx) ctx->valid = TRUE;  /* All Cache written by now */
#endif
	}
}

void
wlc_phy_cals_acphy(phy_type_calmgr_ctx_t *ctx, uint8 legacy_caltype, uint8 searchmode)
{
	/* XXX FIXME:
	 * ToDo:
	 *       - CTS-to-self for Rx cal / all cals? see nphy
	 *       - radar protection ? covered already?
	 */
	phy_ac_calmgr_info_t *info = (phy_ac_calmgr_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint8 phase_id = pi->cal_info->cal_phase_id;
	uint8 tx_pwr_ctrl_state = pi->txpwrctrl;
	acphy_cal_result_t *accal = &pi->cal_info->u.accal;
	bool suspend = FALSE;
	uint32 cal_start_time;
	uint16 cal_exec_time;
	bool hwrssi_en_ori; // = 0;

#if defined(PHYCAL_CACHING)
	ch_calcache_t *ctxcatch = wlc_phy_get_chanctx(pi, pi->radio_chanspec);
#endif

	BCM_REFERENCE(legacy_caltype);

	PHY_CAL(("wl%d: Running ACPHY periodic calibration: Searchmode: %d. phymode: 0x%x \n",
	         pi->sh->unit, searchmode, phy_get_phymode(pi)));

	if (NORADIO_ENAB(pi->pubpi)) {
		return;
	}

#ifdef ATE_BUILD
	PHY_INFORM(("===> wl%d:"
		"Running ACPHY periodic calibration: Searchmode: %d. phymode: 0x%x \n",
		pi->sh->unit, searchmode, phy_get_phymode(pi)));
#endif /* ATE_BUILD */

	/* -----------------
	 *  Initializations
	 * -----------------
	 */

	/* Exit immediately if we are running on Quickturn */
	if (ISSIM_ENAB(pi->sh->sih)) {
		phy_calmgr_mphase_reset(pi->calmgri);
		return;
	}

	/* skip cal if phy is muted */
	if (PHY_MUTED(pi) && !TINY_RADIO(pi)) {
		return;
	}

	/* Suspend MAC if haven't done so */
	wlc_phy_conditional_suspend(pi, &suspend);

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
		wlc_phy_txpwrctrl_enable_acphy(pi, PHY_TPC_HW_OFF);
	}
	// save the hwrssi setting and disable hwrssi
	if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		hwrssi_en_ori = phy_ac_noise_get_hwrssi_en(pi);
		phy_ac_rssi_mode_select(pi, 0); // 0 = agc based rssi, 1 is for hwrssi.
	} else {
		hwrssi_en_ori = 0;
	}

	/* Check for hang PHY CAL Status */
	wlc_phy_iqlocal_state_check_acphy(pi);

	cal_start_time = OSL_SYSUPTIME();
	phy_utils_phyreg_enter(pi);

	/* Disable classifier, and init/setup */
	if (pi->pubpi->phy_rev >= 47)
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);
	phy_ac_calmgr_init_cals(pi, &searchmode, accal, phase_id);

	/* -------------------
	 *  Calibration Calls
	 * -------------------
	 */

	PHY_CAL(("wlc_phy_cals_acphy: Time=%d, LastTi=%d, SrchMd=%d, PhIdx=%d,"
		" Chan=%d, LastCh=%d, vld=%d\n",
		pi->sh->now, pi->cal_info->last_cal_time, searchmode, phase_id,
		pi->radio_chanspec, accal->chanspec,
		accal->txiqlocal_coeffsvalid));

	if (ACMAJORREV_129(pi->pubpi->phy_rev) && (legacy_caltype != PHY_PERICAL_FULL)) {
		pi->u.pi_acphy->calmgri->fullcal = FALSE;
	} else {
		pi->u.pi_acphy->calmgri->fullcal = TRUE;
	}
	if (phase_id == PHY_CAL_PHASE_IDLE) {
		cal_exec_time = 29000;
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
		    (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
		     !ACMAJORREV_128(pi->pubpi->phy_rev))) {
			if (pi->sh->up) {
				wlc_phy_cts2self(pi, cal_exec_time);
			}
		}
		phy_ac_calmgr_singleshot(pi, searchmode, accal);
	} else {
		phy_ac_calmgr_multiphase(pi, phase_id, searchmode);
	}

	/* Call it after all the cals (single/multi) so that we are sure
	  there is no call to cts2self after noise-cal
	*/
	if (!ACMAJORREV_130(pi->pubpi->phy_rev))
	  phy_ac_rxgcrs_cal(pi->u.pi_acphy->rxgcrsi);

	/* ----------
	 *  Cleanups
	 * ----------
	 */
	phy_ac_calmgr_clean(pi);

	if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
		/* After cal cleanup, with final radio config and with c2c_sync re-enabled,
		 * re-calibrate DC (BCAWLAN-214876)
		 */
		phy_ac_dccal(pi);
	}

	if (pi->pubpi->phy_rev >= 47) {
		phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	}
	if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
		wlc_phy_txpwrctrl_enable_acphy(pi, tx_pwr_ctrl_state);
	}

	/* Resume MAC */
	phy_utils_phyreg_exit(pi);
	wlc_phy_conditional_resume(pi, &suspend);

	pi->cal_dur += OSL_SYSUPTIME() - cal_start_time;

	/* If TX iqcal hangs, issue mac-phy reset to get out of bad state */
	if ((READ_PHYREG(pi, iqloCalCmd) & 0xc000) &&
	    ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {

		/* Reset multiphase cal */
		phy_calmgr_mphase_reset(pi->calmgri);

		/* Trigger PHY-crash fatal error
		 * to re-init
		 */
		PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :",
				__FUNCTION__));
		PHY_FATAL_ERROR_MESG(("TXIQLO cal failed \n"));
		PHY_FATAL_ERROR(pi, PHY_RC_TXIQLO_CAL_FAILED);

#if defined(PHYCAL_CACHING)
		/* invalidate the cal result if txiqcal hangs */
		if (ctxcatch) {
			ctxcatch->valid = FALSE;
		}
#endif /* PHYCAL_CACHING */
	}

	// restore hwrssi
	if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
		phy_ac_rssi_mode_select(pi, hwrssi_en_ori);
		phy_ac_rxgcrs_cal(pi->u.pi_acphy->rxgcrsi);
	}
}

void
wlc_phy_low_rate_adc_enable_acphy(phy_info_t *pi, bool enable)
{
	uint8 core, mode;
	MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 1);
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, lowRateTssi, core, lb_tssi_adc_lowrate_mode, enable);
		MOD_PHYREGCE(pi, lowRateTssi2, core, lb_tssi_adc_lowrate_mode, enable);
	}

	MOD_PHYREG(pi, sdfeClkGatingCtrl, disableRxStallonTx, enable);
	MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 0);

	/* Disable - not needed yet */
	if (ACMAJORREV_129(pi->pubpi->phy_rev) && 0) {
		mode = enable ? pi->u.pi_acphy->sromi->srom_low_adc_rate_en : 0;
	    phy_ac_chanmgr_low_rate_tssi_rfseq_fiforst_dly(pi, enable);
		wlc_phy_set_rfseqext_tbl(pi, mode);
	}
}

void wlc_phy_set_txfgc_acphy(phy_info_t *pi, uint8 set_val, uint8 exp_val)
{
	bool suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (!suspend)
		wlapi_suspend_mac_and_wait(pi->sh->physhim);

	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		if (set_val == 0) {
			ASSERT(READ_PHYREGFLD(pi, fineclockgatecontrol,
			forcetxgatedClksOn) == exp_val);
		}
		MOD_PHYREG(pi, fineclockgatecontrol, forcetxgatedClksOn, set_val);
	}

	if (!suspend)
		wlapi_enable_mac(pi->sh->physhim);
}

/* Special function to detect hanging IQ Cal PHYREG status and overwrite it if necessary.
 * SWWLAN-144335
 */
static void
wlc_phy_iqlocal_state_check_acphy(phy_info_t *pi)
{
	uint16 iqlo_cal_en = 0;

	iqlo_cal_en = phy_utils_read_phyreg(pi, ACPHY_iqloCalCmdGctl(pi->pubpi.phy_rev)) &
			ACPHY_iqloCalCmdGctl_iqlo_cal_en_MASK(pi->pubpi.phy_rev);

	if (iqlo_cal_en) {
		PHY_ERROR(("wlc_phy_iqlocal_state_check_acphy: iqlo cal still on.\n"));
		ASSERT(0);
		phy_utils_and_phyreg(pi, ACPHY_iqloCalCmdGctl(pi->pubpi->phy_rev),
			(uint16)~ACPHY_iqloCalCmdGctl_iqlo_cal_en_MASK(pi->pubpi->phy_rev));
	}
}
