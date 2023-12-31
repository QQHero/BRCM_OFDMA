/*
 * Common interface to channel definitions.
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
 * $Id: wlc_channel.c 806838 2022-01-05 16:38:14Z $
 */

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */

#include <wlc_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <siutils.h>
#include <bcmendian.h>
#include <802.11.h>
#include <wpa.h>
#include <sbconfig.h>
#include <pcicfg.h>
#include <bcmsrom.h>
#include <wlioctl.h>
#ifdef BCMSUP_PSK
#include <eapol.h>
#include <bcmwpa.h>
#endif /* BCMSUP_PSK */
#include <bcmdevs.h>
#include <d11.h>
#include <wlc_rate.h>
#include <wlc_pub.h>
#include <wlc.h>
#include <wlc_bmac.h>
#include <wlc_phy_hal.h>
#include <phy_radar_api.h>
#include <phy_radio_api.h>
#include <phy_rxgcrs_api.h>
#include <phy_tpc_api.h>
#include <phy_utils_api.h>
#include <wl_export.h>
#include <wlc_stf.h>
#include <wlc_channel.h>
#ifdef WLC_TXCAL
#include <wlc_calload.h>
#endif
#ifdef WL_SARLIMIT
#include <phy_tpc_api.h>
#include <wlc_sar_tbl.h>
#endif /* WL_SARLIMIT */
#include "wlc_clm_data.h"
#include <wlc_11h.h>
#include <wlc_tpc.h>
#include <wlc_dfs.h>
#include <wlc_11d.h>
#include <wlc_cntry.h>
#include <wlc_prot_g.h>
#include <wlc_prot_n.h>
#include <wlc_ie_mgmt.h>
#include <wlc_ie_mgmt_ft.h>
#include <wlc_ie_reg.h>
#ifdef WLOLPC
#include <wlc_olpc_engine.h>
#endif /* WLOLPC */
#include <wlc_ht.h>
#include <wlc_vht.h>
#include <wlc_event_utils.h>
#include <wlc_srvsdb.h>
#include <wlc_dump.h>
#include <wlc_iocv.h>
#ifdef WLC_TXPWRCAP
#include <phy_txpwrcap_api.h>
#endif
#ifdef DONGLEBUILD
#include <rte_trap.h>
#endif /* DONGLEBUILD */
#include <phy_radio_api.h>
#include <wlc_bsscfg.h>
#ifdef WL_MODESW
#include <wlc_modesw.h>
#endif /* WL_MODESW */
#include <bcmwifi_rclass.h>
#if defined(WL_AIR_IQ)
#include <wlc_scan.h>
#endif
#include <wlc_duration.h>

#ifdef WLC_TXPWRCAP
#define WL_TXPWRCAP(x) WL_NONE(x)
static int wlc_channel_txcap_set_country(wlc_cm_info_t *wlc_cmi);
static int wlc_channel_txcap_phy_update(wlc_cm_info_t *wlc_cmi,
    wl_txpwrcap_tbl_t *txpwrcap_tbl, int* cellstatus);
static int wlc_channel_txcapver(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);
#endif /* WLC_TXPWRCAP */

static uint wlc_convert_chanspec_bw_to_clm_bw(uint chanspec_bw);
static uint wlc_convert_clm_bw_to_chanspec_bw(uint clm_bw);

#define TXPWRCAP_NUM_SUBBANDS 5
#define TENLOGTEN20MHz 13 /* 10log10(20) = 13 */
#define TENLOGTEN40MHz 16 /* 10log10(40) = 16 */
#define TENLOGTEN80MHz 19 /* 10log10(80) = 19 */

#define BLOB_LITERAL "BLOB"
#define MAGIC_SEQ_LEN 4
#define PATRIM_MAGIC_SEQ 0x3D3F313D

typedef struct txcap_file_cc_group_info {
    uint8    num_cc;
    char    cc_list[1][2];
    /* int8 low_cap[num_subbands * num_antennas] */
    /* int8 high_cap[num_subbands * num_antennas] */
} txcap_file_cc_group_info_t;

#define TXCAP_FILE_HEADER_FLAG_WCI2_AND_HOST_MASK 0x1
typedef struct txcap_file_header {
    uint8    magic[4];
    uint16    version;
    uint16    flags;
    char    title[64];
    char    creation[64];
    uint8    num_subbands;
    uint8    num_antennas_per_core[TXPWRCAP_MAX_NUM_CORES];
    uint8    num_cc_groups;
    txcap_file_cc_group_info_t    cc_group_info;
} txcap_file_header_t;

#define WLC_CHAN_NUM_TXCHAIN    4
struct wlc_channel_txchain_limits {
    int8    chain_limit[WLC_CHAN_NUM_TXCHAIN];    /**< quarter dBm limit for each chain */
};

typedef struct wlc_cm_band {
    uint16        locale_flags;        /* locale_info_t flags */
    chanvec_t    valid_channels;        /* List of valid channels in the country */
    chanvec_t    *radar_channels;    /* List of radar sensitive channels */
    struct wlc_channel_txchain_limits chain_limits;    /* per chain power limit */
    uint8        PAD[4];
} wlc_cm_band_t;

#define PPR_BUF_NUM 2
#define PPR_RU_BUF_NUM 2    /* UL-OFDMA, RU type PPR */

/* Pre-alloc buf for PPR */
typedef struct wlc_cm_ppr_buffer {
    int lock;    /* 0 Free, 1 Locked */
    int8 *txpwr;    /* buf pointer, for ppr_t or ppr_ru_t */
} wlc_cm_ppr_buffer_t;

struct wlc_cm_info {
    wlc_pub_t    *pub;
    wlc_info_t    *wlc;
    char        srom_ccode[WLC_CNTRY_BUF_SZ];    /* Country Code in SROM */
    uint        srom_regrev;            /* Regulatory Rev for the SROM ccode */
    clm_country_t country;            /* Current country iterator for the CLM data */
    char        ccode[WLC_CNTRY_BUF_SZ];    /* current internal Country Code */
    uint        regrev;                /* current Regulatory Revision */
    char        country_abbrev[WLC_CNTRY_BUF_SZ];    /* current advertised ccode */
    wlc_cm_band_t    bandstate[MAXBANDS];    /* per-band state (one per phy/radio) */
    /* quiet channels currently for radar sensitivity or 11h support */
    chanvec_t    quiet_channels;        /* channels on which we cannot transmit */

    struct clm_data_header* clm_base_dataptr;
    int clm_base_data_len;

    /* List of radar sensitive channels for the current locale */
    chanvec_t locale_radar_channels;

    /* restricted channels */
    chanvec_t    restricted_channels;    /* copy of the global restricted channels of the */
                        /* current local */
    bool        has_restricted_ch;

    /* regulatory class */
    rcvec_t        valid_rcvec;        /* List of valid regulatory class in the country */
    bool        sar_enable;        /* Use SAR as part of regulatory power calc */
#ifdef WL_SARLIMIT
    sar_limit_t    sarlimit;        /* sar limit per band/sub-band */
#endif
    /* List of valid regulatory class in the country */
    chanvec_t    allowed_5g_channels;    /* CLM valid 5G channels in the country */
    chanvec_t    allowed_6g_channels;    /* CLM valid 6G channels in the country */

    uint32        clmload_status;        /* detailed clmload status */
    wlc_blob_info_t *clmload_wbi;
    uint32        txcapload_status;    /* detailed TX CAP load status */
    wlc_blob_info_t *txcapload_wbi;
    txcap_file_header_t        *txcap_download;
    uint32                txcap_download_size;
    txcap_file_cc_group_info_t    *current_country_cc_group_info;
    uint8                current_country_cc_group_info_index;
    uint32                txcap_high_cap_timeout;
    uint8                txcap_high_cap_active;
    struct wl_timer            *txcap_high_cap_timer;
    uint8                txcap_download_num_antennas;
    uint8                txcap_config[TXPWRCAP_NUM_SUBBANDS];
    uint8                txcap_state[TXPWRCAP_NUM_SUBBANDS];
    uint8                txcap_wci2_cell_status_last;
    uint8                txcap_cap_states_per_cc_group;
    wl_txpwrcap_tbl_t               txpwrcap_tbl;
    int                             cellstatus;
    bcmwifi_rclass_type_t        cur_rclass_type;
    bool                use_global;
    uint8                rclass_bandmask;
    wlc_cm_ppr_buffer_t    ppr_buf[PPR_BUF_NUM];    /* Pre-alloc buf for PPR */
    wlc_cm_ppr_buffer_t    ppr_ru_buf[PPR_RU_BUF_NUM];    /* Pre-alloc buf for RU type PPR */
    dload_error_status_t blobload_status;        /* detailed blob load status */
};

typedef struct paoffset_srom {
    uint8 magic_seq[MAGIC_SEQ_LEN];
    uint16 hdr_len;        /* Header length including magic sequence */
    uint16 num_slices;    /* Number of slices.
                 * Will be 1 in case of MIMO and 2 in case of SDB
                 */
    uint16 slice0_offset;
    uint16 slice0_len;
    uint16 slice1_offset;
    uint16 slice1_len;
} paoffset_srom_t;

#if defined(BCMPCIEDEV_SROM_FORMAT) && defined(WLC_TXCAL)
static int wlc_srom_read_blobdata(struct wlc_info *wlc, wlc_cm_info_t *wlc_cmi);
static dload_error_status_t wlc_handle_cal_dload_wrapper(struct wlc_info *wlc,
    wlc_blob_segment_t *segments, uint32 segment_count);
#endif
static dload_error_status_t wlc_handle_clm_dload(wlc_info_t *wlc,
    wlc_blob_segment_t *segments, uint32 segment_count);

#ifdef WLC_TXPWRCAP
static dload_error_status_t wlc_handle_txcap_dload(wlc_info_t *wlc_cmi,
    wlc_blob_segment_t *segments, uint32 segment_count);
#endif /* WLC_TXPWRCAP */

static void wlc_channels_init(wlc_cm_info_t *wlc_cmi, clm_country_t country);
static void wlc_set_country_common(
    wlc_cm_info_t *wlc_cmi, const char* country_abbrev, const char* ccode, uint regrev,
    clm_country_t country);
static int wlc_channels_commit(wlc_cm_info_t *wlc_cmi);
static void wlc_chanspec_list_ordered(wlc_cm_info_t *wlc_cmi, wl_uint32_list_t *list,
    chanspec_t chanspec_mask);
static bool wlc_japan_ccode(const char *ccode);
static uint8 wlc_get_rclass(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec);
static void wlc_regclass_vec_init(wlc_cm_info_t *wlc_cmi);
static void wlc_upd_restricted_chanspec_flag(wlc_cm_info_t *wlc_cmi);
static int wlc_channel_update_txchain_offsets(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr);
#if BAND5G
static void wlc_channel_set_radar_chanvect(wlc_cm_info_t *wlc_cmi, wlcband_t *band, uint16 flags);
#endif

/* IE mgmt callbacks */
#ifdef WLTDLS
static uint wlc_channel_tdls_calc_rc_ie_len(void *ctx, wlc_iem_calc_data_t *calc);
static int wlc_channel_tdls_write_rc_ie(void *ctx, wlc_iem_build_data_t *build);
#endif /* WLTDLS */
#ifdef WL11H
static uint wlc_channel_calc_rclass_ie_len(void *ctx, wlc_iem_calc_data_t *data);
static int wlc_channel_write_rclass_ie(void *ctx, wlc_iem_build_data_t *data);
#endif /* WL11H */
#if defined(WL_SARLIMIT) && defined(BCMDBG)
static void wlc_channel_sarlimit_dump(wlc_cm_info_t *wlc_cmi, sar_limit_t *sar);
#endif /* WL_SARLIMIT && BCMDBG */
static void wlc_channel_spurwar_locale(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec);

static void wlc_channel_psdlimit_check(wlc_cm_info_t *wlc_cmi);

#if defined(BCMDBG)
static int wlc_channel_dump_reg_ppr(void *handle, struct bcmstrbuf *b);
static int wlc_channel_dump_reg_local_ppr(void *handle, struct bcmstrbuf *b);
static int wlc_channel_dump_srom_ppr(void *handle, struct bcmstrbuf *b);
static int wlc_channel_dump_margin(void *handle, struct bcmstrbuf *b);
static int wlc_channel_dump_locale(void *handle, struct bcmstrbuf *b);
static void wlc_channel_dump_locale_chspec(wlc_info_t *wlc, struct bcmstrbuf *b,
                                           chanspec_t chanspec, ppr_t *txpwr);
static int wlc_channel_init_ccode(wlc_cm_info_t *wlc_cmi, char* country_abbrev, uint srom_regrev,
    int ca_len);

static int wlc_dump_max_power_per_channel(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);
static int wlc_clm_limits_dump(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b, bool he);
static int wlc_dump_clm_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);
static int wlc_dump_clm_he_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);
static int wlc_dump_clm_ru_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);

static int wlc_dump_country_aggregate_map(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b);
static int wlc_channel_supported_country_regrevs(void *handle, struct bcmstrbuf *b);

static bool wlc_channel_clm_chanspec_valid(wlc_cm_info_t *wlc_cmi, chanspec_t chspec);
static int wlc_channel_band_chain_limit(wlc_cm_info_t *wlc_cm, uint bandtype,
    struct wlc_channel_txchain_limits *lim);

const char fraction[4][4] = {"   ", ".25", ".5 ", ".75"};
#define QDB_FRAC(x)    (x) / WLC_TXPWR_DB_FACTOR, fraction[(x) % WLC_TXPWR_DB_FACTOR]
#define QDB_FRAC_TRUNC(x)    (x) / WLC_TXPWR_DB_FACTOR, \
    ((x) % WLC_TXPWR_DB_FACTOR) ? fraction[(x) % WLC_TXPWR_DB_FACTOR] : ""
#endif

#define    COPY_LIMITS(src, index, dst, cnt)    \
        bcopy(&src.limit[index], txpwr->dst, cnt)
#define    COPY_DSSS_LIMS(src, index, dst)    \
        bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_CCK)
#define    COPY_OFDM_LIMS(src, index, dst)    \
        bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_OFDM)
#define    COPY_MCS_LIMS(src, index, dst)    \
        bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_MCS_1STREAM)
#ifdef WL11AC
#define    COPY_VHT_LIMS(src, index, dst)    \
        bcopy(&src.limit[index], txpwr->dst, WL_NUM_RATES_EXTRA_VHT)
#else
#define    COPY_VHT_LIMS(src, index, dst)
#endif

#define CLM_DSSS_RATESET(src) ((const ppr_dsss_rateset_t*)&src->limit[WL_RATE_1X1_DSSS_1])
#define CLM_OFDM_1X1_RATESET(src) ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X1_OFDM_6])
#define CLM_MCS_1X1_RATESET(src) ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X1_MCS0])

#define CLM_DSSS_1X2_MULTI_RATESET(src) \
    ((const ppr_dsss_rateset_t*)&src->limit[WL_RATE_1X2_DSSS_1])
#define CLM_OFDM_1X2_CDD_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X2_CDD_OFDM_6])
#define CLM_MCS_1X2_CDD_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X2_CDD_MCS0])

#define CLM_DSSS_1X3_MULTI_RATESET(src) \
    ((const ppr_dsss_rateset_t*)&src->limit[WL_RATE_1X3_DSSS_1])
#define CLM_OFDM_1X3_CDD_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X3_CDD_OFDM_6])
#define CLM_MCS_1X3_CDD_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X3_CDD_MCS0])

#define CLM_DSSS_1X4_MULTI_RATESET(src) \
    ((const ppr_dsss_rateset_t*)&src->limit[WL_RATE_1X4_DSSS_1])
#define CLM_OFDM_1X4_CDD_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X4_CDD_OFDM_6])
#define CLM_MCS_1X4_CDD_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X4_CDD_MCS0])

#define CLM_MCS_2X2_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X2_SDM_MCS8])
#define CLM_MCS_2X2_STBC_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X2_STBC_MCS0])

#define CLM_MCS_2X3_STBC_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X3_STBC_MCS0])
#define CLM_MCS_2X3_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X3_SDM_MCS8])

#define CLM_MCS_2X4_STBC_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X4_STBC_MCS0])
#define CLM_MCS_2X4_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X4_SDM_MCS8])

#define CLM_MCS_3X3_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_3X3_SDM_MCS16])
#define CLM_MCS_3X4_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_3X4_SDM_MCS16])

#define CLM_MCS_4X4_SDM_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_4X4_SDM_MCS24])

#define CLM_OFDM_1X2_TXBF_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X2_TXBF_OFDM_6])
#define CLM_MCS_1X2_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X2_TXBF_MCS0])
#define CLM_OFDM_1X3_TXBF_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X3_TXBF_OFDM_6])
#define CLM_MCS_1X3_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X3_TXBF_MCS0])
#define CLM_OFDM_1X4_TXBF_RATESET(src) \
    ((const ppr_ofdm_rateset_t*)&src->limit[WL_RATE_1X4_TXBF_OFDM_6])
#define CLM_MCS_1X4_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_1X4_TXBF_MCS0])
#define CLM_MCS_2X2_TXBF_RATESET(src) \
    ((const ppr_ht_mcs_rateset_t*)&src->limit[WL_RATE_2X2_TXBF_SDM_MCS8])
#define CLM_MCS_2X3_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X3_TXBF_SDM_MCS8])
#define CLM_MCS_2X4_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_2X4_TXBF_SDM_MCS8])
#define CLM_MCS_3X3_TXBF_RATESET(src) \
    ((const ppr_ht_mcs_rateset_t*)&src->limit[WL_RATE_3X3_TXBF_SDM_MCS16])
#define CLM_MCS_3X4_TXBF_RATESET(src) \
    ((const ppr_vht_mcs_rateset_t*)&src->limit[WL_RATE_3X4_TXBF_SDM_MCS16])
#define CLM_MCS_4X4_TXBF_RATESET(src) \
    ((const ppr_ht_mcs_rateset_t*)&src->limit[WL_RATE_4X4_TXBF_SDM_MCS24])

#if defined WLTXPWR_CACHE
static chanspec_t last_chanspec = 0;
#endif /* WLTXPWR_CACHE */

clm_result_t clm_aggregate_country_lookup(const ccode_t cc, unsigned int rev,
    clm_agg_country_t *agg);
clm_result_t clm_aggregate_country_map_lookup(const clm_agg_country_t agg,
    const ccode_t target_cc, unsigned int *rev);

static void
wlc_channel_read_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr, clm_power_limits_t *limits,
    wl_tx_bw_t bw);

static void
wlc_channel_read_ub_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr,
    clm_ru_power_limits_t *ru_limits, wl_tx_bw_t bw);

static void
wlc_channel_read_ru_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_ru_t *ru_txpwr,
    clm_ru_power_limits_t *ru_limits);

static clm_result_t wlc_clm_power_limits(
    const clm_country_locales_t *locales, clm_band_t band,
    unsigned int chan, int ant_gain, clm_limits_type_t limits_type,
    const clm_limits_params_t *params, void *limits, bool ru);

static bool is_current_band(wlc_info_t *wlc, enum wlc_bandunit bandunit);

/* QDB() macro takes a dB value and converts to a quarter dB value */
#ifdef QDB
#undef QDB
#endif
#define QDB(n) ((n) * WLC_TXPWR_DB_FACTOR)

/* Regulatory Matrix Spreadsheet (CLM) MIMO v3.8.6.4
 * + CLM v4.1.3
 * + CLM v4.2.4
 * + CLM v4.3.1 (Item-1 only EU/9 and Q2/4).
 * + CLM v4.3.4_3x3 changes(Skip changes for a13/14).
 * + CLMv 4.5.3_3x3 changes for Item-5(Cisco Evora (change AP3500i to Evora)).
 * + CLMv 4.5.3_3x3 changes for Item-3(Create US/61 for BCM94331HM, based on US/53 power levels).
 * + CLMv 4.5.5 3x3 (changes from Create US/63 only)
 * + CLMv 4.4.4 3x3 changes(Create TR/4 (locales Bn7, 3tn), EU/12 (locales 3s, 3sn) for Airties.)
 */

/*
 * Radar channel sets
 */

#if BAND5G
static const chanvec_t radar_set1 = { /* Channels 52 - 64, 100 - 144 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11,    /* 52 - 60 */
    0x01, 0x00, 0x00, 0x00, 0x10, 0x11, 0x11, 0x11,        /* 64, 100 - 124 */
    0x11, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,        /* 128 - 144 */
    0x00, 0x00, 0x00, 0x00}
};
#endif    /* BAND5G */

/*
 * Restricted channel sets
 */

#define WLC_REGCLASS_USA_2G_20MHZ    12
#define WLC_REGCLASS_EUR_2G_20MHZ    4
#define WLC_REGCLASS_JPN_2G_20MHZ    30
#define WLC_REGCLASS_JPN_2G_20MHZ_CH14    31
#define WLC_REGCLASS_GLOBAL_2G_20MHZ        81
#define WLC_REGCLASS_GLOBAL_2G_20MHZ_CH14    82

/* Parameters of clm RU rates (802.11ax OFDMA) */
typedef struct wlc_cm_ru_rate_p {
    wl_tx_mode_t        mode;
    wl_tx_nss_t        nss;
    wl_tx_chains_t        chain;
} wlc_cm_ru_rate_p_t;

/* For 802.11ax, there are 19 tx modes,
 * ex. WL_RU_RATE_1X1_26SS1 to WL_RU_RATE_4X4_TXBF_26SS4 in clm_ru_rates_t
 */
static const wlc_cm_ru_rate_p_t ru_rate_tbl[] = {
    {WL_TX_MODE_NONE,    WL_TX_NSS_1,    WL_TX_CHAINS_1},    /* RATE_1X1_SS1 */
    {WL_TX_MODE_CDD,    WL_TX_NSS_1,    WL_TX_CHAINS_2},    /* RATE_1X2_SS1 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_2,    WL_TX_CHAINS_2},    /* RATE_2X2_SS2 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_1,    WL_TX_CHAINS_2},    /* RATE_1X2_TXBF_SS1 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_2,    WL_TX_CHAINS_2},    /* RATE_2X2_TXBF_SS2 */
    {WL_TX_MODE_CDD,    WL_TX_NSS_1,    WL_TX_CHAINS_3},    /* RATE_1X3_SS1 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_2,    WL_TX_CHAINS_3},    /* RATE_2X3_SS2 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_3,    WL_TX_CHAINS_3},    /* RATE_3X3_SS3 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_1,    WL_TX_CHAINS_3},    /* RATE_1X3_TXBF_SS1 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_2,    WL_TX_CHAINS_3},    /* RATE_2X3_TXBF_SS2 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_3,    WL_TX_CHAINS_3},    /* RATE_3X3_TXBF_SS3 */
    {WL_TX_MODE_CDD,    WL_TX_NSS_1,    WL_TX_CHAINS_4},    /* RATE_1X4_SS1 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_2,    WL_TX_CHAINS_4},    /* RATE_2X4_SS2 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_3,    WL_TX_CHAINS_4},    /* RATE_3X4_SS3 */
    {WL_TX_MODE_NONE,    WL_TX_NSS_4,    WL_TX_CHAINS_4},    /* RATE_4X4_SS4 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_1,    WL_TX_CHAINS_4},    /* RATE_1X4_TXBF_SS1 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_2,    WL_TX_CHAINS_4},    /* RATE_2X4_TXBF_SS2 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_3,    WL_TX_CHAINS_4},    /* RATE_3X4_TXBF_SS3 */
    {WL_TX_MODE_TXBF,    WL_TX_NSS_4,    WL_TX_CHAINS_4},    /* RATE_4X4_TXBF_SS4 */
};

/* iovar table */
enum wlc_channel_iov {
    IOV_RCLASS                = 1, /* read rclass */
    IOV_CLMLOAD                = 2,
    IOV_CLMLOAD_STATUS            = 3,
    IOV_TXCAPLOAD                = 4,
    IOV_TXCAPLOAD_STATUS            = 5,
    IOV_TXCAPVER                = 6,
    IOV_TXCAPCONFIG                = 7,
    IOV_TXCAPSTATE                = 8,
    IOV_TXCAPHIGHCAPTO            = 9,
    IOV_TXCAPDUMP                = 10,
    IOV_QUIETCHAN                = 11,
    IOV_DIS_CH_GRP                = 12,
    IOV_DIS_CH_GRP_CONF            = 13,
    IOV_DIS_CH_GRP_USER            = 14,
    IOV_IS_EDCRS_EU                = 15,
    IOV_RAND_CH                = 16,
    IOV_LAST
};

static const bcm_iovar_t cm_iovars[] = {
    {"rclass", IOV_RCLASS, 0, 0, IOVT_UINT16, 0},
    {"clmload", IOV_CLMLOAD, IOVF_SET_DOWN, 0, IOVT_BUFFER, 0},
    {"clmload_status", IOV_CLMLOAD_STATUS, 0, 0, IOVT_UINT32, 0},
    {"txcapload", IOV_TXCAPLOAD, 0, 0, IOVT_BUFFER, 0},
    {"txcapload_status", IOV_TXCAPLOAD_STATUS, 0, 0, IOVT_UINT32, 0},
    {"txcapver", IOV_TXCAPVER, 0, 0, IOVT_BUFFER, 0},
    {"txcapconfig", IOV_TXCAPCONFIG, IOVF_SET_DOWN, 0, IOVT_BUFFER, sizeof(wl_txpwrcap_ctl_t)},
    {"txcapstate", IOV_TXCAPSTATE, 0, 0, IOVT_BUFFER, sizeof(wl_txpwrcap_ctl_t)},
    {"txcaphighcapto", IOV_TXCAPHIGHCAPTO, 0, 0, IOVT_UINT32, 0},
    {"txcapdump", IOV_TXCAPDUMP, 0, 0, IOVT_BUFFER, sizeof(wl_txpwrcap_dump_v3_t)},
    {"dis_ch_grp", IOV_DIS_CH_GRP, 0, 0, IOVT_UINT32, 0},
    {"dis_ch_grp_conf", IOV_DIS_CH_GRP_CONF, 0, 0, IOVT_UINT32, 0},
    {"dis_ch_grp_user", IOV_DIS_CH_GRP_USER, (IOVF_SET_DOWN), 0, IOVT_UINT32, 0},
    {"is_edcrs_eu", IOV_IS_EDCRS_EU, 0, 0, IOVT_BOOL, 0},
    {"rand_ch", IOV_RAND_CH, 0, 0, IOVT_UINT32, 0},
    {NULL, 0, 0, 0, 0, 0}
};

/* This includes the auto generated ROM IOCTL/IOVAR patch handler C source file (if auto patching is
 * enabled). It must be included after the prototypes and declarations above (since the generated
 * source file may reference private constants, types, variables, and functions).
 */
#include <wlc_patch.h>

/** TRUE if the caller supplied bandunit matches the currently selected band */
static bool
is_current_band(wlc_info_t *wlc, enum wlc_bandunit bandunit)
{
    return bandunit == wlc->band->bandunit;
}

clm_result_t
wlc_locale_get_channels(clm_country_locales_t *locales, clm_band_t band,
    chanvec_t *channels, chanvec_t *restricted)
{
    bzero(channels, sizeof(chanvec_t));
    bzero(restricted, sizeof(chanvec_t));

    return clm_country_channels(locales, band, (clm_channels_t *)channels,
        (clm_channels_t *)restricted);
}

clm_result_t wlc_get_flags(clm_country_locales_t *locales, clm_band_t band, uint16 *flags)
{
    unsigned long clm_flags = 0;

    clm_result_t result = clm_country_flags(locales, band, &clm_flags);

    *flags = 0;
    if (result == CLM_RESULT_OK) {
        if (clm_flags & CLM_FLAG_CBP_FCC) {
            *flags |= WLC_CBP_FCC;
        }

        switch (clm_flags & CLM_FLAG_DFS_MASK) {
        case CLM_FLAG_DFS_JP:
            *flags |= WLC_DFS_JP;
            break;
        case CLM_FLAG_DFS_UK:
            *flags |= WLC_DFS_UK;
            break;
        case CLM_FLAG_DFS_EU:
            *flags |= WLC_DFS_EU;
            break;
        case CLM_FLAG_DFS_US:
            *flags |= WLC_DFS_FCC;
            break;
        case CLM_FLAG_DFS_NONE:
            break;
        default:
            result = CLM_RESULT_ERR;
            WL_ERROR(("%s: Unsupported CLM DFS flag 0x%X\n",
                __FUNCTION__, (unsigned int)(clm_flags & CLM_FLAG_DFS_MASK)));
            break;
        }

        if (clm_flags & CLM_FLAG_EDCRS_EU) {
            *flags |= WLC_EDCRS_EU;
        }

        if (clm_flags & CLM_FLAG_FILTWAR1)
            *flags |= WLC_FILT_WAR;

        if (clm_flags & CLM_FLAG_TXBF)
            *flags |= WLC_TXBF;

        if (clm_flags & CLM_FLAG_NO_MIMO)
            *flags |= WLC_NO_MIMO;
        else {
            if (clm_flags & CLM_FLAG_NO_40MHZ)
                *flags |= WLC_NO_40MHZ;
            if (clm_flags & CLM_FLAG_NO_80MHZ)
                *flags |= WLC_NO_80MHZ;
            if (clm_flags & CLM_FLAG_NO_160MHZ)
                *flags |= WLC_NO_160MHZ;
        }

        if (clm_flags & CLM_FLAG_LO_GAIN_NBCAL)
            *flags |= WLC_LO_GAIN_NBCAL;

        if ((band == CLM_BAND_2G) && (clm_flags & CLM_FLAG_HAS_DSSS_EIRP))
            *flags |= WLC_EIRP;
        if ((band == CLM_BAND_5G) && (clm_flags & CLM_FLAG_HAS_OFDM_EIRP))
            *flags |= WLC_EIRP;
        if ((band == CLM_BAND_6G) && (clm_flags & CLM_FLAG_HAS_OFDM_EIRP))
            *flags |= WLC_EIRP;
    }

    return result;
}

clm_result_t wlc_get_locale(clm_country_t country, clm_country_locales_t *locales)
{
    return clm_country_def(country, locales);
}

/* autocountry default country code list */
static const char def_autocountry[][WLC_CNTRY_BUF_SZ] = {
    "XY",
    "XA",
    "XB",
    "X0",
    "X1",
    "X2",
    "X3",
    "XS",
    "XV",
    "XT"
};

static const char BCMATTACHDATA(rstr_ccode)[] = "ccode";
static const char BCMATTACHDATA(rstr_regrev)[] = "regrev";

static bool
wlc_autocountry_lookup(char *cc)
{
    uint i;

    for (i = 0; i < ARRAYSIZE(def_autocountry); i++)
        if (!strcmp(def_autocountry[i], cc))
            return TRUE;

    return FALSE;
}

static bool
wlc_lookup_advertised_cc(char* ccode, const clm_country_t country)
{
    ccode_t advertised_cc;
    bool rv = FALSE;
    if (CLM_RESULT_OK == clm_country_advertised_cc(country, advertised_cc)) {
        memcpy(ccode, advertised_cc, 2);
        ccode[2] = '\0';
        rv = TRUE;
    }

    return rv;
}

static char * BCMRAMFN(wlc_channel_ccode_default)(void)
{
#if defined(WLTEST) && !defined(WLTEST_DISABLED)
    return "#a";
#else
    return "#n";
#endif
}

static int
wlc_channel_init_ccode(wlc_cm_info_t *wlc_cmi, char* country_abbrev, uint srom_regrev, int ca_len)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    int result = BCME_OK;
    uint regrev = srom_regrev;
    clm_country_t country;

    result = wlc_country_lookup(wlc, country_abbrev, regrev, &country);

    /* default to ALL if country was not specified or not found */
    if (result != CLM_RESULT_OK) {
        strncpy(country_abbrev, "#a", ca_len - 1);
        regrev = 0;
        result = wlc_country_lookup(wlc, country_abbrev, regrev, &country);
        if (result == CLM_RESULT_OK) {
            WL_ERROR(("wl%d: %s: Country code is set for default \"ALL\".\n",
                wlc_cmi->pub->unit, __FUNCTION__));
        }
    }

    /* Default to the NULL country(#n) which has no channels, if country ALL is not found */
    if (result != CLM_RESULT_OK) {
        strncpy(country_abbrev, wlc_channel_ccode_default(), ca_len - 1);
        regrev = 0;
        result = wlc_country_lookup(wlc, country_abbrev, regrev, &country);
    }

    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: Error: Country code \"%s\" is not found.\n",
            wlc_cmi->pub->unit, __FUNCTION__, country_abbrev));

        ASSERT(result == CLM_RESULT_OK);

        return result;
    }

    /* save default country for exiting 11d regulatory mode */
    wlc_cntry_set_default(wlc->cntry, country_abbrev);

    /* initialize autocountry_default to driver default */
    if (wlc_autocountry_lookup(country_abbrev))
        wlc_11d_set_autocountry_default(wlc->m11d, country_abbrev);
    else
        wlc_11d_set_autocountry_default(wlc->m11d, "XV");

    /* Calling set_countrycode() once does not generate any event, if called more than
     * once generates COUNTRY_CODE_CHANGED event which will cause the driver to crash
     * at startup since bsscfg structure is still not initialized.
     */
    wlc_set_countrycode(wlc_cmi, country_abbrev, regrev);

    /* update edcrs_eu for the initial country setting */
    wlc->is_edcrs_eu = wlc_is_edcrs_eu(wlc);

    return result;
}

static int
wlc_cm_doiovar(void *hdl, uint32 actionid,
    void *params, uint p_len, void *arg, uint len, uint val_size, struct wlc_if *wlcif)
{
    wlc_cm_info_t *wlc_cmi = (wlc_cm_info_t *)hdl;
    int err = BCME_OK;
    int32 int_val = 0;
    int32 *ret_int_ptr;

    BCM_REFERENCE(len);
    BCM_REFERENCE(wlcif);
    BCM_REFERENCE(val_size);

    /* convenience int and bool vals for first 8 bytes of buffer */
    if (p_len >= (int)sizeof(int_val))
        bcopy(params, &int_val, sizeof(int_val));

    /* convenience int ptr for 4-byte gets (requires int aligned arg) */
    ret_int_ptr = (int32 *)arg;

    switch (actionid) {
    case IOV_GVAL(IOV_RCLASS): {
        chanspec_t chspec = *(chanspec_t*)params;
        if (!wf_chspec_valid(chspec)) {
            err = BCME_BADARG;
            break;
        }
        *ret_int_ptr = wlc_get_rclass(wlc_cmi, chspec);
        WL_INFORM(("chspec:%x rclass:%d\n", chspec, *ret_int_ptr));
        break;
    }
    case IOV_SVAL(IOV_CLMLOAD): {
        wl_dload_data_t dload_data;
        uint8 *bufptr;
        dload_error_status_t status;

        /* Make sure we have at least a dload data structure */
        if (p_len < sizeof(wl_dload_data_t)) {
            err =  BCME_ERROR;
            wlc_cmi->clmload_status = DLOAD_STATUS_IOVAR_ERROR;
            break;
        }

        /* copy to stack so structure wl_data_data has any required processor alignment */
        memcpy(&dload_data, (wl_dload_data_t *)params, sizeof(wl_dload_data_t));

        WL_NONE(("%s: IOV_CLMLOAD flag %04x, type %04x, len %d, crc %08x\n",
            __FUNCTION__, dload_data.flag, dload_data.dload_type,
            dload_data.len, dload_data.crc));

        if (((dload_data.flag & DLOAD_FLAG_VER_MASK) >> DLOAD_FLAG_VER_SHIFT)
            != DLOAD_HANDLER_VER) {
            err =  BCME_ERROR;
            wlc_cmi->clmload_status = DLOAD_STATUS_IOVAR_ERROR;
            break;
        }

        bufptr = ((wl_dload_data_t *)(params))->data;

        status = wlc_blob_download(wlc_cmi->clmload_wbi, dload_data.flag,
            bufptr, dload_data.len, wlc_handle_clm_dload);
        switch (status) {
            case DLOAD_STATUS_DOWNLOAD_SUCCESS:
            case DLOAD_STATUS_DOWNLOAD_IN_PROGRESS:
                err = BCME_OK;
                wlc_cmi->clmload_status = status;
                break;
            default:
                err = BCME_ERROR;
                wlc_cmi->clmload_status = status;
                break;
        }
        break;
    }
    case IOV_GVAL(IOV_CLMLOAD_STATUS): {
        *((uint32 *)arg) = wlc_cmi->clmload_status;
        break;
    }
#ifdef WLC_TXPWRCAP
    case IOV_SVAL(IOV_TXCAPLOAD): {
        wl_dload_data_t dload_data;
        uint8 *bufptr;
        dload_error_status_t status;
        if (!WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            err = BCME_UNSUPPORTED;
            break;
        }
        /* Make sure we have at least a dload data structure */
        if (p_len < sizeof(wl_dload_data_t)) {
            err =  BCME_ERROR;
            wlc_cmi->txcapload_status = DLOAD_STATUS_IOVAR_ERROR;
            break;
        }
        /* copy to stack so structure wl_data_data has any required processor alignment */
        memcpy(&dload_data, (wl_dload_data_t *)params, sizeof(wl_dload_data_t));
        WL_NONE(("%s: IOV_TXCAPLOAD flag %04x, type %04x, len %d, crc %08x\n",
            __FUNCTION__, dload_data.flag, dload_data.dload_type,
            dload_data.len, dload_data.crc));
        if (((dload_data.flag & DLOAD_FLAG_VER_MASK) >> DLOAD_FLAG_VER_SHIFT)
            != DLOAD_HANDLER_VER) {
            err =  BCME_ERROR;
            wlc_cmi->txcapload_status = DLOAD_STATUS_IOVAR_ERROR;
            break;
        }
        bufptr = ((wl_dload_data_t *)(params))->data;
        status = wlc_blob_download(wlc_cmi->txcapload_wbi, dload_data.flag,
            bufptr, dload_data.len, wlc_handle_txcap_dload);
        switch (status) {
            case DLOAD_STATUS_DOWNLOAD_SUCCESS:
            case DLOAD_STATUS_DOWNLOAD_IN_PROGRESS:
                err = BCME_OK;
                wlc_cmi->txcapload_status = status;
                break;
            default:
                err = BCME_ERROR;
                wlc_cmi->txcapload_status = status;
                break;
        }
        break;
    }
    case IOV_GVAL(IOV_TXCAPLOAD_STATUS): {
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            *((uint32 *)arg) = wlc_cmi->txcapload_status;
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_GVAL(IOV_TXCAPVER): {
        struct bcmstrbuf b;
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            bcm_binit(&b, arg, len);
            err = wlc_channel_txcapver(wlc_cmi, &b);
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_GVAL(IOV_TXCAPCONFIG): {
        wl_txpwrcap_ctl_t *txcap_ctl = (wl_txpwrcap_ctl_t *)arg;
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            txcap_ctl->version = TXPWRCAPCTL_VERSION;
            memcpy(txcap_ctl->ctl, wlc_cmi->txcap_config, TXPWRCAP_NUM_SUBBANDS);
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_GVAL(IOV_TXCAPSTATE): {
        wl_txpwrcap_ctl_t *txcap_ctl = (wl_txpwrcap_ctl_t *)arg;
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            txcap_ctl->version = TXPWRCAPCTL_VERSION;
            memcpy(txcap_ctl->ctl, wlc_cmi->txcap_state, TXPWRCAP_NUM_SUBBANDS);
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_SVAL(IOV_TXCAPCONFIG): {
        wl_txpwrcap_ctl_t *txcap_ctl = (wl_txpwrcap_ctl_t *)arg;
        int i;
        if (!WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            err = BCME_UNSUPPORTED;
            break;
        }
        if (txcap_ctl->version != 2) {
            err = BCME_VERSION;
            break;
        }

        /* Check that download has happened so we can do value range checks on values. */
        if (wlc_cmi->txcap_download == NULL) {
            err = BCME_NOTREADY;
            break;
        }

        if (wlc_cmi->txcap_cap_states_per_cc_group == 2) {
            for (i = 0; i < TXPWRCAP_NUM_SUBBANDS; i++) {
                if (txcap_ctl->ctl[i] > TXPWRCAPCONFIG_HOST) {
                    err = BCME_BADARG;
                    break;
                }
            }
        } else {
            for (i = 0; i < TXPWRCAP_NUM_SUBBANDS; i++) {
                if (txcap_ctl->ctl[i] > TXPWRCAPCONFIG_WCI2_AND_HOST) {
                    err = BCME_BADARG;
                    break;
                }
            }
        }

        if (err) break;
        memcpy(wlc_cmi->txcap_config, txcap_ctl->ctl, TXPWRCAP_NUM_SUBBANDS);
        /* Reset txcap state for all sub-bands to the common low cap (first "row")
         * when setting the txcap config.  Note txcapconfig is only allowed when
         * the driver is down.
         */
        memset(wlc_cmi->txcap_state, TXPWRCAPSTATE_LOW_CAP, TXPWRCAP_NUM_SUBBANDS);
        break;
    }
    case IOV_SVAL(IOV_TXCAPSTATE): {
        wl_txpwrcap_ctl_t *txcap_ctl = (wl_txpwrcap_ctl_t *)arg;
        int i;

        if (!WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            err = BCME_UNSUPPORTED;
            break;
        }
        if (txcap_ctl->version != 2) {
            err = BCME_VERSION;
            break;
        }

        /* Check that download has happened so we can do value range checks on values. */
        if (wlc_cmi->txcap_download == NULL) {
            err = BCME_NOTREADY;
            break;
        }

        for (i = 0; i < TXPWRCAP_NUM_SUBBANDS; i++) {
            if (wlc_cmi->txcap_config[i] == TXPWRCAPCONFIG_WCI2_AND_HOST) {
                if (txcap_ctl->ctl[i] > TXPWRCAPSTATE_HOST_HIGH_WCI2_HIGH_CAP) {
                    err = BCME_BADARG;
                    break;
                }
            } else {
                if (txcap_ctl->ctl[i] > TXPWRCAPSTATE_HIGH_CAP) {
                    err = BCME_BADARG;
                    break;
                }
            }
        }
        if (err) break;
        memcpy(wlc_cmi->txcap_state, txcap_ctl->ctl, TXPWRCAP_NUM_SUBBANDS);
        WL_TXPWRCAP(("%s: txcap_high_cap_timer deactivated, was %s\n", __FUNCTION__,
            wlc_cmi->txcap_high_cap_active ? "active" : "deactive"));
        wl_del_timer(wlc_cmi->wlc->wl, wlc_cmi->txcap_high_cap_timer);
        wlc_cmi->txcap_high_cap_active = 0;
        WL_TXPWRCAP(("%s: txcap_high_cap_timer activated for %d seconds\n",
            __FUNCTION__,
            wlc_cmi->txcap_high_cap_timeout));
        wlc_cmi->txcap_high_cap_active = 1;
        wl_add_timer(wlc_cmi->wlc->wl,
            wlc_cmi->txcap_high_cap_timer,
            1000 * wlc_cmi->txcap_high_cap_timeout,
            FALSE);
        wlc_channel_txcap_phy_update(wlc_cmi, NULL, NULL);
        break;
    }
    case IOV_GVAL(IOV_TXCAPHIGHCAPTO): {
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            *((uint32 *)arg) = wlc_cmi->txcap_high_cap_timeout;
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_SVAL(IOV_TXCAPHIGHCAPTO): {
        if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            wlc_cmi->txcap_high_cap_timeout = *((uint32 *)arg);
        } else {
            err = BCME_UNSUPPORTED;
        }
        break;
    }
    case IOV_GVAL(IOV_TXCAPDUMP): {
        wl_txpwrcap_dump_v3_t *txcap_dump = (wl_txpwrcap_dump_v3_t *)arg;
        uint8 *p;
        uint8 num_subbands;
        uint8 num_antennas;
        if (!WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
            err = BCME_UNSUPPORTED;
            break;
        }
        bzero(txcap_dump, sizeof(wl_txpwrcap_dump_t));
        txcap_dump->version = 3;
        txcap_dump->current_country[0] = wlc_cmi->country_abbrev[0];
        txcap_dump->current_country[1] = wlc_cmi->country_abbrev[1];
        txcap_dump->current_channel = wf_chspec_ctlchan(wlc_cmi->wlc->chanspec);
        txcap_dump->high_cap_state_enabled = wlc_cmi->txcap_high_cap_active;
        txcap_dump->wci2_cell_status_last = wlc_cmi->txcap_wci2_cell_status_last;
        memcpy(txcap_dump->config, wlc_cmi->txcap_config, TXPWRCAP_NUM_SUBBANDS);
        memcpy(txcap_dump->state, wlc_cmi->txcap_state, TXPWRCAP_NUM_SUBBANDS);
        if (wlc_cmi->txcap_download == NULL) {
            txcap_dump->download_present = 0;
        } else {
            txcap_dump->download_present = 1;
            num_subbands = wlc_cmi->txcap_download->num_subbands,
            txcap_dump->num_subbands = num_subbands;
            memcpy(txcap_dump->num_antennas_per_core,
                wlc_cmi->txcap_download->num_antennas_per_core,
                TXPWRCAP_MAX_NUM_CORES);
            num_antennas = wlc_cmi->txcap_download_num_antennas;
            txcap_dump->num_antennas = num_antennas;
            txcap_dump->num_cc_groups = wlc_cmi->txcap_download->num_cc_groups;
            txcap_dump->current_country_cc_group_info_index =
                wlc_cmi->current_country_cc_group_info_index;
            txcap_dump->cap_states_per_cc_group =
                wlc_cmi->txcap_cap_states_per_cc_group;
            p = (uint8 *)wlc_cmi->current_country_cc_group_info;
            p += OFFSETOF(txcap_file_cc_group_info_t, cc_list) +
                2 * wlc_cmi->current_country_cc_group_info->num_cc;
            memcpy(txcap_dump->host_low_wci2_low_cap, p, num_subbands * num_antennas);
            p += num_subbands * num_antennas;
            memcpy(txcap_dump->host_low_wci2_high_cap, p, num_subbands * num_antennas);
            if (wlc_cmi->txcap_cap_states_per_cc_group == 4) {
                p += num_subbands * num_antennas;
                memcpy(txcap_dump->host_high_wci2_low_cap, p,
                    num_subbands * num_antennas);
                p += num_subbands * num_antennas;
                memcpy(txcap_dump->host_high_wci2_high_cap, p,
                    num_subbands * num_antennas);
            }
        }
        break;
    }
#endif /* WLC_TXPWRCAP */

    case IOV_GVAL(IOV_DIS_CH_GRP):
        *((uint32 *)arg) = wlc_cmi->wlc->pub->_dis_ch_grp_conf |
                wlc_cmi->wlc->pub->_dis_ch_grp_user;
        WL_INFORM(("wl%d: ch grp conf: 0x%08x, user: 0x%08x\n", wlc_cmi->wlc->pub->unit,
            wlc_cmi->wlc->pub->_dis_ch_grp_conf, wlc_cmi->wlc->pub->_dis_ch_grp_user));
        break;
    case IOV_GVAL(IOV_DIS_CH_GRP_CONF):
        *((uint32 *)arg) = wlc_cmi->wlc->pub->_dis_ch_grp_conf;
        WL_INFORM(("wl%d: ch grp conf: 0x%08x, user: 0x%08x\n", wlc_cmi->wlc->pub->unit,
            wlc_cmi->wlc->pub->_dis_ch_grp_conf, wlc_cmi->wlc->pub->_dis_ch_grp_user));
        break;
    case IOV_GVAL(IOV_DIS_CH_GRP_USER):
        *((uint32 *)arg) = wlc_cmi->wlc->pub->_dis_ch_grp_user;
        WL_INFORM(("wl%d: ch grp conf: 0x%08x, user: 0x%08x\n", wlc_cmi->wlc->pub->unit,
            wlc_cmi->wlc->pub->_dis_ch_grp_conf, wlc_cmi->wlc->pub->_dis_ch_grp_user));
        break;
    case IOV_SVAL(IOV_DIS_CH_GRP_USER):
        if (IS_DIS_CH_GRP_VALID(int_val | wlc_cmi->wlc->pub->_dis_ch_grp_conf)) {
            wlc_cmi->wlc->pub->_dis_ch_grp_user = (uint32) int_val;
        } else {
            WL_ERROR(("wl%d: ignoring invalid dis_ch_grp from user 0x%x (conf: 0x%x)\n",
                    wlc_cmi->wlc->pub->unit, int_val,
                    wlc_cmi->wlc->pub->_dis_ch_grp_conf));
            err = BCME_BADARG;
        }
        WL_INFORM(("wl%d: ch grp conf: 0x%08x, user: 0x%08x\n", wlc_cmi->wlc->pub->unit,
            wlc_cmi->wlc->pub->_dis_ch_grp_conf, wlc_cmi->wlc->pub->_dis_ch_grp_user));
        break;

    case IOV_GVAL(IOV_IS_EDCRS_EU):
        *ret_int_ptr = wlc_cmi->wlc->is_edcrs_eu;
        break;

    case IOV_GVAL(IOV_RAND_CH):
        *ret_int_ptr = wlc_channel_rand_chanspec(wlc_cmi->wlc, int_val, FALSE);
        break;

    case IOV_SVAL(IOV_RAND_CH):
        *ret_int_ptr = wlc_channel_rand_chanspec(wlc_cmi->wlc, int_val, FALSE);
        break;

    default:
        err = BCME_UNSUPPORTED;
        break;
    }

    return err;
}

#ifdef WLC_TXPWRCAP
static void
wlc_txcap_high_cap_timer(void *arg)
{
    wlc_info_t* wlc = arg;

    ASSERT(wlc->cmi);

    WL_TXPWRCAP(("%s: txcap_high_cap_timer expired\n", __FUNCTION__));

    wlc->cmi->txcap_high_cap_active = 0;
    wlc_channel_txcap_phy_update(wlc->cmi, NULL, NULL);
}
#endif /* WLC_TXPWRCAP */

#if defined(BCMPCIEDEV_SROM_FORMAT) && defined(WLC_TXCAL)
static int
BCMATTACHFN(wlc_srom_read_blobdata)(wlc_info_t *wlc, wlc_cm_info_t *wlc_cmi)
{
    uint16 *buf;
    int caldata_size, retval;
    wl_dload_data_t *dload_data;
    wlc_info_t *wlc1;
    wlc_pub_t *pub = wlc->pub;
    const uint32 patrim_magic = PATRIM_MAGIC_SEQ;
    uint32 srom_size = get_srom_size(wlc->pub->sromrev);

    if (srom_size == 0) {
        caldata_size = sizeof(wl_dload_data_t);
    } else {
        caldata_size = sizeof(wl_dload_data_t) + (srom_size - 1);
    }

    dload_data = (wl_dload_data_t *) MALLOCZ(wlc->pub->osh, caldata_size);
    if (dload_data == NULL) {
        WL_ERROR(("MALLOC FAILURE \n"));
        return BCME_ERROR;
    }
    dload_data->flag = DL_BEGIN | DL_END;
    dload_data->dload_type = DL_TYPE_CLM;
    dload_data->len = caldata_size - OFFSETOF(wl_dload_data_t, data);
    dload_data->crc = 0;
    buf = (uint16*)dload_data->data;

    if (!_initvars_srom_pci_caldata(wlc->pub->sih, buf, wlc->pub->sromrev)) {
        if (memcmp(buf, BLOB_LITERAL, sizeof(BLOB_LITERAL) - 1) == 0) {
            retval = wlc_blob_download(wlc_cmi->clmload_wbi, dload_data->flag,
                (uint8 *)buf, dload_data->len,
                wlc_handle_cal_dload_wrapper);
            wlc->cmi->blobload_status = retval;
            WL_ERROR(("Blob download status: retval:%d\n", retval));
        } else if (memcmp(buf, &patrim_magic, sizeof(patrim_magic)) == 0) {
            uint32 hnd_crc32, crc_offset = 0;

            paoffset_srom_t * paoffset = (paoffset_srom_t *)buf;
            if (paoffset->num_slices == 1 || paoffset->num_slices == 2) {
                if (paoffset->num_slices == 1)
                    crc_offset = paoffset->slice0_offset +
                    paoffset->slice0_len;
                else if (paoffset->num_slices == 2)
                    crc_offset = paoffset->slice1_offset +
                    paoffset->slice1_len;

                hnd_crc32 = hndcrc32((uint8*)buf, crc_offset, 0xffffffff);
                hnd_crc32 = hnd_crc32 ^ 0xffffffff;
                if (*(uint32*)(buf + (crc_offset/2)) == hnd_crc32) {
                    wlc_phy_read_patrim_srom(wlc->pi,
                        (int16 *)(((uint8 *)buf) + paoffset->slice0_offset),
                        paoffset->slice0_len);
                }
                else {
                    WL_ERROR(("CRC mismatch !!!\n"));
                }
            }
            else {
                WL_ERROR(("Invalid number of slices\n"));
            }

        } else {
            WL_ERROR(("Cal data in SROM Not valid\n"));
        }
    }
    else {
        WL_ERROR(("caldata read failed\n"));
    }
    MFREE(pub->osh, dload_data, sizeof(wl_dload_data_t));

    return BCME_OK;
}
#endif /* BCMPCIEDEV_SROM_FORMAT && WLC_TXCAL */

static int
wlc_channel_ppr_ru_prealloc(wlc_cm_info_t *wlc_cmi)
{
    int ppr_ru_buf_i;
    uint buf_size = ppr_ru_size();
    wlc_cm_ppr_buffer_t *ppr_ru_buf;

    for (ppr_ru_buf_i = 0; ppr_ru_buf_i < PPR_RU_BUF_NUM; ppr_ru_buf_i++) {
        ppr_ru_buf = &(wlc_cmi->ppr_ru_buf[ppr_ru_buf_i]);
        ppr_ru_buf->lock = 0;
        if ((ppr_ru_buf->txpwr = (int8 *)MALLOCZ(wlc_cmi->wlc->osh, buf_size)) == NULL) {
            WL_ERROR(("wl%d: %s: out of memory", wlc_cmi->pub->unit, __FUNCTION__));
            return BCME_NOMEM;
        }
    }
    return BCME_OK;
}

static void
wlc_channel_ppr_ru_prealloc_free(wlc_cm_info_t *wlc_cmi)
{
    int ppr_ru_buf_i;
    wlc_cm_ppr_buffer_t *ppr_ru_buf;

    for (ppr_ru_buf_i = 0; ppr_ru_buf_i < PPR_RU_BUF_NUM; ppr_ru_buf_i++) {
        ppr_ru_buf = &(wlc_cmi->ppr_ru_buf[ppr_ru_buf_i]);
        if (ppr_ru_buf->txpwr) {
            MFREE(wlc_cmi->wlc->osh, ppr_ru_buf->txpwr, ppr_ru_size());
        }
    }
}

/* Release prealloc RU PPR buf */
static void
wlc_channel_release_prealloc_ppr_ru_buf(wlc_cm_info_t *wlc_cmi, int ppr_ru_buf_i)
{
    ASSERT(wlc_cmi->ppr_ru_buf[ppr_ru_buf_i].lock != 0);
    wlc_cmi->ppr_ru_buf[ppr_ru_buf_i].lock = 0;
}

/* Acquire ru ppr memory from available prealloc RU PPR buf */
static int
wlc_channel_acquire_ppr_ru_from_prealloc_buf(wlc_cm_info_t *wlc_cmi, ppr_ru_t **ru_txpwr)
{
    int ppr_ru_buf_i;
    wlc_cm_ppr_buffer_t *ppr_ru_buf;

    for (ppr_ru_buf_i = 0; ppr_ru_buf_i < PPR_BUF_NUM; ppr_ru_buf_i++) {
        ppr_ru_buf = &(wlc_cmi->ppr_ru_buf[ppr_ru_buf_i]);
        if (ppr_ru_buf->lock == 1) {
            continue;
        }
        if ((*ru_txpwr = ppr_ru_create_prealloc(ppr_ru_buf->txpwr, ppr_ru_size()))
            == NULL) {
            return BCME_NOMEM;
        }
        ppr_ru_buf->lock = 1;
        return ppr_ru_buf_i;
    }
    /* Prealloc RU PPR buf is not expected to be insufficient */
    WL_ERROR(("wl%d: %s: Out of Prealloc RU PPR buf\n", wlc_cmi->pub->unit, __FUNCTION__));
    ASSERT(0);
    return BCME_NOMEM;
}

static int
wlc_channel_ppr_prealloc(wlc_cm_info_t *wlc_cmi)
{
    int ppr_buf_i;
    uint buf_size = ppr_size(ppr_get_max_bw());
    wlc_cm_ppr_buffer_t *ppr_buf;

    for (ppr_buf_i = 0; ppr_buf_i < PPR_BUF_NUM; ppr_buf_i++) {
        ppr_buf = &(wlc_cmi->ppr_buf[ppr_buf_i]);
        ppr_buf->lock = 0;
        if ((ppr_buf->txpwr = (int8 *)MALLOCZ(wlc_cmi->wlc->osh, buf_size)) == NULL) {
            WL_ERROR(("wl%d: %s: out of memory", wlc_cmi->pub->unit, __FUNCTION__));
            return BCME_NOMEM;
        }
    }
    return BCME_OK;
}

static void
wlc_channel_ppr_prealloc_free(wlc_cm_info_t *wlc_cmi)
{
    int ppr_buf_i;
    wlc_cm_ppr_buffer_t *ppr_buf;

    for (ppr_buf_i = 0; ppr_buf_i < PPR_BUF_NUM; ppr_buf_i++) {
        ppr_buf = &(wlc_cmi->ppr_buf[ppr_buf_i]);
        if (ppr_buf->txpwr) {
            MFREE(wlc_cmi->wlc->osh, ppr_buf->txpwr, ppr_size(ppr_get_max_bw()));
        }
    }
}

/* Release prealloc PPR buf */
static void
wlc_channel_release_prealloc_ppr_buf(wlc_cm_info_t *wlc_cmi, int ppr_buf_i)
{
    ASSERT(wlc_cmi->ppr_buf[ppr_buf_i].lock != 0);
    wlc_cmi->ppr_buf[ppr_buf_i].lock = 0;
}

/* Acquire ppr memory from available prealloc PPR buf */
static int
wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cm_info_t *wlc_cmi, ppr_t **txpwr, wl_tx_bw_t bw)
{
    int ppr_buf_i;
    wlc_cm_ppr_buffer_t *ppr_buf;

    for (ppr_buf_i = 0; ppr_buf_i < PPR_BUF_NUM; ppr_buf_i++) {
        ppr_buf = &(wlc_cmi->ppr_buf[ppr_buf_i]);
        if (ppr_buf->lock == 1) {
            continue;
        }
        if ((*txpwr = ppr_create_prealloc(bw, ppr_buf->txpwr, ppr_size(bw))) == NULL) {
            return BCME_NOMEM;
        }
        ppr_buf->lock = 1;
        return ppr_buf_i;
    }
    /* Prealloc PPR buf is not expected to be insufficient */
    WL_ERROR(("wl%d: %s: Out of Prealloc PPR buf\n", wlc_cmi->pub->unit, __FUNCTION__));
    ASSERT(0);
    return BCME_NOMEM;
}

wlc_cm_info_t *
BCMATTACHFN(wlc_channel_mgr_attach)(wlc_info_t *wlc)
{
    clm_result_t result;
    wlc_cm_info_t *wlc_cmi;
    const char *ccode = getvar(wlc->pub->vars, rstr_ccode);
    char country_abbrev[WLC_CNTRY_BUF_SZ];
    wlc_pub_t *pub = wlc->pub;
    enum wlc_bandunit bandunit;

    WL_TRACE(("wl%d: wlc_channel_mgr_attach\n", wlc->pub->unit));

    if ((wlc_cmi = (wlc_cm_info_t *)MALLOCZ(pub->osh, sizeof(wlc_cm_info_t))) == NULL) {
        WL_ERROR(("wl%d: %s: out of memory, malloced %d bytes", pub->unit,
            __FUNCTION__, MALLOCED(pub->osh)));
        return NULL;
    }

    wlc_cmi->pub = pub;
    wlc_cmi->wlc = wlc;
    /* XXX TEMPORARY HACK.  Since wlc's cmi is set by the caller with the
     * return value/handle, we can't use it via wlc until we return.  Yet
     * because are slowly morphy the code in steps, we do indeed use it
     * from inside wlc_channel.  This won't happen eventually.  So the hack:
     * set wlc'c cmi from inside right now!
     */
    wlc->cmi = wlc_cmi;

#if defined(WLC_TXPWRCAP) && !defined(WLC_TXPWRCAP_DISABLED)
    wlc->pub->_txpwrcap = TRUE;
#endif

    if ((wlc_cmi->clmload_wbi = wlc_blob_attach(wlc)) == NULL) {
        MFREE(pub->osh, wlc_cmi, sizeof(wlc_cm_info_t));
        return NULL;
    }
#if defined(WLC_TXPWRCAP)
    if (WLTXPWRCAP_ENAB(wlc)) {
        if ((wlc_cmi->txcapload_wbi = wlc_blob_attach(wlc)) == NULL) {
            MODULE_DETACH(wlc_cmi->clmload_wbi, wlc_blob_detach);
            MFREE(pub->osh, wlc_cmi, sizeof(wlc_cm_info_t));
            return NULL;
        }
        memset(wlc_cmi->txcap_config, TXPWRCAPCONFIG_WCI2, TXPWRCAP_NUM_SUBBANDS);
        memset(wlc_cmi->txcap_state, TXPWRCAPSTATE_LOW_CAP, TXPWRCAP_NUM_SUBBANDS);
        if (!(wlc_cmi->txcap_high_cap_timer = wl_init_timer(wlc->wl,
            wlc_txcap_high_cap_timer, wlc, "high cap timer"))) {
            WL_ERROR(("wl%d: %s wl_init_timer for txcap_high_cap_timer failed\n",
                wlc->pub->unit, __FUNCTION__));
            MODULE_DETACH(wlc_cmi->txcapload_wbi, wlc_blob_detach);
            MODULE_DETACH(wlc_cmi->clmload_wbi, wlc_blob_detach);
            MFREE(pub->osh, wlc_cmi, sizeof(wlc_cm_info_t));
            return NULL;
        }
        /* Last cell status is unknown */
        wlc_cmi->txcap_wci2_cell_status_last = 2;
    }
#endif /* WLC_TXPWRCAP */

    /* init the per chain limits to max power so they have not effect */
    FOREACH_WLC_BAND(wlc, bandunit) {
        memset(&wlc_cmi->bandstate[bandunit].chain_limits, WLC_TXPWR_MAX,
               sizeof(struct wlc_channel_txchain_limits));
    }

    /* get the SPROM country code or local, required to initialize channel set below */
    bzero(country_abbrev, WLC_CNTRY_BUF_SZ);
    if (ccode) {
#ifndef ATE_BUILD
#ifndef OPENSRC_IOV_IOCTL
        int err;
#endif /* OPENSRC_IOV_IOCTL */
#endif /* ATE_BUILD */
        strncpy(country_abbrev, ccode, WLC_CNTRY_BUF_SZ - 1);
#ifndef ATE_BUILD
#ifndef OPENSRC_IOV_IOCTL
        err = wlc_cntry_external_to_internal(country_abbrev,
            sizeof(country_abbrev));
        if (err != BCME_OK) {
            /* XXX Gross Hack. In non-BCMDBG non-WLTEST mode,
             * we want to reject country abbreviations ALL and RDR.
             * So convert them to something really bogus, and
             * rely on other code to reject them in favour
             * of a default.
             */
            strncpy(country_abbrev, "__", WLC_CNTRY_BUF_SZ - 1);
        }
#endif /* OPENSRC_IOV_IOCTL */
#endif /* ATE_BUILD */
    } else {
        /* Internally and in the CLM xml use "ww" for legacy ccode of zero,
           i.e. '\0\0'.  Some very old hardware used a zero ccode in SROM
           as an aggregate identifier (along with a regrev).  There never
           was an actual country code of zero but just some aggregates.
           When the CLM was re-designed to use xml, a country code of zero
           was no longer legal.  Any xml references were changed to "ww",
           which was already being used as the preferred alternate to
           using zero.  Here at attach time we are translating a SROM/NVRAM
           value of zero to srom ccode value of "ww".  The below code makes
           it look just like the SROM/NVRAM had the value of "ww" for these
           very old hardware.
         */
        strncpy(country_abbrev, "ww", WLC_CNTRY_BUF_SZ - 1);
    }

#if defined(BCMDBG) || defined(WLTEST) || defined(ATE_BUILD)
    /* convert "ALL/0" country code to #a/0 */
    if (!strncmp(country_abbrev, "ALL", WLC_CNTRY_BUF_SZ)) {
        strncpy(country_abbrev, "#a", sizeof(country_abbrev) - 1);
    }
#endif /* BCMDBG || WLTEST || ATE_BUILD */

    /* Pre-alloc buf for PPR to avoid frequent mem alloc/free whenever the chanspec change */
    if (wlc_channel_ppr_prealloc(wlc_cmi)) {
        WL_ERROR(("wl%d: %s wlc_channel_ppr_prealloc failed\n",
            wlc->pub->unit, __FUNCTION__));
        goto fail;
    }

    /* Pre-alloc buf for RU PPR to avoid frequent mem alloc/free whenever the chanspec change */
    if (wlc_channel_ppr_ru_prealloc(wlc_cmi)) {
        WL_ERROR(("wl%d: %s wlc_channel_ppr_ru_prealloc failed\n",
            wlc->pub->unit, __FUNCTION__));
        goto fail;
    }

    strncpy(wlc_cmi->srom_ccode, country_abbrev, WLC_CNTRY_BUF_SZ - 1);
    wlc_cmi->srom_regrev = getintvar(wlc->pub->vars, rstr_regrev);

    result = clm_init(&clm_header);
    ASSERT(result == CLM_RESULT_OK);

    /* these are initialised to zero until they point to malloced data */
    wlc_cmi->clm_base_dataptr = NULL;
    wlc_cmi->clm_base_data_len = 0;

    result = wlc_channel_init_ccode(wlc_cmi, country_abbrev, wlc_cmi->srom_regrev,
        sizeof(country_abbrev));

    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s failed\n", wlc->pub->unit, __FUNCTION__));
        goto fail;
    }

    BCM_REFERENCE(result);

#ifdef WLTDLS
    /* setupreq */
    if (TDLS_ENAB(pub)) {
        if (wlc_ier_add_build_fn(wlc->ier_tdls_srq, DOT11_MNG_REGCLASS_ID,
            wlc_channel_tdls_calc_rc_ie_len, wlc_channel_tdls_write_rc_ie, wlc_cmi)
            != BCME_OK) {
            WL_ERROR(("wl%d: %s wlc_ier_add_build_fn failed, "
                "reg class in tdls setupreq\n", wlc->pub->unit, __FUNCTION__));
            goto fail;
        }
        /* setupresp */
        if (wlc_ier_add_build_fn(wlc->ier_tdls_srs, DOT11_MNG_REGCLASS_ID,
            wlc_channel_tdls_calc_rc_ie_len, wlc_channel_tdls_write_rc_ie, wlc_cmi)
            != BCME_OK) {
            WL_ERROR(("wl%d: %s wlc_ier_add_build_fn failed, "
                "reg class in tdls setupresp\n", wlc->pub->unit, __FUNCTION__));
            goto fail;
        }
    }
#endif /* WLTDLS */
#ifdef WL11H
    if (WL11H_ENAB(wlc)) {
        uint16 build_rclassfstbmp =
            FT2BMP(FC_BEACON) |
            FT2BMP(FC_PROBE_RESP) |
#ifdef STA
            FT2BMP(FC_ASSOC_REQ) |
            FT2BMP(FC_REASSOC_REQ) |
#endif
            FT2BMP(FC_PROBE_REQ);

        /* bcn/prbreq/prbrsp/assocreq/reassocreq */
        if (wlc_iem_add_build_fn_mft(wlc->iemi, build_rclassfstbmp, DOT11_MNG_REGCLASS_ID,
                wlc_channel_calc_rclass_ie_len, wlc_channel_write_rclass_ie,
                wlc_cmi) != BCME_OK) {
            WL_ERROR(("wl%d: %s: wlc_iem_add_build_fn failed, he cap ie\n",
                wlc->pub->unit,    __FUNCTION__));
            goto fail;
        }
    }
#endif /* WL11H */

    /* register module */
    if (wlc_module_register(wlc->pub, cm_iovars, "cm", wlc_cmi, wlc_cm_doiovar,
        NULL, NULL, NULL)) {
        WL_ERROR(("wl%d: %s wlc_module_register() failed\n", wlc->pub->unit, __FUNCTION__));

        goto fail;
    }

#if defined(BCMDBG)
    wlc_dump_register(wlc->pub, "locale", wlc_channel_dump_locale, wlc);
    wlc_dump_register(wlc->pub, "txpwr_reg",
                      (dump_fn_t)wlc_channel_dump_reg_ppr, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "txpwr_local",
                      (dump_fn_t)wlc_channel_dump_reg_local_ppr, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "txpwr_srom",
                      (dump_fn_t)wlc_channel_dump_srom_ppr, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "txpwr_margin",
                      (dump_fn_t)wlc_channel_dump_margin, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "country_regrevs",
                      (dump_fn_t)wlc_channel_supported_country_regrevs, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "agg_map",
                      (dump_fn_t)wlc_dump_country_aggregate_map, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "txpwr_reg_max",
                      (dump_fn_t)wlc_dump_max_power_per_channel, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "clm_limits",
                      (dump_fn_t)wlc_dump_clm_limits, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "clm_he_limits",
                      (dump_fn_t)wlc_dump_clm_he_limits, (void *)wlc_cmi);
    wlc_dump_register(wlc->pub, "clm_ru_limits",
                      (dump_fn_t)wlc_dump_clm_ru_limits, (void *)wlc_cmi);
#endif
#if defined(BCMPCIEDEV_SROM_FORMAT) && defined(WLC_TXCAL)
    if (wlc_srom_read_blobdata(wlc, wlc_cmi) == BCME_ERROR)
        goto fail;
#endif
    return wlc_cmi;

    goto fail;

fail:    /* error handling */
    wlc->cmi = NULL;
    MODULE_DETACH(wlc_cmi, wlc_channel_mgr_detach);
    return NULL;
}

static dload_error_status_t
wlc_handle_clm_dload(wlc_info_t *wlc, wlc_blob_segment_t *segments, uint32 segment_count)
{
    wlc_cm_info_t *wlc_cmi = wlc->cmi;
    dload_error_status_t status = DLOAD_STATUS_DOWNLOAD_SUCCESS;
    clm_result_t clm_result = CLM_RESULT_OK;
    clm_country_t country;
    char country_abbrev[WLC_CNTRY_BUF_SZ];
    struct clm_data_header* clm_dataptr;
    int clm_data_len;
    uint32 chip;

    /* Make sure we have a chip id segment and clm data segemnt */
    if (segment_count < 2)
        return DLOAD_STATUS_CLM_BLOB_FORMAT;

    /* Check to see if chip id segment is correct length */
    if (segments[1].length != 4)
        return DLOAD_STATUS_CLM_BLOB_FORMAT;

    /* Check to see if chip id matches this chip's actual value */
    chip = load32_ua(segments[1].data);
    if (chip != CHIPID(wlc_cmi->pub->sih->chip))
        return DLOAD_STATUS_CLM_MISMATCH;

    /* Process actual clm data segment */
    clm_dataptr = (struct clm_data_header *)(segments[0].data);
    clm_data_len = segments[0].length;

    /* At this point forward we are responsible for freeing this data pointer */
    segments[0].data = NULL;

    /* Free any previously downloaded base data */
    if (wlc_cmi->clm_base_dataptr != NULL) {
        MFREE(wlc_cmi->pub->osh, wlc_cmi->clm_base_dataptr,
            wlc_cmi->clm_base_data_len);
    }
    if (clm_dataptr != NULL) {
        WL_NONE(("wl%d: Pointing API at new base data: v%s\n",
            wlc_cmi->pub->unit, clm_dataptr->clm_version));
        clm_result = clm_init(clm_dataptr);
        if (clm_result != CLM_RESULT_OK) {
            WL_ERROR(("wl%d: %s: Error loading new base CLM"
                " data.\n",
                wlc_cmi->pub->unit, __FUNCTION__));
            status = DLOAD_STATUS_CLM_DATA_BAD;
            MFREE(wlc_cmi->pub->osh, clm_dataptr,
                clm_data_len);
        }
    }
    if ((clm_dataptr == NULL) || (clm_result != CLM_RESULT_OK)) {
        WL_NONE(("wl%d: %s: Reverting to base data.\n",
            wlc_cmi->pub->unit, __FUNCTION__));
        clm_init(&clm_header);
        wlc_cmi->clm_base_data_len = 0;
        wlc_cmi->clm_base_dataptr = NULL;
    } else {
        wlc_cmi->clm_base_dataptr = clm_dataptr;
        wlc_cmi->clm_base_data_len = clm_data_len;
    }

    if (wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country) ==
        CLM_RESULT_OK)
        wlc_cmi->country = country;
    else
        wlc_cmi->country = 0;

    bzero(country_abbrev, WLC_CNTRY_BUF_SZ);
    strncpy(country_abbrev, wlc_cmi->srom_ccode, WLC_CNTRY_BUF_SZ - 1);

    wlc_channel_init_ccode(wlc_cmi, country_abbrev, wlc_cmi->srom_regrev,
        sizeof(country_abbrev));

    return status;
}

void
BCMATTACHFN(wlc_channel_mgr_detach)(wlc_cm_info_t *wlc_cmi)
{
    if (wlc_cmi) {
        wlc_info_t *wlc = wlc_cmi->wlc;
        wlc_pub_t *pub = wlc->pub;

        wlc_module_unregister(wlc->pub, "cm", wlc_cmi);

#ifdef WLC_TXPWRCAP
        if (wlc_cmi->txcap_high_cap_timer) {
            wl_del_timer(wlc->wl, wlc_cmi->txcap_high_cap_timer);
            wl_free_timer(wlc->wl, wlc_cmi->txcap_high_cap_timer);
        }
        wlc_blob_detach(wlc_cmi->txcapload_wbi);
#endif /* WLC_TXPWRCAP */

        wlc_blob_detach(wlc_cmi->clmload_wbi);

        if (wlc_cmi->clm_base_dataptr != NULL) {
            MFREE(pub->osh, wlc_cmi->clm_base_dataptr,
                wlc_cmi->clm_base_data_len);
        }

        wlc_channel_ppr_prealloc_free(wlc_cmi);
        wlc_channel_ppr_ru_prealloc_free(wlc_cmi);

        MFREE(pub->osh, wlc_cmi, sizeof(wlc_cm_info_t));
    }
}

const char* wlc_channel_country_abbrev(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->country_abbrev;
}

const char* wlc_channel_ccode(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->ccode;
}

const char *wlc_channel_srom_ccode(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->srom_ccode;
}

uint wlc_channel_regrev(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->regrev;
}

uint wlc_channel_country(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->country;
}

void wlc_channel_setcountry(wlc_cm_info_t *wlc_cmi, clm_country_t country)
{
    wlc_cmi->country = country;
}

uint wlc_channel_srom_regrev(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->srom_regrev;
}

uint16 wlc_channel_locale_flags(wlc_cm_info_t *wlc_cmi)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    return wlc_cmi->bandstate[wlc->band->bandunit].locale_flags;
}

uint16 wlc_channel_locale_flags_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    return wlc_cmi->bandstate[bandunit].locale_flags;
}

/*
 * return the first valid chanspec for the locale, if one is not found and hw_fallback is true
 * then return the first h/w supported chanspec.
 */
chanspec_t
wlc_default_chanspec(wlc_cm_info_t *wlc_cmi, bool hw_fallback)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    chanspec_t  chspec;

    chspec = wlc_create_chspec(wlc, 0);
    /* try to find a chanspec that's valid in this locale */
    if ((chspec = wlc_next_chanspec(wlc_cmi, chspec, CHAN_TYPE_ANY, 0)) == INVCHANSPEC)
        /* try to find a chanspec valid for this hardware */
        if (hw_fallback)
            chspec = phy_utils_chanspec_band_firstch(
                WLC_PI(wlc),
                wlc->band->bandtype);
    return chspec;
}

/*
 * Return the next channel's chanspec.
 */
chanspec_t
wlc_next_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t cur_chanspec, int type, bool any_band)
{
    uint8 ch;
    uint8 cur_chan = CHSPEC_CHANNEL(cur_chanspec);
    chanspec_t chspec;

    /* 0 is an invalid chspec, routines trying to find the first available channel should
     * now be using wlc_default_chanspec (above)
     */
    ASSERT(cur_chanspec);

    /* current channel must be valid */
    if (cur_chan > MAXCHANNEL) {
        return ((chanspec_t)INVCHANSPEC);
    }
    /* Try all channels in current band */
    ch = cur_chan + 1;
    for (; ch <= MAXCHANNEL; ch++) {
        if (ch == MAXCHANNEL)
            ch = 0;
        if (ch == cur_chan)
            break;
        /* create the next channel spec */
        chspec = cur_chanspec & ~WL_CHANSPEC_CHAN_MASK;
        chspec |= ch;
        if (wlc_valid_chanspec(wlc_cmi, chspec)) {
            if ((type == CHAN_TYPE_ANY) ||
            (type == CHAN_TYPE_CHATTY && !wlc_quiet_chanspec(wlc_cmi, chspec)) ||
            (type == CHAN_TYPE_QUIET && wlc_quiet_chanspec(wlc_cmi, chspec)))
                return chspec;
        }
    }

    if (!any_band)
        return ((chanspec_t)INVCHANSPEC);

    /* Couldn't find any in current band, try other band */
    ch = cur_chan + 1;
    for (; ch <= MAXCHANNEL; ch++) {
        if (ch == MAXCHANNEL)
            ch = 0;
        if (ch == cur_chan)
            break;

        /* create the next channel spec */
        chspec = cur_chanspec & ~(WL_CHANSPEC_CHAN_MASK | WL_CHANSPEC_BAND_MASK);
        chspec |= (ch | WL_CHANNEL_2G5G_BAND(ch));

        if (wlc_valid_chanspec_db(wlc_cmi, chspec)) {
            if ((type == CHAN_TYPE_ANY) ||
            (type == CHAN_TYPE_CHATTY && !wlc_quiet_chanspec(wlc_cmi, chspec)) ||
            (type == CHAN_TYPE_QUIET && wlc_quiet_chanspec(wlc_cmi, chspec)))
                return chspec;
        }
    }

    return ((chanspec_t)INVCHANSPEC);
}

chanspec_t
wlc_default_chanspec_by_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    struct wlcband *band;
    chanspec_t chanspec;

    band = wlc_get_band(wlc_cmi->wlc, bandunit);
    if (!band)
        return INVCHANSPEC;

    chanspec = wf_create_20MHz_chspec(band->first_ch,
        wlc_bandunit2chspecband(bandunit));
    if (!wlc_valid_chanspec_db(wlc_cmi, chanspec)) {
        chanspec = wlc_next_chanspec(wlc_cmi,
            chanspec, CHAN_TYPE_ANY, TRUE);
    }
    return chanspec;
}

/** return chanvec for a given country code and band */
bool
wlc_channel_get_chanvec(struct wlc_info *wlc, const char* country_abbrev,
    int bandtype, chanvec_t *channels)
{
    clm_band_t band;
    clm_result_t result = CLM_RESULT_ERR;
    clm_country_t country;
    clm_country_locales_t locale;
    chanvec_t unused;
    uint i;
    chanvec_t channels_20;
    char ccode[WLC_CNTRY_BUF_SZ];
    uint regrev;

    if (!strcmp(wlc->cmi->country_abbrev, country_abbrev) ||
        !strcmp(wlc->cmi->ccode, country_abbrev)) {
        /* If the desired ccode matches the current advertised ccode
         * or the current ccode, set regrev for the current regrev
         * and set ccode for the current ccode
         */
        strncpy(ccode, wlc->cmi->ccode, WLC_CNTRY_BUF_SZ-1);
        ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
        regrev = wlc->cmi->regrev;
    } else {
        /* Set default regrev for 0 */
        strncpy(ccode, country_abbrev, WLC_CNTRY_BUF_SZ-1);
        ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
        regrev = 0;
    }

    result = wlc_country_lookup(wlc, ccode, regrev, &country);
    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: Unsupported country code %s/%d\n",
            wlc->pub->unit, __FUNCTION__, ccode, regrev));
        return FALSE;
    }

    result = wlc_get_locale(country, &locale);
    if (bandtype != WLC_BAND_2G && bandtype != WLC_BAND_5G && bandtype != WLC_BAND_6G)
        return FALSE;

    band = BANDTYPE2CLMBAND(bandtype);
    wlc_locale_get_channels(&locale, band, channels, &unused);

    /* don't mask 2GHz channels, but check 5G/6G channels */
    if (bandtype == WLC_BAND_5G || bandtype == WLC_BAND_6G) {
        clm_channels_params_t params;

        clm_channels_params_init(&params);
        params.bw = CLM_BW_20;
        clm_valid_channels(&locale, bandtype == WLC_BAND_5G ? CLM_BAND_5G: CLM_BAND_6G,
                &params, (clm_channels_t*)&channels_20);
        for (i = 0; i < sizeof(chanvec_t); i++) {
            channels->vec[i] &= channels_20.vec[i];
        }
    }

    return TRUE;
}

static int
wlc_valid_countrycode_channels(wlc_cm_info_t *wlc_cmi, char *mapped_ccode, uint *mapped_regrev,
    clm_country_t country)
{
    clm_country_locales_t locale;
    clm_band_t clm_band;
    uint16 flags;
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint j, chan;
    wlcband_t * band;
    chanvec_t sup_chan;
    chanvec_t * temp_chan;
    wlc_cm_band_t band_state;
    char bandbuf[12];
    clm_result_t result = wlc_get_locale(country, &locale);

    if (result != CLM_RESULT_OK) {
        return BCME_BADARG;
    }

    /* malloc to prevent stack size exceeding 1024 bytes for NIC builds */
    if ((temp_chan = (chanvec_t *)MALLOCZ(wlc->osh, sizeof(chanvec_t))) == NULL) {
        WL_ERROR(("wl%d: %s: out of memory", wlc_cmi->pub->unit, __FUNCTION__));
        return BCME_NOMEM;
    }

    band = wlc->band;

    clm_band = BANDTYPE2CLMBAND(band->bandtype);
    result = wlc_get_flags(&locale, clm_band, &flags);
    band_state.locale_flags = flags;

    wlc_locale_get_channels(&locale, clm_band,
        &band_state.valid_channels,
        temp_chan);

    /* set the channel availability,
     * masking out the channels that may not be supported on this phy
     */
    phy_radio_get_valid_chanvec(WLC_PI_BANDUNIT(wlc, band->bandunit),
        band->bandtype, &sup_chan);
    for (j = 0; j < sizeof(chanvec_t); j++) {
        band_state.valid_channels.vec[j] &= sup_chan.vec[j];
    }
    MFREE(wlc->osh, temp_chan, sizeof(chanvec_t));

    /* search for the existence of any valid channel */
    for (chan = 0; chan < MAXCHANNEL; chan++) {
        if (isset(band_state.valid_channels.vec, chan)) {
            break;
        }
    }
    if (chan == MAXCHANNEL) {
        wlc_get_bands_str(wlc, bandbuf, sizeof(bandbuf));
        WL_ERROR(("wl%d: %s: no valid channel for %s/%d. bands %s band %s bandlocked %d\n",
            wlc->pub->unit, __FUNCTION__, mapped_ccode, *mapped_regrev, bandbuf,
            wlc_bandunit_name(wlc->band->bandunit), wlc->bandlocked));
        return BCME_BADARG;
    }

    if ((band_state.locale_flags & WLC_NO_MIMO) && N_ENAB(wlc->pub)) {
        wlc_get_bands_str(wlc, bandbuf, sizeof(bandbuf));
        WL_ERROR(("wl%d: %s: no MCS rates for %s/%d. nbands %s band %s bandlocked %d,"
            " nmode need to be disabled.\n", wlc->pub->unit, __FUNCTION__,
            mapped_ccode, *mapped_regrev, bandbuf,
            wlc_bandunit_name(wlc->band->bandunit), wlc->bandlocked));
        return BCME_BADARG;
    }
    return BCME_OK;
}

int
wlc_valid_countrycode(wlc_cm_info_t *wlc_cmi, const char *country_abbrev, const char *ccode,
    uint regrev)
{
    clm_result_t result = CLM_RESULT_ERR;
    clm_country_t country;
    char mapped_ccode[WLC_CNTRY_BUF_SZ];
    int ret;

    if (ccode[0] == '\0') {
        strncpy(mapped_ccode, country_abbrev, WLC_CNTRY_BUF_SZ-1);
        mapped_ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
    } else {
        strncpy(mapped_ccode, ccode, WLC_CNTRY_BUF_SZ-1);
        mapped_ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
    }
    /* find the matching built-in country definition */
    result = wlc_country_lookup_direct(mapped_ccode, regrev, &country);

    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: Unsupported country code %s/%d\n",
            wlc_cmi->wlc->pub->unit, __FUNCTION__, mapped_ccode, regrev));
        return BCME_BADARG;
    }

    ret = wlc_valid_countrycode_channels(wlc_cmi, mapped_ccode, &regrev, country);

    return ret;
}

/* set the driver's current country and regulatory information using a country code
 * as the source. Lookup built in country information found with the country code.
 */
int
wlc_set_countrycode(wlc_cm_info_t *wlc_cmi, const char* ccode, uint regrev)
{
    int retval;

    WL_NONE(("wl%d: %s: ccode \"%s\"\n", wlc_cmi->wlc->pub->unit, __FUNCTION__, ccode));
    retval = wlc_set_countrycode_rev(wlc_cmi, ccode, regrev);

    if (retval == BCME_OK)
        wlc_phy_set_country(WLC_PI(wlc_cmi->wlc), wlc_channel_ccode(wlc_cmi));
    else
        wlc_phy_set_country(WLC_PI(wlc_cmi->wlc), NULL);

    return retval;
}

int
wlc_set_countrycode_rev(wlc_cm_info_t *wlc_cmi, const char* ccode, uint regrev)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    clm_result_t result = CLM_RESULT_ERR;
    clm_country_t country;
    char country_abbrev[WLC_CNTRY_BUF_SZ] = { 0 };

    BCM_REFERENCE(wlc);
    WL_NONE(("wl%d: %s: (country_abbrev \"%s\", ccode \"%s\", regrev %d) SPROM \"%s\"/%u\n",
             wlc->pub->unit, __FUNCTION__,
             country_abbrev, ccode, regrev, wlc_cmi->srom_ccode, wlc_cmi->srom_regrev));

    /* find the matching built-in country definition */
    result = wlc_country_lookup_direct(ccode, regrev, &country);

    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: Unsupported country code %s/%d\n",
                  wlc->pub->unit, __FUNCTION__, ccode, regrev));
        return BCME_BADARG;
    }

    /* Set the driver state for the country.
     * Getting the advertised country code from CLM.
     * Else use the one comes from ccode.
     */
    if (wlc_lookup_advertised_cc(country_abbrev, country)) {
        wlc_set_country_common(wlc_cmi, country_abbrev,
        ccode, regrev, country);
    } else {
        wlc_set_country_common(wlc_cmi, ccode, ccode, regrev, country);
    }

    return 0;
}

/* set the driver's newband with the channels */
void
wlc_channels_init_ext(wlc_cm_info_t *wlc_cmi)
{
    wlc_channels_init(wlc_cmi, wlc_cmi->country);
}

static bool
wlc_11b_mimo_disabled_in_clm(wlc_info_t *wlc)
{
    clm_power_limits_t limits;
    clm_country_t country;
    clm_country_locales_t locale;
    clm_limits_params_t lim_params;
    chanvec_t sup_chan, temp_chan;
    uint8 chan;

    country = wlc->cmi->country;
    if (wlc_get_locale(country, &locale) != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: fail to get locale. ccode %s/%d\n",
            wlc->pub->unit, __FUNCTION__,
            wlc_channel_ccode(wlc->cmi), wlc_channel_regrev(wlc->cmi)));
        return FALSE;
    }

    /* Get 2G channels supported in CLM for the country */
    wlc_locale_get_channels(&locale, CLM_BAND_2G, &sup_chan, &temp_chan);

    /* Choose the first supported channel */
    for (chan = 1; chan <= CH_MAX_2G_CHANNEL; chan++) {
        if (isset(sup_chan.vec, chan))
            break;
    }

    if (chan > CH_MAX_2G_CHANNEL) {
        WL_ERROR(("wl%d: %s: No 2G channel supported.\n",
            wlc->pub->unit, __FUNCTION__));
        return FALSE;
    }

    if (clm_limits_params_init(&lim_params) != CLM_RESULT_OK) {
        return FALSE;
    }

    /* Retrieve CLM power limits */
    if (clm_limits(&locale, CLM_BAND_2G, chan, 0, CLM_LIMITS_TYPE_CHANNEL, &lim_params, &limits)
         != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: fail to retrieve clm limits\n",
            wlc->pub->unit, __FUNCTION__));
        return FALSE;
    }

    /* When 11b mimo is disabled in CLM, only DSSS is enabled,
     * DSSS_MULTIx are disabled in CLM.
     */
    if ((limits.limit[WL_RATE_1X4_DSSS_1] == WL_RATE_DISABLED) &&
        (limits.limit[WL_RATE_1X3_DSSS_1] == WL_RATE_DISABLED) &&
        (limits.limit[WL_RATE_1X2_DSSS_1] == WL_RATE_DISABLED) &&
        (limits.limit[WL_RATE_1X1_DSSS_1] != WL_RATE_DISABLED)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* set the driver's current country and regulatory information using a country code
 * as the source. Look up built in country information found with the country code.
 */
static void
wlc_set_country_common(wlc_cm_info_t *wlc_cmi,
                       const char* country_abbrev,
                       const char* ccode, uint regrev, clm_country_t country)
{
    clm_result_t result = CLM_RESULT_ERR;
    clm_country_locales_t locale;
    uint16 flags;
    wlc_info_t *wlc = wlc_cmi->wlc;
    char prev_country_abbrev[WLC_CNTRY_BUF_SZ];
    enum wlc_bandunit bandunit;
    uint8 core_mask = 1;
    int i;

#if defined WLTXPWR_CACHE
    wlc_phy_txpwr_cache_invalidate(phy_tpc_get_txpwr_cache(WLC_PI(wlc)));
#endif    /* WLTXPWR_CACHE */

    /* Ensure NUL string terminator before printing */
    wlc_cmi->country_abbrev[WLC_CNTRY_BUF_SZ - 1] = '\0';
    wlc_cmi->ccode[WLC_CNTRY_BUF_SZ - 1] = '\0';
    WL_REGULATORY(("wl%d: %s: country/abbrev/ccode/regrev "
            "from 0x%04x/%s/%s/%d to 0x%04x/%s/%s/%d\n",
            wlc->pub->unit, __FUNCTION__,
            wlc_cmi->country, wlc_cmi->country_abbrev, wlc_cmi->ccode, wlc_cmi->regrev,
            country, country_abbrev, ccode, regrev));

    if (wlc_cmi->country == country && wlc_cmi->regrev == regrev &&
            wlc_cmi->country_abbrev[0] && wlc_cmi->ccode[0] &&
            strncmp(wlc_cmi->country_abbrev, country_abbrev, WLC_CNTRY_BUF_SZ) == 0 &&
            strncmp(wlc_cmi->ccode, ccode, WLC_CNTRY_BUF_SZ) == 0) {
        WL_REGULATORY(("wl%d: %s: Avoid setting current country again.\n",
            wlc->pub->unit, __FUNCTION__));
        return;
    }

    /* save current country state */
    wlc_cmi->country = country;

    bzero(&prev_country_abbrev, WLC_CNTRY_BUF_SZ);
    strncpy(prev_country_abbrev, wlc_cmi->country_abbrev, WLC_CNTRY_BUF_SZ - 1);

    strncpy(wlc_cmi->country_abbrev, country_abbrev, WLC_CNTRY_BUF_SZ-1);
    strncpy(wlc_cmi->ccode, ccode, WLC_CNTRY_BUF_SZ-1);
    wlc_cmi->regrev = regrev;

    result = wlc_get_locale(country, &locale);
    ASSERT(result == CLM_RESULT_OK);

    result = wlc_get_flags(&locale, CLM_BAND_2G, &flags);
    ASSERT(result == CLM_RESULT_OK);
    BCM_REFERENCE(result);

    if (flags & WLC_EDCRS_EU) {
        wlc_phy_set_locale(WLC_PI(wlc), REGION_EU);
    } else {
        wlc_phy_set_locale(WLC_PI(wlc), REGION_OTHER);
    }

#ifdef WL_BEAMFORMING
    if (TXBF_ENAB(wlc->pub)) {
        if (flags & WLC_TXBF) {
            wlc_stf_set_txbf(wlc, TRUE);
        } else {
            wlc_stf_set_txbf(wlc, FALSE);
        }
    }
#endif

    /* disable/restore nmode based on country regulations */
    if ((flags & WLC_NO_MIMO) && BAND_ENABLED(wlc, BAND_5G_INDEX)) {
        result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
        ASSERT(result == CLM_RESULT_OK);
    }

    if ((flags & WLC_NO_MIMO) && BAND_ENABLED(wlc, BAND_6G_INDEX)) {
        result = wlc_get_flags(&locale, CLM_BAND_6G, &flags);
        ASSERT(result == CLM_RESULT_OK);
    }

    if (flags & WLC_NO_MIMO) {
        wlc_set_nmode(wlc->hti, OFF);
        wlc->stf->no_cddstbc = TRUE;
    } else {
        wlc->stf->no_cddstbc = FALSE;
        wlc_prot_n_mode_reset(wlc->prot_n, FALSE);
    }

    FOREACH_WLC_BAND(wlc, bandunit) {
        wlc_stf_ss_update(wlc, wlc->bandstate[bandunit]);
    }

#if defined(AP) && defined(RADAR)
    if (RADAR_ENAB(wlc->pub) && BAND_ENABLED(wlc, BAND_5G_INDEX)) {
        phy_radar_detect_mode_t mode;
        result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);

        mode = ISDFS_JP(flags) ? RADAR_DETECT_MODE_JP :
            ISDFS_UK(flags) ? RADAR_DETECT_MODE_UK :
            ISDFS_EU(flags) ? RADAR_DETECT_MODE_EU : RADAR_DETECT_MODE_FCC;

        phy_radar_detect_mode_set(WLC_PI(wlc), mode);
    }
#endif /* AP && RADAR */

    /* Set caps before wlc_channels_init() so recalc target has correct cap values */
#ifdef WLC_TXPWRCAP
    if (WLTXPWRCAP_ENAB(wlc)) {
        wlc_channel_txcap_set_country(wlc_cmi);
        /* In case driver is up, but current channel didn't change as a
         * result of a country change, we need to make sure we update the
         * phy txcaps based on this new country's values.
         */
        /* XXX BUG? There are paths in wlc_channel_set_chanspec()
         * which call wlc_bmac_set_chanspec() without first calling
         * wlc_channel_reg_limits() to computing new txpwr limits for
         * a newly * adopted country or possibly new power constraints.
         * This seems to be related to txpwr caching/channel change optimizations.
         * Did this code consider these other reasons for txpwr limit changes
         * besides a channel change?
         */
        wlc_channel_txcap_phy_update(wlc_cmi, NULL, NULL);
    }
#endif /* WLC_TXPWRCAP */

    wlc_channels_init(wlc_cmi, country);

    /* Country code changed */
    if (strlen(prev_country_abbrev) > 1 &&
        strncmp(wlc_cmi->country_abbrev, prev_country_abbrev,
                strlen(wlc_cmi->country_abbrev)) != 0) {
        /* need to reset chan_blocked */
        if (WLDFS_ENAB(wlc->pub) && wlc->dfs)
            wlc_dfs_reset_all(wlc->dfs);
        /* need to reset afe_override */
        wlc_channel_spurwar_locale(wlc_cmi, wlc->chanspec);

        wlc_mac_event(wlc, WLC_E_COUNTRY_CODE_CHANGED, NULL,
            0, 0, 0, wlc_cmi->country_abbrev, strlen(wlc_cmi->country_abbrev) + 1);
    }
    else {
        if ((!strncmp(wlc_cmi->country_abbrev, "#a", sizeof("#a") - 1)) &&
            WLDFS_ENAB(wlc->pub) && wlc->dfs)
            wlc_dfs_reset_all(wlc->dfs);
    }
#ifdef WLOLPC
    if (OLPC_ENAB(wlc_cmi->wlc)) {
        WL_RATE(("olpc process: ccrev=%s regrev=%d\n", ccode, regrev));
        /* olpc needs to flush any stored chan info and cal if needed */
        wlc_olpc_eng_reset(wlc_cmi->wlc->olpc_info);
    }
#endif /* WLOLPC */

    wlc_ht_frameburst_limit(wlc->hti);

    wlc_country_clear_locales(wlc->cntry);

    if (wlc_11b_mimo_disabled_in_clm(wlc)) {
        /* When 11b mimo is disabled in CLM, only DSSS is enabled,
         * DSSS_MULTIx are disabled in CLM. Thus can only enable one CCK txcore.
         */
        for (i = 0; i < WLC_NUM_TXCHAIN_MAX; i++) {
            if (wlc->stf->txchain & core_mask) {
                wlc->stf->txcore_override[CCK_IDX] = core_mask;
                break;
            }
            core_mask <<= 1;
        }
    }

    return;
}

/* returns true if current country's CLM flag matches CBP_FCC.
 * CBP_FCC is defined in CLM for countries that follow FCC 6GHz contention based protocol rule.
 */
bool
wlc_is_cbp_fcc(wlc_info_t *wlc)
{
    wlc_cm_info_t *wlc_cmi = wlc->cmi;

    if (wlc_cmi->bandstate[BAND_6G_INDEX].locale_flags & WLC_CBP_FCC) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* returns true if current country's CLM flag matches EDCRS_EU.
 * (Some countries like Brazil will return false here though using DFS as in EU)
 * EDCRS_EU flag is defined in CLM for countries that follow EU harmonized standards.
 */
bool
wlc_is_edcrs_eu(struct wlc_info *wlc)
{
    wlc_cm_info_t *wlc_cmi = wlc->cmi;
    clm_result_t result;
    clm_country_locales_t locale;
    clm_country_t country;
    uint16 flags;
    bool edcrs_eu_flag_set;

    result = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);

    if (result != CLM_RESULT_OK) {
        return FALSE;
    }

    result = wlc_get_locale(country, &locale);
    if (result != CLM_RESULT_OK) {
        return FALSE;
    }

    result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
    if (result != CLM_RESULT_OK) {
        return FALSE;
    }

    edcrs_eu_flag_set = ((flags & WLC_EDCRS_EU) == WLC_EDCRS_EU);

    WL_REGULATORY(("wl%d: %s: EDCRS_EU flag is %sset for country %s (flags:0x%02X)\n",
            wlc->pub->unit, __FUNCTION__,
            edcrs_eu_flag_set?"":"not ", wlc_cmi->ccode, flags));
    return edcrs_eu_flag_set;
}

#ifdef RADAR
/* returns true if current CLM flag matches DFS_EU or DFS_UK for the operation mode/band;
 * (Some countries like Brazil will return true here though not in EU/EDCRS)
 */
bool
wlc_is_dfs_eu_uk(struct wlc_info *wlc)
{
    wlc_cm_info_t *wlc_cmi = wlc->cmi;
    clm_result_t result;
    clm_country_locales_t locale;
    clm_country_t country;
    uint16 flags;

    if (!BAND_ENABLED(wlc, BAND_5G_INDEX) &&
#ifdef BGDFS_2G
            !BGDFS_2G_ENAB(wlc->pub) &&
#endif /* BGDFS_2G */
            TRUE)
        return FALSE;

    result = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
    if (result != CLM_RESULT_OK)
        return FALSE;

    result = wlc_get_locale(country, &locale);
    if (result != CLM_RESULT_OK)
        return FALSE;

    result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
    if (result != CLM_RESULT_OK)
        return FALSE;

    return (ISDFS_EU(flags) || ISDFS_UK(flags));
}

bool
wlc_is_european_weather_radar_channel(struct wlc_info *wlc, chanspec_t chanspec)
{
    uint8 weather_sb[] = { 120, 124, 128 }; /* EU weather channel 20MHz sidebands */
    uint8 channel, idx, weather_len = ARRAYSIZE(weather_sb);
    uint8 last_channel;

    if ((!wlc_valid_chanspec_db(wlc->cmi, chanspec) && wlc->band->bandtype != WLC_BAND_2G) ||
            !wlc_is_dfs_eu_uk(wlc)) {
        return FALSE;
    }

    FOREACH_20_SB_EFF(chanspec, channel, last_channel) {
        for (idx = 0; idx < weather_len; idx++) {
            if (channel == weather_sb[idx]) {
                return TRUE;
            }
        }
    }

    return FALSE;
}
#endif /* RADAR */

/* Lookup a country info structure from a null terminated country code
 * The lookup is case sensitive.
 */
clm_result_t
wlc_country_lookup(struct wlc_info *wlc, const char* ccode, uint regrev, clm_country_t *country)
{
    clm_result_t result = CLM_RESULT_ERR;

    /* check for currently supported ccode size */
    if (strlen(ccode) > (WLC_CNTRY_BUF_SZ - 1)) {
        WL_ERROR(("wl%d: %s: ccode \"%s\" too long for match\n",
            WLCWLUNIT(wlc), __FUNCTION__, ccode));
        return CLM_RESULT_ERR;
    }

    /* find the matching built-in country definition */
    result = wlc_country_lookup_direct(ccode, regrev, country);

    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d: %s: No country found, failed lookup. ccode %s regrev %d\n",
                 wlc->pub->unit, __FUNCTION__, ccode, regrev));
    }

    return result;
}

clm_result_t
clm_aggregate_country_lookup(const ccode_t cc, unsigned int rev, clm_agg_country_t *agg)
{
    return clm_agg_country_lookup(cc, rev, agg);
}

clm_result_t
clm_aggregate_country_map_lookup(const clm_agg_country_t agg, const ccode_t target_cc,
    unsigned int *rev)
{
    return clm_agg_country_map_lookup(agg, target_cc, rev);
}

#if defined(BCMDBG)
static int
wlc_dump_country_aggregate_map(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    const char *cur_ccode = wlc_cmi->ccode;
    uint cur_regrev = wlc_cmi->regrev;
    clm_agg_country_t agg = 0;
    clm_result_t result;
    int agg_iter;

    /* Use "ww", WorldWide, for the lookup value for '\0\0' */
    if (cur_ccode[0] == '\0')
        cur_ccode = "ww";

    clm_iter_init(&agg_iter);
    if ((result = clm_aggregate_country_lookup(cur_ccode, cur_regrev, &agg)) == CLM_RESULT_OK) {
        clm_agg_map_t map_iter;
        ccode_t cc;
        unsigned int rev;

        bcm_bprintf(b, "Map for %s/%u ->\n", cur_ccode, cur_regrev);
        clm_iter_init(&map_iter);
        while ((result = clm_agg_map_iter(agg, &map_iter, cc, &rev)) == CLM_RESULT_OK) {
            bcm_bprintf(b, "%c%c/%u\n", cc[0], cc[1], rev);
        }
    } else {
        bcm_bprintf(b, "No lookaside table for %s/%u\n", cur_ccode, cur_regrev);
    }
    return 0;

}
#endif

/* Lookup a country info structure from a null terminated country
 * abbreviation and regrev directly with no translation.
 */
clm_result_t
wlc_country_lookup_direct(const char* ccode, uint regrev, clm_country_t *country)
{
    return clm_country_lookup(ccode, regrev, country);
}

#if defined(STA) && defined(WL11D)
/* Lookup a country info structure considering only legal country codes as found in
 * a Country IE; two ascii alpha followed by " ", "I", or "O".
 * Do not match any user assigned application specifc codes that might be found
 * in the driver table.
 */
clm_result_t
wlc_country_lookup_ext(wlc_info_t *wlc, const char *ccode, clm_country_t *country)
{
    clm_result_t result = CLM_RESULT_NOT_FOUND;
    char country_str_lookup[WLC_CNTRY_BUF_SZ] = { 0 };
    uint regrev = 0;

    /* only allow ascii alpha uppercase for the first 2 chars, and " ", "I", "O" for the 3rd */
    /* Also allow operating classes supported for dot11CountryString as per Annex E in spec */
    if (!((0x80 & ccode[0]) == 0 && bcm_isupper(ccode[0]) &&
          (0x80 & ccode[1]) == 0 && bcm_isupper(ccode[1]) &&
          (ccode[2] == ' ' || ccode[2] == 'I' || ccode[2] == 'O'||
           ccode[2] == BCMWIFI_RCLASS_TYPE_US || ccode[2] == BCMWIFI_RCLASS_TYPE_EU ||
           ccode[2] == BCMWIFI_RCLASS_TYPE_JP || ccode[2] == BCMWIFI_RCLASS_TYPE_GBL)))
        return result;

    /* for lookup in the driver table of country codes, only use the first
     * 2 chars, ignore the 3rd character " ", "I", "O" qualifier
     */
    country_str_lookup[0] = ccode[0];
    country_str_lookup[1] = ccode[1];

    /* do not match ISO 3166-1 user assigned country codes that may be in the driver table */
    if (!strcmp("AA", country_str_lookup) ||    /* AA */
        !strcmp("ZZ", country_str_lookup) ||    /* ZZ */
        country_str_lookup[0] == 'X' ||        /* XA - XZ */
        (country_str_lookup[0] == 'Q' &&        /* QM - QZ */
         (country_str_lookup[1] >= 'M' && country_str_lookup[1] <= 'Z')))
        return result;

    if (!strcmp(wlc->cmi->country_abbrev, country_str_lookup) ||
        !strcmp(wlc->cmi->ccode, country_str_lookup)) {
        /* If the desired ccode matches the current advertised ccode
         * or the current ccode, set regrev for the current regrev
         * and set ccode for the current ccode
         */
        strncpy(country_str_lookup, wlc->cmi->ccode, WLC_CNTRY_BUF_SZ-1);
        country_str_lookup[WLC_CNTRY_BUF_SZ-1] = '\0';
        regrev = wlc->cmi->regrev;
    }

    return wlc_country_lookup(wlc, country_str_lookup, regrev, country);
}
#endif /* STA && WL11D */

#ifdef BGDFS_2G
/* use this for basic check about DFS radar channels on a 2G band locked card
 * Returns TRUE if the chanspec is a DFS radar channel else returns FALSE
 */
bool
wlc_channel_2g_dfs_chan(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    clm_result_t result;
    clm_country_locales_t locale;
    clm_country_t country;
    const uint8 *vec = radar_set1.vec;
    uint16 flags;
    uint8 channel;
    uint8 last_channel;

    result = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d country look up failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return FALSE;
    }

    result = wlc_get_locale(country, &locale);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d getting country locale failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return FALSE;
    }

    result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d getting locale flags failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return FALSE;
    }

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        if (isset(vec, channel)) {
            return TRUE;
        }
    }
    WL_REGULATORY(("wl%d chspec 0x%x is NOT a radar channel\n",
        WLCWLUNIT(wlc_cmi->wlc), chspec));
    return FALSE;
}
int
wlc_channel_set_phy_radar_detect(wlc_cm_info_t *wlc_cmi)
{
    clm_result_t result;
    clm_country_locales_t locale;
    clm_country_t country;
    phy_radar_detect_mode_t mode;
    uint16 flags;

    result = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d country look up failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return BCME_ERROR;
    }

    result = wlc_get_locale(country, &locale);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d getting country locale failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return BCME_ERROR;
    }

    result = wlc_get_flags(&locale, CLM_BAND_5G, &flags);
    if (result != CLM_RESULT_OK) {
        WL_REGULATORY(("wl%d getting locale flags failed\n", WLCWLUNIT(wlc_cmi->wlc)));
        return BCME_ERROR;
    }

    mode = ISDFS_JP(flags) ? RADAR_DETECT_MODE_JP :
        ISDFS_UK(flags) ? RADAR_DETECT_MODE_UK :
        ISDFS_EU(flags) ? RADAR_DETECT_MODE_EU : RADAR_DETECT_MODE_FCC;

    phy_radar_detect_mode_set(WLC_PI(wlc_cmi->wlc), mode);

    WL_REGULATORY(("wl%d %s phy_radar_detect_mode_set with mode %d flags 0x%x\n",
        WLCWLUNIT(wlc_cmi->wlc), __FUNCTION__, mode, flags));
    return BCME_OK;
}

#endif /* BGDFS_2G */

#if BAND5G
static void
wlc_channel_set_radar_chanvect(wlc_cm_info_t *wlc_cmi, wlcband_t *band, uint16 flags)
{
    uint j;
    const uint8 *vec = NULL;

    /* Return when No Flag for DFS TPC */
    if (!(flags & WLC_DFS_TPC)) {
        return;
    }

    vec = radar_set1.vec;

    for (j = 0; j < sizeof(chanvec_t); j++) {
        wlc_cmi->bandstate[band->bandunit].radar_channels->vec[j] =
            vec[j] &
            wlc_cmi->bandstate[band->bandunit].
            valid_channels.vec[j];
    }
}
#endif /* BAND5G */

static void
wlc_channels_init(wlc_cm_info_t *wlc_cmi, clm_country_t country)
{
    clm_country_locales_t locale;
    uint16 flags;
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint j;
    wlcband_t * band;
    chanvec_t sup_chan, temp_chan;
    enum wlc_bandunit bandunit;
    clm_result_t result = wlc_get_locale(country, &locale);

    if (result != CLM_RESULT_OK) {
        WL_ERROR(("wl%d: %s: fail to get locale. ccode %s/%d",
            wlc->pub->unit, __FUNCTION__,
            wlc_channel_ccode(wlc->cmi), wlc_channel_regrev(wlc->cmi)));
        ASSERT(0);
        return;
    }

    bzero(&wlc_cmi->restricted_channels, sizeof(chanvec_t));
    bzero(&wlc_cmi->locale_radar_channels, sizeof(chanvec_t));
    bzero(&wlc_cmi->allowed_5g_channels, sizeof(chanvec_t));
    bzero(&wlc_cmi->allowed_6g_channels, sizeof(chanvec_t));

    FOREACH_WLC_BAND(wlc, bandunit) {
        clm_band_t tmp_band;

        band = wlc->bandstate[bandunit];

        tmp_band = BANDTYPE2CLMBAND(band->bandtype);
        result = wlc_get_flags(&locale, tmp_band, &flags);
        wlc_cmi->bandstate[band->bandunit].locale_flags = flags;

        wlc_locale_get_channels(&locale, tmp_band,
            &wlc_cmi->bandstate[band->bandunit].valid_channels,
            &temp_chan);

        /* Initialize restricted channels.
         * 6GHz does not have restricted channels
         */
        if (tmp_band != CLM_BAND_6G) {
            for (j = 0; j < sizeof(chanvec_t); j++) {
                wlc_cmi->restricted_channels.vec[j] |= temp_chan.vec[j];
            }
        }
#if BAND5G /* RADAR */
        wlc_cmi->bandstate[band->bandunit].radar_channels =
            &wlc_cmi->locale_radar_channels;
        if (BAND_5G(band->bandtype)) {
            wlc_channel_set_radar_chanvect(wlc_cmi, band, flags);
        }

        if (BAND_5G(band->bandtype)) {
            clm_channels_params_t params;
            clm_bandwidth_t bw;

            clm_channels_params_init(&params);
            params.add = TRUE;

            for (bw = CLM_BW_20; bw < CLM_BW_NUM; bw++) {
                if (bw == CLM_BW_80_80)
                    params.this_80_80 = TRUE;

                params.bw = bw;
                clm_valid_channels(&locale, CLM_BAND_5G, &params,
                    (clm_channels_t*)&wlc_cmi->allowed_5g_channels);
                params.this_80_80 = FALSE;
            }
        }
#endif /* BAND5G */
#if BAND6G
        if (BAND_6G(band->bandtype)) {
            clm_channels_params_t params;
            clm_bandwidth_t bw;

            clm_channels_params_init(&params);
            params.add = TRUE;

            for (bw = CLM_BW_20; bw < CLM_BW_NUM; bw++) {
                if (bw == CLM_BW_80_80)
                    continue;

                params.bw = bw;
                clm_valid_channels(&locale, CLM_BAND_6G, &params,
                    (clm_channels_t*)&wlc_cmi->allowed_6g_channels);
            }
        }
#endif /* BAND6G */

        /* set the channel availability,
         * masking out the channels that may not be supported on this phy
         */
        phy_radio_get_valid_chanvec(WLC_PI_BANDUNIT(wlc, band->bandunit),
            band->bandtype, &sup_chan);
        for (j = 0; j < sizeof(chanvec_t); j++)
            wlc_cmi->bandstate[band->bandunit].valid_channels.vec[j] &=
                sup_chan.vec[j];
    } /* FOREACH_WLC_BAND */

    wlc_upd_restricted_chanspec_flag(wlc_cmi);
    wlc_quiet_channels_reset(wlc_cmi);
    wlc_channels_commit(wlc_cmi);

#if defined(WL_GLOBAL_RCLASS)
    wlc_update_rcinfo(wlc_cmi, TRUE);
#else
    wlc_update_rcinfo(wlc_cmi, FALSE);
#endif
}

/* Update the radio state (enable/disable) and tx power targets
 * based on a new set of channel/regulatory information
 */
static int
wlc_channels_commit(wlc_cm_info_t *wlc_cmi)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    phy_info_t *pi = WLC_PI(wlc);
    char bandbuf[12];
    uint chan = INVCHANNEL;
    ppr_t* txpwr = NULL;
    ppr_ru_t *ru_txpwr = NULL;
    int ppr_buf_i, ppr_ru_buf_i;
    int ret = BCME_OK;
    enum wlc_bandunit bandunit;
    bool valid_ch_found = FALSE;

    FOREACH_WLC_BAND_UNORDERED(wlc, bandunit) {
        wlcband_t *band = wlc->bandstate[bandunit];
        uint bandtype = band->bandtype;

        FOREACH_WLC_BAND_CHANNEL20(band, chan) {
            if (wlc_valid_channel20(wlc->cmi,
                CH20MHZ_CHSPEC(chan, BANDTYPE_CHSPEC(bandtype)), FALSE)) {
                valid_ch_found = TRUE;
                break;
            }
        }
    }

    /* based on the channel search above, set or clear WL_RADIO_COUNTRY_DISABLE */
    if (!valid_ch_found) {
        wlc_get_bands_str(wlc, bandbuf, sizeof(bandbuf));
        /* country/locale with no valid channels, set the radio disable bit */
        mboolset(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE);
        WL_ERROR(("wl%d: %s: no valid channel for \"%s\" bands %s bandlocked %d\n",
                  wlc->pub->unit, __FUNCTION__,
                  wlc_cmi->country_abbrev, bandbuf, wlc->bandlocked));
        ret = BCME_BADCHAN;
    } else if (mboolisset(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE)) {
        /* country/locale with valid channel, clear the radio disable bit */
        mboolclr(wlc->pub->radio_disabled, WL_RADIO_COUNTRY_DISABLE);
    }

    if (wlc->pub->up && valid_ch_found) {
        /* recompute tx power for new country info */

        /* XXX REVISIT johnvb  What chanspec should we use when changing country
         * and where do we get it if we don't create/set it ourselves, i.e.
         * wlc's chanspec or the phy's chanspec?  Also see the REVISIT comment
         * in wlc_channel_set_txpower_limit().
         */

        /* Where do we get a good chanspec? wlc, phy, set it ourselves? */

        ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cmi, &txpwr,
            PPR_CHSPEC_BW(wlc->chanspec));
        if (ppr_buf_i < 0)
            return BCME_NOMEM;
        ppr_ru_buf_i = wlc_channel_acquire_ppr_ru_from_prealloc_buf(wlc_cmi, &ru_txpwr);
        if (ppr_ru_buf_i < 0) {
            wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
            return BCME_NOMEM;
        }

        wlc_channel_reg_limits(wlc_cmi, wlc->chanspec, txpwr, ru_txpwr);
#ifdef WL11AX
        wlc_phy_set_ru_power_limits(pi, ru_txpwr);
#endif /* WL11AX */

        /* XXX REVISIT johvnb  When setting a new country is it OK to erase
         * the current 11h/local constraint?  If we do need to maintain the 11h
         * constraint, we could either cache it here in the channel code or just
         * have the higher level, non wlc_channel.c code, reissue a call to
         * wlc_channel_set_txpower_limit with the old constraint.  This requires
         * us to identify which of the few wlc_[channel]_set_countrycode calls could
         * have a previous constraint constraint active.  This raises the
         * related question of how a constraint goes away?  Caching the constraint
         * here in wlc_channel would be logical, but unfortunately it doesn't
         * currently have its own structure/state.
         */
        ppr_apply_max(txpwr, WLC_TXPWR_MAX);
        /* Where do we get a good chanspec? wlc, phy, set it ourselves? */
        wlc_phy_txpower_limit_set(pi, txpwr, wlc->chanspec);

        wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
        wlc_channel_release_prealloc_ppr_ru_buf(wlc_cmi, ppr_ru_buf_i);
    }

    return ret;
} /* wlc_channels_commit */

chanvec_t *
wlc_quiet_chanvec_get(wlc_cm_info_t *wlc_cmi)
{
    return &wlc_cmi->quiet_channels;
}

chanvec_t *
wlc_valid_chanvec_get(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    return &wlc_cmi->bandstate[bandunit].valid_channels;
}

/* reset the quiet channels vector to the union of the restricted and radar channel sets */
void
wlc_quiet_channels_reset(wlc_cm_info_t *wlc_cmi)
{
#if BAND5G
    wlc_info_t *wlc = wlc_cmi->wlc;
    enum wlc_bandunit bandunit;
    wlcband_t *band;
#endif /* BAND5G */

    /* initialize quiet channels for restricted channels */
    bcopy(&wlc_cmi->restricted_channels, &wlc_cmi->quiet_channels, sizeof(chanvec_t));

#if BAND5G /* RADAR */
    FOREACH_WLC_BAND(wlc, bandunit) {
        band = wlc->bandstate[bandunit];
        /* initialize quiet channels for radar if we are in spectrum management mode */
        if (WL11H_ENAB(wlc)) {
            uint j;
            const chanvec_t *chanvec;

            chanvec = wlc_cmi->bandstate[band->bandunit].radar_channels;
            for (j = 0; j < sizeof(chanvec_t); j++)
                wlc_cmi->quiet_channels.vec[j] |= chanvec->vec[j];
        }
    } /* FOREACH_WLC_BAND */
#endif /* BAND5G */
}

bool
wlc_quiet_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    wlc_pub_t *pub = wlc_cmi->wlc->pub;
    uint8 channel;
    uint8 last_channel;

    /* Quiet channels only exist in the 5G band */
    if (!CHSPEC_IS5G(chspec)) {
        return FALSE;
    }

    /* minimal bandwidth configuration check */
    if ((CHSPEC_IS40(chspec) && !(VHT_ENAB(pub) || N_ENAB(pub))) ||
            ((CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec) || CHSPEC_IS80(chspec)) &&
            !VHT_ENAB(pub))) {
        return FALSE;
    }

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        if (isset(wlc_cmi->quiet_channels.vec, channel)) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Channels that are shared with radar need to be marked as 'quite' (passive).
 *
 * @param[in] chspec         Caller provided chanspec, one chanspec may encompass multiple channels
 * @param[in] chspec_exclude All channels in this chanspec are not to be modified by this function
 */
void
wlc_set_quiet_chanspec_exclude(wlc_cm_info_t *wlc_cmi, chanspec_t chspec, chanspec_t chspec_exclude)
{
    uint8 channel, i, idx = 0, exclude_arr[8];
    bool must_exclude;
    uint8 last_channel;

    ASSERT(CHSPEC_IS5G(chspec));
    ASSERT(CHSPEC_IS5G(chspec_exclude));

    FOREACH_20_SB_EFF(chspec_exclude, channel, last_channel) {
        exclude_arr[idx++] = channel;
    }

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        must_exclude = FALSE;
        for (i = 0; i < idx; ++i) {
            if (exclude_arr[i] == channel) {
                must_exclude = TRUE;
                break;
            }
        }

        if (!must_exclude &&
            wlc_radar_chanspec(wlc_cmi, CH20MHZ_CHSPEC(channel, WL_CHANSPEC_BAND_5G))) {
            setbit(wlc_cmi->quiet_channels.vec, channel);
            WL_REGULATORY(("%s: Setting quiet bit for channel %d of chanspec 0x%x \n",
                __FUNCTION__, channel, chspec));
        }
    }
} /* wlc_set_quiet_chanspec_exclude */

void
wlc_set_quiet_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    wlc_pub_t *pub = wlc_cmi->wlc->pub;
    uint8 channel;
    uint8 last_channel;

    /* minimal bandwidth configuration check */
    if ((CHSPEC_IS40(chspec) && !(VHT_ENAB(pub) || N_ENAB(pub))) ||
            ((CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec) || CHSPEC_IS80(chspec)) &&
            !VHT_ENAB(pub))) {
        return;
    }

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        if (wlc_radar_chanspec(wlc_cmi, CHBW_CHSPEC(WL_CHANSPEC_BW_20, channel))) {
            setbit(wlc_cmi->quiet_channels.vec, channel);
            WL_REGULATORY(("%s: Setting quiet bit for channel %d of chanspec 0x%x \n",
                __FUNCTION__, channel, chspec));
        }
    }
}

void
wlc_clr_quiet_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    wlc_pub_t *pub = wlc_cmi->wlc->pub;
    uint8 channel;
    uint8 last_channel;

    if (!wlc_dfs_valid_ap_chanspec(wlc_cmi->wlc, chspec)) {
        WL_REGULATORY(("wl%d:%s:chspec[%x] is blocked, return \n",
            wlc_cmi->wlc->pub->unit, __FUNCTION__, chspec));
        return;
    }
    /* minimal bandwidth configuration check */
    if ((CHSPEC_IS40(chspec) && !(VHT_ENAB(pub) || N_ENAB(pub))) ||
            ((CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec) || CHSPEC_IS80(chspec)) &&
            !VHT_ENAB(pub))) {
        return;
    }

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        clrbit(wlc_cmi->quiet_channels.vec, channel);
    }
}

/**
 * Is the channel valid for the current locale, on one of the bands that the phy/radio supports ?
 * (but don't consider channels not available due to bandlocking)
 */
bool
wlc_valid_channel20(wlc_cm_info_t *wlc_cmi, chanspec_t chspec, bool current_bu)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    enum wlc_bandunit bandunit = CHSPEC_BANDUNIT(chspec);
    bool ret;

    ASSERT(CHSPEC_IS20(chspec));

    if ((wlc->bandlocked || current_bu) && !is_current_band(wlc, bandunit)) {
        return FALSE;
    }

    ret = wlc_valid_channel20_in_band(wlc_cmi, bandunit, CHSPEC_CHANNEL(chspec));
    return ret;
}

/* Is the channel valid for the current locale and specified band? */
bool
wlc_valid_channel20_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit, uint val)
{
    return ((val < MAXCHANNEL) &&
        isset(wlc_cmi->bandstate[bandunit].valid_channels.vec, val));
}

uint8*
wlc_channel_get_valid_channels_vec(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    return (wlc_cmi->bandstate[bandunit].valid_channels.vec);
}

/* Is the 40 MHz allowed for the current locale and specified band? */
bool
wlc_valid_40chanspec_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    return (((wlc_cmi->bandstate[bandunit].locale_flags & (WLC_NO_MIMO | WLC_NO_40MHZ)) == 0) &&
        wlc->pub->phy_bw40_capable);
}

/* Is 80 MHz allowed for the current locale and specified band? */
bool
wlc_valid_80chanspec_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    return (((wlc_cmi->bandstate[bandunit].locale_flags & (WLC_NO_MIMO | WLC_NO_80MHZ)) == 0) &&
        wlc->pub->phy_bw80_capable);
}

/* Is chanspec allowed to use 80Mhz bandwidth for the current locale? */
bool
wlc_valid_80chanspec(struct wlc_info *wlc, chanspec_t chanspec)
{
    uint16 locale_flags;

    locale_flags = wlc_channel_locale_flags_in_band(wlc->cmi, CHSPEC_BANDUNIT(chanspec));

    if (CHSPEC_IS6G(chanspec)) {
        return (HE_ENAB_BAND(wlc->pub, (CHSPEC_BANDTYPE(chanspec))) &&
         !(locale_flags & WLC_NO_80MHZ) &&
         WL_BW_CAP_80MHZ(wlc->bandstate[BAND_6G_INDEX]->bw_cap) &&
         wlc_valid_chanspec_db(wlc->cmi, (chanspec)));
    } else {
        return (VHT_ENAB_BAND(wlc->pub, (CHSPEC_BANDTYPE(chanspec))) &&
         !(locale_flags & WLC_NO_80MHZ) &&
         WL_BW_CAP_80MHZ(wlc->bandstate[BAND_5G_INDEX]->bw_cap) &&
         wlc_valid_chanspec_db(wlc->cmi, (chanspec)));
    }
}

/* Is 80+80 MHz allowed for the current locale and specified band? */
bool
wlc_valid_8080chanspec_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    return (((wlc_cmi->bandstate[bandunit].locale_flags & (WLC_NO_MIMO|WLC_NO_160MHZ)) == 0) &&
        WL_BW_CAP_160MHZ(wlc->bandstate[bandunit]->bw_cap));
}

/* Is chanspec allowed to use 80+80Mhz bandwidth for the current locale? */
bool
wlc_valid_8080chanspec(struct wlc_info *wlc, chanspec_t chanspec)
{
    uint16 locale_flags;
    int bandtype = CHSPEC_BANDTYPE(chanspec);
    enum wlc_bandunit bandunit = CHSPEC_BANDUNIT(chanspec);

    locale_flags = wlc_channel_locale_flags_in_band(wlc->cmi, CHSPEC_BANDUNIT(chanspec));

    return ((VHT_ENAB_BAND(wlc->pub, bandtype) || HE_ENAB_BAND(wlc->pub, bandtype)) &&
        !(locale_flags & WLC_NO_160MHZ) &&
        WL_BW_CAP_160MHZ(wlc->bandstate[bandunit]->bw_cap) &&
        wlc_valid_chanspec_db(wlc->cmi, (chanspec)));
}

/* Is 160 MHz allowed for the current locale and specified band? */
bool
wlc_valid_160chanspec_in_band(wlc_cm_info_t *wlc_cmi, enum wlc_bandunit bandunit)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    return (((wlc_cmi->bandstate[bandunit].locale_flags & (WLC_NO_MIMO|WLC_NO_160MHZ)) == 0) &&
        wlc->pub->phy_bw160_capable);
}

/* Is chanspec allowed to use 160Mhz bandwidth for the current locale? */
bool
wlc_valid_160chanspec(struct wlc_info *wlc, chanspec_t chanspec)
{
    uint16 locale_flags;
    int bandtype = CHSPEC_BANDTYPE(chanspec);

    locale_flags = wlc_channel_locale_flags_in_band(wlc->cmi, CHSPEC_BANDUNIT(chanspec));

    if (CHSPEC_IS6G(chanspec)) {
        return (HE_ENAB_BAND(wlc->pub, bandtype) &&
         !(locale_flags & WLC_NO_160MHZ) &&
         WL_BW_CAP_160MHZ(wlc->bandstate[BAND_6G_INDEX]->bw_cap) &&
         wlc_valid_chanspec_db(wlc->cmi, (chanspec)));
    } else {
        return ((VHT_ENAB_BAND(wlc->pub, bandtype) || HE_ENAB_BAND(wlc->pub, bandtype)) &&
                !(locale_flags & WLC_NO_160MHZ) &&
                WL_BW_CAP_160MHZ(wlc->bandstate[BAND_5G_INDEX]->bw_cap) &&
                wlc_valid_chanspec_db(wlc->cmi, (chanspec)));
    }
}

static void
wlc_channel_txpower_limits(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr)
{
    uint8 local_constraint;
    wlc_info_t *wlc = wlc_cmi->wlc;

    wlc_channel_reg_limits(wlc_cmi, wlc->chanspec, txpwr, NULL);

    local_constraint = wlc_tpc_get_local_constraint_qdbm(wlc->tpc);

    if (!AP_ONLY(wlc->pub)) {
        ppr_apply_constraint_total_tx(txpwr, local_constraint);
    }
}

static void
wlc_channel_spurwar_locale(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec)
{
#if ACCONF
    wlc_info_t *wlc = wlc_cmi->wlc;
    int override;
    uint rev;
    bool isCN, isX2, isX51A, dBpad;

    isX51A = (wlc->pub->sih->boardtype == BCM94360X51A) ? TRUE : FALSE;
    dBpad = (wlc->pub->boardflags4 & BFL4_SROM12_4dBPAD) ? 1 : 0;
    if (!isX51A && !dBpad)
        return;

    isCN = bcmp("CN", wlc_channel_country_abbrev(wlc_cmi), 2) ? FALSE : TRUE;
    isX2 = bcmp("X2", wlc_channel_country_abbrev(wlc_cmi), 2) ? FALSE : TRUE;
    rev = wlc_channel_regrev(wlc_cmi);

    if (D11REV_LE(wlc->pub->corerev, 40)) {
        wlc->stf->coremask_override = SPURWAR_OVERRIDE_OFF;
        return;
    }

    if (wlc_iovar_getint(wlc, "phy_afeoverride", &override) == BCME_OK) {
        override &= ~PHY_AFE_OVERRIDE_DRV;
        wlc->stf->coremask_override = SPURWAR_OVERRIDE_OFF;
        if (CHSPEC_IS5G(chanspec) &&
            ((isCN && rev == 204) || (isX2 && rev == 204) || /* X87 module */
             (isCN && rev == 242) || (isX2 && rev == 2242) || /* X238D module */
             (isX51A && ((isCN && rev == 40) || (isX2 && rev == 19))))) { /* X51A module */
            override |= PHY_AFE_OVERRIDE_DRV;
            if (isX51A || CHSPEC_IS20(chanspec))
                wlc->stf->coremask_override = SPURWAR_OVERRIDE_X51A;
            else if (dBpad)
                wlc->stf->coremask_override = SPURWAR_OVERRIDE_DBPAD;
        }
        wlc_iovar_setint(wlc, "phy_afeoverride", override);
    }
#endif /* ACCONF */
}

void
wlc_channel_set_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    wlc_stf_info_t *stf = wlc->stf;
    wlc_bsscfg_t *bsscfg = wlc->primary_bsscfg;
    int ppr_buf_i, ppr_ru_buf_i;
    ppr_t* txpwr = NULL;
    ppr_ru_t *ru_txpwr = NULL;
    int8 local_constraint_qdbm;
    wl_txpwrcap_tbl_t *txpwrcap_tbl_ptr = NULL;
    int *cellstatus_ptr = NULL;
    bool update_cap = FALSE;
#if defined WLTXPWR_CACHE
    tx_pwr_cache_entry_t* cacheptr;
#endif

    /* Save our copy of the chanspec */
		/* dump_flag_qqdx */
		printk("change wlc->chanspec:wlc_channel:wlc_channel_set_chanspe:(0x%04x:0x%04x)",wlc->chanspec,chanspec);
		/* dump_flag_qqdx */
    wlc->chanspec = chanspec;

    WLDURATION_ENTER(wlc_hw->wlc, DUR_CHAN_SET_CHSPEC);
    /* bw 160/80p80MHz for 4366 */
    if (WLC_PHY_AS_80P80(wlc, chanspec)) {
        /* For 160Mhz or 80+80Mhz, if phy is 80p80. Only 2x2 or 1x1 are avaiable.
         * We also need to adjust the mcsmap, 4->2, 2->1.
         * txstreams and rxstreams must be at least 2.
         */
        ASSERT((stf->txstreams > 1) && (stf->rxstreams > 1));
        /* Number of tx and rx streams must be even to combine
         * two 80 streams for 160Mhz.
         */
        ASSERT((stf->txstreams & 1) == 0 && (stf->rxstreams & 1) == 0);
        if (stf->op_txstreams >= 2) {
            stf->op_txstreams = stf->txstreams >> 1;
            stf->op_rxstreams = stf->rxstreams >> 1;
            update_cap = TRUE;
        }
    }

    WL_RATE(("wl%d: ch 0x%04x, u:%d, "
        "htc 0x%x, hrc 0x%x, htcc 0x%x, hrcc 0x%x, tc 0x%x, rc 0x%x, "
        "ts 0x%x, rs 0x%x, ots 0x%x, ors 0x%x\n",
        WLCWLUNIT(wlc), chanspec, update_cap, stf->hw_txchain, stf->hw_rxchain,
        stf->hw_txchain_cap, stf->hw_rxchain_cap, stf->txchain, stf->rxchain,
        stf->txstreams, stf->rxstreams, stf->op_txstreams, stf->op_rxstreams));
    if (update_cap) {
        wlc_ht_stbc_tx_set(wlc->hti, wlc->band->band_stf_stbc_tx);
        WL_INFORM(("Update STBC TX for HT/VHT/HE cap for chanspec(0x%x)\n", chanspec));
        wlc_default_rateset(wlc, &bsscfg->current_bss->rateset);
    }

#ifdef WLC_TXPWRCAP
    if (WLTXPWRCAP_ENAB(wlc_cmi->wlc)) {
        if (wlc_cmi->txcap_download != NULL) {
            txpwrcap_tbl_ptr = &(wlc_cmi->txpwrcap_tbl);
            cellstatus_ptr = &(wlc_cmi->cellstatus);
            wlc_channel_txcap_phy_update(wlc_cmi, txpwrcap_tbl_ptr, cellstatus_ptr);
        }
    }
#endif /* WLC_TXPWRCAP */
#if defined WLTXPWR_CACHE
    cacheptr = phy_tpc_get_txpwr_cache(WLC_PI(wlc));
    if (wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) != TRUE) {
        int result;
        chanspec_t kill_chan = 0;

        BCM_REFERENCE(result);

        if (last_chanspec != 0)
            kill_chan = wlc_phy_txpwr_cache_find_other_cached_chanspec(cacheptr,
                last_chanspec);

        if (kill_chan != 0) {
            wlc_phy_txpwr_cache_clear(wlc_cmi->pub->osh, cacheptr, kill_chan);
        }
        result = wlc_phy_txpwr_setup_entry(cacheptr, chanspec);
        ASSERT(result == BCME_OK);
    }
    last_chanspec = chanspec;

    if ((wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, 0) != WL_RATE_DISABLED) &&
        wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec)) {
        wlc_channel_update_txchain_offsets(wlc_cmi, NULL);
        wlc_bmac_set_chanspec(wlc->hw, chanspec,
            (wlc_quiet_chanspec(wlc_cmi, chanspec) != 0), NULL,
            txpwrcap_tbl_ptr, cellstatus_ptr);
        goto exit;
    }
#endif /* WLTXPWR_CACHE */
    WL_TSLOG(wlc, __FUNCTION__, TS_ENTER, 0);

    ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cmi, &txpwr,
        PPR_CHSPEC_BW(chanspec));
    if (ppr_buf_i < 0)
        goto exit;
    ppr_ru_buf_i = wlc_channel_acquire_ppr_ru_from_prealloc_buf(wlc_cmi, &ru_txpwr);
    if (ppr_ru_buf_i < 0) {
        wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
        goto exit;
    }
#ifdef SRHWVSDB
    /* If txpwr shms are saved, no need to go through power init fns again */
    if (wlc_srvsdb_save_valid(wlc, chanspec)) {
        goto apply_chanspec;
    }
#endif /* SRHWVSDB */
    wlc_channel_spurwar_locale(wlc_cmi, chanspec);
    wlc_channel_reg_limits(wlc_cmi, chanspec, txpwr, ru_txpwr);
    /* For APs, need to wait until reg limits are set before retrieving constraint. */
    local_constraint_qdbm = wlc_tpc_get_local_constraint_qdbm(wlc->tpc);
    if (!AP_ONLY(wlc->pub)) {
        ppr_apply_constraint_total_tx(txpwr, local_constraint_qdbm);
    }

#ifdef WL11AX
    wlc_phy_set_ru_power_limits(WLC_PI(wlc), ru_txpwr);
#endif /* WL11AX */
    wlc_channel_update_txchain_offsets(wlc_cmi, txpwr);

    WL_TSLOG(wlc, "After wlc_channel_update_txchain_offsets", 0, 0);

#ifdef SRHWVSDB
apply_chanspec:
#endif /* SRHWVSDB */
    wlc_bmac_set_chanspec(wlc->hw, chanspec,
        (wlc_quiet_chanspec(wlc_cmi, chanspec) != 0),
        txpwr, txpwrcap_tbl_ptr, cellstatus_ptr);
    wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
    wlc_channel_release_prealloc_ppr_ru_buf(wlc_cmi, ppr_ru_buf_i);
    WL_TSLOG(wlc, __FUNCTION__, TS_EXIT, 0);

    wlc_channel_psdlimit_check(wlc_cmi);

exit:
    WLDURATION_EXIT(wlc_hw->wlc, DUR_CHAN_SET_CHSPEC);
    return;
}

int
wlc_channel_set_txpower_limit(wlc_cm_info_t *wlc_cmi, uint8 local_constraint_qdbm)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    int ppr_buf_i, ppr_ru_buf_i;
    ppr_t *txpwr = NULL;
    ppr_ru_t *ru_txpwr = NULL;
    int8 tx_maxpwr;
    int8 txpwr_cap_min = wlc_tpc_get_pwr_cap_min(wlc->tpc);

    /* XXX REVISIT johnvb  Make sure wlc->chanspec updated first!  This is a hack.
     * Since the purpose of wlc_channel is to encapsulate the operations related to
     * country/regulatory including protecting/enforcing the restrictions in partial
     * open source builds, using wlc's "upper" copy of chanspec breaks the design.
     * While it would be possible to use the "lower" phy copy via wlc_phy_get_chanspec()
     * this would cause an extra call/RPC in a BMAC dongle build.  The "correct"
     * solution is making wlc_channel into a "object" with its own structure/state,
     * attach, detach, etc.
     */

    if (!wlc->clk)
        return BCME_NOCLK;

    ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cmi, &txpwr,
        PPR_CHSPEC_BW(wlc->chanspec));
    if (ppr_buf_i < 0)
        return BCME_NOMEM;
    ppr_ru_buf_i = wlc_channel_acquire_ppr_ru_from_prealloc_buf(wlc_cmi, &ru_txpwr);
    if (ppr_ru_buf_i < 0) {
        wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
        return BCME_NOMEM;
    }

    wlc_channel_reg_limits(wlc_cmi, wlc->chanspec, txpwr, ru_txpwr);

    if (!AP_ONLY(wlc->pub)) {
        ppr_apply_constraint_total_tx(txpwr, local_constraint_qdbm);
    }

#ifdef WL11AX
    wlc_phy_set_ru_power_limits(WLC_PI(wlc), ru_txpwr);
#endif /* WL11AX */

    tx_maxpwr = ppr_get_max(txpwr);

    /* Validate the new txpwr */
    if ((tx_maxpwr < txpwr_cap_min) ||
        (txpwr_cap_min == WL_RATE_DISABLED && tx_maxpwr <
         wlc_phy_maxtxpwr_lowlimit(WLC_PI(wlc)))) {

        wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
        wlc_channel_release_prealloc_ppr_ru_buf(wlc_cmi, ppr_ru_buf_i);

        return BCME_RANGE;
    }

#if defined WLTXPWR_CACHE
    wlc_phy_txpwr_cache_invalidate(phy_tpc_get_txpwr_cache(WLC_PI(wlc)));
#endif    /* WLTXPWR_CACHE */

    wlc_channel_update_txchain_offsets(wlc_cmi, txpwr);

    wlc_phy_txpower_limit_set(WLC_PI(wlc), txpwr, wlc->chanspec);

    wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);
    wlc_channel_release_prealloc_ppr_ru_buf(wlc_cmi, ppr_ru_buf_i);

    return 0;
}

clm_limits_type_t clm_chanspec_to_limits_type(chanspec_t chspec)
{
    clm_limits_type_t lt = CLM_LIMITS_TYPE_CHANNEL;

    if (CHSPEC_IS40(chspec)) {
        switch (CHSPEC_CTL_SB(chspec)) {
        case WL_CHANSPEC_CTL_SB_L:
            lt = CLM_LIMITS_TYPE_SUBCHAN_L;
            break;
        case WL_CHANSPEC_CTL_SB_U:
            lt = CLM_LIMITS_TYPE_SUBCHAN_U;
            break;
        default:
            ASSERT(0);
            break;
        }
    }
#ifdef WL11AC
    else if (CHSPEC_IS80(chspec) || CHSPEC_IS8080(chspec)) {
        switch (CHSPEC_CTL_SB(chspec)) {
        case WL_CHANSPEC_CTL_SB_LL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LL;
            break;
        case WL_CHANSPEC_CTL_SB_LU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LU;
            break;
        case WL_CHANSPEC_CTL_SB_UL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_UL;
            break;
        case WL_CHANSPEC_CTL_SB_UU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_UU;
            break;
        default:
            ASSERT(0);
            break;
        }
    } else if (CHSPEC_IS160(chspec)) {
        switch (CHSPEC_CTL_SB(chspec)) {
        case WL_CHANSPEC_CTL_SB_LLL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LLL;
            break;
        case WL_CHANSPEC_CTL_SB_LLU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LLU;
            break;
        case WL_CHANSPEC_CTL_SB_LUL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LUL;
            break;
        case WL_CHANSPEC_CTL_SB_LUU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_LUU;
            break;
        case WL_CHANSPEC_CTL_SB_ULL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_ULL;
            break;
        case WL_CHANSPEC_CTL_SB_ULU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_ULU;
            break;
        case WL_CHANSPEC_CTL_SB_UUL:
            lt = CLM_LIMITS_TYPE_SUBCHAN_UUL;
            break;
        case WL_CHANSPEC_CTL_SB_UUU:
            lt = CLM_LIMITS_TYPE_SUBCHAN_UUU;
            break;
        default:
            ASSERT(0);
            break;
        }
    }
#endif /* WL11AC */
    return lt;
}

#ifdef WL11AC
/* Converts limits_type of control channel to the limits_type of
 * the larger-BW subchannel(s) enclosing it (e.g. 40in80, 40in160, 80in160)
 */
clm_limits_type_t clm_get_enclosing_subchan(clm_limits_type_t ctl_subchan, uint lvl)
{
    clm_limits_type_t lt = ctl_subchan;
    if (lvl == 1) {
        /* Get 40in80 given 20in80, 80in160 given 20in160, 40in8080 given 20in8080 */
        switch (ctl_subchan) {
            case CLM_LIMITS_TYPE_SUBCHAN_LL:
            case CLM_LIMITS_TYPE_SUBCHAN_LU:
            case CLM_LIMITS_TYPE_SUBCHAN_LLL:
            case CLM_LIMITS_TYPE_SUBCHAN_LLU:
            case CLM_LIMITS_TYPE_SUBCHAN_LUL:
            case CLM_LIMITS_TYPE_SUBCHAN_LUU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_L;
                break;
            case CLM_LIMITS_TYPE_SUBCHAN_UL:
            case CLM_LIMITS_TYPE_SUBCHAN_UU:
            case CLM_LIMITS_TYPE_SUBCHAN_ULL:
            case CLM_LIMITS_TYPE_SUBCHAN_ULU:
            case CLM_LIMITS_TYPE_SUBCHAN_UUL:
            case CLM_LIMITS_TYPE_SUBCHAN_UUU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_U;
                break;
            default:
                break;
        }
    } else if (lvl == 2) {
        /* Get 40in160 given 20in160 */
        switch (ctl_subchan) {
            case CLM_LIMITS_TYPE_SUBCHAN_LLL:
            case CLM_LIMITS_TYPE_SUBCHAN_LLU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_LL;
                break;
            case CLM_LIMITS_TYPE_SUBCHAN_LUL:
            case CLM_LIMITS_TYPE_SUBCHAN_LUU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_LU;
                break;
            case CLM_LIMITS_TYPE_SUBCHAN_ULL:
            case CLM_LIMITS_TYPE_SUBCHAN_ULU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_UL;
                break;
            case CLM_LIMITS_TYPE_SUBCHAN_UUL:
            case CLM_LIMITS_TYPE_SUBCHAN_UUU:
                lt = CLM_LIMITS_TYPE_SUBCHAN_UU;
                break;
            default: break;
        }
    }
    return lt;
}
#endif /* WL11AC */

#ifdef WL_SARLIMIT
static void
wlc_channel_sarlimit_get_default(wlc_cm_info_t *wlc_cmi, sar_limit_t *sar)
{
/* XXX Make this an empty function if WL_SARLIMIT_DISABLED is defined.  This is an
 * unusual usage of the dongle SAR_ENAB() configuration define to remove the references
 * to wlc_sar_tbl[] and wlc_sar_tbl_len which are defined in the generated file
 * wlc_sar_tbl.c.  When SAR is disabled and thus not used, we don't want to ship the
 * SAR "compiler".  You can not undefine the primary feature flag, WL_SARLIMIT, because
 * it controls the code and data structure generation and must stay consistent with the
 * definition used to build a ROM, otherwise you would get many abandoned functions and
 * data structures resulting in a large increase in RAM usage.  To complete the removal
 * of the sar tool, WL_SARLIMIT_DISABLED is also used in a couple of makefiles to remove
 * the dependency and the pattern rule.
 */
#ifndef WL_SARLIMIT_DISABLED
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint idx;

    for (idx = 0; idx < wlc_sar_tbl_len; idx++) {
        if (wlc_sar_tbl[idx].boardtype == wlc->pub->sih->boardtype) {
            memcpy((uint8 *)sar, (uint8 *)&(wlc_sar_tbl[idx].sar), sizeof(sar_limit_t));
            break;
        }
    }
#endif /* WL_SARLIMIT_DISABLED */
}

void
wlc_channel_sar_init(wlc_cm_info_t *wlc_cmi)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    memset((uint8 *)wlc_cmi->sarlimit.band2g,
           wlc->bandstate[BAND_2G_INDEX]->sar,
           WLC_TXCORE_MAX);
    memset((uint8 *)wlc_cmi->sarlimit.band5g,
           wlc->bandstate[BAND_5G_INDEX]->sar,
           (WLC_TXCORE_MAX * WLC_SUBBAND_MAX));

    wlc_channel_sarlimit_get_default(wlc_cmi, &wlc_cmi->sarlimit);
#ifdef BCMDBG
    wlc_channel_sarlimit_dump(wlc_cmi, &wlc_cmi->sarlimit);
#endif /* BCMDBG */
}

#ifdef BCMDBG
void
wlc_channel_sarlimit_dump(wlc_cm_info_t *wlc_cmi, sar_limit_t *sar)
{
    int i;

    BCM_REFERENCE(wlc_cmi);

    WL_ERROR(("\t2G:    %2d%s %2d%s %2d%s %2d%s\n",
              QDB_FRAC(sar->band2g[0]), QDB_FRAC(sar->band2g[1]),
              QDB_FRAC(sar->band2g[2]), QDB_FRAC(sar->band2g[3])));
    for (i = 0; i < WLC_SUBBAND_MAX; i++) {
        WL_ERROR(("\t5G[%1d]  %2d%s %2d%s %2d%s %2d%s\n", i,
                  QDB_FRAC(sar->band5g[i][0]), QDB_FRAC(sar->band5g[i][1]),
                  QDB_FRAC(sar->band5g[i][2]), QDB_FRAC(sar->band5g[i][3])));
    }
}
#endif /* BCMDBG */
int
wlc_channel_sarlimit_get(wlc_cm_info_t *wlc_cmi, sar_limit_t *sar)
{
    memcpy((uint8 *)sar, (uint8 *)&wlc_cmi->sarlimit, sizeof(sar_limit_t));
    return 0;
}

int
wlc_channel_sarlimit_set(wlc_cm_info_t *wlc_cmi, sar_limit_t *sar)
{
    memcpy((uint8 *)&wlc_cmi->sarlimit, (uint8 *)sar, sizeof(sar_limit_t));
    return 0;
}

/* given chanspec and return the subband index */
static uint
wlc_channel_sarlimit_subband_idx(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec)
{
    uint8 chan = CHSPEC_CHANNEL(chanspec);

    BCM_REFERENCE(wlc_cmi);

    if (chan < CHANNEL_5G_MID_START)
        return 0;
    else if (chan >= CHANNEL_5G_MID_START && chan < CHANNEL_5G_HIGH_START)
        return 1;
    else if (chan >= CHANNEL_5G_HIGH_START && chan < CHANNEL_5G_UPPER_START)
        return 2;
    else
        return 3;
}

/* Get the sar limit for the subband containing this channel */
void
wlc_channel_sarlimit_subband(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec, uint8 *sar)
{
    int idx = 0;

    if (CHSPEC_IS5G(chanspec)) {
        idx = wlc_channel_sarlimit_subband_idx(wlc_cmi, chanspec);
        memcpy((uint8 *)sar, (uint8 *)wlc_cmi->sarlimit.band5g[idx], WLC_TXCORE_MAX);
    } else {
        memcpy((uint8 *)sar, (uint8 *)wlc_cmi->sarlimit.band2g, WLC_TXCORE_MAX);
    }
}
#endif /* WL_SARLIMIT */

bool
wlc_channel_sarenable_get(wlc_cm_info_t *wlc_cmi)
{
    return (wlc_cmi->sar_enable);
}

void
wlc_channel_sarenable_set(wlc_cm_info_t *wlc_cmi, bool state)
{
    wlc_cmi->sar_enable = state ? TRUE : FALSE;
}

/* Set Regulatory Limits of TXBF0 rates for WL_RATE_DISABLED.
 * TXBF0 means nss = ntx. TXBF0 rates are disabled in PHY because
 * there is no array gain in such cases.
 */
static void
wlc_channel_disable_clm_txbf0(clm_power_limits_t *limits)
{
    /* The first rate of TXBF0 rate groups */
    const uint16 txbf0[] = {
        /* 2 Streams */
        WL_RATE_2X2_TXBF_VHT0SS2,
        WL_RATE_2X2_TXBF_HE0SS2,

        /* 3 Streams */
        WL_RATE_3X3_TXBF_VHT0SS3,
        WL_RATE_3X3_TXBF_HE0SS3,

        /* 4 Streams */
        WL_RATE_4X4_TXBF_VHT0SS4,
        WL_RATE_4X4_TXBF_HE0SS4
    };
    int i;

    for (i = 0; i < ARRAYSIZE(txbf0); i++) {
        memset(&limits->limit[txbf0[i]], (unsigned char)WL_RATE_DISABLED,
            WL_RATESET_SZ_HE_MCS);
    }
}

void
wlc_channel_reg_limits(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec, ppr_t *txpwr,
    ppr_ru_t *ru_txpwr)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    unsigned int chan;
    clm_country_t country;
    clm_result_t result = CLM_RESULT_ERR;
    clm_country_locales_t locale;
    clm_power_limits_t limits;
    clm_ru_power_limits_t ru_limits;
    uint16 flags;
    clm_band_t bandtype;
    wlcband_t * band;
    int ant_gain;
    clm_limits_params_t lim_params;
#ifdef WL_SARLIMIT
    uint8 sarlimit[WLC_TXCORE_MAX];
#endif
    bool valid = TRUE;
    uint lim_count = 0;
    clm_limits_type_t lim_types[5];
    wl_tx_bw_t lim_ppr_bw[5];
    uint i;

    ppr_clear(txpwr);
    if (ru_txpwr) {
        ppr_ru_clear(ru_txpwr);
    }
    BCM_REFERENCE(result);

    if (clm_limits_params_init(&lim_params) != CLM_RESULT_OK) {
        ASSERT(0);
#ifdef DONGLEBUILD
        HND_DIE();
#endif /* DONGLEBUILD */
    }

    band = wlc->bandstate[CHSPEC_BANDUNIT(chanspec)];
    bandtype = BANDTYPE2CLMBAND(band->bandtype);

    /* XXX: JIRA:SWWLAN-36509: Locale prioritazation feature: On
    * 2.4 GHz ONLY, do not pick up the 11d region for scanning or
    * association. Use autocountry_default.
    */
    /* Lookup channel in autocountry_default if not in current country */
    valid = wlc_valid_chanspec_db(wlc_cmi, chanspec);
    if ((WLC_LOCALE_PRIORITIZATION_2G_ENABLED(wlc) &&
        bandtype == CLM_BAND_2G && !WLC_CNTRY_DEFAULT_ENAB(wlc)) ||
        !(valid)) {
        if (!valid || WLC_AUTOCOUNTRY_ENAB(wlc)) {
            const char *def = wlc_11d_get_autocountry_default(wlc->m11d);
            result = wlc_country_lookup(wlc, def, 0, &country);
            if (result != CLM_RESULT_OK) {
                WL_ERROR(("wl%d: %s: Country lookup failure."
                    "ccode %s chanspec 0x%X valid %d\n",
                    wlc->pub->unit, __FUNCTION__, def, chanspec, valid));
                ASSERT(0);
#ifdef DONGLEBUILD
                HND_DIE();
#endif /* DONGLEBUILD */
#ifdef CONFIG_TENDA_PRIVATE_WLAN
                return;
#endif
            }
        } else {
            country = wlc_cmi->country;
        }
    } else
        country = wlc_cmi->country;

    chan = CHSPEC_CHANNEL(chanspec);
    ant_gain = band->antgain;
    lim_params.sar = WLC_TXPWR_MAX;
    band->sar = band->sar_cached;
    if (wlc_cmi->sar_enable) {
#ifdef WL_SARLIMIT
        /* when WL_SARLIMIT is enabled, update band->sar = MAX(sarlimit[i]) */
        wlc_channel_sarlimit_subband(wlc_cmi, chanspec, sarlimit);
        if ((CHIPID(wlc->pub->sih->chip) != BCM4360_CHIP_ID) &&
            (CHIPID(wlc->pub->sih->chip) != BCM4350_CHIP_ID) &&
            !BCM43602_CHIP(wlc->pub->sih->chip)) {
            uint i;
            band->sar = 0;
            for (i = 0; i < WLC_BITSCNT(wlc->stf->hw_txchain); i++)
                band->sar = MAX(band->sar, sarlimit[i]);

            WL_NONE(("%s: in %s Band, SAR %d apply\n", __FUNCTION__,
                     wlc_bandunit_name(wlc->band->bandunit), band->sar));
        }
        /* Don't write sarlimit to registers when called for reporting purposes */
        if (chanspec == wlc->chanspec) {
            uint32 sar_lims = (uint32)(sarlimit[0] | sarlimit[1] << 8 |
                                       sarlimit[2] << 16 | sarlimit[3] << 24);
#ifdef WLTXPWR_CACHE
            tx_pwr_cache_entry_t* cacheptr = phy_tpc_get_txpwr_cache(WLC_PI(wlc));

            wlc_phy_set_cached_sar_lims(cacheptr, chanspec, sar_lims);
#endif    /* WLTXPWR_CACHE */
            if (wlc->pub->up && wlc->clk) {
                wlc_phy_sar_limit_set(WLC_PI_BANDUNIT(wlc,
                    band->bandunit), sar_lims);
            }
        }
#endif /* WL_SARLIMIT */
        lim_params.sar = band->sar;
    }

    if (strcmp(wlc_cmi->country_abbrev, "#a") == 0) {
        band->sar = WLC_TXPWR_MAX;
        lim_params.sar = WLC_TXPWR_MAX;
#if defined(BCMDBG) || defined(WLTEST)
#ifdef WL_SARLIMIT
        if (wlc->clk) {
            wlc_phy_sar_limit_set(WLC_PI_BANDUNIT(wlc, band->bandunit),
                ((WLC_TXPWR_MAX & 0xff) | (WLC_TXPWR_MAX & 0xff) << 8 |
                (WLC_TXPWR_MAX & 0xff) << 16 | (WLC_TXPWR_MAX & 0xff) << 24));
        }
#endif /* WL_SARLIMIT */
#endif /* BCMDBG || WLTEST */
    }
    result = wlc_get_locale(country, &locale);
    if (result != CLM_RESULT_OK) {
        ASSERT(0);
#ifdef DONGLEBUILD
        HND_DIE();
#endif /* DONGLEBUILD */
    }

    result = wlc_get_flags(&locale, bandtype, &flags);
    if (result != CLM_RESULT_OK) {
        ASSERT(0);
#ifdef DONGLEBUILD
        HND_DIE();
#endif /* DONGLEBUILD */
    }

    wlc_bmac_filter_war_upd(wlc->hw, FALSE);

    wlc_bmac_lo_gain_nbcal_upd(wlc->hw, (flags & WLC_LO_GAIN_NBCAL) != 0);

    /* Need to set the txpwr_local_max to external reg max for
     * this channel as per the locale selected for AP.
     */
#ifdef AP
#ifdef WLTPC
    if (AP_ONLY(wlc->pub)) {
        uint8 pwr = wlc_get_reg_max_power_for_channel(wlc->cmi, wlc->chanspec, TRUE);
        wlc_tpc_set_local_max(wlc->tpc, pwr);
    }
#endif /* WLTPC */
#endif /* AP */

    lim_types[0] = CLM_LIMITS_TYPE_CHANNEL;
    switch (CHSPEC_BW(chanspec)) {

    case WL_CHANSPEC_BW_20:
        lim_params.bw = CLM_BW_20;
        lim_count = 1;
        lim_ppr_bw[0] = WL_TX_BW_20;
        break;

    case WL_CHANSPEC_BW_40:
        lim_params.bw = CLM_BW_40;
        lim_count = 2;
        lim_types[1] = clm_chanspec_to_limits_type(chanspec);
        lim_ppr_bw[0] = WL_TX_BW_40;
        lim_ppr_bw[1] = WL_TX_BW_20IN40;
        break;

#ifdef WL11AC
    case WL_CHANSPEC_BW_80: {
        clm_limits_type_t ctl_limits_type =
            clm_chanspec_to_limits_type(chanspec);

        lim_params.bw = CLM_BW_80;
        lim_count = 3;
        lim_types[1] = clm_get_enclosing_subchan(ctl_limits_type, 1);
        lim_types[2] = ctl_limits_type;
        lim_ppr_bw[0] = WL_TX_BW_80;
        lim_ppr_bw[1] = WL_TX_BW_40IN80;
        lim_ppr_bw[2] = WL_TX_BW_20IN80;
        break;
    }

    case WL_CHANSPEC_BW_160: {
        clm_limits_type_t ctl_limits_type =
            clm_chanspec_to_limits_type(chanspec);

        lim_params.bw = CLM_BW_160;
        lim_count = 4;
        lim_types[1] = clm_get_enclosing_subchan(ctl_limits_type, 1);
        lim_types[2] = clm_get_enclosing_subchan(ctl_limits_type, 2);
        lim_types[3] = ctl_limits_type;
        lim_ppr_bw[0] = WL_TX_BW_160;
        lim_ppr_bw[1] = WL_TX_BW_80IN160;
        lim_ppr_bw[2] = WL_TX_BW_40IN160;
        lim_ppr_bw[3] = WL_TX_BW_20IN160;
        break;
    }

    case WL_CHANSPEC_BW_8080: {
        clm_limits_type_t ctl_limits_type =
            clm_chanspec_to_limits_type(chanspec);

        chan = wf_chspec_primary80_channel(chanspec);
        lim_params.other_80_80_chan = wf_chspec_secondary80_channel(chanspec);

        lim_params.bw = CLM_BW_80_80;
        lim_count = 5;
        lim_types[1] = clm_get_enclosing_subchan(ctl_limits_type, 1);
        lim_types[2] = clm_get_enclosing_subchan(ctl_limits_type, 2);
        lim_types[3] = ctl_limits_type;
        lim_types[4] = CLM_LIMITS_TYPE_CHANNEL;  /* special case: 8080chan2 */
        lim_ppr_bw[0] = WL_TX_BW_8080;
        lim_ppr_bw[1] = WL_TX_BW_80IN8080;
        lim_ppr_bw[2] = WL_TX_BW_40IN8080;
        lim_ppr_bw[3] = WL_TX_BW_20IN8080;
        lim_ppr_bw[4] = WL_TX_BW_8080CHAN2;
        break;
    }
#endif /* WL11AC */

    default:
        ASSERT(0);
        break;
    }

    if (!strcmp("Q1", wlc_channel_ccode(wlc->cmi)) && (wlc_channel_regrev(wlc->cmi) == 164)) {
        lim_params.device_category = CLM_DEVICE_CATEGORY_SP;
    }

    /* Calculate limits for each (sub)channel */
    for (i = 0; i < lim_count; i++) {
#ifdef WL11AC
        /* For 8080CHAN2, just swap primary and secondary channel
         * and calculate 80MHz limit
         */
        if (lim_ppr_bw[i] == WL_TX_BW_8080CHAN2) {
            lim_params.other_80_80_chan = chan;
            chan = wf_chspec_secondary80_channel(chanspec);
        }
#endif /* WL11AC */

        if (wlc_clm_power_limits(&locale, bandtype, chan, ant_gain, lim_types[i],
            &lim_params, (void *)&limits, FALSE) == CLM_RESULT_OK) {
            /* Disable CLM TXBF0 rates */
            wlc_channel_disable_clm_txbf0(&limits);
            wlc_channel_read_bw_limits_to_ppr(wlc_cmi, txpwr, &limits, lim_ppr_bw[i]);
        }
        if (wlc_clm_power_limits(&locale, bandtype, chan, ant_gain, lim_types[i],
            &lim_params, (void *)&ru_limits, TRUE) == CLM_RESULT_OK) {
            wlc_channel_read_ub_bw_limits_to_ppr(wlc_cmi, txpwr, &ru_limits,
                lim_ppr_bw[i]);
            if ((ru_txpwr != NULL) && (i == 0)) {
                wlc_channel_read_ru_bw_limits_to_ppr(wlc_cmi, ru_txpwr, &ru_limits);
            }

        }
    }
    WL_NONE(("Channel(chanspec) %d (0x%4.4x)\n", chan, chanspec));
    /* Convoluted WL debug conditional execution of function to avoid warnings. */
    WL_NONE(("%s",
        (phy_tpc_dump_txpower_limits(WLC_PI_BANDUNIT(wlc, band->bandunit),
        txpwr), "")));
}

#define CLM_HE_RATESET(src, index) ((const ppr_he_mcs_rateset_t *)&src->limit[index])

static void
wlc_channel_read_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr, clm_power_limits_t *limits,
    wl_tx_bw_t bw)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint max_chains = PHYCORENUM(WLC_BITSCNT(wlc->stf->hw_txchain));
    /* HE_ENAB ensure hardware HE capable */
    bool he_cap = HE_ENAB(wlc->pub) && (wlc->pub->he_features != 0);

    BCM_REFERENCE(wlc);        /* SISO chip has fixed vals for PHYCORENUM and TXBF_ENAB */

    /* Port the values for this bandwidth */
    ppr_set_dsss(txpwr, bw, WL_TX_CHAINS_1,    CLM_DSSS_RATESET(limits));

    ppr_set_ofdm(txpwr, bw, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
        CLM_OFDM_1X1_RATESET(limits));

    ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
        CLM_MCS_1X1_RATESET(limits));

    /* Set HE power limit if HE capability is TRUE */
    if (he_cap) {
        ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            CLM_HE_RATESET(limits, WL_RATE_1X1_HE0SS1), WL_HE_RT_SU);
    }

    if (max_chains > 1) {
        ppr_set_dsss(txpwr, bw, WL_TX_CHAINS_2, CLM_DSSS_1X2_MULTI_RATESET(limits));

        ppr_set_ofdm(txpwr, bw, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
            CLM_OFDM_1X2_CDD_RATESET(limits));

        ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
            CLM_MCS_1X2_CDD_RATESET(limits));

        ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_2,
            CLM_MCS_2X2_STBC_RATESET(limits));

        ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
            CLM_MCS_2X2_SDM_RATESET(limits));

        if (he_cap) {
            ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                CLM_HE_RATESET(limits, WL_RATE_1X2_HE0SS1), WL_HE_RT_SU);

            ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_2,
                CLM_HE_RATESET(limits, WL_RATE_2X2_HE0SS2), WL_HE_RT_SU);
        }

        if (max_chains > 2) {
            ppr_set_dsss(txpwr, bw, WL_TX_CHAINS_3, CLM_DSSS_1X3_MULTI_RATESET(limits));

            ppr_set_ofdm(txpwr, bw, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                CLM_OFDM_1X3_CDD_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                CLM_MCS_1X3_CDD_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_3,
                CLM_MCS_2X3_STBC_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
                CLM_MCS_2X3_SDM_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_3,
                CLM_MCS_3X3_SDM_RATESET(limits));

            if (he_cap) {
                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, CLM_HE_RATESET(limits, WL_RATE_1X3_HE0SS1),
                    WL_HE_RT_SU);

                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, CLM_HE_RATESET(limits, WL_RATE_2X3_HE0SS2),
                    WL_HE_RT_SU);

                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, CLM_HE_RATESET(limits, WL_RATE_3X3_HE0SS3),
                    WL_HE_RT_SU);
            }

            if (max_chains > 3) {
                ppr_set_dsss(txpwr, bw, WL_TX_CHAINS_4,
                    CLM_DSSS_1X4_MULTI_RATESET(limits));

                ppr_set_ofdm(txpwr, bw, WL_TX_MODE_CDD, WL_TX_CHAINS_4,
                    CLM_OFDM_1X4_CDD_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_4, CLM_MCS_1X4_CDD_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_4, CLM_MCS_2X4_STBC_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_4, CLM_MCS_2X4_SDM_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_4, CLM_MCS_3X4_SDM_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_4, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_4, CLM_MCS_4X4_SDM_RATESET(limits));

                if (he_cap) {
                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_1X4_HE0SS1),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_NONE,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_2X4_HE0SS2),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_NONE,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_3X4_HE0SS3),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_4, WL_TX_MODE_NONE,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_4X4_HE0SS4),
                        WL_HE_RT_SU);
                }
            }
        }
    }
#if defined(WL_BEAMFORMING)
    if (TXBF_ENAB(wlc->pub) && (max_chains > 1)) {
        ppr_set_ofdm(txpwr, bw, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
            CLM_OFDM_1X2_TXBF_RATESET(limits));

        ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
            CLM_MCS_1X2_TXBF_RATESET(limits));

        ppr_set_ht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
            CLM_MCS_2X2_TXBF_RATESET(limits));

        if (he_cap) {
            ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
                CLM_HE_RATESET(limits, WL_RATE_1X2_TXBF_HE0SS1), WL_HE_RT_SU);

            ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF, WL_TX_CHAINS_2,
                CLM_HE_RATESET(limits, WL_RATE_2X2_TXBF_HE0SS2), WL_HE_RT_SU);
        }

        if (max_chains > 2) {
            ppr_set_ofdm(txpwr, bw, WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
                CLM_OFDM_1X3_TXBF_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
                CLM_MCS_1X3_TXBF_RATESET(limits));

            ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
                CLM_MCS_2X3_TXBF_RATESET(limits));

            ppr_set_ht_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_TXBF, WL_TX_CHAINS_3,
                CLM_MCS_3X3_TXBF_RATESET(limits));

            if (he_cap) {
                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_3,
                    CLM_HE_RATESET(limits, WL_RATE_1X3_TXBF_HE0SS1),
                    WL_HE_RT_SU);

                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_3,
                    CLM_HE_RATESET(limits, WL_RATE_2X3_TXBF_HE0SS2),
                    WL_HE_RT_SU);

                ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_3,
                    CLM_HE_RATESET(limits, WL_RATE_3X3_TXBF_HE0SS3),
                    WL_HE_RT_SU);
            }

            if (max_chains > 3) {
                ppr_set_ofdm(txpwr, bw, WL_TX_MODE_TXBF, WL_TX_CHAINS_4,
                    CLM_OFDM_1X4_TXBF_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_4, CLM_MCS_1X4_TXBF_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_4, CLM_MCS_2X4_TXBF_RATESET(limits));

                ppr_set_vht_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_4, CLM_MCS_3X4_TXBF_RATESET(limits));

                ppr_set_ht_mcs(txpwr, bw, WL_TX_NSS_4, WL_TX_MODE_TXBF,
                    WL_TX_CHAINS_4, CLM_MCS_4X4_TXBF_RATESET(limits));
                if (he_cap) {
                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_1, WL_TX_MODE_TXBF,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_1X4_TXBF_HE0SS1),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_2, WL_TX_MODE_TXBF,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_2X4_TXBF_HE0SS2),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_3, WL_TX_MODE_TXBF,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_3X4_TXBF_HE0SS3),
                        WL_HE_RT_SU);

                    ppr_set_he_mcs(txpwr, bw, WL_TX_NSS_4, WL_TX_MODE_TXBF,
                        WL_TX_CHAINS_4,
                        CLM_HE_RATESET(limits, WL_RATE_4X4_TXBF_HE0SS4),
                        WL_HE_RT_SU);
                }
            }
        }
    }
#endif /* defined(WL_BEAMFORMING) */
}

#define CLM_HE_UB_RATESET(src, index) ((const int8)src->limit[index])

/* Read DL-OFDMA UB and LUB CLM power limits to pprpbw_t */
static void
wlc_channel_read_ub_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr,
    clm_ru_power_limits_t *ru_limits, wl_tx_bw_t bw)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint max_chains = PHYCORENUM(WLC_BITSCNT(wlc->stf->hw_txchain));
    wl_he_rate_type_t type;
    clm_ru_rates_t rate;
    uint offset, type_index = 0;
    int8 power;
    int tbl_index;
    /* HE_ENAB ensure hardware HE capable */
    bool he_cap = HE_ENAB(wlc->pub) && (wlc->pub->he_features != 0);

    BCM_REFERENCE(wlc);

    /* Set HE power limit if HE capability is TRUE */
    if (he_cap == FALSE) {
        return;
    }
    /* Port the values for this bandwidth */
    for (type = WL_HE_RT_UB; type <= WL_HE_RT_LUB; type++) {
        for (rate = WL_RU_RATE_1X1_UBSS1; rate <= WL_RU_RATE_4X4_TXBF_UBSS4; rate++) {
            tbl_index = rate - WL_RU_RATE_1X1_UBSS1;
            if (ru_rate_tbl[tbl_index].chain <= max_chains) {
                offset = type_index * WL_RU_NUM_MODE;
                power = CLM_HE_UB_RATESET(ru_limits, (rate + offset));
                ppr_set_same_he_mcs(txpwr, bw, ru_rate_tbl[tbl_index].nss,
                    ru_rate_tbl[tbl_index].mode, ru_rate_tbl[tbl_index].chain,
                    power, type);
            }
        }
        type_index++;
    }
}

/* Read UL-OFDMA CLM power limits to pprpbw_ru_t */
static void
wlc_channel_read_ru_bw_limits_to_ppr(wlc_cm_info_t *wlc_cmi, ppr_ru_t *ru_txpwr,
    clm_ru_power_limits_t *ru_limits)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint max_chains = PHYCORENUM(WLC_BITSCNT(wlc->stf->hw_txchain));
    wl_he_rate_type_t type;
    clm_ru_rates_t rate;
    uint offset, type_index = 0;
    int8 power;
    /* HE_ENAB ensure hardware HE capable */
    bool he_cap = HE_ENAB(wlc->pub) && (wlc->pub->he_features != 0);

    BCM_REFERENCE(wlc);

    /* Set HE power limit if HE capability is TRUE */
    if (he_cap == FALSE) {
        return;
    }
    /* Port the values for this bandwidth */
    for (type = WL_HE_RT_RU26; type <= WL_HE_RT_RU996; type++) {
        for (rate = WL_RU_RATE_1X1_26SS1; rate <= WL_RU_RATE_4X4_TXBF_26SS4; rate++) {
            if (ru_rate_tbl[rate].chain <= max_chains) {
                offset = type_index * WL_RU_NUM_MODE;
                power = CLM_HE_UB_RATESET(ru_limits, (rate + offset));
                ppr_set_same_he_ru_mcs(ru_txpwr, type, ru_rate_tbl[rate].nss,
                    ru_rate_tbl[rate].mode, ru_rate_tbl[rate].chain, power);
            }
        }
        type_index++;
        if (type == WL_HE_RT_RU106) {
            /* Type UB and LUB are located between RU106 and RU242,
             * refer to clm_ru_rates_t
             */
            type_index += 2;
        }
    }
}

static clm_result_t
wlc_clm_power_limits(
    const clm_country_locales_t *locales, clm_band_t band,
    unsigned int chan, int ant_gain, clm_limits_type_t limits_type,
    const clm_limits_params_t *params, void *limits, bool ru)
{
    if (ru) {
        return clm_ru_limits(locales, band, chan, ant_gain, limits_type, params,
            (clm_ru_power_limits_t *)limits);
    } else {
        return clm_limits(locales, band, chan, ant_gain, limits_type, params,
            (clm_power_limits_t *)limits);
    }
}

#if defined(TD_EASYMESH_SUPPORT) && defined(CONFIG_TENDA_PRIVATE_WLAN)
static bool
wlc_us_ccode(const char *ccode)
{
    return (!strncmp("US", ccode, WLC_CNTRY_BUF_SZ - 1) ||
        !strncmp("Q1", ccode, WLC_CNTRY_BUF_SZ - 1) ||
        !strncmp("Q2", ccode, WLC_CNTRY_BUF_SZ - 1) ||
        !strncmp("ALL", ccode, WLC_CNTRY_BUF_SZ - 1));
}

bool
wlc_eu_code(struct wlc_info *wlc)
{
    return wlc ? (!strncmp("EU", wlc->cmi->country_abbrev, WLC_CNTRY_BUF_SZ - 1) || 
                  !strncmp("DE", wlc->cmi->country_abbrev, WLC_CNTRY_BUF_SZ - 1)) : FALSE;
}

bool
wlc_us_code(struct wlc_info *wlc)
{
    return wlc_us_ccode(wlc->cmi->country_abbrev);
}
#endif

/* Returns TRUE if currently set country is Japan or variant */
bool
wlc_japan(struct wlc_info *wlc)
{
    return wlc_japan_ccode(wlc->cmi->country_abbrev);
}

/* JP, J1 - J10 are Japan ccodes */
static bool
wlc_japan_ccode(const char *ccode)
{
    return (ccode[0] == 'J' &&
        (ccode[1] == 'P' || (ccode[1] >= '1' && ccode[1] <= '9')));
}

bool
BCMATTACHFN(wlc_is_ccode_lockdown)(wlc_info_t *wlc)
{
    char *vars = wlc->pub->vars;
    char *s, *ccodelock;
    int len;
    char name[] = "ccodelock";

    len = strlen(name);

    /* ccode lockdown is only supported on OTP with CIS format */
    if (si_is_sprom_available(wlc->pub->sih))
        return FALSE;

    if (!vars)
        return FALSE;

    /* look in vars[] */
    for (s = vars; s && *s;) {
        if ((bcmp(s, name, len) == 0) && (s[len] == '=')) {
            ccodelock = (&s[len+1]);
            if ((int8)bcm_atoi(ccodelock) == 1)
                return TRUE;
        }
        while (*s++)
            ;
    }
    return FALSE;
}

static const bcmwifi_rclass_info_t *
wlc_channel_get_rclass_info(wlc_cm_info_t *cmi, uint8 rclass)
{
    wlc_pub_t *pub = cmi->wlc->pub;
    const bcmwifi_rclass_info_t *rcinfo = NULL;
    bcmwifi_rclass_type_t rctype;

    BCM_REFERENCE(pub);

    if (!isset(cmi->valid_rcvec.vec, rclass)) {
        WL_ERROR(("wl%d: %s: regulatory class %d not supported\n", pub->unit,
            __FUNCTION__, rclass));
        return NULL;
    }

    if (cmi->use_global)
        rctype = BCMWIFI_RCLASS_TYPE_GBL;
    else
        rctype = cmi->cur_rclass_type;

    if (bcmwifi_rclass_get_rclass_info(rctype, rclass, &rcinfo) < 0) {
        WL_ERROR(("wl%d: %s: regulatory class %d not supported by table %d\n",
            pub->unit, __FUNCTION__, rclass, rctype));
        return NULL;
    }
    return rcinfo;
}

uint8
wlc_rclass_extch_get(wlc_cm_info_t *wlc_cmi, uint8 rclass)
{
    const bcmwifi_rclass_info_t *rcinfo;
    uint8 extch = DOT11_EXT_CH_NONE;

    rcinfo = wlc_channel_get_rclass_info(wlc_cmi, rclass);
    if (!rcinfo) {
        goto exit;
    }

    if (rcinfo->bw != BCMWIFI_BW_40) {
        WL_ERROR(("wl%d: %s %d not a 40MHz regulatory class\n",
            wlc_cmi->wlc->pub->unit, wlc_cmi->country_abbrev, rclass));
        goto exit;
    }

    /* extch is a secondary channel offset */
    if (rcinfo->flags & BCMWIFI_RCLASS_FLAGS_PRIMARY_UPPER) {
        extch = DOT11_EXT_CH_LOWER;
    } else if (rcinfo->flags & BCMWIFI_RCLASS_FLAGS_PRIMARY_LOWER) {
        extch = DOT11_EXT_CH_UPPER;
    }

exit:
    WL_REGULATORY(("wl%d: %s regulatory class %d has ctl chan %s\n",
        wlc_cmi->wlc->pub->unit, wlc_cmi->country_abbrev, rclass,
        ((!extch) ? "NONE" : (((extch == DOT11_EXT_CH_LOWER) ? "LOWER" : "UPPER")))));

    return extch;
}

static uint8
wlc_get_rclass(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec)
{
    bcmwifi_rclass_type_t rctype;
    bcmwifi_rclass_opclass_t rclass;
    int err;

    if (wlc_cmi->use_global)
        rctype = BCMWIFI_RCLASS_TYPE_GBL;
    else
        rctype = wlc_cmi->cur_rclass_type;

    err = bcmwifi_rclass_get_opclass(rctype, chanspec, &rclass);
    if (err < 0) {
        WL_REGULATORY(("wl%d: No regulatory class found for %u chanspec 0x%x: err=%d\n",
            WLCWLUNIT(wlc_cmi->wlc), rctype, chanspec, err));
        rclass = 0;
    }
    return rclass;
}

uint8
wlc_get_regclass(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec)
{
    uint8 rclass = wlc_get_rclass(wlc_cmi, chanspec);

    if (rclass && !isset(wlc_cmi->valid_rcvec.vec, rclass)) {
        WL_ERROR(("wl%d: given regulatory class %d unsupported by %s\n",
            WLCWLUNIT(wlc_cmi->wlc), rclass, wlc_cmi->country_abbrev));
        rclass = 0;
    }
    return rclass;
}

static uint8 wlc_regclass_type_select(wlc_cm_info_t *wlc_cmi)
{
    uint8 regtype_sel = 0;

    if (!wlc_cmi->use_global)
        regtype_sel = NON_GLOBAL_OP_CLASS_SEL;
    else {
    #if !defined(MBO_AP) && defined(MBO_STA)
        regtype_sel = GLOBAL_OP_CLASS_SEL;
    #else
        regtype_sel = NON_GLOBAL_OP_CLASS_SEL | GLOBAL_OP_CLASS_SEL;
    #endif /* !MBO_AP && MBO_STA */
    }
    return regtype_sel;
}

static void
wlc_regclass_chanspec_set_vec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    bcmwifi_rclass_opclass_t opclass;
    rcvec_t *rcvec = &wlc_cmi->valid_rcvec;
    int err;
    uint8 regtype_sel = 0;

    if (!wlc_valid_chanspec_db(wlc_cmi, chspec))
        return;

    regtype_sel = wlc_regclass_type_select(wlc_cmi);

    if (regtype_sel & NON_GLOBAL_OP_CLASS_SEL) {
        err = bcmwifi_rclass_get_opclass(wlc_cmi->cur_rclass_type, chspec, &opclass);
        if (err == BCME_OK && !isset(rcvec->vec, opclass)) {
            setbit(rcvec->vec, opclass);
            rcvec->count++;
        }
    }

    if (regtype_sel & GLOBAL_OP_CLASS_SEL) {
        err = bcmwifi_rclass_get_opclass(BCMWIFI_RCLASS_TYPE_GBL, chspec, &opclass);
        if (err == BCME_OK && !isset(rcvec->vec, opclass)) {
            setbit(rcvec->vec, opclass);
            rcvec->count++;
        }
    }
}

static void
wlc_regclass_vec_init(wlc_cm_info_t *wlc_cmi)
{
    uint8 ch;
    chanspec_t chanspec;
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint8 saved_bw_cap[MAXBANDS];
    rcvec_t *rcvec = &wlc_cmi->valid_rcvec;
    enum wlc_bandunit bandunit;
    wlcband_t *band;

    FOREACH_WLC_BAND(wlc, bandunit) {
        /* save bw cap */
        band = wlc->bandstate[bandunit];
        saved_bw_cap[bandunit] = band->bw_cap;

        /* temporarily enable all bw caps */
        band->bw_cap |= WLC_BW_40MHZ_BIT;

        /* 2G does not have higher bandwidth support */
        if (bandunit == BAND_2G_INDEX)
            continue;

#ifdef WL11AC
        band->bw_cap |= WLC_BW_80MHZ_BIT;
#endif /* WL11AC */
#ifdef WL11AC_160
        band->bw_cap |= WLC_BW_160MHZ_BIT;
#endif /* WL11AC_160 */
    }

    bzero(rcvec, sizeof(*rcvec));
    FOREACH_WLC_BAND(wlc, bandunit) {
        band = wlc->bandstate[bandunit];
        FOREACH_WLC_BAND_CHANNEL20(band, ch) {
            chanspec_band_t chspec_band = BANDTYPE_CHSPEC(band->bandtype);
            chanspec = CH20MHZ_CHSPEC(ch, chspec_band);
            wlc_regclass_chanspec_set_vec(wlc_cmi, chanspec);

            if (N_ENAB(wlc->pub)) {
                chanspec = wf_create_40MHz_chspec_primary_sb(ch,
                    WL_CHANSPEC_CTL_SB_LOWER, chspec_band);
                wlc_regclass_chanspec_set_vec(wlc_cmi, chanspec);

                chanspec = wf_create_40MHz_chspec_primary_sb(ch,
                    WL_CHANSPEC_CTL_SB_UPPER, chspec_band);
                wlc_regclass_chanspec_set_vec(wlc_cmi, chanspec);
            }
#ifdef WL11AC
            if (VHT_ENAB(wlc->pub) || HE_ENAB(wlc->pub)) {
                chanspec = CH80MHZ_CHSPEC(chspec_band, ch + 3 * CH_10MHZ_APART,
                        WL_CHANSPEC_CTL_SB_LL);
                wlc_regclass_chanspec_set_vec(wlc_cmi, chanspec);
#ifdef WL11AC_160
                chanspec = CH160MHZ_CHSPEC(chspec_band,
                        ch + 7 * CH_10MHZ_APART, WL_CHANSPEC_CTL_SB_LLL);
                wlc_regclass_chanspec_set_vec(wlc_cmi, chanspec);
#endif /* WL11AC_160 */
            }
#endif /* WL11AC */
        }
    }

    /* restore the saved bw caps */
    FOREACH_WLC_BAND(wlc, bandunit) {
        wlc->bandstate[bandunit]->bw_cap = saved_bw_cap[bandunit];
    }
} /* wlc_regclass_vec_init */

int
wlc_regclass_get_band(wlc_cm_info_t *wlc_cmi, uint8 rclass, chanspec_band_t *band)
{
    const bcmwifi_rclass_info_t *rcinfo;
    int ret = BCME_UNSUPPORTED;

    rcinfo = wlc_channel_get_rclass_info(wlc_cmi, rclass);
    if (rcinfo) {
        *band = bcmwifi_rclass_band_rc2chspec(rcinfo->band);
        ret = BCME_OK;
    }
    return ret;
}

#if defined(BCMDBG)
void
wlc_dump_rclist(const char *name, uint8 *rclist, uint8 rclen, struct bcmstrbuf *b)
{
    uint i;

    if (!rclen)
        return;

    bcm_bprintf(b, "%s [ ", name ? name : "");
    for (i = 0; i < rclen; i++) {
        bcm_bprintf(b, "%d ", rclist[i]);
    }
    bcm_bprintf(b, "]");
    bcm_bprintf(b, "\n");

    return;
}

/* format a qdB value as integer and decimal fraction in a bcmstrbuf */
static void
wlc_channel_dump_qdb(struct bcmstrbuf *b, int qdb)
{
    if ((qdb >= 0) || (qdb % WLC_TXPWR_DB_FACTOR == 0))
        bcm_bprintf(b, "%2d%s", QDB_FRAC(qdb));
    else
        bcm_bprintf(b, "%2d%s",
            qdb / WLC_TXPWR_DB_FACTOR + 1,
            fraction[WLC_TXPWR_DB_FACTOR - (qdb % WLC_TXPWR_DB_FACTOR)]);
}

/* helper function for wlc_channel_dump_txppr() to print one set of power targets with label */
static void
wlc_channel_dump_pwr_range(struct bcmstrbuf *b, const char *label, int8 *ptr, uint count)
{
    uint i;

    bcm_bprintf(b, "%s ", label);
    for (i = 0; i < count; i++) {
        if (ptr[i] != WL_RATE_DISABLED) {
            wlc_channel_dump_qdb(b, ptr[i]);
            bcm_bprintf(b, " ");
        } else
            bcm_bprintf(b, "-     ");
    }
    bcm_bprintf(b, "\n");
}

/* helper function to print a target range line with the typical 8 targets */
static void
wlc_channel_dump_pwr_range8(struct bcmstrbuf *b, const char *label, int8* ptr)
{
    wlc_channel_dump_pwr_range(b, label, (int8*)ptr, 8);
}

#ifdef WL11AC

#define NUM_MCS_RATES WL_NUM_RATES_VHT
#define CHSPEC_TO_TX_BW(c)    (\
    CHSPEC_IS8080(c) ? WL_TX_BW_8080 : \
    (CHSPEC_IS160(c) ? WL_TX_BW_160 : \
    (CHSPEC_IS80(c) ? WL_TX_BW_80 : \
    (CHSPEC_IS40(c) ? WL_TX_BW_40 : WL_TX_BW_20))))

#else

#define NUM_MCS_RATES WL_NUM_RATES_MCS_1STREAM
#define CHSPEC_TO_TX_BW(c)    (CHSPEC_IS40(c) ? WL_TX_BW_40 : WL_TX_BW_20)

#endif

/* helper function to print a target range line with the typical 8 targets */
static void
wlc_channel_dump_pwr_range_mcs(struct bcmstrbuf *b, const char *label, int8 *ptr)
{
    wlc_channel_dump_pwr_range(b, label, (int8*)ptr, NUM_MCS_RATES);
}

/* format the contents of a ppr_t structure for a bcmstrbuf */
static void
wlc_channel_dump_txppr(struct bcmstrbuf *b, ppr_t *txpwr, wl_tx_bw_t bw, wlc_info_t *wlc)
{
    ppr_dsss_rateset_t dsss_limits;
    ppr_ofdm_rateset_t ofdm_limits;
    ppr_vht_mcs_rateset_t mcs_limits;

    if (bw == WL_TX_BW_20) {
        bcm_bprintf(b, "\n20MHz:\n");
        ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss_limits);
        wlc_channel_dump_pwr_range(b,  "DSSS              ", dsss_limits.pwr,
            WL_RATESET_SZ_DSSS);
        ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_2, &dsss_limits);
            wlc_channel_dump_pwr_range(b, "DSSS_MULTI1       ", dsss_limits.pwr,
                WL_RATESET_SZ_DSSS);
            ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_3, &dsss_limits);
                wlc_channel_dump_pwr_range(b,  "DSSS_MULTI2       ",
                    dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_4,
                        &dsss_limits);
                    wlc_channel_dump_pwr_range(b,  "DSSS_MULTI3       ",
                        dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                    ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
    } else if (bw == WL_TX_BW_40) {

        bcm_bprintf(b, "\n40MHz:\n");
        ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_40, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n20in40MHz:\n");
        ppr_get_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_1, &dsss_limits);
        wlc_channel_dump_pwr_range(b,  "DSSS              ", dsss_limits.pwr,
            WL_RATESET_SZ_DSSS);
        ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_2, &dsss_limits);
            wlc_channel_dump_pwr_range(b, "DSSS_MULTI1       ", dsss_limits.pwr,
                WL_RATESET_SZ_DSSS);
            ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_3, &dsss_limits);
                wlc_channel_dump_pwr_range(b,  "DSSS_MULTI2       ",
                    dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_dsss(txpwr, WL_TX_BW_20IN40, WL_TX_CHAINS_4,
                        &dsss_limits);
                    wlc_channel_dump_pwr_range(b,  "DSSS_MULTI3       ",
                        dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                    ppr_get_ofdm(txpwr, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN40, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

#ifdef WL11AC
    } else if (bw == WL_TX_BW_80) {
        bcm_bprintf(b, "\n80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_80, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
        bcm_bprintf(b, "\n20in80MHz:\n");
        ppr_get_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_1, &dsss_limits);
        wlc_channel_dump_pwr_range(b,  "DSSS              ", dsss_limits.pwr,
            WL_RATESET_SZ_DSSS);
        ppr_get_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);
        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_2, &dsss_limits);
            wlc_channel_dump_pwr_range(b, "DSSS_MULTI1       ", dsss_limits.pwr,
                WL_RATESET_SZ_DSSS);
            ppr_get_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_3, &dsss_limits);
                wlc_channel_dump_pwr_range(b,  "DSSS_MULTI2       ",
                    dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                ppr_get_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_dsss(txpwr, WL_TX_BW_20IN80, WL_TX_CHAINS_4,
                        &dsss_limits);
                    wlc_channel_dump_pwr_range(b,  "DSSS_MULTI3       ",
                        dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                    ppr_get_ofdm(txpwr, WL_TX_BW_20IN80, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN80, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n40in80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_40IN80, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN80, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
#endif /* WL11AC */

#ifdef WL11AC_160
    } else if (bw == WL_TX_BW_160) {
        bcm_bprintf(b, "\n160MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_160, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_160, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_160, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_160, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_160, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
        bcm_bprintf(b, "\n20in160MHz:\n");
        ppr_get_dsss(txpwr, WL_TX_BW_20IN160, WL_TX_CHAINS_1, &dsss_limits);
        wlc_channel_dump_pwr_range(b,  "DSSS              ", dsss_limits.pwr,
            WL_RATESET_SZ_DSSS);
        ppr_get_ofdm(txpwr, WL_TX_BW_20IN160, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);
        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_dsss(txpwr, WL_TX_BW_20IN160, WL_TX_CHAINS_2, &dsss_limits);
            wlc_channel_dump_pwr_range(b, "DSSS_MULTI1       ", dsss_limits.pwr,
                WL_RATESET_SZ_DSSS);
            ppr_get_ofdm(txpwr, WL_TX_BW_20IN160, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_dsss(txpwr, WL_TX_BW_20IN160, WL_TX_CHAINS_3,
                    &dsss_limits);
                wlc_channel_dump_pwr_range(b,  "DSSS_MULTI2       ",
                    dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                ppr_get_ofdm(txpwr, WL_TX_BW_20IN160, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_dsss(txpwr, WL_TX_BW_20IN160, WL_TX_CHAINS_4,
                        &dsss_limits);
                    wlc_channel_dump_pwr_range(b,  "DSSS_MULTI3       ",
                        dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                    ppr_get_ofdm(txpwr, WL_TX_BW_20IN160, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN160, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n40in160MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_40IN160, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_40IN160, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_40IN160, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_40IN160, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN160, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n80in160MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_80IN160, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_80IN160, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_80IN160, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_80IN160, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN160, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
#endif /* WL11AC_160 */

#ifdef WL11AC_80P80
    } else if (bw == WL_TX_BW_8080) {
        bcm_bprintf(b, "\n80+80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_8080, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_8080, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_8080, WL_TX_MODE_CDD, WL_TX_CHAINS_3,
                    &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_1, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2, WL_TX_MODE_STBC,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_3, WL_TX_MODE_NONE,
                    WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_8080, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
        bcm_bprintf(b, "\n80+80MHz chan2:\n");
        ppr_get_dsss(txpwr, WL_TX_BW_8080CHAN2, WL_TX_CHAINS_1, &dsss_limits);
        wlc_channel_dump_pwr_range(b,  "DSSS              ", dsss_limits.pwr,
            WL_RATESET_SZ_DSSS);
        ppr_get_ofdm(txpwr, WL_TX_BW_8080CHAN2, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);
        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_dsss(txpwr, WL_TX_BW_8080CHAN2, WL_TX_CHAINS_2, &dsss_limits);
            wlc_channel_dump_pwr_range(b, "DSSS_MULTI1       ", dsss_limits.pwr,
                WL_RATESET_SZ_DSSS);
            ppr_get_ofdm(txpwr, WL_TX_BW_8080CHAN2, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_dsss(txpwr, WL_TX_BW_8080CHAN2, WL_TX_CHAINS_3,
                    &dsss_limits);
                wlc_channel_dump_pwr_range(b,  "DSSS_MULTI2       ",
                    dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                ppr_get_ofdm(txpwr, WL_TX_BW_8080CHAN2, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_dsss(txpwr, WL_TX_BW_8080CHAN2, WL_TX_CHAINS_4,
                        &dsss_limits);
                    wlc_channel_dump_pwr_range(b,  "DSSS_MULTI3       ",
                        dsss_limits.pwr, WL_RATESET_SZ_DSSS);
                    ppr_get_ofdm(txpwr, WL_TX_BW_8080CHAN2, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_8080CHAN2, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n20in80+80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_20IN8080, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_20IN8080, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_20IN8080, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_20IN8080, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_20IN8080, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n40in80+80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_40IN8080, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_40IN8080, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_40IN8080, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_40IN8080, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_40IN8080, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }

        bcm_bprintf(b, "\n80in80+80MHz:\n");

        ppr_get_ofdm(txpwr, WL_TX_BW_80IN8080, WL_TX_MODE_NONE, WL_TX_CHAINS_1,
            &ofdm_limits);
        wlc_channel_dump_pwr_range8(b, "OFDM              ", ofdm_limits.pwr);
        ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1, &mcs_limits);
        wlc_channel_dump_pwr_range_mcs(b, "MCS0_7            ", mcs_limits.pwr);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_get_ofdm(txpwr, WL_TX_BW_80IN8080, WL_TX_MODE_CDD, WL_TX_CHAINS_2,
                &ofdm_limits);
            wlc_channel_dump_pwr_range8(b, "OFDM_CDD1         ", ofdm_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD1       ", mcs_limits.pwr);

            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2, WL_TX_MODE_STBC,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC       ", mcs_limits.pwr);
            ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2, WL_TX_MODE_NONE,
                WL_TX_CHAINS_2, &mcs_limits);
            wlc_channel_dump_pwr_range_mcs(b, "MCS8_15           ", mcs_limits.pwr);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_get_ofdm(txpwr, WL_TX_BW_80IN8080, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3, &ofdm_limits);
                wlc_channel_dump_pwr_range8(b, "OFDM_CDD2         ",
                    ofdm_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD2       ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP1",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP1    ",
                    mcs_limits.pwr);
                ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3, &mcs_limits);
                wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                    mcs_limits.pwr);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_get_ofdm(txpwr, WL_TX_BW_80IN8080, WL_TX_MODE_CDD,
                        WL_TX_CHAINS_4, &ofdm_limits);
                    wlc_channel_dump_pwr_range8(b, "OFDM_CDD3         ",
                        ofdm_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_1,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_CDD3       ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_STBC, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS0_7_STBC_SPEXP2",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_2,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS8_15_SPEXP2    ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_3,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS16_23          ",
                        mcs_limits.pwr);
                    ppr_get_vht_mcs(txpwr, WL_TX_BW_80IN8080, WL_TX_NSS_4,
                        WL_TX_MODE_NONE, WL_TX_CHAINS_4, &mcs_limits);
                    wlc_channel_dump_pwr_range_mcs(b, "MCS24_31          ",
                        mcs_limits.pwr);
                }
            }
        }
#endif /* WL11AC_80P80 */
    }

    bcm_bprintf(b, "\n");
}
#endif

/*
 *     if (wlc->country_list_extended) all country listable.
 *    else J1 - J10 is excluded.
 */
static bool
wlc_country_listable(struct wlc_info *wlc, const char *countrystr)
{
    bool listable = TRUE;

    if (wlc->country_list_extended == FALSE) {
        if (countrystr[0] == 'J' &&
            (countrystr[1] >= '1' && countrystr[1] <= '9')) {
            listable = FALSE;
        }
    }

    return listable;
}

clm_country_t
wlc_get_country(struct wlc_info *wlc)
{
    return wlc->cmi->country;
}

int
wlc_get_channels_in_country(struct wlc_info *wlc, void *arg)
{
    chanvec_t channels;
    wl_channels_in_country_t *cic = (wl_channels_in_country_t *)arg;
    chanvec_t sup_chan;
    uint count, need, i;

    if (cic->band != WLC_BAND_5G && cic->band != WLC_BAND_2G && cic->band != WLC_BAND_6G) {
        WL_ERROR(("Invalid band %d\n", cic->band));
        return BCME_BADBAND;
    }

    if (IS_SINGLEBAND(wlc) && (cic->band != (uint)wlc->band->bandtype)) {
        WL_ERROR(("Invalid band %d for card\n", cic->band));
        return BCME_BADBAND;
    }

    if (wlc_channel_get_chanvec(wlc, cic->country_abbrev, cic->band, &channels) == FALSE) {
        WL_ERROR(("Invalid country %s\n", cic->country_abbrev));
        return BCME_NOTFOUND;
    }

    phy_radio_get_valid_chanvec(WLC_PI(wlc), cic->band, &sup_chan);
    for (i = 0; i < sizeof(chanvec_t); i++)
        sup_chan.vec[i] &= channels.vec[i];

    /* find all valid channels */
    for (count = 0, i = 0; i < sizeof(sup_chan.vec)*NBBY; i++) {
        if (isset(sup_chan.vec, i))
            count++;
    }

    need = sizeof(wl_channels_in_country_t) + count*sizeof(cic->channel[0]);

    if (need > cic->buflen) {
        /* too short, need this much */
        WL_ERROR(("WLC_GET_COUNTRY_LIST: Buffer size: Need %d Received %d\n",
            need, cic->buflen));
        cic->buflen = need;
        return BCME_BUFTOOSHORT;
    }

    for (count = 0, i = 0; i < sizeof(sup_chan.vec)*NBBY; i++) {
        if (isset(sup_chan.vec, i))
            cic->channel[count++] = i;
    }

    cic->count = count;
    return 0;
}

int
wlc_get_country_list(struct wlc_info *wlc, void *arg)
{
    chanvec_t channels;
    chanvec_t unused;
    wl_country_list_t *cl = (wl_country_list_t *)arg;
    clm_country_locales_t locale;
    chanvec_t sup_chan;
    uint need, chan_mask_idx, cc_idx;
    clm_country_t country_iter;
    char countrystr[sizeof(ccode_t) + 1] = {0};
    ccode_t cc;
    unsigned int regrev;

    if (cl->band_set == FALSE) {
        /* get for current band */
        cl->band = wlc->band->bandtype;
    }

    if (cl->band != WLC_BAND_5G && cl->band != WLC_BAND_2G && cl->band != WLC_BAND_6G) {
        WL_ERROR(("Invalid band %d\n", cl->band));
        return BCME_BADBAND;
    }

    if (!BAND_ENABLED(wlc, wlc_bandtype2bandunit(cl->band))) {
        WL_INFORM(("Invalid band %d for card\n", cl->band));
        cl->count = 0;
        return 0;
    }

    phy_radio_get_valid_chanvec(WLC_PI(wlc), cl->band, &sup_chan);

    need = sizeof(wl_country_list_t);
    cl->count = 0;
    (void)clm_iter_init(&country_iter);
    while (clm_country_iter(&country_iter, cc, &regrev) == CLM_RESULT_OK) {
        memcpy(countrystr, cc, sizeof(ccode_t));
        if (!wlc_country_listable(wlc, countrystr)) {
            continue;
        }
        /* Checking if current CC already in list. Note that those CC that are not in list
         * (because not eligible or because buffer too short) may be checked more than once
         * and counted in 'need' more than once
         */
        for (cc_idx = 0; cc_idx < cl->count; ++cc_idx) {
            if (!memcmp(&cl->country_abbrev[cc_idx*WLC_CNTRY_BUF_SZ], cc,
                sizeof(ccode_t)))
            {
                break;
            }
        }
        if (cc_idx < cl->count) {
            continue;
        }
        /* Checking if region, corresponding to current CC, supports required band
         */
        if ((wlc_get_locale(country_iter, &locale) != CLM_RESULT_OK) ||
            (wlc_locale_get_channels(&locale,
            BANDTYPE2CLMBAND(cl->band), &channels, &unused)
            != CLM_RESULT_OK))
        {
            continue;
        }
        for (chan_mask_idx = 0; chan_mask_idx < sizeof(sup_chan.vec); chan_mask_idx++) {
            if (sup_chan.vec[chan_mask_idx] & channels.vec[chan_mask_idx]) {
                break;
            }
        }
        if (chan_mask_idx == sizeof(sup_chan.vec)) {
            continue;
        }
        /* CC Passed all checks. If there is space in buffer - put it to buffer */
        need += WLC_CNTRY_BUF_SZ;
        if (need <= cl->buflen) {
            memcpy(&cl->country_abbrev[cl->count*WLC_CNTRY_BUF_SZ], cc,
                sizeof(ccode_t));
            cl->country_abbrev[cl->count*WLC_CNTRY_BUF_SZ + sizeof(ccode_t)] = 0;
            cl->count++;
        }
    }
    if (need > cl->buflen) {
        WL_ERROR(("WLC_GET_COUNTRY_LIST: Buffer size %d is too short, need %d\n",
            cl->buflen, need));
        cl->buflen = need;
        return BCME_BUFTOOSHORT;
    }
    return 0;
}

static clm_band_t
wlc_channel_chanspec_to_clm_band(chanspec_t chspec)
{
    clm_band_t band;

    if (CHSPEC_IS2G(chspec)) {
        band = CLM_BAND_2G;
    } else if (CHSPEC_IS5G(chspec)) {
        band = CLM_BAND_5G;
    }
    else if (CHSPEC_IS6G(chspec)) {
        band = CLM_BAND_6G;
    } else {
        WL_ERROR(("wlc_channel_chanspec_to_clm_band: unknown CLM band for chspec 0x%04X\n",
                  chspec));
        band = CLM_BAND_2G;
#ifdef DONGLEBUILD
        HND_DIE();
#endif /* DONGLEBUILD */
        ASSERT(0);
    }

    return band;
}

/* Get regulatory max power for a given channel in a given locale.
 * for external FALSE, it returns limit for brcm hw
 * ---- for 2.4GHz channel, it returns cck limit, not ofdm limit.
 * for external TRUE, it returns 802.11d Country Information Element -
 *    Maximum Transmit Power Level.
 */
int8
wlc_get_reg_max_power_for_channel_ex(wlc_cm_info_t *wlc_cmi, clm_country_locales_t *locales,
    chanspec_t chspec, bool external)
{
    int8 maxpwr = WL_RATE_DISABLED;
    clm_band_t band;
    uint chan;

    band = wlc_channel_chanspec_to_clm_band(chspec);
    chan = wf_chspec_primary20_chan(chspec);

    if (external) {
        int int_limit;

        if (clm_regulatory_limit(locales, band, chan, &int_limit) == CLM_RESULT_OK) {
            maxpwr = (uint8)int_limit;
        }
    } else {
        clm_power_limits_t limits;
        clm_limits_params_t lim_params;

        if ((clm_limits_params_init(&lim_params) == CLM_RESULT_OK) &&
            (clm_limits(locales, band, chan, 0, CLM_LIMITS_TYPE_CHANNEL,
                        &lim_params, &limits) == CLM_RESULT_OK)) {
            int i;

            for (i = 0; i < WL_NUMRATES; i++) {
                if (maxpwr < limits.limit[i])
                    maxpwr = limits.limit[i];
            }
        }
    }

    return (maxpwr);
}

/* Get regulatory max power for a given channel.
 * Current internal Country Code and current Regulatory Revision are used to lookup the country info
 * structure and from that the country locales structure. The clm functions used for this are
 * processor intensive. Avoid calling this function in a loop, try using
 * wlc_get_reg_max_power_for_channel_ex() instead.
 * for external FALSE, it returns limit for brcm hw
 * ---- for 2.4GHz channel, it returns cck limit, not ofdm limit.
 * for external TRUE, it returns 802.11d Country Information Element -
 *    Maximum Transmit Power Level.
 */
int8
wlc_get_reg_max_power_for_channel(wlc_cm_info_t *wlc_cmi, chanspec_t chspec, bool external)
{
    clm_country_locales_t locales;
    clm_country_t country = wlc_cmi->country;
    clm_result_t result;

    if (country == CLM_ITER_NULL) {
        result = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
        if (result != CLM_RESULT_OK) {
            wlc_cmi->country = CLM_ITER_NULL;
            return WL_RATE_DISABLED;
        } else {
            wlc_cmi->country = country;
        }
    }

    result = wlc_get_locale(country, &locales);
    if (result != CLM_RESULT_OK) {
        return WL_RATE_DISABLED;
    }

    return wlc_get_reg_max_power_for_channel_ex(wlc_cmi, &locales, chspec, external);
}

/* Enum translation element - translates one value */
typedef struct wlc_enum_translation {
    uint from, to;
} wlc_enum_translation_t;

/* Enum translation. Returns 'to' from enum translation element with given 'from' (if found) or
 * 'def_value'
 */
static uint
wlc_translate_enum(const wlc_enum_translation_t *translation, size_t len, uint from, uint def_value)
{
    while (len--) {
        if (translation++->from == from) {
            return translation[-1].to;
        }
    }
    return def_value;
}

/* Get 6GHz regulatory max power from CLM (TPE tab) for Transmit Power Envelope (TPE) element */
int
wlc_get_6g_tpe_reg_max_power(wlc_cm_info_t *wlc_cmi, const clm_country_locales_t *locales,
    clm_tpe_regulatory_limits_t *limits, clm_regulatory_limit_dest_t limit_dest,
    clm_device_category_t device_category, chanspec_t chanspec)
{
    clm_result_t ret;
    clm_band_t band = wlc_channel_chanspec_to_clm_band(chanspec);
    clm_regulatory_power_limits_t reg_limits;
    clm_regulatory_limits_params_t reg_limits_params;
    clm_regulatory_limit_type_t rl_type;
    uint bw_loop, primary20_ch, center_ch, bw;
    chanspec_bw_t chspec_bw;
    chanspec_bw_t chspec_bw_loop;

    if (!CHSPEC_IS6G(chanspec)) {
        WL_ERROR(("wl%d: %s: Invalid band\n", wlc_cmi->pub->unit, __FUNCTION__));
    }

    memset(limits->limit, (unsigned char)WL_RATE_DISABLED, sizeof(limits->limit));

    (void)clm_regulatory_limits_params_init(&reg_limits_params);
    reg_limits_params.dest = limit_dest;
    reg_limits_params.device_category = device_category;
    reg_limits_params.ant_gain = 0;

    primary20_ch = wf_chspec_primary20_chan(chanspec);
    chspec_bw_loop = CHSPEC_BW(chanspec);
    if ((bw = wlc_convert_chanspec_bw_to_clm_bw(chspec_bw_loop)) == (uint)~0) {
        WL_ERROR(("wl%d: %s: Fail to get chanspec bandwidth\n",
                wlc_cmi->pub->unit, __FUNCTION__));
        return BCME_ERROR;
    }

    for (bw_loop = CLM_BW_20; bw_loop <= bw; bw_loop++) {
        reg_limits_params.bw = bw_loop;
        for (rl_type = CLM_REGULATORY_LIMIT_TYPE_CHANNEL;
            rl_type < CLM_REGULATORY_LIMIT_TYPE_NUM; rl_type++) {
            reg_limits_params.limit_type = rl_type;
            ret = clm_regulatory_limits(locales, band, &reg_limits_params, &reg_limits);
            if (ret != CLM_RESULT_OK) {
                WL_INFORM(("wl%d: %s: Fail to get CLM Regulatory Limits\n",
                        wlc_cmi->pub->unit, __FUNCTION__));
                return BCME_ERROR;
            }

            if (((chspec_bw = wlc_convert_clm_bw_to_chanspec_bw(bw_loop)))
                    == (chanspec_bw_t)~0) {
                WL_ERROR(("wl%d: %s: Fail to get chanspec bandwidth\n",
                        wlc_cmi->pub->unit, __FUNCTION__));
                return BCME_ERROR;
            }
            if ((center_ch = wf_6G_primary20_ch_to_center_ch(primary20_ch, chspec_bw))
                == INVCHANNEL) {
                WL_ERROR(("wl%d: %s: Fail to get center channel\n",
                        wlc_cmi->pub->unit, __FUNCTION__));
                return BCME_ERROR;
            }

            limits->limit[bw_loop][rl_type] = reg_limits.limits[center_ch];
        }
    }
    return BCME_OK;
}

static uint
wlc_convert_chanspec_bw_to_clm_bw(uint chanspec_bw)
{
    /* WL_CHANSPEC_BW_... -> clm_bandwidth_t translation */
    static const wlc_enum_translation_t bw_translation1[] = {
        {WL_CHANSPEC_BW_20, CLM_BW_20},
        {WL_CHANSPEC_BW_40, CLM_BW_40},
#ifdef WL11AC
#ifdef WL_CHANSPEC_BW_80
        {WL_CHANSPEC_BW_80, CLM_BW_80},
#endif /* WL_CHANSPEC_BW_80: */
#ifdef WL_CHANSPEC_BW_160
        {WL_CHANSPEC_BW_160, CLM_BW_160},
#endif /* WL_CHANSPEC_BW_160: */
#ifdef WL_CHANSPEC_BW_8080
        {WL_CHANSPEC_BW_8080, CLM_BW_80_80},
#endif /* WL_CHANSPEC_BW_8080 */
#endif /* WL11AC */
    };

    return wlc_translate_enum(bw_translation1,
            ARRAYSIZE(bw_translation1), chanspec_bw, ~0);
}

static uint
wlc_convert_clm_bw_to_chanspec_bw(uint clm_bw)
{
    static const wlc_enum_translation_t bw_translation[] = {
        {CLM_BW_20, WL_CHANSPEC_BW_20},
        {CLM_BW_40, WL_CHANSPEC_BW_40},
#ifdef WL11AC
#ifdef WL_CHANSPEC_BW_80
        {CLM_BW_80, WL_CHANSPEC_BW_80},
#endif /* WL_CHANSPEC_BW_80: */
#ifdef WL_CHANSPEC_BW_160
        {CLM_BW_160, WL_CHANSPEC_BW_160},
#endif /* WL_CHANSPEC_BW_160: */
#ifdef WL_CHANSPEC_BW_8080
        {CLM_BW_80_80, WL_CHANSPEC_BW_8080},
#endif /* WL_CHANSPEC_BW_8080 */
#endif /* WL11AC */
    };

    return wlc_translate_enum(bw_translation,
            ARRAYSIZE(bw_translation), clm_bw, ~0);
}

static void
wlc_channel_psdlimit_check(wlc_cm_info_t *wlc_cmi)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    clm_tpe_regulatory_limits_t limits;
    clm_result_t ret;
    clm_country_locales_t locales;
    clm_country_t country = wlc_cmi->country;

    wlc->stf->psd_limit_indicator = 0;
    if (wlc->lpi_mode == AUTO) {
        // indicate MAC the current LPI mode
        wlc_mhf(wlc, MHF3, MHF3_6G_LPI_MODE, 0, WLC_BAND_ALL);
    }

    if (!CHSPEC_IS6G(wlc->chanspec)) {
        return;
    }

    if (country == CLM_ITER_NULL) {
        ret = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
        if (ret != CLM_RESULT_OK) {
            wlc_cmi->country = CLM_ITER_NULL;
            ASSERT(0);
        } else {
            wlc_cmi->country = country;
        }
    }

    ret = wlc_get_locale(country, &locales);
    ASSERT(ret == CLM_RESULT_OK);

    wlc_get_6g_tpe_reg_max_power(wlc_cmi, &locales, &limits,
        CLM_REGULATORY_LIMIT_DEST_CLIENT,
        CLM_DEVICE_CATEGORY_LP, wlc->chanspec);
    //FIXME lpi indicator is not considered under STA/repeater mode
    if (limits.limit[0][0] > (limits.limit[0][1] + TENLOGTEN20MHz) &&
        limits.limit[1][0] > (limits.limit[1][1] + TENLOGTEN40MHz) &&
        limits.limit[2][0] > (limits.limit[2][1] + TENLOGTEN80MHz) &&
        (strcmp(wlc_channel_country_abbrev(wlc_cmi), "#a") != 0)) {
        /*
        PSD limit + 13dB (BW20 power) < EIRP &&
        PSD limit + 16dB (BW40 power) < EIRP &&
        PSD limit + 19dB (BW80 power) < EIRP && not country ALL
        */
        wlc->stf->psd_limit_indicator = 1;
        // indicate MAC the current LPI mode
        if (wlc->lpi_mode == AUTO) {
            wlc_mhf(wlc, MHF3, MHF3_6G_LPI_MODE, MHF3_6G_LPI_MODE, WLC_BAND_ALL);
        }
    }
}

#if defined(BCMDBG)
static int
wlc_dump_max_power_per_channel(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    wlc_info_t *wlc = wlc_cmi->wlc;

    if (CHSPEC_IS6G(wlc->chanspec)) {
        char chanspec_str[CHANSPEC_STR_LEN];
        clm_tpe_regulatory_limits_t limits;
        clm_result_t ret;
        clm_country_locales_t locales;
        clm_country_t country = wlc_cmi->country;
        chanspec_bw_t chspec_bw_loop;
        uint bw, bw_loop;
        const char *bw2str[] = {"20", "40", "80", "160"};

        if (country == CLM_ITER_NULL) {
            ret = wlc_country_lookup_direct(wlc_cmi->ccode, wlc_cmi->regrev, &country);
            if (ret != CLM_RESULT_OK) {
                wlc_cmi->country = CLM_ITER_NULL;
                return BCME_ERROR;
            } else {
                wlc_cmi->country = country;
            }
        }

        ret = wlc_get_locale(country, &locales);
        if (ret != CLM_RESULT_OK) {
            return BCME_ERROR;
        }

        wlc_get_6g_tpe_reg_max_power(wlc_cmi, &locales, &limits,
            CLM_REGULATORY_LIMIT_DEST_CLIENT,
            CLM_DEVICE_CATEGORY_LP, wlc->chanspec);

        chspec_bw_loop = CHSPEC_BW(wlc->chanspec);
        if ((bw = wlc_convert_chanspec_bw_to_clm_bw(chspec_bw_loop)) == (uint)~0) {
            WL_ERROR(("wl%d: %s: Fail to get chanspec bandwidth\n",
                    wlc_cmi->pub->unit, __FUNCTION__));
            return BCME_ERROR;
        }

        bcm_bprintf(b, "6GHz LPI AP chanspec %s (0x%X)\n",
            wf_chspec_ntoa(wlc->chanspec, chanspec_str), wlc->chanspec);
        bcm_bprintf(b, "Reg Max Power\n");

        for (bw_loop = CLM_BW_20; bw_loop <= bw; bw_loop++) {
            bcm_bprintf(b, "BW %sMHz  EIRP %d dBm PSD %d dBm/MHz\n",
                bw2str[bw_loop], limits.limit[bw_loop][0],
                limits.limit[bw_loop][1]);
        }
    } else {
        int8 ext_pwr = wlc_get_reg_max_power_for_channel(wlc_cmi, wlc->chanspec, TRUE);

        bcm_bprintf(b, "Reg Max Power (External) %d\n", ext_pwr);
    }
    return BCME_OK;
}

static int
wlc_get_clm_limits(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec, uint lim_count,
    clm_power_limits_t **limits, clm_ru_power_limits_t *ru_limits)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    unsigned int chan;
    clm_country_t country;
    clm_country_locales_t locale;
    clm_band_t bandtype;
    /* Max limit count is 4 includes 160MHz, 80in160MHz, 40in160MHz, 20in160MHz */
    clm_limits_type_t lim_types[4];
    wlcband_t *band;
    int ant_gain;
    clm_limits_params_t lim_params;
    uint i;

    if (!wlc_valid_chanspec_db(wlc_cmi, chanspec)) {
        WL_ERROR(("wl%d: %s: invalid chanspec 0x%04x\n",
            wlc->pub->unit, __FUNCTION__, chanspec));
        return BCME_BADCHAN;
    }

    country = wlc_cmi->country;
    if (wlc_get_locale(country, &locale) != CLM_RESULT_OK) {
        return BCME_ERROR;
    }

    if (clm_limits_params_init(&lim_params) != CLM_RESULT_OK) {
        return BCME_ERROR;
    }

    if (limits != NULL) {
        for (i = 0; i < lim_count; i++) {
            if (limits[i] != NULL) {
                memset(limits[i], (unsigned char)WL_RATE_DISABLED,
                    sizeof(clm_power_limits_t));
            } else {
                break;
            }
        }
        if (i != lim_count) {
            WL_ERROR(("wl%d: %s: Allocated memory is not sufficient\n",
                wlc->pub->unit, __FUNCTION__));
            return BCME_ERROR;
        }
    }
    if (ru_limits != NULL) {
        memset(ru_limits, (unsigned char)WL_RATE_DISABLED, sizeof(clm_ru_power_limits_t));
    }

    chan = CHSPEC_CHANNEL(chanspec);
    band = wlc->bandstate[CHSPEC_BANDUNIT(chanspec)];
    bandtype = BANDTYPE2CLMBAND(band->bandtype);
    ant_gain = band->antgain;
    band->sar = band->sar_cached;
    if (wlc_cmi->sar_enable) {
        lim_params.sar = band->sar;
    }
    if (strcmp(wlc_cmi->country_abbrev, "#a") == 0) {
        band->sar = WLC_TXPWR_MAX;
        lim_params.sar = WLC_TXPWR_MAX;
    }

    lim_types[0] = CLM_LIMITS_TYPE_CHANNEL;
    switch (CHSPEC_BW(chanspec)) {

    case WL_CHANSPEC_BW_20:
        lim_params.bw = CLM_BW_20;
        break;

    case WL_CHANSPEC_BW_40:
        lim_params.bw = CLM_BW_40;
        /* 20in40MHz */
        lim_types[1] = clm_chanspec_to_limits_type(chanspec);
        break;

    case WL_CHANSPEC_BW_80: {
        clm_limits_type_t ctl_limits_type =
            clm_chanspec_to_limits_type(chanspec);

        lim_params.bw = CLM_BW_80;
        /* 40in80MHz */
        lim_types[1] = clm_get_enclosing_subchan(ctl_limits_type, 1);
        /* 20in80MHz */
        lim_types[2] = ctl_limits_type;
        break;
    }

    case WL_CHANSPEC_BW_160: {
        clm_limits_type_t ctl_limits_type =
            clm_chanspec_to_limits_type(chanspec);

        lim_params.bw = CLM_BW_160;
        /* 80in160MHz */
        lim_types[1] = clm_get_enclosing_subchan(ctl_limits_type, 1);
        /* 40in160MHz */
        lim_types[2] = clm_get_enclosing_subchan(ctl_limits_type, 2);
        /* 20in160MHz */
        lim_types[3] = ctl_limits_type;
        break;
    }

    default:
        WL_ERROR(("wl%d: %s: Unsupported bandwidth 0x%04x\n",
            wlc->pub->unit, __FUNCTION__, CHSPEC_BW(chanspec)));
        return BCME_UNSUPPORTED;
    }

    if (limits != NULL) {
        /* Calculate limits for each (sub)channel */
        for (i = 0; i < lim_count; i++) {
            if (limits[i] != NULL) {
                clm_limits(&locale, bandtype, chan, ant_gain, lim_types[i],
                    &lim_params, limits[i]);
            }
        }
    }

    if (ru_limits != NULL) {
        clm_ru_limits(&locale, bandtype, chan, ant_gain, lim_types[0], &lim_params,
            ru_limits);
    }

    return BCME_OK;
}

/* Return TRUE if HE rate index of clm_rates_t. bcmwifi_rates.h */
static bool
wlc_is_he_rate_index(clm_rates_t rate_i)
{
    if (((rate_i >= WL_RATE_1X1_HE0SS1) && (rate_i <= WL_RATE_1X1_HE11SS1)) ||
        ((rate_i >= WL_RATE_1X2_HE0SS1) && (rate_i <= WL_RATE_1X2_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X2_HE0SS2) && (rate_i <= WL_RATE_2X2_HE11SS2)) ||
        ((rate_i >= WL_RATE_1X2_TXBF_HE0SS1) && (rate_i <= WL_RATE_1X2_TXBF_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X2_TXBF_HE0SS2) && (rate_i <= WL_RATE_2X2_TXBF_HE11SS2)) ||
        ((rate_i >= WL_RATE_1X3_HE0SS1) && (rate_i <= WL_RATE_1X3_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X3_HE0SS2) && (rate_i <= WL_RATE_2X3_HE11SS2)) ||
        ((rate_i >= WL_RATE_3X3_HE0SS3) && (rate_i <= WL_RATE_3X3_HE11SS3)) ||
        ((rate_i >= WL_RATE_1X3_TXBF_HE0SS1) && (rate_i <= WL_RATE_1X3_TXBF_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X3_TXBF_HE0SS2) && (rate_i <= WL_RATE_2X3_TXBF_HE11SS2)) ||
        ((rate_i >= WL_RATE_3X3_TXBF_HE0SS3) && (rate_i <= WL_RATE_3X3_TXBF_HE11SS3)) ||
        ((rate_i >= WL_RATE_1X4_HE0SS1) && (rate_i <= WL_RATE_1X4_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X4_HE0SS2) && (rate_i <= WL_RATE_2X4_HE11SS2)) ||
        ((rate_i >= WL_RATE_3X4_HE0SS3) && (rate_i <= WL_RATE_3X4_HE11SS3)) ||
        ((rate_i >= WL_RATE_4X4_HE0SS4) && (rate_i <= WL_RATE_4X4_HE11SS4)) ||
        ((rate_i >= WL_RATE_1X4_TXBF_HE0SS1) && (rate_i <= WL_RATE_1X4_TXBF_HE11SS1)) ||
        ((rate_i >= WL_RATE_2X4_TXBF_HE0SS2) && (rate_i <= WL_RATE_2X4_TXBF_HE11SS2)) ||
        ((rate_i >= WL_RATE_3X4_TXBF_HE0SS3) && (rate_i <= WL_RATE_3X4_TXBF_HE11SS3)) ||
        ((rate_i >= WL_RATE_4X4_TXBF_HE0SS4) && (rate_i <= WL_RATE_4X4_TXBF_HE11SS4))) {
        return TRUE;
    }
    return FALSE;
}

static int
wlc_clm_limits_dump(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b, bool he)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    /* Max limit count is 4 includes 160MHz, 80in160MHz, 40in160MHz, 20in160MHz */
    clm_power_limits_t *limits[4] = {NULL, NULL, NULL, NULL};
    chanspec_t chanspec = phy_utils_get_chanspec(wlc->pi);
    uint16 lim_count = 0, bw, j;
    clm_rates_t rate_i;
    char chanspec_str[CHANSPEC_STR_LEN];
    char tmp[6];
    int ret = BCME_OK;

    bw = CHSPEC_BW(chanspec);
    switch (bw) {
        case WL_CHANSPEC_BW_20:
            /* 20MHz */
            lim_count = 1;
            break;
        case WL_CHANSPEC_BW_40:
            /* 40MHz, 20in40MHz */
            lim_count = 2;
            break;
        case WL_CHANSPEC_BW_80:
            /* 80MHz, 40in80MHz, 20in80MHz */
            lim_count = 3;
            break;
        case WL_CHANSPEC_BW_160:
            /* 160MHz, 80in160MHz, 40in160MHz, 20in160MHz */
            lim_count = 4;
            break;
        default:
            WL_ERROR(("wl%d: %s: Unsupported bandwidth 0x%X\n",
                wlc_cmi->pub->unit, __FUNCTION__, CHSPEC_BW(chanspec)));
            return BCME_UNSUPPORTED;
    }

    for (j = 0; j < lim_count; j++) {
        if ((limits[j] = (clm_power_limits_t *)MALLOCZ(wlc_cmi->wlc->osh, WL_NUMRATES))
            == NULL) {
            WL_ERROR(("wl%d: %s: out of memory", wlc_cmi->pub->unit, __FUNCTION__));
            ret = BCME_NOMEM;
            goto done;
        }
    }

    ret = wlc_get_clm_limits(wlc_cmi, chanspec, lim_count, limits, NULL);
    if (ret != BCME_OK)
        goto done;

    /* Print header */
    bcm_bprintf(b, "%s%s\n", "Current Channel: ", wf_chspec_ntoa(chanspec, chanspec_str));
    bcm_bprintf(b, "%s", "Band: ");
    switch (CHSPEC_BAND(chanspec)) {
        case WL_CHANSPEC_BAND_2G:
            bcm_bprintf(b, "%s\n", "2.4GHz");
            break;
        case WL_CHANSPEC_BAND_5G:
            bcm_bprintf(b, "%s\n", "5GHz");
            break;
        case WL_CHANSPEC_BAND_6G:
            bcm_bprintf(b, "%s\n", "6GHz");
            break;
        default:
            bcm_bprintf(b, "%s\n", "Incorrect Band");
            ret = BCME_BADBAND;
            goto done;
    }
    switch (bw) {
        case WL_CHANSPEC_BW_20:
            bcm_bprintf(b, "\nRate 20MHz\n");
            break;
        case WL_CHANSPEC_BW_40:
            bcm_bprintf(b, "\n     20in\n");
            bcm_bprintf(b, "Rate 40   40MHz\n");
            break;
        case WL_CHANSPEC_BW_80:
            bcm_bprintf(b, "\n     20in 40in \n");
            bcm_bprintf(b, "Rate 80   80   80MHz\n");
            break;
        case WL_CHANSPEC_BW_160:
            bcm_bprintf(b, "\n     20in 40in 80in\n");
            bcm_bprintf(b, "Rate 160  160  160  160MHz\n");
            break;
    }
    /* Print rate index (clm_rates_t, bcmwifi_rates.h) and clm power limits */
    for (rate_i = 0; rate_i < WL_NUMRATES; rate_i++) {
        if (he) {
            if (!wlc_is_he_rate_index(rate_i))
                continue;
        } else {
            if (wlc_is_he_rate_index(rate_i))
                continue;
        }

        sprintf(tmp, "%d", rate_i);
        bcm_bprintf(b, "%-5s", tmp);
        for (j = 1; j <= lim_count; j++) {
            if (limits[lim_count - j]->limit[rate_i] == WL_RATE_DISABLED) {
                sprintf(tmp, "-");
            } else {
                sprintf(tmp, "%d", limits[lim_count - j]->limit[rate_i]);
            }
            if (j == lim_count) {
                bcm_bprintf(b, "%s", tmp);
            } else {
                bcm_bprintf(b, "%-5s", tmp);
            }
        }
        bcm_bprintf(b, "\n");
    }
done:
    for (j = 0; j < lim_count; j++) {
        if (limits[j] != NULL) {
            MFREE(wlc_cmi->wlc->osh, limits[j], WL_NUMRATES);
        }
    }
    return ret;
}

/* Dump non-HE clm power limits per rate index of clm_rates_t. bcmwifi_rates.h */
static int
wlc_dump_clm_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    return wlc_clm_limits_dump(wlc_cmi, b, FALSE);
}

/* Dump HE clm power limits per rate index of clm_rates_t. bcmwifi_rates.h */
static int
wlc_dump_clm_he_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    return wlc_clm_limits_dump(wlc_cmi, b, TRUE);
}

/* Dump RU/UB/LUB clm power limits per rate index of clm_ru_rates_t. bcmwifi_rates.h */
static int
wlc_dump_clm_ru_limits(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    clm_ru_power_limits_t *ru_limits = NULL;
    chanspec_t chanspec = phy_utils_get_chanspec(wlc->pi);
    clm_ru_rates_t rate_i;
    char chanspec_str[CHANSPEC_STR_LEN];
    char tmp[6];
    int ret = BCME_OK;

    if ((ru_limits = (clm_ru_power_limits_t *)MALLOCZ(wlc_cmi->wlc->osh, WL_RU_NUMRATES))
        == NULL) {
        WL_ERROR(("wl%d: %s: out of memory", wlc_cmi->pub->unit, __FUNCTION__));
        return BCME_NOMEM;
    }

    ret = wlc_get_clm_limits(wlc_cmi, chanspec, 0, NULL, ru_limits);
    if (ret != BCME_OK)
        goto done;

    /* Print header */
    bcm_bprintf(b, "%s%s\n", "Current Channel: ", wf_chspec_ntoa(chanspec, chanspec_str));
    bcm_bprintf(b, "%s", "Band: ");
    switch (CHSPEC_BAND(chanspec)) {
        case WL_CHANSPEC_BAND_2G:
            bcm_bprintf(b, "%s\n", "2.4GHz");
            break;
        case WL_CHANSPEC_BAND_5G:
            bcm_bprintf(b, "%s\n", "5GHz");
            break;
        case WL_CHANSPEC_BAND_6G:
            bcm_bprintf(b, "%s\n", "6GHz");
            break;
        default:
            bcm_bprintf(b, "%s\n", "Incorrect Band");
            ret = BCME_BADBAND;
            goto done;
    }
    switch (CHSPEC_BW(chanspec)) {
        case WL_CHANSPEC_BW_20:
            bcm_bprintf(b, "\nRate 20MHz\n");
            break;
        case WL_CHANSPEC_BW_40:
            bcm_bprintf(b, "\nRate 40MHz\n");
            break;
        case WL_CHANSPEC_BW_80:
            bcm_bprintf(b, "\nRate 80MHz\n");
            break;
        case WL_CHANSPEC_BW_160:
            bcm_bprintf(b, "\nRate 160MHz\n");
            break;
    }
    /* Print rate index (clm_ru_rates_t, bcmwifi_rates.h) and RU/UB/LUB clm power limits */
    for (rate_i = 0; rate_i < WL_RU_NUMRATES; rate_i++) {
        sprintf(tmp, "%d", rate_i);
        bcm_bprintf(b, "%-5s", tmp);

        if (ru_limits->limit[rate_i] == WL_RATE_DISABLED) {
            sprintf(tmp, "-");
        } else {
            sprintf(tmp, "%d", ru_limits->limit[rate_i]);
        }
        bcm_bprintf(b, "%s\n", tmp);
    }
done:
    if (ru_limits != NULL) {
        MFREE(wlc_cmi->wlc->osh, ru_limits, WL_RU_NUMRATES);
    }
    return ret;
}
#endif

static bool
wlc_channel_clm_chanspec_valid(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    if (CHSPEC_IS2G(chspec)) {
        return TRUE;
    } else if (CHSPEC_IS5G(chspec)) {
        if (CHSPEC_IS20(chspec) || CHSPEC_IS40(chspec) || CHSPEC_IS80(chspec)) {
            return isset(wlc_cmi->allowed_5g_channels.vec, CHSPEC_CHANNEL(chspec));
        } else if (CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec)) {
            return (isset(wlc_cmi->allowed_5g_channels.vec,
                wf_chspec_primary80_channel(chspec))&&
                isset(wlc_cmi->allowed_5g_channels.vec,
                wf_chspec_secondary80_channel(chspec)));
        }
    } else if (CHSPEC_IS6G(chspec)) {
        return isset(wlc_cmi->allowed_6g_channels.vec, CHSPEC_CHANNEL(chspec));
    }
    return FALSE;
}

    /* dump_flag_qqdx */
extern bool start_game_is_on;
extern uint32 channel_set_print_flag_qqdx;
bool print_flag_qqdx = FALSE;
extern chanspec_t chanspec_real_set;
    /* dump_flag_qqdx */
/**
 * Validate the chanspec for this locale, for 40MHz we need to also check that the sidebands
 * are valid 20MHz channels in this locale and they are also a legal HT combination
 */
static bool
wlc_valid_chanspec_ext(wlc_cm_info_t *wlc_cmi, chanspec_t chspec, bool current_bu)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint8 channel = CHSPEC_CHANNEL(chspec);
    uint8 cmn_bwcap = WLC_BW_CAP_20MHZ;
    enum wlc_bandunit bandunit;
    int bandtype;
    
    /* dump_flag_qqdx */
    if(start_game_is_on&&(channel_set_print_flag_qqdx>OSL_SYSUPTIME())&&(channel_set_print_flag_qqdx<(OSL_SYSUPTIME()+100))){
        print_flag_qqdx = TRUE;
        printk("wlc_valid_chanspec_ext_time:(%u;%u)",channel_set_print_flag_qqdx,OSL_SYSUPTIME());
        if(chanspec_real_set != chspec){
            //dump_stack();//仅打印未知来源的chspec来源。
        }
    }else{
        print_flag_qqdx = FALSE;
    }
    /* dump_flag_qqdx */
    /* AirIQ uses chanspec 7/80, 14/80. Make exception */
#if defined(WL_AIR_IQ)
    if (wlc->scan->state & SCAN_STATE_PROHIBIT) {
        if (CHSPEC_IS2G(chspec) && CHSPEC_IS80(chspec) && ((CHSPEC_CHANNEL(chspec) == 7) ||
            (CHSPEC_CHANNEL(chspec) == 14))) {
    /* dump_flag_qqdx */
    if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext0:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return TRUE;
        }
    }
#endif /* WL_AIR_IQ */               


    /* check the chanspec */
    if (!wf_chspec_valid(chspec)) {
        WL_NONE(("wl%d: invalid 802.11 chanspec 0x%x\n", wlc->pub->unit, chspec));
    /* dump_flag_qqdx */
        if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext1:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return FALSE;
    }

    /* reject chanspec for band that is not enabled */
    /* For RNR, band could be different from current enabled band be vigilent */
    if (!CHSPEC_IS6G(chspec) && !BAND_ENABLED(wlc, CHSPEC_BANDUNIT(chspec))) {
    /* dump_flag_qqdx */
        if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext2:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return FALSE;
    }

    cmn_bwcap = (wlc->pub->phy_bw160_capable) ? WLC_BW_CAP_160MHZ :
        ((wlc->pub->phy_bw80_capable) ? WLC_BW_CAP_80MHZ :
        ((wlc->pub->phy_bw40_capable) ? WLC_BW_CAP_40MHZ : WLC_BW_CAP_20MHZ));

    if (CHSPEC_BANDUNIT(WL_CHANNEL_2G5G_BAND(channel)) != CHSPEC_BANDUNIT(chspec) &&
            !CHSPEC_IS6G(chspec)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext3:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;

        }

    if (CHSPEC_IS5G(chspec) && IS_5G_CH_GRP_DISABLED(wlc, channel)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext4:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return FALSE;
    }

    bandunit = CHSPEC_BANDUNIT(chspec);
    bandtype = CHSPEC_BANDTYPE(chspec);

    /* Check a 20Mhz channel */
    if (CHSPEC_IS20(chspec)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext5:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return wlc_valid_channel20(wlc_cmi, chspec, current_bu);
    } else if (CHSPEC_IS40(chspec)) { /* Check a 40Mhz channel */
        if (!WL_BW_CAP_40MHZ(cmn_bwcap)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext6:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        if (!VALID_40CHANSPEC_IN_BAND(wlc, bandunit)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext7:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;


        }
        if (!wlc_valid_channel20(wlc_cmi,
             CH20MHZ_CHSPEC(LOWER_20_SB(channel),
                BANDTYPE_CHSPEC(bandtype)), current_bu) ||
            !wlc_valid_channel20(wlc_cmi,
             CH20MHZ_CHSPEC(UPPER_20_SB(channel),
                BANDTYPE_CHSPEC(bandtype)), current_bu)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext8:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;

                }

        if (!wlc_channel_clm_chanspec_valid(wlc_cmi, chspec)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext9:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;

        }

        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext10:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return TRUE;
    } else if (CHSPEC_IS80(chspec)) { /* Check a 80MHz channel - only 5G band supports 80MHz */
        chanspec_t chspec40;

        /* Only 5G and 6G support 80MHz
         * Check the chanspec band with BAND_5G6G(). BAND_5G6G() is conditionally compiled
         * on BAND5G/6G support. This check will turn into a constant 'FALSE' when compiling
         * without BAND5G *and* BAND6G support.
         */
        if (!BAND_5G6G(bandtype)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext11:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        /* Make sure that the phy is 80MHz capable and that
         * we are configured for 80MHz on the band
         */
        if (!WL_BW_CAP_80MHZ(cmn_bwcap)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext12:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        /* Ensure that vhtmode is enabled if applicable */
        if (!VHT_ENAB_BAND(wlc->pub, bandtype) && !HE_ENAB_BAND(wlc->pub, bandtype)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext13:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        if (!VALID_80CHANSPEC_IN_BAND(wlc, bandunit)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext14:(0x%04x)",chspec);
        /* dump_flag_qqdx */

            return FALSE;
        }

        if (!wlc_channel_clm_chanspec_valid(wlc_cmi, chspec)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext15:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;

        }

        /* Make sure both 40 MHz side channels are valid
         * Create a chanspec for each 40MHz side side band and check
         */
        chspec40 = (chanspec_t)((channel - CH_20MHZ_APART) |
                                WL_CHANSPEC_CTL_SB_L |
                                WL_CHANSPEC_BW_40 |
                                CHSPEC_BAND(chspec));
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext16-:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        if (!wlc_valid_chanspec_ext(wlc_cmi, chspec40, current_bu)) {
            WL_TMP(("wl%d: %s: 80MHz: chanspec %0X -> chspec40 %0X "
                    "failed valid check\n",
                    wlc->pub->unit, __FUNCTION__, chspec, chspec40));
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext16:(0x%04x)",chspec);
        /* dump_flag_qqdx */

            return FALSE;
        }

        chspec40 = (chanspec_t)((channel + CH_20MHZ_APART) |
                                WL_CHANSPEC_CTL_SB_L |
                                WL_CHANSPEC_BW_40 |
                                CHSPEC_BAND(chspec));
                                
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext17-:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        if (!wlc_valid_chanspec_ext(wlc_cmi, chspec40, current_bu)) {
            WL_TMP(("wl%d: %s: 80MHz: chanspec %0X -> chspec40 %0X "
                    "failed valid check\n",
                    wlc->pub->unit, __FUNCTION__, chspec, chspec40));

        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext17:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext18:(0x%04x)",chspec);
        /* dump_flag_qqdx */

        return TRUE;
    } else if (CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec)) {
        chanspec_t chspec80;

        /* Only 5G and 6G support 80 +80 MHz
         * Check the chanspec band with BAND_5G6G() since BAND_5G6G() is conditionally
         * compiled on BAND5G/BAND6G support. This check will turn into a constant 'FALSE'
         * when compiling without BAND5G *and* BAND6G support.
         */
        if (!BAND_5G6G(bandtype)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext19:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        /* Make sure that the phy is 160/80+80 MHz capable and that
         * we are configured for 160MHz on the band
         */
        if (!WL_BW_CAP_160MHZ(cmn_bwcap)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext20:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        /* Ensure that vhtmode is enabled if applicable */
        if (!VHT_ENAB_BAND(wlc->pub, bandtype) && !HE_ENAB_BAND(wlc->pub, bandtype)) {
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext21:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        if (CHSPEC_IS8080(chspec)) {
            if (!VALID_8080CHANSPEC_IN_BAND(wlc, bandunit)){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext22:(0x%04x)",chspec);
        /* dump_flag_qqdx */
                return FALSE;

            }
        }

        if (CHSPEC_IS160(chspec)) {
            if (!VALID_160CHANSPEC_IN_BAND(wlc, CHSPEC_BANDUNIT(chspec))){
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext23:(0x%04x)",chspec);
        /* dump_flag_qqdx */
                return FALSE;

            }
        }

        chspec80 = (chanspec_t)(wf_chspec_primary80_channel(chspec) |
                                WL_CHANSPEC_CTL_SB_L |
                                WL_CHANSPEC_BW_80 |
                                CHSPEC_BAND(chspec));
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext24-:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        if (!wlc_valid_chanspec_ext(wlc_cmi, chspec80, current_bu)) {
            WL_TMP(("wl%d: %s: 80 + 80 MHz: chanspec %0X -> chspec80 %0X "
                    "failed valid check\n",
                    wlc->pub->unit, __FUNCTION__, chspec, chspec80));

        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext24:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }

        chspec80 = (chanspec_t)(wf_chspec_secondary80_channel(chspec) |
                                WL_CHANSPEC_CTL_SB_L |
                                WL_CHANSPEC_BW_80 |
                                CHSPEC_BAND(chspec));
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext25-:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        if (!wlc_valid_chanspec_ext(wlc_cmi, chspec80, current_bu)) {
            WL_TMP(("wl%d: %s: 80 + 80 MHz: chanspec %0X -> chspec80 %0X "
                    "failed valid check\n",
                    wlc->pub->unit, __FUNCTION__, chspec, chspec80));

        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext25:(0x%04x)",chspec);
        /* dump_flag_qqdx */
            return FALSE;
        }
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext26:(0x%04x)",chspec);
        /* dump_flag_qqdx */
        return TRUE;
    }
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_ext27:(0x%04x)",chspec);
        /* dump_flag_qqdx */

    return FALSE;
}

bool
wlc_valid_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec:(0x%04x)",chspec);
        /* dump_flag_qqdx */
    return wlc_valid_chanspec_ext(wlc_cmi, chspec, TRUE);
}

bool
wlc_valid_chanspec_db(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
        /* dump_flag_qqdx */
            if(print_flag_qqdx)        printk("wlc_valid_chanspec_db:(0x%04x)",chspec);
        /* dump_flag_qqdx */
    return wlc_valid_chanspec_ext(wlc_cmi, chspec, FALSE);
}

/*
 *  Fill in 'list' with validated chanspecs, looping through channels using the chanspec_mask.
 */
static void
wlc_chanspec_list_ordered(wlc_cm_info_t *wlc_cmi, wl_uint32_list_t *list, chanspec_t chanspec_mask)
{
    uint8 channel, start_chan = 0, end_chan = MAXCHANNEL - 1, inc_chan = 1;
    chanspec_t chanspec;
    uint j;
    static const uint16 ctl_sb_bw160[] = {
        WL_CHANSPEC_CTL_SB_LLL,
        WL_CHANSPEC_CTL_SB_LLU,
        WL_CHANSPEC_CTL_SB_LUL,
        WL_CHANSPEC_CTL_SB_LUU,
        WL_CHANSPEC_CTL_SB_ULL,
        WL_CHANSPEC_CTL_SB_ULU,
        WL_CHANSPEC_CTL_SB_UUL,
        WL_CHANSPEC_CTL_SB_UUU
    };
    static const uint16 ctl_sb_bw80[] = {
        WL_CHANSPEC_CTL_SB_LL,
        WL_CHANSPEC_CTL_SB_LU,
        WL_CHANSPEC_CTL_SB_UL,
        WL_CHANSPEC_CTL_SB_UU
    };
    static const uint16 ctl_sb_bw40[] = {
        WL_CHANSPEC_CTL_SB_LOWER,
        WL_CHANSPEC_CTL_SB_UPPER
    };
    static const uint16 ctl_sb_bw20[] = {
        WL_CHANSPEC_CTL_SB_LOWER,
    };
    uint16 const *ctl_sb = NULL;
    uint ctl_sb_size = 0;

    switch (CHSPEC_BAND(chanspec_mask)) {
    case WL_CHANSPEC_BAND_2G:
        switch (CHSPEC_BW(chanspec_mask)) {
        case WL_CHANSPEC_BW_20:
            start_chan = CH_MIN_2G_CHANNEL;
            end_chan = CH_MAX_2G_CHANNEL;
            ctl_sb = ctl_sb_bw20;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw20);
            break;
        case WL_CHANSPEC_BW_40:
            start_chan = CH_MIN_2G_40M_CHANNEL;
            end_chan = CH_MAX_2G_40M_CHANNEL;
            ctl_sb = ctl_sb_bw40;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw40);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    case WL_CHANSPEC_BAND_5G:
        /*
         * In 5G there is a gap so we must increment
         * channels one by one so not as efficient as
         * the 6G band below.
         */
        switch (CHSPEC_BW(chanspec_mask)) {
        case WL_CHANSPEC_BW_20:
            start_chan = CH_MIN_5G_CHANNEL;
            end_chan = CH_MAX_5G_CHANNEL;
            ctl_sb = ctl_sb_bw20;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw20);
            break;
        case WL_CHANSPEC_BW_40:
            start_chan = CH_MIN_5G_40M_CHANNEL;
            end_chan = CH_MAX_5G_40M_CHANNEL;
            ctl_sb = ctl_sb_bw40;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw40);
            break;
        case WL_CHANSPEC_BW_80:
            start_chan = CH_MIN_5G_80M_CHANNEL;
            end_chan = CH_MAX_5G_80M_CHANNEL;
            ctl_sb = ctl_sb_bw80;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw80);
            break;
        case WL_CHANSPEC_BW_8080:
        case WL_CHANSPEC_BW_160:
            start_chan = CH_MIN_5G_160M_CHANNEL;
            end_chan = CH_MAX_5G_160M_CHANNEL;
            ctl_sb = ctl_sb_bw160;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw160);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    case WL_CHANSPEC_BAND_6G:
        switch (CHSPEC_BW(chanspec_mask)) {
        case WL_CHANSPEC_BW_20:
            start_chan = CH_MIN_6G_CHANNEL;
            end_chan = CH_MAX_6G_CHANNEL;
            inc_chan = CH_20MHZ_APART;
            ctl_sb = ctl_sb_bw20;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw20);
            /* For 6G channel 2 */
            chanspec = (chanspec_mask | 2 | ctl_sb[0]);
            if (wlc_valid_chanspec_db(wlc_cmi, chanspec)) {
                list->element[list->count] = chanspec;
                list->count++;
            }
            break;
        case WL_CHANSPEC_BW_40:
            start_chan = CH_MIN_6G_40M_CHANNEL;
            end_chan = CH_MAX_6G_40M_CHANNEL;
            inc_chan = CH_20MHZ_APART << 1;
            ctl_sb = ctl_sb_bw40;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw40);
            break;
        case WL_CHANSPEC_BW_80:
            start_chan = CH_MIN_6G_80M_CHANNEL;
            end_chan = CH_MAX_6G_80M_CHANNEL;
            inc_chan = CH_20MHZ_APART << 2;
            ctl_sb = ctl_sb_bw80;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw80);
            break;
        case WL_CHANSPEC_BW_8080:
        case WL_CHANSPEC_BW_160:
            start_chan = CH_MIN_6G_160M_CHANNEL;
            end_chan = CH_MAX_6G_160M_CHANNEL;
            inc_chan = CH_20MHZ_APART << 3;
            ctl_sb = ctl_sb_bw160;
            ctl_sb_size = ARRAYSIZE(ctl_sb_bw160);
            break;
        default:
            ASSERT(0);
            break;
        }
        break;
    default:
        ASSERT(0);
        break;
    }

    for (channel = start_chan; channel <= end_chan; channel += inc_chan) {
        for (j = 0; j < ctl_sb_size; j++) {
            chanspec = (chanspec_mask | channel | ctl_sb[j]);
            if (wlc_valid_chanspec_db(wlc_cmi, chanspec)) {
                list->element[list->count] = chanspec;
                list->count++;
            }
        }
    }
}

/*
 * Returns a list of valid chanspecs meeting the provided settings
 */
void
wlc_get_valid_chanspecs(wlc_cm_info_t *wlc_cmi, wl_uint32_list_t *list, enum wlc_bandunit bu_req,
    uint8 bwmask, const char *abbrev)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    chanspec_t chanspec;
    clm_country_t country;
    clm_result_t result = CLM_RESULT_ERR;
    clm_result_t flag_result = CLM_RESULT_ERR;
    uint16 flags;
    clm_country_locales_t locale;
    chanvec_t saved_valid_channels[MAXBANDS], locale_channels, unused;
    uint16 saved_locale_flags[MAXBANDS];
    uint8 saved_bwcap[MAXBANDS];
#ifdef WL11AC
    int bandtype;
#endif /* WL11AC */
    enum wlc_bandunit bandunit;
    wlcband_t *band;
    bool want_80p80 = (bwmask == WLC_VALID_CHANSPECS_WANT_80P80);
    bool bandlocked = wlc->bandlocked;
    char ccode[WLC_CNTRY_BUF_SZ];
    uint regrev;

    /* Check if request band is valid for this card */
    if (!BAND_ENABLED(wlc, bu_req))
        return;

    bwmask &= (wlc->pub->phy_bw160_capable) ? WLC_BW_CAP_160MHZ :
        ((wlc->pub->phy_bw80_capable) ? WLC_BW_CAP_80MHZ :
        ((wlc->pub->phy_bw40_capable) ? WLC_BW_CAP_40MHZ : WLC_BW_CAP_20MHZ));

    /* see if we need to look up country. Else, current locale */
    if (strcmp(abbrev, "")) {
        if (!strcmp(wlc->cmi->country_abbrev, abbrev) ||
            !strcmp(wlc->cmi->ccode, abbrev)) {
            /* If the desired ccode matches the current advertised ccode
             * or the current ccode, set regrev for the current regrev
             * and set ccode for the current ccode
             */
            strncpy(ccode, wlc->cmi->ccode, WLC_CNTRY_BUF_SZ-1);
            ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
            regrev = wlc->cmi->regrev;
        } else {
            /* Set default regrev for 0 */
            strncpy(ccode, abbrev, WLC_CNTRY_BUF_SZ-1);
            ccode[WLC_CNTRY_BUF_SZ-1] = '\0';
            regrev = 0;
        }

        result = wlc_country_lookup(wlc, ccode, regrev, &country);
        if (result != CLM_RESULT_OK) {
            WL_REGULATORY(("Unsupported country \"%s\"\n", abbrev));
            return;
        }
        result = wlc_get_locale(country, &locale);

        flag_result = wlc_get_flags(&locale, wlc_bandunit2clmband(bu_req), &flags);
        BCM_REFERENCE(flag_result);
    }

    /* Save current locales */
    if (result == CLM_RESULT_OK) {

        wlc_locale_get_channels(&locale, wlc_bandunit2clmband(bu_req),
            &locale_channels, &unused);
        FOREACH_WLC_BAND(wlc, bandunit) {
            bcopy(&wlc_cmi->bandstate[bandunit].valid_channels,
                &saved_valid_channels[bandunit], sizeof(chanvec_t));
            bcopy(&locale_channels, &wlc_cmi->bandstate[bandunit].valid_channels,
                sizeof(chanvec_t));
            saved_locale_flags[bandunit] = wlc_cmi->bandstate[bandunit].locale_flags;
            wlc_cmi->bandstate[bandunit].locale_flags = flags;
        }
        /* for specific locale ignore bandlock */
        wlc->bandlocked = FALSE;
    }

    FOREACH_WLC_BAND(wlc, bandunit) {
        /* save bw_cap */
        band = wlc->bandstate[bandunit];
        saved_bwcap[bandunit] = band->bw_cap;
        band->bw_cap = bwmask;
    }

    /* Go through 20MHZ chanspecs */
    if (WL_BW_CAP_20MHZ(bwmask)) {
        chanspec = wlc_bandunit2chspecband(bu_req) | WL_CHANSPEC_BW_20;
        wlc_chanspec_list_ordered(wlc_cmi, list, chanspec);
    }

    /* Go through 40MHZ chanspecs only if N mode and PHY is capable of 40MHZ */
    if (WL_BW_CAP_40MHZ(bwmask) && N_ENAB(wlc->pub)) {
        chanspec = wlc_bandunit2chspecband(bu_req);
        chanspec |= WL_CHANSPEC_BW_40;
        wlc_chanspec_list_ordered(wlc_cmi, list, chanspec);
    }

#ifdef WL11AC
    bandtype = wlc_bandunit2bandtype(bu_req);

    /* Go through 80MHZ chanspecs only if VHT mode and PHY is capable of 80MHZ  */
    if ((bu_req != BAND_2G_INDEX) && WL_BW_CAP_80MHZ(bwmask) &&
        (VHT_ENAB_BAND(wlc->pub, bandtype) || HE_ENAB_BAND(wlc->pub, bandtype))) {
        chanspec = wlc_bandunit2chspecband(bu_req);
        chanspec |= WL_CHANSPEC_BW_80;
        wlc_chanspec_list_ordered(wlc_cmi, list, chanspec);
    }

     /* Go through 8080MHZ chanspecs only if VHT mode and PHY is capable of 8080MHZ  */
    if (want_80p80 && (VHT_ENAB_BAND(wlc->pub, bandtype) || HE_ENAB_BAND(wlc->pub, bandtype))) {
        uint16 ctl_sb[] = {
            WL_CHANSPEC_CTL_SB_LLL,
            WL_CHANSPEC_CTL_SB_LLU,
            WL_CHANSPEC_CTL_SB_LUL,
            WL_CHANSPEC_CTL_SB_LUU,
        };
        if (bu_req == BAND_5G_INDEX) {
            uint i;
            int j;

            /* List of all valid channel ID combinations */
            static const uint8 chan_id[] = {
                0x20,
                0x02,
                0x30,
                0x03,
                0x40,
                0x04,
                0x50,
                0x05,
                0x21,
                0x12,
                0x31,
                0x13,
                0x41,
                0x14,
                0x51,
                0x15,
                0x42,
                0x24,
                0x52,
                0x25,
                0x53,
                0x35,
                0x54,
                0x45
            };

            for (i = 0; i < ARRAYSIZE(chan_id); i++) {
                for (j = 0; j < ARRAYSIZE(ctl_sb); j++) {
                    chanspec = WL_CHANSPEC_BAND_5G;
                    chanspec |= WL_CHANSPEC_BW_8080 | ctl_sb[j] | chan_id[i];
                    if (wlc_valid_chanspec_db(wlc->cmi, chanspec)) {
                        list->element[list->count] = chanspec;
                        list->count++;
                    }
                }
            }
        } else if (bu_req == BAND_6G_INDEX) {
            /* TODO:6GHZ determine 80+80 chanspecs in 6G */
        }
    }
#ifdef WL11AC_160
    /* Go through 5G 160MHZ chanspecs only if VHT mode and PHY is capable of 160MHZ  */
    if (WL_BW_CAP_160MHZ(bwmask) &&
        (VHT_ENAB_BAND(wlc->pub, bandtype) || HE_ENAB_BAND(wlc->pub, bandtype))) {
        static const uint16 ctl_sb[] = {
            WL_CHANSPEC_CTL_SB_LLL,
            WL_CHANSPEC_CTL_SB_LLU,
            WL_CHANSPEC_CTL_SB_LUL,
            WL_CHANSPEC_CTL_SB_LUU,
            WL_CHANSPEC_CTL_SB_ULL,
            WL_CHANSPEC_CTL_SB_ULU,
            WL_CHANSPEC_CTL_SB_UUL,
            WL_CHANSPEC_CTL_SB_UUU
        };
        if (bu_req == BAND_5G_INDEX) {
            static const uint8 center_chan_5g_160[] = {50, 114, 163};
            uint i;
            int j;

            for (i = 0; i < ARRAYSIZE(center_chan_5g_160); i++) {
                for (j = 0; j < ARRAYSIZE(ctl_sb); j++) {
                    chanspec = wlc_bandunit2chspecband(bu_req);
                    chanspec |= WL_CHANSPEC_BW_160 | ctl_sb[j];
                    chanspec |= center_chan_5g_160[i];
                    if (wlc_valid_chanspec_db(wlc->cmi, chanspec)) {
                        list->element[list->count] = chanspec;
                        list->count++;
                    }
                }
            }
        } else if (bu_req == BAND_6G_INDEX) {
            chanspec = wlc_bandunit2chspecband(bu_req);
            chanspec |= WL_CHANSPEC_BW_160;
            wlc_chanspec_list_ordered(wlc_cmi, list, chanspec);
        }
    }
#endif /* WL11AC_160 */
#endif /* WL11AC */

    /* restore bw_cap */
    FOREACH_WLC_BAND(wlc, bandunit) {
        wlc->bandstate[bandunit]->bw_cap = saved_bwcap[bandunit];

        if (result != CLM_RESULT_OK)
            continue;

        wlc_cmi->bandstate[bandunit].locale_flags = saved_locale_flags[bandunit];
        bcopy(&saved_valid_channels[bandunit],
            &wlc_cmi->bandstate[bandunit].valid_channels,
            sizeof(chanvec_t));
    }
    /* bandlock only need to be restored when locale changed */
    if (result == CLM_RESULT_OK)
        wlc->bandlocked = bandlocked;
}

/*
 * API identifies passed chanspec present in the passed country.
 * Returns TRUE if chanspec queried to country match else FALSE
 * Caller should ensure to pass valid chanspec and country present in the CLM data
 */
bool
wlc_valid_chanspec_cntry(wlc_cm_info_t *wlc_cmi, const char *country_abbrev,
        chanspec_t home_chanspec)
{
    uint i;
    wl_uint32_list_t *list;
    wlc_info_t *wlc = wlc_cmi->wlc;
    char abbrev[WLC_CNTRY_BUF_SZ];
    chanspec_t chanspec = home_chanspec;
    bool found_chanspec = FALSE;
    uint bw = WL_CHANSPEC_BW_20;

    bzero(abbrev, WLC_CNTRY_BUF_SZ);
    list = (wl_uint32_list_t *)MALLOCZ(wlc->osh, (WL_NUMCHANSPECS+1) * sizeof(uint32));

    if (!list) {
        WL_ERROR(("wl%d: %s: out of mem, %d bytes malloced\n", wlc->pub->unit,
                  __FUNCTION__, MALLOCED(wlc->osh)));
        return FALSE;
    }

    list->count = 0;

    strncpy(abbrev, country_abbrev, WLC_CNTRY_BUF_SZ - 1);
    abbrev[WLC_CNTRY_BUF_SZ - 1] = '\0';

    /* chanspec is valid during associated case only. */
    if (CHSPEC_IS20(chanspec) || (chanspec == 0)) {
        bw = WL_CHANSPEC_BW_20;
    } else if (CHSPEC_IS40(chanspec)) {
        bw = WL_CHANSPEC_BW_40;
    } else if (CHSPEC_IS80(chanspec)) {
        bw = WL_CHANSPEC_BW_80;
    }

    wlc_get_valid_chanspecs(wlc->cmi, list,
        chanspec ? CHSPEC_BANDUNIT(chanspec) : wlc->band->bandunit,
        wlc_chspec_bw2bwcap_bit(bw), abbrev);

    for (i = 0; i < list->count; i++) {
        chanspec_t chanspec_e = (chanspec_t) list->element[i];

        if (chanspec == chanspec_e) {
            found_chanspec = TRUE;
            WL_ERROR(("wl%d: %s: found chanspec 0x%04x\n",
                       wlc->pub->unit, __FUNCTION__, chanspec_e));
            break;
        }
    }

    if (list)
        MFREE(wlc->osh, list, (WL_NUMCHANSPECS+1) * sizeof(uint32));

    return found_chanspec;
}

/* Return true if the channel is a valid channel that is radar sensitive
 * in the current country/locale
 */
bool
wlc_radar_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
#if BAND5G /* RADAR */
    const chanvec_t *radar_channels;
    uint8 channel;
    uint8 last_channel;

    /* The radar_channels chanvec may be a superset of valid channels,
     * so be sure to check for a valid channel first.
     */
    if (!chspec || !wlc_valid_chanspec_db(wlc_cmi, chspec) || !CHSPEC_IS5G(chspec)) {
        return FALSE;
    }

    radar_channels = wlc_cmi->bandstate[BAND_5G_INDEX].radar_channels;

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        if (isset(radar_channels->vec, channel)) {
            return TRUE;
        }
    }

#endif    /* BAND5G */
    return FALSE;
}

/* Return true if the channel is a valid channel that is radar sensitive
 * in the current country/locale
 */
bool
wlc_restricted_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    chanvec_t *restricted_channels;
    uint8 channel;
    uint8 last_channel;

    /* The restriced_channels chanvec may be a superset of valid channels,
     * so be sure to check for a valid channel first.
     */

    if (!chspec || !wlc_valid_chanspec_db(wlc_cmi, chspec)) {
        return FALSE;
    }

    restricted_channels = &wlc_cmi->restricted_channels;

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        if (isset(restricted_channels->vec, channel)) {
            return TRUE;
        }
    }

    return FALSE;
}

static bool
is_clm_restricted_chanspec(chanspec_t chspec, chanvec_t *restricted_chanvec)
{
    uint8 lower_sb, upper_sb;
    uint8 channel = CHSPEC_CHANNEL(chspec);
    if (CHSPEC_IS20(chspec)) {
        lower_sb = upper_sb = channel;
    } else if (CHSPEC_IS40(chspec)) {
        lower_sb = LOWER_20_SB(channel);
        upper_sb = UPPER_20_SB(channel);
    } else if (CHSPEC_IS80(chspec)) {
        lower_sb = LOWER_20_SB(LOWER_40_SB(channel));
        upper_sb = UPPER_20_SB(UPPER_40_SB(channel));
    } else {
        return FALSE;
    }
    for (; lower_sb <= upper_sb; lower_sb += CH_20MHZ_APART) {
        if (isset(restricted_chanvec->vec, lower_sb)) {
            return TRUE;
        }
    }
    return FALSE;
}

bool
wlc_channel_clm_restricted_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{

    /* 6GHz does not have restricted channels */
    if (!chspec || !wlc_valid_chanspec_db(wlc_cmi, chspec) || CHSPEC_IS6G(chspec)) {
        return FALSE;
    }

    if (CHSPEC_IS8080(chspec) || CHSPEC_IS160(chspec)) {
        return is_clm_restricted_chanspec(wf_chspec_primary80_channel(chspec),
            &wlc_cmi->restricted_channels) ||
            is_clm_restricted_chanspec(wf_chspec_secondary80_channel(chspec),
            &wlc_cmi->restricted_channels);
    }
    return is_clm_restricted_chanspec(chspec, &wlc_cmi->restricted_channels);
}

void
wlc_clr_restricted_chanspec(wlc_cm_info_t *wlc_cmi, chanspec_t chspec)
{
    uint8 channel;
    uint8 last_channel;

    FOREACH_20_SB_EFF(chspec, channel, last_channel) {
        clrbit(wlc_cmi->restricted_channels.vec, channel);
    }
    wlc_upd_restricted_chanspec_flag(wlc_cmi);
}

static void
wlc_upd_restricted_chanspec_flag(wlc_cm_info_t *wlc_cmi)
{
    uint j;

    for (j = 0; j < (int)sizeof(chanvec_t); j++)
        if (wlc_cmi->restricted_channels.vec[j]) {
            wlc_cmi->has_restricted_ch = TRUE;
            return;
        }

    wlc_cmi->has_restricted_ch = FALSE;
}

bool
wlc_has_restricted_chanspec(wlc_cm_info_t *wlc_cmi)
{
    return wlc_cmi->has_restricted_ch;
}

#if defined(BCMDBG)

const bcm_bit_desc_t fc_flags[] = {
    {WLC_EIRP, "EIRP"},
    {WLC_DFS_TPC, "DFS/TPC"},
    {WLC_NO_80MHZ, "No 80MHz"},
    {WLC_NO_40MHZ, "No 40MHz"},
    {WLC_NO_MIMO, "No MIMO"},
    {WLC_RADAR_TYPE_EU, "EU_RADAR"},
    {WLC_TXBF, "TxBF"},
    {WLC_FILT_WAR, "FILT_WAR"},
    {WLC_NO_160MHZ, "No 160Mhz"},
    {WLC_EDCRS_EU, "EDCRS_EU"},
    {WLC_LO_GAIN_NBCAL, "LO_GAIN"},
    {WLC_RADAR_TYPE_UK, "UK_RADAR"},
    {WLC_RADAR_TYPE_JP, "JP_RADAR"},
    {0, NULL}
};

/** Dumps the locale for one caller supplied chanspec */
static void
wlc_channel_dump_locale_chspec(wlc_info_t *wlc, struct bcmstrbuf *b, chanspec_t chanspec,
                               ppr_t *txpwr)
{
    int max_cck, max_ofdm;
    int restricted;
    int radar = 0;
    int quiet;
    ppr_dsss_rateset_t dsss_limits;
    ppr_ofdm_rateset_t ofdm_limits;
    ppr_ht_mcs_rateset_t mcs_limits;
    char max_ofdm_str[32];
    char max_ht20_str[32];
    char max_ht40_str[32];
    char max_cck_str[32];
    int max_ht20 = 0, max_ht40 = 0;
    uint chan = CHSPEC_CHANNEL(chanspec);
    uint16 band = CHSPEC_BAND(chanspec);

    if (!wlc_valid_chanspec_db(wlc->cmi, chanspec)) {
        chanspec = CH40MHZ_CHSPEC(band, chan, WL_CHANSPEC_CTL_SB_LOWER);
        if (!wlc_valid_chanspec_db(wlc->cmi, chanspec)) {
            chanspec = CH80MHZ_CHSPEC(band, chan, WL_CHANSPEC_CTL_SB_LOWER);
            if (!wlc_valid_chanspec_db(wlc->cmi, chanspec))
                return;
        }
    }

    radar = wlc_radar_chanspec(wlc->cmi, chanspec);
    restricted = wlc_restricted_chanspec(wlc->cmi, chanspec);
    quiet = wlc_quiet_chanspec(wlc->cmi, chanspec);

    wlc_channel_reg_limits(wlc->cmi, chanspec, txpwr, NULL);

    ppr_get_dsss(txpwr, WL_TX_BW_20, WL_TX_CHAINS_1, &dsss_limits);
    max_cck = dsss_limits.pwr[0];

    ppr_get_ofdm(txpwr, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1, &ofdm_limits);
    max_ofdm = ofdm_limits.pwr[0];

    if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
        ppr_get_ht_mcs(txpwr, WL_TX_BW_20, WL_TX_NSS_1, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2, &mcs_limits);
        max_ht20 = mcs_limits.pwr[0];

        ppr_get_ht_mcs(txpwr, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_CDD,
            WL_TX_CHAINS_2, &mcs_limits);
        max_ht40 = mcs_limits.pwr[0];
    }

    if (CHSPEC_IS2G(chanspec))
        if (max_cck != WL_RATE_DISABLED)
            snprintf(max_cck_str, sizeof(max_cck_str), "%2d%s", QDB_FRAC(max_cck));
        else
            strncpy(max_cck_str, "-    ", sizeof(max_cck_str));

    else
        strncpy(max_cck_str, "     ", sizeof(max_cck_str));

    if (max_ofdm != WL_RATE_DISABLED)
        snprintf(max_ofdm_str, sizeof(max_ofdm_str),
         "%2d%s", QDB_FRAC(max_ofdm));
    else
        strncpy(max_ofdm_str, "-    ", sizeof(max_ofdm_str));

    if (N_ENAB(wlc->pub)) {

        if (max_ht20 != WL_RATE_DISABLED)
            snprintf(max_ht20_str, sizeof(max_ht20_str),
             "%2d%s", QDB_FRAC(max_ht20));
        else
            strncpy(max_ht20_str, "-    ", sizeof(max_ht20_str));

        if (max_ht40 != WL_RATE_DISABLED)
            snprintf(max_ht40_str, sizeof(max_ht40_str),
             "%2d%s", QDB_FRAC(max_ht40));
        else
            strncpy(max_ht40_str, "-    ", sizeof(max_ht40_str));

        bcm_bprintf(b, "%s%3u %s%s%s     %s %s  %s %s\n",
                    (CHSPEC_IS40(chanspec)?">":" "), chan,
                    (radar ? "R" : "-"), (restricted ? "S" : "-"),
                    (quiet ? "Q" : "-"),
                    max_cck_str, max_ofdm_str,
                    max_ht20_str, max_ht40_str);
    } else {
        bcm_bprintf(b, "%s%3u %s%s%s     %s %s\n",
                    (CHSPEC_IS40(chanspec)?">":" "), chan,
                    (radar ? "R" : "-"), (restricted ? "S" : "-"),
                    (quiet ? "Q" : "-"),
                    max_cck_str, max_ofdm_str);
    }
} /* wlc_channel_dump_locale_chspec */

/* FTODO need to add 80mhz to this function */
static int
wlc_channel_dump_locale(void *handle, struct bcmstrbuf *b)
{
    wlc_info_t *wlc = (wlc_info_t*)handle;
    wlc_cm_info_t* wlc_cmi = wlc->cmi;
    int i;
    uint chan;
    char flagstr[64];
    chanspec_t chanspec;
    clm_country_locales_t locales;
    clm_country_t country;
    enum wlc_bandunit bandunit;
    uint16 flags;
    ppr_t *txpwr;

    clm_result_t result = wlc_country_lookup_direct(wlc_cmi->ccode,
        wlc_cmi->regrev, &country);

    if (result != CLM_RESULT_OK) {
        return -1;
    }

    if ((txpwr = ppr_create(wlc->pub->osh, ppr_get_max_bw())) == NULL) {
        return BCME_ERROR;
    }

    bcm_bprintf(b, "srom_ccode \"%s\" srom_regrev %u\n",
                wlc_cmi->srom_ccode, wlc_cmi->srom_regrev);

    result = wlc_get_locale(country, &locales);
    if (result != CLM_RESULT_OK) {
        ppr_delete(wlc->pub->osh, txpwr);
        return -1;
    }

    FOREACH_WLC_BAND(wlc, bandunit) {
        wlc_get_flags(&locales, wlc_bandunit2clmband(bandunit), &flags);
        bcm_format_flags(fc_flags, flags, flagstr, 64);
        bcm_bprintf(b, "%s Flags: %s\n", wlc_bandunit_name(bandunit), flagstr);
    }

    if (N_ENAB(wlc->pub))
        bcm_bprintf(b, "  Ch Rdr/reS DSSS  OFDM   HT    20/40\n");
    else
        bcm_bprintf(b, "  Ch Rdr/reS DSSS  OFDM\n");

    FOREACH_WLC_BAND_UNORDERED(wlc, bandunit) {
        wlcband_t *band = wlc->bandstate[bandunit];
        FOREACH_WLC_BAND_CHANNEL20(band, chan) {
            chanspec = CH20MHZ_CHSPEC(chan, BANDTYPE_CHSPEC(band->bandtype));
            wlc_channel_dump_locale_chspec(wlc, b, chanspec, txpwr);
        }
    }

    bcm_bprintf(b, "supported regulatory class (%d%s):\n",
        wlc->cmi->cur_rclass_type, wlc->cmi->use_global ? "/GBL" : "");
    for (i = 0; i <= MAXREGCLASS; i++) {
        if (isset(wlc->cmi->valid_rcvec.vec, i))
            bcm_bprintf(b, "%d ", i);
    }
    bcm_bprintf(b, "\n");

    bcm_bprintf(b, "has_restricted_ch %s\n", wlc_cmi->has_restricted_ch ? "TRUE" : "FALSE");

    ppr_delete(wlc->pub->osh, txpwr);
    return 0;
}
#endif

static void
wlc_channel_margin_summary_mapfn(void *context, uint8 *a, uint8 *b)
{
    uint8 margin;
    uint8 *pmin = (uint8*)context;

    if (*a > *b)
        margin = *a - *b;
    else
        margin = 0;

    *pmin = MIN(*pmin, margin);
}

/* Map the given function with its context value over the power targets
 * appropriate for the given band and bandwidth in two txppr structs.
 * If the band is 2G, DSSS/CCK rates will be included.
 * If the bandwidth is 20MHz, only 20MHz targets are included.
 * If the bandwidth is 40MHz, both 40MHz and 20in40 targets are included.
 */
static void
wlc_channel_map_txppr_binary(ppr_mapfn_t fn, void* context, uint bandtype, uint bw,
    ppr_t *a, ppr_t *b, wlc_info_t *wlc)
{
    if (bw == WL_CHANSPEC_BW_20) {
        if (bandtype == WL_CHANSPEC_BAND_2G)
            ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20, WL_TX_CHAINS_1);
        ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_NONE, WL_TX_CHAINS_1);
    }

    /* map over 20MHz rates for 20MHz channels */
    if (bw == WL_CHANSPEC_BW_20) {
        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            if (bandtype == WL_CHANSPEC_BAND_2G) {
                ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20, WL_TX_CHAINS_2);
                if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                    ppr_map_vec_dsss(fn, context, a, b,
                        WL_TX_BW_20, WL_TX_CHAINS_3);
                }
            }
        }
        ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
            WL_TX_MODE_NONE, WL_TX_CHAINS_1);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
                WL_TX_MODE_CDD, WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
                WL_TX_MODE_STBC, WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
                WL_TX_MODE_NONE, WL_TX_CHAINS_2);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3);
                ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3);

                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);

                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_4);
                    ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_4);
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20,
                        WL_TX_NSS_4, WL_TX_MODE_NONE, WL_TX_CHAINS_4);
                }
            }
        }
    } else
    /* map over 40MHz and 20in40 rates for 40MHz channels */
    {
        ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_NONE, WL_TX_CHAINS_1);
        ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1,
                WL_TX_MODE_CDD, WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
                WL_TX_MODE_STBC, WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
                WL_TX_MODE_NONE, WL_TX_CHAINS_2);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3);

                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);

                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4);
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_4);
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_40,
                        WL_TX_NSS_4, WL_TX_MODE_NONE, WL_TX_CHAINS_4);
                }
            }
        }
        /* 20in40 legacy */
        if (bandtype == WL_CHANSPEC_BAND_2G) {
            ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_CHAINS_1);
            if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
                ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40,
                    WL_TX_CHAINS_2);
                if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                    ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_CHAINS_3);
                    if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                        ppr_map_vec_dsss(fn, context, a, b, WL_TX_BW_20IN40,
                            WL_TX_CHAINS_4);
                    }
                }
            }
        }

        ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1);

        /* 20in40 HT */
        ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1, WL_TX_MODE_NONE,
            WL_TX_CHAINS_1);

        if (PHYCORENUM(wlc->stf->op_txstreams) > 1) {
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1,
                WL_TX_MODE_CDD,    WL_TX_CHAINS_2);
            ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
                WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
                WL_TX_MODE_STBC, WL_TX_CHAINS_2);
            ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
                WL_TX_MODE_NONE, WL_TX_CHAINS_2);

            if (PHYCORENUM(wlc->stf->op_txstreams) > 2) {
                ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_MODE_CDD,
                    WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_1,
                    WL_TX_MODE_CDD, WL_TX_CHAINS_3);

                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
                    WL_TX_MODE_STBC, WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_2,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);
                ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40, WL_TX_NSS_3,
                    WL_TX_MODE_NONE, WL_TX_CHAINS_3);

                if (PHYCORENUM(wlc->stf->op_txstreams) > 3) {
                    ppr_map_vec_ofdm(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_MODE_CDD, WL_TX_CHAINS_4);
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_NSS_1, WL_TX_MODE_CDD, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_NSS_2, WL_TX_MODE_STBC, WL_TX_CHAINS_4);
                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_NSS_2, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_NSS_3, WL_TX_MODE_NONE, WL_TX_CHAINS_4);

                    ppr_map_vec_ht_mcs(fn, context, a, b, WL_TX_BW_20IN40,
                        WL_TX_NSS_4, WL_TX_MODE_NONE, WL_TX_CHAINS_4);
                }
            }
        }
    }
}

/* calculate the offset from each per-rate power target in txpwr to the supplied
 * limit (or zero if txpwr[x] is less than limit[x]), and return the smallest
 * offset of relevant rates for bandtype/bw.
 */
static uint8
wlc_channel_txpwr_margin(ppr_t *txpwr, ppr_t *limit, uint bandtype, uint bw, wlc_info_t *wlc)
{
    uint8 margin = 0xff;

    wlc_channel_map_txppr_binary(wlc_channel_margin_summary_mapfn, &margin,
                                 bandtype, bw, txpwr, limit, wlc);
    return margin;
}

/* return a ppr_t struct with the phy srom limits for the given channel */
static void
wlc_channel_srom_limits(wlc_cm_info_t *wlc_cmi, chanspec_t chanspec,
    ppr_t *srommin, ppr_t *srommax)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    uint8 min_srom;

    if (srommin != NULL)
        ppr_clear(srommin);
    if (srommax != NULL)
        ppr_clear(srommax);

    if (!PHYTYPE_HT_CAP(wlc_cmi->wlc->band))
        return;

    wlc_phy_txpower_sromlimit(WLC_PI(wlc), chanspec, &min_srom, srommax, 0);
    if (srommin != NULL)
        ppr_set_cmn_val(srommin, min_srom);
}

/* Set a per-chain power limit for the given band
 * Per-chain offsets will be used to make sure the max target power does not exceed
 * the per-chain power limit
 */
static int
wlc_channel_band_chain_limit(wlc_cm_info_t *wlc_cmi, uint bandtype,
                             struct wlc_channel_txchain_limits *lim)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    int ppr_buf_i;
    ppr_t* txpwr = NULL;
    enum wlc_bandunit bandunit = wlc_bandtype2bandunit(bandtype);

    if (!PHYTYPE_HT_CAP(wlc_cmi->wlc->band))
        return BCME_UNSUPPORTED;

    wlc_cmi->bandstate[bandunit].chain_limits = *lim;

    if (CHSPEC_BANDUNIT(wlc->chanspec) != bandunit)
        return 0;

#if defined(WLTXPWR_CACHE)
    wlc_phy_txpwr_cache_invalidate(phy_tpc_get_txpwr_cache(WLC_PI(wlc)));
#endif

    ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cmi, &txpwr,
        PPR_CHSPEC_BW(wlc->chanspec));
    if (ppr_buf_i < 0)
        return BCME_NOMEM;

    /* update the current tx chain offset if we just updated this band's limits */
    wlc_channel_txpower_limits(wlc_cmi, txpwr);
    wlc_channel_update_txchain_offsets(wlc_cmi, txpwr);
    wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);

    return 0;
}

/* update the per-chain tx power offset given the current power targets to implement
 * the correct per-chain tx power limit
 */
static int
wlc_channel_update_txchain_offsets(wlc_cm_info_t *wlc_cmi, ppr_t *txpwr)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    struct wlc_channel_txchain_limits *lim;
    wl_txchain_pwr_offsets_t offsets;
    chanspec_t chanspec;
    int i, err;
    int max_pwr;
    int band, bw;
    int limits_present = FALSE;
    uint8 delta, margin, err_margin;
    wl_txchain_pwr_offsets_t cur_offsets;
#ifdef BCMDBG
    char chanbuf[CHANSPEC_STR_LEN];
#endif
#if defined WLTXPWR_CACHE
    tx_pwr_cache_entry_t* cacheptr = phy_tpc_get_txpwr_cache(WLC_PI(wlc));
#endif
    int ppr_buf_i;
    ppr_t* srompwr = NULL;

    if (!PHYTYPE_HT_CAP(wlc->band)) {
        return BCME_UNSUPPORTED;
    }

    chanspec = wlc->chanspec;

#if defined WLTXPWR_CACHE
    if ((wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) == TRUE) &&
        (wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, 0) != WL_RATE_DISABLED)) {

        for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
            offsets.offset[i] =
                wlc_phy_get_cached_txchain_offsets(cacheptr, chanspec, i);
        }

    /* always set, at least for the moment */
        err = wlc_stf_txchain_pwr_offset_set(wlc, &offsets);

        return err;
    }
#endif /* WLTXPWR_CACHE */

    band = CHSPEC_BAND(chanspec);
    bw = CHSPEC_BW(chanspec);

    ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc_cmi, &srompwr,
        PPR_CHSPEC_BW(chanspec));
    if (ppr_buf_i < 0)
        return BCME_NOMEM;
    /* initialize the offsets to a default of no offset */
    memset(&offsets, 0, sizeof(wl_txchain_pwr_offsets_t));

    lim = &wlc_cmi->bandstate[CHSPEC_BANDUNIT(chanspec)].chain_limits;

    /* see if there are any chain limits specified */
    for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
        if (lim->chain_limit[i] < WLC_TXPWR_MAX) {
            limits_present = TRUE;
            break;
        }
    }

    /* if there are no limits, we do not need to do any calculations */
    if (limits_present) {

#ifdef WLTXPWR_CACHE
        ASSERT(txpwr != NULL);
#endif
        /* find the max power target for this channel and impose
         * a txpwr delta per chain to meet the specified chain limits
         * Bound the delta by the tx power margin
         */

        /* get the srom min powers */
        wlc_channel_srom_limits(wlc_cmi, wlc->chanspec, srompwr, NULL);

        /* find the dB margin we can use to adjust tx power */
        margin = wlc_channel_txpwr_margin(txpwr, srompwr, band, bw, wlc);

        /* reduce the margin by the error margin 1.5dB backoff */
        err_margin = 6;    /* 1.5 dB in qdBm */
        margin = (margin >= err_margin) ? margin - err_margin : 0;

        /* get the srom max powers */
        wlc_channel_srom_limits(wlc_cmi, wlc->chanspec, NULL, srompwr);

        /* combine the srom limits with the given regulatory limits
         * to find the actual channel max
         */
        /* wlc_channel_txpwr_vec_combine_min(srompwr, txpwr); */
        ppr_apply_vector_ceiling(srompwr, txpwr);

        /* max_pwr = (int)wlc_channel_txpwr_max(srompwr, band, bw, wlc); */
        max_pwr = (int)ppr_get_max_for_bw(srompwr, PPR_CHSPEC_BW(chanspec));
        WL_NONE(("wl%d: %s: channel %s max_pwr %d margin %d\n",
                 wlc->pub->unit, __FUNCTION__,
                 wf_chspec_ntoa(wlc->chanspec, chanbuf), max_pwr, margin));

        /* for each chain, calculate an offset that keeps the max tx power target
         * no greater than the chain limit
         */
        for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
            WL_NONE(("wl%d: %s: chain_limit[%d] %d",
                     wlc->pub->unit, __FUNCTION__,
                     i, lim->chain_limit[i]));
            if (lim->chain_limit[i] < max_pwr) {
                delta = max_pwr - lim->chain_limit[i];

                WL_NONE((" desired delta -%u lim delta -%u",
                         delta, MIN(delta, margin)));

                /* limit to the margin allowed for our adjustmets */
                delta = MIN(delta, margin);

                offsets.offset[i] = -delta;
            }
            WL_NONE(("\n"));
        }
    } else {
        WL_NONE(("wl%d: %s skipping limit calculation since limits are MAX\n",
                 wlc->pub->unit, __FUNCTION__));
    }

#if defined WLTXPWR_CACHE
    if (wlc_phy_txpwr_cache_is_cached(cacheptr, chanspec) == TRUE) {
        for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
            wlc_phy_set_cached_txchain_offsets(cacheptr, chanspec, i,
                offsets.offset[i]);
        }
    }
#endif
    err = wlc_iovar_op(wlc, "txchain_pwr_offset", NULL, 0,
                       &cur_offsets, sizeof(wl_txchain_pwr_offsets_t), IOV_GET, NULL);

    if (!err && bcmp(&cur_offsets.offset, &offsets.offset, WL_NUM_TXCHAIN_MAX)) {

        err = wlc_iovar_op(wlc, "txchain_pwr_offset", NULL, 0,
            &offsets, sizeof(wl_txchain_pwr_offsets_t), IOV_SET, NULL);
    }

    if (err) {
        WL_ERROR(("wl%d: txchain_pwr_offset failed: error %d\n",
            WLCWLUNIT(wlc), err));
    }

    wlc_channel_release_prealloc_ppr_buf(wlc_cmi, ppr_buf_i);

    return err;
}

#if defined(BCMDBG)
static int
wlc_channel_dump_reg_ppr(void *handle, struct bcmstrbuf *b)
{
    wlc_cm_info_t *wlc_cmi = (wlc_cm_info_t*)handle;
    wlc_info_t *wlc = wlc_cmi->wlc;
    ppr_t* txpwr;
    char chanbuf[CHANSPEC_STR_LEN];
    int ant_gain;
    int sar;

    wlcband_t* band = wlc->band;

    ant_gain = band->antgain;
    sar = band->sar;

    if ((txpwr = ppr_create(wlc_cmi->pub->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
        return BCME_ERROR;
    }
    wlc_channel_reg_limits(wlc_cmi, wlc->chanspec, txpwr, NULL);

    bcm_bprintf(b, "Regulatory Limits for channel %s (SAR:",
        wf_chspec_ntoa(wlc->chanspec, chanbuf));
    if (sar == WLC_TXPWR_MAX)
        bcm_bprintf(b, " -  ");
    else
        bcm_bprintf(b, "%2d%s", QDB_FRAC_TRUNC(sar));
    bcm_bprintf(b, " AntGain: %2d%s)\n", QDB_FRAC_TRUNC(ant_gain));
    wlc_channel_dump_txppr(b, txpwr, CHSPEC_TO_TX_BW(wlc->chanspec), wlc);

    ppr_delete(wlc_cmi->pub->osh, txpwr);
    return 0;
}

/* dump of regulatory power with local constraint factored in for the current channel */
static int
wlc_channel_dump_reg_local_ppr(void *handle, struct bcmstrbuf *b)
{
    wlc_cm_info_t *wlc_cmi = (wlc_cm_info_t*)handle;
    wlc_info_t *wlc = wlc_cmi->wlc;
    ppr_t* txpwr;
    char chanbuf[CHANSPEC_STR_LEN];

    if ((txpwr = ppr_create(wlc_cmi->pub->osh, PPR_CHSPEC_BW(wlc->chanspec))) == NULL) {
        return BCME_ERROR;
    }

    wlc_channel_txpower_limits(wlc_cmi, txpwr);

    bcm_bprintf(b, "Regulatory Limits with constraint for channel %s\n",
                wf_chspec_ntoa(wlc->chanspec, chanbuf));
    wlc_channel_dump_txppr(b, txpwr, CHSPEC_TO_TX_BW(wlc->chanspec), wlc);

    ppr_delete(wlc_cmi->pub->osh, txpwr);
    return 0;
}

/* dump of srom per-rate max/min values for the current channel */
static int
wlc_channel_dump_srom_ppr(void *handle, struct bcmstrbuf *b)
{
    wlc_cm_info_t *wlc_cmi = (wlc_cm_info_t*)handle;
    wlc_info_t *wlc = wlc_cmi->wlc;
    ppr_t* srompwr;
    char chanbuf[CHANSPEC_STR_LEN];

    if ((srompwr = ppr_create(wlc_cmi->pub->osh, ppr_get_max_bw())) == NULL) {
        return BCME_ERROR;
    }

    wlc_channel_srom_limits(wlc_cmi, wlc->chanspec, NULL, srompwr);

    bcm_bprintf(b, "PHY/SROM Max Limits for channel %s\n",
                wf_chspec_ntoa(wlc->chanspec, chanbuf));
    wlc_channel_dump_txppr(b, srompwr, CHSPEC_TO_TX_BW(wlc->chanspec), wlc);

    wlc_channel_srom_limits(wlc_cmi, wlc->chanspec, srompwr, NULL);

    bcm_bprintf(b, "PHY/SROM Min Limits for channel %s\n",
                wf_chspec_ntoa(wlc->chanspec, chanbuf));
    wlc_channel_dump_txppr(b, srompwr, CHSPEC_TO_TX_BW(wlc->chanspec), wlc);

    ppr_delete(wlc_cmi->pub->osh, srompwr);
    return 0;
}

static void
wlc_channel_margin_calc_mapfn(void *ignore, uint8 *a, uint8 *b)
{
    BCM_REFERENCE(ignore);

    if (*a > *b)
        *a = *a - *b;
    else
        *a = 0;
}

/* dumps dB margin between a rate an the lowest allowable power target, and
 * summarize the min of the margins for the current channel
 */
static int
wlc_channel_dump_margin(void *handle, struct bcmstrbuf *b)
{
    wlc_cm_info_t *wlc_cmi = (wlc_cm_info_t*)handle;
    wlc_info_t *wlc = wlc_cmi->wlc;
    ppr_t* txpwr;
    ppr_t* srommin;
    chanspec_t chanspec;
    int band, bw;
    uint8 margin;
    char chanbuf[CHANSPEC_STR_LEN];

    chanspec = wlc->chanspec;
    band = CHSPEC_BAND(chanspec);
    bw = CHSPEC_BW(chanspec);

    if ((txpwr = ppr_create(wlc_cmi->pub->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
        return 0;
    }
    if ((srommin = ppr_create(wlc_cmi->pub->osh, PPR_CHSPEC_BW(chanspec))) == NULL) {
        ppr_delete(wlc_cmi->pub->osh, txpwr);
        return 0;
    }

    wlc_channel_txpower_limits(wlc_cmi, txpwr);

    /* get the srom min powers */
    wlc_channel_srom_limits(wlc_cmi, wlc->chanspec, srommin, NULL);

    /* find the dB margin we can use to adjust tx power */
    margin = wlc_channel_txpwr_margin(txpwr, srommin, band, bw, wlc);

    /* calulate the per-rate margins */
    wlc_channel_map_txppr_binary(wlc_channel_margin_calc_mapfn, NULL,
                                 band, bw, txpwr, srommin, wlc);

    bcm_bprintf(b, "Power margin for channel %s, min = %u\n",
                wf_chspec_ntoa(wlc->chanspec, chanbuf), margin);
    wlc_channel_dump_txppr(b, txpwr, CHSPEC_TO_TX_BW(wlc->chanspec), wlc);

    ppr_delete(wlc_cmi->pub->osh, srommin);
    ppr_delete(wlc_cmi->pub->osh, txpwr);
    return 0;
}

static int
wlc_channel_supported_country_regrevs(void *handle, struct bcmstrbuf *b)
{
    int iter;
    ccode_t cc;
    unsigned int regrev;

    BCM_REFERENCE(handle);

    if (clm_iter_init(&iter) == CLM_RESULT_OK) {
        while (clm_country_iter((clm_country_t*)&iter, cc, &regrev) == CLM_RESULT_OK) {
            bcm_bprintf(b, "%c%c/%u\n", cc[0], cc[1], regrev);

        }
    }
    return 0;
}

#endif

void
wlc_dump_clmver(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    const struct clm_data_header* clm_base_headerptr = wlc_cmi->clm_base_dataptr;
    const char* verstrptr;
    const char* useridstrptr;

    if (clm_base_headerptr == NULL)
        clm_base_headerptr = &clm_header;

    bcm_bprintf(b, "API: %d.%d\nData: %s\nCompiler: %s\n%s\n",
        clm_base_headerptr->format_major, clm_base_headerptr->format_minor,
        clm_base_headerptr->clm_version, clm_base_headerptr->compiler_version,
        clm_base_headerptr->generator_version);
    verstrptr = clm_get_base_app_version_string();
    if (verstrptr != NULL)
        bcm_bprintf(b, "Customization: %s\n", verstrptr);
    useridstrptr = clm_get_string(CLM_STRING_TYPE_USER_STRING, CLM_STRING_SOURCE_BASE);
    if (useridstrptr != NULL)
        bcm_bprintf(b, "Creation: %s\n", useridstrptr);
}

void
wlc_channel_update_txpwr_limit(wlc_info_t *wlc)
{
    int ppr_buf_i, ppr_ru_buf_i;
    ppr_t *txpwr = NULL;
    ppr_ru_t *ru_txpwr = NULL;

    ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc->cmi, &txpwr,
        PPR_CHSPEC_BW(wlc->chanspec));
    if (ppr_buf_i < 0)
        return;
    ppr_ru_buf_i = wlc_channel_acquire_ppr_ru_from_prealloc_buf(wlc->cmi, &ru_txpwr);
    if (ppr_ru_buf_i < 0) {
        wlc_channel_release_prealloc_ppr_buf(wlc->cmi, ppr_buf_i);
        return;
    }

    wlc_channel_reg_limits(wlc->cmi, wlc->chanspec, txpwr, NULL);
#ifdef WL11AX
    wlc_phy_set_ru_power_limits(WLC_PI(wlc), ru_txpwr);
#endif /* WL11AX */
    wlc_phy_txpower_limit_set(WLC_PI(wlc), txpwr, wlc->chanspec);

    wlc_channel_release_prealloc_ppr_buf(wlc->cmi, ppr_buf_i);
    wlc_channel_release_prealloc_ppr_ru_buf(wlc->cmi, ppr_ru_buf_i);
}

#if defined(WL11H) || defined(WLTDLS)
static int
wlc_channel_write_supp_opclasses_ie(wlc_cm_info_t *cmi, bcmwifi_rclass_opclass_t cur_opclass,
    uint8 *ie_buf, uint buflen)
{
    bcm_tlv_t *ie = (bcm_tlv_t *)ie_buf;
    uint8 *opclass, *opclasses;
    uint8 i;

    if (buflen <= cmi->valid_rcvec.count)
        return BCME_BUFTOOSHORT;

    ie->id = DOT11_MNG_REGCLASS_ID;

    opclasses = opclass = &ie->data[0];

    *opclass++ = cur_opclass;

    for (i = 0; i <= MAXREGCLASS; i++) {
        if (isset(cmi->valid_rcvec.vec, i)) {
            *opclass++ = i;
        }
    }
    ie->len = (uint8)(opclass - opclasses);

    ASSERT(ie->len == cmi->valid_rcvec.count + 1);
    return BCME_OK;
}
#endif /* WL11H || WLTDLS */

#ifdef WLTDLS
/* Regulatory Class IE in TDLS Setup frames */
static uint
wlc_channel_tdls_calc_rc_ie_len(void *ctx, wlc_iem_calc_data_t *calc)
{
    wlc_cm_info_t *cmi = (wlc_cm_info_t *)ctx;
    wlc_iem_ft_cbparm_t *ftcbparm = calc->cbparm->ft;

    if (!isset(ftcbparm->tdls.cap, DOT11_TDLS_CAP_CH_SW)) {
        return 0;
    }

    return TLV_HDR_LEN + 1 + cmi->valid_rcvec.count;
}

static int
wlc_channel_tdls_write_rc_ie(void *ctx, wlc_iem_build_data_t *build)
{
    wlc_cm_info_t *cmi = (wlc_cm_info_t *)ctx;
    wlc_iem_ft_cbparm_t *ftcbparm = build->cbparm->ft;
    chanspec_t chanspec = ftcbparm->tdls.chspec;
    uint8 opclass;

    if (!isset(ftcbparm->tdls.cap, DOT11_TDLS_CAP_CH_SW)) {
        return BCME_OK;
    }

    opclass = wlc_get_regclass(cmi, chanspec);

    return wlc_channel_write_supp_opclasses_ie(cmi, opclass, build->buf, build->buf_len);
}
#endif /* WLTDLS */

void
wlc_channel_tx_power_target_min_max(struct wlc_info *wlc,
    chanspec_t chanspec, int *min_pwr, int *max_pwr)
{
    int cur_min = 0xFFFF, cur_max = 0;
    int txpwr_ppr_buf_i, srommax_ppr_buf_i;
    ppr_t *txpwr = NULL;
    ppr_t *srommax = NULL;
    int8 min_srom;
    chanspec_t channel = wf_chspec_ctlchan(chanspec) |
        CHSPEC_BAND(chanspec) | WL_CHANSPEC_BW_20;

    txpwr_ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc->cmi, &txpwr,
        PPR_CHSPEC_BW(channel));
    if (txpwr_ppr_buf_i < 0)
        return;

    srommax_ppr_buf_i = wlc_channel_acquire_ppr_from_prealloc_buf(wlc->cmi, &srommax,
        PPR_CHSPEC_BW(channel));
    if (srommax_ppr_buf_i < 0) {
        wlc_channel_release_prealloc_ppr_buf(wlc->cmi, txpwr_ppr_buf_i);
        return;
    }
    /* use the control channel to get the regulatory limits and srom max/min */
    wlc_channel_reg_limits(wlc->cmi, channel, txpwr, NULL);

    wlc_phy_txpower_sromlimit(WLC_PI(wlc), channel, (uint8*)&min_srom, srommax, 0);

    /* bound the regulatory limit by srom min/max */
    ppr_apply_vector_ceiling(txpwr, srommax);
    ppr_apply_min(txpwr, min_srom);

    WL_NONE(("min_srom %d\n", min_srom));

    cur_min = ppr_get_min(txpwr, min_srom);
    cur_max = ppr_get_max(txpwr);

    *min_pwr = (int)cur_min;
    *max_pwr = (int)cur_max;

    wlc_channel_release_prealloc_ppr_buf(wlc->cmi, txpwr_ppr_buf_i);
    wlc_channel_release_prealloc_ppr_buf(wlc->cmi, srommax_ppr_buf_i);
}

int8
wlc_channel_max_tx_power_for_band(struct wlc_info *wlc, int band, int8 *min)
{
    uint chan;
    chanspec_t chspec;
    int chspec_band;
    int8 max = 0, tmpmin = 0x7F;
    int chan_min;
    int chan_max = 0;
    wlc_cm_info_t *cmi = wlc->cmi;

    chspec_band = (band == WLC_BAND_2G) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G;

    for (chan = 0; chan < MAXCHANNEL; chan++) {
        chspec = chan | WL_CHANSPEC_BW_20 | chspec_band;

        if (!wlc_valid_chanspec_db(cmi, chspec))
            continue;

        wlc_channel_tx_power_target_min_max(wlc, chspec, &chan_min, &chan_max);

        max = MAX(chan_max, max);
        tmpmin = MIN(chan_min, tmpmin);
    }
    *min = tmpmin;
    return max;
}

void
wlc_channel_set_tx_power(struct wlc_info *wlc,
    int band, int num_chains, int *txcpd_power_offset, int *tx_chain_offset)
{
    int i;
    int8 band_max, band_min = 0;
    int8 offset;
    struct wlc_channel_txchain_limits lim;
    wlc_cm_info_t *cmi = wlc->cmi;
    bool offset_diff = FALSE;

    /* init limits to max value for int8 to not impose any limit */
    memset(&lim, 0x7f, sizeof(struct wlc_channel_txchain_limits));

    /* find max target power for the band */
    band_max = wlc_channel_max_tx_power_for_band(wlc, band, &band_min);

    for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
        if (i < num_chains) {
            offset =
            (int8) (*(txcpd_power_offset + i) * WLC_TXPWR_DB_FACTOR);
        }
        else
            offset = 0;

        if (offset != 0)
            lim.chain_limit[i] = band_max + offset;

        /* check if the new offsets are equal to the previous offsets */
        if (*(tx_chain_offset + i) != *(txcpd_power_offset + i)) {
            offset_diff = TRUE;
        }
    }

    if (offset_diff == TRUE) {
        wlc_channel_band_chain_limit(cmi, band, &lim);
    }
}

void
wlc_channel_apply_band_chain_limits(struct wlc_info *wlc,
    int band, int8 band_max, int num_chains, int *txcpd_power_offset, int *tx_chain_offset)
{
    int i;
    int8 offset;
    struct wlc_channel_txchain_limits lim;
    wlc_cm_info_t *cmi = wlc->cmi;
    bool offset_diff = FALSE;

    /* init limits to max value for int8 to not impose any limit */
    memset(&lim, 0x7f, sizeof(struct wlc_channel_txchain_limits));

    for (i = 0; i < WLC_CHAN_NUM_TXCHAIN; i++) {
        if (i < num_chains) {
            offset = (int8) (*(txcpd_power_offset + i) * WLC_TXPWR_DB_FACTOR);
        }
        else
            offset = 0;

        if (offset != 0)
            lim.chain_limit[i] = band_max + offset;

        /* check if the new offsets are equal to the previous offsets */
        if (*(tx_chain_offset + i) != *(txcpd_power_offset + i)) {
            offset_diff = TRUE;
        }
    }

    if (offset_diff == TRUE) {
        wlc_channel_band_chain_limit(cmi, band, &lim);
    }
}

#ifdef WLC_TXPWRCAP
static int
wlc_channel_txcap_set_country(wlc_cm_info_t *wlc_cmi)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    int err = BCME_OK;
    BCM_REFERENCE(wlc);
    WL_TXPWRCAP(("%s: country %c%c, channel %d\n", __FUNCTION__,
        wlc_cmi->country_abbrev[0], wlc_cmi->country_abbrev[1],
        wf_chspec_ctlchan(wlc->chanspec)));
    if (wlc_cmi->txcap_download == NULL) {
        WL_TXPWRCAP(("%s: No txcap download installed\n", __FUNCTION__));
        goto exit;
    }
    /* Search in txcap table for the current country and subband to find limits for antennas */
    {
        uint8 *p;
        txcap_file_header_t *txcap_file_header;
        uint8 num_subbands;
        uint8 num_cc_groups;
        uint8 num_antennas;
        txcap_file_cc_group_info_t *cc_group_info;
        uint8 num_cc;
        uint8 *cc = NULL; /* WAR for wrong compiler might not be defined warning */
        int i, j;
        p = (uint8 *)wlc_cmi->txcap_download;
        txcap_file_header = (txcap_file_header_t *)p;
        num_subbands = txcap_file_header->num_subbands;
        num_antennas = 0;
        for (i = 0; i < TXPWRCAP_MAX_NUM_CORES; i++) {
            num_antennas += txcap_file_header->num_antennas_per_core[i];
        }
        num_cc_groups = txcap_file_header->num_cc_groups;
        p += OFFSETOF(txcap_file_header_t, cc_group_info);
        for (i = 0; i < num_cc_groups; i++) {
            cc_group_info = (txcap_file_cc_group_info_t *)p;
            num_cc = cc_group_info->num_cc;
            p += 1;
            WL_TXPWRCAP(("%s: checking cc group %d, num_cc %d\n",
                __FUNCTION__, i, num_cc));
            for (j = 0; j < num_cc; j++) {
                cc = (uint8 *)p;
                p += 2;
                if (!memcmp(cc, "*", 1) ||
                    !memcmp(cc, wlc_cmi->country_abbrev, 2)) {

                    WL_TXPWRCAP(("%s: Matched entry %d in this cc group %d\n",
                        __FUNCTION__, j, i));
                    wlc_cmi->current_country_cc_group_info = cc_group_info;
                    wlc_cmi->current_country_cc_group_info_index = i;
                    wlc_cmi->txcap_download_num_antennas = num_antennas;
                    goto exit;
                }
            }
            if (wlc_cmi->txcap_cap_states_per_cc_group == 2) {
                p += 2*num_subbands*num_antennas;
            } else {
                p += 4*num_subbands*num_antennas;
            }
        }
        WL_TXPWRCAP(("%s: Country not found! Clearing current_country_cc_group_info\n",
            __FUNCTION__));
        wlc_cmi->current_country_cc_group_info = NULL;
    }
exit:
    return err;
}

static int
wlc_channel_txcap_phy_update(wlc_cm_info_t *wlc_cmi,
    wl_txpwrcap_tbl_t *txpwrcap_tbl, int* cellstatus)
{
    wlc_info_t *wlc = wlc_cmi->wlc;
    int err = BCME_OK;
    wl_txpwrcap_tbl_t wl_txpwrcap_tbl;
    uint8 *p;
    int8 *pwrcap_cell_on;
    int8 *pwrcap_cell_off;
    int8 *pwrcap;
    uint8 num_subbands;
    uint8 num_antennas;
    int new_cell_status;
    WL_TXPWRCAP(("%s: country %c%c, channel %d\n", __FUNCTION__,
        wlc_cmi->country_abbrev[0], wlc_cmi->country_abbrev[1],
        wf_chspec_ctlchan(wlc->chanspec)));
    if (wlc_cmi->current_country_cc_group_info) {
        uint8 channel;
        uint8 subband = 0;
        num_subbands =  wlc_cmi->txcap_download->num_subbands,
        num_antennas = wlc_cmi->txcap_download_num_antennas;
        memcpy(wl_txpwrcap_tbl.num_antennas_per_core,
            wlc_cmi->txcap_download->num_antennas_per_core,
            sizeof(wl_txpwrcap_tbl.num_antennas_per_core));
        channel = wf_chspec_ctlchan(wlc->chanspec);
        if (channel <= 14) {
            subband = 0;
        } else if (channel <= 48) {
            subband = 1;
        } else if (channel <= 64) {
            subband = 2;
        } else if (channel <= 144) {
            subband = 3;
        } else {
            subband = 4;
        }
        WL_TXPWRCAP(("Mapping channel %d to subband %d\n", channel, subband));
        p = (uint8 *)wlc_cmi->current_country_cc_group_info;
        p += OFFSETOF(txcap_file_cc_group_info_t, cc_list) +
            2 * wlc_cmi->current_country_cc_group_info->num_cc;
        pwrcap_cell_on = (int8 *)(p + subband*num_antennas);
        p += num_subbands*num_antennas;
        pwrcap_cell_off = (int8 *)(p + subband*num_antennas);
        if (wlc_cmi->txcap_config[subband] == TXPWRCAPCONFIG_WCI2_AND_HOST) {
            int8 *pwrcap_cell_on_2;
            int8 *pwrcap_cell_off_2;
            int ant;
            pwrcap_cell_on_2 = (int8 *)
                (pwrcap_cell_on + 2 * (num_subbands*num_antennas));
            pwrcap_cell_off_2 = (int8 *)
                (pwrcap_cell_off + 2 * (num_subbands*num_antennas));
            if (wlc_cmi->txcap_wci2_cell_status_last & 2) {
                if (wlc_cmi->txcap_high_cap_active &&
                    (wlc_cmi->txcap_state[subband] ==
                    TXPWRCAPSTATE_HOST_LOW_WCI2_HIGH_CAP ||
                    wlc_cmi->txcap_state[subband] ==
                    TXPWRCAPSTATE_HOST_HIGH_WCI2_HIGH_CAP)) {
                    new_cell_status = 0;
                } else {
                    new_cell_status = 1;
                }
            } else if (wlc_cmi->txcap_wci2_cell_status_last & 1) {
                new_cell_status = 1;
            } else {
                new_cell_status = 0;
            }
            /* XXX If no recent host state update then use worst case WCI2 on/off
             * values from head or body.
             */
            if (!wlc_cmi->txcap_high_cap_active) {
                for (ant = 0; ant < num_antennas; ant++) {
                    wl_txpwrcap_tbl.pwrcap_cell_on[ant] =
                    MIN(pwrcap_cell_on[ant], pwrcap_cell_on_2[ant]);
                    wl_txpwrcap_tbl.pwrcap_cell_off[ant] =
                    MIN(pwrcap_cell_off[ant], pwrcap_cell_off_2[ant]);
                }
            } else if (wlc_cmi->txcap_state[subband] ==
                TXPWRCAPSTATE_HOST_LOW_WCI2_LOW_CAP ||
                wlc_cmi->txcap_state[subband] ==
                TXPWRCAPSTATE_HOST_LOW_WCI2_HIGH_CAP) {
                memcpy(&wl_txpwrcap_tbl.pwrcap_cell_on, pwrcap_cell_on,
                    num_antennas);
                memcpy(&wl_txpwrcap_tbl.pwrcap_cell_off, pwrcap_cell_off,
                    num_antennas);
                } else { /* TXPWRCAPSTATE_HOST_HIGH WCI2_LOW_CAP or WCI2_HIGH_CAP */
                    memcpy(&wl_txpwrcap_tbl.pwrcap_cell_on, pwrcap_cell_on_2,
                        num_antennas);
                    memcpy(&wl_txpwrcap_tbl.pwrcap_cell_off, pwrcap_cell_off_2,
                        num_antennas);
                }
            } else
        if (wlc_cmi->txcap_config[subband] == TXPWRCAPCONFIG_WCI2) {
            if (wlc_cmi->txcap_wci2_cell_status_last & 2) {
                if (wlc_cmi->txcap_high_cap_active &&
                    wlc_cmi->txcap_state[subband] == TXPWRCAPSTATE_HIGH_CAP) {
                    new_cell_status = 0;
                } else {
                    new_cell_status = 1;
                }
            } else if (wlc_cmi->txcap_wci2_cell_status_last & 1) {
                new_cell_status = 1;
            } else {
                new_cell_status = 0;
            }
            memcpy(&wl_txpwrcap_tbl.pwrcap_cell_on, pwrcap_cell_on, num_antennas);
            memcpy(&wl_txpwrcap_tbl.pwrcap_cell_off, pwrcap_cell_off, num_antennas);
        } else { /* TXPWRCAPCONFIG_HOST */
            /* Use the same values for cell on and off.  You might think you would pass
             * both the cell on and off values as normal and rely on the new cell status
             * value to pick the host selected/forced value.  But because the PSM uses
             * the cell on (i.e. low cap) values as a 'fail safe' as it wakes from sleep
             * you have to pass the same values.  In this way you defeat the 'fail-safe'
             * which is what you want if you are in host mode and WCI2 is not relevant.
             *
             * Technically the duplication is only needed for a new cell status of
             * off.  If the new cell status is on then the PSM is going to use the
             * cell on values, both for fail-safe and othewise.  The * cell off values
             * will not get used by the PSM.
             */
            if (wlc_cmi->txcap_high_cap_active &&
                wlc_cmi->txcap_state[subband] == TXPWRCAPSTATE_HIGH_CAP) {
                new_cell_status = 0;
                pwrcap = pwrcap_cell_off;
            } else {
                new_cell_status = 1;
                pwrcap = pwrcap_cell_on;
            }
            memcpy(&wl_txpwrcap_tbl.pwrcap_cell_on, pwrcap, num_antennas);
            memcpy(&wl_txpwrcap_tbl.pwrcap_cell_off, pwrcap, num_antennas);
        }
        if (txpwrcap_tbl) {
            memcpy(txpwrcap_tbl, &wl_txpwrcap_tbl, sizeof(wl_txpwrcap_tbl_t));
        } else {
            err = wlc_phy_txpwrcap_tbl_set(WLC_PI(wlc), &wl_txpwrcap_tbl);
        }

        if (cellstatus) {
            *cellstatus = new_cell_status;
        }
        else {
            WL_TXPWRCAP(("%s: setting phy cell status to %s\n",
                __FUNCTION__, new_cell_status ? "ON" : "OFF"));
            wlc_phyhal_txpwrcap_set_cellstatus(WLC_PI(wlc), new_cell_status);
        }
    } else {
    }
    return err;
}

static dload_error_status_t
wlc_handle_txcap_dload(wlc_info_t *wlc, wlc_blob_segment_t *segments, uint32 segment_count)
{
    wlc_cm_info_t *wlc_cmi = wlc->cmi;
    dload_error_status_t status = DLOAD_STATUS_DOWNLOAD_SUCCESS;
    txcap_file_header_t *txcap_dataptr;
    int txcap_data_len;
    uint32 chip;

    /* Make sure we have a chip id segment and txcap data segemnt */
    if (segment_count < 2) {
        return DLOAD_STATUS_TXCAP_BLOB_FORMAT;
    }
    /* Check to see if chip id segment is correct length */
    if (segments[1].length != 4) {
        return DLOAD_STATUS_TXCAP_BLOB_FORMAT;
    }
    /* Check to see if chip id matches this chip's actual value */
    chip = load32_ua(segments[1].data);
    /* There are cases that chip id is updated for modules but not ref board, we need to check
     * both here
     */
    if ((chip != SI_CHIPID(wlc_cmi->pub->sih)) && (chip != wlc_cmi->pub->sih->chip)) {
        return DLOAD_STATUS_TXCAP_MISMATCH;
    }
    /* Process actual clm data segment */
    txcap_dataptr = (txcap_file_header_t *)(segments[0].data);
    txcap_data_len = segments[0].length;
    /* Check that the header is big enough to access fixed fields */
    if (txcap_data_len < sizeof(txcap_file_header_t))  {
        return DLOAD_STATUS_TXCAP_DATA_BAD;
    }
    /* Check for magic string at the beginning */
    if (memcmp(txcap_dataptr->magic, "TXCP", 4) != 0) {
        return DLOAD_STATUS_TXCAP_DATA_BAD;
    }
    /* Check for version 2.  That is all we support, i.e. no backward compatiblity */
    if (txcap_dataptr->version != 2) {
        return DLOAD_STATUS_TXCAP_DATA_BAD;
    } else {
        if (txcap_dataptr->flags & TXCAP_FILE_HEADER_FLAG_WCI2_AND_HOST_MASK) {
            wlc_cmi->txcap_cap_states_per_cc_group = 4;
        } else {
            wlc_cmi->txcap_cap_states_per_cc_group = 2;
        }
    }
    /* At this point forward we are responsible for freeing this data pointer */
    segments[0].data = NULL;
    /* Save the txcap download after freeing any previous txcap download */
    if (wlc_cmi->txcap_download != NULL) {
        MFREE(wlc_cmi->pub->osh, wlc_cmi->txcap_download, wlc_cmi->txcap_download_size);
    }
    wlc_cmi->txcap_download = txcap_dataptr;
    wlc_cmi->txcap_download_size = txcap_data_len;
    /* XXX JAVB REVIST - what else should be clean up on new download: timer?, ???
     * note txcapload requires/enforces driver is down
     */
    memset(wlc_cmi->txcap_config, TXPWRCAPCONFIG_WCI2, TXPWRCAP_NUM_SUBBANDS);
    memset(wlc_cmi->txcap_state, TXPWRCAPSTATE_LOW_CAP, TXPWRCAP_NUM_SUBBANDS);

    wlc_channel_txcap_set_country(wlc_cmi);
    wlc_channel_txcap_phy_update(wlc_cmi, NULL, NULL);

    return status;
}

static int
wlc_channel_txcapver(wlc_cm_info_t *wlc_cmi, struct bcmstrbuf *b)
{
    if (wlc_cmi->txcap_download == NULL) {
        bcm_bprintf(b, "No txcap file downloaded\n");
    } else {
         /* Print the downloaded txcap binary file structure */
        uint8 *p;
        txcap_file_header_t *txcap_file_header;
        p = (uint8 *)wlc_cmi->txcap_download;
        txcap_file_header = (txcap_file_header_t *)p;
        bcm_bprintf(b, "Data Title: %s\n", txcap_file_header->title);
        bcm_bprintf(b, "Data Creation: %s\n", txcap_file_header->creation);
    }
    return BCME_OK;
}

void
wlc_channel_txcap_cellstatus_cb(wlc_cm_info_t *wlc_cmi, int cellstatus)
{
    WL_TXPWRCAP(("%s: ucode reporting cellstatus %s\n", __FUNCTION__,
        (cellstatus & 2) ? "UNKNOWN" :
        (cellstatus & 1) ? "ON" : "OFF"));
    wlc_cmi->txcap_wci2_cell_status_last = (uint8)cellstatus;
    /* Special processing to allow cell status/diversity to work when there is
     * no txcap file downloaded.
     */
    if (wlc_cmi->txcap_download == NULL) {
        int new_cell_status;
        if (cellstatus & 2) {
            /* Unknown cell status is translated to a cell status of 0/off
             * which means that diversity is enabled.  This allows diversity
             * if there is no txcap file downloaded.
             */
            new_cell_status = 0;
        } else {
            new_cell_status = cellstatus & 1;
        }
        WL_TXPWRCAP(("%s: setting phy cell status to %s\n",
            __FUNCTION__, new_cell_status ? "ON" : "OFF"));
        wlc_phyhal_txpwrcap_set_cellstatus(WLC_PI(wlc_cmi->wlc), new_cell_status);
    } else {
        wlc_channel_txcap_phy_update(wlc_cmi, NULL, NULL);
    }
}
#endif /* WLC_TXPWRCAP */

#if defined(BCMPCIEDEV_SROM_FORMAT) && defined(WLC_TXCAL)
static dload_error_status_t
wlc_handle_cal_dload_wrapper(wlc_info_t *wlc, wlc_blob_segment_t *segments,
    uint32 segment_count)
{
    return wlc_handle_cal_dload(wlc, segments, segment_count);
}
#endif /* WLC_TXCAL */

#ifdef WL_AP_CHAN_CHANGE_EVENT
void
wlc_channel_send_chan_event(wlc_info_t *wlc, wlc_bsscfg_t *cfg, wl_chan_change_reason_t reason,
    chanspec_t chanspec)
{
    wl_event_change_chan_t evt_data;
    ASSERT(wlc != NULL);

    memset(&evt_data, 0, sizeof(evt_data));

    evt_data.version = WL_CHAN_CHANGE_EVENT_VER_1;
    evt_data.length = WL_CHAN_CHANGE_EVENT_LEN_VER_1;
    evt_data.target_chanspec = chanspec;
    evt_data.reason = reason;

    wlc_bss_mac_event(wlc, cfg, WLC_E_AP_CHAN_CHANGE, NULL, 0,
        0, 0, (void *)&evt_data, sizeof(evt_data));
    WL_TRACE(("wl%d: CHAN Change event to ch 0x%02x\n", WLCWLUNIT(wlc),
        evt_data.target_chanspec));
}
#endif /* WL_CHAN_CHANGE_EVENT */

/* Returns a valid random chanspec or 0 on error */
chanspec_t
wlc_channel_rand_chanspec(wlc_info_t *wlc, bool exclude_current, bool radar_detected)
{
    chanspec_t def_chspec = wlc->default_bss->chanspec, chspec, rand_chspec = 0, pri_ch;
    int band_idx = wlc_chspecband2bandunit_noassert(CHSPEC_BAND(def_chspec));
    uint16 bw, bw_idx = 0;
    uint32 rand_num = (wlc->clk ? R_REG(wlc->osh, D11_TSF_RANDOM(wlc)) : 0) + wlc->chanspec;
    int16 rand_idx;
    char chanbuf[CHANSPEC_STR_LEN] = {0};
    /* list of bandwidth in decreasing order of magnitude */
    uint16 bw_list[] = { WL_CHANSPEC_BW_160, WL_CHANSPEC_BW_80,
            WL_CHANSPEC_BW_40, WL_CHANSPEC_BW_20 };
    /* length of the list of bandwidth */
    uint16 bw_list_len = ARRAYSIZE(bw_list);
    char abbrev[WLC_CNTRY_BUF_SZ] = {0};
    wl_uint32_list_t *list = NULL;
    uint16 alloc_len = ((1 + WL_NUMCHANSPECS) * sizeof(uint32));
    int i, j;
    wlc_cm_info_t *cmi = wlc->cmi;

    BCM_REFERENCE(chanbuf);

    if ((list = (wl_uint32_list_t *)MALLOC(wlc->osh, alloc_len)) == NULL) {
        WL_ERROR(("wl%d: %s: out of memory", wlc->pub->unit, __FUNCTION__));
        return 0;
    }

    while (bw_idx < bw_list_len && rand_chspec == 0) {
        bw = bw_list[bw_idx++];
        if (!wlc_is_valid_bw(wlc, wlc->primary_bsscfg, band_idx, bw)) {
            continue; /* skip unsupported bandwidth */
        }

        list->count = 0;
        wlc_get_valid_chanspecs(cmi, list, band_idx,
            wlc_chspec_bw2bwcap_bit(bw), abbrev);

        /* filter the list; remove restricted, unwanted chanspecs */
        for (i = 0; i < list->count; i++) {
            chspec = list->element[i];
            pri_ch = wf_chspec_primary20_chan(chspec);

            if ((exclude_current && wf_chspec_overlap(chspec, wlc->chanspec)) ||
                    wlc_restricted_chanspec(cmi, chspec) ||
#ifdef RADAR /* Deprioritize TDWR */
                    (band_idx == BAND_5G_INDEX &&
                    (!wlc_dfs_valid_ap_chanspec(wlc, chspec) ||
                    wlc_is_european_weather_radar_channel(wlc, chspec) ||
                    (radar_detected && wlc_quiet_chanspec(cmi, chspec)))) ||
#endif /* RADAR */
                    (band_idx == BAND_6G_INDEX && !WF_IS_6G_PSC_CHAN(pri_ch))) {
                list->count--;
                /* Remove chanspec from list */
                for (j = i; j < list->count; j++) {
                    list->element[j] = list->element[j + 1];
                }
                i--;
                continue;
            }
        }

        if (list->count == 0) {
            continue; /* no valid channels of given bandwidth found */
        }
        rand_idx = rand_num % list->count;
        rand_chspec = list->element[rand_idx];
    }
    MFREE(wlc->osh, list, alloc_len);

    if (rand_chspec == 0) {
        return 0;
    }

    WL_REGULATORY(("wl%d: %s: selected random chanspec %s (%04x)\n", wlc->pub->unit,
            __FUNCTION__, wf_chspec_ntoa(rand_chspec, chanbuf), rand_chspec));

    ASSERT(wlc_valid_chanspec_db(wlc->cmi, rand_chspec));

    return rand_chspec;
}

/*
 * Return a valid 2g chanspec of current BW. If none are found, returns 0.
 */
chanspec_t
wlc_channel_next_2gchanspec(wlc_cm_info_t *wlc_cmi, chanspec_t cur_chanspec)
{
    chanspec_t chspec;
    uint16 chan;
    uint16 cur_bw = CHSPEC_BW_GE(cur_chanspec, WL_CHANSPEC_BW_80) ? WL_CHANSPEC_BW_40 :
            CHSPEC_BW(cur_chanspec);

    for (chan = 1; chan <= CH_MAX_2G_CHANNEL; chan++) {
        chspec = CHBW_CHSPEC(cur_bw, chan);
        if (wlc_valid_chanspec(wlc_cmi, chspec)) {
            return chspec;
        }
    }
    return 0;
}

#ifdef WL_GLOBAL_RCLASS
/* check client's 2g/5g/6g band and global operating support class capability */
uint8
wlc_sta_supports_global_rclass(uint8 *rclass_bitmap)
{
    const bcmwifi_rclass_info_t *rcinfo;
    uint8 global_opclass_support = 0;
    int max_iter;

    /* As of now check for Global opclass support, if present update
     * and return.
     * TODO: check for country specific opclass if present and update
     * country specific opclass info, may be useful later
     */
    FOREACH_RCLASS_INFO(BCMWIFI_RCLASS_TYPE_GBL, rcinfo, max_iter) {
        /*
         * operating classes 128 (5G 80MHz) and 129 (5G 160MHz) are
         * harmonized across all Annex E tables so skip them.
         */
        if (rcinfo->rclass == 128 || rcinfo->rclass == 129)
            continue;

        if (isset(rclass_bitmap, rcinfo->rclass)) {
            switch (rcinfo->band) {
            case BCMWIFI_BAND_2G:
                global_opclass_support |= (1 << BAND_2G_INDEX);
                break;
            case BCMWIFI_BAND_5G:
                global_opclass_support |= (1 << BAND_5G_INDEX);
                break;
            case BCMWIFI_BAND_6G:
                global_opclass_support |= (1 << BAND_6G_INDEX);
                break;
            default:
                ASSERT(0);
                break;
            }
        }
    }
    return global_opclass_support;
}

#endif /* WL_GLOBAL_RCLASS */

/* check client's 2g/5g/6g band from country specific supported rclasses */
uint8
wlc_sta_supported_bands_from_rclass(bcmwifi_rclass_type_t rc_type, uint8 *rclass_bitmap)
{
    const bcmwifi_rclass_info_t *rcinfo;
    uint8 band = 0;
    int max_iter;

    FOREACH_RCLASS_INFO(rc_type, rcinfo, max_iter) {

        if (isset(rclass_bitmap, rcinfo->rclass)) {
            switch (rcinfo->band) {
            case BCMWIFI_BAND_2G:
                band |= (1 << BAND_2G_INDEX);
                break;
            case BCMWIFI_BAND_5G:
                band |= (1 << BAND_5G_INDEX);
                break;
            case BCMWIFI_BAND_6G:
                band |= (1 << BAND_6G_INDEX);
                break;
            default:
                ASSERT(0);
                break;
            }
        }
    }
    return band;
}

bcmwifi_rclass_type_t
wlc_channel_get_cur_rclass(struct wlc_info *wlc)
{
    bcmwifi_rclass_type_t rc_type;

    if (wlc->cmi->use_global)
        rc_type = BCMWIFI_RCLASS_TYPE_GBL;
    else
        rc_type = wlc->cmi->cur_rclass_type;
    return rc_type;
}

void
wlc_update_rcinfo(wlc_cm_info_t *wlc_cmi, bool use_global)
{
    bcmwifi_rclass_type_t rc_type;
    uint bandmask = wlc_cmi->pub->_bandmask;

    if (wlc_cmi->wlc->bandlocked)
        bandmask = (1 << wlc_cmi->wlc->band->bandunit);

    rc_type = bcmwifi_rclass_get_rclasstype_from_cntrycode(wlc_cmi->country_abbrev);

    /* bail out if nothing changed */
    if (rc_type == wlc_cmi->cur_rclass_type && use_global == wlc_cmi->use_global &&
        wlc_cmi->rclass_bandmask == bandmask)
        return;

    wlc_cmi->use_global = use_global;
    wlc_cmi->cur_rclass_type = rc_type;
    wlc_cmi->rclass_bandmask = bandmask;

    wlc_regclass_vec_init(wlc_cmi);
}

uint8 *
wlc_write_wide_bw_chan_ie(chanspec_t chspec, uint8 *cp, int buflen)
{
    dot11_wide_bw_chan_ie_t *wide_bw_chan_ie;
    uint8 center_chan;

    /* perform buffer length check. */
    /* if not big enough, return buffer untouched */
    BUFLEN_CHECK_AND_RETURN((TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN), buflen, cp);

    wide_bw_chan_ie = (dot11_wide_bw_chan_ie_t *) cp;
    wide_bw_chan_ie->id = DOT11_NGBR_WIDE_BW_CHAN_SE_ID;
    wide_bw_chan_ie->len = DOT11_WIDE_BW_IE_LEN;

    if (CHSPEC_IS40(chspec))
        wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_40;
    else if (CHSPEC_IS80(chspec))
        wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_80;
    else if (CHSPEC_IS160(chspec))
        wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_160;
    else if (CHSPEC_IS8080(chspec))
        wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_80_80;
    else
        wide_bw_chan_ie->channel_width = WIDE_BW_CHAN_WIDTH_20;

    if (CHSPEC_IS8080(chspec)) {
        wide_bw_chan_ie->center_frequency_segment_0 =
            wf_chspec_primary80_channel(chspec);
        wide_bw_chan_ie->center_frequency_segment_1 =
            wf_chspec_secondary80_channel(chspec);
    }
    else {
        center_chan = CHSPEC_CHANNEL(chspec) >> WL_CHANSPEC_CHAN_SHIFT;
        wide_bw_chan_ie->center_frequency_segment_0 = center_chan;
        wide_bw_chan_ie->center_frequency_segment_1 = 0;
    }

    cp += (TLV_HDR_LEN + DOT11_WIDE_BW_IE_LEN);

    return cp;
}

#ifdef WL11H
static uint wlc_channel_calc_rclass_ie_len(void *ctx, wlc_iem_calc_data_t *data)
{
    wlc_cm_info_t *cmi = ctx;
    wlc_bsscfg_t *cfg = data->cfg;

    if (!BSS_WL11H_ENAB(cmi->wlc, cfg))
        return 0;

    if (!cmi->valid_rcvec.count)
        return 0;

    /* first octet is current operating class */
    return TLV_HDR_LEN + 1 + cmi->valid_rcvec.count;
}

static int wlc_channel_write_rclass_ie(void *ctx, wlc_iem_build_data_t *data)
{
    wlc_cm_info_t *cmi = ctx;
    wlc_info_t *wlc = cmi->wlc;
    wlc_bsscfg_t *cfg = data->cfg;
    chanspec_t chanspec;
    uint8 opclass;

    if (!BSS_WL11H_ENAB(wlc, cfg))
        return BCME_OK;

    if (cfg->associated)
        chanspec = cfg->current_bss->chanspec;
    else
        chanspec = wlc->default_bss->chanspec;

    opclass = wlc_get_regclass(cmi, chanspec);

    return wlc_channel_write_supp_opclasses_ie(cmi, opclass, data->buf, data->buf_len);
}
#endif /* WL11H */
