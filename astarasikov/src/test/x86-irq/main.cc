#include <irq_session/connection.h>
#include <base/printf.h>

#include <pci_session/connection.h>
#include <pci_device/client.h>

#include <io_port_session/connection.h>

using namespace Genode;

//static int N_IRQ = 5;
static int N_IRQ = 11;

enum Pci_config { IRQ = 0x3c, REV = 0x8, CMD = 0x4 };

static void _disable_msi(::Pci::Device_client * pci)
{
	enum { PM_CAP_OFF = 0x34, MSI_CAP = 0x5, MSI_ENABLED = 0x1 };
	uint8_t cap = pci->config_read(PM_CAP_OFF,::Pci::Device::ACCESS_8BIT);

	/* iterate through cap pointers */
	for (uint16_t val = 0; cap; cap = val >> 8) {
		val = pci->config_read(cap,::Pci::Device::ACCESS_16BIT);

		if ((val & 0xff) != MSI_CAP)
			continue;
		uint16_t msi =
		    pci->config_read(cap + 2,::Pci::Device::ACCESS_16BIT);

		if (msi & MSI_ENABLED) {
			pci->config_write(cap + 2,
					  msi ^ MSI_CAP,::Pci::Device::
					  ACCESS_8BIT);
			PINF("Disabled MSIs %x", msi);
		}
	}
}

#define PCI_CLASS_USB_MASK 0xc03FF
#define PCI_CLASS_USB_UHCI 0xc0300
#define PCI_CLASS_USB_OHCI 0xc0310
#define PCI_CLASS_USB_EHCI 0xc0320

enum {
	USBINTR = 4,
	USBLEGSUP = 0xc0,
	USBLEGSUP_DEFAULT = 0x2000,
	USBRES_INTEL = 0xc4,
};

int main()
{
	static Pci::Connection pci;
	using namespace Pci; 

	try {
		Io_port_connection io(0x64, 2);
		io.outb(0, 0xff);
	}
	catch (...) {
		PDBG("failed to do IO");
	}

	Pci::Device_capability cap = pci.first_device();
	while (cap.valid()) {
		Pci::Device_client device(cap);

		unsigned char _dev, _bus, _fn;
		device.bus_address(&_bus, &_dev, &_fn);
		PDBG("%s: found bus=%x dev=%x fn=%x vid=%x pid=%x",
			__func__, _bus, _dev, _fn,
			device.vendor_id(), device.device_id());

		/* enable bus master bits */
		uint16_t cmd = device.config_read(CMD, Device::ACCESS_16BIT);
		cmd |= 0x4;
		device.config_write(CMD, cmd, Device::ACCESS_16BIT);

		pci.config_extended(cap);

		_disable_msi(&device);

		//unsigned long intr = device.config_read(INTR_OFF
		uint32_t intr = device.config_read(0x3c, Device::ACCESS_32BIT);
		PDBG("Interrupt pin: %lu line: %lu", (intr >> 8) & 0xff, intr & 0xff);
		//device.config_write(0x3c, (intr & 0xffff00ff) | (11 << 8),
		//	Device::ACCESS_32BIT);

		uint32_t pclass = device.class_code();
		PDBG("class %x", pclass);

#if 0
		if ((pclass & PCI_CLASS_USB_MASK) == PCI_CLASS_USB_UHCI) {
			N_IRQ = intr & 0xff;
			PDBG("setting usb irq to %d", N_IRQ);

			device.config_write(USBLEGSUP, 0x8f00,
				Device::ACCESS_16BIT);
			
			device.config_write(USBLEGSUP, 0x2000,
				Device::ACCESS_16BIT);

			device.config_write(USBRES_INTEL, 0, Device::ACCESS_16BIT);
		}
#endif
#if 0
		if ((pclass & PCI_CLASS_USB_MASK) == PCI_CLASS_USB_EHCI) {
			N_IRQ = intr & 0xff;
			PDBG("setting EHCI usb irq to %d", N_IRQ);

			uint32_t ecap = device.config_read(0, Device::ACCESS_32BIT);

			uint32_t intr = device.config_read(ecap + 8, Device::ACCESS_32BIT);
			intr |= 0x1f;
			device.config_write(ecap + 8, intr, Device::ACCESS_32BIT);
			
			uint32_t cmd = device.config_read(ecap, Device::ACCESS_32BIT);
			cmd |= 1;
			device.config_write(ecap, cmd, Device::ACCESS_32BIT);
		}
#endif
#if 1
		if ((pclass & PCI_CLASS_USB_MASK) == PCI_CLASS_USB_OHCI) {
			N_IRQ = intr & 0xff;
			PDBG("setting OHCI usb irq to %d", N_IRQ);
			
			uint32_t revision = device.config_read(0, Device::ACCESS_32BIT);

			uint32_t intrena = device.config_read(4 * 5, Device::ACCESS_32BIT);
			PDBG("revision %x intrena %x", revision, intrena);
			//device.config_write(4 * 5, 0xffffffff, Device::ACCESS_32BIT);
		}
#endif
		
		cap = pci.next_device(cap);
	}

	static Irq_connection irq(N_IRQ);
	PINF("wait_for_irq %d", N_IRQ);
	int i = 0;
	while (1) {
		i++;
		irq.wait_for_irq();
		if (i & 1) {
			PINF("IRQ %d", N_IRQ);
		}
		else {
			PERR("IRQ %d", N_IRQ);
		}
	}

	return 0;
}

