/*
 * Required functions exported by the wlc_pdrssi.c
 * to common driver code
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
 * $Id: wlc_pdmthd.h 491282 2014-07-15 19:26:17Z $
 */
#ifndef _wlc_pdmthd_h
#define _wlc_pdmthd_h

/****************************************************
 * Assumption is that we can use the same interface for different
 * methods. We will start with rssi based proximity method.
 * The state machine is as follows.
*****************************************************
*/
typedef struct pdmthd_interface  pdmthd_if_t;

/****************************************************
 * Function Type:
 * int (*pdm_configure) (pdmthd_if_t *svcif);
 * Purpose:
 * Configures the initialization parameters of any proximity detection
 * methods. The configmsgs_t  is a union that contains configufation
 * messages for many methods.
 * Return value:
*****************************************************
 */
typedef int (*pdm_configure)(pdmthd_if_t *svcif, uint8 mode, wlc_bsscfg_t *bsscfg);

/****************************************************
 * Function Type:
 * int (*pdm_start)(pdmthd_if_t *svcif, bool start);
 * Purpose:
 * Starts the state machine of the proximity method.
 * Return value:
*****************************************************
 */
typedef int (*pdm_start)(pdmthd_if_t *svcif, bool start);

/****************************************************************
 * Function Type:
 * int (*pdm_pushaf)(pdmthd_if_t *  svcif, struct ether_addr *sa,
 * 	struct ether_addr *da, wlc_d11rxhdr_t *wrxh, uint8 *body, int body_len);
 * Purpose:
 * The upper layer pushes the action frames from WLC to proximity detection method.
 * Returnvalue:
*****************************************************************
 */
typedef int (*pdm_pushaf)(pdmthd_if_t *  svcif, struct ether_addr *sa,
	struct ether_addr *da, wlc_d11rxhdr_t *wrxh, uint8 *body, int body_len, uint32 rspec);

/********************************************************
 * Function Type:
 * int (*pdm_status)(pdmthd_if_t *svcif, bool *is_active, wl_proxd_status_iovar_t *iovp);
 * Purpose:
 * This interface get the proximity detection status.
 * Return value:
***********************************************************
 */
typedef int (*pdm_status)(pdmthd_if_t *svcif, bool *is_active, wl_proxd_status_iovar_t *iovp);

/********************************************************
 * Function Type:
 * int (*pdm_release)(pdmthd_if_t *  svcif);
 * Purpose:
 * This interface releases all memory associated with this particular
 * method. Upper layer should not call any further access to proximity
 * detection methods.
 * Returnvalue:
**********************************************************
 */
typedef int (*pdm_release)(pdmthd_if_t *  svcif);

/****************************************************
 * Function Type:
 * int (*pdm_monitor) (pdmthd_if_t *svcif, struct ether_addr *peer);
 * Purpose:
 * Start the monitor mode of state machine of the proximity method.
 * Return value:
*****************************************************
 */
typedef int (*pdm_monitor)(pdmthd_if_t *svcif, struct ether_addr *peer);

/****************************************************
 * Function Type:
 * typedef int (*pdm_rw_params)(void *params, len, bool write);
	read/write module parameters
*****************************************************
 */
typedef int (*pdm_rw_params)(pdmthd_if_t *svcif, void *pbuf, int len, bool write);

/****************************************************
 * Function Type:
 * typedef int (*pdm_collect)(void *params, ...);
	read module collecting data
*****************************************************
 */
typedef int (*pdm_collect)(pdmthd_if_t *svcif, wl_proxd_collect_query_t *quety,
	void *buff, int len, uint16 *reqLen);

/* structure declaration for function interfaces */
/* function calls for proximity detection method */
struct pdmthd_interface {
	pdm_configure			mconfig;	/* config function */
	pdm_start			mstart;		/* for start & stop */
	pdm_pushaf			mpushaf;	/* process rxed action frame */
	pdm_status			mstatus;	/* get status */
	pdm_release			mrelease;	/* release the method */
	pdm_monitor			mmonitor;	/* monitor the state */
	pdm_rw_params			rw_params;	/* get parameters */
	pdm_collect			collect;	/* get collect data */
	wl_proxd_params_common_t	*params_ptr;	/* points to methods params */
};

/* structure declaration for function tof_rtd_adj */
struct tof_rtd_adj_params {
	int		bw;		/* bandwidth */
	int32		*H;		/* channel freq response */
	int32		*Hi;		/* obsolete keep it for ROM validation */
	int		thresh_scale[2];/* scale number of simple threshold crossing */
	int		w_len;		/* search window length */
	int		w_offset;	/* search window offset */
	int32		*w_ext;		/* hardware channel smoothing data */
	int32		gd_ns;		/* gd in ns */
	int32		adj_ns;		/* RX time adjustment */
	int32		*p_A;		/* RSSI refine */
	int		thresh_log2[2]; /* log2 number of simple threshold crossing */
	bool		gd_shift;	/* center window using gd */
	int32		gd;			/* gd in samples */

};

/*
 * proximity service (pdsvc.c) keeps all methods .create calls in an array of fn[] ptrs
 * of this type, the array is initialized in pdsvc ATTACH function
 * so all method's create fn must follow this prototype ( e.g TOF cr method)
 */
typedef pdmthd_if_t* (*proxd_method_create)(wlc_info_t *wlc, uint16 mode,
	pdsvc_funcs_t* funcsp, struct ether_addr *selfmac, pdsvc_payload_t *payloadp);

/* RSSI factory method is defined here.: original RSSI cr method   */
extern pdmthd_if_t* wlc_pdrssi_create_method(wlc_info_t *wlc, uint16 mode,
	pdsvc_funcs_t* funcsp, struct ether_addr *selfmac, pdsvc_payload_t *payloadp);

/*  new TOF  method  */
extern pdmthd_if_t* wlc_pdtof_create_method(wlc_info_t *wlc, uint16 mode,
	pdsvc_funcs_t* funcsp, struct ether_addr *selfmac, pdsvc_payload_t *payloadp);

extern uint32 wlc_pdsvc_average(uint32 *arr, int n);
extern uint32 wlc_pdsvc_deviation(uint32 *arr, int n, uint8 decimaldigits);
extern uint32 wlc_pdsvc_mode(wlc_info_t *wlc, uint32 *arr, int n, uint32 *totalmodes,
	uint32 *modecnt, uint32 *maxp, uint32 *minp, int range);
extern uint32 wlc_pdsvc_median(uint32 *arr, int n);
extern uint32 wlc_pdsvc_sqrt(uint32 x);
extern int wlc_pdtof_get_tune(pdmthd_if_t* svcif, void *pbuf, int len);
extern int wlc_pdtof_set_tune(pdmthd_if_t* svcif, void *pbuf, int len);

extern int tof_rtd_adj(wlc_info_t *wlc, struct tof_rtd_adj_params *params);
extern int wlc_pdtof_get_ftmperiod(pdmthd_if_t* svcif);
extern int wlc_pdtof_set_ftmperiod(pdmthd_if_t* svcif, uint32 val);
extern int wlc_pdtof_allowmac(pdmthd_if_t* svcif, struct ether_addr *addr);
extern uint16 wlc_pdtof_get_tx_mask(pdmthd_if_t* svcif);

#ifdef RSSI_REFINE
extern int32 find_crossing(int32* p_A, int max, int nfft, uint32 threshold);
#endif
extern int wlc_tof_seq_iov(pdmthd_if_t* svcif, uint32 flags, int* p_result);
extern struct ether_addr *wlc_pdsvc_report_list(wlc_info_t *wlc, int *cntptr);

#endif /* _wlc_pdmthd_h */
