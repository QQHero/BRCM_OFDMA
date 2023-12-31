/*
 * Common interface to the 802.11 AP Power Save state per scb
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
 * $Id: wlc_apps.c 801631 2021-07-29 01:54:47Z $
 */

/**
 * @file
 * @brief
 * Twiki: [WlDriverPowerSave]
 */

/* Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>

#ifndef AP
#error "AP must be defined to include this module"
#endif  /* AP */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <802.11.h>
#include <wpa.h>
#include <wlioctl.h>
#include <sbhndpio.h>
#include <sbhnddma.h>
#include <hnddma.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc_keymgmt.h>
#include <wlc_channel.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_ap.h>
#include <wlc_apps.h>
#include <wlc_scb.h>
#include <wlc_phy_hal.h>
#include <bcmwpa.h>
#include <wlc_bmac.h>
#ifdef PROP_TXSTATUS
#include <wlc_wlfc.h>
#endif
#ifdef WLTDLS
#include <wlc_tdls.h>
#endif
#include <wl_export.h>
#ifdef WLAMPDU
#include <wlc_ampdu.h>
#endif /* WLAMPDU */
#include <wlc_pcb.h>
#ifdef WLTOEHW
#include <wlc_tso.h>
#endif /* WLTOEHW */
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#ifdef WLWNM
#include <wlc_wnm.h>
#endif
#include "wlc_txc.h"
#include <wlc_tx.h>
#include <wlc_mbss.h>
#include <wlc_txmod.h>
#include <wlc_pspretend.h>
#include <wlc_qoscfg.h>
#ifdef WL_BEAMFORMING
#include <wlc_txbf.h>
#endif /* WL_BEAMFORMING */
#ifdef BCMFRWDPOOLREORG
#include <hnd_poolreorg.h>
#endif /* BCMFRWDPOOLREORG */
#ifdef WLCFP
#include <wlc_cfp.h>
#endif
#include <wlc_he.h>

#if defined(HNDPQP)
#ifndef WL_USE_SUPR_PSQ
#error "WL_USE_SUPR_PSQ needs to be defined with HNDPQP"
#endif

#define APPS_PKTQ_MLEN(wlc, scb)    pktq_pqp_mlen(wlc, scb)

#include <hnd_pqp.h>
static int wlc_apps_pqp_pgi_cb(pqp_cb_t* cb);
static int wlc_apps_pqp_pgo_cb(pqp_cb_t* cb);
#else /* HNDPQP */
#define APPS_PKTQ_MLEN(wlc, scb)    pktq_mlen(wlc, scb)
#endif /* !HNDPQP */

#ifdef WLTAF
#include <wlc_taf.h>
#endif
#ifdef PKTQ_LOG
#include <wlc_perf_utils.h>
#endif

#include <wlc_event_utils.h>
#include <wlc_pmq.h>
#include <wlc_twt.h>
#ifdef WL_PS_STATS
#include <wlc_perf_utils.h>
#include <wlc_dump.h>
#endif /* WL_PS_STATS */
#ifdef WLNAR
#include <wlc_nar.h>
#endif

#ifdef PKTQ_LOG
#include <wlc_perf_utils.h>
#endif
#ifdef BCMPCIEDEV
#include <wlc_sqs.h>
#include <wlc_amsdu.h>
#endif /* BCMPCIEDEV */

// Forward declarations
static void wlc_apps_ps_timedout(wlc_info_t *wlc, struct scb *scb);
static bool wlc_apps_ps_send(wlc_info_t *wlc, struct scb *scb, uint prec_bmp, uint32 flags);
static bool wlc_apps_ps_enq_resp(wlc_info_t *wlc, struct scb *scb, void *pkt, int prec);
static void wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec);
static int wlc_apps_apsd_delv_count(wlc_info_t *wlc, struct scb *scb);
static int wlc_apps_apsd_ndelv_count(wlc_info_t *wlc, struct scb *scb);
static void wlc_apps_apsd_send(wlc_info_t *wlc, struct scb *scb);
static void wlc_apps_txq_to_psq(wlc_info_t *wlc, struct scb *scb);
static void wlc_apps_move_cpktq_to_psq(wlc_info_t *wlc, struct cpktq *cpktq, struct scb* scb);

#ifdef PROP_TXSTATUS
static void wlc_apps_move_to_psq(wlc_info_t *wlc, struct pktq *txq, struct scb* scb);
#ifdef WLNAR
static void wlc_apps_nar_txq_to_psq(wlc_info_t *wlc, struct scb *scb);
#endif /* WLNAR */
#ifdef WLAMPDU
static void wlc_apps_ampdu_txq_to_psq(wlc_info_t *wlc, struct scb *scb);
#endif /* WLAMPDU */
#endif /* PROP_TXSTATUS */

static void wlc_apps_apsd_eosp_send(wlc_info_t *wlc, struct scb *scb);
static int wlc_apps_bss_init(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_apps_bss_deinit(void *ctx, wlc_bsscfg_t *cfg);
static void wlc_apps_bss_wd_ps_check(void *handle);
#if defined(BCMDBG)
static void wlc_apps_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b);
#else
#define wlc_apps_bss_dump NULL
#endif
static void wlc_apps_omi_waitpmq_war(wlc_info_t *wlc);

#ifdef WLTWT
static void wlc_apps_twt_enter_ready(wlc_info_t *wlc, scb_t *scb);
#else
#define wlc_apps_twt_enter_ready(a, b) do {} while (0)
#endif /* WLTWT */

#if defined(MBSS)
static void wlc_apps_bss_ps_off_start(wlc_info_t *wlc, struct scb *bcmc_scb);
#else
#define wlc_apps_bss_ps_off_start(wlc, bcmc_scb)
#endif /* MBSS */

/* IE mgmt */
static uint wlc_apps_calc_tim_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_apps_write_tim_ie(void *ctx, wlc_iem_build_data_t *data);
static void wlc_apps_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data);
static void wlc_apps_bss_updn(void *ctx, bsscfg_up_down_event_data_t *evt);
static int wlc_apps_send_psp_response_cb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, void *data);
#ifdef WL_PS_STATS
/* ps stats */
static int wlc_apps_stats_dump(void *ctx, struct bcmstrbuf *b);
static int wlc_apps_stats_dump_clr(void *ctx);
void wlc_apps_upd_pstime(struct scb *scb);
#endif /* WL_PS_STATS */
static uint16 wlc_apps_scb_pktq_tot(wlc_info_t *wlc, struct scb *scb);

#ifdef HNDPQP
static uint32 wlc_apps_scb_pqp_ps_flush_prec(wlc_info_t *wlc, struct scb *scb,
    struct pktq *pq, int prec);
static void wlc_apps_scb_pqp_map_pkts(wlc_info_t *wlc, struct scb *scb,
    map_pkts_cb_fn cb, void *ctx);
static void wlc_apps_scb_pqp_pktq_filter(wlc_info_t *wlc, struct scb *scb, void *ctx);

/* Asynchronous PQP PGI flags for PS transition */
#define PQP_PGI_PS_OFF        0x0001
#define PQP_PGI_PS_TWT_OFF    0x0002
#define PQP_PGI_PS_BCMC_OFF    0x0004
#define PQP_PGI_PS_SEND        0x0008

#define PQP_PGI_PS_TRANS_OFF    (PQP_PGI_PS_OFF | PQP_PGI_PS_TWT_OFF | PQP_PGI_PS_BCMC_OFF)

#define PQP_PGI_PS_TRANS(psinfo)        (psinfo)->pqp_ps_trans
#define PQP_PGI_PS_PRECMAP(psinfo)        (psinfo)->pqp_ps_precbmp
#define PQP_PGI_PS_EXTRAFLAG(psinfo)        (psinfo)->pqp_ps_extraflag

#define PQP_PGI_PS_SEND_ISSET(psinfo)        ((psinfo)->pqp_ps_trans & PQP_PGI_PS_SEND)
#define PQP_PGI_PS_TRANS_OFF_ISSET(psinfo)    ((psinfo)->pqp_ps_trans & PQP_PGI_PS_TRANS_OFF)

#define PQP_PGI_PS_TRANS_PEND_SET(psinfo, trans, precbmp, flag) \
({ \
    ASSERT((psinfo)->pqp_ps_trans == 0); \
    (psinfo)->pqp_ps_trans = (trans); \
    (psinfo)->pqp_ps_precbmp = (precbmp); \
    (psinfo)->pqp_ps_extraflag = (flag); \
})
#define PQP_PGI_PS_TRANS_PEND_CLR(psinfo) \
({ \
    (psinfo)->pqp_ps_trans = 0; \
    (psinfo)->pqp_ps_precbmp = 0; \
    (psinfo)->pqp_ps_extraflag = 0; \
    (psinfo)->ps_trans_status &= ~SCB_PS_TRANS_OFF_IN_PROG; \
})
#endif /* HNDPQP */

#ifdef PROP_TXSTATUS
/* Maximum suppressed broadcast packets handled */
#define BCMC_MAX 4
#endif

/* Flags for psp_flags */
#define PS_MORE_DATA        0x01
#define PS_PSP_REQ_PEND        0x02
#define PS_PSP_ONRESP        0x04    /* a pspoll req is under handling (SWWLAN-42801) */

/* PS transition status flags of apps_scb_psinfo */
#define SCB_PS_TRANS_OFF_PEND        0x01    /* Pend PS off until txfifo draining is done */
#define SCB_PS_TRANS_OFF_BLOCKED    0x02    /* Block attempts to switch PS off */
#define SCB_PS_TRANS_OFF_IN_PROG    0x04    /* PS off transition is already in progress */

/* PS transition status flags of apps_bss_psinfo */
#define BSS_PS_ON_BY_TWT        0x01    /* Force PS of bcmc scb by TWT */

#define PSQ_PKTQ_LEN_DEFAULT    512        /* Max 512 packets */

/* power-save mode definitions */
#ifndef PSQ_PKTS_LO
#define PSQ_PKTS_LO        5    /* max # PS pkts scb can enq */
#endif
#ifndef PSQ_PKTS_HI
#define    PSQ_PKTS_HI        500
#endif /* PSQ_PKTS_HI */

/* flags of apps_scb_psinfo */
#define SCB_PS_FIRST_SUPPR_HANDLED    0x01
#define SCB_PS_FLUSH_IN_PROGR        0x02

/* Threshold of triggers before resetting usp */
#define APSD_TRIG_THR        5

struct apps_scb_psinfo {
    int        suppressed_pkts;    /* Count of suppressed pkts in the
                         * scb's "psq". Used as a check to
                         * mark psq as requiring
                         * normalization (or not).
                         */
    struct pktq    psq;        /** PS defer (multi priority) packet queue */
#ifdef WL_USE_SUPR_PSQ
    struct spktq supr_psq[WLC_PREC_COUNT]; /* Use to store PS Supressed pkts */
#endif
    bool        psp_pending;    /* whether uncompleted PS-POLL response is pending */
    uint8        psp_flags;    /* various ps mode bit flags defined below */
    uint8        flags;        /* a.o. have we handled first supr'd frames ? */
    bool        apsd_usp;    /* APSD Unscheduled Service Period in progress */
    int        apsd_cnt;    /* Frames left in this service period */
    int        apsd_trig_cnt;    /* apsd trigger received, but not processed */
    mbool        tx_block;    /* tx has been blocked */
    bool        ext_qos_null;    /* Send extra QoS NULL frame to indicate EOSP
                     * if last frame sent in SP was MMPDU
                     */
    uint8        ps_trans_status; /* PS transition status flag */
/* PROP_TXSTATUS starts */
    bool        apsd_hpkt_timer_on;
    bool        apsd_tx_pending;
    struct wl_timer    *apsd_hpkt_timer;
    struct scb    *scb;
    wlc_info_t    *wlc;
    int        apsd_v2r_in_transit;
/* PROP_TXSTATUS ends */
    uint16        tbtt;        /**< count of tbtt intervals since last ageing event */
    uint16        listen;        /**< minimum # bcn's to buffer PS traffic */
    uint32        ps_discard;    /* cnt of PS pkts which were dropped */
    uint32        supr_drop;    /* cnt of Supr pkts dropped */
    uint32        ps_queued;    /* cnt of PS pkts which were queued */
    uint32        last_in_ps_tsf;
    uint32        last_in_pvb_tsf;
    bool        in_pvb;        /* This STA already set in partial virtual bitmap */
    bool        change_scb_state_to_auth; /* After disassoc, scb goes into reset state
                           * and switch back to auth state, in case scb
                           * is in power save and AP holding disassoc
                           * frame for this, let the disassoc frame
                           * go out with NULL data packet from STA and
                           * wlc_apps module to change the state to auth
                           * state.
                           */
    uint8        ps_requester;    /* source of ps requests */
    bool        twt_active;    /**< TWT is active on SCB (twt_enter was called) */
    bool        twt_wait4drain_enter;
    bool        twt_wait4drain_exit;
    bool        twt_wait4drain_norm;    /* Drain txfifo for supr_q normalization */
    bool        pspoll_pkt_waiting;
#ifdef HNDPQP
    uint16        pqp_ps_trans; /* Asynchronous PQP PGI flags for PS transition */
    uint16        pqp_ps_precbmp; /* Asynchronous PQP PGI prec bitmap */
    uint32        pqp_ps_extraflag; /* additional flags on SDU */
#endif /* HNDPQP */
};

#define SCB_PSPOLL_PKT_WAITING(psinfo)        (psinfo)->pspoll_pkt_waiting

typedef struct apps_scb_psinfo apps_scb_psinfo_t;

typedef struct apps_bss_info
{
    uint8        pvb[251];        /* full partial virtual bitmap */
    uint16        aid_lo;            /* lowest aid with traffic pending */
    uint16        aid_hi;            /* highest aid with traffic pending */

    uint32        ps_nodes;        /* num of STAs in PS-mode */
    uint8        ps_trans_status;    /* PS transition status flag */
    uint32        bcmc_pkts_seen;        /* Packets thru BC/MC SCB queue WLCNT */
    uint32        bcmc_discards;        /* Packets discarded due to full queue WLCNT */
} apps_bss_info_t;

struct apps_wlc_psinfo
{
    int        cfgh;            /* bsscfg cubby handle */
    osl_t        *osh;            /* pointer to os handle */
    int        scb_handle;        /* scb cubby handle to retrieve data from scb */
    uint32        ps_nodes_all;        /* Count of nodes in PS across all BBSes */
    uint        ps_deferred;        /* cnt of all PS pkts buffered on unicast scbs */
    uint32        ps_discard;        /* cnt of all PS pkts which were dropped */
    uint32        supr_drop;        /* cnt of Supr pkts dropped */
    uint32        ps_aged;        /* cnt of all aged PS pkts */
    uint        psq_pkts_lo;        /* # ps pkts are always enq'able on scb */
    uint        psq_pkts_hi;        /* max # of ps pkts enq'able on a single scb */
};

/* AC bitmap to precedence bitmap mapping (constructed in wlc_attach) */
static uint wlc_acbitmap2precbitmap[16] = { 0 };

/* Map AC bitmap to precedence bitmap */
#define WLC_ACBITMAP_TO_PRECBITMAP(ab)  wlc_apps_ac2precbmp_info()[(ab) & 0xf]

/* AC bitmap to tid bitmap mapping (constructed in wlc_attach) */
static uint8 wlc_acbitmap2tidbitmap[16] = { 0 };

/* Map AC bitmap to tid bitmap */
#define WLC_ACBITMAP_TO_TIDBITMAP(ab)  wlc_acbitmap2tidbitmap[(ab) & 0xf]

#define SCB_PSINFO_LOC(psinfo, scb) ((apps_scb_psinfo_t **)SCB_CUBBY(scb, (psinfo)->scb_handle))
#define SCB_PSINFO(psinfo, scb) (*SCB_PSINFO_LOC(psinfo, scb))

/* apps info accessor */
#define APPS_BSSCFG_CUBBY_LOC(psinfo, cfg) ((apps_bss_info_t **)BSSCFG_CUBBY((cfg), (psinfo)->cfgh))
#define APPS_BSSCFG_CUBBY(psinfo, cfg) (*(APPS_BSSCFG_CUBBY_LOC(psinfo, cfg)))

#define BSS_PS_NODES(psinfo, bsscfg) ((APPS_BSSCFG_CUBBY(psinfo, bsscfg))->ps_nodes)

static int wlc_apps_scb_psinfo_init(void *context, struct scb *scb);
static void wlc_apps_scb_psinfo_deinit(void *context, struct scb *scb);
static uint wlc_apps_scb_psinfo_secsz(void *context, struct scb *scb);
static uint wlc_apps_txpktcnt(void *context);
static void wlc_apps_ps_flush_tid(void *context, struct scb *scb, uint8 tid);
#ifdef BCMPCIEDEV
static INLINE void _wlc_apps_scb_psq_resp(wlc_info_t *wlc, struct scb *scb,
    struct apps_scb_psinfo *scb_psinfo);
static void wlc_apps_scb_apsd_dec_v2r_in_transit(wlc_info_t *wlc, struct scb *scb,
    void *pkt, int prec);
#endif /* BCMPCIEDEV */
static void wlc_apps_apsd_hpkt_tmout(void *arg);

static void wlc_apps_apsd_complete(wlc_info_t *wlc, void *pkt, uint txs);
static void wlc_apps_psp_resp_complete(wlc_info_t *wlc, void *pkt, uint txs);
#ifdef WL_USE_SUPR_PSQ
/* Version operation on ps suppress queue */
static void wlc_apps_dequeue_scb_supr_psq(wlc_info_t *wlc, struct scb *scb);
#endif /* WL_USE_SUPR_PSQ */

#define  DOT11_MNG_TIM_NON_PVB_LEN  (DOT11_MNG_TIM_FIXED_LEN + TLV_HDR_LEN)

#ifdef BCMPCIEDEV
static int wlc_apps_sqs_vpkt_count(wlc_info_t *wlc, struct scb *scb);
static int wlc_apps_apsd_ndelv_vpkt_count(wlc_info_t *wlc, struct scb *scb);
static int wlc_apps_apsd_delv_vpkt_count(wlc_info_t *wlc, struct scb *scb);
/* Packet Queue watermarks used for fetching packets from host */
#define WLC_APSD_DELV_CNT_HIGH_WATERMARK    4
#define WLC_APSD_DELV_CNT_LOW_WATERMARK        2
#endif /* BCMPCIEDEV */

/* Accessor Function */
static uint *BCMRAMFN(wlc_apps_ac2precbmp_info)(void)
{
    return wlc_acbitmap2precbitmap;
}

static txmod_fns_t BCMATTACHDATA(apps_txmod_fns) = {
    wlc_apps_ps_enq,
    wlc_apps_txpktcnt,
    wlc_apps_ps_flush_tid,
    NULL,
    NULL
};

#if defined(BCMDBG)
static void wlc_apps_scb_psinfo_dump(void *context, struct scb *scb, struct bcmstrbuf *b);
#else
/* Use NULL to pass as reference on init */
#define wlc_apps_scb_psinfo_dump NULL
#endif

#if defined(BCMDBG)
/* Limited dump routine for APPS SCB info */
static void
wlc_apps_scb_psinfo_dump(void *context, struct scb *scb, struct bcmstrbuf *b)
{
    wlc_info_t *wlc = (wlc_info_t *)context;
    struct apps_scb_psinfo *scb_psinfo;
    struct apps_bss_info *bss_info;
    wlc_bsscfg_t *bsscfg;
    struct pktq *pktq;

    if (scb == NULL)
        return;

    bsscfg = scb->bsscfg;
    if (bsscfg == NULL)
        return;

    if (!BSSCFG_AP(bsscfg))
        return;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL)
        return;

    bcm_bprintf(b, "     APPS psinfo on SCB %p is %p; scb-PS is %s; Listen is %d\n",
                OSL_OBFUSCATE_BUF(scb), OSL_OBFUSCATE_BUF(scb_psinfo),
            SCB_PS(scb) ? "on" : "off", scb_psinfo->listen);

    bcm_bprintf(b, "     tx_block %d ext_qos_null %d psp_pending %d discards %d queued %d\n",
        scb_psinfo->tx_block, scb_psinfo->ext_qos_null, scb_psinfo->psp_pending,
        scb_psinfo->ps_discard, scb_psinfo->ps_queued);

    if (SCB_ISMULTI(scb)) {
        bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
        bcm_bprintf(b, "       SCB is multi. node count %d\n",
                    BSS_PS_NODES(wlc->psinfo, bsscfg));
        bcm_bprintf(b, "       BC/MC Seen %d Disc %d\n",
                    WLCNTVAL(bss_info->bcmc_pkts_seen), WLCNTVAL(bss_info->bcmc_discards));

    }

    pktq = &scb_psinfo->psq;
    if (pktq == NULL) {
        bcm_bprintf(b, "       Packet queue is NULL\n");
        return;
    }

    bcm_bprintf(b, "       Pkt Q %p Que len %d Max %d Avail %d\n", OSL_OBFUSCATE_BUF(pktq),
        pktq_n_pkts_tot(pktq), pktq_max(pktq), pktq_avail(pktq));
}
#endif /* BCMDBG */

int
BCMATTACHFN(wlc_apps_attach)(wlc_info_t *wlc)
{
    scb_cubby_params_t cubby_params;
    apps_wlc_psinfo_t *wlc_psinfo;
    bsscfg_cubby_params_t bsscfg_cubby_params;
    int i;

    if (!(wlc_psinfo = MALLOCZ(wlc->osh, sizeof(apps_wlc_psinfo_t)))) {
        WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
                  wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->osh)));
        wlc->psinfo = NULL;
        return -1;
    }

    /* reserve cubby space in the bsscfg container for per-bsscfg private data */
    bzero(&bsscfg_cubby_params, sizeof(bsscfg_cubby_params));
    bsscfg_cubby_params.context = wlc_psinfo;
    bsscfg_cubby_params.fn_deinit = wlc_apps_bss_deinit;
    bsscfg_cubby_params.fn_init = wlc_apps_bss_init;
    bsscfg_cubby_params.fn_dump = wlc_apps_bss_dump;

    if ((wlc_psinfo->cfgh = wlc_bsscfg_cubby_reserve_ext(wlc, sizeof(apps_bss_info_t *),
                                                          &bsscfg_cubby_params)) < 0) {
        WL_ERROR(("wl%d: %s: wlc_bsscfg_cubby_reserve() failed\n",
                  wlc->pub->unit, __FUNCTION__));
        return -1;
    }

    /* bsscfg up/down callback */
    if (wlc_bsscfg_updown_register(wlc, wlc_apps_bss_updn, wlc_psinfo) != BCME_OK) {
        WL_ERROR(("wl%d: %s: wlc_bsscfg_updown_register() failed\n",
            wlc->pub->unit, __FUNCTION__));
        return -1;
    }

    /* Set the psq watermarks */
    wlc_psinfo->psq_pkts_lo = PSQ_PKTS_LO;
    wlc_psinfo->psq_pkts_hi = PSQ_PKTS_HI;

    /* calculate the total ps pkts required */
    wlc->pub->psq_pkts_total = wlc_psinfo->psq_pkts_hi +
            (wlc->pub->tunables->maxscb * wlc_psinfo->psq_pkts_lo);

    /* reserve cubby in the scb container for per-scb private data */
    bzero(&cubby_params, sizeof(cubby_params));

    cubby_params.context = wlc;
    cubby_params.fn_init = wlc_apps_scb_psinfo_init;
    cubby_params.fn_deinit = wlc_apps_scb_psinfo_deinit;
    cubby_params.fn_secsz = wlc_apps_scb_psinfo_secsz;
    cubby_params.fn_dump = wlc_apps_scb_psinfo_dump;

    wlc_psinfo->scb_handle = wlc_scb_cubby_reserve_ext(wlc,
                                                       sizeof(apps_scb_psinfo_t *),
                                                       &cubby_params);

    if (wlc_psinfo->scb_handle < 0) {
        WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve() failed\n",
                  wlc->pub->unit, __FUNCTION__));
        wlc_apps_detach(wlc);
        return -2;
    }

    /* construct mapping from AC bitmap to precedence bitmap */
    for (i = 0; i < 16; i++) {
        wlc_acbitmap2precbitmap[i] = 0;
        if (AC_BITMAP_TST(i, AC_BE))
            wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_BE;
        if (AC_BITMAP_TST(i, AC_BK))
            wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_BK;
        if (AC_BITMAP_TST(i, AC_VI))
            wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_VI;
        if (AC_BITMAP_TST(i, AC_VO))
            wlc_acbitmap2precbitmap[i] |= WLC_PREC_BMP_AC_VO;
    }

    /* construct mapping from AC bitmap to tid bitmap */
    for (i = 0; i < 16; i++) {
        wlc_acbitmap2tidbitmap[i] = 0;
        if (AC_BITMAP_TST(i, AC_BE))
            wlc_acbitmap2tidbitmap[i] |= TID_BMP_AC_BE;
        if (AC_BITMAP_TST(i, AC_BK))
            wlc_acbitmap2tidbitmap[i] |= TID_BMP_AC_BK;
        if (AC_BITMAP_TST(i, AC_VI))
            wlc_acbitmap2tidbitmap[i] |= TID_BMP_AC_VI;
        if (AC_BITMAP_TST(i, AC_VO))
            wlc_acbitmap2tidbitmap[i] |= TID_BMP_AC_VO;
    }

    /* register module */
    if (wlc_module_register(wlc->pub, NULL, "apps", wlc, NULL, wlc_apps_bss_wd_ps_check, NULL,
        NULL)) {
        WL_ERROR(("wl%d: %s wlc_module_register() failed\n",
                  wlc->pub->unit, __FUNCTION__));
        return -4;
    }

    /* register packet class callback */
    if (wlc_pcb_fn_set(wlc->pcb, 1, WLF2_PCB2_APSD, wlc_apps_apsd_complete) != BCME_OK) {
        WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
        return -5;
    }
    if (wlc_pcb_fn_set(wlc->pcb, 1, WLF2_PCB2_PSP_RSP, wlc_apps_psp_resp_complete) != BCME_OK) {
        WL_ERROR(("wl%d: %s wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
        return -6;
    }

    /* register IE mgmt callbacks */
    /* bcn */
    if (wlc_iem_add_build_fn(wlc->iemi, FC_BEACON, DOT11_MNG_TIM_ID,
            wlc_apps_calc_tim_ie_len, wlc_apps_write_tim_ie, wlc) != BCME_OK) {
        WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, tim in bcn\n",
                  wlc->pub->unit, __FUNCTION__));
        return -7;
    }

#if defined(PSPRETEND) && !defined(PSPRETEND_DISABLED)
    if ((wlc->pps_info = wlc_pspretend_attach(wlc)) == NULL) {
        WL_ERROR(("wl%d: %s: wlc_pspretend_attach failed\n",
                  wlc->pub->unit, __FUNCTION__));
        return -8;
    }
    wlc->pub->_pspretend = TRUE;
#endif /* PSPRETEND  && !PSPRETEND_DISABLED */
    wlc_txmod_fn_register(wlc->txmodi, TXMOD_APPS, wlc, apps_txmod_fns);

    /* Add client callback to the scb state notification list */
    if (wlc_scb_state_upd_register(wlc, wlc_apps_scb_state_upd_cb, wlc) != BCME_OK) {
        WL_ERROR(("wl%d: %s: unable to register callback %p\n",
                  wlc->pub->unit, __FUNCTION__,
            OSL_OBFUSCATE_BUF(wlc_apps_scb_state_upd_cb)));
        return -9;
    }

#ifdef WL_PS_STATS
    /* Register dump ps stats */
    wlc_dump_add_fns(wlc->pub, "ps_stats", wlc_apps_stats_dump, wlc_apps_stats_dump_clr, wlc);
#endif /* WL_PS_STATS */

    wlc_psinfo->osh = wlc->osh;
    wlc->psinfo = wlc_psinfo;

#if defined(HNDPQP)
    /* Configure PQP callbacks */
    pqp_config(wlc->osh, hwa_dev, wlc_apps_pqp_pgo_cb,
        wlc_apps_pqp_pgi_cb);
#endif

    return 0;
}

void
BCMATTACHFN(wlc_apps_detach)(wlc_info_t *wlc)
{
    apps_wlc_psinfo_t *wlc_psinfo;

    ASSERT(wlc);

#ifdef PSPRETEND
    wlc_pspretend_detach(wlc->pps_info);
#endif /* PSPRETEND */

    wlc_psinfo = wlc->psinfo;

    if (!wlc_psinfo)
        return;

    /* All PS packets shall have been flushed */
    ASSERT(wlc_psinfo->ps_deferred == 0);

    wlc_scb_state_upd_unregister(wlc, wlc_apps_scb_state_upd_cb, wlc);
    (void)wlc_bsscfg_updown_unregister(wlc, wlc_apps_bss_updn, wlc_psinfo);

    wlc_module_unregister(wlc->pub, "apps", wlc);

    MFREE(wlc->osh, wlc_psinfo, sizeof(apps_wlc_psinfo_t));
    wlc->psinfo = NULL;
}

#if defined(BCMDBG)
/* Verify that all ps packets have been flushed.
 * This is called after bsscfg down notify callbacks were processed.
 * So wlc_apps_bss_updn has been called for each bsscfg and all packets should have been flushed.
 */
void
wlc_apps_wlc_down(wlc_info_t *wlc)
{
    apps_wlc_psinfo_t *wlc_psinfo;

    ASSERT(wlc);

    wlc_psinfo = wlc->psinfo;

    if (!wlc_psinfo)
        return;

    /* All PS packets have been flushed */
    ASSERT(wlc_psinfo->ps_deferred == 0);
}
#endif /* BCMDBG */

/* bsscfg cubby */
static int
wlc_apps_bss_init(void *ctx, wlc_bsscfg_t *cfg)
{
    apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
    wlc_info_t *wlc = cfg->wlc;
    apps_bss_info_t **papps_bss = APPS_BSSCFG_CUBBY_LOC(wlc_psinfo, cfg);
    apps_bss_info_t *apps_bss = NULL;
    UNUSED_PARAMETER(wlc);

    /* Allocate only for AP || TDLS bsscfg */
    if (BSSCFG_HAS_PSINFO(cfg)) {
        if (!(apps_bss = (apps_bss_info_t *)MALLOCZ(wlc_psinfo->osh,
            sizeof(apps_bss_info_t)))) {
            WL_ERROR(("wl%d: %s out of memory, malloced %d bytes\n",
                wlc->pub->unit, __FUNCTION__, MALLOCED(wlc_psinfo->osh)));
            return BCME_NOMEM;
        }
    }
    *papps_bss = apps_bss;

    return BCME_OK;
}

static void
wlc_apps_bss_deinit(void *ctx, wlc_bsscfg_t *cfg)
{
    apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
    apps_bss_info_t **papps_bss = APPS_BSSCFG_CUBBY_LOC(wlc_psinfo, cfg);
    apps_bss_info_t *apps_bss = *papps_bss;

    if (apps_bss != NULL) {
        MFREE(wlc_psinfo->osh, apps_bss, sizeof(apps_bss_info_t));
        *papps_bss = NULL;
    }

    return;
}

/* Return the count of all the packets being held by APPS TxModule */
static uint
wlc_apps_txpktcnt(void *context)
{
    wlc_info_t *wlc = (wlc_info_t *)context;
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

    return (wlc_psinfo->ps_deferred);
}

/* Return the count of all the packets being held in scb psq */
uint
wlc_apps_scb_txpktcnt(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return 0;

    return (pktq_n_pkts_tot(&scb_psinfo->psq));
}

static int
wlc_apps_scb_psinfo_init(void *context, struct scb *scb)
{
    wlc_info_t *wlc = (wlc_info_t *)context;
    apps_scb_psinfo_t **cubby_info;
    wlc_bsscfg_t *bsscfg;

    cubby_info = SCB_PSINFO_LOC(wlc->psinfo, scb);
    ASSERT(*cubby_info == NULL);

    bsscfg = SCB_BSSCFG(scb);
    ASSERT(bsscfg != NULL);

    if (BSSCFG_HAS_PSINFO(bsscfg)) {
        *cubby_info = wlc_scb_sec_cubby_alloc(wlc, scb, sizeof(apps_scb_psinfo_t));
        if (*cubby_info == NULL)
            return BCME_NOMEM;

        if (!((*cubby_info)->apsd_hpkt_timer = wl_init_timer(wlc->wl,
            wlc_apps_apsd_hpkt_tmout, *cubby_info, "appsapsdhkpt"))) {
            WL_ERROR(("wl: apsd_hpkt_timer failed\n"));
            return 1;
        }
        (*cubby_info)->wlc = wlc;
        (*cubby_info)->scb = scb;
        /* PS state init */
#ifdef WL_USE_SUPR_PSQ
        /* Init the max psq size, but limit the psq length at enqueue */
        pktq_init(&(*cubby_info)->psq, WLC_PREC_COUNT, PSQ_PKTQ_LEN_DEFAULT);
        {    /* Init supress ps spktq */
            int prec;
            for (prec = 0; prec < WLC_PREC_COUNT; prec++)
                spktq_init(&(*cubby_info)->supr_psq[prec], PSQ_PKTQ_LEN_DEFAULT);
        }
#else
        pktq_init(&(*cubby_info)->psq, WLC_PREC_COUNT, PSQ_PKTQ_LEN_DEFAULT);
#endif
    }
    return 0;
}

static void
wlc_apps_scb_psinfo_deinit(void *context, struct scb *remove)
{
    wlc_info_t *wlc = (wlc_info_t *)context;
    apps_scb_psinfo_t **cubby_info = SCB_PSINFO_LOC(wlc->psinfo, remove);
    struct pktq*    psq;

    if (*cubby_info) {
        psq = &(*cubby_info)->psq;

        wl_del_timer(wlc->wl, (*cubby_info)->apsd_hpkt_timer);
        (*cubby_info)->apsd_hpkt_timer_on = FALSE;
        wl_free_timer(wlc->wl, (*cubby_info)->apsd_hpkt_timer);

        if (AP_ENAB(wlc->pub) || (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(remove)) &&
                SCB_PS(remove))) {
#ifdef WL_USE_SUPR_PSQ
            if (SCB_PSINFO(wlc->psinfo, remove)->suppressed_pkts > 0) {
                wlc_apps_dequeue_scb_supr_psq(wlc, remove);
            }
#endif /* WL_USE_SUPR_PSQ */
            if (SCB_PS(remove) && !SCB_ISMULTI(remove)) {
                wlc_apps_scb_ps_off(wlc, remove, TRUE);
            } else if (!pktq_empty(psq))
                wlc_apps_ps_flush(wlc, remove);
        }

#ifdef WL_USE_SUPR_PSQ
        {    /* Deinit supress ps spktq */
            int prec;
            struct spktq* suppr_psq;
            for (prec = 0; prec < WLC_PREC_COUNT; prec++) {
                suppr_psq = &(*cubby_info)->supr_psq[prec];
#ifdef HNDPQP
                ASSERT(!pqp_owns(&suppr_psq->q));
#endif /* HNDPQP */
                spktq_deinit(suppr_psq);

            }
        }
#endif /* WL_USE_SUPR_PSQ */

        /* Now clear the pktq_log allocated buffer */
#ifdef PKTQ_LOG
        wlc_pktq_stats_free(wlc, psq);
#endif
#ifdef HNDPQP
        ASSERT(!pktq_mpqp_owns(psq, WLC_PREC_BMP_ALL));
#endif
        pktq_deinit(psq);

        wlc_scb_sec_cubby_free(wlc, remove, *cubby_info);
        *cubby_info = NULL;
    }
}

static uint
wlc_apps_scb_psinfo_secsz(void *context, struct scb *scb)
{
    wlc_bsscfg_t *cfg;
    BCM_REFERENCE(context);

    cfg = SCB_BSSCFG(scb);
    ASSERT(cfg != NULL);

    if (BSSCFG_HAS_PSINFO(cfg)) {
        return sizeof(apps_scb_psinfo_t);
    }

    return 0;
}

void
wlc_apps_dbg_dump(wlc_info_t *wlc, int hi, int lo)
{
    struct scb *scb;
    struct scb_iter scbiter;
    struct apps_scb_psinfo *scb_psinfo;

    printf("WLC discards:%d, ps_deferred:%d\n",
        wlc->psinfo->ps_discard,
            wlc->psinfo->ps_deferred);

    FOREACHSCB(wlc->scbstate, &scbiter, scb) {
        scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
        if (scb_psinfo == NULL)
            continue;

        printf("scb at %p for [%02x:%02x:%02x:%02x:%02x:%02x]\n",
            OSL_OBFUSCATE_BUF(scb),
            scb->ea.octet[0], scb->ea.octet[1], scb->ea.octet[2],
                scb->ea.octet[3], scb->ea.octet[4], scb->ea.octet[5]);
        printf("  (psq_items,state)=(%d,%s), psq_bucket: %d items\n",
            scb_psinfo->psq.n_pkts_tot,
            ((scb->PS == FALSE) ? " OPEN" : "CLOSE"),
            scb_psinfo->psq.n_pkts_tot);
    }

    if (hi) {
        printf("setting psq (hi,lo) to (%d,%d)\n", hi, lo);
        wlc->psinfo->psq_pkts_hi = hi;
        wlc->psinfo->psq_pkts_lo = lo;
    } else {
        printf("leaving psq (hi,lo) as is (%d,%d)\n",
               wlc->psinfo->psq_pkts_hi, wlc->psinfo->psq_pkts_lo);
    }

    return;
}

void
wlc_apps_apsd_usp_end(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

#ifdef WLTDLS
    if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
        if (wlc_tdls_in_pti_interval(wlc->tdls, scb))
            return;
        wlc_tdls_apsd_usp_end(wlc->tdls, scb);
    }
#endif /* WLTDLS */

    scb_psinfo->apsd_usp = FALSE;

#ifdef WLTDLS
    if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
        (wlc_apps_apsd_delv_count(wlc, scb) > 0)) {
        /* send PTI again */
        wlc_tdls_send_pti(wlc->tdls, scb);
    }
#endif /* WLTDLS */

}

#ifdef BCMDBG
void
wlc_apps_print_ps_stuck_info(wlc_info_t *wlc, struct scb *scb)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    uint32 ps_in_pvb_duration = 0;
    uint32 ps_duration = 0;
    uint32 i;

    if (!scb_psinfo || !SCB_PS(scb)) {
        return;
    }

    if (wlc->clk) {
        ps_duration = wlc_lifetime_now(wlc) - scb_psinfo->last_in_ps_tsf;
        ps_in_pvb_duration = wlc_lifetime_now(wlc) -
            scb_psinfo->last_in_pvb_tsf;
    }

    WL_ERROR(("\tinPS: %d, inPVB %d, APPS PSQ len:%d\n"
        "ps_trans_status 0x%x, ps_requester 0x%x, block_datafifo 0x%x",
        ps_duration,
        ps_in_pvb_duration,
        pktq_n_pkts_tot(&scb_psinfo->psq),
        scb_psinfo->ps_trans_status,
        scb_psinfo->ps_requester,
        wlc->block_datafifo));

#ifdef PSPRETEND
    if (SCB_PS_PRETEND(scb)) {
        WL_ERROR((", ps_pretend 0x%x\n", scb->ps_pretend));
    } else
#endif /* PSPRETEND */
    {
        WL_ERROR(("\n"));
    }

#if defined(WL_PS_SCB_TXFIFO_BLK)
    WL_ERROR(("ps_scb_txfifo_blk: %d,  ps_txfifo_blk_scb_cnt: %d\n",
        wlc->ps_scb_txfifo_blk, wlc->ps_txfifo_blk_scb_cnt));
    WL_ERROR(("ps_txfifo_blk: %d,  pkts_inflt_fifocnt_tot:%d\n",
        SCB_PS_TXFIFO_BLK(scb), SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)));
#endif /* WL_PS_SCB_TXFIFO_BLK */

    for (i = 0; i < NUMPRIO; i++) {
        if (SCB_PKTS_INFLT_FIFOCNT_VAL(scb, i)) {
            WL_ERROR(("\tpkts_inflt_fifocnt: prio-%d =>%d\n", i,
                SCB_PKTS_INFLT_FIFOCNT_VAL(scb, i)));
        }
    }

    for (i = 0; i < (NUMPRIO * 2); i++) {
        if (SCB_PKTS_INFLT_CQCNT_VAL(scb, i)) {
            WL_ERROR(("\tpkts_inflt_cqcnt: prec-%d =>%d\n", i,
                SCB_PKTS_INFLT_CQCNT_VAL(scb, i)));
        }
    }
}
#endif /* BCMDBG */

static INLINE void
_wlc_apps_scb_ps_off(wlc_info_t *wlc, struct scb *scb, struct apps_scb_psinfo *scb_psinfo,
    wlc_bsscfg_t *bsscfg)
{
#ifdef WLTAF
    /* Update TAF state */
    wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, TAF_PARAM(scb->PS),
        TAF_SCBSTATE_POWER_SAVE);
#endif /* WLTAF */
#ifdef WLCFP
    if (CFP_ENAB(wlc->pub) == TRUE) {
        /* CFP path doesn't handle out of order packets.
         * There might be some OOO packets queued from legacy
         * patch when STA was in PS. So drain those packets
         * first before actually resume CFP.
         */
        if (SCB_AMPDU(scb)) {
            wlc_ampdu_txeval_alltid(wlc, scb, TRUE);
        }
    }
#endif /* WLCFP */

    scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_IN_PROG;
#ifdef WL_BEAMFORMING
    if (TXBF_ENAB(wlc->pub)) {
        /* Notify txbf module of the scb's PS change */
        wlc_txbf_scb_ps_notify(wlc->txbf, scb, FALSE);
    }
#endif /* WL_BEAMFORMING */

#ifdef PROP_TXSTATUS
    if (PROP_TXSTATUS_ENAB(wlc->pub)) {
        wlc_check_txq_fc(wlc, SCB_WLCIFP(scb)->qi);
#ifdef WLAMPDU
        if (AMPDU_ENAB(wlc->pub)) {
            wlc_check_ampdu_fc(wlc->ampdu_tx, scb);
        }
#endif /* WLAMPDU */
    }
#endif /* PROP_TXSTATUS */
    if (scb_psinfo->change_scb_state_to_auth) {
        wlc_scb_resetstate(wlc, scb);
        wlc_scb_setstatebit(wlc, scb, AUTHENTICATED);

        wlc_bss_mac_event(wlc, bsscfg, WLC_E_DISASSOC_IND, &scb->ea,
            WLC_E_STATUS_SUCCESS, DOT11_RC_BUSY, 0, NULL, 0);
        scb_psinfo->change_scb_state_to_auth = FALSE;
    }

}

/* This routine deals with all PS transitions from ON->OFF */
void
wlc_apps_scb_ps_off(wlc_info_t *wlc, struct scb *scb, bool discard)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    struct scb *bcmc_scb;
    apps_bss_info_t *bss_info;
    wlc_bsscfg_t *bsscfg;
    struct ether_addr ea;

    /* sanity */
    ASSERT(scb);

    bsscfg = SCB_BSSCFG(scb);
    ASSERT(bsscfg != NULL);

    bss_info = APPS_BSSCFG_CUBBY(wlc_psinfo, bsscfg);
    ASSERT(bss_info);

    /* process ON -> OFF PS transition */
    WL_PS_EX(scb, ("wl%d.%d: %s "MACF" AID %d\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
        __FUNCTION__, ETHER_TO_MACF(scb->ea), SCB_AID(scb)));

#if defined(BCMDBG) && defined(PSPRETEND)
    if (SCB_PS_PRETEND(scb)) {
        wlc_pspretend_scb_time_upd(wlc->pps_info, scb);
    }
#endif /* PSPRETEND */

    /* update PS state info */
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    /* if already in this process but came here due to pkt callback then
     * just return.
     */
    if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_IN_PROG))
        return;
    scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_IN_PROG;

    ASSERT(bss_info->ps_nodes);
    if (bss_info != NULL) {
        bss_info->ps_nodes--;
    }

    ASSERT(wlc_psinfo->ps_nodes_all);
    wlc_psinfo->ps_nodes_all--;
    ASSERT(scb->PS);
    scb->PS = FALSE;
#ifdef WL_PS_STATS
    wlc_apps_upd_pstime(scb);
#endif /* WL_PS_STATS */
    scb_psinfo->last_in_ps_tsf = 0;
    scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_PEND;

#ifdef PSPRETEND
    if (wlc->pps_info != NULL) {
        wlc_pspretend_scb_ps_off(wlc->pps_info, scb);
    }
#endif /* PSPRETEND */

#if defined(BCMPCIEDEV)
    /* Reset all pending fetch and rollback flow fetch ptrs */
    wlc_scb_flow_ps_update(wlc->wl, SCB_FLOWID(scb), FALSE);
#endif

    /* XXX wlc_wlfc_scb_ps_off did a reset of AMPDU seq with a BAR.
     * Done only for FD mode. Required .??
     */

    /* Reset Packet waiting status */
    SCB_PSPOLL_PKT_WAITING(scb_psinfo) =  FALSE;

    /* Unconfigure the APPS from the txpath */
    wlc_txmod_unconfig(wlc->txmodi, scb, TXMOD_APPS);

    /* If this is last STA to leave PS mode,
     * trigger BCMC FIFO drain and
     * set BCMC traffic to go through regular fifo
     */
    if ((bss_info != NULL) && (bss_info->ps_nodes == 0) && BSSCFG_HAS_BCMC_SCB(bsscfg) &&
        !(bss_info->ps_trans_status & BSS_PS_ON_BY_TWT)) {
        WL_PS_EX(scb, ("wl%d.%d: %s - bcmc off\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
            __FUNCTION__));
        bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);

        if (MBSS_ENAB(wlc->pub)) { /* MBSS PS handling is a bit more complicated. */
            wlc_apps_bss_ps_off_start(wlc, bcmc_scb);
        } else {
            bcmc_scb->PS = FALSE;
#ifdef WL_PS_STATS
            wlc_apps_upd_pstime(bcmc_scb);
#endif /* WL_PS_STATS */
#ifdef PSPRETEND
            // Does PSPretend really apply to bcmc?
            bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */
            /* If packets are pending in TX_BCMC_FIFO,
             * then ask ucode to transmit them immediately
             */
            if (!wlc->bcmcfifo_drain && TXPKTPENDGET(wlc, TX_BCMC_FIFO)) {
                wlc->bcmcfifo_drain = TRUE;
                wlc_mhf(wlc, MHF2, MHF2_TXBCMC_NOW, MHF2_TXBCMC_NOW, WLC_BAND_AUTO);
            }
        }
    }

    scb_psinfo->psp_pending = FALSE;
    scb_psinfo->flags &= ~SCB_PS_FIRST_SUPPR_HANDLED;
    wlc_apps_apsd_usp_end(wlc, scb);
    scb_psinfo->apsd_cnt = 0;

    /* save ea before calling wlc_apps_ps_flush */
    ea = scb->ea;

    /* clear the PVB entry since we are leaving PM mode */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }

    /* Note: We do not clear up any pending PS-POLL pkts
     * which may be enq'd with the IGNOREPMQ bit set. The
     * relevant STA should stay awake until it rx's these
     * response pkts
     */

    if (discard == FALSE) {
#ifdef HNDPQP
        /* Page in all Host resident packets into dongle */
        wlc_apps_scb_pqp_pgi(wlc, scb, WLC_PREC_BMP_ALL, PKTQ_PKTS_ALL, PQP_PGI_PS_OFF);
#endif /* HNDPQP */

        /* Move pmq entries to Q1 (ctl) for immediate tx */
        while (wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0))
            ;
#ifdef HNDPQP
        /* If there are still pkts in host, it means PQP is out of resource.
         * PQP will set flags for current PS transition and
         * resume the remain process when resource is available.
         */
        if (PQP_PGI_PS_TRANS_OFF_ISSET(scb_psinfo))
            return;
#endif /* HNDPQP */
    } else { /* free any pending frames */
        wlc_apps_ps_flush(wlc, scb);

        /* callbacks in wlc_apps_ps_flush are not allowed to free scb */
        if (!ETHER_ISMULTI(&ea) && (wlc_scbfind(wlc, bsscfg, &ea) == NULL)) {
            WL_ERROR(("wl%d.%d: %s exiting, scb for "MACF" was freed\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
                __FUNCTION__, ETHER_TO_MACF(ea)));
            ASSERT(0);
            return;
        }
    }

    _wlc_apps_scb_ps_off(wlc, scb, scb_psinfo, bsscfg);
}

static void
wlc_apps_bcmc_scb_ps_on(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
    struct scb *bcmc_scb;
    /*  Use the bsscfg pointer of this scb to help us locate the
     *  correct bcmc_scb so that we can turn on PS
     */
    bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
    ASSERT(bcmc_scb->bsscfg == bsscfg);

    if (SCB_PS(bcmc_scb)) {
        WL_PS0(("wl%d.%d: %s [bcmc_scb] Already in PS!\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
        /* ps on had already been processed by bcmc pmq suppressed pkt,
         * and now we are here upon successive unicast pmq suppressed pkt
         * or actual pmq entry addition interrupt.
         * Therefore do nothing here and just get out.
         */
        return;
    }
#if defined(MBSS) && (defined(BCMDBG) || defined(WLMSG_PS))
    if (MBSS_ENAB(wlc->pub)) {
        uint32 mc_fifo_pkts = wlc_mbss_get_bcmc_pkts_sent(wlc, bsscfg);

        if (mc_fifo_pkts != 0) {
            WL_PS_EX(bcmc_scb, ("wl%d.%d: %s START PS-ON; bcmc %d\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, mc_fifo_pkts));
        }
    }
#endif /* MBSS && (BCMDBG || WLMSG_PS) */

    WL_PS0(("wl%d.%d: %s bcmc SCB PS on!\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
    bcmc_scb->PS = TRUE;
#ifdef WL_PS_STATS
    /* skip the counter increment if PS is already on */
    if (bcmc_scb->ps_starttime == 0) {
        bcmc_scb->ps_on_cnt++;
        bcmc_scb->ps_starttime = OSL_SYSUPTIME();
    }
#endif /* WL_PS_STATS */
    if (wlc->bcmcfifo_drain) {
        wlc->bcmcfifo_drain = FALSE;
        wlc_mhf(wlc, MHF2, MHF2_TXBCMC_NOW, 0, WLC_BAND_AUTO);
    }
}

/* This deals with all PS transitions from OFF->ON */
void
wlc_apps_scb_ps_on(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo;
    apps_bss_info_t *bss_info;
    wlc_bsscfg_t *bsscfg;

    ASSERT(scb);

    bsscfg = SCB_BSSCFG(scb);
    ASSERT(bsscfg != NULL);

    if (BSSCFG_STA(bsscfg) && !BSS_TDLS_ENAB(wlc, bsscfg) && !BSSCFG_IBSS(bsscfg)) {
        WL_PS_EX(scb, ("wl%d.%d: %s "MACF" AID %d: BSSCFG_STA(bsscfg)=%s, "
            "BSS_TDLS_ENAB(wlc,bsscfg)=%s\n\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            ETHER_TO_MACF(scb->ea), SCB_AID(scb),
            BSSCFG_STA(bsscfg) ? "TRUE" : "FALSE",
            BSS_TDLS_ENAB(wlc, bsscfg) ? "TRUE" : "FALSE"));
        return;
    }

    /* process OFF -> ON PS transition */
    wlc_psinfo = wlc->psinfo;
    bss_info = APPS_BSSCFG_CUBBY(wlc_psinfo, bsscfg);
    ASSERT(bss_info);

    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    WL_PS_EX(scb, ("wl%d.%d: %s "MACF" AID %d in_pvb %d fifo pkts pending %d\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ETHER_TO_MACF(scb->ea),
        SCB_AID(scb), scb_psinfo->in_pvb, TXPKTPENDTOT(wlc)));

    scb_psinfo->flags &= ~SCB_PS_FIRST_SUPPR_HANDLED;

    /* update PS state info */
    bss_info->ps_nodes++;
    wlc_psinfo->ps_nodes_all++;
    ASSERT(!scb->PS);
    scb->PS = TRUE;
#ifdef WL_PS_STATS
    if (scb->ps_starttime == 0) {
        scb->ps_on_cnt++;
        scb->ps_starttime = OSL_SYSUPTIME();
    }
#endif /* WL_PS_STATS */
    if (wlc->clk) {
        scb_psinfo->last_in_ps_tsf = wlc_lifetime_now(wlc);
    }

    scb_psinfo->tbtt = 0;

#ifdef WLTAF
    /* Update TAF state */
    wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, TAF_PARAM(scb->PS),
        TAF_SCBSTATE_POWER_SAVE);
#endif /* WLTAF */

#if defined(BCMPCIEDEV)
    /* Reset all pending fetch and rollback flow fetch ptrs */
    wlc_scb_flow_ps_update(wlc->wl, SCB_FLOWID(scb), TRUE);
#endif
    /* If this is first STA to enter PS mode, set BCMC traffic
     * to go through BCMC Fifo. If bcmcfifo_drain is set, then clear
     * the drain bit.
     */
    if (bss_info->ps_nodes == 1 && BSSCFG_HAS_BCMC_SCB(bsscfg)) {
        wlc_apps_bcmc_scb_ps_on(wlc, bsscfg);
    }

    /* Add the APPS to the txpath for this SCB */
    wlc_txmod_config(wlc->txmodi, scb, TXMOD_APPS);

    /* ps enQ any pkts on the txq, narq, ampduq */
    wlc_apps_txq_to_psq(wlc, scb);

    /* XXX TODO : Rollback packets to flow ring rather than
     * sending them into PSq during PS transition for PQP
     */
#ifdef PROP_TXSTATUS
#ifdef WLAMPDU
    /* This causes problems for PSPRETEND */
    wlc_apps_ampdu_txq_to_psq(wlc, scb);
#endif /* WLAMPDU */
#ifdef WLNAR
    wlc_apps_nar_txq_to_psq(wlc, scb);
#endif /* WLNAR */
#endif /* PROP_TXSTATUS */

    /* If there is anything in the data fifo then allow it to drain */
#if defined(WL_PS_SCB_TXFIFO_BLK)
    if (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)) {
        scb->ps_txfifo_blk = TRUE;
        wlc->ps_scb_txfifo_blk = TRUE;
        wlc->ps_txfifo_blk_scb_cnt++;
    }
#else /* ! WL_PS_SCB_TXFIFO_BLK */

    if (TXPKTPENDTOT(wlc) > 0) {
        wlc_block_datafifo(wlc, DATA_BLOCK_PS, DATA_BLOCK_PS);

#if defined(WL_PS_STATS)
        /* only update time when previous DATA_BLOCK_PS is off */
        if (wlc->datablock_starttime == 0) {
            int pktpend_cnts = TXPKTPENDTOT(wlc);

            wlc->datablock_cnt++;
            wlc->datablock_starttime = OSL_SYSUPTIME_US();

            if ((wlc->pktpend_min == 0) || (wlc->pktpend_min > pktpend_cnts)) {
                wlc->pktpend_min = pktpend_cnts;
            }

            if (wlc->pktpend_max < pktpend_cnts) {
                wlc->pktpend_max = pktpend_cnts;
            }

            wlc->pktpend_tot += pktpend_cnts;
        }
#endif /* WL_PS_STATS */
    }
#endif /* ! WL_PS_SCB_TXFIFO_BLK */

#ifdef WL_BEAMFORMING
    if (TXBF_ENAB(wlc->pub)) {
        /* Notify txbf module of the scb's PS change */
        wlc_txbf_scb_ps_notify(wlc->txbf, scb, TRUE);
    }
#endif /* WL_BEAMFORMING */

#ifdef HNDPQP
    /* PQP page out on SCB PS ON */
    wlc_apps_scb_pqp_pgo(wlc, scb, WLC_PREC_BMP_ALL);
#endif
}

/* Mark the source of PS on/off request so that one doesn't turn PS off while
 * another source still needs PS on.
 */
void
wlc_apps_ps_requester(wlc_info_t *wlc, struct scb *scb, uint8 on_rqstr, uint8 off_rqstr)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);

    if ((scb_psinfo && ((scb_psinfo->ps_requester & on_rqstr) == 0) && (on_rqstr)) ||
        (scb_psinfo->ps_requester & off_rqstr)) {
        WL_PS_EX(scb, ("wl%d.%d: %s AID %d ON 0x%x OFF 0x%x   PS 0x%x\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            SCB_AID(scb), on_rqstr, off_rqstr, scb_psinfo->ps_requester));
    }

    if (wlc->clk && on_rqstr) {
        scb_psinfo->last_in_ps_tsf = wlc_lifetime_now(wlc);
    }
    scb_psinfo->ps_requester |= on_rqstr;
    scb_psinfo->ps_requester &= ~off_rqstr;
}

/* Workaround to prevent scb to remain in PS, waiting on an OMI PMQ or OMI HTC.
 * Both need to be received to exit PS, adding a timeout to guard against stuck condition.
 */
static void
wlc_apps_omi_waitpmq_war(wlc_info_t *wlc)
{
    struct scb_iter scbiter;
    struct scb *scb;
    struct apps_scb_psinfo *scb_psinfo;
    uint32 ps_duration, cur_time;

    FOREACHSCB(wlc->scbstate, &scbiter, scb) {
        scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
        if (!scb_psinfo) {
            continue;
        }
        cur_time = wlc_lifetime_now(wlc);
        ps_duration = ((cur_time - scb_psinfo->last_in_ps_tsf) +
                500) / 1000;
        if (SCB_PS(scb) && (ps_duration >= 1000)) {
            if (scb_psinfo->ps_requester & PS_SWITCH_OMI) {
                WL_ERROR(("wl%d.%d "MACF": %s: WAR No OMI PMQ entry\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    ETHER_TO_MACF(scb->ea), __FUNCTION__));
                wlc_apps_ps_requester(wlc, scb, 0, PS_SWITCH_OMI);
                wlc_he_omi_pmq_reset(wlc, scb);
                wlc_apps_process_ps_switch(wlc, scb, PS_SWITCH_OFF);
            }
        }
    }
}

/* "normalize" a packet queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
static bool
is_pkt_wlf3_supr(void *pkt) /* callback to pktq_promote */
{
    if (WLPKTTAG(pkt)->flags3 & WLF3_SUPR) {
        WLPKTTAG(pkt)->flags3 &= ~WLF3_SUPR;
        return TRUE;
    }
    return FALSE;
}

/* Version operation on common queue */
static void
cpktq_supr_norm(wlc_info_t *wlc, struct cpktq *cpktq)
{
    struct pktq *pktq = &cpktq->cq;    /** Multi-priority packet queue */

    BCM_REFERENCE(wlc);

    if (pktq_n_pkts_tot(pktq) == 0)
        return;

    pktq_promote(pktq, is_pkt_wlf3_supr);
}

#ifdef WL_USE_SUPR_PSQ
/* Version operation on ps suppress queue */
static void
wlc_apps_dequeue_scb_supr_psq(wlc_info_t *wlc, struct scb *scb)
{
    int prec, discards;
    struct apps_scb_psinfo *scb_psinfo;
    void *pkt;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    ASSERT(scb_psinfo);
#ifdef HNDPQP
    /* PQP handles normalizing(joining) PS Q and suppress PS Q */
    wlc_apps_scb_suprq_pqp_normalize(wlc, scb);
    return;
#endif /* HNDPQP */

    if (scb_psinfo->suppressed_pkts > pktq_avail(&scb_psinfo->psq)) {
        /* SCB PSQ has no room for supr_psq. Discard the packets from the lowest prec psq */
        discards = scb_psinfo->suppressed_pkts - pktq_avail(&scb_psinfo->psq);

        WL_PS_EX(scb, (" %s: "MACF" PSQ discard %d suprs %d psqlen %d psq_avail %d\n",
            __FUNCTION__, ETHER_TO_MACF(scb->ea),
            discards, scb_psinfo->suppressed_pkts,
            pktq_n_pkts_tot(&scb_psinfo->psq),
            pktq_avail(&scb_psinfo->psq)));

        for (prec = 0; (prec < WLC_PREC_COUNT) && (discards > 0); prec++) {
            while (discards > 0) {
                pkt = pktq_pdeq_tail(&scb_psinfo->psq, prec);
                if (pkt == NULL)
                    break;

                discards--;
                wlc->psinfo->ps_discard++;
                scb_psinfo->ps_discard++;
                if (!SCB_ISMULTI(scb)) {
                    wlc->psinfo->ps_deferred--;
                }
                PKTFREE(wlc->osh, pkt, TRUE);
            }
        }
    }

    for (prec = (WLC_PREC_COUNT-1); prec >= 0; prec--) {
        if (spktq_n_pkts(&scb_psinfo->supr_psq[prec]) == 0) {
            continue;
        }
        WL_PS_EX(scb, (" %s: (Norm) prec %d suplen %d psqlen %d\n", __FUNCTION__,
            prec, spktq_n_pkts(&scb_psinfo->supr_psq[prec]),
            pktq_n_pkts_tot(&scb_psinfo->psq)));

        pktq_prepend(&scb_psinfo->psq, prec, &scb_psinfo->supr_psq[prec]);
    }

    ASSERT(pktq_n_pkts_tot(&scb_psinfo->psq) <= PSQ_PKTQ_LEN_DEFAULT);
    scb_psinfo->suppressed_pkts = 0;
}
#else /* ! WL_USE_SUPR_PSQ */
/* Version operation on txq */
/** @param pktq   Multi-priority packet queue */
static void
wlc_pktq_supr_norm(wlc_info_t *wlc, struct pktq *pktq)
{
    BCM_REFERENCE(wlc);

    if (pktq_n_pkts_tot(pktq) == 0)
        return;

    pktq_promote(pktq, is_pkt_wlf3_supr);
}
#endif /* ! WL_USE_SUPR_PSQ */

/* "normalize" the SCB's PS queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
void
wlc_apps_scb_psq_norm(wlc_info_t *wlc, struct scb *scb)
{
    apps_wlc_psinfo_t *wlc_psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq *pktq;        /**< multi-priority packet queue */

    BCM_REFERENCE(pktq);
    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    pktq = &scb_psinfo->psq;

    /* Only normalize if necessary */
    if (scb_psinfo->suppressed_pkts) {
#ifdef WL_USE_SUPR_PSQ
        wlc_apps_dequeue_scb_supr_psq(wlc, scb);
#else
        wlc_pktq_supr_norm(wlc, pktq);
        scb_psinfo->suppressed_pkts = 0;
#endif
        /* In the case of traffic oversubscription, the low priority
         * packets may stay long in TxFIFO and be suppreesed by MAC
         * when the STA goes to sleep.  Resetting scb ampdu alive may
         * avoid unnecessary ampdu session reset.
         */
        wlc_ampdu_scb_reset_alive(wlc, scb);
    }
}

/* "normalize" the BSS's txq queue - move packets tagged with WLF3_SUPR flag
 * to the front and retain the order in case other packets were inserted
 * in the queue before.
 */
static void
wlc_apps_bsscfg_txq_norm(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
    /* Only normalize if necessary. Normalizing a large txq can
     * take >= 100 milliseconds under the context of a soft isr.
     */
    if (cfg->wlcif->qi->suppressed_pkts) {
        cfg->wlcif->qi->suppressed_pkts = 0;
        cpktq_supr_norm(wlc, &cfg->wlcif->qi->cpktq);
    }
}

#if defined(WL_PS_SCB_TXFIFO_BLK)

/* Process any pending PS states */
void
wlc_apps_process_pend_ps(wlc_info_t *wlc)
{
    struct scb *scb;
    int idx;
    wlc_bsscfg_t *cfg;

    ASSERT(wlc->ps_scb_txfifo_blk);

    wlc_block_datafifo(wlc, DATA_BLOCK_PS, 0);

#ifdef WL_PS_STATS
    if (wlc->datablock_starttime != 0) {
        uint32 datablock_delta;
        uint32 datablock_now = OSL_SYSUPTIME_US();

        datablock_delta = datablock_now - wlc->datablock_starttime;
        if (datablock_delta > wlc->datablock_maxtime) {
            wlc->datablock_maxtime = datablock_delta;
        }

        if ((wlc->datablock_mintime == 0) ||
                (wlc->datablock_mintime > datablock_delta)) {
            wlc->datablock_mintime = datablock_delta;
        }

        wlc->datablock_tottime += datablock_delta;
        wlc->datablock_starttime = 0;
    }

    /* TODO: Similar debug logic for Data Block per STA (ps_scb_txfifo_blk) feature */
#endif /* WL_PS_STATS */

    /* If no packets are pending for all stations is PS TxFIFO block state,
     * Clear global PS fifo drain & normalize bcmc PSQ
     */
    if (wlc->ps_txfifo_blk_scb_cnt == 0) {

        wlc->ps_scb_txfifo_blk = FALSE;

        /* Notify bmac to clear the PMQ */
        wlc_pmq_process_ps_switch(wlc, NULL, PS_SWITCH_FIFO_FLUSHED);

        FOREACH_BSS(wlc, idx, cfg) {
            scb = WLC_BCMCSCB_GET(wlc, cfg);
            if (scb && SCB_PS(scb)) {
                WL_PS_EX(scb, ("wl%d.%d %s: Normalizing bcmc PSQ of BSSID "MACF"\n",
                    wlc->pub->unit, idx, __FUNCTION__,
                    ETHER_TO_MACF(cfg->BSSID)));
                wlc_apps_bsscfg_txq_norm(wlc, cfg);
            }
        }

        if (MBSS_ENAB(wlc->pub)) {
            wlc_apps_bss_ps_on_done(wlc);
        }
        /* If any suppressed BCMC packets at the head of txq,
         * they need to be sent to hw fifo right now.
         */
        if (wlc->active_queue != NULL && WLC_TXQ_OCCUPIED(wlc)) {
            wlc_send_q(wlc, wlc->active_queue);
        }
    }
}

#else /* ! WL_PS_SCB_TXFIFO_BLK */

/* Process any pending PS states */
void
wlc_apps_process_pend_ps(wlc_info_t *wlc)
{
    struct scb_iter scbiter;
    struct scb *scb;
    int idx;
    wlc_bsscfg_t *cfg;

    /* PMQ todo : we should keep track of pkt pending for each scb and wait for
       individual drains, instead of blocking and draining the whole pipe.
    */

    ASSERT(wlc->block_datafifo & DATA_BLOCK_PS);
    ASSERT(TXPKTPENDTOT(wlc) == 0);

    WL_PS0(("%s unblocking fifo\n", __FUNCTION__));
    /* XXX Should the clearing of block_datafifo be done after
     * folding pkts to normal TxQs in case this triggers tx operations?
     */
    wlc_block_datafifo(wlc, DATA_BLOCK_PS, 0);
#ifdef WL_PS_STATS
    {
        uint32 datablock_delta;
        uint32 datablock_now = OSL_SYSUPTIME_US();
        /* in case starttime was cleared */
        if (wlc->datablock_starttime != 0) {
            datablock_delta = datablock_now - wlc->datablock_starttime;
            if (datablock_delta > wlc->datablock_maxtime) {
                wlc->datablock_maxtime = datablock_delta;
            }

            if ((wlc->datablock_mintime == 0) ||
                (wlc->datablock_mintime > datablock_delta)) {
                wlc->datablock_mintime = datablock_delta;
            }

            wlc->datablock_tottime += datablock_delta;
            wlc->datablock_starttime = 0;
        }
    }
#endif /* WL_PS_STATS */

    /* notify bmac to clear the PMQ */
    wlc_pmq_process_ps_switch(wlc, NULL, PS_SWITCH_FIFO_FLUSHED);

    FOREACH_BSS(wlc, idx, cfg) {
        scb = WLC_BCMCSCB_GET(wlc, cfg);
        if (scb && SCB_PS(scb)) {
            WL_PS_EX(scb, ("wl%d.%d %s Normalizing bcmc PSQ of BSSID "MACF"\n",
                wlc->pub->unit, idx, __FUNCTION__, ETHER_TO_MACF(cfg->BSSID)));
            wlc_apps_bsscfg_txq_norm(wlc, cfg);
        }
    }
    FOREACHSCB(wlc->scbstate, &scbiter, scb) {
        WL_PS_EX(scb, ("wl%d.%d %s Normalizing PSQ for STA "MACF"\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            ETHER_TO_MACF(scb->ea)));
        if (SCB_PS(scb) && !SCB_ISMULTI(scb)) {
            struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
            ASSERT(scb_psinfo);

            /* XXX Note that this is being done for every unicast PS SCB,
             * instead of just the ones that are transitioning to PS ON. If we
             * had a state for an SCB in transition to ON, then we could focus
             * this work.
             */
            wlc_apps_scb_psq_norm(wlc, scb);
            if (scb_psinfo->twt_wait4drain_enter) {
                wlc_apps_twt_enter_ready(wlc, scb);
                scb_psinfo->twt_wait4drain_enter = FALSE;
            } else if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_PEND)) {
                /* we may get here as result of SCB deletion, so avoid
                 * re-enqueueing frames in that case by discarding them
                 */

                bool discard = SCB_DEL_IN_PROGRESS(scb) ? TRUE : FALSE;
#ifdef PSPRETEND
                if (SCB_PS_PRETEND_BLOCKED(scb)) {
                    WL_ERROR(("wl%d.%d: %s SCB_PS_PRETEND_BLOCKED, "
                        "expected to see PMQ PPS entry\n",
                        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                        __FUNCTION__));
                }
#endif /* PSPRETEND */
                WL_PS_EX(scb, ("wl%d.%d %s Allowing PS Off for STA "MACF"\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, ETHER_TO_MACF(scb->ea)));
                wlc_apps_scb_ps_off(wlc, scb, discard);
            }
        }
#ifdef PSPRETEND
        if (SCB_CLEANUP_IN_PROGRESS(scb) || SCB_DEL_IN_PROGRESS(scb) ||
            SCB_MARKED_DEL(scb)) {
            WL_PS_EX(scb, ("wl%d.%d: %s scb del in progress or  marked for del \n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__));
        } else if (SCB_PS_PRETEND_PROBING(scb)) {
            wlc_pspretend_probe_sched(wlc->pps_info, scb);
        }
#endif /* PSPRETEND */
    }

    if (MBSS_ENAB(wlc->pub)) {
        wlc_apps_bss_ps_on_done(wlc);
    }
    /* If any suppressed BCMC packets at the head of txq,
     * they need to be sent to hw fifo right now.
     */
    if (wlc->active_queue != NULL && WLC_TXQ_OCCUPIED(wlc)) {
        wlc_send_q(wlc, wlc->active_queue);
    }
}
#endif /* ! WL_PS_SCB_TXFIFO_BLK */

/* wlc_apps_ps_flush()
 * Free any pending PS packets for this STA
 *
 * Called when APPS is handling a driver down transision, when an SCB is deleted,
 * when wlc_apps_scb_ps_off() is called with the discard param
 *    - from wlc_scb_disassoc_cleanup()
 *    - when a STA re-associates
 *    - from a deauth completion
 */
void
wlc_apps_ps_flush(wlc_info_t *wlc, struct scb *scb)
{
    uint8 tid;
#if defined(BCMDBG)
    struct apps_scb_psinfo *scb_psinfo;
    BCM_REFERENCE(scb_psinfo);
#endif

    for (tid = 0; tid < NUMPRIO; tid++) {
        wlc_apps_ps_flush_tid(wlc, scb, tid);
    }
#if defined(BCMDBG)
    {
        scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
        ASSERT(scb_psinfo == NULL || pktq_empty(&scb_psinfo->psq));
    }
#endif
}

/* When a flowring is deleted or scb is removed, the packets related to that flowring or scb have
 * to be flushed.
 */
static void
wlc_apps_ps_flush_tid(void *context, struct scb *scb, uint8 tid)
{
    wlc_info_t *wlc = (wlc_info_t *)context;
    int prec;
    uint32 n_pkts_flushed = 0;
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo;
    struct pktq *pq;

    ASSERT(scb);
    ASSERT(wlc);

    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    if (scb_psinfo == NULL)
        return;

#ifdef WL_USE_SUPR_PSQ
    /* Checking PS suppress queue and moving ps suppressed frames accordingly */
    if (scb_psinfo->suppressed_pkts > 0) {
        wlc_apps_dequeue_scb_supr_psq(wlc, scb);
    }
#endif /* WL_USE_SUPR_PSQ */

    pq = &scb_psinfo->psq;

    if (scb_psinfo->flags & SCB_PS_FLUSH_IN_PROGR) {
        WL_PS_EX(scb, ("wl%d.%d: %s flush SCB "MACF" in progress\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea)));
        return;
    }
    scb_psinfo->flags |= SCB_PS_FLUSH_IN_PROGR;
    prec = WLC_PRIO_TO_PREC(tid);

#ifdef HNDPQP
    /* Page in all Host resident packets into dongle */
    n_pkts_flushed += wlc_apps_scb_pqp_ps_flush_prec(wlc, scb, pq, prec);
#else  /* !HNDPQP */
    if (!pktqprec_empty(pq, prec)) {
        n_pkts_flushed += wlc_txq_pktq_pflush(wlc, pq, prec);
    }
#endif /* !HNDPQP */

    prec = WLC_PRIO_TO_HI_PREC(tid);

#ifdef HNDPQP
    /* Page in all Host resident packets into dongle */
    n_pkts_flushed += wlc_apps_scb_pqp_ps_flush_prec(wlc, scb, pq, prec);
#else  /* !HNDPQP */
    if (!pktqprec_empty(pq, prec)) {
        n_pkts_flushed += wlc_txq_pktq_pflush(wlc, pq, prec);
    }
#endif /* !HNDPQP */

    if (n_pkts_flushed > 0) {
        WL_PS_EX(scb, ("wl%d.%d: %s flushing %d packets for "MACF" AID %d tid %d\n",
            wlc->pub->unit,    WLC_BSSCFG_IDX(scb->bsscfg), __FUNCTION__,
            n_pkts_flushed,    ETHER_TO_MACF(scb->ea), SCB_AID(scb), tid));

        if (!SCB_ISMULTI(scb)) {
            ASSERT(wlc_psinfo->ps_deferred >= n_pkts_flushed);
            wlc_psinfo->ps_deferred -= n_pkts_flushed;
        }
    }

    scb_psinfo->flags &= ~SCB_PS_FLUSH_IN_PROGR;

    /* If there is a valid aid (the bcmc scb wont have one) then ensure
     * the PVB is updated.
     */
    if (scb->aid && scb_psinfo->in_pvb)
        wlc_apps_pvb_update(wlc, scb);
}

#ifdef PROP_TXSTATUS
void
wlc_apps_ps_flush_mchan(wlc_info_t *wlc, struct scb *scb)
{
    void *pkt;
    int prec;
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo;
    struct pktq tmp_q;        /**< multi-priority packet queue */

    ASSERT(scb);
    ASSERT(wlc);

    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    if (scb_psinfo == NULL)
        return;
    pktq_init(&tmp_q, WLC_PREC_COUNT, PSQ_PKTQ_LEN_DEFAULT);

    /* Don't care about dequeue precedence */
    while ((pkt = pktq_deq(&scb_psinfo->psq, &prec))) {
        if (!SCB_ISMULTI(scb)) {
            wlc_psinfo->ps_deferred--;
        }

        if (!(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
            WLFC_PKTFLAG_PKTFROMHOST)) {
            pktq_penq(&tmp_q, prec, pkt);
            continue;
        }

        WLPKTTAG(pkt)->flags &= ~WLF_PSMARK; /* clear the timestamp */

        wlc_suppress_sync_fsm(wlc, scb, pkt, TRUE);
        wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS, pkt, FALSE);
        PKTFREE(wlc->osh, pkt, TRUE);

        /* XXX what about this check from wlc_apps_ps_flush()?
         * code after comment: "callback may have freed scb"
         */

     }
    /* Enqueue back the frames generated in dongle */
    while ((pkt = pktq_deq(&tmp_q, &prec))) {

#ifdef BCMFRWDPOOLREORG
        if (BCMFRWDPOOLREORG_ENAB() && PKTISFRWDPKT(wlc->pub->osh, pkt) &&
                PKTISFRAG(wlc->pub->osh, pkt)) {
            PKTFREE(wlc->osh, pkt, TRUE);
        } else
#endif
        {
            pktq_penq(&scb_psinfo->psq, prec, pkt);
            if (!SCB_ISMULTI(scb)) {
                wlc_psinfo->ps_deferred++;
            }
        }
    }

    /* If there is a valid aid (the bcmc scb wont have one) then ensure
     * the PVB is cleared.
     */
    if (scb->aid && scb_psinfo->in_pvb)
        wlc_apps_pvb_update(wlc, scb);
}
#endif /* defined(PROP_TXSTATUS) */

/* Return TRUE if packet has been enqueued on a ps queue, FALSE otherwise */
#define WLC_PS_APSD_HPKT_TIME 12 /* in ms */

bool
wlc_apps_psq(wlc_info_t *wlc, void *pkt, int prec)
{
    apps_wlc_psinfo_t *wlc_psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    struct scb *scb;
    uint psq_len;
    uint psq_len_diff;

    scb = WLPKTTAGSCBGET(pkt);
    ASSERT(SCB_PS(scb) || SCB_TWTPS(scb) || wlc_twt_scb_active(wlc->twti, scb));
    ASSERT(wlc);

    /* Do not enq bcmc pkts on a psq, also
     * ageing out packets may have disassociated the STA, so return FALSE if so
     * unless scb is wds
     */
    if (!SCB_ASSOCIATED(scb) && !BSSCFG_IBSS(SCB_BSSCFG(scb)) && !SCB_WDS(scb)) {
        return FALSE;
    }

    ASSERT(!SCB_ISMULTI(scb));

    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

#ifdef BCMPCIEDEV
    wlc_apps_scb_apsd_dec_v2r_in_transit(wlc, scb, pkt, prec);
#endif /* BCMPCIEDEV */

    if (scb_psinfo->flags & SCB_PS_FLUSH_IN_PROGR) {
        WL_PS_EX(scb, ("wl%d.%d: %s flush in progress\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        return FALSE;
    }

#if defined(PROP_TXSTATUS)
    if (PROP_TXSTATUS_ENAB(wlc->pub)) {
        /* If the host sets a flag marking the packet as "in response to
           credit request for pspoll" then only the fimrware enqueues it.
           Otherwise wlc drops it by sending a wlc_suppress.
        */
        if ((WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
            WLFC_PKTFLAG_PKTFROMHOST) &&
            HOST_PROPTXSTATUS_ACTIVATED(wlc) &&
            (!(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
            WLFC_PKTFLAG_PKT_REQUESTED))) {
            WLFC_DBGMESG(("R[0x%.8x]\n", (WLPKTTAG(pkt)->wl_hdr_information)));
            return FALSE;
        }
    }
#endif /* PROP_TXSTATUS */

#ifdef WL_USE_SUPR_PSQ
    psq_len = pktq_n_pkts_tot(&scb_psinfo->psq) + scb_psinfo->suppressed_pkts;
#else
    psq_len = pktq_n_pkts_tot(&scb_psinfo->psq);
#endif
    /* Deferred PS pkt flow control
     * If this scb currently contains less than the minimum number of PS pkts
     * per scb then enq it. If the total number of PS enq'd pkts exceeds the
     * watermark and more than the minimum number of pkts are already enq'd
     * for this STA then do not enq the pkt.
     */
#ifdef PROP_TXSTATUS
    if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
        FALSE)
#endif /* PROP_TXSTATUS */
    {
        if (psq_len > wlc_psinfo->psq_pkts_lo &&
            wlc_psinfo->ps_deferred > wlc_psinfo->psq_pkts_hi) {
            WL_PS_EX(scb, ("wl%d.%d: %s can't buffer packet for "MACF" AID %d, %d "
                "queued for "
                "scb, %d for WL\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, ETHER_TO_MACF(scb->ea), SCB_AID(scb),
                pktq_n_pkts_tot(&scb_psinfo->psq), wlc_psinfo->ps_deferred));
            return FALSE;
        }
    }

    WL_PS_EX(scb, ("wl%d.%d: %s enq %p to PSQ, prec = 0x%x, scb_psinfo->apsd_usp = %s\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, pkt, prec,
        scb_psinfo->apsd_usp ? "TRUE" : "FALSE"));

#ifdef WL_USE_SUPR_PSQ
    if (WLPKTTAG(pkt)->flags3 & WLF3_SUPR) {
        /* Removing WLF3_SUPR flag since it doesn't require normalization.
         * supr_psq will be prepended to psq.
         */
        WLPKTTAG(pkt)->flags3 &= ~WLF3_SUPR;

        /* Return FALSE if supr_psq is full */
        if (scb_psinfo->suppressed_pkts >= spktq_max(&scb_psinfo->supr_psq[prec])) {
            WL_PS_EX(scb, ("wl%d.%d:%s(): supr_psq full\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
            wlc_psinfo->supr_drop++;
            scb_psinfo->supr_drop++;
            return FALSE;
        }

        /* Bump suppressed count, indicating need for psq normalization */
        scb_psinfo->suppressed_pkts++;
#ifdef WL_PS_STATS
        scb->suprps_cnt++;
#endif /* WL_PS_STATS */

        spktq_enq(&scb_psinfo->supr_psq[prec], pkt);
    } else if (pktq_n_pkts_tot(&scb_psinfo->psq) >= PSQ_PKTQ_LEN_DEFAULT) {
        WL_PS_EX(scb, ("wl%d.%d:%s(): psq length exceeded\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        return FALSE;
    } else if (!wlc_prec_enq(wlc, &scb_psinfo->psq, pkt, prec)) {
        WL_PS_EX(scb, ("wl%d.%d:%s(): wlc_prec_enq() failed\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        return FALSE;
    }
#else /* ! WL_USE_SUPR_PSQ */
    if (!wlc_prec_enq(wlc, &scb_psinfo->psq, pkt, prec)) {
        WL_PS_EX(scb, ("wl%d.%d: %s wlc_prec_enq() failed\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        return FALSE;
    } else {
        /* Else pkt was added to psq. If it's a suppressed pkt, mark the
         * psq as dirty, thus requiring normalization.
         */
        if (WLPKTTAG(pkt)->flags3 & WLF3_SUPR) {
            /* Bump suppressed count, indicating need for psq normalization */
            scb_psinfo->suppressed_pkts++;
#ifdef WL_PS_STATS
            scb->suprps_cnt++;
#endif /* WL_PS_STATS */
        }
    }
#endif /* ! WL_USE_SUPR_PSQ */

    /* increment total count of PS pkts enqueued in WL driver */
#ifdef WL_USE_SUPR_PSQ
    psq_len_diff = pktq_n_pkts_tot(&scb_psinfo->psq) + scb_psinfo->suppressed_pkts - psq_len;
#else
    psq_len_diff = pktq_n_pkts_tot(&scb_psinfo->psq) - psq_len;
#endif /* ! WL_USE_SUPR_PSQ */
    ASSERT(psq_len_diff < 2);
    if (psq_len_diff == 1)
        wlc_psinfo->ps_deferred++;
    else if (psq_len_diff == 0)
        WL_PS_EX(scb, ("wl%d.%d: %s wlc_prec_enq() dropped pkt.\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));

#ifdef WLTDLS
    if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
        if (!scb_psinfo->apsd_usp)
            wlc_tdls_send_pti(wlc->tdls, scb);
        else if (wlc_tdls_in_pti_interval(wlc->tdls, scb)) {
            scb_psinfo->apsd_cnt = wlc_apps_apsd_delv_count(wlc, scb);
            if (scb_psinfo->apsd_cnt)
                wlc_apps_apsd_send(wlc, scb);
            return TRUE;
        }
    }
#endif /* WLTDLS */

    /* Check if the PVB entry needs to be set */
    if (scb->aid && !scb_psinfo->in_pvb)
        wlc_apps_pvb_update(wlc, scb);

#ifdef BCMPCIEDEV
    _wlc_apps_scb_psq_resp(wlc, scb, scb_psinfo);
#endif /* BCMPCIEDEV */
    return (TRUE);
}

/*
 * Move a PS-buffered packet to the txq and send the txq.
 * Returns TRUE if a packet was available to dequeue and send.
 * extra_flags are added to packet flags (for SDU, only to last MPDU)
 */
static bool
wlc_apps_ps_send(wlc_info_t *wlc, struct scb *scb, uint prec_bmp, uint32 extra_flags)
{
    void *pkt = NULL;
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo;
    int prec;
    struct pktq *psq;        /**< multi-priority packet queue */
    bool apsd_end = FALSE;

    ASSERT(wlc);
    ASSERT(scb);

    if (SCB_DEL_IN_PROGRESS(scb)) {
        /* free any pending frames */
        WL_PS_EX(scb, ("wl%d.%d: %s AID %d, prec %x scb is marked for deletion, "
            " freeing all pending PS packets.\n",
            wlc->pub->unit,    WLC_BSSCFG_IDX(scb->bsscfg), __FUNCTION__,
            SCB_AID(scb), prec_bmp));
        wlc_apps_ps_flush(wlc, scb);
        return FALSE;
    }

    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);
    psq = &scb_psinfo->psq;

#ifdef HNDPQP
    /* Attempt PQP PGI of 1 pdu from highest prec before dequeuing.
     * TODO: The fragmented packets must be sent together.
     */
    if (!PQP_PGI_PS_TRANS_OFF_ISSET(scb_psinfo) && !pktq_mpqp_pgi(psq, prec_bmp)) {
        /* If PQP PGI wait for resource then set flag to resume the ps process.
         * TODO: handle the previous PGI ps trans if it is not completed.
         */
        PQP_PGI_PS_TRANS_PEND_SET(scb_psinfo, PQP_PGI_PS_SEND, 0, extra_flags);
        return TRUE;
    }
#endif /* HNDPQP */

    if (SCB_MARKED_DEL(scb)) {
        while ((pkt = pktq_mdeq(psq, prec_bmp, &prec)) != NULL) {
            if ((WLPKTTAG(pkt)->flags & WLF_CTLMGMT) == 0) {
                WL_PS_EX(scb, ("wl%d.%d: %s AID %d, prec %x dropped data pkt %p\n",
                    wlc->pub->unit,    WLC_BSSCFG_IDX(scb->bsscfg),
                    __FUNCTION__, SCB_AID(scb), prec_bmp, pkt));
                if (extra_flags & WLF_APSD)
                    apsd_end = TRUE;

                PKTFREE(wlc->osh, pkt, TRUE);
                /* Decrement the global ps pkt cnt */
                if (!SCB_ISMULTI(scb)) {
                    wlc_psinfo->ps_deferred--;
                }
            } else {
                break;
            }
        }
    } else {
        pkt = pktq_mdeq(psq, prec_bmp, &prec);
    }

    /* Dequeue the packet with highest precedence out of a given set of precedences */
    if (!pkt) {
        WL_TRACE(("wl%d.%d: %s AID %d, prec %x no traffic\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(scb->bsscfg), __FUNCTION__, SCB_AID(scb), prec_bmp));
        return FALSE;        /* no traffic to send */
    }

    /* Decrement the global ps pkt cnt */
    if (!SCB_ISMULTI(scb)) {
        wlc_psinfo->ps_deferred--;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s AID %d, prec %x dequed pkt %p\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(scb->bsscfg), __FUNCTION__, SCB_AID(scb), prec_bmp, pkt));
    /*
     * If it's the first MPDU in a series of suppressed MPDUs that make up an SDU,
     * enqueue all of them together before calling wlc_send_q.
     */
    /*
     * It's possible that hardware resources may be available for
     * one fragment but not for another (momentarily).
     */
    if (WLPKTTAG(pkt)->flags & WLF_TXHDR) {
        struct dot11_header *h;
        uint tsoHdrSize = 0;
        void *next_pkt;
        uint seq_num, next_seq_num;
        bool control;

#ifdef WLTOEHW
        tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)PKTDATA(wlc->osh, pkt));
#endif
        h = (struct dot11_header *)
            (PKTDATA(wlc->osh, pkt) + tsoHdrSize + D11_TXH_LEN_EX(wlc));
        control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

        /* Control frames does not have seq field; directly queue
         * them.
         */
        if (!control) {
            seq_num = ltoh16(h->seq) >> SEQNUM_SHIFT;

            while ((next_pkt = pktqprec_peek(psq, prec)) != NULL) {
                /* Stop if different SDU */
                if (!(WLPKTTAG(next_pkt)->flags & WLF_TXHDR))
                    break;

                /* Stop if different sequence number */
#ifdef WLTOEHW
                tsoHdrSize = WLC_TSO_HDR_LEN(wlc,
                        (d11ac_tso_t*)PKTDATA(wlc->osh, next_pkt));
#endif
                h = (struct dot11_header *) (PKTDATA(wlc->osh, next_pkt) +
                    tsoHdrSize + D11_TXH_LEN_EX(wlc));
                control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

                /* stop if different ft; control frames does
                 * not have sequence control.
                 */
                if (control)
                    break;

                if (AMPDU_AQM_ENAB(wlc->pub)) {
                    next_seq_num = ltoh16(h->seq) >> SEQNUM_SHIFT;
                    if (next_seq_num != seq_num)
                        break;
                }

                /* Enqueue the PS-Poll response at higher precedence level */
                if (!wlc_apps_ps_enq_resp(wlc, scb, pkt,
                    WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))) &&
                    (extra_flags & WLF_APSD)) {
                    apsd_end = TRUE;
                }

                /* Dequeue the peeked packet */
                pkt = pktq_pdeq(psq, prec);
                ASSERT(pkt == next_pkt);

                /* Decrement the global ps pkt cnt */
                if (!SCB_ISMULTI(scb))
                    wlc_psinfo->ps_deferred--;
            }
        }
    }

    /* Set additional flags on SDU or on final MPDU */
    WLPKTTAG(pkt)->flags |= extra_flags;

    /* Enqueue the packet at higher precedence level */
    if (!wlc_apps_ps_enq_resp(wlc, scb, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))) || apsd_end)
        wlc_apps_apsd_usp_end(wlc, scb);

    /* Send to hardware (latency for first APSD-delivered frame is especially important) */
    wlc_send_q(wlc, SCB_WLCIFP(scb)->qi);

#ifdef PROP_TXSTATUS
    if (extra_flags & WLF_APSD)
        scb_psinfo->apsd_tx_pending = TRUE;
#endif

    return TRUE;
}

static bool
wlc_apps_ps_enq_resp(wlc_info_t *wlc, struct scb *scb, void *pkt, int prec)
{
    wlc_txq_info_t *qi = SCB_WLCIFP(scb)->qi;

    /* register WLF2_PCB2_PSP_RSP for pkt */
    WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);

    /* Ensure the pkt marker (used for ageing) is cleared */
    WLPKTTAG(pkt)->flags &= ~WLF_PSMARK;

    WL_PS_EX(scb, ("wl%d.%d: %s %p supr %d apsd %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
        __FUNCTION__, OSL_OBFUSCATE_BUF(pkt), (WLPKTTAG(pkt)->flags & WLF_TXHDR) ? 1 : 0,
        (WLPKTTAG(pkt)->flags & WLF_APSD) ? 1 : 0));

    /* Enqueue in order of precedence */
    if (!cpktq_prec_enq(wlc, &qi->cpktq, pkt, prec, FALSE)) {
        WL_ERROR(("wl%d.%d: %s txq full, frame discarded\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        PKTFREE(wlc->osh, pkt, TRUE);
        return FALSE;
    }

    return TRUE;
}

void
wlc_apps_set_listen_prd(wlc_info_t *wlc, struct scb *scb, uint16 listen)
{
    struct apps_scb_psinfo *scb_psinfo;
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo != NULL)
        scb_psinfo->listen = listen;
}

uint16
wlc_apps_get_listen_prd(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo != NULL)
        return scb_psinfo->listen;
    return 0;
}

static bool
wlc_apps_psq_ageing_needed(wlc_info_t *wlc, struct scb *scb)
{
    wlc_bss_info_t *current_bss = scb->bsscfg->current_bss;
    uint16 interval;
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    /* Using scb->listen + 1 sec for ageing to avoid packet drop.
     * In WMM-PS:Test Case 4.10(M.V) which is legacy mixed with wmmps.
     * buffered frame will be dropped because ageing occurs.
     */
    interval = scb_psinfo->listen + (1000/current_bss->beacon_period);

#ifdef WLWNM_AP
    if (WLWNM_ENAB(wlc->pub)) {
        uint32 wnm_scbcap = wlc_wnm_get_scbcap(wlc, scb);
        int sleep_interval = wlc_wnm_scb_sm_interval(wlc, scb);

        if (SCB_WNM_SLEEP(wnm_scbcap) && sleep_interval) {
            interval = MAX((current_bss->dtim_period * sleep_interval), interval);
        }
    }
#endif /* WLWNM_AP */

    return (scb_psinfo->tbtt >= interval);
}

/* Reclaim as many PS pkts as possible
 *    Reclaim from all STAs with pending traffic.
 */
void
wlc_apps_psq_ageing(wlc_info_t *wlc)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    struct scb_iter scbiter;
    struct scb *tscb;

    if (wlc_psinfo->ps_nodes_all == 0) {
        return; /* No one in PS */
    }

    FOREACHSCB(wlc->scbstate, &scbiter, tscb) {
        if (!tscb->permanent && SCB_PS(tscb) && wlc_apps_psq_ageing_needed(wlc, tscb)) {
            scb_psinfo = SCB_PSINFO(wlc_psinfo, tscb);
            scb_psinfo->tbtt = 0;
            /* Initiate an ageing event per listen interval */
            if (!pktq_empty(&scb_psinfo->psq))
                wlc_apps_ps_timedout(wlc, tscb);
        }
    }
}

/**
 * Context structure used by wlc_apps_ps_timeout_filter() while filtering a ps pktq
 */
struct wlc_apps_ps_timeout_filter_info {
    uint              count;    /**< total num packets deleted */
};

/**
 * Pktq filter function to age-out pkts on an SCB psq.
 */
static pktq_filter_result_t
wlc_apps_ps_timeout_filter(void* ctx, void* pkt)
{
    struct wlc_apps_ps_timeout_filter_info *info;
    pktq_filter_result_t ret;

    info = (struct wlc_apps_ps_timeout_filter_info *)ctx;

    /* If not marked just move on */
    if ((WLPKTTAG(pkt)->flags & WLF_PSMARK) == 0) {
        WLPKTTAG(pkt)->flags |= WLF_PSMARK;
        ret = PKT_FILTER_NOACTION;
    } else {
        info->count++;
        ret = PKT_FILTER_DELETE;
    }

    return ret;
}

/* check if we should age pkts or not */
static void
wlc_apps_ps_timedout(wlc_info_t *wlc, struct scb *scb)
{
    struct ether_addr ea;
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct pktq *psq;        /**< multi-priority packet queue */
    wlc_bsscfg_t *bsscfg;
    struct wlc_apps_ps_timeout_filter_info info;

    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    psq = &scb_psinfo->psq;
    ASSERT(!pktq_empty(psq));

    /* save ea and bsscfg before call pkt flush */
    ea = scb->ea;
    bsscfg = SCB_BSSCFG(scb);

    /* init the state for the pktq filter */
    info.count = 0;
    BCM_REFERENCE(psq);

#ifdef HNDPQP
    wlc_apps_scb_pqp_pktq_filter(wlc, scb, (void *)&info);
#else  /* !HNDPQP */
    /* Age out all pkts that have been through one previous listen interval */
    wlc_txq_pktq_filter(wlc, psq, wlc_apps_ps_timeout_filter, &info);
#endif /* !HNDPQP */

    WL_PS_EX(scb, ("wl%d.%d: %s timing out %d packet for "MACF" AID %d, %d remain\n",
        wlc->pub->unit,
        WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, info.count, ETHERP_TO_MACF(&ea),
        SCB_AID(scb), pktq_n_pkts_tot(psq)));

    /* Decrement the global ps pkt cnt */
    if (!SCB_ISMULTI(scb)) {
        ASSERT(wlc_psinfo->ps_deferred >= info.count);
        wlc_psinfo->ps_deferred -= info.count;
    }
    wlc_psinfo->ps_aged += info.count;

    /* callback may have freed scb, exit early if so */
    if (wlc_scbfind(wlc, bsscfg, &ea) == NULL) {
        WL_PS_EX(scb, ("wl%d.%d: %s exiting, scb for "MACF" was freed after last packet"
            " timeout\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__, ETHERP_TO_MACF(&ea)));
        return;
    }

    /* update the beacon PVB, but only if the SCB was not deleted */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }
}

/* Provides how many packets can be 'released' to the PS queue.
 * Decision factors include:
 *   - PSQ avail len
 *   - Deferred PS pkt flow control watermark
 *   - #packets already pending in SCB PS queue
 */
uint32
wlc_apps_release_count(wlc_info_t *wlc, struct scb *scb, int prec)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq *q;        /**< multi-priority packet queue */
    uint32 psq_len;
    uint32 release_cnt;

    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    q = &scb_psinfo->psq;

    psq_len = pktq_n_pkts_tot(q);

    /* PS queue avail */
    release_cnt = MIN(pktq_avail(q), pktqprec_avail_pkts(q, prec));

    /* Deferred PS pkt flow control */
#ifdef PROP_TXSTATUS
    if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc))
#endif /* PROP_TXSTATUS */
    {
        uint32 deferred_cnt;

        /* Limit packet enq to flow control watermark */
        if (wlc_psinfo->ps_deferred < wlc_psinfo->psq_pkts_hi) {
            deferred_cnt = wlc_psinfo->psq_pkts_hi - wlc_psinfo->ps_deferred;
        } else {
            deferred_cnt = 0;
        }

        /* Release mininum allowed PS pkts per scb */
        if (deferred_cnt < wlc_psinfo->psq_pkts_lo) {
            deferred_cnt = wlc_psinfo->psq_pkts_lo > psq_len ?
                wlc_psinfo->psq_pkts_lo - psq_len : deferred_cnt;
        }

        release_cnt = MIN(release_cnt, deferred_cnt);
    }

    return release_cnt;
}

/* _wlc_apps_ps_enq()
 *
 * Try to PS enq a pkt, return false if we could not
 *
 * _wlc_apps_ps_enq() Called from:
 *    wlc_apps_ps_enq() TxMod
 *    wlc_apps_move_to_psq()
 *        wlc_apps_ampdu_txq_to_psq() --| from wlc_apps_scb_ps_on()
 *        wlc_apps_txq_to_psq() --------| from wlc_apps_scb_ps_on()
 * Implements TDLS PRI response handling to bypass PSQ buffering
 */
static void
_wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec)
{
    wlc_info_t *wlc = (wlc_info_t *)ctx;
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

    ASSERT(!SCB_ISMULTI(scb));
    ASSERT(!PKTISCHAINED(pkt));

    /* in case of PS mode */
    if ((WLPKTTAG(pkt)->flags & WLF_8021X)) {
        prec = WLC_PRIO_TO_HI_PREC(MAXPRIO);
    }

#ifdef WLTDLS
    /* for TDLS PTI resp, don't enq to PSQ, send right away */
    if (BSS_TDLS_ENAB(wlc, SCB_BSSCFG(scb)) && SCB_PS(scb) &&
        (WLPKTTAG(pkt)->flags & WLF_PSDONTQ)) {
        WL_PS_EX(scb, ("wl%d.%d: %s skip enq to PSQ\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        SCB_TX_NEXT(TXMOD_APPS, scb, pkt, prec);
        return;
    }
#endif /* WLTDLS */

    if (!wlc_apps_psq(wlc, pkt, prec)) {
        struct apps_scb_psinfo *scb_psinfo;
        wlc_psinfo->ps_discard++;
        ASSERT(scb);
        scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
        ASSERT(scb_psinfo);
        scb_psinfo->ps_discard++;
#ifdef PROP_TXSTATUS
        if (PROP_TXSTATUS_ENAB(wlc->pub)) {
            /* wlc decided to discard the packet, host should hold onto it,
             * this is a "suppress by wl" instead of D11
             */
            WL_PS_EX(scb, ("wl%d.%d: %s ps pkt %p suppressed AID %d\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, pkt, SCB_AID(scb)));
            wlc_suppress_sync_fsm(wlc, scb, pkt, TRUE);
            wlc_process_wlhdr_txstatus(wlc, WLFC_CTL_PKTFLAG_WLSUPPRESS, pkt, FALSE);
        } else
#endif /* PROP_TXSTATUS */
        {
            WL_PS_EX(scb, ("wl%d.%d: %s ps pkt %p discarded AID %d\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, pkt, SCB_AID(scb)));
        }
#ifdef WLTAF
        wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_PREC(prec), pkt,
            TAF_TXPKT_STATUS_SUPPRESSED_FREE);
#endif
        PKTFREE(wlc->osh, pkt, TRUE);
    } else {
        struct apps_scb_psinfo *scb_psinfo;
        scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
        scb_psinfo->ps_queued++;
#ifdef WLTAF
        wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_PREC(prec), pkt,
            TAF_TXPKT_STATUS_PS_QUEUED);
#endif
    }
}

/* PS TxModule enqueue function */
static void
wlc_apps_ps_enq(void *ctx, struct scb *scb, void *pkt, uint prec)
{
    wlc_info_t *wlc = (wlc_info_t *)ctx;

    _wlc_apps_ps_enq(wlc, scb, pkt, prec);

#ifdef HNDPQP
    /* Page out normal PS queue */
    wlc_apps_scb_psq_prec_pqp_pgo(wlc, scb, prec);
#endif /* HNDPQP */
}

/* Try to ps enq the pkts on the txq */
static void
wlc_apps_txq_to_psq(wlc_info_t *wlc, struct scb *scb)
{
    wlc_apps_move_cpktq_to_psq(wlc, &(SCB_WLCIFP(scb)->qi->cpktq), scb);
}

#ifdef PROP_TXSTATUS
#ifdef WLNAR
/* Try to ps enq the pkts on narq */
static void
wlc_apps_nar_txq_to_psq(wlc_info_t *wlc, struct scb *scb)
{
    struct pktq *txq = wlc_nar_txq(wlc->nar_handle, scb);
    if (txq) {
        wlc_apps_move_to_psq(wlc, txq, scb);
    }
}
#endif /* WLNAR */

#ifdef WLAMPDU
/* This causes problems for PSPRETEND */
/* ps enq pkts on ampduq */
static void
wlc_apps_ampdu_txq_to_psq(wlc_info_t *wlc, struct scb *scb)
{
    if (AMPDU_ENAB(wlc->pub)) {
        struct pktq *txq = wlc_ampdu_txq(wlc->ampdu_tx, scb);
        if (txq) wlc_apps_move_to_psq(wlc, txq, scb);
    }
}
#endif /* WLAMPDU */

/** @param pktq   Multi-priority packet queue */
static void
wlc_apps_move_to_psq(wlc_info_t *wlc, struct pktq *pktq, struct scb* scb)
{
    void *head_pkt = NULL, *pkt;
    int prec;

#ifdef WLTDLS
    bool q_empty = TRUE;
    apps_wlc_psinfo_t *wlc_psinfo;
    struct apps_scb_psinfo *scb_psinfo;
#endif

    ASSERT(AP_ENAB(wlc->pub) || BSS_TDLS_BUFFER_STA(SCB_BSSCFG(scb)) ||
        BSSCFG_IBSS(SCB_BSSCFG(scb)));

    PKTQ_PREC_ITER(pktq, prec) {
        head_pkt = NULL;
        /* PS enq all the pkts we can */
        while (pktqprec_peek(pktq, prec) != head_pkt) {
            pkt = pktq_pdeq(pktq, prec);
            if (pkt == NULL) {
                /* txq could be emptied in _wlc_apps_ps_enq() */
                WL_ERROR(("WARNING: wl%d: %s NULL pkt\n", wlc->pub->unit,
                    __FUNCTION__));
                break;
            }
            if (scb != WLPKTTAGSCBGET(pkt)) {
                if (!head_pkt)
                    head_pkt = pkt;
                pktq_penq(pktq, prec, pkt);
                continue;
            }
            /* Enqueueing at the same prec may create a remote
             * possibility of suppressed pkts being reordered.
             * Needs to be investigated...
             */
            _wlc_apps_ps_enq(wlc, scb, pkt, prec);

#if defined(WLTDLS)
            if (TDLS_ENAB(wlc->pub) && wlc_tdls_isup(wlc->tdls))
                q_empty = FALSE;
#endif /* defined(WLTDLS) */
        }
    }

#if defined(WLTDLS)
    if (TDLS_ENAB(wlc->pub)) {
        ASSERT(wlc);
        wlc_psinfo = wlc->psinfo;

        ASSERT(scb);
        ASSERT(scb->bsscfg);
        scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
        ASSERT(scb_psinfo);
        if (!q_empty && !scb_psinfo->apsd_usp)
            wlc_tdls_send_pti(wlc->tdls, scb);
    }
#endif /* defined(WLTDLS) */
}
#endif /* PROP_TXSTATUS */

static void
wlc_apps_move_cpktq_to_psq(wlc_info_t *wlc, struct cpktq *cpktq, struct scb* scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    void *head_pkt = NULL, *pkt;
    int prec;
    struct pktq *txq = &cpktq->cq;        /**< multi-priority packet queue */
    struct pktq *psq;
#ifdef WLTDLS
    bool q_empty = TRUE;
#endif

    ASSERT(AP_ENAB(wlc->pub) || BSS_TDLS_BUFFER_STA(SCB_BSSCFG(scb)) ||
        BSSCFG_IBSS(SCB_BSSCFG(scb)));

    ASSERT(wlc);
    ASSERT(scb);
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);
    psq = &scb_psinfo->psq;

    BCM_REFERENCE(scb_psinfo);
    BCM_REFERENCE(psq);

#ifdef HNDPQP
    /* Previous PGI PS transition is not finish.
     * Reset the status and page-out the remain packets in pktq.
     */
    if (PQP_PGI_PS_TRANS(scb_psinfo)) {
        wlc_apps_scb_pqp_reset_ps_trans(wlc, scb);
    }
#endif /* HNDPQP */

    PKTQ_PREC_ITER(txq, prec) {
        head_pkt = NULL;
        /* PS enq all the pkts we can */
        while (pktqprec_peek(txq, prec) != head_pkt) {
            pkt = cpktq_pdeq(cpktq, prec);
            if (pkt == NULL) {
                /* txq could be emptied in _wlc_apps_ps_enq() */
                WL_ERROR(("WARNING: wl%d: %s NULL pkt\n", wlc->pub->unit,
                    __FUNCTION__));
                break;
            }
            if (scb != WLPKTTAGSCBGET(pkt)) {
                if (!head_pkt)
                    head_pkt = pkt;
                cpktq_penq(wlc, cpktq, prec, pkt, FALSE);
                continue;
            }
            /* Enqueueing at the same prec may create a remote
             * possibility of suppressed pkts being reordered.
             * Needs to be investigated...
             */
            _wlc_apps_ps_enq(wlc, scb, pkt, prec);

#if defined(WLTDLS)
            if (TDLS_ENAB(wlc->pub) && wlc_tdls_isup(wlc->tdls))
                q_empty = FALSE;
#endif /* defined(WLTDLS) */
        }

#ifdef HNDPQP
        /* After ps enq the packets from common txq.
         * If there are packets in this pktq and PQP owns this pktq.
         * To make sure the order of the packets,
         * Use prepend to page out normal PS queue.
         */
        if (pktqprec_n_pkts(psq, prec) && pqp_owns(&psq->q[prec])) {
            pktq_prec_pqp_pgo(psq, prec, PQP_PREPEND, scb_psinfo);
        }
#endif /* HNDPQP */
    }

#if defined(WLTDLS)
    if (TDLS_ENAB(wlc->pub)) {
        if (!q_empty && !scb_psinfo->apsd_usp)
            wlc_tdls_send_pti(wlc->tdls, scb);
    }
#endif /* defined(WLTDLS) */
}

#ifdef BCMPCIEDEV
static INLINE void
_wlc_apps_scb_psq_resp(wlc_info_t *wlc, struct scb *scb, struct apps_scb_psinfo *scb_psinfo)
{
    ASSERT(scb_psinfo);

    if (scb_psinfo->twt_active) {
        return;
    }

    /* Check if a previous PS POLL is waiting for a packet */
    if (SCB_PSPOLL_PKT_WAITING(scb_psinfo)) {
        wlc_apps_send_psp_response(wlc, scb, 0);
    }

    /* Check if waiting for packet from WMM PS trigger */
    if (scb_psinfo->apsd_hpkt_timer_on) {
#ifdef WLAMSDU_TX
        uint16 max_sf_frames = wlc_amsdu_scb_max_sframes(wlc->ami, scb);
#else
        uint16 max_sf_frames = 1;
#endif
        wl_del_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer);

        if (!scb_psinfo->apsd_tx_pending && scb_psinfo->apsd_usp &&
            scb_psinfo->apsd_cnt) {

            /* Respond to WMM PS trigger */
            wlc_apps_apsd_send(wlc, scb);

            if (scb_psinfo->apsd_cnt > 1 &&
                wlc_apps_apsd_delv_count(wlc, scb) <=
                (WLC_APSD_DELV_CNT_LOW_WATERMARK * max_sf_frames)) {
                ac_bitmap_t ac_to_request;
                ac_to_request = scb->apsd.ac_delv & AC_BITMAP_ALL;

                /* Request packets from host flow ring */
                scb_psinfo->apsd_v2r_in_transit +=
                    wlc_sqs_psmode_pull_packets(wlc, scb,
                    WLC_ACBITMAP_TO_TIDBITMAP(ac_to_request),
                    (WLC_APSD_DELV_CNT_HIGH_WATERMARK * max_sf_frames));

                wl_add_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer,
                    WLC_PS_APSD_HPKT_TIME, FALSE);
            } else
                scb_psinfo->apsd_hpkt_timer_on = FALSE;
        } else
            scb_psinfo->apsd_hpkt_timer_on = FALSE;

    }
}

static void
wlc_apps_scb_apsd_dec_v2r_in_transit(wlc_info_t *wlc, struct scb *scb, void *pkt, int prec)
{
    struct apps_scb_psinfo *scb_psinfo;
    int v2r_pkt_cnt;
    uint32 precbitmap;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    if (scb_psinfo->apsd_hpkt_timer_on && (scb_psinfo->apsd_v2r_in_transit > 0)) {
        /* Check bitmap and pkttag flag to make sure this packet is from pciedev layer.
         * And then decrease the counter of apsd_v2r_in_transit.
         */
        precbitmap = WLC_ACBITMAP_TO_PRECBITMAP(scb->apsd.ac_delv);
        if (isset(&precbitmap, prec) && !(WLPKTTAG(pkt)->flags & WLF_TXHDR)) {
            if (WLPKTTAG_AMSDU(pkt)) {
                v2r_pkt_cnt = wlc_amsdu_msdu_cnt(wlc->osh, pkt);
            } else {
                v2r_pkt_cnt = 1;
            }
            scb_psinfo->apsd_v2r_in_transit -= v2r_pkt_cnt;
            ASSERT(scb_psinfo->apsd_v2r_in_transit >= 0);
        }
    }
}
#endif /* BCMPCIEDEV */

/* Set/clear PVB entry according to current state of power save queues */
void
wlc_apps_pvb_update(wlc_info_t *wlc, struct scb *scb)
{
    uint16 aid;
    struct apps_scb_psinfo *scb_psinfo;
    int ps_count;
    int pktq_total, pktq_ndelv_count;
    apps_bss_info_t *bss_info;

    ASSERT(wlc);
    ASSERT(scb);
    ASSERT(scb->bsscfg);
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL) {
        return;
    }

    bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, scb->bsscfg);
    if (bss_info == NULL) {
        return;
    }

    ps_count = 0;
    if ((SCB_PS(scb) || SCB_TWTPS(scb)) && !SCB_DEL_IN_PROGRESS(scb)) {
        /* get available packet count for the given flow */
        pktq_total = wlc_apps_scb_pktq_tot(wlc, scb);
        pktq_ndelv_count = wlc_apps_apsd_ndelv_count(wlc, scb);

#ifdef BCMPCIEDEV
        /* Virtual packets */
        pktq_total += wlc_apps_sqs_vpkt_count(wlc, scb);
        /* If there are no packets in psq and virtual packets is zero.
         * Check apsd_v2r_in_transit for any packets which are not in psq.
         */
        if (pktq_total == 0)
            pktq_total = scb_psinfo->apsd_v2r_in_transit;
        pktq_ndelv_count += wlc_apps_apsd_ndelv_vpkt_count(wlc, scb);
#endif
        /* WMM/APSD 3.6.1.4: if no ACs are delivery-enabled (legacy), or all ACs are
         * delivery-enabled (special case), the PVB should indicate if any packet is
         * buffered.  Otherwise, the PVB should indicate if any packets are buffered
         * for non-delivery-enabled ACs only.
         */
        ps_count = ((scb->apsd.ac_delv == AC_BITMAP_NONE ||
            scb->apsd.ac_delv == AC_BITMAP_ALL) ?
            pktq_total : pktq_ndelv_count);

        /* When PSPretend is in probing mode, it sends probes (null data) to STA where
         * APPS is bypassed (PS_DONTQ). However it does expect that TIM bit is set. This
         * is normally true, since a frame got suppressed (which triggered PSPretend), but
         * this frame can get flushed, so the count is upped here when PSPretend is
         * probing to make sure TIM is set for this destination.
         */
        if (SCB_PS_PRETEND_PROBING(scb)) {
            ps_count++;
        }
    }

    aid = SCB_AID(scb);
    ASSERT(aid != 0);
    ASSERT((aid / NBBY) < ARRAYSIZE(bss_info->pvb));

    if (ps_count > 0) {
        if (scb_psinfo->in_pvb)
            return;
        WL_PS_EX(scb, ("wl%d.%d: %s setting AID %d scb:%p ps_count %d psqlen %d\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, aid,
            scb, ps_count, pktq_total));
        /* set the bit in the pvb */
        setbit(bss_info->pvb, aid);

        /* reset the aid range */
        if ((aid < bss_info->aid_lo) || !bss_info->aid_lo) {
            bss_info->aid_lo = aid;
        }
        if (aid > bss_info->aid_hi) {
            bss_info->aid_hi = aid;
        }

        scb_psinfo->in_pvb = TRUE;
        if (wlc->clk) {
            scb_psinfo->last_in_pvb_tsf = wlc_lifetime_now(wlc);
        }
    } else {
        if (!scb_psinfo->in_pvb) {
            return;
        }

        WL_PS_EX(scb, ("wl%d.%d: %s clearing AID %d scb:%p\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, aid, scb));
        /* clear the bit in the pvb */
        clrbit(bss_info->pvb, aid);

        /* reset the aid range */
        if (aid == bss_info->aid_hi) {
            /* find the next lowest aid value with PS pkts pending */
            for (aid = aid - 1; aid; aid--) {
                if (isset(bss_info->pvb, aid)) {
                    bss_info->aid_hi = aid;
                    break;
                }
            }
            /* no STAs with pending traffic ? */
            if (aid == 0) {
                bss_info->aid_hi = bss_info->aid_lo = 0;
            }
        } else if (aid == bss_info->aid_lo) {
            uint16 max_aid = wlc_ap_aid_max(wlc->ap);
            /* find the next highest aid value with PS pkts pending */
            for (aid = aid + 1; aid < max_aid; aid++) {
                if (isset(bss_info->pvb, aid)) {
                    bss_info->aid_lo = aid;
                    break;
                }
            }
            ASSERT(aid != max_aid);
        }

        scb_psinfo->in_pvb = FALSE;
        scb_psinfo->last_in_pvb_tsf = 0;
    }

    /* Update the PVB in the bcn template */
    WL_PS_EX(scb, ("wl%d.%d: %s -> wlc_bss_update_beacon\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
    WL_APSTA_BCN(("wl%d: %s -> wlc_bss_update_beacon\n", wlc->pub->unit, __FUNCTION__));

        wlc_bss_update_beacon(wlc, scb->bsscfg, TRUE);

}

/* Increment the TBTT count for PS SCBs in a particular bsscfg */
static void
wlc_bss_apps_tbtt_update(wlc_bsscfg_t *cfg)
{
    wlc_info_t *wlc = cfg->wlc;
    struct apps_scb_psinfo *scb_psinfo;
    struct scb *scb;
    struct scb_iter scbiter;

    ASSERT(cfg != NULL);
    ASSERT(BSSCFG_AP(cfg));

    /* If touching all the PS scbs is too inefficient then we
     * can maintain a single count and only create an ageing event
     * using the longest listen interval requested by a STA
     */

    /* increment the tbtt count on all PS scbs */
    /* APSTA: For APSTA, don't bother aging AP SCBs */
    FOREACH_BSS_SCB(wlc->scbstate, &scbiter, cfg, scb) {
        if (!scb->permanent && SCB_PS(scb)) {
            scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
            if (scb_psinfo->tbtt < 0xFFFF) /* do not wrap around */
                scb_psinfo->tbtt++;
        }
    }
}

/* Increment the TBTT count for all PS SCBs */
void
wlc_apps_tbtt_update(wlc_info_t *wlc)
{
    int idx;
    wlc_bsscfg_t *cfg;

    /* If touching all the PS scbs multiple times is too inefficient
     * then we can restore the old code and have all scbs updated in one pass.
     */

    FOREACH_UP_AP(wlc, idx, cfg)
            wlc_bss_apps_tbtt_update(cfg);
}

/* return the count of PS buffered pkts for an SCB */
int
wlc_apps_psq_len(wlc_info_t *wlc, struct scb *scb)
{
    int pktq_total;
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    pktq_total = pktq_n_pkts_tot(&scb_psinfo->psq);

    return pktq_total;
}

/* return the count of PS buffered pkts for an SCB which are delivery-enabled ACs. */
int
wlc_apps_psq_delv_count(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint32 precbitmap;
    int delv_count;

    if (scb->apsd.ac_delv == AC_BITMAP_NONE)
        return 0;

    precbitmap = WLC_ACBITMAP_TO_PRECBITMAP(scb->apsd.ac_delv);

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    delv_count = APPS_PKTQ_MLEN(&scb_psinfo->psq, precbitmap);

    return delv_count;
}

/* return the count of PS buffered pkts for an SCB which are non-delivery-enabled ACs. */
int
wlc_apps_psq_ndelv_count(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    ac_bitmap_t ac_non_delv;
    uint32 precbitmap;
    int count;

    if (scb->apsd.ac_delv == AC_BITMAP_ALL)
        return 0;

    ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;
    precbitmap = WLC_ACBITMAP_TO_PRECBITMAP(ac_non_delv);

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    count = APPS_PKTQ_MLEN(&scb_psinfo->psq, precbitmap);

    return count;
}

/* called from bmac when a PS state switch is detected from the transmitter.
 * On PS ON switch, directly call wlc_apps_scb_ps_on();
 *  On PS OFF, check if there are tx packets pending. If so, make a PS OFF reservation
 *  and wait for the drain. Otherwise, switch to PS OFF.
 *  Sends a message to the bmac pmq manager to signal that we detected this switch.
 *  PMQ manager will delete entries when switch states are in sync and the queue is drained.
 *  return 1 if a switch occured. This allows the caller to invalidate
 *  the header cache.
 */
void BCMFASTPATH
wlc_apps_process_ps_switch(wlc_info_t *wlc, struct scb *scb, uint8 ps_on)
{
    struct apps_scb_psinfo *scb_psinfo;

    /* only process ps transitions for associated sta's, IBSS bsscfg and WDS peers */
    if (!(SCB_ASSOCIATED(scb) || SCB_IS_IBSS_PEER(scb) || SCB_WDS(scb))) {
        /* send notification to bmac that this entry doesn't exist up here. */
        uint8 ps = PS_SWITCH_STA_REMOVED;

        if ((wlc->block_datafifo & DATA_BLOCK_PS) ||
#if defined(WL_PS_SCB_TXFIFO_BLK)
            wlc->ps_scb_txfifo_blk ||
#endif /* WL_PS_SCB_TXFIFO_BLK */
            FALSE) {
            ps |= PS_SWITCH_OFF;
        } else {
            ps |= PS_SWITCH_FIFO_FLUSHED;
        }

        wlc_pmq_process_ps_switch(wlc, scb, ps);
        return;
    }

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (scb_psinfo == NULL)
        return;

    if (ps_on) {
        if ((ps_on & PS_SWITCH_PMQ_SUPPR_PKT)) {
            WL_PS_EX(scb, ("wl%d.%d: %s Req by suppr pkt! "MACF"\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, ETHERP_TO_MACF(&scb->ea)));
            scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_BLOCKED;
        } else if ((ps_on & (PS_SWITCH_PMQ_ENTRY | PS_SWITCH_OMI))) {
            if (!SCB_PS(scb)) {
                WL_PS_EX(scb, ("wl%d.%d: %s Actual PMQ entry (0x%x) processing "
                    MACF"\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, ps_on, ETHERP_TO_MACF(&scb->ea)));
            }
            /* This PS ON request is from actual PMQ entry addition. */
            scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_BLOCKED;
        }
        if (!SCB_PS(scb)) {
#ifdef PSPRETEND
            /* reset pretend status */
            scb->ps_pretend &= ~PS_PRETEND_ON;
            if (PSPRETEND_ENAB(wlc->pub) &&
                (ps_on & PS_SWITCH_PMQ_PSPRETEND) && !SCB_ISMULTI(scb)) {
                wlc_pspretend_on(wlc->pps_info, scb, PS_PRETEND_ACTIVE_PMQ);
            }
            else
#endif /* PSPRETEND */
            {
                wlc_apps_scb_ps_on(wlc, scb);
            }

            WL_PS_EX(scb, ("wl%d.%d: %s "MACF" - PS %s, pretend mode off\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
                ETHERP_TO_MACF(&scb->ea), SCB_PS(scb) ? "on":"off"));
        }
#ifdef PSPRETEND
        else if (SCB_PS_PRETEND(scb) && (ps_on & PS_SWITCH_PMQ_PSPRETEND)) {
            WL_PS_EX(scb, ("wl%d.%d: %s "MACF" PS pretend was already active now "
                "with new PMQ PPS entry\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, ETHERP_TO_MACF(&scb->ea)));
            scb->ps_pretend |= PS_PRETEND_ACTIVE_PMQ;
        }
#endif /* PSPRETEND */
        else {
#ifdef PSPRETEND
            if (SCB_PS_PRETEND(scb) &&
                (ps_on & (PS_SWITCH_PMQ_ENTRY | PS_SWITCH_OMI))) {
                if (wlc->pps_info != NULL) {
                    wlc_pspretend_scb_ps_off(wlc->pps_info, scb);
                }
            }
#endif /* PSPRETEND */
            /* STA is already in PS, clear PS OFF pending bit only */
            scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_PEND;
        }
    } else if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_BLOCKED)) {
        WL_PS_EX(scb, ("wl%d.%d: %s "MACF" PS off attempt is blocked by WAITPMQ\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
            __FUNCTION__, ETHERP_TO_MACF(&scb->ea)));
    } else if (scb_psinfo->ps_requester) {
        WL_PS_EX(scb, ("wl%d.%d: %s "MACF" PS off attempt is blocked by another "
            "source. 0x%x\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            ETHERP_TO_MACF(&scb->ea), scb_psinfo->ps_requester));
    } else {

#if defined(WL_PS_SCB_TXFIFO_BLK)
        if (SCB_PS_TXFIFO_BLK(scb))
#else /* ! WL_PS_SCB_TXFIFO_BLK */
        if (wlc->block_datafifo & DATA_BLOCK_PS)
#endif /* ! WL_PS_SCB_TXFIFO_BLK */
        {
            /* Prevent ON -> OFF transitions while data fifo is blocked.
             * We need to finish flushing HW and reque'ing before we
             * can allow the STA to come out of PS.
             */
            WL_PS_EX(scb, ("wl%d.%d: %s "MACF" DATA_BLOCK_PS %d pkts pending%s\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
                ETHERP_TO_MACF(&scb->ea), TXPKTPENDTOT(wlc),
                SCB_PS_PRETEND(scb) ? " (ps pretend active)" : ""));
            scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
        }
#ifdef PSPRETEND
        else if (SCB_PS_PRETEND_BLOCKED(scb) &&
            !(scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_PEND)) {
            /* Prevent ON -> OFF transitions if we were expecting to have
             * seen a PMQ entry for ps pretend and we have not had it yet.
             * This is to ensure that when that entry does come later, it
             * does not cause us to enter ps pretend mode when that condition
             * should have been cleared
             */
            WL_PS_EX(scb, ("wl%d.%d: %s "MACF" ps pretend pending off waiting for "
                "the PPS ON PMQ entry\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, ETHERP_TO_MACF(&scb->ea)));
            scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
        }
#endif /* PSPRETEND */
        else if (SCB_PS(scb))  {
#ifdef PSPRETEND
            if (SCB_PS_PRETEND_BLOCKED(scb)) {
                WL_PS_EX(scb, ("wl%d.%d: %s "MACF" ps pretend pending off waiting "
                    "for the PPS ON PMQ, but received PPS OFF PMQ more than "
                    "once. Consider PPS ON PMQ as lost\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, ETHERP_TO_MACF(&scb->ea)));
            }
#endif /* PSPRETEND */
            /* Once TWT is active, ignore late PMQ off events. PM is forced to PS,
             * and it should stay there. TWT will take control of PM.
             */
            if (!scb_psinfo->twt_active) {
                wlc_apps_scb_ps_off(wlc, scb, FALSE);
            } else {
                WL_TWT(("wl%d.%d: %s: Block PS OFF "MACF" (%d/%d/%d/%d/%d)\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, ETHER_TO_MACF(scb->ea),
                    scb_psinfo->twt_active, SCB_PS(scb),
                    SCB_TWTPS(scb), scb_psinfo->twt_wait4drain_enter,
                    SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)));
            }
        }
    }

    /* When per station PS block is enabled, next station entry in PMQ might have packets
     * pending to be suppressed so clearing PMQ in wlc_pmq_processpmq() after reading all
     * entries in PMQ fifo
     */
    /* indicate fifo state  */
#if defined(WL_PS_SCB_TXFIFO_BLK)
    if (!wlc->ps_scb_txfifo_blk) {
#else
    if (!(wlc->block_datafifo & DATA_BLOCK_PS)) {
#endif /* WL_PS_SCB_TXFIFO_BLK */
        ps_on |= PS_SWITCH_FIFO_FLUSHED;
    }

    wlc_pmq_process_ps_switch(wlc, scb, ps_on);
    return;

}

/* wlc_apps_pspoll_resp_prepare()
 * Do some final pkt touch up before DMA ring for PS delivered pkts.
 * Also track pspoll response state (pkt callback and more_data signaled)
 */
void
wlc_apps_pspoll_resp_prepare(wlc_info_t *wlc, struct scb *scb,
                             void *pkt, struct dot11_header *h, bool last_frag)
{
    struct apps_scb_psinfo *scb_psinfo;

    ASSERT(scb);
    ASSERT(SCB_PS(scb));

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    /*
     * FC_MOREDATA is set for every response packet being sent while STA is in PS.
     * This forces STA to send just one more PS-Poll.  If by that time we actually
     * have more data, it'll be sent, else a Null data frame without FC_MOREDATA will
     * be sent.  This technique often improves TCP/IP performance.  The last NULL Data
     * frame is sent with the WLF_PSDONTQ flag.
     */

    h->fc |= htol16(FC_MOREDATA);

    /* Register pkt callback for PS-Poll response */
    if (last_frag && !SCB_ISMULTI(scb)) {
        WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);
        scb_psinfo->psp_pending = TRUE;
    }

    scb_psinfo->psp_flags |= PS_MORE_DATA;
}

/* Fix PDU that is being sent as a PS-Poll response or APSD delivery frame. */
void
wlc_apps_ps_prep_mpdu(wlc_info_t *wlc, void *pkt)
{
    bool last_frag;
    struct dot11_header *h;
    uint16    macCtlLow, frameid;
    struct scb *scb;
    wlc_bsscfg_t *bsscfg;
    wlc_key_info_t key_info;
    wlc_txh_info_t txh_info;

    scb = WLPKTTAGSCBGET(pkt);

    wlc_get_txh_info(wlc, pkt, &txh_info);
    h = txh_info.d11HdrPtr;

    WL_PS_EX(scb, ("wl%d.%d: %s pkt %p flags 0x%x flags2 %x fc 0x%x\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        OSL_OBFUSCATE_BUF(pkt), WLPKTTAG(pkt)->flags, WLPKTTAG(pkt)->flags2, h->fc));

    /*
     * Set the IGNOREPMQ bit.
     *
     * PS bcast/mcast pkts have following differences from ucast:
     *    1. use the BCMC fifo
     *    2. FC_MOREDATA is set by ucode (except for the kludge)
     *    3. Don't set IGNOREPMQ bit as ucode ignores PMQ when draining
     *       during DTIM, and looks at PMQ when draining through
     *       MHF2_TXBCMC_NOW
     */
    if (ETHER_ISMULTI(txh_info.TxFrameRA)) {
        ASSERT(!SCB_WDS(scb));

        /* Kludge required from wlc_dofrag */
        bsscfg = SCB_BSSCFG(scb);

        /* Update the TxFrameID in both the txh_info struct and the packet header */
        if (D11_TXFID_GET_FIFO(wlc, txh_info.TxFrameID) != TX_BCMC_FIFO) {
            frameid = wlc_compute_frameid(wlc, txh_info.TxFrameID, TX_BCMC_FIFO);
            txh_info.TxFrameID = htol16(frameid);
            wlc_set_txh_frameid(wlc, pkt, frameid);
        }

        ASSERT(!SCB_A4_DATA(scb));

        /* APSTA: MUST USE BSS AUTH DUE TO SINGLE BCMC SCB; IS THIS OK? */
        wlc_keymgmt_get_bss_tx_key(wlc->keymgmt, bsscfg, FALSE, &key_info);

        if (!bcmwpa_is_wpa_auth(bsscfg->WPA_auth) || key_info.algo != CRYPTO_ALGO_AES_CCM)
            h->fc |= htol16(FC_MOREDATA);
    }
    else if (!SCB_ISMULTI(scb)) {
        /* There is a hack to send uni-cast P2P_PROBE_RESP frames using bsscfg's
        * mcast scb because of no uni-cast scb is available for bsscfg, we need to exclude
        * such hacked packtes from uni-cast processing.
        */
        last_frag = (ltoh16(h->fc) & FC_MOREFRAG) == 0;
        /* Set IGNOREPMQ bit (otherwise, it may be suppressed again) */
        macCtlLow = ltoh16(txh_info.MacTxControlLow);
        if (D11REV_GE(wlc->pub->corerev, 40)) {
            d11txhdr_t* txh = txh_info.hdrPtr;
            macCtlLow |= D11AC_TXC_IPMQ;
            *D11_TXH_GET_MACLOW_PTR(wlc, txh) = htol16(macCtlLow);
#ifdef PSPRETEND
            if (PSPRETEND_ENAB(wlc->pub)) {
                uint16 macCtlHigh = ltoh16(txh_info.MacTxControlHigh);
                macCtlHigh &= ~D11AC_TXC_PPS;
                *D11_TXH_GET_MACHIGH_PTR(wlc, txh) = htol16(macCtlHigh);
            }
#endif /* PSPRETEND */
        } else {
            d11txh_pre40_t* nonVHTHdr = &(txh_info.hdrPtr->pre40);
            macCtlLow |= TXC_IGNOREPMQ;
            nonVHTHdr->MacTxControlLow = htol16(macCtlLow);
        }

        /*
         * Set FC_MOREDATA and EOSP bit and register callback.  WLF_APSD is set
         * for all APSD delivery frames.  WLF_PSDONTQ is set only for the final
         * Null frame of a series of PS-Poll responses.
         */
        if (WLPKTTAG(pkt)->flags & WLF_APSD)
            wlc_apps_apsd_prepare(wlc, scb, pkt, h, last_frag);
        else if (!(WLPKTTAG(pkt)->flags & WLF_PSDONTQ))
            wlc_apps_pspoll_resp_prepare(wlc, scb, pkt, h, last_frag);
    }
}

/* Packet callback fn for WLF2_PCB2_PSP_RSP
 *
 */
static void
wlc_apps_psp_resp_complete(wlc_info_t *wlc, void *pkt, uint txs)
{
    struct scb *scb;
    struct apps_scb_psinfo *scb_psinfo;
#ifdef WLTAF
    bool taf_in_use = wlc_taf_in_use(wlc->taf_handle);
#endif

    BCM_REFERENCE(txs);

    /* Is this scb still around */
    if ((scb = WLPKTTAGSCBGET(pkt)) == NULL)
        return;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL)
        return;

#ifdef WLTAF
    if (taf_in_use && (wlc_apps_apsd_ndelv_count(wlc, scb) > 0)) {
        /* If psq is empty, release new packets from ampdu or nar */
        if ((wlc_apps_psq_ndelv_count(wlc, scb) == 0) &&
            !wlc_taf_scheduler_blocked(wlc->taf_handle)) {
            /* Trigger a new TAF schedule cycle */
            wlc_taf_schedule(wlc->taf_handle, PKTPRIO(pkt), scb, FALSE);
        }
    }
#endif

    /* clear multiple ps-poll frame protection */
    scb_psinfo->psp_flags &= ~PS_PSP_ONRESP;

    /* Check if the PVB entry needs to be cleared */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }

    if (scb_psinfo->psp_pending) {
        scb_psinfo->psp_pending = FALSE;
        // Obsolete with PS_PSP_ONRESP logic -- no chance for tx below
        if (scb_psinfo->tx_block != 0) {
            WL_PS_EX(scb, ("wl%d.%d: %s tx blocked\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
            return;
        }
        // Obsolete with PS_PSP_ONRESP logic -- prevents PS_PSP_REQ_PEND from being set
        if (scb_psinfo->psp_flags & PS_PSP_REQ_PEND) {
            /* send the next ps pkt if requested */
            scb_psinfo->psp_flags &= ~(PS_MORE_DATA | PS_PSP_REQ_PEND);
            WL_PS_EX(scb, ("wl%d.%d %s send more frame.\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
            if (!wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0)) {
#ifdef WLCFP
                if (CFP_ENAB(wlc->pub) && SCB_AMPDU(scb)) {
                    wlc_cfp_ampdu_ps_send(wlc, scb, ALLPRIO, 0);
                }
#endif /* WLCFP */
            }
        }
    }
}

static int
wlc_apps_send_psp_response_cb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, void *pkt, void *data)
{
    BCM_REFERENCE(wlc);
    BCM_REFERENCE(cfg);
    BCM_REFERENCE(data);

    /* register packet callback */
    WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);
    return BCME_OK;
}

/* wlc_apps_send_psp_response()
 *
 * This function is used in rx path when we get a PSPoll.
 * Also used for proptxstatus when a tx pkt is queued to the driver and
 * SCB_PROPTXTSTATUS_PKTWAITING() was set (pspoll happend, but no pkts local).
 */
void
wlc_apps_send_psp_response(wlc_info_t *wlc, struct scb *scb, uint16 fc)
{
    struct apps_scb_psinfo *scb_psinfo;
    int pktq_total;

    ASSERT(scb);
    ASSERT(wlc);

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    WL_PS_EX(scb, ("wl%d.%d: %s\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
            __FUNCTION__));
    /* Ignore trigger frames received during tx block period */
    if (scb_psinfo->tx_block != 0) {
        WL_PS_EX(scb, ("wl%d.%d: %s tx blocked; ignoring PS poll\n", wlc->pub->unit,
               WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));

        return;
    }

    if (scb_psinfo->twt_active) {
        return;
    }

    /* get available packet count for the given flow */
    pktq_total = wlc_apps_scb_pktq_tot(wlc, scb);

#ifdef BCMPCIEDEV
    /* Check for real packets and transient packets ; If not request from Host */
    if ((pktq_total == 0) && (wlc_sqs_v2r_pkts_tot(scb) == 0) &&
        wlc_sqs_vpkts_tot(scb)) {
        /* Request packet from host flow ring */
        wlc_sqs_psmode_pull_packets(wlc, scb,
            WLC_ACBITMAP_TO_TIDBITMAP(AC_BITMAP_ALL), 1);

        SCB_PSPOLL_PKT_WAITING(scb_psinfo) = TRUE;
        return;
    }

    SCB_PSPOLL_PKT_WAITING(scb_psinfo) = FALSE;
#endif /* BCMPCIEDEV */

    /* enable multiple ps-poll frame check */
    if (scb_psinfo->psp_flags & PS_PSP_ONRESP) {
        WL_PS_EX(scb, ("wl%d.%d: %s previous ps-poll frame under handling. "
            "drop new ps-poll frame\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));

        return;
    } else {
        scb_psinfo->psp_flags |= PS_PSP_ONRESP;
    }

    /*
     * Send a null data frame if there are no PS buffered
     * frames on APSD non-delivery-enabled ACs (WMM/APSD 3.6.1.6).
     */
    if (pktq_total == 0 || ((wlc_apps_apsd_ndelv_count(wlc, scb) == 0) &&
#ifdef BCMPCIEDEV
        (wlc_apps_apsd_ndelv_vpkt_count(wlc, scb) == 0) &&
#endif
        TRUE)) {
        /* Ensure pkt is not queued on psq */
        if (wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, 0, WLF_PSDONTQ,
            PRIO_8021D_BE, wlc_apps_send_psp_response_cb, NULL) == FALSE) {
            WL_ERROR(("wl%d: %s PS-Poll null data response failed\n",
                wlc->pub->unit, __FUNCTION__));
            scb_psinfo->psp_pending = FALSE;
        } else {
            scb_psinfo->psp_pending = TRUE;
        }
        scb_psinfo->psp_flags &= ~PS_MORE_DATA;
    }
    /* Check if we should ignore the ps poll */
    else if (scb_psinfo->psp_pending && !SCB_ISMULTI(scb)) {
        /* Reply to a non retried PS Poll pkt after the current
         * psp_pending has completed (if that pending pkt indicated "more
         * data"). This aids the stalemate introduced if a STA acks a ps
         * poll response but the AP misses that ack
         */
        if ((scb_psinfo->psp_flags & PS_MORE_DATA) && !(fc & FC_RETRY))
            scb_psinfo->psp_flags |= PS_PSP_REQ_PEND;
    } else {
        ac_bitmap_t ac_non_delv;
        uint32 precbitmap;

        /* Check whether there are any legacy frames before sending
         * any delivery enabled frames
         */
        if (wlc_apps_apsd_delv_count(wlc, scb) > 0) {
            ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;
            precbitmap = WLC_ACBITMAP_TO_PRECBITMAP(ac_non_delv);
        } else {
            ac_non_delv = AC_BITMAP_ALL;
            precbitmap = WLC_PREC_BMP_ALL;
        }

        if (!wlc_apps_ps_send(wlc, scb, precbitmap, 0)) {
#ifdef WLCFP
            if (CFP_ENAB(wlc->pub) && SCB_AMPDU(scb)) {
                wlc_cfp_ampdu_ps_send(wlc, scb,
                    WLC_ACBITMAP_TO_TIDBITMAP(ac_non_delv), 0);
            }
#endif /* WLCFP */
        }
    }
}

/* get PVB info */
static INLINE void
wlc_apps_tim_pvb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 *offset, int16 *length)
{
    apps_bss_info_t *bss_psinfo;
    uint8 n1 = 0, n2;

    bss_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, cfg);

#ifdef WL_MBSSID
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && BSSCFG_IS_MAIN_AP(cfg)) {
        wlc_bsscfg_t *bsscfg;
        apps_bss_info_t *bsscfg_psinfo;
        int i;
        uint16 aid_lo, aid_hi;
        uint16 max_bss_count;

        max_bss_count = wlc_ap_get_maxbss_count(wlc->ap);

        aid_lo = (uint8)bss_psinfo->aid_lo;
        aid_hi = (uint8)bss_psinfo->aid_hi;
        FOREACH_UP_AP(wlc, i, bsscfg) {
            if (bsscfg != cfg) {
                bsscfg_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
                if (!aid_lo || ((bsscfg_psinfo->aid_lo < aid_lo) &&
                        bsscfg_psinfo->aid_lo)) {
                    aid_lo = bsscfg_psinfo->aid_lo;
                }
                if (bsscfg_psinfo->aid_hi > aid_hi) {
                    aid_hi = bsscfg_psinfo->aid_hi;
                }
            }
        }

        if (aid_lo) {
            n1 = (uint8)((aid_lo - max_bss_count)/8);
        }
        /* n1 must be highest even number */
        n1 &= ~1;
        n2 = (uint8)(aid_hi/8);
        /* offset is set to zero support, to support (non mbssid support)stations */
        if (!wlc_ap_get_block_mbssid(wlc)) {
            n1 = 0;
        }
    } else
#endif /* WL_MBSSID */
    {
        n1 = (uint8)(bss_psinfo->aid_lo/8);
        /* n1 must be highest even number */
        n1 &= ~1;
        n2 = (uint8)(bss_psinfo->aid_hi/8);
    }

    *offset = n1;
    *length = n2 - n1 + 1;

    ASSERT(*offset <= 127);
    ASSERT(*length >= 1 && *length <= sizeof(bss_psinfo->pvb));
}

/* calculate TIM IE length */
static uint
wlc_apps_tim_len(wlc_info_t *wlc, wlc_bsscfg_t *cfg)
{
    uint8 offset;
    int16 length;
#ifdef WL_MBSSID
    uint16 max_bss_count;
    max_bss_count = wlc_ap_get_maxbss_count(wlc->ap);
#endif /* WL_MBSSID */

    ASSERT(cfg != NULL);
    ASSERT(BSSCFG_AP(cfg));

    wlc_apps_tim_pvb(wlc, cfg, &offset, &length);

#ifdef WL_MBSSID
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
        return TLV_HDR_LEN + DOT11_MNG_TIM_FIXED_LEN + length + (max_bss_count/8);
    } else {
        return TLV_HDR_LEN + DOT11_MNG_TIM_FIXED_LEN + length;
    }
#else
    return TLV_HDR_LEN + DOT11_MNG_TIM_FIXED_LEN + length;
#endif /* WL_MBSSID */
}

/* Fill in the TIM element for the specified bsscfg */
static int
wlc_apps_tim_create(wlc_info_t *wlc, wlc_bsscfg_t *cfg, uint8 *buf, uint len)
{
    apps_bss_info_t *bss_psinfo;
    uint8 offset;
    int16 length;
    wlc_bss_info_t *current_bss;
#ifdef WL_MBSSID
    uint16 max_bss_cnt;
    uint16 bcmc_bytes;
#endif /* WL_MBSSID */
    ASSERT(cfg != NULL);
    ASSERT(BSSCFG_AP(cfg));
    ASSERT(buf != NULL);

#ifdef WL_MBSSID
    max_bss_cnt = wlc_ap_get_maxbss_count(wlc->ap);
    bcmc_bytes = max_bss_cnt >= 8 ? max_bss_cnt/8 : 1;
#endif /* WL_MBSSID */
    /* perform length check to make sure tim buffer is big enough */
    if (wlc_apps_tim_len(wlc, cfg) > len)
        return BCME_BUFTOOSHORT;

    current_bss = cfg->current_bss;

    wlc_apps_tim_pvb(wlc, cfg, &offset, &length);

    buf[0] = DOT11_MNG_TIM_ID;
    /* set the length of the TIM */
#ifdef WL_MBSSID
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
        buf[1] = (uint8)(DOT11_MNG_TIM_FIXED_LEN + length + bcmc_bytes);
    } else {
        buf[1] = (uint8)(DOT11_MNG_TIM_FIXED_LEN + length);
    }
#else
    buf[1] = (uint8)(DOT11_MNG_TIM_FIXED_LEN + length);
#endif /* WL_MBSSID */
    buf[2] = (uint8)(current_bss->dtim_period - 1);
    buf[3] = (uint8)current_bss->dtim_period;
    /* set the offset field of the TIM */
    buf[4] = offset;
    /* copy the PVB into the TIM */
    bss_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, cfg);
#ifndef WL_MBSSID
    bcopy(&bss_psinfo->pvb[offset], &buf[DOT11_MNG_TIM_NON_PVB_LEN], length);
#else
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype)) {
        /* copy bc/mc bits corresponding to all mbss */
        bcopy(&bss_psinfo->pvb[0], &buf[DOT11_MNG_TIM_NON_PVB_LEN], bcmc_bytes);
        /* copy the sta's traffic bits of primary bss */
        bcopy(&bss_psinfo->pvb[bcmc_bytes + offset],
            &buf[DOT11_MNG_TIM_NON_PVB_LEN + bcmc_bytes], length);
    } else {
        bcopy(&bss_psinfo->pvb[offset],    &buf[DOT11_MNG_TIM_NON_PVB_LEN], length);
    }
    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && BSSCFG_IS_MAIN_AP(cfg)) {
        wlc_bsscfg_t *bsscfg;
        apps_bss_info_t *bsscfg_psinfo;
        int i, j;

        FOREACH_UP_AP(wlc, i, bsscfg) {
            if (bsscfg != cfg) {
                bsscfg_psinfo = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
                for (j = 0; j < length; j++) {
                    buf[DOT11_MNG_TIM_NON_PVB_LEN + j + bcmc_bytes]
                        |= bsscfg_psinfo->pvb[bcmc_bytes + offset + j];
                }
            }
        }
    }
#endif /* WL_MBSSID */

    return BCME_OK;
}

#if defined(BCMDBG)
static void
wlc_apps_bss_dump(void *ctx, wlc_bsscfg_t *cfg, struct bcmstrbuf *b)
{
    apps_wlc_psinfo_t *wlc_psinfo = (apps_wlc_psinfo_t *)ctx;
    apps_bss_info_t *bss_psinfo;
    uint8 offset;
    int16 length;

    ASSERT(cfg != NULL);

    bss_psinfo = APPS_BSSCFG_CUBBY(wlc_psinfo, cfg);
    if (bss_psinfo == NULL) {
        return;
    }

    wlc_apps_tim_pvb(cfg->wlc, cfg, &offset, &length);

    bcm_bprintf(b, "     offset %u length %d", offset, length);
    bcm_bprhex(b, " pvb ", TRUE, &bss_psinfo->pvb[offset], length);
}
#endif

/* wlc_apps_scb_supr_enq()
 *
 * Re-enqueue a suppressed frame.
 *
 * This fn is similar to wlc_apps_suppr_frame_enq(), except:
 *   - handles only unicast SCB traffic
 *   - SCB is Associated and in PS
 *   - Handles the suppression of PSPoll response. wlc_apps_suppr_frame does not handle
 *     these because they would not be suppressed due to PMQ suppression since PSP response
 *     are queued while STA is PS, so ignore PMQ is set.
 *
 * Called from:
 *  wlc.c:wlc_pkt_abs_supr_enq()
 *    <- wlc_dotxstatus()
 *    <- wlc_ampdu_dotxstatus_aqm_complete()
 *    <- wlc_ampdu_dotxstatus_complete() (non-AQM)
 *
 *  wlc_ampdu_suppr_pktretry()
 *   Which is called rom wlc_ampdu_dotxstatus_aqm_complete(),
 *   similar to TX_STATUS_SUPR_PPS (Pretend PS) case in wlc_dotxstatus()
 */
bool
wlc_apps_scb_supr_enq(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;

    ASSERT(scb != NULL);
    ASSERT(SCB_PS(scb));
    ASSERT(!SCB_ISMULTI(scb));
    ASSERT((SCB_ASSOCIATED(scb)) || (SCB_WDS(scb) != NULL));
    ASSERT(pkt != NULL);

    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    if (WLF2_PCB2(pkt) == WLF2_PCB2_PSP_RSP) {
        /* This packet was the ps-poll response.
         * Clear multiple ps-poll frame protection.
         */
        scb_psinfo->psp_flags &= ~PS_PSP_ONRESP;
        scb_psinfo->psp_pending = FALSE;
    }

    /* unregister pkt callback */
    WLF2_PCB2_UNREG(pkt);

    /* PR56242: tag the pkt so that we can identify them later and move them
     * to the front when tx fifo drain/flush finishes.
     */
    WLPKTTAG(pkt)->flags3 |= WLF3_SUPR;

    /* Mark as retrieved from HW FIFO */
    WLPKTTAG(pkt)->flags |= WLF_FIFOPKT;

    WL_PS_EX(scb, ("wl%d.%d: %s SUPPRESSED packet %p "MACF" PS:%d \n",
           wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
           pkt, ETHER_TO_MACF(scb->ea), SCB_PS(scb)));

    /* If enqueue to psq successfully, return FALSE so that PDU is not freed */
    /* Enqueue at higher precedence as these are suppressed packets */
    if (wlc_apps_psq(wlc, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt)))) {
        WLPKTTAG(pkt)->flags &= ~WLF_APSD;
#ifdef WLTAF
        /* Attempting re-enque into psq due to suppress indication.
         * Adjust TAF scores.
         */
        wlc_taf_txpkt_status(wlc->taf_handle, scb, PKTPRIO(pkt), pkt,
            TAF_TXPKT_STATUS_SUPPRESSED);
#endif
        return FALSE;
    }

    /* XXX The error message and accounting here is not really correct---f wlc_apps_psq() fails,
     * it is not always because a pkt was dropped.  In PropTxStatus, pkts are not enqueued if
     * the are host pkts, but they are suppressed back to host.  The drop accounting should only
     * be for pktq len limit, psq_pkts_hi high water mark, or SCB no longer associated.
     */
    WL_ERROR(("wl%d.%d: %s ps suppr pkt discarded\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
    wlc_psinfo->ps_discard++;
    scb_psinfo->ps_discard++;
#ifdef WLTAF
    wlc_taf_txpkt_status(wlc->taf_handle, scb, PKTPRIO(pkt), pkt,
        TAF_TXPKT_STATUS_SUPPRESSED_FREE);
#endif

    return TRUE;
}

/* wlc_apps_suppr_frame_enq()
 *
 * Enqueue a suppressed PDU to psq after fixing up the PDU
 *
 * Called from
 * wlc_dotxstatus():
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *   -> supr_status == TX_STATUS_SUPR_PPS (Pretend PS)
 *
 * wlc_ampdu_dotxstatus_aqm_complete()
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *
 * wlc_ampdu_dotxstatus_complete() (non-AQM)
 *   -> supr_status == TX_STATUS_SUPR_PMQ (PS)
 *
 * wlc_ampdu_suppr_pktretry()
 *   Which is called from wlc_ampdu_dotxstatus_aqm_complete(),
 *   similar to TX_STATUS_SUPR_PPS (Pretend PS) case in wlc_dotxstatus()
 */
bool
wlc_apps_suppr_frame_enq(wlc_info_t *wlc, void *pkt, tx_status_t *txs, bool last_frag)
{
    uint16 frag = 0;
    uint16 txcnt;
    uint16 seq_num = 0;
    struct scb *scb = WLPKTTAGSCBGET(pkt);
    struct apps_scb_psinfo *scb_psinfo;
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct dot11_header *h;
    uint16 txc_hwseq;
    wlc_txh_info_t txh_info;
    bool control;
    bool scb_put_in_ps = FALSE;

    ASSERT(scb != NULL);

    BCM_REFERENCE(scb_put_in_ps);

    if (!SCB_PS(scb) && !SCB_ISMULTI(scb)) {
        /* Due to races in what indications are processed first, we either get
         * a PMQ indication that a SCB has entered PS mode, or we get a PMQ
         * suppressed packet. This is the patch where a PMQ suppressed packet is
         * the first indication that a SCB is in PS mode.
         * Signal the PS switch with the flag that the indication was a suppress packet.
         */
        WL_PS_EX(scb, ("wl%d.%d: %s PMQ entry interrupt delayed! "MACF"\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea)));
        wlc_apps_process_ps_switch(wlc, scb, PS_SWITCH_PMQ_SUPPR_PKT);
        scb_put_in_ps = TRUE;
    }

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL)
        return TRUE;

    if (!(scb_psinfo->flags & SCB_PS_FIRST_SUPPR_HANDLED)) {
        /* Is this the first suppressed frame, and either is partial
         * MSDU or has been retried at least once, driver needs to
         * preserve the retry count and sequence number in the PDU so that
         * next time it is transmitted, the receiver can put it in order
         * or discard based on txcnt. For partial MSDU, reused sequence
         * number will allow reassembly
         */
        wlc_get_txh_info(wlc, pkt, &txh_info);

        h = txh_info.d11HdrPtr;
        control = FC_TYPE(ltoh16(h->fc)) == FC_TYPE_CTL;

        if (!control) {
            seq_num = ltoh16(h->seq);
            frag = seq_num & FRAGNUM_MASK;
        }

        if (D11REV_GE(wlc->pub->corerev, 40)) {
            /* TxStatus in txheader is not needed in chips with MAC agg. */
            /* txcnt = txs->status.frag_tx_cnt << TX_STATUS_FRM_RTX_SHIFT; */
            txcnt = 0;
        }
        else {
            txcnt = txs->status.raw_bits & (TX_STATUS_FRM_RTX_MASK);
        }

        if ((frag || txcnt) && !control) {
            /* If the seq num was hw generated then get it from the
             * status pkt otherwise get it from the original pkt
             */
            if (D11REV_GE(wlc->pub->corerev, 40)) {
                txc_hwseq = txh_info.MacTxControlLow & htol16(D11AC_TXC_ASEQ);
            } else {
                txc_hwseq = txh_info.MacTxControlLow & htol16(TXC_HWSEQ);
            }

            if (txc_hwseq)
                seq_num = txs->sequence;
            else
                seq_num = seq_num >> SEQNUM_SHIFT;

            h->seq = htol16((seq_num << SEQNUM_SHIFT) | (frag & FRAGNUM_MASK));

            /* Clear hwseq flag in maccontrol low */
            /* set the retry counts */
            if (D11REV_GE(wlc->pub->corerev, 40)) {
                d11txhdr_t* txh = txh_info.hdrPtr;
                *D11_TXH_GET_MACLOW_PTR(wlc, txh) &=  ~htol16(D11AC_TXC_ASEQ);
                if (D11REV_LT(wlc->pub->corerev, 128)) {
                    txh->rev40.PktInfo.TxStatus = htol16(txcnt);
                }
            } else {
                d11txh_pre40_t* nonVHTHdr = &(txh_info.hdrPtr->pre40);
                nonVHTHdr->MacTxControlLow &= ~htol16(TXC_HWSEQ);
                nonVHTHdr->TxStatus = htol16(txcnt);
            }

            WL_PS_EX(scb, ("wl%d.%d: %s Partial MSDU PDU %p - frag:%d seq_num:%d txcnt:"
                " %d\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
                OSL_OBFUSCATE_BUF(pkt), frag, seq_num, txcnt));
        }

        /* This ensures that all the MPDUs of the same SDU get
         * same seq_num. This is a case when first fragment was retried
         */
        if (last_frag || !(frag || txcnt))
            scb_psinfo->flags |= SCB_PS_FIRST_SUPPR_HANDLED;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s SUPPRESSED packet %p - "MACF" PS:%d\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        OSL_OBFUSCATE_BUF(pkt), ETHER_TO_MACF(scb->ea), SCB_PS(scb)));

    /* PR56242: tag the pkt so that we can identify them later and move them
     * to the front when tx fifo drain/flush finishes.
     */
    WLPKTTAG(pkt)->flags3 |= WLF3_SUPR;

    /* Mark as retrieved from HW FIFO */
    WLPKTTAG(pkt)->flags |= WLF_FIFOPKT;

    if (wlc->lifetime_txfifo) {
        /* Clear packet expiration for PS path if TXFIFO lifetime mode enabled,
         * to avoid unintended packet drop.
         */
        WLPKTTAG(pkt)->flags &= ~WLF_EXPTIME;
    }
    /* If in PS mode, enqueue the suppressed PDU to PSQ for ucast SCB otherwise txq */
    if (SCB_PS(scb) && !SCB_ISMULTI(scb) && !(WLPKTTAG(pkt)->flags & WLF_PSDONTQ)) {
#ifdef PROP_TXSTATUS
        /* in proptxstatus, the host will resend these suppressed packets */
        /* -memredux- dropped the packet body already anyway. */
        if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
            !(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
              WLFC_PKTFLAG_PKTFROMHOST))
#endif
        {
            /* If enqueue to psq successfully, return FALSE so that PDU is not freed */
            /* Enqueue at higher precedence as these are suppressed packets */

            /* Drop the non-8021X packets when keay exchange is in progress */
            if ((scb->dropblock_dur == 0) || (WLPKTTAG(pkt)->flags & WLF_8021X)) {
                if (wlc_apps_psq(wlc, pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt)))) {
#if defined(WL_PS_SCB_TXFIFO_BLK)
                    if (scb_put_in_ps && !SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)) {
                        /* The last and only packet, make sure normalization
                         * is triggered.
                         */
                        scb->ps_txfifo_blk = TRUE;
                        wlc->ps_txfifo_blk_scb_cnt++;
                    }
#endif /* WL_PS_SCB_TXFIFO_BLK */
#ifdef WLTAF
                    /* Attempting re-enque into psq due to suppress indication.
                     * Adjust TAF scores.
                     */
                    wlc_taf_txpkt_status(wlc->taf_handle, scb, PKTPRIO(pkt),
                        pkt, TAF_TXPKT_STATUS_SUPPRESSED);
#endif
                    return FALSE;
                }
            } else {
                WLCNTINCR(wlc->pub->_cnt->txfail);
                WLCNTSCBADD(scb->scb_stats.tx_failures, 1);
            }

            WL_PS_EX(scb, ("wl%d.%d: %s "MACF" ps suppr pkt discarded\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
                ETHER_TO_MACF(scb->ea)));
            wlc_psinfo->ps_discard++;
            scb_psinfo->ps_discard++;
        }
#ifdef WLTAF
        wlc_taf_txpkt_status(wlc->taf_handle, scb, PKTPRIO(pkt), pkt,
            TAF_TXPKT_STATUS_SUPPRESSED_FREE);
#endif
        return TRUE;
    }

#ifdef PROP_TXSTATUS
    if (!PROP_TXSTATUS_ENAB(wlc->pub) || !HOST_PROPTXSTATUS_ACTIVATED(wlc) ||
        !(WL_TXSTATUS_GET_FLAGS(WLPKTTAG(pkt)->wl_hdr_information) &
          WLFC_PKTFLAG_PKTFROMHOST) ||
        (pktqprec_n_pkts(WLC_GET_CQ(SCB_WLCIFP(scb)->qi),
                   WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt))) < BCMC_MAX))
#endif
    {
        if (cpktq_prec_enq(wlc, &SCB_WLCIFP(scb)->qi->cpktq,
                         pkt, WLC_PRIO_TO_HI_PREC(PKTPRIO(pkt)), FALSE)) {
            /* If the just enqueued frame is suppressed, mark the
             * queue with suppressed_pkts, indicating normalization
             * is required.
             * Should also qualify the packet's condition with
             *  "if (WLPKTTAG(pkt)->flags3 & WLF3_SUPR)"
             * before incrementing suppressed_pkts, but it's already
             * set above, so just use an assert-check.
             */
            ASSERT((WLPKTTAG(pkt)->flags3) & WLF3_SUPR);
            SCB_WLCIFP(scb)->qi->suppressed_pkts += 1;
#ifdef WL_PS_STATS
            WLC_BCMCSCB_GET(wlc, scb->bsscfg)->suprps_cnt += 1;
#endif /* WL_PS_STATS */

#if defined(WL_PS_SCB_TXFIFO_BLK)
            /* If BCMC frames are suppressed, block TX FIFO for PSQ normalization */
            wlc_block_datafifo(wlc, DATA_BLOCK_PS, DATA_BLOCK_PS);
            wlc->ps_scb_txfifo_blk = TRUE;

#if defined(WL_PS_STATS)
            /* only update time when previous DATA_BLOCK_PS is off */
            if (wlc->datablock_starttime == 0) {
                int pktpend_cnts = TXPKTPENDTOT(wlc);

                wlc->datablock_cnt++;
                wlc->datablock_starttime = OSL_SYSUPTIME_US();

                if ((wlc->pktpend_min == 0) || (wlc->pktpend_min > pktpend_cnts)) {
                    wlc->pktpend_min = pktpend_cnts;
                }

                if (wlc->pktpend_max < pktpend_cnts) {
                    wlc->pktpend_max = pktpend_cnts;
                }

                wlc->pktpend_tot += pktpend_cnts;
            }
#endif /* WL_PS_STATS */
#endif /* WL_PS_SCB_TXFIFO_BLK */

            /* Frame enqueued, caller doesn't free */
            return FALSE;
        }
    }

    /* error exit, return TRUE to have caller free packet */
    return TRUE;
}

#ifdef WLTWT

/* Put bcmc in (or out) PS as a result of TWT active link for scb */
static void
wlc_apps_bcmc_force_ps_by_twt(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, bool on)
{
    apps_bss_info_t *bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
    struct scb *bcmc_scb;

    bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
    ASSERT(bcmc_scb->bsscfg == bsscfg);

    if (on) {
        WL_PS0(("wl%d.%d: %s - ON\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
            __FUNCTION__));
        if (!SCB_PS(bcmc_scb)) {
            wlc_apps_bcmc_scb_ps_on(wlc, bsscfg);
        }
        bss_info->ps_trans_status |= BSS_PS_ON_BY_TWT;
    } else if (bss_info->ps_trans_status & BSS_PS_ON_BY_TWT) {
        WL_PS0(("wl%d.%d: %s - OFF\n", wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
            __FUNCTION__));
        bss_info->ps_trans_status &= ~BSS_PS_ON_BY_TWT;
        /* Set PS flag on bcmc_scb to avoid ASSERTs */
        bcmc_scb->PS = TRUE;
        if (bss_info->ps_nodes == 0) {
            wlc_apps_bss_ps_off_start(wlc, bcmc_scb);
        }
    }
}

bool
wlc_apps_twt_sp_enter_ps(wlc_info_t *wlc, scb_t *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL) {
        return FALSE;
    }
    if (SCB_TWTPS(scb)) {
        return TRUE;
    }
    /* There is a race condition possible which is tough to handle.The SCB may
     * haven been taken out of TWT but still packets arrive with TWT suppress.
     * They cant be enqueued using regular PS, as there may not come PM event from
     * remote for long time. We cant put SCB in PS_TWT, as we are already supposed to
     * have exited. The best would be to use pspretend and get that module solve the
     * exit of SP. But hat is hard. For now are just going to drop the packet. May
     * need some fixing later. Lets not drop it but re-enque, gives possible out of
     * order, but less problematic. May cause AMPDU assert?
     */
    if (!(wlc_twt_scb_active(wlc->twti, scb))) {
        return FALSE;
    }
    WL_PS_EX(scb, ("wl%d.%d: %s Activating PS_TWT AID %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, SCB_AID(scb)));

    scb->PS_TWT = TRUE;
#ifdef WL_PS_STATS
    if (scb->ps_starttime == 0) {
        scb->ps_on_cnt++;
        scb->ps_starttime = OSL_SYSUPTIME();
    }
#endif /* WL_PS_STATS */
    /* NOTE: set flags to SCB_PS_FIRST_SUPPR_HANDLED then this function does not have to
     * use txstatus towards wlc_apps_suppr_frame_enq. So if suppport for
     * first_suppr_handled is needed then txstatus has to be passed on this fuction !!
     */
    scb_psinfo->flags |= SCB_PS_FIRST_SUPPR_HANDLED;
    /* Add the APPS to the txpath for this SCB */
    wlc_txmod_config(wlc->txmodi, scb, TXMOD_APPS);

#ifdef WLTAF
    /* Update TAF state */
    wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, NULL,
        TAF_SCBSTATE_TWT_SP_EXIT);
#endif /* WLTAF */

#if defined(BCMPCIEDEV)
    /* Reset all pending fetch and rollback flow fetch ptrs */
    wlc_scb_flow_ps_update(wlc->wl, SCB_FLOWID(scb), TRUE);
#endif
    /* ps enQ any pkts on the txq, narq, ampduq */
    wlc_apps_txq_to_psq(wlc, scb);

    /* XXX TODO : Rollback packets to flow ring rather than
     * sending them into PSq during PS transition for PQP
     */
#ifdef PROP_TXSTATUS
#ifdef WLAMPDU
    /* This causes problems for PSPRETEND */
    wlc_apps_ampdu_txq_to_psq(wlc, scb);
#endif /* WLAMPDU */
#ifdef WLNAR
    wlc_apps_nar_txq_to_psq(wlc, scb);
#endif /* WLNAR */
#endif /* PROP_TXSTATUS */
    if (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)) {
        scb_psinfo->twt_wait4drain_norm = TRUE;
    }

#ifdef HNDPQP
    /* PQP page out of suppress PSq and PSq */
    wlc_apps_scb_pqp_pgo(wlc, scb, WLC_PREC_BMP_ALL);
#endif
    return TRUE;
}

bool
wlc_apps_suppr_twt_frame_enq(wlc_info_t *wlc, void *pkt)
{
    scb_t *scb = WLPKTTAGSCBGET(pkt);
    struct apps_scb_psinfo *scb_psinfo;
    bool ret_value;
    bool current_ps_state;

    ASSERT(scb != NULL);
    ASSERT(!SCB_ISMULTI(scb));

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL) {
        return TRUE;
    }

    if (!wlc_apps_twt_sp_enter_ps(wlc, scb)) {
        /* On exit TWT the packet arrives here. The SCB should be in PS, use normal
         * wlc_apps_suppr_frame_enq to handle the packet.
         */
        if (SCB_PS(scb)) {
            ret_value = wlc_apps_suppr_frame_enq(wlc, pkt, NULL, TRUE);
        } else {
            ret_value = TRUE;
        }
        return ret_value;
    }

    /* Simulate PS to be able to call suppr_frame_enq */
    current_ps_state = scb->PS;
    scb->PS = TRUE;
    ret_value = wlc_apps_suppr_frame_enq(wlc, pkt, NULL, TRUE);
    ASSERT(scb->PS);
    scb->PS = current_ps_state;

    /* If enqueue to supr psq successfully. Driver should do psq normalization.
     * But below case will not call wlc_apps_scb_psq_norm() to do it.
     * 0. There is only one suppression packet.
     * 1. wlc_ampdu_dotxstatus_aqm_complete() decrease SCB_TOT_PKTS_INFLT_FIFOCNT_VAL to 0.
     * 2. wlc_apps_suppr_twt_frame_enq()
     * 3. wlc_apps_twt_sp_enter_ps(), SCB_TOT_PKTS_INFLT_FIFOCNT_VAL is 0.
     *    So twt_wait4drain_norm will not be set to TRUE.
     * 4. wlc_apps_trigger_on_complete(), will not call wlc_apps_scb_psq_norm()
     *    because twt_wait4drain_norm is FALSE.
     * When this happened, driver left this packet in supr psq without enqueue to psq.
     * This packet will not be sent via psq.
     * It will trigger ampdu watchdog to cleanup the ampdu ini for this tid.
     * Set twt_wait4drain_norm to TRUE if driver enqueue to supr psq or psq successfully.
     */
    if (!ret_value) {
        scb_psinfo->twt_wait4drain_norm = TRUE;
    }

    return ret_value;
}

static INLINE void
_wlc_apps_twt_sp_release_ps(wlc_info_t *wlc, scb_t *scb)
{
#ifdef WLTAF
    /* Update TAF state */
    wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NO_SOURCE, NULL,
        TAF_SCBSTATE_TWT_SP_ENTER);
#endif /* WLTAF */
}

void
wlc_apps_twt_sp_release_ps(wlc_info_t *wlc, scb_t *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL)
        return;

    if (!SCB_TWTPS(scb)) {
        return;
    }

    if (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)) {
        /* We cant release the PS yet, we are still waiting for more suppress */
        WL_PS_EX(scb, ("wl%d.%d: %s PS_TWT active AID %d, delaying release, fifocnt %d\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            SCB_AID(scb), SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)));
        scb_psinfo->twt_wait4drain_exit = TRUE;
        return;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s PS_TWT active, releasing AID %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, SCB_AID(scb)));
    scb->PS_TWT = FALSE;
#ifdef WL_PS_STATS
    wlc_apps_upd_pstime(scb);
#endif /* WL_PS_STATS */
    scb_psinfo->twt_wait4drain_exit = FALSE;

    /* clear the PVB entry since we are leaving PM mode */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }

#if defined(BCMPCIEDEV)
    /* Reset all pending fetch and rollback flow fetch ptrs */
    wlc_scb_flow_ps_update(wlc->wl, SCB_FLOWID(scb), FALSE);
#endif
    /* XXX wlc_wlfc_scb_ps_off did a reset of AMPDU seq with a BAR.
     * Done only for FD mode. Required .??
     * XXX With prop tx gone, TWT need to handle packet release through TAF
     */

    /* Unconfigure the APPS from the txpath */
    wlc_txmod_unconfig(wlc->txmodi, scb, TXMOD_APPS);
#ifdef HNDPQP
    /* Page in all Host resident packets into dongle */
    wlc_apps_scb_pqp_pgi(wlc, scb, WLC_PREC_BMP_ALL, PKTQ_PKTS_ALL, PQP_PGI_PS_TWT_OFF);
#endif /* HNDPQP */

    while (wlc_apps_ps_send(wlc, scb, WLC_PREC_BMP_ALL, 0));

#ifdef HNDPQP
    /* If there are still pkts in host, it means PQP is out of resource.
     * PQP will set flags for current PS transition and
     * resume the remain process when resource is available.
     */
    if (PQP_PGI_PS_TRANS_OFF_ISSET(scb_psinfo))
        return;
#endif /* HNDPQP */

    _wlc_apps_twt_sp_release_ps(wlc, scb);
}

/*
 * This function is to be called when fifo (LO/HW) is empty and twt was to be entered. The SCB will
 * get PS state clearead and switched to TWT_PS mode. PS mode should be active, as it is expected
 * that data for this SCB is blocked to drain (LO/HW) queue allowing it to switch it over to TWT
 */
static void
wlc_apps_twt_enter_ready(wlc_info_t *wlc, scb_t *scb)
{
    apps_wlc_psinfo_t *wlc_psinfo = wlc->psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    apps_bss_info_t *bss_info;

    WL_TWT(("%s Enter AID %d\n", __FUNCTION__, SCB_AID(scb)));

    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    bss_info = APPS_BSSCFG_CUBBY(wlc_psinfo, SCB_BSSCFG(scb));
    ASSERT(bss_info);

    /* SCB_PS_TRANS_OFF_BLOCKED should be cleared as for entering TWT we may have used
     * PS_SWITCH_PMQ_SUPPR_PKT which caused this flag to be set.
     */
    scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_BLOCKED;
#ifdef WL_PS_STATS
    if (scb->PS) {
        wlc_apps_upd_pstime(scb);
    }
#endif /* WL_PS_STATS */
    ASSERT(scb->PS);
    scb->PS = FALSE;
    scb->PS_TWT = TRUE;
#ifdef WL_PS_STATS
    if (scb->ps_starttime == 0) {
        scb->ps_on_cnt++;
        scb->ps_starttime = OSL_SYSUPTIME();
    }
#endif /* WL_PS_STATS */
    ASSERT(bss_info->ps_nodes);
    bss_info->ps_nodes--;
    ASSERT(wlc_psinfo->ps_nodes_all);
    wlc_psinfo->ps_nodes_all--;
    /* Check if the PVB entry needs to be cleared */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }

    wlc_twt_apps_ready_for_twt(wlc->twti, scb);
}

/*
 * This function is to be called when scb puts first SP in TWT mode. APPS will take the necessary
 * steps to get the queues flushed (if needed). Once LO/HW queues are empty PS mode is cleared and
 * TWT_PS becomes active for this SCB. When this happens a callback to TWT will be made to
 * notify TWT that SCB is prepared and ready to be used for TWT.
 */
void
wlc_apps_twt_enter(wlc_info_t *wlc, scb_t *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct scb_iter scbiter;
    scb_t *scb_tst;
    uint total;

#if defined(WL_PS_SCB_TXFIFO_BLK)
    WL_TWT(("%s Enter AID %d current PS %d (%d/%d)\n", __FUNCTION__, SCB_AID(scb), SCB_PS(scb),
        SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb), SCB_PS_TXFIFO_BLK(scb)));
#else /* ! WL_PS_SCB_TXFIFO_BLK */
    WL_TWT(("%s Enter AID %d current PS %d (%d)\n", __FUNCTION__, SCB_AID(scb), SCB_PS(scb),
        SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)));
#endif /* ! WL_PS_SCB_TXFIFO_BLK */

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL)
        return;

    ASSERT(!scb_psinfo->twt_active);
    scb_psinfo->twt_active = TRUE;

    /* Clear twt_wait4drain_exit as we may have set this and it will cause (delayed)
     * exiting TWT even though we already got (re-)entered here.
     */
    scb_psinfo->twt_wait4drain_exit = FALSE;

    /* Force enter of PS when it is not active */
    if (SCB_PS(scb)) {
        WL_TWT(("%s SCB already in PS\n", __FUNCTION__));
#ifdef PSPRETEND
        /* Make sure pspretend wont control PS states anymore, TWT takes over */
        if (wlc->pps_info != NULL) {
            wlc_pspretend_scb_ps_off(wlc->pps_info, scb);
        }
#endif /* PSPRETEND */
    } else {
        WL_TWT(("%s Forcing SCB in PS\n", __FUNCTION__));
        wlc_apps_process_ps_switch(wlc, scb, PS_SWITCH_PMQ_SUPPR_PKT);
        /* Remove the wait for PMQ block */
        scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_BLOCKED;
    }

#if defined(WL_PS_SCB_TXFIFO_BLK)
    if (SCB_PS_TXFIFO_BLK(scb)) {
#else /* ! WL_PS_SCB_TXFIFO_BLK */
    if (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb) && (wlc->block_datafifo & DATA_BLOCK_PS)) {
#endif /* ! WL_PS_SCB_TXFIFO_BLK */
        /* Set flag to identify the wait for drainage complete */
        scb_psinfo->twt_wait4drain_enter = TRUE;
    } else {
        /* APPS is ready, queus are empty, link can be put in TWT mode */
        wlc_apps_twt_enter_ready(wlc, scb);
    }

    /* Check if we should put BCMC for BSSCFG in PS. If this is first SCB to enter TWT and
     * then keep BCMC in PS.
     */
    total = 0;
    FOREACH_BSS_SCB(wlc->scbstate, &scbiter, SCB_BSSCFG(scb), scb_tst) {
        if (SCB_PSINFO(wlc->psinfo, scb_tst)->twt_active) {
            total++;
        }
    }
    if (total == 1) {
        wlc_apps_bcmc_force_ps_by_twt(wlc, SCB_BSSCFG(scb), TRUE);
    }
}

void
wlc_apps_twt_exit(wlc_info_t *wlc, scb_t *scb, bool enter_pm)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct scb_iter scbiter;
    scb_t *scb_tst;
    uint total;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL) {
        return;
    }

    WL_TWT(("%s Enter AID %d new PM %d (%d/%d/%d/%d/%d)\n", __FUNCTION__, SCB_AID(scb),
        enter_pm, scb_psinfo->twt_active, SCB_PS(scb), SCB_TWTPS(scb),
        scb_psinfo->twt_wait4drain_enter, SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)));

    if (!scb_psinfo->twt_active) {
        return;
    }
    scb_psinfo->twt_active = FALSE;

    /* Clear twt_wait4drain_enter as we may have set this and it will cause (delayed)
     * entering TWT even though we already got exited here.
     */
    scb_psinfo->twt_wait4drain_enter = FALSE;

    /* Force enter of PS when it is not active, even if PM mode indicates PM off then PM
     * has to be initiated first if there are packets in flight. This is needed so the TWT
     * flags get removed from the packets. Also if current mode is PS_TWT then switch to
     * regular PM mode.
     */
    if (SCB_TWTPS(scb) || (enter_pm) || (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb))) {
        WL_TWT(("%s Forcing SCB in PS\n", __FUNCTION__));
        wlc_apps_process_ps_switch(wlc, scb, PS_SWITCH_PMQ_SUPPR_PKT);
        /* Remove the wait for PMQ block, as the packet is 'simulated' */
        scb_psinfo->ps_trans_status &= ~SCB_PS_TRANS_OFF_BLOCKED;
        scb->PS_TWT = FALSE;
#ifdef WL_PS_STATS
        wlc_apps_upd_pstime(scb);
#endif /* WL_PS_STATS */
    }

    /* Check if we should take BCMC for BSSCFG out of PS. If this is last SCB to exit TWT for
     * this bsscfg then switch back to normal BCMC PS mode.
     */
    total = 0;
    FOREACH_BSS_SCB(wlc->scbstate, &scbiter, SCB_BSSCFG(scb), scb_tst) {
        if (SCB_PSINFO(wlc->psinfo, scb_tst)->twt_active) {
            total++;
        }
    }
    if (total == 0) {
        wlc_apps_bcmc_force_ps_by_twt(wlc, SCB_BSSCFG(scb), FALSE);
    }

    /* If PM off then start releasing packets. Use PM routines to leave the PM mode. */
    if (!enter_pm) {
        wlc_apps_process_ps_switch(wlc, scb, PS_SWITCH_OFF);
    }
}

#endif /* WLTWT */

/*
 * This function is to be called once the txstatus processing has completed, and the SCB
 * counters have been updated. Do note that SCB can be null. In that case iteration over all
 * SCBs should be performed. This trigger is to be called from wlc_txfifo_complete, which
 * normally gets called to (among other things) handle/finalize txdata block. In this function
 * datablock on a per scb is to be handled. Currently in use for TWT & PS.
 */
void
wlc_apps_trigger_on_complete(wlc_info_t *wlc, scb_t *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    /* TO BE FIXED, take quick approach for testing now */
    if ((!scb) || (SCB_INTERNAL(scb))) {
        return;
    }

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (scb_psinfo == NULL) {
        return;
    }
#ifdef HNDPQP
    /* Page out PS/Suppress PS queue at the end of Tx status processing.
     * If PGI_PS_TRANS is in progress, don't page out the PS queue.
     */
    if (PQP_PGI_PS_TRANS(scb_psinfo) == 0)
        wlc_apps_scb_pqp_pgo(wlc, scb, WLC_PREC_BMP_ALL);
#endif
    /* When this was the last packet outstanding in LO/HW queue then..... */
    if (SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb) == 0) {
        if (SCB_TWTPS(scb)) {
            if (scb_psinfo->twt_wait4drain_norm) {
                wlc_apps_scb_psq_norm(wlc, scb);
                scb_psinfo->twt_wait4drain_norm = FALSE;
            }
            if (scb_psinfo->twt_wait4drain_exit) {
                wlc_apps_twt_sp_release_ps(wlc, scb);
            } else {
                wlc_twt_ps_suppress_done(wlc->twti, scb);
            }
        }

#if defined(WL_PS_SCB_TXFIFO_BLK)
        if (wlc->ps_txfifo_blk_scb_cnt) {

            if (SCB_PS(scb) && !SCB_ISMULTI(scb)) {

                if (SCB_PS_TXFIFO_BLK(scb)) {
                    scb->ps_txfifo_blk = FALSE;
                    wlc->ps_txfifo_blk_scb_cnt--;

                    /* Notify bmac to remove PMQ entry for this STA */
                    wlc_pmq_process_ps_switch(wlc, scb, PS_SWITCH_STA_REMOVED);

                    WL_PS_EX(scb, ("wl%d.%d %s: Normalizing PSQ for "
                        "STA "MACF"\n",
                        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                        __FUNCTION__, ETHER_TO_MACF(scb->ea)));

                    wlc_apps_scb_psq_norm(wlc, scb);
                }

                if (scb_psinfo->twt_wait4drain_enter) {
                    wlc_apps_twt_enter_ready(wlc, scb);
                    scb_psinfo->twt_wait4drain_enter = FALSE;

                } else if ((scb_psinfo->ps_trans_status & SCB_PS_TRANS_OFF_PEND)) {
                    /* we may get here as result of SCB deletion, so avoid
                     * re-enqueueing frames in that case by discarding them
                     */

                    bool discard = SCB_DEL_IN_PROGRESS(scb)? TRUE : FALSE;
#ifdef PSPRETEND
                    if (SCB_PS_PRETEND_BLOCKED(scb)) {
                        WL_ERROR(("wl%d.%d: %s: SCB_PS_PRETEND_BLOCKED, "
                            "expected to see PMQ PPS entry\n",
                            wlc->pub->unit,
                            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                            __FUNCTION__));
                    }
#endif /* PSPRETEND */
                    WL_PS_EX(scb, ("wl%d.%d %s: Allowing PS Off for "
                        "STA "MACF"\n",
                        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                        __FUNCTION__, ETHER_TO_MACF(scb->ea)));
                    wlc_apps_scb_ps_off(wlc, scb, discard);
                }

            } /* SCB_PS(scb) && !SCB_ISMULTI(scb) */

#ifdef PSPRETEND
            if (SCB_CLEANUP_IN_PROGRESS(scb) || SCB_DEL_IN_PROGRESS(scb) ||
                SCB_MARKED_DEL(scb)) {
                WL_PS_EX(scb, ("wl%d.%d: %s: scb del in progress or marked"
                    " for del \n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__));
            } else if (SCB_PS_PRETEND_PROBING(scb)) {
                wlc_pspretend_probe_sched(wlc->pps_info, scb);
            }
#endif /* PSPRETEND */
        }
#endif /* WL_PS_SCB_TXFIFO_BLK */
    }
}

/*
 * WLF_PSDONTQ notes
 *    wlc_pkt_abs_supr_enq() drops PSDONTQ pkts on suppress with comment
 *        "toss driver generated NULL frame"
 *    wlc_dotxstatus(): PSPRETEND avoids working on PSDONTQ frames with the comment
 *        "the flag for WLF_PSDONTQ is checked because this is used for probe packets."
 *        Also in same fn pspretend avoids psdontq frame for wlc_apps_process_pspretend_status()
 *    wlc_queue_80211_frag(): added PSDONTQ flag to non-bufferable mgmt in r490247
 *        Auth, (Re)Assoc, Probe, Bcn, ATIM
 *    wlc_queue_80211_frag(): USE the PSDONTQ flag to send to TxQ instead of scb->psq
 *    wlc_sendctl(): USE the PSDONTQ flag to send to TxQ instead of scb->psq
 *    wlc_ap_sta_probe(): SETS for PSPretend NULL DATA, clear on AP sta probe.
 *    wlc_ap_do_pspretend_probe(): SETS for PSPretend NULL DATA
 *    _wlc_apps_ps_enq(): TDLS checks and passes to next TxMod "for TDLS PTI resp, don't enq to PSQ,
 *        send right away" odd that this fn is also called for txq_to_psq() for PS on transition,
 *        so don't think TxMod should be happening
 *    wlc_apps_ps_prep_mpdu(): checks PSDONTQ to identify final NULL DATA in pspoll
 *        chain termination
 *    wlc_apps_send_psp_response(): SETTING PSDONTQ in final NULL DATA for pspoll chain
 *    wlc_apps_suppr_frame_enq(): handling PMQ suppressed pkts, sends PSDONTQ pkts to TxQ instead
 *        of psq, just like wlc_queue_80211_frag() would have
 *    wlc_apps_apsd_eosp_send(): SETTING PSDONTQ in final NULL DATA for APSD service period
 *    wlc_apps_apsd_prepare(): ??? TDLS checks PSDONTQ in it's logic to clear eosp
 *    wlc_p2p_send_prbresp(): SETS PSDONTQ for Probe Resp
 *    wlc_probresp_send_probe_resp(): SETS PSDONTQ for Probe Resp
 *    wlc_tdls_send_pti_resp(): SETS PSDONTQ for action frame
 *    wlc_tx_fifo_sync_complete(): (OLD! Non-NEW_TXQ!) would send any frames recovered during sync
 *        to the psq for a SCB_PS() sta if not PSDONTQ. Looks like it was soft PMQ processing
 *        at the end of sync. Seems like this would have thrown off tx_intransit counts?
 *    wlc_ap_wds_probe():  SETS for PSPretend NULL DATA, clear on AP sta probe.
 */

/*
 * APSD Host Packet Timeout (hpkt_tmout)
 * In order to keep a U-APSD Service Period active in a Prop_TxStatus configuration,
 * a timer is used to indicate that a packet may be arriving soon from the host.
 *
 * Normally an apsd service period would end as soon as there were no more pkts queued
 * for a destination. But since there may be a lag from request to delivery of a pkt
 * from the host, the hpkt_tmout timer is set when a host pkt request is made.
 *
 * The pkt completion routine wlc_apps_apsd_complete() will normally send the next packet,
 * or end the service period if no more pkts. Instead of ending the serivice period,
 * if "apsd_hpkt_timer_on" is true, nothing is done in wlc_apps_apsd_complete(), and instead
 * this routine will end the service period if the timer expires.
 */
static void
wlc_apps_apsd_hpkt_tmout(void *arg)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct scb *scb;
    wlc_info_t *wlc;

    scb_psinfo = (struct apps_scb_psinfo *)arg;
    ASSERT(scb_psinfo);

    scb = scb_psinfo->scb;
    wlc = scb_psinfo->wlc;

    ASSERT(scb);
    ASSERT(wlc);

    /* send the eosp if still valid (entry to p2p abs makes apsd_usp false)
    * and no pkt in transit/waiting on pkt complete
    */

    if (scb_psinfo->apsd_usp == TRUE && !scb_psinfo->apsd_tx_pending &&
        (scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null)) {
        wlc_apps_apsd_send(wlc, scb);
    }
    scb_psinfo->apsd_hpkt_timer_on = FALSE;
}

static void
wlc_apps_apsd_send(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint prec_bmp;
#ifdef BCMPCIEDEV
    ac_bitmap_t ac_to_request = scb->apsd.ac_delv & AC_BITMAP_ALL;
#endif

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);
    ASSERT(scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null);

    /*
     * If there are no buffered frames, send a QoS Null on the highest delivery-enabled AC
     * (which AC to use is not specified by WMM/APSD).
     */
    if (scb_psinfo->ext_qos_null ||
        ((wlc_apps_apsd_delv_count(wlc, scb) == 0) &&
#ifdef BCMPCIEDEV
        (wlc_apps_apsd_delv_vpkt_count(wlc, scb) == 0) &&
#endif
        TRUE)) {
#ifdef WLTDLS
        if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
            wlc_tdls_in_pti_interval(wlc->tdls, scb)) {
            return;
        }
#endif /* WLTDLS */
        wlc_apps_apsd_eosp_send(wlc, scb);
        return;
    }

    prec_bmp = wlc_apps_ac2precbmp_info()[scb->apsd.ac_delv];

#ifdef BCMPCIEDEV
    /* Continuous pkt flow till last packet is is needed for Wi-Fi P2P 6.1.12/6.1.13.
     * by fetching pkts from host one after another
     * and wait till either timer expires or new packet is received
     */
#ifdef WLAMSDU_TX
    uint16 max_sf_frames = wlc_amsdu_scb_max_sframes(wlc->ami, scb);
#else
    uint16 max_sf_frames = 1;
#endif

    if (!scb_psinfo->apsd_hpkt_timer_on &&
        scb_psinfo->apsd_cnt > 1 &&
        (wlc_apps_apsd_delv_count(wlc, scb) <=
        (WLC_APSD_DELV_CNT_LOW_WATERMARK * max_sf_frames))) {

        /* fetch packets from host flow ring */
        scb_psinfo->apsd_v2r_in_transit +=
            wlc_sqs_psmode_pull_packets(wlc, scb,
            WLC_ACBITMAP_TO_TIDBITMAP(ac_to_request),
            (WLC_APSD_DELV_CNT_HIGH_WATERMARK * max_sf_frames));

        wl_add_timer(wlc->wl, scb_psinfo->apsd_hpkt_timer,
            WLC_PS_APSD_HPKT_TIME, FALSE);
        scb_psinfo->apsd_hpkt_timer_on = TRUE;

        if (wlc_apps_apsd_delv_count(wlc, scb) == 0)
            return;
    }
#endif /* BCMPCIEDEV */
    /*
     * Send a delivery frame.  When the frame goes out, the wlc_apps_apsd_complete()
     * callback will attempt to send the next delivery frame.
     */
    if (!wlc_apps_ps_send(wlc, scb, prec_bmp, WLF_APSD)) {
#ifdef WLCFP
        if (CFP_ENAB(wlc->pub) && SCB_AMPDU(scb) && !wlc_cfp_ampdu_ps_send(wlc, scb,
            WLC_ACBITMAP_TO_TIDBITMAP(scb->apsd.ac_delv), WLF_APSD))
#endif /* WLCFP */
            wlc_apps_apsd_usp_end(wlc, scb);
    }

}

#ifdef WLTDLS
void
wlc_apps_apsd_tdls_send(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    if (BSS_TDLS_ENAB(wlc, scb->bsscfg)) {
        if (!scb_psinfo->apsd_usp)
            return;

        scb_psinfo->apsd_cnt = wlc_apps_apsd_delv_count(wlc, scb);

        if (scb_psinfo->apsd_cnt)
            wlc_apps_apsd_send(wlc, scb);
        else
            wlc_apps_apsd_eosp_send(wlc, scb);
    }
    return;
}
#endif /* WLTDLS */

static const uint8 apsd_delv_acbmp2maxprio[] = {
    PRIO_8021D_BE, PRIO_8021D_BE, PRIO_8021D_BK, PRIO_8021D_BK,
    PRIO_8021D_VI, PRIO_8021D_VI, PRIO_8021D_VI, PRIO_8021D_VI,
    PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC,
    PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC, PRIO_8021D_NC
};

/* Send frames in a USP, called in response to receiving a trigger frame */
void
wlc_apps_apsd_trigger(wlc_info_t *wlc, struct scb *scb, int ac)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    /* Ignore trigger frames received during tx block period */
    if (scb_psinfo->tx_block != 0) {
        WL_PS_EX(scb, ("wl%d.%d: %s tx blocked; ignoring trigger\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        return;
    }

    /* Ignore trigger frames received during an existing USP */
    if (scb_psinfo->apsd_usp) {
        WL_PS_EX(scb, ("wl%d.%d: %s already in USP; ignoring trigger\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));

        /* Reset usp if the num of triggers exceeds the threshold */
        if (++scb_psinfo->apsd_trig_cnt > APSD_TRIG_THR) {
            scb_psinfo->apsd_trig_cnt = 0;
            wlc->apsd_usp_reset++;
            wlc_apps_apsd_usp_end(wlc, scb);
        }
        return;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s ac %d buffered %d delv %d\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ac,
        pktqprec_n_pkts(&scb_psinfo->psq, ac), wlc_apps_apsd_delv_count(wlc, scb)));

    scb_psinfo->apsd_usp = TRUE;

    /* initialize the delivery count for this SP */
    scb_psinfo->apsd_cnt = scb->apsd.maxsplen;

    /*
     * Send the first delivery frame.  Subsequent delivery frames will be sent by the
     * completion callback of each previous frame.  This is not very efficient, but if
     * we were to queue a bunch of frames to different FIFOs, there would be no
     * guarantee that the MAC would send the EOSP last.
     */

    wlc_apps_apsd_send(wlc, scb);
}

static void
wlc_apps_apsd_eosp_send(wlc_info_t *wlc, struct scb *scb)
{
    int prio = (int)apsd_delv_acbmp2maxprio[scb->apsd.ac_delv & 0xf];
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    WL_PS_EX(scb, ("wl%d.%d: %s sending QoS Null prio=%d\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, prio));

    scb_psinfo->ext_qos_null = FALSE;
    scb_psinfo->apsd_cnt = 0;

    if (wlc_sendnulldata(wlc, scb->bsscfg, &scb->ea, 0,
        (WLF_PSDONTQ | WLF_APSD), prio, NULL, NULL) == FALSE) {
        WL_ERROR(("wl%d: %s could not send QoS Null\n",
            wlc->pub->unit, __FUNCTION__));
        wlc_apps_apsd_usp_end(wlc, scb);
    }

    /* just reset the apsd_uspflag, don't update the apsd_endtime to allow TDLS PTI */
    /* to send immediately for the first packet */
    if (BSS_TDLS_ENAB(wlc, scb->bsscfg))
        wlc_apps_apsd_usp_end(wlc, scb);
}

/* Make decision if we need to count MMPDU in SP */
static bool
wlc_apps_apsd_count_mmpdu_in_sp(wlc_info_t *wlc, struct scb *scb, void *pkt)
{
    BCM_REFERENCE(wlc);
    BCM_REFERENCE(scb);
    BCM_REFERENCE(pkt);

    return TRUE;
}

/* wlc_apps_apsd_prepare()
 *
 * Keeps track of APSD info as pkts are prepared for TX. Updates the apsd_cnt (number of pkts
 * sent during a Service Period) and marks the EOSP bit on the pkt or calls for
 * a null data frame to do the same (ext_qos_null) if the end of the SP has been reached.
 */
void
wlc_apps_apsd_prepare(wlc_info_t *wlc, struct scb *scb, void *pkt,
    struct dot11_header *h, bool last_frag)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint16 *pqos;
    bool qos;
    bool more = FALSE;
    bool eosp = FALSE;

    /* The packet must have 802.11 header */
    ASSERT(WLPKTTAG(pkt)->flags & WLF_TXHDR);

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    /* Set MoreData if there are still buffered delivery frames */
    if ((wlc_apps_apsd_delv_count(wlc, scb) > 0) ||
#ifdef BCMPCIEDEV
        (wlc_apps_apsd_delv_vpkt_count(wlc, scb) > 0) ||
#endif
        FALSE)
    {
        h->fc |= htol16(FC_MOREDATA);
    } else {
        h->fc &= ~htol16(FC_MOREDATA);
    }

    qos = ((ltoh16(h->fc) & FC_KIND_MASK) == FC_QOS_DATA) ||
          ((ltoh16(h->fc) & FC_KIND_MASK) == FC_QOS_NULL);

    /* SP countdown */
    if (last_frag &&
        (qos || wlc_apps_apsd_count_mmpdu_in_sp(wlc, scb, pkt))) {
        /* Indicate EOSP when this is the last MSDU in the psq */
        /* JQL: should we keep going in case there are on-the-fly
         * MSDUs and let the completion callback to check if there is
         * any other buffered MSDUs then and indicate the EOSP using
         * an extra QoS NULL frame?
         */
        more = (ltoh16(h->fc) & FC_MOREDATA) != 0;
        if (!more)
            scb_psinfo->apsd_cnt = 1;
        /* Decrement count of packets left in service period */
        if (scb_psinfo->apsd_cnt != WLC_APSD_USP_UNB)
            scb_psinfo->apsd_cnt--;
    }

    /* SP termination */
    if (qos) {
        pqos = (uint16 *)((uint8 *)h +
            (SCB_A4_DATA(scb) ? DOT11_A4_HDR_LEN : DOT11_A3_HDR_LEN));
        ASSERT(ISALIGNED(pqos, sizeof(*pqos)));

        /* Set EOSP if this is the last frame in the Service Period */
#ifdef WLTDLS
        /* Trigger frames are delivered in PTI interval, because
         * the the PTI response frame triggers the delivery of buffered
         * frames before PTI response is processed by TDLS module.
         * QOS null frames have WLF_PSDONTQ, why should they not terminate
         * an SP?
         */
        if (BSS_TDLS_ENAB(wlc, scb->bsscfg) &&
            (wlc_tdls_in_pti_interval(wlc->tdls, scb) ||
            (SCB_PS(scb) && (WLPKTTAG(pkt)->flags & WLF_PSDONTQ) &&
            (scb_psinfo->apsd_cnt != 0)    &&
            ((ltoh16(h->fc) & FC_KIND_MASK) != FC_QOS_NULL)))) {
            eosp = FALSE;
        }
        else
#endif
        eosp = scb_psinfo->apsd_cnt == 0 && last_frag;
        if (eosp)
            *pqos |= htol16(QOS_EOSP_MASK);
        else
            *pqos &= ~htol16(QOS_EOSP_MASK);
    }
    /* Send an extra QoS Null to terminate the USP in case
     * the MSDU doesn't have a EOSP field i.e. MMPDU.
     */
    else if (scb_psinfo->apsd_cnt == 0)
        scb_psinfo->ext_qos_null = TRUE;

    /* Register callback to end service period after this frame goes out */
    if (last_frag) {
        WLF2_PCB2_REG(pkt, WLF2_PCB2_APSD);
    }

    WL_PS_EX(scb, ("wl%d.%d: %s pkt %p qos %d more %d eosp %d cnt %d lastfrag %d\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        OSL_OBFUSCATE_BUF(pkt), qos, more, eosp,
        scb_psinfo->apsd_cnt, last_frag));
}

/* End the USP when the EOSP has gone out
 * Pkt callback fn
 */
static void
wlc_apps_apsd_complete(wlc_info_t *wlc, void *pkt, uint txs)
{
    struct scb *scb;
    struct apps_scb_psinfo *scb_psinfo;
#ifdef WLTAF
    bool taf_in_use = wlc_taf_in_use(wlc->taf_handle);
#endif

    /* Is this scb still around */
    if ((scb = WLPKTTAGSCBGET(pkt)) == NULL) {
        WL_ERROR(("%s(): scb = %p, WLPKTTAGSCBGET(pkt) = %p\n",
            __FUNCTION__, OSL_OBFUSCATE_BUF(scb),
            OSL_OBFUSCATE_BUF(WLPKTTAGSCBGET(pkt))));
        return;
    }

#ifdef BCMDBG
    /* What to do if not ack'd?  Don't want to hang in USP forever... */
    if (txs & TX_STATUS_ACK_RCV)
        WL_PS_EX(scb, ("wl%d.%d: %s delivery frame %p sent\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
    else
        WL_PS_EX(scb, ("wl%d.%d: %s delivery frame %p sent (no ACK)\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, OSL_OBFUSCATE_BUF(pkt)));
#else

    BCM_REFERENCE(txs);

#endif

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    /* Check if the PVB entry needs to be cleared */
    if (scb_psinfo->in_pvb) {
        wlc_apps_pvb_update(wlc, scb);
    }

#ifdef PROP_TXSTATUS
    scb_psinfo->apsd_tx_pending = FALSE;
#endif
    /* apsd trigger is completed */
    scb_psinfo->apsd_trig_cnt = 0;

#ifdef WLTAF
    if (taf_in_use && (wlc_apps_apsd_delv_count(wlc, scb) > 0)) {
        /* If psq is empty, release new packets from ampdu or nar */
        if ((wlc_apps_psq_delv_count(wlc, scb) == 0) &&
            !wlc_taf_scheduler_blocked(wlc->taf_handle)) {
            /* Trigger a new TAF schedule cycle */
            wlc_taf_schedule(wlc->taf_handle, PKTPRIO(pkt), scb, FALSE);
        }
    }
#endif

    /* Send more frames until the End Of Service Period */
    if (scb_psinfo->apsd_cnt > 0 || scb_psinfo->ext_qos_null) {
        if (scb_psinfo->tx_block != 0) {
            WL_PS_EX(scb, ("wl%d.%d: %s tx blocked, cnt %u\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, scb_psinfo->apsd_cnt));
            return;
        }
#ifdef PROP_TXSTATUS
        if (!scb_psinfo->apsd_hpkt_timer_on)
#endif
            wlc_apps_apsd_send(wlc, scb);
        return;
    }

    wlc_apps_apsd_usp_end(wlc, scb);
}

void
wlc_apps_scb_tx_block(wlc_info_t *wlc, struct scb *scb, uint reason, bool block)
{
    struct apps_scb_psinfo *scb_psinfo;

    ASSERT(scb != NULL);

    WL_PS_EX(scb, ("wl%d.%d: %s block %d reason %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, block, reason));

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    if (block) {
        mboolset(scb_psinfo->tx_block, reason);
        /* terminate the APSD USP */
        scb_psinfo->apsd_usp = FALSE;
        scb_psinfo->apsd_cnt = 0;
#ifdef PROP_TXSTATUS
        scb_psinfo->apsd_tx_pending = FALSE;
#endif
    } else {
        mboolclr(scb_psinfo->tx_block, reason);
    }
}

int
wlc_apps_scb_apsd_cnt(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    ASSERT(scb != NULL);

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    WL_PS_EX(scb, ("wl%d.%d: %s apsd_cnt = %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, scb_psinfo->apsd_cnt));

    return scb_psinfo->apsd_cnt;
}
#ifdef BCMPCIEDEV
/* Return the total v_pkts and v2r_pkts for the given scb */
static int
wlc_apps_sqs_vpkt_count(wlc_info_t *wlc, struct scb *scb)
{
    int tot_count;

    ASSERT(scb);

    tot_count = wlc_sqs_vpkts_tot(scb);    /* Virtual packets */
    tot_count += wlc_sqs_v2r_pkts_tot(scb);    /* Transient packets */

    return tot_count;
}
/*
 * Return the number of virtual packets  pending on delivery-enabled ACs.
 * Include both v_pkts and v2r_pkts
 */
static int
wlc_apps_apsd_delv_vpkt_count(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    int delv_count;

    if (scb->apsd.ac_delv == AC_BITMAP_NONE)
        return 0;

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    /* Virtual packets in host */
    delv_count = wlc_sqs_vpkts_multi_ac(scb,
        WLC_ACBITMAP_TO_TIDBITMAP(scb->apsd.ac_delv));

    /* Transient packets */
    delv_count += wlc_sqs_v2r_pkts_multi_ac(scb,
        WLC_ACBITMAP_TO_TIDBITMAP(scb->apsd.ac_delv));

    /* Real packets in dongle but not in psq */
    delv_count += scb_psinfo->apsd_v2r_in_transit;

    return delv_count;
}
/*
 * Return the number of virtual packets pending on non-delivery-enabled ACs.
 * Include both v_pkts and v2r_pkts
 */
static int
wlc_apps_apsd_ndelv_vpkt_count(wlc_info_t *wlc, struct scb *scb)
{
    ac_bitmap_t ac_non_delv;
    int ndelv_count;

    if (scb->apsd.ac_delv == AC_BITMAP_ALL)
        return 0;

    ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;

    /* Virtual packets in host */
    ndelv_count = wlc_sqs_vpkts_multi_ac(scb,
        WLC_ACBITMAP_TO_TIDBITMAP(ac_non_delv));

    /* Transient packets */
    ndelv_count += wlc_sqs_v2r_pkts_multi_ac(scb,
        WLC_ACBITMAP_TO_TIDBITMAP(ac_non_delv));

    return ndelv_count;
}
#endif /* BCMPCIEDEV */

/*
 * Return the number of frames pending on delivery-enabled ACs.
 */
static int
wlc_apps_apsd_delv_count(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint32 precbitmap;
    int delv_count;

    if (scb->apsd.ac_delv == AC_BITMAP_NONE)
        return 0;

    precbitmap = wlc_apps_ac2precbmp_info()[scb->apsd.ac_delv];

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    delv_count = APPS_PKTQ_MLEN(&scb_psinfo->psq, precbitmap);

    /* PS buffered pkts also need to account the packets in AMPDUq and NARq */
    delv_count += wlc_ampdu_scb_pktq_mlen(wlc->ampdu_tx, scb,
        WLC_ACBITMAP_TO_TIDBITMAP(scb->apsd.ac_delv));
#ifdef WLNAR
    delv_count += wlc_nar_scb_pktq_mlen(wlc->nar_handle, scb, precbitmap);
#endif

    return delv_count;
}

/*
 * Return the number of frames pending on non-delivery-enabled ACs.
 */
static int
wlc_apps_apsd_ndelv_count(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    ac_bitmap_t ac_non_delv;
    uint32 precbitmap;
    int count;

    if (scb->apsd.ac_delv == AC_BITMAP_ALL)
        return 0;

    ac_non_delv = ~scb->apsd.ac_delv & AC_BITMAP_ALL;
    precbitmap = wlc_apps_ac2precbmp_info()[ac_non_delv];

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    count = APPS_PKTQ_MLEN(&scb_psinfo->psq, precbitmap);

    /* PS buffered pkts also need to account the packets in AMPDUq and NARq */
    count += wlc_ampdu_scb_pktq_mlen(wlc->ampdu_tx, scb,
        WLC_ACBITMAP_TO_TIDBITMAP(ac_non_delv));
#ifdef WLNAR
    count += wlc_nar_scb_pktq_mlen(wlc->nar_handle, scb, precbitmap);
#endif

    return count;
}

uint8
wlc_apps_apsd_ac_available(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint32 precbitmap;
    uint8 ac_bitmap = 0;
    struct pktq* q;            /**< multi-priority packet queue */

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    q = &scb_psinfo->psq;

    precbitmap = wlc_apps_ac2precbmp_info()[(1 << AC_BK)];
    if (APPS_PKTQ_MLEN(q, precbitmap))
        ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_BK;

    precbitmap = wlc_apps_ac2precbmp_info()[(1 << AC_BE)];
    if (APPS_PKTQ_MLEN(q, precbitmap))
        ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_BE;

    precbitmap = wlc_apps_ac2precbmp_info()[(1 << AC_VO)];
    if (APPS_PKTQ_MLEN(q, precbitmap))
        ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_VO;

    precbitmap = wlc_apps_ac2precbmp_info()[(1 << AC_VI)];
    if (APPS_PKTQ_MLEN(q, precbitmap))
        ac_bitmap |= TDLS_PU_BUFFER_STATUS_AC_VI;

    return ac_bitmap;
}

/* periodically check whether BC/MC queue needs to be flushed */
static void
wlc_apps_bss_wd_ps_check(void *handle)
{
    wlc_info_t *wlc = (wlc_info_t *)handle;
    struct scb *bcmc_scb;
    wlc_bsscfg_t *bsscfg;
    apps_bss_info_t *bss_info;
    uint i;

    if (wlc->excursion_active) {
        return;
    }

    if (wlc->clk) {
        wlc_apps_omi_waitpmq_war(wlc);
    }
    FOREACH_AP(wlc, i, bsscfg) {
        if (!wlc->excursion_active && BSSCFG_HAS_BCMC_SCB(bsscfg)) {
            bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
            ASSERT(bcmc_scb != NULL);
            ASSERT(bcmc_scb->bsscfg == bsscfg);

            bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
            ASSERT(bss_info);
            if ((SCB_PS(bcmc_scb) == TRUE) && (TXPKTPENDGET(wlc, TX_BCMC_FIFO) == 0) &&
                (!(bss_info->ps_trans_status & BSS_PS_ON_BY_TWT))) {
                if (MBSS_ENAB(wlc->pub)) {
                    if (bsscfg->bcmc_fid_shm != INVALIDFID) {
                        WL_ERROR(("wl%d.%d: %s cfg(%p) bcmc_fid = 0x%x"
                            " bcmc_fid_shm = 0x%x, resetting bcmc_fids"
                            " tot pend %d mc_pkts %d\n",
                            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg),
                            __FUNCTION__, bsscfg, bsscfg->bcmc_fid,
                            bsscfg->bcmc_fid_shm,
                            TXPKTPENDTOT(wlc),
                            wlc_mbss_get_bcmc_pkts_sent(wlc, bsscfg)));
                    }
                    /* Reset the BCMC FIDs, no more pending bcmc packets */
                    wlc_mbss_bcmc_reset(wlc, bsscfg);
                } else {
                    BCMCFID(wlc, INVALIDFID);
                    bcmc_scb->PS = FALSE;
#ifdef WL_PS_STATS
                    wlc_apps_upd_pstime(bcmc_scb);
#endif /* WL_PS_STATS */
                }
            }
        }
    }
}

#if defined(MBSS)

/* Enqueue a BC/MC packet onto it's BSS's PS queue */
int
wlc_apps_bcmc_ps_enqueue(wlc_info_t *wlc, struct scb *bcmc_scb, void *pkt)
{
    struct apps_scb_psinfo *scb_psinfo;
    apps_bss_info_t *bss_info;
#ifdef PKTQ_LOG
    struct pktq *q = NULL;
    pktq_counters_t* prec_cnt = NULL;
#endif

    scb_psinfo = SCB_PSINFO(wlc->psinfo, bcmc_scb);
    ASSERT(scb_psinfo);
    bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bcmc_scb->bsscfg);

#ifdef PKTQ_LOG
    q = &scb_psinfo->psq;

    if (q->pktqlog) {
        prec_cnt = q->pktqlog->_prec_cnt[(MAXPRIO)];

        /* check for auto enabled logging */
        if (prec_cnt == NULL && (q->pktqlog->_prec_log & PKTQ_LOG_AUTO)) {
            prec_cnt = wlc_txq_prec_log_enable(wlc, q, (uint32)(MAXPRIO), FALSE);

            if (prec_cnt) {
                wlc_read_tsf(wlc, &prec_cnt->_logtimelo, &prec_cnt->_logtimehi);
            }
        }
    }
    WLCNTCONDINCR(prec_cnt, prec_cnt->requested);
#endif /* PKTQ_LOG */

    /* Check that packet queue length is not exceeded */
    if (pktq_full(&scb_psinfo->psq) || pktqprec_full(&scb_psinfo->psq, MAXPRIO)) {
#ifdef PKTQ_LOG
        WLCNTCONDINCR(prec_cnt, prec_cnt->dropped);
#endif
        WL_NONE(("wlc_apps_bcmc_ps_enqueue: queue full.\n"));
        WLCNTINCR(bss_info->bcmc_discards);
        return BCME_ERROR;
    }
    (void)pktq_penq(&scb_psinfo->psq, MAXPRIO, pkt);
    WLCNTINCR(bss_info->bcmc_pkts_seen);
#ifdef PKTQ_LOG
    if (prec_cnt) {
        uint32 qlen = pktqprec_n_pkts(q, MAXPRIO);
        uint32* max_used = &prec_cnt->max_used;

        WLCNTINCR(prec_cnt->stored);

        if (qlen > *max_used) {
            *max_used = qlen;
        }
    }
#endif
    return BCME_OK;
}

/* Last STA has gone out of PS.  Check state of its BSS */

static void
wlc_apps_bss_ps_off_start(wlc_info_t *wlc, struct scb *bcmc_scb)
{
    wlc_bsscfg_t *bsscfg;

    bsscfg = bcmc_scb->bsscfg;
    ASSERT(bsscfg != NULL);

    if (!BCMC_PKTS_QUEUED(bsscfg)) {
        /* No pkts in BCMC fifo */
        wlc_apps_bss_ps_off_done(wlc, bsscfg);
    } else { /* Mark in transition */
        ASSERT(bcmc_scb->PS); /* Should only have BCMC pkts if in PS */
        bsscfg->flags |= WLC_BSSCFG_PS_OFF_TRANS;
        WL_PS_EX(bcmc_scb, ("wl%d.%d: %s START PS-OFF. last fid 0x%x. shm fid 0x%x\n",
            wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg), __FUNCTION__,
            bsscfg->bcmc_fid, bsscfg->bcmc_fid_shm));
#if defined(BCMDBG_MBSS_PROFILE) /* Start transition timing */
        if (bsscfg->ps_start_us == 0) {
            bsscfg->ps_start_us = R_REG(wlc->osh, D11_TSFTimerLow(wlc));
        }
#endif /* BCMDBG_MBSS_PROFILE */
    }
}

/*
 * Due to a STA transitioning to PS on, all packets have been drained from the
 * data fifos.  Update PS state of all BSSs (if not in PS-OFF transition).
 *
 * Note that it's possible that a STA has come out of PS mode during the
 * transition, so we may return to PS-OFF (abort the transition).  Since we
 * don't keep state of which STA and which BSS started the transition, we
 * simply check them all.
 */

void
wlc_apps_bss_ps_on_done(wlc_info_t *wlc)
{
    wlc_bsscfg_t *bsscfg;
    apps_bss_info_t *bss_info;
    struct scb *bcmc_scb;
    int i;

    FOREACH_UP_AP(wlc, i, bsscfg) {
        if (!(bsscfg->flags & WLC_BSSCFG_PS_OFF_TRANS)) { /* Ignore BSS in PS-OFF trans */
            bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg);
            bss_info = APPS_BSSCFG_CUBBY(wlc->psinfo, bsscfg);
            if ((bss_info->ps_nodes) ||
                (bss_info->ps_trans_status & BSS_PS_ON_BY_TWT)) {
#if defined(MBSS)
                if (!SCB_PS(bcmc_scb) && MBSS_ENAB(wlc->pub)) {
                    /* PS off, MC pkts to data fifo should be cleared */
                    ASSERT(wlc_mbss_get_bcmc_pkts_sent(wlc, bsscfg) == 0);
                    wlc_mbss_increment_ps_trans_cnt(wlc, bsscfg);
                    WL_NONE(("wl%d.%d: DONE PS-ON\n",
                        wlc->pub->unit, WLC_BSSCFG_IDX(bsscfg)));
                }
#endif
                bcmc_scb->PS = TRUE;
#ifdef WL_PS_STATS
                if (bcmc_scb->ps_starttime == 0) {
                    bcmc_scb->ps_on_cnt++;
                    bcmc_scb->ps_starttime = OSL_SYSUPTIME();
                }
#endif /* WL_PS_STATS */
            } else { /* Unaffected BSS or transition aborted for this BSS */
                bcmc_scb->PS = FALSE;
#ifdef WL_PS_STATS
                wlc_apps_upd_pstime(bcmc_scb);
#endif /* WL_PS_STATS */
#ifdef PSPRETEND
                bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */
            }
        }
    }
}

static INLINE void
_wlc_apps_bss_ps_off_done(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
#if defined(BCMDBG_MBSS_PROFILE)
    if (bsscfg->ps_start_us != 0) {
        uint32 diff_us;

        diff_us = R_REG(wlc->osh, D11_TSFTimerLow(wlc)) - bsscfg->ps_start_us;
        if (diff_us > bsscfg->max_ps_off_us)
            bsscfg->max_ps_off_us = diff_us;
        bsscfg->tot_ps_off_us += diff_us;
        bsscfg->ps_off_count++;
        bsscfg->ps_start_us = 0;
    }
#endif /* BCMDBG_MBSS_PROFILE */
}

/*
 * Last STA for a BSS exitted PS; BSS has no pkts in BC/MC fifo.
 * Check whether other stations have entered PS since and update
 * state accordingly.
 *
 * That is, it is possible that the BSS state will remain PS
 * TRUE (PS delivery mode enabled) if a STA has changed to PS-ON
 * since the start of the PS-OFF transition.
 */

void
wlc_apps_bss_ps_off_done(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg)
{
    struct scb *bcmc_scb;

    ASSERT(bsscfg->bcmc_fid_shm == INVALIDFID);
    ASSERT(bsscfg->bcmc_fid == INVALIDFID);

    if (!(bcmc_scb = WLC_BCMCSCB_GET(wlc, bsscfg))) {
        bsscfg->flags &= ~WLC_BSSCFG_PS_OFF_TRANS; /* Clear transition flag */
        return;
    }

    ASSERT(SCB_PS(bcmc_scb));

    if (BSS_PS_NODES(wlc->psinfo, bsscfg) != 0) {
        /* Aborted transtion:  Set PS delivery mode */
        bcmc_scb->PS = TRUE;
#ifdef WL_PS_STATS
        if (bcmc_scb->ps_starttime == 0) {
            bcmc_scb->ps_on_cnt++;
            bcmc_scb->ps_starttime = OSL_SYSUPTIME();
        }
#endif /* WL_PS_STATS */
    } else { /* Completed transition: Clear PS delivery mode */
        bcmc_scb->PS = FALSE;
#ifdef WL_PS_STATS
        wlc_apps_upd_pstime(bcmc_scb);
#endif /* WL_PS_STATS */
#ifdef PSPRETEND
        bcmc_scb->ps_pretend &= ~PS_PRETEND_ON;
#endif /* PSPRETEND */

#ifdef MBSS
        wlc_mbss_increment_ps_trans_cnt(wlc, bsscfg);
#endif
        if (bsscfg->flags & WLC_BSSCFG_PS_OFF_TRANS) {
            WL_PS_EX(bcmc_scb, ("wl%d.%d: %s DONE PS-OFF.\n", wlc->pub->unit,
                WLC_BSSCFG_IDX(bsscfg), __FUNCTION__));
        }
    }
    bsscfg->flags &= ~WLC_BSSCFG_PS_OFF_TRANS; /* Clear transition flag */
#ifdef HNDPQP
    /* Page in all Host resident packets into dongle */
    wlc_apps_scb_pqp_pgi(wlc, bcmc_scb, WLC_PREC_BMP_ALL, PKTQ_PKTS_ALL, PQP_PGI_PS_BCMC_OFF);
#endif  /* HNDPQP */

    /* Forward any packets in MC-PSQ according to new state */
    while (wlc_apps_ps_send(wlc, bcmc_scb, WLC_PREC_BMP_ALL, 0));

#ifdef HNDPQP
    /* If there are still pkts in host, it means PQP is out of resource.
     * PQP will set flags for current PS transition and
     * resume the remain process when resource is available.
     */
    if (PQP_PGI_PS_TRANS_OFF_ISSET(SCB_PSINFO(wlc->psinfo, bcmc_scb)))
        return;
#endif /* HNDPQP */

    _wlc_apps_bss_ps_off_done(wlc, bsscfg);
}

#endif /* MBSS */

/*
 * Return the bitmap of ACs with buffered traffic.
 */
uint8
wlc_apps_apsd_ac_buffer_status(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint32 precbitmap;
    uint8 ac_bitmap = 0;
    struct pktq *q;            /**< multi-priority packet queue */
    int i;

    /* PS buffered pkts are on the scb_psinfo->psq */
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    q = &scb_psinfo->psq;

    for (i = 0; i < AC_COUNT; i++) {
        precbitmap = wlc_apps_ac2precbmp_info()[(uint32)(1 << i)];

        if (APPS_PKTQ_MLEN(q, precbitmap))
            AC_BITMAP_SET(ac_bitmap, i);
    }

    return ac_bitmap;
}

/* TIM */
static uint
wlc_apps_calc_tim_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
    wlc_info_t *wlc = (wlc_info_t *)ctx;
    wlc_bsscfg_t *cfg = data->cfg;

    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && !BSSCFG_IS_MAIN_AP(cfg)) {
        return 0;
    }

    if (BSSCFG_AP(cfg))
        return wlc_apps_tim_len(wlc, cfg);

    return 0;
}

static int
wlc_apps_write_tim_ie(void *ctx, wlc_iem_build_data_t *data)
{
    wlc_info_t *wlc = (wlc_info_t *)ctx;
    wlc_bsscfg_t *cfg = data->cfg;

    if (MBSSID_ENAB(wlc->pub, wlc->band->bandtype) && !BSSCFG_IS_MAIN_AP(cfg)) {
        return BCME_ERROR;
    }
    if (BSSCFG_AP(cfg)) {
        wlc_apps_tim_create(wlc, cfg, data->buf, data->buf_len);

        data->cbparm->ft->bcn.tim_ie = data->buf;

#ifdef MBSS
        if (MBSS_ENAB(wlc->pub)) {
            wlc_mbss_set_bcn_tim_ie(wlc, cfg, data->buf);
        }
#endif /* MBSS */
    }

    return BCME_OK;
}

static void
wlc_apps_scb_state_upd_cb(void *ctx, scb_state_upd_data_t *notif_data)
{
    wlc_info_t *wlc = (wlc_info_t *)ctx;
    struct scb *scb;
    uint8 oldstate;

    ASSERT(notif_data != NULL);

    scb = notif_data->scb;
    ASSERT(scb != NULL);
    oldstate = notif_data->oldstate;

    if (BSSCFG_AP(scb->bsscfg) && (oldstate & ASSOCIATED) && !SCB_ASSOCIATED(scb)) {
        if (SCB_PS(scb) && !SCB_ISMULTI(scb)) {
            WL_PS_EX(scb, ("%s: SCB disassociated, take it out of PS\n", __FUNCTION__));
            wlc_apps_scb_ps_off(wlc, scb, TRUE);
        }
    }
}

static void
wlc_apps_bss_updn(void *ctx, bsscfg_up_down_event_data_t *evt)
{
    wlc_bsscfg_t *bsscfg = evt->bsscfg;
    wlc_info_t *wlc = bsscfg->wlc;
    struct scb *scb;
    struct scb_iter scbiter;
    struct apps_scb_psinfo *scb_psinfo;

    if (!evt->up) {
        FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
            scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

            if (scb_psinfo == NULL)
                continue;

            if (SCB_PS(scb) && !SCB_ISMULTI(scb))
                wlc_apps_scb_ps_off(wlc, scb, TRUE);
            else if (!pktq_empty(&scb_psinfo->psq))
                wlc_apps_ps_flush(wlc, scb);
        }

        if (BSSCFG_HAS_BCMC_SCB(bsscfg)) {
            wlc_apps_ps_flush(wlc, WLC_BCMCSCB_GET(wlc, bsscfg));
            if (MBSS_ENAB(wlc->pub)) {
                wlc_mbss_bcmc_reset(wlc, bsscfg);
            }
        }
    }
}

void
wlc_apps_ps_trans_upd(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

#if defined(WL_PS_SCB_TXFIFO_BLK)
    ASSERT(SCB_PS_TXFIFO_BLK(scb));
#else /* ! WL_PS_SCB_TXFIFO_BLK */
    ASSERT(wlc->block_datafifo & DATA_BLOCK_PS);
#endif /* ! WL_PS_SCB_TXFIFO_BLK */

    /* We have received an ack in pretend state and are free to exit */
    WL_PS_EX(scb, ("wl%d.%d: %s "MACF" received successful txstatus in threshold "
        "ps pretend active state\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
        __FUNCTION__, ETHER_TO_MACF(scb->ea)));
    ASSERT(scb->PS);
    scb_psinfo->ps_trans_status |= SCB_PS_TRANS_OFF_PEND;
}

/** @return   Multi-priority packet queue */
struct pktq *
wlc_apps_get_psq(wlc_info_t * wlc, struct scb * scb)
{
    if ((wlc == NULL) ||
        (scb == NULL) ||
        (SCB_PSINFO(wlc->psinfo, scb) == NULL)) {
        return NULL;
    }

    return &SCB_PSINFO(wlc->psinfo, scb)->psq;
}

void
wlc_apps_map_pkts(wlc_info_t *wlc, struct scb *scb, map_pkts_cb_fn cb, void *ctx)
{
    struct pktq *pktq;        /**< multi-priority packet queue */

#ifdef WL_USE_SUPR_PSQ
    /* supr_psq packets should be prepended to psq before psq packets was freed. */
    if ((scb != NULL) && (SCB_PSINFO(wlc->psinfo, scb) != NULL) &&
        (SCB_PSINFO(wlc->psinfo, scb)->suppressed_pkts > 0)) {
        wlc_apps_dequeue_scb_supr_psq(wlc, scb);
    }
#endif /* WL_USE_SUPR_PSQ */

#ifdef HNDPQP
    /* Page in all Host resident packets into dongle */
    wlc_apps_scb_pqp_map_pkts(wlc, scb, cb, ctx);
    return;
#endif /* HNDPQP */

    if ((pktq = wlc_apps_get_psq(wlc, scb)))
        wlc_scb_psq_map_pkts(wlc, pktq, cb, ctx);
}

void
wlc_apps_set_change_scb_state(wlc_info_t *wlc, struct scb *scb, bool reset)
{
    apps_wlc_psinfo_t *wlc_psinfo;
    struct apps_scb_psinfo *scb_psinfo;
    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    scb_psinfo->change_scb_state_to_auth = reset;
}

#ifdef WL_PS_STATS
void
wlc_apps_upd_pstime(struct scb *scb)
{
    uint32 ps_delta;
    uint32 ps_now = OSL_SYSUPTIME();
    /* in case ps_starttime was cleared */
    if (scb->ps_starttime == 0) {
        scb->ps_on_cnt = 0;
    } else {
        ps_delta = ps_now - scb->ps_starttime;
        if (ps_delta > scb->ps_maxtime)
            scb->ps_maxtime = ps_delta;

        if ((scb->ps_mintime == 0) || (scb->ps_mintime > ps_delta))
            scb->ps_mintime = ps_delta;

        /* Update STA PS duration histogram */
        PS_UPD_HISTOGRAM(ps_delta, scb);

        /* Update total duraion */
        scb->ps_tottime += ps_delta;
        scb->ps_starttime = 0;
    }
}

static int
wlc_apps_stats_dump(void *ctx, struct bcmstrbuf *b)
{
    int i, j, k;
    wlc_info_t *wlc = ctx;
    int32 ps_cnt;
    int32 datablock_cnts;
    struct apps_scb_psinfo *scb_psinfo;
    struct scb *scb;
    struct scb_iter scbiter;
    wlc_bsscfg_t *bsscfg;

    if ((wlc->block_datafifo) && (wlc->datablock_cnt > 0))
        datablock_cnts = wlc->datablock_cnt - 1;
    else
        datablock_cnts = wlc->datablock_cnt;

    if (datablock_cnts > 0) {
        bcm_bprintf(b, "data_block_ps %d cnt %d dur avg:%u (min:%u max:%u) usec, "
            "pktpend avg:%u (min:%u max:%u), ",
            wlc->block_datafifo, wlc->datablock_cnt,
            wlc->datablock_tottime/(uint32)datablock_cnts,
            wlc->datablock_mintime, wlc->datablock_maxtime,
            wlc->pktpend_tot/(uint32)datablock_cnts,
            wlc->pktpend_min, wlc->pktpend_max);
    } else {
        bcm_bprintf(b, "data_block_ps 0 cnt 0 avg:0 (min:0 max:0) usec, "
            "pktpend avg:0 (min:0 max:0), ");
    }
#if defined(WL_PS_SCB_TXFIFO_BLK)
    bcm_bprintf(b, "scb_blk %d cnt %d ",
        wlc->ps_scb_txfifo_blk, wlc->ps_txfifo_blk_scb_cnt);
#endif /* WL_PS_SCB_TXFIFO_BLK */
    bcm_bprintf(b, "\n");

    /* Look through all SCBs and display relevant PS info */
    i = j = k = 0;  /* i=BSSs, j=BSS.SCBs, k=total scbs */
    bcm_bprintf(b, "SCB: hist bins: [0..200ms 200ms..500ms 500ms..2sec 2sec..5sec 5sec..]\n");
    FOREACH_BSS(wlc, i, bsscfg) {
        j = 0;
        FOREACH_BSS_SCB(wlc->scbstate, &scbiter, bsscfg, scb) {
            if ((scb->PS || scb->PS_TWT) && (scb->ps_on_cnt > 0))
                ps_cnt = scb->ps_on_cnt - 1;
            else
                ps_cnt = scb->ps_on_cnt;

            if (ps_cnt > 0) {
                bcm_bprintf(b, " %2d.%3d "MACF"%s ps %d ps_on %d dur avg:%u "
                    "(min:%u max:%u) msec, hist:[%u %u %u %u %u], supr %u",
                    i, j, ETHER_TO_MACF(scb->ea), (scb->permanent ? "*":" "),
                    (scb->PS || scb->PS_TWT), scb->ps_on_cnt,
                    scb->ps_tottime/(uint32)ps_cnt,
                    scb->ps_mintime, scb->ps_maxtime,
                    scb->ps_dur0_200, scb->ps_dur200_500, scb->ps_dur500_2,
                    scb->ps_dur2_5, scb->ps_dur5, scb->suprps_cnt);
            } else {
                bcm_bprintf(b, " %2d.%3d "MACF"%s ps %d ps_on 0 dur avg:0 "
                    "(min:0 max:0) msec, hist:[0 0 0 0 0], supr %u",
                    i, j, ETHER_TO_MACF(scb->ea), (scb->permanent ? "*":" "),
                    (scb->PS || scb->PS_TWT), scb->suprps_cnt);
            }

            if (scb->PS) {
                scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
                ps_cnt = pktq_n_pkts_tot(&scb_psinfo->psq);

                bcm_bprintf(b, " psqlen %u\n", ps_cnt);
            } else {
                bcm_bprintf(b, "\n");
            }
            j++;
            k++;
        }
    }

    if (k == 0) {
        bcm_bprintf(b, " %s\n", "No SCBs");
    }

    bcm_bprintf(b, "BSSCFG: \n");
    FOREACH_UP_AP(wlc, i, bsscfg) {
        scb = WLC_BCMCSCB_GET(wlc, bsscfg);
        if (scb->PS && (scb->ps_on_cnt > 0))
            ps_cnt = scb->ps_on_cnt - 1;
        else
            ps_cnt = scb->ps_on_cnt;

        if (ps_cnt > 0) {
            bcm_bprintf(b, " "MACF" ps %d ps_on %d dur avg:%u (min:%u max:%u) msec, "
                "bcmc_supr %u",
                ETHER_TO_MACF(bsscfg->BSSID), scb->PS, scb->ps_on_cnt,
                scb->ps_tottime/(uint32)ps_cnt,
                scb->ps_mintime, scb->ps_maxtime, scb->suprps_cnt);
        } else {
            bcm_bprintf(b, " "MACF" ps %d ps_on 0 dur avg:0 (min:0 max:0) msec, "
                "bcmc_supr %u",
                ETHER_TO_MACF(bsscfg->BSSID), scb->PS, scb->suprps_cnt);
        }
        bcm_bprintf(b, "\n");
    }
    return BCME_OK;
}

static int
wlc_apps_stats_dump_clr(void *ctx)
{
    wlc_info_t *wlc = ctx;
    struct scb *scb;
    struct scb_iter scbiter;
    wlc_bsscfg_t *bsscfg;
    int i;

    FOREACHSCB(wlc->scbstate, &scbiter, scb) {
        scb->ps_on_cnt = 0;
        scb->ps_mintime = 0;
        scb->ps_maxtime = 0;
        scb->ps_tottime = 0;
        scb->ps_starttime = 0;
        scb->suprps_cnt = 0;
        scb->ps_dur0_200 = 0;
        scb->ps_dur200_500 = 0;
        scb->ps_dur500_2 = 0;
        scb->ps_dur2_5 = 0;
        scb->ps_dur5 = 0;
    }

    FOREACH_UP_AP(wlc, i, bsscfg) {
        scb = WLC_BCMCSCB_GET(wlc, bsscfg);
        scb->ps_on_cnt = 0;
        scb->ps_mintime = 0;
        scb->ps_maxtime = 0;
        scb->ps_tottime = 0;
        scb->ps_starttime = 0;
        scb->suprps_cnt = 0;
        scb->ps_dur0_200 = 0;
        scb->ps_dur200_500 = 0;
        scb->ps_dur500_2 = 0;
        scb->ps_dur2_5 = 0;
        scb->ps_dur5 = 0;
    }
    wlc->datablock_starttime = 0;
    wlc->datablock_cnt = 0;
    wlc->datablock_mintime = 0;
    wlc->datablock_maxtime = 0;
    wlc->datablock_tottime = 0;
    wlc->pktpend_min = 0;
    wlc->pktpend_max = 0;
    wlc->pktpend_tot = 0;

    return BCME_OK;
}
#endif /* WL_PS_STATS */

#ifdef WLCFP
#if defined(AP) && defined(BCMPCIEDEV)
void
wlc_apps_scb_psq_resp(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    _wlc_apps_scb_psq_resp(wlc, scb, scb_psinfo);
}

void
wlc_apps_scb_apsd_tx_pending(wlc_info_t *wlc, struct scb *scb, uint32 extra_flags)
{
    struct apps_wlc_psinfo *wlc_psinfo;
    struct apps_scb_psinfo *scb_psinfo;

    wlc_psinfo = wlc->psinfo;
    scb_psinfo = SCB_PSINFO(wlc_psinfo, scb);
    ASSERT(scb_psinfo);

    if (extra_flags & WLF_APSD)
        scb_psinfo->apsd_tx_pending = TRUE;
}
#endif /* AP && PCIEDEV */

void
wlc_cfp_apps_ps_send(wlc_info_t *wlc, struct scb *scb, void *pkt, uint8 prio)
{
    wlc_txq_info_t *qi = SCB_WLCIFP(scb)->qi;

#ifdef BCMPCIEDEV
    wlc_apps_scb_apsd_dec_v2r_in_transit(wlc, scb, pkt, WLC_PRIO_TO_PREC(prio));
#endif /* BCMPCIEDEV */

    /* register WLF2_PCB2_PSP_RSP for pkt */
    WLF2_PCB2_REG(pkt, WLF2_PCB2_PSP_RSP);

    /* Ensure the pkt marker (used for ageing) is cleared */
    WLPKTTAG(pkt)->flags &= ~WLF_PSMARK;

    WL_PS_EX(scb, ("wl%d.%d: ps_enq_resp %p supr %d apsd %d\n",
           wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), OSL_OBFUSCATE_BUF(pkt),
           (WLPKTTAG(pkt)->flags & WLF_TXHDR) ? 1 : 0,
           (WLPKTTAG(pkt)->flags & WLF_APSD) ? 1 : 0));

    /* Enqueue the PS-Poll response at higher precedence level */
    if (!cpktq_prec_enq(wlc, &qi->cpktq, pkt, WLC_PRIO_TO_HI_PREC(prio), FALSE)) {
        WL_ERROR(("wl%d.%d: %s: txq full, frame discarded\n",
                  wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__));
        PKTFREE(wlc->osh, pkt, TRUE);
        wlc_apps_apsd_usp_end(wlc, scb);
        return;
    }

    /* Send to hardware (latency for first APSD-delivered frame is especially important) */
    wlc_send_q(wlc, qi);
}
#endif /* WLCFP */

/* Return avaiable real packets in the system for the given flow */
static uint16
wlc_apps_scb_pktq_tot(wlc_info_t *wlc, struct scb *scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    int pktq_total;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    ASSERT(scb_psinfo);

    /* get the count of pkts buffered for the scb */
    pktq_total = pktq_n_pkts_tot(&scb_psinfo->psq);

#ifdef WL_USE_SUPR_PSQ
    pktq_total += scb_psinfo->suppressed_pkts;
#endif

    /* PS buffered pkts also need to account the packets in AMPDUq and NARq */
    pktq_total += wlc_ampdu_scb_txpktcnt(wlc->ampdu_tx, scb);
#ifdef WLNAR
    pktq_total += wlc_nar_scb_txpktcnt(wlc->nar_handle, scb);
#endif

    return pktq_total;
}

#if defined(HNDPQP)
/* PQP integration for APPS module. */

/* Check if a given scb->psq:prec is owned by PQP */
bool
wlc_apps_scb_pqp_owns(wlc_info_t* wlc, struct scb* scb, uint16 prec_bmp)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    pq;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return FALSE;

    pq = &scb_psinfo->psq;

    ASSERT(pq);

    return (pktq_mpqp_owns(pq, prec_bmp));
}
/* PQP Page out callback called on every successfull page out completion */
static int
wlc_apps_pqp_pgo_cb(pqp_cb_t* cb)
{
    void* pkt;
    pkt = (void*)cb->pqp_pkt;

    /* Free the dongle Packet now */
    ASSERT(pkt);

#if defined(PQP_USE_MGMT_LOCAL)
    /* Do not free mgmt local buffer */
    if (PKTISMGMTTXPKT(cb->osh, pkt))
        return 0;
#endif

    /* PQP specific PKTFREE where all packet callbacks are skipped */
    PQP_PKTFREE(cb->osh, pkt);

    return 0;
}

/* Calcaulte the pktq prec */
static INLINE uint16
wlc_apps_pqp_psq_ptr2idx(struct pktq *psq, void *pktq_ptr)
{
    uint32 pktq_idx = (uint32)
        (((uintptr)pktq_ptr - (uintptr)psq) / sizeof(pktq_prec_t));
    ASSERT(pktq_idx < WLC_PREC_COUNT);
    return pktq_idx;
}

/* PQP Page IN callback called on page in of requested packets */
static int
wlc_apps_pqp_pgi_cb(pqp_cb_t* cb)
{
    wlc_info_t *wlc;
    struct scb *scb;
    struct apps_scb_psinfo *scb_psinfo;
    wlc_bsscfg_t *bsscfg;
    struct pktq *pq;
    pqp_pktq_t  *pqp_pktq;
    uint32 extra_flag;
    int prec;
    uint prec_bmp;

    scb_psinfo = (struct apps_scb_psinfo *)cb->ctx;
    ASSERT(scb_psinfo);
    pqp_pktq = cb->pqp_pktq;

    wlc = scb_psinfo->wlc;
    scb = scb_psinfo->scb;
    pq = &scb_psinfo->psq;
    bsscfg = SCB_BSSCFG(scb);
    ASSERT(bsscfg != NULL);

    if (PQP_PGI_PS_TRANS(scb_psinfo) == 0) {
        return 0;
    }

    extra_flag = PQP_PGI_PS_EXTRAFLAG(scb_psinfo);
    prec = wlc_apps_pqp_psq_ptr2idx(pq, pqp_pktq);
    prec_bmp = (1 << prec);

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d  prec %d ps_trans 0x%x\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), prec, PQP_PGI_PS_TRANS(scb_psinfo)));

    /* pq->hi_prec will be changed in pktq_mdeq(),
     * If last pktq_pqp_pgi() didn't page-in all pkts in pktq and
     * then called wlc_apps_ps_send(). pq->hi_prec will be set to 0.
     * hi_prec should be re-assigned before pktq_mdeq().
     * Set hi_prec start from prec.
     */
    if (pq->hi_prec < prec)
        pq->hi_prec = prec;

    if (PQP_PGI_PS_SEND_ISSET(scb_psinfo)) {
        PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

        wlc_apps_ps_send(wlc, scb, prec_bmp, extra_flag);

        return 0;
    } else if (PQP_PGI_PS_TRANS_OFF_ISSET(scb_psinfo)) {
        while (wlc_apps_ps_send(wlc, scb, prec_bmp, extra_flag));
    }

    if (!pktq_mpqp_owns(pq, PQP_PGI_PS_PRECMAP(scb_psinfo))) {
        WL_PS_EX(scb, ("%s: pgi_ps_trans 0x%x finished\n", __FUNCTION__,
            PQP_PGI_PS_TRANS(scb_psinfo)));

        switch (PQP_PGI_PS_TRANS(scb_psinfo)) {
        case PQP_PGI_PS_OFF:
            _wlc_apps_scb_ps_off(wlc, scb, scb_psinfo, bsscfg);
            break;
        case PQP_PGI_PS_TWT_OFF:
            _wlc_apps_twt_sp_release_ps(wlc, scb);
            break;
        case PQP_PGI_PS_BCMC_OFF:
            _wlc_apps_bss_ps_off_done(wlc, bsscfg);
            break;
        default:
            ASSERT(0);
            break;
        }

        PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);
    }

    return 0;
}

void
wlc_apps_scb_pqp_pgi(wlc_info_t* wlc, struct scb* scb, uint16 prec_bmp, int fill_pkts,
    uint16 ps_trans)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    pq;
    int prec;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (!scb_psinfo)
        return;

    pq = &scb_psinfo->psq;
    if (!pq)
        return;

    if (!pktq_mpqp_owns(pq, prec_bmp)) {
        return;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d  prec bmp 0x%x ps_trans 0x%x\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), prec_bmp, ps_trans));

    /* Reset previous PGI flags for PS transition */
    PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

    PKTQ_PREC_ITER(pq, prec) {
        if ((prec_bmp & (1 << prec)) == 0)
            continue;

        /* PQP Page in all the packets for a given prec */
        /* If PGI resource is out, it will resume when resource is avaiable.
         * Set cont_pkts to 1 to explicitly invoke QCB to
         * free more resource for next resource resume callback.
         */
        pktq_pqp_pgi(pq, prec, 1, fill_pkts);
    }

    /* Expected to page in the full queue. Check the status */
    if (pktq_mpqp_owns(pq, prec_bmp)) {
        WL_PS_EX(scb, ("%s : PQP PGI waiting for resources : SCB flow: %d "
            "prec 0x%x ::: <Called from 0x%p>\n",
            __FUNCTION__, SCB_FLOWID(scb), prec_bmp, CALL_SITE));

        /* If PQP PGI wait for resource then set flag to resume the ps process.
         * TODO: handle the previous PGI ps trans if it is not completed.
         */
        PQP_PGI_PS_TRANS_PEND_SET(scb_psinfo, ps_trans, prec_bmp, 0);
    }

    return;
}

#define PQP_PGI_PS_POLLLOOP    1000

/* Page In regular PS Q for a given prec and flush pkts */
static uint32
wlc_apps_scb_pqp_ps_flush_prec(wlc_info_t *wlc, struct scb *scb, struct pktq *pq, int prec)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint32 n_pkts_flushed = 0;
    uint32 loop_count = PQP_PGI_PS_POLLLOOP;
    bool retry;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (!scb_psinfo)
        return 0;

    /* Reset previous PGI flags for PS transition */
    PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

    if (pktq_pqp_pkt_cnt(pq, prec) == 0) {
        return 0;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d prec 0x%x n_pkts %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), prec, pktqprec_n_pkts(pq, prec)));

    /* Set flag to use reserved buf for page in */
    pqp_pgi_rsv_buf(TRUE);

pgi_retry:
    retry = FALSE;

    /* PQP Page in all the packets for this prec */
    pktq_pqp_pgi(pq, prec, PKTQ_PKTS_ALL, PKTQ_PKTS_ALL);

    if (pktq_pqp_owns(pq, prec)) {
        if (pktqprec_n_pkts(pq, prec) == 0) {
            WL_ERROR(("wl%d.%d: %s SCB flowid: %d PGI 0 pkts\n",
                wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                __FUNCTION__, SCB_FLOWID(scb)));
#ifdef HWA_PKTPGR_BUILD
            // SHARED DBM is not enough, use RESV DBM to fill pqplbuf pool
            hwa_pktpgr_pqplbufpool_rsv_ddbm_fill(WL_HWA_DEVP(wlc));
            goto pgi_retry;
#else  /* !HWA_PKTPGR_BUILD */
            ASSERT(0);
            return 0;
#endif /* !HWA_PKTPGR_BUILD */
        }
        retry = TRUE;
    }

    if (!pktqprec_empty(pq, prec)) {
        n_pkts_flushed += wlc_txq_pktq_pflush(wlc, pq, prec);
    }

    /* Retry PGI if there are pkts in host. */
    if (retry) {
        loop_count--;
        ASSERT(loop_count);
        goto pgi_retry;
    }

    /* Clear flag to use reserved buf */
    pqp_pgi_rsv_buf(FALSE);

    return n_pkts_flushed;
}

/* Page In regular PS Q to execute map_pkts_cb_fn */
static void
wlc_apps_scb_pqp_map_pkts(wlc_info_t *wlc, struct scb *scb, map_pkts_cb_fn cb, void *ctx)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq *psq;
    struct spktq map_psq;
    struct pktq_prec *q_A, *q_B; /**< single precedence packet queue */
    uint16 n_pkts;
    void *pkt;
    uint32 map_result, loop_count;
    int prec;
    bool retry;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (!scb_psinfo)
        return;

    psq = &scb_psinfo->psq;
    ASSERT(psq);

    spktq_init(&map_psq, PKTQ_LEN_MAX);

    /* Reset previous PGI flags for PS transition */
    PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

    if (pktq_n_pkts_tot(psq) == 0) {
        return;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d cb 0x%p ctx 0x%p total %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), cb, ctx, pktq_n_pkts_tot(psq)));

    /* Set flag to use reserved buf for page in */
    pqp_pgi_rsv_buf(TRUE);

    PKTQ_PREC_ITER(psq, prec) {
        if (pktq_pqp_pkt_cnt(psq, prec) == 0)
            continue;

        loop_count = PQP_PGI_PS_POLLLOOP;
pgi_retry:
        retry = FALSE;

        /* PQP Page in all the packets for this prec */
        pktq_pqp_pgi(psq, prec, PKTQ_PKTS_ALL, PKTQ_PKTS_ALL);

        if (pktq_pqp_owns(psq, prec)) {
            if (pktqprec_n_pkts(psq, prec) == 0) {
                WL_ERROR(("wl%d.%d: %s SCB flowid: %d PGI 0 pkts\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, SCB_FLOWID(scb)));
#ifdef HWA_PKTPGR_BUILD
                // SHARED DBM is not enough, use RESV DBM to fill pqplbuf pool
                hwa_pktpgr_pqplbufpool_rsv_ddbm_fill(WL_HWA_DEVP(wlc));
                goto pgi_retry;
#else  /* !HWA_PKTPGR_BUILD */
                ASSERT(0);
                return;
#endif /* !HWA_PKTPGR_BUILD */
            }
            retry = TRUE;
        }

        /* Do map_pkts_cb_fn and enqueu pkts to tempary spktq for later page-out */
        while ((pkt = pktq_pdeq(psq, prec)) != NULL) {
            map_result = cb(ctx, pkt);
            if (map_result & MAP_PKTS_CB_FREE) {
                PKTFREE(wlc->osh, pkt, TRUE);
            } else {
                spktq_enq(&map_psq, pkt);
            }
        }

        /* Page out pkts and release DBM resource */
        if (spktq_n_pkts(&map_psq)) {
            spktq_pqp_pgo(&map_psq, PQP_APPEND, scb_psinfo);
        }

        /* Retry PGI if there are pkts in host. */
        if (retry) {
            loop_count--;
            ASSERT(loop_count);
            goto pgi_retry;
        }

        /* If  PGI operation finished, append Map PSq to regular PSq */
        /* All Pkts in regular PSq should be moved to Map PSq */
        ASSERT(pktqprec_n_pkts(psq, prec) == 0);

        /* PS queue */
        q_A = &(psq->q[prec]);

        /* Mapp PS Q */
        q_B = &map_psq.q;
        n_pkts = pqp_pkt_cnt(q_B);

        /* Append the queues q_A  =  q_B -> [link] q_A */
        pqp_join(q_A, q_B, PQP_APPEND, scb_psinfo);

        if (n_pkts) {
            if (prec > psq->hi_prec)
                psq->hi_prec = (uint8)prec;
            psq->n_pkts_tot += n_pkts;
        }
    }

    /* Clear flag to use reserved buf */
    pqp_pgi_rsv_buf(FALSE);
}

/* Page In regular PS Q to age out all pkts that have been through one previous listen interval */
static void
wlc_apps_scb_pqp_pktq_filter(wlc_info_t *wlc, struct scb *scb, void *ctx)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq *psq;
    struct spktq temp_psq;
    struct pktq_prec *q_A, *q_B; /**< single precedence packet queue */
    uint16 n_pkts;
    void *pkt;
    int prec;
    bool retry;
    uint32 loop_count;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);
    if (!scb_psinfo)
        return;

    psq = &scb_psinfo->psq;
    ASSERT(psq);

    spktq_init(&temp_psq, PKTQ_LEN_MAX);

    /* Reset previous PGI flags for PS transition */
    PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

    if (pktq_n_pkts_tot(psq) == 0) {
        return;
    }

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d total %d\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), pktq_n_pkts_tot(psq)));

    /* Set flag to use reserved buf for page in */
    pqp_pgi_rsv_buf(TRUE);

    PKTQ_PREC_ITER(psq, prec) {
        loop_count = PQP_PGI_PS_POLLLOOP;
pgi_retry:
        retry = FALSE;

        /* PQP Page in all the packets for this prec */
        pktq_pqp_pgi(psq, prec, PKTQ_PKTS_ALL, PKTQ_PKTS_ALL);

        if (pktq_pqp_owns(psq, prec)) {
            if (pktqprec_n_pkts(psq, prec) == 0) {
                WL_ERROR(("wl%d.%d: %s SCB flowid: %d PGI 0 pkts\n",
                    wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
                    __FUNCTION__, SCB_FLOWID(scb)));
#ifdef HWA_PKTPGR_BUILD
                // SHARED DBM is not enough, use RESV DBM to fill pqplbuf pool
                hwa_pktpgr_pqplbufpool_rsv_ddbm_fill(WL_HWA_DEVP(wlc));
                goto pgi_retry;
#else  /* !HWA_PKTPGR_BUILD */
                ASSERT(0);
                return;
#endif /* !HWA_PKTPGR_BUILD */
            }
            retry = TRUE;
        }

        /* Age out all pkts that have been through one previous listen interval */
        if (pktqprec_n_pkts(psq, prec)) {
            wlc_txq_pktq_pfilter(wlc, psq, prec, wlc_apps_ps_timeout_filter, ctx);
        }

        if (pktqprec_n_pkts(psq, prec)) {
            /* enqueu pkts to temporary spktq for later page-out */
            while ((pkt = pktq_pdeq(psq, prec)) != NULL) {
                spktq_enq(&temp_psq, pkt);
            }

            /* Page Out the single prio queue */
            spktq_pqp_pgo(&temp_psq, PQP_APPEND, scb_psinfo);
        }

        /* Retry PGI if there are pkts in host. */
        if (retry) {
            loop_count--;
            ASSERT(loop_count);
            goto pgi_retry;
        }

        /* If  PGI operation finished, append Temporary PSq back to regular PSq */
        /* All Pkts in regular PSq should be moved to temporary PSq */
        ASSERT(pktqprec_n_pkts(psq, prec) == 0);

        /* PS queue */
        q_A = &(psq->q[prec]);

        /* Temporary PS Q */
        q_B = &temp_psq.q;
        n_pkts = pqp_pkt_cnt(q_B);

        /* Append the queues q_A  =  q_B -> [link] q_A */
        pqp_join(q_A, q_B, PQP_APPEND, scb_psinfo);

        if (n_pkts) {
            if (prec > psq->hi_prec)
                psq->hi_prec = (uint8)prec;
            psq->n_pkts_tot += n_pkts;
        }
    }

    /* Clear flag to use reserved buf */
    pqp_pgi_rsv_buf(FALSE);
}

/* Page Out regular PS Q and suppress PS Q for given SCB */
void
wlc_apps_scb_pqp_pgo(wlc_info_t* wlc, struct scb* scb, uint16 prec_bmp)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    psq;
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return;

    psq = &scb_psinfo->psq;

    ASSERT(psq);

#ifdef WL_USE_SUPR_PSQ
    /* Page out Suppress PS queue */
    wlc_apps_scb_suprq_pqp_pgo(wlc, scb, prec_bmp);
#endif /* WL_USE_SUPR_PSQ */

    /* Page out normal PS queue if there are pkts in queue */
    if (pktq_mlen(psq, prec_bmp)) {
        WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d  prec bmp 0x%x\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            SCB_FLOWID(scb), prec_bmp));
        pktq_pqp_pgo(psq, prec_bmp, PQP_APPEND, scb_psinfo);
    }
}

/* Page out normal PS queue of a given prec for a given SCB */
void
wlc_apps_scb_psq_prec_pqp_pgo(wlc_info_t* wlc, struct scb* scb, int prec)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    psq;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return;

    psq = &scb_psinfo->psq;
    ASSERT(psq);

    if (pktqprec_n_pkts(psq, prec)) {
        WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d  prec 0x%x\n", wlc->pub->unit,
            WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
            SCB_FLOWID(scb), prec));

        /* Page out normal PS queue */
        pktq_prec_pqp_pgo(psq, prec, PQP_APPEND, scb_psinfo);
    }
}

/* Previous PGI PS transition is not finish. Use prepend to page-out PS queue */
void
wlc_apps_scb_pqp_reset_ps_trans(wlc_info_t* wlc, struct scb* scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    psq;
    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return;

    psq = &scb_psinfo->psq;

    ASSERT(psq);

    WL_ERROR(("wl%d.%d: %s SCB flowid: %d previous PS trans 0x%x is not finish %p\n",
        wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), PQP_PGI_PS_TRANS(scb_psinfo), CALL_SITE));

    /* Reset previous PGI flags for PS transition */
    PQP_PGI_PS_TRANS_PEND_CLR(scb_psinfo);

#ifdef WL_USE_SUPR_PSQ
    /* Page out Suppress PS queue */
    wlc_apps_scb_suprq_pqp_pgo(wlc, scb, WLC_PREC_BMP_ALL);
#endif /* WL_USE_SUPR_PSQ */

    /* Page out normal PS queue if there are pkts in queue */
    if (pktq_n_pkts_tot(psq))
        pktq_pqp_pgo(psq, WLC_PREC_BMP_ALL, PQP_PREPEND, scb_psinfo);
}

#ifdef WL_USE_SUPR_PSQ
/* Page out suppress PS Q for a given SCB */
void
wlc_apps_scb_suprq_pqp_pgo(wlc_info_t* wlc, struct scb* scb, uint16 prec_bmp)
{
    struct apps_scb_psinfo *scb_psinfo;
    uint16 prec;
    struct spktq* suppr_psq;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return;

    /* Return if there are no suppressed packets */
    if (scb_psinfo->suppressed_pkts == 0)
        return;

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d  prec bmp 0x%x\n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
        SCB_FLOWID(scb), prec_bmp));

    /* Loop through all available prec suppress PSq */
    for (prec = 0; prec < WLC_PREC_COUNT; prec++) {
        if ((prec_bmp & (1 << prec)) == 0)
            continue;

        /* Suppress PS Q */
        suppr_psq = &scb_psinfo->supr_psq[prec];

        /* Page out suppress PS Queue */
        spktq_pqp_pgo(suppr_psq, PQP_APPEND, scb_psinfo);
    }
}

/* Join a suppress PSq and regular PSq as part of PSq normalization */
void
wlc_apps_scb_suprq_pqp_normalize(wlc_info_t* wlc, struct scb* scb)
{
    struct apps_scb_psinfo *scb_psinfo;
    struct pktq*    pq;
    struct pktq_prec *q_A, *q_B;            /**< single precedence packet queue */
    int prec, hi_prec = 0;
    struct spktq* suppr_psq;

    scb_psinfo = SCB_PSINFO(wlc->psinfo, scb);

    if (!scb_psinfo)
        return;

    /* Previous PGI PS transition is not finish.
     * Reset the status and page-out the remain packets in pktq.
     */
    if (PQP_PGI_PS_TRANS(scb_psinfo)) {
        wlc_apps_scb_pqp_reset_ps_trans(wlc, scb);
    }

    /* Return if there are no suppressed packets */
    if (scb_psinfo->suppressed_pkts == 0)
        return;

    pq = &scb_psinfo->psq;
    ASSERT(pq);

    WL_PS_EX(scb, ("wl%d.%d: %s SCB flowid: %d \n", wlc->pub->unit,
        WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
        __FUNCTION__, SCB_FLOWID(scb)));

    /* Walk through each prec */
    for (prec = (WLC_PREC_COUNT-1); prec >= 0; prec--) {
        /* PS queue */
        q_A = &(pq->q[prec]);

        /* Suppress PS Q */
        suppr_psq = &scb_psinfo->supr_psq[prec];
        q_B = &suppr_psq->q;

        /* Append the queues q_A  =  q_B -> [link] q_A */
        pqp_join(q_A, q_B, PQP_APPEND, scb_psinfo);

        if ((pqp_pkt_cnt(q_A)) && (prec > hi_prec)) {
            hi_prec = prec;
        }
    }

    /* Reset PSq and suppress PSq stats after the join */
    pq->n_pkts_tot += scb_psinfo->suppressed_pkts;
    pq->hi_prec = (uint8)hi_prec;
    scb_psinfo->suppressed_pkts = 0;
}
#endif /* WL_USE_SUPR_PSQ */
#endif /* HNDPQP */
