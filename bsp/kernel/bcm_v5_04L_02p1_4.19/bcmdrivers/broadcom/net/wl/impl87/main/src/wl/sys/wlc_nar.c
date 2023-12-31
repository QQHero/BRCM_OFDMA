/**
 * Non Aggregated Regulation
 *
 * @file
 * @brief
 * This module contains the NAR transmit module. Hooked in the transmit path at the same
 * level as the A-MPDU transmit module, it provides balance amongst MPDU and AMPDU traffic
 * by regulating the number of in-transit packets for non-aggregating stations.
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
 * $Id: wlc_nar.c 810380 2022-04-06 11:30:33Z $
 *
 */

/*
 * Include files.
 *
 */
#include <wlc_cfg.h>
#include <osl.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <siutils.h>
#include <d11.h>
#include <wlioctl.h>
#include <wlc_key.h>
#include <wlc_bsscfg.h>
#include <wlc.h>
#include <wlc_scb.h>
#include <wlc_scb_ratesel.h>	/* wlc_scb_ratesel_get_primary() */
#include <wlc_tso.h>		/* wlc_tso_hdr_length(), for PKTQ_LOG */
#include <wlc_ampdu.h>
#include <wlc_rate.h>
#include <wlc_nar.h>
#include <wlc_pcb.h>

#ifdef WLATF_BARE
#include <wlc_airtime.h>
#include <wlc_prot.h>
#include <wlc_prot_g.h>
#include <wlc_prot_n.h>
#endif /* WLATF_BARE */

#include <wlc_txmod.h>
#include <wlc_perf_utils.h>
#include <wlc_dump.h>
#include <wlc_tx.h>
#include <wlc_pktc.h>
#include <wlc_pspretend.h>
#include <wlc_twt.h>

#ifdef WLTAF
#include <wlc_pub.h>
#include <wlc_taf.h>
#endif

#include <wlfc_proto.h>

#ifdef WLAMSDU_TX
#include <wlc_amsdu.h>
#endif

#include <wlc_apps.h>

static void wlc_nar_release_all_from_queue(wlc_nar_info_t *nit, struct scb *scb, int prec);

/*
 * Initial parameter settings. Most can be tweaked by wl debug commands. Some we may want to
 * make tuneables, others bcminternal as they should not really be used in production.
 */
#if defined(DONGLEBUILD)
#define NAR_QUEUE_LENGTH	128	/**< Configurable - size for station pkt queues */
#else
/* As there is no backup queue in NIC mode increasing the nar queue size to 512 */
#define NAR_QUEUE_LENGTH	512	/**< Configurable - size for station pkt queues */
#endif /* DONGLEBUILD */
#define NAR_MIN_QUEUE_LEN	128	/**< Fixed - minimum pkt queue length */

#ifndef NAR_MAX_TRANSIT_PACKETS
#define NAR_MAX_TRANSIT_PACKETS	64	/**< Configurable - Limit of transit packets */
#endif

#define NAR_MAX_RELEASE		32	/**< Configurable - number of pkts to release at once */
#define NAR_TX_STUCK_TIMEOUT	5	/**< Fixed - seconds before reset of in-transit count */

/*
 * Maximum in flight time in microseconds
 */
#define NAR_DEFAULT_TRANSIT_US	4000

#if defined(BCMDBG)		/* Only keep stats if we can display them (via dump cmd) */
#define NAR_STATS (1)
#define WL_NAR_MSG(m) WL_SPARE1(m)	/* WL_TRACE(m) */
#else
#undef NAR_STATS
#define WL_NAR_MSG(m)			/* nada */
#endif /* BCMDBG */

#define WLC_NAR_AMSDU(wlc, scb, tid) \
	((AMSDU_TX_ENAB((wlc)->pub) && \
	(SCB_AMSDU(scb) != 0) && AMSDU_TX_AC_ENAB((wlc)->ami, (tid))) && \
	(RSPEC_ISVHT((scb)->scb_stats.tx_rate) || RSPEC_ISHE((scb)->scb_stats.tx_rate)) && \
	(((scb)->wsec & (WEP_ENABLED | TKIP_ENABLED)) == 0))

#if defined(NAR_STATS)
/*
 * If we do statistics, store the counters in an array, indexed by the enum below.
 * Makes it easy to display them.
 */
enum {
	NAR_COUNTER_PACKETS_RECEIVED	= 0,	/**< packets handed to us for processing */
	NAR_COUNTER_PACKETS_DROPPED	= 1,	/**< packets we failed to queue */
	NAR_COUNTER_PACKETS_QUEUED	= 2,	/**< packets we successfully queued */
	NAR_COUNTER_PACKETS_PASSED_ON	= 3,	/**< packets we passed on for transmission */
	NAR_COUNTER_PACKETS_TRANSMITTED = 4,	/**< packets whose transmission was notified */
	NAR_COUNTER_PACKETS_DEQUEUED	= 5,	/**< packets dequeued from queue */
	NAR_COUNTER_AMPDU_TXS_IGNORED	= 6,	/**< xmit status for ampdu and ignoring ampdu */
	NAR_COUNTER_KICKSTARTS_IN_WD	= 7,	/**< Transmission kickstarted in watchdog */
	NAR_COUNTER_KICKSTARTS_IN_INT	= 8,	/**< Transmission kickstarted in interrupt */
	NAR_COUNTER_KICKSTARTS_IN_TX	= 9,	/**< Transmission kickstarted in tx function */
	NAR_COUNTER_KICKSTARTS_IN_TX_QF = 10,	/**< Transmission kickstarted in tx function */
	NAR_COUNTER_PEAK_TRANSIT	= 11,	/**< Peak in-transit packets */
	NAR_COUNTER_TXRESET		= 12,	/**< Number of in-transit counter resets */
	NAR_COUNTER_TX_STATUS		= 13,	/**< Number of dotxstatus events */
	NAR_COUNTER_NCONS		= 14,	/**< Packets consumed */
#if defined(PKTQ_LOG)
	NAR_COUNTER_ACKED		= 15,	/**< Packets acked */
	NAR_COUNTER_NOT_ACKED		= 16,	/**< Packets not acked */
#endif /* PKTQ_LOG */
	NAR_COUNTER_AGGREGATING_CHANGED = 17,	/**< Number of block ack status changes */
	NAR_COUNTER_PACKETS_AGGREGATING = 18,	/**< Number of packets passed to AMPDU txmod */
	NAR_COUNTER_MAX				/* end marker. Remember to update cmap[] below */
};

typedef struct {
	uint32	counter[NAR_COUNTER_MAX];
} nar_stats_t;

static void
nar_stats_dump(nar_stats_t * stats, struct bcmstrbuf *b)
{
	int i;
	static struct {
		int		cval;
		const char	*cname;
	} cmap[NAR_COUNTER_MAX] = {
		{NAR_COUNTER_PACKETS_RECEIVED, "Packets Received"},
		{NAR_COUNTER_PACKETS_DROPPED, "Packets Dropped (Q full)"},
		{NAR_COUNTER_PACKETS_AGGREGATING, "Aggregating passed on"},
		{NAR_COUNTER_AGGREGATING_CHANGED, "Aggregation changed"},
		{NAR_COUNTER_PACKETS_QUEUED, "Packets Queued"},
		{NAR_COUNTER_PACKETS_DEQUEUED, "Packets Dequeued"},
		{NAR_COUNTER_PACKETS_PASSED_ON, "Packets passed down"},
		{NAR_COUNTER_PACKETS_TRANSMITTED, "Packets tx done"},
		{NAR_COUNTER_AMPDU_TXS_IGNORED, "tx done ignored (ampdu)"},
		{NAR_COUNTER_KICKSTARTS_IN_WD, "kickstarts in watchdog"},
		{NAR_COUNTER_TXRESET, "tx stuck reset"},
		{NAR_COUNTER_KICKSTARTS_IN_INT, "kickstarts in tx done"},
		{NAR_COUNTER_KICKSTARTS_IN_TX, "kickstarts in tx"},
		{NAR_COUNTER_KICKSTARTS_IN_TX_QF, "kickstarts in tx, Q full"},
		{NAR_COUNTER_PEAK_TRANSIT, "Peak transit packets"},
		{NAR_COUNTER_TX_STATUS, "TX Done Events"},
		{NAR_COUNTER_NCONS, "NCONS"},
#if defined(PKTQ_LOG)
		{NAR_COUNTER_ACKED, "Packets ACKed"},
		{NAR_COUNTER_NOT_ACKED, "Packets not ACKed"},
#endif /* PKTQ_LOG */
	};

	for (i = 0; i < NAR_COUNTER_MAX; ++i) {
		bcm_bprintf(b, "%25s : %9u%c", cmap[i].cname,
			stats->counter[cmap[i].cval], (i & 1) ? '\n' : ' ');
	}
	bcm_bprintf(b, "\n");
}

#define NAR_STATS_DUMP(sptr, bptr) nar_stats_dump(sptr, bptr)
#define NAR_STATS_GET(ctx, ctr) (ctx)->stats.counter[NAR_COUNTER_##ctr]
#define NAR_STATS_PEAK(ctx, ctr, val) do { \
	if ((val) > (ctx)->stats.counter[NAR_COUNTER_##ctr]) { \
		(ctx)->stats.counter[NAR_COUNTER_##ctr] = (val); \
	} \
} while (0)

#define NAR_STATS_INC(scb, ctr) do { \
	(scb)->stats.counter[NAR_COUNTER_##ctr]++; \
	(scb)->nit->stats.counter[NAR_COUNTER_##ctr]++; \
} while (0)

#define NAR_STATS_N_INC(scb, ctr, count) do { \
	(scb)->stats.counter[NAR_COUNTER_##ctr] += (count); \
	(scb)->nit->stats.counter[NAR_COUNTER_##ctr] += (count); \
} while (0)

#else /* not NAR_STATS */

#define NAR_STATS_DUMP(sptr, b)	/* nada */
#define NAR_STATS_INC(scb, ctr)	/* nada */
#define NAR_STATS_N_INC(scb, ctr, count)  /* nada */
#define NAR_STATS_GET(ctx, ctr)	/* nada */
#define NAR_STATS_PEAK(ctx, ctr, val)	/* nada */

#endif /* NAR_STATS */

/*
 * The next two are not for statistics, they are used for control
 * purposes: do not comment out!
 */

#define NAR_COUNTER_INC(scb, ctr) do { (scb)->ctr++; ASSERT((scb)->ctr != 0); \
	(scb)->nit->ctr++; } while (0)

#define NAR_COUNTER_DEC(scb, ctr) do { ASSERT((scb)->ctr > 0); (scb)->ctr--; \
	(scb)->nit->ctr--; } while (0)

/* Find basic rate for a given rate */
#define NAR_BASIC_RATE(band, rspec)	((band)->basic_rate[RSPEC_REFERENCE_RATE(rspec)])
/*
 * Driver level data.
 */
struct wlc_nar_info {
	wlc_info_t	*wlc;			/**< backlink to wlc */
	int		scb_handle;		/**< handle to scb structure */
	uint32		transit_packet_limit;	/**< per-scb transit limit */
	uint32		packets_in_transit;	/**< #packets in transit, waiting for tx done */
	uint16		release_at_once;	/**< per-scb release limit */
	uint16		queue_length;		/**< Length of per-scb pktq */
#if defined(NAR_STATS)
	nar_stats_t	stats;			/**< various counters */
#endif
#ifdef WLATF_BARE
	uint		txq_time_allowance_us;	/**< Time allowed per tid for packets in transit */
#endif
};

/*
 * Per SCB data, malloced, to which a pointer is stored in the SCB cubby.
 * Note these are only allocated if the SCB is not ignored (ie, internal or AMPDU) and
 * the feature is enabled - in all other cases the pointer will be null. Ye be warned.
 */
typedef struct {

	wlc_nar_info_t	*nit;			/**< backlink to nit */
	struct scb	*scb_bl;		/**< backlink to current scb */
	osl_t		*osh;			/**< backlink to osh */

	struct pktq	tx_queue;		/**< multi-priority packet queue */
	uint32		tx_stuck_time;		/**< in seconds - used for watchdog cleanup */

	uint32		packets_in_transit;	/**< not just stats - used for control */
	uint16          pkts_intransit_prec[WLC_PREC_COUNT];

#if defined(NAR_STATS)
	nar_stats_t	stats;			/**< various counters */
#endif
#ifdef WLTAF
	uint16		taf_prec_active;
	uint16		taf_prec_enabled;
#endif
#ifdef WLATF_BARE
	uint		packet_airtime_us[WLC_PREC_COUNT]; /**< Released and inflight airtime */
	uint		txq_time_allowance_us;	/**< Packet airtime release limit (us) */

	/* Cached vars used to reduce calculation effort */
	uint		last_packet_rspec;	/**< Estimated rspec of last packet */
	uint		last_packet_nbytes;	/**< Size of last released packet in bytes */
	uint		last_packet_airtime_us;	/**< Estimated airtime of last released packet */
	uint		last_packet_overhead_us;
	uint		last_ack_rspec;		/**< Most recent lookup of ACK packet rate */
	uint		last_ctrl_rspec;
	uint		last_state_flags;
	bool		last_state_nProt;
	bool		last_state_gProt;
#endif

} nar_scb_cubby_t;

#define SCB_NAR_CUBBY_PTR(nit, scb) ((nar_scb_cubby_t **)(SCB_CUBBY((scb), (nit)->scb_handle)))
#define SCB_NAR_CUBBY(nit, scb) (*SCB_NAR_CUBBY_PTR(nit, scb))

#if defined(NAR_STATS)
static void
nar_stats_clear_all(wlc_nar_info_t * nit)
{
	struct scb     *scb;
	struct scb_iter scbiter;
	nar_scb_cubby_t *cubby;

	memset(&nit->stats, 0, sizeof(nit->stats));

	FOREACHSCB(nit->wlc->scbstate, &scbiter, scb) {

		cubby = SCB_NAR_CUBBY(nit, scb);

		if (cubby) {
			memset(&cubby->stats, 0, sizeof(cubby->stats));
		}
	}
}
#endif /* NAR_STATS */

/*
 * Forward declarations.
 */
static int BCMFASTPATH wlc_nar_release_from_queue(nar_scb_cubby_t * cubby, int prec);
static bool BCMFASTPATH wlc_nar_fair_share_reached(nar_scb_cubby_t * cubby, int prec);
static void BCMFASTPATH wlc_nar_pkt_freed(wlc_info_t *wlc, void *pkt, uint txs);
static void wlc_nar_scbcubby_exit(void *handle, struct scb *scb);
static void BCMFASTPATH wlc_nar_transmit_packet(void *handle, struct scb *, void *pkt, uint prec);
static void wlc_nar_flush_scb_tid(void *handle, struct scb *scb, uint8 tid);
static uint BCMFASTPATH wlc_nar_count_queued_packets(void *handle);
static INLINE uint16 wlc_nar_txq_avail(wlc_info_t* wlc, struct scb *scb, uint prec);

static const txmod_fns_t nar_txmod_fns = {
	wlc_nar_transmit_packet,
	wlc_nar_count_queued_packets,
	wlc_nar_flush_scb_tid,
	NULL, NULL
};

#if defined(PKTQ_LOG)
/**
 * PKTQ_LOG helper function, called by the "wl pktq_stats -m<macaddress>" command.
 * @return   The address of the (multi-prio) pktq, or NULL if no pktq exists for the passed scb.
 */
struct pktq *
wlc_nar_prec_pktq(wlc_info_t * wlc, struct scb *scb)
{
	wlc_nar_info_t *nit;
	nar_scb_cubby_t *cubby;

	nit = wlc->nar_handle;

	cubby = SCB_NAR_CUBBY(nit, scb);

	return ((cubby) ? &cubby->tx_queue : NULL);

}
#endif /* PKTQ_LOG */

/*
 * Module iovar handling.
 */
enum wlc_nar_iov {
	IOV_NAR_TRANSIT_LIMIT = 3,	/**< max packets in transit allowed */
	IOV_NAR_QUEUE_LEN = 4,		/**< per station queue length */
	IOV_NAR_RELEASE = 5,		/**< # of packets to release at once */
	IOV_NAR_CLEAR_DUMP = 7,		/* Clear statistics (aka dump, like for ampdu) */
	IOV_NAR_LAST
};

static const bcm_iovar_t nar_iovars[] = {
	{"nar_transit_limit", IOV_NAR_TRANSIT_LIMIT, 0, 0, IOVT_UINT32, 0},
#if defined(NAR_STATS)
	{"nar_clear_dump", IOV_NAR_CLEAR_DUMP, 0, 0, IOVT_VOID, 0},
#endif
	{NULL, 0, 0, 0, 0, 0}
};

static int
wlc_nar_doiovar(void *handle, uint32 actionid,
	void *params, uint plen, void *arg,
	uint alen, uint vsize, struct wlc_if *wlcif)
{
	wlc_nar_info_t *nit = handle;
	int32           int_val = 0;
	int32          *ret_int_ptr = arg;
	int             status = BCME_OK;

	if (plen >= sizeof(int_val)) {
		memcpy(&int_val, params, sizeof(int_val));
	}

	switch (actionid) {

		case IOV_GVAL(IOV_NAR_TRANSIT_LIMIT):
			*ret_int_ptr = nit->transit_packet_limit;
			break;

		case IOV_SVAL(IOV_NAR_TRANSIT_LIMIT):
			nit->transit_packet_limit = MAX(1, int_val);
			break;

#if defined(NAR_STATS)
		case IOV_SVAL(IOV_NAR_CLEAR_DUMP):
			nar_stats_clear_all(nit);
			break;
#endif /* NAR_STATS */

		default:
			status = BCME_UNSUPPORTED;
			break;

	}
	return status;
}

/*
 * Module attach and detach functions.
 */
static void
wlc_nar_watchdog(void *handle)
{
	wlc_nar_info_t *nit = handle;
	wlc_info_t     *wlc = nit->wlc;
	struct scb     *scb;
	struct scb_iter scbiter;
	nar_scb_cubby_t *cubby;
	int             prec;
	bool            stuck;
#ifdef WLTAF
	bool taf_enabled = FALSE;
	bool taf_in_use = wlc_taf_nar_in_use(wlc->taf_handle, &taf_enabled);
	bool taf_blocked = wlc_taf_scheduler_blocked(wlc->taf_handle);
#else
	const bool taf_in_use = FALSE;
#endif
	/*
	 * XXX Better safe than sorry, or am I being too paranoid here ?
	 * If there are packets in one or more SCB queues, this may be normal, or
	 * they may have been forgotten somehow. Kickstart the queue emptying.
	 * Likewise, if an SCB has changed to aggregating, clean up and forget it.
	 */

	FOREACHSCB(wlc->scbstate, &scbiter, scb) {

		cubby = SCB_NAR_CUBBY(nit, scb);
		if (!cubby) {
			continue;
		}
#ifdef WLTAF
		for (prec = 0; taf_enabled && !taf_blocked && cubby->taf_prec_enabled &&
			(prec < WLC_PREC_COUNT); ++prec) {

			/* bitwise AND as this is a bitmask */
			bool remove = (cubby->taf_prec_enabled & (1 << prec) &
				~cubby->taf_prec_active) ? TRUE : FALSE;

			if (!remove) {
				continue;
			}

			/* tell TAF it can remove this context as it is not in use by NAR */
			wlc_taf_link_state(wlc->taf_handle, scb, TAF_PREC(prec), TAF_NAR,
				TAF_LINKSTATE_REMOVE);
			cubby->taf_prec_enabled &= ~(1 << prec);

			WL_TAFF(wlc, MACF" prec %u remove\n", ETHER_TO_MACF(scb->ea), prec);

			if (cubby->taf_prec_enabled == 0) {
				wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NAR,
					NULL, TAF_SCBSTATE_SOURCE_DISABLE);
			}
		}
#endif /* WLTAF */
		if (!taf_in_use && pktq_n_pkts_tot(&cubby->tx_queue)) {
			/* If there has been no tx activity for a while, we
			 * may have some stuck in transit counts (ie, because
			 * the lower layers dropped the packets). Clean up
			 * before trying to dequeue.
			 */
			stuck = FALSE;
			for (prec = 0; prec < WLC_PREC_COUNT; ++prec) {
				if (wlc_nar_fair_share_reached(cubby, prec)) {
					stuck = TRUE;
					if (cubby->tx_stuck_time > NAR_TX_STUCK_TIMEOUT) {
						NAR_STATS_INC(cubby, TXRESET);
						WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %d"
							" NAR intransit reset.\n",
							wlc->pub->unit,
							WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
							__FUNCTION__,
							ETHER_TO_MACF(scb->ea),
							prec));
						nit->packets_in_transit -=
							cubby->pkts_intransit_prec[prec];
						cubby->packets_in_transit -=
							cubby->pkts_intransit_prec[prec];
						cubby->pkts_intransit_prec[prec] = 0;
#ifdef WLATF_BARE
						cubby->packet_airtime_us[prec] = 0;
#endif /* WLATF_BARE */
					}
				}
			}
			/* struck is set when packets of stuck in any precedence queues */
			if (stuck) {
					++cubby->tx_stuck_time;
					WL_NAR_MSG(("wl%d.%d: %s: "MACF": NAR stuck %d\n",
						wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
						__FUNCTION__, ETHER_TO_MACF(scb->ea),
						cubby->tx_stuck_time));
			} else {
				/* Below fair share, so not stuck. */
				cubby->tx_stuck_time = 0;
			}

			for (prec = 0; prec < WLC_PREC_COUNT; ++prec) {

				if (wlc_nar_release_from_queue(cubby, prec)) {
					/* Released some, so not stuck. */
					cubby->tx_stuck_time = 0;
					NAR_STATS_INC(cubby, KICKSTARTS_IN_WD);
				} /* released some packets */
			} /* for each precedence */
		} /* packets in queue */
	} /* for each SCB */
}

static void
wlc_nar_reset_release_state(nar_scb_cubby_t *cubby)
{
	memset(cubby->pkts_intransit_prec, 0, sizeof(cubby->pkts_intransit_prec));
	cubby->packets_in_transit = 0;
#ifdef WLATF_BARE
	cubby->last_packet_rspec = 0;
	cubby->last_packet_nbytes = 0;
	cubby->last_packet_airtime_us = 0;
	memset(cubby->packet_airtime_us, 0, (sizeof(uint32) * WLC_PREC_COUNT));
	cubby->last_ack_rspec = 0;
	cubby->last_ctrl_rspec = 0;
	cubby->last_state_nProt = FALSE;
	cubby->last_state_gProt = FALSE;
	cubby->last_state_flags = 0;
#endif
}

void
wlc_nar_flush_scb_queues(wlc_nar_info_t * nit, struct scb *scb)
{
	uint8 tid;

	for (tid = 0; tid < NUMPRIO; tid++) {
		wlc_nar_flush_scb_tid(nit, scb, tid);
	}
}

static void
wlc_nar_flush_all_queues(wlc_nar_info_t * nit)
{
	struct scb     *scb;
	struct scb_iter scbiter;
	FOREACHSCB(nit->wlc->scbstate, &scbiter, scb) {

		wlc_nar_flush_scb_queues(nit, scb);

	}
}

/*
 * Interface going down. Flush queues and clean up a.
 */
static int
wlc_nar_down(void *handle)
{
	wlc_nar_info_t *nit = handle;

	wlc_nar_flush_all_queues(nit);

	nit->packets_in_transit = 0;

	return BCME_OK;
}

#if defined(BCMDBG)
static int
wlc_nar_dump(void *handle, struct bcmstrbuf *b)
{
	wlc_nar_info_t *nit = handle;
	nar_scb_cubby_t *cubby;
	struct scb     *scb;
	struct scb_iter scbiter;
	int             i;
	int total_pkts = 0;

#define _VPRINT(nl, s, v) bcm_bprintf(b, "%25s : %9u%c", s, v, (nl++&1) ? '\n':' ')

	i = 0;
	_VPRINT(i, "station in-transit limit", nit->transit_packet_limit);
	_VPRINT(i, "packet queue length", nit->queue_length);
	FOREACHSCB(nit->wlc->scbstate, &scbiter, scb) {
		cubby = SCB_NAR_CUBBY(nit, scb);
		if (!cubby) {
			continue;
		}
		total_pkts +=  pktq_n_pkts_tot(&cubby->tx_queue);
	}

	_VPRINT(i, "packets now in queues", total_pkts);
	_VPRINT(i, "packets now in transit", nit->packets_in_transit);
	bcm_bprintf(b, "\n");

	NAR_STATS_DUMP(&nit->stats, b);

	/*
	 * Dump all SCBs.
	 */
	FOREACHSCB(nit->wlc->scbstate, &scbiter, scb) {

		bcm_bprintf(b, "-------- Station : "MACF", %s, rate %dkbps\n",
			ETHER_TO_MACF(scb->ea),
			SCB_AMPDU(scb) ? "AMPDU Capable" : "Legacy",
			wf_rspec_to_rate(wlc_scb_ratesel_get_primary(nit->wlc, scb, NULL)));

		cubby = SCB_NAR_CUBBY(nit, scb);
		if (!cubby)
			continue;

		i = 0;
		_VPRINT(i, "packets in station queues", pktq_n_pkts_tot(&cubby->tx_queue));
		_VPRINT(i, "packets in transit", cubby->packets_in_transit);
		_VPRINT(i, "seconds in transit", cubby->tx_stuck_time);
		for (i = 0; i < WLC_PREC_COUNT; i ++)
		{
			bcm_bprintf(b, "prec [%d] pktq_prec[%d] intransit[%d]",
					i, pktqprec_n_pkts(&cubby->tx_queue, i),
					cubby->pkts_intransit_prec[i]);
			bcm_bprintf(b, "\n");
		}
		bcm_bprintf(b, "\n");

		NAR_STATS_DUMP(&cubby->stats, b);
	}

#undef _VPRINT

	return BCME_OK;
}
#endif /* BCMDBG */

/*
 * Initialise module parameters.
 */

static void
wlc_nar_module_init(wlc_nar_info_t * nit)
{
	nit->queue_length = NAR_QUEUE_LENGTH;

	/*
	 * Set the transit limit to be the same as the AMPDU limit.
	 */

	nit->transit_packet_limit = NAR_MAX_TRANSIT_PACKETS;

	nit->release_at_once = NAR_MAX_RELEASE;

#ifdef WLATF_BARE
	nit->txq_time_allowance_us = NAR_DEFAULT_TRANSIT_US;
#endif /* WLATF_BARE */
}

/*
 * SCB cubby functions.
 */
static uint
wlc_nar_scbcubby_secsz(void *handle, struct scb *scb)
{
	uint size = 0;

	if (scb && !SCB_INTERNAL(scb)) {
		size = sizeof(nar_scb_cubby_t);
	}
	return size;
}

static int
wlc_nar_scbcubby_init(void *handle, struct scb *scb)
{
	wlc_nar_info_t *nit = handle;

	*SCB_NAR_CUBBY_PTR(nit, scb) = wlc_scb_sec_cubby_alloc(nit->wlc, scb,
		wlc_nar_scbcubby_secsz(handle, scb));
	if (*SCB_NAR_CUBBY_PTR(nit, scb) != NULL) {
		nar_scb_cubby_t *cubby = *SCB_NAR_CUBBY_PTR(nit, scb);
		/* Initialise the cubby. */
		cubby->nit = nit;
		cubby->osh = nit->wlc->osh;
		cubby->scb_bl = scb;	/* save the scb back link */
#ifdef WLATF_BARE
		cubby->txq_time_allowance_us = nit->txq_time_allowance_us;
#endif /* WLATF_BARE */

		wlc_nar_reset_release_state(cubby);

		pktq_init(&cubby->tx_queue, WLC_PREC_COUNT, nit->queue_length);

		*SCB_NAR_CUBBY_PTR(nit, scb) = cubby;

		wlc_txmod_config(nit->wlc->txmodi, scb, TXMOD_NAR);
	}
	return BCME_OK;
}

/*
 * wlc_nar_scbcubby_exit() - clean up and strip a cubby from an scb.
 *
 * Called either by scb functions, or from our internal watchdog when an SCB has become
 * aggregating, to release anything associated with the cubby and the cubby itself, and
 * clear the scb cubby pointer.
 *
 */
static void
wlc_nar_scbcubby_exit(void *handle, struct scb *scb)
{
	wlc_nar_info_t *nit = handle;
	nar_scb_cubby_t* cubby = *SCB_NAR_CUBBY_PTR(nit, scb);

	wlc_nar_scb_close_link(nit->wlc, scb);

	if (cubby != NULL) {
		wlc_txmod_unconfig(nit->wlc->txmodi, scb, TXMOD_NAR);

		wlc_nar_flush_scb_queues(nit, scb);

#ifdef PKTQ_LOG
		wlc_pktq_stats_free(nit->wlc, &cubby->tx_queue);
#endif
		wlc_scb_sec_cubby_free(nit->wlc, scb, cubby);

		*SCB_NAR_CUBBY_PTR(nit, scb) = NULL;
	}
}

/*
 * Module initialisation and cleanup.
 */
wlc_nar_info_t *
BCMATTACHFN(wlc_nar_attach) (wlc_info_t * wlc)
{
	wlc_nar_info_t *nit;
	scb_cubby_params_t cubby_params;
	/*
	 * Allocate and initialise our main structure.
	 */
	nit = MALLOCZ(wlc->pub->osh, sizeof(wlc_nar_info_t));
	if (!nit) {
		WL_ERROR(("wl%d: %s: out of mem, malloced %d bytes\n",
				wlc->pub->unit, __FUNCTION__, MALLOCED(wlc->pub->osh)));
		goto fail;
	}

	/* save backlink to wlc */
	nit->wlc = wlc;

	/* set up module parameters */
	wlc_nar_module_init(nit);

	/* register module */
	if (wlc_module_register(wlc->pub, nar_iovars, "nar", nit,
		wlc_nar_doiovar, wlc_nar_watchdog, NULL, wlc_nar_down)) {
		WL_ERROR(("wl%d: %s: wlc_module_register failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	/* reserve cubby in the scb container for per-scb private data */
	bzero(&cubby_params, sizeof(cubby_params));

	cubby_params.context = nit;
	cubby_params.fn_init = wlc_nar_scbcubby_init;
	cubby_params.fn_deinit = wlc_nar_scbcubby_exit;
	cubby_params.fn_secsz = wlc_nar_scbcubby_secsz;
	nit->scb_handle = wlc_scb_cubby_reserve_ext(wlc, sizeof(nar_scb_cubby_t *),
		&cubby_params);

	if (nit->scb_handle < 0) {
		WL_ERROR(("%s: wlc_scb_cubby_reserve failed\n", __FUNCTION__));
		goto fail;
	}

	/* register packet class callback */
	if (wlc_pcb_fn_set(wlc->pcb, 0, WLF2_PCB1_NAR, wlc_nar_pkt_freed) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_pcb_fn_set() failed\n", wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(BCMDBG)
	wlc_dump_register(wlc->pub, "nar", wlc_nar_dump, nit);
#endif /* BCMDBG */

	wlc_txmod_fn_register(wlc->txmodi, TXMOD_NAR, nit, nar_txmod_fns);

	/* all fine, return handle */
	return nit;

fail:
	wlc_nar_detach(nit);
	return NULL;
}

void
BCMATTACHFN(wlc_nar_detach) (wlc_nar_info_t *nit)
{
	if (nit == NULL) {
		return;
	}

	/*
	 * Flush scb queues
	 */
	wlc_nar_flush_all_queues(nit);

	wlc_module_unregister(nit->wlc->pub, "nar", nit);
	MFREE(nit->wlc->pub->osh, nit, sizeof(*nit));
}

/*
 * The actual txmod packet processing functions.
 */

/*
 * Test whether the nar tx queue is full.
 */
static bool BCMFASTPATH
wlc_nar_queue_is_full(nar_scb_cubby_t * cubby)
{
	return (pktq_n_pkts_tot(&cubby->tx_queue) >= cubby->nit->queue_length);
}

/*
 * Place a packet on the corresponding nar precedence queue.
 * Updates the count of packets in queue and returns true or false to indicate
 * success or failure.
 */
static bool BCMFASTPATH
wlc_nar_enqueue_packet(nar_scb_cubby_t * cubby, void *pkt, uint prec)
{
	struct scb *scb = cubby->scb_bl;
	wlc_info_t *wlc = cubby->nit->wlc;

	BCM_REFERENCE(scb);

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %d\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea), prec));

	if (wlc_prec_enq(wlc, &cubby->tx_queue, pkt, prec)) {
		NAR_STATS_INC(cubby, PACKETS_QUEUED);
		return TRUE;
	}
	return FALSE;
}

/*
 * Dequeue a packet from one of the nar precedence queues, and adjust the count of packets.
 * Returns the dequeued packet, or NULL.
 */
static void *BCMFASTPATH
wlc_nar_dequeue_packet(nar_scb_cubby_t * cubby, uint prec, uint8* nr_of_sdus)
{
	void *p;
	struct scb *scb = cubby->scb_bl;
	wlc_info_t *wlc = cubby->nit->wlc;
#if defined(WLAMSDU_TX)
	amsdu_info_t *ami;
#endif /* WLAMSDU_TX */

	ASSERT(nr_of_sdus != NULL);

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %d\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea), prec));

	p = pktq_pdeq(&cubby->tx_queue, prec);

	if (p) {
		*nr_of_sdus = 1;
#ifdef WLAMSDU_TX
		if (WLC_NAR_AMSDU(wlc, scb, PKTPRIO(p))) {
			ami = wlc->ami;

			*nr_of_sdus += wlc_amsdu_agg_attempt(ami, scb, &cubby->tx_queue, p, prec,
				PKTPRIO(p));
		}
#endif /* WLAMSDU_TX */

		WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec: %d, AMSDU sf: %d\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
			ETHER_TO_MACF(scb->ea), prec, *nr_of_sdus));
		cubby->tx_stuck_time = 0;
		/* ISSUE: packets dequeued by pspretend is not taken into account */
		NAR_STATS_N_INC(cubby, PACKETS_DEQUEUED, *nr_of_sdus);
	} else {
		*nr_of_sdus = 0;
	}
	return p;
}

#ifdef WLATF_BARE
static uint BCMFASTPATH
wlc_nar_packet_airtime(nar_scb_cubby_t * cubby, uint prec, void *pkt)
{
	struct scb *scb = cubby->scb_bl;
	ratespec_t rspec;
	bool useRTS;
	bool gProt = FALSE;
	bool nProt = FALSE;
	bool shortPreamble = FALSE;
	uint ctl_rspec;
	uint ack_rspec;
	uint pkt_overhead_us = 0;
	uint payload_us = 0;
	wlc_bsscfg_t *bsscfg;
	wlc_info_t *wlc;
	wlcband_t *band;
	uint nbytes;
	uint flags = 0;

	ASSERT(scb->bsscfg);

	bsscfg = scb->bsscfg;
	wlc = bsscfg->wlc;

	rspec = wlc_scb_ratesel_get_primary(wlc, scb, NULL);
	nbytes = pkttotlen(cubby->osh, pkt) + wlc_airtime_dot11hdrsize(scb->wsec);
	gProt = (wlc->band->gmode && !RSPEC_ISCCK(rspec) && WLC_PROT_G_CFG_G(wlc->prot_g, bsscfg));
	nProt = ((WLC_PROT_N_CFG_N(wlc->prot_n, bsscfg) == WLC_N_PROTECTION_20IN40) &&
		RSPEC_IS40MHZ(rspec));

	if ((rspec == cubby->last_packet_rspec) &&
		(nbytes == cubby->last_packet_nbytes) &&
		(gProt == cubby->last_state_gProt) &&
		(nProt == cubby->last_state_nProt))
	{
		return cubby->last_packet_airtime_us;
	}

	band = wlc->bandstate[CHSPEC_BANDUNIT(bsscfg->current_bss->chanspec)];

	/* XXX
	 * Assume Mixed mode as the cost of calculating is too high
	 * Can change if we find an advantage to GF calculation
	 */
	flags |= (WLC_AIRTIME_MIXEDMODE);

	/* Frame protection and RTS/CTS */

	useRTS = !SCB_ISMULTI(scb) &&
		((nbytes > wlc->RTSThresh) || (WLPKTTAG(pkt)->flags & WLF_USERTS));

	if (useRTS) {
		/* RTS/CTS mode */
		flags |= (WLC_AIRTIME_RTSCTS);
	}

	/*
	 * Note if useRTS is also set, we will do RTS CTS and if g prot is set,
	 * release time is calculated for 11Mbit/s or lower (non ERP rates)
	 */
	if (gProt || nProt) {
		/* CTS to self */
		flags |= (WLC_AIRTIME_CTS2SELF);
	}

	/*
	 * Short slot for ERP STAs
	 * The AP bsscfg will keep track of all sta's shortslot/longslot cap,
	 * and keep current_bss->capability up to date.
	 */
	if (BAND_5G(band->bandtype) || bsscfg->current_bss->capability & DOT11_CAP_SHORTSLOT)
			flags |= (WLC_AIRTIME_SHORTSLOT);

	/*
	 * Short preamble:
	 * Specified for the ERP CCK rates
	 * If SCB is short preamble capable and and shortpreamble is enabled
	 * and rate is NOT 1M or Override is NOT set to LONG preamble then use SHORT preamble
	 * else use LONG preamble
	 * OFDM should bypass below preamble set, but old chips are OK since they ignore that bit
	 * XXX before 4331A0 PR79339 is fixed, restrict to CCK, which is actually correct
	 */
	shortPreamble = (WLC_PROT_CFG_SHORTPREAMBLE(wlc->prot, bsscfg) &&
		(scb->flags & SCB_SHORTPREAMBLE) && RSPEC_ISCCK(rspec)) &&
		(!((RSPEC2RATE(rspec) == WLC_RATE_1M) ||
		(bsscfg->PLCPHdr_override == WLC_PLCP_LONG)));

	/* Calculate the ACK rate */
	if ((cubby->last_packet_rspec == rspec) &&
		(cubby->last_state_gProt == gProt) &&
		(cubby->last_state_nProt == nProt)) {

		/*
		 * If settings except the frame size remain the same,
		 * can reuse the overhead time previously.
		 */
		ack_rspec = cubby->last_ack_rspec;
		ctl_rspec = cubby->last_ctrl_rspec;
		if (flags == cubby->last_state_flags) {
			pkt_overhead_us = cubby->last_packet_overhead_us;
		} else {
			cubby->last_state_flags = flags;
		}
	} else {
		cubby->last_packet_rspec = rspec;

		ack_rspec = NAR_BASIC_RATE(band, rspec);
		if (shortPreamble)
			ack_rspec |= WL_RSPEC_SHORT_PREAMBLE;
		cubby->last_ack_rspec = ack_rspec;

		/* Calculate overhead time */
		if (gProt) {
			/* Use 11Mbits or lower as the control rate for 2.4G */
			ctl_rspec = NAR_BASIC_RATE(band, LEGACY_RSPEC(WLC_RATE_11M));
		} else {
			ctl_rspec = ack_rspec;
		}
		cubby->last_ctrl_rspec = ctl_rspec;
		cubby->last_state_flags = flags;
		cubby->last_packet_rspec = rspec;
		cubby->last_state_gProt = gProt;
		cubby->last_state_nProt = nProt;
	}

	if (!pkt_overhead_us) {
		/* The prec to AC converson should be converted to a macro if it is used
		 * more than once
		 */
		pkt_overhead_us = wlc_airtime_pkt_overhead_us(flags,
			ctl_rspec, ack_rspec, bsscfg, wme_fifo2ac[prec >> 2]);
		cubby->last_packet_overhead_us = pkt_overhead_us;
	}

	/* Calculate payload time including PLCP */
	payload_us = wlc_airtime_packet_time_us(flags, rspec, nbytes);

	cubby->last_packet_nbytes = nbytes;
	cubby->last_packet_airtime_us = payload_us + pkt_overhead_us;

	return cubby->last_packet_airtime_us;
}

static INLINE void
wlc_nar_dec_packet_airtime(nar_scb_cubby_t *cubby, void * pkt)
{
	uint airtime_us = WLPKTTAG(pkt)->pktinfo.atf.pkt_time;
	bool enabled = cubby->nit->wlc->atf;
	uint prec = WLC_PRIO_TO_PREC(PKTPRIO(pkt));
	WLPKTTAG(pkt)->pktinfo.atf.pkt_time = 0;

	if ((enabled) && (airtime_us <= cubby->packet_airtime_us[prec])) {
		cubby->packet_airtime_us[prec] -= airtime_us;

	} else {
		cubby->packet_airtime_us[prec] = 0;
	}
}

static INLINE void
wlc_nar_add_packet_airtime(nar_scb_cubby_t *cubby, uint prec, void * pkt)
{
	bool enabled = cubby->nit->wlc->atf;
	uint airtime_us = 0;

	if (enabled) {
		airtime_us = wlc_nar_packet_airtime(cubby, prec, pkt);
		WLPKTTAG(pkt)->pktinfo.atf.pkt_time = (uint16)airtime_us;
		cubby->packet_airtime_us[prec] += airtime_us;
	}
}

static INLINE bool
wlc_nar_airtime_share_reached(nar_scb_cubby_t * cubby, uint prec)
{
	bool enabled = cubby->nit->wlc->atf;
	return ((enabled) && (cubby->packet_airtime_us[prec] >= cubby->txq_time_allowance_us));
}
#endif /* WLATF_BARE */

/*
 * Test whether our fair share of transmits has been reached.
 *
 * Fair share is only based on the following:
 * a)Released packet airtime.
 * b)Number of packets in transit.
 */
static bool BCMFASTPATH
wlc_nar_fair_share_reached(nar_scb_cubby_t * cubby, int prec)
{
#ifdef PSPRETEND
	wlc_info_t *wlc = cubby->nit->wlc;
	struct scb * scb = cubby->scb_bl;
	bool ps_pretend_limit_transit = SCB_PS_PRETEND(scb);
	/* scb->ps_pretend_start contains the TSF time for when ps pretend was
	 * last activated. In poor link conditions, there is a high chance that
	 * ps pretend will be re-triggered.
	 * Within a short time window following ps pretend, this code is trying to
	 * constrain the number of packets in transit and so avoid a large number of
	 * packets repetitively going between transmit and ps mode.
	 */
	if (!ps_pretend_limit_transit && SCB_PS_PRETEND_WAS_RECENT(scb)) {
		ps_pretend_limit_transit =
			wlc_pspretend_limit_transit(wlc->pps_info, scb,
				cubby->pkts_intransit_prec[prec], FALSE);
	}
#ifdef PROP_TXSTATUS
	if (PSPRETEND_ENAB(wlc->pub) && SCB_TXMOD_ACTIVE(scb, TXMOD_APPS) &&
		PROP_TXSTATUS_ENAB(wlc->pub) &&	HOST_PROPTXSTATUS_ACTIVATED(wlc)) {
		/* If TXMOD_APPS is active, and proptxstatus is enabled, force release.
		 * With proptxstatus enabled, they are meant to go to wlc_apps_ps_enq()
		 * to get their pktbufs freed in dongle, and instead stored in the host.
		 */
		ASSERT(SCB_PS(scb) || wlc_twt_scb_active(wlc->twti, scb));
		ps_pretend_limit_transit = FALSE;
	}
#endif /* PROP_TXSTATUS */
	/*
	 * Unless forced, ensure that we do not have too many packets in transit
	 * for this priority.
	 */
	if (ps_pretend_limit_transit) {
		return TRUE;
	}
#endif /* PSPRETEND */

	return (
#ifdef WLATF_BARE
		wlc_nar_airtime_share_reached(cubby, prec) ||
#endif

		(cubby->pkts_intransit_prec[prec] >= cubby->nit->transit_packet_limit));
}

static void BCMFASTPATH
wlc_nar_pkt_freed(wlc_info_t *wlc, void *pkt, uint txs)
{
	wlc_nar_info_t *nit = wlc->nar_handle;
	struct scb *scb;
	nar_scb_cubby_t *cubby;
	uint prec;
#ifdef WLTAF
	void* head_pkt = pkt;
	bool taf_in_use = nit ? wlc_taf_nar_in_use(nit->wlc->taf_handle, NULL) : FALSE;
#endif

	/* no packet or nar_handle is NULL */
	if (!pkt || !nit)
		return;

	scb = WLPKTTAGSCBGET(pkt);

	/*
	 * Return if we are not set up to handle the tx status for this scb.
	 */
	if (!scb) {		       /* no SCB */
#ifdef WLTAF
		wlc_taf_pktfree_check(wlc, pkt);
#endif
		return;
	}

	cubby = SCB_NAR_CUBBY(nit, scb);
	if (!cubby) {
#ifdef WLTAF
		wlc_taf_txpkt_status(wlc->taf_handle, scb, PKTPRIO(pkt), pkt,
			TAF_TXPKT_STATUS_PKTFREE_RESET);
#endif
		return;
	}

	prec = WLC_PRIO_TO_PREC(PKTPRIO(pkt));

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %d, txs %08x, %d in transit\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea),
		prec, txs, cubby->packets_in_transit));

#ifdef WLTAF
	if (!wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_PREC(prec), pkt,
		TAF_TXPKT_STATUS_PKTFREE))
#endif
	{
#ifdef WLATF_BARE
		wlc_nar_dec_packet_airtime(cubby, pkt);
#endif
	};

	if (cubby->packets_in_transit) {
		NAR_STATS_INC(cubby, PACKETS_TRANSMITTED);
		NAR_COUNTER_DEC(cubby, packets_in_transit);
		if (cubby->pkts_intransit_prec[prec] > 0) {
			cubby->pkts_intransit_prec[prec] --;
		} else {
			WL_ERROR(("wl%d.%d: %s: "MACF": prec %d, nar intransit counter is"
				" already zero, pkt seq 0x%x, len %d\n", wlc->pub->unit,
				WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__,
				ETHER_TO_MACF(scb->ea), prec,
				WL_SEQ_GET_NUM(WLPKTTAG(pkt)->seq),
				PKTLEN(nit->wlc->osh, pkt)));
			ASSERT(0);
		}
	}

#ifdef WLTAF
	if (taf_in_use) {
		if (SCB_PS(scb) || wlc_taf_scheduler_blocked(nit->wlc->taf_handle) ||
			(WLPKTTAG(head_pkt)->flags & WLF_CTLMGMT)) {
			/* avoid triggering TAF when not necessary */
			return;
		}

		/* Trigger a new TAF schedule cycle */
		wlc_taf_schedule(nit->wlc->taf_handle, TAF_PREC(prec), scb, FALSE);
	}
#endif
}

/*
 * Pass a packet to the next txmod below, incrementing the in transit count.
 */
static void BCMFASTPATH
wlc_nar_pass_packet_on(nar_scb_cubby_t * cubby, struct scb *scb, void *pkt, uint prec)
{
	wlc_pkttag_t *pkttag = WLPKTTAG(pkt);
	uint16 seq = 0; /* d11 sequence */
	uint cnt_prec = prec;

	WLF2_PCB1_REG(pkt, WLF2_PCB1_NAR);

#if defined(PROP_TXSTATUS)
	if (!GET_DRV_HAS_ASSIGNED_SEQ(pkttag->seq))
#endif /* PROP_TXSTATUS */
	{
		/* assign seqnum and save in pkttag */
		seq = SCB_SEQNUM(scb, PKTPRIO(pkt)) & (SEQNUM_MAX - 1);
		SCB_SEQNUM(scb, PKTPRIO(pkt))++;
		WL_SEQ_SET_NUM(pkttag->seq, seq);
		SET_WL_HAS_ASSIGNED_SEQ(pkttag->seq);
	}

	NAR_COUNTER_INC(cubby, packets_in_transit);
	if (!SCB_QOS(scb)) {
		/* XXX In case SCB_QOS is false for a station, all traffic will get BE priority,
		 * a.o. in wlc_scb_get_txfifo. This is causing an issue for the
		 * pkts_intransit_prec counters for non-BE priority traffic,
		 * such as 802.1X packets.
		 */
		cnt_prec = WLC_PRIO_TO_PREC(PRIO_8021D_BE);
		PKTSETPRIO(pkt, PRIO_8021D_BE);
	}
	cubby->pkts_intransit_prec[cnt_prec] ++;

	NAR_STATS_PEAK(cubby, PEAK_TRANSIT, cubby->packets_in_transit);

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %d transit count %d seq 0x%x next %p\n",
		cubby->nit->wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
		__FUNCTION__, ETHER_TO_MACF(scb->ea), cnt_prec,
		cubby->pkts_intransit_prec[cnt_prec], seq, PKTNEXT(cubby->nit->wlc->osh, pkt)));

	SCB_TX_NEXT(TXMOD_NAR, scb, pkt, prec);	/* pass to next module */

	NAR_STATS_INC(cubby, PACKETS_PASSED_ON);
}

#ifdef WLTAF
static void
wlc_nar_taf_link_upd(wlc_nar_info_t *nit, nar_scb_cubby_t * cubby, int prec, bool prio_is_agg)
{
	wlc_info_t *wlc = nit->wlc;
	struct scb * scb;

	if (!cubby) {
		return;
	}
	scb = cubby->scb_bl;

	if (prio_is_agg) {
		/* AMPDU */
		if (cubby->taf_prec_active & (1 << prec)) {
			/* tell TAF we are not using this NAR context anymore */
			wlc_taf_link_state(wlc->taf_handle, scb, TAF_PREC(prec), TAF_NAR,
				TAF_LINKSTATE_NOT_ACTIVE);
			cubby->taf_prec_active &= ~(1 << prec);

			if (cubby->taf_prec_active == 0) {
				wlc_taf_scb_state_update(wlc->taf_handle, scb,
					TAF_NAR, NULL, TAF_SCBSTATE_SOURCE_DISABLE);
			}
			WL_TAFF(wlc, MACF" prio_is_agg prec %u (new active map 0x%x)\n",
				ETHER_TO_MACF(scb->ea), prec, cubby->taf_prec_active);
		}
	} else {
		/* non-AMPDU, ie NAR */
		if (!cubby->taf_prec_active) {
			/* tell TAF we have started using NAR */
			wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NAR, NULL,
				TAF_SCBSTATE_SOURCE_ENABLE);
		}

		if (!(cubby->taf_prec_active & (1 << prec))) {
			cubby->taf_prec_active |= (1 << prec);
			cubby->taf_prec_enabled |= (1 << prec);
			/* tell TAF we are using a NAR prec */
			wlc_taf_link_state(wlc->taf_handle, scb, TAF_PREC(prec), TAF_NAR,
				TAF_LINKSTATE_ACTIVE);
			WL_TAFF(wlc, MACF" prio_is_nar prec %u (new active map 0x%x)\n",
				ETHER_TO_MACF(scb->ea), prec, cubby->taf_prec_active);
		}
	}
}

void * BCMFASTPATH wlc_nar_get_taf_scb_info(void *narh, struct scb* scb)
{
	wlc_nar_info_t *nit = (wlc_nar_info_t *)narh;

	return (nit && scb) ? (void*)SCB_NAR_CUBBY(nit, scb) : NULL;
}

void * BCMFASTPATH wlc_nar_get_taf_scb_prec_info(void *scb_h, int tid)
{
	return TAF_PARAM(WLC_PRIO_TO_PREC(tid));
}

uint16 BCMFASTPATH wlc_nar_get_taf_scb_prec_pktlen(void *narh, void *scbh, void *tidh, uint32 ts)
{
	int prec = (int)(size_t)tidh;
	nar_scb_cubby_t * cubby = (nar_scb_cubby_t *)scbh;

	if (!cubby) {
		return 0;
	}

	return pktqprec_n_pkts(&cubby->tx_queue, prec) +
		pktqprec_n_pkts(&cubby->tx_queue, prec + 1);
}

bool wlc_nar_taf_release(void* narh, void* scbh, void* tidh, bool force,
	taf_scheduler_public_t* taf)
{
	wlc_nar_info_t *nit = (wlc_nar_info_t *)narh;
	wlc_info_t* wlc = nit->wlc;
	int nreleased = 0;
	int prec = (int)(size_t)tidh;
	int hiprec = prec + 1;
	nar_scb_cubby_t * cubby = (nar_scb_cubby_t *)scbh;
	bool finished = FALSE;
	bool prio_is_aggregating = FALSE;
	int apsd_cnt = 0;
	struct scb * scb;
	uint8 nr_of_sdus;

	if (!cubby) {
		WL_ERROR(("%s: no cubby!\n", __FUNCTION__));
		return FALSE;
	}
	if (taf->how != TAF_RELEASE_LIKE_IAS) {
		ASSERT(0);
		taf->complete = TAF_REL_COMPLETE_ERR;
		return FALSE;
	}

	scb = cubby->scb_bl;

	prio_is_aggregating = (SCB_AMPDU(scb) && SCB_TXMOD_ACTIVE(scb, TXMOD_AMPDU) &&
		wlc_ampdu_ba_on_tid(scb, TAF_PREC(prec)));

	if (prio_is_aggregating) {
		wlc_nar_taf_link_upd(nit, cubby, prec, TRUE);
	}

#ifdef HNDPQP
	/* If PSQ is not empty and SCB is PS off,
	 * it means PQP PGI wait for resource to complete the transaction.
	 * Block the release until PQP PGI is finished.
	 */
	if (!prio_is_aggregating && !taf->ias.is_ps_mode && wlc_apps_psq_len(wlc, scb)) {
		taf->ias.was_emptied = TRUE;
		taf->complete = TAF_REL_COMPLETE_PS;
		return FALSE;
	}
#endif /* HNDPQP */

	if (taf->ias.is_ps_mode && (taf->how == TAF_RELEASE_LIKE_IAS)) {
		/* If packet AC is delivery-enabled, get the apsd_cnt and
		 * release the packets to the psq.
		 */
		if (AC_BITMAP_TST(scb->apsd.ac_delv, prio2ac[TAF_PREC(prec)])) {
			apsd_cnt = wlc_apps_scb_apsd_cnt(wlc, scb);
		}
	}

	while (!finished) {
		void *p;

		p = wlc_nar_dequeue_packet(cubby, hiprec, &nr_of_sdus);
		if (!p) {
			p = wlc_nar_dequeue_packet(cubby, prec, &nr_of_sdus);
		}
		if (!p) {
			finished = TRUE;
			if (taf->how == TAF_RELEASE_LIKE_IAS) {
				taf->ias.was_emptied = TRUE;
			}
			if (taf->how == TAF_RELEASE_LIKE_DEFAULT) {
				taf->def.was_emptied = TRUE;
			}
			taf->complete = TAF_REL_COMPLETE_EMPTIED;
			break;
		}
		if (TAF_IS_TAGGED(WLPKTTAG(p))) {
			/* re-sending packets previously released by TAF is design error */
			ASSERT(0);
			wlc_taf_txpkt_status(wlc->taf_handle, scb, TAF_PREC(prec), p,
				TAF_TXPKT_STATUS_REQUEUED);
		}
		nreleased += nr_of_sdus;

		if (taf->how == TAF_RELEASE_LIKE_IAS) {
			uint32 taf_pkt_tag;

			if (!taf->ias.is_ps_mode)  {
				uint32 pktbytes = pkttotlen(wlc->osh, p);
				uint32 taf_pkt_time_units = TAF_PKTBYTES_TO_UNITS((uint16)pktbytes,
					taf->ias.pkt_rate, taf->ias.byte_rate);

				if (taf_pkt_time_units == 0) {
					taf_pkt_time_units = 1;
				}

				if (taf_pkt_time_units < taf->ias.min_mpdu_dur_units) {
					taf_pkt_time_units = taf->ias.min_mpdu_dur_units;
				}

				taf->ias.actual.released_bytes += (uint16)pktbytes;

				taf_pkt_tag = TAF_UNITS_TO_PKTTAG(taf_pkt_time_units);
				taf->ias.actual.released_units += TAF_PKTTAG_TO_UNITS(taf_pkt_tag);

				if (!prio_is_aggregating &&
					(taf->ias.actual.released_units +
					taf->ias.total.released_units) >=
					taf->ias.time_limit_units) {

					finished = TRUE;

					taf->complete = TAF_REL_COMPLETE_TIME_LIMIT;
				}
				if (!prio_is_aggregating && (taf->ias.released_units_limit > 0) &&
					(taf->ias.actual.released_units >=
					taf->ias.released_units_limit)) {

					finished = TRUE;

					taf->complete = TAF_REL_COMPLETE_REL_LIMIT;
				}
			} else {
				taf_pkt_tag = TAF_PKTTAG_PS;

				/* If APSD Unscheduled Service Period in progress,
				 * max packets in psq should be apsd_cnt to
				 * satisfy the apsd trigger process.
				 */
				if ((apsd_cnt == 0) ||
					(taf->ias.actual.release >= (apsd_cnt - 1))) {
					finished = TRUE;
					taf->ias.was_emptied = TRUE;
				}
			}
			taf->ias.actual.release++;
			TAF_SET_TAG_IDX(WLPKTTAG(p), taf->ias.index);
			TAF_SET_TAG_UNITS(WLPKTTAG(p), taf_pkt_tag);
			TAF_SET_TAG(WLPKTTAG(p));
		}
		else if (taf->how == TAF_RELEASE_LIKE_DEFAULT) {
			taf->def.actual.release++;
			WLPKTTAG(p)->pktinfo.taf.def.tag = SCB_PS(scb) ?
				TAF_PKTTAG_PS : TAF_PKTTAG_DEFAULT;
			TAF_SET_TAG(WLPKTTAG(p));
		}
		wlc_nar_pass_packet_on(cubby, cubby->scb_bl, p, prec);
	}
	WL_NAR_MSG(("wl%d.%d: %s: "MACF": prec %u released %u\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea),
		prec, nreleased));
	return nreleased > 0;
}
#endif /* WLTAF */

/* Determine the amount of available space on the destination queue */
static INLINE uint16
wlc_nar_txq_avail(wlc_info_t* wlc, struct scb *scb, uint prec)
{
	if (SCB_TXMOD_ACTIVE(scb, TXMOD_APPS)) {
		return  wlc_apps_release_count(wlc, scb, prec);
	}

	return wlc_txq_pktq_pavail(scb, prec);
}

/*
 * Release one (or more) packets from the queue and pass it on.
 *
 * Returns the number of packets dequeued.
 *
 */
static int BCMFASTPATH
wlc_nar_release_from_queue(nar_scb_cubby_t * cubby, int prec)
{
	int ntorelease;
	int nreleased = 0;
	struct scb *scb = cubby->scb_bl;
	wlc_info_t *wlc = cubby->nit->wlc;
	uint8 nr_of_sdus;

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(scb);
#if defined(WLTAF) && !defined(WLSQS)
	/* XXX currently SQS cannot handle DWDS stations which must bypass TAF, hence this
	 * assert might trigger for SQS and DWDS case, so conditonally compile for non WLSQS only
	 */
	ASSERT(!wlc_taf_nar_in_use(cubby->scb_bl->bsscfg->wlc->taf_handle, NULL));
#endif

#ifdef HNDPQP
	/* If PSQ is not empty and SCB is PS off,
	 * it means PQP PGI wait for resource to complete the transaction.
	 * Block the release until PQP PGI is finished.
	 */
	if (!SCB_PS(scb) && wlc_apps_psq_len(wlc, scb)) {
		return nreleased;
	}
#endif /* HNDPQP */

	ntorelease = MIN(cubby->nit->release_at_once, wlc_nar_txq_avail(wlc, scb, prec));

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": Prec %d can release %d, %d in queue, "
		"fair share %sreached.\n", wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
		__FUNCTION__, ETHER_TO_MACF(scb->ea), prec, ntorelease,
		pktq_n_pkts_tot(&cubby->tx_queue),
		wlc_nar_fair_share_reached(cubby, prec) ? "" : "NOT "));
	/*
	 * See if this SCB has packets queued, if so, reinject the next few in line.
	 */
	while (ntorelease-- &&	       /* we can release some more */
		pktqprec_n_pkts(&cubby->tx_queue, prec)  &&	/* we have some to release */
		!wlc_nar_fair_share_reached(cubby, prec)) {	/* fair share check */

		void           *p;

		p = wlc_nar_dequeue_packet(cubby, prec, &nr_of_sdus);

		if (p) {
			nreleased += nr_of_sdus;
#ifdef WLATF_BARE
			wlc_nar_add_packet_airtime(cubby, prec, p);
#endif
			wlc_nar_pass_packet_on(cubby, cubby->scb_bl, p, prec);
		}
	}

	return nreleased;
}

#if defined(PKTQ_LOG)

/*
 * Helper function for dotxstatus to test whether one of several packets was acked.
 */
static bool BCMFASTPATH
wlc_nar_txs_was_acked(tx_status_macinfo_t * status, uint16 idx)
{
	return (idx < 32) ? (status->mactxs->ack_map1 & (1 << idx)) :
			(status->mactxs->ack_map2 & (1 << (idx - 32)));
}

/*
 * PKTQ_LOG helper function to determine the packet payload length.
 */
static INLINE int BCMFASTPATH
wlc_nar_payload_length(wlc_info_t * wlc, struct scb *scb, void *pkt)
{
	int pktlen;
	int d11len;

	d11len = D11_TXH_LEN_EX(wlc)
		+ DOT11_MAC_HDR_LEN + DOT11_LLC_SNAP_HDR_LEN + (SCB_QOS(scb) ? DOT11_QOS_LEN : 0);

#if defined(WLTOEHW)
	if (WLCISACPHY(wlc->band) && wlc->toe_capable && !wlc->toe_bypass) {
		/*
		 * take the toe header into account
		 */
		d11len += wlc_tso_hdr_length((d11ac_tso_t *) PKTDATA(wlc->osh, pkt));
	}
#endif /* WLTOEHW */

	pktlen = pkttotlen(wlc->osh, pkt);

	return ((pktlen < d11len) ? 0 : pktlen - d11len);
}
#endif /* PKTQ_LOG */

/*
 * Callback for once a packet has been transmitted. Here we simply decrement the count of in
 * transit packets, and trigger transmission of more packets of the same precedence.
 *
 * XXX Problem: If the lower layers decide not to transmit the packet (ie, drop it), we do not
 * XXX get any indication of what happened. Hence the tx_stuck_time cleanup in the watchdog. Sigh.
 *
 */
void BCMFASTPATH
wlc_nar_dotxstatus(wlc_nar_info_t *nit, struct scb *scb, void *pkt, tx_status_t * txs,
	bool pps_retry, uint32 tx_rate_prim, ratespec_t pri_rspec)
{
	nar_scb_cubby_t *cubby;
	int             prec;
	uint32          pcount;		/* number of packets for which we get tx status */
	uint32          pcounted;
	wlc_info_t*     wlc;

#if defined(PKTQ_LOG)
	pktq_counters_t *pq_counters = NULL;

#ifdef PSPRETEND
	uint supr_status;
#endif /* PSPRETEND */
#endif /* PKTQ_LOG */
#ifdef WLTAF
	bool taf_in_use;
#endif
	BCM_REFERENCE(tx_rate_prim);
	BCM_REFERENCE(pri_rspec);
	BCM_REFERENCE(pcount);
	BCM_REFERENCE(pcounted);

	if (nit == NULL || txs == NULL) {
		ASSERT(FALSE);
		return;
	}

	wlc = nit->wlc;
#if defined(PSPRETEND) && defined(PKTQ_LOG)
	supr_status = txs->status.suppr_ind;
#endif
#ifdef WLTAF
	taf_in_use = wlc_taf_nar_in_use(wlc->taf_handle, NULL);
#endif

	/*
	 * Return if we are not set up to handle the tx status for this scb.
	 */
	if (!scb ||		       /* no SCB */
	    !pkt) {		       /* no packet */
		return;
	}

	cubby = SCB_NAR_CUBBY(nit, scb);
	if (!cubby) {
		return;
	}

	NAR_STATS_INC(cubby, TX_STATUS);

	if (WLPKTTAG(pkt)->flags & WLF_AMPDU_MPDU) {
		/*
		 * Ignore aggregating stations
		 */
		NAR_STATS_INC(cubby, AMPDU_TXS_IGNORED);
		return;
	}

	/*
	 * Figure out for which precedence this packet was
	 */

	prec = WLC_PRIO_TO_PREC(PKTPRIO(pkt));

	/*
	 * AQM can interrupt once for a number of packets sent - handle that.
	 */

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		pcount = 1;
	} else {
		pcount = ((txs->status.raw_bits & TX_STATUS40_NCONS) >> TX_STATUS40_NCONS_SHIFT);

#if defined(NAR_STATS)
		cubby->stats.counter[NAR_COUNTER_NCONS] += pcount;
#endif /* NAR_STATS */
	}

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": Status %08x [%s %s] for "
		"%d %s packet(s), prec %d, %d in transit\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea),
		txs->status.raw_bits, txs->status.was_acked ? "acked" : "",
		txs->status.is_intermediate ? "intermediate" : "", pcount,
		(WLPKTTAG(pkt)->flags & WLF_AMPDU_MPDU) ? "AMPDU" : "MPDU", prec,
		cubby->packets_in_transit));

#if defined(PKTQ_LOG)
	for (pcounted = 0; pcounted < pcount; ++pcounted) {
		bool acked;
		int pktlen, tx_frame_count;

		/*
		 * Update pktq_stats counters
		 */

		pktlen = wlc_nar_payload_length(wlc, scb, pkt);

		WL_NAR_MSG(("wl%d.%d: %s: "MACF": pkt %d/%d, len %d\n", wlc->pub->unit,
			WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, ETHER_TO_MACF(scb->ea),
			pcounted, pcount, pktlen));
		if (pcount == 1) {
			acked = txs->status.was_acked;
		} else {
			acked = wlc_nar_txs_was_acked(&(txs->status), (uint16)pcounted);
		}

		if (cubby->tx_queue.pktqlog) {
			pq_counters = cubby->tx_queue.pktqlog->_prec_cnt[prec];
			if (pq_counters == NULL &&
					(cubby->tx_queue.pktqlog->_prec_log & PKTQ_LOG_AUTO)) {
				pq_counters = wlc_txq_prec_log_enable(wlc, &cubby->tx_queue,
					(uint32)prec, TRUE);

				if (pq_counters) {
					wlc_read_tsf(wlc, &pq_counters->_logtimelo,
						&pq_counters->_logtimehi);
				}
			}
		}

		tx_frame_count = txs->status.frag_tx_cnt;

		if (D11REV_GE(wlc->pub->corerev, 128)) {
			WLCNTCONDADD(pq_counters, pq_counters->airtime,
				(uint64)TX_STATUS128_TXDUR(TX_STATUS_MACTXS_S2(txs)));
		} else if (D11REV_GE(wlc->pub->corerev, 40)) {
			WLCNTCONDADD(pq_counters, pq_counters->airtime,
				(uint64)TX_STATUS40_TX_MEDIUM_DELAY(txs));
		}

		WLCNTCONDADD(pq_counters, pq_counters->txrate_main,
			tx_rate_prim * (tx_frame_count ?: 1));
#ifdef PSPRETEND
		if (pps_retry) {
			if (!supr_status) {
				WLCNTCONDINCR(pq_counters, pq_counters->ps_retry);
			}
		} else if (acked) {
#else
		if (acked) {
#endif /* PSPRETEND */
			if (pq_counters) {
				WLCNTADD(pq_counters->txrate_succ, tx_rate_prim);
				WLCNTINCR(pq_counters->acked);
				WLCNTADD(pq_counters->throughput, (uint64)pktlen);
				if (tx_frame_count) {
					WLCNTADD(pq_counters->retry, (tx_frame_count - 1));
				}
				if (pri_rspec && !RSPEC_ISLEGACY(pri_rspec)) {
					uint32 mcs = 0, nss = 0,  bw = 0;

					mcs = wlc_ratespec_mcs(pri_rspec);
					nss = wlc_ratespec_nss(pri_rspec);
					bw = wlc_ratespec_bw(pri_rspec);

					WLCNTINCR(pq_counters->mcs[mcs]);
					WLCNTINCR(pq_counters->nss[(nss-1)]);
					WLCNTADD(pq_counters->txbw_succ, bw);
				}
			}
			NAR_STATS_INC(cubby, ACKED);

		} else {
			WLCNTCONDINCR(pq_counters, pq_counters->retry_drop);
			WLCNTCONDADD(pq_counters, pq_counters->retry, tx_frame_count);

			NAR_STATS_INC(cubby, NOT_ACKED);

		} /* not acked */
	} /* for all packets */
#endif /* PKTQ_LOG */

#ifdef WLTAF
	if (taf_in_use) {
		return;
	}
#endif
	for (prec = (WLC_PREC_COUNT - 1); prec >= 0; prec --) {
		if (wlc_nar_release_from_queue(cubby, prec)) {
			NAR_STATS_INC(cubby, KICKSTARTS_IN_INT);
		} /* released some packets */
	} /* for each precedence */
}

/**
 * Inform TAF that it should no longer retrieve new data packets for this scb.
 * This function is called when scb is marked for deletion.
 */
void
wlc_nar_scb_close_link(wlc_info_t *wlc, struct scb *scb)
{
#ifdef WLTAF
	wlc_nar_info_t *nit;
	nar_scb_cubby_t *cubby;
	uint8 prec;

	if (!wlc_taf_enabled(wlc->taf_handle)) {
		return;
	}
	WL_TAFF(wlc, MACF"\n", ETHER_TO_MACF(scb->ea));

	nit = wlc->nar_handle;
	cubby = SCB_NAR_CUBBY(nit, scb);

	if (!cubby) {
		return;
	}

	for (prec = 0; cubby->taf_prec_active && prec < WLC_PREC_COUNT; prec++) {
		if (cubby->taf_prec_active & (1 << prec)) {
			WL_TAFF(wlc, MACF" prec %u: in transit %u\n",
				ETHER_TO_MACF(scb->ea), prec, cubby->pkts_intransit_prec[prec]);

			wlc_taf_link_state(wlc->taf_handle, scb, TAF_PREC(prec), TAF_NAR,
				TAF_LINKSTATE_NOT_ACTIVE);
			cubby->taf_prec_active &= ~(1 << prec);
		}
	}
	wlc_taf_scb_state_update(wlc->taf_handle, scb, TAF_NAR, NULL, TAF_SCBSTATE_SOURCE_DISABLE);
#endif /* WLTAF */
}

/*
 * Packet TXMOD handler.
 *
 * Here we simply pass on the packets unless we already have a certain amount in transit, in
 * which case we queue the packet for later transmission.
 *
 */

static void BCMFASTPATH
wlc_nar_transmit_packet(void *handle, struct scb *scb, void *pkt, uint prec)
{
	wlc_nar_info_t *nit = handle;
	wlc_info_t     *wlc = nit->wlc;
	nar_scb_cubby_t *cubby;
	bool prio_is_aggregating = FALSE;
#if defined(PKTC) || defined(PKTC_TX_DONGLE)
	void *pkt1 = NULL;
#endif /* #if defined(PKTC) */
#ifdef WLTAF
	bool taf_enabled = FALSE;
	bool taf_in_use = wlc_taf_nar_in_use(wlc->taf_handle, &taf_enabled);
#else
	const bool taf_in_use = FALSE;
#endif

	ASSERT(scb != NULL);		/* Why would we get called with no SCB ? */
	ASSERT(prec < WLC_PREC_COUNT);

	if (!scb) {
		PKTFREE(wlc->osh, pkt, TRUE);
		TX_PKTDROP_COUNT(nit->wlc, scb, TX_PKTDROP_RSN_NO_SCB);
		return;
	}

	cubby = SCB_NAR_CUBBY(nit, scb);

	/* BCMC does not get cubby, so all BCMC traffic will pass on */
	if (!cubby) {
		SCB_TX_NEXT(TXMOD_NAR, scb, pkt, prec);   /* Pass the packet on */
		return;
	}

	NAR_STATS_INC(cubby, PACKETS_RECEIVED);

	prio_is_aggregating =
		(SCB_AMPDU(scb) && SCB_TXMOD_ACTIVE(scb, TXMOD_AMPDU) &&
		wlc_ampdu_ba_on_tid(scb, PKTPRIO(pkt)) &&
#ifdef WLTAF
	        (!(WLPKTTAG(pkt)->flags3 & WLF3_BYPASS_AMPDU) || !taf_in_use));
#else
		TRUE);
#endif

	/* If ampdu is enabled, pass on the packets */
	if (prio_is_aggregating) {
#ifdef WLTAF
		if (taf_enabled) {
			wlc_nar_taf_link_upd(nit, cubby, prec, TRUE);
		}
#endif /* WLTAF */
		/* If NAR is holding packets, release them now as BA has been established */
		if (pktqprec_n_pkts(&cubby->tx_queue, prec) > 0) {
			wlc_nar_release_all_from_queue(nit, scb, prec);
		}
		/* We have not queued this packet, pass it to the next layer (ampdu) */
		NAR_STATS_INC(cubby, PACKETS_AGGREGATING);
		SCB_TX_NEXT(TXMOD_NAR, scb, pkt, prec);   /* Pass the packet on */
		return;
	}

#ifdef WLTAF
	if (taf_enabled) {
		wlc_nar_taf_link_upd(nit, cubby, prec, FALSE);
	}
#endif /* WLTAF */

#if defined(PKTC) || defined(PKTC_TX_DONGLE)
	FOREACH_CHAINED_PKT(pkt, pkt1) {
		PKTCLRCHAINED(wlc->osh, pkt);
		if (pkt1 != NULL) {
			wlc_pktc_sdu_prep(wlc, scb, pkt, pkt1);
		}
#endif /* PKTC || PKTC_TX_DONGLE */

		/*
		 * If the queue is already full, try to dequeue some packets
		 * before queueing this one.
		 */
		if (!taf_in_use && wlc_nar_queue_is_full(cubby)) {
			if (wlc_nar_release_from_queue(cubby, prec)) {
				NAR_STATS_INC(cubby, KICKSTARTS_IN_TX_QF);
			}
		}

		/*
		 * Enqueue packet, then try to dequeue up to fair share.
		 */
		if (!prio_is_aggregating && !wlc_nar_enqueue_packet(cubby, pkt, prec)) {
			/*
			 * Failed to queue packet. Drop it on the floor.
			 */
			PKTFREE(wlc->osh, pkt, TRUE);
			TX_PKTDROP_COUNT(wlc, scb, TX_PKTDROP_RSN_NAR_Q_FULL);
			NAR_STATS_INC(cubby, PACKETS_DROPPED);
			WLCNTINCR(wlc->pub->_cnt->txnobuf);
		}
#if defined(PKTC) || defined(PKTC_TX_DONGLE)
	}
#endif /* PKTC || PKTC_TX_DONGLE */

#ifdef WLTAF
	if (taf_in_use) {
		if (wlc_taf_schedule(wlc->taf_handle, TAF_PREC(prec), scb, FALSE)) {
			return;
		}
		/* fall through here */
	}
#endif
	if (wlc_nar_release_from_queue(cubby, prec)) {
		NAR_STATS_INC(cubby, KICKSTARTS_IN_TX);
	}
	return;
}

/*
 * txmod callback to return the count of queued packets.
 */
static uint BCMFASTPATH
wlc_nar_count_queued_packets(void *handle)
{
	wlc_nar_info_t *nit = handle;
	int total_pkts = 0;
	struct scb_iter scbiter;
	struct scb     *scb;
	nar_scb_cubby_t *cubby;
	FOREACHSCB(nit->wlc->scbstate, &scbiter, scb) {
		cubby = SCB_NAR_CUBBY(nit, scb);
		if (!cubby) {
			continue;
		}
		total_pkts +=  pktq_n_pkts_tot(&cubby->tx_queue);
	}
	return total_pkts;
}

/*
 * Release all packets from the queue and pass it on.
 */
static void
wlc_nar_release_all_from_queue(wlc_nar_info_t *nit, struct scb *scb, int prec)
{
	void *pkt;
	nar_scb_cubby_t *cubby;

	cubby = SCB_NAR_CUBBY(nit, scb);

	if (!cubby) {
		return;
	}

	WL_NAR_MSG(("wl%d.%d: %s: "MACF": Prec %d can release %d (/%d) in queue.\n",
		nit->wlc->pub->unit, WLC_BSSCFG_IDX(SCB_BSSCFG(scb)),
		__FUNCTION__, ETHER_TO_MACF(scb->ea),
		prec, pktqprec_n_pkts(&cubby->tx_queue, prec),
		pktq_n_pkts_tot(&cubby->tx_queue)));
	/*
	 * See if this SCB has packets queued, if so, reinject the next few in line.
	 */
	while ((pkt = pktq_pdeq(&cubby->tx_queue, prec))) {
		NAR_STATS_INC(cubby, PACKETS_DEQUEUED);
		NAR_STATS_INC(cubby, PACKETS_AGGREGATING);
		SCB_TX_NEXT(TXMOD_NAR, scb, pkt, prec); /* pass to ampdu module */
	}
}

/*
* NAR is done with this flow, AMPDU will now handle this.
*/
void
wlc_nar_addba(wlc_nar_info_t *nit, struct scb *scb, int prec)
{
	ASSERT(scb);

#ifdef WLTAF
	if (wlc_taf_enabled(nit->wlc->taf_handle)) {
		/* hi-prec and normal prec */
		wlc_nar_taf_link_upd(nit, SCB_NAR_CUBBY(nit, scb), prec + 1, TRUE);
		wlc_nar_taf_link_upd(nit, SCB_NAR_CUBBY(nit, scb), prec, TRUE);
	}
#endif /* WLTAF */

	/* hi-prec and normal prec */
	wlc_nar_release_all_from_queue(nit, scb, prec + 1);
	wlc_nar_release_all_from_queue(nit, scb, prec);
}

/* When a flowring is deleted or scb is removed, the packets related to that flowring or scb have
 * to be flushed.
 */
static void
wlc_nar_flush_scb_tid(void *handle, struct scb *scb, uint8 tid)
{
	wlc_nar_info_t *nit = (wlc_nar_info_t *)handle;
	nar_scb_cubby_t *scb_narinfo;
	wlc_info_t *wlc = nit->wlc;
	struct pktq *pq;			/**< multi-priority packet queue */
	int prec;
	uint32 n_pkts_flushed;

	ASSERT(scb);

	scb_narinfo = SCB_NAR_CUBBY(nit, scb);

	if (scb_narinfo == NULL) {
		return;
	}

	pq = &scb_narinfo->tx_queue;

	prec = WLC_PRIO_TO_PREC(tid);

	if (pktqprec_empty(pq, prec)) {
		return;
	}

	n_pkts_flushed = wlc_txq_pktq_pflush(wlc, pq, prec);
#ifdef WLTAF
	if (scb_narinfo->taf_prec_active & (1 << prec)) {
		wlc_taf_link_state(wlc->taf_handle, scb, TAF_PREC(prec), TAF_NAR,
			TAF_LINKSTATE_SOFT_RESET);
	}
#endif /* WLTAF */

	BCM_REFERENCE(n_pkts_flushed);
	WL_NAR_MSG(("wl%d.%d: %s flushing %d packets for "MACF" AID %d tid %d\n", wlc->pub->unit,
		WLC_BSSCFG_IDX(SCB_BSSCFG(scb)), __FUNCTION__, n_pkts_flushed,
		ETHER_TO_MACF(scb->ea), SCB_AID(scb), tid));

	NAR_STATS_N_INC(scb_narinfo, PACKETS_DEQUEUED, n_pkts_flushed);

	if (pktq_empty(pq)) {
		/* Reset counters */
		wlc_nar_reset_release_state(scb_narinfo);
	}
}

#ifdef PROP_TXSTATUS
/** returns (multi prio) tx queue to use for a caller supplied scb (= one remote party) */
struct pktq *
wlc_nar_txq(wlc_nar_info_t * nit, struct scb *scb)
{
	nar_scb_cubby_t *cubby;

	cubby = SCB_NAR_CUBBY(nit, scb);
	return ((cubby) ? &cubby->tx_queue : NULL);

}
#endif /* PROP_TXSTATUS */

/* Return total intransit NAR packets across SCBs */
uint32
wlc_nar_tx_in_tansit(wlc_nar_info_t *nit)
{
	if (nit) {
		return nit->packets_in_transit;
	}

	return 0;
}
/* Get queued up packets for a given scb */
uint16 BCMFASTPATH
wlc_scb_nar_n_pkts(wlc_nar_info_t * nit, struct scb *scb, uint8 prio)
{
	nar_scb_cubby_t *cubby;

	cubby = SCB_NAR_CUBBY(nit, scb);

	if (!cubby) {
		return 0;
	}

	return pktqprec_n_pkts(&cubby->tx_queue, WLC_PRIO_TO_PREC(prio));
}

uint BCMFASTPATH
wlc_nar_scb_txpktcnt(wlc_nar_info_t * nit, struct scb *scb)
{
	uint pktcnt = 0;
	nar_scb_cubby_t *cubby;

	cubby = SCB_NAR_CUBBY(nit, scb);

	if (!cubby) {
		return 0;
	}

	pktcnt = pktq_n_pkts_tot(&cubby->tx_queue);

	return pktcnt;
}

uint BCMFASTPATH
wlc_nar_scb_pktq_mlen(wlc_nar_info_t * nit, struct scb *scb,
	uint32 precbitmap)
{
	uint pktcnt = 0;
	nar_scb_cubby_t *cubby;

	cubby = SCB_NAR_CUBBY(nit, scb);

	if (!cubby) {
		return 0;
	}

	pktcnt = pktq_mlen(&cubby->tx_queue, precbitmap);

	return pktcnt;
}
