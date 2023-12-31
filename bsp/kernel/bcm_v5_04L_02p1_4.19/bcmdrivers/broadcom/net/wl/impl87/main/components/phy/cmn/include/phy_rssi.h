/*
 * RSSI Compute module internal interface (to other PHY modules).
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
 * $Id: phy_rssi.h 685864 2017-02-19 17:48:59Z $
 */

#ifndef _phy_rssi_h_
#define _phy_rssi_h_

#include <typedefs.h>
#include <phy_api.h>

/* forward declaration */
typedef struct phy_rssi_info phy_rssi_info_t;

/* attach/detach */
phy_rssi_info_t *phy_rssi_attach(phy_info_t *pi);
void phy_rssi_detach(phy_rssi_info_t *ri);

/*
 * Compare rssi at different antennas and return the antenna index that
 * has the largest rssi value.
 *
 * Return value is a bitvec, the bit index of '1' is the antenna index.
 */
uint8 phy_rssi_compare_ant(phy_rssi_info_t *ri);

/*
 * rssi merge mode?
 */
#define RSSI_ANT_MERGE_MAX	0	/* pick max rssi of all antennas */
#define RSSI_ANT_MERGE_MIN	1	/* pick min rssi of all antennas */
#define RSSI_ANT_MERGE_AVG	2	/* pick average rssi of all antennas */

/*
 * Init gain error table.
 */
void phy_rssi_init_gain_err(phy_rssi_info_t *ri);
int wlc_phy_sharedant_acphy(phy_info_t *pi);

#if defined(WLTEST)
int wlc_phy_pkteng_stats_get(phy_rssi_info_t *rssii, void *a, int alen, int8 *gain_correct);
#endif

int phy_rssi_set_gain_delta_2g(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);
int phy_rssi_get_gain_delta_2g(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);
int phy_rssi_set_gain_delta_5g(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);
int phy_rssi_get_gain_delta_5g(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);

int phy_rssi_set_gain_delta_2gb(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);
int phy_rssi_get_gain_delta_2gb(phy_rssi_info_t *rssii, uint32 aid, int8 *deltaValues);
int phy_rssi_set_cal_freq_2g(phy_rssi_info_t *rssii, int8 *nvramValues);
int phy_rssi_get_cal_freq_2g(phy_rssi_info_t *rssii, int8 *nvramValues);

int8 phy_rssi_get_rssi(phy_info_t *pi, const uint8 core);
#endif /* _phy_rssi_h_ */
