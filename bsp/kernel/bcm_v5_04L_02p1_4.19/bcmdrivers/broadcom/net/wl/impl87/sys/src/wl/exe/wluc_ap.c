/*
 * wl ap command module
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
 * $Id: wluc_ap.c 810879 2022-04-18 20:46:20Z $
 */

#include <wlioctl.h>

#if	defined(DONGLEBUILD)
#include <typedefs.h>
#include <osl.h>
#endif

#include <bcmutils.h>
#include <bcmendian.h>
#include "wlu_common.h"
#include "wlu.h"
#include <limits.h>

static cmd_func_t wl_maclist_1;
static cmd_func_t wl_management_info;
static cmd_func_t wl_bsscfg_enable;
static cmd_func_t wl_radar;
static cmd_func_t wl_map;
static cmd_func_t wl_map_profile;
static cmd_func_t wl_map_8021q_settings;
static cmd_func_t wl_force_bcn_rspec;
static cmd_func_t wl_override_clm_tpe;
static cmd_func_t wl_reg_info;
static cmd_func_t wl_rand_ch;

static cmd_t wl_ap_cmds[] = {
	{ "ap", wl_int, WLC_GET_AP, WLC_SET_AP,
	"Set AP mode: 0 (STA) or 1 (AP)" },
	{ "tkip_countermeasures", wl_int, -1, WLC_TKIP_COUNTERMEASURES,
	"Enable or disable TKIP countermeasures (TKIP-enabled AP only)\n"
	"\t0 - disable\n"
	"\t1 - enable" },
	{ "shortslot_restrict", wl_int, WLC_GET_SHORTSLOT_RESTRICT, WLC_SET_SHORTSLOT_RESTRICT,
	"Get/Set AP Restriction on associations for 11g Short Slot Timing capable STAs.\n"
	"\t0 - Do not restrict association based on ShortSlot capability\n"
	"\t1 - Restrict association to STAs with ShortSlot capability" },
	{ "ignore_bcns", wl_int, WLC_GET_IGNORE_BCNS, WLC_SET_IGNORE_BCNS,
	"AP only (G mode): Check for beacons without NONERP element"
	"(0=Examine beacons, 1=Ignore beacons)" },
	{ "scb_timeout", wl_int, WLC_GET_SCB_TIMEOUT, WLC_SET_SCB_TIMEOUT,
	"AP only: inactivity timeout value for authenticated stas" },
	{ "assoclist", wl_maclist, WLC_GET_ASSOCLIST, -1,
	"AP only: Get the list of associated MAC addresses."},
	{ "radar", wl_radar, WLC_GET_RADAR, WLC_SET_RADAR,
	"Enable/Disable radar. One-shot Radar simulation with optional sub-band"},
	{ "authe_sta_list", wl_maclist_1, WLC_GET_VAR, -1,
	"Get authenticated sta mac address list"},
	{ "autho_sta_list", wl_maclist_1, WLC_GET_VAR, -1,
	"Get authorized sta mac address list"},
	{ "beacon_info", wl_management_info, WLC_GET_VAR, -1,
	"Returns the 802.11 management frame beacon information\n"
	"Usage: wl beacon_info [-f file] [-r]\n"
	"\t-f Write beacon data to file\n"
	"\t-r Raw hex dump of beacon data" },
	{ "probe_resp_info", wl_management_info, WLC_GET_VAR, -1,
	"Returns the 802.11 management frame probe response information\n"
	"Usage: wl probe_resp_info [-f file] [-r]\n"
	"\t-f Write probe response data to file\n"
	"\t-r Raw hex dump of probe response data" },
	{ "bss", wl_bsscfg_enable, WLC_GET_VAR, WLC_SET_VAR,
	"set/get BSS enabled status: up/down"},
	{ "closednet", wl_bsscfg_int, WLC_GET_VAR, WLC_SET_VAR,
	"set/get BSS closed network attribute"},
	{ "ap_isolate", wl_bsscfg_int, WLC_GET_VAR, WLC_SET_VAR,
	"set/get AP isolation"},
	{ "mode_reqd", wl_bsscfg_int, WLC_GET_VAR, WLC_SET_VAR,
	"Set/Get operational capabilities required for STA to associate to the BSS "
	"supported by the interface.\n"
	"\tUsage: wl [-i ifname] mode_reqd [value]\n"
	"\t       wl mode_reqd [-C bss_idx ] [value]\n"
	"\t\t     <ifname> is the name of the interface corresponding to the BSS.\n"
	"\t\t\t   If the <ifname> is not given, the primary BSS is assumed.\n"
	"\t\t     <bss_idx> is the the BSS configuration index.\n"
	"\t\t\t   If the <bss_idx> is not given, configuraion #0 is assumed\n"
	"\t\t     <value> is the numeric values in the range [0..3]\n"
	"\t\t     0 - no requirements on joining devices.\n"
	"\t\t     1 - devices must advertise ERP (11g) capabilities to be allowed to associate\n"
	"\t\t\t   to a 2.4 GHz BSS.\n"
	"\t\t     2 - devices must advertise HT (11n) capabilities to be allowed to associate\n"
	"\t\t\t   to a BSS.\n"
	"\t\t     3 - devices must advertise VHT (11ac) capabilities to be allowed to associate\n"
	"\t\t\t   to a BSS.\n"
	"\t\t     4 - devices must advertise HE (11ax) capabilities to be allowed to associate\n"
	"\t\t\t   to a BSS.\n"
	"\tThe command returns an error if the BSS interface is up.\n"
	"\tThis configuration can only be changed while the BSS interface is down.\n"
	"\tNote that support for HT implies support for ERP,\n"
	"\tsupport for VHT implies support for HT,\n"
	"\tand support for HE implies support for VHT."},
	{ "map", wl_map, WLC_GET_VAR, WLC_SET_VAR,
	"Set / Get Multi-AP flag\n"},
	{ "map_profile", wl_map_profile, WLC_GET_VAR, WLC_SET_VAR,
	"Set / Get Multi-AP Profile\n"},
	{ "map_8021q_settings", wl_map_8021q_settings, WLC_GET_VAR, WLC_SET_VAR,
	"Set / Get Multi-AP Default 802.1Q Settings\n"},
	{ "ucast_disassoc_on_bss_down", wl_bsscfg_int, WLC_GET_VAR, WLC_SET_VAR,
	"set/get sending unicast disassoc to stas when bss is going down"},
	{ "force_bcn_rspec", wl_force_bcn_rspec, WLC_GET_VAR, WLC_SET_VAR, RATE_6G_USAGE},
	{ "override_clm_tpe", wl_override_clm_tpe, WLC_GET_VAR, WLC_SET_VAR,
	"Set/get override value of CLM tpe power values\n"
	"\tUsage: wl [-i ifname] override_clm_tpe [mode] ([power in dBm] | [disable])\n"
	"\t(Valid range power in dBm: -63.5 to 63dBm)\n"
	"\t(mode - 1 EIRP power in dBm: upto 4 value 20 40 80 160 Mhz)\n"
	"\t(mode - 2 EIRP psd  power in dBm/MHz: upto 8 value for each 20 in 160 MHz)\n"
	"\tEg: wl override_clm_tpe 1 22 24 26 28 (EIRP 20Mhz:22 40Mhz:24 80Mhz:26 160Mhz:28 \n"
	"\tEg: wl override_clm_tpe 1 22 24  (EIRP 20Mhz:22 40Mhz:24 \n"
	"\tEg: wl override_clm_tpe 2 1 2 3 4 5 6 7 8 (EIRP PSD for each 20 Mhz in 160Mhz \n"
	"\tEg: wl override_clm_tpe 1 to display the EIRP override values\n"
	"\tEg: wl override_clm_tpe 2 to display EIRP PSD override values\n"
	"\tEg: wl override_clm_tpe 1/2  disable\n" },
	{"pref_transmit_bss", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Set/Get preferred transmit bss (Index) - \n"
	"\tAP bss index need to be set as preferred transmit bss \n"
	"\tOnce the index is set, Beaconing starts when the indexed bss is up"
	"\tUntill the pref_transmit bss indexed bss is up, there won't be any beacon\n"
	"\tOnly works in radio down state \n"
	"\tEg: To choose wl0.3 as pref transmit bss - wl pref_transmit_bss 3 \n"
	"\t(interface - index) wl0 - 0, wl0.1 - 1 .... wl0.n - n \n"
	"\tdefault index set by driver is 0xFF,\n"
	"\tWhen index is 0xFF(iovar not set) first up ap bsscfg is selected as transmit bss \n"},
	{ "reg_info", wl_reg_info, WLC_GET_VAR, -1,
	"Returns regulatory information details\n" },
	{ "edcrs_hi_event_mode", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Get or set EDCRS_HI event mode.\nUsage:\n(To get call without arguments)\n"
	"\twl edcrs_hi_event_mode [mode]\n"
	"\tmode:\n"
	"\t\t-1: auto (chooses based on band and regulatory requirements or in-driver reaction)\n"
	"\t\t0: Disable\n"
	"\t\t1: Enable event to host\n"
	"\t\t2: Enable in-driver channel change on EDCRS_HI\n"
	"\t\t3: Enable event to host and fallback to in-driver channel change on EDCRS_HI\n"
	"\t\t4: Simulate one shot EDCRS_HI event (if already enabled)\n" },
	{ "edcrs_hi_event_status", wl_varint, WLC_GET_VAR, -1,
	"Get EDCRS_HI event status. See help for edcrs_hi_event_mode for details\n" },
	{ "edcrs_hi_simulate", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"\tSimulate one shot EDCRS_HI event (if already enabled)\n"
	"\t\tExperimental! Optionally pass duration in seconds, range:1-255, e.g.,\n"
	"\twl edcrs_hi_simulate 7\n"
	"\t\twill simulate high EDCRS for 7 seconds" },
	{ "edcrs_txbcn_inact_thresh", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Get/set TX beacon inactivity threshold in seconds.\n"
	"\tChannel change and/or related events will be generated as required by "
	"edcrs_hi_event_mode/status if \n"
	"\t\t- TX Beacon inactivity is observed for edcrs_txbcn_inact_thresh consecutive seconds "
	"and \n"
	"\t\t- the cumulative EDCRS duration in each second crosses edcrs_dur_thresh_us microsecs\n"
	},
	{ "edcrs_dur_thresh_us", wl_varint, WLC_GET_VAR, WLC_SET_VAR,
	"Get/set threshold in microsecs for acceptable EDCRS duration in 1s.\n"
	"\tChannel change and/or related events will be generated as required by "
	"edcrs_hi_event_mode/status if \n"
	"\t\t- the cumulative EDCRS duration in each second crosses edcrs_dur_thresh_us microsecs"
	"and \n"
	"\t\t- TX Beacon inactivity is observed for edcrs_txbcn_inact_thresh consecutive seconds\n"
	"\tUsage:wl edcrs_dur_thresh_us [val]\n"
	"\t\t'val', when passed, must be in range: 0 to 1000000.\n" },
	{ "rand_ch", wl_rand_ch, WLC_GET_VAR, -1,
	"Get valid random chanspec\n"
	"\toptional argument 1 to exclude current chanspec\n" },
	{ "beacon_len", wl_varint, WLC_GET_VAR, -1,
	"Returns estimated MBSSID beacon length when driver is down\n"
	"Usage: wl beacon_len\n" },
	{ NULL, NULL, 0, 0, NULL }
};

static char *buf;

/* module initialization */
void
wluc_ap_module_init(void)
{
	/* get the global buf */
	buf = wl_get_buf();

	/* register ap commands */
	wl_module_cmds_register(wl_ap_cmds);
}

/*
 * Get Radar Enable/Disable status
 * Set one-shot radar simulation with optional subband
 */
int
wl_radar(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int val;
	char *endptr = NULL;
	UNUSED_PARAMETER(cmd);

	if (!*++argv) {
		if ((ret = wlu_get(wl, WLC_GET_RADAR, &val, sizeof(int))) < 0) {
			return ret;
		}

		val = dtoh32(val);
		wl_printint(val);
	} else {
		struct {
			int val;
			uint sub;
		} radar;
		radar.val = strtol(*argv, &endptr, 0);
		if (*endptr != '\0') {
			/* not all the value string was parsed by strtol */
			return BCME_USAGE_ERROR;
		}

		radar.val = htod32(radar.val);
		radar.sub = 0;
		if (!*++argv) {
			ret = wlu_set(wl, WLC_SET_RADAR, &radar.val, sizeof(radar.val));
		} else {
			radar.sub = strtol(*argv, &endptr, 0);
			if (*endptr != '\0') {
				/* not all the value string was parsed by strtol */
				return BCME_USAGE_ERROR;
			}
			radar.sub = htod32(radar.sub);
			ret = wlu_set(wl, WLC_SET_RADAR, &radar, sizeof(radar));
		}
	}

	return ret;
}

static int
wl_bsscfg_enable(void *wl, cmd_t *cmd, char **argv)
{
	char *endptr;
	const char *val_name = "bss";
	int bsscfg_idx = 0;
	int val;
	int consumed;
	int ret;

	UNUSED_PARAMETER(cmd);

	/* skip the command name */
	argv++;

	/* parse a bsscfg_idx option if present */
	if ((ret = wl_cfg_option(argv, val_name, &bsscfg_idx, &consumed)) != 0)
		return ret;

	argv += consumed;
	if (consumed == 0) { /* Use the -i parameter if that was present */
		bsscfg_idx = -1;
	}

	if (!*argv) {
		bsscfg_idx = htod32(bsscfg_idx);
		ret = wlu_iovar_getbuf(wl, val_name, &bsscfg_idx, sizeof(bsscfg_idx),
		                      buf, WLC_IOCTL_MAXLEN);
		if (ret < 0)
			return ret;
		val = *(int*)buf;
		val = dtoh32(val);
		if (val)
			printf("up\n");
		else
			printf("down\n");
		return 0;
	} else {
		struct {
			int cfg;
			int val;
		} bss_setbuf;
		if (!stricmp(*argv, "move"))
			val = WLC_AP_IOV_OP_MOVE;
		else if (!stricmp(*argv, "ap"))
			val = WLC_AP_IOV_OP_MANUAL_AP_BSSCFG_CREATE;
		else if (!stricmp(*argv, "sta"))
			val = WLC_AP_IOV_OP_MANUAL_STA_BSSCFG_CREATE;
		else if (!stricmp(*argv, "up"))
			val = WLC_AP_IOV_OP_ENABLE;
		else if (!stricmp(*argv, "down"))
			val = WLC_AP_IOV_OP_DISABLE;
		else if (!stricmp(*argv, "del"))
			val = WLC_AP_IOV_OP_DELETE;
		else {
			val = strtol(*argv, &endptr, 0);
			if (*endptr != '\0') {
				/* not all the value string was parsed by strtol */
				return BCME_USAGE_ERROR;
			}
		}
		bss_setbuf.cfg = htod32(bsscfg_idx);
		bss_setbuf.val = htod32(val);

		return wlu_iovar_set(wl, val_name, &bss_setbuf, sizeof(bss_setbuf));
	}
}

static void dump_management_fields(uint8 *data, int len)
{
	int i, tag_len;
	uint8 tag;
	char temp[64];
	uint8 *p;

	while (len > 0) {
		/* Get the tag */
		tag = *data;
		data++; len--;

		/* Get the tag length */
		tag_len = (int) *data;
		data++; len--;

		printf("Tag:%d Len:%d - ", tag, tag_len);

		switch (tag) {
		case DOT11_MNG_SSID_ID:
			for (i = 0; i < tag_len; i++) {
				temp[i] = data[i];
			}
			if (i < 64) {
				temp[i] = '\0';
			}
			printf("SSID: '%s'\n", temp);
			break;
		case DOT11_MNG_FH_PARMS_ID:
			printf("FH Parameter Set\n");
			break;
		case DOT11_MNG_DS_PARMS_ID:
			printf("DS Parameter Set\n");
			break;
		case DOT11_MNG_CF_PARMS_ID:
			printf("CF Parameter Set\n");
			break;
		case DOT11_MNG_RATES_ID:
			printf("Supported Rates\n");
			break;
		case DOT11_MNG_TIM_ID:
			printf("Traffic Indication Map (TIM)\n");
			break;
		case DOT11_MNG_IBSS_PARMS_ID:
			printf("IBSS Parameter Set\n");
			break;
		case DOT11_MNG_COUNTRY_ID:
			p = data;
			printf("Country '%c%c%c'\n",
			       data[0], data[1], data[2]);
			p += DOT11_CNTRY_STRING_LEN;
			/* Country IE, Subband and Operating Triplets have the same size  */
			STATIC_ASSERT(DOT11_CNTRY_SUBBAND_LEN == DOT11_CNTRY_OPERATING_LEN);
			while (((data+tag_len) - p) >= DOT11_CNTRY_SUBBAND_LEN) {
				if (p[0] < DOT11_CNTRY_OPERATING_EXT_ID) {
					printf("Start Channel: %d, Channels: %d, "
					       "Max TX Power: %d dBm\n",
					       p[0], p[1], p[2]);
					p += DOT11_CNTRY_SUBBAND_LEN;
				} else {
					printf("Operating Extension ID: %d, Operating Class: %d, "
					       "Coverage Class: %d\n",
					       p[0], p[1], p[2]);
					p += DOT11_CNTRY_OPERATING_LEN;
				}
			}
			break;
		case DOT11_MNG_HOPPING_PARMS_ID:
			printf("Hopping Pattern Parameters\n");
			break;
		case DOT11_MNG_HOPPING_TABLE_ID:
			printf("Hopping Pattern Table\n");
			break;
		case DOT11_MNG_REQUEST_ID:
			printf("Request\n");
			break;
		case DOT11_MNG_QBSS_LOAD_ID:
			printf("QBSS Load\n");
			break;
		case DOT11_MNG_EDCA_PARAM_ID:
			printf("EDCA Parameter\n");
			break;
		case DOT11_MNG_CHALLENGE_ID:
			printf("Challenge text\n");
			break;
		case DOT11_MNG_PWR_CONSTRAINT_ID:
			printf("Power Constraint\n");
			break;
		case DOT11_MNG_PWR_CAP_ID:
			printf("Power Capability\n");
			break;
		case DOT11_MNG_TPC_REQUEST_ID:
			printf("Transmit Power Control (TPC) Request\n");
			break;
		case DOT11_MNG_TPC_REPORT_ID:
			printf("Transmit Power Control (TPC) Report\n");
			break;
		case DOT11_MNG_SUPP_CHANNELS_ID:
			printf("Supported Channels\n");
			break;
		case DOT11_MNG_CHANNEL_SWITCH_ID:
			printf("Channel Switch Announcement\n");
			break;
		case DOT11_MNG_MEASURE_REQUEST_ID:
			printf("Measurement Request\n");
			break;
		case DOT11_MNG_MEASURE_REPORT_ID:
			printf("Measurement Report\n");
			break;
		case DOT11_MNG_QUIET_ID:
			printf("Quiet\n");
			break;
		case DOT11_MNG_IBSS_DFS_ID:
			printf("IBSS DFS\n");
			break;
		case DOT11_MNG_ERP_ID:
			printf("ERP Information\n");
			break;
		case DOT11_MNG_TS_DELAY_ID:
			printf("TS Delay\n");
			break;
		case DOT11_MNG_HT_CAP:
			printf("HT Capabilities\n");
			break;
		case DOT11_MNG_QOS_CAP_ID:
			printf("QoS Capability\n");
			break;
		case DOT11_MNG_NONERP_ID:
			printf("NON-ERP\n");
			break;
		case DOT11_MNG_RSN_ID:
			printf("RSN\n");
			break;
		case DOT11_MNG_EXT_RATES_ID:
			printf("Extended Supported Rates\n");
			break;
		case DOT11_MNG_AP_CHREP_ID:
			printf("AP Channel Report\n");
			break;
		case DOT11_MNG_NEIGHBOR_REP_ID:
			printf("Neighbor Report\n");
			break;
		case DOT11_MNG_MDIE_ID:
			printf("Mobility Domain\n");
			break;
		case DOT11_MNG_FTIE_ID:
			printf("Fast BSS Transition\n");
			break;
		case DOT11_MNG_FT_TI_ID:
			printf("802.11R Timeout Interval\n");
			break;
		case DOT11_MNG_REGCLASS_ID:
			printf("Regulatory Class\n");
			break;
		case DOT11_MNG_EXT_CSA_ID:
			printf("Extended CSA\n");
			break;
		case DOT11_MNG_HT_ADD:
			printf("HT Information\n");
			break;
		case DOT11_MNG_EXT_CHANNEL_OFFSET:
			printf("Ext Channel\n");
			break;
#ifdef BCMWAPI_WAI
		case DOT11_MNG_WAPI_ID:
			printf("WAPI\n");
			break;
#endif
		case DOT11_MNG_RRM_CAP_ID:
			printf("Radio Measurement\n");
			break;
		case DOT11_MNG_HT_BSS_COEXINFO_ID:
			printf("OBSS Coexistence INFO\n");
			break;
		case DOT11_MNG_HT_BSS_CHANNEL_REPORT_ID:
			printf("OBSS Intolerant Channel List\n");
			break;
		case DOT11_MNG_HT_OBSS_ID:
			printf("OBSS HT Info\n");
			break;
#ifdef DOT11_MNG_CHANNEL_USAGE
		case DOT11_MNG_CHANNEL_USAGE:
			printf("Channel Usage\n");
			break;
#endif
		case DOT11_MNG_LINK_IDENTIFIER_ID:
			printf("TDLS Link Identifier\n");
			break;
		case DOT11_MNG_WAKEUP_SCHEDULE_ID:
			printf("TDLS Wakeup Schedule\n");
			break;
		case DOT11_MNG_CHANNEL_SWITCH_TIMING_ID:
			printf("TDLS Channel Switch Timing\n");
			break;
		case DOT11_MNG_PTI_CONTROL_ID:
			printf("TDLS PTI Control\n");
			break;
		case DOT11_MNG_PU_BUFFER_STATUS_ID:
			printf("TDLS PU Buffer Status\n");
			break;
		case DOT11_MNG_EXT_CAP_ID:
			printf("Management Ext Capability\n");
			break;
		case DOT11_MNG_PROPR_ID:
			printf("Proprietary\n");
			break;
		case DOT11_MNG_VHT_CAP_ID:
			printf("VHT Capabilities\n");
			break;
		case DOT11_MNG_VHT_OPERATION_ID:
			printf("VHT Operation\n");
			break;
		case DOT11_MNG_ID_EXT_ID:
			if (*data == EXT_MNG_HE_CAP_ID) {
				printf("ID Extension - HE Capabilities\n");
			}
			else if (*data == EXT_MNG_HE_OP_ID) {
				printf("ID Extension - HE Operation\n");
			}
			else {
				printf("ID Extension - %d\n", *data);
			}
			break;
		default:
			if (tag_len <= len) {
				printf("Unsupported tag\n");
			} else {
				/* Just dump the remaining data */
				printf("Unsupported tag error/malformed\n");
				tag_len = len;
			}
			break;
		} /* switch */

		wl_hexdump(data, tag_len);

		data += tag_len;
		len -= tag_len;
	} /* while */
}

static void dump_management_info(uint8 *data, int len)
{
	struct dot11_management_header hdr;
	struct dot11_bcn_prb parms;

	if (len <= (int) (sizeof(hdr)+sizeof(parms))) {
		/* Management packet invalid */
		return;
	}

	memcpy(&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	len -= sizeof(hdr);

	memcpy(&parms, data, sizeof(parms));
	data += sizeof(parms);
	len -= sizeof(parms);

	/* 802.11 MAC header */
	printf("Frame Ctl: 0x%04x\n", ltoh16(hdr.fc));
	printf("Duration : 0x%04x\n", ltoh16(hdr.durid));
	printf("Dest addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       hdr.da.octet[0],
	       hdr.da.octet[1],
	       hdr.da.octet[2],
	       hdr.da.octet[3],
	       hdr.da.octet[4],
	       hdr.da.octet[5]);
	printf("Src addr : %02x:%02x:%02x:%02x:%02x:%02x\n",
	       hdr.sa.octet[0],
	       hdr.sa.octet[1],
	       hdr.sa.octet[2],
	       hdr.sa.octet[3],
	       hdr.sa.octet[4],
	       hdr.sa.octet[5]);
	printf("BSSID    : %02x:%02x:%02x:%02x:%02x:%02x\n",
	       hdr.bssid.octet[0],
	       hdr.bssid.octet[1],
	       hdr.bssid.octet[2],
	       hdr.bssid.octet[3],
	       hdr.bssid.octet[4],
	       hdr.bssid.octet[5]);
	printf("Seq ctl  : 0x%04x\n", hdr.seq);

	/* 802.11 management frame */
	printf("Timestamp: 0x%08x%08x\n",
	       ltoh32(parms.timestamp[0]), ltoh32(parms.timestamp[1]));
	printf("Beacon Interval: 0x%04x\n", ltoh16(parms.beacon_interval));
	printf("Capabilities: 0x%04x\n", ltoh32(parms.capability));

	dump_management_fields(data, len);
}

static int
wl_management_info(void *wl, cmd_t *cmd, char**argv)
{
	int ret = 0;
	int len;
	uint8 *data;
	FILE *fp = NULL;
	char *fname = NULL;
	int raw = 0;

	/* Skip the command name */
	argv++;

	while (*argv) {
		char *s = *argv;

		if (!strcmp(s, "-f") && argv[1] != NULL) {
			/* Write packet to a file */
			fname = argv[1];
			argv += 2;
		} else if (!strcmp(s, "-r")) {
			/* Do a hex dump to console */
			raw = 1;
			argv++;
		} else
			return BCME_USAGE_ERROR;
	}

	/* Get the beacon information */
	strcpy(buf, cmd->name);
	if ((ret = wlu_get(wl, cmd->get, buf, WLC_IOCTL_MAXLEN)) < 0)
		return ret;

	/*
	 * Dump out the beacon data. The first word (4 bytes) is the
	 * length of the management packet followed by the data itself.
	 */
	len = dtoh32(*(int *)buf);

	if (len <= 0 || len > (int)(WLC_IOCTL_MAXLEN - sizeof(int))) {
		/* Nothing to do */
		return ret;
	}

	data = (uint8 *) (buf + sizeof(int));
	printf("Data: %p Len: %d bytes\n", data, len);

	if (fname != NULL) {
		/* Write the packet to a file */
		if ((fp = fopen(fname, "wb")) == NULL) {
			fprintf(stderr, "Failed to open file %s\n",
			        fname);
			ret = BCME_BADARG;
		} else {
			ret = fwrite(data, 1, len, fp);

			if (ret != len) {
				fprintf(stderr,
				        "Error write %d bytes to file %s, rc %d!\n",
				        len, fname, ret);
				ret = -1;
			}
		}
	} else if (raw) {
		/* Hex dump */
		wl_hexdump(data, (uint)len);
	} else {
		/* Print management (w/some decode) */
		dump_management_info(data, len);
	}

	if (fp)
		fclose(fp);

	return ret;
}

static int
wl_force_bcn_rspec(void *wl, cmd_t *cmd, char**argv)
{
	int ret = 0;
	int value;
	bool value_is_ratespec = FALSE; /**< ratespec instead of [500Kbps] units */
	bool set;
	char *endptr = NULL;
	char **tmp = argv;
	int n_args = 1;

	argv++; /* Skip the command name */

	while (*argv != NULL) {
		char *s = *argv;

		if (s[0] == '-') {
			value_is_ratespec = TRUE; /* means: user specified non-legacy format */
		}
		argv++;
		n_args++;
	}

	argv = tmp;

	if (value_is_ratespec) {
		return wl_rate(wl, cmd, argv); /* offers user a rich syntax to express a ratespec */
	}

	argv++; /* Skip the command name */
	set = (n_args == 1 ? FALSE : TRUE);

	if (set) {
		value = strtol(*argv, &endptr, 0);
		if (*endptr != '\0') {
			/* not all the value string was parsed by strtol */
			return BCME_USAGE_ERROR;
		}
		value &= WL_RSPEC_RATE_MASK; /* for backward compatibility with e.g. UTF */
		return wlu_iovar_setint(wl, cmd->name, value);
	}

	if ((ret = wlu_iovar_getint(wl, cmd->name, &value)) != BCME_OK) {
		return ret;
	}

	value &= WL_RSPEC_RATE_MASK; /* for backward compatibility with e.g. UTF */

	if (value < 10)
		printf("%d\n", value);
	else
		printf("%d (0x%x)\n", value, value);

	return ret;
}

static void wl_print_half_dbm(int val)
{
	int dbm = val/2;

	if (val == INT_MAX) {
		printf("Disabled (0x%08x)\n", val);
	} else if ((dbm < -63 || dbm > 63) || (dbm == 63 && (val & 1))) {
		printf("Out of range [-63.5dBm, 63dBm] (0x%08X)\n", val);
	} else {
		printf("%s%d.%cdBm (0x%02X)\n", (val == -1 ? "-" : ""), dbm,
				((val & 1) ? '5':'0'), val);
	}
}

static int
wl_override_clm_tpe(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	wlc_ioctl_tx_pwr_t tx_pwr;
	wlc_ioctl_tx_pwr_t *get_tx_pwr;
	int val, dbm, i;
	char *endp;
	int pwr_len = 0;
	int mode;

	if (!*++argv) {
		return 0;
	}

	memset(&tx_pwr, 0, sizeof(tx_pwr));

	mode  = (int) strtoul(*argv, &endp, 10);
	/* GET */
	if (!*++argv) {
		if ((ret = wlu_iovar_getbuf(wl, cmd->name, &mode,
				sizeof(mode), buf, WLC_IOCTL_MAXLEN)) != BCME_OK) {
			return ret;
		}
		get_tx_pwr = (wlc_ioctl_tx_pwr_t *)buf;
		get_tx_pwr->len = dtoh32(get_tx_pwr->len);
		for (i = 0; i < get_tx_pwr->len; i++) {
			get_tx_pwr->pwr[i] = dtoh32(get_tx_pwr->pwr[i]);
		}
		if (mode == WL_OVERRIDE_EIRP) {
			if (get_tx_pwr->len >= 1) {
				wl_print_half_dbm(get_tx_pwr->pwr[0]);
			}
			if (get_tx_pwr->len >= 2) {
				wl_print_half_dbm(get_tx_pwr->pwr[1]);
			}
			if (get_tx_pwr->len >= 3) {
				wl_print_half_dbm(get_tx_pwr->pwr[2]);
			}
			if (get_tx_pwr->len == 4) {
				wl_print_half_dbm(get_tx_pwr->pwr[3]);
			}
		}
		if (mode == WL_OVERRIDE_EIRP_PSD) {
			for (i = 0; i < get_tx_pwr->len; i++) {
				wl_print_half_dbm(get_tx_pwr->pwr[i]);
			}
		}
		return ret;
	}

	/* SET */
	if (!strcmp(*argv, "disable")) {
		tx_pwr.mode = mode;
		if (mode == WL_OVERRIDE_EIRP) {
			tx_pwr.len = 4;
			for (i = 0; i < 4; i++) {
				tx_pwr.pwr[i] = INT_MAX;
			}
		}
		if (mode == WL_OVERRIDE_EIRP_PSD) {
			tx_pwr.len = 8;
			for (i = 0; i < 8; i++) {
				tx_pwr.pwr[i] = INT_MAX;
			}
		}
	} else {
		tx_pwr.mode = mode;
		while (*argv) {
			int len = strlen(*argv);
			dbm = (int) strtoul(*argv, &endp, 10);
			if (dbm < -63 || dbm > 63) {
				printf("Out of range [-63.5dBm, 63dBm]\n");
				return BCME_RANGE;
			}
			val = dbm << 1;
			if (endp < (*argv + len - 1) && endp[0] == '.' && endp[1] == '5') {
				if (dbm >= 63) {
					printf("Out of range [-63.5dBm, 63dBm]\n");
					return BCME_RANGE;
				}
				val += (dbm < 0 || (*argv)[0] == '-') ? -1 : 1;
			}
			val = htod32(val);
			tx_pwr.pwr[pwr_len] = val;
			pwr_len++;
			argv++;
		}
		tx_pwr.len = pwr_len;
	}

	return wlu_iovar_set(wl, cmd->name, &tx_pwr, sizeof(wlc_ioctl_tx_pwr_t));
}

static int
wl_reg_info(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_OK;
	uint i;
	char chstr[CHANSPEC_STR_LEN];
	wl_reg_info_t *reg_info = (wl_reg_info_t *)buf;

	strcpy(buf, argv[0]);

	if ((ret = wlu_get(wl, cmd->get, buf, WLC_IOCTL_MEDLEN)) < 0) {
		printf("Error fetching: %d\n", ret);
		return ret;
	}
	/* in-place dtoh */
	reg_info->ver = dtoh16(reg_info->ver);
	reg_info->len = dtoh16(reg_info->len);
	if (reg_info->len < sizeof(*reg_info)) {
		printf("Unexpected length: %d\n", reg_info->len);
		return BCME_BADLEN;
	}

	reg_info->type = dtoh16(reg_info->type);
	reg_info->flags = dtoh16(reg_info->flags);
	reg_info->chspec = dtoh16(reg_info->chspec);
	reg_info->afc_bmp = dtoh16(reg_info->afc_bmp);
	reg_info->sp_bmp = dtoh16(reg_info->sp_bmp);
	reg_info->lpi_bmp = dtoh16(reg_info->lpi_bmp);

	printf("Regulatory information details:\n");
	printf("Type: 0x%04x/%u, Flags:0x%04x/%u, Ch:%s(0x%04x)\n",
			reg_info->type, reg_info->type,
			reg_info->flags, reg_info->flags,
			wf_chspec_ntoa(reg_info->chspec, chstr), reg_info->chspec);
	printf("reg_info_field: %d, override: %d\n",
			reg_info->reg_info_field, reg_info->reg_info_override);

	printf("AFC_BMP: 0x%04x, SP_BMP: 0x%04x, LPI_BMP: 0x%04x\n",
			reg_info->afc_bmp, reg_info->sp_bmp, reg_info->lpi_bmp);

	printf("AFC_EIRP: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->afc_eirp[i]);
	}
	printf("\nSP_EIRP: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->sp_eirp[i]);
	}
	printf("\nLPI_EIRP: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->lpi_eirp[i]);
	}

	printf("\nAFC_PSD: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->afc_psd[i]);
	}
	printf("\nSP_PSD: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->sp_psd[i]);
	}
	printf("\nLPI_PSD: \t");
	for (i = 0; i < TPE_PSD_COUNT; ++i) {
		printf("%d \t", reg_info->lpi_psd[i]);
	}
	printf("\n");

	return ret;
}

static int
wl_rand_ch(void *wl, cmd_t *cmd, char **argv)
{
	int ret = BCME_OK, val;
	char chstr[CHANSPEC_STR_LEN];
	char smbuf[WLC_IOCTL_SMLEN];
	chanspec_t chspec;
	int param = 0, param_len = 0;

	/* toss the command name */
	argv++;

	if (*argv) {
		param = atoi(*argv); /* optional parameter */
		param_len = sizeof(param);
	}
	if ((ret = wlu_iovar_getbuf(wl, cmd->name, &param, param_len,
			smbuf, sizeof(smbuf)))) {
		printf("err=%d \n", ret);
		return ret;
	}

	val = dtoh32(*((int*)smbuf));
	chspec = wl_chspec32_from_driver(val);
	wf_chspec_ntoa(chspec, chstr);
	printf("%s (0x%x)\n", chstr, chspec);

	return ret;
}

static int
wl_maclist_1(void *wl, cmd_t *cmd, char **argv)
{
	struct maclist *maclist;
	struct ether_addr *ea;
	uint i;
	int ret;

	strcpy(buf, argv[0]);

	if ((ret = wlu_get(wl, cmd->get, buf, WLC_IOCTL_MAXLEN)) < 0)
		return ret;

	maclist = (struct maclist *)buf;
	maclist->count = dtoh32(maclist->count);
	if (maclist->count > ((WLC_IOCTL_MAXLEN - sizeof(maclist->count)) / ETHER_ADDR_LEN)) {
		printf("%s: limiting count from %u to %u!\n", __FUNCTION__, maclist->count,
			(uint32)((WLC_IOCTL_MAXLEN - sizeof(maclist->count)) / ETHER_ADDR_LEN));
		maclist->count = (WLC_IOCTL_MAXLEN - sizeof(maclist->count)) / ETHER_ADDR_LEN;
	}

	for (i = 0, ea = maclist->ea; i < maclist->count; i++, ea++)
		printf("%s %s\n", cmd->name, wl_ether_etoa(ea));
	return 0;
}

static dbg_msg_t wl_map_msgs[] = {
	{1,	"Fronthaul-BSS"},
	{2,	"Backhaul-BSS"},
	{4,	"Backhaul-STA"},
	{32,	"Profile 1 Backhaul-STA Assoc Disallowed"},
	{64,	"Profile 2 Backhaul-STA Assoc Disallowed"},
	{128,	"Block Non-MAP STAs"},
	{0, ""}
};

void
wl_map_print(uint32 map)
{
	int i;
	printf("0x%x:", map);
	for (i = 0; wl_map_msgs[i].value; i++)
		if (map & wl_map_msgs[i].value)
			printf("  %s", wl_map_msgs[i].string);
	printf("\n");
	return;

}

static int
wl_map(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int map;

	if (!*++argv) {
		ret = wlu_iovar_getint(wl, cmd->name, &map);
		if (ret < 0)
			return ret;
		wl_map_print(map);
		return 0;
	}

	map = strtoul(*argv, NULL, 0);
	return wlu_iovar_setint(wl, cmd->name, map);
}

static int
wl_map_profile(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int map_profile;

	if (!*++argv) {
		ret = wlu_iovar_getint(wl, cmd->name, &map_profile);
		if (ret < 0)
			return ret;
		printf("map profile %d\n", map_profile);
		return 0;
	}

	map_profile = strtoul(*argv, NULL, 0);
	return wlu_iovar_setint(wl, cmd->name, map_profile);
}

static int
wl_map_8021q_settings(void *wl, cmd_t *cmd, char **argv)
{
	int ret;
	int prim_vlan_id;

	if (!*++argv) {
		ret = wlu_iovar_getint(wl, cmd->name, &prim_vlan_id);
		if (ret < 0)
			return ret;
		printf("Primary VLAN ID %d\n", dtoh16(prim_vlan_id));
		return 0;
	}

	prim_vlan_id = htod16(strtoul(*argv, NULL, 0));
	return wlu_iovar_setint(wl, cmd->name, prim_vlan_id);
}
