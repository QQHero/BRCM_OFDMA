/*
   Copyright (c) 2015 Broadcom
   All Rights Reserved

    <:label-BRCM:2015:DUAL/GPL:standard

    Unless you and Broadcom execute a separate written software license
    agreement governing use of this software, this software is licensed
    to you under the terms of the GNU General Public License version 2
    (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
    with the following added to such license:

       As a special exception, the copyright holders of this software give
       you permission to link this software with independent modules, and
       to copy and distribute the resulting executable under terms of your
       choice, provided that you also meet, for each linked independent
       module, the terms and conditions of the license of that module.
       An independent module is a module which is not derived from this
       software.  The special exception does not apply to any modifications
       of the software.

    Not withstanding the above, under no circumstances may you combine
    this software in any way with any other Broadcom software provided
    under a license other than the GPL, without Broadcom's express prior
    written consent.

:>
*/

#ifndef _PMI_AG_H_
#define _PMI_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"

bdmf_error_t ag_drv_pmi_lp_0_set(bdmf_boolean cr_xgwan_top_wan_misc_pmi_lp_en, bdmf_boolean cr_xgwan_top_wan_misc_pmi_lp_write);
bdmf_error_t ag_drv_pmi_lp_0_get(bdmf_boolean *cr_xgwan_top_wan_misc_pmi_lp_en, bdmf_boolean *cr_xgwan_top_wan_misc_pmi_lp_write);
bdmf_error_t ag_drv_pmi_lp_1_set(uint32_t cr_xgwan_top_wan_misc_pmi_lp_addr);
bdmf_error_t ag_drv_pmi_lp_1_get(uint32_t *cr_xgwan_top_wan_misc_pmi_lp_addr);
bdmf_error_t ag_drv_pmi_lp_2_set(uint16_t cr_xgwan_top_wan_misc_pmi_lp_wrdata, uint16_t cr_xgwan_top_wan_misc_pmi_lp_maskdata);
bdmf_error_t ag_drv_pmi_lp_2_get(uint16_t *cr_xgwan_top_wan_misc_pmi_lp_wrdata, uint16_t *cr_xgwan_top_wan_misc_pmi_lp_maskdata);
bdmf_error_t ag_drv_pmi_lp_3_get(bdmf_boolean *pmi_lp_err, bdmf_boolean *pmi_lp_ack, uint16_t *pmi_lp_rddata);

#ifdef USE_BDMF_SHELL
enum
{
    cli_pmi_lp_0,
    cli_pmi_lp_1,
    cli_pmi_lp_2,
    cli_pmi_lp_3,
};

int bcm_pmi_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_pmi_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

