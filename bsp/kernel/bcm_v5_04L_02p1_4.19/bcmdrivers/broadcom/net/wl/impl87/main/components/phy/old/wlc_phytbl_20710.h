/*
 * Radio 20710 table definition header file
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
 * $Id$
 */

#ifndef _WLC_PHYTBL_20710_H_
#define _WLC_PHYTBL_20710_H_

/* XXX: Define wlc_cfg.h to be the first header file included as some builds
 * get their feature flags thru this file.
 */
#include <wlc_cfg.h>
#include <typedefs.h>

#include "wlc_phy_int.h"
#include "phy_ac_rxgcrs.h"

typedef struct _chan_info_radio20710_rffe_2G {
	/* 2G tuning data */
	uint8 RFP0_logen_reg1_logen_mix_ctune;
	uint8 RF0_rx5g_reg1_rxdb_lna_tune;
	uint8 RF1_rx5g_reg1_rxdb_lna_tune;
	uint8 RF0_logen_core_reg3_logen_lc_ctune;
	uint8 RF1_logen_core_reg3_logen_lc_ctune;
	uint8 RF0_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF1_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF0_txdb_pad_reg3_txdb_pad5g_tune;
	uint8 RF1_txdb_pad_reg3_txdb_pad5g_tune;
	uint8 RF0_rx5g_reg5_rxdb_mix_Cin_tune;
	uint8 RF1_rx5g_reg5_rxdb_mix_Cin_tune;
} chan_info_radio20710_rffe_2G_t;

typedef struct _chan_info_radio20710_rffe_5G {
	/* 5G tuning data */
	uint8 RFP0_logen_reg1_logen_mix_ctune;
	uint8 RF0_logen_core_reg3_logen_lc_ctune;
	uint8 RF1_logen_core_reg3_logen_lc_ctune;
	uint8 RF0_rx5g_reg1_rxdb_lna_tune;
	uint8 RF1_rx5g_reg1_rxdb_lna_tune;
	uint8 RF0_rx5g_reg5_rxdb_mix_Cin_tune;
	uint8 RF1_rx5g_reg5_rxdb_mix_Cin_tune;
	uint8 RF0_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF1_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF0_txdb_pad_reg3_txdb_pad5g_tune;
	uint8 RF1_txdb_pad_reg3_txdb_pad5g_tune;
} chan_info_radio20710_rffe_5G_t;

typedef struct _chan_info_radio20710_rffe_6G {
	/* 5G tuning data */
	uint8 RFP0_logen_reg1_logen_mix_ctune;
	uint8 RF0_logen_core_reg3_logen_lc_ctune;
	uint8 RF1_logen_core_reg3_logen_lc_ctune;
	uint8 RF0_rx5g_reg1_rxdb_lna_tune;
	uint8 RF1_rx5g_reg1_rxdb_lna_tune;
	uint8 RF0_rx5g_reg5_rxdb_mix_Cin_tune;
	uint8 RF1_rx5g_reg5_rxdb_mix_Cin_tune;
	uint8 RF0_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF1_tx2g_mix_reg4_txdb_mx_tune;
	uint8 RF0_txdb_pad_reg3_txdb_pad5g_tune;
	uint8 RF1_txdb_pad_reg3_txdb_pad5g_tune;
} chan_info_radio20710_rffe_6G_t;

typedef struct _chan_info_radio20710_rffe {
	uint16 channel;
	uint16 freq;
	union {
		/* In this union, make sure the largest struct is at the top. */
		chan_info_radio20710_rffe_6G_t val_6G;
		chan_info_radio20710_rffe_5G_t val_5G;
		chan_info_radio20710_rffe_2G_t val_2G;
	} u;
} chan_info_radio20710_rffe_t;

/* Get the right radio tuning table and return the number of elements */
uint16 wlc_get_20710_chan_tune_table(phy_info_t *pi, chanspec_t chanspec,
		const chan_info_radio20710_rffe_t **p_chan_info_tbl);

#if (defined(BCMDBG) && defined(DBG_PHY_IOV)) || defined(BCMDBG_PHYDUMP)
extern const radio_20xx_dumpregs_t dumpregs_20710_rev0[];
#endif

/* Radio referred values tables */
extern const radio_20xx_prefregs_t prefregs_20710_rev0[];

extern int8 BCMATTACHDATA(lna12_gain_tbl_2g_20710rX_ilna)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_2g_20710rX_elna)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_5g_20710rX)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_6gs05_20710rX)[2][N_LNA12_GAINS];
extern int8 BCMATTACHDATA(lna12_gain_tbl_6gs6_20710rX)[2][N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_2g_20710r0)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_2g_20710rx)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_5g_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_2g_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_5g_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs0_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs0_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs1_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs1_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs2_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs2_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs3_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs3_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs4_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs4_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs5_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs5_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_rout_map_6gs6_20710rX)[N_LNA12_GAINS];
extern uint8 BCMATTACHDATA(lna1_gain_map_6gs6_20710rX)[N_LNA12_GAINS];
extern int8 BCMATTACHDATA(gainlimit_tbl_20710rX)[RXGAIN_CONF_ELEMENTS][MAX_RX_GAINS_PER_ELEM];
extern int8 BCMATTACHDATA(tia_gain_tbl_20710rX)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(tia_gainbits_tbl_20710rX)[N_TIA_GAINS];
extern int8 BCMATTACHDATA(biq01_gain_tbl_20710rX)[2][N_BIQ01_GAINS];
extern int8 BCMATTACHDATA(biq01_gainbits_tbl_20710rX)[2][N_BIQ01_GAINS];

#endif	/* _WLC_PHYTBL_20710_H_ */
