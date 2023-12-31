/*
 * ACPHY RADIO control module implementation
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
 * $Id: phy_ac_radio.c 810630 2022-04-11 22:09:25Z $
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <phy_dbg.h>
#include <phy_mem.h>
#include <bcm_math.h>
#include <phy_radio.h>
#include "phy_type_radio.h"
#include <phy_ac.h>
#include <phy_ac_radio.h>
#include <phy_ac_misc.h>
#include <phy_ac_tbl.h>
#include <phy_papdcal.h>
#include <phy_type_papdcal.h>
#include <phy_tpc.h>
#include <phy_utils_reg.h>
#include <wlc_phy_radio.h>
#include <wlc_phytbl_ac.h>
#include <wlc_radioreg_20693.h>
#include <wlc_radioreg_20698.h>
#include <wlc_radioreg_20704.h>
#include <wlc_radioreg_20707.h>
#include <wlc_radioreg_20708.h>
#include <wlc_radioreg_20709.h>
#include <wlc_radioreg_20710.h>
#include <wlc_phyreg_ac.h>
#include <sbchipc.h>
#include <hndpmu.h>
#include <wlc_phytbl_20693.h>
#include "wlc_phytbl_20698.h"
#include "wlc_phytbl_20704.h"
#include "wlc_phytbl_20707.h"
#include "wlc_phytbl_20708.h"
#include "wlc_phytbl_20709.h"
#include "wlc_phytbl_20710.h"
#include "phy_ac_tpc.h"
#include "phy_ac_vcocal.h"
#include "bcmutils.h"
#include "phy_ac_pllconfig_20698.h"
#include "phy_ac_pllconfig_20704.h"
#include "phy_ac_pllconfig_20707.h"
#include "phy_ac_pllconfig_20708.h"
#include "phy_ac_pllconfig_20709.h"
#include "phy_ac_pllconfig_20710.h"
#include "phy_utils_var.h"
#include "phy_rstr.h"
#include <phy_stf.h>
#include <wlc_phy_shim.h>

#ifdef ATE_BUILD
#include <wl_ate.h>
#endif
#include <phy_dbg.h>

#include <phy_rstr.h>
#include <bcmdevs.h>

#ifndef ALL_NEW_PHY_MOD
/* < TODO: all these are going away... */
#include <wlc_phy_int.h>
#include <wlc_phy_hal.h>
/* TODO: all these are going away... > */
#endif

#include <phy_ac_info.h>
#include <bcmotp.h>

typedef enum {
	TINY_RCAL_MODE0_OTP = 0,
	TINY_RCAL_MODE1_STATIC,
	TINY_RCAL_MODE2_SINGLE_CORE_CAL,
	TINY_RCAL_MODE3_BOTH_CORE_CAL
} acphy_tiny_rcal_modes_t;

#ifdef PHY_DUMP_BINARY
/* AUTOGENRATED by the tool : phyreg.py
 * These values cannot be in const memory since
 * the const area might be over-written in case of
 * crash dump
 */
phyradregs_list_t rad20693_majorrev2_registers[] = {
	{0x0,  {0x80, 0x0, 0x0, 0x89}},
	{0x8a,  {0x80, 0x0, 0x0, 0xe7}},
	{0x200,  {0x80, 0x0, 0x0, 0x89}},
	{0x28a,  {0x80, 0x0, 0x0, 0xe7}},
};
#endif /* PHY_DUMP_BINARY */

#define MAX_2069_RCCAL_WAITLOOPS (100 * 100)
#define NUM_2069_RCCAL_CAPS 3
#define ADCCAL_REPEAT_FAILURE 0

typedef struct {
	phy_ac_radio_info_t info;
	phy_ac_radio_data_t data;
} phy_ac_radio_mem_t;

typedef struct {
	uint16 gi;
	uint16 g21;
	uint16 g32;
	uint16 g43;
	uint16 r12;
	uint16 r34;
	uint16 gff1;
	uint16 gff2;
	uint16 gff3;
	uint16 gff4;
	uint16 g11;
	uint16 ri3;
	uint16 g54;
	uint16 g65;
} tiny_adc_tuning_array_t;

/* local functions */
static uint wlc_phy_cap_cal_status_rail(phy_info_t *pi, uint8 rail, uint8 core);
static void phy_ac_radio_std_params_attach(phy_ac_radio_info_t *info);
static void wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(phy_info_t *pi);
static void phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on);
static void phy_ac_radio_on(phy_type_radio_ctx_t *ctx);
static void phy_ac_radio_init_lpmode(phy_ac_radio_info_t *ri);
static int phy_ac_radio_get_valid_chanvec(phy_type_radio_ctx_t *ctx, chanspec_band_t band,
	chanvec_t *valid_ch);
#ifdef PHY_DUMP_BINARY
static int phy_ac_radio_getlistandsize(phy_type_radio_ctx_t *ctx, phyradregs_list_t **radreglist,
	uint16 *radreglist_sz);
#endif
#if (defined(BCMDBG) && defined(DBG_PHY_IOV)) || defined(BCMDBG_PHYDUMP)
static int phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b);
#else
#define phy_ac_radio_dump NULL
#endif

/* 20693 tuning Table related */
static void wlc_phy_radio20693_pll_tune(phy_info_t *pi, const void *chan_info_pll,
	uint8 core);
static void wlc_phy_radio20693_rffe_tune(phy_info_t *pi, const void *chan_info_rffe,
	uint8 core);
static void wlc_phy_radio20693_pll_tune_wave2(phy_info_t *pi, const void *chan_info_pll,
	uint8 core);
static void wlc_phy_radio20693_rffe_rsdb_wr(phy_info_t *pi, uint8 core, uint8 reg_type,
	uint16 reg_val, uint16 reg_off);
static void wlc_phy_radio20693_set_logencore_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_afeclkdiv_12g_sel(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core);
static void wlc_phy_radio20693_afe_clk_div_mimoblk_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode);
static void wlc_phy_radio20693_set_logen_mimosrcdes_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint16 mac_mode);
static void wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(phy_info_t *pi,
	uint8 core, uint8 chan);
static void wlc_phy_set_regtbl_on_pwron_acphy(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm1_lpopt(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm1_lpopt_restore(phy_info_t *pi);
static void wlc_phy_radio20693_mimo_cm23_lp_opt(phy_info_t *pi, uint8 coremask);
static void wlc_phy_radio20693_mimo_cm23_lp_opt_restore(phy_info_t *pi);
static void wlc_phy_radio20693_afecal(phy_info_t *pi);

static void
wlc_phy_radio20698_adc_cap_cal(phy_info_t *pi, uint8 adc);
static void
wlc_phy_radio20704_adc_cap_cal(phy_info_t *pi);
static void
wlc_phy_radio20707_adc_cap_cal(phy_info_t *pi);
static void
wlc_phy_radio20709_adc_cap_cal(phy_info_t *pi);
static void
wlc_phy_radio20710_adc_cap_cal(phy_info_t *pi, uint8 adc);
static void
wlc_phy_radio20698_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20704_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20707_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20708_afe_cal_setup(phy_info_t *pi, uint8 core, uint8 num_adc_rails);
static void
wlc_phy_radio20709_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20710_adc_cap_cal_setup(phy_info_t *pi, uint8 core);
static void
wlc_phy_radio20698_adc_cap_cal_parallel(phy_info_t *pi, uint8 core,
	uint8 adc, uint16 *timeout);
static void
wlc_phy_radio20704_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout);
static void
wlc_phy_radio20707_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout);
static void
wlc_phy_radio20709_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout);
static void
wlc_phy_radio20710_adc_cap_cal_parallel(phy_info_t *pi, uint8 core,
	uint8 adc, uint16 *timeout);
static void
wlc_phy_radio20698_apply_adc_cal_result(phy_info_t *pi, uint8 core, uint8 adc);
static void
wlc_phy_radio20698_tiadc_cal(phy_info_t *pi);
static void
wlc_phy_radio20708_tiadc_cal(phy_info_t *pi);
static void
wlc_phy_radio20698_tiadc_cal_parallel(phy_info_t *pi, uint8 core);
static uint8
wlc_phy_radio20708_tiadc_cal_parallel(phy_info_t *pi, uint8 coremask);
static void
wlc_phy_radio20698_loopback_adc_dac(phy_info_t *pi);
static void
wlc_phy_radio20708_loopback_adc_dac(phy_info_t *pi, uint8 core_num);
static void
wlc_phy_radio20698_hssmpl_buffer_setup(phy_info_t *pi, uint16 p, uint16 q);
static void
wlc_phy_radio20698_hs_loopback(phy_info_t *pi, bool swap_ab, uint16 DepthCount);
static void
wlc_phy_radio20698_adc_offset_gain_cal(phy_info_t *pi);
static void
wlc_phy_radio20708_adc_offset_gain_cal(phy_info_t *pi);
static void
wlc_phy_radio20708_afe_cal_pllreg_save(phy_info_t *pi, bool save);
static void
wlc_phy_radio20708_afe_cal_main(phy_info_t *pi);

/* Cleanup Chanspec */
static void chanspec_setup_radio_20693(phy_info_t *pi);
static void chanspec_setup_radio_2069(phy_info_t *pi);
static void chanspec_setup_radio_20698(phy_info_t *pi);
static void chanspec_setup_radio_20704(phy_info_t *pi);
static void chanspec_setup_radio_20707(phy_info_t *pi);
static void chanspec_setup_radio_20708(phy_info_t *pi);
static void chanspec_setup_radio_20709(phy_info_t *pi);
static void chanspec_setup_radio_20710(phy_info_t *pi);
static void chanspec_tune_radio_20693(phy_info_t *pi);
static void chanspec_tune_radio_2069(phy_info_t *pi);
static void chanspec_tune_radio_20698(phy_info_t *pi);
static void chanspec_tune_radio_20704(phy_info_t *pi);
static void chanspec_tune_radio_20707(phy_info_t *pi);
static void chanspec_tune_radio_20708(phy_info_t *pi);
static void chanspec_tune_radio_20709(phy_info_t *pi);
static void chanspec_tune_radio_20710(phy_info_t *pi);

static void wlc_phy_chanspec_radio2069_setup(phy_info_t *pi, const void *chan_info,
        uint8 toggle_logen_reset);
static void wlc_phy_chanspec_radio20704_setup(phy_info_t *pi, chanspec_t chanspec,
        uint8 toggle_logen_reset, uint8 logen_mode);
static void wlc_phy_chanspec_radio20707_setup(phy_info_t *pi, chanspec_t chanspec,
        uint8 toggle_logen_reset, uint8 logen_mode);
static void wlc_phy_chanspec_radio20709_setup(phy_info_t *pi, chanspec_t chanspec,
        uint8 toggle_logen_reset, uint8 logen_mode);
static void wlc_phy_chanspec_radio20710_setup(phy_info_t *pi, chanspec_t chanspec,
        uint8 toggle_logen_reset, uint8 logen_mode);
static void wlc_phy_logen_2g_sync(phy_info_t *pi, bool p1c_mode);

#define RADIO20693_TUNE_REG_WR_SHIFT 0
#define RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT 1
/* Define affinities for each core
for 4349A0:
core 0's affinity: "2g"
core 1's affinity: "5g"
*/
#define RADIO20693_CORE0_AFFINITY 2
#define RADIO20693_CORE1_AFFINITY 5

/* Tuning reg type */
#define RADIO20693_TUNEREG_TYPE_2G 2
#define RADIO20693_TUNEREG_TYPE_5G 5
#define RADIO20693_TUNEREG_TYPE_NONE 0

#define RADIO20693_NUM_PLL_TUNE_REGS 32
/* Rev5,7 & 6,8 */
#define RADIO20693_NUM_RFFE_TUNE_REGS 3

#define BCM4349_UMC_FAB_ID 5
#define BCM4349_TSMC_FAB_ID 2
#define UMC_FAB_RCAL_VAL 8
#define TSMC_FAB_RCAL_VAL 11

static const uint8 rffe_tune_reg_map_20693[RADIO20693_NUM_RFFE_TUNE_REGS] = {
	RADIO20693_TUNEREG_TYPE_2G,
	RADIO20693_TUNEREG_TYPE_5G,
	RADIO20693_TUNEREG_TYPE_2G
};

/* TIA LUT tables to be used in wlc_tiny_tia_config() */
static const uint8 tiaRC_tiny_8b_20[]= { /* LUT 0--51 (20 MHz) */
	0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xff,
	0xb7, 0xb5, 0x97, 0x81, 0xe5, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x14, 0x1d, 0x28, 0x34, 0x34, 0x34,
	0x34, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint8 tiaRC_tiny_8b_40[]= { /* LUT 0--51 (40 MHz) */
	0xff, 0xe1, 0xb9, 0x81, 0x5d, 0xff, 0xad, 0x82,
	0x6d, 0x5d, 0x4c, 0x41, 0x73, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x16, 0x16, 0x1b, 0x1d, 0x1d, 0x1f,
	0x20, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x4c,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint8 tiaRC_tiny_8b_80[]= { /* LUT 0--51 (80 MHz) */
	0xc2, 0x86, 0x5d, 0xff, 0xbe, 0x88, 0x5f, 0x43,
	0x37, 0x2e, 0x26, 0x20, 0x39, 0x00, 0x00, 0x00,
	0x0b, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0x0d,
	0x0d, 0x07, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00,
	0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x4c,
	0x5b, 0x6c, 0x80, 0x80
};

static const uint16 tiaRC_tiny_16b_20[]= { /* LUT 52--82 (20 MHz) */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00b5, 0x0080, 0x005a,
	0x0040, 0x002d, 0x0020, 0x0017, 0x0000, 0x0100, 0x00b5, 0x0080,
	0x005b, 0x0040, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0080, 0x001f, 0x1fe0, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_20_rem[]= { /* LUT 79--82 (20 MHz) */
	0x001f, 0x1fe0, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_20_rem_war[]= { /* LUT 79--82 (20 MHz) */
	0x001f, 0x1fe0, 0xc500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80[]= { /* LUT 52--82 (80 MHz) */
	0x0000, 0x0000, 0x0000, 0x016b, 0x0100, 0x00b6, 0x0080, 0x005a,
	0x0040, 0x002d, 0x0020, 0x0017, 0x0000, 0x0100, 0x00b5, 0x0080,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0080, 0x0007, 0x1ff8, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80_rem[]= { /* LUT 79--82 (80 MHz) */
	0x0007, 0x1ff8, 0xf500, 0x00ff
};

static const uint16 tiaRC_tiny_16b_80_rem_war[]= { /* LUT 79--82 (80 MHz) */
	0x0007, 0x1ff8, 0xc500, 0x00ff
};

static const uint16 *tiaRC_tiny_16b_40 = tiaRC_tiny_16b_20;  /* LUT 52--78 (40 MHz) (no LUT 67) */
static const uint16 *tiaRC_tiny_16b_40_rem = tiaRC_tiny_16b_20_rem;  /* LUT 79--82 (40 MHz) */

/* Coefficients generated by 47xxtcl/rgphy/20691/ */
/* lpf_tx_coefficient_generator/filter_tx_tiny_generate_python_and_tcl.py */
static const uint16 lpf_g10[7][15] = {
	{1188, 1527, 1866, 2206, 2545, 2545, 1188, 1188,
	1188, 1188, 1188, 1188, 1188, 1188, 1188},
	{3300, 4242, 5185, 6128, 7071, 7071, 3300, 3300,
	3300, 3300, 3300, 3300, 3300, 3300, 3300},
	{16059, 16059, 16059, 17294, 18529, 18529, 9882,
	1976, 2470, 3088, 3953, 4941, 6176, 7906, 12353},
	{24088, 24088, 25941, 31500, 37059, 37059, 14823,
	2964, 3705, 4632, 5929, 7411, 9264, 11859, 18529},
	{29647, 32118, 34589, 42001, 49412, 49412, 19765,
	3705, 4941, 6176, 7411, 9882, 12353, 14823, 24706},
	{32941, 36236, 39530, 42824, 46118, 49412, 19765,
	4117, 4941, 6588, 8235, 9882, 13176, 16470, 26353},
	{10706, 10706, 10706, 11529, 12353, 12353, 6588,
	1317, 1647, 2058, 2635, 3294, 4117, 5270, 8235}
};
static const uint16 lpf_g12[7][15] = {
	{1882, 1922, 1866, 1752, 1606, 1275, 2984, 14956,
	11880, 9436, 7495, 5954, 4729, 3756, 2370},
	{5230, 5341, 5185, 4868, 4461, 3544, 8289, 41544,
	33000, 26212, 20821, 16539, 13137, 10435, 6584},
	{24872, 19757, 15693, 13424, 11425, 9075, 24258, 24316,
	24144, 23972, 24374, 24201, 24029, 24432, 24086},
	{37309, 29635, 25351, 24452, 22850, 18151, 36388, 36474,
	36216, 35959, 36561, 36302, 36044, 36648, 36130},
	{44360, 38172, 32654, 31496, 29433, 23379, 46870, 44045,
	46648, 46318, 44150, 46759, 46428, 44254, 46538},
	{49288, 43066, 37319, 32113, 27471, 23379, 46870, 48939,
	46648, 49406, 49055, 46759, 49523, 49172, 49640},
	{16581, 13171, 10462, 8949, 7616, 6050, 16172, 16210,
	16096, 15981, 16249, 16134, 16019, 16288, 16057}
};
static const uint16 lpf_g21[7][15] = {
	{1529, 1497, 1542, 1643, 1793, 2257, 965, 192, 242,
	305, 384, 483, 609, 766, 1215},
	{4249, 4160, 4285, 4565, 4981, 6270, 2681, 534, 673,
	847, 1067, 1343, 1691, 2129, 3375},
	{6135, 7723, 9723, 11367, 13356, 16814, 6290, 6275,
	6320, 6365, 6260, 6305, 6350, 6245, 6335},
	{9202, 11585, 13543, 14041, 15025, 18916, 9435, 9413,
	9480, 9548, 9391, 9458, 9525, 9368, 9503},
	{13760, 15990, 18693, 19380, 20738, 26108, 13023, 13858,
	13085, 13178, 13825, 13054, 13147, 13793, 13116},
	{22016, 25197, 29078, 33791, 39502, 46415, 23152, 22173,
	23262, 21964, 22121, 23207, 21912, 22068, 21860},
	{4090, 5149, 6482, 7578, 8904, 11209, 4193, 4183, 4213,
	4243, 4173, 4203, 4233, 4163, 4223}
};
static const uint16 lpf_g11[7] = {994, 2763, 12353, 18529, 17470, 23293, 8235};
static const uint16 g_passive_rc_tx[7] = {62, 172, 772, 1158, 1544, 2058, 514};
static const uint16 biases[7] = {24, 48, 96, 96, 128, 128, 96};
static const int8 g_index1[15] = {0, 1, 2, 3, 4, 5, -2, -9, -8, -7, -6, -5, -4, -3, -1};

static uint8 wlc_phy_radio20693_tuning_get_access_info(phy_info_t *pi, uint8 core, uint8 reg_type);
static uint8 wlc_phy_radio20693_tuning_get_access_type(phy_info_t *pi, uint8 core, uint8 reg_type);

static void wlc_phy_radio20693_minipmu_pwron_seq(phy_info_t *pi);
static void wlc_phy_radio20693_pmu_override(phy_info_t *pi);
static void wlc_phy_radio20693_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20698_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20704_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20707_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20708_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20709_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20710_xtal_pwrup(phy_info_t *pi);
static void wlc_phy_radio20693_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20693_pmu_pll_config(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy_20693(phy_info_t *pi);
static void wlc_phy_switch_radio_acphy(phy_info_t *pi, bool on);
static void wlc_phy_radio2069_mini_pwron_seq_rev16(phy_info_t *pi);
static void wlc_phy_radio2069_mini_pwron_seq_rev32(phy_info_t *pi);
static void wlc_phy_radio2069_pwron_seq(phy_info_t *pi);
static void wlc_phy_tiny_radio_pwron_seq(phy_info_t *pi);
static void wlc_phy_radio2069_rcal(phy_info_t *pi);
static void wlc_phy_radio2069_rccal(phy_info_t *pi);
static void wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi);
static void WLBANDINITFN(wlc_phy_static_table_download_acphy)(phy_info_t *pi);
static void wlc_phy_radio_tiny_rcal(phy_info_t *pi, acphy_tiny_rcal_modes_t mode);
static void wlc_phy_radio_tiny_rcal_wave2(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio_tiny_rccal(phy_ac_radio_info_t *ri);
static void wlc_phy_radio_tiny_rccal_wave2(phy_ac_radio_info_t *ri);
static void wlc_phy_radio20698_minipmu_pwron_seq(phy_info_t *pi);
#ifdef MINIPMU_RESOURCE_20704
static void wlc_phy_radio20704_minipmu_pwron_seq(phy_info_t *pi);
#endif
#ifdef MINIPMU_RESOURCE_20707
static void wlc_phy_radio20707_minipmu_pwron_seq(phy_info_t *pi);
#endif
#ifdef MINIPMU_RESOURCE_20708
static void wlc_phy_radio20708_minipmu_pwron_seq(phy_info_t *pi);
#endif
#ifdef MINIPMU_RESOURCE_20709
static void wlc_phy_radio20709_minipmu_pwron_seq(phy_info_t *pi);
#endif
#ifdef MINIPMU_RESOURCE_20710
static void wlc_phy_radio20710_minipmu_pwron_seq(phy_info_t *pi);
#endif

static void wlc_phy_radio20698_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20704_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20707_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20708_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20709_pwron_seq_phyregs(phy_info_t *pi);
static void wlc_phy_radio20710_pwron_seq_phyregs(phy_info_t *pi);

int8 wlc_phy_28nm_radio_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20698_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20704_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20707_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20708_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20709_minipmu_cal(phy_info_t *pi);
static void wlc_phy_radio20710_minipmu_cal(phy_info_t *pi);

static void wlc_phy_radio20698_r_cal(phy_info_t *pi, uint8 mode);
static bool wlc_phy_radio20704_r_cal(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio20707_r_cal(phy_info_t *pi, uint8 mode);
static bool wlc_phy_radio20708_r_cal(phy_info_t *pi, uint8 mode);
static bool wlc_phy_radio20709_r_cal(phy_info_t *pi, uint8 mode);
static bool wlc_phy_radio20710_r_cal(phy_info_t *pi, uint8 mode);
static void wlc_phy_radio20698_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20704_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20707_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20708_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20709_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20710_rc_cal(phy_info_t *pi);
static void wlc_phy_radio20698_rffe_tune(phy_info_t *pi, chanspec_t chanspec,
		const chan_info_radio20698_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20704_rffe_tune(phy_info_t *pi,
		const chan_info_radio20704_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20707_rffe_tune(phy_info_t *pi,
		const chan_info_radio20707_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20708_rffe_tune(phy_info_t *pi,
		const chan_info_radio20708_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20709_rffe_tune(phy_info_t *pi,
		const chan_info_radio20709_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20710_rffe_tune(phy_info_t *pi,
		const chan_info_radio20710_rffe_t *chan_info_rffe, uint8 core);
static void wlc_phy_radio20698_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20704_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20707_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20708_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20708_txadc_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20709_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20710_txdac_bw_setup(phy_info_t *pi);
static void wlc_phy_radio20698_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq,
	uint8 logen_mode);
static void wlc_phy_radio20704_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq,
	uint8 logen_mode);
static void wlc_phy_radio20707_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq,
		uint8 logen_mode);
static void wlc_phy_radio20708_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq,
		uint8 logen_mode);
static void wlc_phy_radio20709_upd_band_related_reg(phy_info_t *pi, uint8 logen_mode);
static void wlc_phy_radio20710_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq,
	uint8 logen_mode);
static void wlc_phy_radio20698_pmu_pll_pwrup(phy_info_t *pi, uint8 pll);
static void wlc_phy_radio20704_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20707_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20708_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20709_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20710_pmu_pll_pwrup(phy_info_t *pi);
static void wlc_phy_radio20698_wars(phy_info_t *pi);
static void wlc_phy_radio20704_wars(phy_info_t *pi);
static void wlc_phy_radio20707_wars(phy_info_t *pi);
static void wlc_phy_radio20708_wars(phy_info_t *pi);
static void wlc_phy_radio20709_wars(phy_info_t *pi);
static void wlc_phy_radio20710_wars(phy_info_t *pi);
static void wlc_phy_radio20698_refclk_en(phy_info_t *pi);
static void wlc_phy_radio20698_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20704_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20707_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20708_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20709_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_radio20710_upd_prfd_values(phy_info_t *pi);
static void wlc_phy_set_clb_femctrl_static_settings(phy_info_t *pi);

#ifdef RADIO_HEALTH_CHECK
static bool phy_ac_radio_check_pll_lock(phy_type_radio_ctx_t *ctx);
#endif /* RADIO_HEALTH_CHECK */
/* query radio idcode */
static uint32
_phy_ac_radio_query_idcode(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 idcode;

	idcode = phy_ac_radio_query_idcode(pi);

	return idcode;
}

static void phy_ac_radio_nvram_attach(phy_info_t *pi, phy_ac_radio_info_t *radioi);

void wlc_phy_radio20698_powerup_RFP1(phy_info_t *pi, bool pwrup);
void wlc_phy_radio20698_pu_rx_core(phy_info_t *pi, uint core, uint fc, bool restore);

void wlc_phy_radio20708_powerup_RFPll(phy_info_t *pi, uint core, bool pwrup);
void wlc_phy_radio20708_pu_rx_core(phy_info_t *pi, uint core, uint fc, bool restore);

/* micro for loading 20708 tuning table */
#define RADIO_TUNING_REGS(type, core, regnm, fldnm)	\
	(CHSPEC_IS6G(pi->radio_chanspec) ? \
		chan_info_rffe->u.val_6G.type##core##_##regnm##_##fldnm : \
	(CHSPEC_IS5G(pi->radio_chanspec) ? \
		chan_info_rffe->u.val_5G.type##core##_##regnm##_##fldnm : \
		chan_info_rffe->u.val_2G.type##core##_##regnm##_##fldnm))

#define LOAD_RF_TUNING_REG_20708(pllcore, regnm, fldnm)	\
	if (pllcore == 0) { \
		MOD_RADIO_REG_20708(pi, regnm, 0, fldnm, RADIO_TUNING_REGS(RF, 0, regnm, fldnm)); \
		MOD_RADIO_REG_20708(pi, regnm, 1, fldnm, RADIO_TUNING_REGS(RF, 1, regnm, fldnm)); \
		MOD_RADIO_REG_20708(pi, regnm, 2, fldnm, RADIO_TUNING_REGS(RF, 2, regnm, fldnm)); \
		MOD_RADIO_REG_20708(pi, regnm, 3, fldnm, RADIO_TUNING_REGS(RF, 3, regnm, fldnm)); \
	} else {	\
		MOD_RADIO_REG_20708(pi, regnm, 3, fldnm, RADIO_TUNING_REGS(RF, 3, regnm, fldnm)); \
	}

#define LOAD_RFP_TUNING_REG_20708(core, regnm, fldnm)	\
	if (core == 0) { \
		MOD_RADIO_PLLREG_20708(pi, regnm, 0, fldnm, \
		RADIO_TUNING_REGS(RFP, 0, regnm, fldnm)); \
	} else { \
		MOD_RADIO_PLLREG_20708(pi, regnm, 1, fldnm, \
		RADIO_TUNING_REGS(RFP, 1, regnm, fldnm)); \
	}

/* Register/unregister ACPHY specific implementation to common layer. */
phy_ac_radio_info_t *
BCMATTACHFN(phy_ac_radio_register_impl)(phy_info_t *pi, phy_ac_info_t *aci,
	phy_radio_info_t *ri)
{
	phy_ac_radio_info_t *info;
	acphy_pmu_core1_off_radregs_t *pmu_c1_off_info_orig = NULL;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = NULL;
	phy_type_radio_fns_t fns;

	PHY_TRACE(("%s\n", __FUNCTION__));

	/* allocate all storage in once */
	if ((info = phy_malloc(pi, sizeof(phy_ac_radio_mem_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc failed\n", __FUNCTION__));
		goto fail;
	}

	if ((pmu_c1_off_info_orig =
		phy_malloc(pi, sizeof(acphy_pmu_core1_off_radregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc pmu_c1_off_info_orig failed\n", __FUNCTION__));
		goto fail;
	}
	if ((pmu_lp_opt_orig =
		phy_malloc(pi, sizeof(acphy_pmu_mimo_lp_opt_radregs_t))) == NULL) {
		PHY_ERROR(("%s: phy_malloc pmu_lp_opt_orig failed\n", __FUNCTION__));
		goto fail;
	}

	info->pi = pi;
	info->aci = aci;
	info->ri = ri;
	info->data = &(((phy_ac_radio_mem_t *) info)->data);

	info->pmu_c1_off_info_orig = pmu_c1_off_info_orig;
	info->pmu_lp_opt_orig = pmu_lp_opt_orig;
	pmu_lp_opt_orig->is_orig = FALSE;

	/* By default, use 5g pll for 2g will be disabled */
	info->data->use_5g_pll_for_2g = (uint8)PHY_GETINTVAR_DEFAULT(pi, rstr_use5gpllfor2g, 0);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		pi->rcal_value = UMC_FAB_RCAL_VAL;
	} else {
		/* falling back to some value */
		pi->rcal_value = UMC_FAB_RCAL_VAL;
	}

	/* Populate PLL configuration table */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
		info->pll_conf_20698 = phy_ac_radio20698_populate_pll_config_tbl(pi);
		if (info->pll_conf_20698 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20698_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
		info->pll_conf_20704 = phy_ac_radio20704_populate_pll_config_tbl(pi);
		if (info->pll_conf_20704 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20704_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
		info->pll_conf_20707 = phy_ac_radio20707_populate_pll_config_tbl(pi);
		if (info->pll_conf_20707 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20707_populate_pll_config_tbl failed\n",
				__FUNCTION__));
			goto fail;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {
		info->maincore_on_pll1 = 0;
		info->pll_conf_20708 = phy_ac_radio20708_populate_pll_config_tbl(pi);
		if (info->pll_conf_20708 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20708_populate_pll_config_tbl failed\n",
				__FUNCTION__));
			goto fail;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {
		info->pll_conf_20709 = phy_ac_radio20709_populate_pll_config_tbl(pi);
		if (info->pll_conf_20709 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20709_populate_pll_config_tbl failed\n",
				__FUNCTION__));
			goto fail;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
		info->pll_conf_20710 = phy_ac_radio20710_populate_pll_config_tbl(pi);
		if (info->pll_conf_20710 == NULL) {
			PHY_ERROR(("%s: phy_ac_radio20710_populate_pll_config_tbl failed\n",
					__FUNCTION__));
			goto fail;
		}
	}

	/* Read srom params from nvram */
	phy_ac_radio_nvram_attach(pi, info);

	phy_ac_radio_std_params_attach(info);

	/* make sure the radio is off until we do an "up" */
	phy_ac_radio_switch(info, OFF);

	/* Register PHY type specific implementation */
	bzero(&fns, sizeof(fns));
	fns.ctrl = phy_ac_radio_switch;
	fns.on = phy_ac_radio_on;
#ifdef PHY_DUMP_BINARY
	fns.getlistandsize = phy_ac_radio_getlistandsize;
#endif
	fns.dump = phy_ac_radio_dump;
	fns.id = _phy_ac_radio_query_idcode;
#ifdef RADIO_HEALTH_CHECK
	fns.pll_lock = phy_ac_radio_check_pll_lock;
#endif /* RADIO_HEALTH_CHECK */
	fns.get_valid_chanvec = phy_ac_radio_get_valid_chanvec;

	fns.ctx = info;

	phy_ac_radio_init_lpmode(info);

	phy_radio_register_impl(ri, &fns);

	return info;

fail:
	if (pmu_c1_off_info_orig != NULL) {
		phy_mfree(pi, pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));
	}
	if (pmu_lp_opt_orig != NULL) {
		phy_mfree(pi, pmu_lp_opt_orig, sizeof(*pmu_lp_opt_orig));
	}
	if (info != NULL) {
		phy_mfree(pi, info, sizeof(phy_ac_radio_mem_t));
	}
	return NULL;
}

void
BCMATTACHFN(phy_ac_radio_unregister_impl)(phy_ac_radio_info_t *info)
{
	phy_info_t *pi;
	phy_radio_info_t *ri;

	ASSERT(info);
	pi = info->pi;
	ri = info->ri;

	phy_radio_unregister_impl(ri);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
		phy_ac_radio20698_populate_pll_config_mfree(pi, info->pll_conf_20698);
		info->pll_conf_20698 = NULL;
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
		phy_ac_radio20704_populate_pll_config_mfree(pi, info->pll_conf_20704);
		info->pll_conf_20704 = NULL;
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
		phy_ac_radio20707_populate_pll_config_mfree(pi, info->pll_conf_20707);
		info->pll_conf_20707 = NULL;
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {
		phy_ac_radio20708_populate_pll_config_mfree(pi, info->pll_conf_20708);
		info->pll_conf_20708 = NULL;
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {
		phy_ac_radio20709_populate_pll_config_mfree(pi, info->pll_conf_20709);
		info->pll_conf_20709 = NULL;
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
		phy_ac_radio20710_populate_pll_config_mfree(pi, info->pll_conf_20710);
		info->pll_conf_20710 = NULL;
	}

	phy_mfree(pi, info->pmu_c1_off_info_orig, sizeof(acphy_pmu_core1_off_radregs_t));
	phy_mfree(pi, info->pmu_lp_opt_orig, sizeof(acphy_pmu_mimo_lp_opt_radregs_t));
	phy_mfree(pi, info, sizeof(phy_ac_radio_mem_t));
}

static void
BCMATTACHFN(phy_ac_radio_std_params_attach)(phy_ac_radio_info_t *radioi)
{
	radioi->data->dac_mode = 1;
	radioi->prev_subband = 15;
	radioi->data->acphy_4335_radio_pd_status = 0;
	radioi->data->rccal_gmult = 128;
	radioi->data->rccal_gmult_rc = 128;
	radioi->data->rccal_cmult_rc = 128;
	radioi->data->rccal_dacbuf = 12;
	/* AFE */
	radioi->afeRfctrlCoreAfeCfg10 = READ_PHYREG(radioi->pi, RfctrlCoreAfeCfg10);
	radioi->afeRfctrlCoreAfeCfg20 = READ_PHYREG(radioi->pi, RfctrlCoreAfeCfg20);
	radioi->afeRfctrlOverrideAfeCfg0 = READ_PHYREG(radioi->pi, RfctrlOverrideAfeCfg0);
	/* Radio RX */
	radioi->rxRfctrlCoreRxPus0 = READ_PHYREG(radioi->pi, RfctrlCoreRxPus0);
	radioi->rxRfctrlOverrideRxPus0 = READ_PHYREG(radioi->pi, RfctrlOverrideRxPus0);
	/* Radio TX */
	radioi->txRfctrlCoreTxPus0 = READ_PHYREG(radioi->pi, RfctrlCoreTxPus0);
	radioi->txRfctrlOverrideTxPus0 = READ_PHYREG(radioi->pi, RfctrlOverrideTxPus0);

	/* {radio, rfpll, pllldo}_pu = 0 */
	radioi->radioRfctrlCmd = READ_PHYREG(radioi->pi, RfctrlCmd);
	radioi->radioRfctrlCoreGlobalPus = READ_PHYREG(radioi->pi, RfctrlCoreGlobalPus);
	radioi->radioRfctrlOverrideGlobalPus = READ_PHYREG(radioi->pi, RfctrlOverrideGlobalPus);

	/* 4349A0: priority flags for 80p80 and RSDB modes
	   these flags indicate which core's tuning settings takes
	   precedence in conflict scenarios
	 */
	radioi->crisscross_priority_core_80p80 = 0;
	radioi->crisscross_priority_core_rsdb = 0;

	/* find out if criss-cross is active from nvram.
	 FIXME: for now, hardcoding it till nvram param gets fixed
	 */
	radioi->is_crisscross_actv = 0;
	radioi->data->acphy_force_lpvco_2G = 0;

	radioi->acphy_lp_mode = 1;
	radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
	radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
	radioi->data->dac_clk_x2_mode = 1;
	radioi->wbcal_afediv_ratio = -1;
}

/* switch radio on/off */
static void
phy_ac_radio_switch(phy_type_radio_ctx_t *ctx, bool on)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;

	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
		(RADIOMAJORREV(pi) != 3) && (pi->radio_is_on == TRUE) && (!on)) {
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			wlc_phy_radio20693_mimo_lpopt_restore(pi);
		}
	}

	/* sync up soft_radio_state with hard_radio_state */
	pi->radio_is_on = FALSE;

	PHY_TRACE(("wl%d: %s %d Phymode: %x\n", pi->sh->unit,
		__FUNCTION__, on, phy_get_phymode(pi)));

	wlc_phy_switch_radio_acphy(pi, on);
}

/* turn radio on */
static void
WLBANDINITFN(phy_ac_radio_on)(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_ac_info_t *aci = info->aci;
	phy_info_t *pi = aci->pi;

	/* To make sure that chanspec_set_acphy doesn't get called twice during init */
	phy_ac_chanmgr_get_data(aci->chanmgri)->init_done = FALSE;

	/* update phy_corenum and phy_coremask */
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		phy_ac_update_phycorestate(pi);
	}

	phy_ac_radio_switch(ctx, ON);

	/* Init regs/tables only once that do not get reset on phy_reset */
	wlc_phy_set_regtbl_on_pwron_acphy(pi);
}

/* Internal data api between ac modules */
phy_ac_radio_data_t *
phy_ac_radio_get_data(phy_ac_radio_info_t *radioi)
{
	return radioi->data;
}

struct pll_config_20698_tbl_s *
phy_ac_radio_get_20698_pll_config(phy_ac_radio_info_t *radioi)
{
	return radioi->pll_conf_20698;
}

/* query radio idcode */
uint32
phy_ac_radio_query_idcode(phy_info_t *pi)
{
	uint32 b0, b1;

	W_REG(pi->sh->osh, D11_radioregaddr(pi), (uint16)0);
#ifdef __mips__
	(void)R_REG(pi->sh->osh, D11_radioregaddr(pi));
#endif
	b0 = (uint32)R_REG(pi->sh->osh, D11_radioregdata(pi));
	W_REG(pi->sh->osh, D11_radioregaddr(pi), (uint16)1);
#ifdef __mips__
	(void)R_REG(pi->sh->osh, D11_radioregaddr(pi));
#endif
	b1 = (uint32)R_REG(pi->sh->osh, D11_radioregdata(pi));

#ifdef BCMRADIOREV
	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_ERROR(("%s: radiorev 0x%x, override to 0x%x\n", __FUNCTION__, b0, BCMRADIOREV));
		b0 = BCMRADIOREV;
	}
#endif	/* BCMRADIOREV */

	/* For ACPHY (starting with 4360A0), address 0 has the revid and
	   address 1 has the devid
	*/
	return (b0 << 16) | b1;
}

/* parse radio idcode and write to pi->pubpi */
void
phy_ac_radio_parse_idcode(phy_info_t *pi, uint32 idcode)
{
	PHY_TRACE(("%s: idcode 0x%08x\n", __FUNCTION__, idcode));

	pi->pubpi->radioid = (idcode & IDCODE_ACPHY_ID_MASK) >> IDCODE_ACPHY_ID_SHIFT;
	pi->pubpi->radiorev = (idcode & IDCODE_ACPHY_REV_MASK) >> IDCODE_ACPHY_REV_SHIFT;

	/* ACPHYs do not use radio ver. This param is invalid */
	pi->pubpi->radiover = 0;
}

void
wlc_phy_lp_mode(phy_ac_radio_info_t *ri, int8 lp_mode)
{
	return;
}

static void
BCMATTACHFN(phy_ac_radio_init_lpmode)(phy_ac_radio_info_t *ri)
{
	ri->lpmode_2g = ACPHY_LPMODE_NONE;
	ri->lpmode_5g = ACPHY_LPMODE_NONE;

	if (ACMAJORREV_4(ri->pi->pubpi->phy_rev)) {
		switch (BF3_ACPHY_LPMODE_2G(ri->aci)) {
			case 1:
				ri->lpmode_2g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 0:
			default:
				ri->lpmode_2g = ACPHY_LPMODE_NONE;
		}

		switch (BF3_ACPHY_LPMODE_5G(ri->aci)) {
			case 1:
				ri->lpmode_5g = ACPHY_LPMODE_LOW_PWR_SETTINGS_1;
				break;
			case 0:
			default:
				ri->lpmode_5g = ACPHY_LPMODE_NONE;
		}
	}
}

void
acphy_set_lpmode(phy_info_t *pi, acphy_lp_opt_levels_t lp_opt_lvl)
{
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		/* 4349 related power optimizations */
		bool for_2g = ((ri->lpmode_2g != ACPHY_LPMODE_NONE) &&
			(CHSPEC_IS2G(pi->radio_chanspec)));
		bool for_5g = ((ri->lpmode_5g != ACPHY_LPMODE_NONE) &&
			(CHSPEC_ISPHY5G6G(pi->radio_chanspec)));
		phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

		switch (lp_opt_lvl) {
		case ACPHY_LP_RADIO_LVL_OPT:
		{
			if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
					(phy_get_phymode(pi) == PHYMODE_MIMO) &&
					((for_2g == TRUE) || (for_5g == TRUE))) {
				uint8 new_rxchain = stf_shdata->phyrxchain;
				uint8 old_rxchain = phy_ac_chanmgr_get_data
					(ri->aci->chanmgri)->phyrxchain_old;
				uint8 to_cm1_rx = (new_rxchain == 1) &&
					((old_rxchain == 2) || (old_rxchain == 3));
				uint8 rx_cm1_to_cm1 = (new_rxchain == 1) && (old_rxchain == 1);

			PHY_TRACE(("[%s]chains: newrx:%d oldrx: %d newtx:%d both_txrxchain:%d\n",
				__FUNCTION__, stf_shdata->phyrxchain,
				phy_ac_chanmgr_get_data
				(ri->aci->chanmgri)->phyrxchain_old,
				stf_shdata->phytxchain, phy_ac_chanmgr_get_data
				(ri->aci->chanmgri)->both_txchain_rxchain_eq_1));

				wlc_phy_radio20693_mimo_lpopt_restore(pi);
				if (to_cm1_rx || rx_cm1_to_cm1) {
					wlc_phy_radio20693_mimo_cm1_lpopt(pi);
				} else {
					wlc_phy_radio20693_mimo_cm23_lp_opt(pi, new_rxchain);
				}
			}
		}
		break;
		case ACPHY_LP_CHIP_LVL_OPT:
			break;
		case ACPHY_LP_PHY_LVL_OPT:
			if ((for_5g == TRUE) && (stf_shdata->phyrxchain != 2)) {
				MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 1);
				MOD_PHYREG(pi, CRSMiscellaneousParam,
					b_over_ag_falsedet_en, 0);
			} else {
				MOD_PHYREG(pi, CRSMiscellaneousParam, bphy_pre_det_en, 0);
				MOD_PHYREG(pi, CRSMiscellaneousParam,
					b_over_ag_falsedet_en, 1);
			}
			/* JIRA: CRDOT11ACPHY-848. Recommendation from RTL team:
			 * Because of RTL issue, some logic related to this table is
			 * in un-initialized state and is consuming current. Just reading
			 * the table (it should be the last operation performed on this
			 *  table) will fix this.
			 */
			if (!((phy_get_phymode(pi) == PHYMODE_RSDB) &&
				(phy_get_current_core(pi) == 1))) {
				uint16 val;
				wlc_phy_table_read_acphy(pi, ACPHY_TBL_ID_BFECONFIG2X2TBL,
					1, 0, 16, &val);
				BCM_REFERENCE(val);
			}
			/*
			if {[hwaccess] == $def(hw_jtag)} {
				jtag w 0x18004492 2 0x0
				jtag w 0x18001492 2 0x2
			}
			*/

			WRITE_PHYREG(pi, FFTSoftReset, 0x2);
			break;
		default:
			break;
		}

	}
}

int
wlc_phy_get_recip_LO_div_phase(phy_info_t *pi)
{
	if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
		return wlc_phy_get_recip_LO_div_phase_20704(pi);
	} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
		return wlc_phy_get_recip_LO_div_phase_20710(pi);
	} else {
		return 0;
	}
}

int
wlc_phy_get_recip_LO_div_phase_20704(phy_info_t *pi)
{
	int core, tmp, LO_phase_diff;
	int ncores = 2;
	uint8 lodivstate[2] = {0, 0};

	/* Add check for supported chip revisions
	** If not supported, return a 0 phase
	*/
	if (!ACMAJORREV_51(pi->pubpi->phy_rev))
		return 0;

	for (core = 0; core < ncores; core++) {
		/* Power up Logen div phase detectors */
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 0);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd, 0);

		tmp = READ_RADIO_REG_20704(pi, SPARE_CFG13, core);
		lodivstate[core] = (tmp >> 15) & 0x1;

		/* Power down Logen div phase detectors */
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd, 1);
	}

	if (lodivstate[0] != lodivstate[1]) {
		LO_phase_diff = 180;
	} else {
		LO_phase_diff = 0;
	}

	return LO_phase_diff;
}

int
wlc_phy_get_recip_LO_div_phase_20710(phy_info_t *pi)
{
	int core, LO_phase_diff;
	int ncores = 2;
	uint8 lodivstate[2] = {0, 0};

	/* Add check for supported chip revisions
	** If not supported, return a 0 phase
	*/
	if (!ACMAJORREV_131(pi->pubpi->phy_rev))
		return 0;

	for (core = 0; core < ncores; core++) {
		/* Power up Logen div phase detectors */
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 0);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd, 0);

		lodivstate[core] =
			READ_RADIO_REGFLD_20710(pi, RF, LOGEN_CORE_REG5, core, logen_2g_phase_flag);

		/* Power down Logen div phase detectors */
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd, 1);
	}

	if (lodivstate[0] != lodivstate[1]) {
		LO_phase_diff = 180;
	} else {
		LO_phase_diff = 0;
	}

	return LO_phase_diff;
}

void
wlc_phy_force_lpvco_2G(phy_info_t *pi, int8 force_lpvco_2G)
{
	return;
}

/* Proc to power on dac_clocks though override */
bool wlc_phy_poweron_dac_clocks(phy_info_t *pi, uint8 core, uint16 *orig_dac_clk_pu,
	uint16 *orig_ovr_dac_clk_pu)
{
	uint16 dacpwr;

	dacpwr = READ_RADIO_REGFLD_TINY(pi, TX_DAC_CFG1, core, DAC_pwrup);

	if (dacpwr == 0) {
		*orig_dac_clk_pu = READ_RADIO_REGFLD_TINY(pi, CLK_DIV_CFG1, core,
				dac_clk_pu);
		*orig_ovr_dac_clk_pu = READ_RADIO_REGFLD_TINY(pi, CLK_DIV_OVR1, core,
				ovr_dac_clk_pu);
		MOD_RADIO_REG_TINY(pi, CLK_DIV_CFG1, core, dac_clk_pu, 1);
		MOD_RADIO_REG_TINY(pi, CLK_DIV_OVR1, core, ovr_dac_clk_pu, 1);
	}

	return (dacpwr == 0);
}

/* Proc to resotre dac_clock_pu and the corresponding ovrride registers */
void wlc_phy_restore_dac_clocks(phy_info_t *pi, uint8 core, uint16 orig_dac_clk_pu,
	uint16 orig_ovr_dac_clk_pu)
{
	MOD_RADIO_REG_TINY(pi, CLK_DIV_CFG1, core, dac_clk_pu, orig_dac_clk_pu);
	MOD_RADIO_REG_TINY(pi, CLK_DIV_OVR1, core, ovr_dac_clk_pu, orig_ovr_dac_clk_pu);
}

#if (defined(BCMDBG) && defined(DBG_PHY_IOV)) || defined(BCMDBG_PHYDUMP)
static int
phy_ac_radio_dump(phy_type_radio_ctx_t *ctx, struct bcmstrbuf *b)
{
	phy_ac_radio_info_t *gi = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = gi->pi;
	const char *name = NULL;
	int i, jtag_core;
	uint16 addr = 0;
	const radio_20xx_dumpregs_t *radio20xxdumpregs = NULL;
	bool use_binary_dump = FALSE;

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		name = "20693";
		if ((RADIO20693_MAJORREV(pi->pubpi->radiorev) == 1) ||
			(RADIO20693_MAJORREV(pi->pubpi->radiorev) == 2)) {
			uint16 phymode = phy_get_phymode(pi);
			if (phymode == PHYMODE_RSDB)
				radio20xxdumpregs = dumpregs_20693_rsdb;
			else if (phymode == PHYMODE_MIMO)
				radio20xxdumpregs = dumpregs_20693_mimo;
			else if (phymode == PHYMODE_80P80)
				radio20xxdumpregs = dumpregs_20693_80p80;
			else
				ASSERT(0);
		} else if (RADIO20693_MAJORREV(pi->pubpi->radiorev) == 3) {
			radio20xxdumpregs = dumpregs_20693_rev32;
		} else {
			return BCME_NOTFOUND;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
		name = "2069";
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			radio20xxdumpregs = dumpregs_2069_rev32;
		} else if ((RADIO2069REV(pi->pubpi->radiorev) == 16) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 18)) {
			radio20xxdumpregs = dumpregs_2069_rev16;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 17) {
			radio20xxdumpregs = dumpregs_2069_rev17;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 25) {
			radio20xxdumpregs = dumpregs_2069_rev25;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 64) {
			radio20xxdumpregs = dumpregs_2069_rev64;
		} else if (RADIO2069REV(pi->pubpi->radiorev) == 66) {
			radio20xxdumpregs = dumpregs_2069_rev64;
		} else {
			radio20xxdumpregs = dumpregs_2069_rev0;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
		name = "20698";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
		   case 0:
				radio20xxdumpregs = dumpregs_20698_rev0;
				break;
		   case 1:
				radio20xxdumpregs = dumpregs_20698_rev1;
				break;
		   case 2:
				radio20xxdumpregs = dumpregs_20698_rev2;
				break;
		   case 3:
				radio20xxdumpregs = dumpregs_20698_rev3;
				break;
		default:
			return BCME_ERROR;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
		name = "20704";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
			case 0:
			case 1:
			case 2:
			case 3:
				radio20xxdumpregs = dumpregs_20704_rev0;
				break;
			default:
				return BCME_ERROR;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
		name = "20707";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
			case 0:
				radio20xxdumpregs = dumpregs_20707_rev0;
				break;
			case 1:
				radio20xxdumpregs = dumpregs_20707_rev1;
				break;
			default:
				return BCME_ERROR;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {
		name = "20708";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
			case 0:
			case 1:
			case 2:
				radio20xxdumpregs = dumpregs_20708_rev0;
				break;
			default:
				return BCME_ERROR;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {
		name = "20709";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
			case 0:
			case 1:
				radio20xxdumpregs = dumpregs_20709_rev0;
				break;
			default:
				return BCME_ERROR;
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
		name = "20710";
		switch (HW_RADIOREV(pi->pubpi->radiorev)) {
			case 0:
			case 1:
				radio20xxdumpregs = dumpregs_20710_rev0;
				break;
			default:
				return BCME_ERROR;
		}
	} else
		return BCME_ERROR;

	if (!use_binary_dump) {
		i = 0;
		/* to indicate developer to put in the table */
		ASSERT(radio20xxdumpregs != NULL);
		if (radio20xxdumpregs == NULL)
			return BCME_OK; /* If the table is not yet implemented, */
				/* just skip the output (for non-assert builds) */

		bcm_bprintf(b, "----- %8s -----\n", name);
		bcm_bprintf(b, "Add Value");
		if (PHYCORENUM(pi->pubpi->phy_corenum) > 1)
			bcm_bprintf(b, "0 Value1 ...");
		bcm_bprintf(b, "\n");

		while ((addr = radio20xxdumpregs[i].address) != 0xffff) {

			if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
				jtag_core = (addr & JTAG_2069_MASK);
				addr &= (~JTAG_2069_MASK);
			} else
				jtag_core = 0;

			if ((RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) &&
				(jtag_core == JTAG_2069_ALL)) {
				switch (PHYCORENUM(pi->pubpi->phy_corenum)) {
				case 1:
					bcm_bprintf(b, "%03x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0));
					break;
				case 2:
					bcm_bprintf(b, "%03x %04x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0),
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR1));
					break;
				case 3:
					bcm_bprintf(b, "%03x %04x %04x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR0),
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR1),
						phy_utils_read_radioreg(pi, addr | JTAG_2069_CR2));
					break;
				default:
					break;
				}
			} else {
				if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
					(RADIOMAJORREV(pi) == 3)) {
					if ((addr >> JTAG_20693_SHIFT) ==
						(JTAG_20693_CR0 >> JTAG_20693_SHIFT)) {
						bcm_bprintf(b, "%03x %04x %04x %04x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR0),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR1),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR2),
						phy_utils_read_radioreg(pi, addr | JTAG_20693_CR3));
					} else if ((addr >> JTAG_20693_SHIFT) ==
							(JTAG_20693_PLL0 >> JTAG_20693_SHIFT)) {
						bcm_bprintf(b, "%03x %04x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | JTAG_20693_PLL0),
						phy_utils_read_radioreg(pi,
							addr | JTAG_20693_PLL1));
					}
				} else {
				bcm_bprintf(b, "%03x %04x\n", addr,
						phy_utils_read_radioreg(pi, addr | jtag_core));
				}
			}
			i++;
		}
	} else {
#ifdef PHY_DUMP_BINARY
		int buffer_length = BCMSTRBUF_LEN(b);

		/* Setup the page. */
		phy_dbg_page_parser(pi, TAG_TYPE_RAD, buffer_length);

		/* use binary dump table. */
		phy_dbg_dump_tbls(pi, TAG_TYPE_RAD,
			(uchar *)BCMSTRBUF_BUF(b), &buffer_length, FALSE);
#else
		return BCME_UNSUPPORTED;
#endif
	}

	return BCME_OK;
}
#endif

static void
BCMATTACHFN(phy_ac_radio_nvram_attach)(phy_info_t *pi, phy_ac_radio_info_t *radioi)
{
#ifndef BOARD_FLAGS3
	uint32 bfl3; /* boardflags3 */
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#endif

	if (BF3_RCAL_OTP_VAL_EN(pi->u.pi_acphy) == 1) {
		if (!otp_read_word(pi->sh->sih, ACPHY_RCAL_OFFSET, &pi->sromi->rcal_otp_val)) {
			pi->sromi->rcal_otp_val &= 0xf;
		} else {
			if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
					pi->sromi->rcal_otp_val = ACPHY_RCAL_VAL_2X2;
				} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
					pi->sromi->rcal_otp_val = ACPHY_RCAL_VAL_1X1;
				}
			}
		}
	}

	pi->dacratemode2g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_dacratemode2g, 1));
	pi->dacratemode5g = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_dacratemode5g, 1));
	radioi->data->srom_txnospurmod5g = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_txnospurmod5g, 1);
	radioi->data->srom_txnospurmod2g = (uint8)PHY_GETINTVAR_DEFAULT_SLICE(pi,
		rstr_txnospurmod2g, 0);
	radioi->data->vcodivmode = (uint8) (PHY_GETINTVAR_DEFAULT_SLICE(pi, rstr_vcodivmode, 0));

	/* PAD always on */
	if (ACMAJORREV_128(pi->pubpi->phy_rev) || ACMAJORREV_129(pi->pubpi->phy_rev)) {
		radioi->data->keep_pad_on = (bool)PHY_GETINTVAR_DEFAULT_SLICE(pi,
			rstr_keep_pad_on, 1);
	} else {
		radioi->data->keep_pad_on = FALSE;
	}

#ifndef BOARD_FLAGS3
	if ((PHY_GETVAR_SLICE(pi, rstr_boardflags3)) != NULL) {
		bfl3 = (uint32)PHY_GETINTVAR_SLICE(pi, rstr_boardflags3);
		BF3_RCAL_WAR(pi_ac) = ((bfl3 & BFL3_RCAL_WAR) != 0);
		BF3_RCAL_OTP_VAL_EN(pi_ac) = ((bfl3 & BFL3_RCAL_OTP_VAL_EN) != 0);
		BF3_TSSI_DIV_WAR(pi_ac) = (bfl3 & BFL3_TSSI_DIV_WAR) >> BFL3_TSSI_DIV_WAR_SHIFT;
	} else {
		BF3_RCAL_WAR(pi_ac) = 0;
		BF3_RCAL_OTP_VAL_EN(pi_ac) = 0;
		BF3_TSSI_DIV_WAR(pi_ac) = 0;
	}
#endif /* BOARD_FLAGS3 */
}

static void wlc_phy_radio20698_pmu_pll_pwrup(phy_info_t *pi, uint8 pll)
{
	/* 20698_procs.tcl r708059: 20698_pmu_pll_pwrup */

	/* power up x2, cp, vco ldo's (not sure if x2, cpldo and vcoldo are ON at reset) */
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_pu_ldo, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_pu_ldo, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_pu_ldo_VCO, 0x1);

	/* toggling bias_reset */
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_bias_reset, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_bias_reset, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_bias_reset_CP, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_bias_reset_CP, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_bias_reset_VCO, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_bias_reset_VCO, 0x1);
	OSL_DELAY(89);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_bias_reset, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_bias_reset, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_bias_reset_CP, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_bias_reset_CP, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, pll, ldo_1p8_bias_reset_VCO, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, pll, ovr_ldo_1p8_bias_reset_VCO, 0x0);
}

static void wlc_phy_radio20704_pmu_pll_pwrup(phy_info_t *pi)
{
	/* 20704_procs.tcl r770326: 20704_pmu_pll_pwrup */

	/* Power up CP LDO, VCO LDOs */
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_OVR1, 0, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_CP1, 0, rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_CFG1, 0, rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_CFG1, 0, rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_MMD1, 0, rfpll_mmd_en_phasealign, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_CFG1, 0, rfpll_pfd_en, 0xf);
	MOD_RADIO_PLLREG_20704(pi, PLL_VCO5, 0, rfpll_vco_tempco_en, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_LF6, 0, rfpll_lf_cm_pu, 0x1);
	MOD_RADIO_PLLREG_20704(pi, PLL_VCO7, 0, rfpll_vco_core1_en, 0x1);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_REG0, 0, logen_vcobuf_pu, 0x1);
}

static void wlc_phy_radio20707_pmu_pll_pwrup(phy_info_t *pi)
{

	/* Power up CP LDO, VCO LDOs */
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_OVR1, 0, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_CP1, 0, rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_CFG1, 0, rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_CFG1, 0, rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_MMD1, 0, rfpll_mmd_en_phasealign, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_CFG1, 0, rfpll_pfd_en, 0xf);
	MOD_RADIO_PLLREG_20707(pi, PLL_VCO5, 0, rfpll_vco_tempco_en, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_LF6, 0, rfpll_lf_cm_pu, 0x1);
	MOD_RADIO_PLLREG_20707(pi, PLL_VCO7, 0, rfpll_vco_core1_en, 0x1);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_vcobuf_pu, 0x1);
}

static void wlc_phy_radio20708_pmu_pll_pwrup(phy_info_t *pi)
{
	uint16 core;

	/* Power up CP LDO, VCO LDOs */
	for (core = 0; core < 2; core++) {
		MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_ldo_1p8_pu_ldo_CP, 0x1);
		MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
		MOD_RADIO_PLLREG_20708(pi, PLL_HVLDO1, core, rfpll_cp_ldo_pu, 0x1);
		MOD_RADIO_PLLREG_20708(pi, PLL_HVLDO1, core, rfpll_vco_ldo_pu, 0x1);
	}
}

static void wlc_phy_radio20709_pmu_pll_pwrup(phy_info_t *pi)
{

	/* Power up CP LDO, VCO LDOs */
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_OVR1, 0, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_CP1, 0, rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_CFG1, 0, rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_CFG1, 0, rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_MMD1, 0, rfpll_mmd_en_phasealign, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_CFG1, 0, rfpll_pfd_en, 0xf);
	MOD_RADIO_PLLREG_20709(pi, PLL_VCO5, 0, rfpll_vco_tempco_en, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_LF6, 0, rfpll_lf_cm_pu, 0x1);
	MOD_RADIO_PLLREG_20709(pi, PLL_VCO7, 0, rfpll_vco_core1_en, 0x1);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_vcobuf_pu, 0x1);
}

static void wlc_phy_radio20710_pmu_pll_pwrup(phy_info_t *pi)
{
	/* 20710_procs.tcl r770326: 20710_pmu_pll_pwrup */

	/* Power up CP LDO, VCO LDOs */
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_HVLDO1, 0, ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_OVR1, 0, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_CP1, 0, rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_CFG1, 0, rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_CFG1, 0, rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_CFG1, 0, rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_MMD1, 0, rfpll_mmd_en_phasealign, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_CFG1, 0, rfpll_pfd_en, 0xf);
	MOD_RADIO_PLLREG_20710(pi, PLL_VCO5, 0, rfpll_vco_tempco_en, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_LF6, 0, rfpll_lf_cm_pu, 0x1);
	MOD_RADIO_PLLREG_20710(pi, PLL_VCO7, 0, rfpll_vco_core1_en, 0x1);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_REG0, 0, logen_vcobuf_pu, 0x1);
}

/* Work-arounds */
static void wlc_phy_radio20698_wars(phy_info_t *pi)
{
	// Nothing yet.
}

/* Work-arounds */
static void wlc_phy_radio20704_wars(phy_info_t *pi)
{
	// Nothing yet.
}

/* Work-arounds */
static void wlc_phy_radio20707_wars(phy_info_t *pi)
{
	// Nothing yet.
}

/* Work-arounds */
static void wlc_phy_radio20708_wars(phy_info_t *pi)
{
	// Nothing yet.
}

/* Work-arounds */
static void wlc_phy_radio20709_wars(phy_info_t *pi)
{
	// Nothing yet.
}

/* Work-arounds */
static void wlc_phy_radio20710_wars(phy_info_t *pi)
{
	// Nothing yet.
}

static void wlc_phy_radio20698_refclk_en(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_refclk_en */
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 0, RefDoubler_pu_pfddrv, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, 0, ovr_RefDoubler_pu_pfddrv, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 1, RefDoubler_pu_pfddrv, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, 1, ovr_RefDoubler_pu_pfddrv, 0x1);

	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkvcocal, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	/* power up doubler ldo */
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_pu_ldo, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_pu_ldo, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_bias_reset, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_bias_reset, 0x1);
	OSL_DELAY(ISSIM_ENAB(pi->sh->sih)? 10 : 89);
	MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER1, 0, RefDoubler_ldo_1p8_bias_reset, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR2, 0, ovr_RefDoubler_ldo_1p8_bias_reset, 0x0);
}

void wlc_phy_radio20698_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20698_procs.tcl r727996: 20698_sel_logen_mode */

	/* Mode 0. 4x4(core0,1,2,3)  ------------------ this is 43684 default mode ---------
	 * Mode 1  3x3(core0,1,2)  -------------------- 43684 3x3
	 * Mode 2. 2x2(core0,1)      ------------------ 43684 2x2
	 * Mode 3. 1x1 (core 0) ----------------------- 43684 1x1
	 * Mode 4. 3x3(core0,1,2) + 1x1(core3) -------- 43684 3+1
	 * Mode 5. Off
	 */
	const uint8 logen_gm2_pu[6]           =  {1, 1, 1, 1, 1, 0};
	const uint8 logen_gm3_1_pu[6]         =  {0, 0, 0, 0, 1, 0};
	const uint8 logen_gm3_2_pu[6]         =  {1, 0, 0, 0, 0, 0};
	const uint8 logen_gm_pu[4][6]         = {{0, 0, 0, 0, 0, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {0, 0, 0, 0, 0, 0}};
	const uint8 logen_out_en[4][6]        = {{0, 0, 0, 0, 0, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {1, 0, 0, 0, 0, 0}};
	const uint8 logen_sel_inp_south[4][6] = {{1, 1, 1, 1, 1, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {1, 1, 1, 1, 1, 0},
	                                         {0, 0, 0, 0, 1, 0}};
	const uint8 logen_sel_inp_north[4][6] = {{0, 0, 0, 0, 0, 0},
	                                         {0, 0, 0, 0, 0, 0},
	                                         {0, 0, 0, 0, 0, 0},
	                                         {1, 1, 1, 1, 0, 0}};
	uint8 core;

	ASSERT(mode < 6);

	MOD_RADIO_REG_20698(pi, LOGEN_REG2, 2, logen_gm2_pu, logen_gm2_pu[mode]);
	MOD_RADIO_REG_20698(pi, LOGEN_REG2, 2, logen_gm3_1_pu, logen_gm3_1_pu[mode]);
	MOD_RADIO_REG_20698(pi, LOGEN_REG2, 2, logen_gm3_2_pu, logen_gm3_2_pu[mode]);
	for (core = 0; core < 4; core++) {
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG0, core,
				logen_gm_pu, logen_gm_pu[core][mode]);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, core,
				logen_out_en, logen_out_en[core][mode]);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, core,
				logen_sel_inp_south, logen_sel_inp_south[core][mode]);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, core,
				logen_sel_inp_north, logen_sel_inp_north[core][mode]);
		RADIO_REG_LIST_START
			/* FIXME43684: Following logen pu's should probably move
			 * to preferred values
			 */
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_lc_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_bias_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_buf_AFE_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_bias_ptat, 0x4)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gm_local_idac104u, 0x5)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_south_idac104u, 0x5)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_north_idac104u, 0x5)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

void wlc_phy_radio20704_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20704_procs.tcl r741315: 20704_sel_logen_mode
	 * Mode 0. 2x2(core0,1)
	 * Mode 1. 1x1 (core 1)
	 * Mode 2. Off
	 */
	const uint8 ncores = 2;
	const uint8 logen_gm3_1_pu[3]        = {1, 1, 0};
	const uint8 logen_pu[3]              = {1, 1, 0};
	const uint8 logen_mux_pu[3]          = {1, 1, 0};
	const uint8 logen_mixer_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_sel[3]       = {0, 0, 0};
	const uint8 gm_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 lc_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 buf_AFE_pu[][3]          = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_bias_pu[][3]       = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_out_en[][3]        = {{1, 0, 0},
	                                        {1, 0, 0}};
	const uint8 logen_sel_inp_south[][3] = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 rx_db_mux_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 tx_db_mux_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	uint8 core;

	ASSERT(mode < 3);
	ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) == ncores);

	MOD_RADIO_PLLREG_20704(pi, LOGEN_REG2, 0,
			logen_gm3_1_pu, logen_gm3_1_pu[mode]);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_REG0, 0,
			logen_pu, logen_pu[mode]);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mux_pu, logen_mux_pu[mode]);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mixer_pu, logen_mixer_pu[mode]);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_pu, logen_clkx2_pu[mode]);
	MOD_RADIO_PLLREG_20704(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_sel, logen_clkx2_sel[mode]);

	for (core = 0; core < ncores; core++) {
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG0, core,
				logen_gm_pu, gm_pu[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG0, core,
				logen_lc_pu, lc_pu[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG0, core,
				logen_buf_AFE_pu, buf_AFE_pu[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG0, core,
				logen_bias_pu, logen_bias_pu[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core,
				logen_out_en, logen_out_en[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core,
				logen_sel_inp_south, logen_sel_inp_south[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core,
				logen_rx_db_mux_pu, rx_db_mux_pu[core][mode]);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core,
				logen_tx_db_mux_pu, tx_db_mux_pu[core][mode]);
	}
}

void wlc_phy_radio20707_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20707_procs.tcl 20707_sel_logen_mode
	* Mode 0. 3x3 (core0,1,2)
	* Mode 1. 2x2 (core0,1)
	* Mode 2. 1x1 (core 0)
	* Mode 3. Off
	*/
	const uint8 ncores = 3;
	const uint8 logen_gm3_1_pu[4]        = {1, 1, 1, 0};
	const uint8 logen_pu[4]              = {1, 1, 1, 0};
	const uint8 logen_mux_pu[4]          = {1, 1, 1, 0};
	const uint8 logen_mixer_pu[4]        = {1, 1, 1, 0};
	const uint8 logen_clkx2_pu[4]        = {1, 1, 1, 0};
	const uint8 logen_clkx2_sel[4]       = {0, 0, 0, 0};
	const uint8 gm_pu[][4]               = {{1, 1, 1, 0},
	                                        {1, 1, 1, 0},
	                                        {1, 1, 1, 0}};
	const uint8 lc_pu[][4]               = {{1, 1, 1, 0},
	                                        {1, 1, 1, 0},
	                                        {1, 1, 1, 0}};
	const uint8 buf_AFE_pu[][4]          = {{1, 1, 1, 0},
	                                        {1, 1, 0, 0},
	                                        {1, 0, 0, 0}};
	const uint8 logen_bias_pu[][4]       = {{1, 1, 1, 0},
	                                        {1, 1, 1, 0},
	                                        {1, 1, 1, 0}};
	const uint8 logen_out_en[][4]        = {{1, 1, 1, 0},
	                                        {1, 1, 1, 0},
	                                        {1, 1, 1, 0}};
	const uint8 logen_sel_inp_south[][4] = {{1, 1, 1, 0},
	                                        {1, 1, 1, 0},
	                                        {1, 1, 1, 0}};
	const uint8 rx_db_mux_pu[][4]        = {{1, 1, 1, 0},
	                                        {1, 1, 0, 0},
	                                        {1, 0, 0, 0}};
	const uint8 buf_2G_pu[][4]           = {{1, 1, 1, 0},
	                                        {1, 1, 0, 0},
	                                        {1, 0, 0, 0}};

	uint8 core;

	ASSERT(mode < 4);
	ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) == ncores);

	MOD_RADIO_PLLREG_20707(pi, LOGEN_REG2, 0,
		logen_gm3_1_pu, logen_gm3_1_pu[mode]);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0,
		logen_pu, logen_pu[mode]);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0,
		logen_6g_mux_pu, logen_mux_pu[mode]);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0,
		logen_6g_mixer_pu, logen_mixer_pu[mode]);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0,
		logen_6g_clk_x2_pu, logen_clkx2_pu[mode]);
	MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0,
		logen_6g_clk_x2_sel, logen_clkx2_sel[mode]);

	for (core = 0; core < ncores; core++) {
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core,
			logen_gm_pu, gm_pu[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core,
			logen_lc_pu, lc_pu[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core,
			logen_buf_AFE_pu, buf_AFE_pu[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core,
			logen_bias_pu, logen_bias_pu[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG1, core,
			logen_out_en, logen_out_en[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG1, core,
			logen_sel_inp_south, logen_sel_inp_south[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG1, core,
			logen_rx_db_mux_pu, rx_db_mux_pu[core][mode]);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core,
			logen_buf_2G_pu, buf_2G_pu[core][mode]);
	}
}

void wlc_phy_radio20708_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20708_procs.tcl 20708_sel_logen_mode */
	/* logen_mode	0 Off */
	/* logen_mode	1 4x4, PLL0 only */
	/* logen_mode	2 3+1, PLL0 for Main, PLL1 for Scan */
	/* logen_mode	3 3x3, PLL0 only */
	/* logen_mode	4 4x4, PLL1 only */
	/* logen_mode	5 3+1, PLL1 for Main, PLL0 for Scan */
	/* logen_mode	6 3x3, PLL1 only */
	/* logen_mode	7 debug, PLL0 for Scan core only */
	/* 0(off) 1(pll0) 2(pll0&1) 3(pll0) 4(pll1) 5(pll1&0) 6(pll1) 7(debug) */
	const uint8 on[8]      = {0, 1, 1, 1, 1, 1, 1, 0};
	const uint8 lo1[8]     = {0, 1, 1, 0, 1, 1, 0, 1};
	const uint8 lo0_0[8]   = {0, 1, 1, 1, 0, 0, 0, 0};
	const uint8 lo1_0[8]   = {0, 1, 0, 0, 0, 1, 0, 1};
	const uint8 lo0_1[8]   = {0, 0, 0, 0, 1, 1, 1, 0};
	const uint8 lo1_1[8]   = {0, 0, 1, 0, 1, 0, 0, 0};
	const uint8 lo0_sel[8] = {0, 0, 0, 0, 1, 1, 1, 0};
	const uint8 lo1_sel[8] = {0, 0, 1, 1, 1, 0, 0, 0};
	const uint8 pll0_on[8] = {0, 1, 1, 1, 0, 1, 0, 1};
	const uint8 pll1_on[8] = {0, 0, 1, 0, 1, 1, 1, 0};

	ASSERT(mode < 8);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 0, logen_pu_mux, pll0_on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 1, logen_pu_mux, pll1_on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 0, logen_mixer_pu, pll0_on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 1, logen_mixer_pu, pll1_on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 0, logen_buff_pu, pll0_on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 1, logen_buff_pu, pll1_on[mode]);

	MOD_RADIO_PLLREG_20708(pi, LOGEN_TOP_OVR0, 0, ovr_logen_lomux_pu, 1);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_TOP_OVR0, 1, ovr_logen_lomux_pu, 1);

	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 0, logen_lomux_pu, on[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 1, logen_lomux_pu, lo1[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, 0, logen_lomux_pu_buf0, lo0_0[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, 1, logen_lomux_pu_buf0, lo1_0[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, 0, logen_lomux_pu_buf1, lo0_1[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, 1, logen_lomux_pu_buf1, lo1_1[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 0, logen_lomux_sel, lo0_sel[mode]);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, 1, logen_lomux_sel, lo1_sel[mode]);
}

void wlc_phy_radio20709_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20709_procs.tcl r798817: 20709_sel_logen_mode
	 * Mode 0. 2x2(core0,1)
	 * Mode 1. 1x1 (core 1)
	 * Mode 2. Off
	 */
	const uint8 ncores = 2;
	const uint8 logen_gm3_1_pu[3]        = {1, 1, 0};
	const uint8 logen_pu[3]              = {1, 1, 0};
	const uint8 logen_mux_pu[3]          = {1, 1, 0};
	const uint8 logen_mixer_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_sel[3]       = {0, 0, 0};
	const uint8 gm_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 lc_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 buf_AFE_pu[][3]          = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_bias_pu[][3]       = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_out_en[][3]        = {{1, 0, 0},
	                                        {1, 0, 0}};
	const uint8 logen_sel_inp_south[][3] = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 rx_db_mux_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 buf_2G_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	uint8 core;

	ASSERT(mode < 3);
	ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) == ncores);

	MOD_RADIO_PLLREG_20709(pi, LOGEN_REG2, 0,
			logen_gm3_1_pu, logen_gm3_1_pu[mode]);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0,
			logen_pu, logen_pu[mode]);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mux_pu, logen_mux_pu[mode]);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mixer_pu, logen_mixer_pu[mode]);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_pu, logen_clkx2_pu[mode]);
	MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_sel, logen_clkx2_sel[mode]);

	for (core = 0; core < ncores; core++) {
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG0, core,
			logen_gm_pu, gm_pu[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG0, core,
			logen_lc_pu, lc_pu[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG0, core,
			logen_buf_AFE_pu, buf_AFE_pu[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG0, core,
			logen_bias_pu, logen_bias_pu[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG1, core,
			logen_out_en, logen_out_en[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG1, core,
			logen_sel_inp_south, logen_sel_inp_south[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG1, core,
			logen_rx_db_mux_pu, rx_db_mux_pu[core][mode]);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG0, core,
			logen_buf_2G_pu, buf_2G_pu[core][mode]);
	}
}

void wlc_phy_radio20710_sel_logen_mode(phy_info_t *pi, uint8 mode)
{
	/* 20710_procs.tcl r741315: 20710_sel_logen_mode
	 * Mode 0. 2x2(core0,1)
	 * Mode 1. 1x1 (core 1)
	 * Mode 2. Off
	 */
	const uint8 ncores = 2;
	const uint8 logen_gm3_1_pu[3]        = {1, 1, 0};
	const uint8 logen_pu[3]              = {1, 1, 0};
	const uint8 logen_mux_pu[3]          = {1, 1, 0};
	const uint8 logen_mixer_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_pu[3]        = {1, 1, 0};
	const uint8 logen_clkx2_sel[3]       = {0, 0, 0};
	const uint8 gm_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 lc_pu[][3]               = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 buf_AFE_pu[][3]          = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_bias_pu[][3]       = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 logen_out_en[][3]        = {{1, 0, 0},
	                                        {1, 0, 0}};
	const uint8 logen_sel_inp_south[][3] = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 rx_db_mux_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	const uint8 tx_db_mux_pu[][3]        = {{1, 0, 0},
	                                        {1, 1, 0}};
	uint8 core;

	ASSERT(mode < 3);
	ASSERT(PHYCORENUM((pi)->pubpi->phy_corenum) == ncores);

	MOD_RADIO_PLLREG_20710(pi, LOGEN_REG2, 0,
			logen_gm3_1_pu, logen_gm3_1_pu[mode]);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_REG0, 0,
			logen_pu, logen_pu[mode]);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mux_pu, logen_mux_pu[mode]);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_CORE_REG6, 0,
			logen_6g_mixer_pu, logen_mixer_pu[mode]);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_pu, logen_clkx2_pu[mode]);
	MOD_RADIO_PLLREG_20710(pi, LOGEN_CORE_REG6, 0,
			logen_6g_clk_x2_sel, logen_clkx2_sel[mode]);

	for (core = 0; core < ncores; core++) {
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG0, core,
				logen_gm_pu, gm_pu[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG0, core,
				logen_lc_pu, lc_pu[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG0, core,
				logen_buf_AFE_pu, buf_AFE_pu[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG0, core,
				logen_bias_pu, logen_bias_pu[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core,
				logen_out_en, logen_out_en[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core,
				logen_sel_inp_south, logen_sel_inp_south[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core,
				logen_rx_db_mux_pu, rx_db_mux_pu[core][mode]);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG1, core,
				logen_tx_db_mux_pu, tx_db_mux_pu[core][mode]);
	}
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20698_upd_prfd_values(phy_info_t *pi)
{
	/* 20698_procs.tcl r727996: 20698_upd_prfd_values */
	uint i = 0;
	const radio_20xx_prefregs_t *prefregs_20698_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20698_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 0:
		prefregs_20698_ptr = prefregs_20698_rev0;
		break;
	case 1:
		prefregs_20698_ptr = prefregs_20698_rev1;
		break;
	case 2:
		prefregs_20698_ptr = prefregs_20698_rev2;
		break;
	case 3:
		prefregs_20698_ptr = prefregs_20698_rev3;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
	}

	/* Update preferred values */
	while (prefregs_20698_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20698_ptr[i].address,
			(uint16)prefregs_20698_ptr[i].init);
		i++;
	}

	wlc_phy_radio20698_sel_logen_mode(pi, 0);
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20704_upd_prfd_values(phy_info_t *pi)
{
	/* 20704_procs.tcl r821625: 20704_upd_prfd_values */

	uint i = 0;
	uint core = 0;
	const radio_20xx_prefregs_t *prefregs_20704_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20704_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 0:
	case 1:
	case 2:
	case 3:
		prefregs_20704_ptr = prefregs_20704_rev0;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
	}

	/* Update preferred values */
	while (prefregs_20704_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20704_ptr[i].address,
			(uint16)prefregs_20704_ptr[i].init);
		i++;
	}

	MOD_RADIO_PLLREG_20704(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 0x1);
	wlc_phy_radio20704_sel_logen_mode(pi, 0);

	FOREACH_CORE(pi, core) {
		/* Power-Down gia 2GRX */
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1);
		/* Power-Down gia 2GTX */
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd, 1);
		/* ADC ref current source: 1 = cleaner reference for better ADC slope */
		MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_20u_ctal_uncal_en, 1);
	}
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20707_upd_prfd_values(phy_info_t *pi)
{

	uint i = 0;
	uint core = 0;
	const radio_20xx_prefregs_t *prefregs_20707_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20707_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
		case 0:
			prefregs_20707_ptr = prefregs_20707_rev0;
			break;
		case 1:
			prefregs_20707_ptr = prefregs_20707_rev1;
			break;
		default:
			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
			ASSERT(FALSE);
	}

	/* Update preferred values */
	while (prefregs_20707_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20707_ptr[i].address,
		(uint16)prefregs_20707_ptr[i].init);
		i++;
	}

	wlc_phy_radio20707_sel_logen_mode(pi, 0);

	MOD_RADIO_PLLREG_20707(pi, PLL_VCO7, 0, rfpll_vco_ctail_bot, 3);
	MOD_RADIO_PLLREG_20707(pi, PLL_VCO6, 0, rfpll_vco_ctail_top, 3);
	MOD_RADIO_PLLREG_20707(pi, PLL_HVLDO3, 0, ldo_1p8_ldo_CP_vout_sel, 4);
	MOD_RADIO_PLLREG_20707(pi, PLL_VCO7, 0, rfpll_vco_buf_sel_1p8V_1p0V, 1);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1);
		MOD_RADIO_REG_20707(pi, RX5G_REG4, core, rxdb_gm_cc, 15);
		MOD_RADIO_REG_20707(pi, RX5G_REG2, core, rxdb_lna_cc, 15);
		MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0);
		MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_buf_bw, 3);
		MOD_RADIO_REG_20707(pi, TXDAC_REG1, core, iqdac_lowcm_en, 0);
		MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_attn, 0);
		MOD_RADIO_REG_20707(pi, TXDAC_REG1, core, iqdac_buf_op_cur, 1);
		MOD_RADIO_REG_20707(pi, TXDAC_REG1, core, iqdac_buf_suref_ctrl, 2);
		MOD_RADIO_REG_20707(pi, PMU_OP1, core, wlpmu_TXldo_adj, 4);
		MOD_RADIO_REG_20707(pi, RX2G_REG4, core, rx_ldo_adj, 2);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core, logen_5g_rx_rccr_iqbias_short, 1);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG0, core, logen_5g_tx_rccr_iqbias_short, 1);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG2, core, logen_rx_rccr_idac26u, 7);
	}
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20708_upd_prfd_values(phy_info_t *pi)
{

	uint i = 0;
	uint core = 0, pllcore = 0;
	const radio_20xx_prefregs_t *prefregs_20708_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20708_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
		case 0:
		case 1:
		case 2:
			prefregs_20708_ptr = prefregs_20708_rev0;
			break;
		default:
			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
			ASSERT(FALSE);
	}

	/* Update preferred values */
	while (prefregs_20708_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20708_ptr[i].address,
		(uint16)prefregs_20708_ptr[i].init);
		i++;
	}

	wlc_phy_radio20708_pll_logen_pu_sel(pi, 0, pi->u.pi_acphy->radioi->maincore_on_pll1);

	/* Settings below need are here temporarily,
	 * they will be moved to preferred values at a later point
	 */
	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
		    /* for solving the single core rx problem */
			MOD_RADIO_REG_20708_ENTRY(pi, LOGEN_CORE_OVR0, core,
				ovr_logen_core_buff_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LOGEN_CORE_REG0, core,
				logen_core_buff_pu, 0x1)

		    /* this needs to be driven by ucode!
			 * In RX always 1, in TX 1 for 20/40  and 0 for 80/160
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG12, core, lpf_spare, 1)

			/* Force iqdac 8u bias pu high, was missing in radio model */
			MOD_RADIO_REG_20708_ENTRY(pi, TXDAC_REG0, core, iqdac_pu_ibn_8u_1p8bg, 1)

			/* Suggested by WCC for common mode stability */
			MOD_RADIO_REG_20708_ENTRY(pi, TXDAC_REG1, core, iqdac_buf_suref_ctrl, 1)
			MOD_RADIO_REG_20708_ENTRY(pi, TXDAC_REG7, core, iqdac_buf_biasing_trim, 2)

			/* For insufficiently filtered VDD rails, preferable to keep
			 * LOGEN LDO nominal, to take advantage of PSRR
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, PMU_OP1, core, wlpmu_LOGENldo_adj, 0)

			/* For insufficiently filtered VDD rails, preferable to keep
			 * TXLDO LDO nominal, to take advantage of PSRR
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, PMU_OP1, core, wlpmu_TXldo_adj, 0)

			/* 0 corresponds to nominal output 0.9V */
			MOD_RADIO_REG_20708_ENTRY(pi, RX2G_REG4, core, rx_ldo_adj, 0)

			/* At the presend of over 0 dBm signal at the LNA input this bit
			 * increases LNA current a bit
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_lna_pu_pulse, 1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core, rxdb_lna_pu_pulse, 1)

			/* Legacy, keep in 1 for 2G, 5G irrelevant */
			MOD_RADIO_REG_20708_ENTRY(pi, LOGEN_CORE_REG5, core,
				logen_core_lophase_pd, 1)

			/* Keep the AuxPGA ovr state in 1 and the default state to 1 for the TSSI.
			 * The AuxPGA needs to NOT toggle, because it loads the BGR and creates
			 * settling. Let the cal routines change the on /off state as they require.
			 */
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 1)
			MOD_RADIO_REG_20708_ENTRY(pi, AUXPGA_CFG1, core, auxpga_i_pu, 1)

		RADIO_REG_LIST_EXECUTE(pi, core);
	}

	/* WRSSI1 settings from Pavlos (some already default setttings) */
	if (RADIOMAJORREV(pi) < 2) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_wrssi_enable, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_low_pu, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_mid_pu, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_high_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_higher_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_highest_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_drive_strength, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_gpaio_enable, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_sel_path, 1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_wrssi_enable, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_drive_strength, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_gpaio_enable, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_low_pu, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_mid_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_high_pu, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_higher_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_highest_pu, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_atten, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_sel_path, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_cur_ctrl_her, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_cur_ctrl_hst, 0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG1, core,
					rxdb_wrssi1_threshold_low, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_WRSSI1_REG1, core,
					rxdb_wrssi1_threshold_high, 4)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR1, core, ovr_rxadc_pu_nap, 1);
		MOD_RADIO_REG_20708(pi, RXADC_CFG0, core, rxadc_pu_nap, 0);
		MOD_RADIO_REG_20708(pi, PMU_CFG2, core, wlpmu_logenldo_LPF_shortb, 1);
	}

	/* A1 mixer diode filtering, prevent from toggling to reduce transients. */
	if (RADIOMAJORREV(pi) == 1) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20708(pi, TX2G_CFG1_OVR, core, ovr_txdb_mx_bias_reset_bb, 1);
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core, txdb_mx_bias_reset_bb, 1);
		}
	}

	/* For B0, some more investigation might be needed */
	if (RADIOMAJORREV(pi) >= 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20708(pi, TX2G_CFG1_OVR, core, ovr_txdb_mx_bias_reset_lo, 0);
			MOD_RADIO_REG_20708(pi, TX2G_CFG1_OVR, core, ovr_txdb_mx_bias_reset_cas, 0);
			MOD_RADIO_REG_20708(pi, TX2G_CFG1_OVR, core, ovr_txdb_mx_bias_reset_bb, 0);
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core, txdb_mx_bias_reset_lo, 0);
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core, txdb_mx_bias_reset_cas, 0);
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core, txdb_mx_bias_reset_bb, 0);
			MOD_RADIO_REG_20708(pi, TXDB_REG0, core, tx_mx_bias_lo_res, 7);
			MOD_RADIO_REG_20708(pi, TXDB_REG0, core, tx_mx_bias_gm_res, 31);
			MOD_RADIO_REG_20708(pi, TXDAC_REG7, core, iqdac_buf_biasing_trim, 0);
		}
	}

	for (pllcore = 0; pllcore < 2; pllcore++) {
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG1, pllcore, afediv_dac_clk_buff_p0, 0);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG1, pllcore, afediv_dac_clk_buff_n0, 0);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG1, pllcore, afediv_adc_clk2x_buff_p0, 0);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG1, pllcore, afediv_adc_clk2x_buff_n0, 0);
	};

	MOD_RADIO_PLLREG_20708(pi, XTAL7, 0, xtal_ldo_bias, 12);
	MOD_RADIO_PLLREG_20708(pi, XTAL4, 0, xtal_ldo_bias_startup, 0);
	MOD_RADIO_PLLREG_20708(pi, PLL_LF1, 0, rfpll_lf_bias_cm, 3);
	MOD_RADIO_PLLREG_20708(pi, PLL_LF1, 1, rfpll_lf_bias_cm, 3);
	MOD_RADIO_PLLREG_20708(pi, XTAL5, 0, xtal_xcore_nmos, 31);
	MOD_RADIO_PLLREG_20708(pi, XTAL5, 0, xtal_xcore_pmos, 31);
	MOD_RADIO_PLLREG_20708(pi, PLL_CP1, 0, rfpll_cp_idac_op_sel, 1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CP1, 1, rfpll_cp_idac_op_sel, 1);
	if (BCM6715X_PKG(pi->sh->sih->otpflag) || BCM6715X2_PKG(pi->sh->sih->otpflag)) {
		MOD_RADIO_PLLREG_20708(pi, XTAL7, 0, xtal_ldo_Vbuck_ctrl, 6);
		MOD_RADIO_PLLREG_20708(pi, XTAL7, 0, xtal_ldo_Vout_ctrl, 7);
	} else {
		MOD_RADIO_PLLREG_20708(pi, XTAL7, 0, xtal_ldo_Vout_ctrl, 12);
	}
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG2, 0, logen_loldo_adj, 0);
	MOD_RADIO_PLLREG_20708(pi, LOGEN_REG2, 1, logen_loldo_adj, 0);
	MOD_RADIO_PLLREG_20708(pi, XTAL6, 0, xtal_synth0buf_rstrg, 15);
	MOD_RADIO_PLLREG_20708(pi, XTAL6, 0, xtal_synth0buf_fstrg, 15);
	MOD_RADIO_PLLREG_20708(pi, XTAL5, 0, xtal_synth1buf_rstrg, 15);
	MOD_RADIO_PLLREG_20708(pi, XTAL6, 0, xtal_synth1buf_fstrg, 15);

	if (RADIOMAJORREV(pi) >= 2) {
		MOD_RADIO_PLLREG_20708(pi, PLL_LVLDO1, 0, rfpll_pfdmmd_ldo_vout_sel, 5);
		MOD_RADIO_PLLREG_20708(pi, PLL_LVLDO1, 1, rfpll_pfdmmd_ldo_vout_sel, 5);
	} else {
		MOD_RADIO_PLLREG_20708(pi, PLL_LVLDO1, 0, rfpll_pfdmmd_ldo_vout_sel, 0);
		MOD_RADIO_PLLREG_20708(pi, PLL_LVLDO1, 1, rfpll_pfdmmd_ldo_vout_sel, 0);
	}
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20709_upd_prfd_values(phy_info_t *pi)
{
	/* 20709_procs.tcl r823755: 20709_upd_prfd_values */

	uint i = 0;
	uint core = 0;
	const radio_20xx_prefregs_t *prefregs_20709_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20709_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 0:
	case 1:
		prefregs_20709_ptr = prefregs_20709_rev0;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
	}

	/* Update preferred values */
	while (prefregs_20709_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20709_ptr[i].address,
			(uint16)prefregs_20709_ptr[i].init);
		i++;
	}

	wlc_phy_radio20709_sel_logen_mode(pi, 0);

	ACPHY_REG_LIST_START
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_VCO7, 0, rfpll_vco_ctail_bot, 3)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_VCO6, 0, rfpll_vco_ctail_top, 3)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_HVLDO3, 0, ldo_1p8_ldo_CP_vout_sel, 4)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_VCO7, 0, rfpll_vco_buf_sel_1p8V_1p0V, 1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 7)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, LOGEN_REG1, 0, logen_bias_mix, 5)
	ACPHY_REG_LIST_EXECUTE(pi);

	FOREACH_CORE(pi, core) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG0, core, iqdac_buf_bw, 3)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG1, core, iqdac_lowcm_en, 0)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG0, core, iqdac_attn, 0)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG1, core, iqdac_buf_op_cur, 1)
			MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG1, core, iqdac_buf_suref_ctrl, 2)
			MOD_RADIO_REG_20709_ENTRY(pi, PMU_OP1, core, wlpmu_TXldo_adj, 4)
			MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
				logen_5g_rx_rccr_iqbias_short, 1)
			MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
				logen_5g_tx_rccr_iqbias_short, 1)
			MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG3, core, logen_lc_idac26u, 7)
			MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG2, core,
				logen_rx_rccr_idac26u, 7)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

/* Load additional perferred values after power up */
static void wlc_phy_radio20710_upd_prfd_values(phy_info_t *pi)
{
	/* 20710_procs.tcl r904339: 20710_upd_prfd_values */

	uint i = 0;
	uint core = 0;
	const radio_20xx_prefregs_t *prefregs_20710_ptr = NULL;

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20710_ID);

	/* Choose the right table to use */
	switch (RADIOREV(pi->pubpi->radiorev)) {
	case 0:
	case 1:
		prefregs_20710_ptr = prefregs_20710_rev0;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIOREV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		prefregs_20710_ptr = prefregs_20710_rev0;
	}

	/* Update preferred values */
	while (prefregs_20710_ptr[i].address != 0xffff) {
		phy_utils_write_radioreg(pi, prefregs_20710_ptr[i].address,
			(uint16)prefregs_20710_ptr[i].init);
		i++;
	}

	MOD_RADIO_PLLREG_20710(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 0x1);
	wlc_phy_radio20710_sel_logen_mode(pi, 0);

	FOREACH_CORE(pi, core) {
		ACPHY_REG_LIST_START
			/* Power-Down gia 2GRX */
			MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core, logen_2g_phase, 1)
			/* Power-Down gia 2GTX */
			MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core, logen_2g_phase_rx_pd,
					1)
			MOD_RADIO_REG_20710_ENTRY(pi, AUXPGA_OVR1, core, ovr_auxpga_i_pu, 1)
			MOD_RADIO_REG_20710_ENTRY(pi,	AUXPGA_CFG1, core, auxpga_i_pu, 1)
			/* ADC ref current source: 1 = cleaner reference for better ADC slope */
			MOD_RADIO_REG_20710_ENTRY(pi, RXADC_CFG1, core, rxadc_20u_ctal_uncal_en,
					1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

static uint
wlc_phy_init_radio_prefregs_allbands(phy_info_t *pi, const radio_20xx_prefregs_t *radioregs)
{
	uint i;

	for (i = 0; radioregs[i].address != 0xffff; i++) {
		phy_utils_write_radioreg(pi, radioregs[i].address,
		                (uint16)radioregs[i].init);
		if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			OSL_DELAY(100);
		}
	}

	return i;
}

void
wlc_phy_chanspec_radio20698_setup(phy_info_t *pi, chanspec_t chanspec,
	uint8 toggle_logen_reset, uint8 logen_mode)
{
	/* 20698_procs.tcl r708059: 20698_fc */

	phy_ac_chanmgr_info_t *chanmgri = pi->u.pi_acphy->chanmgri;
	const chan_info_radio20698_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20698(pi, chanspec, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		if (logen_mode == 4)
			wlc_phy_logen_reset(pi, 3);
		else
			wlc_phy_logen_reset(pi, 0);
	}

	phy_ac_chanmgr_get_data(chanmgri)->vco_pll_adjust_state = TRUE;
	if (logen_mode == 4) {
		wlc_phy_radio20698_rffe_tune(pi, chanspec, chan_info, 3);
		wlc_phy_radio20698_upd_band_related_reg(pi, chan_info->freq, logen_mode);
		wlc_phy_radio20698_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20698,
		                            chan_info->freq, logen_mode);
	} else {
		/* rffe tuning */
		wlc_phy_radio20698_rffe_tune(pi, chanspec, chan_info, 0);

		/* band related settings */
		wlc_phy_radio20698_upd_band_related_reg(pi, chan_info->freq, 0);

		/* pll tuning */
		wlc_phy_radio20698_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20698,
		                            chan_info->freq, 0);
	}

	/* Do a VCO cal */
	wlc_phy_20698_radio_vcocal(pi, VCO_CAL_MODE_20698, VCO_CAL_COUPLING_MODE_20698, logen_mode);
}

static void
wlc_phy_chanspec_radio20704_setup(phy_info_t *pi, chanspec_t chanspec, uint8 toggle_logen_reset,
	uint8 logen_mode)
{
	/* 20704_procs.tcl r788667: 20704_fc */

	const chan_info_radio20704_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20704(pi, chanspec, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* rffe tuning */
	wlc_phy_radio20704_rffe_tune(pi, chan_info, 0);

	/* band related settings */
	wlc_phy_radio20704_upd_band_related_reg(pi, chan_info->freq, 0);

	/* pll tuning */
	wlc_phy_radio20704_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20704, chan_info->freq);

	/* Do a VCO cal */
	wlc_phy_20704_radio_vcocal(pi);
}

static void
wlc_phy_chanspec_radio20707_setup(phy_info_t *pi, chanspec_t chanspec,
		uint8 toggle_logen_reset, uint8 logen_mode)
{
	const chan_info_radio20707_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20707(pi, chanspec, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* rffe tuning */
	wlc_phy_radio20707_rffe_tune(pi, chan_info, 0);

	/* band related settings */
	wlc_phy_radio20707_upd_band_related_reg(pi, chan_info->freq, 0);

	/* pll tuning */
	wlc_phy_radio20707_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20707, chan_info->freq);

	/* Same constant being used as before - Needs to be set here every time, due to
	 * possible toggling after sanity check which was added for Arcadian issue
	 */
	if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
		MOD_RADIO_PLLREG_20707(pi, PLL_VCO2, 0, rfpll_vco_cap_mode,
		    RFPLL_VCO_CAP_MODE_DEC);
	}

	/* Do a VCO cal */
	wlc_phy_20707_radio_vcocal(pi);
}

void
wlc_phy_chanspec_radio20708_setup(phy_info_t *pi, chanspec_t chanspec,
		uint8 toggle_logen_reset, uint8 pll_num, uint8 logen_mode)
{
	const chan_info_radio20708_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

#ifdef BCMQT
	/* No radio module in Veloce so skip */
	//return;
#endif

	if (wlc_phy_chan2freq_20708(pi, chanspec, &chan_info) < 0)
		return;

	/* re-arrange the function orders below to match TCL implementation */

	/* rffe tuning */
	wlc_phy_radio20708_rffe_tune(pi, chan_info, pll_num);

	/* band related settings */
	wlc_phy_radio20708_upd_band_related_reg(pi, chan_info->freq, logen_mode);

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		if ((logen_mode == 2) || (logen_mode == 5))
			wlc_phy_logen_reset(pi, 3);
		else
			wlc_phy_logen_reset(pi, 0);
	}

	/* pll tuning */
	wlc_phy_radio20708_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20708,
		chan_info->freq, pll_num);

	/* Do a VCO cal */
	wlc_phy_20708_radio_vcocal(pi, VCO_CAL_MODE_20708, logen_mode);
}

static void
wlc_phy_chanspec_radio20709_setup(phy_info_t *pi, chanspec_t chanspec, uint8 toggle_logen_reset,
	uint8 logen_mode)
{
	/* 20709_procs.tcl r798817: 20709_fc */

	const chan_info_radio20709_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20709(pi, chanspec, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* rffe tuning */
	wlc_phy_radio20709_rffe_tune(pi, chan_info, 0);

	/* band related settings */
	wlc_phy_radio20709_upd_band_related_reg(pi, 0);

	/* pll tuning */
	wlc_phy_radio20709_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20709, chan_info->freq);

	/* Do a VCO cal */
	wlc_phy_20709_radio_vcocal(pi);
}

static void
wlc_phy_chanspec_radio20710_setup(phy_info_t *pi, chanspec_t chanspec, uint8 toggle_logen_reset,
	uint8 logen_mode)
{
	/* 20710_procs.tcl r788667: 20710_fc */

	const chan_info_radio20710_rffe_t *chan_info;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlc_phy_chan2freq_20710(pi, chanspec, &chan_info) < 0)
		return;

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	/* rffe tuning */
	wlc_phy_radio20710_rffe_tune(pi, chan_info, 0);

	/* band related settings */
	wlc_phy_radio20710_upd_band_related_reg(pi, chan_info->freq, 0);

	/* pll tuning */
	wlc_phy_radio20710_pll_tune(pi, pi->u.pi_acphy->radioi->pll_conf_20710, chan_info->freq);

	/* Do a VCO cal */
	wlc_phy_20710_radio_vcocal(pi);
}

int
wlc_phy_chan2freq_20698(phy_info_t *pi, chanspec_t chanspec,
	const chan_info_radio20698_rffe_t **chan_info)
{
	uint32 index;
	uint32 tbl_len = 0;
	uint8 channel = CHSPEC_CHANNEL(chanspec);
	const chan_info_radio20698_rffe_t *p_chan_info_tbl = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	tbl_len = phy_get_chan_tune_tbl_20698(pi, CHSPEC_BAND(chanspec), &p_chan_info_tbl);

	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel; index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(index < tbl_len);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

int
wlc_phy_chan2freq_20704(phy_info_t *pi, chanspec_t chanspec,
	const chan_info_radio20704_rffe_t **chan_info)
{
	uint16 index;
	uint16 tbl_len = 0;
	uint8 channel = CHSPEC_CHANNEL(chanspec);
	const chan_info_radio20704_rffe_t *p_chan_info_tbl = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Get the right table to use */
	tbl_len = wlc_get_20704_chan_tune_table(pi, chanspec, &p_chan_info_tbl);

	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
	     index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(0);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

int
wlc_phy_chan2freq_20707(phy_info_t *pi, chanspec_t chanspec,
		const chan_info_radio20707_rffe_t **chan_info)
{
	uint32 index;
	uint32 tbl_len = 0;
	const chan_info_radio20707_rffe_t *p_chan_info_tbl = NULL;
	uint8 channel =  CHSPEC_CHANNEL(chanspec);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	tbl_len = phy_get_chan_tune_tbl_20707(pi, CHSPEC_BAND(chanspec),
		&p_chan_info_tbl);
	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
		index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(index < tbl_len);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

int
wlc_phy_chan2freq_20708(phy_info_t *pi, chanspec_t chanspec,
		const chan_info_radio20708_rffe_t **chan_info)
{
	uint32 index;
	uint32 tbl_len = 0;
	const chan_info_radio20708_rffe_t *p_chan_info_tbl = NULL;
	uint8 channel =  CHSPEC_CHANNEL(chanspec);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	tbl_len = phy_get_chan_tune_tbl_20708(pi, CHSPEC_BAND(chanspec),
		&p_chan_info_tbl);
	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
		index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(index < tbl_len);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

int
wlc_phy_chan2freq_20709(phy_info_t *pi, chanspec_t chanspec,
	const chan_info_radio20709_rffe_t **chan_info)
{
	uint32 index;
	uint32 tbl_len = 0;
	uint8 channel = CHSPEC_CHANNEL(chanspec);
	const chan_info_radio20709_rffe_t *p_chan_info_tbl = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	tbl_len = wlc_get_20709_chan_tune_table(pi, chanspec, &p_chan_info_tbl);

	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
	     index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(index < tbl_len);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

int
wlc_phy_chan2freq_20710(phy_info_t *pi, chanspec_t chanspec,
	const chan_info_radio20710_rffe_t **chan_info)
{
	uint16 index;
	uint16 tbl_len = 0;
	uint8 channel = CHSPEC_CHANNEL(chanspec);
	const chan_info_radio20710_rffe_t *p_chan_info_tbl = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Get the right table to use */
	tbl_len = wlc_get_20710_chan_tune_table(pi, chanspec, &p_chan_info_tbl);

	for (index = 0; index < tbl_len && p_chan_info_tbl[index].channel != channel;
	     index++);

	if (index >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
			pi->sh->unit, __FUNCTION__, channel));
		if (!ISSIM_ENAB(pi->sh->sih)) {
			/* Do not assert on QT since we leave the tables empty on purpose */
			ASSERT(0);
		}
		return -1;
	}
	*chan_info = p_chan_info_tbl + index;
	return p_chan_info_tbl[index].freq;
}

static void wlc_phy_radio20698_rffe_tune(phy_info_t *pi, chanspec_t chanspec,
	const chan_info_radio20698_rffe_t *chan_info_rffe, uint8 core)
{
	/* 20698_procs.tcl r708059: 20698_upd_tuning_tbl */
	/* 20698_tuning.tcl r708059: defines registers and fields */

	if (CHSPEC_IS6G(chanspec)) {
		/* 6G front end */
		ASSERT((RADIOREV(pi->pubpi->radiorev) >= 3));
		if (core != 3) {
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_mix_ctune,
				chan_info_rffe->u.val_6G.RFP0_logen_reg1_logen_mix_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
				chan_info_rffe->u.val_6G.RF0_logen_core_reg3_logen_lc_ctune);
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_mix_ctune,
				chan_info_rffe->u.val_6G.RFP1_logen_reg1_logen_mix_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
				chan_info_rffe->u.val_6G.RF1_logen_core_reg3_logen_lc_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
				chan_info_rffe->u.val_6G.RF2_logen_core_reg3_logen_lc_ctune);
		}
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 3, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF3_logen_core_reg3_logen_lc_ctune);
		if (core != 3) {
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 0, logen_rccr_tune,
			chan_info_rffe->u.val_6G.RF0_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 0, mx5g_tune,
			chan_info_rffe->u.val_6G.RF0_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 0, tx5g_pa_tune,
			chan_info_rffe->u.val_6G.RF0_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 0, pad5g_tune,
			chan_info_rffe->u.val_6G.RF0_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 0, rx5g_lna_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 0, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 1, logen_rccr_tune,
			chan_info_rffe->u.val_6G.RF1_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 1, mx5g_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 1, tx5g_pa_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 1, pad5g_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 1, rx5g_lna_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 1, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 2, logen_rccr_tune,
			chan_info_rffe->u.val_6G.RF2_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 2, mx5g_tune,
			chan_info_rffe->u.val_6G.RF2_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 2, tx5g_pa_tune,
			chan_info_rffe->u.val_6G.RF2_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 2, pad5g_tune,
			chan_info_rffe->u.val_6G.RF2_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 2, rx5g_lna_tune,
			chan_info_rffe->u.val_6G.RF2_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 2, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF2_rx5g_reg5_rx5g_mix_Cin_tune);
		}
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 3, logen_rccr_tune,
			chan_info_rffe->u.val_6G.RF3_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 3, mx5g_tune,
			chan_info_rffe->u.val_6G.RF3_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 3, tx5g_pa_tune,
			chan_info_rffe->u.val_6G.RF3_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 3, pad5g_tune,
			chan_info_rffe->u.val_6G.RF3_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 3, rx5g_lna_tune,
			chan_info_rffe->u.val_6G.RF3_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 3, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF3_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_PLLREG_20698(pi, XTAL2, 0, xtal_cmos_pg_ctrl0,
			chan_info_rffe->u.val_6G.RFP0_xtal2_xtal_cmos_pg_ctrl0);
		MOD_RADIO_PLLREG_20698(pi, PLL_LVLDO1, 0, ldo_1p0_ldo_LOGEN_vout_sel,
			chan_info_rffe->u.val_6G.RFP0_pll_lvldo1_ldo_1p0_ldo_LOGEN_vout_sel);
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO3, 0, ldo_1p8_ldo_CP_vout_sel,
			chan_info_rffe->u.val_6G.RFP0_pll_hvldo3_ldo_1p8_ldo_CP_vout_sel);
		MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 0, RefDoublerbuf_rstrg,
			chan_info_rffe->u.val_6G.RFP0_pll_refdoubler3_RefDoublerbuf_rstrg);
		MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 0, RefDoublerbuf_fstrg,
			chan_info_rffe->u.val_6G.RFP0_pll_refdoubler3_RefDoublerbuf_fstrg);
		MOD_RADIO_PLLREG_20698(pi, XTAL1, 0, xtal_LDO_Vctrl,
			chan_info_rffe->u.val_6G.RFP0_xtal1_xtal_LDO_Vctrl);
	} else if (CHSPEC_IS5G(chanspec)) {
		/* 5G front end */
		if (core != 3) {
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_mix_ctune,
				chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_mix_ctune,
				chan_info_rffe->u.val_5G.RFP1_logen_reg1_logen_mix_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
			MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
				chan_info_rffe->u.val_5G.RF2_logen_core_reg3_logen_lc_ctune);
		}
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 3, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF3_logen_core_reg3_logen_lc_ctune);
		if (core != 3) {
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 0, logen_rccr_tune,
			chan_info_rffe->u.val_5G.RF0_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 0, mx5g_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 0, tx5g_pa_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 0, pad5g_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 0, rx5g_lna_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 0, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 1, logen_rccr_tune,
			chan_info_rffe->u.val_5G.RF1_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 1, mx5g_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 1, tx5g_pa_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 1, pad5g_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 1, rx5g_lna_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 1, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 2, logen_rccr_tune,
			chan_info_rffe->u.val_5G.RF2_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 2, mx5g_tune,
			chan_info_rffe->u.val_5G.RF2_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 2, tx5g_pa_tune,
			chan_info_rffe->u.val_5G.RF2_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 2, pad5g_tune,
			chan_info_rffe->u.val_5G.RF2_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 2, rx5g_lna_tune,
			chan_info_rffe->u.val_5G.RF2_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 2, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF2_rx5g_reg5_rx5g_mix_Cin_tune);
		}
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG1, 3, logen_rccr_tune,
			chan_info_rffe->u.val_5G.RF3_logen_core_reg1_logen_rccr_tune);
		MOD_RADIO_REG_20698(pi, TX5G_MIX_REG2, 3, mx5g_tune,
			chan_info_rffe->u.val_5G.RF3_tx5g_mix_reg2_mx5g_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PA_REG4, 3, tx5g_pa_tune,
			chan_info_rffe->u.val_5G.RF3_tx5g_pa_reg4_tx5g_pa_tune);
		MOD_RADIO_REG_20698(pi, TX5G_PAD_REG3, 3, pad5g_tune,
			chan_info_rffe->u.val_5G.RF3_tx5g_pad_reg3_pad5g_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG1, 3, rx5g_lna_tune,
			chan_info_rffe->u.val_5G.RF3_rx5g_reg1_rx5g_lna_tune);
		MOD_RADIO_REG_20698(pi, RX5G_REG5, 3, rx5g_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF3_rx5g_reg5_rx5g_mix_Cin_tune);
		MOD_RADIO_PLLREG_20698(pi, XTAL2, 0, xtal_cmos_pg_ctrl0,
			chan_info_rffe->u.val_5G.RFP0_xtal2_xtal_cmos_pg_ctrl0);
		MOD_RADIO_PLLREG_20698(pi, PLL_LVLDO1, 0, ldo_1p0_ldo_LOGEN_vout_sel,
			chan_info_rffe->u.val_5G.RFP0_pll_lvldo1_ldo_1p0_ldo_LOGEN_vout_sel);
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO3, 0, ldo_1p8_ldo_CP_vout_sel,
			chan_info_rffe->u.val_5G.RFP0_pll_hvldo3_ldo_1p8_ldo_CP_vout_sel);
		MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 0, RefDoublerbuf_rstrg,
			chan_info_rffe->u.val_5G.RFP0_pll_refdoubler3_RefDoublerbuf_rstrg);
		MOD_RADIO_PLLREG_20698(pi, PLL_REFDOUBLER3, 0, RefDoublerbuf_fstrg,
			chan_info_rffe->u.val_5G.RFP0_pll_refdoubler3_RefDoublerbuf_fstrg);
		MOD_RADIO_PLLREG_20698(pi, XTAL1, 0, xtal_LDO_Vctrl,
			chan_info_rffe->u.val_5G.RFP0_xtal1_xtal_LDO_Vctrl);
	} else {
		/* 2G front end */
		if (core != 3) {
		MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP1_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF2_logen_core_reg3_logen_lc_ctune);
		}
		MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, 3, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF3_logen_core_reg3_logen_lc_ctune);
		if (core != 3) {
		MOD_RADIO_REG_20698(pi, TX2G_MIX_REG4, 0, mx2g_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20698(pi, TX2G_PAD_REG3, 0, pad2g_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20698(pi, LNA2G_REG1, 0, lna2g_lna1_freq_tune,
			chan_info_rffe->u.val_2G.RF0_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20698(pi, RX2G_REG3, 0, rx2g_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF0_rx2g_reg3_rx2g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, TX2G_MIX_REG4, 1, mx2g_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20698(pi, TX2G_PAD_REG3, 1, pad2g_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20698(pi, LNA2G_REG1, 1, lna2g_lna1_freq_tune,
			chan_info_rffe->u.val_2G.RF1_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20698(pi, RX2G_REG3, 1, rx2g_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF1_rx2g_reg3_rx2g_mix_Cin_tune);
		MOD_RADIO_REG_20698(pi, TX2G_MIX_REG4, 2, mx2g_tune,
			chan_info_rffe->u.val_2G.RF2_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20698(pi, TX2G_PAD_REG3, 2, pad2g_tune,
			chan_info_rffe->u.val_2G.RF2_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20698(pi, LNA2G_REG1, 2, lna2g_lna1_freq_tune,
			chan_info_rffe->u.val_2G.RF2_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20698(pi, RX2G_REG3, 2, rx2g_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF2_rx2g_reg3_rx2g_mix_Cin_tune);
		}
		MOD_RADIO_REG_20698(pi, TX2G_MIX_REG4, 3, mx2g_tune,
			chan_info_rffe->u.val_2G.RF3_tx2g_mix_reg4_mx2g_tune);
		MOD_RADIO_REG_20698(pi, TX2G_PAD_REG3, 3, pad2g_tune,
			chan_info_rffe->u.val_2G.RF3_tx2g_pad_reg3_pad2g_tune);
		MOD_RADIO_REG_20698(pi, LNA2G_REG1, 3, lna2g_lna1_freq_tune,
			chan_info_rffe->u.val_2G.RF3_lna2g_reg1_lna2g_lna1_freq_tune);
		MOD_RADIO_REG_20698(pi, RX2G_REG3, 3, rx2g_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF3_rx2g_reg3_rx2g_mix_Cin_tune);
	}
}

static void wlc_phy_radio20704_rffe_tune(phy_info_t *pi,
	const chan_info_radio20704_rffe_t *chan_info_rffe, uint8 core)
{
	/* 20704_procs.tcl r783913: 20704_upd_tuning_tbl */
	/* 20704_rev0_tuning.tcl r783701: defines registers and fields */

	switch (pi->radio_chanspec & WL_CHANSPEC_BAND_MASK) {
	case WL_CHANSPEC_BAND_2G:
		/* 2G front end */
		MOD_RADIO_PLLREG_20704(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_2G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_2G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		break;
	case WL_CHANSPEC_BAND_5G:
		/* 5G front end */
		MOD_RADIO_PLLREG_20704(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_5G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_5G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_5G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_5G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		break;
	case WL_CHANSPEC_BAND_6G:
		/* 6G front end, for now same as 5G */
		MOD_RADIO_PLLREG_20704(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_6G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20704(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_6G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_6G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_6G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20704(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_6G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		break;
	default:
		ASSERT(0);
	}
}

static void wlc_phy_radio20707_rffe_tune(phy_info_t *pi,
	const chan_info_radio20707_rffe_t *chan_info_rffe, uint8 core)
{

	if (CHSPEC_IS6G(pi->radio_chanspec)) {
		/* 5G front end */
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_6G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF2_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 2, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF2_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 0, tx5g_mx_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 1, tx5g_mx_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 2, tx5g_mx_tune,
			chan_info_rffe->u.val_6G.RF2_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 0, tx5g_pad_tune,
			chan_info_rffe->u.val_6G.RF0_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 1, tx5g_pad_tune,
			chan_info_rffe->u.val_6G.RF1_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 2, tx5g_pad_tune,
			chan_info_rffe->u.val_6G.RF2_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_div3en,
			chan_info_rffe->u.val_6G.RFP0_logen_reg0_logen_div3en);
	} else if (CHSPEC_IS5G(pi->radio_chanspec)) {
		/* 5G front end */
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF2_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 2, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF2_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 0, tx5g_mx_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 1, tx5g_mx_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_MIX_REG4, 2, tx5g_mx_tune,
			chan_info_rffe->u.val_5G.RF2_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 0, tx5g_pad_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 1, tx5g_pad_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX5G_PAD_REG3, 2, tx5g_pad_tune,
			chan_info_rffe->u.val_5G.RF2_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_div3en,
			chan_info_rffe->u.val_5G.RFP0_logen_reg0_logen_div3en);
	} else {
		/* 2G front end */
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, RX5G_REG1, 2, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF2_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, LOGEN_CORE_REG3, 2, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF2_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20707(pi, TX2G_MIX_REG4, 0, tx2g_mx_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_tx2g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX2G_MIX_REG4, 1, tx2g_mx_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_tx2g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX2G_MIX_REG4, 2, tx2g_mx_tune,
			chan_info_rffe->u.val_2G.RF2_tx2g_mix_reg4_tx2g_mx_tune);
		MOD_RADIO_REG_20707(pi, TX2G_PAD_REG3, 0, tx2g_pad_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_pad_reg3_tx2g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX2G_PAD_REG3, 1, tx2g_pad_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_pad_reg3_tx2g_pad_tune);
		MOD_RADIO_REG_20707(pi, TX2G_PAD_REG3, 2, tx2g_pad_tune,
			chan_info_rffe->u.val_2G.RF2_tx2g_pad_reg3_tx2g_pad_tune);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_div3en,
			chan_info_rffe->u.val_2G.RFP0_logen_reg0_logen_div3en);
	}
}

static void wlc_phy_radio20708_rffe_tune(phy_info_t *pi,
	const chan_info_radio20708_rffe_t *chan_info_rffe, uint8 pllcore)
{
	/* 20708_procs.tcl r867743/r867754: 20708_upd_tuning_tbl */
	/* 20708_rev0_tuning.tcl r868422 defines registers and fields */

	// per-core RF regs
	LOAD_RF_TUNING_REG_20708(pllcore, LOGEN_CORE_REG1, logen_core_buf_ctune);
	LOAD_RF_TUNING_REG_20708(pllcore, TX2G_MIX_REG4,   txdb_mx_tune);
	LOAD_RF_TUNING_REG_20708(pllcore, TXDB_PAD_REG3,   tx_pad_tune);
	LOAD_RF_TUNING_REG_20708(pllcore, RX5G_REG1,       rxdb_lna_tune);
	LOAD_RF_TUNING_REG_20708(pllcore, TX2G_PAD_REG2,   tx_pad_xfmr_sw);
	LOAD_RF_TUNING_REG_20708(pllcore, TX2G_MIX_REG0,   tx_mx_xfmr_sw_s);
	LOAD_RF_TUNING_REG_20708(pllcore, TX2G_MIX_REG0,   tx_mx_xfmr_sw_p);
	LOAD_RF_TUNING_REG_20708(pllcore, RX5G_REG4,       rxdb_gm_cc);

	// PLL regs
	LOAD_RFP_TUNING_REG_20708(pllcore, LOGEN_REG2, logen_ctune);
}

static void wlc_phy_radio20709_rffe_tune(phy_info_t *pi,
	const chan_info_radio20709_rffe_t *chan_info_rffe, uint8 core)
{
	/* 20709_procs.tcl r798817: 20709_upd_tuning_tbl */
	/* 20709_rev0_tuning.tcl r778784 defines registers and fields */

	if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
		/* 5G front end */
		MOD_RADIO_PLLREG_20709(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20709(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20709(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20709(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20709(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20709(pi, TX5G_MIX_REG4, 0, tx5g_mx_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20709(pi, TX5G_MIX_REG4, 1, tx5g_mx_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_mix_reg4_tx5g_mx_tune);
		MOD_RADIO_REG_20709(pi, TX5G_PAD_REG3, 0, tx5g_pad_tune,
			chan_info_rffe->u.val_5G.RF0_tx5g_pad_reg3_tx5g_pad_tune);
		MOD_RADIO_REG_20709(pi, TX5G_PAD_REG3, 1, tx5g_pad_tune,
			chan_info_rffe->u.val_5G.RF1_tx5g_pad_reg3_tx5g_pad_tune);
	} else {
		/* 2G front end */
		MOD_RADIO_PLLREG_20709(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20709(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20709(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20709(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20709(pi, TX2G_MIX_REG4, 0, tx2g_mx_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_tx2g_mx_tune);
		MOD_RADIO_REG_20709(pi, TX2G_MIX_REG4, 1, tx2g_mx_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_tx2g_mx_tune);
		MOD_RADIO_REG_20709(pi, TX2G_PAD_REG3, 0, tx2g_pad_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_pad_reg3_tx2g_pad_tune);
		MOD_RADIO_REG_20709(pi, TX2G_PAD_REG3, 1, tx2g_pad_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_pad_reg3_tx2g_pad_tune);
		MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_div3en,
			chan_info_rffe->u.val_2G.RFP0_logen_reg0_logen_div3en);
	}
}

static void wlc_phy_radio20710_rffe_tune(phy_info_t *pi,
	const chan_info_radio20710_rffe_t *chan_info_rffe, uint8 core)
{
	/* 20710_procs.tcl r783913: 20710_upd_tuning_tbl */
	/* 20710_rev0_tuning.tcl r783701: defines registers and fields */

	switch (pi->radio_chanspec & WL_CHANSPEC_BAND_MASK) {
	case WL_CHANSPEC_BAND_2G:
		/* 2G front end */
		MOD_RADIO_PLLREG_20710(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_2G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_2G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_2G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_2G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_2G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_2G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_2G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		break;
	case WL_CHANSPEC_BAND_5G:
		/* 5G front end */
		MOD_RADIO_PLLREG_20710(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_5G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_5G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_5G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_5G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_5G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_5G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_5G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		break;
	case WL_CHANSPEC_BAND_6G:
		/* 6G front end, for now same as 5G */
		MOD_RADIO_PLLREG_20710(pi, LOGEN_REG1, 0, logen_mix_ctune,
			chan_info_rffe->u.val_6G.RFP0_logen_reg1_logen_mix_ctune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 0, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF0_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, LOGEN_CORE_REG3, 1, logen_lc_ctune,
			chan_info_rffe->u.val_6G.RF1_logen_core_reg3_logen_lc_ctune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 0, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG1, 1, rxdb_lna_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg1_rxdb_lna_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 0, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF0_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20710(pi, RX5G_REG5, 1, rxdb_mix_Cin_tune,
			chan_info_rffe->u.val_6G.RF1_rx5g_reg5_rxdb_mix_Cin_tune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 0, txdb_mx_tune,
			chan_info_rffe->u.val_6G.RF0_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, 1, txdb_mx_tune,
			chan_info_rffe->u.val_6G.RF1_tx2g_mix_reg4_txdb_mx_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 0, txdb_pad5g_tune,
			chan_info_rffe->u.val_6G.RF0_txdb_pad_reg3_txdb_pad5g_tune);
		MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, 1, txdb_pad5g_tune,
			chan_info_rffe->u.val_6G.RF1_txdb_pad_reg3_txdb_pad5g_tune);
		break;
	default:
		ASSERT(0);
	}
}

static void
wlc_phy_radio20698_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq, uint8 logen_mode)
{
	/* 20698_procs.tcl r844802: 20698_upd_band_related_reg */

	uint8 core;

	if (chan_freq < 3000) {
		if (logen_mode == 4)
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div3en, 1);
		else
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div3en, 1);

		if (RADIOREV(pi->pubpi->radiorev) >= 3) {
			if (logen_mode == 4) {
				//RFP1
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_vco_delay_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_vco_delay, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div_sel, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1,
						logen_mixer_bufmode_en, 0);
			} else {
				//RFP0
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_vco_delay_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_vco_delay, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div_sel, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0,
						logen_mixer_bufmode_en, 0);
			}
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG5, core,
						logen_rneg_cneg_sel, 0);
			}
		}

		FOREACH_CORE(pi, core) {
			if ((logen_mode != 4) || (logen_mode == 4 && core == 3)) {
			RADIO_REG_LIST_START
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
			                              logen_div2_pu, 0x1)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
			                              logen_buf_2G_pu, 0x1)
			    MOD_RADIO_REG_20698_ENTRY(pi, TXDAC_REG3, core,
			                              i_config_IQDACbuf_cm_2g_sel, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_low_pu, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_mid_pu, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_high_pu, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_low_pu, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_mid_pu, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_high_pu, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	} else if ((chan_freq < 6100) && (chan_freq != 5995)) {
		if (logen_mode == 4)
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div3en, 0);
		else
			MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div3en, 0);

		if (RADIOREV(pi->pubpi->radiorev) >= 3) {
			if (logen_mode == 4) {
				//RFP1
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_vco_delay_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_vco_delay, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div_sel, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1,
						logen_mixer_bufmode_en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_bias_mix_qboost, 4);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_bias_mix, 4);
			} else {
				//RFP0
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_vco_delay_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_vco_delay, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div_sel, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0,
						logen_mixer_bufmode_en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 4);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_bias_mix, 4);
			}
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG5, core,
						logen_rneg_cneg_sel, 0);
			}
		}

		FOREACH_CORE(pi, core) {
			if ((logen_mode != 4) || (logen_mode == 4 && core == 3)) {
			RADIO_REG_LIST_START
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
			                              logen_gm_local_idac104u, 5)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
			                              logen_gmin_south_idac104u, 5)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG4, core,
			                              logen_gmin_north_idac104u, 5)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG3, core,
			                              logen_lc_idac26u, 3)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG3, core,
			                              logen_spare0_idac26u, 3)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG2, core,
			                              logen_tx_rccr_idac26u, 4)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
			                              logen_div2_pu, 0x0)
			    MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
			                              logen_buf_2G_pu, 0x0)
			    MOD_RADIO_REG_20698_ENTRY(pi, TXDAC_REG3, core,
			                              i_config_IQDACbuf_cm_2g_sel, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_low_pu, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_mid_pu, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG5, core,
			                              rx2g_wrssi1_high_pu, 0)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_low_pu, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_mid_pu, 1)
			    MOD_RADIO_REG_20698_ENTRY(pi, RX5G_WRSSI, core,
			                              rx5g_wrssi1_high_pu, 1)
			RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	} else { // 6G settings
		ASSERT(CHSPEC_IS6G(pi->radio_chanspec));
		if (RADIOREV(pi->pubpi->radiorev) >= 3) {
			if (logen_mode == 4) {
				//RFP1
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_vco_delay_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_vco_delay, 3);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div3en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 1, logen_div_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 1,
						logen_mixer_bufmode_en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_bias_mix_qboost, 7);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 1, logen_bias_mix, 7);
			} else {
				//RFP0
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_vco_delay_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_vco_delay, 3);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_pu, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div3en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_x2_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mux_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG0, 0, logen_div_sel, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0, logen_mixer_pu, 1);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG4, 0,
						logen_mixer_bufmode_en, 0);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 7);
				MOD_RADIO_PLLREG_20698(pi, LOGEN_REG1, 0, logen_bias_mix, 7);
			}

			FOREACH_CORE(pi, core) {
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, core,
						logen_lc_idac26u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG3, core,
						logen_spare0_idac26u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG5, core,
						logen_rneg_cneg_sel, 1);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 7);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x0);
				MOD_RADIO_REG_20698(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x0);
				MOD_RADIO_REG_20698(pi, TXDAC_REG3, core,
						i_config_IQDACbuf_cm_2g_sel, 0);
				MOD_RADIO_REG_20698(pi, RX2G_REG5, core,
						rx2g_wrssi1_low_pu, 0);
				MOD_RADIO_REG_20698(pi, RX2G_REG5, core,
						rx2g_wrssi1_mid_pu, 0);
				MOD_RADIO_REG_20698(pi, RX2G_REG5, core,
						rx2g_wrssi1_high_pu, 0);
				MOD_RADIO_REG_20698(pi, RX5G_WRSSI, core,
						rx5g_wrssi1_low_pu, 1);
				MOD_RADIO_REG_20698(pi, RX5G_WRSSI, core,
						rx5g_wrssi1_mid_pu, 1);
				MOD_RADIO_REG_20698(pi, RX5G_WRSSI, core,
						rx5g_wrssi1_high_pu, 1);
			}
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20698(pi, TX2G_MIX_REG2, core, tx2g_mx_idac_bb, 15);
		if (RADIOREV(pi->pubpi->radiorev) >= 3) {
			// Better mixer linearity together with using both mixer cores in gaintable
			MOD_RADIO_REG_20698(pi, TX5G_MIX_REG1, core, tx5g_idac_mx_bbdc, 15);
		} else {
			MOD_RADIO_REG_20698(pi, TX5G_MIX_REG1, core, tx5g_idac_mx_bbdc, 26);
		}
	}
}

static void
wlc_phy_radio20704_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq, uint8 logen_mode)
{
	/* 20704_procs.tcl r853242: 20704_upd_band_related_reg */

	uint8 core;

	/* NOTE: settings of logen_div3en are done in wlc_phy_radio20704_pll_tune */
	if (chan_freq <= 3000) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_div2_setb, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x3)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x3)
			RADIO_REG_LIST_EXECUTE(pi, core);

			if (RADIOREV(pi->pubpi->radiorev) > 2) {
				/* Recovering 2 dB gain lost by PAD offtune for
				 * radiorev 3 and up.
				 */
				MOD_RADIO_REG_20704(pi, TX2G_MIX_REG0, core,
						txdb_mx_gm_offset_en, 1);
			}

			RADIO_REG_LIST_START
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
						logen_6g_div_sel, 1)
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
						logen_6g_vco_delay_pu, 0)
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
						logen_6g_delay_p, 3)
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
						logen_6g_vco_delay, 0)
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
						logen_pu, 1)
				MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG1, 0,
						logen_bias_mix, 5)
			RADIO_REG_LIST_EXECUTE(pi, 0);
		}
		if (pi->epagain2g == 2) {
			// load iPA tuning stuff
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG1,
							core, tx2g_pa_idac_cas, 0x32)
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG2,
						core, tx2g_pa_idac_gm, 0x10)
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0,
							core, tx2g_pa_gm_bias_bw, 0x3)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}

			if (RADIOREV(pi->pubpi->radiorev) > 0) {
				/* 63178A1 and above */
				FOREACH_CORE(pi, core) {
					RADIO_REG_LIST_START
						MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG1,
							core, tx2g_pa_idac_cas, 0x28)
						MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PAD_REG1,
							core, txdb_pad_idac_gm, 0x6)
						MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3,
							core, txdb_pad_idac_cas, 0x13)
						MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PAD_REG2,
							core, txdb_pad_idac_tune, 0xf)
						MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PAD_REG0,
							core, txdb_pad_bias_cas_off, 0x9)
					RADIO_REG_LIST_EXECUTE(pi, core);
				}
			}
		} else {
			//load ePA tuning stuf
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG1,
							core, tx2g_pa_idac_cas, 0x1e)
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG2,
							core, tx2g_pa_idac_gm, 0x5)
					MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0,
							core, tx2g_pa_gm_bias_bw, 0x3)
					MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3,
							core, txdb_pad_ind_short, 0x0)
					MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3,
							core, txdb_pad5g_tune, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	}
	else if (chan_freq < 6290) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG0, core,
						tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x5)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x3)
			RADIO_REG_LIST_EXECUTE(pi, core);
			if (chan_freq < 6000) {
				MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1);
			} else {
				MOD_RADIO_REG_20704(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x0);
			}
		}

		RADIO_REG_LIST_START
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_div_sel, 1)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
					logen_6g_vco_delay_pu, 0)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_delay_p, 3)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_vco_delay, 0)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
					logen_pu, 1)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG1, 0,
					logen_bias_mix, 5)
		RADIO_REG_LIST_EXECUTE(pi, 0);
	}
	else {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG0, core,
						tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x0)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x7)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x7)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x7)
				MOD_RADIO_REG_20704_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x7)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}

		RADIO_REG_LIST_START
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_div_sel, 0)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
					logen_6g_vco_delay_pu, 1)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG0, 0,
					logen_pu, 0)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_REG1, 0,
					logen_bias_mix, 7)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_delay_p, 0)
			MOD_RADIO_PLLREG_20704_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_vco_delay, 3)
		RADIO_REG_LIST_EXECUTE(pi, 0);
	}
}

static void
wlc_phy_radio20707_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq, uint8 logen_mode)
{

	uint8 core;
	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(pi->radio_chanspec),
	                           CHSPEC_IS6G(pi->radio_chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(pi->radio_chanspec) ? WF_CHAN_FACTOR_5_G
	                                                          :  WF_CHAN_FACTOR_2_4_G);
	if (CHSPEC_IS2G(pi->radio_chanspec)) {

		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_div_sel, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_6g_vco_delay_pu, 0);

		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_sel, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_delay_p, 3);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mixer_bufmode_en, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mux_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_vco_delay, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 7);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix, 5);

		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rx_db_mux_sel, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_txbuf_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_txbuf_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_rxbuf_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_rxbuf_pu, 1)

				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gm_local_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_south_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_north_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_lc_idac26u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_spare0_idac26u, 3)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rneg_cneg_sel, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG2, core,
					logen_tx_rccr_idac26u, 4)
				/* Reduces the IQ flicker noise */
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_5g_tx_rccr_iqbias_short, 1)
				/* 2G iPA/ePA gaintables are calibrated with pad_gain_offset=0 */
				MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG0, core,
					tx2g_pad_gain_offset_en, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		if (pi->epagain2g == 2) {
			/* load 2G iPA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG1,
							core, tx2g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG2,
						core, tx2g_ipa_idac_gm, 16)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0,
							core, tx2g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0,
							core, tx2g_ipa_bias_bw, 3)
					/* load 2G iPA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_MIX_REG2,
							core, tx2g_mx_idac_bb, 12)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_gm, 6)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG3,
							core, tx2g_pad_idac_cas, 18)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
				if (RADIOMAJORREV(pi) == 0 && RADIOMAJORREV(pi) == 0 &&
					core == 2) {
					/* 6710A0 Core2 instability */
					MOD_RADIO_REG_20707(pi, TX2G_IPA_REG1,
							core, tx2g_ipa_idac_cas, 30);
					MOD_RADIO_REG_20707(pi, TX2G_IPA_REG2,
							core, tx2g_ipa_idac_gm, 12);
				}
			}
		} else {
			/* load 2G ePA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG1,
							core, tx2g_ipa_idac_cas, 60)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG2,
						core, tx2g_ipa_idac_gm, 4)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0,
							core, tx2g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_IPA_REG0,
							core, tx2g_ipa_bias_bw, 3)
					/* load 2G ePA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_MIX_REG2,
							core, tx2g_mx_idac_bb, 10)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_gm, 1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG3,
							core, tx2g_pad_idac_cas, 2)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_cas_off, 2)
					MOD_RADIO_REG_20707_ENTRY(pi, TX2G_PAD_REG1,
							core, tx2g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	} else if (fc < 6290) { /* 5G and low 6G */

		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_div_sel, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_6g_vco_delay_pu, 0);

		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_sel, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_delay_p, 3);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mixer_bufmode_en, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mux_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_vco_delay, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 7);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix, 5);

		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_txbuf_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_txbuf_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_rxbuf_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_rxbuf_pu, 1)

				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gm_local_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_south_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_north_idac104u, 5)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_lc_idac26u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_spare0_idac26u, 3)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rneg_cneg_sel, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG2, core,
					logen_tx_rccr_idac26u, 4)
				/* Reduces the IQ flicker noise */
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_5g_tx_rccr_iqbias_short, 1)
				/* 5G iPA/ePA gaintables are calibrated with pad_gain_offset=0 */
				MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG0, core,
					tx5g_pad_gain_offset_en, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		if (pi->epagain5g == 2) {
			/* load 5G iPA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					/* 5G iPA needs high cas for flat IM3 responce */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 35)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 36)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 5G iPA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG0,
							core, tx5g_mx_gm_offset_en, 1)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 16)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 12)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			/* load 5G ePA tuning stuff (ePA mode is with less iPA slices/higher bias)
			 */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 45)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 5G ePA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 12)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 8)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}

	} else { /* Settings for 6 GHz (fc>=6290MHz) */
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_div_sel, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_6g_vco_delay_pu, 1);

		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_clk_x2_sel, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_delay_p, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mixer_bufmode_en, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_mux_pu, 1);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_CORE_REG6, 0, logen_6g_vco_delay, 3);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG0, 0, logen_pu, 0);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix_qboost, 7);
		MOD_RADIO_PLLREG_20707(pi, LOGEN_REG1, 0, logen_bias_mix, 7);

		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20707_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x0)
				MOD_RADIO_REG_20707_ENTRY(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_txbuf_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_txbuf_pu, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_rxbuf_pu, 0)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_rxbuf_pu, 1)

				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gm_local_idac104u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_south_idac104u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG4, core,
					logen_gmin_north_idac104u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_lc_idac26u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG3, core,
					logen_spare0_idac26u, 7)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rneg_cneg_sel, 1)
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG2, core,
					logen_tx_rccr_idac26u, 7)
				/* Reduces the IQ flicker noise */
				MOD_RADIO_REG_20707_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_5g_tx_rccr_iqbias_short, 1)
				/* 5G iPA/ePA gaintables are calibrated with pad_gain_offset=0 */
				MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG0, core,
					tx5g_pad_gain_offset_en, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		if (pi->epagain5g == 2) {
			/* load 6G iPA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 35)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 36)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 6G iPA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 18)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 12)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			/* load 6G ePA tuning stuff (ePA mode is with less iPA slices/higher bias)
			 */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 45)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 6G ePA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 12)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 8)
					MOD_RADIO_REG_20707_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	}
}

static void
wlc_phy_radio20708_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq, uint8 logen_mode)
{
	uint8 core, nf;
	uint8 pll_core, pll_start, pll_end;
	uint32 ctune_const1[3] = {0x23deeecd, 0x8f7bbb34, 0x8f7bbb34};
	uint32 ctune_const2[3] = {0x169f79b4, 0x17395aee, 0x169f79b4};
	uint16 logen_ctune = 0;
	uint32 const1 = 0, const2 = 0, temp = 0;
	uint8 rfpll_synth_pu;

	if (chan_freq < 3000) {
		FOREACH_CORE(pi, core) {
			if (((logen_mode != 2) && (logen_mode != 5)) ||
				((logen_mode == 2 || logen_mode == 5) && core == 3)) {
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_dc_level, 3);
				if (RADIOMAJORREV(pi) < 2) {
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_low, 7);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_mid, 7);
					MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
						rxdb_spare, 0);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
						rxdb_wrssi1_atten, 2); //2G
				}
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_core_div2_pu, 1);
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, core,
					logen_core_div2_pu, 1);
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG4, core,
					logen_core_dll_ctrl_crude_delay_adj, 0);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_s, 0);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_p, 1);
				MOD_RADIO_REG_20708(pi, TXDB_CFG1_OVR, core,
					ovr_tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_REG0, core,
					tx_pad_band_sel, 0);
			        //MOD_RADIO_REG_20708(pi, TX2G_PAD_REG2, core,
			        //	tx_pad_xfmr_sw, 1);
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core,
					txdb_pad2g_tune, 3);
			}
		}
	} else if ((chan_freq >= 4800) && (chan_freq <= 5500)) {
		FOREACH_CORE(pi, core) {
			if (((logen_mode != 2) && (logen_mode != 5)) ||
				((logen_mode == 2 || logen_mode == 5) && core == 3)) {
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_dc_level, 3);
				if (RADIOMAJORREV(pi) < 2) {
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_low, 7);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_mid, 7);
					MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
						rxdb_spare, 0);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
						rxdb_wrssi1_atten, 0); //5G
				}
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG4, core,
					logen_core_dll_ctrl_crude_delay_adj, 16);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_s, 1);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_p, 1);
				MOD_RADIO_REG_20708(pi, TXDB_CFG1_OVR, core,
					ovr_tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_REG0, core,
					tx_pad_band_sel, 1);
			        //MOD_RADIO_REG_20708(pi, TX2G_PAD_REG2, core,
			        //	tx_pad_xfmr_sw, 1);
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core,
					txdb_pad2g_tune, 0);
			}
		}
	} else if ((chan_freq > 5500) && (chan_freq <= 6000)) {
		FOREACH_CORE(pi, core) {
			if (((logen_mode != 2) && (logen_mode != 5)) ||
				((logen_mode == 2 || logen_mode == 5) && core == 3)) {
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_dc_level, 3);
				if (RADIOMAJORREV(pi) < 2) {
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_low, 7);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_mid, 7);
					MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
						rxdb_spare, 0);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
						rxdb_wrssi1_atten, 0); //5G
				}
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG4, core,
					logen_core_dll_ctrl_crude_delay_adj, 16);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_s, 1);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_p, 1);
				MOD_RADIO_REG_20708(pi, TXDB_CFG1_OVR, core,
					ovr_tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_REG0, core,
					tx_pad_band_sel, 1);
			        //MOD_RADIO_REG_20708(pi, TX2G_PAD_REG2, core,
			        //	tx_pad_xfmr_sw, 0);
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core,
					txdb_pad2g_tune, 0);
			}
		}
	} else if ((chan_freq > 6000) && (chan_freq <= 6250)) {
		FOREACH_CORE(pi, core) {
			if (((logen_mode != 2) && (logen_mode != 5)) ||
				((logen_mode == 2 || logen_mode == 5) && core == 3)) {
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_dc_level, 3);
				//MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
				//	rxdb_wrssi1_atten, 0); //5G
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_s, 0);
			        //MOD_RADIO_REG_20708(pi, TX2G_MIX_REG0, core,
			        //	tx_mx_xfmr_sw_p, 1);
				MOD_RADIO_REG_20708(pi, TXDB_CFG1_OVR, core,
					ovr_tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_REG0, core,
					tx_pad_band_sel, 1);
			        //MOD_RADIO_REG_20708(pi, TX2G_PAD_REG2, core,
			        //	tx_pad_xfmr_sw, 0);
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core,
					txdb_pad2g_tune, 0);
				if (RADIOMAJORREV(pi) < 2) {
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_low, 15);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_mid, 7);
					MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
						rxdb_spare, 1);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
						rxdb_wrssi1_atten, 3);
				}
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG4, core,
					logen_core_dll_ctrl_crude_delay_adj, 16);
			}
		}
	} else {
		FOREACH_CORE(pi, core) {
			if (((logen_mode != 2) && (logen_mode != 5)) ||
				((logen_mode == 2 || logen_mode == 5) && core == 3)) {
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0);
				MOD_RADIO_REG_20708(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 1);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 3);
				MOD_RADIO_REG_20708(pi, RX5G_REG5, core,
					rxdb_mix_dc_level, 3);
				MOD_RADIO_REG_20708(pi, TXDB_CFG1_OVR, core,
					ovr_tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_REG0, core,
					tx_pad_band_sel, 1);
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core,
					txdb_pad2g_tune, 0);
				if (RADIOMAJORREV(pi) < 2) {
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_low, 15);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
						rxdb_wrssi1_threshold_mid, 7);
					MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
						rxdb_spare, 1);
					MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
						rxdb_wrssi1_atten, 3);
				}
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG4, core,
					logen_core_dll_ctrl_crude_delay_adj, 16);
			}
		}
	}

	// for 6715 A1
	FOREACH_CORE(pi, core) {
		if (((logen_mode != 2) && (logen_mode != 5)) ||
			((logen_mode == 2 || logen_mode == 5) && core == 3)) {
			if (RADIOMAJORREV(pi) == 1) {
				MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
					rxdb_wrssi1_threshold_low, 15);
				MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG1, core,
					rxdb_wrssi1_threshold_mid, 7);
				MOD_RADIO_REG_20708(pi, RXDB_SPARE, core,
					rxdb_spare, 1);
				MOD_RADIO_REG_20708(pi, RXDB_WRSSI1_REG0, core,
					rxdb_wrssi1_atten, 3);
			}
		}
	}

	// BELOW LNA BIAS
	FOREACH_CORE(pi, core) {
		if (((logen_mode != 2) && (logen_mode != 5)) ||
			((logen_mode == 2 || logen_mode == 5) && core == 3)) {
			if (RADIOMAJORREV(pi) >= 1) {
				MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 14);
			} else {
				if (chan_freq <= 3000) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 5);
				} else if ((chan_freq >= 4800) && (chan_freq <= 5825)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 6);
				} else if ((chan_freq > 5825) && (chan_freq <= 6350)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 5);
				} else if ((chan_freq > 6350) && (chan_freq <= 6550)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 5);
				} else if ((chan_freq > 6550) && (chan_freq <= 6750)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 6);
				} else if ((chan_freq > 6750) && (chan_freq <= 6950)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 8);
				} else if ((chan_freq > 6950) && (chan_freq <= 7200)) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 8);
				} else if (chan_freq > 7200) {
					MOD_RADIO_REG_20708(pi, RX5G_REG2, core, rxdb_lna_cc, 14);
				}
			}
		}
	}

	if (chan_freq <= 3000) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 12)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 24)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 12)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 24)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else if ((chan_freq >= 4800) && (chan_freq <= 5320)) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 10)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 18)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else if ((chan_freq > 5320) && (chan_freq <= 5825)) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 10)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 18)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else if ((chan_freq > 5825) && (chan_freq <= 6250)) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 10)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 18)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else if ((chan_freq > 6250) && (chan_freq <= 6750)) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 10)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 18)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_bb, 10)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_BIAS_REG0, core,
					txdb_bias_slope_mx_bb, 7)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG0, core,
					txdb_mx_gm_offset_en, 1)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG2, core,
					txdb_mx_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_MIX_REG3, core,
					txdb_mx_idac_lo, 3)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_gm, 18)
				MOD_RADIO_REG_20708_ENTRY(pi, TXDB_PAD_REG3, core,
					txdb_pad_idac_cas, 31)
				MOD_RADIO_REG_20708_ENTRY(pi, TX2G_PAD_REG1, core,
					txdb_pad_idac_cas_off, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}

	/* For A1 specific settings */
	if (RADIOMAJORREV(pi) >= 1) {
		FOREACH_CORE(pi, core) {
			if (RADIOMAJORREV(pi) >= 2) {
				MOD_RADIO_REG_20708(pi, TX2G_MIX_REG2, core, txdb_mx_idac_bb, 8);
				MOD_RADIO_REG_20708(pi, TX2G_MIX_REG1, core, tx_mx_bias_bgr_sel, 1);
			} else {
				MOD_RADIO_REG_20708(pi, TX2G_MIX_REG2, core, txdb_mx_idac_bb, 12);
			}
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG1, core, txdb_mx_idac_mirror_cas, 0);
			MOD_RADIO_REG_20708(pi, TX2G_MIX_REG3, core, txdb_mx_idac_mirror_lo, 15);
			if (chan_freq <= 3000) {
				MOD_RADIO_REG_20708(pi, TXDB_PAD_REG3, core, txdb_pad2g_tune, 0);
				if (RADIOMAJORREV(pi) >= 2) {
					MOD_RADIO_REG_20708(pi, TX2G_PAD_REG1, core,
						txdb_pad_idac_gm, 13);
					MOD_RADIO_REG_20708(pi, TX2G_PAD_REG1, core,
						txdb_pad_idac_mirror_cas, 3);
				}
			} else {
				if (RADIOMAJORREV(pi) >= 2) {
					MOD_RADIO_REG_20708(pi, TX2G_PAD_REG1, core,
						txdb_pad_idac_gm, 16);
					MOD_RADIO_REG_20708(pi, TX2G_PAD_REG1, core,
						txdb_pad_idac_mirror_cas, 3);
				}
			}
		}
	}

	if (logen_mode == 2 || logen_mode == 5) { /* for 3+1 mode */
		pll_start = pi->u.pi_acphy->radioi->maincore_on_pll1 ? 0 : 1;
		pll_end = pi->u.pi_acphy->radioi->maincore_on_pll1 ? 1 : 2;
	} else { /* for 4x4 mode */
		pll_start = 0;
		pll_end = 2;
	}

	for (pll_core = pll_start; pll_core < pll_end; pll_core++) {
		rfpll_synth_pu = READ_RADIO_PLLREGFLD_20708(pi, PLL_CFG1, pll_core, rfpll_synth_pu);
		if (chan_freq < 3000) {
			MOD_RADIO_PLLREG_20708(pi, LOGEN_CORE_REG6, pll_core, logen_delay, 3);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix_qboost, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_en_div3, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div34, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div5,
				(rfpll_synth_pu == 1) ? 1 : 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_en_x2, 1);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_sel_mux, 1);
		} else if ((chan_freq >= 4800) && (chan_freq <= 5905)) {
			MOD_RADIO_PLLREG_20708(pi, LOGEN_CORE_REG6, pll_core, logen_delay, 3);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix_qboost, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_en_div3, 1);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div34,
				(rfpll_synth_pu == 1) ? 1 : 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div5, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_en_x2, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_sel_mux, 0);
		} else {
			MOD_RADIO_PLLREG_20708(pi, LOGEN_CORE_REG6, pll_core, logen_delay, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix_qboost, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_bias_mix, 5);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_en_div3, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div34,
				(rfpll_synth_pu == 1) ? 1 : 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_pu_div5, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG1, pll_core, logen_en_x2, 0);
			MOD_RADIO_PLLREG_20708(pi, LOGEN_REG0, pll_core, logen_sel_mux, 0);
		}
	}

	/* logen_ctune related variables and calculations
	 * For now use the following and overwritting the corresponding entries in
	 * 20708_rev0_tuning.tcl. After finalizing logen optimization, we will use only
	 * the 20708_rev0_tuning.tcl.
	 *
	 * lc_tune = 1.15
	 * cstp_tune = 0.0183
	 * cpar_tune = [0.414, 0.425, 0.414] for 2G/5G/6G
	 *
	 * ctune_const1 (0.31.1) = 10^9/([16,4,4,]*pi^2*cstp_tune*lc_tune)
	 *                       = [300906342.49, 1203625369.95,1203625369.95] for 2G/5G/6G
	 * ctune_const2 (0.8.24) = cpar_tune/cstp_tune = [22.6230, 23.2240, 22.6230] for 2G/5G/6G
	 *
	 * logen_ctune = round(ctune_const1/fc^2 - ctune_const2)
	 */
	if (chan_freq < 3000) {
		const1 = ctune_const1[0];
		const2 = ctune_const2[0];
	} else if ((chan_freq >= 4800) && (chan_freq <= 5905)) {
		const1 = ctune_const1[1];
		const2 = ctune_const2[1];
	} else {
		const1 = ctune_const1[2];
		const2 = ctune_const2[2];
	}
	nf = math_fp_div_64(const1, chan_freq*chan_freq, 1, 0, &temp);
	temp = math_fp_round_32(temp, (nf - 24));
	logen_ctune = math_fp_round_32(temp - const2, 24);

	for (pll_core = pll_start; pll_core < pll_end; pll_core++) {
		MOD_RADIO_PLLREG_20708(pi, LOGEN_REG2, pll_core, logen_ctune, logen_ctune);
	}
}

static void
wlc_phy_radio20709_upd_band_related_reg(phy_info_t *pi, uint8 logen_mode)
{
	/* 20709_procs.tcl r849450 : 20709_upd_band_related_reg */
	uint8 core;
	uint8 channel;
	channel = CHSPEC_CHANNEL(pi->radio_chanspec);

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		if (channel == 1 || channel == 2 || channel == 3 ||
		channel == 4 || channel == 5 || channel == 8 ||
		channel == 9 || channel == 10 || channel == 11 ||
		channel == 12 || channel == 14) {
			MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_div3en, 0);
		} else {
			MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_div3en, 1);
		}
		MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0, logen_6g_div_sel, 1);
		MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_6g_vco_delay_pu, 0);

		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rx_db_mux_sel, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_2g_phase, 1)
				MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG0, core,
					tx2g_pad_gain_offset_en, 0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 1)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4, core,
					rxdb_gm_cc, 7)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG2, core,
					rxdb_lna_cc, 9)
				MOD_RADIO_REG_20709_ENTRY(pi, RX2G_REG4, core,
					rx_ldo_adj, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		if (PHY_IPA(pi)) {
			/* load 2G iPA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					/* Settings for the iPA mode (iPA/PAD) */
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG1, core,
						tx2g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG2, core,
						tx2g_ipa_idac_gm, 16)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG1, core,
						tx2g_pad_idac_gm, 5)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG3, core,
						tx2g_pad_idac_cas, 18)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG1, core,
						tx2g_pad_idac_mirror_cas, 6)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_MIX_REG2, core,
						tx2g_mx_idac_bb, 12)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			/* load 2G ePA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					/* load ePA tuning stuff */
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG1, core,
						tx2g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG2, core,
						tx2g_ipa_idac_gm, 8)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_IPA_REG0, core,
						tx2g_ipa_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG1, core,
						tx2g_pad_idac_gm, 4)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG3, core,
						tx2g_pad_idac_cas, 8)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG1, core,
						tx2g_pad_idac_mirror_cas, 6)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_MIX_REG2, core,
						tx2g_mx_idac_bb, 22)
					MOD_RADIO_REG_20709_ENTRY(pi, TX2G_PAD_REG3, core,
						tx2g_pad_tune, 0)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	} else {
		if (channel == 36 || channel == 64 || channel == 100 || channel == 108 ||
		channel == 110 || channel == 112 || channel == 114 || channel == 116 ||
		channel == 124 || channel == 126 || channel == 189) {
			MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_div3en, 1);
		} else {
			MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_div3en, 0);
		}
		MOD_RADIO_PLLREG_20709(pi, LOGEN_CORE_REG6, 0, logen_6g_div_sel, 1);
		MOD_RADIO_PLLREG_20709(pi, LOGEN_REG0, 0, logen_6g_vco_delay_pu, 0);

		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20709_ENTRY(pi, TXDAC_REG3, core,
					i_config_IQDACbuf_cm_2g_sel, 0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_pu, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG5, core,
					logen_div2_txbuf_pu, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG1, core,
					logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0, core,
					tx5g_ipa_bias_bw, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG5, core,
					rxdb_mix_Cin_tune, 0x3)
				MOD_RADIO_REG_20709_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_5g_tx_rccr_iqbias_short, 0x1)
				MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG0, core,
					tx5g_pad_gain_offset_en, 0x0)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG4, core,
					rxdb_gm_cc, 15)
				MOD_RADIO_REG_20709_ENTRY(pi, RX5G_REG2, core,
					rxdb_lna_cc, 15)
				MOD_RADIO_REG_20709_ENTRY(pi, RX2G_REG4, core,
					rx_ldo_adj, 4)
				MOD_RADIO_REG_20709_ENTRY(pi, TX5G_MIX_REG0, core,
					tx5g_mx_gm_offset_en, 1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		if (PHY_IPA(pi)) {
			/* load 5G iPA tuning stuff */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					/* 5G iPA needs high cas for flat IM3 responce */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 45)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 36)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 5G iPA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 18)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 12)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			/* load 5G ePA tuning stuff (ePA mode is with less iPA slices/higher bias)
			 */
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG1,
							core, tx5g_ipa_idac_cas, 40)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG2,
							core, tx5g_ipa_idac_gm, 45)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_gm_bias_bw, 3)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_IPA_REG0,
							core, tx5g_ipa_bias_bw, 3)
					/* load 5G ePA tuning stuff (for PAD/Mixer) */
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_MIX_REG2,
							core, tx5g_mx_idac_bb, 22)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_gm, 4)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG3,
							core, tx5g_pad_idac_cas, 8)
					MOD_RADIO_REG_20709_ENTRY(pi, TX5G_PAD_REG1,
							core, tx5g_pad_idac_mirror_cas, 6)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	}
}

static void
wlc_phy_radio20710_upd_band_related_reg(phy_info_t *pi, uint32 chan_freq, uint8 logen_mode)
{
	/* 20710_procs.tcl r925642: 20710_upd_band_related_reg */

	uint8 core;

	/* NOTE: settings of logen_div3en are done in wlc_phy_radio20710_pll_tune */
	if (chan_freq <= 3000) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_div2_setb, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x3)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x3)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
		RADIO_REG_LIST_START
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_div_sel, 1)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_6g_vco_delay_pu, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_delay_p, 3)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_vco_delay, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_pu, 1)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG1, 0,
					logen_bias_mix, 5)
		RADIO_REG_LIST_EXECUTE(pi, 0);
		if (PHY_IPA(pi)) {
			// load iPA tuning stuff
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG1,
							core, tx2g_pa_idac_cas, 0x32)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG2,
						core, tx2g_pa_idac_gm, 0x10)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0,
							core, tx2g_pa_gm_bias_bw, 0x3)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG1,
						core, tx2g_pa_idac_cas, 0x28)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PAD_REG1,
						core, txdb_pad_idac_gm, 0x6)
					MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3,
						core, txdb_pad_idac_cas, 0x13)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PAD_REG2,
						core, txdb_pad_idac_tune, 0xf)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PAD_REG0,
						core, txdb_pad_bias_cas_off, 0x9)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG0,
						core, txdb_mx_gm_offset_en, 0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			//load ePA tuning stuf
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG1,
							core, tx2g_pa_idac_cas, 0x1e)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG2,
							core, tx2g_pa_idac_gm, 0x5)
					MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0,
							core, tx2g_pa_gm_bias_bw, 0x3)
					MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3,
							core, txdb_pad_ind_short, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
				if (PHY_EPAPD(pi)) {
					MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, core,
							txdb_pad5g_tune, 0xd);
					MOD_RADIO_REG_20710(pi, TX2G_MIX_REG0, core,
							txdb_mx_gm_offset_en, 0x0);
				} else {
					MOD_RADIO_REG_20710(pi, TXDB_PAD_REG3, core,
							txdb_pad5g_tune, 0x0);
					MOD_RADIO_REG_20710(pi, TX2G_MIX_REG0, core,
							txdb_mx_gm_offset_en, 0x1);
				}
			}
		}
	}
	else if (chan_freq < 6290) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG0, core,
						tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x5)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x3)
			RADIO_REG_LIST_EXECUTE(pi, core);
			if (PHY_EPAPD(pi)) {
				MOD_RADIO_REG_20710(pi, TX2G_MIX_REG0, core,
						txdb_mx_gm_offset_en, 0x0);
			}
			if (chan_freq < 6000) {
				MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x1);
			} else {
				MOD_RADIO_REG_20710(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x0);
			}
		}

		RADIO_REG_LIST_START
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_div_sel, 1)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_6g_vco_delay_pu, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_delay_p, 3)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_vco_delay, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_pu, 1)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG1, 0,
					logen_bias_mix, 5)
		RADIO_REG_LIST_EXECUTE(pi, 0);
	} else {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_buf_2G_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_div2_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_OVR0, core,
						ovr_logen_div2_txbuf_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_rx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG1, core,
						logen_tx_db_mux_sel, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG0, core,
						logen_2g_div2_rx_setb, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna2g_bias_en, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG6, core,
						rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, RX5G_REG5, core,
						rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_bw, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_PA_REG0, core,
						tx2g_pa_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_tx2g_pa_bias_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG0, core,
						tx2g_bias_pu, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_CFG1_OVR, core,
						ovr_tx2g_bias_pu, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_CFG1_OVR, core,
						ovr_txdb_pad_sw_5G, 0x1)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_sw_2G, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TX2G_MIX_REG4, core,
						txdb_mx_ind_short_2ary, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, TXDB_PAD_REG3, core,
						txdb_pad_ind_short, 0x0)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gm_local_idac104u, 0x7)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_south_idac104u, 0x7)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG4, core,
						logen_gmin_north_idac104u, 0x7)
				MOD_RADIO_REG_20710_ENTRY(pi, LOGEN_CORE_REG2, core,
						logen_tx_rccr_idac26u, 0x7)
			RADIO_REG_LIST_EXECUTE(pi, core);
			if (PHY_EPAPD(pi)) {
				MOD_RADIO_REG_20710(pi, TX2G_MIX_REG0, core,
						txdb_mx_gm_offset_en, 0x0);
			}
		}

		RADIO_REG_LIST_START
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_div_sel, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_6g_vco_delay_pu, 1)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG0, 0,
					logen_pu, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_REG1, 0,
					logen_bias_mix, 7)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_delay_p, 0)
			MOD_RADIO_PLLREG_20710_ENTRY(pi, LOGEN_CORE_REG6, 0,
					logen_6g_vco_delay, 3)
		RADIO_REG_LIST_EXECUTE(pi, 0);
	}
}

static void
chanspec_tune_radio_20698(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	wlc_phy_radio20698_txdac_bw_setup(pi);
}

static void
chanspec_tune_radio_20704(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	wlc_phy_radio20704_txdac_bw_setup(pi);
}

static void
chanspec_tune_radio_20707(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	wlc_phy_radio20707_txdac_bw_setup(pi);
}

static void
chanspec_tune_radio_20708(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}
}

static void
chanspec_tune_radio_20709(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}
		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	wlc_phy_radio20709_txdac_bw_setup(pi);
}

static void
chanspec_tune_radio_20710(phy_info_t *pi)
{
	uint8 core;
	uint16 bbmult[PHY_CORE_MAX], bbmult_zero = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
		pi->dacratemode2g : pi->dacratemode5g;

	/* adc_cap_cal */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac) || CCT_BAND_CHG(pi_ac)) {
		FOREACH_CORE(pi, core) {
			wlc_phy_get_tx_bbmult_acphy(pi, &bbmult[core], core);
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult_zero, core);
		}

		wlc_phy_radio_afecal(pi);

		FOREACH_CORE(pi, core) {
			wlc_phy_set_tx_bbmult_acphy(pi, &bbmult[core], core);
		}
	}

	wlc_phy_radio20710_txdac_bw_setup(pi);
}

static void
wlc_phy_radio20698_rc_cal(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_rc_cal */

	uint8 cal, done, core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[] = {0x1, 0x0};
	uint8 sc[] = {0x0, 0x1};
	uint8 X1[] = {0x1c, 0x40};
	uint16 rccal_trc_set[] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < 2; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20698(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20698(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20698(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20698(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20698(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20698(pi, RCCAL_CFG2, 1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
		n0 = READ_RADIO_REGFLD_20698(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;
		n1 = READ_RADIO_REGFLD_20698(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d\n", pi->sh->unit, __FUNCTION__, n0));
		PHY_INFORM(("wl%d: %s n1 = %d\n", pi->sh->unit, __FUNCTION__, n1));

		if (cal == 0) {
			/* lpf  values */
			data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {
			/* dacbuf  */
			gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
			pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24) / (gmult_dacbuf);
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20698(pi, RCCAL_CFG0, 1, rccal_pu, 0);

	}
	/* nominal values when rc cal failes  */
	if (done == 0) {
		pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
		/* LPF: Default  = 4096 + 4% switch resistance = 4260 */
		data->rccal_gmult_rc = 4260;
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20698(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20698(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20698(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}
	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

#ifdef ATE_BUILD
	ate_buffer_regval[0].rccal_gmult_rc = data->rccal_gmult_rc;
	ate_buffer_regval[0].rccal_cmult_rc = data->rccal_cmult_rc;
#endif /* ATE_BUILD */
}

#define NUM_RC_CALS 2
static void
wlc_phy_radio20704_rc_cal(phy_info_t *pi)
{
	/* 20704_procs.tcl r770744: 20704_rc_cal */
	uint8 cal, done, core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[NUM_RC_CALS] = {0x1, 0x0};
	uint8 sc[NUM_RC_CALS] = {0x0, 0x1};
	uint8 X1[NUM_RC_CALS] = {0x1c, 0x40};
	uint16 rccal_trc_set[NUM_RC_CALS] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < NUM_RC_CALS; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20704(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20704(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20704(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20704(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20704(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20704(pi, RCCAL_CFG2, 1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
		n0 = READ_RADIO_REGFLD_20704(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;
		n1 = READ_RADIO_REGFLD_20704(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d n1 = %d\n", pi->sh->unit, __FUNCTION__, n0, n1));

		if (cal == 0) {
			/* nominal values when rc cal failes  */
			if (done == 0) {
				/* LPF: Default  = 4096 + 4% switch resistance = 4260 */
				data->rccal_gmult_rc = 4260;
			} else {
				/* lpf  values */
				data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
				data->rccal_gmult_rc = data->rccal_gmult;
			}
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {

			/* nominal values when rc cal failes  */
			if (done == 0) {
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
			} else {
				/* dacbuf  */
				gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24)
						/ (gmult_dacbuf);
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20704(pi, RCCAL_CFG0, 1, rccal_pu, 0);
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20704(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20704(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20704(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
}

static void
wlc_phy_radio20707_rc_cal(phy_info_t *pi)
{
	/* 20707_procs.tcl r770744: 20707_rc_cal */
	uint8 cal, done, core, ii, itr = 10;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[NUM_RC_CALS] = {0x1, 0x0};
	uint8 sc[NUM_RC_CALS] = {0x0, 0x1};
	uint8 X1[NUM_RC_CALS] = {0x1c, 0x40};
	uint16 rccal_trc_set[NUM_RC_CALS] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn, gmult, cmult, gmult_acc = 0, cmult_acc = 0;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);
	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < NUM_RC_CALS; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20707(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20707(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20707(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20707(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20707(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20707(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20707(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		for (ii = 0; ii < itr; ii++) {
			/* Start RCCAL */
			MOD_RADIO_REG_20707(pi, RCCAL_CFG2, 1, rccal_START, 0);
			MOD_RADIO_REG_20707(pi, RCCAL_CFG2, 1, rccal_START, 1);

			/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
			done = 0;
			for (rccal_itr = 0;
				 (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
				 rccal_itr++) {
				OSL_DELAY(100);
				done = READ_RADIO_REGFLD_20707(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
			}

			/* Stop RCCAL */
			MOD_RADIO_REG_20707(pi, RCCAL_CFG2, 1, rccal_START, 0);
			/* Make sure that RC Cal ran to completion */

			ASSERT(done);

			/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
			n0 = READ_RADIO_REGFLD_20707(pi, RF, RCCAL_CFG5, 1, rccal_N0);
			if (n0 & 0x1000)
				n0 = n0 - 0x2000;
			n1 = READ_RADIO_REGFLD_20707(pi, RF, RCCAL_CFG6, 1, rccal_N1);
			if (n1 & 0x1000)
				n1 = n1 - 0x2000;

			dn = n1 - n0;
			PHY_INFORM(("wl%d: %s n0 = %d n1 = %d\n", pi->sh->unit,
			            __FUNCTION__, n0, n1));

			if (cal == 0) {
				/* nominal values when rc cal failes  */
				if (done == 0) {
					/* LPF: Default  = 4096 + 4% switch resistance = 4260 */
					gmult = 4260;
				} else {
					/* lpf  values */
					gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
				}
				gmult_acc += gmult;
				PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
				            __FUNCTION__, gmult));
			} else if (cal == 1) {
				/* nominal values when rc cal failes  */
				if (done == 0) {
					cmult = 4096;
				} else {
					/* dacbuf  */
					gmult_dacbuf  = (gmult_dacbuf_k * dn) /
					    (pi->xtalfreq >> 12);
					cmult = (1 << 24) / (gmult_dacbuf);
				}
				cmult_acc += cmult;
				PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				            __FUNCTION__, cmult));
			}
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20707(pi, RCCAL_CFG0, 1, rccal_pu, 0);
	}

	/* Average it */
	data->rccal_gmult = gmult_acc / itr;
	data->rccal_gmult_rc = data->rccal_gmult;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult =  cmult_acc / itr;
	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20707(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20707(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20707(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

static void
wlc_phy_radio20708_rc_cal(phy_info_t *pi)
{
	/* 20707_procs.tcl r770744: 20707_rc_cal */
	uint8 cal, done;
	int core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[NUM_RC_CALS] = {0x1, 0x0};
	uint8 sc[NUM_RC_CALS] = {0x0, 0x1};
	uint8 X1[NUM_RC_CALS] = {0x1c, 0x40};
	uint16 rccal_trc_set[NUM_RC_CALS] = {0x22d, 0x10a};
	uint16 tia_cfg1_ovr[PHY_CORE_MAX], tia_reg7[PHY_CORE_MAX];
	uint16 lpf_ovr1[PHY_CORE_MAX], lpf_reg6[PHY_CORE_MAX];

	/* Note: in case Q1 value (currently Q1=1; Q1_cycles=4*(Q1+1)=8) is changed
	 * when chip is back, gmult_dacbuf_k and gmult_lpf_k should be re-calculated
	 * gmult_lpf_k    = floor(1/(1024.0e3 *  0.952e-12 * 2*log(2) *8)));
	 * gmult_dacbuf_k = floor(1/(1024.0e3 *  0.952e-12 * 2*log(2) *8)));
	 */
	uint32 gmult_lpf_k = 92494;
	uint32 gmult_dacbuf_k = 92494;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4960;
	data->rccal_gmult_rc     = 4960;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	/* Save tia/lpf regs */
	FOREACH_CORE(pi, core) {
	  tia_cfg1_ovr[core] = phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, TIA_CFG1_OVR, core));
	  tia_reg7[core] = phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, TIA_REG7, core));
	  lpf_ovr1[core] = phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, LPF_OVR1, core));
	  lpf_reg6[core] = phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, LPF_REG6, core));
	}

	/* unstable RC_CAL - keeping lpf/tia pu,bias = 0 seems to fix it */
	FOREACH_CORE(pi, core) {
	  MOD_RADIO_REG_20708(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 1);
	  MOD_RADIO_REG_20708(pi, TIA_REG7, core, tia_pu, 0);
	  MOD_RADIO_REG_20708(pi, TIA_CFG1_OVR, core, ovr_tia_bias_pu, 1);
	  MOD_RADIO_REG_20708(pi, TIA_REG7, core, tia_bias_pu, 0);

	  MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_bq_pu, 1);
	  MOD_RADIO_REG_20708(pi, LPF_REG6, core, lpf_bq_pu, 0);
	  MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_bias_pu, 1);
	  MOD_RADIO_REG_20708(pi, LPF_REG6, core, lpf_bias_pu, 0);
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x1);
	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < NUM_RC_CALS; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_P1, 0x1);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_R1, 0x1);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20708(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
				(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
				rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20708(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20708(pi, RCCAL_CFG2, 1, rccal_START, 0);
		/* Make sure that RC Cal ran to completion */

		ASSERT(done);

		/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
		n0 = READ_RADIO_REGFLD_20708(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;
		n1 = READ_RADIO_REGFLD_20708(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d n1 = %d\n", pi->sh->unit, __FUNCTION__, n0, n1));

		if (cal == 0) {
			/* nominal values when rc cal failes  */
			if (done == 0) {
				/* LPF: Default  = 4096 + 4% switch resistance = 4960 */
				data->rccal_gmult_rc = 4960;
			} else {
				/* lpf  values */
				data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
				data->rccal_gmult_rc = data->rccal_gmult;
			}
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
				__FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {

			/* nominal values when rc cal failes  */
			if (done == 0) {
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4960;
			} else {
				/* dacbuf  */
				gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24)
					/ (gmult_dacbuf);
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20708(pi, RCCAL_CFG0, 1, rccal_pu, 0);
	}

	/* Restore tia/lpf regs */
	FOREACH_CORE(pi, core) {
	  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, TIA_CFG1_OVR, core), tia_cfg1_ovr[core]);
	  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, TIA_REG7, core), tia_reg7[core]);
	  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, LPF_OVR1, core), lpf_ovr1[core]);
	  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, LPF_REG6, core), lpf_reg6[core]);
	}

	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20708(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20708(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20708(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x0);

	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

#ifdef ATE_BUILD
	ate_buffer_regval[0].rccal_gmult_rc = data->rccal_gmult_rc;
	ate_buffer_regval[0].rccal_cmult_rc = data->rccal_cmult_rc;
#endif /* ATE_BUILD */
}

#define NUM_RC_CALS 2
static void
wlc_phy_radio20709_rc_cal(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_rc_cal */
	uint8 cal, done, core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[NUM_RC_CALS] = {0x1, 0x0};
	uint8 sc[NUM_RC_CALS] = {0x0, 0x1};
	uint8 X1[NUM_RC_CALS] = {0x1c, 0x40};
	uint16 rccal_trc_set[NUM_RC_CALS] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < NUM_RC_CALS; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20709(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20709(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20709(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20709(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20709(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20709(pi, RCCAL_CFG2, 1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
		n0 = READ_RADIO_REGFLD_20709(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;
		n1 = READ_RADIO_REGFLD_20709(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d n1 = %d\n", pi->sh->unit, __FUNCTION__, n0, n1));

		if (cal == 0) {
			/* nominal values when rc cal failes  */
			if (done == 0) {
				/* LPF: Default  = 4096 + 4% switch resistance = 4260 */
				data->rccal_gmult_rc = 4260;
			} else {
				/* lpf  values */
				data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
				data->rccal_gmult_rc = data->rccal_gmult;
			}
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {

			/* nominal values when rc cal failes  */
			if (done == 0) {
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
			} else {
				/* dacbuf  */
				gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24)
						/ (gmult_dacbuf);
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20709(pi, RCCAL_CFG0, 1, rccal_pu, 0);
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20709(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20709(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20709(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
}

static void
wlc_phy_radio20710_rc_cal(phy_info_t *pi)
{
	/* 20710_procs.tcl r770744: 20710_rc_cal */
	uint8 cal, done, core;
	uint16 rccal_itr;
	int16 n0, n1;
	/* lpf, dacbuf */
	uint8 sr[NUM_RC_CALS] = {0x1, 0x0};
	uint8 sc[NUM_RC_CALS] = {0x0, 0x1};
	uint8 X1[NUM_RC_CALS] = {0x1c, 0x40};
	uint16 rccal_trc_set[NUM_RC_CALS] = {0x22d, 0x10a};

	uint32 gmult_dacbuf_k = 77705;
	uint32 gmult_lpf_k = 74800;
	uint32 gmult_dacbuf;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	uint32 dn;

	pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
	data->rccal_gmult_rc     = 4260;
	data->rccal_cmult_rc     = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	if (ISSIM_ENAB(pi->sh->sih)) {
		PHY_INFORM(("wl%d: %s BYPASSED in QT\n", pi->sh->unit, __FUNCTION__));
		return;
	}
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	/* Calibrate lpf, dacbuf cal 0 = lpf 1 = dacbuf */
	for (cal = 0; cal < NUM_RC_CALS; cal++) {
		/* Setup */
		/* Q!=2 -> 16 counts */
		MOD_RADIO_REG_20710(pi, RCCAL_CFG2, 1, rccal_Q1, 0x1);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG0, 1, rccal_sr, sr[cal]);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG0, 1, rccal_sc, sc[cal]);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG2, 1, rccal_X1, X1[cal]);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG3, 1, rccal_Trc, rccal_trc_set[cal]);

		/* Toggle RCCAL PU */
		MOD_RADIO_REG_20710(pi, RCCAL_CFG0, 1, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG0, 1, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_20710(pi, RCCAL_CFG2, 1, rccal_START, 0);
		MOD_RADIO_REG_20710(pi, RCCAL_CFG2, 1, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
		     (rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
		     rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_REGFLD_20710(pi, RF, RCCAL_CFG4, 1, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_20710(pi, RCCAL_CFG2, 1, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		/* N0 and N1 are 13 bit unsigned and can overflow/wrap around */
		n0 = READ_RADIO_REGFLD_20710(pi, RF, RCCAL_CFG5, 1, rccal_N0);
		if (n0 & 0x1000)
			n0 = n0 - 0x2000;
		n1 = READ_RADIO_REGFLD_20710(pi, RF, RCCAL_CFG6, 1, rccal_N1);
		if (n1 & 0x1000)
			n1 = n1 - 0x2000;

		dn = n1 - n0;
		PHY_INFORM(("wl%d: %s n0 = %d n1 = %d\n", pi->sh->unit, __FUNCTION__, n0, n1));

		if (cal == 0) {
			/* nominal values when rc cal failes  */
			if (done == 0) {
				/* LPF: Default  = 4096 + 4% switch resistance = 4260 */
				data->rccal_gmult_rc = 4260;
			} else {
				/* lpf  values */
				data->rccal_gmult = (gmult_lpf_k * dn) / (pi->xtalfreq >> 12);
				data->rccal_gmult_rc = data->rccal_gmult;
			}
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, data->rccal_gmult));
		} else if (cal == 1) {

			/* nominal values when rc cal failes  */
			if (done == 0) {
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = 4096;
			} else {
				/* dacbuf  */
				gmult_dacbuf  = (gmult_dacbuf_k * dn) / (pi->xtalfreq >> 12);
				pi->u.pi_acphy->radioi->rccal_dacbuf_cmult = (1 << 24)
						/ (gmult_dacbuf);
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf_cmult = %d\n", pi->sh->unit,
				__FUNCTION__, pi->u.pi_acphy->radioi->rccal_dacbuf_cmult));
		}
		/* Turn off rccal */
		MOD_RADIO_REG_20710(pi, RCCAL_CFG0, 1, rccal_pu, 0);
	}
	/* Programming lpf gmult and tia gmult */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20710(pi, TIA_GMULT_BQ1, core, tia_gmult_bq1, data->rccal_gmult_rc);
		MOD_RADIO_REG_20710(pi, LPF_GMULT_BQ2, core, lpf_gmult_bq2, data->rccal_gmult_rc);
		MOD_RADIO_REG_20710(pi, LPF_GMULT_RC, core, lpf_gmult_rc, data->rccal_gmult_rc);
	}

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	data->rccal_cmult_rc = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;
}

/* Global Radio PU's */
static void wlc_phy_radio20698_pwron_seq_phyregs(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_pwron_seq_phyregs */

	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything	 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20698_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void wlc_phy_radio20704_pwron_seq_phyregs(phy_info_t *pi)
{
	/* 20704_procs.tcl r770851: 20704_pwron_seq_phyregs */

	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything	 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20704_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void wlc_phy_radio20707_pwron_seq_phyregs(phy_info_t *pi)
{
	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything         */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20707_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void wlc_phy_radio20708_pwron_seq_phyregs(phy_info_t *pi)
{
	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything except tia/lpf as that causes rccal issue */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20708_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void wlc_phy_radio20709_pwron_seq_phyregs(phy_info_t *pi)
{
	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything         */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20709_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void wlc_phy_radio20710_pwron_seq_phyregs(phy_info_t *pi)
{
	/* 20710_procs.tcl r770851: 20710_pwron_seq_phyregs */

	/*  Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu */
	/*  rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1 */

	/* power down everything	 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* Using usleep of 100us below, so don't need these */
	WRITE_PHYREG(pi, Pllldo_resetCtrl, 0);
	WRITE_PHYREG(pi, Rfpll_resetCtrl, 0);
	WRITE_PHYREG(pi, Logen_AfeDiv_reset, 0x2000);

	/*  Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, 0);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);

	/*  Reset radio, jtag */
	WRITE_PHYREG(pi, RfctrlCmd, 0x7);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/*  Update preferred values */
	wlc_phy_radio20710_upd_prfd_values(pi);

	/*  {rfpll, pllldo, logen}_{pu, reset} */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, 0x2);

	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x180);
	OSL_DELAY(1000);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
}

static void
wlc_phy_radio20698_r_cal(phy_info_t *pi, uint8 mode)
{
	/* 20698_procs.tcl r708059: 20698_r_cal */

	/* Format: 20698_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, rcal_value, i, iter = 3, rcal_values[3];
	uint16 loop_iter;
	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20698_procs.tcl */
		rcal_value = 0x32;
		MOD_RADIO_PLLREG_20698(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20698(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		for (i = 0; i < iter; i++) {
			/* Run R Cal and use its output */
			MOD_RADIO_REG_20698(pi, GPAIO_SEL9, 2, toptestmux_pu, 1);
			MOD_RADIO_REG_20698(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
			MOD_RADIO_PLLREG_20698(pi, BG_REG3, 0, bg_rcal_pu, 1);
			MOD_RADIO_PLLREG_20698(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

			rcal_valid = 0;
			loop_iter = 0;
			while ((rcal_valid == 0) && (loop_iter <= 1000)) {
				/* 1 ms delay wait time */
				OSL_DELAY(1000);
				rcal_valid = READ_RADIO_PLLREGFLD_20698(pi, RCAL_CFG_NORTH, 0,
				                                        rcal_valid);
				loop_iter++;
				if (ISSIM_ENAB(pi->sh->sih)) {
					/* Bypass Rcal on QT to speed up dongle bring up */
					PHY_ERROR(("*** SIM: Bypass RCal on QT\n"));
					break;
				}
			}

			if (rcal_valid == 1) {
				rcal_values[i] = READ_RADIO_PLLREGFLD_20698(pi, RCAL_CFG_NORTH, 0,
				                                            rcal_value);

				/* Very coarse sanity check */
				if ((rcal_values[i] < 2) || (60 < rcal_values[i])) {
					PHY_ERROR(("*** ERROR: R Cal value out of range."
					" 6bit Rcal = %d.\n", rcal_values[i]));
					rcal_values[i] = 0x32;
				}
			} else {
				PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
					__FUNCTION__, rcal_valid));
				/* take some sane default value */
				rcal_values[i] = 0x32;
			}
			MOD_RADIO_PLLREG_20698(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_values[i]);
			MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
			MOD_RADIO_PLLREG_20698(pi, BG_REG8, 0, bg_rcal_trim, rcal_values[i]);
			MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);

			/* Power down blocks not needed anymore */
			MOD_RADIO_REG_20698(pi, GPAIO_SEL9, 2, toptestmux_pu, 0);
			MOD_RADIO_REG_20698(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
			MOD_RADIO_PLLREG_20698(pi, BG_REG3, 0, bg_rcal_pu, 0);
			MOD_RADIO_PLLREG_20698(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		}
		/* Use majority of rcal_values if there is any */
		if (rcal_values[0] == rcal_values[1] || rcal_values[0] == rcal_values[2]) {
			data->rcal_value = rcal_values[0];
		} else if (rcal_values[1] == rcal_values[2]) {
			data->rcal_value = rcal_values[1];
		} else {
			data->rcal_value = 0x32;
		}
#ifdef ATE_BUILD
			ate_buffer_regval[0].rcal_value[0] = data->rcal_value;
#endif
		MOD_RADIO_PLLREG_20707(pi, BG_REG1, 0, bg_rtl_rcal_trim, data->rcal_value);
		MOD_RADIO_PLLREG_20707(pi, BG_REG8, 0, bg_rcal_trim, data->rcal_value);
	}
}

static bool
wlc_phy_radio20704_r_cal(phy_info_t *pi, uint8 mode)
{
	/* 20704_procs.tcl r770319: 20704_r_cal */

	/* Format: 20704_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, loop_iter, rcal_value = 0;
	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	bool fail = FALSE;
	/* Nominal R-cal value is 50, ATE holds a margin of +/-6 ticks, take 2 extra */
	const uint8 rcal_nom = 50, rcal_lo = 42, rcal_hi = 58;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);
	/* activate clkrcal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20704(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
		rcal_value = 0xFF;
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20704_procs.tcl */
		rcal_value = rcal_nom;
		MOD_RADIO_PLLREG_20704(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20704(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20704(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20704(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* Run R Cal and use its output */
		MOD_RADIO_REG_20704(pi, GPAIO_SEL9, 1, toptestmux_pu, 1);
		MOD_RADIO_REG_20704(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
		MOD_RADIO_PLLREG_20704(pi, BG_REG3, 0, bg_rcal_pu, 1);
		MOD_RADIO_PLLREG_20704(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20704(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20704(pi, RCAL_CFG_NORTH, 0, rcal_value);

			/* sanity check */
			if ((rcal_value < rcal_lo) || (rcal_value > rcal_hi)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
				rcal_value = rcal_nom;
				fail = TRUE;
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
			/* take some sane default value */
			rcal_value = rcal_nom;
			fail = TRUE;
		}
		MOD_RADIO_PLLREG_20704(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20704(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20704(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20704(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);

#ifdef ATE_BUILD
		ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20704(pi, GPAIO_SEL9, 1, toptestmux_pu, 0);
		MOD_RADIO_REG_20704(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
		MOD_RADIO_PLLREG_20704(pi, BG_REG3, 0, bg_rcal_pu, 0);
		MOD_RADIO_PLLREG_20704(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
	}

	/* deactivate clkrcal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	data->rcal_value = rcal_value;
	return fail;
}

static void
wlc_phy_radio20707_r_cal(phy_info_t *pi, uint8 mode)
{
	/* Format: 20707_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, rcal_value, i, iter = 3;
	uint16 loop_iter;
	uint8 rcal_values[3];
	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);
	/* activate clkrcal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20707(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20707_procs.tcl */
		rcal_value = 0x32;
		MOD_RADIO_PLLREG_20707(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20707(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20707(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20707(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		for (i = 0; i < iter; i++) {
			/* Run R Cal and use its output */
			MOD_RADIO_REG_20707(pi, GPAIO_SEL9, 1, toptestmux_pu, 1);
			MOD_RADIO_REG_20707(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
			MOD_RADIO_PLLREG_20707(pi, BG_REG3, 0, bg_rcal_pu, 1);
			MOD_RADIO_PLLREG_20707(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

			rcal_valid = 0;
			loop_iter = 0;
			while ((rcal_valid == 0) && (loop_iter <= 1000)) {
				/* 1 ms delay wait time */
				OSL_DELAY(1000);
				rcal_valid = READ_RADIO_PLLREGFLD_20707(pi, RCAL_CFG_NORTH, 0,
				                                        rcal_valid);
				loop_iter++;
			}

			if (rcal_valid == 1) {
				rcal_values[i] = READ_RADIO_PLLREGFLD_20707(pi, RCAL_CFG_NORTH, 0,
				                                            rcal_value);

				/* Very coarse sanity check */
				if ((rcal_values[i] < 2) || (60 < rcal_values[i])) {
					PHY_ERROR(("*** ERROR: R Cal value out of range."
					" 6bit Rcal = %d.\n", rcal_values[i]));
					rcal_values[i] = 0x32;
				}
			} else {
				PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
					__FUNCTION__, rcal_valid));
				/* take some sane default value */
				rcal_values[i] = 0x32;
			}
			MOD_RADIO_PLLREG_20707(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_values[i]);
			MOD_RADIO_PLLREG_20707(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
			MOD_RADIO_PLLREG_20707(pi, BG_REG8, 0, bg_rcal_trim, rcal_values[i]);
			MOD_RADIO_PLLREG_20707(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);

			/* Power down blocks not needed anymore */
			MOD_RADIO_REG_20707(pi, GPAIO_SEL9, 1, toptestmux_pu, 0);
			MOD_RADIO_REG_20707(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
			MOD_RADIO_PLLREG_20707(pi, BG_REG3, 0, bg_rcal_pu, 0);
			MOD_RADIO_PLLREG_20707(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		}

		/* Use majority of rcal_values if there is any */
		if (rcal_values[0] == rcal_values[1] || rcal_values[0] == rcal_values[2]) {
			data->rcal_value = rcal_values[0];
		} else if (rcal_values[1] == rcal_values[2]) {
			data->rcal_value = rcal_values[1];
		} else {
			data->rcal_value = 0x32;
		}
#ifdef ATE_BUILD
			ate_buffer_regval[0].rcal_value[0] = data->rcal_value;
#endif
		MOD_RADIO_PLLREG_20707(pi, BG_REG1, 0, bg_rtl_rcal_trim, data->rcal_value);
		MOD_RADIO_PLLREG_20707(pi, BG_REG8, 0, bg_rcal_trim, data->rcal_value);
	}
	/* deactivate clkrcal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

static bool
wlc_phy_radio20708_r_cal(phy_info_t *pi, uint8 mode)
{
	/*
	 * Format: 20708_r_cal [<mode>] [<rcalcode>]
	 * If no arguments given, then mode is assumed to be 1
	 * The rcalcode argument works only with mode 1 and is optional.
	 * If given, then that is the rcal value what will be used in the radio.
	 *
	 * Documentation:
	 * Mode 0 = Don't run cal, use rcal value stored in OTP.
	 *          This is what driver should do but is not good for bringup
	 *          because the OTP is not programmed yet.
	 * Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal value
	 *          given here. Good for initial bringup.
	 * Mode 2 = Run rcal and use the return value in bandgap. Needs the external
	 *          10k resistor to be connected to GPAIO otherwise cal will return bogus value.
	 *
	 * Note: The default argument value for mode is 1 i.e. use static rcal value
	 *          instead of running rcal or reading OTP value. But this is
	 *          only for bringup weeks/months.
	 */
	uint8 rcal_valid, loop_iter, rcal_value;
	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	bool fail = FALSE;
	/* Nominal R-cal value is 50, ATE holds a margin of +/-6 ticks, take 2 extra */
	const uint8 rcal_nom = 50, rcal_lo = 42, rcal_hi = 58;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x1);
	/* activate clkrcal */
	MOD_RADIO_PLLREG_20708(pi, XTAL0, 0, xtal_clkrcal_pu, 0x1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20707_procs.tcl */
		rcal_value = rcal_nom;
		MOD_RADIO_PLLREG_20708(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
		MOD_RADIO_PLLREG_20708(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
	} else if (mode == 2) {
		/* Run R Cal and use its output */
		MOD_RADIO_REG_20708(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, BG_REG3, 0, bg_rcal_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		//rcal_value = 0x64;
		//OSL_DELAY(1000);
		//MOD_RADIO_PLLREG_20708(pi, BG_REG2, 0, bg_ate_rcal_trim_en, 1);
		//MOD_RADIO_PLLREG_20708(pi, BG_REG8, 0, bg_ate_rcal_trim, rcal_value);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20708(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20708(pi, RCAL_CFG_NORTH, 0, rcal_value);

			/* sanity check */
			if ((rcal_value < rcal_lo) || (rcal_value > rcal_hi)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
				rcal_value = rcal_nom;
				fail = TRUE;
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
			/* take some sane default value */
			rcal_value = rcal_nom;
			fail = TRUE;
		}
		MOD_RADIO_PLLREG_20708(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
		MOD_RADIO_PLLREG_20708(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);

		data->rcal_value = rcal_value;
#ifdef ATE_BUILD
		ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif
		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20708(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
		//MOD_RADIO_PLLREG_20708(pi, BG_REG2, 0, bg_ate_rcal_trim_en, 0);
		MOD_RADIO_PLLREG_20708(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		MOD_RADIO_PLLREG_20708(pi, BG_REG3, 0, bg_rcal_pu, 0);
	}
	/* deactivate clkrcal */
	MOD_RADIO_PLLREG_20708(pi, XTAL0, 0, xtal_clkrcal_pu, 0x0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x0);
	return fail;
}

static bool
wlc_phy_radio20709_r_cal(phy_info_t *pi, uint8 mode)
{
	/* 20709_procs.tcl r798817: 20709_r_cal */

	/* Format: 20709_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, loop_iter, rcal_value;
	bool fail = FALSE;
	/* Nominal R-cal value is 50, ATE holds a margin of +/-6 ticks, take 2 extra */
	const uint8 rcal_nom = 50, rcal_lo = 42, rcal_hi = 58;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);
	/* activate clkrcal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20709(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20709_procs.tcl */
		rcal_value = rcal_nom;
		MOD_RADIO_PLLREG_20709(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20709(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20709(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20709(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* Run R Cal and use its output */
		MOD_RADIO_REG_20709(pi, GPAIO_SEL9, 1, toptestmux_pu, 1);
		MOD_RADIO_REG_20709(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
		MOD_RADIO_PLLREG_20709(pi, BG_REG3, 0, bg_rcal_pu, 1);
		MOD_RADIO_PLLREG_20709(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20709(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20709(pi, RCAL_CFG_NORTH, 0, rcal_value);

			/* sanity check */
			if ((rcal_value < rcal_lo) || (rcal_value > rcal_hi)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
				rcal_value = rcal_nom;
				fail = TRUE;
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
			/* take some sane default value */
			rcal_value = rcal_nom;
			fail = TRUE;
		}
		MOD_RADIO_PLLREG_20709(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20709(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20709(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20709(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);

#ifdef ATE_BUILD
		ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20709(pi, GPAIO_SEL9, 1, toptestmux_pu, 0);
		MOD_RADIO_REG_20709(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
		MOD_RADIO_PLLREG_20709(pi, BG_REG3, 0, bg_rcal_pu, 0);
		MOD_RADIO_PLLREG_20709(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
	}

	/* deactivate clkrcal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
	return fail;
}

static bool
wlc_phy_radio20710_r_cal(phy_info_t *pi, uint8 mode)
{
	/* 20710_procs.tcl r770319: 20710_r_cal */

	/* Format: 20710_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */
	uint8 rcal_valid, loop_iter, rcal_value = 0;
	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;
	bool fail = FALSE;
	/* Nominal R-cal value is 50, ATE holds a margin of +/-6 ticks, take 2 extra */
	const uint8 rcal_nom = 50, rcal_lo = 42, rcal_hi = 58;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);
	/* activate clkrcal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1);

	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20710(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0x1);
		rcal_value = 0xFF;
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 0x32 */
		/* value taken from 20710_procs.tcl */
		rcal_value = rcal_nom;
		MOD_RADIO_PLLREG_20710(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20710(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20710(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20710(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* Run R Cal and use its output */
		MOD_RADIO_REG_20710(pi, GPAIO_SEL9, 1, toptestmux_pu, 1);
		MOD_RADIO_REG_20710(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 1);
		MOD_RADIO_PLLREG_20710(pi, BG_REG3, 0, bg_rcal_pu, 1);
		MOD_RADIO_PLLREG_20710(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			/* 1 ms delay wait time */
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20710(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20710(pi, RCAL_CFG_NORTH, 0, rcal_value);

			/* sanity check */
			if ((rcal_value < rcal_lo) || (rcal_value > rcal_hi)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				" 6bit Rcal = %d.\n", rcal_value));
				rcal_value = rcal_nom;
				fail = TRUE;
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
				__FUNCTION__, rcal_valid));
			/* take some sane default value */
			rcal_value = rcal_nom;
			fail = TRUE;
		}
		MOD_RADIO_PLLREG_20710(pi, BG_REG1, 0, bg_rtl_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20710(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20710(pi, BG_REG8, 0, bg_rcal_trim, rcal_value);
		MOD_RADIO_PLLREG_20710(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 1);

#ifdef ATE_BUILD
		ate_buffer_regval[0].rcal_value[0] = rcal_value;
#endif

		/* Power down blocks not needed anymore */
		MOD_RADIO_REG_20710(pi, GPAIO_SEL9, 1, toptestmux_pu, 0);
		MOD_RADIO_REG_20710(pi, GPAIO_SEL8, 1, gpaio_rcal_pu, 0);
		MOD_RADIO_PLLREG_20710(pi, BG_REG3, 0, bg_rcal_pu, 0);
		MOD_RADIO_PLLREG_20710(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
	}

	/* deactivate clkrcal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	data->rcal_value = rcal_value;
	return fail;
}

static void
wlc_phy_radio20698_txdac_bw_setup(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059:  20698_txdac_bw_setup */

	uint8 core;

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x0);
		MOD_RADIO_REG_20698(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
	}
}

static void
wlc_phy_radio20704_txdac_bw_setup(phy_info_t *pi)
{
	/* 20704_procs.tcl r785868:  20704_txdac_bw_setup */

	uint8 core;

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x0);
		MOD_RADIO_REG_20704(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
	}
}

static void
wlc_phy_radio20707_txdac_bw_setup(phy_info_t *pi)
{

	uint8 core;

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x0);
		MOD_RADIO_REG_20707(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
	}
}

static void
wlc_phy_radio20708_txdac_bw_setup(phy_info_t *pi)
{

	uint8 core, nf;
	uint8 cfb_default_values[4] = {30, 33, 80, 109};
	uint8 rfb_default_values[4] = {1, 0, 0, 0};
	uint8 IQDAC_lowpwr_en[4] = {1, 0, 0, 0};
	uint8 IQDAC_rdeg_inc[4] = {1, 0, 0, 0};
	uint8 bias_reg0_bits_4to0[4] = {22, 0, 0, 0};
	uint8 bias_reg0_bits_7to5[4] = {4, 0, 0, 0};
	uint32 cfb_tmp, cfb, bias_reg0;
	uint8 rfb, lowpwr_en, rdeg_inc, bits_4to0, bits_7to5;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	cfb_tmp = (CHSPEC_IS20(pi->radio_chanspec)) ? cfb_default_values[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? cfb_default_values[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? cfb_default_values[2] :
			cfb_default_values[3];
	nf = math_fp_div_64(cfb_tmp, dacbuf_cmult, 0, 12, &cfb);
	cfb = math_fp_round_32(cfb, nf);
	cfb = (cfb > 127) ? 127 : (uint8) cfb;
	rfb = (CHSPEC_IS20(pi->radio_chanspec)) ? rfb_default_values[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? rfb_default_values[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? rfb_default_values[2] :
			rfb_default_values[3];
	lowpwr_en = (CHSPEC_IS20(pi->radio_chanspec)) ? IQDAC_lowpwr_en[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? IQDAC_lowpwr_en[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? IQDAC_lowpwr_en[2] :
			IQDAC_lowpwr_en[3];
	rdeg_inc = (CHSPEC_IS20(pi->radio_chanspec)) ? IQDAC_rdeg_inc[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? IQDAC_rdeg_inc[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? IQDAC_rdeg_inc[2] :
			IQDAC_rdeg_inc[3];
	bits_4to0 = (CHSPEC_IS20(pi->radio_chanspec)) ? bias_reg0_bits_4to0[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? bias_reg0_bits_4to0[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? bias_reg0_bits_4to0[2] :
			bias_reg0_bits_4to0[3];
	bits_7to5 = (CHSPEC_IS20(pi->radio_chanspec)) ? bias_reg0_bits_7to5[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? bias_reg0_bits_7to5[1] :
			(CHSPEC_IS80(pi->radio_chanspec)) ? bias_reg0_bits_7to5[2] :
			bias_reg0_bits_7to5[3];
	bias_reg0 = ((bits_7to5 << 5) | (bits_4to0 & 0x1f));

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20708(pi, TXDAC_REG1, core, iqdac_buf_Cfb, cfb);
		MOD_RADIO_REG_20708(pi, TXDAC_REG1, core, iqdac_Rfb, rfb);
		MOD_RADIO_REG_20708(pi, TXDAC_REG1, core, iqdac_lowpwr_en, lowpwr_en);
		MOD_RADIO_REG_20708(pi, TXDAC_REG3, core, iqdac_rdeg_inc, rdeg_inc);
		MOD_RADIO_REG_20708(pi, TXDAC_REG8, core, iqdac_bias_reg0, bias_reg0);
	}
}

static void
wlc_phy_radio20708_txadc_bw_setup(phy_info_t *pi)
{
	uint8  core, bwIdx;
	uint16 lat_fix_dly_values_12to11[4] = {0x2, 0x2, 0x1, 0x1};
	uint16 clk_tck_dly_values_5to4[4] = {0x0, 0x0, 0x1, 0x1};
	uint16 ref_bias_values[4] = {0x1, 0x1, 0x0, 0x0};
	uint16 temp, temp1, temp2, temp3;

	bwIdx = (CHSPEC_IS20(pi->radio_chanspec)) ? 0 :
		(CHSPEC_IS40(pi->radio_chanspec)) ? 1 :
		(CHSPEC_IS80(pi->radio_chanspec)) ? 2 : 3;
	temp1 = lat_fix_dly_values_12to11[bwIdx];
	temp2 = clk_tck_dly_values_5to4[bwIdx];
	temp3 = ref_bias_values[bwIdx];

	FOREACH_CORE(pi, core) {
		temp = READ_RADIO_REGFLD_20708(pi, RF, RXADC_REG2, core, rxadc_core_ctrl_LSB);
		temp = (temp & 0xE7FF) | (temp1 << 11);
		MOD_RADIO_REG_20708(pi, RXADC_REG2, core, rxadc_core_ctrl_LSB, temp);

		temp = READ_RADIO_REGFLD_20708(pi, RF, RXADC_REG2, core, rxadc_core_ctrl_LSB);
		temp = (temp & 0xFFCF) | (temp2 << 4);
		MOD_RADIO_REG_20708(pi, RXADC_CFG12, core, rxadc_clk, temp);

		MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR3, core, ovr_rxadc_power_mode, 1);
		MOD_RADIO_REG_20708(pi, RXADC_CFG5, core, rxadc_power_mode, temp3);
	}
}

static void
wlc_phy_radio20709_txdac_bw_setup(phy_info_t *pi)
{
	/* 20709_procs.tcl r849991:  20709_txdac_bw_setup */

	uint8 core, cfb;
	uint8 cfb_default_code[4] = {127, 64, 0, 0};
	uint32 cfb_tmp;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	cfb_tmp = (CHSPEC_IS20(pi->radio_chanspec)) ? cfb_default_code[0] :
			(CHSPEC_IS40(pi->radio_chanspec)) ? cfb_default_code[1] :
			cfb_default_code[2];
	cfb_tmp *= dacbuf_cmult;
	cfb_tmp >>= 12;
	cfb = (cfb_tmp > 127 || !PHY_IPA(pi)) ? 127 : (uint8) cfb_tmp;

	FOREACH_CORE(pi, core) {
		/* default is 0 but set to 1 during adccal */
		MOD_RADIO_REG_20709(pi, TXDAC_REG1, core, iqdac_buf_op_cur, 0x0);

		MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x0);
		MOD_RADIO_REG_20709(pi, TXDAC_REG1, core, iqdac_buf_Cfb, cfb);
	}
}

static void
wlc_phy_radio20710_txdac_bw_setup(phy_info_t *pi)
{
	/* 20710_procs.tcl r905339:  20710_txdac_bw_setup */

	uint8 core, cfb;
	uint32 cfb_tmp;
	uint32 dacbuf_cmult = pi->u.pi_acphy->radioi->rccal_dacbuf_cmult;

	cfb_tmp = 28;
	cfb_tmp *= dacbuf_cmult;
	cfb_tmp >>= 12;
	cfb = (cfb_tmp > 127) ? 127 : (uint8) cfb_tmp;

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x0);
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			MOD_RADIO_REG_20710(pi, TXDAC_REG1, core, iqdac_buf_Cfb, cfb);
		} else {
			MOD_RADIO_REG_20710(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 127);
		}
	}
}

static void wlc_phy_radio20698_minipmu_cal(phy_info_t *pi)
{
	/* 20698_procs.tcl r731818: 20698_minipmu_cal */
	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(25);

	/* start cal */
	MOD_RADIO_PLLREG_20698(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done	 */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20698(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
		if (ISSIM_ENAB(pi->sh->sih)) {
			/* Bypass Mini PMU Cal on QT to speed up dongle bring up */
			PHY_ERROR(("*** SIM: Bypass Mini PMU Cal on QT\n"));
			calsuccesful = FALSE;
			break;
		}
	}

	/* cleanup */
	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
	RADIO_REG_LIST_EXECUTE(pi, 0);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20698(pi, PMU_CFG3, core, vref_select, 0x1);
		MOD_RADIO_REG_20698(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20698(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
				__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20698(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}
}

static void wlc_phy_radio20704_minipmu_cal(phy_info_t *pi)
{
	/* 20704_procs.tcl r821426: 20704_minipmu_cal */

	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	MOD_RADIO_PLLREG_20704(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 0x1);
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_OVR1, 0, ovr_bg_pulse, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(30);
	MOD_RADIO_PLLREG_20704(pi, BG_REG2, 0, bg_pulse, 0x1);
	MOD_RADIO_PLLREG_20704(pi, BG_REG2, 0, bg_pulse, 0x0);
	OSL_DELAY(30);
	/* start cal */
	MOD_RADIO_PLLREG_20704(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done	 */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20704(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20704(pi, PMU_CFG3, core, vref_select, 0x1);
		MOD_RADIO_REG_20704(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20704(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
				__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20704(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

static void wlc_phy_radio20707_minipmu_cal(phy_info_t *pi)
{
	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_OVR1, 0, ovr_bg_pulse, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(30);
	MOD_RADIO_PLLREG_20707(pi, BG_REG2, 0, bg_pulse, 0x1);
	MOD_RADIO_PLLREG_20707(pi, BG_REG2, 0, bg_pulse, 0x0);
	OSL_DELAY(30);
	/* start cal */
	MOD_RADIO_PLLREG_20707(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done     */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20707(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20707(pi, PMU_CFG3, core, vref_select, 0x1);
		MOD_RADIO_REG_20707(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20707(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
			__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20707(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG9,  0, bg_wlpmu_cal_mancodes, 0x20)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

static void wlc_phy_radio20708_minipmu_cal(phy_info_t *pi)
{
	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_OVR1, 0, ovr_bg_pulse, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);

	OSL_DELAY(30);
	MOD_RADIO_PLLREG_20708(pi, BG_REG2, 0, bg_pulse, 0x1);
	MOD_RADIO_PLLREG_20708(pi, BG_REG2, 0, bg_pulse, 0x0);
	OSL_DELAY(30);
	/* start cal */
	MOD_RADIO_PLLREG_20708(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done     */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20708(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20708(pi, PMU_CFG3, core, vref_select, 0x1);
		//MOD_RADIO_REG_20708(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20708(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
			__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20708(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG9,  0, bg_wlpmu_cal_mancodes, 0x20)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20708(pi, XTAL9, 0, xtal_clkrccal_pu, 0x0);

	/* For A1 onwards, to reduce transient settlings */
	if (RADIOMAJORREV(pi) >= 1) {
		MOD_RADIO_PLLREG_20708(pi, BG_OVR1, 0, ovr_bg_pulse, 1);
		MOD_RADIO_PLLREG_20708(pi, BG_REG2, 0, bg_pulse, 1);
	}
}

static void wlc_phy_radio20709_minipmu_cal(phy_info_t *pi)
{
	/* 20709_procs.tcl r821431: 20709_minipmu_cal */

	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	MOD_RADIO_PLLREG_20709(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 0x1);
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_OVR1, 0, ovr_bg_pulse, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(30);
	MOD_RADIO_PLLREG_20709(pi, BG_REG2, 0, bg_pulse, 0x1);
	OSL_DELAY(30);
	/* start cal */
	MOD_RADIO_PLLREG_20709(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done	 */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20709(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20709(pi, PMU_CFG3, core, vref_select, 0x1);
		MOD_RADIO_REG_20709(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20709(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
				__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20709(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

static void wlc_phy_radio20710_minipmu_cal(phy_info_t *pi)
{
	/* 20710_procs.tcl r821426: 20710_minipmu_cal */

	uint8 waitcounter;
	uint8 core;
	bool calsuccesful;

	MOD_RADIO_PLLREG_20710(pi, FRONTLDO_REG3, 0, i_fldo_vout_adj_1p0, 0x1);
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_en_i, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_OVR1, 0, ovr_bg_pulse, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(30);
	MOD_RADIO_PLLREG_20710(pi, BG_REG2, 0, bg_pulse, 0x1);
	MOD_RADIO_PLLREG_20710(pi, BG_REG2, 0, bg_pulse, 0x0);
	OSL_DELAY(30);
	/* start cal */
	MOD_RADIO_PLLREG_20710(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x1);
	OSL_DELAY(25);

	/* Wait for cal_done	 */
	calsuccesful = TRUE;
	waitcounter = 0;
	while (READ_RADIO_PLLREGFLD_20710(pi, BG_REG11, 0, bg_wlpmu_cal_done) == 0) {
		OSL_DELAY(100);
		waitcounter++;
		if (waitcounter > 100) {
			/* This means the cal_done bit is not 1 even after waiting a while */
			/* Exit gracefully */
			PHY_INFORM(("wl%d: %s: Warning : Mini PMU Cal Failed\n",
				pi->sh->unit, __FUNCTION__));
			calsuccesful = FALSE;
			break;
		}
	}

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20710(pi, PMU_CFG3, core, vref_select, 0x1);
		MOD_RADIO_REG_20710(pi, RX2G_REG4, core, rx_ldo_refsel, 0x1);
	}

	if (calsuccesful) {
		uint8 tempcalcode;

		tempcalcode = READ_RADIO_PLLREGFLD_20710(pi, BG_REG11, 0, bg_wlpmu_calcode);
		PHY_INFORM(("wl%d: %s MiniPMU done. Cal Code = %d\n", pi->sh->unit,
				__FUNCTION__, tempcalcode));
		MOD_RADIO_PLLREG_20710(pi, BG_REG9, 0, bg_wlpmu_cal_mancodes, tempcalcode);
	}

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_bypcal, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_startcal, 0x0)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
}

/* 20693 radio related functions */
int
wlc_phy_chan2freq_20693(phy_info_t *pi, uint8 channel,
	const chan_info_radio20693_pll_t **chan_info_pll,
	const chan_info_radio20693_rffe_t **chan_info_rffe,
	const chan_info_radio20693_pll_wave2_t **chan_info_pll_wave2)
{
	uint i;
	uint16 freq;
	const chan_info_radio20693_pll_t *chan_info_tbl_pll = NULL;
	const chan_info_radio20693_rffe_t *chan_info_tbl_rffe = NULL;
	const chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part1 = NULL;
	const chan_info_radio20693_pll_wave2_t *chan_info_tbl_pll_wave2_part2 = NULL;
	uint32 tbl_len, tbl_len1 = 0;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* Choose the right table to use */
	switch (RADIO20693REV(pi->pubpi->radiorev)) {
	case 3:
	case 4:
	case 5:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 6:
		chan_info_tbl_pll = chan_tuning_20693_rev6_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev6_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev6_pll);
		break;
	case 7:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 8:
		chan_info_tbl_pll = chan_tuning_20693_rev6_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev6_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev6_pll);
		break;
	case 10:
		chan_info_tbl_pll = chan_tuning_20693_rev10_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev10_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev10_pll);
		break;
	case 11:
	case 12:
	case 13:
		chan_info_tbl_pll = chan_tuning_20693_rev13_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev13_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev13_pll);
		break;
	case 14:
	case 19:
		chan_info_tbl_pll = chan_tuning_20693_rev14_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev14_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev14_pll);
		break;
	case 15:
		if (PHY_XTAL_IS37M4(pi)) {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_37p4MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_37p4MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_37p4MHz);
		} else {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		}
		break;
	case 16:
	case 17:
	case 20:
		chan_info_tbl_pll = chan_tuning_20693_rev5_pll;
		chan_info_tbl_rffe = chan_tuning_20693_rev5_rffe;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev5_pll);
		break;
	case 18:
	case 21:
		if (PHY_XTAL_IS37M4(pi)) {
			chan_info_tbl_pll = chan_tuning_20693_rev18_pll;
			chan_info_tbl_rffe = chan_tuning_20693_rev18_rffe;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev18_pll);
		} else {
			chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
			chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
			tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		}
		break;
	case 23:
		/* Preferred values for 53573/53574/47189-B0 */
		chan_info_tbl_pll = chan_tuning_20693_rev15_pll_40MHz;
		chan_info_tbl_rffe = chan_tuning_20693_rev15_rffe_40MHz;
		tbl_len = ARRAYSIZE(chan_tuning_20693_rev15_pll_40MHz);
		break;
	case 32:
		chan_info_tbl_pll  = NULL;
		chan_info_tbl_rffe = NULL;
		chan_info_tbl_pll_wave2_part1 = chan_tuning_20693_rev32_pll_part1;
		chan_info_tbl_pll_wave2_part2 = chan_tuning_20693_rev32_pll_part2;
		tbl_len1 = ARRAYSIZE(chan_tuning_20693_rev32_pll_part1);
		tbl_len = tbl_len1 + ARRAYSIZE(chan_tuning_20693_rev32_pll_part2);
		break;
	case 33:
		chan_info_tbl_pll  = NULL;
		chan_info_tbl_rffe = NULL;
		chan_info_tbl_pll_wave2_part1 = chan_tuning_20693_rev33_pll_part1;
		chan_info_tbl_pll_wave2_part2 = chan_tuning_20693_rev33_pll_part2;
		tbl_len1 = ARRAYSIZE(chan_tuning_20693_rev32_pll_part1);
		tbl_len = tbl_len1 + ARRAYSIZE(chan_tuning_20693_rev32_pll_part2);
		break;
	case 9:
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20693REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return -1;
	}

	if (RADIOMAJORREV(pi) == 3 &&
	    chan_info_tbl_pll_wave2_part1 && chan_info_tbl_pll_wave2_part2) {
		for (i = 0; i < tbl_len; i++) {
			if (i >= tbl_len1) {
				if (chan_info_tbl_pll_wave2_part2[i-tbl_len1].chan == channel) {
					*chan_info_pll_wave2 =
							&chan_info_tbl_pll_wave2_part2[i-tbl_len1];
					freq = chan_info_tbl_pll_wave2_part2[i-tbl_len1].freq;
					break;
				}
			} else {
				if (chan_info_tbl_pll_wave2_part1[i].chan == channel) {
					*chan_info_pll_wave2 = &chan_info_tbl_pll_wave2_part1[i];
					freq = chan_info_tbl_pll_wave2_part1[i].freq;
					break;
				}
			}
		}

		if (i >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(i < tbl_len);
			}
			return -1;
		}
	} else {
		for (i = 0; i < tbl_len && chan_info_tbl_pll[i].chan != channel; i++);

		if (i >= tbl_len) {
			PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
				pi->sh->unit, __FUNCTION__, channel));
			if (!ISSIM_ENAB(pi->sh->sih)) {
				/* Do not assert on QT since we leave the tables empty on purpose */
				ASSERT(i < tbl_len);
			}
			return -1;
		}

		*chan_info_pll = &chan_info_tbl_pll[i];
		*chan_info_rffe = &chan_info_tbl_rffe[i];
		freq = chan_info_tbl_pll[i].freq;
	}

	return freq;
}

void
phy_ac_radio_20693_chanspec_setup(phy_info_t *pi, uint8 ch, uint8 toggle_logen_reset,
	uint8 pllcore, uint8 mode)
{
	const chan_info_radio20693_pll_t *chan_info_pll;
	const chan_info_radio20693_rffe_t *chan_info_rffe;
	const chan_info_radio20693_pll_wave2_t *chan_info_pll_wave2;
	int fc[NUM_CHANS_IN_CHAN_BONDING];
	uint8 core, start_pllcore = 0, end_pllcore = 0;
	uint8 chans[NUM_CHANS_IN_CHAN_BONDING] = {0, 0};

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (pllcore >= 2)
		pllcore = 0;

	/* mode = 0, setting per pllcore input
	 * mode = 1, setting both pllcores=0/1 at once
	 */
	if (mode == 0) {
		start_pllcore = pllcore;
		end_pllcore = pllcore;
	} else if (mode == 1) {
		start_pllcore = 0;
		end_pllcore = 1;
	} else {
		ASSERT(0);
	}

	if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
		ASSERT(start_pllcore != end_pllcore);
		wf_chspec_get_80p80_channels(pi->radio_chanspec, chans);
	} else {
		chans[pllcore] = ch;
	}

	for (core = start_pllcore; core <= end_pllcore; core++) {
		fc[core] = wlc_phy_chan2freq_20693(pi, chans[core], &chan_info_pll,
				&chan_info_rffe, &chan_info_pll_wave2);

		if (fc[core] < 0)
			return;

		/* logen_reset needs to be toggled whenever bandsel bit if changed */
		/* On a bw change, phy_reset is issued which causes currentBand getting */
		/* reset to 0 */
		/* So, issue this on both band & bw change */
		if (toggle_logen_reset == 1) {
			wlc_phy_logen_reset(pi, core);
		}

		if (RADIOMAJORREV(pi) != 3) {
			/* pll tuning */
			wlc_phy_radio20693_pll_tune(pi, chan_info_pll, core);

			/* rffe tuning */
			wlc_phy_radio20693_rffe_tune(pi, chan_info_rffe, core);
		} else {
			/* pll tuning */
			wlc_phy_radio20693_pll_tune_wave2(pi, chan_info_pll_wave2, core);
			/* XXX: Temp fix for 4365 till radio tuning load proc gets
			 * fixed. junz
			 */
			wlc_phy_radio20693_sel_logen_mode(pi);
		}
	}

	if ((RADIOMAJORREV(pi) == 3) &&
			!((pllcore == 1) && phy_get_phymode(pi) == PHYMODE_3x3_1x1)) {
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1, ldo_1p2_xtalldo1p2_ctl, 0xe);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO1, ldo_1p2_lowquiescenten_PFDMMD, 1);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO1, ldo_1p2_ldo_PFDMMD_vout_sel, 0xc);
		MOD_RADIO_ALLPLLREG_20693(pi, PLL_LVLDO2, ldo_1p2_ldo_VCOBUF_vout_sel, 0xc);
		MOD_RADIO_ALLREG_20693(pi, TX_DAC_CFG1, DAC_scram_off, 1);

		if (pi->sromi->sr13_1p5v_cbuck) {
			MOD_RADIO_ALLREG_20693(pi, PMU_CFG1, wlpmu_vrefadj_cbuck, 0x5);
			PHY_TRACE(("wl%d: %s: setting wlpmu_vrefadj_cbuck=0x5",
				pi->sh->unit, __FUNCTION__));
		}

		if (CHSPEC_IS20(pi->radio_chanspec) && !PHY_AS_80P80(pi, pi->radio_chanspec)) {
			if (fc[pllcore] <= 2472) {
				if (fc[pllcore] == 2427) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
					ldo_1p2_xtalldo1p2_ctl, 0xe);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
				} else {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
					ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
				}
				if (pi->sromi->sr13_cck_spur_en == 1) {
					/* 2G cck spur reduction setting for 4366, core0 */
					MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG4, pllcore,
						wl_xtal_wlanrf_ctrl, 0x3);
					PHY_TRACE(("wl%d: %s, fc: %d, 2g cck spur reduction\n",
						pi->sh->unit, __FUNCTION__, fc[pllcore]));
				}
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG4,
					wl_xtal_wlanrf_ctrl, 0x3);
			} else if (fc[pllcore] >= 5180 && fc[pllcore] <= 5320) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x8);
			} else if (fc[pllcore] >= 5745 && fc[pllcore] <= 5825) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x8);
			} else {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xe);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
			}
		} else if (CHSPEC_IS40(pi->radio_chanspec) || CHSPEC_IS80(pi->radio_chanspec)) {
			if (fc[pllcore] >= 5745 && fc[pllcore] <= 5825) {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				                          ldo_1p2_xtalldo1p2_ctl, 0xa);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x8);
			} else {
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1,
				                          ldo_1p2_xtalldo1p2_ctl, 0xe);
				MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
				MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
			}
		} else {
			MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTALLDO1, ldo_1p2_xtalldo1p2_ctl, 0xe);
			MOD_RADIO_ALLPLLREG_20693(pi, PLL_XTAL_OVR1, ovr_xtal_core, 0x1);
			MOD_RADIO_ALLPLLREG_20693(pi, WL_XTAL_CFG2, wl_xtal_core, 0x24);
		}
	}

	if (RADIOMAJORREV(pi) == 3) {
		uint16 phymode = phy_get_phymode(pi);
		if (phymode == PHYMODE_BGDFS) {
			if (pllcore == 1) {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 4, 1, FALSE);
			}
		} else {
			if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 2, 1, TRUE);
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			} else {
				wlc_phy_radio_tiny_vcocal_wave2(pi, 0, 1, 1, TRUE);
			}
		}
	} else if (!((phy_get_phymode(pi) == PHYMODE_MIMO) && (pllcore == 1))) {
	/* Do a VCO cal after writing the tuning table regs */
	/* in mimo mode, vco cal needs to be done only for core 0 */
		wlc_phy_radio_tiny_vcocal(pi);
	}
}

static void
wlc_phy_radio20693_afeclkpath_setup(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint8 direct_ctrl_en)
{
	uint8 pupd;
	uint16 mac_mode;
	pupd = (direct_ctrl_en == 1) ? 0 : 1;
	mac_mode = phy_get_phymode(pi);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (direct_ctrl_en == 0) {
		MOD_RADIO_REG_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_12g_buf_pu, pupd);
		MOD_RADIO_REG_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_buf_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_5g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_5g_mimosrc_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_5g_mimodes_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_2g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_2g_mimosrc_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_OVR2, core, ovr_logencore_2g_mimodes_pu, pupd);
		MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_div2_pu, pupd);
		MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_6g_mimo_pu, pupd);

		/* set logen mimo src des pu */
		wlc_phy_radio20693_set_logen_mimosrcdes_pu(pi, core, adc_mode, mac_mode);
	}

	/* power up or down logen core based on band */
	wlc_phy_radio20693_set_logencore_pu(pi, adc_mode, core);
	/* Inside pll vco cell bufs 12G logen outputs at VCO freq */
	wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(pi, adc_mode, core);
	/* powerup afe clk div 12g sel */
	wlc_phy_radio20693_afeclkdiv_12g_sel(pi, adc_mode, core);
	/* power up afe clkdiv mimo blocks */
	wlc_phy_radio20693_afe_clk_div_mimoblk_pu(pi, core, adc_mode);
}

static void
wlc_phy_radio20693_set_logen_mimosrcdes_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode, uint16 mac_mode)
{
	uint8 pupd;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	pupd = (mac_mode == PHYMODE_MIMO) ? 1 : 0;

	/* band a */
	if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
		if (core == 0) {
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, pupd);
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, 0);
		} else {
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, 0);
			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, pupd);
		}

		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_2g_mimosrc_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_2g_mimodes_pu, 0);
	} else {
		/* band g */
		if (adc_mode == RADIO_20693_SLOW_ADC)  {
			if (core == 0) {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, pupd);
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimodes_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, 0);
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimodes_pu, pupd);
			}
		} else {
			if (core == 0) {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, pupd);
			} else {
				MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
					logencore_2g_mimosrc_pu, 0);
			}

			MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core,
				logencore_2g_mimodes_pu, 0);
		}

		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimosrc_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG3, core, logencore_5g_mimodes_pu, 0);
	}
}

static void
wlc_phy_radio20693_adc_powerupdown(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 pupdbit, uint8 core)
{
	uint8 en_ovrride;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (pupdbit == 1) {
	    /* use direct control */
		en_ovrride = 0;
	} else {
	    /* to force powerdown */
		en_ovrride = 1;
	}
	FOREACH_CORE(pi, core) {
		switch (adc_mode) {
		case RADIO_20693_FAST_ADC:
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_clk_fast_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG15, core, adc_clk_fast_pu, pupdbit);
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_fast_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG1, core, adc_fast_pu, pupdbit);
			break;
		case RADIO_20693_SLOW_ADC:
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_clk_slow_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG15, core, adc_clk_slow_pu, pupdbit);
	        MOD_RADIO_REG_20693(pi, ADC_OVR1, core, ovr_adc_slow_pu, en_ovrride);
	        MOD_RADIO_REG_20693(pi, ADC_CFG1, core, adc_slow_pu, pupdbit);
			break;
	    default:
	        PHY_ERROR(("wl%d: %s: Wrong ADC mode \n",
	           pi->sh->unit, __FUNCTION__));
			ASSERT(0);
	        break;
	    }
	    MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_rst_clk, 0x0);
	}
}

static void
wlc_phy_radio20693_set_logencore_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 pupd = 0;
	bool mimo_core_idx_1;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	/* Based on the adcmode and band set the logencore_pu for 2G and 5G */
	if ((mimo_core_idx_1 == 1) &&
		(CHSPEC_IS2G(pi->radio_chanspec)) && (adc_mode == RADIO_20693_FAST_ADC)) {
		pupd = 1;
	} else if (mimo_core_idx_1 == 1) {
		pupd = 0;
	} else {
		pupd = 1;
	}

	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_5g_pu, 1);
	MOD_RADIO_REG_20693(pi, LOGEN_OVR1, core, ovr_logencore_2g_pu, 1);

	if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_5g_pu, pupd);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_2g_pu, 0);
	} else {
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_5g_pu, 0);
		MOD_RADIO_REG_20693(pi, LOGEN_CFG2, core, logencore_2g_pu, pupd);
	}
}

static void
wlc_phy_radio20693_set_rfpll_vco_12g_buf_pu(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 pupd = 0;
	bool mimo_core_idx_1;
	bool is_per_core_phy_bw80;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));
	/* Based on the adcmode and band set the logencore_pu for 2G and 5G */
	if (mimo_core_idx_1 == 1) {
		pupd = 0;
	} else {
		pupd = 1;
	}
	if ((adc_mode == RADIO_20693_FAST_ADC) || (is_per_core_phy_bw80 == 1) ||
		(CHSPEC_ISPHY5G6G(pi->radio_chanspec))) {
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_12g_buf_pu, pupd);
	} else {
		MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_12g_buf_pu, 0);
	}
	/* Feeds MMD and external clocks */
	MOD_RADIO_REG_20693(pi, PLL_CFG1, core, rfpll_vco_buf_pu, pupd);
}

static void
wlc_phy_radio20693_afeclkdiv_12g_sel(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	bool is_per_core_phy_bw80;
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 afeclkdiv;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);
	if ((adc_mode == RADIO_20693_FAST_ADC) || (is_per_core_phy_bw80 == 1)) {
		/* 80Mhz  : afe_div = 3 */
		afeclkdiv = 0x4;
	} else {
		if (CHSPEC_IS20(pi->radio_chanspec)) {
			/* 20Mhz  : afe_div = 8 */
			afeclkdiv = 0x3;
		} else {
			/* 40Mhz  : afe_div = 4 */
			afeclkdiv = 0x2;
		}
	}
	if ((row >= 0) && (adc_mode != RADIO_20693_FAST_ADC)) {
		afeclkdiv = altclkpln[row].afeclkdiv;
	}
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_12g_sel, afeclkdiv);
}

static void
wlc_phy_radio20693_afe_clk_div_mimoblk_pu(phy_info_t *pi, uint8 core,
	radio_20693_adc_modes_t adc_mode) {

	uint8 pupd = 0;
	bool mimo_core_idx_1;
	bool is_per_core_phy_bw80;
	mimo_core_idx_1 = ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core == 1)) ? 1 : 0;
	is_per_core_phy_bw80 = (CHSPEC_IS80(pi->radio_chanspec) ||
		CHSPEC_IS8080(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec));

	if (mimo_core_idx_1 == 1) {
		pupd = 1;
	} else {
		pupd = 0;
	}
	/* Enable the radio Ovrs */
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_6g_mimo_pu, 1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_div2_pu, 1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_mimo_pu, 1);
	if (CHSPEC_IS2G(pi->radio_chanspec))  {
		if (adc_mode == RADIO_20693_SLOW_ADC) {
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_div2_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_6g_mimo_pu, pupd);
		} else {
			if (wlapi_txbf_enab(pi->sh->physhim)) {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, pupd);
			}
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_div2_pu, pupd);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_6g_mimo_pu, 0);
		}
	} else if (CHSPEC_ISPHY5G6G(pi->radio_chanspec))  {
		if ((adc_mode == RADIO_20693_SLOW_ADC) &&
			((is_per_core_phy_bw80 == 0))) {
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
				core, afe_clk_div_12g_mimo_pu, 0);
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
				afe_clk_div_12g_mimo_div2_pu, pupd);
		} else {
			if (wlapi_txbf_enab(pi->sh->physhim)) {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1,
					core, afe_clk_div_12g_mimo_pu, 0);
			} else {
				MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
					afe_clk_div_12g_mimo_pu, pupd);
			}
			MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core,
				afe_clk_div_12g_mimo_div2_pu, 0);
			MOD_RADIO_REG_20693(pi, SPARE_CFG8, core,
				afe_BF_se_enb0, 1);
			MOD_RADIO_REG_20693(pi, SPARE_CFG8, core,
				afe_BF_se_enb1, 1);
		}
		MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_6g_mimo_pu, 0);
	}
}

static void wlc_phy_radio20693_pll_tune(phy_info_t *pi, const void *chan_info_pll,
	uint8 core)
{
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct;
	const chan_info_radio20693_pll_t *ci20693_pll = chan_info_pll;

	uint16 pll_tune_reg_offsets_20693[] = {
		RADIO_REG_20693(pi, PLL_VCOCAL1, core),
		RADIO_REG_20693(pi, PLL_VCOCAL11, core),
		RADIO_REG_20693(pi, PLL_VCOCAL12, core),
		RADIO_REG_20693(pi, PLL_FRCT2, core),
		RADIO_REG_20693(pi, PLL_FRCT3, core),
		RADIO_REG_20693(pi, PLL_HVLDO1, core),
		RADIO_REG_20693(pi, PLL_LF4, core),
		RADIO_REG_20693(pi, PLL_LF5, core),
		RADIO_REG_20693(pi, PLL_LF7, core),
		RADIO_REG_20693(pi, PLL_LF2, core),
		RADIO_REG_20693(pi, PLL_LF3, core),
		RADIO_REG_20693(pi, SPARE_CFG1, core),
		RADIO_REG_20693(pi, SPARE_CFG14, core),
		RADIO_REG_20693(pi, SPARE_CFG13, core),
		RADIO_REG_20693(pi, TXMIX2G_CFG5, core),
		RADIO_REG_20693(pi, TXMIX5G_CFG6, core),
		RADIO_REG_20693(pi, PA5G_CFG4, core),
	};

	PHY_TRACE(("wl%d: %s\n core: %d Channel: %d\n", pi->sh->unit, __FUNCTION__,
		core, ci20693_pll->chan));

	off_ptr = &pll_tune_reg_offsets_20693[0];
	val_ptr = &ci20693_pll->pll_vcocal1;

	for (ct = 0; ct < ARRAYSIZE(pll_tune_reg_offsets_20693); ct++)
		phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
}

static void wlc_phy_radio20693_pll_tune_wave2(phy_info_t *pi, const void *chan_info_pll,
	uint8 core)
{
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct, rdcore;
	const chan_info_radio20693_pll_wave2_t *ci20693_pll = chan_info_pll;

	uint16 pll_tune_reg_offsets_20693[] = {
		RADIO_PLLREG_20693(pi, WL_XTAL_CFG3, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL18, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL3, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL4, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL7, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL8, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL20, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL1, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL12, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL13, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL10, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL11, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL19, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL6, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL9, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL17, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL15, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL2, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL24, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL26, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL25, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL21, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL22, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL23, core),
		RADIO_PLLREG_20693(pi, PLL_VCO7, core),
		RADIO_PLLREG_20693(pi, PLL_VCO2, core),
		RADIO_PLLREG_20693(pi, PLL_FRCT2, core),
		RADIO_PLLREG_20693(pi, PLL_FRCT3, core),
		RADIO_PLLREG_20693(pi, PLL_VCO6, core),
		RADIO_PLLREG_20693(pi, PLL_VCO5, core),
		RADIO_PLLREG_20693(pi, PLL_VCO4, core),
		RADIO_PLLREG_20693(pi, PLL_LF4, core),
		RADIO_PLLREG_20693(pi, PLL_LF5, core),
		RADIO_PLLREG_20693(pi, PLL_LF7, core),
		RADIO_PLLREG_20693(pi, PLL_LF2, core),
		RADIO_PLLREG_20693(pi, PLL_LF3, core),
		RADIO_PLLREG_20693(pi, PLL_CP4, core),
		RADIO_PLLREG_20693(pi, PLL_VCOCAL5, core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN0_DRV, core),
		RADIO_PLLREG_20693(pi, LO2G_VCO_DRV_CFG2, core),
		RADIO_PLLREG_20693(pi, LO2G_LOGEN1_DRV, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE0_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE1_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE2_CFG1, core),
		RADIO_PLLREG_20693(pi, LO5G_CORE3_CFG1, core),
	};

	uint8 ctbase = ARRAYSIZE(pll_tune_reg_offsets_20693);

	PHY_TRACE(("wl%d: %s\n pll_core: %d Channel: %d\n", pi->sh->unit, __FUNCTION__,
	        core, ci20693_pll->chan));

	off_ptr = &pll_tune_reg_offsets_20693[0];
	val_ptr = &ci20693_pll->wl_xtal_cfg3;

	for (ct = 0; ct < ARRAYSIZE(pll_tune_reg_offsets_20693); ct++) {
		phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
	}

	if (ACMAJORREV_33(pi->pubpi->phy_rev) &&
			PHY_AS_80P80(pi, pi->radio_chanspec) &&
			(core == 1)) {
		FOREACH_CORE(pi, rdcore) {
		   uint16 radio_tune_reg_offsets_20693[] = {
				RADIO_REG_20693(pi, LNA2G_TUNE,			rdcore),
				RADIO_REG_20693(pi, LOGEN2G_RCCR,		rdcore),
				RADIO_REG_20693(pi, TXMIX2G_CFG5,		rdcore),
				RADIO_REG_20693(pi, PA2G_CFG2,			rdcore),
				RADIO_REG_20693(pi, TX_LOGEN2G_CFG1,	rdcore),
				RADIO_REG_20693(pi, LNA5G_TUNE,			rdcore),
				RADIO_REG_20693(pi, LOGEN5G_RCCR,		rdcore),
				RADIO_REG_20693(pi, TX5G_TUNE,			rdcore),
				RADIO_REG_20693(pi, TX_LOGEN5G_CFG1,	rdcore)
			};

			if (rdcore >= 2) {
				for (ct = 0; ct < ARRAYSIZE(radio_tune_reg_offsets_20693); ct++) {
					off_ptr = &radio_tune_reg_offsets_20693[0];
					phy_utils_write_radioreg(pi,
						off_ptr[ct], val_ptr[ctbase + ct]);
					PHY_INFORM(("wl%d: %s, %6x \n", pi->sh->unit, __FUNCTION__,
						phy_utils_read_radioreg(pi, off_ptr[ct])));
				}
			}
		}
	} else {
		/* Do not apply these settings when configuring secondary PLL
		 * for PHYMODE_3x3_1x1 (core = 1).
		 */
		uint16 radio_tune_allreg_offsets_20693[] = {
			RADIO_ALLREG_20693(pi, LNA2G_TUNE),
			RADIO_ALLREG_20693(pi, LOGEN2G_RCCR),
			RADIO_ALLREG_20693(pi, TXMIX2G_CFG5),
			RADIO_ALLREG_20693(pi, PA2G_CFG2),
			RADIO_ALLREG_20693(pi, TX_LOGEN2G_CFG1),
			RADIO_ALLREG_20693(pi, LNA5G_TUNE),
			RADIO_ALLREG_20693(pi, LOGEN5G_RCCR),
			RADIO_ALLREG_20693(pi, TX5G_TUNE),
			RADIO_ALLREG_20693(pi, TX_LOGEN5G_CFG1)
		};
		uint16 radio_tune_reg_core3_offsets_20693[] = {
			RADIO_REG_20693(pi, LNA2G_TUNE, 3),
			RADIO_REG_20693(pi, LOGEN2G_RCCR, 3),
			RADIO_REG_20693(pi, TXMIX2G_CFG5, 3),
			RADIO_REG_20693(pi, PA2G_CFG2, 3),
			RADIO_REG_20693(pi, TX_LOGEN2G_CFG1, 3),
			RADIO_REG_20693(pi, LNA5G_TUNE, 3),
			RADIO_REG_20693(pi, LOGEN5G_RCCR, 3),
			RADIO_REG_20693(pi, TX5G_TUNE, 3),
			RADIO_REG_20693(pi, TX_LOGEN5G_CFG1, 3)
		};
		if (core == 1 && phy_get_phymode(pi) == PHYMODE_3x3_1x1) {
			off_ptr = &radio_tune_reg_core3_offsets_20693[0];
			for (ct = 0; ct < ARRAYSIZE(radio_tune_reg_core3_offsets_20693); ct++) {
				phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ctbase + ct]);
				PHY_INFORM(("wl%d: %s, %6x \n", pi->sh->unit, __FUNCTION__,
				    phy_utils_read_radioreg(pi, off_ptr[ct])));
			}
		} else {
			off_ptr = &radio_tune_allreg_offsets_20693[0];
			for (ct = 0; ct < ARRAYSIZE(radio_tune_allreg_offsets_20693); ct++) {
				phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ctbase + ct]);
				PHY_INFORM(("wl%d: %s, %6x \n", pi->sh->unit, __FUNCTION__,
				    phy_utils_read_radioreg(pi, off_ptr[ct])));
			}
		}
	}
}

static void wlc_phy_radio20693_rffe_rsdb_wr(phy_info_t *pi, uint8 core, uint8 reg_type,
	uint16 reg_val, uint16 reg_off)
{
	uint8 access_info, write, is_crisscross_wr;
	access_info = wlc_phy_radio20693_tuning_get_access_info(pi, core, reg_type);
	write = (access_info >> RADIO20693_TUNE_REG_WR_SHIFT) & 1;
	is_crisscross_wr = (access_info >> RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT) & 1;

	PHY_TRACE(("wl%d: %s\n"
		"core: %d reg_type: %d reg_val: %x reg_off: %x write %d is_crisscross_wr %d\n",
		pi->sh->unit, __FUNCTION__,
		core, reg_type, reg_val, reg_off, write, is_crisscross_wr));

	if (write != 0) {
		/* switch base address to other core to access regs via criss-cross path */
		if (is_crisscross_wr == 1) {
			si_d11_switch_addrbase(pi->sh->sih, !core);
		}

		phy_utils_write_radioreg(pi, reg_off, reg_val);

		/* restore the based adress */
		if (is_crisscross_wr == 1) {
			si_d11_switch_addrbase(pi->sh->sih, core);
		}
	}
}
static void wlc_phy_radio20693_rffe_tune(phy_info_t *pi, const void *chan_info_rffe,
	uint8 core)
{
	const chan_info_radio20693_rffe_t *ci20693_rffe = chan_info_rffe;
	const uint16 *val_ptr;
	uint16 *off_ptr;
	uint8 ct;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint16 phymode = phy_get_phymode(pi);

	uint16 rffe_tune_reg_offsets_20693_cr0[RADIO20693_NUM_RFFE_TUNE_REGS] = {
		RADIO_REG_20693(pi, LNA2G_TUNE, 0),
		RADIO_REG_20693(pi, LNA5G_TUNE, 0),
		RADIO_REG_20693(pi, PA2G_CFG2,  0)
	};
	uint16 rffe_tune_reg_offsets_20693_cr1[RADIO20693_NUM_RFFE_TUNE_REGS] = {
		RADIO_REG_20693(pi, LNA2G_TUNE, 1),
		RADIO_REG_20693(pi, LNA5G_TUNE, 1),
		RADIO_REG_20693(pi, PA2G_CFG2,  1)
	};

	PHY_TRACE(("wl%d: %s core: %d crisscross_actv: %d\n", pi->sh->unit, __FUNCTION__, core,
		pi_ac->radioi->is_crisscross_actv));

	val_ptr = &ci20693_rffe->lna2g_tune;
	if ((pi_ac->radioi->is_crisscross_actv == 0) || (phymode == PHYMODE_MIMO)) {
		/* crisscross is disabled => its a 2 antenna board.
		in the case of RSDB mode:
			this proc gets called during each core init and hence
			just writing to current core would suffice.
		in the case of 80p80 mode:
			this proc gets called twice since fc is a two
			channel list and hence just writing to current core would suffice.
		in the case of MIMO mode:
			it should be a 2 antenna board.
		so, just writing to $core would take care of all the use case.
		 */
		off_ptr = (core == 0) ? &rffe_tune_reg_offsets_20693_cr0[0] :
			&rffe_tune_reg_offsets_20693_cr1[0];
		for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++)
			phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
	} else {
		/* crisscross is active */
		if (phymode == PHYMODE_RSDB) {
			const uint8 *map_ptr = &rffe_tune_reg_map_20693[0];
			off_ptr = &rffe_tune_reg_offsets_20693_cr0[0];
			for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++) {
				wlc_phy_radio20693_rffe_rsdb_wr(pi, phy_get_current_core(pi),
					map_ptr[ct], val_ptr[ct], off_ptr[ct]);
			}
		} else if (phymode == PHYMODE_80P80) {
			uint8 access_info, write, is_crisscross_wr;

			access_info = wlc_phy_radio20693_tuning_get_access_info(pi, core, 0);
			write = (access_info >> RADIO20693_TUNE_REG_WR_SHIFT) & 1;
			is_crisscross_wr = (access_info >>
				RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT) & 1;

			PHY_TRACE(("wl%d: %s\n write %d is_crisscross_wr %d\n",
				pi->sh->unit, __FUNCTION__,	write, is_crisscross_wr));

			if (write != 0) {
				core = (is_crisscross_wr == 0) ? core : !core;
				off_ptr = (core == 0) ? &rffe_tune_reg_offsets_20693_cr0[0] :
					&rffe_tune_reg_offsets_20693_cr1[0];

				for (ct = 0; ct < RADIO20693_NUM_RFFE_TUNE_REGS; ct++) {
					phy_utils_write_radioreg(pi, off_ptr[ct], val_ptr[ct]);
				}
			} /* Write != 0 */
		} /* 80P80 */
	} /* Criss-Cross Active */
	if (ACMAJORREV_4(pi->pubpi->phy_rev) && (!(PHY_IPA(pi))) &&
		(RADIO20693REV(pi->pubpi->radiorev) == 13) && CHSPEC_IS2G(pi->radio_chanspec)) {
		/* 4349A2 TX2G die-2/rev-13 iPA used as ePA */
		wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(pi, core,
			CHSPEC_CHANNEL(pi->radio_chanspec));
	}
}

/* This function return 2bits.
bit 0: 0 means donot write. 1 means write
bit 1: 0 means direct access. 1 means criss-cross access
*/
static uint8 wlc_phy_radio20693_tuning_get_access_info(phy_info_t *pi, uint8 core, uint8 reg_type)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 access_info = 0;

	ASSERT(phymode != PHYMODE_MIMO);

	PHY_TRACE(("wl%d: %s core: %d phymode: %d\n", pi->sh->unit, __FUNCTION__, core, phymode));

	if (phymode == PHYMODE_80P80) {
		if (core != pi_ac->radioi->crisscross_priority_core_80p80) {
			access_info = 0;
		} else {
			access_info = (1 << RADIO20693_TUNE_REG_WR_SHIFT);
			/* figure out which core has 5G affinity */
			if (RADIO20693_CORE1_AFFINITY == 5) {
				access_info |= (core == 1) ? 0 :
					(1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
			} else {
				access_info |= (core == 0) ? 0 :
					(1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
			}
		}
	} else if (phymode == PHYMODE_RSDB)	{
		uint8 band_curr_cr, band_oth_cr;
		phy_info_t *other_pi = phy_get_other_pi(pi);

		band_curr_cr = CHSPEC_IS2G(pi->radio_chanspec) ? RADIO20693_TUNEREG_TYPE_2G :
			RADIO20693_TUNEREG_TYPE_5G;
		band_oth_cr = CHSPEC_IS2G(other_pi->radio_chanspec) ? RADIO20693_TUNEREG_TYPE_2G :
			RADIO20693_TUNEREG_TYPE_5G;

		PHY_TRACE(("wl%d: %s current_core: %d other_core: %d\n", pi->sh->unit, __FUNCTION__,
			core, !core));
		PHY_TRACE(("wl%d: %s band_curr_cr: %d band_oth_cr: %d\n", pi->sh->unit,
			__FUNCTION__, band_curr_cr, band_oth_cr));

		/* if a tuning reg is neither a 2G reg nor a 5g reg,
		   then assume this register belongs to band of current core
		 */
		if (reg_type == RADIO20693_TUNEREG_TYPE_NONE) {
			reg_type = band_curr_cr;
		}

		/* Do not write anything if reg_type and band_curr_cr are different */
		if (reg_type != band_curr_cr) {
			access_info = 0;
			PHY_TRACE(("w%d: %s. IGNORE: reg_type and band_curr_cr are different\n",
				pi->sh->unit, __FUNCTION__));
			return (access_info);
		}

		/* find out if there is a conflict of bands between two cores.
		 if 	--> no conflict case
		 else 	--> Conflict case
		 */
		if (band_curr_cr != band_oth_cr) {
			PHY_TRACE(("wl%d: %s. No conflict case. bands are different\n",
				pi->sh->unit, __FUNCTION__));

			/* find out if its a direct access or criss-cross_access */
			access_info = wlc_phy_radio20693_tuning_get_access_type(pi, core, reg_type);
		} else {
			PHY_TRACE(("wl%d: %s. Conflict case. bands are same\n",
				pi->sh->unit, __FUNCTION__));
			if (core != pi_ac->radioi->crisscross_priority_core_rsdb) {
				PHY_TRACE(("wl%d: %s IGNORE: priority is not met\n",
					pi->sh->unit, __FUNCTION__));

				PHY_TRACE(("wl%d: %s current_core: %d Priority: %d\n",
					pi->sh->unit, __FUNCTION__, core,
					pi_ac->radioi->crisscross_priority_core_rsdb));

				access_info = 0;
			}

			/* find out if its a direct access or criss-cross_access */
			access_info = wlc_phy_radio20693_tuning_get_access_type(pi, core, reg_type);
		}
	}

	PHY_TRACE(("wl%d: %s. Access Info %x\n",
		pi->sh->unit, __FUNCTION__, access_info));

	return (access_info);
}
static uint8 wlc_phy_radio20693_tuning_get_access_type(phy_info_t *pi, uint8 core, uint8 reg_type)
{
	uint8 is_dir_access, access_info;

	/* find out if its a direct access or criss-cross_access */
	is_dir_access = ((core == 0) && (reg_type == RADIO20693_CORE0_AFFINITY));
	is_dir_access |= ((core == 1) && (reg_type == RADIO20693_CORE1_AFFINITY));

	/* its a direct access */
	access_info = (1 << RADIO20693_TUNE_REG_WR_SHIFT);

	if (is_dir_access == 0) {
		/* Its a criss-cross access */
		access_info |= (1 << RADIO20693_TUNE_REG_CRISSCROSS_WR_SHIFT);
	}

	return (access_info);
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

/* ********************************************* */
/*				Internal Definitions					*/
/* ********************************************* */

static void
wlc_phy_radio20693_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;
	/* Power up the TX LDO */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_en, 0x1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_TXldo_pu, 1);
	}
	OSL_DELAY(110);
	/* Power up the radio Band gap and the 2.5V reg */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, VREG_CFG, core, vreg25_pu, 1);
		if (RADIOMAJORREV(pi) == 3) {
			MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_pu, 1);
		} else {
			MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_pu, 1);
		}
	}
	OSL_DELAY(10);
	/* Power up the VCO, AFE, RX & ADC Ldo's */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_RXldo_pu, 1);
		MOD_RADIO_REG_TINY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 1);
	}
	OSL_DELAY(12);
	/* Enable the Synth and Force the wlpmu_spare to 0 */
	if (RADIOMAJORREV(pi) != 3) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, PLL_CFG1, core, synth_pwrsw_en, 1);
			MOD_RADIO_REG_TINY(pi, PMU_CFG5, core, wlpmu_spare, 0);
		}
	}
}

static void wlc_phy_radio20698_minipmu_pwron_seq(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_minipmu_pwron_seq */
	uint8 core;

	/* Turn on 1p8LDOs and Bandgap  */
	MOD_RADIO_PLLREG_20698(pi, LDO1P8_STAT, 0, ldo1p8_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, WLLDO1P8_OVR, 0, ovr_ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20698(pi, LDO1P8_STAT, 1, ldo1p8_pu, 0x1);
	MOD_RADIO_REG_20698(pi, WLLDO1P8_OVR, 1, ovr_ldo1p8_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, BG_REG2, 0, bg_pu_bgcore, 0x1);
	MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_bg_pu_bgcore, 0x1);
	MOD_RADIO_PLLREG_20698(pi, BG_REG2, 0, bg_pu_V2I, 0x1);
	MOD_RADIO_PLLREG_20698(pi, BG_OVR1, 0, ovr_bg_pu_V2I, 0x1);

	/* Hold RF PLL in reset */
	MOD_RADIO_PLLREG_20698(pi, PLL_CFG2, 0, rfpll_rst_n, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, 0, ovr_rfpll_rst_n, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_VCOCAL1, 0, rfpll_vcocal_rst_n, 0x0);
	MOD_RADIO_PLLREG_20698(pi, PLL_VCOCAL_OVR1, 0, ovr_rfpll_vcocal_rst_n, 0x1);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG4, core, wlpmu_en, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_en, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LDO1P65_STAT, core, wlpmu_ldo1p6_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_ldo1p6_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG1, core, wlpmu_AFEldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_AFEldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG1, core, wlpmu_TXldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_TXldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG1, core, wlpmu_LOGENldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_LOGENldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG1, core, wlpmu_ADCldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_ADCldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG4, core, wlpmu_LDO2P1_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_OVR1, core, ovr_wlpmu_LDO2P1_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG4, core, rx_ldo_pu, 0x1)
			// Use miniPMU reference for power up
			MOD_RADIO_REG_20698_ENTRY(pi, PMU_CFG3, core, vref_select, 0x0)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

#ifdef MINIPMU_RESOURCE_20704
static void wlc_phy_radio20704_minipmu_pwron_seq(phy_info_t *pi)
{
	/* 20704_procs.tcl r744991: 20704_minipmu_pwron_seq */
	uint8 core;

	RADIO_REG_LIST_START
		/* main BG is turned on */
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, core);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20704_ENTRY(pi, BG_OVR2, core, ovr_bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_OVR2, core, ovr_bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ptat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_REG13, core, bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_REG13, core, bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_REG13, core, bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20704_ENTRY(pi, BG_REG13, core, bg_ptat_uncal_mirror_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}
#endif /* MINIPMU_RESOURCE_20704 */

#ifdef MINIPMU_RESOURCE_20707
static void wlc_phy_radio20707_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;

	RADIO_REG_LIST_START
		/* main BG is turned on */
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, core);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20707_ENTRY(pi, BG_OVR2, core, ovr_bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_OVR2, core,
				ovr_bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_OVR2, core, ovr_bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_OVR2, core,
				ovr_bg_ptat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_REG13, core, bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_REG13, core, bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_REG13, core, bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20707_ENTRY(pi, BG_REG13, core, bg_ptat_uncal_mirror_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}
#endif /* MINIPMU_RESOURCE_20707 */

#ifdef MINIPMU_RESOURCE_20708
static void wlc_phy_radio20708_minipmu_pwron_seq(phy_info_t *pi)
{
	uint8 core;

	RADIO_REG_LIST_START
		/* main BG is turned on */
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20708_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, core);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20708_ENTRY(pi, BG_OVR2, core, ovr_bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_OVR2, core,
				ovr_bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_OVR2, core, ovr_bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_OVR2, core,
				ovr_bg_ptat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_REG13, core, bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_REG13, core, bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_REG13, core, bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, BG_REG13, core, bg_ptat_uncal_mirror_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}
#endif /* MINIPMU_RESOURCE_20708 */

#ifdef MINIPMU_RESOURCE_20709
static void wlc_phy_radio20709_minipmu_pwron_seq(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_minipmu_pwron_seq */
	uint8 core;

	RADIO_REG_LIST_START
		/* main BG is turned on */
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, core);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20709_ENTRY(pi, BG_OVR2, core, ovr_bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_OVR2, core, ovr_bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ptat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_REG13, core, bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_REG13, core, bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_REG13, core, bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20709_ENTRY(pi, BG_REG13, core, bg_ptat_uncal_mirror_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}
#endif /* MINIPMU_RESOURCE_20709 */

#ifdef MINIPMU_RESOURCE_20710
static void wlc_phy_radio20710_minipmu_pwron_seq(phy_info_t *pi)
{
	/* 20710_procs.tcl r744991: 20710_minipmu_pwron_seq */
	uint8 core;

	RADIO_REG_LIST_START
		/* main BG is turned on */
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_clk10M_en, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_bg_ref, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_vref_sel, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, BG_REG10, 0, bg_wlpmu_pu_cbuck_ref, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, core);

	FOREACH_CORE(pi, core) {
		RADIO_REG_LIST_START
			/* Overrides */
			MOD_RADIO_REG_20710_ENTRY(pi, BG_OVR2, core, ovr_bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_OVR2, core, ovr_bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_OVR2, core,
					ovr_bg_ptat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_REG13, core, bg_ctat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_REG13, core, bg_ctat_uncal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_REG13, core, bg_ptat_cal_mirror_pu, 0x1)
			MOD_RADIO_REG_20710_ENTRY(pi, BG_REG13, core, bg_ptat_uncal_mirror_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}
#endif /* MINIPMU_RESOURCE_20710 */

static void wlc_phy_radio20693_pmu_override(phy_info_t *pi)
{
	uint8  core;
	uint16 data;

	FOREACH_CORE(pi, core) {
		data = READ_RADIO_REG_20693(pi, PMU_OVR, core);
		data = data | 0xFC;
		phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OVR, core), data);
	}
}

static void
phy_ac_radio_20693_xtal_pwrup(phy_info_t *pi, uint8 core)
{
	RADIO_REG_LIST_START
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, core,
			ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, core,
			ovr_ldo_1p2_xtalldo1p2_vref_bias_reset, 1)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, core, ldo_1p2_xtalldo1p2_BG_pu, 1)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, core, ovr_ldo_1p2_xtalldo1p2_BG_pu, 1)
	RADIO_REG_LIST_EXECUTE(pi, core);
	OSL_DELAY(300);
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, core, ldo_1p2_xtalldo1p2_vref_bias_reset, 0);
}

static void
wlc_phy_radio20693_xtal_pwrup(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	/* Note: Core0 XTAL will be powerup by PMUcore in chipcommon */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		((phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1))) {
		/* Powerup xtal ldo for MAC_Core = 1 in case of RSDB mode */
		phy_ac_radio_20693_xtal_pwrup(pi, 0);
	} else if ((phy_get_phymode(pi) != PHYMODE_RSDB)) {
		/* Powerup xtal ldo for Core 1 in case of MIMO and 80+80 */
		phy_ac_radio_20693_xtal_pwrup(pi, 1);
	}
}

static void
wlc_phy_radio20698_xtal_pwrup(phy_info_t *pi)
{
	/* 20698_procs.tcl r724356: 20698_xtal_pwrup */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID));

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_spare, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_resetb, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0_OVR, 0, ovr_xtal_resetb, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_bypass, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0_OVR, 0, ovr_xtal_bypass, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_PWRDN, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0_OVR, 0, ovr_xtal_PWRDN, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_pd, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0_OVR, 0, ovr_xtal_pd, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL0, 0, xtal_xcore_bias, 0x1)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_en_drv, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_div2_sel, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_cml_en_ch, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_LDO_tail, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_LDO_Vctrl, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_LDO_bias, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_cml_cur, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_cmos_en_ALL, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL1, 0, xtal_drv_cur, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL2, 0, xtal_d2c_bias, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_ctrl1, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL2, 0, xtal_cmos_en_ch, 0x3f)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_ctrl0, 0x0)
		MOD_RADIO_PLLREG_20698_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_en_ch, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
}

static void
wlc_phy_radio20704_xtal_pwrup(phy_info_t *pi)
{
	/* 20704_procs.tcl r773183: 20704_xtal_pwrup */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID));

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20704_ENTRY(pi, PLL_REFDOUBLER5, 0, RefDoubler_pfdclk0_pu, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_delay, 0x8)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL0, 0, xtal_spare, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL0, 0, xtal_xcore_bias, 0x1)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_cml_en_ch, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_LDO_tail, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_LDO_Vctrl, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_LDO_bias, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_cml_cur, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL1, 0, xtal_cmos_en_ALL, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL2, 0, xtal_d2c_bias, 0x0)
		MOD_RADIO_PLLREG_20704_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_en_ch, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
}

static void
wlc_phy_radio20707_xtal_pwrup(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID));

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20707_ENTRY(pi, PLL_REFDOUBLER5, 0, RefDoubler_pfdclk0_pu, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_delay, 0x8)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL0, 0, xtal_spare, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL0, 0, xtal_xcore_bias, 0x1)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_cml_en_ch, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_LDO_tail, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_LDO_Vctrl, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_LDO_bias, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_cml_cur, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL1, 0, xtal_cmos_en_ALL, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL2, 0, xtal_d2c_bias, 0x0)
		MOD_RADIO_PLLREG_20707_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_en_ch, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
}

static void
wlc_phy_radio20708_xtal_pwrup(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID));

	//MOD_RADIO_PLLREG_20708(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1);
}

static void
wlc_phy_radio20709_xtal_pwrup(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_xtal_pwrup */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID));

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER5, 0, RefDoubler_pfdclk0_pu, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_delay, 0x8)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkvcocal, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrcal, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL0, 0, xtal_spare, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL0, 0, xtal_xcore_bias, 0x1)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_cml_en_ch, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_LDO_tail, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_LDO_Vctrl, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_LDO_bias, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_cml_cur, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL1, 0, xtal_cmos_en_ALL, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL2, 0, xtal_d2c_bias, 0x0)
		MOD_RADIO_PLLREG_20709_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_en_ch, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);
}

static void
wlc_phy_radio20710_xtal_pwrup(phy_info_t *pi)
{
	/* 20710_procs.tcl r967651: 20710_xtal_pwrup */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID));

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20710_ENTRY(pi, PLL_REFDOUBLER5, 0, RefDoubler_pfdclk0_pu, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_pu, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, PLL_REFDOUBLER1, 0, RefDoubler_delay, 0x8)
	RADIO_REG_LIST_EXECUTE(pi, 0);
	OSL_DELAY(1);

	RADIO_REG_LIST_START
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL0, 0, xtal_spare, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL0, 0, xtal_xcore_bias, 0x1)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL1, 0, xtal_cml_en_ch, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL1, 0, xtal_LDO_tail, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL1, 0, xtal_LDO_bias, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL1, 0, xtal_cml_cur, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL1, 0, xtal_cmos_en_ALL, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL2, 0, xtal_d2c_bias, 0x0)
		MOD_RADIO_PLLREG_20710_ENTRY(pi, XTAL2, 0, xtal_cmos_pg_en_ch, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);

	MOD_RADIO_PLLREG_20710(pi, XTAL1, 0, xtal_LDO_Vctrl,
			pi->sh->sih->otpflag & WIFI0_HWCFG_OPT0_IND ? 2 : 0);
}

static void
wlc_phy_radio20693_upd_prfd_values(phy_info_t *pi)
{
	uint i = 0;
	const radio_20xx_prefregs_t *prefregs_20693_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* Choose the right table to use */
	switch (RADIO20693REV(pi->pubpi->radiorev)) {
	case 3:
	case 4:
	case 5:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 6:
		prefregs_20693_ptr = prefregs_20693_rev6;
		break;
	case 7:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 8:
		prefregs_20693_ptr = prefregs_20693_rev6;
		break;
	case 10:
		prefregs_20693_ptr = prefregs_20693_rev10;
		break;
	case 11:
	case 12:
	case 13:
		prefregs_20693_ptr = prefregs_20693_rev13;
		break;
	case 14:
		prefregs_20693_ptr = prefregs_20693_rev14;
		break;
	case 19:
		prefregs_20693_ptr = prefregs_20693_rev19;
		break;
	case 15:
		if (PHY_XTAL_IS37M4(pi)) {
			prefregs_20693_ptr = prefregs_20693_rev15_37p4MHz;
		} else {
			prefregs_20693_ptr = prefregs_20693_rev15_40MHz;
		}
		break;
	case 16:
	case 17:
	case 20:
		prefregs_20693_ptr = prefregs_20693_rev5;
		break;
	case 18:
	case 21:
		if (PHY_XTAL_IS37M4(pi)) {
			prefregs_20693_ptr = prefregs_20693_rev18;
		} else {
			prefregs_20693_ptr = prefregs_20693_rev15_40MHz;
		}
		break;
	case 23:
		/* Preferred values for 53573/53574/47189-B0 */
		prefregs_20693_ptr = prefregs_20693_rev23_40MHz;
		break;
	case 32:
	case 33:
		prefregs_20693_ptr = prefregs_20693_rev32;
		break;
	case 9:
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO20693REV(pi->pubpi->radiorev)));
		ASSERT(FALSE);
		return;
	}

	/* Update preferred values */
	while (prefregs_20693_ptr[i].address != 0xffff) {
		if (!((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(prefregs_20693_ptr[i].address & JTAG_20693_CR1)))
			phy_utils_write_radioreg(pi, prefregs_20693_ptr[i].address,
				(uint16)prefregs_20693_ptr[i].init);
		i++;
	}
}

void
wlc_phy_radio20693_vco_opt(phy_info_t *pi, bool isVCOTUNE_5G)
{
	uint8 core;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	PHY_TRACE(("%s\n", __FUNCTION__));
	if (pi_ac->radioi->lpmode_2g != ACPHY_LPMODE_NONE) {
		if (CHSPEC_IS2G(pi->radio_chanspec) && (PHY_IPA(pi))) {
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_VCO3, core,
						rfpll_vco_en_alc, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_VCO6, core,
						rfpll_vco_ALC_ref_ctrl, 0x8)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO2, core,
						ldo_2p5_lowquiescenten_CP, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO2, core,
						ldo_2p5_lowquiescenten_VCO, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO4, core,
						ldo_2p5_static_load_CP, 0x1)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO4, core,
						ldo_2p5_static_load_VCO, 0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		} else {
			FOREACH_CORE(pi, core) {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_VCO3, core,
						rfpll_vco_en_alc, 0x0)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_VCO6, core,
						rfpll_vco_ALC_ref_ctrl, 0x8)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO2, core,
						ldo_2p5_lowquiescenten_CP, 0x0)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO2, core,
						ldo_2p5_lowquiescenten_VCO, 0x0)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO4, core,
						ldo_2p5_static_load_CP, 0x0)
					MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO4, core,
						ldo_2p5_static_load_VCO, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
			}
		}
	}
	/* Loading vcotune settings */
	if (ROUTER_4349(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20693(pi, PLL_VCO3, core,
				rfpll_vco_en_alc, isVCOTUNE_5G ? 0x1 : 0x0);
			MOD_RADIO_REG_20693(pi, PLL_VCO6, core,
				rfpll_vco_ALC_ref_ctrl, isVCOTUNE_5G ? 0x6 : 0x0);
			MOD_RADIO_REG_20693(pi, PMU_CFG2, core,
				wlpmu_VCOldo_adj, isVCOTUNE_5G ? 0x3 : 0x0);
		}
	}
}

int8
wlc_phy_tiny_radio_minipmu_cal(phy_info_t *pi)
{
	uint8 core, calsuccesful = 1;
	int8 cal_status = 1;
	int16 waitcounter = 0;
	phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
	bool is_rsdb_core1_cal = FALSE;

	if (BCM53573_CHIP(pi->sh->chip) && (CHIPREV((pi)->sh->chiprev) == 0)) {
		return 0;
	}

	ASSERT(TINY_RADIO(pi));
	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return 0;

	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) &&
		(phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1) &&
		(RADIO20693REV(pi->pubpi->radiorev) <= 0x17) &&
		(RADIO20693REV(pi->pubpi->radiorev) != 0x13)) {
		 // this fix is not needed from 4355B1 variants
		is_rsdb_core1_cal = TRUE;
		wlc_phy_cals_mac_susp_en_other_cr(pi, TRUE);
	}

	/* Setup the cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_RXldo_pu, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}

	FOREACH_CORE(pi, core) {
		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
			if (RADIOMAJORREV(pi) == 2) {
				MOD_RADIO_REG_20693(pi, PMU_CFG3, core, wlpmu_selavg_lo, 0);
			} else {
				MOD_RADIO_REG_20693(pi, PMU_CFG3, core, wlpmu_selavg, 0);
			}
		}
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_vref_select, 1);
		MOD_RADIO_REG_TINY(pi, PMU_CFG2, core, wlpmu_bypcal, 0);
		MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 1);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		/* ldobg_cal_clken coming out from radiodig_xxx_core1 is not used */
		if (is_rsdb_core1_cal) {
			MOD_RADIO_REG_TINY(pi_core0, PMU_STAT, core, wlpmu_ldobg_cal_clken, 1);
		} else {
			MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 1);
		}
	}
	OSL_DELAY(100);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 1);
	}
	FOREACH_CORE(pi, core) {
		/* Wait for cal_done */
		if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
			ACMAJORREV_33(pi->pubpi->phy_rev)) {
			waitcounter = 0;
			calsuccesful = 1;
			while (READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core,
				wlpmu_ldobg_cal_done) == 0) {
				OSL_DELAY(100);
				waitcounter ++;
				if (waitcounter > 100) {
					/* cal_done bit is not 1 even after waiting for a while */
					/* Exit gracefully */
					PHY_TRACE(("\nWarning:Mini PMU Cal Failed on Core%d \n",
						core));
					calsuccesful = 0;
					break;
				}
			}
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
			if (calsuccesful == 1) {
				PHY_TRACE(("MiniPMU cal done on core %d, Cal code = %d", core,
					READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_calcode)));
			} else {
				PHY_TRACE(("Cal Unsuccesful on Core %d", core));
				cal_status = -1;
			}
		} else {
			SPINWAIT(READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core,
				wlpmu_ldobg_cal_done) == 0, ACPHY_SPINWAIT_MINIPMU_CAL_STATUS);
			if (READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_done) == 0) {
				PHY_ERROR(("%s : Cal Unsuccesful on Core %d\n", __FUNCTION__,
					core));
				cal_status = -1;
				PHY_FATAL_ERROR_MESG(
					(" %s: SPINWAIT ERROR : PMU cal failed on Core%d\n",
					__FUNCTION__, core));
				PHY_FATAL_ERROR(pi, PHY_RC_PMUCAL_FAILED);
			}
			MOD_RADIO_REG_TINY(pi, PMU_OP, core, wlpmu_ldoref_start_cal, 0);
			PHY_TRACE(("MiniPMU cal done on core %d, Cal code = %d\n", core,
				READ_RADIO_REGFLD_TINY(pi, PMU_STAT, core, wlpmu_calcode)));
		}
	}
	FOREACH_CORE(pi, core) {
		if (is_rsdb_core1_cal) {
			MOD_RADIO_REG_TINY(pi_core0, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		} else {
			MOD_RADIO_REG_TINY(pi, PMU_STAT, core, wlpmu_ldobg_cal_clken, 0);
		}
	}
	if (is_rsdb_core1_cal) {
		wlc_phy_cals_mac_susp_en_other_cr(pi, FALSE);
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		FOREACH_CORE(pi, core) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_VCOldo_pu, 1)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_AFEldo_pu, 1)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_OP, core, wlpmu_RXldo_pu, 1)
				MOD_RADIO_REG_TINY_ENTRY(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 1)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	}

	return cal_status;
}

static void
wlc_phy_radio20693_pmu_pll_config(phy_info_t *pi)
{
	uint8 core;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* # powerup pll */
	FOREACH_CORE(pi, core) {
		if ((phy_get_phymode(pi) == PHYMODE_MIMO) && (core != 0))
			break;

		RADIO_REG_LIST_START
			/* # VCO */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_CFG1, core, rfpll_vco_pu, 1)
			/* # VCO LDO */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_VCO, 1)
			/* # VCO buf */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 1)
			/* # Synth global */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_CFG1, core, rfpll_synth_pu, 1)
			/* # Charge Pump */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_CP1, core, rfpll_cp_pu, 1)
			/* # Charge Pump LDO */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_CP, 1)
			/* # PLL lock monitor */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_CFG1, core, rfpll_monitor_pu, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_CP, 1)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_VCO, 1)
		RADIO_REG_LIST_EXECUTE(pi, core);
	}
	OSL_DELAY(89);
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_CP, 0);
		MOD_RADIO_REG_20693(pi, PLL_HVLDO1, core, ldo_2p5_bias_reset_VCO, 0);
	}
}

void
phy_ac_radio_20693_pmu_pll_config_wave2(phy_info_t *pi, uint8 pll_mode)
{
	// PLL/VCO operating modes: set by pll_mode
	// Mode 0. RFP0 non-coupled, e.g. 4x4 MIMO non-1024QAM
	// Mode 1. RFP0 coupled, e.g. 4x4 MIMO 1024QAM
	// Mode 2. RFP0 non-coupled + RFP1 non-coupled: 2x2 + 2x2 MIMO non-1024QAM
	// Mode 3. RFP0 non-coupled + RFP1 coupled: 3x3 + 1x1 scanning in 80MHz mode
	// Mode 4. RFP0 coupled + RFP1 non-coupled: 3x3 MIMO 1024QAM + 1x1 scanning in 20MHz mode
	// Mode 5. RFP0 coupled + RFP1 coupled: 3x3 MIMO 1024QAM + 1x1 scanning in 80MHz mode
	// Mode 6. RFP1 non-coupled
	// Mode 7. RFP1 coupled

	uint8 core, start_core, end_core;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_INFORM(("wl%d: %s: pll_mode %d\n",
			pi->sh->unit, __FUNCTION__, pll_mode));

	switch (pll_mode) {
	case 0:
	        // intentional fall through
	case 1:
	        start_core = 0;
		end_core   = 0;
		break;
	case 2:
	        // intentional fall through
	case 3:
	        // intentional fall through
	case 4:
	        // intentional fall through
	case 5:
	        start_core = 0;
		end_core   = 1;
		break;
	case 6:
	        // intentional fall through
	case 7:
	        start_core = 1;
		end_core   = 1;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported PLL/VCO operating mode %d\n",
			pi->sh->unit, __FUNCTION__, pll_mode));
		ASSERT(0);
		return;
	}

	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3f);

	for (core = start_core; core <= end_core; core++) {
		uint8  ct;
		// Put all regs/fields write/modification in a array
		uint16 pll_regs_bit_vals1[][3] = {
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_synth_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_cp_pu,    1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_vco_buf_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_ldo_2p5_pu_ldo_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_ldo_2p5_pu_ldo_VCO, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_synth_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CP1,  core, rfpll_cp_pu,      1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_vco_pu,     1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1, core, rfpll_monitor_pu, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_LF6,  core, rfpll_lf_cm_pu,   1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1, core, ldo_2p5_pu_ldo_VCO, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG1,   core, rfpll_pfd_en, 0x2),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core, ovr_rfpll_rst_n, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL_OVR1, core,
			ovr_rfpll_vcocal_rst_n, 1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core,
			ovr_rfpll_ldo_cp_bias_reset,  1),
			RADIO_PLLREGC_FLD_20693(pi, RFPLL_OVR1, core,
			ovr_rfpll_ldo_vco_bias_reset, 1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG2,    core, rfpll_rst_n,            0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL1, core, rfpll_vcocal_rst_n,     0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_CP,  1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_VCO, 1),
		};
		uint16 pll_regs_bit_vals2[][3] = {
			RADIO_PLLREGC_FLD_20693(pi, PLL_CFG2,    core, rfpll_rst_n,            1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_VCOCAL1, core, rfpll_vcocal_rst_n,     1),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_CP,  0),
			RADIO_PLLREGC_FLD_20693(pi, PLL_HVLDO1,  core, ldo_2p5_bias_reset_VCO, 0),
		};

		// now write/modification to radio regs
		for (ct = 0; ct < ARRAYSIZE(pll_regs_bit_vals1); ct++) {
			phy_utils_mod_radioreg(pi, pll_regs_bit_vals1[ct][0],
			                       pll_regs_bit_vals1[ct][1],
			                       pll_regs_bit_vals1[ct][2]);
		}
		OSL_DELAY(10);

		for (ct = 0; ct < ARRAYSIZE(pll_regs_bit_vals2); ct++) {
			phy_utils_mod_radioreg(pi, pll_regs_bit_vals2[ct][0],
			                       pll_regs_bit_vals2[ct][1],
			                       pll_regs_bit_vals2[ct][2]);
		}
	}
}

static void
wlc_phy_switch_radio_acphy_20693(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	/* minipmu_cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)
		wlc_phy_tiny_radio_minipmu_cal(pi);

	/* r cal */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
#ifdef ATE_BUILD
	  /* ATE firmware performs the rcal and the value is put in the OTP. */
	  wlc_phy_radio_tiny_rcal_wave2(pi, 2);
#else
	  wlc_phy_radio_tiny_rcal_wave2(pi, 2);
#endif
	}

	/* power up pmu pll */
	if (RADIOMAJORREV(pi) == 3) {
		if (phy_get_phymode(pi) == PHYMODE_80P80 ||
				PHY_AS_80P80(pi, pi->radio_chanspec)) {
				/* for 4365 C0 - turn on both PLL */
			phy_ac_radio_20693_pmu_pll_config_wave2(pi, 2);
		} else {
			phy_ac_radio_20693_pmu_pll_config_wave2(pi, 0);
		}
	} else {
		wlc_phy_radio20693_pmu_pll_config(pi);
	}

	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3)) {
		/* minipmu_cal */
		wlc_phy_tiny_radio_minipmu_cal(pi);

	/* RCAL */
#ifdef ATE_BUILD
		/* ATE firmware performs the rcal and the value is put in the OTP. */
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE3_BOTH_CORE_CAL);
#else
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
/* FIXME: JIRA: HW4349-535HW:
 * Need latch for Wlan RCAL code core0/1 coming out of OTP
 * OTP bits are used to store the RCAL codes (from ATE) and are being used in radioDig_20691.
 * But when the OTP is turned off (power-off), the output of the OTP is zero. Thus, we need
 * isolation latches on these bits and store the RCAL-code at the toplevel, so the OTP can
 * be turned off and we hold the RCAL code bits.
 * Currently the WAR is to read the RCAL code from OTP and store it in the firmware and
 * configure the RadioDig20691 such that the output of OTP is not used.
 * (Other WAR/option is to keep OTP powered on whenever Radio is being turned ON.)
 */
			if ((pi->sromi->rcal_otp_val >= 2) &&
				(pi->sromi->rcal_otp_val <= 12)) {
				pi->rcal_value = pi->sromi->rcal_otp_val;
			}
		}
		wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE1_STATIC);
#endif /* ATE_BUILD */
	}
}

static void
wlc_phy_switch_radio_acphy(phy_info_t *pi, bool on)
{
	uint8 core, pll = 0, retry = 0;
	uint16 data = 0;
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
#ifdef LOW_TX_CURRENT_SETTINGS_2G
	uint8 is_ipa = 0;
#endif

	PHY_TRACE(("wl%d: %s %s corenum %d\n", pi->sh->unit, __FUNCTION__, on ? "ON" : "OFF",
		pi->pubpi->phy_corenum));

	if (on) {
		if (!pi->radio_is_on) {
			if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev16(pi);
				} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
					/*  set the low pwoer reg before radio init */
					wlc_phy_set_lowpwr_phy_reg_rev3(pi);
					wlc_phy_radio2069_mini_pwron_seq_rev32(pi);
				}
				if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
						((RADIO2069_MINORREV(pi->pubpi->radiorev) == 16) ||
						(RADIO2069_MINORREV(pi->pubpi->radiorev) == 17))) {
				   /* ToUnblock the QT tests on 4364-3x3 flavor added the */
				   /* reg config. Based on radio team reccomendatation */
				   /* to be reverted after proper settings */
				   MOD_RADIO_REG(pi, RFP, GP_REGISTER, gp_pcie, 3);
				}

				wlc_phy_radio2069_pwron_seq(pi);
				/* 4364 dual phy. when 3x3 is powered down, make sure the 1x1 */
				/* femctrl lines do not toggle ant0/ant1 this is done by giving */
				/* bt the control on ant0/ant1. If bt is not there (bt_reg_on 0) */
				/* this is fine. Ultimately bt firmware needs to write proper */
				/* values in the bt_clb_upi registers bt2clb_swctrl_smask_bt_antX */
				if (IS_4364_3x3(pi)) {
					si_gci_set_femctrl_mask_ant01(pi->sh->sih, pi->sh->osh, 1);
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
				acphy_pmu_core1_off_radregs_t *porig =
					(pi_ac->radioi->pmu_c1_off_info_orig);
				acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig =
					(pi_ac->radioi->pmu_lp_opt_orig);

				porig->is_orig = FALSE;
				pmu_lp_opt_orig->is_orig = FALSE;

				/* ------------------------------------------------------------- */
				/* In case of 4349A0, there will be 2 XTALs. Core0 XTAL will be  */
				/* Pwrdup by PMU and Core1 XTAL needs to be pwrdup accordingly   */
				/* in all the modes like RSDB, MIMO and 80P80                    */
				/* ------------------------------------------------------------- */
				wlc_phy_radio20693_xtal_pwrup(pi);

				wlc_phy_tiny_radio_pwron_seq(pi);

				/* JIRA: HW53573-139. For 4349-B0 and 53573 radio, enabling
				 * pmu_overrides to enable the pmu register programming
				 */
				if (RADIOMAJORREV(pi) == 2) {
					FOREACH_CORE(pi, core) {
						RADIO_REG_LIST_START
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_en, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_AFEldo_pu, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_TXldo_pu, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_VCOldo_pu, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_RXldo_pu, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_ADCldo_pu, 0x1)
							MOD_RADIO_REG_20693_ENTRY(pi, PMU_OVR, core,
								ovr_wlpmu_selavg_lo, 0x1)
						RADIO_REG_LIST_EXECUTE(pi, core);
					}
				}

				// 4365: Enable the PMU overrides because we don't want the powerup
				// sequence to be controlled by the PMU sequencer
				if (RADIOMAJORREV(pi) == 3)
					wlc_phy_radio20693_pmu_override(pi);

				/* Power up the radio mini PMU Seq */
				wlc_phy_radio20693_minipmu_pwron_seq(pi);

				/* pll config, minipmu cal, RCAL */
				wlc_phy_switch_radio_acphy_20693(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
				/* 20698_procs.tcl r708059: 20698_init  */

				/* Global Radio PU's */
				wlc_phy_radio20698_pwron_seq_phyregs(pi);

				/* Xtal powerup */
				wlc_phy_radio20698_xtal_pwrup(pi);

				/* PU the TX/RX/VCO/..LDO's  */
				wlc_phy_radio20698_minipmu_pwron_seq(pi);

				/* Turn on reference clocks */
				wlc_phy_radio20698_refclk_en(pi);

				/* R_CAL: apply static rcal value */
				wlc_phy_radio20698_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20698_minipmu_cal(pi);

				/* Actual R_CAL */
				wlc_phy_radio20698_r_cal(pi, 2);

				/* RC CAL */
				wlc_phy_radio20698_rc_cal(pi);

				/* PLL PWR-UP */
				wlc_phy_radio20698_pmu_pll_pwrup(pi, 0);
				wlc_phy_radio20698_pmu_pll_pwrup(pi, 1);

				/* WAR's */
				wlc_phy_radio20698_wars(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
				/* 20704_procs.tcl r769742: 20704_init  */

				/* Global Radio PU's */
				wlc_phy_radio20704_pwron_seq_phyregs(pi);

				/* Xtal powerup */
				wlc_phy_radio20704_xtal_pwrup(pi);

				/* PU the TX/RX/VCO/..LDO's  */
#ifdef MINIPMU_RESOURCE_20704
				/* 20180625 AJG Don't need this, PMU Resource can take care of it */
				wlc_phy_radio20704_minipmu_pwron_seq(pi);
#endif

				/* R_CAL: apply static rcal value */
				wlc_phy_radio20704_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20704_minipmu_cal(pi);

				/* Actual R_CAL
				 * Due to BCM6756-267 the r-cal may fail with small probability
				 * If this happens, retrying will very likely succeed.
				 * NOTE: the code in wlc_phy_radio20704_r_cal() already sets a
				 *        nominal value if it detects that r-cal has failed
				 */
				for (retry = 0; retry < 4; retry++) {
					if (!wlc_phy_radio20704_r_cal(pi, 2)) {
						break;
					} else {
						PHY_ERROR(("%s: R-Cal retry #%d.\n",
							__FUNCTION__, retry+1));
					}
				}

				/* RC CAL */
				wlc_phy_radio20704_rc_cal(pi);

				/* PMU PLL PWR-UP */
				wlc_phy_radio20704_pmu_pll_pwrup(pi);

				/* WAR's */
				wlc_phy_radio20704_wars(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {

				/* Global Radio PU's */
				wlc_phy_radio20707_pwron_seq_phyregs(pi);

				/* Xtal powerup */
				wlc_phy_radio20707_xtal_pwrup(pi);

				/* PU the TX/RX/VCO/..LDO's  */
#ifdef MINIPMU_RESOURCE_20707
				/* 20180625 AJG Don't need this, PMU Resource can take care of it */
				wlc_phy_radio20707_minipmu_pwron_seq(pi);
#endif

				/* R_CAL: apply static rcal value */
				wlc_phy_radio20707_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20707_minipmu_cal(pi);

				/* Actual R_CAL */
				wlc_phy_radio20707_r_cal(pi, 2);

				/* RC CAL */
				wlc_phy_radio20707_rc_cal(pi);

				/* PMU PLL PWR-UP */
				wlc_phy_radio20707_pmu_pll_pwrup(pi);

				/* WAR's */
				wlc_phy_radio20707_wars(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {

				/* Global Radio PU's */
				wlc_phy_radio20708_pwron_seq_phyregs(pi);

				/* PU the TX/RX/VCO/..LDO's  */
#ifdef MINIPMU_RESOURCE_20708
				/* 20180625 AJG Don't need this, PMU Resource can take care of it */
				wlc_phy_radio20708_minipmu_pwron_seq(pi);
#endif

				/* Xtal powerup */
				wlc_phy_radio20708_xtal_pwrup(pi);

				/* PMU PLL PWR-UP */
				wlc_phy_radio20708_pmu_pll_pwrup(pi);

				/* bypass afe cal in Veloce */
				if (!ISSIM_ENAB(pi->sh->sih)) {
					/* Perform MINI PMU CAL */
					wlc_phy_radio20708_minipmu_cal(pi);

					/* R_CAL: apply static rcal value */
					wlc_phy_radio20708_r_cal(pi, 1);

					/* Actual R_CAL
					 * Due to BCM6756-267 the r-cal may fail with small
					 * probability. If this happens, retrying will very
					 * likely succeed.
					 * NOTE: the code in wlc_phy_radio20704_r_cal() already
					 * sets a nominal value if it detects that r-cal has failed
					 */
					for (retry = 0; retry < 4; retry++) {
						if (!wlc_phy_radio20708_r_cal(pi, 2)) {
							break;
						} else {
							PHY_ERROR(("%s: R-Cal retry #%d.\n",
								__FUNCTION__, retry+1));
						}
					}

					/* RC CAL */
					wlc_phy_radio20708_rc_cal(pi);
				} else {
					PHY_ERROR(("bypass afe-cal in QT\n"));
				}

				/* WAR's */
				wlc_phy_radio20708_wars(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {

				/* Global Radio PU's */
				wlc_phy_radio20709_pwron_seq_phyregs(pi);

				/* Xtal powerup */
				wlc_phy_radio20709_xtal_pwrup(pi);

				/* PU the TX/RX/VCO/..LDO's  */
#ifdef MINIPMU_RESOURCE_20709
				/* 20180625 AJG Don't need this, PMU Resource can take care of it */
				wlc_phy_radio20709_minipmu_pwron_seq(pi);
#endif

				/* R_CAL: apply static rcal value */
				wlc_phy_radio20709_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20709_minipmu_cal(pi);

				/* Actual R_CAL
				 * Due to BCM6756-267 the r-cal may fail with small probability
				 * If this happens, retrying will very likely succeed.
				 * NOTE: the code in wlc_phy_radio20709_r_cal() already sets
				 *        a nominal value if it detects that r-cal has failed
				 */
				for (retry = 0; retry < 4; retry++) {
					if (!wlc_phy_radio20709_r_cal(pi, 2)) {
						break;
					} else {
						PHY_ERROR(("%s: R-Cal retry #%d.\n",
							__FUNCTION__, retry+1));
					}
				}

				/* RC CAL */
				wlc_phy_radio20709_rc_cal(pi);

				/* PMU PLL PWR-UP */
				wlc_phy_radio20709_pmu_pll_pwrup(pi);

				/* WAR's */
				wlc_phy_radio20709_wars(pi);
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
				/* 20710_procs.tcl r769742: 20710_init  */

				/* Global Radio PU's */
				wlc_phy_radio20710_pwron_seq_phyregs(pi);

				/* Xtal powerup */
				wlc_phy_radio20710_xtal_pwrup(pi);

				/* PU the TX/RX/VCO/..LDO's  */
#ifdef MINIPMU_RESOURCE_20710
				/* PMU Resource can take care of it */
				wlc_phy_radio20710_minipmu_pwron_seq(pi);
#endif

				/* R_CAL: apply static rcal value */
				wlc_phy_radio20710_r_cal(pi, 1);

				/* Perform MINI PMU CAL */
				wlc_phy_radio20710_minipmu_cal(pi);

				/* Actual R_CAL
				 * Due to BCM6756-267 the r-cal may fail with small probability
				 * If this happens, retrying will very likely succeed.
				 * NOTE: the code in wlc_phy_radio20710_r_cal() already sets
				 *       a nominal value if it detects that r-cal has failed
				 */
				for (retry = 0; retry < 4; retry++) {
					if (!wlc_phy_radio20710_r_cal(pi, 2)) {
						break;
					} else {
						PHY_ERROR(("%s: R-Cal retry #%d.\n",
							__FUNCTION__, retry+1));
					}
				}

				/* RC CAL */
				wlc_phy_radio20710_rc_cal(pi);

				/* PMU PLL PWR-UP */
				wlc_phy_radio20710_pmu_pll_pwrup(pi);

				/* WAR's */
				wlc_phy_radio20710_wars(pi);
			}

			if (!TINY_RADIO(pi) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) &&
				!(RADIOID_IS(pi->pubpi->radioid, BCM20710_ID))) {

			/* --------------------------RCAL WAR ---------------------------- */
			/* Currently RCAL resistor is not connected on the board. The pin  */
			/* labelled TSSI_G/GPIO goes into the TSSI pin of the FEM through  */
			/* a 0 Ohm resistor. There is an option to add a shunt 10k to GND  */
			/* on this trace but it is depop. Adding shunt resistance on the   */
			/* TSSI line may affect the voltage from the FEM to our TSSI input */
			/* So, this issue is worked around by forcing below registers      */
			/* THIS IS APPLICABLE FOR BOARDTYPE = $def(boardtype)              */
			/* --------------------------RCAL WAR ---------------------------- */

				if (BF3_RCAL_OTP_VAL_EN(pi_ac) == 1) {
					data = pi->sromi->rcal_otp_val;
				} else {
					if (BF3_RCAL_WAR(pi_ac) == 1) {
						if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
							data = ACPHY_RCAL_VAL_2X2;
						} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev)
						           == 1) {
							data = ACPHY_RCAL_VAL_1X1;
						}
					}
#ifndef ATE_BUILD
					else if (RADIO2069REV(pi->pubpi->radiorev) == 64) {
					   /* for 4364 3x3 hard coding for now */
					   MOD_RADIO_REG(pi, RF2, BG_CFG1, rcal_trim, 10);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_bg_rcal_trim, 1);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_otp_rcal_sel, 0);

					} else if (RADIO2069REV(pi->pubpi->radiorev) == 66) {
					   MOD_RADIO_REG(pi, RF2, BG_CFG1, rcal_trim, 7);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_bg_rcal_trim, 1);
					   MOD_RADIO_REG(pi, RF2, OVR2, ovr_otp_rcal_sel, 0);
					}
#endif
					else {
							wlc_phy_radio2069_rcal(pi);
					}
				}

				if (BF3_RCAL_WAR(pi_ac) == 1 ||
					BF3_RCAL_OTP_VAL_EN(pi_ac) == 1) {
					if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
						if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
							FOREACH_CORE(pi, core) {
								MOD_RADIO_REGC(pi, GE32_BG_CFG1,
									core, rcal_trim, data);
								MOD_RADIO_REGC(pi, GE32_OVR2, core,
									ovr_bg_rcal_trim, 1);
								MOD_RADIO_REGC(pi, GE32_OVR2, core,
									ovr_otp_rcal_sel, 0);
							}
						} else if (RADIO2069_MAJORREV
							(pi->pubpi->radiorev) == 1) {
							MOD_RADIO_REG(pi, RFP,  GE16_BG_CFG1,
								rcal_trim, data);
							MOD_RADIO_REG(pi, RFP,  GE16_OVR2,
								ovr_bg_rcal_trim, 1);
							MOD_RADIO_REG(pi, RFP,  GE16_OVR2,
								ovr_otp_rcal_sel, 0);
						}
					}
				}
			}

			if (TINY_RADIO(pi)) {
				if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
				    RADIOMAJORREV(pi) == 3) {
					wlc_phy_radio_tiny_rccal_wave2(pi_ac->radioi);
				} else {
					wlc_phy_radio_tiny_rccal(pi_ac->radioi);
				}
			} else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID) ||
			           RADIOID_IS(pi->pubpi->radioid, BCM20704_ID) ||
			           RADIOID_IS(pi->pubpi->radioid, BCM20707_ID) ||
			           RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) ||
			           RADIOID_IS(pi->pubpi->radioid, BCM20709_ID) ||
			           RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
				/* rccal already done above */
			} else {
				wlc_phy_radio2069_rccal(pi);
			}

			if (phy_ac_chanmgr_get_data(pi_ac->chanmgri)->init_done) {
				wlc_phy_set_regtbl_on_pwron_acphy(pi);
				wlc_phy_chanspec_set_acphy(pi, pi->radio_chanspec);
			}
			pi->radio_is_on = TRUE;
		}
	} else {
		/* wlc_phy_radio2069_off(); */
		pi->radio_is_on = FALSE;

		/* FEM */
		ACPHYREG_BCAST(pi, RfctrlIntc0, 0);

		if (!ACMAJORREV_4(pi->pubpi->phy_rev)) {
			ACPHY_REG_LIST_START
				/* AFE */
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreAfeCfg10, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreAfeCfg20, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x1fff)

				/* Radio RX */
				ACPHYREG_BCAST_ENTRY(pi, RfctrlCoreRxPus0, 0)
				ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0xffff)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Radio TX */
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);

		/* {radio, rfpll, pllldo}_pu = 0 */
		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0);

		/* Remove rfctrl_bundle_en to control rfpll_pu from chip_pu */
		if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, RfctrlCmd, rfctrl_bundle_en, 0);
		}

		if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
			if (RADIOMAJORREV(pi) != 3) {
				FOREACH_CORE(pi, core) {
					/* PD the RFPLL */
					data = READ_RADIO_REG_TINY(pi, PLL_CFG1, core);
					data = data & 0xE0FF;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PLL_CFG1, core), data);

					RADIO_REG_LIST_START
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_HVLDO1, core,
							ldo_2p5_pu_ldo_CP, 0)
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_HVLDO1, core,
							ldo_2p5_pu_ldo_VCO, 0)
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_CP1, core,
							rfpll_cp_pu, 0)
						/* Clear the VCO signal */
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_CFG2, core,
							rfpll_rst_n, 0)
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_VCOCAL13, core,
							rfpll_vcocal_rst_n, 0)
						MOD_RADIO_REG_TINY_ENTRY(pi, PLL_VCOCAL1, core,
							rfpll_vcocal_cal, 0)
						/* Power Down the mini PMU */
						MOD_RADIO_REG_TINY_ENTRY(pi, PMU_CFG4, core,
							wlpmu_ADCldo_pu, 0)
					RADIO_REG_LIST_EXECUTE(pi, core);

					data = READ_RADIO_REG_TINY(pi, PMU_OP, core);
					data = data & 0xFF61;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PMU_OP, core), data);
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_pu, 0);
					MOD_RADIO_REG_TINY(pi, VREG_CFG, core, vreg25_pu, 0);
				}
			} else {
				for (pll = 0; pll < 2; pll++) {
					/* PD the RFPLL */
					data = READ_RADIO_PLLREG_20693(pi, PLL_CFG1, pll);
					data = data & 0xE0FF;
					phy_utils_write_radioreg(pi, RADIO_PLLREG_20693(pi,
						PLL_CFG1, pll), data);
					MOD_RADIO_PLLREG_20693(pi, PLL_HVLDO1, pll,
						ldo_2p5_pu_ldo_CP, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_HVLDO1, pll,
						ldo_2p5_pu_ldo_VCO, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_CP1, pll, rfpll_cp_pu, 0);
					/* Clear the VCO signal */
					MOD_RADIO_PLLREG_20693(pi, PLL_CFG2, pll, rfpll_rst_n, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_VCOCAL13, pll,
						rfpll_vcocal_rst_n, 0);
					MOD_RADIO_PLLREG_20693(pi, PLL_VCOCAL1, pll,
						rfpll_vcocal_cal, 0);
				}
				FOREACH_CORE(pi, core) {
					/* Power Down the mini PMU */
					MOD_RADIO_REG_20693(pi, PMU_CFG4, core, wlpmu_ADCldo_pu, 0);
					data = READ_RADIO_REG_20693(pi, PMU_OP, core);
					data = data & 0xFF61;
					phy_utils_write_radioreg(pi, RADIO_REG_20693(pi,
						PMU_OP, core), data);
					MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_pu, 0);
					MOD_RADIO_REG_20693(pi, VREG_CFG, core, vreg25_pu, 0);
				}
			}
			if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
				((phy_get_current_core(pi) == PHY_RSBD_PI_IDX_CORE1))) {
				/* Powerdown xtal ldo for MAC_Core = 1 in case of RSDB mode */
				MOD_RADIO_REG_TINY(pi, PLL_XTALLDO1, 0,
					ldo_1p2_xtalldo1p2_BG_pu, 0);
			} else if ((phy_get_phymode(pi) != PHYMODE_RSDB)) {
				/* Powerdown xtal ldo for Core 1 in case of MIMO and 80+80 */
				MOD_RADIO_REG_TINY(pi, PLL_XTALLDO1, 1,
					ldo_1p2_xtalldo1p2_BG_pu, 0);
			}
		}

		/* 4364 dual phy. when 3x3 is powered down, make sure the 1x1 femctrl lines */
		/* do not toggle ant0/ant1 this is done by giving bt the control on ant0/ant1 */
		/* If bt is not there (bt_reg_on 0) this is fine. Ultimately bt firmware needs */
		/* to write proper values in the bt_clb_upi registers bt2clb_swctrl_smask_bt_antX */
		if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID) && IS_4364_3x3(pi)) {
			si_gci_set_femctrl_mask_ant01(pi->sh->sih, pi->sh->osh, 0);
		}

		/* Turn off the mini PMU enable on all cores when going down.
		 * Will be powered up in the UP sequence
		 */
		if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)) {
			if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
				FOREACH_CORE(pi, core)
				        MOD_RADIO_REGC(pi, GE32_PMU_OP, core, wlpmu_en, 0);
			}
		}

		if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			/* Powering down PLL LDO and also putting RFPLL in reset */
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x1);
			WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x5);
		} else {
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x0);
			WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0x1);
		}

		/* Ref JIRA64559 for more details. Summary of the issue - In "wl down" path */
		/* AFE_CLK_DIV block was not being turned off, but the */
		/* preceding blocks - RF_LDO and RF_PLL were turned off. So, the output of RF_PLL */
		/* is in undefined state and with that as input, the output of AFE_CLK_DIV block */
		/* would also be in undefined state. The output clock of this block was causing */
		/* BT sensitivity degradation */
		if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_sel_div_ovr, 1);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_sel_div, 0x0);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_en_ovr, 1);
			MOD_PHYREG(pi, AfeClkDivOverrideCtrl, afediv_en, 0x0);
		}

		/* These register turn on AFE_CLK_DIV block in "wl down" path. Leaving the block
		 * turned on was causing BT BER issue. RB: 46565.
		 */
		if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0xf);
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) {
			ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0x1);
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x408);
		} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
			ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0x1);
			WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x8);
		}
	}

	if (BCM4350_CHIP(pi->sh->chip) &&
	    !CST4350_CHIPMODE_HSIC20D(pi->sh->sih->chipst)) {
		/* Power down HSIC */
		ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
			MOD_RADIO_REG(pi, RFP,  GE16_PLL_XTAL2, xtal_pu_HSIC, 0x0);
		}
	}

#ifdef LOW_TX_CURRENT_SETTINGS_2G
/* TODO: iPa low power in 2G - check if condition is needed) */
	if ((CHSPEC_IS2G(pi->radio_chanspec) && (pi->sromi->extpagain2g == 2)) ||
		(CHSPEC_ISPHY5G6G(pi->radio_chanspec) && (pi->sromi->extpagain5g == 2))) {
		is_ipa = 1;
	} else {
		is_ipa = 0;
	}

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) && (is_ipa == 0)) {
		PHY_TRACE(("Modifying PA Bias settings for lower power for 2G!\n"));
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_main, 0x36);
		MOD_RADIO_REG(pi, RFX, PA2G_IDAC2, pa2g_biasa_aux, 0x36);
	}

#endif /* LOW_TX_CURRENT_SETTINGS_2G */
}

static void
wlc_phy_radio2069_mini_pwron_seq_rev16(phy_info_t *pi)
{
	uint8 cntr = 0;

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, wlpmu_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, VCOldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, TXldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, AFEldo_pu, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, RXldo_pu, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	OSL_DELAY(100);

	ACPHY_REG_LIST_START
		/* WAR for XTAL power up issues */
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_xtal_pu_corebuf_pfd, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_pfddrv, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_BT, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_caldrv, 1)

		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, synth_pwrsw_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, wlpmu_ldobg_clk_en, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PMU_OP, ldoref_start_cal, 1)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (!ISSIM_ENAB(pi->sh->sih)) {
		while (READ_RADIO_REGFLD(pi, RFP, GE16_PMU_STAT, ldobg_cal_done) == 0) {
			OSL_DELAY(100);
			cntr++;
			if (cntr > 100) {
				PHY_ERROR(("PMU cal Fail \n"));
				break;
			}
		}
	}

	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, ldoref_start_cal, 0);
	MOD_RADIO_REG(pi, RFP, GE16_PMU_OP, wlpmu_ldobg_clk_en, 0);
}

static void
wlc_phy_radio2069_mini_pwron_seq_rev32(phy_info_t *pi)
{

	uint8 cntr = 0;
	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0x9e);

	OSL_DELAY(100);

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0xbe)

		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0x20be)
		WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_PMU_OP, 0x60be)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (!ISSIM_ENAB(pi->sh->sih)) {
		while (READ_RADIO_REGFLD(pi, RF0, GE32_PMU_STAT, ldobg_cal_done) == 0 ||
		       READ_RADIO_REGFLD(pi, RF1, GE32_PMU_STAT, ldobg_cal_done) == 0) {

			OSL_DELAY(100);
			cntr++;
			if (cntr > 100) {
				PHY_ERROR(("PMU cal Fail ...222\n"));
				break;
			}
		}
	}
	phy_utils_write_radioreg(pi, RFX_2069_GE32_PMU_OP, 0xbe);
}

static void
wlc_phy_radio2069_pwron_seq(phy_info_t *pi)
{
	/* Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu
	   So, to make rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1
	*/
	uint16 txovrd = READ_PHYREG(pi, RfctrlCoreTxPus0);
	uint16 rfctrlcmd = READ_PHYREG(pi, RfctrlCmd) & 0xfc38;

	ACPHY_REG_LIST_START
		/* Using usleep of 100us below, so don't need these */
		WRITE_PHYREG_ENTRY(pi, Pllldo_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Rfpll_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Logen_AfeDiv_reset, 0x2000)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd & 0x7e7f);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, READ_PHYREG(pi, RfctrlOverrideTxPus0) | 0x180);

	/* ***  Start Radio rfpll pwron seq  ***
	   Start with chip_pu = 0, por_reset = 0, rfctrl_bundle_en = 0
	*/
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd);

	/* Toggle jtag reset (not required for uCode PM) */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 1);
	OSL_DELAY(1);
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 0);

	/* Update preferred values (not required for uCode PM) */
	wlc_phy_radio2069_upd_prfd_values(pi);

	/* Toggle radio_reset (while radio_pu = 1) */
	MOD_RADIO_REG(pi, RF2, VREG_CFG, bg_filter_en, 0);   /* radio_reset = 1 */
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 6);   /* radio_pwrup = 1, rfpll_pu = 0 */
	OSL_DELAY(100);                                      /* radio_reset to be high for 100us */
	MOD_RADIO_REG(pi, RF2, VREG_CFG, bg_filter_en, 1);   /* radio_reset = 0 */

	/* {rfpll, pllldo, logen}_{pu, reset} pwron seq */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xd);
	WRITE_PHYREG(pi, RfctrlCmd, rfctrlcmd | 2);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd | 0x180);
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, txovrd & 0xfeff);
}

static void
wlc_phy_tiny_radio_pwron_seq(phy_info_t *pi)
{
	ASSERT(TINY_RADIO(pi));
	/* Note: if RfctrlCmd.rfctrl_bundle_en = 0, then rfpll_pu = radio_pu
	   So, to make rfpll_pu = 0 & radio_pwrup = 1, make RfctrlCmd.rfctrl_bundle_en = 1
	*/

	/* # power down everything */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x3ff);
	} else {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0)
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, 0x3ff)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	ACPHY_REG_LIST_START
		/* Using usleep of 100us below, so don't need these */
		WRITE_PHYREG_ENTRY(pi, Pllldo_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Rfpll_resetCtrl, 0)
		WRITE_PHYREG_ENTRY(pi, Logen_AfeDiv_reset, 0x2000)

		/* Start with everything off: {radio, rfpll, plldlo, logen}_{pu, reset} = 0 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0xd)

		/* # Reset radio, jtag */
		/* NOTE: por_force doesnt have any effect in 4349A0 and */
		/* radio jtag reset is taken care by the PMU */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x7)
	ACPHY_REG_LIST_EXECUTE(pi);
	/* # radio_reset to be high for 100us */
	OSL_DELAY(100);
	WRITE_PHYREG(pi, RfctrlCmd, 0x6);

	/* Update preferred values (not required for uCode PM) */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		wlc_phy_radio20693_upd_prfd_values(pi);
	}

	ACPHY_REG_LIST_START
		/* # {rfpll, pllldo, logen}_{pu, reset} */
		/* # pllldo_{pu, reset} = 1, rfpll_reset = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0xd)
		/* # {radio, rfpll}_pwrup = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x2)

		/* # logen_{pwrup, reset} = 1 */
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0x180)
	ACPHY_REG_LIST_EXECUTE(pi);
		OSL_DELAY(100); /* # resets to be on for 100us */
	/* # {pllldo, rfpll}_reset = 0 */
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
		/* # logen_reset = 0 */
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
		ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);
	} else {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, 0x80)
			/* # leave overrides for logen_{pwrup, reset} */
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, 0x180)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
}

#define MAX_2069_RCAL_WAITLOOPS 100
/* rcal takes ~50us */
static void
wlc_phy_radio2069_rcal(phy_info_t *pi)
{
	uint8 done, rcal_val, core;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ACPHY_REG_LIST_START
		/* Power-up rcal clock (need both of them for rcal) */
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 1)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1)

		/* Rcal can run with 40mhz cls, no diving */
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL1, 0)
	ACPHY_REG_LIST_EXECUTE(pi);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Make connection with the external 10k resistor */
	/* Turn off all test points in cgpaio block to avoid conflict */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 1);
		}
		ACPHY_REG_LIST_START
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG3, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFX_2069_GE32_CGPAIO_CFG5, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE1, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_TOP_SPARE6, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_ENTRY(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 1)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG2, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG3, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG4, 0)
			WRITE_RADIO_REG_ENTRY(pi, RF2_2069_CGPAIO_CFG5, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	/* NOTE: xtal_pu, xtal_buf_pu & xtalldo_pu direct control lines should be(& are) ON */

	/* Toggle the rcal pu for calibration engine */
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 0);
	OSL_DELAY(1);
	MOD_RADIO_REG(pi, RF2, RCAL_CFG, pu, 1);

	/* Wait for rcal to be done, max = 10us * 100 = 1ms  */
	done = 0;

	SPINWAIT(READ_RADIO_REGFLD(pi, RF2, RCAL_CFG,
		i_wrf_jtag_rcal_valid) == 0, ACPHY_SPINWAIT_RCAL_STATUS);

	done = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_valid);

	if (done == 0) {
		PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR : RCAL Failed", __FUNCTION__));
		PHY_FATAL_ERROR(pi, PHY_RC_RCAL_INVALID);
	}

	/* Status */
	rcal_val = READ_RADIO_REGFLD(pi, RF2, RCAL_CFG, i_wrf_jtag_rcal_value);
	rcal_val = rcal_val >> 1;
	PHY_INFORM(("wl%d: %s rcal=%d\n", pi->sh->unit, __FUNCTION__, rcal_val));
#ifdef ATE_BUILD
	/* Same RCAL value for all 3 cores, hence for logging forcing core=0 */
	wl_ate_set_buffer_regval(RCAL_VALUE, rcal_val,
			0, phy_get_current_core(pi), pi->sh->chip);
#endif

	/* Valid range of values for rcal */
	ASSERT((rcal_val > 0) && (rcal_val < 15));

	/*  Power down blocks not needed anymore */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_CGPAIO_CFG1, core, cgpaio_pu, 0);
		}
	} else {
		MOD_RADIO_REG(pi, RF2, CGPAIO_CFG1, cgpaio_pu, 0);
	}

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0)
		MOD_RADIO_REG_ENTRY(pi, RF2, RCAL_CFG, pu, 0)
	ACPHY_REG_LIST_EXECUTE(pi);
}

/* rccal takes ~3ms per, i.e. ~9ms total */
static void
wlc_phy_radio2069_rccal(phy_info_t *pi)
{
	uint8 cal, core, rccal_val[NUM_2069_RCCAL_CAPS];
	uint16 n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x0, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x40};
	uint16 trc[] = {0x14a, 0x101, 0x11a};
	uint16 gmult_const = 193;

	phy_ac_radio_data_t *data = pi->u.pi_acphy->radioi->data;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
		if PHY_XTAL_IS40M(pi) {
		  if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
		                      (RADIO2069REV(pi->pubpi->radiorev) == 26)) {
			gmult_const = 70;
			trc[0] = 0x294;
			trc[1] = 0x202;
			trc[2] = 0x214;
		  } else {
			gmult_const = 160;
		  }
		} else if (PHY_XTAL_IS37M4(pi)) {
		  if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
		                      (RADIO2069REV(pi->pubpi->radiorev) == 26)) {
			gmult_const = 77;
			trc[0] = 0x45a;
			trc[1] = 0x1e0;
			trc[2] = 0x214;
		  } else {
			gmult_const = 158;
			trc[0] = 0x22d;
			trc[1] = 0xf0;
			trc[2] = 0x10a;
		  }
		} else if (PHY_XTAL_IS52M(pi)) {
			if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
				(RADIO2069REV(pi->pubpi->radiorev) == 26)) {
				gmult_const = 77;
				trc[0] = 0x294;
				trc[1] = 0x202;
				trc[2] = 0x214;
			  } else {
				gmult_const = 160;
				trc[0] = 0x14a;
				trc[1] = 0x101;
				trc[2] = 0x11a;
			  }
		} else {
			gmult_const = 160;
		}

	} else {
		gmult_const = 193;
		if ((RADIO2069REV(pi->pubpi->radiorev) == 64) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 66)) {
			gmult_const = 216;
			trc[0] = 0x22d;
			trc[1] = 0xf0;
			trc[2] = 0x10a;
		}
	}

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);
	MOD_RADIO_REG(pi, RFP, PLL_XTAL5, xtal_sel_RCCAL, 2);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, sr, sr[cal]);
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, sc, sc[cal]);
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_X1, x1[cal]);
		phy_utils_write_radioreg(pi, RF2_2069_RCCAL_TRC, trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REGC(pi, DAC_CFG2, core, DACbuf_fixed_cap, 0);
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
					MOD_RADIO_REGC(pi, GE16_OVR22, core,
					               ovr_afe_DACbuf_fixed_cap, 1);
				} else {
					MOD_RADIO_REGC(pi, OVR21, core,
					               ovr_afe_DACbuf_fixed_cap, 1);
				}
			}
		}

		/* Toggle RCCAL power */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_START, 1);

		/* This delay is required before reading the RCCAL_LOGIC2
		*   without this delay, the cal values are incorrect.
		*/
		OSL_DELAY(100);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		SPINWAIT(READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC2, rccal_DONE) == 0,
				MAX_2069_RCCAL_WAITLOOPS);
		if (READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC2, rccal_DONE) == 0) {
			PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR : RCCAL invalid \n",
				__FUNCTION__));
			PHY_FATAL_ERROR(pi, PHY_RC_RCCAL_INVALID);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG(pi, RF2, RCCAL_LOGIC1, rccal_START, 0);

		if (cal == 0) {
			/* lpf */
			n0 = READ_RADIO_REG(pi, RF2, RCCAL_LOGIC3);
			n1 = READ_RADIO_REG(pi, RF2, RCCAL_LOGIC4);
			/* gmult = (30/40) * (n1-n0) = (193 * (n1-n0)) >> 8 */
			rccal_val[cal] = (gmult_const * (n1 - n0)) >> 8;
			data->rccal_gmult = rccal_val[cal];
			data->rccal_gmult_rc = data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_LPF, data->rccal_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */
		} else if (cal == 1) {
			/* adc */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_adc1p2);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_ADC, rccal_val[cal], -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */

			/* don't change this loop to active core loop,
			   gives slightly higher floor, why?
			*/
			FOREACH_CORE(pi, core) {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0, rccal_val[cal]);
				MOD_RADIO_REGC(pi, TIA_CFG3, core, rccal_hpc, rccal_val[cal]);
			}
			/* Store value, might be overriden upon channel change */
			pi->u.pi_acphy->radioi->rccal_adc_rc = rccal_val[cal];
		} else {
			/* dacbuf */
			rccal_val[cal] = READ_RADIO_REGFLD(pi, RF2, RCCAL_LOGIC5, rccal_raw_dacbuf);
			data->rccal_dacbuf = rccal_val[cal];

			/* take away the override on dacbuf fixed cap */
			FOREACH_CORE(pi, core) {
				if (RADIO2069_MAJORREV(pi->pubpi->radiorev) > 0) {
					MOD_RADIO_REGC(pi, GE16_OVR22, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				} else {
					MOD_RADIO_REGC(pi, OVR21, core,
					               ovr_afe_DACbuf_fixed_cap, 0);
				}
			}
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, rccal_val[cal]));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(RCCAL_DACBUF, data->rccal_dacbuf, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif /* ATE_BUILD */
		}

		/* Turn off rccal */
		MOD_RADIO_REG(pi, RF2, RCCAL_CFG, pu, 0);
	}

	/* Powerdown rccal driver */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
}

static void
wlc_phy_radio2069_upd_prfd_values(phy_info_t *pi)
{
	uint8 core;
	const radio_20xx_prefregs_t *prefregs_2069_ptr = NULL;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	switch (RADIO2069REV(pi->pubpi->radiorev)) {
	case 3:
		prefregs_2069_ptr = prefregs_2069_rev3;
		break;
	case 4:
	case 8:
		prefregs_2069_ptr = prefregs_2069_rev4;
		break;
	case 7:
		prefregs_2069_ptr = prefregs_2069_rev4;
		break;
	case 16:
		prefregs_2069_ptr = prefregs_2069_rev16;
		break;
	case 17:
		prefregs_2069_ptr = prefregs_2069_rev17;
		break;
	case 18:
		prefregs_2069_ptr = prefregs_2069_rev18;
		break;
	case 23:
		prefregs_2069_ptr = prefregs_2069_rev23;
		break;
	case 24:
		prefregs_2069_ptr = prefregs_2069_rev24;
		break;
	case 25:
		prefregs_2069_ptr = prefregs_2069_rev25;
		break;
	case 26:
		prefregs_2069_ptr = prefregs_2069_rev26;
		break;
	case 32:
	case 33:
	case 34:
	case 35:
	case 37:
	case 38:
		prefregs_2069_ptr = prefregs_2069_rev33_37;
		break;
	case 39:
	case 40:
	case 44:
		prefregs_2069_ptr = prefregs_2069_rev39;
		break;
	case 36:
		prefregs_2069_ptr = prefregs_2069_rev36;
		break;
	case 64:
	case 66:
		prefregs_2069_ptr = prefregs_2069_rev64;
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
		ASSERT(0);
		return;
	}

	/* Update preferred values */
	wlc_phy_init_radio_prefregs_allbands(pi, prefregs_2069_ptr);

	if (BOARDFLAGS(GENERIC_PHY_INFO(pi)->boardflags) &
		BFL_SROM11_WLAN_BT_SH_XTL) {
		MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_BT, 1);
	}

	/* **** NOTE : Move the following to XLS (whenever possible) *** */

	/* Reg conflict with 2069 rev 16 */
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) {
		ACPHY_REG_LIST_START
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_rst_n, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_en_vcocal, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR16, ovr_rfpll_vcocal_rstn, 1)

			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_cal_rst_n, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_pll_pu, 1)
			MOD_RADIO_REG_ENTRY(pi, RFP, OVR15, ovr_rfpll_vcocal_cal, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	} else {
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			ACPHY_REG_LIST_START
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_en_vcocal, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR17, ovr_rfpll_vcocal_rstn, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_rst_n, 1)

				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_cal_rst_n, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_pll_pu, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR16, ovr_rfpll_vcocal_cal, 1)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Until this is moved to XLS (give bg_filter_en control to radio) */
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			MOD_RADIO_REG(pi, RFP, GE32_OVR1, ovr_vreg_bg_filter_en, 1);
		}

		/* Ensure that we read the values that are actually applied */
		/* to the radio block and not just the radio register values */

		phy_utils_write_radioreg(pi, RFX_2069_GE16_READOVERRIDES, 1);
		MOD_RADIO_REG(pi, RFP, GE16_READOVERRIDES, read_overrides, 1);

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			/* For VREG power up in radio jtag as there is a bug in the digital
			 * connection
			 */
			if (RADIO2069_MINORREV(pi->pubpi->radiorev) < 5) {
				MOD_RADIO_REG(pi, RF2, VREG_CFG, pup, 1);
				MOD_RADIO_REG(pi, RFP, GE32_OVR1, ovr_vreg_pup, 1);
			}

			/* This OVR enable is required to change the value of
			 * reg(RFP_pll_xtal4.xtal_outbufstrg) and used the value from the
			 * jtag. Otherwise the direct control from the chip has a fixed
			 * non-programmable value!
			 */
			MOD_RADIO_REG(pi, RFP, GE16_OVR27, ovr_xtal_outbufstrg, 1);
		}
	}

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) < 2) {
		MOD_RADIO_REG(pi, RF2, BG_CFG1, bg_pulse, 1);
	}
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, GE32_BG_CFG1, core, bg_pulse, 1);
			MOD_RADIO_REGC(pi, GE32_OVR2, core, ovr_bg_pulse, 1);
		}
	}

	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
			(((RADIO2069_MINORREV(pi->pubpi->radiorev)) == 16) ||
			(RADIO2069_MINORREV(pi->pubpi->radiorev) == 17))) {
			/* 4364 3x3 : to address XTAL spur on core2 */
				MOD_RADIO_REG(pi, RFP, PLL_XTAL8, xtal_repeater3_size, 4);
	}

	/* Give control of bg_filter_en to radio */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) < 2)
		MOD_RADIO_REG(pi, RF2, OVR2, ovr_vreg_bg_filter_en, 1);

	FOREACH_CORE(pi, core) {
		/* Fang's recommended settings */
		MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_9_8, 1);
		MOD_RADIO_REGC(pi, ADC_RC2, core, adc_ctrl_RC_17_16, 2);

		/* These should be 0, as they are controlled via direct control lines
		   If they are 1, then during 5g, they will turn on
		*/
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_cas_pu, 0);
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_2gtx_pu, 0);
		MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_pu, 0);
		MOD_RADIO_REGC(pi, PAD2G_CFG1, core, pad2g_pu, 0);
	}
	/* SWWLAN-39535  LNA1 clamping issue for 4360b1 and 43602a0 */
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 0) &&
		((RADIO2069REV(pi->pubpi->radiorev) == 7) ||
		(RADIO2069REV(pi->pubpi->radiorev) == 8) ||
		(RADIO2069REV(pi->pubpi->radiorev) == 13))) {
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, RXRF5G_CFG1, core, pu_pulse, 1);
			MOD_RADIO_REGC(pi, OVR18, core, ovr_rxrf5g_pu_pulse, 1);
		}
	}
}

static void
wlc_2069_rfpll_150khz(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF4, rfpll_lf_lf_r1, 0)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF4, rfpll_lf_lf_r2, 2)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 2)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF7, rfpll_lf_lf_rs_cm, 2)
		MOD_RADIO_REG_ENTRY(pi, RFP, PLL_LF7, rfpll_lf_lf_rf_cm, 0xff)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xffff)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xffff)
	ACPHY_REG_LIST_EXECUTE(pi);
}

static void
wlc_phy_2069_4335_set_ovrds(phy_info_t *pi)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR30, 0x1df3)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR31, 0x1ffc)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR32, 0x0078)
	ACPHY_REG_LIST_EXECUTE(pi);

	if (CHSPEC_IS2G(pi->radio_chanspec)) {
		phy_utils_write_radioreg(pi, RF0_2069_GE16_OVR28, 0x0);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0x0);
	} else {
		phy_utils_write_radioreg(pi, RF0_2069_GE16_OVR28, 0xffff);
		phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0xffff);
		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1)
			if (PHY_IPA(pi))
			    phy_utils_write_radioreg(pi, RFP_2069_GE16_OVR29, 0x6900);
	}
}

static void
wlc_phy_2069_4350_set_ovrds(phy_ac_radio_info_t *ri)
{
	uint8 core, afediv_size, afeldo1;
	phy_info_t *pi = ri->pi;
	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(pi->radio_chanspec),
		CHSPEC_IS2G(pi->radio_chanspec) ? WF_CHAN_FACTOR_2_4_G
		: WF_CHAN_FACTOR_5_G);
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	ACPHY_REG_LIST_START
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR30, 0x1df3)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR31, 0x1ffc)
		WRITE_RADIO_REG_ENTRY(pi, RFP_2069_GE16_OVR32, 0x0078)

		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_CP, 0x1)
		MOD_RADIO_REG_ENTRY(pi, RFP, GE16_PLL_HVLDO4, ldo_2p5_static_load_VCO, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);
	if (PHY_IPA(pi)&&(pi->xtalfreq == 37400000)&&CHSPEC_IS2G(pi->radio_chanspec)) {
		FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
			MOD_RADIO_REGC(pi, PA2G_CFG1, core, pa2g_bias_reset, 1);
			MOD_RADIO_REGC(pi, GE16_OVR13, core, ovr_pa2g_bias_reset, 1);
			}
	}
	if ((RADIOREV(pi->pubpi->radiorev) == 36) || (RADIOREV(pi->pubpi->radiorev) >= 39)) {
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, radio_logen2g, idac_qb, 0x2)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_itx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_irx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2gN5g, idac_qrx, 0x3)
			MOD_PHYREG_ENTRY(pi, radio_logen2g, idac_qtx, 0x3)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CFG4, rfpll_spare2, 0x6)
			MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CFG4, rfpll_spare3, 0x34)

			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_TOP_SPARE7, 0x1)
			WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF1, 0x48)
		ACPHY_REG_LIST_EXECUTE(pi);
	}
	if (!PHY_IPA(pi)) {
		if (pi->xtalfreq == 37400000) {
			MOD_RADIO_REG(pi, RFP, PLL_XTALLDO1,
				ldo_1p2_xtalldo1p2_ctl, 0xb);
		}
		if (CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
			if (CHSPEC_IS80(pi->radio_chanspec)) {
				switch (fc) {
				case 5690:
					afediv_size = 0xf;
					afeldo1 = 0x7;
					break;
				case 5775:
					afediv_size = 0xf;
					afeldo1 = 0x0;
					break;
				case 5210:
				case 5290:
					afediv_size = 0x8;
					afeldo1 = 0x0;
					break;
				default:
					afediv_size = 0xB;
					afeldo1 = 0x7;
				}
				MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1,
				    afediv_main_driver_size, afediv_size);
				MOD_RADIO_REG(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, afeldo1);
			} else {
				MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1, afediv_main_driver_size,
				     (fc == 5190) ? 0xa : 0x8);
				MOD_RADIO_REG(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, 0x7);
			}
			/* Override PAD gain for core 0 to be 255 */
			MOD_RADIO_REG(pi, RF0, GE16_OVR14, ovr_pad5g_gc, 0x1);
			MOD_RADIO_REG(pi, RF0, PAD5G_CFG1, gc, 0x7F);
			if (ri->data->srom_txnospurmod5g == 1) {
				/* Override PAD gain for core 1 to be 255 */
				MOD_RADIO_REG(pi, RF1, GE16_OVR14, ovr_pad5g_gc, 0x1);
				MOD_RADIO_REG(pi, RF1, PAD5G_CFG1, gc, 0x7F);
			} else {
				RADIO_REG_LIST_START
					MOD_RADIO_REG_ENTRY(pi, RF1, PAD5G_IDAC, idac_main, 0x28)
					MOD_RADIO_REG_ENTRY(pi, RF1, PAD5G_TUNE, idac_aux, 0x28)
					MOD_RADIO_REGC_ENTRY(pi, PAD5G_INCAP, 1,
						idac_incap_compen_main, 0x8)
					MOD_RADIO_REGC_ENTRY(pi, PAD5G_INCAP, 1,
						idac_incap_compen_aux, 0x8)
				RADIO_REG_LIST_EXECUTE(pi, 1);
			}
			if (pi->fabid == 4) {
				/* UMC tuning for 5G EVM */
				FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
					MOD_RADIO_REGC(pi, PAD5G_IDAC, core, idac_main, 0x2);
					MOD_RADIO_REGC(pi, PAD5G_TUNE, core, idac_aux, 0x2a);
					MOD_RADIO_REGC(pi, PAD5G_INCAP, core,
						idac_incap_compen_main, 5);
					MOD_RADIO_REGC(pi, PAD5G_INCAP, core,
						idac_incap_compen_aux, 5);
					MOD_RADIO_REGC(pi, GE16_OVR14, core,
						ovr_pga5g_gainboost, 1);
					MOD_RADIO_REGC(pi, PGA5G_CFG1, core, gainboost, 0xf);
				}
			}
		} else {
			ACPHY_REG_LIST_START
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_AFEDIV1,
					afediv_main_driver_size, 0x8)
				MOD_RADIO_REG_ENTRY(pi, RF1, GE32_PMU_CFG2, AFEldo_adj, 0x0)
			ACPHY_REG_LIST_EXECUTE(pi);
			if ((ri->data->srom_txnospurmod2g == 0) && (pi->xtalfreq == 37400000)) {
				if (fc == 2412) {
				    ACPHY_REG_LIST_START
					/* setting 200khz loopbw */
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xFFD4)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xF3F9)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xA)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xA)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xC65)
				    ACPHY_REG_LIST_EXECUTE(pi);
				}
				if (fc == 2467) {
				    ACPHY_REG_LIST_START
					/* setting 200khz loopbw */
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xFDCF)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xEDF3)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xC68)
				    ACPHY_REG_LIST_EXECUTE(pi);
				}
			}
		}
	} else if ((PHY_IPA(pi)) && (CHSPEC_IS80(pi->radio_chanspec))) {
		MOD_RADIO_REG(pi, RFP, GE16_AFEDIV1,
		    afediv_main_driver_size, 0xb);
	}
	/* set xtal_pu_RCCAL1 to 0 by default to avoid it staying high without rcal */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL1, 0);
}

static void
wlc_phy_chanspec_radio2069_setup(phy_info_t *pi, const void *chan_info, uint8 toggle_logen_reset)
{
	uint8 core;
	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(pi->radio_chanspec),
	        CHSPEC_IS2G(pi->radio_chanspec) ? WF_CHAN_FACTOR_2_4_G
	        : WF_CHAN_FACTOR_5_G);
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));
	ASSERT(chan_info != NULL);

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* logen_reset needs to be toggled whenever bandsel bit if changed */
	/* On a bw change, phy_reset is issued which causes currentBand getting reset to 0 */
	/* So, issue this on both band & bw change */
	if (toggle_logen_reset == 1) {
		wlc_phy_logen_reset(pi, 0);
	}

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		const chan_info_radio2069revGE32_t *ciGE32 = chan_info;

		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ciGE32->RFP_pll_vcocal5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ciGE32->RFP_pll_vcocal6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ciGE32->RFP_pll_vcocal2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ciGE32->RFP_pll_vcocal1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11, ciGE32->RFP_pll_vcocal11);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12, ciGE32->RFP_pll_vcocal12);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ciGE32->RFP_pll_frct2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ciGE32->RFP_pll_frct3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10, ciGE32->RFP_pll_vcocal10);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ciGE32->RFP_pll_xtal3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ciGE32->RFP_pll_vco2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1, ciGE32->RFP_logen5g_cfg1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ciGE32->RFP_pll_vco8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ciGE32->RFP_pll_vco6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ciGE32->RFP_pll_vco3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1, ciGE32->RFP_pll_xtalldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ciGE32->RFP_pll_hvldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ciGE32->RFP_pll_hvldo2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ciGE32->RFP_pll_vco5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ciGE32->RFP_pll_vco4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE32->RFP_pll_lf4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE32->RFP_pll_lf5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE32->RFP_pll_lf7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE32->RFP_pll_lf2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE32->RFP_pll_lf3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE32->RFP_pll_cp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE32->RFP_pll_lf6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL4, ciGE32->RFP_pll_xtal4);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE, ciGE32->RFP_logen2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_LNA2G_TUNE, ciGE32->RFX_lna2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX2G_CFG1, ciGE32->RFX_txmix2g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA2G_CFG2, ciGE32->RFX_pga2g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD2G_TUNE, ciGE32->RFX_pad2g_tune);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1, ciGE32->RFP_logen5g_tune1);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2, ciGE32->RFP_logen5g_tune2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_IDAC1, ciGE32->RFP_logen5g_idac1);
		phy_utils_write_radioreg(pi, RFX_2069_LNA5G_TUNE, ciGE32->RFX_lna5g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX5G_CFG1, ciGE32->RFX_txmix5g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA5G_CFG2, ciGE32->RFX_pga5g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD5G_TUNE, ciGE32->RFX_pad5g_tune);
		if ((RADIO2069_MINORREV(pi->pubpi->radiorev) == 4) &&
		    pi->sh->chippkg == 2 && PHY_XTAL_IS37M4(pi)) {
			if (fc == 5290) {
				ACPHY_REG_LIST_START
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL5, 0X5)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL6, 0x1C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL2, 0xA09)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL1, 0xF89)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL11, 0xD4)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL12, 0x2A70)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT2, 0x350)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT3, 0xA9C1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL10, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL3, 0x488)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO2, 0xCE8)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_CFG1, 0x40)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO8, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO6, 0x1D6f)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO3, 0x1F00)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTALLDO1, 0x780)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO1, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO2, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO5, 0x49C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO4, 0x3504)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xD6F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xECBE)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xDDE2)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF6, 0x1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL4, 0x36FF)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_TUNE1, 0x80)
				ACPHY_REG_LIST_EXECUTE(pi);
			} else if (fc == 5180) {
				ACPHY_REG_LIST_START
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL5, 0x5)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL6, 0x1C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL2, 0xA09)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL1, 0xF37)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL11, 0xCF)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL12, 0xC106)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT2, 0x33F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_FRCT3, 0x41B)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCOCAL10, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL3, 0x488)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO2, 0xCE8)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_CFG1, 0x40)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO8, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO6, 0x1D6F)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO3, 0x1F00)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTALLDO1, 0x780)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO1, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_HVLDO2, 0x0)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO5, 0x49C)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_VCO4, 0x3505)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF4, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF5, 0xB)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF7, 0xD6D)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF2, 0xF1C3)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF3, 0xE2E7)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_CP4, 0xBC28)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_LF6, 0x1)
					WRITE_RADIO_REG_ENTRY(pi, RFP_2069_PLL_XTAL4, 0x36CF)
					WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_TUNE1, 0xA0)
				ACPHY_REG_LIST_EXECUTE(pi);
			}
		}
		/* Move nbclip by 2dBs to the right */
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REGC(pi, NBRSSI_CONFG, core, nbrssi_ib_Refladder, 7);
			MOD_RADIO_REGC(pi, DAC_CFG1, core, DAC_invclk, 1);
		}

		/* Fix drift/unlock behavior */
		MOD_RADIO_REG(pi, RFP, PLL_CFG3, rfpll_spare1, 0x8);

	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		if ((RADIO2069REV(pi->pubpi->radiorev) != 25) &&
			(RADIO2069REV(pi->pubpi->radiorev) != 26)) {
			const chan_info_radio2069revGE16_t *ciGE16 = chan_info;

			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ciGE16->RFP_pll_vcocal5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ciGE16->RFP_pll_vcocal6);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ciGE16->RFP_pll_vcocal2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ciGE16->RFP_pll_vcocal1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
			                         ciGE16->RFP_pll_vcocal11);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
			                         ciGE16->RFP_pll_vcocal12);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ciGE16->RFP_pll_frct2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ciGE16->RFP_pll_frct3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
			                         ciGE16->RFP_pll_vcocal10);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ciGE16->RFP_pll_xtal3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ciGE16->RFP_pll_vco2);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
			                         ciGE16->RFP_logen5g_cfg1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ciGE16->RFP_pll_vco8);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ciGE16->RFP_pll_vco6);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ciGE16->RFP_pll_vco3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
			                         ciGE16->RFP_pll_xtalldo1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ciGE16->RFP_pll_hvldo1);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ciGE16->RFP_pll_hvldo2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ciGE16->RFP_pll_vco5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ciGE16->RFP_pll_vco4);

			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE16->RFP_pll_lf4);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE16->RFP_pll_lf5);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE16->RFP_pll_lf7);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE16->RFP_pll_lf2);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE16->RFP_pll_lf3);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE16->RFP_pll_cp4);
			phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE16->RFP_pll_lf6);

			phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
			                         ciGE16->RFP_logen2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE, ciGE16->RF0_lna2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
			                         ciGE16->RF0_txmix2g_cfg1);
			phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2, ciGE16->RF0_pga2g_cfg2);
			phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE, ciGE16->RF0_pad2g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
			                         ciGE16->RFP_logen5g_tune1);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
			                         ciGE16->RFP_logen5g_tune2);
			phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
			                         ciGE16->RF0_logen5g_rccr);
			phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE, ciGE16->RF0_lna5g_tune);
			phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
			                         ciGE16->RF0_txmix5g_cfg1);
			phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2, ciGE16->RF0_pga5g_cfg2);
			phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE, ciGE16->RF0_pad5g_tune);
			/*
			* phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5, ciGE16->RFP_pll_cp5);
			* phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1, ciGE16->RF0_afediv1);
			* phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2, ciGE16->RF0_afediv2);
			* phy_utils_write_radioreg(pi, RF0_2069_ADC_CFG5, ciGE16->RF0_adc_cfg5);
			*/

		} else {
			if (!PHY_XTAL_IS52M(pi)) {
				const chan_info_radio2069revGE25_t *ciGE25 = chan_info;
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5,
				                         ciGE25->RFP_pll_vcocal5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6,
				                         ciGE25->RFP_pll_vcocal6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2,
				                         ciGE25->RFP_pll_vcocal2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1,
				                         ciGE25->RFP_pll_vcocal1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
					ciGE25->RFP_pll_vcocal11);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
					ciGE25->RFP_pll_vcocal12);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2,
				                         ciGE25->RFP_pll_frct2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3,
				                         ciGE25->RFP_pll_frct3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
					ciGE25->RFP_pll_vcocal10);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3,
				                         ciGE25->RFP_pll_xtal3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CFG3,
				                         ciGE25->RFP_pll_cfg3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
				                         ciGE25->RFP_pll_vco2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
					ciGE25->RFP_logen5g_cfg1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8,
				                         ciGE25->RFP_pll_vco8);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6,
				                         ciGE25->RFP_pll_vco6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3,
				                         ciGE25->RFP_pll_vco3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
					ciGE25->RFP_pll_xtalldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1,
				                         ciGE25->RFP_pll_hvldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2,
				                         ciGE25->RFP_pll_hvldo2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5,
				                         ciGE25->RFP_pll_vco5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4,
				                         ciGE25->RFP_pll_vco4);

				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE25->RFP_pll_lf4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE25->RFP_pll_lf5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE25->RFP_pll_lf7);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE25->RFP_pll_lf2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE25->RFP_pll_lf3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE25->RFP_pll_cp4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE25->RFP_pll_lf6);

				phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
					ciGE25->RFP_logen2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE,
				                         ciGE25->RF0_lna2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
					ciGE25->RF0_txmix2g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2,
				                         ciGE25->RF0_pga2g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE,
				                         ciGE25->RF0_pad2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
					ciGE25->RFP_logen5g_tune1);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
					ciGE25->RFP_logen5g_tune2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
					ciGE25->RF0_logen5g_rccr);
				phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE,
				                         ciGE25->RF0_lna5g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
					ciGE25->RF0_txmix5g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2,
				                         ciGE25->RF0_pga5g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE,
				                         ciGE25->RF0_pad5g_tune);

				/*
				 * phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5,
				 *                          ciGE25->RFP_pll_cp5);
				 * phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1,
				 *                          ciGE25->RF0_afediv1);
				 * phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2,
				 *                          ciGE25->RF0_afediv2);
				 * phy_utils_write_radioreg(pi, RF0_2069_ADC_CFG5,
				 *                          ciGE25->RF0_adc_cfg5);
				 */

			} else {
				const chan_info_radio2069revGE25_52MHz_t *ciGE25 = chan_info;

				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5,
				                         ciGE25->RFP_pll_vcocal5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6,
				                         ciGE25->RFP_pll_vcocal6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2,
				                         ciGE25->RFP_pll_vcocal2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1,
				                         ciGE25->RFP_pll_vcocal1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11,
					ciGE25->RFP_pll_vcocal11);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12,
					ciGE25->RFP_pll_vcocal12);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2,
				                         ciGE25->RFP_pll_frct2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3,
				                         ciGE25->RFP_pll_frct3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10,
					ciGE25->RFP_pll_vcocal10);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3,
				                         ciGE25->RFP_pll_xtal3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
				                         ciGE25->RFP_pll_vco2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1,
					ciGE25->RFP_logen5g_cfg1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8,
				                         ciGE25->RFP_pll_vco8);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6,
				                         ciGE25->RFP_pll_vco6);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3,
				                         ciGE25->RFP_pll_vco3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1,
					ciGE25->RFP_pll_xtalldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1,
				                         ciGE25->RFP_pll_hvldo1);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2,
				                         ciGE25->RFP_pll_hvldo2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5,
				                         ciGE25->RFP_pll_vco5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4,
				                         ciGE25->RFP_pll_vco4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ciGE25->RFP_pll_lf4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ciGE25->RFP_pll_lf5);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ciGE25->RFP_pll_lf7);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ciGE25->RFP_pll_lf2);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ciGE25->RFP_pll_lf3);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ciGE25->RFP_pll_cp4);
				phy_utils_write_radioreg(pi, RFP_2069_PLL_LF6, ciGE25->RFP_pll_lf6);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE,
					ciGE25->RFP_logen2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LNA2G_TUNE,
				                         ciGE25->RF0_lna2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX2G_CFG1,
					ciGE25->RF0_txmix2g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA2G_CFG2,
				                         ciGE25->RF0_pga2g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD2G_TUNE,
				                         ciGE25->RF0_pad2g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1,
					ciGE25->RFP_logen5g_tune1);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2,
					ciGE25->RFP_logen5g_tune2);
				phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_RCCR,
					ciGE25->RF0_logen5g_rccr);
				phy_utils_write_radioreg(pi, RF0_2069_LNA5G_TUNE,
				                         ciGE25->RF0_lna5g_tune);
				phy_utils_write_radioreg(pi, RF0_2069_TXMIX5G_CFG1,
					ciGE25->RF0_txmix5g_cfg1);
				phy_utils_write_radioreg(pi, RF0_2069_PGA5G_CFG2,
				                         ciGE25->RF0_pga5g_cfg2);
				phy_utils_write_radioreg(pi, RF0_2069_PAD5G_TUNE,
				                         ciGE25->RF0_pad5g_tune);
			}

			/* 43162 FCBGA Settings improving Tx EVM */
			/* (1) ch4/ch4m settings to reduce 500k xtal spur */
			/* (2) Rreducing 2440/2480 RX spur */
			if (RADIO2069REV(pi->pubpi->radiorev) == 25 && PHY_XTAL_IS40M(pi)) {
			  ACPHY_REG_LIST_START
			    MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR23, ovr_xtal_coresize_nmos, 0x1)
			    MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL1, xtal_coresize_nmos, 0x8)
			    MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR23, ovr_xtal_coresize_pmos, 0x1)
			    MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL1, xtal_coresize_pmos, 0x8)
			  ACPHY_REG_LIST_EXECUTE(pi);

			  if (CHSPEC_IS2G(pi->radio_chanspec)) {
			    if (CHSPEC_CHANNEL(pi->radio_chanspec) < 12) {
			      if (CHSPEC_CHANNEL(pi->radio_chanspec) == 4)
						phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2,
						                         0xce4);
			      ACPHY_REG_LIST_START
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_bufstrg_BT, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x7)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x3)
			      ACPHY_REG_LIST_EXECUTE(pi);
			    } else {
			      ACPHY_REG_LIST_START
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL5, xtal_bufstrg_BT, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x0)
			        MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR27, ovr_xtal_xtbufstrg, 0x1)
			        MOD_RADIO_REG_ENTRY(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x1)
			      ACPHY_REG_LIST_EXECUTE(pi);
			    }
			  }
			}
		}
	} else {
		const chan_info_radio2069_t *ci = chan_info;

		/* Write chan specific tuning register */
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL5, ci->RFP_pll_vcocal5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL6, ci->RFP_pll_vcocal6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL2, ci->RFP_pll_vcocal2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL1, ci->RFP_pll_vcocal1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL11, ci->RFP_pll_vcocal11);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL12, ci->RFP_pll_vcocal12);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT2, ci->RFP_pll_frct2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_FRCT3, ci->RFP_pll_frct3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCOCAL10, ci->RFP_pll_vcocal10);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTAL3, ci->RFP_pll_xtal3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, ci->RFP_pll_vco2);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_CFG1, ci->RF0_logen5g_cfg1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO8, ci->RFP_pll_vco8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO6, ci->RFP_pll_vco6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO3, ci->RFP_pll_vco3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_XTALLDO1, ci->RFP_pll_xtalldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO1, ci->RFP_pll_hvldo1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_HVLDO2, ci->RFP_pll_hvldo2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO5, ci->RFP_pll_vco5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO4, ci->RFP_pll_vco4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF4, ci->RFP_pll_lf4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF5, ci->RFP_pll_lf5);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF7, ci->RFP_pll_lf7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF2, ci->RFP_pll_lf2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_LF3, ci->RFP_pll_lf3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP4, ci->RFP_pll_cp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP1, ci->RFP_pll_dsp1);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP2, ci->RFP_pll_dsp2);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP3, ci->RFP_pll_dsp3);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP4, ci->RFP_pll_dsp4);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP6, ci->RFP_pll_dsp6);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP7, ci->RFP_pll_dsp7);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP8, ci->RFP_pll_dsp8);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_DSP9, ci->RFP_pll_dsp9);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN2G_TUNE, ci->RF0_logen2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_LNA2G_TUNE, ci->RFX_lna2g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX2G_CFG1, ci->RFX_txmix2g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA2G_CFG2, ci->RFX_pga2g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD2G_TUNE, ci->RFX_pad2g_tune);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE1, ci->RF0_logen5g_tune1);
		phy_utils_write_radioreg(pi, RF0_2069_LOGEN5G_TUNE2, ci->RF0_logen5g_tune2);
		phy_utils_write_radioreg(pi, RFX_2069_LOGEN5G_RCCR, ci->RFX_logen5g_rccr);
		phy_utils_write_radioreg(pi, RFX_2069_LNA5G_TUNE, ci->RFX_lna5g_tune);
		phy_utils_write_radioreg(pi, RFX_2069_TXMIX5G_CFG1, ci->RFX_txmix5g_cfg1);
		phy_utils_write_radioreg(pi, RFX_2069_PGA5G_CFG2, ci->RFX_pga5g_cfg2);
		phy_utils_write_radioreg(pi, RFX_2069_PAD5G_TUNE, ci->RFX_pad5g_tune);
		phy_utils_write_radioreg(pi, RFP_2069_PLL_CP5, ci->RFP_pll_cp5);
		phy_utils_write_radioreg(pi, RF0_2069_AFEDIV1, ci->RF0_afediv1);
		phy_utils_write_radioreg(pi, RF0_2069_AFEDIV2, ci->RF0_afediv2);
		phy_utils_write_radioreg(pi, RFX_2069_ADC_CFG5, ci->RFX_adc_cfg5);

		/* We need different values for ADC_CFG5 for cores 1 and 2
		 * in order to get the best reduction of spurs from the AFE clk
		 */
		if (RADIO2069REV(pi->pubpi->radiorev) < 4) {
			ACPHY_REG_LIST_START
				WRITE_RADIO_REG_ENTRY(pi, RF1_2069_ADC_CFG5, 0x3e9)
				WRITE_RADIO_REG_ENTRY(pi, RF2_2069_ADC_CFG5, 0x3e9)
				MOD_RADIO_REG_ENTRY(pi, RFP, PLL_CP4, rfpll_cp_ioff, 0xa0)
			ACPHY_REG_LIST_EXECUTE(pi);
		}

		/* Reduce 500 KHz spur at fc=2427 MHz for both 4360 A0 and B0 */
		if (CHSPEC_CHANNEL(pi->radio_chanspec) == 4 &&
			((RADIO2069_MINORREV(pi->pubpi->radiorev) != 16) &&
				(RADIO2069_MINORREV(pi->pubpi->radiorev) != 17))) {
			phy_utils_write_radioreg(pi, RFP_2069_PLL_VCO2, 0xce4);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x5);
		}

		/* 43602 XTAL SPUR 2G WAR */
		if (ACMAJORREV_5(pi->pubpi->phy_rev) && CHSPEC_IS2G(pi->radio_chanspec)&&
				!(IS_4364_3x3(pi))) {
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_xtbufstrg, 0x3);
			MOD_RADIO_REG(pi, RFP, PLL_XTAL4, xtal_outbufstrg, 0x2);
		}

		/* Move nbclip by 2dBs to the right */
		MOD_RADIO_REG(pi, RFX, NBRSSI_CONFG, nbrssi_ib_Refladder, 7);

		/* 5g only: Changing RFPLL bandwidth to be 150MHz */
		if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) && !(IS_4364_3x3(pi)))
			wlc_2069_rfpll_150khz(pi);

		if ((RADIO2069_MINORREV(pi->pubpi->radiorev) == 7) &&
		   (BFCTL(pi->u.pi_acphy) == 3)) {
			if ((BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 3 ||
			     BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 5) &&
			    CHSPEC_IS2G(pi->radio_chanspec)) {
			  ACPHY_REG_LIST_START
			    /* 43602 MCH2: offtune to win back linear output power */
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_TUNE, pad2g_tune, 0x1)

			    /* increase gain */
			    MOD_RADIO_REG_ENTRY(pi, RFX, PGA2G_CFG1, pga2g_gainboost, 0x2)
			    WRITE_RADIO_REG_ENTRY(pi, RFX_2069_PAD2G_INCAP, 0x7e7e)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_IDAC, pad2g_idac_main, 0x38)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PGA2G_INCAP, pad2g_idac_aux, 0x38)
			    MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_IDAC, pad2g_idac_cascode, 0xe)
			  ACPHY_REG_LIST_EXECUTE(pi);
			} else if (BF3_FEMCTRL_SUB(pi->u.pi_acphy) == 0 &&
			           CHSPEC_ISPHY5G6G(pi->radio_chanspec)) {
			    /* 43602 MCH5: increase gain to win back linear output power */
			    MOD_RADIO_REGC(pi, TXMIX5G_CFG1, 2, gainboost, 0x4);
			    MOD_RADIO_REGC(pi, PGA5G_CFG1, 2, gainboost, 0x4);
			    MOD_RADIO_REG(pi, RFX, PAD5G_IDAC, idac_main, 0x3d);
			    MOD_RADIO_REG(pi, RFX, PAD5G_TUNE, idac_aux, 0x3d);
			}
		}
	}

	if (RADIO2069REV(pi->pubpi->radiorev) >= 4) {
		/* Make clamping stronger */
		phy_utils_write_radioreg(pi, RFX_2069_ADC_CFG5, 0x83e0);
	}

	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
	    (!(PHY_IPA(pi)))) {
	    MOD_RADIO_REG(pi, RFP, PLL_CP4, rfpll_cp_ioff, 0xe0);
	}

	/* increasing pabias to get good evm with pagain3 */
	if ((RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) &&
	    !(ACRADIO_2069_EPA_IS(pi->pubpi->radiorev))) {
		phy_utils_write_radioreg(pi, RF0_2069_PA5G_IDAC2, 0x8484);

		if (PHY_IPA(pi)) {
			ACPHY_REG_LIST_START
				WRITE_RADIO_REG_ENTRY(pi, RF0_2069_LOGEN5G_IDAC1, 0x3F37)
				WRITE_RADIO_REG_ENTRY(pi, RF0_2069_PGA5G_IDAC, 0x3838)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_OVR2, ovr_bg_pulse, 1)
				MOD_RADIO_REG_ENTRY(pi, RFP, GE16_BG_CFG1, bg_pulse, 1)
			ACPHY_REG_LIST_EXECUTE(pi);

			FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
				MOD_RADIO_REGC(pi, PAD5G_IDAC, core, idac_main, 0x20);
				MOD_RADIO_REGC(pi, PAD5G_TUNE, core, idac_aux, 0x20);

				MOD_RADIO_REGC(pi, PA5G_INCAP, core,
					pa5g_idac_incap_compen_main, 0x8);
				MOD_RADIO_REGC(pi, PA5G_INCAP, core,
					pa5g_idac_incap_compen_aux, 0x8);
				MOD_RADIO_REGC(pi, PA2G_INCAP, core,
					pa2g_ptat_slope_incap_compen_main, 0x0);
				MOD_RADIO_REGC(pi, PA2G_INCAP, core,
					pa2g_ptat_slope_incap_compen_aux, 0x0);

				if (pi->sh->chippkg == BCM4335_FCBGA_PKG_ID) {
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_main, 0x1);
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_aux, 0x1);
				} else {
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_main, 0x3);
					MOD_RADIO_REGC(pi, PA2G_CFG2, core,
						pa2g_bias_filter_aux, 0x3);
				}

				MOD_RADIO_REGC(pi, PAD5G_INCAP, core, idac_incap_compen_main, 0xc);
				MOD_RADIO_REGC(pi, PAD5G_INCAP, core, idac_incap_compen_aux, 0xc);
				MOD_RADIO_REGC(pi, PGA5G_INCAP, core, idac_incap_compen, 0x8);
				MOD_RADIO_REGC(pi, PA5G_CFG2, core, pa5g_bias_cas, 0x58);
				MOD_RADIO_REGC(pi, PA5G_IDAC2, core, pa5g_biasa_main, 0x84);
				MOD_RADIO_REGC(pi, PA5G_IDAC2, core, pa5g_biasa_aux, 0x84);
				MOD_RADIO_REGC(pi, GE16_OVR21, core, ovr_mix5g_gainboost, 0x1);
				MOD_RADIO_REGC(pi, TXMIX5G_CFG1, core, gainboost, 0x0);
				MOD_RADIO_REGC(pi, TXGM_CFG1, core, gc_res, 0x0);
				MOD_RADIO_REGC(pi, PA2G_CFG3, core, pa2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PAD2G_SLOPE, core, pad2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PGA2G_CFG2, core, pga2g_ptat_slope_main, 0x7);
				MOD_RADIO_REGC(pi, PGA2G_IDAC, core, pga2g_idac_main, 0x15);
				MOD_RADIO_REGC(pi, PAD2G_TUNE, core, pad2g_idac_tuning_bias, 0xc);
				MOD_RADIO_REGC(pi, TXMIX2G_CFG1, core, lodc, 0x3);
			}
		}
	}

	/* Tuning for 43569 iPA */
	if ((RADIOREV(pi->pubpi->radiorev) == 0x27 ||
	  RADIOREV(pi->pubpi->radiorev) == 0x29 ||
	  RADIOREV(pi->pubpi->radiorev) == 0x28 ||
	  RADIOREV(pi->pubpi->radiorev) == 0x2C) &&
	  (PHY_XTAL_IS40M(pi))) {
		ACPHY_REG_LIST_START
			/* 2G */
			MOD_RADIO_REG_ENTRY(pi, RFX, TXMIX2G_CFG1, tune, 0x1)
			MOD_RADIO_REG_ENTRY(pi, RF0, PGA2G_CFG2, pga2g_tune, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RF1, PGA2G_CFG2, pga2g_tune, 0x0)
			MOD_RADIO_REG_ENTRY(pi, RFX, PAD2G_TUNE, pad2g_tune, 0x1)
			WRITE_RADIO_REG_ENTRY(pi, RF0_2069_PAD2G_INCAP, 0x7808)
			WRITE_RADIO_REG_ENTRY(pi, RF1_2069_PAD2G_INCAP, 0x7a0a)
			MOD_RADIO_REG_ENTRY(pi, RFX, PA2G_CFG2, pa2g_bias_filter_main, 0xf)
			MOD_RADIO_REG_ENTRY(pi, RFX, PA2G_CFG2, pa2g_bias_filter_aux, 0xf)
			/* 5G */
			MOD_RADIO_REG_ENTRY(pi, RF0, PGA5G_CFG2, tune, 0xa)
			MOD_RADIO_REG_ENTRY(pi, RF1, PGA5G_CFG2, tune, 0x8)
		ACPHY_REG_LIST_EXECUTE(pi);

		/* 43570 only */
		if (CST4350_IFC_MODE(pi->sh->sih->chipst) == CST4350_IFC_MODE_PCIE) {
			ACPHY_REG_LIST_START
				WRITE_RADIO_REG_ENTRY(pi, RFX_2069_PGA5G_IDAC, 0x3030)
				MOD_RADIO_REG_ENTRY(pi, RFX, PAD5G_IDAC, idac_main, 0x36)
			ACPHY_REG_LIST_EXECUTE(pi);
		}
	}
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		wlc_phy_2069_4335_set_ovrds(pi);
	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		wlc_phy_2069_4350_set_ovrds(pi->u.pi_acphy->radioi);
	}

	/* Offset RCCAL by 4 when using divide by 5 for ADC stability */
	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2 && (!PHY_IPA(pi))) {
		FOREACH_CORE(pi, core) {
			if ((!(fc == 5190) && CHSPEC_IS40(pi->radio_chanspec)) ||
			    (CHSPEC_IS20(pi->radio_chanspec) &&
			     ((((fc == 5180) && (pi->sh->chippkg != 2)) ||
			       ((fc >= 5200) && (fc <= 5320)) ||
			       ((fc >= 5745) && (fc <= 5825)))))) {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0,
				               pi_ac->radioi->rccal_adc_rc + 4);
			} else {
				MOD_RADIO_REGC(pi, ADC_RC1, core, adc_ctl_RC_4_0,
				               pi_ac->radioi->rccal_adc_rc);
			}
		}
	}

	/* 4335C0: Current optimization */
	acphy_set_lpmode(pi, ACPHY_LP_RADIO_LVL_OPT);

	/* Do a VCO cal after writing the tuning table regs */
	if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
		/* BCAWLAN-226487 - add a settling period to avoid RFPLL unlock */
		OSL_DELAY(200);
	}
	wlc_phy_radio2069_vcocal(pi);
}

void
wlc_phy_radio2069_check_vco_cal_acphy(phy_info_t *pi)
{
	bool mac_suspend;

	mac_suspend = (R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);
	if (mac_suspend) wlapi_suspend_mac_and_wait(pi->sh->physhim);

	/* Monitor vcocal refresh bit and relock PLL if out of range */
	if (READ_RADIO_REGFLD(pi, RFP, PLL_DSPR27, rfpll_monitor_need_refresh)) {
		PHY_INFORM(("wl%d: %s: RFPLL needs refresh, trigger VCO cal\n",
		            pi->sh->unit, __FUNCTION__));

		wlc_phy_radio2069_vcocal(pi);
	}

	if (mac_suspend)
		wlapi_enable_mac(pi->sh->physhim);
}

/**
 * initialize the static tables defined in auto-generated wlc_phytbl_ac.c,
 * see acphyprocs.tcl, proc acphy_init_tbls
 * After called in the attach stage, all the static phy tables are reclaimed.
 */
static void
WLBANDINITFN(wlc_phy_static_table_download_acphy)(phy_info_t *pi)
{
	uint idx;
	uint8 stall_val;
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);
	if (pi->phy_init_por) {
		/* these tables are not affected by phy reset, only power down */
		if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			if ((wlc_phy_ac_phycap_maxbw(pi) > BW_20MHZ)) {
				for (idx = 0; idx < acphytbl_info_sz_rev128; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev128[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_info_rev128,
					acphyzerotbl_info_cnt_rev128);
			}
			else {
				for (idx = 0; idx < acphytbl_info_sz_maxbw20_rev128; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_maxbw20_rev128[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_info_maxbw20_rev128,
					acphyzerotbl_info_cnt_maxbw20_rev128);
			}
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev51; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev51[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev51,
				acphyzerotbl_info_cnt_rev51);
		} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev128; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev128[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev128,
				acphyzerotbl_info_cnt_rev128);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev130; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev130[idx]);
			}
			/* FIXME6715 */
			//wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev130,
			//	acphyzerotbl_info_cnt_rev130);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev131; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev131[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev131,
				acphyzerotbl_info_cnt_rev131);
		} else if (ACMAJORREV_32(pi->pubpi->phy_rev) ||
				ACMAJORREV_33(pi->pubpi->phy_rev) ||
				ACMAJORREV_47_129(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev32; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev32[idx]);
			}
			if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
				for (idx = 0; idx < acphytbl_info_sz_rev47; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev47[idx]);
				}
				if (ACMINORREV_GE(pi, 1)) {
					for (idx = 0; idx < acphytbl_info_sz_rev47_1; idx++) {
						wlc_phy_table_write_ext_acphy(pi,
							&acphytbl_info_rev47_1[idx]);
					}
				}
			}
			if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
				for (idx = 0; idx < acphytbl_info_sz_rev129; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev129[idx]);
				}
			}
			if (ACMAJORREV_33(pi->pubpi->phy_rev)) {
				for (idx = 0; idx < acphytbl_info_sz_rev33; idx++) {
					wlc_phy_table_write_ext_acphy(pi,
						&acphytbl_info_rev33[idx]);
				}
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev32,
				acphyzerotbl_info_cnt_rev32);
		} else if (ACMAJORREV_5(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev9; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev9[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev9,
				acphyzerotbl_info_cnt_rev9);
		} else if (ACMAJORREV_2(pi->pubpi->phy_rev)) {
			if (ACREV_IS(pi->pubpi->phy_rev, 9)) { /* 43602a0 */
				for (idx = 0; idx < acphytbl_info_sz_rev9; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev9[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev9,
					acphyzerotbl_info_cnt_rev9);
			} else {
				for (idx = 0; idx < acphytbl_info_sz_rev3; idx++) {
					wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev3[idx]);
				}
				wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev3,
				acphyzerotbl_info_cnt_rev3);
			}
		} else if (ACMAJORREV_0(pi->pubpi->phy_rev)) {
			for (idx = 0; idx < acphytbl_info_sz_rev0; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev0[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev0,
				acphyzerotbl_info_cnt_rev0);
		} else if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
			uint16 phymode = phy_get_phymode(pi);
			for (idx = 0; idx < acphytbl_info_sz_rev12; idx++) {
				wlc_phy_table_write_ext_acphy(pi, &acphytbl_info_rev12[idx]);
			}
			wlc_phy_clear_static_table_acphy(pi, acphyzerotbl_info_rev12,
				acphyzerotbl_info_cnt_rev12);

			if ((phymode == PHYMODE_MIMO) || (phymode == PHYMODE_80P80)) {
				wlc_phy_clear_static_table_acphy(pi,
					acphyzerotbl_delta_MIMO_80P80_info_rev12,
					acphyzerotbl_delta_MIMO_80P80_info_cnt_rev12);
			}
		}
	}

	ACPHY_ENABLE_STALL(pi, stall_val);
}

/*  R Calibration (takes ~50us) */
static void
wlc_phy_radio_tiny_rcal(phy_info_t *pi, acphy_tiny_rcal_modes_t calmode)
{
	/* Format: 20691_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */

	uint8 rcal_valid, rcal_value;
	uint8 core = 0;
	bool is_mode3_and_rsdb_core1 = FALSE;
	uint16 macmode = phy_get_phymode(pi);
	uint8 curr_core = phy_get_current_core(pi);
	phy_info_t *other_pi = phy_get_other_pi(pi), *tmp_pi;

	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));

	if ((calmode == TINY_RCAL_MODE3_BOTH_CORE_CAL) &&
		(macmode == PHYMODE_RSDB) && (curr_core == PHY_RSBD_PI_IDX_CORE1)) {
		is_mode3_and_rsdb_core1 = TRUE;
	}

	if ((calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) ||
		(calmode == TINY_RCAL_MODE3_BOTH_CORE_CAL)) {

		FOREACH_CORE(pi, core) {
			if (calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) {
				if (((macmode == PHYMODE_RSDB) &&
					(curr_core == PHY_RSBD_PI_IDX_CORE1)) ||
					((macmode != PHYMODE_RSDB) && (core == 1))) {
					break;
				}
			}

			/* This should run only for core 0 */
			/* Run R Cal and use its output */
			/* JIRA: SW4349-1383.
			 * instead of swithc base adress to core 0, use other_pi
			 */
			tmp_pi = is_mode3_and_rsdb_core1 ? other_pi : pi;

			MOD_RADIO_REG_TINY(tmp_pi, PLL_XTAL2, 0, xtal_pu_RCCAL1, 0x1);
			MOD_RADIO_REG_TINY(tmp_pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0x1);

			RADIO_REG_LIST_START
				/* Make connection with the external 10k resistor */
				/* Also powers up rcal (RF_rcal_cfg.rcal_pu = 1) */
				MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL2, core, gpaio_pu, 1)
				MOD_RADIO_REG_TINY_ENTRY(pi, RCAL_CFG_NORTH, core, rcal_pu, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL0, core,
					gpaio_sel_0to15_port, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, GPAIO_SEL1, core,
					gpaio_sel_16to31_port, 0)
				MOD_RADIO_REG_TINY_ENTRY(pi, RCAL_CFG_NORTH, core, rcal_pu, 1)
			RADIO_REG_LIST_EXECUTE(pi, core);

			SPINWAIT(READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
				core, rcal_valid) == 0,
				ACPHY_SPINWAIT_RCAL_STATUS);
			if (READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_valid) == 0) {
				PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR :", __FUNCTION__));
				PHY_FATAL_ERROR_MESG(("RCAL is not valid for Core%d\n", core));
				PHY_FATAL_ERROR(pi, PHY_RC_RCAL_INVALID);
			}
			rcal_valid =  READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_valid);

			if (rcal_valid == 1) {
				rcal_value = READ_RADIO_REGFLD_TINY(pi, RCAL_CFG_NORTH,
					core, rcal_value) >> 1;

				if (pi->fabid == BCM4349_UMC_FAB_ID) {
					/* In UMC B0 & B1 materials, core0 reference voltage
					 * (Vref) used by RCAL is ~150mV lower from target value
					 * 0.52V It creates 1~2 codes lower than optimal value
					 * 1 RCAL code corresponds to ~150mV change in Vref
					 * So it is recommend to add one code to core0 RCAL
					 * results in ATE
					 */
					if (!((macmode == PHYMODE_RSDB) ? curr_core : core))
						rcal_value += 1;
				}

				PHY_TRACE(("*** rcal_value: %x\n", rcal_value));

#ifdef ATE_BUILD
				wl_ate_set_buffer_regval(RCAL_VALUE, rcal_value,
						core, phy_get_current_core(pi), pi->sh->chip);
#endif

				/* Use the output of the rcal engine */
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
				if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
					/* Copy the rcal value instead of latching */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core,
						ovr_bg_rcal_trim,  1);
					MOD_RADIO_REG_TINY(pi, BG_CFG1, core,
						bg_rcal_trim,  rcal_value);
				}
				else { /* Use latched output of rcal engine */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core,
						ovr_bg_rcal_trim, 0);
				}
				/* Very coarse sanity check */
				if ((rcal_value < 2) || (12 < rcal_value)) {
					PHY_ERROR(("*** ERROR: R Cal value out of range."
					" 4bit Rcal = %d.\n", rcal_value));
				}
			} else {
				PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
					__FUNCTION__, rcal_valid));
			}
			/* Power down blocks not needed anymore */
			MOD_RADIO_REG_TINY(pi, GPAIO_SEL2, core, gpaio_pu, 0);
			MOD_RADIO_REG_TINY(pi, PLL_XTAL2, core, xtal_pu_RCCAL1, 0);
			MOD_RADIO_REG_TINY(pi, PLL_XTAL2, core, xtal_pu_RCCAL,  0);
			MOD_RADIO_REG_TINY(pi, RCAL_CFG_NORTH, core, rcal_pu, 0);
		}

		if (calmode == TINY_RCAL_MODE2_SINGLE_CORE_CAL) {
			/* for RSDB/MIMO core 1, copy rcal value from pi of core 0 */
			if (macmode != PHYMODE_RSDB) {
				rcal_value = READ_RADIO_REGFLD_TINY(pi, BG_CFG1,
					0, bg_rcal_trim);
				core = 1;
			} else {
				phy_info_t *pi_core0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
				rcal_value = READ_RADIO_REGFLD_TINY(pi_core0, BG_CFG1,
					0, bg_rcal_trim);
				core = 0;
			}
			MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim,  rcal_value);
			MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim,  1);
		}
	} else {
		FOREACH_CORE(pi, core) {
			if (calmode == TINY_RCAL_MODE0_OTP) {
				if ((pi->sromi->rcal_otp_val >= 2) &&
					(pi->sromi->rcal_otp_val <= 12)) {
					/* Use OTP stored rcal value */
					MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 1);
				} else {
					wlc_phy_radio_tiny_rcal(pi, TINY_RCAL_MODE1_STATIC);
				}
			} else if (calmode == TINY_RCAL_MODE1_STATIC) {
				/* 4345c0 and later */
				if (pi->fabid == BCM4349_TSMC_FAB_ID)
					rcal_value = TSMC_FAB_RCAL_VAL;
				else
					rcal_value = UMC_FAB_RCAL_VAL;

				MOD_RADIO_REG_TINY(pi, BG_CFG1, core, bg_rcal_trim, rcal_value);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
				MOD_RADIO_REG_TINY(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
			}
		}
	}
}

static void
wlc_phy_radio_tiny_rcal_wave2(phy_info_t *pi, uint8 mode)
{
	/* Format: 20691_r_cal [<mode>] [<rcalcode>] */
	/* If no arguments given, then mode is assumed to be 1 */
	/* The rcalcode argument works only with mode 1 and is optional. */
	/* If given, then that is the rcal value what will be used in the radio. */

	/* Documentation: */
	/* Mode 0 = Don't run cal, use rcal value stored in OTP. This is what driver should */
	/* do but is not good for bringup because the OTP is not programmed yet. */
	/* Mode 1 = Don't run cal, don't use OTP rcal value, use static rcal */
	/* value given here. Good for initial bringup. */
	/* Mode 2 = Run rcal and use the return value in bandgap. Needs the external 10k */
	/* resistor to be connected to GPAIO otherwise cal will return bogus value. */

	uint8 rcal_valid, loop_iter, rcal_value;
	uint8 core = 0;
	/* Skip this function for QT */
	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));
	if (mode == 0) {
		/* Use OTP stored rcal value */
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 1);
	} else if (mode == 1) {
		/* Default RCal code to be used with mode 1 is 8 */
		MOD_RADIO_PLLREG_20693(pi, BG_CFG1, core, bg_rcal_trim, 8);
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, core, ovr_otp_rcal_sel, 0);
		MOD_RADIO_PLLREG_20693(pi, BG_OVR1, core, ovr_bg_rcal_trim, 1);
	} else if (mode == 2) {
		/* This should run only for core 0 */
		/* Run R Cal and use its output */
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);
		MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x33);

		/* Make connection with the external 10k resistor */
		/* clear all the test-point */
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL0, gpaio_sel_0to15_port, 0);
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL1, gpaio_sel_16to31_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL0, 0,
		                    top_gpaio_sel_0to15_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL1, 0,
		                    top_gpaio_sel_16to31_port, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL2, 0, top_gpaio_pu, 1);
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 1);

		rcal_valid = 0;
		loop_iter = 0;
		while ((rcal_valid == 0) && (loop_iter <= 100)) {
			OSL_DELAY(1000);
			rcal_valid = READ_RADIO_PLLREGFLD_20693(pi, RCAL_CFG_NORTH, 0, rcal_valid);
			loop_iter ++;
		}

		if (rcal_valid == 1) {
			rcal_value = READ_RADIO_PLLREGFLD_20693(pi,
				RCAL_CFG_NORTH, 0, rcal_value) >> 1;

			/* Use the output of the rcal engine */
			MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_otp_rcal_sel, 0);
			/* MOD_RADIO_PLLREG_20693(pi, BG_CFG1, 0, bg_rcal_trim, rcal_value); */
			MOD_RADIO_PLLREG_20693(pi, BG_OVR1, 0, ovr_bg_rcal_trim, 0);

			/* Very coarse sanity check */
			if ((rcal_value < 2) || (12 < rcal_value)) {
				PHY_ERROR(("*** ERROR: R Cal value out of range."
				           " 4bit Rcal = %d.\n", rcal_value));
			}
		} else {
			PHY_ERROR(("%s RCal unsucessful. RCal valid bit is %d.\n",
			           __FUNCTION__, rcal_valid));
		}

		/* Power down blocks not needed anymore */
		MOD_RADIO_ALLREG_20693(pi, GPAIO_SEL2, gpaio_pu, 0);
		MOD_RADIO_REG_20693(pi, GPAIOTOP_SEL2, 0, top_gpaio_pu, 0);
		MOD_RADIO_PLLREG_20693(pi, RCAL_CFG_NORTH, 0, rcal_pu, 0);
		MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3);
	}
}

static void
wlc_phy_radio_tiny_rccal(phy_ac_radio_info_t *ri)
{
	uint8 cal;
	uint16 n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x68};
	uint16 trc[] = {0x22d, 0xf0, 0x134};
	phy_info_t *pi = ri->pi;
	uint32 dn;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));

	/* For RSDB core 1, copy core 0 values to core 1 for rc cal and return */
	if ((phy_get_phymode(pi) == PHYMODE_RSDB) &&
		(phy_get_current_core(pi) != PHY_RSBD_PI_IDX_CORE0) &&
		(ACMAJORREV_4(pi->pubpi->phy_rev))) {
		phy_info_t *pi_rsdb0 = phy_get_pi(pi, PHY_RSBD_PI_IDX_CORE0);
		ri->data->rccal_gmult = pi_rsdb0->u.pi_acphy->radioi->data->rccal_gmult;
		ri->data->rccal_gmult_rc = pi_rsdb0->u.pi_acphy->radioi->data->rccal_gmult_rc;
		ri->rccal_adc_gmult = pi_rsdb0->u.pi_acphy->radioi->rccal_adc_gmult;
		ri->data->rccal_dacbuf = pi_rsdb0->u.pi_acphy->radioi->data->rccal_dacbuf;
		return;
	}

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 1);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_X1, x1[cal]);
		phy_utils_write_radioreg(pi, RADIO_REG(pi, RCCAL_CFG2, 0), trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			MOD_RADIO_REG_TINY(pi, TX_DAC_CFG5, 0, DACbuf_fixed_cap, 0);
			MOD_RADIO_REG_TINY(pi, TX_BB_OVR1, 0, ovr_DACbuf_fixed_cap, 1);
		}

		/* Toggle RCCAL power */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_START, 1);

		/* This delay is required before reading the RCCAL_CFG3
		*   without this delay, the cal values are incorrect.
		*/
		OSL_DELAY(100);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		SPINWAIT(READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG3, 0, rccal_DONE) == 0,
				MAX_2069_RCCAL_WAITLOOPS);
		if (READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG3, 0, rccal_DONE) == 0) {
			PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR : RCCAL invalid \n",
				__FUNCTION__));
			PHY_FATAL_ERROR(pi, PHY_RC_RCCAL_INVALID);
		}

		/* Stop RCCAL */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG1, 0, rccal_START, 0);

		n0 = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */

		if (cal == 0) {
			/* lpf */
			/* set k [expr {$is_adc ? 102051 : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->data->rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			ri->data->rccal_gmult_rc = ri->data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->data->rccal_gmult));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_LPF, ri->data->rccal_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? 102051 : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->rccal_adc_gmult = (85000 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->rccal_adc_gmult));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(GMULT_ADC, ri->rccal_adc_gmult, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		} else {
			/* dacbuf */
			ri->data->rccal_dacbuf = READ_RADIO_REGFLD_TINY(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_REG_TINY(pi, TX_BB_OVR1, 0, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, ri->data->rccal_dacbuf));
#ifdef ATE_BUILD
			wl_ate_set_buffer_regval(RCCAL_DACBUF, ri->data->rccal_dacbuf, -1,
			phy_get_current_core(pi), pi->sh->chip);
#endif
		}
		/* Turn off rccal */
		MOD_RADIO_REG_TINY(pi, RCCAL_CFG0, 0, rccal_pu, 0);
	}
	/* Powerdown rccal driver */
	MOD_RADIO_REG_TINY(pi, PLL_XTAL2, 0, xtal_pu_RCCAL, 0);
}

static void
wlc_phy_radio_tiny_rccal_wave2(phy_ac_radio_info_t *ri)
{
	uint8 cal, done;
	uint16 rccal_itr, n0, n1;

	/* lpf, adc, dacbuf */
	uint8 sr[] = {0x1, 0x1, 0x0};
	uint8 sc[] = {0x0, 0x2, 0x1};
	uint8 x1[] = {0x1c, 0x70, 0x68};
	uint16 trc[] = {0x22d, 0xf0, 0x134};
	phy_info_t *pi = ri->pi;
	uint32 dn;

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(TINY_RADIO(pi));

	/* Powerup rccal driver & set divider radio (rccal needs to run at 20mhz) */
	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x23);

	/* Calibrate lpf, adc, dacbuf */
	for (cal = 0; cal < NUM_2069_RCCAL_CAPS; cal++) {
		/* Setup */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_sr, sr[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_sc, sc[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_X1, x1[cal]);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG2, 0, rccal_Trc, trc[cal]);

		/* For dacbuf force fixed dacbuf cap to be 0 while calibration, restore it later */
		if (cal == 2) {
			MOD_RADIO_ALLREG_20693(pi, TX_DAC_CFG5, DACbuf_fixed_cap, 0);
			MOD_RADIO_ALLREG_20693(pi, TX_BB_OVR1, ovr_DACbuf_fixed_cap, 1);
		}

		/* Toggle RCCAL power */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 0);
		OSL_DELAY(1);
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 1);

		OSL_DELAY(35);

		/* Start RCCAL */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_START, 1);

		/* Wait for rcal to be done, max = 100us * 100 = 10ms  */
		done = 0;
		for (rccal_itr = 0;
			(rccal_itr < MAX_2069_RCCAL_WAITLOOPS) && (done == 0);
			rccal_itr++) {
			OSL_DELAY(100);
			done = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG3, 0, rccal_DONE);
		}

		/* Stop RCCAL */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG1, 0, rccal_START, 0);

		/* Make sure that RC Cal ran to completion */
		ASSERT(done);

		n0 = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG4, 0, rccal_N0);
		n1 = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG5, 0, rccal_N1);
		dn = n1 - n0; /* set dn [expr {$N1 - $N0}] */

		if (cal == 0) {
			/* lpf */
			/* set k [expr {$is_adc ? (($def(d11_radio_major_rev) == 3)? */
			/* 85000 : 102051) : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->data->rccal_gmult = (101541 * dn) / (pi->xtalfreq >> 12);
			ri->data->rccal_gmult_rc = ri->data->rccal_gmult;
			PHY_INFORM(("wl%d: %s rccal_lpf_gmult = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->data->rccal_gmult));
#ifdef ATE_BUILD
			ate_buffer_regval[0].gmult_lpf = ri->data->rccal_gmult;
#endif
		} else if (cal == 1) {
			/* adc */
			/* set k [expr {$is_adc ? (($def(d11_radio_major_rev) == 3)? */
			/* 85000 : 102051) : 101541}] */
			/* set gmult_p12 [expr {$prod1 / $fxtal_pm12}] */
			ri->rccal_adc_gmult = (85000 * dn) / (pi->xtalfreq >> 12);
			PHY_INFORM(("wl%d: %s rccal_adc = %d\n", pi->sh->unit,
			            __FUNCTION__, ri->rccal_adc_gmult));
#ifdef ATE_BUILD
			ate_buffer_regval[0].gmult_adc = ri->rccal_adc_gmult;
#endif
		} else {
			/* dacbuf */
			ri->data->rccal_dacbuf = READ_RADIO_PLLREGFLD_20693(pi, RCCAL_CFG6, 0,
			                                              rccal_raw_dacbuf);
			MOD_RADIO_ALLREG_20693(pi, TX_BB_OVR1, ovr_DACbuf_fixed_cap, 0);
			PHY_INFORM(("wl%d: %s rccal_dacbuf = %d\n", pi->sh->unit,
				__FUNCTION__, ri->data->rccal_dacbuf));
#ifdef ATE_BUILD
			ate_buffer_regval[0].rccal_dacbuf = ri->data->rccal_dacbuf;
#endif
		}
		/* Turn off rccal */
		MOD_RADIO_PLLREG_20693(pi, RCCAL_CFG0, 0, rccal_pu, 0);
	}

	MOD_RADIO_PLLREG_20693(pi, WL_XTAL_CFG1, 0, wl_xtal_out_pu, 0x3);
}

/* ***************************** */
/*		External Defintions			*/
/* ***************************** */

/*
Initialize chip regs(RWP) & tables with init vals that do not get reset with phy_reset
*/
static void
wlc_phy_set_regtbl_on_pwron_acphy(phy_info_t *pi)
{
	uint8 core;
	uint16 val;
	uint16 rfseq_bundle_48[3];

	bool flag2rangeon = ((CHSPEC_IS2G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi2g(pi->tpci)) ||
		(CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
		phy_tpc_get_tworangetssi5g(pi->tpci))) && PHY_IPA(pi);

	/* force afediv(core 0, 1, 2) always high */
	if (!(ACMAJORREV_2(pi->pubpi->phy_rev) || ACMAJORREV_5(pi->pubpi->phy_rev)))
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0x77);

	ACPHY_REG_LIST_START
		/* Remove RFCTRL signal overrides for all cores */
		ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0)
		ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0)
		ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0)
	ACPHY_REG_LIST_EXECUTE(pi);
	if ((ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) ||
			ACMAJORREV_5(pi->pubpi->phy_rev)) {
		ACPHYREG_BCAST(pi, AfeClkDivOverrideCtrlN0, 0);
		WRITE_PHYREG(pi, AfeClkDivOverrideCtrl, 0);
	}

	/* logen_pwrup = 1, logen_reset = 0 */
	ACPHYREG_BCAST(pi, RfctrlCoreTxPus0, 0x80);
	ACPHYREG_BCAST(pi, RfctrlOverrideTxPus0, 0x180);

	/* Force LOGENs on both cores in the 5G to be powered up for 435x */
	if (ACMAJORREV_2(pi->pubpi->phy_rev) && !ACMINORREV_0(pi)) {
		ACPHY_REG_LIST_START
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideTxPus0, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlCoreTxPus0, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlOverrideTxPus1, logen5g_lob_pwrup, 1)
			MOD_PHYREG_ENTRY(pi, RfctrlCoreTxPus1, logen5g_lob_pwrup, 1)
		ACPHY_REG_LIST_EXECUTE(pi);
	}

	if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
		PHY_INFORM(("rfseq_bundle_tbl : {0x01C7034 0x01C7014 0x02C703E"
		 " 0x02C701C} \n"));

		/* set rfseq_bundle_tbl {0x01C7034 0x01C7014 0x02C703E 0x02C701C} */
		/* acphy_write_table RfseqBundle $rfseq_bundle_tbl 0 */
		rfseq_bundle_48[0] = 0x703C;
		rfseq_bundle_48[1] = 0x1c;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 0, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x7014;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 1, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x700e;
		rfseq_bundle_48[1] = 0x2c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 2, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x702c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 3, 48, rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x6020;
		rfseq_bundle_48[1] = 0x20;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 4, 48, rfseq_bundle_48);

		/* apply CLB FEM priority static settings */
		wlc_phy_set_clb_femctrl_static_settings(pi);

		/* Reset pktproc state and force RESET2RX sequence
		This is required to allow bundle commands loading in RFseq
		before we switch to bundle controls RfctrlCmd=0x306
		*/
		WRITE_PHYREG(pi, RfseqMode, 0);
		wlc_phy_resetcca_acphy(pi);

		/* Remove RFCTRL signal overrides for all cores */
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x306)
			WRITE_PHYREG_ENTRY(pi, AfeClkDivOverrideCtrl, 0x1)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrlN0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideTxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrl28nm0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, LogenOverrideCtrl28nm0, 0x0)
		ACPHY_REG_LIST_EXECUTE(pi);
		/* for 4347, aux slice, setting rfpll_clk_buffer_pu using phy override */
		if (wlapi_si_coreunit(pi->sh->physhim) == DUALMAC_AUX) {
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0xc);
		}
		else {
			WRITE_PHYREG(pi, RfctrlCoreGlobalPus, 0x4);
		}
		WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, 0xd);
		MOD_PHYREG(pi, RfctrlCmd, chip_pu, 0x1);
	} else if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
		/* FIXME63178: all code in this clause is likely redundant for rev47,51 */
		ACPHY_REG_LIST_START
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrl28nm0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, LogenOverrideCtrl28nm0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfCtrlCorePwrSw0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, dyn_radiob0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, Extra1AfeClkDivOverrideCtrl28nm0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, Extra2AfeClkDivOverrideCtrl28nm0, 0x0)
			MOD_PHYREG_ENTRY(pi, RfctrlCmd, syncResetEn, 0)
		ACPHY_REG_LIST_EXECUTE(pi);
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, Dac_gain, core, afe_iqdac_att, 0xf);
		}
	}

	/* Switch off rssi2 & rssi3 as they are not used in normal operation */
	ACPHYREG_BCAST(pi, RfctrlCoreRxPus0, 0);
	ACPHYREG_BCAST(pi, RfctrlOverrideRxPus0, 0x5000);

	/* Disable the SD-ADC's overdrive detect feature */
	if (!(ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev))) {
		val = READ_PHYREG(pi, RfctrlCoreAfeCfg20) |
		    ACPHY_RfctrlCoreAfeCfg20_afe_iqadc_reset_ov_det_MASK(pi->pubpi->phy_rev);
		ACPHYREG_BCAST(pi, RfctrlCoreAfeCfg20, val);
		val = READ_PHYREG(pi, RfctrlOverrideAfeCfg0) |
		    ACPHY_RfctrlOverrideAfeCfg0_afe_iqadc_reset_ov_det_MASK(pi->pubpi->phy_rev);
		ACPHYREG_BCAST(pi, RfctrlOverrideAfeCfg0, val);
	}

	/* initialize all the tables defined in auto-generated wlc_phytbl_ac.c,
	 * see acphyprocs.tcl, proc acphy_init_tbls
	 *  skip static one after first up
	 */
	PHY_TRACE(("wl%d: %s, dnld tables = %d\n", pi->sh->unit,
	           __FUNCTION__, pi->phy_init_por));

	/* these tables are not affected by phy reset, only power down */
	wlc_phy_static_table_download_acphy(pi);

	/* Initialize idle-tssi to be -420 before doing idle-tssi cal */
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, -190);
	} else {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_path0, 0x25C);
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev) ||
				ACMAJORREV_128(pi->pubpi->phy_rev)) {
			ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
			ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_third_path0, 0x25C);
		}
	}

	if (ACMAJORREV_2(pi->pubpi->phy_rev) && BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}

	if (flag2rangeon) {
		ACPHYREG_BCAST(pi, TxPwrCtrlIdleTssi_second_path0, 0x25C);
	}

	/* Enable the Ucode TSSI_DIV WAR */
	if (ACMAJORREV_2(pi->pubpi->phy_rev) && BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
		wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
		               WLC_BAND_ALL);
	}

	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
		uint8 lutinitval = 0;
		uint16 offset;
		phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

		BCM_REFERENCE(stf_shdata);

		if (BF3_TSSI_DIV_WAR(pi->u.pi_acphy)) {
			wlapi_bmac_mhf(pi->sh->physhim, MHF2, MHF2_PPR_HWPWRCTL, MHF2_PPR_HWPWRCTL,
			WLC_BAND_2G);
		}
		/* Clean up the estPwrShiftLUT Table */
		/* Table is 26bit witdth, we are writing 32bit at a time */
		FOREACH_ACTV_CORE(pi, stf_shdata->hw_phyrxchain, core) {
			for (offset = 0; offset < 40; offset++) {
				wlc_phy_table_write_acphy(pi,
					wlc_phy_get_tbl_id_estpwrshftluts(pi, core),
					1, offset, 32, &lutinitval);
			}
		}
	}

	/* #FIXME: this is to force direct_nap radio ctrl to 0 for now. Proper fix is through
	 * rfseq tbls.
	 */
	if (TINY_RADIO(pi)) {
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, RfctrlCoreRxPus, core, fast_nap_bias_pu, 0);
			MOD_PHYREGCE(pi, RfctrlOverrideRxPus, core, fast_nap_bias_pu, 1);
		}
	}

	if ((TINY_RADIO(pi) ||
			ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev) ||
			ACMAJORREV_128(pi->pubpi->phy_rev)) &&
			(PHY_IPA(pi) || phy_papdcal_epacal(pi->papdcali))) {
		/* Zero out paplutselect table used in txpwrctrl */
		uint8 initvalue = 0;
		uint16 j;
		uint8 papdlutsel_table_ids[] = { ACPHY_TBL_ID_PAPDLUTSELECT0,
			ACPHY_TBL_ID_PAPDLUTSELECT1, ACPHY_TBL_ID_PAPDLUTSELECT2,
			ACPHY_TBL_ID_PAPDLUTSELECT3};

		for (j = 0; j < 128; j++) {
			if (ACMAJORREV_129_130(pi->pubpi->phy_rev)) {
				FOREACH_CORE(pi, core) {
					wlc_phy_table_write_acphy(pi,
					papdlutsel_table_ids[core], 1, j, 8, &initvalue);
				}
			} else {
				wlc_phy_table_write_acphy(pi,
				ACPHY_TBL_ID_PAPDLUTSELECT0, 1, j, 8, &initvalue);
				if (ACMAJORREV_51_131(pi->pubpi->phy_rev) ||
					(ACMAJORREV_4(pi->pubpi->phy_rev) &&
					(phy_get_phymode(pi) == PHYMODE_MIMO))) {
					wlc_phy_table_write_acphy(pi,
					ACPHY_TBL_ID_PAPDLUTSELECT1, 1, j, 8, &initvalue);
				}
			}
		}
	}

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		// still empty here
	}

	if (ACMAJORREV_GE47(pi->pubpi->phy_rev) && !ACMAJORREV_128(pi->pubpi->phy_rev)) {
		ACPHY_REG_LIST_START
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlCmd, 0x306)
			WRITE_PHYREG_ENTRY(pi, AfeClkDivOverrideCtrl, 0x1)
			ACPHYREG_BCAST_ENTRY(pi, AfeClkDivOverrideCtrlN0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlIntc0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideTxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideRxPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideGains0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfCT0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLpfSwtch0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideExtraAfeDivCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideETCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLowPwrCfg0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideAuxTssi0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideLogenBias0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, RfctrlOverrideNapPus0, 0x0)
			ACPHYREG_BCAST_ENTRY(pi, AfectrlOverride0, 0x0)
			WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0x4)
			//using phy override
			WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0xd)
			MOD_PHYREG_ENTRY(pi, RfctrlCmd, chip_pu, 1)
		ACPHY_REG_LIST_EXECUTE(pi);

		if (ACMAJORREV_GE47(pi->pubpi->phy_rev)) {
			ACPHYREG_BCAST(pi, RfctrlOverrideAfeDivCfg0, 0);
		} else {
			ACPHYREG_BCAST(pi, RfctrlOverrideAfeDivCfg0, 0x8f0b);   // in tcl it is 0
		}

		/* BFD HW reset for phyrev 51(63178/47622), 129 (6710, ...), 130 (6715),
		 * 131 (6756)
		 */
		if (ACMAJORREV_51_129_130_131(pi->pubpi->phy_rev)) {
			uint32 bfdIdxLut_zeros = 0;
			uint16 i;
			uint16 n = ACMAJORREV_51_131(pi->pubpi->phy_rev) ? 228 : 232;
			MOD_PHYREG(pi, bfdsConfig, bfdReset, 1);
			for (i = 0; i < n; i++) {
				wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_BFDINDEXLUT, 1,
					i, 32, &bfdIdxLut_zeros);
			}
		}

		// Temporary disable 11b/ag txerror check
		// TODO: Already in wlc_phy_set_reg_on_reset_majorrev32_33_37_47_51_129_131
		if (ACMAJORREV_47(pi->pubpi->phy_rev) || ACMAJORREV_129(pi->pubpi->phy_rev)) {
			MOD_PHYREG(pi, phytxerrorMaskReg3, miscmask, 0);
		}

		if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			/*  HW43684-555: PayDecode Hang, either need this OR PHY_CTL[15],
				i.e. ihrp to enable demod clocks
			*/
			MOD_PHYREG(pi, PHY1_Clock_Root_Gating_Control,
			           PHY1_demod_root_gating_enable, 0);
		}

		/* set rfseq_bundle_tbl {0x01C7034 0x01C7014 0x02C703E 0x02C701C} */
		/* acphy_write_table RfseqBundle $rfseq_bundle_tbl 0 */
		/* acphy_write_table RfseqBundle */
		/* {0x01C703C 0x01C7014 0x02C700E 0x02C702C 0x0206020 0x400 0x0} 0; */
		rfseq_bundle_48[0] = 0x703c;
		rfseq_bundle_48[1] = 0x1c;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 0, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x7014;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 1, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x700e;
		rfseq_bundle_48[1] = 0x2c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 2, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x702c;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 3, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x6020;
		rfseq_bundle_48[1] = 0x20;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 4, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x0400;
		rfseq_bundle_48[1] = 0x00;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 5, 48,
				rfseq_bundle_48);
		rfseq_bundle_48[0] = 0x0000;
		rfseq_bundle_48[1] = 0x00;
		rfseq_bundle_48[2] = 0x0;
		wlc_phy_table_write_acphy(pi, ACPHY_TBL_ID_RFSEQBUNDLE, 1, 6, 48,
				rfseq_bundle_48);

	}
}

static void  wlc_phy_set_clb_femctrl_static_settings(phy_info_t *pi)
{
	uint16 swctrl2glinesAnt0 = 0, swctrl2glinesAnt1 = 0;

	/* CLB FEM priority static settings */
	/* Only 2G switch control lines should allow the BT prisel mux logic */
	if (PHYCORENUM((pi)->pubpi->phy_corenum) >= 2) {
		swctrl2glinesAnt0 = pi->u.pi_acphy->sromi->nvram_femctrl_clb.map_2g[0][0] |
			pi->u.pi_acphy->sromi->nvram_femctrl_clb.map_2g[1][0];
		swctrl2glinesAnt1 = pi->u.pi_acphy->sromi->nvram_femctrl_clb.map_2g[0][1] |
			pi->u.pi_acphy->sromi->nvram_femctrl_clb.map_2g[1][1];
	}

	/* 4347_clb_reg_write clb_swctrl_smask_coresel_ant0_en 1 */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x1<<29, 0x1<<29);
	/* 4347_clb_reg_write clb_swctrl_smask_coresel_ant1_en 1 */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x1<<30, 0x1<<30);

	/* clb_swctrl_dmask_bt_ant0[9:0]: enable BT mux logic on 2G switch control lines */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x3ff<<17,
		(0x3ff & ~swctrl2glinesAnt0)<<17);

	/* disable BT AoA override by default; uCode to enable dynamically for AoA grants */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, 0x3<<18, 0);
	/* clb_swctrl_smask_wlan_ant0[9:0] */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_07, 0x3ff<<20,
		pi->u.pi_acphy->sromi->clb_swctrl_smask_ant0 <<20);

	/* clb_swctrl_dmask_bt_ant1[9:0]: enable BT mux logic on 2G switch control lines */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_10, 0x3ff<<10,
		(0x3ff & ~swctrl2glinesAnt1)<<10);

	/* clb_swctrl_smask_wlan_ant1[9:0] */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_11, 0x3ff<<0,
		pi->u.pi_acphy->sromi->clb_swctrl_smask_ant1 <<0);

	/* program btc prisel mask (main/aux) */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_08, 0x3<<27,
		pi->u.pi_acphy->sromi->nvram_femctrl_clb.btc_prisel_mask<<27);

	/* program btc prisel ant mask */
	si_gci_chipcontrol(pi->sh->sih, CC_GCI_CHIPCTRL_11, 0x3<<26,
		pi->u.pi_acphy->sromi->nvram_femctrl_clb.btc_prisel_ant_mask<<26);

	/* Force WLAN PRISEL - otherwise for 4347B0 Aux-2G, BT stays in control.
	* This is causing incorrect lna2g_reg1 values. Also these lna2g_reg1 values
	* could not be changed using radio override bits.
	* Force WLAN priority until the priority control issue is resolved
	*/
	if (ACMAJORREV_128(pi->pubpi->phy_rev) && ACMINORREV_1(pi)) {
		/* Force WLAN antenna and priority */
		wlc_phy_btcx_override_enable(pi);
	}
}

void
wlc_phy_radio2069_pwrdwn_seq(phy_ac_radio_info_t *ri)
{
	phy_info_t *pi = ri->pi;

	/* AFE */
	ri->afeRfctrlCoreAfeCfg10 = READ_PHYREG(pi, RfctrlCoreAfeCfg10);
	ri->afeRfctrlCoreAfeCfg20 = READ_PHYREG(pi, RfctrlCoreAfeCfg20);
	ri->afeRfctrlOverrideAfeCfg0 = READ_PHYREG(pi, RfctrlOverrideAfeCfg0);
	ACPHY_REG_LIST_START
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreAfeCfg10, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreAfeCfg20, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideAfeCfg0, 0x1fff)
	ACPHY_REG_LIST_EXECUTE(pi);

	/* Radio RX */
	ri->rxRfctrlCoreRxPus0 = READ_PHYREG(pi, RfctrlCoreRxPus0);
	ri->rxRfctrlOverrideRxPus0 = READ_PHYREG(pi, RfctrlOverrideRxPus0);
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, 0x40);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, 0xffbf);

	/* Radio TX */
	ri->txRfctrlCoreTxPus0 = READ_PHYREG(pi, RfctrlCoreTxPus0);
	ri->txRfctrlOverrideTxPus0 = READ_PHYREG(pi, RfctrlOverrideTxPus0);
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, 0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, 0x3ff);

	/* {radio, rfpll, pllldo}_pu = 0 */
	ri->radioRfctrlCmd = READ_PHYREG(pi, RfctrlCmd);
	ri->radioRfctrlCoreGlobalPus = READ_PHYREG(pi, RfctrlCoreGlobalPus);
	ri->radioRfctrlOverrideGlobalPus = READ_PHYREG(pi, RfctrlOverrideGlobalPus);
	ACPHY_REG_LIST_START
		MOD_PHYREG_ENTRY(pi, RfctrlCmd, chip_pu, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlCoreGlobalPus, 0)
		WRITE_PHYREG_ENTRY(pi, RfctrlOverrideGlobalPus, 0x1)
	ACPHY_REG_LIST_EXECUTE(pi);
}

void
wlc_phy_radio2069_pwrup_seq(phy_ac_radio_info_t *ri)
{
	phy_info_t *pi = ri->pi;

	/* AFE */
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg10, ri->afeRfctrlCoreAfeCfg10);
	WRITE_PHYREG(pi, RfctrlCoreAfeCfg20, ri->afeRfctrlCoreAfeCfg20);
	WRITE_PHYREG(pi, RfctrlOverrideAfeCfg0, ri->afeRfctrlOverrideAfeCfg0);

	/* Restore Radio RX */
	WRITE_PHYREG(pi, RfctrlCoreRxPus0, ri->rxRfctrlCoreRxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideRxPus0, ri->rxRfctrlOverrideRxPus0);

	/* Radio TX */
	WRITE_PHYREG(pi, RfctrlCoreTxPus0, ri->txRfctrlCoreTxPus0);
	WRITE_PHYREG(pi, RfctrlOverrideTxPus0, ri->txRfctrlOverrideTxPus0);

	/* {radio, rfpll, pllldo}_pu = 0 */
	WRITE_PHYREG(pi, RfctrlCmd, ri->radioRfctrlCmd);
	WRITE_PHYREG(pi, RfctrlCoreGlobalPus, ri->radioRfctrlCoreGlobalPus);
	WRITE_PHYREG(pi, RfctrlOverrideGlobalPus, ri->radioRfctrlOverrideGlobalPus);
}

void
wlc_acphy_get_radio_loft(phy_info_t *pi,
	uint8 *ei0,
	uint8 *eq0,
	uint8 *fi0,
	uint8 *fq0)
{
	/* Not required for 4345 */
	*ei0 = 0;
	*eq0 = 0;
	*fi0 = 0;
	*fq0 = 0;
}

void
wlc_acphy_set_radio_loft(phy_info_t *pi,
	uint8 ei0,
	uint8 eq0,
	uint8 fi0,
	uint8 fq0)
{
	/* Not required for 4345 */
	return;
}

void
wlc_phy_radio20693_config_bf_mode(phy_info_t *pi, uint8 core)
{
	uint8 pupd, band_sel;
	uint8 afeBFpu0 = 0, afeBFpu1 = 0, afeclk_div12g_pu = 1;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (wlapi_txbf_enab(pi->sh->physhim)) {
		if (phy_get_phymode(pi) == PHYMODE_MIMO) {
			if (core == 1) {
				afeBFpu0 = 0;
				afeBFpu1 = 1;
				afeclk_div12g_pu = 0;
			} else {
				afeBFpu0 = 1;
				afeBFpu1 = 0;
				afeclk_div12g_pu = 1;
			}
		}
		pupd = 1;
	} else {
		pupd = 0;
	}

	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu0, afeBFpu0);
	MOD_RADIO_REG_20693(pi, SPARE_CFG8, core, afe_BF_pu1, afeBFpu1);
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, afe_clk_div_12g_pu, afeclk_div12g_pu);
	MOD_RADIO_REG_20693(pi, CLK_DIV_OVR1, core, ovr_afeclkdiv_12g_pu, 0x1);

	band_sel = 0;

	if (CHSPEC_IS2G(pi->radio_chanspec))
		band_sel = 1;

	/* Set the 2G divider PUs */

	/* MOD_RADIO_REG_20693(pi, RXMIX2G_CFG1, core, rxmix2g_pu, 1); */
	MOD_RADIO_REG_20693(pi, TX_LOGEN2G_CFG1, core, logen2g_tx_pu_bias, (pupd & band_sel));
	/* Set the corresponding 2G Ovrs */
	/* MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST2, core, ovr_rxmix2g_pu, 1); */
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core, ovr_logen2g_tx_pu_bias,
		(pupd & band_sel));

	/* Set the 5G divider PUs */
	/* MOD_RADIO_REG_20693(pi, LNA5G_CFG3, core, mix5g_en, (pupd & (1-band_sel))); */
	MOD_RADIO_REG_20693(pi, TX_LOGEN5G_CFG1, core, logen5g_tx_enable_5g_low_band,
		(pupd & (1-band_sel)));
	/* Set the corresponding 5G Ovrs */
	/* MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_mix5g_en, (pupd & (1-band_sel))); */
	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_logen5g_tx_enable_5g_low_band,
		(pupd & (1-band_sel)));
}

static void
wlc_phy_radio20693_mimo_cm1_lpopt(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi), temp;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	/* Turn off core 1 PMU + other lp optimizations */
	ASSERT(!porig->is_orig);
	porig->is_orig = TRUE;
	PHY_TRACE(("%s: Applying core 0 optimizations\n", __FUNCTION__));

	wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(pi);

	RADIO_REG_LIST_START
		/* Core 0 Settings */
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_lowquiescenten,
			0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 0, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, PLL_CP1, 0, rfpll_cp_bias_low_power, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, VREG_CFG, 0, vreg25_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, VREG_OVR1, 0, ovr_vreg25_pu, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);

	/* MEM opt. Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
	   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
	 */
	temp = READ_RADIO_REG_TINY(pi, PMU_OP, 0);
	phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 0), (temp | 0x009E));

	RADIO_REG_LIST_START
		MOD_RADIO_REG_20693_ENTRY(pi, PMU_CFG4, 0, wlpmu_ADCldo_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG2, 0, logencore_det_stg1_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR1, 0, ovr_logencore_det_stg1_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG3, 0, logencore_2g_mimodes_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimodes_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_CFG3, 0, logencore_2g_mimosrc_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 0, ovr_logencore_2g_mimosrc_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_CFG1, 0, afe_clk_div_6g_mimo_pu, 0x0)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_6g_mimo_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_CFG1, 0, afe_clk_div_12g_pu, 0x1)
		MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 0, ovr_afeclkdiv_12g_pu, 0x1)
	RADIO_REG_LIST_EXECUTE(pi, 0);

	MOD_RADIO_REG_20693(pi, SPARE_CFG2, 0, afe_clk_div_se_drive, 0x0);
	/* Power down core0 MIMO LO buffer */
	MOD_RADIO_REG_20693(pi, LOGEN_OVR2, 0, ovr_logencore_5g_mimosrc_pu, 0x1);
	MOD_RADIO_REG_20693(pi, LOGEN_CFG3, 0, logencore_5g_mimosrc_pu, 0x0);

	if (phy_ac_chanmgr_get_data(pi_ac->chanmgri)->both_txchain_rxchain_eq_1 == TRUE) {
		PHY_TRACE(("%s: Applying core 1 optimizations\n", __FUNCTION__));
		RADIO_REG_LIST_START
			/* Core 1 Settings */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_lowquiescenten, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_pull_down_sw, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, AUXPGA_OVR1, 1, ovr_auxpga_i_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, VREG_CFG, 1, vreg25_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, VREG_OVR1, 1, ovr_vreg25_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, 1);

		/* MEM opt. UN-Setting wlpmu_en, wlpmu_VCOldo_pu, wlpmu_TXldo_pu,
		   wlpmu_AFEldo_pu, wlpmu_RXldo_pu
		 */
		temp = READ_RADIO_REG_TINY(pi, PMU_OP, 1);
		phy_utils_write_radioreg(pi, RADIO_REG_20693(pi, PMU_OP, 1), (temp & ~0x009E));

		RADIO_REG_LIST_START
			MOD_RADIO_REG_20693_ENTRY(pi, PMU_CFG4, 1, wlpmu_ADCldo_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR1, 1, ovr_logencore_det_stg1_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 1,
				ovr_logencore_2g_mimodes_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, LOGEN_OVR2, 1,
				ovr_logencore_2g_mimosrc_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 1,
				ovr_afeclkdiv_6g_mimo_pu, 0x1)
			MOD_RADIO_REG_20693_ENTRY(pi, CLK_DIV_OVR1, 1, ovr_afeclkdiv_12g_pu, 0x1)
			/* Power down core1 XTAL LDO */
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTALLDO1, 1,
				ldo_1p2_xtalldo1p2_BG_pu, 0x0)
			MOD_RADIO_REG_20693_ENTRY(pi, PLL_XTAL_OVR1, 1,
				ovr_ldo_1p2_xtalldo1p2_BG_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, 1);
	}
}
static void wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	uint8 core;
	uint16 *ptr_start = (uint16 *)&porig->pll_xtalldo1[0];

	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PLL_CP1, core),
			RADIO_REG_20693(pi, VREG_CFG, core),
			RADIO_REG_20693(pi, VREG_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
			RADIO_REG_20693(pi, LOGEN_CFG2, core),
			RADIO_REG_20693(pi, LOGEN_OVR1, core),
			RADIO_REG_20693(pi, LOGEN_CFG3, core),
			RADIO_REG_20693(pi, LOGEN_OVR2, core),
			RADIO_REG_20693(pi, CLK_DIV_CFG1, core),
			RADIO_REG_20693(pi, CLK_DIV_OVR1, core),
			RADIO_REG_20693(pi, SPARE_CFG2, core),
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->tx_top_2g_ovr_east[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}
	}
}
void wlc_phy_radio20693_mimo_lpopt_restore(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	ASSERT(!((pmu_lp_opt_orig->is_orig == TRUE) && (porig->is_orig == TRUE)));

	if (porig->is_orig == TRUE) {
		wlc_phy_radio20693_mimo_cm1_lpopt_restore(pi);
	} else if (pmu_lp_opt_orig->is_orig == TRUE) {
		wlc_phy_radio20693_mimo_cm23_lp_opt_restore(pi);
	}
}
static void wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	uint16 *ptr_start = (uint16 *)&pmu_lp_opt_orig->pll_xtalldo1[0];

	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_OVR, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &pmu_lp_opt_orig->pmu_cfg4[PHY_CORE_MAX-1]);
			*ptr = _READ_RADIO_REG(pi, porig_offsets[ct]);
		}
	}

}

static void
wlc_phy_radio20693_mimo_cm23_lp_opt(phy_info_t *pi, uint8 coremask)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);

	ASSERT(phy_get_phymode(pi) == PHYMODE_MIMO);
	ASSERT(coremask != 1);
	ASSERT(!pmu_lp_opt_orig->is_orig);
	pmu_lp_opt_orig->is_orig = TRUE;

	PHY_TRACE(("%s: Applying CM2/CM3 radio LP optimizations for coremask : %d\n",
		__FUNCTION__, coremask));

	wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(pi);

	/* Power down core1 XTAL LDO */
	MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, 1, ldo_1p2_xtalldo1p2_BG_pu, 0x0);
	MOD_RADIO_REG_20693(pi, PLL_XTAL_OVR1, 1, ovr_ldo_1p2_xtalldo1p2_BG_pu, 0x1);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20693(pi, PLL_XTALLDO1, core, ldo_1p2_xtalldo1p2_pull_down_sw, 0x0);
	}
}

static void
wlc_phy_radio20693_mimo_cm23_lp_opt_restore(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	uint16 *ptr_start = (uint16 *)&pmu_lp_opt_orig->pll_xtalldo1[0];

	ASSERT(phy_get_phymode(pi) == PHYMODE_MIMO);
	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	if (pmu_lp_opt_orig->is_orig == TRUE) {
		pmu_lp_opt_orig->is_orig = FALSE;
		PHY_TRACE(("%s: Restoring the radio lp settings\n", __FUNCTION__));

		FOREACH_CORE(pi, core) {
			uint8 ct;
			uint16 *ptr;
			uint16 porig_offsets[] = {
				RADIO_REG_20693(pi, PLL_XTALLDO1, core),
				RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
				RADIO_REG_20693(pi, PMU_OP, core),
				RADIO_REG_20693(pi, PMU_OVR, core),
				RADIO_REG_20693(pi, PMU_CFG4, core),
			};
			/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
			   the data access logic goes beyond this length to access next set of
			   elements, assuming contiguous memory allocation.
			   So, ARRAY OVERRUN is intentional here
			 */
			for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
				ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
				ASSERT(ptr <= &pmu_lp_opt_orig->pmu_cfg4[PHY_CORE_MAX-1]);
				phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
			}
		}
		/* JIRA: SW4349-1462:
		 * fix for '[4355] idle tssi for core 0 is coming as zero
		 * when txchain/rxchain are 2'
		 */
		wlc_phy_radio20693_minipmu_pwron_seq(pi);
	}
}

static void
wlc_phy_radio20693_mimo_cm1_lpopt_restore(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	uint16 *ptr_start = (uint16 *)&porig->pll_xtalldo1[0];
	uint8 core;

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);
	ASSERT(ISALIGNED(ptr_start, sizeof(uint16)));

	ASSERT(porig->is_orig);
	porig->is_orig = FALSE;
	PHY_TRACE(("%s: Restoring the core0/1 optimizations to their def values\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		uint8 ct;
		uint16 *ptr;
		uint16 porig_offsets[] = {
			RADIO_REG_20693(pi, PLL_XTALLDO1, core),
			RADIO_REG_20693(pi, PLL_XTAL_OVR1, core),
			RADIO_REG_20693(pi, PLL_CP1, core),
			RADIO_REG_20693(pi, VREG_CFG, core),
			RADIO_REG_20693(pi, VREG_OVR1, core),
			RADIO_REG_20693(pi, PMU_OP, core),
			RADIO_REG_20693(pi, PMU_CFG4, core),
			RADIO_REG_20693(pi, LOGEN_CFG2, core),
			RADIO_REG_20693(pi, LOGEN_OVR1, core),
			RADIO_REG_20693(pi, LOGEN_CFG3, core),
			RADIO_REG_20693(pi, LOGEN_OVR2, core),
			RADIO_REG_20693(pi, CLK_DIV_CFG1, core),
			RADIO_REG_20693(pi, CLK_DIV_OVR1, core),
			RADIO_REG_20693(pi, SPARE_CFG2, core),
			RADIO_REG_20693(pi, AUXPGA_OVR1, core),
			RADIO_REG_20693(pi, TX_TOP_2G_OVR_EAST, core)
		};

		/* Here ptr is pointing to a PHY_CORE_MAX length array of uint16s. However,
		   the data access logis goec beyond this length to access next set of elements,
		   assuming contiguous memory allocation. So, ARRAY OVERRUN is intentional here
		 */
		for (ct = 0; ct < ARRAYSIZE(porig_offsets); ct++) {
			ptr = ptr_start + ((ct * PHY_CORE_MAX) + core);
			ASSERT(ptr <= &porig->tx_top_2g_ovr_east[PHY_CORE_MAX-1]);
			phy_utils_write_radioreg(pi, porig_offsets[ct], *ptr);
		}
	}
	wlc_phy_radio20693_minipmu_pwron_seq(pi);
	wlc_phy_tiny_radio_minipmu_cal(pi);
	wlc_phy_radio20693_pmu_pll_config(pi);
	wlc_phy_radio20693_afecal(pi);
}

void wlc_phy_radio20693_mimo_core1_pmu_restore_on_bandhcange(phy_info_t *pi)
{
	uint16 phymode = phy_get_phymode(pi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	acphy_pmu_core1_off_radregs_t *porig = (pi_ac->radioi->pmu_c1_off_info_orig);
	acphy_pmu_mimo_lp_opt_radregs_t *pmu_lp_opt_orig = (pi_ac->radioi->pmu_lp_opt_orig);
	bool for_2g = ((pi_ac->radioi->lpmode_2g != ACPHY_LPMODE_NONE) &&
		(CHSPEC_IS2G(pi->radio_chanspec)));
	bool for_5g = ((pi_ac->radioi->lpmode_5g != ACPHY_LPMODE_NONE) &&
		(CHSPEC_ISPHY5G6G(pi->radio_chanspec)));

	BCM_REFERENCE(phymode);
	ASSERT(phymode == PHYMODE_MIMO);

	if (((for_2g == TRUE) || (for_5g == TRUE))) {
		if (porig->is_orig == TRUE) {
			wlc_phy_radio20693_mimo_cm1_lpopt_saveregs(pi);
		} else if (pmu_lp_opt_orig->is_orig == TRUE) {
			wlc_phy_radio20693_mimo_cm23_lp_opt_saveregs(pi);
		}
	}
}

static void wlc_phy_radio20693_tx2g_set_freq_tuning_ipa_as_epa(phy_info_t *pi,
	uint8 core, uint8 chan)
{
	uint8 mix2g_tune_tbl[] = { 0x4, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x2, 0x2, 0x2,
		0x2, 0x2, 0x1};
	uint8 mix2g_tune_val;
	ASSERT((chan >= 1) && (chan <= 14));
	mix2g_tune_val = mix2g_tune_tbl[chan - 1];
	MOD_RADIO_REG_20693(pi, TXMIX2G_CFG5, core,
		mx2g_tune, mix2g_tune_val);
}

void
phy_ac_dsi_radio_fns(phy_info_t *pi)
{
	wlc_phy_radio2069_mini_pwron_seq_rev16(pi);
	wlc_phy_radio2069_pwron_seq(pi);
	wlc_phy_radio2069_vcocal(pi);
	wlc_phy_radio2069x_vcocal_isdone(pi, TRUE, FALSE);
}

static void
chanspec_setup_radio_20693(phy_info_t *pi)
{
	uint8 core = 0;
	const chan_info_radio20693_pll_t *chan_info_pll;
	const chan_info_radio20693_rffe_t *chan_info_rffe;
	const chan_info_radio20693_pll_wave2_t *chan_info_pll_wave2;
	chanspec_t chspec[NUM_CHANS_IN_CHAN_BONDING];
	uint8 n_freq_seg, chan0;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	phy_ac_chanmgr_get_operating_chanspecs(pi, chspec);
	chan0 = CHSPEC_CHANNEL(chspec[0]);

	pi_ac->radioi->data->fc = wlc_phy_chan2freq_20693(pi, chan0, &chan_info_pll,
		&chan_info_rffe, &chan_info_pll_wave2);

	if (pi_ac->radioi->data->fc >= 0) {
		if (RADIOMAJORREV(pi) == 3) {
			if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
				PHY_INFORM(("wl%d: %s: setting chspec1=0x%04x, chspec2=0x%04x\n",
						pi->sh->unit, __FUNCTION__, chspec[0], chspec[1]));
				if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac)) {
					// FIXME check if PLL-1 is on, bypass this
					phy_ac_radio_20693_pmu_pll_config_wave2(pi, 7);
				}
				/* setting both pll cores - mode = 1 */
				phy_ac_radio_20693_chanspec_setup(pi, chan0,
					CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0, 1);
			} else if (CHSPEC_IS160(pi->radio_chanspec)) {
				ASSERT(0);
			} else {
				phy_ac_radio_20693_chanspec_setup(pi, chan0,
					CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0, 0);
			}
			if (CCT_INIT(pi_ac) || CCT_BW_CHG_80P80(pi_ac)) {
				if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
					wlc_phy_radio20693_afe_clkdistribtion_mode(pi, 1);
				} else {
					wlc_phy_radio20693_afe_clkdistribtion_mode(pi, 0);
				}
			}
		} else {
			if (phy_get_phymode(pi) == PHYMODE_80P80) {
				for (n_freq_seg = 0; n_freq_seg < NUM_CHANS_IN_CHAN_BONDING;
						n_freq_seg++) {
					phy_ac_radio_20693_chanspec_setup(pi,
						CHSPEC_CHANNEL(chspec[n_freq_seg]),
						CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac),
						n_freq_seg, 0);
				}
			} else {
				FOREACH_CORE(pi, core) {
					phy_ac_radio_20693_chanspec_setup(pi, chan0,
						CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), core, 0);
				}
			}
		}
	}
}

static void
chanspec_setup_radio_2069(phy_info_t *pi)
{
	chanspec_t chspec[NUM_CHANS_IN_CHAN_BONDING];
	const void *chan_info = NULL;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	phy_ac_chanmgr_get_operating_chanspecs(pi, chspec);
	pi_ac->radioi->data->fc = wlc_phy_chan2freq_acphy(pi,
		CHSPEC_CHANNEL(chspec[0]), &chan_info);

	if (pi_ac->radioi->data->fc >= 0) {
		wlc_phy_chanspec_radio2069_setup(pi, chan_info,
			CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac));
	}
}

static void
chanspec_tune_radio_20693(phy_info_t *pi)
{
	uint16 dac_rate;
	uint8 lpf_gain = 1, lpf_bw, biq_bw_ofdm, biq_bw_cck, core;

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		/* FIXME: hard-code DAC rate mode for 4365 to be 1
		 * (needs to come from SROM)
		 */
		pi->u.pi_acphy->radioi->data->dac_mode = 1;
	} else {
		pi_ac->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec) ?
			pi->dacratemode2g : pi->dacratemode5g;
		wlc_phy_dac_rate_mode_acphy(pi, pi_ac->radioi->data->dac_mode);
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac)) {
				/* bw_change requires afe cal. */
				/* so that all the afe_iqadc signals are correctly set */
				wlc_phy_radio_afecal(pi);
			}
		}
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				lpf_bw = 1;
				lpf_gain = -1;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else {
				lpf_bw = 3;
				lpf_gain = -1;
			}
		} else {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				lpf_bw = 3;
				lpf_gain = -1;
			} else {
				lpf_bw = 3;
				lpf_gain = -1;
			}
		}
	} else {
		dac_rate = wlc_phy_get_dac_rate_from_mode(pi, pi_ac->radioi->data->dac_mode);

		/* For 4349 do afecal on chan change to take care of RSDB dac spurs */
		wlc_phy_radio_afecal(pi);

		/* tune lpf setting for 20693 based on dac rate */
		if (!(PHY_IPA(pi))) {
			if (dac_rate == 200) {
				lpf_bw = 1;
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					lpf_bw = 1;
					lpf_gain = 1;
				} else {
					lpf_bw = 2;
					lpf_gain = 0;
				}
			} else if (dac_rate == 400) {
				lpf_bw = 3;
				lpf_gain = 0;
			} else {
				lpf_bw = 5;
				lpf_gain = 0;
			}
		} else {
			if (dac_rate == 200) {
				lpf_bw = 2;
				if (CHSPEC_IS2G(pi->radio_chanspec)) {
					lpf_gain = 2;
				} else {
					lpf_gain = 0;
				}
			} else if (dac_rate == 400) {
				lpf_bw = 3;
				lpf_gain = 0;
			} else {
				lpf_bw = 5;
				lpf_gain = 0;
			}
		}
	}

	if (!(PHY_IPA(pi)) && (RADIOREV(pi->pubpi->radiorev) == 13)) {
		lpf_gain = 0;
	} else {
		if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) &&
		RADIOMAJORREV(pi) == 3)) {
			lpf_gain = CHSPEC_IS2G(pi->radio_chanspec) ? 2 : 0;
		}
	}

	biq_bw_ofdm = lpf_bw;
	biq_bw_cck = lpf_bw - 1;

	/* WAR for FDIQI when bq_bw = 9, 25 MHz */
	if (!PHY_PAPDEN(pi) && !PHY_IPA(pi) && CHSPEC_IS2G(pi->radio_chanspec) &&
		CHSPEC_IS20(pi->radio_chanspec) && !(RADIOREV(pi->pubpi->radiorev) == 13)) {
		biq_bw_ofdm = lpf_bw - 1;
	}

	wlc_phy_radio_tiny_lpf_tx_set(pi, lpf_bw, lpf_gain, biq_bw_ofdm, biq_bw_cck);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		wlc_phy_radio20693_force_dacbuf_setting(pi);
	} else {
		/* dac swap */
		FOREACH_CORE(pi, core) {
			if (core == 0)
				MOD_PHYREG(pi, Core1TxControl, iqSwapEnable, 1);
			else if (core == 1)
				MOD_PHYREG(pi, Core2TxControl, iqSwapEnable, 1);
		}

		/* adc swap */
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq0, 1);
		MOD_PHYREG(pi, RxFeCtrl1, swap_iq1, 1);
	}

	OSL_DELAY(20);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && RADIOMAJORREV(pi) == 3) {
		if (PHY_AS_80P80(pi, pi->radio_chanspec)) {
			uint8 bands[NUM_CHANS_IN_CHAN_BONDING];
			phy_ac_chanmgr_get_chan_freq_range_80p80(pi, 0, bands);
			pi_ac->radioi->prev_subband = bands[0];
			PHY_INFORM(("wl%d: %s: FIXME for 80P80\n", pi->sh->unit, __FUNCTION__));
		} else {
			pi_ac->radioi->prev_subband =
				phy_ac_chanmgr_get_chan_freq_range(pi, 0, PRIMARY_FREQ_SEGMENT);
		}
	}
}

static void
chanspec_tune_radio_2069(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 tx_gain_tbl_id = wlc_phy_get_tbl_id_gainctrlbbmultluts(pi, 0);

	/* bw_change requires afe cal. */
	if (CCT_INIT(pi_ac) || CCT_BW_CHG(pi_ac))
		wlc_phy_radio_afecal(pi);

	/* Applicable for non-TINY radios */
	if (BF3_VLIN_EN_FROM_NVRAM(pi_ac)) {
		uint16 txidxval, txgaintemp1[3], txgaintemp1a[3];
		uint16 tempmask;
		uint16 vlinmask;
		if (pi_ac->radioi->prev_subband != 15) {
			for (txidxval = phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlin_txidx;
					txidxval < 128; txidxval++) {
				wlc_phy_table_read_acphy(pi, tx_gain_tbl_id, 1,
						txidxval, 48, &txgaintemp1);
				txgaintemp1a[0] = (txgaintemp1[0] & 0x7FFF) -
					(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->bbmult_comp);
				txgaintemp1a[1] = txgaintemp1[1];
				txgaintemp1a[2] = txgaintemp1[2];
				wlc_phy_tx_gain_table_write_acphy(pi_ac->tbli, 1,
						txidxval, 48, txgaintemp1a);
			}
		}
		tempmask = READ_PHYREGFLD(pi, FemCtrl, femCtrlMask);
		if (CHSPEC_IS2G (pi->radio_chanspec)) {
			vlinmask =
			1<<(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlinmask2g_from_nvram);
		}
		else {
			vlinmask =
			1<<(phy_ac_chanmgr_get_data(pi_ac->chanmgri)->vlinmask5g_from_nvram);
		}
		MOD_PHYREG(pi, FemCtrl, femCtrlMask, (tempmask|vlinmask));
		wlc_phy_vlin_en_acphy(pi);
	}
	pi_ac->radioi->prev_subband =
		phy_ac_chanmgr_get_chan_freq_range(pi, 0, PRIMARY_FREQ_SEGMENT);
}

static void
chanspec_setup_radio_20698(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20698_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0);
}

static void
chanspec_setup_radio_20704(phy_info_t *pi)
{

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20704_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0);
}

static void
chanspec_setup_radio_20707(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20707_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0);
}

static void
chanspec_setup_radio_20708(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 pll_num = pi_ac->radioi->maincore_on_pll1 ? 1 : 0;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20708_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), pll_num, 0);
}

static void
chanspec_setup_radio_20709(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20709_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0);
}

static void
chanspec_setup_radio_20710(phy_info_t *pi)
{

	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	/* Do resetcca to send out band_sel signal */
	wlc_phy_resetcca_acphy(pi);

	wlc_phy_chanspec_radio20710_setup(pi, pi->radio_chanspec,
		CCT_BAND_CHG(pi_ac) | CCT_BW_CHG(pi_ac), 0);
}

void
chanspec_setup_radio(phy_info_t *pi)
{
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))
		chanspec_setup_radio_20693(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID))
		chanspec_setup_radio_2069(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID))
		chanspec_setup_radio_20698(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID))
		chanspec_setup_radio_20704(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID))
		chanspec_setup_radio_20707(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID))
		chanspec_setup_radio_20708(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID))
		chanspec_setup_radio_20709(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID))
		chanspec_setup_radio_20710(pi);
	else {
		PHY_ERROR(("wl%d %s: Invalid RADIOID! %d\n",
			PI_INSTANCE(pi), __FUNCTION__, pi->pubpi->radioid));
		ASSERT(0);
	}
}

void
chanspec_tune_radio(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;

	if (ISSIM_ENAB(pi->sh->sih) && ACMAJORREV_130(pi->pubpi->phy_rev)) {
		return;
	}

	/* PR114734
	 * parallel VCO cal to save the channel switch time
	 */
	if (pi_ac->radioi->data->fc >= 0) {
		if (ACMAJORREV_47(pi->pubpi->phy_rev)) {
			wlc_phy_radio20698_vcocal_done_check(pi, FALSE);
		} else if (ACMAJORREV_51(pi->pubpi->phy_rev)) {
			wlc_phy_radio20704_vcocal_isdone(pi, FALSE);
		} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			wlc_phy_radio20709_vcocal_isdone(pi, FALSE);
		} else if (ACMAJORREV_129(pi->pubpi->phy_rev)) {
			wlc_phy_radio20707_vcocal_isdone(pi, FALSE);
			wlc_phy_radio20707_vco_cal_sanity_check(pi);
		} else if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			wlc_phy_radio20708_vcocal_isdone(pi, FALSE, 0);
		} else if (ACMAJORREV_128(pi->pubpi->phy_rev)) {
			wlc_phy_radio20709_vcocal_isdone(pi, FALSE);
		} else if (ACMAJORREV_131(pi->pubpi->phy_rev)) {
			wlc_phy_radio20710_vcocal_isdone(pi, FALSE);
		} else {
			wlc_phy_radio2069x_vcocal_isdone(pi, FALSE, FALSE);
		}
	}

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID))
		chanspec_tune_radio_20693(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM2069_ID))
		chanspec_tune_radio_2069(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID))
		chanspec_tune_radio_20698(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID))
		chanspec_tune_radio_20704(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID))
		chanspec_tune_radio_20707(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID))
		chanspec_tune_radio_20708(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID))
		chanspec_tune_radio_20709(pi);
	else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID))
		chanspec_tune_radio_20710(pi);
	else {
		PHY_ERROR(("wl%d %s: Invalid RADIOID! %d\n",
			PI_INSTANCE(pi), __FUNCTION__, pi->pubpi->radioid));
		ASSERT(0);
	}

	/* If 6715 & 2g, trigger logen synch for implicit TXBF use case */
	/* Place this function here after logen and afe_cal is stablized */
	wlc_phy_logen_2g_sync(pi, FALSE);
}

static void
wlc_phy_logen_2g_sync(phy_info_t *pi, bool p1c_mode)
{
	/* If it's not "6715 in 2g", use early return */
	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20708_ID) &&
	      CHSPEC_IS2G(pi->radio_chanspec))) {
		return;
	}

	if (p1c_mode) {
		// 3p1 mode
		MOD_RADIO_REG_20708(pi, LOGEN_CORE3_OVR, 3, ovr_logen_Sopq, 0x1);
		MOD_RADIO_REG_20708(pi, LOGEN_CORE3, 3, logen_Sopq, 0x1);
	} else {
		// 4x4 mode
		MOD_RADIO_REG_20708(pi, LOGEN_CORE3_OVR, 3, ovr_logen_Sopq, 0x1);
		MOD_RADIO_REG_20708(pi, LOGEN_CORE3, 3, logen_Sopq, 0x0);
	}

	MOD_RADIO_REG_20708(pi, LOGEN_REG3, 0, logen_div2_set, 0x1);
	OSL_DELAY(1);
	MOD_RADIO_REG_20708(pi, LOGEN_REG3, 0, logen_div2_set, 0x0);
}

uint16
wlc_phy_get_dac_rate_from_mode(phy_info_t *pi, uint8 dac_rate_mode)
{
	uint16 dac_rate = 200;

	if (CHSPEC_BW_LE20(pi->radio_chanspec)) {
		switch (dac_rate_mode) {
			case 2:
				dac_rate = 600;
				break;
			case 3:
				dac_rate = 400;
				break;
			default: /* dac rate mode 1 */
				dac_rate = 200;
				break;
		}
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		switch (dac_rate_mode) {
			case 2:
				dac_rate = 600;
				break;
			default: /* dac rate mode 1 and 3 */
				dac_rate = 400;
				break;
		}
	} else {
		dac_rate = 600;
	}

	return (dac_rate);
}

void wlc_phy_dac_rate_mode_acphy(phy_info_t *pi, uint8 dac_rate_mode)
{
	uint8 bw_idx = 0;
	uint8 stall_val;
	uint16 sdfeClkGatingCtrl_orig;

	bw_idx = (CHSPEC_IS80(pi->radio_chanspec) || PHY_AS_80P80(pi, pi->radio_chanspec))
				? 2 : (CHSPEC_IS160(pi->radio_chanspec)) ? 3
				: (CHSPEC_IS40(pi->radio_chanspec)) ? 1 : 0;
	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	sdfeClkGatingCtrl_orig = READ_PHYREG(pi, sdfeClkGatingCtrl);
	if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 1);
		ACPHY_DISABLE_STALL(pi);
		WRITE_PHYREG(pi, sdfeClkGatingCtrl, 0xe);
		MOD_PHYREG(pi, fineclockgatecontrol, forceAfeClocksOff, 1);
	}

	if (dac_rate_mode == 2) {
		si_core_cflags(pi->sh->sih, 0x300, 0x200);
	} else if (dac_rate_mode == 3) {
		si_core_cflags(pi->sh->sih, 0x300, 0x300);
	} else {
		si_core_cflags(pi->sh->sih, 0x300, 0x100);
	}

	if (ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
		MOD_PHYREG(pi, RxFeCtrl1, soft_sdfeFifoReset, 0);
		ACPHY_ENABLE_STALL(pi, stall_val);
		WRITE_PHYREG(pi, sdfeClkGatingCtrl, sdfeClkGatingCtrl_orig);
		MOD_PHYREG(pi, fineclockgatecontrol, forceAfeClocksOff, 0);
	}

	if ((dac_rate_mode == 1) || (bw_idx == 2)) {
		MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr, 0);
		MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 0);
	} else {
		MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr, 1);
		if (bw_idx == 1)
			MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 2);
		else
			MOD_PHYREG(pi, sdfeClkGatingCtrl, txlbclkmode_ovr_value, 1);
	}

	if (TINY_RADIO(pi)) {
		wlc_phy_farrow_setup_tiny(pi, pi->radio_chanspec);
	} else {
		if (!ACMAJORREV_128(pi->pubpi->phy_rev) &&
			!ACMAJORREV_130_131(pi->pubpi->phy_rev)) {
			wlc_phy_farrow_setup_acphy(pi, pi->radio_chanspec);
		}
	}
}

#ifdef PHY_DUMP_BINARY
/* The function is forced to RAM since it accesses non-const tables */
static int BCMRAMFN(phy_ac_radio_getlistandsize_20693)(phy_info_t *pi,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	if (RADIO20693_MAJORREV(pi->pubpi->radiorev) == 2) {
		*radreglist = (phyradregs_list_t *) &rad20693_majorrev2_registers[0];
		*radreglist_sz = sizeof(rad20693_majorrev2_registers);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported BCM20693 radio rev %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radiorev));
		return BCME_UNSUPPORTED;
	}

	return BCME_OK;
}

static int phy_ac_radio_getlistandsize(phy_type_radio_ctx_t *ctx,
                    phyradregs_list_t **radreglist, uint16 *radreglist_sz)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	int ret = BCME_UNSUPPORTED;
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		ret = phy_ac_radio_getlistandsize_20693(pi, radreglist, radreglist_sz);
	} else {
		PHY_INFORM(("%s: wl%d: unsupported radio ID %d\n",
			__FUNCTION__,  pi->sh->unit,  pi->pubpi->radioid));
	}
	return ret;
}
#endif /* PHY_DUMP_BINARY */

void
phy_ac_radio_cal_init(phy_info_t *pi)
{
	int core;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;

	if (TINY_RADIO(pi)) {
		/* switch back to original rx2tx seq and dly for tiny cal */
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			wlapi_suspend_mac_and_wait(pi->sh->physhim);
			phy_utils_phyreg_enter(pi);
		}
		phy_ac_rfseq_mode_set(pi, 1);

		if (pi->u.pi_acphy->radioi->data->dac_mode != 1) {
			pi->u.pi_acphy->radioi->data->dac_mode = 1;
			wlc_phy_dac_rate_mode_acphy(pi, pi->u.pi_acphy->radioi->data->dac_mode);
		}

		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
			phy_utils_phyreg_exit(pi);
			wlapi_enable_mac(pi->sh->physhim);
		}
	} else {
		if ((ACMAJORREV_128(pi->pubpi->phy_rev) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en))) {
			/* switch to cal rx2tx seq and dly */
			phy_ac_rfseq_mode_set(pi, 1);
		}
		if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			/* Even though we tia_pu=1 during rxiqcal,but it shows instability at -40,FF
			   Keep it high before all calibrations fixes it. Exact root cause unknown.
			 */
			FOREACH_CORE(pi, core) {
			  ri->tia_cfg1_ovr[core] =
			    phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, TIA_CFG1_OVR, core));
			  ri->tia_reg7[core] =
			    phy_utils_read_radioreg(pi, RADIO_REG_20708(pi, TIA_REG7, core));

			  MOD_RADIO_REG_20708(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 1);
			  MOD_RADIO_REG_20708(pi, TIA_REG7, core, tia_pu, 1);
			}
		}
	}
}

void
phy_ac_radio_cal_reset(phy_info_t *pi, int16 idac_i, int16 idac_q)
{
	int core;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;

	if (TINY_RADIO(pi)) {
	/* switch to normal rx2tx seq and dly after tiny cal */
		phy_ac_rfseq_mode_set(pi, 0);
		if (pi->u.pi_acphy->radioi->data->dac_mode != (CHSPEC_IS2G(pi->radio_chanspec)
			? pi->dacratemode2g : pi->dacratemode5g)) {
			pi->u.pi_acphy->radioi->data->dac_mode = CHSPEC_IS2G(pi->radio_chanspec)
				? pi->dacratemode2g : pi->dacratemode5g;
			wlc_phy_dac_rate_mode_acphy(pi, pi->u.pi_acphy->radioi->data->dac_mode);
		}
	} else {
		if ((ACMAJORREV_128(pi->pubpi->phy_rev) &&
			(pi->u.pi_acphy->sromi->srom_low_adc_rate_en))) {
			/* switch to normal rx2tx seq and dly after cal */
			phy_ac_rfseq_mode_set(pi, 0);
		}
		if (ACMAJORREV_130(pi->pubpi->phy_rev)) {
			FOREACH_CORE(pi, core) {
			  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, TIA_CFG1_OVR, core),
			                           ri->tia_cfg1_ovr[core]);
			  phy_utils_write_radioreg(pi, RADIO_REG_20708(pi, TIA_REG7, core),
			                           ri->tia_reg7[core]);
			}
		}
	}

	if (pi->u.pi_acphy->c2c_sync_en) {
		/* Enable core2core sync */
		phy_ac_chanmgr_core2core_sync_setup(pi->u.pi_acphy->chanmgri, TRUE);
	}
}

static int
wlc_tiny_sigdel_fast_mult(int raw_p8, int mult_p12, int max_val, int rshift)
{
	int prod;
	/* >> 20 follows from .12 fixed-point for mult, various.8 for raw */
	prod = (mult_p12 * raw_p8) >> rshift;
	return (prod > max_val) ? max_val : prod;
}

static int
wlc_tiny_sigdel_wrap(int prod, int max_val)
{
	/* to make sure you hit the maximum number of bits in word allocated  */
	return (prod > max_val) ? max_val : prod;
}

#define WLC_TINY_GI_MULT_P12		4096U
#define WLC_TINY_GI_MULT_TWEAK_P12	4096U
#define WLC_TINY_GI_MULT		WLC_TINY_GI_MULT_P12
static void
wlc_tiny_sigdel_slow_tune(phy_info_t *pi, int g_mult_raw_p12,
	tiny_adc_tuning_array_t *gvalues, uint8 bw)
{
	int g_mult_p12;
	int ri = 0;
	int r21;
	int r32;
	int r43;
	int rff1;
	int rff2;
	int rff3;
	int rff4;
	int r12v;
	int r34v;
	int r11v;
	int g21;
	int g32;
	int g43;
	int r12;
	int r34;
	int g11;
	int temp;
	int gff3_val = 0;
	uint8 shift_val = 0;
	BCM_REFERENCE(pi);
	/* RC cals the slow ADC IN 40MHz channels or 20MHz bandwidth, based on g_mult */
	/* input signal scaling, changes ADC gain, 4096 <=> 1.0 for g_mult and gi_mult */
	/* Function is 32 bit (signed) integer arithmetic and a/b division rounding  */
	/* is performed in integers from: (a-1)/b+1 */

	/* ERR! 20691_rc_cal "adc" returns the RC value, so correction is 1/rccal! */
	/* so invert it */
	/* This is a nice way of inverting the number... jnh */
	/* inverse of gmult precomputed to minimise division operations for speed */
	/* 4.12 fixed point so scale reciprocal by 2^24 */
	g_mult_p12 = g_mult_raw_p12 > 0 ? g_mult_raw_p12 : 1;
	/* tweak to g_mult */
	g_mult_p12 = (WLC_TINY_GI_MULT_TWEAK_P12 * g_mult_p12) >> 12;

	/* to avoid divide by zeros and negative values */
	if (g_mult_p12 <= 0)
		g_mult_p12 = 1;

	if (bw == 20) {
	    shift_val = 1;
		ri = 10176;
		gff3_val = 32000;
	} else if (bw == 40) {
		shift_val = 0;
		ri = 10670;
		gff3_val = 12000;
	}

	/* RC cal in slow ADC is mostly of the form Runit/(Rval/(g_mult/2**12)-Roff). */
	/* For integer manipulation do Runit/({Rval*2**12}/gmult-Roff).  */
	/* where Rval*2**12 are res design values in matlab script {x kint234 , kr12, kr34} */
	/* but right shifted a number of times */
	/* All but r11 and rff4 resistances are x2 for 20MHz. */

	/* Rvals from matlab already scaled by kint234, kr12 */
	/* or kr34 (due to amplifier finite GBW) */
	/* x2 for half the BW and half the sampling frequency. */
	ri = ri << shift_val;
	r21 = 8323 << shift_val;
	r32 = 6390 << shift_val;
	r43 = 6827 << shift_val;
	rff1 = 19768 << shift_val;
	rff2 = 16916 << shift_val;
	rff3 = 29113  << shift_val;
	/* rff4 does not double with 20MHz channels, 10MHz BW. */
	rff4 = 100000;
	r12v = 83205 << shift_val;
	r34v = 243530 << shift_val;
	/* rff4 does not double with 20MHz channels, 10MHz BW. */
	r11v = 8000;

	/* saturate correctly when you get negative numbers and round divisions */
	/* subject to gmult twice so scale gmult back to 12b so it only divides with 12b+ r21 */
	g21 = (g_mult_p12 * g_mult_p12) >> 12;
	if (g21 <= 0)
		g21 = 1;
	g21 = ((r21 << 12) - 1) / g21 + 1;
	g21 = (512000 - 1) / g21 + 1;
	g21 = wlc_tiny_sigdel_wrap(g21, 127);
	g32 = (256000 - 1) / (((r32 << 12) - 1) / g_mult_p12 + 1) + 1;
	g32 = wlc_tiny_sigdel_wrap(g32, 127);
	g43 = (256000 - 1) / (((r43 << 12) - 1) / g_mult_p12 + 1) + 1;
	g43  = wlc_tiny_sigdel_wrap(g43, 127);

	/* gff1234 subject to gmult and gimult; step operations so range */
	/* is not exceeded and scale is correct */
	/* gff1234 will overflow if $g_mult_p12*$gi_mult < 1023*2, eq. to 0.25, */
	/* assuming rff1234 {<<2} < 131072 */

	/* gi */
	temp = (((ri << 12) - 1) / WLC_TINY_GI_MULT) - 4000 + 1;
	ASSERT(temp > 0);	/* should be a positive value */
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gi = (uint16) temp;

	/* gff1 */
	temp = ((((((rff1 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - 8000 + 1;
	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff1 = (uint16) temp;

	/* gff2 */
	temp = ((((((rff2 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - 4000 + 1;
	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff2 = (uint16) temp;

	/* gff3 */
	temp = ((((((rff3 << 12) - 1) / g_mult_p12 + 1) << 12) - 1) / WLC_TINY_GI_MULT) - gff3_val
	    + 1;

	if (temp <= 0)
		temp = 1;
	temp = (256000 - 1) / temp + 1;
	temp = wlc_tiny_sigdel_wrap(temp, 127);
	gvalues->gff3 = (uint16) temp;

	/* gff4 */
	temp = (((rff4 << 12) - 1) / WLC_TINY_GI_MULT) - 72000 + 1;	/* subject to gimult only */
	ASSERT(temp > 0);	/* should be a positive value */
	temp = (256000 - 1) / temp + 1;
	temp  = wlc_tiny_sigdel_wrap(temp, 255);
	gvalues->gff4 = (uint16) temp;

	/* stays constant to RC shifts, g21 shifts twice for it. */
	r12 = (r12v - 1) / 4000 + 1;
	r12 = wlc_tiny_sigdel_wrap(r12, 127);

	if (bw == 20)
		r34 = ((((r34v << 12) - 1) / g_mult_p12 +1) - 128000 - 1) / 4000 + 1;
	else
		r34 = ((r34v << 12) - 1) / g_mult_p12 / 4000 + 1;

	if (r34 <= 0)
		r34 = 1;
	r34 = wlc_tiny_sigdel_wrap(r34, 127);
	g11 = (((r11v << 12) - 1) / g_mult_p12 +1) - 2000;
	if (g11 <= 0)
		g11 = 1;
	g11 = (128000 - 1) / g11 + 1;
	g11 = wlc_tiny_sigdel_wrap(g11, 127);

	gvalues->g21 = (uint16) g21;
	gvalues->g32 = (uint16) g32;
	gvalues->g43 = (uint16) g43;
	gvalues->r12 = (uint16) r12;
	gvalues->r34 = (uint16) r34;
	gvalues->g11 = (uint16) g11;
	PHY_TRACE(("gi   = %i\n", gvalues->gi));
	PHY_TRACE(("g21  = %i\n", gvalues->g21));
	PHY_TRACE(("g32  = %i\n", gvalues->g32));
	PHY_TRACE(("g43  = %i\n", gvalues->g43));
	PHY_TRACE(("r12  = %i\n", gvalues->r12));
	PHY_TRACE(("r34  = %i\n", gvalues->r34));
	PHY_TRACE(("gff1 = %i\n", gvalues->gff1));
	PHY_TRACE(("gff2 = %i\n", gvalues->gff2));
	PHY_TRACE(("gff3 = %i\n", gvalues->gff3));
	PHY_TRACE(("gff4 = %i\n", gvalues->gff4));
	PHY_TRACE(("g11  = %i\n", gvalues->g11));
}

static void
wlc_tiny_sigdel_fast_tune(phy_info_t *pi, int g_mult_raw_p12, tiny_adc_tuning_array_t *gvalues)
{
	int g_mult_tweak_p12;
	int g_mult_p12;
	int g_inv_p12;
	int gi_inv_p12;
	int gi_p8;
	int ri3_p8;
	int g21_p8;
	int g32_p8;
	int g43_p8;
	int g54_p8;
	int g65_p8;
	int r12_p8;
	int r34_p8;
	int gi, ri3, g21, g32, g43, g54, g65, r12, r34;

	BCM_REFERENCE(pi);

	/* tweak to g_mult to trade off stability over PVT versus performance */
	g_mult_tweak_p12 = 4096;
	g_mult_p12 = (g_mult_tweak_p12 * g_mult_raw_p12) >> 12;

	/* inverse of gmult precomputed to minimise division operations for speed */
	g_inv_p12 = 16777216 / g_mult_p12;
	gi_inv_p12 = 16777216 / WLC_TINY_GI_MULT;

	/* untuned values in p8 fixed-point format, ie. multiplied by 2^8 */
	gi_p8 = 16384;
	ri3_p8 = 1477;
	g21_p8 = 17997;
	g32_p8 = 18341;
	g43_p8 = 15551;
	g54_p8 = 19915;
	g65_p8 = 12369;
	r12_p8 = 1156;
	r34_p8 = 4331;

	/* RC cal */
	gi = wlc_tiny_sigdel_fast_mult(gi_p8, WLC_TINY_GI_MULT, 127, 20);
	ri3 = wlc_tiny_sigdel_fast_mult(ri3_p8, gi_inv_p12, 63, 20);
	g21 = wlc_tiny_sigdel_fast_mult(g21_p8, g_mult_p12, 127, 20);
	g32 = wlc_tiny_sigdel_fast_mult(g32_p8, g_mult_p12, 127, 20);
	g43 = wlc_tiny_sigdel_fast_mult(g43_p8, g_mult_p12, 127, 20);
	g54 = wlc_tiny_sigdel_fast_mult(g54_p8, g_mult_p12, 127, 20);
	g65 = wlc_tiny_sigdel_fast_mult(g65_p8, g_mult_p12, 63, 20);
	r12 = wlc_tiny_sigdel_fast_mult(r12_p8, g_inv_p12, 63, 20);
	r34 = wlc_tiny_sigdel_fast_mult(r34_p8, g_inv_p12, 63, 20);

	gvalues->gi = (uint16) gi;
	gvalues->ri3 = (uint16) ri3;
	gvalues->g21 = (uint16) g21;
	gvalues->g32 = (uint16) g32;
	gvalues->g43 = (uint16) g43;
	gvalues->g54 = (uint16) g54;
	gvalues->g65 = (uint16) g65;
	gvalues->r12 = (uint16) r12;
	gvalues->r34 = (uint16) r34;
}

static void
wlc_tiny_adc_setup_slow(phy_info_t *pi, tiny_adc_tuning_array_t *gvalues, uint8 bw, uint8 core)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 adcclkdiv = 0x1;
	uint8 sipodiv = 0x1;

	ASSERT(TINY_RADIO(pi));

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);

	if (row >= 0) {
		adcclkdiv = altclkpln[row].adcclkdiv;
		sipodiv = altclkpln[row].sipodiv;
	}
	/* SETS UP THE slow ADC IN 20MHz channels or 10MHz bandwidth */
	/* Function should be 32 bit (signed) arithmetic */
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) && (bw == 20)) {
		if (RADIOMAJORREV(pi) != 3)
			MOD_RADIO_REG_20693(pi, SPARE_CFG2, core, adc_clk_slow_div, adcclkdiv);
	}

	/* Setup internal dividers and sipo for 1G2Hz mode */
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, sipodiv);
	/* set adc_clk_slow_div3 to 0x0 in 20MHz mode, 0x1 in 40MHz mode */
	if (bw == 20)
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x0);
	else if (bw == 40)
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x1);
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) && (bw == 40) &&
		(RADIOMAJORREV(pi) != 3))
		MOD_RADIO_REG_20693(pi, SPARE_CFG2, core, adc_clk_slow_div, adcclkdiv);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, sipodiv);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x0);

	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_pu, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);

	/* Setup biases */
	/* Slow adc halves opamp current from 20MHz to 40MHz channels */
	/* Opamp1 is 26u/0, 4u/2 other 3 are 26u/0, 4u/4,	*/
	/* so opamp1 (40M, 20M) = (0x20, 0x10), opamp234 = (0x10, 0x8) */
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp1, (0x10 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp2, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp3, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp4, (0x8 * (bw/20)));
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_ff_mult_opamp, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref4_control, 0x40);

	/* Setup transconductances. These are tuned with gmult(RC) and/or gimult(input gain) */
	/* rnm */
	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_gi, gvalues->gi);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g21, gvalues->g21);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g32, gvalues->g32);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g43, gvalues->g43);
	/* rff */
	MOD_RADIO_REG_TINY(pi, ADC_CFG16, core, adc_gff1, gvalues->gff1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG16, core, adc_gff2, gvalues->gff2);
	MOD_RADIO_REG_TINY(pi, ADC_CFG17, core, adc_gff3, gvalues->gff3);
	MOD_RADIO_REG_TINY(pi, ADC_CFG17, core, adc_gff4, gvalues->gff4);
	/* resonator and r11 */
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_r12, gvalues->r12);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_r34, gvalues->r34);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g54, gvalues->g11);

	/* Setup feedback DAC and tweak delay compensation */
	if (bw == 20) {
		/* In slow 40MHz ADC rt is 0x0, in 20MHz ADC rt is 0x2 */
	    MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_rt, 0x2);
		/* In slow 40MHz ADC slow_dacs is 0x2 in 20MHz ADC rt is 0x1 */
	    MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_slow_dacs, 0x1);
	} else if (bw == 40) {
		/* In slow 40MHz ADC rt is 0x0, in 20MHz ADC rt is 0x2 */
		MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_rt, 0x0);
		/* In slow 40MHz ADC slow_dacs is 0x2 in 20MHz ADC rt is 0x1 */
		MOD_RADIO_REG_TINY(pi, ADC_CFG19, core, adc_slow_dacs, 0x2);
	}
	if (RADIOMAJORREV(pi) != 3) {
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x0);
	}
}

static void
wlc_tiny_adc_setup_fast(phy_info_t *pi, tiny_adc_tuning_array_t *gvalues, uint8 core)
{

	/* Bypass override of slow/fast adc to power up/down for 4365 radio */
	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))) {
		/* EXPLICITLY ENABLE/DISABLE ADCs and INTERNAL CLKs?  */
		/* This part is comment out in TCL */
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_fast_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_slow_pu, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_slow_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_clk_fast_pu, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_clk_slow_pu, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_pu, 0x0);
	}
	/* Setup internal dividers and sipo for 3G2Hz mode. */
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG15, core, adc_clk_slow_div3, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_drive_strength, 0x4);
	MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_sipo_div8, 0x0);
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp1, 0x60);
	MOD_RADIO_REG_TINY(pi, ADC_CFG6, core, adc_biasadj_opamp2, 0x60);
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp3, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG7, core, adc_biasadj_opamp4, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_ff_mult_opamp, 0x1);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG9, core, adc_cmref4_control, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG10, core, adc_sipo_sel_fast, 0x1);

	/* Turn on overload detector */
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_bias_comp, 0x40);
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_threshold, 0x3);
	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_reset_duration, 0x3);
	MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_adc_od_pu, 0x1);

	MOD_RADIO_REG_TINY(pi, ADC_CFG18, core, adc_od_pu, 0x1);
	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))
		MOD_RADIO_REG_TINY(pi, ADC_CFG13, core, adc_od_disable, 0x0);

	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_gi, gvalues->gi);

	/* typo in spreadsheet for TC only - should be ri3 but got called ri1 */
	MOD_RADIO_REG_TINY(pi, ADC_CFG2, core, adc_ri3, gvalues->ri3);

	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g21, gvalues->g21);
	MOD_RADIO_REG_TINY(pi, ADC_CFG3, core, adc_g32, gvalues->g32);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g43, gvalues->g43);
	MOD_RADIO_REG_TINY(pi, ADC_CFG4, core, adc_g54, gvalues->g54);
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_g65, gvalues->g65);
	MOD_RADIO_REG_TINY(pi, ADC_CFG5, core, adc_r12, gvalues->r12);
	MOD_RADIO_REG_TINY(pi, ADC_CFG8, core, adc_r34, gvalues->r34);
	/* Is it need in 4365? */
	if (!(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID) && (RADIOMAJORREV(pi) == 3))) {
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x1);
		MOD_RADIO_REG_TINY(pi, ADC_CFG1, core, adc_adcs_reset, 0x0);
		MOD_RADIO_REG_TINY(pi, ADC_OVR1, core, ovr_reset_adc, 0x0);
	}
}

static void
wlc_phy_radio20693_adc_setup(phy_ac_radio_info_t *ri, uint8 core,
	radio_20693_adc_modes_t adc_mode)
{
	tiny_adc_tuning_array_t gvalues;
	uint8 bw;
	phy_info_t *pi = ri->pi;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	bw = CHSPEC_IS20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	if ((adc_mode == RADIO_20693_FAST_ADC) || (CHSPEC_IS80(pi->radio_chanspec)) ||
		(CHSPEC_IS160(pi->radio_chanspec)) ||
		(CHSPEC_IS8080(pi->radio_chanspec))) {
		/* 20 and 40MHz fast mode and 80MHz channel */
		wlc_tiny_sigdel_fast_tune(pi, ri->rccal_adc_gmult, &gvalues);
		wlc_tiny_adc_setup_fast(pi, &gvalues, core);
	} else {
		/* slow mode for 20 and 40MHz channel */
		wlc_tiny_sigdel_slow_tune(pi, ri->rccal_adc_gmult, &gvalues, bw);
		wlc_tiny_adc_setup_slow(pi, &gvalues, bw, core);
	}
}

/*
 *  The TIA has 13 distinct gain steps.
 *  Each of the tia_* scalers are packed with the
 *  tia settings for each gain step.
 *  Mapping for each gain step is:
 *  pwrup_amp2, amp2_bypass, R1, R2, R3, R4, C1, C2, enable_st1
 */
static void
wlc_tiny_tia_config(phy_info_t *pi, uint8 core)
{
/*
 *  The TIA has 13 distinct gain steps.
 *  Each of the tia_* scalers are packed with the
 *  tia settings for each gain step.
 *  Mapping for each gain step is:
 *  pwrup_amp2, amp2_bypass, R1, R2, R3, R4, C1, C2, enable_st1
 */
	const uint8  *p8;
	const uint16 *p16;
	const uint16 *prem = NULL;
	uint16 lut, lut_51, lut_82;
	int i;

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_80) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_40) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_20) == 52);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_80) == 30);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_20) == 30);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_80_rem) == 4);
		STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_16b_20_rem) == 4);
	}

	ASSERT(TINY_RADIO(pi));

	if (CHSPEC_IS80(pi->radio_chanspec) ||
	PHY_AS_80P80(pi, pi->radio_chanspec)) {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_80_rem;
		else {
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_80) +
				ARRAYSIZE(tiaRC_tiny_16b_80) == 82);
		}
		p8 = tiaRC_tiny_8b_80;
		p16 = tiaRC_tiny_16b_80;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_40_rem;
		else {
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_40) +
				ARRAYSIZE(tiaRC_tiny_16b_20) == 82);
		}
		p8 = tiaRC_tiny_8b_40;
		p16 = tiaRC_tiny_16b_40;
	} else {
		if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev))
			prem = tiaRC_tiny_16b_20_rem;
		else {
			STATIC_ASSERT(ARRAYSIZE(tiaRC_tiny_8b_20) +
				ARRAYSIZE(tiaRC_tiny_16b_20) == 82);
		}
		p8 = tiaRC_tiny_8b_20;
		p16 = tiaRC_tiny_16b_20;
	}

	lut = RADIO_REG(pi, TIA_LUT_0, core);
	lut_51 = RADIO_REG(pi, TIA_LUT_51, core);
	lut_82 = RADIO_REG(pi, TIA_LUT_82, core);

	/* the assumption is that all the TIA LUT registers are in sequence */
	ASSERT(lut_82 - lut == 81);

	if (ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) {
		for (i = 0; i < 52; i++) /* LUT0-LUT51 */
			phy_utils_write_radioreg(pi, lut++, p8[i]);

		for (i = 0; i < 26; i++) /* LUT52-LUT78 (no LUT67) */
			phy_utils_write_radioreg(pi, lut++, p16[i]);

		for (i = 0; i < 4; i++) /* LUT79-LUT82 */
			phy_utils_write_radioreg(pi, lut++, prem[i]);
	} else {
		do {
			phy_utils_write_radioreg(pi, lut++, *p8++);
		} while (lut <= lut_51);

		do {
			phy_utils_write_radioreg(pi, lut++, *p16++);
		} while (lut <= lut_82);
	}
}

/*  lookup radio-chip-specific channel code */
int
wlc_phy_chan2freq_acphy(phy_info_t *pi, uint8 channel, const void **chan_info)
{
	uint i;
	const chan_info_radio2069_t *chan_info_tbl = NULL;
	const chan_info_radio2069revGE16_t *chan_info_tbl_GE16 = NULL;
	const chan_info_radio2069revGE25_t *chan_info_tbl_GE25 = NULL;
	const chan_info_radio2069revGE32_t *chan_info_tbl_GE32 = NULL;
	const chan_info_radio2069revGE25_52MHz_t *chan_info_tbl_GE25_52MHz = NULL;

	phy_ac_radio_info_t *radioi = pi->u.pi_acphy->radioi;
	uint32 tbl_len = 0;
	int freq;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));
	switch (RADIO2069_MAJORREV(pi->pubpi->radiorev)) {
	case 0:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
		case 3:
			chan_info_tbl = chan_tuning_2069rev3;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev3);
		break;

		case 4:
		case 8:
			chan_info_tbl = chan_tuning_2069rev4;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev4);
			break;
		case 7: /* e.g. 43602a0 */
			chan_info_tbl = chan_tuning_2069rev7;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev7);
			break;
		case 64: /* e.g. 4364 */
			chan_info_tbl = chan_tuning_2069rev64;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev64);
			break;
		case 66: /* e.g. 4364 */
			chan_info_tbl = chan_tuning_2069rev66;
			tbl_len = ARRAYSIZE(chan_tuning_2069rev66);
			break;
		default:

			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			           pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
			ASSERT(0);
		}
		break;

	case 1:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
			case 16:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_40_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17);
					}
				}
				radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
				break;
			case 17:
			case 23:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 =
						       chan_tuning_2069rev_GE16_40_lp;
						tbl_len =
						       ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
#ifndef ACPHY_1X1_37P4
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 = chan_tuning_2069rev_GE16_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_16_17;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_16_17);
					}
#else
					if ((RADIO2069REV(pi->pubpi->radiorev)) == 23) {
						chan_info_tbl_GE16 =
						 chan_tuning_2069rev_23_2Glp_5Gnonlp;
						tbl_len =
						 ARRAYSIZE(chan_tuning_2069rev_23_2Glp_5Gnonlp);

					} else {
						chan_info_tbl_GE16 =
						 chan_tuning_2069rev_GE16_2Glp_5Gnonlp;
						tbl_len =
						 ARRAYSIZE(chan_tuning_2069rev_GE16_2Glp_5Gnonlp);
					}
#endif /* ACPHY_1X1_37P4 */
				}
				radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
				break;
			case 18:
			case 24:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure the LP mode settings */
						/* For Rev16/17/18 using the same LP setting TBD */
						chan_info_tbl_GE16 =
						    chan_tuning_2069rev_GE16_40_lp;
						tbl_len =
						    ARRAYSIZE(chan_tuning_2069rev_GE16_40_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_18_40;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_18_40);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else {
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
					    (radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						/* In this configure LP mode settings */
						/* For Rev16/17/18 using same LP setting TBD */
						chan_info_tbl_GE16 =
						       chan_tuning_2069rev_GE16_lp;
						tbl_len =
						       ARRAYSIZE(chan_tuning_2069rev_GE16_lp);
					} else {
						chan_info_tbl_GE16 = chan_tuning_2069rev_18;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_18);
					}
				}
				radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
				break;
			case 25:
			case 26:
				if (PHY_XTAL_IS40M(pi)) {
#ifndef ACPHY_1X1_37P4
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;

					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						chan_info_tbl_GE25 =
							chan_tuning_2069rev_GE_25_40MHz_lp;
						tbl_len =
						ARRAYSIZE(chan_tuning_2069rev_GE_25_40MHz_lp);
					} else {
						chan_info_tbl_GE25 =
						     chan_tuning_2069rev_GE_25_40MHz;
						tbl_len =
						     ARRAYSIZE(chan_tuning_2069rev_GE_25_40MHz);
					}
#else
					ASSERT(0);
#endif /* ACPHY_1X1_37P4 */
				} else if (PHY_XTAL_IS52M(pi)) {
					chan_info_tbl_GE25_52MHz = phy_ac_tbl_get_data
						(radioi->aci->tbli)->chan_tuning;
					tbl_len = phy_ac_tbl_get_data
						(radioi->aci->tbli)->chan_tuning_tbl_len;
				} else {
					radioi->data->acphy_lp_status = radioi->acphy_lp_mode;
					if ((radioi->acphy_lp_mode == 2) ||
						(radioi->acphy_lp_mode == 3) ||
						(radioi->data->acphy_force_lpvco_2G == 1 &&
						CHSPEC_IS2G(pi->radio_chanspec))) {
						chan_info_tbl_GE25 = chan_tuning_2069rev_GE_25_lp;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE_25_lp);
					} else {
						chan_info_tbl_GE25 = chan_tuning_2069rev_GE_25;
						tbl_len = ARRAYSIZE(chan_tuning_2069rev_GE_25);
					}
				}
				radioi->acphy_prev_lp_mode = radioi->acphy_lp_mode;
				break;
			default:
				PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
				   pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
				ASSERT(0);
		}

		break;

	case 2:
		switch (RADIO2069REV(pi->pubpi->radiorev)) {
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 44:
			/* can have more conditions based on different radio revs */
			/*  RADIOREV(pi->pubpi->radiorev) =32/33/34 */
			/* currently tuning tbls for these are all same */
			chan_info_tbl_GE32 = phy_ac_tbl_get_data(radioi->aci->tbli)->chan_tuning;
			tbl_len = phy_ac_tbl_get_data(radioi->aci->tbli)->chan_tuning_tbl_len;
			break;

		default:

			PHY_ERROR(("wl%d: %s: Unsupported radio revision %d\n",
			           pi->sh->unit, __FUNCTION__, RADIO2069REV(pi->pubpi->radiorev)));
			ASSERT(0);
		}
		break;
	default:
		PHY_ERROR(("wl%d: %s: Unsupported radio major revision %d\n",
		           pi->sh->unit, __FUNCTION__, RADIO2069_MAJORREV(pi->pubpi->radiorev)));
		ASSERT(0);
	}

	for (i = 0; i < tbl_len; i++) {

		if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
			if (chan_info_tbl_GE32[i].chan == channel)
				break;
		} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
			if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
			   (RADIO2069REV(pi->pubpi->radiorev) == 26))  {
			    if (!PHY_XTAL_IS52M(pi)) {
					if (chan_info_tbl_GE25[i].chan == channel)
						break;
				} else {
					if (chan_info_tbl_GE25_52MHz[i].chan == channel)
						break;
				}
			}
			else if (chan_info_tbl_GE16[i].chan == channel)
				break;
		} else {
			if (chan_info_tbl[i].chan == channel)
				break;
		}
	}

	if (i >= tbl_len) {
		PHY_ERROR(("wl%d: %s: channel %d not found in channel table\n",
		           pi->sh->unit, __FUNCTION__, channel));
		ASSERT(i < tbl_len);

		return -1;
	}

	if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 2) {
		*chan_info = &chan_info_tbl_GE32[i];
		freq = chan_info_tbl_GE32[i].freq;
	} else if (RADIO2069_MAJORREV(pi->pubpi->radiorev) == 1) {
		if ((RADIO2069REV(pi->pubpi->radiorev) == 25) ||
			(RADIO2069REV(pi->pubpi->radiorev) == 26)) {
				if (!PHY_XTAL_IS52M(pi)) {
					*chan_info = &chan_info_tbl_GE25[i];
					freq = chan_info_tbl_GE25[i].freq;
				} else {
					*chan_info = &chan_info_tbl_GE25_52MHz[i];
					freq = chan_info_tbl_GE25_52MHz[i].freq;
				}
		} else {
			*chan_info = &chan_info_tbl_GE16[i];
			freq = chan_info_tbl_GE16[i].freq;
		}
	} else {
		*chan_info = &chan_info_tbl[i];
		freq = chan_info_tbl[i].freq;
	}

	return freq;
}

/* XXX 20691_lpf_tx_set is the top Tx LPF function and should be the usual function called from
 * acphyprocs or used from the REPL in the lab. This function is called for e.g. the 20693.
 */
void
wlc_phy_radio_tiny_lpf_tx_set(phy_info_t *pi, int8 bq_bw, int8 bq_gain,
	int8 rc_bw_ofdm, int8 rc_bw_cck)
{
	uint8 i, core;
	uint16 gmult;
	uint16 gmult_rc;
	uint16 g10_tuned, g11_tuned, g12_tuned, g21_tuned, bias;

	gmult = pi->u.pi_acphy->radioi->data->rccal_gmult;
	gmult_rc = pi->u.pi_acphy->radioi->data->rccal_gmult_rc;

	/* search for given bq_gain */
	for (i = 0; i < ARRAYSIZE(g_index1); i++) {
		if (bq_gain == g_index1[i])
			break;
	}

	if (i < ARRAYSIZE(g_index1)) {
		uint16 g_passive_rc_tx_tuned_ofdm, g_passive_rc_tx_tuned_cck;
		g10_tuned = (lpf_g10[bq_bw][i] * gmult) >> 15;
		g11_tuned = (lpf_g11[bq_bw] * gmult) >> 15;
		g12_tuned = (lpf_g12[bq_bw][i] * gmult) >> 15;
		g21_tuned = (lpf_g21[bq_bw][i] * gmult) >> 15;
		g_passive_rc_tx_tuned_ofdm = (g_passive_rc_tx[rc_bw_ofdm] * gmult_rc) >> 15;
		g_passive_rc_tx_tuned_cck = (g_passive_rc_tx[rc_bw_cck] * gmult_rc) >> 15;
		bias = biases[bq_bw];
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG3, core, lpf_g10, g10_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG7, core, lpf_g11, g11_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG4, core, lpf_g12, g12_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG5, core, lpf_g21, g21_tuned);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG6, core, lpf_g_passive_rc_tx,
				g_passive_rc_tx_tuned_ofdm);
			MOD_RADIO_REG_TINY(pi, TX_LPF_CFG8, core, lpf_bias_bq, bias);
		}
		if (D11REV_IS(pi->sh->corerev, 47) || D11REV_IS(pi->sh->corerev, 54) ||
			D11REV_IS(pi->sh->corerev, 58)) {
			/* Note down the values of the passive_rc for OFDM and CCK in Shmem */
			wlapi_bmac_write_shm(pi->sh->physhim,
				M_LPF_PASSIVE_RC_OFDM(pi), g_passive_rc_tx_tuned_ofdm);
			wlapi_bmac_write_shm(pi->sh->physhim, M_LPF_PASSIVE_RC_CCK(pi),
				g_passive_rc_tx_tuned_cck);
		}
	} else {
		PHY_ERROR(("wl%d: %s: Invalid bq_gain %d\n", pi->sh->unit, __FUNCTION__, bq_gain));
	}
	/* Change IIR filter shape to solve bandedge problem for 42q(5210q) channel */
	if ((ACMAJORREV_32(pi->pubpi->phy_rev) || ACMAJORREV_33(pi->pubpi->phy_rev)) &&
		(CHSPEC_IS80(pi->radio_chanspec))) {
		if (CHSPEC_CHANNEL(pi->radio_chanspec) == 42) {
			WRITE_PHYREG(pi, txfilt80st0a1, 0xf8b);
			WRITE_PHYREG(pi, txfilt80st0a2, 0x343);
			WRITE_PHYREG(pi, txfilt80st1a1, 0xe72);
			WRITE_PHYREG(pi, txfilt80st1a2, 0x11f);
			WRITE_PHYREG(pi, txfilt80st2a1, 0xc57);
			WRITE_PHYREG(pi, txfilt80st2a2, 0x166);
			/* Reducing rc_filter bandwidth for 42q channel */
			/* MOD_RADIO_ALLREG_20693(pi, TX_LPF_CFG6, lpf_g_passive_rc_tx, 25); */
		} else {
			WRITE_PHYREG(pi, txfilt80st0a1, 0xf93);
			WRITE_PHYREG(pi, txfilt80st0a2, 0x36e);
			WRITE_PHYREG(pi, txfilt80st1a1, 0xdf7);
			WRITE_PHYREG(pi, txfilt80st1a2, 0x257);
			WRITE_PHYREG(pi, txfilt80st2a1, 0xbe2);
			WRITE_PHYREG(pi, txfilt80st2a2, 0x16c);
		}
	}
}

static void
wlc_phy_radio20693_set_channel_bw(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int row;
	uint8 dac_rate_mode;
	uint8 dacclkdiv = 0;
	uint16 dac_rate;
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	}
	row = wlc_phy_radio20693_altclkpln_get_chan_row(pi);

	dac_rate_mode = pi_ac->radioi->data->dac_mode;
	dac_rate = wlc_phy_get_dac_rate_from_mode(pi, dac_rate_mode);
	ASSERT(dac_rate_mode <= 2);

	if (adc_mode == RADIO_20693_FAST_ADC) {
		if (dac_rate_mode == 1) {
			if (dac_rate == 200)
				dacclkdiv = 3;
			else if (dac_rate == 400)
				dacclkdiv = 1;
			else
				dacclkdiv = 0;
		} else if (dac_rate_mode == 2) {
			dacclkdiv = 0;
		}
	} else {
		if (dac_rate_mode == 1) {
#if !defined(MACOSX)
			if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
				if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
					(CHSPEC_IS20(pi->radio_chanspec) ||
					CHSPEC_IS40(pi->radio_chanspec)) &&
					!PHY_IPA(pi) && !ROUTER_4349(pi)) {
					dacclkdiv = 1;
				} else {
					dacclkdiv = 0;
				}
			} else {
				dacclkdiv = 0;
			}
#else
			dacclkdiv = 0;
#endif /* MACOSX */
		} else {
			PHY_ERROR(("wl%d: %s: unknown dac rate mode slow\n",
				pi->sh->unit, __FUNCTION__));
		}
	}
	if ((row >= 0) && (adc_mode != RADIO_20693_FAST_ADC)) {
		dacclkdiv = altclkpln[row].dacclkdiv;
	}
	MOD_RADIO_REG_20693(pi, CLK_DIV_CFG1, core, sel_dac_div, dacclkdiv);
	if (ACMAJORREV_4(pi->pubpi->phy_rev)) {
#if !defined(MACOSX)
		if (CHSPEC_ISPHY5G6G(pi->radio_chanspec) &&
			(CHSPEC_IS20(pi->radio_chanspec) ||
			CHSPEC_IS40(pi->radio_chanspec)) &&
			!PHY_IPA(pi) && !ROUTER_4349(pi)) {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc, 0);
		} else {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc, 4);
		}
#endif /* MACOSX */

		/* SWWLAN-79504: Increasing the DAC frequency for Ch-64 in order to avoid DAC spur
		 * in other core in RSDB mode
		 */
		if (row >= 0) {
			MOD_RADIO_REG_20693(pi, TX_AFE_CFG1, core, afe_ctl_misc,
				altclkpln[row].dacdiv << 2);
		}

	}
}

int
wlc_phy_radio20693_altclkpln_get_chan_row(phy_info_t *pi)
{
	const chan_info_radio20693_altclkplan_t *altclkpln = altclkpln_radio20693;
	int tbl_len = ARRAYSIZE(altclkpln_radio20693);
	int row;
	int channel = CHSPEC_CHANNEL(pi->radio_chanspec);
	int bw	= CHSPEC_IS20(pi->radio_chanspec) ? 20 : CHSPEC_IS40(pi->radio_chanspec) ? 40 : 80;

	if (ROUTER_4349(pi)) {
		altclkpln = altclkpln_radio20693_router4349;
	        tbl_len = ARRAYSIZE(altclkpln_radio20693_router4349);
	}
	for (row = 0; row < tbl_len; row++) {
		if ((altclkpln[row].channel == channel) && (altclkpln[row].bw == bw)) {
			break;
		}
	}

	/* 53574: SWWLAN-79504 & SWWLAN-76882: DAC/ADC-SIPO spur issue is seen in RSDB mode */
	if (ROUTER_4349(pi) && (phy_get_phymode(pi) != PHYMODE_RSDB)) {
		row = tbl_len;
	}
	return ((row < tbl_len) &&
		(ROUTER_4349(pi) ? ALTCLKPLN_ENABLE_ROUTER4349 : ALTCLKPLN_ENABLE)) ? row : -1;
}

void
wlc_phy_radio20708_tx2cal_normal_adc_rate(phy_info_t *pi, bool use_ovr, bool is_lowrate_mode)
{
	uint8 pllcore;
	uint8 ovr_status;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	/* read the current ovr status */
	ovr_status = READ_RADIO_PLLREGFLD_20708(pi, AFEDIV_CFG1_OVR, 0,
			ovr_afediv_en_low_rate_tssi);

	/* return if intended use_ovr is already set */
	if (use_ovr == ovr_status) {
		return;
	}

	if (use_ovr) {
		for (pllcore = 0; pllcore < 2; pllcore++) {
			ri->RFP_afediv_cfg1_ovr_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_CFG1_OVR, pllcore);
			ri->RFP_afediv_reg3_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_REG3, pllcore);
			MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
				ovr_afediv_en_low_rate_tssi, 1);
			MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG3, pllcore,
				afediv_en_low_rate_tssi, is_lowrate_mode);
		}
	} else {
		for (pllcore = 0; pllcore < 2; pllcore++) {
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
				ri->RFP_afediv_cfg1_ovr_orig[pllcore]);
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_REG3, pllcore,
				ri->RFP_afediv_reg3_orig[pllcore]);
		}
	}
}

static void
wlc_phy_radio20693_adc_config_overrides(phy_info_t *pi,
	radio_20693_adc_modes_t adc_mode, uint8 core)
{
	uint8 is_fast;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	is_fast = (adc_mode == RADIO_20693_FAST_ADC);
	wlc_phy_radio20693_adc_powerupdown(pi, RADIO_20693_SLOW_ADC, !is_fast, core);
	wlc_phy_radio20693_adc_powerupdown(pi, RADIO_20693_FAST_ADC, is_fast, core);
}

static void
wlc_phy_radio20693_adc_dac_setup(phy_info_t *pi, radio_20693_adc_modes_t adc_mode, uint8 core)
{
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (RADIOMAJORREV(pi) != 3) {
		wlc_phy_radio20693_afeclkpath_setup(pi, core, adc_mode, 0);
		wlc_phy_radio20693_adc_config_overrides(pi, adc_mode, core);
	}

	wlc_phy_radio20693_adc_setup(pi->u.pi_acphy->radioi, core, adc_mode);

	if (RADIOMAJORREV(pi) != 3)
		wlc_phy_radio20693_set_channel_bw(pi, adc_mode, core);

	if (RADIOMAJORREV(pi) != 3)
		wlc_phy_radio20693_config_bf_mode(pi, core);
}

static void
wlc_phy_radio20693_setup_crisscorss_ovr(phy_info_t *pi, uint8 core)
{
	uint8 pupd = 1;
	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_tx5g_80p80_cas_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_5G_OVR2, core, ovr_tx5g_80p80_gm_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR1_EAST, core, ovr_tx2g_20p20_cas_pu, pupd);
	MOD_RADIO_REG_20693(pi, TX_TOP_2G_OVR1_EAST, core, ovr_tx2g_20p20_gm_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_src_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_des_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_5G_OVR, core, ovr_rx5g_80p80_gc, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_src_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_des_pu, pupd);
	MOD_RADIO_REG_20693(pi, RX_TOP_2G_OVR_EAST, core, ovr_rx2g_20p20_gc, pupd);
}

static void
wlc_phy_radio20693_afecal(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = pi->u.pi_acphy;
	uint8 core;
	radio_20693_adc_modes_t adc_mode;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM20693_ID));
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (((phy_ac_chanmgr_get_data(pi_ac->chanmgri))->fast_adc_en == 1) ||
		(CHSPEC_IS80(pi->radio_chanspec)) || (CHSPEC_IS160(pi->radio_chanspec)) ||
		(CHSPEC_IS8080(pi->radio_chanspec))) {
		adc_mode = RADIO_20693_FAST_ADC;
	} else {
		adc_mode = RADIO_20693_SLOW_ADC;
	}

	FOREACH_CORE(pi, core) {
		wlc_phy_radio20693_adc_dac_setup(pi, adc_mode, core);
		wlc_tiny_tia_config(pi, core);
		if (RADIOMAJORREV(pi) != 3) {
			wlc_phy_radio20693_setup_crisscorss_ovr(pi, core);
		}
	}
}

static void
wlc_phy_radio20698_afecal(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_afe_cal */

	uint8 core;
	uint8 restore_ext_5g_papu[PHY_CORE_MAX];
	uint8 restore_override_ext_pa[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20698_ID);
	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		restore_ext_5g_papu[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, ext_5g_papu);
		restore_override_ext_pa[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, override_ext_pa);
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
	}

	wlc_phy_radio20698_adc_cap_cal(pi, 0);
	//for 160(TI-ADC) calibrate rail 1 too.
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		wlc_phy_radio20698_adc_cap_cal(pi, 1);
	}

	wlc_phy_radio20698_txdac_bw_setup(pi);
	wlc_phy_radio20698_adc_offset_gain_cal(pi);
	/* Restore PA reg value after reseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, ext_5g_papu, restore_ext_5g_papu[core]);
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, override_ext_pa, restore_override_ext_pa[core]);
	}
}

static void
wlc_phy_radio20704_afecal(phy_info_t *pi)
{
	/* 20704_procs.tcl r773183: 20704_afe_cal */

	uint8 core;
	uint8 restore_override_ext_pa[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20704_ID);
	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		restore_override_ext_pa[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, override_ext_pa);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	wlc_phy_radio20704_adc_cap_cal(pi);
	wlc_phy_radio20704_txdac_bw_setup(pi);

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20704(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	/* Restore PA reg value after reseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, override_ext_pa, restore_override_ext_pa[core]);
	}
}

static void
wlc_phy_radio20707_afecal(phy_info_t *pi)
{
	uint8 core;
	uint8 restore_ext_5g_papu[PHY_CORE_MAX];
	uint8 restore_override_ext_pa[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20707_ID);

	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		restore_ext_5g_papu[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, ext_5g_papu);
		restore_override_ext_pa[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, override_ext_pa);
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
	}
	/* activate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	wlc_phy_radio20707_adc_cap_cal(pi);
	wlc_phy_radio20707_txdac_bw_setup(pi);

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20707(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);
	/* Restore PA reg value after reseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, ext_5g_papu, restore_ext_5g_papu[core]);
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, override_ext_pa, restore_override_ext_pa[core]);
	}
}

static void
wlc_phy_radio20708_afecal(phy_info_t *pi)
{
	uint8 core;
	uint16 rfctrlIntc[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20708_ID);

	/* Save rfctrlIntc value before rfseq setting */
	FOREACH_CORE(pi, core)
		rfctrlIntc[core] = READ_PHYREGCE(pi, RfctrlIntc, core);

	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
	}

	wlc_phy_radio20708_afe_cal_main(pi);
	wlc_phy_radio20708_txdac_bw_setup(pi);
	wlc_phy_radio20708_txadc_bw_setup(pi);

	// TI-ADC cal
	wlc_phy_radio20708_adc_offset_gain_cal(pi);

	/* Restore rfctrlIntc value after rfseq setting */
	FOREACH_CORE(pi, core)
		WRITE_PHYREGCE(pi, RfctrlIntc, core, rfctrlIntc[core]);
}

static void
wlc_phy_radio20709_afecal(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_afe_cal */

	uint8 core;
	uint8 restore_override_ext_pa[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20709_ID);
	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		restore_override_ext_pa[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, override_ext_pa);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	wlc_phy_radio20709_adc_cap_cal(pi);
	wlc_phy_radio20709_txdac_bw_setup(pi);

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20709(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	/* Restore PA reg value after reseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, override_ext_pa, restore_override_ext_pa[core]);
	}
}

static void
wlc_phy_radio20710_afecal(phy_info_t *pi)
{
	/* 20710_procs.tcl r773183: 20710_afe_cal */

	uint8 core;
	uint8 restore_override_ext_pa[PHY_CORE_MAX];

	ASSERT(RADIOID(pi->pubpi->radioid) == BCM20710_ID);
	/* Disable PA during rfseq setting */
	FOREACH_CORE(pi, core) {
		restore_override_ext_pa[core] = READ_PHYREGFLDCE(pi, RfctrlIntc,
				core, override_ext_pa);
		MOD_PHYREGCE(pi, RfctrlIntc, core, ext_5g_papu, 0);
		MOD_PHYREGCE(pi, RfctrlIntc, core, override_ext_pa, 1);
	}

	/* activate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x1);

	wlc_phy_radio20710_adc_cap_cal(pi, 0);
	//for 160(TI-ADC) calibrate rail 1 too.
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		wlc_phy_radio20710_adc_cap_cal(pi, 1);
	}

	wlc_phy_radio20710_txdac_bw_setup(pi);

	/* deactivate clkrccal */
	MOD_RADIO_PLLREG_20710(pi, PLL_REFDOUBLER2, 0, RefDoubler_pu_clkrccal, 0x0);

	/* Restore PA reg value after reseq setting */
	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, RfctrlIntc,
				core, override_ext_pa, restore_override_ext_pa[core]);
	}
}

static uint
wlc_phy_cap_cal_status_rail(phy_info_t *pi, uint8 rail, uint8 core)
{
	//returns true when adc_cap_cal is successfully done
	uint status = 1;
	if (rail == 0) {
			status *= ((READ_RADIO_REG_20698(pi, RXADC_CAL_STATUS, core)
					& 0x2) >>1); // mask for bit o_rxadc_cal_done_adc0_I
			status *= ((READ_RADIO_REG_20698(pi, RXADC_CAL_STATUS, core)
					& 0x4) >>2); // mask for bit o_rxadc_cal_done_adc0_Q
	} else {//rail 1
			status *= ((READ_RADIO_REG_20698(pi, RXADC_CAL_STATUS, core)
					& 0x8) >>3); // mask for bit o_cal_done_adc1_I
			status *= ((READ_RADIO_REG_20698(pi, RXADC_CAL_STATUS, core)
					& 0x1) >>0); // mask for bit o_cal_done_adc1_Q
	}
	return status;
}

static void
wlc_phy_radio20698_adc_cap_cal(phy_info_t *pi, uint8 adc)
{
	/* 20698_procs.tcl r708059: 20698_adc_cap_cal */

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core, max_chains;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;

	int8 cap_cal_iterations_left;
	uint8 cap_cal_status;
	const uint8 CAP_CAL_SUCCESS = 1;
	const int8 MAX_CAP_CAL_ITER = 3;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20698_afe_div_ratio(pi, 1, 0, 0);
	}

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
	                        max_chains, max_chains);

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	 *      to get also the first time after a power cycle valid results
	 */
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		wlc_phy_radio20698_adc_cap_cal_setup(pi, core);
		cap_cal_iterations_left = MAX_CAP_CAL_ITER;
		cap_cal_status = ~CAP_CAL_SUCCESS;
		while (cap_cal_iterations_left > 0) {
			if (cap_cal_status == CAP_CAL_SUCCESS) {
				PHY_INFORM(("Successful after %d iteration \n",
						MAX_CAP_CAL_ITER - cap_cal_iterations_left));
				break;
			} else {
				MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
				wlc_phy_radio20698_adc_cap_cal_parallel(pi, core,
						adc, &adccapcal_Timeout);
				cap_cal_iterations_left--;
				cap_cal_status = wlc_phy_cap_cal_status_rail(pi, adc, core);
			}
		}
		if (cap_cal_status != CAP_CAL_SUCCESS) {
			PHY_ERROR(("%s: ADC_CAP_CAL Failed FOR RAIL %d EVERY TIME!\n",
					__FUNCTION__, adc));
		}

	    //wlc_phy_radio20698_adc_cap_cal_setup(pi, core);
	    //MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
	    //wlc_phy_radio20698_adc_cap_cal_parallel(pi, core, adc, &adccapcal_Timeout);
	    if (adccapcal_Timeout == 0) {
	        PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
	            "due to cal TIMEOUT\n", __FUNCTION__, core));
	        MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
	    } else {
	        wlc_phy_radio20698_apply_adc_cal_result(pi, core, adc);
	        MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
	    }
	    /* Cleanup */
	    MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
	    MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
	}
	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	/* Re-enable low ADC rate if configured */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20698_afe_div_ratio(pi, 0, 0, 0);
	}
}

static void
wlc_phy_radio20704_adc_cap_cal(phy_info_t *pi)
{
	/* 20704_procs.tcl r773183: 20704_adc_cap_cal */

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core, max_chains;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	uint8 cap_cal_iter;
	/* Current 28nm AFE has calibration start issue. WAR is to re-calibrate on fail
	 * (see HW63178-298)
	 */
	const uint8 MAX_CAP_CAL_ITER = 10;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20704_afe_div_ratio(pi, 1);
	}

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
	                        max_chains, max_chains);

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	 *      to get also the first time after a power cycle valid results
	 */
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {

		wlc_phy_radio20704_adc_cap_cal_setup(pi, core);
		for (cap_cal_iter = 0; cap_cal_iter < MAX_CAP_CAL_ITER; cap_cal_iter++) {
			MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
			wlc_phy_radio20704_adc_cap_cal_parallel(pi, core, &adccapcal_Timeout);
			if (adccapcal_Timeout > 0) break;
		}

		if (adccapcal_Timeout == 0) {
			PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
					"due to cal TIMEOUT\n", __FUNCTION__, core));
			MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
			/* This should never happen, so make sure it is seen in development driver.
			 * In external driver, switch off correction as last resort
			 */
			ASSERT(0);
		} else {
			PHY_INFORM(("%s: adc_cap_cal -- core%d - SUCCESS after "
					"%d iteration(s)\n", __FUNCTION__, core, cap_cal_iter));
			MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
		}
		/* Cleanup */
		MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
		MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
		MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
		MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	}

	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);
	/* Re-enable low ADC rate if configured */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20704_afe_div_ratio(pi, 0);
	}
}

static void
wlc_phy_radio20707_adc_cap_cal(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	uint8 cap_cal_iter;
	const uint8 MAX_CAP_CAL_ITER = 10;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20707_afe_div_ratio(pi, 1);
	}

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	*      to get also the first time after a power cycle valid results
	*/
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);
	FOREACH_CORE(pi, core) {

		wlc_phy_radio20707_adc_cap_cal_setup(pi, core);
		for (cap_cal_iter = 0; cap_cal_iter < MAX_CAP_CAL_ITER; cap_cal_iter++) {
			MOD_RADIO_REG_20707(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
			wlc_phy_radio20707_adc_cap_cal_parallel(pi, core, &adccapcal_Timeout);
			if (adccapcal_Timeout > 0) break;
		}

		if (adccapcal_Timeout == 0) {
			PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
				"due to cal TIMEOUT\n", __FUNCTION__, core));
			MOD_RADIO_REG_20707(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
		} else {
			PHY_INFORM(("%s: adc_cap_cal -- core%d - SUCCESS after "
				"%d iteration(s)\n", __FUNCTION__, core, cap_cal_iter));
			MOD_RADIO_REG_20707(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
		}
		/* Cleanup */
		MOD_RADIO_REG_20707(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
		MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
		MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
		MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	}
	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);
	/* Re-enable low ADC rate if configured */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20707_afe_div_ratio(pi, 0);
	}
}

static void
wlc_phy_radio20708_afe_cal_main(phy_info_t *pi)
{
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core, max_chains;
	uint8 pllcore;
	uint16 bbmult = 0;

	uint16 cal_iter;
	uint8 afecal_done;
	uint8 num_adc_rails;
	uint8 rail, iq;
	uint8 cap0, cap1, cap2;
	const uint16 MAX_CAL_ITER = 400;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	/* Save and overwrite chains, Cals over all possible Tx/Rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
	                        max_chains, max_chains);

	if (CHSPEC_IS160(pi->radio_chanspec)) {
	     num_adc_rails = 2;
	} else {
	     num_adc_rails = 1;
	}

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	*      to get also the first time after a power cycle valid results
	*/
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* --- From here to the end of this function is equivalent to old
	*  adc_cap_cal
	*/
	/* Save RF PLL registers */
	wlc_phy_radio20708_afe_cal_pllreg_save(pi, TRUE);

	for (pllcore = 0; pllcore < 2; pllcore++) {
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_pu_inbuf, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
			afediv_pu_inbuf, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_adc_div_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
			afediv_adc_div_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_outbuf_adc_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
			afediv_pu_outbuf_adc, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_outbuf_dac_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
			afediv_pu_outbuf_dac, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_dac_div_pu, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
			afediv_dac_div_pu, 1);

		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_adc_div1, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG4, pllcore,
			afediv_adc_div1, 48);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_dac_div1, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG5, pllcore,
			afediv_dac_div1, 12);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
			ovr_afediv_en_low_rate_tssi, 1);
		MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG3, pllcore,
			afediv_en_low_rate_tssi, 0);
	}

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		/* Save AFE registers */
		phy_ac_reg_cache_save_percore(pi_ac, RADIOREGS_AFECAL, core);
		/* AFE cal setip */
		wlc_phy_radio20708_afe_cal_setup(pi, core, num_adc_rails);

		if (0) {
			PHY_INFORM(("core:%d; ovr_rxadc_puI = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_rxadc_puI)));
			PHY_INFORM(("core:%d; rxadc_puI = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core, rxadc_puI)));
			PHY_INFORM(("core:%d; ovr_rxadc_puQ = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_rxadc_puQ)));
			PHY_INFORM(("core:%d; rxadc_puQ = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core, rxadc_puQ)));
			PHY_INFORM(("core:%d; ovr_rxadc_pu_adc_clk = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_rxadc_pu_adc_clk)));
			PHY_INFORM(("core:%d; rxadc_pu_adc_clk = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core,
				rxadc_pu_adc_clk)));
			PHY_INFORM(("core:%d; ovr_rxadc_pu_refI = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR1, core,
				ovr_rxadc_pu_refI)));
			PHY_INFORM(("core:%d; rxadc_pu_refI = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core, rxadc_pu_refI)));
			PHY_INFORM(("core:%d; ovr_rxadc_pu_refQ = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR1, core,
				ovr_rxadc_pu_refQ)));
			PHY_INFORM(("core:%d; rxadc_pu_refQ = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core, rxadc_pu_refQ)));
			PHY_INFORM(("core:%d; ovr_rxadc_pu_refbias = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR1, core,
				ovr_rxadc_pu_refbias)));
			PHY_INFORM(("core:%d; rxadc_pu_refbias = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG0, core,
				rxadc_pu_refbias)));
			PHY_INFORM(("core:%d; ovr_rxadc0_enb = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_rxadc0_enb)));
			PHY_INFORM(("core:%d; rxadc0_enb = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core, rxadc0_enb)));
			PHY_INFORM(("core:%d; ovr_rxadc1_enb = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_rxadc1_enb)));
			PHY_INFORM(("core:%d; rxadc1_enb = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG5, core, rxadc1_enb)));

			PHY_INFORM(("core:%d; ovr_iqdac_pwrup_diode = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_iqdac_pwrup_diode)));
			PHY_INFORM(("core:%d; iqdac_pwrup_diode = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core,
				iqdac_pwrup_diode)));
			PHY_INFORM(("core:%d; ovr_iqdac_pwrup_I = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_iqdac_pwrup_I)));
			PHY_INFORM(("core:%d; iqdac_pwrup_I = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core, iqdac_pwrup_I)));
			PHY_INFORM(("core:%d; ovr_iqdac_pwrup_Q = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_iqdac_pwrup_Q)));
			PHY_INFORM(("core:%d; iqdac_pwrup_Q = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core, iqdac_pwrup_Q)));
			PHY_INFORM(("core:%d; iqdac_pu_ibn_8u_1p8bg = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core,
				iqdac_pu_ibn_8u_1p8bg)));
			PHY_INFORM(("core:%d; ovr_iqdac_buf_pu = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_iqdac_buf_pu)));
			PHY_INFORM(("core:%d; iqdac_buf_pu = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core, iqdac_buf_pu)));
			PHY_INFORM(("core:%d; ovr_iqdac_reset = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, AFE_CFG1_OVR2, core,
				ovr_iqdac_reset)));
			PHY_INFORM(("core:%d; iqdac_reset = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, TXDAC_REG0, core, iqdac_reset)));

			PHY_INFORM(("core:%d; rxadc_cal_cap = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG3, core, rxadc_cal_cap)));
			PHY_INFORM(("core:%d; rxadc_buffer_per = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core,
				rxadc_buffer_per)));
			PHY_INFORM(("core:%d; rxadc_wait_per = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core, rxadc_wait_per)));
			PHY_INFORM(("core:%d; rxadc_sum_per = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG2, core, rxadc_sum_per)));
			PHY_INFORM(("core:%d; rxadc_sum_div = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core, rxadc_sum_div)));
			PHY_INFORM(("core:%d; rxadc_corr_dis = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core, rxadc_corr_dis)));
			PHY_INFORM(("core:%d; rxadc_coeff_sel = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_CFG1, core,
				rxadc_coeff_sel)));
			PHY_INFORM(("core:%d; rxadc_core_ctrl_MSB = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_REG3, core,
				rxadc_core_ctrl_MSB)));
			PHY_INFORM(("core:%d; rxadc_core_ctrl_LSB = %d \n", core,
				READ_RADIO_REGFLD_20708(pi, RF, RXADC_REG2, core,
				rxadc_core_ctrl_LSB)));
		}

		for (rail = 0; rail < num_adc_rails; rail++) {
			for (iq = 0; iq < 2; iq++) {
				/* --- Reset ADC0/1 I/Q channel cal engine */
				MOD_AFE_CAL_RFREGS_20708(pi, AFE_CFG1_OVR2, core,
					ovr_rxadc_reset_n_adc, rail, iq, 1);
				MOD_AFE_CAL_RFREGS_20708(pi, RXADC_CFG0, core,
					rxadc_reset_n_adc, rail, iq, 0);
				OSL_DELAY(20);
				MOD_AFE_CAL_RFREGS_20708(pi, RXADC_CFG0, core,
					rxadc_reset_n_adc, rail, iq, 1);
				OSL_DELAY(20);

				MOD_AFE_CAL_RFREGS_20708(pi, RXADC_CFG0, core,
					rxadc_start_cal_adc, rail, iq, 1);
				OSL_DELAY(20);

				if ((rail == 0) && (iq == 0)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc0_I, 0x3);
				} else if ((rail == 0) && (iq == 1)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc0_Q, 0x3);
				} else if ((rail == 1) && (iq == 0)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc1_I, 0x3);
				} else {
					MOD_RADIO_REG_20708(pi, RXADC_CFG4, core,
					    rxadc_coeff_out_ctrl_adc1_Q, 0x3);
				}

				if (0) {
					PHY_INFORM(("core:%d; rail:%d; iq:%d; reset = %d \n", core,
						rail, iq, READ_AFE_CAL_RFREGS_20708(pi, RF,
						RXADC_CFG0, core, rxadc_reset_n_adc, rail, iq)));
					PHY_INFORM(("core:%d; rail:%d; iq:%d; start = %d \n", core,
						rail, iq, READ_AFE_CAL_RFREGS_20708(pi, RF,
						RXADC_CFG0, core, rxadc_start_cal_adc, rail, iq)));
				}

				afecal_done = READ_AFE_CAL_RFREGS_20708(pi, RF,
					RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc, rail, iq);
				cal_iter = 0;
				while ((afecal_done != 1) && (cal_iter < MAX_CAL_ITER)) {
					OSL_DELAY(10);
					afecal_done = READ_AFE_CAL_RFREGS_20708(pi, RF,
						RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc,
						rail, iq);
					cal_iter++;
				}

				if (cal_iter == MAX_CAL_ITER) {
					PHY_ERROR(("%s: adc_afe_cal -- core%d - DISABLED"
						"correction, due to cal TIMEOUT\n",
						__FUNCTION__, core));
					MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
						rxadc_corr_dis, 0x1);
				} else {
					PHY_INFORM(("%s: adc_afe_cal -- core%d - SUCCESS after "
						"%d iteration(s)\n", __FUNCTION__, core, cal_iter));
					MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
						rxadc_corr_dis, 0x0);
				}

				/* Stop ADC0/1 I/Q channel calibration */
				MOD_AFE_CAL_RFREGS_20708(pi, RXADC_CFG0, core,
					rxadc_start_cal_adc, rail, iq, 0);
			}
		}
		/* Restore AFE registers */
		phy_ac_reg_cache_restore_percore(pi_ac, RADIOREGS_AFECAL, core);
	}

	/* Restore RF PLL registers */
	wlc_phy_radio20708_afe_cal_pllreg_save(pi, FALSE);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		for (rail = 0; rail < num_adc_rails; rail++) {
			for (iq = 0; iq < 2; iq++) {
				if ((rail == 0) && (iq == 0)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc0_I, 0x3);
				} else if ((rail == 0) && (iq == 1)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc0_Q, 0x3);
				} else if ((rail == 1) && (iq == 0)) {
					MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
					    rxadc_coeff_out_ctrl_adc1_I, 0x3);
				} else {
					MOD_RADIO_REG_20708(pi, RXADC_CFG4, core,
					    rxadc_coeff_out_ctrl_adc1_Q, 0x3);
				}

				MOD_AFE_CAL_RFREGS_20708(pi, AFE_CFG1_OVR1, core,
					ovr_rxadc_coeff_cap0_adc, rail, iq, 1);
				MOD_AFE_CAL_RFREGS_20708(pi, AFE_CFG1_OVR1, core,
					ovr_rxadc_coeff_cap1_adc, rail, iq, 1);
				MOD_AFE_CAL_RFREGS_20708(pi, AFE_CFG1_OVR1, core,
					ovr_rxadc_coeff_cap2_adc, rail, iq, 1);

				if ((rail == 0) && (iq == 0)) {
					cap0 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP0_STAT,
						core, o_coeff_cap0_adc0_I);
					cap1 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP1_STAT,
						core, o_coeff_cap1_adc0_I);
					cap2 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP2_STAT,
						core, o_coeff_cap2_adc0_I);
					MOD_RADIO_REG_20708(pi, RXADC_CFG6, core,
					    rxadc_coeff_cap0_adc0_I, cap0);
					MOD_RADIO_REG_20708(pi, RXADC_CFG5, core,
					    rxadc_coeff_cap1_adc0_I, cap1);
					MOD_RADIO_REG_20708(pi, RXADC_CFG5, core,
					    rxadc_coeff_cap2_adc0_I, cap2);
				} else if ((rail == 0) && (iq == 1)) {
					cap0 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP0_STAT,
						core, o_coeff_cap0_adc0_Q);
					cap1 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP1_STAT,
						core, o_coeff_cap1_adc0_Q);
					cap2 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP2_STAT,
						core, o_coeff_cap2_adc0_Q);
					MOD_RADIO_REG_20708(pi, RXADC_CFG9, core,
					    rxadc_coeff_cap0_adc0_Q, cap0);
					MOD_RADIO_REG_20708(pi, RXADC_CFG8, core,
					    rxadc_coeff_cap1_adc0_Q, cap1);
					MOD_RADIO_REG_20708(pi, RXADC_CFG8, core,
					    rxadc_coeff_cap2_adc0_Q, cap2);
				} else if ((rail == 1) && (iq == 0)) {
					cap0 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP0_ADC1_STAT,
						core, o_coeff_cap0_adc1_I);
					cap1 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP1_ADC1_STAT,
						core, o_coeff_cap1_adc1_I);
					cap2 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP2_ADC1_STAT,
						core, o_coeff_cap2_adc1_I);
					MOD_RADIO_REG_20708(pi, RXADC_CFG6, core,
					    rxadc_coeff_cap0_adc1_I, cap0);
					MOD_RADIO_REG_20708(pi, RXADC_CFG4, core,
					    rxadc_coeff_cap1_adc1_I, cap1);
					MOD_RADIO_REG_20708(pi, RXADC_CFG7, core,
					    rxadc_coeff_cap2_adc1_I, cap2);
				} else {
					cap0 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP0_ADC1_STAT,
						core, o_coeff_cap0_adc1_Q);
					cap1 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP1_ADC1_STAT,
						core, o_coeff_cap1_adc1_Q);
					cap2 = READ_RADIO_REGFLD_20708(pi, RF, RXADC_CAP2_ADC1_STAT,
						core, o_coeff_cap2_adc1_Q);
					MOD_RADIO_REG_20708(pi, RXADC_CFG9, core,
					    rxadc_coeff_cap0_adc1_Q, cap0);
					MOD_RADIO_REG_20708(pi, RXADC_CFG4, core,
					    rxadc_coeff_cap1_adc1_Q, cap1);
					MOD_RADIO_REG_20708(pi, RXADC_CFG7, core,
					    rxadc_coeff_cap2_adc1_Q, cap2);
				}

				PHY_INFORM(("core: %d; adc: %d: iq: %d coeff0 = %d \n",
				    core, rail, iq, cap0));
				PHY_INFORM(("core: %d; adc: %d: iq: %d coeff1 = %d \n",
				    core, rail, iq, cap1));
				PHY_INFORM(("core: %d; adc: %d: iq: %d coeff2 = %d \n",
				    core, rail, iq, cap2));
			}
		}
		MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		    rxadc_coeff_sel, 1);
	}

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);
}

static void
wlc_phy_radio20708_afe_cal_pllreg_save(phy_info_t *pi, bool save)
{
	uint8 pllcore;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;
	PHY_TRACE(("wl%d: %s\n", pi->sh->unit, __FUNCTION__));

	if (save) {
		for (pllcore = 0; pllcore < 2; pllcore++) {
			ri->RFP_afediv_cfg1_ovr_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_CFG1_OVR, pllcore);
			ri->RFP_afediv_reg3_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_REG3, pllcore);
			ri->RFP_afediv_reg0_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_REG0, pllcore);
			ri->RFP_afediv_reg4_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_REG4, pllcore);
			ri->RFP_afediv_reg5_orig[pllcore] = READ_RADIO_PLLREG_20708(pi,
				AFEDIV_REG5, pllcore);
		}
	} else {
		for (pllcore = 0; pllcore < 2; pllcore++) {
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pllcore,
				ri->RFP_afediv_cfg1_ovr_orig[pllcore]);
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_REG3, pllcore,
				ri->RFP_afediv_reg3_orig[pllcore]);
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pllcore,
				ri->RFP_afediv_reg0_orig[pllcore]);
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_REG4, pllcore,
				ri->RFP_afediv_reg4_orig[pllcore]);
			WRITE_RADIO_PLLREG_20708(pi, AFEDIV_REG5, pllcore,
				ri->RFP_afediv_reg5_orig[pllcore]);
		}
	}
}

static void
wlc_phy_radio20709_adc_cap_cal(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_adc_cap_cal */

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core, max_chains;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	uint8 cap_cal_iter;
	/* Current 28nm AFE has calibration start issue. WAR is to re-calibrate on fail
	 * (see HW63178-298)
	 */
	const uint8 MAX_CAP_CAL_ITER = 10;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20709_afe_div_ratio(pi, 1);
	}

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
	                        max_chains, max_chains);

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	 *      to get also the first time after a power cycle valid results
	 */
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {

		wlc_phy_radio20709_adc_cap_cal_setup(pi, core);
		for (cap_cal_iter = 0; cap_cal_iter < MAX_CAP_CAL_ITER; cap_cal_iter++) {
			MOD_RADIO_REG_20709(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
			wlc_phy_radio20709_adc_cap_cal_parallel(pi, core, &adccapcal_Timeout);
			if (adccapcal_Timeout > 0) {
				break;
			}
		}

		if (adccapcal_Timeout == 0) {
			PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
					"due to cal TIMEOUT\n", __FUNCTION__, core));
			MOD_RADIO_REG_20709(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
			/* This should never happen, so make sure it is seen in development driver.
			 * In external driver, switch off correction as last resort
			 */
			ASSERT(0);
		} else {
			PHY_INFORM(("%s: adc_cap_cal -- core%d - SUCCESS after "
					"%d iteration(s)\n", __FUNCTION__, core, cap_cal_iter));
			MOD_RADIO_REG_20709(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
		}
		/* Cleanup */
		MOD_RADIO_REG_20709(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
		MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
		MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
		MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	}

	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);
	/* Re-enable low ADC rate if configured */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20709_afe_div_ratio(pi, 0);
	}
}

static void
wlc_phy_radio20710_adc_cap_cal(phy_info_t *pi, uint8 adc)
{
	/* 20710_procs.tcl r773183: 20710_adc_cap_cal */

	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;
	uint8 core, max_chains;
	uint16 adccapcal_Timeout;
	uint16 bbmult = 0;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0;
	uint8 enTx = 0;
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	uint8 cap_cal_iter;

	/* Current 28nm AFE has calibration start issue. WAR is to re-calibrate on fail
	 * (see HW63178-298)
	 */
	const uint8 MAX_CAP_CAL_ITER = 10;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20710_afe_div_ratio(pi, 1, 0, FALSE);
	}

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
	                        max_chains, max_chains);

	/* WAR: Force RFSeq is needed to get rid of time-outs and set_tx_bbmult
	 *      to get also the first time after a power cycle valid results
	 */
	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);
	FOREACH_CORE(pi, core) {
		wlc_phy_set_tx_bbmult_acphy(pi, &bbmult, core);
	}

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);

	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {

		wlc_phy_radio20710_adc_cap_cal_setup(pi, core);
		for (cap_cal_iter = 0; cap_cal_iter < MAX_CAP_CAL_ITER; cap_cal_iter++) {
			MOD_RADIO_REG_20710(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x0);
			wlc_phy_radio20710_adc_cap_cal_parallel(pi, core, adc, &adccapcal_Timeout);
			if (adccapcal_Timeout > 0) {
				break;
			}
		}

		if (adccapcal_Timeout == 0) {
			PHY_ERROR(("%s: adc_cap_cal -- core%d - DISABLED correction, "
					"due to cal TIMEOUT\n", __FUNCTION__, core));
			MOD_RADIO_REG_20710(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x1);
			/* This should never happen, so make sure it is seen in development driver.
			 * In external driver, switch off correction as last resort
			 */
			ASSERT(0);
		} else {
			PHY_INFORM(("%s: adc_cap_cal -- core%d - SUCCESS after "
					"%d iteration(s)\n", __FUNCTION__, core, cap_cal_iter));
			MOD_RADIO_REG_20710(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
		}
		/* Cleanup */
		MOD_RADIO_REG_20710(pi, RXADC_CFG1, core, rxadc_coeff_sel, 0x1);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, afe_en_loopback, 0x0);
		if (adc == 0) {
			MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0x0);
			MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0x0);
		} else {
			MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0x0);
			MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0x0);
		}
	}

	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);
	/* Re-enable low ADC rate if configured */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20710_afe_div_ratio(pi, 0, 0, FALSE);
	}
}

static void
wlc_phy_radio20698_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	/* 20698_procs.tcl r708059: 20698_adc_cap_cal */

	/* Power up DAC by override and put it in reset */
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_reset, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_iqdac_reset, 0x1);

	/* Power up ADC by override */
	MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_puI, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0x1);
	MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_puQ, 0x1);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0x1);
	MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
	MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
	MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);

	/* Enable AFE loopback */
	MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
	/* set DAC Vcm = 0.5*Vdd for calibration (make sure set back to 0 after calibration) */
	MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);

	/* Disconnect ADC from others */
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
	MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
	MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);

	/* for calibration make sure to not disable the correction */
	MOD_RADIO_REG_20698(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
}

static void
wlc_phy_radio20704_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	/* 20704_procs.tcl r785868: 20704_adc_cap_cal */

	/* Power up DAC by override and put it in reset */
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_reset, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_iqdac_reset, 0x1);

	/* Power up ADC by override */
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_puI, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0x1);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_puQ, 0x1);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0x1);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
	MOD_RADIO_REG_20704(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);

	/* Enable AFE loopback */
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
	/* set DAC Vcm = 0.5*Vdd for calibration (make sure set back to 0 after calibration) */
	MOD_RADIO_REG_20704(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);

	/* Disconnect ADC from others */
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
	MOD_RADIO_REG_20704(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
	MOD_RADIO_REG_20704(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);

	/* for calibration make sure to not disable the correction */
	MOD_RADIO_REG_20704(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
}

static void
wlc_phy_radio20707_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	/* Power up DAC by override and put it in reset */
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_reset, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_iqdac_reset, 0x1);

	/* Power up ADC by override */
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_puI, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0x1);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_puQ, 0x1);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0x1);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
	MOD_RADIO_REG_20707(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);

	/* Enable AFE loopback */
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
	/* set DAC Vcm = 0.5*Vdd for calibration (make sure set back to 0 after calibration) */
	MOD_RADIO_REG_20707(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);

	/* Disconnect ADC from others */
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
	MOD_RADIO_REG_20707(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
	MOD_RADIO_REG_20707(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);

	/* for calibration make sure to not disable the correction */
	MOD_RADIO_REG_20707(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
}

static void
wlc_phy_radio20708_afe_cal_setup(phy_info_t *pi, uint8 core, uint8 num_adc_rails)
{
	/* pwrup ADC */
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_rxadc_puI, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_puI, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_rxadc_puQ, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_puQ, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_rxadc_pu_adc_clk, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_pu_adc_clk, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR1, core,
		ovr_rxadc_pu_refI, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_pu_refI, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR1, core,
		ovr_rxadc_pu_refQ, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_pu_refQ, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR1, core,
		ovr_rxadc_pu_refbias, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG0, core,
		rxadc_pu_refbias, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_rxadc0_enb, 1);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc0_enb, 0);
	if (num_adc_rails == 2) {
		MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
			ovr_rxadc1_enb, 1);
		MOD_RADIO_REG_20708(pi, RXADC_CFG5, core,
			rxadc1_enb, 0);
	} else {
		MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
			ovr_rxadc1_enb, 1);
		MOD_RADIO_REG_20708(pi, RXADC_CFG5, core,
			rxadc1_enb, 1);
	}

	/* pwrup DAC */
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_iqdac_pwrup_diode, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_pwrup_diode, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_iqdac_pwrup_I, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_pwrup_I, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_iqdac_pwrup_Q, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_pwrup_Q, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_pu_ibn_8u_1p8bg, 1);
	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_iqdac_buf_pu, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_buf_pu, 1);

	wlc_phy_radio20708_loopback_adc_dac(pi, core);

	MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, core,
		ovr_iqdac_reset, 1);
	MOD_RADIO_REG_20708(pi, TXDAC_REG0, core,
		iqdac_reset, 1);

	/* Cal engine settings */
	MOD_RADIO_REG_20708(pi, RXADC_CFG3, core,
		rxadc_cal_cap, 7);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc_buffer_per, 31);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc_wait_per, 31);
	MOD_RADIO_REG_20708(pi, RXADC_CFG2, core,
		rxadc_sum_per, 127);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc_sum_div, 5);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc_corr_dis, 0);
	MOD_RADIO_REG_20708(pi, RXADC_CFG1, core,
		rxadc_coeff_sel, 0);
	MOD_RADIO_REG_20708(pi, RXADC_REG3, core,
		rxadc_core_ctrl_MSB, 1);
	MOD_RADIO_REG_20708(pi, RXADC_REG2, core,
		rxadc_core_ctrl_LSB, 0x8010);
}

static void
wlc_phy_radio20709_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	/* 20709_procs.tcl r798817: 20709_adc_cap_cal */

	/* Power up DAC by override and put it in reset */
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_reset, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_iqdac_reset, 0x1);

	/* Power up ADC by override */
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_puI, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0x1);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_puQ, 0x1);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0x1);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
	MOD_RADIO_REG_20709(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);

	/* Enable AFE loopback */
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
	/* set DAC Vcm = 0.5*Vdd for calibration (make sure set back to 0 after calibration) */
	MOD_RADIO_REG_20709(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);

	/* Disconnect ADC from others */
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
	MOD_RADIO_REG_20709(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
	MOD_RADIO_REG_20709(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);

	/* for calibration make sure to not disable the correction */
	MOD_RADIO_REG_20709(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
}

static void
wlc_phy_radio20710_adc_cap_cal_setup(phy_info_t *pi, uint8 core)
{
	/* 20710_procs.tcl r785868: 20710_adc_cap_cal */

	/* Power up DAC by override and put it in reset */
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_iqdac_buf_pu, 0x1);
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_diode, 0x1);
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_I, 0x1);
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_iqdac_pwrup_Q, 0x1);
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_reset, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_iqdac_reset, 0x1);

	/* Power up ADC by override */
	MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_puI, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puI, 0x1);
	MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_puQ, 0x1);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_rxadc_puQ, 0x1);
	MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_power_mode_enb, 0x0);
	MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_power_mode, 0x3);
	MOD_RADIO_REG_20710(pi, AFE_CFG1_OVR2, core, ovr_rxadc_power_mode, 0x1);

	/* Enable AFE loopback */
	MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
	/* set DAC Vcm = 0.5*Vdd for calibration (make sure set back to 0 after calibration) */
	MOD_RADIO_REG_20710(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);

	/* Disconnect ADC from others */
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
	MOD_RADIO_REG_20710(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
	MOD_RADIO_REG_20710(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);

	/* for calibration make sure to not disable the correction */
	MOD_RADIO_REG_20710(pi, RXADC_CFG1, core, rxadc_corr_dis, 0x0);
}

static void
wlc_phy_radio20698_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint8 adc,
	uint16 *timeout)
{
	/* 20698_procs.tcl r708059: 20698_adc_cap_cal */

	uint16 adccapcal_Timeout = 400; /* 400 * 10us = 4ms max time to wait */
	uint16 adccapcal1_Timeout = 400; /* 400 * 10us = 4ms max time to wait */
	uint16 adccapcal_done;
	uint16 adccapcal1_done;
//	uint8 icoeff0, qcoeff0, icoeff1, qcoeff1, icoeff2, qcoeff2;

	if (adc == 0) {
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 0x0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 0x1);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 0x1);
		OSL_DELAY(100);
		MOD_RADIO_REG_20698(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc0_I, 0x3);
		MOD_RADIO_REG_20698(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc0_Q, 0x3);
		/* Trigger CAL */
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0x1);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0x1);

		/* Test both I and Q rail done bit */
		adccapcal_done = READ_RADIO_REGFLD_20698(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc0_I);
		adccapcal_done &= READ_RADIO_REGFLD_20698(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc0_Q);
		while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
			OSL_DELAY(10);
			adccapcal_done = READ_RADIO_REGFLD_20698(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc0_I);
			adccapcal_done &= READ_RADIO_REGFLD_20698(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc0_Q);
			adccapcal_Timeout--;
		}
	} else {
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 0x0);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 0x0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 0x1);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 0x1);
		OSL_DELAY(100);
		MOD_RADIO_REG_20698(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc1_I, 0x3);
		MOD_RADIO_REG_20698(pi, RXADC_CFG4, core, rxadc_coeff_out_ctrl_adc1_Q, 0x3);
		/* Trigger CAL */
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0x1);
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0x1);

		/* Test both I and Q rail done bit */
		adccapcal1_done = READ_RADIO_REGFLD_20698(pi, RF, RXADC_CAL_STATUS,
			core, o_cal_done_adc1_I);
		adccapcal1_done &= READ_RADIO_REGFLD_20698(pi, RF, RXADC_CAL_STATUS,
			core, o_cal_done_adc1_Q);
		while ((adccapcal1_done != 1) && (adccapcal1_Timeout > 0)) {
			OSL_DELAY(10);
			adccapcal1_done = READ_RADIO_REGFLD_20698(pi, RF,
				RXADC_CAL_STATUS, core, o_cal_done_adc1_I);
			adccapcal1_done &= READ_RADIO_REGFLD_20698(pi, RF,
				RXADC_CAL_STATUS, core, o_cal_done_adc1_Q);
			adccapcal1_Timeout--;
		}
	}

	if (adccapcal_Timeout == 0) {
		PHY_INFORM(("%s: ADC cap calibration TIMEOUT\n",
			__FUNCTION__));
	}
	if (adccapcal1_Timeout == 0) {
		PHY_INFORM(("%s: ADC1 cap calibration TIMEOUT\n",
			__FUNCTION__));
	}
	*timeout = adccapcal_Timeout;
	if (adc == 0) {
		PHY_INFORM(("core: %d; adc: %d: icoeff0 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG6, core,
				rxadc_coeff_cap0_adc0_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff0 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG9, core,
				rxadc_coeff_cap0_adc0_Q)));
		PHY_INFORM(("core: %d; adc: %d: icoeff1 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG5, core,
				rxadc_coeff_cap1_adc0_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff1 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG8, core,
				rxadc_coeff_cap1_adc0_Q)));
		PHY_INFORM(("core: %d; adc: %d: icoeff2 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG5, core,
				rxadc_coeff_cap2_adc0_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff2 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG8, core,
				rxadc_coeff_cap2_adc0_Q)));
	} else {
		PHY_INFORM(("core: %d; adc: %d: icoeff0 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG6,
				core, rxadc_coeff_cap0_adc1_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff0 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG9,
				core, rxadc_coeff_cap0_adc1_Q)));
		PHY_INFORM(("core: %d; adc: %d: icoeff1 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG4,
				core, rxadc_coeff_cap1_adc1_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff1 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG4,
				core, rxadc_coeff_cap1_adc1_Q)));
		PHY_INFORM(("core: %d; adc: %d: icoeff2 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG7,
				core, rxadc_coeff_cap2_adc1_I)));
		PHY_INFORM(("core: %d; adc: %d: qcoeff2 = %d \n", core, adc,
				READ_RADIO_REGFLD_20698(pi, RF, RXADC_CFG7,
				core, rxadc_coeff_cap2_adc1_Q)));
	}
}

static void
wlc_phy_radio20704_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout)
{
	/* 20704_procs.tcl r773183: 20704_adc_cap_cal */

	/* Calibration takes 1024 ADC clocks with maximum delays. Clock is between 50MHz and 400MHz,
	 * Therefore duration of cal is between 5us and 20us
	 */
	uint16 adccapcal_Timeout = 5; /* 5 * 5us = 25us max time to wait */
	uint16 adccapcal_done;

	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x0);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x0);
	OSL_DELAY(100);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x1);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x1);
	OSL_DELAY(100);
	MOD_RADIO_REG_20704(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_I, 0x3);
	MOD_RADIO_REG_20704(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_Q, 0x3);
	OSL_DELAY(1);
	/* Trigger CAL */
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x1);
	MOD_RADIO_REG_20704(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x1);
	OSL_DELAY(5);

	/* Test both I and Q rail done bit */
	adccapcal_done = READ_RADIO_REGFLD_20704(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_I);
	adccapcal_done &= READ_RADIO_REGFLD_20704(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_Q);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(5);
		adccapcal_done = READ_RADIO_REGFLD_20704(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_I);
		adccapcal_done &= READ_RADIO_REGFLD_20704(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_Q);
		adccapcal_Timeout--;
	}

	if (adccapcal_Timeout == 0) {
		PHY_INFORM(("%s: ADC cap calibration time-out, retrying...\n",
			__FUNCTION__));
	}
	*timeout = adccapcal_Timeout;
	PHY_INFORM(("core: %d; icoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG6, core,
			rxadc_coeff_cap0_adc_I)));
	PHY_INFORM(("core: %d; qcoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG9, core,
			rxadc_coeff_cap0_adc_Q)));
	PHY_INFORM(("core: %d; icoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap1_adc_I)));
	PHY_INFORM(("core: %d; qcoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap1_adc_Q)));
	PHY_INFORM(("core: %d; icoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap2_adc_I)));
	PHY_INFORM(("core: %d; qcoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20704(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap2_adc_Q)));
}

static void
wlc_phy_radio20707_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout)
{
	uint16 adccapcal_Timeout = 400; /* 400 * 10us = 4ms max time to wait */
	uint16 adccapcal1_Timeout = 400; /* 400 * 10us = 4ms max time to wait */
	uint16 adccapcal_done;

	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x0);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x0);
	OSL_DELAY(100);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x1);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x1);
	OSL_DELAY(100);
	MOD_RADIO_REG_20707(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_I, 0x3);
	MOD_RADIO_REG_20707(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_Q, 0x3);
	OSL_DELAY(1);
	/* Trigger CAL */
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x1);
	MOD_RADIO_REG_20707(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x1);
	OSL_DELAY(10);

	/* Test both I and Q rail done bit */
	adccapcal_done = READ_RADIO_REGFLD_20707(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_I);
	adccapcal_done &= READ_RADIO_REGFLD_20707(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_Q);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(10);
		adccapcal_done = READ_RADIO_REGFLD_20707(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_I);
		adccapcal_done &= READ_RADIO_REGFLD_20707(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_Q);
		adccapcal_Timeout--;
	}

	if (adccapcal_Timeout == 0) {
		PHY_ERROR(("%s: ADC cap calibration TIMEOUT\n",
			__FUNCTION__));
	}
	if (adccapcal1_Timeout == 0) {
		PHY_ERROR(("%s: ADC1 cap calibration TIMEOUT\n",
			__FUNCTION__));
	}
	*timeout = adccapcal_Timeout;
	PHY_INFORM(("core: %d; icoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG6, core,
			rxadc_coeff_cap0_adc_I)));
	PHY_INFORM(("core: %d; qcoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG9, core,
			rxadc_coeff_cap0_adc_Q)));
	PHY_INFORM(("core: %d; icoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap1_adc_I)));
	PHY_INFORM(("core: %d; qcoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap1_adc_Q)));
	PHY_INFORM(("core: %d; icoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap2_adc_I)));
	PHY_INFORM(("core: %d; qcoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20707(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap2_adc_Q)));
}

static void
wlc_phy_radio20709_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint16 *timeout)
{
	/* 20709_procs.tcl r798817: 20709_adc_cap_cal */

	/* Calibration takes 1024 ADC clocks with maximum delays. Clock is between 50MHz and 400MHz,
	 * Therefore duration of cal is between 5us and 20us
	 */
	uint16 adccapcal_Timeout = 5; /* 5 * 5us = 25us max time to wait */
	uint16 adccapcal_done;

	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x0);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x0);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x0);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x0);
	OSL_DELAY(100);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_reset_n_adc_I, 0x1);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_reset_n_adc_Q, 0x1);
	OSL_DELAY(100);
	MOD_RADIO_REG_20709(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_I, 0x3);
	MOD_RADIO_REG_20709(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc_Q, 0x3);
	OSL_DELAY(1);
	/* Trigger CAL */
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_I, 0x1);
	MOD_RADIO_REG_20709(pi, RXADC_CFG0, core, rxadc_start_cal_adc_Q, 0x1);
	OSL_DELAY(5);

	/* Test both I and Q rail done bit */
	adccapcal_done = READ_RADIO_REGFLD_20709(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_I);
	adccapcal_done &= READ_RADIO_REGFLD_20709(pi, RF, RXADC_CAL_STATUS,
		core, o_rxadc_cal_done_adc_Q);
	while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
		OSL_DELAY(5);
		adccapcal_done = READ_RADIO_REGFLD_20709(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_I);
		adccapcal_done &= READ_RADIO_REGFLD_20709(pi, RF,
			RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc_Q);
		adccapcal_Timeout--;
	}

	if (adccapcal_Timeout == 0) {
		PHY_INFORM(("%s: ADC cap calibration time-out, retrying...\n",
			__FUNCTION__));
	}
	*timeout = adccapcal_Timeout;
	PHY_INFORM(("core: %d; icoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG6, core,
			rxadc_coeff_cap0_adc_I)));
	PHY_INFORM(("core: %d; qcoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG9, core,
			rxadc_coeff_cap0_adc_Q)));
	PHY_INFORM(("core: %d; icoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap1_adc_I)));
	PHY_INFORM(("core: %d; qcoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap1_adc_Q)));
	PHY_INFORM(("core: %d; icoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap2_adc_I)));
	PHY_INFORM(("core: %d; qcoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20709(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap2_adc_Q)));
}

static void
wlc_phy_radio20710_adc_cap_cal_parallel(phy_info_t *pi, uint8 core, uint8 adc, uint16 *timeout)
{
	/* 20710_procs.tcl r773183: 20710_adc_cap_cal */

	/* Calibration takes 1024 ADC clocks with maximum delays. Clock is between 50MHz and 400MHz,
	 * Therefore duration of cal is between 5us and 20us
	 */
	uint16 adccapcal_Timeout = 5; /* 5 * 5us = 25us max time to wait */
	uint16 adccapcal1_Timeout = 5; /* 5 * 5us = 25us max time to wait */
	uint16 adccapcal_done;
	uint16 adccapcal1_done;

	if (adc == 0) {
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 0x0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_I, 0x1);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc0_Q, 0x1);
		OSL_DELAY(100);
		MOD_RADIO_REG_20710(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc0_I, 0x3);
		MOD_RADIO_REG_20710(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc0_Q, 0x3);
		OSL_DELAY(1);
		/* Trigger CAL */
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_I, 0x1);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc0_Q, 0x1);
		OSL_DELAY(5);

		/* Test both I and Q rail done bit */
		adccapcal_done = READ_RADIO_REGFLD_20710(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc0_I);
		adccapcal_done &= READ_RADIO_REGFLD_20710(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc0_Q);
		while ((adccapcal_done != 1) && (adccapcal_Timeout > 0)) {
			OSL_DELAY(5);
			adccapcal_done = READ_RADIO_REGFLD_20710(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc0_I);
			adccapcal_done &= READ_RADIO_REGFLD_20710(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc0_Q);
			adccapcal_Timeout--;
		}
	} else {
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 0x0);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 0x0);
		OSL_DELAY(100);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_I, 0x1);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_reset_n_adc1_Q, 0x1);
		OSL_DELAY(100);
		MOD_RADIO_REG_20710(pi, RXADC_CFG3, core, rxadc_coeff_out_ctrl_adc1_I, 0x3);
		MOD_RADIO_REG_20710(pi, RXADC_CFG4, core, rxadc_coeff_out_ctrl_adc1_Q, 0x3);
		OSL_DELAY(1);
		/* Trigger CAL */
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_I, 0x1);
		MOD_RADIO_REG_20710(pi, RXADC_CFG0, core, rxadc_start_cal_adc1_Q, 0x1);
		OSL_DELAY(5);

		/* Test both I and Q rail done bit */
		adccapcal1_done = READ_RADIO_REGFLD_20710(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc1_I);
		adccapcal1_done &= READ_RADIO_REGFLD_20710(pi, RF, RXADC_CAL_STATUS,
			core, o_rxadc_cal_done_adc1_Q);
		while ((adccapcal1_done != 1) && (adccapcal1_Timeout > 0)) {
			OSL_DELAY(5);
			adccapcal1_done = READ_RADIO_REGFLD_20710(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc1_I);
			adccapcal1_done &= READ_RADIO_REGFLD_20710(pi, RF,
				RXADC_CAL_STATUS, core, o_rxadc_cal_done_adc1_Q);
			adccapcal1_Timeout--;
		}
	}

	if (adccapcal_Timeout == 0) {
		PHY_INFORM(("%s: ADC0 cap calibration time-out, retrying...\n",
			__FUNCTION__));
	}
	if (adccapcal1_Timeout == 0) {
		PHY_INFORM(("%s: ADC1 cap calibration time-out, retrying...\n",
			__FUNCTION__));
	}

	if (adc == 0) {
		*timeout = adccapcal_Timeout;
		PHY_INFORM(("core: %d; icoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG6, core,
			rxadc_coeff_cap0_adc0_I)));
		PHY_INFORM(("core: %d; qcoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG9, core,
			rxadc_coeff_cap0_adc0_Q)));
		PHY_INFORM(("core: %d; icoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap1_adc0_I)));
		PHY_INFORM(("core: %d; qcoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap1_adc0_Q)));
		PHY_INFORM(("core: %d; icoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG5, core,
			rxadc_coeff_cap2_adc0_I)));
		PHY_INFORM(("core: %d; qcoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG8, core,
			rxadc_coeff_cap2_adc0_Q)));
	} else {
		*timeout = adccapcal1_Timeout;
		PHY_INFORM(("core: %d; icoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG6, core,
			rxadc_coeff_cap0_adc1_I)));
		PHY_INFORM(("core: %d; qcoeff0 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG9, core,
			rxadc_coeff_cap0_adc1_Q)));
		PHY_INFORM(("core: %d; icoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG4, core,
			rxadc_coeff_cap1_adc1_I)));
		PHY_INFORM(("core: %d; qcoeff1 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG4, core,
			rxadc_coeff_cap1_adc1_Q)));
		PHY_INFORM(("core: %d; icoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG7, core,
			rxadc_coeff_cap2_adc1_I)));
		PHY_INFORM(("core: %d; qcoeff2 = %d \n", core,
			READ_RADIO_REGFLD_20710(pi, RF, RXADC_CFG7, core,
			rxadc_coeff_cap2_adc1_Q)));
	}
}

static void
wlc_phy_radio20698_apply_adc_cal_result(phy_info_t *pi, uint8 core, uint8 adc)
{
	/* 20698_procs.tcl r708059: 20698_apply_adc_cal_result */

}

static void
wlc_phy_radio20698_tiadc_cal(phy_info_t *pi)
{
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0, enTx = 0, max_chains;
	uint8 core;
	uint8 mac_suspend;

	uint16 p, q, DepthCount;

	mac_suspend = (R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

	if (mac_suspend) wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

	/* Disable low adc rate if configured, since the ADC calibration engine works on
	 * the ADC clock
	 */
	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
		wlc_phy_radio20698_afe_div_ratio(pi, 1, 0, 0);
	}

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
		max_chains, max_chains);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);

	/* Save AFE registers */
	phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);

	// Set HS Sample Play @ DAC freq / (q/p) ~ 3MHz
	p = 1; q = 256;
	wlc_phy_radio20698_hssmpl_buffer_setup(pi, p, q);

	/* Play 8MHz tx single tone */
	wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_8MHz, 151, TX_TONE_IQCAL_MODE_OFF, TRUE);

	/* HS Sample Play, wait 10us to take effect */
	DepthCount = (uint16)(q/p);
	DepthCount = (DepthCount >> 1) - 1;
	wlc_phy_radio20698_hs_loopback(pi, FALSE, DepthCount);
	OSL_DELAY(10);

	/* Main TI-ADC cal function, per core */
	FOREACH_CORE(pi, core) {
		wlc_phy_radio20698_tiadc_cal_parallel(pi, core);
	}

	/* Stop HS Sample Play */
	MOD_PHYREG(pi, SamplePlayHighSpeed, start, 0x0);
	MOD_PHYREG(pi, SamplePlayHighSpeed, enable, 0x0);

	/* Stop playing 8MHz tx single tone */
	wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

	/* Restore AFE registers */
	phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);

	/* Restore Rx and Tx chains */
	wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20698_afe_div_ratio(pi, 0, 0, 0);
	}

	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	if (mac_suspend) wlapi_enable_mac(pi->sh->physhim);

	PHY_INFORM(("%s: TI ADC Calibration Finish\n", __FUNCTION__));
}

static void
wlc_phy_radio20708_tiadc_cal(phy_info_t *pi)
{
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);
	phy_info_acphy_t *pi_ac = (phy_info_acphy_t *)pi->u.pi_acphy;

	/* Local copy of phyrxchains & EnTx bits before overwrite */
	uint8 enRx = 0, enTx = 0, max_chains, cal_cores, core;
	uint8 mac_suspend;
	uint8 retry = 0;

	uint16 p, q, max_val, DepthCount;

	mac_suspend = (R_REG(pi->sh->osh, D11_MACCONTROL(pi)) & MCTL_EN_MAC);

	if (mac_suspend) wlapi_suspend_mac_and_wait(pi->sh->physhim);
	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, TRUE);

	/* Always force normal adc rate during tiadc cal, since the ADC calibration engine
	 * works on the ADC clock
	 */
	wlc_phy_low_rate_adc_enable_acphy(pi, FALSE);
	wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 1, 0);

	/* Save and overwrite chains, Cals over all possible t/rx cores */
	max_chains = stf_shdata->hw_phytxchain | stf_shdata->hw_phyrxchain;
	wlc_phy_update_rxchains((wlc_phy_t *)pi, &enRx, &enTx,
		max_chains, max_chains);
	cal_cores = max_chains;

	wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_RX2TX);

	while ((retry < 3) && (cal_cores > 0)) {
		/* Save AFE registers */
		phy_ac_reg_cache_save(pi_ac, RADIOREGS_AFECAL);

		// Set HS Sample Play @ DAC freq / (q/p) ~ 3MHz
		p = 1; q = 256;
		// Slightly reduce tone magnitude for further retry
		max_val = 2048 - (retry << 7);
		wlc_phy_radio20708_hssmpl_buffer_setup(pi, p, q, max_val);

		/* Play 8MHz tx single tone */
		wlc_phy_tx_tone_acphy(pi, ACPHY_IQCAL_TONEFREQ_8MHz,
		                      151, TX_TONE_IQCAL_MODE_OFF, TRUE);

		/* HS Sample Play, wait 10us to take effect */
		DepthCount = (uint16)(q/p);
		DepthCount = (DepthCount >> 1) - 1;
		wlc_phy_radio20708_hs_loopback(pi, FALSE, DepthCount);
		OSL_DELAY(10);

		/* Main TI-ADC cal function for all cores */
		cal_cores = wlc_phy_radio20708_tiadc_cal_parallel(pi, cal_cores);

		/* Stop HS Sample Play */
		MOD_PHYREG(pi, SamplePlayHighSpeed, start, 0x0);
		MOD_PHYREG(pi, SamplePlayHighSpeed, enable, 0x0);

		/* Stop playing 8MHz tx single tone */
		wlc_phy_stopplayback_acphy(pi, STOPPLAYBACK_W_CCA_RESET);

		/* Restore AFE registers */
		phy_ac_reg_cache_restore(pi_ac, RADIOREGS_AFECAL);

		wlc_phy_force_rfseq_acphy(pi, ACPHY_RFSEQ_TX2RX);

		/* Restore Rx and Tx chains */
		wlc_phy_restore_rxchains((wlc_phy_t *)pi, enRx, enTx);

		retry++;
	}

	if ((retry == 3) && (cal_cores > 0)) {
		FOREACH_CORE(pi, core) {
			if (cal_cores & (1 << core)) {
				PHY_ERROR(("%s: No valid CompGain found for core %d"
				    " after retry, overwriting 0x7fff\n", __FUNCTION__, core));
				WRITE_PHYREGCE(pi, sarAfeCompGainIOvrVal, core, 0x7fff);
				WRITE_PHYREGCE(pi, sarAfeCompGainQOvrVal, core, 0x7fff);
			}
		}
	}

	if (pi->u.pi_acphy->sromi->srom_low_adc_rate_en) {
		wlc_phy_low_rate_adc_enable_acphy(pi, TRUE);
		wlc_phy_radio20708_tx2cal_normal_adc_rate(pi, 0, 0);
	}

	phy_rxgcrs_stay_in_carriersearch(pi->rxgcrsi, FALSE);
	if (mac_suspend) wlapi_enable_mac(pi->sh->physhim);

	PHY_INFORM(("%s: TI ADC Calibration Finish\n", __FUNCTION__));
}

static void
wlc_phy_radio20698_tiadc_cal_parallel(phy_info_t *pi, uint8 core)
{
	// Accumulate 2^Accum = 64 cycles for cal
	// It uses 64 * 256 samples @750MHz ~ 21us
	// Numerically, we need 1ms to have stable results
	uint8 Acuum = 6;
	// wait 15 samples for ADC to settle
	uint8 settleT = 15;
	uint16 tiadc_cal_Timeout = 400; // 400 * 0.2ms* 2 = 160ms max time to wait
	uint16 tiadc_cal_done;
	uint16 tiadc_cal_ite;
	bool tiadc_cal_success = TRUE;
	uint16 IOver1 = 0, IOver2 = 0, IOver3 = 0;
	uint16 QOver1 = 0, QOver2 = 0, QOver3 = 0;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* Reset the previous TI-ADC override setting */
	MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x0);
	MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x0);
	MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x0);
	MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x0);

	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_settle_time, settleT);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_acc_cnt, Acuum);

	/* For I component */
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x1);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_iq_sel, 0x1);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x1);
	for (tiadc_cal_ite = 0; tiadc_cal_ite < tiadc_cal_Timeout; tiadc_cal_ite++) {
		MOD_PHYREGCE(pi, AfeCalConfig, core, start_cal, 0x1);
		OSL_DELAY(200);

		tiadc_cal_done = READ_PHYREGFLDCE(pi, AfeCalConfig4, core, adc_cal_done);
		if (tiadc_cal_done == 1) {
			IOver1 = READ_PHYREGCE(pi, AfeCalConfig2, core);
			IOver2 = READ_PHYREGCE(pi, AfeCalConfig3, core);
			IOver3 = READ_PHYREGCE(pi, AfeCalConfig4, core);

			PHY_INFORM(("I-TI ADC Calibration done for core %d after %d iteration\n",
				core, tiadc_cal_ite));
			PHY_INFORM(("AfeCalconfig2%d is: 0x%04x\n", core, IOver1));
			PHY_INFORM(("AfeCalConfig3%d is: 0x%04x\n", core, IOver2));
			PHY_INFORM(("AfeCalconfig4%d is: 0x%04x\n", core, IOver3));
			break;
		}
	}
	if (tiadc_cal_done == 0) {
		tiadc_cal_success = FALSE;
		PHY_ERROR(("%s: I-TI ADC Calibration Failed For core %d EVERY TIME!\n",
			__FUNCTION__, core));
	}

	/* For Q component */
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x1);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_iq_sel, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x1);
	for (tiadc_cal_ite = 0; tiadc_cal_ite < tiadc_cal_Timeout; tiadc_cal_ite++) {
		MOD_PHYREGCE(pi, AfeCalConfig, core, start_cal, 0x1);
		OSL_DELAY(200);
		tiadc_cal_done = READ_PHYREGFLDCE(pi, AfeCalConfig4, core, adc_cal_done);
		if (tiadc_cal_done == 1) {
			QOver1 = READ_PHYREGCE(pi, AfeCalConfig2, core);
			QOver2 = READ_PHYREGCE(pi, AfeCalConfig3, core);
			QOver3 = READ_PHYREGCE(pi, AfeCalConfig4, core);

			PHY_INFORM(("Q-TI ADC Calibration done for core %d after %d iteration\n",
				core, tiadc_cal_ite));
			PHY_INFORM(("AfeCalconfig2%d is: 0x%04x\n", core, QOver1));
			PHY_INFORM(("AfeCalConfig3%d is: 0x%04x\n", core, QOver2));
			PHY_INFORM(("AfeCalconfig4%d is: 0x%04x\n", core, QOver3));
			break;
		}
	}
	if (tiadc_cal_done == 0) {
		tiadc_cal_success = FALSE;
		PHY_ERROR(("%s: Q-TI ADC Calibration Failed For core %d EVERY TIME!\n",
			__FUNCTION__, core));
	}

	/* Apply Calibration if seccessful for both I and Q: I and Q are swapped */
	if (tiadc_cal_success) {
		WRITE_PHYREGCE(pi, sarAfeCompGainIOvrVal, core, QOver1);
		WRITE_PHYREGCE(pi, sarAfeCompDC0IOvrVal, core, QOver2);
		WRITE_PHYREGCE(pi, sarAfeCompDC1IOvrVal, core, QOver3 & 0x1fff);
		WRITE_PHYREGCE(pi, sarAfeCompGainQOvrVal, core, IOver1);
		WRITE_PHYREGCE(pi, sarAfeCompDC0QOvrVal, core, IOver2);
		WRITE_PHYREGCE(pi, sarAfeCompDC1QOvrVal, core, IOver3 & 0x1fff);
		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_q, (IOver3 & 0x4000) >> 14);
		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_i, (QOver3 & 0x4000) >> 14);

		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
		MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x1);
	}
	MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x0);
}

static uint8
wlc_phy_radio20708_tiadc_cal_parallel(phy_info_t *pi, uint8 coremask)
{
	// Accumulate 2^Accum = 64 cycles for cal
	// It uses 64 * 256 samples @750MHz ~ 21us
	// Numerically, we need 1ms to have stable results
	uint8 Acuum = 6;
	// wait 15 samples for ADC to settle
	uint8 settleT = 15;
	uint8 startCore, core;
	uint16 tiadc_cal_Timeout = 400; // 400 * 0.2ms* 2 = 160ms max time to wait
	uint16 tiadc_cal_done;
	uint16 tiadc_cal_ite;
	bool tiadc_cal_success = TRUE;
	uint16 IOver1[4] = {0, 0, 0, 0}, IOver2[4] = {0, 0, 0, 0}, IOver3[4] = {0, 0, 0, 0};
	uint16 QOver1[4] = {0, 0, 0, 0}, QOver2[4] = {0, 0, 0, 0}, QOver3[4] = {0, 0, 0, 0};
	uint8 retry_cores = 0;

	// 24576 = 2^15 *3/4, catching not only known wrap-around issue CRBCAD11PHY-4079
	// also other apparently wrong values
	uint16 valid_compgain_thresh = 24576;

	PHY_INFORM(("%s\n", __FUNCTION__));

	if (pi->pubpi->phy_coremask & 0x1) {
		startCore = 0;
	} else if (pi->pubpi->phy_coremask & 0x2) {
		startCore = 1;
	} else if (pi->pubpi->phy_coremask & 0x4) {
		startCore = 2;
	} else {
		startCore = 3;
	}

	/* For I component */
	FOREACH_CORE(pi, core) {
		if (coremask & (1 << core)) {
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_iq_sel, 0x1);
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_settle_time, settleT);
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_acc_cnt, Acuum);
			/* Reset the previous TI-ADC override setting */
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x1);
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x0);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x0);
		}
	}

	/* enabling, and starting the engine based on start Core */
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, adc_cal_en, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, adc_cal_en, 0x1);
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, start_cal, 0x1);

	FOREACH_CORE(pi, core) {
		if (coremask & (1 << core)) {
			for (tiadc_cal_ite = 0;
			     tiadc_cal_ite < tiadc_cal_Timeout;
			     tiadc_cal_ite++) {
				OSL_DELAY(200);

				tiadc_cal_done =
				    READ_PHYREGFLDCE(pi, AfeCalConfig4, core, adc_cal_done);
				if (tiadc_cal_done == 1) {
					IOver1[core] = READ_PHYREGCE(pi, AfeCalConfig2, core);
					IOver2[core] = READ_PHYREGCE(pi, AfeCalConfig3, core);
					IOver3[core] = READ_PHYREGCE(pi, AfeCalConfig4, core);

					PHY_INFORM(
					    ("I-TI ADC Calibration done for core %d after %d"
						"iteration\n", core, tiadc_cal_ite));
					PHY_INFORM(("AfeCalconfig2%d is: 0x%04x\n",
					            core, IOver1[core]));
					PHY_INFORM(("AfeCalConfig3%d is: 0x%04x\n",
					            core, IOver2[core]));
					PHY_INFORM(("AfeCalconfig4%d is: 0x%04x\n",
					            core, IOver3[core]));
					break;
				}
			}
			if (tiadc_cal_done == 0) {
				tiadc_cal_success = FALSE;
				PHY_ERROR(("%s: I-TI ADC Calibration Failed For core %d"
				    " EVERY TIME!\n", __FUNCTION__, core));
			}
		}
	}

	/* For Q component */
	FOREACH_CORE(pi, core) {
		if (coremask & (1 << core)) {
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_iq_sel, 0x0);
			/* Reset the previous TI-ADC override setting */
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x1);
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_clear, 0x0);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x0);
		}
	}

	/* enabling, and starting the engine based on start Core */
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, adc_cal_en, 0x0);
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, adc_cal_en, 0x1);
	MOD_PHYREGCE(pi, AfeCalConfig, startCore, start_cal, 0x1);

	FOREACH_CORE(pi, core) {
		if (coremask & (1 << core)) {
			for (tiadc_cal_ite = 0;
			     tiadc_cal_ite < tiadc_cal_Timeout;
			     tiadc_cal_ite++) {
				OSL_DELAY(200);
				tiadc_cal_done =
				    READ_PHYREGFLDCE(pi, AfeCalConfig4, core, adc_cal_done);
				if (tiadc_cal_done == 1) {
					QOver1[core] = READ_PHYREGCE(pi, AfeCalConfig2, core);
					QOver2[core] = READ_PHYREGCE(pi, AfeCalConfig3, core);
					QOver3[core] = READ_PHYREGCE(pi, AfeCalConfig4, core);

					PHY_INFORM(("Q-TI ADC Calibration done for core %d after %d"
						"iteration\n", core, tiadc_cal_ite));
					PHY_INFORM(("AfeCalconfig2%d is: 0x%04x\n",
					            core, QOver1[core]));
					PHY_INFORM(("AfeCalConfig3%d is: 0x%04x\n",
					            core, QOver2[core]));
					PHY_INFORM(("AfeCalconfig4%d is: 0x%04x\n",
					            core, QOver3[core]));
					break;
				}
			}
			if (tiadc_cal_done == 0) {
				tiadc_cal_success = FALSE;
				PHY_ERROR(("%s: Q-TI ADC Calibration Failed For core %d"
				    " EVERY TIME!\n", __FUNCTION__, core));
			}
		}
	}

	/* Apply Calibration if seccessful for both I and Q: I and Q are swapped */
	FOREACH_CORE(pi, core) {
		if (coremask & (1 << core)) {
			if (tiadc_cal_success) {
				WRITE_PHYREGCE(pi, sarAfeCompDC0IOvrVal, core, QOver2[core]);
				WRITE_PHYREGCE(pi, sarAfeCompDC1IOvrVal, core,
				    QOver3[core] & 0x1fff);
				WRITE_PHYREGCE(pi, sarAfeCompDC0QOvrVal, core, IOver2[core]);
				WRITE_PHYREGCE(pi, sarAfeCompDC1QOvrVal, core,
				    IOver3[core] & 0x1fff);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_q,
					(IOver3[core] & 0x4000) >> 14);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_i,
					(QOver3[core] & 0x4000) >> 14);

				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x1);

				/* check validity of cal results */
				if ((QOver1[core] < valid_compgain_thresh) ||
				    (IOver1[core] < valid_compgain_thresh)) {
					retry_cores |= (1 << core);
				} else {
					WRITE_PHYREGCE(pi, sarAfeCompGainIOvrVal, core,
					    QOver1[core]);
					WRITE_PHYREGCE(pi, sarAfeCompGainQOvrVal, core,
					    IOver1[core]);
				}
			} else {
				retry_cores |= (1 << core);
			}
			MOD_PHYREGCE(pi, AfeCalConfig, core, adc_cal_en, 0x0);
		}
	}

	return retry_cores;
}

static void
wlc_phy_radio20698_loopback_adc_dac(phy_info_t *pi)
{
	uint8 core;
	FOREACH_CORE(pi, core) {
		/* Enable AFE loopback */
		MOD_RADIO_REG_20698(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
		// set DAC Vcm = 0.5*Vdd for calibration
		// make sure set back to 0 after calibration
		MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_cmsel, 0x1);
		MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x3);
		MOD_RADIO_REG_20698(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 0x50);

		/* Disconnect ADC from others */
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
		MOD_RADIO_REG_20698(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
		MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);
	}
}

static void
wlc_phy_radio20708_loopback_adc_dac(phy_info_t *pi, uint8 core_num)
{
	uint8 core;
	uint8 core_start;
	uint8 core_end;

	/* single-core loopback setup if "core_num < 4",
	*  all-core loopback setup if "core_num =4".
	*/
	if (core_num < 4) {
		core_start = core_num;
		core_end = core_num+1;
	} else {
		core_start = 0;
		core_end = core_num;
	}

	for (core = core_start; core < core_end; core++) {
		/* Enable AFE loopback */
		MOD_RADIO_REG_20708(pi, RXADC_CFG0, core, afe_en_loopback, 0x1);
		// set DAC Vcm = 0.5*Vdd for calibration
		// make sure set back to 0 after calibration
		MOD_RADIO_REG_20708(pi, RXADC_CFG10, core, rxadc_loopbacksw_series_en, 0x3);
		MOD_RADIO_REG_20708(pi, TXDAC_REG7, core, iqdac_buf_bias_cur, 0x4);
		MOD_RADIO_REG_20708(pi, TXDAC_REG0, core, iqdac_buf_cc, 0x3);
		MOD_RADIO_REG_20708(pi, TXDAC_REG1, core, iqdac_buf_Cfb, 0x50);
		MOD_RADIO_REG_20708(pi, TXDAC_REG3, core, iqdac_buf_cmsel, 0x1);
		MOD_RADIO_REG_20708(pi, TXDAC_REG0, core, iqdac_pu_ibn_8u_1p8bg, 0x1);

		/* Disconnect ADC from others */
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_bq2_adc, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_adc, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_bq2_rc, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_sw_bq2_rc, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_dac_bq2, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_sw_dac_bq2, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_dac_rc, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_sw_dac_rc, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_bq1_adc, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_adc, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_bq1_bq2, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR2, core, ovr_lpf_sw_bq1_bq2, 0x1);
		MOD_RADIO_REG_20708(pi, LPF_REG7, core, lpf_sw_aux_adc, 0x0);
		MOD_RADIO_REG_20708(pi, LPF_OVR2, core, ovr_lpf_sw_aux_adc, 0x1);
	}
}

static void
wlc_phy_radio20698_hssmpl_buffer_setup(phy_info_t *pi, uint16 p, uint16 q)
{
	uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
		ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
	uint8 core;
	uint32 entry, offset, len;
	int32 real, imag;
	uint32 val;

	math_cint32 sin_cos_val;
	math_fixed theta = 0, rot = 0;
	uint16 max_val = 2048;
	bool padp_calEnb[PHY_CORE_MAX];

	PHY_INFORM(("%s\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		/* To write ables in epsilon_table_ids, dac clk power up WAR */
		MOD_PHYREGCE(pi, wbcal_ctl_21, core, wb_mem_access_sel, 0);
		MOD_PHYREGCE(pi, papdEpsilonTable, core, mem_access_sel, 0);

		padp_calEnb[core] = READ_PHYREGFLDCEE(pi, PapdCalShifts, core, papd_calEnb);
		MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 0);
	}

	val = 0; len = 512;
	for (entry = 0; entry < len; entry++) {
		offset = entry & 0x1ff;
		FOREACH_CORE(pi, core) {
			wlc_phy_table_write_acphy(pi, epsilon_table_ids[core], 1, offset, 32, &val);
		}
	}
	PHY_INFORM(("%s: Even/Odd Tables reset\n", __FUNCTION__));

	/* Each sample varies 2*pi*p/q rad */
	rot = FIXED((p * 360)) / q;
	theta = 0;

	len = (uint16)(q/p);
	for (entry = 0; entry < len; entry++) {
		offset = entry & 0x1ff;

		/* compute phasor */
		math_cmplx_cordic(theta, &sin_cos_val);

		real = (uint16)(FLOAT(sin_cos_val.i * max_val) & 0x1fff);
		imag = (uint16)(FLOAT(sin_cos_val.q * max_val) & 0x1fff);
		val = (imag << 13) | real;

		FOREACH_CORE(pi, core) {
			wlc_phy_table_write_acphy(pi, epsilon_table_ids[core], 1, offset, 32, &val);
		}
		theta += rot;
	}
	PHY_INFORM(("%s: Even/Odd Tables written\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, wbcal_ctl_21, core, wb_mem_access_sel, 1);
		MOD_PHYREGCE(pi, papdEpsilonTable, core, mem_access_sel, 1);

		MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, padp_calEnb[core]);
	}
}

void
wlc_phy_radio20708_hssmpl_buffer_setup(phy_info_t *pi, uint16 p, uint16 q, uint16 max_val)
{
	uint8 epsilon_table_ids[] = { ACPHY_TBL_ID_EPSILON0, ACPHY_TBL_ID_EPSILON1,
		ACPHY_TBL_ID_EPSILON2, ACPHY_TBL_ID_EPSILON3};
	uint8 core;
	uint32 entry, offset, len;
	int32 real, imag;
	uint32 val;

	math_cint32 sin_cos_val;
	math_fixed theta = 0, rot = 0;
	bool padp_calEnb[PHY_CORE_MAX];

	PHY_INFORM(("%s\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		/* To write ables in epsilon_table_ids, dac clk power up WAR */
		MOD_PHYREGCE(pi, wbcal_ctl_21, core, wb_mem_access_sel, 0);
		MOD_PHYREGCE(pi, papdEpsilonTable, core, mem_access_sel, 0);

		padp_calEnb[core] = READ_PHYREGFLDCEE(pi, PapdCalShifts, core, papd_calEnb);
		MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, 0);
	}

	val = 0; len = 512;
	for (entry = 0; entry < len; entry++) {
		offset = entry & 0x1ff;
		FOREACH_CORE(pi, core) {
			wlc_phy_table_write_acphy(pi, epsilon_table_ids[core], 1, offset, 32, &val);
		}
	}
	PHY_INFORM(("%s: Even/Odd Tables reset\n", __FUNCTION__));

	/* Each sample varies 2*pi*p/q rad */
	rot = FIXED((p * 360)) / q;
	theta = 0;

	len = (uint16)(q/p);
	for (entry = 0; entry < len; entry++) {
		offset = entry & 0x1ff;

		/* compute phasor */
		math_cmplx_cordic(theta, &sin_cos_val);

		real = (uint16)(FLOAT(sin_cos_val.i * max_val) & 0x1fff);
		imag = (uint16)(FLOAT(sin_cos_val.q * max_val) & 0x1fff);
		val = (imag << 13) | real;

		FOREACH_CORE(pi, core) {
			wlc_phy_table_write_acphy(pi, epsilon_table_ids[core], 1, offset, 32, &val);
		}
		theta += rot;
	}
	PHY_INFORM(("%s: Even/Odd Tables written\n", __FUNCTION__));

	FOREACH_CORE(pi, core) {
		MOD_PHYREGCE(pi, wbcal_ctl_21, core, wb_mem_access_sel, 1);
		MOD_PHYREGCE(pi, papdEpsilonTable, core, mem_access_sel, 1);

		MOD_PHYREGCEE(pi, PapdCalShifts, core, papd_calEnb, padp_calEnb[core]);
	}
}

static void
wlc_phy_radio20698_hs_loopback(phy_info_t *pi, bool swap_ab, uint16 DepthCount)
{
	uint8 core;

	PHY_INFORM(("%s\n", __FUNCTION__));

	/* set the specific settings for high speed sample play */
	MOD_PHYREG(pi, Core1TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core2TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core3TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core4TxControl, loft_comp_swap_ab, swap_ab);

	MOD_PHYREG(pi, SamplePlayHighSpeed, enable, 0x1);
	MOD_PHYREG(pi, SamplePlayHighSpeed, DacTestMode, 0x1);
	MOD_PHYREG(pi, sampleLoopCountHighSpeed, LoopCount, 0xffff);
	MOD_PHYREG(pi, SPB_remainderHighSpeed, remainder, 0x0);
	MOD_PHYREG(pi, sampleStartAddrHighSpeed, startAddr, 0x0);
	MOD_PHYREG(pi, sampleDepthCountHighSpeed, DepthCount, DepthCount);

	/* Start sample play */
	MOD_PHYREG(pi, SamplePlayHighSpeed, start, 0x1);

	wlc_phy_radio20698_loopback_adc_dac(pi);

	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20698(pi, TXDAC_REG0, core, iqdac_buf_bw, 0x3);
	}
}

void
wlc_phy_radio20708_hs_loopback(phy_info_t *pi, bool swap_ab, uint16 DepthCount)
{
	PHY_INFORM(("%s\n", __FUNCTION__));

	/* set the specific settings for high speed sample play */
	MOD_PHYREG(pi, Core1TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core2TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core3TxControl, loft_comp_swap_ab, swap_ab);
	MOD_PHYREG(pi, Core4TxControl, loft_comp_swap_ab, swap_ab);

	MOD_PHYREG(pi, SamplePlayHighSpeed, enable, 0x1);
	MOD_PHYREG(pi, SamplePlayHighSpeed, DacTestMode, 0x1);
	MOD_PHYREG(pi, sampleLoopCountHighSpeed, LoopCount, 0xffff);
	MOD_PHYREG(pi, SPB_remainderHighSpeed, remainder, 0x0);
	MOD_PHYREG(pi, sampleStartAddrHighSpeed, startAddr, 0x0);
	MOD_PHYREG(pi, sampleDepthCountHighSpeed, DepthCount, DepthCount);

	/* Start sample play */
	MOD_PHYREG(pi, SamplePlayHighSpeed, start, 0x1);

	wlc_phy_radio20708_loopback_adc_dac(pi, 4);
}

static void
wlc_phy_radio20698_adc_offset_gain_cal(phy_info_t *pi)
{
	/* 20698_procs.tcl r708059: 20698_adc_offset_gain_cal */

	/* Disable TI-ADC since no much improvement is observed */
	if (0) {
		uint8 core;

		/* For phybw = 160M, do TI-ADC cal */
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			wlc_phy_radio20698_tiadc_cal(pi);
		} else {
			/* Reset the previous TI-ADC override setting */
			FOREACH_CORE(pi, core) {
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
				MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x0);
			}
		}
	}
}

static void
wlc_phy_radio20708_adc_offset_gain_cal(phy_info_t *pi)
{
	uint8 core;

	/* For phybw = 160M, do TI-ADC cal */
	if (CHSPEC_IS160(pi->radio_chanspec)) {
		wlc_phy_radio20708_tiadc_cal(pi);
	} else {
		/* Reset the previous TI-ADC override setting */
		FOREACH_CORE(pi, core) {
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc0_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompDc1_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompGain_ovr, 0x1);
			MOD_PHYREGCE(pi, sarAfeCompCtrl, core, sarAfeCompSel_ovr, 0x0);
		}
	}
}

void
wlc_phy_radio20698_afe_div_ratio(phy_info_t *pi, uint8 use_ovr,
	chanspec_t chanspec_sc, uint8 sc_mode)
{
	/* 20698_procs.tcl r847973: 20698_afe_div_ratio */

	uint16 nad, nda, val;
	uint8 core;
	uint16 overrides;
	uint16 adc_div2 = 2;   /* FIXME43684: low-rate TSSI not implemented yet */
	uint16 dac_div2 = 1;
	chanspec_t chanspec = (sc_mode == 0) ? pi->radio_chanspec : chanspec_sc;

	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(chanspec),
	                           CHSPEC_IS6G(chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(chanspec) ? WF_CHAN_FACTOR_5_G
	                                                :  WF_CHAN_FACTOR_2_4_G);

	if (fc >= 6535) {
		if (CHSPEC_IS160((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 9;
			nad = 20;
		} else if (CHSPEC_IS80((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 9;
			nad = 20;
		} else if (CHSPEC_IS40((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 15;
			nad = 30;
		} else {
			nda = 30;
			nad = 60;
		}
		adc_div2 = 1;
		dac_div2 = 1;
	} else if (fc >= 5900) {
		if (CHSPEC_IS160((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			if (fc == 6025) {
				nda = 8;
				nad = 17;
			} else {
				nda = 8;
				nad = 18;
			}
		} else if (CHSPEC_IS80((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 9;
			nad = 18;
		} else if (CHSPEC_IS40((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 15;
			nad = 27;
		} else {
			nda = 30;
			nad = 54;
		}
		adc_div2 = 1;
		dac_div2 = 1;
	} else {
		if (CHSPEC_IS160((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 7;
			nad = 8;
		} else if (CHSPEC_IS80((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 8;
			nad = 8;
		} else if (CHSPEC_IS40((sc_mode == 0) ? pi->radio_chanspec : chanspec_sc)) {
			nda = 12;
			nad = 12;
		} else {
			nda = 24;
			nad = 24;
		}
		adc_div2 = 2;
		dac_div2 = (CHSPEC_IS160(chanspec) == 0) ? 2 : 1;
	}

	/* VELOCE AFE-DIV Settings */
	if (ISSIM_ENAB(pi->sh->sih)) {
		if (CHSPEC_IS2G(chanspec)) {
			if (CHSPEC_IS20(chanspec)) {
				nda = 4; nad = 8; adc_div2 = 0;
			} else if (CHSPEC_IS40(chanspec)) {
				nda = 2; nad = 4; adc_div2 = 1;
			} else {
				ASSERT(0);
			}
		} else {
			/* There is no radio model in veloce */
			nad *= adc_div2;
			adc_div2 = 1;
		}
	}

	if (use_ovr) {
		/* set 2nd rail of ADC while in BW160 */
		if ((sc_mode == 1) && CHSPEC_IS160(chanspec_sc)) {
			MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, 3, ovr_rxadc1_en, 1);
			MOD_RADIO_REG_20698(pi, RXADC_CFG5, 3, rxadc1_en, 1);
		} else {
			if (CHSPEC_IS160(pi->radio_chanspec))
				MOD_RADIO_REG_20698(pi, RXADC_CFG5, 3, rxadc1_en, 1);
			else
				MOD_RADIO_REG_20698(pi, RXADC_CFG5, 3, rxadc1_en, 0);
			MOD_RADIO_REG_20698(pi, AFE_CFG1_OVR2, 3, ovr_rxadc1_en, 0);
		}

		FOREACH_CORE(pi, core) {
			/* Set AFEDIV overrides:
				ovr_afediv_adc_div1 = 1 * 0x40
				ovr_afediv_adc_div2 = 1 * 0x20
				ovr_afediv_dac_div1 = 1 * 0x10
				ovr_afediv_dac_div2 = 1 * 0x08
			*/
			if ((sc_mode == 0) || (sc_mode == 1 && core == 3)) {
			    overrides = READ_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core);
			    overrides |= 0x0078;
			    WRITE_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core, overrides);
			    MOD_RADIO_REG_20698(pi, AFEDIV_REG1, core, afediv_dac_div2, dac_div2);
			    MOD_RADIO_REG_20698(pi, AFEDIV_REG1, core, afediv_dac_div1,
			        nda/dac_div2);
			    MOD_RADIO_REG_20698(pi, AFEDIV_REG2, core, afediv_adc_div2, adc_div2);
			    MOD_RADIO_REG_20698(pi, AFEDIV_REG2, core, afediv_adc_div1, nad);
			}
		}
		/* 2G BW80/160 AFEDIV needs to be 7 */
		if ((sc_mode == 1) && CHSPEC_IS2G(chanspec_sc) && (CHSPEC_IS80(chanspec_sc) ||
			CHSPEC_IS160(chanspec_sc))) {
			MOD_RADIO_REG_20698(pi, AFEDIV_REG2, 3, afediv_adc_div1, 0x7);
		}
	} else {
		FOREACH_CORE(pi, core) {
			/* take away the overrides (NOT of 0x0078) */
			val = READ_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core);
			WRITE_RADIO_REG_20698(pi, AFEDIV_CFG1_OVR, core, val & 0xff87);
		}
	}
}

void
wlc_phy_radio20704_afe_div_ratio(phy_info_t *pi, uint8 use_ovr)
{
	/* 20704_procs.tcl : 20704_afe_div_ratio */

	uint8 core;
	uint16 overrides;
	uint16 save_ReadOverrides;
	uint16 adc_div1, adc_div2;
	uint16 dac_div1, dac_div2;
	chanspec_t chanspec = pi->radio_chanspec;
	const uint16 afediv_overrides =
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_adc_div1_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_adc_div2_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_dac_div1_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_dac_div2_MASK(pi->pubpi->radiorev);

	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(chanspec),
	                           CHSPEC_IS6G(chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(chanspec) ? WF_CHAN_FACTOR_5_G
	                                                :  WF_CHAN_FACTOR_2_4_G);

	if (fc >= 6535) {
		dac_div2 = 1;
		adc_div2 = 1;
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			dac_div1 = 9;
			adc_div1 = 20;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 15;
			adc_div1 = 30;
		} else {
			dac_div1 = 30;
			adc_div1 = 60;
		}
	} else if (fc >= 5900) {
		dac_div2 = 1;
		adc_div2 = 1;
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			dac_div1 = 9;
			adc_div1 = 18;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 15;
			adc_div1 = 27;
		} else {
			dac_div1 = 24;
			adc_div1 = 54;
		}
	} else {
		dac_div2 = 2;
		adc_div2 = 2;
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			dac_div1 = 4;
			adc_div1 = 8;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 6;
			adc_div1 = 12;
		} else if (CHSPEC_IS2G(pi->radio_chanspec) && pi->sromi->dacdiv10_2g) {
			dac_div1 = 10;
			adc_div1 = 24;
		} else {
			dac_div1 = 12;
			adc_div1 = 24;
		}
	}

	/* VELOCE AFE-DIV Settings */
	if (ISSIM_ENAB(pi->sh->sih)) {
		dac_div2 = 1;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				/* adc_div2 needs to be 0 */
				dac_div1 = 4; adc_div1 = 8; adc_div2 = 0;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				dac_div1 = 2; adc_div1 = 4; adc_div2 = 1;
			} else {
				ASSERT(0);
			}
		} else {
			/* There is no radio model in veloce */
			adc_div1 *= adc_div2;
			adc_div2 = 1;
		}
	}

	FOREACH_CORE(pi, core) {
		if (use_ovr) {
			overrides = READ_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core);
			overrides |= afediv_overrides;
			save_ReadOverrides = READ_RADIO_REG_20704(pi, READOVERRIDES, core);
			// Read override values i.s.o direct control values, otherwise
			// a sequence of MOD_RADIO_REG will give incorrect result
			MOD_RADIO_REG_20704(pi, READOVERRIDES, core, ReadOverrides_reg, 0);
			MOD_RADIO_REG_20704(pi, AFEDIV_REG1, core, afediv_dac_div2, dac_div2);
			MOD_RADIO_REG_20704(pi, AFEDIV_REG1, core, afediv_dac_div1, dac_div1);
			MOD_RADIO_REG_20704(pi, AFEDIV_REG2, core, afediv_adc_div2, adc_div2);
			MOD_RADIO_REG_20704(pi, AFEDIV_REG2, core, afediv_adc_div1, adc_div1);
			WRITE_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core, overrides);
			WRITE_RADIO_REG_20704(pi, READOVERRIDES, core, save_ReadOverrides);
		} else {
			/* back to direct control of dividers */
			overrides = READ_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core);
			overrides &= ~afediv_overrides;
			WRITE_RADIO_REG_20704(pi, AFEDIV_CFG1_OVR, core, overrides);
		}
	}
}

void
wlc_phy_radio20707_afe_div_ratio(phy_info_t *pi, uint8 use_ovr)
{
	uint16 nad;
	uint16 nda;
	uint8 core;
	uint16 overrides;
	uint16 adc_div2 = 1;
	uint16 dac_div2 = 2;
	uint16 save_ReadOverrides;
	const uint16 afediv_overrides =
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_adc_div1_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_adc_div2_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_dac_div1_MASK(pi->pubpi->radiorev) |
		RF_20704_AFEDIV_CFG1_OVR_ovr_afediv_dac_div2_MASK(pi->pubpi->radiorev);

	chanspec_t chanspec = pi->radio_chanspec;

	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(chanspec),
	                           CHSPEC_IS6G(chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(chanspec) ? WF_CHAN_FACTOR_5_G
	                                                 : WF_CHAN_FACTOR_2_4_G);

	if (fc < 5900) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			nda = 8;
			nad = 16;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			nda = 12;
			nad = 24;
		} else {
			nda = 24;
			nad = 48;
		}
	} else if (fc < 6535) {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			nda = 8;
			nad = 18;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			nda = 15;
			nad = 27;
		} else {
			nda = 30;
			nad = 54;
		}
	} else {
		if (CHSPEC_IS80(pi->radio_chanspec)) {
			nda = 9;
			nad = 20;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			nda = 15;
			nad = 30;
	        } else {
			nda = 30;
			nad = 60;
	        }
	}

	/* VELOCE AFE-DIV Settings */
	if (ISSIM_ENAB(pi->sh->sih)) {
		dac_div2 = 1;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				nda = 4; nad = 8; adc_div2 = 0;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				nda = 2; nad = 4; adc_div2 = 1;
			} else {
				ASSERT(0);
			}
		} else {
			/* There is no radio model in veloce */
			nad *= adc_div2;
			adc_div2 = 1;
		}
	}

	FOREACH_CORE(pi, core) {
		if (use_ovr) {
			overrides = READ_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core);
			overrides |= afediv_overrides;
			save_ReadOverrides = READ_RADIO_REG_20707(pi, READOVERRIDES, core);
			// Read override values i.s.o direct control values, otherwise
			// a sequence of MOD_RADIO_REG will give incorrect result
			MOD_RADIO_REG_20707(pi, READOVERRIDES, core, ReadOverrides_reg, 0);
			MOD_RADIO_REG_20707(pi, AFEDIV_REG1, core, afediv_dac_div2, dac_div2);
			MOD_RADIO_REG_20707(pi, AFEDIV_REG1, core, afediv_dac_div1, nda/dac_div2);
			MOD_RADIO_REG_20707(pi, AFEDIV_REG2, core, afediv_adc_div2, adc_div2);
			MOD_RADIO_REG_20707(pi, AFEDIV_REG2, core, afediv_adc_div1, nad);
			WRITE_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core, overrides);
			WRITE_RADIO_REG_20707(pi, READOVERRIDES, core, save_ReadOverrides);
		} else {
			/* back to direct control of dividers */
			overrides = READ_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core);
			overrides &= ~afediv_overrides;
			WRITE_RADIO_REG_20707(pi, AFEDIV_CFG1_OVR, core, overrides);
		}
	}
}

void
wlc_phy_radio20708_afe_div_ratio(phy_info_t *pi, uint8 use_ovr,
	chanspec_t chanspec_sc, uint8 sc_mode, bool adc_restore, uint8 dac_rate_mode, bool wbcal)
{
	uint16 nad = 0;
	uint16 nda = 0;
	uint16 core;
	uint16 adc_div2 = 0;
	uint8  adc_rate_mode;
	bool low_rate_tssi;
	phy_ac_radio_info_t *ri = pi->u.pi_acphy->radioi;

	chanspec_t chanspec = (sc_mode == 0) ? pi->radio_chanspec : chanspec_sc;

	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(chanspec),
	                           CHSPEC_IS6G(chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(chanspec) ? WF_CHAN_FACTOR_5_G
	                                                 : WF_CHAN_FACTOR_2_4_G);
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	adc_rate_mode = READ_PHYREG(pi, AdcboostCtrl) & 0x3;
	low_rate_tssi = pi->u.pi_acphy->sromi->srom_low_adc_rate_en;

	if (!wbcal) {
		/* 6715 */
		if (fc < 5855) {
			if (CHSPEC_IS160(chanspec)) {
				nda = 5;
				nad = (fc == 5815)? 17: 16;
			} else if (CHSPEC_IS80(chanspec)) {
				nda = 8;
				nad = (fc < 3000) ? 14: 16;
			} else if (CHSPEC_IS40(chanspec)) {
				nda = 12;
				nad = 24;
			} else {
				nda = 24;
				nad = (fc < 3000) &&
					(!PHY_COREMASK_SISO(stf_shdata->phyrxchain)) ? 54 : 48;
			}
		} else if (fc < 6535) {
			if (CHSPEC_IS160(chanspec)) {
				if (fc < 6250) {
					nda = 5;
				} else {
					nda = 6;
				}
				nad = (fc == 6025)? 17: 18;
			} else if (CHSPEC_IS80(chanspec)) {
				nda = 9;
				nad = 18;
			} else if (CHSPEC_IS40(chanspec)) {
				nda = 15;
				nad = 27;
			} else {
				nda = 30;
				nad = (!PHY_COREMASK_SISO(stf_shdata->phyrxchain)) ? 54 : 46;
			}
		} else {
			if (CHSPEC_IS160(chanspec)) {
				nda = 6;
				nad = (fc >= 6985) ? 21: 20;
			} else if (CHSPEC_IS80(chanspec)) {
				nda = 9;
				nad = 20;
			} else if (CHSPEC_IS40(chanspec)) {
				nda = 15;
				nad = 30;
			} else {
				nda = 30;
				nad = (!PHY_COREMASK_SISO(stf_shdata->phyrxchain)) ? 60 :
					(((fc >= 6695) && (fc <= 6775)) ? 55 : 54);
			}
		}

		if ((adc_rate_mode == 2) && (CHSPEC_IS20(chanspec) ||
			CHSPEC_IS40(chanspec))) {
			if (fc < 5180) {
				nad = 14;
			} else if (fc < 5855) {
				nad = 16;
			} else if (fc < 6535) {
				nad = 18;
			} else {
				nad = 20;
			}
		} else if ((adc_rate_mode == 3) && CHSPEC_IS20(chanspec)) {
			if (fc < 5180) {
				nad = 24;
			} else if (fc < 5855) {
				nad = 24;
			} else if (fc < 6535) {
				nad = 27;
			} else {
				nad = 30;
			}
		}

		/* if low_rate_tssi enable, increasing ADC div to reduce ADC clk */
		if (low_rate_tssi) {
			if (CHSPEC_IS160(chanspec)) {
				adc_div2 = 63;
			} else if (CHSPEC_IS80(chanspec)) {
				adc_div2 = 31;
			} else if (CHSPEC_IS40(chanspec)) {
				adc_div2 = 31;
			} else {
				adc_div2 = 15;
			}
		} else {
			adc_div2 = 1;
		}

		if (dac_rate_mode == 2) {
		  nda = 8;
		  nad = 16;
		}

		/* VELOCE AFE-DIV Settings */
		if (ISSIM_ENAB(pi->sh->sih)) {

			if (CHSPEC_IS2G(chanspec)) {
				if (CHSPEC_IS20(chanspec)) {
					nda = 4; nad = 8; adc_div2 = 0;
				} else if (CHSPEC_IS40(chanspec)) {
					nda = 2; nad = 4; adc_div2 = 1;
				} else {
					ASSERT(0);
				}
			} else {
				/* There is no radio model in veloce */
				nad *= adc_div2;
				adc_div2 = 1;
			}
		}
	} else {
		if (ri->wbcal_afediv_ratio != -1) {
			/* bit 0-5: nad, bit 8-13: nda */
			nda = (ri->wbcal_afediv_ratio >> 8) & 0x3F;
			nad = ri->wbcal_afediv_ratio & 0x3F;
		} else {
			/* special NAD/NDA setting for wbcal */
			if (dac_rate_mode == 1) { // Used by 80M and 160M
				if (CHSPEC_IS160(chanspec)) {
					if (fc < 6665) { // 5G, 6G low/mid
						nda = 5;
						nad = 20;
					} else { // 6G high
						nda = 6;
						nad = 24;
					}
				} else if (CHSPEC_IS80(chanspec)) {
					if (fc <= 5320) { // 5G low
						nda = 9;
						nad = 18;
					} else if (fc < 6115) { // 5G mid/high, 6G low
						nda = 10;
						nad = 20;
					} else if (fc < 6665) { // 6G mid
						nda = 11;
						nad = 22;
					} else { // 6G high
						nda = 12;
						nad = 24;
					}
				} else {
					PHY_ERROR(("%s: Unsupported BW for dac_rate_mode"
						" = 1 \n", __FUNCTION__));
				}
			} else if (dac_rate_mode == 2) { // Used by 20M and 40M
				if (fc <= 3000) { // 2G
					nda = 8;
					nad = 16;
				} else if (fc <= 5320) { // 5G low
					nda = 9;
					nad = 18;
				} else if (fc < 6115) { // 5G mid/high, 6G low
					nda = 10;
					nad = 20;
				} else if (fc < 6665) { // 6G mid
					nda = 11;
					nad = 22;
				} else { // 6G high
					nda = 12;
					nad = 24;
				}
			} else {
				/* Unsupported dac_rate_mode for wbcal */
				ASSERT(0);
			}
		}
		PHY_PAPD(("wl%d %s WBPAPD CAL: nda = %d, nad = %d \n", pi->sh->unit, __FUNCTION__,
			nda, nad));
	}

	if (use_ovr) {
		for (core = 0; core < 2; core++) {
			if ((sc_mode == 0) || (sc_mode == 1 &&
				core == (pi->u.pi_acphy->radioi->maincore_on_pll1 ? 0 : 1))) {
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core,
					ovr_afediv_dac_div1, 1);
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core,
					ovr_afediv_adc_div2, 1);
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core,
					ovr_afediv_adc_div1, 1);
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG5, core, afediv_dac_div1, nda);
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG4, core, afediv_adc_div2,
					adc_div2);
				MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG4, core, afediv_adc_div1, nad);
			}
		}

		if ((sc_mode == 1) && CHSPEC_IS160(chanspec)) {
			/* take ADC1 out of reset mode and enable it */
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_I, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG0, 3, rxadc_reset_n_adc1_I, 1);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_Q, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG0, 3, rxadc_reset_n_adc1_Q, 1);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc1_enb, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG5, 3, rxadc1_enb, 0);
		} else if ((sc_mode == 0) && !CHSPEC_IS160(chanspec) && (adc_restore == 1)) {
			/* restore ADC1 settings */
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_I, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG0, 3, rxadc_reset_n_adc1_I, 0);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_Q, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG0, 3, rxadc_reset_n_adc1_Q, 0);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc1_enb, 1);
			MOD_RADIO_REG_20708(pi, RXADC_CFG5, 3, rxadc1_enb, 1);
			/* recover override */
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_I, 0);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc_reset_n_adc1_Q, 0);
			MOD_RADIO_REG_20708(pi, AFE_CFG1_OVR2, 3, ovr_rxadc1_enb, 0);
		}
	} else {
		/* remove override mode */
		for (core = 0; core < 2; core++) {
			MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core, ovr_afediv_dac_div1, 0);
			MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core, ovr_afediv_adc_div2, 0);
			MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, core, ovr_afediv_adc_div1, 0);
		}
	}
}

void
wlc_phy_radio20709_afe_div_ratio(phy_info_t *pi, uint8 use_ovr)
{
	/* 20709_procs.tcl r798817: 20709_afe_div_ratio */

	uint16 nad;
	uint16 nda;
	uint8 core;
	uint16 overrides;
	uint16 adc_div2 = 2;
	uint16 dac_div2 = 2;

	if (CHSPEC_IS80(pi->radio_chanspec)) {
		nda = 8;
		nad = 8;
	} else if (CHSPEC_IS40(pi->radio_chanspec)) {
		nda = 12;
		nad = 12;
	} else {
		nda = 24;
		nad = 24;
	}

	/* VELOCE AFE-DIV Settings */
	if (ISSIM_ENAB(pi->sh->sih)) {
		dac_div2 = 1;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				nda = 4; nad = 8; adc_div2 = 0;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				nda = 2; nad = 4; adc_div2 = 1;
			} else {
				ASSERT(0);
			}
		} else {
			/* There is no radio model in veloce */
			nad *= adc_div2;
			adc_div2 = 1;
		}
	}

	if (use_ovr) {
		FOREACH_CORE(pi, core) {
			/* Set AFEDIV overrides:
				ovr_afediv_adc_div1 = 1 * 0x40
				ovr_afediv_adc_div2 = 1 * 0x20
				ovr_afediv_dac_div1 = 1 * 0x10
				ovr_afediv_dac_div2 = 1 * 0x08
			*/
			overrides = READ_RADIO_REG_20709(pi, AFEDIV_CFG1_OVR, core);
			overrides |= 0x0078;
			WRITE_RADIO_REG_20709(pi, AFEDIV_CFG1_OVR, core, overrides);
			MOD_RADIO_REG_20709(pi, AFEDIV_REG1, core, afediv_dac_div2, dac_div2);
			MOD_RADIO_REG_20709(pi, AFEDIV_REG1, core, afediv_dac_div1, nda/dac_div2);
			MOD_RADIO_REG_20709(pi, AFEDIV_REG2, core, afediv_adc_div2, adc_div2);
			MOD_RADIO_REG_20709(pi, AFEDIV_REG2, core, afediv_adc_div1, nad);
		}
	} else {
		/* remove override mode */
		FOREACH_CORE(pi, core) {
			WRITE_RADIO_REG_20709(pi, AFEDIV_CFG1_OVR, core, 0);
		}
	}
}

void
wlc_phy_radio20710_afe_div_ratio(phy_info_t *pi, uint8 use_ovr, uint8 dac_rate_mode, bool wbcal)
{
	/* 20710_procs.tcl : 20710_afe_div_ratio */

	uint8 core;
	uint16 overrides;
	uint16 save_ReadOverrides;
	uint16 adc_div1, adc_div2;
	uint16 dac_div1, dac_div2;
	chanspec_t chanspec = pi->radio_chanspec;
	const uint16 afediv_overrides =
		RF_20710_AFEDIV_CFG1_OVR_ovr_afediv_adc_div1_MASK(pi->pubpi->radiorev) |
		RF_20710_AFEDIV_CFG1_OVR_ovr_afediv_adc_div2_MASK(pi->pubpi->radiorev) |
		RF_20710_AFEDIV_CFG1_OVR_ovr_afediv_dac_div1_MASK(pi->pubpi->radiorev) |
		RF_20710_AFEDIV_CFG1_OVR_ovr_afediv_dac_div2_MASK(pi->pubpi->radiorev);

	uint32 fc = wf_channel2mhz(CHSPEC_CHANNEL(chanspec),
	                           CHSPEC_IS6G(chanspec) ? WF_CHAN_FACTOR_6_G
	                         : CHSPEC_IS5G(chanspec) ? WF_CHAN_FACTOR_5_G
	                                                :  WF_CHAN_FACTOR_2_4_G);

	if (wbcal && (dac_rate_mode == 1) && CHSPEC_IS160(pi->radio_chanspec)) {
		dac_div1 = 4;
		dac_div2 = 2;
		adc_div1 = 16;
		adc_div2 = 2;
	} else if (dac_rate_mode == 2) {
		if (fc >= 5905) {
			dac_div1 = 6;
			dac_div2 = 2;
			adc_div1 = 12;
			adc_div2 = 2;
		} else if (fc >= 3000) {
			dac_div1 = 9;
			dac_div2 = 1;
			adc_div1 = 9;
			adc_div2 = 2;
		} else {
			dac_div1 = 4;
			dac_div2 = 2;
			adc_div1 = 8;
			adc_div2 = 2;
		}
	} else if ((fc >= 6535) && !wbcal) {
		dac_div2 = 1;
		adc_div2 = 1;
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			dac_div1 = 9;
			adc_div1 = 20;
		} else if (CHSPEC_IS80(pi->radio_chanspec)) {
			dac_div1 = 9;
			adc_div1 = 20;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 15;
			adc_div1 = 30;
		} else {
			dac_div1 = 30;
			adc_div1 = 60;
		}
	} else if ((fc >= 5905) && !wbcal) {
		dac_div2 = 1;
		adc_div2 = 1;
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			dac_div1 = 8;
			adc_div1 = (fc == 6025) ? 17 : 18;
		} else if (CHSPEC_IS80(pi->radio_chanspec)) {
			dac_div1 = 9;
			adc_div1 = 18;
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 15;
			adc_div1 = 27;
		} else {
			dac_div1 = 24;
			adc_div1 = 54;
		}
	} else {
		dac_div2 = 2;
		adc_div2 = 2;
		if (CHSPEC_IS160(pi->radio_chanspec)) {
			dac_div2 = 1;
			dac_div1 = 7;
			if (fc == 5815) {
				adc_div2 = 1;
				adc_div1 = 17;
			} else {
				adc_div1 = 8;
			}
		} else if (CHSPEC_IS80(pi->radio_chanspec)) {
			if (wbcal) {
				if (fc >= 5905) {
					dac_div1 = 6;
					adc_div1 = 12;
				} else if (fc >= 5500) {
					dac_div1 = 5;
					adc_div1 = 10;
				} else {
					dac_div1 = 9;
					dac_div2 = 1;
					adc_div1 = 9;
				}
			} else {
				dac_div1 = 4;
				adc_div1 = 8;
			}
		} else if (CHSPEC_IS40(pi->radio_chanspec)) {
			dac_div1 = 6;
			adc_div1 = 12;
		} else if (CHSPEC_IS2G(pi->radio_chanspec) && pi->sromi->dacdiv10_2g) {
			dac_div1 = 10;
			adc_div1 = 24;
		} else {
			dac_div1 = 12;
			adc_div1 = 24;
		}
	}

	/* VELOCE AFE-DIV Settings */
	if (ISSIM_ENAB(pi->sh->sih)) {
		dac_div2 = 1;
		if (CHSPEC_IS2G(pi->radio_chanspec)) {
			if (CHSPEC_IS20(pi->radio_chanspec)) {
				/* adc_div2 needs to be 0 */
				dac_div1 = 4; adc_div1 = 8; adc_div2 = 0;
			} else if (CHSPEC_IS40(pi->radio_chanspec)) {
				dac_div1 = 2; adc_div1 = 4; adc_div2 = 1;
			} else {
				ASSERT(0);
			}
		} else {
			/* There is no radio model in veloce */
			adc_div1 *= adc_div2;
			adc_div2 = 1;
		}
	}

	FOREACH_CORE(pi, core) {
		if (use_ovr) {
			overrides = READ_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core);
			overrides |= afediv_overrides;
			save_ReadOverrides = READ_RADIO_REG_20710(pi, READOVERRIDES, core);
			// Read override values i.s.o direct control values, otherwise
			// a sequence of MOD_RADIO_REG will give incorrect result
			MOD_RADIO_REG_20710(pi, READOVERRIDES, core, ReadOverrides_reg, 0);
			MOD_RADIO_REG_20710(pi, AFEDIV_REG1, core, afediv_dac_div2, dac_div2);
			MOD_RADIO_REG_20710(pi, AFEDIV_REG1, core, afediv_dac_div1, dac_div1);
			MOD_RADIO_REG_20710(pi, AFEDIV_REG2, core, afediv_adc_div2, adc_div2);
			MOD_RADIO_REG_20710(pi, AFEDIV_REG2, core, afediv_adc_div1, adc_div1);
			WRITE_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core, overrides);
			WRITE_RADIO_REG_20710(pi, READOVERRIDES, core, save_ReadOverrides);
		} else {
			/* back to direct control of dividers */
			overrides = READ_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core);
			overrides &= ~afediv_overrides;
			WRITE_RADIO_REG_20710(pi, AFEDIV_CFG1_OVR, core, overrides);
		}
	}
}

static void
wlc_phy_radio2069_afecal_invert(phy_info_t *pi)
{
	uint8 core;
	uint16 calcode;
	uint8 phyrxchain;

	BCM_REFERENCE(phyrxchain);

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Switch on the clk */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);

	/* Output calCode = 1:14, latched = 15:28 */

	phyrxchain = phy_stf_get_data(pi->stfi)->phyrxchain;
	FOREACH_ACTV_CORE(pi, phyrxchain, core) {
		/* Use calCodes 1:14 instead of 15:28 */
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_iqadc_flash_calcode_Ich, 1);
		MOD_RADIO_REGC(pi, OVR3, core, ovr_afe_iqadc_flash_calcode_Qch, 1);

		/* Invert the CalCodes */
		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE28, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE14(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE27, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE13(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE26, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE12(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE25, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE11(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE24, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE10(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE23, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE9(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE22, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE8(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE21, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE7(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE20, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE6(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE19, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE5(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE18, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE4(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE17, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE3(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE16, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE2(core), ~calcode & 0xffff);

		calcode = READ_RADIO_REGC(pi, RF, ADC_CALCODE15, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CALCODE1(core), ~calcode & 0xffff);
	}

	/* Turn off the clk */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
}

#define MAX_2069_AFECAL_WAITLOOPS 10
#define SAVE_RESTORE_AFE_CFG_REGS 3
/* 3 registers per AFE core: RfctrlCoreAfeCfg1, RfctrlCoreAfeCfg2, RfctrlOverrideAfeCfg */
static void
wlc_phy_radio2069_afecal(phy_info_t *pi)
{
	uint8 core, done_i, done_q;
	uint16 adc_cfg4, afe_cfg_arr[SAVE_RESTORE_AFE_CFG_REGS*PHY_CORE_MAX] = {0};
	phy_stf_data_t *stf_shdata = phy_stf_get_data(pi->stfi);

	BCM_REFERENCE(stf_shdata);

	if (ISSIM_ENAB(pi->sh->sih))
		return;

	ASSERT(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID));

	/* Used to latch (clk register) rcal, rccal, ADC cal code */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 1);

	/* Save config registers and issue reset */
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		/* Cfg1  in 0 to PHYCORENUM-1 */
		afe_cfg_arr[core] = READ_PHYREGCE(pi, RfctrlCoreAfeCfg1, core);
		/* Cfg2 in PHYCORENUM to 2*PHYCORENUM -1 */
		afe_cfg_arr[core + PHYCORENUM(pi->pubpi->phy_corenum)] =
		  READ_PHYREGCE(pi, RfctrlCoreAfeCfg2, core);
		/* Overrides in 2*PHYCORENUM to 3*PHYCORENUM - 1 */
		afe_cfg_arr[core + 2*PHYCORENUM(pi->pubpi->phy_corenum)] =
		  READ_PHYREGCE(pi, RfctrlOverrideAfeCfg, core);
		MOD_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, afe_iqadc_reset, 1);
		MOD_PHYREGCE(pi, RfctrlOverrideAfeCfg, core, afe_iqadc_reset, 1);
		MOD_RADIO_REGC(pi, ADC_CFG3, core, flash_calrstb, 0); /* reset */
	}

	OSL_DELAY(100);

	/* Bring each AFE core back from reset and perform cal */
	FOREACH_ACTV_CORE(pi, stf_shdata->phyrxchain, core) {
		MOD_RADIO_REGC(pi, ADC_CFG3, core, flash_calrstb, 1);
		adc_cfg4 = READ_RADIO_REGC(pi, RF, ADC_CFG4, core);
		phy_utils_write_radioreg(pi, RF_2069_ADC_CFG4(core), adc_cfg4 | 0xf);

		SPINWAIT((READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core),
			ADC_STATUS, i_wrf_jtag_afe_iqadc_Ich_cal_state) &&
			READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
				i_wrf_jtag_afe_iqadc_Qch_cal_state)) == 0,
				ACPHY_SPINWAIT_AFE_CAL_STATUS);
		done_i = READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
			i_wrf_jtag_afe_iqadc_Ich_cal_state);
		done_q = READ_RADIO_REGFLDC(pi, RF_2069_ADC_STATUS(core), ADC_STATUS,
			i_wrf_jtag_afe_iqadc_Qch_cal_state);
		/* Don't assert for QT */
		if (!ISSIM_ENAB(pi->sh->sih)) {
			if (done_i == 0 || done_q == 0) {
				PHY_FATAL_ERROR_MESG((" %s: SPINWAIT ERROR:", __FUNCTION__));
				PHY_FATAL_ERROR_MESG(("AFECAL FAILED on Core %d\n", core));
				PHY_FATAL_ERROR(pi, PHY_RC_AFE_CAL_FAILED);
			}
		}

		/* calMode = 0 */
		phy_utils_write_radioreg(pi, RF_2069_ADC_CFG4(core), (adc_cfg4 & 0xfff0));
		/* Restore AFE config registers for that core with saved values */
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg1, core, afe_cfg_arr[core]);
		WRITE_PHYREGCE(pi, RfctrlCoreAfeCfg2, core,
			afe_cfg_arr[core + PHYCORENUM(pi->pubpi->phy_corenum)]);
		WRITE_PHYREGCE(pi, RfctrlOverrideAfeCfg, core,
			afe_cfg_arr[core + 2 * PHYCORENUM(pi->pubpi->phy_corenum)]);
	}

	/* Turn off clock */
	MOD_RADIO_REG(pi, RFP, PLL_XTAL2, xtal_pu_RCCAL, 0);
	if (RADIO2069REV(pi->pubpi->radiorev) < 4) {
	  /* JIRA (CRDOT11ACPHY-153) calCodes are inverted for 4360a0 */
	  wlc_phy_radio2069_afecal_invert(pi);
	}
}

void
wlc_phy_radio_afecal(phy_info_t *pi)
{
	wlc_phy_resetcca_acphy(pi);
	OSL_DELAY(1);

	if (RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) {
		wlc_phy_radio20693_afecal(pi);
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20698_afecal(pi);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20704_afecal(pi);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20707_afecal(pi);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20708_afecal(pi);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20709_afecal(pi);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
		if (!ISSIM_ENAB(pi->sh->sih)) {
			wlc_phy_radio20710_afecal(pi);
		}
	} else {
		wlc_phy_radio2069_afecal(pi);
	}
}

void
wlc_phy_radio_vco_opt(phy_info_t *pi, uint8 vco_mode)
{
}

#ifdef RADIO_HEALTH_CHECK
static bool
phy_ac_radio_check_pll_lock(phy_type_radio_ctx_t *ctx)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	bool ret = TRUE;
	int8 need_refresh = 0;

	wlapi_suspend_mac_and_wait(pi->sh->physhim);
	if ((RADIOID_IS(pi->pubpi->radioid, BCM20693_ID)) ||
		(RADIOID_IS(pi->pubpi->radioid, BCM2069_ID)))
		need_refresh = READ_RADIO_REGFLD(pi, RFP, PLL_DSPR27, rfpll_monitor_need_refresh);

	wlapi_enable_mac(pi->sh->physhim);
	if (need_refresh == 1)
		ret = FALSE;
	return (ret);
}
#endif /* RADIO_HEALTH_CHECK */

#if defined(WLTEST)
int
phy_ac_radio_set_pd(phy_ac_radio_info_t *radioi, uint16 int_val)
{
	PHY_ERROR(("RADIO PD is not supported for this chip \n"));
	return BCME_UNSUPPORTED;
}
#endif /* WLTEST */

int
phy_ac_get_wbcal_afediv_ratio(phy_ac_radio_info_t *radioi, int32 *ret_val)
{
	*ret_val = (int32)radioi->wbcal_afediv_ratio;
	return BCME_OK;
}

int
phy_ac_set_wbcal_afediv_ratio(phy_ac_radio_info_t *radioi, int32 set_val)
{
	if (set_val < -1 || set_val > 65535) {
		return BCME_ERROR;
	} else {
		radioi->wbcal_afediv_ratio = set_val;
		return BCME_OK;
	}
}

void
wlc_phy_radio20698_set_tx_notch(phy_info_t *pi)
{
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */
	if (RADIOMAJORREV(pi) < 2) {
		/* Set Tx notch with override registers as A0/A1 do not have direct control.
		 * Note: the override bits do not exist, the override registers are always active.
		 */
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20698(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
			MOD_RADIO_REG_20698(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
		}
	} else {
		/* FIXME43684 Set Tx notch by override; direct control not being used yet */
		FOREACH_CORE(pi, core) {
			MOD_RADIO_REG_20698(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
			MOD_RADIO_REG_20698(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
			MOD_RADIO_REG_20698(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
			MOD_RADIO_REG_20698(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
		}
	}
}

void
wlc_phy_radio20704_set_tx_notch(phy_info_t *pi)
{
	/* 20704_procs.tcl r753992: 20704_set_tx_notch */
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */

	/* Set Tx notch by override; direct control not being used yet */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20704(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
		MOD_RADIO_REG_20704(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
		MOD_RADIO_REG_20704(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
		MOD_RADIO_REG_20704(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
	}
}

void
wlc_phy_radio20707_set_tx_notch(phy_info_t *pi)
{
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */

	/* Set Tx notch by override; direct control not being used yet */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20707(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
		MOD_RADIO_REG_20707(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
		MOD_RADIO_REG_20707(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
		MOD_RADIO_REG_20707(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
	}
}

void
wlc_phy_radio20708_set_tx_notch(phy_info_t *pi)
{
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	//uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */

	/* Set Tx notch by override; direct control not being used yet */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
		MOD_RADIO_REG_20708(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
		/* Gaintable has entries for RC Notch gain control.
		 * Leave the control to the gaintable now.
		 * http://confluence.broadcom.com/pages/viewpage.action?pageId=574801910
		 */
		//MOD_RADIO_REG_20708(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
		//MOD_RADIO_REG_20708(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);

		if (CHSPEC_IS80(pi->radio_chanspec) || CHSPEC_IS160(pi->radio_chanspec)) {
			MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias, 1);
			MOD_RADIO_REG_20708(pi, LPF_REG11, core, lpf_opamp3_bias, 45);
			MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias, 1);
			MOD_RADIO_REG_20708(pi, LPF_REG6, core, lpf_opamp4_bias, 45);
		} else {
			MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_opamp3_bias, 1);
			MOD_RADIO_REG_20708(pi, LPF_REG11, core, lpf_opamp3_bias, 20);
			MOD_RADIO_REG_20708(pi, LPF_OVR1, core, ovr_lpf_opamp4_bias, 1);
			MOD_RADIO_REG_20708(pi, LPF_REG6, core, lpf_opamp4_bias, 20);
		}
	}
}

void
wlc_phy_radio20709_set_tx_notch(phy_info_t *pi)
{
	/* 20709_procs.tcl r798817: 20709_set_tx_notch */
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */

	/* Set Tx notch by override; direct control not being used yet */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20709(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
		MOD_RADIO_REG_20709(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
		MOD_RADIO_REG_20709(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
		MOD_RADIO_REG_20709(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
	}
}

void
wlc_phy_radio20710_set_tx_notch(phy_info_t *pi)
{
	/* 20710_procs.tcl r753992: 20710_set_tx_notch */
	uint8 core;
	uint8 notch_bw =
			CHSPEC_IS20(pi->radio_chanspec)? 0 :
			CHSPEC_IS40(pi->radio_chanspec)? 1 :
			CHSPEC_IS80(pi->radio_chanspec)? 2 : 3;
	uint8 notch_gain = 4; /* 0=-12dB, 1=-9dB, 2=-6dB, 3=-3dB, 4=0dB */

	/* Set Tx notch by override; direct control not being used yet */
	FOREACH_CORE(pi, core) {
		MOD_RADIO_REG_20710(pi, LPF_OVR1, core, ovr_lpf_rc_bw, 1);
		MOD_RADIO_REG_20710(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_bw, notch_bw);
		MOD_RADIO_REG_20710(pi, LPF_OVR2, core, ovr_lpf_rc_gain, 1);
		MOD_RADIO_REG_20710(pi, LPF_NOTCH_CONTROL2, core, lpf_rc_gain, notch_gain);
	}
}

void
wlc_phy_radio20708_powerup_RFPll(phy_info_t *pi, uint core, bool pwrup)
{
	/* 20708_procs.tcl 20708_powerup_RFPll */
	/* turn on rfpll1 in 3+1 mode, disable it in normal mode */
	uint rfpll_val = pwrup ? 0x1 : 0x0;
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_vcobuf1_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_VCO2, core, rfpll_vcobuf1_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CP1, core, rfpll_cp_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_monitor_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG1, core, rfpll_monitor_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG1, core, rfpll_synth_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG1, core, rfpll_vco_buf_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG1, core, rfpll_vco_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_pfd_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG2, core, rfpll_pfd_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_mmd_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_CFG1, core, rfpll_mmd_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_OVR1, core, ovr_rfpll_frct_pu, 0x1);
	MOD_RADIO_PLLREG_20708(pi, PLL_FRCT1, core, rfpll_frct_pu, rfpll_val);

	MOD_RADIO_PLLREG_20708(pi, PLL_VCO7, core, rfpll_vco_core1_en, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_VCO2, core, rfpll_vcoafebuf_pu, rfpll_val);
	MOD_RADIO_PLLREG_20708(pi, PLL_LF6, core, rfpll_lf_cm_pu, rfpll_val);
}

void
wlc_phy_radio20698_powerup_RFP1(phy_info_t *pi, bool pwrup)
{
	/* 20698_procs.tcl 20698_powerup_RFP */
	/* turn on rfpll1 in 3+1 mode, disable it in normal mode */
	uint core = 1;

	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_rfpll_cp_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_rfpll_vco_buf_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_rfpll_synth_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_rfpll_vco_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_ldo_1p8_pu_ldo_CP, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_ldo_1p8_pu_ldo_VCO, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_rfpll_monitor_pu, 0x1);
	MOD_RADIO_PLLREG_20698(pi, PLL_OVR1, core, ovr_RefDoubler_pu_pfddrv, 0x1);

	if (pwrup) {
		/* ----- Powering up RFPLL1 ----- */
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_vco_pu, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_synth_pu, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_monitor_pu, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_CP1, core, rfpll_cp_pu, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, core, ldo_1p8_pu_ldo_CP, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, core, ldo_1p8_pu_ldo_VCO, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_LF6, core, rfpll_lf_cm_pu, 0x1);
	} else {
		/* ----- Powering down RFPLL1 ----- */
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_vco_pu, 0x0);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_synth_pu, 0x0);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_monitor_pu, 0x0);
		MOD_RADIO_PLLREG_20698(pi, PLL_CFG1, core, rfpll_vco_buf_pu, 0x0);
		MOD_RADIO_PLLREG_20698(pi, PLL_CP1, core, rfpll_cp_pu, 0x0);
		/* keep ldo ON (doesn't hurt anything) */
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, core, ldo_1p8_pu_ldo_CP, 0x1);
		/* keep ldo ON for core3 LOGen to work in 4x4 mode */
		MOD_RADIO_PLLREG_20698(pi, PLL_HVLDO1, core, ldo_1p8_pu_ldo_VCO, 0x1);
		MOD_RADIO_PLLREG_20698(pi, PLL_LF6, core, rfpll_lf_cm_pu, 0x0);
	}
}

void
wlc_phy_radio20708_pu_rx_core(phy_info_t *pi, uint core, uint ch, bool restore)
{
	/* 20708_procs.tcl 20708_pu_rx_core, core here means rxcore */
	if (restore == 0) {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_lna_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core, rxdb_lna_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_gm_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG4, core, rxdb_gm_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_mix_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core, rxdb_mix_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_bias_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG7, core, tia_bias_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_REG7, core, tia_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx_ldo_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX2G_REG4, core, rx_ldo_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bias_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG6, core, lpf_bias_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bq_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_REG6, core, lpf_bq_pu, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_lna_gc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core, rxdb_lna_gc, 0x7)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_gm_gc, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core, rxdb_gm_gc,	0x3)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core, ovr_rxdb_lna_rout, 0x1)
			MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core, rxdb_lna_rout, 0x0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		if (ch < 15) {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna2g_bias_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx2g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX2G_REG4, core,
					rx2g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna5g_bias_en,	0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx5g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core,
					rx5g_lna_tr_rx_en, 0x0)
				RADIO_REG_LIST_EXECUTE(pi, core);
		} else {
			RADIO_REG_LIST_START
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG5, core,
					rxdb_sel2g5g_loadind, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna2g_bias_en,	0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna2g_bias_en,	0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx2g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX2G_REG4, core,
					rx2g_lna_tr_rx_en, 0x0)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG6, core,
					rxdb_lna5g_bias_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx5g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20708_ENTRY(pi, RX5G_REG1, core,
					rx5g_lna_tr_rx_en, 0x1)
				RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20708_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_rx_ldo_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna2g_bias_en, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna5g_bias_en, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx2g_lna_tr_rx_en, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rx5g_lna_tr_rx_en, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_gm_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_mix_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core,
					ovr_tia_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, TIA_CFG1_OVR, core,
					ovr_tia_bias_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
					ovr_lpf_bias_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, LPF_OVR1, core,
					ovr_lpf_bq_pu, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna_gc, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_gm_gc, 0x0)
			MOD_RADIO_REG_20708_ENTRY(pi, RXDB_CFG1_OVR, core,
					ovr_rxdb_lna_rout, 0x0)
			RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

void
wlc_phy_radio20698_pu_rx_core(phy_info_t *pi, uint core, uint ch, bool restore)
{
	/* 20698_procs.tcl 20698_pu_rx_core, core here means rxcore */
	if (!restore) {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20698_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bq_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bias_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LPF_REG6, core, lpf_bias_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, LPF_REG6, core, lpf_bq_pu, 0x1)

			MOD_RADIO_REG_20698_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_bias_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, TIA_REG7, core, tia_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, TIA_REG7, core, tia_bias_pu, 0x1)

			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx_ldo_pu, 0x1)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG4, core, rx_ldo_pu, 0x1)
		RADIO_REG_LIST_EXECUTE(pi, core);
		if (ch < 15) {
			RADIO_REG_LIST_START
				/* ----- turn on rx2g core${core} only ----- */
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_lna1_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_rx2g_gm_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core,
					lna2g_lna1_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core,
					lna2g_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG3, core, rx2g_gm_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_lna1_gain, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core,
					lna2g_lna1_gain, 0x5)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_rx2g_gm_gain, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG3, core, rx2g_gm_gain, 0x3)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_lna1_Rout, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core,
					lna2g_lna1_Rout, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG1, core, rx2g_lo_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_rx2g_lo_en, 0x1)

				/* ----- turn off rx5g core${core} only ----- */
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_gm_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_mix_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_pu, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG4, core, rx5g_gm_pu, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG5, core, rx5g_mix_pu, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core,
					rx5g_lna_tr_rx_en, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_rx_rccr_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_rx_rccr_pu, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_buf_rccr_pu, 0x0)
			RADIO_REG_LIST_EXECUTE(pi, core);
		} else {
			RADIO_REG_LIST_START
				/* ----- turn off rx2g core${core} only ----- */
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_lna1_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_lna2g_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core,
					ovr_rx2g_gm_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core, lna2g_lna1_pu, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core, lna2g_tr_rx_en, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG3, core, rx2g_gm_en, 0x0)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_div2_rxbuf_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_div2_rxbuf_pu, 0x0)

				/* ----- turn on rx5g core${core} only ----- */
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_gm_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_mix_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG4, core, rx5g_gm_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG5, core, rx5g_mix_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core,
					rx5g_lna_tr_rx_en, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_rx_rccr_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_rx_rccr_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core,
					logen_buf_rccr_pu, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_gm_gc, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_gc, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_gc, 0x7)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG5, core, rx5g_gm_gc, 0x3)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
					ovr_rx5g_lna_rout, 0x1)
				MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_rout, 0x7)
			RADIO_REG_LIST_EXECUTE(pi, core);
		}
	} else {
		RADIO_REG_LIST_START
			MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core, lna2g_lna1_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, LNA2G_REG1, core, lna2g_tr_rx_en, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_REG3, core, rx2g_gm_en, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_REG0, core, logen_buf_rccr_pu, 0x0)

			MOD_RADIO_REG_20698_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bq_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, LPF_OVR1, core, ovr_lpf_bias_pu, 0x0)

			MOD_RADIO_REG_20698_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_bias_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, TIA_CFG1_OVR, core, ovr_tia_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx_ldo_pu, 0x0)

			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_lna2g_lna1_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_lna2g_tr_rx_en, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx2g_gm_en, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
				ovr_logen_div2_rxbuf_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_lna2g_lna1_gain, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx2g_gm_gain, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_lna2g_lna1_Rout, 0x0)

			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG4, core, rx5g_gm_pu, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG5, core, rx5g_mix_pu, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG5, core, rx5g_gm_gc, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_pu, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_tr_rx_en, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_gc, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_REG1, core, rx5g_lna_rout, 0X0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_gm_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_mix_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_lna_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core,
				ovr_rx5g_lna_tr_rx_en, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_gm_gc, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_lna_gc, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX5G_CFG1_OVR, core, ovr_rx5g_lna_rout, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, LOGEN_CORE_OVR0, core,
				ovr_logen_rx_rccr_pu, 0x0)
			MOD_RADIO_REG_20698_ENTRY(pi, RX2G_CFG1_OVR, core, ovr_rx2g_lo_en, 0x0)

		RADIO_REG_LIST_EXECUTE(pi, core);
	}
}

void wlc_phy_radio20708_afediv_pu(phy_info_t *pi, uint8 pll_core, bool pwrup)
{
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_pu_clk_buffers, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG2, pll_core, afediv_pu_clk_buffers, pwrup);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_dac_div_pu, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pll_core, afediv_dac_div_pu, pwrup);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_adc_div_pu, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pll_core, afediv_adc_div_pu, pwrup);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_pu_inbuf, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pll_core, afediv_pu_inbuf, pwrup);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_outbuf_adc_pu, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pll_core, afediv_pu_outbuf_adc, pwrup);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_CFG1_OVR, pll_core, ovr_afediv_outbuf_dac_pu, 1);
	MOD_RADIO_PLLREG_20708(pi, AFEDIV_REG0, pll_core, afediv_pu_outbuf_dac, pwrup);
}

void wlc_phy_radio20708_afediv_control(phy_info_t *pi, uint8 mode)
{
	/* 20708_procs.tcl 20708_afediv_control */
	/* logen_mode   1 4x4, PLL0 only */
	/* logen_mode   2 3+1, PLL0 for Main, PLL1 for Scan */
	/* logen_mode   4 4x4, PLL1 only */
	/* logen_mode   5 3+1, PLL1 for Main, PLL0 for Scan */

	wlc_phy_radio20708_afediv_pu(pi, 0, 1);

	if (mode == 1) { /* 4x4, PLL0 only */
		wlc_phy_radio20708_afediv_pu(pi, 1, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_afe_div_srst_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_adc2x_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_dac_sel, 0);
	} else if (mode == 4) { /* 4x4, PLL1 only */
		wlc_phy_radio20708_afediv_pu(pi, 1, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_afe_div_srst_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_adc2x_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_dac_sel, 0);
	} else if (mode == 2) { /* 3+1, PLL0 for Main, PLL1 for Scan */
	        wlc_phy_radio20708_afediv_pu(pi, 1, 1);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_afe_div_srst_sel, 1);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_adc2x_sel, 1);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_dac_sel, 1);
	} else if (mode == 5) { /* 3+1, PLL1 for Main, PLL0 for Scan */
		wlc_phy_radio20708_afediv_pu(pi, 1, 1);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_afe_div_srst_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_adc2x_sel, 0);
		MOD_RADIO_REG_20708(pi, AFEDIV_REG8, 3, i_clk_to_afe_dac_sel, 0);
	}
}

void wlc_phy_radio20708_pll_logen_pu_sel(phy_info_t *pi, uint8 mode_p1c, uint8 pll_core)
{
	if ((mode_p1c == 0) && (pll_core == 0)) {
		/* 4x4 (PLL0) */
		wlc_phy_radio20708_powerup_RFPll(pi, 0, 1); /* power up PLL0 */
		wlc_phy_radio20708_powerup_RFPll(pi, 1, 0); /* power down PLL1 */
		wlc_phy_radio20708_sel_logen_mode(pi, 1);
		wlc_phy_radio20708_afediv_control(pi, 1);
	} else if ((mode_p1c == 0) && (pll_core == 1)) {
		/* 4x4 (PLL1) */
		wlc_phy_radio20708_powerup_RFPll(pi, 1, 1); /* power up PLL1 */
		wlc_phy_radio20708_powerup_RFPll(pi, 0, 0); /* power down PLL0 */
		wlc_phy_radio20708_sel_logen_mode(pi, 4);
		wlc_phy_radio20708_afediv_control(pi, 4);
	} else if ((mode_p1c == 1) && (pll_core == 0)) {
		/* 3+1 (PLL0, PLL1) -> PLL0 for Main, PLL1 for Scan */
		wlc_phy_radio20708_powerup_RFPll(pi, 1, 1); /* power up PLL1 */
		wlc_phy_radio20708_powerup_RFPll(pi, 0, 1); /* power up PLL0 */
		wlc_phy_radio20708_sel_logen_mode(pi, 2);
		wlc_phy_radio20708_afediv_control(pi, 2);
	} else if ((mode_p1c == 1) && (pll_core == 1)) {
		/* 3+1 (PLL1, PLL0) -> PLL1 for Main, PLL0 for Scan */
		wlc_phy_radio20708_powerup_RFPll(pi, 1, 1); /* power up PLL1 */
		wlc_phy_radio20708_powerup_RFPll(pi, 0, 1); /* power up PLL0 */
		wlc_phy_radio20708_sel_logen_mode(pi, 5);
		wlc_phy_radio20708_afediv_control(pi, 5);
	}
}

void wlc_phy_radio20708_logen_core_setup(phy_info_t *pi, chanspec_t chanspec_sc,
	bool zwdfs, bool restore)
{
	uint8 core, set;
	chanspec_t chanspec = pi->radio_chanspec;
	/* 0(2g4x4) 1(5g4x4) 2(2g3x3+5g1x1 core3) 3(5g3x3+2g1x1 core3) */
	const uint8 mux_sel[4]	= {1, 0, 0, 1};
	const uint8 buff_pu[4]	= {1, 0, 0, 1};
	const uint8 div2_pu[4]	= {1, 0, 0, 1};
	const uint8 dll_pu[4]	= {0, 1, 1, 0};

	if (zwdfs) {
		if (CHSPEC_IS2G(chanspec) && CHSPEC_IS2G(chanspec_sc)) {
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_mux_sel, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG5, 3, logen_core_mux_sel, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_2g_buff_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, 3, logen_core_2g_buff_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_div2_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, 3, logen_core_div2_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_dll_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG2, 3, logen_core_dll_pu, 0);
			return;
		} else if (CHSPEC_IS5G(chanspec) && CHSPEC_IS5G(chanspec_sc)) {
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_mux_sel, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG5, 3, logen_core_mux_sel, 0);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_2g_buff_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, 3, logen_core_2g_buff_pu, 0);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_div2_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, 3, logen_core_div2_pu, 0);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, 3, ovr_logen_core_dll_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG2, 3, logen_core_dll_pu, 1);
			return;
		}
	}

	FOREACH_CORE(pi, core) {
		if (zwdfs && (CHSPEC_IS2G(chanspec) && CHSPEC_ISPHY5G6G(chanspec_sc)) &&
			(core == 3)) {
			set = 2;
		} else if (zwdfs && (CHSPEC_ISPHY5G6G(chanspec) && CHSPEC_IS2G(chanspec_sc)) &&
			(core == 3)) {
			set = 3;
		} else {
			set = CHSPEC_IS2G(chanspec) ? 0 : 1;
		}

		if (!zwdfs || (zwdfs && ((CHSPEC_IS2G(chanspec) && CHSPEC_ISPHY5G6G(chanspec_sc)) ||
			(CHSPEC_ISPHY5G6G(chanspec) && CHSPEC_IS2G(chanspec_sc))) && (core == 3))) {
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core, ovr_logen_core_mux_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG5, core, logen_core_mux_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core, ovr_logen_core_mux_sel, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG5, core,
				logen_core_mux_sel, mux_sel[set]);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
				ovr_logen_core_2g_buff_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, core,
				logen_core_2g_buff_pu, buff_pu[set]);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core, ovr_logen_core_div2_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG0, core,
				logen_core_div2_pu, div2_pu[set]);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core, ovr_logen_core_dll_pu, 1);
			MOD_RADIO_REG_20708(pi, LOGEN_CORE_REG2, core,
				logen_core_dll_pu, dll_pu[set]);

			if (restore) {
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_core_mux_sel, 0);
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_core_2g_buff_pu, 0);
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_core_div2_pu, 0);
				MOD_RADIO_REG_20708(pi, LOGEN_CORE_OVR0, core,
					ovr_logen_core_dll_pu, 0);
			}
		}
	}
}

/*
 * Fill channel vector map with valid 20MHz channels for the selected band based
 * on the radio tuning table of the current chip. It returns the number of channels
 * set in the vector map.
 */
static int
phy_ac_radio_get_valid_chanvec(phy_type_radio_ctx_t *ctx, chanspec_band_t band,
	chanvec_t *valid_chans)
{
	phy_ac_radio_info_t *info = (phy_ac_radio_info_t *)ctx;
	phy_info_t *pi = info->pi;
	uint32 tbl_len, i, n_20m_chans = 0;

	if (RADIOID_IS(pi->pubpi->radioid, BCM20698_ID)) {
		const chan_info_radio20698_rffe_t *chan_info_tbl;

		tbl_len = phy_get_chan_tune_tbl_20698(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* rev3 and higher have table per band */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, 3,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20704_ID)) {
		const chan_info_radio20704_rffe_t *chan_info_tbl;

		tbl_len = wlc_get_20704_chan_tune_table(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* rev3 and higher have table per band */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, 3,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20707_ID)) {
		const chan_info_radio20707_rffe_t *chan_info_tbl;

		tbl_len = phy_get_chan_tune_tbl_20707(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* all revs have table per band */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, 0,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20708_ID)) {
		const chan_info_radio20708_rffe_t *chan_info_tbl;

		tbl_len = phy_get_chan_tune_tbl_20708(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* all revs have table per band */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, 0,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20709_ID)) {
		const chan_info_radio20709_rffe_t *chan_info_tbl;

		tbl_len = wlc_get_20709_chan_tune_table(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* one table containing multiple bands */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, -1,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	} else if (RADIOID_IS(pi->pubpi->radioid, BCM20710_ID)) {
		const chan_info_radio20710_rffe_t *chan_info_tbl;

		tbl_len = wlc_get_20710_chan_tune_table(pi, band, &chan_info_tbl);
		for (i = 0; i < tbl_len; i++) {
			/* table per band */
			n_20m_chans += phy_radio_add_valid_20MHz_chan(info->ri, 3,
				band, chan_info_tbl[i].freq, chan_info_tbl[i].channel, valid_chans);
		}
	}
	return n_20m_chans;
}

void wlc_phy_radio20708_refdoubler_delay_cal(phy_info_t *pi)
{
	uint8 stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);

	stall_val = READ_PHYREGFLD(pi, RxFeCtrl1, disable_stalls);
	ACPHY_DISABLE_STALL(pi);

	MOD_RADIO_PLLREG_20708(pi, PLL_REFDOUBLER5, 0, RefDoubler_cal_init, 1);
	MOD_RADIO_PLLREG_20708(pi, PLL_REFDOUBLER5, 0, RefDoubler_cal_init, 0);
	OSL_DELAY(100);

	ACPHY_ENABLE_STALL(pi, stall_val);
}
