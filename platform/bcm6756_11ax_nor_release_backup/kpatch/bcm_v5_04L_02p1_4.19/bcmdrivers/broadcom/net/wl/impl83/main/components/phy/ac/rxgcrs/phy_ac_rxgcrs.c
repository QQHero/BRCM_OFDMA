/*
 * ACPHY Rx Gain Control and Carrier Sense module implementation
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
 * $Id: phy_ac_rxgcrs.c 801233 2021-07-19 12:17:20Z $
 */

#include <phy_cfg.h>
#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include "phy_type_rxgcrs.h"
#include <phy_utils_var.h>
#include <phy_ac.h>
#include <phy_ac_rxgcrs.h>
#include <phy_ac_btcx.h>
#include <phy_ac_fcbs.h>
#include <phy_ac_noise.h>
#include <phy_ac_rssi.h>
#include <phy_rxgcrs_api.h>
#include <phy_noise_api.h>
#include <phy_misc_api.h>
#include <phy_ocl_api.h>
#include <phy_ac_ocl.h>
#include <phy_ac_chanmgr.h>

/* ************************ */
/* Modules used by this module */
/* ************************ */
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20698.h>
#include <wlc_radioreg_20704.h>
#include <wlc_radioreg_20707.h>
#include <wlc_radioreg_20708.h>
#include <wlc_radioreg_20709.h>
#include <wlc_radioreg_20710.h>
#include <wlc_phy_radio.h>
#include <wlc_phyreg_ac.h>

#include <phy_utils_reg.h>
#include <phy_ac_info.h>
#include <phy_stf.h>

#include <hndpmu.h>
#include <bcmdevs.h>
#include <phy_rstr.h>
#include <d11.h>
#include <wlc_phy_shim.h>

/* BCMATTACHDATA are destroyed after attach, so need to be populated at register()
these are chip & band  specific settings, so need to populate them
 */
typedef struct {
    int8  lna12_gain_tbl[2][N_LNA12_GAINS];
    uint8 lna1Rout_tbl[N_LNA12_GAINS];
} phy_ac_rxg_params_attach_t;

/* Rx Gain Control params */
typedef struct {
    phy_ac_rxg_params_attach_t tbl2g, tbl5g, tbl6g;
    phy_ac_rxg_params_attach_t tbl6glo, tbl6glo2, tbl6glo3,
        tbl6gmd, tbl6ghi, tbl6ghi2, tbl6ghi3;
} phy_ac_rxg_params_rfbands_t;

/* Rx Gain Control params */
typedef struct {
    /* band-specific */
    phy_ac_rxg_params_attach_t *rftbls;

    int8 gainlimit_tbl[RXGAIN_CONF_ELEMENTS][MAX_RX_GAINS_PER_ELEM];
    int8 tia_gain_tbl[N_TIA_GAINS];
    int8 tia_gainbits_tbl[N_TIA_GAINS];
    int8 biq01_gain_tbl[2][N_BIQ01_GAINS];
    int8 biq01_gainbits_tbl[2][N_BIQ01_GAINS];
    int8 fast_agc_clip_gains[4];
    int16 maxgain[ACPHY_MAX_RX_GAIN_STAGES];

    uint8 lna1_min_indx;
    uint8 lna1_max_indx;
    uint8 lna2_min_indx;
    uint8 lna2_max_indx;
    uint8 max_analog_gain;

    uint8 lna1_byp_indx;
    int8  lna1_byp_gain;
    uint8 lna1_byp_rout;
    int8  lna1_byp_nfhit;

    /* SSAGC Specific */
    int8 ssagc_clip_gains[SSAGC_CLIP1_GAINS_CNT];
    uint8 ssagc_clip2_tbl[SSAGC_CLIP2_TBL_IDX_CNT];
    uint8 ssagc_rssi_thresh[SSAGC_N_RSSI_THRESH];
    uint8 ssagc_clipcnt_thresh[SSAGC_N_CLIPCNT_THRESH];
    bool ssagc_lna1byp_flag[SSAGC_CLIP1_GAINS_CNT];
    uint8 ssagc_lna_byp_gain_thresh;
    bool ssagc_en;

    /* MclipAGC */
    bool mclip_agc_en;
    uint16 subband_edadj;
} phy_ac_rxg_params_t;

/* Rx Gain Control params table ID's */
typedef enum {
    LNA12_GAIN_TBL_2G,
    LNA12_GAIN_TBL_5G,
    LNA12_GAIN_TBL_6G,
    LNA12_GAIN_TBL_6G_LOW,
    LNA12_GAIN_TBL_6G_LOW2,
    LNA12_GAIN_TBL_6G_LOW3,
    LNA12_GAIN_TBL_6G_MID,
    LNA12_GAIN_TBL_6G_HI,
    LNA12_GAIN_TBL_6G_HI2,
    LNA12_GAIN_TBL_6G_HI3,
    TIA_GAIN_TBL,
    TIA_GAIN_BITS_TBL,
    BIQ01_GAIN_TBL,
    BIQ01_GAIN_BITS_TBL,
    GAIN_LIMIT_TBL,
    LNA1_ROUTMAP_TBL_2G,
    LNA1_ROUTMAP_TBL_5G,
    LNA1_ROUTMAP_TBL_6G,
    LNA1_ROUTMAP_TBL_6G_LOW,
    LNA1_ROUTMAP_TBL_6G_LOW2,
    LNA1_ROUTMAP_TBL_6G_LOW3,
    LNA1_ROUTMAP_TBL_6G_MID,
    LNA1_ROUTMAP_TBL_6G_HI,
    LNA1_ROUTMAP_TBL_6G_HI2,
    LNA1_ROUTMAP_TBL_6G_HI3,
    LNA1_GAINMAP_TBL_2G,
    LNA1_GAINMAP_TBL_5G,
    LNA1_GAINMAP_TBL_6G,
    LNA1_GAINMAP_TBL_6G_LOW,
    LNA1_GAINMAP_TBL_6G_LOW2,
    LNA1_GAINMAP_TBL_6G_LOW3,
    LNA1_GAINMAP_TBL_6G_MID,
    LNA1_GAINMAP_TBL_6G_HI,
    LNA1_GAINMAP_TBL_6G_HI2,
    LNA1_GAINMAP_TBL_6G_HI3,
    LNA2_GAINMAP_TBL_2G,
    LNA2_GAINMAP_TBL_5G,
    MAX_GAIN_TBL,
    FAST_AGC_CLIP_GAIN_TBL,
    SSAGC_CLIP_GAIN_TBL,
    SSAGC_LNA1BYP_TBL,
    SSAGC_CLIP2_TBL,
    SSAGC_CLIPCNT_THRESH_TBL_2G,
    SSAGC_RSSI_THRESH_TBL_2G,
    SSAGC_CLIPCNT_THRESH_TBL_5G,
    SSAGC_RSSI_THRESH_TBL_5G,
    FAST_AGC_LONF_CLIP_GAIN_TBL
} phy_ac_rxg_param_tbl_t;

/* module private states */
struct phy_ac_rxgcrs_info {
    phy_info_t *pi;
    phy_ac_info_t *aci;
    phy_rxgcrs_info_t *cmn_info;

    acphy_desense_values_t *lte_desense, *bt_desense;
    acphy_desense_values_t    *curr_desense, *zero_desense, *total_desense;
    uint8 desense_forced_dB;
    uint8 desense_forced_ofdm, desense_forced_bphy;

    acphy_fem_rxgains_t *fem_rxgains; /* Array of size PHY_CORE_MAX */
    acphy_rxgainctrl_t *rxgainctrl_params; /* Array of size PHY_CORE_MAX */
    phy_ac_rxg_params_rfbands_t *rxg_params_rfbands;
    phy_ac_rxg_params_t *rxg_params;
    int16    rxgainctrl_maxout_gains[ACPHY_MAX_RX_GAIN_STAGES];
    uint16    clip1_th, edcrs_en;
    uint16    initGain_codeA;
    uint16    initGain_codeB;
    uint16    deaf_count;
    uint8    phy_crs_th_from_crs_cal;
    uint8    rxgainctrl_stage_len[ACPHY_MAX_RX_GAIN_STAGES];
    int8    phy_noise_cache_crsmin[PHY_SIZE_NOISE_CACHE_ARRAY][PHY_CORE_MAX];
    int8    rx_elna_bypass_gain_th[PHY_CORE_MAX];
    int8    phy_noise_in_crs_min[PHY_CORE_MAX];    /* noise power in dB for all cores */
    int8    phy_noise_pwr_array[PHY_SIZE_NOISE_ARRAY][PHY_CORE_MAX];
    int8    phy_noise_counter;    /* Dummy variable for noise averaging */
    uint8    phy_debug_crscal_counter;
    uint8    phy_debug_crscal_channel;
    /* Asymmetric AWGN noise jammer fix */
    uint8    srom_asymmetricjammermod;
    int8    lna2_complete_gaintbl[12];
    /* Parameter to enable subband_cust in 43012A0 */
    int8    enable_subband_cust;
    uint8    w1clipmod;
    bool    thresh_noise_cal; /* enable/disable additional entries in noise cal table */
    bool    crsmincal_enable;        /* Flag for enabling auto crsminpower cal */
    bool    force_crsmincal;
    uint16  force_crsminval;
    bool    mdgain_trtx_allowed;
    /* lesi */
    bool lesi_cap, lesi_on;
    int8 lesi_ovrd;
    int8 lesiscale_2g;
    int8 lesiscale_5g;
    bool tia_idx_max_eq_init;
    /* Flag for enabling auto lesiscale cal */
    bool lesiscalecal_enable;
    bool fixed_gain_ncal;
    int8 nbclip_dBm[PHY_CORE_MAX], w1clip_dBm[PHY_CORE_MAX];
    int8 mclip_rssi[PHY_CORE_MAX][10];
};

static const char BCMATTACHDATA(rstr_subband_cust)[]              = "subband_cust";
static const char BCMATTACHDATA(rstr_ssagc_en)[]                  = "ssagc_en";
static const char BCMATTACHDATA(rstr_rxgains2gelnagainaD)[]       = "rxgains2gelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains2gtrelnabypaD)[]      = "rxgains2gtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains2gtrisoaD)[]          = "rxgains2gtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5gelnagainaD)[]       = "rxgains5gelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5gtrelnabypaD)[]      = "rxgains5gtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5gtrisoaD)[]          = "rxgains5gtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5gmelnagainaD)[]      = "rxgains5gmelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5gmtrelnabypaD)[]     = "rxgains5gmtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5gmtrisoaD)[]         = "rxgains5gmtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains5ghelnagainaD)[]      = "rxgains5ghelnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains5ghtrelnabypaD)[]     = "rxgains5ghtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains5ghtrisoaD)[]         = "rxgains5ghtrisoa%d";
static const char BCMATTACHDATA(rstr_rxgains6gbandXelnagainaD)[]  = "rxgains6gband%delnagaina%d";
static const char BCMATTACHDATA(rstr_rxgains6gbandXtrelnabypaD)[] = "rxgains6gband%dtrelnabypa%d";
static const char BCMATTACHDATA(rstr_rxgains6gbandXtrisoaD)[]     = "rxgains6gband%dtrisoa%d";

//static const char BCMATTACHDATA(rstr_acphy_for_dynamic_ed)[]    = "acphy_for_dynamic_ed";

static const uint8 tiny4365_g_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 tiny4365_g_lna_gain_map[] = {2, 3, 4, 5, 5, 5};
static const uint8 tiny4365_a_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 tiny4365_a_lna_gain_map[] = {4, 5, 6, 7, 7, 7};
static const uint8 ac_tia_gain_tiny_4365[] = {10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 37, 37, 37};
static const uint8 ac_tia_gainbits_tiny_4365[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9};

/* local functions */
static void phy_ac_rxgcrs_std_params_attach(phy_ac_rxgcrs_info_t *info);
static int phy_ac_rxgcrs_init(phy_type_rxgcrs_ctx_t *ctx);
static void phy_ac_rxgcrs_set_locale(phy_type_rxgcrs_ctx_t *ctx, uint8 region_group);
static int8 phy_ac_rxgcrs_get_avg_noisepwr(int8 noisepwr_array[]);
static int phy_ac_rxgcrs_get_avg_noisepwr_per_core(const phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core,
    int8 cmplx_pwr_dbm[]);
static void phy_ac_upd_lna1_bypass(phy_info_t *pi, uint8 core);
static int8 wlc_phy_rxgainctrl_encode_pktgain_acphy(phy_info_t *pi, uint8 core, int8 gain_dB,
                                     bool trloss, bool lna1byp, uint8 *gidx);
static void BCMATTACHFN(wlc_phy_srom_read_gainctrl_acphy)(phy_ac_rxgcrs_info_t *rxgcrs_info);
static int BCMATTACHFN(phy_ac_populate_rxg_params)(phy_ac_rxgcrs_info_t *rxgcrs_info);
static void phy_ac_rxgcrs_populate_rxg_params_band_specific(phy_ac_rxgcrs_info_t *rxgcrsi);
static void phy_ac_rxgcrs_populate_rxg_params_bw_specific(phy_info_t *pi);
static void phy_ac_single_shot_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change);
static void phy_ac_ssagc_rxgainctrl(phy_info_t *pi, uint8 core);
static void phy_ac_populate_ssagc_clipgain_tables(phy_info_t *pi, uint32 *rssi_clip_gains,
        uint8 core);
static uint32 phy_ac_ssgac_pack_gains(phy_info_t *pi, uint8 *gain_idx, bool trtx, bool lna1byp,
        bool trrx);
static void phy_ac_ssagc_control_config(phy_info_t *pi);
static void phy_ac_set_ssagc_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds);
static void phy_ac_set_ssagc_clipcnt_thresh(phy_info_t *pi, uint8 core, uint8 *thresholds);
static void phy_ac_ssagc_rssi_setup(phy_info_t *pi, uint8 core, bool init, bool band_change,
        bool bw_change);
static void phy_ac_fastagc_config(phy_info_t *pi);
static void phy_ac_ssagc_config(phy_info_t *pi, bool init, bool band_change,
        bool bw_change);
static void* BCMRAMFN(phy_ac_get_rxg_param_tbl)(phy_info_t *pi, phy_ac_rxg_param_tbl_t tbl_id,
        uint8 ind_2d_tbl);
static void wlc_phy_adjust_ed_thres_acphy(phy_type_rxgcrs_ctx_t * ctx, int32 *assert_thresh_dbm,
    bool set_threshold);
static bool wlc_phy_is_edcrs_high_acphy(phy_info_t *pi);
static int phy_ac_rxgcrs_setup_fixedgain_noisecal(phy_ac_rxgcrs_info_t *rxgcrsi);
static int phy_ac_rxgcrs_enable_fixedgain_noisecal(phy_ac_rxgcrs_info_t *rxgcrsi, uint16 enable);
static void wlc_phy_forced_desense_ofssets(phy_ac_rxgcrs_info_t *rxgcrsi, int8 offs[]);
#ifndef ATE_BUILD
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
static int wlc_phy_noisecal_run_acphy(phy_type_rxgcrs_ctx_t *ctx, void *a, bool set);
#endif
#endif /* !ATE_BUILD */
static rxgain_ovrd_t g_rxgainindx_cmd_ovrd[PHY_CORE_MAX];
static void
wlc_phy_lna1bypass_acphy(phy_info_t *pi, uint8 enable);
static void wlc_phy_mclip_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change);
static void wlc_phy_mclip_agc_rssi_setup(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_rev47(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_rev51(phy_info_t *pi);
static void wlc_phy_mclip_agc_force_clipgains_rev51(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_rev129(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_rev130(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_rev131(phy_info_t *pi);
static void wlc_phy_mclip_agc_rssi_setup_acphy(phy_info_t *pi);
static void wlc_phy_agc_nbw1mux_sel(phy_info_t *pi, uint8 core, int8 mclip_rssi[]);
static void wlc_phy_mclip_agc_clipcnt_thresh(phy_info_t *pi, uint8 core,
        const uint8 thresh_list[]);
static uint8 wlc_phy_mclip_agc_calc_lna_bypass_rssi(phy_info_t *pi, int8 *mclip_rssi,
        int8 sens_lna1Byp_lo, int8 sens_hi);
static uint32 wlc_phy_mclip_agc_pack_gains(phy_info_t *pi, uint8 *gain_indx, uint8 trtx,
        uint8 lna1byp);
static void phy_ac_rxgcrs_mclip_aci_fix(phy_info_t *pi, uint diff_th, uint16 idx_offset);
static int phy_ac_rxgcrs_get_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 *ret_int_ptr);
static int phy_ac_rxgcrs_set_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 int_val);
static int acphy_get_rxgain_index(phy_type_rxgcrs_ctx_t *ctx, int32 *index);
static int acphy_set_rxgain_index(phy_type_rxgcrs_ctx_t *ctx, int32 index);
static void phy_ac_rxgcrs_set_crs_min_pwr(phy_info_t *pi, uint8 ac_th, int8 offset_0,
    int8 offset_1, int8 offset_2, int8 offset_3, bool sec_only);
static void phy_ac_rxgcrs_fp_offset_calc(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 fp[], int8 offset[],
    int8 cmplx_pwr_dbm[], int8 thresh[], int8 lesi_scale[], uint8 idxlists, uint8 core);
static uint16 phy_ac_rxgcrs_sel_classifier(phy_type_rxgcrs_ctx_t *ctx, uint16 val);
static bool phy_ac_rxgcrs_wd(phy_wd_ctx_t *ctx);
static void
phy_ac_rxgcrs_stay_in_carriersearch(phy_type_rxgcrs_ctx_t *ctx, bool enable);
static int phy_ac_rxgcrs_force_tr2t(phy_type_rxgcrs_ctx_t *ctx, bool force);
static int phy_ac_calc_tia_gainlimit(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core, int8 pktgain);

#if defined(BCMDBG)
static void phy_ac_rxgcrs_phydump_chanest(phy_type_rxgcrs_ctx_t *ctx,
    struct bcmstrbuf *b);
#endif
#ifdef WLTEST
static void
phy_ac_rxgcrs_get_chanest(phy_type_rxgcrs_ctx_t *ctx, uint16 fftk, uint16 idx,
    int16 *ch_re, int16 *ch_im);
#endif /* WLTEST */
#if defined(DBG_BCN_LOSS)
static int
phy_ac_rxgcrs_dump_phycal_rx_min(phy_type_rxgcrs_ctx_t *ctx, struct bcmstrbuf *b);
#endif /* DBG_BCN_LOSS */

#ifndef WLC_DISABLE_ACI
static void wlc_phy_desense_apply_default_acphy(phy_info_t *pi);
static void
phy_ac_desense_update_crsmin(phy_info_t *pi, uint8 crsmin_init, int8 crsmin_high,
    int8 crsmin_md, int8 crsmin_lo, bool sec_only);
static void
phy_ac_desense_bphy(phy_info_t *pi, uint8 bphy_minshiftbits, uint16 bphy_peakenergy,
    uint8 bphy_desense);
#endif
//static void  wlc_phy_nvram_dyn_ed_read(phy_info_t *pi);

/* register phy type specific implementation */
phy_ac_rxgcrs_info_t *
BCMATTACHFN(phy_ac_rxgcrs_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
    phy_rxgcrs_info_t *cmn_info)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info;
    phy_type_rxgcrs_fns_t fns;

    PHY_TRACE(("%s\n", __FUNCTION__));

    /* allocate all storage together */
    if ((rxgcrs_info = phy_malloc(pi, sizeof(phy_ac_rxgcrs_info_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->lte_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc lte_desense failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->bt_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
        PHY_ERROR(("%s: bt_desense malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->curr_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
        PHY_ERROR(("%s: curr_desense malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->zero_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
        PHY_ERROR(("%s: zero_desense malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->total_desense = phy_malloc(pi, sizeof(acphy_desense_values_t))) == NULL) {
        PHY_ERROR(("%s: total_desense malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->fem_rxgains =
        phy_malloc(pi, sizeof(acphy_fem_rxgains_t) * PHY_CORE_MAX)) == NULL) {
        PHY_ERROR(("%s: fem_rxgains malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxgcrs_info->rxgainctrl_params =
        phy_malloc(pi, sizeof(acphy_rxgainctrl_t) * PHY_CORE_MAX)) == NULL) {
        PHY_ERROR(("%s: rxgainctrl_params malloc failed\n", __FUNCTION__));
        goto fail;
    }

    rxgcrs_info->pi = pi;
    rxgcrs_info->aci = aci;
    rxgcrs_info->cmn_info = cmn_info;
    rxgcrs_info->desense_forced_dB = 0;  /* No Force */
    rxgcrs_info->desense_forced_ofdm = 0;  /* No Force */
    rxgcrs_info->desense_forced_bphy = 0;  /* No Force */

    /* register watchdog fn */
    if (phy_wd_add_fn(pi->wdi, phy_ac_rxgcrs_wd, rxgcrs_info,
        PHY_WD_PRD_1TICK, PHY_WD_1TICK_RXGCRS,
        PHY_WD_FLAG_NONE) != BCME_OK) {
        PHY_ERROR(("%s: phy_wd_add_fn failed\n", __FUNCTION__));
        goto fail;
    }

    /* register PHY type specific implementation */
    bzero(&fns, sizeof(fns));
    fns.ctx = rxgcrs_info;
    fns.init_rxgcrs = phy_ac_rxgcrs_init;
    fns.set_locale = phy_ac_rxgcrs_set_locale;
    fns.adjust_ed_thres = wlc_phy_adjust_ed_thres_acphy;
    fns.is_edcrs_high = wlc_phy_is_edcrs_high_acphy;
    fns.get_rxdesens = phy_ac_rxgcrs_get_rxdesens;
    fns.set_rxdesens = phy_ac_rxgcrs_set_rxdesens;
    fns.get_rxgainindex = acphy_get_rxgain_index;
    fns.set_rxgainindex = acphy_set_rxgain_index;
#ifndef ATE_BUILD
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
    fns.forcecal_noise = wlc_phy_noisecal_run_acphy;
#endif
#endif /* !ATE_BUILD */
    fns.sel_classifier = phy_ac_rxgcrs_sel_classifier;
    fns.stay_in_carriersearch = phy_ac_rxgcrs_stay_in_carriersearch;
#if defined(BCMDBG)
    fns.phydump_chanest = phy_ac_rxgcrs_phydump_chanest;
#endif
#ifdef WLTEST
    fns.get_chanest = phy_ac_rxgcrs_get_chanest;
#endif /* WLTEST */
#if defined(DBG_BCN_LOSS)
    fns.phydump_phycal_rxmin = phy_ac_rxgcrs_dump_phycal_rx_min;
#endif /* DBG_BCN_LOSS */
    fns.force_tr2t = phy_ac_rxgcrs_force_tr2t;

    /* Read rxgainctrl srom entries - elna gain, trloss */
    wlc_phy_srom_read_gainctrl_acphy(rxgcrs_info);

    if (phy_ac_populate_rxg_params(rxgcrs_info) != BCME_OK) {
        goto fail;
    }

    if (ACPHY_HWACI_HWTBL_MITIGATION(pi) || ACPHY_MCLIP_ACI_MITIGATION(pi)) {
        phy_ac_noise_hwaci_switching_regs_tbls_list_init(pi);
    }

    phy_ac_rxgcrs_std_params_attach(rxgcrs_info);

    if (phy_rxgcrs_register_impl(cmn_info, &fns) != BCME_OK) {
        PHY_ERROR(("%s: phy_rxgcrs_register_impl failed\n", __FUNCTION__));
        goto fail;
    }

    rxgcrs_info->enable_subband_cust = (uint8)PHY_GETINTVAR_DEFAULT(pi,
        rstr_subband_cust, 0);

    return rxgcrs_info;

    /* error handling */
fail:
    phy_ac_rxgcrs_unregister_impl(rxgcrs_info);
    return NULL;
}

void
BCMATTACHFN(phy_ac_rxgcrs_unregister_impl)(phy_ac_rxgcrs_info_t *rxgcrs_info)
{
    phy_info_t *pi;

    PHY_TRACE(("%s\n", __FUNCTION__));

    if (rxgcrs_info == NULL)
        return;

    pi = rxgcrs_info->pi;

    /* unregister from common */
    phy_rxgcrs_unregister_impl(rxgcrs_info->cmn_info);

    if (rxgcrs_info->rxg_params_rfbands) {
        phy_mfree(pi, rxgcrs_info->rxg_params_rfbands, sizeof(phy_ac_rxg_params_rfbands_t));
    }

    if (rxgcrs_info->rxg_params) {
        phy_mfree(pi, rxgcrs_info->rxg_params, sizeof(phy_ac_rxg_params_t));
    }

    if (rxgcrs_info->rxgainctrl_params) {
        phy_mfree(pi, rxgcrs_info->rxgainctrl_params,
        sizeof(acphy_rxgainctrl_t) * PHY_CORE_MAX);
    }

    if (rxgcrs_info->fem_rxgains) {
        phy_mfree(pi, rxgcrs_info->fem_rxgains, sizeof(acphy_fem_rxgains_t) * PHY_CORE_MAX);
    }

    if (rxgcrs_info->total_desense) {
        phy_mfree(pi, rxgcrs_info->total_desense, sizeof(acphy_desense_values_t));
    }
    if (rxgcrs_info->zero_desense) {
        phy_mfree(pi, rxgcrs_info->zero_desense, sizeof(acphy_desense_values_t));
    }
    if (rxgcrs_info->curr_desense) {
        phy_mfree(pi, rxgcrs_info->curr_desense, sizeof(acphy_desense_values_t));
    }
    if (rxgcrs_info->bt_desense) {
        phy_mfree(pi, rxgcrs_info->bt_desense, sizeof(acphy_desense_values_t));
    }
    if (rxgcrs_info->lte_desense) {
        phy_mfree(pi, rxgcrs_info->lte_desense, sizeof(acphy_desense_values_t));
    }
    phy_mfree(pi, rxgcrs_info, sizeof(phy_ac_rxgcrs_info_t));
}

/* Enable/disable LNA1 bypass using radio overrides. */
/* See TCL proc acphy_lna1bypass_en */
static void
wlc_phy_lna1bypass_acphy(phy_info_t *pi, uint8 enable)
{
    ASSERT(enable <= 1);

    if (CHSPEC_IS2G(pi->radio_chanspec) || (enable == 0)) {
        MOD_RADIO_REG(pi, RFX, LNA2G_CFG1, lna1_bypass, enable);
        MOD_RADIO_REG(pi, RFX, OVR6, ovr_lna2g_lna1_bypass, enable);
        MOD_RADIO_REG(pi, RFX, LNA2G_CFG1, lna1_pu, 1 - enable);
        MOD_RADIO_REG(pi, RFX, OVR6, ovr_lna2g_lna1_pu, enable);
    }
    if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) || (enable == 0)) {
        MOD_RADIO_REG(pi, RFX, LNA5G_CFG1, lna1_bypass, enable);
        MOD_RADIO_REG(pi, RFX, OVR4, ovr_lna5g_lna1_bypass, enable);
        MOD_RADIO_REG(pi, RFX, LNA5G_CFG1, lna1_pu, 1 - enable);
        MOD_RADIO_REG(pi, RFX, OVR7, ovr_lna5g_lna1_pu, enable);
    }
}

/* inter-module data API */
bool
phy_ac_rxgcrs_get_md_trtx(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    return rxgcrsi->mdgain_trtx_allowed;
}

uint16
phy_ac_rxgcrs_get_deaf_count(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    return rxgcrsi->deaf_count;
}

static int
WLBANDINITFN(phy_ac_rxgcrs_init)(phy_type_rxgcrs_ctx_t *ctx)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrsi->pi;

    if (pi->phy_init_por) {
        rxgcrsi->mdgain_trtx_allowed = FALSE;
        pi->interf->home_chanspec = pi->radio_chanspec;
    }

    /* Sets Assert and Deassert thresholds for all 20MHz subbands for EDCRS */
    /* edcrs phyreg is shadowed.
     * For shadowed reg/table, initialed values have to be put on both sets.
     */
#ifdef ENABLE_FCBS
        if (IS_FCBS(pi)) {
            for (chanidx = 0; chanidx < MAX_FCBS_CHANS; chanidx++) {
                wlc_phy_prefcbsinit_acphy(pi, chanidx);
            }
            wlc_phy_prefcbsinit_acphy(pi, 0);
        }
#endif

    return BCME_OK;
}

static void
BCMATTACHFN(wlc_phy_srom_read_gainctrl_acphy)(phy_ac_rxgcrs_info_t *rxgcrs_info)
{
    phy_info_t *pi = rxgcrs_info->pi;
    uint8 core, srom_rx, ant, sb;
    char srom_name[30];
    phy_info_acphy_t *pi_ac;
    uint8 raw_elna, raw_trloss, raw_bypass;
    uint8 i, subband_5gh_valid;
#ifndef BOARD_FLAGS3
    uint32 bfl3; /* boardflags3 */
#endif

    pi_ac = rxgcrs_info->aci;
    BCM_REFERENCE(pi_ac);

#ifndef BOARD_FLAGS
    BF_ELNA_2G(pi_ac) = (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
                                  BFL_SROM11_EXTLNA) != 0;
    BF_ELNA_5G(pi_ac) = (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
                                  BFL_SROM11_EXTLNA_5GHz) != 0;
#endif /* BOARD_FLAGS */

    subband_5gh_valid = 1;

    FOREACH_CORE(pi, core) {
        ant = phy_get_rsdbbrd_corenum(pi, core);
        pi_ac->sromi->femrx_2g[ant].elna = 0;
        pi_ac->sromi->femrx_2g[ant].trloss = 0;
        pi_ac->sromi->femrx_2g[ant].elna_bypass_tr = 0;

        /*  -------  2G -------  */
        if (BF_ELNA_2G(pi_ac)) {
            snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gelnagainaD, ant);
            if (PHY_GETVAR(pi, srom_name) != NULL) {
                srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
                pi_ac->sromi->femrx_2g[ant].elna = (2 * srom_rx) + 6;
            }

            snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gtrelnabypaD, ant);
            if (PHY_GETVAR(pi, srom_name) != NULL) {
                pi_ac->sromi->femrx_2g[ant].elna_bypass_tr =
                        (uint8)PHY_GETINTVAR(pi, srom_name);
            }
        }

        snprintf(srom_name, sizeof(srom_name),  rstr_rxgains2gtrisoaD, ant);
        if (PHY_GETVAR(pi, srom_name) != NULL) {
            srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
            pi_ac->sromi->femrx_2g[ant].trloss = (2 * srom_rx) + 8;
        }

        if (PHY_BAND5G_ENAB(pi)) {
            pi_ac->sromi->femrx_5g[ant].elna = 0;
            pi_ac->sromi->femrx_5g[ant].trloss = 0;
            pi_ac->sromi->femrx_5g[ant].elna_bypass_tr = 0;

            pi_ac->sromi->femrx_5gm[ant].elna = 0;
            pi_ac->sromi->femrx_5gm[ant].trloss = 0;
            pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr = 0;

            pi_ac->sromi->femrx_5gh[ant].elna = 0;
            pi_ac->sromi->femrx_5gh[ant].trloss = 0;
            pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr = 0;

            /*  -------  5G -------  */
            if (BF_ELNA_5G(pi_ac)) {
                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5gelnagainaD, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL) {
                    srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
                    pi_ac->sromi->femrx_5g[ant].elna = (2 * srom_rx) + 6;
                }

                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5gtrelnabypaD, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL) {
                    pi_ac->sromi->femrx_5g[ant].elna_bypass_tr =
                            (uint8)PHY_GETINTVAR(pi, srom_name);
                }
            }

            snprintf(srom_name, sizeof(srom_name), rstr_rxgains5gtrisoaD, ant);
            if (PHY_GETVAR(pi, srom_name) != NULL) {
                srom_rx = (uint8)PHY_GETINTVAR(pi, srom_name);
                pi_ac->sromi->femrx_5g[ant].trloss = (2 * srom_rx) + 8;
            }

            /*  -------  5G (mid) -------  */
            raw_elna = 0; raw_trloss = 0; raw_bypass = 0;
            if (BF_ELNA_5G(pi_ac)) {
                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5gmelnagainaD, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL)
                    raw_elna = (uint8)PHY_GETINTVAR(pi, srom_name);

                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5gmtrelnabypaD, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL)
                    raw_bypass = (uint8)PHY_GETINTVAR(pi, srom_name);
            }
            snprintf(srom_name, sizeof(srom_name), rstr_rxgains5gmtrisoaD, ant);
            if (PHY_GETVAR(pi, srom_name) != NULL)
                raw_trloss = (uint8)PHY_GETINTVAR(pi, srom_name);

            if (((raw_elna == 0) && (raw_trloss == 0) && (raw_bypass == 0)) ||
                ((raw_elna == 7) && (raw_trloss == 0xf) && (raw_bypass == 1))) {
                /* No entry in SROM, use generic 5g ones */
                pi_ac->sromi->femrx_5gm[ant].elna =
                        pi_ac->sromi->femrx_5g[ant].elna;
                pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr =
                        pi_ac->sromi->femrx_5g[ant].elna_bypass_tr;
                pi_ac->sromi->femrx_5gm[ant].trloss =
                        pi_ac->sromi->femrx_5g[ant].trloss;
            } else {
                if (BF_ELNA_5G(pi_ac)) {
                    pi_ac->sromi->femrx_5gm[ant].elna = (2 * raw_elna) + 6;
                    pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr = raw_bypass;
                }
                pi_ac->sromi->femrx_5gm[ant].trloss = (2 * raw_trloss) + 8;
            }

            /*  -------  5G (high) -------  */
            raw_elna = 0; raw_trloss = 0; raw_bypass = 0;
            if (BF_ELNA_5G(pi_ac)) {
                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5ghelnagainaD, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL)
                    raw_elna = (uint8)PHY_GETINTVAR(pi, srom_name);

                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains5ghtrelnabypaD, ant);

                if (PHY_GETVAR(pi, srom_name) != NULL)
                    raw_bypass = (uint8)PHY_GETINTVAR(pi, srom_name);
            }
            snprintf(srom_name, sizeof(srom_name), rstr_rxgains5ghtrisoaD, ant);
            if (PHY_GETVAR(pi, srom_name) != NULL)
                raw_trloss = (uint8)PHY_GETINTVAR(pi, srom_name);

            if (((raw_elna == 0) && (raw_trloss == 0) && (raw_bypass == 0)) ||
                ((raw_elna == 7) && (raw_trloss == 0xf) && (raw_bypass == 1))) {
                /* No entry in SROM, use generic 5g ones */
                pi_ac->sromi->femrx_5gh[ant].elna =
                        pi_ac->sromi->femrx_5gm[ant].elna;
                pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr =
                        pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr;
                pi_ac->sromi->femrx_5gh[ant].trloss =
                        pi_ac->sromi->femrx_5gm[ant].trloss;
                subband_5gh_valid = 0;
            } else {
                if (BF_ELNA_5G(pi_ac)) {
                    pi_ac->sromi->femrx_5gh[ant].elna = (2 * raw_elna) + 6;
                    pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr = raw_bypass;
                }
                pi_ac->sromi->femrx_5gh[ant].trloss = (2 * raw_trloss) + 8;
            }

            /*  -------  6G : we have 7 subbands -------  */
            for (sb = 0; sb < NUM_6G_SUBBANDS; sb++) {
                pi_ac->sromi->femrx_6g[ant][sb].elna = 0;
                pi_ac->sromi->femrx_6g[ant][sb].trloss = 0;
                pi_ac->sromi->femrx_6g[ant][sb].elna_bypass_tr = 0;
                raw_elna = 0; raw_trloss = 0; raw_bypass = 0;
                if (BF_ELNA_5G(pi_ac)) {
                    snprintf(srom_name, sizeof(srom_name),
                        rstr_rxgains6gbandXelnagainaD, sb, ant);
                    if (PHY_GETVAR(pi, srom_name) != NULL)
                        raw_elna = (uint8)PHY_GETINTVAR(pi, srom_name);

                    snprintf(srom_name, sizeof(srom_name),
                        rstr_rxgains6gbandXtrelnabypaD, sb, ant);

                    if (PHY_GETVAR(pi, srom_name) != NULL)
                        raw_bypass = (uint8)PHY_GETINTVAR(pi, srom_name);
                }
                snprintf(srom_name, sizeof(srom_name),
                    rstr_rxgains6gbandXtrisoaD, sb, ant);
                if (PHY_GETVAR(pi, srom_name) != NULL)
                    raw_trloss = (uint8)PHY_GETINTVAR(pi, srom_name);

                if (((raw_elna == 0) && (raw_trloss == 0) && (raw_bypass == 0)) ||
                    ((raw_elna == 7) && (raw_trloss == 0xf) && (raw_bypass == 1))) {
                    /* No entry in SROM, use 5gh or generic 5g ones */
                    if (subband_5gh_valid == 1) {
                        pi_ac->sromi->femrx_6g[ant][sb].elna =
                                pi_ac->sromi->femrx_5gh[ant].elna;
                        pi_ac->sromi->femrx_6g[ant][sb].elna_bypass_tr =
                                pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr;
                        pi_ac->sromi->femrx_6g[ant][sb].trloss =
                                pi_ac->sromi->femrx_5gh[ant].trloss;
                    } else {
                        pi_ac->sromi->femrx_6g[ant][sb].elna =
                                pi_ac->sromi->femrx_5g[ant].elna;
                        pi_ac->sromi->femrx_6g[ant][sb].elna_bypass_tr =
                                pi_ac->sromi->femrx_5g[ant].elna_bypass_tr;
                        pi_ac->sromi->femrx_6g[ant][sb].trloss =
                                pi_ac->sromi->femrx_5g[ant].trloss;
                    }
                } else {
                    if (BF_ELNA_5G(pi_ac)) {
                        pi_ac->sromi->femrx_6g[ant][sb].elna =
                            (2 * raw_elna) + 6;
                        pi_ac->sromi->femrx_6g[ant][sb].elna_bypass_tr =
                            raw_bypass;
                    }
                    pi_ac->sromi->femrx_6g[ant][sb].trloss =
                            (2 * raw_trloss) + 8;
                }
            }
        }
    }

    pi_ac->sromi->dot11b_opts = (uint32)PHY_GETINTVAR_DEFAULT(pi, rstr_dot11b_opts, 1);

    if (PHY_GETVAR(pi, rstr_tiny_maxrxgain) != NULL) {
      for (i = 0; i < 3; i++) {
        pi_ac->sromi->tiny_maxrxgain[i] =
        (uint8) PHY_GETINTVAR_ARRAY_DEFAULT(pi, rstr_tiny_maxrxgain, i, 0);
      }
    } else {
        for (i = 0; i < 3; i++) {
          pi_ac->sromi->tiny_maxrxgain[i] = 0;
        }
    }

    pi_ac->sromi->gainctrlsph = (bool)PHY_GETINTVAR(pi, rstr_gainctrlsph);

#ifndef BOARD_FLAGS3
    if ((PHY_GETVAR_SLICE(pi, rstr_boardflags3)) != NULL) {
        bfl3 = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_boardflags3);
        BF3_LTECOEX_GAINTBL_EN(pi_ac) = (bfl3 & BFL3_LTECOEX_GAINTBL_EN) >>
            BFL3_LTECOEX_GAINTBL_EN_SHIFT;
    } else {
        BF3_LTECOEX_GAINTBL_EN(pi_ac) = 0;
    }
#endif /* BOARD_FLAGS3 */

    rxgcrs_info->w1clipmod = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_w1clipmod, 0);
    rxgcrs_info->srom_asymmetricjammermod = (uint8)PHY_GETINTVAR_DEFAULT_SLICE
        (pi, rstr_asymmetricjammermod, 0);
    rxgcrs_info->thresh_noise_cal = (bool)PHY_GETINTVAR_DEFAULT(pi, rstr_thresh_noise_cal, 1);
}

/* watchdog callback */
static bool
phy_ac_rxgcrs_wd(phy_wd_ctx_t *ctx)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrsi->pi;

    /* Enabling crsmin_cal from watchdog. crsmin phyregs are only updated if the
     * (current noise power - prev value from cache) is above certain threshold
     */
    if (rxgcrsi->crsmincal_enable || (rxgcrsi->lesiscalecal_enable &&
        (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            ACMAJORREV_128(pi->pubpi->phy_rev)))) {
            phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

            if (rxgcrsi->fixed_gain_ncal != 0) {
                phy_ac_rxgcrs_enable_fixedgain_noisecal(rxgcrsi, 0);
            }
            BCM_REFERENCE(stf_shdata);

        phy_noise_sample_request_crsmincal(pi);
    }
    /* Dynamic ED threshold */
    /* checking major versions */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        /* Checking nvram parameter */
        if (pi->acphy_for_dynamic_ed == 1) {
            /* defer dynamic edcrs RM is progress */
            if (!SCAN_RM_IN_PROGRESS(pi)) {
                PHY_ACI(("%s: Dynamic threshold is enabled in NVRAM.\n",
                    __FUNCTION__));
                wlc_dynamic_ed_thresh(pi);
            }
        }
    }

    return TRUE;
}

static int
BCMATTACHFN(phy_ac_populate_rxg_params)(phy_ac_rxgcrs_info_t *rxgcrs_info)
{
    phy_ac_rxg_params_rfbands_t *rfband;
    phy_ac_rxg_params_t *rxg_params;
    phy_info_t *pi = rxgcrs_info->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 *lna1_rout_map, *lna1_gain_map, i;
    uint8 region_group = wlc_phy_get_locale(pi->rxgcrsi);

    if ((rfband = phy_malloc(pi, sizeof(phy_ac_rxg_params_rfbands_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }

    if ((rxg_params = phy_malloc(pi, sizeof(phy_ac_rxg_params_t))) == NULL) {
        PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
        goto fail;
    }
    rxgcrs_info->rxg_params_rfbands = rfband;
    rxgcrs_info->rxg_params = rxg_params;

    /* inits */
    rxg_params->rftbls = &rfband->tbl2g;
    rxg_params->mclip_agc_en = FALSE;
    rxg_params->ssagc_en = (bool)PHY_GETINTVAR_DEFAULT(pi, rstr_ssagc_en, 0);
    rxg_params->lna1_byp_nfhit = -1; /* -1 => NOT used */

    /* ilna table choice */
    pi->sromi->sr18_ilna_tbl = (uint8) ((pi->sh->boardflags4 & BFL4_SROM18_ILNA_TBL) >> 29);

    if ((pi->sh->boardflags4 & BFL4_SROM18_SUBBAND_ED_ADJ) > 0) {
        rxg_params->subband_edadj =
            (uint16)PHY_GETINTVAR_DEFAULT(pi, rstr_subband_ed_adj, 0);
    } else {
        if (region_group ==  REGION_EU) {
            rxg_params->subband_edadj = 0x0;
        } else {
        // FIXME: may need lpi indicator from CLM
            rxg_params->subband_edadj = 0x2520;
        }
    }

    if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        /* Copy LNA12 gain params to structure element */
        for (i = 0; i < 2; i++) {
            memcpy(rfband->tbl2g.lna12_gain_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_2G, i),
                    sizeof(int8) * N_LNA12_GAINS);

            /* If A band supported */
            if (PHY_BAND5G_ENAB(pi)) {
                memcpy(rfband->tbl5g.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_5G, i),
                        sizeof(int8) * N_LNA12_GAINS);

                /* 6G tables. TODO: use pi->u.pi_acphy->phy_caps & PHY_CAP_SUP_6G */
                if (ACMAJORREV_47(pi->pubpi->phy_rev) &&
                    (RADIOREV(pi->pubpi->radiorev) > 2)) {
                    memcpy(rfband->tbl6g.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G, i),
                        sizeof(int8) * N_LNA12_GAINS);

                } else if (ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
                    BF_ELNA_5G(pi_ac)) {
                    memcpy(rfband->tbl6glo.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_LOW,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6glo2.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_LOW2,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6glo3.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_LOW3,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6gmd.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_MID,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6ghi.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6ghi2.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI2,
                        i), sizeof(int8) * N_LNA12_GAINS);
                    memcpy(rfband->tbl6ghi3.lna12_gain_tbl[i],
                        phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI3,
                        i), sizeof(int8) * N_LNA12_GAINS);

                } else if (ACMAJORREV_129(pi->pubpi->phy_rev) &&
                    BF_ELNA_5G(pi_ac)) {
                memcpy(rfband->tbl6glo.lna12_gain_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_LOW, i),
                    sizeof(int8) * N_LNA12_GAINS);
                memcpy(rfband->tbl6gmd.lna12_gain_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_MID, i),
                    sizeof(int8) * N_LNA12_GAINS);
                memcpy(rfband->tbl6ghi.lna12_gain_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI, i),
                    sizeof(int8) * N_LNA12_GAINS);
                memcpy(rfband->tbl6ghi2.lna12_gain_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI2, i),
                    sizeof(int8) * N_LNA12_GAINS);

                } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    if (RADIOREV(pi->pubpi->radiorev) < 2) {
                        memcpy(rfband->tbl6g.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G,
                         i), sizeof(int8) * N_LNA12_GAINS);
                    } else {
                        memcpy(rfband->tbl6glo.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_LOW,
                         i), sizeof(int8) * N_LNA12_GAINS);
                        memcpy(rfband->tbl6glo2.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi,
                         LNA12_GAIN_TBL_6G_LOW2,
                         i), sizeof(int8) * N_LNA12_GAINS);
                        memcpy(rfband->tbl6gmd.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_MID,
                         i), sizeof(int8) * N_LNA12_GAINS);
                        memcpy(rfband->tbl6ghi.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI,
                         i), sizeof(int8) * N_LNA12_GAINS);
                        memcpy(rfband->tbl6ghi2.lna12_gain_tbl[i],
                         phy_ac_get_rxg_param_tbl(pi, LNA12_GAIN_TBL_6G_HI2,
                         i), sizeof(int8) * N_LNA12_GAINS);
                    }

                } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
                    /* 6878 has no 6G support */
                } else {
                    PHY_INFORM(("%s: Unsupport 6G chip/config\n",
                        __FUNCTION__));
                }

            }
        }

        /* Copy gain limit table */
        for (i = 0; i < RXGAIN_CONF_ELEMENTS; i++) {
            memcpy(rxg_params->gainlimit_tbl[i],
                    phy_ac_get_rxg_param_tbl(pi, GAIN_LIMIT_TBL, i),
                    sizeof(int8) * MAX_RX_GAINS_PER_ELEM);
        }

        /* LNA1 Rout table (2G) */
        lna1_rout_map = phy_ac_get_rxg_param_tbl(pi, LNA1_ROUTMAP_TBL_2G, 0);
        lna1_gain_map = phy_ac_get_rxg_param_tbl(pi, LNA1_GAINMAP_TBL_2G, 0);
        for (i = 0; i < N_LNA12_GAINS; i++) {
            rfband->tbl2g.lna1Rout_tbl[i] = (lna1_rout_map[i] << 3) |
                    lna1_gain_map[i];
        }

        /* LNA1 Rout table (5G), if A band supported */
        if (PHY_BAND5G_ENAB(pi)) {
            lna1_rout_map = phy_ac_get_rxg_param_tbl(pi, LNA1_ROUTMAP_TBL_5G, 0);
            lna1_gain_map = phy_ac_get_rxg_param_tbl(pi, LNA1_GAINMAP_TBL_5G, 0);

            for (i = 0; i < N_LNA12_GAINS; i++) {
                rfband->tbl5g.lna1Rout_tbl[i] = (lna1_rout_map[i] << 3) |
                    lna1_gain_map[i];
            }

            if (ACMAJORREV_47(pi->pubpi->phy_rev) &&
                (RADIOREV(pi->pubpi->radiorev) > 2)) {
                uint8 *lna1_rout_map_6g;
                uint8 *lna1_gain_map_6g;

                lna1_rout_map_6g = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_ROUTMAP_TBL_6G, 0);
                lna1_gain_map_6g = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_GAINMAP_TBL_6G, 0);

                for (i = 0; i < N_LNA12_GAINS; i++) {
                    rfband->tbl6g.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g[i] << 3) |
                        lna1_gain_map_6g[i];
                }
            }

            if (ACMAJORREV_51_131(pi->pubpi->phy_rev) && BF_ELNA_5G(pi_ac)) {
                uint8 *lna1_rout_map_6g_subband[7];
                uint8 *lna1_gain_map_6g_subband[7];

                for (i = 0; i < 7; i++) {
                    lna1_rout_map_6g_subband[i] = phy_ac_get_rxg_param_tbl(pi,
                        i + LNA1_ROUTMAP_TBL_6G_LOW, 0);
                    lna1_gain_map_6g_subband[i] = phy_ac_get_rxg_param_tbl(pi,
                        i + LNA1_GAINMAP_TBL_6G_LOW, 0);
                }

                for (i = 0; i < N_LNA12_GAINS; i++) {
                    rfband->tbl6glo.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[0][i] << 3) |
                        lna1_gain_map_6g_subband[0][i];
                    rfband->tbl6glo2.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[1][i] << 3) |
                        lna1_gain_map_6g_subband[1][i];
                    rfband->tbl6glo3.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[2][i] << 3) |
                        lna1_gain_map_6g_subband[2][i];
                    rfband->tbl6gmd.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[3][i] << 3) |
                        lna1_gain_map_6g_subband[3][i];
                    rfband->tbl6ghi.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[4][i] << 3) |
                        lna1_gain_map_6g_subband[4][i];
                    rfband->tbl6ghi2.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[5][i] << 3) |
                        lna1_gain_map_6g_subband[5][i];
                    rfband->tbl6ghi3.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_subband[6][i] << 3) |
                        lna1_gain_map_6g_subband[6][i];
                }
            } else if (ACMAJORREV_129(pi->pubpi->phy_rev) && BF_ELNA_5G(pi_ac)) {
                uint8 *lna1_rout_map_6g_low, *lna1_rout_map_6g_mid;
                uint8 *lna1_rout_map_6g_hi, *lna1_rout_map_6g_hi2;
                uint8 *lna1_gain_map_6g_low, *lna1_gain_map_6g_mid;
                uint8 *lna1_gain_map_6g_hi, *lna1_gain_map_6g_hi2;
                lna1_rout_map_6g_low = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_ROUTMAP_TBL_6G_LOW, 0);
                lna1_gain_map_6g_low = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_GAINMAP_TBL_6G_LOW, 0);
                lna1_rout_map_6g_mid = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_ROUTMAP_TBL_6G_MID, 0);
                lna1_gain_map_6g_mid = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_GAINMAP_TBL_6G_MID, 0);
                lna1_rout_map_6g_hi = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_ROUTMAP_TBL_6G_HI, 0);
                lna1_gain_map_6g_hi = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_GAINMAP_TBL_6G_HI, 0);
                lna1_rout_map_6g_hi2 = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_ROUTMAP_TBL_6G_HI2, 0);
                lna1_gain_map_6g_hi2 = phy_ac_get_rxg_param_tbl(pi,
                    LNA1_GAINMAP_TBL_6G_HI2, 0);
                for (i = 0; i < N_LNA12_GAINS; i++) {
                    rfband->tbl6glo.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_low[i] << 3) |
                        lna1_gain_map_6g_low[i];
                    rfband->tbl6gmd.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_mid[i] << 3) |
                        lna1_gain_map_6g_mid[i];
                    rfband->tbl6ghi.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_hi[i] << 3) |
                        lna1_gain_map_6g_hi[i];
                    rfband->tbl6ghi2.lna1Rout_tbl[i] =
                        (lna1_rout_map_6g_hi2[i] << 3) |
                        lna1_gain_map_6g_hi2[i];
                }
            }

            if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                if (RADIOREV(pi->pubpi->radiorev) < 2) {

                    uint8 *lna1_rout_map_6g;
                    uint8 *lna1_gain_map_6g;

                    lna1_rout_map_6g = phy_ac_get_rxg_param_tbl(pi,
                                          LNA1_ROUTMAP_TBL_6G, 0);
                    lna1_gain_map_6g = phy_ac_get_rxg_param_tbl(pi,
                                          LNA1_GAINMAP_TBL_6G, 0);

                    for (i = 0; i < N_LNA12_GAINS; i++) {
                        rfband->tbl6g.lna1Rout_tbl[i] =
                                (lna1_rout_map_6g[i] << 3) |
                                lna1_gain_map_6g[i];
                    }
                } else {
                    uint8 *lna1_rout_map_6g_low, *lna1_rout_map_6g_mid;
                    uint8 *lna1_rout_map_6g_hi, *lna1_rout_map_6g_hi2;
                    uint8 *lna1_gain_map_6g_low, *lna1_gain_map_6g_mid;
                    uint8 *lna1_gain_map_6g_hi, *lna1_gain_map_6g_hi2;
                    uint8 *lna1_rout_map_6g_low2, *lna1_gain_map_6g_low2;
                    lna1_rout_map_6g_low = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_ROUTMAP_TBL_6G_LOW, 0);
                    lna1_gain_map_6g_low = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_GAINMAP_TBL_6G_LOW, 0);
                    lna1_rout_map_6g_mid = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_ROUTMAP_TBL_6G_MID, 0);
                    lna1_gain_map_6g_mid = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_GAINMAP_TBL_6G_MID, 0);
                    lna1_rout_map_6g_hi = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_ROUTMAP_TBL_6G_HI, 0);
                    lna1_gain_map_6g_hi = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_GAINMAP_TBL_6G_HI, 0);
                    lna1_rout_map_6g_low2 = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_ROUTMAP_TBL_6G_LOW2, 0);
                    lna1_gain_map_6g_low2 = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_GAINMAP_TBL_6G_LOW2, 0);
                    lna1_rout_map_6g_hi2 = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_ROUTMAP_TBL_6G_HI2, 0);
                    lna1_gain_map_6g_hi2 = phy_ac_get_rxg_param_tbl(pi,
                        LNA1_GAINMAP_TBL_6G_HI2, 0);
                    for (i = 0; i < N_LNA12_GAINS; i++) {
                        rfband->tbl6glo.lna1Rout_tbl[i] =
                            (lna1_rout_map_6g_low[i] << 3) |
                            lna1_gain_map_6g_low[i];
                        rfband->tbl6gmd.lna1Rout_tbl[i] =
                            (lna1_rout_map_6g_mid[i] << 3) |
                            lna1_gain_map_6g_mid[i];
                        rfband->tbl6ghi.lna1Rout_tbl[i] =
                            (lna1_rout_map_6g_hi[i] << 3) |
                            lna1_gain_map_6g_hi[i];
                        rfband->tbl6glo2.lna1Rout_tbl[i] =
                            (lna1_rout_map_6g_low2[i] << 3) |
                            lna1_gain_map_6g_low2[i];
                        rfband->tbl6ghi2.lna1Rout_tbl[i] =
                            (lna1_rout_map_6g_hi2[i] << 3) |
                            lna1_gain_map_6g_hi2[i];
                    }
                }
            }
        }

        /* TIA Gain and GainBits table */
        memcpy(rxg_params->tia_gain_tbl,
                phy_ac_get_rxg_param_tbl(pi, TIA_GAIN_TBL, 0),
                sizeof(int8) * N_TIA_GAINS);
        memcpy(rxg_params->tia_gainbits_tbl,
                phy_ac_get_rxg_param_tbl(pi, TIA_GAIN_BITS_TBL, 0),
                sizeof(int8) * N_TIA_GAINS);

        /* Copy BIQ01 gain params to structure element */
        for (i = 0; i < 2; i++) {
            memcpy(rxg_params->biq01_gain_tbl[i],
                phy_ac_get_rxg_param_tbl(pi, BIQ01_GAIN_TBL, i),
                sizeof(int8) * N_BIQ01_GAINS);
            memcpy(rxg_params->biq01_gainbits_tbl[i],
                phy_ac_get_rxg_param_tbl(pi, BIQ01_GAIN_BITS_TBL, i),
                sizeof(int8) * N_BIQ01_GAINS);
        }

        /* If these are band-specific for any chip move them to
           phy_ac_rxgcrs_populate_rxg_params_band_specific()
        */
        rxg_params->lna1_min_indx = 0;
        rxg_params->lna1_byp_indx = 6;
        rxg_params->lna1_byp_nfhit = -1;

        rxg_params->lna2_min_indx = 3;
        rxg_params->lna2_max_indx = 3;

        /* mclip agc is default to on for 4347 */
        if (!ACMAJORREV_47(pi->pubpi->phy_rev)) {
            /* hw-bug */
            rxg_params->mclip_agc_en =
                (bool)PHY_GETINTVAR_DEFAULT(pi, "mclip_agc_en", 1);
        }
    }

    return (BCME_OK);

fail:
    if (rfband != NULL)
        phy_mfree(pi, rfband, sizeof(phy_ac_rxg_params_rfbands_t));

    if (rxg_params != NULL)
        phy_mfree(pi, rxg_params, sizeof(phy_ac_rxg_params_t));

    return (BCME_NOMEM);
}

static void
phy_ac_rxgcrs_populate_rxg_params_band_specific(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_rfbands_t *rfbands = rxgcrsi->rxg_params_rfbands;
    phy_ac_rxg_params_t *rxg_params = rxgcrsi->rxg_params;
    typedef struct subband_end2tbls {
        uint16 end_freq;
        phy_ac_rxg_params_attach_t *rftbl;
    } subband_end2tbls_t;
    subband_end2tbls_t *subband_end_freq = NULL;
    int8 i, sz = 0;
    subband_end2tbls_t subband_end_freq_7subbands[] = {
        {6105, &rfbands->tbl6glo},
        {6265, &rfbands->tbl6glo2},
        {6425, &rfbands->tbl6glo3},
        {6585, &rfbands->tbl6gmd},
        {6745, &rfbands->tbl6ghi},
        {6905, &rfbands->tbl6ghi2},
        {7200, &rfbands->tbl6ghi3},
    };
    subband_end2tbls_t subband_end_freq_5subbands[] = {
        {6451, &rfbands->tbl6glo},
        {6651, &rfbands->tbl6glo2},
        {6751, &rfbands->tbl6gmd},
        {6951, &rfbands->tbl6ghi},
        {7200, &rfbands->tbl6ghi2},
    };
    subband_end2tbls_t subband_end_freq_4subbands[] = {
        {6351, &rfbands->tbl6glo},
        {6510, &rfbands->tbl6gmd},
        {6851, &rfbands->tbl6ghi},
        {7200, &rfbands->tbl6ghi2},
    };
    subband_end2tbls_t subband_end_freq_1subband[] = {
        {7200, &rfbands->tbl6g},
    };

    if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
        subband_end_freq = &subband_end_freq_7subbands[0];
        sz = ARRAYSIZE(subband_end_freq_7subbands);
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        subband_end_freq = &subband_end_freq_4subbands[0];
        sz = ARRAYSIZE(subband_end_freq_4subbands);
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        if (RADIOREV(pi->pubpi->radiorev) < 2) {
            subband_end_freq = &subband_end_freq_1subband[0];
            sz = ARRAYSIZE(subband_end_freq_1subband);
        } else {
            subband_end_freq = &subband_end_freq_5subbands[0];
            sz = ARRAYSIZE(subband_end_freq_5subbands);
        }
    } else if ((ACMAJORREV_47(pi->pubpi->phy_rev) && (RADIOREV(pi->pubpi->radiorev) > 2))) {
        subband_end_freq = &subband_end_freq_1subband[0];
        sz = ARRAYSIZE(subband_end_freq_1subband);
    }
    i = sz;

    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        rxg_params->rftbls = &rfbands->tbl2g;
    } else if (CHSPEC_IS5G(pi->radio_chanspec)) {
        rxg_params->rftbls = &rfbands->tbl5g;
    } else if (CHSPEC_IS6G(pi->radio_chanspec) && subband_end_freq &&
        ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        while ((--i >= 0) && (CHAN6G_FREQ(CHSPEC_CHANNEL(pi->radio_chanspec)) <
            subband_end_freq[i].end_freq));
        /* In case frequency is above max (7.2GHz), reuse highest defined subband */
        rxg_params->rftbls = subband_end_freq[i == sz - 1 ? i : i + 1].rftbl;
    } else {
        rxg_params->rftbls = &rfbands->tbl5g;
    }

    if (!IS_28NM_RADIO(pi) && !IS_16NM_RADIO(pi)) return;

    /* lna1-max index */
    if (ACMAJORREV_47(pi->pubpi->phy_rev) && RADIOREV(pi->pubpi->radiorev > 0)) {
        rxg_params->lna1_max_indx = 4;
    } else if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (BF_ELNA_2G(pi_ac)) {
                rxg_params->lna1_max_indx = 4;
            } else {
                rxg_params->lna1_max_indx = 5;
            }
        } else {
            rxg_params->lna1_max_indx = 4;
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev) || ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_max_indx = BF_ELNA_2G(pi_ac) ? 4 : 5;
        } else {
            rxg_params->lna1_max_indx = BF_ELNA_5G(pi_ac) ? 4 : 5;
        }
    } else {
        rxg_params->lna1_max_indx = 5;
    }

    /* lna1 bypass settings */
    if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        /* Rev 51 has no external LNA and puts lna1 in bypass for high input levels */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) &&
                (RADIOREV(pi->pubpi->radiorev) == 0)) {
                rxg_params->lna1_byp_gain = -16;
                rxg_params->lna1_byp_rout = 0;
            } else {
                rxg_params->lna1_byp_gain = -17;
                rxg_params->lna1_byp_rout = 10;
            }
        } else {
            rxg_params->lna1_byp_gain = -8;
            rxg_params->lna1_byp_rout = 11;
        }
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_byp_rout = 9;
            rxg_params->lna1_byp_gain = -16;
            rxg_params->lna1_byp_nfhit = 25;
        } else {
            rxg_params->lna1_byp_rout = 11;
            rxg_params->lna1_byp_gain = -11;
            rxg_params->lna1_byp_nfhit = 25;
        }
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_byp_rout = 0;
            rxg_params->lna1_byp_gain = -6;
            rxg_params->lna1_byp_nfhit = 25;
        } else {
            if (RADIOREV(pi->pubpi->radiorev) < 2) {
                rxg_params->lna1_byp_rout = 0;
                rxg_params->lna1_byp_gain = -4;
                rxg_params->lna1_byp_nfhit = 25;
            } else {
                /* For MCH5 */
                if ((pi->sromi->sr18_ilna_tbl == 1) &&
                    (CHSPEC_IS5G(pi->radio_chanspec))) {
                    rxg_params->lna1_byp_rout = 6;
                    rxg_params->lna1_byp_gain = 0;
                    rxg_params->lna1_byp_nfhit = 25;
                } else {
                    if (CHSPEC_IS5G(pi->radio_chanspec)) {
                        rxg_params->lna1_byp_rout = 6;
                        rxg_params->lna1_byp_gain = -3;
                        rxg_params->lna1_byp_nfhit = 25;
                    } else {
                        uint8 sb;
                        sb = (CHSPEC_CHANNEL(pi->radio_chanspec) < 101) ? 0:
                          (CHSPEC_CHANNEL(pi->radio_chanspec) < 141) ? 1:
                          (CHSPEC_CHANNEL(pi->radio_chanspec) < 161) ? 2:
                          (CHSPEC_CHANNEL(pi->radio_chanspec) < 201) ? 3: 4;

                        rxg_params->lna1_byp_rout = 6;
                        switch (sb) {
                        case 0:
                        case 1:
                            rxg_params->lna1_byp_gain = -1;
                            break;
                        case 2:
                        case 3:
                            rxg_params->lna1_byp_gain = -2;
                            break;
                        case 4:
                            rxg_params->lna1_byp_gain = -3;
                            break;
                        default:
                            rxg_params->lna1_byp_gain = -3;
                            break;
                        }
                        rxg_params->lna1_byp_nfhit = 25;
                    }
                }
            }
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_byp_gain = -15;
            rxg_params->lna1_byp_rout = 10;
        } else {
            rxg_params->lna1_byp_gain = -10;
            rxg_params->lna1_byp_rout = 11;
        }
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_byp_gain = -17;
            rxg_params->lna1_byp_rout = 10;

        } else {
            rxg_params->lna1_byp_gain = -8;
            rxg_params->lna1_byp_rout = 11;
        }
    } else {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rxg_params->lna1_byp_gain = -12;
            rxg_params->lna1_byp_rout = 10;
        } else {
            rxg_params->lna1_byp_gain = -5;
            rxg_params->lna1_byp_rout = 8;
        }
    }
}

static void
phy_ac_rxgcrs_populate_rxg_params_bw_specific(phy_info_t *pi)
{
    phy_ac_rxg_params_t *rxg_params = pi->u.pi_acphy->rxgcrsi->rxg_params;
    int8 val;

    if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        /* Set gain limit for TIA=15 to be used for low RSSI */
        val = (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec)) ?
            127 : 55;
        rxg_params->gainlimit_tbl[TIA_TBL_IND][7] = val;
    }
}

static void
BCMATTACHFN(phy_ac_rxgcrs_std_params_attach)(phy_ac_rxgcrs_info_t *ri)
{
    uint8 gain_len[] = {2, 6, 7, 10, 8, 8, 11}; /* elna, lna1, lna2, mix, bq0, bq1, dvga */
    uint8 i, core_num;
    phy_info_t *pi = ri->pi;

    if (TINY_RADIO(pi)) {
        gain_len[3] = 12; /* tia */
        gain_len[5] = 3;  /* farrow */
        if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) {
            gain_len[TIA_ID] = 13;
            gain_len[BQ1_ID] = 3;
            gain_len[DVGA_ID] = 15;
        }
    } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        gain_len[2] = 4;
        gain_len[3] = 16;
        gain_len[4] = 6;
        gain_len[5] = 3; /* BIQ1 - Not present */
    }

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
        ri->rxgainctrl_stage_len[i] = gain_len[i];

    ri->crsmincal_enable = TRUE;
    ri->force_crsmincal  = FALSE;

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* temporary increase by 5dB */
        ri->phy_crs_th_from_crs_cal = ACPHY_CRSMIN_DEFAULT + 13;
    } else {
        ri->phy_crs_th_from_crs_cal = ACPHY_CRSMIN_DEFAULT;
    }

    /* default clip1_th & edcrs_en */
    ri->clip1_th = 0x404e;
    ri->edcrs_en = 0xffff;
    bzero((uint8 *)ri->phy_noise_pwr_array, sizeof(ri->phy_noise_pwr_array));
    bzero((uint8 *)ri->phy_noise_in_crs_min, sizeof(ri->phy_noise_in_crs_min));
    ri->phy_debug_crscal_counter = 0;
    ri->phy_debug_crscal_channel = 0;
    ri->phy_noise_counter = 0;
    ri->lesiscalecal_enable =  TRUE;

    if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
        uint8 subband_num;
        for (subband_num = 0; subband_num <= 4; subband_num++)
            FOREACH_CORE(pi, core_num)
                ri->phy_noise_cache_crsmin[subband_num][core_num] = -30;
    } else {
        FOREACH_CORE(pi, core_num) {
            ri->phy_noise_cache_crsmin[0][core_num] = -30;
            if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
            (!(BF_ELNA_5G(ri->aci)) && ACMAJORREV_4(pi->pubpi->phy_rev))) {
                ri->phy_noise_cache_crsmin[1][core_num] = -32;
                ri->phy_noise_cache_crsmin[2][core_num] = -32;
                ri->phy_noise_cache_crsmin[3][core_num] = -31;
                ri->phy_noise_cache_crsmin[4][core_num] = -31;
            } else {
                ri->phy_noise_cache_crsmin[1][core_num] = -28;
                ri->phy_noise_cache_crsmin[2][core_num] = -28;
                ri->phy_noise_cache_crsmin[3][core_num] = -26;
                ri->phy_noise_cache_crsmin[4][core_num] = -25;
            }
        }
    }
    /* lesi */
    ri->lesi_cap = FALSE;
    ri->lesi_ovrd = -1;   /* Auto */
    ri->lesiscale_2g = 0;   /* Default LESI scale for 2G */
    ri->lesiscale_5g = 0;   /* Default LESI scale for 5G */
    ri->lesi_on = FALSE;  /* Current state of LESI */
    ri->tia_idx_max_eq_init = FALSE;

    if (ACMAJORREV_47(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev) || ACMAJORREV_129_130(pi->pubpi->phy_rev) ||
        ACMAJORREV_131(pi->pubpi->phy_rev)) {
        ri->lesi_cap = TRUE;
    } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        ri->lesi_cap = FALSE;
    }

    /* gainctrl/tia (tiny) related */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        ri->tia_idx_max_eq_init = TRUE;
        if (ACMAJORREV_33(pi->pubpi->phy_rev)) {
            /* chippkg bit-2: LESI 0: Enable ; 1: Disable */
            if ((pi->sh->chippkg & 0x4) == 0)
                ri->lesi_cap = TRUE;
        }
    }

    if (ACMAJORREV_128(pi->pubpi->phy_rev) || IS_4364_3x3(pi)) {
        ri->fixed_gain_ncal = 1;
    } else {
        ri->fixed_gain_ncal = 0;
    }
}

static void *
BCMATTACHFN(phy_ac_get_rxg_param_tbl)(phy_info_t *pi, phy_ac_rxg_param_tbl_t tbl_id,
uint8 ind_2d_tbl)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    void *lna1_rout_map_2g, *lna1_gain_map_2g, *lna12_gain_tbl_2g;
    void *lna1_rout_map_5g, *lna1_gain_map_5g, *lna12_gain_tbl_5g;
    void *lna1_rout_map_6g, *lna1_gain_map_6g, *lna12_gain_tbl_6g;

    void *lna1_rout_map_6g_hi2, *lna1_gain_map_6g_hi2, *lna12_gain_tbl_6g_hi2;
    void *lna1_rout_map_6g_hi,  *lna1_gain_map_6g_hi, *lna12_gain_tbl_6g_hi;
    void *lna1_rout_map_6g_mid, *lna1_gain_map_6g_mid, *lna12_gain_tbl_6g_mid;
    void *lna1_rout_map_6g_low, *lna1_gain_map_6g_low, *lna12_gain_tbl_6g_low;
    void *lna1_rout_map_6g_low2, *lna1_gain_map_6g_low2, *lna12_gain_tbl_6g_low2;

    if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (BF_ELNA_2G(pi_ac)) {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20709r0_elna[ind_2d_tbl];
        } else {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20709r0_ilna[ind_2d_tbl];
        }

        if (BF_ELNA_5G(pi_ac)) {
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20709r0_elna[ind_2d_tbl];
        } else {
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20709r0_ilna[ind_2d_tbl];
        }

        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return (void *)lna12_gain_tbl_2g;

            case LNA12_GAIN_TBL_5G:
                return (void *)lna12_gain_tbl_5g;

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20709r0;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20709r0;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20709r0[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20709r0[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20709r0[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return (void *)lna1_rout_map_2g_20709r0;

            case LNA1_ROUTMAP_TBL_5G:
                return (void *)lna1_rout_map_5g_20709r0;

            case LNA1_GAINMAP_TBL_2G:
                return (void *)lna1_gain_map_2g_20709r0;

            case LNA1_GAINMAP_TBL_5G:
                return (void *)lna1_gain_map_5g_20709r0;

            default:
                return NULL;
        }
    } else if (ACMAJORREV_47(pi->pubpi->phy_rev) &&
            (RADIOREV(pi->pubpi->radiorev) == 0)) {
        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return (void *)lna12_gain_tbl_2g_20698r0[ind_2d_tbl];

            case LNA12_GAIN_TBL_5G:
                return (void *)lna12_gain_tbl_5g_20698r0[ind_2d_tbl];

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20698r0;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20698r0;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20698r0[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20698r0[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20698r0[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return (void *)lna1_rout_map_2g_20698r0;

            case LNA1_ROUTMAP_TBL_5G:
                return (void *)lna1_rout_map_5g_20698r0;

            case LNA1_GAINMAP_TBL_2G:
                return (void *)lna1_gain_map_2g_20698r0;

            case LNA1_GAINMAP_TBL_5G:
                return (void *)lna1_gain_map_5g_20698r0;

            default:
                return NULL;
        }
    } else if (ACMAJORREV_47(pi->pubpi->phy_rev) &&
            (RADIOREV(pi->pubpi->radiorev) > 0)) {

        if (RADIOREV(pi->pubpi->radiorev) > 2) {
            lna1_rout_map_5g = (void *)lna1_rout_map_56g_20698r3;
            lna1_gain_map_5g = (void *)lna1_gain_map_56g_20698r3;
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20698r3[ind_2d_tbl];

            lna1_rout_map_6g = (void *)lna1_rout_map_56g_20698r3;
            lna1_gain_map_6g = (void *)lna1_gain_map_56g_20698r3;
            lna12_gain_tbl_6g = (void *)lna12_gain_tbl_6g_20698r3[ind_2d_tbl];
        } else {
            lna1_rout_map_5g = (void *)lna1_rout_map_5g_20698rX;
            lna1_gain_map_5g = (void *)lna1_gain_map_5g_20698rX;
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20698rX[ind_2d_tbl];

            lna1_rout_map_6g = NULL;
            lna1_gain_map_6g = NULL;
            lna12_gain_tbl_6g = NULL;
        }

        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return (void *)lna12_gain_tbl_2g_20698rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_5G:
                return lna12_gain_tbl_5g;

            case LNA12_GAIN_TBL_6G:
                return lna12_gain_tbl_6g;

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20698rX;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20698rX;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20698rX[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20698rX[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20698rX[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return (void *)lna1_rout_map_2g_20698rX;

            case LNA1_ROUTMAP_TBL_5G:
                return lna1_rout_map_5g;

            case LNA1_ROUTMAP_TBL_6G:
                return lna1_rout_map_6g;

            case LNA1_GAINMAP_TBL_2G:
                return (void *)lna1_gain_map_2g_20698rX;

            case LNA1_GAINMAP_TBL_5G:
                return lna1_gain_map_5g;

            case LNA1_GAINMAP_TBL_6G:
                return lna1_gain_map_6g;

            default:
                return NULL;
        }
    } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        if (BF_ELNA_2G(pi_ac)) {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20704rX_elna[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20704rX_elna;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20704rX_elna;
        } else {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20704rX_ilna[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20704rx;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20704rX;
        }
        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return lna12_gain_tbl_2g;

            case LNA12_GAIN_TBL_5G:
                return (void *)lna12_gain_tbl_5g_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW:
                return lna12_gain_tbl_6gs0_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW2:
                return lna12_gain_tbl_6gs1_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW3:
                return lna12_gain_tbl_6gs2_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_MID:
                return lna12_gain_tbl_6gs3_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI:
                return lna12_gain_tbl_6gs4_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI2:
                return lna12_gain_tbl_6gs5_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI3:
                return lna12_gain_tbl_6gs6_20704rX[ind_2d_tbl];

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20704rX;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20704rX;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20704rX[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20704rX[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20704rX[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                if (RADIOREV(pi->pubpi->radiorev) == 0) {
                    return (void *)lna1_rout_map_2g_20704r0;
                } else {
                    return (void *)lna1_rout_map_2g;
                }

            case LNA1_ROUTMAP_TBL_5G:
                return (void *)lna1_rout_map_5g_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW:
                return lna1_rout_map_6gs0_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW2:
                return lna1_rout_map_6gs1_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW3:
                return lna1_rout_map_6gs2_20704rX;

            case LNA1_ROUTMAP_TBL_6G_MID:
                return lna1_rout_map_6gs3_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI:
                return lna1_rout_map_6gs4_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI2:
                return lna1_rout_map_6gs5_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI3:
                return lna1_rout_map_6gs6_20704rX;

            case LNA1_GAINMAP_TBL_2G:
                return (void *)lna1_gain_map_2g;

            case LNA1_GAINMAP_TBL_5G:
                return (void *)lna1_gain_map_5g_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW:
                return lna1_gain_map_6gs0_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW2:
                return lna1_gain_map_6gs1_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW3:
                return lna1_gain_map_6gs2_20704rX;

            case LNA1_GAINMAP_TBL_6G_MID:
                return lna1_gain_map_6gs3_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI:
                return lna1_gain_map_6gs4_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI2:
                return lna1_gain_map_6gs5_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI3:
                return lna1_gain_map_6gs6_20704rX;

            default:
                return NULL;
        }
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        if (BF_ELNA_2G(pi_ac)) {
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20707rX_elna;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20707rX_elna;
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20707rX_elna[ind_2d_tbl];
        } else {
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20707rX_ilna;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20707rX_ilna;
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20707rX_ilna[ind_2d_tbl];
        }

        if (BF_ELNA_5G(pi_ac)) {
            lna1_rout_map_5g = (void *)lna1_rout_map_5g_20707rX_elna;
            lna1_gain_map_5g = (void *)lna1_gain_map_5g_20707rX_elna;
            lna12_gain_tbl_5g = (void *)
                lna12_gain_tbl_5g_20707rX_elna[ind_2d_tbl];

            lna1_rout_map_6g_low = (void *)lna1_rout_map_6g_low_20707rX_elna;
            lna1_gain_map_6g_low = (void *)lna1_gain_map_6g_low_20707rX_elna;
            lna12_gain_tbl_6g_low = (void *)
                lna12_gain_tbl_6g_20707rX_elna[ind_2d_tbl];

            lna1_rout_map_6g_mid = (void *)lna1_rout_map_6g_mid_20707rX_elna;
            lna1_gain_map_6g_mid = (void *)lna1_gain_map_6g_mid_20707rX_elna;
            lna12_gain_tbl_6g_mid = (void *)
                 lna12_gain_tbl_6g_20707rX_elna[ind_2d_tbl];

            lna1_rout_map_6g_hi = (void *)lna1_rout_map_6g_hi_20707rX_elna;
            lna1_gain_map_6g_hi = (void *)lna1_gain_map_6g_hi_20707rX_elna;
            lna12_gain_tbl_6g_hi = (void *)
                 lna12_gain_tbl_6g_20707rX_elna[ind_2d_tbl];

            lna1_rout_map_6g_hi2 = (void *)lna1_rout_map_6g_hi2_20707rX_elna;
            lna1_gain_map_6g_hi2 = (void *)lna1_gain_map_6g_hi2_20707rX_elna;
            lna12_gain_tbl_6g_hi2 = (void *)
                 lna12_gain_tbl_6g_20707rX_elna[ind_2d_tbl];
        } else {
            lna1_rout_map_5g = (void *)lna1_rout_map_5g_20707rX_ilna;
            lna1_gain_map_5g = (void *)lna1_gain_map_5g_20707rX_ilna;
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20707rX_ilna[ind_2d_tbl];

            lna12_gain_tbl_6g_hi2 = NULL;
            lna12_gain_tbl_6g_hi = NULL;
            lna12_gain_tbl_6g_mid = NULL;
            lna12_gain_tbl_6g_low = NULL;
        }

        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return lna12_gain_tbl_2g;

            case LNA12_GAIN_TBL_5G:
                return lna12_gain_tbl_5g;

            case LNA12_GAIN_TBL_6G_LOW:
                return lna12_gain_tbl_6g_low;

            case LNA12_GAIN_TBL_6G_MID:
                return lna12_gain_tbl_6g_mid;

            case LNA12_GAIN_TBL_6G_HI:
                return lna12_gain_tbl_6g_hi;

            case LNA12_GAIN_TBL_6G_HI2:
                return lna12_gain_tbl_6g_hi2;

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20707rX;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20707rX;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20707rX[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20707rX[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20707rX[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return lna1_rout_map_2g;

            case LNA1_ROUTMAP_TBL_5G:
                return lna1_rout_map_5g;

            case LNA1_ROUTMAP_TBL_6G_LOW:
                return lna1_rout_map_6g_low;

            case LNA1_ROUTMAP_TBL_6G_MID:
                return lna1_rout_map_6g_mid;

            case LNA1_ROUTMAP_TBL_6G_HI:
                return lna1_rout_map_6g_hi;

            case LNA1_ROUTMAP_TBL_6G_HI2:
                return lna1_rout_map_6g_hi2;

            case LNA1_GAINMAP_TBL_2G:
                return lna1_gain_map_2g;

            case LNA1_GAINMAP_TBL_5G:
                return lna1_gain_map_5g;

            case LNA1_GAINMAP_TBL_6G_LOW:
                return lna1_gain_map_6g_low;

            case LNA1_GAINMAP_TBL_6G_MID:
                return lna1_gain_map_6g_mid;

            case LNA1_GAINMAP_TBL_6G_HI:
                return lna1_gain_map_6g_hi;

            case LNA1_GAINMAP_TBL_6G_HI2:
                return lna1_gain_map_6g_hi2;

            default:
                return NULL;
        }
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
    if (RADIOREV(pi->pubpi->radiorev) < 2) {
        lna1_rout_map_2g = (void *)lna1_rout_map_2g_20708r0;
        lna1_gain_map_2g = (void *)lna1_gain_map_2g_20708r0;
        lna1_rout_map_5g = (void *)lna1_rout_map_5g_20708r0;
        lna1_gain_map_5g = (void *)lna1_gain_map_5g_20708r0;
        lna1_rout_map_6g = (void *)lna1_rout_map_6g_20708r0;
        lna1_gain_map_6g = (void *)lna1_gain_map_6g_20708r0;

        lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20708r0[ind_2d_tbl];
        lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20708r0[ind_2d_tbl];
        lna12_gain_tbl_6g = (void *)lna12_gain_tbl_6g_20708r0[ind_2d_tbl];

        lna1_rout_map_6g_hi2 = NULL;
        lna1_rout_map_6g_hi = NULL;
        lna1_rout_map_6g_mid = NULL;
        lna1_rout_map_6g_low2 = NULL;
        lna1_rout_map_6g_low = NULL;
        lna1_gain_map_6g_hi2 = NULL;
        lna1_gain_map_6g_hi = NULL;
        lna1_gain_map_6g_mid = NULL;
        lna1_gain_map_6g_low2 = NULL;
        lna1_gain_map_6g_low = NULL;
        lna12_gain_tbl_6g_hi2 = NULL;
        lna12_gain_tbl_6g_hi = NULL;
        lna12_gain_tbl_6g_mid = NULL;
        lna12_gain_tbl_6g_low2 = NULL;
        lna12_gain_tbl_6g_low = NULL;
    } else {
        if (pi->sromi->sr18_ilna_tbl == 1) {
            // mch2
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20708r2_ilnatbl1[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20708r2_ilnatbl1;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20708r2_ilnatbl1;
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20708r2_ilnatbl1[ind_2d_tbl];
            lna1_rout_map_5g = (void *)lna1_rout_map_5g_20708r2_ilnatbl1;
            lna1_gain_map_5g = (void *)lna1_gain_map_5g_20708r2_ilnatbl1;
        } else {
            // mch5
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20708r2[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20708r2;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20708r2;
            lna12_gain_tbl_5g = (void *)lna12_gain_tbl_5g_20708r2[ind_2d_tbl];
            lna1_rout_map_5g = (void *)lna1_rout_map_5g_20708r2;
            lna1_gain_map_5g = (void *)lna1_gain_map_5g_20708r2;
        }

        lna1_rout_map_6g_low = (void *)lna1_rout_map_6g_low_20708r2;
        lna1_gain_map_6g_low = (void *)lna1_gain_map_6g_low_20708r2;
        lna1_rout_map_6g_low2 = (void *)lna1_rout_map_6g_low2_20708r2;
        lna1_gain_map_6g_low2 = (void *)lna1_gain_map_6g_low2_20708r2;
        lna1_rout_map_6g_mid = (void *)lna1_rout_map_6g_mid_20708r2;
        lna1_gain_map_6g_mid = (void *)lna1_gain_map_6g_mid_20708r2;
        lna1_rout_map_6g_hi = (void *)lna1_rout_map_6g_hi_20708r2;
        lna1_gain_map_6g_hi = (void *)lna1_gain_map_6g_hi_20708r2;
        lna1_rout_map_6g_hi2 = (void *)lna1_rout_map_6g_hi2_20708r2;
        lna1_gain_map_6g_hi2 = (void *)lna1_gain_map_6g_hi2_20708r2;
        lna12_gain_tbl_6g_low = (void *)lna12_gain_tbl_6g_low_20708r2[ind_2d_tbl];
        lna12_gain_tbl_6g_low2 = (void *)lna12_gain_tbl_6g_low2_20708r2[ind_2d_tbl];
        lna12_gain_tbl_6g_mid = (void *)lna12_gain_tbl_6g_mid_20708r2[ind_2d_tbl];
        lna12_gain_tbl_6g_hi = (void *)lna12_gain_tbl_6g_hi_20708r2[ind_2d_tbl];
        lna12_gain_tbl_6g_hi2 = (void *)lna12_gain_tbl_6g_hi2_20708r2[ind_2d_tbl];

        lna1_rout_map_6g = NULL;
        lna1_gain_map_6g = NULL;
        lna12_gain_tbl_6g = NULL;
    }

        switch (tbl_id) {
            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20708r0[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20708r0[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20708r0[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return lna1_rout_map_2g;

            case LNA1_ROUTMAP_TBL_5G:
                return lna1_rout_map_5g;

            case LNA1_ROUTMAP_TBL_6G:
                return lna1_rout_map_6g;

            case LNA1_ROUTMAP_TBL_6G_LOW:
                return lna1_rout_map_6g_low;

            case LNA1_ROUTMAP_TBL_6G_LOW2:
                return lna1_rout_map_6g_low2;

            case LNA1_ROUTMAP_TBL_6G_MID:
                return lna1_rout_map_6g_mid;

            case LNA1_ROUTMAP_TBL_6G_HI:
                return lna1_rout_map_6g_hi;

            case LNA1_ROUTMAP_TBL_6G_HI2:
                return lna1_rout_map_6g_hi2;

            case LNA1_GAINMAP_TBL_2G:
                return lna1_gain_map_2g;

            case LNA1_GAINMAP_TBL_5G:
                return lna1_gain_map_5g;

            case LNA1_GAINMAP_TBL_6G:
                return lna1_gain_map_6g;

            case LNA1_GAINMAP_TBL_6G_LOW:
                return lna1_gain_map_6g_low;

            case LNA1_GAINMAP_TBL_6G_LOW2:
                return lna1_gain_map_6g_low2;

            case LNA1_GAINMAP_TBL_6G_MID:
                return lna1_gain_map_6g_mid;

            case LNA1_GAINMAP_TBL_6G_HI:
                return lna1_gain_map_6g_hi;

            case LNA1_GAINMAP_TBL_6G_HI2:
                return lna1_gain_map_6g_hi2;

            case LNA12_GAIN_TBL_2G:
                return lna12_gain_tbl_2g;

            case LNA12_GAIN_TBL_5G:
                return lna12_gain_tbl_5g;

            case LNA12_GAIN_TBL_6G:
                return lna12_gain_tbl_6g;

            case LNA12_GAIN_TBL_6G_LOW:
                return lna12_gain_tbl_6g_low;

            case LNA12_GAIN_TBL_6G_LOW2:
                return lna12_gain_tbl_6g_low2;

            case LNA12_GAIN_TBL_6G_MID:
                return lna12_gain_tbl_6g_mid;

            case LNA12_GAIN_TBL_6G_HI:
                return lna12_gain_tbl_6g_hi;

            case LNA12_GAIN_TBL_6G_HI2:
                return lna12_gain_tbl_6g_hi2;

            case TIA_GAIN_TBL:
                if (RADIOREV(pi->pubpi->radiorev) < 2) {
                    return (void *)tia_gain_tbl_20708r0;
                } else {
                    return (void *)tia_gain_tbl_20708r2;
                }

            case TIA_GAIN_BITS_TBL:
                if (RADIOREV(pi->pubpi->radiorev) < 2) {
                    return (void *)tia_gainbits_tbl_20708r0;
                } else {
                    return (void *)tia_gainbits_tbl_20708r2;
                }

            default:
                return NULL;
        }
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        if (BF_ELNA_2G(pi_ac)) {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20704rX_elna[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20704rX_elna;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20704rX_elna;
        } else {
            lna12_gain_tbl_2g = (void *)lna12_gain_tbl_2g_20704rX_ilna[ind_2d_tbl];
            lna1_rout_map_2g = (void *)lna1_rout_map_2g_20704rx;
            lna1_gain_map_2g = (void *)lna1_gain_map_2g_20704rX;
        }
        switch (tbl_id) {
            case LNA12_GAIN_TBL_2G:
                return lna12_gain_tbl_2g;

            case LNA12_GAIN_TBL_5G:
                return (void *)lna12_gain_tbl_5g_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW:
                return lna12_gain_tbl_6gs0_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW2:
                return lna12_gain_tbl_6gs1_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_LOW3:
                return lna12_gain_tbl_6gs2_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_MID:
                return lna12_gain_tbl_6gs3_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI:
                return lna12_gain_tbl_6gs4_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI2:
                return lna12_gain_tbl_6gs5_20704rX[ind_2d_tbl];

            case LNA12_GAIN_TBL_6G_HI3:
                return lna12_gain_tbl_6gs6_20704rX[ind_2d_tbl];

            case TIA_GAIN_TBL:
                return (void *)tia_gain_tbl_20710rX;

            case TIA_GAIN_BITS_TBL:
                return (void *)tia_gainbits_tbl_20710rX;

            case BIQ01_GAIN_TBL:
                return (void *)biq01_gain_tbl_20704rX[ind_2d_tbl];

            case BIQ01_GAIN_BITS_TBL:
                return (void *)biq01_gainbits_tbl_20704rX[ind_2d_tbl];

            case GAIN_LIMIT_TBL:
                return (void *)gainlimit_tbl_20710rX[ind_2d_tbl];

            case LNA1_ROUTMAP_TBL_2G:
                return (void *)lna1_rout_map_2g;

            case LNA1_ROUTMAP_TBL_5G:
                return (void *)lna1_rout_map_5g_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW:
                return lna1_rout_map_6gs0_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW2:
                return lna1_rout_map_6gs1_20704rX;

            case LNA1_ROUTMAP_TBL_6G_LOW3:
                return lna1_rout_map_6gs2_20704rX;

            case LNA1_ROUTMAP_TBL_6G_MID:
                return lna1_rout_map_6gs3_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI:
                return lna1_rout_map_6gs4_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI2:
                return lna1_rout_map_6gs5_20704rX;

            case LNA1_ROUTMAP_TBL_6G_HI3:
                return lna1_rout_map_6gs6_20704rX;

            case LNA1_GAINMAP_TBL_2G:
                return (void *)lna1_gain_map_2g;

            case LNA1_GAINMAP_TBL_5G:
                return (void *)lna1_gain_map_5g_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW:
                return lna1_gain_map_6gs0_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW2:
                return lna1_gain_map_6gs1_20704rX;

            case LNA1_GAINMAP_TBL_6G_LOW3:
                return lna1_gain_map_6gs2_20704rX;

            case LNA1_GAINMAP_TBL_6G_MID:
                return lna1_gain_map_6gs3_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI:
                return lna1_gain_map_6gs4_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI2:
                return lna1_gain_map_6gs5_20704rX;

            case LNA1_GAINMAP_TBL_6G_HI3:
                return lna1_gain_map_6gs6_20704rX;

            default:
                return NULL;
        }
    } else {
        return NULL;
    }
}

//static void
//BCMATTACHFN(wlc_phy_nvram_dyn_ed_read)(phy_info_t *pi)
//{
//    /* Read the phy_ac_dynamic_th from NVRAM */
//    pi->acphy_for_dynamic_ed = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_acphy_for_dynamic_ed, 0);
//}

void
wlc_dynamic_ed_thresh_init_const(phy_info_t *pi)
{
    if (pi->const_dy_ed_initialized) {
        return;
    }
    if (pi->const_sed_disable == 0) {pi->const_sed_disable = DYN_ED_SED_DISABLE;}
    if (pi->const_ed_monitor_window == 0) {pi->const_ed_monitor_window = DYN_ED_WINDOW;}
    if (pi->const_sed_upper_bound == 0) {pi->const_sed_upper_bound = DYN_ED_SED_UPPER_BOUND;}
    if (pi->const_sed_lower_bound == 0) {pi->const_sed_lower_bound = DYN_ED_SED_LOWER_BOUND;}
    if (pi->const_ed_th_high == 0) {pi->const_ed_th_high = DYN_ED_THRESH_HIGH;}
    if (pi->const_ed_th_low == 0) {pi->const_ed_th_low = DYN_ED_THRESH_LOW;}
    if (pi->const_ed_inc_step == 0) {pi->const_ed_inc_step = DYN_ED_INC_STEP;}
    if (pi->const_ed_dec_step == 0) {pi->const_ed_dec_step = DYN_ED_DEC_STEP;}

    pi->const_dy_ed_initialized = TRUE;
    return;
}

/* Dynamic ED algorithm
 * The feature is called every second in a watchdog. It monitors the media for
 * "const_ed_monitor_window cycles" (e.g. 3cycles=3sec). It measures SED
 * (significant energy detected) ratio.
 * If SED is larger than "const_sed_upper_bound" (eg.40%) then it starts to increase
 * the "assert ED threshold" by const_ed_inc_step (e.g. 1dB) up to "const_ed_th_high".
 * and if the SED goes below "const_sed_lower_bound" the threshold will be decreased
 * by "const_ed_dec_step" (till we reach "const_ed_th_low")
 * to avoid unnecessary threshold modification.
 * If SED is too high (larger than "const_sed_disable"), the feature will be disabled;
 * i.e. the threshold will be reset to the static edcrs threshold
 */
void
wlc_dynamic_ed_thresh(phy_info_t *pi)
{
    uint16 lowRegRead, highRegRead;
    uint32 edCount;
    int32 adj_ed_thresh, curr_ed_thresh, curr_static_ed_thresh;
    bool edcrs_flag;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    wlc_dynamic_ed_thresh_init_const(pi);

    /* Reading Reg to see if edcrs is set or not */
    edcrs_flag = wlc_edcrs_enabled(pi);

#ifdef CONFIG_TENDA_PRIVATE_WLAN
    uint8 txop;
    curr_ed_thresh = phy_ac_noise_get_dynamic_ed_thresh(pi, TRUE, &txop);
#else
    curr_ed_thresh = phy_ac_noise_get_dynamic_ed_thresh(pi, TRUE);
#endif /* CONFIG_TENDA_PRIVATE_WLAN */

    PHY_ACI(("%s: curr_ed_thresh is %d\n",__FUNCTION__, curr_ed_thresh));

    wlc_phy_adjust_ed_thres_acphy(pi_ac->rxgcrsi, &curr_static_ed_thresh, FALSE);
    PHY_ACI(("%s: curr_static_ed_thresh is %d\n",__FUNCTION__, curr_static_ed_thresh));

    if ((curr_ed_thresh == 0) || (curr_ed_thresh != curr_static_ed_thresh)) {
        curr_ed_thresh = curr_static_ed_thresh;
        phy_ac_noise_set_dynamic_ed_thresh(pi, curr_ed_thresh, TRUE);
        PHY_ACI(("%s: Reset dynamic edcrs threshold as static edcrs threshold %d \n",__FUNCTION__, curr_ed_thresh));
    }

    pi->ed_count++;

    lowRegRead = wlapi_bmac_read_shm(pi->sh->physhim, M_CCA_EDCRSDUR_L(pi));
    highRegRead = wlapi_bmac_read_shm(pi->sh->physhim, M_CCA_EDCRSDUR_H(pi));

    edCount = lowRegRead+ (highRegRead * 65536);
    /* when counter overflow, do not make any decision... just update the counters */
    if ((pi->ed_count >= pi->const_ed_monitor_window) || (edCount < pi->ed_counter)) {
        if (edCount >= pi->ed_counter) {
            pi->sed = (edCount - pi->ed_counter) / pi->const_ed_monitor_window /10000;
            pi->sed = (pi->sed >= 100) ? 101 : pi->sed; // cap the sed by 100%
            adj_ed_thresh = curr_ed_thresh;

            if (pi->sed >= pi->const_sed_disable ||
                !pi->dynamic_ed_thresh_enable || !edcrs_flag) {
                /* If there is a high SED ratio (i.e. EU TEST going on) or
                 * the feature is turned off using WL command, then
                 * reset to the static edcrs threshold
                 */
                if (!pi->dynamic_ed_thresh_enable || !edcrs_flag) {
                    PHY_ACI(("%s: dynamic_ed_thresh_enable = ", __FUNCTION__));
                    PHY_ACI(("%d. edcrs_flag = %d\n",
                        pi->dynamic_ed_thresh_enable, edcrs_flag));
                } else {
                    PHY_ACI(("%s: SED is %d\n", __FUNCTION__, pi->sed));
                }

                PHY_ACI(("Reset as static edcrs threshold\n"));
                wlc_phy_apply_edcrs_acphy(pi);

                wlc_phy_adjust_ed_thres_acphy(pi_ac->rxgcrsi,
                    &adj_ed_thresh, FALSE);
            } else {
#ifdef CONFIG_TENDA_PRIVATE_WLAN
                if (txop <= 10) {    /* txop <= 10, Current environmental interference is serious */
                    PHY_ACI(("%s: TXOP=%d <= 10. Increasing the threshold.\n",
                        __FUNCTION__, txop));
                    adj_ed_thresh = curr_ed_thresh + pi->const_ed_inc_step * 2;
                    PHY_ACI(("%s: SED is %d adj_ed_thresh=%d\n",
                        __FUNCTION__, pi->sed, adj_ed_thresh));
                } else {
#endif /* CONFIG_TENDA_PRIVATE_WLAN */
                if (pi->sed > pi->const_sed_upper_bound) {
                    PHY_ACI(("%s: SED is %d. Increasing the threshold.\n",
                        __FUNCTION__, pi->sed));
                    adj_ed_thresh = curr_ed_thresh + pi->const_ed_inc_step;
                } else if (pi->sed < pi->const_sed_lower_bound) {
                    PHY_ACI(("%s: SED is %d. Decreasing the threshold.\n",
                        __FUNCTION__, pi->sed));
                    adj_ed_thresh = curr_ed_thresh - pi->const_ed_dec_step;
                }
#ifdef CONFIG_TENDA_PRIVATE_WLAN
                }
#endif /* CONFIG_TENDA_PRIVATE_WLAN */

                if (adj_ed_thresh > pi->const_ed_th_high) {
                    PHY_ACI(("%s: ED threshold %d is beyond higher limit %d\n",
                        __FUNCTION__, adj_ed_thresh, pi->const_ed_th_high));
                    adj_ed_thresh = pi->const_ed_th_high;
                } else if (adj_ed_thresh < pi->const_ed_th_low) {
                    PHY_ACI(("%s: ED threshold %d is beyond lower limit %d\n",
                        __FUNCTION__, adj_ed_thresh, pi->const_ed_th_low));
                    adj_ed_thresh = pi->const_ed_th_low;
                }
            }

            // change edcrs threshold and store latest in cache
            if (adj_ed_thresh != curr_ed_thresh) {
                PHY_ACI(("%s: ED threshold change to %d\n",
                    __FUNCTION__, adj_ed_thresh));
                wlc_phy_adjust_ed_thres_acphy(pi_ac->rxgcrsi, &adj_ed_thresh, TRUE);
                phy_ac_noise_set_dynamic_ed_thresh(pi, adj_ed_thresh, TRUE);
            }

        }

        pi->ed_counter = edCount;
        pi->ed_count = 0;
    }
}

bool
wlc_edcrs_enabled(phy_info_t *pi)
{
    uint16 val;
    if (D11REV_GE(pi->sh->corerev, 40))
        val = wlapi_bmac_read_shm(pi->sh->physhim, M_IFS_PRICRS(pi));
    else
        val = wlapi_bmac_read_shm(pi->sh->physhim, M_IFSCTL1(pi));
    return (val & IFS_CTL_ED_SEL_MASK) ? TRUE : FALSE;
}
int
wlc_phy_update_ed_thres(phy_info_t *pi, int32 *assert_thresh_dbm, bool set_threshold)
{
    int err = BCME_OK;
    err = wlc_phy_adjust_ed_thres(pi, assert_thresh_dbm, set_threshold);
    return err;
}

void wlc_dynamic_ed_thresh_enable(phy_info_t *pi, bool set_dynamic_ed_th)
{
    pi->dynamic_ed_thresh_enable = set_dynamic_ed_th;
    return;
}
void
wlc_dynamic_ed_th_overwrite_mon_win(phy_info_t *pi, int val)
{
    pi->const_ed_monitor_window = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_sed_dis(phy_info_t *pi, int val)
{
    pi->const_sed_disable = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_sed_high(phy_info_t *pi, int val)
{
    pi->const_sed_upper_bound = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_sed_low(phy_info_t *pi, int val)
{
    pi->const_sed_lower_bound = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_th_high(phy_info_t *pi, int val)
{
    pi->const_ed_th_high = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_th_low(phy_info_t *pi, int val)
{
    pi->const_ed_th_low = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_step_inc(phy_info_t *pi, int val)
{
    pi->const_ed_inc_step = val;
    return;
}
void
wlc_dynamic_ed_th_overwrite_step_dec(phy_info_t *pi, int val)
{
    pi->const_ed_dec_step = val;
    return;
}

void
wlc_dynamic_ed_th_overwrite_acphy(phy_info_t *pi, int val)
{
    pi->acphy_for_dynamic_ed = val;
    return;
}

void
phy_ac_rxgcrs_set_desense(phy_ac_rxgcrs_info_t *rxgcrs_info, acphy_desense_values_t *desense,
    phy_ac_desense_type_t type)
{
    acphy_desense_values_t *local_desense;
    PHY_TRACE(("%s\n", __FUNCTION__));
    switch (type) {
        case LTE_DESENSE:
            local_desense = rxgcrs_info->lte_desense;
            break;
        case BT_DESENSE:
            local_desense = rxgcrs_info->bt_desense;
            break;
        case CURR_DESENSE:
            local_desense = rxgcrs_info->curr_desense;
            break;
        case ZERO_DESENSE:
            local_desense = rxgcrs_info->zero_desense;
            break;
        case TOTAL_DESENSE:
            local_desense = rxgcrs_info->total_desense;
            break;
        default:
            PHY_ERROR(("Unsupported desense type\n"));
            return;
    }
    memcpy(local_desense, desense, sizeof(acphy_desense_values_t));
}

acphy_desense_values_t
phy_ac_rxgcrs_get_desense(phy_ac_rxgcrs_info_t *rxgcrs_info, phy_ac_desense_type_t type)
{
    acphy_desense_values_t local_desense, *prxgcrs_desense = NULL;
    PHY_TRACE(("%s\n", __FUNCTION__));
    switch (type) {
        case CURR_DESENSE:
            prxgcrs_desense = rxgcrs_info->curr_desense;
            break;
        case ZERO_DESENSE:
            prxgcrs_desense = rxgcrs_info->zero_desense;
            break;
        case TOTAL_DESENSE:
            prxgcrs_desense = rxgcrs_info->total_desense;
            break;
        default:
            PHY_ERROR(("Unsupported desense type\n"));
    }
    if (prxgcrs_desense != NULL) {
        memcpy(&local_desense, prxgcrs_desense, sizeof(acphy_desense_values_t));
    } else {
        bzero(&local_desense, sizeof(acphy_desense_values_t));
    }
    return local_desense;
}

#if defined(BCMDBG) || defined(WLTEST)
void
phy_ac_rxgcrs_cal_dump(phy_ac_rxgcrs_info_t *rxgcrsi, struct bcmstrbuf *b)
{
    uint8 core;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(rxgcrsi->pi->stfi);

    BCM_REFERENCE(stf_shdata);

    bcm_bprintf(b, "crs_min_pwr cal:\n");
    bcm_bprintf(b, "   crsmin_cal ran %d times for channel %d:\n",
                rxgcrsi->phy_debug_crscal_counter,
                rxgcrsi->phy_debug_crscal_channel);

    bcm_bprintf(b, "   Noise power used for setting crs_min thresholds : ");
    FOREACH_ACTV_CORE(rxgcrsi->pi, stf_shdata->phyrxchain, core) {
        bcm_bprintf(b, "Core-%d : %d, ", core, rxgcrsi->phy_noise_in_crs_min[core]);
    }
}
#endif

/* ********************************************* */
/*                Internal Definitions                    */
/* ********************************************* */
static const uint8 ac_lna1_2g[]       = {0xf6, 0xff, 0x6, 0xc, 0x12, 0x19};
static const uint8 ac_lna1_2g_tiny[]    = {0xfa, 0x00, 0x6, 0xc, 0x12, 0x18};
static const uint8 ac_tiny_g_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 ac_tiny_g_lna_gain_map[] = {2, 3, 4, 5, 5, 5};
static const uint8 ac_tiny_a_lna_rout_map[] = {11, 11, 11, 11, 7, 0};
static const uint8 ac_tiny_a_lna_gain_map[] = {4, 5, 6, 7, 7, 7};
static const uint8 ac_4365_a_lna_rout_map[] = {9, 9, 9, 9, 6, 0};
static const uint8 ac_lna1_5g_tiny[]    = {0xfb, 0x00, 0x6, 0xc, 0x12, 0x18};
static const uint8 ac_lna1_5g_4365[]    = {0xfe, 0x03, 0x9, 0x10, 0x14, 0x18};
static const uint8 ac_lna1_5g[] = {0xf9, 0xfe, 0x4, 0xa, 0x10, 0x17};
static const uint8 ac_lna1_2g_43352_ilna[]    = {0xff, 0xff, 0x6, 0xc, 0x12, 0x19};
static const uint8 ac_lna2_2g_43352_ilna[] = {0xfa, 0xfa, 0xfe, 0x01, 0x4, 0x7, 0xb};
static const uint8 ac_lna2_5g[] = {0xf5, 0xf8, 0xfb, 0xfe, 0x2, 0x5, 0x9};
static const uint8 ac_lna2_2g_gm2[] = {0xf4, 0xf8, 0xfc, 0xff, 0x2, 0x5, 0x9};
static const uint8 ac_lna2_2g_gm3[] = {0xf6, 0xfa, 0xfe, 0x01, 0x4, 0x7, 0xb};
static const uint8 ac_lna2_tiny[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static const uint8 ac_lna2_tiny_4349[] = {0xEE, 0xf4, 0xfa, 0x0, 0x0, 0x0, 0x0};
static const uint8 ac_lna2_tiny_4365_2g[] = {0xf4, 0xfa, 0, 6, 6, 6, 6};
static const uint8 ac_lna2_tiny_4365_5g[] = {0xf1, 0xf7, 0xfd, 3, 3, 3, 3};
static const uint8 ac_lna1_rout_delta_2g[] = {0, 1, 2, 3, 5, 7, 8, 11, 13, 15, 18, 20};
static const uint8 ac_lna1_rout_delta_5g[] = {10, 7, 4, 2, 0};
#ifndef WLC_DISABLE_ACI
static void wlc_phy_desense_mf_high_thresh_acphy(phy_info_t *pi, bool on);
static void wlc_phy_desense_print_phyregs_acphy(phy_info_t *pi, const char str[]);
#endif

static int8 wlc_phy_rxgainctrl_calc_low_sens_acphy(phy_info_t *pi, int8 clipgain,
    bool trtx, bool lna1byp, uint8 core);
static void wlc_phy_set_analog_rxgain(phy_info_t *pi, uint8 clipgain,
    uint8 *gain_idx, bool trtx, uint8 core);
static int8 wlc_phy_rxgainctrl_calc_high_sens_acphy(phy_info_t *pi, int8 clipgain,
    bool trtx, bool lna1byp, uint8 core);
static uint8 wlc_phy_rxgainctrl_set_init_clip_gain_acphy(phy_info_t *pi, uint8 clipgain,
    int8 gain_dB, bool trtx, bool lna1byp, uint8 core);
static void wlc_phy_rxgainctrl_nbclip_acphy(phy_info_t *pi, uint8 core,
    int8 rxpwr_dBm);
static void wlc_phy_rxgainctrl_w1clip_acphy(phy_info_t *pi, uint8 core,
    int8 rxpwr_dBm);
#ifndef WLC_DISABLE_ACI
static void wlc_phy_set_crs_min_pwr_higain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only);
static void wlc_phy_set_crs_min_pwr_mdgain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only);
static void wlc_phy_set_crs_min_pwr_logain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only);
static void wlc_phy_set_crs_min_pwr_clip2gain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only);
#endif
static void wlc_phy_limit_rxgaintbl_acphy(uint8 gaintbl[], uint8 gainbitstbl[], uint8 sz,
    const uint8 default_gaintbl[], uint8 min_idx, uint8 max_idx);
static void wlc_phy_rxgainctrl_nbclip_acphy_tiny(phy_info_t *pi, uint8 core, int8 rxpwr_dBm);
#define ACPHY_NUM_NB_THRESH 8
#define ACPHY_NUM_NB_THRESH_TINY 9
#define ACPHY_NUM_W1_THRESH 12

#define NB_LOW 0
#define NB_MID 1
#define NB_HIGH 2
#ifndef WLC_DISABLE_ACI

/*
Default - High MF thresholds are used only if pktgain < 81dB.
To always use high mf thresholds, change this to 98dBs
*/
static void
wlc_phy_desense_mf_high_thresh_acphy(phy_info_t *pi, bool on)
{
    uint16 val;
    uint8 core;

    if (on) {
        val = 0x5f62;
    } else {
        val = (TINY_RADIO(pi) || ACMAJORREV_GE37(pi->pubpi->phy_rev)) ? 0x454b : 0x4e51;
    }
    if (ACREV_GE(pi->pubpi->phy_rev, 32)) {
        if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) {
            /* Use default values for now */
        } else {
            FOREACH_CORE(pi, core) {
                WRITE_PHYREGCE(pi, crshighlowpowThresholdl, core, val);
                WRITE_PHYREGCE(pi, crshighlowpowThresholdu, core, val);
                WRITE_PHYREGCE(pi, crshighlowpowThresholdlSub1, core, val);
                WRITE_PHYREGCE(pi, crshighlowpowThresholduSub1, core, val);

                if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
                    WRITE_PHYREGCE(pi, crshighlowpowThreshold_4_, core, val);
                    WRITE_PHYREGCE(pi, crshighlowpowThreshold_5_, core, val);
                    WRITE_PHYREGCE(pi, crshighlowpowThreshold_6_, core, val);
                    WRITE_PHYREGCE(pi, crshighlowpowThreshold_7_, core, val);
                }
            }
        }
    } else {
        WRITE_PHYREG(pi, crshighlowpowThresholdl, val);
        WRITE_PHYREG(pi, crshighlowpowThresholdu, val);
        WRITE_PHYREG(pi, crshighlowpowThresholdlSub1, val);
        WRITE_PHYREG(pi, crshighlowpowThresholduSub1, val);
    }
}

static void
wlc_phy_desense_print_phyregs_acphy(phy_info_t *pi, const char str[])
{
}

#endif /* #ifndef WLC_DISABLE_ACI */

void wlc_phy_get_rxgain_acphy(phy_info_t *pi, rxgain_t rxgain[], int16 *tot_gain,
                              uint8 force_gain_type)
{
    uint8  core, bw_idx, ant, core_freq_segment_map;
    uint16 code_A, code_B, gain_tblid, stall_val;
    int8   gain_dvga, gain_bq0, gain_bq1, gain_lna1, gain_lna2, gain_mix, tr_loss = 0;
    int8   elna_gain[PHY_CORE_MAX], lna1_gain_corr[PHY_CORE_MAX];
    int8   LNA1_GAINSTEP_ERR[CH_5G_4BAND] = {1, 2, 2, 2}; /* 5g:l,ml,mu,h */
    bool   elna_present, tr_tx;
    int8   subband_idx;
    bool lna1byp;
    uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
    /* Mclip gain */
    bool mclip_agc_en;
    uint32 mclip_gaincode;
    int8   mclip_gainidx;

    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    lna1byp = CHSPEC_IS2G(pi->radio_chanspec) ?
        ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
        & BFL2_LNA1BYPFORTR2G) != 0) :
        ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
        & BFL2_LNA1BYPFORTR5G) != 0);

    bzero(elna_gain, sizeof(elna_gain));
    (void)memset(lna1_gain_corr, 0, sizeof(lna1_gain_corr));

    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        elna_present = BF_ELNA_2G(pi_ac);
        bw_idx = (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
    } else {
        elna_present = BF_ELNA_5G(pi_ac);
        bw_idx = (CHSPEC_IS80(pi->radio_chanspec) ||
                PHY_AS_80P80(pi, pi->radio_chanspec)) ? 2 :
                (CHSPEC_IS160(pi->radio_chanspec)) ? 3 :
                (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
    }
    /* 43602 China Spur WAR Gain Boosting for PAD */
    if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) && ACMAJORREV_5(pi->pubpi->phy_rev) &&
        (pi->sromi->dBpad)) {
        /* Boost Core 2 radio gain */
        MOD_RADIO_REGC(pi, TXMIX5G_CFG1, 2, gainboost, 0x9);
        MOD_RADIO_REGC(pi, PGA5G_CFG1, 2, gainboost, 0x7);
    }

    mclip_agc_en = phy_ac_rxgcrs_get_cap_mclip_agc_en(pi);

    FOREACH_CORE(pi, core) {
        if (force_gain_type == 4) {
            if (mclip_agc_en) {
                /* Use mclip gaincode with elna off */
                /* For REV40, use lna index of 5 (same as in initgain) */
                /* To avoid lna gain step delta affecting tr offset measurement */
                /* {0 5 3 0 0 2 0} tr = 1 */
                mclip_gaincode = 0x410075;
                /* construct code_A,B to reuse existing code and for consistency */
                code_A  =  (mclip_gaincode >> 1)&0x7ff;
                code_B  =  ((mclip_gaincode >> 23)&0x1)<<1 |
                    ((!mclip_gaincode & 0x1))<<2 | (mclip_gaincode & 0x1)<<3 |
                    ((mclip_gaincode >> 12)&0x7)<<4 |
                    ((mclip_gaincode >> 15)&0x7)<<8 |
                    ((mclip_gaincode >> 18)&0xf)<<12;
            }
            else {
                code_A  =  READ_PHYREGC(pi, cliploGainCodeA, core);
                code_B   = READ_PHYREGC(pi, cliploGainCodeB, core);
            }
        } else if (force_gain_type == 3) {
            if (mclip_agc_en) {
                /* Use mclip gaincode with lna index 3 */
                /* REV40: in case lna1 offset delta between idx 5 and 3 */
                /* Use force_gain_type 3 to calibation and compensate the delta */
                /* {0 3 3 0 0 2 0} tr = 0 */
                mclip_gaincode = 0x41006c;
                /* construct code_A,B to reuse existing code and for consistency */
                code_A  =  (mclip_gaincode >> 1)&0x7ff;
                code_B  =  ((mclip_gaincode >> 23)&0x1)<<1 |
                    ((!mclip_gaincode & 0x1))<<2 | (mclip_gaincode & 0x1)<<3 |
                    ((mclip_gaincode >> 12)&0x7)<<4 |
                    ((mclip_gaincode >> 15)&0x7)<<8 |
                    ((mclip_gaincode >> 18)&0xf)<<12;
            } else {
                code_A  =  READ_PHYREGC(pi, clipmdGainCodeA, core);
                code_B   = READ_PHYREGC(pi, clipmdGainCodeB, core);
            }
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
                ACMAJORREV_4(pi->pubpi->phy_rev)) {
                core_freq_segment_map = phy_ac_chanmgr_get_data
                    (pi_ac->chanmgri)->core_freq_mapping[core];
                subband_idx = wlc_phy_rssi_get_chan_freq_range_acphy(pi,
                        core_freq_segment_map, core);
                lna1_gain_corr[core] = LNA1_GAINSTEP_ERR[subband_idx];
            }
        } else if (force_gain_type == 2) {
            if (mclip_agc_en) {
                /* 2G and 5G has different lineup; Use similar gain */
                mclip_gainidx = CHSPEC_IS2G(pi->radio_chanspec) ? 1 : 3;
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RSSICLIPGAIN0+core*32, 1,
                                         (1+mclip_gainidx), 32,
                                         &mclip_gaincode);
                /* construct code_A,B to reuse existing code and for consistency */
                code_A  =  (mclip_gaincode >> 1)&0x7ff;
                code_B  =  ((mclip_gaincode >> 23)&0x1)<<1 |
                    ((!mclip_gaincode & 0x1))<<2 | (mclip_gaincode & 0x1)<<3 |
                    ((mclip_gaincode >> 12)&0x7)<<4 |
                    ((mclip_gaincode >> 15)&0x7)<<8 |
                    ((mclip_gaincode >> 18)&0xf)<<12;
            } else {
                code_A  =  READ_PHYREGC(pi, clipHiGainCodeA, core);
                code_B   = READ_PHYREGC(pi, clipHiGainCodeB, core);
            }
        } else if (force_gain_type == 1) {
            /* Change limited to 4350, Olympic program
             * When we issue iqest with -i 1 option, INIT gain is applied.
             * But because of Interference_code, the INIT gain can change
             * So, for 4350, we have forced the fixed init gain by hardcoding it.
             */
            if (ACMAJORREV_2(pi->pubpi->phy_rev) && ACMINORREV_1(pi)) {
                code_A    =  pi_ac->rxgcrsi->initGain_codeA;
                code_B     = pi_ac->rxgcrsi->initGain_codeB;
            } else {
                code_A  =  READ_PHYREGC(pi, InitGainCodeA, core);
                code_B   = READ_PHYREGC(pi, InitGainCodeB, core);
            }
        } else if (force_gain_type == 7) {
            code_A    =  pi_ac->rxgcrsi->initGain_codeA;
            code_B     = pi_ac->rxgcrsi->initGain_codeB;
        } else if (force_gain_type == 8) {
            code_A    =  pi_ac->rxgcrsi->initGain_codeA;
            code_B     = pi_ac->rxgcrsi->initGain_codeB;
        } else if (force_gain_type == 9) {
            if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                /* Bump gain by 6dB for better estimation below -82dBm */
                code_A  =  0x4aa;
                code_B   = 0x1204;
            } else {
                code_A  =  0x16a;
                code_B   = 0x554;
            }
        } else if (force_gain_type == 6) {
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 0);
            continue;
        } else {
            return;
        }

        rxgain[core].lna1 = (code_A >> 1) & 0x7;
        rxgain[core].lna2 = (code_A >> 4) & 0x7;
        rxgain[core].mix  = (code_A >> 7) & 0xf;
        rxgain[core].lpf0 = (code_B >> 4) & 0x7;
        rxgain[core].lpf1 = (code_B >> 8) & 0x7;
        rxgain[core].dvga = (code_B >> 12) & 0xf;

        if (core == 0) {
            gain_tblid =  ACPHY_TBL_ID_GAIN0;
        } else if (core == 1) {
            gain_tblid =  ACPHY_TBL_ID_GAIN1;
        } else {
            gain_tblid =  ACPHY_TBL_ID_GAIN2;
        }

        stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
        ACPHY_DISABLE_STALL(pi);
        /* ELNA */
        if (elna_present == 1) {
            wlc_phy_table_read_acphy(pi, gain_tblid, 1, (0x0 + (code_A & 0x1)),
                                     8, &elna_gain[core]);
        }

        /* lna1, lna2 and mixer */
        wlc_phy_table_read_acphy(pi, gain_tblid, 1, (0x8 + rxgain[core].lna1), 8,
                &gain_lna1);
        gain_lna1 -= lna1_gain_corr[core];
        gain_lna2 = pi_ac->rxgcrsi->lna2_complete_gaintbl[rxgain[core].lna2];
        wlc_phy_table_read_acphy(pi, gain_tblid, 1, (0x20 + rxgain[core].mix), 8,
                                 &gain_mix);
        ACPHY_ENABLE_STALL(pi, stall_val);

        gain_bq0 = 3 * rxgain[core].lpf0;
        /* For 4347, lpf1 is not used. lpf1 index is still set to 2 */
        /* But lpf1 gain should be set to 0 regardless of index   */
        if (ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
            gain_bq1 = 0;
        }
        else {
            gain_bq1 = 3 * rxgain[core].lpf1;
        }
        gain_dvga = 3 * rxgain[core].dvga;

        if (force_gain_type == 4) {
            if (mclip_agc_en) {
                tr_tx = mclip_gaincode & 0x1;
            } else {
                tr_tx = READ_PHYREGFLD(pi, Core0cliploGainCodeB, clip1loTrTxIndex);
            }
            if (lna1byp) {
                    tr_loss =  (READ_PHYREG(pi, TRLossValue) & 0x7f);
                    gain_lna1 = READ_PHYREGFLD(pi, Core0_lna1BypVals,
                            lna1BypGain0);
            }
            if (tr_tx) {
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                            tr_sw_tx_pu, 1);
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                            tr_sw_rx_pu, 0);
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                            override_tr_sw, 1);
                    if (core == 0) {
                        tr_loss = READ_PHYREGFLD(pi, Core0_TRLossValue,
                            freqGainTLoss0);
                    } else if (core == 1) {
                        tr_loss = READ_PHYREGFLD(pi, Core1_TRLossValue,
                            freqGainTLoss1);
                    } else if (core == 2) {
                        tr_loss = READ_PHYREGFLD(pi, Core2_TRLossValue,
                            freqGainTLoss2);
                    } else {
                        tr_loss = READ_PHYREGFLD(pi, Core3_TRLossValue,
                            freqGainTLoss3);
                    }
            }
         } else if ((force_gain_type == 3) &&
                    (mclip_agc_en || (pi_ac->rxgcrsi->mdgain_trtx_allowed))) {
            if (mclip_agc_en) {
                tr_tx = mclip_gaincode & 0x1;
            } else {
                tr_tx = READ_PHYREGFLD(pi, Core0clipmdGainCodeB, clip1mdTrTxIndex);
            }
            if (tr_tx) {
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                        tr_sw_tx_pu, 1);
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                        tr_sw_rx_pu, 0);
                    MOD_PHYREGCE(pi, RfctrlIntc, core,
                        override_tr_sw, 1);
                    if (core == 0) {
                        tr_loss = READ_PHYREGFLD(pi, Core0_TRLossValue,
                            freqGainTLoss0);
                    } else if (core == 1) {
                        tr_loss = READ_PHYREGFLD(pi, Core1_TRLossValue,
                            freqGainTLoss1);
                    } else if (core == 2) {
                        tr_loss = READ_PHYREGFLD(pi, Core2_TRLossValue,
                            freqGainTLoss2);
                    } else {
                        tr_loss = READ_PHYREGFLD(pi, Core3_TRLossValue,
                            freqGainTLoss3);
                    }
            } else {
                    tr_loss =  READ_PHYREG(pi, TRLossValue) & 0x7f;
            }
        } else {
            tr_loss =  READ_PHYREG(pi, TRLossValue) & 0x7f;
        }
        /* Total gain: */
        tot_gain[core] = elna_gain[core] + gain_lna1 + gain_lna2 + gain_mix + gain_bq0
                +  gain_bq1 - tr_loss + gain_dvga;
        /* core_freq_segment_map is only required for 80P80 mode.
        For other modes, it is ignored
        */
        core_freq_segment_map =
            phy_ac_chanmgr_get_data(pi_ac->chanmgri)->core_freq_mapping[core];

        /* adjust total gain based on common rssi correction factor: */
        ant = phy_get_rsdbbrd_corenum(pi, core);
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            tot_gain[core] -=
                    pi_ac->sromi->rssioffset.rssi_corr_normal[ant][bw_idx];
        } else {
            if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
                phy_ac_chanmgr_get_chan_freq_range_80p80(pi, 0, bands);
                subband_idx = (core <= 1) ? (bands[0] - 1) : (bands[1] - 1);
            } else {
                subband_idx = phy_ac_chanmgr_get_chan_freq_range(pi,
                    pi->radio_chanspec, core_freq_segment_map)-1;
            }
            tot_gain[core] -=
                    pi_ac->sromi->rssioffset.rssi_corr_normal_5g[ant][subband_idx]
                    [bw_idx];
        }
        PHY_RXIQ(("In %s: | Mode = %d | Code_A = %X | Code_B = %X |"
              "\n", __FUNCTION__, force_gain_type, code_A, code_B));
    }
}

static int8
wlc_phy_rxgainctrl_calc_low_sens_acphy(phy_info_t *pi, int8 clipgain, bool trtx, bool lna1byp,
    uint8 core)
{
    /* c9s1 1% sensitivity for 20/40/80/160 mhz */
    int sens, sens_bw_c9[] = {-66, -63, -60, -57};
    /* c11s1_ldpc 1% sensitivity for 20/40/80/160 mhz */
    int sens_bw_c11[] = {-63, -60, -57, -54};
    /* low_end_sens = -clip_gain - low_sen_adjust */
    uint8 low_sen_adjust[] = {25, 22, 19, 16};
    uint8 low_sen_adjust_rev47 = 22;
    uint8 bw_idx, elna_idx, trloss, elna_bypass_tr;
    int8 elna, detection, demod, nfhit;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
    acphy_desense_values_t *desense = pi_ac->rxgcrsi->total_desense;
    uint8 idx, max_lna2;
    int8 extra_loss = 0;

    ASSERT(core < PHY_CORE_MAX);
    bw_idx = CHSPEC_BW_LE20(pi->radio_chanspec) ? 0 :
            CHSPEC_IS40(pi->radio_chanspec) ? 1 :
            (CHSPEC_IS80(pi->radio_chanspec) ||
            PHY_AS_80P80(pi, pi->radio_chanspec)) ? 2 :
            3;
    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        sens =  sens_bw_c9[bw_idx];
    } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
        sens =  sens_bw_c11[bw_idx];
    } else {
        sens =  sens_bw_c9[bw_idx];
    }

    if (lna1byp) {
        nfhit = rxg_params->lna1_byp_nfhit;
        if (nfhit == -1) {
            sens = sens + 7;
        } else {
            sens = sens + nfhit;
        }
    }

    elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
    elna = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0][elna_idx];
    trloss = pi_ac->rxgcrsi->fem_rxgains[core].trloss;
    elna_bypass_tr = pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr;

    if (TINY_RADIO(pi))
        detection = -6 - (clipgain + low_sen_adjust[bw_idx]);
    else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev))
        detection = 0 - (clipgain + low_sen_adjust_rev47);
    else
        detection = 0 - (clipgain + low_sen_adjust[bw_idx]);

    demod = trtx ? (sens + trloss - (elna_bypass_tr * elna)) : sens;
    demod += desense->nf_hit_lna12;

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        /* sens is worse if lower lna2 gains are used */
        if (trtx) {
            idx = pi_ac->rxgcrsi->rxgainctrl_stage_len[LNA2_ID] - 1;
            max_lna2 =
                pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[LNA2_ID][idx];
            extra_loss = 3 * (3 - max_lna2);
            demod += MAX(0, extra_loss);
        }
    }

    return MAX(detection, demod);
}

int
acphy_get_rxgain_index(phy_type_rxgcrs_ctx_t *ctx, int32 *index)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;
    rxgain_t rxgain[PHY_CORE_MAX];
    int16 gain_db[PHY_CORE_MAX];
    uint8 rssi_rev = phy_ac_rssi_get_data(pi->u.pi_acphy->rssii)->rssi_cal_rev;
    int i;
    bool forced;
    uint32 ind;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if (CHIPID(pi->sh->chip) != BCM4350_CHIP_ID)
        return BCME_UNSUPPORTED;

    *index = 0;

    /* find gain */
    FOREACH_CORE(pi, i) {
        forced = READ_PHYREGFLDCE(pi, RfctrlOverrideGains, i, rxgain);
        if (i == 0) {

            if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
                /* prevent any rssi cal */
                phy_ac_rssi_set_cal_rev(pi->u.pi_acphy->rssii, TRUE);

                /* when forced, init gain will contain current gain. In normal
                 * operation we use GainInfo. These correspond to added gain modes
                 * 11 and 10 respectively
                 */

                /* init gain or pkt */

                wlc_phy_get_rxgain_acphy(pi, rxgain, gain_db, forced ? 11 : 10);
                phy_ac_rssi_set_cal_rev(pi->u.pi_acphy->rssii, rssi_rev);
            } else { /* ACHWACIREV(pi) */
                wlc_phy_get_rxgain_acphy(pi, rxgain, gain_db, 10);
            }
        }
        /* Broadloss of 2 dB is applied. Common to both 4345 and 4350 */
        if (rxgain[i].trtx == 0)
            gain_db[i] += 2;

        /* compute index with round */
        ind = ((gain_db[i] + 20)*85+255)/256;

        if (ind >= 64)
            return BCME_ERROR;

        if (ACPHY_ENABLE_FCBS_HWACI(pi)) {

          /* if ACI mode, index reference from 64 */
          if (rxgcrs_info->curr_desense->ofdm_desense ||
              rxgcrs_info->curr_desense->bphy_desense ||
              rxgcrs_info->curr_desense->lna1_tbl_desense)
            ind += 64;
        } else if (ACHWACIREV(pi)) {
            if (phy_ac_noise_get_data(pi_ac->noisei)->hw_aci_status)
                ind += 64;
        }

        *index |= ind<<(8*i);
        if (ACHWACIREV(pi) && forced) {
            *index = phy_ac_noise_get_data(pi_ac->noisei)->gain_idx_forced;
        }
    }
    PHY_INFORM(("GIDX_OVR: Core0 | phyreg 0x722: %x | 0x730: %x |"
        "0x731: %x |0x734: %x | \n",
        READ_PHYREG(pi, RfctrlOverrideGains0),
        READ_PHYREG(pi, RfctrlCoreRXGAIN10),
        READ_PHYREG(pi, RfctrlCoreRXGAIN20),
        READ_PHYREG(pi, RfctrlCoreLpfGain0)));
    PHY_INFORM(("GIDX_OVR: Core1 | phyreg 0x922: %x | 0x930: %x |"
        "0x931: %x |0x934: %x | \n",
        READ_PHYREG(pi, RfctrlOverrideGains1),
        READ_PHYREG(pi, RfctrlCoreRXGAIN11),
        READ_PHYREG(pi, RfctrlCoreRXGAIN21),
        READ_PHYREG(pi, RfctrlCoreLpfGain1)));

    return BCME_OK;
}

static rxgain_ovrd_t *
BCMRAMFN(phy_ac_rxgcrs_get_rxgainindx_cmd_ovrd)(void)
{
    return g_rxgainindx_cmd_ovrd;
}

int
acphy_set_rxgain_index(phy_type_rxgcrs_ctx_t *ctx, int32 index)
{
    phy_ac_rxgcrs_info_t *ri = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = ri->pi;
    uint8 core;
    int gain_step;
    int8 dB;
    int32 rb_index = 0;
    uint8 gain_idx[ACPHY_MAX_RX_GAIN_STAGES] = {0, 0, 0, 0, 0, 0, 0};
    bool trtx;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    rxgain_t rxgain[PHY_CORE_MAX];
    bool blocker_mode, forced;
    bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
        : BF_ELNA_5G(pi_ac);
    rxgain_ovrd_t rxgain_ovrd[PHY_CORE_MAX];
    uint8 ind = 0; /* per core gain index */
    bool suspend;
    rxgain_ovrd_t *rxgainindx_cmd_ovrd = phy_ac_rxgcrs_get_rxgainindx_cmd_ovrd();

    if (CHIPID(pi->sh->chip) != BCM4350_CHIP_ID)
        return BCME_UNSUPPORTED;

    phy_ac_noise_set_gainidx(pi_ac->noisei, (uint16) index);

    /* find gain */
    FOREACH_CORE(pi, core) {

        ind = index >> (8*core);

        forced = READ_PHYREGFLDCE(pi, RfctrlOverrideGains, core, rxgain);

        /* map from index to gain */
        blocker_mode = ind > 63;
        gain_step = ind & 0x3f;

        /* return an error in out of bounds
        (rely on encode gain for actualy max/min checking)
        */
        gain_step = MIN(MAX(gain_step, 0), 38);

        /* round ? may need to add bias for unique unique mapping */
        dB = gain_step * 3 - 20;

        if (ACPHY_ENABLE_FCBS_HWACI(pi)) {

            ri->curr_desense->clipgain_desense[0] = 0;
            ri->curr_desense->clipgain_desense[1] = 0;
            ri->curr_desense->clipgain_desense[2] = 0;
            ri->curr_desense->clipgain_desense[3] = 0;

            if ((blocker_mode == 0) || (ind == 255)) {
                ri->curr_desense->ofdm_desense = 0;
                ri->curr_desense->bphy_desense = 0;
                ri->curr_desense->lna1_tbl_desense = 0;
            } else {
                ri->curr_desense->ofdm_desense = HWACI_OFDM_DESENSE;
                ri->curr_desense->bphy_desense = HWACI_BPHY_DESENSE;
                ri->curr_desense->lna1_tbl_desense = HWACI_LNA1_DESENSE;
                ri->curr_desense->clipgain_desense[0] = HWACI_CLIP_INIT_DESENSE;
                ri->curr_desense->clipgain_desense[1] = HWACI_CLIP_HIGH_DESENSE;
                ri->curr_desense->clipgain_desense[2] = HWACI_CLIP_MED_DESENSE;
                ri->curr_desense->clipgain_desense[3] = HWACI_CLIP_LO_DESENSE;
            }

            /* this will update AGC settings for ALL cores, even though
            further below we only copy those of interest. For multicore
            devices this will have to be handled more carefully...
            */
            wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(pi, core, ACPHY_TBL_ID_GAIN0,
            ACPHY_TBL_ID_GAINBITS0);
            wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
#ifndef WLC_DISABLE_ACI
        } else if (ACHWACIREV(pi)) {
            int state;
            if (blocker_mode == 0) {
                state = 0;
            } else if (ind == 255) {
                state = -1;
            } else {
                state = 1;
            }
            wlc_phy_hwaci_override_acphy(pi, state);
#endif /* WLC_DISABLE_ACI */
        }

        if (ind == 255) {
            /* index 255 - release any gain override */
            WRITE_PHYREGCE(pi, RfctrlOverrideGains, core,
            rxgainindx_cmd_ovrd[core].rfctrlovrd);
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core,
            rxgainindx_cmd_ovrd[core].rxgain);
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN2, core,
            rxgainindx_cmd_ovrd[core].rxgain2);
            WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core,
            rxgainindx_cmd_ovrd[core].lpfgain);
            MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 0);
#ifndef WLC_DISABLE_ACI
            if (ACHWACIREV(pi)) {
                MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 0);
                MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
                MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 0);
                wlc_phy_hwaci_override_acphy(pi, -1);
            }
#endif /* WLC_DISABLE_ACI */
            continue;
        }

        if (dB < ri->rx_elna_bypass_gain_th[core])
            trtx = elna_present;
        else
            trtx = 0;

        if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
            (void)wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, dB, trtx,
                FALSE, INIT_GAIN, gain_idx);
            /* write to gain code A , also allows us to use get_rxgain */
            wlc_phy_set_analog_rxgain(pi, 0, gain_idx, trtx, core);

            acphy_get_rxgain_index(pi, &rb_index); /* debug only */
        } else if (ACHWACIREV(pi)) {
            (void)wlc_phy_rxgainctrl_encode_pktgain_acphy(pi, core, dB, trtx,
                FALSE, gain_idx);
        }

        /* also write to rfctrl (not strictly necessary if we are
        enforcing stay-in-carrier search
        */
        rxgain[core].lna1 = gain_idx[1];
        rxgain[core].lna2 = gain_idx[2];
        rxgain[core].mix  = gain_idx[3];
        rxgain[core].lpf0 = gain_idx[4];
        rxgain[core].lpf1 = gain_idx[5];
        rxgain[core].dvga = gain_idx[6];
        PHY_INFORM(("GIDX_vals core%d: index %d:%d | dB: %d ,%d |elna %d lna1 %d lna2 %d "
            "mix %d biq0 %d biq1 %d dvga %d trtx %d EBYP_th %d | BKR %d\n",
            core, ind, ((rb_index>>(core*8))&0xff), dB,
            wlc_phy_rxgainctrl_encode_pktgain_acphy(pi, core, dB, trtx,
            FALSE, gain_idx),
            gain_idx[0], gain_idx[1], gain_idx[2], gain_idx[3], gain_idx[4],
            gain_idx[5], gain_idx[6], trtx, ri->rx_elna_bypass_gain_th[core],
            blocker_mode));

        if (forced == 0) {
            /* gain is currently unforced - save override state for to restore
            at a later date if required
            */
            rxgainindx_cmd_ovrd[core].rfctrlovrd =
            READ_PHYREGCE(pi, RfctrlOverrideGains, core);
            rxgainindx_cmd_ovrd[core].rxgain =
            READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);
            rxgainindx_cmd_ovrd[core].rxgain2 =
            READ_PHYREGCE(pi, RfctrlCoreRXGAIN2, core);
            rxgainindx_cmd_ovrd[core].lpfgain =
            READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
        }

        /* override tr/tx */
        if (trtx) {
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, trtx);
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 1);
        } else {
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
            MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 0);
        }
    }
    /* override all core gains
     * Note, with this implementation we must have all cores overriden,
     * because we are using rfctrl_overriderxgain.
     */
    if (ind != 255)
        wlc_phy_rfctrl_override_rxgain_acphy(pi, 0, rxgain, rxgain_ovrd);

    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend)
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
    /* flush init gain changes */
    wlc_phy_resetcca_acphy(pi);
    if (!suspend)
        wlapi_enable_mac(pi->sh->physhim);

    return BCME_OK;

}

static int8
wlc_phy_rxgainctrl_encode_pktgain_acphy(phy_info_t *pi, uint8 core, int8 gain_dB,
                                     bool trloss, bool lna1byp, uint8 *gidx)
{
    int16 gain_needed, tr;
    int8 i, k;
    int8 *gaintbl_this_stage, gain_this_stage = 0;
    int16 gain_applied = 0;
    uint8 *gainbitstbl_this_stage;
    uint8 gaintbl_len, sb;
    int16 st_gain = 24, max_bq0_idx = 7, max_bq1_idx = 0;
    int16 gain_lim_off[4] = {0, 8, 16, 32};
    int8 gain_lim_entries[12];
    int16 bq0_index = 0, bq1_index = 0;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    int8   elna_gain[PHY_CORE_MAX];
    bool   elna_present;
    uint8 ch5g_sb[2] = {100, 132};
    /* BQ0 stage number. At which stage, remanining gain is rounded */

    PHY_TRACE(("%s: TARGET %d\n", __FUNCTION__, gain_dB));

    ASSERT(core < PHY_CORE_MAX);

    if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        ch5g_sb[1] = 149;
    }

    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        elna_present = BF_ELNA_2G(pi_ac);
    } else {
        elna_present = BF_ELNA_5G(pi_ac);
    }
    if (elna_present == 1) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            elna_gain[core] = pi_ac->sromi->femrx_2g[core].elna;
        } else if (CHSPEC_IS5G(pi->radio_chanspec)) {
            if (CHSPEC_CHANNEL(pi->radio_chanspec) < ch5g_sb[0]) {
                elna_gain[core] = pi_ac->sromi->femrx_5g[core].elna;
            } else if (CHSPEC_CHANNEL(pi->radio_chanspec) < ch5g_sb[1]) {
                elna_gain[core] = pi_ac->sromi->femrx_5gm[core].elna;
            } else {
                elna_gain[core] = pi_ac->sromi->femrx_5gh[core].elna;
            }
        } else if (CHSPEC_IS6G(pi->radio_chanspec)) {
            if ((ACMAJORREV_130(pi->pubpi->phy_rev)) && (RADIOMAJORREV(pi) >= 2)) {
                sb = (CHSPEC_CHANNEL(pi->radio_chanspec) < 101) ? 0:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 141) ? 1:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 161) ? 2:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 201) ? 3: 4;
            } else {
                sb = (CHSPEC_CHANNEL(pi->radio_chanspec) < 33) ? 0:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 65) ? 1:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 97) ? 2:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 129) ? 3:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 161) ? 4:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 193) ? 5: 6;
            }
            elna_gain[core] = pi_ac->sromi->femrx_6g[core][sb].elna;
        } else {
            elna_gain[core] = pi_ac->sromi->femrx_5g[core].elna;
        }
    } else {
        elna_gain[core] = 0;
    }

    if (trloss) {
        /* Values: elna_gain = 12, trloss = 18 */
        tr =  elna_gain[core] - pi_ac->rxgcrsi->fem_rxgains[core].trloss;
    } else {
        tr = elna_gain[core];
    }
    gain_needed = gain_dB;

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {

        gaintbl_this_stage = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[i];
        gainbitstbl_this_stage = pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[i];
        gaintbl_len = pi_ac->rxgcrsi->rxgainctrl_stage_len[i];

        if ((i == 1) || (i == 2) || (i == 3)) {
            /* For LNA1/2, read the gaindB, gainBits and gainLim tables. */
            wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAIN0, gaintbl_len,
                gain_lim_off[i], 8, gaintbl_this_stage);
            wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINBITS0, gaintbl_len,
                gain_lim_off[i], 8, gainbitstbl_this_stage);
            wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, gaintbl_len,
                gain_lim_off[i], 8, gain_lim_entries);
        }
        if (i == 0) {
            /* ELNA */
            /* If elna not bypassed tr == 12, else tr == 12 - 18 */
            gain_this_stage = (int8)tr;
        } else if ((i == 1) || (i == 2) || (i == 3)) {
            /* LNA1, LNA2 and Mix-Tia */
            for (k = gaintbl_len - 1; k >= 0; k--) {
                    if ((gain_needed >= gain_lim_entries[k]) || (k == 0)) {
                        gain_this_stage = gaintbl_this_stage[k];
                    } else {
                        continue;
                    }
                gain_this_stage = gaintbl_this_stage[k];
                gidx[i] = gainbitstbl_this_stage[k];
                break;
            }
        } else if (i == 4) {
            /* Biquad 0 */
            /* Logic for choosing BQ0 gain.
             * Step1: Is remaining gain >= 24, if yes, BQ0 will give 24 dB gain
             * If no, is remaining gain >= 24/2, if yes, BQ0 will give 12 dB gain
             * If no, is remaining gain >= 24/2/2, if yes, BQ0 will give 6 dB gain
             * If no, is remaining gain >= 24/2/2/2, if yes, BQ0 will give 3 dB gain
             */
            while ((gain_needed < st_gain) && (st_gain != 0)) {
                st_gain = st_gain / 2;
            }
            bq0_index = st_gain / 3;
            bq0_index = MIN(max_bq0_idx, bq0_index);
            bq0_index = bq0_index > 0 ? bq0_index : 0;
            gain_this_stage = bq0_index * 3;
            gidx[i] = (int8)bq0_index;

        } else if (i == 5) {
            /* Biquad 1 */
            /* Logic behind choosing biquad gain indices -
             * First biquad 0's index is arrived at. Then, remaning gain from Biquad 1.
             */
            max_bq1_idx = (READ_PHYREG(pi, Core0_BiQuad_MaxGain) - bq0_index * 3) / 3;
            bq1_index = gain_needed / 3;
            bq1_index = MIN(bq1_index, max_bq1_idx);

            bq1_index = bq1_index > 0 ? bq1_index : 0;
            gain_this_stage = bq1_index * 3;
            gidx[i] = (int8)bq1_index;

        } else if (i == 6) {
            /* DVGA */
            if (gain_needed >= 3) {
                gidx[i] = (int8)(gain_needed / 3);
                gain_this_stage =  gidx[i] * 3;
            } else {
                gidx[i] = 0;
                gain_this_stage =  0;
            }
        } else {
            gain_this_stage = 0;
        }

        gain_applied += gain_this_stage;
        gain_needed = gain_needed - gain_this_stage;
    }
    PHY_INFORM(("gain_applied = %d, tr = %d \n",
        gain_applied, tr));

    return (int8)gain_applied;
}

static void
wlc_phy_set_analog_rxgain(phy_info_t *pi, uint8 clipgain, uint8 *gain_idx, bool trtx, uint8 core)
{
    uint8 lna1, lna2, mix, bq0, bq1, tx, rx, dvga;
    uint16 gaincodeA, gaincodeB, final_gain;
    bool lna1byp;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    ASSERT(core < PHY_CORE_MAX);

    lna1 = gain_idx[1];
    lna2 = gain_idx[2];
    mix = gain_idx[3];
    bq0 = (TINY_RADIO(pi)) ? 0 : gain_idx[4];
    bq1 = gain_idx[5];
    dvga = (TINY_RADIO(pi) || IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) ? gain_idx[6] : 0;

    lna1byp = pi_ac->rxgcrsi->fem_rxgains[core].lna1byp;
    if (lna1byp || !trtx) {
        tx = 0; rx = 1;
    } else {
        tx = 1; rx = 0;
    }

    gaincodeA = ((mix << 7) | (lna2 << 4) | (lna1 << 1));
    gaincodeB = (dvga<<12) | (bq1 << 8) | (bq0 << 4) | (tx << 3) | (rx << 2);

    if (clipgain == 0) {
        WRITE_PHYREGC(pi, InitGainCodeA, core, gaincodeA);
        WRITE_PHYREGC(pi, InitGainCodeB, core, gaincodeB);
        final_gain = ((bq1 << 13) | (bq0 << 10) | (mix << 6) | (lna2 << 3) | (lna1 << 0));
        if (core == 3)
            wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x509, 16,
                                  &final_gain);
        else
            wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0xf9 + core), 16,
                                  &final_gain);

        if (TINY_RADIO(pi) || IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            uint8 gmrout;
            uint16 rfseq_init_aux;
            uint8 offset = ACPHY_LNAROUT_BAND_OFFSET(pi,
                pi->radio_chanspec)    + lna1 +
                ACPHY_LNAROUT_CORE_RD_OFST(pi, core);

            wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, offset, 8, &gmrout);

            if (ACMAJORREV_GE37(pi->pubpi->phy_rev))
                rfseq_init_aux = dvga;
            else
                rfseq_init_aux = (((0xf & (gmrout >> 3)) << 4) | dvga);

            if (core == 3)
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, 0x506, 16,
                    &rfseq_init_aux);
            else
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1, (0xf6 + core),
                    16, &rfseq_init_aux);

            /* elna index always zero, not required */
        }
    } else if (clipgain == 1) {
        WRITE_PHYREGC(pi, clipHiGainCodeA, core, gaincodeA);
        WRITE_PHYREGC(pi, clipHiGainCodeB, core, gaincodeB);
    } else if (clipgain == 2) {
        WRITE_PHYREGC(pi, clipmdGainCodeA, core, gaincodeA);
        WRITE_PHYREGC(pi, clipmdGainCodeB, core, gaincodeB);
    } else if (clipgain == 3) {
        WRITE_PHYREGC(pi, cliploGainCodeA, core, gaincodeA);
        WRITE_PHYREGC(pi, cliploGainCodeB, core, gaincodeB);
    } else if (clipgain == 4) {
        WRITE_PHYREGC(pi, clip2GainCodeA, core, gaincodeA);
        WRITE_PHYREGC(pi, clip2GainCodeB, core, gaincodeB);
    }

    if (lna1byp) {
        MOD_PHYREGC(pi, cliploGainCodeB, core, clip1loLna1Byp, 1);
        if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            WRITE_PHYREGC(pi, _lna1BypVals, core, 0xfa1);
            MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);
        }
    } else {
        MOD_PHYREGC(pi, cliploGainCodeB, core, clip1loLna1Byp, 0);
    }

}

static int8
wlc_phy_rxgainctrl_calc_high_sens_acphy(phy_info_t *pi, int8 clipgain, bool trtx, bool lna1byp,
    uint8 core)
{
    uint8 high_sen_adjust = 23;  /* high_end_sens = high_sen_adjust - clip_gain */
    uint8 elna_idx, trloss;
    int8 elna, saturation, clipped, lna1_sat;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if (TINY_RADIO(pi)) {
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            lna1_sat = CHSPEC_IS2G(pi->radio_chanspec) ? -12 : -10;
            high_sen_adjust = 20;
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                lna1_sat = -10;
                high_sen_adjust = 17;
            } else {
                lna1_sat = -10; /* changed this to match dingo */
                high_sen_adjust = 19;
            }
        }
        if (lna1byp)
            lna1_sat = CHSPEC_IS2G(pi->radio_chanspec) ? (lna1_sat + 10):(lna1_sat + 7);
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
      lna1_sat = (RADIOMAJORREV(pi) > 1) ? -18 : -8;
      high_sen_adjust = 20;
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev)) {
            lna1_sat = -8;
            high_sen_adjust = 20;
    } else {
        lna1_sat = -16;
        high_sen_adjust = 23;
    }

    ASSERT(core < PHY_CORE_MAX);
    elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
    elna = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0][elna_idx];
    trloss = pi_ac->rxgcrsi->fem_rxgains[core].trloss;

    /* c9 needs lna1 input to be below -lna1_sat */
    saturation = trtx ? (trloss + lna1_sat - elna) : 0 + lna1_sat - elna;
    clipped = high_sen_adjust - clipgain;

    return MIN(saturation, clipped);
}

/* Wrapper to call encode_gain & set init/clip gains */
static uint8
wlc_phy_rxgainctrl_set_init_clip_gain_acphy(phy_info_t *pi, uint8 clipgain, int8 gain_dB,
    bool trtx, bool lna1byp, uint8 core)
{
    uint8 gain_idx[ACPHY_MAX_RX_GAIN_STAGES];
    uint8 gain_applied;

    gain_applied = wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, gain_dB, trtx, lna1byp,
        clipgain, gain_idx);
    wlc_phy_set_analog_rxgain(pi, clipgain, gain_idx, trtx, core);

    return gain_applied;
}

static void
phy_ac_rxgcrs_max_anagain_to_initgain_lesi(phy_ac_rxgcrs_info_t *info, uint8 i, uint8 core,
    uint8 gidx)
{
    uint8 l, offset = 0, gainbits[15];
    int8 gains[15];
    uint16 gainbits_tblid, gains_tblid;
    phy_info_t *pi = info->pi;
    uint8 gainbit_tbl_entry_size = sizeof(info->rxgainctrl_params[0].gainbitstbl[0][0]);
    uint8 gain_tbl_entry_size = sizeof(info->rxgainctrl_params[0].gaintbl[0][0]);

    if (i == 1) {
        offset = 8;
    } else if (i == 2) {
        offset = 16;
    } else if (i == 3) {
        offset = 32;
    } else {
        ASSERT(0);
    }

    /* gainBits */
    for (l = 0; l < info->rxgainctrl_stage_len[i]; l++)
        gainbits[l] = (l > gidx) ? gidx : info->rxgainctrl_params[core].gainbitstbl[i][l];
    gainbits_tblid = (core == 0) ? ACPHY_TBL_ID_GAINBITS0 : (core == 1) ?
            ACPHY_TBL_ID_GAINBITS1 : (core == 2) ?
            ACPHY_TBL_ID_GAINBITS2 : ACPHY_TBL_ID_GAINBITS3;
    wlc_phy_table_write_acphy(
        pi, gainbits_tblid, info->rxgainctrl_stage_len[i],
        offset, ACPHY_GAINBITS_TBL_WIDTH, gainbits);
    memcpy(info->rxgainctrl_params[core].gainbitstbl[i], gainbits,
           gainbit_tbl_entry_size * info->rxgainctrl_stage_len[i]);

    /* gaintbl */
    for (l = 0; l < info->rxgainctrl_stage_len[i]; l++)
        gains[l] = (l > gidx) ?
                info->rxgainctrl_params[core].gaintbl[i][gidx] :
                info->rxgainctrl_params[core].gaintbl[i][l];
    gains_tblid = (core == 0) ? ACPHY_TBL_ID_GAIN0 : (core == 1) ?
            ACPHY_TBL_ID_GAIN1 : (core == 2) ?
            ACPHY_TBL_ID_GAIN2 : ACPHY_TBL_ID_GAIN3;
    wlc_phy_table_write_acphy(
        pi, gains_tblid, info->rxgainctrl_stage_len[i],
        offset, ACPHY_GAINDB_TBL_WIDTH, gains);
    memcpy(info->rxgainctrl_params[core].gaintbl[i], gains,
           gain_tbl_entry_size * info->rxgainctrl_stage_len[i]);

}

uint8
wlc_phy_rxgainctrl_encode_gain_acphy(phy_info_t *pi, uint8 core, int8 gain_dB,
    bool trloss, bool lna1byp, uint8 clipgain, uint8 *gidx)
{
    int16 min_gains[ACPHY_MAX_RX_GAIN_STAGES], max_gains[ACPHY_MAX_RX_GAIN_STAGES];
    int8 k, idx, maxgain_this_stage, gain, bq0_idx = 0, bq0_gain = 0;
    int16 sum_min_gains, gain_needed, tr = 0;
    uint8 i, j;
    int8 *gaintbl_this_stage, gain_this_stage = 0;
    int16 total_gain = 0;
    int16 gain_applied = 0;
    uint8 *gainbitstbl_this_stage;
    uint8 gaintbl_len, lowest_idx;
    int8 lna1mingain, lna1maxgain;
    phy_ac_rxgcrs_info_t *info = pi->u.pi_acphy->rxgcrsi;
    acphy_rxgainctrl_t gainctrl_params;
    uint8 rnd_stage;
    int8 lna1_byp_gain = 0;
    uint8 lna1_byp_ind = 0;
    bool upd = FALSE;
    acphy_desense_values_t *desense = NULL;
    uint8 desense_state;

    PHY_TRACE(("%s: TARGET %d\n", __FUNCTION__, gain_dB));
    ASSERT(core < PHY_CORE_MAX);

    if ((desense = phy_ac_noise_get_desense(pi->u.pi_acphy->noisei)) == NULL) {
        desense_state = 0;
    } else {
        desense_state = phy_ac_noise_get_desense_state(pi->u.pi_acphy->noisei);
    }

    if (TINY_RADIO(pi)) {
        rnd_stage = ACPHY_MAX_RX_GAIN_STAGES - 4;
    } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        rnd_stage = ACPHY_MAX_RX_GAIN_STAGES - 2;
    } else {
        rnd_stage = ACPHY_MAX_RX_GAIN_STAGES - 3;
    }
    memcpy(&gainctrl_params, &info->rxgainctrl_params[core],
        sizeof(acphy_rxgainctrl_t));

    /* Over-write tia table to all same if using tia_high_mode */
    if (clipgain == INIT_GAIN) {
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            idx = 7;
            gain = info->rxgainctrl_params[core].gaintbl[TIA_ID][idx];
            upd = TRUE;
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            idx = 15;
            gain = (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
                    !ACMAJORREV_128(pi->pubpi->phy_rev)) ?
                info->rxgainctrl_params[core].gaintbl[TIA_ID][N_TIA_GAINS-1] : 28;
            if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                if ((desense_state != 0) && (CHSPEC_IS5G(pi->radio_chanspec))) {
                    idx = 5;
                    gain = info->rxgainctrl_params[core].gaintbl[TIA_ID][idx];
                    bq0_idx = 1;
                    bq0_gain = info->rxgainctrl_params[core].
                       gaintbl[BQ0_ID][bq0_idx];
                }
            }
            upd = TRUE;
        }

        if (upd) {
            for (j = 0; j < info->rxgainctrl_stage_len[TIA_ID]; j++) {
                gainctrl_params.gainbitstbl[TIA_ID][j] = idx;
                gainctrl_params.gaintbl[TIA_ID][j] = gain;
                if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    if ((desense_state != 0) &&
                         CHSPEC_IS5G(pi->radio_chanspec)) {
                        gainctrl_params.gainbitstbl[BQ0_ID][j] = bq0_idx;
                        gainctrl_params.gaintbl[BQ0_ID][j] = bq0_gain;
                    }
                }
            }
        }
    }

    if (lna1byp) {
        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
                max_gains[i] = info->rxgainctrl_maxout_gains[i];
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            lna1_byp_gain = (READ_PHYREGC(pi, _lna1BypVals, core) &
                ACPHY_Core0_lna1BypVals_lna1BypGain0_MASK(pi->pubpi->phy_rev))
                >> ACPHY_Core0_lna1BypVals_lna1BypGain0_SHIFT(pi->pubpi->phy_rev);
            for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
                max_gains[i] = info->rxgainctrl_maxout_gains[i];
        } else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
            for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
                max_gains[i] = info->rxgainctrl_maxout_gains[i];
        } else {
            tr = info->fem_rxgains[core].trloss;
            lna1mingain = gainctrl_params.gaintbl[1][0];
            lna1maxgain = gainctrl_params.gaintbl[1][5];
            tr = tr - (lna1maxgain - lna1mingain);
            PHY_INFORM(("lna1mingain=%d ,lna1maxgain=%d, new tr=%d \n",
                lna1mingain, lna1maxgain, tr));
            for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
                max_gains[i] = info->rxgainctrl_maxout_gains[i] + tr;
        }
    } else if (trloss) {
        tr =  info->fem_rxgains[core].trloss;
        for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
            max_gains[i] = info->rxgainctrl_maxout_gains[i] +
                    info->fem_rxgains[core].trloss;
    } else {
        for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
            max_gains[i] = info->rxgainctrl_maxout_gains[i];
    }
    gain_needed = gain_dB + tr;
    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
        min_gains[i] = gainctrl_params.gaintbl[i][0];

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
        if (i == rnd_stage) {
            if ((gain_needed % 3) == 2)
                ++gain_needed;
            if (!TINY_RADIO(pi) && (gain_needed > 30))
                gain_needed = 30;
        }
        sum_min_gains = 0;

        for (j = i + 1; j < ACPHY_MAX_RX_GAIN_STAGES; j++) {
            if (TINY_RADIO(pi) && i < 5 && j >= 5)
                break;

            sum_min_gains += min_gains[j];
        }

        maxgain_this_stage = gain_needed - sum_min_gains;
        gaintbl_this_stage = gainctrl_params.gaintbl[i];
        gainbitstbl_this_stage = gainctrl_params.gainbitstbl[i];
        gaintbl_len = info->rxgainctrl_stage_len[i];

        if (lna1byp && (i == 1)) {
            gaintbl_len = 1;
            if (!ACMAJORREV_32(pi->pubpi->phy_rev) &&
                !ACMAJORREV_33(pi->pubpi->phy_rev)) {
                lna1_byp_gain = READ_PHYREGFLD(pi, Core0_lna1BypVals, lna1BypGain0);
                lna1_byp_ind = READ_PHYREGFLD(pi, Core0_lna1BypVals, lna1BypIndex0);
            }
        }

        /* TIA special mode:
           INIT - tia has all same values
           PKTG - tia goes from 0-6
         */
        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            if (i == 3)
                gaintbl_len = (clipgain == INIT_GAIN) ? 1 : 7;
            if (ACMAJORREV_130(pi->pubpi->phy_rev) && CHSPEC_IS5G(pi->radio_chanspec)) {
                if ((desense_state != 0) && (i == BQ0_ID) &&
                        (clipgain == INIT_GAIN))
                    gaintbl_len = 1;
                }
        }

        for (k = gaintbl_len - 1; k >= 0; k--) {
            if (lna1byp && (i == 1)) {
                gain_this_stage = lna1_byp_gain;
            } else {
                gain_this_stage = gaintbl_this_stage[k];
            }
            total_gain = gain_this_stage + gain_applied;
            lowest_idx = 0;

            if (gainbitstbl_this_stage[k] == gainbitstbl_this_stage[0])
                lowest_idx = 1;

            if ((lowest_idx == 1) || (lna1byp && (i == 1)) ||
                ((gain_this_stage <= maxgain_this_stage) && (total_gain
                                                             <= max_gains[i]))) {
                if (lna1byp && (i == 1)) {
                    gidx[i] = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                        ACMAJORREV_33(pi->pubpi->phy_rev)) ?
                        6 : lna1_byp_ind;
                } else {
                    gidx[i] = gainbitstbl_this_stage[k];
                }
                gain_applied += gain_this_stage;
                gain_needed = gain_needed - gain_this_stage;
                break;
            }
        }

        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            /* If we want to fix max_tia_gain = initgain (for lesi) */
            if ((clipgain == 0) && (i > 0) && (i <= 3) && info->tia_idx_max_eq_init) {
                phy_ac_rxgcrs_max_anagain_to_initgain_lesi(info, i,
                    core, gidx[i]);
            }
        }
    }
    PHY_INFORM(("gain_applied = %d, tr = %d \n", gain_applied, tr));

    return (gain_applied - tr);
}

static void
wlc_phy_rxgainctrl_nbclip_acphy(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
    /* Multuply all pwrs by 10 to avoid floating point math */
    int rxpwrdBm_60mv, pwr;
    int pwr_60mv[] = {-40, -40, -40};     /* 20, 40, 80 */
    uint8 nb_thresh[] = {0, 35, 60, 80, 95, 120, 140, 156}; /* nb_thresh*10 to avoid float */
    const char *reg_name[ACPHY_NUM_NB_THRESH] = {"low", "low", "mid", "mid", "mid",
                           "mid", "high", "high"};
    uint8 mux_sel[] = {0, 0, 1, 1, 1, 1, 2, 2};
    uint8 reg_val[] = {1, 0, 1, 2, 0, 3, 1, 0};
    uint8 nb, i;
    int nb_thresh_bq[ACPHY_NUM_NB_THRESH];
    int v1, v2, vdiff1, vdiff2;
    uint8 idx[ACPHY_MAX_RX_GAIN_STAGES];
    uint16 initgain_codeA;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    ASSERT(core < PHY_CORE_MAX);
    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

    rxpwrdBm_60mv = (CHSPEC_IS80(pi->radio_chanspec) ||
        PHY_AS_80P80(pi, pi->radio_chanspec)) ? pwr_60mv[2] :
        CHSPEC_IS160(pi->radio_chanspec) ? 0 :
        (CHSPEC_IS40(pi->radio_chanspec)) ? pwr_60mv[1] : pwr_60mv[0];

    for (i = 0; i < ACPHY_NUM_NB_THRESH; i++) {
        nb_thresh_bq[i] = rxpwrdBm_60mv + nb_thresh[i];
    }

    /* Get the INITgain code */
    initgain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);

    idx[0] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initExtLnaIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initExtLnaIndex);
    idx[1] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initLnaIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initLnaIndex);
    idx[2] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initlna2Index)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initlna2Index);
    idx[3] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initmixergainIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initmixergainIndex);
    idx[4] = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
    idx[5] = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ1Index);
    idx[6] = READ_PHYREGFLDC(pi, InitGainCodeB, core, initvgagainIndex);

    pwr = rxpwr_dBm;
    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES - 2; i++)
        pwr += pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[i][idx[i]];
    if (pi_ac->rxgcrsi->curr_desense->elna_bypass == 1)
        pwr = pwr - pi_ac->rxgcrsi->fem_rxgains[core].trloss;
    pwr = pwr * 10;

    nb = 0;
    if (pwr < nb_thresh_bq[0]) {
        nb = 0;
    } else if (pwr > nb_thresh_bq[ACPHY_NUM_NB_THRESH - 1]) {
        nb = ACPHY_NUM_NB_THRESH - 1;

        /* Reduce the bq0 gain, if can't achieve nbclip with highest nbclip thresh */
        if ((pwr - nb_thresh_bq[ACPHY_NUM_NB_THRESH - 1]) > 20) {
            if ((idx[4] > 0) && (idx[5] < 7)) {
                MOD_PHYREGC(pi, InitGainCodeB, core, InitBiQ0Index, idx[4] - 1);
                MOD_PHYREGC(pi, InitGainCodeB, core, InitBiQ1Index, idx[5] + 1);
            }
        }
    } else {
        for (i = 0; i < ACPHY_NUM_NB_THRESH - 1; i++) {
            v1 = nb_thresh_bq[i];
            v2 = nb_thresh_bq[i + 1];
            if ((pwr >= v1) && (pwr <= v2)) {
                vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
                vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

                if (vdiff1 < vdiff2)
                    nb = i;
                else
                    nb = i+1;
                break;
            }
        }
    }

    MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, mux_sel[nb]);

    if (strcmp(reg_name[nb], "low") == 0) {
        MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_low, reg_val[nb]);
    } else if (strcmp(reg_name[nb], "mid") == 0) {
        MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_mid, reg_val[nb]);
    } else {
        MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_high, reg_val[nb]);
    }
}

static void
wlc_phy_rxgainctrl_w1clip_acphy(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
    /* Multuply all pwrs by 10 to avoid floating point math */

    int lna1_rxpwrdBm_lo4;
    int lna1_pwrs_w1clip[] = {-340, -340, -340};   /* 20, 40, 80 */
    uint8 *w1_hi, w1_delta[] = {0, 19, 35, 49, 60, 70, 80, 88, 95, 102, 109, 115};
    uint8 w1_delta_hi2g[] = {0, 19, 35, 49, 60, 70, 80, 92, 105, 120, 130, 140};
    uint8 w1_delta_hi5g[] = {0, 19, 35, 49, 60, 70, 80, 96, 113, 130, 155, 180};
    int w1_thresh_low[ACPHY_NUM_W1_THRESH], w1_thresh_mid[ACPHY_NUM_W1_THRESH];
    int w1_thresh_high[ACPHY_NUM_W1_THRESH];
    int *w1_thresh;
    uint8 i, w1_muxsel, w1;
    uint8 elna, lna1_idx;
    int v1, v2, vdiff1, vdiff2, pwr, lna1_diff;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    ASSERT(core < PHY_CORE_MAX);
    w1_hi = CHSPEC_IS2G(pi->radio_chanspec) ? &w1_delta_hi2g[0] : &w1_delta_hi5g[0];

    if (TINY_RADIO(pi)) {
        if (CHSPEC_IS2G(pi->radio_chanspec))
            lna1_rxpwrdBm_lo4 = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) ? -360 : -370;
        else
            lna1_rxpwrdBm_lo4 = -310;
    } else
        lna1_rxpwrdBm_lo4 = (CHSPEC_IS80(pi->radio_chanspec) ||
                PHY_AS_80P80(pi, pi->radio_chanspec)) ? lna1_pwrs_w1clip[2] :
                CHSPEC_IS160(pi->radio_chanspec) ? 0 :
                (CHSPEC_IS40(pi->radio_chanspec)) ?
                lna1_pwrs_w1clip[1] : lna1_pwrs_w1clip[0];

    /* mid is 6dB higher than low, and high is 6dB higher than mid */
    for (i = 0; i < ACPHY_NUM_W1_THRESH; i++) {
        w1_thresh_low[i] = lna1_rxpwrdBm_lo4 + w1_delta[i];
        w1_thresh_mid[i] = 60 + w1_thresh_low[i];
        w1_thresh_high[i] = 120 + lna1_rxpwrdBm_lo4 + w1_hi[i];
    }

    elna = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0][0];

    lna1_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);

    if (TINY_RADIO(pi)) {
        lna1_diff = 24 - pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[1][lna1_idx];
    } else if (CHSPEC_IS2G(pi->radio_chanspec) &&
        (ACMAJORREV_2(pi->pubpi->phy_rev) && ACMINORREV_3(pi))) {
        lna1_diff = 25 - pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[1][lna1_idx];
    } else {
        lna1_diff = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[1][5] -
                    pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[1][lna1_idx];
    }

    pwr = rxpwr_dBm + elna - lna1_diff;
    if (pi_ac->rxgcrsi->curr_desense->elna_bypass == 1)
        pwr = pwr - pi_ac->rxgcrsi->fem_rxgains[core].trloss;
    pwr = pwr * 10;

    if (pwr <= w1_thresh_low[0]) {
        w1 = 0;
        w1_muxsel = 0;
    } else if (pwr >= w1_thresh_high[ACPHY_NUM_W1_THRESH - 1]) {
        w1 = 11;
        w1_muxsel = 2;
    } else {
        if (pwr > w1_thresh_mid[ACPHY_NUM_W1_THRESH - 1]) {
            w1_thresh = w1_thresh_high;
            w1_muxsel = 2;
        } else if (pwr < w1_thresh_mid[0]) {
            w1_thresh = w1_thresh_low;
            w1_muxsel = 0;
        } else {
            w1_thresh = w1_thresh_mid;
            w1_muxsel = 1;
        }

        for (w1 = 0; w1 < ACPHY_NUM_W1_THRESH - 1; w1++) {
            v1 = w1_thresh[w1];
            v2 = w1_thresh[w1 + 1];
            if ((pwr >= v1) && (pwr <= v2)) {
                vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
                vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

                if (vdiff2 <= vdiff1)
                    w1 = w1 + 1;

                break;
            }
        }
    }

    if (TINY_RADIO(pi)) {
        MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, w1_muxsel);

        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            MOD_RADIO_REG_TINY(pi, LNA2G_RSSI1, core, lna2g_dig_wrssi1_threshold, w1+4);
        } else {
            MOD_RADIO_REG_TINY(pi, LNA5G_RSSI1, core, lna5g_dig_wrssi1_threshold, w1+4);
        }
    } else {
        /* the w1 thresh array is wrt w1 code = 4 */
        /*    MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcW1ClipCntTh, w1 + 4); */
        MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, w1_muxsel);
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            MOD_RADIO_REGC(pi, LNA2G_RSSI, core, dig_wrssi1_threshold, w1 + 4);
        } else {
            MOD_RADIO_REGC(pi, LNA5G_RSSI, core, dig_wrssi1_threshold, w1 + 4);
        }
    }
}

#ifndef WLC_DISABLE_ACI
static void
wlc_phy_set_crs_min_pwr_higain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only)
{
    uint8 rxcrs_prim_loc = CHSPEC_SB(pi->radio_chanspec);

    if (!(sec_only && (rxcrs_prim_loc == 1))) {
        MOD_PHYREG(pi, crsminpoweru0, crsminpower1, thresh);
        MOD_PHYREG(pi, crsmfminpoweru0, crsmfminpower1, thresh);
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 0))) {
            MOD_PHYREG(pi, crsminpowerl0, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerl0, crsmfminpower1, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 2))) {
            MOD_PHYREG(pi, crsminpowerlSub10, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerlSub10, crsmfminpower1,  thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 3))) {
            MOD_PHYREG(pi, crsminpoweruSub10, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpoweruSub10, crsmfminpower1,  thresh);
        }
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 4))) {
            MOD_PHYREG(pi, crsminpowerULL0, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerULL0, crsmfminpower1, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 5))) {
            MOD_PHYREG(pi, crsminpowerULU0, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerULU0, crsmfminpower1, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 6))) {
            MOD_PHYREG(pi, crsminpowerUUL0, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUL0, crsmfminpower1, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 7))) {
            MOD_PHYREG(pi, crsminpowerUUU0, crsminpower1, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUU0, crsmfminpower1, thresh);
        }
    }
}
static void
wlc_phy_set_crs_min_pwr_mdgain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only)
{
    uint8 rxcrs_prim_loc = CHSPEC_SB(pi->radio_chanspec);

    if (!(sec_only && (rxcrs_prim_loc == 1))) {
        MOD_PHYREG(pi, crsminpoweru1, crsminpower2, thresh);
        MOD_PHYREG(pi, crsmfminpoweru1, crsmfminpower2, thresh);
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 0))) {
            MOD_PHYREG(pi, crsminpowerl1, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerl1, crsmfminpower2, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 2))) {
            MOD_PHYREG(pi, crsminpowerlSub11, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerlSub11, crsmfminpower2, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 3))) {
            MOD_PHYREG(pi, crsminpoweruSub11, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpoweruSub11, crsmfminpower2, thresh);
        }
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 4))) {
            MOD_PHYREG(pi, crsminpowerULL1, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerULL1, crsmfminpower2, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 5))) {
            MOD_PHYREG(pi, crsminpowerULU1, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerULU1, crsmfminpower2, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 6))) {
            MOD_PHYREG(pi, crsminpowerUUL1, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUL1, crsmfminpower2, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 7))) {
            MOD_PHYREG(pi, crsminpowerUUU1, crsminpower2, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUU1, crsmfminpower2, thresh);
        }
    }
}
static void
wlc_phy_set_crs_min_pwr_logain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only)
{
    uint8 rxcrs_prim_loc = CHSPEC_SB(pi->radio_chanspec);

    if (!(sec_only && (rxcrs_prim_loc == 1))) {
        MOD_PHYREG(pi, crsminpoweru1, crsminpower3, thresh);
        MOD_PHYREG(pi, crsmfminpoweru1, crsmfminpower3, thresh);
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 0))) {
            MOD_PHYREG(pi, crsminpowerl1, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerl1, crsmfminpower3, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 2))) {
            MOD_PHYREG(pi, crsminpowerlSub11, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerlSub11, crsmfminpower3, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 3))) {
            MOD_PHYREG(pi, crsminpoweruSub11, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpoweruSub11, crsmfminpower3, thresh);
        }
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 4))) {
            MOD_PHYREG(pi, crsminpowerULL1, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerULL1, crsmfminpower3, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 5))) {
            MOD_PHYREG(pi, crsminpowerULU1, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerULU1, crsmfminpower3, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 6))) {
            MOD_PHYREG(pi, crsminpowerUUL1, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUL1, crsmfminpower3, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 7))) {
            MOD_PHYREG(pi, crsminpowerUUU1, crsminpower3, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUU1, crsmfminpower3, thresh);
        }
    }
}

static void
wlc_phy_set_crs_min_pwr_clip2gain_acphy(phy_info_t *pi, uint8 thresh, bool sec_only)
{
    uint8 rxcrs_prim_loc = CHSPEC_SB(pi->radio_chanspec);

    if (!(sec_only && (rxcrs_prim_loc == 1))) {
        MOD_PHYREG(pi, crsminpoweru2, crsminpower4, thresh);
        MOD_PHYREG(pi, crsmfminpoweru2, crsmfminpower4, thresh);
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 0))) {
            MOD_PHYREG(pi, crsminpowerl2, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerl2, crsmfminpower4, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 2))) {
            MOD_PHYREG(pi, crsminpowerlSub12, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerlSub12, crsmfminpower4,  thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 3))) {
            MOD_PHYREG(pi, crsminpoweruSub12, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpoweruSub12, crsmfminpower4,  thresh);
        }
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 4))) {
            MOD_PHYREG(pi, crsminpowerULL2, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerlULL2, crsmfminpower4, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 5))) {
            MOD_PHYREG(pi, crsminpowerULU2, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerULU2, crsmfminpower4, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 6))) {
            MOD_PHYREG(pi, crsminpowerUUL2, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerlUUL2, crsmfminpower4, thresh);
        }
        if (!(sec_only && (rxcrs_prim_loc == 7))) {
            MOD_PHYREG(pi, crsminpowerUUU2, crsminpower4, thresh);
            MOD_PHYREG(pi, crsmfminpowerUUU2, crsmfminpower4, thresh);
        }
    }
}

#endif /* !WLC_DISABLE_ACI */

static void
wlc_phy_limit_rxgaintbl_acphy(uint8 gaintbl[], uint8 gainbitstbl[], uint8 sz,
    const uint8 default_gaintbl[], uint8 min_idx, uint8 max_idx)
{
    uint8 i;

    for (i = 0; i < sz; i++) {
        if (i < min_idx) {
            gaintbl[i] = default_gaintbl[min_idx];
            gainbitstbl[i] = min_idx;
        } else if (i > max_idx) {
            gaintbl[i] = default_gaintbl[max_idx];
            gainbitstbl[i] = max_idx;
        } else {
            gaintbl[i] = default_gaintbl[i];
            gainbitstbl[i] = i;
        }
    }
}

static void
wlc_phy_rxgainctrl_nbclip_acphy_tiny(phy_info_t *pi, uint8 core, int8 rxpwr_dBm)
{
    /* Multuply all pwrs by 10 to avoid floating point math */
    int16 rxpwrdBm_bw, pwr, tmp_pwr;
    int16 pwr_dBm[] = {-40, -40, -40};    /* 20, 40, 80 */
    int16 nb_thresh[] = { 0, 20, 40, 60, 80, 100, 120, 140, 160}; /* nb_thresh*10 avoid float */
    const char * const reg_name[ACPHY_NUM_NB_THRESH_TINY] = {"low", "low", "low", "mid", "mid",
                                                        "high", "high", "high", "high"};
    uint8 mux_sel[] = {0, 0, 0, 1, 1, 2, 2, 2, 2};
    uint8 reg_val[] = {0, 2, 4, 2, 4, 1, 3, 5, 7};
    uint8 nb, i;
    int v1, v2, vdiff1, vdiff2;
    int8 idx[ACPHY_MAX_RX_GAIN_STAGES] = {-1, -1, -1, -1, -1, -1, -1};
    uint16 initgain_codeA;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    bool use_w3_detector = FALSE; /* To chhose betwen NB(T2) and W3(T1) detectors */
    int16 t2_delta_dB = 0; /* the power delta between T2 and T1 detectors */
    int16 nb_thresh_end;

    ASSERT(core < PHY_CORE_MAX);
    ASSERT(TINY_RADIO(pi));

    rxpwrdBm_bw = (CHSPEC_IS80(pi->radio_chanspec) ||
            PHY_AS_80P80(pi, pi->radio_chanspec)) ? pwr_dBm[2] :
            (CHSPEC_IS160(pi->radio_chanspec)) ? 0 :
            (CHSPEC_IS40(pi->radio_chanspec)) ? pwr_dBm[1] : pwr_dBm[0];

    PHY_TRACE(("%s: adc pwr %d \n", __FUNCTION__, rxpwrdBm_bw));

    nb_thresh_end = nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1] + rxpwrdBm_bw;

    /* Get the INITgain code */
    initgain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);

    idx[0] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initExtLnaIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initExtLnaIndex);
    idx[1] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initLnaIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initLnaIndex);
    if (!ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev)) {
        idx[2] = (initgain_codeA &
            ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initlna2Index)) >>
            ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initlna2Index);
    }
    idx[3] = (initgain_codeA &
        ACPHY_REG_FIELD_MASK(pi, InitGainCodeA, core, initmixergainIndex)) >>
        ACPHY_REG_FIELD_SHIFT(pi, InitGainCodeA, core, initmixergainIndex);

    pwr = rxpwr_dBm;

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES - 2; i++) {
        if (idx[i] >= 0) {
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                tmp_pwr = pi_ac->rxgcrsi->rxgainctrl_params
                                [core].gaintbl[i][idx[i]];
                pwr += tmp_pwr;
            } else
                pwr += pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[i][idx[i]];
        }
    }

    if (pi_ac->rxgcrsi->curr_desense->elna_bypass == 1)
        pwr -= pi_ac->rxgcrsi->fem_rxgains[core].trloss;

    pwr *= 10;

    /* Use T1 detector bank if T2 bank is not covering pwr, and in 5G only (for the */
    /* time being). As it can be seen for T1 vs. T2 delta to be > 0 TIA index >4 is */
    /* needed, typically for the current 2G gain line up TIA index = 4 (BIQ 2 is off), */
    /* and T1 doesn't help. To make this work for 2G, it is needed to decresse LNA */
    /* index which can lower sensitivity. */
    if (pwr > nb_thresh_end) {
      use_w3_detector = TRUE;

      if (CHSPEC_IS80(pi->radio_chanspec) ||
        PHY_AS_80P80(pi, pi->radio_chanspec)) {
        if (idx[3] >= 8)
          t2_delta_dB = 75;
        else if (idx[3] >= 4)
          t2_delta_dB = 60;
        else
          t2_delta_dB = 0;
      } else if (CHSPEC_IS160(pi->radio_chanspec)) {
          ASSERT(0);
      } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        if (idx[3] >= 8)
          t2_delta_dB = 75;
        else if (idx[3] >= 5)
          t2_delta_dB = 60;
        else
          t2_delta_dB = 0;
      } else {
        if (idx[3] >= 5)
          t2_delta_dB = 60;
        else
          t2_delta_dB = 0;
      }

    }

    use_w3_detector = use_w3_detector && (t2_delta_dB > 0);
    for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY; i++) {
      nb_thresh[i] += (rxpwrdBm_bw + t2_delta_dB);
    }

    if (pwr < nb_thresh[0]) {
        nb = 0;
    } else if (pwr > nb_thresh[ACPHY_NUM_NB_THRESH_TINY - 1]) {
        nb = ACPHY_NUM_NB_THRESH_TINY - 1;
    } else {
        nb = 0;
        for (i = 0; i < ACPHY_NUM_NB_THRESH_TINY - 1; i++) {
            v1 = nb_thresh[i];
            v2 = nb_thresh[i + 1];
            if ((pwr >= v1) && (pwr <= v2)) {
                vdiff1 = pwr > v1 ? (pwr - v1) : (v1 - pwr);
                vdiff2 = pwr > v2 ? (pwr - v2) : (v2 - pwr);

                nb = (vdiff1 < vdiff2) ? i : i + 1;
                break;
            }
        }
    }

    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, wrssi3_ref_low_sel, 0);
        MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_mid_sel, 0);
        MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_high_sel, 0);
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_low_sel, 0);
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_mid_sel, 0);
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_high_sel, 0);
    }

    MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, mux_sel[nb]);
    if (use_w3_detector) {
      MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 1);
      if (strcmp(reg_name[nb], "low") == 0) {
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, wrssi3_ref_low_sel, reg_val[nb]);
      } else if (strcmp(reg_name[nb], "mid") == 0) {
        MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_mid_sel, reg_val[nb]);
      } else {
        MOD_RADIO_REG_TINY(pi, TIA_CFG12, core, wrssi3_ref_high_sel, reg_val[nb]);
      }
    } else {
      MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, 0);
      if (strcmp(reg_name[nb], "low") == 0) {
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_low_sel, reg_val[nb]);
      } else if (strcmp(reg_name[nb], "mid") == 0) {
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_mid_sel, reg_val[nb]);
      } else {
        MOD_RADIO_REG_TINY(pi, TIA_CFG13, core, nbrssi_ref_high_sel, reg_val[nb]);
      }
    }
}

/* ********************************************* */
/*                External Definitions                    */
/* ********************************************* */
/**********  DESENSE : ACI, NOISE, BT (start)  ******** */

void
wlc_phy_rxgainctrl_gainctrl_acphy_tiny(phy_info_t *pi, uint8 init_desense)
{
    /* at present this is just a place holder for
     * 'static' ELNA configuration. Eventually both TCL and
     * driver should be changes to follow the auto-calc
     * routine used in wlc_phy_rxgainctrl_gainctrl_acphy
     */
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi_ac->rxgcrsi;
    bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
                                                          : BF_ELNA_5G(pi_ac);
    uint8 max_analog_gain;
    int16 maxgain[ACPHY_MAX_RX_GAIN_STAGES] = {0, 0, 0, 0, -100, 125, 125};
    uint8 i, core;
    uint8 stall_val;
    bool init_trtx, hi_trtx, md_trtx, lo_trtx, lna1byp, clip2_trtx;
    bool lo_lna1byp_core, lo_lna1byp = FALSE, md_lna1byp = FALSE;
    int8 md_low_end, hi_high_end, lo_low_end, md_high_end;
    int8 clip2_gain;
    int8 hi_gain1, mid_gain1, lo_gain1;
    int8 nbclip_pwrdBm, w1clip_pwrdBm;
    int8 clip_gain[] = {61, 41, 19, 1};

    rxgcrsi->mdgain_trtx_allowed = FALSE;

    MOD_PHYREG(pi, RxSdFeConfig6, rx_farrow_rshift_0, 0);

    max_analog_gain = READ_PHYREGFLD(pi, Core0HpFBw, maxAnalogGaindb);

    /* fill in gain limits for analog stages */
    maxgain[0] = maxgain[1] = maxgain[2] = maxgain[3] = max_analog_gain;

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
        rxgcrsi->rxgainctrl_maxout_gains[i] = maxgain[i];

    /* Keep track of it (used in interf_mitigation) */
    rxgcrsi->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);

    init_trtx = elna_present & rxgcrsi->curr_desense->elna_bypass;
    hi_trtx = elna_present & rxgcrsi->curr_desense->elna_bypass;
    md_trtx = elna_present & (rxgcrsi->mdgain_trtx_allowed |
        rxgcrsi->curr_desense->elna_bypass);
    lo_trtx = elna_present;
    clip2_trtx = md_trtx;

    if (ACPHY_LO_NF_MODE_ELNA_TINY(pi)) {
        clip_gain[0] = ACPHY_INIT_GAIN_TINY;
        clip_gain[1] = 42;
        clip_gain[2] = 25;
        clip_gain[3] = 15;
    } else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev)) {
        clip_gain[0] = CHSPEC_IS2G(pi->radio_chanspec) ?
                ACPHY_INIT_GAIN_4365_2G : ACPHY_INIT_GAIN_4365_5G;
        clip_gain[1] = CHSPEC_IS2G(pi->radio_chanspec) ? 47 : 44;
        clip_gain[2] = 31;
        clip_gain[3] = 21;

        clip2_trtx = lo_trtx;
        if (!elna_present) {
            lo_lna1byp = TRUE;
        }
    } else {
        if (!elna_present || md_trtx) {
            clip_gain[2] = 19;
            clip_gain[3] = 1;
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                clip_gain[2] = 24;
                clip_gain[3] = 10;
            } else {
                clip_gain[2] = 24;
                clip_gain[3] = 15;
            }
        }
    }

    /* Reduce clip low gain to fix the hi-sens hit seen with post-rxfilter preemption */
    if (ACMAJORREV_4(pi->pubpi->phy_rev))
        clip_gain[3] = 5;

    if (ACPHY_ENABLE_FCBS_HWACI(pi))
        /* Apply desense */
        for (i = 0; i < 4; i++)
            clip_gain[i] -= rxgcrsi->curr_desense->clipgain_desense[i];

    /* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
        clip2_gain = clip2_trtx ? clip_gain[3] : (clip_gain[2] + clip_gain[3]) >> 1;
    else
        clip2_gain = md_trtx ? clip_gain[3] : (clip_gain[2] + clip_gain[3]) >> 1;

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    if (stall_val == 0)
        ACPHY_DISABLE_STALL(pi);

    FOREACH_CORE(pi, core) {
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            lna1byp = rxgcrsi->fem_rxgains[core].lna1byp;
            lo_lna1byp_core = lo_lna1byp | lna1byp;

            /* 0,1,2,3 for Init, hi, md and lo gains respectively */
            wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, clip_gain[0] -
                                                        init_desense, init_trtx,
                                                        FALSE, core);
            hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1, clip_gain[1],
                                                                   hi_trtx, FALSE,
                                                                   core);
            mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2, clip_gain[2],
                                                                    md_trtx, md_lna1byp,
                                                                    core);
            wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain, clip2_trtx,
                                                        FALSE, core);
            lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3, clip_gain[3],
                                                                   lo_trtx,
                                                                   lo_lna1byp_core,
                                                                   core);

            /* NB_CLIP */
            md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx,
                                                                md_lna1byp, core);
            hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx,
                                                                  FALSE, core);
            lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx,
                                                                lo_lna1byp_core, core);
            md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1,
                md_trtx, md_lna1byp, core);
        } else {
            /* not supported on Tiny yet */
            lna1byp = rxgcrsi->fem_rxgains[core].lna1byp;

            /* 0,1,2,3 for Init, hi, md and lo gains respectively */
            wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, clip_gain[0], init_trtx,
                                                        FALSE, core);

            hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1, clip_gain[1],
                                                                   hi_trtx, FALSE,
                                                                   core);

            mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2, clip_gain[2],
                                                                    md_trtx, FALSE,
                                                                    core);

            wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain, md_trtx,
                                                        FALSE, core);

            lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3, clip_gain[3],
                                                                   lo_trtx, lna1byp,
                                                                   core);

            /* NB_CLIP */
            md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx,
                    0, core);
            hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx,
                    0, core);
            lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx,
                    0, core);
            md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1,
                    md_trtx, 0, core);
        }
        if (ROUTER_4349(pi) && CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
            !BF_ELNA_5G(pi->u.pi_acphy)) {

            /* http://confluence.broadcom.com/display/WLAN/53573-Driver-GainRangeCurves
             * JIRAs: SWWLAN-87252: Fix for PER Hump at ~-35dBm in 5G 20 20in40 20in80
             * Issue: High-Gain is having PER hump at ~-35dBm
             * Fix: Moving the NbClip Thsh in such a way that Mid gain will be applied
             * in this region
             */
            hi_high_end = -33;
        }

        w1clip_pwrdBm = (lo_low_end + md_high_end) >> 1;

        /* -1 times pwr to avoid rounding off error */
        if (CHSPEC_IS20(pi->radio_chanspec)) {
            /*
             * use mid-point, as late clip2 is causing minor humps,
             * move nb/w1 clip to a lower pwr
             */
            nbclip_pwrdBm = (md_low_end + hi_high_end) >> 1;
        } else {
            /*
             * (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
             * low-end sensitivity degrades, keep more margin there.
             */
            nbclip_pwrdBm = (((-2 * md_low_end) + (-3 * hi_high_end)) * 13) >> 6;
            nbclip_pwrdBm = -nbclip_pwrdBm;
        }

        wlc_phy_rxgainctrl_nbclip_acphy_tiny(pi, core, nbclip_pwrdBm);
        wlc_phy_rxgainctrl_w1clip_acphy(pi, core, w1clip_pwrdBm);

        /* save threshold for gain index functions */
        if (md_trtx)
            /* assume 12 dB backoff */
          rxgcrsi->rx_elna_bypass_gain_th[core] = clip_gain[0] + (-13 - nbclip_pwrdBm/10);
        else if (lo_trtx)
          rxgcrsi->rx_elna_bypass_gain_th[core] = clip_gain[0] + (-13 - w1clip_pwrdBm/10);
        else
          rxgcrsi->rx_elna_bypass_gain_th[core] = -100;
    }
    ACPHY_ENABLE_STALL(pi, stall_val);

    /* Saving the values of Init gain to be used
     * in "wlc_phy_get_rxgain_acphy" function (used in rxiqest)
     */
    rxgcrsi->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, 0);
    rxgcrsi->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, 0);

    if ((phy_ac_noise_get_data(pi_ac->noisei)->hw_aci_status == FALSE) &&
        !ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev)) {
        if (rxgcrsi->fixed_gain_ncal != 0) {
            phy_ac_rxgcrs_setup_fixedgain_noisecal(rxgcrsi);
        }
    }
}

/* 43684+ normal-agc .
It calcualtes all the CLIP-gains, but for nb/w1 clip, we just select from
the mclip_rssi array (which is already available). Don't change any clip thresholds, as
those are used by mclip.
Idea is to use normal-agc for ACI mitigation, and so keep everything ready, without
changing any RSSI thresholds.
*/
void
wlc_phy_rxgainctrl_gainctrl_acphy_28nm(phy_info_t *pi, uint8 init_desense)
{
    /* at present this is just a place holder for
     * 'static' ELNA configuration. Eventually both TCL and
     * driver should be changes to follow the auto-calc
     * routine used in wlc_phy_rxgainctrl_gainctrl_acphy
     */
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi_ac->rxgcrsi;
    bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
                                                          : BF_ELNA_5G(pi_ac);
    uint8 i, core, stall_val;
    int byp_loss;
    bool init_trtx, hi_trtx, md_trtx, lo_trtx, lna1byp, clip2_trtx;
    bool lo_lna1byp_core, lo_lna1byp = FALSE, md_lna1byp = FALSE;
    int8 md_low_end, hi_high_end, lo_low_end, md_high_end;
    int8 clip2_gain;
    int8 hi_gain1, mid_gain1, lo_gain1;
    int8 nbclip_pwrdBm, w1clip_pwrdBm;
    const int8 clip_gain[] = {ACPHY_INIT_GAIN_28NM, ACPHY_HI_GAIN_28NM,
                              ACPHY_MD_GAIN_28NM, ACPHY_LO_GAIN_28NM};

    rxgcrsi->mdgain_trtx_allowed = FALSE;

    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
        rxgcrsi->rxgainctrl_maxout_gains[i] = 67;
    rxgcrsi->rxgainctrl_maxout_gains[ACPHY_MAX_RX_GAIN_STAGES-1] = 100;

    /* Keep track of it (used in interf_mitigation) */
    rxgcrsi->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);

    init_trtx = elna_present & rxgcrsi->curr_desense->elna_bypass;
    hi_trtx = elna_present & rxgcrsi->curr_desense->elna_bypass;
    md_trtx = elna_present & (rxgcrsi->mdgain_trtx_allowed |
        rxgcrsi->curr_desense->elna_bypass);
    lo_trtx = elna_present;
    clip2_trtx = md_trtx;

    /* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
    clip2_gain = clip2_trtx ? clip_gain[3] : (clip_gain[2] + clip_gain[3]) >> 1;

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    if (stall_val == 0)
        ACPHY_DISABLE_STALL(pi);

    FOREACH_CORE(pi, core) {
        lna1byp = rxgcrsi->fem_rxgains[core].lna1byp;
        lo_lna1byp_core = lo_lna1byp | lna1byp;
        byp_loss = rxgcrsi->curr_desense->elna_bypass ?
            pi_ac->rxgcrsi->fem_rxgains[core].trloss - rxgcrsi->fem_rxgains[core].elna : 0;

        /* 0,1,2,3 for Init, hi, md and lo gains respectively */
        wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, clip_gain[0] - byp_loss -
                                                    init_desense, init_trtx,
                                                    FALSE, core);
        hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1,
                                                               clip_gain[1] - byp_loss,
                                                               hi_trtx, FALSE,
                                                               core);
        mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2,
                                                                clip_gain[2] - byp_loss,
                                                                md_trtx, md_lna1byp,
                                                                core);
        wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain - byp_loss,
                                                    clip2_trtx, FALSE, core);
        lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3,
                                                               clip_gain[3] - byp_loss,
                                                               lo_trtx,
                                                               lo_lna1byp_core,
                                                               core);

        md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx,
                                                            md_lna1byp, core);
        hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx,
                                                              FALSE, core);
        lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx,
                                                            lo_lna1byp_core, core);
        md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1,
                                                              md_trtx, md_lna1byp, core);

        /*
         * (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
         * low-end sensitivity degrades, keep more margin there.
         */
        nbclip_pwrdBm = (-13 * ((2 * md_low_end) + (3 * hi_high_end))) >> 6;
        nbclip_pwrdBm = -nbclip_pwrdBm;

        w1clip_pwrdBm = (-13 * ((2 * lo_low_end) + (3 * md_high_end))) >> 6;
        w1clip_pwrdBm = -w1clip_pwrdBm;

        rxgcrsi->nbclip_dBm[core] = nbclip_pwrdBm;
        rxgcrsi->w1clip_dBm[core] = w1clip_pwrdBm;
    }

    /* Setup RSSI based on INITgain */
    wlc_phy_mclip_agc_rssi_setup(pi);

    FOREACH_CORE(pi, core) {
        /* Select nb/w1 clip muxes for normal AGC */
        wlc_phy_agc_nbw1mux_sel(pi, core, rxgcrsi->mclip_rssi[core]);
    }
    ACPHY_ENABLE_STALL(pi, stall_val);

    /* Saving the values of Init gain to be used
     * in "wlc_phy_get_rxgain_acphy" function (used in rxiqest)
     */
    rxgcrsi->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, 0);
    rxgcrsi->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, 0);
}

uint8
wlc_phy_get_max_lna_index_acphy(phy_info_t *pi, uint8 lna)
{
    uint8 max_idx, desense_state;
    acphy_desense_values_t *desense = NULL;
    uint8 elna_bypass, lna1_backoff, lna2_backoff;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    desense = pi_ac->rxgcrsi->total_desense;
    elna_bypass = desense->elna_bypass;
    lna1_backoff = desense->lna1_tbl_desense;
    lna2_backoff = desense->lna2_tbl_desense;

    desense_state = phy_ac_noise_get_desense_state(pi->u.pi_acphy->noisei);

    /* Find default max_idx */
    if (lna == ELNA_ID) {
        max_idx = elna_bypass;       /* elna */
    } else if (lna == LNA1_ID) {               /* lna1 */
        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            max_idx = rxg_params->lna1_max_indx;
            max_idx = MAX(0, max_idx  - lna1_backoff);
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            max_idx = desense_state != 0 ? desense->lna1_idx_max:
                ACPHY_4365_MAX_LNA1_IDX;
            max_idx = MAX(0, max_idx - lna1_backoff);
        } else {
            max_idx = MAX(0, ACPHY_MAX_LNA1_IDX  - lna1_backoff);
        }
    } else {                             /* lna2 */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (BF_ELNA_2G(pi_ac) && (elna_bypass == 0)) {
                uint8 core = 0;
                if (pi_ac->sromi->femrx_2g[core].elna > 10) {
                    max_idx = ACPHY_ELNA2G_MAX_LNA2_IDX;
                } else if (pi_ac->sromi->femrx_2g[core].elna > 8) {
                    max_idx = ACPHY_ELNA2G_MAX_LNA2_IDX_L;
                } else {
                    max_idx = ACPHY_ILNA2G_MAX_LNA2_IDX;
                }
            } else {
                max_idx = ACPHY_ILNA2G_MAX_LNA2_IDX;
            }
        } else {
            max_idx = (BF_ELNA_5G(pi_ac) && (elna_bypass == 0)) ?
                    ACPHY_ELNA5G_MAX_LNA2_IDX : ACPHY_ILNA5G_MAX_LNA2_IDX;
        }

        /* Fix max idx for 4349 to 3 */
        if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            max_idx = ACPHY_20693_MAX_LNA2_IDX;
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            max_idx = rxg_params->lna2_max_indx;
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            /* Fix max lna2 idx for 4365 to 1 */
            max_idx = desense_state != 0 ? desense->lna2_idx_max:
                ACPHY_4365_MAX_LNA2_IDX;
        }

        max_idx = MAX(0, max_idx - lna2_backoff);
    }

    return max_idx;
}

#ifndef WLC_DISABLE_ACI
void
wlc_phy_desense_apply_default_acphy(phy_info_t *pi)
{
    uint16 digigainlimit;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    /* when channel is changed, and the current channel is in mitigatation, then
       we need to restore the values. wlc_phy_rxgainctrl_gainctrl_acphy() takes
       care of all the gainctrl part, but we need to still restore back bphy regs
    */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev) ||
        (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev))) {
        wlc_phy_set_crs_min_pwr_higain_acphy(pi, ACPHY_CRSMIN_GAINHI, FALSE);

        if (ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS160(pi->radio_chanspec)) {
            wlc_phy_set_crs_min_pwr_mdgain_acphy(pi, ACPHY_CRSMIN_GAINHI, FALSE);
            wlc_phy_set_crs_min_pwr_logain_acphy(pi, ACPHY_CRSMIN_GAINHI, FALSE);
            wlc_phy_set_crs_min_pwr_clip2gain_acphy(pi, ACPHY_CRSMIN_GAINHI, FALSE);
        }
    } else {
        wlc_phy_set_crs_min_pwr_higain_acphy(pi, ACPHY_CRSMIN_DEFAULT, FALSE);
    }

    wlc_phy_desense_mf_high_thresh_acphy(pi, FALSE);
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            WRITE_PHYREG(pi, DigiGainLimit0, 0xC477);
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
            digigainlimit = READ_PHYREG(pi, DigiGainLimit0) & 0x8000;
            WRITE_PHYREG(pi, DigiGainLimit0, digigainlimit | 0x4477);
        } else {
            WRITE_PHYREG(pi, DigiGainLimit0, 0x4477);
        }

        WRITE_PHYREG(pi, PeakEnergyL, 0x10);
    }

    wlc_phy_desense_print_phyregs_acphy(pi, "restore");
    /* turn on LESI */
    if (pi_ac->rxgcrsi->lesi_cap) {
        phy_ac_rxgcrs_lesi(pi_ac->rxgcrsi, phy_ac_chanmgr_lesi_en(pi), 0);
    }
    /* channal update reset for default */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        wlc_phy_mlua_adjust_acphy(pi, FALSE);
    }
}

void
phy_ac_desense_update_crsmin(phy_info_t *pi, uint8 crsmin_init, int8 crsmin_high,
    int8 crsmin_md, int8 crsmin_lo, bool sec_only)
{
    uint8 crsmin_thresh;

    /* adjust crsmin threshold, 8 ticks increase gives 3dB rejection */
    crsmin_thresh = ACPHY_CRSMIN_DEFAULT + ((88 * crsmin_init) >> 5);  /* init gain */
#ifdef BCMLTECOEX
    if (phy_ac_btcx_get_data(pi->u.pi_acphy->btcxi)->ltecx_mode == 1)
        phy_ac_rxgcrs_set_crs_min_pwr(pi,
            MAX(crsmin_thresh, ACPHY_CRSMIN_DEFAULT),
                0, 0, 0, 0, sec_only);
    else
#endif
    if (ACMAJORREV_128(pi->pubpi->phy_rev) &&
        (wlapi_bmac_btc_mode_get(pi->sh->physhim) == 7)) {
        crsmin_thresh = MAX(0, pi->u.pi_acphy->rxgcrsi->phy_crs_th_from_crs_cal +
            ((88 * crsmin_init) >> 5));  /* init gain */
        phy_ac_rxgcrs_set_crs_min_pwr(pi, crsmin_thresh,
            0, 0, 0, 0, sec_only);
    } else {
        if (pi->u.pi_acphy->rxgcrsi->crsmincal_enable == TRUE) {
            phy_ac_rxgcrs_set_crs_min_pwr(pi, MAX(crsmin_thresh,
                pi->u.pi_acphy->rxgcrsi->phy_crs_th_from_crs_cal),
                0, 0, 0, 0, sec_only);
        }
    }

    if (ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS160(pi->radio_chanspec)) {
        /* conservatively set crs with clip2_gain
         * use the same threshold as init gain
         */
        if (pi->u.pi_acphy->rxgcrsi->crsmincal_enable == TRUE) {
            crsmin_thresh = MAX(crsmin_thresh,
                pi->u.pi_acphy->rxgcrsi->phy_crs_th_from_crs_cal);
        }
        wlc_phy_set_crs_min_pwr_clip2gain_acphy(pi, crsmin_thresh, sec_only);

        /* crs lo_gain */
        crsmin_thresh = ACPHY_CRSMIN_DEFAULT
            + MAX(0, ((88 * crsmin_lo) >> 5));
        wlc_phy_set_crs_min_pwr_logain_acphy(pi, crsmin_thresh, sec_only);

        /* crs md_gain */
        crsmin_thresh = ACPHY_CRSMIN_DEFAULT
            + MAX(0, ((88 * crsmin_md) >> 5));
        wlc_phy_set_crs_min_pwr_mdgain_acphy(pi, crsmin_thresh, sec_only);
    }

    /* crs high_gain */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        crsmin_thresh = MAX(ACPHY_CRSMIN_GAINHI,
                            ACPHY_CRSMIN_DEFAULT + ((88 * crsmin_high) >> 5));
    } else {
        crsmin_thresh = ACPHY_CRSMIN_DEFAULT + MAX(0, ((88 * crsmin_high) >> 5));
    }
    wlc_phy_set_crs_min_pwr_higain_acphy(pi, crsmin_thresh, sec_only);
}

void
phy_ac_desense_bphy(phy_info_t *pi, uint8 bphy_minshiftbits, uint16 bphy_peakenergy,
        uint8 bphy_desense)
{
    uint16 digigainlimit;
    uint8 crsmin_thresh;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if (!CHSPEC_IS2G(pi->radio_chanspec)) return;

    /* bphy desense */
    if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* adjust bphy crsmin, 16 ticks roughly gives 4dB desense */
        crsmin_thresh = (bphy_desense) > 0 ?
            ACPHY_BPHYCRSMIN + (bphy_desense<<2) : 0;
        MOD_PHYREG(pi, bphycrsminpower0, bphycrsminpower0, crsmin_thresh);
    } else {
        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            if (pi_ac->rxgcrsi->desense_forced_bphy) {
                crsmin_thresh = MIN(0xff, (bphy_desense) > 0 ?
                                    0x4a + ((83*bphy_desense) >> 4) : 0);
                MOD_PHYREG(pi, bphycrsminpower0, bphycrsminpower0, crsmin_thresh);
                /* Don't touch crsmin for HIgain unless requried ***
                   crsmin_high = MIN(0xff, MAX(0, bphy_desense + ACPHY_HI_GAIN_28NM
                   - ACPHY_INIT_GAIN_28NM) << 2);
                   MOD_PHYREG(pi, bphycrsminpower0, bphycrsminpower1, crsmin_high);
                */
                WRITE_PHYREG(pi, DigiGainLimit0, 0xc477);
                WRITE_PHYREG(pi, PeakEnergyL, 0x10);
            } else {
                WRITE_PHYREG(pi, bphycrsminpower0, 0);
                WRITE_PHYREG(pi, DigiGainLimit0,
                             0xC400 | bphy_minshiftbits);
                WRITE_PHYREG(pi, PeakEnergyL, bphy_peakenergy);
            }
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                   ACMAJORREV_33(pi->pubpi->phy_rev)) {
            digigainlimit = READ_PHYREG(pi, DigiGainLimit0) & 0x8000;
            WRITE_PHYREG(pi, DigiGainLimit0,
                         digigainlimit | 0x4400 | bphy_minshiftbits);
            WRITE_PHYREG(pi, PeakEnergyL, bphy_peakenergy);
        } else {
            WRITE_PHYREG(pi,  DigiGainLimit0,
                         0x4400 | bphy_minshiftbits);
            WRITE_PHYREG(pi, PeakEnergyL, bphy_peakenergy);
        }
    }
}
/*********** Desense (geneal) ********** */
/* IMP NOTE: make sure whatever regs are changed here are either:
1. reset back to default below OR
2. Updated in gainctrl()
*/
void
wlc_phy_desense_apply_acphy(phy_info_t *pi, bool apply_desense)
{
    /* reset:
       1 --> clear aci settings (the ones that gainctrl does not clear)
       0 --> apply aci_noise_bt mitigation
    */
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    acphy_desense_values_t *desense, *curr_desense;
    uint8 ofdm_desense, ofdm_sec_desense, bphy_desense, initgain_desense = 0;
    uint8 crsmin_init, crsmin_sec_init = 0;
    int8 crsmin_high = 0, crsmin_md = 0, crsmin_lo = 0;
    uint8 bphy_minshiftbits[] = {0x77, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04,
         0x04, 0x04, 0x04};
    uint16 bphy_peakenergy[]  = {0x10, 0x60, 0x10, 0x4c, 0x60, 0x30, 0x40, 0x40, 0x38, 0x2e,
         0x40, 0x34, 0x40};
    uint16 bphy_peakenergy_initgain0[] = {0x10, 0x60, 0x10, 0x4c, 0x60, 0x30, 0x40, 0x50,
      0x80, 0xa0, 0x100, 0x160, 0x200};
    uint8 bphy_initgain_backoff[] = {0, 0, 0, 0, 0, 0, 0, 3, 6, 9, 9, 12, 12};
    bool use_bphy_initgain_tbl = TRUE;
    uint8 max_bphy_shiftbits = ARRAYSIZE(bphy_minshiftbits);
    uint8 max_initgain_desense = 12, max_anlg_desense = 9;   /* only desnese bq0 */
    bool  ofdm_desense_extra_halfdB = 0, ofdm_sec_desense_extra_halfdB = 0;
    uint8 core, bphy_idx = 0, max_ofdm_desense, max_bphy_desense;
    bool call_gainctrl = FALSE;
    uint8 init_gain;
    int8 init_gain_change = 0;

    desense = pi_ac->rxgcrsi->total_desense;
    curr_desense = pi_ac->rxgcrsi->curr_desense;

    /* Don't use any initgain desense for 43684,
       as there is not much post tia intigain to backoff
    */
    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        use_bphy_initgain_tbl = FALSE;
    }

    wlapi_suspend_mac_and_wait(pi->sh->physhim);

    if (!apply_desense && !pi_ac->rxgcrsi->desense_forced_dB) {
        wlc_phy_desense_apply_default_acphy(pi);
    } else {
        /* Get total desense based on aci & bt & lte */
        wlc_phy_desense_calc_total_acphy(pi_ac->rxgcrsi);

        max_bphy_desense = MAX(ACPHY_ACI_MAX_DESENSE_BPHY_DB,
                               pi_ac->rxgcrsi->desense_forced_bphy);
        bphy_desense = MIN(max_bphy_desense, desense->bphy_desense);

        max_ofdm_desense = MAX(ACPHY_ACI_MAX_DESENSE_OFDM_DB,
                               pi_ac->rxgcrsi->desense_forced_ofdm);
        ofdm_desense = MIN(max_ofdm_desense, desense->ofdm_desense);
        ofdm_desense_extra_halfdB = desense->ofdm_desense_extra_halfdB;

        ofdm_sec_desense =
            MIN(max_ofdm_desense - ofdm_desense, desense->ofdm_sec_desense);
        ofdm_sec_desense_extra_halfdB = desense->ofdm_sec_desense_extra_halfdB;

        /* Make sure lesi is disabled if secondary desense is enabled */
        if (!((ofdm_sec_desense == 0) && (ofdm_sec_desense_extra_halfdB == 0)) &&
            pi_ac->rxgcrsi->lesi_cap) {
            ASSERT(ofdm_desense >= ACPHY_ACI_MAX_LESI_DESENSE_DB);
        }

        /* channal update set to interference mode */
        if (ofdm_desense > 0) {
          if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
            wlc_phy_mlua_adjust_acphy(pi, TRUE);
          }
        } else {
          if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
            wlc_phy_mlua_adjust_acphy(pi, FALSE);
          }
        }

        /* Update total desense */
        if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            desense->ofdm_desense = ofdm_desense;
            desense->bphy_desense = bphy_desense;
        }

        /* Initgain can change due to bphy desense */
        if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev))) &&
            (curr_desense->bphy_desense != bphy_desense))
            call_gainctrl = TRUE;

        /* Update current desense */
        curr_desense->ofdm_desense = ofdm_desense;
        curr_desense->bphy_desense = bphy_desense;
        curr_desense->ofdm_desense_extra_halfdB = ofdm_desense_extra_halfdB;
        curr_desense->ofdm_sec_desense = ofdm_sec_desense;
        curr_desense->ofdm_sec_desense_extra_halfdB = ofdm_sec_desense_extra_halfdB;

        /* if any ofdm desense is needed, first start using higher
           mf thresholds (1dB sens loss)
        */
        wlc_phy_desense_mf_high_thresh_acphy(pi, (ofdm_desense > 0));

        if (pi_ac->rxgcrsi->lesi_cap && phy_ac_chanmgr_lesi_en(pi)) {
            /* First ACPHY_ACI_MAX_LESI_DESENSE_DB-1 of desense is done using lesi_crs
               ACPHY_ACI_MAX_LESI_DESENSE_DB+ dB is done by turning off lesi
               remainder is done using initgain/crsmin (with lesi off)
            */
            if (ofdm_desense < ACPHY_ACI_MAX_LESI_DESENSE_DB) {
                phy_ac_rxgcrs_lesi(pi_ac->rxgcrsi, TRUE,
                                   (2*ofdm_desense) + ofdm_desense_extra_halfdB);
                ofdm_desense = 0; ofdm_desense_extra_halfdB = 0;
            } else {
                phy_ac_rxgcrs_lesi(pi_ac->rxgcrsi, FALSE, 0);
                ofdm_desense -= ACPHY_ACI_MAX_LESI_DESENSE_DB;
            }
        } else {
            /* 1st dB is for mf_high thresh */
            if (ofdm_desense > 0) ofdm_desense --;
        }

        /* Distribute desense between INITgain & crsmin (ofdm) & digigain (bphy) */
        if (CHSPEC_IS2G(pi->radio_chanspec) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
            /* round to 2, as bphy desnese table is in 2dB steps */
            bphy_idx = MIN((bphy_desense + 1) >> 1, max_bphy_shiftbits - 1);
            if (use_bphy_initgain_tbl) {
                initgain_desense = bphy_initgain_backoff[bphy_idx];
            }
        }

#ifdef BCMLTECOEX
        if (phy_ac_btcx_get_data(pi_ac->btcxi)->ltecx_mode == 1)
            initgain_desense = ofdm_desense;
        else
#endif
        initgain_desense = MIN(initgain_desense, max_initgain_desense);
        desense->analog_gain_desense_ofdm = MAX(desense->analog_gain_desense_ofdm,
            MIN(max_anlg_desense, ofdm_desense));

        if (ACPHY_HWACI_WITH_DESENSE_ENG(pi) && CHSPEC_IS2G(pi->radio_chanspec)) {
            desense->clipgain_desense[0] = MAX(desense->clipgain_desense[0],
                initgain_desense);
            if (curr_desense->clipgain_desense[0] != desense->clipgain_desense[0])
            {
                init_gain_change = desense->clipgain_desense[0] -
                    curr_desense->clipgain_desense[0];

                curr_desense->clipgain_desense[0] =
                    desense->clipgain_desense[0];
                call_gainctrl = TRUE;
            }
        }

        /* If HWACI triggers on signal on which we do not glitch, then
        * GBD_desense will be 0 and CRS will be controlled by crs_min_pwr_cal.
        * Under such scenario, CRS caled value (calculated without HWACI desense)
        * should be compensated for any change in INIT gain.
        */
        if (init_gain_change > 0) {
            if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                pi->u.pi_acphy->rxgcrsi->phy_crs_th_from_crs_cal -=
                ((88 * init_gain_change) >> 5);
            }
        }

        /* OFDM Desense */
        /* With init gain, max crsmin desense = 30dB, after which ADC will start clipping */
        /* With HI gain, crsmin desense = new_sens - old_sens
           = (-96 + desense) - (0 - (higain + 27))
           = -69 + desense + higain
        */

        /*  In TINY, clipping happens close to -60 dBm. SO Max desense of crsmin_init
         *  should be increased from 30 to 35 dB.
         *
         *  crsmin_high is changed to take care of any change in default HI or INIT gain.
        */
        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            crsmin_init = MIN(35, MAX(0, ofdm_desense - desense->clipgain_desense[0]));
            crsmin_high = MAX(0, ofdm_desense + ACPHY_HI_GAIN - 69);
        } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
            crsmin_init = MIN(35, MAX(0, ofdm_desense - desense->clipgain_desense[0]));
            crsmin_sec_init =
                MIN(35, MAX(0, ofdm_desense + ofdm_sec_desense -
                            desense->clipgain_desense[0]));
            if (pi_ac->rxgcrsi->rxg_params->mclip_agc_en) {
                /* Don't touch md/lo for mclips, and max 6dBs for hi */
                crsmin_high = MIN(6, ofdm_desense + ACPHY_HI_GAIN_28NM  -
                                  ACPHY_INIT_GAIN_28NM -
                                  desense->clipgain_desense[1]);
            } else {
                crsmin_high = ofdm_desense + ACPHY_HI_GAIN_28NM  -
                    ACPHY_INIT_GAIN_28NM - desense->clipgain_desense[1];
                crsmin_md = ofdm_desense + ACPHY_MD_GAIN_28NM  -
                    ACPHY_INIT_GAIN_28NM - desense->clipgain_desense[2];
                crsmin_lo = ofdm_desense + ACPHY_LO_GAIN_28NM  -
                    ACPHY_INIT_GAIN_28NM - desense->clipgain_desense[3];
            }
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            init_gain = CHSPEC_IS2G(pi->radio_chanspec) ?
                    ACPHY_INIT_GAIN_4365_2G : ACPHY_INIT_GAIN_4365_5G;
            crsmin_init = MIN(35, MAX(0, ofdm_desense - desense->clipgain_desense[0]));
            crsmin_high = ofdm_desense + ACPHY_HI_GAIN_TINY -
                init_gain - desense->clipgain_desense[1];
        } else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
            /* changing the max allowed desense in init gain  from 35 to 41
                since ACPHY_CRSMIN_DEFAULT threshold on top of which the desense
                is applied is set to 6dB lower power
            */
            crsmin_init = MIN(41, MAX(0, ofdm_desense - desense->clipgain_desense[0]));

            crsmin_high = MAX(0, ofdm_desense + ACPHY_HI_GAIN_TINY -
                ACPHY_INIT_GAIN_TINY - desense->clipgain_desense[1]);
        } else {
            crsmin_init = MAX(0, ofdm_desense - initgain_desense);
            crsmin_high = ofdm_desense + ACPHY_HI_GAIN - 69;
        }
        PHY_ACI(("aci_mode1, desense, init = %d, bphy_idx = %d, crsmin = {%d %d %d %d}\n",
                 initgain_desense, bphy_idx, crsmin_init,
                 crsmin_high, crsmin_md, crsmin_lo));

        if ((curr_desense->elna_bypass != desense->elna_bypass) ||
            (curr_desense->lna1_gainlmt_desense != desense->lna1_gainlmt_desense) ||
            (curr_desense->lna1_tbl_desense != desense->lna1_tbl_desense) ||
            (curr_desense->lna2_tbl_desense != desense->lna2_tbl_desense) ||
            (curr_desense->lna2_gainlmt_desense != desense->lna2_gainlmt_desense) ||
            (curr_desense->mixer_setting_desense != desense->mixer_setting_desense) ||
            (curr_desense->analog_gain_desense_ofdm != desense->analog_gain_desense_ofdm) ||
            (curr_desense->analog_gain_desense_bphy != desense->analog_gain_desense_bphy)) {
            if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
                /* Update the current structure to hold the new desense values */
                memcpy(curr_desense, desense, sizeof(acphy_desense_values_t));
                wlc_phy_rxgainctrl_set_gaintbls_acphy(pi, TRUE, TRUE, TRUE);
            } else {
                wlc_phy_upd_lna1_lna2_gains_acphy(pi);
            }
            call_gainctrl = TRUE;
        }

        /* if lna1/lna2 gaintable has changed, call gainctrl as it effects all clip gains */
        if (call_gainctrl) {
            if (TINY_RADIO(pi)) {
                if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev))
                    wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi,
                        initgain_desense);
                else
                    wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
            } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                wlc_phy_agc_28nm(pi, TRUE, TRUE, TRUE); // TODO: Need to check
            } else {
                wlc_phy_rxgainctrl_gainctrl_acphy(pi);
            }
        }
#ifdef OCL
    if (PHY_OCL_ENAB(pi->sh->physhim)) {
        /* update ocl init gain rfseq if init gain desense is done */
        phy_ac_ocl_config(pi_ac->ocli);
    }
#endif /* OCL */

    if (!TINY_RADIO(pi) && !IS_28NM_RADIO(pi) && !IS_16NM_RADIO(pi)) {
            /* Update INITgain */
            FOREACH_CORE(pi, core) {
                wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0,
                                                            ACPHY_INIT_GAIN
                                                            - initgain_desense,
                                                            desense->elna_bypass,
                                                            FALSE, core);
            }
        }

        phy_ac_desense_update_crsmin(pi, crsmin_init, crsmin_high,
                                     crsmin_md, crsmin_lo, FALSE);

        // Apply secondary desense
        // Only 43684 of phybw 160MHz with secondary desense cap
        if (ACMAJORREV_47(pi->pubpi->phy_rev) && CHSPEC_IS160(pi->radio_chanspec)) {
            if ((ofdm_sec_desense_extra_halfdB != 0) &&
                (ofdm_sec_desense_extra_halfdB != 0)) {
                ofdm_sec_desense = ofdm_sec_desense + 1;
            }

            if (ofdm_sec_desense != 0) {
                crsmin_high = crsmin_high + ofdm_sec_desense;
                crsmin_md = crsmin_md + ofdm_sec_desense;
                crsmin_lo = crsmin_lo + ofdm_sec_desense;
                phy_ac_desense_update_crsmin(pi, crsmin_sec_init, crsmin_high,
                    crsmin_md, crsmin_lo, TRUE);
            }
        }

        if (use_bphy_initgain_tbl) {
            phy_ac_desense_bphy(pi, bphy_minshiftbits[bphy_idx],
                                bphy_peakenergy[bphy_idx],
                                bphy_desense);
        } else {
            phy_ac_desense_bphy(pi, bphy_minshiftbits[bphy_idx],
                                bphy_peakenergy_initgain0[bphy_idx],
                                bphy_desense);
        }

        wlc_phy_desense_print_phyregs_acphy(pi, "apply");
    }

    /* Inform rate contorl to slow down is mitigation is on */
    wlc_phy_aci_updsts_acphy(pi);

    wlapi_enable_mac(pi->sh->physhim);
}

static void
wlc_phy_forced_desense_ofssets(phy_ac_rxgcrs_info_t *rxgcrsi, int8 offs[])
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac =  rxgcrsi->aci;
    int8 bphy_off = 0, ofdm_off = 0;

    /* adjust per chip */
    if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS5G(pi->radio_chanspec)) ofdm_off = 5;
    } else if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        bphy_off = BF_ELNA_2G(pi_ac) ? 2 : 1;
        if (CHSPEC_IS2G(pi->radio_chanspec))
            ofdm_off = BF_ELNA_2G(pi_ac) ? 5 : 4;
        else
            ofdm_off = BF_ELNA_5G(pi_ac) ? 3 : 0;
    }

    /* Detection offset wrt to 20mhz */
    if (CHSPEC_IS40(pi->radio_chanspec)) {
        ofdm_off += -2;
    } else if (CHSPEC_IS80(pi->radio_chanspec)) {
        ofdm_off += -4;
    } else if (CHSPEC_IS160(pi->radio_chanspec)) {
        ofdm_off += -5;
    }

    offs[0] = bphy_off;
    offs[1] = ofdm_off;
}

void
wlc_phy_desense_calc_total_acphy(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    int8 bphy_ofdm_diff, bphy_off, ofdm_off, offs[2];
    phy_info_t *pi = rxgcrsi->pi;
    uint8 i;

#ifdef BCMLTECOEX
    acphy_desense_values_t *bt, *aci, *lte;
#else
    acphy_desense_values_t *bt, *aci;
#endif

    acphy_desense_values_t *total = rxgcrsi->total_desense;

    if ((aci = phy_ac_noise_get_desense(rxgcrsi->aci->noisei)) == NULL) {
        aci = rxgcrsi->zero_desense;
    }

#ifdef BCMLTECOEX
    if ((phy_ac_btcx_get_data(rxgcrsi->aci->btcxi)->btc_mode == 0 &&
        phy_ac_btcx_get_data(rxgcrsi->aci->btcxi)->ltecx_mode == 0) ||
        wlc_phy_is_scan_chan_acphy(pi) || CHSPEC_ISPHY5G6G(pi->radio_chanspec) ||
        ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* only consider aci desense */
        memcpy(total, aci, sizeof(acphy_desense_values_t));
    } else {
        /* Merge BT & ACI & LTE desense, take max */
        bt  = rxgcrsi->bt_desense;
        lte = rxgcrsi->lte_desense;
        total->ofdm_desense = MAX(MAX(aci->ofdm_desense, bt->ofdm_desense),
            lte->ofdm_desense);
        total->ofdm_sec_desense = aci->ofdm_sec_desense;
        total->bphy_desense = MAX(MAX(aci->bphy_desense, bt->bphy_desense),
            lte->bphy_desense);
        total->lna1_tbl_desense = MAX(MAX(aci->lna1_tbl_desense, bt->lna1_tbl_desense),
            lte->lna1_tbl_desense);
        total->analog_gain_desense_ofdm = MAX(MAX(aci->analog_gain_desense_ofdm,
            bt->analog_gain_desense_ofdm), lte->analog_gain_desense_ofdm);
        total->analog_gain_desense_bphy = MAX(MAX(aci->analog_gain_desense_bphy,
            bt->analog_gain_desense_bphy), lte->analog_gain_desense_bphy);
        total->lna2_tbl_desense = MAX(MAX(aci->lna2_tbl_desense, bt->lna2_tbl_desense),
            lte->lna2_tbl_desense);
        total->lna1_gainlmt_desense = MAX(MAX(aci->lna1_gainlmt_desense,
            bt->lna1_gainlmt_desense), lte->lna1_gainlmt_desense);
        total->lna2_gainlmt_desense = MAX(MAX(aci->lna2_gainlmt_desense,
            bt->lna2_gainlmt_desense), lte->lna2_gainlmt_desense);
        total->elna_bypass = MAX(MAX(aci->elna_bypass, bt->elna_bypass),
            lte->elna_bypass);
        total->mixer_setting_desense = MAX(MAX(aci->mixer_setting_desense,
            bt->mixer_setting_desense), lte->mixer_setting_desense);
        total->nf_hit_lna12 =  MAX(MAX(aci->nf_hit_lna12, bt->nf_hit_lna12),
            lte->nf_hit_lna12);
        total->on = aci->on | bt->on | lte->on;
        for (i = 0; i < 4; i++)
            total->clipgain_desense[i] = MAX(MAX(aci->clipgain_desense[i],
                bt->clipgain_desense[i]), lte->clipgain_desense[i]);
    }
#else
    if ((phy_ac_btcx_get_data(rxgcrsi->aci->btcxi)->btc_mode == 0) ||
        wlc_phy_is_scan_chan_acphy(pi) || CHSPEC_ISPHY5G6G(pi->radio_chanspec) ||
        ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev))) {
        /* only consider aci desense */
        memcpy(total, aci, sizeof(acphy_desense_values_t));
    } else {
        /* Merge BT & ACI desense, take max */
        bt  = rxgcrsi->bt_desense;
        total->ofdm_desense = MAX(aci->ofdm_desense, bt->ofdm_desense);
        total->ofdm_sec_desense = aci->ofdm_sec_desense;
        total->bphy_desense = MAX(aci->bphy_desense, bt->bphy_desense);
        total->analog_gain_desense_ofdm = MAX(aci->analog_gain_desense_ofdm,
            bt->analog_gain_desense_ofdm);
        total->analog_gain_desense_bphy = MAX(aci->analog_gain_desense_bphy,
            bt->analog_gain_desense_bphy);
        total->lna1_tbl_desense = MAX(aci->lna1_tbl_desense, bt->lna1_tbl_desense);
        total->lna2_tbl_desense = MAX(aci->lna2_tbl_desense, bt->lna2_tbl_desense);
        total->lna1_gainlmt_desense =
          MAX(aci->lna1_gainlmt_desense, bt->lna1_gainlmt_desense);
        total->lna2_gainlmt_desense =
          MAX(aci->lna2_gainlmt_desense, bt->lna2_gainlmt_desense);
        total->mixer_setting_desense =
          MAX(aci->mixer_setting_desense, bt->mixer_setting_desense);
        total->elna_bypass = MAX(aci->elna_bypass, bt->elna_bypass);
        total->nf_hit_lna12 =  MAX(aci->nf_hit_lna12, bt->nf_hit_lna12);
        total->on = aci->on | bt->on;
        for (i = 0; i < 4; i++)
            total->clipgain_desense[i] = MAX(aci->clipgain_desense[i],
                bt->clipgain_desense[i]);
    }
#endif /* BCMLTECOEX */

        /* Take Max if forced */
        rxgcrsi->desense_forced_ofdm = 0;
        rxgcrsi->desense_forced_bphy = 0;
        if (!wlc_phy_is_scan_chan_acphy(pi)) {
            /* get per chip/bang/bw offsets */
            wlc_phy_forced_desense_ofssets(rxgcrsi, offs);
            bphy_off = offs[0];
            ofdm_off = offs[1];

            bphy_ofdm_diff = (pi->u.pi_acphy->rxgcrsi->lesi_cap ?
                              ACPHY_ACI_MAX_LESI_DESENSE_DB : 0) -  BPHY_OFDM_SEN_DIFF;

            if (rxgcrsi->desense_forced_dB > 0)
                rxgcrsi->desense_forced_bphy = rxgcrsi->desense_forced_dB +
                    bphy_off;
            if (rxgcrsi->desense_forced_dB > 0)
                rxgcrsi->desense_forced_ofdm = MAX(0, rxgcrsi->desense_forced_dB +
                                                   bphy_ofdm_diff + ofdm_off);

            if (CHSPEC_IS2G(pi->radio_chanspec))
                total->bphy_desense = MAX(total->bphy_desense,
                                          rxgcrsi->desense_forced_bphy);

            if (total->ofdm_desense < rxgcrsi->desense_forced_ofdm) {
                total->ofdm_desense = rxgcrsi->desense_forced_ofdm;
                total->ofdm_desense_extra_halfdB = 0;
            }
        }

        /* uCode detected high pwr RSSI. Time to save ilna1 */
        if (phy_ac_hirssi_set(pi) == TRUE)
            total->elna_bypass = TRUE;

}
#endif /* #ifndef WLC_DISABLE_ACI */

void
wlc_phy_rxgainctrl_gainctrl_acphy(phy_info_t *pi)
{
    bool elna_present;
    bool init_trtx, hi_trtx, md_trtx, lo_trtx, lna1byp;
    uint8 init_gain = ACPHY_INIT_GAIN, hi_gain = ACPHY_HI_GAIN;
    uint8 mid_gain = 35, lo_gain, clip2_gain;
    uint8 hi_gain1, mid_gain1, lo_gain1;
    /* 1% PER point used for all the PER numbers */

    /* For bACI/ACI: max output pwrs {elna, lna1, lna2, mix, bq0, bq1, dvga} */
    uint8 maxgain_2g[] = {43, 43, 43, 52, 52, 100, 0};
    uint8 maxgain_5g[] = {47, 47, 47, 52, 52, 100, 0};

    uint8 i, core, elna_idx;
    int8 md_low_end, hi_high_end, lo_low_end, md_high_end, max_himd_hi_end;
    int8 nbclip_pwrdBm, w1clip_pwrdBm;
    phy_ac_rxgcrs_info_t *ri = pi->u.pi_acphy->rxgcrsi;
    bool elnabyp_en = phy_ac_hirssi_get(pi);

    ri->mdgain_trtx_allowed = TRUE;
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
            ri->rxgainctrl_maxout_gains[i] = maxgain_2g[i];
        elna_present = BF_ELNA_2G(ri->aci);
    } else {
        for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
            ri->rxgainctrl_maxout_gains[i] = maxgain_5g[i];
        elna_present = BF_ELNA_5G(ri->aci);
    }

    /* Keep track of it (used in interf_mitigation) */
    ri->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);
    init_trtx = elna_present & ri->curr_desense->elna_bypass;
    hi_trtx = elna_present & ri->curr_desense->elna_bypass;
    md_trtx = elna_present & (ri->mdgain_trtx_allowed | ri->curr_desense->elna_bypass);
    lo_trtx = TRUE;

    /* with elna if md_gain uses TR != T, then LO_gain needs to be higher */
    if (elnabyp_en)
        lo_gain = ((!elna_present) || md_trtx) ? 15 : 30;
    else
        lo_gain = ((!elna_present) || md_trtx) ? 20 : 30;
    clip2_gain = md_trtx ? lo_gain : (mid_gain + lo_gain) >> 1;

    FOREACH_CORE(pi, core) {
        lna1byp = ri->fem_rxgains[core].lna1byp;
        if (lna1byp) {
            mid_gain = 35;
            lo_gain = 15;
            PHY_INFORM((" iPa chip, set lo_gain = 15 \n"));
            hi_gain = 48;
        }
        elna_idx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        max_himd_hi_end =
            - 16 - ri->rxgainctrl_params[core].gaintbl[0][elna_idx];
        if (ri->curr_desense->elna_bypass == 1)
            max_himd_hi_end += ri->fem_rxgains[core].trloss;

        /* 0,1,2,3 for Init, hi, md and lo gains respectively */
        wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, init_gain, init_trtx,
            FALSE, core);
        hi_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 1, hi_gain,
            hi_trtx, FALSE, core);
        mid_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 2, mid_gain,
            md_trtx, FALSE, core);
        wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 4, clip2_gain, md_trtx,
            FALSE, core);
        lo_gain1 = wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 3, lo_gain,
            lo_trtx, lna1byp, core);

        /* NB_CLIP */
        md_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, mid_gain1, md_trtx, 0,
                core);
        hi_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, hi_gain1, hi_trtx, 0,
                core);
        lo_low_end = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, lo_gain1, lo_trtx, 0,
                core);
        md_high_end = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, mid_gain1, md_trtx, 0,
                core);

        /* -1 times pwr to avoid rounding off error */
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            /* use mid-point, as late clip2 is causing minor humps,
               move nb/w1 clip to a lower pwr
            */
            nbclip_pwrdBm = (md_low_end + hi_high_end) >> 1;
        } else {
            /* (0.4*md/lo + 0.6*hi/md) fixed point. On fading channels,
               low-end sensitivity degrades, keep more margin there.
            */
            nbclip_pwrdBm = (((-2*md_low_end)+(-3*hi_high_end)) * 13) >> 6;
            nbclip_pwrdBm *= -1;
            if (CHSPEC_IS80(pi->radio_chanspec) &&
                ((ACMAJORREV_2(pi->pubpi->phy_rev)) ||
                 ACMAJORREV_5(pi->pubpi->phy_rev))) {
                nbclip_pwrdBm -= 4;
            } else if (CHSPEC_IS80(pi->radio_chanspec) &&
                       ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) && PHY_ILNA(pi)) &&
                       (RADIOREV(pi->pubpi->radiorev) == 0x2C))) {
                /* to cure the 20ll PER hump issue for 43569a2/43570a2
                 * http://confluence.broadcom.com/x/NC-UEg
                 */
                nbclip_pwrdBm -= 6;
            }
        }

        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            if (elnabyp_en) {
                w1clip_pwrdBm = (((-2*lo_low_end) + (-3*md_high_end)) * 13) >> 6;
                w1clip_pwrdBm *= -1;
            } else {
                w1clip_pwrdBm = (lo_low_end + md_high_end) >> 1;
            }
        } else {
            w1clip_pwrdBm = (((-2*lo_low_end) + (-3*md_high_end)) * 13) >> 6;
            w1clip_pwrdBm *= -1;
        }
        if ((ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_3(pi) &&
            !ACMINORREV_5(pi) && !PHY_ILNA(pi)) &&
            !(CHSPEC_IS2G(pi->radio_chanspec) && ri->w1clipmod)) {
            /* Do not increase w1clip to prevent LNA1 clamping for 2G */
            w1clip_pwrdBm += 3;
        }

        wlc_phy_rxgainctrl_nbclip_acphy(pi, core, nbclip_pwrdBm);
        wlc_phy_rxgainctrl_w1clip_acphy(pi, core, w1clip_pwrdBm);
    }

    /* After coming out of this loop, gain ctrl is done. So, values of Init gain in the phyregs
     * are the correct/default ones. These should be the ones that should be used in
     * "wlc_phy_get_rxgain_acphy" function (used in rxiqest). So, we have to save the default
     * Init gain phyregs in some variables. We assume, that both core's init gains will be same
     * and thus save only core 0's init gain and this should be fine for single core chips too.
     */
    if (ri->fixed_gain_ncal != 0) {
        phy_ac_rxgcrs_setup_fixedgain_noisecal(ri);
    }
}

void
wlc_phy_upd_lna1_lna2_gains_acphy(phy_info_t *pi)
{
    wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 1);
    wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(pi, 2);
    wlc_phy_upd_lna1_lna2_gaintbls_acphy(pi, 1);
    wlc_phy_upd_lna1_lna2_gaintbls_acphy(pi, 2);
}

void
wlc_phy_upd_lna1_lna2_gaintbls_acphy(phy_info_t *pi, uint8 lna12)
{
    uint8 offset, sz, core, desense_state;
    uint8 gaintbl[10];
    uint8 gainbitstbl[10], lna2rout_offset;
    uint8 max_idx, min_idx;
    const uint8 *default_gaintbl = NULL;
    uint16 gain_tblid, gainbits_tblid;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;
    acphy_desense_values_t *desense = rxgcrsi->total_desense;
    phy_ac_rxg_params_t *rxg_params = rxgcrsi->rxg_params;
    uint8 lna1rout_tbl[6], lna1rout_offset;
    uint8 lna1, lna1_idx, lna1_rout;
    uint8 stall_val;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if ((lna12 < LNA1_ID) || (lna12 > LNA2_ID)) return;

    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    sz = rxgcrsi->rxgainctrl_stage_len[lna12];

    desense_state = phy_ac_noise_get_desense_state(pi->u.pi_acphy->noisei);

    if (lna12 == LNA1_ID) {          /* lna1 */
        offset = 8;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                default_gaintbl = (uint8 *)(rxg_params->rftbls->lna12_gain_tbl[0]);
            } else    if (!TINY_RADIO(pi)) {
                if (PHY_ILNA(pi)) { /* iLNA only chip */
                    default_gaintbl = ac_lna1_2g_43352_ilna;
                } else {
                    default_gaintbl = ac_lna1_2g;
                }
            } else {
                default_gaintbl = ac_lna1_2g_tiny;
            }
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            default_gaintbl = (uint8 *)(rxg_params->rftbls->lna12_gain_tbl[0]);
        } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            default_gaintbl = ac_lna1_5g_4365;
        } else {
            if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
                /* Undo LNA1 bypass that could be set by the desense code */
                /* Note: Leaving this on for 5G gives strange effects */
                wlc_phy_lna1bypass_acphy(pi, 0);
            }
            default_gaintbl = (TINY_RADIO(pi)) ? ac_lna1_5g_tiny : ac_lna1_5g;
        }
    } else {  /* lna2 */
        offset = 16;
        if (TINY_RADIO(pi)) {
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                default_gaintbl = CHSPEC_IS2G(pi->radio_chanspec) ?
                        ac_lna2_tiny_4365_2g : ac_lna2_tiny_4365_5g;
            } else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                default_gaintbl = ac_lna2_tiny_4349;
            } else {
                default_gaintbl = ac_lna2_tiny;
            }
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            default_gaintbl = (uint8 *) rxg_params->rftbls->lna12_gain_tbl[1];
        } else if (CHSPEC_IS2G(pi->radio_chanspec)) {
            if (PHY_ILNA(pi)) { /* iLNA only chip */
                /* note gaintbl is same as ac_lna2_2g_ilna */
                default_gaintbl = ac_lna2_2g_43352_ilna;
            } else {
                default_gaintbl = (desense->elna_bypass)
                    ? ac_lna2_2g_gm3 : ac_lna2_2g_gm2;
            }
        } else {
            default_gaintbl = ac_lna2_5g;
        }
        memcpy(rxgcrsi->lna2_complete_gaintbl, default_gaintbl, sizeof(uint8)*sz);
    }

    max_idx = wlc_phy_get_max_lna_index_acphy(pi, lna12);
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        if (desense_state != 0)
            min_idx = (lna12 == LNA1_ID) ? desense->lna1_idx_min: desense->lna2_idx_min;
        else
            min_idx = (lna12 == LNA1_ID) ? 0 : max_idx;
    } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        min_idx = (lna12 == LNA1_ID) ? rxg_params->lna1_min_indx :
            rxg_params->lna2_min_indx;
    } else {
        min_idx = ACMAJORREV_4(pi->pubpi->phy_rev) ? max_idx
            : ((pi->pubpi->phy_rev == 0) ? 1 : 0);
    }
    wlc_phy_limit_rxgaintbl_acphy(gaintbl, gainbitstbl, sz, default_gaintbl, min_idx, max_idx);

    /* Update rxgcrsi->curr_desense (used in interf_mitigation) */
    if (lna12 == LNA1_ID) {
        rxgcrsi->curr_desense->lna1_tbl_desense = desense->lna1_tbl_desense;
    } else {
        rxgcrsi->curr_desense->lna2_tbl_desense = desense->lna2_tbl_desense;
    }

    if (TINY_RADIO(pi)) {
        uint8 x;
        uint8 lnarout_val, lnarout_val2G, lnarout_val5G, lna2rout_val;

        if (ACMAJORREV_4(pi->pubpi->phy_rev) || ACMAJORREV_32(pi->pubpi->phy_rev) ||
            ACMAJORREV_33(pi->pubpi->phy_rev)) {
            FOREACH_CORE(pi, core) {
                for (x = 0; x < 6; x++) {
                    lnarout_val2G = (ac_tiny_g_lna_rout_map[x] << 3) |
                        ac_tiny_g_lna_gain_map[x];
                    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                        ACMAJORREV_33(pi->pubpi->phy_rev))
                        lnarout_val5G = (ac_4365_a_lna_rout_map[x] << 3) |
                                ac_tiny_a_lna_gain_map[x];
                    else
                        lnarout_val5G = (ac_tiny_a_lna_rout_map[x] << 3) |
                            ac_tiny_a_lna_gain_map[x];
                    lnarout_val = CHSPEC_ISPHY5G6G(pi->radio_chanspec) ?
                        lnarout_val5G : lnarout_val2G;
                    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1,
                        (ACPHY_LNAROUT_BAND_OFFSET(pi,
                        pi->radio_chanspec)    + x +
                        ACPHY_LNAROUT_CORE_WRT_OFST(pi->pubpi->phy_rev,
                        core)),    8, &lnarout_val);
                    if (!ACMAJORREV_32(pi->pubpi->phy_rev) &&
                        !ACMAJORREV_33(pi->pubpi->phy_rev)) {
                        lna2rout_val =  wlc_phy_get_max_lna_index_acphy(pi,
                            LNA2_ID);
                        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                            1, (x + 16 +
                            ACPHY_LNAROUT_CORE_WRT_OFST(
                            pi->pubpi->phy_rev, core)), 8,
                            &lna2rout_val);
                    }
                }
                MOD_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxrf_lna2_gain,
                    wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID));
            }
        } else {
            /* 2G index is 0->5 */
            for (x = 0; x < 6; x++) {
                lnarout_val = (ac_tiny_g_lna_rout_map[x] << 3) |
                    ac_tiny_g_lna_gain_map[x];
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, x, 8,
                    &lnarout_val);
            }

            /* 5G index is 8->13 */
            for (x = 0; x < 6; x++) {
                lnarout_val =  (ac_tiny_a_lna_rout_map[x] << 3) |
                    ac_tiny_a_lna_gain_map[x];
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, 8 + x, 8,
                                          &lnarout_val);
            }
        }
    }

    /* Update gaintbl */
    FOREACH_CORE(pi, core) {
        if (core == 0) {
            gain_tblid =  ACPHY_TBL_ID_GAIN0;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS0;
        } else if (core == 1) {
            gain_tblid =  ACPHY_TBL_ID_GAIN1;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS1;
        } else if (core == 2) {
            gain_tblid =  ACPHY_TBL_ID_GAIN2;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS2;
        } else {
            gain_tblid =  ACPHY_TBL_ID_GAIN3;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS3;
        }

        if (!TINY_RADIO(pi) && !IS_28NM_RADIO(pi) && !IS_16NM_RADIO(pi)) {
            lna1rout_offset = core * 24;
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                lna1rout_offset += 8;

            if (lna12 == LNA1_ID) {
                uint8 i;
                if (pi_ac->rxgcrsi->fem_rxgains[core].lna1byp) {
                    /* Additional settings to use LNA1 bypass instead of T/R */
                    wlc_phy_upd_lna1_bypass_acphy(pi, core);
                }
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 6,
                                         lna1rout_offset, 8, lna1rout_tbl);
                for (i = 0; i < 6; i++) {
                    lna1 = lna1rout_tbl[gainbitstbl[i]];
                    lna1_idx = lna1 & 0x7;
                    lna1_rout = (lna1 >> 3) & 0xf;
                    gaintbl[i] = CHSPEC_IS2G(pi->radio_chanspec) ?
                            default_gaintbl[lna1_idx] -
                            ac_lna1_rout_delta_2g[lna1_rout]:
                            default_gaintbl[lna1_idx] -
                            ac_lna1_rout_delta_5g[lna1_rout];
                }
            }
        }

        memcpy(rxgcrsi->rxgainctrl_params[core].gaintbl[lna12],
            gaintbl, sizeof(uint8)*sz);
        wlc_phy_table_write_acphy(pi, gain_tblid, sz, offset, 8, gaintbl);
        memcpy(rxgcrsi->rxgainctrl_params[core].gainbitstbl[lna12], gainbitstbl,
            sizeof(uint8)*sz);
        wlc_phy_table_write_acphy(pi, gainbits_tblid, sz, offset, 8, gainbitstbl);

        /* Need to fill this table also for lna2bits */
        lna2rout_offset = (core * 24) + 16;
        if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
          !ACMAJORREV_128(pi->pubpi->phy_rev)) {
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                                          sz, lna2rout_offset, 8, gainbitstbl);
        }
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_upd_lna1_lna2_gainlimittbls_acphy(phy_info_t *pi, uint8 lna12)
{
    uint8 i, sz, max_idx, core, lna2_limit_size, lna1_rout = 0;
    uint8 lna1rout_tbl[N_LNA12_GAINS];
    uint8 max_idx_pktgain_limit, max_idx_non_init_lna1rout_tbl, lna1rout_offset;
    uint8 lna1_tbl[] = {11, 12, 14, 32, 36, 40};
    uint8 lna2_tbl[] = {0, 0, 0, 3, 3, 3, 3};
    uint8 tiny_lna2_tbl[] = {127, 127, 127, 127};
    uint8 tiny_tia_tbl[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127, 127};
    uint8 lna2_gainlmt_4365[] = {0, 0, 0, 0, 0, 0, 0};
    uint8 tia_gainlmt_4365[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 127};
    uint8 *tiny_gainlmt_lna2, *tiny_gainlmt_tia;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;
    acphy_desense_values_t *desense = rxgcrsi->total_desense;
    uint16 gainlimitid;

    if (!TINY_RADIO(pi) && !IS_28NM_RADIO(pi) && !IS_16NM_RADIO(pi)) {
        sz = (lna12 == LNA1_ID) ? 6 : 7;

        /* Limit based on desense mitigation mode */
        if (lna12 == LNA1_ID) {
            max_idx = MAX(0, (sz - 1) - desense->lna1_gainlmt_desense);
            rxgcrsi->curr_desense->lna1_gainlmt_desense = desense->lna1_gainlmt_desense;

            max_idx_pktgain_limit = max_idx;
            /* if rout desense is active, we have to change the non-init gain entries of
             * lna1_rout tbl and modify pkt gain limit tbl to not use init-gain instead
             * of just limiting LNA1 gain in the pkt gain limit tbls
             */
            max_idx_non_init_lna1rout_tbl = (desense->lna1rout_gainlmt_desense > 0) ?
                    max_idx : (sz - 2);

            /* LNA1 gain in pktgain limit tbl has to be capped
               only to not use init gain
            */
            max_idx_pktgain_limit = (desense->lna1rout_gainlmt_desense > 0) ?
                    (sz - 2) : max_idx;
            lna1_rout = CHSPEC_IS2G(pi->radio_chanspec) ?
                    0 + desense->lna1rout_gainlmt_desense :
                    4 - desense->lna1rout_gainlmt_desense;
            for (i = 0; i <= sz - 2; i++) {
                lna1rout_tbl[i] = (lna1_rout << 3) |
                        MAX(0, max_idx_non_init_lna1rout_tbl - (sz - 2 - i));
            }

            if (!(ACREV_IS(pi->pubpi->phy_rev, 0) || TINY_RADIO(pi))) {
                /* WRITE the lna1 rout table (only first 5 entries) */
                FOREACH_CORE(pi, core) {
                    lna1rout_offset = core * 24;
                    if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                        lna1rout_offset += 8;
                    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, sz-1,
                                              lna1rout_offset, 8, lna1rout_tbl);
                }
            }
        } else {
            max_idx_pktgain_limit = MAX(0, (sz - 1) - desense->lna2_gainlmt_desense);
            rxgcrsi->curr_desense->lna2_gainlmt_desense = desense->lna2_gainlmt_desense;
        }

        /* Write 0x7f to entries not to be used */
        for (i = (max_idx_pktgain_limit + 1); i < sz; i++) {
            if (lna12 == LNA1_ID) {
                lna1_tbl[i] = 0x7f;
            } else {
                lna2_tbl[i] = 0x7f;
            }
        }

        if (lna12 == LNA1_ID) {
            if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT0, sz,
                    8, 8,  lna1_tbl);
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT1, sz,
                    8, 8,  lna1_tbl);
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT2, sz,
                    8, 8,  lna1_tbl);
            } else {
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz,
                    8, 8, lna1_tbl);
            }
        }
        else {
            if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT0, sz,
                    16, 8, lna2_tbl);
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT1, sz,
                    16, 8, lna2_tbl);
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT2, sz,
                    16, 8, lna2_tbl);
            } else {
                wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_GAINLIMIT, sz,
                    16, 8, lna2_tbl);
            }
        }
    } else {
        tiny_gainlmt_lna2 = tiny_lna2_tbl;
        tiny_gainlmt_tia = tiny_tia_tbl;

        FOREACH_CORE(pi, core) {
            if (ACMAJORREV_4(pi->pubpi->phy_rev) || IS_28NM_RADIO(pi) ||
                IS_16NM_RADIO(pi)) {
                if (core == 0)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT0;
                else if (core == 1)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT1;
                else if (core == 2)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT2;
                else /* (core == 3) */
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT3;

                /* get max lna2 indx */
                lna2_limit_size =
                    wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID) + 1;
            } else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                /* Use different for 4365 */
                tiny_gainlmt_lna2 = lna2_gainlmt_4365;
                tiny_gainlmt_tia = tia_gainlmt_4365;
                if (core == 0)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT0;
                else if (core == 1)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT1;
                else if (core == 2)
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT2;
                else /* (core == 3) */
                    gainlimitid = ACPHY_TBL_ID_GAINLIMIT3;
                /* get max lna2 indx */
                lna2_limit_size = wlc_phy_get_max_lna_index_acphy(pi, LNA2_ID) + 1;
                lna2_limit_size = 7;
            } else {
                gainlimitid = ACPHY_TBL_ID_GAINLIMIT;
                lna2_limit_size = 2;
            }

            if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                wlc_phy_upd_lna1_lna2_gainlimittbls_28nm(pi, lna12,
                    gainlimitid, core);
            } else {
                wlc_phy_table_write_acphy(pi, gainlimitid, lna2_limit_size,
                    16, 8, tiny_gainlmt_lna2);
                wlc_phy_table_write_acphy(pi, gainlimitid, lna2_limit_size,
                    16+64, 8, tiny_gainlmt_lna2);
                if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) {
                    wlc_phy_table_write_acphy(pi, gainlimitid,
                        13, 32, 8, tiny_gainlmt_tia);
                    wlc_phy_table_write_acphy(pi, gainlimitid,
                        13, 32+64, 8, tiny_gainlmt_tia);
                } else {
                    wlc_phy_table_write_acphy(pi, gainlimitid,
                        12, 32, 8, tiny_gainlmt_tia);
                    wlc_phy_table_write_acphy(pi, gainlimitid,
                        12, 32+64, 8, tiny_gainlmt_tia);
                }
            }
        }
    }
}

void
wlc_phy_upd_lna1_lna2_gainlimittbls_28nm(phy_info_t *pi, uint8 lna12, uint16 gainlimitid,
    uint8 core)
{
    uint8 i, sz, max_idx, lna1_rout = 0, lna1_gain;
    uint8 lna1rout_tbl[N_LNA12_GAINS];
    uint8 max_idx_pktgain_limit, max_idx_non_init_lna1rout_tbl, lna1rout_offset;
    uint8 lna1_tbl[] = {11, 12, 14, 32, 36, 40};
    phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;
    acphy_desense_values_t *desense = rxgcrsi->total_desense;
    phy_ac_rxg_params_t *rxg_params = rxgcrsi->rxg_params;

    if (lna12 == LNA1_ID) {
        /* Size for LNA1 gain table */
        sz =  6;

        /* Copy default gain limit table */
        memcpy(lna1_tbl, rxg_params->gainlimit_tbl[LNA1_TBL_IND],
            sizeof(uint8)*sz);
        memcpy(lna1rout_tbl, rxg_params->rftbls->lna1Rout_tbl,
               sizeof(uint8)*sz);

        /* Limit based on desense mitigation mode */
        max_idx = MAX(0, (sz - 1) - desense->lna1_gainlmt_desense);
        rxgcrsi->curr_desense->lna1_gainlmt_desense =
            desense->lna1_gainlmt_desense;

        /* if rout desense is active, we have to change the non-init
         * gain entries of lna1_rout tbl and modify pkt gain limit
         * tbl to not use init-gain instead of just limiting LNA1
         * gain in the pkt gain limit tbls
         */
        max_idx_non_init_lna1rout_tbl =
            (desense->lna1rout_gainlmt_desense > 0) ?
            max_idx : (sz - 2);

        /* LNA1 gain in pktgain limit tbl has to be capped
           only to not use init gain
        */
        max_idx_pktgain_limit =
            (desense->lna1rout_gainlmt_desense > 0) ?
            (sz - 2) : max_idx;

        for (i = 0; i < max_idx_non_init_lna1rout_tbl; i++) {
            lna1_rout = wlc_phy_get_lna_gain_rout(pi, i,
                    GET_LNA_ROUT);
            lna1_gain = wlc_phy_get_lna_gain_rout(pi, i,
                    GET_LNA_GAINCODE);
            lna1_gain -= desense->lna1rout_gainlmt_desense;
            lna1rout_tbl[i] = (lna1_rout << 3) | lna1_gain;
        }

        /* WRITE the lna1 rout table (only first 5 entries) */
        lna1rout_offset = core * 24;
        if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
            lna1rout_offset += 8;
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT,
            sz, lna1rout_offset, 8, lna1rout_tbl);

        /* Write 0x7f to entries not to be used */
        for (i = (max_idx_pktgain_limit + 1); i < sz; i++) {
            lna1_tbl[i] = 0x7f;
        }

        wlc_phy_table_write_acphy(pi, gainlimitid, 6, 8, 8,
            lna1_tbl);
        wlc_phy_table_write_acphy(pi, gainlimitid, 6, 8+64, 8,
            lna1_tbl);
    } else {
        wlc_phy_table_write_acphy(pi, gainlimitid, 4, 16, 8,
            rxg_params->gainlimit_tbl[LNA2_TBL_IND]);
        wlc_phy_table_write_acphy(pi, gainlimitid, 4, 16+64, 8,
            rxg_params->gainlimit_tbl[LNA2_TBL_IND]);
    }
}

void
wlc_phy_upd_lna1_bypass_acphy(phy_info_t *pi, uint8 core)
{
    uint8 idx = 6, rout, rout_gain, rout_offset, rout_tbl;
    uint16 lna1byp_val, gainbits_tblid;

    rout_offset = 0 + (core * 24) + idx;
    if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
        rout_offset += 8;
    }
    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rout = 9; rout_gain = -6;
        } else {
            rout = 0; rout_gain = 1;
        }
    } else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            rout = 10; rout_gain = -3;
        } else {
            rout = 4; rout_gain = -12;
        }
    } else {
        rout = 0; rout_gain = 0; rout_offset = 0;
        ASSERT(0); /* Not sure what should go in here */
    }

    lna1byp_val = (1 << ACPHY_Core0_lna1BypVals_lna1BypEn0_SHIFT(pi->pubpi.phy_rev)) |
            (idx << ACPHY_Core0_lna1BypVals_lna1BypIndex0_SHIFT(pi->pubpi.phy_rev)) |
            (rout_gain << ACPHY_Core0_lna1BypVals_lna1BypGain0_SHIFT(pi->pubpi.phy_rev));

    WRITE_PHYREGC(pi, _lna1BypVals, core, lna1byp_val);
    MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);

    /*  Set the Rout to be used for index 6 */
    rout_tbl = (rout << 3) + idx;
    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, rout_offset, 8, &rout_tbl);

    gainbits_tblid = (core == 0) ? ACPHY_TBL_ID_GAINBITS0 : (core == 1) ?
            ACPHY_TBL_ID_GAINBITS1 : (core == 2) ? ACPHY_TBL_ID_GAINBITS2 :
            ACPHY_TBL_ID_GAINBITS3;
    wlc_phy_table_write_acphy(pi, gainbits_tblid, 1, 6, 8, &idx);

    if (!ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        if (PHYCORENUM(pi->pubpi->phy_corenum) < 4) {
            /* WAR for LNA1 bypass sticky on Core 0, CRDOT11ACPHY-1161 */
            MOD_PHYREG(pi, SlnaControl, SlnaCore, 3);
        }
    }
}

void wlc_phy_rfctrl_override_rxgain_acphy(phy_info_t *pi, uint8 restore,
                                           rxgain_t rxgain[], rxgain_ovrd_t rxgain_ovrd[])
{
    uint8 core, lna1_Rout, lna2_Rout;
    uint16 reg_rxgain, reg_rxgain2, reg_lpfgain;
    uint8 stall_val;
    uint8 lna1_gm;
    uint8 offset;
    bool suspend;
    uint8 lna1byp = 0;

    if (restore == 1) {
        /* restore the stored values */
        FOREACH_CORE(pi, core) {
            WRITE_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain_ovrd[core].rfctrlovrd);
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core, rxgain_ovrd[core].rxgain);
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN2, core, rxgain_ovrd[core].rxgain2);
            WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core, rxgain_ovrd[core].lpfgain);
            PHY_INFORM(("%s, Restoring RfctrlOverride(rxgain) values\n", __FUNCTION__));
        }
    } else {
        suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
        if (!suspend) {
            wlapi_suspend_mac_and_wait(pi->sh->physhim);
            phy_utils_phyreg_enter(pi);
        }
        stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
        ACPHY_DISABLE_STALL(pi);
        FOREACH_CORE(pi, core) {
            /* Save the original values */
            rxgain_ovrd[core].rfctrlovrd = READ_PHYREGCE(pi, RfctrlOverrideGains, core);
            rxgain_ovrd[core].rxgain = READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);
            rxgain_ovrd[core].rxgain2 = READ_PHYREGCE(pi, RfctrlCoreRXGAIN2, core);
            rxgain_ovrd[core].lpfgain = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);

            offset = (TINY_RADIO(pi) || IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) ?
                (rxgain[core].lna1 +
                ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec)) :
                (5 + ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec));

            wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                1, offset + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
                8, &lna1_Rout);
            /* For rev40 lna2 not used but table has entries for lna2 */
            if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                    1, 16 + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
                    8, &lna2_Rout);
            } else {
                /* LnaRoutLUT WAR for 4349A2 */
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                    1, 22 + ACPHY_LNAROUT_CORE_RD_OFST(pi, core),
                    8, &lna2_Rout);
            }

            /* LNA1 bypss check */
            if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                lna1byp = (rxgain[core].lna1 == LNA1_BYPASS_INDEX) ? 1 : 0;
                PHY_INFORM(("%s, LNA1 bypass mode override\n", __FUNCTION__));
            }

            /* Write the rxgain override registers */
            lna1_gm = (TINY_RADIO(pi) || IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi))
                ? (lna1_Rout & 0x7) : rxgain[core].lna1;

            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN1, core,
                          (lna1byp << 14) | (rxgain[core].dvga << 10) |
                          (rxgain[core].mix << 6) | (rxgain[core].lna2 << 3) | lna1_gm);
            WRITE_PHYREGCE(pi, RfctrlCoreRXGAIN2, core,
                          (((lna2_Rout >> 3) & 0xf) << 4 | ((lna1_Rout >> 3) & 0xf)));
            WRITE_PHYREGCE(pi, RfctrlCoreLpfGain, core,
                          (rxgain[core].lpf1 << 3) | rxgain[core].lpf0);

            MOD_PHYREGCE(pi, RfctrlOverrideGains, core, rxgain, 1);
            MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq1_gain, 1);
            MOD_PHYREGCE(pi, RfctrlOverrideGains, core, lpf_bq2_gain, 1);

            reg_rxgain = READ_PHYREGCE(pi, RfctrlCoreRXGAIN1, core);
            reg_rxgain2 = READ_PHYREGCE(pi, RfctrlCoreRXGAIN2, core);
            reg_lpfgain = READ_PHYREGCE(pi, RfctrlCoreLpfGain, core);
            PHY_INFORM(("%s, core %d. rxgain_ovrd = 0x%x, lpf_ovrd = 0x%x\n",
                        __FUNCTION__, core, reg_rxgain, reg_lpfgain));
            PHY_INFORM(("%s, core %d. rxgain_rout_ovrd = 0x%x\n",
                        __FUNCTION__, core, reg_rxgain2));
            BCM_REFERENCE(reg_rxgain);
            BCM_REFERENCE(reg_rxgain2);
            BCM_REFERENCE(reg_lpfgain);
        }
        ACPHY_ENABLE_STALL(pi, stall_val);
        if (!suspend) {
            phy_utils_phyreg_exit(pi);
            wlapi_enable_mac(pi->sh->physhim);
        }
    }
}

uint8 wlc_phy_calc_extra_init_gain_acphy(phy_info_t *pi, uint8 extra_gain_3dB,
    rxgain_t rxgain[])
{
    uint16 init_gain_code[4];
    uint8 core, MAX_DVGA, MAX_LPF, MAX_MIX;
    uint8 dvga, mix, lpf0, lpf1;
    uint8 dvga_inc, lpf0_inc, lpf1_inc;
    uint8 max_inc, gain_ticks = extra_gain_3dB;

    MAX_DVGA = 4; MAX_LPF = 10; MAX_MIX = 4;
    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 3, 0xf9, 16, &init_gain_code);

    /* Find if the requested gain increase is possible */
    FOREACH_CORE(pi, core) {
        dvga = 0;
        mix = (init_gain_code[core] >> 6) & 0xf;
        lpf0 = (init_gain_code[core] >> 10) & 0x7;
        lpf1 = (init_gain_code[core] >> 13) & 0x7;
        max_inc = MAX(0, MAX_DVGA - dvga) + MAX(0, MAX_LPF - lpf0 - lpf1) +
                MAX(0, MAX_MIX - mix);
        gain_ticks = MIN(gain_ticks, max_inc);
    }
    if (gain_ticks != extra_gain_3dB) {
        PHY_INFORM(("%s: Unable to find enough extra gain. Using extra_gain = %d\n",
                    __FUNCTION__, 3 * gain_ticks));
    }
        /* Do nothing if no gain increase is required/possible */
    if (gain_ticks == 0) {
        return gain_ticks;
    }
    /* Find the mix, lpf0, lpf1 gains required for extra INITgain */
    FOREACH_CORE(pi, core) {
        uint8 gain_inc = gain_ticks;
        dvga = 0;
        mix = (init_gain_code[core] >> 6) & 0xf;
        lpf0 = (init_gain_code[core] >> 10) & 0x7;
        lpf1 = (init_gain_code[core] >> 13) & 0x7;
        dvga_inc = MIN((uint8) MAX(0, MAX_DVGA - dvga), gain_inc);
        dvga += dvga_inc;
        gain_inc -= dvga_inc;
        lpf1_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
        lpf1 += lpf1_inc;
        gain_inc -= lpf1_inc;
        lpf0_inc = MIN((uint8) MAX(0, MAX_LPF - lpf1 - lpf0), gain_inc);
        lpf0 += lpf0_inc;
        gain_inc -= lpf0_inc;
        mix += MIN((uint8) MAX(0, MAX_MIX - mix), gain_inc);
        rxgain[core].lna1 = init_gain_code[core] & 0x7;
        rxgain[core].lna2 = (init_gain_code[core] >> 3) & 0x7;
        rxgain[core].mix  = mix;
        rxgain[core].lpf0 = lpf0;
        rxgain[core].lpf1 = lpf1;
        rxgain[core].dvga = dvga;
    }
    return gain_ticks;
}

/* ************************************* */
/*        Carrier Sense related definitions        */
/* ************************************* */
static void wlc_phy_ofdm_crs_acphy(phy_info_t *pi, bool enable);
static void wlc_phy_clip_det_acphy(phy_info_t *pi, bool enable);

static void
wlc_phy_ofdm_crs_acphy(phy_info_t *pi, bool enable)
{
    uint8 core, enable_bit;
    uint8 lesi_en;

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    enable_bit = enable ? 1 : 0;

    /* MAC should be suspended before calling this function */
    ASSERT((R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC) == 0);

    if (pi->u.pi_acphy->rxgcrsi->lesi_cap) {
        /* Even with lesi cap, lesi could be off due to lesi desense */
        lesi_en = (pi->u.pi_acphy->rxgcrsi->lesi_on & enable) ? 1 : 0;
        MOD_PHYREG(pi, lesi_control, lesiCrsEn, lesi_en);
        MOD_PHYREG(pi, lesi_control, lesiFstrEn, lesi_en);
    }

    if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        FOREACH_CORE(pi, core) {
            MOD_PHYREGCE(pi, crsControlu, core, totEnable, enable_bit);
            MOD_PHYREGCE(pi, crsControll, core, totEnable, enable_bit);
            MOD_PHYREGCE(pi, crsControluSub1, core, totEnable, enable_bit);
            MOD_PHYREGCE(pi, crsControllSub1, core, totEnable, enable_bit);
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
                (wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
                MOD_PHYREGCE(pi, crsControl_4_, core, totEnable, enable_bit);
                MOD_PHYREGCE(pi, crsControl_5_, core, totEnable, enable_bit);
                MOD_PHYREGCE(pi, crsControl_6_, core, totEnable, enable_bit);
                MOD_PHYREGCE(pi, crsControl_7_, core, totEnable, enable_bit);
            }
        }
    } else if (enable) {
        if (ACREV_GE(pi->pubpi->phy_rev, 32)) {
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                FOREACH_CORE(pi, core) {
                MOD_PHYREGCE(pi, crsControlu, core, totEnable, 1);
                MOD_PHYREGCE(pi, crsControll, core, totEnable, 1);
                MOD_PHYREGCE(pi, crsControluSub1, core, totEnable, 1);
                MOD_PHYREGCE(pi, crsControllSub1, core, totEnable, 1);
                }
            } else {
                MOD_PHYREG(pi, crsControlu0, totEnable, 1);
                if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                    ACPHY_REG_LIST_START
                        MOD_PHYREG_ENTRY(pi, crsControll0, totEnable, 1)
                        MOD_PHYREG_ENTRY(pi, crsControllSub10, totEnable, 1)
                        MOD_PHYREG_ENTRY(pi, crsControluSub10, totEnable, 1)
                    ACPHY_REG_LIST_EXECUTE(pi);
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 2) {
                    ACPHY_REG_LIST_START
                        MOD_PHYREG_ENTRY(pi, crsControlu1, totEnable, 1)
                    ACPHY_REG_LIST_EXECUTE(pi);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                        ACPHY_REG_LIST_START
                            MOD_PHYREG_ENTRY(pi, crsControll1,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControllSub11,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControluSub11,
                                totEnable, 1)
                        ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 3) {
                    MOD_PHYREG(pi, crsControlu2, totEnable, 1);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                        ACPHY_REG_LIST_START
                            MOD_PHYREG_ENTRY(pi, crsControll2,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControllSub12,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControluSub12,
                                totEnable, 1)
                        ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 4) {
                    MOD_PHYREG(pi, crsControlu3, totEnable, 1);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                        ACPHY_REG_LIST_START
                            MOD_PHYREG_ENTRY(pi, crsControll3,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControllSub13,
                                totEnable, 1)
                            MOD_PHYREG_ENTRY(pi, crsControluSub13,
                                totEnable, 1)
                        ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
            }
        } else {
            ACPHY_REG_LIST_START
                MOD_PHYREG_ENTRY(pi, crsControlu, totEnable, 1)
                MOD_PHYREG_ENTRY(pi, crsControll, totEnable, 1)
                MOD_PHYREG_ENTRY(pi, crsControluSub1, totEnable, 1)
                MOD_PHYREG_ENTRY(pi, crsControllSub1, totEnable, 1)
            ACPHY_REG_LIST_EXECUTE(pi);
        }
    } else {
        if (ACREV_GE(pi->pubpi->phy_rev, 32)) {
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                FOREACH_CORE(pi, core) {
                MOD_PHYREGCE(pi, crsControlu, core, totEnable, 0);
                MOD_PHYREGCE(pi, crsControll, core, totEnable, 0);
                MOD_PHYREGCE(pi, crsControluSub1, core, totEnable, 0);
                MOD_PHYREGCE(pi, crsControllSub1, core, totEnable, 0);
                }
            } else {
                MOD_PHYREG(pi, crsControlu0, totEnable, 0);
                if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                ACPHY_REG_LIST_START
                    MOD_PHYREG_ENTRY(pi, crsControll0, totEnable, 0)
                    MOD_PHYREG_ENTRY(pi, crsControllSub10, totEnable, 0)
                    MOD_PHYREG_ENTRY(pi, crsControluSub10, totEnable, 0)
                ACPHY_REG_LIST_EXECUTE(pi);
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 2) {
                    MOD_PHYREG(pi, crsControlu1, totEnable, 0);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                    ACPHY_REG_LIST_START
                        MOD_PHYREG_ENTRY(pi, crsControll1, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControllSub11, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControluSub11, totEnable, 0)
                    ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 3) {
                    MOD_PHYREG(pi, crsControlu2, totEnable, 0);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                    ACPHY_REG_LIST_START
                        MOD_PHYREG_ENTRY(pi, crsControll2, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControllSub12, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControluSub12, totEnable, 0)
                    ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
                if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 4) {
                    MOD_PHYREG(pi, crsControlu3, totEnable, 0);
                    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
                    ACPHY_REG_LIST_START
                        MOD_PHYREG_ENTRY(pi, crsControll3, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControllSub13, totEnable, 0)
                        MOD_PHYREG_ENTRY(pi, crsControluSub13, totEnable, 0)
                    ACPHY_REG_LIST_EXECUTE(pi);
                    }
                }
            }
        } else {
            ACPHY_REG_LIST_START
                MOD_PHYREG_ENTRY(pi, crsControlu, totEnable, 0)
                MOD_PHYREG_ENTRY(pi, crsControll, totEnable, 0)
                MOD_PHYREG_ENTRY(pi, crsControluSub1, totEnable, 0)
                MOD_PHYREG_ENTRY(pi, crsControllSub1, totEnable, 0)
            ACPHY_REG_LIST_EXECUTE(pi);
        }
    }
}

static void
wlc_phy_clip_det_acphy(phy_info_t *pi, bool enable)
{
    uint8 core;
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

    /* Make clip detection difficult (impossible?) */
    /* don't change this loop to active core loop, gives 100% per, why? */
    FOREACH_CORE(pi, core) {
        if (ACREV_IS(pi->pubpi->phy_rev, 0)) {
            if (enable) {
                WRITE_PHYREGC(pi, Clip1Threshold, core,
                    pi_ac->rxgcrsi->clip1_th);
            } else {
                WRITE_PHYREGC(pi, Clip1Threshold, core, 0xffff);
            }
        } else {
            if (enable) {
                phy_utils_and_phyreg(pi, ACPHYREGC(pi, computeGainInfo, core),
                    (uint16)~ACPHY_REG_FIELD_MASK(pi, computeGainInfo, core,
                    disableClip1detect));
            } else {
                phy_utils_or_phyreg(pi, ACPHYREGC(pi, computeGainInfo, core),
                    ACPHY_REG_FIELD_MASK(pi, computeGainInfo, core,
                    disableClip1detect));
            }
        }
    }

}

void wlc_phy_force_crsmin_acphy(phy_info_t *pi, void *p)
{
    int8 *set_crs_thr = p;
    /* Local copy of phyrxchains & EnTx bits before overwrite */
    uint8 enRx = 0;
    uint8 enTx = 0;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    /* Prepare Mac and Phregs */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Save and overwrite Rx chains */
    wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
        stf_shdata->hw_phyrxchain, stf_shdata->phytxchain);

    if (set_crs_thr[0] == -1) {
        bool save_phynoise_disable;
        /* Auto crsmin power mode */
        PHY_CAL(("Setting auto crsmin power mode\n"));
        /* save and enable the noisecal schedule state */
        save_phynoise_disable = phy_noise_sched_get(pi);
        phy_noise_sched_set(pi, PHY_FORCEMEASURE_MODE, FALSE);
        phy_noise_sample_request_crsmincal(pi);
        /* restore the noisecal schedule state */
        phy_noise_sched_set(pi, PHY_FORCEMEASURE_MODE,
            save_phynoise_disable);
        pi->u.pi_acphy->rxgcrsi->crsmincal_enable = TRUE;
        pi->u.pi_acphy->rxgcrsi->force_crsminval = 0;

        // run it once to restore cached values
        (void) phy_ac_rxgcrs_min_pwr_cal(pi->u.pi_acphy->rxgcrsi, PHY_CRS_SET_FROM_CACHE);
    } else if (set_crs_thr[0] == 0) {
        /* Default crsmin value */
        PHY_CAL(("Setting default crsmin: %d\n", ACPHY_CRSMIN_DEFAULT));
        phy_ac_rxgcrs_set_crs_min_pwr(pi, ACPHY_CRSMIN_DEFAULT, 0, 0, 0, 0, FALSE);
        pi->u.pi_acphy->rxgcrsi->crsmincal_enable = FALSE;
        pi->u.pi_acphy->rxgcrsi->force_crsminval = ACPHY_CRSMIN_DEFAULT;
    } else {
        /* Set the crsmin power value to be 'set_crs_thr' */
        PHY_CAL(("Setting crsmin: %d %d %d %d\n",
            set_crs_thr[0], set_crs_thr[1], set_crs_thr[2], set_crs_thr[3]));
        phy_ac_rxgcrs_set_crs_min_pwr(pi, set_crs_thr[0], 0, set_crs_thr[1],
            set_crs_thr[2], set_crs_thr[3], FALSE);
        pi->u.pi_acphy->rxgcrsi->crsmincal_enable = FALSE;
        pi->u.pi_acphy->rxgcrsi->force_crsminval = set_crs_thr[0];
    }

    /* Restore Rx chains */
    wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

    /* Enable mac */
    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);
}

void phy_ac_rxgcrs_force_lesiscale(phy_ac_rxgcrs_info_t *rxgcrsi, void *p)
{
    phy_info_t *pi = rxgcrsi->pi;
    int8  *set_lesi_scale = p;
    /* Local copy of phyrxchains & EnTx bits before overwrite */
    uint8 enRx = 0;
    uint8 enTx = 0;
    int8 zerodBs[] = {64, 64, 64, 64};
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    BCM_REFERENCE(stf_shdata);

    /* Prepare Mac and Phregs */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);
    phy_utils_phyreg_enter(pi);

    /* Save and overwrite Rx chains */
    wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
        stf_shdata->hw_phyrxchain, stf_shdata->phytxchain);

    if (set_lesi_scale[0] == -1) {
        /* Auto crsmin power mode */
        PHY_CAL(("Setting auto crsmin power mode\n"));
        phy_noise_sample_request_crsmincal(pi);
        rxgcrsi->lesiscalecal_enable = TRUE;
    } else if (set_lesi_scale[0] == 0) {
        /* Default crsmin value */
        PHY_CAL(("Setting default crsmin: %d\n", ACPHY_LESISCALE_DEFAULT));
        phy_ac_rxgcrs_set_lesiscale(rxgcrsi, zerodBs);
        rxgcrsi->lesiscalecal_enable = FALSE;
    } else {
        /* Set the crsmin power value to be 'set_crs_thr' */
        printf("Setting LESI scale: %d %d %d %d\n",
            set_lesi_scale[0], set_lesi_scale[1], set_lesi_scale[2], set_lesi_scale[3]);
        phy_ac_rxgcrs_set_lesiscale(rxgcrsi, set_lesi_scale);
        rxgcrsi->lesiscalecal_enable = FALSE;
    }

    /* Restore Rx chains */
    wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

    /* Enable mac */
    phy_utils_phyreg_exit(pi);
    wlapi_enable_mac(pi->sh->physhim);
}

static void
phy_ac_rxgcrs_asymmetric_jammer_mod(phy_info_t *pi, int8 cmplx_pwr_dbm[])
{
    /* Asymmetric AWGN noise jammer fix */
    int8 noise_delta = cmplx_pwr_dbm[1] - cmplx_pwr_dbm[0];

    if (ABS(noise_delta) > 3) {
        int8 freqweight = MAX(MIN(32 + noise_delta * 4 / 3, 0x3d), 0x3);
        uint8 nvar_adj = MAX(MIN((16 * ABS(noise_delta) / 5) + 0xe, 0x80), 0xe);

        MOD_PHYREG(pi, FreqGainBypass, bypass, 1);
        MOD_PHYREG(pi, FreqGainBypass, bypassValue, freqweight);
        MOD_PHYREG(pi, FSTRMetricTh, lowPwr_min_metric_th, 0x1c);
        if (noise_delta > 0) {
            MOD_PHYREG(pi, nvcfg0, noisevar_nf_radio_qdb_core1, nvar_adj);
        } else {
            MOD_PHYREG(pi, nvcfg0, noisevar_nf_radio_qdb_core0, nvar_adj);
        }
    } else {
        MOD_PHYREG(pi, FreqGainBypass, bypass, 0);
        MOD_PHYREG(pi, FSTRMetricTh, lowPwr_min_metric_th, 0x20);
        MOD_PHYREG(pi, nvcfg0, noisevar_nf_radio_qdb_core1, 0xE);
        MOD_PHYREG(pi, nvcfg0, noisevar_nf_radio_qdb_core0, 0xE);
    }
}

static uint8
phy_ac_rxgcrs_get_chan_freq_range(phy_info_t *pi, uint8 core)
{
    uint8 chan_freq_range;

    /* check the freq range of the current channel */
    /* 2G, 5GL, 5GM, 5GH, 5GX */
    if (ACMAJORREV_33(pi->pubpi->phy_rev) && PHY_AS_80P80(pi, pi->radio_chanspec)) {
        uint8 chans[NUM_CHANS_IN_CHAN_BONDING] = {0, 0};

        phy_ac_chanmgr_get_chan_freq_range_80p80(pi, pi->radio_chanspec, chans);
        chan_freq_range = chans[0]; // FIXME - core0/1: chans[0], core2/3 chans[1]
        ASSERT(chan_freq_range < PHY_SIZE_NOISE_CACHE_ARRAY);
    } else {
        /* core_freq_segment_map is only required for 80P80 mode.
        For other modes, it is ignored
        */
        uint8 core_freq_segment_map =
            phy_ac_chanmgr_get_data(pi->u.pi_acphy->chanmgri)->core_freq_mapping[core];

        chan_freq_range = phy_ac_chanmgr_get_chan_freq_range(pi, 0, core_freq_segment_map);
    }

    return chan_freq_range;
}

static int
phy_ac_rxgcrs_crsmin_run_auto(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core, uint8 chan_freq_range,
    int8 cmplx_pwr_dbm[])
{
    int8 tmp_diff;
    const phy_info_t *pi = rxgcrsi->pi;

    /* cache_update threshold: 2 4335 and 4350, and 3 for 4360 */
    uint8 cache_up_th = (ACMAJORREV_0(pi->pubpi->phy_rev)) ? 3 : 2;

    int ret = phy_ac_rxgcrs_get_avg_noisepwr_per_core(rxgcrsi, core, cmplx_pwr_dbm);

    if (ret != BCME_OK) {
        return ret;
    }

    BCM_REFERENCE(pi);

    /* update noisecal_cache with valid noise power */
    /* check if the new noise pwr reading is same as cache */
    tmp_diff = rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core];
    if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
        tmp_diff -= cmplx_pwr_dbm[core];
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        tmp_diff = (tmp_diff + 3) - cmplx_pwr_dbm[core];
    } else {
        tmp_diff = (tmp_diff + 7) - cmplx_pwr_dbm[core];
    }

    /* run crscal with the current noise pwr if the call comes from phy_cals */
    if (ABS(tmp_diff) >= cache_up_th || rxgcrsi->force_crsmincal) {
        ret = 1;    /* indicate to run_cal */
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core] =
                cmplx_pwr_dbm[core];
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core] =
                cmplx_pwr_dbm[core] - 3;
        } else {
            rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core] =
                cmplx_pwr_dbm[core] - 7;
        }
    }

    return ret;
}

static int
phy_ac_rxgcrs_get_thresh_idx(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core, uint8 crsmin_cal_mode,
                             uint8 chan_freq_range, int8 cmplx_pwr_dbm[], uint8 *idxlists)
{
    int ret;
    int8 idx;
    phy_info_t *pi = rxgcrsi->pi;

    BCM_REFERENCE(pi);

    if (crsmin_cal_mode == PHY_CRS_RUN_AUTO) {
        ret = phy_ac_rxgcrs_crsmin_run_auto(rxgcrsi, core, chan_freq_range, cmplx_pwr_dbm);
    } else {
        /* Enter here only from chan_change */

        /* During chan_change, read back the noise pwr from cache */
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            cmplx_pwr_dbm[core] =
                rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core];
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            cmplx_pwr_dbm[core] =
                rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core] + 3;
        } else {
            cmplx_pwr_dbm[core] =
                rxgcrsi->phy_noise_cache_crsmin[chan_freq_range][core] + 7;
        }

        ret = BCME_OK;
    }

    /* get the index number for crs_th table */
    idx = cmplx_pwr_dbm[core];
    if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
        if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
            idx += 38;
            // was 39 but there is a 1dB added to noise power in
            //rxgcrsi->phy_noise_pwr_array[rxgcrsi->phy_noise_counter][core] =
            //phy_ac_noise_get_data(pi->u.pi_acphy->noisei)
            //->phy_noise_all_core[core] + 1;
        }  else {
            idx += 36;
        }
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            idx += 34;
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            idx += 33;
        } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev)) {
            idx += 33;
            // was 34 but there is a 1dB added to noise power in:
            //rxgcrsi->phy_noise_pwr_array[rxgcrsi->phy_noise_counter][core] =
            //phy_ac_noise_get_data(pi->u.pi_acphy->noisei)->
            //phy_noise_all_core[core] + 1;
        } else {
            idx += 34;
        }
    } else {
        if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
            idx += 33;
        } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
            idx += 32;
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            idx += 29;
        } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev)) {
            idx += 29;
            // was 30 but there is a 1dB added to noise power in:
            //rxgcrsi->phy_noise_pwr_array[rxgcrsi->phy_noise_counter][core] =
            //phy_ac_noise_get_data(pi->u.pi_acphy->noisei)
            //->phy_noise_all_core[core] + 1;
        } else {
            idx += 30;
        }
    }

    PHY_CAL(("%s: cmplx_pwr (%d) =======  %d\n", __FUNCTION__, core, cmplx_pwr_dbm[core]));
    *idxlists = (idx < 0) ? 0 : (uint8) idx;

    return ret;
}

static uint8
phy_ac_rxgcrs_get_thresh_idx_lesi(phy_info_t *pi, uint8 core, int8 cmplx_pwr_dbm[])
{
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    uint8 dvga = READ_PHYREGFLDC(pi, InitGainCodeB, core, initvgagainIndex);
    const uint8  thresh_sz_lesi = 22; /* i.e. not the full size of the array */
    int8 idxlists_lesi = cmplx_pwr_dbm[core];
    idxlists_lesi -= 3 * dvga;

    /* [43684B0&B1] WAR for SISO PER spike when fstrSwitchEn=0 for 5G band. */
    if (ACMAJORREV_47(pi->pubpi->phy_rev) || ACMAJORREV_129(pi->pubpi->phy_rev))
        idxlists_lesi += 3;

    /* [6715B0] apply same 43684 WAR for 5G/6G band. */
    if (ACMAJORREV_130(pi->pubpi->phy_rev) &&
        (RADIOREV(pi->pubpi->radiorev) > 1) &&
        CHSPEC_ISPHY5G6G(pi->radio_chanspec))
        idxlists_lesi += 3;

    /* currently adding 2 MORE (total 5) ticks to the offset only for 5G, ELNA/MCM for 6710. */
    if (ACMAJORREV_129(pi->pubpi->phy_rev) && CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
        if ((BF_ELNA_5G(pi_ac) == 0) && CHSPEC_IS40(pi->radio_chanspec))
            idxlists_lesi += 2;
        else
            idxlists_lesi += 2;
    }

    /* Because LESI is too sensitive, we see flooring even on small spurs */
    if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        idxlists_lesi += 6;
        /* Need to further desensitize BW20 due to flooring */
        if (CHSPEC_IS20(pi->radio_chanspec)) {
            idxlists_lesi += 2;
        }
    }

    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        idxlists_lesi -= pi_ac->rxgcrsi->lesiscale_2g;
    } else {
        idxlists_lesi -= pi_ac->rxgcrsi->lesiscale_5g;
    }

    BCM_REFERENCE(pi);

    if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
        idxlists_lesi += 40;
    } else if (CHSPEC_IS40(pi->radio_chanspec)) {
        idxlists_lesi += 37;
    } else if (CHSPEC_IS80(pi->radio_chanspec)) {
        idxlists_lesi += 34;
    } else {
        idxlists_lesi += 31;
    }

    return (idxlists_lesi < 0) ? 0 :
           (idxlists_lesi > thresh_sz_lesi) ? thresh_sz_lesi : (uint8)idxlists_lesi;
}

static void
phy_ac_rxgcrs_set_min_pwr(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 fp[], int8 offset[],
    int8 lesi_scale[], uint8 fp_min_of_all_cores, uint8 enRx)
{
    bool suspend;
    phy_info_t *pi = rxgcrsi->pi;

    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

    /* since we are touching phy regs mac has to be suspended */
    if (!suspend) {
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
        phy_utils_phyreg_enter(pi);
    }

    /* call for updating the crsmin thresholds */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        if (rxgcrsi->crsmincal_enable)
            phy_ac_rxgcrs_set_crs_min_pwr(pi, fp[0], 0, offset[1], offset[2],
                offset[3], FALSE);
        if (rxgcrsi->lesiscalecal_enable)
            phy_ac_rxgcrs_set_lesiscale(rxgcrsi, lesi_scale);
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (!(enRx == 3)) {
            /* For SISO cases the offset can be large. This offset applies */
            /* to all clip thresholds. Hence setting the offset to 0 and */
            /* just updating init gain thresholds with the max of both cores. */
            fp_min_of_all_cores = MAX(fp[0], fp[1]);
            offset[0] = 0;
            offset[1] = 0;
        } else {
            offset[0] = fp[0] - fp_min_of_all_cores;
            offset[1] = fp[1] - fp_min_of_all_cores;
        }
        phy_ac_rxgcrs_set_crs_min_pwr(pi, fp_min_of_all_cores, offset[0], offset[1],
            offset[2], offset[3], FALSE);
        if (rxgcrsi->lesiscalecal_enable)
            phy_ac_rxgcrs_set_lesiscale(rxgcrsi, lesi_scale);
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        if (rxgcrsi->crsmincal_enable)
            phy_ac_rxgcrs_set_crs_min_pwr(pi, fp_min_of_all_cores, offset[0], offset[1],
                    offset[2], offset[3], FALSE);
        if (rxgcrsi->lesiscalecal_enable)
            phy_ac_rxgcrs_set_lesiscale(rxgcrsi, lesi_scale);
    } else {
        phy_ac_rxgcrs_set_crs_min_pwr(pi, fp[0], 0, offset[1], offset[2], offset[3], FALSE);
    }

    /* resume mac */
    if (!suspend) {
        phy_utils_phyreg_exit(pi);
        wlapi_enable_mac(pi->sh->physhim);
    }
}

static const int8  scale_lesi[] = {64, 57, 50, 45, 40, 35, 32, 28, 25, 22,
        20, 18, 16, 14, 12, 11, 10,  9,  8,  7,  6,  5,  5};

static const int8   thresh_20[] = {39, 42, 45, 48, 51, 53, 54, 57, 60, 63,
        66, 68, 70, 72, 75, 78, 80, 83, 86, 90, 92, 94, 96, 99, 101, 104, 107,
        109, 112, 115, 117, 120, 123, 125, 128, 131, 133}; /* idx 0 --> -36 dBm */
static const int8   thresh_40[] = {41, 44, 46, 48, 50, 52, 54, 56, 58, 60,
        63, 66, 69, 71, 74, 76, 79, 82, 86, 89, 92, 94, 96, 99, 101, 104, 107, 109,
                  112, 115, 117, 120, 123, 125, 128, 131, 133}; /* idx 0 --> -34 dBm */
static const int8   thresh_80[] = {41, 44, 46, 48, 50, 52, 55, 57, 60, 63,
        65, 68, 70, 72, 74, 77, 80, 84, 87, 90, 92, 94, 96, 99, 101, 104, 107, 109,
        112, 115, 117, 120, 123, 125, 128, 131, 133}; /* idx 0 --> -30 dBm */
#define THRESH_SZ_FULL    sizeof(thresh_20)

static const int8 thresh_20M_47[] = { 41,  44,  47,  49,  52,  55,  57,  60,  63,  65,  68,
        71,  73,  76,  79,  81,  84,  87,  89,  92}; /* idx 0 -->-39 dBm */
static const int8 thresh_40M_47[] = { 47,  49,  52,  55,  57,  60,  63,  65,  68,  71,  73,
        76,  79,  81,  84,  87}; /* idx 0 -->-34 dBm */
static const int8 thresh_80M_47[] = { 51,  54,  57,  59,  62,  65,  67,  70,  73,  75,  78,
        81,  83,  85,  88}; /* idx 0 -->-30 dBm */
static const int8 thresh_160M_47[] = { 44,  47,  50,  52,  55,  58,  60,  63,  66,  68,  71,
        74,  76,  78,  81}; /* idx 0 -->-30 dBm */
#define THRESH_SZ_20_47  sizeof(thresh_20M_47)
#define THRESH_SZ_40_47  sizeof(thresh_40M_47)
#define THRESH_SZ_80_47  sizeof(thresh_80M_47)
#define THRESH_SZ_160_47  sizeof(thresh_160M_47)

static const int8 thresh_20M_51[] = { 34,  37,  39,  42,  45,  47,  50,  53,  55,  58,  61,
        63,  66,  69,  71,  74,  77,  79,  82,  85}; /* idx 0 --> -39 dBm */
static const int8 thresh_40M_51[] = { 39,  42,  45,  47,  50,  53,  55,  58,  61,  63,  66,
        69,  71,  74,  77,  79}; /* idx 0 --> -34 dBm */
static const int8 thresh_80M_51[] = { 33,  36,  38,  41,  44,  46,  49,  52,  54,  57,  60,
        62,  65,  68,  70,  73}; /* idx 0 --> -34 dBm */
#define THRESH_SZ_20_51  sizeof(thresh_20M_51)
#define THRESH_SZ_40_51  sizeof(thresh_40M_51)
#define THRESH_SZ_80_51  sizeof(thresh_80M_51)

static const int8 thresh_20M_129[] = { 39,  41,  44,  47,  49,  52,  55,  57,  60,  63,  65,
        68,  71,  73,  76,  79,  81,  84,  87,  89}; /* idx 0 --> -39 dBm */
static const int8 thresh_40M_129[] = { 41,  44,  47,  49,  52,  55,  57,  60,  63,  65,  68,
        71,  73,  76,  79,  81,  84}; /* idx 0 --> -35 dBm */
static const int8 thresh_80M_129[] = { 40,  43,  45,  48,  51,  53,  56,  59,  61,  64,  67,
        69,  72,  75,  77,  80,  82,  85}; /* idx 0 --> -33 dBm */
#define THRESH_SZ_20_129  sizeof(thresh_20M_129)
#define THRESH_SZ_40_129  sizeof(thresh_40M_129)
#define THRESH_SZ_80_129  sizeof(thresh_80M_129)

static const int8 thresh_20M_130[] = { 42,  45,  47,  50,  53,  55,  58,  61,  63,  66,  69,
        71,  74,  76,  79,  81,  84,  86,  89,  92}; /* idx 0 --> -39 dBm */
static const int8 thresh_40M_130[] = { 46,  49,  51,  54,  57,  59,  62,  65,  67,  70,  72,
        75,  78,  80,  83,  87}; /* idx 0 --> -34 dBm */
static const int8 thresh_80M_130[] = { 48,  51,  54,  56,  59,  62,  64,  67,  69,  72,  75,
        77,  80,  82,  85}; /* idx 0 --> -30 dBm */
static const int8 thresh_160M_130[] = { 46,  49,  51,  54,  57,  59,  62,  65,  67,  70,
        73,  76,  78,  80,  83}; /* idx 0 --> -30 dBm */

#define THRESH_SZ_20_130  sizeof(thresh_20M_130)
#define THRESH_SZ_40_130  sizeof(thresh_40M_130)
#define THRESH_SZ_80_130  sizeof(thresh_80M_130)
#define THRESH_SZ_160_130  sizeof(thresh_160M_130)

static uint8
phy_ac_rxgcrs_set_thresh(const phy_ac_rxgcrs_info_t *rxgcrsi, int8 thresh[])
{
    uint8  thresh_sz = 15;    /* i.e. not the full size of the array */
    uint8  i;
    phy_info_t *pi = rxgcrsi->pi;

    BCM_REFERENCE(i);
    if (ACMAJORREV_51_131(pi->pubpi->phy_rev) || ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_20M_51, THRESH_SZ_20_51);
            thresh_sz = THRESH_SZ_20_51;
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_40M_51, THRESH_SZ_40_51);
            thresh_sz = THRESH_SZ_40_51;
        } else { // 80 MHz
            (void)memcpy(thresh, thresh_80M_51, THRESH_SZ_80_51);
            thresh_sz = THRESH_SZ_80_51;
        }
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_20M_129, THRESH_SZ_20_129);
            thresh_sz = THRESH_SZ_20_129;
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_40M_129, THRESH_SZ_40_129);
            thresh_sz = THRESH_SZ_40_129;
        } else { // 80 MHz
            (void)memcpy(thresh, thresh_80M_129, THRESH_SZ_80_129);
            thresh_sz = THRESH_SZ_80_129;
        }
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_20M_130, THRESH_SZ_20_130);
            thresh_sz = THRESH_SZ_20_130;
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_40M_130, THRESH_SZ_40_130);
            thresh_sz = THRESH_SZ_40_130;
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_80M_130, THRESH_SZ_80_130);
            thresh_sz = THRESH_SZ_80_130;
        } else { // 160 MHz
            (void)memcpy(thresh, thresh_160M_130, THRESH_SZ_160_130);
            thresh_sz = THRESH_SZ_160_130;
        }
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_20M_47, THRESH_SZ_20_47);
            thresh_sz = THRESH_SZ_20_47;
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_40M_47, THRESH_SZ_40_47);
            thresh_sz = THRESH_SZ_40_47;
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_80M_47, THRESH_SZ_80_47);
            thresh_sz = THRESH_SZ_80_47;
        } else {//160M
            (void)memcpy(thresh, thresh_160M_47, THRESH_SZ_160_47);
            thresh_sz = THRESH_SZ_160_47;
        }
    } else {
        if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_20, THRESH_SZ_FULL);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            (void)memcpy(thresh, thresh_40, THRESH_SZ_FULL);
        } else {
            (void)memcpy(thresh, thresh_80, THRESH_SZ_FULL);
        }
    }

    /* Increase crsmin by 1.5dB for 4360/43602. Helps with Zero-pkt-loss (less fall triggers) */
    if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)) {
        for (i = 0; i < thresh_sz; i++) {
            thresh[i] += 4;
        }
    } else if (ACMAJORREV_4(pi->pubpi->phy_rev) && rxgcrsi->thresh_noise_cal) {
        thresh_sz = THRESH_SZ_FULL; /* use the full size of the table */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
        /* Increase by 9 for 2G */
            for (i = 3; i < thresh_sz; i++) {
            /* Increase crsmin by 3 dB for 4349 when
            * inband blocker >= -30dBm
            */
                thresh[i] += 9;
            }
        } else {
            /* Increase by 6 for 5G */
            for (i = 12; i < thresh_sz; i++) {
            /* Increase crsmin by 2dB for 4349 when inband blocker >= -24dBm */
                thresh[i] += 6;
            }
        }
    }

    return thresh_sz;
}

int
phy_ac_rxgcrs_min_pwr_cal(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 crsmin_cal_mode)
{
    int ret = BCME_OK;
    int8   cmplx_pwr_dbm[PHY_CORE_MAX];
    uint8  thresh_sz;
    int8   thresh[THRESH_SZ_FULL];
    uint8  core;
    uint8  fp[PHY_CORE_NUM_4];
    int8  lesi_scale[PHY_CORE_MAX] = {0};
    uint8  run_cal = 0;
    uint8  max_idx = 0;
    uint8 enRx = 0;
    int8 offset[PHY_CORE_NUM_4] = { 0 };
    uint8 fp_min_of_all_cores = 255;
    uint8 cal_run = 0; /* 0-none, 1-crs, 2-lesi, 3-both */
    phy_info_t *pi;
    phy_stf_data_t *stf_shdata;

    ASSERT(rxgcrsi != NULL);
    pi = rxgcrsi->pi;
    ASSERT(pi != NULL);
    stf_shdata = phy_stf_get_data(pi->stfi);

    bzero((uint8 *)cmplx_pwr_dbm, sizeof(cmplx_pwr_dbm));
    bzero((uint8 *)fp, sizeof(fp));

    thresh_sz = phy_ac_rxgcrs_set_thresh(rxgcrsi, thresh);    /* configures thresh array */

    /* Initialize */
    FOREACH_CORE(pi, core) {
        fp[core] = 0x36;
    }

    enRx = stf_shdata->phyrxchain;

    /* upate the noise pwr array with most recent noise pwr */
    if (crsmin_cal_mode == PHY_CRS_RUN_AUTO) {
        FOREACH_CORE(pi, core)  {
            rxgcrsi->phy_noise_pwr_array[rxgcrsi->phy_noise_counter][core] =
            phy_ac_noise_get_data(pi->u.pi_acphy->noisei)->phy_noise_all_core[core] + 1;
        }
        rxgcrsi->phy_noise_counter = (rxgcrsi->phy_noise_counter + 1) %
                                     PHY_SIZE_NOISE_ARRAY;
    }

    FOREACH_CORE(pi, core) {
        uint8 idxlists;
        uint8 chan_freq_range = phy_ac_rxgcrs_get_chan_freq_range(pi, core);

        ret = phy_ac_rxgcrs_get_thresh_idx(rxgcrsi, core, crsmin_cal_mode,
                                           chan_freq_range, cmplx_pwr_dbm, &idxlists);
        if (ret < 0) {
            goto exit;
        }
        if (ret == 1) {
            run_cal++;
            ret = BCME_OK;
        }

        if (idxlists > (thresh_sz - 1)) {
            idxlists = thresh_sz - 1;
        }

        max_idx  = MAX(max_idx, idxlists);
        phy_ac_rxgcrs_fp_offset_calc(rxgcrsi, fp, offset, cmplx_pwr_dbm, thresh,
                                           lesi_scale, idxlists, core);

        fp_min_of_all_cores = MIN(fp_min_of_all_cores, fp[core]);

        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            int8 jammer_db = cmplx_pwr_dbm[core] - 69 - (-95);
            if (jammer_db < 5) jammer_db = 0;
            if (jammer_db > 35) jammer_db = 35;
            WRITE_PHYREGC(pi, ACIJammerDeltaDb, core, jammer_db);
        }

    }

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        offset[1] = fp[1] - fp_min_of_all_cores;
        offset[2] = fp[2] - fp_min_of_all_cores;
        offset[3] = fp[3] - fp_min_of_all_cores;
        fp[0] = fp_min_of_all_cores;
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        offset[0] = fp[0] - fp_min_of_all_cores;
        offset[1] = fp[1] - fp_min_of_all_cores;
        offset[2] = fp[2] - fp_min_of_all_cores;
        offset[3] = fp[3] - fp_min_of_all_cores;
    }

    /* check if current noise pwr is different from the one in cache */
    if ((run_cal == 0) && (crsmin_cal_mode == PHY_CRS_RUN_AUTO)) {
        goto exit;
    }

    cal_run = 3;   /* run both crs & lesi cal */

    /* Below flag is set from phy_cals only */
    if (crsmin_cal_mode == PHY_CRS_RUN_AUTO)
        rxgcrsi->force_crsmincal = FALSE;

    /* if noise desense is on, then the below variable will be used for comparison */
    rxgcrsi->phy_crs_th_from_crs_cal = MAX(MAX(fp[0], fp[1]), fp[2]);

    if (((!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) &&
        pi->phywatchdog_override) || ((pi->phywatchdog_override) && IS_4364_3x3(pi))) {
        if (!ACHWACIREV(pi)) {
            /* if desense is forced, then reset the variable below to default */
            if (rxgcrsi->desense_forced_dB != 0) {
                rxgcrsi->phy_crs_th_from_crs_cal = ACPHY_CRSMIN_DEFAULT;
                cal_run = 2;  /* skip crs-cal */
                goto exit;
            }
        }

        /* Don't update the crsmin registers if any desense(aci/bt) is on */
        if (rxgcrsi->total_desense->on) {
            cal_run = 2;   /* skip crs-cal */
            goto exit;
        }
    }

    /* Debug: keep count of all calls to crsmin_cal  */
    /* Debug: store the channel info  */
    /* Debug: store the noise pwr used for updating crs thresholds */
    rxgcrsi->phy_debug_crscal_counter++;
    rxgcrsi->phy_debug_crscal_channel = CHSPEC_CHANNEL(pi->radio_chanspec);
    FOREACH_CORE(pi, core)  {
        /* Debug info: for dumping the noise pwr used in crsmin_cal */
        rxgcrsi->phy_noise_in_crs_min[core] = cmplx_pwr_dbm[core];
    }

exit:
    if (cal_run == 3) {
        /* This runs both crs & lesi cal */
        phy_ac_rxgcrs_set_min_pwr(rxgcrsi, fp, offset, lesi_scale,
                                  fp_min_of_all_cores, enRx);
    } else if (cal_run == 2) {
        /* crs-cal is skipped, but do call lesiscale */
        if (rxgcrsi->lesiscalecal_enable)
            phy_ac_rxgcrs_set_lesiscale(rxgcrsi, lesi_scale);
    }

    return ret;
}

static void phy_ac_rxgcrs_fp_offset_calc(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 fp[], int8 offset[],
    int8 cmplx_pwr_dbm[], int8 thresh[], int8 lesi_scale[], uint8 idxlists, uint8 core)
{
    phy_info_t *pi = rxgcrsi->pi;
    uint8 max_fp = 255;

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev)) {
        int8 weakest_rssi = phy_ac_noise_get_weakest_rssi(rxgcrsi->aci->noisei);

        /* Get weakest rssi, and find maximum crsmin allowed based on rssi */
        if (weakest_rssi != 0) {
            uint8 max_desense = MAX(0, weakest_rssi + 87);

            // 8 ticks is 3dBs. 8/3 = 43/16
            max_fp = MIN(max_fp, ACPHY_CRSMIN_DEFAULT + ((max_desense*43)>>4));
        }
    }
    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
        uint8 idxlists_lesi = phy_ac_rxgcrs_get_thresh_idx_lesi(pi, core, cmplx_pwr_dbm);

        if (ISSIM_ENAB(pi->sh->sih)) {
            fp[core] = fp[0];
        } else {
            fp[core] = thresh[idxlists];
        }

        /* Limit fp based on weakest rssi */
        if (fp[core] > max_fp)
            fp[core] = max_fp;

        lesi_scale[core] = scale_lesi[idxlists_lesi];
    } else {
        fp[core] = thresh[idxlists];

        offset[core] = fp[core] - fp[0];
        if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
            if (core == 1) {    /* core 1 */
                /* If the offset is > 6dB assume degenerate RX senario */
                if (ABS(offset[1]) > 16) {
                    fp[0] = MAX(fp[0], fp[1]);
                    offset[1] = 0;
                }
                if ((rxgcrsi->srom_asymmetricjammermod)) {
                    phy_ac_rxgcrs_asymmetric_jammer_mod(pi, cmplx_pwr_dbm);
                }
            } else if (core == 2) {    /* core 2 */
                /* If the offset is > 6dB assume degenerate RX senario */
                if (ABS(offset[2]) > 16) {
                    fp[0] = MAX(fp[0], fp[2]);
                    offset[1] = 0;
                    offset[2] = 0;
                }
            }
        }
    }
}

static int8
phy_ac_rxgcrs_get_avg_noisepwr(int8 noises[])
{
    int16 accum = 0;
    int8 min_val, avg_val = 0;
    uint8 i, j, loop, min_idx, cnt = 0;

    /* Take only lowest half noise vals, as higher values could be over pkt */
    loop = PHY_SIZE_NOISE_ARRAY >> 1;
    for (j = 0; j < loop; j++) {
        min_val = 100;  min_idx = PHY_SIZE_NOISE_ARRAY;
        for (i = 0; i < PHY_SIZE_NOISE_ARRAY; i++) {
            if ((noises[i] != 0) && (noises[i] < min_val)) {
                min_val = noises[i];
                min_idx = i;
            }
        }

        if (min_idx < PHY_SIZE_NOISE_ARRAY) {
            accum += noises[min_idx];
            noises[min_idx] = 0;
            cnt++;
        } else {
            break;    // no more valid values
        }
    }

    /* Average it */
    if (cnt > 0) avg_val = accum / cnt;

    return avg_val;
}

static int
phy_ac_rxgcrs_get_avg_noisepwr_per_core(const phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core,
    int8 cmplx_pwr_dbm[])
{
    uint8 cnt;
    const phy_info_t *pi = rxgcrsi->pi;

    BCM_REFERENCE(pi);
    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE47(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
        int8   noises_core[PHY_SIZE_NOISE_ARRAY];

        /* noises_core would be change in get_avg_noisepwr, so be careful */
        for (cnt = 0; cnt < PHY_SIZE_NOISE_ARRAY; cnt++) {
            noises_core[cnt] = rxgcrsi->phy_noise_pwr_array[cnt][core];
        }

        /* Get avg noise value discarding few max values */
        cmplx_pwr_dbm[core] = phy_ac_rxgcrs_get_avg_noisepwr(noises_core);

        if (cmplx_pwr_dbm[core] == 0) {
            return BCME_ERROR;
        }
    } else {
        /* Enter here only from ucode noise interrupt */
        int16  acum_noise_pwr = 0;

        /* average of the most recent noise pwrs */
        for (cnt = 0; cnt < PHY_SIZE_NOISE_ARRAY; cnt++) {
            if (rxgcrsi->phy_noise_pwr_array[cnt][core] != 0)
            {
                acum_noise_pwr += rxgcrsi->phy_noise_pwr_array[cnt][core];
            } else {
                break;
            }
        }

        if (cnt != 0) {
            cmplx_pwr_dbm[core] = acum_noise_pwr / cnt;
        } else {
            return BCME_ERROR;
        }
    }

    return BCME_OK;
}

static void
phy_ac_rxgcrs_set_crs_min_pwr(phy_info_t *pi, uint8 ac_th, int8 offset_0, int8 offset_1,
    int8 offset_2, int8 offset_3, bool sec_only)
{
    uint8 mfcrs_1bit;    /* match filter carrier sense 1 bit mode */
    uint8 mf_th = ac_th;
    int8  mf_off0 = offset_0;
    int8  mf_off1 = offset_1;
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    uint8 rxcrs_prim_loc = CHSPEC_SB(pi->radio_chanspec);
    BCM_REFERENCE(mf_off0);

    if (!ACMAJORREV_32(pi->pubpi->phy_rev) && !ACMAJORREV_33(pi->pubpi->phy_rev))
        PHY_TRACE(("%s: AC %d\n", __FUNCTION__, ac_th));

    /* 1-bit MF: 4335A0, 4335B0, 4350 */
    /* 6-bit MF: 4335C0 */
    if (ACMAJORREV_2(pi->pubpi->phy_rev) && ACMINORREV_0(pi)) {
        mfcrs_1bit = 1;
    } else {
        mfcrs_1bit = 0;
    }

    if (ac_th == 0) {
        if (mfcrs_1bit == 1) {
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                mf_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 60 :
                    pi_ac->paramsi->mfcrs_th_bw20;
                ac_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 58 :
                    pi_ac->paramsi->accrs_th_bw20;
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                mf_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 60 :
                    pi_ac->paramsi->mfcrs_th_bw40;
                ac_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 58 :
                    pi_ac->paramsi->accrs_th_bw40;
            } else if (CHSPEC_IS80(pi->radio_chanspec) ||
                PHY_AS_80P80(pi, pi->radio_chanspec)) {
                mf_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 67 :
                    pi_ac->paramsi->mfcrs_th_bw80;
                ac_th = (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev)) ? 60 :
                    pi_ac->paramsi->accrs_th_bw80;
            } else if (CHSPEC_IS160(pi->radio_chanspec)) {
                ASSERT(0);
            }
        } else {
            mf_th = ACPHY_CRSMIN_DEFAULT;
            ac_th = ACPHY_CRSMIN_DEFAULT;
        }
        pi_ac->rxgcrsi->phy_crs_th_from_crs_cal = ac_th;
    } else {
        if (mfcrs_1bit == 1) {
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                mf_th = ((ac_th*101)/100) + 2;
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                mf_th = ((ac_th*109)/100) - 2;
            } else if (CHSPEC_IS80(pi->radio_chanspec) ||
                PHY_AS_80P80(pi, pi->radio_chanspec)) {
                mf_th = ((ac_th*105)/100) + 2;
            } else if (CHSPEC_IS160(pi->radio_chanspec)) {
                mf_th = ((ac_th*105)/100) + 2;
                //ASSERT(0);
            }
        }
    }

    /* Adjust offset values for 1-bit MF */
    /* Not needed for 4335 and 4360, will be needed for 4350, disabled for now */

    if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
        uint8 phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;

        BCM_REFERENCE(phyrxchain);
        if (((PHYCOREMASK(phyrxchain) >> 1) & 0x1) &&
            (CHSPEC_ISPHY5G6G(pi->radio_chanspec))) {
            if (CHSPEC_IS20(pi->radio_chanspec)) {
                mf_off1 = ((((ac_th+offset_1)*101)/100) + 2)-mf_th;
            } else if (CHSPEC_IS40(pi->radio_chanspec)) {
                mf_off1 = ((((ac_th+offset_1)*109)/100) - 2)-mf_th;
            } else if (CHSPEC_IS80(pi->radio_chanspec)) {
                mf_off1 = ((((ac_th+offset_1)*105)/100) + 2)-mf_th;
            }
        }
    }

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
        PHY_CAL(("%s: (AC_th, MF_th) = (%d, %d)\n", __FUNCTION__, ac_th, mf_th));

    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        /* Desensing AC-CRS by 16 ticks helps lowering error floors
        * for 5G 40MHz/80MHz channels
        */
        if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
            !CHSPEC_BW_LE20(pi->radio_chanspec)) {
            ac_th += 9; /* match dingo */
        }
    } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
        /* Desensing AC-CRS helps lowering error floors
         * in presense of low LTE jammers
         */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            ac_th += 12;
        } else {
            ac_th += 10;
        }
        if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
            !CHSPEC_BW_LE20(pi->radio_chanspec)) {
            ac_th += 12;
        }
        if (CHSPEC_IS80(pi->radio_chanspec)) {
            ac_th += 8;
        }

    }

#ifdef OCL
    if (PHY_OCL_ENAB(pi->sh->physhim) && ACMAJORREV_4(pi->pubpi->phy_rev)) {
        uint8 active_cm;
        bool ocl_en;
        phy_ocl_status_get(pi, NULL, &active_cm, &ocl_en);
        if (ocl_en) {
            if (active_cm == 1) {
                mf_th = mf_th + mf_off0;
                ac_th = ac_th + offset_0;
                offset_0 = CRSMIN_MIN;
                mf_off0 = CRSMIN_MIN;
                offset_1 = CRSMIN_MAX;
                mf_off1 = CRSMIN_MAX;
            } else if (active_cm == 2) {
                mf_th = mf_th + mf_off1;
                ac_th = ac_th + offset_1;
                offset_1 = CRSMIN_MIN;
                mf_off1 = CRSMIN_MIN;
                offset_0 = CRSMIN_MAX;
                mf_off0 = CRSMIN_MAX;
            }
        }
    }
#endif /* OCL */

    if (!(sec_only && (rxcrs_prim_loc == 1))) {
        MOD_PHYREG(pi, crsminpoweru0, crsminpower0, ac_th);
        MOD_PHYREG(pi, crsmfminpoweru0, crsmfminpower0, mf_th);
    }
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        if (!(sec_only && (rxcrs_prim_loc == 0))) {
            MOD_PHYREG(pi, crsminpowerl0, crsminpower0, ac_th);
            MOD_PHYREG(pi, crsmfminpowerl0, crsmfminpower0, mf_th);
        }
        if (!(sec_only && (rxcrs_prim_loc == 2))) {
            MOD_PHYREG(pi, crsminpowerlSub10, crsminpower0, ac_th);
            MOD_PHYREG(pi, crsmfminpowerlSub10, crsmfminpower0,  mf_th);
        }
        if (!(sec_only && (rxcrs_prim_loc == 3))) {
            MOD_PHYREG(pi, crsminpoweruSub10, crsminpower0, ac_th);
            MOD_PHYREG(pi, crsmfminpoweruSub10, crsmfminpower0,  mf_th);
        }
        if (ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
            //BW160 crsmin setting
            if (!(sec_only && (rxcrs_prim_loc == 4))) {
                MOD_PHYREG(pi, crsminpowerULL0, crsminpower0, ac_th);
                MOD_PHYREG(pi, crsmfminpowerULL0, crsmfminpower0, mf_th);
            }
            if (!(sec_only && (rxcrs_prim_loc == 5))) {
                MOD_PHYREG(pi, crsminpowerULU0, crsminpower0, ac_th);
                MOD_PHYREG(pi, crsmfminpowerULU0, crsmfminpower0, mf_th);
            }
            if (!(sec_only && (rxcrs_prim_loc == 6))) {
                MOD_PHYREG(pi, crsminpowerUUL0, crsminpower0, ac_th);
                MOD_PHYREG(pi, crsmfminpowerUUL0, crsmfminpower0, mf_th);
            }
            if (!(sec_only && (rxcrs_prim_loc == 7))) {
                MOD_PHYREG(pi, crsminpowerUUU0, crsminpower0, ac_th);
                MOD_PHYREG(pi, crsmfminpowerUUU0, crsmfminpower0, mf_th);
            }
        }
    }

    wlc_phy_set_crs_min_offsets_acphy(pi, 0, offset_0, mf_off0);
    if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev)) {
        /* Force the offsets for core-0 */
        /* Core 0 */
        wlc_phy_set_crs_min_offsets_acphy(pi, 0, 0, 0);
    }

    if (PHYCORENUM(pi->pubpi->phy_corenum) > 1) {
        /* Force the offsets for core-1 */
        /* Core 1 */
        wlc_phy_set_crs_min_offsets_acphy(pi, 1, offset_1, mf_off1);
    }
    if (PHYCORENUM(pi->pubpi->phy_corenum) > 2) {
        /* Force the offsets for core-2 */
        /* Core 2 */
        wlc_phy_set_crs_min_offsets_acphy(pi, 2, offset_2, offset_2);
    }
    if (PHYCORENUM(pi->pubpi->phy_corenum) > 3) {
        /* Force the offsets for core-3 */
        /* Core 3 */
        wlc_phy_set_crs_min_offsets_acphy(pi, 3, offset_3, offset_3);
    }
}

void
wlc_phy_set_crs_min_offsets_acphy(phy_info_t *pi, uint8 core, int8 ac_offset, int8 mf_offset)
{
    MOD_PHYREGCE(pi, crsminpoweroffset, core, crsminpowerOffsetu, ac_offset);
    MOD_PHYREGCE(pi, crsmfminpoweroffset, core, crsmfminpowerOffsetu, mf_offset);
    if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
        MOD_PHYREGCE(pi, crsminpoweroffset, core, crsminpowerOffsetl,
            ac_offset);
        MOD_PHYREGCE(pi, crsmfminpoweroffset, core, crsmfminpowerOffsetl,
            mf_offset);
        MOD_PHYREGCE(pi, crsminpoweroffsetSub1, core, crsminpowerOffsetlSub1,
            ac_offset);
        MOD_PHYREGCE(pi, crsmfminpoweroffsetSub1, core, crsmfminpowerOffsetlSub1,
            mf_offset);
        MOD_PHYREGCE(pi, crsminpoweroffsetSub1, core, crsminpowerOffsetuSub1,
            ac_offset);
        MOD_PHYREGCE(pi, crsmfminpoweroffsetSub1, core, crsmfminpowerOffsetuSub1,
            mf_offset);
        if (ACMAJORREV_47_129_130(pi->pubpi->phy_rev)) {
            //for BW160 crsmin phyreg
            MOD_PHYREGCE(pi, crsminpoweroffset2_, core,
                crsminpowerOffsetulu, ac_offset);
            MOD_PHYREGCE(pi, crsminpoweroffset2_, core,
                crsminpowerOffsetull, ac_offset);
            MOD_PHYREGCE(pi, crsminpoweroffset3_, core,
                crsminpowerOffsetuuu, ac_offset);
            MOD_PHYREGCE(pi, crsminpoweroffset3_, core,
                crsminpowerOffsetuul, ac_offset);
            MOD_PHYREGCE(pi, crsmfminpoweroffset2_, core,
                crsmfminpowerOffsetulu, mf_offset);
            MOD_PHYREGCE(pi, crsmfminpoweroffset2_, core,
                crsmfminpowerOffsetull, mf_offset);
            MOD_PHYREGCE(pi, crsmfminpoweroffset3_, core,
                crsmfminpowerOffsetuuu, mf_offset);
            MOD_PHYREGCE(pi, crsmfminpoweroffset3_, core,
                crsmfminpowerOffsetuul, mf_offset);
        }
    }
}

void
phy_ac_rxgcrs_set_lesiscale(phy_ac_rxgcrs_info_t *rxgcrsi, int8 *lesi_scale)
{
    uint8 core;
    phy_info_t *pi = rxgcrsi->pi;

    if (rxgcrsi->lesi_cap) {
        uint8 phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
        BCM_REFERENCE(phyrxchain);
        FOREACH_ACTV_CORE(pi, phyrxchain, core) {
            if (ACMAJORREV_32(pi->pubpi->phy_rev)) {
                MOD_PHYREGCE(pi, lesiPathControl0, core,
                    inpScalingFactor, lesi_scale[core]);
            } else {
                MOD_PHYREGCE(pi, lesiInputScaling0_, core,
                    inpScalingFactor_0, lesi_scale[core]);
                MOD_PHYREGCE(pi, lesiInputScaling1_, core,
                    inpScalingFactor_1, lesi_scale[core]);
                MOD_PHYREGCE(pi, lesiInputScaling2_, core,
                    inpScalingFactor_2, lesi_scale[core]);
                MOD_PHYREGCE(pi, lesiInputScaling3_, core,
                    inpScalingFactor_3, lesi_scale[core]);
                if (ACMAJORREV_47_130(pi->pubpi->phy_rev) ||
                    ACMAJORREV_131(pi->pubpi->phy_rev)) {
                    MOD_PHYREGCE(pi, lesiInputScaling4_, core,
                        inpScalingFactor_4, lesi_scale[core]);
                    MOD_PHYREGCE(pi, lesiInputScaling5_, core,
                        inpScalingFactor_5, lesi_scale[core]);
                    MOD_PHYREGCE(pi, lesiInputScaling6_, core,
                        inpScalingFactor_6, lesi_scale[core]);
                    MOD_PHYREGCE(pi, lesiInputScaling7_, core,
                        inpScalingFactor_7, lesi_scale[core]);
                }
            }
        }
    }
}

static void
phy_ac_rxgcrs_stay_in_carriersearch(phy_type_rxgcrs_ctx_t *ctx, bool enable)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;
    phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
    uint8 class_mask, phyrxchain;

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    /* MAC should be suspended before calling this function */
    ASSERT((R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC) == 0);
    if (enable) {
        if (pi_ac->rxgcrsi->deaf_count == 0) {
            phy_rxgcrs_sel_classifier(pi, 4);
            wlc_phy_ofdm_crs_acphy(pi, FALSE);
            wlc_phy_clip_det_acphy(pi, FALSE);

            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
                (wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
                WRITE_PHYREG(pi, ed_crsEn_80u, 0);
            }

            WRITE_PHYREG(pi, ed_crsEn, 0);
            if (pi->pubpi->phy_rev < 32)
                wlc_phy_resetcca_acphy(pi);
        }

        pi_ac->rxgcrsi->deaf_count++;
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
            wlc_phy_resetcca_acphy(pi);
        }
    } else {
      ASSERT(pi_ac->rxgcrsi->deaf_count > 0);

        pi_ac->rxgcrsi->deaf_count--;
        if (pi_ac->rxgcrsi->deaf_count == 0) {
            class_mask = CHSPEC_IS2G(pi->radio_chanspec) ? 7 : 6;   /* No bphy in 5g */
            phy_rxgcrs_sel_classifier(pi, class_mask);
            wlc_phy_ofdm_crs_acphy(pi, TRUE);
            wlc_phy_clip_det_acphy(pi, TRUE);

            /* edcrs gets enabled back for all cores. So re-apply edcrs_en only */
            /* for the active core(s) for non-OCL and OCL cases */
            if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
                phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
                BCM_REFERENCE(phyrxchain);
                if (phyrxchain == 1) {
                    WRITE_PHYREG(pi, ed_crsEn, 0x00F);
                }
                if (phyrxchain == 2) {
                    WRITE_PHYREG(pi, ed_crsEn, 0x0F0);
                }
                if (phyrxchain == 3) {
                    WRITE_PHYREG(pi, ed_crsEn, 0x0FF);
#ifdef OCL
                    uint8 active_cm;
                    bool ocl_en;
                    phy_ocl_status_get(pi, NULL,
                        &active_cm, &ocl_en);
                    if (ocl_en) {
                        if (active_cm == 1) {
                            WRITE_PHYREG(pi, ed_crsEn, 0x00F);
                        } else {
                            WRITE_PHYREG(pi, ed_crsEn, 0x0F0);
                        }
                    }
#endif
                }
            } else {
                WRITE_PHYREG(pi, ed_crsEn, pi_ac->rxgcrsi->edcrs_en);

                if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
                    (wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
                    WRITE_PHYREG(pi, ed_crsEn_80u,
                        pi_ac->rxgcrsi->edcrs_en);
                }

            }
        }
    }
}

#if defined(BCMDBG)
void wlc_phy_force_gainlevel_acphy(phy_info_t *pi, int16 int_val)
{
    uint8 core;
    bool suspend = FALSE;
    uint8 phyrxchain;

    BCM_REFERENCE(phyrxchain);

    /* Supsend MAC */
    wlc_phy_conditional_suspend(pi, &suspend);

    phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
    FOREACH_ACTV_CORE(pi, phyrxchain, core) {
        /* disable clip2 */
        MOD_PHYREGC(pi, computeGainInfo, core, disableClip2detect, 1);
        WRITE_PHYREGC(pi, Clip2Threshold, core, 0xffff);
        printf("wlc_phy_force_gainlevel_acphy (%d) : ", int_val);
        switch (int_val) {
        case 0:
            printf("initgain -- adc never clips.\n");
            if (ACREV_IS(pi->pubpi->phy_rev, 0)) {
                WRITE_PHYREGC(pi, Clip1Threshold, core, 0xffff);
            } else {
                /* In 4349 to force INIT gain in core-1, we have to disable
                 * Clip1Detect in core-0, writing 1 to core-1
                 * Disableclip1detect register has no effect
                 */
                if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
                    MOD_PHYREGC(pi, computeGainInfo, 0, disableClip1detect, 1);
                    WRITE_PHYREGC(pi, Clip1Threshold, 0, 0xffff);
                    WRITE_PHYREGC(pi, Adcclip, 0, 0xff);
                } else {
                    MOD_PHYREGC(pi, computeGainInfo, core,
                            disableClip1detect, 1);
                    WRITE_PHYREGC(pi, Clip1Threshold, core, 0xffff);
                    WRITE_PHYREGC(pi, Adcclip, core, 0xff);
                }
            }
            break;
        case 1:
            printf("clip hi -- adc always clips, nb never clips.\n");
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcNbClipCntTh, 0xff);
            break;
        case 2:
            printf("clip md -- adc/nb always clips, w1 never clips.\n");
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcNbClipCntTh, 0);
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcW1ClipCntTh, 0xff);
            break;
        case 3:
            printf("clip lo -- adc/nb/w1 always clips.\n");
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcNbClipCntTh, 0);
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcW1ClipCntTh, 0);
            break;
        case 4:
            printf("adc clip.\n");
            WRITE_PHYREGC(pi, clipHiGainCodeA, core, 0x0);
            WRITE_PHYREGC(pi, clipHiGainCodeB, core, 0x8);
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcNbClipCntTh, 0xff);
            break;
        case 5:
            printf("nb clip.\n");
            WRITE_PHYREGC(pi, clipmdGainCodeA, core, 0xfffe);
            WRITE_PHYREGC(pi, clip2GainCodeA, core, 0xfffe);
            WRITE_PHYREGC(pi, clipmdGainCodeB, core, 0x554);
            MOD_PHYREGC(pi, FastAgcClipCntTh, core, fastAgcW1ClipCntTh, 0xff);
            WRITE_PHYREGC(pi, clip2GainCodeB, core, 0x554);
            break;
        case 6:
            printf("w1 clip.\n");
            WRITE_PHYREGC(pi, cliploGainCodeA, core, 0xfffe);
            WRITE_PHYREGC(pi, clip2GainCodeA, core, 0xfffe);
            WRITE_PHYREGC(pi, cliploGainCodeB, core, 0x554);
            MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, 0);
            WRITE_PHYREGC(pi, clip2GainCodeB, core, 0x554);
            if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
                MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_Refctrl_low, 1);
            }
            break;
        }
    }
    printf("wlc_phy_force_gainlevel_acphy (%d)\n", int_val);

    /* Resume MAC */
    wlc_phy_conditional_resume(pi, &suspend);
}
#endif

uint8 wlc_phy_get_lna_gain_rout(phy_info_t *pi, uint8 idx, acphy_lna_gain_rout_t type)
{
    uint8 ret_val;
    phy_ac_rxg_params_t *rxg_params = pi->u.pi_acphy->rxgcrsi->rxg_params;

    if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        if (type == GET_LNA_GAINCODE) {
            ret_val = (rxg_params->rftbls->lna1Rout_tbl[idx] & 0x7);
        } else {
            ret_val = (rxg_params->rftbls->lna1Rout_tbl[idx] >> 3);
        }
    } else {
        if (type == GET_LNA_GAINCODE) {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                ret_val = ac_tiny_g_lna_gain_map[idx];
            } else {
                ret_val = ac_tiny_a_lna_gain_map[idx];
            }
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                ret_val = ac_tiny_g_lna_rout_map[idx];
            } else {
                ret_val = ac_tiny_a_lna_rout_map[idx];
            }
        }
    }
    return (ret_val);
}

/* This function tells you locale info, e.g EU, so that correct edcrs setting could be done */
static void
phy_ac_rxgcrs_set_locale(phy_type_rxgcrs_ctx_t *ctx, uint8 region_group)
{
    /* USAGE: if (region_group == LOCALE_EU) */
    /* Nothing for now - just a template */
}

static void
wlc_phy_rxgainctrl_set_gaintbls_acphy_2069(phy_info_t *pi,
    uint8 core, uint16 gain_tblid, uint16 gainbits_tblid)
{
    uint16 gmsz;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    acphy_desense_values_t *desense = pi_ac->rxgcrsi->total_desense;

    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        uint8 mixbits_2g[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
        uint8 mix_2g_43352_ilna[] = {0x9, 0x9, 0x9, 0x9, 0x9,
            0x9, 0x9, 0x9, 0x9, 0x9};
        uint8 mix_2g[]  = {0x3, 0x3, 0x3, 0x3, 0x3,
            0x3, 0x3, 0x3, 0x3, 0x3};
        if (PHY_ILNA(pi)) {
            /* Use lna2_gm_sz = 3 (for ACI), mix/tia_gm_sz = 2 */
            ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, 0x2c);
            ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);

            wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_2g_43352_ilna);
            wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_2g);

            /* copying values into gaintbl arrays
                to avoid reading from table
            */
            memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[3],
                mix_2g_43352_ilna,
                sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[3]);
        } else {
            if ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
                 RADIOMAJORREV(pi) == 2) ||
                (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) &&
                 RADIOMAJORREV(pi) == 0 &&
                 (RADIOMINORREV(pi) == 16 || RADIOMINORREV(pi) == 17))) {
                /* Use lna2_gm_sz = 2 (for ACI), mix/tia_gm_sz = 1 */
                gmsz = desense->elna_bypass ? 0x2c : 0x28;
                wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8,
                    mix_2g_43352_ilna);
                /* copying values into gaintbl arrays
                to avoid reading from table
                */
                memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[3],
                    mix_2g_43352_ilna,
                    sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[3]);
            } else {
                /* Use lna2_gm_sz = 2 (for ACI), mix/tia_gm_sz = 1 */
                gmsz = desense->elna_bypass ? 0x1c : 0x18;
                wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_2g);
                memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[3], mix_2g,
                    sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[3]);
            }
            ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, gmsz);
            ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);
            wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_2g);
            /* copying values into gaintbl arrays to avoid reading from table */
            memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[3], mixbits_2g,
                sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[3]);
        }
    } else {
        /* 5g settings */
        uint8 mix5g_elna[]  = {0x7, 0x7, 0x7, 0x7, 0x7,
                               0x7, 0x7, 0x7, 0x7, 0x7};
        uint8 mixbits5g_elna[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
        uint8 mix5g_ilna[]  = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16};
        uint8 mixbits5g_ilna[] = {5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
        uint8 *mix_5g;
        uint8 *mixbits_5g;

        /* Mixer tables based on elna/ilna */
        if (BF_ELNA_5G(pi_ac)) {
            mix_5g = mix5g_elna;
            mixbits_5g = mixbits5g_elna;
        } else {
            mix_5g = mix5g_ilna;
            mixbits_5g = mixbits5g_ilna;
        }

        /* Use lna2_gm_sz = 3, mix/tia_gm_sz = 2 */
        ACPHYREG_BCAST(pi, RfctrlCoreLowPwr0, 0x2c);
        ACPHYREG_BCAST(pi, RfctrlOverrideLowPwrCfg0, 0xc);

        wlc_phy_table_write_acphy(pi, gain_tblid, 10, 32, 8, mix_5g);
        wlc_phy_table_write_acphy(pi, gainbits_tblid, 10, 32, 8, mixbits_5g);

        /* copying values into gaintbl arrays to avoid reading from table */
        memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[3], mix_5g,
               sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[3]);
    }
}

void
phy_ac_rxgcrs_upd_mix_gains(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    phy_info_t *pi = rxgcrsi->pi;
    acphy_desense_values_t *desense = NULL;
    uint8 core, idx, desense_state;
    int8 tia_mix_gain[13];
    uint8 tia_mix_gainbits[13];
    uint8 tia_mix_gainlimits[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7f};
    uint8 gain_tbl_entry_size =
        sizeof(rxgcrsi->rxgainctrl_params[0].gaintbl[0][0]);
    uint8 gainbit_tbl_entry_size =
        sizeof(rxgcrsi->rxgainctrl_params[0].gainbitstbl[0][0]);
    uint16 gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT0;
    uint16 gain_tblid = ACPHY_TBL_ID_GAIN0;
    uint16 gainbits_tblid = ACPHY_TBL_ID_GAINBITS0;
    const uint8 *default_gain_tbl = ac_tia_gain_tiny_4365;
    const uint8 *default_gainbits_tbl = ac_tia_gainbits_tiny_4365;
    uint8 stall_val;

    if ((desense = phy_ac_noise_get_desense(rxgcrsi->aci->noisei)) == NULL) {
        desense_state = 0;
    } else {
        desense_state = phy_ac_noise_get_desense_state(rxgcrsi->aci->noisei);
    }

    /* disable stalls for table write */
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    if (desense_state != 0) {
        for (idx = 0; idx < rxgcrsi->rxgainctrl_stage_len[TIA_ID]; idx++) {
            tia_mix_gainbits[idx] = MIN(desense->mix_idx_max,
                    MAX(idx, desense->mix_idx_min));
            tia_mix_gain[idx] = default_gain_tbl[tia_mix_gainbits[idx]];
        }
    } else {
        memcpy(tia_mix_gain, default_gain_tbl,
                sizeof(uint8) * rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
        memcpy(tia_mix_gainbits, default_gainbits_tbl,
                sizeof(uint8) * rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
    }

    for (core = 0; core < PHY_CORE_MAX; core++) {
        gain_tblid = ACPHY_TBL_ID_GAIN0 + core * 32;
        gainbits_tblid = ACPHY_TBL_ID_GAINBITS0 + core * 32;
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT0 + core * 32;

        wlc_phy_set_agc_gaintbls_acphy(
            pi,
            gain_tblid, tia_mix_gain,
            gainbits_tblid, tia_mix_gainbits,
            gainlimit_tblid, tia_mix_gainlimits, tia_mix_gainlimits,
            TIA_OFFSET,
            rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
        memcpy(rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID],
               tia_mix_gain,
               gain_tbl_entry_size * rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
        memcpy(rxgcrsi->rxgainctrl_params[core].gainbitstbl[TIA_ID],
               tia_mix_gainbits,
               gainbit_tbl_entry_size * rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
    }

    ACPHY_ENABLE_STALL(pi, stall_val);
}

void
wlc_phy_rxgainctrl_set_gaintbls_acphy_wave2(
    phy_info_t *pi,
    uint8      core,
    uint16     gain_tblid,
    uint16     gainbits_tblid)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    bool elna_present = CHSPEC_IS2G(pi->radio_chanspec) ?
        BF_ELNA_2G(pi_ac) : BF_ELNA_5G(pi_ac);
    int8 elna = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0][0];
    int8 elna_gain[] = {0, 0};
    uint8 elna_gainbits[] = {0, 0};
    uint8 elna_gainlimits[] = {0, 0};

    uint8 lna2rout[] = {0, 1, 2, 3, 3, 3, 3};
    int8 tia_mix_gain[] = {10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 37, 37, 37};
    uint8 tia_mix_gainbits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 9, 9};
    uint8 tia_mix_gainlimits[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x7f};

    int8 dvga1_gain[] = {-12, -6, 0};
    uint8 dvga1_gainbits[] = {0, 1, 2};
    uint8 dvga1_gainlimits[] = {0, 0, 0};
    int8 dvga_gain[] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42};
    uint8 dvga_gainbits[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    uint8 gain_tbl_entry_size = sizeof(pi_ac->rxgcrsi->rxgainctrl_params[0].gaintbl[0][0]);
    uint8 gainbit_tbl_entry_size =
        sizeof(pi_ac->rxgcrsi->rxgainctrl_params[0].gainbitstbl[0][0]);
    uint16 gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT;

    if (core == 0)
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT0;
    else if (core == 1)
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT1;
    else if (core == 2)
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT2;
    else
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT3;

    /* ELNA */
    elna_gain[0] = elna_present ? elna : 0;
    elna_gain[1] = elna_present ? elna : 0;
    wlc_phy_set_agc_gaintbls_acphy(
        pi,
        gain_tblid, elna_gain,
        gainbits_tblid, elna_gainbits,
        gainlimit_tblid, elna_gainlimits, elna_gainlimits,
        ELNA_OFFSET,
        pi_ac->rxgcrsi->rxgainctrl_stage_len[ELNA_ID]);
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID],
           elna_gain,
           gain_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[ELNA_ID]);
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[ELNA_ID],
           elna_gainbits,
           gainbit_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[ELNA_ID]);

    /* LNA2 ROUT (there is no ROUT for lna2, but index is important) */
    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 7,
                              (ACPHY_LNAROUT_CORE_WRT_OFST(pi->pubpi.phy_rev,
                                                           core) + 16), 8, &lna2rout);
    /* LNA1 & LNA2 */
    wlc_phy_upd_lna1_lna2_gains_acphy(pi);

    /* TIA+MIX */
    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        phy_ac_rxgcrs_upd_mix_gains(pi_ac->rxgcrsi);
    } else {
        wlc_phy_set_agc_gaintbls_acphy(
            pi,
            gain_tblid, tia_mix_gain,
            gainbits_tblid, tia_mix_gainbits,
            gainlimit_tblid, tia_mix_gainlimits, tia_mix_gainlimits,
            TIA_OFFSET,
            pi_ac->rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
        memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID],
               tia_mix_gain,
               gain_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
        memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[TIA_ID],
               tia_mix_gainbits,
               gainbit_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[TIA_ID]);
    }
    /* BQ0 */
    memset(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID],
           0,
           gain_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[BQ0_ID]);
    memset(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[BQ0_ID],
           0,
           gainbit_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[BQ0_ID]);
    /* BQ1/DVGA1 */
    wlc_phy_set_agc_gaintbls_acphy(
        pi,
        gain_tblid, dvga1_gain,
        gainbits_tblid, dvga1_gainbits,
        gainlimit_tblid, dvga1_gainlimits, dvga1_gainlimits,
        BQ1_OFFSET,
        pi_ac->rxgcrsi->rxgainctrl_stage_len[BQ1_ID]);
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ1_ID],
           dvga1_gain,
           gain_tbl_entry_size *  pi_ac->rxgcrsi->rxgainctrl_stage_len[BQ1_ID]);
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[BQ1_ID],
           dvga1_gainbits,
           gainbit_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[BQ1_ID]);
    /* DVGA */
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[DVGA_ID],
           dvga_gain,
           gain_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[DVGA_ID]);
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[DVGA_ID],
           dvga_gainbits,
           gainbit_tbl_entry_size * pi_ac->rxgcrsi->rxgainctrl_stage_len[DVGA_ID]);

    wlc_phy_upd_lna1_bypass_acphy(pi, core);
}

void
wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(phy_info_t *pi,
    uint8 core, uint16 gain_tblid, uint16 gainbits_tblid)
{
    phy_ac_rxgcrs_info_t *ri = pi->u.pi_acphy->rxgcrsi;
    acphy_desense_values_t *desense = ri->total_desense;
    uint8 farrow_shift = 2;

    /* TIA */
    /* 2g & 5g settings */
    uint8 tia[]     = {10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43};
    uint8 tiabits[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11};
    int8  lna1_gain_g[] = { -6, 0, 6, 12, 18, 24};
    int8  lna1_gain_a[] = { -5, 0, 6, 12, 18, 24};
    int8  *lna1_gain = (CHSPEC_IS2G(pi->radio_chanspec) || !ACMAJORREV_4(pi->pubpi->phy_rev))
        ? lna1_gain_g : lna1_gain_a;
    uint8 lna1_gainbits[] = {0, 1, 2, 3, 4, 5};
    int8 elna_gain = ri->rxgainctrl_params[core].gaintbl[0][0];
    uint8 i;
    bool elna_present = CHSPEC_IS2G(pi->radio_chanspec) ? BF_ELNA_2G(ri->aci)
                                                        : BF_ELNA_5G(ri->aci);
    uint8 lna1_min_indx; /* default ilna parameters */
    uint8 lna1_max_indx;
    uint8 tia_max_indx;
    int8 lna1_max_gain = lna1_gain[5];
    /* eventually we may move into lna_present clause below */
    const uint8 unused = GAINLIMIT_MAX_OUT;
    const uint8 used = 0;
    uint16 gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT;

    if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
        gainlimit_tblid = (core) ? ACPHY_TBL_ID_GAINLIMIT1 : ACPHY_TBL_ID_GAINLIMIT0;
    } else if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
        if (core == 0)
            gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT0;
        else if (core == 1)
            gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT1;
        else if (core == 2)
            gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT2;
        else
            gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT3;
    } else {
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT;
    }

    wlc_phy_table_write_acphy(pi, gainbits_tblid, 1, DVGA1_OFFSET + 3,
        ACPHY_GAINBITS_TBL_WIDTH, &farrow_shift);

    if (desense->mixer_setting_desense > 0) {
            tia_max_indx = 11;
            tia_max_indx -= ri->curr_desense->mixer_setting_desense;

            /* limit tia */
            for (i = 0; i < 12; i++) {
                if (i >= (tia_max_indx+1)) {
                    tia[i]     = tia[tia_max_indx];
                    tiabits[i] = tia_max_indx;
            /* below, input value 1 is to write one table entry at a time */
                wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, TIA_OFFSET + i,
                    ACPHY_GAINLMT_TBL_WIDTH, &unused);
                } else {
            /* below, input value 1 is to write one table entry at a time */
                wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, TIA_OFFSET + i,
                    ACPHY_GAINLMT_TBL_WIDTH, &used);
                }
            }
        }

    wlc_phy_table_write_acphy(pi, gain_tblid, ri->rxgainctrl_stage_len[TIA_ID],
        TIA_OFFSET,    ACPHY_GAINDB_TBL_WIDTH, tia);
    wlc_phy_table_write_acphy(pi, gainbits_tblid, ri->rxgainctrl_stage_len[TIA_ID],
        TIA_OFFSET,    ACPHY_GAINBITS_TBL_WIDTH, tiabits);

    /* copying values into gaintbl arrays to avoid reading from table */
    memcpy(ri->rxgainctrl_params[core].gaintbl[TIA_ID], tia,
        sizeof(ri->rxgainctrl_params[core].gaintbl[0][0])
        * ri->rxgainctrl_stage_len[TIA_ID]);

    memcpy(ri->rxgainctrl_params[core].gainbitstbl[TIA_ID], tiabits,
        sizeof(ri->rxgainctrl_params[core].gainbitstbl[0][0])
        * ri->rxgainctrl_stage_len[TIA_ID]);

    /* backoff LNA1 for ELNA/iLNA(ACI) operation */
    if (elna_present) {
        if (ACPHY_LO_NF_MODE_ELNA_TINY(pi))
            lna1_min_indx = 5;
        else
            lna1_min_indx = 4;
        if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
            if (ri->curr_desense->lna1_tbl_desense < lna1_min_indx)
                lna1_min_indx -= ri->curr_desense->lna1_tbl_desense;
            else
                lna1_min_indx = 0;
        }

        lna1_max_indx = 5;

        /* find maximum gain */
        for (i = lna1_min_indx;
                    (i <= lna1_max_indx) &&
                    (elna_gain + lna1_gain[i] < lna1_max_gain);
                    i++)
            ;

        if ((desense->lna1_tbl_desense > 0) && (ACMAJORREV_4(pi->pubpi->phy_rev))) {
            lna1_max_indx = lna1_min_indx;
        } else {
            lna1_max_indx = (i > lna1_max_indx) ? lna1_max_indx : i;
        }
    } else {
        /* iLNA LNA1 configuration
         * eg. set lna1_max_lna=4 to reduce LNA1 gain by 6 dB.
         * The algorithm will allocate more gain to tia.
         * Further changes in gain distribution can be tuned by setting max_analog_gain.
         */
        lna1_min_indx = 0; /* unused */
        lna1_max_indx = 5;
    }

    /* modify encode gain to obey gainlimit */

    lna1_max_gain = lna1_gain[lna1_max_indx];

    /* limit LNA1 appropriately */
    for (i = 0; i < 6; i++) {
        if (i >= (lna1_max_indx + 1)) {
            lna1_gain[i]     = lna1_gain[lna1_max_indx];
            lna1_gainbits[i] = lna1_max_indx;
            /* below, input value 1 is to write one table entry at a time */
            wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, LNA1_OFFSET + i,
                ACPHY_GAINLMT_TBL_WIDTH, &unused);
            /* below, input value 1 is to write one table entry at a time */
            wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, (LNA1_OFFSET +
            GAINLMT_TBL_BAND_OFFSET + i), ACPHY_GAINLMT_TBL_WIDTH, &unused);
        } else {
            /* below, input value 1 is to write one table entry at a time */
            wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, LNA1_OFFSET + i,
                ACPHY_GAINLMT_TBL_WIDTH, &used);
            /* below, input value 1 is to write one table entry at a time */
            wlc_phy_table_write_acphy(pi, gainlimit_tblid, 1, (LNA1_OFFSET +
            GAINLMT_TBL_BAND_OFFSET + i), ACPHY_GAINLMT_TBL_WIDTH, &used);
        }
    }

    /* need this for encode gain to work correctly */
    wlc_phy_table_write_acphy(pi, gain_tblid, ri->rxgainctrl_stage_len[LNA1_ID],
        LNA1_OFFSET, ACPHY_GAINDB_TBL_WIDTH, lna1_gain);
    wlc_phy_table_write_acphy(pi, gainbits_tblid, ri->rxgainctrl_stage_len[LNA1_ID],
        LNA1_OFFSET, ACPHY_GAINBITS_TBL_WIDTH, lna1_gainbits);

    /* copying values into gaintbl arrays to avoid reading from hw */
    memcpy(ri->rxgainctrl_params[core].gaintbl[LNA1_ID], lna1_gain,
        sizeof(ri->rxgainctrl_params[core].gaintbl[0][0])
        * ri->rxgainctrl_stage_len[LNA1_ID]);

    memcpy(ri->rxgainctrl_params[core].gainbitstbl[LNA1_ID], lna1_gainbits,
        sizeof(ri->rxgainctrl_params[core].gainbitstbl[0][0])
        * ri->rxgainctrl_stage_len[LNA1_ID]);

    /* write max analog gain */
    if (ACPHY_ENABLE_FCBS_HWACI(pi)) {
        uint8 max_analog_gain;

        max_analog_gain = (MAX_ANALOG_RX_GAIN_TINY >=
            ri->curr_desense->analog_gain_desense_ofdm) ? (MAX_ANALOG_RX_GAIN_TINY -
            ri->curr_desense->analog_gain_desense_ofdm) : 0;

        MOD_PHYREGC(pi, HpFBw, core, maxAnalogGaindb, max_analog_gain);
        max_analog_gain = (MAX_ANALOG_RX_GAIN_TINY >=
            ri->curr_desense->analog_gain_desense_bphy) ? (MAX_ANALOG_RX_GAIN_TINY -
            ri->curr_desense->analog_gain_desense_bphy) : 0;
        MOD_PHYREGC(pi, DSSScckPktGain, core, dsss_cck_maxAnalogGaindb, max_analog_gain);
    } else {
        MOD_PHYREGC(pi, HpFBw, core, maxAnalogGaindb, MAX_ANALOG_RX_GAIN_TINY);
        MOD_PHYREGC(pi, DSSScckPktGain, core, dsss_cck_maxAnalogGaindb,
                    MAX_ANALOG_RX_GAIN_TINY);
    }
}

void
wlc_phy_rxgainctrl_set_gaintbls_acphy(phy_info_t *pi,
    bool init, bool band_change, bool bw_change)
{
    uint8 elna[2], ant;

    /* lna1 GainLimit */
    uint8 stall_val, core, i, sb;
    uint16 save_forclks, gain_tblid, gainbits_tblid;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    bool suspend;
    uint8 ch5g_sb[2] = {100, 132};

    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend) {
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
        phy_utils_phyreg_enter(pi);
    }
    /* Disable stall before writing tables */
    stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
    ACPHY_DISABLE_STALL(pi);

    if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        ch5g_sb[1] = 149;
    }

    /* If receiver is in active demod, it will NOT update the Gain tables */
    save_forclks = READ_PHYREG(pi, fineRxclockgatecontrol);
    MOD_PHYREG(pi, fineRxclockgatecontrol, forcegaingatedClksOn, 1);

    /* LNA1/2 (always do this, as the previous channel could have been in ACI mitigation) */
    wlc_phy_upd_lna1_lna2_gains_acphy(pi);

    FOREACH_CORE(pi, core) {
        if (core == 0) {
            gain_tblid =  ACPHY_TBL_ID_GAIN0;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS0;
        } else if (core == 1) {
            gain_tblid =  ACPHY_TBL_ID_GAIN1;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS1;
        } else if (core == 2) {
            gain_tblid =  ACPHY_TBL_ID_GAIN2;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS2;
        } else {
            gain_tblid =  ACPHY_TBL_ID_GAIN3;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS3;
        }

        /* FEM - elna, trloss (from srom) */
        ant = phy_get_rsdbbrd_corenum(pi, core);
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            elna[0] = elna[1] = pi_ac->sromi->femrx_2g[ant].elna;
            pi_ac->rxgcrsi->fem_rxgains[core].elna = elna[0];
            pi_ac->rxgcrsi->fem_rxgains[core].trloss =
                pi_ac->sromi->femrx_2g[ant].trloss;
            pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr =
                    pi_ac->sromi->femrx_2g[ant].elna_bypass_tr;
            pi_ac->rxgcrsi->fem_rxgains[core].lna1byp =
                ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
                & BFL2_LNA1BYPFORTR2G) != 0);
        } else if (CHSPEC_IS5G(pi->radio_chanspec)) {
            if (CHSPEC_CHANNEL(pi->radio_chanspec) < ch5g_sb[0]) {
                elna[0] = elna[1] = pi_ac->sromi->femrx_5g[ant].elna;
                pi_ac->rxgcrsi->fem_rxgains[core].elna = elna[0];
                pi_ac->rxgcrsi->fem_rxgains[core].trloss =
                    pi_ac->sromi->femrx_5g[ant].trloss;
                pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr =
                        pi_ac->sromi->femrx_5g[ant].elna_bypass_tr;
                pi_ac->rxgcrsi->fem_rxgains[core].lna1byp =
                    ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
                    & BFL2_LNA1BYPFORTR5G) != 0);
            } else if (CHSPEC_CHANNEL(pi->radio_chanspec) < ch5g_sb[1]) {
                elna[0] = elna[1] = pi_ac->sromi->femrx_5gm[ant].elna;
                pi_ac->rxgcrsi->fem_rxgains[core].elna = elna[0];
                pi_ac->rxgcrsi->fem_rxgains[core].trloss =
                    pi_ac->sromi->femrx_5gm[ant].trloss;
                pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr =
                        pi_ac->sromi->femrx_5gm[ant].elna_bypass_tr;
                pi_ac->rxgcrsi->fem_rxgains[core].lna1byp =
                    ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
                    & BFL2_LNA1BYPFORTR5G) != 0);
            } else {
                elna[0] = elna[1] = pi_ac->sromi->femrx_5gh[ant].elna;
                pi_ac->rxgcrsi->fem_rxgains[core].elna = elna[0];
                pi_ac->rxgcrsi->fem_rxgains[core].trloss =
                    pi_ac->sromi->femrx_5gh[ant].trloss;
                pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr =
                    pi_ac->sromi->femrx_5gh[ant].elna_bypass_tr;
                pi_ac->rxgcrsi->fem_rxgains[core].lna1byp =
                    ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
                    & BFL2_LNA1BYPFORTR5G) != 0);
            }
        } else if (CHSPEC_IS6G(pi->radio_chanspec)) {
            if ((ACMAJORREV_130(pi->pubpi->phy_rev)) && (RADIOMAJORREV(pi) >= 2)) {
                sb = (CHSPEC_CHANNEL(pi->radio_chanspec) < 101) ? 0:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 141) ? 1:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 161) ? 2:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 201) ? 3: 4;
            } else {
                sb = (CHSPEC_CHANNEL(pi->radio_chanspec) < 33) ? 0:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 65) ? 1:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 97) ? 2:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 129) ? 3:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 161) ? 4:
                     (CHSPEC_CHANNEL(pi->radio_chanspec) < 193) ? 5: 6;
            }
            elna[0] = elna[1] = pi_ac->sromi->femrx_6g[ant][sb].elna;
            pi_ac->rxgcrsi->fem_rxgains[core].elna = elna[0];
            pi_ac->rxgcrsi->fem_rxgains[core].trloss =
                pi_ac->sromi->femrx_6g[ant][sb].trloss;
            pi_ac->rxgcrsi->fem_rxgains[core].elna_bypass_tr =
                    pi_ac->sromi->femrx_6g[ant][sb].elna_bypass_tr;
            pi_ac->rxgcrsi->fem_rxgains[core].lna1byp =
                ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
                & BFL2_LNA1BYPFORTR5G) != 0);
        }

        if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
            /* At this point eLNA gain is known; calculate TIA gainlimit */
            pi_ac->rxgcrsi->rxg_params->gainlimit_tbl[TIA_TBL_IND][7] =
                phy_ac_calc_tia_gainlimit(pi_ac->rxgcrsi, core, 70);
        }

        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            uint8 num_rssi_cal_gi;
            num_rssi_cal_gi = CHSPEC_IS2G(pi->radio_chanspec)?
                pi_ac->sromi->num_rssi_cal_gi_2g:
                pi_ac->sromi->num_rssi_cal_gi_5g;
            if (num_rssi_cal_gi == 3) {
                /* Adjust gaindB table based on GI 3 RSSI CAL results */
                /* Apply adj only for 3 point calibation (GI 1/4/3) */
                wlc_phy_rxgain_adj_forrssi_acphy(pi, core, 3);
            }
        }

        /* RSSI elna on/off delta adjusted through trinT register */
        wlc_phy_set_trloss_reg_acphy(pi, core);
        wlc_phy_table_write_acphy(pi, gain_tblid, 2, 0, 8, elna);

        memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0],
               elna, sizeof(uint8)*pi_ac->rxgcrsi->rxgainctrl_stage_len[0]);

        /* MIX, LPF / TIA */
        if (TINY_RADIO(pi)) {
            /* always update - XXX FIXME:
             * current lna1/lna2 settings reset a lot of the state
             */
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                wlc_phy_rxgainctrl_set_gaintbls_acphy_wave2(
                    pi, core, gain_tblid, gainbits_tblid);
            } else {
                wlc_phy_rxgainctrl_set_gaintbls_acphy_tiny(
                    pi, core, gain_tblid, gainbits_tblid);
            }
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            if (init||band_change) {
                wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm(pi, core, gain_tblid,
                    gainbits_tblid); //TODO: Need to Check
            }
        } else if (init || band_change) {
            wlc_phy_rxgainctrl_set_gaintbls_acphy_2069(pi, core, gain_tblid,
                gainbits_tblid);
        }

        if (init) {
            /* Store gainctrl info (to be used for Auto-Gainctrl)
             * lna1,2 taken care in wlc_phy_upd_lna1_lna2_gaintbls_acphy()
             */
            wlc_phy_table_read_acphy(pi, gainbits_tblid, 1, 0, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[0]);
            wlc_phy_table_read_acphy(pi, gainbits_tblid, 10, 32, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[3]);
            wlc_phy_table_read_acphy(pi, gain_tblid, 8, 96, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[4]);
            wlc_phy_table_read_acphy(pi, gain_tblid, 8, 112, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[5]);
            wlc_phy_table_read_acphy(pi, gainbits_tblid, 8, 96, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[4]);
            wlc_phy_table_read_acphy(pi, gainbits_tblid, 8, 112, 8,
                pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[5]);

            if (TINY_RADIO(pi) || IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
                /* initialise DVGA table */
                for (i = 0; i < pi_ac->rxgcrsi->rxgainctrl_stage_len[6]; i++) {
                    pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[6][i] =
                        3 * i;
                    pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[6][i] =
                        i;
                }
            }
        }
    }

    /* Restore */
    WRITE_PHYREG(pi, fineRxclockgatecontrol, save_forclks);
    ACPHY_ENABLE_STALL(pi, stall_val);
    if (!suspend) {
        phy_utils_phyreg_exit(pi);
        wlapi_enable_mac(pi->sh->physhim);
    }
}

void
wlc_phy_set_trloss_reg_acphy(phy_info_t *pi, int8 core)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 boardloss = 2, trt, trr, ant, core_freq_segment_map;
    int8 rssi_tr_offset = 0;
    int8  bw_idx, subband_idx;
    uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
    uint16 intc, mask = ACPHY_RfctrlIntc0_override_tr_sw_MASK(rev) |
            ACPHY_RfctrlIntc0_tr_sw_tx_pu_MASK(rev) | ACPHY_RfctrlIntc0_tr_sw_rx_pu_MASK(rev);
    bool suspend = FALSE;
    bool rssi_cal_rev = phy_ac_rssi_get_data(pi_ac->rssii)->rssi_cal_rev;
    wlc_phy_conditional_suspend(pi, &suspend);

    bw_idx = (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec)) ? 2 :
            (CHSPEC_IS160(pi->radio_chanspec)) ? 3 :
            (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
    core_freq_segment_map = phy_ac_chanmgr_get_data(pi_ac->chanmgri)->core_freq_mapping[core];
    ant = phy_get_rsdbbrd_corenum(pi, core);

    if (rssi_cal_rev == FALSE) {
        if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
            phy_ac_chanmgr_get_chan_freq_range_80p80(pi, pi->radio_chanspec, bands);
            subband_idx = (core <= 1) ? (bands[0] - 1) : (bands[1] - 1);
        } else {
            subband_idx = phy_ac_chanmgr_get_chan_freq_range(pi,
              pi->radio_chanspec, core_freq_segment_map)-1;
        }
    } else {
        subband_idx = wlc_phy_rssi_get_chan_freq_range_acphy(pi, core_freq_segment_map,
            core);
    }
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
      if (rssi_cal_rev == FALSE) {
        rssi_tr_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
            [ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
            [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx];
      } else {
        rssi_tr_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      }
    } else {
      if (rssi_cal_rev == FALSE) {
        rssi_tr_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
            [ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
            [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      } else {
        rssi_tr_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      }
    }
    if (rssi_cal_rev == TRUE) {
      /* With new scheme, rssi gain delta's are in qdB steps */
      rssi_tr_offset = (rssi_tr_offset + 2) >> 2;
    }

    /* adjust TRLoss with rssi cal param */
    trr = boardloss;
    trt = pi_ac->rxgcrsi->fem_rxgains[core].trloss + rssi_tr_offset;

    /* Not sure why all 4335 boards don't need boardloss = 2 (chip default) */
    if (ACMAJORREV_0(pi->pubpi->phy_rev) ||
        ACMAJORREV_2(pi->pubpi->phy_rev) ||
        ACMAJORREV_5(pi->pubpi->phy_rev) ||
        ACMAJORREV_128(pi->pubpi->phy_rev))
        trt += boardloss;

    /* Clear up RfctrlIntc override (as uCode might have set it for hirssi_elnabypass */
    if (PHY_SW_HIRSSI_UCODE_CAP(pi)) {
        intc = phy_utils_read_phyreg(pi, ACPHYREGCE(pi, RfctrlIntc, core)) & (~mask);
        phy_utils_write_phyreg(pi, ACPHYREGCE(pi, RfctrlIntc, core), intc);
    }
    switch (core) {
    case 0 :
        if (ACREV_IS(pi->pubpi->phy_rev, 0)) {
            /* adjust TRLoss with rssi cal param */
            MOD_PHYREG(pi, TRLossValue, freqGainTLoss, trt);
            MOD_PHYREG(pi, TRLossValue, freqGainRLoss, trr);
        } else {
            MOD_PHYREG(pi, Core0_TRLossValue, freqGainTLoss0, trt);
            MOD_PHYREG(pi, Core0_TRLossValue, freqGainRLoss0, trr);
            /* for 4347, the hwrssi uses TRloss from the common reg */
            if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
                MOD_PHYREG(pi, TRLossValue, freqGainTLoss, trt);
                MOD_PHYREG(pi, TRLossValue, freqGainRLoss, trr);
            }
        }
        break;
    case 1:
        if (PHYCORENUM(pi->pubpi->phy_corenum) > 1 &&
            ACREV_GT(pi->pubpi->phy_rev, 0)) {
            /* adjust TRLoss with rssi cal param */
            MOD_PHYREG(pi, Core1_TRLossValue, freqGainTLoss1, trt);
            MOD_PHYREG(pi, Core1_TRLossValue, freqGainRLoss1, trr);
        }
        break;
    case 2:
        if (PHYCORENUM(pi->pubpi->phy_corenum) > 2 &&
            ACREV_GT(pi->pubpi->phy_rev, 0)) {
            /* adjust TRLoss with rssi cal param */
            MOD_PHYREG(pi, Core2_TRLossValue, freqGainTLoss2, trt);
            MOD_PHYREG(pi, Core2_TRLossValue, freqGainRLoss2, trr);
        }
        break;
    case 3:
        if (PHYCORENUM(pi->pubpi->phy_corenum) > 3 &&
            ACREV_GT(pi->pubpi->phy_rev, 0)) {
            /* adjust TRLoss with rssi cal param */
            MOD_PHYREG(pi, Core3_TRLossValue, freqGainTLoss3, trt);
            MOD_PHYREG(pi, Core3_TRLossValue, freqGainRLoss3, trr);
        }
        break;
    default:
        break;
    }
    wlc_phy_conditional_resume(pi, &suspend);
}

void
wlc_phy_set_lna1byp_reg_acphy(phy_info_t *pi, int8 core)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    int8 lna1_gain_offset = 0, core_freq_segment_map, ant;
    int8  bw_idx, subband_idx;
    bool suspend = FALSE;
    uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
    bool rssi_cal_rev = phy_ac_rssi_get_data(pi_ac->rssii)->rssi_cal_rev;
    wlc_phy_conditional_suspend(pi, &suspend);

    bw_idx = (CHSPEC_IS80(pi->radio_chanspec)) ? 2 :
            (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
    core_freq_segment_map = phy_ac_chanmgr_get_data(pi_ac->chanmgri)->core_freq_mapping[core];
    ant = phy_get_rsdbbrd_corenum(pi, core);

    if (rssi_cal_rev == FALSE) {
        if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
            phy_ac_chanmgr_get_chan_freq_range_80p80(pi, pi->radio_chanspec, bands);
            subband_idx = (core <= 1) ? (bands[0] - 1) : (bands[1] - 1);
        } else {
            subband_idx = phy_ac_chanmgr_get_chan_freq_range(pi,
              pi->radio_chanspec, core_freq_segment_map)-1;
        }
    } else {
        subband_idx = wlc_phy_rssi_get_chan_freq_range_acphy(pi, core_freq_segment_map,
            core);
    }
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
      if (rssi_cal_rev == FALSE) {
        lna1_gain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
            [ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g[ant]
            [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx];
      } else {
        lna1_gain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      }
    } else {
      if (rssi_cal_rev == FALSE) {
        lna1_gain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
            [ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g[ant]
            [ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      } else {
        lna1_gain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_OFF][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
      }
    }

    if (rssi_cal_rev == TRUE) {
      /* With new scheme, rssi gain delta's are in qdB steps */
      lna1_gain_offset = (lna1_gain_offset + 2) >> 2;
    }

    wlc_phy_conditional_resume(pi, &suspend);
}

void
wlc_phy_rxgain_adj_forrssi_acphy(phy_info_t *pi, int8 core, uint8 force_gain_type)
{
    /* REV40 force_gain_type 3 lna1 gain step offset adjustment */
    /* Assumes force_gain_type 3 uses lna1 of 3 */
    /* Only support rssi_cal_rev == TRUE */

    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 ant, core_freq_segment_map, midgain_index, lna1_index, sz, x, gainbits;
    int8  bw_idx, subband_idx, gain_lna1, rssi_midgain_offset;
    uint16 gain_tblid, gainbits_tblid;
    uint8 rssi_rev = phy_ac_rssi_get_data(pi_ac->rssii)->rssi_cal_rev;

    if (force_gain_type == 3 && (rssi_rev == TRUE)) {
        midgain_index = ACPHY_GAIN_DELTA_GainIndex_3;
        /* lna1 index 3 used for force_gain_type 3 to calibrate lna1 gain step delta */
        lna1_index = 3;
    } else {
        /* Fix me. Only adjust for force_gain_type 3 for now */
        /* Add more type as needed */
        return;
    }

    bw_idx = (CHSPEC_IS80(pi->radio_chanspec)) ? 2 :
            (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
    core_freq_segment_map = phy_ac_chanmgr_get_data(pi_ac->chanmgri)->core_freq_mapping[core];
    ant = phy_get_rsdbbrd_corenum(pi, core);

    subband_idx = wlc_phy_rssi_get_chan_freq_range_acphy(pi,
                                                         core_freq_segment_map, core);
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        rssi_midgain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][midgain_index][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_2g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
    } else {
        rssi_midgain_offset =
          pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][midgain_index][bw_idx][subband_idx]-
            pi_ac->sromi->rssioffset.rssi_corr_gain_delta_5g_sub
            [ant][ACPHY_GAIN_DELTA_ELNA_ON][bw_idx][subband_idx];
    }

    rssi_midgain_offset = (rssi_midgain_offset + 2) >> 2;
    /* Limit the range of adjustment */
    rssi_midgain_offset = MAX(MIN(rssi_midgain_offset, ACPHY_MIDGAIN_OFFSET_LIMIT),
                              -ACPHY_MIDGAIN_OFFSET_LIMIT);
    if (rssi_midgain_offset) {
        if (core == 0) {
            gain_tblid =  ACPHY_TBL_ID_GAIN0;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS0;
        } else if (core == 1) {
            gain_tblid =  ACPHY_TBL_ID_GAIN1;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS1;
        }  else if (core == 2) {
            gain_tblid =  ACPHY_TBL_ID_GAIN2;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS2;
        } else {
            gain_tblid =  ACPHY_TBL_ID_GAIN3;
            gainbits_tblid =  ACPHY_TBL_ID_GAINBITS3;
        }

        sz = pi->u.pi_acphy->rxgcrsi->rxgainctrl_stage_len[1];
        for (x = 0; x < sz; x++) {
            wlc_phy_table_read_acphy(pi, gainbits_tblid, 1, (0x8 + x), 8,
                                     &gainbits);
            if (gainbits <= lna1_index) {
                wlc_phy_table_read_acphy(pi, gain_tblid, 1, (0x8 + x), 8,
                                         &gain_lna1);
                /* Update gaindB for lna1 index 3 and below */
                gain_lna1 -= rssi_midgain_offset;
                wlc_phy_table_write_acphy(pi, gain_tblid, 1, (0x8 + x), 8,
                                          &gain_lna1);
            }
        }
    }
}

void
wlc_phy_set_agc_gaintbls_acphy(
    phy_info_t *pi,
    uint32 gain_tblid,
    const void *gain,
    uint32 gainbits_tblid,
    const void *gainbits,
    uint32 gainlimits_tblid,
    const void *gainlimits_ofdm,
    const void *gainlimits_cck,
    uint32 offset,
    uint32 len)
{
    wlc_phy_table_write_acphy(
        pi, gain_tblid, len, offset,
        ACPHY_GAINDB_TBL_WIDTH, gain);
    wlc_phy_table_write_acphy(
        pi, gainbits_tblid, len, offset,
        ACPHY_GAINBITS_TBL_WIDTH, gainbits);
    wlc_phy_table_write_acphy(
        pi, gainlimits_tblid, len, offset,
        ACPHY_GAINLMT_TBL_WIDTH, gainlimits_ofdm);
    wlc_phy_table_write_acphy(
        pi, gainlimits_tblid, len, offset + GAINLMT_TBL_BAND_OFFSET,
        ACPHY_GAINLMT_TBL_WIDTH, gainlimits_cck);
}

void
phy_ac_get_fem_rxgains(phy_info_t *pi, int8 *rx_gains)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;

    rx_gains[0] = rxgcrsi->fem_rxgains[0].elna;
    rx_gains[1] = rxgcrsi->fem_rxgains[0].trloss;
    rx_gains[2] = rxgcrsi->fem_rxgains[0].elna_bypass_tr;
}

void phy_ac_get_rxgains_ctrl(phy_info_t *pi, int8 *rx_gains, int8 *input_param)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = pi->u.pi_acphy->rxgcrsi;

    rx_gains[0]  = rxgcrsi->rxgainctrl_params[0].gaintbl[1][input_param[0]];
    rx_gains[1]  = rxgcrsi->rxgainctrl_params[0].gaintbl[2][input_param[1]];
    rx_gains[2] = rxgcrsi->rxgainctrl_params[0].gaintbl[3][input_param[2]];
    rx_gains[3] = rxgcrsi->rxgainctrl_params[0].gaintbl[4][input_param[3]];
    rx_gains[4] = rxgcrsi->rxgainctrl_params[0].gaintbl[5][input_param[4]];
}

/* Additional settings to use LNA1 bypass instead of T/R */
static void
phy_ac_upd_lna1_bypass(phy_info_t *pi, uint8 core)
{
    uint8 rout = 0;
    int8  gain = 0;
    uint8 idx = 6;    /* Rout table index to be used when in bypass */
    uint16 byp_val;
    uint8 rout_val;
    uint32 rout_offset;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        /* 43012 bypass settings */
        gain = rxg_params->lna1_byp_gain;
        rout = rxg_params->lna1_byp_rout;
        idx = rxg_params->lna1_byp_indx;
    }

    /* Make sure the AGC knows the gain when LNA1 is in bypass */
    byp_val = (0xff & gain) << ACPHY_Core0_lna1BypVals_lna1BypGain0_SHIFT(pi->pubpi->phy_rev);

    /* Gain/Rout table index to be used when in bypass */
    byp_val |= idx << ACPHY_Core0_lna1BypVals_lna1BypIndex0_SHIFT(pi->pubpi->phy_rev);

    /* Have the AGC use the gain set in lna1BypGain */
    byp_val |= 1 << ACPHY_Core0_lna1BypVals_lna1BypEn0_SHIFT(pi->pubpi->phy_rev);

    WRITE_PHYREGC(pi, _lna1BypVals, core, byp_val);

    /* Set the Rout to be used for index 6 */
    rout_val = (rout << 3) + idx;
    if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
        rout_offset = (core * 24) + idx + (CHSPEC_IS2G(pi->radio_chanspec) ? 0 : 8);
    } else {
        rout_offset = (core * 24) + idx + (CHSPEC_IS2G(pi->radio_chanspec) ? 0 : 16);
    }
    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, rout_offset, 8, &rout_val);

    /* Power down LNA1 when bypassed */
    MOD_PHYREG(pi, AfePuCtrl, lna1_pd_during_byp, 1);

    /* Only for chips with a dedicated BT-LNA, no shared LNA */
    ASSERT(READ_PHYREGFLD(pi, SlnaControl, SlnaEn) == 0);

    if (!ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
        /* WAR for LNA1 bypass sticky on Core 0, CRDOT11ACPHY-1161 */
        MOD_PHYREG(pi, SlnaControl, SlnaCore, 3);
    }
}

static void
phy_ac_single_shot_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change)
{
    uint8 core;
    uint8 gain_idx[ACPHY_MAX_RX_GAIN_STAGES];
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
    int8 init_gain = rxg_params->fast_agc_clip_gains[0];

    if (init == TRUE) {
        /* SSAGC control config */
        phy_ac_ssagc_control_config(pi);
    }

    if ((init == TRUE) || (band_change == TRUE) || (bw_change == TRUE)) {
        FOREACH_CORE(pi, core) {
            phy_ac_ssagc_rssi_setup(pi, core, init, band_change, bw_change);
            phy_ac_ssagc_rxgainctrl(pi, core);

            /* Configure init gain */
            wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, init_gain, 0, 0,
                    INIT_GAIN, gain_idx);
            wlc_phy_set_analog_rxgain(pi, INIT_GAIN, gain_idx, 0, core);
        }
    }
}

static void
phy_ac_ssagc_rxgainctrl(phy_info_t *pi, uint8 core)
{
    uint8 max_analog_gain, i;
    bool trtx, trrx, lna1byp_en, lna1byp;
    uint32 rssi_clip_gains[SSAGC_CLIP1_GAINS_CNT];
    uint8 gain_idx[ACPHY_MAX_RX_GAIN_STAGES];
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;
    bool elna_present = (CHSPEC_IS2G(pi->radio_chanspec)) ? BF_ELNA_2G(pi_ac)
                                                          : BF_ELNA_5G(pi_ac);

    /* Max analog gain configuration */
    max_analog_gain = rxg_params->max_analog_gain;

    /* fill in gain limits for analog stages */
    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++) {
        if (rxg_params->maxgain[i] == 0) {
            pi_ac->rxgcrsi->rxgainctrl_maxout_gains[i] = max_analog_gain;
        } else {
            pi_ac->rxgcrsi->rxgainctrl_maxout_gains[i] = rxg_params->maxgain[i];
        }
    }

    lna1byp_en = CHSPEC_IS2G(pi->radio_chanspec) ?
            ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
            & BFL2_LNA1BYPFORTR2G) != 0) :
            ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
            & BFL2_LNA1BYPFORTR5G) != 0);
    pi_ac->rxgcrsi->fem_rxgains[core].lna1byp = lna1byp_en;

    if (lna1byp_en == TRUE) {
        phy_ac_upd_lna1_bypass(pi, core);
    }

    /* Auto distribute gain */
    for (i = 0; i < SSAGC_CLIP1_GAINS_CNT; i++) {
        lna1byp = FALSE; trtx = FALSE; trrx = TRUE;

        if (rxg_params->ssagc_clip_gains[i] < rxg_params->ssagc_lna_byp_gain_thresh) {
            if (elna_present) {
                lna1byp = FALSE; trtx = TRUE; trrx = FALSE;
            } else if (lna1byp_en) {
                lna1byp = TRUE; trtx = FALSE; trrx = TRUE;
            }
        }

        wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, rxg_params->ssagc_clip_gains[i],
                trtx, lna1byp, SSAGC_CLIP_GAIN, gain_idx);
        rssi_clip_gains[i] = phy_ac_ssgac_pack_gains(pi, gain_idx, trtx, lna1byp, trrx);
    }

    /* Write clip gain table */
    phy_ac_populate_ssagc_clipgain_tables(pi, rssi_clip_gains, core);
}

static void
phy_ac_populate_ssagc_clipgain_tables(phy_info_t *pi, uint32 *rssi_clip_gains, uint8 core)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    /* ClipGain table */
    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RSSICLIPGAIN0, SSAGC_CLIP1_GAINS_CNT, 0, 32,
            rssi_clip_gains);

    /* Clip2Idx table */
    wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RSSICLIP2GAIN0,
            SSAGC_CLIP2_TBL_IDX_CNT, 0, 8, rxg_params->ssagc_clip2_tbl);
}

static uint32
phy_ac_ssgac_pack_gains(phy_info_t *pi, uint8 *gain_idx, bool trtx, bool lna1byp, bool trrx)
{
    uint8 elna = gain_idx[0];
    uint8 lna1 = gain_idx[1];
    uint8 lna2 = gain_idx[2];
    uint8 tia = gain_idx[3];
    uint8 bq0 = gain_idx[4];
    uint8 bq1 = gain_idx[5];
    uint8 dvga = gain_idx[6];

    uint32 pack_gain_idx = (lna1byp << 23) | (trrx << 22) | (dvga << 18) | (bq1 << 15) |
            (bq0 << 12) | (tia << 8) | (lna2 << 5) | (lna1 << 2) | (elna << 1) | trtx;
    return pack_gain_idx;
}

static void
phy_ac_ssagc_control_config(phy_info_t *pi)
{
}

static void
phy_ac_ssagc_rssi_setup(phy_info_t *pi, uint8 core, bool init, bool band_change, bool bw_change)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    if (init == TRUE) {
        phy_ac_set_ssagc_RSSI_detectors(pi, core,
                                        rxg_params->ssagc_rssi_thresh);
    }

    if (bw_change == TRUE) {
        /* Configure hi and low digital thresholds */
        phy_ac_set_ssagc_clipcnt_thresh(pi, core,
                                        rxg_params->ssagc_clipcnt_thresh);
    }
}

static void
phy_ac_set_ssagc_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    /* Configure W1 threshold */
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        MOD_RADIO_REG_28NM(pi, RF, LNA2G_REG3, core, rx2g_wrssi1_threshold, w1);
    } else {
        MOD_RADIO_REG_28NM(pi, RF, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1);
    }

    /* Configure W3 thresholds */
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold2, w3_hi);
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold1, w3_mid);
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold0, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold2, nb_hi);
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold1, nb_mid);
    MOD_RADIO_REG_28NM(pi, RF, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold0, nb_low);
}

static void
phy_ac_set_20698_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID));

    /* Configure W1 threshold */
    if (CHSPEC_IS2G(pi->radio_chanspec)) {
        MOD_RADIO_REG_20698(pi, LNA2G_REG3, core, lna2g_dig_wrssi1_threshold, w1);
        if (RADIOREV(pi->pubpi->radiorev) > 1)
            MOD_RADIO_REG_20698(pi, RX2G_REG1, core, rx2g_spare, w1);
    } else {
        MOD_RADIO_REG_20698(pi, RX5G_WRSSI1, core, rx5g_wrssi_threshold, w1);
        if (RADIOREV(pi->pubpi->radiorev) > 1)
            MOD_RADIO_REG_20698(pi, RX5G_REG6, core, rx5g_spare, w1);
    }

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20698(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

static void
phy_ac_set_20704_RSSI_detectors(phy_info_t *pi, uint8 core, const uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID));

    /* Configure W1 threshold */
    MOD_RADIO_REG_20704(pi, RX5G_WRSSI1, core, rxdb_wrssi1_threshold, w1);
    MOD_RADIO_REG_20704(pi, RX5G_WRSSI1, core, rxdb_wrssi1_vbias, w1);

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20704(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

static void
phy_ac_set_20707_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID));

    /* Configure W1 threshold */
    MOD_RADIO_REG_20707(pi, RX5G_WRSSI1, core, rxdb_wrssi1_threshold, w1);
    MOD_RADIO_REG_20707(pi, RX5G_WRSSI1, core, rxdb_wrssi1_vbias, w1);

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20707(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

static void
phy_ac_set_20708_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    uint8 w1_highest = thresholds[0];
    uint8 w1_higher = thresholds[1];
    uint8 w1_high = thresholds[2];
    uint8 w3_low = thresholds[3];
    uint8 w3_mid = thresholds[4];
    uint8 w3_hi = thresholds[5];
    uint8 nb_low = thresholds[6];
    uint8 nb_mid = thresholds[7];
    uint8 nb_hi = thresholds[8];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID));

    /* Configure W1 threshold */
    MOD_RADIO_REG_20708(pi, RX5G_WRSSI1, core, rxdb_wrssi1_threshold_highest, w1_highest);
    MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core, rxdb_wrssi1_threshold_higher, w1_higher);
    if (RADIOMAJORREV(pi) < 2) {
        MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
                rxdb_wrssi1_threshold_high, w1_high);
    } else {
        MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
                rxdb_wrssi1_threshold_mid, w1_high);
    }

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20708(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

/* Same functionality as phy_ac_set_ssagc_RSSI_detectors above but for 6878 */
static void
phy_ac_set_20709_RSSI_detectors(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID));

    /* Configure W1 threshold */
    MOD_RADIO_REG_20709(pi, RX5G_WRSSI1, core, rxdb_wrssi1_threshold, w1);
    MOD_RADIO_REG_20709(pi, RX5G_WRSSI1, core, rxdb_wrssi1_vbias, w1);

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20709(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

static void
phy_ac_set_20710_RSSI_detectors(phy_info_t *pi, uint8 core, const uint8 *thresholds)
{
    uint8 w1 = thresholds[0];
    uint8 w3_low = thresholds[1];
    uint8 w3_mid = thresholds[2];
    uint8 w3_hi = thresholds[3];
    uint8 nb_low = thresholds[4];
    uint8 nb_mid = thresholds[5];
    uint8 nb_hi = thresholds[6];

    ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID));

    /* Configure W1 threshold */
    MOD_RADIO_REG_20710(pi, RX5G_WRSSI1, core, rxdb_wrssi1_threshold, w1);
    MOD_RADIO_REG_20710(pi, RX5G_WRSSI1, core, rxdb_wrssi1_vbias, w1);

    /* Configure W3 thresholds */
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_high, w3_hi);
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_mid, w3_mid);
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG1, core, tia_nbrssi1_threshold_low, w3_low);

    /* Configure NB thresholds */
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_high, nb_hi);
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_mid, nb_mid);
    MOD_RADIO_REG_20710(pi, TIA_NBRSSI_REG2, core, tia_nbrssi2_threshold_low, nb_low);
}

static void
phy_ac_set_ssagc_clipcnt_thresh(phy_info_t *pi, uint8 core, uint8 *thresholds)
{
    MOD_PHYREGC(pi, SsAgcW1ClipCntTh, core, ssAgcW1ClipCntThLo, thresholds[0]);
    MOD_PHYREGC(pi, SsAgcW1ClipCntTh, core, ssAgcW1ClipCntThHi, thresholds[1]);
    MOD_PHYREGC(pi, SsAgcW2ClipCntTh, core, ssAgcW2ClipCntThLo, thresholds[2]);
    MOD_PHYREGC(pi, SsAgcW2ClipCntTh, core, ssAgcW2ClipCntThHi, thresholds[3]);
    MOD_PHYREGC(pi, SsAgcNbClipCntTh1, core, ssAgcNbClipCntThLo, thresholds[4]);
    MOD_PHYREGC(pi, SsAgcNbClipCntTh1, core, ssAgcNbClipCntThHi, thresholds[5]);
}

static void
phy_ac_fastagc_config(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    /* Disable SSAGC flag */
    rxg_params->ssagc_en = FALSE;

    if (!(ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev))) {
        ACPHY_REG_LIST_START
            MOD_PHYREG_ENTRY(pi, defer_setClip1_CtrLen, defer_setclip1gain_len, 20)
            MOD_PHYREG_ENTRY(pi, defer_setClip2_CtrLen, defer_setclip2gain_len, 16)
        ACPHY_REG_LIST_EXECUTE(pi);
    }

    /* Configure registers related to Fast AGC */
    ACPHY_REG_LIST_START
        MOD_PHYREG_ENTRY(pi, defer_carrier_search_ctr,
                defer_carrier_search_ctr, 112)

        /* Disable pre-clip */
        MOD_PHYREG_ENTRY(pi, ADC_PreClip_Enable, adc_preclip1_enable, 0)
        MOD_PHYREG_ENTRY(pi, ADC_PreClip_Enable, adc_preclip2_enable, 0)

        /* Disable SSAGC */
        MOD_PHYREG_ENTRY(pi, singleShotAgcCtrl, singleShotAgcEn, 0)
    ACPHY_REG_LIST_EXECUTE(pi);
}

static void
phy_ac_ssagc_config(phy_info_t *pi, bool init, bool band_change, bool bw_change)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    /* Enable SSAGC flag */
    rxg_params->ssagc_en = TRUE;

    phy_ac_single_shot_agc(pi, init, band_change, bw_change);
}

void
phy_ac_agc_config(phy_info_t *pi, uint8 agc_type)
{
    if (agc_type == FAST_AGC) {
        phy_ac_fastagc_config(pi);
    } else if (agc_type == SINGLE_SHOT_AGC) {
        phy_ac_ssagc_config(pi, TRUE, TRUE, TRUE);
    } else {
        PHY_ERROR(("%s: Invalid AGC type %d failed\n", __FUNCTION__, agc_type));
        ASSERT(0);
    }
}

void
chanspec_setup_rxgcrs(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#ifndef WLC_DISABLE_ACI
    uint16 orig_reg_vals;
    uint8 stall_val = 0;
    aci_reg_list_entry *reg_list_ptr;
    aci_tbl_list_entry_at_init *tbl_list_ptr;
    uint8 p_phytbl7_8_buf[119];
    uint32 p_phytbl24_28_buf[22];
    uint16 rfseq_aci_a_c0, rfseq_aci_a_c1, rfseq_aci_b_c0, rfseq_aci_b_c1;
#endif
    int8 desense_state;
    int8 desense_state_init;

    /* JIRA(CRDOT11ACPHY-143) - Turn off receiver during channel change */
    pi_ac->rxgcrsi->deaf_count = 0;
    phy_ac_rxgcrs_stay_in_carriersearch(pi_ac->rxgcrsi, TRUE);

    bzero((uint8 *)pi_ac->rxgcrsi->phy_noise_in_crs_min,
          sizeof(pi_ac->rxgcrsi->phy_noise_in_crs_min));
    bzero((uint8 *)pi_ac->rxgcrsi->phy_noise_pwr_array,
          sizeof(pi_ac->rxgcrsi->phy_noise_pwr_array));
    bzero((int8 *)pi_ac->rxgcrsi->nbclip_dBm,
          sizeof(pi_ac->rxgcrsi->nbclip_dBm));
    bzero((int8 *)pi_ac->rxgcrsi->w1clip_dBm,
          sizeof(pi_ac->rxgcrsi->w1clip_dBm));

    /* Debug parameters: printed by 'wl dump phycal' */
    pi_ac->rxgcrsi->phy_debug_crscal_counter = 0;
    pi_ac->rxgcrsi->phy_noise_counter = 0;

    /* Consider CRS bit always in CRS cal except forced */
    pi->cal_info->ignore_crs_status = FALSE;

    /* phy forcecal request not present */
    pi->cal_info->phy_forcecal_request = FALSE;

    /* force the knoise low gain = high gain or not */
    pi->cal_info->crsmincal_knoise_lowgain_ovr = FALSE;

    /* 6G has multiple subbands */
    if (CCT_BAND_CHG(pi_ac) || CHSPEC_IS6G(pi->radio_chanspec)) {
        phy_ac_rxgcrs_populate_rxg_params_band_specific(pi_ac->rxgcrsi);
    }

    phy_ac_rxgcrs_populate_rxg_params_bw_specific(pi);

    /* set the crsmin_th from cache at chan_change */
    (void) phy_ac_rxgcrs_min_pwr_cal(pi->u.pi_acphy->rxgcrsi, PHY_CRS_SET_FROM_CACHE);

    if (ACPHY_HWACI_HWTBL_MITIGATION(pi) || ACPHY_MCLIP_ACI_MITIGATION(pi)) {
        desense_state_init = 1;
    } else {
        desense_state_init = 0;
    }

    if (ISSIM_ENAB(pi->sh->sih) && !ACMAJORREV_130(pi->pubpi->phy_rev))
        return;

#ifndef WLC_DISABLE_ACI
    /* Merge ACI & BT params into one */
    if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
        wlc_phy_desense_calc_total_acphy(pi_ac->rxgcrsi);
    }
#endif /* !WLC_DISABLE_ACI */

    wlc_phy_rxgainctrl_set_gaintbls_acphy(pi,
        CCT_INIT(pi_ac), CCT_BAND_CHG(pi_ac), CCT_BW_CHG(pi_ac));

#ifndef WLC_DISABLE_ACI
    /*
     * XXX FIXME If hwaci fcbs enabled for 5G and sub band nvram
     * used need to re-init on each channel
     */
    if (ACPHY_ENABLE_FCBS_HWACI(pi) && (CCT_INIT(pi_ac) || CCT_BAND_CHG(pi_ac))) {
        if ((ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
            !ACMAJORREV_128(pi->pubpi->phy_rev)) &&
            (pi->sh->interference_mode & ACPHY_ACI_HWACI_PKTGAINLMT))
            wlc_phy_hwaci_mitigation_enable_acphy(pi, HWACI_AUTO_SW, TRUE);
        else
            wlc_phy_hwaci_mitigation_enable_acphy(pi,
            ((pi->sh->interference_mode & ACPHY_HWACI_MITIGATION) > 0)
#ifdef WL_ACI_FCBS
            ? HWACI_AUTO_FCBS : 0,
#else
            ? HWACI_AUTO_SW : 0,
#endif
            TRUE);
    }
#endif /* WLC_DISABLE_ACI */
    for (desense_state = desense_state_init; desense_state >= 0; desense_state--) {
        if ((ACPHY_HWACI_HWTBL_MITIGATION(pi)))  {
            phy_ac_noise_hwaci_mitigation(pi->u.pi_acphy->noisei, desense_state);
        }

        if (TINY_RADIO(pi)) {
            wlc_phy_rxgainctrl_gainctrl_acphy_tiny(pi, 0);
        } else if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            wlc_phy_agc_28nm(pi, CCT_INIT(pi_ac),
                CCT_BAND_CHG(pi_ac), CCT_BW_CHG(pi_ac)); //TODO:Need to Check
        } else {
            /* Set INIT, Clip gains, clip thresh (srom based) */
            wlc_phy_rxgainctrl_gainctrl_acphy(pi);
        }

#ifndef WLC_DISABLE_ACI
        if (!ACPHY_ENABLE_FCBS_HWACI(pi) || ACPHY_HWACI_WITH_DESENSE_ENG(pi)) {
            /*
             * Desense on top of default gainctrl, if desense on
             * (otherwise restore defaults)
             */
            wlc_phy_desense_apply_acphy(pi, pi_ac->rxgcrsi->total_desense->on);
        }
        if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
            ACPHY_MCLIP_ACI_MITIGATION(pi)) {
            stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
            wlapi_suspend_mac_and_wait(pi->sh->physhim);
            ACPHY_DISABLE_STALL(pi);

            if ((ACPHY_HWACI_HWTBL_MITIGATION(pi) ||
                ACPHY_MCLIP_ACI_MITIGATION(pi)) && desense_state == 1) {
                for (reg_list_ptr =
                    phy_ac_noise_get_data(pi_ac->noisei)->hwaci_phyreg_list;
                    reg_list_ptr->regaddr != 0xFFFF; reg_list_ptr++) {
                    orig_reg_vals =
                        phy_utils_read_phyreg(pi, reg_list_ptr->regaddr);
                    phy_utils_write_phyreg(pi, reg_list_ptr->regaddraci,
                        orig_reg_vals);
                }

                for (tbl_list_ptr =
                    phy_ac_noise_get_data(pi_ac->noisei)->hwaci_phytbl_list;
                    tbl_list_ptr->tblidaci != 0xFFFF; tbl_list_ptr++) {
                    if (CCT_INIT(pi_ac) || tbl_list_ptr->post_init) {
                        switch (tbl_list_ptr->tbl_width) {
                            case 7:
                            case 8:
                                wlc_phy_table_read_acphy(pi,
                                    tbl_list_ptr->tblid,
                                    tbl_list_ptr->tbl_len,
                                    tbl_list_ptr->start_offset,
                                    8, p_phytbl7_8_buf);
                                wlc_phy_table_write_acphy(pi,
                                    tbl_list_ptr->tblidaci,
                                    tbl_list_ptr->tbl_len,
                                    tbl_list_ptr->start_offset,
                                    8, p_phytbl7_8_buf);
                                break;
                            case 24:
                            case 28:
                                wlc_phy_table_read_acphy(pi,
                                    tbl_list_ptr->tblid,
                                    tbl_list_ptr->tbl_len,
                                    tbl_list_ptr->start_offset,
                                    32, p_phytbl24_28_buf);
                                wlc_phy_table_write_acphy(pi,
                                    tbl_list_ptr->tblidaci,
                                    tbl_list_ptr->tbl_len,
                                    tbl_list_ptr->start_offset,
                                    32, p_phytbl24_28_buf);
                                break;
                        }
                    }
                }
                /* copy initgain for ACI to corresponding rfseq table */
                if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
                  ACMAJORREV_128(pi->pubpi->phy_rev)) {
                       wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0xf6, 16, &rfseq_aci_a_c0);
                       wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0xf7, 16, &rfseq_aci_a_c1);
                       wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0xf9, 16, &rfseq_aci_b_c0);
                       wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0xfa, 16, &rfseq_aci_b_c1);
                       wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0x5d0, 16, &rfseq_aci_a_c0);
                       wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0x5d2, 16, &rfseq_aci_a_c1);
                       wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0x5d1, 16, &rfseq_aci_b_c0);
                       wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQ, 1,
                               0x5d3, 16, &rfseq_aci_b_c1);
                }
            }
            if (ACPHY_MCLIP_ACI_MITIGATION(pi) &&
                (pi->sh->interference_mode & ACPHY_MCLIP_ACI_MIT))
                wlc_phy_mclip_aci_mitigation(pi, TRUE);
            ACPHY_ENABLE_STALL(pi, stall_val);
            wlapi_enable_mac(pi->sh->physhim);
        }
#endif /* !WLC_DISABLE_ACI */
    }

    /* Set up ED thresholds */
    wlc_phy_apply_edcrs_acphy(pi);

    if (ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
        /* force a reset2rx seq to let gain changes take effects */
        wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RESET2RX);
    }
}

void
wlc_phy_rxgainctrl_set_gaintbls_acphy_28nm(phy_info_t *pi,
    uint8 core, uint16 gain_tblid, uint16 gainbits_tblid)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxg_params_t *rxg_params = pi_ac->rxgcrsi->rxg_params;

    /* TIA */
    /* 2g & 5g settings */
    uint8 tia[16];
    uint8 tiabits[16];
    uint8 tialimit[16];
    uint8 i;
    uint8 elna_gainbits[] = {0, 0};
    uint8 elna_gainlimits[] = {0, 0};
    uint8 lna1_byp_bit = 6;

    uint16 gainlimit_tblid;
    if (core == 0) {
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT0;
    } else if (core == 1) {
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT1;
    } else if (core == 2) {
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT2;
    } else {
        gainlimit_tblid = ACPHY_TBL_ID_GAINLIMIT3;
    }

    /* ELNA setting */
    wlc_phy_table_write_acphy(pi, gainbits_tblid, pi_ac->rxgcrsi->rxgainctrl_stage_len[ELNA_ID],
        EXT_LNA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, elna_gainbits);
    wlc_phy_table_write_acphy(pi, gainlimit_tblid,
        pi_ac->rxgcrsi->rxgainctrl_stage_len[ELNA_ID],
        EXT_LNA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, elna_gainlimits);

    /* LNA1 settting already taken care of by upd_lna1_lna2_gains_acphy */

    /* Put LNA1 bypass index in gainbits */
    wlc_phy_table_write_acphy(pi, gainbits_tblid, 1,
        (LNA1_OFFSET + 6), ACPHY_GAINBITS_TBL_WIDTH, &lna1_byp_bit);

    /* LNA1 bypass related config */
    phy_ac_upd_lna1_bypass(pi, core);

    /* TIA setting */
    memcpy(tia, rxg_params->tia_gain_tbl, sizeof(int8) * N_TIA_GAINS);
    memcpy(tiabits, rxg_params->tia_gainbits_tbl, sizeof(int8) * N_TIA_GAINS);
    memcpy(tialimit, rxg_params->gainlimit_tbl[TIA_TBL_IND], sizeof(int8) * N_TIA_GAINS);
    /* The folloiwng code is only valid and necessary if N_TIA_GAINS is 12 and the HW
     * has a 16 entry TIA table
     */
    ASSERT(N_TIA_GAINS == 12);
    for (i = 0; i < 4; i++) {
        tia[N_TIA_GAINS+i] = rxg_params->tia_gain_tbl[N_TIA_GAINS-1];
        tiabits[N_TIA_GAINS+i] = rxg_params->tia_gainbits_tbl[N_TIA_GAINS-1];
        tialimit[N_TIA_GAINS+i] = rxg_params->gainlimit_tbl[TIA_TBL_IND][N_TIA_GAINS-1];
    }

    /* program tia table */
    wlc_phy_table_write_acphy(pi, gain_tblid, 16, TIA_OFFSET, ACPHY_GAINDB_TBL_WIDTH, tia);

    wlc_phy_table_write_acphy(pi, gainbits_tblid, 16, TIA_OFFSET, ACPHY_GAINBITS_TBL_WIDTH,
        tiabits);

    wlc_phy_table_write_acphy(pi, gainlimit_tblid, 16, TIA_OFFSET, ACPHY_GAINLMT_TBL_WIDTH,
        tialimit);
    wlc_phy_table_write_acphy(pi, gainlimit_tblid, 16, 64+TIA_OFFSET, ACPHY_GAINLMT_TBL_WIDTH,
        tialimit);

    /* copying values into gaintbl arrays to avoid reading from table */
    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID], tia,
        sizeof(pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[0][0])
        * N_TIA_GAINS);

    memcpy(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[TIA_ID], tiabits,
        sizeof(pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[0][0])
        * N_TIA_GAINS);

    /* BIQ0 setting */
    wlc_phy_table_write_acphy(pi, gain_tblid, N_BIQ01_GAINS, BIQ0_OFFSET,
        ACPHY_GAINDB_TBL_WIDTH, rxg_params->biq01_gain_tbl[0]);
    wlc_phy_table_write_acphy(pi, gainbits_tblid, N_BIQ01_GAINS, BIQ0_OFFSET,
        ACPHY_GAINBITS_TBL_WIDTH, rxg_params->biq01_gainbits_tbl[0]);

    /* BIQ1 setting */
    wlc_phy_table_write_acphy(pi, gain_tblid, 3, DVGA1_OFFSET, ACPHY_GAINDB_TBL_WIDTH,
        rxg_params->biq01_gain_tbl[1]);
    wlc_phy_table_write_acphy(pi, gainbits_tblid, 3, DVGA1_OFFSET, ACPHY_GAINBITS_TBL_WIDTH,
        rxg_params->biq01_gainbits_tbl[1]);

}

extern void wlc_phy_agc_28nm(phy_info_t *pi, bool init, bool band_change, bool bw_change)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi_ac->rxgcrsi;
    uint8 desense = 0;

    /* In elnabypaas mode, always issue gainctrl as one channel could be
       putting elna in bypass mode
    */
    if (pi->sh->interference_mode & ACPHY_ACI_ELNABYPASS)
        init = TRUE;

    if (rxgcrsi->rxg_params->mclip_agc_en) {
        wlc_phy_mclip_agc(pi, init, band_change, bw_change);
    } else {
        if (init || band_change || bw_change) {
            desense = rxgcrsi->curr_desense->clipgain_desense[0];
            wlc_phy_rxgainctrl_gainctrl_acphy_28nm(pi, desense);
        }
    }

    if (ACMAJORREV_47_51_129_130_131(pi->pubpi->phy_rev))
        phy_ac_rxgcrs_upd_knoise_shmem(pi, FALSE);
}

static void wlc_phy_mclip_agc(phy_info_t *pi, bool init, bool band_change, bool bw_change)
{
    phy_ac_rxgcrs_info_t *info = pi->u.pi_acphy->rxgcrsi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    const uint aci_diff_th_rev51 = 1;
    const uint16 aci_idx_offset_rev51 = 0;
    const uint aci_diff_th_rev128 = 1;
    const uint16 aci_idx_offset_rev128 = 0;
    const uint aci_diff_th_rev129 = 2;
    const uint16 aci_idx_offset_rev129 = 1;
    const uint aci_diff_th_rev130 = 2;
    const uint16 aci_idx_offset_rev130 = 1;
    info->curr_desense->elna_bypass = wlc_phy_get_max_lna_index_acphy(pi, ELNA_ID);

    /* Enable Mclip */
    phy_ac_rxgcrs_set_mclip_agc_en(pi, TRUE);

    if (init) {
        if (!ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
            FOREACH_CORE(pi, core) {
                /* set proper clip detector mux connection */
                WRITE_PHYREGC(pi, _RSSIMuxSel2, core,  0x06db);
            }
        }

        if (ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
            // By default disable Clip3 for mclip AGC as it was not decreasing
            // the gain in earlier PHY's, see CRDOT11ACPHY-2670
            uint16 clip3_en = 0;

            if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
                // 63178 has WCC RTL fix merged
                clip3_en = 1;
            } else if (CHSPEC_IS160(pi->radio_chanspec) &&
                       ACMAJORREV_47(pi->pubpi->phy_rev)) {
                clip3_en = 1;
            } else if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
                clip3_en = 0;
            }
            MOD_PHYREG(pi, ADC_PreClip_Enable, small_sig_gain_indx_decr, clip3_en);
        }
    }

    if (init || band_change || bw_change) {
        /* program init gain, legacy CLIP gains(ACI mitagation) & RSSIs */
        wlc_phy_rxgainctrl_gainctrl_acphy_28nm(pi, info->curr_desense->clipgain_desense[0]);
        if (ACMAJORREV_51_131(pi->pubpi->phy_rev)) {
            phy_ac_rxgcrs_mclip_aci_fix(pi, aci_diff_th_rev51, aci_idx_offset_rev51);
        } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            phy_ac_rxgcrs_mclip_aci_fix(pi, aci_diff_th_rev128, aci_idx_offset_rev128);
        } else if (ACMAJORREV_129(pi->pubpi->phy_rev) && 0) {
            /* Don't enable it yet. Need to first analyze it */
            phy_ac_rxgcrs_mclip_aci_fix(pi, aci_diff_th_rev129, aci_idx_offset_rev129);
        } else if (ACMAJORREV_130(pi->pubpi->phy_rev) && 0) {
            /* Don't enable it yet. Need to first analyze it */
            phy_ac_rxgcrs_mclip_aci_fix(pi, aci_diff_th_rev130, aci_idx_offset_rev130);
        }

        /* different clip gains for rev51 2G BW40 with eLNA for better jammer performance */
        if (ACMAJORREV_51_131(pi->pubpi->phy_rev) &&
          BF_ELNA_2G(pi_ac) && CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_force_clipgains_rev51(pi);
        } else {
            wlc_phy_mclip_agc_rxgainctrl(pi);
        }
    }
}

static void wlc_phy_mclip_agc_rssi_setup(phy_info_t *pi)
{
    if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
        wlc_phy_mclip_agc_rssi_setup_rev47(pi);
    } else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
        wlc_phy_mclip_agc_rssi_setup_rev51(pi);
    } else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        wlc_phy_mclip_agc_rssi_setup_rev129(pi);
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        wlc_phy_mclip_agc_rssi_setup_rev130(pi);
    } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
        wlc_phy_mclip_agc_rssi_setup_rev131(pi);
    } else {
        wlc_phy_mclip_agc_rssi_setup_acphy(pi);
    }
}

static void wlc_phy_mclip_agc_rssi_setup_acphy(phy_info_t *pi)
{

    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    uint8 ana_thld_5g_40_1[7] = {10, 4, 4, 4, 2, 2, 2};
    uint8 ana_thld_2g_r40[7] = {14, 4, 4, 4, 2, 2, 2};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 120, 120, 120, 120, 120, 120};
    const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  60,  60,  60,  60,  60,  60};
    uint8 clipcnt_thresh_bw20[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain, adc_gain, nb_gain, w1_gain;
    int8 *mclip_rssi;

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */
        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            if (CHSPEC_IS2G(pi->radio_chanspec) && (BF_ELNA_2G(pi_ac))) {
                phy_ac_set_20709_RSSI_detectors(pi, core, ana_thld_2g_r40);
                adclo_ref = -12;
                nblo_ref = -3;
                w1md_ref = 4;
            } else {
                phy_ac_set_20709_RSSI_detectors(pi, core, ana_thld_5g_40_1);
                adclo_ref = -12;
                nblo_ref = -2;
                w1md_ref = 4;
            }
        }

        if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw40);
        } else {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core, clipcnt_thresh_bw20);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        nb_gain = w1_gain + lna2_gain + tia_gain;
        adc_gain = nb_gain + lpf_gain;

        if (IS_28NM_RADIO(pi) || IS_16NM_RADIO(pi)) {
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;
            mclip_rssi[2] = mclip_rssi[1] + 6;
            mclip_rssi[3] = mclip_rssi[2] + 6;
            mclip_rssi[4] = mclip_rssi[3] + 1;
            mclip_rssi[5] = mclip_rssi[4] + 6;
            mclip_rssi[6] = mclip_rssi[5] + 6;
            mclip_rssi[8] = w1md_ref - w1_gain;
            mclip_rssi[7] = mclip_rssi[8] - 6;
            mclip_rssi[9] = mclip_rssi[8] + 6;
        }
    }

}

static void wlc_phy_mclip_agc_rssi_setup_rev47(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_ac_rxgcrs_info_t *rxgcrsi = pi_ac->rxgcrsi;
    uint8 core;
    uint8 ana_thld_20698r0_2g[7]  = {14, 3, 3, 3, 2, 2, 2};
    uint8 ana_thld_20698r0_5g[7]  = {8, 3, 3, 3, 2, 2, 2};
    uint8 ana_thld_20698rX_2g[7]  = {11, 6, 6, 6, 1, 1, 1};
    uint8 ana_thld_20698rX_5g[7]  = {11, 3, 3, 3, 2, 2, 2};
    const uint8 clipcnt_thresh_bw160[9] = {160, 160, 160, 160, 160, 160, 160, 160, 160};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 80, 80, 80, 80, 80, 80};
    const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  40,  40,  40,  40,  40,  40};
    uint8 clipcnt_thresh_bw20_2G[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
    uint8 clipcnt_thresh_bw20_5G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain;
    int8 adc_gain, nb_gain, w1_gain, w3_gain;
    int8 *mclip_rssi;

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */

        if (RADIOREV(pi->pubpi->radiorev) == 0) {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                phy_ac_set_20698_RSSI_detectors(pi, core,
                    ana_thld_20698r0_2g);
                } else {
                phy_ac_set_20698_RSSI_detectors(pi, core,
                    ana_thld_20698r0_5g);
                }
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                phy_ac_set_20698_RSSI_detectors(pi, core,
                    ana_thld_20698rX_2g);
            } else {
                phy_ac_set_20698_RSSI_detectors(pi, core,
                    ana_thld_20698rX_5g);
            }
        }

        if (CHSPEC_IS160(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                clipcnt_thresh_bw160);
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                clipcnt_thresh_bw40);
        } else {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw20_5G);
            else
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw20_2G);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        w3_gain = w1_gain + lna2_gain + tia_gain;
        nb_gain = w3_gain + lpf_gain;
        adc_gain = nb_gain;

        if (RADIOREV(pi->pubpi->radiorev) == 0) {
            adclo_ref = -10;
            nblo_ref = -6;
            w1md_ref = CHSPEC_IS2G(pi->radio_chanspec) ? -2 : 0;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;
            mclip_rssi[2] = mclip_rssi[1] + 6;
            mclip_rssi[3] = mclip_rssi[2] + 6;
            mclip_rssi[4] = mclip_rssi[3] + 1;
            mclip_rssi[5] = mclip_rssi[4] + 6;
            mclip_rssi[6] = mclip_rssi[5] + 6;
            mclip_rssi[8] = w1md_ref - w1_gain;
            mclip_rssi[7] = mclip_rssi[8] - 6;
            mclip_rssi[9] = mclip_rssi[8] +
                (CHSPEC_IS2G(pi->radio_chanspec) ? 12 : 6);
        } else {
            adclo_ref = -10;
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                nblo_ref = 0;
                w1md_ref = 2;

                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;
                mclip_rssi[2] = mclip_rssi[1] + 5;
                mclip_rssi[3] = mclip_rssi[2] + 8;

                /* w3_lo & nb_lo are 14dB apart if lpf_gain = 0 */
                mclip_rssi[4] = (nblo_ref + 14) - w3_gain;
                mclip_rssi[5] = mclip_rssi[4] + 6;
                mclip_rssi[6] = mclip_rssi[5] + 6;

                mclip_rssi[8] = w1md_ref - w1_gain;
                mclip_rssi[7] = mclip_rssi[8] - 7;
                mclip_rssi[9] = mclip_rssi[8] + 6;
            } else {
                nblo_ref = -6;
                w1md_ref = 0;
                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;
                mclip_rssi[2] = mclip_rssi[1] + 6;
                mclip_rssi[3] = mclip_rssi[2] + 8;

                /* w3_lo & nb_lo are 16dB apart if lpf_gain = 0 */
                mclip_rssi[4] = (nblo_ref + 16) - w3_gain;
                mclip_rssi[5] = mclip_rssi[4] + 2;
                mclip_rssi[6] = mclip_rssi[5] + 6;

                mclip_rssi[8] = w1md_ref - w1_gain;
                mclip_rssi[7] = mclip_rssi[8] - 5;
                mclip_rssi[9] = mclip_rssi[8] + 6;
            }
        }
    }
}

static void wlc_phy_mclip_agc_rssi_setup_rev129(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    uint8 ana_thld_20707rX_2g[7]  = {14, 3, 3, 3, 2, 2, 2};
    uint8 ana_thld_20707rX_5g[7]  = {13, 7, 7, 7, 3, 3, 3};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 80, 80, 80, 80, 80, 80};
    const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  40,  40,  40,  40,  40,  40};
    uint8 clipcnt_thresh_bw20_2G[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
    uint8 clipcnt_thresh_bw20_5G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain, adc_gain, nb_gain, w1_gain;
    int8 *mclip_rssi;

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            phy_ac_set_20707_RSSI_detectors(pi, core,
                    ana_thld_20707rX_2g);
        } else {
            phy_ac_set_20707_RSSI_detectors(pi, core,
                    ana_thld_20707rX_5g);
        }

        if (CHSPEC_IS160(pi->radio_chanspec)) {
            ASSERT(0);
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw40);
        } else {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_5G);
            else
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_2G);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        nb_gain = w1_gain + lna2_gain + tia_gain;
        adc_gain = nb_gain + lpf_gain;

        adclo_ref = -10;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            nblo_ref = -3;
            w1md_ref = 2;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 5;    // NBmd = NBlo + 5
            mclip_rssi[3] = mclip_rssi[2] + 8;    // NBhi = NBmd + 8
            mclip_rssi[4] = mclip_rssi[3] + 1;    // W3lo = NBhi + 1
            mclip_rssi[5] = mclip_rssi[4] + 5;    // W3md = W3lo + 5
            mclip_rssi[6] = mclip_rssi[5] + 5;    // W3hi = W3md + 5
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 6;    // W1lo = W1md - 6
            mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
        } else {
            nblo_ref = -1;
            w1md_ref = 4;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
            mclip_rssi[3] = mclip_rssi[2] + 5;    // NBhi = NBmd + 5
            mclip_rssi[4] = mclip_rssi[3] + 2;    // W3lo = NBi + 2
            mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3Lo + 6
            mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3Md + 6
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
            mclip_rssi[9] = mclip_rssi[8] + 7;    // W1hi = W1md + 7
        }
    }
}

static void wlc_phy_mclip_agc_rssi_setup_rev130(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    uint8 ana_thld_20708rX_2g[9]  = {0, 11, 15, 7, 7, 7, 6, 6, 6};
    uint8 ana_thld_20708rX_2g_b0[9]  = {8,  8,  8, 7, 7, 7, 2, 2, 2};
    uint8 ana_thld_20708rX_5g[9]  = {0, 11, 15, 7, 7, 7, 6, 6, 6};
    uint8 ana_thld_20708rX_5g_b0[9]  = {3, 1, 2, 5, 5, 5, 4, 4, 4};
    uint8 ana_thld_20708rX_6g_b0[9]  = {3, 7, 9, 7, 7, 7, 4, 4, 4};
    uint8 ana_thld_20708rX_5g_bw160_b0[9]  = {3, 3, 1, 7, 7, 7, 1, 1, 1};
    uint8 ana_thld_20708rX_5g_bw160_a0[9]  = {0, 6, 15, 4, 4, 4, 6, 6, 6};
    uint8 ana_thld_20708rX_5g_bw160[9]  = {0, 0, 15, 2, 2, 2, 6, 6, 6};
    const uint8 clipcnt_thresh_bw160_a0[9] = {160, 80, 80, 80, 80, 80, 160, 160, 160};
    const uint8 clipcnt_thresh_bw160_b0[9] = {90, 90, 90, 90, 90, 90, 90, 90, 90};
    const uint8 clipcnt_thresh_bw160[9] = {160, 160, 160, 160, 160, 160, 160, 160, 160};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 80, 80, 80, 80, 80, 80};
    uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  40,  40,  40,  40,  40,  40};
    uint8 clipcnt_thresh_bw20_2G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    uint8 clipcnt_thresh_bw20_5G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain, adc_gain, nb_gain, w1_gain;
    int8 *mclip_rssi;
    const bool chspec_is5g160 = CHSPEC_IS160(pi->radio_chanspec) &&
            CHSPEC_IS5G(pi->radio_chanspec);
    acphy_desense_values_t *desense = NULL;
    uint8 desense_state;

    if ((desense = phy_ac_noise_get_desense(pi->u.pi_acphy->noisei)) == NULL) {
        desense_state = 0;
    } else {
        desense_state = phy_ac_noise_get_desense_state(pi->u.pi_acphy->noisei);
    }

    if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        if ((desense_state != 0) && CHSPEC_IS5G(pi->radio_chanspec)) {
            clipcnt_thresh_bw40[4] = 50;
            clipcnt_thresh_bw20_5G[4] = 50;
            clipcnt_thresh_bw20_2G[4] = 40;
        }
    }

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */
        if (RADIOMAJORREV(pi) < 2) {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                phy_ac_set_20708_RSSI_detectors(pi, core,
                        ana_thld_20708rX_2g);
            } else {
                if (chspec_is5g160) {
                    if (RADIOMAJORREV(pi) == 0) {
                        phy_ac_set_20708_RSSI_detectors(pi, core,
                                ana_thld_20708rX_5g_bw160_a0);
                    } else {
                        phy_ac_set_20708_RSSI_detectors(pi, core,
                                ana_thld_20708rX_5g_bw160);
                    }
                } else {
                    phy_ac_set_20708_RSSI_detectors(pi, core,
                            ana_thld_20708rX_5g);
                }
            }
        } else {
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                phy_ac_set_20708_RSSI_detectors(pi, core,
                        ana_thld_20708rX_2g_b0);
            } else {
                if (CHSPEC_IS160(pi->radio_chanspec)) {
                    phy_ac_set_20708_RSSI_detectors(pi, core,
                            ana_thld_20708rX_5g_bw160_b0);
                } else {
                    if (CHSPEC_IS5G(pi->radio_chanspec)) {
                        phy_ac_set_20708_RSSI_detectors(pi, core,
                                ana_thld_20708rX_5g_b0);
                    } else {
                        phy_ac_set_20708_RSSI_detectors(pi, core,
                                ana_thld_20708rX_6g_b0);
                    }
                }
            }
        }

        if (CHSPEC_IS160(pi->radio_chanspec)) {
            if (RADIOMAJORREV(pi) == 0) {
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw160_a0);
            } else if (RADIOMAJORREV(pi) >= 2) {
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw160_b0);
            } else {
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw160);
            }
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                clipcnt_thresh_bw40);
        } else {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw20_5G);
            else
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw20_2G);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        nb_gain = w1_gain + lna2_gain + tia_gain;
        adc_gain = nb_gain + lpf_gain;

        if (RADIOMAJORREV(pi) < 2) {
            adclo_ref = -10;
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                nblo_ref = -3;
                w1md_ref = 2;
                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
                mclip_rssi[2] = mclip_rssi[1] + 5;    // NBmd = NBlo + 5
                mclip_rssi[3] = mclip_rssi[2] + 8;    // NBhi = NBmd + 8
                mclip_rssi[4] = mclip_rssi[3] + 1;    // W3lo = NBhi + 1
                mclip_rssi[5] = mclip_rssi[4] + 5;    // W3md = W3lo + 5
                mclip_rssi[6] = mclip_rssi[5] + 5;    // W3hi = W3md + 5
                mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
                mclip_rssi[7] = mclip_rssi[8] - 6;    // W1lo = W1md - 6
                mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
            } else {
                nblo_ref = -1;
                w1md_ref = 4;
                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
                mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
                mclip_rssi[3] = mclip_rssi[2] + 5;    // NBhi = NBmd + 5
                mclip_rssi[4] = mclip_rssi[3] + 2;    // W3lo = NBi + 2
                mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3Lo + 6
                mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3Md + 6
                mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
                mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
                mclip_rssi[9] = mclip_rssi[8] + 7;    // W1hi = W1md + 7
            }
        } else {
            adclo_ref = -10;
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                nblo_ref = -8;
                w1md_ref = 0;
                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
                mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
                mclip_rssi[3] = mclip_rssi[2] + 6;    // NBhi = NBmd + 6
                mclip_rssi[4] = mclip_rssi[3] + 4;    // W3lo = NBhi + 4
                mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3lo + 6
                mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3md + 6
                mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
                mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
                mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
            } else {
                nblo_ref = -5;
                w1md_ref = 0;
                mclip_rssi[0] = adclo_ref - adc_gain;
                mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
                mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
                mclip_rssi[3] = mclip_rssi[2] + 5;    // NBhi = NBmd + 5
                mclip_rssi[4] = mclip_rssi[3] + 0;    // W3lo = NBi  + 0
                mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3Lo + 6
                mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3Md + 6
                mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
                mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
                mclip_rssi[9] = mclip_rssi[8] + 7;    // W1hi = W1md + 7
            }
        }
    }
}

static void wlc_phy_mclip_agc_rssi_setup_rev51(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    const uint8 ana_thld_20704rX_2g[7]  = {14, 3, 3, 3, 2, 2, 2};
    const uint8 ana_thld_20704rX_2g_bw40[7]  = {14, 6, 6, 6, 7, 7, 7};
    const uint8 ana_thld_20704rX_5g[7]  = {13, 7, 7, 7, 3, 3, 3};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 80, 80, 80, 80, 80, 80};
    const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  40,  40,  40,  40,  40,  40};
    uint8 clipcnt_thresh_bw20_2G[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
    uint8 clipcnt_thresh_bw20_5G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain, adc_gain, nb_gain, w1_gain;
    int8 *mclip_rssi;
    uint8 const* ana_thld_20704rX;

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* different rssi clip thresholds for 2G BW40 with eLNA
                for better ACI/blocker performance
                */
            if (BF_ELNA_2G(pi_ac) && CHSPEC_IS40(pi->radio_chanspec)) {
                ana_thld_20704rX = ana_thld_20704rX_2g_bw40;
            } else {
                ana_thld_20704rX = ana_thld_20704rX_2g;
            }
        } else {
            ana_thld_20704rX = ana_thld_20704rX_5g;
        }
        phy_ac_set_20704_RSSI_detectors(pi, core, ana_thld_20704rX);

        if (CHSPEC_IS160(pi->radio_chanspec)) {
            ASSERT(0);
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw40);
        } else {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_5G);
            else
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_2G);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        nb_gain = w1_gain + lna2_gain + tia_gain;
        adc_gain = nb_gain + lpf_gain;

        adclo_ref = -10;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            nblo_ref = 0;
            w1md_ref = 2;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 5;    // NBmd = NBlo + 5
            mclip_rssi[3] = mclip_rssi[2] + 8;    // NBhi = NBmd + 8
            mclip_rssi[4] = mclip_rssi[3] + 1;    // W3lo = NBhi + 1
            mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3lo + 6
            mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3md + 6
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
            mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
        } else {
            nblo_ref = -6;
            w1md_ref = 0;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
            mclip_rssi[3] = mclip_rssi[2] + 8;    // NBhi = NBmd + 8
            mclip_rssi[4] = mclip_rssi[3] + 2;    // W3lo = NBi + 2
            mclip_rssi[5] = mclip_rssi[4] + 2;    // W3md = W3Lo + 2
            mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3Md + 6
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 5;    // W1lo = W1md - 5
            mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
        }
    }
}

static void wlc_phy_mclip_agc_force_clipgains_rev51(phy_info_t *pi)
{
    uint8 core;
    const uint32 mclip_gaincode_rev51[] = {0x400f70, 0x400470, 0x40036c, 0x400368, 0x400268,
    0x400268, 0x400364, 0x400471, 0x400271, 0x400369, 0x400365};

    FOREACH_CORE(pi, core) {
        /* override clip gains */
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RSSICLIPGAIN0 + core * 32, 11, 0, 32,
                mclip_gaincode_rev51);
    }

}

static void wlc_phy_mclip_agc_rssi_setup_rev131(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 core;
    const uint8 ana_thld_20710rX_2g[7]  = {14, 3, 3, 3, 2, 2, 2};
    const uint8 ana_thld_20710rX_2g_bw40[7]  = {14, 6, 6, 6, 7, 7, 7};
    const uint8 ana_thld_20710rX_5g[7]  = {14, 7, 7, 7, 3, 3, 3};
    const uint8 clipcnt_thresh_bw160[9] = {160, 160, 160, 160, 160, 160, 160, 160, 160};
    const uint8 clipcnt_thresh_bw80[9] = {80, 80, 80, 80, 80, 80, 80, 80, 80};
    const uint8 clipcnt_thresh_bw40[9] = {40, 40, 40,  40,  40,  40,  40,  40,  40};
    uint8 clipcnt_thresh_bw20_2G[9] = {20, 20, 20,  30,  30,  30,  30,  30,  30};
    uint8 clipcnt_thresh_bw20_5G[9] = {20, 20, 20,  20,  20,  20,  20,  20,  20};
    int8 adclo_ref = 0, nblo_ref = 0, w1md_ref = 0;
    uint8 elna_indx, lna1_indx, lna2_indx, tia_indx, lpf_indx, trtx;
    int8 elna_gain, lna1_gain, lna2_gain, tia_gain, lpf_gain, adc_gain, nb_gain, w1_gain;
    int8 *mclip_rssi;
    uint8 const* ana_thld_20710rX;

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];

        /* each core is programed the same threshold. can be different if necessary */
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            /* different rssi clip thresholds for 2G BW40 with eLNA
                for better ACI/blocker performance
                */
            if (BF_ELNA_2G(pi_ac) && CHSPEC_IS40(pi->radio_chanspec)) {
                ana_thld_20710rX = ana_thld_20710rX_2g_bw40;
            } else {
                ana_thld_20710rX = ana_thld_20710rX_2g;
            }
        } else {
            ana_thld_20710rX = ana_thld_20710rX_5g;
        }
        phy_ac_set_20710_RSSI_detectors(pi, core, ana_thld_20710rX);

        if (CHSPEC_IS160(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw160);
        } else if (CHSPEC_IS80(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw80);
        } else if (CHSPEC_IS40(pi->radio_chanspec)) {
            wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                    clipcnt_thresh_bw40);
        } else {
            if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_5G);
            else
                wlc_phy_mclip_agc_clipcnt_thresh(pi, core,
                        clipcnt_thresh_bw20_2G);
        }

        elna_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initExtLnaIndex);
        lna1_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initLnaIndex);
        lna2_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initlna2Index);
        tia_indx = READ_PHYREGFLDC(pi, InitGainCodeA, core, initmixergainIndex);
        lpf_indx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitBiQ0Index);
        trtx = READ_PHYREGFLDC(pi, InitGainCodeB, core, InitHiTrTxIndex);
        if (tia_indx > 11) tia_indx = 11;

        elna_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[ELNA_ID][elna_indx];
        lna1_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA1_ID][lna1_indx];
        lna2_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[LNA2_ID][lna2_indx];
        tia_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[TIA_ID][tia_indx];
        lpf_gain = pi_ac->rxgcrsi->rxgainctrl_params[core].gaintbl[BQ0_ID][lpf_indx];
        w1_gain = elna_gain + lna1_gain;
        w1_gain -= (trtx) ? pi_ac->rxgcrsi->fem_rxgains[core].trloss : 0;
        nb_gain = w1_gain + lna2_gain + tia_gain;
        adc_gain = nb_gain + lpf_gain;

        adclo_ref = -10;
        if (CHSPEC_IS2G(pi->radio_chanspec)) {
            nblo_ref = 0;
            w1md_ref = 2;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 5;    // NBmd = NBlo + 5
            mclip_rssi[3] = mclip_rssi[2] + 8;    // NBhi = NBmd + 8
            mclip_rssi[4] = mclip_rssi[3] + 1;    // W3lo = NBhi + 1
            mclip_rssi[5] = mclip_rssi[4] + 6;    // W3md = W3lo + 6
            mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3md + 6
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 7;    // W1lo = W1md - 7
            mclip_rssi[9] = mclip_rssi[8] + 6;    // W1hi = W1md + 6
        } else {
            nblo_ref = 2;
            w1md_ref = 5;
            mclip_rssi[0] = adclo_ref - adc_gain;
            mclip_rssi[1] = nblo_ref - nb_gain;    // NBlo
            mclip_rssi[2] = mclip_rssi[1] + 6;    // NBmd = NBlo + 6
            mclip_rssi[3] = mclip_rssi[2] + 6;    // NBhi = NBmd + 6
            mclip_rssi[4] = mclip_rssi[3] + 5;    // W3lo = NBhi + 5
            mclip_rssi[5] = mclip_rssi[4] + 5;    // W3md = W3Lo + 5
            mclip_rssi[6] = mclip_rssi[5] + 6;    // W3hi = W3Md + 6
            mclip_rssi[8] = w1md_ref - w1_gain;    // W1md
            mclip_rssi[7] = mclip_rssi[8] - 9;    // W1lo = W1md - 9
            mclip_rssi[9] = mclip_rssi[8] + 15;    // W1hi = W1md + 15
        }
    }
}

void wlc_phy_mclip_agc_rxgainctrl(phy_info_t *pi)
{
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    int8 margin_low_sens = 15, margin_high_sens = 12;
    int8 det_adjust_list[] = {24, 22, 20, 18};
    int8 high_sens_adjust_list_2g[] = {14, 14, 14, 14};
    int8 high_sens_adjust_list_2g_rev128[] = {10, 10, 10, 10};
    int8 high_sens_adjust_list_5g[] = {6, 8, 9, 10};
    int8 high_sens_adjust_list_rev129[] = {10, 10, 10, 10};
    int8 high_sens_adjust_list_rev130[] = {10, 10, 10, 10};
    int8 *high_sens_adjust_list;
    int8 high_adjust, det_adjust;
    bool elna_present = CHSPEC_IS2G(pi->radio_chanspec) ? BF_ELNA_2G(pi_ac)
                                                        : BF_ELNA_5G(pi_ac);
    bool lna1_byp = CHSPEC_IS2G(pi->radio_chanspec) ?
        ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
        & BFL2_LNA1BYPFORTR2G) != 0) :
        ((BOARDFLAGS2(GENERIC_PHY_INFO(pi)->boardflags2)
        & BFL2_LNA1BYPFORTR5G) != 0);
    uint8 i, tbl_sz;
    uint8 bw_indx = CHSPEC_IS20(pi->radio_chanspec) ? 0 :
            CHSPEC_IS40(pi->radio_chanspec) ? 1 :
            CHSPEC_IS80(pi->radio_chanspec) ? 2 :
            ACMAJORREV_47_130(pi->pubpi->phy_rev) ? 3 : 2;

    int8 sens_hi, sens_lna1Byp_lo, sens_lna1Byp_hi, sens_elnaByp_lo = -1;
    int8 lna1Byp_gain_idx;
    int8 elnaByp_gain_idx;
    int8 sens_high, sens_low;
    uint8 gain_indx[7];
    uint32 mclip_gaincode[11];
    uint8 core;
    int8 rssi, rssi_low, rssi_high, clipgain, min_gain, max_gain, trtx, lna1byp;
    uint8 mag;
    int8 *mclip_rssi;
    bool lna1byp_en = FALSE;
    const bool chspec_is160 = ((CHSPEC_IS160(pi->radio_chanspec)) &&
     ((RADIOMAJORREV(pi) >= 2) ||
     ((RADIOMAJORREV(pi) < 2) && (CHSPEC_IS5G(pi->radio_chanspec)))));
    uint32 clp2_tbl_bw160_iLNAbyp[] =
            {0x9987433, 0x9987644, 0x9999965, 0x9999965,
            0x9999977, 0x9999997, 0x9999999, 0x9999999};
    uint32 clp2_tbl_bw160_Nor[] =
            {0x9987433, 0x9987644, 0x9987655, 0x9998766,
            0x9998777, 0x9999888, 0x9999999, 0x9999999};
    uint32 clp2_tbl[8];
    uint8 tbl;
    tbl_sz = (uint8) ARRAYSIZE(clp2_tbl);

    mag = READ_PHYREGFLD(pi, Core0HpFBw, maxAnalogGaindb);
    for (i = 0; i < ACPHY_MAX_RX_GAIN_STAGES; i++)
        pi_ac->rxgcrsi->rxgainctrl_maxout_gains[i] = mag;

    if (ACMAJORREV_130(pi->pubpi->phy_rev))
        lna1byp_en = phy_ac_noise_get_lna1byp(pi_ac->rxgcrsi->aci->noisei);

    if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
        high_sens_adjust_list = high_sens_adjust_list_rev129;
    } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
        high_sens_adjust_list = high_sens_adjust_list_rev130;
    } else if (CHSPEC_IS2G(pi->radio_chanspec)) {
        high_sens_adjust_list = (ACMAJORREV_GE37(pi->pubpi->phy_rev)) ?
            high_sens_adjust_list_2g_rev128: high_sens_adjust_list_2g;
    } else {
        high_sens_adjust_list = high_sens_adjust_list_5g;
    }
    det_adjust = det_adjust_list[bw_indx];
    high_adjust = high_sens_adjust_list[bw_indx];

    if (ACMAJORREV_130(pi->pubpi->phy_rev) && elna_present && chspec_is160) {
        if (lna1byp_en == TRUE) {
            for (i = 0; i < tbl_sz; i++)
                clp2_tbl[i] = clp2_tbl_bw160_iLNAbyp[i];
        } else {
            for (i = 0; i < tbl_sz; i++)
                clp2_tbl[i] = clp2_tbl_bw160_Nor[i];
        }
    }

    FOREACH_CORE(pi, core) {
        mclip_rssi = (int8 *) pi_ac->rxgcrsi->mclip_rssi[core];
        sens_hi = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, 20, 0, 0, core);
        sens_lna1Byp_lo = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, 60, 0, 1, core);
        sens_lna1Byp_hi = wlc_phy_rxgainctrl_calc_high_sens_acphy(pi, 20, 0, 1, core);
        lna1Byp_gain_idx = wlc_phy_mclip_agc_calc_lna_bypass_rssi(pi, mclip_rssi,
                                                                  sens_lna1Byp_lo, sens_hi);
        tbl = (uint8) ACPHY_TBL_ID_MCLPAGCCLIP2TBL0 + 32 * core;
        elnaByp_gain_idx = 100;
        if (elna_present) {
            // Elna Bypass mode
            if (ACMAJORREV_GE40(pi->pubpi->phy_rev)) {
                if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    if (lna1byp_en == TRUE) {
                        if (chspec_is160) {
                            lna1Byp_gain_idx = 3;
                        }
                    } else {
                        if (chspec_is160) {
                            if (RADIOMAJORREV(pi) < 2) {
                                lna1Byp_gain_idx = 7;
                            } else {
                                lna1Byp_gain_idx = 6;
                            }
                        } else {
                            lna1Byp_gain_idx = 100;
                        }
                    }
                    if (chspec_is160)
                        wlc_phy_table_write_acphy(pi, tbl, tbl_sz, 0x1,
                            32, clp2_tbl);
                } else {
                    /* disable using lna1bypass as elna is present */
                    lna1Byp_gain_idx = 100;
                }
            }
            if (pi_ac->rxgcrsi->curr_desense->elna_bypass) {
                elnaByp_gain_idx = 0;
            } else {
                sens_elnaByp_lo = wlc_phy_rxgainctrl_calc_low_sens_acphy(pi, 60,
                    1, 0, core);
                elnaByp_gain_idx = wlc_phy_mclip_agc_calc_lna_bypass_rssi(pi,
                    mclip_rssi, sens_elnaByp_lo, sens_lna1Byp_hi);
                if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
                    elnaByp_gain_idx = elnaByp_gain_idx - 1;
                }
                if (ACMAJORREV_130(pi->pubpi->phy_rev) && (chspec_is160)) {
                    if (RADIOMAJORREV(pi) < 2) {
                        elnaByp_gain_idx = 9;
                    } else {
                        elnaByp_gain_idx = 8;
                    }
                }
                if (ACMAJORREV_131(pi->pubpi->phy_rev) &&
                        CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
                    // 6756 elnaByp_gain_idx was getting to low for 6G and
                    // prevents high power RxPER spike for 5G/160 MHz
                    elnaByp_gain_idx = 6;
                }
            }
        }
        PHY_INFORM(("sens_hi %d sens_lna1Byp_lo %d sens_lna1Byp_hi %d, lna1Byp_gain_idx %d,"
            "sens_elnaByp_lo %d elnaByp_gain_idx %d\n",
            sens_hi, sens_lna1Byp_lo, sens_lna1Byp_hi, lna1Byp_gain_idx,
            sens_elnaByp_lo, elnaByp_gain_idx));

        for (i = 0; i <= 10; i++) {
            if (i == 0) {
                if (!(ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
                    !ACMAJORREV_128(pi->pubpi->phy_rev))) {
                    rssi = mclip_rssi[0];
                    sens_high = rssi + margin_high_sens;
                    clipgain = high_adjust - sens_high;
                    clipgain = MIN(clipgain, ACPHY_INIT_GAIN_28NM);
                } else {
                    clipgain = ACPHY_INIT_GAIN_28NM;
                }
            } else if (i == 1) {
                rssi = mclip_rssi[1];
                sens_high = rssi + margin_high_sens;
                clipgain = high_adjust - sens_high;
            } else if (i == 10) {
                rssi = mclip_rssi[9];
                sens_low = rssi - margin_low_sens;
                clipgain = 0 - sens_low - det_adjust;
            } else {
                rssi_low = mclip_rssi[i-1];
                rssi_high = mclip_rssi[i];
                sens_low = rssi_low - margin_low_sens;
                sens_high = rssi_high + margin_high_sens;
                min_gain = 0 - sens_low - det_adjust;
                max_gain = high_adjust - sens_high;
                clipgain = (min_gain + max_gain)>>1;
            }

            if (elna_present) {
                trtx = (i >= elnaByp_gain_idx) ? 1 : 0;
                lna1byp = (i >= lna1Byp_gain_idx) && (trtx == 0);
            } else if (lna1_byp) {
                trtx = 0;
                /* possibility to shift calculated value */
                if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
                    lna1byp = (i >= (lna1Byp_gain_idx + 0));
                } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    lna1byp = (i >= (lna1Byp_gain_idx + 0));
                } else if (ACMAJORREV_51_131(pi->pubpi->phy_rev) ||
                    ACMAJORREV_128(pi->pubpi->phy_rev))  {
                    lna1byp = (i >= (lna1Byp_gain_idx + 2));
                } else {
                    lna1byp = (i >= (lna1Byp_gain_idx + 3));
                }
            } else {
                trtx = 0;
                lna1byp = 0;
            }

            if ((ACMAJORREV_130(pi->pubpi->phy_rev)) && (i > 0)) {
                if (trtx == 1) {
                    clipgain =  clipgain - 6;
                } else {
                    clipgain =  clipgain - 3;
                }
            }

            if (i == 0) {
                /* Program init gain */
                wlc_phy_rxgainctrl_set_init_clip_gain_acphy(pi, 0, clipgain,
                    trtx, 0, core);
                if (pi_ac->rxgcrsi->fixed_gain_ncal != 0) {
                    phy_ac_rxgcrs_setup_fixedgain_noisecal(pi_ac->rxgcrsi);
                }
                wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, clipgain, trtx,
                        0, 0, gain_indx);
            } else {
                wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, clipgain, trtx,
                        lna1byp, 7, gain_indx);
            }
            mclip_gaincode[i] = wlc_phy_mclip_agc_pack_gains(pi, gain_indx,
                    trtx, lna1byp);
            PHY_INFORM(("mclip_agc: clip%d code 0x%08x\n", i, mclip_gaincode[i]));
        }

        /* program clip gain */
        wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RSSICLIPGAIN0 + core * 32, 11, 0, 32,
                mclip_gaincode);
    }
}

/*
Select nb/w1 clip muxes for normal AGC
These are not used for mclip, as all nb/w3/w1 (lo/md/hi) are used in mclip
*/
static void wlc_phy_agc_nbw1mux_sel(phy_info_t *pi, uint8 core, int8 mclip_rssi[])
{
    uint8 nb_start = 1, w1_start = 7;
    uint8 nbw3mux[] = {0,0,0,1,1,1}; /* nb or w3 */
    uint8 nbmux[] = {0, 1, 2, 0, 1, 2}; /* lo/md/hi */
    uint8 i, idx, diff, min_diff;

    int8 nbclip, w1clip;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    nbclip = pi_ac->rxgcrsi->nbclip_dBm[core];
    w1clip = pi_ac->rxgcrsi->w1clip_dBm[core];

    /* NB clip */
    if (nbclip == 0) {
        idx = 4; /* w3_md */
    } else {
        idx =  nb_start; /* ignore idx 0 (adc_clip) */
        min_diff = ABS(mclip_rssi[idx] - nbclip);
        for (i = 2; i < 7; i++) {
            diff = ABS(mclip_rssi[i] -  nbclip);
            if (diff < min_diff) {
                min_diff = diff;
                idx = i;
            }
        }
        idx -= nb_start;  /* ref back to start pos 0 */
    }
    MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW3ClipMuxSel, nbw3mux[idx]);
    MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcNbClipMuxSel, nbmux[idx]);

    /* w1 clip */
    if (w1clip == 0) {
        idx = 1; /* md */
    } else {
        idx =  w1_start; /* w1 starting pos */
        min_diff = ABS(mclip_rssi[idx] - w1clip);
        for (i = 8; i < 10; i++) {
            diff = ABS(mclip_rssi[i] -  w1clip);
            if (diff <= min_diff) {
                min_diff = diff;
                idx = i;
            }
        }
        idx -= w1_start;  /* ref back to start pos 0 */
    }
    MOD_PHYREGC(pi, RssiClipMuxSel, core, fastAgcW1ClipMuxSel, idx);
}

static void wlc_phy_mclip_agc_clipcnt_thresh(phy_info_t *pi, uint8 core, const uint8 thresh_list[])
{
    /* W1 HI/MD/LW */
    MOD_PHYREGC(pi, mClpAgcW1ClipCntTh,  core, mClpAgcW1ClipCntThHi, thresh_list[0]);
    MOD_PHYREGC(pi, mClpAgcMDClipCntTh2, core, mClpAgcW1ClipCntThMd, thresh_list[1]);
    MOD_PHYREGC(pi, mClpAgcW1ClipCntTh,  core, mClpAgcW1ClipCntThLo, thresh_list[2]);
    /* W3 HI/MD/LW */
    MOD_PHYREGC(pi, mClpAgcW3ClipCntTh,  core, mClpAgcW3ClipCntThHi, thresh_list[3]);
    MOD_PHYREGC(pi, mClpAgcMDClipCntTh1, core, mClpAgcW3ClipCntThMd, thresh_list[4]);
    MOD_PHYREGC(pi, mClpAgcW3ClipCntTh,  core, mClpAgcW3ClipCntThLo, thresh_list[5]);
    /* NB HI/MD/LW */
    MOD_PHYREGC(pi, mClpAgcNbClipCntTh,  core, mClpAgcNbClipCntThHi, thresh_list[6]);
    MOD_PHYREGC(pi, mClpAgcMDClipCntTh1, core, mClpAgcNbClipCntThMd, thresh_list[7]);
    MOD_PHYREGC(pi, mClpAgcNbClipCntTh,  core, mClpAgcNbClipCntThLo, thresh_list[8]);

}

static uint8
wlc_phy_mclip_agc_calc_lna_bypass_rssi(phy_info_t *pi, int8 *mclip_rssi, int8 lo, int8 hi)
{
    int8 target_rssi, rssi_diff, min_rssi_diff = 30, min_rssi_idx = 0, i;
    target_rssi = (lo + hi) >> 1;

    for (i = 0; i < 10; i++) {
        rssi_diff = ABS(mclip_rssi[i] - target_rssi);
        if (rssi_diff < min_rssi_diff) {
            min_rssi_diff = rssi_diff;
            min_rssi_idx = i+1;  // We start with G1, not G0
        }
    }
    return min_rssi_idx;
}

static uint32
wlc_phy_mclip_agc_pack_gains(phy_info_t *pi, uint8 *gain_indx, uint8 trtx, uint8 lna1byp)
{
    uint32 gaincode;
    gaincode = ((lna1byp&0x1)<<23) + (1<<22) +
        ((gain_indx[6]&0xF)<<18) +
        ((gain_indx[5]&0x7)<<15) +
        ((gain_indx[4]&0x7)<<12) +
        ((gain_indx[3]&0xF)<< 8) +
        ((gain_indx[2]&0x7)<< 5) +
        ((gain_indx[1]&0x7)<< 2) +
        ((gain_indx[0]&0x1)<< 1) + (trtx&0x1);

    return gaincode;

}

void
wlc_phy_apply_edcrs_acphy(phy_info_t *pi)
{
    uint8 region_group = wlc_phy_get_locale(pi->rxgcrsi);
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    acphy_desense_values_t *desense = NULL;
    int32 edcrs_desense, edcrs = 0;
    int32 final_edcrs, prev_edcrs;

    /* Check if desense is setting edcrs */
    desense = pi_ac->rxgcrsi->total_desense;
    edcrs_desense = desense->edcrs;

    if (VALID_ED_THRESH(edcrs_desense)) {
        edcrs = edcrs_desense;
    } else {
        if (region_group ==  REGION_EU) {
            edcrs = CHSPEC_IS2G(pi->radio_chanspec) ?
                pi->srom_eu_edthresh2g : pi->srom_eu_edthresh5g;
        } else {
            edcrs = CHSPEC_IS2G(pi->radio_chanspec) ?
                pi_ac->sromi->ed_thresh2g : pi_ac->sromi->ed_thresh5g;
        }
    }

    if (VALID_ED_THRESH(edcrs)) {
#ifdef CONFIG_TENDA_PRIVATE_WLAN
        /* Edcrs parameters are abnormal due to scanning; 
        Here we assign the initial value const_ed_th_high;
        The default value is DYN_ED_THRESH_HIGH*/
        final_edcrs = STATIC_ED_THRESH_DEFAULT;
#else
        final_edcrs = edcrs;
#endif
    } else {
        final_edcrs = pi_ac->sromi->ed_thresh_default;
    }
    wlc_phy_adjust_ed_thres_acphy(pi_ac->rxgcrsi, &final_edcrs, TRUE);

    // check static edcrs threshold change
    if (!SCAN_RM_IN_PROGRESS(pi) && !ISSIM_ENAB(pi->sh->sih)) {
#ifdef CONFIG_TENDA_PRIVATE_WLAN
        prev_edcrs = phy_ac_noise_get_dynamic_ed_thresh(pi, FALSE, NULL);
#else
        prev_edcrs = phy_ac_noise_get_dynamic_ed_thresh(pi, FALSE);
#endif /* CONFIG_TENDA_PRIVATE_WLAN */
        if (prev_edcrs != final_edcrs) {
            // Use the new static edcrs threshold for dynamic edcrs
            phy_ac_noise_set_dynamic_ed_thresh(pi, final_edcrs, TRUE);
            phy_ac_noise_set_dynamic_ed_thresh(pi, final_edcrs, FALSE);
        }
    }

}

static void wlc_phy_adjust_ed_thres_acphy(phy_type_rxgcrs_ctx_t *ctx, int32 *assert_thresh_dbm,
    bool set_threshold)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_ac_rxg_params_t *rxg_params = rxgcrs_info->rxg_params;
    phy_info_t * pi = rxgcrs_info->pi;
    bool suspend;
    int32 assert_local_dBm,  assert, deassert;
    int32 assert_mid, deassert_mid;
    int32 assert_edge, deassert_edge;
    uint8 adj = 0;
    uint8 mid_adj = 0;
    uint8 edge_adj = 0;
    /* For adc code 512, non-TINY maps 0.4;TINY maps 0.2; 43684 maps 0.3.
       so there is a 6dB and 3dB difference in formula,
       and ed logic in chip does not take care of it
    */
    if (TINY_RADIO(pi) &&
        (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))) {
        adj = 6;
    } else if (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
               !ACMAJORREV_128(pi->pubpi->phy_rev)) {
        adj = 3;
    }
    if (CHSPEC_IS80(pi->radio_chanspec)) {
        mid_adj =  (rxg_params->subband_edadj & 0xf0) >> 4;
        edge_adj = (rxg_params->subband_edadj &  0xf) >> 0;
    } else if (CHSPEC_IS160(pi->radio_chanspec)) {
        mid_adj =  (rxg_params->subband_edadj & 0xf000) >> 12;
        edge_adj = (rxg_params->subband_edadj &  0xf00) >> 8;
    }
    /* Set the EDCRS Assert and De-assert Threshold
    The de-assert threshold is set to 6dB lower then the assert threshold
    Accurate Formula:64*log2(round((10.^((THRESHOLD_dBm +65-30)./10).*50).*(2^9./0.4).^2))
    Simplified Accurate Formula: 64*(THRESHOLD_dBm + 75)/(10*log10(2)) + 832;
    Implemented Approximate Formula: 640000*(THRESHOLD_dBm + 75)/30103 + 832;
    */

    /* Suspend MAC */
    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend) wlc_phy_conditional_suspend(pi, &suspend);

    if (set_threshold == TRUE) {
        /* For adc code 512, non-TINY maps 0.4;TINY maps 0.2; 43684 maps 0.3.
        so there is a 6dB and 3dB difference in formula,
        and ed logic in chip does not take care of it
        */
        assert_local_dBm = *assert_thresh_dbm + adj;
        assert = (640000*(assert_local_dBm + 75) + 25045696)/30103;
        deassert = (640000*(assert_local_dBm + 69) + 25045696)/30103;

        assert_mid = (640000*(assert_local_dBm - mid_adj + 75) + 25045696)/30103;
        deassert_mid = (640000*(assert_local_dBm - mid_adj + 69) + 25045696)/30103;

        assert_edge = (640000*(assert_local_dBm - edge_adj + 75) + 25045696)/30103;
        deassert_edge = (640000*(assert_local_dBm - edge_adj + 69) + 25045696)/30103;

        if (ACREV_GE(pi->pubpi->phy_rev, 32)) {
            /* Set assert threshold */
            ACPHYREG_BCAST(pi, ed_crs20LAssertThresh00, assert);
            ACPHYREG_BCAST(pi, ed_crs20LAssertThresh10, assert);
            ACPHYREG_BCAST(pi, ed_crs20UAssertThresh00, assert);
            ACPHYREG_BCAST(pi, ed_crs20UAssertThresh10, assert);
            ACPHYREG_BCAST(pi, ed_crs20Lsub1AssertThresh00, assert);
            ACPHYREG_BCAST(pi, ed_crs20Lsub1AssertThresh10, assert);
            ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh00, assert);
            ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh10, assert);

            /* Set deassert threshold */
            ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh00, deassert);
            ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh10, deassert);
            ACPHYREG_BCAST(pi, ed_crs20UDeassertThresh00, deassert);
            ACPHYREG_BCAST(pi, ed_crs20UDeassertThresh10, deassert);
            ACPHYREG_BCAST(pi, ed_crs20Lsub1DeassertThresh00, deassert);
            ACPHYREG_BCAST(pi, ed_crs20Lsub1DeassertThresh10, deassert);
            ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh00, deassert);
            ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh10, deassert);

            if ((wlc_phy_ac_phycap_maxbw(pi) > BW_80MHZ)) {
                ACPHYREG_BCAST(pi, ed_crs20_4_AssertThresh00, assert);
                ACPHYREG_BCAST(pi, ed_crs20_4_AssertThresh10, assert);
                ACPHYREG_BCAST(pi, ed_crs20_5_AssertThresh00, assert);
                ACPHYREG_BCAST(pi, ed_crs20_5_AssertThresh10, assert);
                ACPHYREG_BCAST(pi, ed_crs20_6_AssertThresh00, assert);
                ACPHYREG_BCAST(pi, ed_crs20_6_AssertThresh10, assert);
                ACPHYREG_BCAST(pi, ed_crs20_7_AssertThresh00, assert);
                ACPHYREG_BCAST(pi, ed_crs20_7_AssertThresh10, assert);

                ACPHYREG_BCAST(pi, ed_crs20_4_DeassertThresh00, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_4_DeassertThresh10, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_5_DeassertThresh00, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_5_DeassertThresh10, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_6_DeassertThresh00, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_6_DeassertThresh10, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_7_DeassertThresh00, deassert);
                ACPHYREG_BCAST(pi, ed_crs20_7_DeassertThresh10, deassert);
            }
            if (CHSPEC_IS6G(pi->radio_chanspec)) {
                /*
                shifting ED threshold by 2dB for 20luu&20ull,
                since incumbent signal is located in their edge
                shifting ED threshold by 5dB for edge subband
                FIXME: add iovar/nvram to configure 2dB/5dB offset
                FIXME: need to be conditioned with LPI or country ALL
                */
                if (CHSPEC_IS80(pi->radio_chanspec)) {
                    //20LL
                    ACPHYREG_BCAST(pi, ed_crs20LAssertThresh00, assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LAssertThresh10, assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh00,
                        deassert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh10,
                        deassert_edge);
                    //20LU
                    ACPHYREG_BCAST(pi, ed_crs20UAssertThresh00, assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20UAssertThresh10, assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20UDeassertThresh00,
                        deassert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20UDeassertThresh10,
                        deassert_mid);
                    //20UL
                    ACPHYREG_BCAST(pi, ed_crs20Lsub1AssertThresh00,
                        assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Lsub1AssertThresh10,
                        assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Lsub1DeassertThresh00,
                        deassert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Lsub1DeassertThresh10,
                        deassert_mid);
                    //20UU
                    ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh00,
                        assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh10,
                        assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh00,
                        deassert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh10,
                        deassert_edge);
                } else if (CHSPEC_IS160(pi->radio_chanspec)) {
                    //20LLL
                    ACPHYREG_BCAST(pi, ed_crs20LAssertThresh00, assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LAssertThresh10, assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh00,
                        deassert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20LDeassertThresh10,
                        deassert_edge);
                    //20LUU
                    ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh00,
                        assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1AssertThresh10,
                        assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh00,
                        deassert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20Usub1DeassertThresh10,
                        deassert_mid);
                    //20ULL
                    ACPHYREG_BCAST(pi, ed_crs20_4_AssertThresh00, assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20_4_AssertThresh10, assert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20_4_DeassertThresh00,
                        deassert_mid);
                    ACPHYREG_BCAST(pi, ed_crs20_4_DeassertThresh10,
                        deassert_mid);
                    //20UUU
                    ACPHYREG_BCAST(pi, ed_crs20_7_AssertThresh00,
                        assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20_7_AssertThresh10,
                        assert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20_7_DeassertThresh00,
                        deassert_edge);
                    ACPHYREG_BCAST(pi, ed_crs20_7_DeassertThresh10,
                        deassert_edge);
                }
            }
        } else {
            WRITE_PHYREG(pi, ed_crs20LAssertThresh0, assert);
            WRITE_PHYREG(pi, ed_crs20LAssertThresh1, assert);
            WRITE_PHYREG(pi, ed_crs20UAssertThresh0, assert);
            WRITE_PHYREG(pi, ed_crs20UAssertThresh1, assert);
            WRITE_PHYREG(pi, ed_crs20Lsub1AssertThresh0, assert);
            WRITE_PHYREG(pi, ed_crs20Lsub1AssertThresh1, assert);
            WRITE_PHYREG(pi, ed_crs20Usub1AssertThresh0, assert);
            WRITE_PHYREG(pi, ed_crs20Usub1AssertThresh1, assert);

            /* Set the EDCRS De-assert Threshold */
            WRITE_PHYREG(pi, ed_crs20LDeassertThresh0, deassert);
            WRITE_PHYREG(pi, ed_crs20LDeassertThresh1, deassert);
            WRITE_PHYREG(pi, ed_crs20UDeassertThresh0, deassert);
            WRITE_PHYREG(pi, ed_crs20UDeassertThresh1, deassert);
            WRITE_PHYREG(pi, ed_crs20Lsub1DeassertThresh0, deassert);
            WRITE_PHYREG(pi, ed_crs20Lsub1DeassertThresh1, deassert);
            WRITE_PHYREG(pi, ed_crs20Usub1DeassertThresh0, deassert);
            WRITE_PHYREG(pi, ed_crs20Usub1DeassertThresh1, deassert);
        }
    } else {
        if (ACREV_GE(pi->pubpi->phy_rev, 32)) {
            assert = READ_PHYREG(pi, ed_crs20LAssertThresh00);
        } else {
            assert = READ_PHYREG(pi, ed_crs20LAssertThresh0);
        }
        assert_local_dBm = ((((assert - 832)*30103)) - 48000000)/640000;
        *assert_thresh_dbm = assert_local_dBm - adj;
        if (CHSPEC_IS6G(pi->radio_chanspec) &&
            (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec))) {
            *assert_thresh_dbm += edge_adj;
        }
    }

    /* Resume MAC */
    if (!suspend) wlc_phy_conditional_resume(pi, &suspend);
}

static bool wlc_phy_is_edcrs_high_acphy(phy_info_t *pi)
{
    bool edcrs = FALSE, suspend;
    uint loop_count = 100, percentage = 60, edcrs_high_count = 0;
    uint16 reg_val;
    uint8 i = 0;

    if (!pi->sh->up)
        return edcrs;

    /* suspend mac if haven't done so */
    wlc_phy_conditional_suspend(pi, &suspend);
    phy_utils_phyreg_enter(pi);

    /* Check EDCRS a few times to decide if the medium is busy.
       If medium is busy, skip PAPD this time around.
    */
    for (i = 0; i < loop_count; i++) {
        reg_val = READ_PHYREG(pi, ed_crs);
        if ((reg_val & 0xffff) > 0) {
            edcrs_high_count++;
        }
    }

    if (edcrs_high_count > (loop_count * percentage / 100))
        edcrs = TRUE;

    /* Resume MAC */
    phy_utils_phyreg_exit(pi);
    wlc_phy_conditional_resume(pi, &suspend);

    return edcrs;
}

#ifndef ATE_BUILD
#if defined(WLTEST) || defined(DBG_PHY_IOV) || defined(WFD_PHY_LL_DEBUG)
/*
 * This function has been implemented to get over a problem seen in the multi-DUT test scenario.
 * In this scenarios because of the Tx Cal going on in the other neighbor DUT the Noise
 * calibration in this DUT would get high noise numbers. To avoid this problem this IOVAR can
 * be used to trigger a noise cal at a DUT under known conditions (other DUTs are not doing
 * any calibration). The algo in the PHY averages the last 4 values received from the noise cal
 * and uses them to populate the value in the crsminpoweru0 register. In order to refresh the
 * value in this register to a good value the post processing function (phy_noise_sample_intr)
 * is triggered 4 times after doing a single cal at the PSM, so as to flush out the stale readings.
 */
static int
wlc_phy_noisecal_run_acphy(phy_type_rxgcrs_ctx_t *ctx, void *a, bool set)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = (phy_ac_rxgcrs_info_t *) ctx;
    phy_info_t *pi = rxgcrsi->pi;
    int status = 0, i;
    bool phywatchdog_override;
    bool force_crsmincal;
    bool trigger_crsmin_cal;

    uint8 enRx = 0;
    uint8 enTx = 0;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    BCM_REFERENCE(stf_shdata);
    /* Save and overwrite Rx chains */
    wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
        stf_shdata->hw_phyrxchain, stf_shdata->phytxchain);

    /* Backup the watchdog's flag. */
    phywatchdog_override = pi->phywatchdog_override;
    pi->phywatchdog_override = 0;

    /* Suspend the PSM */
    wlapi_suspend_mac_and_wait(pi->sh->physhim);

    /* Ensure no Noise cal in progress */
    status = R_REG(pi->sh->osh, D11_MACCOMMAND(pi));
    if (status & MCMD_BG_NOISE) {
        ASSERT(0);
        PHY_ERROR(("Fatal error. Aborting, PSM not done with previous noise cal.\n"));
        wlapi_enable_mac(pi->sh->physhim);
        return BCME_ERROR;
    }

#ifndef BCMQT_PHYLESS
    /* Start noise measurements. */
    OR_REG(pi->sh->osh, D11_MACCOMMAND(pi), MCMD_BG_NOISE);

    /* Let the PSM run to perform Noise Cal */
    wlapi_enable_mac(pi->sh->physhim);

    SPINWAIT(R_REG(pi->sh->osh, D11_MACCOMMAND(pi)) & MCMD_BG_NOISE,
        ACPHY_SPINWAIT_NOISE_CAL_STATUS);
    if (R_REG(pi->sh->osh, D11_MACCOMMAND(pi)) & MCMD_BG_NOISE) {
        PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR : Noise cal failed \n", __FUNCTION__));
        PHY_FATAL_ERROR(pi, PHY_RC_NOISE_CAL_FAILED);
    }
#endif /* BCMQT_PHYLESS */

    /* Backup CRS Min cal params */
    force_crsmincal = rxgcrsi->force_crsmincal;
    trigger_crsmin_cal = phy_ac_noise_get_data(pi->u.pi_acphy->noisei)->trigger_crsmin_cal;

    /* Calling it PHY_SIZE_NOISE_ARRAY times so as to take a fresh reading */
    for (i = 0; i < PHY_SIZE_NOISE_ARRAY; i++) {
        /* To set the phy reg to new cal value set the CRS Min cal params */
        rxgcrsi->force_crsmincal = TRUE;
        phy_ac_noise_set_trigger_crsmin_cal(pi->u.pi_acphy->noisei, TRUE);
        phy_noise_sample_intr(pi);
    }

    /* Restore the watchdog's flag. */
    pi->phywatchdog_override = phywatchdog_override;
    phy_ac_noise_set_trigger_crsmin_cal(pi->u.pi_acphy->noisei, trigger_crsmin_cal);
    rxgcrsi->force_crsmincal = force_crsmincal;

    /* Restore Rx chains */
    wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

    return BCME_OK;
}
#endif
#endif /* !ATE_BUILD */

void
phy_ac_rxgcrs_cal(phy_ac_rxgcrs_info_t *rxgcrsi)
{
#ifndef ATE_BUILD
    phy_info_t *pi = rxgcrsi->pi;
    /* XXX: Do the noise cal as first thing,
       for some reason doing it at the end casues bad noise value
    */
    if ((pi->cal_info->cal_phase_id == PHY_CAL_PHASE_IDLE) ||
        (pi->cal_info->cal_phase_id == PHY_CAL_PHASE_RXCAL)) {
        /* For phy_force_cal request, do crs cal request from outside this function call
         */
        if ((rxgcrsi->crsmincal_enable ||
             rxgcrsi->lesiscalecal_enable) &&
            !(pi->cal_info->phy_forcecal_request)) {

            bool save_phynoise_disable;

            rxgcrsi->force_crsmincal = TRUE;
            PHY_CAL(("%s : crsminpwr cal\n", __FUNCTION__));
            if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
                ACMAJORREV_33(pi->pubpi->phy_rev)) {
                phy_noise_sample_request_crsmincal(pi);
            } else {
                /* save and enable the noisecal schedule state  */
                save_phynoise_disable = phy_noise_sched_get(pi);
                phy_noise_sched_set(pi, PHY_CAL_MODE, FALSE);
                /* 43684 WAR for PER spikes (BCAWLAN-214570) when unrealistically
                 * low noise is reported by knoise (such as -107dBm or less)
                 * => knoise used low gain.
                 * Forcing low gain = high gain for knoise during crsmin sample req
                 */
                if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
                    phy_ac_rxgcrs_upd_knoise_shmem(pi, TRUE);
                }
                phy_noise_sample_request_crsmincal(pi);
                 /* restore the noisecal schedule state */
                phy_noise_sched_set(pi, PHY_CAL_MODE, save_phynoise_disable);
            }
        }
    }
#endif /* ATE_BUILD */
}

void
phy_ac_ovr_rxgain_noise_sample_request(phy_ac_rxgcrs_info_t *rxgcrsi, bool set_ovr)
{
    phy_info_t *pi = rxgcrsi->pi;

    if (rxgcrsi->crsmincal_enable && set_ovr) {
        bool save_phynoise_disable;

        rxgcrsi->force_crsmincal = TRUE;

        /* Temporary hack until we have ucode to force rxgain before noise measurement */
        /* We returnFromDeaf when ucode completes noise measurement and inturrups */
        wlc_phy_deaf_acphy(pi, TRUE);

        /* Noise cal is triggered by doing the forcecal hence ignore
            the crs status bit if Noise is measured when the CRS is high
        */
        pi->cal_info->ignore_crs_status = TRUE;
        if (rxgcrsi->fixed_gain_ncal != 0) {
            pi->cal_info->ignore_crs_status = TRUE;
            phy_ac_rxgcrs_enable_fixedgain_noisecal(rxgcrsi, 0x0001);
        }

        /* save and enable the noisecal schedule state  */
        save_phynoise_disable = phy_noise_sched_get(pi);
        phy_noise_sched_set(pi, PHY_CAL_MODE, FALSE);
        phy_noise_sample_request_crsmincal(pi);
         /* restore the noisecal schedule state */
        phy_noise_sched_set(pi, PHY_CAL_MODE, save_phynoise_disable);
    } else {
        wlc_phy_deaf_acphy(pi, FALSE);
    }
}

static int
phy_ac_rxgcrs_get_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 *ret_int_ptr)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    *ret_int_ptr = (int32)rxgcrs_info->desense_forced_dB;
    return BCME_OK;
}

static int
phy_ac_rxgcrs_set_rxdesens(phy_type_rxgcrs_ctx_t *ctx, int32 int_val)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;
    int8 negative = -1;
    int err = BCME_OK;

    if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
        ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
        wlapi_suspend_mac_and_wait(pi->sh->physhim);

        if (int_val < 0) int_val = 0;
        rxgcrs_info->desense_forced_dB = (uint8) int_val;
        wlc_phy_desense_apply_acphy(pi, TRUE);

        wlapi_enable_mac(pi->sh->physhim);
    } else if (!(ACPHY_ENABLE_FCBS_HWACI(pi))) {
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
        if (int_val <= 0) {
            /* disable phy_rxdesens and restore
             * default interference mode
             */
            rxgcrs_info->desense_forced_dB = 0;
            pi->sh->interference_mode_override = FALSE;
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                pi->sh->interference_mode = pi->sh->interference_mode_2G;
            } else {
                pi->sh->interference_mode = pi->sh->interference_mode_5G;
            }

#ifndef WLC_DISABLE_ACI
            /* turn off interference mode
             * before entering another mode
             */
            if (pi->sh->interference_mode != INTERFERE_NONE) {
                phy_noise_set_mode(pi->noisei, INTERFERE_NONE, TRUE);
            }
            if (!phy_noise_set_mode(pi->noisei, pi->sh->interference_mode, TRUE)) {
                err = BCME_BADOPTION;
            }
#endif /* !defined(WLC_DISABLE_ACI) */

            /* restore crsmincal automode, and force crsmincal */
            wlc_phy_force_crsmin_acphy(pi, &negative);
            phy_ac_rxgcrs_force_lesiscale(rxgcrs_info, &negative);

        } else {
            /* enable phy_rxdesens and disable interference mode
            * through override mode
            */
            pi->sh->interference_mode_override = TRUE;
            pi->sh->interference_mode_2G_override = INTERFERE_NONE;
            pi->sh->interference_mode_5G_override = INTERFERE_NONE;
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                pi->sh->interference_mode = pi->sh->interference_mode_2G_override;
            } else {
                pi->sh->interference_mode = pi->sh->interference_mode_5G_override;
            }
            phy_noise_set_mode(pi->noisei, INTERFERE_NONE, TRUE);

            /* disable crsmincal */
            rxgcrs_info->crsmincal_enable = FALSE;
            rxgcrs_info->lesiscalecal_enable = FALSE;

            /* apply desense */
            rxgcrs_info->desense_forced_dB = (uint8) int_val;
            wlc_phy_desense_apply_acphy(pi, TRUE);

        }
        wlapi_enable_mac(pi->sh->physhim);
    } else {
        err = BCME_UNSUPPORTED;
    }
    return err;
}

void
phy_ac_rxgcrs_lesi(phy_ac_rxgcrs_info_t *rxgcrsi, bool on, uint8 delta_halfdB)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
    uint8 max_rxchain = stf_shdata->hw_phyrxchain;
    const bool chspec_is20 = CHSPEC_IS20(pi->radio_chanspec);
    const bool chspec_is40 = CHSPEC_IS40(pi->radio_chanspec);
    const bool chspec_is80 = CHSPEC_IS80(pi->radio_chanspec);
    const bool chspec_is160 = CHSPEC_IS160(pi->radio_chanspec);

    uint8 upper, lower;
    uint8 low_regime_5g =  84;
    uint8 low_regime_rev129_5g = 80;
    uint8 low_regime_rev129_6g = 70;
    uint8 low_regime_rev130_5g = 83;
    uint8 low_regime_rev130_6g = 83;

    uint8 lsb, msb, delta = (delta_halfdB << 3);   // 16 ticks = 1dB
    uint16 val = 0;

    uint8 crs_det[4];
    uint8 crs_delta20P_th1[] = {0x44, 0x33};
    uint8 crs_delta20P_th2[] = {0x2a, 0x26};
    bool elna_board;

    int16 gainerr[PHY_CORE_MAX];
    bool srom_isempty = FALSE;
    uint8 core;

    // LESI not supported for this chip OR don't want to enable it
    if (!pi_ac->rxgcrsi->lesi_cap)
        return;

    /* If forced */
    if (pi_ac->rxgcrsi->lesi_ovrd != -1)
        on = (pi_ac->rxgcrsi->lesi_ovrd == 0) ? FALSE : TRUE;

    pi_ac->rxgcrsi->lesi_on = on;

    if (on) {
        MOD_PHYREG(pi, lesi_control, lesiFstrEn, 1);
        MOD_PHYREG(pi, lesi_control, lesiCrsEn, 1);

        /* 4357B0: help reduce high c4s1 PER floor randomly across chans
        * when LESI enabled
        */
        MOD_PHYREG(pi, lesiFstrControl3, cCrsFftInpAdj, chspec_is20 ? 0x0 : chspec_is40 ?
            0x1 : chspec_is80 ? 0x3 : 0x7);

        if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
            ACPHYREG_BCAST(pi, lesiFstrClassifierEqualizationFactor0_0, chspec_is20 ?
                0x2020 : chspec_is40 ? 0x2020 : chspec_is80 ? 0x2030 : 0x2020);
            ACPHYREG_BCAST(pi, lesiFstrClassifierEqualizationFactor1_0, chspec_is20 ?
                0x2020 : chspec_is40 ? 0x2020 : chspec_is80 ? 0x3020 : 0x2020);
        } else {
            ACPHYREG_BCAST(pi, lesiFstrClassifierEqualizationFactor0_0, chspec_is20 ?
                0x2020 : chspec_is40 ? 0x2020 : chspec_is80 ? 0x2030 : 0x2030);
            ACPHYREG_BCAST(pi, lesiFstrClassifierEqualizationFactor1_0, chspec_is20 ?
                0x2020 : chspec_is40 ? 0x2020 : chspec_is80 ? 0x3020 : 0x3020);
        }
        ACPHYREG_BCAST(pi, LesiFstrFdNoisePower0, chspec_is20 ? 0xaf : chspec_is40 ?
            0xaf : chspec_is80 ? 0x64 : 0x64);
        if (!ACMAJORREV_32(pi->pubpi->phy_rev)) {
            MOD_PHYREG(pi, lesiFstrControl4, lCrsFftOp1Adj, chspec_is20 ?
                0x1 : chspec_is40 ? 0x2 : chspec_is80 ? 0x4 : 0x8);
            MOD_PHYREG(pi, lesiFstrControl4, delayCmbCfo, chspec_is20 ?
                0x0 : chspec_is40 ? 0x1 : chspec_is80 ? 0x1 : 0x1);
        }

        if (ACMAJORREV_33(pi->pubpi->phy_rev)) {
            /* the default value caused the degradation on SGI for C1s4 */
            MOD_PHYREG(pi, lesiFstrControl5, lesi_sgi_hw_adj, 0x13);

            /* PER hump at low power regime siwthcing point at 2G band,
             * the WAR is to set fstrSwitchEn
             */
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 1);
                MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 0);
            }
        } else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            /* PER spike around lesi crs switching point in 2G band
            * tune LowPowerRegime thresholds as defaults turn on lesi at
            * higher rssi level than needed
            */

            ACPHYREG_BCAST(pi, LowPowerRegimeControlUpperThreshold0, 0x54);
            ACPHYREG_BCAST(pi, LowPowerRegimeControlLowerThreshold0, 0x4e);
        }

        if (ACMAJORREV_47_129_130_131(pi->pubpi->phy_rev)) {
            /* PER hump at low power regime switching point at 2G band,
             * the WAR is to set fstrSwitchEn.
             * Change LowPowerRegime for 5G for DFS radar tests.
             */
            if (CHSPEC_IS2G(pi->radio_chanspec)) {
                MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 1);
                MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 0);

                ACPHYREG_BCAST(pi, LowPowerRegimeControlUpperThreshold0, 0x4e);
                ACPHYREG_BCAST(pi, LowPowerRegimeControlLowerThreshold0, 0x48);
            } else {
                /* PER hump at low power regime switching point at 6G band,
                 * the WAR is to set fstrSwitchEn. FIXME: 6g 80MHz & 160MHz
                 * will be enabled later when threshold is tuned
                 */
                if (ACMAJORREV_47_129(pi->pubpi->phy_rev) &&
                    CHSPEC_IS6G(pi->radio_chanspec) &&
                    (chspec_is20 || chspec_is40)) {
                    MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 1);
                    MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 0);
                } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    if (RADIOREV(pi->pubpi->radiorev) < 2) {
                        if (CHSPEC_IS6G(pi->radio_chanspec) ||
                            (chspec_is20 || chspec_is160)) {
                            MOD_PHYREG(pi, lesiFstrControl4,
                                fstrSwitchEn, 1);
                            MOD_PHYREG(pi, lesiFstrControl4,
                                selLesiCstr, 0);
                        } else {
                            MOD_PHYREG(pi, lesiFstrControl4,
                                fstrSwitchEn, 0);
                            MOD_PHYREG(pi, lesiFstrControl4,
                                selLesiCstr, 1);
                        }
                    } else {
                        MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 1);
                        MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 0);
                    }
                } else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
                    MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 1);
                    MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 0);
                } else {
                    MOD_PHYREG(pi, lesiFstrControl4, fstrSwitchEn, 0);
                    MOD_PHYREG(pi, lesiFstrControl4, selLesiCstr, 1);
                }

                if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
                  upper = (CHSPEC_IS6G(pi->radio_chanspec) ? low_regime_rev129_6g :
                     low_regime_rev129_5g);
                  lower = upper - 6;
                } else if (ACMAJORREV_130(pi->pubpi->phy_rev) ||
                    ACMAJORREV_131(pi->pubpi->phy_rev)) {
                  upper = (CHSPEC_IS6G(pi->radio_chanspec) ? low_regime_rev130_6g :
                           low_regime_rev130_5g);
                  lower = upper - 3;
                } else {
                    upper = low_regime_5g;
                    lower = upper - 6;
                }

                if (ACMAJORREV_130(pi->pubpi->phy_rev) && chspec_is160) {
                    srom_isempty = wlc_phy_get_rxgainerr_phy(pi, gainerr);
                    if (!srom_isempty) {
                        FOREACH_CORE(pi, core) {
                            MOD_PHYREGCE(pi,
                                LowPowerRegimeControlUpperThreshold,
                                core, UpThre,
                                upper - (gainerr[core] >> 1));
                            MOD_PHYREGCE(pi,
                                LowPowerRegimeControlLowerThreshold,
                                core, LowThre,
                                lower - (gainerr[core] >> 1));
                        }
                    } else {
                        ACPHYREG_BCAST(pi,
                            LowPowerRegimeControlUpperThreshold0,
                            upper);
                        ACPHYREG_BCAST(pi,
                            LowPowerRegimeControlLowerThreshold0,
                            lower);
                    }
                } else {
                    ACPHYREG_BCAST(pi, LowPowerRegimeControlUpperThreshold0,
                        upper);
                    ACPHYREG_BCAST(pi, LowPowerRegimeControlLowerThreshold0,
                        lower);
                }
            }

            WRITE_PHYREG(pi, lesiP20FstrModeSwitchHiPower, 0x2a1);
            WRITE_PHYREG(pi, lesiP20FstrModeSwitchLoPower, 0x134);
            if (!ACMAJORREV_130(pi->pubpi->phy_rev)) {
                MOD_PHYREG(pi, lesiFstrModeSwitchLoPower,
                    fstrSwitchPwrLo1c, chspec_is20 ? 0x134 :
                    chspec_is40 ? 0x2e0 : chspec_is80 ? 0x640 : 0x134);
                MOD_PHYREG(pi, lesiFstrModeSwitchHiPower,
                    fstrSwitchPwrHi1c, chspec_is20 ? 0x2a1 :
                    chspec_is40 ? 0x380 : chspec_is80 ? 0x960 : 0x2a1);
            } else {
                if (RADIOREV(pi->pubpi->radiorev) < 2) {
                    MOD_PHYREG(pi, lesiFstrModeSwitchLoPower,
                        fstrSwitchPwrLo1c, chspec_is20 ? 0x134 :
                        chspec_is40 ? 0x2e0 : chspec_is80 ? 0x640 : 0x320);
                    MOD_PHYREG(pi, lesiFstrModeSwitchHiPower,
                        fstrSwitchPwrHi1c, chspec_is20 ? 0x2a1 :
                        chspec_is40 ? 0x380 : chspec_is80 ? 0x960 : 0x480);
                } else {
                    // 6715B0 and above
                    MOD_PHYREG(pi, lesiFstrModeSwitchLoPower,
                        fstrSwitchPwrLo1c, chspec_is20 ? 0x134 :
                        chspec_is40 ? 0x2e0 : chspec_is80 ? 0x2e0 : 0x27b);
                    MOD_PHYREG(pi, lesiFstrModeSwitchHiPower,
                        fstrSwitchPwrHi1c, chspec_is20 ? 0x2a1 :
                        chspec_is40 ? 0x380 : chspec_is80 ? 0x380 : 0x393);
                }
            }

            MOD_PHYREG(pi, lesiFstrControl0, winmf_1stPeak_Scl, 0x8);

            // For B0
            MOD_PHYREG(pi, lesi_control_2, crsHoldOffStMode, 1);
            MOD_PHYREG(pi, lesi_control_2, ignoreGainSettleForChEst1, 1);
            MOD_PHYREG(pi, lesiFstrBw160LeakageComp, lesiFstrleakageCompFactor, 0x10);
            MOD_PHYREG(pi, lesiFstrBw160LeakageComp, lesiFstrleakageCompEn, 1);
            MOD_PHYREG(pi, lesiFstrBw160LeakageComp, lesiCrsleakageCompFactor, 0x3a);
            MOD_PHYREG(pi, lesiFstrBw160LeakageComp, lesiCrsleakageCompEn, 1);
            if (chspec_is160 && !ACMAJORREV_131(pi->pubpi->phy_rev)) {
                // Incorrect default values for 160MHz
                if (!ACMAJORREV_130(pi->pubpi->phy_rev)) {
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor1_0,
                        0x2020);
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor0_2,
                        0x2020);
                } else {
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor0_0,
                        0x2030);
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor0_1,
                        0x2030);
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor1_2,
                        0x3020);
                    WRITE_PHYREG(pi, lesiFstrClassifierEqualizationFactor1_3,
                        0x3020);
                }
            }
            WRITE_PHYREG(pi, lesiCrsTypRxPowerPerCore20P, 0x150);
            WRITE_PHYREG(pi, lesiCrsHighRxPowerPerCore20P, 0x89a);
            WRITE_PHYREG(pi, lesiCrsMinRxPowerPerCore20P, 0x5);

            WRITE_PHYREG(pi, lesiCrsTypRxPowerPerCore, chspec_is20 ?
                0x150 : chspec_is40 ? 0x228 : chspec_is80 ? 0x308 : 0x418);
            WRITE_PHYREG(pi, lesiCrsHighRxPowerPerCore, chspec_is20 ?
                0x89a : chspec_is40 ? 0x9c1 : chspec_is80 ? 0xaa2 : 0xd8b);
            WRITE_PHYREG(pi, lesiCrsMinRxPowerPerCore, chspec_is20 ?
                0x5 : chspec_is40 ? 0x18 : chspec_is80 ? 0x2c : 0x5);

            MOD_PHYREG(pi, lesiFstrClassifyThreshold0, MaxScaleLowValue, 0x3);

            if (!ACMAJORREV_130(pi->pubpi->phy_rev)) {
                MOD_PHYREG(pi, lesiFstrClassifyThreshold0, MaxScaleHighValue, 0x5a);
                MOD_PHYREG(pi, lesiHTPktGain, LesiHTAgcPktgainWait, 0xc);
                MOD_PHYREG(pi, lesiHTPktGain, LesiHTAGCStrtAdjst, chspec_is20 ?
                    0x8 : chspec_is40 ? 0x18 : chspec_is80 ? 0x38 : 0x38);

                if (!ACMAJORREV_129(pi->pubpi->phy_rev))
                    MOD_PHYREG(pi, lesiFstrControl5, lesi_sgi_hw_adj, 0x9);
            } else {
                MOD_PHYREG(pi, lesiFstrClassifyThreshold0, MaxScaleHighValue, 0x80);
                MOD_PHYREG(pi, lesiHTPktGain, LesiHTAgcPktgainWait, 0x10);
                MOD_PHYREG(pi, lesiHTPktGain, LesiHTAGCStrtAdjst, chspec_is20 ?
                    0x14 : chspec_is40 ? 0x28 : chspec_is80 ? 0x50 : 0xa1);
                MOD_PHYREG(pi, lesiFstrControl5, lesi_sgi_hw_adj, 0x0);
                if (chspec_is20) {
                    MOD_PHYREG(pi, HTAGCWaitCounters, HTAgcStartWait, 0x24);
                }
            }

            MOD_PHYREG(pi, lesiFstrControl5, lCrsFftInpAdj, chspec_is20 ?
                9: chspec_is40 ? 0x13 : chspec_is80 ? 0x28 : 0x51);
            WRITE_PHYREG(pi, dc_fix_filter_config_lesi, chspec_is20 ?
                0x180d : chspec_is40 ? 0x3a1f : chspec_is80 ? 0x763f : 0xa85d);
            MOD_PHYREG(pi, lesiFstrControl3, lesi_sgi_adj, 0xa);

            /* CRDOT11ACPHY-2555:
            * lesi and bphy_pre_det set in aux block bphy pkt
            */
            if (pi->pubpi->slice == DUALMAC_AUX) {
                MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 0);
            }

            elna_board = (BF_ELNA_2G(pi_ac) || BF_ELNA_5G(pi_ac)) ? TRUE : FALSE;
            /* DC compensation, added in 6710A0 */
            /* 6715A0 has RTL fixes, therefore leaving it enabled for all configs
            *  by default via phyreg default values
            */
            if (ACMAJORREV_129(pi->pubpi->phy_rev) && elna_board) {
                MOD_PHYREG(pi, lesi_control_2, dcMeanCompensateEn, 1);
                MOD_PHYREG(pi, lesi_control_2, dcRotMeanCompensateEn, 0);
            } else if (ACMAJORREV_129(pi->pubpi->phy_rev) && !elna_board) {
                MOD_PHYREG(pi, lesi_control_2, dcMeanCompensateEn, 0);
                MOD_PHYREG(pi, lesi_control_2, dcRotMeanCompensateEn, 0);
            } else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
                if (RADIOREV(pi->pubpi->radiorev) < 2) {
                    MOD_PHYREG(pi, lesi_control_2, dcMeanCompensateEn, 0);
                    MOD_PHYREG(pi, lesi_control_2, dcRotMeanCompensateEn, 0);
                } else if (chspec_is20) {
                    MOD_PHYREG(pi, lesi_control_2, dcMeanCompensateEn, 1);
                    MOD_PHYREG(pi, lesi_control_2, dcRotMeanCompensateEn, 0);
                } else {
                    MOD_PHYREG(pi, lesi_control_2, dcMeanCompensateEn, 1);
                    MOD_PHYREG(pi, lesi_control_2, dcRotMeanCompensateEn, 1);
                }
            }
        } else {
            MOD_PHYREG(pi, lesiCrsTypRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                0x15b : chspec_is40 ? 0x228 : chspec_is80 ? 0x184 : 0x27e);
            MOD_PHYREG(pi, lesiCrsHighRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                0x76e : chspec_is40 ? 0x9c1 : chspec_is80 ? 0x551 : 0x64b);
            MOD_PHYREG(pi, lesiCrsMinRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                0x5 : chspec_is40 ? 0x18 : chspec_is80 ? 0x2c : 0x58);

            MOD_PHYREG(pi, lesiFstrControl0, winmf_1stPeak_Scl, 0x8);
            MOD_PHYREG(pi, lesiCrsTypRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                    0x15b : chspec_is40 ? 0x228 : 0x308);
            MOD_PHYREG(pi, lesiCrsHighRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                    0x76e : chspec_is40 ? 0x9c1 : 0xAA2);
            MOD_PHYREG(pi, lesiCrsMinRxPowerPerCore, PowerLevelPerCore, chspec_is20 ?
                    0x5 : chspec_is40 ? 0x18 : 0x4);
            MOD_PHYREG(pi, lesiFstrClassifyThreshold0, MaxScaleHighValue, 0x5a);
            MOD_PHYREG(pi, lesiFstrControl3, lCrsFftInpAdj, chspec_is20 ?
                0x9: chspec_is40 ? 0x13 : chspec_is80 ? 0x28 : 0x51);
            if (!ACMAJORREV_32(pi->pubpi->phy_rev)) {
                WRITE_PHYREG(pi, lesiFstrModeSwitchHiPower, chspec_is20 ?
                    0x239 : chspec_is40 ? 0x33d : chspec_is80 ? 0x211 : 0x308);
                WRITE_PHYREG(pi, lesiFstrModeSwitchLoPower, chspec_is20 ?
                    0xec : chspec_is40 ? 0x19d : chspec_is80 ? 0x13f : 0x239);
            }
        }

        /* **************** LESI DESENSE *************** */
        if (ACMAJORREV_47_129_130_131(pi->pubpi->phy_rev)) {
            crs_det[0] = chspec_is20 ? 0x41 : chspec_is40 ? 0x2d : chspec_is80 ?
            0x23 : 0x1a;
            crs_det[1] = chspec_is20 ? 0x30 : chspec_is40 ? 0x20 : chspec_is80 ?
            0x1b : 0x13;
            crs_det[2] = chspec_is20 ? 0x28 : chspec_is40 ? 0x1b : chspec_is80 ?
            0x17 : 0x11;
            crs_det[3] = chspec_is20 ? 0x23 : chspec_is40 ? 0x18 : chspec_is80 ?
            0x14 : 0x0f;
        } else {
            crs_det[0] = chspec_is20 ? 0x3b : chspec_is40 ? 0x2a : 0x1d;
            crs_det[1] = chspec_is20 ? 0x2a : chspec_is40 ? 0x1d : 0x15;
            crs_det[2] = chspec_is20 ? 0x22 : chspec_is40 ? 0x18 : 0x11;
            crs_det[3] = chspec_is20 ? 0x1d : chspec_is40 ? 0x15 : 0x0e;
        }
        lsb = delta + crs_det[0]; msb = delta + crs_det[1];
        val = (msb << 8) | lsb;
        WRITE_PHYREG(pi, lesiCrs1stDetThreshold_1, val);
        WRITE_PHYREG(pi, lesiCrs2ndDetThreshold_1, val);

        if (max_rxchain > 2) {
            lsb = delta + crs_det[2]; msb = delta + crs_det[3];
            val = (msb << 8) | lsb;
            WRITE_PHYREG(pi, lesiCrs1stDetThreshold_2, val);
            WRITE_PHYREG(pi, lesiCrs2ndDetThreshold_2, val);
        }

        if (ACMAJORREV_33(pi->pubpi->phy_rev) || ACMAJORREV_GE37(pi->pubpi->phy_rev)) {
            /* Increase the 20 primary crs detection thresold
               which cause the PER floor
            */
            lsb = delta + crs_delta20P_th1[0];
            msb = delta + crs_delta20P_th1[1];
            val = (msb << 8) | lsb;
            WRITE_PHYREG(pi, lesiCrs20P1stDetThreshold_1, val);
            WRITE_PHYREG(pi, lesiCrs20P2ndDetThreshold_1, val);

            if (max_rxchain > 2) {
                lsb = delta + crs_delta20P_th2[0];
                msb = delta + crs_delta20P_th2[1];
                val = (msb << 8) | lsb;
                WRITE_PHYREG(pi, lesiCrs20P1stDetThreshold_2, val);
                WRITE_PHYREG(pi, lesiCrs20P2ndDetThreshold_2, val);
            }
        }
    } else {
        MOD_PHYREG(pi, lesi_control, lesiFstrEn, 0);
        MOD_PHYREG(pi, lesi_control, lesiCrsEn, 0);
        if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
            if ((pi->pubpi->slice == DUALMAC_AUX ||
                ACMAJORREV_128(pi->pubpi->phy_rev)) &&
                CHSPEC_IS2G(pi->radio_chanspec))
                MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 1);
        }
    }

    if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
        MOD_PHYREG(pi, lesi_control, lesiFstrEn, 0);
        MOD_PHYREG(pi, lesi_control, lesiCrsEn, 0);
        pi_ac->rxgcrsi->lesi_on = FALSE;
    }
}

int
phy_ac_rxgcrs_iovar_get_lesi(phy_ac_rxgcrs_info_t *rxgcrsi, int32 *ret_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    *ret_val = 0;

    if (phy_ac_rxgcrs_get_cap_lesi(pi)) {
        *ret_val = (int32) (READ_PHYREG(pi, lesi_control) & 0x1);
    }

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_get_lesi_cap(phy_ac_rxgcrs_info_t *rxgcrsi, int32 *ret_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    *ret_val = phy_ac_rxgcrs_get_cap_lesi(pi) ? 1 : 0;
    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_get_lesi_ovrd(phy_ac_rxgcrs_info_t *rxgcrsi, int32 *ret_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    *ret_val = (int32) pi_ac->rxgcrsi->lesi_ovrd;

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_set_lesi_ovrd(phy_ac_rxgcrs_info_t *rxgcrsi, int32 set_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if ((set_val < -2) || (set_val > 1))
        return BCME_ERROR;

    if (!phy_ac_rxgcrs_get_cap_lesi(pi))
        return BCME_ERROR;

    pi_ac->rxgcrsi->lesi_ovrd = set_val;
    if (set_val == -1) {
        /* Auto Mode */
        wlc_phy_desense_apply_acphy(pi, pi_ac->rxgcrsi->total_desense->on);
    } else if (set_val == 0) {
        phy_ac_rxgcrs_lesi(rxgcrsi, FALSE, 0);
    } else if (set_val == 1) {
        phy_ac_rxgcrs_lesi(rxgcrsi, TRUE, 0);
    }

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_get_lesiscale_offset(phy_ac_rxgcrs_info_t *rxgcrsi, int8 band, int32 *ret_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if (band == 2) {
        *ret_val = (int32) pi_ac->rxgcrsi->lesiscale_2g;
    } else if (band == 5) {
        *ret_val = (int32) pi_ac->rxgcrsi->lesiscale_5g;
    }

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_set_lesiscale_offset(phy_ac_rxgcrs_info_t *rxgcrsi, int8 band, int32 set_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

    if (!phy_ac_rxgcrs_get_cap_lesi(pi))
        return BCME_ERROR;

    if (band == 2) {
        pi_ac->rxgcrsi->lesiscale_2g = set_val;
    } else if (band == 5) {
        pi_ac->rxgcrsi->lesiscale_5g = set_val;
    }

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_get_force_rxgain(phy_ac_rxgcrs_info_t *rxgcrsi,    int32 *ret_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    uint8 ovrd, core;

    *ret_val = 0;

    FOREACH_CORE(pi, core) {
      ovrd = READ_PHYREGFLDCE(pi, RfctrlOverrideGains, core, rxgain);
        *ret_val += (ovrd << core);
    }

    return BCME_OK;
}

// trtx[0], lna1[3:1], lna2[6:4], mixtia[10:7], lpf0[13:11], lpf1[16:14], dvga[21:17]
int
phy_ac_rxgcrs_iovar_set_force_rxgain(phy_ac_rxgcrs_info_t *rxgcrsi, int32 set_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    rxgain_t rxgain[PHY_CORE_MAX];
    rxgain_ovrd_t rxgain_ovrd[PHY_CORE_MAX];
    uint8 core;

    if (set_val == 0xffff) {
      // Remove Override
      FOREACH_CORE(pi, core) {
        rxgain_ovrd[core].rfctrlovrd = 0;
        rxgain_ovrd[core].rxgain = 0;
        rxgain_ovrd[core].rxgain2 = 0;
        rxgain_ovrd[core].lpfgain = 0;

        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, 0);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 0);
      }
      wlc_phy_rfctrl_override_rxgain_acphy(pi, 1, rxgain, rxgain_ovrd);
    } else {
      FOREACH_CORE(pi, core) {
        rxgain[core].trtx = set_val & 0x1;
        rxgain[core].lna1 = (set_val >> 1) & 0x7;
        rxgain[core].lna2 = (set_val >> 4) & 0x7;
        rxgain[core].mix  = (set_val >> 7) & 0xf;
        rxgain[core].lpf0 = (set_val >> 11) & 0x7;
        rxgain[core].lpf1 = (set_val >> 14) & 0x7;
        rxgain[core].dvga = (set_val >> 17) & 0xf;

        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_tx_pu, rxgain[core].trtx);
        MOD_PHYREGCE(pi, RfctrlIntc, core, tr_sw_rx_pu, !rxgain[core].trtx);
        MOD_PHYREGCE(pi, RfctrlIntc, core, override_tr_sw, 1);
      }
      wlc_phy_rfctrl_override_rxgain_acphy(pi, 0, rxgain, rxgain_ovrd);
    }

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_get_subband_edadj(phy_ac_rxgcrs_info_t *rxgcrsi, int32 *ret_val)
{
    phy_ac_rxg_params_t *rxg_params = rxgcrsi->rxg_params;
    *ret_val = rxg_params->subband_edadj;

    return BCME_OK;
}

int
phy_ac_rxgcrs_iovar_set_subband_edadj(phy_ac_rxgcrs_info_t *rxgcrsi, int32 set_val)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_ac_rxg_params_t *rxg_params = rxgcrsi->rxg_params;
    rxg_params->subband_edadj = set_val;
    wlc_phy_apply_edcrs_acphy(pi);
    return BCME_OK;
}

void phy_ac_rxgcrs_clean_noise_array(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    uint8 indx, i;
    uint8 phyrxchain = phy_stf_get_data(rxgcrsi->pi->stfi)->phyrxchain;

    BCM_REFERENCE(phyrxchain);

    FOREACH_ACTV_CORE(rxgcrsi->pi, phyrxchain, i) {
        for (indx = 0; indx < PHY_SIZE_NOISE_ARRAY; indx++) {
            rxgcrsi->phy_noise_pwr_array[indx][i] = 0;
        }
    }
    rxgcrsi->phy_noise_counter = 0;
}

static uint16
phy_ac_rxgcrs_sel_classifier(phy_type_rxgcrs_ctx_t *ctx, uint16 val)
{
    phy_info_t *pi = ((phy_ac_rxgcrs_info_t *) ctx)->pi;
    uint16 curr_ctl, new_ctl;
    bool suspend;
    uint16 mask = ACPHY_ClassifierCtrl_classifierSel_MASK(pi->pubpi->phy_rev);

    PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

    if (!pi->sh->clk)
        return BCME_NOCLK;

    suspend = !(R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
    if (!suspend) {
        wlapi_suspend_mac_and_wait(pi->sh->physhim);
    }
    /* Turn on/off classification (bphy, ofdm, and wait_ed), mask and
     * val are bit fields, bit 0: bphy, bit 1: ofdm, bit 2: wait_ed;
     * for types corresponding to bits set in mask, apply on/off state
     * from bits set in val.
     */
    curr_ctl = READ_PHYREG(pi, ClassifierCtrl);

    new_ctl = (curr_ctl & (~mask)) | (val & mask);

    WRITE_PHYREG(pi, ClassifierCtrl, new_ctl);

    if (!suspend) {
        wlapi_enable_mac(pi->sh->physhim);
    }
    return new_ctl;
}

#if defined(BCMDBG)
static void
phy_ac_rxgcrs_phydump_chanest(phy_type_rxgcrs_ctx_t *ctx, struct bcmstrbuf *b)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;
    uint16 num_rx, num_sts, num_tones, start_tone, max_tones;
    uint16 k, r, t, fftk;
    uint32 ch;
    uint16 ch_re_ma, ch_im_ma;
    uint8  ch_re_si, ch_im_si;
    int16  ch_re, ch_im;
    int8   ch_exp;
    uint8  dump_tones;

    num_rx = (uint8)PHYCORENUM(pi->pubpi->phy_corenum);
    if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
        ACMAJORREV_4(pi->pubpi->phy_rev)) {
        /* number of spatial streams supported */
        num_sts = 2;
    } else {
        num_sts = 4;
    }
    if (CHSPEC_IS40(pi->radio_chanspec)) {
        num_tones = 128;
#ifdef CHSPEC_IS80
    } else if (CHSPEC_IS80(pi->radio_chanspec)) {
        num_tones = 256;
#endif /* CHSPEC_IS80 */
    } else {
        num_tones = 64;
    }

    bcm_bprintf(b, "num_tones=%d\n", num_tones);

    if (!ACMAJORREV_129(pi->pubpi->phy_rev)) {
        /* Dump only 16 sub-carriers at a time */
        dump_tones = 16;
        /* Reset the dump counter */
        if (pi->phy_chanest_dump_ctr > (num_tones/dump_tones - 1))
            pi->phy_chanest_dump_ctr = 0;

        start_tone = pi->phy_chanest_dump_ctr * dump_tones;
        pi->phy_chanest_dump_ctr++;
        max_tones = 256;
    } else {
        dump_tones = num_tones;
        start_tone = 0;
        max_tones = 2048;
    }

    for (r = 0; r < num_rx; r++) {
        bcm_bprintf(b, "rx=%d\n", r);
        for (t = 0; t < num_sts; t++) {
            bcm_bprintf(b, "sts=%d\n", t);
            for (k = start_tone; k < (start_tone + dump_tones); k++) {
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_CHANEST(r), 1,
                                         t*max_tones + k, 32, &ch);

                if (ACMAJORREV_0(pi->pubpi->phy_rev) ||
                    (ACMAJORREV_2(pi->pubpi->phy_rev) &&
                    SW_ACMINORREV_2(pi->pubpi->phy_rev)) ||
                    ACMAJORREV_5(pi->pubpi->phy_rev) ||
                    ACMAJORREV_32(pi->pubpi->phy_rev) ||
                    ACMAJORREV_33(pi->pubpi->phy_rev) ||
                    (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
                    !ACMAJORREV_128(pi->pubpi->phy_rev))) {
                    /* Q11 FLP (12,12,6) */
                    ch_re_ma  = ((ch >> 18) & 0x7ff);
                    ch_re_si  = ((ch >> 29) & 0x001);
                    ch_im_ma  = ((ch >>  6) & 0x7ff);
                    ch_im_si  = ((ch >> 17) & 0x001);
                    ch_exp    = ((int8)((ch << 2) & 0xfc)) >> 2;
                    ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
                    ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;
                } else if (ACMAJORREV_2(pi->pubpi->phy_rev) ||
                        ACMAJORREV_4(pi->pubpi->phy_rev) ||
                        ACMAJORREV_128(pi->pubpi->phy_rev)) {
                    /* Q8 FLP (9,9,5) */
                    ch_re_ma  = ((ch >> 14) & 0xff);
                    ch_re_si  = ((ch >> 22) & 0x01);
                    ch_im_ma  = ((ch >>  5) & 0xff);
                    ch_im_si  = ((ch >> 13) & 0x01);
                    ch_exp    = ((int8)((ch << 3) & 0xf8)) >> 3;
                    ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
                    ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;
                } else {
                    /* Q13 FXP (14,14) */
                    ch_re_ma  = ((ch >> 14) & 0x3fff);
                    ch_im_ma  = ((ch >>  0) & 0x3fff);
                    ch_exp = 0;
                    ch_re = ((int16)((ch_re_ma << 2) & 0xfffc)) >> 2;
                    ch_im = ((int16)((ch_im_ma << 2) & 0xfffc)) >> 2;
                }
                fftk = ((k < num_tones/2) ? (k + num_tones/2) : (k - num_tones/2));

                bcm_bprintf(b, "chan(%d,%d,%d)=(%d+i*%d)*2^%d;\n",
                            r+1, t+1, fftk+1, ch_re, ch_im, ch_exp);
            }
        }
    }
}
#endif

#ifdef WLTEST
static void
phy_ac_rxgcrs_get_chanest(phy_type_rxgcrs_ctx_t *ctx, uint16 fftk, uint16 idx,
    int16 *ch_re, int16 *ch_im)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;
    uint32 ch, tbl_id;
    uint16 ch_re_ma, ch_im_ma;
    uint8  ch_re_si, ch_im_si;

    tbl_id = ACPHY_TBL_ID_CHANEST(idx);
    wlc_phy_table_read_acphy(pi, tbl_id, 1, fftk, 32, &ch);
    if (ACMAJORREV_0(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev) ||
        ACMAJORREV_32(pi->pubpi->phy_rev) ||
        ACMAJORREV_33(pi->pubpi->phy_rev) ||
        (ACMAJORREV_GE47(pi->pubpi->phy_rev) &&
        !ACMAJORREV_128(pi->pubpi->phy_rev))) {
        ch_re_ma  = ((ch >> 18) & 0x7ff);
        ch_re_si  = ((ch >> 29) & 0x001);
        ch_im_ma  = ((ch >>  6) & 0x7ff);
        ch_im_si  = ((ch >> 17) & 0x001);
    } else {
        ch_re_ma  = ((ch >> 14) & 0xff);
        ch_re_si  = ((ch >> 22) & 0x01);
        ch_im_ma  = ((ch >>  5) & 0xff);
        ch_im_si  = ((ch >> 13) & 0x01);
    }
    *ch_re = (ch_re_si > 0) ? -ch_re_ma : ch_re_ma;
    *ch_im = (ch_im_si > 0) ? -ch_im_ma : ch_im_ma;
}
#endif /* WLTEST */

#if defined(DBG_BCN_LOSS)
static int
phy_ac_rxgcrs_dump_phycal_rx_min(phy_type_rxgcrs_ctx_t *ctx, struct bcmstrbuf *b)
{
    phy_ac_rxgcrs_info_t *rxgcrs_info = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrs_info->pi;

    PHY_ERROR(("wl%d: %s: AC Phy not yet supported\n", pi->sh->unit, __FUNCTION__));
    return BCME_UNSUPPORTED;
}
#endif /* DBG_BCN_LOSS */

static int
phy_ac_rxgcrs_setup_fixedgain_noisecal(phy_ac_rxgcrs_info_t *rxgcrsi)
{
    phy_info_t *pi = rxgcrsi->pi;
    phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

    uint16 LPFgain, RFgain, LNARout, lna1idx, lna2idx, lna1_addr, LNAgm;
    uint8 lna1Rout, lna2Rout, core = 0;

    rxgcrsi->initGain_codeA = READ_PHYREGC(pi, InitGainCodeA, core);
    rxgcrsi->initGain_codeB = READ_PHYREGC(pi, InitGainCodeB, core);
        if (phy_ac_noise_get_data(pi->u.pi_acphy->noisei)->hw_aci_status == FALSE) {
            if (!IS_4364_3x3(pi)) {
                LPFgain  = (((rxgcrsi->initGain_codeB) & 0x700) >> 0x5) |
                (((rxgcrsi->initGain_codeB) & 0x70) >> 0x4);
                RFgain = rxgcrsi->initGain_codeA >> 1;
                lna1idx = RFgain & 0x7;
                lna2idx = (RFgain >> 3) & 0x7;
                lna1_addr = lna1idx +
                ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec);
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                    1, lna1_addr, 8, &lna1Rout);
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT,
                    1, (16 + lna2idx), 8, &lna2Rout);
                LNARout = (((lna2Rout >> 3) & 0xf) << 4) |
                    ((lna1Rout >> 3) & 0xf);
                LNAgm = TINY_RADIO(pi) ? (lna1Rout & 0x7) : lna1idx;
                /* add DVGA [13:10] and LNA1 settings [2:0] */
                RFgain &= 0x3F8;  /*  mask off LNA1 index */
                RFgain |= (((rxgcrsi->initGain_codeB) & 0xf000)>>2)  | LNAgm;
            } else {
                LPFgain  = (((rxgcrsi->initGain_codeB) & 0x700) >> 0x5) |
                    (((rxgcrsi->initGain_codeB) & 0x70) >> 0x4);
                RFgain = ((rxgcrsi->initGain_codeA) >> 1) & 0x3FF;
                lna1idx = RFgain & 0x7;
                lna2idx = (RFgain >> 3) & 0x7;
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1,
                    lna1idx, 8, &lna1Rout);
                wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1,
                    (16 + lna2idx), 8, &lna2Rout);
                LNARout = ((lna2Rout & 0xF0) | (lna1Rout >>4)) & 0xFF;
            }

            wlapi_bmac_write_shm(pi->sh->physhim, M_NCAL_ACTIVE_CORES(pi),
                0x100 | (stf_shdata->hw_phyrxchain & 0xff));

            phy_ac_rxgcrs_enable_fixedgain_noisecal(rxgcrsi, 0);
            wlapi_bmac_write_shm(pi->sh->physhim,
                M_NCAL_RFCTRLOVR_0(pi), 0x1722);
            wlapi_bmac_write_shm(pi->sh->physhim,
                M_NCAL_RFCTRLOVR_VAL(pi), 0xe);
            wlapi_bmac_write_shm(pi->sh->physhim, M_NCAL_RXGAIN_HI(pi), RFgain);
            wlapi_bmac_write_shm(pi->sh->physhim, M_NCAL_LPFGAIN_HI(pi), LPFgain);
            /* Writing the gains to the PHY registers */
            ACPHYREG_BCAST(pi, RfctrlCoreRXGAIN10, RFgain);
            ACPHYREG_BCAST(pi, RfctrlCoreRXGAIN20, LNARout);
            ACPHYREG_BCAST(pi, RfctrlCoreLpfGain0, LPFgain);
        }
    return BCME_OK;
}

static int
phy_ac_rxgcrs_enable_fixedgain_noisecal(phy_ac_rxgcrs_info_t *rxgcrsi, uint16 enable)
{
    phy_info_t *pi = rxgcrsi->pi;
    /* M_NCAL_CFG_BMP : Bit Map
     * C_FORCE_GAIN_BIT : 0
     * C_KNOISE_NBIT    : 1
     * C_IMMEDIATE_MEAS : 3
     * C_IMMEDIATE_MEAS2: 4
     * based on above map enable is bit-and with 0x0001
     */
    wlapi_bmac_write_shm(pi->sh->physhim,
        M_NCAL_CFG_BMP(pi), (enable & 0x0001));
    return BCME_OK;
}

void
phy_ac_rxgcrs_set_mclip_agc_en(phy_info_t *pi, bool en)
{
    uint8 val = en ? 1 : 0;

    // Enable/Disable Mclip
    MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcSdwTblEn, val);
    MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcEn, val);
    MOD_PHYREG(pi, clip_detect_normpwr_var_mux, mClpAgcSdwTblSel, 1);
}

bool
phy_ac_rxgcrs_get_cap_mclip_agc_en(phy_info_t *pi)
{
    return pi->u.pi_acphy->rxgcrsi->rxg_params->mclip_agc_en;
}

bool
phy_ac_rxgcrs_get_cap_lesi(phy_info_t *pi)
{
    return pi->u.pi_acphy->rxgcrsi->lesi_cap;
}

static void
phy_ac_rxgcrs_mclip_aci_fix(phy_info_t *pi, uint diff_th, uint16 idx_offset)
{
    uint8 core;

    ASSERT(ACPHY_ACI_MULTI_CLIP_EN(pi->pubpi->phy_rev) != INVALID_ADDRESS);

    /* Enable new feature see HW43684-501 for details */
    MOD_PHYREG(pi, ACI_MULTI_CLIP_EN, multi_clip_aci_en, 1);
    MOD_PHYREG(pi, ACI_Mitigation_InputSelect, aci_mitigation_changed_disable, 1);
    FOREACH_CORE(pi, core) {
        MOD_PHYREGCE(pi, ACI_MULTI_CLIP_CTRL, core, multi_clip_aci_diff_th, diff_th);
        MOD_PHYREGCE(pi, ACI_MULTI_CLIP_CTRL, core, multi_clip_aci_idx_offset, idx_offset);
    }
}

void phy_ac_rxcgrs_restore_force_crsmin(phy_info_t *pi)
{
    uint16 ac_th = pi->u.pi_acphy->rxgcrsi->force_crsminval;
    if (ac_th > 0) {
        PHY_CAL(("wl%d: %s Restore force_crs_min = %d\n",
                 pi->sh->unit, __FUNCTION__, ac_th));
        phy_ac_rxgcrs_set_crs_min_pwr(pi, ac_th, 0, 0, 0, 0, FALSE);
        pi->u.pi_acphy->rxgcrsi->crsmincal_enable = FALSE;
    }
}

/* Uses default INITgain values for knoise, without desense */
void
phy_ac_rxgcrs_upd_knoise_shmem(phy_info_t *pi, bool lowgain_eq_initgain)
{
    uint8 gain_indx[ACPHY_MAX_RX_GAIN_STAGES] = {0, 0, 0, 0, 0, 0, 0};
    uint8 initgain, core = 0, offset, gc_rout;
    uint8 logain_3dB_ticks = 8;  // 24 dBs loweer than initgain
    uint8 lna1, gm, tia, bq0, dvga, gc, rout;
    uint8 tia_lo, bq0_lo, dvga_lo;
    uint16 rfgain;
    phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
    uint8 mintia = pi_ac->rxgcrsi->rxgainctrl_params[core].gainbitstbl[TIA_ID][0];

    // Only supported for 28nm for now
    if (!IS_28NM_RADIO(pi) && !IS_16NM_RADIO(pi)) return;

    /* INITgain is HI for uCode knoise */
    initgain = ACPHY_INIT_GAIN_28NM;
    wlc_phy_rxgainctrl_encode_gain_acphy(pi, core, initgain, 0, 0, 0, gain_indx);
    lna1 = gain_indx[1];
    gm = gain_indx[2];
    tia = gain_indx[3];
    bq0 = gain_indx[4];
    dvga = gain_indx[6];

    offset = ACPHY_LNAROUT_BAND_OFFSET(pi, pi->radio_chanspec)    + lna1 +
        ACPHY_LNAROUT_CORE_RD_OFST(pi, core);
    wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_LNAROUT, 1, offset, 8, &gc_rout);
    gc = gc_rout & 7;
    rout = (gc_rout >> 3) & 0xf;
    rfgain = (dvga << 10) | (tia << 6) | (gm << 3) | gc;

    wlapi_bmac_write_shm(pi->sh->physhim, M_RXGAIN_HI(pi), rfgain);
    wlapi_bmac_write_shm(pi->sh->physhim, M_RXGAINOVR2_VAL(pi), rout);
    wlapi_bmac_write_shm(pi->sh->physhim, M_LPFGAIN_HI(pi), bq0);

    /* force low gain = high gain for knoise during crsmincal */
    if (lowgain_eq_initgain) {
        wlapi_bmac_write_shm(pi->sh->physhim, M_RXGAIN_LO(pi), rfgain);
        wlapi_bmac_write_shm(pi->sh->physhim, M_LPFGAIN_LO(pi), bq0);
        pi->cal_info->crsmincal_knoise_lowgain_ovr = TRUE;
        return;
    }

    /* INITgain - 24dB is LO for uCode (whatever possible) */

    // Reduce dvga first
    dvga_lo = dvga;
    if (logain_3dB_ticks > 0) {
        dvga_lo = MAX(0, dvga - logain_3dB_ticks);
        logain_3dB_ticks = logain_3dB_ticks - (dvga - dvga_lo);
    }

    // bq0 next
    bq0_lo = bq0;
    if (logain_3dB_ticks > 0) {
        bq0_lo = MAX(0, bq0 - logain_3dB_ticks);
        logain_3dB_ticks = logain_3dB_ticks - (bq0 - bq0_lo);
    }

    // tia last
    if (tia > 6) tia = 6;
    tia_lo = tia;
    if (logain_3dB_ticks > 0) {
        tia_lo = MAX(mintia, tia - logain_3dB_ticks);
        logain_3dB_ticks = logain_3dB_ticks - (tia - tia_lo);
    }

    rfgain = (dvga_lo << 10) | (tia_lo << 6) | (gm << 3) | gc;
    wlapi_bmac_write_shm(pi->sh->physhim, M_RXGAIN_LO(pi), rfgain);
    wlapi_bmac_write_shm(pi->sh->physhim, M_LPFGAIN_LO(pi), bq0_lo);
}

static int
phy_ac_rxgcrs_force_tr2t(phy_type_rxgcrs_ctx_t *ctx, bool force)
{
    phy_ac_rxgcrs_info_t *rxgcrsi = (phy_ac_rxgcrs_info_t *)ctx;
    phy_info_t *pi = rxgcrsi->pi;

    // Override using RfctrlInt.override_tr_sw is not working so reprogram FEMCTRL
    return wlc_phy_femctrl_table_tr2t(pi, force);
}

static int
phy_ac_calc_tia_gainlimit(phy_ac_rxgcrs_info_t *rxgcrsi, uint8 core, int8 pktgain)
{
    int8 elna_gain, lna1_gain, gain_app, tia_gainlimit = 0;

    if (rxgcrsi && rxgcrsi->fem_rxgains &&
            rxgcrsi->rxg_params && rxgcrsi->rxg_params->rftbls) {
        elna_gain = rxgcrsi->fem_rxgains[core].elna;
        lna1_gain = rxgcrsi->rxg_params->rftbls->lna12_gain_tbl[0][N_LNA12_GAINS - 2];
        gain_app = elna_gain + lna1_gain;
        tia_gainlimit = pktgain - gain_app;
        PHY_INFORM(("core=%d pktgain=%d, elna_gain=%d, lna1_gain=%d, gain_app=%d, "
            "tia_gainlim=%d\n", core, pktgain, elna_gain, lna1_gain, gain_app,
            tia_gainlimit));
    } else {
        ASSERT(0);
    }

    if (tia_gainlimit < 0) {
        tia_gainlimit = 0;
    }
    return tia_gainlimit;
}
