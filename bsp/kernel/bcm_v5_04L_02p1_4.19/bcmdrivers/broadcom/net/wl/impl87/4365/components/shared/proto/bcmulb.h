/*
 * ULB IE and frame definitions
 * (Ultra Low Bandwidth)
 *
 * As part of ULB Dynamic Operation following is the new element defined
 * (Broadcom Proprietary IE, OUI=>0x00904c):
 *	- ULB IE (OUI=>0x00904c, OUI_Type=>91) with following attribute
 *	- ULB Capability Attribute  (TLV attribute with attribute ID = 1)
 *	- ULB Operations Attribute  (TLV attribute with attribute ID = 2)
 *	- ULB Mode Switch Attribute (TLV attribute with attribute ID = 3)
 *
 * Following new Proprietary Action Frames have been defined
 * (Action Category=>127, OUI=>0x00904c, Action_Type=> 15):
 *	- ULB Mode Switch Request Action Frame  (Action_Sub_Type => 1)
 *	- ULB Mode Switch Response Action Frame (Action_Sub_Type => 2)
 *
 * Refer to following Twiki for further details:
 * http://hwnbu-twiki.broadcom.com/bin/view/Mwgroup/UltraLowBandMode
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
 * $Id: bcmulb.h 682869 2017-02-03 14:13:37Z $
 */
#ifndef _BCMULB_H_
#define _BCMULB_H_

#include <typedefs.h>
#include <proto/802.11.h>

/* This marks the start of a packed structure section. */
#include <packed_section_start.h>

#define ULB_BRCM_PROP_IE_TYPE		91
#define ULB_CAP_ATTR_ID			1
#define ULB_CAP_ATTR_LEN		(sizeof(ulb_cap_attr_t))
#define ULB_OPR_ATTR_ID			2
#define ULB_OPR_ATTR_LEN		(sizeof(ulb_opr_attr_t))
#define ULB_MODE_SW_ATTR_ID		3
#define ULB_MODE_SW_ATTR_LEN		(sizeof(ulb_mode_sw_attr_t))
#define ULB_BRCM_PROP_IE_OUI_OVRHD	4
/* Minimum ULB Prop-IE length. There must be atleast be Capability Attribute */
#define MIN_ULB_BRCM_PROP_IE_LEN	(ULB_BRCM_PROP_IE_OUI_OVRHD + ULB_CAP_ATTR_LEN)
#define ULB_MODE_SW_REQ_AF_LEN		sizeof(ulb_mode_sw_req_t)
#define ULB_MODE_SW_RSP_AF_LEN		sizeof(ulb_mode_sw_rsp_t)
#define ULB_DYN_FRAG_AF_SIZE		sizeof(ulb_dyn_frag_req_t)

/* Action frame type for ULB */
#define BRCM_ULB_AF_TYPE		15
enum {
	BRCM_ULB_SWITCH_REQ_SUBTYPE = 1,	/* ULB Mode Switch Request */
	BRCM_ULB_SWITCH_RSP_SUBTYPE = 2,	/* ULB Mode Switch Response */
	BRCM_ULB_DYN_FRAG_SUBTYPE   = 3,	/* ULB Mode DynFrag Request */
};

/* ULB Mode Switch Status Code - Used in sending ULB Mode Switch Response Action Frame */
enum {
	ULB_MODE_SW_SC_INVALID = 0,
	ULB_MODE_SW_SC_SUCCESS = 1
};

/* BRCM ULB IE format */
BWL_PRE_PACKED_STRUCT struct ulb_prop_ie {
	uint8	id;		/* IE ID, 221, DOT11_MNG_VS_ID */
	uint8	len;		/* IE length */
	uint8	oui[3];
	uint8	type;           /* type indicates what follows */
	uint8 data[1];		/* Pointer to ULB attributes in TLV format */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_prop_ie ulb_prop_ie_t;

/* ULB Operations field format */
BWL_PRE_PACKED_STRUCT struct ulb_opr_field {
	uint8	cur_opr_bw;	/* Current Operational Bandwidth */
	uint8	pri_opr_bw;	/* Primary Operational Bandwidth */
	uint8	pri_ch_num;	/* Primary Channel Number */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_opr_field ulb_opr_field_t;

/* ULB Capability Attribute format */
BWL_PRE_PACKED_STRUCT struct ulb_cap_attr {
	uint8	id;			/* Attribute ID, ULB_CAP_ATTR_ID */
	uint8	len;			/* Attribute length */
	uint8	ulb_stdaln_cap;		/* Standalone Capability */
	uint8	ulb_dyn_coex_cap;	/* Dynamic Coex Capability */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_cap_attr ulb_cap_attr_t;

/* ULB Operations Attribute format */
BWL_PRE_PACKED_STRUCT struct ulb_opr_attr {
	uint8	id;			/* Attribute ID, ULB_OPR_ATTR_ID */
	uint8	len;			/* Attribute length */
	ulb_opr_field_t ulb_opr_field;	/* ULB Operations Field */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_opr_attr ulb_opr_attr_t;

/* ULB Mode Switch Attribute format */
BWL_PRE_PACKED_STRUCT struct ulb_mode_sw_attr {
	uint8	id;			/* Attribute ID, ULB_MODE_SW_ATTR_ID */
	uint8	len;			/* Attribute length */
	ulb_opr_field_t ulb_opr_field;	/* ULB Operations Field */
	uint8	cnt;			/* Mode SW Count Field */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_mode_sw_attr ulb_mode_sw_attr_t;

/* ULB Action Frame */
/* ULB Mode Switch Request Action Frame Format */
BWL_PRE_PACKED_STRUCT struct ulb_mode_sw_req {
	uint8	category;		/* DOT11_ACTION_CAT_VS */
	uint8	OUI[3];			/* OUI - BRCM_PROP_OUI */
	uint8	type;			/* Action VS Type - BRCM_ULB_AF_TYPE */
	uint8	subtype;		/* Action VS Subtype - BRCM_ULB_SWITCH_REQ_SUBTYPE */
	uint8	dia_token;		/* nonzero, identifies req/rsp transaction */
	ulb_opr_field_t	opr_field;	/* ULB Operations Field */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_mode_sw_req ulb_mode_sw_req_t;

/* ULB Mode Switch Response Action Frame Format */
BWL_PRE_PACKED_STRUCT struct ulb_mode_sw_rsp {
	uint8	category;		/* DOT11_ACTION_CAT_VS */
	uint8	OUI[3];			/* OUI - BRCM_PROP_OUI */
	uint8	type;			/* Action VS Type - BRCM_ULB_AF_TYPE */
	uint8	subtype;		/* Action VS Subtype - BRCM_ULB_SWITCH_RSP_SUBTYPE */
	uint8	dia_token;		/* nonzero, identifies req/rsp transaction */
	uint8	status;			/* Mode Switch Request Status */
	ulb_opr_field_t	opr_field;	/* ULB Operations Field */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_mode_sw_rsp ulb_mode_sw_rsp_t;

/* ULB Action Frame */
/* ULB Dynamic fragmentation Action Frame Format */
BWL_PRE_PACKED_STRUCT struct ulb_dyn_frag_req {
	uint8	category;		/* DOT11_ACTION_CAT_VS */
	uint8	OUI[3];			/* OUI - BRCM_PROP_OUI */
	uint8	type;			/* Action VS Type - BRCM_ULB_AF_TYPE */
	uint8	subtype;		/* Action VS Subtype - BRCM_ULB_DYN_FRAG_SUBTYPE */
	uint32  bt_tasks;		/* Bt  task types */
} BWL_POST_PACKED_STRUCT;
typedef struct ulb_dyn_frag_req ulb_dyn_frag_req_t;

/* This marks the end of a packed structure section. */
#include <packed_section_end.h>

#endif /* _BCMULB_H_ */
