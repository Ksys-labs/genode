/*
 * \brief  Genode C API framebuffer functions of the L4Linux support library
 * \author Stefan Kalkowski
 * \date   2009-06-08
 */

/*
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
#include <nic_session/connection.h>
#include <timer_session/connection.h>

#include <vcpu.h>
#include <linux.h>

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


namespace {
	
	class Nic_device
	{
		private:
			enum {
				PACKET_SIZE = Nic::Packet_allocator::DEFAULT_PACKET_SIZE,
				RX_BUF_SIZE = Nic::Session::RX_QUEUE_SIZE * PACKET_SIZE,
				TX_BUF_SIZE = Nic::Session::TX_QUEUE_SIZE * PACKET_SIZE,
			};
	
			Nic::Packet_allocator      _tx_block_alloc;
			Nic::Connection            _session;
			Genode::Native_capability  _irq_cap;
			Genode::Signal_context     _rx;

			char                       _name[16];
			
		public:
			Nic_device(const char *name)
			: _tx_block_alloc(Genode::env()->heap()),
			  _session(&_tx_block_alloc, TX_BUF_SIZE, RX_BUF_SIZE, name),
			  _irq_cap(L4lx::vcpu_connection()->alloc_irq())
			{
				Genode::strncpy(_name, name, sizeof(_name));
			}
			
			Nic::Connection         *session() { return &_session;       }
			Fiasco::l4_cap_idx_t    irq_cap()  { return  _irq_cap.dst(); }
			const char             *name()     { return  _name;          }
			Genode::Signal_context *context()  { return &_rx;            }
	};

	class Signal_thread : public Genode::Thread<8192>
	{
		private:
			Nic_device         **_devs;
			int                  _count;
			Genode::Lock        *_sync;

		protected:
			void entry()
			{
				using namespace Fiasco;
				using namespace Genode;

				Signal_receiver receiver;
				for (int i = 0; i < _count; i++) {
					Signal_context_capability cap(receiver.manage(_devs[i]->context()));
					_devs[i]->session()->rx_channel()->sigh_ready_to_ack(cap);
					_devs[i]->session()->rx_channel()->sigh_packet_avail(cap);
				}
				_sync->unlock();

				while (true) {
					Signal s = receiver.wait_for_signal();
					for (int i = 0; i < _count; i++) {
						if (_devs[i]->context() == s.context()) {
							if (l4_error(l4_irq_trigger(_devs[i]->irq_cap())) != -1)
								PWRN("IRQ net trigger failed\n");
						}
					}
				}
			}

		public:

			Signal_thread(Nic_device **devs, int count, Genode::Lock *sync)
			: Genode::Thread<8192>("net-signal-thread"), _devs(devs), _count(count), _sync(sync)
			{
				start();
			}
	};
}

static Nic_device *devices[MAX_GENODE_NET] = {0};
static bool initialized[MAX_GENODE_NET]   = {false};

static Nic_device *nic(int num) {

	Linux::Irq_guard guard;

	if (!initialized[num]) {
		char if_name[16];
		Genode::snprintf(if_name, 16, "eth%d", num);
		
		PDBG("initialize NIC %s", if_name);

		try {
			devices[num] = new (Genode::env()->heap()) Nic_device(if_name);
			initialized[num] = true;
		} catch(...) { }
	}
	return devices[num];
}


using namespace Fiasco;

extern "C" {

	typedef FASTCALL void (receive_func)(void*, void*, unsigned long);
	
	static receive_func *receive_packet[MAX_GENODE_NET]  = {0};
	static void         *net_device[MAX_GENODE_NET]      = {0};
	
	void genode_net_start(int num, void *dev, receive_func *func)
	{
		receive_packet[num] = func;
		net_device[num]     = dev;
	}

	void genode_net_stop(int num)
	{
		receive_packet[num] = 0;
		net_device[num]     = 0;
	}

	l4_cap_idx_t genode_net_irq_cap(int num)
	{
		Linux::Irq_guard guard;
		return nic(num)->irq_cap();
	}

	void genode_net_mac(int num, void* mac, unsigned long size)
	{
		Linux::Irq_guard guard;
		using namespace Genode;

		Nic::Mac_address m = nic(num)->session()->mac_address();
		memcpy(mac, &m.addr, min(sizeof(m.addr), (size_t)size));
	}

	int genode_net_tx(int num, void* addr, unsigned long len)
	{
		Linux::Irq_guard guard;
		static Counter counter[MAX_GENODE_NET];

		try {
			Packet_descriptor packet = nic(num)->session()->tx()->alloc_packet(len);
			void* content            = nic(num)->session()->tx()->packet_content(packet);

			Genode::memcpy((char *)content, addr, len);
			nic(num)->session()->tx()->submit_packet(packet);

			counter[num].inc(len);

			return 0;
		/* 'Packet_alloc_failed' */
		} catch(...) {
			return 1;
		}
	}


	int genode_net_tx_ack_avail(int num) {
		return nic(num)->session()->tx()->ack_avail(); }


	void genode_net_tx_ack(int num)
	{
		Linux::Irq_guard guard;

		Packet_descriptor packet = nic(num)->session()->tx()->get_acked_packet();
		nic(num)->session()->tx()->release_packet(packet);
	}


	void genode_net_rx_receive(int num)
	{
		Linux::Irq_guard guard;
		static Counter counter[MAX_GENODE_NET];

		if (nic(num)) {
			while(nic(num)->session()->rx()->packet_avail()) {
				Packet_descriptor p = nic(num)->session()->rx()->get_packet();

				if (receive_packet[num] && net_device[num])
					receive_packet[num](net_device[num], nic(num)->session()->rx()->packet_content(p), p.size());
				
				counter[num].inc(p.size());
				nic(num)->session()->rx()->acknowledge_packet(p);
			}
		}
	}


	int genode_net_ready(int num)
	{
		return nic(num) ? 1 : 0;
	}
	
	void genode_net_run(int count)
	{
		Linux::Irq_guard guard;
		static Genode::Lock lock(Genode::Lock::LOCKED);
		static Signal_thread th(devices, count, &lock);
		lock.lock();
	}


	void *genode_net_memcpy(void *dst, void const *src, unsigned long size) {
		return Genode::memcpy(dst, src, size); }
		
	int genode_net_strcmp(const char *s1, const char *s2, unsigned long size) {
		return Genode::strcmp(s1, s2, size); }
}
