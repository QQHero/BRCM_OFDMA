/*
 * HND RTE misc interfaces.
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
 * $Id: rte.h 799461 2021-05-31 14:00:17Z $
 */

#ifndef _rte_h_
#define _rte_h_

#include <typedefs.h>
#include <siutils.h>
#include <osl_ext.h>

#define WATCHDOG_DISABLE	0

/* ========================== system ========================== */
si_t *hnd_init(void);
/* Each CPU/Arch must implement this interface - idle loop */
void hnd_idle(si_t *sih);
void hnd_sys_enab(si_t *sih);
void hnd_set_stack_size(uint stksz);
#if defined(__ARM_ARCH_7A__) && defined(RTE_CACHED) && !defined(BCM_BOOTLOADER)
extern void ca7_execute_protect_on(si_t *sih);
extern void ca7_disable_rodata_protection(si_t *sih);
#else
static inline void ca7_execute_protect_on(si_t *sih) { return; }
#endif /* defined(__ARM_ARCH_7R__) && defined(RTE_CACHED) */

/* ======================= debug ===================== */
void hnd_memtrace_enab(bool on);
void BCMATTACHFN(hnd_debug_info_net_dev_set)(uint32 net_dev);
void BCMATTACHFN(hnd_debug_info_bus_dev_set)(uint32 bus_dev);

/* ============================ misc =========================== */
extern int hnd_register_trapnotify_callback(void *cb, void *arg);
extern void *hnd_get_fatal_logbuf(uint32 requested_size, uint32 *allocated_size);
extern void* hnd_enable_gci_gpioint(uint8 gpio, uint8 sts, gci_gpio_handler_t hdlr, void *arg);
extern void hnd_disable_gci_gpioint(void *gci_i);
/* ============================ thread =========================== */
osl_ext_status_t hnd_thread_create(char* name, void *stack, unsigned int stack_size,
	osl_ext_task_priority_t priority, osl_ext_task_entry func, osl_ext_task_arg_t arg,
	osl_ext_task_t *task);
void hnd_thread_run_scheduler(void);
void hnd_thread_default_implementation(osl_ext_task_arg_t arg);
void hnd_thread_initialize(void);

osl_ext_status_t hnd_thread_set_scheduler(osl_ext_task_t *thread, void *scheduler);
void* hnd_thread_get_scheduler(osl_ext_task_t *thread);
osl_ext_task_t* hnd_thread_find_by_name(const char *name);

bool hnd_in_isr(void);
osl_ext_task_t* hnd_get_last_thread(void);

osl_ext_status_t hnd_thread_watchdog_init(osl_ext_task_t *thread, osl_ext_time_ms_t interval,
	osl_ext_time_ms_t timeout);
osl_ext_status_t hnd_thread_watchdog_start(osl_ext_task_t *thread, osl_ext_time_ms_t interval,
	osl_ext_time_ms_t timeout);
osl_ext_status_t hnd_thread_watchdog_feed(osl_ext_task_t *thread, osl_ext_time_ms_t timeout);
void hnd_thread_watchdog_enable(bool enable);

void hnd_thread_check_os_stack(void);

#endif /* _rte_h_ */
