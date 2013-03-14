/*
 * \brief  Genode's server side of virtual ethernet driver for l4linux
 * \author Ivan Loskutov <ivan.loskutov@ksyslabs.org>
 * \date   2013-02-08
 */

/*
 * Copyright (C) 2012-2013 Ksys Labs LLC
 * Copyright (C) 2009-2013 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <base/allocator_avl.h>
#include <base/env.h>
#include <base/exception.h>
#include <base/printf.h>
#include <util/misc_math.h>
#include <util/string.h>
#include <nic/packet_allocator.h>
#include <nic/component.h>
#include <timer_session/connection.h>
#include <cap_session/connection.h>

#include <nic/driver.h>

#include <vcpu.h>
#include <linux.h>

#include <os/packet_stream.h>
#include <base/allocator_avl.h>

namespace Fiasco {
#include <genode/net.h>
#include <l4/sys/irq.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/ktrace.h>
}

#define TX_BENCH 0
#define RX_BENCH 0


/**
 * Debugging/Tracing
 */
#if TX_BENCH | RX_BENCH
struct Counter : public Genode::Thread<8192>
{
	int             cnt;
	Genode::size_t size;

	void entry()
	{
		Timer::Connection _timer;
		int interval = 5;
		while(1) {
			_timer.msleep(interval * 1000);
			PDBG("LX Packets %d/s bytes/s: %d", cnt / interval, size / interval);
			cnt = 0;
			size = 0;
		}
	}

	void inc(Genode::size_t s) { cnt++; size += s; }

	Counter() : cnt(0), size(0)  { start(); }
};
#else
struct Counter { inline void inc(Genode::size_t s) { } };
#endif

enum { TX_QUEUE_SIZE = 256, RX_QUEUE_SIZE = 256 };

typedef Packet_stream_policy<Packet_descriptor, TX_QUEUE_SIZE, RX_QUEUE_SIZE, char>
        Net_packet_stream_policy;
		
static Genode::Native_capability cap = L4lx::vcpu_connection()->alloc_irq();
enum { TRANSPORT_DS_SIZE = 16*1024 };

static Genode::Dataspace_capability tx_ds_cap = Genode::env()->ram_session()->alloc(TRANSPORT_DS_SIZE);
static Genode::Dataspace_capability rx_ds_cap = Genode::env()->ram_session()->alloc(TRANSPORT_DS_SIZE);

class Sink : public Packet_stream_sink<Net_packet_stream_policy>
{
	public:

		/**
		 * Constructor
		 */
		Sink(Genode::Dataspace_capability ds_cap)
		:
			Packet_stream_sink<Net_packet_stream_policy>(ds_cap)
		{}
		
		bool packet_available() {
			return packet_avail(); }

		Packet_descriptor receive() {
			return get_packet(); }
		
		char* packet_cont(Packet_descriptor &packet) {
			return packet_content(packet); }
		
		void acknowledgement(Packet_descriptor &packet) {
			acknowledge_packet(packet); }
};

class Source : private Genode::Allocator_avl, public  Packet_stream_source<Net_packet_stream_policy>
{
	public:

		/**
		 * Constructor
		 */
		Source(Genode::Dataspace_capability ds_cap)
		:
			/* init bulk buffer allocator, storing its meta data on the heap */
			Genode::Allocator_avl(Genode::env()->heap()),
			Packet_stream_source<Net_packet_stream_policy>(this, ds_cap)
		{}
		
		void generate_packet(const void* data, Genode::size_t size)
		{
			Packet_descriptor packet = alloc_packet(size);

			char *content = packet_content(packet);
			if (!content) {
				PWRN("Source: invalid packet");
			}

			Genode::memcpy(content, data, size);

			submit_packet(packet);
		}
		
		bool acknowledge_avail() {
			return ack_avail(); }

		void acknowledge_packet()
		{
			Packet_descriptor packet = get_acked_packet();
			char *content = packet_content(packet);
			if (!content) {
				PWRN("Source: invalid packet");

			}
			release_packet(packet);
		}
};

static Sink   tx_sink(tx_ds_cap);
static Source tx_source(tx_ds_cap);
static Sink   rx_sink(rx_ds_cap);
static Source rx_source(rx_ds_cap);

static Genode::Signal_receiver rx_receiver;
static Genode::Signal_context  rx_context;
static Genode::Signal_context_capability rx_context_cap = rx_receiver.manage(&rx_context);
static Genode::Signal_transmitter rx_transmitter(rx_context_cap);

namespace {
	class L4lnetsrv : public Nic::Driver
	{
		private:
			
			struct Rx_thread : Genode::Thread<0x2000>
			{
				Nic::Driver &driver;

				Rx_thread(Nic::Driver &driver)
				: Genode::Thread<0x2000>("rx"), driver(driver) { }
				
				Packet_descriptor packet;
				
				void entry()
				{
					while (true) {
						rx_receiver.wait_for_signal();

						/* inform driver about incoming packet */
						driver.handle_irq(0);
					}
				}
			};

			Nic::Rx_buffer_alloc  &_rx_buffer_alloc;
			Nic::Mac_address       _mac_addr;
			
			Fiasco::l4_cap_idx_t    _cap;
			Genode::Lock           *_sync;
			
			Rx_thread               _rx_thread;

		public:

			/**
			* Exception type
			*/
			class Device_not_supported { };

			/**
			* Constructor
			*
			* \throw  Device_not_supported
			*/
			L4lnetsrv(Nic::Rx_buffer_alloc &rx_buffer_alloc, Fiasco::l4_cap_idx_t cap, Genode::Lock *sync)
			:
				_rx_buffer_alloc(rx_buffer_alloc), _cap(cap), _sync(sync),
				_rx_thread(*this)
			{
				_mac_addr.addr[0] = 0x02;
				_mac_addr.addr[1] = 0x01;
				_mac_addr.addr[2] = 0x02;
				_mac_addr.addr[3] = 0x03;
				_mac_addr.addr[4] = 0x04;
				_mac_addr.addr[5] = 0x05;
				
				_rx_thread.start();
			}

			/**
			* Destructor
			*/
			~L4lnetsrv()
			{
				PINF("disable NIC");
			}


			/***************************
			** Nic::Driver interface **
			***************************/
			Nic::Mac_address mac_address()
			{
				return _mac_addr;
			}

			void tx(char const *packet, Genode::size_t size)
			{
				using namespace Fiasco;

				try {
					tx_source.generate_packet(packet, size);
					
					if (l4_error(l4_irq_trigger(_cap)) != -1)
						PWRN("IRQ net trigger failed\n");
					
					tx_source.acknowledge_packet();
					
				} catch (Packet_stream_source<>::Packet_alloc_failed) {
					PWRN("tx: Packet allocation failed");
				}
			}


			/******************************
			** Irq_activation interface **
			******************************/
			void handle_irq(int)
			{
				while(rx_sink.packet_available()) {
					Packet_descriptor packet = rx_sink.receive();

					char *content = rx_sink.packet_cont(packet);
					if (!content)
					{
						PWRN("Invalid packet");
						return;
					}
					// read data
					void *buffer = _rx_buffer_alloc.alloc(packet.size());
					Genode::memcpy(buffer, content, packet.size());
					_rx_buffer_alloc.submit();
					
					rx_sink.acknowledgement(packet);
				}
			}
	};
	
	struct L4lnetsrv_driver_factory : Nic::Driver_factory
	{
		Fiasco::l4_cap_idx_t    _cap;
		Genode::Lock           *_sync;
		
		void init(Fiasco::l4_cap_idx_t cap, Genode::Lock *sync)
		{
			_cap = cap;
			_sync = sync;
		}
			
		Nic::Driver *create(Nic::Rx_buffer_alloc &alloc)
		{
			return new (Genode::env()->heap()) L4lnetsrv(alloc, _cap, _sync);
		}

		void destroy(Nic::Driver *driver)
		{
			Genode::destroy(Genode::env()->heap(), static_cast<L4lnetsrv *>(driver));
		}

	} driver_factory;

	class Server_thread : public Genode::Thread<8192>
	{
		protected:

			void entry()
			{
				using namespace Genode;

				enum { STACK_SIZE = 4096 };
				static Cap_connection cap;
				static Rpc_entrypoint ep(&cap, STACK_SIZE, "nic2_ep");

				static Nic::Root nic_root(&ep, env()->heap(), driver_factory);
				env()->parent()->announce(ep.manage(&nic_root));
				
				driver_factory._sync->unlock();
				
				sleep_forever();
			}
		
		public:

			Server_thread(Fiasco::l4_cap_idx_t cap, Genode::Lock *sync)
			: Genode::Thread<8192>("netsrv-server-thread")
			{
				driver_factory.init(cap, sync);
				start();
			}
	};
}

extern "C" {

	static FASTCALL void (*receive_packet)(void*, void*, unsigned long) = 0;
	static void *net_device                                             = 0;


	void genode_netsrv_start(void *dev, FASTCALL void (*func)(void*, void*, unsigned long))
	{
		receive_packet = func;
		net_device     = dev;
	}


	Fiasco::l4_cap_idx_t genode_netsrv_irq_cap()
	{
		Linux::Irq_guard guard;
		return cap.dst();
	}


	void genode_netsrv_stop()
	{
		net_device     = 0;
		receive_packet = 0;
	}


	void genode_netsrv_mac(void* mac, unsigned long size)
	{
		Linux::Irq_guard guard;
		using namespace Genode;
		
		Nic::Mac_address m;
		m.addr[0] = 0x02;
		m.addr[1] = 0x01;
		m.addr[2] = 0x02;
		m.addr[3] = 0x03;
		m.addr[4] = 0x04;
		m.addr[5] = 0x04;
		memcpy(mac, &m.addr, min(sizeof(m.addr), (size_t)size));
	}


	int genode_netsrv_tx(void* addr, unsigned long len)
	{
		Linux::Irq_guard guard;
		static Counter counter;
		
		try {
			rx_source.generate_packet(addr, len);
			rx_transmitter.submit();
					
			counter.inc(len);
			
		} catch(...) {
			/* Packet_alloc_failed' */
			return 1;
		}
		return 0;
	}


	int genode_netsrv_tx_ack_avail() {
		Linux::Irq_guard guard;
		return rx_source.acknowledge_avail();
	}


	void genode_netsrv_tx_ack()
	{
		Linux::Irq_guard guard;
		rx_source.acknowledge_packet();
	}


	void genode_netsrv_rx_receive()
	{
		Linux::Irq_guard guard;
		static Counter counter;
		
		while (tx_sink.packet_available())
		{
			Packet_descriptor packet = tx_sink.receive();
			
			char *content = tx_sink.packet_cont(packet);
			if (!content)
				PWRN("Invalid packet");

			if (receive_packet && net_device)
				receive_packet(net_device, content, packet.size());
			
			counter.inc(packet.size());
			
			tx_sink.acknowledgement(packet);
		}
	}


	int genode_netsrv_ready()
	{
		static bool initialized = false;
		
		if (!initialized)
		{
			Linux::Irq_guard guard;
			static Genode::Lock lock(Genode::Lock::LOCKED);
			
			tx_source.register_sigh_packet_avail(tx_sink.sigh_packet_avail());
			tx_source.register_sigh_ready_to_ack(tx_sink.sigh_ready_to_ack());
			tx_sink.register_sigh_ready_to_submit(tx_source.sigh_ready_to_submit());
			tx_sink.register_sigh_ack_avail(tx_source.sigh_ack_avail());
			
			rx_source.register_sigh_packet_avail(rx_sink.sigh_packet_avail());
			rx_source.register_sigh_ready_to_ack(rx_sink.sigh_ready_to_ack());
			rx_sink.register_sigh_ready_to_submit(rx_source.sigh_ready_to_submit());
			rx_sink.register_sigh_ack_avail(rx_source.sigh_ack_avail());
			
			static Server_thread th(cap.dst(), &lock);
			lock.lock();
		}
		return 1;
	}


	void *genode_netsrv_memcpy(void *dst, void const *src, unsigned long size) {
		return Genode::memcpy(dst, src, size); }
}
