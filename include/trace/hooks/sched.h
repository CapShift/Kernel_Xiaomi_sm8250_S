/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched
#define TRACE_INCLUDE_PATH trace/hooks
#if !defined(_TRACE_HOOK_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HOOK_SCHED_H
#include <linux/tracepoint.h>
#include <trace/hooks/vendor_hooks.h>
/*
 * Following tracepoints are not exported in tracefs and provide a
 * mechanism for vendor modules to hook and extend functionality
 */
#if defined(CONFIG_TRACEPOINTS)
struct task_struct;
DECLARE_RESTRICTED_HOOK(android_rvh_check_preempt_wakeup,
	TP_PROTO(struct task_struct *p, int *ignore),
	TP_ARGS(p, ignore), 1);
DECLARE_RESTRICTED_HOOK(android_rvh_check_preempt_tick,
	TP_PROTO(struct task_struct *p, unsigned long *ideal_runtime),
	TP_ARGS(p, ideal_runtime), 1);
DECLARE_RESTRICTED_HOOK(android_rvh_find_best_target,
	TP_PROTO(struct task_struct *p, int cpu, int *ignore),
	TP_ARGS(p, cpu, ignore), 1);
struct cpumask;
DECLARE_RESTRICTED_HOOK(android_rvh_cpupri_find_fitness,
	TP_PROTO(struct task_struct *p, struct cpumask *lowest_mask),
	TP_ARGS(p, lowest_mask), 1);
#endif

/* macro versions of hooks are no longer required */

#endif /* _TRACE_HOOK_SCHED_H */
/* This part must be outside protection */
#include <trace/define_trace.h>
