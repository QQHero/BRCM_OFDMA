# Config makefile that maps config based target names to features.
#
# Copyright (C) 2016, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#
# <<Broadcom-WL-IPTag/Proprietary:>>
#
# $Id: 4363c0-roml-config.mk 800465 2021-06-28 20:30:30Z $

# Variables that map config names to features - 'TARGET_OPTIONS_config_[bus-type]_xxx'.
TARGET_OPTIONS_config_pcie_base	:= pcie-ag-splitrx-fdap-mbss-txbf-pktctx-amsdutx-ampduretry-chkd2hdma-splitassoc-hostmemucode-bgdfs-assoc_lt-airiq
ifeq ($(WLCLMLOAD),1)
	TARGET_OPTIONS_config_pcie_base := $(addsuffix -noclminc-clm_min,$(TARGET_OPTIONS_config_pcie_base))
endif
ifeq ($(__CONFIG_GMAC3__),1)
ifeq ($(__CONFIG_FLATLAS__),1)
	WLCX := "-wlcx"
endif
endif
TARGET_OPTIONS_config_ext_and_int := mfp-wnm-osen-wl11k-wl11u-proptxstatus-obss-dbwsw-ringer-dmaindex16-stamon-fbt-mbo-map
TARGET_OPTIONS_config_pcie_external := $(TARGET_OPTIONS_config_pcie_base)-$(TARGET_OPTIONS_config_ext_and_int)
TARGET_OPTIONS_config_pcie_wltest := $(TARGET_OPTIONS_config_pcie_base)-mfgtest-seqcmds-phydbg-phydump-dbgam-dbgams-ringer-dmaindex16-hostpmac

TARGET_OPTIONS_config_pcie_internal := $(TARGET_OPTIONS_config_pcie_base)-$(TARGET_OPTIONS_config_ext_and_int)-txpwr-err-assert-dbgam-dbgams-mfgtest-dump$(WLCX)-acksupr-authrmf-dhdhdr-htxhdr-amsdufrag-mbo-hostpmac
TARGET_OPTIONS_config_pcie_release := pcie-ag-splitrx-fdap-mbss-mfp-wnm-osen-wl11k-wl11u-txbf-pktctx-amsdutx-ampduretry-chkd2hdma-proptxstatus-11nprop-obss-dbwsw-ringer-dmaindex16-bgdfs-stamon-hostpmac-splitassoc-hostmemucode-dhdhdr-fbt-htxhdr-amsdufrag-assoc_lt-airiq-mbo
TARGET_OPTIONS_config_pcie_debug   := pcie-ag-splitrx-fdap-mbss-mfp-wnm-osen-wl11k-wl11u-txbf-pktctx-amsdutx-ampduretry-chkd2hdma-proptxstatus-11nprop-obss-dbwsw-ringer-dmaindex16-bgdfs-stamon-hostpmac-splitassoc-hostmemucode-dhdhdr-fbt-htxhdr-amsdufrag-assoc_lt-airiq-mbo
TARGET_OPTIONS_config_pcie_mfgtest := pcie-ag-splitrx-fdap-mbss-mfgtest-seqcmds-phydbg-phydump-txbf-pktctx-amsdutx-ampduretry-chkd2hdma-proptxstatus-11nprop-dbgam-dbgams-ringer-dmaindex16-bgdfs-hostpmac-splitassoc-hostmemucode-fbt-assoc_lt-airiq-mbo
