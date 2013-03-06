/*
 * \brief  Genode C API for PCI on L4Linux
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2013-03-04
 */

/*
 * Copyright (C) 2013 Ksys Labs LLC
 * Copyright (C) 2009-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__GENODE__PCI_H_
#define _INCLUDE__GENODE__PCI_H_

#include <l4/sys/compiler.h>
#include <l4/sys/types.h>

#include <genode/linkage.h>

enum {
	GENODE_MAX_PCI_BUS = 256,
	GENODE_MAX_PCI_DEV = 32,
};

#ifdef __cplusplus
extern "C" {
#endif

L4_CV unsigned genode_read_pci_config(unsigned char bus, unsigned char slot,
	unsigned char func, unsigned char offset);
L4_CV int genode_pci_read(void *bus, unsigned devfn, unsigned where, unsigned size,
	unsigned *value);
L4_CV int genode_pci_write(void *bus, unsigned devfn, unsigned where, unsigned size,
	unsigned value);
L4_CV void genode_pci_init_l4lx(void **busses);

#ifdef __cplusplus
}
#endif

#endif /* _INCLUDE__GENODE__PCI_H_ */
