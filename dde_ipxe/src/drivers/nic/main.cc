/*
 * \brief  NIC driver based on iPXE
 * \author Christian Helmuth
 * \date   2011-11-17
 */

/*
 * Copyright (C) 2011-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode */
#include <base/env.h>
#include <base/sleep.h>
#include <base/printf.h>
#include <cap_session/connection.h>
#include <nic/component.h>

#include <os/config.h>
#include <util/token.h>
#include <util/string.h>


/* DDE */
extern "C" {
#include <dde_ipxe/nic.h>
}

struct nic_config nic_cfg;

namespace Ipxe {

	class Driver : public Nic::Driver
	{
		public:

			static Driver *instance;

			static void dde_rx_handler(unsigned if_index,
			                           const char *packet,
			                           unsigned packet_len)
			{
				instance->rx_handler(packet, packet_len);
			}

		private:

			Nic::Mac_address      _mac_addr;
			Nic::Rx_buffer_alloc &_alloc;

		public:

			Driver(Nic::Rx_buffer_alloc &alloc)
			: _alloc(alloc)
			{
				PINF("--- init iPXE NIC");
				
				int cnt = dde_ipxe_nic_init();
				PINF("    number of devices: %d", cnt);

				PINF("--- init rx_callbacks");
				dde_ipxe_nic_register_rx_callback(dde_rx_handler);

				PINF("--- get MAC address");
				dde_ipxe_nic_get_mac_addr(1, _mac_addr.addr);
				PINF("    %02x:%02x:%02x:%02x:%02x:%02x",
				     _mac_addr.addr[0], _mac_addr.addr[1], _mac_addr.addr[2],
				     _mac_addr.addr[3], _mac_addr.addr[4], _mac_addr.addr[5]);
			}

			void rx_handler(const char *packet, unsigned packet_len)
			{
// 				PDBG("packet_len = %d", packet_len);
				void *buffer = _alloc.alloc(packet_len);
				Genode::memcpy(buffer, packet, packet_len);
				_alloc.submit();
			}


			/***************************
			 ** Nic::Driver interface **
			 ***************************/

			Nic::Mac_address mac_address() { return _mac_addr; }

			void tx(char const *packet, Genode::size_t size)
			{
// 				PDBG("size = %d", size);
				if (dde_ipxe_nic_tx(1, packet, size))
					PWRN("Sending packet failed!");
			}


			/******************************
			 ** Irq_activation interface **
			 ******************************/

			void handle_irq(int) { 
// 				PDBG("");
				/* not used */ 
			}
	};

	class Driver_factory : public Nic::Driver_factory
	{
		Nic::Driver *create(Nic::Rx_buffer_alloc &alloc)
		{
			Driver::instance = new (Genode::env()->heap()) Ipxe::Driver(alloc);
			return Driver::instance;
		}

		void destroy(Nic::Driver *)
		{
			Genode::destroy(Genode::env()->heap(), Driver::instance);
			Driver::instance = 0;
		}
	};

} /* namespace Ipxe */


Ipxe::Driver * Ipxe::Driver::instance = 0;

inline bool is_hex(char c)
{
	return Genode::digit(c, true) != -1;
}

struct Scanner_policy_number
{
	static bool identifier_char(char c, unsigned  i )
	{
		return is_hex(c) && c !='.' && c !=':';
	}
};
typedef Genode::Token<Scanner_policy_number> TokenNumb;

int main(int, char **)
{
	using namespace Genode;

	printf("--- iPXE NIC driver started ---\n");

	nic_cfg.is_configured = 0;
	try {
		Genode::Xml_node nic_node = Genode::config()->xml_node().sub_node("nic");
		char     pci_dev[16] = {0};
		char     irq[3] = {0};

		do {
			try {
				nic_node.attribute("device").value(pci_dev, sizeof(pci_dev));
			}
			catch(Genode::Xml_node::Nonexistent_attribute)
			{
				PERR("Missing \"device\" attribute. Ignore node.");
				break;
			}

			char tmp[3];
			PDBG("%s",pci_dev);
			
			TokenNumb     t(pci_dev);
			unsigned long val[3] = {0};
			int cnt = 0;
			
			while(t)
			{
				if (t.type() == TokenNumb::WHITESPACE || t[0] == '.' || t[0] == ':') {
					t = t.next();
					continue;
				}
				t.string(tmp, sizeof(tmp));

				unsigned long  tmpc = 0;
				Genode::ascii_to(tmp, &tmpc, 16);

				val[cnt] = tmpc & 0xFF;

				t = t.next();

				if (cnt == 3)
					break;
				cnt++;
			}
			// bus
			nic_cfg.bus = val[0];

			// dev
			nic_cfg.dev = val[1];

			// func
			nic_cfg.func = val[2];
			
			nic_cfg.is_configured = 1;

			PDBG("nic device=%02x:%02x.%x", nic_cfg.bus, nic_cfg.dev, nic_cfg.func);


		} while(0);
	}
	catch (...) {
		PDBG("No Nic config");
	}
	/**
	 * Factory used by 'Nic::Root' at session creation/destruction time
	 */
	static Ipxe::Driver_factory driver_factory;

	enum { STACK_SIZE = 2*sizeof(addr_t)*1024 };
	static Cap_connection cap;
	static Rpc_entrypoint ep(&cap, STACK_SIZE, "nic_ep");

	static Nic::Root nic_root(&ep, env()->heap(), driver_factory);
	env()->parent()->announce(ep.manage(&nic_root));

	sleep_forever();
	return 0;
}

