/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _INIT_SENTIENT_H
#define _INIT_SENTIENT_H

#ifdef CONFIG_SENTIENT_KERNEL
void sentient_awakening(void);
#else
static inline void sentient_awakening(void) { }
#endif

#endif /* _INIT_SENTIENT_H */
