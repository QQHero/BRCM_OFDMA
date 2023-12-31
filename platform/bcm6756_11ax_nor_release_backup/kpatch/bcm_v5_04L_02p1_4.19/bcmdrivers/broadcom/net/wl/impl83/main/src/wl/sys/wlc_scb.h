/*
 * Common interface to the 802.11 Station Control Block (scb) structure
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
 * $Id: wlc_scb.h 801256 2021-07-19 18:27:17Z $
 */

#ifndef _wlc_scb_h_
#define _wlc_scb_h_

#include <typedefs.h>
#include <ethernet.h>
#include <wlc_types.h>
#include <wlc_rate.h>
#include <wlioctl.h>
#include <wlc_bsscfg.h>
#include <d11_cfg.h>

#define WLC_SCB_REPLAY_LIMIT 64    /* Maximal successive reply failure */

#if defined(TD_STEER) && defined(CONFIG_TENDA_PRIVATE_WLAN)
#include <td_multiap_steer.h>
#endif
typedef struct wlc_scb_stats {
    uint32 tx_pkts;            /**< # of packets transmitted (ucast) */
    uint32 tx_failures;        /**< # of packets failed */
    uint32 rx_ucast_pkts;        /**< # of unicast packets received */
    uint32 rx_mcast_pkts;        /**< # of multicast packets received */
    ratespec_t tx_rate;        /**< Rate of last successful tx frame */
    ratespec_t rx_rate;        /**< Rate of last successful rx frame */
    uint32 rx_decrypt_succeeds;    /**< # of packet decrypted successfully */
    uint32 rx_decrypt_failures;    /**< # of packet decrypted unsuccessfully */
    uint32 tx_mcast_pkts;        /**< # of mcast pkts txed */
    uint64 tx_ucast_bytes;        /**< data bytes txed (ucast) */
    uint64 tx_mcast_bytes;        /**< data bytes txed (mcast) */
    uint64 rx_ucast_bytes;        /**< data bytes recvd ucast */
    uint64 rx_mcast_bytes;        /**< data bytes recvd mcast */
    uint32 tx_pkts_retried;        /**< # of packets where a retry was necessary */
    uint32 tx_pkts_retry_exhausted;    /**< # of packets where a retry was exhausted */
    ratespec_t tx_rate_mgmt;    /**< Rate of last transmitted management frame */
    uint32 tx_rate_fallback;    /**< last used lowest fallback TX rate */
    uint32 rx_pkts_retried;        /**< # rx with retry bit set */
    uint32 tx_pkts_total;
    uint32 tx_pkts_retries;
    uint32 tx_pkts_fw_total;
    uint32 tx_pkts_fw_retries;
    uint32 tx_pkts_fw_retry_exhausted;
    uint32 rx_succ_replay_failures;    /**< # of successive replay failure  */
    uint32 dyn_coremask_thpt[DYNTXC_MAX_COEFNUM];
    uint32 dyn_coremask_pkt[DYNTXC_MAX_COEFNUM];
    uint16 dyn_coremask_count;
    uint16 dyn_coremask_bestidx;
} wlc_scb_stats_t;

#ifdef WLC_MEDIAN_RATE_REPORT

#define WLCNT_SCB_RX_RATE_INDEX(scb)        (scb->scb_rate_stats.rx_rate_index)
#define WLCNT_SCB_TX_RATE_INDEX(scb)        (scb->scb_rate_stats.tx_rate_index)

typedef struct wlc_scb_rate_stats {
    ratespec_t tx_rate_log[NRATELOG];
    ratespec_t rx_rate_log[NRATELOG];
    uint8       tx_rate_index;    /* index into tx_rate array */
    uint8       rx_rate_index;    /* index into rx_rate array */
} wlc_scb_rate_stats_t;
#endif /* WLC_MEDIAN_RATE_REPORT */

struct scb_trf_info {
    uint timestamp;        /* stores the time stamp of the last txfail packet
                 * to maintain circular buffer
                 */
    uint8 idx;
    uint8 histo[WL_TRF_MAX_SECS];
};

#ifdef SCB_MEMDBG
extern void scb_sanitycheck(scb_module_t *scbstate, scb_t* scb);
#else
#define scb_sanitycheck(_scbstate, _scb)    do {} while (0)
#endif /* SCB_MEMDBG */

#define SCB_UPDATE_USED(wlc, scb) ({                                \
    ASSERT((wlc) != NULL);                                    \
    ASSERT((scb) != NULL);                                    \
    if (((scb)->mark != 0) || ((scb)->state == 0)) {                    \
        scb_t* local_scb = wlc_scbfind((wlc), SCB_BSSCFG(scb), &(scb)->ea);        \
        if ((local_scb) && (local_scb == scb)) {                    \
            (scb)->used = (wlc)->pub->now;                        \
        }                                        \
    } else {                                        \
        (scb)->used = (wlc)->pub->now;                            \
    }                                            \
    scb_sanitycheck((wlc)->scbstate, (scb));                        \
})

/**
 * ------------------------------------------------------------------------------------------------
 * LowTxq and DMA/AQM Fifo per SCB in flight counters
 * ------------------------------------------------------------------------------------------------
 */

#define SCB_PKTS_INFLT_FIFOCNT_INCR(scb, prio)                            \
    do {                                            \
        /**< Increment by 1 */                                \
        ((scb)->pkts_inflt_fifocnt[(prio)]++);                        \
        ((scb)->pkts_inflt_fifocnt_all++);                        \
    } while (0)

#define SCB_PKTS_INFLT_FIFOCNT_ADD(scb, prio, delta)                        \
    do {                                            \
        /**< Increment by delta */                            \
        ((scb)->pkts_inflt_fifocnt[(prio)] += (delta));                    \
        ((scb)->pkts_inflt_fifocnt_all += (delta));                    \
    } while (0)

#define SCB_CHECK_DRAIN_COMPLETE(wlc, scb, prio)                        \
    do {                                            \
        if ((scb->ac_suspend != 0) &&                            \
            ((scb)->pkts_inflt_fifocnt[(prio)] == 0) &&                \
            !SCB_MARKED_DEL((scb))) {                        \
            wlc_scb_resume_traffic((wlc), (scb), prio2ac[(prio)]);            \
        }                                        \
    } while (0)

#ifdef WL_TXPKTPEND_SYNC
#define SCB_PKTS_INFLT_FIFOCNT_DECR(wlc, scb, prio)                        \
    do {                                            \
        /**< Decrement by 1 */                                \
        ((scb)->pkts_inflt_fifocnt[(prio)]--);                        \
        ((scb)->pkts_inflt_fifocnt_all--);                        \
        SCB_CHECK_DRAIN_COMPLETE((wlc), (scb), (prio));                    \
        if ((scb)->pkts_inflt_fifocnt[(prio)] < 0)                    \
            (wlc)->pktpend_sync |= TXPKTPEND_SYNC_EN;                \
    } while (0)

#define SCB_PKTS_INFLT_FIFOCNT_SUB(wlc, scb, prio, delta)                    \
    do {                                            \
        /**< Decrement by specified value */                        \
        ((scb)->pkts_inflt_fifocnt[(prio)] -= (delta));                    \
        ((scb)->pkts_inflt_fifocnt_all -= (delta));                    \
        SCB_CHECK_DRAIN_COMPLETE((wlc), (scb), (prio));                    \
        if ((scb)->pkts_inflt_fifocnt[(prio)] < 0)                    \
            (wlc)->pktpend_sync |= TXPKTPEND_SYNC_EN;                \
    } while (0)
#else
#define SCB_PKTS_INFLT_FIFOCNT_DECR(wlc, scb, prio)                        \
    do {                                            \
        /**< Decrement by 1 */                                \
        ((scb)->pkts_inflt_fifocnt[(prio)]--);                        \
        ASSERT((scb)->pkts_inflt_fifocnt[(prio)] >= 0);                    \
        ((scb)->pkts_inflt_fifocnt_all--);                        \
        ASSERT((scb)->pkts_inflt_fifocnt_all >= 0);                    \
        SCB_CHECK_DRAIN_COMPLETE((wlc), (scb), (prio));                    \
    } while (0)

#define SCB_PKTS_INFLT_FIFOCNT_SUB(wlc, scb, prio, delta)                    \
    do {                                            \
        /**< Decrement by specified value */                        \
        ((scb)->pkts_inflt_fifocnt[(prio)] -= (delta));                    \
        ASSERT((scb)->pkts_inflt_fifocnt[(prio)] >= 0);                    \
        ((scb)->pkts_inflt_fifocnt_all -= (delta));                    \
        ASSERT((scb)->pkts_inflt_fifocnt_all >= 0);                    \
        SCB_CHECK_DRAIN_COMPLETE((wlc), (scb), (prio));                    \
    } while (0)
#endif /* WL_TXPKTPEND_SYNC */

#define SCB_PKTS_INFLT_FIFOCNT_VAL(scb, prio)                            \
        ((scb)->pkts_inflt_fifocnt[(prio)])        /**< Return value */

#define SCB_TOT_PKTS_INFLT_FIFOCNT_VAL(scb)                            \
        ((scb)->pkts_inflt_fifocnt_all)            /**< Return value */

#ifdef WLTAF
#define SCB_UTXD_INFLT_VAL(scb) ((scb)->utxd_inflt)

#define SCB_UTXD_INFLT_INCR(scb) ((scb)->utxd_inflt++)

#define SCB_UTXD_INFLT_ADD(scb, delta) ((scb)->utxd_inflt += (delta))

#define SCB_UTXD_INFLT_DECR(scb) \
    ({ \
        ((scb)->utxd_inflt--); \
        ASSERT((scb)->utxd_inflt >= 0); \
    })

#define SCB_UTXD_INFLT_SUB(scb, delta) \
    ({ \
        ((scb)->utxd_inflt -= (delta)); \
        ASSERT((scb)->utxd_inflt >= 0); \
    })
#else
#define SCB_UTXD_INFLT_VAL(scb) (0)
#define SCB_UTXD_INFLT_DECR(scb)
#endif /* WLTAF */

/**
 * ------------------------------------------------------------------------------------------------
 * Common Queue per SCB in flight counters
 * ------------------------------------------------------------------------------------------------
 */

#define SCB_PKTS_INFLT_CQCNT_INCR(scb, prec)                        \
        ((scb)->pkts_inflt_cqcnt[(prec)]++)        /**< Increment by 1 */

#define SCB_PKTS_INFLT_CQCNT_DECR(scb, prec)                        \
    do {                                        \
        int16 *pkts_inflt_cqcnt = &(scb)->pkts_inflt_cqcnt[(prec)];        \
        (*pkts_inflt_cqcnt)--;                /**< Decrement by 1 */    \
        ASSERT(*pkts_inflt_cqcnt >= 0);                        \
    } while (0)

#define SCB_PKTS_INFLT_CQCNT_VAL(scb, prec)                        \
        ((scb)->pkts_inflt_cqcnt[(prec)])        /**< Return value */

#define SCB_PKTS_TOT_INFLT_CQCNT_VAL(scb)                        \
    ((scb)->pkts_inflt_cqcnt[0] +                            \
     (scb)->pkts_inflt_cqcnt[1] +                            \
     (scb)->pkts_inflt_cqcnt[2] +                            \
     (scb)->pkts_inflt_cqcnt[3] +                            \
     (scb)->pkts_inflt_cqcnt[4] +                            \
     (scb)->pkts_inflt_cqcnt[5] +                            \
     (scb)->pkts_inflt_cqcnt[6] +                            \
     (scb)->pkts_inflt_cqcnt[7] +                            \
     (scb)->pkts_inflt_cqcnt[8] +                            \
     (scb)->pkts_inflt_cqcnt[9] +                            \
     (scb)->pkts_inflt_cqcnt[10] +                            \
     (scb)->pkts_inflt_cqcnt[11] +                            \
     (scb)->pkts_inflt_cqcnt[12] +                            \
     (scb)->pkts_inflt_cqcnt[13] +                            \
     (scb)->pkts_inflt_cqcnt[14] +                            \
     (scb)->pkts_inflt_cqcnt[15])

/**
 * Information about a specific remote entity, and the relation between the local and that remote
 * entity. Station Control Block.
 */
struct scb {
    uint32        flags;        /**< various bit flags as defined below */
    uint32        flags2;        /**< various bit flags2 as defined below */
    uint32        flags3;        /**< various bit flags3 as defined below */
    wlc_bsscfg_t    *bsscfg;    /**< bsscfg to which this scb belongs */
    struct ether_addr ea;        /**< station address, must be aligned */
    uint16        aid;        /**< association ID */
    uint16        flowid;        /**< SW flowid per scb */
    uint16        host_ringid[NUMPRIO];    /**< Host ring id [flowringid /pktfwd tbl id */
    uint8        state;        /**< current state bitfield of auth/assoc process */
    bool        permanent;    /**< scb should not be reclaimed */
    uint8        mark;        /**< various marking bitfield */
    uint8        rx_lq_samp_req;    /**< recv link quality sampling request - fast path */
    uint32        pktc_pps;    /**< pps counter for activating pktc */
    uint        used;        /**< time of last use */
    uint32        assoctime;    /**< time of association */
    enum wlc_bandunit bandunit;    /**< the band this scb belongs to */
    uint32        WPA_auth;    /**< WPA: authenticated key management */
    uint32        wsec;        /**< ucast security algo. should match key->algo.
                     * Needed before key is set
                     */
    wlc_rateset_t    rateset;    /**< operational rates for this remote station */
    uint16        seqctl[NUMPRIO]; /**< seqctl of last received frame (for dups) */
    uint16        seqctl_nonqos;    /**< seqctl of last received frame (for dups) for
                     * non-QoS data and management
                     */
    uint16        seqnum[NUMPRIO]; /**< WME: driver maintained sw seqnum per priority */

    /* APSD configuration */
    struct {
        uint16        maxsplen;   /**< Maximum Service Period Length from assoc req */
        ac_bitmap_t    ac_defl;    /**< Bitmap of ACs enabled for APSD from assoc req */
        ac_bitmap_t    ac_trig;    /**< Bitmap of ACs currently trigger-enabled */
        ac_bitmap_t    ac_delv;    /**< Bitmap of ACs currently delivery-enabled */
    } apsd;

    uint16        cap;        /**< sta's advertized capability field */
    uint8        auth_alg;    /**< 802.11 authentication mode */
    uint8        ps_pretend;    /**< AP pretending STA is in PS mode */
    bool        PS;        /**< remote STA in PS mode */
    bool        PS_TWT;        /**< remote STA in TWT PS mode */
    bool        ps_txfifo_blk;    /**< STA in PS and draining txfifo. block release */
    uint8        dh_params_included; /* Diffie Hellman params included in assoc response */
    dh_params_ie_t *dh_params;
    wlc_if_t    *wds;        /**< per-port WDS cookie */
    tx_path_node_t    *tx_path;    /**< pkt tx path (allocated as scb cubby) */
    wl_if_stats_t    *if_stats;
    uint16        restrict_txwin;    /* WL_CS_RESTRICT_RELEASE */
    uint8        restrict_deadline; /* WL_CS_RESTRICT_RELEASE */

    uint8        pmkid_included; /* PMKID included in assoc request */

    /* it's allocated as an array, use 'delay_stats + ac' to access per ac element! */
    scb_delay_stats_t *delay_stats;    /**< per-AC delay stats (allocated as scb cubby) */
    txdelay_params_t *txdelay_params; /* WLPKTDLYSTAT_IND */
    wlc_deauth_send_cbargs_t* sent_deauth; /**< keep track of deauth notification */

    wlc_scb_stats_t    scb_stats;    /* WLCNTSCB */

#ifdef WL_PKTDROP_STATS
    wlc_pktdrop_brief_counters_t *pktdrop_brief_counters;
    wlc_pktdrop_full_counters_t *pktdrop_full_counters;
#endif /* WL_PKTDROP_STATS */

#if defined(STA) && defined(DBG_BCN_LOSS)
    struct wlc_scb_dbg_bcn dbg_bcn;
#endif
    uint8        sup_chan_width;    /**< VHT only. Channel width supported */
    uint8        ext_nss_bw_sup;    /* IEEE 802.11 REVmc Draft 8.0 EXT_NSS_BW support */
    uint8 trf_enable_flag;        /* This variable stores the bit map of type whose
                     * stats are being maintained for txfail event
                     */
    int8 rssi;            /* rssi for the received pkt */
    struct scb_trf_info scb_trf_data[WL_TRF_MAX_QUEUE];
#ifdef WLSCB_HISTO
    wl_rate_histo_maps2_t *histo;    /**< mapped histogram of rates */
#endif /* WLSCB_HISTO */
#if defined(BCMHWA) && defined(HWA_RXFILL_BUILD)
    uint32 rx_auth_tsf;    /* 32 bits of auth Rx timestamp */
    uint32 rx_auth_tsf_reference;
#endif
#ifdef AP
    uint        wsec_auth_timeout;    /* timeout to handle nas/eap restart */
#endif /* AP */
#if defined(WLATM_PERC)
    uint16        staperc;    /* airtime percentage of sta */
    uint16        sched_staperc;    /* Schedule context airtime percentage of sta */
    uint32        last_active_usec;
#endif /* WLATM_PERC */

    int16 pkts_inflt_fifocnt[NUMPRIO];
    int32 pkts_inflt_fifocnt_all;
    int16 pkts_inflt_cqcnt[NUMPRIO * 2];
#ifdef WLTAF
    int16 utxd_inflt; /* # of utxd in ATIM fifo */
#endif

    uint32 mem_bytes; /* bytes of memory allocated for this scb */
#ifdef WL_PS_STATS
    /* PS counters */
    uint32 suprps_cnt;    /* Total num of packets suppressed by ucode because of PS */
    int32  ps_on_cnt;    /* Total num of times transitioning to power-on */
    uint32 ps_starttime;    /* Starting time of the client PS druation */
    uint32 ps_mintime;
    uint32 ps_maxtime;
    uint32 ps_tottime;    /* Aggregated time for the average time calculation */
    uint32 ps_dur0_200;    /* Histogram bin for 0..200ms */
    uint32 ps_dur200_500;    /* Histogram bin for 200..500ms */
    uint32 ps_dur500_2;    /* Histogram bin for 500ms..2sec */
    uint32 ps_dur2_5;    /* Histogram bin for 2sec..5sec */
    uint32 ps_dur5;        /* Histogram bin for 5sec.. */
#endif /* WL_PS_STATS */
#ifdef BCM_CSIMON
    wlc_csimon_sta_t *csimon; /* CSI Monitor info for this SCB */
#endif
    rsnxe_ie_info_t *rsnxe_info; /* STA rsnxe IE info */
    bcm_tlv_t *rsn_ie;      /* RSN IE included in assoc response for OWE */
    uint16 dyn_coremask;
    uint8 fifo_idx[AC_COUNT];
    uint8 ac_suspend;
#if defined(TD_STEER) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    td_11k_rrm_info_t beacon_rep;
#endif

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    bool is_ai_mesh;
    bool node_free_disable;
#endif

#ifdef CONFIG_TENDA_PRIVATE_WLAN
    unsigned long last_time;
#endif
#ifdef BCMDBG
    bool msg_macfltr;    /* Debug message mac filter */
#endif /* BCMDBG */
#if defined(WLCNTSCB) && defined(WLC_MEDIAN_RATE_REPORT)
    wlc_scb_rate_stats_t scb_rate_stats;
#endif /* WLCNTSCB && WLC_MEDIAN_RATE_REPORT */
    uint8 dropblock_dur;    /* drop and block during new key process */
};

/** Iterator for scb list */
struct scb_iter {
    struct scb    *next;            /**< next scb in bss */
    wlc_bsscfg_t    *next_bss;        /**< next bss pointer */
    bool        all;            /**< walk all bss or not */
};

#define SCB_BSSCFG(a)           ((a)->bsscfg)
#define SCB_WLC(a)              ((a)->bsscfg->wlc)

/** Initialize an scb iterator pre-fetching the next scb as it moves along the list */
void wlc_scb_iterinit(scb_module_t *scbstate, struct scb_iter *scbiter,
    wlc_bsscfg_t *bsscfg);
/** move the iterator */
struct scb *wlc_scb_iternext(scb_module_t *scbstate, struct scb_iter *scbiter);

/* Iterate thru' scbs of specified bss */
#define FOREACH_BSS_SCB(scbstate, scbiter, bss, scb) \
    for (wlc_scb_iterinit((scbstate), (scbiter), (bss)); \
         ((scb) = wlc_scb_iternext((scbstate), (scbiter))) != NULL; )

/* Iterate thru' scbs of all bss. Use this only when needed. For most of
 * the cases above one should suffice.
 */
#define FOREACHSCB(scbstate, scbiter, scb) \
    for (wlc_scb_iterinit((scbstate), (scbiter), NULL); \
         ((scb) = wlc_scb_iternext((scbstate), (scbiter))) != NULL; )

/* module attach/detach interface */
scb_module_t *wlc_scb_attach(wlc_info_t *wlc);
void wlc_scb_detach(scb_module_t *scbstate);

/* scb cubby cb functions */
typedef int (*scb_cubby_init_t)(void *, struct scb *);
typedef void (*scb_cubby_deinit_t)(void *, struct scb *);
/* return the secondary cubby size */
typedef uint (*scb_cubby_secsz_t)(void *, struct scb *);
typedef void (*scb_cubby_dump_t)(void *, struct scb *, struct bcmstrbuf *b);

typedef struct scb_cubby_params {
    void *context;
    scb_cubby_init_t fn_init;
    scb_cubby_deinit_t fn_deinit;
    scb_cubby_dump_t fn_dump;
    scb_cubby_secsz_t fn_secsz;
} scb_cubby_params_t;

/**
 * This function allocates an opaque cubby of the requested size in the scb container.
 * The cb functions fn_init/fn_deinit are called when a scb is allocated/freed.
 * The cb fn_secsz is called if registered to tell the secondary cubby size.
 * The functions are called with the context passed in and a scb pointer.
 * It returns a handle that can be used in macro SCB_CUBBY to retrieve the cubby.
 * Function returns a negative number on failure, non-negative for cubby offsets.
 */
#ifdef BCMDBG
int wlc_scb_cubby_reserve_ext(wlc_info_t *wlc, uint size, scb_cubby_params_t *params,
                              const char *func);
int wlc_scb_cubby_reserve(wlc_info_t *wlc, uint size,
                          scb_cubby_init_t fn_init, scb_cubby_deinit_t fn_deinit,
                          scb_cubby_dump_t fn_dump, void *context,
                          const char *func);

/* Macro defines to automatically supply the function name parameter */
#define wlc_scb_cubby_reserve(wlc, size, fn_init, fn_deinit, fn_dump, ctx) \
    wlc_scb_cubby_reserve(wlc, size, fn_init, fn_deinit, fn_dump, ctx, __FUNCTION__)

#define wlc_scb_cubby_reserve_ext(wlc, size, params) \
    wlc_scb_cubby_reserve_ext(wlc, size, params, __FUNCTION__)
#else
int wlc_scb_cubby_reserve_ext(wlc_info_t *wlc, uint size, scb_cubby_params_t *params);
int wlc_scb_cubby_reserve(wlc_info_t *wlc, uint size,
                          scb_cubby_init_t fn_init, scb_cubby_deinit_t fn_deinit,
                          scb_cubby_dump_t fn_dump, void *context);
#endif /* BCMDBG */

/* macro to retrieve pointer to module specific opaque data in scb container */
#define SCB_CUBBY(scb, handle)    (void *)(((uint8 *)(scb)) + handle)

/**
 * This function allocates a secondary cubby in the secondary cubby area.
 */
void *wlc_scb_sec_cubby_alloc(wlc_info_t *wlc, struct scb *scb, uint secsz);
void wlc_scb_sec_cubby_free(wlc_info_t *wlc, struct scb *scb, void *secptr);

/*
 * Accessors
 */

struct wlcband *wlc_scbband(wlc_info_t *wlc, struct scb *scb);

/** Find station control block corresponding to the remote id */
struct scb *wlc_scbfind(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea);

/** Lookup station control for ID. If not found, create a new entry. */
struct scb *wlc_scblookup(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, const struct ether_addr *ea);

/** Lookup station control for ID. If not found, create a new entry. */
struct scb *wlc_scblookupband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                              const struct ether_addr *ea, enum wlc_bandunit bandunit);

/** Get scb from band */
struct scb *wlc_scbfindband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
                            const struct ether_addr *ea, enum wlc_bandunit bandunit);

/** Move the scb's band info */
void wlc_scb_update_band_for_cfg(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg, chanspec_t chanspec);

extern struct scb *wlc_scbibssfindband(wlc_info_t *wlc, const struct ether_addr *ea,
    enum wlc_bandunit bandunit, wlc_bsscfg_t **bsscfg);

/** Find the STA acorss all APs */
extern struct scb *wlc_scbapfind(wlc_info_t *wlc, const struct ether_addr *ea);

extern struct scb *wlc_scbbssfindband(wlc_info_t *wlc, const struct ether_addr *hwaddr,
    const struct ether_addr *ea, enum wlc_bandunit bandunit, wlc_bsscfg_t **bsscfg);

extern struct scb *wlc_scbfind_from_wlcif(wlc_info_t *wlc, struct wlc_if *wlcif, uint8 *addr);

struct scb *wlc_bcmcscb_alloc(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct wlcband *band);
#define wlc_bcmcscb_free(wlc, scb) wlc_scbfree(wlc, scb)
struct scb *wlc_hwrsscb_alloc(wlc_info_t *wlc, struct wlcband *band);
#define wlc_hwrsscb_free(wlc, scb) wlc_scbfree(wlc, scb)
#ifdef WL_PROXDETECT
struct scb *wlc_proxdscb_alloc(wlc_info_t *wlc, struct wlcband *band);
#define wlc_proxdscb_free(wlc, scb) wlc_scbfree(wlc, scb)
#endif

void wlc_scb_dh_params_free(wlc_info_t *wlc, struct scb *scb);
void wlc_scb_rsn_ie_free(wlc_info_t *wlc, struct scb *scb);
void wlc_scb_rsnxe_info_free(wlc_info_t *wlc, struct scb *scb);
bool wlc_scbfree(wlc_info_t *wlc, struct scb *remove);

/** * "|" operation */
void wlc_scb_setstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state);

/** * "& ~" operation . */
void wlc_scb_clearstatebit(wlc_info_t *wlc, struct scb *scb, uint8 state);

/** * reset all state. the multi ssid array is cleared as well. */
void wlc_scb_resetstate(wlc_info_t *wlc, struct scb *scb);

void wlc_scb_reinit(wlc_info_t *wlc);

/** sort rates for a single scb */
void wlc_scb_sortrates(wlc_info_t *wlc, struct scb *scb);
/** sort rates for all scb in wlc */
void wlc_scblist_validaterates(wlc_info_t *wlc);

#ifdef WL_MBO
#define SCB3_MBO        0x00800000      /* MBO */
#define SCB_MBO(a)        ((a)->flags3 & SCB3_MBO)
#else
#define SCB_MBO(a)        FALSE
#endif /* WL_MBO */

/* SCB flags */
#define SCB_NONERP        0x00000001    /**< No ERP */
#define SCB_LONGSLOT        0x00000002    /**< Long Slot */
#define SCB_SHORTPREAMBLE    0x00000004    /**< Short Preamble ok */
#define SCB_8021XHDR        0x00000008    /**< 802.1x Header */
#define SCB_WPA_SUP        0x00000010    /**< 0 - authenticator, 1 - supplicant */
#define SCB_DEAUTH        0x00000020    /**< 0 - ok to deauth, 1 - no (just did) */
#define SCB_WMECAP        0x00000040    /**< WME Cap; may ONLY be set if WME_ENAB(wlc) */
#define SCB_WPSCAP        0x00000080    /**< Peer is WPS capable */
#define SCB_BRCM        0x00000100    /**< BRCM AP or STA */
#define SCB_WDS_LINKUP        0x00000200    /**< WDS link up */
#define SCB_LEGACY_AES        0x00000400    /**< legacy AES device */
#define SCB_AMSDU_SPP        0x00000800    /**< AMSDU SPP */
#define SCB_MYAP        0x00001000    /**< We are associated to this AP */
#define SCB_PENDING_PROBE    0x00002000    /**< Probe is pending to this SCB */
#define SCB_AMSDUCAP        0x00004000    /**< A-MSDU capable */
#define SCB_PSPRETEND_PROBE    0x00008000    /**< PSPretend probe */
#define SCB_HTCAP        0x00010000    /**< HT (MIMO) capable device */
#define SCB_RECV_PM        0x00020000    /**< state of PM bit in last data frame recv'd */
#define SCB_AMPDUCAP        0x00040000    /**< A-MPDU capable */
#define SCB_IS40        0x00080000    /**< 40MHz capable */
#define SCB_NONGF        0x00100000    /**< Not Green Field capable */
#define SCB_APSDCAP        0x00200000    /**< APSD capable */
#define SCB_DEFRAG_INPROG    0x00400000    /**< Defrag in progress, summary for all prio */
#define SCB_PENDING_PSPOLL    0x00800000    /**< PS-Poll is pending to this SCB */
#define SCB_RIFSCAP        0x01000000    /**< RIFS capable */
#define SCB_HT40INTOLERANT    0x02000000    /**< 40 Intolerant */
#define SCB_WMEPS        0x04000000    /**< PS + WME w/o APSD capable */
#define SCB_SENT_APSD_TRIG    0x08000000    /**< APSD Trigger Null Frame was recently sent */
#define SCB_COEX_MGMT        0x10000000    /**< Coexistence Management supported */
#define SCB_IBSS_PEER        0x20000000    /**< Station is an IBSS peer */
#define SCB_STBCCAP        0x40000000    /**< STBC Capable */
#define SCB_DTPCCAP        0x80000000    /**< DTPC Capable */

/* scb flags2 */
#define SCB2_SGI20_CAP        0x00000001    /**< 20MHz SGI Capable */
#define SCB2_SGI40_CAP        0x00000002    /**< 40MHz SGI Capable */
#define SCB2_RX_LARGE_AGG    0x00000004    /**< device can rx large aggs */
#define SCB2_BCMC        0x00000008    /**< scb is used to support bc/mc traffic */
#define SCB2_HWRS        0x00000010    /**< scb is used to hold h/w rateset */
#define SCB2_WAIHDR        0x00000020    /**< WAI Header */
#define SCB2_P2P        0x00000040    /**< WiFi P2P */
#define SCB2_LDPCCAP        0x00000080    /**< LDPC Cap */
#define SCB2_BCMDCS        0x00000100    /**< BCM_DCS */
#define SCB2_MFP        0x00000200    /**< 802.11w MFP_ENABLE */
#define SCB2_SHA256        0x00000400    /**< sha256 for AKM */
#define SCB2_VHTCAP        0x00000800    /**< VHT (11ac) capable device */
#define SCB2_HECAP        0x00001000    /**< HE (11ax) capable device */
#define SCB2_AMSDU_DISABLE    0x00002000      /** < Disable AMSDU */
#define SCB2_PROXD        0x00004000    /**< scb is used to support proximity detection */
#define SCB2_IGN_SMPS        0x08000000    /**< ignore SM PS update */
#define SCB2_IS80        0x10000000    /**< 80MHz capable */
#define SCB2_AMSDU_IN_AMPDU_CAP    0x20000000    /**< AMSDU over AMPDU */
#define SCB2_XXXX        0x40000000
#define SCB2_DWDS_ACTIVE    0x80000000    /**< DWDS is active */

/* scb flags3 */
#define SCB3_A4_DATA        0x00000001    /**< scb does 4 addr data frames */
#define SCB3_A4_NULLDATA    0x00000002    /**< scb does 4-addr null data frames */
#define SCB3_A4_8021X        0x00000004    /**< scb does 4-addr 8021x frames */
#define SCB3_XXX        0x00000008
#define SCB3_FTM_INITIATOR    0x00000010    /**< fine timing meas initiator */
#define SCB3_FTM_RESPONDER    0x00000020    /**< fine timing meas responder */
#define SCB3_MAP_CAP        0x00000040    /**< MultiAp capable */
#define SCB3_ECSA_SUPPORT    0x00000080    /* ECSA supported STA */
#define SCB3_MAP_P2_CAP        0x00000100    /**< Multiap profile-2 */
#define SCB3_XXXX        0x00000200
#define SCB3_XXXXX        0x00000400
#ifdef WL_RELMCAST
#define SCB3_RELMCAST        0x00000800    /**< Reliable Multicast */
#define SCB3_RELMCAST_NOACK    0x00001000    /**< Reliable Multicast No ACK rxed */
#endif
#define SCB3_PKTC        0x00002000    /**< Enable packet chaining */
#define SCB3_OPER_MODE_NOTIF    0x00004000    /**< 11ac Oper Mode Notif'n */
#define SCB3_RRM        0x00008000    /**< Radio Measurement */
#define SCB3_DWDS_CAP        0x00010000    /**< DWDS capable */
#define SCB3_IS_160        0x00020000    /**< VHT 160 cap */
#define SCB3_IS_80_80        0x00040000    /**< VHT 80+80 cap */
#define SCB3_1024QAM_CAP    0x00080000    /**< VHT 1024QAM rates cap */
#define SCB3_HT_PROP_RATES_CAP    0x00100000    /**< Broadcom proprietary 11n rates */

#define SCB3_TS_MASK        0x00600000    /* Traffic Stream 2 flags */
#define SCB3_TS_ATOS        0x00000000
#define SCB3_TS_ATOS2        0x00200000
#define SCB3_TS_EBOS        0x00400000

#define SCB3_UNUSED        0x00800000    /* Unused */

#define SCB3_VHTMU        0x01000000    /**< VHT MU cap */
#define SCB3_HEMMU        0x02000000    /**< HE MU-MIMO cap */

/* ULOFDMA bit is set when ULMU is enabled. Over the air transmission could be in MU-MIMO
 * or in OFDMA. Ideally we should rename ULOFDMA to ULMU and introduce ULMMU for MU-MIMO
 * and ULOMU for OFDMA. This change will require touching many files and some of those include
 * shared header files with phy/ucode. To minimize disruption, keeping ULOFDMA as synonymous to
 * ULMU. Based on the value of tagid tx will be MU-MIMO or OFDMA. ULMMU bit will be set when tagid
 * is above a certain value which is defined in ULMU module. Default is ULOMU.
 * Disabling ULMU will require clearing both ULOFDMA and ULMMU bits.
*/
#define SCB3_DLOFDMA        0x04000000    /**< HE DL-OFDMA cap */
#define SCB3_ULOFDMA        0x08000000    /**< HE UL-OFDMA cap */
#define SCB3_ULMMU        0x10000000    /**< HE UL-MUMIMO cap */
#define SCB3_HT_BEAMFORMEE    0x20000000    /* Receive NDP Capable */
#define SCB3_HT_BEAMFORMER    0x40000000    /* Transmit NDP Capable */
#define SCB3_CFP        0x80000000    /* Enable CFP module */

#define SCB_TS_EBOS(a)        (((a)->flags3 & SCB3_TS_MASK) == SCB3_TS_EBOS)
#define SCB_TS_ATOS(a)        (((a)->flags3 & SCB3_TS_MASK) == SCB3_TS_ATOS)
#define SCB_TS_ATOS2(a)        (((a)->flags3 & SCB3_TS_MASK) == SCB3_TS_ATOS2)

/* scb association state bitfield */
#define AUTHENTICATED        1    /**< 802.11 authenticated (open or shared key) */
#define ASSOCIATED        2    /**< 802.11 associated */
#define PENDING_AUTH        4    /**< Waiting for 802.11 authentication response */
#define PENDING_ASSOC        8    /**< Waiting for 802.11 association response */
#define AUTHORIZED        0x10    /**< 802.1X authorized */
#ifdef CONFIG_TENDA_REPEATER_STATUS
#define WRONG_KEY         0x20    /*4-Way Handshake failed*/
#endif
/* scb association state helpers */
#define SCB_ASSOCIATED(a)    ((a)->state & ASSOCIATED)
#define SCB_ASSOCIATING(a)    ((a)->state & PENDING_ASSOC)
#define SCB_AUTHENTICATING(a)    ((a)->state & PENDING_AUTH)
#define SCB_AUTHENTICATED(a)    ((a)->state & AUTHENTICATED)
#define SCB_AUTHORIZED(a)    ((a)->state & AUTHORIZED)
#define SCB_PENDING_AUTH(a)    ((a)->state & PENDING_AUTH)
#ifdef CONFIG_TENDA_REPEATER_STATUS
#define SCB_WRONG_KEY(a) ((a)->state & WRONG_KEY)
#endif
#define SCB_DURING_JOIN(a)  (SCB_ASSOCIATED(a) || \
    SCB_ASSOCIATING(a) || SCB_AUTHENTICATED(a) || SCB_AUTHORIZED(a) || \
    SCB_PENDING_AUTH(a))

/* scb marking bitfield - used by scb module itself */
#define SCB_MARK_TO_DEL        (1 << 0)    /**< mark to be deleted in watchdog */
#define SCB_DEL_IN_PROG        (1 << 1)    /**< delete in progress - clip recursion */
#define SCB_MARK_TO_REM        (1 << 2)
#define SCB_UNUSABLE_STA_TO    (1 << 3)    /**< scb has been in
                          * wlc_ap_scb_unusable_stas_timeout
                          */
#define SCB_CLEANUP_IN_PROG    (1 << 4)    /**< cleanup in progress - clip recursion */

/* scb marking bitfield helpers */
#define SCB_DEL_IN_PROGRESS(a)        ((a)->mark & SCB_DEL_IN_PROG)
#define SCB_CLEANUP_IN_PROGRESS(a)    ((a)->mark & SCB_CLEANUP_IN_PROG)
#define SCB_MARKED_DEL(a)        ((a)->mark & SCB_MARK_TO_DEL)
#define SCB_FIRST_TO(a)            (!((a)->mark & SCB_UNUSABLE_STA_TO))
#define SCB_MARK_DEL(a)    do { \
    (a)->mark |= SCB_MARK_TO_DEL; \
    wlc_scb_close_link(SCB_WLC((a)), (a)); \
} while (0)

#define SCB_IS_UNUSABLE(scb)    (!(scb) || SCB_MARKED_DEL(scb) || SCB_DEL_IN_PROGRESS(scb))

/* flag access */
#define SCB_ISMYAP(a)           ((a)->flags & SCB_MYAP)
#define SCB_ISPERMANENT(a)      ((a)->permanent)
#define    SCB_INTERNAL(a)        ((a)->flags2 & (SCB2_BCMC | SCB2_HWRS | SCB2_PROXD))
#define    SCB_BCMC(a)        ((a)->flags2 & SCB2_BCMC)
#define    SCB_HWRS(a)        ((a)->flags2 & SCB2_HWRS)
#define    SCB_PROXD(a)        ((a)->flags2 & SCB2_PROXD)
#define SCB_IS_BRCM(a)        ((a)->flags & SCB_BRCM)
#define SCB_ISMULTI(a)        SCB_BCMC(a)

#ifdef WL_SPP_AMSDU
#define SCB_SPP_AMSDU(a)    ((scb->flags & SCB_AMSDU_SPP) !=  0)
#else
#define SCB_SPP_AMSDU(a)    FALSE
#endif /* WL_SPP_AMSDU */

#define SCB_AUTH_TIMEOUT    10    /* # seconds: interval to wait for auth reply from
                     * supplicant/authentication server.
                     */
/* scb_info macros */
#ifdef AP
#define SCB_PS(a)        ((a) && (a)->PS)
#define SCB_TWTPS(a)        ((a) && (a)->PS_TWT)
#ifdef WDS
#define SCB_WDS(a)        ((a)->wds)
#else
#define SCB_WDS(a)        NULL
#endif
#define SCB_INTERFACE(a)        ((a)->wds ? (a)->wds->wlif : (a)->bsscfg->wlcif->wlif)
#define SCB_WLCIFP(a)           ((a)->wds ? (a)->wds : ((a)->bsscfg->wlcif))
#define WLC_BCMC_PSMODE(wlc, bsscfg) (SCB_PS(WLC_BCMCSCB_GET(wlc, bsscfg)))

#if defined(WL_PS_SCB_TXFIFO_BLK)
#define SCB_PS_TXFIFO_BLK(a)    ((a)->ps_txfifo_blk)
#endif /* WL_PS_SCB_TXFIFO_BLK */

#else    /* ! AP */
#define SCB_PS(a)        FALSE
#define SCB_TWTPS(a)        FALSE
#define SCB_WDS(a)        NULL
#define SCB_INTERFACE(a)        ((a)->bsscfg->wlcif->wlif)
#define SCB_WLCIFP(a)           ((a)->bsscfg->wlcif)
#define WLC_BCMC_PSMODE(wlc, bsscfg) (FALSE)
#define SCB_PS_TXFIFO_BLK(a)    FALSE
#endif /* ! AP */

#define SCB_FLOWID_LOCAL_MASK        (0xFFF)                 /* Local Flowid Mask */
#define SCB_FLOWID_INCARN_SHIFT        12
#define SCB_FLOWID_INCARN_MASK        (0x3 << SCB_FLOWID_INCARN_SHIFT) /* Incarnation ID Mask */
#define SCB_FLOWID_RADIO_SHIFT        14
#define SCB_FLOWID_RADIO_MASK        (0x3 << SCB_FLOWID_RADIO_SHIFT)     /* Radio ID mask */

#define SCB_FLOWID_LOCAL(flowid)    ((flowid) & SCB_FLOWID_LOCAL_MASK)
#define SCB_FLOWID_RADIO(flowid)    (((flowid) & SCB_FLOWID_RADIO_MASK) >> \
    SCB_FLOWID_RADIO_SHIFT)
#define SCB_FLOWID_INCARN(flowid)    (((flowid) & SCB_FLOWID_INCARN_MASK) >> \
    SCB_FLOWID_INCARN_SHIFT)

#define SCB_FLOWID_LOCAL_ASSERT(flowid)    \
    ASSERT(((flowid) & ~SCB_FLOWID_LOCAL_MASK) == 0)

/* Global ID across radios */
#define SCB_FLOWID_GLOBAL_SET(radio, incarn, flowid) \
    (((radio) << SCB_FLOWID_RADIO_SHIFT) | \
    ((incarn) << SCB_FLOWID_INCARN_SHIFT) | \
    (flowid))

#define SCB_AID(a)            ((a)->aid)
/* Special purpose API to access gloabl ID */
#define SCB_FLOWID_GLOBAL(a)        ((a)->flowid)
/* Genral USE API for scb to ID mapping */
#define SCB_FLOWID(a)            (SCB_FLOWID_GLOBAL(a) & SCB_FLOWID_LOCAL_MASK)
#define SCB_HOST_RINGID(a, prio)    ((a)->host_ringid[(prio)])

#ifdef WME
#define SCB_WME(a)        ((a)->flags & SCB_WMECAP)    /* Also implies WME_ENAB(wlc) */
#else
#define SCB_WME(a)        ((void)(a), FALSE)
#endif

#ifdef WLAMPDU
#define SCB_AMPDU(a)        ((a)->flags & SCB_AMPDUCAP)
#else
#define SCB_AMPDU(a)        FALSE
#endif

#ifdef WLAMSDU
#define SCB_AMSDU(a)        ((a)->flags & SCB_AMSDUCAP)
#define SCB_AMSDU_IN_AMPDU(a)    ((a)->flags2 & SCB2_AMSDU_IN_AMPDU_CAP)
#else
#define SCB_AMSDU(a)        FALSE
#define SCB_AMSDU_IN_AMPDU(a) FALSE
#endif

#define SCB_DTPC_CAP(scb)    ((scb)->flags & SCB_DTPCCAP)

#define SCB_HT_CAP(a)        (((a)->flags & SCB_HTCAP) != 0)
#define SCB_VHT_CAP(a)        (((a)->flags2 & SCB2_VHTCAP) != 0)
#define SCB_ISGF_CAP(a)        (((a)->flags & (SCB_HTCAP | SCB_NONGF)) == SCB_HTCAP)
#define SCB_NONGF_CAP(a)    (((a)->flags & (SCB_HTCAP | SCB_NONGF)) == \
                    (SCB_HTCAP | SCB_NONGF))
#define SCB_COEX_CAP(a)        ((a)->flags & SCB_COEX_MGMT)
#define SCB_STBC_CAP(a)        ((a)->flags & SCB_STBCCAP)
#define SCB_HT_LDPC_CAP(a)    (SCB_HT_CAP(a) && ((a)->flags2 & SCB2_LDPCCAP))
#define SCB_HT_PROP_RATES_CAP(a) (((a)->flags3 & SCB3_HT_PROP_RATES_CAP) != 0)
#define SCB_HT_PROP_RATES_CAP_SET(a) ((a)->flags3 |= SCB3_HT_PROP_RATES_CAP)

#ifdef WL11AC
#define SCB_OPER_MODE_NOTIF_CAP(a) ((a)->flags3 & SCB3_OPER_MODE_NOTIF)
#else
#define SCB_OPER_MODE_NOTIF_CAP(a) (0)
#endif /* WL11AC */

#ifdef WL11AX
#define SCB_HE_CAP(a)            (((a)->flags2 & SCB2_HECAP) != 0)
#define SCB_SET_HE_CAP(a)        ((a)->flags2 |= SCB2_HECAP)
#else
#define SCB_HE_CAP(a)        FALSE
#endif /* WL11AX */

#define SCB_IS_IBSS_PEER(a)    ((a)->flags & SCB_IBSS_PEER)
#define SCB_SET_IBSS_PEER(a)    ((a)->flags |= SCB_IBSS_PEER)
#define SCB_UNSET_IBSS_PEER(a)    ((a)->flags &= ~SCB_IBSS_PEER)

#if defined(PKTC) || defined(PKTC_DONGLE)
#define SCB_PKTC_ENABLE(a)    ((a)->flags3 |= SCB3_PKTC)
#define SCB_PKTC_DISABLE(a)    ((a)->flags3 &= ~SCB3_PKTC)
#define SCB_PKTC_ENABLED(a)    ((a)->flags3 & SCB3_PKTC)
#else
#define SCB_PKTC_ENABLE(a)
#define SCB_PKTC_DISABLE(a)
#define SCB_PKTC_ENABLED(a)    FALSE
#endif

#define SCB_QOS(a)        ((a)->flags & (SCB_WMECAP | SCB_HTCAP))

#ifdef WLP2P
#define SCB_P2P(a)        ((a)->flags2 & SCB2_P2P)
#else
#define SCB_P2P(a)        FALSE
#endif

#ifdef DWDS
#define SCB_DWDS_CAP(a)        ((a)->flags3 & SCB3_DWDS_CAP)
#define SCB_DWDS(a)        ((a)->flags2 & SCB2_DWDS_ACTIVE)
#else
#define SCB_DWDS(a)        FALSE
#define SCB_DWDS_CAP(a)        FALSE
#endif

#ifdef MULTIAP
#define SCB_MAP_CAP(a)        ((a)->flags3 & SCB3_MAP_CAP)
#define SCB_MAP_P2_CAP(a)    ((a)->flags3 & SCB3_MAP_P2_CAP)
#else
#define SCB_MAP_CAP(a)        FALSE
#define SCB_MAP_P2_CAP(a)    FALSE
#endif

#define SCB_ECSA_CAP(a)        (((a)->flags3 & SCB3_ECSA_SUPPORT) != 0)
#define SCB_DWDS_ACTIVATE(a)    ((a)->flags2 |= SCB2_DWDS_ACTIVE)
#define SCB_DWDS_DEACTIVATE(a)    ((a)->flags2 &= ~SCB2_DWDS_ACTIVE)

#define SCB_LEGACY_WDS(a)    (SCB_WDS(a) && !SCB_DWDS(a))

#define SCB_A4_DATA(a)        ((a)->flags3 & SCB3_A4_DATA)
#define SCB_A4_DATA_ENABLE(a)    ((a)->flags3 |= SCB3_A4_DATA)
#define SCB_A4_DATA_DISABLE(a)    ((a)->flags3 &= ~SCB3_A4_DATA)

#define SCB_A4_NULLDATA(a)    ((a)->flags3 & SCB3_A4_NULLDATA)
#define SCB_A4_8021X(a)        ((a)->flags3 & SCB3_A4_8021X)

#define SCB_MFP(a)        ((a) && ((a)->flags2 & SCB2_MFP))
#define SCB_MFP_ENABLE(a)    ((a)->flags2 |= SCB2_MFP)
#define SCB_MFP_DISABLE(a)    ((a)->flags2 &= ~SCB2_MFP)

#define SCB_SHA256(a)        ((a) && ((a)->flags2 & SCB2_SHA256))

#define SCB_1024QAM_CAP(a)    ((a)->flags3 & SCB3_1024QAM_CAP)

#define SCB_VHTMU(a)        (((a)->flags3 & (SCB3_VHTMU)) ? 1 : 0)
#define SCB_HEMMU(a)        (((a)->flags3 & (SCB3_HEMMU)) ? 1 : 0)
#define SCB_MU(a)        (((a)->flags3 & (SCB3_VHTMU | SCB3_HEMMU) ? 1 : 0))

#define SCB_VHTMU_ENABLE(a)    ((a)->flags3 |= SCB3_VHTMU)
#define SCB_VHTMU_DISABLE(a)    ((a)->flags3 &= ~SCB3_VHTMU)

#define SCB_HEMMU_ENABLE(a)    ((a)->flags3 |= SCB3_HEMMU)
#define SCB_HEMMU_DISABLE(a)    ((a)->flags3 &= ~SCB3_HEMMU)

#define SCB_DLOFDMA(a)        (((a)->flags3 & (SCB3_DLOFDMA)) ? 1 : 0)
#define SCB_DLOFDMA_ENABLE(a)    ((a)->flags3 |= SCB3_DLOFDMA)
#define SCB_DLOFDMA_DISABLE(a)    ((a)->flags3 &= ~SCB3_DLOFDMA)

#define SCB_ULOFDMA(a)        (((a)->flags3 & (SCB3_ULOFDMA)) ? 1 : 0)
#define SCB_ULOFDMA_ENABLE(a)    ((a)->flags3 |= SCB3_ULOFDMA)
#define SCB_ULOFDMA_DISABLE(a)    ((a)->flags3 &= ~SCB3_ULOFDMA)

#define SCB_ULMMU(a)        (((a)->flags3 & (SCB3_ULMMU)) ? 1 : 0)
#define SCB_ULMMU_ENABLE(a)    ((a)->flags3 |= SCB3_ULMMU)
#define SCB_ULMMU_DISABLE(a)    ((a)->flags3 &= ~SCB3_ULMMU)

#define SCB_ULMU_DISABLE(a)    do { SCB_ULMMU_DISABLE(a); SCB_ULOFDMA_DISABLE(a); } while (0)

#define SCB_SEQNUM(scb, prio)    (scb)->seqnum[(prio)]

#ifdef WL11K
#define SCB_RRM(a)        ((a)->flags3 & SCB3_RRM)
#else
#define SCB_RRM(a)        FALSE
#endif /* WL11K */

#define SCB_FTM(_a)        (((_a)->flags3 & (SCB3_FTM_INITIATOR | SCB3_FTM_RESPONDER)) != 0)
#define SCB_FTM_INITIATOR(_a)    (((_a)->flags3 & SCB3_FTM_INITIATOR) != 0)
#define SCB_FTM_RESPONDER(_a)    (((_a)->flags3 & SCB3_FTM_RESPONDER) != 0)

/* gets WLPKTTAG from p and checks if it is an AMSDU or not to return 1 or 0 respectively */
#define WLPKTTAG_AMSDU(p) (WLPKTFLAG_AMSDU(WLPKTTAG(p)) ? 1 : 0)

#ifdef WLSCB_HISTO
/* pass VHT rspec to get histo index (MCS + 12 * NSS) */
#define VHT2HISTO(rspec) (((rspec) & WL_RSPEC_VHT_MCS_MASK) + \
        12 * (((rspec & 0x70) >> WL_RSPEC_VHT_NSS_SHIFT) - 1))

/* pass HT rspec to get VHT histo index mapping; (or WL_NUM_VHT_RATES at most) */
#define HT2HISTO(rspec) (((rspec) & WL_RSPEC_RATE_MASK) >= WL_NUM_HT_RATES ? \
        (WL_NUM_VHT_RATES - 1) : (((rspec) & WL_RSPEC_RATE_MASK) % 8) + \
        12 * (((rspec) & WL_RSPEC_RATE_MASK) / 8))

/* array of 55 elements (0-54); elements provide mapping to index for
 *    1, 2, 5 (5.5), 6, 9, 11, 12, 18, 24, 36, 48, 54
 * eg.
 *    wlc_legacy_rate_index[1] is 0
 *    wlc_legacy_rate_index[2] is 1
 *    wlc_legacy_rate_index[5] is 2
 *    ...
 *    wlc_legacy_rate_index[48] is 10
 *    wlc_legacy_rate_index[54] is 11
 * For numbers in between (eg. 3, 4, 49, 50) maps to closest lower index
 * eg.
 *    wlc_legacy_rate_index[3] is 1 (same as 2Mbps)
 *    wlc_legacy_rate_index[4] is 1 (same as 2Mbps)
 */
extern uint8 wlc_legacy_rate_index[];
extern void wlc_scb_histo_add(wl_rate_histo_map2_t *map, ratespec_t rspec, int32 count);

/* pass legacy rspec to get index 0-11 */
#define LEGACY2HISTO(rspec) (((rspec) & WL_RSPEC_RATE_MASK) > 108 ? (WL_NUM_LEGACY_RATES - 1) : \
        wlc_legacy_rate_index[((rspec) & WL_RSPEC_RATE_MASK) >> 1])

/* maps rspec to histogram index */
#define RSPEC2HISTO(rspec) ((RSPEC_ISVHT(rspec) || RSPEC_ISHE(rspec) || ((rspec) & 0x80) != 0) ? \
        VHT2HISTO(rspec): (RSPEC_ISHT(rspec) ? HT2HISTO(rspec) : (\
        RSPEC_ISLEGACY(rspec) ? LEGACY2HISTO(rspec): (0))))

#define WLSCB_HISTO_ADD(map, rspec, count) wlc_scb_histo_add(&(map), (rspec), (count))

/* increment counter of recent rate index */
#define WLSCB_HISTO_INC_RECENT(map, count) ((map).arr[(map).recent_index] += (count))

/* increments rxmap for the rspec */
#define WLSCB_HISTO_RX(scb, rspec, count) do { \
    if ((scb)->histo) { \
        WLSCB_HISTO_ADD((scb)->histo->rx, rspec, count); \
    } \
} while (0)

/* increments rxmap for the recent rspec */
#define WLSCB_HISTO_RX_INC_RECENT(scb, count) do { \
    if ((scb)->histo) { \
        WLSCB_HISTO_INC_RECENT((scb)->histo->rx, count); \
    } \
} while (0)

/* increments txmap for the rspec by given count */
#define WLSCB_HISTO_TX(scb, rspec, count) do { \
    if ((scb)->histo) { \
        WLSCB_HISTO_ADD((scb)->histo->tx, rspec, count); \
    } \
} while (0)

/* checks for HT/VHT/HE before updating WLSCB_HISTO_TX() */
#define WLSCB_HISTO_TX_IF_HTVHTHE(scb, rspec, count) do { \
    if (((scb)->histo) && (((rspec) & WL_RSPEC_ENCODING_MASK) != 0)) { \
        WLSCB_HISTO_ADD((scb)->histo->tx, rspec, count); \
    } \
} while (0)

#else /* WLSCB_HISTO */
#define WLSCB_HISTO_RX(a, b, c) do { BCM_REFERENCE(a); BCM_REFERENCE(b); BCM_REFERENCE(c); \
} while (0)
#define WLSCB_HISTO_RX_INC_RECENT(a, b) do { BCM_REFERENCE(a); BCM_REFERENCE(b); } while (0)
#define WLSCB_HISTO_TX(a, b, c) do { BCM_REFERENCE(a); BCM_REFERENCE(b); BCM_REFERENCE(c); \
} while (0)
#define WLSCB_HISTO_TX_IF_HTVHTHE(a, b, c) do { BCM_REFERENCE(a); BCM_REFERENCE(b); \
BCM_REFERENCE(c); } while (0)
#endif /* WLSCB_HISTO */

#ifdef WLCNTSCB
#define WLCNTSCBINCR(a)            ((a)++)    /**< Increment by 1 */
#define WLCNTSCBDECR(a)            ((a)--)    /**< Decrement by 1 */
#define WLCNTSCBADD(a,delta)        ((a) += (delta)) /**< Increment by specified value */
#define WLCNTSCBSET(a,value)        ((a) = (value)) /**< Set to specific value */
#define WLCNTSCBVAL(a)            (a)    /**< Return value */
#define WLCNTSCB_INCR_LIMIT(a, limit)    do { (a) = ((a) + 1) & limit; } while (0)
#define WLCNTSCB_COND_SET(c, a, v)    do { if (c) (a) = (v); } while (0)
#define WLCNTSCB_COND_ADD(c, a, d)    do { if (c) (a) += (d); } while (0)
#define WLCNTSCB_COND_INCR(c, a)    do { if (c) (a) += (1); } while (0)
#define WLCNTSCB_COND_INCR_LIMIT(c, a, limit)    do { if (c) (a) = ((a) + 1) & limit; } while (0)
#else /* WLCNTSCB */
#define WLCNTSCBINCR(a)            /* No stats support */
#define WLCNTSCBDECR(a)            /* No stats support */
#define WLCNTSCBADD(a,delta)        /* No stats support */
#define WLCNTSCBSET(a,value)        /* No stats support */
#define WLCNTSCBVAL(a)        0    /* No stats support */
#define WLCNTSCB_INCR_LIMIT(a, limit)    /* No stats support */
#define WLCNTSCB_COND_SET(c, a, v)    /* No stats support */
#define WLCNTSCB_COND_ADD(c, a, d)    /* No stats support */
#define WLCNTSCB_COND_INCR(c, a)    /* No stats support */
#define WLCNTSCB_COND_INCR_LIMIT(c, a, limit)    /* No stats support */
#endif /* WLCNTSCB */

#if defined(CONFIG_TENDA_PRIVATE_WLAN) && defined(CONFIG_TENDA_WLAN_AUTOHIDE_SSID)    /* The meaning of the td_auto_hide value */
#define SCB_AUTO_HIDE_OFF       0
#define SCB_AUTO_HIDE_ON        1
#define SCB_AUTO_HIDE        2
#define SCB_USER_HIDE        3    /* The SSID was hidden by the user */
#endif

#define SCB_SHORT_TIMEOUT     60    /**< # seconds of idle time after which we will reclaim an
                    * authenticated SCB if we would otherwise fail
                    * an SCB allocation.
                    */

extern void wlc_scb_switch_band(wlc_info_t *wlc, struct scb *scb, int new_bandunit,
    wlc_bsscfg_t *bsscfg);

extern struct scb * wlc_scbfind_dualband(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,
    const struct ether_addr *addr);

extern void wlc_scb_cleanup_unused(wlc_info_t *wlc);

typedef struct {
    struct scb *scb;
    uint8    oldstate;
} scb_state_upd_data_t;
typedef void (*scb_state_upd_cb_t)(void *arg, scb_state_upd_data_t *data);
extern int wlc_scb_state_upd_register(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg);
extern int wlc_scb_state_upd_unregister(wlc_info_t *wlc, scb_state_upd_cb_t fn, void *arg);

extern void wlc_scb_bss_state_upd(void *ctx, bsscfg_state_upd_data_t *evt_data);

#if defined(BCMDBG)
void wlc_scb_dump_scb(wlc_info_t *wlc, wlc_bsscfg_t *cfg, struct scb *scb,
    struct bcmstrbuf *b, int idx);
#endif

void wlc_scbfind_delete(wlc_info_t *wlc, wlc_bsscfg_t *bsscfg,    struct ether_addr *ea);

#if defined(STA) && defined(DBG_BCN_LOSS)
int wlc_get_bcn_dbg_info(struct wlc_bsscfg *cfg, struct ether_addr *addr,
    struct wlc_scb_dbg_bcn *dbg_bcn);
#endif

extern int wlc_scb_wlc_cfg_update(wlc_info_t *wlc_from, wlc_info_t *wlc_to, wlc_bsscfg_t *cfg_to,
    struct scb *scb);

#if defined(BCM_PKTFWD_FLCTL)
extern void wlc_scb_get_link_credits(wlc_info_t *wlc, wlc_if_t *wlcif, uint8 *addr,
    uint16 cfp_flowid, int32 *credits);
extern int wlc_scb_link_update(wlc_info_t *wlc, wlc_if_t *wlcif, uint8 *addr, uint16 *cfp_flowid);
#endif /* BCM_PKTFWD_FLCTL */

#ifdef WL_PS_STATS
#define PS_UPD_HISTOGRAM(val, scb)    \
{\
    if ((int)(val) > 5000) {    \
        scb->ps_dur5++;     \
    } else if ((int)(val) > 2000) {    \
        scb->ps_dur2_5++;     \
    } else if ((int)(val) > 500) {    \
        scb->ps_dur500_2++;    \
    } else if ((int)(val) > 200) {    \
        scb->ps_dur200_500++;    \
    } else {            \
        scb->ps_dur0_200++;    \
    } \
}
#endif /* WL_PS_STATS */
extern void wlc_scb_amt_delink(wlc_info_t *wlc, int amt_idx, uint16 flowid);
extern int wlc_scb_amt_link(wlc_info_t *wlc, int amt_idx, const struct ether_addr *amt_addr);
extern uint16 wlc_scb_amt_linkid_get(wlc_info_t *wlc, int amt_idx);
extern struct scb* BCMFASTPATH wlc_scb_amt_lookup(wlc_info_t *wlc, uint16 amt_idx);
extern void wlc_scb_host_ring_link(wlc_info_t *wlc, uint16 ringid, uint8 tid,
    struct scb *scb,  uint16* flowid);
#ifdef BCMPCIEDEV
extern int wlc_scb_flow_ring_tid_delink(wlc_info_t *wlc,
    struct scb *scb, uint8 tid);
#endif /* BCMPCIEDEV */
#ifdef BCMDBG
extern struct scb* BCMFASTPATH wlc_scb_flowid_global_lookup(uint16 flowid_global, void* call_site);
#else
extern struct scb* BCMFASTPATH wlc_scb_flowid_global_lookup(uint16 flowid_global);
#endif /* BCMDBG */
extern struct scb* BCMFASTPATH wlc_scb_flowid_lookup(wlc_info_t *wlc, uint16 flowid);
extern uint16 wlc_scb_flowid_addr_lookup(wlc_info_t *wlc, struct ether_addr *ea);
extern int wlc_scb_amt_incarnation_id_get(wlc_info_t *wlc, int amt_idx);
extern void wlc_scb_pause_traffic(wlc_info_t* wlc, struct scb *scb, uint8 ac);
extern void wlc_scb_resume_traffic(wlc_info_t* wlc, struct scb *scb, uint8 ac);
#ifdef WLC_MEDIAN_RATE_REPORT
extern ratespec_t wlc_scb_get_median_rate(ratespec_t *arr, int arrsz);
#endif /* WLC_MEDIAN_RATE_REPORT */

#endif /* _wlc_scb_h_ */
