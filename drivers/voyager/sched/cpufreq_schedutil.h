/* SPDX-License-Identifier: GPL-2.0 */
#include "../include/sched_ext.h"

struct sugov_tunables {
	struct gov_attr_set	attr_set;
	unsigned int		up_rate_limit_us;
	unsigned int		down_rate_limit_us;
	unsigned int		hispeed_load;
	unsigned int		hispeed_freq;
	unsigned int		rtg_boost_freq;
	bool			pl;
#ifdef CONFIG_CPUFREQ_GOV_SCHEDUTIL_TARGET_LOAD
	spinlock_t		target_loads_lock;
	unsigned int		*target_loads;
	int			ntarget_loads;
#endif /* CONFIG_CPUFREQ_GOV_SCHEDUTIL_TARGET_LOAD */
	unsigned int		target_load_thresh;
	unsigned int		target_load_shift;
};

struct sugov_policy {
	struct cpufreq_policy	*policy;

	u64 last_ws;
	u64 curr_cycles;
	u64 last_cyc_update_time;
	unsigned long avg_cap;
	struct sugov_tunables	*tunables;
	struct list_head	tunables_hook;
	unsigned long hispeed_util;
	unsigned long rtg_boost_util;
	unsigned long max;

	raw_spinlock_t		update_lock;	/* For shared policies */
	u64			last_freq_update_time;
	s64			min_rate_limit_ns;
	s64			up_rate_delay_ns;
	s64			down_rate_delay_ns;
	unsigned int		next_freq;
	unsigned int		cached_raw_freq;
	unsigned int		prev_cached_raw_freq;

	/* The next fields are only needed if fast switch cannot be used: */
	struct			irq_work irq_work;
	struct			kthread_work work;
	struct			mutex work_lock;
	struct			kthread_worker worker;
	struct task_struct	*thread;
	bool			work_in_progress;

	bool			limits_changed;
	bool			need_freq_update;
};

struct sugov_cpu {
	struct update_util_data	update_util;
	struct sugov_policy	*sg_policy;
	unsigned int		cpu;

	bool			iowait_boost_pending;
	unsigned int		iowait_boost;
	u64			last_update;

	struct sched_walt_cpu_load walt_load;

	unsigned long util;
	unsigned int flags;

	unsigned long		bw_dl;
	unsigned long		min;
	unsigned long		max;

	/* The field below is for single-CPU policies only: */
#ifdef CONFIG_NO_HZ_COMMON
	unsigned long		saved_idle_calls;
#endif
};

static DEFINE_PER_CPU(struct sugov_cpu, sugov_cpu);
static unsigned int stale_ns;
static DEFINE_PER_CPU(struct sugov_tunables *, cached_tunables);

unsigned long sugov_get_util(struct sugov_cpu *sg_cpu);
