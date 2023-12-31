/**
 * -----------------------------------------------------------------------------
 *
 * HWA Core Register Definitions
 * Assembled from various HWA top, common and blocks reg_defs.h files
 *
 * These definitions must be compatible with all HWA revisions. When a new
 * chipset's HWA revision is added, include the RegDB generated signature.
 *
 *
 * Family   Revision     Chips
 * ------   --------     ----------------------------
 * HWA2.0   128          43684Ax (Deprecated/Deleted)
 * HWA2.1   129          43684Bx May 30 2018
 * HWA2.1   130          43684Cx Mar 25 2019
 * HWA2.2   131           6715Ax Oct 27 2019
 * HWA2.2   133           6715B0 Jul 09 2020
 *
 * -----------------------------------------------------------------------------
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
 * <<Broadcom-WL-IPTag/Proprietary:>>
 *
 * $Id$
 *
 * vim: set ts=4 noet sw=4 tw=80:
 * -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * -----------------------------------------------------------------------------
 */

/**
 * -----------------------------------------------------------------------------
 *  XXX Internal
 *
 *  reg_defs.h                 Rev   RegDB timestamp             TXT SHA
 *  ------------------------   ---   ------------------------    -------
 *  hwa_top_reg_defs.h         130   Mon Mar 25 20:05:16 2019    e513820
 *  hwa_common_reg_defs.h      130   Mon Mar 25 20:05:11 2019    1c23a2d
 *  hwa_bm_reg_defs.h          130   Mon Mar 25 20:05:10 2019    1010c13
 *  hwa_tx_reg_defs.h          130   Mon Mar 25 20:05:01 2019    0796610
 *  hwa_txdma_reg_defs.h       130   Mon Mar 25 20:04:59 2019    047dcd7
 *  hwa_tx_status_reg_defs.h   130   Mon Mar 25 20:04:58 2019    a9a5baa
 *  hwa_rx_reg_defs.h          130   Mon Mar 25 20:05:07 2019    0ffdd81
 *  hwa_cpl_reg_defs.h         130   Mon Mar 25 20:05:04 2019    cc1ef1f
 *  hwa_dma_reg_defs.h         130   Mon Mar 25 20:05:13 2019    34374a5
 *
 *  hwa_top_reg_defs.h         131   Tue Dec  3 00:35:09 2019    cddf284b
 *  hwa_common_reg_defs.h      131   Tue Dec  3 00:35:02 2019    8a44ce51
 *  hwa_bm_reg_defs.h          131   Tue Dec  3 00:35:00 2019    1010c137
 *  hwa_tx_reg_defs.h          131   Tue Dec  3 00:34:49 2019    06c9c692
 *  hwa_txdma_reg_defs.h       131   Tue Dec  3 00:34:47 2019    9749323d
 *  hwa_tx_status_reg_defs.h   131   Tue Dec  3 00:34:46 2019    aa399d11
 *  hwa_rx_reg_defs.h          131   Tue Dec  3 00:34:56 2019    fa4402a1
 *  hwa_cpl_reg_defs.h         131   Tue Dec  3 00:34:51 2019    9b0f78ad
 *  hwa_dma_reg_defs.h         131   Tue Dec  3 00:35:03 2019    17e600ec
 *  hwa_pager_reg_defs.h       131   Tue Dec  3 00:35:07 2019    17bd81fd
 *
 *  hwa_top_reg_defs.h         133   Thu Aug  6 02:08:25 2020    cddf284b
 *  hwa_common_reg_defs.h      133   Thu Aug  6 02:08:17 2020    da470bf5
 *  hwa_bm_reg_defs.h          133   Thu Aug  6 02:08:15 2020    1010c137
 *  hwa_tx_reg_defs.h          133   Thu Aug  6 02:08:03 2020    06c9c692
 *  hwa_txdma_reg_defs.h       133   Thu Aug  6 02:08:01 2020    88a992e7
 *  hwa_tx_status_reg_defs.h   133   Thu Aug  6 02:07:59 2020    aa399d11
 *  hwa_rx_reg_defs.h          133   Thu Aug  6 02:08:11 2020    5a607713
 *  hwa_cpl_reg_defs.h         133   Thu Aug  6 02:08:06 2020    9b0f78ad
 *  hwa_dma_reg_defs.h         133   Thu Aug  6 02:08:19 2020    17e600ec
 *  hwa_pager_reg_defs.h       133   Thu Aug  6 02:08:23 2020    001227c8
 *
 *  hc_hin_reg_defs.h                Thu Aug 24 17:42:31 2017    Unknown
 *
 * -----------------------------------------------------------------------------
 */

#ifndef _HWA_REGS_H
#define _HWA_REGS_H

#include <typedefs.h>
#include <bcmdefs.h>
#include <hwa_reg_defs.h> // regdb.pl generated SHIFT and MASK macros

#define HWA_REGS_NULL                   ((hwa_regs_t *)NULL)
#define HWA_AUDIT_REGS(regs)            HWA_ASSERT((regs) != HWA_REGS_NULL)

// HW default capacity
#define HWA_TX_CORES_MAX_HW 2
#define HWA_RX_CORES_MAX_HW 2

// SW default capacity
#define HWA_TX_CORES_MAX 1
#define HWA_RX_CORES_MAX 1

// RSDB related: Set of 2 registers
#define DECL_RSDB_CORES(NAME) \
	union { \
		struct { uint32 NAME##0; uint32 NAME##1; }; \
		uint32 NAME[2]; \
	}

// 64 bit Host addresses: dma64addr_t as a pair of uint32 [lo, hi]
#define DECL_DMA64ADDR(NAME) \
	union { \
		struct { uint32 NAME##low; uint32 NAME##high; }; \
		struct { uint32 NAME##lo; uint32 NAME##hi; }; \
		struct { uint32 NAME##_lo; uint32 NAME##_hi; }; \
		struct { uint32 NAME##_l; uint32 NAME##_h; }; \
		dma64addr_t NAME; \
	}

// RxPath registers layout for rings
#define DECL_RXPATH_RING(NAME)                                              \
	struct {                                                                \
		DECL_DMA64ADDR(NAME##_ring_addr);               /* 0x000 - 0x007 */ \
		uint32 NAME##_ring_wrindex;                     /* 0x008         */ \
		uint32 NAME##_ring_rdindex;                     /* 0x00c         */ \
		uint32 NAME##_ring_cfg;                         /* 0x010         */ \
		uint32 NAME##_intraggr_seqnum_cfg;              /* 0x014         */ \
	}                                                   /* 0x000 - 0x017 */

// RxPath registers layout for rings
#define DECL_RXPATH_FIFO(NAME)                                              \
	struct {                                                                \
		DECL_RXPATH_RING(NAME);                         /* 0x000 - 0x017 */ \
		uint32 NAME##_PAD0[3];                          /* 0x018 - 0x023 */ \
		uint32 NAME##_wrindexupd_addrlo;                /* 0x024         */ \
		uint32 NAME##_status;                           /* 0x028         */ \
		uint32 NAME##_PAD1[1];                          /* 0x02c - 0x02f */ \
	}                                                   /* 0x000 - 0x02f */

// FIXUP for hwa_tx_regs_t
#define HWA_TX_PKT_CH_MAX           8                   // 8 chain contexts

typedef volatile struct hwa_cplintr {
	DECL_DMA64ADDR(addr);                               // 0x000
	uint32 val;                                         // 0x008
	uint32 PAD;                                         // 0x00c
} hwa_cplintr_t;

typedef volatile struct hwa_tx_pkt_ch {                 // 0x000
	uint32 head_ptr;                                    // 0x000
	uint32 tail_ptr;                                    // 0x004
	uint32 octet_count;                                 // 0x008
	union {                                             //
		uint32 ctrl;                                    // 0x00c
		struct {                                        //
			uint16 pktcount;                            // 0x00c
			uint16 timestampms16;                       // 0x00e
		};                                              //
	};                                                  //
	uint32 timestamp;                                   // 0x010
} hwa_tx_pkt_ch_t;                                      // 0x014  20B   5Regs

#define HWA_TXDMA_ETH_TYPE_OUI_MAX  4

#define HWA_CPL_CEDQ_MAX            10
#define HWA_CPL_CEDQ_TOT            5                   // 43684 : 1 TX + 4 RX

// FIXUP for hwa_cpl_regs_t
typedef volatile struct hwa_cpl_cedq {                  // 0x000
	uint32 base;                                        // 0x000
	uint32 depth;                                       // 0x004
	uint32 wridx;                                       // 0x008
	uint32 rdidx;                                       // 0x00c
} hwa_cpl_cedq_t;                                       // 0x010  16B   4Regs

// FIXUP for hwa_dma_regs_t
// sbhnddma.h dma64regs_t: control, ptr, addrlow, addrhigh, status0, status1
#include <sbhnddma.h>

#define HWA_DMA_CHANNELS            2

typedef volatile struct hwa_dma_chint {                 // 0x000
	uint32 status;                                      // 0x000
	uint32 mask;                                        // 0x004
} hwa_dma_chint_t;                                      // 0x007   8B   2Regs

typedef volatile struct hwa_dma_channel {               // 0x000
	dma64regs_t tx;                                     // 0x000 - 0x017
	uint32 PAD[2];                                      // 0x018 - 0x01f
	dma64regs_t rx;                                     // 0x020 - 0x037
	uint32 PAD[2];                                      // 0x038 - 0x03f
} hwa_dma_channel_t;                                    // 0x040  64B  16Regs

// Packet Pager registers layout for pktpool
typedef volatile struct hwa_pp_pool {
	dma64addr_t addr;                                   // 0x000 - 0x007
	uint32      ctrl;                                   // 0x008
	uint32      size;                                   // 0x00c
	uint32      intr_th;                                // 0x010
	uint32      alloc_index;                            // 0x014
	uint32      dealloc_index;                          // 0x018
	uint32      dealloc_status;                         // 0x01c
} hwa_pp_pool_t;                                        // 0x020 32B    8Regs

// Packet Pager registers layout for ring
typedef volatile struct hwa_pp_ring {
	uint32      addr;                                   // 0x000
	uint32      wr_index;                               // 0x004
	uint32      rd_index;                               // 0x008
	uint32      cfg;                                    // 0x00c
	uint32      lazyint_cfg;                            // 0x010
	uint32      debug;                                  // 0x014
} hwa_pp_ring_t;                                        // 0x018 24B    6Regs

// Packet Pager registers layout for pkt inuse counter/hwm
typedef volatile struct hwa_pp_pkt_inuse {
	uint32      all;                                    // 0x000
	uint32      tx;                                     // 0x004
	uint32      rx;                                     // 0x008
} hwa_pp_pkt_inuse_t;                                   // 0x00c 12B    3Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_TOP REGISTERS: hwa_top_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_top_regs {                  // 0x000
	uint32 PAD[1];                                      // 0x000 - 0x003
	uint32 debug_fifobase;                              // 0x004
	DECL_DMA64ADDR(debug_fifodata);                     // 0x008 - 0x00f
	uint32 PAD[52];                                     // 0x010 - 0x1df
	uint32 clkctlstatus;                                // 0x0e0
	uint32 workaround;                                  // 0x0e4
	uint32 powercontrol;                                // 0x0e8
	uint32 hwahwcap1;                                   // 0x0ec
	uint32 hwahwcap2;                                   // 0x0f0
	uint32 PAD[3];                                      // 0x0f4 - 0x0ff
} hwa_top_regs_t;                                       // 0x100 256B  64Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_COMMON REGISTERS: hwa_common_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_common_regs {               // 0x000
	DECL_RSDB_CORES(rxpost_wridx_r);                    // 0x000 - 0x007
	uint32 PAD[2];                                      // 0x008 - 0x00f
	uint32 dma_desc_chk_ctrl;                           // 0x010
	uint32 dma_desc_chk_th;                             // 0x014
	uint32 PAD[2];                                      // 0x018 - 0x01f
	uint32 dma_desc_chk_sts0;                           // 0x020
	uint32 dma_desc_chk_sts1;                           // 0x024
	uint32 dma_desc_chk_sts2;                           // 0x028
	uint32 dma_dbg_msg_addr;                            // 0x02c
	uint32 dma_dbg_msg_data;                            // 0x030
	uint32 PAD[19];                                     // 0x034 - 0x07f
	uint32 intstatus;                                   // 0x080
	uint32 intmask;                                     // 0x084
	uint32 statscontrolreg;                             // 0x088
	uint32 statdonglreaddressreg;                       // 0x08c
	uint32 statminbusytimereg;                          // 0x090
	uint32 errorstatusreg;                              // 0x094
	uint32 directaxierrorstatus;                        // 0x098
	uint32 statdonglreaddresshireg;                     // 0x09c
	DECL_DMA64ADDR(txintraddr);                         // 0x0a0 - 0x0a7
	uint32 txintrval;                                   // 0x0a8
	DECL_DMA64ADDR(rxintraddr);                         // 0x0ac - 0x0b3
	uint32 rxintrval;                                   // 0x0b4
	DECL_DMA64ADDR(cplintr_tx_addr);                    // 0x0b8 - 0x0bf
	uint32 cplintr_tx_val;                              // 0x0c0
	uint32 intr_control;                                // 0x0c4
	uint32 PAD[2];                                      // 0x0c8 - 0x0cf
	uint32 module_clk_enable;                           // 0x0d0
	uint32 module_clkgating_enable;                     // 0x0d4
	uint32 module_reset;                                // 0x0d8
	uint32 module_clkavail;                             // 0x0dc
	uint32 module_clkext;                               // 0x0e0
	uint32 module_enable;                               // 0x0e4
	uint32 module_idle;                                 // 0x0e8
	uint32 PAD;                                         // 0x0ec
	uint32 dmatxsel;                                    // 0x0f0
	uint32 dmarxsel;                                    // 0x0f4
	uint32 dmaarbcfg;                                   // 0x0f8
	uint32 PAD;                                         // 0x0fc
	uint32 gpiomuxcfg;                                  // 0x100
	uint32 gpioout;                                     // 0x104
	uint32 gpiooe;                                      // 0x108
	DECL_RSDB_CORES(mac_base_addr_core);                // 0x10c - 0x113
	uint32 mac_frmtxstatus;                             // 0x114
	uint32 mac_dma_ptr;                                 // 0x118
	uint32 mac_ind_xmtptr;                              // 0x11c
	uint32 mac_ind_qsel;                                // 0x120
	uint32 PAD[3];                                      // 0x124 - 0x12f
	uint32 hwa2hwcap;                                   // 0x130
	uint32 hwa2swpkt_high32_pa;                         // 0x134
	uint32 hwa2pciepc_pkt_high32_pa;                    // 0x138
	uint32 PAD;                                         // 0x13c
	uint32 dma_sof_status;                              // 0x140
	uint32 dma_eof_status;                              // 0x144
	uint32 PAD[6];                                      // 0x148 - 0x15f
	hwa_cplintr_t cplintr_rx[4];                        // 0x160 - 0x19f
	uint32 pageintstatus;                               // 0x1a0
	uint32 pageintmask;                                 // 0x1a4
	uint32 PAD[2];                                      // 0x1a8 - 0x1af
	uint32 datacaptureaddrlo;                           // 0x1b0
	uint32 datacaptureaddrhi;                           // 0x1b4
	uint32 datacapturecfg;                              // 0x1b8
	uint32 datacapturectrl;                             // 0x1bc
	uint32 datacapturewridx;                            // 0x1c0
	uint32 dcdataloading;                               // 0x1c4
	uint32 dcmaskbit;                                   // 0x1c8
	uint32 dctriggersig;                                // 0x1cc
	uint32 PAD[4];                                      // 0x1d0 - 0x1df
	uint32 dmatx0ctrl;                                  // 0x1e0
	uint32 dmatx1ctrl;                                  // 0x1e4
	uint32 dmarx0ctrl;                                  // 0x1e8
	uint32 dmarx1ctrl;                                  // 0x1ec
	uint32 dmatx2ctrl;                                  // 0x1f0
	uint32 dmarx2ctrl;                                  // 0x1f4
	uint32 PAD[2];                                      // 0x1f8 - 0x1ff
} hwa_common_regs_t;                                    // 0x200 512B 128Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_BM REGISTERS: hwa_bm_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_bm_regs {                   // 0x000
	uint32 alloc_index;                                 // 0x000
	DECL_DMA64ADDR(alloc_addr);                         // 0x004 - 0x00b
	uint32 dealloc_index;                               // 0x00c
	uint32 dealloc_status;                              // 0x010
	uint32 buffer_config;                               // 0x014
	DECL_DMA64ADDR(pool_start_addr);                    // 0x016 - 0x01f
	uint32 bm_ctrl;                                     // 0x020
	uint32 PAD[7];                                      // 0x024 - 0x03f
} hwa_bm_regs_t;                                        // 0x040  64B  16Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_TX REGISTERS: hwa_tx_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_tx_regs {                   // 0x000
	uint32 txpost_config;                               // 0x000
	uint32 txpost_wi_ctrl;                              // 0x004
	uint32 fw_cmdq_lazy_intr;                           // 0x008
	uint32 txpost_ethr_type;                            // 0x00c
	uint32 fw_cmdq_base_addr;                           // 0x010
	uint32 fw_cmdq_wr_idx;                              // 0x014
	uint32 fw_cmdq_rd_idx;                              // 0x018
	uint32 fw_cmdq_ctrl;                                // 0x01c
	hwa_tx_pkt_ch_t tx_pkt_ch[HWA_TX_PKT_CH_MAX];       // 0x020 - 0x0bf
	uint32 pkt_chq_base_addr;                           // 0x0c0
	uint32 pkt_chq_wr_idx;                              // 0x0c4
	uint32 pkt_chq_rd_idx;                              // 0x0c8
	uint32 pkt_chq_ctrl;                                // 0x0cc
	uint32 txpost_frc_base_addr;                        // 0x0d0
	uint32 pktdealloc_ring_addr;                        // 0x0d4
	uint32 pktdealloc_ring_wrindex;                     // 0x0d8
	uint32 pktdealloc_ring_rdindex;                     // 0x0dc
	uint32 pktdealloc_ring_depth;                       // 0x0e0
	uint32 pktdealloc_ring_lazyintrconfig;              // 0x0e4
	uint32 pkt_ch_valid;                                // 0x0e8
	uint32 pkt_ch_flowid_reg[HWA_TX_PKT_CH_MAX / 2];    // 0x0ec - 0x0fb
	uint32 dma_desc_template;                           // 0x0fc
	uint32 h2d_wr_ind_array_base_addr;                  // 0x100
	uint32 h2d_rd_ind_array_base_addr;                  // 0x104
	DECL_DMA64ADDR(h2d_rd_ind_array_host_base_addr);    // 0x108 - 0x10f
	uint32 txplavg_weights_reg;                         // 0x110
	uint32 txp_host_rdidx_update_reg;                   // 0x114
	uint32 txpost_status_reg;                           // 0x118
	uint32 txpost_status_reg2;                          // 0x11c
	uint32 pktdeallocmgr_tfrstatus;                     // 0x120
	uint32 pktdealloc_localfifo_cfg_status;             // 0x124
	uint32 txpost_cfg1;                                 // 0x128
	uint32 PAD;                                         // 0x12c
	uint32 txpost_aggr_config;                          // 0x130
	uint32 txpost_aggr_wi_ctrl;                         // 0x134
	uint32 PAD[2];                                      // 0x138 - 0x13f
	uint32 txpost_debug_reg;                            // 0x140
	uint32 PAD[3];                                      // 0x144 - 0x14f
	uint32 hwapp_config;                                // 0x150
	uint32 PAD[27];                                     // 0x154 - 0x1bf
} hwa_tx_regs_t;                                        // 0x1c0 448B 112Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_TXDMA REGISTERS: hwa_txdma_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_txdma_regs {                // 0x000
	uint32 hwa_txdma2_cfg1;                             // 0x000
	uint32 hwa_txdma2_cfg2;                             // 0x004
	uint32 hwa_txdma2_cfg3;                             // 0x008
	uint32 hwa_txdma2_cfg4;                             // 0x00c
	uint32 state_sts;                                   // 0x010
	uint32 state_sts2;                                  // 0x014
	uint32 state_sts3;                                  // 0x018
	uint32 PAD;                                         // 0x01c
	DECL_DMA64ADDR(sw2hwa_tx_pkt_chain_q_base_addr);    // 0x020 - 0x027
	uint32 sw2hwa_tx_pkt_chain_q_wr_index;              // 0x028
	uint32 sw2hwa_tx_pkt_chain_q_rd_index;              // 0x02c
	uint32 sw2hwa_tx_pkt_chain_q_ctrl;                  // 0x030
	uint32 PAD[3];                                      // 0x034 - 0x3f
	uint32 fifo_index;                                  // 0x040
	DECL_DMA64ADDR(fifo_base_addr);                     // 0x044 - 0x4b
	uint32 fifo_wr_index;                               // 0x04c
	uint32 fifo_rd_index;                               // 0x050
	uint32 fifo_depth;                                  // 0x054
	uint32 fifo_attrib;                                 // 0x058
	uint32 PAD;                                         // 0x05c
	DECL_DMA64ADDR(aqm_base_addr);                      // 0x060 - 0x067
	uint32 aqm_wr_index;                                // 0x068
	uint32 aqm_rd_index;                                // 0x06c
	uint32 aqm_depth;                                   // 0x070
	uint32 aqm_attrib;                                  // 0x074
	uint32 PAD[2];                                      // 0x078 - 0x07f
	uint32 sw_tx_pkt_nxt_h;                             // 0x080
	uint32 dma_desc_template_txdma;                     // 0x084
	uint32 PAD[2];                                      // 0x088 - 0x08f
	uint32 pp_pageout_cfg;                              // 0x090
	uint32 pp_pageout_sts;                              // 0x094
	uint32 pp_pagein_cfg;                               // 0x098
	uint32 pp_pagein_sts;                               // 0x09c
	uint32 pp_pageout_dataptr_th;                       // 0x0a0
	uint32 pp_page_debug;                               // 0x0a4
	uint32 pp_txdma_dma_template;                       // 0x0a8
	uint32 PAD[21];                                     // 0x0ac - 0x0ff
} hwa_txdma_regs_t;                                     // 0x100 256B  64Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_TX_STATUS REGISTERS: hwa_tx_status_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_tx_status_regs {            // 0x000
	DECL_DMA64ADDR(tseq_base);                          // 0x000 - 0x007
	uint32 tseq_size;                                   // 0x008
	uint32 tseq_wridx;                                  // 0x00c
	uint32 tseq_rdidx;                                  // 0x010
	uint32 tse_ctl;                                     // 0x014
	uint32 tse_sts;                                     // 0x018
	uint32 txs_debug_reg;                               // 0x01c
	uint32 dma_desc_template_txs;                       // 0x020
	uint32 txe_cfg1;                                    // 0x024
	uint32 tse_axi_base;                                // 0x028
	uint32 tse_axi_ctl;                                 // 0x02c
	uint32 PAD[20];                                     // 0x030 - 0x7f
} hwa_tx_status_regs_t;                                 // 0x080 128B  32Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_RX REGISTERS: hwa_rx_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_rx_regs {                   // 0x000
	DECL_RXPATH_RING(rxpsrc);                           // 0x000 - 0x017
	uint32 rxpsrc_seqnum_cfg;                           // 0x018
	uint32 rxpsrc_seqnum_status;                        // 0x01c
	DECL_DMA64ADDR(rxpsrc_rdindexupd_addr);             // 0x020 - 0x027
	uint32 rxpsrc_status;                               // 0x028
	uint32 rxpsrc_ring_hwa2cfg;                         // 0x02c
	DECL_RXPATH_RING(rxpdest);                          // 0x030 - 0x047
	uint32 PAD[4];                                      // 0x048 - 0x057
	uint32 rxpdest_status;                              // 0x058
	uint32 PAD[1];                                      // 0x05c - 0x05f
	DECL_RXPATH_FIFO(d0dest);                           // 0x060 - 0x08f
	DECL_RXPATH_FIFO(d1dest);                           // 0x090 - 0x0bf
	DECL_RXPATH_RING(d11bdest);                         // 0x0c0 - 0x0d7
	uint32 d11bdest_threshold_l1l0;                     // 0x0d8
	uint32 d11bdest_threshold_l2;                       // 0x0dc
	uint32 d11bdest_ring_wrindex_dir;                   // 0x0e0
	uint32 pagein_status;                               // 0x0e4
	uint32 recycle_status;                              // 0x0e8
	uint32 recycle_cfg;                                 // 0x0ec
	DECL_RXPATH_RING(freeidxsrc);                       // 0x0f0 - 0x107
	uint32 debug_d11bdest_err;                          // 0x108
	uint32 debug_d11bdest_seq;                          // 0x10c
	uint32 debug_tail_err;                              // 0x110
	uint32 debug_link_err;                              // 0x114
	uint32 pagein_cfg;                                  // 0x118
	uint32 PAD[1];                                      // 0x11c - 0x11f
	uint32 rxpmgr_cfg;                                  // 0x120
	uint32 PAD[4];                                      // 0x124 - 0x133
	uint32 rxpmgr_tfrstatus;                            // 0x134
	uint32 d0mgr_tfrstatus;                             // 0x138
	uint32 d1mgr_tfrstatus;                             // 0x13c
	uint32 d11bmgr_tfrstatus;                           // 0x140
	uint32 freeidxmgr_tfrstatus;                        // 0x144
	uint32 rxp_localfifo_cfg_status;                    // 0x148
	uint32 PAD[1];                                      // 0x14c - 0x14f
	uint32 d0_localfifo_cfg_status;                     // 0x150
	uint32 d1_localfifo_cfg_status;                     // 0x154
	uint32 d11b_localfifo_cfg_status;                   // 0x158
	uint32 freeidx_localfifo_cfg_status;                // 0x15c
	uint32 mac_counter_ctrl;                            // 0x160
	uint32 mac_counter_status;                          // 0x164
	uint32 fw_alert_cfg;                                // 0x168
	uint32 fw_rxcompensate;                             // 0x16c
	uint32 rxfill_ctrl0;                                // 0x170
	uint32 rxfill_ctrl1;                                // 0x174
	uint32 rxfill_compresslo;                           // 0x178
	uint32 rxfill_compresshi;                           // 0x17c
	DECL_DMA64ADDR(rxfill_desc0_templ);                 // 0x180 - 0x187
	DECL_DMA64ADDR(rxfill_desc1_templ);                 // 0x188 - 0x18f
	uint32 rxfill_status0;                              // 0x190
	uint32 rph_reserve_cfg;                             // 0x194
	uint32 rph_sw_buffer_addr;                          // 0x198
	uint32 rph_reserve_req;                             // 0x19c
	uint32 rph_reserve_resp;                            // 0x1a0
	uint32 debug_intstatus;                             // 0x1a4
	uint32 debug_errorstatus;                           // 0x1a8
	uint32 debug_hwa2status;                            // 0x1ac
	uint32 debug_hwa2errorstatus;                       // 0x1b0
	uint32 debug_freeidx_err;                           // 0x1b4
	uint32 debug_freeidx_cnt;                           // 0x1b8
	uint32 debug_pagein_cnt;                            // 0x1bc
	uint32 debug_pagein_time;                           // 0x1c0
	uint32 PAD[7];                                      // 0x1c4 - 0x1df
} hwa_rx_regs_t;                                        // 0x1e0 480B 120Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_CPL REGISTERS: hwa_cpl_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_cpl_regs {                  // 0x000
	uint32 ce_sts;                                      // 0x000
	uint32 ce_bus_err;                                  // 0x004
	uint32 PAD[2];                                      // 0x008 - 0x00f
	hwa_cpl_cedq_t cedq[HWA_CPL_CEDQ_MAX];              // 0x010 - 0x0af
	// SWAP THESE TWO and use DECL_DMA64ADDR !!!
	uint32 host_wridx_addr_h;                           // 0x0b0
	uint32 host_wridx_addr_l;                           // 0x0b4
	uint32 cpl_dma_config;                              // 0x0b8
	uint32 dma_desc_template_cpl;                       // 0x0bc
	uint32 PAD[80];                                     // 0x0c0 - 0x1ff
} hwa_cpl_regs_t;                                       // 0x200 512B 128Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_DMA REGISTERS: hwa_dma_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_dma_regs {                  // 0x000
	uint32 corecontrol;                                 // 0x000
	uint32 corecapabilities;                            // 0x004
	uint32 intcontrol;                                  // 0x008
	uint32 PAD[5];                                      // 0x00c - 0x01f
	uint32 intstatus;                                   // 0x020
	uint32 PAD[3];                                      // 0x024 - 0x02f
	hwa_dma_chint_t chint[HWA_DMA_CHANNELS];            // 0x030 - 0x03f
	uint32 PAD[5];                                      // 0x040 - 0x053
	uint32 chintrcvlazy[HWA_DMA_CHANNELS];              // 0x054 - 0x05b
	uint32 PAD[7];                                      // 0x05c - 0x077
	uint32 clockctlstatus;                              // 0x078
	uint32 workaround;                                  // 0x07c
	uint32 powercontrol;                                // 0x080
	uint32 gpioselect;                                  // 0x084
	uint32 gpiooutout;                                  // 0x088
	uint32 gpiooe;                                      // 0x08c
	uint32 PAD[28];                                     // 0x090 - 0x0ff
	hwa_dma_channel_t channels[HWA_DMA_CHANNELS];       // 0x100 - 0x17f
	uint32 PAD[32];                                     // 0x180 - 0x1ff
} hwa_dma_regs_t;                                       // 0x200 512B 128Regs

/*
 * -----------------------------------------------------------------------------
 * HWA_PAGER REGISTERS: hwa_pager_reg_defs.h
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_pager_regs {                // 0x000
	uint32          pp_pager_cfg;                       // 0x000
	uint32          pp_pktctx_size;                     // 0x004
	uint32          pp_pktbuf_size;                     // 0x008
	uint32          PAD[5];                             // 0x00c - 0x01f
	hwa_pp_ring_t   pagein_req_ring;                    // 0x020 - 0x037
	uint32          PAD[2];                             // 0x038 - 0x03f
	hwa_pp_ring_t   pagein_rsp_ring;                    // 0x040 - 0x057
	uint32          pagein_intstatus;                   // 0x058
	uint32          PAD;                                // 0x05c
	hwa_pp_ring_t   pageout_req_ring;                   // 0x060 - 0x077
	uint32          PAD[2];                             // 0x078 - 0x07f
	hwa_pp_ring_t   pageout_rsp_ring;                   // 0x080 - 0x097
	uint32          pageout_intstatus;                  // 0x098
	uint32          PAD;                                // 0x09c
	hwa_pp_ring_t   pagemgr_req_ring;                   // 0x0a0 - 0x0b7
	uint32          PAD[2];                             // 0x0b8 - 0x0bf
	hwa_pp_ring_t   pagemgr_rsp_ring;                   // 0x0c0 - 0x0d7
	uint32          pagemgr_intstatus;                  // 0x0d8
	uint32          PAD;                                // 0x0dc
	hwa_pp_ring_t   freepkt_req_ring;                   // 0x0e0 - 0x0f7
	uint32          PAD[2];                             // 0x0f8 - 0x0ff
	hwa_pp_ring_t   freerph_req_ring;                   // 0x100 - 0x117
	uint32          PAD[2];                             // 0x118 - 0x11f
	uint32          rx_alloc_transaction_id;            // 0x120
	uint32          rx_free_transaction_id;             // 0x124
	uint32          tx_alloc_transaction_id;            // 0x128
	uint32          tx_free_transaction_id;             // 0x12c
	uint32          PAD[4];                             // 0x130 - 0x13f
	hwa_pp_pool_t   hostpktpool;                        // 0x140 - 0x15f
	hwa_pp_pool_t   dnglpktpool;                        // 0x160 - 0x17f
	uint32          pagerbm_intstatus;                  // 0x180
	uint32          PAD[3];                             // 0x184 - 0x18f
	uint32          pp_dma_descr_template;              // 0x190
	uint32          pp_pagein_req_ddbmth;               // 0x194
	uint32          pp_dma_descr_template_2;            // 0x198
	uint32          PAD;                                // 0x19c
	uint32          pp_apkt_cfg;                        // 0x1a0
	uint32          pp_rx_apkt_cfg;                     // 0x1a4
	uint32          pp_fpkt_cfg;                        // 0x1a8
	uint32          pp_phpl_cfg;                        // 0x1ac
	uint32          PAD[4];                             // 0x1b0 - 0x1bf
	uint32          pp_apkt_sts_dbg;                    // 0x1c0
	uint32          pp_rx_apkt_sts_dbg;                 // 0x1c4
	uint32          pp_fpkt_sts_dbg;                    // 0x1c8
	uint32          pp_tb_sts_dbg;                      // 0x1cc
	uint32          pp_push_sts_dbg;                    // 0x1d0
	uint32          pp_pull_sts_dbg;                    // 0x1d4
	uint32          pp_fpkt_sts_dbg2;                   // 0x1d8
	uint32          pp_fpkt_sts_dbg3;                   // 0x1dc
	uint32          PAD[8];                             // 0x1e0 - 0x1ff
	hwa_pp_pkt_inuse_t hostpkt_cnt;                     // 0x200 - 0x20b
	uint32          PAD;                                // 0x20c
	hwa_pp_pkt_inuse_t dngltpkt_cnt;                    // 0x210 - 0x21b
	uint32          PAD;                                // 0x21c
	hwa_pp_pkt_inuse_t hostpkt_hwm;                     // 0x220 - 0x22b
	uint32          PAD;                                // 0x22c
	hwa_pp_pkt_inuse_t dngltpkt_hwm;                    // 0x230 - 0x23b
	uint32          PAD;                                // 0x23c
	uint32          pp_pagein_req_stats;                // 0x240
	uint32          pp_pageout_req_stats;               // 0x244
	uint32          pp_pagealloc_req_stats;             // 0x248
	uint32          pp_pagefree_req_stats;              // 0x24c
	uint32          pp_pagefreerph_req_stats;           // 0x250
} hwa_pager_regs_t;                                     // 0x200 512B 128Reg

/*
 * -----------------------------------------------------------------------------
 * HWA REGISTER MAP
 * -----------------------------------------------------------------------------
 */
typedef volatile struct hwa_regs {                      // 0x000
	// Common to TX and RX
	uint32                  PAD[64];                    // 0x000 - 0x0ff
	hwa_top_regs_t          top;                        // 0x100 - 0x1ff
	hwa_common_regs_t       common;                     // 0x200 - 0x3ff

	//  TX Block
	hwa_bm_regs_t           tx_bm;                      // 0x400 - 0x43f
	hwa_tx_regs_t           tx;                         // 0x440 - 0x5ff
	hwa_txdma_regs_t        txdma;                      // 0x600 - 0x6ff
	hwa_tx_status_regs_t    tx_status[HWA_TX_CORES_MAX_HW]; // 0x700 - 0x7ff

	// RX Blocks
	hwa_bm_regs_t           rx_bm;                      // 0x800 - 0x83f
	hwa_rx_regs_t           rx_core[HWA_RX_CORES_MAX_HW];  // 0x840 - 0xa1f
	                                                    // 0xa20 - 0xbff
	// Completion Blocks
	hwa_cpl_regs_t          cpl;                        // 0xc00 - 0xdff

	// DMA Blocks
	hwa_dma_regs_t          dma;                        // 0xe00 - 0xfff

	// ------------------------------------------------ 4KB boundary

#ifdef HWA_PKTPGR_BUILD
	// Packet Pager Block
	hwa_pager_regs_t        pager;                      // 0x1000 - 0x11ff
#endif /* HWA_PKTPGR_BUILD */

} hwa_regs_t;

/*
 * -----------------------------------------------------------------------------
 * HWA HC HIN REGISTER MAP: hc_hin_reg_defs.h
 * HWA PSM REGISTER MAP: hc_psm_reg_defs.h
 * -----------------------------------------------------------------------------
 */

// MAC HandCoded Host Interface: objaddr and objdata for indirect access
typedef volatile struct hc_hin_regs {
	uint32 PAD[22];                                     // 0x000 - 0x057
	uint32 ctdma_ctl;                                   // 0x058
	uint32 PAD[65];                                     // 0x05c - 0x15c
	uint32 objaddr;                                     // 0x160
	uint32 objdata;                                     // 0x164
	uint32 PAD[150];                                    // 0x168 - 0x3bf
	uint32 rxfilteren;                                  // 0x3c0
	uint32 rxhwactrl;                                   // 0x3c4
	uint32 PAD[250];                                    // 0x3c8 - 0x7af
	uint16 PAD;                                         // 0x7b0
	uint16 hwa_macif_ctl;                               // 0x7b2
} hc_hin_regs_t;

#endif /* _HWA_REGS_H */
