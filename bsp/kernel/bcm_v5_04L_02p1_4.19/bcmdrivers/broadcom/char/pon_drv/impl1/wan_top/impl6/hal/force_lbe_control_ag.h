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

#ifndef _FORCE_LBE_CONTROL_AG_H_
#define _FORCE_LBE_CONTROL_AG_H_

#include "access_macros.h"
#include "bdmf_interface.h"
#ifdef USE_BDMF_SHELL
#include "bdmf_shell.h"
#endif
#include "rdp_common.h"


/**************************************************************************************************/
/* cfg_force_lbe: CFG_FORCE_LBE - 0: The MAC controls the LBE signal.1: The LBE signal is forced  */
/*                to cfg_force_lbe_value.                                                         */
/* cfg_force_lbe_value: CFG_FORCE_LBE_VALUE - This field is only used when cfg_force_lbe is set.  */
/*                       0: LBE is set to 0. 1: LBE is set to 1.                                  */
/* cfg_force_lbe_oe: CFG_FORCE_LBE_OE - 0: The MAC and cr_xgwan_top_wan_misc_wan_cfg_laser_oe con */
/*                   trol the LBE output enable signal.  1: The LBE output enable signal is force */
/*                   d to cfg_force_lbe_oe_value.                                                 */
/* cfg_force_lbe_oe_value: CFG_FORCE_LBE_OE_VALUE - This field is only used when cfg_force_lbe_oe */
/*                          is set.  This signal is then inverted prior to connecting to the OEB  */
/*                         pin.  0: LBE output enable is set to 0. 1: LBE output enable is set to */
/*                          1.                                                                    */
/**************************************************************************************************/
typedef struct
{
    bdmf_boolean cfg_force_lbe;
    bdmf_boolean cfg_force_lbe_value;
    bdmf_boolean cfg_force_lbe_oe;
    bdmf_boolean cfg_force_lbe_oe_value;
} force_lbe_control_force_lbe_control_control;

bdmf_error_t ag_drv_force_lbe_control_force_lbe_control_control_set(const force_lbe_control_force_lbe_control_control *force_lbe_control_control);
bdmf_error_t ag_drv_force_lbe_control_force_lbe_control_control_get(force_lbe_control_force_lbe_control_control *force_lbe_control_control);

#ifdef USE_BDMF_SHELL
enum
{
    cli_force_lbe_control_force_lbe_control_control,
};

int bcm_force_lbe_control_cli_get(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);
bdmfmon_handle_t ag_drv_force_lbe_control_cli_init(bdmfmon_handle_t driver_dir);
#endif


#endif

