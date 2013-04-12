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

#include <base/allocator_guard.h>

#include <os/ring_buffer.h>

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
		int interval = 1;
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

//#define PERF_COUNT

#ifdef PERF_COUNT
struct PerfCounter : public Genode::Thread<8192>
{
	enum { 
		BUFFER_SIZE = 2000,
		MSG_LEN = 256,
		ST_ENTER = 0,
		ST_EXIT,
		PNUM = 0,
	};
	
	struct Msg {
		unsigned long    time;
		char             name[MSG_LEN];
		int              state;
		Msg(Genode::uint64_t t, const char *n, int st) : time(t), state(st) {
			Genode::strncpy(name, n, MSG_LEN-1);
		}
		Msg() {}
	};
	
	Ring_buffer<Msg, BUFFER_SIZE> msg_buf;

	void entry()
	{
		Timer::Connection _timer;
		int interval = 1;
		while(1) {
			_timer.msleep(interval * 1000);
			while ( !msg_buf.empty() ) {
				Msg m = msg_buf.get();
				Genode::printf("trace: %s %s %lu\n", m.state ? "EXIT" : "ENTER", m.name, m.time);
			}
		}
	}
	
	inline unsigned long timer()
	{
		static Timer::Connection _timer;
		return _timer.elapsed_ms();
	}

	void enter(const char *name) {
		try {
			msg_buf.add(Msg(timer(), name, ST_ENTER));
		} catch(...) {
			PWRN("msg buf full. skipped");
		}
	}
	
	void exit(const char *name) { 
		try {
			msg_buf.add(Msg(timer(), name, ST_EXIT));
		} catch(...) {
			PWRN("msg buf full. skipped");
		}
	}
	

	PerfCounter() { start(); }
};
#else
struct PerfCounter {
	inline void enter(const char *name) { }
	inline void exit(const char *name)  { }
};
#endif

static Genode::Native_capability cap = L4lx::vcpu_connection()->alloc_irq();

static PerfCounter perf;

#define PERF_ENTER   perf.enter(__PRETTY_FUNCTION__)
#define PERF_EXIT   perf.exit(__PRETTY_FUNCTION__)

namespace {
	class Rx_handler;
	Rx_handler *rx_handler = 0;
	
	struct RxBuf {
		void *buf;
		Genode::size_t size;
	} rx_buf;

	Genode::Lock          _rx_sync;
	
	class Guarded_range_allocator
	{
		private:

			Genode::Allocator_guard _guarded_alloc;
			Nic::Packet_allocator   _range_alloc;

		public:

			Guarded_range_allocator(Genode::Allocator *backing_store,
			                        Genode::size_t     amount)
			: _guarded_alloc(backing_store, amount),
			  _range_alloc(&_guarded_alloc) {}

			Genode::Allocator_guard *guarded_allocator() {
				return &_guarded_alloc; }

			Genode::Range_allocator *range_allocator() {
				return static_cast<Genode::Range_allocator *>(&_range_alloc); }
	};
	
	class Communication_buffer : Genode::Ram_dataspace_capability
	{
		public:

			Communication_buffer(Genode::size_t size)
			: Genode::Ram_dataspace_capability(Genode::env()->ram_session()->alloc(size))
			{ }

			~Communication_buffer() { Genode::env()->ram_session()->free(*this); }

			Genode::Dataspace_capability dataspace() { return *this; }
	};
	
	class Tx_rx_communication_buffers
	{
		private:

			Communication_buffer _tx_buf, _rx_buf;

		public:

			Tx_rx_communication_buffers(Genode::size_t tx_size,
			                            Genode::size_t rx_size)
			: _tx_buf(tx_size), _rx_buf(rx_size) { }

			Genode::Dataspace_capability tx_ds() { return _tx_buf.dataspace(); }
			Genode::Dataspace_capability rx_ds() { return _rx_buf.dataspace(); }
	};
	
	class Packet_handler : public Genode::Thread<8192>
	{
		private:
			Genode::Semaphore _startup_sem;       /* thread startup sync */

		protected:

			virtual void acknowledge_last_one()            = 0;

			virtual void next_packet(void** src,
			                         Genode::size_t *size) = 0;
									 
			virtual void finalize_packet(void *eth,
			                             Genode::size_t size) {}

		public:
			Packet_handler() {}

			/*
			 * Thread's entry code.
			 */
			void entry()
			{
				void*          src;
				Genode::size_t size;
// 				PDBG("entry");
				_startup_sem.up();
				while (true) {
					acknowledge_last_one();
					next_packet(&src, &size);
					finalize_packet(src, size);
				}
			}

			/*
			 * Block until thread is ready to execute.
			 */
			void wait_for_startup() { _startup_sem.down(); }
	};
	
	class Session_component;
	class Rx_handler
	{
		private:
			Session_component *_component;

		public:

			Rx_handler(Session_component *component)
			:
				_component(component)
			{
// 				PDBG("Rx_handler");
			}

			void send_packet(void *src, Genode::size_t size);
	};
	
	class Session_component : public Guarded_range_allocator, private Tx_rx_communication_buffers,
	                          public Nic::Session_rpc_object
	{
		private:

			class Tx_handler : public Packet_handler
			{
				private:
					Packet_descriptor  _tx_packet;
					Session_component *_component;
					Fiasco::l4_cap_idx_t   _cap;


				public:

					Tx_handler(Session_component *component, Fiasco::l4_cap_idx_t cap)
					:
					    Packet_handler(),
					    _component(component),
						_cap(cap)
					{
// 						PDBG("Tx_handler");
					}
					
					void acknowledge_last_one() {
						PERF_ENTER;
						if (!_tx_packet.valid())
							return;
						
						if (!_component->tx_sink()->ready_to_ack())
							PDBG("need to wait until ready-for-ack");
						_component->tx_sink()->acknowledge_packet(_tx_packet);
						PERF_EXIT;
					}

					void next_packet(void** src, Genode::size_t *size) {
// 						PDBG("");
						while (true) {
							/* block for a new packet */
							_tx_packet = _component->tx_sink()->get_packet();
							if (!_tx_packet.valid()) {
								PWRN("received invalid packet");
								continue;
							}

							*src  = _component->tx_sink()->packet_content(_tx_packet);
							*size = _tx_packet.size();
							return;
						}
					}
					
					void finalize_packet(void *src, Genode::size_t size)
					{
// 						PDBG("packet from NIC to l4linux");
						using namespace Fiasco;
						PERF_ENTER;
						rx_buf.buf = src;
						rx_buf.size = size;
						
						if (l4_error(l4_irq_trigger(_cap)) != -1)
							PWRN("IRQ net trigger failed\n");
						
						_rx_sync.lock();
						
						rx_buf.buf = 0;
						rx_buf.size = 0;
						PERF_EXIT;
					}
			};
			
			Tx_handler             _tx_handler;
			
			Nic::Mac_address       _mac_addr;

		public:

			/**
			 * Constructor
			 *
			 * \param tx_buf_size        buffer size for tx channel
			 * \param rx_buf_size        buffer size for rx channel
			 * \param rx_block_alloc     rx block allocator
			 * \param ep                 entry point used for packet stream
			 */
			Session_component(Genode::Allocator      *allocator,
			                  Genode::size_t          amount,
			                  Genode::size_t          tx_buf_size,
			                  Genode::size_t          rx_buf_size,
			                  Genode::Rpc_entrypoint &ep,
							  Fiasco::l4_cap_idx_t    cap,
					          Rx_handler            **rx_handler
 							)
			:
				Guarded_range_allocator(allocator, amount),
				Tx_rx_communication_buffers(tx_buf_size, rx_buf_size),
				Session_rpc_object(Tx_rx_communication_buffers::tx_ds(),
				                   Tx_rx_communication_buffers::rx_ds(),
				                   this->range_allocator(), ep),
				_tx_handler(this, cap)
			{
// 				PDBG("enter");
				_mac_addr.addr[0] = 0x02;
				_mac_addr.addr[1] = 0x01;
				_mac_addr.addr[2] = 0x02;
				_mac_addr.addr[3] = 0x03;
				_mac_addr.addr[4] = 0x04;
				_mac_addr.addr[5] = 0x05;
				
				_tx_handler.start();
				_tx_handler.wait_for_startup();
				
				*rx_handler = new (Genode::env()->heap()) Rx_handler(this);
				
// 				PDBG("exit");
			}

			/**
			 * Destructor
			 */
			~Session_component()
			{
			}
			
			Nic::Session::Tx::Sink*   tx_sink()   { return _tx.sink();   }
			Nic::Session::Rx::Source* rx_source() { return _rx.source(); }
			Nic::Mac_address          mac_address()  { return _mac_addr; }
	};

	/**
	 * Shortcut for single-client root component
	 */
	typedef Genode::Root_component<Session_component, Genode::Single_client> Root_component;

	/*
	 * Root component, handling new session requests.
	 */
	class Root : public Root_component
	{
		private:

			Genode::Rpc_entrypoint &_ep;
			
			Fiasco::l4_cap_idx_t   _cap;
			Genode::Lock          *_sync;
			
			Rx_handler             **_rx_handler;

		protected:

			/*
			 * Always returns the singleton nic-session component.
			 */
			Session_component *_create_session(const char *args)
			{
				using namespace Genode;

				Genode::size_t ram_quota =
					Arg_string::find_arg(args, "ram_quota"  ).ulong_value(0);
				Genode::size_t tx_buf_size =
					Arg_string::find_arg(args, "tx_buf_size").ulong_value(0);
				Genode::size_t rx_buf_size =
					Arg_string::find_arg(args, "rx_buf_size").ulong_value(0);

				/* delete ram quota by the memory needed for the session */
				Genode::size_t session_size = max((Genode::size_t)4096, sizeof(Session_component)
				                                  + sizeof(Allocator_avl));
				if (ram_quota < session_size)
					throw Root::Quota_exceeded();

				/*
				 * Check if donated ram quota suffices for both
				 * communication buffers. Also check both sizes separately
				 * to handle a possible overflow of the sum of both sizes.
				 */
				if (tx_buf_size                  > ram_quota - session_size
					|| rx_buf_size               > ram_quota - session_size
					|| tx_buf_size + rx_buf_size > ram_quota - session_size) {
					PERR("insufficient 'ram_quota', got %zd, need %zd",
					     ram_quota, tx_buf_size + rx_buf_size + session_size);
					throw Root::Quota_exceeded();
				}

				return new (md_alloc()) Session_component(env()->heap(),
														  ram_quota - session_size,
															tx_buf_size,
				                                          rx_buf_size,
				                                          _ep,
														  _cap,
														_rx_handler
 														);
			}

		public:

			Root(Genode::Rpc_entrypoint *session_ep,
			     Genode::Allocator      *md_alloc,
			     Fiasco::l4_cap_idx_t    cap,
				 Rx_handler             **rx_handler
				)
			:
				Root_component(session_ep, md_alloc),
				_ep(*session_ep),
				_cap(cap),
				_rx_handler(rx_handler)
			{ }
	};
	
	class Server_thread : public Genode::Thread<8192>
	{
		private:
			Fiasco::l4_cap_idx_t  _cap;
			Genode::Lock         *_sync;
			
		protected:

			void entry()
			{
				using namespace Genode;

				enum { STACK_SIZE = 4096 };
				static Cap_connection cap;
				static Rpc_entrypoint ep(&cap, STACK_SIZE, "nic_srv_ep");
				
				static Root nic_root(&ep, env()->heap(), _cap, &rx_handler);
				env()->parent()->announce(ep.manage(&nic_root));
				
				_sync->unlock();
				
				sleep_forever();
			}
		
		public:

			Server_thread(Fiasco::l4_cap_idx_t cap, Genode::Lock *sync)
			: Genode::Thread<8192>("netsrv-server-thread"),
			_cap(cap),
			_sync(sync)
			{
				start();
			}
	};
	

	void Rx_handler::send_packet(void *src, Genode::size_t size)
	{
		PERF_ENTER;
		Nic::Session::Rx::Source *source = _component->rx_source();

		while (true) {
			/* flush remaining acknowledgements */
			while (source->ack_avail())
				source->release_packet(source->get_acked_packet());

			try {
				/* allocate packet in rx channel */
				Packet_descriptor rx_packet = source->alloc_packet(size);

				Genode::memcpy((void*)source->packet_content(rx_packet),
							(void*)src, size);
				source->submit_packet(rx_packet);
				PERF_EXIT;
				return;
			} catch (Nic::Session::Rx::Source::Packet_alloc_failed) { }
		}
		PERF_EXIT;
	}
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
// 		PERF_ENTER;
		static Counter counter;
		
// 		PDBG("l4linux transmit - send to NIC receive buf");

		try {
			rx_handler->send_packet(addr, len);
			counter.inc(len);
		} catch(...) {
			/* Packet_alloc_failed' */
// 			PERF_EXIT;
			return 1;
		}
// 		PERF_EXIT;
		return 0;
	}


	int genode_netsrv_tx_ack_avail() {
		Linux::Irq_guard guard;
		return 0;
	}


	void genode_netsrv_tx_ack()
	{
		Linux::Irq_guard guard;
	}


	void genode_netsrv_rx_receive()
	{
		Linux::Irq_guard guard;
		static Counter counter;
// 		PERF_ENTER;
		
// 		PDBG("l4linux receive - read from NIC transmit buf");
		if (!rx_buf.buf || !rx_buf.size)
		{
			PERR("buffer wrong");
			return;
		}
		
		if (receive_packet && net_device)
			receive_packet(net_device, rx_buf.buf, rx_buf.size);

		counter.inc(rx_buf.size);
		
		_rx_sync.unlock();
// 		PERF_EXIT;

	}


	int genode_netsrv_ready()
	{
		static bool initialized = false;
		
		if (!initialized)
		{
			Linux::Irq_guard guard;
			static Genode::Lock lock(Genode::Lock::LOCKED);
			
			static Server_thread th(cap.dst(), &lock);
			lock.lock();
		}
		return 1;
	}


	void *genode_netsrv_memcpy(void *dst, void const *src, unsigned long size) {
		return Genode::memcpy(dst, src, size); }
}
