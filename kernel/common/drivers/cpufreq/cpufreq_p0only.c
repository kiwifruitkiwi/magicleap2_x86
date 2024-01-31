/*
 * P0 only CPU Frequency Governor.
 *
 * Copyright (C) 2020 - Advanced Micro Devices, Inc. All rights reserved.
 */

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>

static void cpufreq_gov_p0only_limits(struct cpufreq_policy *policy)
{
	pr_debug("setting to %u kHz\n", policy->max);
	__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_L);
}

static struct cpufreq_governor cpufreq_gov_p0only = {
	.name		= "p0only",
	.limits		= cpufreq_gov_p0only_limits,
	.owner		= THIS_MODULE,
};

static int __init cpufreq_gov_p0only_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_p0only);
}

static void __exit cpufreq_gov_p0only_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_p0only);
}

module_init(cpufreq_gov_p0only_init);
module_exit(cpufreq_gov_p0only_exit);
