/*
 * HWA library routines for MAC facing blocks: 1b, 2a, 3b, and 4a
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
 */

/*
 * README, how to dump hwa
 * Usage:
 * wl dump hwa [-b <blocks> -v -r -s -f <HWA fifos> -h]
 *	b: specific hwa blocks that you are interesting
 *	<blocks>: <top cmn dma 1a 1b 2a 2b 3a 3b 4a 4b>
 *	v: verbose, to dump more information
 *	r: dump registers
 *	s: dump txfifo shadow
 *	f: specific fifos that you are interesting
 *	<HWA fifos>: <0..69>
 *	h: help
 * NOTE: The <HWA fifos> is physical fifo index.
 *	More FIFO mapping info is in WLC_HW_MAP_TXFIFO.
 * NOTE: Dump 3b TxFIFO context is dangerous, it may cause AXI timeout.
 *	(Use it only for debugging purpose)
 * Example: Dump HWA 3b block info for fifos 65, 67.
 *	wl dump hwa [-b 3b -v -r -f 65 67]
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <wlc.h>
#include <wlc_tx.h>
#include <wlc_rx.h>
#include <wlc_ampdu_rx.h>
#include <wlc_amsdu.h>
#include <wlc_hw.h>
#include <wlc_hw_priv.h>
#include <wlc_bmac.h>
#include <wlc_pktfetch.h>
#include <phy_rssi_api.h>
#ifdef DONGLEBUILD
#include <wl_rte_priv.h>
#endif /* DONGLEBUILD */
#include <bcmmsgbuf.h>
#include <hwa_lib.h>
#include "hnddma_priv.h"
#include <wlc_event_utils.h>
#ifdef WLCFP
#include <wlc_cfp.h>
#endif /* WLCFP */
#include <bcmudp.h>
#ifdef HNDBME
#include <hndbme.h>
#endif
#include <bcmhme.h>
#ifdef BCM_BUZZZ
#include <bcm_buzzz.h> // Full Dongle
#endif /* BCM_BUZZZ */
#ifdef WLDURATION
#include <wlc_duration.h>
#endif
#if defined(STS_XFER)
#include <wlc_sts_xfer.h>
#endif /* STS_XFER */
#include <wlc_ulmu.h>

#ifdef HWA_QT_TEST
uint32 hwa_kflag = 0;
#endif

typedef struct wl_info wl_info_t; // forward declaration

/* debug/trace */
uint32 hwa_msg_level =
#if defined(BCMDBG) || defined(HWA_DUMP)
	HWA_ERROR_VAL;
#else
	0;
#endif /* BCMDBG */

#if defined(HWA_PKTPGR_BUILD)
/* specific Packet Pager bring up debug */
uint32 hwa_pp_dbg =
#if defined(BCMDBG) || defined(HWA_DUMP)
	(HWA_PP_DBG_2A | HWA_PP_DBG_3A | HWA_PP_DBG_3B |
	HWA_PP_DBG_4A | HWA_PP_DBG_4B | HWA_PP_DBG_MGR);
#else
	0;
#endif /* BCMDBG */
#endif /* HWA_PKTPGR_BUILD */

#if defined(HWA_DUMP)
uint32 hwa_pktdump_level = 0;
#endif

#if defined(HWA_PKTPGR_BUILD)
#if defined(HWA_PKTPGR_D11B_AUDIT_ENABLED)
#define D11B_AUDIT_ENABLED(corerev)	TRUE
#else
// WAR for 6715Ax  fetch all d11b resource and free.
#define D11B_AUDIT_ENABLED(corerev)	HWAREV_LE(corerev, 132)
#endif /* HWA_PKTPGR_D11B_AUDIT_ENABLED */
#endif /* HWA_PKTPGR_BUILD */

#ifdef HWA_RXFILL_BUILD

#ifdef HWA_RXFILL_RXFREE_AUDIT_ENABLED

/* Audits the RX buffer id between HWA and SW */
static inline void
_hwa_rxfill_rxfree_audit(hwa_rxfill_t *rxfill, uint32 bufidx, const bool alloc)
{
	// Always +1, because the bufidx can be zero.
	bufidx += 1;

	if (bufidx > HWA_RXPATH_PKTS_MAX) {
		HWA_ERROR(("_hwa_rxfill_rxfree_audit: Invalid bufidx<%d>, %s\n", bufidx,
			alloc ? "alloc" : "free"));
		return;
	}

	if (alloc) {
		if (!bcm_mwbmap_isfree(rxfill->rxfree_map, bufidx)) {
			HWA_ERROR(("RxBM audit: Get duplicate rxbuffer<%d> "
				"from RxBM\n", bufidx));
			return;
		}
		bcm_mwbmap_force(rxfill->rxfree_map, bufidx);
	} else {
		if (bcm_mwbmap_isfree(rxfill->rxfree_map, bufidx)) {
			HWA_ERROR(("RxBM audit: Double free rxbuffer<%d> "
				"to RxBM\n", bufidx));
			return;
		}
		bcm_mwbmap_free(rxfill->rxfree_map, bufidx);
	}
} // _hwa_rxfill_rxfree_audit

#if defined(HWA_PKTPGR_BUILD)
/* Audits the RX buffer id between HWA and SW */
void
hwa_rxfill_rxfree_audit(struct hwa_dev *dev, uint32 core,
	void *rx_buffer, const bool alloc)
{
	uint32 bufidx;
	hwa_pktpgr_t *pktpgr;

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Audit parameters and pre-conditions
	HWA_ASSERT(rx_buffer != NULL);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Inside the DDBM pool.
	HWA_ASSERT(((uint32)HWA_TABLE_INDX(hwa_pp_lbuf_t, pktpgr->dnglpktpool_mem,
		rx_buffer)) < pktpgr->dnglpktpool_max);

	// Align at size of hwa_pp_lbuf_t
	HWA_ASSERT((((uintptr)rx_buffer - HWA_PTR2UINT(
		pktpgr->dnglpktpool_mem)) % sizeof(hwa_pp_lbuf_t)) == 0);

	// Convert rxbuffer pointer to its index within DDBM
	bufidx = HWA_TABLE_INDX(hwa_pp_lbuf_t, pktpgr->dnglpktpool_mem, rx_buffer);
	HWA_ASSERT(bufidx < pktpgr->dnglpktpool_max);

	// Pass to audit bitmap
	_hwa_rxfill_rxfree_audit(&dev->rxfill, bufidx, alloc);

} // hwa_rxfill_rxfree_audit

#else
/* Audits the RX buffer id between HWA and SW */
void
hwa_rxfill_rxfree_audit(struct hwa_dev *dev, uint32 core,
	void *rx_buffer, const bool alloc)
{
	uint32 bufidx;
	hwa_rxfill_t *rxfill;
	HWA_DEBUG_EXPR(uint32 offset);

	rxfill = &dev->rxfill;

	// Audit parameters and pre-conditions
	HWA_ASSERT(rx_buffer != NULL);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Audit rx_buffer, wrong rx_buffer causes 1b stuck.
	HWA_ASSERT(rx_buffer >= dev->rx_bm.memory);
	HWA_ASSERT(rx_buffer <= ((void *)((hwa_rxbuffer_t*)dev->rx_bm.memory) +
		(dev->rx_bm.pkt_total - 1)));
	HWA_DEBUG_EXPR({
		offset = ((char *)rx_buffer) - ((char *)dev->rx_bm.memory);
		HWA_ASSERT((offset % dev->rx_bm.pkt_size) == 0);
	});

	// Convert rxbuffer pointer to its index within Rx Buffer Manager
	bufidx = HWA_TABLE_INDX(hwa_rxbuffer_t, dev->rx_bm.memory, rx_buffer);

	_hwa_rxfill_rxfree_audit(rxfill, bufidx, alloc);

} // hwa_rxfill_rxfree_audit
#endif /* HWA_PKTPGR_BUILD */

#endif /* HWA_RXFILL_RXFREE_AUDIT_ENABLED */

hwa_rxfill_t *
BCMATTACHFN(hwa_rxfill_attach)(hwa_dev_t *dev)
{
	hwa_rxfill_t *rxfill;

	HWA_ASSERT(dev != (hwa_dev_t*)NULL);

	// Setup locals
	rxfill = &dev->rxfill;

	// Reset PageIn Rx recv histogram
	HWA_PKTPGR_EXPR(HWA_BCMDBG_EXPR(memset(rxfill->rx_recv_histogram, 0,
		sizeof(rxfill->rx_recv_histogram))));

#ifdef HWA_RXFILL_RXFREE_AUDIT_ENABLED

	// Handler for hwa rx buffer index
	rxfill->rxfree_map = bcm_mwbmap_init(dev->osh, HWA_RXPATH_PKTS_MAX + 1);
	if (rxfill->rxfree_map == NULL) {
		HWA_ERROR(("rxfree_map for audit allocation failed\n"));
		goto failure;
	}
	bcm_mwbmap_force(rxfill->rxfree_map, 0); /* id=0 is invalid */

	return rxfill;

failure:
	hwa_rxfill_detach(rxfill);
	HWA_WARN(("%s attach failure\n", HWA1b));

	return ((hwa_rxfill_t*)NULL);
#else

	return rxfill;

#endif /* HWA_RXFILL_RXFREE_AUDIT_ENABLED */
}

void
BCMATTACHFN(hwa_rxfill_detach)(hwa_rxfill_t *rxfill)
{
#ifdef HWA_RXFILL_RXFREE_AUDIT_ENABLED
	hwa_dev_t *dev;

	HWA_FTRACE(HWA1b);

	if (rxfill == (hwa_rxfill_t*)NULL)
		return;

	// Audit pre-conditions
	dev = HWA_DEV(rxfill);

	// Handler for hwa rx buffer index
	if (rxfill->rxfree_map != NULL) {
		bcm_mwbmap_fini(dev->osh, rxfill->rxfree_map);
		rxfill->rxfree_map = NULL;
	}
#endif /* HWA_RXFILL_RXFREE_AUDIT_ENABLED */

	return;
}

/*
 * -----------------------------------------------------------------------------
 * Section: HWA1b RxFill block ... may be used in Dongle and NIC mode
 * -----------------------------------------------------------------------------
 *
 * CONFIG:
 * -------
 * - hwa_rxfill_fifo_attach() may be invoked by WL layer, after allocating the
 *   MAC FIFO0 and/or FIFO1 DMA descriptor rings. FIFO DMA descriptor rings will
 *   continue to be allocated by WL layer using hnddma library dma_ringalloc().
 *   WL layer will pass the MD descriptor ring parameters to HWA1b RxFill block
 *   at attach time using hwa_rxfill_fifo_attach(). No registers are configured
 *   in this HWA1b RxFill library function, as hwa_attach() may not yet be
 *   invoked.
 *
 * - hwa_rxfill_init() may be invoked AFTER hwa_rxfill_fifo_attach() is invoked.
 *   H2S and S2H interfaces are allocated and the HWA1b RxFill block is
 *   configured. RxFill relies on Rx Buffer Manager.
 *
 * RUNTIME:
 * --------
 * - hwa_rxfill_rxbuffer_free() handles rxBuffer free requests from upper layer.
 *   An RxBuffer may be paired with a host side buffer context saved in the RPH.
 *
 * - hwa_rxfill_rxbuffer_process() is invoked by the H2S WR index update
 *   interrupt path. The WR index will be updated when the D11 MAC completes
 *   packet reception into the Rx FIFO0 and FIFO1. This function processes the
 *   received packet from RD to WR index, by invoking the bound upper layer
 *   handler. The RPH and the rxBuffer pointer are handed to the upper layer
 *   handler, which may chose to convert to the native network stack packet
 *   context, e.g. mbuf, skbuff, fkbuf, lbuf. In Full Dongle, with .11 to .3
 *   conversion offloaded to the D11 MAC, the excplicit conversion to RxLbufFrag
 *   may be skipped and directly RxRordering using RxCpls may be performed.
 *
 * DEBUG:
 * ------
 * hwa_rxfill_fifo_avail() queries HWA for number of available RxBuffers.
 * hwa_rxfill_dump() debug support for HWA1b RxFill block
 *
 * -----------------------------------------------------------------------------
 */

static int hwa_rxfill_bmac_recv(void *context, uintptr arg1, uintptr arg2,
	uint32 core, uint32 arg4);
static int hwa_rxfill_bmac_done(void *context, uintptr arg1, uintptr arg2,
	uint32 core, uint32 rxfifo_cnt);

void // WL layer may directly configure HWA MAC FIFOs
hwa_rxfill_fifo_attach(void *wlc, uint32 core, uint32 fifo,
	uint32 depth, dma64addr_t fifo_addr, uint32 dmarcv_ptr, bool hme_macifs)
{
	hwa_dev_t *dev;
	hwa_rxfill_t *rxfill; // HWA1b RxFill SW state

	HWA_TRACE(("%s PHASE MAC FIFO ATTACH wlc<%p> core<%u> fifo<%u>"
		" depth<%u> addr<0x%08x,0x%08x> dmarecv_ptr<0x%08x>\n",
		HWA1b, wlc, core, fifo,
		depth, fifo_addr.loaddr, fifo_addr.hiaddr, dmarcv_ptr));

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit

	// Audit parameters and pre-conditions
	HWA_ASSERT(wlc != (void*)NULL);
	HWA_ASSERT(core < HWA_RX_CORES);
	HWA_ASSERT(fifo < HWA_RXFIFO_MAX);
	HWA_ASSERT(depth != 0U);
	HWA_ASSERT((fifo_addr.loaddr | fifo_addr.hiaddr) != 0U);

	// Setup locals
	rxfill = &dev->rxfill;

	// Save settings in local state
	rxfill->wlc[core] = wlc;
	rxfill->fifo_depth[core][fifo] = depth;
	rxfill->dmarcv_ptr[core][fifo] = dmarcv_ptr;
	rxfill->fifo_addr[core][fifo].loaddr = fifo_addr.loaddr;
	rxfill->fifo_addr[core][fifo].hiaddr = fifo_addr.hiaddr;
	rxfill->hme_macifs[core][fifo] = hme_macifs;

	// Mark FIFO as initialized. Used in construction of H2S RxFIFO interface
	rxfill->inited[core][fifo] = TRUE;

} // hwa_rxfill_fifo_attach

int // HWA1b: Allocate resources configuration for HWA1b block
hwa_rxfill_preinit(hwa_rxfill_t *rxfill)
{
	hwa_dev_t *dev;
	void *memory;
	uint32 core, depth, mem_sz;
	uint8 rxh_offset;
	hwa_regs_t *regs;

	HWA_FTRACE(HWA1b);

	// Audit pre-conditions
	dev = HWA_DEV(rxfill);

	// Setup locals
	regs = dev->regs;

	// Configure RxFill
	if (dev->host_addressing == HWA_32BIT_ADDRESSING) {
		rxfill->config.rph_size = sizeof(hwa_rxpost_hostinfo32_t);
		rxfill->config.addr_offset =
			OFFSETOF(hwa_rxpost_hostinfo32_t, data_buf_haddr32);
	} else { // HWA_64BIT_ADDRESSING 64bit host
		rxfill->config.rph_size = sizeof(hwa_rxpost_hostinfo64_t);
		rxfill->config.addr_offset =
			OFFSETOF(hwa_rxpost_hostinfo64_t, data_buf_haddr64);
	}

	//These fields will be updated later
	dev->rxfill.config.wrxh_offset = 0;
	dev->rxfill.config.d11_offset = 0;
	dev->rxfill.config.len_offset = ~0;

	// Verify HWA1b block's structures
	HWA_ASSERT(sizeof(hwa_rxfill_rxfree_t) == HWA_RXFILL_RXFREE_BYTES);
	HWA_ASSERT(sizeof(hwa_rxfill_rxfifo_t) == HWA_RXFILL_RXFIFO_BYTES);

#if defined(HWA_PKTPGR_BUILD)

	// Only one core
	core = 0;

	// No RPH in front of RX data buffer.
	rxfill->config.rph_size = 0;

	// Setup D11 offset and SW RXHDR offset
	rxfill->config.d11_offset = WLC_RXHDR_LEN;

	/* XXX
	 * By now, pp_rxlfrag_data_buf_offset is set as 26 (offset 26*4 = 104) in d11hwasim.
	 * This field is for HWA2.2 pager mode only.
	 * For FIFO1 descriptor generation, D1_offset is set to 104 (corresponding to
	 * pp_rxlfrag_data_buf_offset) and d1_len is set to 152. D1_offset and d1_len are for
	 * HWA2.1 and HWA2.2.
	 * NOTE: both offsets are start from rxlfrag base address.
	*/

	// Assume, MAC enforces 8B alignment
	// Assume Pager RxLfrag data buffers start at
	// HWA_RX_PAGEIN_STATUS_PP_RXLFRAG_DATA_BUF_OFFSET will be
	// 8B alignment too.
	rxh_offset = (rxfill->config.rph_size + rxfill->config.d11_offset);
	rxfill->config.wrxh_offset = ROUNDUP(rxh_offset, 8) - rxfill->config.d11_offset;
	// No RPH in front of RX data buffer, so in SW we can use whole size(24+128).
	// NOTE: 152B was used in HWA2.1 RX which has RPH header but in HWA2.2 RPH
	// info is place in Lbuf FRAGINFO so we can repurposed the 12B RPH for other usage.
	rxfill->config.rx_size = HWA_PP_RXLFRAG_DATABUF_LEN - rxfill->config.wrxh_offset;

	HWA_INFO(("%s config rph<%u> offset d11<%u> len<%u> addr<%u>\n",
		HWA1b, dev->rxfill.config.rph_size,
		dev->rxfill.config.d11_offset, dev->rxfill.config.len_offset,
		dev->rxfill.config.addr_offset));

	depth = rxfill->fifo_depth[core][0];
	mem_sz = depth * sizeof(hwa_rxfill_rxfifo_t);

	// D11B ring depth must smaller then pktpg::freerph_req_ring
	// because D11B reclaim by HW may generate size of full D11B depth
	// freerph elements in freerph req ring.
	HWA_ASSERT(depth <= dev->pktpgr.freerph_req_ring.depth);

	if (dev->d11b_axi) {
		// Use HWA internal "D11B" AXI memory address in pager mode.
		// Switch to use external dongle memory as HWA2.1 if you need more
		// than HWA_PKTPGR_D11BDEST_DEPTH
		HWA_ASSERT(depth <= HWA_PKTPGR_D11BDEST_DEPTH);
		memory = HWA_UINT2PTR(void, hwa_axi_addr(dev, HWA_AXI_D11BINT_MEMORY));
		HWA_TRACE(("%s rxfifo_ring @ memory[%p,%u]\n", HWA1b, memory, mem_sz));
	} else {
		// Allocate and initialize H2S "D11BDEST" interface
		// Let's force to 8B alignment in case the fastdma is set.
		if ((memory = MALLOC_ALIGN(dev->osh, mem_sz, 3)) == NULL) {
			HWA_ERROR(("%s rxfifo malloc size<%u> failure\n", HWA1b, mem_sz));
			HWA_ASSERT(memory != (void*)NULL);
			goto failure;
		}
		ASSERT(ISALIGNED(memory, 8));
		bzero(memory, mem_sz);
		HWA_INFO(("%s rxfifo_ring +memory[%p,%u]\n", HWA1b, memory, mem_sz));
	}

	hwa_ring_init(&rxfill->rxfifo_ring[core], "D11", HWA_RXFILL_ID,
		HWA_RING_H2S, HWA_RXFILL_RXFIFO_H2S_RINGNUM, depth, memory,
		&regs->rx_core[core].d11bdest_ring_wrindex,
		&regs->rx_core[core].d11bdest_ring_rdindex);

	if (D11B_AUDIT_ENABLED(dev->corerev)) {
		// Allocate a same "D11BDEST" size audit memory
		if ((memory = MALLOCZ(dev->osh, mem_sz)) == NULL) {
				HWA_ERROR(("%s d11b_audit_table malloc size<%u> failure\n",
					HWA1b, mem_sz));
				HWA_ASSERT(memory != (void*)NULL);
				goto failure;
			}
			bzero(memory, mem_sz);
			HWA_INFO(("%s d11b_audit_table +memory[%p,%u]\n", HWA1b, memory, mem_sz));

			// Setup.
			bcm_ring_init(&rxfill->d11b_audit_state);
			rxfill->d11b_audit_table = memory;
			rxfill->d11b_audit_depth = depth;
			rxfill->d11b_audit_dirty = 0;
	}

#else

	// Setup D11 offset and SW RXHDR offset
	rxfill->config.d11_offset = WLC_RXHDR_LEN;
	// XXX, 43684A0 MAC enforces 8B alignment, assume buffers in RXBM are 8B aligned already.
	// B0 has the same requirement.
	rxh_offset = (rxfill->config.rph_size + rxfill->config.d11_offset);
	rxfill->config.wrxh_offset = ROUNDUP(rxh_offset, 8) - rxfill->config.d11_offset;

	rxfill->config.rx_size = HWA_RXBUFFER_BYTES - rxfill->config.wrxh_offset;

	HWA_TRACE(("%s config rph<%u> offset d11<%u> len<%u> addr<%u>\n",
		HWA1b, dev->rxfill.config.rph_size,
		dev->rxfill.config.d11_offset, dev->rxfill.config.len_offset,
		dev->rxfill.config.addr_offset));

	// FIXME during integration phase ... allocate Split-Lbuf RxBuffers
	// FIXME ignore NIC mode for now ...
	// We need RxBM at 8B alignment.
	mem_sz = HWA_RXBUFFER_BYTES * HWA_RXPATH_PKTS_MAX;
	// We need RxBM at 8B alignment.
	if ((memory = MALLOC_ALIGN(dev->osh, mem_sz, 3)) == NULL) {
		HWA_ERROR(("%s rxbm malloc size<%u> failure\n", HWA00, mem_sz));
		HWA_ASSERT(memory != (void*)NULL);
		goto failure;
	}
	ASSERT(ISALIGNED(memory, 8));
	bzero(memory, mem_sz);

	HWA_TRACE(("%s rxbm +memory[%p,%u]\n", HWA00, memory, mem_sz));
	hwa_bm_config(dev, &dev->rx_bm, "BM RxPATH", HWA_RX_BM,
		HWA_RXPATH_PKTS_MAX, HWA_RXBUFFER_BYTES,
		HWA_PTR2UINT(memory), HWA_PTR2HIADDR(memory), memory);

	rxfill->rxbm_base = dev->rx_bm.pkt_base.loaddr; // loaddr ONLY

	for (core = 0; core < HWA_RX_CORES; core++) {
		// Allocate and initialize S2H "FREEIDXSRC" interface
		depth = HWA_RXFILL_RXFREE_DEPTH;
		mem_sz = depth * sizeof(hwa_rxfill_rxfree_t);
		// (HWA_RXFILL_RXFREE_BYTES == 4B, should be NO)
		if ((memory = MALLOCZ(dev->osh, mem_sz)) == NULL) {
			HWA_ERROR(("%s rxfree malloc size<%u> failure\n", HWA1b, mem_sz));
			HWA_ASSERT(memory != (void*)NULL);
			goto failure;
		}
		HWA_TRACE(("%s rxfree_ring +memory[%p,%u]\n", HWA1b, memory, mem_sz));
		hwa_ring_init(&rxfill->rxfree_ring[core], "RXF", HWA_RXFILL_ID,
			HWA_RING_S2H, HWA_RXFILL_RXFREE_S2H_RINGNUM, depth, memory,
			&regs->rx_core[core].freeidxsrc_ring_wrindex,
			&regs->rx_core[core].freeidxsrc_ring_rdindex);

		// Allocate and initialize H2S "D11BDEST" interface
		depth = rxfill->fifo_depth[core][0];
		mem_sz = depth * sizeof(hwa_rxfill_rxfifo_t);
		// (HWA_RXFILL_RXFIFO_BYTES == 4B, should be NO)
		if ((memory = MALLOCZ(dev->osh, mem_sz)) == NULL) {
			HWA_ERROR(("%s rxfifo malloc size<%u> failure\n", HWA1b, mem_sz));
			HWA_ASSERT(memory != (void*)NULL);
			goto failure;
		}
		HWA_TRACE(("%s rxfifo_ring +memory[%p,%u]\n", HWA1b, memory, mem_sz));
		hwa_ring_init(&rxfill->rxfifo_ring[core], "D11", HWA_RXFILL_ID,
			HWA_RING_H2S, HWA_RXFILL_RXFIFO_H2S_RINGNUM, depth, memory,
			&regs->rx_core[core].d11bdest_ring_wrindex,
			&regs->rx_core[core].d11bdest_ring_rdindex);
	}

	// Override registered dpc callback handler
	hwa_register(dev, HWA_RXFIFO_PROC_CB, dev, hwa_rxfill_bmac_recv);
	hwa_register(dev, HWA_RXFIFO_DONE_CB, dev, hwa_rxfill_bmac_done);

#endif /* HWA_PKTPGR_BUILD */

	return HWA_SUCCESS;

failure:

	hwa_rxfill_free(rxfill);

	return HWA_FAILURE;

}

void // HWA1b: Free resources for HWA1b block
hwa_rxfill_free(hwa_rxfill_t *rxfill)
{
	void *memory;
	uint32 core, mem_sz;
	hwa_dev_t *dev;
	uint8 d11b_axi = 0;

	HWA_FTRACE(HWA1b);

	if (rxfill == (hwa_rxfill_t*)NULL)
		return; // nothing to release done

	// Audit pre-conditions
	dev = HWA_DEV(rxfill);
	HWA_PKTPGR_EXPR(d11b_axi = dev->d11b_axi);

#if !defined(HWA_PKTPGR_BUILD)
	if (dev->rx_bm.memory != (void*)NULL) {
		memory = dev->rx_bm.memory;
		mem_sz = HWA_RXBUFFER_BYTES * HWA_RXPATH_PKTS_MAX;
		HWA_TRACE(("%s rx_bm -memory[%p,%u]\n", HWA1b, memory, mem_sz));
		MFREE(dev->osh, memory, mem_sz);
		dev->rx_bm.memory = (void*)NULL;
	}
#endif /* HWA_PKTPGR_BUILD */

	for (core = 0; core < HWA_RX_CORES; core++) {
#if !defined(HWA_PKTPGR_BUILD)
		// Release resources used by HWA1b RxFill "FREEIDXSRC" interface
		if (rxfill->rxfree_ring[core].memory != (void*)NULL) {
			memory = rxfill->rxfree_ring[core].memory;
			mem_sz = HWA_RXFILL_RXFREE_DEPTH * sizeof(hwa_rxfill_rxfree_t);
			HWA_TRACE(("%s rxfree_ring -memory[%p,%u]\n",
			       HWA1b, memory, mem_sz));
			MFREE(dev->osh, memory, mem_sz);
			hwa_ring_fini(&rxfill->rxfree_ring[core]);
			rxfill->rxfree_ring[core].memory = (void*)NULL;
		}
#endif

		// Release resources used by HWA1b RxFill "D11BDEST" interface
		if (rxfill->rxfifo_ring[core].memory != (void*)NULL) {
			memory = rxfill->rxfifo_ring[core].memory;
			mem_sz = rxfill->fifo_depth[core][0] * sizeof(hwa_rxfill_rxfifo_t);
			HWA_TRACE(("%s rxfifo_ring -memory[%p,%u]\n",
			       HWA1b, memory, mem_sz));
			if (!d11b_axi)
				MFREE(dev->osh, memory, mem_sz);
			hwa_ring_fini(&rxfill->rxfifo_ring[core]);
			rxfill->rxfifo_ring[core].memory = (void*)NULL;

#if defined(HWA_PKTPGR_BUILD)
			// Free "D11BDEST" audit memory
			if (rxfill->d11b_audit_table != (void*)NULL) {
				MFREE(dev->osh, rxfill->d11b_audit_table, mem_sz);
				rxfill->d11b_audit_table = (void*)NULL;
			}
#endif
		}
	}
}

int // HWA1b RxFill: Init RxFill interfaces AFTER MAC FIFOs are attached.
hwa_rxfill_init(hwa_rxfill_t *rxfill)
{
	void *memory;
	uint32 u32, core;
	uint32 ring_cfg, ring_intraggr; // S2H and H2S ring interfaces
	uint32 fifo_cfg, fifo_intraggr; // MAC RX FIFO0 and FIFO1
	hwa_dev_t *dev;
	hwa_regs_t *regs;
	uint16 u16;
	HWA_PKTPGR_EXPR(uint32 pp_desc_burst);
	HWA_PKTPGR_EXPR(uint32 burst_len1);

	// Setup locals
	dev = HWA_DEV(rxfill);
	regs = dev->regs;

	// Make a copy
	rxfill->rxp_data_buf_len = hwa_rxpost_data_buf_len();

	// Confirm that the HWA Rx Buffer Manager is indeed initialized
	// All buffer indices are with respect to Rx Buffer Manager base address
	NO_HWA_PKTPGR_EXPR(HWA_ASSERT(dev->rx_bm.enabled == TRUE));

	// Compute values common to HWA1b RxFill FREEIDXSRC and D11BDEST interfaces

	// D0DEST and D1DEST use same SHIFT MASK values
	fifo_intraggr = (0U
		| BCM_SBF(HWA_RXFILL_FIFO_INTRAGGR_COUNT,
		        HWA_RX_D0DEST_INTRAGGR_SEQNUM_CFG_AGGR_COUNT)
		| BCM_SBF(HWA_RXFILL_FIFO_INTRAGGR_TMOUT,
		        HWA_RX_D0DEST_INTRAGGR_SEQNUM_CFG_AGGR_TIMER)
		| 0U);

	// Descritprs of D0 and D1 are aggregated and transferred to memory in burst.
	HWA_PKTPGR_EXPR({
		uint32 burst_len;
		if (HWAREV_GE(dev->corerev, 133)) {
			/* 8 aggregation for >= Rev133 */
			burst_len = 0;
			burst_len1 = (0U
				| BCM_SBIT(HWA_RX_D1DEST_INTRAGGR_SEQNUM_CFG_PP_DESC_BURST_LEN1)
				| 0U);
		} else {
			/* 4 aggregation for < Rev133 */
			burst_len = 1;
			burst_len1 = 0;
		}

		pp_desc_burst = (0U
			/* If set, descritprs of D0 and D1 are aggregated and transferred
			 * to memory in burst.
			 */
			| BCM_SBIT(HWA_RX_D0DEST_INTRAGGR_SEQNUM_CFG_PP_DESC_BURST)
			/* If set, 4 descritprs of D0 and D1 are aggregated. Otherwise,
			 * 2 descriptors are aggregated.
			 */
			| BCM_SBF(burst_len, HWA_RX_D0DEST_INTRAGGR_SEQNUM_CFG_PP_DESC_BURST_LEN)
			/* Time out value to be used to force the descriptors transferred out.
			 * This is specified in 64 microseconds.
			 */
			| BCM_SBF(16, HWA_RX_D0DEST_INTRAGGR_SEQNUM_CFG_PP_DESC_TIMER)
			| 0U);
	});

	// Same reg layout for both S2H FREEIDXSRC and H2S D11BDEST ring interfaces.
	// Using D11BDEST based SHIFT and MASK macros for programming registers

	// FREEIDXSRC and D11BDEST use same SHIFT and MASK values
	ring_cfg = (0U
		| BCM_SBF(dev->macif_placement, HWA_RX_D11BDEST_RING_CFG_TEMPLATE_NOTPCIE)
		| BCM_SBF(dev->macif_coherency, HWA_RX_D11BDEST_RING_CFG_TEMPLATE_COHERENT)
		| BCM_SBF(0, HWA_RX_D11BDEST_RING_CFG_TEMPLATE_ADDREXT)
		| 0U);

	// ring_intraggr configuration is applied FREEIDXSRC, D11BDEST,

	// FREEIDXSRC and D11BDEST use same SHIFT and MASK values
	ring_intraggr = (0U
		| BCM_SBF(HWA_RXFILL_RING_INTRAGGR_COUNT,
		        HWA_RX_D11BDEST_INTRAGGR_SEQNUM_CFG_AGGR_COUNT)
		| BCM_SBF(HWA_RXFILL_RING_INTRAGGR_TMOUT,
		        HWA_RX_D11BDEST_INTRAGGR_SEQNUM_CFG_AGGR_TIMER)
		| 0U);

	// Configure HWA1b "FREEIDXSRC" and "D11BDEST" interfaces for inited cores
	for (core = 0; core < HWA_RX_CORES; core++)
	{
		uint32 notpcie, coherent;

		// In NIC mode, only MAC FIFO0 will be configured: FIXME broken RxFILL
		if (rxfill->inited[core][0] == FALSE) {
			HWA_ASSERT(dev->driver_mode == HWA_NIC_MODE);
			continue;
		}

		// Decide the fifo notpcie and coherent values
		// XXX, actually HWA DMA doesn't care about notpcie or coherent
		notpcie = dev->macif_placement;
		if (notpcie == HWA_MACIF_IN_DNGLMEM && rxfill->hme_macifs[core][0]) {
			// clear notpcie if hme_macifs is true in dongle mode.
			notpcie = HWA_MACIF_IN_HOSTMEM;
		}
		coherent = dev->macif_coherency;
		if (coherent == HWA_HW_COHERENCY && rxfill->hme_macifs[core][0]) {
			// clear coherent if hme_macifs is true in dongle mode.
			coherent = HWA_SW_COHERENCY;
		}

		// Same register layout for both "D0DEST" and "D1DEST"
		// Using D0DEST SHIFT and MASK macros for depth and coherency settings
		fifo_cfg = (
			BCM_SBF(coherent, HWA_RX_D0DEST_RING_CFG_TEMPLATE_COHERENT) |
			BCM_SBF(rxfill->fifo_depth[core][0], HWA_RX_D0DEST_RING_CFG_DEPTH));

		// Configure HWA MAC FIFO0
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d0dest_ring_addr_lo,
			rxfill->fifo_addr[core][0].loaddr);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d0dest_ring_addr_hi,
			rxfill->fifo_addr[core][0].hiaddr);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d0dest_ring_cfg, fifo_cfg);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d0dest_intraggr_seqnum_cfg,
			(fifo_intraggr HWA_PKTPGR_EXPR(| pp_desc_burst)));
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d0dest_wrindexupd_addrlo,
			rxfill->dmarcv_ptr[core][0]);

#ifdef BCMPCIEDEV
		// In FD, assert FIFO0 and FIFO1 are both present and depths are equal
		HWA_ASSERT(dev->driver_mode == HWA_FD_MODE);
		HWA_ASSERT(rxfill->inited[core][1] == TRUE);
		HWA_ASSERT(rxfill->fifo_depth[core][1] == rxfill->fifo_depth[core][0]);

		// Configure HWA MAC FIFO1 for FullDongle mode HWA1b RxFill block
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d1dest_ring_addr_lo,
			rxfill->fifo_addr[core][1].loaddr);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d1dest_ring_addr_hi,
			rxfill->fifo_addr[core][1].hiaddr);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d1dest_ring_cfg, fifo_cfg);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d1dest_intraggr_seqnum_cfg,
			(fifo_intraggr HWA_PKTPGR_EXPR(| burst_len1)));
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d1dest_wrindexupd_addrlo,
			rxfill->dmarcv_ptr[core][1]);
#endif /* BCMPCIEDEV */

#if !defined(HWA_PKTPGR_BUILD)
		// Initialize S2H "FREEIDXSRC" interface
		memory = rxfill->rxfree_ring[core].memory;
		u32 = HWA_PTR2UINT(memory);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core],
		                freeidxsrc_ring_addr_lo, u32);
		u32 = HWA_PTR2HIADDR(memory); // hiaddr for NIC mode 64b hosts
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core],
		                freeidxsrc_ring_addr_hi, u32);
		u32 = (ring_cfg |
			BCM_SBF(HWA_RXFILL_RXFREE_DEPTH, HWA_RX_FREEIDXSRC_RING_CFG_DEPTH));
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], freeidxsrc_ring_cfg, u32);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core],
		                freeidxsrc_intraggr_seqnum_cfg, ring_intraggr);
#endif /* !HWA_PKTPGR_BUILD */

		// Initialize H2S "D11BDEST" interface
		memory = rxfill->rxfifo_ring[core].memory;
		u32 = HWA_PTR2UINT(memory);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d11bdest_ring_addr_lo, u32);
		u32 = HWA_PTR2HIADDR(memory); // hiaddr for NIC mode 64b hosts
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d11bdest_ring_addr_hi, u32);
		u32 = (0U
			| ring_cfg
			| BCM_SBF(rxfill->fifo_depth[core][0], HWA_RX_D11BDEST_RING_CFG_DEPTH)
		HWA_PKTPGR_EXPR(
			| BCM_SBIT(HWA_RX_D11BDEST_RING_CFG_SEQ_NO_CHK)
			| BCM_SBIT(HWA_RX_D11BDEST_RING_CFG_SEQ_NO_INSERT))
			| BCM_SBIT(HWA_RX_D11BDEST_RING_CFG_INDEX_AFTER_MAC)
			| 0U);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], d11bdest_ring_cfg, u32);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core],
		                d11bdest_intraggr_seqnum_cfg, ring_intraggr);

		// HWA1b RxFill bypass is disabled by default ... unless HWA1b is broken
		u32 = BCM_SBF(0, HWA_RX_RXPMGR_CFG_BYPASS);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxpmgr_cfg, u32);

		// Setup the minimum number of descriptors threshold for FIFO refilling
		HWA_ASSERT(rxfill->fifo_depth[core][0] > HWA_RXFILL_FIFO_MIN_THRESHOLD);
		u32 = BCM_SBF(HWA_RXFILL_FIFO_MIN_THRESHOLD,
		              HWA_RX_MAC_COUNTER_CTRL_POSTCNT);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], mac_counter_ctrl, u32);

		// Setup SW alert threshold and duration for FIFO starvation
		u32 = (0U
			| BCM_SBF(HWA_RXFILL_FIFO_ALERT_THRESHOLD,
			        HWA_RX_FW_ALERT_CFG_ALERT_THRESH)
			| BCM_SBF(HWA_RXFILL_FIFO_ALERT_DURATION,
			        HWA_RX_FW_ALERT_CFG_ALERT_TIMER)
			| 0U);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], fw_alert_cfg, u32);

		// fw_rxcompensate ... not required

		// Setup RxFILL Ctrl0
		u32 = (0U
			| BCM_SBIT(HWA_RX_RXFILL_CTRL0_USE_CORE0_FIH)
			// | BCM_SBIT(HWA_RX_RXFILL_CTRL0_RPH_COMPRESS_ENABLE) NA in HWA2.0
			| BCM_SBIT(HWA_RX_RXFILL_CTRL0_WAIT_FOR_D11B_DONE)
			// RX Descr1 buffers based on MAC IF placement in dongle SysMem
			| BCM_SBF(notpcie, HWA_RX_RXFILL_CTRL0_TEMPLATE_NOTPCIE)
			| BCM_SBF(coherent, HWA_RX_RXFILL_CTRL0_TEMPLATE_COHERENT)
			| BCM_SBF(0, HWA_RX_RXFILL_CTRL0_TEMPLATE_ADDREXT)
			// Core1 shares RXP from Core0
			| BCM_SBIT(HWA_RX_RXFILL_CTRL0_USE_CORE0_RXP)
			| BCM_SBF(rxfill->config.rph_size, HWA_RX_RXFILL_CTRL0_RPHSIZE)
			// NA in HWA2.0
			// | BCM_SBF(rxfill->config.len_offset,
			//          HWA_RX_RXFILL_CTRL0_LEN_OFFSET_IN_RPH)
			| BCM_SBF(rxfill->config.addr_offset,
				HWA_RX_RXFILL_CTRL0_ADDR_OFFSET_IN_RPH)
			| 0U);

		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_ctrl0, u32);

		// Setup RxFILL Ctrl1
		{
			uint32 d11b_offset, d11b_length;

#if defined(HWA_PKTPGR_BUILD)
			// d11b_length is used to program the MAC descriptor.
			d11b_offset = rxfill->config.wrxh_offset +
				rxfill->config.d11_offset;
			d11b_length = HWA_PP_RXLFRAG_DATABUF_LEN - d11b_offset;
			// d11b_offset is start from base address of rxlfrag.
			d11b_offset += HWA_PP_RXLFRAG_DATABUF_OFFSET;
#else
			d11b_offset = rxfill->config.wrxh_offset +
				rxfill->config.d11_offset;
			d11b_length = HWA_RXBUFFER_BYTES - d11b_offset;
#endif

			u32 = (0U
				| BCM_SBF(d11b_length, HWA_RX_RXFILL_CTRL1_D1_LEN)
				| BCM_SBF(d11b_offset, HWA_RX_RXFILL_CTRL1_D1_OFFSET)
				| BCM_SBF(HWA_RXFILL_MIN_FETCH_THRESH_RXP,
					HWA_RX_RXFILL_CTRL1_MIN_FETCH_THRESH_RXP)
				| BCM_SBF(HWA_RXFILL_MIN_FETCH_THRESH_FREEIDX,
					HWA_RX_RXFILL_CTRL1_MIN_FETCH_THRESH_FREEIDX)
				| BCM_SBF(dev->macif_coherency,
					HWA_RX_RXFILL_CTRL1_TEMPLATE_COHERENT_NONDMA)
				| 0U);
			HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_ctrl1, u32);
		}

		// Setup RxPost to RPH Compression
		u32 = 0U; // No compression in HWA-2.0
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_compresslo, u32);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_compresshi, u32);

		// Reference sbhnddma.h descriptor control flags 1
		// NotPcie bit#18: This field must be zero for receive descriptors
#if defined(BCMPCIEDEV) /* FD mode SW driver */
		u32 = (0U
			// bits 15:00 - reserved, will cause protocol error if not set to 0
			// | D64_CTRL1_FIXEDBURST
			| D64_CTRL1_COHERENT
			// | D64_CTRL1_NOTPCIE
			// | D64_CTRL1_DS
			//   bits 27:20 - core specific flags
			//   bit  28    - D64_CTRL1_EOT will be set by HWA1b
			//   bit  29    - D64_CTRL1_IOC will be set by HWA1b
			//   bit  30    - D64_CTRL1_EOF will be set by HWA1b
			//   bit  31    - D64_CTRL1_SOF will be set by HWA1b
			| 0U);
#else /* !BCMPCIEDEV */
		u32 = 0U;
		if (dev->macif_placement || dev->host_coherency)
			u32 |= D64_CTRL1_COHERENT;
#endif /* !BCMPCIEDEV */
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_desc0_templ_lo, u32);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_desc1_templ_lo, u32);

		// Reference sbhnddma.h descriptor control flags 2
		u32 = (0U
			// bits 47:32 - buffer byte count will be set by HWA1b
			// bits 49:48 - address extension is 0
			// bit  50    - D64_CTRL2_PARITY parity always calculated internally
			// bits 63:51 - reserved, will cause protocol error if not set to 0
			| 0U);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_desc0_templ_hi, u32);
		HWA_WR_REG_NAME(HWA1b, regs, rx_core[core], rxfill_desc1_templ_hi, u32);

	} // for core

	// Enable Req-Ack based interface between MAC-HWA on rx DMA is enabled.
	u16 = BCM_SBIT(_HWA_MACIF_CTL_RXDMAEN);
	HWA_UPD_REG16_ADDR(HWA1b, &dev->mac_regs->hwa_macif_ctl, u16, _HWA_MACIF_CTL_RXDMAEN);

	return HWA_SUCCESS;

} // hwa_rxfill_init

int // HWA1b RxFill: Deinit RxFill interfaces
hwa_rxfill_deinit(hwa_rxfill_t *rxfill)
{
	hwa_dev_t *dev;

	// Setup locals
	dev = HWA_DEV(rxfill);

	// Disable Req-Ack based interface between MAC-HWA on rx DMA.
	HWA_UPD_REG16_ADDR(HWA1b, &dev->mac_regs->hwa_macif_ctl, 0, _HWA_MACIF_CTL_RXDMAEN);

	return HWA_SUCCESS;
}

#if defined(HWA_PKTPGR_BUILD)

void // Fetch all RxBuffers
hwa_rxfill_d11b_fetch_all(hwa_dev_t *dev, uint32 core)
{
	hwa_rxfill_t *rxfill; // SW rxfill state
	hwa_ring_t *rxfifo_ring; // D11BDEST RxFIFO ring
	hwa_rxfill_rxfifo_t *d11b_elem; // element in D11BDEST RxFIFO ring
	hwa_rxfill_rxfifo_t *audit_elem; // element in D11B audit ring
	int elem_ix;
	uint16 depth;
	uint32 u32, wr_idx, rd22_idx, wr_idx_dir;

	HWA_FTRACE(HWA1b);

	// Audit parameters and pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Setup locals
	rxfill = &dev->rxfill;
	rxfifo_ring = &rxfill->rxfifo_ring[core];

	// Read D11B ring RD, WR index
	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], d11bdest_ring_wrindex);
	wr_idx = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_WR_INDEX);
	HWA_INFO(("d11bdest_ring_wrindex<0x%x,%d>\n", u32, wr_idx));
	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], d11bdest_ring_rdindex);
	rd22_idx = BCM_GBF(u32, HWA_RX_D11BDEST_RING_RDINDEX_RD22_INDEX);
	HWA_INFO(("d11bdest_ring_rdindex<0x%x,%d>\n", u32, rd22_idx));
	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], d11bdest_ring_wrindex_dir);
	wr_idx_dir = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_DIR_WR_INDEX_DIRECT);
	HWA_INFO(("d11bdest_ring_wrindex_dir<0x%x,%d>\n", u32, wr_idx_dir));
	depth = rxfifo_ring->depth;

	// Audit freerph ring must have enough empty solts.
	// Delta between rd22_idx and wr_idx_dir has included dirty and clean,
	// the maximum delta is d11b ring depth - 1;
	HWA_ASSERT(hwa_pktpgr_req_ring_wait_for_avail(dev, hwa_pktpgr_freerph_req_ring,
		NTXPACTIVE(rd22_idx, wr_idx_dir, depth), FALSE));

	// Bypass if no audit or SW WAR.
	if (!D11B_AUDIT_ENABLED(dev->corerev))
		return;

	// d11b_audit ring must be empty
	HWA_ASSERT(bcm_ring_is_empty(&rxfill->d11b_audit_state));

	// dirty count, cannot use  hwa_ring_cons_avail
	rxfill->d11b_audit_dirty = NTXPACTIVE(rd22_idx, wr_idx, depth);

	HWA_INFO(("%s reclaim core<%u> rxbuffers RD<%u> WR<%u> WRDIR<%u> dirty <%u>\n",
		HWA1b, core, rd22_idx, wr_idx, wr_idx_dir, rxfill->d11b_audit_dirty));

	while (rd22_idx != wr_idx_dir) {
		// Qeue D11B info to audit ring.
		elem_ix = bcm_ring_prod(&rxfill->d11b_audit_state, rxfill->d11b_audit_depth);
		audit_elem = BCM_RING_ELEM(rxfill->d11b_audit_table, elem_ix,
			sizeof(hwa_rxfill_rxfifo_t));

		// Fetch location of packet in rxfifo to process [both dirty and clean]
		if (dev->d11b_axi) {
			uint32 *sys_mem;
			hwa_mem_addr_t axi_mem_addr;

			// src
			axi_mem_addr = HWA_TABLE_ADDR(hwa_rxfill_rxfifo_t,
				HWA_PTR2UINT(rxfifo_ring->memory), rd22_idx);
			// dst
			sys_mem = &audit_elem->u32[0];
			// read to dst
			HWA_RD_MEM32(HWA3b, hwa_rxfill_rxfifo_t, axi_mem_addr, sys_mem);
		} else {
			// src
			d11b_elem = HWA_RING_ELEM(hwa_rxfill_rxfifo_t, rxfifo_ring, rd22_idx);
			// read to dst
			*audit_elem = *d11b_elem;
		}

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: HDBM
		// B0: Audit is not necessary because HWA de-allocate HDBM during recycle flow.
		if (HWAREV_LE(dev->corerev, 132)) {
			hwa_dbm_idx_audit(dev, audit_elem->pkt_mapid, DBM_AUDIT_RX,
				HWA_HDBM, DBM_AUDIT_ALLOC);
		}
#endif

		// Next
		rd22_idx = (rd22_idx + 1) % depth;
	}
}

void // Reclaim all RxBuffers
hwa_rxfill_rxbuffer_reclaim(hwa_dev_t *dev, uint32 core)
{
	uint32 u32;
	hwa_rxfill_t *rxfill;
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t *freerph_ring;
	hwa_rxfill_rxfifo_t *audit_elem;
	hwa_pp_freed11_req_t *freed11_elem;

	int i, wr_idx;
	bool rxcpl_pend;
	uint16 depth, recycle_wr_index;
	int d11b_audit_total, recycle_count;
	int elem_ix;
	uint32 audit_elem_fail_cnt;

	HWA_FTRACE(HWA1b);
	BCM_REFERENCE(recycle_count);
	BCM_REFERENCE(d11b_audit_total);

	// Audit parameters and pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Bypass if no audit or SW WAR.
	if (!D11B_AUDIT_ENABLED(dev->corerev))
		return;

	// Setup locals
	audit_elem_fail_cnt = 0;
	rxcpl_pend = FALSE;
	rxfill = &dev->rxfill;
	pktpgr = &dev->pktpgr;
	freerph_ring = &pktpgr->freerph_req_ring;

	// Number of recycled put in freerph ring
	u32 = HWA_RD_REG_NAME(HWA1x, dev->regs, rx_core[core], recycle_status);
	recycle_wr_index = BCM_GBF(u32, HWA_RX_RECYCLE_STATUS_RECYCLE_WR_INDEX);
	wr_idx = HWA_RING_STATE(freerph_ring)->write;
	depth = freerph_ring->depth;
	recycle_count = NTXPACTIVE(wr_idx, recycle_wr_index, depth);
	ASSERT(recycle_count < depth);

	// A0: Free dirty, Audit clean and free it too.
	// B0: Audit dirty and clean. No free, keep it in freerph ring for reusing.
	d11b_audit_total = bcm_ring_cons_avail(&rxfill->d11b_audit_state, rxfill->d11b_audit_depth);
	if (dev->d11b_recycle_pagein) {
		HWA_INFO(("%s d11b_audit_total<%d> = recycle_count<%d>.\n",
			HWA1b, d11b_audit_total, recycle_count));
		HWA_ASSERT(d11b_audit_total == recycle_count);
	} else {
		HWA_INFO(("%s d11b_audit_total<%d> = recycle_count<%d> + d11b_audit_dirty<%d>.\n",
			HWA1b, d11b_audit_total, recycle_count, rxfill->d11b_audit_dirty));
		HWA_ASSERT(d11b_audit_total == (recycle_count + rxfill->d11b_audit_dirty));
	}

	// Free dirty.
	if (!dev->d11b_recycle_pagein) {
		// Note, free dirty may cause AMPDU seq hole but AMPDU timeout WAR it.
		// Legacy path has same behavior.
		rxcpl_pend = FALSE;
		for (i = 0; i < rxfill->d11b_audit_dirty; i++) {
			elem_ix = bcm_ring_cons(&rxfill->d11b_audit_state,
				rxfill->d11b_audit_depth);
			HWA_ASSERT(elem_ix != BCM_RING_EMPTY);
			audit_elem = BCM_RING_ELEM(rxfill->d11b_audit_table, elem_ix,
				sizeof(hwa_rxfill_rxfifo_t));

			// We cannot free dirty RPH to freerph ring becasue it is
			// locked for D11B reclaim processing.
			// We only can free host pktid to Host.
			hwa_rxpath_queue_rxcomplete_fast(dev, audit_elem->host_pktid);
			rxcpl_pend = TRUE;

			// Free HDBM, the buffer index of HDBM is equal to pkt_mapid
			hwa_pktpgr_dbm_free(dev, HWA_HDBM, audit_elem->pkt_mapid, FALSE);
		}

		// Commit
		if (rxcpl_pend)
			hwa_rxpath_xmit_rxcomplete_fast(dev);
	}

	// A0: Audit clean and free it.
	// B0: Audit dirty and clean. No free, keep it in freerph ring for reusing
	rxcpl_pend = FALSE;
	while ((elem_ix = bcm_ring_cons(&rxfill->d11b_audit_state,
		rxfill->d11b_audit_depth)) != BCM_RING_EMPTY) {
		// Audit each elem.
		audit_elem = BCM_RING_ELEM(rxfill->d11b_audit_table, elem_ix,
			sizeof(hwa_rxfill_rxfifo_t));
		freed11_elem = BCM_RING_ELEM(freerph_ring->memory, wr_idx,
			sizeof(hwa_pp_freed11_req_t));
		if (HWAREV_LE(dev->corerev, 132)) {
			HWA_ASSERT(freed11_elem->trans == HWA_PP_PAGEMGR_FREE_D11B);
		} else {
			// XXX, CRBCAHWA-633, RECYCLE_STATUS::recycle_freed11b_mode(bit 27).
			// If bit 27 is 0, the FREE_RPH is generated in recycle flow and the
			// corresponding host page is de-allocated to Pager.
			// If bit 27 is 1, the FREE_D11B is generated in recycle flow.
			HWA_ASSERT(freed11_elem->trans == HWA_PP_PAGEMGR_FREE_RPH);
		}

		if ((audit_elem->host_pktid != freed11_elem->host_pktid) ||
			(HWAREV_LE(dev->corerev, 132) &&
			(audit_elem->pkt_mapid != freed11_elem->pkt_mapid)) ||
			(audit_elem->data_buf_haddr64.hi != freed11_elem->data_buf_haddr64.hi) ||
			(audit_elem->data_buf_haddr64.lo != freed11_elem->data_buf_haddr64.lo)) {
			audit_elem_fail_cnt++;
		}

		// Free it if d11b_recycle_war is enabled
		if (dev->d11b_recycle_war) {
			// Free to Host.
			hwa_rxpath_queue_rxcomplete_fast(dev, audit_elem->host_pktid);
			rxcpl_pend = TRUE;

			// Free to HDBM, for HDBM pkt_mapid is equal to buffer index
			if (HWAREV_LE(dev->corerev, 132)) {
				hwa_pktpgr_dbm_free(dev, HWA_HDBM, audit_elem->pkt_mapid,
					FALSE);
			}
			// B0: Free HDBM is not necessary because HWA de-allocate HDBM
			// during recycle flow.
		}
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		else {
			// Free: HDBM
			if (HWAREV_LE(dev->corerev, 132)) {
				hwa_dbm_idx_audit(dev, audit_elem->pkt_mapid, DBM_AUDIT_RX,
					HWA_HDBM, DBM_AUDIT_FREE);
			}
			// B0: Audit is not necessary because HWA de-allocate HDBM
			// during recycle flow.
		}
#endif

		// Next one in freerph
		hwa_ring_next_index(&wr_idx, depth);
	}

	// Commit
	if (rxcpl_pend)
		hwa_rxpath_xmit_rxcomplete_fast(dev);

	if (audit_elem_fail_cnt != 0) {
		HWA_ERROR(("D11B reclaim, audit total fail<%d>\n", audit_elem_fail_cnt));
	}
}

#else

int // Handle a Free RxBuffer request from WLAN driver, returns success|failure
hwa_rxfill_rxbuffer_free(struct hwa_dev *dev, uint32 core,
	hwa_rxbuffer_t *rx_buffer, bool has_rph)
{
	hwa_rxfill_t *rxfill;
	hwa_ring_t *rxfree_ring; // S2H FREEIDXSRC interface
	hwa_rxfill_rxfree_t *rxfree;
	HWA_DEBUG_EXPR(uint32 offset);

	rxfill = &dev->rxfill;

	HWA_TRACE(("%s free core<%u> rxbuffer<%p> has_rph<%u>\n",
		HWA1b, core, rx_buffer, has_rph));

	// Audit parameters and pre-conditions
	HWA_ASSERT(rx_buffer != (hwa_rxbuffer_t*)NULL);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Audit rx_buffer, wrong rx_buffer causes 1b stuck.
	HWA_ASSERT(rx_buffer >= (hwa_rxbuffer_t*)dev->rx_bm.memory);
	HWA_ASSERT(rx_buffer <= ((hwa_rxbuffer_t*)dev->rx_bm.memory) + (dev->rx_bm.pkt_total - 1));
	HWA_DEBUG_EXPR({
		offset = ((char *)rx_buffer) - ((char *)dev->rx_bm.memory);
		HWA_ASSERT((offset % dev->rx_bm.pkt_size) == 0);
	});

	// Setup locals
	rxfree_ring = &rxfill->rxfree_ring[core];

	if (hwa_ring_is_full(rxfree_ring)) {
		// FIXME
		HWA_ASSERT(1);
		goto failure;
	}

	// Find the location where rxfree needs to be constructed, and populate it
	rxfree = HWA_RING_PROD_ELEM(hwa_rxfill_rxfree_t, rxfree_ring);

	// Convert rxbuffer pointer to its index within Rx Buffer Manager
	rxfree->index = HWA_TABLE_INDX(hwa_rxbuffer_t,
	                               dev->rx_bm.memory, rx_buffer);

	/* XXX, I encounter below errors
	 * cmn::errorstatusreg report 3
	 * rx::debug_errorstatus 4
	 * RxBM audit: Get duplicate rxbuffer<154> from RxBM  which
	 * the rxbuffer idx is just freed to freeindexQ in paired type.
	 * Add a WAR at pciedev_lbuf_callback
	 * XXX, CRBCAHWA-558
	 */
	if (HWAREV_LE(dev->corerev, 130)) {
		HWA_ASSERT(has_rph == FALSE);
	}

	rxfree->control_info = (has_rph == TRUE) ?
	                        HWA_RXFILL_RXFREE_PAIRED : HWA_RXFILL_RXFREE_SIMPLE;

#ifdef HWA_RXFILL_RXFREE_AUDIT_ENABLED
	_hwa_rxfill_rxfree_audit(rxfill, rxfree->index, FALSE);
#endif

	hwa_ring_prod_upd(rxfree_ring, 1, TRUE); // update/commit WR

	HWA_STATS_EXPR(rxfill->rxfree_cnt[core]++);

	HWA_TRACE(("%s RXF[%u,%u][index<%u> ctrl<%u>]\n", HWA1b,
		HWA_RING_STATE(rxfree_ring)->write, HWA_RING_STATE(rxfree_ring)->read,
		rxfree->index, rxfree->control_info));

	return HWA_SUCCESS;

failure:
	HWA_WARN(("%s rxbuffer free <0x%p> failure\n", HWA1b, rx_buffer));

	return HWA_FAILURE;

} // hwa_rxfill_rxbuffer_free

void // Reclaim all RxBuffers in RxBM
hwa_rxfill_rxbuffer_reclaim(hwa_dev_t *dev, uint32 core)
{
	uintptr rxbm_base; // loaddr of Rx Buffer Manager
	hwa_rxfill_t *rxfill; // SW rxfill state
	hwa_ring_t *h2s_ring; // H2S D11BDEST RxFIFO ring
	hwa_ring_t *s2h_ring; // S2H FREEIDXSRC RxFREE ring
	hwa_rxbuffer_t *rxbuffer; // pointer to RxBuffer
	hwa_rxfill_rxfifo_t *rxfifo; // element in H2S D11BDEST RxFIFO ring
	hwa_rxpost_hostinfo_t *rph_req;
	uint32 rxfifo_cnt; // total rxbuffers processed
	int wr_idx, rd_idx;
	uint16 depth;

	HWA_FTRACE(HWA1b);

	// Audit parameters and pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Setup locals
	rxfill = &dev->rxfill;
	rxbm_base = rxfill->rxbm_base;
	h2s_ring = &rxfill->rxfifo_ring[core];
	s2h_ring = &rxfill->rxfree_ring[core];

	hwa_ring_cons_get(h2s_ring); // fetch HWA rxfifo ring's WR index once

	rxfifo_cnt = hwa_rxfill_fifo_avail(rxfill, core);
	depth = h2s_ring->depth;
	wr_idx = (HWA_RING_STATE(h2s_ring)->write + rxfifo_cnt) % depth;
	rd_idx = HWA_RING_STATE(h2s_ring)->read;

	HWA_ERROR(("%s reclaim core<%u> %u rxbuffers RD<%u> WR<%u>\n",
		HWA1b, core, rxfifo_cnt, rd_idx, wr_idx));

	while (rd_idx != wr_idx) {
		// Fetch location of packet in rxfifo to process
		rxfifo = HWA_RING_ELEM(hwa_rxfill_rxfifo_t, h2s_ring, rd_idx);

		// Get RxBuffer from RxBM
		rxbuffer = HWA_TABLE_ELEM(hwa_rxbuffer_t, rxbm_base, rxfifo->index);

		rph_req = (hwa_rxpost_hostinfo_t *)rxbuffer;

#ifdef BCMDBG
		HWA_DEBUG_EXPR({
		// Show RPH value,
		if (dev->host_addressing & HWA_32BIT_ADDRESSING) { // 32bit host
			HWA_PRINT("%s rph32 alloc pktid<%u> haddr<0x%08x:0x%08x>\n", HWA1x,
				rph_req->hostinfo32.host_pktid, dev->host_physaddrhi,
				rph_req->hostinfo32.data_buf_haddr32);
		} else { // 64bit host
			HWA_PRINT("%s rph64 alloc pktid<%u> haddr<0x%08x:0x%08x>\n", HWA1x,
				rph_req->hostinfo64.host_pktid,
				rph_req->hostinfo64.data_buf_haddr64.hiaddr,
				rph_req->hostinfo64.data_buf_haddr64.loaddr);
		}});
#endif /* BCMDBG */

		hwa_rxpath_queue_rxcomplete_fast(dev, rph_req->hostinfo64.host_pktid);

		rd_idx = (rd_idx + 1) % depth;
	}

	ASSERT(rd_idx == wr_idx);

	hwa_rxpath_xmit_rxcomplete_fast(dev);

	// Reset RxFIFO ring and RxFREE ring
	HWA_RING_STATE(h2s_ring)->read = 0;
	HWA_RING_STATE(h2s_ring)->write = 0;
	HWA_RING_STATE(s2h_ring)->read = 0;
	HWA_RING_STATE(s2h_ring)->write = 0;

}

int // Process H2S RxFIFO interface WR index update interrupt from MAC
hwa_rxfill_rxbuffer_process(hwa_dev_t *dev, uint32 core, bool bound)
{
	uint32 elem_ix; // location of next element to read
	uintptr rxbm_base; // loaddr of Rx Buffer Manager
	hwa_rxfill_t *rxfill; // SW rxfill state
	hwa_ring_t *h2s_ring; // H2S D11BDEST RxFIFO ring
	hwa_rxbuffer_t *rxbuffer; // pointer to RxBuffer
	hwa_rxfill_rxfifo_t *rxfifo; // element in H2S D11BDEST RxFIFO ring
	hwa_handler_t *rxfifo_recv_handler; // upstream rx buffer processing
	hwa_handler_t *rxfifo_done_handler; // upstream rx fifo done processing
	uint32 rxfifo_cnt; // total rxbuffers processed
	int ret, elem_ix_pend; // location of next pend element to read
	wlc_info_t *wlc;	// wlc pointer
#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_RXSTS)
	rx_list_t rx_sts_list = {NULL};
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_RXSTS */

	HWA_FTRACE(HWA1b);

	// Audit parameters and pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_RX_CORES);

	// Setup locals
	rxfill = &dev->rxfill;
	rxbm_base = rxfill->rxbm_base;
	wlc = (wlc_info_t *)rxfill->wlc[core];

	// Read and save TSF register once
	rxfill->tsf_l = R_REG(wlc->osh, D11_TSFTimerLow(wlc));

	// Fetch registered upstream callback handlers
	rxfifo_recv_handler = &dev->handlers[HWA_RXFIFO_PROC_CB];
	rxfifo_done_handler = &dev->handlers[HWA_RXFIFO_DONE_CB];

	h2s_ring = &rxfill->rxfifo_ring[core];

	hwa_ring_cons_get(h2s_ring); // fetch HWA rxfifo ring's WR index once
	rxfifo_cnt = 0U;

	// Consume all packets to be received in FIFO, sending them upstream
	// Use hwa_ring_cons_pend, because rxfifo_recv_handler could return error.
	while ((elem_ix = hwa_ring_cons_pend(h2s_ring, &elem_ix_pend)) != BCM_RING_EMPTY) {

		// Fetch location of next packet in rxfifo to process
		rxfifo = HWA_RING_ELEM(hwa_rxfill_rxfifo_t, h2s_ring, elem_ix);

		// Send RxBuffer to upstream handler
		rxbuffer = HWA_TABLE_ELEM(hwa_rxbuffer_t, rxbm_base, rxfifo->index);

		HWA_TRACE(("%s elem_ix<%u> recv core<%u> rxbuffer<%p>\n",
			HWA1b, elem_ix, core, rxbuffer));

		ret = (*rxfifo_recv_handler->callback)(rxfifo_recv_handler->context,
			(uintptr)rxfill->wlc[core], (uintptr)rxbuffer, core, rxfifo->index);

		// Callback cannot handle it, don't update ring read and break the loop.
		if (ret != HWA_SUCCESS) {
#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_RXSTS)
			if (STS_RX_ENAB(wlc->pub) || STS_RX_OFFLOAD_ENAB(wlc->pub)) {
#if defined(STS_FIFO_RXEN)
				if (STS_RX_ENAB(wlc->pub)) {
					dma_sts_rx(wlc->hw->di[STS_FIFO], &rx_sts_list);
					if (rx_sts_list.rx_head != NULL) {
						wlc_bmac_dma_rxfill(wlc->hw, STS_FIFO);
					}
				}
#endif /* STS_FIFO_RXEN */
				wlc_bmac_recv_append_sts_list(wlc, &wlc->hw->rx_sts_list,
					&rx_sts_list);
			}
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_RXSTS */
			break;
		}

		// Commit a previously pending read
		hwa_ring_cons_done(h2s_ring, elem_ix_pend);

		rxfifo_cnt++;

		if ((rxfifo_cnt % HWA_RXFILL_LAZY_RD_UPDATE) == 0) {
			if (bound) {
				break;
			} else {
				hwa_ring_cons_put(h2s_ring); // commit RD index lazily
			}
		}
	}

	hwa_ring_cons_put(h2s_ring); // commit RD index now

	// Done processing all rx packets in fifo
	(*rxfifo_done_handler->callback)(rxfifo_done_handler->context,
		(uintptr)rxfill->wlc[core], (uintptr)0, core, rxfifo_cnt);

	HWA_STATS_EXPR(rxfill->rxfifo_cnt[core] += rxfifo_cnt);

	if (!hwa_ring_is_empty(h2s_ring)) {
		/* need re-schdeule */
		HWA_ASSERT(core == 0);
		dev->intstatus |= HWA_COMMON_INTSTATUS_D11BDEST0_INT_MASK;
	}

	return HWA_SUCCESS;

} // hwa_rxfill_rxbuffer_process

uint32 // Return the number of available RxBuffers for reception in the RX FIFOs
hwa_rxfill_fifo_avail(hwa_rxfill_t *rxfill, uint32 core)
{
	uint32 u32;
	hwa_dev_t *dev;

	HWA_FTRACE(HWA1b);

	// Audit parameters and pre-conditions
	dev = HWA_DEV(rxfill);
	HWA_ASSERT(core < HWA_RX_CORES);

	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], mac_counter_status);

	HWA_ERROR(("%s mac_counter_status sat<%u> need_post<%u> aval<%u>\n",
		HWA1b, BCM_GBF(u32, HWA_RX_MAC_COUNTER_STATUS_SATURATED),
		BCM_GBF(u32, HWA_RX_MAC_COUNTER_STATUS_NEED_POST),
		BCM_GBF(u32, HWA_RX_MAC_COUNTER_STATUS_CTR_VAL)));

	u32 = BCM_GBF(u32, HWA_RX_MAC_COUNTER_STATUS_CTR_VAL);

	return u32;

} // hwa_rxfill_fifo_avail

#endif /* !HWA_PKTPGR_BUILD */

#if defined(BCMDBG) || defined(HWA_DUMP)

void // Debug support for HWA1b RxFill block
hwa_rxfill_dump(hwa_rxfill_t *rxfill, struct bcmstrbuf *b, bool verbose)
{
	uint32 core, fifo;

	HWA_BPRINT(b, "%s dump<%p>\n", HWA1b, rxfill);

	if (rxfill == (hwa_rxfill_t*)NULL)
		return;

	HWA_BPRINT(b, "+ Config: rph_sz<%u> offset align<%u> d11<%u> len<%u> addr<%u>\n",
		rxfill->config.rph_size,
		(rxfill->config.wrxh_offset - rxfill->config.rph_size),
		rxfill->config.d11_offset, rxfill->config.len_offset,
		rxfill->config.addr_offset);

	for (core = 0; core < HWA_RX_CORES; core++) { // per inited core

		if (rxfill->inited[core][0] == FALSE)
			continue;

		HWA_BPRINT(b, "+ core%u wlc<0x%p>\n", core, rxfill->wlc[core]);

		HWA_STATS_EXPR(
			HWA_BPRINT(b, "+ rxfree_cnt<%u> rxfifo_cnt<%u>\n",
				rxfill->rxfree_cnt[core], rxfill->rxfifo_cnt[core]));
		HWA_BPRINT(b, "+ rxgiants<%u>\n", rxfill->rxgiants[core]);

		NO_HWA_PKTPGR_EXPR(hwa_ring_dump(&rxfill->rxfree_ring[core],
			b, "+ rxfree_ring"));
		hwa_ring_dump(&rxfill->rxfifo_ring[core], b, "+ rxfifo_ring");

		for (fifo = 0; fifo < HWA_RXFIFO_MAX; fifo++) { // per inited fifo
			if (rxfill->inited[core][fifo] == TRUE) {
				HWA_BPRINT(b, "+ core%u fifo%u<0x%08x,0x%08x>"
					" depth<%u> dmarcv_ptr<0x%08x>\n",
					core, fifo,
					rxfill->fifo_addr[core][fifo].loaddr,
					rxfill->fifo_addr[core][fifo].hiaddr,
					rxfill->fifo_depth[core][fifo],
					rxfill->dmarcv_ptr[core][fifo]);
			}
		} // for fifo

	} // for core

} // hwa_rxfill_dump

#if defined(WLTEST) || defined(HWA_DUMP)

// Debug dump of various Transfer Status, using RXPMGR TRFSTATUS layout
#define HWA_RXFILL_TFRSTATUS_DECLARE(mgr) \
void hwa_rxfill_##mgr##_tfrstatus(hwa_rxfill_t *rxfill, uint32 core) { \
	hwa_dev_t *dev = HWA_DEV(rxfill); \
	uint32 u32; \
	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], mgr##_tfrstatus); \
	HWA_PRINT("%s core<%u> %s_tfrstatus<%u> sz<%u> occup<%u> avail<%u>\n", \
		HWA1b, core, #mgr, \
		BCM_GBF(u32, HWA_RX_RXPMGR_TFRSTATUS_STATE), \
		BCM_GBF(u32, HWA_RX_RXPMGR_TFRSTATUS_TRANSFER_SIZE), \
		BCM_GBF(u32, HWA_RX_RXPMGR_TFRSTATUS_SRC_OCCUPIED_EL), \
		BCM_GBF(u32, HWA_RX_RXPMGR_TFRSTATUS_DEST_SPACE_AVAIL_EL)); \
}
HWA_RXFILL_TFRSTATUS_DECLARE(rxpmgr);     // hwa_rxfill_rxpmgr_tfrstatus
HWA_RXFILL_TFRSTATUS_DECLARE(d0mgr);      // hwa_rxfill_d0mgr_tfrstatus
HWA_RXFILL_TFRSTATUS_DECLARE(d1mgr);      // hwa_rxfill_d1mgr_tfrstatus
HWA_RXFILL_TFRSTATUS_DECLARE(d11bmgr);    // hwa_rxfill_d11bmgr_tfrstatus
HWA_RXFILL_TFRSTATUS_DECLARE(freeidxmgr); // hwa_rxfill_freeidxmgr_tfrstatus

// Debug dump of various localfifo configuration, using RXP LOCALFIFO CFG STATUS
#define HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(mgr) \
void hwa_rxfill_##mgr##_localfifo_status(hwa_rxfill_t *rxfill, uint32 core) { \
	hwa_dev_t *dev = HWA_DEV(rxfill); \
	uint32 u32; \
	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], \
	          mgr##_localfifo_cfg_status); \
	HWA_PRINT("%s core<%u> %s_localfifo_cfg_status " \
		"max_items<%u> wrptr<%u> rdptr<%u> notempty<%u> isfull<%u>\n", \
		HWA1b, core, #mgr, \
		BCM_GBF(u32, HWA_RX_RXP_LOCALFIFO_CFG_STATUS_RXPDESTFIFO_MAX_ITEMS), \
		BCM_GBF(u32, HWA_RX_RXP_LOCALFIFO_CFG_STATUS_RXPDESTFIFO_WRPTR), \
		BCM_GBF(u32, HWA_RX_RXP_LOCALFIFO_CFG_STATUS_RXPDESTFIFO_RDPTR), \
		BCM_GBF(u32, HWA_RX_RXP_LOCALFIFO_CFG_STATUS_RXPDESTFIFO_NOTEMPTY), \
		BCM_GBF(u32, HWA_RX_RXP_LOCALFIFO_CFG_STATUS_RXPDESTFIFO_FULL)); \
}
HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(rxp);  // hwa_rxfill_rxp_localfifo_status
HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(d0);   // hwa_rxfill_d0_localfifo_status
HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(d1);   // hwa_rxfill_d1_localfifo_status
HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(d11b); // hwa_rxfill_d11b_localfifo_status
HWA_RXFILL_LOCALFIFO_STATUS_DECLARE(freeidx);

void // Dump the HWA1b RxFill status
hwa_rxfill_status(hwa_rxfill_t *rxfill, uint32 core)
{
	uint32 u32;
	hwa_dev_t *dev;

	dev = HWA_DEV(rxfill);

	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[core], rxfill_status0);

	HWA_PRINT("%s status<%u> fih_buf_valid<%u> "
		"rxpf[empty<%u> mac<%u>] full[d0<%u> d1<%u> d11b<%u>] d11b_avail<%u> "
		"buf_index<%u>\n",
		HWA1b, BCM_GBF(u32, HWA_RX_RXFILL_STATUS0_STATE),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_FIH_BUF_VALID),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_RXPF_EMPTY),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_RXPF_EMPTY_FOR_MAC),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_D0_FULL),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_D1_FULL),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_D11B_FULL),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_POPPED_BUF_AVAIL),
		BCM_GBIT(u32, HWA_RX_RXFILL_STATUS0_BUF_INDEX));

} // hwa_rxfill_status

#endif

#endif /* BCMDBG */

#if defined(HWA_PKTPGR_BUILD)

#if defined(HWA_DUMP)
static void
_hwa_rxfill_pagein_rx_dump_pkt(hwa_pp_lbuf_t *rxlbuf, const char *title,
	bool one_shot)
{
	uint32 pkt_index = 0;
	hwa_pp_lbuf_t *curr = rxlbuf;

	/* Ignore dump */
	if (!(hwa_pktdump_level & HWA_PKTDUMP_RX)) {
		return;
	}

	HWA_ASSERT(curr);

	while (curr) {
		pkt_index++;
		HWA_PRINT("  [%s] 1b:rxlbuf-%d at <%p>\n", title, pkt_index, curr);
		HWA_PRINT("    control:\n");
		HWA_PRINT("      pkt_mapid<%u>\n", PKTMAPID(curr));
		HWA_PRINT("      flowid<%u>\n", PKTGETCFPFLOWID(curr));
		HWA_PRINT("      next<%p>\n", PKTNEXT(OSH_NULL, curr));
		HWA_PRINT("      link<%p>\n", PKTLINK(curr));
		HWA_PRINT("      flags<0x%x>\n", PKTFLAGS(curr));
		HWA_PRINT("      head<%p>\n", PKTHEAD(OSH_NULL, curr));
		HWA_PRINT("      end<%p>\n", PKTEND(OSH_NULL, curr));
		HWA_PRINT("      data<%p>\n", PKTDATA(OSH_NULL, curr));
		HWA_PRINT("      len<%u>\n", PKTLEN(OSH_NULL, curr));
		HWA_PRINT("      ifid<%u>\n", PKTIFINDEX(OSH_NULL, curr));
		HWA_PRINT("      prio<%u>\n", HWAPKTPRIO(curr));
		HWA_PRINT("    fraginfo:\n");
		HWA_PRINT("      frag_num<%u>\n", PKTFRAGTOTNUM(OSH_NULL, curr));
		HWA_PRINT("      flags<%u>\n", PKTFLAGSEXT(OSH_NULL, curr));

		HWA_PRINT("      host_pktid<0x%x>\n", PKTHOSTPKTID(curr, 0));
		HWA_PRINT("      host_datalen<%u>\n", PKTFRAGLEN(OSH_NULL, curr, 0));
		HWA_PRINT("      data_buf_haddr64<0x%08x,0x%08x>\n",
			PKTFRAGDATA_LO(OSH_NULL, curr, 0),
			PKTFRAGDATA_HI(OSH_NULL, curr, 0));
		HWA_PRINT("    misc:\n");
		HWA_PRINT("      rxcpl_id<%u>\n", PKTRXCPLID(OSH_NULL, curr));
		/* Raw dump if need. */
		if (hwa_pktdump_level & HWA_PKTDUMP_RX_RAW) {
			prhex(title, (uint8 *)curr, sizeof(hwa_pp_lbuf_t));
		}

		if (one_shot)
			break;
		curr = (hwa_pp_lbuf_t *)PKTLINK(curr);
	}
}
#endif /* HWA_DUMP */

/* Use D11B intr and hwa_pktpgr_pagein_rxprocess
 * to trigger this function again.
 * Now, we don't request more than two hwa_pktpgr_pagein_rxprocess
 * in hwa_pktpgr_pagein_req_ring.
 * XXX, FIXME: Need to optimize this function.
 */
int
hwa_rxfill_pagein_rx_req(hwa_dev_t *dev, uint32 core, uint16 rxpkt_ready, bool bound)
{
	bool in_emergency;
	int pktpgr_trans_id;
	int16 ddbm_avail_sw;
	uint16 pkt_count;
	uint32 u32;
	hwa_pktpgr_t *pktpgr;
	hwa_pp_pagein_req_rxprocess_t req;
	HWA_PP_DBG_EXPR(uint32 write);
	HWA_PP_DBG_EXPR(uint32 read);
	HWA_PP_DBG_EXPR(uint32 read22);

	HWA_FTRACE(HWA1b);

	// Audit parameters and pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_RX_CORES);

	//Setup locals
	pktpgr = &dev->pktpgr;
	pktpgr_trans_id = HWA_FAILURE;
	in_emergency = FALSE;
	ddbm_avail_sw = pktpgr->ddbm_avail_sw;

	// Read occupied_after_mac
	if (rxpkt_ready == HWA_D11BDEST_RXPKT_READY_INVALID) {
		HWA_PP_DBG_EXPR({
			u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_wrindex);
			write = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_WR_INDEX);
			u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_rdindex);
			read = BCM_GBF(u32, HWA_RX_D11BDEST_RING_RDINDEX_RD_INDEX);
			read22 = BCM_GBF(u32, HWA_RX_D11BDEST_RING_RDINDEX_RD22_INDEX);
		});
		u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_wrindex_dir);
		rxpkt_ready = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_DIR_OCCUPIED_AFTER_MAC);
		HWA_PP_DBG(HWA_PP_DBG_2A, "  RX: write<%d> read<%d/%d> rxpkt_ready<%d>\n",
			write, read, read22, rxpkt_ready);
	}

	if (rxpkt_ready == 0 || ddbm_avail_sw <= 0) {
		HWA_PP_DBG(HWA_PP_DBG_2A, "  rxpkt_ready<%d> ddbm_avail_sw<%d> "
			"ddbm_sw_rx<%d>, return\n", rxpkt_ready, ddbm_avail_sw,
			pktpgr->ddbm_sw_rx);

		if (rxpkt_ready && dev->pktpgr.pgi_rxpkt_req == 0) {
			if (!PGIEMERISRXSSTARVE(pktpgr)) {
				if ((ddbm_avail_sw + (HWA_PKTPGR_DDBM_SW_RESV/2)) > 0) {
					PGIEMERSETRXSSTARVE(pktpgr);
					ddbm_avail_sw = MIN(rxpkt_ready, 64);
					in_emergency = TRUE;
					goto ReqN;
				}
			}
			/* Re-schedule Rx to avoid Rx stop */
			dev->intstatus |= HWA_COMMON_INTSTATUS_D11BDEST0_INT_MASK;
			/* Re-schedule to consume the response ring */
			dev->intstatus |= HWA_COMMON_INTSTATUS_PACKET_PAGER_INTR_MASK;
			dev->pageintstatus |=
				(HWA_COMMON_PAGEIN_INT_MASK | HWA_COMMON_PAGEOUT_INT_MASK);
			pktpgr->pgi_rxpkt_fail++;
		}

		// For PHYRXS
		hwa_rxfill_bmac_done(dev, (uintptr)dev->rxfill.wlc[0], 0, 0, 0);
		return HWA_SUCCESS;
	}

ReqN:

	pkt_count     = MIN(pktpgr->pgi_rxpkt_count_max, rxpkt_ready);
	pkt_count     = MIN(ddbm_avail_sw, pkt_count);

	req.trans     = HWA_PP_PAGEIN_RXPROCESS;
	req.pkt_count = pkt_count; // requested number of packets
	// Use same pkt_count as bound for multiple read.
	req.pkt_bound = pkt_count;
	req.tagged    = HWA_PP_CMD_NOT_TAGGED; // not tagged req

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBFBF;
	}
#endif

	pktpgr_trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagein_req_ring, &req);
	if (pktpgr_trans_id == HWA_FAILURE) {
		HWA_ERROR(("%s PAGEIN::REQ RXPROCESS failure pkt<%u> bound<%u>\n",
			HWA2a, req.pkt_count, req.pkt_bound));
		return BCME_NORESOURCE;
	}

	if (in_emergency) {
		pktpgr->pgi_emer_id[PGI_EMER_TYPE_RX] = (uint8)pktpgr_trans_id;
		pktpgr->pgi_rx_emer++;
	}

	// DDBM accounting
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
	HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_rx, pkt_count);
	HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_rx, pktpgr->ddbm_sw_rx_hwm);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pkt_count,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	HWA_COUNT_INC(pktpgr->pgi_rxpkt_in_transit, pkt_count);
	HWA_COUNT_INC(pktpgr->pgi_rxpkt_req, 1);

	HWA_PP_DBG(HWA_PP_DBG_2A, "  >>PAGEIN::REQ RXPROCESS    : pkts %3u bound %3u "
		"==RX-REQ(%d)==>\n\n", req.pkt_count, req.pkt_bound, pktpgr_trans_id);

	// Next req if any.
	rxpkt_ready -= pkt_count;
	if (rxpkt_ready && pktpgr->pgi_rxpkt_req < pktpgr->pgi_rxpkt_req_max &&
		pktpgr->ddbm_avail_sw > (int)pktpgr->pgi_rxpkt_count_max) {
		goto ReqN;
	}

	return HWA_SUCCESS;
} // hwa_rxfill_pagein_rx_req

uint32 *
hwa_rxfill_pagein_rx_recv_histogram(struct hwa_dev *dev, bool clear)
{
	HWA_BCMDBG_EXPR({
		hwa_rxfill_t *rxfill;

		HWA_ASSERT(dev != (struct hwa_dev *)NULL);

		rxfill = &dev->rxfill;

		if (clear) {
			bzero(rxfill->rx_recv_histogram,
				sizeof(rxfill->rx_recv_histogram));
		}

		return rxfill->rx_recv_histogram;
	});

	return NULL;
}

void
hwa_rxfill_pagein_rx_rsp(hwa_dev_t *dev, uint8 pktpgr_trans_id,
	int pkt_count, hwa_pp_lbuf_t * pktlist_head, hwa_pp_lbuf_t * pktlist_tail)
{
	int pkt;
	uint32 u32;
	uint16 rxpkt_ready;
	hwa_rxfill_t *rxfill;
	hwa_pktpgr_t *pktpgr;
	wlc_info_t *wlc;
	hwa_pp_lbuf_t *rxlbuf, *rxlbuf_next;
	HWA_PP_DBG_EXPR(uint32 write);
	HWA_PP_DBG_EXPR(uint32 read);
	HWA_PP_DBG_EXPR(uint32 read22);

	HWA_FTRACE(HWA1b);

	HWA_AUDIT_DEV(dev);

	// Setup locals
	rxfill = &dev->rxfill;
	pktpgr = &dev->pktpgr;
	rxlbuf = pktlist_head;
	wlc = (wlc_info_t *)rxfill->wlc[0];

	// Do next request immediately if need.
	HWA_PP_DBG_EXPR({
		u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_wrindex);
		write = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_WR_INDEX);
		u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_rdindex);
		read = BCM_GBF(u32, HWA_RX_D11BDEST_RING_RDINDEX_RD_INDEX);
		read22 = BCM_GBF(u32, HWA_RX_D11BDEST_RING_RDINDEX_RD22_INDEX);
	});

	u32 = HWA_RD_REG_NAME(HWA1b, dev->regs, rx_core[0], d11bdest_ring_wrindex_dir);
	rxpkt_ready = BCM_GBF(u32, HWA_RX_D11BDEST_RING_WRINDEX_DIR_OCCUPIED_AFTER_MAC);

	HWA_PP_DBG(HWA_PP_DBG_2A, "   %s: write<%d> read<%d/%d> rxpkt_ready<%d>\n", __FUNCTION__,
		write, read, read22, rxpkt_ready);

	if (dev->reinit || !dev->up) {
		// Drop them since wlc_bmac_sts_reset has done.
		if (pkt_count != 0) {
			HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);

			// Free RPH, cannot free back to freerph ring because
			// later D11B reclaim may request many empty slots
			for (pkt = 0; pkt < pkt_count - 1; ++pkt) {
				// Audit it before free
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
				// Alloc: Rx reinit case
				hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM,
					DBM_AUDIT_ALLOC);
#endif
				// Free to Host
				hwa_rxpath_queue_rxcomplete_fast(dev, RPH_HOSTPKTID(rxlbuf));
				rxlbuf = (hwa_pp_lbuf_t *)PKTLINK(rxlbuf);
			}
			HWA_ASSERT(rxlbuf == pktlist_tail);

			// Audit it before free
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
			// Alloc: Rx reinit case
			hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM,
				DBM_AUDIT_ALLOC);
#endif
			// Free latest rxlbuf RPH to Host
			hwa_rxpath_queue_rxcomplete_fast(dev, RPH_HOSTPKTID(rxlbuf));

			// Commit.
			hwa_rxpath_xmit_rxcomplete_fast(dev);

			// Free packets(DDBM + HDBM)
			hwa_pktpgr_free_rx(dev, pktlist_head, pktlist_tail, pkt_count);
		}

		// Release emergency flag
		if (PGIEMERISRXSSTARVE(pktpgr) &&
			(pktpgr->pgi_emer_id[PGI_EMER_TYPE_RX] == pktpgr_trans_id)) {
			// Reset it.
			PGIEMERRESETRXSSTARVE(pktpgr);
		}

		return; // Return here
	} else if (rxpkt_ready > pktpgr->pgi_rxpkt_in_transit) {
		rxpkt_ready -= pktpgr->pgi_rxpkt_in_transit;
		(void)hwa_rxfill_pagein_rx_req(dev, 0, rxpkt_ready, HWA_PROCESS_BOUND);
	}

	// Process current response.
	HWA_PP_DBG(HWA_PP_DBG_2A, "  <<PAGEIN::RSP RXPROCESS    : pkts %3u list[%p(%d) .. %p(%d)] "
		"<==RX-RSP(%d)==\n\n", pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), pktpgr_trans_id);

	if (pkt_count == 0) {
		HWA_ERROR(("%s: pkt_count is 0\n", __FUNCTION__));
		goto done; // For PHYRXS
	}

	HWA_BCMDBG_EXPR({
		uint index;
		index = ((pkt_count - 1) >> HWA_PGI_RX_RECV_HISTOGRAM_UNIT_SHIFT);
		if (index >= HWA_PGI_RX_RECV_HISTOGRAM_MAX)
			rxfill->rx_recv_histogram[HWA_PGI_RX_RECV_HISTOGRAM_MAX-1]++;
		else
			rxfill->rx_recv_histogram[index]++;
	});

	HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);

	// Read and save TSF register once
	rxfill->tsf_l = R_REG(wlc->osh, D11_TSFTimerLow(wlc));

	// Rx: 24 Bytes data_buf_haddr64[1..3] memory can be used for data_buffer
	for (pkt = 0; pkt < pkt_count - 1; ++pkt) {
		rxlbuf_next = (hwa_pp_lbuf_t *)PKTLINK(rxlbuf);
		hwa_rxfill_bmac_recv(dev, (uintptr)rxfill->wlc[0], (uintptr)rxlbuf, 0, 0);
		rxlbuf = rxlbuf_next;
	}
	HWA_ASSERT(rxlbuf == pktlist_tail);

	// latest rxlbuf
	hwa_rxfill_bmac_recv(dev, (uintptr)rxfill->wlc[0], (uintptr)rxlbuf, 0, 0);

done:
	// Done processing all rx packets in fifo
	hwa_rxfill_bmac_done(dev, (uintptr)rxfill->wlc[0], 0, 0, pkt_count);

	// Release emergency flag
	if (PGIEMERISRXSSTARVE(pktpgr) &&
		(pktpgr->pgi_emer_id[PGI_EMER_TYPE_RX] == pktpgr_trans_id)) {
		// Reset it.
		PGIEMERRESETRXSSTARVE(pktpgr);
	}

	return;
}   // hwa_rxfill_pagein_rx_rsp()

static int
hwa_rxfill_bmac_recv(void *context, uintptr arg1, uintptr arg2,
	uint32 core, uint32 arg4)
{
	int ret;
	uint16 pktlen;
	hwa_dev_t *dev = (hwa_dev_t *)context;
	wlc_info_t *wlc = (wlc_info_t *)arg1;
	hwa_pp_lbuf_t *rxlbuf = (hwa_pp_lbuf_t *)arg2;
	hwa_rxfill_t *rxfill;
	wlc_d11rxhdr_t *wrxh;
	d11rxhdr_t *rxh;
	rxcpl_info_t *p_rxcpl_info;

	HWA_FTRACE(HWA2a);

	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(rxlbuf);

	// Setup locals
	ret = BCME_OK;
	rxfill = &dev->rxfill;
	p_rxcpl_info = NULL;

	HWA_PKT_DUMP_EXPR(_hwa_rxfill_pagein_rx_dump_pkt(rxlbuf,
		"PAGEIN_RXPKT", TRUE));

	// Audit it before pass to WL
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	// Alloc: Rx normal case
	hwa_pktpgr_dbm_audit(dev, rxlbuf, DBM_AUDIT_RX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif

	// Get resources.
	// XXX FIXME, error handing for rxcpl_id allocation failure.
	if (PKTRXCPLID(dev->osh, rxlbuf)) {
		HWA_ERROR(("  rxcpl_id must be zero [%d].\n", PKTRXCPLID(dev->osh, rxlbuf)));
	}

	p_rxcpl_info = bcm_alloc_rxcplinfo();
	if (p_rxcpl_info == NULL) {
		HWA_ERROR(("couldn't allocate rxcpl info: %p \n", rxlbuf));
		HWA_ASSERT(0);
		ret = BCME_NORESOURCE;
		goto toss;
	}
	PKTSETRXCPLID(dev->osh, rxlbuf, p_rxcpl_info->rxcpl_id.idx);

	// 4B:flags
	PKTRESETFLAGS(dev->osh, rxlbuf);
	// HWA RXFRAG packet
	PKTSETHWAPKT(dev->osh, rxlbuf);
	PKTSETRXFRAG(dev->osh, rxlbuf);

	// 4B:data
	HWA_ASSERT(PKTDATA(dev->osh, rxlbuf) != NULL);
	// Set head and end.
	PKTSETRXBUFRANGE(dev->osh, rxlbuf, ((uchar *)rxlbuf +
		HWA_PP_RXLFRAG_DATABUF_OFFSET), HWA_PP_RXLFRAG_DATABUF_LEN);
	// Clear poolid to make sure HDBM doesn't has garbage value remain.
	PKTSETPOOL_RX(dev->osh, rxlbuf, FALSE, NULL);

	// 2B:len
	rxh = (d11rxhdr_t *)PKTDATA(dev->osh, rxlbuf);
	pktlen = PKTLEN(dev->osh, rxlbuf);
	HWA_ASSERT(pktlen == D11RXHDR_ACCESS_VAL(rxh, wlc->pub->corerev, RxFrameSize));

	/* 32B:pkttag Lbuf Packet Tage */
	bzero((char *)PKTTAG(rxlbuf), HWA_PP_PKTTAG_BYTES);

	// 4B:host_pktid[0]
	HWA_ASSERT(PKTFRAGPKTID(dev->osh, rxlbuf) > 0);

	// It should be same as rxfill->rxp_data_buf_len
	HWA_ASSERT(PKTFRAGLEN(dev->osh, rxlbuf, LB_FRAG1) > 0);

	// 8B:data_buf_haddr64[0]
	HWA_ASSERT(PKTFRAGDATA_LO(dev->osh, rxlbuf, LB_FRAG1) > 0);

	// Frame length checking.
	pktlen += wlc->hwrxoff;
	if (pktlen == wlc->hwrxoff ||
		pktlen > rxfill->config.rx_size) {
		HWA_ERROR(("Invalid rx frame length %d, drop it @ %p\n", pktlen, rxlbuf));
		rxfill->rxgiants[core]++;
		ret = BCME_RXFAIL;
		goto toss;
	}

	// Dongle part

	// Push to SW RXHDR
	// The data set by HW is start from RX Status
	// We need push WLC_RXHDR_LEN bytes to SW RXHDR
	wrxh = (wlc_d11rxhdr_t *)PKTPUSH(dev->osh, rxlbuf, WLC_RXHDR_LEN);

	// Reset correct len, include hwrxoff
	PKTSETLEN(dev->osh, rxlbuf, pktlen);

#ifdef BULKRX_PKTLIST
	wlc_bmac_process_split_fifo_pkt(wlc->hw, rxlbuf, WLC_RXHDR_LEN);
#endif

	// Fillup Fifo info in rxstatus
	HWA_ASSERT(D11RXHDR_GE129_ACCESS_VAL(rxh, fifo) != RX_FIFO2);
	*(D11RXHDR_GE129_ACCESS_REF(rxh, fifo)) = (uint8)RX_FIFO1;

	// Record TSF info inside SW header
	wrxh->tsf_l = rxfill->tsf_l;

	HWA_TRACE(("%s frag<%p> data<%p> len <%d> head<%p> "
		"wlc_d11rxhdr_t<%d> used_len<%d>\n", __FUNCTION__,
		rxlbuf, PKTDATA(dev->osh, rxlbuf),
		PKTLEN(dev->osh, rxlbuf), PKTHEAD(dev->osh, rxlbuf),
		wlc->hwrxoff, PKTFRAGUSEDLEN(dev->osh, rxlbuf)));

	HWA_PKT_DUMP_EXPR(_hwa_rxfill_pagein_rx_dump_pkt(rxlbuf,
		"PAGEIN_RXPKT_RCEVD", TRUE));

	if (rxfill->rx_tail == NULL) {
		rxfill->rx_head = rxfill->rx_tail = rxlbuf;
	} else {
		PKTSETLINK(rxfill->rx_tail, rxlbuf);
		rxfill->rx_tail = rxlbuf;
	}

	/* Pkt processing Done */
	return HWA_SUCCESS;

toss:

	PKTSETLINK(rxlbuf, NULL);
	PKTFREE(dev->osh, rxlbuf, FALSE);
	return ret;
}

#else

/*
 * This is HWA RX handle function to process a HWA rxbuffer and construct a WL packet
 * to forward to WL subsystem.
 *
 * In order to compatible with legacy dongle WL driver, this function require a zero size of
 * data buffer lbuf_frag packet (need DHDHDR (split lbuf) support ) which is getting from
 * SHARED_RXFRAG_POOL.  Then this function will connect HWA rxbuffer data partion to
 * lbuf struct.
 *
 * FIXME: now we use pure SW wlc_rxframe_chainable function for PKTC.  If HWA 2a FHR,
 * PKTCLASS and AMT are enabled we can offload part of SW comparsion to it.
 *
 * After a WL packet is constructed, we pass it to WL subsystem through wlc_recv, once
 * CFP is enabled we should pass it to CFP instead.
 *
 * NOTE: FIXME: for now this function only consider FD mode.
 */
static int
hwa_rxfill_bmac_recv(void *context, uintptr arg1, uintptr arg2, uint32 core, uint32 arg4)
{
	hwa_dev_t *dev = (hwa_dev_t *)context;
	uchar *rxbuffer = (uchar *)arg2;
	hwa_rxfill_t *rxfill;
	hwa_rxpost_hostinfo_t *rph_req;
	wlc_info_t *wlc = (wlc_info_t *)arg1;
	struct lbuf_frag *frag;
	wlc_d11rxhdr_t *wrxh;
	d11rxhdr_t *rxh;
	uint16 rx_size;

	HWA_FTRACE(HWA2a);
	HWA_ASSERT(context != (void *)NULL);
	HWA_ASSERT(arg1 != 0);
	HWA_ASSERT(arg2 != 0);

	rxfill = &dev->rxfill;

	//prhex("hwa_rxfill_bmac_recv: rxbuffer Raw Data", (uchar *)rxbuffer, HWA_RXBUFFER_BYTES);

	rph_req = (hwa_rxpost_hostinfo_t *)rxbuffer;
	HWA_DEBUG_EXPR({
	// Show RPH value,
	if (dev->host_addressing & HWA_32BIT_ADDRESSING) { // 32bit host
		HWA_PRINT("%s rph32 alloc pktid<%u> haddr<0x%08x:0x%08x>\n", HWA1x,
			rph_req->hostinfo32.host_pktid, dev->host_physaddrhi,
			rph_req->hostinfo32.data_buf_haddr32);
	} else { // 64bit host
		HWA_PRINT("%s rph64 alloc pktid<%u> haddr<0x%08x:0x%08x>\n", HWA1x,
			rph_req->hostinfo64.host_pktid,
			rph_req->hostinfo64.data_buf_haddr64.hiaddr,
			rph_req->hostinfo64.data_buf_haddr64.loaddr);
	}});

	// Get one lbuf_frag from rxfrag pool.
	frag = (struct lbuf_frag *)pktpool_get_ext(SHARED_RXFRAG_POOL, lbuf_basic, NULL);
	if (frag == NULL)
		return HWA_FAILURE;

	/* Set up lbuf frag */
	/* Set this packet as HWA packet in first place */
	PKTSETHWAPKT(dev->osh, frag);
	PKTSETRXFRAG(dev->osh, frag);

	/* Load 64 bit host address */
	PKTSETFRAGDATA_HI(dev->osh, frag, LB_FRAG1, rph_req->hostinfo64.data_buf_haddr64.hiaddr);
	PKTSETFRAGDATA_LO(dev->osh, frag, LB_FRAG1, rph_req->hostinfo64.data_buf_haddr64.loaddr);

	/* frag len */
	PKTSETFRAGLEN(dev->osh, frag, LB_FRAG1, rxfill->rxp_data_buf_len);

	// Set host_pktid
	PKTSETFRAGPKTID(dev->osh, frag, rph_req->hostinfo64.host_pktid);
#if defined(BCMPCIE_IPC_HPA)
	hwa_rxpath_hpa_req_test(dev, rph_req->hostinfo64.host_pktid);
#endif
#ifdef BCM_BUZZZ
	BUZZZ_KPI_PKT1(KPI_PKT_MAC_RXFIFO, 1, rph_req->hostinfo64.host_pktid);
#endif

	// Start from RxStatus
	PKTSETBUF(dev->osh, frag, rxbuffer + rxfill->config.wrxh_offset, rxfill->config.rx_size);

	// Save the wrxh_offset value for rxbuffer point retrieving.
	PKTSETHWARXOFFSET(frag, rxfill->config.wrxh_offset);

	// SW RXHDR
	wrxh = (wlc_d11rxhdr_t *)PKTDATA(dev->osh, frag);
	rxh = &wrxh->rxhdr;

	// Dongle part
	rx_size = (uint16)MIN(rxfill->config.rx_size, wlc->hwrxoff +
		D11RXHDR_ACCESS_VAL(rxh, wlc->pub->corerev, RxFrameSize));
	PKTSETLEN(dev->osh, frag, rx_size);

#ifdef BULKRX_PKTLIST
	wlc_bmac_process_split_fifo_pkt(wlc->hw, frag, WLC_RXHDR_LEN);
#endif

	// Fillup Fifo info in rxstatus
	*(D11RXHDR_GE129_ACCESS_REF(rxh, fifo)) = (uint8)RX_FIFO1;

	// Record TSF info inside SW header
	wrxh->tsf_l = rxfill->tsf_l;

	HWA_TRACE(("%s frag<%p> rxbuffer[rph]<%p> data<%p> len <%d> head<%p> end<%p> "
		"wrxh_offset<%d> wlc_d11rxhdr_t<%d> used_len<%d>\n", HWA1x, frag,
		LBHWARXPKT(frag), PKTDATA(dev->osh, frag), PKTLEN(dev->osh, frag),
		PKTHEAD(dev->osh, frag), frag->lbuf.end,
		rxfill->config.wrxh_offset, wlc->hwrxoff,
		PKTFRAGUSEDLEN(dev->osh, frag)));

	// Audit it before pass to WL
#ifdef HWA_RXFILL_RXFREE_AUDIT_ENABLED
	_hwa_rxfill_rxfree_audit(rxfill, arg4, TRUE);
#endif

	if (rxfill->rx_tail == NULL) {
		rxfill->rx_head = rxfill->rx_tail = frag;
	} else {
		PKTSETLINK(rxfill->rx_tail, frag);
		rxfill->rx_tail = frag;
	}

	/* Pkt processing Done */
	return HWA_SUCCESS;
}

#endif /* HWA_PKTPGR_BUILD */

/*
 * This function will be called when we finished hwa_rxfill_rxbuffer_process() .
 * You can add some post processes in function if needed, for example PKTC stuff.
 * If there is any pending SW pktc chain, we pass it to WL through wlc_sendup_chain().
 * NOTE: FIXME: for now this function only consider FD mode.
 */
static int
hwa_rxfill_bmac_done(void *context, uintptr arg1, uintptr arg2, uint32 core, uint32 rxfifo_cnt)
{
	hwa_dev_t *dev = (hwa_dev_t *)context;
	wlc_info_t *wlc = (wlc_info_t *)arg1;
	hwa_rxfill_t *rxfill;
	void *p;
	rx_list_t rx_list = {NULL};
#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_RXSTS)
	rx_list_t rx_sts_list = {NULL};
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_RXSTS */
#ifdef PKTC_DONGLE
	void *pktc_head = NULL;
	void *pktc_tail = NULL;
	uint16 pktc_index = 0;
#endif
	HWA_FTRACE(HWA1b);
	HWA_ASSERT(context != (void *)NULL);
	HWA_ASSERT(arg1 != 0);

	rxfill = &dev->rxfill;

	/* Terminate PKTLINK */
	if (rxfill->rx_tail) {
		PKTSETLINK(rxfill->rx_tail, NULL);
	}

	rx_list.rx_head = rxfill->rx_head;
	rx_list.rx_tail = rxfill->rx_tail;

#if defined(STS_FIFO_RXEN) || defined(WLC_OFFLOADS_RXSTS)
	if (STS_RX_ENAB(wlc->pub) || STS_RX_OFFLOAD_ENAB(wlc->pub)) {
#if defined(STS_FIFO_RXEN)
		if (STS_RX_ENAB(wlc->pub)) {
			dma_sts_rx(wlc->hw->di[STS_FIFO], &rx_sts_list);
			if (rx_sts_list.rx_head != NULL) {
				wlc_bmac_dma_rxfill(wlc->hw, STS_FIFO);
			}
		}
#endif /* STS_FIFO_RXEN */
		(void) wlc_bmac_recv_process_sts(wlc->hw, RX_FIFO1, &rx_list,
			&rx_sts_list, WLC_RXHDR_LEN);
	}
#endif /* STS_FIFO_RXEN || WLC_OFFLOADS_RXSTS */

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(wlc->pub)) {
		wlc_sts_xfer_bmac_recv(wlc, RX_FIFO1, &rx_list, WLC_RXHDR_LEN);
	}
#endif /* STS_XFER_PHYRXS */

	while (rx_list.rx_head != NULL) {
		p = rx_list.rx_head;
		rx_list.rx_head = PKTLINK(p);
		PKTSETLINK(p, NULL);
		ASSERT(PKTNEXT(wlc->osh, p) == NULL);

#ifdef BULKRX_PKTLIST
		if (PKTISRXCORRUPTED(dev->osh, p)) {
			PKTFREE(dev->osh, p, FALSE);
			continue;
		}
#endif /* BULKRX_PKTLIST */

#if defined(STS_XFER_PHYRXS)
		{
			wlc_d11rxhdr_t *wrxh = (wlc_d11rxhdr_t *)PKTDATA(dev->osh, p);
			if (STS_XFER_PHYRXS_ENAB(wlc->pub) &&
				(!RXS_GE128_VALID_PHYRXSTS(&wrxh->rxhdr, wlc->pub->corerev))) {
				/* PhyRx Status is not valid */
				WLPKTTAG(p)->phyrxs_seqid = STS_XFER_PHYRXS_SEQID_INVALID;
				wrxh->radio_unit = HNDD11_RADIO_UNIT_INVALID;
			}
		}
#endif /* STS_XFER_PHYRXS */

#if defined(WLCFP)
		if (CFP_RCB_ENAB(wlc->cfp)) {
			/* Classify the packets based on per packet info. On the very first
			 * unchained packet, release all chained packets and continue.
			 */
			wlc_cfp_rxframe(wlc, p);
		} else
#endif /* WLCFP */
		{
			/* Legacy RX processing */
#ifdef PKTC_DONGLE
			if (PKTC_ENAB(wlc->pub) &&
				wlc_rxframe_chainable(wlc, (void **)&p, pktc_index)) {
				if (p != NULL) {
					PKTCENQTAIL(pktc_head, pktc_tail, p);
					pktc_index++;
				}
			} else
#endif /* PKTC_DONGLE */
			{
#ifdef PKTC_DONGLE
				if (pktc_tail) {
					/* pass to WL, wlc_sendup_chain will set up the tsf */
					wlc_sendup_chain(wlc, pktc_head);
					pktc_tail = NULL;
					pktc_index = 0;
				}
#endif /* PKTC_DONGLE */

				/* Legacy Slow RX path */
				wlc_recv(wlc, p);
			}
		}

	}

	rxfill->rx_head = rxfill->rx_tail = NULL;
	rx_list.rx_tail = NULL;

#if defined(WLCFP)
	if (CFP_RCB_ENAB(wlc->cfp)) {
		/* Sendup chained CFP packets */
		wlc_cfp_rx_sendup(wlc, NULL);
	} else
#endif /* WLCFP */
	{
#if defined(PKTC_DONGLE)
		if (pktc_tail) {
			/* pass to WL, wlc_sendup_chain will set up the tsf */
			wlc_sendup_chain(wlc, pktc_head);
		}
#endif /* PKTC_DONGLE */
	}

#if defined(STS_XFER_PHYRXS)
	if (STS_XFER_PHYRXS_ENAB(wlc->pub)) {
		wlc_sts_xfer_bmac_recv_done(wlc, RX_FIFO1);
	}
#endif /* STS_XFER_PHYRXS_ENAB */

	HWA_TRACE(("%s %s(): core<%u> rxfifo_cnt<%u>\n", HWA00, __FUNCTION__, core, rxfifo_cnt));
	return HWA_SUCCESS;
}

#endif /* HWA_RXFILL_BUILD */

#ifdef HWA_MAC_BUILD
/*
 * -----------------------------------------------------------------------------
 * Section: Configure HWA Common registers required per MAC Core
 * -----------------------------------------------------------------------------
 */

void // Invoked by MAC to configure the HWA Common block registers
hwa_mac_config(hwa_dev_t *dev, hwa_mac_config_t config,
	uint32 core, volatile void *ptr, uint32 val)
{
	uint32 v32;
	hwa_regs_t *regs;

	HWA_TRACE(("%s PHASE MAC config<%u> core<%u> ptr<%p> val<0x%08x,%u>\n",
		HWA00, config, core, ptr, val, val));

	// Audit parameters and pre-conditions
	HWA_ASSERT(core < HWA_RX_CORES);

	regs = dev->regs;

	switch (config) {

		case HWA_HC_HIN_REGS: // carries RxFilterEn and RxHwaCtrl registers
			HWA_ASSERT(ptr != (volatile void*)NULL);
			dev->mac_regs = (volatile hc_hin_regs_t*)ptr;
			break;

		case HWA_MAC_AXI_BASE:
			HWA_ASSERT(val != 0U);
			HWA_RXDATA_EXPR(dev->rxdata.mac_fhr_base = val + MAC_AXI_RXDATA_FHRTABLE);
			HWA_RXDATA_EXPR(dev->rxdata.mac_fhr_stats = val + MAC_AXI_RXDATA_FHRSTATS);
			break;

		case HWA_MAC_BASE_ADDR:
			HWA_ASSERT(val != 0U);
			if (core == 0) {
				HWA_WR_REG_NAME(HWA00, regs, common, mac_base_addr_core0, val);
			} else {
				HWA_WR_REG_NAME(HWA00, regs, common, mac_base_addr_core1, val);
			}
			break;

		case HWA_MAC_FRMTXSTATUS:
			v32 = HWA_RD_REG_NAME(HWA00, regs, common, mac_frmtxstatus);
			if (core == 0) {
				v32 = BCM_CBF(v32,
					HWA_COMMON_MAC_FRMTXSTATUS_MAC_FRMTXSTATUS_CORE0);
				v32 |= BCM_SBF(val,
					HWA_COMMON_MAC_FRMTXSTATUS_MAC_FRMTXSTATUS_CORE0);
			} else {
				v32 = BCM_CBF(v32,
					HWA_COMMON_MAC_FRMTXSTATUS_MAC_FRMTXSTATUS_CORE1);
				v32 |= BCM_SBF(val,
					HWA_COMMON_MAC_FRMTXSTATUS_MAC_FRMTXSTATUS_CORE1);
			}
			HWA_WR_REG_NAME(HWA00, regs, common, mac_frmtxstatus, v32);
			break;

		case HWA_MAC_DMA_PTR:
			v32 = HWA_RD_REG_NAME(HWA00, regs, common, mac_dma_ptr);
			if (core == 0)
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_DMA_PTR_MAC_DMA_PTR_CORE0);
			else
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_DMA_PTR_MAC_DMA_PTR_CORE1);
			HWA_WR_REG_NAME(HWA00, regs, common, mac_dma_ptr, v32);
			break;

		case HWA_MAC_IND_XMTPTR:
			v32 = HWA_RD_REG_NAME(HWA00, regs, common, mac_ind_xmtptr);
			if (core == 0) {
				v32 = BCM_CBF(v32, HWA_COMMON_MAC_IND_XMTPTR_MAC_XMT_PTR_CORE0);
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_IND_XMTPTR_MAC_XMT_PTR_CORE0);
			}
			else {
				v32 = BCM_CBF(v32, HWA_COMMON_MAC_IND_XMTPTR_MAC_XMT_PTR_CORE1);
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_IND_XMTPTR_MAC_XMT_PTR_CORE1);
			}
			HWA_WR_REG_NAME(HWA00, regs, common, mac_ind_xmtptr, v32);
			break;

		case HWA_MAC_IND_QSEL:
			v32 = HWA_RD_REG_NAME(HWA00, regs, common, mac_ind_qsel);
			if (core == 0) {
				v32 = BCM_CBF(v32, HWA_COMMON_MAC_IND_QSEL_MAC_INDQSEL_CORE0);
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_IND_QSEL_MAC_INDQSEL_CORE0);
			}
			else {
				v32 = BCM_CBF(v32, HWA_COMMON_MAC_IND_QSEL_MAC_INDQSEL_CORE1);
				v32 |= BCM_SBF(val, HWA_COMMON_MAC_IND_QSEL_MAC_INDQSEL_CORE1);
			}
			HWA_WR_REG_NAME(HWA00, regs, common, mac_ind_qsel, v32);
			break;

		default:
			HWA_ERROR(("%s HWA[%d]  mac config<%u> invalid\n", HWA00, dev->unit,
				config));
			break;

	} // switch

} // hwa_mac_config

#endif /* HWA_MAC_BUILD */

#ifdef HWA_RXDATA_BUILD
/*
 * -----------------------------------------------------------------------------
 * Section: HWA2a RxData block bringup: attach, detach, and init phases
 * -----------------------------------------------------------------------------
 */

#ifdef HWA_RXDATA_FHR_IND_BUILD
// FIXME: Determine hos to use direct access, and use same solution for both
// FHR Register file and FHR counter statistics. Then remove this function.
static void // Use Indirect Access to configure FHR register file
hwa_rxdata_fhr_indirect_write(hwa_rxdata_t *rxdata,
	hwa_rxdata_fhr_entry_t *filter)
{
	int i, j;
	uint32 v32;
	hwa_dev_t *dev;
	volatile hc_hin_regs_t *mac_regs;
	hwa_rxdata_fhr_param_t *param;

	HWA_TRACE(("%s FHR write filter<%u>\n", HWA2a, filter->config.id));

	// Audit pre-conditions
	dev = HWA_DEV(rxdata);

	HWA_ASSERT(dev->mac_regs != (hc_hin_regs_t*)NULL);

	// Setup locals
	mac_regs = dev->mac_regs;

	// Select FHR
	v32 = (0U
		| BCM_SBF(MAC_AXI_RXDATA_FHR_RF(filter->config.id), _OBJADDR_INDEX)
		| BCM_SBF(MAC_AXI_RXDATA_FHR_SELECT, _OBJADDR_SELECT)
		| BCM_SBIT(_OBJADDR_WRITEINC)
		| 0U);

	HWA_WR_REG_ADDR(HWA2a, &mac_regs->objaddr, v32);
	v32 = HWA_RD_REG_ADDR(HWA2a, &mac_regs->objaddr); // ensure WR completes

	// Write-out the entire filter using WR address auto increment and ObjData
	v32 = filter->u32[0]; // hwa_rxdata_fhr_entry_t::config
	HWA_WR_REG_ADDR(HWA2a, &mac_regs->objdata, v32);

	for (i = 0; i < HWA_RXDATA_FHR_PARAMS_MAX; i++) {
		param = &filter->params[i];
		for (j = 0; j < HWA_RXDATA_FHR_PARAM_WORDS; j++) { // 5 Words
			v32 = param->u32[j];
			HWA_WR_REG_ADDR(HWA2a, &mac_regs->objdata, v32);
		} // for params: config, bitmask[0..31, 32..63] pattern[0..31, 32..63]
	} // for param_count

} // hwa_rxdata_fhr_indirect_write

#endif /* HWA_RXDATA_FHR_IND_BUILD */

void // HWA2a RxData: Cleanup/Free resources used by RxData block
BCMATTACHFN(hwa_rxdata_detach)(hwa_rxdata_t *rxdata)
{
	HWA_FTRACE(HWA2a);
	// ... placeholder
} // hwa_rxdata_detach

hwa_rxdata_t *
BCMATTACHFN(hwa_rxdata_attach)(hwa_dev_t *dev)
{
	int i;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// 32bit map used to track FHR filter types: see fhr_pktfetch, fhr_l2filter
	HWA_ASSERT(HWA_RXDATA_FHR_FILTERS_MAX == (NBBY * NBU32));
	HWA_ASSERT(sizeof(hwa_rxdata_fhr_entry_t) == HWA_RXDATA_FHR_ENTRY_BYTES);
	HWA_ASSERT(sizeof(hwa_rxdata_fhr_t) == HWA_RXDATA_FHR_BYTES);

	rxdata = &dev->rxdata;

	// Place all FHR filters into the free list
	for (i = 0; i < HWA_RXDATA_FHR_FILTERS_SW; i++) {
		rxdata->fhr[i].next = &rxdata->fhr[i + 1];
		rxdata->fhr[i].filter.config.id = i;
		rxdata->fhr[i].filter.config.type = HWA_RXDATA_FHR_FILTER_DISABLED;
	}
	rxdata->fhr[i - 1].next = (hwa_rxdata_fhr_filter_t*)NULL;
	rxdata->fhr_flist = &rxdata->fhr[0];

	return rxdata;

} // hwa_rxdata_attach

void // HWA2a RxData: Init RxData block after MAC has setup mac_fhr_base
hwa_rxdata_init(hwa_rxdata_t *rxdata)
{
	uint32 v32;
	hwa_dev_t *dev;

	HWA_FTRACE(HWA2a);

	// Audit pre-conditions
	dev = HWA_DEV(rxdata);

	// Ensure that MAC has initialized mac_fhr_base, mac_fhr_stats and mac_regs
	HWA_ASSERT(rxdata->mac_fhr_base != 0U);
	HWA_ASSERT(rxdata->mac_fhr_stats != 0U);
	HWA_ASSERT(dev->mac_regs != (hc_hin_regs_t*)NULL);

	v32 = BCM_SBIT(_RXHWACTRL_GLOBALFILTEREN) |
		BCM_SBIT(_RXHWACTRL_CLRALLFILTERSTAT);
	if (HWAREV_GE(dev->corerev, 130)) {
		v32 |= BCM_SBIT(_RXHWACTRL_PKTCOMPEN);
	}
	HWA_WR_REG_ADDR(HWA2a, &dev->mac_regs->rxhwactrl, v32);

	// Rx filter configuration
	if (!dev->inited) {
		hwa_rxdata_fhr_filter_init_pktfetch(rxdata);
	} else {
		// After MAC reset, rx filter configuration will be reset.
		// Driver need to reconfigure it.
		hwa_rxdata_fhr_filter_reinit(rxdata);
	}

} // hwa_rxdata_init

void // HWA2a RxData: Deinit RxData block
hwa_rxdata_deinit(hwa_rxdata_t *rxdata)
{
	hwa_dev_t *dev;

	HWA_FTRACE(HWA2a);

	// Audit pre-conditions
	dev = HWA_DEV(rxdata);

	// Ensure that MAC has initialized mac_fhr_base, mac_fhr_stats and mac_regs
	HWA_ASSERT(rxdata->mac_fhr_base != 0U);
	HWA_ASSERT(rxdata->mac_fhr_stats != 0U);
	HWA_ASSERT(dev->mac_regs != (hc_hin_regs_t*)NULL);

	HWA_WR_REG_ADDR(HWA2a, &dev->mac_regs->rxhwactrl, 0);

}

// HWA2a RxData FHR Table Management

// VeriWave specific UDP source/destination port numbers
#define VW_UDP_SRC_PORT	(45000)
#define VW_UDP_DST_PORT	(46000)

// Init exist known pktfetch filter
void
hwa_rxdata_fhr_filter_init_pktfetch(hwa_rxdata_t *rxdata)
{
	hwa_dev_t *dev;
	wlc_info_t *wlc;
	int32 fid;
	uint32 offset, bitmask, pattern;

	HWA_FTRACE(HWA2a);

	// Audit pre-conditions
	dev = HWA_DEV(rxdata);

	wlc = (wlc_info_t *)dev->wlc;
	BCM_REFERENCE(wlc);

	/* HW will take care ETHER_TYPE_1_OFFSET by itself. */

	/* EAPOL_FETCH */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_802_1X);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);
		hwa_rxdata_fhr_filter_add(fid);
	}

#ifdef MULTIAP
	/* EAPOL_FETCH if it is VLAN TAGGED */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 2);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_8021Q);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset += VLAN_TAG_LEN;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_802_1X);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		hwa_rxdata_fhr_filter_add(fid);
	}
#endif /* MULTIAP */

	/* Set a filter for NON IP packet
	 *
	 * HW should set match the filter if incoming packet is not IPV4 and not IPV6
	 * Set the polarity fields of the params
	 */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 2);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IP);
		hwa_rxdata_fhr_param_add(fid, 1, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IPV6);
		hwa_rxdata_fhr_param_add(fid, 1, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		hwa_rxdata_fhr_filter_add(fid);
	}

	/* Multicast Filter : addr[0] & 1 == 1 */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 1);
	if (fid != HWA_FAILURE) {
		offset = HW_HDR_CONV_PAD;	/* 1st byte in DA */
		bitmask = 0x1;
		pattern = 0x1;	/* Check for 0x1 */
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 1);
		hwa_rxdata_fhr_filter_add(fid);
	}

	/* Packet filter to catch invalid AMSDU transmission from 4360 sta side */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_LLC_SNAP_DA, 0, 2);
	if (fid != HWA_FAILURE) {
		offset = HW_HDR_CONV_PAD;
		bitmask = 0xffffffff;
		pattern = HTON32(0xAAAA0300);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 4);

		offset = HW_HDR_CONV_PAD + 4;
		bitmask = 0xffff;
		pattern = HTON16(0x0000);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		hwa_rxdata_fhr_filter_add(fid);
	}
#ifdef WLTDLS
	/* TDLS_PKTFETCH */
	if (TDLS_ENAB(wlc->pub)) {
		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_89_0D);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);
			hwa_rxdata_fhr_filter_add(fid);
		}
	}
#endif /* WLTDLS */

#ifdef BCMWAPI_WAI
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_WAI);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);
		hwa_rxdata_fhr_filter_add(fid);
	}
#endif /* BCMWAPI_WAI */

	/* Dynamic frameburst filters */
	/* UDPv6 Packet */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 4);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IPV6);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset += (ETHER_TYPE_LEN + IPV6_NEXT_HDR_OFFSET);
		bitmask = 0xff;
		pattern = IP_PROT_UDP;
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 1);

		/* source port */
		offset += (IPV6_SRC_IP_OFFSET - IPV6_NEXT_HDR_OFFSET + IPV6_ADDR_LEN * 2);
		bitmask = 0xffff;
		pattern = HTON16(VW_UDP_SRC_PORT);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		/* destination port */
		offset += UDP_DEST_PORT_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(VW_UDP_DST_PORT);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		hwa_rxdata_fhr_filter_add(fid);
		rxdata->udpv6_filter |= BCM_BIT(fid);
		rxdata->chainable_filters |= BCM_BIT(fid);
	}
	/* UDPv4 Packet */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 4);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IP);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset += (ETHER_TYPE_LEN + IPV4_PROT_OFFSET);
		bitmask = 0xff;
		pattern = IP_PROT_UDP;
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 1);

		/* source port */
		offset += (IPV4_MIN_HEADER_LEN - IPV4_PROT_OFFSET);
		bitmask = 0xffff;
		pattern = HTON16(VW_UDP_SRC_PORT);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		/* destination port */
		offset += UDP_DEST_PORT_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(VW_UDP_DST_PORT);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		hwa_rxdata_fhr_filter_add(fid);
		rxdata->udpv4_filter |= BCM_BIT(fid);
		rxdata->chainable_filters |= BCM_BIT(fid);
	}
	/* TCPv6 Packet */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 2);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IPV6);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset += (ETHER_TYPE_LEN + IPV6_NEXT_HDR_OFFSET);
		bitmask = 0xff;
		pattern = IP_PROT_TCP;
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 1);

		hwa_rxdata_fhr_filter_add(fid);
		rxdata->tcp_filter |= BCM_BIT(fid);
		rxdata->chainable_filters |= BCM_BIT(fid);
	}
	/* TCPv4 Packet */
	fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_L2FILTER, 0, 2);
	if (fid != HWA_FAILURE) {
		offset = ETHER_TYPE_2_OFFSET;
		bitmask = 0xffff;
		pattern = HTON16(ETHER_TYPE_IP);
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 2);

		offset += (ETHER_TYPE_LEN + IPV4_PROT_OFFSET);
		bitmask = 0xff;
		pattern = IP_PROT_TCP;
		hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask, (uint8 *)&pattern, 1);

		hwa_rxdata_fhr_filter_add(fid);
		rxdata->tcp_filter |= BCM_BIT(fid);
		rxdata->chainable_filters |= BCM_BIT(fid);
	}
}

void
hwa_rxdata_fhr_filter_reinit(hwa_rxdata_t *rxdata)
{
	hwa_dev_t *dev;
	hwa_rxdata_fhr_entry_t *filter;
	hwa_rxdata_fhr_filter_type_t filter_type;
	int id;

	HWA_FTRACE(HWA2a);

	// Audit pre-conditions
	dev = HWA_DEV(rxdata);

	for (id = 0; id < HWA_RXDATA_FHR_FILTERS_SW; id++) {
		filter = &rxdata->fhr[id].filter;
		if (filter->config.type != HWA_RXDATA_FHR_FILTER_DISABLED) {
			filter_type = filter->config.type; // SW only
			filter->config.type = 0U; // SW use only

#ifdef HWA_RXDATA_FHR_IND_BUILD
			hwa_rxdata_fhr_indirect_write(rxdata, filter);
#else  /* ! HWA_RXDATA_FHR_IND_BUILD */
			{
				hwa_mem_addr_t fhr_addr;
				// Copy the filter to the HWA2a FHR Reg File using 32bit AXI access
				fhr_addr = HWA_TABLE_ADDR(hwa_rxdata_fhr_entry_t,
					rxdata->mac_fhr_base, filter_id);

				// FIXME: Reference section 5.1.2 for Direct Access using AXI Slave
				HWA_WR_MEM32(HWA2a, hwa_rxdata_fhr_entry_t, fhr_addr, filter);
			}
#endif /* ! HWA_RXDATA_FHR_IND_BUILD */

			filter->config.type = filter_type; // SW only
		}
	}

	HWA_WR_REG_ADDR(HWA2a, &dev->mac_regs->rxfilteren, rxdata->rxfilteren);

}

#ifdef WLNDOE
// FIXME: Enable filter in the right place.
void
hwa_rxdata_fhr_filter_ndoe(bool enable)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	int32 fid, i;
	uint32 offset, bitmask, pattern;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	if (enable) {
		if (rxdata->ndoe_filter != 0) {
			// Already enable it, do nothing.
			return;
		}

		// NDOE_PKTFETCH_NS
		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 3);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_IPV6);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);

			offset += (ETHER_TYPE_LEN + ICMP6_NEXTHDR_OFFSET_SPLIT_MODE4);
			bitmask = 0xff;
			pattern = ICMPV6_HEADER_TYPE;
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 1);

			offset += (ICMP6_TYPE_OFFSET_SPLIT_MODE4 -
				ICMP6_NEXTHDR_OFFSET_SPLIT_MODE4);
			bitmask = 0xff;
			pattern = ICMPV6_PKT_TYPE_NS;
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 1);
			hwa_rxdata_fhr_filter_add(fid);
			rxdata->ndoe_filter |= BCM_BIT(fid);
		}

		/* NDOE_PKTFETCH_RA */
		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 3);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_IPV6);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);

			offset += (ETHER_TYPE_LEN + ICMP6_NEXTHDR_OFFSET_SPLIT_MODE4);
			bitmask = 0xff;
			pattern = ICMPV6_HEADER_TYPE;
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 1);

			offset += (ICMP6_TYPE_OFFSET_SPLIT_MODE4 -
				ICMP6_NEXTHDR_OFFSET_SPLIT_MODE4);
			bitmask = 0xff;
			pattern = ICMPV6_PKT_TYPE_RA;
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 1);
			hwa_rxdata_fhr_filter_add(fid);
			rxdata->ndoe_filter |= BCM_BIT(fid);
		}
	} else {
		if (rxdata->ndoe_filter) {
			for (i = 0; i < HWA_RXDATA_FHR_FILTERS_MAX; i++) {
				if (isset(&rxdata->ndoe_filter, i)) {
					hwa_rxdata_fhr_filter_del(i);
				}
			}
			rxdata->ndoe_filter = 0;
		}
	}

}
#endif /* WLNDOE */

#if defined(ICMP)
// FIXME: Enable filter in the right place.
void
hwa_rxdata_fhr_filter_ip(bool enable)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	int32 fid, i;
	uint32 offset, bitmask, pattern;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	if (enable) {
		if (rxdata->ip_filter != 0) {
			// Already enable it, do nothing.
			return;
		}

		// PKTFETCH_IPV4
		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_IP);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);
			hwa_rxdata_fhr_filter_add(fid);
			rxdata->ip_filter |= BCM_BIT(fid);
		}

		// PKTFETCH_IPV6
		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_IPV6);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);
			hwa_rxdata_fhr_filter_add(fid);
			rxdata->ip_filter |= BCM_BIT(fid);
		}
	} else {
		if (rxdata->ip_filter) {
			for (i = 0; i < HWA_RXDATA_FHR_FILTERS_MAX; i++) {
				if (isset(&rxdata->ip_filter, i)) {
					hwa_rxdata_fhr_filter_del(i);
				}
			}
			rxdata->ip_filter = 0;
		}
	}

}
#endif /* defined(ICMP) */

#ifdef WL_TBOW
// FIXME: Enable filter in the right place.
void
hwa_rxdata_fhr_filter_tbow(bool enable)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	int32 fid, i;
	uint32 offset, bitmask, pattern;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	if (enable) {
		if (rxdata->tbow_filter != 0) {
			// Already enable it, do nothing.
			return;
		}

		fid = hwa_rxdata_fhr_filter_new(HWA_RXDATA_FHR_PKTFETCH, 0, 1);
		if (fid != HWA_FAILURE) {
			offset = ETHER_TYPE_2_OFFSET;
			bitmask = 0xffff;
			pattern = HTON16(ETHER_TYPE_TBOW);
			hwa_rxdata_fhr_param_add(fid, 0, offset, (uint8 *)&bitmask,
				(uint8 *)&pattern, 2);
			hwa_rxdata_fhr_filter_add(fid);
			rxdata->tbow_filter |= BCM_BIT(fid);
		}
	} else {
		if (rxdata->tbow_filter) {
			for (i = 0; i < HWA_RXDATA_FHR_FILTERS_MAX; i++) {
				if (isset(&rxdata->tbow_filter, i)) {
					hwa_rxdata_fhr_filter_del(i);
				}
			}
			rxdata->tbow_filter = 0;
		}
	}

}
#endif /* WL_TBOW */

int // Allocate a new filter
hwa_rxdata_fhr_filter_new(hwa_rxdata_fhr_filter_type_t filter_type,
	uint32 filter_polarity, uint32 param_count)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	hwa_rxdata_fhr_entry_t *filter;

	HWA_TRACE(("%s new filter type<%u> polarity<%u> count<%u>\n",
		HWA2a, filter_type, filter_polarity, param_count));

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	HWA_ASSERT((param_count > 0) && (param_count < HWA_RXDATA_FHR_PARAMS_MAX));

	if (rxdata->fhr_flist == (hwa_rxdata_fhr_filter_t*)NULL) {
		HWA_WARN(("%s filter new fail\n", HWA2a));

		HWA_STATS_EXPR(rxdata->fhr_err_cnt++);

		return HWA_FAILURE;
	}

	if (rxdata->fhr_build != (hwa_rxdata_fhr_filter_t*)NULL) {
		HWA_WARN(("%s ovewriting filter<%u>\n",
			HWA2a, rxdata->fhr_build->filter.config.id));
	} else {
		rxdata->fhr_build = rxdata->fhr_flist;
		rxdata->fhr_flist = rxdata->fhr_flist->next;
		rxdata->fhr_build->next = NULL;
	}

	// Prepare the filter context ...
	filter = &rxdata->fhr_build->filter;

	filter->config.type        = filter_type; // SW only
	filter->config.polarity    = filter_polarity;
	filter->config.param_count = param_count;

	rxdata->param_count = 0U;

	return filter->config.id;

} // hwa_rxdata_fhr_filter_new

// Build the filter by specifying parameters, and finally configure in HWA FHR
int // Build a new filter by adding a parameter
hwa_rxdata_fhr_param_add(uint32 filter_id, uint32 polarity, uint32 offset,
	uint8 *bitmask, uint8 *pattern, uint32 match_sz)
{
	int i, j;
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	hwa_rxdata_fhr_param_t *param;
	hwa_rxdata_fhr_entry_t *filter;
	uint32 prefix;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	HWA_ASSERT(filter_id < HWA_RXDATA_FHR_FILTERS_MAX);
	HWA_ASSERT(offset < (1 << 12)); // 11 bit offset in hwa_rxdata_fhr_entry_t
	HWA_ASSERT((bitmask != NULL) && (pattern != NULL));
	HWA_ASSERT(match_sz <= HWA_RXDATA_FHR_PATTERN_BYTES);

	HWA_ASSERT(rxdata->fhr_build != (hwa_rxdata_fhr_filter_t*)NULL);
	HWA_ASSERT(rxdata->param_count <
	           rxdata->fhr_build->filter.config.param_count);

	// Fetch filter
	filter = &rxdata->fhr_build->filter;
	HWA_ASSERT(filter->config.id == filter_id);

	// Shift the last valid byte to the end so that we can
	// filter small length packet.  i.e. EAPOL-WSC-START
	prefix = HWA_RXDATA_FHR_PATTERN_BYTES - match_sz;
	if (offset >= prefix) {
		offset = offset - prefix;
	}
	else {
		prefix = offset;
		offset = 0;
	}

	// Add parameter to the filter
	param = &filter->params[rxdata->param_count];
	param->config.polarity = polarity;
	param->config.offset   = offset;
	for (i = 0; i < prefix; i++) {
		param->bitmask[i]  = 0x00;
		param->pattern[i]  = 0x00;
	}
	for (j = 0; j < match_sz; i++, j++) {
		param->bitmask[i]  = bitmask[j];
		param->pattern[i]  = pattern[j];
	}
	for (; i < HWA_RXDATA_FHR_PATTERN_BYTES; i++) {
		param->bitmask[i]  = 0x00;
		param->pattern[i]  = 0x00;
	}

	rxdata->param_count++;

	HWA_ASSERT(rxdata->param_count <= filter->config.param_count);

	return filter_id;

} // hwa_rxdata_fhr_param_add

int // Add the built filter into MAC FHR register file
hwa_rxdata_fhr_filter_add(uint32 filter_id)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	hc_hin_regs_t *mac_regs;
	hwa_rxdata_fhr_entry_t *filter;
	hwa_rxdata_fhr_filter_type_t filter_type;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;
	mac_regs = dev->mac_regs;

	HWA_ASSERT(rxdata->mac_fhr_base != 0U);
	HWA_ASSERT(mac_regs != (hc_hin_regs_t*)NULL);
	HWA_ASSERT(filter_id < HWA_RXDATA_FHR_FILTERS_MAX);
	HWA_ASSERT(rxdata->fhr_build != (hwa_rxdata_fhr_filter_t*)NULL);

	// Setup locals
	filter = &rxdata->fhr_build->filter;
	HWA_ASSERT(filter->config.id == filter_id);
	HWA_ASSERT(filter->config.param_count == rxdata->param_count);

	filter_type = filter->config.type; // SW only
	switch (filter_type) {
		case HWA_RXDATA_FHR_PKTFETCH:
			setbit(&rxdata->fhr_pktfetch, filter_id); break;
		case HWA_RXDATA_FHR_L2FILTER:
			setbit(&rxdata->fhr_l2filter, filter_id); break;
		case HWA_RXDATA_FHR_LLC_SNAP_DA:
			setbit(&rxdata->llc_snap_da_filter, filter_id); break;
		default: HWA_ASSERT(0); break;
	}

	filter->config.type = 0U; // SW use only

	// This count indicates the number of patterns to match.
	// A value of 0 indicates one pattern to match and so on.
	// This Param Count cannot be greater than the value that is supported by a FHR.
	filter->config.param_count -= 1;

#ifdef HWA_RXDATA_FHR_IND_BUILD
	hwa_rxdata_fhr_indirect_write(rxdata, filter);
#else  /* ! HWA_RXDATA_FHR_IND_BUILD */
	{
		hwa_mem_addr_t fhr_addr;
		// Copy the filter to the HWA2a FHR Reg File using 32bit AXI access
		fhr_addr = HWA_TABLE_ADDR(hwa_rxdata_fhr_entry_t,
		                              rxdata->mac_fhr_base, filter_id);

		// FIXME: Reference section 5.1.2 for Direct Access using AXI Slave
		HWA_WR_MEM32(HWA2a, hwa_rxdata_fhr_entry_t, fhr_addr, filter);
	}
#endif /* ! HWA_RXDATA_FHR_IND_BUILD */

	// Enable the filter
	setbit(&rxdata->rxfilteren, filter_id);
	HWA_WR_REG_ADDR(HWA2a, &mac_regs->rxfilteren, rxdata->rxfilteren);

	HWA_STATS_EXPR(rxdata->fhr_ins_cnt++);

	filter->config.type = filter_type; // SW only

	rxdata->param_count = 0U;
	rxdata->fhr_build = (hwa_rxdata_fhr_filter_t*)NULL;

	return filter_id;

} // hwa_rxdata_fhr_filter_add

int // Delete a previously configured FHR filter
hwa_rxdata_fhr_filter_del(uint32 filter_id)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	hc_hin_regs_t *mac_regs;
	hwa_rxdata_fhr_filter_t *fhr_filter;
	hwa_rxdata_fhr_entry_t *filter;
	hwa_rxdata_fhr_filter_type_t filter_type;

	HWA_FTRACE(HWA2a);

	// Audit preconditions
	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;
	mac_regs = dev->mac_regs;

	HWA_ASSERT(rxdata->mac_fhr_base != 0U);
	HWA_ASSERT(mac_regs != (hc_hin_regs_t*)NULL);
	HWA_ASSERT(filter_id < HWA_RXDATA_FHR_FILTERS_MAX);
	HWA_ASSERT(isset(&rxdata->rxfilteren, filter_id));

	// Setup locals
	fhr_filter = &rxdata->fhr[filter_id];
	filter = &fhr_filter->filter;
	HWA_ASSERT(filter->config.id == filter_id);

	filter_type = filter->config.type; // SW only
	switch (filter_type) {
		case HWA_RXDATA_FHR_PKTFETCH:
			clrbit(&rxdata->fhr_pktfetch, filter_id); break;
		case HWA_RXDATA_FHR_L2FILTER:
			clrbit(&rxdata->fhr_l2filter, filter_id); break;
		case HWA_RXDATA_FHR_LLC_SNAP_DA:
			clrbit(&rxdata->llc_snap_da_filter, filter_id); break;
		default: HWA_ASSERT(0); break;
	}

	filter->config.type = 0;
	filter->config.polarity = 0;
	filter->config.param_count = 0;
	memset(filter->params, 0, HWA_RXDATA_FHR_ENTRY_PARAMS_BYTES);

	// Disable the filter
	clrbit(&rxdata->rxfilteren, filter_id);
	HWA_WR_REG_ADDR(HWA2a, &mac_regs->rxfilteren, rxdata->rxfilteren);

#ifdef HWA_RXDATA_FHR_IND_BUILD
	hwa_rxdata_fhr_indirect_write(rxdata, filter);
#else  /* ! HWA_RXDATA_FHR_IND_BUILD */
	{
		hwa_mem_addr_t fhr_addr;
		// Copy the filter to the HWA2a FHR Reg File using 32bit AXI access
		fhr_addr = HWA_TABLE_ADDR(hwa_rxdata_fhr_entry_t,
		                              rxdata->mac_fhr_base, filter_id);

		// FIXME: Reference section 5.1.2 for Direct Access using AXI Slave
		HWA_WR_MEM32(HWA2a, hwa_rxdata_fhr_entry_t, fhr_addr, filter);
	}
#endif /* ! HWA_RXDATA_FHR_IND_BUILD */

	HWA_STATS_EXPR(rxdata->fhr_del_cnt++);

	filter->config.type = HWA_RXDATA_FHR_FILTER_DISABLED;

	if (rxdata->fhr_flist == (hwa_rxdata_fhr_filter_t*)NULL) {
		rxdata->fhr_flist = fhr_filter;
	} else {
		fhr_filter->next = rxdata->fhr_flist;
		rxdata->fhr_flist = fhr_filter;
	}

	return filter_id;

} // hwa_rxdata_fhr_filter_del

uint32 // Determine whether any filters hit a pktfetch filter type
hwa_rxdata_fhr_is_pktfetch(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
#ifdef WOWLPF
	wlc_info_t *wlc;
#endif

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

#ifdef WOWLPF
	wlc = (wlc_info_t *)dev->wlc;
	if (WOWLPF_ACTIVE(wlc->pub))
		return TRUE;
#endif

	return (rxdata->fhr_pktfetch & fhr_filter_match);

} // hwa_rxdata_fhr_is_pktfetch

uint32 // Determine whether any filters hit a l2filter type
hwa_rxdata_fhr_is_l2filter(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (rxdata->fhr_l2filter & fhr_filter_match);
}

// Identified an RX corruption at 43684 side with 4360 sta transmitting.
// The pattern which leads to corruption is as below
//
// AMSDU QoS bit in d11header is set but subframne header is missing.
// LLC SNAP header of AA AA 03 00 00 00 appears at the place of subframe header.
// WAR at 43684 side to filter out such packets instead of trapping.
uint32 // Determine whether any filters hit a llc snap DA type [aa aa 03 00 00 00]
hwa_rxdata_fhr_is_llc_snap_da(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (rxdata->llc_snap_da_filter & fhr_filter_match);
}

uint32 // Determine whether any filters hit a UDPv6 type
hwa_rxdata_fhr_is_udpv6(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (rxdata->udpv6_filter & fhr_filter_match);
}

uint32 // Determine whether any filters hit a UDPv4 type
hwa_rxdata_fhr_is_udpv4(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (rxdata->udpv4_filter & fhr_filter_match);
}

uint32 // Determine whether any filters hit a TCP type
hwa_rxdata_fhr_is_tcp(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (rxdata->tcp_filter & fhr_filter_match);
}

uint32 // Clear chainable filter bits
hwa_rxdata_fhr_unchainable(uint32 fhr_filter_match)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	return (fhr_filter_match & ~(rxdata->chainable_filters));
}

// FHR match statistics management
uint32 // Get the hit statistics for a filter
hwa_rxdata_fhr_hits_get(uint32 filter_id)
{
	hwa_dev_t *dev;
	hwa_rxdata_t *rxdata;
	uint32 filter_hits;
	hwa_mem_addr_t fhr_stats_addr;

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	rxdata = &dev->rxdata;

	HWA_ASSERT(rxdata->mac_fhr_stats != 0U);
	HWA_ASSERT(filter_id < HWA_RXDATA_FHR_FILTERS_MAX);

	if (!dev->up) {
		return 0;
	}

	fhr_stats_addr = HWA_TABLE_ADDR(hwa_rxdata_fhr_stats_entry_t,
	                                    rxdata->mac_fhr_stats, filter_id);

	HWA_RD_MEM32(HWA2a, hwa_rxdata_fhr_stats_entry_t,
	             fhr_stats_addr, &filter_hits);

	HWA_TRACE(("%s fhr filter<%u> hits<%u>\n", HWA2a, filter_id, filter_hits));

	return filter_hits;

} // hwa_rxdata_fhr_hits_get

void // Clear the statistics for a filter or ALL filters i.e filter_id = ~0U
hwa_rxdata_fhr_hits_clr(uint32 filter_id)
{
	uint32 v32;
	hwa_dev_t *dev;

	HWA_FTRACE(HWA2a);

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit

	HWA_ASSERT(dev->mac_regs != (hc_hin_regs_t*)NULL);

	// Register is shared with ENables, so use read-modify-write
	v32 = HWA_RD_REG_ADDR(HWA2a, &dev->mac_regs->rxhwactrl);
	v32 &= _RXHWACTRL_GLOBALFILTEREN_MASK | _RXHWACTRL_PKTCOMPEN_MASK;

	if (filter_id == ~0U) {
		v32 |= BCM_SBIT(_RXHWACTRL_CLRALLFILTERSTAT);
	} else {
		v32 |= BCM_SBF(filter_id, _RXHWACTRL_ID) |
		      BCM_SBIT(_RXHWACTRL_CLRAFILTERSTAT);
	}

	HWA_WR_REG_ADDR(HWA2a, &dev->mac_regs->rxhwactrl, v32);

} // hwa_rxdata_fhr_hits_clr

#if defined(BCMDBG) || defined(HWA_DUMP)
// HWA2a RxData debug support

static void hwa_rxdata_filter_dump(hwa_rxdata_fhr_entry_t *filter, struct bcmstrbuf *b);

static void // HWA2a RxData dump a FHR filter
hwa_rxdata_filter_dump(hwa_rxdata_fhr_entry_t *filter, struct bcmstrbuf *b)
{
	int id, byte;
	hwa_rxdata_fhr_param_t *param;
	HWA_ASSERT(filter != (hwa_rxdata_fhr_entry_t*)NULL);

	// Filter config
	HWA_BPRINT(b, "+ Filter<%u> polarity<%u> params<%u>\n",
		filter->config.id, filter->config.polarity, filter->config.param_count+1);

	for (id = 0; id <= filter->config.param_count; id++) {
		param = &filter->params[id];
		// Param config
		HWA_BPRINT(b, "+    Param<%u> polarity<%u> offset<%u>: bitmask[",
			id, param->config.polarity, param->config.offset);
		for (byte = 0; byte < HWA_RXDATA_FHR_PATTERN_BYTES; byte++)
			HWA_BPRINT(b, "%02X", param->bitmask[byte]); // param bitmask bytes
		HWA_BPRINT(b, "] pattern[");
		for (byte = 0; byte < HWA_RXDATA_FHR_PATTERN_BYTES; byte++)
			HWA_BPRINT(b, "%02X", param->pattern[byte]); // param pattern bytes
		HWA_BPRINT(b, "]\n");
	}

	HWA_BPRINT(b, "+    Filter_hits<%u>\n", hwa_rxdata_fhr_hits_get(filter->config.id));

} // hwa_rxdata_filter_dump

void // HWA2a RxData: dump FHR table
hwa_rxdata_fhr_dump(hwa_rxdata_t *rxdata, struct bcmstrbuf *b, bool verbose)
{
	int id;
	hwa_rxdata_fhr_entry_t *filter;

	HWA_ASSERT(rxdata != (hwa_rxdata_t*)NULL);

	HWA_BPRINT(b, "%s FHR rxfilteren<0x%08x> pktfetch<0x%08x> l2filter<0x%08x>\n",
		HWA2a, rxdata->rxfilteren, rxdata->fhr_pktfetch, rxdata->fhr_l2filter);

	if (rxdata->fhr_build != (hwa_rxdata_fhr_filter_t*)NULL) {
		HWA_BPRINT(b, "+ fhr_build id<%u> param_count<%u>\n",
			rxdata->fhr_build->filter.config.id, rxdata->param_count);
		hwa_rxdata_filter_dump(&rxdata->fhr_build->filter, b);
	}

	for (id = 0; id < HWA_RXDATA_FHR_FILTERS_SW; id++) {

		filter = &rxdata->fhr[id].filter;
		if (filter->config.type != HWA_RXDATA_FHR_FILTER_DISABLED)
			hwa_rxdata_filter_dump(filter, b);
	}

	HWA_STATS_EXPR(HWA_BPRINT(b, "+ FHR stats ins<%u> del<%u> err<%u>\n",
		rxdata->fhr_ins_cnt, rxdata->fhr_del_cnt, rxdata->fhr_err_cnt));

} // hwa_rxdata_fhr_dump

void // HWA2a RxData: debug dump
hwa_rxdata_dump(hwa_rxdata_t *rxdata, struct bcmstrbuf *b, bool verbose)
{
	HWA_BPRINT(b, "%s dump<%p>\n", HWA2a, rxdata);

	if (rxdata == (hwa_rxdata_t*)NULL)
		return;

	HWA_BPRINT(b, "%s mac_fhr_base<0x%08x> mac_fhr_stats<0x%08x>\n",
		HWA2a, rxdata->mac_fhr_base, rxdata->mac_fhr_stats);

	hwa_rxdata_fhr_dump(rxdata, b, verbose);

} // hwa_rxdata_dump

#endif /* BCMDBG */

#endif /* HWA_RXDATA_BUILD */

#ifdef HWA_TXFIFO_BUILD
/*
 * -----------------------------------------------------------------------------
 * Section: HWA3b TxFifo block bringup: attach, detach, and init phases
 * -----------------------------------------------------------------------------
 */

// Free pkts of TxFIFOs shadow context
static void hwa_txfifo_shadow_reclaim(hwa_dev_t *dev, uint32 core, uint32 fifo_idx);
#ifdef HWA_DUMP
static void _hwa_txfifo_dump_pkt(void *pkt, struct bcmstrbuf *b, const char *title,
	bool one_shot, bool raw);
#endif

void // HWA3b: Cleanup/Free resources used by TxFifo block
BCMATTACHFN(hwa_txfifo_detach)(hwa_txfifo_t *txfifo)
{
	void *memory;
	uint32 mem_sz;
	hwa_dev_t *dev;
	wlc_info_t *wlc;

	HWA_FTRACE(HWA3b);

	if (txfifo == (hwa_txfifo_t*)NULL)
		return;

	// Audit pre-conditions
	dev = HWA_DEV(txfifo);

	wlc = (wlc_info_t *)dev->wlc;

#if !defined(HWA_PKTPGR_BUILD)
	// HWA3b TxFifo PktChain Ring: free memory and reset ring
	if (txfifo->pktchain_ring.memory != (void*)NULL) {
		memory = txfifo->pktchain_ring.memory;
		mem_sz = txfifo->pktchain_ring.depth * txfifo->pktchain_fmt;
		HWA_TRACE(("%s pktchain_ring -memory[%p:%u]\n", HWA3b, memory, mem_sz));
		MFREE(dev->osh, memory, mem_sz);
		hwa_ring_fini(&txfifo->pktchain_ring);
		txfifo->pktchain_ring.memory = (void*)NULL;
	}
#endif /* !HWA_PKTPGR_BUILD */

	// HWA3b TxFifo shadow context: free memory
	if (txfifo->txfifo_shadow != (void*)NULL) {
		memory = txfifo->txfifo_shadow;
		if (sizeof(memory) == sizeof(int)) {
			mem_sz = HWA_TX_FIFOS * sizeof(hwa_txfifo_shadow32_t);
		} else { // NIC 64bit
			mem_sz = HWA_TX_FIFOS * sizeof(hwa_txfifo_shadow64_t);
		}
		HWA_TRACE(("%s txfifos shadow context -memory[%p:%u]\n", HWA3b, memory, mem_sz));
		MFREE(dev->osh, memory, mem_sz);
		txfifo->txfifo_shadow = (void*)NULL;
	}

	if (wlc->hwa_txpkt) {
		MFREE(dev->osh, wlc->hwa_txpkt, sizeof(*wlc->hwa_txpkt));
		wlc->hwa_txpkt = NULL;
	}

} // hwa_txfifo_detach

hwa_txfifo_t * // HWA3b: Allocate resources for TxFifo block
BCMATTACHFN(hwa_txfifo_attach)(hwa_dev_t *dev)
{
	void *memory;
	hwa_regs_t *regs;
	hwa_txfifo_t *txfifo;
	wlc_info_t *wlc;
	uint32 mem_sz;
	NO_HWA_PKTPGR_EXPR(uint32 depth);
	NO_HWA_PKTPGR_EXPR(hwa_txdma_regs_t *txdma);

	HWA_FTRACE(HWA3b);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	regs = dev->regs;
	txfifo = &dev->txfifo;
	wlc = (wlc_info_t *)dev->wlc;
	NO_HWA_PKTPGR_EXPR(txdma = &regs->txdma);

	// Verify HWA3b block's structures
	HWA_ASSERT(HWA_TX_CORES == 1); // XXX HWA3b in 43684 supports only one core
	HWA_ASSERT(sizeof(hwa_txfifo_pktchain32_t) == HWA_TXFIFO_PKTCHAIN32_BYTES);
	HWA_ASSERT(sizeof(hwa_txfifo_pktchain64_t) == HWA_TXFIFO_PKTCHAIN64_BYTES);
	HWA_ASSERT(sizeof(hwa_txfifo_ovflwqctx_t) == HWA_TXFIFO_OVFLWQCTX_BYTES);
	HWA_ASSERT(sizeof(hwa_txfifo_fifoctx_t) == HWA_TXFIFO_FIFOCTX_BYTES);

	// Confirm HWA TxFIFO Capabilities against 43684 Generic
	{
		uint32 cap1, cap2;
		BCM_REFERENCE(cap1); BCM_REFERENCE(cap2);
		cap1 = HWA_RD_REG_NAME(HWA3b, regs, top, hwahwcap1);
		cap2 = HWA_RD_REG_NAME(HWA3b, regs, top, hwahwcap2);
		HWA_ASSERT(BCM_GBF(cap1, HWA_TOP_HWAHWCAP1_MAXSEGCNT3A3B) ==
			HWA_TX_DATABUF_SEGCNT_MAX);
		HWA_ASSERT(BCM_GBF(cap2, HWA_TOP_HWAHWCAP2_NUMMACTXFIFOQ) ==
			HWA_TX_FIFOS);
		HWA_ASSERT(BCM_GBIT(cap2, HWA_TOP_HWAHWCAP2_TOP2REGS_RINFO_CAP) ==
			HWA_TXFIFO_CHICKEN_FEATURE);
		HWA_ASSERT(BCM_GBIT(cap2, HWA_TOP_HWAHWCAP2_TOP2REGS_PKTINFO_CAP) ==
			HWA_TXFIFO_CHICKEN_FEATURE);
		HWA_ASSERT(BCM_GBIT(cap2, HWA_TOP_HWAHWCAP2_TOP2REGS_CACHEINFO_CAP) ==
			HWA_TXFIFO_CHICKEN_FEATURE);
		HWA_ASSERT(BCM_GBIT(cap2, HWA_TOP_HWAHWCAP2_TOP2REGS_OVFLOWQ_PRESENT) ==
			(HWA_OVFLWQ_MAX != 0));
	}

	// FD always uses a 32b packet chain. NIC may use 32bit or 64bit.
	if (sizeof(memory) == sizeof(int)) {
		txfifo->pktchain_fmt = sizeof(hwa_txfifo_pktchain32_t);
	} else { // NIC 64bit
		txfifo->pktchain_fmt = sizeof(hwa_txfifo_pktchain64_t);
	}

#if !defined(HWA_PKTPGR_BUILD)
	// Allocate and initialize S2H pktchain xmit request interface
	depth = HWA_TXFIFO_PKTCHAIN_RING_DEPTH;
	mem_sz = depth * txfifo->pktchain_fmt;
	// hwa_regs::txdma::sw2hwa_tx_pkt_chain_q_base_addr_h
	if ((memory = MALLOCZ(dev->osh, mem_sz)) == NULL) {
		HWA_ERROR(("%s pktchain_ring malloc size<%u> failure\n",
			HWA3b, mem_sz));
		HWA_ASSERT(memory != (void*)NULL);
		goto failure;
	}
	HWA_TRACE(("%s pktchain_ring +memory[%p,%u]\n", HWA3b, memory, mem_sz));
	hwa_ring_init(&txfifo->pktchain_ring, "CHN",
		HWA_TXFIFO_ID, HWA_RING_S2H, HWA_TXFIFO_PKTCHAIN_S2H_RINGNUM,
		depth, memory, &txdma->sw2hwa_tx_pkt_chain_q_wr_index,
		&txdma->sw2hwa_tx_pkt_chain_q_rd_index);
#endif /* !HWA_PKTPGR_BUILD */

	// Initialize the Overflow Queue AXI memory address
	txfifo->ovflwq_addr = hwa_axi_addr(dev, HWA_AXI_TXFIFO_OVFLWQS);

	// Initialize the TxFIFO and AQM FIFO AXI memory address
	txfifo->txfifo_addr = hwa_axi_addr(dev, HWA_AXI_TXFIFO_TXFIFOS);

	txfifo->fifo_total = HWA_TX_FIFOS;

	// Allocate and initialize TxFIFOs shadow context
	if (sizeof(memory) == sizeof(int)) {
		mem_sz = HWA_TX_FIFOS * sizeof(hwa_txfifo_shadow32_t);
	} else { // NIC 64bit
		mem_sz = HWA_TX_FIFOS * sizeof(hwa_txfifo_shadow64_t);
	}
	if ((memory = MALLOCZ(dev->osh, mem_sz)) == NULL) {
		HWA_ERROR(("%s txfifos shadow context malloc size<%u> failure\n",
			HWA3b, mem_sz));
		HWA_ASSERT(memory != (void*)NULL);
		goto failure;
	}
	txfifo->txfifo_shadow = memory;
	HWA_TRACE(("%s txfifos shadow +memory[%p,%u]\n", HWA3b, memory, mem_sz));

	wlc->hwa_txpkt = (wlc_hwa_txpkt_t*)MALLOCZ(dev->osh, sizeof(*wlc->hwa_txpkt));
	if (wlc->hwa_txpkt == NULL) {
		HWA_ERROR(("%s hwa_txpkt alloc failed.\n", HWA3b));
		goto failure;
	}

	// XXX, WAR for stopping OvfQ not idle.
	HWA_PKTPGR_EXPR(wlc->hwa_txpkt->mpdus_per_req = HWA_TXFIFO_AQM_DESC_FREE_SPACE);

	return txfifo;

failure:
	hwa_txfifo_detach(txfifo);
	HWA_WARN(("%s attach failure\n", HWA3b));

	return ((hwa_txfifo_t*)NULL);

} // hwa_txfifo_attach

void // hwa_txfifo_init
hwa_txfifo_init(hwa_txfifo_t *txfifo)
{
	uint32 u32, i, hi32, addr64;
	hwa_dev_t *dev;
	hwa_regs_t *regs;
	NO_HWA_PKTPGR_EXPR(hwa_ring_t *pktchain_ring); // S2H pktchain ring context
	uint16 u16;

	HWA_FTRACE(HWA3b);

	// Audit pre-conditions
	dev = HWA_DEV(txfifo);

	// Setup locals
	regs = dev->regs;
	NO_HWA_PKTPGR_EXPR(pktchain_ring = &txfifo->pktchain_ring);

	// Consume all PGO resp ring
	HWA_PKTPGR_EXPR(hwa_pktpgr_response(dev, hwa_pktpgr_pageout_rsp_ring,
		HWA_PKTPGR_PAGEOUT_CALLBACK));

	for (i = 0; i < HWA_TX_FIFOS; i++) {
		if (isset(txfifo->fifo_enab, i)) {
			// Use register interface to program hwa_txfifo_fifoctx_t in AXI memory
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_index, i);
			// PKT FIFO context: hwa_txfifo_fifoctx::pkt_fifo
			// hwa_txfifo_fifoctx::pkt_fifo.base.loaddr
			u32 = txfifo->fifo_base[i].loaddr;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_base_addrlo, u32);
			// hwa_txfifo_fifoctx::pkt_fifo.base.hiaddr
			u32 = txfifo->fifo_base[i].hiaddr;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_base_addrhi, u32);
			// hwa_txfifo_fifoctx::pkt_fifo.curr_ptr
			u32 = txfifo->fifo_base[i].loaddr & 0xFFFF;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_rd_index, u32);
			// hwa_txfifo_fifoctx::pkt_fifo.last_ptr
			u32 = txfifo->fifo_base[i].loaddr & 0xFFFF;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_wr_index, u32);
			// hwa_txfifo_fifoctx::pkt_fifo.attrib
			u32 = (0U
				| BCM_SBIT(HWA_TXFIFO_FIFO_COHERENCY)
				| 0U);
			// the coherency, notpcie in fifo_attrib is specific HWA3B to generate DD
			// for payload in DDR.  The DD for head in SysMem is from txdma_flags in
			// wlc_bmac_hwa_convert_to_3bswpkt
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_attrib, u32);
			// hwa_txfifo_fifoctx::pkt_fifo.depth
			u32 = txfifo->fifo_depth[i];
			HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_depth, u32);
			// AQM FIFO context: hwa_txfifo_fifoctx::aqm_fifo
			// hwa_txfifo_fifoctx::aqm_fifo.base.loaddr
			u32 = txfifo->aqm_fifo_base[i].loaddr;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_base_addrlo, u32);
			// hwa_txfifo_fifoctx::aqm_fifo.base.hiaddr
			u32 = txfifo->aqm_fifo_base[i].hiaddr;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_base_addrhi, u32);
			// hwa_txfifo_fifoctx::aqm_fifo.curr_ptr
			u32 = txfifo->aqm_fifo_base[i].loaddr & 0xFFFF;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_rd_index, u32);
			// hwa_txfifo_fifoctx::aqm_fifo.last_ptr
			u32 = txfifo->aqm_fifo_base[i].loaddr & 0xFFFF;
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_wr_index, u32);
			// hwa_txfifo_fifoctx::aqm_fifo.attrib
			if (txfifo->hme_macifs[i]) {
				u32 = 0; // no coherency, notpcie if hme_macifs is set
			} else {
				u32 = (0U
					| BCM_SBIT(HWA_TXFIFO_FIFO_COHERENCY)
					| BCM_SBIT(HWA_TXFIFO_FIFO_NOTPCIE)
					| 0U);
			}
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_attrib, u32);
			// hwa_txfifo_fifoctx::aqm_fifo.depth
			u32 = txfifo->aqm_fifo_depth[i];
			HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_depth, u32);

			HWA_PKTPGR_EXPR({
				if (!isset(txfifo->shadow_inited, i)) {
					hwa_txfifo_shadow32_t *shadow32;
					setbit(txfifo->shadow_inited, i);
					// Read txfifo shadow packet count
					shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
					shadow32[i].hw.txd_avail = txfifo->fifo_depth[i] -
						HWA_PKTPGR_MACFIFO_EMPTY_CNT -
						HWA_TXFIFO_TXDESC_MAX_PER_MPDU;
					shadow32[i].hw.aqm_avail = txfifo->aqm_fifo_depth[i] -
						HWA_PKTPGR_MACAQM_EMPTY_CNT - 1;
				}
			});
		}
	}

#if !defined(HWA_PKTPGR_BUILD)
	u32 = HWA_PTR2UINT(pktchain_ring->memory);
	HWA_WR_REG_NAME(HWA3b, regs, txdma,
		sw2hwa_tx_pkt_chain_q_base_addr_l, u32);

	u32 = BCM_SBF(HWA_TXFIFO_PKTCHAIN_RING_DEPTH,
			HWA_TXDMA_SW2HWA_TX_PKT_CHAIN_Q_CTRL_PKTCHAINQDEPTH);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, sw2hwa_tx_pkt_chain_q_ctrl, u32);

	u32 = HWA_PTR2HIADDR(pktchain_ring->memory);
	HWA_WR_REG_NAME(HWA3b, regs, txdma,
		sw2hwa_tx_pkt_chain_q_base_addr_h, u32);
#endif /* !HWA_PKTPGR_BUILD */

	// Settings "NotPCIE, Coherent and AddrExt" for misc HW DMA transactions
	u32 = (0U
	// PCIE Source or Destination: NIC mode MAC Ifs may be placed in SysMem
	// FIXME: Is this used for TxFIFO/AQM FIFO or S2H PktChainIf
	// FIXME: NIC mode, would like to have option to place FIFOs in SysMem,
	// FIXME: while S2H PktChainIf in Host mem.
	/* XXX HW only use TCM setting.  HWA read 3B pktchain, swpkt and write
	 * txd/aqm 16B entry.  Since HWA DMA doesn't care coherent, notpcie so
	 * we don't need to change it further.
	 */
	| BCM_SBF(dev->macif_placement, // NIC mode S2H PktChainIf is over PCIe
	          HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMAPCIEDESCTEMPLATENOTPCIE)
	| BCM_SBF(dev->macif_coherency, // NIC mode S2H PktChainIf is host_coh
		HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMAPCIEDESCTEMPLATECOHERENT)
	| BCM_SBF(0U,
		HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMAPCIEDESCTEMPLATEADDREXT)
	// TCM Source or Destination: NotPcie = 1, Coh = 1, AddrExt = 0b00
	| BCM_SBIT(HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMATCMDESCTEMPLATENOTPCIE)
	| BCM_SBIT(HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMATCMDESCTEMPLATECOHERENT)
	| BCM_SBF(0U, HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMATCMDESCTEMPLATEADDREXT)
	// HWA local memory as source or destination: NotPcie = 1
	| BCM_SBIT(HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMAHWADESCTEMPLATENOTPCIE)
	// MAC TxFIFO WR update
#ifdef BCMPCIEDEV
	| BCM_SBIT(HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_NONDMAHWADESCTEMPLATECOHERENT)
#else
	| BCM_SBIT(dev->host_coherency,
		HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_NONDMAHWADESCTEMPLATECOHERENT)
#endif
	HWA_PKTPGR_EXPR(
		| BCM_SBIT(HWA_TXDMA_DMA_DESC_TEMPLATE_TXDMA_DMATCMDESCTEMPLATEWCAQM))
	| 0U);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, dma_desc_template_txdma, u32);

	// Setup HWA_TXDMA2_CFG1
#ifdef BCMPCIEDEV
	// Dongle uses 32bit pointers for head tail, with hi32 = 0U
	hi32 = 0U;
	addr64 = 0U;
#else
	hi32 = dev->host_physaddrhi;
	addr64 = dev->host_addressing;
#endif /* ! BCMPCIEDEV */
	u32 = (0U
		| BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_PKTCHAIN_NEW_FORMAT)
		| BCM_SBF(addr64, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_PKTCHAIN_64BITADDRESS)
		| BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_USE_OVFLOWQ)
		// STOP_OVFLOWQ always true in PKTPGR mode.
	HWA_PKTPGR_EXPR(
		| BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_STOP_OVFLOWQ))
		// BCM_CBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_NON_AQM_CTDMA_MODE)
		// XXX CRWLHWA-446,  0: with direct path; 1 : with APB interface.
		| BCM_SBF(dev->txfifo_apb, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_LAST_PTR_UPDATE)
		// XXX CRWLHWA-439, 0: sw; 1: hw
		| BCM_SBF(dev->txfifo_hwupdnext, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_HW_UPDATE_PKTNXT)
		// SW check TXFIFO context with signal burst. 0:muliti 1:signal.
		| BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_CHECK_TXFIFO_CONTEXT)
	HWA_PKTPGR_EXPR(
		| BCM_SBF(dev->flexible_txlfrag,
			HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_AQM_AGG_UPDATE_PER_POUT_EN)
		| BCM_SBF(dev->flexible_txlfrag, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_FLEXIBLE_LFRAG))
		| 0U);
	if (HWAREV_GE(dev->corerev, 131)) {
		uint8 new_aqmdscr_fmt = 0;

		if (((wlc_info_t *)dev->wlc)->short_aqmdd) {
			new_aqmdscr_fmt = 1;
			HWA_ERROR(("%s: Use short aqm descriptor format\n", HWA3b));
		}
		u32 |= (0U
			// 43684 only support 0: Read AQM descriptor back.
			| BCM_SBF(dev->txd_cal_mode,
				HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_CURRENTIDX_CALCULATION)
			// wait for delay to do TXDMA currentIdx calculation
			| BCM_SBF(4,
				HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_CURRENTIDX_CAL_WTIME)
			// new_aqmdscr_fmt, 0: 16B AQM descriptor, 1: 8B AQM descriptor.
			| BCM_SBF(new_aqmdscr_fmt, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_AQM_8B_SUPPORT)
			| 0U);
	}
	HWA_WR_REG_NAME(HWA3b, regs, txdma, hwa_txdma2_cfg1, u32);

	// HWA2.1: Setup HWA_TXDMA2_CFG2
	// We must to set txdma_aggr_aqm_descriptor_enable when txfifo_apb is 0,
	// otherwise we will hit frameid mismatch type3.
	// RTL simulation suggest to keep txdma_aggr_aqm_descriptor_enable and
	// txdma_aggr_aqm_descriptor_threshold as default 1 and 3.

	// XXX: HWA2.1: HWA_TXFIFO_AGGR_AQM_DESC_THRESHOLD has bug that will cause
	// txs->frameid 0x8ac1 seq 277 txh->TxFrameID 0x8a41 seq 276 when it
	// configure more than 0. (0 imply 1 aqm).  So, keep it update per one aqm.
	// Enlarge HWA_TXDMA_HWA_TXDMA2_CFG3_AQM_DESC_FREE_SPACE
	// doesn't help.
	u32 = (0U
#if defined(HWA_PKTPGR_BUILD)
		// XXX, WAR for stopping OvfQ not idle,  (15+1) mpdu
		| BCM_SBF(HWA_TXFIFO_AGGR_AQM_DESC_THRESHOLD,
			HWA_TXDMA_HWA_TXDMA2_CFG2_TXDMA_AGGR_AQM_DESCRIPTOR_THRESHOLD)
		| BCM_SBF(HWA_TXFIFO_AGGR_AQM_DESC_THRESHOLD2,
			HWA_TXDMA_HWA_TXDMA2_CFG2_TXDMA_AGGR_AQM_DESCRIPTOR_THRESHOLD2)
#else
		| BCM_SBF(0,
			HWA_TXDMA_HWA_TXDMA2_CFG2_TXDMA_AGGR_AQM_DESCRIPTOR_THRESHOLD)
#endif
		| BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG2_TXDMA_AGGR_AQM_DESCRIPTOR_ENABLE)
		| BCM_SBF(HWA_TXFIFO_PKTCNT_THRESHOLD,
			HWA_TXDMA_HWA_TXDMA2_CFG2_TXDMA_SWTXPKT_CNT_REACH)
		| 0U);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, hwa_txdma2_cfg2, u32);

	// Setup HWA_TXDMA2_CFG3
	u32 = HWA_TXFIFO_LIMIT_THRESHOLD;
	// aqm_desc_free_space
	u32 |= (BCM_SBF(HWA_TXFIFO_AQM_DESC_FREE_SPACE,
		HWA_TXDMA_HWA_TXDMA2_CFG3_AQM_DESC_FREE_SPACE));
	HWA_WR_REG_NAME(HWA3b, regs, txdma, hwa_txdma2_cfg3, u32);
	// XXX, must be <= 32, HW limition
	HWA_PKTPGR_EXPR(STATIC_ASSERT(HWA_TXFIFO_AQM_DESC_FREE_SPACE <= 32));

	// Setup HWA_TXDMA2_CFG4 that controls descriptor generation
	u32 = BCM_SBF(HWA_TXFIFO_EMPTY_THRESHOLD,
		HWA_TXDMA_HWA_TXDMA2_CFG4_TXDMA_EMPTY_CNT_REACH);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, hwa_txdma2_cfg4, u32);

	// pktchain head, tail and swtx::pkt_next all point to packets and
	// share the same hi32 address. reg sw_tx_pkt_nxt_h is redundant. FIXME
	HWA_WR_REG_NAME(HWA3b, regs, txdma, sw_tx_pkt_nxt_h, hi32);

#if defined(HWA_PKTPGR_BUILD)
	// WDMA is for "dest" SysMem, RDMA is for "src" DDR
	u32 = HWA_RD_REG_NAME(HWA3b, regs, txdma, pp_pagein_cfg);
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEIN_CFG_PP_PAGEIN_RDMA_COHERENT);
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEIN_CFG_PP_PAGEIN_RDMA_NOTPCIE);
	// Not sure if we can turn it on
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEIN_CFG_PP_PAGEIN_WDMA_WCPDESC);
	u32 = u32
	    | BCM_SBIT(HWA_TXDMA_PP_PAGEIN_CFG_PP_PAGEIN_WDMA_COHERENT)
	    | BCM_SBIT(HWA_TXDMA_PP_PAGEIN_CFG_PP_PAGEIN_WDMA_NOTPCIE)
	    | 0U;
	// Set TxStatus pagein threshold. Same as pager::pp_pagein_req_ddbmth
	u32 = BCM_CBF(u32, HWA_TXDMA_PP_PAGEIN_CFG_PP_DDBM_AVAILCNT);
	u32 |= BCM_SBF(HWA_PKTPGR_PAGEIN_REQ_DDBMTH,
		HWA_TXDMA_PP_PAGEIN_CFG_PP_DDBM_AVAILCNT);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, pp_pagein_cfg, u32);

	// WDMA is for "dest" DDR, RDMA is for "src" SysMEM
	u32 = HWA_RD_REG_NAME(HWA3b, regs, txdma, pp_pageout_cfg);
	//Not sure if we can turn it on
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_PAGEOUT_WDMA_WCPDESC);
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_PAGEOUT_WDMA_COHERENT);
	u32 = BCM_CBIT(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_PAGEOUT_WDMA_NOTPCIE);
	u32 = u32
	    | BCM_SBIT(HWA_TXDMA_PP_PAGEOUT_CFG_PP_PAGEOUT_RDMA_COHERENT)
	    | BCM_SBIT(HWA_TXDMA_PP_PAGEOUT_CFG_PP_PAGEOUT_RDMA_NOTPCIE)
	    | 0U;
	u32 = BCM_CBF(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_HDBM_AVAILCNT)
		| BCM_SBF(HWA_PKTPGR_HDBM_AVAILCNT,
			HWA_TXDMA_PP_PAGEOUT_CFG_PP_HDBM_AVAILCNT);
	u32 = BCM_CBF(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_MACFIFO_EMPTY_CNT)
		| BCM_SBF(HWA_PKTPGR_MACFIFO_EMPTY_CNT,
			HWA_TXDMA_PP_PAGEOUT_CFG_PP_MACFIFO_EMPTY_CNT);
	u32 = BCM_CBF(u32, HWA_TXDMA_PP_PAGEOUT_CFG_PP_MACAQM_EMPTY_CNT)
		| BCM_SBF(HWA_PKTPGR_MACAQM_EMPTY_CNT,
			HWA_TXDMA_PP_PAGEOUT_CFG_PP_MACAQM_EMPTY_CNT);
	HWA_WR_REG_NAME(HWA3b, regs, txdma, pp_pageout_cfg, u32);
#endif /* HWA_PKTPGR_BUILD */

	// XXX CRWLHWA-446
	// Enable Req-Ack based MAC_HWA i/f is enabled for tx Dma last index update.
	u16 = 0;

	if (dev->txfifo_apb == 0) {
		u16 |= BCM_SBIT(_HWA_MACIF_CTL_TXDMAEN);
	}
	HWA_UPD_REG16_ADDR(HWA3b, &dev->mac_regs->hwa_macif_ctl, u16, _HWA_MACIF_CTL_TXDMAEN);

	return;
} // hwa_txfifo_init

void // HWA3b: Deinit TxFifo block
hwa_txfifo_deinit(hwa_txfifo_t *txfifo)
{
	hwa_dev_t *dev;

	HWA_FTRACE(HWA3b);

	// Audit pre-conditions
	dev = HWA_DEV(txfifo);

	//Disable Req-Ack based MAC_HWA i/f is enabled for tx Dma last index update.
	if (dev->txfifo_apb == 0) {
		HWA_UPD_REG16_ADDR(HWA3b, &dev->mac_regs->hwa_macif_ctl, 0, _HWA_MACIF_CTL_TXDMAEN);
	}
}

int // Configure a TxFIFO's pkt and aqm ring context in HWA AXI memory
hwa_txfifo_config(struct hwa_dev *dev, uint32 core, uint32 fifo_idx,
	dma64addr_t fifo_base, uint32 fifo_depth, dma64addr_t aqm_fifo_base,
	uint32 aqm_fifo_depth, bool hme_macifs)
{
	hwa_txfifo_t *txfifo;

	HWA_TRACE(("%s config core<%u> FIFO %u base<0x%08x,0x%08x> depth<%u>"
		" AQM base<0x%08x,0x%08x> depth<%u>\n",
		HWA3b, core, fifo_idx, fifo_base.hiaddr, fifo_base.loaddr, fifo_depth,
		aqm_fifo_base.hiaddr, aqm_fifo_base.loaddr, aqm_fifo_depth));

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	txfifo = &dev->txfifo;

	// Save settings in local state
	txfifo->fifo_base[fifo_idx] = fifo_base;
	txfifo->fifo_depth[fifo_idx] = fifo_depth;
	txfifo->aqm_fifo_base[fifo_idx] = aqm_fifo_base;
	txfifo->aqm_fifo_depth[fifo_idx] = aqm_fifo_depth;
	txfifo->hme_macifs[fifo_idx] = hme_macifs;

	setbit(txfifo->fifo_enab, fifo_idx);

	return HWA_SUCCESS;

} // hwa_txfifo_config

#if defined(HWA_PKTPGR_BUILD)

void // Prepare to stop 3b generate new Tx AQM before MAC suspend.
hwa_txfifo_disable_prep(struct hwa_dev *dev, uint32 core)
{
	hwa_regs_t *regs;
	hwa_pager_regs_t *pager_regs;
	uint32 u32, state, loop_count;

	if (!dev)
		return;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);

	// Setup locals
	regs = dev->regs;
	pager_regs = &regs->pager;

	// Pageout ring stop bit on and wait for idle
	// Pageout ring stop also means HWA stop MAC AQM/TX DD generation.
	u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pageout_req_ring.cfg);
	u32 |= BCM_SBIT(HWA_PAGER_PP_PAGEOUT_REQ_RING_CFG_PPOUTREQSTOP);
	HWA_WR_REG_ADDR(HWApp, &pager_regs->pageout_req_ring.cfg, u32);
	// Poll PAGEOUT_REQ_RING_DEBUG::STATE == 0
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_ADDR(HWApp, &pager_regs->pageout_req_ring.debug);
		state = BCM_GBF(u32, HWA_PAGER_PP_PAGEOUT_REQ_RING_DEBUG_PPOUTREQFSM);
		HWA_TRACE(("%s Polling PAGEOUT_REQ_RING_DEBUG::STATE <%d>\n",
			__FUNCTION__, state));
	} while (state != 0 && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s PAGEOUT_REQ_RING_DEBUG::STATE is not idle <%d>\n",
			__FUNCTION__, state));
	}

	HWA_INFO(("%s: Stop PageOut Req\n", __FUNCTION__));

	// NOTE:
	//     HWA2.1:  1. Consume all 3b pktchain ring to OvfQ.  2. Stop ovflowq
	//     HWA2.2
	//     1. Some packets are stay in hwa_pktpgr_pageout_req_ring. For any fifo id.
	//          hwa_pktpgr_pageout_req_ring is like OvfQ in HWA2.1 but it's common Q like.
	//     2. Some packets are stay in HWA shadow OvfQ. Per fifo id.
	//          We don't have the backup Q like HWA2.1 ovfQ in HWA anymore in HWA2.2,
	//          packets are moved to D11 MAC FIFO directly from hwa_pktpgr_pageout_req_ring.
}

void // Enable or Disable 3b block
hwa_txfifo_enable(struct hwa_dev *dev, uint32 core, bool enable)
{
	uint32 u32;

	HWA_FTRACE(HWA3b);

	if (!dev)
		return;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);

	// Handle enable case and return.
	if (enable) {
		// STOP_OVFLOWQ is always true in PKTPGR mode.

		// Clear PageOut req ring stop bit.
		u32 = HWA_RD_REG_ADDR(HWApp, &dev->regs->pager.pageout_req_ring.cfg);
		u32 = BCM_CBIT(u32, HWA_PAGER_PP_PAGEOUT_REQ_RING_CFG_PPOUTREQSTOP);
		HWA_WR_REG_ADDR(HWApp, &dev->regs->pager.pageout_req_ring.cfg, u32);
		HWA_INFO(("%s: Start PageOut Req\n", __FUNCTION__));

		return;
	}

	// In PKTPGR mode we cannot stop 3B.  Because 3B need to
	// process OvfQ which is HW Tx shadow.
}

#else

void // Prepare to disable 3b block before MAC suspend.
hwa_txfifo_disable_prep(struct hwa_dev *dev, uint32 core)
{
	hwa_txfifo_t *txfifo;
	uint32 u32, idle, loop_count;
	uint32 dma_desc_busy;

	HWA_FTRACE(HWA3b);

	if (!dev)
		return;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);

	// Setup locals
	txfifo = &dev->txfifo;

	// Before disable HWA_MODULE_TXFIFO, we need to
	// wait until HWA consume all 3b pktchain ring
	loop_count = HWA_FSM_IDLE_POLLLOOP;
	while (!hwa_ring_is_cons_all(&txfifo->pktchain_ring)) {
		HWA_TRACE(("%s HWA consuming 3b pktchain ring\n", __FUNCTION__));
		OSL_DELAY(1);
		if (--loop_count == 0) {
			HWA_ERROR(("%s Cannot consume 3b pktchain ring\n", __FUNCTION__));
			break;
		}
	}
	// Poll STATE_STS::sw_pkt_curstate <= 1
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, state_sts);
		idle = BCM_GBF(u32, HWA_TXDMA_STATE_STS_SW_PKT_CURSTATE);
		HWA_TRACE(("%s Polling STATE_STS::sw_pkt_curstate <%d>\n",
			__FUNCTION__, idle));
	} while (idle > 1 && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s STATE_STS::sw_pkt_curstate is not idle <%u>\n",
			__FUNCTION__, idle));
	}

	// Set HWA_TXDMA2_CFG1::txdma_stop_ovflowq bit.
	u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, hwa_txdma2_cfg1);
	u32 |= BCM_SBIT(HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_STOP_OVFLOWQ);
	HWA_WR_REG_NAME(HWA3b, dev->regs, txdma, hwa_txdma2_cfg1, u32);

	// Poll STATE_STS2::overflowq_state <= 1
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, state_sts2);
		idle = BCM_GBF(u32, HWA_TXDMA_STATE_STS2_OVERFLOWQ_STATE);
		HWA_TRACE(("%s Polling STATE_STS2::overflowq_state <%d>\n",
			__FUNCTION__, idle));
	} while (idle > 1 && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s STATE_STS2::overflowq_state is not idle <%d>\n",
			__FUNCTION__, idle));
	}

	// Poll STATE_STS::dma_des_curstate and fifo_dma_curstate are both 0
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, state_sts);
		dma_desc_busy = BCM_GBF(u32, HWA_TXDMA_STATE_STS_DMA_DES_CURSTATE);
		dma_desc_busy |= BCM_GBF(u32, HWA_TXDMA_STATE_STS_FIFO_DMA_CURSTATE);
		HWA_TRACE(("%s Polling STATE_STS::dma_desc_busy <%d>\n",
			__FUNCTION__, dma_desc_busy));
	} while (dma_desc_busy && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s STATE_STS::state_sts dma desc is not idle <%u>\n",
			__FUNCTION__, u32));
	}
}

void // Enable or Disable 3b block
hwa_txfifo_enable(struct hwa_dev *dev, uint32 core, bool enable)
{
	uint32 u32, idle, loop_count;
	uint32 dma_desc_busy;

	HWA_FTRACE(HWA3b);

	if (!dev)
		return;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);

	// Handle enable case and return.
	if (enable) {
		// Enable TXFIFO module
		hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_ENABLE, enable);

		// Clear HWA_TXDMA2_CFG1::txdma_stop_ovflowq
		u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, hwa_txdma2_cfg1);
		u32 = BCM_CBIT(u32, HWA_TXDMA_HWA_TXDMA2_CFG1_TXDMA_STOP_OVFLOWQ);
		HWA_WR_REG_NAME(HWA3b, dev->regs, txdma, hwa_txdma2_cfg1, u32);

		return;
	}

	// Poll STATE_STS::dma_des_curstate and fifo_dma_curstate are both 0
	// Check again after MAC suspended.
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		u32 = HWA_RD_REG_NAME(HWA3b, dev->regs, txdma, state_sts);
		dma_desc_busy = BCM_GBF(u32, HWA_TXDMA_STATE_STS_DMA_DES_CURSTATE);
		dma_desc_busy |= BCM_GBF(u32, HWA_TXDMA_STATE_STS_FIFO_DMA_CURSTATE);
		HWA_TRACE(("%s Polling STATE_STS::dma_desc_busy <%d>\n",
			__FUNCTION__, dma_desc_busy));
	} while (dma_desc_busy && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s STATE_STS::state_sts dma desc is not idle <%u>\n",
			__FUNCTION__, u32));
	}

	// Disable TXFIFO module
	hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_ENABLE, enable);

	// Poll module idle bit
	loop_count = HWA_MODULE_IDLE_BURNLOOP;
	do { // Burnloop: allowing HWA3b to complete a previous DMA
		idle = hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_IDLE, TRUE);
	} while (!idle && loop_count--);

	if (!loop_count) {
		HWA_ERROR(("%s %s: Tx block idle<%u> loop<%u>\n", HWA3b, __FUNCTION__,
			idle, HWA_MODULE_IDLE_BURNLOOP));
	}
}
#endif /* HWA_PKTPGR_BUILD */

void // Reset TxFIFO's curr_ptr and last_ptr of pkt and aqm ring context in HWA AXI memory
hwa_txfifo_dma_init(struct hwa_dev *dev, uint32 core, uint32 fifo_idx)
{
	hwa_regs_t *regs;
	hwa_txfifo_t *txfifo;
	uint32 u32;

	HWA_TRACE(("%s %s core<%u> FIFO %u\n", HWA3b, __FUNCTION__, core, fifo_idx));

	if (!dev || !dev->up) {
		return;
	}

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Setup locals
	regs = dev->regs;
	txfifo = &dev->txfifo;

	// Consume all PGO resp ring
	HWA_PKTPGR_EXPR(hwa_pktpgr_response(dev, hwa_pktpgr_pageout_rsp_ring,
		HWA_PKTPGR_PAGEOUT_CALLBACK));

	if (isset(txfifo->fifo_enab, fifo_idx)) {
		// Use register interface to program hwa_txfifo_fifoctx_t in AXI memory
		HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_index, fifo_idx);

		// PKT FIFO context: hwa_txfifo_fifoctx::pkt_fifo
		// hwa_txfifo_fifoctx::pkt_fifo.curr_ptr
		u32 = txfifo->fifo_base[fifo_idx].loaddr & 0xFFFF;
		HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_rd_index, u32);
		// hwa_txfifo_fifoctx::pkt_fifo.last_ptr
		u32 = txfifo->fifo_base[fifo_idx].loaddr & 0xFFFF;
		HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_wr_index, u32);
		// hwa_txfifo_fifoctx::pkt_fifo.depth
		// We have to reconfig depth field to trigger HWA update
		u32 = txfifo->fifo_depth[fifo_idx];
		HWA_WR_REG_NAME(HWA3b, regs, txdma, fifo_depth, u32);

		// AQM FIFO context: hwa_txfifo_fifoctx::aqm_fifo
		// hwa_txfifo_fifoctx::aqm_fifo.curr_ptr
		u32 = txfifo->aqm_fifo_base[fifo_idx].loaddr & 0xFFFF;
		HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_rd_index, u32);
		// hwa_txfifo_fifoctx::aqm_fifo.last_ptr
		u32 = txfifo->aqm_fifo_base[fifo_idx].loaddr & 0xFFFF;
		HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_wr_index, u32);
		// hwa_txfifo_fifoctx::aqm_fifo.depth
		// We have to reconfig depth field to trigger HWA update
		u32 = txfifo->aqm_fifo_depth[fifo_idx];
		HWA_WR_REG_NAME(HWA3b, regs, txdma, aqm_depth, u32);
	}
}

/* XXX NOTE: Don't accress fifoctx through AXI.
 * We may hit following error.
 * AXI timeout  CoreID: 851
 *        errlog: lo 0x28509820, hi 0x00000000, id 0x001b001b, flags 0x00111200, status 0x00000002
 *
 * Use txfifo_shadow as an indicator of active.
 */
uint // Get TxFIFO's active descriptor count
hwa_txfifo_dma_active(struct hwa_dev *dev, uint32 core, uint32 fifo_idx)
{
	uint16 mpdu_count;
	hwa_txfifo_shadow32_t *shadow32;

	if (!dev)
		return 0;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// Read txfifo shadow packet count
	shadow32 = (hwa_txfifo_shadow32_t *)dev->txfifo.txfifo_shadow;
	mpdu_count = shadow32[fifo_idx].mpdu_count;

	// For PKTPGR, both HW and SW mpdu_count
	HWA_PKTPGR_EXPR(mpdu_count += shadow32[fifo_idx].hw.mpdu_count);

	return mpdu_count;
}

#ifdef HWA_PKTPGR_BUILD
// HWA2.2: hwa_txfifo_dma_reclaim
//     1. Some packets are stay in HWA shadow OvfQ. Per fifo id.  (Those packet
//          don't have TxS yet)
//     2. Some packets are stay in hwa_pktpgr_pageout_req_ring. For any fifo id.
//          hwa_pktpgr_pageout_req_ring is like OvfQ in HWA2.1 but it's common Q like.
//
//     So:
//     1. PageIn TxS for those not TxS acked packets to SW shadow.
//     2. Remove packets [use invalid bit] in PageOut Req Q to SW shadow.
//
#endif

void // Reclaim MAC Tx DMA posted packets
hwa_txfifo_dma_reclaim(struct hwa_dev *dev, uint32 core)
{
	uint i;
	int16 pktcnt;
	wlc_info_t *wlc;
	hwa_txfifo_t *txfifo;
	HWA_PKTPGR_EXPR(hwa_txfifo_shadow32_t *shadow32);
	HWA_PKTPGR_EXPR(hwa_txfifo_shadow32_t *myshadow32);
	HWA_PKTPGR_EXPR(uint32 physical_fifo);
	NO_HWA_PKTPGR_EXPR(hwa_ring_t *pktchain_ring); // S2H pktchain ring context

	HWA_FTRACE(HWA3b);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	wlc = (wlc_info_t *)dev->wlc;
	txfifo = &dev->txfifo;
	HWA_PKTPGR_EXPR(shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow);
	NO_HWA_PKTPGR_EXPR(pktchain_ring = &txfifo->pktchain_ring);

	for (i = 0; i < WLC_HW_NFIFO_INUSE(wlc); i++) {

		HWA_PKTPGR_EXPR({
			physical_fifo = WLC_HW_MAP_TXFIFO(wlc, i);

			myshadow32 = &shadow32[physical_fifo];
			if (myshadow32->hw.mpdu_count == 0) {
				continue;
			}

			if (TXPKTPENDGET(wlc, i) == 0) {
				HWA_ERROR(("%s TXPKTPEND[%d] is 0, there are %d pkts in shadow\n",
					HWA3b, i, myshadow32->hw.mpdu_count));
			}

			// Pagein packets in HW shadow to SW shadow.
			while (hwa_pktpgr_txfifo_shadow_reclaim(dev, physical_fifo, FALSE)) {
				/* free any posted tx packets */
				hwa_txfifo_shadow_reclaim(dev, 0, i);
			}

			// Remove specific packets in PageOut Req Q to SW shadow.
			hwa_pktpgr_pageout_ring_shadow_reclaim(dev, physical_fifo);
		});

		/* free any posted tx packets */
		hwa_txfifo_shadow_reclaim(dev, 0, i);

		pktcnt = TXPKTPENDGET(wlc, i);
		if (pktcnt > 0) {
			HWA_ERROR(("%s fifo %d REMAINS %d pkts\n", HWA3b, i, pktcnt));
		}
		HWA_TRACE(("%s pktpend fifo %d cleared\n", HWA3b, i));
	}

	// XXX, CRBCAHWA-581
	// 3B reset impacts 3A/4A HWA internal memory regions, so we need to
	// make sure 3A/4A DMA jobs are done.  Here the MAC has suspended.
	// Wait until 3A finish schedcmd and txfree_ring
	if (HWAREV_IS(dev->corerev, 129)) {
		HWA_TXPOST_EXPR(hwa_txpost_wait_to_finish(&dev->txpost));
	}

	// Disable 3b block, because we don't disable 3b for hwa_pktpgr_txfifo_shadow_reclaim
	HWA_PKTPGR_EXPR(hwa_module_request(dev, HWA_MODULE_TXFIFO,
		HWA_MODULE_ENABLE, FALSE));

	// Reset 3b block
	hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_RESET, TRUE);
	hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_RESET, FALSE);

	// Reset S2H Packet Chain ring
	NO_HWA_PKTPGR_EXPR({
		HWA_RING_STATE(pktchain_ring)->read = 0;
		HWA_RING_STATE(pktchain_ring)->write = 0;
	});
}

static void // Free pkts of TxFIFOs shadow context
hwa_txfifo_shadow_reclaim(hwa_dev_t *dev, uint32 core, uint32 fifo_idx)
{
	wlc_info_t *wlc;
	uint32 physical_fifo;
	void *p;
	int16 cnt;

	HWA_FTRACE(HWA3b);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	wlc = (wlc_info_t *)dev->wlc;
	physical_fifo = WLC_HW_MAP_TXFIFO(wlc, fifo_idx);
	cnt = 0;

	while ((p = hwa_txfifo_getnexttxp32(dev, physical_fifo, HNDDMA_RANGE_ALL))) {
		PKTFREE(dev->osh, p, TRUE);
		cnt++;
	}

	if (cnt > 0) {
		if (TXPKTPENDGET(wlc, fifo_idx) < cnt) {
			HWA_ERROR(("%s fifo %d TXPKTPEND %d free_cnt %d\n", HWA3b,
				fifo_idx, TXPKTPENDGET(wlc, fifo_idx), cnt));
			cnt = TXPKTPENDGET(wlc, fifo_idx);
		}
		wlc_txfifo_complete(wlc, NULL, fifo_idx, cnt);
		HWA_INFO(("%s reclaim fifo %d pkts %d\n", HWA3b, fifo_idx, cnt));
	}
}

#if defined(HWA_DUMP)

#ifdef HWA_PKTPGR_BUILD
static void
_hwa_txfifo_dump_pkt(void *pkt, struct bcmstrbuf *b,
	const char *title, bool one_shot, bool raw)
{
	uint32 pkt_index, mpdu_index;
	hwa_pp_lbuf_t *pkt_curr, *mpdu_curr;

	HWA_ASSERT(pkt);

	pkt_index = 0;
	mpdu_index = 0;
	pkt_curr = (hwa_pp_lbuf_t *)pkt;
	mpdu_curr = (hwa_pp_lbuf_t *)pkt;

	while (mpdu_curr) {
		mpdu_index++;
		while (pkt_curr) {
			pkt_index++;
			HWA_BPRINT(b, "  [%s] 3b:txlbuf-%d.%d at <%p>\n", title, mpdu_index,
				pkt_index, pkt_curr);
			HWA_PRINT("    control:\n");
			HWA_PRINT("      pkt_mapid<%u>\n", PKTMAPID(pkt_curr));
			HWA_PRINT("      flowid<%u>\n", PKTGETCFPFLOWID(pkt_curr));
			HWA_PRINT("      next<%p>\n", PKTNEXT(OSH_NULL, pkt_curr));
			HWA_PRINT("      link<%p>\n", PKTLINK(pkt_curr));
			HWA_PRINT("      flags<0x%x>\n", PKTFLAGS(pkt_curr));
			HWA_PRINT("      data<%p>\n", PKTDATA(OSH_NULL, pkt_curr));
			HWA_PRINT("      len<%u>\n", PKTLEN(OSH_NULL, pkt_curr));
			HWA_PRINT("      ifid<%u>\n", PKTIFINDEX(OSH_NULL, pkt_curr));
			HWA_PRINT("      prio<%u>\n", HWAPKTPRIO(pkt_curr));
			HWA_PRINT("      num_desc<%u> amsdu_total_len<%u> txdma_flags<0x%x>\n",
				HWAPKTNDESC(pkt_curr), HWAPKTAMSDUTLEN(pkt_curr),
				HWAPKTTXDMAFLAGS(pkt_curr));
			HWA_PRINT("      head<%p>\n", PKTHEAD(OSH_NULL, pkt_curr));
			HWA_PRINT("      end<%p>\n", PKTEND(OSH_NULL, pkt_curr));
			HWA_PRINT("      hroom<%d>\n", PKTHEADROOM(OSH_NULL, pkt_curr));
			HWA_PRINT("      troom<%d>\n", PKTTAILROOM(OSH_NULL, pkt_curr));
			HWA_PRINT("    fraginfo:\n");
			HWA_PRINT("      frag_num<%u>\n", PKTFRAGTOTNUM(OSH_NULL, pkt_curr));
			HWA_PRINT("      flags<%u>\n", PKTFLAGSEXT(OSH_NULL, pkt_curr));
			HWA_PRINT("      flowring_id<%u>\n", PKTFRAGFLOWRINGID(OSH_NULL, pkt_curr));
#if defined(PROP_TXSTATUS)
			HWA_PRINT("      rd_idx <%u,%u,%u,%u>\n",
				PKTFRAGRINGINDEX(OSH_NULL, pkt_curr), PKTRDIDX(pkt_curr, 0),
				PKTRDIDX(pkt_curr, 1), PKTRDIDX(pkt_curr, 2));
#endif /* PROP_TXSTATUS */
			HWA_PRINT("      host_pktid<0x%x, 0x%x, 0x%x, 0x%x>\n",
				PKTHOSTPKTID(pkt_curr, 0), PKTHOSTPKTID(pkt_curr, 1),
				PKTHOSTPKTID(pkt_curr, 2), PKTHOSTPKTID(pkt_curr, 3));
			HWA_PRINT("      host_datalen<%u, %u, %u, %u>\n",
				PKTFRAGLEN(OSH_NULL, pkt_curr, 0),
				PKTFRAGLEN(OSH_NULL, pkt_curr, 1),
				PKTFRAGLEN(OSH_NULL, pkt_curr, 2),
				PKTFRAGLEN(OSH_NULL, pkt_curr, 3));
			HWA_PRINT("      data_buf_haddr64<0x%08x,0x%08x / "
				"0x%08x,0x%08x / 0x%08x,0x%08x / 0x%08x,0x%08x>\n",
				PKTFRAGDATA_LO(OSH_NULL, pkt_curr, 0),
				PKTFRAGDATA_HI(OSH_NULL, pkt_curr, 0),
				PKTFRAGDATA_LO(OSH_NULL, pkt_curr, 1),
				PKTFRAGDATA_HI(OSH_NULL, pkt_curr, 1),
				PKTFRAGDATA_LO(OSH_NULL, pkt_curr, 2),
				PKTFRAGDATA_HI(OSH_NULL, pkt_curr, 2),
				PKTFRAGDATA_LO(OSH_NULL, pkt_curr, 3),
				PKTFRAGDATA_HI(OSH_NULL, pkt_curr, 3));
			/* Raw dump if need. */
			if (raw) {
				prhex(title, (uint8 *)pkt_curr, sizeof(hwa_pp_lbuf_t));
			}

			pkt_curr = (hwa_pp_lbuf_t *)pkt_curr->context.control.next;
		}
		if (one_shot)
			break;
		mpdu_curr = (hwa_pp_lbuf_t *)PKTLINK(mpdu_curr);
		pkt_curr = mpdu_curr;
	}
}

#else

static void
_hwa_txfifo_dump_pkt(void *pkt, struct bcmstrbuf *b,
	const char *title, bool one_shot, bool raw)
{
	uint32 pkt_index = 0;
	hwa_txfifo_pkt_t *curr = (hwa_txfifo_pkt_t *)pkt;

	HWA_ASSERT(pkt);
	BCM_REFERENCE(raw);

	while (curr) {
		pkt_index++;
		HWA_BPRINT(b, "  [%s] 3b:swpkt-%d at <%p> lbuf <%p>\n", title, pkt_index, curr,
			HWAPKT2LFRAG((char *)curr));
		HWA_BPRINT(b, "    <%p>:\n", curr);
		HWA_BPRINT(b, "           next <%p>\n", curr->next);
		HWA_BPRINT(b, "           daddr <0x%x>\n", curr->hdr_buf_daddr32);
		HWA_BPRINT(b, "           numdesc <%u>\n", curr->num_desc);
		HWA_BPRINT(b, "           dlen <%u>\n", curr->hdr_buf_dlen);
		HWA_BPRINT(b, "           amsdulen <%u>\n", curr->amsdu_total_len);
		HWA_BPRINT(b, "           hlen <%u>\n", curr->data_buf_hlen);
		HWA_BPRINT(b, "           haddr<0x%08x,0x%08x>\n", curr->data_buf_haddr.loaddr,
			curr->data_buf_haddr.hiaddr);
		HWA_BPRINT(b, "           txdmaflags <0x%x>\n", curr->txdma_flags);
		if (one_shot)
			break;
		curr = curr->next;
	}
}
#endif /* HWA_PKTPGR_BUILD */

void
hwa_txfifo_dump_pkt(void *pkt, struct bcmstrbuf *b,
	const char *title, bool one_shot)
{
	/* Ignore dump */
	if (!(hwa_pktdump_level & HWA_PKTDUMP_TXFIFO)) {
		return;
	}

	_hwa_txfifo_dump_pkt(pkt, b, title, one_shot,
		(hwa_pktdump_level & HWA_PKTDUMP_TXFIFO_RAW) ? TRUE : FALSE);
}
#endif /* HWA_DUMP */

#if (HWA_DEBUG_BUILD >= 1) && (defined(BCMDBG) || 0 || defined(HWA_DUMP))
static void
_hwa_txfifo_dump_shadow32(hwa_txfifo_t *txfifo, struct bcmstrbuf *b,
	uint32 fifo_idx, bool dump_txfifo_shadow)
{
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;

	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);

	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];

	HWA_BPRINT(b, "+ %2u txshadow32 head<0x%08x> tail<0x%08x> count<%u,%u> "
#ifdef HWA_PKTPGR_BUILD
		"hw.count<%u,%u> hw.avail<%u,%u> "
#endif
		"stats.count<%u,%u>\n", fifo_idx,
		myshadow32->pkt_head, myshadow32->pkt_tail,
		myshadow32->pkt_count, myshadow32->mpdu_count,
#ifdef HWA_PKTPGR_BUILD
		myshadow32->hw.pkt_count, myshadow32->hw.mpdu_count,
		myshadow32->hw.txd_avail, myshadow32->hw.aqm_avail,
#endif
		myshadow32->stats.pkt_count, myshadow32->stats.mpdu_count);

	HWA_PKT_DUMP_EXPR({
		if (dump_txfifo_shadow) {
			void *pkt;

			// Save in the shadow before update the write index.
			pkt = HWA_UINT2PTR(void, myshadow32->pkt_head);
			if (pkt == NULL)
				return;

			_hwa_txfifo_dump_pkt(pkt, b, "shadow32", FALSE, FALSE);
		}
	});
}
#endif /* HWA_DEBUG_BUILD >= 1 */

#ifdef HWA_PKTPGR_BUILD

#ifdef PSPL_TX_TEST

// Sanity check and fixup head and end
static void
_hwa_txfifo_pspl_test_pull_rsp_fixup(hwa_dev_t *dev, hwa_pp_lbuf_t *pp_lbuf)
{
	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Data address remap is correct.
	// Set head and end.
	// TXFRAG can only use HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES,
	PKTSETTXBUFRANGE(dev->osh, pp_lbuf, PKTPPBUFFERP(pp_lbuf),
		HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES);
}

static bool
hwa_txfifo_pktchain_xmit_request_pspl_test_push(hwa_dev_t *dev, uint32 fifo_idx,
	void *pktchain_head, void *pktchain_tail, uint16 pkt_count, uint16 mpdu_count,
	uint16 txdescs)
{
	hwa_txfifo_t *txfifo;
	hwa_pktpgr_t *pktpgr;
	hwa_txfifo_shadow32_t *shadow32;

	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);
	HWA_FTRACE(HWA3b);

	// Setup locals
	txfifo = &dev->txfifo;
	pktpgr = &dev->pktpgr;

	// PSPL test not start
	if (!pktpgr->pspl_enable)
		return FALSE;  // Not take it.

	// TEST: TxPost-->Push-->Pull-->TxFIFO
	// HWA_PP_PAGEOUT_HDBM_PKTLIST_WR test: before HWA_PP_PAGEMGR_PUSH_MPDU and
	// HWA_PP_PAGEMGR_PULL_MPDU are ready, let's just limit 4-in-1 AMSDU to use
	// HWA_PP_PAGEMGR_PUSH so that we don't need more SW WAR to fixup next pointer.
	if (pktpgr->pspl_mpdu_enable) { // XXX, CRBCAHWA-669
		void *head, *tail;
		uint16 pcount, mcount;
		uint16 prev_pkt_mapid;

		// Setup locals
		head = pktchain_head;
		tail = pktchain_tail;
		pcount = pkt_count;
		mcount = mpdu_count;
		prev_pkt_mapid = HWA_PP_PKT_MAPID_INVALID;

		// Break 1 link to 2 links
		if (pktpgr->pspl_mpdu_append_enable && mpdu_count >= 4) {
			void *first_h, *first_t, *second_h, *second_t, *curr;

			// Setup locals
			pktpgr->pspl_mpdu_pkt_mapid_h = PKTMAPID(pktchain_head);
			pktpgr->pspl_mpdu_pkt_mapid_t = PKTMAPID(pktchain_tail);
			pktpgr->pspl_mpdu_count = mpdu_count;
			pktpgr->pspl_pkt_count = pkt_count;

			// Step1:  Break 1 link to 2 links.  Assume: <= 8-in-1 AMSDU.
			curr = pktchain_head; // MPDU1
			first_h = pktchain_head;
			second_t = pktchain_tail;
			pcount = 0; mcount = 0;

			mcount++; pcount++;
			if (PKTNEXT(dev->osh, curr)) pcount++;

			curr = PKTLINK(curr); // MPDU2
			first_t = curr;
			mcount++; pcount++;
			if (PKTNEXT(dev->osh, curr)) pcount++;

			second_h = PKTLINK(curr); // MPDU3
			PKTSETLINK(curr, NULL); // Terminate first link

			prev_pkt_mapid = PKTMAPID(first_t);

			// Step2: Send first one.
			hwa_pktpgr_push_mpdu_req(dev, fifo_idx, HWA_PP_CMD_NOT_TAGGED,
				first_h, first_t, pcount, mcount, HWA_PP_PKT_MAPID_INVALID);
			pktpgr->pspl_mpdu_append_count++;

			// Step3: Send second one.
			head = second_h;
			tail = second_t;
			pcount = pkt_count - pcount;
			mcount = mpdu_count - mcount;
			pktpgr->pspl_mpdu_append_count++;

			// Go through to below hwa_pktpgr_push_mpdu_req

			// Step4: One shot test.
			pktpgr->pspl_mpdu_append_enable = FALSE;
		}

		hwa_pktpgr_push_mpdu_req(dev, fifo_idx, HWA_PP_CMD_NOT_TAGGED,
			head, tail, pcount, mcount, prev_pkt_mapid);
	} else {
		hwa_pktpgr_push_req(dev, fifo_idx, HWA_PP_CMD_NOT_TAGGED,
			pktchain_head, pktchain_tail, pkt_count, mpdu_count);
	}
	// Increase hw count after we pageout pktlist
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo_idx; // Move to specific index
	shadow32->hw.pkt_count += pkt_count;
	shadow32->hw.mpdu_count += mpdu_count;

	// Decrease TX/AQM available descriptors
	hwa_txfifo_descs_dec(txfifo, fifo_idx, txdescs, mpdu_count);

	return TRUE;
}

// NOTE: pktlist_head and pktlist_tail address space are in DDBM.
void
hwa_txfifo_pktchain_xmit_request_pspl_test_pull(hwa_dev_t *dev, uint8 pktpgr_trans_id,
	int fifo_idx, int mpdu_count, int pkt_count,
	hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	uint16 pkt;
	hwa_pp_pageout_req_pktlist_t req;
	hwa_pp_lbuf_t *pp_lbuf;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup local
	pp_lbuf = pktlist_head;
	pkt = 0;

	if (mpdu_count == 0) {
		HWA_ERROR(("%s: mpdu_count is 0\n", __FUNCTION__));
		return;
	}

	// Fixup head and end
	HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);
	for (pkt = 0; pkt < mpdu_count - 1; ++pkt) {
		_hwa_txfifo_pspl_test_pull_rsp_fixup(dev, pp_lbuf);
		pp_lbuf = (hwa_pp_lbuf_t*)PKTLINK(pp_lbuf);
	}
	HWA_ASSERT(pp_lbuf == pktlist_tail);
	_hwa_txfifo_pspl_test_pull_rsp_fixup(dev, pp_lbuf);

	// Transmit TxLfrags: *WithResponse
	// Must use HWA_PP_PAGEOUT_PKTLIST_WR if SW Tx/AQM descriptor accounting is enabled.
	req.trans        = HWA_PP_PAGEOUT_PKTLIST_WR;
	req.fifo_idx     = (uint8)fifo_idx;
	req.zero         = 0;
	req.mpdu_count   = mpdu_count;
	req.pkt_count    = pkt_count;
	req.pktlist_head = (uint32)pktlist_head;
	req.pktlist_tail = (uint32)pktlist_tail;
	if (HWAREV_LE(dev->corerev, 132)) {
		HWA_ASSERT(pkt_count <= HWA_PKTPGR_PAGEOUT_PKTCOUNT_MAX);
		req.pkt_count_copy = (uint8)pkt_count;
	}
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBF;
	}

	HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)pktlist_head, NULL,
		"PULL:PAGEOUT_PKTLIST_WR", TRUE));

	if (hwa_pktpgr_request(dev, hwa_pktpgr_pageout_req_ring, &req) == HWA_FAILURE)
		goto failure;

	// Increase hw count after we pageout pktlist.
	// Done in hwa_txfifo_pktchain_xmit_request already

	// Decrease TX/AQM available descriptors
	// Done in hwa_txfifo_pktchain_xmit_request already

	HWA_PP_DBG(HWA_PP_DBG_MGR_TX, "  >>PAGEOUT::REQ PULL:PKTLIST_WR pkts %3u/%3u "
		"list[0x%p(%d) .. 0x%p(%d)] fifo %3u ==PULL:PKTLIST_WR(%d)==>\n\n",
		pkt_count, mpdu_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx, pktpgr_trans_id);

#if defined(BCMDBG) || defined(HWA_DUMP)
	HWA_DEBUG_EXPR(_hwa_txfifo_dump_shadow32(&dev->txfifo, NULL, fifo_idx, TRUE));
#endif

	return;

failure:
	HWA_WARN(("%s PAGEOUT::REQ PULL:PKTLIST_NR failure fifo<%u> head<%p> tail<%p>"
		" pkt<%u> mpdu<%u>\n", HWA3b, fifo_idx, pktlist_head, pktlist_tail,
		pkt_count, mpdu_count));

	return;
}

#endif /* PSPL_TX_TEST */

void
hwa_txfifo_descs_inc(hwa_txfifo_t *txfifo, uint32 fifo_idx, uint16 txdescs,
	uint16 aqmdescs)
{
	hwa_txfifo_shadow32_t *shadow32;

	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo_idx; // Move to specific index

	// Increase TX/AQM available descriptors
	shadow32->hw.txd_avail += txdescs;
	shadow32->hw.aqm_avail += aqmdescs;
}

void
hwa_txfifo_descs_dec(hwa_txfifo_t *txfifo, uint32 fifo_idx, uint16 txdescs,
	uint16 aqmdescs)
{
	hwa_txfifo_shadow32_t *shadow32;

	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo_idx; // Move to specific index

	// Decrease TX/AQM available descriptors
	HWA_ASSERT(shadow32->hw.txd_avail >= txdescs);
	HWA_ASSERT(shadow32->hw.aqm_avail >= aqmdescs);
	shadow32->hw.txd_avail -= txdescs;
	shadow32->hw.aqm_avail -= aqmdescs;
}

bool
hwa_txfifo_descs_isfull(struct hwa_dev *dev, uint fifo, uint16 txdescs, uint16 aqmdescs)
{
	wlc_info_t *wlc;
	hwa_txfifo_t *txfifo;
	wlc_hwa_txpkt_t *hwa_txpkt;
	hwa_txfifo_shadow32_t *shadow32;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo < HWA_TX_FIFOS);
	txfifo = &dev->txfifo;
	wlc = (wlc_info_t *)dev->wlc;
	hwa_txpkt = wlc->hwa_txpkt;
	BCM_REFERENCE(hwa_txpkt);

	// Read txfifo shadow packet count
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo; // Move to specific index

	// For PKTPGR, both HW and SW mpdu_count
	if (aqmdescs >= shadow32->hw.aqm_avail) {
		HWA_TRACE(("aqm-%d: request %d >= avail %d\n", fifo, aqmdescs,
			shadow32->hw.aqm_avail));
		return TRUE;
	}

	if (txdescs >= shadow32->hw.txd_avail) {
		HWA_TRACE(("fifo-%d: request %d >= avail %d hwa_txpkt<%d,%d>\n",
			fifo, txdescs, shadow32->hw.txd_avail, hwa_txpkt->mpdu_count,
			hwa_txpkt->txdescs));
		return TRUE;
	}

	return FALSE;
}

void * // Provide the TXed packet
hwa_txfifo_getnexttxp32(struct hwa_dev *dev, uint32 fifo_idx, uint32 range)
{
	uint16 pkt_count;
	void *p;
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_pp_lbuf_t *txlbuf, *next;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txlbuf = HWA_UINT2PTR(hwa_pp_lbuf_t, myshadow32->pkt_head);
	if (txlbuf == NULL) {
		if (range != HNDDMA_RANGE_ALL) {
			HWA_ERROR(("%s TX FIFO-%d shadow is empty\n",
				HWA3b, fifo_idx));
		}
		return NULL;
	}

	//The num_desc in HWA3b_Misc0 should not be 0
	ASSERT(txlbuf->context.control.txfifo.num_desc > 0);

	//Update shdow32[fifo_idx]
	next = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
	if (next == NULL) {
		ASSERT(myshadow32->mpdu_count == 1);
		myshadow32->pkt_head = NULL;
		myshadow32->pkt_tail = NULL;
	} else {
		myshadow32->pkt_head = HWA_PTR2UINT(next);
	}

	// pkt_count
	p = txlbuf;
	pkt_count = 1;
	for (; PKTNEXT(dev->osh, p); p = PKTNEXT(dev->osh, p))
		pkt_count++;
	HWA_ASSERT(myshadow32->pkt_count >= pkt_count);
	myshadow32->pkt_count -= pkt_count;
	myshadow32->mpdu_count--;

	// Set terminater of MPDU
	HWAPKTSETLINK(txlbuf, NULL);

	HWA_TRACE(("%s Get a txp %p from TX FIFO-%d shadow\n", HWA3b, txlbuf, fifo_idx));

	HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)txlbuf, NULL, "getnexttxp32", FALSE));

	return (void*)txlbuf;
}

// hwa_txfifo_peeknexttxp will be called by wlc_dotxstatus after pagein txstatus.
void * // Like hwa_txfifo_getnexttxp32 but no reclaim
hwa_txfifo_peeknexttxp(struct hwa_dev *dev, uint32 fifo_idx)
{
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_pp_lbuf_t *txlbuf;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txlbuf = HWA_UINT2PTR(hwa_pp_lbuf_t, myshadow32->pkt_head);
	if (txlbuf == NULL)
		return NULL;

	//The num_desc in pkt_head should not be 0
	ASSERT(txlbuf->context.control.txfifo.num_desc > 0);

	return (void*)txlbuf;
}

int // HWA txfifo map function.
hwa_txfifo_map_pkts(struct hwa_dev *dev, uint32 fifo_idx, void *cb, void *ctx)
{
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_pp_lbuf_t *txlbuf, *curr;
	map_pkts_cb_fn map_pkts_cb = (map_pkts_cb_fn)cb;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	// SW shadow map
	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txlbuf = HWA_UINT2PTR(hwa_pp_lbuf_t, myshadow32->pkt_head);
	if (txlbuf) {
		//Do map on each packets that num_desc is not 0
		curr = txlbuf;
		while (curr) {
			ASSERT(curr->context.control.txfifo.num_desc > 0);
			/* ignoring the return 'delete' bool since hwa
			 * does not allow deleting pkts on the ring.
			 */
			(void)map_pkts_cb(ctx, (void*)curr);
			curr = (hwa_pp_lbuf_t *)PKTLINK(curr);
		}
	}

	// HW shadow map, sync mode
	hwa_pktpgr_map_pkts(dev, fifo_idx, cb, ctx);

	return BCME_OK;
}

// Handle a request from WLAN driver for transmission of a packet chain
// PAGEOUT REQUEST PKTLIST
bool
hwa_txfifo_pktchain_xmit_request(struct hwa_dev *dev, uint32 core,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count, uint16 txdescs)
{
	bool pkt_local, pktchain_ring_full;
	int pktpgr_trans_id;
	hwa_pp_cmd_t req_cmd;
	hwa_txfifo_t *txfifo;
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t   *pageout_req_ring;
	hwa_txfifo_shadow32_t *shadow32;

	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);
	HWA_FTRACE(HWA3b);

	// Setup locals
	txfifo = &dev->txfifo;
	pktpgr = &dev->pktpgr;
	pktpgr_trans_id = HWA_FAILURE;
	pktchain_ring_full = FALSE;
	pageout_req_ring = &pktpgr->pageout_req_ring;

	// Assume PageOut Req Q is specific for Tx packet.  It's equial to 3B pktchain_ring.
	// Caller need to check if pageout_req_ring is full.
	HWA_ASSERT(!hwa_ring_is_full(pageout_req_ring));

	pkt_local = PKTISMGMTTXPKT(dev->osh, pktchain_head);
	if (pkt_local) {
		// Transmit Local Pkt
		HWA_ASSERT(mpdu_count == 1);
		HWA_ASSERT(pkt_count == 1);
		HWA_ASSERT(txdescs == 1);
		req_cmd.pageout_req_pktlocal.trans       = HWA_PP_PAGEOUT_PKTLOCAL;
		req_cmd.pageout_req_pktlocal.fifo_idx    = (uint16)fifo_idx;
		req_cmd.pageout_req_pktlocal.pkt_local   = (uint32)pktchain_head;
		if (HWAREV_LE(dev->corerev, 132)) {
			req_cmd.pageout_req_pktlocal.pkt_count_copy = (uint8)pkt_count;
		}

#ifdef HWA_QT_TEST
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133)) {
			req_cmd.pageout_req_pktlocal.swar = 0xBF;
		}
#endif

		// Store original local packet address
		PKTSETLOCAL(dev->osh, pktchain_head, pktchain_head);

		if (PKTHASHMEDATA(dev->osh, pktchain_head)) {
			PKTSETLOCALFIXUP(dev->osh, pktchain_head);
		}

		HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt(pktchain_head, NULL,
			"PAGEOUT_PKTLOCAL", FALSE));
	} else {
#ifdef PSPL_TX_TEST
		if (hwa_txfifo_pktchain_xmit_request_pspl_test_push(dev, fifo_idx,
			pktchain_head, pktchain_tail, pkt_count, mpdu_count, txdescs))
			return FALSE;
#endif

		// Transmit TxLfrags: *WithResponse
		// Must use HWA_PP_PAGEOUT_PKTLIST_WR if SW Tx/AQM descriptor accounting is enabled.
		req_cmd.pageout_req_pktlist.trans        = HWA_PP_PAGEOUT_PKTLIST_WR;
		req_cmd.pageout_req_pktlist.fifo_idx     = (uint8)fifo_idx;
		req_cmd.pageout_req_pktlist.zero         = 0;
		req_cmd.pageout_req_pktlist.mpdu_count   = mpdu_count;
		req_cmd.pageout_req_pktlist.pkt_count    = pkt_count;
		req_cmd.pageout_req_pktlist.pktlist_head = (uint32)pktchain_head;
		req_cmd.pageout_req_pktlist.pktlist_tail = (uint32)pktchain_tail;
		if (HWAREV_LE(dev->corerev, 132)) {
			HWA_ASSERT(pkt_count <= HWA_PKTPGR_PAGEOUT_PKTCOUNT_MAX);
			req_cmd.pageout_req_pktlist.pkt_count_copy = (uint8)pkt_count;
		}

#ifdef HWA_QT_TEST
		// XXX, CRBCAHWA-662
		if (HWAREV_GE(dev->corerev, 133)) {
			req_cmd.pageout_req_pktlist.swar = 0xBF;
		}
#endif

		HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt(pktchain_head, NULL,
			"PAGEOUT_PKTLIST_WR", FALSE));

#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
{
		int i;
		void *txlbuf, *next;

		// Dealloc: TxPost normal case.
		txlbuf = pktchain_head;
		for (i = 0; i < mpdu_count; i++) {
			hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
				DBM_AUDIT_FREE);
			next = PKTNEXT(dev->osh, txlbuf);
			for (; next; next = PKTNEXT(dev->osh, next)) {
				hwa_pktpgr_dbm_audit(dev, next, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
					DBM_AUDIT_FREE);
			}
			txlbuf = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
		}
}
#endif /* HWA_PKTPGR_DBM_AUDIT_ENABLED */

	}

	// PageOut Req should not fail, because caller checked the ring status already.
	pktpgr_trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pageout_req_ring, &req_cmd);
	if (pktpgr_trans_id == HWA_FAILURE) {
		pktchain_ring_full = TRUE;
		goto failure;
	}

	// Update PageOut local counter.
	if (pkt_local) {
		HWA_COUNT_INC(pktpgr->pgo_local_req, 1);
	} else {
		HWA_COUNT_INC(pktpgr->pgo_pktlist_req, 1);
		// DDBM accounting, move to resp.
	}

	// Increase hw count after we pageout pktlist
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo_idx; // Move to specific index
	shadow32->hw.pkt_count += pkt_count;
	shadow32->hw.mpdu_count += mpdu_count;

	// Decrease TX/AQM available descriptors
	hwa_txfifo_descs_dec(txfifo, fifo_idx, txdescs, mpdu_count);

	HWA_PP_DBG(HWA_PP_DBG_3B, "  >>PAGEOUT::REQ %s pkts %3u/%3u txdma_desc %3u "
		"list[0x%p(%d) .. 0x%p(%d)] fifo %3u ==%s(%d)==>\n\n",
		pkt_local ? "PKTLOCAL" : "PKTLIST_WR", pkt_count, mpdu_count, txdescs,
		pktchain_head, PKTMAPID(pktchain_head), pktchain_tail, PKTMAPID(pktchain_tail),
		fifo_idx, pkt_local ? "TXLOCAL-REQ" : "TXLIST-REQ", pktpgr_trans_id);

#if defined(BCMDBG) || defined(HWA_DUMP)
	HWA_DEBUG_EXPR(_hwa_txfifo_dump_shadow32(txfifo, NULL, fifo_idx, TRUE));
#endif

	// Notify WL txfifo pktchain ring is full
	if (hwa_ring_is_full(pageout_req_ring)) {
		wlc_bmac_hwa_txfifo_ring_full(dev->wlc, TRUE);
		pktchain_ring_full = TRUE;
	}

	return pktchain_ring_full;

failure:
	HWA_WARN(("%s PAGEOUT::REQ %s failure fifo<%u> head<%p> tail<%p>"
		" pkt<%u> mpdu<%u>\n", HWA3b, pkt_local ? "PKTLOCAL" : "PKTLIST_WR",
		fifo_idx, pktchain_head, pktchain_tail, pkt_count, mpdu_count));

	return pktchain_ring_full;

} // hwa_txfifo_pktchain_xmit_request

#else

void // Clear OvflowQ pkt_count, mpdu_count to avoid 3b process reclaimed packets
hwa_txfifo_clear_ovfq(struct hwa_dev *dev, uint32 core, uint32 fifo_idx)
{
	hwa_txfifo_t *txfifo;
	uint32 *sys_mem;
	hwa_mem_addr_t axi_mem_addr;
	hwa_txfifo_ovflwqctx_t ovflwq;

	txfifo = &dev->txfifo;
	sys_mem = &ovflwq.u32[0];
	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);

	if (fifo_idx >= txfifo->fifo_total || !isset(txfifo->fifo_enab, fifo_idx)) {
		HWA_ERROR(("%s: Wrong argument fifo_idx<%u:%u>\n",
			__FUNCTION__, fifo_idx, txfifo->fifo_total));
		return;
	}

	// Clear pkt_count
	/* NOTE: We need to update one full context (28-bytes at least)
	 * to trigger HWA update
	 */
	axi_mem_addr = HWA_TABLE_ADDR(hwa_txfifo_ovflwqctx_t,
		txfifo->ovflwq_addr, fifo_idx);
	HWA_RD_MEM32(HWA3b, hwa_txfifo_ovflwqctx_t, axi_mem_addr, sys_mem);
	if (ovflwq.pkt_count) {
		// Add pkt_count to append_count to make it sync with shadow pkt_count
		ovflwq.append_count += ovflwq.pkt_count;
		// Clear counts
		ovflwq.pkt_count = 0;
		ovflwq.mpdu_count = 0;
		HWA_WR_MEM32(HWA3b, hwa_txfifo_ovflwqctx_t, axi_mem_addr, sys_mem);
	}
}

static INLINE void __hwa_txfifo_pktchain32_post(hwa_txfifo_t *txfifo,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count);
static INLINE void __hwa_txfifo_pktchain64_post(hwa_txfifo_t *txfifo,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count);

static INLINE void
__hwa_txfifo_pktchain32_post(hwa_txfifo_t *txfifo,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count)
{
	hwa_txfifo_pktchain32_t *s2h_req32;
	hwa_ring_t *pktchain_ring;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_txfifo_pkt_t *txfifo_pkt;

	ASSERT(pktchain_head);
	ASSERT(pktchain_tail);
	ASSERT(pkt_count);
	ASSERT(mpdu_count);

	pktchain_ring = &txfifo->pktchain_ring;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];

	s2h_req32 = HWA_RING_PROD_ELEM(hwa_txfifo_pktchain32_t, pktchain_ring);
	s2h_req32->pkt_head   = HWA_PTR2UINT(pktchain_head);
	s2h_req32->pkt_tail   = HWA_PTR2UINT(pktchain_tail);
	s2h_req32->pkt_count  = pkt_count;
	s2h_req32->mpdu_count = mpdu_count;
	s2h_req32->fifo_idx   = fifo_idx;

	// Save in the shadow before update the write index.
	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);
	if (myshadow32->pkt_head == NULL) {
		myshadow32->pkt_head = HWA_PTR2UINT(pktchain_head);
		myshadow32->pkt_tail = HWA_PTR2UINT(pktchain_tail);
		myshadow32->pkt_count = pkt_count;
		myshadow32->mpdu_count = mpdu_count;
	} else {
		/* SW MUST mantain the next pointer linking even in A0, we saw a
		 * case that 3b doesn't update the swpkt1 next to point to swpkt2
		 * becase 3b process swpkt1 from 3b chainQ to ovfQ to MAC DMA
		 * to fast so that swpkt1/swpkt2 never co-exist at ovfQ.
		 */
		txfifo_pkt = HWA_UINT2PTR(hwa_txfifo_pkt_t, myshadow32->pkt_tail);
		HWAPKTSETNEXT(txfifo_pkt, pktchain_head);
		myshadow32->pkt_tail = HWA_PTR2UINT(pktchain_tail);
		myshadow32->pkt_count += pkt_count;
		myshadow32->mpdu_count += mpdu_count;
	}
	myshadow32->stats.pkt_count += pkt_count;
	myshadow32->stats.mpdu_count += mpdu_count;
} // __hwa_txfifo_pktchain32_post

static INLINE void
__hwa_txfifo_pktchain64_post(hwa_txfifo_t *txfifo,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count)
{
	// FIXME: NIC mode 64bit addressing mode ...
	HWA_ASSERT(0);
	HWA_ERROR(("XXX: %s pktchain64 not ready\n", HWA3b));
} // __hwa_txfifo_pktchain64_post

void * // Provide the TXed packet
hwa_txfifo_getnexttxp32(struct hwa_dev *dev, uint32 fifo_idx, uint32 range)
{
	uint16 pkt_count;
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_txfifo_pkt_t *txfifo_pkt, *curr;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);

	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txfifo_pkt = HWA_UINT2PTR(hwa_txfifo_pkt_t, myshadow32->pkt_head);
	if (txfifo_pkt == NULL) {
		if (range != HNDDMA_RANGE_ALL) {
			HWA_ERROR(("%s TX FIFO-%d shadow is empty\n",
				HWA3b, fifo_idx));
		}
		return NULL;
	}

	// For debug HWA3b update the swpkt next.
	//HWA_DEBUG_EXPR(_hwa_txfifo_dump_shadow32(txfifo, NULL, fifo_idx, TRUE));

	//The num_desc in pkt_head should not be 0
	ASSERT(txfifo_pkt->num_desc > 0);

	//Find the next non-zero num_desc 3b-SWPTK
	curr = txfifo_pkt;
	pkt_count = 1;
	while (curr->next && curr->next->num_desc == 0) {
		pkt_count++;
		curr = curr->next;
	}

	//Update shdow32[fifo_idx]
	if (curr->next == NULL) {
		ASSERT(myshadow32->pkt_count == pkt_count);
		ASSERT(myshadow32->mpdu_count == 1);
		myshadow32->pkt_head = NULL;
		myshadow32->pkt_tail = NULL;
		myshadow32->pkt_count = 0;
		myshadow32->mpdu_count = 0;
	} else {
		myshadow32->pkt_head = HWA_PTR2UINT(curr->next);
		myshadow32->pkt_count -= pkt_count;
		myshadow32->mpdu_count--;
	}

	// Set terminater of MPDU
	// for debug, don't clear it so that we can trace it.
	// HWAPKTSETNEXT(curr, NULL);

	HWA_TRACE(("%s Get a txp %p (swpkt@%p) from TX FIFO-%d shadow\n", HWA3b,
		(void*)HWAPKT2LFRAG((char *)txfifo_pkt), txfifo_pkt, fifo_idx));

	//HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)txfifo_pkt, NULL, "getnexttxp32", FALSE));

	return (void*)HWAPKT2LFRAG((char *)txfifo_pkt);
}

void * // Like hwa_txfifo_getnexttxp32 but no reclaim
hwa_txfifo_peeknexttxp(struct hwa_dev *dev, uint32 fifo_idx)
{
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_txfifo_pkt_t *txfifo_pkt;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);

	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txfifo_pkt = HWA_UINT2PTR(hwa_txfifo_pkt_t, myshadow32->pkt_head);
	if (txfifo_pkt == NULL)
		return NULL;

	//The num_desc in pkt_head should not be 0
	ASSERT(txfifo_pkt->num_desc > 0);

	return (void*)HWAPKT2LFRAG((char *)txfifo_pkt);
}

int // HWA txfifo map function.
hwa_txfifo_map_pkts(struct hwa_dev *dev, uint32 fifo_idx, void *cb, void *ctx)
{
	hwa_txfifo_t *txfifo;
	hwa_txfifo_shadow32_t *shadow32, *myshadow32;
	hwa_txfifo_pkt_t *txfifo_pkt, *curr;
	map_pkts_cb_fn map_pkts_cb = (map_pkts_cb_fn)cb;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);

	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	myshadow32 = &shadow32[fifo_idx];
	txfifo_pkt = HWA_UINT2PTR(hwa_txfifo_pkt_t, myshadow32->pkt_head);

	if (txfifo_pkt == NULL)
		return BCME_OK;

	//The num_desc in pkt_head should not be 0
	ASSERT(txfifo_pkt->num_desc > 0);

	//Do map on each packets that num_desc is not 0
	curr = txfifo_pkt;
	while (curr) {
		if (curr->num_desc) {
			/* ignoring the return 'delete' bool since hwa
			 * does not allow deleting pkts on the ring.
			 */
			(void)map_pkts_cb(ctx, (void*)HWAPKT2LFRAG((char *)curr));
		}
		curr = curr->next;
	}

	return BCME_OK;
}

bool // Handle a request from WLAN driver for transmission of a packet chain
hwa_txfifo_pktchain_xmit_request(struct hwa_dev *dev, uint32 core,
	uint32 fifo_idx, void *pktchain_head, void *pktchain_tail,
	uint16 pkt_count, uint16 mpdu_count, uint16 txdescs)
{
	hwa_ring_t *pktchain_ring; // S2H pktchain ring context
	hwa_txfifo_t *txfifo;
	bool pktchain_ring_full;

	HWA_ASSERT(dev != (struct hwa_dev*)NULL);
	HWA_FTRACE(HWA3b);
	BCM_REFERENCE(txdescs);

	txfifo = &dev->txfifo;
	pktchain_ring = &txfifo->pktchain_ring;
	pktchain_ring_full = FALSE;

	// Caller need to check if pktchain_ring is full.
	HWA_ASSERT(!hwa_ring_is_full(pktchain_ring));

	if (txfifo->pktchain_fmt == sizeof(hwa_txfifo_pktchain32_t))
		__hwa_txfifo_pktchain32_post(txfifo,
			fifo_idx, pktchain_head, pktchain_tail, pkt_count, mpdu_count);
	else
		__hwa_txfifo_pktchain64_post(txfifo,
			fifo_idx, pktchain_head, pktchain_tail, pkt_count, mpdu_count);

	hwa_ring_prod_upd(pktchain_ring, 1, TRUE); // update/commit WR

	// Notify WL txfifo pktchain ring is full
	if (hwa_ring_is_full(pktchain_ring)) {
		wlc_bmac_hwa_txfifo_ring_full(dev->wlc, TRUE);
		pktchain_ring_full = TRUE;
	}

	HWA_TRACE(("%s xmit pktchain[%u,%u]"
		" fifo<%u> head<%p> tail<%p> pkt<%u> mpdu<%u>\n",
		HWA3b, HWA_RING_STATE(pktchain_ring)->write,
		HWA_RING_STATE(pktchain_ring)->read,
		fifo_idx, pktchain_head, pktchain_tail, pkt_count, mpdu_count));

#if defined(BCMDBG) || defined(HWA_DUMP)
	HWA_DEBUG_EXPR(_hwa_txfifo_dump_shadow32(txfifo, NULL, fifo_idx, TRUE));
#endif

	return pktchain_ring_full;

} // hwa_txfifo_pktchain_xmit_request
#endif /* HWA_PKTPGR_BUILD */

bool
hwa_txfifo_pktchain_ring_isfull(struct hwa_dev *dev)
{
	hwa_ring_t *txpkt_ring;
#ifdef HWA_PKTPGR_BUILD
	hwa_pktpgr_t *pktpgr;
#else
	hwa_txfifo_t *txfifo;
#endif

	HWA_ASSERT(dev != (struct hwa_dev*)NULL);

	// Setup locals
#ifdef HWA_PKTPGR_BUILD
	pktpgr = &dev->pktpgr;
	txpkt_ring = &pktpgr->pageout_req_ring;
#else
	txfifo = &dev->txfifo;
	txpkt_ring = &txfifo->pktchain_ring;
#endif

	return (hwa_ring_is_full(txpkt_ring));
}

#if defined(BCM_BUZZZ_KPI_QUE_LEVEL) && (BCM_BUZZZ_KPI_QUE_LEVEL > 0)
static uint8 * buzzz_mac_txfifo(uint8 *buzzz_log);
static uint8 * // Log all logical Tx Fifos pkt and mpdu count, occupancy
buzzz_mac_txfifo(uint8 *buzzz_log)
{
	uint16 fifo_idx, fifo_cnt;
	hwa_dev_t *dev;
	hwa_txfifo_shadow32_t * shadow32;

	struct buzzz_log_fifo {
		uint16 pkt_count; uint16 mpdu_count;
	} * buzzz_log_fifo;
	bcm_buzzz_subsys_hdr_t *buzzz_log_mac;

	dev = HWA_DEVP(FALSE); // CAUTION: global access without audit
	if (dev == NULL) return buzzz_log;

	fifo_cnt = WLC_HW_NFIFO_INUSE((wlc_info_t *)dev->wlc);

	buzzz_log_mac  = (bcm_buzzz_subsys_hdr_t*)buzzz_log;
	buzzz_log_mac->id  = BUZZZ_MAC_SUBSYS;
	buzzz_log_mac->u8  = fifo_cnt;

	buzzz_log_fifo = (struct buzzz_log_fifo *)(buzzz_log_mac + 1);

	shadow32 = (hwa_txfifo_shadow32_t *)dev->txfifo.txfifo_shadow;
	for (fifo_idx = 0; fifo_idx < fifo_cnt; fifo_idx++) {
		buzzz_log_fifo->pkt_count  = shadow32[fifo_idx].pkt_count;
		buzzz_log_fifo->mpdu_count = shadow32[fifo_idx].mpdu_count;
		buzzz_log_fifo++;
	}

	return (uint8*)buzzz_log_fifo;

} /* buzzz_mac_txfifo */
#endif /* BCM_BUZZZ_KPI_QUE_LEVEL */

// HWA3b TxFifo block statistics collection
static void _hwa_txfifo_stats_dump(hwa_dev_t *dev, uintptr buf, uint32 core);

void // Clear statistics for HWA3b TxFifo block
hwa_txfifo_stats_clear(hwa_txfifo_t *txfifo, uint32 core)
{
	hwa_dev_t *dev;

	dev = HWA_DEV(txfifo);

	hwa_stats_clear(dev, HWA_STATS_TXDMA); // common

} // hwa_txfifo_stats_clear

void // Print the common statistics for HWA3b TxFifo block
_hwa_txfifo_stats_dump(hwa_dev_t *dev, uintptr buf, uint32 core)
{
	hwa_txfifo_stats_t *txfifo_stats = &dev->txfifo.stats;
	struct bcmstrbuf *b = (struct bcmstrbuf *)buf;

	HWA_BPRINT(b, "%s statistics pktcq_empty<%u> ovfq_empty<%u> "
		"ovfq_full<%u> stall<%u>\n", HWA3b,
		txfifo_stats->pktc_empty, txfifo_stats->ovf_empty,
		txfifo_stats->ovf_full, txfifo_stats->pull_fsm_stall);
} // _hwa_txfifo_stats_dump

void // Query and dump common statistics for HWA3b TxFifo block
hwa_txfifo_stats_dump(hwa_txfifo_t *txfifo, struct bcmstrbuf *b, uint8 clear_on_copy)
{
	hwa_dev_t *dev;

	dev = HWA_DEV(txfifo);

	// FIXME TBD is this per core?
	hwa_txfifo_stats_t *txfifo_stats = &txfifo->stats;
	hwa_stats_copy(dev, HWA_STATS_TXDMA,
		HWA_PTR2UINT(txfifo_stats), HWA_PTR2HIADDR(txfifo_stats),
		/* num_sets */ 1, clear_on_copy, &_hwa_txfifo_stats_dump,
		(uintptr)b, 0U);

} // hwa_txfifo_stats_dump

int
hwa_txfifo_get_ovfq(hwa_dev_t *dev, uint32 fifo_idx, hwa_txfifo_ovflwqctx_t *ovflwq)
{
	uint32 *sys_mem;
	hwa_txfifo_t *txfifo;
	hwa_mem_addr_t axi_mem_addr;

	HWA_ASSERT(fifo_idx < HWA_OVFLWQ_MAX);
	HWA_ASSERT(ovflwq);

	txfifo = &dev->txfifo;
	sys_mem = &ovflwq->u32[0];

	if (fifo_idx >= txfifo->fifo_total ||
		!isset(txfifo->fifo_enab, fifo_idx)) {
		HWA_ERROR(("Wrong argument fifo_idx<%u:%u>\n",
			fifo_idx, txfifo->fifo_total));
		return HWA_FAILURE;
	}

	axi_mem_addr = HWA_TABLE_ADDR(hwa_txfifo_ovflwqctx_t,
		txfifo->ovflwq_addr, fifo_idx);
	HWA_RD_MEM32(HWA3b, hwa_txfifo_ovflwqctx_t, axi_mem_addr, sys_mem);

	return ovflwq->mpdu_count;
}

#if defined(BCMDBG) || defined(HWA_DUMP)

void // Dump HWA3b state
hwa_txfifo_state(hwa_dev_t *dev)
{
	hwa_regs_t *regs;

	HWA_AUDIT_DEV(dev);
	regs = dev->regs;

	HWA_PRINT("%s state sts<0x%08x. sts2<0x%08x> sts3<0x%08x>\n", HWA3b,
		HWA_RD_REG_NAME(HWA3b, regs, txdma, state_sts),
		HWA_RD_REG_NAME(HWA3b, regs, txdma, state_sts2),
		HWA_RD_REG_NAME(HWA3b, regs, txdma, state_sts3));

} // hwa_txfifo_state

void
hwa_txfifo_dump_ovfq(struct hwa_dev *dev, struct bcmstrbuf *b, uint32 fifo_idx)
{
	int mpdu_count;
	hwa_txfifo_ovflwqctx_t ovflwq;

	mpdu_count = hwa_txfifo_get_ovfq(dev, fifo_idx, &ovflwq);
	if (mpdu_count == HWA_FAILURE)
		return;

	HWA_BPRINT(b, "+ %2u ovflwq head<0x%08x,0x%08x> tail<0x%08x,0x%08x>"
		" count<%u,%u> hwm<%u,%u> append<%u>\n", fifo_idx,
		ovflwq.pktq_head.hiaddr, ovflwq.pktq_head.loaddr,
		ovflwq.pktq_tail.hiaddr, ovflwq.pktq_tail.loaddr,
		ovflwq.pkt_count, ovflwq.mpdu_count,
		ovflwq.pkt_count_hwm, ovflwq.mpdu_count_hwm,
		ovflwq.append_count);
}

/* NOTE:  Accress fifoctx through AXI is dangerous,
 * it could be AXI timeout.  Only for debug.
 */
void
hwa_txfifo_dump_fifoctx(struct hwa_dev *dev, struct bcmstrbuf *b, uint32 fifo_idx)
{
	hwa_txfifo_t *txfifo;
	uint32 *sys_mem;
	hwa_mem_addr_t axi_mem_addr;
	hwa_txfifo_fifoctx_t fifoctx;

	txfifo = &dev->txfifo;
	sys_mem = &fifoctx.u32[0];
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);

	if (fifo_idx >= txfifo->fifo_total || !isset(txfifo->fifo_enab, fifo_idx)) {
		HWA_BPRINT(b, "Wrong argument fifo_idx<%u:%u>\n",
			fifo_idx, txfifo->fifo_total);
		return;
	}

#if !defined(HWA_PKTPGR_BUILD) // PKTPGR set TXDMA_STOP_OVFLOWQ always true results 3B IDLE.
	// Don't access txfifoctx when 3B is disable
	if (hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_IDLE, TRUE)) {
		HWA_BPRINT(b, "+ Ignore fifoctx (3B is disabled)\n");
		return;
	}
#endif

	axi_mem_addr = HWA_TABLE_ADDR(hwa_txfifo_fifoctx_t,
		txfifo->txfifo_addr, fifo_idx);
	HWA_RD_MEM32(HWA3b, hwa_txfifo_fifoctx_t,
		axi_mem_addr, sys_mem);
	HWA_BPRINT(b, "+ %2u fifoctx-tx  base<0x%08x,0x%08x> "
		"curr_ptr<0x%04x> last_ptr<0x%04x> attrib<%u> depth<%u>\n", fifo_idx,
		fifoctx.pkt_fifo.base.hiaddr, fifoctx.pkt_fifo.base.loaddr,
		fifoctx.pkt_fifo.curr_ptr, fifoctx.pkt_fifo.last_ptr,
		fifoctx.pkt_fifo.attrib, fifoctx.pkt_fifo.depth);
	HWA_BPRINT(b, "+ %2u fifoctx-aqm base<0x%08x,0x%08x> "
		"curr_ptr<0x%04x> last_ptr<0x%04x> attrib<%u> depth<%u>\n", fifo_idx,
		fifoctx.aqm_fifo.base.hiaddr, fifoctx.aqm_fifo.base.loaddr,
		fifoctx.aqm_fifo.curr_ptr, fifoctx.aqm_fifo.last_ptr,
		fifoctx.aqm_fifo.attrib, fifoctx.aqm_fifo.depth);
}

void
hwa_txfifo_dump_shadow(struct hwa_dev *dev, struct bcmstrbuf *b, uint32 fifo_idx)
{
	hwa_txfifo_t *txfifo = &dev->txfifo;

	if (fifo_idx >= txfifo->fifo_total || !isset(txfifo->fifo_enab, fifo_idx)) {
		HWA_BPRINT(b, "Wrong argument fifo_idx<%u:%u>\n",
			fifo_idx, txfifo->fifo_total);
		return;
	}

	_hwa_txfifo_dump_shadow32(txfifo, b, fifo_idx, FALSE);
}

void // HWA3b TxFifo: debug dump
hwa_txfifo_dump(hwa_txfifo_t *txfifo, struct bcmstrbuf *b,
	bool verbose, bool dump_regs, bool dump_txfifo_shadow,
	uint8 *fifo_bitmap)
{
	NO_HWA_PKTPGR_EXPR(hwa_dev_t *dev);

	HWA_BPRINT(b, "%s dump<%p>\n", HWA3b, txfifo);

	if (txfifo == (hwa_txfifo_t*)NULL)
		return;

	NO_HWA_PKTPGR_EXPR(hwa_ring_dump(&txfifo->pktchain_ring, b, "+ pktchain"));
	HWA_BPRINT(b, "+ requests<%u> fmt<%u>\n",
		txfifo->pktchain_id, txfifo->pktchain_fmt);
	HWA_BPRINT(b, "+ TxFIFO config<%u> total<%u>\n+ TxFIFO Enabled:\n",
		txfifo->fifo_config, txfifo->fifo_total);

	/* XXX, Sometime TxFIFO AQM context dump may cause AXI timeout when 3B is in trouble,
	 * so move registers/stats dump before TxFIFO AQM context dump.
	 */
#if defined(WLTEST) || defined(HWA_DUMP)
	if (dump_regs == TRUE)
		hwa_txfifo_regs_dump(txfifo, b);
#endif

	HWA_STATS_EXPR(
		HWA_BPRINT(b, "+ pktchain<%u> pkt_xmit<%u> tx_desc<%u>\n",
		          txfifo->pktchain_cnt, txfifo->pkt_xmit_cnt,
		          txfifo->tx_desc_cnt));

	if (verbose == TRUE) {
		hwa_txfifo_stats_dump(txfifo, b, /* clear */ 0);
	}

	if (verbose) {
		uint32 idx;
		uint32 *sys_mem;
		hwa_mem_addr_t axi_mem_addr;
		hwa_txfifo_ovflwqctx_t ovflwq;
		hwa_txfifo_fifoctx_t fifoctx;

		HWA_BPRINT(b, "+ Overflow Queue Context total<%u>\n", txfifo->fifo_total);
		sys_mem = &ovflwq.u32[0];
		for (idx = 0; idx < txfifo->fifo_total; idx++) {
			if (fifo_bitmap && !isset(fifo_bitmap, idx))
				continue;

			if (isset(txfifo->fifo_enab, idx)) {
				axi_mem_addr = HWA_TABLE_ADDR(hwa_txfifo_ovflwqctx_t,
					txfifo->ovflwq_addr, idx);
				HWA_RD_MEM32(HWA3b, hwa_txfifo_ovflwqctx_t,
					axi_mem_addr, sys_mem);
				HWA_BPRINT(b, "+ %2u ovflwq head<0x%08x,0x%08x> tail<0x%08x,0x%08x>"
					" count<%u,%u> hwm<%u,%u> append<%u>\n", idx,
					ovflwq.pktq_head.hiaddr, ovflwq.pktq_head.loaddr,
					ovflwq.pktq_tail.hiaddr, ovflwq.pktq_tail.loaddr,
					ovflwq.pkt_count, ovflwq.mpdu_count,
					ovflwq.pkt_count_hwm, ovflwq.mpdu_count_hwm,
					ovflwq.append_count);
				HWA_STATS_EXPR(HWA_BPRINT(b, " req<%u>\n", txfifo->req_cnt[idx]));
			}
		}

		// dump txfifo/aqm context
		HWA_BPRINT(b, "+ TxFIFO and AQM Context total<%u>\n", txfifo->fifo_total);

#if !defined(HWA_PKTPGR_BUILD) // PKTPGR set TXDMA_STOP_OVFLOWQ always true results 3B IDLE.
		// Don't access txfifoctx when 3B is disable
		dev = HWA_DEV(txfifo);
		if (hwa_module_request(dev, HWA_MODULE_TXFIFO, HWA_MODULE_IDLE, TRUE)) {
			HWA_BPRINT(b, "+  Ignore fifoctx (3B is disabled)\n");
			goto dump_shadow;
		}
#endif
		sys_mem = &fifoctx.u32[0];
		for (idx = 0; idx < txfifo->fifo_total; idx++) {
			if (fifo_bitmap && !isset(fifo_bitmap, idx))
				continue;

			if (isset(txfifo->fifo_enab, idx)) {
				axi_mem_addr = HWA_TABLE_ADDR(hwa_txfifo_fifoctx_t,
					txfifo->txfifo_addr, idx);
				HWA_RD_MEM32(HWA3b, hwa_txfifo_fifoctx_t,
					axi_mem_addr, sys_mem);
				HWA_BPRINT(b, "+  TX%2u fifoctx base<0x%08x,0x%08x> "
					"curr_ptr<0x%04x> last_ptr<0x%04x> attrib<%u> depth<%u>\n",
					idx,
					fifoctx.pkt_fifo.base.hiaddr, fifoctx.pkt_fifo.base.loaddr,
					fifoctx.pkt_fifo.curr_ptr, fifoctx.pkt_fifo.last_ptr,
					fifoctx.pkt_fifo.attrib, fifoctx.pkt_fifo.depth);
				HWA_BPRINT(b, "+ AQM%2u fifoctx base<0x%08x,0x%08x> "
					"curr_ptr<0x%04x> last_ptr<0x%04x> attrib<%u> depth<%u>\n",
					idx,
					fifoctx.aqm_fifo.base.hiaddr, fifoctx.aqm_fifo.base.loaddr,
					fifoctx.aqm_fifo.curr_ptr, fifoctx.aqm_fifo.last_ptr,
					fifoctx.aqm_fifo.attrib, fifoctx.aqm_fifo.depth);
			}
		}

NO_HWA_PKTPGR_EXPR(dump_shadow:);

		// dump shadow
		HWA_BPRINT(b, "+ Tx Shadow32 total<%u>\n", txfifo->fifo_total);
		for (idx = 0; idx < txfifo->fifo_total; idx++) {
			if (fifo_bitmap && !isset(fifo_bitmap, idx))
				continue;

			if (isset(txfifo->fifo_enab, idx)) {
				_hwa_txfifo_dump_shadow32(txfifo, b, idx, dump_txfifo_shadow);
			}
		}
	}
} // hwa_txfifo_dump

#if defined(WLTEST) || defined(HWA_DUMP)

void // HWA3b TxFifo: dump block registers
hwa_txfifo_regs_dump(hwa_txfifo_t *txfifo, struct bcmstrbuf *b)
{
	hwa_dev_t *dev;
	hwa_regs_t *regs;

	if (txfifo == (hwa_txfifo_t*)NULL)
		return;

	dev = HWA_DEV(txfifo);
	regs = dev->regs;

	HWA_BPRINT(b, "%s registers[0x%p] offset[0x%04x]\n",
		HWA3b, &regs->txdma, OFFSETOF(hwa_regs_t, txdma));

	HWA_BPR_REG(b, txdma, hwa_txdma2_cfg1);
	HWA_BPR_REG(b, txdma, hwa_txdma2_cfg2);
	HWA_BPR_REG(b, txdma, hwa_txdma2_cfg3);
	HWA_BPR_REG(b, txdma, hwa_txdma2_cfg4);
	HWA_BPR_REG(b, txdma, state_sts);
	HWA_BPR_REG(b, txdma, state_sts2);
	HWA_BPR_REG(b, txdma, state_sts3);
	HWA_BPR_REG(b, txdma, sw2hwa_tx_pkt_chain_q_base_addr_lo);
	HWA_BPR_REG(b, txdma, sw2hwa_tx_pkt_chain_q_base_addr_hi);
	HWA_BPR_REG(b, txdma, sw2hwa_tx_pkt_chain_q_wr_index);
	HWA_BPR_REG(b, txdma, sw2hwa_tx_pkt_chain_q_rd_index);
	HWA_BPR_REG(b, txdma, sw2hwa_tx_pkt_chain_q_ctrl);
	HWA_BPR_REG(b, txdma, fifo_index);
	HWA_BPR_REG(b, txdma, fifo_base_addr_lo);
	HWA_BPR_REG(b, txdma, fifo_base_addr_hi);
	HWA_BPR_REG(b, txdma, fifo_wr_index);
	HWA_BPR_REG(b, txdma, fifo_rd_index);
	HWA_BPR_REG(b, txdma, fifo_depth);
	HWA_BPR_REG(b, txdma, fifo_attrib);
	HWA_BPR_REG(b, txdma, aqm_base_addr_lo);
	HWA_BPR_REG(b, txdma, aqm_base_addr_hi);
	HWA_BPR_REG(b, txdma, aqm_wr_index);
	HWA_BPR_REG(b, txdma, aqm_rd_index);
	HWA_BPR_REG(b, txdma, aqm_depth);
	HWA_BPR_REG(b, txdma, aqm_attrib);
	HWA_BPR_REG(b, txdma, sw_tx_pkt_nxt_h);
	HWA_BPR_REG(b, txdma, dma_desc_template_txdma);
#ifdef HWA_PKTPGR_BUILD
	if (HWAREV_GE(dev->corerev, 131)) {
		HWA_BPR_REG(b, txdma, pp_pageout_cfg);
		HWA_BPR_REG(b, txdma, pp_pageout_sts);
		HWA_BPR_REG(b, txdma, pp_pagein_cfg);
		HWA_BPR_REG(b, txdma, pp_pagein_sts);
		HWA_BPR_REG(b, txdma, pp_pageout_dataptr_th);
	}
	if (HWAREV_GE(dev->corerev, 133)) {
		HWA_BPR_REG(b, txdma, pp_page_debug);
		HWA_BPR_REG(b, txdma, pp_txdma_dma_template);
	}
#endif /* HWA_PKTPGR_BUILD */
} // hwa_txfifo_regs_dump

#endif

#endif /* BCMDBG */

#endif /* HWA_TXFIFO_BUILD */

#if defined(HWA_TXSTAT_BUILD) || defined(STS_XFER_TXS_PP)
/**
 * Read a 16 byte status package from the TxStatus.
 * The first word has a valid bit that indicates if the fifo had
 * a valid entry or if the fifo was empty.
 *
 * @return int BCME_OK if the entry was valid, BCME_NOTREADY if
 *         the TxStatus fifo was empty, BCME_NODEVICE if the
 *         read returns 0xFFFFFFFF indicating a target abort
 */
static int BCMFASTPATH
hwa_txstat_read_txs_pkg16(wlc_info_t *wlc, wlc_txs_pkg16_t *txs)
{
	uint32 s1;

	s1 = txs->word[0];

	if ((s1 & TX_STATUS_VALID) == 0) {
		return BCME_NOTREADY;
	}
	/* Invalid read indicates a dead chip */
	if (s1 == 0xFFFFFFFF) {
		return BCME_NODEVICE;
	}

#if defined(WL_TXS_LOG)
	wlc_bmac_txs_hist_pkg16(wlc->hw, txs);
#endif /* WL_TXS_LOG */

	return BCME_OK;
}

#endif /* HWA_TXSTAT_BUILD || STS_XFER_TXS_PP */

#ifdef HWA_TXSTAT_BUILD
/*
 * -----------------------------------------------------------------------------
 * Section: HWA4a TxSTAT block bringup: attach, detach, and init phases
 * -----------------------------------------------------------------------------
 */

static int hwa_txstat_bmac_proc(void *context, uintptr arg1, uintptr arg2,
	uint32 core, uint32 arg4);
static int hwa_txstat_bmac_done(void *context, uintptr arg1, uintptr arg2,
	uint32 core, uint32 proc_cnt);

// HWA4a TxStat block level management
void
BCMATTACHFN(hwa_txstat_detach)(hwa_txstat_t *txstat) // txstat may be NULL
{
	void *memory;
	uint32 core, mem_sz;
	hwa_dev_t *dev;

	HWA_FTRACE(HWA4a);

	if (txstat == (hwa_txstat_t*)NULL)
		return; // nothing to release done

	// Audit pre-conditions
	dev = HWA_DEV(txstat);

	// HWA4a TxStat Status Ring: free memory and reset ring
	for (core = 0; core < HWA_TX_CORES_MAX; core++) {
		if (txstat->status_ring[core].memory != (void*)NULL) {
			HWA_ASSERT(txstat->status_size[core] != 0U);
			memory = txstat->status_ring[core].memory;
			mem_sz = txstat->status_ring[core].depth
			         * txstat->status_size[core];
			HWA_TRACE(("%s HWA[%d] status_ring[%u] -memory[%p:%u]\n",
				HWA4a, dev->unit, core, memory, mem_sz));
			MFREE(dev->osh, memory, mem_sz);
			hwa_ring_fini(&txstat->status_ring[core]);
			txstat->status_ring[core].memory = (void*)NULL;
			txstat->status_size[core] = 0U;
		}
	}

	return;

} // hwa_txstat_detach

hwa_txstat_t *
BCMATTACHFN(hwa_txstat_attach)(hwa_dev_t *dev)
{
	void *memory;
	uint32 core, mem_sz, depth;
	hwa_regs_t *regs;
	hwa_txstat_t *txstat;

	HWA_FTRACE(HWA4a);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	regs = dev->regs;
	txstat = &dev->txstat;

	// Allocate the H2S txstatus interface
	for (core = 0; core < HWA_TX_CORES; core++) {
		txstat->status_size[core] = HWA_TXSTAT_RING_ELEM_SIZE;
		depth = HWA_TXSTAT_QUEUE_DEPTH;
		mem_sz = depth * txstat->status_size[core];

		// Let's force to 8B alignment in case the fastdma is set.
		if ((memory = MALLOC_ALIGN(dev->osh, mem_sz, 3)) == NULL) {
			HWA_ERROR(("%s HWA[%d] status_ring<%u> malloc size<%u> failure\n",
				HWA4a, dev->unit, core, mem_sz));
			HWA_ASSERT(memory != (void*)NULL);
			goto failure;
		}
		ASSERT(ISALIGNED(memory, 8));
		bzero(memory, mem_sz);
		OSL_CACHE_FLUSH(memory, mem_sz);

		HWA_TRACE(("%s HWA[%d] status_ring<%u> +memory[%p,%u]\n",
			HWA4a, dev->unit, core, memory, mem_sz));
		hwa_ring_init(&txstat->status_ring[core], "TXS", HWA_TXSTAT_ID,
			HWA_RING_H2S, HWA_TXSTAT_QUEUE_H2S_RINGNUM, depth, memory,
			&regs->tx_status[core].tseq_wridx,
			&regs->tx_status[core].tseq_rdidx);
	} // for core

	// Initialize the TxStat memory AXI memory address
	txstat->txstat_addr = hwa_axi_addr(dev, HWA_AXI_TXSTAT_MEMORY);
	HWA_TRACE(("%s HWA[%d] AXI memory address <0x%p>\n",
		HWA4a, dev->unit, (void *)(txstat->txstat_addr)));

#if defined(HWA_PKTPGR_BUILD)
	txstat->elem_ix_bad = -1;
#else
	// Registered dpc callback handler
	hwa_register(dev, HWA_TXSTAT_PROC_CB, dev, hwa_txstat_bmac_proc);
	hwa_register(dev, HWA_TXSTAT_DONE_CB, dev, hwa_txstat_bmac_done);
#endif

	return txstat;

failure:
	hwa_txstat_detach(txstat);
	HWA_WARN(("%s attach failure\n", HWA4a));

	return ((hwa_txstat_t*)NULL);

} // hwa_txstat_attach

void // HWA4a TxStatus block initialization
hwa_txstat_init(hwa_txstat_t *txstat)
{
	uint32 v32, core;
	hwa_dev_t *dev;
	hwa_regs_t *regs;
	void *memory;
	uint16 v16;

	HWA_FTRACE(HWA4a);

	// Audit pre-conditions
	dev = HWA_DEV(txstat);

	// Setup locals
	regs = dev->regs;

	for (core = 0; core < HWA_TX_CORES; core++) {
		v32 = (0U
		// PCIE as source | destination: NotPcie = 0, Coh = host, AddrExt = 0b00
		//| BCM_SBIT(HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_DMAPCIEDESCTEMPLATENOTPCIE)
		| BCM_SBF(dev->host_coherency,
			HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_DMAPCIEDESCTEMPLATECOHERENT)
		| BCM_SBF(0U,
			HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_DMAPCIEDESCTEMPLATEADDREXT)
		// HWA local memory as source or address FIXME Typo in regs doc
		//| BCM_SBIT(HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_DMAHWADESCTEMPLATENOTPCIE)
		| BCM_SBIT(HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_DMAHWADESCTEMPLATECOHERENT)
		//| BCM_SBIT(HWA_TX_STATUS_DMA_DESC_TEMPLATE_TXS_NONDMAHWADESCTEMPLATECOHERENT)
		| 0U);
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core],
			dma_desc_template_txs, v32);

		// XXX CRWLHWA-454
		// Access mode
		// 0: get MAC TX status with APB interface. 1: get MAX TX status with AXI interface.
		v32 = BCM_SBF((dev->txstat_apb == 0) ? 1 : 0,
			HWA_TX_STATUS_TXE_CFG1_ACCESSMODE);
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core], txe_cfg1, v32);

		// Initialize the H2S txstatus interface
		memory = txstat->status_ring[core].memory;
		v32 = HWA_PTR2UINT(memory);
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core], tseq_base_lo, v32);
		v32 = HWA_PTR2HIADDR(memory); // hiaddr for NIC mode 64b hosts
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core], tseq_base_hi, v32);

		v32 = HWA_TXSTAT_QUEUE_DEPTH;
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core], tseq_size, v32);
		v32 = (0U
			| BCM_SBF(HWA_TXSTAT_RING_ELEM_SIZE,
				HWA_TX_STATUS_TSE_CTL_MACTXSTATUSSIZE)
			| BCM_SBF(HWA_TXSTAT_INTRAGGR_TMOUT,
				HWA_TX_STATUS_TSE_CTL_LAZYINTRTIMEOUT)
			| BCM_SBF(HWA_TXSTAT_INTRAGGR_COUNT,
				HWA_TX_STATUS_TSE_CTL_LAZYCOUNT)
			| 0U);
		HWA_WR_REG_NAME(HWA4a, regs, tx_status[core], tse_ctl, v32);
	} // for core

	// Enable Req-Ack based MAC_HWA i/f is enabled for txStatus.
	v16 = BCM_SBIT(_HWA_MACIF_CTL_TXSTATUSEN);
	v16 |= BCM_SBF(HWA_TXSTAT_RING_ELEM_SIZE/HWA_TXSTAT_PKG_SIZE,
		_HWA_MACIF_CTL_TXSTATUS_COUNT);
	// XXX CRWLHWA-454:
	// 0: get MAC TX status with APB interface. 1: get MAX TX status with AXI interface.
	if (dev->txstat_apb == 0) {
		v16 |= BCM_SBIT(_HWA_MACIF_CTL_TXSTATUSMEM);
	} else {
		v16 = BCM_CBIT(v16, _HWA_MACIF_CTL_TXSTATUSMEM);
	}

	HWA_UPD_REG16_ADDR(HWA4a, &dev->mac_regs->hwa_macif_ctl, v16, _HWA_MACIF_CTL_TXSTATUS_XFER);

	// Assign the txstat interrupt mask.
	dev->defintmask |= HWA_COMMON_INTSTATUS_TXSWRINDUPD_INT_CORE0_MASK;
	HWA_ASSERT(HWA_TX_CORES == 1);

} // hwa_txstat_init

void // HWA4a TxStatus block deinitialization
hwa_txstat_deinit(hwa_txstat_t *txstat)
{
	hwa_dev_t *dev;

	HWA_FTRACE(HWA4a);

	// Audit pre-conditions
	dev = HWA_DEV(txstat);

	// Disable Req-Ack based MAC_HWA i/f for txStatus.
	HWA_UPD_REG16_ADDR(HWA4a, &dev->mac_regs->hwa_macif_ctl, 0, _HWA_MACIF_CTL_TXSTATUSEN);

}

void
hwa_txstat_wait_to_finish(hwa_txstat_t *txstat, uint32 core)
{
	hwa_dev_t *dev;
	uint32 v32, loop_count;
	uint32 busy, start_curstate, num_tx_status_count;

	HWA_FTRACE(HWA4a);

	HWA_ASSERT(core < HWA_TX_CORES);

	// Audit pre-conditions
	dev = HWA_DEV(txstat);

	// Poll txs_debug_reg to make sure it's done
	loop_count = 0;
	do {
		if (loop_count)
			OSL_DELAY(1);
		v32 = HWA_RD_REG_NAME(HWA4a, dev->regs, tx_status[core], txs_debug_reg);
		start_curstate = BCM_GBF(v32, HWA_TX_STATUS_TXS_DEBUG_REG_START_CURSTATE);
		num_tx_status_count = BCM_GBF(v32, HWA_TX_STATUS_TXS_DEBUG_REG_NUM_TX_STATUS_COUNT);
		busy = (start_curstate > 1) | (num_tx_status_count != 0);
		HWA_TRACE(("%s HWA[%d] Polling txs_debug_reg <%u:%u>\n", __FUNCTION__, dev->unit,
			start_curstate, num_tx_status_count));
	} while (busy && ++loop_count != HWA_FSM_IDLE_POLLLOOP);
	if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
		HWA_WARN(("%s HWA[%d] txs_debug_reg is not idle <%u:%u>\n", __FUNCTION__,
			dev->unit, start_curstate, num_tx_status_count));
		txstat->status_stall = TRUE;
	}
}

void // HWA4a TxStatus block reclaim
hwa_txstat_reclaim(hwa_dev_t *dev, uint32 core, bool reinit)
{
	hwa_txstat_t *txstat; // SW txstat state
	hwa_ring_t *h2s_ring; // H2S TxStatus status_ring
	NO_HWA_PKTPGR_EXPR(wlc_info_t *wlc);
	NO_HWA_PKTPGR_EXPR(bool fatal);
	NO_HWA_PKTPGR_EXPR(hwa_handler_t *proc_handler); // upstream per TxStatus handler
	NO_HWA_PKTPGR_EXPR(hwa_handler_t *done_handler); // upstream all TxStatus done handler
	hwa_module_block_t blk = HWA_MODULE_TXSTAT0;

	HWA_FTRACE(HWA4a);
	HWA_ASSERT(core == 0);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	txstat = &dev->txstat;
	h2s_ring = &txstat->status_ring[core];

	// Fetch registered upstream callback handler
	NO_HWA_PKTPGR_EXPR({
		proc_handler = &dev->handlers[HWA_TXSTAT_PROC_CB];
		done_handler = &dev->handlers[HWA_TXSTAT_DONE_CB];
	});

	// Make sure 4a's job is done.
	hwa_txstat_wait_to_finish(txstat, core);

	// Don't reset HWA 4a for normal cases.
	if (!reinit && !txstat->status_stall) {
#if defined(HWA_PKTPGR_BUILD)
		// If DDBM resource is not enought to consume all TxS.
		// 1. Do PageIn TxS for part of the TxS
		// 2. Wait for PageIn TxS finished.
		// 3. Continue step 1 if there is unprocessed TxS.
		uint32 loop = HWA_TXS_CONSUME_POLLLOOP;
		uint32 pgi_txs_pending = 0;
		hwa_pktpgr_t *pktpgr = &dev->pktpgr;

		// Reset it.
		PGIEMERRESETTXSRECLAIM(pktpgr);

		// Drain DBM in TxCpl(pciedev related).
		hwa_pktpgr_txcpl_sync(dev);

		// Process pending PGI txstatus resp to drain more DBM.
		if (pktpgr->pgi_txs_req_tot) {
			hwa_pktpgr_txstatus_wait_to_finish(dev, -1);
			hwa_pktpgr_txcpl_sync(dev);
		}

		// Consume all TxS.
		while (hwa_txstat_process(dev, 0, HWA_PROCESS_NOBOUND)) {
			// Use SHARED DBM(ddbm_avail_sw) first.
			if (pktpgr->pgi_txs_req_tot == 0) {
				// SHARED DBM is not enough, use RESV DBM.
				PGIEMERSETTXSRECLAIM(pktpgr);
				HWA_INFO(("   Using RESV DBM\n"));
			} else {
				// Use SHARED DBM(ddbm_avail_sw) to consume it piece by piece.
				// In above hwa_txstat_process, all 4A-TxS are convert to PageIn TxS
				// req in PageIn Req Ring, wait for all TxS Rsp process finished.
				loop = HWA_TXS_CONSUME_POLLLOOP;

				PGIEMERRESETTXSRECLAIM(pktpgr);

				hwa_pktpgr_txcpl_sync(dev);
				hwa_pktpgr_txstatus_wait_to_finish(dev, -1);

				// Check pgi_txs_req_tot to detect dead loop due to PGI stuck.
				if (pktpgr->pgi_txs_req_tot != 0) {
					pgi_txs_pending++;
				} else {
					pgi_txs_pending = 0;
				}
				HWA_ASSERT(pgi_txs_pending < HWA_TXS_CONSUME_POLLLOOP);
			}

			loop--;
			if (loop == 0) {
				HWA_WARN(("%s HWA[%d] part of TxS are not consumed\n",
					__FUNCTION__, dev->unit));
				HWA_RING_STATE(h2s_ring)->read = HWA_RING_STATE(h2s_ring)->write;
				txstat->elem_ix_ack = HWA_RING_STATE(h2s_ring)->read;
				txstat->elem_ix_bad = -1;
				// Commit specific RD index
				hwa_ring_cons_put_rd(h2s_ring, txstat->elem_ix_ack);
				break;
			}
		}

		// Wait until txstatus in Resp are done.
		hwa_pktpgr_txcpl_sync(dev);
		hwa_pktpgr_txstatus_wait_to_finish(dev, -1);
#else  /* !HWA_PKTPGR_BUILD */
		(void)hwa_txstat_process(dev, 0, HWA_PROCESS_NOBOUND);
#endif /* !HWA_PKTPGR_BUILD */
		return;
	}

	HWA_ERROR(("%s %s: reinit<%d> stall<%d>\n", HWA4a, __FUNCTION__,
		reinit, txstat->status_stall));

	// Deinit 4a
	hwa_txstat_deinit(txstat);

	// Disable 4a block
	hwa_module_request(dev, blk, HWA_MODULE_ENABLE, FALSE);

	// Process remain txstatus in H2S TxStatus status_ring
	(void)hwa_txstat_process(dev, 0, HWA_PROCESS_NOBOUND);

#if !defined(HWA_PKTPGR_BUILD)

	// Setup locals
	wlc = (wlc_info_t *)dev->wlc;

	// Process stall txstatus
	// XXX, Actually in real test, I don't think it can recover the bad state.
	if (txstat->status_stall) {
		void *txstatus;
		wlc_txs_pkg16_t pkg;
		hwa_txstat_status_t txs_cache;
		uint8 pkg_len;

		// Setup locals
		pkg_len = sizeof(wlc_txs_pkg16_t);
		fatal = FALSE;

		HWA_RD_MEM32(HWA4a, wlc_txs_pkg16_t, txstat->txstat_addr, &pkg);
		if (pkg.word[0] & TX_STATUS40_FIRST) {
			// pkg 1
			bcopy(&pkg, &txs_cache, pkg_len);
			// pkg 2
			wlc_bmac_read_txs_pkg16(wlc->hw, &pkg);
			bcopy(&pkg, ((uint8 *)&txs_cache) + pkg_len, pkg_len);
			prhex("F", (uint8 *)&txs_cache, sizeof(txs_cache));
		} else {
			// pkg 1
			HWA_ASSERT(hwa_ring_is_empty(h2s_ring));
			// The next txs will be placed at read index
			txstatus = HWA_RING_ELEM(hwa_txstat_status_t, h2s_ring,
				HWA_RING_STATE(h2s_ring)->read);
			bcopy(txstatus, &txs_cache, pkg_len);
			// pkg 2
			bcopy(&pkg, ((uint8 *)&txs_cache) + pkg_len, pkg_len);
			prhex("S", (uint8 *)&txs_cache, sizeof(txs_cache));
		}
		// Invoke upstream bound handler
		(*proc_handler->callback)(proc_handler->context,
			(uintptr)&txs_cache, (uintptr)&fatal, core, 0);
		HWA_STATS_EXPR(txstat->proc_cnt[core] += 1);
		(*done_handler->callback)(done_handler->context,
			(uintptr)&fatal, (uintptr)0, core, 1);
		txstat->status_stall = FALSE;
	}

	// Process remain txstatus
	wlc_bmac_txstatus(wlc->hw, FALSE, &fatal);

#else

	// In above hwa_txstat_process, all 4A-TxS are convert to PageIn TxS req in
	// PageIn Req Ring, wait for all TxS Rsp process finished.
	hwa_pktpgr_txstatus_wait_to_finish(dev, -1);

	// Reset variables
	txstat->elem_ix_bad = -1;
	txstat->elem_ix_ack = 0;
#endif /* !HWA_PKTPGR_BUILD */

	// Reset 4a block
	hwa_module_request(dev, blk, HWA_MODULE_RESET, TRUE);
	hwa_module_request(dev, blk, HWA_MODULE_RESET, FALSE);
	HWA_RING_STATE(h2s_ring)->write = 0;
	HWA_RING_STATE(h2s_ring)->read = 0;

	// Enable 4a block
	hwa_module_request(dev, blk, HWA_MODULE_ENABLE, TRUE);

	// Init 4a
	hwa_txstat_init(txstat);

}

int BCMFASTPATH // Consume all txstatus in H2S txstatus interface
hwa_txstat_process(struct hwa_dev *dev, uint32 core, bool bound)
{
	int ret;
	uint32 proc_cnt;
	uint32 elem_ix; // location of next element to read
	void *txstatus; // MAC generate txstatus blob
	hwa_txstat_t *txstat; // SW txstat state
	hwa_ring_t *h2s_ring; // H2S TxStatus status_ring
#if defined(HWA_PKTPGR_BUILD)
	int elem_ix_pend; // location of next pend element to read
#else
	hwa_handler_t *proc_handler; // upstream per TxStatus handler
	hwa_handler_t *done_handler; // upstream all TxStatus done handler
	bool fatal = FALSE;
#endif
	wlc_info_t *wlc;

	HWA_FTRACE(HWA4a);

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(core < HWA_TX_CORES);

	// Setup locals
	proc_cnt = 0;
	txstat = &dev->txstat;
	h2s_ring = &txstat->status_ring[core];
	wlc = (wlc_info_t *)dev->wlc;

	// Fetch registered upstream callback handler
	NO_HWA_PKTPGR_EXPR({
		proc_handler = &dev->handlers[HWA_TXSTAT_PROC_CB];
		done_handler = &dev->handlers[HWA_TXSTAT_DONE_CB];
	});

	HWA_STATS_EXPR(txstat->wake_cnt[core]++);

	if (HWAREV_GE(dev->corerev, 130)) {
		// fetch HWA txstatus ring's WR index once
		hwa_ring_cons_get(h2s_ring);
	} else {
		uint32 loop_count = 0;

		// XXX, CRBCAHWA-592
		// SW probably read the value 0x400 for write index when set the depth to 0x400.
		// Because write index will be updated from 0x3ff to 0x0 through 0x400 in a cycle.
		// This value is invalid. Driver should read it again to fecth the real one.
		do {
			if (loop_count) {
				HWA_TRACE(("%s: WR index is equal to depth <0x%x>\n",
					__FUNCTION__, HWA_RING_STATE(h2s_ring)->write));
				OSL_DELAY(1);
			}

			hwa_ring_cons_get(h2s_ring); // fetch HWA txstatus ring's WR index
		} while ((HWA_RING_STATE(h2s_ring)->write == h2s_ring->depth) &&
			++loop_count != HWA_FSM_IDLE_POLLLOOP);
		if (loop_count == HWA_FSM_IDLE_POLLLOOP) {
			HWA_ERROR(("%s: WR index still invalid <0x%x>\n",
				__FUNCTION__, HWA_RING_STATE(h2s_ring)->write));
			HWA_ASSERT(0);
		}
	}

	/* Perform bulk cache inv */
	HWA_BULK_CACHE_INV(hwa_txstat_status_t, h2s_ring);

	// Consume all TxStatus received in status_ring, handing each upstream
#if defined(HWA_PKTPGR_BUILD)
	/* Process steps
	 * 1. Got TxS notification, read h2s_ring for txstatus packages
	 * 2. Process txstatus ncons, fifo for HWA_PP_PAGEIN_TXSTATUS
	 * 3. Don't commit h2s_ring RD index here because we need txstatus
	 *      packages in hwa_txstat_pagein_rsp.
	 * 4. hwa_txstat_pagein_rsp will commot h2s_ring RD index.
	 */

	// Use hwa_ring_cons_pend, because hwa_txstat_pagein_req could return error.
	while ((elem_ix = hwa_ring_cons_pend(h2s_ring, &elem_ix_pend)) != BCM_RING_EMPTY) {
#else
	while ((elem_ix = hwa_ring_cons_upd(h2s_ring)) != BCM_RING_EMPTY) {
#endif
		txstatus = HWA_RING_ELEM(hwa_txstat_status_t, h2s_ring, elem_ix);

		// XXX: FIXME: we should check wlc->pub->up in hwa_dpc
		if (!wlc->hw->up) {
			ret = HWA_FAILURE;
			break;
		}

		// Invoke upstream bound handler
#if defined(HWA_PKTPGR_BUILD)

		// Process txstatus ncons, fifo for HWA_PP_PAGEIN_TXSTATUS
		ret = hwa_txstat_pagein_req(dev, txstatus, elem_ix);

		// pagein_req_ring full
		if (ret == BCME_NORESOURCE) {
			// BCME_NORESOURCE, ignore hwa_ring_cons_done and break.
			ret = HWA_FAILURE;

			// set elem_ix_pend to previous
			elem_ix_pend = elem_ix;
		} else {
			// BCME_OK or BCME_ERROR

			// Increase proc_cnt
			proc_cnt++;

			// Commit pending SW read
			hwa_ring_cons_done(h2s_ring, elem_ix_pend);
		}

#else

		ret = (*proc_handler->callback)(proc_handler->context,
			(uintptr)txstatus, (uintptr)&fatal, core, 0);

		proc_cnt++;

#endif /* HWA_PKTPGR_BUILD */

		// Callback cannot handle it, break the loop.
		if (ret != HWA_SUCCESS)
			break;

		if ((proc_cnt % HWA_TXSTAT_LAZY_RD_UPDATE) == 0) {
			if (bound) {
				break;
			} else {
				// PKTPGR: delay commit until hwa_txstat_pagein_rsp
				// commit RD index lazily
				NO_HWA_PKTPGR_EXPR(hwa_ring_cons_put(h2s_ring));
			}
		}
	}

	// PKTPGR: delay commit until hwa_txstat_pagein_rsp
	NO_HWA_PKTPGR_EXPR(hwa_ring_cons_put(h2s_ring)); // commit RD index now

	if (proc_cnt) {
		HWA_STATS_EXPR(txstat->proc_cnt[core] += proc_cnt);
		NO_HWA_PKTPGR_EXPR({
		(*done_handler->callback)(done_handler->context,
			(uintptr)&fatal, (uintptr)0, core, proc_cnt);
		});
	}

	if (!hwa_ring_is_empty(h2s_ring)) {
		/* need re-schdeule */
		HWA_ASSERT(core == 0);
		dev->intstatus |= HWA_COMMON_INTSTATUS_TXSWRINDUPD_INT_CORE0_MASK;
	}

	return ret;
}

/*
 * This is HWA TX handle function to process txstatus.
 * NOTE: FIXME: for now this function only consider FD mode.
 */
static int BCMFASTPATH
hwa_txstat_bmac_proc(void *context, uintptr arg1, uintptr arg2, uint32 core, uint32 arg4)
{
	hwa_dev_t *dev = (hwa_dev_t *)context;
	wlc_info_t *wlc = (wlc_info_t *)dev->wlc;
	wlc_txs40_status_t *txs40_status = (wlc_txs40_status_t *)arg1;
	bool *fatal = (bool *)arg2;
	osl_t *osh = dev->osh;
	int txserr = BCME_OK;
	tx_status_t txs;
	/* pkg 1 */
	uint16 status_bits;
	uint16 ncons;

	HWA_FTRACE(HWA4a);
	HWA_ASSERT(context != (void *)NULL);
	HWA_ASSERT(arg1 != 0);
	HWA_ASSERT(arg2 != 0);

	if (txs40_status == NULL) {
		HWA_ERROR(("%s %s(%d) HWA[%d] txstatus is NULL!!!\n",
			HWA4a, __FUNCTION__, __LINE__, dev->unit));
		return HWA_FAILURE;
	}

	/* Link Tx Status Ring buffer to tx_status_t */
	txs.status.mactxs = (tx_status_mactxs_t *) txs40_status;

	/* Get pkg 1 */
	if ((txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[0])) == BCME_OK) {

		HWA_TRACE(("%s:HWA[%d] s1=%0x ampdu=%d\n", __FUNCTION__, dev->unit,
			TX_STATUS_MACTXS_S1(&txs),
			(TX_STATUS_MACTXS_S1(&txs) & TX_STATUS_AMPDU)));

		txs.frameid = (TX_STATUS_MACTXS_S1(&txs) & TX_STATUS_FID_MASK) >>
					TX_STATUS_FID_SHIFT;
		txs.sequence = TX_STATUS_MACTXS_S2(&txs) & TX_STATUS_SEQ_MASK;
		txs.phyerr = TX_STATUS_PTX(TX_STATUS_MACTXS_S2(&txs), wlc->pub->corerev);
		txs.lasttxtime = R_REG(osh, D11_TSFTimerLow(wlc));
		status_bits = TX_STATUS_MACTXS_S1(&txs) & TXS_STATUS_MASK;
		txs.status.raw_bits = status_bits;
		txs.status.is_intermediate = (status_bits & TX_STATUS40_INTERMEDIATE) != 0;
		txs.status.pm_indicated = (status_bits & TX_STATUS40_PMINDCTD) != 0;

		ncons = ((status_bits & TX_STATUS40_NCONS) >> TX_STATUS40_NCONS_SHIFT);
		txs.status.was_acked = ((ncons <= 1) ?
			((status_bits & TX_STATUS40_ACK_RCV) != 0) : TRUE);
		txs.status.suppr_ind =
				(status_bits & TX_STATUS40_SUPR) >> TX_STATUS40_SUPR_SHIFT;

#ifdef BCM_BUZZZ
		BUZZZ_KPI_PKT1(KPI_PKT_MAC_TXSTAT, 2,
			ncons, D11_TXFID_GET_FIFO(wlc, txs.frameid));
		BUZZZ_KPI_QUE1(KPI_QUE_MAC_RD_UPD, 2,
			ncons, D11_TXFID_GET_FIFO(wlc, txs.frameid));
#endif /* BCM_BUZZZ */

		/* pkg 2 comes always */
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[1]);
		/* store saved extras (check valid pkg) */
		if (txserr != BCME_OK) {
			/* if not a valid package, assert and bail */
			HWA_ERROR(("wl%d: %s: package 2 read not valid\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}

		HWA_TRACE(("wl%d: %s calls dotxstatus\n", wlc->pub->unit, __FUNCTION__));

		HWA_PTRACE(("wl%d: %s:: Raw txstatus %08X %08X %08X %08X "
			"%08X %08X %08X %08X\n",
			wlc->pub->unit, __FUNCTION__,
			TX_STATUS_MACTXS_S1(&txs), TX_STATUS_MACTXS_S2(&txs),
			TX_STATUS_MACTXS_S3(&txs), TX_STATUS_MACTXS_S4(&txs),
			TX_STATUS_MACTXS_S5(&txs), TX_STATUS_MACTXS_ACK_MAP1(&txs),
			TX_STATUS_MACTXS_ACK_MAP2(&txs), TX_STATUS_MACTXS_S8(&txs)));

#if defined(TX_STATUS_MACTXS_64BYTES)
		/* pkg 3 comes always */
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[2]);
		/* store saved extras (check valid pkg) */
		if (txserr != BCME_OK) {
			/* if not a valid package, assert and bail */
			HWA_ERROR(("wl%d: %s: package 3 read not valid\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}

		/* pkg 4 comes always */
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[3]);
		/* store saved extras (check valid pkg) */
		if (txserr != BCME_OK) {
			/* if not a valid package, assert and bail */
			HWA_ERROR(("wl%d: %s: package 4 read not valid\n",
				wlc->pub->unit, __FUNCTION__));
			goto done;
		}

		HWA_PTRACE(("Pkg3: %08X %08X %08X %08X\n",
			TX_STATUS_MACTXS_S9(&txs), TX_STATUS_MACTXS_S10(&txs),
			TX_STATUS_MACTXS_S11(&txs), TX_STATUS_MACTXS_S12(&txs)));
		HWA_PTRACE(("Pkg4: %08X %08X %08X %08X\n",
			TX_STATUS_MACTXS_S13(&txs), TX_STATUS_MACTXS_S14(&txs),
			TX_STATUS_MACTXS_S15(&txs), TX_STATUS_MACTXS_S16(&txs)));

#endif /* TX_STATUS_MACTXS_64BYTES */

		txs.status.rts_tx_cnt = ((TX_STATUS_MACTXS_S5(&txs) & TX_STATUS40_RTS_RTX_MASK) >>
								 TX_STATUS40_RTS_RTX_SHIFT);
		txs.status.cts_rx_cnt = ((TX_STATUS_MACTXS_S5(&txs) & TX_STATUS40_CTS_RRX_MASK) >>
								 TX_STATUS40_CTS_RRX_SHIFT);

		if ((TX_STATUS_MACTXS_S5(&txs) & TX_STATUS64_MUTX)) {
			/* Only RT0 entry is used for frag_tx_cnt in ucode */
			txs.status.frag_tx_cnt = TX_STATUS40_TXCNT_RT0(TX_STATUS_MACTXS_S3(&txs));
		} else {
			/* XXX: Need to be recalculated to "TX_STATUS_MACTXS_S3(&txs) & 0xffff"
			 * if this tx was fixed rate.
			 * The recalculation is done in wlc_dotxstatus() as we need
			 * TX descriptor from pkt ptr to know if it was fixed rate or not.
			 */
			txs.status.frag_tx_cnt = TX_STATUS40_TXCNT(TX_STATUS_MACTXS_S3(&txs),
					TX_STATUS_MACTXS_S4(&txs));
		}

		*fatal = wlc_bmac_dotxstatus(wlc->hw, &txs, TX_STATUS_MACTXS_S2(&txs));
	}

done:
	if (txserr == BCME_NODEVICE) {
		HWA_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		HWA_ASSERT(TX_STATUS_MACTXS_S5(&txs) != 0xffffffff);
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return HWA_FAILURE;
	}

	if (*fatal) {
		HWA_ERROR(("HWA[%d] error %d caught in %s\n", dev->unit, *fatal, __FUNCTION__));
		return HWA_FAILURE;
	}

	return HWA_SUCCESS;
}

/*
 * This function will be called when we finished hwa_txstat_process() .
 * You can add some post processes in function if needed.
 * NOTE: FIXME: for now this function only consider FD mode.
 */
static int BCMFASTPATH
hwa_txstat_bmac_done(void *context, uintptr arg1, uintptr arg2, uint32 core, uint32 proc_cnt)
{
	hwa_dev_t *dev = (hwa_dev_t *)context;
	wlc_info_t *wlc = (wlc_info_t *)dev->wlc;
	bool *fatal = (bool *)arg1;

	HWA_TRACE(("%s %s():HWA[%d]: core<%u> proc_cnt<%u>\n", HWA4a, __FUNCTION__,
		dev->unit, core, proc_cnt));

	if (*fatal) {
		HWA_ERROR(("wl%d: %s HAMMERING fatal txs err\n",
			wlc->pub->unit, __FUNCTION__));
		wlc_check_assert_type(wlc, WL_REINIT_RC_INV_TX_STATUS);
		return HWA_FAILURE;
	}

	if (wlc->active_queue != NULL && WLC_TXQ_OCCUPIED(wlc)) {
		WLDURATION_ENTER(wlc, DUR_DPC_TXSTATUS_SENDQ);
		wlc_send_q(wlc, wlc->active_queue);
		WLDURATION_EXIT(wlc, DUR_DPC_TXSTATUS_SENDQ);
	}

	return HWA_SUCCESS;
}

#ifdef HWA_PKTPGR_BUILD

static void
_hwa_txstat_pagein_process(hwa_dev_t *dev)
{
	hwa_txstat_t *txstat;
	hwa_pktpgr_t *pktpgr;
	hwa_ring_t *h2s_ring;
	void *txstatus;
	bool fatal;
	uint32 elem_ix_ack;

	// Setup locals
	txstat = &dev->txstat;
	pktpgr = &dev->pktpgr;
	fatal = FALSE;
	h2s_ring = &txstat->status_ring[0]; // core 0

process:
	// elem_ix_bad can be same as elem_ix_ack, ignore it.
	if (txstat->elem_ix_bad == txstat->elem_ix_ack) {
		txstat->elem_ix_bad = -1;
		HWA_ERROR(("Hit bad pkg at %d\n", txstat->elem_ix_bad));
		hwa_ring_next_index(&txstat->elem_ix_ack, h2s_ring->depth);
	}

	// Get elem_ix_ack and advance it.
	elem_ix_ack = txstat->elem_ix_ack;
	hwa_ring_next_index(&txstat->elem_ix_ack, h2s_ring->depth);

	// Consume TxStatus received in status_ring, handing each upstream
	txstatus = HWA_RING_ELEM(hwa_txstat_status_t, h2s_ring, elem_ix_ack);

	// The bound process is controled by hwa_txstat_process.
	hwa_txstat_bmac_proc(dev, (uintptr)txstatus, (uintptr)&fatal, 0, 0);

	// Commit specific RD index
	hwa_ring_cons_put_rd(h2s_ring, txstat->elem_ix_ack);

	hwa_txstat_bmac_done(dev, (uintptr)&fatal, (uintptr)0, 0, 1);
	if (pktpgr->pgi_txs_unprocessed && (pktpgr->pgi_txs_req_tot == 0)) {
		pktpgr->pgi_txs_unprocessed--;
		HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: process one more txs\n");
		goto process;
	}

} // _hwa_txstat_pagein_process
#endif /* HWA_PKTPGR_BUILD */

// HWA4a TxStat block statistics collection
static void _hwa_txstat_stats_dump(hwa_dev_t *dev, uintptr buf, uint32 core);

void // Clear statistics for HWA4a TxStat block
hwa_txstat_stats_clear(hwa_txstat_t *txstat, uint32 core)
{
	hwa_dev_t *dev;

	dev = HWA_DEV(txstat);

	hwa_stats_clear(dev, HWA_STATS_TXSTS_CORE0 + core); // common

} // hwa_txstat_stats_clear

void // Print the common statistics for HWA4a TxStat block
_hwa_txstat_stats_dump(hwa_dev_t *dev, uintptr buf, uint32 core)
{
	hwa_txstat_stats_t *txstat_stats = &dev->txstat.stats[core];
	struct bcmstrbuf *b = (struct bcmstrbuf *)buf;

	OSL_CACHE_INV(txstat_stats, sizeof(hwa_txstat_stats_t));

	HWA_BPRINT(b, "%s HWA[%d] statistics core[%u] lazy<%u> qfull<%u> stalls<%u> dur<%u>\n",
		HWA4a, dev->unit, core,
		txstat_stats->num_lazy_intr, txstat_stats->num_queue_full_ctr_sat,
		txstat_stats->num_stalls_dma, txstat_stats->dur_dma_busy);

} // _hwa_txstat_stats_dump

void // Query and dump common statistics for HWA4a TxStat block
hwa_txstat_stats_dump(hwa_txstat_t *txstat, struct bcmstrbuf *b, uint8 clear_on_copy)
{
	uint32 core;
	hwa_dev_t *dev;

	dev = HWA_DEV(txstat);

	for (core = 0; core < HWA_TX_CORES; core++) {
		hwa_txstat_stats_t *txstat_stats = &txstat->stats[core];
		hwa_stats_copy(dev, HWA_STATS_TXSTS_CORE0 + core,
			HWA_PTR2UINT(txstat_stats), HWA_PTR2HIADDR(txstat_stats),
			/* num_sets */ 1, clear_on_copy, &_hwa_txstat_stats_dump,
			(uintptr)b, core);
	}

} // hwa_txstat_stats_dump

#if defined(BCMDBG) || defined(HWA_DUMP)

void // HWA4a TxStat: debug dump
hwa_txstat_dump(hwa_txstat_t *txstat, struct bcmstrbuf *b, bool verbose, bool dump_regs)
{
	uint32 core;
	hwa_dev_t *dev;

	dev = HWA_DEV(txstat);

	HWA_BPRINT(b, "%s HWA[%d] dump<%p>\n", HWA4a, dev->unit, txstat);

	if (txstat == (hwa_txstat_t*)NULL)
		return;

	HWA_BPRINT(b, "%s dump<%p>\n", HWA4a, txstat);

	for (core = 0; core < HWA_TX_CORES; core++) {
		hwa_ring_dump(&txstat->status_ring[core], b, "+ status");

		HWA_STATS_EXPR(
			HWA_BPRINT(b, "+ core<%u> wake<%u> proc<%u>\n",
				core, txstat->wake_cnt[core], txstat->proc_cnt[core]));
	}

	if (verbose == TRUE) {
		hwa_txstat_stats_dump(txstat, b, /* clear */ 0);
	}

#if defined(WLTEST) || defined(HWA_DUMP)
	if (dump_regs == TRUE)
		hwa_txstat_regs_dump(txstat, b);
#endif

} // hwa_txstat_dump

#if defined(WLTEST) || defined(HWA_DUMP)

void // HWA4a TxStat: dump block registers
hwa_txstat_regs_dump(hwa_txstat_t *txstat, struct bcmstrbuf *b)
{
	uint32 core;
	hwa_dev_t *dev;
	hwa_regs_t *regs;

	if (txstat == (hwa_txstat_t*)NULL)
		return;

	dev = HWA_DEV(txstat);
	regs = dev->regs;

	for (core = 0; core < HWA_TX_CORES; core++) {
		HWA_BPRINT(b, "%s registers[0x%p] offset[0x%04x]\n",
			HWA4a, &regs->tx_status[core],
			(uint32)OFFSETOF(hwa_regs_t, tx_status[core]));

		HWA_BPR_REG(b, tx_status[core], tseq_base_lo);
		HWA_BPR_REG(b, tx_status[core], tseq_base_hi);
		HWA_BPR_REG(b, tx_status[core], tseq_size);
		HWA_BPR_REG(b, tx_status[core], tseq_wridx);
		HWA_BPR_REG(b, tx_status[core], tseq_rdidx);
		HWA_BPR_REG(b, tx_status[core], tse_ctl);
		HWA_BPR_REG(b, tx_status[core], tse_sts);
		HWA_BPR_REG(b, tx_status[core], txs_debug_reg);
		HWA_BPR_REG(b, tx_status[core], dma_desc_template_txs);
		HWA_BPR_REG(b, tx_status[core], txe_cfg1);
		HWA_BPR_REG(b, tx_status[core], tse_axi_base);
		HWA_BPR_REG(b, tx_status[core], tse_axi_ctl);
	} // for core

} // hwa_txstat_regs_dump

#endif

#endif /* BCMDBG */

#endif /* HWA_TXSTAT_BUILD */

#if (defined(HWA_PKTPGR_BUILD) && defined(HWA_TXSTAT_BUILD)) || \
	defined(STS_XFER_TXS_PP)
/*
 * -----------------------------------------------------------------------------------------
 * Section: PKTPGR Tx Status PAGE-IN Request, Response handlers used by HWA4A & STS_XFER_TXS
 * -----------------------------------------------------------------------------------------
 */

// NOTE: fifo_idx is phycial fifo index.
void
hwa_txfifo_shadow_enq(hwa_dev_t *dev, int fifo_idx, int mpdu_count,
	int pkt_count, int txd_count, hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	hwa_txfifo_t *txfifo;
	hwa_pp_lbuf_t *txlbuf;
	hwa_txfifo_shadow32_t *shadow32;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);
	HWA_ASSERT(fifo_idx < HWA_TX_FIFOS);
	HWA_ASSERT(pktlist_head);
	HWA_ASSERT(pktlist_tail);
	HWA_ASSERT(mpdu_count);

	// Setup locals
	txfifo = &dev->txfifo;
	shadow32 = (hwa_txfifo_shadow32_t *)txfifo->txfifo_shadow;
	shadow32 += fifo_idx; // Move to specific index

	// Save in the shadow before really process Tx Status.
	if (shadow32->pkt_head == NULL) {
		HWA_PP_DBG(HWA_PP_DBG_4A, "  %s: Queue-head %d/%d packet [%p .. %p] to shadow %d "
			"count %d/%d\n", __FUNCTION__, mpdu_count, pkt_count,
			pktlist_head, pktlist_tail, fifo_idx,
			shadow32->pkt_count, shadow32->mpdu_count);

		shadow32->pkt_head = HWA_PTR2UINT(pktlist_head);
		shadow32->pkt_tail = HWA_PTR2UINT(pktlist_tail);
		shadow32->pkt_count = pkt_count;
		shadow32->mpdu_count = mpdu_count;
	} else {
		HWA_PP_DBG(HWA_PP_DBG_4A, "  %s: Queue-tail %d/%d packet [%p .. %p] to shadow %d "
			"[%x .. %x] count %d/%d\n", __FUNCTION__, mpdu_count, pkt_count,
			pktlist_head, pktlist_tail, fifo_idx, shadow32->pkt_head,
			shadow32->pkt_tail, shadow32->pkt_count, shadow32->mpdu_count);

		txlbuf = HWA_UINT2PTR(hwa_pp_lbuf_t, shadow32->pkt_tail);
		HWAPKTSETLINK(txlbuf, pktlist_head);
		shadow32->pkt_tail = HWA_PTR2UINT(pktlist_tail);
		shadow32->pkt_count += pkt_count;
		shadow32->mpdu_count += mpdu_count;
	}

	// Decrease HW count
	HWA_ASSERT(shadow32->hw.pkt_count >= pkt_count);
	HWA_ASSERT(shadow32->hw.mpdu_count >= mpdu_count);
	shadow32->hw.pkt_count -= pkt_count;
	shadow32->hw.mpdu_count -= mpdu_count;

	// Increase HW TXD/AQM count
	shadow32->hw.txd_avail += txd_count;
	shadow32->hw.aqm_avail += mpdu_count;

	// Increase stats count
	shadow32->stats.pkt_count += pkt_count;
	shadow32->stats.mpdu_count += mpdu_count;
} // hwa_txfifo_shadow_enq

// PKTPGR TXS PAGE-IN response callback invoked upon construction of packet list in dongle memory.
static void
hwa_txstat_pagein_process(hwa_dev_t *dev)
{
#if defined(HWA_TXSTAT_BUILD)
	_hwa_txstat_pagein_process(dev);
#elif defined(STS_XFER_TXS_PP)
	wlc_sts_xfer_txs_pagein_process(dev->wlc);
#endif /* STS_XFER_TXS_PP */
} // hwa_txstat_pagein_process

// Sanity check and fixup head and end for local packet
hwa_pp_lbuf_t *
hwa_txstat_pagein_rsp_fixup_local(hwa_dev_t *dev, hwa_pp_lbuf_t *txlbuf,
	int fifo_idx, int mpdu_count, uint8 tagged)
{
	osl_t *osh;
	wlc_info_t *wlc;
	hwa_pp_lbuf_t *txlbuf_local;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	osh = dev->osh;
	wlc = (wlc_info_t *)dev->wlc;

	ASSERT(mpdu_count == 1);

	// Some MGMT packet will be sent to lowtxq first. And then register pcb callback function.
	// Before the pkttag was set, pageout the packet to host may be happened first.
	// So the pkttag in host memory is not correct. EX. refer to wlc_ampdu_send_bar().
	// To address out of sync:
	// 1. Keep the original local packet(from heap) in dongle.
	// 2. After txs pagein, free the DDBM packet from HWA.
	// 3. Use the original one for txs processing.
	txlbuf_local = (hwa_pp_lbuf_t *)PKTLOCAL(osh, txlbuf);
	PKTRESETLOCAL(dev->osh, txlbuf_local);

	HWA_ASSERT(HWAPKTNDESC(txlbuf_local) == 1);

	// Free the DDBM packet
	// txlbuf will be the same as txlbuf_local if doing hwa_pktpgr_pageout_ring_shadow_reclaim.
	if (txlbuf != txlbuf_local) {
		// If driver need to do partial shadow reclaim.
		// Keep the DDBM packet for push req except ULMU_TRIG_FIFO
		if ((tagged == HWA_PP_CMD_TAGGED) &&
			(fifo_idx != WLC_HW_MAP_TXFIFO(wlc, ULMU_TRIG_FIFO))) {
			HWA_INFO(("%s: process tagged mgmt frame for partial shadow reclaim\n",
				__FUNCTION__));
			PKTSETMGMTDDBMPKT(osh, txlbuf_local, txlbuf);
		} else {
			hwa_pktpgr_free_tx(dev, txlbuf, txlbuf, 1);
		}
	}

	// Put HME LCLPK back
	if (PKTHASHMEDATA(osh, txlbuf_local)) {
		hwa_pktpgr_hmedata_free(dev, txlbuf_local);
		PKTRESETHMEDATA(osh, txlbuf_local);

		if (PKTNEEDLOCALFIXUP(osh, txlbuf_local)) {
			// Adjust data, head, and end.
			PKTADJDATA(osh, txlbuf_local, PKTPPBUFFERP(txlbuf_local),
				PKTLEN(osh, txlbuf_local), PKTGETHEADROOM(osh, txlbuf_local));
			PKTRESETLOCALFIXUP(osh, txlbuf_local);
		}
	}

	HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)txlbuf_local,
		NULL, "TXS-FIXUP-LOCAL", TRUE));

	return txlbuf_local;
}

// Sanity check and fixup head and end
void
hwa_txstat_pagein_rsp_fixup(hwa_dev_t *dev, hwa_pp_lbuf_t *txlbuf, int mpdu_count)
{
	osl_t *osh;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	osh = dev->osh;

	// Mark as DDBM PKT
	PKTSETHWADDMBPKT(osh, txlbuf);

	/* XXX FIXME: Assume HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES size of
	 * data_buffer is enough for TxStatus.
	 * NOTE: pktlist_head data_buffer contains the same 128 bytes of
	 * original local packet but normally the PKTDATA is start from the
	 * end in the TX path. So it may or may not enough for	TxStatus,
	 * the valid data maybe too small because the headroom
	 * is bigger.
	 *
	 * 1. Do 128 bytes BME copy from Host to pktlist_head->data_buffer if
	 * PKTHASHMEDATA.
	 * 2. fixup len, current pktlist_head is from DDBM and the data_buffer has same
	 * 128 bytes copy as local packet that we pageout local before.
	 *
	 * XXX FIXME: PQP case need fully data. XXX.  We should reuse PKTHME in host for PQP
	 */
	if (PKTHASHMEDATA(osh, txlbuf)) {
		haddr64_t haddr64;

		// Two cases:
		// 1.Fetched tx packet, the txlbuf:data_buffer is not same segment as heap buffer.
		//      Note: PKTISBUFALLOC will be reset by PKTBUFFREE earlier,
		//      so use !PKTISMGMTTXPKT to identify it.
		// 2. MGMT packet which headroom + pktlen >= HWA_PP_PKTDATABUF_BYTES
		// Packet in either one case has PKTHASHMEDATA flag set.

		HADDR64_HI_SET(haddr64, PKTHME_HI(dev->osh, txlbuf));
		HADDR64_LO_SET(haddr64, PKTHME_LO(dev->osh, txlbuf));

		// We will do hme_put in hwa_pktpgr_hmedata_free
		// TXFLAG only can use HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES,
		// so just BME HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES.
		if (hme_h2d_xfer(&haddr64, PKTPPBUFFERP(txlbuf),
			HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES, TRUE, HME_USER_NOOP) < 0) {
			HWA_ERROR(("hme_h2d_xfer failed, haddr64<0x%x_%x> daddr32<0x%p>\n",
				HADDR64_HI(haddr64), HADDR64_LO(haddr64),
				PKTPPBUFFERP(txlbuf)));
			HWA_ASSERT(0);
		}

		// Adjust data, head, and end.
		// Note, the PKTLEN may > HWA_PP_PKTDATABUF_BYTES.
		// XXX, do we need headroom during TxS processing?
		PKTADJDATA(osh, txlbuf, PKTPPBUFFERP(txlbuf),
			HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES, 0);
	} else {
		// 2. Set head and end.
		// TXFRAG can only use HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES,
		PKTSETTXBUFRANGE(osh, txlbuf, PKTPPBUFFERP(txlbuf),
			HWA_PP_PKTDATABUF_TXFRAG_MAX_BYTES);
	}

	HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)txlbuf, NULL, "TXS-FIXUP", TRUE));
}

// NOTE: fifo_idx is phycial fifo index.
void
hwa_txstat_pagein_rsp(hwa_dev_t *dev, uint8 pktpgr_trans_id,
	uint8 tagged, int fifo_idx, int mpdu_count, int pkt_count,
	hwa_pp_lbuf_t *pktlist_head, hwa_pp_lbuf_t *pktlist_tail)
{
	hwa_pktpgr_t *pktpgr;
	uint16 mpdu, txd_count;
	int16 ddbm_cons;
	hwa_pp_lbuf_t *txlbuf, *txlbuf_link, *tx_head, *tx_tail;
#ifdef BCMDBG
	int pkt_cnt;
#endif /* BCMDBG */
	void *next;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	mpdu = 0;
	txlbuf = pktlist_head;
	pktpgr = &dev->pktpgr;
	tx_head = NULL;
	tx_tail = NULL;
#ifdef BCMDBG
	pkt_cnt = 0;
#endif /* BCMDBG */
	txd_count = 0;

	// Count can be zero when we over pagein txstat.
	if (pkt_count == 0 || mpdu_count == 0) {
		HWA_ERROR(("%s: Over pagein? pkt_count<%d> mpdu_count<%d> <==TXS-RSP(%d)==\n",
			__FUNCTION__, pkt_count, mpdu_count, pktpgr_trans_id));
		// XXX, FIXME, DDBM accounting
		if (tagged == HWA_PP_CMD_TAGGED)
			return;

		goto done;
	}

	if (tagged != HWA_PP_CMD_TAGGED) {
		// DDBM accounting
		// Assume 8-in-1, has decreased # of ncons * 2 in req, increase the delta
		// of mpdu * 2 - pkt_count here.
		ddbm_cons = mpdu_count * 2;
		/* In NAR + AMSDU mode we don't call wlc_amsdu_single_txlfrag_fixup to
		 * support TAF feature, so here we may get 1 mpdu_count with more than
		 * 2 pkt_count case. Because each txlbuf only has one msdu frame.
		 */
		ddbm_cons -= pkt_count; // ddbm_cons over counting.

		// Add over counting delta
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_avail_sw, ddbm_cons);
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_sw_tx, ddbm_cons);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: + %3u : %d @ %s\n", ddbm_cons,
			pktpgr->ddbm_avail_sw, __FUNCTION__);
	} else {
		// DDBM accounting
		HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, pkt_count);
		HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, pkt_count);
		HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", pkt_count,
			pktpgr->ddbm_avail_sw, __FUNCTION__);
	}

	HWA_TXSTAT_EXPR({
	HWA_PP_DBG(HWA_PP_DBG_4A, "  <<PAGEIN::RSP TXSTAT MPDUS : mpdu %3u pkt %3u "
		"list[%p(%d) .. %p(%d)] fifo %3u elem_ix_ack %d <==TXS-RSP(%d)==\n\n", mpdu_count,
		pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx,
		dev->txstat.elem_ix_ack, pktpgr_trans_id);
	});

#if defined(STS_XFER_TXS_PP)
	HWA_PP_DBG(HWA_PP_DBG_4A, "  <<PAGEIN::RSP TXSTAT MPDUS : mpdu %3u pkt %3u "
		"list[%p(%d) .. %p(%d)] fifo %3u <==TXS-RSP(%d)==\n\n", mpdu_count,
		pkt_count, pktlist_head, PKTMAPID(pktlist_head),
		pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx,
		pktpgr_trans_id);
#endif /* STS_XFER_TXS_PP */

	HWA_PKT_DUMP_EXPR(hwa_txfifo_dump_pkt((void *)pktlist_head, NULL,
		"PAGEIN_TXSTAT", FALSE));

	// Sanity check and fixup head and end
	if (PKTLINK(pktlist_tail)) {
		HWA_ERROR(("  <<PAGEIN::RSP TXSTAT MPDUS : mpdu %3u pkt %3u "
			"list[%p(%d) .. %p(%d)] fifo %3u <==TXS-RSP(%d)==\n\n",
			mpdu_count, pkt_count, pktlist_head, PKTMAPID(pktlist_head),
			pktlist_tail, PKTMAPID(pktlist_tail), fifo_idx, pktpgr_trans_id));
		HWA_PKT_DUMP_EXPR(_hwa_txfifo_dump_pkt((void *)pktlist_head, NULL,
			"TXS: head-list", FALSE, TRUE));
		HWA_PKT_DUMP_EXPR(_hwa_txfifo_dump_pkt((void *)pktlist_tail, NULL,
			"TXS: tail-only", TRUE, TRUE));
	}
	HWA_ASSERT(PKTLINK(pktlist_tail) == NULL);

	while (txlbuf != NULL) {
		mpdu++;
#ifdef BCMDBG
		pkt_cnt++;
#endif

		// Walk through packet list for
		// 1. DBM audit, 2. next txlbuf data pointer fix up
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
		// Alloc: TxPost normal case
		hwa_pktpgr_dbm_audit(dev, txlbuf, DBM_AUDIT_TX, DBM_AUDIT_2DBM, DBM_AUDIT_ALLOC);
#endif
		next = PKTNEXT(dev->osh, txlbuf);
		for (; next; next = PKTNEXT(dev->osh, next)) {
#ifdef BCMDBG
			pkt_cnt++;
#endif
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
			// Alloc: TxPost normal case
			hwa_pktpgr_dbm_audit(dev, next, DBM_AUDIT_TX, DBM_AUDIT_2DBM,
				DBM_AUDIT_ALLOC);
#endif
			// Data pointer is reset to 0 in all next txlbuf, fix it up.
			HWA_ASSERT(PKTDATA(dev->osh, next) == NULL);
			PKTSETTXBUF(dev->osh, next, PKTPPBUFFERP(next), 0);
		}

		txlbuf_link = (hwa_pp_lbuf_t *)PKTLINK(txlbuf);
		PKTSETLINK(txlbuf, NULL);

		if (mpdu == mpdu_count) {
			// XXX, pktlist_tail can be the second TxLfrag of a 8-in-1 AMSDU
			// so txlbuf == pktlist_tail is not TRUE.  Check if the tail is the last
			// next of last MPDU.
			// HWA_ASSERT(txlbuf == pktlist_tail);
			// Make sure the txlbuf is last mpdu and pktlist_tail is last next of txlbuf
			if (txlbuf != pktlist_tail) {
				hwa_pp_lbuf_t *pkt = txlbuf;
				HWA_ASSERT(pktlist_tail != NULL);
				while (pkt) {
					pkt = (hwa_pp_lbuf_t *)PKTNEXT(dev->osh, pkt);
					if (pkt == pktlist_tail) {
						// Reassign the pktlist_tail to the real last mpdu
						pktlist_tail = txlbuf;
						break;
					}
				}

				if (pkt == NULL) {
					HWA_ERROR(("TXS: txlbuf %p pktlist_tail %p\n",
						txlbuf, pktlist_tail));
					HWA_ASSERT(txlbuf == pktlist_tail);
				}
			}
		}

		if (PKTISMGMTTXPKT(dev->osh, txlbuf)) {
			txlbuf = hwa_txstat_pagein_rsp_fixup_local(dev, txlbuf, fifo_idx,
				1, tagged);
		} else {
			hwa_txstat_pagein_rsp_fixup(dev, txlbuf, mpdu_count);
		}

		// Get txd_count
		txd_count += HWAPKTNDESC(txlbuf);

		if (tx_tail == NULL) {
			tx_head = tx_tail = txlbuf;
		} else {
			PKTSETLINK(tx_tail, txlbuf);
			tx_tail = txlbuf;
		}

		txlbuf = txlbuf_link;
	}

#ifdef BCMDBG
	ASSERT(mpdu == mpdu_count);
	ASSERT(pkt_cnt == pkt_count);
#endif /* BCMDBG */

	// Link the pktlist to shadow list.
	hwa_txfifo_shadow_enq(dev, fifo_idx, mpdu_count, pkt_count, txd_count,
		tx_head, tx_tail);

	// We just want to pagein pacekts from HW shadow, bypass TxS process.
	if (tagged == HWA_PP_CMD_TAGGED) {
		return;
	}
done:

	hwa_txstat_pagein_process(dev);

	// Release emergency flag
	if (PGIEMERISTXSSTARVE(pktpgr) &&
		(pktpgr->pgi_emer_id[PGI_EMER_TYPE_TXS] == pktpgr_trans_id)) {
		// Reset it.
		PGIEMERRESETTXSSTARVE(pktpgr);
	}

	return;
}

int // PAGEIN REQUEST TXSTATUS
hwa_txstat_pagein_req(hwa_dev_t *dev, void *txs_pkg16, uint32 elem_ix)
{
	wlc_info_t *wlc;
	wlc_txs40_status_t *txs40_status;
	hwa_pktpgr_t *pktpgr;
	uint16 status_bits, ncons, frameid;
	int txserr, pktpgr_trans_id;
	hwa_pp_pagein_req_txstatus_t req;
	HWA_PP_DBG_EXPR(uint16 seq);
	uint16 ddbm_cons;
	uint16 fifo_idx;
	bool is_intermediate, pgi_itxs_op, in_emergency;

	HWA_AUDIT_DEV(dev);
	HWA_FTRACE(HWA4a);

	// Setup locals
	pktpgr = &dev->pktpgr;
	wlc = (wlc_info_t *)dev->wlc;
	txs40_status = (wlc_txs40_status_t *)txs_pkg16;
	pgi_itxs_op = FALSE;
	in_emergency = FALSE;

	// Check pkt 1 and get ncons
	if ((txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[0])) == BCME_OK) {

		// Get frameid, ncons
		frameid = (txs40_status->mactxs.s1 & TX_STATUS_FID_MASK) >> TX_STATUS_FID_SHIFT;
		status_bits = txs40_status->mactxs.s1 & TXS_STATUS_MASK;
		ncons = ((status_bits & TX_STATUS40_NCONS) >> TX_STATUS40_NCONS_SHIFT);
		is_intermediate = (status_bits & TX_STATUS40_INTERMEDIATE) != 0;
		HWA_PP_DBG_EXPR(seq = txs40_status->mactxs.s2 & TX_STATUS_SEQ_MASK);

		// Check pkg 2, it comes always
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[1]);
		if (txserr != BCME_OK) {
			// if not a valid package, assert and bail
			HWA_ERROR(("wl%d: %s: package 2 read not valid, txserr 0x%x\n",
				wlc->pub->unit, __FUNCTION__, txserr));
			goto pkg_err;
		}

#if defined(TX_STATUS_MACTXS_64BYTES)
		/* pkg 3 comes always */
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[2]);
		/* store saved extras (check valid pkg) */
		if (txserr != BCME_OK) {
			// if not a valid package, assert and bail
			HWA_ERROR(("wl%d: %s: package 3 read not valid, txserr 0x%x\n",
				wlc->pub->unit, __FUNCTION__, txserr));
			goto pkg_err;
		}

		/* pkg 4 comes always */
		txserr = hwa_txstat_read_txs_pkg16(wlc, &txs40_status->pkg16[3]);
		/* store saved extras (check valid pkg) */
		if (txserr != BCME_OK) {
			// if not a valid package, assert and bail
			HWA_ERROR(("wl%d: %s: package 4 read not valid, txserr 0x%x\n",
				wlc->pub->unit, __FUNCTION__, txserr));
			goto pkg_err;
		}
#endif /* TX_STATUS_MACTXS_64BYTES */

		HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: frameid 0x%x [s<%d> q<%d>] ncons %d "
			"status_bits 0x%x seq 0x%x elem_ix %d\n", frameid,
			D11_TXFID_GET_SEQ(wlc, frameid), D11_TXFID_GET_FIFO(wlc, frameid),
			ncons, status_bits, seq, elem_ix);
	} else {
		HWA_ERROR(("wl%d: %s: package 1 read not valid, txserr 0x%x\n",
				wlc->pub->unit, __FUNCTION__, txserr));
		goto pkg_err;
	}

#if defined(WL_MU_TX) && defined(WL11AX)
	// For trigger status handling
	if (HE_ULOMU_ENAB(wlc->pub)) {
		if (TGTXS_TGFID(txs40_status->mactxs.s1) == TX_STATUS128_TRIGGERTP) {
			if (pktpgr->pgi_txs_req_tot) {
				pktpgr->pgi_txs_unprocessed++;
				HWA_PP_DBG(HWA_PP_DBG_4A, " TGTXS: increase txs_unprocessed\n");
				return BCME_OK;
			} else {
				HWA_PP_DBG(HWA_PP_DBG_4A, " TGTXS: process txs\n");
				hwa_txstat_pagein_process(dev);
				return BCME_OK;
			}
		}
	}
#endif /* defined(WL_MU_TX) && defined(WL11AX) */

	// Get fifo_idx if it is not a trigger status. frameid in trigger status is fake.
	fifo_idx = WLC_HW_MAP_TXFIFO(wlc, D11_TXFID_GET_FIFO(wlc, frameid));

	if (is_intermediate) {
		// For intermediate txs, ncons should be 0
		HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: intermediate: ncons %d fifo %d\n",
			ncons, fifo_idx);

		if (!isset(pktpgr->pgi_itxs, fifo_idx)) {
			// Force ncons = 1 and increase pktpgr->pgi_txs_itm[fifo]
			// for non-intermediate txs ncons calculation
			setbit(pktpgr->pgi_itxs, fifo_idx);
			ncons = 1;
			pgi_itxs_op = TRUE;
		} else {
			// XXX: force ncons = 0 even it is 0
			ncons = 0;
		}
	} else if (isset(pktpgr->pgi_itxs, fifo_idx)) {
		HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: subtract 1 from ncons %d, fifo %d\n",
			ncons, fifo_idx);
		ASSERT(ncons > 0);
		ncons--;
		clrbit(pktpgr->pgi_itxs, fifo_idx);
		pgi_itxs_op = TRUE;
	}

	// If ncons is 0, driver should have mpdu in SW shadow.
	// XXX: FIXME: do we need to check mpdu count in SW shadow?
	if (ncons == 0) {
		if (pktpgr->pgi_txs_req_tot) {
			pktpgr->pgi_txs_unprocessed++;
			HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: increase txs_unprocessed\n");
			return BCME_OK;
		} else {
			HWA_PP_DBG(HWA_PP_DBG_4A, " TXS: process txs\n");
			hwa_txstat_pagein_process(dev);
			return BCME_OK;
		}
	}

#ifdef HWA_QT_TEST
extra_txs:
#endif

	// Assume 8-in-1, decrease ncons * 2 in req and increase the delta
	// of mpdu * 2 - pkt_count.
	ddbm_cons = ncons * 2;
	HWA_ASSERT(ddbm_cons <= 128);
	if (pktpgr->ddbm_avail_sw < (int16)ddbm_cons) {
		HWA_TRACE(("%s ddbm_avail_sw<%u> ddbm_sw_tx<%u> ddbm_cons<%u>\n",
			__FUNCTION__, pktpgr->ddbm_avail_sw, pktpgr->ddbm_sw_tx,
			ddbm_cons));

		if (PGIEMERISTXSRECLAIM(pktpgr)) {
			PGIEMERRESETTXSRECLAIM(pktpgr);
		} else {
			if (!PGIEMERISTXSSTARVE(pktpgr)) {
				PGIEMERSETTXSSTARVE(pktpgr);
				in_emergency = TRUE;
			} else {
				if (pgi_itxs_op) {
					if (isset(pktpgr->pgi_itxs, fifo_idx)) {
						clrbit(pktpgr->pgi_itxs, fifo_idx);
					} else {
						setbit(pktpgr->pgi_itxs, fifo_idx);
					}
				}

				pktpgr->pgi_txstatus_fail++;
				pktpgr->pgi_txs_cont_fail++;

				/* Re-schedule to consume the response ring */
				hwa_worklet_invoke(dev,
					HWA_COMMON_INTSTATUS_PACKET_PAGER_INTR_MASK,
					(HWA_COMMON_PAGEIN_INT_MASK |
					HWA_COMMON_PAGEOUT_INT_MASK));

				return BCME_NORESOURCE;
			}
		}
	}

	// Get TxStatus shadow list.
	// NOTE: We have to trush ncons.
	req.trans        = HWA_PP_PAGEIN_TXSTATUS;
	req.fifo_idx     = (uint8)fifo_idx;
	req.zero         = 0;
#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-663, case2: PGI TXS ncons = 2 but 1 packet in HW
	if (ncons == 1 && (hwa_kflag & HWA_KFLAG_663_CASE_2)) {
		hwa_kflag &= ~HWA_KFLAG_663_CASE_2;
		ncons++;
		HWA_PRINT(" ---> EXTRA ncons++ = %d\n", ncons);
	}
#endif
	req.mpdu_count   = ncons;
	req.tagged       = HWA_PP_CMD_NOT_TAGGED;

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-662
	if (HWAREV_GE(dev->corerev, 133)) {
		req.swar = 0xBF;
	}

	// XXX, CRBCAHWA-663
	if (hwa_kflag & HWA_KFLAG_663_CASE_1_AUX) {
		hwa_kflag &= ~HWA_KFLAG_663_CASE_1_AUX;
		req.swar = 0xFB;
	}

	// XXX, CRBCAHWA-664
	if (hwa_kflag & HWA_KFLAG_664) {
		hwa_kflag &= ~HWA_KFLAG_664;
		HWA_PRINT(" ---> HWA_KFLAG_664: ncons %d\n", ncons);
	}
#endif /* HWA_QT_TEST */

	// XXX, Do we neeed to pagein txstatus one more packet for
	// wlc_bmac_dmatx_peeknexttxp ?  No, we don't.  Because
	// wlc_bmac_dmatx_peeknexttxp will be called by wlc_dotxstatus
	// after pagein txstatus.
	// NOTE: Be careful, we may pre-pagein txstatus packets
	// to SW shadow for txs->status.is_intermediate but this
	// case can overcome wlc_bmac_dmatx_peeknexttxp.

	// Caller hwa_txstat_process will keep this pkg in 4A when BCME_NORESOURCE.
	pktpgr_trans_id = hwa_pktpgr_request(dev, hwa_pktpgr_pagein_req_ring, &req);
	if (pktpgr_trans_id == HWA_FAILURE) {
		HWA_ERROR(("%s PAGEIN::REQ TXSTATUS failure mpdu<%u> fifo<%u>,"
			" no resource\n", HWA4a, ncons, fifo_idx));
		// Reset it.
		if (PGIEMERISTXSSTARVE(pktpgr)) {
			PGIEMERRESETTXSSTARVE(pktpgr);
		}
		return BCME_NORESOURCE;
	}

	// DDBM accounting
	// XXX we should account it in req but we don't have pkt_count in req
	// A0: hope the 128: a tolance cycle for pgo_tx+ and pgi_txs-can cover it.
	// B0: don't need the DDBM accounting
	// Assume 8-in-1, decrease ncons * 2 in req and increase the delta
	// of mpdu * 2 - pkt_count.
	HWA_DDBM_COUNT_DEC(pktpgr->ddbm_avail_sw, ddbm_cons);
	HWA_DDBM_COUNT_LWM(pktpgr->ddbm_avail_sw, pktpgr->ddbm_avail_sw_lwm);
	HWA_DDBM_COUNT_INC(pktpgr->ddbm_sw_tx, ddbm_cons);
	HWA_DDBM_COUNT_HWM(pktpgr->ddbm_sw_tx, pktpgr->ddbm_sw_tx_hwm);
	HWA_PP_DBG(HWA_PP_DBG_DDBM, " DDBM: - %3u : %d @ %s\n", ddbm_cons,
		pktpgr->ddbm_avail_sw, __FUNCTION__);

	HWA_COUNT_INC(pktpgr->pgi_txs_req[fifo_idx], 1);
	HWA_COUNT_INC(pktpgr->pgi_txs_req_tot, 1);
	HWA_COUNT_HWM(pktpgr->pgi_txs_req_tot, pktpgr->pgi_txs_req_tot_hwm);

	HWA_PP_DBG(HWA_PP_DBG_4A, "  >>PAGEIN::REQ TXSTATUS: mpdu %3u fifo %3u elem_ix %d "
		"==TXS-REQ(%d)==>\n\n", ncons, fifo_idx, elem_ix, pktpgr_trans_id);

	pktpgr->pgi_txs_cont_fail = 0;

	if (in_emergency) {
		pktpgr->pgi_emer_id[PGI_EMER_TYPE_TXS] = (uint8)pktpgr_trans_id;
		pktpgr->pgi_txstatus_emer++;
	}

#ifdef HWA_QT_TEST
	// XXX, CRBCAHWA-663: Duplicate pkt_mapid in PGI TXS Resp,
	// this is happened after we got a ZERO PGI TXS RESP
	if (ncons == 1 && (hwa_kflag & HWA_KFLAG_663_CASE_1)) {
		hwa_kflag &= ~HWA_KFLAG_663_CASE_1;
		hwa_kflag |= HWA_KFLAG_663_CASE_1_AUX;
		HWA_PRINT(" ---> EXTRA PGI TXS\n");
		goto extra_txs;
	}
#endif

	return BCME_OK;

pkg_err:
	// XXX FIXME-PP how to handle more then one.
	HWA_TXSTAT_EXPR({
		hwa_txstat_t *txstat = &dev->txstat;
		HWA_ASSERT(txstat->elem_ix_bad == -1);
		if (txstat->elem_ix_bad == -1)
			txstat->elem_ix_bad = elem_ix;
	});

#if defined(STS_XFER_TXS_PP)
	HWA_ERROR(("wl%d: %s: Invalid Tx Status\n", wlc->pub->unit, __FUNCTION__));
	wlc_sts_xfer_txs_error_dump(dev->wlc, txs_pkg16, TRUE);
	HWA_ASSERT(0);
#endif /* STS_XFER_TXS_PP */

	if (txserr == BCME_NODEVICE) {
		HWA_ERROR(("wl%d: %s: dead chip\n", wlc->pub->unit, __FUNCTION__));
		HWA_ASSERT(txs40_status->mactxs.s1 != 0xffffffff);
		WL_HEALTH_LOG(wlc, DEADCHIP_ERROR);
		return BCME_ERROR;
	}

	return BCME_OK;
} // hwa_txstat_pagein_req

/*
 * XXX: WAR for TXs framid mismatch for 6715Ax
 * The reason is because when HWA Packet Pager pageout tx packet it will do following data moving.
 * - HWA dmaA channel move 256B (include TXD header) from dongle DDBM in sysmem to Host HDBM in DDR
 * - HWA dmaB channel move TXFIFO descriptor + AQMFIFO descriptor + update xmtptr to MAC
 * So if update xmtptr is happened before dmaA done then uCode will get incorrect TXD header
 * This issue have fixed in HWA r132 RTL but 6715Ax.
 */
bool
hwa_txstat_mismatch_bypass(struct hwa_dev *dev)
{
	hwa_pktpgr_t *pktpgr;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Only bypass one time
	if (!pktpgr->pgi_txs_mismatch) {
		pktpgr->pgi_txs_mismatch = TRUE;
		return TRUE;
	} else {
		return FALSE;
	}
}

void
hwa_txstat_mismatch_reset(struct hwa_dev *dev)
{
	hwa_pktpgr_t *pktpgr;

	// Audit pre-conditions
	HWA_AUDIT_DEV(dev);

	// Setup locals
	pktpgr = &dev->pktpgr;

	// Reset if incoming txs is success.
	pktpgr->pgi_txs_mismatch = FALSE;
}
#endif /* (HWA_PKTPGR_BUILD && HWA_TXSTAT_BUILD) || STS_XFER_TXS_PP */

#if defined(BCMHWA) && defined(HWA_RXPATH_BUILD)
void
hwa_wl_reclaim_rx_packets(hwa_dev_t *dev)
{
	wlc_info_t *wlc;

	// Setup locals
	wlc = dev->wlc;

	// Flush amsdu deagg, do PKTFREE
	wlc_amsdu_flush(wlc->ami);

	// Flush ampdu rx reordering queue to Host
	wlc_ampdu_rx_flush_all(wlc);
}
#endif /* BCMHWA && HWA_RXPATH_BUILD */

void
hwa_wl_flush_all(hwa_dev_t *dev)
{
#if defined(HWA_RXFILL_BUILD) || defined(HWA_TXSTAT_BUILD)
	wl_info_t *wl = ((wlc_info_t *)dev->wlc)->wl;

	/* flush wl->rxcpl_list for rxcpl fast path. */
	HWA_RXFILL_EXPR(wl_flush_rxreorderqeue_flow(wl, &wl->rxcpl_list));
#ifdef DONGLEBUILD
	/* flush accumulated txrxstatus here */
	wl_busioctl(wl, BUS_FLUSH_CHAINED_PKTS, NULL, 0, NULL, NULL, FALSE); // function in rte
#else
	BCM_REFERENCE(wl);
#endif /* DONGLEBUILD */
#endif /* HWA_RXFILL_BUILD || HWA_TXSTAT_BUILD */
}

void
hwa_wlc_mac_event(hwa_dev_t *dev, uint reason)
{
	wlc_info_t *wlc = (wlc_info_t *)dev->wlc;
	wlc_mac_event(wlc, WLC_E_HWA_EVENT, NULL, WLC_E_STATUS_SUCCESS, reason, 0, 0, 0);
}

#if defined(BCMHME)
int
hwa_wl_hme_macifs_upd(hwa_dev_t *dev, dma64addr_t hmeaddr, uint32 hmelen)
{
	return (wlc_bmac_hme_macifs_upd(dev->wlc, hmeaddr, hmelen));
}
#endif /* BCMHME */

void
hwa_caps(struct hwa_dev *dev, struct bcmstrbuf *b)
{
	if (dev == (hwa_dev_t*)NULL)
		return;

	HWA_BPRINT(b, "hwa");
	HWA_RXPOST_EXPR(HWA_BPRINT(b, "-1a"));
	HWA_RXFILL_EXPR(HWA_BPRINT(b, "-1b"));
	HWA_RXDATA_EXPR(HWA_BPRINT(b, "-2a"));
	HWA_RXCPLE_EXPR(HWA_BPRINT(b, "-2b"));
	HWA_TXPOST_EXPR(HWA_BPRINT(b, "-3a"));
	HWA_TXFIFO_EXPR(HWA_BPRINT(b, "-3b"));
	HWA_TXSTAT_EXPR(HWA_BPRINT(b, "-4a"));
	HWA_TXCPLE_EXPR(HWA_BPRINT(b, "-4b"));
	HWA_PKTPGR_EXPR(HWA_BPRINT(b, "-pp"));
	HWA_BPRINT(b, " ");
}

/** IOVAR table */
enum {
	IOV_HWA_RX_ENABLE,
	IOV_HWA_MSGLEVEL,
	IOV_HWA_PP_DBG,
	IOV_HWA_PKTDUMPLEVEL,
	IOV_HWA_D11B_RECYCLE_WAR,
	IOV_HWA_RXPKT_COUNT_MAX,
	IOV_HWA_RXPKT_REQ_MAX,
	IOV_HWA_DBM_AUDIT_DUMP,
	IOV_HWA_PSPL,
	IOV_HWA_PSPL_MPDU,
	IOV_HWA_PSPL_MPDU_APPEND,
	IOV_HWA_PSPL_PGO_MPDU,
	IOV_HWA_RXPKT_ADD,
	IOV_HWA_RXPKT_DEL,
	IOV_HWA_RXPKT_ALT,
	IOV_HWA_RXPKT_ALT_TIMES,
	IOV_HWA_RXPKT_ALT_ADD,
	IOV_HWA_KFLAG,
	IOV_HWA_TXS_DDBM_RESV,
	IOV_HWA_TXCPL_MON,
	IOV_LAST
};

static const bcm_iovar_t hwa_iovars[] = {
	{"hwa_rx_en", IOV_HWA_RX_ENABLE, (0), 0, IOVT_BOOL, 0},
	{"hwa_msglevel", IOV_HWA_MSGLEVEL, (0), 0, IOVT_UINT32, 0},
	{"hwa_pktdumplevel", IOV_HWA_PKTDUMPLEVEL, (0), 0, IOVT_UINT32, 0},

#if defined(HWA_PKTPGR_BUILD)
	{"hwa_pp_dbg", IOV_HWA_PP_DBG, (0), 0, IOVT_UINT32, 0},
	{"hwa_d11b_recycle_war", IOV_HWA_D11B_RECYCLE_WAR, (0), 0, IOVT_BOOL, 0},
	{"hwa_rxpkt_count_max", IOV_HWA_RXPKT_COUNT_MAX, (0), 0, IOVT_UINT32, 0},
	{"hwa_rxpkt_req_max", IOV_HWA_RXPKT_REQ_MAX, (0), 0, IOVT_UINT32, 0},
	{"hwa_txs_ddbm_resv", IOV_HWA_TXS_DDBM_RESV, (0), 0, IOVT_UINT32, 0},
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	{"hwa_dbm_audit_dump", IOV_HWA_DBM_AUDIT_DUMP, (0), 0, IOVT_UINT32, 0},
#endif
#ifdef PSPL_TX_TEST
	{"hwa_pspl", IOV_HWA_PSPL, (0), 0, IOVT_UINT32, 0},
	{"hwa_pspl_mpdu", IOV_HWA_PSPL_MPDU, (0), 0, IOVT_UINT32, 0},
	{"hwa_pspl_mpdu_append", IOV_HWA_PSPL_MPDU_APPEND, (0), 0, IOVT_UINT32, 0},
	{"hwa_pspl_pgo_mpdu", IOV_HWA_PSPL_PGO_MPDU, (0), 0, IOVT_UINT32, 0},
#endif
#ifdef HWA_QT_TEST
	{"hwa_rxpkt_add", IOV_HWA_RXPKT_ADD, (0), 0, IOVT_UINT32, 0},
	{"hwa_rxpkt_del", IOV_HWA_RXPKT_DEL, (0), 0, IOVT_UINT32, 0},
	{"hwa_rxpkt_alt", IOV_HWA_RXPKT_ALT, (0), 0, IOVT_UINT32, 0},
	{"hwa_rxpkt_alt_times", IOV_HWA_RXPKT_ALT_TIMES, (0), 0, IOVT_UINT32, 0},
	{"hwa_rxpkt_alt_add", IOV_HWA_RXPKT_ALT_ADD, (0), 0, IOVT_UINT32, 0},
#endif
#endif /* HWA_PKTPGR_BUILD */

#ifdef HWA_QT_TEST
	{"hwa_kflag", IOV_HWA_KFLAG, (0), 0, IOVT_UINT32, 0},
#endif
	{"hwa_txcpl_mon", IOV_HWA_TXCPL_MON, (0), 0, IOVT_BOOL, 0},
	{NULL, 0, 0, 0, 0, 0}
};

static int hwa_doiovar(void *ctx, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif);
static int hwa_up(void *hdl);
static int hwa_down(void *hdl);
static void hwa_watchdog(void *hdl);

int
hwa_wlc_module_register(hwa_dev_t *dev)
{
	wlc_info_t *wlc = (wlc_info_t *)dev->wlc;

	/* register module */
	if (wlc_module_register(wlc->pub, hwa_iovars, "hwa", dev, hwa_doiovar,
		hwa_watchdog, hwa_up, hwa_down) != BCME_OK) {
		HWA_ERROR(("%s HWA[%d] wlc_module_register() failed\n", HWA00, dev->unit));
		return BCME_ERROR;
	}

	return BCME_OK;
}

/** HWA iovar handler */
static int
hwa_doiovar(void *ctx, uint32 actionid,
	void *p, uint plen, void *a, uint alen, uint vsize, struct wlc_if *wlcif)
{
	hwa_dev_t *dev;
	wlc_info_t *wlc; /* WLC module */
	HWA_PKTPGR_EXPR(hwa_pktpgr_t *pktpgr);

	int32 *ret_int_ptr;
	bool bool_val;
	int32 int_val = 0;
	int err = BCME_OK;

	ASSERT(ctx != NULL);
	dev = (hwa_dev_t *)ctx;
	wlc = dev->wlc;
	HWA_PKTPGR_EXPR(pktpgr = &dev->pktpgr);

	BCM_REFERENCE(vsize);
	BCM_REFERENCE(alen);
	BCM_REFERENCE(wlcif);
	BCM_REFERENCE(ret_int_ptr);
	BCM_REFERENCE(bool_val);
	BCM_REFERENCE(wlc);

	/* convenience int ptr for 4-byte gets (requires int aligned arg) */
	ret_int_ptr = (int32 *)a;

	if (plen >= (int)sizeof(int_val))
		bcopy(p, &int_val, sizeof(int_val));

	bool_val = (int_val != 0) ? TRUE : FALSE;

	switch (actionid) {
	case IOV_SVAL(IOV_HWA_RX_ENABLE):
		if (bool_val) {
			hwa_rx_enable(dev);
		}
		break;
	case IOV_GVAL(IOV_HWA_MSGLEVEL):
		*ret_int_ptr = hwa_msg_level;
		break;
	case IOV_SVAL(IOV_HWA_MSGLEVEL):
		hwa_msg_level = int_val;
		break;
#if defined(HWA_DUMP)
	case IOV_GVAL(IOV_HWA_PKTDUMPLEVEL):
		*ret_int_ptr = hwa_pktdump_level;
		break;
	case IOV_SVAL(IOV_HWA_PKTDUMPLEVEL):
		hwa_pktdump_level = (uint32)int_val;
		break;
#endif /* HWA_DUMP */

#if defined(HWA_PKTPGR_BUILD)
	case IOV_GVAL(IOV_HWA_PP_DBG):
		*ret_int_ptr = hwa_pp_dbg;
		break;
	case IOV_SVAL(IOV_HWA_PP_DBG):
		hwa_pp_dbg = int_val;
		break;
	case IOV_GVAL(IOV_HWA_D11B_RECYCLE_WAR):
		*ret_int_ptr = dev->d11b_recycle_war;
		break;
	case IOV_SVAL(IOV_HWA_D11B_RECYCLE_WAR):
		dev->d11b_recycle_war = bool_val;
		break;
	case IOV_GVAL(IOV_HWA_RXPKT_COUNT_MAX):
		*ret_int_ptr = pktpgr->pgi_rxpkt_count_max;
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_COUNT_MAX):
		pktpgr->pgi_rxpkt_count_max = (uint16)int_val;
		break;
	case IOV_GVAL(IOV_HWA_RXPKT_REQ_MAX):
		*ret_int_ptr = pktpgr->pgi_rxpkt_req_max;
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_REQ_MAX):
		pktpgr->pgi_rxpkt_req_max = (uint16)int_val;
		break;
#ifdef HWA_PKTPGR_DBM_AUDIT_ENABLED
	case IOV_SVAL(IOV_HWA_DBM_AUDIT_DUMP):
		hwa_pktpgr_dbm_audit_dump(pktpgr, int_val);
		break;
#endif
#ifdef PSPL_TX_TEST
	case IOV_GVAL(IOV_HWA_PSPL):
		*ret_int_ptr = pktpgr->pspl_enable;
		break;
	case IOV_SVAL(IOV_HWA_PSPL):
		pktpgr->pspl_enable = bool_val;
		HWA_PRINT("Set: hwa_pspl 0x%x\n", pktpgr->pspl_enable);
		break;
	case IOV_GVAL(IOV_HWA_PSPL_MPDU):
		*ret_int_ptr = pktpgr->pspl_mpdu_enable;
		break;
	case IOV_SVAL(IOV_HWA_PSPL_MPDU):
		pktpgr->pspl_mpdu_enable = bool_val;
		if (!bool_val) { // turn pspl_mpdu_append_enable off as well
			pktpgr->pspl_mpdu_append_enable = bool_val;
		}
		HWA_PRINT("Set: hwa_pspl_mpdu 0x%x\n", pktpgr->pspl_mpdu_enable);
		break;
	case IOV_GVAL(IOV_HWA_PSPL_MPDU_APPEND):
		*ret_int_ptr = pktpgr->pspl_mpdu_append_enable;
		break;
	case IOV_SVAL(IOV_HWA_PSPL_MPDU_APPEND):
		pktpgr->pspl_mpdu_append_enable = bool_val;
		if (bool_val) { // turn pspl_mpdu_enable on as well
			pktpgr->pspl_mpdu_enable = bool_val;
		}
		HWA_PRINT("Set: hwa_pspl_mpdu_append 0x%x\n", pktpgr->pspl_mpdu_append_enable);
		break;
	case IOV_GVAL(IOV_HWA_PSPL_PGO_MPDU):
		*ret_int_ptr = pktpgr->pspl_pgo_mpdu_enable;
		break;
	case IOV_SVAL(IOV_HWA_PSPL_PGO_MPDU):
		pktpgr->pspl_pgo_mpdu_enable = bool_val;
		HWA_PRINT("Set: hwa_pspl_pgo_mpdu 0x%x\n", pktpgr->pspl_pgo_mpdu_enable);
		break;
#endif /* PSPL_TX_TEST */
#ifdef HWA_QT_TEST
	case IOV_SVAL(IOV_HWA_RXPKT_ADD):
		pktpgr->rx_alt_en = FALSE;
		hwa_rxpktpool_add(dev, (uint16)int_val, 1);
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_DEL):
		hwa_rxpktpool_del(dev, (uint16)int_val);
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_ALT):
		pktpgr->rx_alt_en = int_val ? TRUE : FALSE;
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_ALT_TIMES):
		pktpgr->rx_alt_times = (uint16)int_val;
		break;
	case IOV_SVAL(IOV_HWA_RXPKT_ALT_ADD):
		HWA_PRINT("Do hwa_rxpktpool_add %d packets %d times\n",
			int_val, pktpgr->rx_alt_times);
		pktpgr->rx_alt_count = (uint16)int_val;
		hwa_rxpktpool_add(dev, (uint16)int_val, pktpgr->rx_alt_times);
		break;
#endif /* HWA_QT_TEST */

	case IOV_GVAL(IOV_HWA_TXS_DDBM_RESV):
		*ret_int_ptr = pktpgr->txs_ddbm_resv;
		break;
	case IOV_SVAL(IOV_HWA_TXS_DDBM_RESV):
		pktpgr->txs_ddbm_resv = (uint32)int_val;
		break;

#endif /* HWA_PKTPGR_BUILD */

#ifdef HWA_QT_TEST
	case IOV_GVAL(IOV_HWA_KFLAG):
		*ret_int_ptr = hwa_kflag;
		break;
	case IOV_SVAL(IOV_HWA_KFLAG):
		hwa_kflag = int_val;
		HWA_PRINT("Set: hwa_kflag 0x%x\n", hwa_kflag);
		break;
#endif

	case IOV_GVAL(IOV_HWA_TXCPL_MON):
		*ret_int_ptr = dev->txcpl_monitor;
		break;
	case IOV_SVAL(IOV_HWA_TXCPL_MON):
		dev->txcpl_monitor = bool_val;
		break;

	default:
		err = BCME_UNSUPPORTED;
		break;
	}

	if (!IOV_ISSET(actionid))
		return err;

	return err;
}

static void
hwa_watchdog(void *hdl)
{
	HWA_TXCPLE_EXPR({
		hwa_dev_t *dev = (hwa_dev_t *)hdl;
		if (dev->txcpl_monitor) {
			hwa_txcpl_monitor(dev);
		}
	});
} /* hwa_watchdog */

static int
hwa_up(void *hdl)
{
	hwa_dev_t *dev = (hwa_dev_t *)hdl;

	HWA_FTRACE(HWA00);

	HWA_ASSERT(dev != (hwa_dev_t*)NULL);

#ifdef DONGLEBUILD
	HWA_DPC_EXPR(hwa_intrsoff(dev));
#endif

	if (dev->reinit) {
		hwa_reinit(dev);
	} else {
		/* Initialize HWA core */
		hwa_init(dev);

		hwa_enable(dev);

		dev->up = TRUE;

		HWA_RXPOST_EXPR(hwa_wlc_mac_event(dev, WLC_E_HWA_RX_POST));
	}

#ifdef DONGLEBUILD
	HWA_DPC_EXPR(hwa_intrson(dev));

	HWA_TRACE(("\n\n----- HWA[%d] INTR ON: intmask<0x%08x> defintmask<0x%08x> "
		"intstatus<0x%08x> -----\n\n", dev->unit, dev->intmask, dev->defintmask,
		dev->intstatus));
#endif /* DONGLEBUILD */

	// Do SW txlbufpool refill, rxlbufpool fill will be delay
	// after Host enable Rx module in hwa_rx_enable.
	// Because we need to make sure we have RXPOST WI.
	HWA_PKTPGR_EXPR(hwa_txpost_txlbufpool_fill(dev));

#if defined(HNDPQP)
	HWA_PKTPGR_EXPR(hwa_pktpgr_pqplbufpool_fill(dev));
#endif

	return BCME_OK;
}

static int
hwa_down(void *hdl)
{
	hwa_dev_t *dev = (hwa_dev_t *)hdl;

	HWA_FTRACE(HWA00);

	HWA_ASSERT(dev != (hwa_dev_t*)NULL);

	if (!dev->up)
		return BCME_OK;

	// Make sure all local packet allocation will be put back.
	HWA_PKTPGR_EXPR(hwa_pktpgr_pageout_local_rsp_wait_to_finish(dev));

#ifdef DONGLEBUILD
	HWA_DPC_EXPR(hwa_intrsoff(dev));
#endif

	hwa_disable(dev);

	hwa_deinit(dev);

	dev->up = FALSE;

	return BCME_OK;
}

#if defined(BCMDBG) || defined(HWA_DUMP)
#define HWA_DUMP_ARGV_MAX	64
#define HWA_DUMP_FIFO_MAX	128
static int
hwa_dump_parse_args(wlc_info_t *wlc, char *args, uint32 *block_bitmap,
	bool *verbose, bool *dump_regs, bool *dump_txfifo_shadow, uint8 *fifo_bitmap,
	bool *help)
{
	int i, err = BCME_OK;
	char *p, **argv = NULL;
	uint argc = 0;
	char opt, curr = '\0';
	uint32 val32;

	if (args == NULL || block_bitmap == NULL || verbose == NULL ||
		dump_regs == NULL || dump_txfifo_shadow == NULL) {
		err = BCME_BADARG;
		goto exit;
	}

	/* allocate argv */
	if ((argv = MALLOC(wlc->osh, sizeof(*argv) * HWA_DUMP_ARGV_MAX)) == NULL) {
		HWA_ERROR(("wl%d: %s: failed to allocate the argv buffer\n",
		          wlc->pub->unit, __FUNCTION__));
		goto exit;
	}

	/* get each token */
	p = bcmstrtok(&args, " ", 0);
	while (p && argc < HWA_DUMP_ARGV_MAX-1) {
		argv[argc++] = p;
		p = bcmstrtok(&args, " ", 0);
	}
	argv[argc] = NULL;

	/* initial default */
	*block_bitmap = HWA_DUMP_ALL;
	*verbose = FALSE;
	*dump_regs = FALSE;
	*dump_txfifo_shadow = FALSE;
	*help = FALSE;

	/* parse argv */
	argc = 0;
	while ((p = argv[argc++])) {
		if (!strncmp(p, "-", 1)) {
			if (strlen(p) > 2) {
				err = BCME_BADARG;
				goto exit;
			}
			opt = p[1];

			switch (opt) {
				case 'b':
					curr = 'b';
					*block_bitmap = 0;
					break;
				case 'v':
					curr = 'v';
					*verbose = TRUE;
					break;
				case 'r':
					curr = 'r';
					*dump_regs = TRUE;
					break;
				case 's':
					curr = 's';
					*dump_txfifo_shadow = TRUE;
					break;
				case 'f':
					curr = 'f';
					for (i = 0; i < HWA_DUMP_FIFO_MAX; i++) {
						clrbit(fifo_bitmap, i);
					}
					break;
				case 'h':
					curr = 'h';
					*help = TRUE;
					break;
				default:
					err = BCME_BADARG;
					goto exit;
			}
		} else {
			switch (curr) {
				case 'b':
					if (!strcmp(p, "all")) {
						*block_bitmap = HWA_DUMP_ALL;
					} else if (!strcmp(p, "top")) {
						*block_bitmap |= HWA_DUMP_TOP;
					} else if (!strcmp(p, "cmn")) {
						*block_bitmap |= HWA_DUMP_CMN;
					} else if (!strcmp(p, "dma")) {
						*block_bitmap |= HWA_DUMP_DMA;
					} else if (!strcmp(p, "1a")) {
						*block_bitmap |= HWA_DUMP_1A;
					} else if (!strcmp(p, "1b")) {
						*block_bitmap |= HWA_DUMP_1B;
					} else if (!strcmp(p, "2a")) {
						*block_bitmap |= HWA_DUMP_2A;
					} else if (!strcmp(p, "2b")) {
						*block_bitmap |= HWA_DUMP_2B;
					} else if (!strcmp(p, "3a")) {
						*block_bitmap |= HWA_DUMP_3A;
					} else if (!strcmp(p, "3b")) {
						*block_bitmap |= HWA_DUMP_3B;
					} else if (!strcmp(p, "4a")) {
						*block_bitmap |= HWA_DUMP_4A;
					} else if (!strcmp(p, "4b")) {
						*block_bitmap |= HWA_DUMP_4B;
					} else if (!strcmp(p, "pp")) {
						*block_bitmap |= HWA_DUMP_PP;
					}
					break;
				case 'f':
					if (strcmp(p, "all") == 0) {
						for (i = 0; i < HWA_DUMP_FIFO_MAX; i++) {
							setbit(fifo_bitmap, i);
						}
					}
					else {
						val32 = (uint32)bcm_strtoul(p, NULL, 0);
						if (val32 < HWA_DUMP_FIFO_MAX) {
							setbit(fifo_bitmap, val32);
						}
					}
					break;
				default:
					err = BCME_BADARG;
					goto exit;
			}
		}
	}

exit:
	if (argv) {
		MFREE(wlc->osh, argv, sizeof(*argv) * HWA_DUMP_ARGV_MAX);
	}

	return err;
}

static int
_hwa_dump(hwa_dev_t *dev, char *dump_args, struct bcmstrbuf *b)
{
	int i, err = BCME_OK;
	uint32 block_bitmap = HWA_DUMP_ALL;
	bool verbose = FALSE;
	bool dump_regs = FALSE;
	bool dump_txfifo_shadow = FALSE;
	bool help = FALSE;
	uint8 fifo_bitmap[(HWA_DUMP_FIFO_MAX/NBBY)+1] = {0};

	/* Set default value for legacy dump */
	for (i = 0; i < HWA_DUMP_FIFO_MAX; i++) {
		setbit(fifo_bitmap, i);
	}

	/* Parse args if needed */
	if (dump_args) {
		err = hwa_dump_parse_args(dev->wlc, dump_args, &block_bitmap, &verbose,
			&dump_regs, &dump_txfifo_shadow, fifo_bitmap, &help);
		if (err != BCME_OK)
			return err;

		if (help) {
			char *help_str_wl = "Usage: wl dump hwa [-b <blocks> -v -r -s "
				"-f <HWA fifos> -h]\n";
			char *help_str_dhd = "Usage: hwa dump -b <blocks> -v -r -s "
				"-f <HWA fifos> -h\n";
			char *help_str_cmn =
				"       blocks: <top> <cmn> <dma> <1a> <1b> <2a> <2b>"
				" <3a> <3b> <4a> <4b>"
#if defined(HWA_PKTPGR_BUILD)
				" <pp>"
#endif
				"\n"
				"       v: verbose\n"
				"       r: dump registers\n"
				"       s: dump txfifo shadow\n"
				"       f: specific fifos\n";
			HWA_BPRINT(b, "%s%s", b ? help_str_wl : help_str_dhd, help_str_cmn);
			return BCME_OK;
		}
	}

	hwa_dump(dev, b, block_bitmap, verbose, dump_regs, dump_txfifo_shadow, fifo_bitmap);

	return BCME_OK;
}

/* wl dump hwa [-b <blocks> -v -r -s -f <HWA fifos> -h] */
int
hwa_wl_dump(struct hwa_dev *dev, struct bcmstrbuf *b)
{
	wlc_info_t *wlc = (wlc_info_t *)dev->wlc;

	return _hwa_dump(dev, wlc->dump_args, b);
}

/* dhd cons "hwa dump -b <blocks> -v -r -s -f <HWA fifos> -h" */
int
hwa_dhd_dump(struct hwa_dev *dev, char *dump_args)
{
	return _hwa_dump(dev, dump_args, NULL);
}

#endif

#if defined(BCM_BUZZZ_KPI_QUE_LEVEL) && (BCM_BUZZZ_KPI_QUE_LEVEL > 0)
uint8 * // Log all MAC facing queues
buzzz_mac(uint8 *buzzz_log)
{
	HWA_TXFIFO_EXPR(buzzz_log = buzzz_mac_txfifo(buzzz_log));
	return buzzz_log;
}
#endif /* BCM_BUZZZ_KPI_QUE_LEVEL */
