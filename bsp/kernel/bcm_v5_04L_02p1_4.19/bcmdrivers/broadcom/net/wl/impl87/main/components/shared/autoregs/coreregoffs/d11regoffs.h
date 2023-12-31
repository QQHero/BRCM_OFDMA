/**
 * @file
 * @brief
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
 * $Id: $
 */

/*
 * This file is used by autoregs to convert so-called 'named initializations' of autoregs struct
 * members into 'immediate initializations'. GCC's preprocessor is used for this conversion.
 *
 * Example line from d11regs_structs_inits.c (named initialization):
 *     .PSM_CHK1_EADDR_ID = PSM_CHK1_EADDR,
 * Example line from d11regs_partial.c (converted to immediate initialization):
 *     .PSM_CHK1_EADDR_ID = (0x0000089E),
 *
 * This file is read on each corerev to convert.
 */

#if (AUTOREGS_COREREV < 40)
#define biststatus	(0x000c)	/* missing */
#define biststatus2	(0x0010)	/* missing */
#define gptimer	(0x0018)
#define usectimer	(0x001c)
#define intstat0	(0x0020)
#define alt_intmask0	(0x0060)
#define inddma	(0x0080)	/* missing */
#define indaqm	(0x00a0)	/* missing */
#define indqsel	(0x00c0)	/* missing */
#define suspreq	(0x00c4)	/* missing */
#define flushreq	(0x00c8)	/* missing */
#define chnflushstatus	(0x00cc)	/* missing */
#define intrcvlzy0	(0x0100)
#define MACCONTROL	(0x0120)
#define MACCOMMAND	(0x0124)
#define MACINTSTATUS	(0x0128)
#define MACINTMASK	(0x012c)
#define XMT_TEMPLATE_RW_PTR	(0x0130)
#define XMT_TEMPLATE_RW_DATA	(0x0134)
#define PMQHOSTDATA	(0x0140)
#define PMQPATL	(0x0144)
#define PMQPATH	(0x0148)
#define CHNSTATUS	(0x0150)
#define PSM_DEBUG	(0x0154)
#define PHY_DEBUG	(0x0158)
#define MacCapability	(0x015c)
#define objaddr	(0x0160)	/* missing */
#define objdata	(0x0164)	/* missing */
#define ALT_MACINTSTATUS	(0x0168)
#define ALT_MACINTMASK	(0x016c)
#define FrmTxStatus	(0x0170)
#define FrmTxStatus2	(0x0174)
#define FrmTxStatus3	(0x0178)
#define FrmTxStatus4	(0x017c)
#define TSFTimerLow	(0x0180)
#define TSFTimerHigh	(0x0184)
#define CFPRep	(0x0188)
#define CFPStart	(0x018c)
#define CFPMaxDur	(0x0190)
#define AvbRxTimeStamp	(0x0194)
#define AvbTxTimeStamp	(0x0198)
#define MacControl1	(0x01a0)
#define MacHWCap1	(0x01a4)
#define MacPatchCtrl	(0x01ac)
#define gptimer_x	(0x01b0)	/* missing */
#define maccontrol_x	(0x01b4)	/* missing */
#define maccontrol1_x	(0x01b8)	/* missing */
#define maccommand_x	(0x01bc)	/* missing */
#define macintstatus_x	(0x01c0)	/* missing */
#define macintmask_x	(0x01c4)	/* missing */
#define altmacintstatus_x	(0x01c8)	/* missing */
#define altmacintmask_x	(0x01cc)	/* missing */
#define psmdebug_x	(0x01d0)	/* missing */
#define phydebug_x	(0x01d4)	/* missing */
#define statctr2	(0x01d8)	/* missing */
#define statctr3	(0x01dc)	/* missing */
#define ClockCtlStatus	(0x01e0)
#define Workaround	(0x01e4)
#define POWERCTL	(0x01e8)
#define xmt0ctl	(0x0200)
#define fifobase	(0x0380)
#define aggfifocnt	(0x0390)	/* missing */
#define aggfifodata	(0x0394)	/* missing */
#define DebugStoreMask	(0x0398)
#define DebugTriggerMask	(0x039c)
#define DebugTriggerValue	(0x03a0)
#define radioregaddr_cross	(0x03c8)
#define radioregdata_cross	(0x03ca)
#define radioregaddr	(0x03d8)
#define radioregdata	(0x03da)
#define rfdisabledly	(0x03dc)
#define PHY_REG_0	(0x03e0)
#define PHY_REG_1	(0x03e2)
#define PHY_REG_2	(0x03e4)
#define PHY_REG_3	(0x03e6)
#define PHY_REG_4	(0x03e8)
#define PHY_REG_5	(0x03ea)
#define PHY_REG_7	(0x03ec)
#define PHY_REG_6	(0x03ee)
#define PHY_REG_8	(0x03f0)
#define PHY_REG_A	(0x03f4)
#define PHY_REG_B	(0x03f6)
#define PHY_REG_C	(0x03f8)
#define PHY_REG_D	(0x03fa)
#define PHY_REG_ADDR	(0x03fc)
#define PHY_REG_DATA	(0x03fe)
#define RCV_FIFO_CTL	(0x0406)
#define RCV_CTL	(0x0408)
#define RCV_FRM_CNT	(0x040a)
#define RCV_STATUS_LEN	(0x040c)
#define RXE_PHYRS_1	(0x0414)
#define RXE_RXCNT	(0x0418)
#define RXE_STATUS1	(0x041a)
#define RXE_STATUS2	(0x041c)
#define RXE_PLCP_LEN	(0x041e)
#define rcm_ctl	(0x0420)
#define rcm_mat_data	(0x0422)
#define rcm_mat_mask	(0x0424)
#define rcm_mat_dly	(0x0426)
#define rcm_cond_mask_l	(0x0428)
#define rcm_cond_mask_h	(0x042a)
#define rcm_cond_dly	(0x042c)
#define EXT_IHR_ADDR	(0x0430)
#define EXT_IHR_DATA	(0x0432)
#define RXE_PHYRS_2	(0x0434)
#define RXE_PHYRS_3	(0x0436)
#define PHY_MODE	(0x0438)
#define rcmta_ctl	(0x043a)
#define rcmta_size	(0x043c)
#define rcmta_addr0	(0x043e)
#define rcmta_addr1	(0x0440)
#define rcmta_addr2	(0x0442)
#define ext_ihr_status	(0x045e)
#define radio_ihr_addr	(0x0460)
#define radio_ihr_data	(0x0462)
#define radio_ihr_status	(0x0468)
#define PSM_MAC_CTLH	(0x0482)
#define PSM_MAC_INTSTAT_L	(0x0484)
#define PSM_MAC_INTSTAT_H	(0x0486)
#define PSM_MAC_INTMASK_L	(0x0488)
#define PSM_MAC_INTMASK_H	(0x048a)
#define PSM_MACCOMMAND	(0x048e)
#define PSM_BRC_0	(0x0490)
#define PSM_PHY_CTL	(0x0492)
#define PSM_IHR_ERR	(0x0494)
#define DMP_OOB_AIN_MASK	(0x0496)
#define psm_int_sel_2	(0x0498)	/* missing */
#define PSM_GPIOIN	(0x049a)
#define PSM_GPIOOUT	(0x049c)
#define PSM_GPIOEN	(0x049e)
#define PSM_BRED_0	(0x04a0)
#define PSM_BRED_1	(0x04a2)
#define PSM_BRED_2	(0x04a4)
#define PSM_BRED_3	(0x04a6)
#define PSM_BRCL_0	(0x04a8)
#define PSM_BRCL_1	(0x04aa)
#define PSM_BRCL_2	(0x04ac)
#define PSM_BRCL_3	(0x04ae)
#define PSM_BRPO_0	(0x04b0)
#define PSM_BRPO_1	(0x04b2)
#define PSM_BRPO_2	(0x04b4)
#define PSM_BRPO_3	(0x04b6)
#define PSM_BRWK_0	(0x04b8)
#define PSM_BRWK_1	(0x04ba)
#define PSM_BRWK_2	(0x04bc)
#define PSM_BRWK_3	(0x04be)
#define PSM_INTSEL_0	(0x04c0)
#define PSMPowerReqStatus	(0x04c8)
#define psm_ihr_err	(0x04ce)
#define SubrStkStatus	(0x04d0)
#define SubrStkRdPtr	(0x04d2)
#define SubrStkRdData	(0x04d4)
#define psm_pc_reg_3	(0x04d6)	/* missing */
#define PSM_BRC_1	(0x04d8)
#define SBRegAddr	(0x04ea)
#define SBRegDataL	(0x04ec)
#define SBRegDataH	(0x04ee)
#define PSMCoreCtlStat	(0x04f0)
#define SbAddrLL	(0x04f4)
#define SbAddrL	(0x04f6)
#define SbAddrH	(0x04f8)
#define SbAddrHH	(0x04fa)
#define TXE_CTL	(0x0500)
#define TXE_AUX	(0x0502)
#define TXE_TS_LOC	(0x0504)
#define TXE_TIME_OUT	(0x0506)
#define TXE_WM_0	(0x0508)
#define TXE_WM_1	(0x050a)
#define TXE_PHYCTL	(0x050c)
#define TXE_STATUS	(0x050e)
#define TXE_MMPLCP0	(0x0510)
#define TXE_MMPLCP1	(0x0512)
#define TXE_PHYCTL1	(0x0514)
#define TXE_PHYCTL2	(0x0516)
#define xmtfifodef	(0x0520)
#define XmtFifoFrameCnt	(0x0522)
#define xmtfifo_byte_cnt	(0x0524)
#define xmtfifo_head	(0x0526)
#define xmtfifo_rd_ptr	(0x0528)
#define xmtfifo_wr_ptr	(0x052a)
#define xmtfifodef1	(0x052c)
#define aggfifo_cmd	(0x052e)
#define aggfifo_stat	(0x0530)
#define aggfifo_cfgctl	(0x0532)
#define aggfifo_cfgdata	(0x0534)
#define aggfifo_mpdunum	(0x0536)
#define aggfifo_len	(0x0538)
#define aggfifo_bmp	(0x053a)
#define aggfifo_ackedcnt	(0x053c)
#define aggfifo_sel	(0x053e)
#define xmtfifocmd	(0x0540)
#define xmtfifoflush	(0x0542)
#define xmtfifothresh	(0x0544)
#define xmtfifordy	(0x0546)
#define xmtfifoprirdy	(0x0548)
#define xmtfiforqpri	(0x054a)
#define xmttplatetxptr	(0x054c)
#define xmttplateptr	(0x0550)
#define SCP_STRTPTR	(0x0552)	/* rename */
#define SCP_STOPPTR	(0x0554)	/* rename */
#define SCP_CURPTR	(0x0556)	/* rename */
#define aggfifo_data	(0x0558)
#define XmtTemplateDataLo	(0x0560)
#define XmtTemplateDataHi	(0x0562)
#define xmtsel	(0x0568)
#define xmttxcnt	(0x056a)
#define xmttxshmaddr	(0x056c)
#define xmt_ampdu_ctl	(0x0570)
#define TSF_CFP_STRT_L	(0x0604)
#define TSF_CFP_STRT_H	(0x0606)
#define TSF_CFP_PRE_TBTT	(0x0612)
#define TSF_CLK_FRAC_L	(0x062e)
#define TSF_CLK_FRAC_H	(0x0630)
#define TSF_RANDOM	(0x065a)
#define TSF_GPT_2_STAT	(0x0666)
#define TSF_GPT_2_CTR_L	(0x0668)
#define TSF_GPT_2_CTR_H	(0x066a)
#define TSF_GPT_2_VAL_L	(0x066c)
#define TSF_GPT_2_VAL_H	(0x066e)
#define TSF_GPT_ALL_STAT	(0x0670)
#define IFS_SIFS_RX_TX_TX	(0x0680)
#define IFS_SIFS_NAV_TX	(0x0682)
#define IFS_SLOT	(0x0684)
#define IFS_CTL	(0x0688)
#define IFS_BOFF_CTR	(0x068a)
#define IFS_STAT	(0x0690)
#define IFS_MEDBUSY_CTR	(0x0692)
#define IFS_TX_DUR	(0x0694)
#define IFS_STAT1	(0x0698)
#define IFS_AIFSN	(0x069c)
#define IFS_CTL1	(0x069e)
#define SLOW_CTL	(0x06a0)
#define SLOW_TIMER_L	(0x06a2)
#define SLOW_TIMER_H	(0x06a4)
#define SLOW_FRAC	(0x06a6)
#define FAST_PWRUP_DLY	(0x06a8)
#define SLOW_PER	(0x06aa)
#define SLOW_PER_FRAC	(0x06ac)
#define SLOW_CALTIMER_L	(0x06ae)
#define SLOW_CALTIMER_H	(0x06b0)
#define BTCX_CTL	(0x06b4)
#define BTCX_STAT	(0x06b6)
#define BTCX_TRANSCTL	(0x06b8)
#define BTCX_PRIORITYWIN	(0x06ba)	// old: BTCXPriorityWin
#define BTCX_TXCONFTIMER	(0x06bc)	// BTCXConfTimer
#define BTCX_PRISELTIMER	(0x06be)	// old: BTCXPriSelTimer
#define BTCX_PRV_RFACT_TIMER	(0x06c0)	// old: BTCXPrvRfActTimer
#define BTCX_CUR_RFACT_TIMER	(0x06c2)	// old: BTCXCurRfActTimer
#define BTCX_RFACT_DUR_TIMER	(0x06c4)	// old: BTCXActDurTimer
#define IFS_CTL_SEL_PRICRS	(0x06c6)
#define IfsCtlSelSecCrs	(0x06c8)
#define BTCX_ECI_ADDR	(0x06f0)	// old: BTCXEciAddr
#define BTCX_ECI_DATA	(0x06f2)	//old: BTCXEciData
#define BTCX_ECI_MASK_ADDR	(0x06f4)	// old: BTCXEciMaskAddr
#define BTCX_ECI_MASK_DATA	(0x06f6)	// old: BTCXEciMaskData
#define COEX_IO_MASK	(0x06f8)	// old: CoexIOMask
#define BTCX_ECI_EVENT_ADDR	(0x06fa)	// old: btcx_eci_event_addr
#define BTCX_ECI_EVENT_DATA	(0x06fc)	// old: btcx_eci_event_data
#define NAV_CTL	(0x0700)
#define NAV_STAT	(0x0702)
#define WEP_CTL	(0x07c0)
#define WEP_STAT	(0x07c2)
#define WEP_HDRLOC	(0x07c4)
#define WEP_PSDULEN	(0x07c6)
#define pcmctl	(0x07d0)
#define pcmstat	(0x07d2)
#define PMQ_CTL	(0x07e0)
#define PMQ_STATUS	(0x07e2)
#define PMQ_PAT_0	(0x07e4)
#define PMQ_PAT_1	(0x07e6)
#define PMQ_PAT_2	(0x07e8)
#define PMQ_DAT	(0x07ea)
#define PMQ_DAT_OR_MAT	(0x07ec)
#define PMQ_DAT_OR_ALL	(0x07ee)
#define CLK_GATE_REQ0	(0x0ae0)
#define CLK_GATE_REQ1	(0x0ae2)
#define CLK_GATE_REQ2	(0x0ae4)
#define CLK_GATE_UCODE_REQ0	(0x0ae6)
#define CLK_GATE_UCODE_REQ1	(0x0ae8)
#define CLK_GATE_UCODE_REQ2	(0x0aea)
#define CLK_GATE_STRETCH0	(0x0aec)
#define CLK_GATE_STRETCH1	(0x0aee)
#define CLK_GATE_MISC	(0x0af0)
#define CLK_GATE_DIV_CTRL	(0x0af2)
#define CLK_GATE_PHY_CLK_CTRL	(0x0af4)
#define CLK_GATE_STS	(0x0af6)
#define CLK_GATE_EXT_REQ0	(0x0af8)
#define CLK_GATE_EXT_REQ1	(0x0afa)
#define CLK_GATE_EXT_REQ2	(0x0afc)
#define CLK_GATE_UCODE_PHY_CLK_CTRL	(0x0afe)
#endif /* (AUTOREGS_COREREV < 40) */
#if (AUTOREGS_COREREV >= 40) && (AUTOREGS_COREREV < 64)
#define biststatus	(0x0000000c)	/* missing */
#define biststatus2	(0x00000010)	/* missing */
#define gptimer		(0x00000018)
#define usectimer	(0x0000001C)
#define intstat0	(0x00000020)
#define intmask0	(0x00000024)
#define intstat1	(0x00000028)
#define intmask1	(0x0000002C)
#define intstat2	(0x00000030)
#define intmask2	(0x00000034)
#define intstat3	(0x00000038)
#define intmask3	(0x0000003C)
#define intstat4	(0x00000040)
#define intmask4	(0x00000044)
#define intstat5	(0x00000048)
#define intmask5	(0x0000004C)
#define alt_intmask0	(0x00000060)
#define alt_intmask1	(0x00000064)
#define alt_intmask2	(0x00000068)
#define alt_intmask3	(0x0000006C)
#define alt_intmask4	(0x00000070)
#define alt_intmask5	(0x00000074)
#define inddma	(0x0080)	/* missing */
#define indaqm	(0x00a0)	/* missing */
#define indqsel	(0x00c0)	/* missing */
#define suspreq	(0x00c4)	/* missing */
#define flushreq	(0x00c8)	/* missing */
#define chnflushstatus	(0x00cc)	/* missing */
#define intrcvlzy0	(0x00000100)
#define intrcvlzy1	(0x00000104)
#define intrcvlzy2	(0x00000108)
#define intrcvlazy3	(0x0000010C)
#define MACCONTROL	(0x00000120)
#define MACCOMMAND	(0x00000124)
#define MACINTSTATUS	(0x00000128)
#define MACINTMASK	(0x0000012C)
#define XMT_TEMPLATE_RW_PTR	(0x00000130)
#define XMT_TEMPLATE_RW_DATA	(0x00000134)
#define StatCtr0	(0x00000138)
#define StatCtr1	(0x0000013C)
#define PMQHOSTDATA	(0x00000140)
#define PMQPATL	(0x00000144)
#define PMQPATH	(0x00000148)
#define STATSEL	(0x0000014C)
#define CHNSTATUS	(0x00000150)
#define PSM_DEBUG	(0x00000154)
#define PHY_DEBUG	(0x00000158)
#define MacCapability	(0x0000015C)
#define objaddr	(0x0160)	/* missing */
#define objdata	(0x0164)	/* missing */
#define ALT_MACINTSTATUS	(0x00000168)
#define ALT_MACINTMASK	(0x0000016C)
#define FrmTxStatus	(0x00000170)
#define FrmTxStatus2	(0x00000174)
#define FrmTxStatus3	(0x00000178)
#define FrmTxStatus4	(0x0000017C)
#define TSFTimerLow	(0x00000180)
#define TSFTimerHigh	(0x00000184)
#define CFPRep	(0x00000188)
#define CFPStart	(0x0000018C)
#define CFPMaxDur	(0x00000190)
#define AvbRxTimeStamp	(0x00000194)
#define AvbTxTimeStamp	(0x00000198)
#define MacControl1	(0x000001A0)
#define MacHWCap1	(0x000001A4)
#define GatedClkEn	(0x000001A8)
#define MacPatchCtrl	(0x000001AC)
#define ClockCtlStatus	(0x000001E0)
#define Workaround	(0x000001E4)
#define POWERCTL	(0x000001E8)
#define xmt0ctl	(0x00000200)
#define xmt0ptr	(0x00000204)
#define xmt0addrlow	(0x00000208)
#define xmt0addrhigh	(0x0000020C)
#define xmt0stat0	(0x00000210)
#define xmt0stat1	(0x00000214)
#define xmt0fifoctl	(0x00000218)
#define xmt0fifodata	(0x0000021C)
#define rcv0ctl	(0x00000220)
#define rcv0ptr	(0x00000224)
#define rcv0addrlow	(0x00000228)
#define rcv0addrhigh	(0x0000022C)
#define rcv0stat0	(0x00000230)
#define rcv0stat1	(0x00000234)
#define rcv0fifoctl	(0x00000238)
#define rcv0fifodata	(0x0000023C)
#define xmt1ctl	(0x00000240)
#define xmt1ptr	(0x00000244)
#define xmt1addrlow	(0x00000248)
#define xmt1addrhigh	(0x0000024C)
#define xmt1stat0	(0x00000250)
#define xmt1stat1	(0x00000254)
#define xmt1fifoctl	(0x00000258)
#define xmt1fifodata	(0x0000025C)
#define rcv1ctl	(0x00000260)
#define rcv1ptr	(0x00000264)
#define rcv1addrlow	(0x00000268)
#define rcv1addrhigh	(0x0000026C)
#define rcv1stat0	(0x00000270)
#define rcv1stat1	(0x00000274)
#define rcv1fifoctl	(0x00000278)
#define rcv1fifodata	(0x0000027C)
#define xmt2ctl	(0x00000280)
#define xmt2ptr	(0x00000284)
#define xmt2addrlow	(0x00000288)
#define xmt2addrhigh	(0x0000028C)
#define xmt2stat0	(0x00000290)
#define xmt2stat1	(0x00000294)
#define xmt2fifoctl	(0x00000298)
#define xmt2fifodata	(0x0000029C)
#define rcv2ctl	(0x000002A0)
#define rcv2ptr	(0x000002A4)
#define rcv2addrlow	(0x000002A8)
#define rcv2addrhigh	(0x000002AC)
#define rcv2stat0	(0x000002B0)
#define rcv2stat1	(0x000002B4)
#define xmt3ctl	(0x000002C0)
#define xmt3ptr	(0x000002C4)
#define xmt3addrlow	(0x000002C8)
#define xmt3addrhigh	(0x000002CC)
#define xmt3stat0	(0x000002D0)
#define xmt3stat1	(0x000002D4)
#define xmt3fifoctl	(0x000002D8)
#define xmt3fifodata	(0x000002DC)
#define xmt4ctl	(0x00000300)
#define xmt4ptr	(0x00000304)
#define xmt4addrlow	(0x00000308)
#define xmt4addrhigh	(0x0000030C)
#define xmt4stat0	(0x00000310)
#define xmt4stat1	(0x00000314)
#define xmt4fifoctl	(0x00000318)
#define xmt4fifodata	(0x0000031C)
#define xmt5ctl	(0x00000340)
#define xmt5ptr	(0x00000344)
#define xmt5addrlow	(0x00000348)
#define xmt5addrhigh	(0x0000034C)
#define xmt5stat0	(0x00000350)
#define xmt5stat1	(0x00000354)
#define xmt5fifoctl	(0x00000358)
#define xmt5fifodata	(0x0000035C)
#define fifobase	(0x00000380)
#define fifodatalow	(0x00000384)
#define fifodatahigh	(0x00000388)
#define DebugStoreMask	(0x00000398)
#define DebugTriggerMask	(0x0000039C)
#define DebugTriggerValue	(0x000003A0)
#define HostFCBSCmdPtr	(0x000003A4)
#define HostFCBSDataPtr	(0x000003A8)
#define HostCtrlStsReg	(0x000003AC)
#define radioregaddr_cross	(0x000003C8)
#define radioregdata_cross	(0x000003CA)
#define radioregaddr	(0x000003D8)
#define radioregdata	(0x000003DA)
#define rfdisabledly	(0x000003DC)
#define PHY_REG_0	(0x000003E0)
#define PHY_REG_1	(0x000003E2)
#define PHY_REG_2	(0x000003E4)
#define PHY_REG_3	(0x000003E6)
#define PHY_REG_4	(0x000003E8)
#define PHY_REG_5	(0x000003EA)
#define PHY_REG_7	(0x000003EC)
#define PHY_REG_6	(0x000003EE)
#define PHY_REG_8	(0x000003F0)
#define PHY_REG_9	(0x000003F2)
#define PHY_REG_A	(0x000003F4)
#define PHY_REG_B	(0x000003F6)
#define PHY_REG_C	(0x000003F8)
#define PHY_REG_D	(0x000003FA)
#define PHY_REG_ADDR	(0x000003FC)
#define PHY_REG_DATA	(0x000003FE)
#define SHM_BUF_BASE	(0x00000402)
#define SHM_BYT_CNT	(0x00000404)
#define RCV_FIFO_CTL	(0x00000406)
#define RCV_CTL	(0x00000408)
#define RCV_FRM_CNT	(0x0000040A)
#define RCV_STATUS_LEN	(0x0000040C)
#define RCV_SHM_STADDR	(0x0000040E)
#define RCV_SHM_STCNT	(0x00000410)
#define RXE_PHYRS_0	(0x00000412)
#define RXE_PHYRS_1	(0x00000414)
#define RXE_COND	(0x00000416)
#define RXE_RXCNT	(0x00000418)
#define RXE_STATUS1	(0x0000041A)
#define RXE_STATUS2	(0x0000041C)
#define RXE_PLCP_LEN	(0x0000041E)
#define RCV_FRM_CNT_Q0	(0x00000420)
#define RCV_FRM_CNT_Q1	(0x00000422)
#define RCV_WRD_CNT_Q0	(0x00000424)
#define RCV_WRD_CNT_Q1	(0x00000426)
#define RCV_RXFIFO_WRBURST	(0x00000428)
#define RCV_PHYFIFO_WRDCNT	(0x0000042A)
#define RCV_BM_STARTPTR_Q0	(0x0000042C)
#define RCV_BM_ENDPTR_Q0	(0x0000042E)
#define EXT_IHR_ADDR	(0x00000430)
#define EXT_IHR_DATA	(0x00000432)
#define RXE_PHYRS_2	(0x00000434)
#define RXE_PHYRS_3	(0x00000436)
#define PHY_MODE	(0x00000438)
#define RCV_BM_STARTPTR_Q1	(0x0000043A)
#define RCV_BM_ENDPTR_Q1	(0x0000043C)
#define RCV_COPYCNT_Q1	(0x0000043E)
#define RXE_PHYRS_ADDR	(0x00000440)
#define RXE_PHYRS_DATA	(0x00000442)
#define RXE_PHYRS_4	(0x00000444)
#define RXE_PHYRS_5	(0x00000446)
#define rxe_errval	(0x0448)	/* missing */
#define rxe_status3	(0x044c)	/* missing */
#define SHM_RXE_ADDR	(0x0000044E)
#define RcvAMPDUCtl0	(0x00000450)
#define RcvAMPDUCtl1	(0x00000452)
#define RcvCtl1	(0x00000454)
#define RcvLFIFOStatus	(0x00000456)
#define RcvAMPDUStatus	(0x00000458)
#define radioihrAddr	(0x00000460)
#define radioihrData	(0x00000462)
#define PSM_SLEEP_TMR	(0x00000480)
#define PSM_MAC_CTLH	(0x00000482)
#define PSM_MAC_INTSTAT_L	(0x00000484)
#define PSM_MAC_INTSTAT_H	(0x00000486)
#define PSM_MAC_INTMASK_L	(0x00000488)
#define PSM_MAC_INTMASK_H	(0x0000048A)
#define PSM_ERR_PC	(0x0000048C)
#define PSM_MACCOMMAND	(0x0000048E)
#define PSM_BRC_0	(0x00000490)
#define PSM_PHY_CTL	(0x00000492)
#define PSM_INTSEL_0	(0x00000494)
#define PSM_INTSEL_1	(0x00000496)
#define PSM_INTSEL_2	(0x00000498)
#define PSM_GPIOIN	(0x0000049A)
#define PSM_GPIOOUT	(0x0000049C)
#define PSM_GPIOEN	(0x0000049E)
#define PSM_BRED_0	(0x000004A0)
#define PSM_BRED_1	(0x000004A2)
#define PSM_BRED_2	(0x000004A4)
#define PSM_BRED_3	(0x000004A6)
#define PSM_BRCL_0	(0x000004A8)
#define PSM_BRCL_1	(0x000004AA)
#define PSM_BRCL_2	(0x000004AC)
#define PSM_BRCL_3	(0x000004AE)
#define PSM_BRPO_0	(0x000004B0)
#define PSM_BRPO_1	(0x000004B2)
#define PSM_BRPO_2	(0x000004B4)
#define PSM_BRPO_3	(0x000004B6)
#define PSM_BRWK_0	(0x000004B8)
#define PSM_BRWK_1	(0x000004BA)
#define PSM_BRWK_2	(0x000004BC)
#define PSM_BRWK_3	(0x000004BE)
#define PSMPowerReqStatus	(0x000004C8)
#define SubrStkStatus	(0x000004D0)
#define SubrStkRdPtr	(0x000004D2)
#define SubrStkRdData	(0x000004D4)
#define PSM_BRC_1	(0x000004D8)
#define PSM_MUL	(0x000004DA)
#define PSM_MACCONTROL1	(0x000004DC)
#define PSMSrchCtrlStatus	(0x000004E0)
#define PSMSrchBase	(0x000004E2)
#define PSMSrchLimit	(0x000004E4)
#define PSMSrchAddress	(0x000004E6)
#define PSMSrchData	(0x000004E8)
#define SBRegAddr	(0x000004EA)
#define SBRegDataL	(0x000004EC)
#define SBRegDataH	(0x000004EE)
#define PSMCoreCtlStat	(0x000004F0)
#define PSMWorkaround	(0x000004F2)
#define SbAddrLL	(0x000004F4)
#define SbAddrL	(0x000004F6)
#define SbAddrH	(0x000004F8)
#define SbAddrHH	(0x000004FA)
#define GpioOut	(0x000004FC)
#define PSM_TXMEM_PDA	(0x000004FE)
#define TXE_CTL	(0x00000500)
#define TXE_AUX	(0x00000502)
#define TXE_TS_LOC	(0x00000504)
#define TXE_TIME_OUT	(0x00000506)
#define TXE_WM_0	(0x00000508)
#define TXE_WM_1	(0x0000050A)
#define TXE_PHYCTL	(0x0000050C)
#define TXE_STATUS	(0x0000050E)
#define TXE_MMPLCP0	(0x00000510)
#define TXE_MMPLCP1	(0x00000512)
#define TXE_PHYCTL1	(0x00000514)
#define TXE_PHYCTL2	(0x00000516)
#define TXE_FRMSTAT_ADDR	(0x00000518)
#define TXE_FRMSTAT_DATA	(0x0000051A)
#define XmtFIFOFullThreshold	(0x00000520)
#define XmtFifoFrameCnt	(0x00000522)
#define BMCReadReq	(0x00000526)
#define BMCReadOffset	(0x00000528)
#define BMCReadLength	(0x0000052A)
#define BMCReadStatus	(0x0000052C)
#define XmtShmAddr	(0x0000052E)
#define PsmMSDUAccess	(0x00000530)
#define MSDUEntryBufCnt	(0x00000532)
#define MSDUEntryStartIdx	(0x00000534)
#define MSDUEntryEndIdx	(0x00000536)
#define SMP_PTR_H	(0x00000538)	/* rename */
#define SCP_CURPTR_H	(0x0000053A)	/* rename */
#define BMCCmd1	(0x0000053C)
#define BMCDynAllocStatus	(0x0000053E)
#define BMCCTL	(0x00000540)
#define BMCConfig	(0x00000542)
#define BMCStartAddr	(0x00000544)
#define BMCSize	(0x0546)	/* missing */
#define BMCCmd	(0x00000548)
#define BMCMaxBuffers	(0x0000054A)
#define BMCMinBuffers	(0x0000054C)
#define BMCAllocCtl	(0x0000054E)
#define BMCDescrLen	(0x00000550)
#define SCP_STRTPTR	(0x0552)	/* rename */
#define SCP_STOPPTR	(0x0554)	/* rename */
#define SCP_CURPTR	(0x0556)	/* rename */
#define SaveRestoreStartPtr	(0x0558)	/* missing */
#define SPP_STRTPTR	(0x0000055A)	/* rename */
#define SPP_STOPPTR	(0x0000055C)	/* rename */
#define XmtDMABusy	(0x0000055E)
#define XmtTemplateDataLo	(0x00000560)
#define XmtTemplateDataHi	(0x00000562)
#define XmtTemplatePtr	(0x00000564)
#define XmtSuspFlush	(0x00000566)
#define XmtFifoRqPrio	(0x00000568)
#define BMCStatCtl	(0x0000056A)
#define BMCStatData	(0x0000056C)
#define BMCMSDUFifoStat	(0x0000056E)
#define XMT_AMPDU_CTL	(0x00000570)
#define XMT_AMPDU_LEN	(0x00000572)
#define XMT_AMPDU_CRC	(0x00000574)
#define TXE_CTL1	(0x00000576)
#define TXE_STATUS1	(0x00000578)
#define TXAMPDUDelim	(0x0000057E)
#define TXE_BM_0	(0x00000580)
#define TXE_BM_1	(0x00000582)
#define TXE_BM_2	(0x00000584)
#define TXE_BM_3	(0x00000586)
#define TXE_BM_4	(0x00000588)
#define TXE_BM_5	(0x0000058A)
#define TXE_BM_6	(0x0000058C)
#define TXE_BM_7	(0x0000058E)
#define TXE_BM_8	(0x00000590)
#define TXE_BM_9	(0x00000592)
#define TXE_BM_10	(0x00000594)
#define TXE_BM_11	(0x00000596)
#define TXE_BM_12	(0x00000598)
#define TXE_BM_13	(0x0000059A)
#define TXE_BM_14	(0x0000059C)
#define TXE_BM_15	(0x0000059E)
#define TXE_BM_16	(0x000005A0)
#define TXE_BM_17	(0x000005A2)
#define TXE_BM_18	(0x000005A4)
#define TXE_BM_19	(0x000005A6)
#define TXE_BM_20	(0x000005A8)
#define TXE_BM_21	(0x000005AA)
#define TXE_BM_22	(0x000005AC)
#define TXE_BM_23	(0x000005AE)
#define TXE_BM_24	(0x000005B0)
#define TXE_BM_25	(0x000005B2)
#define TXE_BM_26	(0x000005B4)
#define TXE_BM_27	(0x000005B6)
#define TXE_BM_28	(0x000005B8)
#define TXE_BM_29	(0x000005BA)
#define TXE_BM_30	(0x000005BC)
#define TXE_BM_31	(0x000005BE)
#define TXE_BV_0	(0x000005C0)
#define TXE_BV_1	(0x000005C2)
#define TXE_BV_2	(0x000005C4)
#define TXE_BV_3	(0x000005C6)
#define TXE_BV_4	(0x000005C8)
#define TXE_BV_5	(0x000005CA)
#define TXE_BV_6	(0x000005CC)
#define TXE_BV_7	(0x000005CE)
#define TXE_BV_8	(0x000005D0)
#define TXE_BV_9	(0x000005D2)
#define TXE_BV_10	(0x000005D4)
#define TXE_BV_11	(0x000005D6)
#define TXE_BV_12	(0x000005D8)
#define TXE_BV_13	(0x000005DA)
#define TXE_BV_14	(0x000005DC)
#define TXE_BV_15	(0x000005DE)
#define TXE_BV_16	(0x000005E0)
#define TXE_BV_17	(0x000005E2)
#define TXE_BV_18	(0x000005E4)
#define TXE_BV_19	(0x000005E6)
#define TXE_BV_20	(0x000005E8)
#define TXE_BV_21	(0x000005EA)
#define TXE_BV_22	(0x000005EC)
#define TXE_BV_23	(0x000005EE)
#define TXE_BV_24	(0x000005F0)
#define TXE_BV_25	(0x000005F2)
#define TXE_BV_26	(0x000005F4)
#define TXE_BV_27	(0x000005F6)
#define TXE_BV_28	(0x000005F8)
#define TXE_BV_29	(0x000005FA)
#define TXE_BV_30	(0x000005FC)
#define TXE_BV_31	(0x000005FE)
#define TSF_CTL	(0x00000600)
#define TSF_STAT	(0x00000602)
#define TSF_CFP_STRT_L	(0x00000604)
#define TSF_CFP_STRT_H	(0x00000606)
#define TSF_CFP_END_L	(0x00000608)
#define TSF_CFP_END_H	(0x0000060A)
#define TSF_CFP_MAX_DUR	(0x0000060C)
#define TSF_CFP_REP_L	(0x0000060E)
#define TSF_CFP_REP_H	(0x00000610)
#define TSF_CFP_PRE_TBTT	(0x00000612)
#define TSF_CFP_CFP_D0_L	(0x00000614)
#define TSF_CFP_CFP_D0_H	(0x00000616)
#define TSF_CFP_CFP_D1_L	(0x00000618)
#define TSF_CFP_CFP_D1_H	(0x0000061A)
#define TSF_CFP_CFP_D2_L	(0x0000061C)
#define TSF_CFP_CFP_D2_H	(0x0000061E)
#define TSF_CFP_TXOP_SQS_L	(0x00000620)
#define TSF_CFP_TXOP_SQS_H	(0x00000622)
#define TSF_CFP_TXOP_PQS	(0x00000624)
#define TSF_CFP_TXOP_SQD_L	(0x00000626)
#define TSF_CFP_TXOP_SQD_H	(0x00000628)
#define TSF_CFP_TXOP_PQD	(0x0000062A)
#define TSF_FES_DUR	(0x0000062C)
#define TSF_CLK_FRAC_L	(0x0000062E)
#define TSF_CLK_FRAC_H	(0x00000630)
#define TSF_TMR_TSF_L	(0x00000632)
#define TSF_TMR_TSF_ML	(0x00000634)
#define TSF_TMR_TSF_MU	(0x00000636)
#define TSF_TMR_TSF_H	(0x00000638)
#define TSF_TMR_TX_OFFSET	(0x0000063A)
#define TSF_TMR_RX_OFFSET	(0x0000063C)
#define TSF_TMR_RX_TS	(0x0000063E)
#define TSF_TMR_TX_TS	(0x00000640)
#define TSF_TMR_RX_END_TS	(0x00000642)
#define TSF_TMR_DELTA	(0x00000644)
#define TSF_GPT_0_STAT	(0x00000646)
#define TSF_GPT_1_STAT	(0x00000648)
#define TSF_GPT_0_CTR_L	(0x0000064A)
#define TSF_GPT_1_CTR_L	(0x0000064C)
#define TSF_GPT_0_CTR_H	(0x0000064E)
#define TSF_GPT_1_CTR_H	(0x00000650)
#define TSF_GPT_0_VAL_L	(0x00000652)
#define TSF_GPT_1_VAL_L	(0x00000654)
#define TSF_GPT_0_VAL_H	(0x00000656)
#define TSF_GPT_1_VAL_H	(0x00000658)
#define TSF_RANDOM	(0x0000065A)
#define RAND_SEED_0	(0x0000065C)
#define RAND_SEED_1	(0x0000065E)
#define RAND_SEED_2	(0x00000660)
#define TSF_ADJUST	(0x00000662)
#define TSF_PHY_HDR_TM	(0x00000664)
#define TSF_GPT_2_STAT	(0x00000666)
#define TSF_GPT_2_CTR_L	(0x00000668)
#define TSF_GPT_2_CTR_H	(0x0000066A)
#define TSF_GPT_2_VAL_L	(0x0000066C)
#define TSF_GPT_2_VAL_H	(0x0000066E)
#define TSF_GPT_ALL_STAT	(0x00000670)
#define TSF_ADJ_CTL	(0x00000672)
#define TSF_ADJ_PORTAL	(0x00000674)
#define IFS_SIFS_RX_TX_TX	(0x00000680)
#define IFS_SIFS_NAV_TX	(0x00000682)
#define IFS_SLOT	(0x00000684)
#define IFS_EIFS	(0x00000686)
#define IFS_CTL	(0x00000688)
#define IFS_BOFF_CTR	(0x0000068A)
#define IFS_SLOT_CTR	(0x0000068C)
#define IFS_FREE_SLOTS	(0x0000068E)
#define IFS_STAT	(0x00000690)
#define IFS_MEDBUSY_CTR	(0x00000692)
#define IFS_TX_DUR	(0x00000694)
#define IFS_RIFS_TIME	(0x00000696)
#define IFS_STAT1	(0x00000698)
#define IFS_EDCAPRI	(0x0000069A)
#define IFS_AIFSN	(0x0000069C)
#define IFS_CTL1	(0x0000069E)
#define SLOW_CTL	(0x000006A0)
#define SLOW_TIMER_L	(0x000006A2)
#define SLOW_TIMER_H	(0x000006A4)
#define SLOW_FRAC	(0x000006A6)
#define FAST_PWRUP_DLY	(0x000006A8)
#define SLOW_PER	(0x000006AA)
#define SLOW_PER_FRAC	(0x000006AC)
#define SLOW_CALTIMER_L	(0x000006AE)
#define SLOW_CALTIMER_H	(0x000006B0)
#define IFS_STAT2	(0x000006B2)
#define BTCX_CTL	(0x000006B4)
#define BTCX_STAT	(0x000006B6)
#define BTCX_TRANSCTL	(0x000006B8)
#define BTCX_PRIORITYWIN	(0x000006BA)	// BTCXPriorityWin
#define BTCX_TXCONFTIMER	(0x06bc)	// BTCXConfTimer
#define BTCX_PRISELTIMER	(0x06be)	// old: BTCXPriSelTimer
#define BTCX_PRV_RFACT_TIMER	(0x06c0)	// old: BTCXPrvRfActTimer
#define BTCX_CUR_RFACT_TIMER	(0x06c2)	// old: BTCXCurRfActTimer
#define BTCX_RFACT_DUR_TIMER	(0x06c4)	// old: BTCXActDurTimer
#define IFS_CTL_SEL_PRICRS	(0x000006C6)
#define IfsCtlSelSecCrs	(0x000006C8)
#define IfStatEdCrs160M	(0x000006CA)
#define CRSEDCntrCtrl	(0x000006CC)
#define CRSEDCntrAddr	(0x000006CE)
#define CRSEDCntrData	(0x000006D0)
#define EXT_STAT_EDCRS160M	(0x000006D2)
#define ERCXControl	(0x000006D4)
#define ERCXStatus	(0x000006D6)
#define ERCXTransCtl	(0x000006D8)
#define ERCXPriorityWin	(0x000006DA)
#define ERCXConfTimer	(0x000006DC)
#define ERCX_PRISELTIMER	(0x000006DE)
#define ERCXPrvRfActTimer	(0x000006E0)
#define ERCXCurRfActTimer	(0x000006E2)
#define ERCXActDurTimer	(0x000006E4)
#define BTCX_ECI_ADDR	(0x06f0)	// old: BTCXEciAddr
#define BTCX_ECI_DATA	(0x06f2)	//old: BTCXEciData
#define BTCX_ECI_MASK_ADDR	(0x06f4)	// old: BTCXEciMaskAddr
#define BTCX_ECI_MASK_DATA	(0x06f6)	// old: BTCXEciMaskData
#define COEX_IO_MASK	(0x06f8)	// old: CoexIOMask
#define BTCX_ECI_EVENT_ADDR	(0x06fa)	// old: btcx_eci_event_addr
#define BTCX_ECI_EVENT_DATA	(0x06fc)	// old: btcx_eci_event_data
#define NAV_CTL		(0x00000700)
#define NAV_STAT	(0x00000702)
#define NAV_CNTR_L	(0x00000704)
#define NAV_CNTR_H	(0x00000706)
#define NAV_TBTT_NOW_L	(0x00000708)
#define WEP_CTL		(0x000007C0)
#define WEP_STAT	(0x000007C2)
#define WEP_HDRLOC	(0x000007C4)
#define WEP_PSDULEN	(0x000007C6)
#define WEP_KEY_ADDR	(0x000007C8)
#define WEP_KEY_DATA	(0x000007CA)
#define WEP_REG_ADDR	(0x000007CC)
#define WEP_REG_DATA	(0x000007CE)
#define PMQ_CTL		(0x000007E0)
#define PMQ_STATUS	(0x000007E2)
#define PMQ_PAT_0	(0x000007E4)
#define PMQ_PAT_1	(0x000007E6)
#define PMQ_PAT_2	(0x000007E8)
#define PMQ_DAT		(0x000007EA)
#define PMQ_DAT_OR_MAT	(0x000007EC)
#define PMQ_DAT_OR_ALL	(0x000007EE)
#define pmqdataor_mat1	(0x07f0)	/* missing */
#define pmqdataor_mat2	(0x07f2)	/* missing */
#define pmqdataor_mat3	(0x07f4)	/* missing */
#define pmq_auxsts	(0x07f6)	/* missing */
#define pmq_ctl1	(0x07f8)	/* missing */
#define pmq_status1	(0x07fa)	/* missing */
#define pmq_addthr	(0x07fc)	/* missing */
#define AQMConfig	(0x00000800)
#define AQMFifoDef	(0x00000802)
#define AQMMaxIdx	(0x00000804)
#define AQMRcvdBA0	(0x00000806)
#define AQMRcvdBA1	(0x00000808)
#define AQMRcvdBA2	(0x0000080A)
#define AQMRcvdBA3	(0x0000080C)
#define AQMBaSSN	(0x0000080E)
#define AQMRefSN	(0x00000810)
#define AQMMaxAggLenLow	(0x00000812)
#define AQMMaxAggLenHi	(0x00000814)
#define AQMAggParams	(0x00000816)
#define AQMMinMpduLen	(0x00000818)
#define AQMMacAdjLen	(0x0000081A)
#define DebugBusCtrl	(0x0000081C)
#define MinConsCnt	(0x0000081E)
#define AQMAggStats	(0x00000820)
#define AQMAggLenLow	(0x00000822)
#define AQMAggLenHi	(0x00000824)
#define AQMIdx	(0x00000826)
#define AQMMpduLen	(0x00000828)
#define AQMTxCnt	(0x0000082A)
#define AQMUpdBA0	(0x0000082C)
#define AQMUpdBA1	(0x0000082E)
#define AQMUpdBA2	(0x00000830)
#define AQMUpdBA3	(0x00000832)
#define AQMAckCnt	(0x00000834)
#define AQMConsCnt	(0x00000836)
#define AQMFifoReady	(0x00000838)
#define AQMStartLoc	(0x0000083A)
#define AQMAggRptr	(0x0000083C)
#define AQMTxcntRptr	(0x0000083E)
#define TDCCTL	(0x00000840)
#define TDC_Plcp0	(0x00000842)
#define TDC_Plcp1	(0x00000844)
#define TDC_FrmLen0	(0x00000846)
#define TDC_FrmLen1	(0x00000848)
#define TDC_Txtime	(0x0000084A)
#define TDC_VhtSigB0	(0x0000084C)
#define TDC_VhtSigB1	(0x0000084E)
#define TDC_LSigLen	(0x00000850)
#define TDC_NSym0	(0x00000852)
#define TDC_NSym1	(0x00000854)
#define TDC_VhtPsduLen0	(0x00000856)
#define TDC_VhtPsduLen1	(0x00000858)
#define TDC_VhtMacPad	(0x0000085A)
#define AQMCurTxcnt	(0x0000085E)
#define ShmDma_Ctl	(0x00000860)
#define ShmDma_TxdcAddr	(0x00000862)
#define ShmDma_ShmAddr	(0x00000864)
#define ShmDma_XferCnt	(0x00000866)
#define Txdc_Addr	(0x00000868)
#define Txdc_Data	(0x0000086A)
#define TXE_VASIP_INTSTS	(0x00000870)
#define MHP_Status	(0x00000880)
#define MHP_FC	(0x00000882)
#define MHP_DUR	(0x00000884)
#define MHP_SC	(0x00000886)
#define MHP_QOS	(0x00000888)
#define MHP_HTC_H	(0x0000088A)
#define MHP_HTC_L	(0x0000088C)
#define MHP_Addr1_H	(0x0000088E)
#define MHP_Addr1_M	(0x00000890)
#define MHP_Addr1_L	(0x00000892)
#define MHP_Addr2_H	(0x000008A0)
#define MHP_Addr2_M	(0x000008A2)
#define MHP_Addr2_L	(0x000008A4)
#define MHP_Addr3_H	(0x000008A6)
#define MHP_Addr3_M	(0x000008A8)
#define MHP_Addr3_L	(0x000008AA)
#define MHP_Addr4_H	(0x000008AC)
#define MHP_Addr4_M	(0x000008AE)
#define MHP_Addr4_L	(0x000008B0)
#define MHP_CFC	(0x000008B2)
#define DAGG_CTL2	(0x000008C0)
#define DAGG_BYTESLEFT	(0x000008C2)
#define DAGG_SH_OFFSET	(0x000008C4)
#define DAGG_STAT	(0x000008C6)
#define DAGG_LEN	(0x000008C8)
#define TXBA_CTL	(0x000008CA)
#define TXBA_DataSel	(0x000008CC)
#define TXBA_Data	(0x000008CE)
#define DAGG_LEN_THR	(0x000008D0)
#define AMT_CTL	(0x000008E0)
#define AMT_Status	(0x000008E2)
#define AMT_Limit	(0x000008E4)
#define AMT_Attr	(0x000008E6)
#define AMT_Match1	(0x000008E8)
#define AMT_Match2	(0x000008EA)
#define AMT_Table_Addr	(0x000008EC)
#define AMT_Table_Data	(0x000008EE)
#define AMT_Table_Val	(0x000008F0)
#define AMT_DBG_SEL	(0x000008F2)
#define RoeCtrl	(0x00000900)
#define RoeStatus	(0x00000902)
#define RoeIPChkSum	(0x00000904)
#define RoeTCPUDPChkSum	(0x00000906)
#define RoeStatus1	(0x00000908)
#define PSOCtl	(0x00000920)
#define PSORxWordsWatermark	(0x00000922)
#define PSORxCntWatermark	(0x00000924)
#define PSOCurRxFramePtrs	(0x00000926)
#define OBFFCtl	(0x00000930)
#define OBFFRxWordsWatermark	(0x00000932)
#define OBFFRxCntWatermark	(0x00000934)
#define PSOOBFFStatus	(0x00000936)
#define LtrRxTimer	(0x00000938)
#define LtrRxWordsWatermark	(0x0000093A)
#define LtrRxCntWatermark	(0x0000093C)
#define RcvHdrConvCtrlSts	(0x0000093E)
#define RcvHdrConvSts	(0x00000940)
#define RcvHdrConvSts1	(0x00000942)
#define RCVLB_DAGG_CTL	(0x00000944)
#define RcvFifo0Len	(0x00000946)
#define RcvFifo1Len	(0x00000948)
#define CRSStatus	(0x00000960)
#define OtherMac_HWStatus_Lo	(0x00000962)
#define OtherMac_HWStatus_Hi	(0x00000964)
#define phyOOBSts	(0x00000966)
#define phyoobAddr	(0x00000968)
#define phyoobData	(0x0000096A)
#define ToECTL	(0x00000A00)
#define ToERst	(0x00000A02)
#define ToECSumNZ	(0x00000A04)
#define ToEChannelState	(0x00000A06)
#define TxSerialCtl	(0x00000A40)
#define TxPlcpLSig0	(0x00000A42)
#define TxPlcpLSig1	(0x00000A44)
#define TxPlcpHtSig0	(0x00000A46)
#define TxPlcpHtSig1	(0x00000A48)
#define TxPlcpHtSig2	(0x00000A4A)
#define TxPlcpVhtSigB0	(0x00000A4C)
#define TxPlcpVhtSigB1	(0x00000A4E)
#define MacHdrFromShmLen	(0x00000A52)
#define TxPlcpLen	(0x00000A54)
#define TxBFRptLen	(0x00000A58)
#define BytCntInTxFrmLo	(0x00000A5A)
#define BytCntInTxFrmHi	(0x00000A5C)
#define TXBFCtl	(0x00000A60)
#define BfmRptOffset	(0x00000A62)
#define BfmRptLen	(0x00000A64)
#define TXBFBfeRptRdCnt	(0x00000A66)
#define PhyDebugL	(0x00000A68)
#define PhyDebugH	(0x00000A6A)
#define PSM_ALT_MAC_INTSTATUS_L	(0x00000A84)
#define PSM_ALT_MAC_INTSTATUS_H	(0x00000A86)
#define PSM_ALT_MAC_INTMASK_L	(0x00000A88)
#define PSM_ALT_MAC_INTMASK_H	(0x00000A8A)
#define PsmMboxAddr	(0x00000A8C)
#define PsmMboxData	(0x00000A8E)
#define PsmMboxOutSts	(0x00000A90)
#define PsmMboxEvent	(0x00000A92)
#define PSM_BASE_0	(0x00000AA0)
#define PSM_BASE_1	(0x00000AA2)
#define PSM_BASE_2	(0x00000AA4)
#define PSM_BASE_3	(0x00000AA6)
#define PSM_BASE_4	(0x00000AA8)
#define PSM_BASE_5	(0x00000AAA)
#define PSM_BASE_6	(0x00000AAC)
#define PSM_BASE_7	(0x00000AAE)
#define PSM_BASE_8	(0x00000AB0)
#define PSM_BASE_9	(0x00000AB2)
#define PSM_BASE_10	(0x00000AB4)
#define PSM_BASE_11	(0x00000AB6)
#define PSM_BASE_12	(0x00000AB8)
#define PSM_BASE_13	(0x00000ABA)
#define StrtCmdPtrIHR	(0x00000AC0)
#define StrtDataPtrIHR	(0x00000AC2)
#define UcodeCtrlStsReg	(0x00000AC4)
#define FastExtIHRAddr	(0x00000AC6)
#define FastExtIHRData	(0x00000AC8)
#define FastRadioIHRAddr	(0x00000ACA)
#define FastRadioIHRData	(0x00000ACC)
#define UcodeFcbsRunTimeCnt	(0x00000ACE)
#define MAC_MEM_INFO_0	(0x00000AD0)
#define MAC_MEM_INFO_1	(0x00000AD2)
#define MAC_BANKX_INDEX	(0x00000AD4)
#define MAC_BANKX_SIZE	(0x00000AD6)
#define MAC_BANKX_INFO	(0x00000AD8)
#define MAC_BANKX_CTRL	(0x00000ADA)
#define MAC_BANKX_ACTIVE_PDA	(0x00000ADC)
#define MAC_BANKX_SLEEP_PDA	(0x00000ADE)
#define CLK_GATE_REQ0	(0x00000AE0)
#define CLK_GATE_REQ1	(0x00000AE2)
#define CLK_GATE_REQ2	(0x00000AE4)
#define CLK_GATE_UCODE_REQ0	(0x00000AE6)
#define CLK_GATE_UCODE_REQ1	(0x00000AE8)
#define CLK_GATE_UCODE_REQ2	(0x00000AEA)
#define CLK_GATE_STRETCH0	(0x00000AEC)
#define CLK_GATE_STRETCH1	(0x00000AEE)
#define CLK_GATE_MISC	(0x00000AF0)
#define CLK_GATE_DIV_CTRL	(0x00000AF2)
#define CLK_GATE_PHY_CLK_CTRL	(0x00000AF4)
#define CLK_GATE_STS	(0x00000AF6)
#define CLK_GATE_EXT_REQ0	(0x00000AF8)
#define CLK_GATE_EXT_REQ1	(0x00000AFA)
#define CLK_GATE_EXT_REQ2	(0x00000AFC)
#define CLK_GATE_UCODE_PHY_CLK_CTRL	(0x00000AFE)
#define RXMapFifoSize	(0x00000B00)
#define RXMapStatus	(0x00000B02)
#define MsduThreshold	(0x00000B04)
#define DebugTxFlowCtl	(0x00000B06)
#define XmtDPQSuspFlush	(0x00000B08)
#define MSDUIdxFifoConfig	(0x00000B0A)
#define MSDUIdxFifoDef	(0x00000B0C)
#define BMCCore0TXAllMaxBuffers	(0x00000B0E)
#define BMCCore1TXAllMaxBuffers	(0x00000B10)
#define BMCDynAllocStatus1	(0x00000B12)
#define DMAMaxOutStBuffers	(0x00000B14)
#define SCS_MASK_L		(0x00000B16)	/* rename */
#define SCS_MASK_H		(0x00000B18)	/* rename */
#define SCM_MASK_L		(0x00000B1A)	/* rename */
#define SCM_MASK_H		(0x00000B1C)	/* rename */
#define SCM_VAL_L		(0x00000B1E)	/* rename */
#define SCM_VAL_H		(0x00000B20)	/* rename */
#define SCT_MASK_L		(0x00000B22)	/* rename */
#define SCT_MASK_H		(0x00000B24)	/* rename */
#define SCT_VAL_L		(0x00000B26)	/* rename */
#define SCT_VAL_H		(0x00000B28)	/* rename */
#define SCX_MASK_L		(0x00000B2A)	/* rename */
#define SCX_MASK_H		(0x00000B2C)	/* rename */
#define SMP_CTRL		(0x00000B2E)	/* rename */
#define Core0BMCAllocStatusTID7	(0x00000B30)
#define Core1BMCAllocStatusTID7	(0x00000B32)
#define MsduFifoReachThreshold	(0x00000B34)
#define BMVpConfig	(0x00000B36)
#define TXE_DBG_BMC_STATUS	(0x00000B38)
#define XmtTemplatePtrOffset	(0x00000B3A)
#define DebugLLMConfig	(0x00000B3C)
#define DebugLLMStatus	(0x00000B3E)

#endif /* (AUTOREGS_COREREV >= 40) && (AUTOREGS_COREREV < 64) */
#if (AUTOREGS_COREREV >= 64) && (AUTOREGS_COREREV < 129)
#define biststatus	(0x0000000c)	/* missing */
#define syncSlowTimeStamp	(0x00000010)
#define syncFastTimeStamp	(0x00000014)
#define gptimer	(0x00000018)
#define usectimer	(0x0000001C)
#define intstat0	(0x00000020)
#define intmask0	(0x00000024)
#define intstat1	(0x00000028)
#define intmask1	(0x0000002C)
#define intstat2	(0x00000030)
#define intmask2	(0x00000034)
#define intstat3	(0x00000038)
#define intmask3	(0x0000003C)
#define intstat4	(0x00000040)
#define intmask4	(0x00000044)
#define intstat5	(0x00000048)
#define intmask5	(0x0000004C)
#define alt_intmask0	(0x00000060)
#define alt_intmask1	(0x00000064)
#define alt_intmask2	(0x00000068)
#define alt_intmask3	(0x0000006C)
#define alt_intmask4	(0x00000070)
#define alt_intmask5	(0x00000074)
#define inddma		(0x00000080)
#define IndXmtPtr	(0x00000084)
#define IndXmtAddrLow	(0x00000088)
#define IndXmtAddrHigh	(0x0000008C)
#define IndXmtStat0	(0x00000090)
#define IndXmtStat1	(0x00000094)
#define indaqm		(0x000000A0)
#define IndAQMptr	(0x000000A4)
#define IndAQMaddrlow	(0x000000A8)
#define IndAQMaddrhigh	(0x000000AC)
#define IndAQMstat0	(0x000000B0)
#define IndAQMstat1	(0x000000B4)
#define IndAQMIntStat	(0x000000B8)
#define IndAQMIntMask	(0x000000BC)
#define IndAQMQSel	(0x000000C0)
#define SUSPREQ	(0x000000C4)
#define FLUSHREQ	(0x000000C8)
#define CHNFLUSH_STATUS	(0x000000CC)
#define CHNSUSP_STATUS	(0x000000D0)
#define intrcvlzy0	(0x00000100)
#define intrcvlzy1	(0x00000104)
#define intrcvlzy2	(0x00000108)
#define intrcvlazy3	(0x0000010C)
#define MACCONTROL	(0x00000120)
#define MACCOMMAND	(0x00000124)
#define MACINTSTATUS	(0x00000128)
#define MACINTMASK	(0x0000012C)
#define XMT_TEMPLATE_RW_PTR	(0x00000130)
#define XMT_TEMPLATE_RW_DATA	(0x00000134)
#define PMQHOSTDATA	(0x00000140)
#define PMQPATL	(0x00000144)
#define PMQPATH	(0x00000148)
#define CHNSTATUS	(0x00000150)
#define PSM_DEBUG	(0x00000154)
#define PHY_DEBUG	(0x00000158)
#define MacCapability	(0x0000015C)
#define objaddr	(0x0160)	/* missing */
#define objdata	(0x0164)	/* missing */
#define ALT_MACINTSTATUS	(0x00000168)
#define ALT_MACINTMASK	(0x0000016C)
#define FrmTxStatus	(0x00000170)
#define FrmTxStatus2	(0x00000174)
#define FrmTxStatus3	(0x00000178)
#define FrmTxStatus4	(0x0000017C)
#define TSFTimerLow	(0x00000180)
#define TSFTimerHigh	(0x00000184)
#define CFPRep	(0x00000188)
#define CFPStart	(0x0000018C)
#define CFPMaxDur	(0x00000190)
#define AvbRxTimeStamp	(0x00000194)
#define AvbTxTimeStamp	(0x00000198)
#define MacControl1	(0x000001A0)
#define MacHWCap1	(0x000001A4)
#define GatedClkEn	(0x000001A8)
#define gptimer_psmx	(0x000001B0)
#define MACCONTROL_psmx	(0x000001B4)
#define MacControl1_psmx	(0x000001B8)
#define MACCOMMAND_psmx	(0x000001BC)
#define MACINTSTATUS_psmx	(0x000001C0)
#define MACINTMASK_psmx	(0x000001C4)
#define ALT_MACINTSTATUS_psmx	(0x000001C8)
#define ALT_MACINTMASK_psmx	(0x000001CC)
#define PSM_DEBUG_psmx	(0x000001D0)
#define PHY_DEBUG_psmx	(0x000001D4)
#define statctr2	(0x01d8)	/* missing */
#define statctr3	(0x01dc)	/* missing */
#define ClockCtlStatus	(0x000001E0)
#define Workaround	(0x000001E4)
#define POWERCTL	(0x000001E8)
#define xmt0ctl	(0x00000200)
#define xmt0ptr	(0x00000204)
#define xmt0addrlow	(0x00000208)
#define xmt0addrhigh	(0x0000020C)
#define xmt0stat0	(0x00000210)
#define xmt0stat1	(0x00000214)
#define xmt0fifoctl	(0x00000218)
#define xmt0fifodata	(0x0000021C)
#define rcv0ctl	(0x00000220)
#define rcv0ptr	(0x00000224)
#define rcv0addrlow	(0x00000228)
#define rcv0addrhigh	(0x0000022C)
#define rcv0stat0	(0x00000230)
#define rcv0stat1	(0x00000234)
#define rcv0fifoctl	(0x00000238)
#define rcv0fifodata	(0x0000023C)
#define xmt1ctl	(0x00000240)
#define xmt1ptr	(0x00000244)
#define xmt1addrlow	(0x00000248)
#define xmt1addrhigh	(0x0000024C)
#define xmt1stat0	(0x00000250)
#define xmt1stat1	(0x00000254)
#define xmt1fifoctl	(0x00000258)
#define xmt1fifodata	(0x0000025C)
#define rcv1ctl	(0x00000260)
#define rcv1ptr	(0x00000264)
#define rcv1addrlow	(0x00000268)
#define rcv1addrhigh	(0x0000026C)
#define rcv1stat0	(0x00000270)
#define rcv1stat1	(0x00000274)
#define rcv1fifoctl	(0x00000278)
#define rcv1fifodata	(0x0000027C)
#define xmt2ctl	(0x00000280)
#define xmt2ptr	(0x00000284)
#define xmt2addrlow	(0x00000288)
#define xmt2addrhigh	(0x0000028C)
#define xmt2stat0	(0x00000290)
#define xmt2stat1	(0x00000294)
#define xmt2fifoctl	(0x00000298)
#define xmt2fifodata	(0x0000029C)
#define rcv2ctl	(0x000002A0)
#define rcv2ptr	(0x000002A4)
#define rcv2addrlow	(0x000002A8)
#define rcv2addrhigh	(0x000002AC)
#define rcv2stat0	(0x000002B0)
#define rcv2stat1	(0x000002B4)
#define xmt3ctl	(0x000002C0)
#define xmt3ptr	(0x000002C4)
#define xmt3addrlow	(0x000002C8)
#define xmt3addrhigh	(0x000002CC)
#define xmt3stat0	(0x000002D0)
#define xmt3stat1	(0x000002D4)
#define xmt3fifoctl	(0x000002D8)
#define xmt3fifodata	(0x000002DC)
#define xmt4ctl	(0x00000300)
#define xmt4ptr	(0x00000304)
#define xmt4addrlow	(0x00000308)
#define xmt4addrhigh	(0x0000030C)
#define xmt4stat0	(0x00000310)
#define xmt4stat1	(0x00000314)
#define xmt4fifoctl	(0x00000318)
#define xmt4fifodata	(0x0000031C)
#define xmt5ctl	(0x00000340)
#define xmt5ptr	(0x00000344)
#define xmt5addrlow	(0x00000348)
#define xmt5addrhigh	(0x0000034C)
#define xmt5stat0	(0x00000350)
#define xmt5stat1	(0x00000354)
#define xmt5fifoctl	(0x00000358)
#define xmt5fifodata	(0x0000035C)
#define fifobase	(0x00000380)
#define fifodatalow	(0x00000384)
#define fifodatahigh	(0x00000388)
#define aggfifocnt	(0x0390)	/* missing */
#define aggfifodata	(0x0394)	/* missing */
#define DebugStoreMask	(0x00000398)
#define DebugTriggerMask	(0x0000039C)
#define DebugTriggerValue	(0x000003A0)
#define TSFTimerLow_X	(0x000003A4)
#define TSFTimerHigh_X	(0x000003A8)
#define radioregaddr_cross	(0x000003C8)
#define radioregdata_cross	(0x000003CA)
#define radioregaddr	(0x000003D8)
#define radioregdata	(0x000003DA)
#define rfdisabledly	(0x000003DC)
#define PHY_REG_0	(0x000003E0)
#define PHY_REG_1	(0x000003E2)
#define PHY_REG_2	(0x000003E4)
#define PHY_REG_3	(0x000003E6)
#define PHY_REG_4	(0x000003E8)
#define PHY_REG_5	(0x000003EA)
#define PHY_REG_7	(0x000003EC)
#define PHY_REG_6	(0x000003EE)
#define PHY_REG_8	(0x000003F0)
#define PHY_REG_9	(0x000003F2)
#define PHY_REG_A	(0x000003F4)
#define PHY_REG_B	(0x000003F6)
#define PHY_REG_C	(0x000003F8)
#define PHY_REG_D	(0x000003FA)
#define PHY_REG_ADDR	(0x000003FC)
#define PHY_REG_DATA	(0x000003FE)
#define SHM_BUF_BASE	(0x00000402)
#define SHM_BYT_CNT	(0x00000404)
#define RCV_FIFO_CTL	(0x00000406)
#define RCV_CTL	(0x00000408)
#define RCV_FRM_CNT	(0x0000040A)
#define RCV_STATUS_LEN	(0x0000040C)
#define RCV_SHM_STADDR	(0x0000040E)
#define RCV_SHM_STCNT	(0x00000410)
#define RXE_PHYRS_0	(0x00000412)
#define RXE_PHYRS_1	(0x00000414)
#define RXE_COND	(0x00000416)
#define RXE_RXCNT	(0x00000418)
#define RXE_STATUS1	(0x0000041A)
#define RXE_STATUS2	(0x0000041C)
#define RXE_PLCP_LEN	(0x0000041E)
#define RCV_FRM_CNT_Q0	(0x00000420)
#define RCV_FRM_CNT_Q1	(0x00000422)
#define RCV_WRD_CNT_Q0	(0x00000424)
#define RCV_WRD_CNT_Q1	(0x00000426)
#define RCV_RXFIFO_WRBURST	(0x00000428)
#define RCV_PHYFIFO_WRDCNT	(0x0000042A)
#define RCV_BM_STARTPTR_Q0	(0x0000042C)
#define RCV_BM_ENDPTR_Q0	(0x0000042E)
#define EXT_IHR_ADDR	(0x00000430)
#define EXT_IHR_DATA	(0x00000432)
#define RXE_PHYRS_2	(0x00000434)
#define RXE_PHYRS_3	(0x00000436)
#define PHY_MODE	(0x00000438)
#define RCV_BM_STARTPTR_Q1	(0x0000043A)
#define RCV_BM_ENDPTR_Q1	(0x0000043C)
#define RCV_COPYCNT_Q1	(0x0000043E)
#define RXE_PHYRS_ADDR	(0x00000440)
#define RXE_PHYRS_DATA	(0x00000442)
#define RXE_PHYRS_4	(0x00000444)
#define RXE_PHYRS_5	(0x00000446)
#define RXE_ERRVAL	(0x00000448)
#define RXE_ERRMASK	(0x0000044A)
#define RXE_STATUS3	(0x0000044C)
#define SHM_RXE_ADDR	(0x0000044E)
#define RCV_AMPDU_CTL0	(0x00000450)
#define RCV_AMPDU_CTL1	(0x00000452)
#define RCV_CTL1	(0x00000454)
#define RCV_LFIFO_STS	(0x00000456)
#define RCV_AMPDU_STS	(0x00000458)
#define AVB_RXTIMESTAMP_L	(0x0000045A)
#define AVB_RXTIMESTAMP_H	(0x0000045C)
#define radioihrAddr	(0x00000460)
#define radioihrData	(0x00000462)
#define PSM_SLEEP_TMR	(0x00000480)
#define PSM_MAC_CTLH	(0x00000482)
#define PSM_MAC_INTSTAT_L	(0x00000484)
#define PSM_MAC_INTSTAT_H	(0x00000486)
#define PSM_MAC_INTMASK_L	(0x00000488)
#define PSM_MAC_INTMASK_H	(0x0000048A)
#define PSM_ERR_PC	(0x0000048C)
#define PSM_MACCOMMAND	(0x0000048E)
#define PSM_BRC_0	(0x00000490)
#define PSM_PHY_CTL	(0x00000492)
#define PSM_IHR_ERR	(0x00000494)
#define psm_int_sel_2	(0x0498)	/* missing */
#define PSM_GPIOIN	(0x0000049A)
#define PSM_GPIOOUT	(0x0000049C)
#define PSM_GPIOEN	(0x0000049E)
#define PSM_BRED_0	(0x000004A0)
#define PSM_BRED_1	(0x000004A2)
#define PSM_BRED_2	(0x000004A4)
#define PSM_BRED_3	(0x000004A6)
#define PSM_BRCL_0	(0x000004A8)
#define PSM_BRCL_1	(0x000004AA)
#define PSM_BRCL_2	(0x000004AC)
#define PSM_BRCL_3	(0x000004AE)
#define PSM_BRPO_0	(0x000004B0)
#define PSM_BRPO_1	(0x000004B2)
#define PSM_BRPO_2	(0x000004B4)
#define PSM_BRPO_3	(0x000004B6)
#define PSM_BRWK_0	(0x000004B8)
#define PSM_BRWK_1	(0x000004BA)
#define PSM_BRWK_2	(0x000004BC)
#define PSM_BRWK_3	(0x000004BE)
#define PSM_INTSEL_0	(0x000004C0)
#define PSM_INTSEL_1	(0x000004C2)
#define PSM_INTSEL_2	(0x000004C4)
#define PSM_INTSEL_3	(0x000004C6)
#define PSM_STAT_CTR0_L	(0x000004C8)
#define PSM_STAT_CTR0_H	(0x000004CA)
#define PSM_STAT_CTR1_L	(0x000004CC)
#define PSM_STAT_CTR1_H	(0x000004CE)
#define PSM_STAT_SEL	(0x000004D0)
#define SubrStkStatus	(0x000004D2)
#define SubrStkRdPtr	(0x000004D4)
#define SubrStkRdData	(0x000004D6)
#define PSM_BRC_1	(0x000004D8)
#define PSM_MUL	(0x000004DA)
#define PSM_MACCONTROL1	(0x000004DC)
#define PSMSrchCtrlStatus	(0x000004E0)
#define PSMSrchBase	(0x000004E2)
#define PSMSrchLimit	(0x000004E4)
#define PSMSrchAddress	(0x000004E6)
#define PSMSrchData	(0x000004E8)
#define SBRegAddr	(0x000004EA)
#define SBRegDataL	(0x000004EC)
#define SBRegDataH	(0x000004EE)
#define PSMCoreCtlStat	(0x000004F0)
#define PSMWorkaround	(0x000004F2)
#define SbAddrLL	(0x000004F4)
#define SbAddrL	(0x000004F6)
#define SbAddrH	(0x000004F8)
#define SbAddrHH	(0x000004FA)
#define GpioOut	(0x000004FC)
#define PSM_TXMEM_PDA	(0x000004FE)
#define TXE_CTL	(0x00000500)
#define TXE_AUX	(0x00000502)
#define TXE_TS_LOC	(0x00000504)
#define TXE_TIME_OUT	(0x00000506)
#define TXE_WM_0	(0x00000508)
#define TXE_WM_1	(0x0000050A)
#define TXE_PHYCTL	(0x0000050C)
#define TXE_STATUS	(0x0000050E)
#define TXE_MMPLCP_0	(0x00000510)
#define TXE_MMPLCP_1	(0x00000512)
#define TXE_PHYCTL_1	(0x00000514)
#define TXE_PHYCTL_2	(0x00000516)
#define TXE_FRMSTAT_ADDR	(0x00000518)
#define TXE_FRMSTAT_DATA	(0x0000051A)
#define TXE_STATUS2	(0x0000051C)
#define XmtFIFOFullThreshold	(0x00000520)
#define BMCReadReq	(0x00000526)
#define BMCReadOffset	(0x00000528)
#define BMCReadLength	(0x0000052A)
#define BMCReadStatus	(0x0000052C)
#define XmtShmAddr	(0x0000052E)
#define PsmMSDUAccess	(0x00000530)
#define MSDUEntryBufCnt	(0x00000532)
#define MSDUEntryStartIdx	(0x00000534)
#define MSDUEntryEndIdx	(0x00000536)
#define SMP_PTR_H	(0x00000538)
#define SCP_CURPTR_H	(0x0000053A)
#define BMCCmd1	(0x0000053C)
#define BMCDynAllocStatus	(0x0000053E)
#define BMCCTL	(0x00000540)
#define BMCConfig	(0x00000542)
#define BMCStartAddr	(0x00000544)
#define BMCSize	(0x0546)	/* missing */
#define BMCCmd	(0x00000548)
#define BMCMaxBuffers	(0x0000054A)
#define BMCMinBuffers	(0x0000054C)
#define BMCAllocCtl	(0x0000054E)
#define BMCDescrLen	(0x00000550)
#define SCP_STRTPTR	(0x00000552)
#define SCP_STOPPTR	(0x00000554)
#define SCP_CURPTR	(0x00000556)
#define SaveRestoreStartPtr	(0x0558)	/* missing */
#define SPP_STRTPTR	(0x0000055A)
#define SPP_STOPPTR	(0x0000055C)
#define XmtDMABusy	(0x0000055E)
#define XmtTemplateDataLo	(0x00000560)
#define XmtTemplateDataHi	(0x00000562)
#define XmtTemplatePtr	(0x00000564)
#define XmtSuspFlush	(0x00000566)
#define XmtFifoRqPrio	(0x00000568)
#define BMCStatCtl	(0x0000056A)
#define BMCStatData	(0x0000056C)
#define BMCMSDUFifoStat	(0x0000056E)
#define XMT_AMPDU_CTL	(0x00000570)
#define XMT_AMPDU_LEN	(0x00000572)
#define XMT_AMPDU_CRC	(0x00000574)
#define TXE_CTL1	(0x00000576)
#define TXE_STATUS1	(0x00000578)
#define AVB_TXTIMESTAMP_L	(0x0000057A)
#define AVB_TXTIMESTAMP_H	(0x0000057C)
#define XMT_AMPDU_DLIM	(0x0000057E)
#define TXE_BM_0	(0x00000580)
#define TXE_BM_1	(0x00000582)
#define TXE_BM_2	(0x00000584)
#define TXE_BM_3	(0x00000586)
#define TXE_BM_4	(0x00000588)
#define TXE_BM_5	(0x0000058A)
#define TXE_BM_6	(0x0000058C)
#define TXE_BM_7	(0x0000058E)
#define TXE_BM_8	(0x00000590)
#define TXE_BM_9	(0x00000592)
#define TXE_BM_10	(0x00000594)
#define TXE_BM_11	(0x00000596)
#define TXE_BM_12	(0x00000598)
#define TXE_BM_13	(0x0000059A)
#define TXE_BM_14	(0x0000059C)
#define TXE_BM_15	(0x0000059E)
#define TXE_BM_16	(0x000005A0)
#define TXE_BM_17	(0x000005A2)
#define TXE_BM_18	(0x000005A4)
#define TXE_BM_19	(0x000005A6)
#define TXE_BM_20	(0x000005A8)
#define TXE_BM_21	(0x000005AA)
#define TXE_BM_22	(0x000005AC)
#define TXE_BM_23	(0x000005AE)
#define TXE_BM_24	(0x000005B0)
#define TXE_BM_25	(0x000005B2)
#define TXE_BM_26	(0x000005B4)
#define TXE_BM_27	(0x000005B6)
#define TXE_BM_28	(0x000005B8)
#define TXE_BM_29	(0x000005BA)
#define TXE_BM_30	(0x000005BC)
#define TXE_BM_31	(0x000005BE)
#define TXE_BV_0	(0x000005C0)
#define TXE_BV_1	(0x000005C2)
#define TXE_BV_2	(0x000005C4)
#define TXE_BV_3	(0x000005C6)
#define TXE_BV_4	(0x000005C8)
#define TXE_BV_5	(0x000005CA)
#define TXE_BV_6	(0x000005CC)
#define TXE_BV_7	(0x000005CE)
#define TXE_BV_8	(0x000005D0)
#define TXE_BV_9	(0x000005D2)
#define TXE_BV_10	(0x000005D4)
#define TXE_BV_11	(0x000005D6)
#define TXE_BV_12	(0x000005D8)
#define TXE_BV_13	(0x000005DA)
#define TXE_BV_14	(0x000005DC)
#define TXE_BV_15	(0x000005DE)
#define TXE_BV_16	(0x000005E0)
#define TXE_BV_17	(0x000005E2)
#define TXE_BV_18	(0x000005E4)
#define TXE_BV_19	(0x000005E6)
#define TXE_BV_20	(0x000005E8)
#define TXE_BV_21	(0x000005EA)
#define TXE_BV_22	(0x000005EC)
#define TXE_BV_23	(0x000005EE)
#define TXE_BV_24	(0x000005F0)
#define TXE_BV_25	(0x000005F2)
#define TXE_BV_26	(0x000005F4)
#define TXE_BV_27	(0x000005F6)
#define TXE_BV_28	(0x000005F8)
#define TXE_BV_29	(0x000005FA)
#define TXE_BV_30	(0x000005FC)
#define TXE_BV_31	(0x000005FE)
#define TSF_CTL	(0x00000600)
#define TSF_STAT	(0x00000602)
#define TSF_CFP_STRT_L	(0x00000604)
#define TSF_CFP_STRT_H	(0x00000606)
#define TSF_CFP_END_L	(0x00000608)
#define TSF_CFP_END_H	(0x0000060A)
#define TSF_CFP_MAX_DUR	(0x0000060C)
#define TSF_CFP_REP_L	(0x0000060E)
#define TSF_CFP_REP_H	(0x00000610)
#define TSF_CFP_PRE_TBTT	(0x00000612)
#define TSF_CFP_CFP_D0_L	(0x00000614)
#define TSF_CFP_CFP_D0_H	(0x00000616)
#define TSF_CFP_CFP_D1_L	(0x00000618)
#define TSF_CFP_CFP_D1_H	(0x0000061A)
#define TSF_CFP_CFP_D2_L	(0x0000061C)
#define TSF_CFP_CFP_D2_H	(0x0000061E)
#define TSF_CFP_TXOP_SQS_L	(0x00000620)
#define TSF_CFP_TXOP_SQS_H	(0x00000622)
#define TSF_CFP_TXOP_PQS	(0x00000624)
#define TSF_CFP_TXOP_SQD_L	(0x00000626)
#define TSF_CFP_TXOP_SQD_H	(0x00000628)
#define TSF_CFP_TXOP_PQD	(0x0000062A)
#define TSF_FES_DUR	(0x0000062C)
#define TSF_CLK_FRAC_L	(0x0000062E)
#define TSF_CLK_FRAC_H	(0x00000630)
#define TSF_TMR_TSF_L	(0x00000632)
#define TSF_TMR_TSF_ML	(0x00000634)
#define TSF_TMR_TSF_MU	(0x00000636)
#define TSF_TMR_TSF_H	(0x00000638)
#define TSF_TMR_TX_OFFSET	(0x0000063A)
#define TSF_TMR_RX_OFFSET	(0x0000063C)
#define TSF_TMR_RX_TS	(0x0000063E)
#define TSF_TMR_TX_TS	(0x00000640)
#define TSF_TMR_RX_END_TS	(0x00000642)
#define TSF_TMR_DELTA	(0x00000644)
#define TSF_GPT_0_STAT	(0x00000646)
#define TSF_GPT_1_STAT	(0x00000648)
#define TSF_GPT_0_CTR_L	(0x0000064A)
#define TSF_GPT_1_CTR_L	(0x0000064C)
#define TSF_GPT_0_CTR_H	(0x0000064E)
#define TSF_GPT_1_CTR_H	(0x00000650)
#define TSF_GPT_0_VAL_L	(0x00000652)
#define TSF_GPT_1_VAL_L	(0x00000654)
#define TSF_GPT_0_VAL_H	(0x00000656)
#define TSF_GPT_1_VAL_H	(0x00000658)
#define TSF_RANDOM	(0x0000065A)
#define RAND_SEED_0	(0x0000065C)
#define RAND_SEED_1	(0x0000065E)
#define RAND_SEED_2	(0x00000660)
#define TSF_ADJUST	(0x00000662)
#define TSF_PHY_HDR_TM	(0x00000664)
#define TSF_GPT_2_STAT	(0x00000666)
#define TSF_GPT_2_CTR_L	(0x00000668)
#define TSF_GPT_2_CTR_H	(0x0000066A)
#define TSF_GPT_2_VAL_L	(0x0000066C)
#define TSF_GPT_2_VAL_H	(0x0000066E)
#define TSF_GPT_ALL_STAT	(0x00000670)
#define TSF_TMR_TX_ERR_TS	(0x00000672)
#define IFS_SIFS_RX_TX_TX	(0x00000680)
#define IFS_SIFS_NAV_TX	(0x00000682)
#define IFS_SLOT	(0x00000684)
#define IFS_EIFS	(0x00000686)
#define IFS_CTL	(0x00000688)
#define IFS_BOFF_CTR	(0x0000068A)
#define IFS_SLOT_CTR	(0x0000068C)
#define IFS_FREE_SLOTS	(0x0000068E)
#define IFS_STAT	(0x00000690)
#define IFS_MEDBUSY_CTR	(0x00000692)
#define IFS_TX_DUR	(0x00000694)
#define IFS_RIFS_TIME	(0x00000696)
#define IFS_STAT1	(0x00000698)
#define IFS_EDCAPRI	(0x0000069A)
#define IFS_AIFSN	(0x0000069C)
#define IFS_CTL1	(0x0000069E)
#define SLOW_CTL	(0x000006A0)
#define SLOW_TIMER_L	(0x000006A2)
#define SLOW_TIMER_H	(0x000006A4)
#define SLOW_FRAC	(0x000006A6)
#define FAST_PWRUP_DLY	(0x000006A8)
#define SLOW_PER	(0x000006AA)
#define SLOW_PER_FRAC	(0x000006AC)
#define SLOW_CALTIMER_L	(0x000006AE)
#define SLOW_CALTIMER_H	(0x000006B0)
#define IFS_STAT2	(0x000006B2)
#define BTCX_CTL	(0x000006B4)
#define BTCX_STAT	(0x000006B6)
#define BTCX_TRANSCTL	(0x000006B8)
#define BTCX_PRIORITYWIN	(0x000006BA)	// BTCXPriorityWin
#define BTCX_TXCONFTIMER	(0x06bc)	// BTCXConfTimer
#define BTCX_PRISELTIMER	(0x06be)	// old: BTCXPriSelTimer
#define BTCX_PRV_RFACT_TIMER	(0x06c0)	// old: BTCXPrvRfActTimer
#define BTCX_CUR_RFACT_TIMER	(0x06c2)	// old: BTCXCurRfActTimer
#define BTCX_RFACT_DUR_TIMER	(0x06c4)	// old: BTCXActDurTimer
#define IFS_CTL_SEL_PRICRS	(0x000006C6)
#define IfsCtlSelSecCrs	(0x000006C8)
#define IfStatEdCrs160M	(0x000006CA)
#define CRSEDCntrCtrl	(0x000006CC)
#define CRSEDCntrAddr	(0x000006CE)
#define CRSEDCntrData	(0x000006D0)
#define EXT_STAT_EDCRS160M	(0x000006D2)
#define ERCXControl	(0x000006D4)
#define ERCXStatus	(0x000006D6)
#define ERCXTransCtl	(0x000006D8)
#define ERCXPriorityWin	(0x000006DA)
#define ERCXConfTimer	(0x000006DC)
#define ERCX_PRISELTIMER	(0x000006DE)
#define ERCXPrvRfActTimer	(0x000006E0)
#define ERCXCurRfActTimer	(0x000006E2)
#define ERCXActDurTimer	(0x000006E4)
#define IFS_MEDBUSY_CRS_CTR	(0x000006E6)
#define IFS_RX_DUR	(0x000006E8)
#define BTCX_ECI_ADDR		(0x06f0)	// old: BTCXEciAddr
#define BTCX_ECI_DATA		(0x06f2)	//old: BTCXEciData
#define BTCX_ECI_MASK_ADDR	(0x06f4)	// old: BTCXEciMaskAddr
#define BTCX_ECI_MASK_DATA	(0x06f6)	// old: BTCXEciMaskData
#define COEX_IO_MASK		(0x06f8)	// old: CoexIOMask
#define BTCX_ECI_EVENT_ADDR	(0x06fa)	// old: btcx_eci_event_addr
#define BTCX_ECI_EVENT_DATA	(0x06fc)	// old: btcx_eci_event_data
#define NAV_CTL		(0x00000700)
#define NAV_STAT	(0x00000702)
#define NAV_CNTR_L	(0x00000704)
#define NAV_CNTR_H	(0x00000706)
#define NAV_TBTT_NOW_L	(0x00000708)
#define NAV_TBTT_NOW_H	(0x0000070A)
#define NAV_DUR	(0x0000070C)
#define NAV_DELTA_L	(0x0000070E)
#define NAV_DELTA_H	(0x00000710)
#define NAV_CTSTO	(0x00000712)
#define WEP_CTL	(0x000007C0)
#define WEP_STAT	(0x000007C2)
#define WEP_HDRLOC	(0x000007C4)
#define WEP_PSDULEN	(0x000007C6)
#define WEP_KEY_ADDR	(0x000007C8)
#define WEP_KEY_DATA	(0x000007CA)
#define WEP_ADDR	(0x000007CC)
#define WEP_DATA	(0x000007CE)
#define WEP_HWKEY_ADDR	(0x000007D0)
#define WEP_HWKEY_LEN	(0x000007D2)
#define WEP_HWMICK_ADDR	(0x000007D4)
#define WEP_HWMICK_LEN	(0x000007D6)
#define PMQ_CTL	(0x000007E0)
#define PMQ_STATUS	(0x000007E2)
#define PMQ_PAT_0	(0x000007E4)
#define PMQ_PAT_1	(0x000007E6)
#define PMQ_PAT_2	(0x000007E8)
#define PMQ_DAT	(0x000007EA)
#define PMQ_DAT_OR_MAT	(0x000007EC)
#define PMQ_DAT_OR_ALL	(0x000007EE)
#define PMQ_DAT_OR_MAT_MU1	(0x000007F0)
#define PMQ_DAT_OR_MAT_MU2	(0x000007F2)
#define PMQ_DAT_OR_MAT_MU3	(0x000007F4)
#define PMQ_AUXPMQ_STATUS	(0x000007F6)
#define PMQ_CTL1	(0x000007F8)
#define PMQ_STATUS1	(0x000007FA)
#define PMQ_ADDTHR	(0x000007FC)
#define CTMode	(0x00000800)
#define SCS_HMASK_L	(0x00000802)
#define SCS_HMASK_H	(0x00000804)
#define SCM_HMASK_L	(0x00000806)
#define SCM_HMASK_H	(0x00000808)
#define SCM_HVAL_L	(0x0000080A)
#define SCM_HVAL_H	(0x0000080C)
#define SCT_HMASK_L	(0x0000080E)
#define SCT_HMASK_H	(0x00000810)
#define SCT_HVAL_L	(0x00000812)
#define SCT_HVAL_H	(0x00000814)
#define SCX_HMASK_L	(0x00000816)
#define SCX_HMASK_H	(0x00000818)
#define DebugBusCtrl	(0x0000081C)
#define BMC_ReadQID	(0x0000081E)
#define BMC_BQFrmCnt	(0x00000820)
#define BMC_BQByteCnt_L	(0x00000822)
#define BMC_BQByteCnt_H	(0x00000824)
#define AQM_BQFrmCnt	(0x00000826)
#define AQM_BQByteCnt_L	(0x00000828)
#define AQM_BQByteCnt_H	(0x0000082A)
#define AQM_BQPrelWM	(0x0000082C)
#define AQM_BQPrelStatus	(0x0000082E)
#define AQM_BQStatus	(0x00000830)
#define BMC_MUDebugConfig	(0x00000832)
#define BMC_MUDebugStatus	(0x00000834)
#define BMCBQCutThruSt0	(0x00000836)
#define BMCBQCutThruSt1	(0x00000838)
#define BMCBQCutThruSt2	(0x0000083A)
#define TDCCTL	(0x00000840)
#define TDC_Plcp0	(0x00000842)
#define TDC_Plcp1	(0x00000844)
#define TDC_FrmLen0	(0x00000846)
#define TDC_FrmLen1	(0x00000848)
#define TDC_Txtime	(0x0000084A)
#define TDC_VhtSigB0	(0x0000084C)
#define TDC_VhtSigB1	(0x0000084E)
#define TDC_LSigLen	(0x00000850)
#define TDC_NSym0	(0x00000852)
#define TDC_NSym1	(0x00000854)
#define TDC_VhtPsduLen0	(0x00000856)
#define TDC_VhtPsduLen1	(0x00000858)
#define TDC_VhtMacPad0	(0x0000085A)
#define TDC_VhtMacPad1	(0x0000085C)
#define TDC_MuVhtMCS	(0x0000085E)
#define ShmDma_Ctl	(0x00000860)
#define ShmDma_TxdcAddr	(0x00000862)
#define ShmDma_ShmAddr	(0x00000864)
#define ShmDma_XferWCnt	(0x00000866)
#define Txdc_Addr	(0x00000868)
#define Txdc_Data	(0x0000086A)
#define V2M_MAXWCNT	(0x0000086E)
#define TXE_VASIP_INTSTS	(0x00000870)
#define MHP_Status	(0x00000880)
#define MHP_FC	(0x00000882)
#define MHP_DUR	(0x00000884)
#define MHP_SC	(0x00000886)
#define MHP_QOS	(0x00000888)
#define MHP_HTC_H	(0x0000088A)
#define MHP_HTC_L	(0x0000088C)
#define MHP_Addr1_H	(0x0000088E)
#define MHP_Addr1_M	(0x00000890)
#define MHP_Addr1_L	(0x00000892)
#define MHP_Addr2_H	(0x000008A0)
#define MHP_Addr2_M	(0x000008A2)
#define MHP_Addr2_L	(0x000008A4)
#define MHP_Addr3_H	(0x000008A6)
#define MHP_Addr3_M	(0x000008A8)
#define MHP_Addr3_L	(0x000008AA)
#define MHP_Addr4_H	(0x000008AC)
#define MHP_Addr4_M	(0x000008AE)
#define MHP_Addr4_L	(0x000008B0)
#define MHP_CFC	(0x000008B2)
#define RXE_STS_AUX_0	(0x000008B4)
#define RXE_STS_AUX_1	(0x000008B6)
#define RXE_STS_AUX_2	(0x000008B8)
#define RXE_STS_AUX_3	(0x000008BA)
#define DAGG_CTL2	(0x000008C0)
#define DAGG_BYTESLEFT	(0x000008C2)
#define DAGG_SH_OFFSET	(0x000008C4)
#define DAGG_STAT	(0x000008C6)
#define DAGG_LEN	(0x000008C8)
#define TXBA_CTL	(0x000008CA)
#define TXBA_DataSel	(0x000008CC)
#define TXBA_Data	(0x000008CE)
#define DAGG_LEN_THR	(0x000008D0)
#define AMT_CTL	(0x000008E0)
#define AMT_Status	(0x000008E2)
#define AMT_Limit	(0x000008E4)
#define AMT_Attr	(0x000008E6)
#define AMT_MATCH1	(0x000008E8)
#define AMT_MATCH2	(0x000008EA)
#define AMT_Table_Addr	(0x000008EC)
#define AMT_Table_Data	(0x000008EE)
#define AMT_Table_Val	(0x000008F0)
#define AMT_DBG_SEL	(0x000008F2)
#define AMT_MATCH	(0x000008F4)
#define AMT_ATTR_A1	(0x000008F6)
#define AMT_ATTR_A2	(0x000008F8)
#define AMT_ATTR_A3	(0x000008FA)
#define AMT_ATTR_BSSID	(0x000008FC)
#define RoeCtrl	(0x00000900)
#define RoeStatus	(0x00000902)
#define RoeIPChkSum	(0x00000904)
#define RoeTCPUDPChkSum	(0x00000906)
#define RoeStatus1	(0x00000908)
#define PSOCtl	(0x00000920)
#define PSORxWordsWatermark	(0x00000922)
#define PSORxCntWatermark	(0x00000924)
#define PSOCurRxFramePtrs	(0x00000926)
#define OBFFCtl	(0x00000930)
#define OBFFRxWordsWatermark	(0x00000932)
#define OBFFRxCntWatermark	(0x00000934)
#define PSOOBFFStatus	(0x00000936)
#define LtrRxTimer	(0x00000938)
#define LtrRxWordsWatermark	(0x0000093A)
#define LtrRxCntWatermark	(0x0000093C)
#define RcvHdrConvCtrlSts	(0x0000093E)
#define RcvHdrConvSts	(0x00000940)
#define RcvHdrConvSts1	(0x00000942)
#define RCVLB_DAGG_CTL	(0x00000944)
#define RcvFifo0Len	(0x00000946)
#define RcvFifo1Len	(0x00000948)
#define CRSStatus	(0x00000960)
#define ToECTL	(0x00000A00)
#define ToERst	(0x00000A02)
#define ToECSumNZ	(0x00000A04)
#define ToEChannelState	(0x00000A06)
#define BMC_REG_SEL	(0x00000A08)
#define SERIAL_REG_SEL	(0x00000A0A)
#define WEP_REG_SEL	(0x00000A0C)
#define TDC_REG_SEL	(0x00000A0E)
#define TxSerialCtl	(0x00000A40)
#define TxPlcpLSig0	(0x00000A42)
#define TxPlcpLSig1	(0x00000A44)
#define TxPlcpHtSig0	(0x00000A46)
#define TxPlcpHtSig1	(0x00000A48)
#define TxPlcpHtSig2	(0x00000A4A)
#define TxPlcpVhtSigB0	(0x00000A4C)
#define TxPlcpVhtSigB1	(0x00000A4E)
#define MacHdrFromShmLen	(0x00000A52)
#define TxPlcpLen	(0x00000A54)
#define XMT_AMPDU_DLIM_H	(0x00000A56)
#define TxBFRptLen	(0x00000A58)
#define BytCntInTxFrmLo	(0x00000A5A)
#define BytCntInTxFrmHi	(0x00000A5C)
#define TXBFCtl	(0x00000A60)
#define BfmRptOffset	(0x00000A62)
#define BfmRptLen	(0x00000A64)
#define TXBFBfeRptRdCnt	(0x00000A66)
#define PhyDebugL	(0x00000A68)
#define PhyDebugH	(0x00000A6A)
#define TXE_CTL2	(0x00000A6C)
#define PSM_ALT_MAC_INTSTATUS_L	(0x00000A84)
#define PSM_ALT_MAC_INTSTATUS_H	(0x00000A86)
#define PSM_ALT_MAC_INTMASK_L	(0x00000A88)
#define PSM_ALT_MAC_INTMASK_H	(0x00000A8A)
#define PsmMboxAddr	(0x00000A8C)
#define PsmMboxData	(0x00000A8E)
#define PSM_REG_MUX	(0x00000A90)
#define PSM_BASE_0	(0x00000AA0)
#define PSM_BASE_1	(0x00000AA2)
#define PSM_BASE_2	(0x00000AA4)
#define PSM_BASE_3	(0x00000AA6)
#define PSM_BASE_4	(0x00000AA8)
#define PSM_BASE_5	(0x00000AAA)
#define PSM_BASE_6	(0x00000AAC)
#define PSM_BASE_7	(0x00000AAE)
#define PSM_BASE_8	(0x00000AB0)
#define PSM_BASE_9	(0x00000AB2)
#define PSM_BASE_10	(0x00000AB4)
#define PSM_BASE_11	(0x00000AB6)
#define PSM_BASE_12	(0x00000AB8)
#define PSM_BASE_13	(0x00000ABA)
#define PSM_BASE_PSMX	(0x00000ABC)
#define PSM_BRC_SEL	(0x00000ABE)
#define PSM_CHK0_CMD	(0x00000AC0)
#define PSM_CHK0_SEL	(0x00000AC2)
#define PSM_CHK0_ERR	(0x00000AC4)
#define PSM_CHK0_INFO	(0x00000AC6)
#define PSM_CHK1_CMD	(0x00000AC8)
#define PSM_CHK1_SEL	(0x00000ACA)
#define PSM_CHK1_ERR	(0x00000ACC)
#define PSM_CHK1_INFO	(0x00000ACE)
#define RXMapFifoSize	(0x00000B00)
#define RXMapStatus	(0x00000B02)
#define MsduThreshold	(0x00000B04)
#define DebugTxFlowCtl	(0x00000B06)
#define XmtDPQSuspFlush	(0x00000B08)
#define MSDUIdxFifoConfig	(0x00000B0A)
#define MSDUIdxFifoDef	(0x00000B0C)
#define BMCCore0TXAllMaxBuffers	(0x00000B0E)
#define BMCCore1TXAllMaxBuffers	(0x00000B10)
#define BMCDynAllocStatus1	(0x00000B12)
#define DMAMaxOutStBuffers	(0x00000B14)
#define SCS_MASK_L		(0x00000B16)	/* rename */
#define SCS_MASK_H		(0x00000B18)	/* rename */
#define SCM_MASK_L		(0x00000B1A)	/* rename */
#define SCM_MASK_H		(0x00000B1C)	/* rename */
#define SCM_VAL_L		(0x00000B1E)	/* rename */
#define SCM_VAL_H		(0x00000B20)	/* rename */
#define SCT_MASK_L		(0x00000B22)	/* rename */
#define SCT_MASK_H		(0x00000B24)	/* rename */
#define SCT_VAL_L		(0x00000B26)	/* rename */
#define SCT_VAL_H		(0x00000B28)	/* rename */
#define SCX_MASK_L		(0x00000B2A)	/* rename */
#define SCX_MASK_H		(0x00000B2C)	/* rename */
#define SMP_CTRL		(0x00000B2E)	/* rename */
#define Core0BMCAllocStatusTID7	(0x00000B30)
#define Core1BMCAllocStatusTID7	(0x00000B32)
#define MsduFifoReachThreshold	(0x00000B34)
#define BMVpConfig	(0x00000B36)
#define TXE_DBG_BMC_STATUS	(0x00000B38)
#define XmtTemplatePtrOffset	(0x00000B3A)
#define BMCCutThruConfig	(0x00000B3C)
#define BMCCutThruTestCfg0	(0x00000B3E)
#define BMCCutThruTestCfg1	(0x00000B40)
#define BMCCutThruTestCfg2	(0x00000B42)
#define BMCCutThruTestCfg3	(0x00000B44)
#define BMCCutThruTestCfg4	(0x00000B46)
#define SysMStartAddrHi	(0x00000B48)
#define SysMStartAddrLo	(0x00000B4A)
#define MSDUEntryLBufLen	(0x00000B4C)
#define BMCMaskAllocReq	(0x00000B4E)
#define DBG_LLMCFG	(0x00000B50)
#define DBG_LLMSTS	(0x00000B52)
#define AQMConfig	(0x00000B60)
#define AQMFifoDef	(0x00000B62)
#define AQM_REG_SEL	(0x00000B64)
#define AQMQMAP	(0x00000B66)
#define AQMCmd	(0x00000B68)
#define AQMConsMsdu	(0x00000B6A)
#define AQMDMACTL	(0x00000B6C)
#define AQMMaxIdx	(0x00000B6E)
#define AQMRcvdBA0	(0x00000B70)
#define AQMRcvdBA1	(0x00000B72)
#define AQMRcvdBA2	(0x00000B74)
#define AQMRcvdBA3	(0x00000B76)
#define AQMBaSSN	(0x00000B78)
#define AQMRefSN	(0x00000B7A)
#define AQMMaxAggLenLow	(0x00000B7C)
#define AQMMaxAggLenHi	(0x00000B7E)
#define AQMAggParams	(0x00000B80)
#define AQMMinMpduLen	(0x00000B82)
#define AQMMacAdjLen	(0x00000B84)
#define AQMMinCons	(0x00000B86)
#define MsduMinCons	(0x00000B88)
#define AQMAggStats	(0x00000B8A)
#define AQMAggNum	(0x00000B8C)
#define AQMAggLenLow	(0x00000B8E)
#define AQMAggLenHi	(0x00000B90)
#define AQMMpduLen	(0x00000B92)
#define AQMStartLoc	(0x00000B94)
#define AQMAggEntry	(0x00000B96)
#define AQMAggIdx	(0x00000B98)
#define AQMTxCnt	(0x00000B9A)
#define AQMAggRptr	(0x00000B9C)
#define AQMTxcntRptr	(0x00000B9E)
#define AQMCurTxcnt	(0x00000BA0)
#define AQMFiFoRptr	(0x00000BA2)
#define AQMFIFO_SOFDPtr	(0x00000BA4)
#define AQMFIFO_SWDCnt	(0x00000BA6)
#define AQMFIFO_TXDPtr_L	(0x00000BA8)
#define AQMFIFO_TXDPtr_ML	(0x00000BAA)
#define AQMFIFO_TXDPtr_MU	(0x00000BAC)
#define AQMFIFO_TXDPtr_H	(0x00000BAE)
#define AQMUpdBA0	(0x00000BB0)
#define AQMUpdBA1	(0x00000BB2)
#define AQMUpdBA2	(0x00000BB4)
#define AQMUpdBA3	(0x00000BB6)
#define AQMAckCnt	(0x00000BB8)
#define AQMConsCnt	(0x00000BBA)
#define AQMFifoRdy_L	(0x00000BBC)
#define AQMFifoRdy_H	(0x00000BBE)
#define AQMFifo_Status	(0x00000BC0)
#define AQMFifoFull_L	(0x00000BC2)
#define AQMFifoFull_H	(0x00000BC4)
#define AQMTBCP_Busy_L	(0x00000BC6)
#define AQMTBCP_Busy_H	(0x00000BC8)
#define AQMDMA_SuspFlush	(0x00000BCA)
#define AQMFIFOSusp_L	(0x00000BCC)
#define AQMFIFOSusp_H	(0x00000BCE)
#define AQMFIFO_SuspPend_L	(0x00000BD0)
#define AQMFIFO_SuspPend_H	(0x00000BD2)
#define AQMFIFOFlush_L	(0x00000BD4)
#define AQMFIFOFlush_H	(0x00000BD6)
#define AQMTXD_CTL	(0x00000BD8)
#define AQMTXD_RdOffset	(0x00000BDA)
#define AQMTXD_RdLen	(0x00000BDC)
#define AQMTXD_DestAddr	(0x00000BDE)
#define AQMTBCP_QSEL	(0x00000BE0)
#define AQMTBCP_Prio	(0x00000BE2)
#define AQMTBCP_PrioFifo	(0x00000BE4)
#define AQMFIFO_MPDULen	(0x00000BE6)
#define AQMTBCP_Max_ReqEntry	(0x00000BE8)
#define AQMTBCP_FCNT	(0x00000BEA)
#define AQMSOFP_MAXTHR	(0x00000BEC)
#define AQMTBCP_CHDIS	(0x00000BEE)
#define AQMSTATUS	(0x00000BF8)
#define AQMDBG_CTL	(0x00000BFA)
#define AQMDBG_DATA	(0x00000BFC)
#endif /* (AUTOREGS_COREREV >= 64) && (AUTOREGS_COREREV < 129) */

#if (AUTOREGS_COREREV >= 129)
#define biststatus	(0x0000000c)	/* missing */
#define syncSlowTimeStamp	(0x00000010)
#define syncFastTimeStamp	(0x00000014)
#define gptimer	(0x00000018)
#define usectimer	(0x0000001C)
#define intstat0	(0x00000020)
#define intmask0	(0x00000024)
#define intstat1	(0x00000028)
#define intmask1	(0x0000002C)
#define intstat2	(0x00000030)
#define intmask2	(0x00000034)
#define intstat3	(0x00000038)
#define intmask3	(0x0000003C)
#define intstat4	(0x00000040)
#define intmask4	(0x00000044)
#define intstat5	(0x00000048)
#define intmask5	(0x0000004C)
#define xmt_dma_chsel	(0x00000050)
#define xmt_dma_intmap0	(0x00000054)
#define alt_intmask0	(0x00000060)
#define alt_intmask1	(0x00000064)
#define alt_intmask2	(0x00000068)
#define alt_intmask3	(0x0000006C)
#define alt_intmask4	(0x00000070)
#define alt_intmask5	(0x00000074)
#define indintstat	(0x00000078)
#define indintmask	(0x0000007C)
#define inddma	(0x00000080)
#define IndXmtPtr	(0x00000084)
#define IndXmtAddrLow	(0x00000088)
#define IndXmtAddrHigh	(0x0000008C)
#define IndXmtStat0	(0x00000090)
#define IndXmtStat1	(0x00000094)
#define indaqm	(0x000000A0)
#define IndAQMptr	(0x000000A4)
#define IndAQMaddrlow	(0x000000A8)
#define IndAQMaddrhigh	(0x000000AC)
#define IndAQMstat0	(0x000000B0)
#define IndAQMstat1	(0x000000B4)
#define IndAQMIntStat	(0x000000B8)
#define IndAQMIntMask	(0x000000BC)
#define IndAQMQSel	(0x000000C0)
#define SUSPREQ	(0x000000C4)
#define FLUSHREQ	(0x000000C8)
#define CHNFLUSH_STATUS	(0x000000CC)
#define CHNSUSP_STATUS	(0x000000D0)
#define SUSPREQ1	(0x000000D4)
#define FLUSHREQ1	(0x000000D8)
#define CHNFLUSH_STATUS1	(0x000000DC)
#define CHNSUSP_STATUS1	(0x000000E0)
#define SUSPREQ2	(0x000000E4)
#define FLUSHREQ2	(0x000000E8)
#define CHNFLUSH_STATUS2	(0x000000EC)
#define CHNSUSP_STATUS2	(0x000000F0)
#define TxDMAIndXmtStat0	(0x000000F8)
#define TxDMAIndXmtStat1	(0x000000FC)
#define intrcvlzy0	(0x00000100)
#define intrcvlzy1	(0x00000104)
#define intrcvlzy2	(0x00000108)
#define intrcvlazy3	(0x0000010C)
#define MACCONTROL	(0x00000120)
#define MACCOMMAND	(0x00000124)
#define MACINTSTATUS	(0x00000128)
#define MACINTMASK	(0x0000012C)
#define XMT_TEMPLATE_RW_PTR	(0x00000130)
#define XMT_TEMPLATE_RW_DATA	(0x00000134)
#define PMQHOSTDATA	(0x00000140)
#define PMQPATL	(0x00000144)
#define PMQPATH	(0x00000148)
#define CHNSTATUS	(0x00000150)
#define PSM_DEBUG	(0x00000154)
#define PHY_DEBUG	(0x00000158)
#define MacCapability	(0x0000015C)
#define objaddr	(0x00000160)
#define objdata	(0x00000164)
#define ALT_MACINTSTATUS	(0x00000168)
#define ALT_MACINTMASK	(0x0000016C)
#define FrmTxStatus	(0x00000170)
#define FrmTxStatus2	(0x00000174)
#define FrmTxStatus3	(0x00000178)
#define FrmTxStatus4	(0x0000017C)
#define TSFTimerLow	(0x00000180)
#define TSFTimerHigh	(0x00000184)
#define CFPRep	(0x00000188)
#define CFPStart	(0x0000018C)
#define CFPMaxDur	(0x00000190)
#define AvbRxTimeStamp	(0x00000194)
#define AvbTxTimeStamp	(0x00000198)
#define MacControl1	(0x000001A0)
#define MacHWCap1	(0x000001A4)
#define GatedClkEn	(0x000001A8)
#define PHYPLUS1CTL	(0x000001AC)
#define gptimer_psmx	(0x000001B0)
#define MACCONTROL_psmx	(0x000001B4)
#define MacControl1_psmx	(0x000001B8)
#define MACCOMMAND_psmx	(0x000001BC)
#define MACINTSTATUS_psmx	(0x000001C0)
#define MACINTMASK_psmx	(0x000001C4)
#define ALT_MACINTSTATUS_psmx	(0x000001C8)
#define ALT_MACINTMASK_psmx	(0x000001CC)
#define PSM_DEBUG_psmx	(0x000001D0)
#define PHY_DEBUG_psmx	(0x000001D4)
#define ClockCtlStatus	(0x000001E0)
#define Workaround	(0x000001E4)
#define POWERCTL	(0x000001E8)
#define xmt0ctl	(0x00000200)
#define xmt0ptr	(0x00000204)
#define xmt0addrlow	(0x00000208)
#define xmt0addrhigh	(0x0000020C)
#define xmt0stat0	(0x00000210)
#define xmt0stat1	(0x00000214)
#define xmt0fifoctl	(0x00000218)
#define xmt0fifodata	(0x0000021C)
#define rcv0ctl	(0x00000220)
#define rcv0ptr	(0x00000224)
#define rcv0addrlow	(0x00000228)
#define rcv0addrhigh	(0x0000022C)
#define rcv0stat0	(0x00000230)
#define rcv0stat1	(0x00000234)
#define rcv0fifoctl	(0x00000238)
#define rcv0fifodata	(0x0000023C)
#define xmt1ctl	(0x00000240)
#define xmt1ptr	(0x00000244)
#define xmt1addrlow	(0x00000248)
#define xmt1addrhigh	(0x0000024C)
#define xmt1stat0	(0x00000250)
#define xmt1stat1	(0x00000254)
#define xmt1fifoctl	(0x00000258)
#define xmt1fifodata	(0x0000025C)
#define rcv1ctl	(0x00000260)
#define rcv1ptr	(0x00000264)
#define rcv1addrlow	(0x00000268)
#define rcv1addrhigh	(0x0000026C)
#define rcv1stat0	(0x00000270)
#define rcv1stat1	(0x00000274)
#define rcv1fifoctl	(0x00000278)
#define rcv1fifodata	(0x0000027C)
#define xmt2ctl	(0x00000280)
#define xmt2ptr	(0x00000284)
#define xmt2addrlow	(0x00000288)
#define xmt2addrhigh	(0x0000028C)
#define xmt2stat0	(0x00000290)
#define xmt2stat1	(0x00000294)
#define xmt2fifoctl	(0x00000298)
#define xmt2fifodata	(0x0000029C)
#define rcv2ctl	(0x000002A0)
#define rcv2ptr	(0x000002A4)
#define rcv2addrlow	(0x000002A8)
#define rcv2addrhigh	(0x000002AC)
#define rcv2stat0	(0x000002B0)
#define rcv2stat1	(0x000002B4)
#define xmt3ctl	(0x000002C0)
#define xmt3ptr	(0x000002C4)
#define xmt3addrlow	(0x000002C8)
#define xmt3addrhigh	(0x000002CC)
#define xmt3stat0	(0x000002D0)
#define xmt3stat1	(0x000002D4)
#define xmt3fifoctl	(0x000002D8)
#define xmt3fifodata	(0x000002DC)
#define xmt4ctl	(0x00000300)
#define xmt4ptr	(0x00000304)
#define xmt4addrlow	(0x00000308)
#define xmt4addrhigh	(0x0000030C)
#define xmt4stat0	(0x00000310)
#define xmt4stat1	(0x00000314)
#define xmt4fifoctl	(0x00000318)
#define xmt4fifodata	(0x0000031C)
#define gptimer_R1	(0x00000320)
#define MACCONTROL_r1	(0x00000324)
#define MacControl1_R1	(0x00000328)
#define MACCOMMAND_R1	(0x0000032C)
#define MACINTSTATUS_R1	(0x00000330)
#define MACINTMASK_R1	(0x00000334)
#define ALT_MACINTSTATUS_R1	(0x00000338)
#define ALT_MACINTMASK_R1	(0x0000033C)
#define xmt5ctl	(0x00000340)
#define xmt5ptr	(0x00000344)
#define xmt5addrlow	(0x00000348)
#define xmt5addrhigh	(0x0000034C)
#define xmt5stat0	(0x00000350)
#define xmt5stat1	(0x00000354)
#define xmt5fifoctl	(0x00000358)
#define xmt5fifodata	(0x0000035C)
#define PSM_DEBUG_R1	(0x00000360)
#define TSFTimerLow_R1	(0x00000364)
#define TSFTimerHigh_R1	(0x00000368)
#define PSM_GpioMonMemPtr	(0x0000036c)
#define PSM_GpioMonMemData	(0x00000370)
#define fifobase	(0x00000380)
#define fifodatalow	(0x00000384)
#define fifodatahigh	(0x00000388)
#define DebugStoreMask	(0x00000398)
#define DebugTriggerMask	(0x0000039C)
#define DebugTriggerValue	(0x000003A0)
#define TSFTimerLow_X	(0x000003A4)
#define TSFTimerHigh_X	(0x000003A8)
#define HostFCBSCmdPtr	(0x000003B4)
#define HostFCBSDataPtr	(0x000003B8)
#define HostCtrlStsReg	(0x000003BC)
#define RxFilterEn	(0x000003C0)	/* rename-as-80 */
#define RxHwaCtrl	(0x000003C4)	/* rename-as-80 */
#define radioregaddr_cross	(0x000003C8)
#define radioregdata_cross	(0x000003CA)
#define radioregaddr	(0x000003D8)
#define radioregdata	(0x000003DA)
#define rfdisabledly	(0x000003DC)
#define PHY_REG_0	(0x000003E0)
#define PHY_REG_1	(0x000003E2)
#define PHY_REG_2	(0x000003E4)
#define PHY_REG_3	(0x000003E6)
#define PHY_REG_4	(0x000003E8)
#define PHY_REG_5	(0x000003EA)
#define PHY_REG_7	(0x000003EC)
#define PHY_REG_6	(0x000003EE)
#define PHY_REG_8	(0x000003F0)
#define PHY_REG_9	(0x000003F2)
#define PHY_REG_A	(0x000003F4)
#define PHY_REG_B	(0x000003F6)
#define PHY_REG_C	(0x000003F8)
#define PHY_REG_D	(0x000003FA)
#define PHY_REG_ADDR	(0x000003FC)
#define PHY_REG_DATA	(0x000003FE)
#define RCV_CTL	(0x00000402)	/* rename */
#define RCV_CTL1	(0x00000404)	/* rename */
#define RXE_CBR_CTL	(0x00000406)
#define RXE_CBR_STAT	(0x00000408)
#define RCV_FIFO_CTL	(0x0000040A)	/* field definition extended */
#define RCV_LFIFO_STS	(0x0000040C)	/* rename */
#define RXE_RXCNT	(0x0000040E)	/* rename */
#define RCV_AMPDU_CTL0	(0x00000410)	/* rename */
#define RCV_AMPDU_CTL1	(0x00000412)	/* rename */
#define RCV_AMPDU_STS	(0x00000414)	/* rename */
#define RXE_ERRVAL	(0x00000416)	/* field definition extended */
#define RXE_ERRMASK	(0x00000418)	/* field definition extended */
#define RcvHdrConvCtrlSts	(0x0000041A)	/* rename */
#define DAGG_LEN_THR	(0x0000041C)	/* rename */
#define RCV_COPYCNT_Q1	(0x0000041E)	/* rename */
#define RXE_PLCP_LEN	(0x00000420)	/* rename */
#define RXE_STATUS1	(0x00000422)	/* rename */
#define RXE_STATUS2	(0x00000424)	/* field definition modified */
#define RXE_STATUS3	(0x00000426)	/* field definition extended */
#define CRSStatus	(0x00000428)	/* rename */
#define AVB_RXTIMESTAMP_L	(0x0000042A)	/* rename */
#define AVB_RXTIMESTAMP_h	(0x0000042C)	/* rename */
#define SHM_BUF_BASE	(0x00000430)	/* rename */
#define SHM_BYT_CNT	(0x00000432)	/* rename */
#define SHM_RXE_ADDR	(0x00000434)	/* field definition extended */
#define RXE_DMA_STCNT	(0x00000436)
#define RCV_STATUS_LEN	(0x00000438)	/* field definition extended */
#define RCV_SHM_STADDR	(0x0000043A)	/* field definition extended */
#define RCV_SHM_STCNT	(0x0000043C)	/* field definition extended */
#define RCV_RXFIFO_WRBURST	(0x0000043E)	/* rename */
#define RCV_PHYFIFO_WRDCNT	(0x00000440)	/* field definition extended */
#define RCV_WRD_CNT_Q0	(0x00000442)	/* rename */
#define RCV_WRD_CNT_Q1	(0x00000444)	/* rename */
#define RcvFifo0Len	(0x00000446)	/* rename */
#define RcvFifo1Len	(0x00000448)	/* rename */
#define RcvHdrConvSts	(0x0000044A)	/* rename */
#define RcvHdrConvSts1	(0x0000044C)	/* rename */
#define RCV_FRM_CNT	(0x0000044E)	/* field definition extended */
#define RCV_FRM_CNT_Q0	(0x00000450)	/* rename */
#define RCV_FRM_CNT_Q1	(0x00000452)	/* rename */
#define OMAC_HWSTS_L	(0x00000458)
#define OMAC_HWSTS_H	(0x0000045A)
#define RXBM_DBG_SEL	(0x0000045C)
#define RXBM_DBG_DATA	(0x0000045E)
#define OBFFCtl	(0x00000460)	/* field definition modified */
#define OBFFRxWordsWatermark	(0x00000462)	/* rename */
#define OBFFRxCntWatermark	(0x00000464)	/* rename */
#define LtrRxTimer	(0x00000466)	/* rename */
#define LtrRxWordsWatermark	(0x00000468)	/* rename */
#define LtrRxCntWatermark	(0x0000046A)	/* rename */
#define RoeCtrl	(0x0000046C)	/* rename */
#define RoeStatus	(0x0000046E)	/* rename */
#define RoeIPChkSum	(0x00000470)	/* rename */
#define RoeTCPUDPChkSum	(0x00000472)	/* rename */
#define RoeStatus1	(0x00000474)	/* rename */
#define RCV_BM_OVFL_DBGSEL	(0x00000478)	/* rename */
#define RCV_BM_ENDPTR_Q0	(0x0000047A)	/* rename */
#define RCV_BM_STARTPTR_Q1	(0x0000047C)	/* rename */
#define RCV_BM_ENDPTR_Q1	(0x0000047E)	/* rename */
#define MHP_Status	(0x00000480)	/* rename */
#define MHP_FC	(0x00000482)	/* rename */
#define MHP_DUR	(0x00000484)	/* rename */
#define MHP_SC	(0x00000486)	/* rename */
#define MHP_QOS	(0x00000488)	/* rename */
#define MHP_HTC_L	(0x0000048A)	/* rename */
#define MHP_HTC_H	(0x0000048C)	/* rename */
#define MHP_Addr1_L	(0x0000048E)	/* rename */
#define MHP_Addr1_M	(0x00000490)	/* rename */
#define MHP_Addr1_H	(0x00000492)	/* rename */
#define MHP_Addr2_L	(0x00000494)	/* rename */
#define MHP_Addr2_M	(0x00000496)	/* rename */
#define MHP_Addr2_H	(0x00000498)	/* rename */
#define MHP_Addr3_L	(0x0000049A)	/* rename */
#define MHP_Addr3_M	(0x0000049C)	/* rename */
#define MHP_Addr3_H	(0x0000049E)	/* rename */
#define MHP_Addr4_L	(0x000004A0)	/* rename */
#define MHP_Addr4_M	(0x000004A2)	/* rename */
#define MHP_Addr4_H	(0x000004A4)	/* rename */
#define MHP_CFC	(0x000004A6)	/* rename */
#define MHP_FCTP	(0x000004A8)
#define MHP_FCTST	(0x000004AA)
#define MHP_DFCTST	(0x000004AC)
#define MHP_EXT0	(0x000004AE)
#define MHP_EXT1	(0x000004B0)
#define MHP_EXT2	(0x000004B2)
#define MHP_EXT3	(0x000004B4)
#define MHP_PLCP0	(0x000004B6)
#define MHP_PLCP1	(0x000004B8)
#define MHP_PLCP2	(0x000004BA)
#define MHP_PLCP3	(0x000004BC)
#define MHP_PLCP4	(0x000004BE)
#define MHP_PLCP5	(0x000004C0)
#define MHP_PLCP6	(0x000004C2)
#define MHP_SEL	(0x000004C4)
#define MHP_DATA	(0x000004C6)
#define MHP_CFG		(0x000004C8)
#define RXE_RCF_CTL	(0x000004D0)
#define RXE_RCF_ADDR	(0x000004D2)
#define RXE_RCF_WDATA	(0x000004D4)
#define RXE_RCF_RDATA	(0x000004D6)
#define RXE_RCF_NP_STATS	(0x000004D8)
#define RXE_RCF_HP_STATS	(0x000004DA)
#define AMT_CTL	(0x000004E0)	/* rename */
#define AMT_Status	(0x000004E2)	/* rename */
#define RXQ_DBG_CTL		(0x00000514)
#define RXQ_DBG_STS		(0x00000516)
#define FP_CTL	(0x00000520)	/* rename-as-80 */
#define FP_CONFIG	(0x00000522)	/* rename-as-80 */
#define FP_STATUS	(0x00000524)	/* field definition extended from 80 */
#define FP_MASK	(0x00000526)	/* field definition extended from 80 */
#define FP_PAT0	(0x00000528)
#define FP_PAT1	(0x0000052A)
#define FP_PAT2	(0x0000052C)
#define FP_SUPAT	(0x0000052E)
#define RXE_FP_SHMADDR	(0x00000530)
#define TXBA_CTL	(0x00000540)	/* rename */
#define RXE_TXBA_CTL1	(0x00000542)
#define TXBA_DataSel	(0x00000544)	/* field definition extended */
#define TXBA_Data	(0x00000546)	/* rename */
#define MTID_STATUS	(0x00000548)
#define BA_LEN	(0x0000054A)
#define BA_LEN_ENC	(0x0000054C)
#define MULTITID_STATUS2	(0x0000054E)
#define BAINFO_SEL	(0x00000550)
#define BAINFO_DATA	(0x00000552)
#define TXBA_UBMP	(0x00000554)
#define TXBA_UCNT	(0x00000556)
#define TXBA_XFRBMP	(0x00000558)
#define TXBA_XFRCTL	(0x0000055A)
#define TXBA_XFRCNT	(0x0000055C)
#define TXBA_XFROFFS	(0x0000055E)
#define RDFBD_CTL0	(0x00000560)
#define RDF_CTL0	(0x00000562)
#define RDF_CTL1	(0x00000564)
#define RDF_CTL2	(0x00000566)
#define RDF_CTL3	(0x00000568)
#define RDF_CTL4	(0x0000056A)
#define RDF_CTL5	(0x0000056C)
#define RDF_STAT1	(0x0000056E)
#define RDF_STAT2	(0x00000570)
#define RDF_STAT3	(0x00000572)
#define RDF_STAT4	(0x00000574)
#define RDF_STAT5	(0x00000576)
#define RDF_STAT6	(0x00000578)
#define RDF_STAT7	(0x0000057A)
#define RDF_STAT8	(0x0000057C)
#define radioihrAddr	(0x00000586)	/* rename */
#define radioihrData	(0x00000588)	/* rename */
#define RXE_PHYSTS_SHMADDR	(0x00000592)
#define RXE_PHYSTS_BMP_SEL	(0x00000594)
#define RXE_PHYSTS_BMP_DATA	(0x00000596)
#define RXE_PHYSTS_TIMEOUT	(0x00000598)
#define RXE_PHYRS_ADDR	(0x000005A0)	/* field definition extended */
#define RXE_PHYRS_DATA	(0x000005A2)	/* rename */
#define RXE_PHYRS_0	(0x000005A4)	/* rename */
#define RXE_PHYRS_1	(0x000005A6)	/* rename */
#define RXE_PHYRS_2	(0x000005A8)	/* rename */
#define RXE_PHYRS_3	(0x000005AA)	/* rename */
#define RXE_PHYRS_4	(0x000005AC)	/* rename */
#define RXE_PHYRS_5	(0x000005AE)	/* rename */
#define RXE_OOB_CFG	(0x000005B0)
#define RXE_OOB_BMP0	(0x000005B2)
#define RXE_OOB_BMP1	(0x000005B4)
#define RXE_OOB_BMP2	(0x000005B6)
#define RXE_OOB_BMP3	(0x000005B8)
#define RXE_OOB_DSCR_ADDR	(0x000005BA)
#define RXE_OOB_DSCR_SIZE	(0x000005BC)
#define RXE_OOB_BUFA_ADDR	(0x000005BE)
#define RXE_OOB_BUFA_SIZE	(0x000005C0)
#define RXE_OOB_BUFB_ADDR	(0x000005C2)
#define RXE_OOB_BUFB_SIZE	(0x000005C4)
#define RXE_OOB_STATUS	(0x000005C6)
#define RXE_OOB_DESC_PTR	(0x000005C8)
#define RXE_OOB_BUFA_PTR	(0x000005CA)
#define RXE_OOB_BUFB_PTR	(0x000005CC)
#define RXE_BFDRPT_CTL	(0x000005CE)
#define RXE_BFDRPT_RST	(0x000005D0)
#define RXE_BFDRPT_LEN	(0x000005D2)
#define RXE_BFDRPT_OFFSET	(0x000005D4)
#define RXE_BFDRPT_MEND	(0x000005D6)
#define RXE_BFDRPT_XFER	(0x000005D8)
#define RXE_BFDRPT_SUCC	(0x000005DA)
#define RXE_BFDRPT_DONE	(0x000005DC)
#define RXE_BFM_HDRSEL	(0x000005DE)
#define RXE_BFM_HDRDATA	(0x000005E0)
#define DAGG_CTL2	(0x000005F0)	/* rename */
#define DAGG_BYTESLEFT	(0x000005F2)	/* rename */
#define DAGG_SH_OFFSET	(0x000005F4)	/* rename */
#define DAGG_STAT	(0x000005F6)	/* rename */
#define DAGG_LEN	(0x000005F8)	/* rename */
#define RCVLB_DAGG_CTL	(0x000005FA)
#define BTCX_CTL	(0x00000600)	/* rename */
#define BTCX_STAT	(0x00000602)	/* rename */
#define BTCX_TRANSCTL	(0x00000604)	/* rename */
#define BTCX_PRIORITYWIN	(0x00000606)	/* rename */
#define BTCX_TXCONFTIMER	(0x00000608)	/* rename */
#define BTCX_PRISELTIMER	(0x0000060A)	/* rename */
#define BTCX_PRV_RFACT_TIMER	(0x0000060C)	/* rename */
#define BTCX_CUR_RFACT_TIMER	(0x0000060E)	/* rename */
#define BTCX_RFACT_DUR_TIMER	(0x00000610)	/* rename */
#define ERCXControl	(0x00000612)	/* rename */
#define ERCXStatus	(0x00000614)	/* rename */
#define ERCXTransCtl	(0x00000616)	/* rename */
#define ERCXPriorityWin	(0x00000618)	/* rename */
#define ERCXConfTimer	(0x0000061A)	/* rename */
#define ERCX_PRISELTIMER	(0x0000061C)	/* rename */
#define ERCXPrvRfActTimer	(0x0000061E)	/* rename */
#define ERCXCurRfActTimer	(0x00000620)	/* rename */
#define ERCXActDurTimer	(0x00000622)	/* rename */
#define BTCX_ECI_ADDR	(0x00000624)	/* rename */
#define BTCX_ECI_DATA	(0x00000626)	/* rename */
#define BTCX_ECI_MASK_ADDR	(0x00000628)	/* rename */
#define BTCX_ECI_MASK_DATA	(0x0000062A)	/* rename */
#define COEX_IO_MASK	(0x0000062C)	/* rename */
#define NAV_CTL	(0x00000640)	/* rename */
#define NAV_STAT	(0x00000642)	/* rename */
#define NAV_CNTR_L	(0x00000644)	/* rename */
#define NAV_CNTR_H	(0x00000646)	/* rename */
#define NAV_TBTT_NOW_L	(0x00000648)	/* rename */
#define NAV_TBTT_NOW_H	(0x0000064A)	/* rename */
#define NAV_DUR	(0x0000064C)	/* rename */
#define NAV_DELTA_L	(0x0000064E)	/* rename */
#define NAV_DELTA_H	(0x00000650)	/* rename */
#define NAV_CTSTO	(0x00000652)	/* rename */
#define IFS_SIFS_RX_TX_TX	(0x00000680)	/* rename */
#define IFS_SIFS_NAV_TX	(0x00000682)	/* rename */
#define IFS_SLOT	(0x00000684)	/* rename */
#define IFS_EIFS	(0x00000686)	/* rename */
#define IFS_CTL	(0x00000688)	/* rename */
#define IFS_BOFF_CTR	(0x0000068A)	/* rename */
#define IFS_SLOT_CTR	(0x0000068C)	/* rename */
#define IFS_FREE_SLOTS	(0x0000068E)	/* rename */
#define IFS_STAT	(0x00000690)	/* rename */
#define IFS_MEDBUSY_CTR	(0x00000692)	/* rename */
#define IFS_MEDBUSY_CRS_CTR	(0x00000694)	/* rename */
#define IFS_RIFS_TIME	(0x00000696)	/* rename */
#define IFS_STAT1	(0x00000698)	/* rename */
#define IFS_EDCAPRI	(0x0000069A)	/* rename */
#define IFS_AIFSN	(0x0000069C)	/* rename */
#define IFS_CTL1	(0x0000069E)	/* rename */
#define IFS_CTL_SRC	(0x000006A0)	/* rename */
#define IFS_CTL_EDCRS	(0x000006A2)	/* rename */
#define IFS_CTL_OBSS	(0x000006A4)	/* rename */
#define IFS_CTL_SEL_PRICRS	(0x000006A6)	/* rename */
#define IFS_CTL_SECCRS	(0x000006A8)	/* rename */
#define IFS_STAT2	(0x000006AA)	/* rename */
#define IFS_STAT3	(0x000006AC)	/* rename */
#define IFS_STAT_CRS	(0x000006AE)	/* rename */
#define IFS_STAT_OBSSCRS	(0x000006B0)	/* rename */
#define IFS_EXTSTAT_CRS	(0x000006B2)	/* rename */
#define IFS_CHAN_SIFS	(0x000006B4)	/* rename */
#define IFS_CHAN_PDIFS	(0x000006B6)	/* rename */
#define IFS_CHAN_BOFF	(0x000006B8)	/* rename */
#define IFS_CCASTATS_EN (0x000006BA)	/* rename */
#define IFS_CCASTATS_ADDR (0x000006BC)	/* rename */
#define IFS_CCASTATS_DATA (0x000006BE)	/* rename */
#define IFS_TX_DUR	(0x000006C0)	/* rename */
#define IFS_RX_DUR	(0x000006C2)	/* rename */
#define IFS_OBSSPD_DUR	(0x000006C4)	/* rename */
#define IFS_RX_DUR_CHAN	(0x000006C6)	/* rename */
#define IFS_OBSSSTATS_EN (0x000006DA)	/* rename */
#define IFS_OBSSSTATS_ADDR (0x000006DC)	/* rename */
#define IFS_OBSSSTATS_DATA (0x000006DE)	/* rename */
#define IFS_DBGSEL	(0x000006E0)	/* rename */

#define FAST_PWRUP_DLY	(0x000006C8)	/* rename */
#define SLOW_CTL	(0x000006CA)	/* rename */
#define SLOW_TIMER_L	(0x000006CC)	/* rename */
#define SLOW_TIMER_H	(0x000006CE)	/* rename */
#define SLOW_FRAC	(0x000006D0)	/* rename */
#define SLOW_PER	(0x000006D2)	/* rename */
#define SLOW_PER_FRAC	(0x000006D4)	/* rename */
#define SLOW_CALTIMER_L	(0x000006D6)	/* rename */
#define SLOW_CALTIMER_H	(0x000006D8)	/* rename */
#define CMDPTR_L	(0x000006E0)
#define CMDPTR_H	(0x000006E2)
#define DATA_PTR_L	(0x000006E4)
#define DATA_PTR_H	(0x000006E6)
#define CTRL_STS	(0x000006E8)
#define PHY_ADDR	(0x000006EA)
#define PHY_DATA	(0x000006EC)
#define RADIO_ADDR	(0x000006EE)
#define RADIO_DATA	(0x000006F0)
#define RUNTIME_CNT	(0x000006F2)
#define TSF_CTL	(0x00000700)	/* rename */
#define TSF_STAT	(0x00000702)	/* rename */
#define TSF_CFP_STRT_L	(0x00000704)	/* rename */
#define TSF_CFP_STRT_H	(0x00000706)	/* rename */
#define TSF_CFP_END_L	(0x00000708)	/* rename */
#define TSF_CFP_END_H	(0x0000070A)	/* rename */
#define TSF_CFP_MAX_DUR	(0x0000070C)	/* rename */
#define TSF_CFP_REP_L	(0x0000070E)	/* rename */
#define TSF_CFP_REP_H	(0x00000710)	/* rename */
#define TSF_CFP_PRE_TBTT	(0x00000712)	/* rename */
#define TSF_CFP_CFP_D0_L	(0x00000714)	/* rename */
#define TSF_CFP_CFP_D0_H	(0x00000716)	/* rename */
#define TSF_CFP_CFP_D1_L	(0x00000718)	/* rename */
#define TSF_CFP_CFP_D1_H	(0x0000071A)	/* rename */
#define TSF_CFP_CFP_D2_L	(0x0000071C)	/* rename */
#define TSF_CFP_CFP_D2_H	(0x0000071E)	/* rename */
#define TSF_CFP_TXOP_SQS_L	(0x00000720)	/* rename */
#define TSF_CFP_TXOP_SQS_H	(0x00000722)	/* rename */
#define TSF_CFP_TXOP_PQS	(0x00000724)	/* rename */
#define TSF_CFP_TXOP_SQD_L	(0x00000726)	/* rename */
#define TSF_CFP_TXOP_SQD_H	(0x00000728)	/* rename */
#define TSF_CFP_TXOP_PQD	(0x0000072A)	/* rename */
#define TSF_FES_DUR	(0x0000072C)	/* rename */
#define TSF_CLK_FRAC_L	(0x0000072E)	/* rename */
#define TSF_CLK_FRAC_H	(0x00000730)	/* rename */
#define TSF_TMR_TSF_L	(0x00000732)	/* rename */
#define TSF_TMR_TSF_ML	(0x00000734)	/* rename */
#define TSF_TMR_TSF_MU	(0x00000736)	/* rename */
#define TSF_TMR_TSF_H	(0x00000738)	/* rename */
#define TSF_TMR_TX_OFFSET	(0x0000073A)	/* rename */
#define TSF_TMR_RX_OFFSET	(0x0000073C)	/* rename */
#define TSF_TMR_RX_TS	(0x0000073E)	/* rename */
#define TSF_TMR_TX_TS	(0x00000740)	/* rename */
#define TSF_TMR_RX_END_TS	(0x00000742)	/* rename */
#define TSF_TMR_DELTA	(0x00000744)	/* rename */
#define TSF_RANDOM	(0x00000746)	/* rename */
#define RAND_SEED_0	(0x00000748)	/* rename */
#define RAND_SEED_1	(0x0000074A)	/* rename */
#define RAND_SEED_2	(0x0000074C)	/* rename */
#define TSF_ADJUST	(0x0000074E)	/* rename */
#define TSF_PHY_HDR_TM	(0x00000750)	/* rename */
#define TSF_TMR_TX_ERR_TS	(0x0000075E)	/* rename */
#define TSF_GPT_0_STAT	(0x00000760)	/* rename */
#define TSF_GPT_0_CTR_L	(0x00000762)	/* rename */
#define TSF_GPT_0_CTR_H	(0x00000764)	/* rename */
#define TSF_GPT_0_VAL_L	(0x00000766)	/* rename */
#define TSF_GPT_0_VAL_H	(0x00000768)	/* rename */
#define TSF_GPT_1_STAT	(0x0000076A)	/* rename */
#define TSF_GPT_1_CTL_L	(0x0000076C)	/* rename */
#define TSF_GPT_1_CTL_H	(0x0000076E)	/* rename */
#define TSF_GPT_1_VAL_L	(0x00000770)	/* rename */
#define TSF_GPT_1_VAL_H	(0x00000772)	/* rename */
#define TSF_GPT_2_STAT	(0x00000774)	/* rename */
#define TSF_GPT_2_CTR_L	(0x00000776)	/* rename */
#define TSF_GPT_2_CTR_H	(0x00000778)	/* rename */
#define TSF_GPT_2_VAL_L	(0x0000077A)	/* rename */
#define TSF_GPT_2_VAL_H	(0x0000077C)	/* rename */
#define TSF_GPT_ALL_STAT	(0x0000077E)	/* rename */
#define PSM_SLEEP_TMR	(0x00000780)	/* rename */
#define PSM_MAC_CTLH	(0x00000782)	/* rename */
#define PSM_MAC_INTSTAT_L	(0x00000784)	/* rename */
#define PSM_MAC_INTSTAT_H	(0x00000786)	/* rename */
#define PSM_MAC_INTMASK_L	(0x00000788)	/* rename */
#define PSM_MAC_INTMASK_H	(0x0000078A)	/* rename */
#define PSM_MACCOMMAND	(0x0000078C)	/* rename */
#define PSM_MUL	(0x0000078E)	/* rename */
#define PSM_MACCONTROL1	(0x00000790)	/* rename */
#define PSM_ALT_MAC_INTSTATUS_L	(0x00000792)	/* rename */
#define PSM_ALT_MAC_INTSTATUS_H	(0x00000794)	/* rename */
#define PSM_ALT_MAC_INTMASK_L	(0x00000796)	/* rename */
#define PSM_ALT_MAC_INTMASK_H	(0x00000798)	/* rename */
#define PsmMboxAddr	(0x0000079A)	/* rename */
#define PsmMboxData	(0x0000079C)	/* rename */
#define PSMSrchCtrlStatus	(0x000007A2)	/* rename */
#define PSMSrchBase	(0x000007A4)	/* rename */
#define PSMSrchLimit	(0x000007A6)	/* rename */
#define PSMSrchAddress	(0x000007A8)	/* rename */
#define PSMSrchData	(0x000007AA)	/* rename */
#define PSM_REG_MUX	(0x000007B0)	/* rename */
#define HWA_MACIF_CTL	(0x000007B2)
#define PSM_DIV_REM_L	(0x000007BC)
#define PSM_DIV_REM_H	(0x000007BE)
#define PSM_PHY_CTL	(0x000007C0)	/* rename */
#define PSM_MACCTL	(0x000007C2)
#define PSM_ERR_PC	(0x000007C4)	/* rename */
#define PSM_IHR_ERR	(0x000007C6)	/* rename */
#define PSM_STAT_CTR0_L	(0x000007C8)	/* rename */
#define PSM_STAT_CTR0_H	(0x000007CA)	/* rename */
#define PSM_STAT_CTR1_L	(0x000007CC)	/* rename */
#define PSM_STAT_CTR1_H	(0x000007CE)	/* rename */
#define PSM_STAT_SEL	(0x000007D0)	/* rename */
#define SubrStkStatus	(0x000007D2)	/* rename */
#define SubrStkRdPtr	(0x000007D4)	/* rename */
#define SubrStkRdData	(0x000007D6)	/* rename */
#define PSM_SBADDR	(0x000007E2)
#define SBRegDataL	(0x000007E4)	/* rename */
#define SBRegDataH	(0x000007E6)	/* rename */
#define PSMCoreCtlStat	(0x000007E8)	/* rename */
#define PSMWorkaround	(0x000007EA)	/* rename */
#define SbAddrLL	(0x000007EC)	/* rename */
#define SbAddrL		(0x000007EE)	/* rename */
#define SbAddrH		(0x000007F0)	/* rename */
#define SbAddrHH	(0x000007F2)	/* rename */
#define PSM_TXMEM_PDA	(0x000007F4)	/* rename */
#define PSM_BRED_0	(0x00000800)	/* rename */
#define PSM_BRED_1	(0x00000802)	/* rename */
#define PSM_BRED_2	(0x00000804)	/* rename */
#define PSM_BRED_3	(0x00000806)	/* rename */
#define PSM_BRCL_0	(0x00000808)	/* rename */
#define PSM_BRCL_1	(0x0000080A)	/* rename */
#define PSM_BRCL_2	(0x0000080C)	/* rename */
#define PSM_BRCL_3	(0x0000080E)	/* rename */
#define PSM_BRPO_0	(0x00000810)	/* rename */
#define PSM_BRPO_1	(0x00000812)	/* rename */
#define PSM_BRPO_2	(0x00000814)	/* rename */
#define PSM_BRPO_3	(0x00000816)	/* rename */
#define PSM_BRWK_0	(0x00000818)	/* rename */
#define PSM_BRWK_1	(0x0000081A)	/* rename */
#define PSM_BRWK_2	(0x0000081C)	/* rename */
#define PSM_BRWK_3	(0x0000081E)	/* rename */
#define PSM_INTSEL_0	(0x00000820)	/* rename */
#define PSM_INTSEL_1	(0x00000822)	/* rename */
#define PSM_INTSEL_2	(0x00000824)	/* rename */
#define PSM_INTSEL_3	(0x00000826)	/* rename */
#define PSM_BRC_SEL	(0x00000828)	/* rename */
#define PSM_BRC_SEL_1	(0x0000082A)	/* rename */
#define PSM_BRC_0	(0x00000830)	/* rename */
#define PSM_BRC_1	(0x00000832)	/* rename */
#define PSM_BRWK_4	(0x0000083A)
#define PSM_BASE_0	(0x00000840)	/* rename */
#define PSM_BASE_1	(0x00000842)	/* rename */
#define PSM_BASE_2	(0x00000844)	/* rename */
#define PSM_BASE_3	(0x00000846)	/* rename */
#define PSM_BASE_4	(0x00000848)	/* rename */
#define PSM_BASE_5	(0x0000084A)	/* rename */
#define PSM_BASE_6	(0x0000084C)	/* rename */
#define PSM_BASE_7	(0x0000084E)	/* rename */
#define PSM_BASE_8	(0x00000850)	/* rename */
#define PSM_BASE_9	(0x00000852)	/* rename */
#define PSM_BASE_10	(0x00000854)	/* rename */
#define PSM_BASE_11	(0x00000856)	/* rename */
#define PSM_BASE_12	(0x00000858)	/* rename */
#define PSM_BASE_13	(0x0000085A)	/* rename */
#define PSM_BASE_14	(0x0000085C)
#define PSM_BASE_15	(0x0000085E)
#define PSM_BASE_16	(0x00000860)
#define PSM_BASE_17	(0x00000862)
#define PSM_BASE_18	(0x00000864)
#define PSM_BASE_19	(0x00000866)
#define PSM_BASE_20	(0x00000868)
#define PSM_BASE_21	(0x0000086A)
#define PSM_BASE_22	(0x0000086C)
#define PSM_BASE_23	(0x0000086E)
#define PSM_LINKMEM_CTL	(0x0000087A)
#define PSM_LINKBLK_SIZE	(0x0000087C)
#define PSM_LINKXFER_SIZE	(0x0000087E)
#define PSM_GPIOIN	(0x00000880)	/* rename */
#define PSM_GPIOOUT	(0x00000882)	/* rename */
#define PSM_GPIOEN	(0x00000884)	/* rename */
#define MAC_GPIOOUT_L	(0x00000886)
#define MAC_GPIOOUT_H	(0x00000888)
#define WEP_CTL	(0x000008A0)	/* rename */
#define WEP_STAT	(0x000008A2)	/* rename */
#define WEP_HDRLOC	(0x000008A4)	/* rename */
#define WEP_PSDULEN	(0x000008A6)	/* rename */
#define WEP_KEY_ADDR	(0x000008A8)	/* rename */
#define WEP_KEY_DATA	(0x000008AA)	/* rename */
#define WEP_ADDR	(0x000008AC)	/* rename */
#define WEP_DATA	(0x000008AE)	/* rename */
#define WEP_HWKEY_ADDR	(0x000008B0)	/* rename */
#define WEP_HWKEY_LEN	(0x000008B2)	/* rename */
#define WEP_HWMICK_ADDR	(0x000008B4)	/* rename */
#define WEP_HWMICK_LEN	(0x000008B6)	/* rename */
#define PMQ_CTL	(0x000008C0)	/* field definition modified */
#define PMQ_CNT	(0x000008C2)	/* rename */
#define PMQ_SRCH_USREN	(0x000008C4)	/* rename */
#define PMQ_USR_SEL	(0x000008C6)	/* rename */
#define PMQ_STATUS	(0x000008C8)	/* rename */
#define PMQ_PAT_0	(0x000008CA)	/* rename */
#define PMQ_PAT_1	(0x000008CC)	/* rename */
#define PMQ_PAT_2	(0x000008CE)	/* rename */
#define PMQ_DAT	(0x000008D0)	/* rename */
#define PMQ_DAT_OR_MAT	(0x000008D2)	/* rename */
#define PMQ_DAT_OR_ALL	(0x000008D4)	/* rename */
#define PMQ_MATCH	(0x000008D6)
#define APMQ_MATCH	(0x000008D8)
#define PMQ_AUXPMQ_STATUS	(0x000008DA)	/* rename */
#define PMQ_STATUS1	(0x000008DC)	/* field definition extended */
#define PMQ_ADDTHR	(0x000008DE)	/* rename */
#define TXE_CTL	(0x00000900)	/* rename */
#define TXE_CTL1	(0x00000902)	/* rename */
#define TXE_CTL2	(0x00000904)	/* field definition extended */
#define TXE_AUX	(0x00000906)	/* field definition modified */
#define TXE_TS_LOC	(0x00000908)	/* rename */
#define TXE_TIME_OUT	(0x0000090A)	/* rename */
#define TXE_WM_0	(0x0000090C)	/* rename */
#define TXE_WM_1	(0x0000090E)	/* rename */
#define TXE_FCS_CTL	(0x00000910)
#define MacHdrFromShmLen	(0x00000912)	/* rename */
#define AMPDU_CTL	(0x00000914)
#define AMPDU_CRC	(0x00000916)
#define TXE_WM_LEG0	(0x00000918)
#define TXE_WM_LEG1	(0x0000091A)
#define TXE_CTL3	(0x0000091C)
#define TXE_EARLYTXMEND_CNT	(0x0000091E)
#define TXE_EARLYTXMEND_BSUB_CNT	(0x00000920)
#define TXE_TXMEND_CNT	(0x00000922)
#define TXE_TXMEND_NCONS	(0x00000924)
#define TXE_TXMEND_PEND	(0x00000926)
#define TXE_TXMEND_USR2GO	(0x00000928)
#define TXE_TXMEND_CONSCNT	(0x0000092A)
#define TXE_STATUS	(0x0000092C)	/* rename */
#define TXE_STATUS2	(0x0000092E)	/* rename */
#define TXE_STATUS1	(0x00000930)	/* rename */
#define TXE_FRMSTAT_ADDR	(0x00000932)	/* rename */
#define TXE_FRMSTAT_DATA	(0x00000934)	/* rename */
#define AVB_TXTIMESTAMP_L	(0x00000936)	/* rename */
#define AVB_TXTIMESTAMP_H	(0x00000938)	/* rename */
#define PhyDebugL	(0x0000093A)	/* rename */
#define PhyDebugH	(0x0000093C)	/* rename */
#define TXE_BVBM_INIT	(0x0000093E)
#define CTMode	(0x00000940)	/* rename */
#define ToECTL	(0x00000942)	/* rename */
#define ToERst	(0x00000944)	/* rename */
#define ToECSumNZ	(0x00000946)	/* rename */
#define ToEChannelState	(0x00000948)
#define BMC_REGSEL	(0x0000094A)	/* field definition extended */
#define SERIAL_REGSEL	(0x0000094C)	/* field definition extended */
#define WEP_REGSEL	(0x0000094E)	/* field definition extended */
#define TDC_REGSEL	(0x00000950)	/* field definition extended */
#define TXE_BV_REG0	(0x00000952)
#define TXE_BM_REG0	(0x00000954)
#define TXE_BV_REG1	(0x00000956)
#define TXE_BM_REG1	(0x00000958)
#define ShmDma_Ctl	(0x00000960)	/* field definition extended */
#define ShmDma_TxdcAddr	(0x00000962)	/* rename */
#define ShmDma_ShmAddr	(0x00000964)	/* field definition extended */
#define ShmDma_XferWCnt	(0x00000966)	/* rename */
#define V2M_MAXWCNT	(0x0000096C)	/* rename */
#define TXE_VASIP_INTSTS	(0x0000096E)	/* field definition extended */
#define TXE_SHMDMA_MPMADDR	(0x00000970)
#define TXE_BITSUB_IDX	(0x00000976)
#define TXE_BM_ADDR	(0x00000978)
#define TXE_BM_DATA	(0x0000097A)
#define TXE_BV_ADDR	(0x0000097C)
#define TXE_BV_DATA	(0x0000097E)
#define TXE_BM_0	(0x00000980)	/* rename */
#define TXE_BM_1	(0x00000982)	/* rename */
#define TXE_BM_2	(0x00000984)	/* rename */
#define TXE_BM_3	(0x00000986)	/* rename */
#define TXE_BM_4	(0x00000988)	/* rename */
#define TXE_BM_5	(0x0000098A)	/* rename */
#define TXE_BM_6	(0x0000098C)	/* rename */
#define TXE_BM_7	(0x0000098E)	/* rename */
#define TXE_BM_8	(0x00000990)	/* rename */
#define TXE_BM_9	(0x00000992)	/* rename */
#define TXE_BM_10	(0x00000994)	/* rename */
#define TXE_BM_11	(0x00000996)	/* rename */
#define TXE_BM_12	(0x00000998)	/* rename */
#define TXE_BM_13	(0x0000099A)	/* rename */
#define TXE_BM_14	(0x0000099C)	/* rename */
#define TXE_BM_15	(0x0000099E)	/* rename */
#define TXE_BV_0	(0x000009A0)	/* rename */
#define TXE_BV_1	(0x000009A2)	/* rename */
#define TXE_BV_2	(0x000009A4)	/* rename */
#define TXE_BV_3	(0x000009A6)	/* rename */
#define TXE_BV_4	(0x000009A8)	/* rename */
#define TXE_BV_5	(0x000009AA)	/* rename */
#define TXE_BV_6	(0x000009AC)	/* rename */
#define TXE_BV_7	(0x000009AE)	/* rename */
#define TXE_BV_8	(0x000009B0)	/* rename */
#define TXE_BV_9	(0x000009B2)	/* rename */
#define TXE_BV_10	(0x000009B4)
#define TXE_BV_11	(0x000009B6)
#define TXE_BV_12	(0x000009B8)
#define TXE_BV_13	(0x000009BA)
#define TXE_BV_14	(0x000009BC)
#define TXE_BV_15	(0x000009BE)
#define PsmMSDUAccess	(0x000009C0)	/* field definition modified */
#define MSDUEntryBufCnt	(0x000009C2)	/* field definition modified */
#define MSDUEntryStartIdx	(0x000009C4)	/* field definition extended */
#define MSDUEntryEndIdx	(0x000009C6)	/* field definition extended */
#define BMCCmd1 (0x000009C8)
#define BMCDynAllocStatus	(0x000009CA)	/* field definition modified */
#define BMCCTL	(0x000009CC)	/* rename */
#define TXE_BMC_CONFIG	(0x000009CE)
#define BMCStartAddr	(0x000009D0)	/* field definition modified */
#define BMCCmd	(0x000009D2)
#define BMCMaxBuffers	(0x000009D4)	/* field definition extended */
#define BMCMinBuffers	(0x000009D6)	/* field definition extended */
#define BMCAllocCtl	(0x000009D8)	/* rename */
#define BMCDescrLen	(0x000009DA)	/* rename */
#define XmtFIFOFullThreshold	(0x000009E0)	/* rename */
#define MSDUIdxFifoConfig	(0x000009E2)	/* field definition extended */
#define MSDUIdxFifoDef	(0x000009E4)	/* rename */
#define BMCCore0TXAllMaxBuffers	(0x000009E6)	/* field definition extended */
#define TXE_BMC_MAXOUTSTBUFS	(0x000009EC)
#define TXE_BMC_CONFIG1	(0x000009EE)
#define XmtFifoRqPrio	(0x000009F0)	/* field definition modified */
#define Core0BMCAllocStatusTplate	(0x000009F2)	/* field definition extended */
#define TXE_LLM_CONFIG	(0x000009F6)
#define BMVpConfig	(0x000009F8)	/* rename */
#define BMCCutThruConfig	(0x000009FA)	/* rename */
#define BMCCutThruTestCfg0	(0x000009FC)	/* rename */
#define BMCCutThruTestCfg1	(0x000009FE)	/* rename */
#define BMCCutThruTestCfg2	(0x00000A00)	/* rename */
#define BMCCutThruTestCfg3	(0x00000A02)	/* rename */
#define BMCCutThruTestCfg4	(0x00000A04)	/* rename */
#define SysMStartAddrLo	(0x00000A06)	/* rename */
#define SysMStartAddrHi	(0x00000A08)	/* rename */
#define TXE_LOCALM_SADDR_L	(0x00000A0A)
#define TXE_LOCALM_SADDR_H	(0x00000A0C)
#define TXE_LOCALM_EADDR_L	(0x00000A0E)
#define TXE_LOCALM_EADDR_H	(0x00000A10)
#define TXE_BMC_ALLOCCTL1	(0x00000A12)
#define TXE_BMC_MALLOCREQ_QB0	(0x00000A14)
#define TXE_BMC_MALLOCREQ_QB1	(0x00000A16)
#define TXE_BMC_MALLOCREQ_QB2	(0x00000A18)
#define TXE_BMC_MALLOCREQ_QB3	(0x00000A1A)
#define TXE_BMC_MALLOCREQ_QB4	(0x00000A1C)
#define SMP_CTRL2		(0x00000A28)
#define SCTSTP_MASK_L		(0x00000A2A)
#define SCTSTP_MASK_H		(0x00000A2C)
#define SCTSTP_VAL_L		(0x00000A2E)
#define SCTSTP_VAL_H		(0x00000A30)
#define SCTSTP_HMASK_L		(0x00000A32)
#define SCTSTP_HMASK_H		(0x00000A34)
#define SCTSTP_HVAL_L		(0x00000A36)
#define SCTSTP_HVAL_H		(0x00000A38)
#define PHYCTL_LEN	(0x00000A40)
#define TXE_PHYCTL	(0x00000A42)	/* rename */
#define TXE_PHYCTL_1	(0x00000A44)	/* rename */
#define TXE_PHYCTL_2	(0x00000A46)	/* rename */
#define TXE_PHYCTL_3	(0x00000A48)	/* rename-as-80 */
#define TXE_PHYCTL_4	(0x00000A4A)	/* rename-as-80 */
#define TXE_PHYCTL_5	(0x00000A4C)	/* rename-as-80 */
#define TXE_PHYCTL_6	(0x00000A4E)	/* rename-as-80 */
#define TXE_PHYCTL_7	(0x00000A50)	/* rename-as-80 */
#define TXE_PHYCTL_8	(0x00000A52)	/* rename-as-80 */
#define TXE_PHYCTL_9	(0x00000A54)	/* rename-as-80 */
#define TXE_PHYCTL_10	(0x00000A56)
#define PLCP_LSIG0	(0x00000A58)
#define PLCP_LSIG1	(0x00000A5A)
#define PLCP_HTSIG0	(0x00000A5C)
#define PLCP_HTSIG1	(0x00000A5E)
#define PLCP_HTSIG2	(0x00000A60)
#define TXE_PLCP_VHTSIGB0	(0x00000A62)
#define TXE_PLCP_VHTSIGB1	(0x00000A64)
#define PLCP_EXT2	(0x00000A66)
#define PLCP_CC1_LEN	(0x00000A68)
#define PLCP_CC2_LEN	(0x00000A6A)
#define TXE_MPMSIZE_SEL	(0x00000A6C)
#define TXE_MPMSIZE_VAL	(0x00000A6E)
#define TXE_PLCPEXT_ADDR	(0x00000A70)
#define TXE_PLCPEXT_DATA	(0x00000A72)
#define TXE_PHYCTLEXT_BASE	(0x00000A74)
#define TXE_PLCPEXT_BASE	(0x00000A76)
#define TXE_SIGB_BASE	(0x00000A78)
#define BytCntInTxFrmLo	(0x00000A7C)	/* rename */
#define BytCntInTxFrmHi	(0x00000A7E)	/* field definition extended */
#define TXBFCtl	(0x00000A80)	/* rename */
#define TxBFRptLen	(0x00000A82)	/* rename */
#define TXBFBfeRptRdCnt	(0x00000A84)	/* rename */
#define TXE_TXDBG_SEL	(0x00000A86)
#define TXE_TXDBG_DATA	(0x00000A88)
#define TXE_BFMRPT_MEMSEL	(0x00000A8A)
#define TXE_BFMRPT_ADDR	(0x00000A8C)
#define TXE_BFMRPT_DATA	(0x00000A8E)
#define TXE_MEND_STATUS	(0x00000A90)
#define TXE_UNFLOW_STATUS	(0x00000A92)
#define TXE_TXERROR_STATUS	(0x00000A94)
#define TXE_SNDFRM_STATUS	(0x00000A96)
#define TXE_TXERROR_USR	(0x00000A98)
#define TXE_MACPAD_PAT_L	(0x00000A9A)
#define TXE_MACPAD_PAT_H	(0x00000A9C)
#define TXE_PHYTXREQ_TMOUT	(0x00000A9E)
#define SMP_PTR_H	(0x00000AA0)	/* rename */
#define SCP_CURPTR	(0x00000AA2)	/* rename */
#define SCP_CURPTR_H	(0x00000AA4)	/* field definition extended */
#define SCP_STRTPTR	(0x00000AA6)	/* rename */
#define SCP_STOPPTR	(0x00000AA8)	/* rename */
#define SPP_STRTPTR	(0x00000AAA)	/* rename */
#define SPP_STOPPTR	(0x00000AAC)	/* rename */
#define SCS_MASK_L	(0x00000AAE)	/* rename */
#define SCS_MASK_H	(0x00000AB0)	/* rename */
#define SCM_MASK_L	(0x00000AB2)	/* rename */
#define SCM_MASK_H	(0x00000AB4)	/* rename */
#define SCM_VAL_L	(0x00000AB6)	/* rename */
#define SCM_VAL_H	(0x00000AB8)	/* rename */
#define SCT_MASK_L	(0x00000ABA)	/* rename */
#define SCT_MASK_H	(0x00000ABC)	/* rename */
#define SCT_VAL_L	(0x00000ABE)	/* rename */
#define SCT_VAL_H	(0x00000AC0)	/* rename */
#define SCX_MASK_L	(0x00000AC2)	/* rename */
#define SCX_MASK_H	(0x00000AC4)	/* rename */
#define SCS_HMASK_L	(0x00000AC6)	/* rename */
#define SCS_HMASK_H	(0x00000AC8)	/* rename */
#define SCM_HMASK_L	(0x00000ACA)	/* rename */
#define SCM_HMASK_H	(0x00000ACC)	/* rename */
#define SCM_HVAL_L	(0x00000ACE)	/* rename */
#define SCM_HVAL_H	(0x00000AD0)	/* rename */
#define SCT_HMASK_L	(0x00000AD2)	/* rename */
#define SCT_HMASK_H	(0x00000AD4)	/* rename */
#define SCT_HVAL_L	(0x00000AD6)	/* rename */
#define SCT_HVAL_H	(0x00000AD8)	/* rename */
#define SCX_HMASK_L	(0x00000ADA)	/* rename */
#define SCX_HMASK_H	(0x00000ADC)	/* rename */
#define SMP_CTRL	(0x00000ADE)	/* rename */
#define XmtTemplateDataLo	(0x00000AE0)	/* rename */
#define XmtTemplateDataHi	(0x00000AE2)	/* rename */
#define XmtTemplatePtr	(0x00000AE4)	/* rename */
#define XmtTemplatePtrOffset	(0x00000AE6)	/* rename */
#define TXE_XMTSUSP_QB0	(0x00000AE8)
#define TXE_XMTSUSP_QB1	(0x00000AEA)
#define TXE_XMTSUSP_QB2	(0x00000AEC)
#define TXE_XMTSUSP_QB3	(0x00000AEE)
#define TXE_XMTSUSP_QB4	(0x00000AF0)
#define TXE_XMTFLUSH_QB0	(0x00000AF2)
#define TXE_XMTFLUSH_QB1	(0x00000AF4)
#define TXE_XMTFLUSH_QB2	(0x00000AF6)
#define TXE_XMTFLUSH_QB3	(0x00000AF8)
#define TXE_XMTFLUSH_QB4	(0x00000AFA)
#define TXE_XMT_SHMADDR	(0x00000AFC)
#define TXE_BMC_READQID	(0x00000AFE)
#define BMCReadReq	(0x00000B00)	/* field definition modified */
#define TXE_BMC_READIDX	(0x00000B02)
#define BMCReadOffset	(0x00000B04)	/* field definition extended */
#define BMCReadLength	(0x00000B06)	/* rename */
#define BMCReadStatus	(0x00000B08)	/* field definition modified */
#define BMC_AQMBQID	(0x00000B0A)
#define BMC_PSMCMD_THRESH	(0x00000B0C)
#define BMC_PSMCMD_LOWVEC	(0x00000B0E)
#define BMC_PSMCMD_EMPTYVEC	(0x00000B10)
#define BMC_PSMCMD_OFLOW	(0x00000B12)
#define BMC_CMD2SCHED_PEND	(0x00000B14)
#define BMC_AQM_TXDRD_PEND	(0x00000B16)
#define BMC_UC_TXDRD_PEND	(0x00000B18)
#define BMC_CMDQ_FREECNT	(0x00000B1A)
#define BMC_CMDQ_FREESTS	(0x00000B1C)
#define BMC_CMDQ_FRMCNT	(0x00000B1E)
#define BMC_FRM2SER_PEND	(0x00000B20)
#define BMC_FRM2SER_PRGR	(0x00000B22)
#define BMC_FRM2MPM_PEND	(0x00000B24)
#define BMC_FRM2MPM_PENDSTS	(0x00000B26)
#define BMC_FRM2MPM_PRGR	(0x00000B28)
#define BMC_BITSUB_FREECNT	(0x00000B2A)
#define BMC_BITSUB_FREESTS	(0x00000B2C)
#define BMC_CMDQ_USR2GO	(0x00000B2E)
#define BMC_BITSUB_USR2GO	(0x00000B30)
#define TXE_BRC_CMDQ_FREEUP	(0x00000B32)
#define TXE_BRC_FRM2MPM_PEND	(0x00000B34)
#define BMC_PSMCMD0	(0x00000B36)
#define BMC_PSMCMD1	(0x00000B38)
#define BMC_PSMCMD2	(0x00000B3A)
#define BMC_PSMCMD3	(0x00000B3C)
#define BMC_PSMCMD4	(0x00000B3E)
#define BMC_PSMCMD5	(0x00000B40)
#define BMC_PSMCMD6	(0x00000B42)
#define BMC_PSMCMD7	(0x00000B44)
#define BMC_RDCLIENT_CTL	(0x00000B46)
#define BMC_RDMGR_USR_RST	(0x00000B48)
#define BMC_BMRD_INFLIGHT_THRESH	(0x00000B4A)
#define BMC_BITSUB_CAP	(0x00000B4C)
#define BMC_ERR	(0x00000B4E)
#define TXE_SCHED_USR_RST	(0x00000B50)
#define TXE_SCHED_SET_UFL	(0x00000B52)
#define TXE_SCHED_UFL_WAIT	(0x00000B54)
#define BMC_SCHED_UFL_NOCMD	(0x00000B56)
#define BMC_CMD2SCHED_PENDSTS	(0x00000B58)
#define BMC_PSMCMD_RST	(0x00000B5A)
#define TXE_SCHED_SENT_L	(0x00000B5C)
#define TXE_SCHED_SENT_H	(0x00000B5E)
#define TXE_SCHED_CTL	(0x00000B60)
#define TXE_MSCHED_USR_EN	(0x00000B62)
#define TXE_MSCHED_SYMB_CYCS	(0x00000B64)
#define TXE_SCHED_UFL_STS	(0x00000B66)
#define TXE_MSCHED_BURSTPH_TOTSZ	(0x00000B68)
#define TXE_MSCHED_BURSTPH_BURSTSZ	(0x00000B6A)
#define TXE_MSCHED_NDPBS_L	(0x00000B6C)
#define TXE_MSCHED_NDPBS_H	(0x00000B6E)
#define TXE_MSCHED_STATE	(0x00000B70)
#define TXE_FRMINPROG_QB0	(0x00000B74)
#define TXE_FRMINPROG_QB1	(0x00000B76)
#define TXE_FRMINPROG_QB2	(0x00000B78)
#define TXE_FRMINPROG_QB3	(0x00000B7A)
#define TXE_FRMINPROG_QB4	(0x00000B7C)
#define TXE_XMT_DMABUSY_QB0	(0x00000B7E)
#define TXE_XMT_DMABUSY_QB1	(0x00000B80)
#define TXE_XMT_DMABUSY_QB2	(0x00000B82)
#define TXE_XMT_DMABUSY_QB3	(0x00000B84)
#define TXE_XMT_DMABUSY_QB4	(0x00000B86)
#define TXE_BMC_FIFOFULL_QB0	(0x00000B88)
#define TXE_BMC_FIFOFULL_QB1	(0x00000B8A)
#define TXE_BMC_FIFOFULL_QB2	(0x00000B8C)
#define TXE_BMC_FIFOFULL_QB3	(0x00000B8E)
#define TXE_BMC_FIFOFULL_QB4	(0x00000B90)
#define TXE_XMT_DPQSL_QB0	(0x00000B92)
#define TXE_XMT_DPQSL_QB1	(0x00000B94)
#define TXE_XMT_DPQSL_QB2	(0x00000B96)
#define TXE_XMT_DPQSL_QB3	(0x00000B98)
#define TXE_XMT_DPQSL_QB4	(0x00000B9A)
#define TXE_AQM_BQSTATUS	(0x00000B9C)
#define BMCStatCtl	(0x00000B9E)	/* field definition modified */
#define BMCStatData	(0x00000BA0)	/* rename */
#define BMC_MUDebugConfig	(0x00000BA2)	/* rename */
#define BMC_MUDebugStatus	(0x00000BA4)	/* rename */
#define BMCBQCutThruSt0	(0x00000BA6)	/* rename */
#define BMCBQCutThruSt1	(0x00000BA8)	/* field definition extended */
#define BMCBQCutThruSt2	(0x00000BAA)	/* rename */
#define TXE_DBG_BMC_STATUS	(0x00000BAC)	/* field definition modified */
#define DBG_LLMCFG	(0x00000BAE)	/* field definition extended */
#define DBG_LLMSTS	(0x00000BB0)	/* field definition extended */
#define DebugBusCtrl	(0x00000BB2)	/* rename */
#define DebugTxFlowCtl	(0x00000BB4)	/* rename */
#define TXE_HDR_PDLIM	(0x00000BB6)
#define TXE_MAC_PADBYTES	(0x00000BB8)
#define TXE_MPDU_MINLEN	(0x00000BBA)
#define TXE_AMP_EOFPD_L	(0x00000BBC)
#define TXE_AMP_EOFPD_H	(0x00000BBE)
#define TXE_AMP_EOFPADBYTES	(0x00000BC0)
#define TXE_PSDULEN_L	(0x00000BC2)
#define TXE_PSDULEN_H	(0x00000BC4)
#define XMTDMA_CTL	(0x00000BC8)
#define TXE_XMTDMA_ACT_RANGE	(0x00000BCC)
#define TXE_XMTDMA_RUWT_QSEL	(0x00000BCE)
#define TXE_XMTDMA_RUWT_DATA	(0x00000BD0)
#define TXE_CTDMA_CTL	(0x00000BD2)
#define TXE_XMTDMA_DBG_CTL	(0x00000BD4)
#define XMTDMA_AQMIF_STS	(0x00000BD6)
#define XMTDMA_PRELD_STS	(0x00000BD8)
#define XMTDMA_ACT_STS	(0x00000BDA)
#define XMTDMA_QUEUE_STS	(0x00000BDC)
#define XMTDMA_ENGINE_STS	(0x00000BDE)
#define XMTDMA_DMATX_STS	(0x00000BE0)
#define XMTDMA_THRUPUTCTL_STS	(0x00000BE2)
#define TXE_SCHED_MPMFC_STS	(0x00000BE4)
#define TXE_SCHED_FORCE_BAD	(0x00000BE6)
#define TXE_BMC_RDMGR_STATE	(0x00000BE8)
#define TXE_SCHED_ERR	(0x00000BEA)
#define TXE_BMC_PFF_CFG	(0x00000BEC)
#define TXE_BMC_PFFSTART_DEF	(0x00000BEE)
#define TXE_BMC_PFFSZ_DEF	(0x00000BF0)
#define AQM_CFG	(0x00000C00)
#define AQM_FIFODEF0	(0x00000C02)
#define AQM_FIFODEF1	(0x00000C04)
#define AQM_TXCNTDEF0	(0x00000C06)
#define AQM_TXCNTDEF1	(0x00000C08)
#define AQM_TXDMAEN	(0x00000C98)
#define AQM_TXDMAREQ	(0x00000C9A)
#define AQMF_READY0	(0x00000CA0)
#define AQMF_READY1	(0x00000CA2)
#define AQMF_READY2	(0x00000CA4)
#define AQMF_READY3	(0x00000CA6)
#define AQMF_READY4	(0x00000CA8)
#define AQMF_MTE0	(0x00000CAA)
#define AQMF_MTE1	(0x00000CAC)
#define AQMF_MTE2	(0x00000CAE)
#define AQMF_MTE3	(0x00000CB0)
#define AQMF_MTE4	(0x00000CB2)
#define AQMF_PLREADY0	(0x00000CB4)
#define AQMF_PLREADY1	(0x00000CB6)
#define AQMF_PLREADY2	(0x00000CB8)
#define AQMF_PLREADY3	(0x00000CBA)
#define AQMF_PLREADY4	(0x00000CBC)
#define AQMF_FULL0	(0x00000CBE)
#define AQMF_FULL1	(0x00000CC0)
#define AQMF_FULL2	(0x00000CC2)
#define AQMF_FULL3	(0x00000CC4)
#define AQMF_FULL4	(0x00000CC6)
#define AQMF_STATUS	(0x00000CC8)
#define MQF_EMPTY0	(0x00000CCA)
#define MQF_EMPTY1	(0x00000CCC)
#define MQF_EMPTY2	(0x00000CCE)
#define MQF_EMPTY3	(0x00000CD0)
#define MQF_EMPTY4	(0x00000CD2)
#define MQF_STATUS	(0x00000CD4)
#define MsduThreshold	(0x00000CD6)	/* rename */
#define TXQ_STATUS	(0x00000CD8)
#define AQM_BQEN	(0x00000CE0)
#define AQM_BQSUSP	(0x00000CE2)
#define AQM_BQFLUSH	(0x00000CE4)
#define AQMTXD_READ	(0x00000D10)
#define AQMTXD_RDOFFSET	(0x00000D12)
#define AQMTXD_RDLEN	(0x00000D14)
#define AQMTXD_DSTADDR	(0x00000D16)
#define AQMTXD_AUTOPF	(0x00000D18)
#define AQMTXD_APFOFFSET	(0x00000D1A)
#define AQMTXD_APFDSTADDR	(0x00000D1C)
#define AQMTXD_PFREADY0	(0x00000D1E)
#define AQMTXD_PFREADY1	(0x00000D20)
#define AQMTXD_PFREADY2	(0x00000D22)
#define AQMTXD_PFREADY3	(0x00000D24)
#define AQMTXD_PFREADY4	(0x00000D26)
#define AQMCT_BUSY0	(0x00000D28)
#define AQMCT_BUSY1	(0x00000D2A)
#define AQMCT_BUSY2	(0x00000D2C)
#define AQMCT_BUSY3	(0x00000D2E)
#define AQMCT_BUSY4	(0x00000D30)
#define AQMCT_QSEL	(0x00000D32)
#define AQMCT_PRI	(0x00000D34)
#define AQMCT_PRIFIFO	(0x00000D36)
#define AQMCT_MAXREQNUM	(0x00000D38)
#define AQMCT_FREECNT	(0x00000D3A)
#define AQMCT_CHDIS	(0x00000D3C)
#define AQMPL_CFG	(0x00000D40)
#define AQMPL_QSEL	(0x00000D42)
#define AQMPL_THRESHOLD	(0x00000D44)
#define AQMPL_EPOCHMASK	(0x00000D46)
#define AQMPL_MAXMPDU	(0x00000D48)
#define AQMPL_DIS	(0x00000D4A)
#define AQMPL_FORCE	(0x00000D4C)
#define AQMTXPL_THRESH	(0x00000D4E)
#define AQMTXPL_MASK	(0x00000D50)
#define AQMTXPL_RDY	(0x00000D52)
#define AQMCSB_REQ	(0x00000D60)
#define AQMF_RPTR	(0x00000D62)
#define AQMF_MPDULEN	(0x00000D64)
#define AQMF_SOFDPTR	(0x00000D66)
#define AQMF_SWDCNT	(0x00000D68)
#define AQMF_FLAG	(0x00000D6A)
#define AQMF_TXDPTR_L	(0x00000D6C)
#define AQMF_TXDPTR_ML	(0x00000D6E)
#define AQMF_TXDPTR_MU	(0x00000D70)
#define AQMF_TXDPTR_H	(0x00000D72)
#define AQMF_SRTIDX	(0x00000D74)
#define AQMF_ENDIDX	(0x00000D76)
#define AQMF_NUMBUF	(0x00000D78)
#define AQMF_BUFLEN	(0x00000D7A)
#define AQMFR_RWD0	(0x00000D7C)
#define AQMFR_RWD1	(0x00000D7E)
#define AQMFR_RWD2	(0x00000D80)
#define AQMFR_RWD3	(0x00000D82)
#define AQMCSB_RPTR	(0x00000D84)
#define AQMCSB_WPTR	(0x00000D86)
#define AQMCSB_BASEL	(0x00000D88)
#define AQMCSB_BA0	(0x00000D8A)
#define AQMCSB_BA1	(0x00000D8C)
#define AQMCSB_BA2	(0x00000D8E)
#define AQMCSB_BA3	(0x00000D90)
#define AQMCSB_QAGGLEN_L	(0x00000D92)
#define AQMCSB_QAGGLEN_H	(0x00000D94)
#define AQMCSB_QAGGNUM	(0x00000D96)
#define AQMCSB_QAGGSTATS	(0x00000D98)
#define AQMCSB_QAGGINFO	(0x00000D9A)
#define AQMCSB_CONSCNT	(0x00000D9C)
#define AQMCSB_TOTCONSCNT	(0x00000D9E)
#define AQMCSB_CFGSTRADDR	(0x00000DA0)
#define AQMCSB_CFGENDADDR	(0x00000DA2)
#define AQM_AGGERR	(0x00000DA8)
#define AQM_QAGGERR	(0x00000DAA)
#define AQM_BRSTATUS	(0x00000DAC)
#define AQM_QBRSTATUS	(0x00000DAE)
#define AQM_DBGCTL	(0x00000DB0)
#define TDC_CTL	(0x00000E00)
#define TDC_USRCFG	(0x00000E02)
#define TDC_RUNCMD	(0x00000E04)
#define TDC_RUNSTS	(0x00000E06)
#define TDC_STATUS	(0x00000E08)
#define TDC_NUSR	(0x00000E0A)
#define TDC_Plcp0	(0x00000E0C)	/* rename */
#define TDC_Plcp1	(0x00000E0E)	/* rename */
#define TDC_PLCP2	(0x00000E10)	/* rename-as-80 */
#define TDC_PLCP3	(0x00000E12)	/* rename-as-80 */
#define TDC_ORIGBW	(0x00000E14)
#define TDC_FRMLEN_L	(0x00000E16)
#define TDC_FRMLEN_H	(0x00000E18)
#define TDC_USRINFO_0	(0x00000E1A)
#define TDC_USRINFO_1	(0x00000E1C)
#define TDC_USRINFO_2	(0x00000E1E)
#define TDC_USRAID	(0x00000E20)
#define TDC_PPET0	(0x00000E22)
#define TDC_PPET1	(0x00000E24)
#define TDC_MAX_LENEXP	(0x00000E26)
#define TDC_MAXDUR	(0x00000E28)
#define TDC_MINMDUR	(0x00000E2A)
#define TDC_HDRDUR	(0x00000E2C)
#define TDC_TRIG_PADDUR	(0x00000E2E)
#define TDC_Txtime	(0x00000E30)	/* rename */
#define TDC_LSIGLEN	(0x00000E32)	/* rename-as-80 */
#define TDC_NSym0	(0x00000E34)	/* rename */
#define TDC_NSym1	(0x00000E36)	/* field definition extended */
#define TDC_PSDULEN_L	(0x00000E38)
#define TDC_PSDULEN_H	(0x00000E3A)
#define TDC_HDR_PDLIM	(0x00000E3C)
#define TDC_MAC_PADLEN	(0x00000E3E)
#define TDC_EOFPDLIM_L	(0x00000E40)
#define TDC_EOFPDLIM_H	(0x00000E42)
#define TDC_MAXPSDULEN_L	(0x00000E44)
#define TDC_MAXPSDULEN_H	(0x00000E46)
#define TDC_MPDU_MINLEN	(0x00000E48)
#define TDC_CCLEN	(0x00000E4A)
#define TDC_RATE_L	(0x00000E4C)
#define TDC_RATE_H	(0x00000E4E)
#define TDC_NSYMINIT_L	(0x00000E50)
#define TDC_NSYMINIT_H	(0x00000E52)
#define TDC_NDBPS_L	(0x00000E54)
#define TDC_NDBPS_H	(0x00000E56)
#define TDC_NDBPS_S	(0x00000E58)
#define TDC_T_PRE	(0x00000E5A)
#define TDC_TDATA	(0x00000E5C)
#define TDC_TDATA_MIN	(0x00000E5E)
#define TDC_TDATA_MAX	(0x00000E60)
#define TDC_TDATA_AVG	(0x00000E62)
#define TDC_TDATA_TOT	(0x00000E64)
#define TDC_HESIGB_NSYM	(0x00000E66)
#define TDC_AGGLEN_L	(0x00000E68)
#define TDC_AGGLEN_H	(0x00000E6A)
#define TDC_TIME_IN	(0x00000E6C)
#define TDC_INITREQ	(0x00000E6E)
#define TDC_AGG0STS	(0x00000E70)
#define TDC_TRIGENDLOG	(0x00000E72)
#define TDC_RUSTS	(0x00000E74)
#define TDC_DAID	(0x00000E76)
#define TDC_FBWCTL	(0x00000E78)
#define TDC_PPCTL	(0x00000E7A)
#define TDC_FBWSTS	(0x00000E7C)
#define TDC_PPSTS	(0x00000E7E)
#define TDC_CTL1	(0x00000E80)
#define TDC_PDBG	(0x00000E82)
#define TDC_PSDU_ACKSTS	(0x00000E84)
#define TDC_AGG_RDYSTS	(0x00000E86)
#define TDC_AGGLEN_OVR_L	(0x00000E88)
#define TDC_AGGLEN_OVR_H	(0x00000E8A)
#define TDC_DROPUSR	(0x00000E8C)
#define TDC_DURMARGIN	(0x00000E8E)
#define TDC_DURTHRESH	(0x00000E90)
#define TDC_MAXTXOP	(0x00000E92)
#define TDC_RXPPET	(0x00000E94)
#define TDC_AGG0TXOP	(0x00000E96)
#endif /* (AUTOREGS_COREREV >= 129) */

/* BCM43684B0 and up */
#if (AUTOREGS_COREREV >= 129) && (AUTOREGS_COREREV < 132)
#define xmt_dma_intmap1	(0x00000058)	/* rename */
#define AMT_Limit		(0x000004E4)	/* rename */
#define AMT_Attr		(0x000004E6)	/* rename */
#define AMT_MATCH1		(0x000004E8)	/* rename */
#define AMT_MATCH2		(0x000004EA)	/* rename */
#define AMT_Table_Addr		(0x000004EC)	/* rename */
#define AMT_Table_Data		(0x000004EE)	/* rename */
#define AMT_Table_Val		(0x000004F0)	/* rename */
#define AMT_DBG_SEL		(0x000004F2)	/* field definition modified */
#define AMT_MATCH		(0x000004F4)	/* rename */
#define AMT_ATTR_A1		(0x000004F6)	/* rename */
#define AMT_ATTR_A2		(0x000004F8)	/* rename */
#define AMT_ATTR_A3		(0x000004FA)	/* rename */
#define AMT_ATTR_BSSID		(0x000004FC)	/* rename */
#define RXE_HDRC_CTL		(0x00000500)
#define RXE_HDRC_STATUS		(0x00000502)
#define RXE_WM_0		(0x00000504)
#define RXE_WM_1		(0x00000506)
#define RXE_BM_ADDR		(0x00000508)
#define RXE_BM_DATA		(0x0000050A)
#define RXE_BV_ADDR		(0x0000050C)
#define RXE_BV_DATA		(0x0000050E)
#define PERUSER_DBG_SEL		(0x00000510)
#define EXT_IHR_ADDR		(0x0000059A)	/* rename */
#define EXT_IHR_DATA		(0x0000059C)	/* rename */
#define PSM_BASE_PSMX		(0x00000870)
#define PSM_RATEMEM_DBG		(0x00000872)	/* for 128: PSM_RLMEM_IDX */
#define PSM_RATEMEM_CTL		(0x00000874)
#define PSM_RATEBLK_SIZE	(0x00000876)
#define PSM_RATEXFER_SIZE	(0x00000878)
#define PSM_CHK0_CMD		(0x0000088A)	/* rename */
#define PSM_CHK0_SEL		(0x0000088C)	/* rename */
#define PSM_CHK0_ERR		(0x0000088E)	/* rename */
#define PSM_CHK0_INFO		(0x00000890)	/* rename */
#define PSM_CHK1_CMD		(0x00000892)	/* rename */
#define PSM_CHK1_SEL		(0x00000894)	/* rename */
#define PSM_CHK1_ERR		(0x00000896)	/* rename */
#define PSM_CHK1_INFO		(0x00000898)	/* rename */
#define Txdc_Addr		(0x00000968)	/* rename */
#define Txdc_Data		(0x0000096A)	/* rename */
#define RXMapFifoSize		(0x000009DC)
#define TXE_BMC_RXMAPSTATUS	(0x000009DE)
#define BMCCore1TXAllMaxBuffers	(0x000009E8)	/* field definition extended */
#define TXE_BMC_DYNSTATUS1	(0x000009EA)
#define Core1BMCAllocStatusTplate	(0x000009F4)	/* field definition extended */
#define TXE_PLCP_LEN		(0x00000A7A)
#define TXE_XMTDMA_PRELD_RANGE	(0x00000BCA)
#define XmtDMABusyOtherQ0to15	(0x00000BF4)
#define XmtDMABusyOtherQ16to31	(0x00000BF6)
#define XmtDMABusyOtherQ32to47	(0x00000BF8)
#define XmtDMABusyOtherQ48to63	(0x00000BFA)
#define XmtDMABusyOtherQ64to69	(0x00000BFC)
#define AQM_INITREQ		(0x00000C0A)
#define AQM_REGSEL		(0x00000C10)
#define AQM_QMAP		(0x00000C12)
#define AQM_MAXAGGRLEN_L	(0x00000C14)
#define AQM_MAXAGGRLEN_H	(0x00000C16)
#define AQM_MAXAGGRNUM		(0x00000C18)
#define AQM_MINMLEN		(0x00000C1A)
#define AQM_MAXOSFRAG		(0x00000C1C)
#define AQM_AGGPARM		(0x00000C1E)
#define AQM_FRPRAM		(0x00000C20)
#define AQM_MINFRLEN		(0x00000C22)
#define AQM_MQBURST		(0x00000C24)
#define AQM_DMLEN		(0x00000C26)
#define AQM_MMLEN		(0x00000C28)
#define AQM_CMLEN		(0x00000C2A)
#define AQM_VQENTRY0		(0x00000C2C)
#define AQM_VQENTRY1		(0x00000C2E)
#define AQM_VQADJ		(0x00000C30)
#define AQM_VQPADADJ		(0x00000C32)
#define AQM_AGGREN		(0x00000C34)
#define AQM_AGGREQ		(0x00000C36)
#define AQM_CAGGLEN_L		(0x00000C38)
#define AQM_CAGGLEN_H		(0x00000C3A)
#define AQM_CAGGNUM		(0x00000C3C)
#define AQM_QAGGSTATS		(0x00000C3E)
#define AQM_QAGGNUM		(0x00000C40)
#define AQM_QAGGINFO		(0x00000C42)
#define AQM_AGGRPTR		(0x00000C44)
#define AQM_QAGGLEN_L		(0x00000C46)
#define AQM_QAGGLEN_H		(0x00000C48)
#define AQM_QMPDULEN		(0x00000C4A)
#define AQM_QFRAGOS		(0x00000C4C)
#define AQM_QTXCNT		(0x00000C4E)
#define AQM_QMPDUINFO0		(0x00000C50)
#define AQM_QMPDUINFO1		(0x00000C52)
#define AQM_TXCNTEN		(0x00000C54)
#define AQM_TXCNTUPD		(0x00000C56)
#define AQM_QTXCNTRPTR		(0x00000C58)
#define AQM_QCURTXCNT		(0x00000C5A)
#define AQM_BASEL		(0x00000C60)
#define AQM_RCVDBA0		(0x00000C62)
#define AQM_RCVDBA1		(0x00000C64)
#define AQM_RCVDBA2		(0x00000C66)
#define AQM_RCVDBA3		(0x00000C68)
#define AQM_BASSN		(0x00000C6A)
#define AQM_REFSN		(0x00000C6C)
#define AQM_QMAXBAIDX		(0x00000C6E)
#define AQM_BAEN		(0x00000C70)
#define AQM_BAREQ		(0x00000C72)
#define AQM_UPDBARD		(0x00000C74)
#define AQM_UPDBA0		(0x00000C76)
#define AQM_UPDBA1		(0x00000C78)
#define AQM_UPDBA2		(0x00000C7A)
#define AQM_UPDBA3		(0x00000C7C)
#define AQM_ACKCNT		(0x00000C7E)
#define AQM_CONSTYPE		(0x00000C80)
#define AQM_CONSEN		(0x00000C82)
#define AQM_CONSREQ		(0x00000C84)
#define AQM_CONSCNT		(0x00000C88)
#define AQM_XUPDEN		(0x00000C90)
#define AQM_XUPDREQ		(0x00000C92)
#define AQMF_HSUSP0		(0x00000CE8)
#define AQMF_HSUSP1		(0x00000CEA)
#define AQMF_HSUSP2		(0x00000CEC)
#define AQMF_HSUSP3		(0x00000CEE)
#define AQMF_HSUSP4		(0x00000CF0)
#define AQMF_HSUSP_PRGR0	(0x00000CF2)
#define AQMF_HSUSP_PRGR1	(0x00000CF4)
#define AQMF_HSUSP_PRGR2	(0x00000CF6)
#define AQMF_HSUSP_PRGR3	(0x00000CF8)
#define AQMF_HSUSP_PRGR4	(0x00000CFA)
#define AQMF_HFLUSH0		(0x00000CFC)
#define AQMF_HFLUSH1		(0x00000CFE)
#define AQMF_HFLUSH2		(0x00000D00)
#define AQMF_HFLUSH3		(0x00000D02)
#define AQMF_HFLUSH4		(0x00000D04)
#define AQM_DBGDATA0		(0x00000DB2)
#define AQM_DBGDATA1		(0x00000DB4)
#define AQM_DBGDATA2		(0x00000DB6)
#define AQM_DBGDATA3		(0x00000DB8)
#define AQM_ERRSEL		(0x00000DBA)
#define AQM_ERRSTS		(0x00000DBC)
#define AQM_SWRESET		(0x00000DBE)
#define PSM_XMT_TPLDATA_L	(0x00000F00)
#define PSM_XMT_TPLDATA_H	(0x00000F02)
#define PSM_XMT_TPLPTR		(0x00000F04)
#define PSM_SMP_CTRL		(0x00000F06)
#define PSM_SCT_MASK_L		(0x00000F08)
#define PSM_SCT_MASK_H		(0x00000F0A)
#define PSM_SCT_VAL_L		(0x00000F0C)
#define PSM_SCT_VAL_H		(0x00000F0E)
#define PSM_SCTSTP_MASK_L	(0x00000F10)
#define PSM_SCTSTP_MASK_H	(0x00000F12)
#define PSM_SCTSTP_VAL_L	(0x00000F14)
#define PSM_SCTSTP_VAL_H	(0x00000F16)
#define PSM_SCX_MASK_L		(0x00000F18)
#define PSM_SCX_MASK_H		(0x00000F1A)
#define PSM_SCM_MASK_L		(0x00000F1C)
#define PSM_SCM_MASK_H		(0x00000F1E)
#define PSM_SCM_VAL_L		(0x00000F20)
#define PSM_SCM_VAL_H		(0x00000F22)
#define PSM_SCS_MASK_L		(0x00000F24)
#define PSM_SCS_MASK_H		(0x00000F26)
#define PSM_SCP_CURPTR		(0x00000F28)
#define PSM_SCP_STRTPTR		(0x00000F2A)
#define PSM_SCP_STOPPTR		(0x00000F2C)
#define PSM_SMP_CTRL2		(0x00000F2E)
#endif /* (AUTOREGS_COREREV >= 129) && (AUTOREGS_COREREV < 132) */

/* Embedded 80MHz and 160MHz 2x2 .ax cores and up */
#if (AUTOREGS_COREREV >= 130)
#define TXE_HTC_LOC		(0x0000095A)
#define TXE_HTC_L		(0x0000095C)
#define TXE_HTC_H		(0x0000095E)
#define BMCHWC_CNT_SEL		(0x000009DC)	/* rename */
#define BMCHWC_CNT		(0x000009DE)	/* rename */
#define BMCHWC_CTL		(0x000009E8)	/* rename */
#define BMC_FRM2MPM_PRGRSTS	(0x000009EA)	/* rename */
#define BMC_PSMCMD8		(0x000009F4)	/* rename */
#define TXE_BMC_PSMCMDQ_CTL	(0x00000A1E)
#define TXE_BMC_ERRSTSEN	(0x00000A20)
#define TX_PREBM_FATAL_ERRVAL	(0x00000A22)
#define TX_PREBM_FATAL_ERRMASK	(0x00000A24)
#define SCS_TAIL_CNT		(0x00000A26)
#define TXE_BMC_ERRSTS		(0x00000B72)
#define XMTDMA_FATAL_ERR_RPTEN	(0x00000BC6)
#define XMTDMA_FATAL_ERR	(0x00000BFE)
#if (AUTOREGS_COREREV < 132)
#define TXE_MEND_SIG		(0x000009FE) /* relocated in later core rev */
#define TXE_MENDLAST_STATUS	(0x00000A00) /* relocated in later core rev */
#endif /* (AUTOREGS_COREREV < 132) */
#endif /* (AUTOREGS_COREREV >= 130) */

/* BCM6710 and up */
#if (AUTOREGS_COREREV >= 131)
#define XMTDMA_PROGERR		(0x00000BF8)
#endif /* (AUTOREGS_COREREV >= 131) */

/* BCM6715 and up */
#if (AUTOREGS_COREREV >= 132)
#define ctdma_ctl		(0x00000058)	/* rename */
#define MAC_BOOT_CTRL		(0x0000014C)
#define MacCapability2		(0x0000019C)
#define RxDmaXferCnt		(0x00000374)
#define RxDmaXferCntMax		(0x00000378)
#define RxFIFOCnt		(0x0000037C)
#define AMT_START		(0x000004E4)
#define AMT_END			(0x000004E6)
#define AMT_Attr		(0x000004E8)
#define AMT_MATCH		(0x000004EA)
#define AMT_MATCH_A1		(0x000004EC)
#define AMT_MATCH_A2		(0x000004EE)
#define AMT_MATCH_A3		(0x000004F0)
#define AMT_MATCH_BSSID		(0x000004F2)
#define AMT_Table_Addr		(0x000004F4)
#define AMT_Table_Data		(0x000004F6)
#define AMT_Table_Val		(0x000004F8)
#define AMT_ATTR_A1		(0x000004FA)
#define AMT_ATTR_A2		(0x000004FC)
#define AMT_ATTR_A3		(0x000004FE)
#define AMT_ATTR_BSSID		(0x00000500)
#define AMT_DBG_SEL		(0x00000502)
#define FP_PAT1A		(0x00000504)
#define EXT_IHR_ADDR		(0x00000580)
#define EXT_IHR_DATA		(0x00000582)
#define PSM_USR_SEL		(0x000007AE)
#define PSM_FATAL_STS		(0x000007D8)
#define PSM_FATAL_MASK		(0x000007DA)
#define HWA_MACIF_CTL1		(0x000007DE)
#define PSM_RATEMEM_DBG		(0x000007F8)
#define PSM_RATEMEM_CTL		(0x000007FA)
#define PSM_RATEBLK_SIZE	(0x000007FC)
#define PSM_RATEXFER_SIZE	(0x000007FE)
#define PSM_BASE_24		(0x00000870)
#define PSM_BASE_25		(0x00000872)
#define PSM_BASE_26		(0x00000874)
#define PSM_BASE_27		(0x00000876)
#define PSM_BASE_PSMX		(0x00000878)
#define PSM_CHK0_CTL		(0x0000088A)
#define PSM_CHK0_LMTL		(0x0000088C)
#define PSM_CHK0_LMTH		(0x0000088E)
#define PSM_CHK0_ERR		(0x00000890)
#define PSM_CHK0_EADDR		(0x00000892)
#define PSM_CHK0_INFO		(0x00000894)
#define PSM_CHK1_CTL		(0x00000896)
#define PSM_CHK1_LMTL		(0x00000898)
#define PSM_CHK1_LMTH		(0x0000089A)
#define PSM_CHK1_ERR		(0x0000089C)
#define PSM_CHK1_EADDR		(0x0000089E)
#define PSM_CHK1_INFO		(0x0000082C)
#define PSM_M2DMA_ADDR		(0x000008F4)
#define PSM_M2DMA_DATA		(0x000008F6)
#define PSM_M2DMA_FREE		(0x000008F8)
#define PSM_M2DMA_CFG		(0x000008FA)
#define PSM_M2DMA_DATA_L	(0x000008FC)
#define PSM_M2DMA_DATA_H	(0x000008FE)
#define TXE_SHMDMA_PHYSTSADDR	(0x00000968)	/* rename */
#define TXE_SHMDMA_MACFIFOADDR	(0x0000096A)	/* rename */
#define PLCP_CC34_LEN		(0x00000A7A)	/* rename */
#define TXE_XMTDMA_SW_RSTCTL	(0x00000BCA)	/* rename */
#define XMTDMA_ACTUSR_DBGCTL	(0x00000BF2)
#define XMTDMA_ACTUSR_VLDSTS	(0x00000BF4)
#define XMTDMA_ACTUSR_QID	(0x00000BF6)
#define TXE_MENDLAST_STATUS	(0x00000BFA)
#define XMTDMA_AQM_ACT_CNT	(0x00000BFC)
#define AQM_MSDUDEF0		(0x00000C0A)
#define AQM_MSDUDEF1		(0x00000C0C)
#define AQM_INITREQ		(0x00000C0E)
#define AQM_PSMTXDSZ		(0x00000C10)
#define AQM_REGSEL		(0x00000C12)
#define AQM_QMAP		(0x00000C14)
#define AQM_MAXAGGRLEN_L	(0x00000C16)
#define AQM_MAXAGGRLEN_H	(0x00000C18)
#define AQM_MAXAGGRNUM		(0x00000C1A)
#define AQM_MINMLEN		(0x00000C1C)
#define AQM_MAXOSFRAG		(0x00000C1E)
#define AQM_NDLIM		(0x00000C20)
#define AQM_MAXNDLIMLEN		(0x00000C22)
#define AQM_AGGPARM		(0x00000C24)
#define AQM_FRPRAM		(0x00000C26)
#define AQM_FRLENLIMIT		(0x00000C28)
#define AQM_FRFIXLEN		(0x00000C2A)
#define AQM_MINFRLEN		(0x00000C2C)
#define AQM_MQBURST		(0x00000C2E)
#define AQM_DMLEN		(0x00000C30)
#define AQM_MMLEN		(0x00000C32)
#define AQM_CMLEN		(0x00000C34)
#define AQM_VQENTRY0		(0x00000C36)
#define AQM_VQENTRY1		(0x00000C38)
#define AQM_VQADJ		(0x00000C3A)
#define AQM_VQPADADJ		(0x00000C3C)
#define AQM_AGGREN		(0x00000C3E)
#define AQM_AGGREQ		(0x00000C40)
#define AQM_CAGGLEN_L		(0x00000C42)
#define AQM_CAGGLEN_H		(0x00000C44)
#define AQM_CAGGNUM		(0x00000C46)
#define AQM_QAGGSTATS		(0x00000C48)
#define AQM_QAGGNUM		(0x00000C4A)
#define AQM_QAGGINFO		(0x00000C4C)
#define AQM_AGGRPTR		(0x00000C4E)
#define AQM_QAGGLEN_L		(0x00000C50)
#define AQM_QAGGLEN_H		(0x00000C52)
#define AQM_QMPDULEN		(0x00000C54)
#define AQM_QFRAGOS		(0x00000C56)
#define AQM_QTXCNT		(0x00000C58)
#define AQM_QFRTXCNT		(0x00000C5A)
#define AQM_QMPDUINFO0		(0x00000C5C)
#define AQM_QMPDUINFO1		(0x00000C5E)
#define AQM_TXCNTEN		(0x00000C60)
#define AQM_TXCNTUPD		(0x00000C62)
#define AQM_QTXCNTRPTR		(0x00000C64)
#define AQM_QCURTXCNT		(0x00000C66)
#define AQM_BASEL		(0x00000C68)
#define AQM_RCVDBA0		(0x00000C6A)
#define AQM_RCVDBA1		(0x00000C6C)
#define AQM_RCVDBA2		(0x00000C6E)
#define AQM_RCVDBA3		(0x00000C70)
#define AQM_BASSN		(0x00000C72)
#define AQM_REFSN		(0x00000C74)
#define AQM_QMAXBAIDX		(0x00000C76)
#define AQM_BAEN		(0x00000C78)
#define AQM_BAREQ		(0x00000C7A)
#define AQM_UPDBARD		(0x00000C7C)
#define AQM_UPDBA0		(0x00000C7E)
#define AQM_UPDBA1		(0x00000C80)
#define AQM_UPDBA2		(0x00000C82)
#define AQM_UPDBA3		(0x00000C84)
#define AQM_ACKCNT		(0x00000C86)
#define AQM_TOTACKCNT		(0x00000C88)
#define AQM_CONSTYPE		(0x00000C8A)
#define AQM_CONSEN		(0x00000C8C)
#define AQM_CONSREQ		(0x00000C8E)
#define AQM_CONSCNT		(0x00000C90)
#define AQM_XUPDEN		(0x00000C92)
#define AQM_XUPDREQ		(0x00000C94)
#define AQM_2TDCCTL		(0x00000C96)
#define AQM_AGGR_PLDRDY		(0x00000C9C)
#define AQM_AUTOBQC_EN		(0x00000CE6)
#define AQM_AUTOBQC		(0x00000CE8)
#define AQM_AUTOBQC_TO		(0x00000CEA)
#define AQM_AUTOBQC_TOSTS	(0x00000CEC)
#define AQM_AUTOBQC_TSCALE	(0x00000CEE)
#define AQM_AUTOBQC_MAXTCNT	(0x00000CF0)
#define AQMF_HSUSP0		(0x00000CF2)
#define AQMF_HSUSP1		(0x00000CF4)
#define AQMF_HSUSP2		(0x00000CF6)
#define AQMF_HSUSP3		(0x00000CF8)
#define AQMF_HSUSP4		(0x00000CFA)
#define AQMF_HSUSP_PRGR0	(0x00000CFC)
#define AQMF_HSUSP_PRGR1	(0x00000CFE)
#define AQMF_HSUSP_PRGR2	(0x00000D00)
#define AQMF_HSUSP_PRGR3	(0x00000D02)
#define AQMF_HSUSP_PRGR4	(0x00000D04)
#define AQMF_HFLUSH0		(0x00000D06)
#define AQMF_HFLUSH1		(0x00000D08)
#define AQMF_HFLUSH2		(0x00000D0A)
#define AQMF_HFLUSH3		(0x00000D0C)
#define AQMF_HFLUSH4		(0x00000D0E)
#define AQMHWC_INPROG		(0x00000D58)
#define AQMHWC_STATUS		(0x00000D5A)
#define AQM_DBGCTL1		(0x00000DB2)
#define AQM_DBGDATA0		(0x00000DB4)
#define AQM_DBGDATA1		(0x00000DB6)
#define AQM_DBGDATA2		(0x00000DB8)
#define AQM_DBGDATA3		(0x00000DBA)
#define AQM_ERRSEL		(0x00000DC0)
#define AQM_ERRSTSEN		(0x00000DC2)
#define AQM_ERRSTS		(0x00000DC4)
#define AQM_SWRESET		(0x00000DE0)
#define AQMFTM_CTL		(0x00000DF2)
#define AQMFTM_MPDULEN		(0x00000DF4)
#define AQMFTM_FLAG		(0x00000DF6)
#define AQM_AUTOBQC_CONSDLY	(0x00000DF8)
#define AQM_MUENGCTL		(0x00000DFA)
#define TDC_NDLIM		(0x00000E98)
#define TDC_MAXNDLIMLEN		(0x00000E9A)
#define TDC_CC34LEN		(0x00000E9C)
#define TDC_NMURU		(0x00000E9E)
#define TDC_MIMOSTS		(0x00000EA0)
#define PSM_XMT_TPLDATA_L	(0x00000F20)
#define PSM_XMT_TPLDATA_H	(0x00000F22)
#define PSM_XMT_TPLPTR		(0x00000F24)
#define PSM_SMP_CTRL		(0x00000F26)
#define PSM_SCT_MASK_L		(0x00000F28)
#define PSM_SCT_MASK_H		(0x00000F2A)
#define PSM_SCT_VAL_L		(0x00000F2C)
#define PSM_SCT_VAL_H		(0x00000F2E)
#define PSM_SCTSTP_MASK_L	(0x00000F30)
#define PSM_SCTSTP_MASK_H	(0x00000F32)
#define PSM_SCTSTP_VAL_L	(0x00000F34)
#define PSM_SCTSTP_VAL_H	(0x00000F36)
#define PSM_SCX_MASK_L		(0x00000F38)
#define PSM_SCX_MASK_H		(0x00000F3A)
#define PSM_SCM_MASK_L		(0x00000F3C)
#define PSM_SCM_MASK_H		(0x00000F3E)
#define PSM_SCM_VAL_L		(0x00000F40)
#define PSM_SCM_VAL_H		(0x00000F42)
#define PSM_SCS_MASK_L		(0x00000F44)
#define PSM_SCS_MASK_H		(0x00000F46)
#define PSM_SCP_CURPTR		(0x00000F48)
#define PSM_SCP_STRTPTR		(0x00000F4A)
#define PSM_SCP_STOPPTR		(0x00000F4C)
#define PSM_SMP_CTRL2		(0x00000F4E)
#define PSM_SCS_TAIL_CNT	(0x00000F50)
#define TXE_BMC_CMDPUSH_CNT	(0x00001000)
#define TXE_BMC_CMDPOP_CNT	(0x00001002)
#define TXE_MEND_SIG		(0x00001004)
#define TXE_RST_TXMEND		(0x00001006)
#define TXE_BMC_BQSEL		(0x00001008)
#define TXE_BMC_WBCNT0		(0x0000100A)
#define TXE_BMC_WBCNT1		(0x0000100C)
#define TXE_XMTDMA_CNTSTS_CTL	(0x0000100E)
#define XMTDMA_QREQBYTE_CNT0	(0x00001010)
#define XMTDMA_QREQBYTE_CNT1	(0x00001012)
#define XMTDMA_QREQ_MPDU_CNT	(0x00001014)
#define XMTDMA_UREQBYTE_CNT0	(0x00001016)
#define XMTDMA_UREQBYTE_CNT1	(0x00001018)
#define XMTDMA_QRESPBYTE_CNT0	(0x0000101A)
#define XMTDMA_QRESPBYTE_CNT1	(0x0000101C)
#define XMTDMA_QRESP_MPDU_CNT	(0x0000101E)
#define XMTDMA_DATAPRI_STS0		(0x00001020)
#define XMTDMA_DATAPRI_STS1		(0x00001022)
#define TXE_MEND_CTL		(0x00001026)
#define TXE_MENDLAST_SIG	(0x00001028)
#define TXE_BMC_WFCNT		(0x0000102A)
#define TXE_SIGBCC3_BASE	(0x0000102C)
#define TXE_SIGBCC4_BASE	(0x0000102E)
#define TXE_BMC_EOFCNT		(0x00001030)
#define TXE_PPR_CFG0		(0x00001180)
#define TXE_PPR_CFG1		(0x00001182)
#define TXE_PPR_CFG2		(0x00001184)
#define TXE_PPR_CFG3		(0x00001186)
#define TXE_PPR_CFG4		(0x00001188)
#define TXE_PPR_CFG5		(0x0000118A)
#define TXE_PPR_CFG6		(0x0000118C)
#define TXE_PPR_CFG7		(0x0000118E)
#define TXE_PPR_CFG8		(0x00001190)
#define TXE_PPR_CFG9		(0x00001192)
#define TXE_PPR_CFG10		(0x00001194)
#define TXE_PPR_CFG11		(0x00001196)
#define TXE_PPR_CFG12		(0x00001198)
#define TXE_PPR_CFG13		(0x0000119A)
#define TXE_PPR_CFG14		(0x0000119C)
#define TXE_PPR_CFG15		(0x0000119E)
#define TXE_PPR_CFG16		(0x000011A0)
#define TXE_PPR_CTL0		(0x000011A2)
#define TXE_PPR_CTL1		(0x000011A4)
#define TXE_PPR_CTL2		(0x000011A6)
#define TXE_PPR_STAT0		(0x000011AA)
#define TXE_PPR_STAT1		(0x000011AC)
#define TXE_PPR_STAT2		(0x000011AE)
#define TXE_PPR_STAT3		(0x000011B0)
#define RXE_IFEVENTDBG_CTL	(0x00001200)
#define RXE_IFEVENTDBG_STAT	(0x00001202)
#define RXE_MEND_FLAT		(0x00001204)
#define RXE_XFERACT_FLAT	(0x00001206)
#define RXE_PHYFIFO_NOT_EMPTY_FLAT	(0x00001208)
#define RXE_PFPLCP_WRDCNT	(0x0000120A)
#define RXE_RXDATA_NOT_EMPTY_FLAT	(0x0000120C)
#define RXE_PHYSTS_SHM_CTL	(0x0000120E)
#define RXE_RXBM_FATAL_ERRVAL	(0x00001210)
#define RXE_RXBM_FATAL_ERRMASK	(0x00001212)
#define RXE_PREBM_FATAL_ERRVAL	(0x00001214)
#define RXE_PREBM_FATAL_ERRMASK	(0x00001216)
#define RXE_CTXSTSFIFO_FATAL_ERRVAL	(0x00001218)
#define RXE_CTXSTSFIFO_FATAL_ERRMASK	(0x0000121A)
#define RXE_POSTBM_FATAL_ERRVAL	(0x0000121C)
#define RXE_POSTBM_FATAL_ERRMASK	(0x0000121E)
#define RXQ_FATAL_ERR_RPTEN	(0x00001220)
#define RXQ_FATAL_ERR		(0x00001222)
#define RXE_RXFRMUP_TSF_L	(0x00001240)
#define RXE_RXFRMUP_TSF_H	(0x00001242)
#define RXE_RXFRMDN_TSF_L	(0x00001244)
#define RXE_RXFRMDN_TSF_H	(0x00001246)
#define RXE_RSF_NP_STATS	(0x00001248)
#define RXE_RSF_HP_STATS	(0x0000124A)
#define RXE_POSTBM_TIMEOUT	(0x0000124C)
#define RDF_CTL8		(0x0000124E)
#define FP_DEBUG		(0x00001250)
#define RXE_RXE2WEP_BYTES	(0x00001252)
#define RXE_WEP2FP_BYTES	(0x00001254)
#define RXE_RXE2BM_BYTES	(0x00001256)
#define RXE_WCS_HDR_THRESHOLD	(0x00001258)
#define RXE_WCS_MIN_THRESHOLD	(0x0000125A)
#define RXE_WCS_MAX_THRESHOLD	(0x0000125C)
#define RXE_DAGG_DEBUG		(0x0000125E)
#define RXE_BFDRPT_XFERPEND	(0x00001260)
#define RXE_BFDRPT_XFERRD	(0x00001262)
#define RXE_BFDRPT_XFERSTS	(0x00001264)
#define RXE_BFDRPT_CAPSTS	(0x00001266)
#define RXE_WCS_TOUT_THRESHOLD	(0x00001268)
#define RXE_WCS_TOUT_STATUS	(0x0000126A)
#define RXE_WCS_CTL		(0x0000126C)
#define RXE_BMCLOOPBACK_DISCARD	(0x0000126E)
#define RXE_STRMRD_THRESHOLD	(0x00001270)
#define RXE_RCM_XFERBMP		(0x00001272)
#define RXE_WCS_COUNTERS	(0x00001274)
#define RXE_WCS_DEBUG		(0x00001276)
#define FP_STATUS6		(0x00001278)
#define FP_ACKTYPETID_BITMAP	(0x0000127A)
#define RXE_PHYSTS_DEBUG1	(0x0000127C)
#define RXE_PHYSTS_DEBUG2	(0x0000127E)
#define RXE_PHYSTS_ADDR		(0x00001280)
#define RXE_PHYSTS_DATA		(0x00001282)
#define RXE_PHYSTS_FREE		(0x00001284)
#define RXE_PHYSTS_CFG		(0x00001286)
#define RXE_PHYSTS_DATA_L	(0x00001288)
#define RXE_PHYSTS_DATA_H	(0x0000128A)
#define RXE_HDRC_CTL		(0x00001300)
#define RXE_HDRC_STATUS		(0x00001302)
#define RXE_WM_0		(0x00001304)
#define RXE_WM_1		(0x00001306)
#define RXE_BM_ADDR		(0x00001308)
#define RXE_BM_DATA		(0x0000130A)
#define RXE_BV_ADDR		(0x0000130C)
#define RXE_BV_DATA		(0x0000130E)
#define PERUSER_DBG_SEL		(0x00001310)
#define CBR_DBG_DATA		(0x00001312)
#define PHYFIFO_SIZE_CFG	(0x00001314)
#define PHYFIFO_SIZE_CFG1	(0x00001316)
#define FP_MUBAR_CONFIG		(0x00001318)
#define FP_MUBAR_TYPE_0		(0x0000131A)
#define FP_MUBAR_TYPE_1		(0x0000131C)
#define FP_MUBAR_TYPE_2		(0x0000131E)
#define FP_MUBAR_TYPE_3		(0x00001320)
#define RXE_EOFPD_CNT		(0x00001322)
#define RXE_EOFPD_THRSH		(0x00001324)
#define RXE_AGGLEN_EST		(0x00001326)
#define RXE_PFWRDCNT_CTL	(0x00001328)
#define RXE_OPMODE_CFG		(0x0000132A)
#define RXE_DBG_CTL		(0x0000132C)
#define POSTBM_DBG_STS		(0x0000132E)
#define PREBM_DBG_STS		(0x00001330)
#define RXE_RCVCTL2		(0x00001332)
#endif /* (AUTOREGS_COREREV >= 132) */
