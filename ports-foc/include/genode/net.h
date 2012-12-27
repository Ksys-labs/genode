/*
 * \brief  Genode C API network functions needed by OKLinux
 * \author Stefan Kalkowski
 * \date   2009-09-10
 */

/*
 * Copyright (C) 2009-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__GENODE__NET_H_
#define _INCLUDE__GENODE__NET_H_

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

#include <genode/linkage.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_GENODE_NET 4

L4_CV void         genode_net_run         (int count);

L4_CV l4_cap_idx_t genode_net_irq_cap     (int num);
L4_CV int          genode_net_ready       (int num);
L4_CV void         genode_net_start       (int num, void *dev, FASTCALL void (*func)(void*, void*, unsigned long));
L4_CV void         genode_net_stop        (int num);
L4_CV void         genode_net_mac         (int num, void* mac_addr, unsigned long size);
L4_CV int          genode_net_tx          (int num, void* addr,     unsigned long len);
L4_CV int          genode_net_tx_ack_avail(int num);
L4_CV void         genode_net_tx_ack      (int num);
L4_CV void         genode_net_rx_receive  (int num);
L4_CV void        *genode_net_memcpy      (void *dst, void const *src, unsigned long size);
L4_CV int          genode_net_strcmp      (const char *s1, const char *s2, unsigned long size);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE__GENODE__NET_H_ */
