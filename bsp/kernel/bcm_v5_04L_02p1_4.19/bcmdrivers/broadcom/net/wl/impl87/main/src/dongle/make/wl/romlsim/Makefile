# Front-end subdir level makefile for hndrte based standalone programs.
#
# Copyright 2022 Broadcom
#
# This program is the proprietary software of Broadcom and/or
# its licensors, and may only be used, duplicated, modified or distributed
# pursuant to the terms and conditions of a separate, written license
# agreement executed between you and Broadcom (an "Authorized License").
# Except as set forth in an Authorized License, Broadcom grants no license
# (express or implied), right to use, or waiver of any kind with respect to
# the Software, and Broadcom expressly reserves all rights in and to the
# Software and all intellectual property rights therein.  IF YOU HAVE NO
# AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
# WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
# THE SOFTWARE.
#
# Except as expressly set forth in the Authorized License,
#
# 1. This program, including its structure, sequence and organization,
# constitutes the valuable trade secrets of Broadcom, and you shall use
# all reasonable efforts to protect the confidentiality thereof, and to
# use this information only in connection with your use of Broadcom
# integrated circuit products.
#
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
# "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
# REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR
# OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
# DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
# NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
# ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
# CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
# OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
#
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
# BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL,
# SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR
# IN ANY WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
# IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR (ii)
# ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF
# OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY
# NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
#
# $Id$
#
# Invoked from src/dongle/make/wl/Makefile.
#

ifeq ($(TOPDIR),)
$(error TOPDIR is not defined)
endif

SUBDIR := $(CURDIR)
SUBDIR := $(SUBDIR:$(TOPDIR)/%=%)

SUBBLDDIR ?= $(SUBDIR)

#####################################################################################

ifeq ($(SBRAND),)

BRANDS := $(patsubst %.mk,%,$(filter-out rules.mk,$(wildcard *.mk)))

# default make target
.PHONY: default
default: all

# show all brands
showbrands: FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# show all targets
showtgts: FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# show all targets in a brand
$(BRANDS:=/showtgts): FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# show all targets
showmaxsize_tgts: FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# show all targets in a brand
$(BRANDS:=/showmaxsize_tgts): FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# clean all brands
.PHONY: clean
clean: $(BRANDS:=/clean)

# clean all targets in a brand
$(BRANDS:=/clean): FORCE
	$(MAKE) SBRAND=$(@:/clean=) $@

# clean a specific target in a brand
$(BRANDS:=/%/clean): _BRAND = $(firstword $(subst /, ,$@))
$(BRANDS:=/%/clean): FORCE
	$(MAKE) SBRAND=$(_BRAND) $@

# build all brands
.PHONY: all
all: $(BRANDS)

# build all targets in a brand
$(BRANDS): FORCE
	$(MAKE) SBRAND=$@

# build a specific target in a brand
$(BRANDS:=/%): _BRAND = $(firstword $(subst /, ,$@))
$(BRANDS:=/%): FORCE
	$(MAKE) SBRAND=$(_BRAND) $@

endif	# !SBRAND

#####################################################################################

ifneq ($(SBRAND),)

# prevent multiple targets from invoking the same romlsim make
.NOTPARALLEL:

include $(SBRAND).mk

ifndef ROMLDIR
$(error ROMLDIR is not defined)
endif

ifeq ($(wildcard $(ROMLDIR)/Makefile),)
$(error $(ROMLDIR)/Makefile does not exist)
endif

# build both ROM and RAM targets
.PHONY: all
all: $(ROMLDIR) $(SBRAND)

# clean ROM subdir
roml/clean: FORCE
	$(MAKE) -C $(ROMLDIR) clean

# clean all targets in a brand
.PHONY: $(SBRAND)/clean
$(SBRAND)/clean: roml/clean
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# clean a specific target in a brand
$(SBRAND)/%/clean: FORCE
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# build all targets
.PHONY: $(SBRAND)
$(SBRAND): roml
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# build a specific target
.PHONY: $(SBRAND)/%
$(SBRAND)/%: roml
	$(MAKE) -C $(TOPDIR) -f $(TOPMK) SUBDIR=$(SUBDIR) SUBBLDDIR=$(SUBBLDDIR) $@

# build rom
roml: FORCE
	$(MAKE) -C $(ROMLDIR) all

endif	# SBRAND

.PHONY: FORCE
FORCE:
