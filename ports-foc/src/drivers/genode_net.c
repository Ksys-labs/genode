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

/* Linux includes */
#include <linux/etherdevice.h>
#include <linux/errno.h>
#include <linux/inet.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/string.h>
#include <linux/types.h>

#include <genode/net.h>


static struct net_device *net_dev[MAX_GENODE_NET];

static int num_genode_net;

int if_nums = 1;
module_param(if_nums, int, 1);

static void FASTCALL
genode_net_receive_packet(void* dev_addr, void *addr,
                          unsigned long size)
{
	struct net_device       *dev   = (struct net_device *) dev_addr;
	struct net_device_stats *stats = (struct net_device_stats*) netdev_priv(dev);

	/* allocate skb */
	struct sk_buff *skb = dev_alloc_skb(size + 4);
	if (!skb) {
		if (printk_ratelimit())
			printk(KERN_NOTICE "genode_net_rx: low on mem - packet dropped!\n");
		stats->rx_dropped++;
		return;
	}

	/* copy packet */
	genode_net_memcpy(skb_put(skb, size), addr, size);

	skb->dev       = dev;
	skb->protocol  = eth_type_trans(skb, dev);
	skb->ip_summed = CHECKSUM_NONE;

	netif_rx(skb);

	stats->rx_packets++;
	stats->rx_bytes += size;
}

static int net_num(struct net_device *dev)
{
	int i;

	for(i = 0; i < num_genode_net; i++) {
		if ( !genode_net_strcmp(net_dev[i]->name, dev->name, IFNAMSIZ) )
			return i;
	}

	return -1;
}

/********************************
 **  Network driver functions  **
 ********************************/

int genode_net_open(struct net_device *dev)
{
	int if_num = net_num(dev);
	if (if_num < 0)
		return -ENOMEM;
	genode_net_start(if_num, dev, genode_net_receive_packet);
	netif_start_queue(dev);
	return 0;
}


int genode_net_close(struct net_device *dev)
{
	int if_num = net_num(dev);
	if (if_num < 0)
		return -ENOMEM;
	
	netif_stop_queue(dev);
	genode_net_stop(if_num);
	return 0;
}


int genode_net_xmit_frame(struct sk_buff *skb, struct net_device *dev)
{
	struct net_device_stats *stats = (struct net_device_stats*) netdev_priv(dev);
	int len                        = skb->len;
	void* addr                     = skb->data;

	int if_num                     = net_num(dev);
	
	if (if_num < 0)
		return -ENOMEM;

	/* collect acknowledgements of old packets */
	while (genode_net_tx_ack_avail(if_num))
		genode_net_tx_ack(if_num);

	/* transmit to nic-session */
	while (genode_net_tx(if_num, addr, len)) {
		/* tx queue is  full, could not enqueue packet */
		genode_net_tx_ack(if_num);
	}
	dev_kfree_skb(skb);

	/* save timestamp */
	dev->trans_start = jiffies;

	stats->tx_packets++;
	stats->tx_bytes += len;

	return 0;
}


struct net_device_stats* genode_net_get_stats(struct net_device *dev)
{
	return (struct net_device_stats*) netdev_priv(dev);
}


void genode_net_tx_timeout(struct net_device *dev)
{
}


static irqreturn_t event_interrupt(int irq, void *data)
{
	int i;
	for(i = 0; i < num_genode_net; i++) {
		if ( net_dev[i]->irq == irq )
		{
			genode_net_rx_receive(i);
			return IRQ_HANDLED;
		}
	}
	return IRQ_NONE;
}


/**************************
 **  De-/Initialization  **
 **************************/

static const struct net_device_ops genode_net_dev_ops =
{
	.ndo_open       = genode_net_open,
	.ndo_stop       = genode_net_close,
	.ndo_start_xmit = genode_net_xmit_frame,
	.ndo_get_stats  = genode_net_get_stats,
	.ndo_tx_timeout = genode_net_tx_timeout
};

/* Setup and register the device. */
static int __init genode_net_init(void)
{
	int                err = 0;
	unsigned           irq;
	l4_cap_idx_t       irq_cap;
	
	struct net_device *dev;
	int                i;
	
	num_genode_net = 0;
	
	if (if_nums <= 0) {
		printk(KERN_WARNING "%s: if_num=%d isn't supported. Using default if_num=1\n", __func__, if_nums);
		if_nums = 1;
	}
	
	if (if_nums > MAX_GENODE_NET) {
		printk(KERN_WARNING "%s: if_num=%d is too big. Using maximum if_num=%d\n", __func__, if_nums, MAX_GENODE_NET);
		if_nums = MAX_GENODE_NET;
	}
	
	for (i = 0; i < if_nums; i++) {
		if (!genode_net_ready(i)) {
			printk (KERN_ERR "%s: Genode NIC%d isn't ready\n", __func__, i);
			continue;
		}

		/* allocate network device */
		if (!(dev = alloc_etherdev(sizeof(struct net_device_stats)))) {
			printk (KERN_ERR "%s: NIC%d - alloc_etherdev failed\n", __func__, i);
			continue;
		}

		dev->netdev_ops      = &genode_net_dev_ops;
		dev->watchdog_timeo  = 20 * HZ;

		/* set MAC address */
		genode_net_mac(i, dev->dev_addr, ETH_ALEN);

		/**
		* Obtain an IRQ for the device.
		*/
		irq_cap = genode_net_irq_cap(i);
		if ((irq = l4x_register_irq(irq_cap)) < 0)
		{
			printk(KERN_WARNING "%s: NIC%d - l4x_register_irq failed\n", __func__, i);
			free_netdev(dev);
			err = -ENOMEM;
			continue;
		}
		
		if ((err = request_irq(irq, event_interrupt, IRQF_SAMPLE_RANDOM,
							"Genode net", dev))) {
			printk(KERN_WARNING "%s: NIC%d - request_irq failed: %d\n", __func__, i, err);
			free_netdev(dev);
			continue;
		}
		dev->irq = irq;

		/* register network device */
		if ((err = register_netdev(dev))) {
			printk(KERN_ERR "%s: NIC%d - Failed to register netdevice: %d\n", __func__, i, err);
			free_netdev(dev);
			continue;
		}
		
		net_dev[num_genode_net++] = dev;
	}

	if (num_genode_net)
	{
		genode_net_run(num_genode_net);
		return 0;
	}
	else
		return err;
};


static void __exit genode_net_exit(void)
{
	int i;
	for (i = 0; i < num_genode_net; i++) {
		unregister_netdev(net_dev[i]);
		free_netdev(net_dev[i]);
	}
}


module_init(genode_net_init);
module_exit(genode_net_exit);
