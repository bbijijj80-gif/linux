.. SPDX-License-Identifier: GPL-2.0

=========================
Custom privilege tiers
=========================

Besides root and the ordinary capabilities, the kernel knows two privilege
tiers that sit between an unprivileged user and a ``CAP_SYS_ADMIN``
administrator.  They are enforced by ``cap_capable()`` in
``security/commoncap.c``, so every ``capable()`` / ``ns_capable()`` check in
the kernel honours them.

Tiers
=====

Tier 2 includes everything tier 1 can do.

======= ========================== ===========================================
Tier    Name                       Granted capabilities
======= ========================== ===========================================
1       Operator                   ``CAP_KILL``, ``CAP_SYS_NICE``,
                                   ``CAP_SYSLOG``, ``CAP_WAKE_ALARM``,
                                   ``CAP_BLOCK_SUSPEND``
2       Application Administrator  tier 1, plus ``CAP_NET_BIND_SERVICE``,
                                   ``CAP_SYS_TIME``, ``CAP_SYS_RESOURCE``,
                                   ``CAP_IPC_LOCK``
======= ========================== ===========================================

The sets are ``CAP_TIER_L1_MASK`` and ``CAP_TIER_L2_MASK`` in
``include/linux/cap_tier.h``.  They deliberately exclude every capability that
lets its holder become root: ``CAP_SYS_ADMIN``, ``CAP_SETUID``,
``CAP_SETGID``, ``CAP_SETFCAP``, ``CAP_DAC_OVERRIDE``, ``CAP_CHOWN``,
``CAP_FOWNER``, ``CAP_SYS_PTRACE``, ``CAP_SYS_MODULE``, ``CAP_SYS_RAWIO``,
``CAP_MKNOD`` and ``CAP_SYS_BOOT`` (``kexec_load()`` would load an arbitrary
kernel).  Do not add such a capability to a tier.

How a task gets a tier
======================

* By holding ``CAP_CUSTOM_SUDO_L1`` (41) or ``CAP_CUSTOM_SUDO_L2`` (42) in its
  effective set.  They are ordinary capability bits: they can be set with
  ``capset()`` or put on an executable as file capabilities.
* By group membership, in the initial user namespace only.  A task whose egid
  or supplementary groups contain the gid stored in the following sysctls is
  in the corresponding tier, much like members of a ``wheel`` group are
  allowed to use sudo::

    # echo 2001 > /proc/sys/kernel/custom_sudo_l1_gid
    # echo 2002 > /proc/sys/kernel/custom_sudo_l2_gid

  The default, 4294967295, means "no group".  Only root can write them.

The tier a task holds is shown in the panic banner as ``[Level: ...]``.

Limits
======

* A task in a child user namespace never gets rights over the host through a
  tier, and group tiers are ignored there altogether.
* Security modules still apply.  SELinux, for instance, additionally checks
  the ``capability`` permission of the task's domain; the two custom
  capabilities are the ``custom_sudo_l1`` and ``custom_sudo_l2`` permissions
  of the ``capability2`` class.
* Code that reads ``cred->cap_effective`` directly instead of calling
  ``capable()`` does not see tiers.
* ``CAP_SYSLOG`` lets an Operator read the kernel log and, with
  ``kptr_restrict=1``, see kernel pointers.
