/*
 * TOF module public interface (to MAC driver).
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
 * $Id: phy_tof_api.h 809672 2022-03-22 04:44:16Z $
 */
#ifndef _phy_tof_api_h_
#define _phy_tof_api_h_

#include <typedefs.h>
#include <phy_api.h>
#include <bcm_math.h>

/* forward declaration */
typedef struct phy_tof_info phy_tof_info_t;

/*  TOF info type mask */
typedef int16 wlc_phy_tof_info_type_t;

enum {
	WLC_PHY_TOF_INFO_TYPE_NONE		= 0x0000,
	WLC_PHY_TOF_INFO_TYPE_FRAME_TYPE	= 0x0001, /* TOF frame type */
	WLC_PHY_TOF_INFO_TYPE_FRAME_BW		= 0x0002, /* Frame BW */
	WLC_PHY_TOF_INFO_TYPE_CFO		= 0x0004, /* CFO */
	WLC_PHY_TOF_INFO_TYPE_RSSI		= 0x0008, /* RSSI */
	WLC_PHY_TOF_INFO_TYPE_SNR		= 0x0010, /* SNR */
	WLC_PHY_TOF_INFO_TYPE_BITFLIPS		= 0x0020, /* No of Bit flips */
	WLC_PHY_TOF_INFO_TYPE_PHYERROR		= 0x0040, /* phy errors -- 1040 */
	WLC_PHY_TOF_INFO_TYPE_ALL		= 0xffff
};

typedef struct wlc_phy_tof_info {
	wlc_phy_tof_info_type_t	info_mask;
	int			frame_type;
	int			frame_bw;
	int			cfo;
	wl_proxd_rssi_t		rssi;
	wl_proxd_snr_t		snr;
	wl_proxd_bitflips_t	bitflips;
	wl_proxd_phy_error_t	tof_phy_error;
} wlc_phy_tof_info_t;

/* #define WL_PROXD_SEQ */
#define WL_PROXD_PHTS_MASK	0x40
#define WL_PROXD_BW_MASK	0x7
#define WL_PROXD_SEQEN		0x80
#define WL_PROXD_80M_40M	1
#define WL_PROXD_80M_20M	2
#define WL_PROXD_40M_20M	3
#define WL_PROXD_160M_80M	4
#define WL_PROXD_160M_40M	5
#define WL_PROXD_160M_20M	6

#define WL_PROXD_RATE_VHT	0
#define WL_PROXD_RATE_6M	1
#define WL_PROXD_RATE_LEGACY	2
#define WL_PROXD_RATE_MCS_0	3
#define WL_PROXD_RATE_MCS	4

#define WL_RSPEC_FTMIDX_MASK 0x00ff
#define WL_RSPEC_FTMIDX_SHIFT 0
#define WL_RSPEC_ACKIDX_MASK 0xff00
#define WL_RSPEC_ACKIDX_SHIFT 8
/* #define TOF_DBG */
/* #define TOF_DBG_SEQ */
/* #define TOF_SEQ_40_IN_40MHz */
/* #define TOF_SEQ_20_IN_80MHz */
/* #define TOF_SEQ_20MHz_BW */
/* #define TOF_SEQ_20MHz_BW_512IFFT */
/* #define TOF_SEQ_40MHz_BW */

/* #define TOF_DEBUG_TIME2 */
#define TOF_PROFILE_BUF_SIZE 15
#define TOF_P_IDX(x) (x < TOF_PROFILE_BUF_SIZE) ? x : 0

#define TOF_CLASSIFIER_MASK			0x7	/* last 3 bits of the register */
#define TOF_CLASSIFIER_BPHY_OFF_OFDM_ON		6
#define TOF_CLASSIFIER_BPHY_ON_OFDM_ON		7

#define PHYTS_ROLE_NONE		0u
#define PHYTS_ROLE_INITIATOR	1u
#define PHYTS_ROLE_TARGET	2u

typedef struct profile_buf
{
	int8 event;
	int8 token;
	int8 follow_token;
	uint32 ts;
} tof_pbuf_t;

typedef struct wlc_phy_tof_secure_2_0 {
	uint16 start_seq_time;
	uint16 delta_time_tx2rx;
} wlc_phy_tof_secure_2_0_t;

int phy_tof_seq_upd_dly(phy_info_t *pi, bool tx, uint8 core, bool mac_suspend);
int phy_tof_seq_params(phy_info_t *pi, bool assign_buffer);
int phy_tof_set_ri_rr(phy_info_t *pi, const uint8* ri_rr, const uint16 len,
	const uint8 core, const bool isInitiator, const bool isSecure,
	wlc_phy_tof_secure_2_0_t  tof_sec_params);
int phy_tof_seq_params_get_set(phy_info_t *pi, uint8 *delays, bool set, bool tx,
	int size);
int phy_tof_dbg(phy_info_t *pi, int arg); /* DEBUG */
void phy_tof_setup_ack_core(phy_info_t *pi, int core);
uint8 phy_tof_num_cores(phy_info_t *pi);
void phy_tof_core_select(phy_info_t *pi, const uint32 gdv_th, const int32 gdmm_th,
		const int8 rssi_th, const int8 delta_rssi_th, uint8* core,
		uint8 core_mask);
int wlc_phy_tof_calc_snr_bitflips(phy_info_t *pi, void *In,
	wl_proxd_bitflips_t *bit_flips, wl_proxd_snr_t *snr);
int phy_tof_chan_freq_response(phy_info_t *pi,
	int len, int nbits, int32* Hr, int32* Hi, uint32* Hraw, const bool single_core,
	uint8 num_sts, bool collect_offset);
int phy_tof_chan_freq_response_api(phy_info_t *pi,
	int len, int nbits, int32* Hr, int32* Hi, uint32* Hraw, const bool single_core,
	uint8 num_sts, bool collect_offset, bool tdcs_en, bool he_frm);
void phy_tof_cmd(phy_info_t *pi, bool seq, int emu_delay);

int wlc_phy_tof(phy_info_t *pi, bool enter, bool tx, bool hw_adj, bool seq_en,
	int core, int emu_delay);
int wlc_phy_tof_info(phy_info_t *pi, wlc_phy_tof_info_t *tof_info,
	wlc_phy_tof_info_type_t tof_info_mask, uint8 core);
int wlc_phy_tof_kvalue(phy_info_t *pi, chanspec_t chanspec, uint32 *kip,
	uint32 *ktp, uint8 seq_en);
int wlc_phy_kvalue(phy_info_t *pi, chanspec_t chanspec, uint32 rspecidx, uint32 *kip,
	uint32 *ktp, uint8 seq_en);

void phy_tof_init_gdmm_th(phy_info_t *pi, int32 *gdmm_th);
void phy_tof_init_gdv_th(phy_info_t *pi, uint32 *gdv_th);

int wlc_phy_chan_mag_sqr_impulse_response(phy_info_t *pi, int frame_type,
	int len, int offset, int nbits, int32* h, int* pgd, uint32* hraw, uint16 tof_shm_ptr);
int wlc_phy_seq_ts(phy_info_t *pi, int n, void* p_buffer, int tx, int cfo, int adj,
	void* pparams, int32* p_ts, int32* p_seq_len, uint32* p_raw, uint8* ri_rr,
	const uint8 smooth_win_en);

#ifdef WL_PROXD_GDCOMP
void phy_tof_gdcomp(cint32* H, int32 theta, int nfft, int delay_imp);
#endif /* WL_PROXD_GDCOMP */

#ifdef WL_PROXD_PHYTS
int phy_tof_phyts_setup_api(phy_info_t *ppi, bool setup, uint8 role);
int phy_tof_phyts_read_sc_api(phy_info_t *ppi, cint16* sc_mem);
int phy_tof_phyts_enable_sc_api(phy_info_t *ppi);
int phy_tof_phyts_pkt_detect_api(phy_info_t *ppi, cint16* sc_mem);
int phy_tof_phyts_mf_api(phy_info_t *ppi, cint16* sc_mem, uint32 *ts, bool mode);
int phy_tof_phyts_set_txpwr_api(phy_info_t *ppi, int8 txpwr_dbm);
int phy_tof_phyts_get_sc_read_buf_sz_api(phy_info_t *ppi, uint32 *sc_read_buf_sz);
#endif /* WL_PROXD_PHYTS */

#endif /* _phy_tof_api_h_ */
