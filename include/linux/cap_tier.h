/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Custom privilege tiers that sit between an unprivileged user and a
 * CAP_SYS_ADMIN administrator.
 *
 * A task is in tier 1 (Operator) or tier 2 (Application Administrator) when
 * it holds CAP_CUSTOM_SUDO_L1 / CAP_CUSTOM_SUDO_L2 in its effective set, or,
 * in the initial user namespace, when its egid or one of its supplementary
 * groups matches the gid configured through the kernel.custom_sudo_l1_gid /
 * kernel.custom_sudo_l2_gid sysctls.
 *
 * Tier 2 includes everything tier 1 can do.  Each tier is granted a fixed
 * subset of ordinary capabilities by cap_capable().  The sets below must
 * never contain a capability that lets its holder become root, for example
 * CAP_SYS_ADMIN, CAP_SETUID, CAP_SETGID, CAP_SETFCAP, CAP_DAC_OVERRIDE,
 * CAP_CHOWN, CAP_FOWNER, CAP_SYS_PTRACE, CAP_SYS_MODULE, CAP_SYS_RAWIO,
 * CAP_MKNOD or CAP_SYS_BOOT (kexec_load() loads an arbitrary kernel).
 */
#ifndef _LINUX_CAP_TIER_H
#define _LINUX_CAP_TIER_H

#include <linux/bits.h>
#include <linux/capability.h>

struct cred;

/* Tier 1, "Operator": day to day operation, cannot reconfigure the system. */
#define CAP_TIER_L1_MASK	(BIT_ULL(CAP_KILL) |			\
				 BIT_ULL(CAP_SYS_NICE) |		\
				 BIT_ULL(CAP_SYSLOG) |			\
				 BIT_ULL(CAP_WAKE_ALARM) |		\
				 BIT_ULL(CAP_BLOCK_SUSPEND))

/* Tier 2, "Application Administrator": tier 1 plus running services. */
#define CAP_TIER_L2_MASK	(CAP_TIER_L1_MASK |			\
				 BIT_ULL(CAP_NET_BIND_SERVICE) |	\
				 BIT_ULL(CAP_SYS_TIME) |		\
				 BIT_ULL(CAP_SYS_RESOURCE) |		\
				 BIT_ULL(CAP_IPC_LOCK))

/**
 * cap_custom_tier - privilege tier of a set of credentials
 * @cred: the credentials to examine
 *
 * Return 0 for no tier, 1 for Operator and 2 for Application Administrator.
 * Only reads @cred: no locks, no allocation, no audit records.
 */
int cap_custom_tier(const struct cred *cred);

#endif /* _LINUX_CAP_TIER_H */
