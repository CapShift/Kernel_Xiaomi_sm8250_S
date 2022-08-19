/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM scheduler
#define TRACE_INCLUDE_PATH ../../drivers/voyager/include/trace/hooks
#if !defined(_TRACE_HOOK_SCHEDULER_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_SCHEDULER_H
#include <linux/tracepoint.h>
#include "vendor_hooks.h"

/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */
#if defined(CONFIG_TRACEPOINTS)
struct sugov_cpu;
DECLARE_RESTRICTED_HOOK(cpufreq_get_util_hook,
	TP_PROTO(struct sugov_cpu *sg_cpu, unsigned long util),
	TP_ARGS(sg_cpu, util), 1);

DECLARE_RESTRICTED_HOOK(cpufreq_start_hook,
	TP_PROTO(struct cpufreq_policy *policy, void (*uu)(struct update_util_data *data, u64 time, unsigned int flags)),
	TP_ARGS(policy, uu), 1);
#endif
#endif /* _TRACE_HOOK_SCHEDULER_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
