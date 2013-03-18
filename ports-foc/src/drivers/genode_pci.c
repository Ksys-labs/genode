/*
 * \brief  NIC driver to access Genode's nic service
 * \author Stefan Kalkowski <stefan.kalkowski@genode-labs.com>
 * \date   2010-09-09
 */

/*
 * Copyright (C) 2006-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#include <linux/pci.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/pci.h>

#include <genode/pci.h>

/******************************************************************************
 * stubs to make it link
 *****************************************************************************/
#ifdef CONFIG_X86
unsigned int pci_probe;
unsigned int pci_early_dump_regs;
int noioapicquirk;
int noioapicreroute = 0;
int pci_routeirq;
#endif

char * __devinit pcibios_setup(char *str) { return str; }
int early_pci_allowed(void) { return 0; }
int __init pci_legacy_init(void) { return 0; }
void __devinit pcibios_fixup_bus(struct pci_bus *b) {}
void __init pcibios_fixup_irqs(void) {}
void __init pcibios_irq_init(void) {}
void pcibios_disable_device(struct pci_dev *dev) {}
void early_dump_pci_devices(void) {}
unsigned int pcibios_assign_all_busses(void) { return 1; }

/******************************************************************************
 * the actual PCI code
 *****************************************************************************/
int pcibios_enable_device(struct pci_dev *dev, int mask)
{
	int err;

	printk("%s: dev=%x\n", __func__, dev);

	if ((err = pci_enable_resources(dev, mask)) < 0) {
		printk("%s: failed to enable resources\n");
		return err;
	}

	//XXX: enable IRQ
	return 0;
}

resource_size_t pcibios_align_resource(void *data, const struct resource * res,
				       resource_size_t size,
				       resource_size_t align)
{
	resource_size_t start = res->start;
	
	printk("%s: data=%x\n", __func__, data);

	if (res->flags & IORESOURCE_IO && start & 0x300)
		start = (start + 0x3ff) & ~0x3ff;

	start = (start + align - 1) & ~(align - 1);

	return start;
}

int pci_mmap_page_range(struct pci_dev *dev, struct vm_area_struct *vma,
			enum pci_mmap_state mmap_state, int write_combine)
{

	printk("%s: dev=%x", __func__, dev);

	return -EINVAL;
}

u32 read_pci_config(u8 bus, u8 slot, u8 func, u8 offset)
{
	printk("%s: bus=%02x slot=%02x func=%02x offset=%02x\n",
		__func__, bus, slot, func, offset);
	return genode_read_pci_config(bus, slot, func, offset);
}

u8 read_pci_config_byte(u8 bus, u8 slot, u8 func, u8 offset)
{
	printk("%s: bus=%02x slot=%02x func=%02x offset=%02x\n",
		__func__, bus, slot, func, offset);
	return 0;
}

u16 read_pci_config_16(u8 bus, u8 slot, u8 func, u8 offset)
{
	printk("%s: bus=%02x slot=%02x func=%02x offset=%02x\n",
		__func__, bus, slot, func, offset);
	return 0;
}

void write_pci_config(u8 bus, u8 slot, u8 func, u8 offset, u32 val)
{
	printk("%s: bus=%02x slot=%02x func=%02x offset=%02x val=%04x\n",
		__func__, bus, slot, func, offset, val);
}

static int pci_read(struct pci_bus *bus, unsigned int devfn, int where,
	int size, u32 *value)
{
	if (!bus) {
		printk("%s: bus is NULL\n", __func__);
		return -1;
	}

	if (!value) {
		printk("%s: value is NULL\n", __func__);
		return -1;
	}

	return genode_pci_read(bus->sysdata, devfn, where, size, value);
}

static int pci_write(struct pci_bus *bus, unsigned int devfn, int where,
	int size, u32 value)
{
	if (!bus) {
		printk("%s: bus is NULL\n", __func__);
		return -1;
	}

	return genode_pci_write(bus->sysdata, devfn, where, size, value);
}

static struct pci_ops genode_pci_ops = {
	.read = pci_read,
	.write = pci_write,
};

static int __init genode_pci_init(void) {
	printk(KERN_INFO "genode_pci: init\n");

	void* busses[GENODE_MAX_PCI_BUS] = {};
	memset(busses, 0, sizeof(busses));
	genode_pci_init_l4lx(busses);
	
	size_t i;
	for (i = 0; i < GENODE_MAX_PCI_BUS; i++) {
		if (busses[i]) {
			pci_scan_bus(i, &genode_pci_ops, busses[i]);
		}
	}

	return 0;
}

subsys_initcall(genode_pci_init);
