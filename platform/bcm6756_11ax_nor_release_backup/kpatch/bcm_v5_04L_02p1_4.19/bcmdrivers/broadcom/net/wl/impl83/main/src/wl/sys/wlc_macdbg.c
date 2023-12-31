/*
 * MAC debug and print functions
 * Broadcom 802.11abg Networking Device Driver
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
 * $Id: wlc_macdbg.c 801135 2021-07-14 20:25:58Z $
 */

/*
 * README, how to dump dma
 * Usage:
 * wl dump dma [-t <types> -f <DMA fifos> -r]
 *	t: specific types that you afre interesting
 *	<types>: <all tx rx aqm hwa>
 *	f: specific fifos that you are interesting
 *	<DMA fifos>: <all 0 1 2 ...>
 *	r: dump rx fifo descriptor table contents.
 * NOTE: The <DMA fifos> is logic fifo index.
 *	More FIFO mapping info is in WLC_HW_MAP_TXFIFO.
 * Example: Dump DMA FIFOs 1 and 3 tx and aqm FIFOs
 *	wl dump dma [-t tx aqm -f 1 3]
*/

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <sbchipc.h>
#include <bcmendian.h>
#include <wlc_types.h>
#include <wlioctl.h>
#include <802.11.h>
#include <d11.h>
#include <hnddma.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_ampdu.h>
#include <wlc_macdbg.h>
#include <wlc_bsscfg.h>
#include <hndpmu.h>
#include <bcmdevs.h>
#include <wlc_hw.h>
#include <wlc_event_utils.h>
#include <wlc_event.h>
#include <phy_dbg_api.h>
#include "d11reglist.h"
#include <wlc_tx.h>
#include <wlc_dump.h>
#include <wlc_iocv.h>
#include <pcie_core.h>
#include <dbg_dump.h>
#include <wlc_mbss.h>
#include <wlc_tx.h>
#include <wlc_scb.h>

#if defined(BCMDBG) || defined(WLVASIP) || defined(WL_PSMR1)
#include <wlc_hw_priv.h>
#endif

#if defined(BCMDBG) && !defined(DONGLEBUILD)
#include <wl_export.h>
#endif /* BCMDBG && DONGLEBUILD */

#ifdef WLVASIP
#include <phy_vasip_api.h>
#include <d11vasip_code.h>
#define SVMP_ACCESS_VIA_PHYTBL
#endif	/* WLVASIP */
#include <wlc_twt.h>
#include <wlc_fifo.h>

/* per user dump */
#define MPDUS_DUMP_NUM		0x8
#define MAX_NUSR(_d11rev) \
		(((_d11rev) == 129 || (_d11rev) == 132) ? 16 : \
		 (((_d11rev) == 130 || (_d11rev) == 131) ? 4 : 0))

#define D11REGS_T_MAX_SIZE	0xFFF // Maximum size of D11 register
#define BITMAP_WIDTH		32 // Bitmap width = 32 bits

#define SC_NUM_OPTNS_GE50	5
#define SC_OPTN_LT50NA		0

/* Tag id and len of allocated buffer
 *	also needs to be stored
 */
#define ID_LEN_SIZE 8

typedef struct _d11dbg_list {
	d11regs_list_t reglist;
} d11dbg_list_t;

/* ucode dump constant */
#define BCM_BUF_SIZE_FOR_PC_DUMP	256
#define BCM_BUF_SIZE_FOR_PMU_DUMP	100
#define BCM_BUF_SIZE_FOR_UCODE_DUMP	12000

#define MACDBG_DUMP_LEN (4 * 1024)

/* frameid tracing */
typedef struct pkt_hist pkt_hist_t;
typedef struct txs_hist txs_hist_t;
typedef struct sync_hist sync_hist_t;

/* this is for dump_ucode_fatal */
typedef struct _d11print_list {
	const char *name;
	const uint16 addr; /**< byte offset relative to d11 enum space start */
} d11print_list_t;

/* this is for dump_shmem */
typedef struct _shmem_list {
	uint16	start;
	uint16	cnt;
} shmem_list_t;

#ifdef BCMDBG
#define MAXDTRACEBUFLEN	1960	/* bytes */

/* Below are each type flag value and data format */
typedef enum {
	DTRACE_ID_TXS	= 0,	/* txstatus */
	DTRACE_ID_TXD	= 1,	/* tx descriptor */
	DTRACE_ID_TXR	= 2,	/* tx rate */
	DTRACE_ID_STR	= 3,	/* string */
	DTRACE_ID_UTXS	= 4,	/* ulmu txstatus */
	DTRACE_ID_UTXD	= 5,	/* ulmu tx descriptor */
	DTRACE_ID_LAST	= 16,	/* max valid type number is 15 */
} dtrace_record_type_t;

typedef struct _dtrace_txs_t {
	uint32 time_in_ms;
	uint32 rspec;
	/* put 2 uint16 variables together for 4B alignment */
	uint16 frameid;
	uint16 raw_bits;
	uint32 s8;	/* can be reused as dbg info in ucode */
	uint32 s3;
	uint32 s4;
	uint32 s5;
	uint32 ack_map1;
	uint32 ack_map2;
} dtrace_txs_t;
typedef struct dtrace_txs_t dtrace_utxs_t;

typedef struct _dtrace_txd_t {
	uint32 time_in_ms;
	uint32 rspec;
	uint16 FrameID;
	uint16 MacControl_0;
	uint16 MacControl_1;
	uint16 MacControl_2;
	uint16 Chanspec;
	uint16 SeqCtl;
	uint16 FrmLen;
	uint8  IVOffset;
	uint8  Lifetime;
	uint16 LinkMemIdxTID;
	uint16 RateMemIdxRateIdx;
} dtrace_txd_t;

typedef struct _dtrace_rate_t {
	uint16	RateCtl;
	uint8	plcp[D11_PHY_HDR_LEN];              /* 6 bytes */
	 /* txphyctl word offsets {0, 1, 2, 6, 7} */
	uint16	TxPhyCtl[D11_REV128_RATEENTRY_TXPHYCTL_WORDS];
} dtrace_rate_t;

typedef struct _dtrace_txr_t {
	uint32 time_in_ms;
	uint16 link_idx;
	uint16 flags;
	dtrace_rate_t	rate_info_block[4];
} dtrace_txr_t;

typedef struct _dtrace_str_t {
	uint32 time_in_ms;
	char str[1];
} dtrace_str_t;

typedef struct _dtrace_utxd_t {
	uint32 time_in_ms;
	d11ulmu_txd_t utxd;
} dtrace_utxd_t;

#endif /* BCMDBG */

#ifdef WLC_MACDBG_FRAMEID_TRACE
/* tx pkt history entry */
struct pkt_hist {
	void *pkt;
	uint32 flags;
	uint32 flags3;
	uint16 frameid;
	uint16 seq;
	uint8 epoch;
	uint8 fifo;
	uint16 macctl_0;
	uint16 macctl_1;
	uint16 macctl_2;
	uint16 txchanspec;
	uint32 lbflags;
	void *scb;
	void *pktdata;
};

/* tx pkt history size need power of 2 for mod2 operation */
#define PKT_HIST_NUM_ENT 2048

/* tx status history entry */
struct txs_hist {
	void *pkt;
	uint16 frameid;
	uint16 seq;
	uint16 status;
	uint16 ncons;
	uint32 s1;
	uint32 s2;
	uint32 s3;
	uint32 s4;
	uint32 s5;
	uint32 ack_map1;
	uint32 ack_map2;
	uint32 s8;
	void *pktdata;
};

/* tx status history size need power of 2 for mod2 operation */
#define TXS_HIST_NUM_ENT 256

/* fifo sync history size */
#define SYNC_HIST_NUM_ENT 64

/* fifo sync history entry */
struct sync_hist {
	void *pkt;
};
#endif /* WLC_MACDBG_FRAMEID_TRACE */

#if defined(BCMDBG_DUMP_D11CNTS)

#define GRP_HIST_INFO_USED_CNT_MASK	0x000f
#define GRP_HIST_INFO_USED_CNT_SHIFT	0
#define GRP_HIST_INFO_M2VSND_CNT_MASK	0x0070
#define GRP_HIST_INFO_M2VSND_CNT_SHIFT	4
#define GRP_HIST_INFO_MU_TYPE_MASK	0x0380
#define GRP_HIST_INFO_MU_TYPE_SHIFT	7
#define GRP_HIST_INFO_GRP_IDX_MASK	0x0c00
#define GRP_HIST_INFO_GRP_IDX_SHIFT	10
#define GRP_HIST_INFO_GRP_EXT_MASK	0x1000
#define GRP_HIST_INFO_GRP_EXT_SHIFT	12
#define GRP_HIST_INFO_NUSR_CAND_MASK	0x6000
#define GRP_HIST_INFO_NUSR_CAND_SHIFT	13
#define GRP_HIST_INFO_SGI_MASK		0x8000
#define GRP_HIST_INFO_SGI_SHIFT		15

#define GRP_HIST_INFO_USED_CNT(val)	\
	((val & GRP_HIST_INFO_USED_CNT_MASK) >> GRP_HIST_INFO_USED_CNT_SHIFT)
#define GRP_HIST_INFO_M2VSND_CNT(val)	\
	((val & GRP_HIST_INFO_M2VSND_CNT_MASK) >> GRP_HIST_INFO_M2VSND_CNT_SHIFT)
#define GRP_HIST_INFO_MU_TYPE(val)	\
	((val & GRP_HIST_INFO_MU_TYPE_MASK) >> GRP_HIST_INFO_MU_TYPE_SHIFT)
#define GRP_HIST_INFO_GRP_IDX(val)	\
	((val & GRP_HIST_INFO_GRP_IDX_MASK) >> GRP_HIST_INFO_GRP_IDX_SHIFT)
#define GRP_HIST_INFO_GRP_EXT(val)	\
	((val & GRP_HIST_INFO_GRP_EXT_MASK) >> GRP_HIST_INFO_GRP_EXT_SHIFT)
#define GRP_HIST_INFO_NUSR_CAND(val)	\
	((val & GRP_HIST_INFO_NUSR_CAND_MASK) >> GRP_HIST_INFO_NUSR_CAND_SHIFT)
#define GRP_HIST_INFO_SGI(val)	\
	((val & GRP_HIST_INFO_SGI_MASK) >> GRP_HIST_INFO_SGI_SHIFT)

#define GRP_HIST_USR_TXVIDX_MASK	0x000f
#define GRP_HIST_USR_TXVIDX_SHIFT	0
#define GRP_HIST_USR_FIDX_MASK		0x00f0
#define GRP_HIST_USR_FIDX_SHIFT		4
#define GRP_HIST_USR_NSSMCS_MASK	0xff00
#define GRP_HIST_USR_NSSMCS_SHIFT	8

#define GRP_HIST_USR_TXVIDX(val)	\
	((val & GRP_HIST_USR_TXVIDX_MASK) >> GRP_HIST_USR_TXVIDX_SHIFT)
#define GRP_HIST_USR_FIDX(val)		\
	((val & GRP_HIST_USR_FIDX_MASK) >> GRP_HIST_USR_FIDX_SHIFT)
#define GRP_HIST_USR_NSSMCS(val)	\
	((val & GRP_HIST_USR_NSSMCS_MASK) >> GRP_HIST_USR_NSSMCS_SHIFT)

#include <packed_section_start.h>
typedef BWL_PRE_PACKED_STRUCT struct {
	uint16 info;		// w[0]
	uint16 txvidx_cand;	// w[1] 4bits per user up to 4 users
	uint16 usr[4];		// w[5:2]
} BWL_POST_PACKED_STRUCT grp_hist_blk_t;
#include <packed_section_end.h>

#define GRP_HIST_NUM		32
#define GRP_HIST_BLK_WSZ	(sizeof(grp_hist_blk_t) * GRP_HIST_NUM)

typedef struct {
	uint16 tx_cnt;
	uint16 txsucc_cnt;
} grp_hist_per_rt0_cnt_t;

typedef struct {
	grp_hist_per_rt0_cnt_t rt0_cnt[4]; // for gpos0~gpos3
} grp_hist_per_blk_t;
#define GRP_HIST_PER_BLK_WSZ	(sizeof(grp_hist_per_blk_t) * GRP_HIST_NUM)

#define FFQ_GAP_HIST_MAX	64 // keep it power of 2

#define FFQ_GAP_HIST_GAP_USEC_MASK	0x0fff
#define FFQ_GAP_HIST_GAP_USEC_SHIFT	0
#define FFQ_GAP_HIST_TX_TECH_MASK	0x3000
#define FFQ_GAP_HIST_TX_TECH_SHIFT	12
#define FFQ_GAP_HIST_AC_MASK		0xc000
#define FFQ_GAP_HIST_AC_SHIFT		14

#define FFQ_GAP_HIST_GAP_USEC(val)	\
	((val & FFQ_GAP_HIST_GAP_USEC_MASK) >> FFQ_GAP_HIST_GAP_USEC_SHIFT)
#define FFQ_GAP_HIST_TX_TECH(val)	\
	((val & FFQ_GAP_HIST_TX_TECH_MASK) >> FFQ_GAP_HIST_TX_TECH_SHIFT)
#define FFQ_GAP_HIST_AC(val)		\
	((val & FFQ_GAP_HIST_AC_MASK) >> FFQ_GAP_HIST_AC_SHIFT)

#include <packed_section_start.h>
typedef BWL_PRE_PACKED_STRUCT struct {
	uint32 last_ffq_sync[4];
	uint16 hist[FFQ_GAP_HIST_MAX];
	uint16 hist_idx;
} BWL_POST_PACKED_STRUCT ffq_gap_blk_t;
#include <packed_section_end.h>
#endif /* BCMDBG_DUMP_D11CNTS */

/* Module private states */

/* struct to save airtime info */
typedef struct {
	uint32 airtime;		/* accumulative airtime */
	uint32 num_upd;		/* number of update */
} wlc_macdbg_airtime_t;

typedef struct wlc_macdbg_perusr_info {
	uint8 fifo;
	uint8 mcs;
	uint8 nss;
	int8 ruidx;
} wlc_macdbg_perusr_info_t;

typedef struct wlc_macdbg_ppdu_stats {
	int8 epoch;
	uint8 nusrs;
	uint8 ntxs_recv;	/* track num txs received for MUtx case */
	uint8 pktBw;
	bool ismu;
	uint8 mutype;
	uint16 txtime;
	wlc_macdbg_perusr_info_t usr_info[16];
} wlc_macdbg_ppdu_stats_t;

enum {
	AIRTIME_TX_SU		= 0,
	AIRTIME_TX_VMU		= 1,
	AIRTIME_TX_OMU		= 2,
	AIRTIME_TX_HMU		= 3,
	AIRTIME_RX_ULMU		= 4,
	AIRTIME_ENUM_MAX	= 5
};

struct wlc_macdbg_info {
	wlc_info_t *wlc;
	int scbh;
	uint16 smpl_ctrl;	/* Sample Capture / Play Contrl */
	void *smpl_info;	/* Sample Capture Setup Params */
	CONST d11regs_list_t *pd11regs; /* dump register list */
	CONST d11regs_list_t *pd11regs_r1; /* dump register list */
	CONST d11regs_list_t *pd11regs_x; /* dump register list for second core if exists */
	uint d11regs_sz;
	uint d11regsr1_sz;
	uint d11regsx_sz;
	uint log_done;		/* reason bitmap */
	/* frameid tracing */
	pkt_hist_t *pkt_hist;
	int pkt_hist_cnt;
	txs_hist_t *txs_hist;
	int txs_hist_cnt;
	sync_hist_t *sync_hist;
	int sync_hist_next;
	int sync_hist_cnt;
	uint32	utrace_capture_count_bytes;
	uchar *exttrap_data_buf;
	uint32 exttrap_data_len;
	wlc_event_t *ed11regs;
	d11print_list_t *pd11print;
	uint d11print_sz;
	uint16 *d11cnts_snapshot;	/* snapshot of the last SHM read */
	uint32 *d11cnts;		/* 32bit bin for each counter */
	d11print_list_t *pd11printx;
	uint d11printx_sz;
	uint16 *d11cntsx_snapshot;	/* snapshot of the last SHMx read */
	uint32 *d11cntsx;		/* 32bit bin for each counter */
	uint16 dtrace_flag;		/* dtrace enabled type flag */
	uint16 dtrace_len;		/* current logged dtrace buffer length */
	uint8 *dtrace_buf;		/* dtrace buffer pointer */
	struct ether_addr dtrace_ea;	/* if not null, whitelist filter for scb->ea */
	uint8 *dmadump_buf;		/* dmadump buffer pointer */
	uint16 psmwd_reason;		/* flag to trigger a PSMWD */
	bool ppdutxs;			/* flag to enable ppdu txs */
	wlc_macdbg_ppdu_stats_t ppdu_info;	/* ppdu info */
	uint32 tsf_time;		/* 32bit timestamp */
	wlc_macdbg_airtime_t airtime_stats[AIRTIME_ENUM_MAX];
};

/* per scb cubby */
typedef struct {
	uint32 tsf_time;		/* 32bit tsf timestamp */
	wlc_macdbg_airtime_t tx_su;	/* accumulative Tx SU airtime */
	wlc_macdbg_airtime_t tx_omu;	/* accumulative Tx ofdma airtime */
	wlc_macdbg_airtime_t tx_mmu;	/* accumulative Tx vht/he mumimo airtime */
	wlc_macdbg_airtime_t rx_ulmu;	/* accumulative Rx ULMU airtime */
} wlc_macdbg_scb_cubby_t;

/* ucode dump constant */
#define BCM4345_BCM4350_BUF_SIZE_FOR_PMU_DUMP	400
#define BCM4345_BCM4350_BUF_SIZE_FOR_UCODE_DUMP	12000

#define	PRVAL(name)	bcm_bprintf(b, "%s %u ", #name, WLCNTVAL(cnt->name))
#define	PRNL()		bcm_bprintf(b, "\n")
#define PRVAL_RENAME(varname, prname)	\
	bcm_bprintf(b, "%s %u ", #prname, WLCNTVAL(cnt->varname))

#define PRREG_INDEX(name, reg) bcm_bprintf(b, #name " 0x%x ", R_REG(wlc->osh, &reg))
#define MAX_MUMIMOAGG_USERS	4
#define MAX_DLOFDMAAGG_USERS	16

#define BCN_TXERR_THRESH 10

typedef enum {
	SMPL_CAPTURE_GE50 = 0,
	SMPL_CAPTURE_LT50 = 1,
	SMPL_CAPTURE_NUM = 2
} smpl_capture_corerev_t;

/* Compile flag validity check */
#if defined(WLC_SRAMPMAC) || defined(WLC_HOSTPMAC)
#ifndef DONGLEBUILD
#error "WLC_SRAMPMAC and WLC_HOSTPMAC are only for DONGLEBUILD!"
#endif
#if defined(WLC_SRAMPMAC) && defined(WLC_HOSTPMAC)
#error "WLC_SRAMPMAC and WLC_HOSTPMAC should be mutually exclusive!"
#endif
#endif /* WLC_SRAMPMAC || WLC_HOSTPMAC */

#define C_SRSPTR_BSZ	4

static int wlc_macdbg_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint vsize, struct wlc_if *wlcif);
static int wlc_macdbg_doioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif);

static int wlc_macdbg_up(void *hdl);
#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC) || defined(WLC_HOSTPMAC)
static int wlc_macdbg_init_dumplist(wlc_macdbg_info_t *macdbg);
#endif /* WL_MACDBG || WLC_SRAMPMAC || WLC_HOSTPMAC */
#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC)
static int wlc_dump_mac(wlc_info_t *wlc, struct bcmstrbuf *b);
#if defined(WL_PSMR1)
static int wlc_dump_mac1(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif /* WL_PSMR1 */
#endif /* WL_MACDBG || WLC_SRAMPMAC */

#ifdef WL_MACDBG
static int wlc_pw_d11regs(wlc_info_t *wlc, CONST d11regs_list_t *pregs, int start_idx,
	struct bcmstrbuf *b, uint8 **p, bool w_en, uint32 w_val);
static int wlc_pd11regs_bylist(wlc_info_t *wlc, CONST d11regs_list_t *d11dbg1, uint d11dbg1_sz,
	int start_idx, struct bcmstrbuf *b, uint8 **p);

static int wlc_write_d11reg(wlc_info_t *wlc, int idx, enum d11reg_type_e type, uint16 byte_offset,
	uint32 w_val);

static int wlc_dump_shmem(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_dump_sctpl(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_dump_peruser(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_dump_bcntpls(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_dump_pio(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_macdbg_pmac(wlc_info_t *wlc, wl_macdbg_pmac_param_t *pmac,
	char *out_buf, int out_len);
#ifdef STA
static int wlc_dump_trig_tpc_params(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif
#ifdef WL_PSMX
static int wlc_dump_shmemx(wlc_info_t *wlc, struct bcmstrbuf *b);
static int wlc_dump_macx(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif
#else
#define wlc_write_d11reg(a, b, c, d, e) 0
#endif /* WL_MACDBG */

#if defined(BCMDBG) || defined(BCMDBG_TXSTALL)
static int wlc_dump_dma(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif

#if defined(BCMDBG)
static int wlc_dump_stats(wlc_info_t *wlc, struct bcmstrbuf *b);
/* MAC Sample Capture */
static int wlc_macdbg_smpl_capture_optns(wlc_info_t *wlc,
	wl_maccapture_params_t *params, bool set);
static int wlc_macdbg_smpl_capture_set(wlc_info_t *wlc, wl_maccapture_params_t *params);
static int wlc_macdbg_smpl_capture_get(wlc_info_t *wlc,
	char *outbuf, int outlen);
#endif /* BCMDBG */

#if defined(BCMDBG_DUMP_D11CNTS)
static int wlc_macdbg_init_printlist(wlc_macdbg_info_t *macdbg);
#ifdef WL_PSMX
static int wlc_macdbg_init_psmx_printlist(wlc_macdbg_info_t *macdbg);
#endif /* WL_PSMX */
static int wlc_dump_d11cnts(wlc_info_t *wlc, struct bcmstrbuf * b);
static int wlc_dump_clear_d11cnts(wlc_info_t *wlc);
static int wlc_dump_grphist(wlc_info_t *wlc, struct bcmstrbuf * b);
static int wlc_dump_ffqgap(wlc_info_t *wlc, struct bcmstrbuf * b);
#define PERCENT_STR_LEN		7
#endif /* BCMDBG_DUMP_D11CNTS */

#ifdef WLC_MACDBG_FRAMEID_TRACE
static int wlc_macdbg_frameid_trace_attach(wlc_macdbg_info_t *macdbg, wlc_info_t *wlc);
static void wlc_macdbg_frameid_trace_detach(wlc_macdbg_info_t *macdbg, wlc_info_t *wlc);
static int wlc_dump_pkt_hist(wlc_info_t *wlc, struct bcmstrbuf *b);
#endif

#if defined(BCMHWA) && (defined(WLTEST) || defined(HWA_DUMP) || defined(BCMDBG_ERR))
static int wlc_hwadbg_reg(wlc_info_t *wlc, wl_hwadbg_reg_param_t *params,
	char *out_buf, int out_len);
#endif

#ifdef BCMDBG
static int wlc_macdbg_set_dtrace(wlc_macdbg_info_t *macdbg, uint16 flag);
static int wlc_macdbg_gen_psmwd(wlc_macdbg_info_t *macdbg, uint16 after);
#ifdef WL_PSMX
static int wlc_macdbg_gen_psmxwd(wlc_macdbg_info_t *macdbg, uint16 after);
#endif /* WL_PSMX */
static void _wlc_macdbg_dtrace_print_buf(wlc_macdbg_info_t *macdbg);
#endif /* BCMDBG */

/* scb cubby */
static int wlc_macdbg_cubby_init(void *handle, struct scb *scb);
static void wlc_macdbg_cubby_deinit(void *handle, struct scb *scb);

#define MACDBG_SCB_LOC(macdbg, scb) ((wlc_macdbg_scb_cubby_t **)SCB_CUBBY(scb, (macdbg)->scbh))
#define MACDBG_SCB(macdbg, scb) (*MACDBG_SCB_LOC(macdbg, scb))

#define D11_RESET_SEQ_VAL_0	0xc7
#define D11_RESET_SEQ_VAL_1	0x15f
#define D11_RESET_SEQ_VAL_2	0x151
#define D11_RESET_SEQ_VAL_3	0x155
#define D11_RESET_SEQ_VAL_4	0xc5
#define MAIN_CORE_SR_SIZE	(256 * 1024)
#define AUX_CORE_SR_SIZE	(128 * 1024)
#define CORE_CLK_CTRL_STS_VAL	0x20
#define CORE_PWR_CTRL_MASK	0xf00
#define VASIP_CODE_OFFSET	0
#define VASIP_WRAP_BASE		0x18114000
#define VASIP_SR_START		0x10000
#define VASIP_SR_SIZE_4361 (32 * 1024)
#define VASIP_SR_SIZE_4369  (32 * 1536)

/* IF AOB not enabled PMU starts at 600 OFFSET from CHIPCOMMON */
#define PMU_OFFSET_AOB_DISAB	600

#define MACDBG_EXT_TRAP_DATABUF_SIZE	128

#define DMA_DUMP_LEN (384 * 1024)

#if defined(BCMDBG) || defined(WL_MACDBG)
#define MACDBG_SET_MACCPTR_PREG(regname, params) \
do { \
	W_REG(wlc->osh, regname ## _l, (uint16)((params) & 0xFFFF)); \
	W_REG(wlc->osh, regname ## _h, (uint16)(((params) >> 16) & 0xFFFF)); \
	if (use_psm) { \
		if (PSMX_ENAB(wlc->pub)) { \
			MACDBG_WRITE_MACREGX((regname ## _l), ((params) & 0xFFFF)); \
			MACDBG_WRITE_MACREGX((regname ## _h), (((params) >> 16) & 0xFFFF)); \
		} \
		if (PSMR1_ENAB(wlc->hw)) { \
			MACDBG_WRITE_MACREGR1((regname ## _l), ((params) & 0xFFFF)); \
			MACDBG_WRITE_MACREGR1((regname ## _h), (((params) >> 16) & 0xFFFF)); \
		} \
	} \
} while (0)

#define MACDBG_SET_MACPTR_CFG(regname, params) \
do { \
	W_REG(wlc->osh, regname, (uint16)((params) & 0xFFFF)); \
	if (use_psm) { \
		if (PSMX_ENAB(wlc->pub)) \
			MACDBG_WRITE_MACREGX(regname, ((params) & 0xFFFF)); \
		if (PSMR1_ENAB(wlc->hw)) \
			MACDBG_WRITE_MACREGR1(regname, ((params) & 0xFFFF)); \
	} \
} while (0)

// If multiple psms have gpio enable set, HW grants priority by X -> R0 -> R1.
#define MACDBG_GET_PSMSEL \
	((!use_psm) ? (WL_MACCAP_SEL_PSMR0) : \
		((MACDBG_SEL_ISPSMX) ? (WL_MACCAP_SEL_PSMX) : \
			((MACDBG_SEL_ISPSMR1) ? (WL_MACCAP_SEL_PSMR1) : (WL_MACCAP_SEL_PSMR0))))
#define MACDBG_SEL_ISPSMX \
	((PSMX_ENAB(wlc->pub)) ? (MACDBG_READ_MACREGX(D11_PSM_SMP_CTRL(wlc)) & SC_ENABLE) : (0))
#define MACDBG_SEL_ISPSMR1 \
	((PSMR1_ENAB(wlc->hw)) ? (MACDBG_READ_MACREGR1(D11_PSM_SMP_CTRL(wlc)) & SC_ENABLE) : (0))
#define MACDBG_SEL_ISPSMR0 \
	(R_REG(wlc->osh, D11_PSM_SMP_CTRL(wlc)) & SC_ENABLE)

#define MACDBG_READ_MACREGX(addr) (wlc_read_macregx(wlc, ((uint16) (long) addr)))
#define MACDBG_WRITE_MACREGX(addr, val) (wlc_write_macregx(wlc, ((uint16) (long) addr), val))
#define MACDBG_READ_MACREGR1(addr) (wlc_read_macreg1(wlc, ((uint16) (long) addr)))
#define MACDBG_WRITE_MACREGR1(addr, val) (wlc_write_macreg1(wlc, ((uint16) (long) addr), val))

#define MACDBG_RMODW_MACREGX(addr, mask) \
do { \
	if (PSMX_ENAB(wlc->pub)) { \
		MACDBG_WRITE_MACREGX(addr, MACDBG_READ_MACREGX(addr) & mask); \
	} \
} while (0)

#define MACDBG_RMODW_MACREGR1(addr, mask) \
do { \
	if (PSMR1_ENAB(wlc->hw)) { \
		MACDBG_WRITE_MACREGR1(addr, MACDBG_READ_MACREGR1(addr) & mask); \
	} \
} while (0)

#define MACDBG_MACCPTR_RREG(regname, psmsel) \
	((!use_psm) ? (R_REG(wlc->osh, regname)) : \
		((psmsel & WL_MACCAP_SEL_PSMX) ? (MACDBG_READ_MACREGX(regname)) : \
			((psmsel & WL_MACCAP_SEL_PSMR0) ? (R_REG(wlc->osh, regname)) : \
				(MACDBG_READ_MACREGR1(regname)))))

#define MACDBG_GET_MACCPTR_PREG(regname, params, psmsel) \
do { \
	(params) = MACDBG_MACCPTR_RREG(regname ## _h, psmsel); \
	(params) <<= 16; \
	(params) |= MACDBG_MACCPTR_RREG(regname ## _l, psmsel); \
} while (0)

#define WL_MACDBG_DFLT_STOREMASK		0xFFFFFFFF
#define WL_MACDBG_GPIO_PC			1
#define WL_MACDBG_GPIO_PCX			0x1d
#define WL_MACDBG_GPIO_PC1			0x31
#define WL_MACDBG_GPIO_SELPSMR0			1
#define WL_MACDBG_GPIO_SELPSMR1			2
#define WL_MACDBG_GPIO_SELPSMX			4
#define WL_MACDBG_GPIO_UTRACE			0
#define WL_MACDBG_GPIO_UTRACEX			0x36
#define WL_MACDBG_GPIO_UTRACE1			0x32
#define WL_MACDBG_DFLT_TRANSMASK_LT128		0x1FFF
#define WL_MACDBG_DFLT_TRANSMASK_GE128		0x3FFF
#define WL_MACDBG_DFLT_TRANSMASK_UTRACE		0xFFFF
#define WL_MACDBG_DFLT_TRANSMASK_ALL		0xFFFFFFFF
#define WL_MACDBG_DFLT_TRIGMASK_LT128		0x1FFF
#define WL_MACDBG_DFLT_TRIGMASK_GE128		0x3FFF
#define WL_MACDBG_IS_EXPERT_MODE		(params->optn_bmp & (1 << WL_MACCAPT_EXPERT))
#define WL_MACDBG_IS_PSMSEL_AUTO		(!(params->optn_bmp & (1 << WL_MACCAPT_PSMSEL)))
#define WL_MACCAPT_PSM_STRT_PTR			0
#define WL_MACCAPT_PSM_STOP_PTR			(0x3000 - 1)
#define WL_MACCAPT_PSM_SIZE	\
	(WL_MACCAPT_PSM_STOP_PTR - WL_MACCAPT_PSM_STRT_PTR + 1)

#ifdef LINUX_POSTMOGRIFY_REMOVAL
#define WL_MACCAPT_STOPTRIG	6
#define	WL_MACCAPT_USETXE	7
#define	WL_MACCAPT_USEPSM	8
#endif /* LINUX_POSTMOGRIFY_REMOVAL */

#define WL_MACDBG_IS_STOPTRIGGER(params) ((params)->optn_bmp & (1 << WL_MACCAPT_STOPTRIG))
#define WL_MACDBG_IS_USETXE(params) ((params)->optn_bmp & (1 << WL_MACCAPT_USETXE))
#define WL_MACDBG_IS_USEPSM(params) ((params)->optn_bmp & (1 << WL_MACCAPT_USEPSM))
#else /* BCMDBG || BCMDBG_DUMP */
#define WL_MACDBG_IS_STOPTRIGGER(params)	0
#define WL_MACDBG_IS_USETXE(params)		0
#define WL_MACDBG_IS_USEPSM(params)		0
#endif

/* airtime related macros */
#define MACDBG_AIRTIME_UPD(counter, delta) \
	do {(counter)->airtime += delta; (counter)->num_upd++;} while (0)
#define MACDBG_AIRTIME_AVG(counter) \
	((counter)->num_upd ? (counter)->airtime / ((counter)->num_upd) : 0)
#define MACDBG_AIRTIME_PERCENT(counter, total_time) \
	(total_time ? (counter)->airtime * 100 / (total_time) : 0)
#define MACDBG_NUMUPD_PERCENT(counter, tot_upd) \
	(tot_upd ? (counter)->num_upd * 100 / (tot_upd) : 0)

#define PHYTXERR_STS_MAC2PHY	0x0080	/* phytxerrorStatusReg0 bit '7' */
#define PHYTXERR_CTXTST_BCN	0x20	/* Tx-descr frame type sub-type beacon */

extern int wlc_get_sssr_reg_info(wlc_info_t *wlc, sssr_reg_info_t *arg);

static void wlc_suspend_mac_debug(wlc_info_t *wlc, uint32 phydebug);

void check_ta(wlc_info_t* wlc, bool ta_ok);

/** iovar table */
enum {
	IOV_MACDBG_PMAC    = 0,	 /* print mac */
	IOV_MACDBG_CAPTURE = 1,	 /* MAC Sample Capture */
	IOV_SRCHMEM        = 2,	 /* ucode search engine memory */
	IOV_DBGSEL         = 3,
	IOV_MACDBG_SHMX    = 4,	 /* set/get shmemx */
	IOV_MACDBG_REGX    = 5,	 /* set/get psmx regs */
	IOV_SSSR_REG_INFO  = 6,
	IOV_UTRACE         = 7,
	IOV_UTRACE_CAPTURE = 8,
	IOV_MACDBG_SHM1    = 9,
	IOV_MACDBG_REG1    = 10,
#ifdef BCMHWA
	IOV_HWADBG_REG     = 11, /* set/get hwa regs */
#endif
	IOV_DTRACE         = 12, /* driver trace */
	IOV_DTRACE_EA      = 13, /* driver trace for specified etheraddr */
	IOV_PSMWD_AFTER    = 14, /* manual cause psm wd */
	IOV_PSMXWD_AFTER   = 15, /* manual cause psmx wd */
	IOV_PSMWD_PHYTXERR = 16, /* manual cause psm wd on tx phy error */
	IOV_PSMWD_REASON = 17, /* trigger a PSMWD under certain conditions */
	IOV_MACDBG_PPDUTXS = 18, /* parse PPDU txs */
	IOV_PSMWD_PHYTXERR_TST = 19,    /* trigger a PSMWD for specific phytxerr tst code */
	IOV_PSMWD_PHYTXERR_THRESH = 20, /* if phytxerr cnt exceeds thresh */
	IOV_PSMWD_PHYTXERR_DUR = 21,    /* within this duration */
	IOV_LAST
};

static const bcm_iovar_t macdbg_iovars[] = {
	{"pmac", IOV_MACDBG_PMAC, (0), 0, IOVT_BUFFER, 0},
	{"mac_capture", IOV_MACDBG_CAPTURE, (0), 0, IOVT_BUFFER, 0},
#if defined(BCMDBG) && defined(MBSS)
	{"srchmem", IOV_SRCHMEM,
	(IOVF_SET_UP | IOVF_GET_UP), 0, IOVT_BUFFER, DOT11_MAX_SSID_LEN + 2 * sizeof(uint32)},
#endif	/* BCMDBG && MBSS */
	{"shmemx", IOV_MACDBG_SHMX, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_BUFFER, 0},
	{"shmem1", IOV_MACDBG_SHM1, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_BUFFER, 0},
	{"macregx", IOV_MACDBG_REGX, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_BUFFER, 0},
	{"macreg1", IOV_MACDBG_REG1, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_BUFFER, 0},
	{"sssr_reg_info", IOV_SSSR_REG_INFO, (IOVF_OPEN_ALLOW), 0, IOVT_BUFFER,
	sizeof(sssr_reg_info_t)},
#ifdef WL_UTRACE
	{"utrace", IOV_UTRACE, (IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_UINT16, 0},
	{"utrace_capture", IOV_UTRACE_CAPTURE,
	(IOVF_GET_UP | IOVF_SET_UP), 0, IOVT_BUFFER, WLC_UTRACE_LEN},
#endif /* WL_UTRACE */
#ifdef BCMHWA
	{"hwareg", IOV_HWADBG_REG, (0), 0, IOVT_BUFFER, 0},
#endif
	{"dtrace", IOV_DTRACE, (0), 0, IOVT_UINT16, 0},
	{"psmwd_after", IOV_PSMWD_AFTER, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_UINT16, 0},
	{"psmxwd_after", IOV_PSMXWD_AFTER, (IOVF_SET_CLK | IOVF_GET_CLK), 0, IOVT_UINT16, 0},
	{"dtrace_ea", IOV_DTRACE_EA, (0), 0, IOVT_BUFFER, 0},
	{"psmwd_phytxerr", IOV_PSMWD_PHYTXERR, (IOVF_SET_UP), 0, IOVT_UINT16, 0},
	{"psmwd_txerr", IOV_PSMWD_PHYTXERR, (IOVF_SET_UP), 0, IOVT_UINT16, 0},
	{"psmwd_reason", IOV_PSMWD_REASON, (IOVF_SET_UP), 0, IOVT_UINT16, 0},
	{"ppdutxs", IOV_MACDBG_PPDUTXS, (IOVF_SET_UP), 0, IOVT_UINT16, 0},
	{NULL, 0, 0, 0, 0, 0}
};
/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

void
BCMATTACHFN(wlc_macdbg_detach)(wlc_macdbg_info_t *macdbg)
{
	wlc_info_t *wlc;

	if (!macdbg)
		return;
	wlc = macdbg->wlc;

	wlc_module_unregister(wlc->pub, "macdbg", macdbg);

	(void)wlc_module_remove_ioctl_fn(wlc->pub, macdbg);

	if (macdbg->exttrap_data_buf) {
		MFREE(wlc->osh, macdbg->exttrap_data_buf, MACDBG_EXT_TRAP_DATABUF_SIZE);
		macdbg->exttrap_data_buf  = NULL;
	}

#if defined(BCMDBG)
	if (macdbg->pd11print) {
		MFREE(wlc->osh, macdbg->pd11print,
			(sizeof(macdbg->pd11print[0]) * macdbg->d11print_sz));
	}

	if (macdbg->d11cnts_snapshot) {
		MFREE(wlc->osh, macdbg->d11cnts_snapshot,
			(sizeof(macdbg->d11cnts_snapshot[0]) * macdbg->d11print_sz));
	}

	if (macdbg->d11cnts) {
		MFREE(wlc->osh, macdbg->d11cnts,
			(sizeof(macdbg->d11cnts[0]) * macdbg->d11print_sz));
	}

#ifdef WL_PSMX
	if (macdbg->pd11printx) {
		MFREE(wlc->osh, macdbg->pd11printx,
			(sizeof(macdbg->pd11printx[0]) * macdbg->d11printx_sz));
	}

	if (macdbg->d11cntsx_snapshot) {
		MFREE(wlc->osh, macdbg->d11cntsx_snapshot,
			(sizeof(macdbg->d11cntsx_snapshot[0]) * macdbg->d11printx_sz));
	}

	if (macdbg->d11cntsx) {
		MFREE(wlc->osh, macdbg->d11cntsx,
			(sizeof(macdbg->d11cntsx[0]) * macdbg->d11printx_sz));
	}
#endif /* WL_PSMX */

	macdbg->d11print_sz = 0;
	macdbg->d11printx_sz = 0;
#endif

#if defined(BCMDBG) || defined(BCMDBG_DUMP_D11CNTS) || defined(WL_MACDBG)
	if (macdbg->smpl_info) {
		MFREE(wlc->osh, macdbg->smpl_info, sizeof(wl_maccapture_params_t));
	}
#endif

#if defined(WLC_HOSTPMAC)
	if (macdbg->ed11regs) {
		wlc_event_free(wlc->eventq, macdbg->ed11regs);
		macdbg->ed11regs = NULL;
	}
#endif /* WLC_HOSTPMAC */

#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC) || defined(WLC_HOSTPMAC)
	macdbg->pd11regs = NULL;
	macdbg->d11regs_sz = 0;
	macdbg->pd11regs_r1 = NULL;
	macdbg->d11regsr1_sz = 0;
	macdbg->pd11regs_x = NULL;
	macdbg->d11regsx_sz = 0;
#endif /* WL_MACDBG || WLC_SRAMPMAC || WLC_HOSTPMAC */

#ifdef WLC_MACDBG_FRAMEID_TRACE
	wlc_macdbg_frameid_trace_detach(macdbg, wlc);
#endif
#ifdef BCMDBG
	wlc_macdbg_set_dtrace(macdbg, 0);
#endif /* BCMDBG */

	if (macdbg->dmadump_buf) {
		MFREE(wlc->osh, macdbg->dmadump_buf, DMA_DUMP_LEN);
		macdbg->dmadump_buf = NULL;
	}

	MFREE(wlc->osh, macdbg, sizeof(*macdbg));
}

wlc_macdbg_info_t *
BCMATTACHFN(wlc_macdbg_attach)(wlc_info_t *wlc)
{
	wlc_pub_t *pub = wlc->pub;
	wlc_macdbg_info_t *macdbg;

	if ((macdbg = MALLOCZ(wlc->osh, sizeof(wlc_macdbg_info_t))) == NULL) {
		WL_ERROR(("wl%d: %s: macdbg memory alloc. failed\n",
			wlc->pub->unit, __FUNCTION__));
		return NULL;
	}
	macdbg->wlc = wlc;

	if ((macdbg->exttrap_data_buf = MALLOCZ(wlc->osh, MACDBG_EXT_TRAP_DATABUF_SIZE)) == NULL) {
		WL_ERROR(("wl%d: %s: macdbg ext trap memory alloc. failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#ifdef WLC_MACDBG_FRAMEID_TRACE
	if (wlc_macdbg_frameid_trace_attach(macdbg, wlc) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_macdbg_frameid_trace_attach failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif

	if ((macdbg->scbh = wlc_scb_cubby_reserve(wlc,
		sizeof(wlc_macdbg_scb_cubby_t*),
		wlc_macdbg_cubby_init, wlc_macdbg_cubby_deinit, NULL, macdbg)) < 0) {

		WL_ERROR(("wl%d: %s: wlc_scb_cubby_reserve() failed\n", wlc->pub->unit,
			__FUNCTION__));
		goto fail;
	}

	if (wlc_module_add_ioctl_fn(pub, macdbg, wlc_macdbg_doioctl, 0, NULL) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_add_ioctl_fn() failed\n",
		          wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

	if ((wlc_module_register(pub, macdbg_iovars, "macdbg",
		macdbg, wlc_macdbg_doiovar, NULL, wlc_macdbg_up, NULL)) != BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_module_register() failed\n",
			wlc->pub->unit, __FUNCTION__));
		goto fail;
	}

#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC) || defined(WLC_HOSTPMAC)
	if (wlc_macdbg_init_dumplist(macdbg) != BCME_OK) {
		goto fail;
	}
#endif /* WL_MACDBG || WLC_SRAMPMAC || WLC_HOSTPMAC */

#if defined(BCMDBG_DUMP_D11CNTS)
	if (wlc_macdbg_init_printlist(macdbg) != BCME_OK) {
		goto fail;
	}

#ifdef WL_PSMX
	if (wlc_macdbg_init_psmx_printlist(macdbg) != BCME_OK) {
		goto fail;
	}
#endif /* WL_PSMX */
#endif /* BCMDBG_DUMP_D11CNTS */

#if defined(BCMDBG) || defined(BCMDBG_DUMP_D11CNTS) || defined(WL_MACDBG)
	if ((macdbg->smpl_info = MALLOCZ(wlc->osh, sizeof(wl_maccapture_params_t))) == NULL) {
		WL_ERROR(("wl%d: %s: smp_info memory alloc. failed\n",
				wlc->pub->unit, __FUNCTION__));
		goto fail;
	}
#endif

#if defined(BCMDBG) || defined(BCMDBG_TXSTALL)
	wlc_dump_register(pub, "dma", (dump_fn_t)wlc_dump_dma, (void *)wlc);
#endif
#if defined(BCMDBG)
	wlc_dump_register(pub, "stats", (dump_fn_t)wlc_dump_stats, (void *)wlc);
#endif

#if defined(BCMDBG_DUMP_D11CNTS)
	wlc_dump_add_fns(pub, "d11cnts", (dump_fn_t)wlc_dump_d11cnts,
		(clr_fn_t)wlc_dump_clear_d11cnts, (void *)wlc);
	wlc_dump_register(pub, "grphist", (dump_fn_t)wlc_dump_grphist, (void *)wlc);
	wlc_dump_register(pub, "ffqgap", (dump_fn_t)wlc_dump_ffqgap, (void *)wlc);
#endif /* BCMDBG_DUMP_D11CNTS */

#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC)
	wlc_dump_register(pub, "mac", (dump_fn_t)wlc_dump_mac, (void *)wlc);
#if defined(WL_PSMR1)
	wlc_dump_register(pub, "mac1", (dump_fn_t)wlc_dump_mac1, (void *)wlc);
#endif /* WL_PSMR1 */
#endif /* WL_MACDBG || WLC_SRAMPMAC */
#ifdef WL_MACDBG
	wlc_dump_register(pub, "shmem", (dump_fn_t)wlc_dump_shmem, (void *)wlc);
	wlc_dump_register(pub, "sctpl", (dump_fn_t)wlc_dump_sctpl, (void *)wlc);
	wlc_dump_register(pub, "peruser", (dump_fn_t)wlc_dump_peruser, (void *)wlc);
	wlc_dump_register(pub, "bcntpl", (dump_fn_t)wlc_dump_bcntpls, (void *)wlc);
	wlc_dump_register(pub, "pio", (dump_fn_t)wlc_dump_pio, (void *)wlc);
#ifdef WL_PSMX
	wlc_dump_register(pub, "macx", (dump_fn_t)wlc_dump_macx, (void *)wlc);
	wlc_dump_register(pub, "shmemx", (dump_fn_t)wlc_dump_shmemx, (void *)wlc);
#endif
#ifdef WLC_MACDBG_FRAMEID_TRACE
	wlc_dump_register(pub, "pkt_hist", (dump_fn_t)wlc_dump_pkt_hist, (void *)wlc);
#endif /* WLC_MACDBG_FRAMEID_TRACE */
#ifdef STA
	wlc_dump_register(pub, "trig_tpc_params", (dump_fn_t)wlc_dump_trig_tpc_params, (void *)wlc);
#endif
#endif /* WL_MACDBG */
	wlc->bcn_txerr_thresh = BCN_TXERR_THRESH;

	return macdbg;
fail:
	if (macdbg->exttrap_data_buf) {
		MFREE(wlc->osh, macdbg->exttrap_data_buf, MACDBG_EXT_TRAP_DATABUF_SIZE);
		macdbg->exttrap_data_buf  = NULL;
	}
	memset(&macdbg->dtrace_ea, '\0', ETHER_ADDR_LEN);

	MODULE_DETACH(macdbg, wlc_macdbg_detach);
	return NULL;
} /* wlc_macdbg_attach */

/*
 * SCB Cubby initialisation and cleanup handlers. Note the cubby itself is a pointer to a struct
 * which is only allocated when the macdbg command is used - until then, it is a NULL pointer.
 */
static int
wlc_macdbg_cubby_init(void *handle, struct scb *scb)
{
	wlc_macdbg_info_t *macdbg = handle;
	wlc_macdbg_scb_cubby_t **macdbg_scb_loc = MACDBG_SCB_LOC(macdbg, scb);

	*macdbg_scb_loc = NULL;
	return BCME_OK;
}

static void
wlc_macdbg_cubby_deinit(void *handle, struct scb *scb)
{
	wlc_macdbg_info_t *macdbg = handle;
	wlc_macdbg_scb_cubby_t **macdbg_scb_loc = MACDBG_SCB_LOC(macdbg, scb);
	wlc_macdbg_scb_cubby_t *macdbg_scb = MACDBG_SCB(macdbg, scb);

	if (macdbg_scb) {
		MFREE(macdbg->wlc->osh, macdbg_scb, sizeof(*macdbg_scb));
		*macdbg_scb_loc = NULL;
	}
}

extern int
wlc_get_sssr_reg_info(wlc_info_t *wlc, sssr_reg_info_t *ptr)
{
#if defined(WL_SSSR)
	uint32 regs;
	uint32 wrap_regs;
	si_t *sih;
	uint32 core_idx;

	if (wlc && wlc->pub) {
		sih = wlc->pub->sih;
	} else {
		return BCME_ERROR;
	}

	if (!ptr) {
		return BCME_ERROR;
	}

	ptr->version = SSSR_REG_INFO_VER;
	/* Length is Size of structure which will be validated at host */
	ptr->length = sizeof(*ptr);
	/* Save current core idx */
	core_idx = si_coreidx(sih);

	regs = (uint32)si_setcore(sih, D11_CORE_ID, 0);
	wrap_regs = (uint32)si_wrapperregs(sih);

	ptr->mac_regs[0].base_regs.xmtaddress = regs+D11_XMT_TEMPLATE_RW_PTR_OFFSET(wlc);
	ptr->mac_regs[0].base_regs.xmtdata = regs+D11_XMT_TEMPLATE_RW_DATA_OFFSET(wlc);
	ptr->mac_regs[0].base_regs.clockcontrolstatus = regs+D11_ClockCtlStatus_OFFSET(wlc);
	ptr->mac_regs[0].base_regs.clockcontrolstatus_val = CORE_CLK_CTRL_STS_VAL;
	ptr->mac_regs[0].wrapper_regs.resetctrl = wrap_regs + AI_RESETCTRL;
	ptr->mac_regs[0].wrapper_regs.itopoobb = wrap_regs + AI_ITIPOOBBOUT;
	ptr->mac_regs[0].wrapper_regs.ioctrl = wrap_regs + AI_IOCTRL;
	ptr->mac_regs[0].wrapper_regs.ioctrl_resetseq_val[0] = D11_RESET_SEQ_VAL_0;
	ptr->mac_regs[0].wrapper_regs.ioctrl_resetseq_val[1] = D11_RESET_SEQ_VAL_1;
	ptr->mac_regs[0].wrapper_regs.ioctrl_resetseq_val[2] = D11_RESET_SEQ_VAL_2;
	ptr->mac_regs[0].wrapper_regs.ioctrl_resetseq_val[3] = D11_RESET_SEQ_VAL_3;
	ptr->mac_regs[0].wrapper_regs.ioctrl_resetseq_val[4] = D11_RESET_SEQ_VAL_4;
	ptr->mac_regs[0].sr_size = MAIN_CORE_SR_SIZE;

	/* setcore chipcommon */
	regs = (uint32)si_setcore(sih, CC_CORE_ID, 0);
	wrap_regs = (uint32)si_wrapperregs(sih);
	ptr->chipcommon_regs.base_regs.intmask = regs + OFFSETOF(chipcregs_t, intmask);
	ptr->chipcommon_regs.base_regs.powerctrl = regs + OFFSETOF(chipcregs_t, powerctl);
	ptr->chipcommon_regs.base_regs.clockcontrolstatus =
		regs + OFFSETOF(chipcregs_t, clk_ctl_st);
	ptr->chipcommon_regs.base_regs.powerctrl_mask = CORE_PWR_CTRL_MASK;

	/* setcore pmu */
	if (AOB_ENAB(sih)) {
		regs = (uint32)si_setcore(sih, PMU_CORE_ID, 0);
	}
	else {
		regs = regs + PMU_OFFSET_AOB_DISAB;
	}

	ptr->pmu_regs.base_regs.pmuintmask0 = regs + OFFSETOF(pmuregs_t, pmuintmask0);
	ptr->pmu_regs.base_regs.pmuintmask1 = regs + OFFSETOF(pmuregs_t, pmuintmask1);
	ptr->pmu_regs.base_regs.resreqtimer = regs + OFFSETOF(pmuregs_t, res_req_timer);
	ptr->pmu_regs.base_regs.macresreqtimer = regs + OFFSETOF(pmuregs_t, mac_res_req_timer);
	ptr->pmu_regs.base_regs.macresreqtimer1 = regs + OFFSETOF(pmuregs_t, mac_res_req_timer1);

#if defined(__ARM_ARCH_7R__)
	/* setcorearm */
	regs = (uint32)si_setcore(sih, ARMCR4_CORE_ID, 0);
	wrap_regs = (uint32)si_wrapperregs(sih);

	ptr->arm_regs.base_regs.clockcontrolstatus = regs + OFFSETOF(cr4regs_t, clk_ctl_st);
	ptr->arm_regs.base_regs.clockcontrolstatus_val = CORE_CLK_CTRL_STS_VAL;
	ptr->arm_regs.wrapper_regs.resetctrl = wrap_regs + AI_RESETCTRL;
	ptr->arm_regs.wrapper_regs.itopoobb = wrap_regs + AI_ITIPOOBBOUT;
#endif /* __ARM_ARCH_7R__ */

	/* setcore pcie */
	regs = (uint32)si_setcore(sih, PCIE2_CORE_ID, 0);
	wrap_regs = (uint32)si_wrapperregs(sih);

	ptr->pcie_regs.base_regs.ltrstate = (uint32)(&(((sbpcieregs_t *)regs)->u.pcie2.ltr_state));
	ptr->pcie_regs.base_regs.clockcontrolstatus =
		(uint32)(&(((sbpcieregs_t *)regs)->u.pcie2.clk_ctl_st));
	/* RESET PCIECLK CNTRL STATUS VAL */
	ptr->pcie_regs.base_regs.clockcontrolstatus_val = 0;
	ptr->pcie_regs.wrapper_regs.itopoobb = wrap_regs + AI_ITIPOOBBOUT;

	if (VASIP_PRESENT(wlc->hw) &&
		si_setcore(wlc->pub->sih, ACPHY_CORE_ID, 0) != NULL) {
		uint32 vasipaddr = 0;
		/* get the VASIP memory base */
		vasipaddr = si_addrspace(wlc->pub->sih, CORE_SLAVE_PORT_0, CORE_BASE_ADDR_0);

		if (vasipaddr) {
			ptr->vasip_regs.wrapper_regs.ioctrl = VASIP_WRAP_BASE + AI_IOCTRL;
			ptr->vasip_regs.vasip_sr_addr =  vasipaddr + VASIP_SR_START;
			if (BCM4369_CHIP(wlc_hw->sih->chip)) {
			   ptr->vasip_regs.vasip_sr_size = VASIP_SR_SIZE_4369;
			}
			else if (CHIPID(wlc_hw->sih->chip) == BCM4361_CHIP_ID) {
				ptr->vasip_regs.vasip_sr_size = VASIP_SR_SIZE_4361;
			}
		}
	}

	/* Restore back the core idx */
	si_setcoreidx(sih, core_idx);
	return BCME_OK;
#else
	return BCME_UNSUPPORTED;
#endif /* WL_SSSR */
} /* wlc_get_sssr_reg_info */

/* add dump enum here */
static int
wlc_macdbg_doiovar(void *hdl, uint32 actionid,
	void *params, uint p_len, void *arg, uint len, uint vsize, struct wlc_if *wlcif)
{
	wlc_macdbg_info_t *macdbg = (wlc_macdbg_info_t*)hdl;
	wlc_info_t *wlc = macdbg->wlc;
	int32 int_val = 0, *ret_int_ptr;
	int err = BCME_OK;

	BCM_REFERENCE(wlc);
	BCM_REFERENCE(wlcif);
	BCM_REFERENCE(int_val);
	BCM_REFERENCE(ret_int_ptr);

	ASSERT(macdbg == wlc->macdbg);

	/* convenience int and bool vals for first 8 bytes of buffer */
	if (p_len >= (int)sizeof(int_val))
		bcopy(params, &int_val, sizeof(int_val));

	ret_int_ptr = (int32 *)arg;

	switch (actionid) {
#ifdef WL_MACDBG
	case IOV_GVAL(IOV_MACDBG_PMAC):
		err = wlc_macdbg_pmac(wlc, params, arg, len);
		break;
#endif /* WL_MACDBG */

#if defined(BCMDBG)
	case IOV_GVAL(IOV_MACDBG_CAPTURE):
		err = wlc_macdbg_smpl_capture_get(wlc, arg, len);
		break;

	case IOV_SVAL(IOV_MACDBG_CAPTURE):
	{
		wl_maccapture_params_t *maccapture_params = (wl_maccapture_params_t *)params;

		if (p_len < (int)sizeof(wl_maccapture_params_t)) {
			err = BCME_BUFTOOSHORT;
			break;
		}

		err = wlc_macdbg_smpl_capture_set(wlc, maccapture_params);
		break;
	}
#endif

#if defined(WL_PSMX)
#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_SHM)
	case IOV_GVAL(IOV_MACDBG_SHMX):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		ASSERT(PSMX_ENAB(wlc->pub));
		*ret_int_ptr = wlc_read_shmx(wlc, rwt->byteoff);
		break;
	}

	case IOV_SVAL(IOV_MACDBG_SHMX):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		ASSERT(PSMX_ENAB(wlc->pub));
		wlc_write_shmx(wlc, rwt->byteoff, (uint16)rwt->val);
		break;
	}
#endif /* BCMDBG || BCMQT || BCMDBG_SHM */

#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_ERR) || defined(WLTEST)
	case IOV_GVAL(IOV_MACDBG_REGX):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2 && rwt->size != 4) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		if (rwt->byteoff < D11REG_IHR_BASE) {
			*ret_int_ptr = R_REG(wlc->osh,
				(volatile uint32*)((uint8*)((uintptr)wlc->regs) + rwt->byteoff));
		} else {
			ASSERT(PSMX_ENAB(wlc->pub));
			*ret_int_ptr = wlc_read_macregx(wlc, rwt->byteoff);
		}
		break;
	}

	case IOV_SVAL(IOV_MACDBG_REGX):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2 && rwt->size != 4) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		if (rwt->byteoff < D11REG_IHR_BASE) {
			W_REG(wlc->osh,
				(volatile uint32*)((uint8*)((uintptr)wlc->regs) + rwt->byteoff),
				rwt->val);
		} else {
			ASSERT(PSMX_ENAB(wlc->pub));
			wlc_write_macregx(wlc, rwt->byteoff, (uint16)rwt->val);
		}
		break;
	}
#endif /* BCMDBG || BCMQT || BCMDBG_ERR || WLTEST */
#endif	/* WL_PSMX */

#if defined(WL_PSMR1)
#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_SHM)
	case IOV_GVAL(IOV_MACDBG_SHM1):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		ASSERT(PSMR1_ENAB(wlc->hw));
		*ret_int_ptr = wlc_read_shm1(wlc, rwt->byteoff);
		break;
	}

	case IOV_SVAL(IOV_MACDBG_SHM1):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		ASSERT(PSMR1_ENAB(wlc->hw));
		wlc_write_shm1(wlc, rwt->byteoff, (uint16)rwt->val);
		break;
	}
#endif /* BCMDBG || BCMQT || BCMDBG_SHM */
#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_ERR) || defined(WLTEST)
	case IOV_GVAL(IOV_MACDBG_REG1):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2 && rwt->size != 4) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		if (rwt->byteoff < D11REG_IHR_BASE) {
			*ret_int_ptr = R_REG(wlc->osh,
				(volatile uint32*)((uint8*)((uintptr)wlc->regs) + rwt->byteoff));
		} else {
			ASSERT(PSMR1_ENAB(wlc->hw));
			*ret_int_ptr = wlc_read_macreg1(wlc, rwt->byteoff);
		}
		break;
	}

	case IOV_SVAL(IOV_MACDBG_REG1):
	{
		rw_reg_t *rwt = params;
		if (rwt->size != 2 && rwt->size != 4) {
			err = BCME_BADLEN;
			break;
		}
		if (rwt->byteoff & (rwt->size - 1)) {
			err = BCME_BADADDR;
			break;
		}
		if (rwt->byteoff < D11REG_IHR_BASE) {
			W_REG(wlc->osh,
				(volatile uint32*)((uint8*)((uintptr)wlc->regs) + rwt->byteoff),
				rwt->val);
		} else {
			ASSERT(PSMR1_ENAB(wlc->hw));
			wlc_write_macreg1(wlc, rwt->byteoff, (uint16)rwt->val);
		}
		break;
	}
#endif /* BCMDBG || BCMQT || BCMDBG_ERR || WLTEST */
#endif /* WL_PSMR1 */
#ifdef WL_UTRACE
	case IOV_GVAL(IOV_UTRACE_CAPTURE):
		wlc_utrace_capture_get(wlc, arg, len);
		break;

	case IOV_GVAL(IOV_UTRACE):
		/* read the start address' 0th bit and return it. */
		*ret_int_ptr = (wlc_bmac_read_shm(wlc->hw, M_UTRACE_SPTR(wlc)) & 0x01);
		break;

	case IOV_SVAL(IOV_UTRACE):
		/* Set the start address' 0th bit. */
		wlc_bmac_write_shm(wlc->hw,
			M_UTRACE_SPTR(wlc),
			((UTRACE_SPTR_MASK &
			wlc_bmac_read_shm(wlc->hw, M_UTRACE_SPTR(wlc))) | (int_val & 0x1)));
		break;
#endif /* WL_UTRACE */
#if defined(BCMDBG) && defined(MBSS)
	case IOV_GVAL(IOV_SRCHMEM): {
		uint reg = (uint)int_val;

		if (reg > 15) {
			err = BCME_BADADDR;
			break;
		}

		wlc_bmac_copyfrom_objmem(wlc->hw, reg * SHM_MBSS_SSIDSE_BLKSZ(wlc),
			arg, SHM_MBSS_SSIDSE_BLKSZ(wlc), OBJADDR_SRCHM_SEL);
		break;
	}

	case IOV_SVAL(IOV_SRCHMEM): {
		uint reg = (uint)int_val;
		int *parg = (int *)arg;

		/* srchmem set params:
		 *	uint32		register num
		 *	uint32		ssid len
		 *	int8[32]	ssid name
		 */

		if (reg > 15) {
			err = BCME_BADADDR;
			break;
		}

		wlc_bmac_copyto_objmem(wlc->hw, reg * SHM_MBSS_SSIDSE_BLKSZ(wlc),
			(parg + 1), SHM_MBSS_SSIDSE_BLKSZ(wlc), OBJADDR_SRCHM_SEL);
		break;
	}
#endif	/* BCMDBG && MBSS */

	case IOV_GVAL(IOV_SSSR_REG_INFO): {
		sssr_reg_info_t *sssr_reg_info = (sssr_reg_info_t *)arg;
		err = wlc_get_sssr_reg_info(wlc, sssr_reg_info);
		break;
	}

	case IOV_SVAL(IOV_SSSR_REG_INFO): {
		err = BCME_UNSUPPORTED;
		break;
	}

#if defined(BCMHWA) && (defined(WLTEST) || defined(HWA_DUMP) || defined(BCMDBG_ERR))
	case IOV_GVAL(IOV_HWADBG_REG):
#if defined(WLTEST)
	case IOV_SVAL(IOV_HWADBG_REG):
#endif
		err = wlc_hwadbg_reg(wlc, params, arg, len);
		break;
#endif

#ifdef BCMDBG
	case IOV_GVAL(IOV_DTRACE):
		*ret_int_ptr = macdbg->dtrace_flag;
		err = BCME_OK;
		break;

	case IOV_SVAL(IOV_DTRACE):
		err = wlc_macdbg_set_dtrace(macdbg, int_val);
		break;

	case IOV_SVAL(IOV_PSMWD_AFTER):
		err = wlc_macdbg_gen_psmwd(macdbg, int_val);
		break;

#ifdef WL_PSMX
	case IOV_SVAL(IOV_PSMXWD_AFTER):
		err = wlc_macdbg_gen_psmxwd(macdbg, int_val);
		break;
#endif /* WL_PSMX */

	case IOV_SVAL(IOV_PSMWD_PHYTXERR):
		if (int_val) {
			wlc_mhf(wlc, MHF5, MHF5_TXERR_SPIN, MHF5_TXERR_SPIN, WLC_BAND_ALL);
			g_assert_type = 3; /* also change/set to 3 to do dump */
		} else {
			wlc_mhf(wlc, MHF5, MHF5_TXERR_SPIN, 0, WLC_BAND_ALL);
		}
		break;

	case IOV_GVAL(IOV_PSMWD_PHYTXERR):
		*ret_int_ptr = (wlc_mhf_get(wlc, MHF5, WLC_BAND_AUTO) & MHF5_TXERR_SPIN);
		err = BCME_OK;
		break;

	case IOV_SVAL(IOV_PSMWD_REASON):
		macdbg->psmwd_reason = int_val;
		break;

	case IOV_GVAL(IOV_PSMWD_REASON):
		*ret_int_ptr = macdbg->psmwd_reason;
		break;

	case IOV_GVAL(IOV_DTRACE_EA):
		memcpy(arg, &macdbg->dtrace_ea, ETHER_ADDR_LEN);
		err = BCME_OK;
		break;

	case IOV_SVAL(IOV_DTRACE_EA):
		memcpy(&macdbg->dtrace_ea, params, ETHER_ADDR_LEN);
		err = BCME_OK;
		break;

	case IOV_SVAL(IOV_MACDBG_PPDUTXS):
		if (!macdbg->ppdutxs && (bool) int_val) {
			// turning on the feature
			macdbg->ppdu_info.epoch = -1;
		}
		macdbg->ppdutxs = (bool) int_val;
		break;

	case IOV_GVAL(IOV_MACDBG_PPDUTXS):
		*ret_int_ptr = macdbg->ppdutxs;
		break;
#endif /* BCMDBG */

	default:
		err = BCME_UNSUPPORTED;
		break;
	}
	return err;
} /* wlc_macdbg_doiovar */

/* ioctl dispatcher */
static int
wlc_macdbg_doioctl(void *ctx, uint32 cmd, void *arg, uint len, struct wlc_if *wlcif)
{
	int bcmerror = BCME_OK;
	wlc_macdbg_info_t *macdbg = ctx;
	wlc_info_t *wlc = macdbg->wlc;
	int val, *pval;
	d11regs_t *regs = wlc->regs;
	uint band = 0;
	osl_t *osh = wlc->osh;
	uint i;
	bool ta_ok = FALSE;

	BCM_REFERENCE(ta_ok);
	BCM_REFERENCE(macdbg);
	BCM_REFERENCE(regs);
	BCM_REFERENCE(i);
	BCM_REFERENCE(band);
	BCM_REFERENCE(osh);

	/* default argument is generic integer */
	pval = (int *) arg;

	/* This will prevent the misaligned access */
	if ((uint32)len >= sizeof(val))
		bcopy(pval, &val, sizeof(val));
	else
		val = 0;

	switch (cmd) {

#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_SHM) || \
	defined(TESTBED_AP_11AX)
	case WLC_GET_UCFLAGS:
		if (!wlc->pub->up) {
			bcmerror = BCME_NOTUP;
			break;
		}

		/* optional band is stored in the second integer of incoming buffer */
		band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : ((int *)arg)[1];

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;
#ifdef WL_PSMX
		if (PSMX_ENAB(wlc->pub) && (val >= MXHF0)) {
			if (val >= MXHF0+MXHFMAX) {
				bcmerror = BCME_RANGE;
				break;
			}
		} else
#endif /* WL_PSMX  */
		if (val >= MHFMAX) {
			bcmerror = BCME_RANGE;
			break;
		}

		*pval = wlc_bmac_mhf_get(wlc->hw, (uint8)val, WLC_BAND_AUTO);
		break;

	case WLC_SET_UCFLAGS:
		if (!wlc->pub->up) {
			bcmerror = BCME_NOTUP;
			break;
		}

		/* optional band is stored in the second integer of incoming buffer */
		band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : ((int *)arg)[1];

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;
		i = (uint16)val;
#ifdef WL_PSMX
		if (PSMX_ENAB(wlc->pub) && (i >= MXHF0)) {
			if (i >= MXHF0+MXHFMAX) {
				bcmerror = BCME_RANGE;
				break;
			}
		} else
#endif /* WL_PSMX  */
		if (i >= MHFMAX) {
			bcmerror = BCME_RANGE;
			break;
		}

		wlc_mhf(wlc, (uint8)i, 0xffff, (uint16)(val >> NBITS(uint16)), WLC_BAND_AUTO);
		break;
#endif /* BCMDBG || BCMQT || BCMDBG_SHM || TESTBED_AP_11AX */

#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_SHM)
	case WLC_GET_SHMEM:
		ta_ok = TRUE;

		/* optional band is stored in the second integer of incoming buffer */
		band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : ((int *)arg)[1];

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;

		if (val & 1) {
			bcmerror = BCME_BADADDR;
			break;
		}

		*pval = wlc_read_shm(wlc, (uint16)val);
		break;

	case WLC_SET_SHMEM:
		ta_ok = TRUE;

		/* optional band is stored in the second integer of incoming buffer */
		band = (len < (int)(2 * sizeof(int))) ? WLC_BAND_AUTO : ((int *)arg)[1];

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;

		if (val & 1) {
			bcmerror = BCME_BADADDR;
			break;
		}

		wlc_write_shm(wlc, (uint16)val, (uint16)(val >> NBITS(uint16)));
		break;

	case WLC_W_REG:
	{
		rw_reg_t *r;
		ta_ok = TRUE;
		r = (rw_reg_t*)arg;
		band = WLC_BAND_AUTO;

		if (len < (int)(sizeof(rw_reg_t) - sizeof(uint))) {
			bcmerror = BCME_BUFTOOSHORT;
			break;
		}

		if (len >= (int)sizeof(rw_reg_t))
			band = r->band;

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;

		if ((r->byteoff < D11REG_IHR_BASE ? (r->byteoff & 0x3) : (r->byteoff & 0x1)) ||
			((r->byteoff + r->size-1) > D11REGS_T_MAX_SIZE)) {
			bcmerror = BCME_BADADDR;
			break;
		}

		if (r->size == sizeof(uint32))
			W_REG(osh, (uint32 *)((uchar *)(uintptr)regs + r->byteoff), r->val);
		else if (r->size == sizeof(uint16))
			W_REG(osh, (uint16 *)((uchar *)(uintptr)regs + r->byteoff), (uint16)r->val);
		else
			bcmerror = BCME_BADADDR;
		break;
	}
#endif /* BCMDBG || BCMQT || BCMDBG_SHM */
#if defined(BCMDBG) || defined(BCMQT) || defined(BCMDBG_ERR) || defined(WLTEST)
	case WLC_R_REG:	/* MAC registers */
	{
		rw_reg_t *r;
#if defined(BCMDBG) || defined(BCMQT)
		ta_ok = TRUE;
#endif /* BCMDBG || BCMQT */
		r = (rw_reg_t*)arg;
		band = WLC_BAND_AUTO;

		if (len < (int)(sizeof(rw_reg_t) - sizeof(uint))) {
			bcmerror = BCME_BUFTOOSHORT;
			break;
		}

		if (len >= (int)sizeof(rw_reg_t))
			band = r->band;

		/* HW is turned off so don't try to access it */
		if (wlc->pub->hw_off) {
			bcmerror = BCME_RADIOOFF;
			break;
		}

		/* bcmerror checking */
		if ((bcmerror = wlc_iocregchk(wlc, band)))
			break;

		if ((r->byteoff < D11REG_IHR_BASE ? (r->byteoff & 0x3) : (r->byteoff & 0x1)) ||
			((r->byteoff + r->size-1) > D11REGS_T_MAX_SIZE)) {
			bcmerror = BCME_BADADDR;
			break;
		}

		if (r->size == sizeof(uint32))
			r->val = R_REG(osh, (uint32 *)((uchar *)(uintptr)regs + r->byteoff));
		else if (r->size == sizeof(uint16))
			r->val = R_REG(osh, (uint16 *)((uchar *)(uintptr)regs + r->byteoff));
		else
			bcmerror = BCME_BADADDR;
		break;
	}
#endif /* BCMDBG || BCMQT|| BCMDBG_ERR || WLTEST */
	default:
		bcmerror = BCME_UNSUPPORTED;
		break;
	}

	check_ta(wlc, ta_ok);

	return bcmerror;
} /* wlc_macdbg_doioctl */

void
check_ta(wlc_info_t* wlc, bool ta_ok)
{
#if defined(BCMDBG) || defined(BCMQT)
	/* The sequence is significant.
	 *   Only if sbclk is TRUE, we can proceed with register access.
	 *   Even though ta_ok is TRUE, we still want to check(and clear) target abort
	 *   si_taclear returns TRUE if there was a target abort, In this case, ta_ok must be TRUE
	 *   to avoid assert
	 *   ASSERT and si_taclear are both under ifdef BCMDBG
	 */
	if (!(wlc->pub->hw_off))
		ASSERT(wlc_bmac_taclear(wlc->hw, ta_ok) || !ta_ok);
#endif /* BCMDBG || BCMQT */
}

static int
wlc_macdbg_up(void *hdl)
{
	wlc_macdbg_info_t *macdbg = (wlc_macdbg_info_t *)hdl;
	wlc_info_t *wlc = macdbg->wlc;
	BCM_REFERENCE(wlc);

	macdbg->ppdu_info.epoch = -1;
	macdbg->ppdutxs = TRUE;

#if defined(BCMDBG)
	/* Sample Capture conflicts with coex - if so, don't enable by default */
	if (!(BTCX_ENAB(wlc->hw))) {
		if (((macdbg->smpl_ctrl) & SC_STRT) && (macdbg->smpl_info)) {
			/* If MAC Sample Capture is set-up, start */
			wlc_macdbg_smpl_capture_set(wlc,
					(wl_maccapture_params_t *)macdbg->smpl_info);
		} else if ((macdbg->smpl_info) && D11REV_GE(wlc->pub->corerev, 129)) {
			/* Enable psmr0 PC sample capture by default */
			wl_maccapture_params_t params;
			memset(&params, 0, sizeof(params));
			if (D11REV_GE(wlc->pub->corerev, 129)) {
				params.optn_bmp = (0xf << WL_MACCAPT_LOGMODE);
			}
			params.gpio_sel = WL_MACDBG_GPIO_PC;

			wlc_macdbg_smpl_capture_set(wlc, &params);
		}
	}
#endif

#ifdef WL_UTRACE
	wlc_utrace_init(wlc);
#endif
	macdbg->log_done = 0;

#if defined(WLC_HOSTPMAC)
	if (macdbg->ed11regs) {
		wlc_event_if(wlc, NULL, macdbg->ed11regs, NULL);
		wlc_process_event(wlc, macdbg->ed11regs);
		/* the event will automatically be freed after sent */
		macdbg->ed11regs = NULL;
	}
#endif /* WLC_HOSTPMAC */

	return BCME_OK;
} /* wlc_macdbg_up */

#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC) || defined(WLC_HOSTPMAC)
static int
BCMATTACHFN(wlc_macdbg_init_dumplist)(wlc_macdbg_info_t *macdbg)
{
	wlc_info_t *wlc = macdbg->wlc;
	uint32 corerev = wlc->pub->corerev;
	uint32 corerev_minor = wlc->pub->corerev_minor;
	int err = BCME_OK;
#if defined(WLC_HOSTPMAC)
	uint desc_idx = 0;
	uint16 event_datalen = 0;
	uint8 *xtlvbuf;
	xtlv_desc_t xtlv_desc[] = {
		{0, 0, NULL},	/* PSMr */
		{0, 0, NULL},	/* PSMx */
		{0, 0, NULL},	/* sctpl regs */
		{0, 0, NULL},	/* SVMP */
		{0, 0, NULL}	/* end marker */
	};
#if D11CONF_GE(128)
	svmp_list_t svmpmems_ge128[] = {
		{0x40000, 0x80},	// interrupt & FW info/config/knob
		{0x40080, 0x40},	// m2v_buf0
		{0x40100, 0x40},	// m2v_buf1
		{0x38000, 0x200},	// m2v/v2m buf for grouping & grp_forced_buf
		{0x40180, 0x80},	// m2v/v2m buf for precoding
		{0x38500, 0x80},	// m2v/v2m buf for on-the-fly grouping and sounding_update
		{0x2bb20, 0x20},	// pwr_per_ant & pwr_per_ant
		{0x40280, 0x400},	// bfds_log_buffer
		{0x40c00, 0x200},	// hwsch_log_buffer
		{0x40e00, 0x200},	// hwsch_v2m_buffer
		{0x30000, 0x20},	// txv_decompressed_report 0
		{0x30900, 0x20},	// txv_decompressed_report 1
		{0x31200, 0x20},	// txv_decompressed_report 2
		{0x31b00, 0x20},	// txv_decompressed_report 3
		{0x3c010, 0x20},	// steering_mcs & recommend_mcs
		{0x3c080, 0x80},	// txv_header_addr
		{0x46000, 0x180},	// ru allocation - ru_alloc_buf & v2m_ru_alloc_buf & ...
		{0x46200, 0x40},	// ru allocation - cqi_rpt_buf & v2m_buf_cqi
		{0x46240, 0x400},	// ru allocation - cqi_thpt_gain
		{0x46800, 0x400},	// ru allocation - cqi_ru_index
		{0x46dc0, 0x400},	// ru allocation - cqi_ru_mask
		{0x47380, 0x20},	// ru allocation - cqi_ready
		{0x47b00, 0x190},	// ru allocation - dbg0
		{0xe000,  0x2000},	// CQI report
		{0x10000, 0x8000},	// TXV_IDX =  0 ~ 31
		{0x18000, 0x8000},	// TXV_IDX = 32 ~ 63
		{0x20000, 0x20},	// TXV_IDX = 64
		{0x22000, 0x20},	// TXV_IDX = 65
		{0x24000, 0x20},	// TXV_IDX = 66
		{0x26000, 0x20}		// TXV_IDX = 67
	};
#endif /* D11CONF_GE(128) */

#if (D11CONF_GE(64) || D11CONF_IS(61))
	svmp_list_t svmpmems[] = {
		{0x20000, 256},
		{0x21e10, 16},
		{0x20300, 16},
		{0x20700, 16},
		{0x20b00, 16},
		{0x20be0, 16},
		{0x20bff, 16},
		{0xc000, 32},
		{0xe000, 32},
		{0x10000, 0x8000},
		{0x18000, 0x8000}
	};
#endif /* D11CONF_GE(64) || D11CONF_IS(61) */

#if D11CONF_GE(40)
	sctplregs_list_t sctplregs = {
		wlc->pub->corerev,
		M_BOM_REV_MAJOR(wlc),
		M_BOM_REV_MINOR(wlc),
		M_UCODE_FEATURES(wlc),
#if (D11CONF_GE(54) || D11CONF_IS(50))
		D11_SMP_CTRL_OFFSET(wlc),
#else
		D11_PSM_PHY_CTL(wlc),
#endif /* D11CONF_GE(64) || D11CONF_IS(61) */
		D11_MacControl1_OFFSET(wlc),
		D11_SCP_STRTPTR_OFFSET(wlc),
		D11_SCP_STOPPTR_OFFSET(wlc),
		D11_SCP_CURPTR_OFFSET(wlc),
		D11_XMT_TEMPLATE_RW_PTR_OFFSET(wlc),
		D11_XMT_TEMPLATE_RW_DATA_OFFSET(wlc),
		0,
		0,
		0,
#if defined(WL_PSMX)
		MX_UTRACE_SPTR(wlc),
		MX_UTRACE_EPTR(wlc),
		0x10,
#else
		0,
		0,
		0,
#endif /* WL_PSMX */
		0 /* for pad field */
};
#endif /* D11CONF_GE(40) */
#endif /* WLC_HOSTPMAC */

#ifdef WLC_MINMACLIST
	if (D11REV_GE(corerev, 40)) {
		macdbg->pd11regs = d11regsmin_ge40;
		macdbg->d11regs_sz = d11regsmin_ge40sz;
	} else {
		macdbg->pd11regs = d11regsmin_pre40;
		macdbg->d11regs_sz = d11regsmin_pre40sz;
	}
	macdbg->pd11regs_x = NULL;
#else /* WLC_MINMACLIST */
	if (D11REV_LT(corerev, 40)) {
		macdbg->pd11regs = d11regs_pre40;
		macdbg->d11regs_sz = d11regs_pre40sz;
	} else if (D11REV_IS(corerev, 48)) {
		macdbg->pd11regs = d11regs48;
		macdbg->d11regs_sz = d11regs48sz;
	} else if (D11REV_IS(corerev, 49)) {
		macdbg->pd11regs = d11regs49;
		macdbg->d11regs_sz = d11regs49sz;
	} else if (D11REV_IS(corerev, 61)) {
		if (wlc->pub->corerev_minor == 5) {
			macdbg->pd11regs = d11regs61dot5;
			macdbg->d11regs_sz = d11regs61dot5sz;
		} else {
			macdbg->pd11regs = d11regs61;
			macdbg->d11regs_sz = d11regs61sz;
		}
	} else if (D11REV_GE(corerev, 65) && D11REV_LT(corerev, 128)) {
		macdbg->pd11regs = d11regs65;
		macdbg->d11regs_sz = d11regs65sz;
	} else if (D11REV_IS(corerev, 129)) {
		macdbg->pd11regs = d11regs129;
		macdbg->d11regs_sz = d11regs129sz;
#if defined(WL_PSMR1)
		macdbg->pd11regs_r1 = d11regsr1129;
		macdbg->d11regsr1_sz = d11regsr1129sz;
#endif
	} else if (D11REV_IS(corerev, 130)) { /* rev130 has no PsmR1 */
		if (D11MINORREV_IS(corerev_minor, 2)) {
			macdbg->pd11regs = d11regs130_2;
			macdbg->d11regs_sz = d11regs130_2sz;
		} else {
			macdbg->pd11regs = d11regs130;
			macdbg->d11regs_sz = d11regs130sz;
		}
	} else if (D11REV_IS(corerev, 131)) {
		macdbg->pd11regs = d11regs131;
		macdbg->d11regs_sz = d11regs131sz;
	} else if (D11REV_IS(corerev, 132)) {
		macdbg->pd11regs = d11regs132;
		macdbg->d11regs_sz = d11regs132sz;
#if defined(WL_PSMR1)
		macdbg->pd11regs_r1 = d11regsr1132;
		macdbg->d11regsr1_sz = d11regsr1132sz;
#endif
	} else {
		/* Default */
		macdbg->pd11regs = d11regs42;
		macdbg->d11regs_sz = d11regs42sz;
	}
#if defined(WL_PSMX)
	if (D11REV_GE(corerev, 65) && D11REV_LT(corerev, 128)) {
		macdbg->pd11regs_x = d11regsx65;
		macdbg->d11regsx_sz = d11regsx65sz;
	} else if (D11REV_IS(corerev, 129)) {
		macdbg->pd11regs_x = d11regsx129;
		macdbg->d11regsx_sz = d11regsx129sz;
	} else if (D11REV_IS(corerev, 130)) {
		if (D11MINORREV_IS(corerev_minor, 2)) {
			macdbg->pd11regs_x = d11regsx130_2;
			macdbg->d11regsx_sz = d11regsx130_2sz;
		} else {
			macdbg->pd11regs_x = d11regsx130;
			macdbg->d11regsx_sz = d11regsx130sz;
		}
	} else if (D11REV_IS(corerev, 131)) {
		macdbg->pd11regs_x = d11regsx131;
		macdbg->d11regsx_sz = d11regsx131sz;
	} else if (D11REV_IS(corerev, 132)) {
		macdbg->pd11regs_x = d11regsx132;
		macdbg->d11regsx_sz = d11regsx132sz;
	} else {
		macdbg->pd11regs_x = NULL;
		macdbg->d11regsx_sz = 0;
	}
#endif /* WL_PSMX */
#endif /* WLC_MINMACLIST */

	WL_TRACE(("%s d11reg_sz %d d11regsx_sz %d\n", __FUNCTION__,
		macdbg->d11regs_sz, macdbg->d11regsx_sz));

#if defined(WLC_HOSTPMAC)
	/* prepare event(s) to send in wl up time */
	MALLOC_SET_NOPERSIST(wlc->osh);
	macdbg->ed11regs = wlc_event_alloc(wlc->eventq, WLC_E_MACDBG);
	if (macdbg->ed11regs == NULL) {
		WL_ERROR(("wl%d: %s R wlc_event_alloc failed\n", wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto exit;
	}
	macdbg->ed11regs->event.status = WLC_E_STATUS_SUCCESS;
	macdbg->ed11regs->event.reason = WLC_E_MACDBG_LISTALL;

	xtlv_desc[desc_idx].type = D11REG_XTLV_PSMR;
	xtlv_desc[desc_idx].len = macdbg->d11regs_sz * (uint16)sizeof(macdbg->pd11regs[0]);
	/* trick to move CONST pointer */
	memcpy(&xtlv_desc[desc_idx].ptr, &macdbg->pd11regs, sizeof(void *));
	event_datalen += bcm_xtlv_size_for_data(xtlv_desc[desc_idx].len, BCM_XTLV_OPTION_ALIGN32);
	desc_idx++;

#if defined(WL_PSMR1)
	if (PSMR1_ENAB(wlc->hw) && macdbg->pd11regs_r1 && macdbg->d11regsr1_sz > 0) {
		WL_ERROR(("wl%d: %s send PSMr1 list\n", wlc->pub->unit, __FUNCTION__));
		xtlv_desc[desc_idx].type = D11REG_XTLV_PSMR1;
		xtlv_desc[desc_idx].len =
			macdbg->d11regsr1_sz * (uint16)sizeof(macdbg->pd11regs_r1[0]);
		/* trick to move CONST pointer */
		memcpy(&xtlv_desc[desc_idx].ptr, &macdbg->pd11regs_r1, sizeof(void *));
		event_datalen +=
			bcm_xtlv_size_for_data(xtlv_desc[desc_idx].len, BCM_XTLV_OPTION_ALIGN32);
		desc_idx++;
	}
#endif /* WL_PSMR1 */

#if defined(WL_PSMX)
	if (PSMX_ENAB(wlc->pub) && macdbg->pd11regs_x && macdbg->d11regsx_sz > 0) {
		xtlv_desc[desc_idx].type = D11REG_XTLV_PSMX;
		xtlv_desc[desc_idx].len =
			macdbg->d11regsx_sz * (uint16)sizeof(macdbg->pd11regs_x[0]);
		/* trick to move CONST pointer */
		memcpy(&xtlv_desc[desc_idx].ptr, &macdbg->pd11regs_x, sizeof(void *));
		event_datalen +=
			bcm_xtlv_size_for_data(xtlv_desc[desc_idx].len, BCM_XTLV_OPTION_ALIGN32);
		desc_idx++;
	}
#endif /* WL_PSMX */

#if (D11CONF_GE(64) || D11CONF_IS(61))
	xtlv_desc[desc_idx].type = D11REG_XTLV_SVMP;
#if D11CONF_GE(128)
	if (D11REV_GE(corerev, 128)) {
		xtlv_desc[desc_idx].len = sizeof(svmpmems_ge128);
		xtlv_desc[desc_idx].ptr = svmpmems_ge128;
	} else
#endif
	{
		xtlv_desc[desc_idx].len = sizeof(svmpmems);
		xtlv_desc[desc_idx].ptr = svmpmems;
	}

	event_datalen += bcm_xtlv_size_for_data(xtlv_desc[desc_idx].len, BCM_XTLV_OPTION_ALIGN32);
	desc_idx++;
#endif /* (D11CONF_GE(64) || D11CONF_IS(61)) */

#if D11CONF_GE(40)
	xtlv_desc[desc_idx].type = D11REG_XTLV_SCTPL;
	xtlv_desc[desc_idx].len = (uint16)sizeof(sctplregs);
	xtlv_desc[desc_idx].ptr = &sctplregs;
	event_datalen += bcm_xtlv_size_for_data(xtlv_desc[desc_idx].len, BCM_XTLV_OPTION_ALIGN32);
	desc_idx++;
#endif /* D11CONF_GE(40) */
	ASSERT(event_datalen > 0);

	macdbg->ed11regs->event.datalen = event_datalen;
	macdbg->ed11regs->data = wlc_event_data_alloc(wlc->eventq, event_datalen, WLC_E_MACDBG);
	if (macdbg->ed11regs->data == NULL) {
		WL_ERROR(("wl%d: %s wlc_event_data_alloc failed\n",
			wlc->pub->unit, __FUNCTION__));
		err = BCME_NOMEM;
		goto exit;
	}

	xtlvbuf = (uint8 *)macdbg->ed11regs->data;
	err = bcm_pack_xtlv_buf_from_mem(&xtlvbuf,
		&event_datalen, xtlv_desc, BCM_XTLV_OPTION_ALIGN32);

	WL_TRACE(("%s err %d edlen %d\n", __FUNCTION__, err, event_datalen));

exit:
	/* event / event_data will be freed in detach function in case of malloc failure */
	MALLOC_CLEAR_NOPERSIST(wlc->osh);
#if !defined(WL_MACDBG)
	/* DHD will keep the list, and dongle will never need these again. */
	macdbg->pd11regs = NULL;
	macdbg->d11regs_sz = 0;
	macdbg->pd11regs_r1 = NULL;
	macdbg->d11regsr1_sz = 0;
	macdbg->pd11regs_x = NULL;
	macdbg->d11regsx_sz = 0;
#endif
#endif /* WLC_HOSTPMAC */

	return err;
} /* wlc_macdbg_init_dumplist */
#endif /* WL_MACDBG || WLC_SRAMPMAC || WLC_HOSTPMAC */

#if defined(WL_MACDBG) || defined(WLC_SRAMPMAC)
/* dump functions */
static int
wlc_print_d11reg(wlc_info_t *wlc, int idx, enum d11reg_type_e type, uint16 addr,
	struct bcmstrbuf *b, uint8 **p)
{
	osl_t *osh;
	uint16 val16 = -1;
	uint32 val32 = -1;
	bool print16 = TRUE;
	volatile uint8 *paddr;
	const char *regname[D11REG_TYPE_MAX] = D11REGTYPENAME;

	osh = wlc->osh;

	paddr = (volatile uint8*)(D11_biststatus(wlc)) - 0xC; /* start address of d11 enum space */

	switch (type) {
	case D11REG_TYPE_IHR32:
		val32 = R_REG(osh, (volatile uint32*)(paddr + addr));
		print16 = FALSE;
		break;
	case D11REG_TYPE_IHR16:
		if (addr < PIHR2K_BASE) {
			val16 = R_REG(osh, (volatile uint16*)(paddr + addr));
		} else {
			val16 = wlc_bmac_read_ihr(wlc->hw, (addr - PIHR_BASE) >> 1);
		}
		break;
	case D11REG_TYPE_SCR:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr << 2,
			&val16, sizeof(val16), OBJADDR_SCR_SEL);
		break;
	case D11REG_TYPE_SHM:
		val16 = wlc_read_shm(wlc, addr);
		break;
	case D11REG_TYPE_TPL:
		W_REG(osh, D11_XMT_TEMPLATE_RW_PTR(wlc), (uint32)addr);
		val32 = R_REG(osh, D11_XMT_TEMPLATE_RW_DATA(wlc));
		print16 = FALSE;
		break;
#if defined(WL_PSMX)
	case D11REG_TYPE_KEYTB:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr,
			&val32, sizeof(val32), OBJADDR_KEYTBL_SEL);
		print16 = FALSE;
		break;
	case D11REG_TYPE_IHRX16:
		ASSERT(PSMX_ENAB(wlc->pub));
		val16 = wlc_read_macregx(wlc, addr);
		break;
	case D11REG_TYPE_SCRX:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr << 2,
			&val16, sizeof(val16), OBJADDR_SCRX_SEL);
		break;
	case D11REG_TYPE_SHMX:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr,
			&val16, sizeof(val16), OBJADDR_SHMX_SEL);
		break;
#endif /* WL_PSMX */

#if defined(WL_PSMR1)
	case D11REG_TYPE_IHR116:
		ASSERT(PSMR1_ENAB(wlc->hw));
		val16 = wlc_read_macreg1(wlc, addr);
		break;
	case D11REG_TYPE_SCR1:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr << 2,
			&val16, sizeof(val16), OBJADDR_SCR1_SEL);
		break;
	case D11REG_TYPE_SHM1:
		wlc_bmac_copyfrom_objmem(wlc->hw, addr,
			&val16, sizeof(val16), OBJADDR_SHM1_SEL);
		break;
#endif /* WL_PSMR1 */

	default:
		if (b) {
			bcm_bprintf(b, "%s: unrecognized type %d!\n", __FUNCTION__, type);
		} else {
			printf("%s: unrecognized type %d!\n", __FUNCTION__, type);
		}
		return 0;
	}

	if (print16) {
		if (b) {
			bcm_bprintf(b, "%-3d %s 0x%-4x = 0x%-4x\n",
				idx, regname[type], addr, val16);
		} else if (!p) {
			printf("%-3d %s 0x%-4x = 0x%-4x\n",
			       idx, regname[type], addr, val16);
		}
		if (p) {
			*((uint16 *)(*p)) = val16;
			*p += sizeof(val16);
		}
	} else {
		if (b) {
			bcm_bprintf(b, "%-3d %s 0x%-4x = 0x%-8x\n",
				idx, regname[type], addr, val32);
		} else if (!p) {
			printf("%-3d %s 0x%-4x = 0x%-8x\n",
			       idx, regname[type], addr, val32);
		}
		if (p) {
			*((uint32 *)(*p)) = val32;
			*p += sizeof(val32);
		}
	}

	return 1;
} /* wlc_print_d11reg */

static int
wlc_pw_d11regs(wlc_info_t *wlc, CONST d11regs_list_t *pregs,
	int start_idx, struct bcmstrbuf *b, uint8 **p,
	bool w_en, uint32 w_val)
{
	uint32 lbmp;
	uint16 byte_offset; /**< relative to the start of the d11 enum space */
	uint16 lcnt;
	int idx;

	byte_offset = pregs->byte_offset;
	idx = start_idx;
	if (pregs->type >= D11REG_TYPE_MAX) {
		if (b && !w_en) {
			bcm_bprintf(b, "%s: wrong type %d\n", __FUNCTION__, pregs->type);
		} else {
			printf("%s: wrong type %d\n", __FUNCTION__, pregs->type);
		}
		return 0;
	}

	lbmp = pregs->bitmap;
	lcnt = pregs->cnt;
	while (lbmp || lcnt) {
		WL_TRACE(("idx %d bitmap %#x cnt %d byte_offset %#x\n",
			idx, lbmp, lcnt, byte_offset));
		if ((lbmp && (lbmp & 0x1)) || (!lbmp && lcnt)) {
			if (w_en) {
				idx += wlc_write_d11reg(wlc, idx, pregs->type, byte_offset, w_val);
			} else {
				idx += wlc_print_d11reg(wlc, idx, pregs->type, byte_offset, b, p);
			}
			if (lcnt) lcnt --;
		}

		lbmp = lbmp >> 1;
		byte_offset += pregs->step;
	}

	return (idx - start_idx);
} /* wlc_pw_d11regs */

static int
wlc_pd11regs_bylist(wlc_info_t *wlc, CONST d11regs_list_t *d11dbg1,
	uint d11dbg1_sz, int start_idx, struct bcmstrbuf *b, uint8 **p)
{
	CONST d11regs_list_t *pregs;
	int i, idx;
	bool suspend_mac = TRUE;

	if (!wlc->clk)
		return BCME_NOCLK;

	/* Don't suspend mac if under psmwd state. Otherwise(in iovar operation) suspend it */
	if (wlc->psm_watchdog_debug) {
		suspend_mac = FALSE;
	}

	WL_TRACE(("%s: ucode compile time 0x%04x 0x%04x\n", __FUNCTION__,
		wlc_read_shm(wlc, 0x4), wlc_read_shm(wlc, 0x6)));

	idx = start_idx;

	if (suspend_mac)
		wlc_bmac_suspend_mac_and_wait(wlc->hw);

	for (i = 0; i < (int)d11dbg1_sz; i++) {
		pregs = &(d11dbg1[i]);
		idx += wlc_pw_d11regs(wlc, pregs, idx, b, p, FALSE, 0);
	}

	if (suspend_mac)
		wlc_bmac_enable_mac(wlc->hw);

	return (idx - start_idx);
}

static int
wlc_dump_mac(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int cnt = 0;

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	ASSERT(wlc->macdbg->pd11regs != NULL && wlc->macdbg->d11regs_sz > 0);
	cnt = wlc_pd11regs_bylist(wlc, wlc->macdbg->pd11regs,
		wlc->macdbg->d11regs_sz, 0, b, NULL);

	return (cnt > 0 ? BCME_OK : BCME_ERROR);
}

#if defined(WL_PSMR1)
static int
wlc_dump_mac1(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int cnt = 0;

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	ASSERT(wlc->macdbg->pd11regs != NULL && wlc->macdbg->d11regsr1_sz > 0);
	cnt = wlc_pd11regs_bylist(wlc, wlc->macdbg->pd11regs_r1,
		wlc->macdbg->d11regsr1_sz, 0, b, NULL);

	return (cnt > 0 ? BCME_OK : BCME_ERROR);
}
#endif /* WL_PSMR1 */
#endif /* WL_MACDBG || WLC_SRAMPMAC */

#ifdef WL_MACDBG
/* dump functions */
static int
wlc_write_d11reg(wlc_info_t *wlc, int idx, enum d11reg_type_e type, uint16 byte_offset,
	uint32 w_val)
{
	osl_t *osh;
	uint16 w_val16 = (uint16)w_val;
	volatile uint8 *paddr;

	osh = wlc->osh;

	paddr = (volatile uint8*)(D11_biststatus(wlc)) - 0xC;

	switch (type) {
	case D11REG_TYPE_IHR32:
		W_REG(osh, (volatile uint32*)(paddr + byte_offset), w_val);
		break;
	case D11REG_TYPE_IHR16:
		if (byte_offset < PIHR2K_BASE) {
			W_REG(osh, (volatile uint16*)(paddr + byte_offset), w_val16);
		} else {
			wlc_bmac_write_ihr(wlc->hw, (byte_offset - PIHR_BASE) >> 1, w_val16);
		}
		break;
	case D11REG_TYPE_SCR:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset << 2,
			&w_val16, sizeof(w_val16), OBJADDR_SCR_SEL);
		break;
	case D11REG_TYPE_SHM:
		wlc_write_shm(wlc, byte_offset, w_val16);
		break;
	case D11REG_TYPE_TPL:
		W_REG(osh, D11_XMT_TEMPLATE_RW_PTR(wlc), (uint32)byte_offset);
		W_REG(osh, D11_XMT_TEMPLATE_RW_DATA(wlc), w_val);
		break;
#if defined(WL_PSMX)
	case D11REG_TYPE_KEYTB:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset,
			&w_val, sizeof(w_val), OBJADDR_KEYTBL_SEL);
		break;
	case D11REG_TYPE_IHRX16:
		wlc_write_macregx(wlc, byte_offset, w_val16);
		break;
	case D11REG_TYPE_SCRX:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset << 2,
			&w_val16, sizeof(w_val16), OBJADDR_SCRX_SEL);
		break;
	case D11REG_TYPE_SHMX:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset,
			&w_val16, sizeof(w_val16), OBJADDR_SHMX_SEL);
		break;
#endif /* WL_PSMX */

#if defined(WL_PSMR1)
	case D11REG_TYPE_IHR116:
		wlc_write_macreg1(wlc, byte_offset, w_val16);
		break;
	case D11REG_TYPE_SCR1:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset << 2,
			&w_val16, sizeof(w_val16), OBJADDR_SCR1_SEL);
		break;
	case D11REG_TYPE_SHM1:
		wlc_bmac_copyto_objmem(wlc->hw, byte_offset,
			&w_val16, sizeof(w_val16), OBJADDR_SHM1_SEL);
		break;
#endif /* WL_PSMR1 */

	default:
		printf("%s: unrecognized type %d!\n", __FUNCTION__, type);
		return 0;
	}

	return 1;
} /* wlc_write_d11reg */

static int
wlc_pw_d11regs_byaddr(wlc_info_t *wlc, CONST d11regs_addr_t *d11addrs,
	uint d11addrs_sz, int start_idx, struct bcmstrbuf *b,
	bool w_en, uint32 w_val)
{
	CONST d11regs_addr_t *paddrs;
	int i, j, idx;

	if (!wlc->clk)
		return BCME_NOCLK;

	idx = start_idx;
	for (i = 0; i < (int)d11addrs_sz; i++) {
		paddrs = &(d11addrs[i]);
		if (paddrs->type >= D11REG_TYPE_MAX) {
			if (b && !w_en)
				bcm_bprintf(b, "%s: wrong type %d. Skip %d entries.\n",
					__FUNCTION__, paddrs->type, paddrs->cnt);
			else
				printf("%s: wrong type %d. Skip %d entries.\n",
				       __FUNCTION__, paddrs->type, paddrs->cnt);
			continue;
		}

		for (j = 0; j < paddrs->cnt; j++) {
			if (w_en) {
				idx += wlc_write_d11reg(wlc, idx, paddrs->type,
					paddrs->byte_offset[j], w_val);
			} else {
				idx += wlc_print_d11reg(wlc, idx, paddrs->type,
					paddrs->byte_offset[j], b, NULL);
			}
		}
	}

	return (idx - start_idx);
}

static int
wlc_macdbg_pmac(wlc_info_t *wlc,
	wl_macdbg_pmac_param_t *params, char *out_buf, int out_len)
{
	uint8 i, type;
	int err = BCME_OK, idx = 0;
	struct bcmstrbuf bstr, *b;
	wl_macdbg_pmac_param_t pmac_params;
	wl_macdbg_pmac_param_t *pmac = &pmac_params;
	d11regs_list_t d11dbg1;
	d11regs_addr_t d11dbg2;
	bool skip1st = FALSE;
	bool align4 = FALSE;

	memcpy(pmac, params, sizeof(wl_macdbg_pmac_param_t));
	bcm_binit(&bstr, out_buf, out_len);
	b = &bstr;

	if (WL_TRACE_ON()) {
		printf("%s:\n", __FUNCTION__);
		printf("type %s\n", pmac->type);
		printf("step %u\n", pmac->step);
		printf("num  %u\n", pmac->num);
		printf("bitmap %#x\n", pmac->bitmap);
		printf("addr_raw %d\n", pmac->addr_raw);
		for (i = 0; i < pmac->addr_num; i++)
			printf("\taddr = %#x\n", pmac->addr[i]);
	}

	if (pmac->addr_num == 0) {
		bcm_bprintf(b, "%s: no address is given!\n", __FUNCTION__);
		//err = BCME_BADARG;
		goto exit;
	}

	if (!strncmp(pmac->type, "shmx", 4)) {
		type = D11REG_TYPE_SHMX;
	} else if (!strncmp(pmac->type, "ihrx", 4)) {
		type = D11REG_TYPE_IHRX16;
	} else if (!strncmp(pmac->type, "keytb", 5)) {
		type = D11REG_TYPE_KEYTB;
		align4 = TRUE;
	} else if (!strncmp(pmac->type, "scrx", 4)) {
		type = D11REG_TYPE_SCRX;
	} else if (!strncmp(pmac->type, "ihr1", 4)) {
		type = D11REG_TYPE_IHR116;
	} else if (!strncmp(pmac->type, "scr1", 4)) {
		type = D11REG_TYPE_SCR1;
	} else if (!strncmp(pmac->type, "shm1", 4)) {
		type = D11REG_TYPE_SHM1;
	} else if (!strncmp(pmac->type, "scr", 3)) {
		type = D11REG_TYPE_SCR;
	} else if (!strncmp(pmac->type, "shm", 3)) {
		type = D11REG_TYPE_SHM;
	} else if (!strncmp(pmac->type, "tpl", 3)) {
		type = D11REG_TYPE_TPL;
		align4 = TRUE;
	} else if (!strncmp(pmac->type, "ihr32", 5)) {
		type = D11REG_TYPE_IHR32;
		align4 = TRUE;
	} else if (!strncmp(pmac->type, "ihr", 3)) {
		type = D11REG_TYPE_IHR16;
	} else {
		bcm_bprintf(b, "Unrecognized type: %s!\n", pmac->type);
		err = BCME_BADARG;
		goto exit;
	}

	if (type >= D11REG_TYPE_GE64 && D11REV_LT(wlc->pub->corerev, 64)) {
		bcm_bprintf(b, "%s: unsupported type %s for corerev %d!\n",
			__FUNCTION__, pmac->type, wlc->pub->corerev);
		goto exit;
	}

	if (type == D11REG_TYPE_SCR || type == D11REG_TYPE_SCRX ||
	    (type == D11REG_TYPE_SCR1)) {
		if (pmac->step == (uint8)(-1)) {
			/* Set the default step when it is not given */
			pmac->step = 1;
		}
	} else {
		uint16 mask = align4 ? 0x3 : 0x1;

		for (i = 0; i < pmac->addr_num; i++) {
			if (pmac->addr_raw && !align4) {
				/* internal address => external address.
				 * only applies to 16-bit access type
				 */
				if ((type == D11REG_TYPE_IHR16) ||
					(type == D11REG_TYPE_IHRX16) ||
					(type == D11REG_TYPE_IHR116)) {
					pmac->addr[i] += D11REG_IHR_WBASE;
				}
				pmac->addr[i] <<= 1;
			}

			if ((type == D11REG_TYPE_IHR16 || type == D11REG_TYPE_IHRX16 ||
			    (type == D11REG_TYPE_IHR116)) &&
			    pmac->addr[i] < D11REG_IHR_BASE) {
				/* host address space: convert local type */
				type = D11REG_TYPE_IHR32;
				align4 = TRUE;
			}

			mask = align4 ? 0x3 : 0x1;
			if ((pmac->addr[i] & mask)) {
				/* Odd addr not expected here for external addr */
				WL_ERROR(("%s: addr %#x is not %s aligned!\n",
					__FUNCTION__, pmac->addr[i],
					align4 ? "dword" : "word"));
				pmac->addr[i] &= ~mask;
			}
		}

		if (pmac->step == (uint8)(-1)) {
			/* Set the default step when it is not given */
			pmac->step = align4 ? 4 : 2;
		} else if (pmac->step & mask) {
			/* step size validation check. */
			bcm_bprintf(b, "%s: wrong step size %d for type %s\n",
				__FUNCTION__, pmac->step, pmac->type);
			goto exit;
		}
	}

	/* generate formatted lists */
	if (pmac->bitmap || pmac->num) {
		skip1st = TRUE;
		d11dbg1.type = type;
		d11dbg1.byte_offset = pmac->addr[0];
		d11dbg1.bitmap = pmac->bitmap;
		d11dbg1.step = pmac->step;
		d11dbg1.cnt = pmac->num;
		WL_TRACE(("d11dbg1: type %d, byte_offset 0x%x, bitmap 0x%x, step %d, cnt %d\n",
			d11dbg1.type,
			d11dbg1.byte_offset,
			d11dbg1.bitmap,
			d11dbg1.step,
			d11dbg1.cnt));
		idx += wlc_pw_d11regs(wlc, &d11dbg1, idx, b, NULL,
			(bool)pmac->w_en, pmac->w_val);
	}

	if (pmac->addr_num) {
		int num = pmac->addr_num;
		uint16 *paddr = d11dbg2.byte_offset;

		if (skip1st) {
			num --;
			paddr ++;
		}

		d11dbg2.type = type;
		d11dbg2.cnt = (uint16)num;
		memcpy(paddr, pmac->addr, (sizeof(uint16) * num));
		WL_TRACE(("d11dbg2: type %d cnt %d\n",
			d11dbg2.type,
			d11dbg2.cnt));
		for (i = 0; i < num; i++) {
			WL_TRACE(("[%d] 0x%x\n", i, d11dbg2.byte_offset[i]));
		}

		if (num > 0) {
			idx += wlc_pw_d11regs_byaddr(wlc, &d11dbg2, 1, idx, b,
				(bool)pmac->w_en, pmac->w_val);
		}
	}

	if (idx == 0) {
		/* print nothing */
		err = BCME_BADARG;
	}

exit:
	return err;
} /* wlc_macdbg_pmac */

static int
wlc_dump_shmem(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	uint i, k = 0;
	uint16 val, addr, end;

	static const shmem_list_t shmem_list[] = {
		{0, 3*1024},
	};

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	for (i = 0; i < ARRAYSIZE(shmem_list); i++) {
		end = shmem_list[i].start + 2 * shmem_list[i].cnt;
		for (addr = shmem_list[i].start; addr < end; addr += 2) {
			val = wlc_read_shm(wlc, addr);
			if (b) {
				bcm_bprintf(b, "%d shm 0x%03x = 0x%04x\n",
					k++, addr, val);
			} else {
				printf("%d shm 0x%03x = 0x%04x\n",
				       k++, addr, val);
			}
		}
	}

	return 0;
}

static int
wlc_dump_peruser(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	osl_t *osh;
	wlc_pub_t *pub;
	uint16 regmux, csb_req, qmap, num_mpdus;
	uint16 reglist_sz, cnt = 0;
	int i, num_txdbg;
	uint16 j;

	uint16 txdbg_sel[] = {
		0x0000, 0x0001, 0x0002,
		0x0100, 0x0110, 0x0200,
		0x0210, 0x0300, 0x0310
	};

	d11regs_list_t reglist[] = {
		{D11REG_TYPE_IHR16, 0x5d0, 0x00000001, 2, 0},
		{D11REG_TYPE_IHR16, 0x7b0, 0x00000001, 2, 0},
		{D11REG_TYPE_IHR16, 0x922, 0x00000017, 2, 0},
		{D11REG_TYPE_IHR16, 0xa7c, 0x00000003, 2, 0},
		{D11REG_TYPE_IHR16, 0xb14, 0x00000129, 2, 0},
		{D11REG_TYPE_IHR16, 0xb70, 0x00400001, 2, 0},
		{D11REG_TYPE_IHR16, 0xbce, 0x00002003, 2, 0},
		{D11REG_TYPE_IHR16, 0xc12, 0x7f80705f, 2, 0},
		{D11REG_TYPE_IHR16, 0xc52, 0x0078010f, 2, 0},
		{D11REG_TYPE_IHR16, 0xcd4, 0x00000001, 2, 0},
		{D11REG_TYPE_IHR16, 0xd66, 0x0000007f, 2, 0},
		{D11REG_TYPE_IHR16, 0xe0a, 0x0180001f, 2, 0}
	};

	d11regs_list_t per_mpdu_list[] = {
		{D11REG_TYPE_IHR16, 0xd64, 0x00000fff, 2, 0}
	};

	d11regs_list_t csb_reglist0[] = {
		{D11REG_TYPE_IHR16, 0xd60, 0x00040001, 2, 0}
	};

	d11regs_list_t csb_reglist2_3[] = {
		{D11REG_TYPE_IHR16, 0xd60, 0x01e00001, 2, 0}
	};

	d11regs_list_t csb_reglist5[] = {
		{D11REG_TYPE_IHR16, 0xd60, 0x40000001, 2, 0}
	};

	osh = wlc->osh;
	pub = wlc->pub;
	num_txdbg = ARRAYSIZE(txdbg_sel);
	reglist_sz = ARRAYSIZE(reglist);

	if (D11REV_LT(pub->corerev, 129)) {
		WL_ERROR(("wl%d: %s only supported for corerev >= 129\n",
			pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	reglist_sz = ARRAYSIZE(reglist);
	regmux = R_REG(osh, D11_PSM_REG_MUX(wlc));

	for (i = 0; i < MAX_NUSR(pub->corerev); i++) {
		W_REG(osh, D11_PSM_REG_MUX(wlc), (uint16)(0xff00 | i));
		cnt += wlc_pd11regs_bylist(wlc, reglist, reglist_sz, 0, b, NULL);

		/* per mpdu */
		num_mpdus = R_REG(osh, D11_AQMF_STATUS(wlc)) & 0x3f;
		if (b) {
			bcm_bprintf(b, "0   ihr 0xcc8  = 0x%x\n", num_mpdus);
		} else {
			printf("0   ihr 0xcc8  = 0x%x\n", num_mpdus);
		}

		num_mpdus = MIN(num_mpdus, MPDUS_DUMP_NUM);
		for (j = 0; j < num_mpdus; j++) {
			W_REG(osh, D11_AQMF_RPTR(wlc), j);
			while (!(R_REG(osh, D11_AQMF_RPTR(wlc)) & 0x8000));
			cnt += wlc_pd11regs_bylist(wlc, per_mpdu_list, 1, 0, b, NULL);
		}

		/* txdbg_sel */
		for (j = 0; j < num_txdbg; j++) {
			if (j < 3) {
				W_REG(osh, D11_TXE_TXDBG_SEL(wlc), txdbg_sel[j]);
			} else {
				W_REG(osh, D11_TXE_TXDBG_SEL(wlc), (uint16)(txdbg_sel[j] | i));
			}
			if (b) {
				bcm_bprintf(b, "0x%x\n", R_REG(osh, D11_TXE_TXDBG_DATA(wlc)));
			} else {
				printf("0x%x\n", R_REG(osh, D11_TXE_TXDBG_DATA(wlc)));
			}
		}

		/* aqmcsb */
		qmap = R_REG(osh, D11_AQM_QMAP(wlc));
		csb_req = R_REG(osh, D11_AQMCSB_REQ(wlc));
		W_REG(osh, D11_AQMCSB_BASEL(wlc), (uint16)0);
		W_REG(osh, D11_AQMCSB_REQ(wlc), (uint16)((csb_req & 0xff80) | qmap));
		for (j = 0; j <= 5; j++) {
			if (j == 1 || j == 4) {
				continue;
			}

			csb_req = R_REG(osh, D11_AQMCSB_REQ(wlc));
			W_REG(osh, D11_AQMCSB_REQ(wlc), (uint16)((csb_req & 0xfcff) | (2 << 8)));
			csb_req = R_REG(osh, D11_AQMCSB_REQ(wlc));
			W_REG(osh, D11_AQMCSB_REQ(wlc), (uint16)((csb_req & 0xc3ff) | (j << 10)));
			while (!(R_REG(osh, D11_AQMCSB_REQ(wlc)) & 0x8000));
			if (j == 0) {
				cnt += wlc_pd11regs_bylist(wlc, csb_reglist0, 1, 0, b, NULL);
			} else if (j == 2 || j == 3) {
				cnt += wlc_pd11regs_bylist(wlc, csb_reglist2_3, 1, 0, b, NULL);
			} else {
				cnt += wlc_pd11regs_bylist(wlc, csb_reglist5, 1, 0, b, NULL);
			}
		}
	}

	/* Need to restore before exiting */
	W_REG(osh, D11_PSM_REG_MUX(wlc), regmux);

	return (cnt > 0) ? BCME_OK : BCME_UNSUPPORTED;
}

static int
wlc_dump_sctpl(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	osl_t *osh;
	wlc_pub_t *pub;
	uint i;
	int gpio_sel, len, offset;
	uint16 scpctl, addr0, addr1, curptr;
	uint8 psmsel;
	wl_maccapture_params_t *cur_params = (wl_maccapture_params_t *)wlc->macdbg->smpl_info;
	volatile uint16 *smpl_ctrl_reg, *strtptr_reg, *stopptr_reg, *curptr_reg;
	volatile uint32 *memptr_reg, *memdata_reg;
	bool use_psm = FALSE;

	BCM_REFERENCE(cur_params);

	pub = wlc->pub;
	if (D11REV_LT(pub->corerev, 40)) {
		WL_ERROR(("wl%d: %s only supported for corerev >= 40\n",
			pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	osh = wlc->osh;

	use_psm = (WL_MACDBG_IS_USEPSM(cur_params) && D11REV_GE(wlc->pub->corerev, 129));
	psmsel = MACDBG_GET_PSMSEL;

	/* stop sample capture */
	if (D11REV_GE(pub->corerev, 56)) {
		smpl_ctrl_reg = use_psm ? D11_PSM_SMP_CTRL(wlc) : D11_SMP_CTRL(wlc);
		scpctl = R_REG(osh, smpl_ctrl_reg);
		W_REG(osh, smpl_ctrl_reg, (uint16)(scpctl & ~SC_STRT));
		if (use_psm) {
			MACDBG_RMODW_MACREGX(smpl_ctrl_reg, ~SC_STRT);
			MACDBG_RMODW_MACREGR1(smpl_ctrl_reg, ~SC_STRT);
		}
	} else {
		scpctl = R_REG(osh, D11_PSM_PHY_CTL(wlc));
		W_REG(osh, D11_PSM_PHY_CTL(wlc), (uint16)(scpctl & ~PHYCTL_SC_STRT));
	}

	strtptr_reg = use_psm ? D11_PSM_SCP_STRTPTR(wlc) : D11_SCP_STRTPTR(wlc);
	stopptr_reg = use_psm ? D11_PSM_SCP_STOPPTR(wlc) : D11_SCP_STOPPTR(wlc);
	curptr_reg = use_psm ? D11_PSM_SCP_CURPTR(wlc) : D11_SCP_CURPTR(wlc);
	memptr_reg = use_psm ? (volatile uint32*)D11_PSM_GpioMonMemPtr(wlc) :
		(volatile uint32*)D11_XMT_TEMPLATE_RW_PTR(wlc);
	memdata_reg = use_psm ? (volatile uint32*)D11_PSM_GpioMonMemData(wlc) :
		(volatile uint32*)D11_XMT_TEMPLATE_RW_DATA(wlc);

	if (b) {
		bcm_bprintf(b, "corerev: %d.%d ucode revision %d.%d features 0x%04x\n",
			pub->corerev, pub->corerev_minor, wlc_read_shm(wlc, M_BOM_REV_MAJOR(wlc)),
			wlc_read_shm(wlc, M_BOM_REV_MINOR(wlc)),
			wlc_read_shm(wlc, M_UCODE_FEATURES(wlc)));
	} else {
		printf("corerev: %d.%d ucode revision %d.%d features 0x%04x\n",
			pub->corerev, pub->corerev_minor, wlc_read_shm(wlc, M_BOM_REV_MAJOR(wlc)),
			wlc_read_shm(wlc, M_BOM_REV_MINOR(wlc)),
			wlc_read_shm(wlc, M_UCODE_FEATURES(wlc)));
	}

	gpio_sel = R_REG(osh, D11_MacControl1(wlc));
	addr0 = MACDBG_MACCPTR_RREG(strtptr_reg, psmsel);
	addr1 = MACDBG_MACCPTR_RREG(stopptr_reg, psmsel);
	curptr = MACDBG_MACCPTR_RREG(curptr_reg, psmsel);
	len = (addr1 - addr0 + 1) * 4;
	offset = addr0 * 4;

	if (b) {
		if (use_psm) {
			bcm_bprintf(b, "PSM (GpioMon) ");
		} else {
			bcm_bprintf(b, "TxeTpl ");
		}
		bcm_bprintf(b, "Capture mode: maccontrol1 0x%02x scctl 0x%02x\n",
			gpio_sel, scpctl);
		bcm_bprintf(b, "Start/stop/cur 0x%04x 0x%04x 0x%04x byt_offset 0x%05x entries %u\n",
			addr0, addr1, curptr, 4 * (curptr - addr0), len>>2);
		bcm_bprintf(b, "offset: low high\n");
	} else {
		if (use_psm) {
			printf("PSM (GpioMon) ");
		} else {
			printf("TxeTpl ");
		}
		printf("Capture mode: maccontrol1 0x%02x scpctl 0x%02x\n", gpio_sel, scpctl);
		printf("Start/stop/cur 0x%04x 0x%04x 0x%04x byt_offset 0x%05x entries %u\n",
		       addr0, addr1, curptr, 4 * (curptr - addr0), len>>2);
		printf("offset: low high\n");
	}

	wlc_bmac_suspend_mac_and_wait(wlc->hw);
	W_REG(osh, memptr_reg, offset);

	for (i = 0; i < (uint)len; i += 4) {
		uint32 tpldata;
		uint16 low16, hi16;

		tpldata = R_REG(osh, memdata_reg);
		hi16 = (tpldata >> 16) & 0xffff;
		low16 = tpldata & 0xffff;
		if (b)
			bcm_bprintf(b, "%05X: %04X %04X\n", i, low16, hi16);
		else
			printf("%05X: %04X %04X\n", i, low16, hi16);
	}

	wlc_bmac_enable_mac(wlc->hw);

	return BCME_OK;
}

/** dump beacon (from read_txe_ram in d11procs.tcl) */
static void
wlc_dump_bcntpl(wlc_info_t *wlc, struct bcmstrbuf *b, int offset, int len)
{
	osl_t *osh = wlc->osh;
	uint i;

	len = (len + 3) & ~3;
	wlc_bmac_suspend_mac_and_wait(wlc->hw);
	W_REG(osh, D11_XMT_TEMPLATE_RW_PTR(wlc), offset);
	bcm_bprintf(b, "tpl: offset %d len %d\n", offset, len);
	for (i = 0; i < (uint)len; i += 4) {
		bcm_bprintf(b, "%04X: %08X\n", i,
			R_REG(osh, D11_XMT_TEMPLATE_RW_DATA(wlc)));
	}

	wlc_bmac_enable_mac(wlc->hw);
}

static int
wlc_dump_bcntpls(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	uint16 len;
#if defined(MBSS)
	uint32 i;
	wlc_bsscfg_t *bsscfg;
	int tlen = wlc->pub->bcn_tmpl_len;
#endif

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

#if defined(MBSS)
	if (MBSS_ENAB(wlc->pub)) {
		FOREACH_BSS(wlc, i, bsscfg) {
			if (bsscfg->up && (BSSCFG_AP(bsscfg) || BSSCFG_IBSS(bsscfg))) {
				int8 bss_idx = wlc_mbss_bss_idx(wlc, bsscfg);

				len = wlc_read_shm(wlc, SHM_MBSS_BCNLEN0(wlc) + (2 * bss_idx));
				bcm_bprintf(b, "bcn %d: len %u bss_idx %d\n", i, len, bss_idx);
				wlc_dump_bcntpl(wlc, b,
					D11AC_T_BCN0_TPL_BASE + (tlen * bss_idx), len);
			}
		}
	} else {
#endif
	len = wlc_read_shm(wlc, M_BCN0_FRM_BYTESZ(wlc));
	bcm_bprintf(b, "bcn 0: len %u\n", len);
	wlc_dump_bcntpl(wlc, b, D11AC_T_BCN0_TPL_BASE, len);
	len = wlc_read_shm(wlc, M_BCN1_FRM_BYTESZ(wlc));
	bcm_bprintf(b, "bcn 1: len %u\n", len);
	wlc_dump_bcntpl(wlc, b, D11AC_T_BCN1_TPL_BASE, len);

#if defined(MBSS)
	}
#endif

	return 0;
}

static int
wlc_dump_pio(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int i;

	if (!wlc->clk)
		return BCME_NOCLK;

	if (!PIO_ENAB(wlc->pub))
		return 0;

	for (i = 0; i < NFIFO_LEGACY; i++) {
		pio_t *pio = WLC_HW_PIO(wlc, i);
		bcm_bprintf(b, "PIO %d: ", i);
		if (pio != NULL)
			wlc_pio_dump(pio, b);
		bcm_bprintf(b, "\n");
	}

	return 0;
}

#ifdef WL_PSMX
static int
wlc_dump_macx(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int cnt = 0;

	if (D11REV_LT(wlc->pub->corerev, 64)) {
		WL_ERROR(("wl%d: %s only supported for corerev >= 64\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	ASSERT(wlc->macdbg->pd11regs_x != NULL && wlc->macdbg->d11regsx_sz > 0);
	cnt = wlc_pd11regs_bylist(wlc, wlc->macdbg->pd11regs_x,
		wlc->macdbg->d11regsx_sz, 0, b, NULL);

	return (cnt > 0 ? BCME_OK : BCME_ERROR);
}

static int
wlc_dump_shmemx(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	uint i, k = 0;
	uint16 val, addr, end;

	static const shmem_list_t shmem_list[] = {
		{0, 4*1024},
	};

	if (D11REV_LT(wlc->pub->corerev, 64)) {
		WL_ERROR(("wl%d: %s only supported for corerev >= 64\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	for (i = 0; i < ARRAYSIZE(shmem_list); i++) {
		end = shmem_list[i].start + 2 * shmem_list[i].cnt;
		for (addr = shmem_list[i].start; addr < end; addr += 2) {
			val = wlc_read_shmx(wlc, addr);
			if (b) {
				bcm_bprintf(b, "%d shmx 0x%03x = 0x%04x\n",
					k++, addr, val);
			} else {
				printf("%d shmx 0x%03x = 0x%04x\n",
					k++, addr, val);
			}
		}
	}

	return 0;
}
#endif /* WL_PSMX */

#ifdef STA
static int
wlc_dump_trig_tpc_params(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	uint offset = M_PPR_BLK(wlc);
	uint32 cmninfo[2];
	uint usrinfo[3];
	int i;

	if (!wlc->clk) {
		WL_ERROR(("wl%d: %s: clock must be on\n", wlc->pub->unit, __FUNCTION__));
		return BCME_NOCLK;
	}

	bcm_bprintf(b, "PATHLOSS: %d dB APTX: %d TRGRSSI: %d %d %d %d TRSSI: %d "
		"TXPWR: %d MAXPWR: %d [dBm]\n",
		wlc_bmac_read_shm(wlc->hw, offset+6),
		wlc_bmac_read_shm(wlc->hw, offset+4),
		(int8) (wlc_bmac_read_shm(wlc->hw, offset) & (0xff)),
		(int8) ((wlc_bmac_read_shm(wlc->hw, offset) & (0xff00)) >> 8),
		(int8) (wlc_bmac_read_shm(wlc->hw, offset+2) & (0xff)),
		(int8) ((wlc_bmac_read_shm(wlc->hw, offset+2) & (0xff00)) >> 8),
		(int8) wlc_bmac_read_shm(wlc->hw, offset+8),
		(wlc_bmac_read_shm(wlc->hw, M_MAX_TXPWR(wlc))-
		wlc_bmac_read_shm(wlc->hw, offset+10)) >> 2,
		(wlc_bmac_read_shm(wlc->hw, M_MAX_TXPWR(wlc))-
		wlc_bmac_read_shm(wlc->hw, offset+12)) >> 2);

	offset = M_RXTRIG_CMNINFO(wlc);
	for (i = 0; i < 2; i++) {
		cmninfo[i] = ((wlc_bmac_read_shm(wlc->hw, offset+4*i) & 0xffff) |
			(wlc_bmac_read_shm(wlc->hw, (offset+4*i+2)) << 16));
	}

	bcm_bprintf(b, "Trig Common Info: %08x | %08x TYPE: %d LSIG: %d BW: %d\n",
		cmninfo[0], cmninfo[1], D11TRIGCI_TYPE(cmninfo[0]),
		D11TRIGCI_LSIG(cmninfo[0]), D11TRIGCI_BW(cmninfo[0]));

	offset = M_RXTRIG_USRINFO(wlc);
	for (i = 0; i < 3; i++) {
		usrinfo[i] = wlc_bmac_read_shm(wlc->hw, offset+2*i);
	}

	bcm_bprintf(b, "Trig User Info: %04x | %04x | %04x AID: %d "
		"RU: %d MCS: %d NSTS: %d\n",
		usrinfo[0], usrinfo[1], usrinfo[2], (usrinfo[0] & TRIG_AID_MASK),
		((usrinfo[0] & TRIG_RU0_MASK) >> TRIG_RU0_SHIFT) |
		((usrinfo[1] & TRIG_RU1_MASK) << TRIG_RU1_SHIFT),
		(usrinfo[1] & TRIG_MCS_MASK) >> TRIG_MCS_SHIFT, usrinfo[1] >> TRIG_NSTS_SHIFT);
	return 0;
}
#endif /* STA */
#endif /* WL_MACDBG */

#if defined(BCMDBG) || defined(BCMDBG_TXSTALL)
#define DUMP_DMA_TYPE_RX	(1 << 0)
#define DUMP_DMA_TYPE_TX	(1 << 1)
#define DUMP_DMA_TYPE_AQM	(1 << 2)
#ifdef BCMHWA
#define DUMP_DMA_TYPE_HWA	(1 << 3)
#define DUMP_DMA_TYPE_ALL	(DUMP_DMA_TYPE_RX | DUMP_DMA_TYPE_TX | DUMP_DMA_TYPE_AQM | \
				DUMP_DMA_TYPE_HWA)
#else
#define DUMP_DMA_TYPE_ALL	(DUMP_DMA_TYPE_RX | DUMP_DMA_TYPE_TX | DUMP_DMA_TYPE_AQM)
#endif
#define DUMP_DMA_FIFO_MAX	128
#define DUMP_DMA_ARGV_MAX	128

/**
 * Format:  wl dump dma [-t <all tx rx aqm hwa> -f <all 0 1 2 3...> -r -d]
 * Example: wl dump dma [-t tx -f all]
 */
static int
wlc_dump_dma_parse_args(wlc_info_t *wlc, uint32 *type, uint8 *fifo, bool *ring, bool *debug)
{
	int i, err = BCME_OK;
	char *args = wlc->dump_args;
	char *p, **argv = NULL;
	uint argc = 0;
	char opt, curr = '\0';
	uint32 val32;

	if (args == NULL || type == NULL || fifo == NULL || ring == NULL) {
		err = BCME_BADARG;
		goto exit;
	}

	/* allocate argv */
	if ((argv = MALLOC(wlc->osh, sizeof(*argv) * DUMP_DMA_ARGV_MAX)) == NULL) {
		WL_ERROR(("wl%d: %s: failed to allocate the argv buffer\n",
		          wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

	/* get each token */
	p = bcmstrtok(&args, " ", 0);
	while (p && argc < DUMP_DMA_ARGV_MAX-1) {
		argv[argc++] = p;
		p = bcmstrtok(&args, " ", 0);
	}
	argv[argc] = NULL;

	/* initial default */
	*type = 0;
	*ring = FALSE;
	for (i = 0; i < DUMP_DMA_FIFO_MAX; i++) {
		clrbit(fifo, i);
	}

	/* parse argv */
	argc = 0;
	while ((p = argv[argc++])) {
		if (!strncmp(p, "-", 1)) {
			if (strlen(p) > 2) {
				err = BCME_BADARG;
				goto exit;
			}
			opt = p[1];

			switch (opt) {
				case 't':
					curr = 't';
					break;
				case 'f':
					curr = 'f';
					break;
				case 'r':
					curr = 'r';
					*ring = TRUE;
					break;
				case 'd':
					curr = 'd';
					*debug = TRUE;
					break;
				default:
					err = BCME_BADARG;
					goto exit;
			}
		} else {
			switch (curr) {
				case 't':
					if (!strcmp(p, "all")) {
						*type = DUMP_DMA_TYPE_ALL;
					} else if (!strcmp(p, "rx")) {
						*type |= DUMP_DMA_TYPE_RX;
					} else if (!strcmp(p, "tx")) {
						*type |= DUMP_DMA_TYPE_TX;
					} else if (!strcmp(p, "aqm")) {
						*type |= DUMP_DMA_TYPE_AQM;
					}
#ifdef BCMHWA
					else if (!strcmp(p, "hwa")) {
						*type |= DUMP_DMA_TYPE_HWA;
					}
#endif
					break;
				case 'f':
					if (strcmp(p, "all") == 0) {
						for (i = 0; i < DUMP_DMA_FIFO_MAX; i++) {
							setbit(fifo, i);
						}
					}
					else if (strcmp(p, "txpend") == 0) {
						for (i = 0; i < WLC_HW_NFIFO_INUSE(wlc); i++) {
							if (TXPKTPENDGET(wlc, i) > 0) {
								setbit(fifo, i);
							}
						}
					}
					else {
						val32 = (uint32)bcm_strtoul(p, NULL, 0);
						if (val32 < DUMP_DMA_FIFO_MAX) {
							setbit(fifo, val32);
						}
					}
					break;
				default:
					err = BCME_BADARG;
					goto exit;
			}
		}
	}

exit:
	if (argv) {
		MFREE(wlc->osh, argv, sizeof(*argv) * DUMP_DMA_ARGV_MAX);
	}

	return err;
}

/** Dumps the state of all DMA channels of the d11 core */
static int
wlc_dump_dma(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	uint i;
	wl_cnt_wlc_t *cnt = wlc->pub->_cnt;
	int err = BCME_OK;
	uint32 type = DUMP_DMA_TYPE_ALL;
	uint8 fifo[(DUMP_DMA_FIFO_MAX/NBBY)+1];
	bool ring = TRUE;
	bool debug = FALSE;

	if (!wlc->clk)
		return BCME_NOCLK;

	/* Set default value for legacy dump */
	memset(fifo, 0, sizeof(fifo));
	for (i = 0; i < NFIFO_LEGACY; i++) {
		setbit(fifo, i);
	}

	if (bcm_bprintf_bypass) {
		/* Don't dump the DMA rings for internal dump */
		ring = FALSE;

		/* Add mu fifos */
		if (D11REV_GE(wlc->pub->corerev, 128)) {
			/* Get inuse mu fifos */
			(void)wlc_fifo_inuse(wlc->fifo, fifo);
		}
	}

	/* Parse args if needed */
	if (wlc->dump_args) {
		err = wlc_dump_dma_parse_args(wlc, &type, fifo, &ring, &debug);
		if (err != BCME_OK) {
			return err;
		}
	}

	/* dump suspend/flush control/status registers */
#ifdef BCM_DMA_CT
	if (BCM_DMA_CT_ENAB(wlc)) {
		wlc_hw_info_t *wlc_hw = wlc->hw;

		bcm_bprintf(b, "SUSPREQ        %x SUSPREQ1        %x SUSPREQ2        %x\n",
			R_REG(wlc->osh, D11_SUSPREQ(wlc_hw)),
			R_REG(wlc->osh, D11_SUSPREQ1(wlc_hw)),
			R_REG(wlc->osh, D11_SUSPREQ2(wlc_hw)));
		bcm_bprintf(b, "CHNSUSP_STATUS %x CHNSUSP_STATUS1 %x CHNSUSP_STATUS2 %x\n",
			R_REG(wlc->osh, D11_CHNSUSP_STATUS(wlc_hw)),
			R_REG(wlc->osh, D11_CHNSUSP_STATUS1(wlc_hw)),
			R_REG(wlc->osh, D11_CHNSUSP_STATUS2(wlc_hw)));
		bcm_bprintf(b, "FLUSHREQ        %x FLUSHREQ1        %x FLUSHREQ2        %x\n",
			R_REG(wlc->osh, D11_FLUSHREQ(wlc_hw)),
			R_REG(wlc->osh, D11_FLUSHREQ1(wlc_hw)),
			R_REG(wlc->osh, D11_FLUSHREQ2(wlc_hw)));
		bcm_bprintf(b, "CHNFLUSH_STATUS %x CHNFLUSH_STATUS1 %x CHNFLUSH_STATUS2 %x\n",
			R_REG(wlc->osh, D11_CHNFLUSH_STATUS(wlc_hw)),
			R_REG(wlc->osh, D11_CHNFLUSH_STATUS1(wlc_hw)),
			R_REG(wlc->osh, D11_CHNFLUSH_STATUS2(wlc_hw)));
	}
#endif /* BCM_DMA_CT */

	/* Start dump */
	for (i = 0; i < WLC_HW_NFIFO_TOTAL(wlc); i++) {
		if ((D11REV_LT(wlc->pub->corerev, 128) && (i == 6 || i == 7)) ||
			i >= DUMP_DMA_FIFO_MAX || isclr(fifo, i)) {
			continue;
		}

		if (i < NFIFO_LEGACY) {
			PRREG_INDEX(intstatus, (D11Reggrp_intctrlregs(wlc, i)->intstatus));
			PRREG_INDEX(intmask, (D11Reggrp_intctrlregs(wlc, i)->intmask));
		}
#ifdef BCM_DMA_CT
		if (BCM_DMA_CT_ENAB(wlc) && WLC_HW_AQM_DI(wlc, i)) {
			/* Set override to TRUE since HWA may update it */
			dma_set_indqsel(WLC_HW_AQM_DI(wlc, i), TRUE);
			PRREG_INDEX(aqm_intstatus, (D11Reggrp_indaqm(wlc, 0)->indintstatus));
			PRREG_INDEX(aqm_intmask, (D11Reggrp_indaqm(wlc, 0)->indintmask));
		}
#endif
		PRNL();

		if (PIO_ENAB(wlc->pub)) {
			continue;
		}

		if (type & DUMP_DMA_TYPE_ALL) {
			hnddma_t *di = WLC_HW_DI(wlc, i);

			if (di) {
				bcm_bprintf(b, "DMA %d:\n", i);
				if (type & DUMP_DMA_TYPE_RX) {
					if (wlc->dump_args == NULL) {
						if (i < MAX_RX_FIFO) {
							bcm_bprintf(b, "  RX%d: ", i);
							dma_dumprx(di, b, ring);
							PRVAL(rxuflo[i]);
							PRNL();
						}
					} else {
						bcm_bprintf(b, "  RX%d: ", i);
						dma_dumprx(di, b, ring);
						PRVAL(rxuflo[i]);
						PRNL();
					}
				}

				if (type & DUMP_DMA_TYPE_TX) {
					bcm_bprintf(b, "  TX%d(p%u): ",
						i, WLC_HW_MAP_TXFIFO(wlc, i));
					dma_dumptx(di, b, ring); // dumps DMA for a single FIFO
					PRVAL(txuflo);
					PRNL();
				}
			}
#ifdef BCM_DMA_CT
			if (BCM_DMA_CT_ENAB(wlc) && (type & DUMP_DMA_TYPE_AQM)) {
				di = WLC_HW_AQM_DI(wlc, i);
				if (di) {
					bcm_bprintf(b, " AQM%d: ", i);
					dma_dumptx(di, b, ring);
				}
			}
#endif /* BCM_DMA_CT */
#if defined(BCMHWA) && (defined(BCMDBG) || defined(HWA_DUMP))
			/* Show HWA Tx shadow info */
			if (type & DUMP_DMA_TYPE_HWA) {
				HWA_TXFIFO_EXPR({
					/* Accress fifoctx through AXI is dangerous */
					if (debug) {
						hwa_txfifo_dump_fifoctx(hwa_dev, b,
							WLC_HW_MAP_TXFIFO(wlc, i));
					}
					hwa_txfifo_dump_ovfq(hwa_dev, b,
						WLC_HW_MAP_TXFIFO(wlc, i));
					hwa_txfifo_dump_shadow(hwa_dev, b,
						WLC_HW_MAP_TXFIFO(wlc, i));
				});
			}
#endif
			BCM_REFERENCE(debug);
			PRNL();
		}
	}

#if defined(BCMHWA) && defined(BCMHME)
	dma_common_hme_dump(wlc->hw->dmacommon, b);
#endif

	PRVAL(dmada); PRVAL(dmade); PRVAL(rxoflo); PRVAL(dmape);
	PRNL();

	return err;
} /* wlc_dump_dma */
#endif

#if defined(BCMDBG)
uint8 *
wlc_macdbg_get_dmadump_buf(wlc_info_t *wlc)
{
	wlc_macdbg_info_t *macdbg = wlc->macdbg;

	/* allocate new buffer if never allocated */
	if (macdbg->dmadump_buf == NULL) {
		macdbg->dmadump_buf = MALLOCZ(wlc->osh, DMA_DUMP_LEN);

		/* assert if allocation failed */
		ASSERT(macdbg->dmadump_buf);
	}

	return macdbg->dmadump_buf;
}

/** To be invoked from driver internally without allocating the string buffer */
static void
wlc_macdbg_dump_dma(wlc_info_t *wlc)
{
	struct bcmstrbuf *b;
	char	*buf = NULL;
	struct  bcmstrbuf bstr;
	osl_t *osh = wlc->osh;
	int i;

	/* dump xmt dma registers for all DMA engines to debug DMA related issues */
	if (D11REV_GE(wlc->pub->corerev, 129)) {
		WL_PRINT(("(idx): XMTDMA_DBG_CTL DMATX_STS ENGINE_STS\n"));
		for (i = 0; i < D11_MAX_DMATX_ENGINES(wlc->pub->corerev); i++) {
			W_REG(osh, D11_TXE_XMTDMA_DBG_CTL(wlc),
				(uint16)D11_TXE_XMTDMA_DBG_CTL_VAL(7, i));
			WL_PRINT(("(%02d): 0x%04x 0x%04x 0x%04x\n", i,
				R_REG(osh, D11_TXE_XMTDMA_DBG_CTL(wlc)),
				R_REG(osh, D11_XMTDMA_DMATX_STS(wlc)),
				R_REG(osh, D11_XMTDMA_ENGINE_STS(wlc))));
		}
	}

	buf = MALLOCZ(osh, MACDBG_DUMP_LEN);
	if (buf) {
		bcm_binit(&bstr, buf, MACDBG_DUMP_LEN);
		b = &bstr;
		bcm_bprintf_bypass = TRUE;
		wlc_dump_dma(wlc, b);
		bcm_bprintf_bypass = FALSE;

		MFREE(osh, buf, MACDBG_DUMP_LEN);
	}

#ifndef DONGLEBUILD
	if (D11REV_IS(wlc->pub->corerev, 131) && wlc->psm_watchdog_debug == TRUE) {
		buf = wlc_macdbg_get_dmadump_buf(wlc);

		if (buf) {
			memset((void *)buf, 0, DMA_DUMP_LEN);
			bcm_binit(&bstr, buf, DMA_DUMP_LEN);
			b = &bstr;

			/* Don't need to free buff after use. It's saved in macdbg->dmadump_buf */
			wlc_dump_dma(wlc, b);
		}
	}
#endif /* DONGLEBUILD */
}

#ifdef WLCNT
#define PRCNT_MACSTAT_TX							\
do {										\
	/* UCODE SHM counters */						\
	/* tx start and those that do not end well */					\
	PRVAL(txallfrm); PRVAL(txbcnfrm); PRVAL(txrtsfrm); PRVAL(txctsfrm);	\
	PRVAL(txackfrm); PRVAL(txback); PRVAL(txdnlfrm); PRNL();		\
	PRVAL(txampdu); PRVAL(txmpdu); PRVAL(txucast);				\
	PRVAL(rxrsptmout); PRVAL(txinrtstxop); PRVAL(txrtsfail); PRNL();	\
	bcm_bprintf(b, "txper_ucastdt %d%% txper_rts %d%%\n",			\
		cnt->txucast > 0 ?						\
		(100 - ((cnt->rxackucast + cnt->rxback) * 100			\
		 / cnt->txucast)) : 0,						\
		cnt->txrtsfrm > 0 ?						\
		(100 - (cnt->rxctsucast * 100 / cnt->txrtsfrm)) : 0);		\
	bcm_bprintf(b, "txfunfl: ");						\
	for (i = 0; i < NFIFO_LEGACY; i++)					\
		bcm_bprintf(b, "%d ", cnt->txfunfl[i]);				\
	PRVAL(txtplunfl); PRVAL(txphyerror); PRNL(); PRNL();			\
} while (0)

#define PRCNT_MACSTAT_RX							\
do {										\
	/* rx with goodfcs */							\
	PRVAL(rxctlucast); PRVAL(rxrtsucast); PRVAL(rxctsucast);		\
	PRVAL(rxackucast); PRVAL(rxback); PRNL();				\
	PRVAL(rxbeaconmbss); PRVAL(rxdtucastmbss);				\
	PRVAL(rxmgucastmbss); PRNL();						\
	PRVAL(rxbeaconobss); PRVAL(rxdtucastobss); PRVAL(rxdtocast);		\
	PRVAL(rxmgocast); PRNL();						\
	PRVAL(rxctlocast); PRVAL(rxrtsocast); PRVAL(rxctsocast); PRNL();	\
	PRVAL(rxctlmcast); PRVAL(rxdtmcast); PRVAL(rxmgmcast); PRNL(); PRNL();	\
										\
	PRVAL(rxcgprqfrm); PRVAL(rxcgprsqovfl); PRVAL(txcgprsfail);		\
	PRVAL(txcgprssuc); PRVAL(prs_timeout); PRNL();				\
	PRVAL(pktengrxducast); PRVAL(pktengrxdmcast);				\
	PRVAL(bcntxcancl);							\
} while (0)

static INLINE void
wlc_dump_macstat(wlc_info_t *wlc, struct bcmstrbuf * b)
{
	int i;
	if (D11REV_GE(wlc->pub->corerev, 40)) {
		wl_cnt_ge40mcst_v1_t *cnt = wlc->pub->_mcst_cnt;
		PRCNT_MACSTAT_TX;
		/* rx start and those that do not end well */
		PRVAL(rxstrt); PRVAL(rxbadplcp); PRVAL(rxcrsglitch);
		PRVAL(rxtoolate); PRVAL(rxnodelim); PRNL();
		PRVAL(rxdrop20s); PRVAL(bphy_badplcp); PRVAL(bphy_rxcrsglitch); PRNL();
		PRVAL(rxbadfcs); PRVAL(rxfrmtoolong); PRVAL(rxfrmtooshrt); PRVAL(rxanyerr); PRNL();
		PRVAL(rxf0ovfl); PRVAL(rxf1ovfl); PRVAL(rxhlovfl); PRVAL(pmqovfl); PRNL();
		PRCNT_MACSTAT_RX;
		PRVAL(missbcn_dbg); PRNL();
		PRNL();
	} else {
		wl_cnt_lt40mcst_v1_t *cnt = wlc->pub->_mcst_cnt;
		PRCNT_MACSTAT_TX;
		PRVAL(rxstrt); PRVAL(rxbadplcp); PRVAL(rxcrsglitch);
		PRVAL(rxtoolate); PRVAL(rxnodelim); PRNL();
		PRVAL(bphy_badplcp); PRVAL(bphy_rxcrsglitch); PRNL();
		PRVAL(rxbadfcs); PRVAL(rxfrmtoolong); PRVAL(rxfrmtooshrt); PRVAL(rxanyerr); PRNL();
		PRVAL(rxf0ovfl); PRVAL(pmqovfl); PRNL();
		PRCNT_MACSTAT_RX;
		PRNL();
		PRVAL(dbgoff46); PRVAL(dbgoff47);
		PRVAL(dbgoff48); PRVAL(phywatch);
		PRNL();
	}
}
#endif /* WLCNT */

/** print aggregate (since driver load time) driver and macstat counter values */
static int
wlc_dump_stats(wlc_info_t *wlc, struct bcmstrbuf * b)
{
#ifdef WLCNT
	int i;
	wl_cnt_wlc_t *cnt = wlc->pub->_cnt;

	if (WLC_UPDATE_STATS(wlc)) {
		wlc_statsupd(wlc);
	}

	/* print UCODE MACSTATs */
	wlc_dump_macstat(wlc, b);

	/* Display reinit counters */
	PRVAL(reinit);
	bcm_bprintf(b, "reinitreason_counts: ");
	for (i = 0; i < WL_REINIT_RC_LAST; i++) {
		if (wlc->pub->reinitrsn->rsn[i]) {
			bcm_bprintf(b, "%d(%d) ", i, wlc->pub->reinitrsn->rsn[i]);
		}
	}

	PRNL();
	PRVAL(reset); PRVAL(pciereset); PRVAL(cfgrestore); PRVAL(dma_hang); PRNL();

	/* summary stat counter line */
	PRVAL(txframe); PRVAL(txbyte); PRVAL(txretrans); PRVAL(txfail);
	PRVAL(txchanrej); PRVAL(tbtt); PRVAL(p2p_tbtt); PRVAL(p2p_tbtt_miss); PRNL();
	PRVAL(rxframe); PRVAL(rxbyte); PRVAL(rxerror); PRNL();
	PRVAL(txprshort); PRVAL(txdmawar); PRVAL(txnobuf); PRVAL(txnoassoc);
	PRVAL(txchit); PRVAL(txcmiss); PRNL();
	PRVAL(txserr); PRVAL(txphyerr); PRVAL(txphycrs); PRVAL(txerror); PRNL();

	PRVAL_RENAME(txfrag, d11_txfrag); PRVAL_RENAME(txmulti, d11_txmulti);
	PRVAL_RENAME(txretry, d11_txretry); PRVAL_RENAME(txretrie, d11_txretrie); PRNL();
	PRVAL_RENAME(txrts, d11_txrts); PRVAL_RENAME(txnocts, d11_txnocts);
	PRVAL_RENAME(txnoack, d11_txnoack); PRVAL_RENAME(txfrmsnt, d11_txfrmsnt); PRNL();

	PRVAL(rxcrc); PRVAL(rxnobuf); PRVAL(rxnondata); PRVAL(rxbadds);
	PRVAL(rxbadcm); PRVAL(rxdup); PRVAL(rxrtry); PRVAL(rxfragerr); PRNL();
	PRVAL(rxrunt); PRVAL(rxgiant); PRVAL(rxnoscb); PRVAL(rxbadproto);
	PRVAL(rxbadsrcmac); PRNL();

	PRVAL_RENAME(rxfrag, d11_rxfrag); PRVAL_RENAME(rxmulti, d11_rxmulti);
	PRVAL_RENAME(rxundec, d11_rxundec); PRVAL_RENAME(rxundec_mcst, d11_rxundec_mcst);
	PRNL();

	PRVAL(rxctl); PRVAL(rxbadda); PRVAL(rxfilter);
	bcm_bprintf(b, "rxuflo: ");
	for (i = 0; i < NFIFO_LEGACY; i++)
		bcm_bprintf(b, "%d ", cnt->rxuflo[i]);
	PRNL(); PRNL();

	/* WPA2 counters */
	PRVAL(tkipmicfaill); PRVAL(tkipicverr); PRVAL(tkipcntrmsr); PRNL();
	PRVAL(tkipreplay); PRVAL(ccmpfmterr); PRVAL(ccmpreplay); PRNL();
	PRVAL(ccmpundec); PRVAL(fourwayfail); PRVAL(wepundec); PRNL();
	PRVAL(wepicverr); PRVAL(decsuccess); PRVAL(rxundec); PRNL();

	PRVAL(tkipmicfaill_mcst); PRVAL(tkipicverr_mcst); PRVAL(tkipcntrmsr_mcst); PRNL();
	PRVAL(tkipreplay_mcst); PRVAL(ccmpfmterr_mcst); PRVAL(ccmpreplay_mcst); PRNL();
	PRVAL(ccmpundec_mcst); PRVAL(fourwayfail_mcst); PRVAL(wepundec_mcst); PRNL();
	PRVAL(wepicverr_mcst); PRVAL(decsuccess_mcst); PRVAL(rxundec_mcst); PRNL();
	PRNL();

	PRVAL(rx1mbps); PRVAL(rx2mbps); PRVAL(rx5mbps5); PRVAL(rx11mbps); PRNL();
	PRVAL(rx6mbps); PRVAL(rx9mbps); PRVAL(rx12mbps); PRVAL(rx18mbps); PRNL();
	PRVAL(rx24mbps); PRVAL(rx36mbps); PRVAL(rx48mbps); PRVAL(rx54mbps); PRNL();

	PRVAL(txmpdu_sgi); PRVAL(rxmpdu_sgi); PRVAL(txmpdu_stbc); PRVAL(rxmpdu_stbc);
	PRVAL(rxmpdu_mu); PRNL();

	PRVAL(cso_normal); PRVAL(cso_passthrough); PRNL();
	PRVAL(chained); PRVAL(chainedsz1); PRVAL(unchained);
	PRVAL(maxchainsz); PRVAL(currchainsz); PRNL();

	/* detailed amangement and control frame counters */
	PRVAL(txbar); PRVAL(txpspoll); PRVAL(rxbar);
	PRVAL(rxpspoll); PRNL();
	PRVAL(txnull); PRVAL(txqosnull); PRVAL(rxnull);
	PRVAL(rxqosnull); PRNL();
	PRVAL(txassocreq); PRVAL(txreassocreq); PRVAL(txdisassoc);
	PRVAL(txassocrsp); PRVAL(txreassocrsp); PRNL();
	PRVAL(txauth); PRVAL(txdeauth); PRVAL(txprobereq);
	PRVAL(txprobersp); PRVAL(txaction); PRNL();
	PRVAL(rxassocreq); PRVAL(rxreassocreq); PRVAL(rxdisassoc);
	PRVAL(rxassocrsp); PRVAL(rxreassocrsp); PRNL();
	PRVAL(rxauth); PRVAL(rxdeauth); PRVAL(rxprobereq);
	PRVAL(rxprobersp); PRVAL(rxaction); PRNL();
#endif /* WLCNT */

	return BCME_OK;
} /* wlc_dump_stats */

/** print extended macstat counter values */

static int
wlc_macdbg_smpl_capture_optns(wlc_info_t *wlc, wl_maccapture_params_t *params, bool set)
{
	uint16 opt_bmp = params->optn_bmp;
	//uint16 smpl_ctrl = 0, d11_regvalue = 0;
	volatile uint16 *smpl_ctrl_reg, *smpl_ctrl2_reg;
	volatile uint16 *sct_mask_reg_h, *sct_mask_reg_l, *sct_val_reg_h, *sct_val_reg_l;
	volatile uint16 *scs_mask_reg_h, *scs_mask_reg_l;
	volatile uint16 *scx_mask_reg_h, *scx_mask_reg_l;
	volatile uint16 *scm_mask_reg_h, *scm_mask_reg_l, *scm_val_reg_h, *scm_val_reg_l;
	int smpl_capture_rev;
	int optn_shft = 0;
	bool use_psm = FALSE;
	bool use_strig = FALSE;

	uint16 smpl_ctrl_en[SMPL_CAPTURE_NUM][SC_NUM_OPTNS_GE50] = {
		{SC_TRIG_EN, SC_STORE_EN, SC_TRANS_EN, SC_MATCH_EN, SC_STOP_EN},
		{PHYCTL_SC_TRIG_EN, PHYCTL_SC_STR_EN, PHYCTL_SC_TRANS_EN,
		SC_OPTN_LT50NA, SC_OPTN_LT50NA}};

	sct_mask_reg_h = sct_mask_reg_l = sct_val_reg_h = sct_val_reg_l = NULL;
	scs_mask_reg_h = scs_mask_reg_l = scx_mask_reg_h = scx_mask_reg_l = NULL;
	scm_mask_reg_h = scm_mask_reg_l = scm_val_reg_h = scm_val_reg_l = NULL;

	use_psm = (WL_MACDBG_IS_USEPSM(params) && D11REV_GE(wlc->pub->corerev, 129));
	use_strig = (WL_MACDBG_IS_STOPTRIGGER(params) && D11REV_GE(wlc->pub->corerev, 129));

	opt_bmp &= ((1 << WL_MACCAPT_TRIG) | (1 << WL_MACCAPT_STORE) |
	(1 << WL_MACCAPT_TRANS) | (1 << WL_MACCAPT_MATCH) |
	(1 << WL_MACCAPT_STOPMODE));

	/* Core Specific Inits */
	if (D11REV_GE(wlc->pub->corerev, 56)) {
		smpl_capture_rev = SMPL_CAPTURE_GE50;
		smpl_ctrl_reg = use_psm ? D11_PSM_SMP_CTRL(wlc) : D11_SMP_CTRL(wlc);
		smpl_ctrl2_reg = use_psm ? D11_PSM_SMP_CTRL2(wlc) : D11_SMP_CTRL2(wlc);
	} else {
		ASSERT(!(opt_bmp & ((1 << WL_MACCAPT_TRANS) | (1 << WL_MACCAPT_MATCH))));
		smpl_capture_rev = SMPL_CAPTURE_LT50;
		smpl_ctrl_reg = D11_PSM_PHY_CTL(wlc);
		smpl_ctrl2_reg = D11_PSM_PHY_CTL(wlc);
	}

	if (D11REV_GE(wlc->pub->corerev, 56)) {
		sct_mask_reg_l = use_strig ?
		(use_psm ? D11_PSM_SCTSTP_MASK_L(wlc) : D11_SCTSTP_MASK_L(wlc)) :
		(use_psm ? D11_PSM_SCT_MASK_L(wlc) : D11_SCT_MASK_L(wlc));
		sct_mask_reg_h = use_strig ?
		(use_psm ? D11_PSM_SCTSTP_MASK_H(wlc) : D11_SCTSTP_MASK_H(wlc)) :
		(use_psm ? D11_PSM_SCT_MASK_H(wlc) : D11_SCT_MASK_H(wlc));
		sct_val_reg_h = use_strig ?
		(use_psm ? D11_PSM_SCTSTP_VAL_H(wlc) : D11_SCTSTP_VAL_H(wlc)) :
		(use_psm ? D11_PSM_SCT_VAL_H(wlc) : D11_SCT_VAL_H(wlc));
		sct_val_reg_l = use_strig ?
		(use_psm ? D11_PSM_SCTSTP_VAL_L(wlc) : D11_SCTSTP_VAL_L(wlc)) :
		(use_psm ? D11_PSM_SCT_VAL_L(wlc) : D11_SCT_VAL_L(wlc));
		scs_mask_reg_l = use_psm ? D11_PSM_SCS_MASK_L(wlc) : D11_SCS_MASK_L(wlc);
		scs_mask_reg_h = use_psm ? D11_PSM_SCS_MASK_H(wlc) : D11_SCS_MASK_H(wlc);
		scx_mask_reg_l = use_psm ? D11_PSM_SCX_MASK_L(wlc) : D11_SCX_MASK_L(wlc);
		scx_mask_reg_h = use_psm ? D11_PSM_SCX_MASK_H(wlc) : D11_SCX_MASK_H(wlc);
		scm_mask_reg_l = use_psm ? D11_PSM_SCM_MASK_L(wlc) : D11_SCM_MASK_L(wlc);
		scm_mask_reg_h = use_psm ? D11_PSM_SCM_MASK_H(wlc) : D11_SCM_MASK_H(wlc);
		scm_val_reg_l = use_psm ? D11_PSM_SCM_VAL_L(wlc): D11_SCM_VAL_L(wlc);
		scm_val_reg_h = use_psm ? D11_PSM_SCM_VAL_H(wlc): D11_SCM_VAL_H(wlc);
	}

	if (set) {
		if (D11REV_GE(wlc->pub->corerev, 56)) {
			MACDBG_SET_MACCPTR_PREG(sct_mask_reg, params->tr_mask);
			MACDBG_SET_MACCPTR_PREG(sct_val_reg, params->tr_val);
			MACDBG_SET_MACCPTR_PREG(scs_mask_reg, params->s_mask);
			MACDBG_SET_MACCPTR_PREG(scx_mask_reg, params->x_mask);
			MACDBG_SET_MACCPTR_PREG(scm_mask_reg, params->m_mask);
			MACDBG_SET_MACCPTR_PREG(scm_val_reg, params->m_val);
			if (D11REV_GE(wlc->pub->corerev, 129)) {
				MACDBG_SET_MACPTR_CFG(smpl_ctrl2_reg,
				(params->optn_bmp >> WL_MACCAPT_LOGMODE) & 0x3f);
			}

			if ((D11REV_IS(wlc->pub->corerev, 132) ||
			(D11REV_IS(wlc->pub->corerev, 130) &&
			(wlc->pub->corerev_minor == 2))) && !use_strig && use_psm) {
				/* WAR for CRBCAD11MAC-5947: Need to set stop trigger value
				 * to be non-zero even if it is not using stop trigger mode.
				 * It applies to GPIOMON (use_psm) case only
				 * XXX temporary extend to 130.2
				 */
				volatile uint16 *sctstp_val_reg_h, *sctstp_val_reg_l;
				sctstp_val_reg_h = D11_PSM_SCTSTP_VAL_H(wlc);
				sctstp_val_reg_l = D11_PSM_SCTSTP_VAL_L(wlc);
				MACDBG_SET_MACCPTR_PREG(sctstp_val_reg, 0xffff);
			}
		} else {
			//corerev < 50
			W_REG(wlc->osh, D11_DebugTriggerMask(wlc), params->tr_mask);
			W_REG(wlc->osh, D11_DebugTriggerValue(wlc), params->tr_val);
			W_REG(wlc->osh, D11_DebugStoreMask(wlc), params->s_mask);
		}

		//common part
		while (opt_bmp && optn_shft < SC_NUM_OPTNS_GE50) {
			if (opt_bmp & 1) {
				params->smpl_ctrl |= (smpl_ctrl_en[smpl_capture_rev][optn_shft]);
			}
			optn_shft++;
			opt_bmp >>= 1;
		}

		if (D11REV_GE(wlc->pub->corerev, 129)) {
			if (use_strig) {
				params->smpl_ctrl &= ~SC_TRIG_EN;
				params->smpl_ctrl |= SC_STRIG_EN;
			}
		}
	} else {
		params->smpl_ctrl = MACDBG_MACCPTR_RREG(smpl_ctrl_reg, params->psmsel_bmp);
		if (D11REV_GE(wlc->pub->corerev, 56)) {
			MACDBG_GET_MACCPTR_PREG(sct_mask_reg, params->tr_mask, params->psmsel_bmp);
			MACDBG_GET_MACCPTR_PREG(sct_val_reg, params->tr_val, params->psmsel_bmp);
			MACDBG_GET_MACCPTR_PREG(scs_mask_reg, params->s_mask, params->psmsel_bmp);
			MACDBG_GET_MACCPTR_PREG(scx_mask_reg, params->x_mask, params->psmsel_bmp);
			MACDBG_GET_MACCPTR_PREG(scm_mask_reg, params->m_mask, params->psmsel_bmp);
			MACDBG_GET_MACCPTR_PREG(scm_val_reg, params->m_val, params->psmsel_bmp);
			if (D11REV_GE(wlc->pub->corerev, 129)) {
				params->optn_bmp |=
				(MACDBG_MACCPTR_RREG(smpl_ctrl2_reg, params->psmsel_bmp) & 0x3f)
					<< WL_MACCAPT_LOGMODE;
			}
		} else {
			// corerev < 50
			params->tr_mask = R_REG(wlc->osh, D11_DebugTriggerMask(wlc));
			params->tr_val = R_REG(wlc->osh, D11_DebugTriggerValue(wlc));
			params->s_mask = R_REG(wlc->osh, D11_DebugStoreMask(wlc));
		}
	}

	/* Enable the SC Options */
	if (params->smpl_ctrl && set) {
		//d11_regvalue = R_REG(wlc->osh, smpl_ctrl_reg);
		W_REG(wlc->osh, smpl_ctrl_reg, (uint16)(params->smpl_ctrl
			| (params->psmsel_bmp << WL_MACCAP_SEL_PSMR0_SHIFT)));
		if (use_psm) {
			if (PSMX_ENAB(wlc->pub))
				MACDBG_WRITE_MACREGX(smpl_ctrl_reg, params->smpl_ctrl |
				((params->psmsel_bmp << WL_MACCAP_SEL_PSMX_SHIFT) & SC_ENABLE));
			if (PSMR1_ENAB(wlc->hw))
				MACDBG_WRITE_MACREGR1(smpl_ctrl_reg, params->smpl_ctrl |
				((params->psmsel_bmp << WL_MACCAP_SEL_PSMR1_SHIFT) & SC_ENABLE));
		}
	}

	return BCME_OK;
} /* wlc_macdbg_smpl_capture_optns */

/* wlc_macdbg_smpl_capture_set initializes MAC sample capture.
 * If UP, also starts the sample capture.
 * Otherwise, sample capture will be started upon wl up
 */
static int
wlc_macdbg_smpl_capture_set(wlc_info_t *wlc, wl_maccapture_params_t *params)
{
	wlc_macdbg_info_t *macdbg = wlc->macdbg;
	wl_maccapture_params_t *cur_params = (wl_maccapture_params_t *)macdbg->smpl_info;
	uint32 d11_regvalue = 0;
	int err = BCME_OK;
	uint16 tpl_size = wlc->hw->tpl_sz;
	bool use_psm = FALSE;
	bool use_tpl = FALSE;
	volatile uint16 *smpl_ctrl_reg, *strtptr_reg, *stopptr_reg, *curptr_reg;

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		WL_ERROR(("wl%d: %s only supported for corerev >=40\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	if (WL_MACDBG_IS_USEPSM(params) && D11REV_LT(wlc->pub->corerev, 129)) {
		WL_ERROR(("wl%d: %s PSM sample capture only supported corerev>=129\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	use_psm = (WL_MACDBG_IS_USEPSM(params) && D11REV_GE(wlc->pub->corerev, 129));
	use_tpl = WL_MACDBG_IS_USETXE(params);

	if (!use_psm && !use_tpl) {
		/* By default if neither usepsm nor usetxe is set, revid>=129 go with psm
		 * revid<129 go with txe
		 */
		if (D11REV_GE(wlc->pub->corerev, 129)) {
			params->optn_bmp |= (1 << WL_MACCAPT_USEPSM);
			params->optn_bmp &= ~(1 << WL_MACCAPT_USETXE);
			use_psm = TRUE;
		} else if (D11REV_GE(wlc->pub->corerev, 56)) {
			params->optn_bmp |= (1 << WL_MACCAPT_USETXE);
		}
	}

	smpl_ctrl_reg = use_psm ? D11_PSM_SMP_CTRL(wlc) : D11_SMP_CTRL(wlc);
	strtptr_reg = use_psm ? D11_PSM_SCP_STRTPTR(wlc) : D11_SCP_STRTPTR(wlc);
	stopptr_reg = use_psm ? D11_PSM_SCP_STOPPTR(wlc) : D11_SCP_STOPPTR(wlc);
	curptr_reg = use_psm ? D11_PSM_SCP_CURPTR(wlc) : D11_SCP_CURPTR(wlc);

	if (!wlc->clk) {
		memcpy((void *)cur_params, (void *)params, sizeof(*params));
		macdbg->smpl_ctrl |= SC_STRT; /* Set the start bit of the soft register */
		WL_ERROR(("%s: MAC Sample Capture params. saved. "
			"Will  start upon wl up\n", __FUNCTION__));
		return err;
	}

	if (params->cmd == WL_MACCAPT_STOP) {
		if (D11REV_GE(wlc->pub->corerev, 56)) {
			/* Clear Start bit */
			d11_regvalue = MACDBG_MACCPTR_RREG(smpl_ctrl_reg, params->psmsel_bmp);
			MACDBG_SET_MACPTR_CFG(smpl_ctrl_reg, d11_regvalue);
		} else {
			/* Clear start bit */
			AND_REG(wlc->osh, D11_PSM_PHY_CTL(wlc), (uint16)~PHYCTL_SC_STRT);
		}

		/* Clear the start bit of the soft register */
		macdbg->smpl_ctrl &= ~SC_STRT;
		return err;
	} else if (params->cmd == WL_MACCAPT_RST) {
		macdbg->smpl_ctrl = 0;
		d11_regvalue = MACDBG_MACCPTR_RREG(strtptr_reg, params->psmsel_bmp);
		MACDBG_SET_MACPTR_CFG(curptr_reg, d11_regvalue);

		/* W_REG(wlc->osh, D11_SCP_STRTPTR(wlc), 0); */
		/* W_REG(wlc->osh, D11_SCP_STOPPTR(wlc), 0); */
		/* Clear smp_ctrl */
		if (D11REV_GE(wlc->pub->corerev, 56)) {
			MACDBG_SET_MACPTR_CFG(smpl_ctrl_reg, macdbg->smpl_ctrl);
		} else {
			W_REG(wlc->osh, D11_PSM_PHY_CTL(wlc), (uint16)0);
		}

		return err;
	}

	if (params->la_mode) {
		if (D11REV_LT(wlc->pub->corerev, 56)) {
			/* Assign all GPIOs to other cores than ChipCommon */
			si_ccreg(wlc->pub->sih, CC_GPIOCTRL, ~0, 0xFFFFFFFF);

			/* Clear PHY Lo/Hi GPIO Out En Regs */
			phy_dbg_gpio_out_enab(WLC_PI(wlc), TRUE);
		}
		/* Enable GPIO based on mask */
		W_REG(wlc->osh, D11_PSM_GPIOEN(wlc), (uint16)params->s_mask);
	}

	/* Fill in default values */

	/* GPIO Output Selection */
	d11_regvalue = (params->gpio_sel << MCTL1_GPIOSEL_SHIFT)
			& MCTL1_GPIOSEL_MASK; /* GPIO_SEL is 6bits */
	d11_regvalue = (R_REG(wlc->osh, D11_MacControl1(wlc)) & ~MCTL1_GPIOSEL_MASK) | d11_regvalue;
	W_REG(wlc->osh, D11_MacControl1(wlc), d11_regvalue);

	/* Sample Collect Start & Stop Ptr */
	if (use_psm) {
		if (params->start_ptr == 0 && params->stop_ptr == 0) {
			params->start_ptr = WL_MACCAPT_PSM_STRT_PTR;
			params->stop_ptr = WL_MACCAPT_PSM_STOP_PTR;
			params->smp_sz = WL_MACCAPT_PSM_SIZE;
		}
	} else {
		if (params->start_ptr != 0 && params->stop_ptr == 0 && params->smp_sz != 0) {
			params->stop_ptr = params->start_ptr + params->smp_sz - 1;
		}

		if (params->start_ptr == 0 && params->stop_ptr == 0 && !WL_MACDBG_IS_EXPERT_MODE) {
			if (tpl_size < T_WOWL_BASE) {
				WL_ERROR(("%s: tpl size %d is smaller than T_WOWL_BASE %d\n",
				__FUNCTION__, tpl_size, T_WOWL_BASE));
			} else {
				/* start_ptr and stop ptr are in 4byte unit */
				params->start_ptr = T_WOWL_BASE >> 2;
				params->stop_ptr = (tpl_size >> 2) - 1;
			}
		}

		if ((params->smp_sz = params->stop_ptr - params->start_ptr + 1) <= 0) {
			WL_ERROR(("%s: smp_sz %d is < 0 start_ptr 0x%x stop_ptr 0x%x\n",
			__FUNCTION__, params->smp_sz, params->start_ptr, params->stop_ptr));
			if (!WL_MACDBG_IS_EXPERT_MODE) {
				return BCME_BADARG;
			}
		}

		if (params->smp_sz > (tpl_size >> 2)) {
			WL_ERROR(("%s: configured smp_sz %d > tpl_sz %d in 4-byte unit\n",
			__FUNCTION__, params->smp_sz, tpl_size >> 2));
			if (!WL_MACDBG_IS_EXPERT_MODE) {
				return BCME_BADARG;
			}
		}

		if (params->start_ptr < T_WOWL_BASE >> 2) {
			WL_ERROR(("%s: conf strt_ptr 0x%x > T_WOWL_BASE 0x%x in 4-byte addr\n",
			__FUNCTION__, params->start_ptr, T_WOWL_BASE >> 2));
			if (!WL_MACDBG_IS_EXPERT_MODE) {
				return BCME_BADARG;
			}
		}
	}

	MACDBG_SET_MACPTR_CFG(strtptr_reg, params->start_ptr);
	MACDBG_SET_MACPTR_CFG(stopptr_reg, params->stop_ptr);

	/* default values for trigger mask */
	if (params->tr_mask == 0 &&
	((params->optn_bmp & (1 << WL_MACCAPT_TRIG)) ||
	(params->optn_bmp & (1 << WL_MACCAPT_STOPTRIG))) &&
	!WL_MACDBG_IS_EXPERT_MODE) {
		if (D11REV_GE(wlc->pub->corerev, 128)) {
			params->tr_mask = WL_MACDBG_DFLT_TRIGMASK_GE128;
		} else if (D11REV_GE(wlc->pub->corerev, 56)) {
			params->tr_mask = WL_MACDBG_DFLT_TRIGMASK_LT128;
		}
	}

	/* default value for store mask */
	if (params->s_mask == 0 &&
	!(params->optn_bmp & (1 << WL_MACCAPT_STORE)) &&
	!WL_MACDBG_IS_EXPERT_MODE) {
		params->s_mask = WL_MACDBG_DFLT_STOREMASK;
		params->optn_bmp |= (1 << WL_MACCAPT_STORE);
	}

	/* default value for psm sel */
	if (WL_MACDBG_IS_PSMSEL_AUTO && !WL_MACDBG_IS_EXPERT_MODE) {
		if (D11REV_GE(wlc->pub->corerev, 128)) {
			switch (params->gpio_sel) {
				case WL_MACDBG_GPIO_PC1:
				case WL_MACDBG_GPIO_UTRACE1:
					params->psmsel_bmp = WL_MACCAP_SEL_PSMR1;
					break;
				case WL_MACDBG_GPIO_PCX:
				case WL_MACDBG_GPIO_UTRACEX:
					params->psmsel_bmp = WL_MACCAP_SEL_PSMX;
					break;
				case WL_MACDBG_GPIO_PC:
				case WL_MACDBG_GPIO_UTRACE:
				default:
					params->psmsel_bmp = WL_MACCAP_SEL_PSMR0;
			}
		}
	} else if (!WL_MACDBG_IS_PSMSEL_AUTO) {
		if (!PSMX_ENAB(wlc->pub) && (params->psmsel_bmp & WL_MACCAP_SEL_PSMX))
			params->psmsel_bmp &= ~WL_MACCAP_SEL_PSMX;
		if (!PSMR1_ENAB(wlc->hw) && (params->psmsel_bmp & WL_MACCAP_SEL_PSMR1))
			params->psmsel_bmp &= ~WL_MACCAP_SEL_PSMR1;
	}

	/* default value for transition mask */
	if (params->x_mask == 0 &&
	!(params->optn_bmp & (1 << WL_MACCAPT_TRANS)) &&
	!WL_MACDBG_IS_EXPERT_MODE) {
		if (D11REV_GE(wlc->pub->corerev, 128)) {
			if (params->gpio_sel == WL_MACDBG_GPIO_PC ||
			params->gpio_sel == WL_MACDBG_GPIO_PCX ||
			params->gpio_sel == WL_MACDBG_GPIO_PC1) {
				params->x_mask = WL_MACDBG_DFLT_TRANSMASK_GE128;
				params->optn_bmp &= ~(7 << WL_MACCAPT_LOGSEL);
				if (params->gpio_sel == WL_MACDBG_GPIO_PC) {
					params->optn_bmp |=
						(WL_MACDBG_GPIO_SELPSMR0 << WL_MACCAPT_LOGSEL);
				} else if (params->gpio_sel == WL_MACDBG_GPIO_PCX) {
					params->optn_bmp |=
						(WL_MACDBG_GPIO_SELPSMX << WL_MACCAPT_LOGSEL);
				} else if (params->gpio_sel == WL_MACDBG_GPIO_PC1) {
					params->optn_bmp |=
						(WL_MACDBG_GPIO_SELPSMR1 << WL_MACCAPT_LOGSEL);
				} else {
					/* pass */
				}
			} else {
				/* for non-pc capture, turn off log_en and filter modes */
				params->optn_bmp &= ~(3 << WL_MACCAPT_LOGMODE);
				params->optn_bmp &= ~(1 << WL_MACCAPT_FILTER);
				params->optn_bmp &= ~(7 << WL_MACCAPT_LOGSEL);
				if ((params->gpio_sel == WL_MACDBG_GPIO_UTRACE) ||
					(params->gpio_sel == WL_MACDBG_GPIO_UTRACEX) ||
					(params->gpio_sel == WL_MACDBG_GPIO_UTRACE1)) {
					params->x_mask = WL_MACDBG_DFLT_TRANSMASK_UTRACE;
				} else {
					params->x_mask = WL_MACDBG_DFLT_TRANSMASK_ALL;
				}
			}
		} else if (D11REV_GE(wlc->pub->corerev, 56)) {
			if (params->gpio_sel == WL_MACDBG_GPIO_PC ||
			params->gpio_sel == WL_MACDBG_GPIO_PCX) {
				params->x_mask = WL_MACDBG_DFLT_TRANSMASK_LT128;
			} else if ((params->gpio_sel == WL_MACDBG_GPIO_UTRACE) ||
				(params->gpio_sel == WL_MACDBG_GPIO_UTRACEX) ||
				(params->gpio_sel == WL_MACDBG_GPIO_UTRACE1)) {
				params->x_mask = WL_MACDBG_DFLT_TRANSMASK_UTRACE;
			} else {
				params->x_mask = WL_MACDBG_DFLT_TRANSMASK_ALL;
			}
		}

		params->optn_bmp |= (1 << WL_MACCAPT_TRANS);
	}

	/* Enable Sample Capture Clock */
	d11_regvalue =  R_REG(wlc->osh, D11_PSMCoreCtlStat(wlc));
	W_REG(wlc->osh, D11_PSMCoreCtlStat(wlc), (uint16)(d11_regvalue | PSM_CORE_CTL_SS));

	/* Setting options bitmap */
	/* Some options are not supported for CoreRev < 50 */
	if (D11REV_LT(wlc->pub->corerev, 56)) {
		if (params->x_mask) {
			WL_ERROR(("%s: Transition Mask not supported below CoreRev 50\n",
				__FUNCTION__));
			params->optn_bmp &= ~(1 << WL_MACCAPT_TRANS);
		}

		if ((params->m_val) || (params->m_mask)) {
			WL_ERROR(("%s: Match Mode not supported below CoreRev 50\n",
				__FUNCTION__));
			params->optn_bmp &= ~(1 << WL_MACCAPT_MATCH);
		}
	}

	/* Sample Capture Options */
	if (params->optn_bmp) {
		if ((err = wlc_macdbg_smpl_capture_optns(wlc, params, TRUE))
				!= BCME_OK) {
			WL_ERROR(("wl%d: %s: wlc_macdbg_smpl_capture_optns failed err=%d\n",
				wlc->pub->unit, __FUNCTION__, err));
			return err;
		}
	}

	/* Start MAC Sample Capture */
	if (D11REV_GE(wlc->pub->corerev, 56)) {
		/* Sample Capture Source and Start bits */
		params->smpl_ctrl |= (SC_SRC_MAC << SC_SRC_SHIFT) | SC_STRT;
		/* Save the latest smpl_ctrl so that it can be restored after wl down/up */
		macdbg->smpl_ctrl = params->smpl_ctrl;
		W_REG(wlc->osh, smpl_ctrl_reg, (uint16)(params->smpl_ctrl
			| (params->psmsel_bmp << WL_MACCAP_SEL_PSMR0_SHIFT)));
		if (use_psm) {
			if (PSMX_ENAB(wlc->pub))
				MACDBG_WRITE_MACREGX(smpl_ctrl_reg, params->smpl_ctrl |
				((params->psmsel_bmp << WL_MACCAP_SEL_PSMX_SHIFT) & SC_ENABLE));
			if (PSMR1_ENAB(wlc->hw))
				MACDBG_WRITE_MACREGR1(smpl_ctrl_reg, params->smpl_ctrl |
				((params->psmsel_bmp << WL_MACCAP_SEL_PSMR1_SHIFT) & SC_ENABLE));
		}

		WL_TRACE(("%s: SampleCollectPlayCtrl(0xb2e): "
			"0x%x, GPIO Out Sel:0x%02x\n", __FUNCTION__,
			MACDBG_MACCPTR_RREG(smpl_ctrl_reg, params->psmsel_bmp),
			R_REG(wlc->osh, D11_MacControl1(wlc))));
	} else {
		d11_regvalue =  R_REG(wlc->osh, D11_PSM_PHY_CTL(wlc));
		d11_regvalue |= (PHYCTL_PHYCLKEN | PHYCTL_SC_STRT | PHYCTL_SC_SRC_LB |
				PHYCTL_SC_TRANS_EN);
		W_REG(wlc->osh, D11_PSM_PHY_CTL(wlc), (uint16)d11_regvalue);

		WL_TRACE(("%s: PHY_CTL(0x492): 0x%x, GPIO Out Sel:0x%02x\n", __FUNCTION__,
				R_REG(wlc->osh, D11_PSM_PHY_CTL(wlc)),
				R_REG(wlc->osh, D11_MacControl1(wlc))));

	}

	/* Store config info */
	memcpy((void *)cur_params, params, sizeof(*params));

	return err;
} /* wlc_macdbg_smpl_capture_set */

static int
wlc_macdbg_smpl_capture_get(wlc_info_t *wlc, char *outbuf, int outlen)
{
	struct bcmstrbuf bstr;
	wlc_macdbg_info_t *macdbg = wlc->macdbg;
	wl_maccapture_params_t *cur_params = (wl_maccapture_params_t *)macdbg->smpl_info;
	wl_maccapture_params_t get_params;
	uint32 cur_ptr = 0; /* Sample Capture cur_ptr */
	bool use_psm = FALSE;
	bool use_strig = FALSE;
	uint16 smp_ctrl_off, sct_mask_off_l, sct_val_off_l;
	uint16 scs_mask_off_l, scx_mask_off_l, scm_mask_off_l, scm_val_off_l;
	uint16 strptr_off, stpptr_off, curptr_off;
	int err = BCME_OK;
	char smp_ctrl2_str[23] = "";
	char psmsel_str[57] = "";
	char psmr1_str[8] = "";
	char psmx_str[8] = "";

	/* HW is turned off so don't try to access it */
	if (wlc->pub->hw_off || DEVICEREMOVED(wlc))
		return BCME_RADIOOFF;

	if (D11REV_LT(wlc->pub->corerev, 40)) {
		WL_ERROR(("wl%d: %s only supported for corerev >=40\n",
			wlc->pub->unit, __FUNCTION__));
		return BCME_UNSUPPORTED;
	}

	bcm_binit(&bstr, outbuf, outlen);

	memset((void *)outbuf, 0, outlen);
	memset((void *)&get_params, 0, sizeof(get_params));

	smp_ctrl_off = sct_mask_off_l = sct_val_off_l = 0;
	scs_mask_off_l = scx_mask_off_l = scm_mask_off_l = scm_val_off_l = 0;
	strptr_off = stpptr_off = curptr_off = 0;

	/* Check clock */
	if (!wlc->clk) {
		bcm_bprintf(&bstr, "MAC Capture\nNo Clock so returning saved state:\n");
		memcpy(&get_params, (void *)cur_params, sizeof(wl_maccapture_params_t));
		/* If no clock, return 0 for cur_ptr */
		goto print_values;
	}

	/* GPIO Out Sel */
	get_params.gpio_sel = (R_REG(wlc->osh, D11_MacControl1(wlc)) & MCTL1_GPIOSEL_MASK)
		>> MCTL1_GPIOSEL_SHIFT;

	use_psm = (WL_MACDBG_IS_USEPSM(cur_params) && D11REV_GE(wlc->pub->corerev, 129));
	use_strig = (WL_MACDBG_IS_STOPTRIGGER(cur_params) && D11REV_GE(wlc->pub->corerev, 129));

	if (use_psm) {
		/* Use PSM */
		get_params.optn_bmp = (1 << WL_MACCAPT_USEPSM);
		get_params.psmsel_bmp = MACDBG_GET_PSMSEL;
		get_params.start_ptr =
			MACDBG_MACCPTR_RREG(D11_PSM_SCP_STRTPTR(wlc), get_params.psmsel_bmp);
		get_params.stop_ptr =
			MACDBG_MACCPTR_RREG(D11_PSM_SCP_STOPPTR(wlc), get_params.psmsel_bmp);
		get_params.smp_sz = get_params.stop_ptr - get_params.start_ptr + 1;
		/* Cur. Ptr */
		cur_ptr = MACDBG_MACCPTR_RREG(D11_PSM_SCP_CURPTR(wlc), get_params.psmsel_bmp);
		smp_ctrl_off = D11_PSM_SMP_CTRL_OFFSET(wlc);
		strptr_off = D11_PSM_SCP_STRTPTR_OFFSET(wlc);
		stpptr_off = D11_PSM_SCP_STOPPTR_OFFSET(wlc);
		curptr_off = D11_PSM_SCP_CURPTR_OFFSET(wlc);
		scs_mask_off_l = D11_PSM_SCS_MASK_L_OFFSET(wlc);
		scm_mask_off_l = D11_PSM_SCM_MASK_L_OFFSET(wlc);
		scm_val_off_l = D11_PSM_SCM_VAL_L_OFFSET(wlc);
		scx_mask_off_l = D11_PSM_SCX_MASK_L_OFFSET(wlc);
		sct_mask_off_l = use_strig ?
			D11_PSM_SCTSTP_MASK_L_OFFSET(wlc) : D11_PSM_SCT_MASK_L_OFFSET(wlc);
		sct_val_off_l = use_strig ?
			D11_PSM_SCTSTP_VAL_L_OFFSET(wlc) : D11_PSM_SCT_VAL_L_OFFSET(wlc);
	} else {
		/* Use TXE */
		get_params.optn_bmp = (1 << WL_MACCAPT_USETXE);
		get_params.start_ptr = R_REG(wlc->osh, D11_SCP_STRTPTR(wlc));
		get_params.stop_ptr = R_REG(wlc->osh, D11_SCP_STOPPTR(wlc));
		get_params.smp_sz = get_params.stop_ptr - get_params.start_ptr + 1;
		/* Cur. Ptr */
		cur_ptr = R_REG(wlc->osh, D11_SCP_CURPTR(wlc));
		smp_ctrl_off = D11_SMP_CTRL_OFFSET(wlc);
		strptr_off = D11_SCP_STRTPTR_OFFSET(wlc);
		stpptr_off = D11_SCP_STOPPTR_OFFSET(wlc);
		curptr_off = D11_SCP_CURPTR_OFFSET(wlc);
		scs_mask_off_l = D11_SCS_MASK_L_OFFSET(wlc);
		scm_mask_off_l = D11_SCM_MASK_L_OFFSET(wlc);
		scm_val_off_l = D11_SCM_VAL_L_OFFSET(wlc);
		scx_mask_off_l = D11_SCX_MASK_L_OFFSET(wlc);
		sct_mask_off_l = use_strig ?
			D11_SCTSTP_MASK_L_OFFSET(wlc) : D11_SCT_MASK_L_OFFSET(wlc);
		sct_val_off_l = use_strig ?
			D11_SCTSTP_VAL_L_OFFSET(wlc) : D11_SCT_VAL_L_OFFSET(wlc);
	}

	/* Read the options from option regs */
	/* Supported options */
	if (D11REV_GE(wlc->pub->corerev, 56)) {
		get_params.optn_bmp |= (1 << WL_MACCAPT_TRIG) | (1 << WL_MACCAPT_STORE) |
			(1 << WL_MACCAPT_TRANS) | (1 << WL_MACCAPT_MATCH);
		if (use_strig) {
			get_params.optn_bmp |= (1 << WL_MACCAPT_STOPTRIG);
		}
	} else {
		get_params.optn_bmp = (1 << WL_MACCAPT_TRIG) | (1 << WL_MACCAPT_STORE);
	}

	if ((err = wlc_macdbg_smpl_capture_optns(wlc, &get_params, FALSE))
		!= BCME_OK) {
		WL_ERROR(("wl%d: %s: wlc_macdbg_smpl_capture_optns failed err=%d\n",
			wlc->pub->unit, __FUNCTION__, err));
		return err;
	}

	if (D11REV_GE(wlc->pub->corerev, 129)) {
		sprintf(smp_ctrl2_str, "smp_ctrl2(%x):0x%x",
		(use_psm) ? D11_PSM_SMP_CTRL2_OFFSET(wlc) : D11_SMP_CTRL2_OFFSET(wlc),
		(get_params.optn_bmp >> WL_MACCAPT_LOGMODE) & 0x3f);
		if (use_psm && (PSMR1_ENAB(wlc->hw) || PSMX_ENAB(wlc->pub))) {
			if (PSMX_ENAB(wlc->pub)) {
				sprintf(psmx_str, "0x%04x/",
				MACDBG_READ_MACREGX(D11_PSM_SMP_CTRL(wlc)));
			}
			if (PSMR1_ENAB(wlc->hw)) {
				sprintf(psmr1_str, "0x%04x/",
				MACDBG_READ_MACREGR1(D11_PSM_SMP_CTRL(wlc)));
			}
			sprintf(psmsel_str, "smp_ctrl %s%sr0 : %s%s0x%04x, psmsel_bmp 0x%01x\n",
				(PSMX_ENAB(wlc->pub)) ? "x/" : "",
				(PSMR1_ENAB(wlc->hw)) ? "r1/" : "",
				psmx_str, psmr1_str,
				R_REG(wlc->osh, D11_PSM_SMP_CTRL(wlc)),
				cur_params->psmsel_bmp);

		}
	}

	bcm_bprintf(&bstr, "MAC Capture Registers(address): val\n");

print_values:
	bcm_bprintf(&bstr,
		"Use %s. Sel:0x%x MacControl1(%x):0x%x smp_ctrl(%x) 0x%x %s\n"
		"start_ptr(%x):0x%04x, stop_ptr(%x):0x%04x, cur_ptr(%x):0x%04x\n"
		"store mask(%x):0x%08x, match mask(%x):0x%08x, match val(%x):0x%x\n"
		"trans mask(%x):0x%08x, %strig mask(%x):0x%08x, %strig val(%x):0x%x\n"
		"entries:%d, size:%d(B) state:0x%02x Logic Analyzer Mode On:%d\n"
		"%s",
		use_psm ? "PSM" : "TPL",
		get_params.gpio_sel,
		D11_MacControl1_OFFSET(wlc),
		(wlc->clk) ? R_REG(wlc->osh, D11_MacControl1(wlc)) : 0xffff,
		smp_ctrl_off,
		get_params.smpl_ctrl,
		smp_ctrl2_str,
		strptr_off,
		get_params.start_ptr,
		stpptr_off,
		get_params.stop_ptr,
		curptr_off,
		cur_ptr,
		scs_mask_off_l,
		get_params.s_mask,
		scm_mask_off_l,
		get_params.m_mask,
		scm_val_off_l,
		get_params.m_val,
		scx_mask_off_l,		/* trans mask offset */
		get_params.x_mask,	/* trans mask val */
		use_strig ? "s" : "",
		sct_mask_off_l,
		get_params.tr_mask,
		use_strig ? "s" : "",
		sct_val_off_l,
		get_params.tr_val,
		get_params.smp_sz, get_params.smp_sz << 2,
		get_params.cmd,
		get_params.la_mode,
		psmsel_str);

#if defined(MBSS)
	if (MBSS_ENAB(wlc->pub) && !use_psm) {
		bcm_bprintf(&bstr, "Warning: MBSS is on. Double-check strt_ptr!\n");
	}
#endif /* defined(MBSS) */

	return err;
} /* wlc_macdbg_smpl_capture_get */

#endif

#if defined(BCMDBG_DUMP_D11CNTS)
static int
BCMATTACHFN(wlc_macdbg_init_printlist)(wlc_macdbg_info_t *macdbg)
{
	wlc_info_t *wlc = macdbg->wlc;
	int err = BCME_OK, i, j;

	/* print list definition. no corerev dependency to define the table.
	 * anything not supported by this corerev will be filtered out.
	 */
	/* SHM counter */
	d11print_list_t d11print_tbl[] = {
		{"rxtrig_myaid", M_RXTRIG_MYAID_CNT(wlc)},
		{"rxtrig_rand", M_RXTRIG_RAND_CNT(wlc)},
		{"rxswrst", M_RXSWRST_CNT(wlc)},
		{"bfd_done", M_BFD_DONE_CNT(wlc)},
		{"bfd_fail", M_BFD_FAIL_CNT(wlc)},
		{"rptovrd", M_RPTOVRD_CNT(wlc)},
		{"rxpfflush", M_RXPFFLUSH_CNT(wlc)},
		{"rxflucmt", M_RXFLUCMT_CNT(wlc)},
		{"rxfluov", M_RXFLUOV_CNT(wlc)},
		{"agg0", M_AGG0_CNT(wlc)},
		{"trigrefill", M_TRIGREFILL_CNT(wlc)},
		{"txtrig", M_TXTRIG_CNT(wlc)},
		{"rxhetbba", M_RXHETBBA_CNT(wlc)},
		{"txbamtid", M_TXBAMTID_CNT(wlc)},
		{"txbamsta", M_TXBAMSTA_CNT(wlc)},
		{"txmba", M_TXMBA_CNT(wlc)},
		{"rxmydtinrsp", M_RXMYDTINRSP(wlc)},
		{"rxexit", M_RXEXIT_CNT(wlc)},
		{"txampdu", M_TXAMPDU_CNT(wlc)},
		{"txtrigampdu", M_TXTRIGAMPDU_CNT(wlc)},
		{"txmurts", M_TXMURTS_CNT(wlc)},
		{"txmubar", M_TXMUBAR_CNT(wlc)},
		{"txhetbba", M_TXHETBBA_CNT(wlc)},
		{"txqnull", M_TXQNL_CNT(wlc)},
		{"txtbdata", M_HETBFP_CNT(wlc)},
		{"skip_ffqcons", M_SKIP_FFQCONS_CNT(wlc)},
	};

	/* count the number of valid entries */
	macdbg->d11print_sz = 0;
	for (i = 0; i < ARRAYSIZE(d11print_tbl); i++) {
		if (d11print_tbl[i].addr == (uint16)(-1)) {
			continue;
		}
		WL_TRACE(("wl%d: %s [%d] (0x%-4x) %s\n", wlc->pub->unit, __FUNCTION__,
			i, d11print_tbl[i].addr, d11print_tbl[i].name));
		macdbg->d11print_sz++;
	}

	/* don't malloc if nothing to print */
	if (macdbg->d11print_sz != 0) {
		/* space for valid d11print entries */
		macdbg->pd11print = MALLOCZ(wlc->osh,
			(macdbg->d11print_sz * sizeof(macdbg->pd11print[0])));
		if (macdbg->pd11print == NULL) {
			WL_ERROR(("wl%d: %s pd11print alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		/* space for SHM read snapshot */
		macdbg->d11cnts_snapshot = MALLOCZ(wlc->osh,
			(macdbg->d11print_sz * sizeof(macdbg->d11cnts_snapshot[0])));
		if (macdbg->d11cnts_snapshot == NULL) {
			WL_ERROR(("wl%d: %s d11cnts_snapshot alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		/* space for 32bit bins for counters */
		macdbg->d11cnts = MALLOCZ(wlc->osh,
			(macdbg->d11print_sz * sizeof(macdbg->d11cnts[0])));
		if (macdbg->d11cnts == NULL) {
			WL_ERROR(("wl%d: %s d11cnts alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		/* move valid d11print entries */
		j = 0;
		for (i = 0; i < ARRAYSIZE(d11print_tbl); i++) {
			if (d11print_tbl[i].addr == (uint16)(-1)) {
				continue;
			}
			memcpy(&macdbg->pd11print[j], &d11print_tbl[i], sizeof(d11print_tbl[0]));
			j++;
		}
	}

exit:
	return err;

} /* wlc_macdbg_init_printlist */

#ifdef WL_PSMX
static int
BCMATTACHFN(wlc_macdbg_init_psmx_printlist)(wlc_macdbg_info_t *macdbg)
{
	wlc_info_t *wlc = macdbg->wlc;
	int err = BCME_OK, i, j;

	/* print list definition. no corerev dependency to define the table.
	 * anything not supported by this corerev will be filtered out.
	 */
	/* SHMx counter */
	d11print_list_t d11printx_tbl[] = {
		{"m2vmsg", MX_M2VMSG_CNT(wlc)},
		{"v2mmsg", MX_V2MMSG_CNT(wlc)},
		{"m2vgrp", MX_M2VGRP_CNT(wlc)},
		{"v2mgrp", MX_V2MGRP_CNT(wlc)},
		{"v2mgrpinv", MX_V2MGRPINV_CNT(wlc)},
		{"m2vsnd", MX_M2VSND_CNT(wlc)},
		{"v2msnd", MX_V2MSND_CNT(wlc)},
		{"v2msndinv", MX_V2MSNDINV_CNT(wlc)},
		{"m2vcqi", MX_M2VCQI_CNT(wlc)},
		{"v2mcqi", MX_V2MCQI_CNT(wlc)},
		{"ffqadd", MX_FFQADD_CNT(wlc)},
		{"ffqdel", MX_FFQDEL_CNT(wlc)},
		{"mfqadd", MX_MFQADD_CNT(wlc)},
		{"mfqdel", MX_MFQDEL_CNT(wlc)},
		{"mfoqadd", MX_MFOQADD_CNT(wlc)},
		{"mfoqdel", MX_MFOQDEL_CNT(wlc)},
		{"ofqadd", MX_OFQADD_CNT(wlc)},
		{"ofqdel", MX_OFQDEL_CNT(wlc)},
		{"rucfg", MX_RUCFG_CNT(wlc)},
		{"m2vru", MX_M2VRU_CNT(wlc)},
		{"v2mru", MX_V2MRU_CNT(wlc)},
		{"ofqagg0", MX_OFQAGG0_CNT(wlc)},
		{"m2sq0", MX_M2SQ0_CNT(wlc)},
		{"m2sq1", MX_M2SQ1_CNT(wlc)},
		{"m2sq2", MX_M2SQ2_CNT(wlc)},
		{"m2sq3", MX_M2SQ3_CNT(wlc)},
		{"m2sq4", MX_M2SQ4_CNT(wlc)},
		{"sndfl", MX_SNDFL_CNT(wlc)},
		{"hecapinv", MX_HEMUCAPINV_CNT(wlc)},
		{"m2sqtxvevt", MX_M2SQTXVEVT_CNT(wlc)},
		{"mmuregrp", MX_MMUREGRP_CNT(wlc)},
		{"mmuemgcgrp", MX_MMUEMGCGRP_CNT(wlc)},
		{"m2vgrpwait", MX_M2VGRPWAIT_CNT(wlc)},
		{"om2sq0", MX_OM2SQ0_CNT(wlc)},
		{"om2sq1", MX_OM2SQ1_CNT(wlc)},
		{"om2sq2", MX_OM2SQ2_CNT(wlc)},
		{"om2sq3", MX_OM2SQ3_CNT(wlc)},
		{"om2sq4", MX_OM2SQ4_CNT(wlc)},
		{"om2sq5", MX_OM2SQ5_CNT(wlc)},
		{"om2sq6", MX_OM2SQ6_CNT(wlc)},
		{"tafwmismatch", MX_TAFWMISMATCH_CNT(wlc)},
		{"omredist", MX_OMREDIST_CNT(wlc)},
		{"omtxd", MX_OMUTXDLMEMTCH_CNT(wlc)},
	};

		/* count the number of valid entries */
	macdbg->d11printx_sz = 0;
	for (i = 0; i < ARRAYSIZE(d11printx_tbl); i++) {
		if (d11printx_tbl[i].addr == (uint16)(-1)) {
			continue;
		}
		WL_TRACE(("wl%d: %s [%d] (0x%-4x) %s\n", wlc->pub->unit, __FUNCTION__,
			i, d11printx_tbl[i].addr, d11printx_tbl[i].name));
		macdbg->d11printx_sz++;
	}

	/* don't malloc if nothing to print */
	if (macdbg->d11printx_sz != 0) {
		macdbg->pd11printx = MALLOCZ(wlc->osh,
			(macdbg->d11printx_sz * sizeof(macdbg->pd11printx[0])));
		if (macdbg->pd11printx == NULL) {
			WL_ERROR(("wl%d: %s pd11printx alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		macdbg->d11cntsx_snapshot = MALLOCZ(wlc->osh,
			(macdbg->d11printx_sz * sizeof(macdbg->d11cntsx_snapshot[0])));
		if (macdbg->d11cntsx_snapshot == NULL) {
			WL_ERROR(("wl%d: %s d11cntsx_snapshot alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		macdbg->d11cntsx = MALLOCZ(wlc->osh,
			(macdbg->d11printx_sz * sizeof(macdbg->d11cntsx[0])));
		if (macdbg->d11cntsx == NULL) {
			WL_ERROR(("wl%d: %s d11cntsx alloc failed\n",
				wlc->pub->unit, __FUNCTION__));
			err = BCME_NOMEM;
			goto exit;
		}

		j = 0;
		for (i = 0; i < ARRAYSIZE(d11printx_tbl); i++) {
			if (d11printx_tbl[i].addr == (uint16)(-1)) {
				continue;
			}
			memcpy(&macdbg->pd11printx[j], &d11printx_tbl[i], sizeof(d11printx_tbl[0]));
			j++;
		}
	}

exit:
	return err;

} /* wlc_macdbg_init_psmx_printlist */
#endif /* WL_PSMX */

void
wlc_macdbg_upd_d11cnts(wlc_info_t *wlc)
{
#ifdef WLCNT
	wlc_macdbg_info_t *macdbg = wlc->macdbg;
	int i;

	if ((macdbg->d11cnts_snapshot != NULL) &&
		(macdbg->pd11print != NULL) && (macdbg->d11cnts != NULL)) {
		for (i = 0; i < macdbg->d11print_sz; i++) {
			uint16 v, v_delta;

			v = wlc_read_shm(wlc, macdbg->pd11print[i].addr);
			v_delta = (v - macdbg->d11cnts_snapshot[i]);
			if (v_delta != 0) {
				/* Update 32bit counter and snapshot */
				macdbg->d11cnts[i] += v_delta;
				macdbg->d11cnts_snapshot[i] = v;
			}
		}
	}

#ifdef WL_PSMX
	/* SHMx counters */
	if ((macdbg->d11cntsx_snapshot != NULL) &&
		(macdbg->pd11printx != NULL) && (macdbg->d11cntsx != NULL)) {
		for (i = 0; i < macdbg->d11printx_sz; i++) {
			uint16 v, v_delta;

			v = wlc_read_shmx(wlc, macdbg->pd11printx[i].addr);
			v_delta = (v - macdbg->d11cntsx_snapshot[i]);
			if (v_delta != 0) {
				/* Update 32bit counter and snapshot */
				macdbg->d11cntsx[i] += v_delta;
				macdbg->d11cntsx_snapshot[i] = v;
			}
		}
	}
#endif  /* WL_PSMX */
#endif /* WLCNT */
	return;
}

#ifdef WLCNT
static void
wlc_dump_muagg_histo(wlc_info_t *wlc, struct bcmstrbuf *b, muagg_histo_t histo)
{
		muagg_histo_bucket_t *entry;
		int i;

#ifdef WL_PSMX
		/* Print out Sched idle time measurements */
		bcm_bprintf(b, "\n%s %4u (in 800msec)", "max sched idle time [%]:",
			wlc_bmac_read_shmx(wlc->hw, MX_SCHED_MAX(wlc)) / SCHED_IDLE_DIV);
		bcm_bprintf(b, "\n%s ", "last 8 measurements  [%]:");
		for (i = 0; i < SCHED_IDLE_LIST; i++) {
			bcm_bprintf(b, "%4u  ", wlc_bmac_read_shmx(wlc->hw,
				MX_SCHED_HIST(wlc) + i * 2) / SCHED_IDLE_DIV);
		}
#endif /* WL_PSMX */

		/* Print out SU info */
		bcm_bprintf(b, "\n\n%30s %6u", "su ampdu cnt:",
			wlc_bmac_read_shm(wlc->hw, M_TXAMPDUSU_CNT(wlc)));

		/* Print out muagg histo sum */
		entry = &histo.bucket[MUAGG_SUM_BUCKET];
		bcm_bprintf(b, "\n%30s %6u", "muagg histo sum:", entry->sum);
		bcm_bprintf(b, "\n%30s ", "muagg histogram:");
		for (i = 0; i < MAX_MUAGG_USERS; i++) {
			bcm_bprintf(b, "%6u  ", entry->num_ppdu[i]);
			if ((i & 0x7) == 0x7 && i != MAX_MUAGG_USERS - 1) {
				bcm_bprintf(b, "\n%30s ", " ");
			}
		}

		/* Print out VHTMU muagg histo */
		entry = &histo.bucket[MUAGG_VHTMU_BUCKET];
		bcm_bprintf(b, "\n%30s %6u", "vht mumimo agg histo sum:", entry->sum);
		bcm_bprintf(b, "\n%30s ", "vht mumimo agg histogram:");
		for (i = 0; i < MAX_VHTMU_USERS; i++) {
			bcm_bprintf(b, "%6u  ", entry->num_ppdu[i]);
		}

		/* Print out HEMUMIMO muagg histo */
		entry = &histo.bucket[MUAGG_HEMUMIMO_BUCKET];
		bcm_bprintf(b, "\n%30s %6u", "he  mumimo agg histo sum:", entry->sum);
		bcm_bprintf(b, "\n%30s ", "he  mumimo agg histogram:");
		for (i = 0; i < MAX_HEMUMIMO_USERS; i++) {
			bcm_bprintf(b, "%6u  ", entry->num_ppdu[i]);
		}

		/* Print out HEOFDMA histo sum */
		entry = &histo.bucket[MUAGG_HEOFDMA_BUCKET];
		bcm_bprintf(b, "\n%30s %6u", "he dlofdma agg histo sum:", entry->sum);
		bcm_bprintf(b,  "\n%30s ", "he dlofdma agg histogram:");
		for (i = 0; i < MAX_HEOFDMA_USERS; i++) {
			bcm_bprintf(b, "%6u  ", entry->num_ppdu[i]);
			if ((i & 0x7) == 0x7 && i != MAX_HEOFDMA_USERS - 1) {
				bcm_bprintf(b, "\n%30s ", " ");
			}
		}
		bcm_bprintf(b, "\n");
}
#endif /* WLCNT */

static int
wlc_dump_d11cnts(wlc_info_t *wlc, struct bcmstrbuf * b)
{
	int i = 0;
	wlc_macdbg_info_t *macdbg = wlc->macdbg;
	wlc_macdbg_airtime_t* p_airtime = &macdbg->airtime_stats[0];
	uint32 delta_time;
	uint32 tot_upd = 0;
	uint32 tot_tx_upd = 0;
	uint32 tot_tx_airtime = 0;
	char percent[PERCENT_STR_LEN];
	static const char *airtime_type_names[AIRTIME_ENUM_MAX] = {
		"su tx", "vhtmu tx", "dlofdma tx", "hemmu tx", "ulmu rx"
	};

#ifdef WLCNT
	muagg_histo_t histo;

	if (macdbg->pd11print == NULL || D11REV_LT(wlc->pub->corerev, 128)) {
		return BCME_UNSUPPORTED;
	}

#ifdef WL_PSMX
	if (macdbg->pd11printx == NULL || D11REV_LT(wlc->pub->corerev, 128)) {
		return BCME_UNSUPPORTED;
	}
#endif /* WL_PSMX */

	/* update from SHM */
	wlc_statsupd(wlc);

	while (i < macdbg->d11print_sz) {
		bcm_bprintf(b, "%s %u ", macdbg->pd11print[i].name, macdbg->d11cnts[i]);
		i++;
		/* new line after every 8 counters */
		if ((i & 0x7) == 0) {
			bcm_bprintf(b, "\n");
		}
	}

#ifdef WL_PSMX
	/* SHMx */
	i = 0;
	bcm_bprintf(b, "\n");
	while (i < macdbg->d11printx_sz) {
		bcm_bprintf(b, "%s %u ", macdbg->pd11printx[i].name, macdbg->d11cntsx[i]);
		i++;
		/* new line after every 8 counters */
		if ((i & 0x7) == 0) {
			bcm_bprintf(b, "\n");
		}
	}
#endif /* WL_PSMX */

	if (wlc->clk) {
		wlc_get_muagg_histo(wlc, &histo);
		wlc_dump_muagg_histo(wlc, b, histo);
	} else {
		bcm_bprintf(b, "\nmuagg histo not available");
	}
	bcm_bprintf(b, "\n");
#endif /* WLCNT */

	/* Following is to dump airtime stats if conditions are met */
	if (!macdbg->ppdutxs || !wlc->clk) {
		return BCME_OK;
	}

	for (i = 0; i < AIRTIME_ENUM_MAX; i++) {
		tot_upd += p_airtime[i].num_upd;
		if (i < AIRTIME_RX_ULMU) {
			tot_tx_upd += p_airtime[i].num_upd;
			tot_tx_airtime += p_airtime[i].airtime;
		}
	}

	if (tot_upd == 0) {
		return BCME_OK;
	}

	delta_time = (R_REG(wlc->osh, D11_TSFTimerLow(wlc))) - macdbg->tsf_time;
	bcm_bprintf(b, "        %15s%15s", "tot", "tot tx");
	for (i = 0; i < AIRTIME_ENUM_MAX; i++) {
		if (p_airtime[i].num_upd) {
			bcm_bprintf(b, "%15s", airtime_type_names[i]);
		}
	}

	/* It is guaranteed tot_upd != 0 */
	snprintf(percent, PERCENT_STR_LEN, "(%d%%)", tot_tx_upd * 100 / tot_upd);
	bcm_bprintf(b, "\nNum     %15d%9d%6s", tot_upd, tot_tx_upd, percent);
	for (i = 0; i < AIRTIME_ENUM_MAX; i++) {
		snprintf(percent, PERCENT_STR_LEN, "(%d%%)",
			MACDBG_NUMUPD_PERCENT(&p_airtime[i], tot_upd));
		if (p_airtime[i].num_upd) {
			bcm_bprintf(b, "%9d%6s", p_airtime[i].num_upd, percent);
		}
	}

	snprintf(percent, PERCENT_STR_LEN, "(%d%%)",
		delta_time ? tot_tx_airtime * 100 / delta_time : 0);
	bcm_bprintf(b, "\nCum (ms)%15d%9d%6s", delta_time/1000,
		tot_tx_airtime / 1000, percent);
	for (i = 0; i < AIRTIME_ENUM_MAX; i++) {
		snprintf(percent, PERCENT_STR_LEN, "(%d%%)",
			MACDBG_AIRTIME_PERCENT(&p_airtime[i], delta_time));
		if (p_airtime[i].num_upd) {
			bcm_bprintf(b, "%9d%6s", p_airtime[i].airtime/1000, percent);
		}
	}
	bcm_bprintf(b, "\nAvg (us)%15s%15d", "",
		tot_tx_upd ? tot_tx_airtime / tot_tx_upd : 0);
	for (i = 0; i < AIRTIME_ENUM_MAX; i++) {
		if (p_airtime[i].num_upd) {
			bcm_bprintf(b, "%15d", MACDBG_AIRTIME_AVG(&p_airtime[i]));
		}
	}
	bcm_bprintf(b, "\n");

	return BCME_OK;
}

static int
wlc_dump_clear_d11cnts(wlc_info_t *wlc)
{
#ifdef WLCNT
	wlc_macdbg_info_t *macdbg = wlc->macdbg;

	if (macdbg->pd11print == NULL || D11REV_LT(wlc->pub->corerev, 128)) {
		return BCME_UNSUPPORTED;
	}

#ifdef WL_PSMX
	if (macdbg->pd11printx == NULL || D11REV_LT(wlc->pub->corerev, 128)) {
		return BCME_UNSUPPORTED;
	}
#endif /* WL_PSMX */
	/* update to clear the pending values in SHM */
	wlc_statsupd(wlc);
	memset(macdbg->d11cnts, 0,
		(sizeof(macdbg->d11cnts[0]) * macdbg->d11print_sz));
#ifdef WL_PSMX
	memset(macdbg->d11cntsx, 0,
		(sizeof(macdbg->d11cntsx[0]) * macdbg->d11printx_sz));
#endif /* WL_PSMX */
	if (wlc->clk) {
		wlc_clear_muagg_histo(wlc);
	}
#endif /* WLCNT */

	if (wlc->clk) {
		macdbg->tsf_time = (R_REG(wlc->osh, D11_TSFTimerLow(wlc)));
	}

	memset(&macdbg->airtime_stats, 0, sizeof(macdbg->airtime_stats));

	return BCME_OK;
}

static int
wlc_dump_grphist(wlc_info_t *wlc, struct bcmstrbuf * b)
{
#ifdef WL_PSMX
	uint16 i, j, curidx;
	grp_hist_blk_t *grp_hist_blk;
	uint16 grp_hist_idx;
	uint16 txvidx_mand_hist[16] = {0, };
	uint16 txvidx_cand_hist[16] = {0, };
	uint16 txvidx_grp_hist[16] = {0, };
#ifdef WLC_MACDBG_FRAMEID_TRACE
	grp_hist_per_blk_t  *grp_hist_per_blk = NULL;
#endif

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

	grp_hist_blk = MALLOCZ(wlc->osh, GRP_HIST_BLK_WSZ);
	if (!grp_hist_blk) {
		return BCME_NOMEM;
	}

	grp_hist_idx = wlc_bmac_read_shmx(wlc->hw, MX_M2VGRP_CNT(wlc)) & (GRP_HIST_NUM - 1);

	wlc_bmac_copyfrom_objmem(wlc->hw, MX_GRP_HIST_BLK(wlc), grp_hist_blk,
		GRP_HIST_BLK_WSZ, OBJADDR_SHMX_SEL);

	bcm_bprintf(b, "[idx] (sndupd_seq mu_type sgi grp_idx_bmp used_cnt)|"
			" (txvidx0 ... txvidxN : txvbmp_cand)|"
			" u0-3: (nssmcs fifo_grp_idx txvidx) (...)... |(txvbmp_grp)\n");

	for (i = 0; i < GRP_HIST_NUM; i++) {
		uint16 txvbmp_cand = 0, txvbmp_grp = 0, grp_idx_bmp;

		curidx = ((grp_hist_idx + i + 1) & (GRP_HIST_NUM - 1));
		grp_idx_bmp = (GRP_HIST_INFO_GRP_EXT(grp_hist_blk[curidx].info) ? 3 : 1) <<
			GRP_HIST_INFO_GRP_IDX(grp_hist_blk[curidx].info);

		bcm_bprintf(b, "[%02d] (%d %d %d 0x%02x %02d)| (",
			i,
			GRP_HIST_INFO_M2VSND_CNT(grp_hist_blk[curidx].info),
			GRP_HIST_INFO_MU_TYPE(grp_hist_blk[curidx].info),
			GRP_HIST_INFO_SGI(grp_hist_blk[curidx].info),
			grp_idx_bmp,
			GRP_HIST_INFO_USED_CNT(grp_hist_blk[curidx].info));
		txvidx_mand_hist[grp_hist_blk[curidx].txvidx_cand & 0xf]++;
		for (j = 0; j < 4; j++) {
			if (j > GRP_HIST_INFO_NUSR_CAND(grp_hist_blk[curidx].info)) {
				bcm_bprintf(b, "-- ");
			} else {
				bcm_bprintf(b, "%02d ", grp_hist_blk[curidx].txvidx_cand & 0xf);
				txvidx_cand_hist[grp_hist_blk[curidx].txvidx_cand & 0xf]++;
				txvbmp_cand |= (1 << (grp_hist_blk[curidx].txvidx_cand & 0xf));
			}
			grp_hist_blk[curidx].txvidx_cand >>= 4;
		}

		bcm_bprintf(b, ": 0x%04x)| u0-3: ", txvbmp_cand);
		for (j = 0; j < 4; j++) {
			if (grp_hist_blk[curidx].usr[j] == 0xffff) {
				bcm_bprintf(b, "(N/A)\t\t");
			} else {
				uint16 txvidx = GRP_HIST_USR_TXVIDX(grp_hist_blk[curidx].usr[j]);
				bcm_bprintf(b, "(0x%02x %02d %02d)\t",
					GRP_HIST_USR_NSSMCS(grp_hist_blk[curidx].usr[j]),
					GRP_HIST_USR_FIDX(grp_hist_blk[curidx].usr[j]),
					txvidx);
				txvidx_grp_hist[txvidx]++,
				txvbmp_grp |= (1 << txvidx);
			}
		}

		bcm_bprintf(b, "|(0x%x)\n", txvbmp_grp);
	}

	bcm_bprintf(b, "txvidx_cand histogram:\t");
	for (i = 0; i < 16; i++) {
		bcm_bprintf(b, "%02d ", txvidx_cand_hist[i]);
	}
	bcm_bprintf(b, "\n");

	bcm_bprintf(b, "txvidx_mand histogram:\t");
	for (i = 0; i < 16; i++) {
		bcm_bprintf(b, "%02d ", txvidx_mand_hist[i]);
	}
	bcm_bprintf(b, "\n");

	bcm_bprintf(b, "txvidx_grp histogram:\t");
	for (i = 0; i < 16; i++) {
		bcm_bprintf(b, "%02d ", txvidx_grp_hist[i]);
	}
	bcm_bprintf(b, "\n");

#ifdef WLC_MACDBG_FRAMEID_TRACE
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		grp_hist_per_blk = MALLOCZ(wlc->osh, GRP_HIST_PER_BLK_WSZ);
	}

	if (grp_hist_per_blk) {
		wlc_macdbg_info_t *macdbg = wlc->macdbg;
		uint16 txs_mutype, tx_cnt, txsucc_cnt, mismatch_cnt = 0;

		for (i = 0; i < TXS_HIST_NUM_ENT; i++) {
			/* 0: vhtmu, 1:hemu, 2:heom */
			txs_mutype = TX_STATUS128_MUTYP(macdbg->txs_hist[i].s5);
			if (!(macdbg->txs_hist[i].s5 & TX_STATUS128_MU_MASK) ||
					txs_mutype == TX_STATUS_MUTP_HEOM) {
				/* skip if SU or DL OFDMA */
				continue;
			}
			curidx = TX_STATUS64_MU_GRPSEQ(macdbg->txs_hist[i].s4);
			if (GRP_HIST_INFO_M2VSND_CNT(grp_hist_blk[curidx].info) !=
					TX_STATUS64_MU_SNDSEQ(macdbg->txs_hist[i].s4)) {
				continue;
			}
			/* compare (hemm | bw) */
			if (GRP_HIST_INFO_MU_TYPE(grp_hist_blk[curidx].info) !=
					((txs_mutype << 2) |
					TX_STATUS64_MU_BW(macdbg->txs_hist[i].s3))) {
				mismatch_cnt++;
				continue;
			}
			/* user's position in the group */
			j = TX_STATUS64_MU_GPOS(macdbg->txs_hist[i].s4);
			if (grp_hist_blk[curidx].usr[j] == 0xffff ||
					GRP_HIST_USR_NSSMCS(grp_hist_blk[curidx].usr[j]) !=
					TX_STATUS64_MU_NSSMCS(macdbg->txs_hist[i].s4)) {
				mismatch_cnt++;
				continue;
			}
			grp_hist_per_blk[curidx].rt0_cnt[j].tx_cnt +=
				TX_STATUS40_TXCNT_RT0(macdbg->txs_hist[i].s3);
			grp_hist_per_blk[curidx].rt0_cnt[j].txsucc_cnt +=
				TX_STATUS40_ACKCNT_RT0(macdbg->txs_hist[i].s3);
		}

		bcm_bprintf(b, "\ntxs grp_hist_blk mismatch_cnt: %d\n", mismatch_cnt);
		bcm_bprintf(b, "[idx] (tx_cnt txsucc_cnt PER) (tx_cnt txsucc_cnt PER) ...\n");
		for (i = 0; i < GRP_HIST_NUM; i++) {
			grp_hist_per_rt0_cnt_t rt0_cnt_zero[4] = {{0, 0}, };
			curidx = ((grp_hist_idx + i + 1) & (GRP_HIST_NUM - 1));

			if (!memcmp(&grp_hist_per_blk[curidx], rt0_cnt_zero,
					sizeof(*rt0_cnt_zero))) {
				continue;
			}
			bcm_bprintf(b, "[%02d]\t", i);
			for (j = 0; j < 4; j++) {
				tx_cnt = grp_hist_per_blk[curidx].rt0_cnt[j].tx_cnt;
				txsucc_cnt = grp_hist_per_blk[curidx].rt0_cnt[j].txsucc_cnt;
				if (tx_cnt) {
					bcm_bprintf(b, "(%03d %03d %d%%)\t",
						tx_cnt, txsucc_cnt,
						((tx_cnt - txsucc_cnt) * 100 / tx_cnt));
				} else {
					bcm_bprintf(b, "(N/A)\t\t");
				}
			}
			bcm_bprintf(b, "\n");
		}
		MFREE(wlc->osh, grp_hist_per_blk, GRP_HIST_PER_BLK_WSZ);

	}
#endif /* WLC_MACDBG_FRAMEID_TRACE */

	MFREE(wlc->osh, grp_hist_blk, GRP_HIST_BLK_WSZ);
#endif /* WL_PSMX */
	return BCME_OK;
}

static int
wlc_dump_ffqgap(wlc_info_t *wlc, struct bcmstrbuf * b)
{
#ifdef WL_PSMX
	ffq_gap_blk_t *ffq_gap_blk;
	uint16 ffq_gap_idx, i, j;
	char *tx_tech_type_str[4] = {"su  ", "vmu ", "hemm", "heom"};

	if (!wlc->clk) {
		return BCME_NOCLK;
	}

	ffq_gap_blk = MALLOCZ(wlc->osh, sizeof(*ffq_gap_blk));
	if (!ffq_gap_blk) {
		return BCME_NOMEM;
	}

	wlc_bmac_copyfrom_objmem(wlc->hw, MX_FFQ_GAP_BLK(wlc), ffq_gap_blk,
		sizeof(*ffq_gap_blk), OBJADDR_SHMX_SEL);
	ffq_gap_idx = ffq_gap_blk->hist_idx;

	bcm_bprintf(b, "(idx) ac tx_tech ffq_gap | ...\n");
	for (i = 0; i < FFQ_GAP_HIST_MAX / 8; i++) {
		for (j = 0; j < 8; j++) {
			uint16 curidx = ((ffq_gap_idx + i + j * 8) & (FFQ_GAP_HIST_MAX - 1));
			bcm_bprintf(b, "(%02d) AC%d %04s %04d | ", i + j * 8,
				FFQ_GAP_HIST_AC(ffq_gap_blk->hist[curidx]),
				tx_tech_type_str[FFQ_GAP_HIST_TX_TECH(ffq_gap_blk->hist[curidx])],
				FFQ_GAP_HIST_GAP_USEC(ffq_gap_blk->hist[curidx]));
		}
		bcm_bprintf(b, "\n");
	}

	bcm_bprintf(b, "\n");
	MFREE(wlc->osh, ffq_gap_blk, sizeof(*ffq_gap_blk));

	return BCME_OK;
#endif /* WL_PSMX */
}
#endif  /* BCMDBG_DUMP_D11CNTS */

/* ************* end of dump_xxx function section *************************** */

/* print upon critical or fatal error */
static void
wlc_suspend_mac_debug(wlc_info_t *wlc, uint32 phydebug)
{
	osl_t *osh;
	wlcband_t *band = wlc->band;
	uint16 val, addr;
	int loop;
	d11print_list_t acphyreg[] = {
		{"fineclk_gatectl", 0x16b},
		{"RfseqStatus0", 0x403},
		{"RfctrlCmd", 0x408}
	};

	BCM_REFERENCE(phydebug);

	/* dump additional regs */
	if (WLCISACPHY(band)) {
		addr = 0x1f0;
	} else if (WLCISNPHY(band)) {
		addr = 0x183;
	} else {
		return;
	}

	osh = wlc->osh;
	for (loop = 0; loop < 10; loop++) {
		W_REG(osh, D11_PHY_REG_ADDR(wlc), addr);
		val = R_REG(osh, D11_PHY_REG_DATA(wlc));
		WL_PRINT(("PHY reg %#x = 0x%x\n", addr, val));
		OSL_DELAY(10);
	}

	if (!WLCISACPHY(band)) {
		return;
	}

	for (loop = 0; loop < ARRAYSIZE(acphyreg); loop ++) {
		W_REG(osh, D11_PHY_REG_ADDR(wlc), acphyreg[loop].addr);
		val = R_REG(osh, D11_PHY_REG_DATA(wlc));
		WL_PRINT(("PHY reg %#x = %#x (%s)\n",
			acphyreg[loop].addr, val, acphyreg[loop].name));
	}

#ifdef WAR4360_UCODE
	W_REG(osh, D11_radioregaddr(wlc), (0x16a | (4<<9)));
	val = R_REG(osh, D11_radioregdata(wlc));
	WL_PRINT(("RADIO reg 0x16a = 0x%x (OVR14 - AFE_PU)\n", val));

	W_REG(osh, D11_radioregaddr(wlc), (0x8e | (4<<9)));
	val = R_REG(osh, D11_radioregdata(wlc));
	WL_PRINT(("RADIO reg 0x8e = 0x%x (pll_adc6 - ADC_PU)\n", val));

	WL_PRINT(("PSM_STACK_STATUS : 0x%x\n", R_REG(osh, D11_SubrStkStatus(wlc))));
	WL_PRINT(("PSM_STACK_ENTRIES:\n"));
	for (loop = 0; loop < 8; loop++) {
		W_REG(osh, D11_SubrStkRdPtr(wlc), loop);
		val = R_REG(osh, D11_SubrStkRdData(wlc));
		WL_PRINT(("0x%04x\n", val));
	}
#endif /* WAR4360_UCODE */
} /* wlc_suspend_mac_debug */

static void
wlc_dump_ucode_fatal_phydebug(wlc_info_t *wlc)
{
	osl_t *osh;
	int i, k;
	uint32 val32[4];
	uint16 val16[4];
	/* Three lists: one common, two corerev specific */
	d11print_list_t *plist[3] = {NULL};
	int lsize[3] = {0};
	/* phyreg */
	d11print_list_t nphyreg[] = {
		{"pktproc", 0x183}, /* repeat four times */
		{"pktproc", 0x183},
		{"pktproc", 0x183},
		{"pktproc", 0x183}
	};
	d11print_list_t acphyreg[] = {
		{"pktproc", 0x1f0}, /* repeat four times */
		{"pktproc", 0x1f0},
		{"pktproc", 0x1f0},
		{"pktproc", 0x1f0}
	};
	d11print_list_t acphy2reg[] = {
		{"TxFifoStatus0", 0x60d},
		{"TxFifoStatus1", 0x80d},
		{"TxFifoStatus2", 0xa0d},
		{"TxFifoStatus3", 0xc0d}
	};
	d11print_list_t acphy3reg[] = {
		{"TxFifoStatus0", 0x60d},
		{"TxFifoStatus1", 0x80d}
	};

	osh = wlc->osh;
	BCM_REFERENCE(val16);
	BCM_REFERENCE(val32);

	WL_PRINT(("phydebug :\n"));
	for (k = 0; k < 32; k += 4) {
		val32[0] = R_REG(osh, D11_PHY_DEBUG(wlc));
		val32[1] = R_REG(osh, D11_PHY_DEBUG(wlc));
		val32[2] = R_REG(osh, D11_PHY_DEBUG(wlc));
		val32[3] = R_REG(osh, D11_PHY_DEBUG(wlc));
		WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[1], val32[2], val32[3]));
	}

	if (WLCISACPHY(wlc->band) || WLCISHECAPPHY(wlc->band)) {
		plist[0] = acphyreg;
		lsize[0] = ARRAYSIZE(acphyreg);
	} else if (WLCISNPHY(wlc->band)) {
		plist[0] = nphyreg;
		lsize[0] = ARRAYSIZE(nphyreg);
	} else {
		lsize[0] = 0;
	}

	lsize[1] = 0;
	lsize[2] = 0;

	/* acphy2 debug info */
	if (WLCISACPHY(wlc->band) && ACREV_GE(wlc->band->phyrev, 44)) {
		plist[1] = acphy3reg;
		lsize[1] = ARRAYSIZE(acphy3reg);
	} else if (WLCISACPHY(wlc->band) && (ACREV_GE(wlc->band->phyrev, 32))) {
		plist[1] = acphy2reg;
		lsize[1] = ARRAYSIZE(acphy2reg);
	}

	for (i = 0; i < 3; i ++) {
		for (k = 0; k < lsize[i]; k++) {
			W_REG(osh, D11_PHY_REG_ADDR(wlc), plist[i][k].addr);
			val16[0] = R_REG(osh, D11_PHY_REG_DATA(wlc));
			WL_PRINT(("%-12s 0x%-4x ", plist[i][k].name, val16[0]));
			if ((k % 4) == 3)
				WL_PRINT(("\n"));
		}
		if (k % 4) {
			WL_PRINT(("\n"));
		}
	}

	/* acphy2 debug info2 */
	if (WLCISACPHY(wlc->band) && (ACREV_GE(wlc->band->phyrev, 32))) {
		uint8 gpiosel = 2;
		d11print_list_t acphy2_gpioregs[] = {
			{"gpiosel", 0x394},
			{"gpiolo", 0x12},
			{"gpiohi", 0x13}
		};

		for (i = 0; i < gpiosel; i ++) {
			/* select gpio */
			W_REG(osh, D11_PHY_REG_ADDR(wlc), acphy2_gpioregs[0].addr);
			W_REG(osh, D11_PHY_REG_DATA(wlc), (uint16)i);

			for (k = 0; k < ARRAYSIZE(acphy2_gpioregs); k++) {
				W_REG(osh, D11_PHY_REG_ADDR(wlc), acphy2_gpioregs[k].addr);
				val16[0] = R_REG(osh, D11_PHY_REG_DATA(wlc));
				WL_PRINT(("%-12s 0x%-4x ",
					acphy2_gpioregs[k].name, val16[0]));
			}
			WL_PRINT(("\n"));
		}
	}
} /* wlc_dump_ucode_fatal_phydebug */

void
wlc_dump_ucode_fatal(wlc_info_t *wlc, uint reason)
{
	wlc_pub_t *pub;
	osl_t *osh;
	uint32 phydebug, psmdebug, pc;
	uint16 val16[5];
	uint32 val32[4];
#if defined(WL_PSMR1)
	uint32 val132[4];
#endif
	int i, k;
	volatile uint8 *paddr;
	/* Three lists: one common, two corerev specific */
	d11print_list_t *plist[3] = {NULL};
	int lsize[3] = {0};
	int idx_d11_reg_list;	/**< index into plist[] and lsize[] */

	/* common list */
	d11print_list_t cmlist[] = {
		{"ifsstat", D11_IFS_STAT_OFFSET(wlc)},
		{"ifsstat1", D11_IFS_STAT1_OFFSET(wlc)},
		{"txectl", D11_TXE_CTL_OFFSET(wlc)},
		{"txestat", D11_TXE_STATUS_OFFSET(wlc)},
		{"rxestat1", D11_RXE_STATUS1_OFFSET(wlc)},
		{"rxestat2", D11_RXE_STATUS2_OFFSET(wlc)},
		{"rcv_frmcnt", D11_RCV_FRM_CNT_OFFSET(wlc)},
		{"rxe_rxcnt", D11_RXE_RXCNT_OFFSET(wlc)},
		{"wepctl", D11_WEP_CTL_OFFSET(wlc)},
	};
	/* reg specific to corerev < 40 */
	d11print_list_t list_lt40[] = {
		{"xmtfifordy", D11_xmtfifordy_OFFSET(wlc)},
		{"pcmctl", D11_pcmctl_OFFSET(wlc)},
		{"pcmstat", D11_pcmstat_OFFSET(wlc)},
	};
	/* reg specific to corerev >= 40 */
	d11print_list_t list_ge40[] = {
		{"aqmfifordy", D11_AQMFifoReady_OFFSET(wlc)},
		{"wepstat", D11_WEP_STAT_OFFSET(wlc)},
		{"wep_ivloc", D11_WEP_HDRLOC_OFFSET(wlc)},
		{"wep_psdulen", D11_WEP_PSDULEN_OFFSET(wlc)},
	};
	/* reg specific to corerev >=64 and < 80 */
	d11print_list_t list_ge64[] = {
		{"wepstat", D11_WEP_STAT_OFFSET(wlc)},
		{"wep_ivloc", D11_WEP_HDRLOC_OFFSET(wlc)},
		{"wep_psdulen", D11_WEP_PSDULEN_OFFSET(wlc)},
		{"aqmfifordyL", D11_AQMFifoRdy_L_OFFSET(wlc)},
		{"aqmfifordyH", D11_AQMFifoRdy_H_OFFSET(wlc)},
		{"rxe_errval", D11_RXE_ERRVAL_OFFSET(wlc)},
		{"rxe_errmask", D11_RXE_ERRMASK_OFFSET(wlc)},
		{"rxe_status3", D11_RXE_STATUS3_OFFSET(wlc)},
		{"txestat2", D11_TXE_STATUS2_OFFSET(wlc)},
	};
	/* reg specific to corerev >= 128 */
	d11print_list_t list_ge128[] = {
		{"aqmf_ready0", D11_AQMF_READY0_OFFSET(wlc)},
		{"aqmf_ready1", D11_AQMF_READY1_OFFSET(wlc)},
		{"aqmf_ready2", D11_AQMF_READY2_OFFSET(wlc)},
		{"aqmf_ready3", D11_AQMF_READY3_OFFSET(wlc)},
		{"aqmf_ready4", D11_AQMF_READY4_OFFSET(wlc)},
		{"wepstat", D11_WEP_STAT_OFFSET(wlc)},
		{"wep_ivloc", D11_WEP_HDRLOC_OFFSET(wlc)},
		{"wep_psdulen", D11_WEP_PSDULEN_OFFSET(wlc)},
		{"rxe_errval", D11_RXE_ERRVAL_OFFSET(wlc)},
		{"rxe_errmask", D11_RXE_ERRMASK_OFFSET(wlc)},
		{"rxe_status3", D11_RXE_STATUS3_OFFSET(wlc)},
		{"txe_status2", D11_TXE_STATUS_OFFSET(wlc)},
		{"dbg_bmc_status", D11_TXE_DBG_BMC_STATUS_OFFSET(wlc)},
		{"psm_chk0_err", D11_PSM_CHK0_ERR_OFFSET(wlc)},
		{"psm_chk1_err", D11_PSM_CHK1_ERR_OFFSET(wlc)},
	};

	/* reg specific to corerev >= 130 */
	d11print_list_t list_ge130[] = {
		{"tx_prebm_fatal_errval", D11_TX_PREBM_FATAL_ERRVAL_OFFSET(wlc)},
		{"txe_bmc_errsts", D11_TXE_BMC_ERRSTS_OFFSET(wlc)},
		{"xmtdma_fatal_err", D11_XMTDMA_FATAL_ERR_OFFSET(wlc)},
	};

	pub = wlc->pub;
	osh = wlc->osh;
	BCM_REFERENCE(val16);
	BCM_REFERENCE(val32);

	/* D11 dma descriptor errors may result in a PSM WD. However, these descriptor errors are
	 * detected in a 1-second timer loop, and thus may be detected later than a PSM WD. To give
	 * the developer a clue that a descriptor error might have been the cause of this PSM WD
	 * occurring, any descriptor error is printed prior to the ucode dump.
	 */
	wlc_bmac_detect_and_handle_fifoerrors(wlc->hw);

	WL_PRINT(("wl%d: reason = ", pub->unit));
	switch (reason) {
		case PSM_FATAL_PSMWD:
			WL_PRINT(("psm watchdog"));
			break;
		case PSM_FATAL_SUSP:
			WL_PRINT(("mac suspend failure"));
			break;
		case PSM_FATAL_WAKE:
			WL_PRINT(("fail to wake up"));
			break;
		case PSM_FATAL_TXSUFL:
			WL_PRINT(("txfifo susp/flush"));
			break;
		case PSM_FATAL_PSMXWD:
			WL_PRINT(("psmx watchdog"));
			break;
		case PSM_FATAL_TXSTUCK:
			WL_PRINT(("txstuck"));
			break;
		default:
			WL_PRINT(("any"));

	}

	if (pub->corerev_minor == 0) {
		WL_PRINT((" at %d seconds. corerev %d ", pub->now, pub->corerev));
	} else {
		WL_PRINT((" at %d seconds. corerev %d.%d ",
			pub->now, pub->corerev, pub->corerev_minor));
	}

	if (!wlc->clk) {
		WL_PRINT(("%s: no clk\n", __FUNCTION__));
		return;
	}

	WL_PRINT(("ucode revision %d.%d features 0x%04x\n",
		wlc_read_shm(wlc, M_BOM_REV_MAJOR(wlc)), wlc_read_shm(wlc, M_BOM_REV_MINOR(wlc)),
		wlc_read_shm(wlc, M_UCODE_FEATURES(wlc))));

	pc = psmdebug = R_REG(osh, D11_PSM_DEBUG(wlc));
	if (D11REV_GE(pub->corerev, 128)) {
		if (reason == PSM_FATAL_PSMWD && !(pc && psmdebug)) {
			psmdebug = wlc_read_shm(wlc, M_PSMWDS_PC(wlc));
			pc = R_REG(osh, D11_PSM_DEBUG(wlc));
		} else {
			// MAC doesn't have PSMWD fired, So read from PC stack regs directly
			k = R_REG(osh, D11_SubrStkStatus(wlc)) & ((1 << C_SRSPTR_BSZ) - 1);
			W_REG(osh, D11_SubrStkRdPtr(wlc), (uint16)k);
			pc = psmdebug = R_REG(osh, D11_SubrStkRdData(wlc));
		}
	}

	phydebug = R_REG(osh, D11_PHY_DEBUG(wlc));
	val32[0] = R_REG(osh, D11_MACCONTROL(wlc));
	val32[1] = R_REG(osh, D11_MACCOMMAND(wlc));
	val16[0] = R_REG(osh, D11_PSM_BRC_0(wlc));
	val16[1] = R_REG(osh, D11_PSM_BRC_1(wlc));
	val16[2] = wlc_read_shm(wlc, M_UCODE_DBGST(wlc));

	wlc_mac_event(wlc, WLC_E_PSM_WATCHDOG, NULL, psmdebug, phydebug, val16[0], NULL, 0);
	WL_PRINT(("psmdebug 0x%08x pc 0x%08x phydebug 0x%x macctl 0x%x maccmd 0x%x\n"
		 "psm_brc 0x%04x psm_brc_1 0x%04x M_UCODE_DBGST 0x%x\n",
		 psmdebug, pc, phydebug, val32[0], val32[1], val16[0], val16[1], val16[2]));

#if defined(WL_PSMR1)
	if (PSMR1_ENAB(wlc->hw)) {
		psmdebug = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
		val32[0] = R_REG(osh, D11_MACCONTROL_r1(wlc));
		val32[1] = R_REG(osh, D11_MACCOMMAND_R1(wlc));
		val16[0] = wlc_read_shm1(wlc, M_UCODE_DBGST(wlc));

		WL_PRINT(("PSMR1: psmdebug 0x%08x macctl 0x%x maccmd 0x%x M_UCODE_DBGST 0x%x\n",
			psmdebug, val32[0], val32[1], val16[0]));
	}
#endif /* WL_PSMR1 */

	wlc_bmac_mctrl(wlc->hw, MCTL_EN_PSMDBG, MCTL_EN_PSMDBG);

	val16[0] = R_REG(osh, D11_PSM_BRWK_0(wlc));
	val16[1] = R_REG(osh, D11_PSM_BRWK_1(wlc));
	val16[2] = R_REG(osh, D11_PSM_BRWK_2(wlc));
	val16[3] = R_REG(osh, D11_PSM_BRWK_3(wlc));
	val16[4] = D11REV_GE(pub->corerev, 129) ? R_REG(osh, D11_PSM_BRWK_4(wlc)) : 0;

	wlc_bmac_mctrl(wlc->hw, MCTL_EN_PSMDBG, 0);

	WL_PRINT(("brwk_0 0x%04x brwk_1 0x%04x "
		"brwk_2 0x%04x brwk_3 0x%04x brwk_4 0x%04x\n",
		val16[0], val16[1], val16[2], val16[3], val16[4]));

	// skip redo of generating mac dumps if already generated once
	if ((wlc->macdbg->log_done) != 0) {
		WL_PRINT(("%s: log_done %#x\n", __FUNCTION__, wlc->macdbg->log_done));
		return;
	}
#if defined(BCMDBG)
	_wlc_macdbg_dtrace_print_buf(wlc->macdbg);
#endif
	paddr = (volatile uint8*)(D11_biststatus(wlc)) - 0xC;
	idx_d11_reg_list = 0;
	plist[idx_d11_reg_list] = cmlist;
	lsize[idx_d11_reg_list++] = ARRAYSIZE(cmlist);

	if (D11REV_GE(pub->corerev, 128)) {
		plist[idx_d11_reg_list] = list_ge128;
		lsize[idx_d11_reg_list++] = ARRAYSIZE(list_ge128);
	} else if (D11REV_GE(pub->corerev, 64)) {
		plist[idx_d11_reg_list] = list_ge64;
		lsize[idx_d11_reg_list++] = ARRAYSIZE(list_ge64);
	} else if (D11REV_GE(pub->corerev, 40)) {
		plist[idx_d11_reg_list] = list_ge40;
		lsize[idx_d11_reg_list++] = ARRAYSIZE(list_ge40);
	} else {
		plist[idx_d11_reg_list] = list_lt40;
		lsize[idx_d11_reg_list++] = ARRAYSIZE(list_lt40);
	}

	if (D11REV_GE(pub->corerev, 130)) {
		plist[idx_d11_reg_list] = list_ge130;
		lsize[idx_d11_reg_list++] = ARRAYSIZE(list_ge130);
	}

	for (i = 0; i < idx_d11_reg_list; i++) {
		if (plist[i] == NULL)
			continue;
		for (k = 0; k < lsize[i]; k++) {
			val16[0] = R_REG(osh, (volatile uint16*)(paddr + plist[i][k].addr));
			WL_PRINT(("%-12s 0x%-4x ", plist[i][k].name, val16[0]));
			if ((k % 4) == 3)
				WL_PRINT(("\n"));
		}
		if (k % 4) {
			WL_PRINT(("\n"));
		}
	}

	if (AMPDU_MAC_ENAB(pub))
		wlc_dump_aggfifo(wlc, NULL);

	WL_PRINT(("PC :\n"));
	if (D11REV_GE(pub->corerev, 128)) {
		val32[0] = psmdebug;
		WL_PRINT(("R0: 0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[0], val32[0], val32[0]));
	}

	for (k = 0; k < 64; k += 4) {
		val32[0] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[1] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[2] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[3] = R_REG(osh, D11_PSM_DEBUG(wlc));
		if (D11REV_LT(pub->corerev, 128)) {
			WL_PRINT(("R0: 0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
				val32[0], val32[1], val32[2], val32[3]));
		}

#if defined(WL_PSMR1)
		if (PSMR1_ENAB(wlc->hw)) {
			val132[0] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
			val132[1] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
			val132[2] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
			val132[3] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
			WL_PRINT(("R1: 0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
				val132[0], val132[1], val132[2], val132[3]));
		}
#endif /* WL_PSMR1 */
	}

	if (phydebug > 0) {
		wlc_dump_ucode_fatal_phydebug(wlc);
	} /* phydebug */

	WL_PRINT(("psm stack_status : 0x%x\n", R_REG(osh, D11_SubrStkStatus(wlc))));
	WL_PRINT(("psm stack_entries:\n"));
	for (k = 0; k < (1 << C_SRSPTR_BSZ); k++) {
		W_REG(osh, D11_SubrStkRdPtr(wlc), (uint16)k);
		val16[0] = R_REG(osh, D11_SubrStkRdData(wlc));
		WL_PRINT(("0x%-8x ", val16[0]));
		if ((k+1)%4 == 0) {
			WL_PRINT(("\n"));
		}
	}

	k = (reason == PSM_FATAL_SUSP) ? 1 : 0;
#ifdef WAR4360_UCODE
	k = (reason == PSM_FATAL_WAKE) ? 1 : k;
#endif
	if (k) {
		wlc_suspend_mac_debug(wlc, phydebug);
	}

	wlc->macdbg->log_done |= (1 << reason);

#if defined(BCMDBG)
	wlc->psm_watchdog_debug = TRUE;
	wlc_macdbg_dump_dma(wlc);
	wlc->psm_watchdog_debug = FALSE;
#endif

	wlc_twt_dump_schedblk(wlc);
} /* wlc_dump_ucode_fatal */

#if defined(WL_PSMX)
#ifdef WLVASIP
void
wlc_dump_vasip_fatal(wlc_info_t *wlc)
{
#if defined(SVMP_ACCESS_VIA_PHYTBL)
	wlc_hw_info_t *wlc_hw = wlc->hw;
	uint16 i, m, n, c[4], s[4];
	uint32 address[18] = {0x20000, 0x20060, 0x20080, 0x21e10, 0x20300, 0x20700, 0x20b00,
		0x20bff, 0xc000, 0xe000, 0x10000, 0x12000, 0x14000, 0x16000, 0x18000, 0x1a000,
		0x1c000, 0x1e000};
	uint16 size[18] = {16, 1, 1, 16, 16, 16, 16, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
	uint16 counter[93];
	uint16 report_idx[10] = {64, 65, 0, 1, 2, 3, 4, 5, 6, 7};

	/*
	 * name           = address, index in array
	 * counter        = 0x20000, 0;
	 * fix_mcs        = 0x20060, 16;
	 * error_code     = 0x20080, 17;
	 * mcs            = 0x21e10, 18;
	 * m2v0           = 0x20300, 34;
	 * m2v1           = 0x20700, 50;
	 * v2m            = 0x20b00, 66;
	 * transfer_done  = 0x20bff, 82;
	 * header0        = 0xc000, 83;
	 * header1        = 0xe000, 84;
	...
	*/
#endif /* SVMP_ACCESS_VIA_PHYTBL */

	if (!wlc->clk) {
		WL_PRINT(("%s: no clk\n", __FUNCTION__));
		return;
	}

#if defined(SVMP_ACCESS_VIA_PHYTBL)
	WL_PRINT(("VASIP watchdog is triggered. VASIP code revision %d.%d \n",
		d11vasipcode_major, d11vasipcode_minor));

	n = 0;
	for (i = 0; i < 18; i++) {
		for (m = 0; m < size[i]; m++) {
			phy_vasip_read_svmp(wlc_hw->band->pi,
				address[i]+m, &counter[n++]);
		}
	}

	for (i = 0; i < 4; i++) {
		s[i] = ((counter[18+i] & 0xf0) >> 4) + 1;
		c[i] = counter[18+i] & 0xf;
	}

	WL_PRINT(("Received Interrupts:\n"
		"      bfr_module_done:0x%x     bfe_module_done:0x%x     bfe_imp_done:0x%x\n"
			"      m2v_transfer_done:0x%x   v2m_transfder_done:0x%x\n"
			"Fixed MCS: %d\n"
			"Number of users: %x\n"
			"Last steered MCS:\n"
			"      user0:c%ds%d;   user1:c%ds%d;   user2:c%ds%d;   user3:c%ds%d\n"
			"Last precoder SNR:\n"
			"      user0:0x%x;   user1:0x%x;   user2:0x%x;   user3:0x%x\n"
			"Error code is 0x%x\n"
			"v2m transfer is done: %d\n",
			counter[2], counter[3], counter[4], counter[8], counter[9], counter[16],
			counter[27], c[0], s[0], c[1], s[1], c[2], s[2], c[3], s[3], counter[23]/4,
			counter[24]/4, counter[25]/4, counter[26]/4, counter[17], counter[82]));

	WL_PRINT(("m2v0 message:\n"));
	for (i = 0; i < 4; i++)
		WL_PRINT(("0x%04x 0x%04x 0x%04x 0x%04x\n", counter[34+i*4], counter[35+4*i],
				counter[36+4*i], counter[37+4*i]));

	WL_PRINT(("m2v1 message:\n"));
	for (i = 0; i < 4; i++)
		WL_PRINT(("0x%04x 0x%04x 0x%04x 0x%04x\n", counter[50+i*4], counter[51+4*i],
				counter[52+4*i], counter[53+4*i]));

	WL_PRINT(("v2m message:\n"));
	for (i = 0; i < 4; i++)
		WL_PRINT(("0x%04x 0x%04x 0x%04x 0x%04x\n", counter[66+i*4], counter[67+4*i],
				counter[68+4*i], counter[69+4*i]));

	for (i = 0; i < 10; i++)
		WL_PRINT(("index %d, addr 0x%04x, header: 0x%04x\n", report_idx[i],
				address[8+i], counter[i+83]));
#endif /* SVMP_ACCESS_VIA_PHYTBL */
} /* wlc_dump_vasip_fatal */
#endif /* WLVASIP */

void
wlc_dump_psmx_fatal(wlc_info_t *wlc, uint reason)
{
	osl_t *osh;
	wlc_pub_t *pub;
	uint32 val32[4];
	uint32 psmdebug_x, pc_x;
#ifdef WLVASIP
	uint16 val16;
#endif
	uint16 k;

	const char reason_str[][20] = {
		"any failure",
		"watchdog",
		"suspend failure",
		"txstuck",
	};

	osh = wlc->osh;
	pub = wlc->pub;

	k = (reason >= PSMX_FATAL_LAST) ? PSMX_FATAL_ANY : reason;

	if (pub->corerev_minor == 0) {
		WL_PRINT(("wl%d: PSMx dump at %d seconds. corerev %d reason:%s ",
			pub->unit, pub->now, pub->corerev, reason_str[k]));
	} else {
		WL_PRINT(("wl%d: PSMx dump at %d seconds. corerev %d.%d reason:%s ",
			pub->unit, pub->now, pub->corerev, pub->corerev_minor, reason_str[k]));
	}

	if (!wlc->clk) {
		WL_PRINT(("%s: no clk\n", __FUNCTION__));
		return;
	}

	WL_PRINT(("ucode revision %d.%d features 0x%04x\n",
		wlc_read_shmx(wlc, MX_BOM_REV_MAJOR(wlc)),
		wlc_read_shmx(wlc, MX_BOM_REV_MINOR(wlc)),
		wlc_read_shmx(wlc, MX_UCODE_FEATURES(wlc))));

	wlc_mac_event(wlc, WLC_E_PSM_WATCHDOG, NULL, R_REG(osh, D11_PSM_DEBUG_psmx(wlc)),
		0, wlc_read_macregx(wlc, 0x490), NULL, 0);
	if (D11REV_GE(pub->corerev, 128)) {
		if (reason == PSM_FATAL_PSMXWD) {
			psmdebug_x = wlc_read_shm(wlc, MX_PSMWDS_PC(wlc));
			pc_x = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
		} else {
			// MAC doesn't have PSMXWD fired, So read from PC stack regs directly
			k = wlc_read_macregx(wlc,
				((uint16)(long)D11_SubrStkStatus(wlc))) & ((1<<C_SRSPTR_BSZ)-1);
			wlc_write_macregx(wlc, ((uint16) (long) D11_SubrStkRdPtr(wlc)), k);
			psmdebug_x = wlc_read_macregx(wlc,
					((uint16) (long) D11_SubrStkRdData(wlc)));
			pc_x = psmdebug_x;
		}
	} else {
		pc_x = psmdebug_x = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
	}

	WL_PRINT(("psmxdebug 0x%08x pc_x 0x%08x macctl_x 0x%x maccmd_x 0x%x\n"
		"psmx_brc 0x%04x psmx_brc_1 0x%04x MX_UCODE_DBGST 0x%x\n",
		psmdebug_x,
		pc_x,
		R_REG(osh, D11_MACCONTROL_psmx(wlc)),
		R_REG(osh, D11_MACCOMMAND_psmx(wlc)),
		wlc_read_macregx(wlc, 0x490),
		wlc_read_macregx(wlc, 0x4d8),
		wlc_read_shmx(wlc, MX_UCODE_DBGST(wlc))));

	if ((wlc->macdbg->log_done) != 0) {
		WL_PRINT(("%s: log_done %#x\n", __FUNCTION__, wlc->macdbg->log_done));
		return;
	}

#if defined(BCMDBG)
	_wlc_macdbg_dtrace_print_buf(wlc->macdbg);
#endif
	wlc->macdbg->log_done |= (1 << PSM_FATAL_PSMXWD);
	WL_PRINT(("PC (psmdebug_x):\n"));
	if (D11REV_GE(pub->corerev, 128)) {
		val32[0] = psmdebug_x;
		WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[0], val32[0], val32[0]));
	} else {
		for (k = 0; k < 64; k += 4) {
			val32[0] = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
			val32[1] = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
			val32[2] = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
			val32[3] = R_REG(osh, D11_PSM_DEBUG_psmx(wlc));
			WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
				val32[0], val32[1], val32[2], val32[3]));
		}
	}

	WL_PRINT(("PC (psmdebug):\n"));
	for (k = 0; k < 8; k += 4) {
		val32[0] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[1] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[2] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[3] = R_REG(osh, D11_PSM_DEBUG(wlc));
		WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[1], val32[2], val32[3]));
	}

	WL_PRINT(("PSMX_STACK_STATUS : 0x%x\n",
		wlc_read_macregx(wlc, ((uint16) (long) D11_SubrStkStatus(wlc)))));
	WL_PRINT(("PSMX_STACK_ENTRIES:\n"));
	for (k = 0; k < (1 << C_SRSPTR_BSZ); k++) {
		wlc_write_macregx(wlc, ((uint16) (long) D11_SubrStkRdPtr(wlc)), k);
		WL_PRINT(("0x%-8x ",
			wlc_read_macregx(wlc, ((uint16) (long) D11_SubrStkRdData(wlc)))));
		if ((k+1)%4 == 0) {
			WL_PRINT(("\n"));
		}
	}

#ifdef WLVASIP
	/* bit5 of txe_vasip_intsts indicates vasip watchdog is triggered */
	val16 = wlc_read_macregx(wlc, 0x870);
	WL_PRINT(("txe_vasip_intsts %#x\n", val16));
	if (val16 & (1 << 5)) {
		wlc_dump_vasip_fatal(wlc);
	}
#endif	/* WLVASIP */

#if defined(BCMDBG)
	wlc->psm_watchdog_debug = TRUE;
	wlc_macdbg_dump_dma(wlc);
	wlc->psm_watchdog_debug = FALSE;
#endif
} /* wlc_dump_psmx_fatal */
#endif /* WL_PSMX */

#if defined(WL_PSMR1)
void
wlc_dump_psmr1_fatal(wlc_info_t *wlc, uint reason)
{
	osl_t *osh;
	wlc_pub_t *pub;
	uint32 val32[4];
	uint16 val16;
	uint16 k;
	const char reason_str[][20] = {
		"any failure",
		"watchdog",
		"suspend failure",
	};

	osh = wlc->osh;
	pub = wlc->pub;

	k = (reason >= PSM_FATAL_LAST) ? PSM_FATAL_ANY : reason;

	if (pub->corerev_minor == 0) {
		WL_PRINT(("wl%d: PSMR1 dump at %d seconds. corerev %d reason:%s ",
			pub->unit, pub->now, pub->corerev, reason_str[k]));
	} else {
		WL_PRINT(("wl%d: PSMR1 dump at %d seconds. corerev %d.%d reason:%s ",
			pub->unit, pub->now, pub->corerev, pub->corerev_minor, reason_str[k]));
	}

	if (!wlc->clk) {
		WL_PRINT(("%s: no clk\n", __FUNCTION__));
		return;
	}

	WL_PRINT(("ucode revision %d.%d features 0x%04x\n",
		wlc_read_shm(wlc, M_BOM_REV_MAJOR(wlc)), wlc_read_shm(wlc, M_BOM_REV_MINOR(wlc)),
		wlc_read_shm(wlc, M_UCODE_FEATURES(wlc))));

	WL_PRINT(("psmdebug 0x%08x macctl 0x%x maccmd 0x%x\n"
		 "psm_brc 0x%04x psm_brc_1 0x%04x M_UCODE_DBGST 0x%x\n",
		  R_REG(osh, D11_PSM_DEBUG_R1(wlc)),
		  R_REG(osh, D11_MACCONTROL_r1(wlc)),
		  R_REG(osh, D11_MACCOMMAND_R1(wlc)),
		  wlc_read_macreg1(wlc, 0x490), // Not sure about the addresses **TBD**
		  wlc_read_macreg1(wlc, 0x4d8),
		  wlc_read_shm1(wlc, M_UCODE_DBGST(wlc))));

	if ((wlc->macdbg->log_done) != 0) {
		WL_PRINT(("%s: log_done %#x\n", __FUNCTION__, wlc->macdbg->log_done));
		return;
	}
#if defined(BCMDBG)
	_wlc_macdbg_dtrace_print_buf(wlc->macdbg);
#endif
	wlc->macdbg->log_done |= (1 << PSM_FATAL_PSMXWD);
	WL_PRINT(("PC (psmdebug_r1):\n"));
	for (k = 0; k < 64; k += 4) {
		val32[0] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
		val32[1] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
		val32[2] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
		val32[3] = R_REG(osh, D11_PSM_DEBUG_R1(wlc));
		WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[1], val32[2], val32[3]));
	}

	WL_PRINT(("PC (psmdebug):\n"));
	for (k = 0; k < 8; k += 4) {
		val32[0] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[1] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[2] = R_REG(osh, D11_PSM_DEBUG(wlc));
		val32[3] = R_REG(osh, D11_PSM_DEBUG(wlc));
		WL_PRINT(("0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
			val32[0], val32[1], val32[2], val32[3]));
	}

	val16 = wlc_read_macreg1(wlc, 0x4d0); /* psm_srs_status */
	WL_PRINT(("psmr1 stack_status : 0x%x\n", val16));
	WL_PRINT(("psmr1 stack_entries:\n"));
	for (k = 0; k < 8; k++) {
		wlc_write_macreg1(wlc, 0x4d2, k); /* psm_srs_ptr */
		val16 = wlc_read_macreg1(wlc, 0x4d4); /* psm_srs_entry */
		WL_PRINT(("0x%04x\n", val16));
	}
} /* wlc_dump_psmr1_fatal */
#endif /* WL_PSMR1 */

#ifdef DONGLEBUILD
static uint16
wlc_macdbg_shm_dump_size(wlc_info_t *wlc);
static int
wlc_macdbg_dump_mac_ucode_binary(wlc_info_t *wlc, uchar *p);
static int
wlc_macdbg_dump_pc_binary(wlc_info_t *wlc, uchar *p);
/* The dumps created by this function are stored in, and overwrites rodata.
 * Make sure all functions in this code path do not reference rodata.
 * Do not use printfs.  Format strings are stored in rodata.
 * Do not use switch statements.  They may generate lookup tables in rodata.
 */
#if defined(WLC_SRAMPMAC)
static void
wlc_fatal_error_phyrad_dump(wlc_info_t *wlc, uint8 type)
{
	bcm_xtlv_t *p;
	int phyrad_size;
	uint16 req_size;
	uint32 allocated_size = 0;

	phyrad_size = phy_dbg_dump_getlistandsize(WLC_PI(wlc), type);

	/* Check if the the size check returned an error */
	if (phyrad_size > 0) {
		req_size = bcm_xtlv_size_for_data(phyrad_size, BCM_XTLV_OPTION_ALIGN32);
		/* try to get a buffer */
		p = (bcm_xtlv_t *)OSL_GET_FATAL_LOGBUF(wlc->osh, req_size, &allocated_size);
		if ((p != NULL) && (allocated_size >= req_size)) {

			bcm_xtlv_pack_xtlv(p, type,
				allocated_size - bcm_xtlv_hdr_size(BCM_XTLV_OPTION_ALIGN32),
				NULL, BCM_XTLV_OPTION_ALIGN32);

			/* store the information in Raw Data format */
			phy_dbg_dump_tbls(WLC_PI(wlc), type, p->data,
				(int *) &allocated_size, TRUE);
		}
	}
}
#endif /* WLC_SRAMPMAC */

/* dongle routine to handle fatal errors */
void
wlc_handle_fatal_error_dump(wlc_info_t *wlc)
{
	bcm_xtlv_t *p;
	uint32 allocated_size = 0;
	uint16 req_size;

	if (!(wlc->pub->up && wlc->clk)) {
		WL_ERROR(("ucode binary dump not executed\n"));
		return;
	}

	if (wlc->fatal_error_dump_done == TRUE)
		return;

#ifdef WL_UTRACE
	/*
	 * Capture the UTrace related info from the template RAM filled by ucode.
	 */
	allocated_size = 0;
	req_size = T_UTRACE_TPL_RAM_SIZE_BYTES+ID_LEN_SIZE;
	req_size = ALIGN_SIZE(req_size, 4);
	req_size += BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32);
	p = (bcm_xtlv_t *)OSL_GET_FATAL_LOGBUF(wlc->osh, req_size, &allocated_size);

	if ((p != NULL) && (allocated_size >= req_size))
	{
		/* add tag and len , each 2 bytes wide Chipc core index */
		p->id = TAG_TYPE_UTRACE;
		p->len = allocated_size - BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32);
		/* store the information in Raw Data format */
		wlc_dump_utrace_logs(wlc, p->data);
	}
#endif /* WL_UTRACE */

	/* Dump d11 PC registers */
	req_size = BCM_BUF_SIZE_FOR_PC_DUMP+ID_LEN_SIZE;
	req_size = bcm_xtlv_size_for_data(req_size, BCM_XTLV_OPTION_ALIGN32);

	/* try to get a buffer */
	p = (bcm_xtlv_t *)OSL_GET_FATAL_LOGBUF(wlc->osh, req_size, &allocated_size);
	if ((p != NULL) && (allocated_size >= req_size)) {
		uint8 *p_data = p->data;
		/* add tag and len , each 2 bytes wide PC index */
		p->id = TAG_TYPE_PC; /* 5 - PC */
		p->len = allocated_size - BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32);
		/* store the information in Raw Data format */
		wlc_macdbg_dump_pc_binary(wlc, p_data);
	}

	req_size = BCM_BUF_SIZE_FOR_UCODE_DUMP;
	req_size = bcm_xtlv_size_for_data(req_size,  BCM_XTLV_OPTION_ALIGN32);

	/* try to get a buffer */
	p = (bcm_xtlv_t *)OSL_GET_FATAL_LOGBUF(wlc->osh, req_size, &allocated_size);
	if ((p != NULL) && (allocated_size >= req_size)) {
		uint8 *p_data = p->data;
		/* add tag and len , each 2 bytes wide D11 core index */
		p->id = TAG_TYPE_MAC; /* 1: use si_coreidx */
		p->len = allocated_size - BCM_XTLV_HDR_SIZE_EX(BCM_XTLV_OPTION_ALIGN32);
		/* store the information in Raw Data format */
		wlc_macdbg_dump_mac_ucode_binary(wlc, p_data);
	}
#if defined(WLC_SRAMPMAC)
	/*  phy and radio register binary dump */
	wlc_fatal_error_phyrad_dump(wlc, TAG_TYPE_PHY);
	wlc_fatal_error_phyrad_dump(wlc, TAG_TYPE_RAD);
#endif /* WLC_SRAMPMAC */
	wlc->fatal_error_dump_done = TRUE;
} /* wlc_handle_fatal_error_dump */

int
wlc_macdbg_dump_mac_ucode_binary(wlc_info_t *wlc, uchar *p)
{
	const d11regs_list_t *pregs;
	uint i;
	volatile uint8 *paddr; /**< byte pointer to the start of d11 enum space */
	uint32	val_32;
	uint16	val_16;
	uint16	byte_offset;
	const d11dbg_list_t * d11dbg1_tmp = (const d11dbg_list_t *)wlc->macdbg->pd11regs;
	uint array_size = wlc->macdbg->d11regs_sz;
	uint32 l_bitmap;
	uint16 l_cnt;

	/* To get to the core address of d11 core reg list this is being done
	  * there is no regs which point to the base address as of now. And D11_biststatus
	  * register points always 12 bytes ahead of d11 core.
	  */
	paddr = (volatile uint8*)D11_biststatus(wlc) - 0xC;

	if (!wlc->clk)
		return BCME_NOCLK;

	*(uint16 *)p = UCODE_CORE_ID(wlc->pub->unit);
	*(uint16 *)(p + 2) = (uint16) array_size;

	p += 4;
	for (i = 0; i < array_size; i++)
	{
		pregs = (const d11regs_list_t*)&(d11dbg1_tmp[i].reglist);
		/* 0xFFFF is  a flag, which will be present in reglist structure
		  * provided by ucode team. If this flag is absent in the structure,
		  * we go ahead and find out how many SHM registers are
		  *present for this chip
		  */
		bcopy(pregs, p, sizeof(d11regs_list_t));
		if ((pregs->type == D11REG_TYPE_SHM) && (pregs->cnt == 0xFFFF)) {
			l_cnt = wlc_macdbg_shm_dump_size(wlc) / pregs->step;
			p += sizeof(d11regs_list_t) - sizeof(pregs->cnt);
			*(uint16 *)(p) = l_cnt;
			p += sizeof(pregs->cnt);
		} else {
			l_cnt = pregs->cnt;
			p += sizeof(d11regs_list_t);
		}

		byte_offset = pregs->byte_offset;
		l_bitmap = pregs->bitmap;
		while (l_bitmap || l_cnt) {
			if ((l_bitmap && (l_bitmap & 0x1)) || (!l_bitmap && l_cnt)) {
				switch (pregs->type) {
				case D11REG_TYPE_IHR32:
					val_32 = R_REG(wlc->osh,
						(volatile uint32*)(paddr + byte_offset));
					bcopy(&val_32, p, sizeof(val_32));
					p += sizeof(uint32);
					break;
				case D11REG_TYPE_IHR16:
					val_16 = R_REG(wlc->osh,
						(volatile uint16*)(paddr + byte_offset));
					bcopy(&val_16, p, sizeof(val_16));
					p += sizeof(val_16);
					break;
				case D11REG_TYPE_SCR:
					wlc_bmac_copyfrom_objmem(wlc->hw, byte_offset << 2,
						&val_16, sizeof(val_16), OBJADDR_SCR_SEL);
					bcopy(&val_16, p, sizeof(uint16));
					p += sizeof(val_16);
					break;
				case D11REG_TYPE_SHM:
					val_16 = wlc_read_shm(wlc, byte_offset);
					bcopy(&val_16, p, sizeof(uint16));
					p += sizeof(val_16);
					break;
				default:
					break;
				}
			}

			l_bitmap = l_bitmap >> 1;
			if (l_cnt) l_cnt--;
			byte_offset += pregs->step;
		}
	}

	return 0;
} /* wlc_macdbg_dump_mac_ucode_binary */

static uint16
wlc_macdbg_shm_dump_size(wlc_info_t *wlc)
{
	uint16 smmem_size = (4*1024);
	if (D11REV_GE(wlc->pub->corerev, 16)) {
		switch (((R_REG(wlc->osh, D11_MacHWCap1(wlc))&0x6)>>1)) {
			case 0: smmem_size = (4*1024); break;
			case 1: smmem_size = (6*1024); break;
			case 2: smmem_size = (8*1024); break;
			default: smmem_size = (8*1024); break;
		}
	}

	return smmem_size;
}

int
wlc_macdbg_dump_pc_binary(wlc_info_t *wlc, uchar *p)
{
	uint k;
	uint32 *ptr;

	*(uint16 *)p = UCODE_CORE_ID(wlc->pub->unit);
	*(uint16 *)(p + 2) = PC_DUMP_DEPTH;

	/* If trap happens without wlc_dump_ucode_fatal
	 *  being called.
	 */
	ptr = (uint32 *)(p + 4);
	for (k = 0; k < PC_DUMP_DEPTH; k++) {
		ptr[k] = R_REG(wlc->osh, D11_PSM_DEBUG(wlc));
	}

	/* halt the d11 core PC. */
	AND_REG(wlc->osh, D11_MACCONTROL(wlc), ~MCTL_PSM_RUN);

	return 0;
}
#endif /* DONGLEBUILD */

void
wlc_dump_mac_fatal(wlc_info_t *wlc, uint reason)
{
	wlc_dump_ucode_fatal(wlc, reason);
#ifdef WL_PSMX
	if (BCM_DMA_CT_ENAB(wlc)) {
		wlc_dump_psmx_fatal(wlc, (reason == PSM_FATAL_TXSTUCK) ?
			PSMX_FATAL_TXSTUCK : PSMX_FATAL_ANY);
	}
#endif /* WL_PSMX */
}

void
wlc_dump_phytxerr(wlc_info_t *wlc, uint16 PhyErr)
{
	uint16 phytxerrctl, corerev, txerr;
	uint16 tst;
	uint16 txerr_physts;
	phy_dbg_txerr_dump(WLC_PI(wlc), PhyErr);

	corerev = wlc->pub->corerev;
	if (D11REV_LT(corerev, 40)) {
		/* not supported for older revid */
	    return;
	}

#define PHYREG_TXERRLOGCTL 0x11c9
	if (D11REV_GE(corerev, 128)) {
		if ((txerr = wlc_read_shm(wlc, M_TXERR_NOWRITE(wlc))) != 0) {
			printf("txerr mac %04x phy %04x %04x %04x tst %02x dur %04x\n"
			"pctlen %04x pctls %04x %04x %04x %04x %04x %04x\n"
			"plcp %04x %04x || %04x %04x %04x || %04x %04x || %02x\n"
			"txbyte %04x %04x unflsts %04x usr %04x\n",
			txerr,
			wlc_read_shm(wlc, M_TXERR_PHYSTS(wlc)),
			wlc_read_shm(wlc, M_TXERR_REASON0(wlc)),
			wlc_read_shm(wlc, M_TXERR_REASON1(wlc)),
			wlc_read_shm(wlc, M_TXERR_CTXTST(wlc)),
			wlc_read_shm(wlc, M_TXERR_TXDUR(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTLEN(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL0(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL1(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL2(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL4(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL9(wlc)),
			wlc_read_shm(wlc, M_TXERR_PCTL10(wlc)),
			wlc_read_shm(wlc, M_TXERR_LSIG0(wlc)),
			wlc_read_shm(wlc, M_TXERR_LSIG1(wlc)),
			wlc_read_shm(wlc, M_TXERR_PLCP0(wlc)),
			wlc_read_shm(wlc, M_TXERR_PLCP1(wlc)),
			wlc_read_shm(wlc, M_TXERR_PLCP2(wlc)),
			wlc_read_shm(wlc, M_TXERR_SIGB0(wlc)),
			wlc_read_shm(wlc, M_TXERR_SIGB1(wlc)),
			wlc_read_shm(wlc, M_TXERR_CCLEN(wlc)),
			wlc_read_shm(wlc, M_TXERR_TXBYTES_L(wlc)),
			wlc_read_shm(wlc, M_TXERR_TXBYTES_H(wlc)),
			wlc_read_shm(wlc, M_TXERR_UNFLSTS(wlc)),
			wlc_read_shm(wlc, M_TXERR_USR(wlc)));
		} else {
			/* no need to print duplicated info if NOWRITE is not set */
			printf("txerr mac %04x\n", txerr);
		}

		/* clear bit-0 of phy register TXERRLOGCTL to resume logging in phy */
		W_REG(wlc->osh, D11_PHY_REG_ADDR(wlc), (uint16)PHYREG_TXERRLOGCTL);
		phytxerrctl = R_REG(wlc->osh, D11_PHY_REG_DATA(wlc));
		phytxerrctl &= ~1;
		W_REG(wlc->osh, D11_PHY_REG_DATA(wlc), phytxerrctl);
		/* If beacon tx mac2phy error */
		tst = wlc_read_shm(wlc, M_TXERR_CTXTST(wlc));
		txerr_physts = wlc_read_shm(wlc, M_TXERR_NOWRITE(wlc));
		WL_ERROR(("%s: tst 0x%04x txerr_physts 0x%04x\n", __FUNCTION__,
			tst, txerr_physts));

		if (((tst & 0xFF) == PHYTXERR_CTXTST_BCN) &&
			(txerr_physts == PHYTXERR_STS_MAC2PHY)) {
			wlc->bcn_txerr++;
			if (wlc->bcn_txerr > wlc->bcn_txerr_thresh) {
				WL_ERROR(("%s:ERROR %d sequential Beacon MAC2PHY "
					"TxErrors Resetting...\n", __FUNCTION__,
					wlc->bcn_txerr));
				/* reset count */
				wlc->bcn_txerr = 0;
				wlc_fatal_error(wlc);
			}
		} else {
			/* reset beacon tx error count */
			wlc->bcn_txerr = 0;
		}
	} else {
		/* plcp dump format here is L-SIG || Sig-A || Sig-B */
		printf("txerr valid (%d) reason %04x %04x tst %02x pctls %04x %04x %04x\n"
		"plcp %04x %04x || %04x %04x %04x || %04x %04x || rxestats2 %04x\n",
		wlc_read_shm(wlc, M_TXERR_NOWRITE(wlc)),
		wlc_read_shm(wlc, M_TXERR_REASON(wlc)),
		wlc_read_shm(wlc, M_TXERR_REASON2(wlc)),
		wlc_read_shm(wlc, M_TXERR_CTXTST(wlc)),
		wlc_read_shm(wlc, M_TXERR_PCTL0(wlc)),
		wlc_read_shm(wlc, M_TXERR_PCTL1(wlc)),
		wlc_read_shm(wlc, M_TXERR_PCTL2(wlc)),
		wlc_read_shm(wlc, M_TXERR_LSIG0(wlc)),
		wlc_read_shm(wlc, M_TXERR_LSIG1(wlc)),
		wlc_read_shm(wlc, M_TXERR_PLCP0(wlc)),
		wlc_read_shm(wlc, M_TXERR_PLCP1(wlc)),
		wlc_read_shm(wlc, M_TXERR_PLCP2(wlc)),
		wlc_read_shm(wlc, M_TXERR_SIGB0(wlc)),
		wlc_read_shm(wlc, M_TXERR_SIGB1(wlc)),
		wlc_read_shm(wlc, M_TXERR_RXESTATS2(wlc)));

#ifdef WLCNT
		wlc_statsupd(wlc);
		printf("txfunfl: %d %d %d %d %d %d txtplunfl %d txphyerror %d\n",
			MCSTVAR(wlc->pub, txfunfl)[0],
			MCSTVAR(wlc->pub, txfunfl)[1],
			MCSTVAR(wlc->pub, txfunfl)[2],
			MCSTVAR(wlc->pub, txfunfl)[3],
			MCSTVAR(wlc->pub, txfunfl)[4],
			MCSTVAR(wlc->pub, txfunfl)[5],
			MCSTVAR(wlc->pub, txtplunfl),
			MCSTVAR(wlc->pub, txphyerror));
#endif /* WLCNT */
#ifdef WL_MU_TX
		if (MU_TX_ENAB(wlc)) {
			uint16 val[4];
			uint16 addr = wlc_read_shm(wlc, M_DEBUGBLK_PTR(wlc));
			wlc_bmac_copyfrom_shm(wlc->hw, shm_addr(addr, 16), val,
				sizeof(uint16) * 4);
			printf("txunfl dbg: dur(2us) %d %d %d %d aggnum %d %d %d %d\n",
				(val[0] >> 8), (val[1] >> 8), (val[2] >> 8), (val[3] >> 8),
				(val[0] & 0xff), (val[1] & 0xff), (val[2] & 0xff), (val[3] & 0xff));
		}
#endif /* WL_MU_TX */
	}

	wlc_write_shm(wlc, M_TXERR_NOWRITE(wlc), 0);
} /* wlc_dump_phytxerr */

#ifdef WLC_MACDBG_FRAMEID_TRACE
void
wlc_macdbg_frameid_trace_pkt(wlc_macdbg_info_t *macdbg, void *pkt, d11txhdr_t *txh,
	uint8 fifo, uint16 txFrameID, uint8 epoch, void *scb)
{
	wlc_info_t *wlc = macdbg->wlc;

	macdbg->pkt_hist[macdbg->pkt_hist_cnt].pkt = pkt;
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].pktdata = (void *)PKTDATA(wlc->osh, pkt);
#ifdef DONGLEBUILD
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].lbflags = PKTFLAGS(pkt);
#endif
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].macctl_0 = *D11_TXH_GET_MACLOW_PTR(wlc, txh);
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].macctl_1 = *D11_TXH_GET_MACHIGH_PTR(wlc, txh);
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		macdbg->pkt_hist[macdbg->pkt_hist_cnt].macctl_2 = txh->rev128.MacControl_2;
		macdbg->pkt_hist[macdbg->pkt_hist_cnt].txchanspec = txh->rev128.Chanspec;
	} else {
		macdbg->pkt_hist[macdbg->pkt_hist_cnt].macctl_2 = 0;
		macdbg->pkt_hist[macdbg->pkt_hist_cnt].txchanspec = 0;
	}

	macdbg->pkt_hist[macdbg->pkt_hist_cnt].flags = WLPKTTAG(pkt)->flags;
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].flags3 = WLPKTTAG(pkt)->flags3;
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].frameid = txFrameID;
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].seq = WLPKTTAG(pkt)->seq;

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		macdbg->pkt_hist[macdbg->pkt_hist_cnt].epoch = epoch;
	}

	macdbg->pkt_hist[macdbg->pkt_hist_cnt].fifo = fifo;
	macdbg->pkt_hist[macdbg->pkt_hist_cnt].scb = scb;

	macdbg->pkt_hist_cnt = MODINC_POW2(macdbg->pkt_hist_cnt, PKT_HIST_NUM_ENT);
}

void
wlc_macdbg_frameid_trace_txs(wlc_macdbg_info_t *macdbg, void *pkt, tx_status_t *txs)
{
	wlc_info_t *wlc = macdbg->wlc;

	macdbg->txs_hist[macdbg->txs_hist_cnt].pkt = pkt;
	macdbg->txs_hist[macdbg->txs_hist_cnt].pktdata = pkt? (void *)PKTDATA(wlc->osh, pkt): NULL;
	macdbg->txs_hist[macdbg->txs_hist_cnt].frameid = txs->frameid;
	macdbg->txs_hist[macdbg->txs_hist_cnt].seq = txs->sequence;
	macdbg->txs_hist[macdbg->txs_hist_cnt].status = txs->status.raw_bits;

	if (D11REV_GE(wlc->pub->corerev, 40)) {
		macdbg->txs_hist[macdbg->txs_hist_cnt].ncons =
			(txs->status.raw_bits & TX_STATUS40_NCONS) >> TX_STATUS40_NCONS_SHIFT;

		macdbg->txs_hist[macdbg->txs_hist_cnt].s1 = TX_STATUS_MACTXS_S1(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].s2 = TX_STATUS_MACTXS_S2(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].s3 = TX_STATUS_MACTXS_S3(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].s4 = TX_STATUS_MACTXS_S4(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].s5 = TX_STATUS_MACTXS_S5(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].ack_map1 = TX_STATUS_MACTXS_ACK_MAP1(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].ack_map2 = TX_STATUS_MACTXS_ACK_MAP2(txs);
		macdbg->txs_hist[macdbg->txs_hist_cnt].s8 = TX_STATUS_MACTXS_S8(txs);
	}

	macdbg->txs_hist_cnt = MODINC_POW2(macdbg->txs_hist_cnt, TXS_HIST_NUM_ENT);
}

void
wlc_macdbg_frameid_trace_sync_start(wlc_macdbg_info_t *macdbg)
{
	macdbg->sync_hist_next = MODDEC_POW2(macdbg->sync_hist_cnt, SYNC_HIST_NUM_ENT);
}

void
wlc_macdbg_frameid_trace_sync(wlc_macdbg_info_t *macdbg, void *pkt)
{
	if (macdbg->sync_hist_cnt == macdbg->sync_hist_next)
		return;

	macdbg->sync_hist[macdbg->sync_hist_cnt].pkt = pkt;

	macdbg->sync_hist_cnt = MODINC_POW2(macdbg->sync_hist_cnt, SYNC_HIST_NUM_ENT);
}

/* dump histories */
void
wlc_macdbg_frameid_trace_dump(wlc_macdbg_info_t *macdbg, uint fifo)
{
#ifndef DONGLEBUILD
	wlc_info_t *wlc = macdbg->wlc;
	int k, j = 0;
#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    int i = 0;
#endif

#if defined(BCMDBG)
	hnddma_t *di = WLC_HW_DI(wlc, fifo);
#endif

	printf("pkt_hist_cnt:%d \n", macdbg->pkt_hist_cnt);
	printf("pkt hist:\n");
	/* start from last valid index */
	k = MODDEC_POW2(macdbg->pkt_hist_cnt, PKT_HIST_NUM_ENT);

	while (k != macdbg->pkt_hist_cnt) {
		if (macdbg->pkt_hist[k].pkt == NULL) {
			j++;
			k = MODDEC_POW2(k, PKT_HIST_NUM_ENT);
			continue;
		}

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
        i++;
        if (i < (macdbg->pkt_hist_cnt/20)) {
            k = MODDEC_POW2(k, PKT_HIST_NUM_ENT);
            continue;
        }
        i = 0;
#endif

		printf("h %04d pkt:%p data:%p scb:%p flgs:0x%08x flgs3:0x%08x frmid:0x%04x"
			"(0x%04x:fifo %02d) seq:0x%04x epch:%d fifo:%02d macctl_l:0x%04x\n",
			k, OSL_OBFUSCATE_BUF(macdbg->pkt_hist[k].pkt),
			macdbg->pkt_hist[k].pktdata,
			OSL_OBFUSCATE_BUF(macdbg->pkt_hist[k].scb),
			macdbg->pkt_hist[k].flags, macdbg->pkt_hist[k].flags3,
			macdbg->pkt_hist[k].frameid,
			D11_TXFID_GET_SEQ(wlc, macdbg->pkt_hist[k].frameid),
			D11_TXFID_GET_FIFO(wlc, macdbg->pkt_hist[k].frameid),
			macdbg->pkt_hist[k].seq,
			macdbg->pkt_hist[k].epoch, macdbg->pkt_hist[k].fifo,
			macdbg->pkt_hist[k].macctl_0);
		k = MODDEC_POW2(k, PKT_HIST_NUM_ENT);
	}

	printf("pkt hist: %d are null or intermediate txs\n", j);
	j = 0;

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    i = 0;
#endif

	printf("txs_hist_cnt:%d \n", macdbg->txs_hist_cnt);
	printf("txs hist:\n");

	/* start from last valid index */
	k = MODDEC_POW2(macdbg->txs_hist_cnt, TXS_HIST_NUM_ENT);

	while (k != macdbg->txs_hist_cnt) {
		if ((macdbg->txs_hist[k].pkt) == NULL) {
			j++;
			k = MODDEC_POW2(k, TXS_HIST_NUM_ENT);
			continue;
		}

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
        i++;
        if (i < (macdbg->txs_hist_cnt/20)) {
            k = MODDEC_POW2(k, TXS_HIST_NUM_ENT);
            continue;
        }
        i = 0;
#endif

		printf("t %04d pkt:%p data:%p frmid:0x%04x(0x%04x:fifo %02d) seq:0x%04x"
			" status:0x%04x ",
			k, OSL_OBFUSCATE_BUF(macdbg->txs_hist[k].pkt),
			macdbg->txs_hist[k].pktdata,
			macdbg->txs_hist[k].frameid,
			D11_TXFID_GET_SEQ(wlc, macdbg->txs_hist[k].frameid),
			D11_TXFID_GET_FIFO(wlc, macdbg->txs_hist[k].frameid),
			macdbg->txs_hist[k].seq,
			macdbg->txs_hist[k].status);
		if (D11REV_GE(wlc->pub->corerev, 40)) {
			printf("ncons:%02d s1:0x%08x s2:0x%08x s3:0x%08x s4:0x%08x s5:0x%08x"
				" ack_map1:0x%08x ack_map2:0x%08x s8:0x%08x\n",
				macdbg->txs_hist[k].ncons,
				macdbg->txs_hist[k].s1, macdbg->txs_hist[k].s2,
				macdbg->txs_hist[k].s3, macdbg->txs_hist[k].s4,
				macdbg->txs_hist[k].s5, macdbg->txs_hist[k].ack_map1,
				macdbg->txs_hist[k].ack_map2, macdbg->txs_hist[k].s8);
		} else {
			printf("\n");
		}

		k = MODDEC_POW2(k, TXS_HIST_NUM_ENT);
	}

	printf("txs hist: %d are null\n", j);
	j = 0;

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
    i = 0;
#endif

	/* start from last valid index */
	k = MODDEC_POW2(macdbg->sync_hist_cnt, SYNC_HIST_NUM_ENT);

	printf("sync_hist_next:%d \n", macdbg->sync_hist_next);
	printf("sync_hist_cnt:%d \n", macdbg->sync_hist_cnt);
	printf("sync hist:\n");
	while (k != macdbg->sync_hist_cnt) {
		if (macdbg->sync_hist[k].pkt == NULL) {
			j++;
			k = MODDEC_POW2(k, SYNC_HIST_NUM_ENT);
			continue;
		}

#if defined(AI_MESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
        i++;
        if (i < (macdbg->sync_hist_cnt/20)) {
            k = MODDEC_POW2(k, SYNC_HIST_NUM_ENT);
            continue;
        }
        i = 0;
#endif

		if (k == macdbg->sync_hist_next) {
			printf("first entry of older sync:\n");
		}
		printf("s %d p:%p\n", k, OSL_OBFUSCATE_BUF(macdbg->sync_hist[k].pkt));
		k = MODDEC_POW2(k, SYNC_HIST_NUM_ENT);
	}

	printf("sync hist: %d are null\n", j);

#if defined(BCMDBG)
	printf("DMA fifo %d =>\n", fifo);
	dma_dumptx(di, NULL, TRUE);
#ifdef BCM_DMA_CT
	if (BCM_DMA_CT_ENAB(wlc)) {
		printf("AQM DMA fifo %d =>\n", fifo);
		di = WLC_HW_AQM_DI(wlc, fifo);
		dma_dumptx(di, NULL, TRUE);
	}
#endif /* BCM_DMA_CT */
#endif
#endif /* !DONGLEBUILD */
} /* wlc_macdbg_frameid_trace_dump */

/* attach/detach */
static int
BCMATTACHFN(wlc_macdbg_frameid_trace_attach)(wlc_macdbg_info_t *macdbg, wlc_info_t *wlc)
{
	if ((macdbg->pkt_hist =
		MALLOCZ(wlc->osh, sizeof(*(macdbg->pkt_hist)) * PKT_HIST_NUM_ENT)) == NULL) {
		return BCME_NOMEM;
	}

	if ((macdbg->txs_hist =
		MALLOCZ(wlc->osh, sizeof(*(macdbg->txs_hist)) * TXS_HIST_NUM_ENT)) == NULL) {
		return BCME_NOMEM;
	}

	if ((macdbg->sync_hist =
		MALLOCZ(wlc->osh, sizeof(*(macdbg->sync_hist)) * SYNC_HIST_NUM_ENT)) == NULL) {
		return BCME_NOMEM;
	}

	return BCME_OK;
}

static void
BCMATTACHFN(wlc_macdbg_frameid_trace_detach)(wlc_macdbg_info_t *macdbg, wlc_info_t *wlc)
{
	if (macdbg->sync_hist != NULL) {
		MFREE(wlc->osh, macdbg->sync_hist, sizeof(*(macdbg->sync_hist))*SYNC_HIST_NUM_ENT);
		macdbg->sync_hist = NULL;
	}
	if (macdbg->txs_hist != NULL) {
		MFREE(wlc->osh, macdbg->txs_hist, sizeof(*(macdbg->txs_hist)) * TXS_HIST_NUM_ENT);
		macdbg->txs_hist = NULL;
	}
	if (macdbg->pkt_hist != NULL) {
		MFREE(wlc->osh, macdbg->pkt_hist, sizeof(*(macdbg->pkt_hist)) * PKT_HIST_NUM_ENT);
		macdbg->pkt_hist = NULL;
	}
}

static int
wlc_dump_pkt_hist(wlc_info_t *wlc, struct bcmstrbuf *b)
{
	int i;
	wlc_macdbg_info_t *macdbg = wlc->macdbg;
	pkt_hist_t *pkt_hist;

	bcm_bprintf(b, "pkt_hist_cnt:%d \n", macdbg->pkt_hist_cnt);
	bcm_bprintf(b, "pkt hist:\n");
	/* start from last valid index */
	i = MODDEC_POW2(macdbg->pkt_hist_cnt, PKT_HIST_NUM_ENT);

	while (i != macdbg->pkt_hist_cnt) {
		pkt_hist = &macdbg->pkt_hist[i];

		if (pkt_hist->pkt == NULL) {
			/* to prev idx */
			i = MODDEC_POW2(i, PKT_HIST_NUM_ENT);
			continue;
		}

		bcm_bprintf(b, "idx %04d pkt:%p data:%p frmid:0x%04x"
			"[0x%04x:fifo %02d(p)/%02d(l)] d11hdr_seq:0x%04x "
			"macctl_0:%04x macctl_1:%04x macctl_2:%04x chanspec:%04x\n",
			i, OSL_OBFUSCATE_BUF(pkt_hist->pkt),
			pkt_hist->pktdata, pkt_hist->frameid,
			D11_TXFID_GET_SEQ(wlc, pkt_hist->frameid),
			pkt_hist->frameid & TXFID_FIFO_MASK,
			D11_TXFID_GET_FIFO(wlc, pkt_hist->frameid),
			pkt_hist->seq, pkt_hist->macctl_0,
			pkt_hist->macctl_1, pkt_hist->macctl_2, pkt_hist->txchanspec);

		/* to prev idx */
		i = MODDEC_POW2(i, PKT_HIST_NUM_ENT);
	}

	return BCME_OK;
}
#endif /* WLC_MACDBG_FRAMEID_TRACE */

#ifdef WL_UTRACE
void
wlc_utrace_capture_get(wlc_info_t *wlc, void *data, int length)
{
	wl_utrace_capture_args_t *capture_args = (wl_utrace_capture_args_t *) data;
	uint32 *utrace_data = (uint32 *) ((uint32)data + sizeof(wl_utrace_capture_args_t));
	uint32 buffer_length_bytes = length - sizeof(*capture_args);
	uint32 utrace_size;
	uint word_count;
	int utrace_start = T_UTRACE_BLK_STRT;

	/* Suspend the MAC before reading the template */
	if (wlc->macdbg->utrace_capture_count_bytes  == 0) {
		wlc_suspend_mac_and_wait(wlc);
	}

	utrace_size = T_UTRACE_TPL_RAM_SIZE_BYTES;
	/* The start point in the template ram for capture. */
	wlc_bmac_templateptr_wreg(wlc->hw, utrace_start
		+ (wlc->macdbg->utrace_capture_count_bytes));
	for (word_count = 0; ((word_count < buffer_length_bytes/sizeof(*utrace_data)) &&
		((wlc->macdbg->utrace_capture_count_bytes + (word_count * sizeof(*utrace_data)))
					< utrace_size)); word_count++) {
		utrace_data[word_count] = (uint32) wlc_bmac_templatedata_rreg(wlc->hw);
	}

	capture_args->length = (word_count * sizeof(*utrace_data));
	 capture_args->version = 2;
	wlc->macdbg->utrace_capture_count_bytes =
	(wlc->macdbg->utrace_capture_count_bytes + capture_args->length);

	if (wlc->macdbg->utrace_capture_count_bytes >= utrace_size) {
		wlc_enable_mac(wlc);
		/* Reset the capture count. */
		wlc->macdbg->utrace_capture_count_bytes = 0;
		capture_args->flag = WLC_UTRACE_READ_END;
	} else {
		/* Set the more data bit. */
		capture_args->flag = WLC_UTRACE_MORE_DATA;
	}
} /* wlc_utrace_capture_get */

void
wlc_utrace_init(wlc_info_t *wlc)
{
	/* Configuring the start and end pointer for the utrace feature. */
	uint32 utrace_eptr;
	uint32 utrace_size;
	uint16 utrace_start = T_UTRACE_BLK_STRT;
	utrace_size = T_UTRACE_TPL_RAM_SIZE_BYTES;

	/* compute the end offset from the last entry - 1 in the TmpDefinitions.shm */
	utrace_eptr = utrace_start + utrace_size -1;

	/* Program the shmems with the start and end pointers.
	* b0 in sptr enables the feature by default
	*/
	/* Initialize Start, End and Current Pointer */
	wlc_bmac_copyto_objmem(wlc->hw, S_UPTR << 2, &utrace_start,
		sizeof(utrace_start), OBJADDR_SCR_SEL);
	wlc_bmac_write_shm(wlc->hw, M_UTRACE_EPTR(wlc), (utrace_eptr));
	wlc_bmac_write_shm(wlc->hw, M_UTRACE_SPTR(wlc),
			(utrace_start | 0x1));
} /* wlc_utrace_init */

#endif /* WL_UTRACE */

#if defined(BCMHWA) && (defined(WLTEST) || defined(HWA_DUMP) || defined(BCMDBG_ERR))
static int
wlc_hwadbg_reg(wlc_info_t *wlc,
	wl_hwadbg_reg_param_t *params, char *out_buf, int out_len)
{
	int err = BCME_OK;
	int32 *ret_int_ptr = (int32 *)out_buf;
	wl_hwadbg_reg_param_t phwa_params;
	wl_hwadbg_reg_param_t *phwa = &phwa_params;

	if (!HWA_ENAB(wlc->pub)) {
		return BCME_UNSUPPORTED;
	}

	memcpy(phwa, params, sizeof(wl_hwadbg_reg_param_t));

	if (WL_TRACE_ON()) {
		printf("%s:\n", __FUNCTION__);
		printf("type <%s>\n", phwa->type);
		printf("offset <%d>\n", phwa->offset);
		printf("w_val <%d>\n", phwa->w_val);
		printf("w_en <%d>\n", phwa->w_en);
	}

	if (phwa->w_en == 0)
		err = hwa_dbg_regread(WL_HWA_DEVP(wlc), phwa->type, phwa->offset, ret_int_ptr);
#if defined(WLTEST)
	else
		err = hwa_dbg_regwrite(WL_HWA_DEVP(wlc), phwa->type, phwa->offset, phwa->w_val);
#endif

	return err;
}
#endif

#ifdef BCMDBG
static int
wlc_macdbg_set_dtrace(wlc_macdbg_info_t *macdbg, uint16 flag)
{
	wlc_info_t *wlc = macdbg->wlc;
	int res = BCME_OK;

	if (flag) {
		if (macdbg->dtrace_buf == NULL) {
			macdbg->dtrace_buf = MALLOCZ(wlc->osh, MAXDTRACEBUFLEN);
			if (macdbg->dtrace_buf == NULL) {
				res = BCME_NOMEM;
				goto exit;
			}
			macdbg->dtrace_len = 0;
		}
	} else {
		if (macdbg->dtrace_buf) {
			/* log whatever collected so far before turning dtrace off */
#ifdef DONGLEBUILD
			wlc_mac_event(wlc, WLC_E_MACDBG, NULL, WLC_E_STATUS_SUCCESS,
				WLC_E_MACDBG_DTRACE, 0, macdbg->dtrace_buf, macdbg->dtrace_len);
#else
			wl_sched_dtrace(wlc->wl, macdbg->dtrace_buf, macdbg->dtrace_len);
#endif /* DONGLEBUILD */
			MFREE(wlc->osh, macdbg->dtrace_buf, MAXDTRACEBUFLEN);
			macdbg->dtrace_buf = NULL;
			macdbg->dtrace_len = 0;
		}
	}

	macdbg->dtrace_flag = flag;
exit:
	return res;
}

static void
_wlc_macdbg_dtrace_print_buf(wlc_macdbg_info_t *macdbg)
{
	int datalen = 0, i = 0, j;
	bcm_xtlv_t *dtrace_rec;

	while (datalen < macdbg->dtrace_len) {
		dtrace_rec = (bcm_xtlv_t *)(macdbg->dtrace_buf + datalen);
		printf("[%02d] %01X ", i, dtrace_rec->id);
		for (j = 0; j < dtrace_rec->len; j++) {
			printf("%02X", dtrace_rec->data[j]);
		}
		printf("\n");
		i++;
		datalen += (dtrace_rec->len + BCM_XTLV_HDR_SIZE);
	}
}

/* check if ID is enabled and return data pointer */
static void *
_wlc_macdbg_dtrace_log_prep(wlc_macdbg_info_t *macdbg, struct scb *scb,
	uint8 dtrace_id, uint16 datalen)
{
	bcm_xtlv_t *dtrace_rec;
	uint16 record_len = datalen + BCM_XTLV_HDR_SIZE;

	if (!(macdbg->dtrace_flag & (1 << dtrace_id))) {
		/* dtrace is not enabled for this type */
		return NULL;
	}

	if (!ETHER_ISNULLADDR(&macdbg->dtrace_ea) && scb &&
		memcmp(&macdbg->dtrace_ea, &scb->ea, ETHER_ADDR_LEN)) {
		return NULL;
	}

	ASSERT(macdbg->dtrace_buf);
	dtrace_rec = (bcm_xtlv_t *)(macdbg->dtrace_buf + macdbg->dtrace_len);
	if ((macdbg->dtrace_len + record_len) > MAXDTRACEBUFLEN) {
		/* if buffer full, log the earlier records and empty */
#ifdef DONGLEBUILD
		wlc_mac_event(macdbg->wlc, WLC_E_MACDBG, NULL, WLC_E_STATUS_SUCCESS,
			WLC_E_MACDBG_DTRACE, 0, macdbg->dtrace_buf, macdbg->dtrace_len);
#else
		wl_sched_dtrace(macdbg->wlc->wl, macdbg->dtrace_buf, macdbg->dtrace_len);
#endif /* DONGLEBUILD */
		dtrace_rec = (bcm_xtlv_t *)macdbg->dtrace_buf;
		macdbg->dtrace_len = 0;
	}

	macdbg->dtrace_len += record_len;

	dtrace_rec->id = dtrace_id;
	dtrace_rec->len = datalen;

	return (dtrace_rec->data);
}

INLINE void
wlc_macdbg_dtrace_log_txs(wlc_macdbg_info_t *macdbg, struct scb *scb,
	ratespec_t *txrspec, tx_status_t *txs)
{
	dtrace_txs_t *dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb,
		DTRACE_ID_TXS, sizeof(*dtrace_data));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	/* Keep little endian format */
	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	dtrace_data->rspec = txrspec[0];
	dtrace_data->frameid = txs->frameid;
	dtrace_data->raw_bits = txs->status.raw_bits;
	dtrace_data->s8 = TX_STATUS_MACTXS_S8(txs);
	dtrace_data->s3 = TX_STATUS_MACTXS_S3(txs);
	dtrace_data->s4 = TX_STATUS_MACTXS_S4(txs);
	dtrace_data->s5 = TX_STATUS_MACTXS_S5(txs);
	dtrace_data->ack_map1 = TX_STATUS_MACTXS_ACK_MAP1(txs);
	dtrace_data->ack_map2 = TX_STATUS_MACTXS_ACK_MAP2(txs);
}

INLINE void
wlc_macdbg_dtrace_log_utxs(wlc_macdbg_info_t *macdbg, struct scb *scb,
	tx_status_t *txs)
{
	dtrace_txs_t *dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb,
		DTRACE_ID_UTXS, sizeof(*dtrace_data));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	/* Keep little endian format */
	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	dtrace_data->raw_bits = txs->status.raw_bits;
	dtrace_data->s8 = TX_STATUS_MACTXS_S8(txs);
	dtrace_data->s3 = TX_STATUS_MACTXS_S3(txs);
	dtrace_data->s4 = TX_STATUS_MACTXS_S4(txs);
	dtrace_data->s5 = TX_STATUS_MACTXS_S5(txs);
	dtrace_data->ack_map1 = TX_STATUS_MACTXS_ACK_MAP1(txs);
	dtrace_data->ack_map2 = TX_STATUS_MACTXS_ACK_MAP2(txs);
}

INLINE void
wlc_macdbg_dtrace_log_txd(wlc_macdbg_info_t *macdbg, struct scb *scb,
	ratespec_t *txrspec, d11txh_rev128_t *txh)
{
	dtrace_txd_t *dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb,
		DTRACE_ID_TXD, sizeof(*dtrace_data));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	/* Keep little endian format */
	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	dtrace_data->rspec = txrspec[0];
	dtrace_data->MacControl_0 = txh->MacControl_0;
	dtrace_data->MacControl_1 = txh->MacControl_1;
	dtrace_data->MacControl_2 = txh->MacControl_2;
	dtrace_data->Chanspec = txh->Chanspec;
	dtrace_data->FrameID = txh->FrameID;
	dtrace_data->SeqCtl = txh->SeqCtl;
	dtrace_data->FrmLen = txh->FrmLen;
	dtrace_data->IVOffset = txh->IVOffset_Lifetime_lo;
	dtrace_data->Lifetime = txh->Lifetime_hi;
	dtrace_data->LinkMemIdxTID = txh->LinkMemIdxTID;
	dtrace_data->RateMemIdxRateIdx = txh->RateMemIdxRateIdx;
}

INLINE void
wlc_macdbg_dtrace_log_utxd(wlc_macdbg_info_t *macdbg, struct scb *scb,
	d11ulmu_txd_t *utxd)
{
	dtrace_utxd_t *dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb,
		DTRACE_ID_UTXD, sizeof(*dtrace_data));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	memcpy(&dtrace_data->utxd, utxd, sizeof(d11ulmu_txd_t));
}

INLINE void
wlc_macdbg_dtrace_log_txr(wlc_macdbg_info_t *macdbg, struct scb *scb,
	uint16 link_idx, d11ratemem_rev128_entry_t *rate_in)
{
	dtrace_rate_t *rate;
	d11ratemem_rev128_rate_t *rate_info_in;
	int i;
	dtrace_txr_t *dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb,
		DTRACE_ID_TXR, sizeof(*dtrace_data));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	/* Keep little endian format */
	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	dtrace_data->flags = rate_in->flags;
	dtrace_data->link_idx = link_idx;
	for (i = 0; i < 4; i++) {
		rate = &dtrace_data->rate_info_block[i];
		rate_info_in = &rate_in->rate_info_block[i];
		STATIC_ASSERT(sizeof(*rate) == OFFSETOF(d11ratemem_rev128_rate_t, BFM0));
		memcpy(rate, rate_info_in, sizeof(*rate));
	}
}

void
wlc_macdbg_dtrace_log_str(wlc_macdbg_info_t *macdbg, struct scb *scb, const char *format, ...)
{
	char strbuf[128];
	uint16 strbuflen;
	va_list args;
	dtrace_str_t *dtrace_data;

	va_start(args, format);
	vsnprintf(strbuf, sizeof(strbuf), format, args);
	va_end(args);

	strbuflen = strlen(strbuf);
	dtrace_data = _wlc_macdbg_dtrace_log_prep(macdbg, scb, DTRACE_ID_STR,
		(OFFSETOF(dtrace_str_t, str) + strbuflen));

	if (dtrace_data == NULL) {
		return; /* this ID not enabled. */
	}

	/* Keep little endian format */
	dtrace_data->time_in_ms = OSL_SYSUPTIME();
	memcpy(dtrace_data->str, strbuf, strbuflen);
}

/*
 * cause psm wd for debug dump mechanism
 * TBD: should extend to any of psm(r, x)
 */
static int
wlc_macdbg_gen_psmwd(wlc_macdbg_info_t *macdbg, uint16 after)
{
	wlc_info_t *wlc = macdbg->wlc;

	WL_ERROR(("%s: request mac crash after %d.\n", __FUNCTION__, after));
	OSL_DELAY(after);

	if (D11REV_GE(wlc->pub->corerev, 128)) {
		/* only ax core impl this */
		W_REG(wlc->osh, D11_MACCOMMAND(wlc->hw), MCMD_PSMWD);
	} else {
		AND_REG(wlc->osh, D11_MACCONTROL(wlc), ~MCTL_PSM_RUN);
	}

	return BCME_OK;
}

/* trigger a PSMWD under certain conditions */
int
wlc_macdbg_psmwd_trigger(wlc_macdbg_info_t *macdbg, uint16 reason)
{
	if (!(macdbg->psmwd_reason & (1 << reason))) {
		return BCME_OK;
	}

	return wlc_macdbg_gen_psmwd(macdbg, 1 /* delay in usec */);
}

#ifdef WL_PSMX
static int
wlc_macdbg_gen_psmxwd(wlc_macdbg_info_t *macdbg, uint16 after)
{
	wlc_info_t *wlc = macdbg->wlc;

	WL_ERROR(("%s: request macx crash after %d.\n", __FUNCTION__, after));
	OSL_DELAY(after);
	if (D11REV_GE(wlc->pub->corerev, 128)) {
		/* only ax core impl this */
		W_REG(wlc->osh, D11_MACCOMMAND_psmx(wlc->hw), MCMD_PSMWD);
	} else {
		AND_REG(wlc->osh, D11_MACCONTROL_psmx(wlc), ~MCTL_PSM_RUN);
	}

	return BCME_OK;
}
#endif /* WL_PSMX */

void
wlc_macdbg_txs_ppdu_info(wlc_macdbg_info_t *macdbg, tx_status_t* txs)
{
	wlc_info_t *wlc = macdbg->wlc;
	wlc_macdbg_ppdu_stats_t* p_ppdu = (wlc_macdbg_ppdu_stats_t*) &macdbg->ppdu_info;
	int8 epoch, i;
	bool ismu;
	uint8 usr_idx, gbmp;

	if (!macdbg->ppdutxs) {
		return;
	}

	WL_INFORM(("wl%d: %s: txs\n"
		"  %04X %04X | %04X %04X %08X | %08X %08X || %08X %08X | %08X %08X\n",
		wlc->pub->unit, __FUNCTION__,
		txs->status.raw_bits, txs->frameid, txs->sequence, txs->phyerr,
		TX_STATUS_MACTXS_S2(txs), TX_STATUS_MACTXS_S3(txs),
		TX_STATUS_MACTXS_S4(txs), TX_STATUS_MACTXS_S5(txs),
		TX_STATUS_MACTXS_ACK_MAP1(txs), TX_STATUS_MACTXS_ACK_MAP2(txs),
		TX_STATUS_MACTXS_S8(txs)));

	ismu = TX_STATUS_MACTXS_S5(txs) & TX_STATUS64_MUTX;
	epoch = TX_STATUS128_EPOCH(TX_STATUS_MACTXS_S5(txs));
	if (!ismu || p_ppdu->epoch == -1 || epoch != p_ppdu->epoch) {
		// this is a new PPDU
		p_ppdu->epoch = epoch;
		p_ppdu->ismu = ismu;
		p_ppdu->pktBw = TX_STATUS64_MU_BW(TX_STATUS_MACTXS_S3(txs));
		p_ppdu->mutype = TX_STATUS128_MUTYP(TX_STATUS_MACTXS_S5(txs));
		p_ppdu->txtime = TX_STATUS128_TXDUR(TX_STATUS_MACTXS_S2(txs));
		p_ppdu->ntxs_recv = 0;
		if (ismu) {
			if (p_ppdu->mutype == TX_STATUS_MUTP_HEOM) {
				p_ppdu->nusrs = TX_STATUS128_HEOM_NUSRS(TX_STATUS_MACTXS_S3(txs));
			} else {
				gbmp = TX_STATUS64_MU_GBMP(TX_STATUS_MACTXS_S4(txs));
				p_ppdu->nusrs = bcm_bitcount(&gbmp, 1);
			}
		} else {
			p_ppdu->nusrs = 1;
		}
	} else {
		// this txs and prev txs belong to the same PPDU
		p_ppdu->ntxs_recv++;
	}

	usr_idx = p_ppdu->ntxs_recv;
	if (usr_idx >= p_ppdu->nusrs) {
		// this is an invalid txs, skip it
		return;
	}

	p_ppdu->usr_info[usr_idx].fifo = D11_TXFID_GET_FIFO(wlc, txs->frameid);

	if (!ismu) {
		/* SU case */
		MACDBG_AIRTIME_UPD(&macdbg->airtime_stats[AIRTIME_TX_SU], p_ppdu->txtime);
		WL_MAC(("wl%d: %s: SU PPDU txtime %d (usec) fifo %d\n",
			wlc->pub->unit, __FUNCTION__, p_ppdu->txtime, p_ppdu->usr_info[0].fifo));
	} else {
		if (p_ppdu->mutype == TX_STATUS_MUTP_HEOM ||
			p_ppdu->mutype == TX_STATUS_MUTP_HEMOM) {
			p_ppdu->usr_info[usr_idx].ruidx =
				TX_STATUS128_HEOM_RUIDX(TX_STATUS_MACTXS_S4(txs));
		} else {
			p_ppdu->usr_info[usr_idx].ruidx = -1;
		}

		p_ppdu->usr_info[usr_idx].mcs = TX_STATUS64_MU_MCS(TX_STATUS_MACTXS_S4(txs));
		p_ppdu->usr_info[usr_idx].nss =	TX_STATUS64_MU_NSS(TX_STATUS_MACTXS_S4(txs)) + 1;
		if (usr_idx + 1 == p_ppdu->nusrs) {
			if (p_ppdu->mutype == TX_STATUS_MUTP_HEOM) {
				MACDBG_AIRTIME_UPD(&macdbg->airtime_stats[AIRTIME_TX_OMU],
					p_ppdu->txtime);
			} else if (p_ppdu->mutype == TX_STATUS_MUTP_VHTMU) {
				MACDBG_AIRTIME_UPD(&macdbg->airtime_stats[AIRTIME_TX_VMU],
					p_ppdu->txtime);
			} else if (p_ppdu->mutype == TX_STATUS_MUTP_HEMM) {
				MACDBG_AIRTIME_UPD(&macdbg->airtime_stats[AIRTIME_TX_HMU],
					p_ppdu->txtime);
			}
			WL_MAC(("wl%d: %s: MU PPDU txtime %d (usec) mutype %d "
				"pktBW %d num users %d:\n",
				wlc->pub->unit, __FUNCTION__, p_ppdu->txtime,
				p_ppdu->mutype, p_ppdu->pktBw, p_ppdu->nusrs));
			for (i = 0; i < p_ppdu->nusrs; i++) {
				WL_MAC(("u%d fifo %d mcs %d nss %d ruidx %d\n", i,
				p_ppdu->usr_info[i].fifo, p_ppdu->usr_info[i].mcs,
				p_ppdu->usr_info[i].nss, p_ppdu->usr_info[i].ruidx));
			}
		}
	}
}

void
wlc_macdbg_txs_ulmu_info(wlc_macdbg_info_t *macdbg, tx_status_t* txs)
{
	uint16 airtime;

	if (!macdbg->ppdutxs) {
		return;
	}

	/* lsig_len * 8 / 6 + lsig_preamble */
	airtime = (TGTXS_LSIG(TX_STATUS_MACTXS_S2(txs)) * 4 / 3) + 20;
	MACDBG_AIRTIME_UPD(&macdbg->airtime_stats[AIRTIME_RX_ULMU], airtime);
}
#endif /* BCMDBG */
