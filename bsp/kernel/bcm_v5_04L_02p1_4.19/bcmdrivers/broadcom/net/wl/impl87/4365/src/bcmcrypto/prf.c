/*
 * prf.c
 * Pseudo-random function used by WPA and TGi
 *
 * Original implementation of hmac_sha1(), PRF(), and test vectors data are
 * from "SSN for 802.11-0.21.doc"
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
 * $Id: prf.c 451682 2014-01-27 20:30:17Z $
 */

#include <typedefs.h>
#include <bcmcrypto/sha1.h>
#include <bcmcrypto/prf.h>

#ifdef BCMCCX
#include <bcmcrypto/ccx.h>
#endif

#ifdef BCMDRIVER
#include <osl.h>
#else
#include <string.h>
#endif	/* BCMDRIVER */

void
hmac_sha1(unsigned char *text, int text_len, unsigned char *key,
          int key_len, unsigned char *digest)
{
	SHA1Context icontext, ocontext;
	uint32 pki[(PRF_MAX_KEY_LEN / sizeof(uint32)) + 1];
	uint32 pko[(PRF_MAX_KEY_LEN / sizeof(uint32)) + 1];
	uint8 *k_ipad = (uint8 *)pki;
	uint8 *k_opad = (uint8 *)pko;
	int i;

	/* if key is longer than 64 bytes reset it to key = SHA1(key) */
	if (key_len > 64) {
		SHA1Context      tctx;
		SHA1Reset(&tctx);
		SHA1Input(&tctx, key, key_len);
		SHA1Result(&tctx, key);
		key_len = 20;
	}

	/*
	 * the HMAC_SHA1 transform looks like:
	 *
	 * SHA1(K XOR opad, SHA1(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */

	/* start out by storing key in pads */
	memcpy(k_ipad, key, key_len);
	memset(&k_ipad[key_len], 0, PRF_MAX_KEY_LEN + 1 - key_len);
	memcpy(k_opad, k_ipad, PRF_MAX_KEY_LEN + 1);

	/* XOR key with ipad and opad values */
	for (i = 0; i < 16; i++) {
		pki[i] ^= 0x36363636;
		pko[i] ^= 0x5c5c5c5c;
	}
	/* init contexts */
	SHA1Reset(&icontext);
	memcpy(&ocontext, &icontext, sizeof(icontext));

	/* perform inner SHA1 */
	SHA1Input(&icontext, k_ipad, 64);     /* start with inner pad */
	SHA1Input(&icontext, text, text_len); /* then text of datagram */
	SHA1Result(&icontext, digest);		/* finish up 1st pass */

	/* perform outer SHA1 */
	SHA1Input(&ocontext, k_opad, 64);     /* start with outer pad */
	SHA1Input(&ocontext, digest, 20);     /* then results of 1st hash */
	SHA1Result(&ocontext, digest);          /* finish up 2nd pass */
}

/* PRF
 * Length of output is in octets rather than bits
 * since length is always a multiple of 8
 * output array is organized so first N octets starting from 0
 * contains PRF output
 *
 * supported inputs are 16, 32, 48, 64
 * output array must be 80 octets in size to allow for sha1 overflow
 */
int
PRF(unsigned char *key, int key_len, unsigned char *prefix,
    int prefix_len, unsigned char *data, int data_len,
    unsigned char *output, int len)
{
	int i;
	unsigned char input[PRF_MAX_I_D_LEN]; /* concatenated input */
	int currentindex = 0;
	int total_len;
	int data_offset = 0;

	if ((prefix_len + data_len + 1) > PRF_MAX_I_D_LEN)
		return (-1);

	if (prefix_len != 0) {
		memcpy(input, prefix, prefix_len);
		input[prefix_len] = 0;		/* single octet 0 */
		data_offset = prefix_len + 1;
	}
	memcpy(&input[data_offset], data, data_len);
	total_len = data_offset + data_len;
	input[total_len] = 0;		/* single octet count, starts at 0 */
	total_len++;
	for (i = 0; i < (len + 19) / 20; i++) {
		hmac_sha1(input, total_len, key, key_len, &output[currentindex]);
		currentindex += 20;	/* next concatenation location */
		input[total_len-1]++;	/* increment octet count */
	}
	return (0);
}

/* faster PRF, inline hmac_sha1 functionality and eliminate redundant
 * initializations
 */
int
fPRF(unsigned char *key, int key_len, const unsigned char *prefix,
     int prefix_len, unsigned char *data, int data_len,
     unsigned char *output, int len)
{
	uint32 pki32[(PRF_MAX_KEY_LEN / sizeof(uint32)) + 1];
	uint32 pko32[(PRF_MAX_KEY_LEN / sizeof(uint32)) + 1];
	uint8 *pki8 = (uint8 *)pki32;
	uint8 *pko8 = (uint8 *)pko32;

	SHA1Context reficontext, refocontext, icontext, ocontext;
	int i, total_len, currentindex = 0;
	uint8 input[PRF_MAX_I_D_LEN];
	int data_offset = 0;

	if ((prefix_len + data_len + 1) > PRF_MAX_I_D_LEN)
		return (-1);

	if (prefix_len != 0) {
		memcpy(input, prefix, prefix_len);
		input[prefix_len] = 0;
		data_offset = prefix_len + 1;
	}
	memcpy(&input[data_offset], data, data_len);
	total_len = data_offset + data_len;
	input[total_len] = 0;
	total_len++;

	/* if key is longer than 64 bytes reset it to key = SHA1(key) */
	if (key_len > PRF_MAX_KEY_LEN) {
		SHA1Context tctx;
		SHA1Reset(&tctx);
		SHA1Input(&tctx, key, key_len);
		SHA1Result(&tctx, key);
		key_len = SHA1HashSize;
	}

	/* store key in pads */
	memcpy(pki8, key, key_len);
	memset(&pki8[key_len], 0, PRF_MAX_KEY_LEN + 1 - key_len);
	memcpy(pko8, pki8, PRF_MAX_KEY_LEN + 1);

	/* XOR key with ipad and opad values */
	for (i = 0; i < PRF_MAX_KEY_LEN / 4; i++) {
		pki32[i] ^= 0x36363636;
		pko32[i] ^= 0x5c5c5c5c;
	}

	/* init reference contexts */
	SHA1Reset(&reficontext);
	memcpy(&refocontext, &reficontext, sizeof(reficontext));
	SHA1Input(&reficontext, pki8, PRF_MAX_KEY_LEN);
	SHA1Input(&refocontext, pko8, PRF_MAX_KEY_LEN);

	for (i = 0; i < (len + SHA1HashSize - 1) / SHA1HashSize; i++) {
		/* copy reference context to working copies */
		memcpy(&icontext, &reficontext, sizeof(reficontext));
		memcpy(&ocontext, &refocontext, sizeof(refocontext));

		/* perform inner SHA1 */
		SHA1Input(&icontext, input, total_len);
		SHA1Result(&icontext, &output[currentindex]);

		/* perform outer SHA1 */
		SHA1Input(&ocontext, &output[currentindex], SHA1HashSize);
		SHA1Result(&ocontext, &output[currentindex]);

		currentindex += SHA1HashSize;
		input[total_len-1]++;
	}

	return (0);
}

#ifdef BCMPRF_TEST
#include <stdio.h>
#include "prf_vectors.h"

#ifdef PRF_TIMING
#include <sys/time.h>
#include <stdlib.h>
#endif /* PRF_TIMING */

void
dprintf(char *label, uint8 *data, int dlen, int status)
{
	int j;
	printf("%s:\n\t", label);
	for (j = 0; j < dlen; j++) {
		printf("%02x ", data[j]);
		if ((j < dlen - 1) && !((j + 1) % 16))
			printf("\n\t");
	}
	printf("\n\t%s\n\n", !status?"Pass":"Fail");
}

#ifdef PRF_TIMING
#define NITER	10000

int
main(int argc, char* argv[])
{
	struct timeval tstart, tend;
	unsigned char output[64 + 20];
	int32 usec;
	int j;

	memset(output, 0, 64);
	if (gettimeofday(&tstart, NULL)) exit(1);
	for (j = 0; j < NITER; j++) {
		fPRF(prf_vec[0].key, prf_vec[0].key_len, (const unsigned char *)"prefix",
			6, prf_vec[0].data, prf_vec[0].data_len, output, 64);
	}
	if (gettimeofday(&tend, NULL)) exit(1);
	dprintf("fPRF", output, 64, 0);
	usec = tend.tv_usec - tstart.tv_usec;
	usec += 1000000 * (tend.tv_sec - tstart.tv_sec);
	printf("usec %d\n", usec);

	memset(output, 0, 64);
	if (gettimeofday(&tstart, NULL)) exit(1);
	for (j = 0; j < NITER; j++) {
		PRF(prf_vec[0].key, prf_vec[0].key_len, (const unsigned char *)"prefix",
			6, prf_vec[0].data, prf_vec[0].data_len, output, 64);
	}
	if (gettimeofday(&tend, NULL)) exit(1);
	dprintf("PRF", output, 64, 0);
	usec = tend.tv_usec - tstart.tv_usec;
	usec += 1000000 * (tend.tv_sec - tstart.tv_sec);
	printf("usec %d\n", usec);

	memset(output, 0, 64);
	if (gettimeofday(&tstart, NULL)) exit(1);
	for (j = 0; j < NITER; j++) {
		fPRF(prf_vec[0].key, prf_vec[0].key_len, (const unsigned char *)"prefix",
			6, prf_vec[0].data, prf_vec[0].data_len, output, 64);
	}
	if (gettimeofday(&tend, NULL)) exit(1);
	dprintf("fPRF", output, 64, 0);
	usec = tend.tv_usec - tstart.tv_usec;
	usec += 1000000 * (tend.tv_sec - tstart.tv_sec);
	printf("usec %d\n", usec);

	memset(output, 0, 64);
	if (gettimeofday(&tstart, NULL)) exit(1);
	for (j = 0; j < NITER; j++) {
		PRF(prf_vec[0].key, prf_vec[0].key_len, (unsigned char *)"prefix",
			6, prf_vec[0].data, prf_vec[0].data_len, output, 64);
	}
	if (gettimeofday(&tend, NULL)) exit(1);
	dprintf("PRF", output, 64, 0);
	usec = tend.tv_usec - tstart.tv_usec;
	usec += 1000000 * (tend.tv_sec - tstart.tv_sec);
	printf("usec %d\n", usec);

	return (0);
}

#else

int
main(int argc, char* argv[])
{
	unsigned char digest[20];
	unsigned char output[64 + 20];
	int k, c, fail = 0;

	for (k = 0; k < NUM_VECTORS; k++) {
		printf("Test Vector %d:\n", k);
		hmac_sha1(prf_vec[k].data, prf_vec[k].data_len, prf_vec[k].key,
			prf_vec[k].key_len, digest);
		c = memcmp(digest, prf_vec[k].digest1, 20);
		dprintf("HMAC_SHA1", digest, 20, c);
		if (c) fail++;

		memset(output, 0, 64);
		if (PRF(prf_vec[k].key, prf_vec[k].key_len,
			prf_vec[k].prefix, prf_vec[k].prefix_len,
			prf_vec[k].data, prf_vec[k].data_len, output, 16))
				fail++;
		c = memcmp(output, prf_vec[k].prf, 16);
		dprintf("PRF", output, 16, c);
		if (c) fail++;

		memset(output, 0, 64);
		if (fPRF(prf_vec[k].key, prf_vec[k].key_len,
			prf_vec[k].prefix, prf_vec[k].prefix_len,
			prf_vec[k].data, prf_vec[k].data_len, output, 16))
				fail++;
		c = memcmp(output, prf_vec[k].prf, 16);
		dprintf("fPRF", output, 16, c);
		if (c) fail++;

		memset(output, 0, 64);
		if (PRF(prf_vec[k].key, prf_vec[k].key_len,
			prf_vec[k].prefix, prf_vec[k].prefix_len,
			prf_vec[k].data, prf_vec[k].data_len, output, 64))
				fail++;
		c = memcmp(output, prf_vec[k].prf, 64);
		dprintf("PRF", output, 64, c);
		if (c) fail++;

		memset(output, 0, 64);
		if (fPRF(prf_vec[k].key, prf_vec[k].key_len,
		         prf_vec[k].prefix, prf_vec[k].prefix_len,
		         prf_vec[k].data, prf_vec[k].data_len, output, 64))
			fail++;
		c = memcmp(output, prf_vec[k].prf, 64);
		dprintf("fPRF", output, 64, c);
		if (c) fail++;
	}

#ifdef BCMCCX
	for (k = 0; k < NUM_CCX_VECTORS; k++) {
		printf("CCX Test Vector %d:\n", k);
		memset(output, 0, 64);
		if (PRF(ccx_prf_vec[k].key, ccx_prf_vec[k].key_len,
		        ccx_prf_vec[k].prefix, ccx_prf_vec[k].prefix_len,
		        ccx_prf_vec[k].data, ccx_prf_vec[k].data_len,
		        output, ccx_prf_vec[k].prf_len))
			fail++;
		c = memcmp(output, ccx_prf_vec[k].prf, ccx_prf_vec[k].prf_len);
		dprintf("PRF", output, ccx_prf_vec[k].prf_len, c);
		if (c) fail++;

		memset(output, 0, 64);
		if (fPRF(ccx_prf_vec[k].key, ccx_prf_vec[k].key_len,
		         ccx_prf_vec[k].prefix, ccx_prf_vec[k].prefix_len,
		         ccx_prf_vec[k].data, ccx_prf_vec[k].data_len, output,
		         ccx_prf_vec[k].prf_len))
			fail++;
		c = memcmp(output, ccx_prf_vec[k].prf, ccx_prf_vec[k].prf_len);
		dprintf("fPRF", output, ccx_prf_vec[k].prf_len, c);
		if (c) fail++;
	}
#endif /* BCMCCX */

	fprintf(stderr, "%s: %s\n", *argv, fail?"FAILED":"PASSED");
	return (fail);
}
#endif /* PRF_TIMING */
#endif /* BCMPRF_TEST */
