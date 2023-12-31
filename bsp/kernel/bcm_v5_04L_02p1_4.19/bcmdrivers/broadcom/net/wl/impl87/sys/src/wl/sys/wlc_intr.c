/*
 * Common (OS-independent) portion of
 * Broadcom 802.11abgn Networking Device Driver
 *
 * @file
 * @brief
 * Interrupt/worklet handlers of common driver. Shared by BMAC driver, High driver,
 * and Full driver.
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
 * $Id: wlc_intr.c 807793 2022-01-31 22:46:38Z $
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <d11_cfg.h>
#include <siutils.h>
#include <wlioctl.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_channel.h>
#include <wlc_rm.h>
#include <wlc.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_tx.h>
#include <wlc_mbss.h>
#include <wlc_macdbg.h>
#include <wl_export.h>
#include <wlc_ap.h>
#include <wlc_11h.h>
#include <wlc_quiet.h>
#include <wlc_hrt.h>
#ifdef WLWNM_AP
#include <wlc_wnm.h>
#endif /* WLWNM_AP */
#ifdef WLDURATION
#include <wlc_duration.h>
#endif

#ifdef WL_BCNTRIM
#include <wlc_bcntrim.h>
#endif
#include <wlc_hw.h>
#include <wlc_phy_hal.h>
#include <wlc_perf_utils.h>
#include <wlc_ltecx.h>
#ifdef WLHEB
#include <wlc_heb.h>
#endif /* WLHEB */
#include <wlc_rx.h>

#include <phy_noise_api.h>

#if defined(WLDIAG)
#include <wlc_diag.h> /* for e.g. wlc_diag_lb_ucode_fake_txstatus() */
#endif /* WLDIAG */

#ifdef WL_AIR_IQ
#include <wlc_airiq.h>
#endif /*  WL_AIR_IQ */

#if defined(BCM_CSIMON)
#include <wlc_csimon.h>
#endif /* BCM_CSIMON */

#if defined(WLC_OFFLOADS_TXSTS) || defined(WLC_OFFLOADS_RXSTS)
#include <wlc_offld.h>
#endif /* WLC_OFFLOADS_TXSTS || WLC_OFFLOADS_RXSTS */
#if defined(STS_XFER)
#include <wlc_sts_xfer.h>
#endif /* STS_XFER */

#if defined(BCMHWA) && (defined(HWA_RXFILL_BUILD) || (defined(HWA_TXSTAT_BUILD) && \
	!defined(DONGLEBUILD)))
#include <hwa_lib.h>
#include <hwa_export.h>
#endif /* BCMHWA && (HWA_RXFILL_BUILD || (HWA_TXSTAT_BUILD && !DONGLEBUILD)) */
#include <wlc_pmq.h>
#include <wlc_twt.h>
#include <wlc_apps.h>

#ifdef BCMDBG
static const bcm_bit_desc_t int_flags[] = {
	{MI_MACSSPNDD,		"MACSSPNDD"},
	{MI_BCNTPL,		"BCNTPL"},
	{MI_TBTT,		"TBTT"},
	{MI_BCNSUCCESS,		"BCNSUCCESS"},
	{MI_BCNCANCLD,		"BCNCANCLD"},
	{MI_ATIMWINEND,		"ATIMWINEND"},
	{MI_PMQ,		"PMQ"},
	{MI_ALTTFS,		"MI_ALTTFS"},
	{MI_NSPECGEN_1,		"NSPECGEN_1"},
	{MI_MACTXERR,		"MACTXERR"},
	{MI_PMQERR,		"PMQERR"},
	{MI_PHYTXERR,		"PHYTXERR"},
	{MI_PME,		"PME"},
	{MI_GP0,		"GP0"},
	{MI_GP1,		"GP1"},
	{MI_DMAINT,		"DMAINT"},
	{MI_TXSTOP,		"TXSTOP"},
	{MI_CCA,		"CCA"},
	{MI_BG_NOISE,		"BG_NOISE"},
	{MI_DTIM_TBTT,		"DTIM_TBTT"},
	{MI_PRQ,		"PRQ"},
	{MI_HEB,		"HEB"},
	{MI_BT_RFACT_STUCK,	"BT_RFACT_STUCK"},
	{MI_BT_PRED_REQ,	"BT_PRED_REQ"},
	{MI_RFDISABLE,		"RFDISABLE"},
	{MI_TFS,		"TFS"},
#ifdef WL_PSMX
	{MI_PSMX,		"PSMX"},
#else
	{MI_BUS_ERROR,		"BUS_ERROR"},
#endif
	{MI_TO,			"TO"},
	{MI_P2P,		"P2P"},
	{MI_HWACI_NOTIFY,	"HWACI_NOTIFY"},
	{MI_PRETWT,		"PRETWT"},
	{0, NULL}
};
#endif /* BCMDBG */

static uint8 wlc_process_per_fifo_intr(wlc_info_t *wlc, bool bounded, wlc_worklet_info_t *worklet);
#if !defined(DMA_TX_FREE)
static uint32 wlc_get_fifo_interrupt(wlc_info_t *wlc, uint8 FIFO);
#endif

/**
 * second-level interrupt processing
 *   Return TRUE if another worklet needs to be re-scheduled. FALSE otherwise.
 *   Param 'bounded' indicates if applicable loops should be bounded.
 *   Param 'worklet' returns info such as how many packets have been received/processed.
 */
bool BCMFASTPATH
wlc_worklet(wlc_info_t *wlc, bool bounded, wlc_worklet_info_t *worklet)
{
	uint32 macintstatus;
#if defined(WL_PSMX)
	uint32 macintstatus_x = 0;
#endif
#ifdef WLC_TXPWRCAP
	int cellstatus;
#endif
	wlc_hw_info_t *wlc_hw = wlc->hw;
	d11regs_t *regs = wlc->regs;
	bool fatal = FALSE;

#if defined(WL_PSMX)
	macintstatus = wlc_hw->macintstatus;
	if ((macintstatus & MI_PSMX) && (D11REV_GE(wlc_hw->corerev, 65))) {
		macintstatus_x = GET_MACINTSTATUS_X(wlc->osh, wlc_hw);
#if defined(WL_AIR_IQ)
		if (macintstatus_x & MIX_AIRIQ) {
			SET_MACINTSTATUS_X(wlc->osh, wlc_hw, MIX_AIRIQ);
			/* Air-IQ data to handle */
			wlc_airiq_vasip_fft_worklet(wlc);
		}
#endif /* WL_AIR_IQ */
	}
#else /* WL_PSMX */
	if (wlc_hw->macintstatus & MI_BUS_ERROR) {
		WL_ERROR(("%s: MI_BUS_ERROR\n",
				__FUNCTION__));

		/* uCode reported bus error. Clear pending errors */
		si_clear_backplane_to(wlc->pub->sih);

		/* Initiate recovery */
		if (wlc_bmac_report_fatal_errors(wlc->hw, WL_REINIT_RC_AXI_BUS_ERROR)) {
			return FALSE;
		}
	}
#endif /* WL_PSMX */

	if (DEVICEREMOVED(wlc)) {
		WL_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		wl_down(wlc->wl);
		return FALSE;
	}

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		wlc_offload_process_m2m_intstatus(wlc);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		wlc_sts_xfer_process_m2m_intstatus(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

#if !defined(DONGLEBUILD) && defined(BCM_CSIMON)
	if (CSIMON_ENAB(wlc->pub)) {
		wlc_csimon_worklet(wlc);
	}
#endif /* BCM_CSIMON */
#if !defined(DONGLEBUILD) && defined(WL_AIR_IQ)
	if (AIRIQ_ENAB(wlc->pub)) {
		wlc_airiq_m2m_dpc(wlc);
	}
#endif
	/* grab and clear the saved software intstatus bits */
	macintstatus = wlc_hw->macintstatus;
	wlc_hw->macintstatus = 0;
#ifdef BCMLTECOEX
	/* Ignore all PSM generated interrupts while in WCI Loopback mode */
	if (BCMLTECOEX_ENAB(wlc->pub) && wlc->ltecx->mws_debug_mode_prev)
		return 0;
#endif /* BCMLTECOEX */

	/* For rev128 and higher this interrupt is used for PreTWT: */
	if ((macintstatus & MI_HWACI_NOTIFY) && (D11REV_LT(wlc_hw->corerev, 128))) {
		wlc_phy_hwaci_mitigate_intr(wlc_hw->band->pi);
	}

	WLDURATION_ENTER(wlc, DUR_DPC);

#ifdef BCMDBG
	if (WL_TRACE_ON()) {
		char flagstr[128];

		uint unit = wlc->pub->unit;
		bcm_format_flags(int_flags, macintstatus, flagstr, sizeof(flagstr));
		WL_PRINT(("wl%d: %s: macintstatus 0x%x %s\n",
			unit, __FUNCTION__, macintstatus, flagstr));
	}
#endif /* BCMDBG */

#ifdef STA
	if (macintstatus & MI_BT_PRED_REQ)
		wlc_bmac_btc_update_predictor(wlc_hw);
#endif /* STA */

	/* TBTT indication */
	/* ucode only gives either TBTT or DTIM_TBTT, not both */
	if (macintstatus & (MI_TBTT | MI_DTIM_TBTT)) {
#ifdef RADIO_PWRSAVE
		/* Enter power save mode */
		wlc_radio_pwrsave_enter_mode(wlc, ((macintstatus & MI_DTIM_TBTT) != 0));
#endif /* RADIO_PWRSAVE */
#ifdef MBSS
		if (MBSS_ENAB(wlc->pub)) {
			(void)wlc_mbss16_tbtt(wlc, macintstatus);
		}
#endif /* MBSS */
		wlc_tbtt(wlc, regs);
	}
	if (macintstatus & MI_TTTT) {
#ifdef WLWNM_AP
		if (AP_ENAB(wlc->pub) && WLWNM_ENAB(wlc->pub)) {
			/* Process TIMBC sendout frame indication */
			wlc_wnm_tttt(wlc);
		}
#endif /* WLWNM_AP */

#ifdef STA
		/* MI_TTTT interrupt is generated by uCode in
		 * case of STA when corresponding SHM is configured by driver.
		 */
		if (wlc->stas_associated) {
			wlc_watchdog_process_ucode_sleep_intr(wlc);
		} else {
			/* could not execute watchdog now, cleanup states for next use */
			mboolclr(wlc->wd_state, DEFERRED_WLC_WD | DEFERRED_PHY_WD);
			wlc_user_wake_upd(wlc, WLC_USER_WAKE_REQ_WDWAITBCN, FALSE);
		}
#endif /* STA */
	}

#ifdef WL_BCNTRIM
	if (WLC_BCNTRIM_ENAB(wlc->pub)) {
		if (macintstatus & MI_BCNTRIM_RX) {
			wlc_bcntrim_recv_process_partial_beacon(wlc->bcntrim);
			macintstatus &= ~MI_BCNTRIM_RX;
		}
	}
#endif

	/* BCN template is available */
	/* ZZZ: Use AP_ACTIVE ? */
	if (macintstatus & MI_BCNTPL) {
		if (AP_ENAB(wlc->pub) && (!APSTA_ENAB(wlc->pub) || wlc->aps_associated)) {
			WL_APSTA_BCN(("wl%d: wlc_worklet: template -> wlc_update_beacon()\n",
			              wlc->pub->unit));
			wlc_update_beacon(wlc);
		}
	}

	/* PMQ entry addition */
	if (macintstatus & MI_PMQ) {
#ifdef AP
		if (wlc_pmq_processpmq(wlc_hw, bounded))
			wlc_hw->macintstatus |= MI_PMQ;
#endif
	}

#ifdef WLHEB
	/* HEB (Hardware Event Block) interrupt */
	if (macintstatus & MI_HEB) {
		wlc_heb_intr_process(wlc->hebi);
	}
#endif /* WLHEB */

	/* tx status */
	if (macintstatus & MI_TFS) {
		WLDURATION_ENTER(wlc, DUR_DPC_TXSTATUS);
#if defined(BCMHWA) && defined(HWA_TXSTAT_BUILD)
		if (HWA_ENAB(wlc->pub)) {
#ifdef DONGLEBUILD
			WL_ERROR(("ERROR: Got MI_TFS intr when HWA 4a enabled\n"));
#else /* !DONGLEBUILD */
			if (hwa_wlc_txstat_process(WL_HWA_DEVP(wlc))) {
				wlc_hw->macintstatus |= MI_TFS;
			}
#endif /* !DONGLEBUILD */
		} else
#elif defined(STS_XFER_TXS)
		if (STS_XFER_TXS_ENAB(wlc_hw->wlc->pub)) {
			if (wlc_sts_xfer_txs_process(wlc_hw->wlc, bounded, &fatal)) {
				wlc_hw->macintstatus |= MI_TFS;
			}
		} else
#endif /* STS_XFER_TXS */
		{
			if (wlc_bmac_txstatus(wlc_hw, bounded, &fatal)) {
				wlc_hw->macintstatus |= MI_TFS;
			}
		}
		WLDURATION_EXIT(wlc, DUR_DPC_TXSTATUS);
		if (fatal) {
			WL_ERROR(("wl%d: %s HAMMERING fatal txs err\n",
				wlc_hw->unit, __FUNCTION__));
			if (wlc_check_assert_type(wlc, WL_REINIT_RC_INV_TX_STATUS)) {
				goto out;
			} else {
				goto exit;
			}
		}
	}

	/* ATIM window end */
	if (macintstatus & MI_ATIMWINEND) {
		/*
		 * OR QValid bits into MACCOMMAND:
		 * - DirFrmQValid set after scrub at TBTT
		 * - BcMcFrmQValid set after BcMc ATIM tx status processed
		 * [XXX - post BcMc ATIM at queue front by swapping]
		 */
		W_REG(wlc->osh, D11_MACCOMMAND(wlc), wlc->qvalid);
		wlc->qvalid = 0;
	}

	/* phy tx error */
#if defined(PHYTXERR_DUMP)
	if (macintstatus & MI_PHYTXERR) {
		WLCNTINCR(wlc->pub->_cnt->txphyerr);
		WL_PRINT(("wl%d: PHYTX error\n", wlc_hw->unit));
		wlc_dump_phytxerr(wlc, 0);
	}
#endif /* PHYTXERR_DUMP */

#ifdef WLRXOV
	if (macintstatus & MI_RXOV) {
		if (WLRXOV_ENAB(wlc->pub))
			wlc_rxov_int(wlc);
	}
#endif

	/* received data or control frame, MI_DMAINT is indication of RX_FIFO interrupt */
	if (macintstatus & MI_DMAINT) {
		if ((BUSTYPE(wlc_hw->sih->bustype) == PCI_BUS) &&
		    wlc_hw->dma_lpbk) {
			wlc_bmac_recover_pkts(wlc_hw->wlc, TX_DATA_FIFO);
			wlc_bmac_dma_rxfill(wlc_hw, RX_FIFO);
		} else
		{
#ifdef WLDIAG /* ucode in loopback mode does not generate txstatus, thus no MI_TFS irq \
	*/
			if (wlc->ucode_loopback_test) {
				wlc_diag_lb_ucode_fake_txstatus(wlc_hw->wlc);
			}
#endif /* WLDIAG */
			WLDURATION_ENTER(wlc, DUR_DPC_RXFIFO);

			if (wlc_process_per_fifo_intr(wlc, bounded, worklet)) {
				wlc_hw->macintstatus |= MI_DMAINT;
			}
			WLDURATION_EXIT(wlc, DUR_DPC_RXFIFO);
		}
	}

	/* TX FIFO suspend/flush completion */
	if (macintstatus & MI_TXSTOP) {
		if (wlc_bmac_tx_fifo_suspended(wlc_hw, TX_DATA_FIFO)) {
			wlc_txstop_intr(wlc);
		}
	}

	/* noise sample collected */
	if (macintstatus & MI_BG_NOISE) {
		WL_NONE(("wl%d: got background noise samples\n", wlc_hw->unit));
		phy_noise_sample_intr(wlc_hw->band->pi);
	}

#if BAND6G || defined(WL_OCE_AP)
	if (macintstatus & MI_BCNSUCCESS) {
		/* start timer for upr or fd in 6G */
		wlc_ap_start_upr_fd_timer(wlc);
	}
#endif /* BAND6G || WL_OCE_AP */

	if (macintstatus & MI_GP0) {
		uint16 psm_assert_reason;
		if ((M_ASSERT_REASON(wlc) != 0xFFFF /* INVALID */) &&
			((psm_assert_reason = wlc_bmac_read_shm(wlc->hw, M_ASSERT_REASON(wlc)))
				!= 0)) {
			WL_PRINT(("\nUcode asserted with reason code %d\n", psm_assert_reason));
			wlc_dump_ucode_fatal(wlc, PSM_FATAL_PSMWD);
			ROMMABLE_ASSERT(0);
		} else {
			WL_PRINT(("wl%d: PSM microcode watchdog fired at %d (seconds)."
				" Resetting.\n",
				wlc->pub->unit, wlc->pub->now));
			wlc_dump_ucode_fatal(wlc, PSM_FATAL_PSMWD);

			WLCNTINCR(wlc->pub->_cnt->psmwds);
#ifndef DONGLEBUILD
#if defined(BCMDBG_ASSERT)
			if (g_assert_type == 2) {
				/* We needs dump files when PSM watchdog happened,
				 * So change the g_assert_type to 3.
				 */
				g_assert_type = 3;
			}
#endif
			/* wlc_iovar_dump(wlc, "ucode_fatal", strlen("ucode_fatal"), NULL, 2000); */
#endif /* DONGLEBUILD */
			WL_ERROR(("wl%d:%s PSM Watchdog!\n", wlc->pub->unit, __FUNCTION__));
			ASSERT(0);
			if (wlc_check_assert_type(wlc, WL_REINIT_RC_PSM_WD) == TRUE) {
				goto out;
			} else {

				goto exit;
			}
		}
	}

#if defined(WL_PSMX)
	if (macintstatus_x & MI_GP0) {
		wlc_dump_psmx_fatal(wlc, PSMX_FATAL_PSMWD);

		WLCNTINCR(wlc->pub->_cnt->psmxwds);
#ifndef DONGLEBUILD
#if defined(BCMDBG_ASSERT)
		if (g_assert_type == 2) {
			/* We needs dump files when PSM watchdog happened,
			 * So change the g_assert_type to 3.
			 */
			g_assert_type = 3;
		}
#endif
		/* wlc_iovar_dump(wlc, "ucode_fatal", strlen("ucode_fatal"), NULL, 2000); */
#endif /* DONGLEBUILD */
		WL_ERROR(("wl%d:%s PSMX Watchdog!\n", wlc->pub->unit, __FUNCTION__));
		ASSERT(0);
		if (wlc_check_assert_type(wlc, WL_REINIT_RC_PSM_WD) == TRUE) {
			goto out;
		} else {
			goto exit;
		}
	}
#endif /* WL_PSMX */

#ifdef WAR4360_UCODE
	if (wlc_hw->need_reinit) {
		/* big hammer */
		WL_ERROR(("wl%d: %s: need to reinit() %d, Big Hammer\n",
			wlc->pub->unit, __FUNCTION__, wlc_hw->need_reinit));
		WLC_FATAL_ERROR(wlc);
	}
#endif /* WAR4360_UCODE */
	/* gptimer timeout */
	if (macintstatus & MI_TO) {
		wlc_hrt_isr(wlc->hrti);
	}

	if ((macintstatus & MI_PRETWT) && (D11REV_GE(wlc_hw->corerev, 128))) {
		wlc_twt_intr(wlc->twti);
	}

#ifdef STA
	if (macintstatus & MI_RFDISABLE) {
		wlc_rfd_intr(wlc);
	}
#endif /* STA */

	if (macintstatus & MI_P2P)
		wlc_p2p_bmac_int_proc(wlc_hw);

#if defined(WLC_TXPWRCAP) || defined(GPIO_TXINHIBIT)
	if (macintstatus & MI_BT_PRED_REQ) {
		macintstatus &= ~MI_BT_PRED_REQ;
#ifdef GPIO_TXINHIBIT
		wlc_bmac_notify_gpio_tx_inhibit(wlc);
#endif
#ifdef WLC_TXPWRCAP
		cellstatus = (wlc_bmac_read_shm(wlc_hw, M_LTECX_STATE(wlc)) &
			(3 << C_LTECX_ST_LTE_ACTIVE)) >> C_LTECX_ST_LTE_ACTIVE;
		wlc_channel_txcap_cellstatus_cb(wlc_hw->wlc->cmi, cellstatus);
#if defined(BCMLTECOEX) && defined(OCL)
		if (OCL_ENAB(wlc->pub) && BCMLTECOEX_ENAB(wlc->pub) &&
				(wlc->ltecx->ocl_iovar_set != 0))
			wlc_lte_ocl_update(wlc->ltecx, cellstatus);
#endif /* BCMLTECOEX & OCL */
#ifdef BCMLTECOEX
	if (BCMLTECOEX_ENAB(wlc->pub)) {
		wlc_ltecx_ant_update(wlc->ltecx);
	}
#endif /* BCMLTECOEX */
#endif /* WLC_TXPWRCAP */
	}
#endif /* WLC_TXPWRCAP || GPIO_TXINHIBIT */

	/* send any enq'd tx packets. Just makes sure to jump start tx */
	if (WLC_TXQ_OCCUPIED(wlc)) {
		wlc_send_q(wlc, wlc->active_queue);
	}

	/* driver may have gone down due to MPC in previous calls, but if not, check PS state */
	ASSERT(!wlc_hw->up || wlc_ps_check(wlc));

#if defined(BCMDBG)
	wlc_update_perf_stats(wlc, WLC_PERF_STATS_DPC);
#endif

#if !defined(BCMNODOWN)
out:
	;
#endif /* !BCMNODOWN */

	/* make sure the bound indication and the implementation are in sync */
	ASSERT(bounded == TRUE || wlc_hw->macintstatus == 0);

exit:
	if (wlc_hw->macintstatus != 0) {
		wlc_radio_upd(wlc);
	}
	WLDURATION_EXIT(wlc, DUR_DPC);

	/* it isn't done and needs to be resched if macintstatus is non-zero */
	return (wlc_hw->macintstatus != 0);
} /* wlc_worklet */

/**
 * wlc_intr_process_rxfifo_interrupts:
 * On arrival of MI_DMA or M2M (PhyRx Status) interrupts, if FIFO specific interrupt status register
 * indicates arrival of new packets we will set Rx packets pending bitmap (wlc_hw->dma_rx_intstatus)
 * wlc_hw->dma_rx_intstatus bitmap is used by WLAN Rx worklet (wlc_dpc) to drive WLAN Rx
 * processing.
 *
 * On arrival of PhyRx Status Interrupt (STS_FIFO or M2M), set Rx FIFO-0/1 and Rx FIFO-2 in
 * wlc_hw->dma_rx_intstatus irrespective of corresponding FIFO interrupt status to process
 * Rx packets (rx_list_t) with PhyRx Status pending.
 */
void BCMFASTPATH
wlc_intr_process_rxfifo_interrupts(wlc_info_t *wlc, bool m2m_intr)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	osl_t *osh = wlc_hw->osh;

#if !defined(DMA_TX_FREE)
	/*
	 * For corerevs >= 5, only fifo interrupt enabled is I_RI in RX_FIFO.
	 * If MI_DMAINT is set, assume it is set and clear the interrupt.
	 */
	if (BCMSPLITRX_ENAB()) {
		/* HWA will handle the RX-FIFO 0/1 */
#if !defined(BCMHWA) || !defined(HWA_RXFILL_BUILD)
		/* check for fif0-0 intr */
		if (wlc_get_fifo_interrupt(wlc, RX_FIFO)) {

			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;

			/* Clear Rx FIFO-0 interrupt status */
			W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, RX_FIFO)->intstatus),
				DEF_RXINTMASK);
		}

		/* check for fif0-1 intr */
		if ((PKT_CLASSIFY_EN(RX_FIFO1) || RXFIFO_SPLIT()) &&
			wlc_get_fifo_interrupt(wlc, RX_FIFO1)) {

			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;

			/* Clear Rx FIFO-1 interrupt status */
			W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, RX_FIFO1)->intstatus),
					DEF_RXINTMASK);
		}

#endif /* !BCMHWA || !HWA_RXFILL_BUILD */

		/* HWA doesn't handle the FIFO2 */
		/* check for fif0-2 intr */
		if (PKT_CLASSIFY_EN(RX_FIFO2) && wlc_get_fifo_interrupt(wlc, RX_FIFO2)) {

			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;

			/* Clear Rx FIFO-2 interrupt status */
			W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, RX_FIFO2)->intstatus),
				DEF_RXINTMASK);
		}

#if defined(STS_XFER_PHYRXS)
		if ((STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub) &&
				wlc_get_fifo_interrupt(wlc, STS_FIFO)) ||
			(STS_XFER_PHYRXS_M2M_ENAB(wlc->pub) && m2m_intr)) {
			/* Received PhyRx Status so atleast one AMPDU is available
			 * Trigger WLAN Rx processing for FIFO 0/1, 2 packets.
			 */
#if defined(BCMHWA) && defined(HWA_RXFILL_BUILD)
			hwa_dev_t *dev = HWA_DEVP(FALSE);

			/** HWA has lazy interrupt enabled so, on receiving STS_FIFO interrupt
			 * trigger HWA rxfill and WLAN Rx processing of FIFO-0/1.
			 */
			WL_INFORM(("Initiate HWA Rx fill on STS_FIFO/M2M interrupt\n"));

			if (wlc_sts_xfer_phyrxs_rxpend(wlc, RX_FIFO)) {
				hwa_worklet_invoke(dev, HWA_COMMON_INTSTATUS_D11BDEST0_INT_MASK, 0);
			}
#else /* ! (BCMHWA && HWA_RXFILL_BUILD) */
			WL_INFORM(("Initiate WLAN Rx for FIFO-1 on STS_FIFO/M2M interrupt\n"));

			if (!(wlc_hw->dma_rx_intstatus & (RX_INTR_FIFO_0 | RX_INTR_FIFO_1)) &&
				wlc_sts_xfer_phyrxs_rxpend(wlc, RX_FIFO1)) {
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;
			}
#endif /* ! (BCMHWA && HWA_RXFILL_BUILD) */

			WL_INFORM(("Initiate WLAN Rx for FIFO-2 on STS_FIFO/M2M interrupt\n"));

			if (!(wlc_hw->dma_rx_intstatus & RX_INTR_FIFO_2) &&
				wlc_sts_xfer_phyrxs_rxpend(wlc, RX_FIFO2)) {
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;
			}

			if (!m2m_intr) {
				/* Clear Rx FIFO-3 (STS_FIFO) interrupt status */
				W_REG(wlc_hw->osh, &(D11Reggrp_intctrlregs(wlc_hw,
					STS_FIFO)->intstatus), DEF_RXINTMASK);
			}
		}
#endif /* STS_XFER_PHYRXS */

#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_M2M_INTR)
		if ((STS_RX_ENAB(wlc->pub) && wlc_get_fifo_interrupt(wlc, STS_FIFO)) ||
			(STS_RX_OFFLOAD_ENAB(wlc->pub) && m2m_intr)) {

#if defined(BCMHWA) && defined(HWA_RXFILL_BUILD)
			hwa_dev_t *dev = HWA_DEVP(FALSE);
			if (wlc_hw->rxpen_list[RXPEN_LIST_IDX0].rx_head) {
				wlc_hw->sts_fifo_intr = TRUE;
				WL_INFORM(("Got STS_FIFO intr => fifo-0 pkt %p\n",
					wlc_hw->rxpen_list[RXPEN_LIST_IDX0].rx_head));
				hwa_worklet_invoke(dev, HWA_COMMON_INTSTATUS_D11BDEST0_INT_MASK, 0);
			}
#else /* ! (HWA && HWA_RXFILL_BUILD) */
			if (!(wlc_hw->dma_rx_intstatus & (RX_INTR_FIFO_0 | RX_INTR_FIFO_1)) &&
				(wlc_hw->rxpen_list[RXPEN_LIST_IDX0].rx_head)) {
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;
				WL_INFORM(("Got STS_FIFO intr => fifo-0 pkt %p\n",
					wlc_hw->rxpen_list[RXPEN_LIST_IDX0].rx_head));
				wlc_hw->sts_fifo_intr = TRUE;
			}
#endif /* defined(BCMHWA) && defined(HWA_RXFILL_BUILD) */

			if (!(wlc_hw->dma_rx_intstatus & RX_INTR_FIFO_2) &&
				(wlc_hw->rxpen_list[RXPEN_LIST_IDX1].rx_head)) {
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;
				WL_INFORM(("Got STS_FIFO intr = > fifo-2 pkt %p\n",
					wlc_hw->rxpen_list[RXPEN_LIST_IDX1].rx_head));
				wlc_hw->sts_fifo_intr = TRUE;
			}

			if (!m2m_intr) {
				W_REG(wlc_hw->osh, &(D11Reggrp_intctrlregs(wlc_hw,
					STS_FIFO)->intstatus), DEF_RXINTMASK);
			}
		}
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_M2M_INTR */

	} else { /* ! BCMSPLITRX_ENAB */

		/* Check for Rx FIFO-0 interrupt */
		if (wlc_get_fifo_interrupt(wlc, RX_FIFO)) {

			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;

			/* Clear Rx FIFO-0 interrupt status */
			W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, RX_FIFO)->intstatus),
				DEF_RXINTMASK);
		}

#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_M2M_INTR)
		if ((STS_RX_ENAB(wlc->pub) && wlc_get_fifo_interrupt(wlc, STS_FIFO)) ||
			(STS_RX_OFFLOAD_ENAB(wlc->pub) && m2m_intr)) {

			/* check for fif0-0 intr */
			if (!(wlc_hw->dma_rx_intstatus & RX_INTR_FIFO_0) &&
				(wlc_hw->rxpen_list[RXPEN_LIST_IDX].rx_head)) {
				WL_INFORM(("Got STS_FIFO intr => fifo-0 pkt %p\n",
					wlc_hw->rxpen_list[RXPEN_LIST_IDX].rx_head));
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;
				wlc_hw->sts_fifo_intr = TRUE;
			}

			if (!m2m_intr) {
				W_REG(wlc_hw->osh, &(D11Reggrp_intctrlregs(wlc_hw,
					STS_FIFO)->intstatus), DEF_RXINTMASK);
			}
		}
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_M2M_INTR */

		/* Check for Rx FIFO-3 (STS_FIFO) interrupt */
#if defined(STS_XFER_PHYRXS)
		if ((STS_XFER_PHYRXS_FIFO_ENAB(wlc->pub) &&
				wlc_get_fifo_interrupt(wlc, STS_FIFO)) ||
			(STS_XFER_PHYRXS_M2M_ENAB(wlc->pub) && m2m_intr)) {

			WL_INFORM(("Initiate WLAN Rx process for FIFO-0 on STS_FIFO interrupt\n"));

			if (wlc_sts_xfer_phyrxs_rxpend(wlc, RX_FIFO)) {
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;
			}

			/* Clear Rx FIFO-3 (STS_FIFO) interrupt status */
			if (!m2m_intr) {
				W_REG(wlc_hw->osh, &(D11Reggrp_intctrlregs(wlc_hw,
					STS_FIFO)->intstatus), DEF_RXINTMASK);
			}
		}
#endif /* STS_XFER_PHYRXS */

	}

#else /* DMA_TX_FREE */
	int i;

	/* Clear rx and tx interrupts */
	/* When cut through DMA enabled, we use the other indirect indaqm DMA
	 * channel registers instead of legacy intctrlregs on TX direction.
	 * The descriptor in aqm_di contains the corresponding txdata DMA
	 * descriptors information which must be filled in tx.
	 */
	if (BCM_DMA_CT_ENAB(wlc_hw->wlc)) {
		for (i = 0; i < wlc_hw->nfifo_inuse; i++) {
			if (i == RX_FIFO) {
				W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, i)->intstatus),
					DEF_RXINTMASK);
				dma_set_indqsel(wlc_hw->aqm_di[i], FALSE);
				W_REG(osh, (&(D11Reggrp_indaqm(wlc_hw, 0)->indintstatus)), I_XI);

			} else if (wlc_hw->aqm_di[i] && dma_txactive(wlc_hw->aqm_di[i])) {
				dma_set_indqsel(wlc_hw->aqm_di[idx], FALSE);
				W_REG(osh, (&(D11Reggrp_indaqm(wlc_hw, 0)->indintstatus)), I_XI);
			}
		}
	} else {
		for (i = 0; i < NFIFO; i++) {

			if (i == RX_FIFO) {
				W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, i)->intstatus),
					DEF_RXINTMASK | I_XI);
			} else if (wlc_hw->di[i] && dma_txactive(wlc_hw->di[i])) {
				W_REG(osh, &(D11Reggrp_intctrlregs(wlc_hw, i)->intstatus), I_XI);
			}
		}
	}
#endif /* DMA_TX_FREE */
} /* wlc_intr_process_rxfifo_interrupts() */

/**
 * Detects if an interrupt was generated and if so, saves interrupt status in a variable for DPC
 * processing, acknowledges the interrupt(s) and clears the macintstatus register.
 * This routine should be called with interrupts off
 * Return:
 *   -1 if DEVICEREMOVED(wlc) evaluates to TRUE;
 *   0 if the interrupt is not for us, or we are in some special cases;
 *   device interrupt status bits otherwise.
 */
static INLINE uint32
wlc_intstatus(wlc_info_t *wlc, bool in_isr)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintstatus, mask;
	osl_t *osh;

	osh = wlc_hw->osh;

	/* macintstatus includes a DMA interrupt summary bit */
	macintstatus = GET_MACINTSTATUS(osh, wlc_hw);

	WL_TRACE(("wl%d: macintstatus: 0x%x\n", wlc_hw->unit, macintstatus));

	macintstatus |= wlc_hw->sw_macintstatus;
	wlc_hw->sw_macintstatus = 0;

	/* detect cardbus removed, in power down(suspend) and in reset */
	if (DEVICEREMOVED(wlc))
		return -1;

	/* DEVICEREMOVED succeeds even when the core is still resetting,
	 * handle that case here.
	 */
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return 0;
	}

	mask = (in_isr ? wlc_hw->macintmask : wlc_hw->defmacintmask);

	/* defer unsolicited interrupts */
	macintstatus &= mask;

	/* if not for us */
	if (macintstatus == 0)
		return 0;

	/* interrupts are already turned off for CFE build
	 * Caution: For CFE Turning off the interrupts again has some undesired
	 * consequences
	 */
#if !defined(_CFE_)

#if defined(WLC_OFFLOADS_M2M_INTR) || defined(STS_XFER_M2M_INTR) || defined(BCM_CSIMON) \
	|| defined(WL_AIR_IQ)
	ASSERT(in_isr || (wlc_hw->macintmask == 0));
	/* If M2M interrupts are enabled, All interrupts will be turned off in ISR handler */
#endif /* WLC_OFFLOADS_M2M_INTR || STS_XFER_M2M_INTR  || BCM_CSIMON || defined(WL_AIR_IQ) */

	/* turn off the interrupts */
	/*
	 * WAR for PR 39712
	 * clear the software/hardware intmask, before clearing macintstatus
	 */
	SET_MACINTMASK(osh, wlc_hw, 0);
	(void)GET_MACINTMASK(osh, wlc_hw);	/* sync readback */
	wlc_hw->macintmask = 0;

#endif /* !defined(_CFE_) */

	/* clear device interrupts */
	SET_MACINTSTATUS(osh, wlc_hw, macintstatus);

	/* MI_DMAINT is indication of non-zero intstatus */
	if (macintstatus & MI_DMAINT) {

#if defined(WLC_OFFLOADS_M2M_INTR) || defined(STS_XFER_PHYRXS_RECV_PROC)
		ASSERT(wlc_hw->defmacintmask & MI_DMAINT);
#endif /* WLC_OFFLOADS_M2M_INTR || STS_XFER_PHYRXS_RECV_PROC */

		wlc_intr_process_rxfifo_interrupts(wlc, FALSE);
	}

	return macintstatus;
} /* wlc_intstatus */

/**
 * Called by DPC when the DPC is rescheduled, updates wlc_hw->macintstatus and wlc_hw->intstatus.
 *
 * Prerequisites:
 * - Caller should have acquired a spinlock against isr concurrency, or guarantee that interrupts
 *   have been disabled.
 *
 * @return  TRUE if they are updated successfully. FALSE otherwise.
 */
bool
wlc_intrsupd(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintstatus;

	/* read and clear macintstatus and intstatus registers */
	macintstatus = wlc_intstatus(wlc, FALSE);

	/* device is removed */
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return FALSE;
	}

#if defined(MBSS)
	if (MBSS_ENAB(wlc->pub) &&
	    (macintstatus & (MI_DTIM_TBTT | MI_TBTT))) {
		/* Record latest TBTT/DTIM interrupt time for latency calc */
		wlc_mbss_record_time(wlc);
	}
#endif /* MBSS */

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		wlc_offload_intrsupd(wlc->offl);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		wlc_sts_xfer_intrsupd(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

	/* update interrupt status in software */
	wlc_hw->macintstatus |= macintstatus;

	return TRUE;
} /* wlc_intrsupd */

/**
 * First-level interrupt processing.
 * @param[out] wantworklet    Will be set to TRUE if further wlc_worklet() processing is required.
 * @return                TRUE if this was our interrupt. If so, interrupts are disabled (masked).
 *
 * Prerequisites:
 * - in NIC builds, the d11 core should be the currently selected core.
 * - caller should have acquired a spinlock against worklet concurrency.
 */
bool BCMFASTPATH
wlc_isr(wlc_info_t *wlc, bool *wantworklet)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintstatus;

	*wantworklet = FALSE;

	if (wlc_hw->macintmask == 0x00000000)
		return (FALSE);

	/* read and clear macintstatus and intstatus registers */
	macintstatus = wlc_intstatus(wlc, TRUE);
	if (macintstatus == 0xffffffff) {
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		WL_ERROR(("DEVICEREMOVED detected in the ISR code path.\n"));
		/* in rare cases, we may reach this condition as a race condition may occur */
		/* between disabling interrupts and clearing the SW macintmask */
		/* clear mac int status as there is no valid interrupt for us */
		wlc_hw->macintstatus = 0;
		/* assume this is our interrupt as before; note: want_worklet is FALSE */
		return (TRUE);
	}

	/* it is not for us */
	if (macintstatus == 0)
		return (FALSE);

	*wantworklet = TRUE;

#if defined(MBSS)
	/* BMAC_NOTE: This mechanism to bail out of sending beacons in
	 * wlc_ap.c:wlc_ap_sendbeacons() does not seem like a good idea across a bus with
	 * non-negligible reg-read time. The time values in the code have
	 * wlc_ap_sendbeacons() bail out if the delay between the isr time and it is >
	 * 4ms. But the check is done in wlc_ap_sendbeacons() with a reg read that might
	 * take on order of 0.3ms.  Should mbss only be supported with beacons in
	 * templates instead of beacons from host driver?
	 */
	if (MBSS_ENAB(wlc->pub) &&
	    (macintstatus & (MI_DTIM_TBTT | MI_TBTT))) {
		/* Record latest TBTT/DTIM interrupt time for latency calc */
		wlc_mbss_record_time(wlc);
	}
#endif /* MBSS */

	/* save interrupt status bits */
	ASSERT(wlc_hw->macintstatus == 0);

	wlc_hw->macintstatus = macintstatus;

#if defined(BCMDBG)
	wlc_update_isr_stats(wlc, macintstatus);
#endif
	return (TRUE);
} /* wlc_isr */

/**
 * Called at e.g. end of DPC handler, or at the end of a 'wl up'.
 *
 * Prerequisite: caller should have acquired a spinlock against isr concurrency, or guarantee that
 * interrupts have been disabled.
 */
void
wlc_intrson(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	ASSERT(wlc_hw->defmacintmask);
	wlc_hw->macintmask = wlc_hw->defmacintmask;
	SET_MACINTMASK(wlc_hw->osh, wlc_hw, wlc_hw->macintmask);

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_offload_intrson(wlc->offl);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_sts_xfer_intrson(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

#if defined(BCMHWA) && defined(HWA_TXSTAT_BUILD) && !defined(DONGLEBUILD)
	/* d11 and hwa are either both enabled or both disabled */
	if (HWA_ENAB(wlc->pub)) {
		hwa_intrson(WL_HWA_DEVP(wlc));
	}
#endif /* BCMHWA && HWA_TXSTAT_BUILD && !DONGLEBUILD */

#if !defined(DONGLEBUILD) && defined(BCM_CSIMON)
	if (CSIMON_ENAB(wlc->pub)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_csimon_intrson(wlc);
	}
#endif /* ! DONGLEBUILD && BCM_CSIMON */

#if !defined(DONGLEBUILD) && defined(WL_AIR_IQ)
	if (AIRIQ_ENAB(wlc->pub)) {
	/* D11 and M2MDMA interrupts are either both enabled or both disabled */
		wlc_airiq_m2m_intrson(wlc);
	}
#endif /* WL_AIR_IQ */
}

/* mask off interrupts */
#define SET_MACINTMASK_OFF(hw) { \
	SET_MACINTMASK(hw->osh, hw, 0); \
	(void)GET_MACINTMASK(hw->osh, hw);	/* sync readback */ \
	OSL_DELAY(1); /* ensure int line is no longer driven */ \
}

/** Called in wl_isr() to turn off all WLAN interrupts */
void
wlc_isr_intrsoff(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if (wlc_hw->macintmask) {
		SET_MACINTMASK_OFF(wlc_hw);
		wlc_hw->macintmask = 0;
	}

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_offload_intrsoff(wlc->offl);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_sts_xfer_intrsoff(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

#if defined(BCMHWA) && defined(HWA_TXSTAT_BUILD) && !defined(DONGLEBUILD)
	/* D11 and HWA interrupts are either both enabled or both disabled */
	if (HWA_ENAB(wlc->pub)) {
		hwa_intrsoff(WL_HWA_DEVP(wlc));
	}
#endif /* BCMHWA && HWA_TXSTAT_BUILD && !DONGLEBUILD */

#if !defined(DONGLEBUILD) && defined(BCM_CSIMON)
	/* D11 and M2MDMA interrupts are either both enabled or both disabled */
	if (CSIMON_ENAB(wlc->pub)) {
		wlc_csimon_intrsoff(wlc);
	}
#endif /* BCM_CSIMON */
#if !defined(DONGLEBUILD) && defined(WL_AIR_IQ)
	/* D11 and M2MDMA interrupts are either both enabled or both disabled */
	if (AIRIQ_ENAB(wlc->pub)) {
		wlc_airiq_m2m_intrsoff(wlc);
	}
#endif /* WL_AIR_IQ */
} /* wlc_isr_intrsoff() */

/**
 * Called during e.g. wlc_init(), wl_reset().
 *
 * Prerequisite: caller should have acquired a spinlock against isr concurrency.
 */
uint32
wlc_intrsoff(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint32 macintmask;

	if (!wlc_hw->clk)
		return 0;

	macintmask = wlc_hw->macintmask;	/* isr can still happen */
	SET_MACINTMASK_OFF(wlc_hw);
	wlc_hw->macintmask = 0;

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_offload_intrsoff(wlc->offl);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_sts_xfer_intrsoff(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

#if defined(BCMHWA) && defined(HWA_TXSTAT_BUILD) && !defined(DONGLEBUILD)
	/* d11 and hwa are either both enabled or both disabled */
	if (HWA_ENAB(wlc->pub)) {
		hwa_intrsoff(WL_HWA_DEVP(wlc));
	}
#endif /* BCMHWA && HWA_TXSTAT_BUILD && !DONGLEBUILD */
	/* return previous macintmask; resolve race between us and our isr */
	return (wlc_hw->macintstatus ? 0 : macintmask);
}

/** deassert interrupt, gets called by dongle firmware builds during wl_isr() */
void
wlc_intrs_deassert(wlc_info_t *wlc)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if (!wlc_hw->clk)
		return;

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_offload_intrs_deassert(wlc->offl);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		/* D11 and M2M interrupts are either both enabled or both disabled */
		wlc_sts_xfer_intrs_deassert(wlc);
	}
#endif /* STS_XFER_M2M_INTR */

	SET_MACINTMASK_OFF(wlc_hw);
}

/**
 * Numerous callers, including the PHY (wlapi_intrsrestore).
 *
 * Prerequisite: caller should have acquired a spinlock against isr concurrency, or guarantee that
 * interrupts have been disabled.
 */
void
wlc_intrsrestore(wlc_info_t *wlc, uint32 macintmask)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;

	if (!wlc_hw->clk)
		return;

	wlc_hw->macintmask = macintmask;
	SET_MACINTMASK(wlc_hw->osh, wlc_hw, wlc_hw->macintmask);

#if defined(WLC_OFFLOADS_M2M_INTR)
	if (D11REV_GE(wlc->pub->corerev, 130)) {
		wlc_offload_intrsrestore(wlc->offl, macintmask);
	}
#endif /* WLC_OFFLOADS_M2M_INTR */

#if defined(STS_XFER_M2M_INTR)
	if (STS_XFER_TXS_ENAB(wlc->pub) || STS_XFER_PHYRXS_M2M_ENAB(wlc->pub)) {
		wlc_sts_xfer_intrsrestore(wlc, macintmask);
	}
#endif /* STS_XFER_M2M_INTR */

}

/** Read per fifo interrupt and process the frame if any interrupt is present */
static uint8
wlc_process_per_fifo_intr(wlc_info_t *wlc, bool bounded, wlc_worklet_info_t *worklet)
{
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint8 fifo_status = wlc_hw->dma_rx_intstatus;
	/* reset sw interrup bit */
	wlc_hw->dma_rx_intstatus = 0;

	/* process fifo- 0 */
	if (fifo_status & RX_INTR_FIFO_0) {
		if (wlc_bmac_recv(wlc_hw, RX_FIFO, bounded, worklet)) {
			/* More frames to be processed */
			wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_0;
		}
	}

	/* process fifo- 1 */
	if (PKT_CLASSIFY_EN(RX_FIFO1) || RXFIFO_SPLIT()) {
		if (fifo_status & RX_INTR_FIFO_1) {
			if (wlc_bmac_recv(wlc_hw, RX_FIFO1, bounded, worklet)) {
				/* More frames to be processed */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_1;
			}
		}
	}
	/* Process fifo-2 */
	if (PKT_CLASSIFY_EN(RX_FIFO2)) {
		if (fifo_status & RX_INTR_FIFO_2) {

			/* Defer processing of Classify FIFO (RX FIFO2) if packet fetch is in
			* progress
			*/
			if (wlc_bmac_classify_fifo_suspended(wlc_hw) ||
				wlc_bmac_recv(wlc_hw, RX_FIFO2, bounded, worklet)) {
				/* More frames to be processed */
				wlc_hw->dma_rx_intstatus |= RX_INTR_FIFO_2;
			}
		}
	}

	return wlc_hw->dma_rx_intstatus;
} /* wlc_process_per_fifo_intr */

#if !defined(DMA_TX_FREE)
/* Return intstatus for rxfifo-x */
static uint32
wlc_get_fifo_interrupt(wlc_info_t *wlc, uint8 FIFO)
{
	return (R_REG(wlc->osh, &(D11Reggrp_intctrlregs(wlc, FIFO)->intstatus)) &
		DEF_RXINTMASK);
}
#endif /* !DMA_TX_FREE */
