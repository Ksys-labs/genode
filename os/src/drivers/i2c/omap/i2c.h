/*
 * \brief  I2C Driver for OMAP4
 * \author Ivan Loskutov
 * \author Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date   2012-09-18
 */

/*
 * Copyright (C) 2012 Ksys Labs LLC
 * Copyright (C) 2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _I2C_H_
#define _I2C_H_

#include <util/mmio.h>

namespace I2C {
	using namespace Genode;
	class Omap4_I2C;
}

struct I2C::Omap4_I2C : Mmio
{
	enum {
		I2C_TIMEOUT                 = 1000,
		CONFIG_SYS_I2C_SPEED        = 100000,
		CONFIG_SYS_I2C_SLAVE        = 1,

		SYSTEM_CLOCK_96             = 96000000,
		I2C_IP_CLK                  = SYSTEM_CLOCK_96,

		I2C_INTERNAL_SAMPLING_CLK   = 19200000,

		I2C_FASTSPEED_SCLL_TRIM     = 6,
		I2C_FASTSPEED_SCLH_TRIM     = 6,


		/* I2C Interrupt Enable Register (I2C_IE): */
		I2C_IE_GC_IE = (1 << 5),
		I2C_IE_XRDY_IE	= (1 << 4), /* Transmit data ready interrupt enable */
		I2C_IE_RRDY_IE	= (1 << 3), /* Receive data ready interrupt enable */
		I2C_IE_ARDY_IE	= (1 << 2), /* Register access ready interrupt enable */
		I2C_IE_NACK_IE	= (1 << 1), /* No acknowledgment interrupt enable */
		I2C_IE_AL_IE	= (1 << 0), /* Arbitration lost interrupt enable */

		/* I2C Status Register (I2C_STAT): */
		I2C_STAT_SBD	= (1 << 15), /* Single byte data */
		I2C_STAT_BB	 = (1 << 12), /* Bus busy */
		I2C_STAT_ROVR	= (1 << 11),  /* Receive overrun */
		I2C_STAT_XUDF	= (1 << 10), /* Transmit underflow */
		I2C_STAT_AAS	= (1 << 9),  /* Address as slave */
		I2C_STAT_GC	= (1 << 5),
		I2C_STAT_XRDY	= (1 << 4),  /* Transmit data ready */
		I2C_STAT_RRDY	= (1 << 3),  /* Receive data ready */
		I2C_STAT_ARDY	= (1 << 2),  /* Register access ready */
		I2C_STAT_NACK	= (1 << 1),  /* No acknowledgment interrupt enable */
		I2C_STAT_AL	= (1 << 0),  /* Arbitration lost interrupt enable */

		/* I2C Interrupt Code Register (I2C_INTCODE): */
		I2C_INTCODE_MASK	= 7,
		I2C_INTCODE_NONE	= 0,
		I2C_INTCODE_AL		= 1,	/* Arbitration lost */
		I2C_INTCODE_NAK		= 2,	/* No acknowledgement/general call */
		I2C_INTCODE_ARDY	= 3,	/* Register access ready */
		I2C_INTCODE_RRDY	= 4,	/* Rcv data ready */
		I2C_INTCODE_XRDY	= 5,	/* Xmit data ready */

		/* I2C Buffer Configuration Register (I2C_BUF): */

		I2C_BUF_RDMA_EN		= (1 << 15), /* Receive DMA channel enable */
		I2C_BUF_XDMA_EN		= (1 << 7),  /* Transmit DMA channel enable */

		/* I2C Configuration Register (I2C_CON): */
		I2C_CON_EN	= (1 << 15),  /* I2C module enable */
		I2C_CON_BE	= (1 << 14),  /* Big endian mode */
		I2C_CON_STB	= (1 << 11),  /* Start byte mode (master mode only) */
		I2C_CON_MST	= (1 << 10),  /* Master/slave mode */
		I2C_CON_TRX	= (1 << 9),   /* Transmitter/receiver mode */
		/* (master mode only) */
		I2C_CON_XA	= (1 << 8),   /* Expand address */
		I2C_CON_STP	= (1 << 1),   /* Stop condition (master mode only) */
		I2C_CON_STT	= (1 << 0),   /* Start condition (master mode only) */

		/* I2C System Test Register (I2C_SYSTEST): */
		I2C_SYSTEST_ST_EN	= (1 << 15), /* System test enable */
		I2C_SYSTEST_FREE	= (1 << 14), /* Free running mode, on brkpoint) */
		I2C_SYSTEST_TMODE_MASK	= (3 << 12), /* Test mode select */
		I2C_SYSTEST_TMODE_SHIFT	= (12),	  /* Test mode select */
		I2C_SYSTEST_SCL_I	= (1 << 3),  /* SCL line sense input value */
		I2C_SYSTEST_SCL_O	= (1 << 2),  /* SCL line drive output value */
		I2C_SYSTEST_SDA_I	= (1 << 1),  /* SDA line sense input value */
		I2C_SYSTEST_SDA_O	= (1 << 0),  /* SDA line drive output value */

		/* I2C System Status Register (I2C_SYSS): */

		I2C_SYSS_RDONE          = (1 << 0),  /* Internel reset monitoring */

	};

	Omap4_I2C(Genode::addr_t const mmio_base) : Mmio(mmio_base)
	{
		init();
	}

	~Omap4_I2C() {
		write<Ie>(0);
		write<Con>(0);
		flush_fifo();
		write<Cnt>(0);
		write<Stat>(0xFFFF);
		_timer.msleep(50);
	}

	struct Revnb_lo      : Register<0x00, 16> {};		/* 0x00 */
	struct Revnb_hi      : Register<0x04, 16> {};		/* 0x04 */
	struct Sysc          : Register<0x20, 16> {};		/* 0x20 */
	struct Irqstatus_raw : Register<0x24, 16> {};		/* 0x24 */
	struct Stat          : Register<0x28, 16> {};		/* 0x28 */
	struct Ie            : Register<0x2c, 16> {};		/* 0x2C */
	struct Irqenable_clr : Register<0x30, 16> {};		/* 0x30 */
	struct Iv            : Register<0x34, 16> {};		/* 0x34 */
	struct Syss          : Register<0x90, 16> {};		/* 0x90 */
	struct Buf           : Register<0x94, 16> {};		/* 0x94 */
	struct Cnt           : Register<0x98, 16> {};		/* 0x98 */
	struct Data          : Register<0x9c,  8> {};		/* 0x9C */
	struct Con           : Register<0xa4, 16> {};		/* 0xA4 */
	struct Oa            : Register<0xa8, 16> {};		/* 0xA8 */
	struct Sa            : Register<0xac, 16> {};		/* 0xAC */
	struct Psc           : Register<0xb0, 16> {};		/* 0xB0 */
	struct Scll          : Register<0xb4, 16> {};		/* 0xB4 */
	struct Sclh          : Register<0xb8, 16> {};		/* 0xB8 */
	struct Systest       : Register<0xbc, 16> {};		/* 0xBC */
	struct Bufstat       : Register<0xc0, 16> {};		/* 0xC0 */

	Timer::Connection _timer;

	void init(int speed = CONFIG_SYS_I2C_SPEED, int slaveadd = CONFIG_SYS_I2C_SLAVE )
	{
		int psc, fsscll, fssclh;
		uint32_t scll, sclh;
		int timeout = I2C_TIMEOUT;

		psc = I2C_IP_CLK / I2C_INTERNAL_SAMPLING_CLK;
		psc -= 1;

		PDBG("I2C Revision: %04x.%04x\n", read<Revnb_hi>(), read<Revnb_lo>());

		// todo: standart speed only
		/* Standard and fast speed */
		fsscll = fssclh = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

		fsscll -= I2C_FASTSPEED_SCLL_TRIM;
		fssclh -= I2C_FASTSPEED_SCLH_TRIM;
		if (((fsscll < 0) || (fssclh < 0)) ||
			((fsscll > 255) || (fssclh > 255))) {
			PERR("Error : I2C initializing clock\n");
			return;
		}

		scll = (unsigned int)fsscll;
		sclh = (unsigned int)fssclh;

		if (read<Con>() & I2C_CON_EN) {
			write<Con>(0);
			_timer.msleep(50);
		}

		write<Sysc>(0x2); /* for ES2 after soft reset */
		_timer.msleep(1);

		write<Con>(I2C_CON_EN);
		while (!(read<Syss>() & I2C_SYSS_RDONE) && timeout--) {
			if (timeout <= 0) {
				PERR("ERROR: Timeout in soft-reset\n");
				return;
			}
			_timer.msleep(1);
		}

		write<Con>(0);
		write<Psc>(psc);
		write<Scll>(scll);
		write<Sclh>(sclh);

		/* own address */
		write<Oa>(slaveadd);
		write<Con>(I2C_CON_EN);

		/* have to enable intrrupts or OMAP i2c module doesn't work */
		write<Ie>(I2C_IE_XRDY_IE | I2C_IE_RRDY_IE | I2C_IE_ARDY_IE | I2C_IE_NACK_IE | I2C_IE_AL_IE);
		_timer.msleep(1);

		flush_fifo();

		write<Stat>(0xFFFF);
		write<Cnt>(0);
	}

	void flush_fifo(void)
	{
		uint16_t stat;

		/* note: if you try and read data when its not there or ready
		* you get a bus error
		*/
		while (1) {
			stat = read<Stat>();
			if (stat == I2C_STAT_RRDY) {
				read<Data>();
				write<Stat>(I2C_STAT_RRDY);
				_timer.msleep(1);
			} else
				break;
		}
	}

	int i2c_read(uint8_t devaddr, uint32_t regoffset, int alen, uint8_t *buffer, int len)
	{
		int i;

		if (alen > 1) {
			PERR("I2C read: addr len %d not supported\n", alen);
			return 1;
		}

		if (regoffset + len > 256) {
			PERR("I2C read: address out of range\n");
			return 1;
		}
		
		int i2c_error = 0;
		uint16_t status;

		/* wait until bus not busy */
		wait_for_bb();

		/* one byte only */
		write<Cnt>(1);
		/* set slave address */
		write<Sa>(devaddr);
		/* no stop bit needed here */
		write<Con>(I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX);

		/* send register offset */
		while (1) {
			status = wait_for_pin();
			if (status == 0 || status & I2C_STAT_NACK) {
				i2c_error = 1;
				goto read_exit;
			}
			if (status & I2C_STAT_XRDY) {
				/* Important: have to use byte access */
				write<Data>(regoffset);
				write<Stat>(I2C_STAT_XRDY);
			}
			if (status & I2C_STAT_ARDY) {
				write<Stat>(I2C_STAT_ARDY);
				break;
			}
		}

		/* set slave address */
		write<Sa>(devaddr);
		/* read one byte from slave */
		write<Cnt>(len);
		/* need stop bit here */
		write<Con>(I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP);

		/* receive data */
		for (i = 0; i < len; i++) {
			status = wait_for_pin();
			if (status == 0 || status & I2C_STAT_NACK) {
				i2c_error = 1;
				goto read_exit;
			}
			if (status & I2C_STAT_RRDY) {
				buffer[i] = read<Data>();
				write<Stat>(I2C_STAT_RRDY);
			}

			if (status & I2C_STAT_ARDY) {
				write<Stat>(I2C_STAT_ARDY);
				break;
			}
		}

	read_exit:
		flush_fifo();
		write<Stat>(0xFFFF);
		write<Cnt>(0);
		return i2c_error;
	}
	
	int i2c_read_byte(uint8_t devaddr, uint8_t regoffset, uint8_t *value)
	{
		return i2c_read(devaddr, regoffset, 1, value, 1);
	}

	int i2c_write(uint8_t chip, uint32_t addr, int alen, uint8_t *buffer, int len)
	{
		int i;
		uint16_t status;
		int i2c_error = 0;

		if (alen > 1) {
			PERR("I2C write: addr len %d not supported\n", alen);
			return 1;
		}

		if (addr + len > 256) {
			PERR("I2C write: address 0x%x + 0x%x out of range\n", addr, len);
			return 1;
		}

		/* wait until bus not busy */
		wait_for_bb();

		/* start address phase - will write regoffset + len bytes data */
		/* TODO consider case when !CONFIG_OMAP243X/34XX/44XX */
		write<Cnt>(alen + len);
		/* set slave address */
		write<Sa>(chip);
		/* stop bit needed here */
		write<Con>(I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX | I2C_CON_STP);

		/* Send address byte */
		status = wait_for_pin();

		if (status == 0 || status & I2C_STAT_NACK) {
			i2c_error = 1;
			PERR("error waiting for i2c address ACK (status=0x%x)\n", status);
			goto write_exit;
		}

		if (status & I2C_STAT_XRDY) {
			write<Data>(addr & 0xFF);
			write<Stat>(I2C_STAT_XRDY);
		} else {
			i2c_error = 1;
			PERR("i2c bus not ready for transmit (status=0x%x)\n", status);
			goto write_exit;
		}

		/* address phase is over, now write data */
		for (i = 0; i < len; i++) {
			status = wait_for_pin();

			if (status == 0 || status & I2C_STAT_NACK) {
				i2c_error = 1;
				PERR("i2c error waiting for data ACK (status=0x%x)\n", status);
				goto write_exit;
			}

			if (status & I2C_STAT_XRDY) {
				write<Data>(buffer[i]);
				write<Stat>(I2C_STAT_XRDY);
			} else {
				i2c_error = 1;
				PERR("i2c bus not ready for Tx (i=%d)\n", i);
				goto write_exit;
			}
		}

	write_exit:
		flush_fifo();
		write<Stat>(0xFFFF);
		return i2c_error;
	}

	void wait_for_bb(void)
	{
		int timeout = I2C_TIMEOUT;
		uint16_t stat;

		write<Stat>(0xFFFF);	/* clear current interrupts...*/
		while ((stat = read<Stat>() & I2C_STAT_BB) && timeout--) {
			write<Stat>(stat);
			_timer.msleep(1);
		}

		if (timeout <= 0) {
			PERR("timed out in wait_for_bb: I2C_STAT=%x\n", read<Stat>());
		}
		write<Stat>(0xFFFF);	 /* clear delayed stuff*/
	}

	uint16_t wait_for_pin(void)
	{
		uint16_t status;
		int timeout = I2C_TIMEOUT;

		do {
			_timer.msleep(1);
			status = read<Stat>();
		} while (!(status &
		(I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
		I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
		I2C_STAT_AL)) && timeout--);

		if (timeout <= 0) {
			PERR("timed out in wait_for_pin: I2C_STAT=%x\n", read<Stat>());
			write<Stat>(0xFFFF);
			status = 0;
		}

		return status;
	}
};

#endif // _I2C_H_
