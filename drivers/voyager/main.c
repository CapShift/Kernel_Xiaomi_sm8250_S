// SPDX-License-Identifier: GPL-2.0

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/signal.h>
#include <misc/voyager.h>

#include <trace/hooks/misc.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("The Voyager");
MODULE_DESCRIPTION("kernel addon");
MODULE_VERSION("0.0.1");

bool skip_charge_therm;
bool mi_thermal_switch;
bool inited = false;
bool enhance_background = true;

module_param(skip_charge_therm, bool, 0644);
module_param(mi_thermal_switch, bool, 0644);
module_param(enhance_background, bool, 0644);

#define vk_err(fmt, ...) \
	pr_err("kernel addon: %s: " fmt, __func__, ##__VA_ARGS__)

static const char *target[] = {
        "MiuiMemoryServic",
        "lmkd",
};

static inline bool is_match(struct task_struct *tsk)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(target); i++) {
		if (!strcmp(target[i], tsk->comm))
			return true;
	}

	return false;
}

inline void sigkill_filter(struct siginfo *info, pid_t pid, bool *ignored)
{
	struct task_struct *pid_task;
	struct pid *pid_struct;
	int sig;

	if (sig != SIGKILL && sig != SIGTERM)
	        return;
        
	sig = info->si_signo;
	if (pid < 0)
		pid = -pid;

	pid_struct = find_get_pid(pid);
	if (IS_ERR(pid_struct)) {
	        return;
	}

	pid_task = get_pid_task(pid_struct, PIDTYPE_PID);
	if (IS_ERR(pid_task)) {
	        return;
	}

        if (is_match(current)) {
                vk_err("Blocking \"%s\"(%d) send signal %d to "
			"\"%s\"(%d)\n", NULL);
	        *ignored = true;
        }
}

static int __init kernel_addon_init(void) {
        printk(KERN_INFO "voyager kernel addon initialized");
        mi_thermal_switch = false;
        skip_charge_therm = false;
      
        inited = true;
        return 0;
}

static void __exit kernel_addon_exit(void) {
        printk(KERN_INFO "kernel addon exit");
}

module_init(kernel_addon_init);
module_exit(kernel_addon_exit);
