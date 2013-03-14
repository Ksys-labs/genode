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

L4_CV l4_cap_idx_t genode_netsrv_irq_cap     (void);
L4_CV int          genode_netsrv_ready       (void);
L4_CV void         genode_netsrv_start       (void *dev, FASTCALL void (*func)(void*, void*, unsigned long));
L4_CV void         genode_netsrv_stop        (void);
L4_CV void         genode_netsrv_mac         (void* mac_addr, unsigned long size);
L4_CV int          genode_netsrv_tx          (void* addr,     unsigned long len);
L4_CV int          genode_netsrv_tx_ack_avail(void);
L4_CV void         genode_netsrv_tx_ack      (void);
L4_CV void         genode_netsrv_rx_receive  (void);
L4_CV void        *genode_netsrv_memcpy      (void *dst, void const *src, unsigned long size);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE__GENODE__NET_H_ */
