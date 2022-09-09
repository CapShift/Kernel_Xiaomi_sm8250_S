/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/energy_model.h>
#include "include/sched_ext.h"

/*
 *  em_perf_domain flags:
 *
 *  EM_PERF_DOMAIN_MICROWATTS: The power values are in micro-Watts or some
 *  other scale.
 *
 *  EM_PERF_DOMAIN_SKIP_INEFFICIENCIES: Skip inefficient states when estimating
 *  energy consumption.
 *
 *  EM_PERF_DOMAIN_ARTIFICIAL: The power values are artificial and might be
 *  created by platform missing real power information
 */
#define EM_PERF_DOMAIN_MICROWATTS BIT(0)
#define EM_PERF_DOMAIN_SKIP_INEFFICIENCIES BIT(1)
#define EM_PERF_DOMAIN_ARTIFICIAL BIT(2)

/*
 * em_perf_state flags:
 *
 * EM_PERF_STATE_INEFFICIENT: The performance state is inefficient. There is
 * in this em_perf_domain, another performance state with a higher frequency
 * but a lower or equal power cost. Such inefficient states are ignored when
 * using em_pd_get_efficient_*() functions.
 */
#define EM_PERF_STATE_INEFFICIENT BIT(0)

#define em_estimate_energy(cost, sum_util, scale_cpu) \
	(((cost) * (sum_util)) / (scale_cpu))

static inline unsigned long map_util_perf(unsigned long util)
{
	return util + (util >> 2);
}

/**
 * em_pd_get_efficient_state() - Get an efficient performance state from the EM
 * @pd   : Performance domain for which we want an efficient frequency
 * @freq : Frequency to map with the EM
 *
 * It is called from the scheduler code quite frequently and as a consequence
 * doesn't implement any check.
 *
 * Return: An efficient performance state, high enough to meet @freq
 * requirement.
 */
static inline
struct em_cap_state *em_pd_get_efficient_state(struct em_perf_domain *pd,
						unsigned long freq)
{
	struct em_cap_state *cs;
	int i;

	for (i = 0; i < pd->nr_cap_states; i++) {
		cs = &pd->table[i];
		if (cs->frequency >= freq) {
			if (pd->flags & EM_PERF_DOMAIN_SKIP_INEFFICIENCIES &&
			    cs->flags & EM_PERF_STATE_INEFFICIENT)
				continue;
			break;
		}
	}

	return cs;
}

/**
 * em_cpu_energy() - Estimates the energy consumed by the CPUs of a
 *		performance domain
 * @pd		: performance domain for which energy has to be estimated
 * @max_util	: highest utilization among CPUs of the domain
 * @sum_util	: sum of the utilization of all CPUs in the domain
 * @allowed_cpu_cap	: maximum allowed CPU capacity for the @pd, which
 *			  might reflect reduced frequency (due to thermal)
 *
 * This function must be used only for CPU devices. There is no validation,
 * i.e. if the EM is a CPU type and has cpumask allocated. It is called from
 * the scheduler code quite frequently and that is why there is not checks.
 *
 * Return: the sum of the energy consumed by the CPUs of the domain assuming
 * a capacity state satisfying the max utilization of the domain.
 */
inline unsigned long em_cpu_energy(struct em_perf_domain *pd,
				unsigned long max_util, unsigned long sum_util,
				unsigned long allowed_cpu_cap)
{
	unsigned long freq, scale_cpu;
	struct em_cap_state *cs;
	int cpu;

	if (!sum_util)
		return 0;

	/*
	 * In order to predict the performance state, map the utilization of
	 * the most utilized CPU of the performance domain to a requested
	 * frequency, like schedutil. Take also into account that the real
	 * frequency might be set lower (due to thermal capping). Thus, clamp
	 * max utilization to the allowed CPU capacity before calculating
	 * effective frequency.
	 */
	cpu = cpumask_first(to_cpumask(pd->cpus));
	scale_cpu = arch_scale_cpu_capacity(NULL, cpu);
	cs = &pd->table[pd->nr_cap_states - 1];

	max_util = map_util_perf(max_util);
	max_util = min(max_util, allowed_cpu_cap);
	freq = map_util_freq(max_util, cs->frequency, scale_cpu);

	/*
	 * Find the lowest performance state of the Energy Model above the
	 * requested frequency.
	 */
	cs = em_pd_get_efficient_state(pd, freq);

	/*
	 * The capacity of a CPU in the domain at the performance state (ps)
	 * can be computed as:
	 *
	 *             cs>freq * scale_cpu
	 *   cs>cap = --------------------                          (1)
	 *                 cpu_max_freq
	 *
	 * So, ignoring the costs of idle states (which are not available in
	 * the EM), the energy consumed by this CPU at that performance state
	 * is estimated as:
	 *
	 *             cs>power * cpu_util
	 *   cpu_nrg = --------------------                          (2)
	 *                   cs>cap
	 *
	 * since 'cpu_util / cs>cap' represents its percentage of busy time.
	 *
	 *   NOTE: Although the result of this computation actually is in
	 *         units of power, it can be manipulated as an energy value
	 *         over a scheduling period, since it is assumed to be
	 *         constant during that interval.
	 *
	 * By injecting (1) in (2), 'cpu_nrg' can be re-expressed as a product
	 * of two terms:
	 *
	 *             cs>power * cpu_max_freq   cpu_util
	 *   cpu_nrg = ------------------------ * ---------          (3)
	 *                    cs>freq            scale_cpu
	 *
	 * The first term is static, and is stored in the em_perf_state struct
	 * as 'cs>cost'.
	 *
	 * Since all CPUs of the domain have the same micro-architecture, they
	 * share the same 'cs>cost', and the same CPU capacity. Hence, the
	 * total energy of the domain (which is the simple sum of the energy of
	 * all of its CPUs) can be factorized as:
	 *
	 *            cs>cost * \Sum cpu_util
	 *   pd_nrg = ------------------------                       (4)
	 *                  scale_cpu
	 */
	return em_estimate_energy(cs->cost, sum_util, scale_cpu);
}
