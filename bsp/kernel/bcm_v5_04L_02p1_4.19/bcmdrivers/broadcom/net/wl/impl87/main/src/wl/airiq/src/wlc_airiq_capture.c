/*
 * @file
 * @brief
 *
 *  Air-IQ data capture
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
 */

#include <wlc_airiq.h>

#include <wlc_event_utils.h>
#include <wlc_event.h>
#include <wlc_vasip.h>
#include <wl_tmr.h>

uint32 * wlc_airiq_fftbuf_readbytes(airiq_info_t *airiqh, uint32 bytes, uint32 addr,
	uint32 **wrapaddr, uint32 *wrapbytes);

int16 wlc_lte_u_rssi_scale_per_chanspec(uint16 chanspec);
uint8 chan2index(uint16 chanspec);
uint32 lte_u_2560log10(uint32 x);
static int32 lte_u_log2(int32 v);
void wlc_lte_u_vasipiqcapture(airiq_info_t *airiqh);
uint32 * wlc_lte_u_iqbuf_readbytes(airiq_info_t *airiqh, uint32 bytes, uint32 addr);
bool wlc_lte_u_create_iqbuf(airiq_info_t *airiqh);
void wlc_lte_u_free_iqbuf(airiq_info_t *airiqh);

/* channel to frequency conversion */
static int
wlc_airiq_phy_chan2fc(uint channel)
{
	/* go from channel number (such as 6) to carrier freq (such as 2442) */
	if (channel >= 184 && channel <= 228) {
		return channel * 5 + 4000;
	} else if (channel >= 32 && channel <= 180) {
		return channel * 5 + 5000;
	} else if (channel >= 1 && channel <= 13) {
		return channel * 5 + 2407;
	} else if (channel == 14) {
		return 2484;
	} else {
		return -1;
	}
}

#ifdef VASIP_HW_SUPPORT
void wlc_airiq_vasipfftcapture(airiq_info_t *airiqh)
{
	uint8 *buffer;
	airiq_fftdata_header_t *hdr;
	int32 datalen = 0;
	uint32 words2read;
	chanspec_t chanspec = 0, chanspec3x3 = 0;
	uint16 seqno = 0;
	uint32 timestamp, gain, bw;
	uint16 *header_ptr;
	uint16 vasip_fft_header[VASIP_FFT_HEADER_SIZE / 2];
	uint16 hdr_bins, hdr_data_bytes;
	bool force_sendup = FALSE;

	airiqh->fft_count++;
	if (airiqh->capture_limit > 0 &&
		airiqh->fft_count >= airiqh->capture_limit && airiqh->iq_capture_enable) {

		wlc_airiq_phy_disable_fft_capture(airiqh);

		/* sendup the message if last FFT on a channel */
		force_sendup = TRUE;

		if (airiqh->scan.home_scan) {
			if (airiqh->sweep_count >= 0) {
				airiqh->sweep_count--;
				if (airiqh->sweep_count <= 0) {
					wlc_airiq_set_enable(airiqh, FALSE);
					airiqh->scan.run_phycal = FALSE;
					/* chanspec restore, disable FFT, disable scan */
					wl_airiq_sendup_scan_complete_alternate(airiqh,
						AIRIQ_SCAN_SUCCESS);
				}
			}
		}
	}

	// Read the FFT Header and extract BW
	/*
	* wlc_svmp_mem_read64(airiqh->wlc->hw, (uint64*)airiqh->fft_buffer,
	*	SVMP_HEADER_ADDR, VASIP_FFT_HEADER_SIZE / 8);
	*/

	wlc_svmp_mem_read_axi(airiqh->wlc->hw, vasip_fft_header,
		airiqh->svmp_header_addr, VASIP_FFT_HEADER_SIZE / 2);

	header_ptr  = (uint16*)vasip_fft_header;
	gain        = ltoh16(vasip_fft_header[3]);
	timestamp   = ltoh32(*(uint32*)(header_ptr + 18));
	seqno       = ltoh16(vasip_fft_header[20]);
	chanspec    = ltoh16(vasip_fft_header[21]);
	chanspec3x3 = ltoh16(vasip_fft_header[22]);

#ifdef BCMDBG
	wlc_airiq_log_fft(airiqh, CHSPEC_CHANNEL(chanspec), 0);
#endif

	bw = CHSPEC_BW(chanspec);

	switch (bw) {
	case WL_CHANSPEC_BW_20:
		datalen =  FFT_SZ(20) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(20);
		hdr_data_bytes = FFT_SZ(20);
		break;
	case WL_CHANSPEC_BW_40:
		datalen =  FFT_SZ(40) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(40);
		hdr_data_bytes = FFT_SZ(40);
		break;
	case WL_CHANSPEC_BW_80:
		datalen = FFT_SZ(80) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(80);
		hdr_data_bytes = FFT_SZ(80);
		break;
	default:
		WL_ERROR(("wl%d: %s: Unknown bandwidth: %d bw\n",
			WLCWLUNIT(airiqh->wlc), __FUNCTION__, bw));
		prhex("FFT header:", (uchar*)header_ptr, VASIP_FFT_HEADER_SIZE);
		//ASSERT(0);
		return;
	}

	buffer = wlc_airiq_msg_get_buffer(airiqh, datalen);

	if (! buffer) {
		WL_ERROR(("wl%d %s: buffer fail.\n", WLCWLUNIT(airiqh->wlc), __FUNCTION__));
		return;
	}

	hdr = (airiq_fftdata_header_t *)buffer;

	/* This assert is not needed since the 64-bit read has been disabled.
	 * ASSERT(ISALIGNED(buffer + sizeof(airiq_fftdata_header_t), sizeof(uint64)));
	 */
	/* simplified logic -- buffer is sized to handle the extra uint64 */
	words2read = ((datalen - sizeof(airiq_fftdata_header_t)) >> 3) + 1;

	/* Read the FFT data */
	/* wlc_svmp_mem_read64(airiqh->wlc->hw,
	*    (uint64*)(buffer + sizeof(airiq_fftdata_header_t)),
	*    SVMP_FFT_DATA_ADDR, words2read);
	*/
	wlc_svmp_mem_read_axi(airiqh->wlc->hw,
		(uint16*)(buffer + sizeof(airiq_fftdata_header_t)),
		airiqh->svmp_fft_data_addr, 4*words2read);

	/* Setup the data hdr */
	hdr->timestamp = timestamp;
	hdr->seqno = seqno;
	hdr->flags = 0;
	hdr->message_type = MESSAGE_TYPE_FFTCPX_VASIP;
	hdr->corerev = airiqh->wlc->pub->corerev;
	hdr->unit = airiqh->wlc->pub->unit;
	hdr->size_bytes = datalen;
	hdr->fc_mhz = (uint16)wlc_airiq_phy_chan2fc(CHSPEC_CHANNEL(chanspec));
	hdr->chanspec = chanspec;
	hdr->gaincode = gain;
	hdr->fc_3x3_mhz = (uint16)wlc_airiq_phy_chan2fc(CHSPEC_CHANNEL(chanspec3x3));
	hdr->bins = hdr_bins;
	hdr->data_bytes = hdr_data_bytes;

	wlc_airiq_msg_sendup(airiqh, datalen, force_sendup);

	/* AIRIQ_DBG(("wl%d: %s: chanspec=%x size=%d timestamp=%08d seqno=%x\n",
	 *  WLCWLUNIT(airiqh->wlc), __FUNCTION__,hdr->chanspec,datalen,hdr->timestamp,hdr->seqno));
	 */

	/* clean V2H_STATUS flag */
	wlc_airiq_phy_clear_svmp_status(airiqh);
	if (D11REV_IS(airiqh->wlc->pub->corerev, 64)) {
		wlc_airiq_phy_ack_vasip_fft(airiqh);
	}
}

static INLINE int
__wlc_airiq_xfer_m2mcpy(wlc_info_t *wlc, void * dst_addr,
                         int dst_len, uint32 *svmp_addrp_u32, void *xfer_arg)
{
	m2m_dd_cpy_t cpy_key;
	dma64addr_t xfer_src, xfer_dst;
	uintptr xfer_dst_u64;

	if (m2m_dd_avail(wlc->osh, wlc->airiq->m2m_dd_airiq) < 1) {
		return BCME_NORESOURCE;
	}

	/* Determine the destination in local circular ring */
	xfer_src.hiaddr = 0U;
	xfer_src.loaddr = (uint32)((uintptr)svmp_addrp_u32);
#ifdef VIRT_TO_PHYS
	xfer_dst_u64 = (uintptr)VIRT_TO_PHYS(dst_addr);
#else
	xfer_dst_u64 = (uintptr)(dst_addr);
#endif
#if defined(__ARM_ARCH_7A__)
	xfer_dst.hiaddr = 0U;
#else
	xfer_dst.hiaddr = (uint32)(((uint64)xfer_dst_u64) >> 32);
#endif
	xfer_dst.loaddr = (uint32)(xfer_dst_u64);

	cpy_key	= m2m_dd_xfer(wlc->osh, wlc->airiq->m2m_dd_airiq, &xfer_src,
	                      &xfer_dst, dst_len ,
	                      xfer_arg,
	                      M2M_DD_XFER_COMMIT | M2M_DD_XFER_RESUME);
	if (cpy_key == M2M_INVALID) {
		WL_ERROR(("wl%d: %s: m2m_dd_xfer failure",
			WLCWLUNIT(wlc), __FUNCTION__));
		ASSERT(0);
	}
	return BCME_OK;
}

void wlc_airiq_vasipfftcapture_m2m(airiq_info_t *airiqh)
{
	uint8 *buffer;
	int32 datalen = 0;
	uint32 bw;
	uint16 chanspec_val;
	uint32 *svmp_addrp_u32;
	int result =0;
	uint32 dummy_tsf_high;

	airiqh->fft_count++;

	chanspec_val = airiqh->scan_chanspec;
	bw = CHSPEC_BW(chanspec_val);

	switch (bw) {
	case WL_CHANSPEC_BW_20:
		datalen =  FFT_SZ(20) + sizeof(airiq_fftdata_header_t);
		break;
	case WL_CHANSPEC_BW_40:
		datalen =  FFT_SZ(40) + sizeof(airiq_fftdata_header_t);
		break;
	case WL_CHANSPEC_BW_80:
		datalen = FFT_SZ(80) + sizeof(airiq_fftdata_header_t);
		break;
	default:
		WL_ERROR(("wl%d: %s: Unknown bandwidth: %d bw\n",
			WLCWLUNIT(airiqh->wlc), __FUNCTION__, bw));
		/* clean V2H_STATUS flag */
		wlc_airiq_phy_clear_svmp_status(airiqh);
		if (D11REV_IS(airiqh->wlc->pub->corerev, 64)) {
			wlc_airiq_phy_ack_vasip_fft(airiqh);
		}
		return;
	}
	datalen = ALIGN_SIZE(datalen,64); /* align to 64 bytes*/
	buffer = airiqh->m2m_buffer;
	svmp_addrp_u32 = wlc_vasip_addr_int(airiqh->wlc->hw, airiqh->svmp_header_addr);
	/* M2M DMA from vasip memory to DMA buffer*/
	result = __wlc_airiq_xfer_m2mcpy(airiqh->wlc, buffer,
                         datalen, svmp_addrp_u32, buffer);

	if( result == BCME_NORESOURCE) {
		/* clean V2H_STATUS flag */
		wlc_airiq_phy_clear_svmp_status(airiqh);
		if (D11REV_IS(airiqh->wlc->pub->corerev, 64)) {
			wlc_airiq_phy_ack_vasip_fft(airiqh);
		}
		airiqh->dma_drop_count++;
	}
	wlc_read_tsf(airiqh->wlc, &airiqh->dma_start_tsf, &dummy_tsf_high);

	airiqh->m2m_dma_inprogress=TRUE;
}

void // function callback at the completion of the DD-based M2M xfer
wlc_airiq_m2m_dd_done_cb(void *usr_cbdata,
    dma64addr_t *xfer_src, dma64addr_t *xfer_dst, int xfer_len, void *xfer_arg)
{
	airiq_info_t *airiqh = (airiq_info_t *)usr_cbdata;
	uint8 *m2m_buffer= xfer_arg, *fft_buffer;
	chanspec_t chanspec = 0, chanspec3x3 = 0;
	uint16 seqno = 0;
	uint32 timestamp, gain;
	airiq_fftdata_header_t *hdr;
	uint32 bw;
	int32 datalen;
	uint16 *header_ptr;
	uint16 hdr_bins, hdr_data_bytes;

	ASSERT(airiqh);
	ASSERT(m2m_buffer);

	OSL_CACHE_INV((void*)(xfer_arg), xfer_len);

	header_ptr  = (uint16*)m2m_buffer;
	gain        = ltoh16(header_ptr[3]);
	timestamp   = ltoh32(*(uint32*)(header_ptr + 18));
	seqno       = ltoh16(header_ptr[20]);
	chanspec    = ltoh16(header_ptr[21]);
	chanspec3x3 = ltoh16(header_ptr[22]);
	bw = CHSPEC_BW(chanspec);
	switch (bw) {
	case WL_CHANSPEC_BW_20:
		datalen =  FFT_SZ(20) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(20);
		hdr_data_bytes = FFT_SZ(20);
		break;
	case WL_CHANSPEC_BW_40:
		datalen =  FFT_SZ(40) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(40);
		hdr_data_bytes = FFT_SZ(40);
		break;
	case WL_CHANSPEC_BW_80:
		datalen = FFT_SZ(80) + sizeof(airiq_fftdata_header_t);
		hdr_bins = FFT_BINS_CNT(80);
		hdr_data_bytes = FFT_SZ(80);
		break;
	default:
		WL_ERROR(("wl%d: %s: Unknown bandwidth: %d bw\n",
			WLCWLUNIT(airiqh->wlc), __FUNCTION__, bw));
		prhex("FFT header:", (uchar*)header_ptr, VASIP_FFT_HEADER_SIZE);
		return;
	}

	fft_buffer = wlc_airiq_msg_get_buffer(airiqh, datalen);

	//write the FFT header to fft_buffer
	hdr = (airiq_fftdata_header_t *)fft_buffer;
	/* Setup the data hdr */
	hdr->timestamp = timestamp;
	hdr->seqno = seqno;
	hdr->flags = 0;
	hdr->message_type = MESSAGE_TYPE_FFTCPX_VASIP;
	hdr->corerev = airiqh->wlc->pub->corerev;
	hdr->unit = airiqh->wlc->pub->unit;
	hdr->size_bytes = datalen;
	hdr->fc_mhz = (uint16)wlc_airiq_phy_chan2fc(CHSPEC_CHANNEL(chanspec));
	hdr->chanspec = chanspec;
	hdr->gaincode = gain;
	hdr->fc_3x3_mhz = (uint16)wlc_airiq_phy_chan2fc(CHSPEC_CHANNEL(chanspec3x3));
	hdr->bins = hdr_bins;
	hdr->data_bytes = hdr_data_bytes;

	/* copy the FFT  data to fft_buffer */
	memcpy(fft_buffer+sizeof(airiq_fftdata_header_t), m2m_buffer + VASIP_FFT_HEADER_SIZE,
			hdr->data_bytes);
	wlc_airiq_msg_sendup(airiqh, datalen, FALSE);

	airiqh->m2m_dma_inprogress=FALSE;
	airiqh->fft_dma_count++;
	/* clean V2H_STATUS flag - Handshake to start next FFT capture  */
	wlc_airiq_phy_clear_svmp_status(airiqh);
	if (D11REV_IS(airiqh->wlc->pub->corerev, 64)) {
		wlc_airiq_phy_ack_vasip_fft(airiqh);
	}

	return;
}

#if !defined(DONGLEBUILD)
bool BCMFASTPATH // ISR for M2M interrupts
wlc_airiq_m2m_isr(wlc_info_t *wlc, bool *wantdpc)
{
	void *cbdata = (void *)m2m_dd_eng_get(wlc->osh,
	   wlc->airiq->m2m_dd_airiq);
	return m2m_dd_isr(cbdata, wantdpc);
}

void BCMFASTPATH // DPC for M2M interrupts
wlc_airiq_m2m_dpc(wlc_info_t *wlc)
{
	void *cbdata = (void *)m2m_dd_eng_get(wlc->osh,
	   wlc->airiq->m2m_dd_airiq);
	m2m_dd_worklet(cbdata);
}

uint // M2M core bitmap in oobselouta30 reg
wlc_airiq_m2m_si_flag(wlc_info_t *wlc)
{
	return m2m_dd_si_flag_get(wlc->osh);
}

void // Disable M2M interrupts
wlc_airiq_m2m_intrsoff(wlc_info_t *wlc)
{
	void *m2m_eng;

	/* Get the M2MDMA engine pointer */
	m2m_eng = (void *)m2m_dd_eng_get(wlc->osh,
	   wlc->airiq->m2m_dd_airiq);

	/* Disable channel-based M2M interrupts for AirIQ */
	m2m_intrsoff(wlc->osh, m2m_eng);
} /* wlc_sts_xfer_intrsoff() */
void // Enable M2M interrupts

wlc_airiq_m2m_intrson(wlc_info_t *wlc)
{
	void *m2m_eng;

	/* Get the M2MDMA engine pointer */
	m2m_eng = (void *)m2m_dd_eng_get(wlc->osh,
	   wlc->airiq->m2m_dd_airiq);

	/* Enable channel-based M2M interrupts for airiq m2m */
	m2m_intrson(wlc->osh, m2m_eng);
} /* wlc_airiq_m2m_intrsoff() */

#endif

// Approximate integer log2() using a lookup table and some mathematical
// tricks.
static int32 lte_u_log2(int32 v)
{
	int32 r;  // result goes here
	int32 rr;

	static const int MultiplyDeBruijnBitPosition[32] =
	{
		0, 9,  1,  10, 13, 21, 2,  29, 11, 14, 16, 18, 22, 25, 3, 30,
		8, 12, 20, 28, 15, 17, 24, 7,  19, 27, 23, 6,  26, 5,  4, 31
	};

	v |= v >> 1; // first round down to one less than a power of 2
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	rr = (v * 0x7C4ACDDU) >> 27;
	r = MultiplyDeBruijnBitPosition[rr];

	return r;
}

// Hybrid LUT/linear interpolation, 256 * 10*log10(x).
// Right shift by 8 bits to get the 10*log10() value.
uint32 lte_u_2560log10(uint32 x)
{
	uint32 r;      /* result goes here */
	uint32 log2;
	uint32 x0, x1, y0, y1;
	/* this lut is from x = 2^(n/2) to 2560*log10(x), n=0..31 */
	static uint32 xlut[65] = {
	0,         1,         2,         3,         4,	        6,        8,        11,
	16,        23,        32,        45,        64,         91,       128,      181,
	256,	   362,       512,       724,       1024,       1448,     2048,     2896,
	4096,      5793,      8192,      11585,	    16384,      23170,	  32768,    46431,
	65536,     92682,     131072,	 185364,    262144,     370728,   524288,   741455,
	1048576,   1482910,   2097152,   2965821,   4194304,    5931642,  8388608, 11863283,
	16777216,  23726566,  33554432,  47453133,  67108864,   94906266, 134217728, 189812531,
	268435456, 379625062, 536870912, 759250125, 1073741824, 1518500250, 2147483648UL,
	3037000500UL, 0xffffffff };

	static int32 lut[65] = {
	0,	0,     771,   1221,  1541,  1927,  2312,  2679,	 3083,	3468,  3853,  4239,  4624,
	5009,	5394,  5780,  6165,  6550,  6936,  7321,  7706,	 8092,	8477,  8862,  9248,  9633,
	10018, 10404, 10789, 11174, 11560, 11945, 12330, 12716, 13101, 13486, 13871, 14257, 14642,
	15027, 15413, 15798, 16183, 16569, 16954, 17339, 17725, 18110, 18495, 18881, 19266, 19651,
	20037, 20422, 20807, 21193, 21578, 21963, 22348, 22734, 23119, 23504, 23890, 24275, 24660
	};

	log2 = 2 * lte_u_log2(x);
	/* this will be in the range 0 to 16 (normally 15) */
	if (x > xlut[log2 + 1]) {
		log2++;
	}
	x0 = xlut[log2];
	x1 = xlut[log2 + 1];
	y0 = lut[log2];
	y1 = lut[log2 + 1];

	// linear interpolation
	r = (y1 - y0) * (x - x0) / (x1 - x0) + y0;

	return r;
}

// channel to index for RSSI scale
uint8 chan2index(uint16 chanspec)
{
	uint16 bw;
	uint16 channel;
	uint8 index;

	bw = CHSPEC_BW(chanspec);
	channel = CHSPEC_CHANNEL(chanspec);

	switch (bw) {
	case WL_CHANSPEC_BW_20:
		if (channel <= 48) {
			// Index 0-3 for channel 36-48
			index = channel / 4 - 9;
		} else {
			// Index 4-8 for channel 149-165
			index = (channel - 1) / 4 - 33;
		}
		break;
	case WL_CHANSPEC_BW_40:
		if (channel <= 48) {
			// Index 9-12 for channel 36-48
			index = channel / 4;
		} else {
			// Index 13-17 for channel 149-165
			index = (channel - 1) / 4 - 24;
		}
		break;
	case WL_CHANSPEC_BW_80:
		if (channel <= 48) {
			// Index 18-21 for channel 36-48
			index = channel / 4 + 9;
		} else {
			// Index 22-26 for channel 149-165
			index = (channel - 1) / 4 - 15;
		}
		break;
	default:
		index = 0;
		WL_ERROR(("%s: chanspec 0x%x unknown bw 0x%x\n",
			__FUNCTION__, chanspec, bw));
		break;
	}
	return index;
}

// RSSI scale per chanspec
int16 wlc_lte_u_rssi_scale_per_chanspec(uint16 chanspec)
{
	int16 rssi_scale[27] = {
		-98, -95, -97, -97, -96, -95, -96, -96, -96, //20MHz index[0-8]
		-97, -94, -97, -96, -96, -95, -95, -96, -96, //40MHz index[9-17]
		-97, -93, -96, -95, -95, -94, -95, -95, -95 //80MHz index[18-26]
	};

	return rssi_scale[chan2index(chanspec)];
}

void wlc_lte_u_detection_status(airiq_info_t *airiqh)
{
return;
}

/* DPC for VASIP FFT */
void wlc_airiq_vasip_fft_worklet(wlc_info_t *wlc)
{
	if (wlc->airiq->scan_type == SCAN_TYPE_AIRIQ) {
		/* read fft from svmp */
#if defined(DONGLEBUILD)
		wlc_airiq_vasipfftcapture(wlc->airiq);
#else
	if(wlc->airiq->vasip_4x4mode )
			wlc_airiq_vasipfftcapture_m2m(wlc->airiq);
		else
			wlc_airiq_vasipfftcapture(wlc->airiq);
#endif
	} else {
		if (lte_u_debug_capture_status(wlc->airiq)) {
			wlc_lte_u_vasipiqcapture(wlc->airiq);
		} else {
			/* read LTE_U detection status from svmp */
			wlc_lte_u_detection_status(wlc->airiq);
		}
	}
}

void lte_u_set_debug_capture(airiq_info_t *airiqh)
{
	wlc_svmp_mem_set_axi(airiqh->wlc->hw, SVMP_LTE_U_DEBUG_CAPTURE, 1,
		airiqh->debug_capture);
return;
}

bool lte_u_debug_capture_status(airiq_info_t* airiqh)
{
	return airiqh->debug_capture;
}

uint32 * wlc_lte_u_iqbuf_readbytes(airiq_info_t *airiqh, uint32 bytes, uint32 addr)
{
	wlc_svmp_mem_read64(airiqh->wlc->hw, (uint64*)(airiqh->iqbuf), addr, bytes / 8);
	return (uint32*)airiqh->iqbuf;
}

bool wlc_lte_u_create_iqbuf(airiq_info_t *airiqh)
{
	airiqh->iqbuf_size = IQMSG_SIZE;

	// Allocate iqbuf
	if ((airiqh->iqbuf = (uint8*)MALLOC(airiqh->wlc->osh, airiqh->iqbuf_size)) == NULL) {
		WL_ERROR(("wl%d: %s: Could not allocate IQ buffer memory \n",
			WLCWLUNIT(airiqh->wlc), __FUNCTION__));
		return FALSE;
	}

	return TRUE;
}

void wlc_lte_u_free_iqbuf(airiq_info_t *airiqh)
{
	if (airiqh->iqbuf != NULL) {
		MFREE(airiqh->wlc->osh, airiqh->iqbuf, airiqh->iqbuf_size);
	}
}

void wlc_lte_u_vasipiqcapture(airiq_info_t *airiqh)
{
	uint8 *buffer;
	lte_u_iqdata_header_t hdr;
	int32 datalen = 0;
	chanspec_t chanspec = 0;
	uint16 seqno = 0;
	uint32 timestamp;
	uint32 *iqbuf_ptr;
	uint16 *header_ptr;
	int32 eventlen = 0;
	lte_u_event_t *lte_u_event;

	airiqh->fft_count++;
	if (airiqh->capture_limit > 0 && airiqh->fft_count >=
			airiqh->capture_limit && airiqh->iq_capture_enable) {
		wlc_lte_u_phy_disable_iq_capture_shm(airiqh);
	}
	airiqh->lte_interrupt_received = TRUE;

	/* Read the IQ data Header and extract BW */
	iqbuf_ptr = wlc_lte_u_iqbuf_readbytes(airiqh, VASIP_IQ_HEADER_SIZE, SVMP_LTE_U_INFO_ADDR);

	if (iqbuf_ptr) {
		header_ptr = (uint16*)iqbuf_ptr;
		timestamp = ltoh32(*(uint32*)(header_ptr + 18));
		seqno = ltoh16(*(header_ptr + 20));
		chanspec = ltoh16(*(header_ptr + 21));

		/* transferring header + IQ data using 2 MAC events since each event has a cap
		 * of 1K bytes
		 */
		eventlen = VASIP_IQ_SIZE / 2 + sizeof(lte_u_iqdata_header_t);
		datalen = eventlen;
		/* Allocate MAC event */
		if ((lte_u_event = MALLOC(airiqh->wlc->osh, sizeof(lte_u_event_t) + eventlen)) == NULL) {
			WL_ERROR(("wl%d: %s: MALLOC failed, malloced %d bytes\n",
				WLCWLUNIT(airiqh->wlc), __FUNCTION__, MALLOCED(airiqh->wlc->osh)));
			return;
		}
		lte_u_event->lte_u_event_type = LTE_U_EVENT_IQ_CAPTURE;
		lte_u_event->data_len = sizeof(lte_u_event_t) + eventlen;
		/* Event header */
		hdr.size_bytes = datalen;
		hdr.corerev = airiqh->wlc->pub->corerev;
		hdr.unit = airiqh->wlc->pub->unit;
		hdr.chanspec = chanspec;
		hdr.seqno = seqno;
		hdr.timestamp = timestamp;
		hdr.data_bytes = datalen - sizeof(lte_u_iqdata_header_t);

		/* Read the IQ data */
		iqbuf_ptr = wlc_lte_u_iqbuf_readbytes(airiqh, VASIP_IQ_SIZE,
				SVMP_LTE_U_SCRATCH_DEC_ADDR);
		if (iqbuf_ptr) {
			buffer = lte_u_event->data;
			if (buffer) {
				/* Event1 (hdr + VASIP_IQ_SIZE/2) */
				hdr.message_type = MESSAGE_TYPE_VASIP_IQEVT1;
				memcpy(lte_u_event->data, &hdr, sizeof(lte_u_iqdata_header_t));
				memcpy(lte_u_event->data + sizeof(lte_u_iqdata_header_t),
					iqbuf_ptr, eventlen - sizeof(lte_u_iqdata_header_t));
				wlc_mac_event(airiqh->wlc, WLC_E_LTE_U_EVENT, NULL,
					WLC_E_STATUS_SUCCESS, 0, 0, (void*)(lte_u_event),
					lte_u_event->data_len);

				/* Event2 (hdr + remaining IQ data VASIP_IQ_SIZE/2) */
				hdr.message_type = MESSAGE_TYPE_VASIP_IQEVT2;
				memcpy(lte_u_event->data, &hdr, sizeof(lte_u_iqdata_header_t));
				memcpy(lte_u_event->data + sizeof(lte_u_iqdata_header_t),
					iqbuf_ptr + VASIP_IQ_SIZE / 8, eventlen);
				wlc_mac_event(airiqh->wlc, WLC_E_LTE_U_EVENT, NULL,
					WLC_E_STATUS_SUCCESS, 0, 0, (void*)(lte_u_event),
					lte_u_event->data_len);
			} else {
				WL_AIRIQ(("wl%d: %s: Could not get bytes\n",
					WLCWLUNIT(airiqh->wlc), __FUNCTION__));
			}
		}
		MFREE(airiqh->wlc->osh, lte_u_event, sizeof(lte_u_event_t) + eventlen);
	}
}

#endif /* VASIP_HW_SUPPORT */
