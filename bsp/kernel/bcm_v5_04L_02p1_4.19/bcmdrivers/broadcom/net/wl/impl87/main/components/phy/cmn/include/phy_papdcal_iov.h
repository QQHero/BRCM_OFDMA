/*
 * PAPD CAL module internal interface - iovar table registration
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
 * $Id: phy_papdcal_iov.h 802962 2021-09-10 22:14:35Z $
 */

#ifndef _phy_papdcal_iov_t_
#define _phy_papdcal_iov_t_

#include <phy_api.h>

/* iovar ids */
enum {
	IOV_PHY_ENABLE_EPA_DPD_2G = 1,
	IOV_PHY_ENABLE_EPA_DPD_5G = 2,
	IOV_PHY_PACALIDX0 = 3,
	IOV_PHY_PACALIDX1 = 4,
	IOV_PHY_PACALIDX = 5,
	IOV_PHY_PAPDBBMULT = 6,
	IOV_PHY_PAPDEXTRAEPSOFFSET = 7,
	IOV_PHY_PAPDTIAGAIN = 8,
	IOV_PAPDCOMP_DISABLE = 9,
	IOV_PAPD_EN_WAR = 10,
	IOV_PHY_SKIPPAPD = 11,
	IOV_PHY_PAPD_ENDEPS = 12,
	IOV_PHY_PAPD_EPSTBL = 13,
	IOV_PHY_PAPD_DUMP = 14,
	IOV_PHY_PAPD_ABORT = 15,
	IOV_PHY_EPACAL2GMASK = 16,
	IOV_PHY_WFD_LL_ENABLE = 17,
	IOV_PHY_WBPAPD_GCTRL = 18,
	IOV_PHY_WBPAPD_MULTITBL = 19,
	IOV_PHY_WBPAPD_SAMPCAPT = 20,
	IOV_PHY_PEAK_PSD_LIMIT = 21
};

/* register iovar table/handlers */
int phy_papdcal_register_iovt(phy_info_t *pi, wlc_iocv_info_t *ii);

#endif /* _phy_papdcal_iov_t_ */
