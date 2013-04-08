#include <base/env.h>
#include <base/printf.h>
#include <base/thread.h>

#include <irq_session/connection.h>
#include <pci_session/connection.h>
#include <pci_device/client.h>

using namespace Genode;

#define IRQ_LOG(LVL) P ## LVL("< IRQ %d", _irq_no)

class Irq_watcher : public Genode::Thread<4096UL>
{
private:
	int _irq_no;
	Irq_connection _irq;

	void entry(void) {
		bool b = false;
		for (;;) {
			PDBG("> wait_for_irq %d", _irq_no);
			_irq.wait_for_irq();
			if (b) {
				IRQ_LOG(INF);
			}
			else {
				IRQ_LOG(ERR);
			}

			b = !b;
		}
	}
public:
	Irq_watcher(int irq) : _irq_no(irq),
		_irq(Irq_connection(irq, Irq_session::TRIGGER_EDGE,
			Irq_session::POLARITY_HIGH))
	{
		start();
	}
};

#define MAX_IRQ 64
static Irq_watcher *watchers[MAX_IRQ] = {};

using namespace Genode;

enum Pci_config {
	IRQ = 0x3c,
	REV = 0x8,
	CMD = 0x4,
};

enum Pci_class {
	PCI_USB_MASK = 0xc03FF,
	PCI_USB = 0xc0300,
	PCI_UHCI = 0xc0300,
	PCI_OHCI = 0xc0310,
	PCI_EHCI = 0xc0320,
	PCI_XHCI = 0xc0330
};

enum HCI_Regs {
	UHCI_LEGSUP = 0xc0,
	UHCI_INTR = 0x4,

	OHCI_CONTROL = 0x4,
	OHCI_INTERRUPT_ENABLE = 0x10,

	EHCI_CAP = 0,
	EHCI_CAP_CMD = 0,
	EHCI_CAP_INTR = 8,
};

enum HCI_Bits {
	EHCI_CMD_RUN = 1,
};

int main() {

	Pci::Connection pci;
	Pci::Device_capability cap = pci.first_device();
	while (cap.valid()) {
		uint8_t bus, dev, func;
		Pci::Device_client device(cap);
		device.bus_address(&bus, &dev, &func);
		PDBG("bus: %x dev: %x func: %x", bus, dev, func);

		uint8_t irq = device.config_read(IRQ, Pci::Device::ACCESS_8BIT);
		PDBG("irq %d", irq);

		uint16_t cmd = device.config_read(CMD, Pci::Device::ACCESS_16BIT);
		cmd |= 4;
		device.config_write(CMD, cmd, Pci::Device::ACCESS_16BIT);

		
		uint16_t vid, pid;
		vid = device.vendor_id();
		pid = device.device_id();
		PDBG("VID:PID %x:%x", vid, pid);

		uint32_t pclass = device.class_code();
		PDBG("PCI class %x", pclass);
		uint32_t uclass = pclass & PCI_USB_MASK;

		uint32_t ecap = 0;
		uint32_t octl;

		if ((uclass & PCI_USB) == PCI_USB) {
			PINF("USB");
			switch (uclass) {
				case PCI_UHCI:
					PINF("UHCI");
					device.config_write(UHCI_LEGSUP,
						0x8000, Pci::Device::ACCESS_16BIT);

					device.config_write(UHCI_INTR,
						0xf, Pci::Device::ACCESS_16BIT);

					break;
				case PCI_OHCI:
					PINF("OHCI");
					octl = device.config_read(OHCI_CONTROL,
						Pci::Device::ACCESS_32BIT);

					octl &= ~0xc0;
					octl |= 0x3c;
					octl |= 0x3;
					octl |= 0x80;
					device.config_write(OHCI_CONTROL,
						octl, Pci::Device::ACCESS_32BIT);

					device.config_write(OHCI_INTERRUPT_ENABLE,
						0xffffffff, Pci::Device::ACCESS_32BIT);
					break;
				case PCI_EHCI:
					PINF("EHCI");
					ecap = device.config_read(EHCI_CAP,
						Pci::Device::ACCESS_32BIT);
					device.config_write(ecap + EHCI_CAP_INTR,
						0x1f, Pci::Device::ACCESS_32BIT);
					device.config_write(ecap + EHCI_CAP_CMD,
						EHCI_CMD_RUN, Pci::Device::ACCESS_32BIT);
					break;
				case PCI_XHCI:
					PINF("XHCI");
					break;
			}

			if (irq < MAX_IRQ && !watchers[irq]) {
				watchers[irq] = new (env()->heap()) Irq_watcher(irq);
			}
		}

		PDBG("===================");
		PDBG("");
		cap = pci.next_device(cap);
	}

	for (;;);

	return 0;
}
