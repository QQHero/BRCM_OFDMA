/*
 * MAC debug and print functions
 * Broadcom 802.11bang Networking Device Driver
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
 * $Id$
 */
#ifndef WLC_MACDBG_H_
#define WLC_MACDBG_H_

#include <typedefs.h>
#include <wlc_types.h>
#include <wlioctl.h>

/* fatal reason code */
#define PSM_FATAL_ANY		0
#define PSM_FATAL_PSMWD		1
#define PSM_FATAL_SUSP		2
#define PSM_FATAL_WAKE		3
#define PSM_FATAL_TXSUFL	4
#define PSM_FATAL_PSMXWD	5
#define PSM_FATAL_TXSTUCK	6
#define PSM_FATAL_LAST		7

#define PSMX_FATAL_ANY		0
#define PSMX_FATAL_PSMWD	1
#define PSMX_FATAL_SUSP		2
#define PSMX_FATAL_TXSTUCK	3
#define PSMX_FATAL_LAST		4

#define	PRVAL(name)	bcm_bprintf(b, "%s %d ", #name, WLCNTVAL(cnt->name))
#define	PRNL()		bcm_bprintf(b, "\n")
#define PRVAL_RENAME(varname, prname)	\
	bcm_bprintf(b, "%s %d ", #prname, WLCNTVAL(cnt->varname))

/* attach/detach */
extern wlc_macdbg_info_t *wlc_macdbg_attach(wlc_info_t *wlc);
extern void wlc_macdbg_detach(wlc_macdbg_info_t *macdbg);

#if defined(DONGLEBUILD) && defined(WLC_HOSTPMAC)
extern void wlc_macdbg_sendup_d11regs(wlc_macdbg_info_t *macdbg);
#else
#define wlc_macdbg_sendup_d11regs(a) do {} while (0)
#endif

extern void wlc_dump_ucode_fatal(wlc_info_t *wlc, uint reason);

/* catch any interrupts from psmx */
#ifdef WL_PSMX
void wlc_bmac_psmx_errors(wlc_info_t *wlc);
void wlc_dump_psmx_fatal(wlc_info_t *wlc, uint reason);
#ifdef VASIP_HW_SUPPORT
void wlc_dump_vasip_fatal(wlc_info_t *wlc);
#endif
#else
#define wlc_bmac_psmx_errors(wlc) do {} while (0)
#endif /* WL_PSMX */

extern void wlc_dump_mac_fatal(wlc_info_t *wlc, uint reason);
#ifdef RXDMA_STUCK_WAR
extern bool wlc_macdbg_is_rxdma_stuck(wlc_info_t *wlc, uint rxfifo);
#endif
#endif /* WLC_MACDBG_H_ */
