#include <linux/moduleparam.h>
#include <linux/module.h>

#include <linux/slab.h>	
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/spi/spi.h> /* spi */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>

#include <linux/in6.h>
#include <asm/checksum.h>

#include "si4463.h"

#define IRQ_NET_CHIP  20//需要根据硬件确定

MODULE_AUTHOR("wu:>");
MODULE_LICENSE("Dual BSD/GPL");

static char  netbuffer[100];
struct net_device *si4463_devs;
/*
 * si4463_rx,recieves a network packet and put the packet into TCP/IP up
 * layer,netif_rx() is the kernel API to do such thing. The recieving
 * procedure must alloc the sk_buff structure to store the data,
 * and the sk_buff will be freed in the up layer.
 */

void si4463_rx(struct net_device *dev, int len, unsigned char *buf)
{
	printk("si4463_rx\n");

    struct sk_buff *skb;
    struct si4463_priv *priv = netdev_priv(dev);//(struct si4463_priv *) dev->priv;

    skb = dev_alloc_skb(len+2);
    if (!skb) {
        printk("si4463_rx can not allocate more memory to store the packet. drop the packet\n");
        priv->stats.rx_dropped++;
        return;
    }
    skb_reserve(skb, 2);
    memcpy(skb_put(skb, len), buf, len);

    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    /* We need not check the checksum */
    skb->ip_summed = CHECKSUM_UNNECESSARY; 
    priv->stats.rx_packets++;
                              
    netif_rx(skb);
    return;
}

static irqreturn_t si4463__interrupt (int irq, void *dev_id)
{
	struct net_device *dev;
	dev = (struct net_device *) dev_id;

    //get data from hardware register

	si4463_rx(dev,100,netbuffer);
	
	return IRQ_HANDLED;
}

int si4463_open(struct net_device *dev)
{
	int ret=0;
	printk(KERN_ALERT "si4463_o2pen\n");

    	ret = request_irq(IRQ_NET_CHIP, si4463__interrupt, IRQF_SHARED,
			  dev->name, dev);
	printk(KERN_ALERT "request_irq returns %d\n", ret);
	if (ret) 
		return ret;
    	printk("request_irq ok\n");
	netif_start_queue(dev);
    	return 0;
}

int si4463_release(struct net_device *dev)
{
	printk("si4463_release\n");
    netif_stop_queue(dev);          
    return 0;
}

/*
 * pseudo network hareware transmit,it just put the data into the 
 * ed_tx device.
 */

void si4463_hw_tx(char *buf, int len, struct net_device *dev)
{
	printk("si4463_hw_rx\n");
    struct si4463_priv *priv;
   
    /* check the ip packet length,it must more then 34 octets */
    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
	{
        printk("Bad packet! It's size is less then 34!\n");
        return;
    }
    /* record the transmitted packet status */
    priv = (struct si4463_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_packets++;
    priv->stats.rx_bytes += len;
    
    /* remember to free the sk_buffer allocated in upper layer. */
    dev_kfree_skb(priv->skb);
}

/*
 * Transmit the packet,called by the kernel when there is an
 * application wants to transmit a packet.
 */

int si4463_tx(struct sk_buff *skb, struct net_device *dev)
{
	printk("si4463_tx\n");
    int len;
    char *data;
    struct si4463_priv *priv = (struct si4463_priv *) netdev_priv(dev);//dev->priv;

    len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    data = skb->data;
    /* stamp the time stamp */
    dev->trans_start = jiffies;

    /* remember the skb and free it in si4463_hw_tx */
    priv->skb = skb;
    
    /* pseudo transmit the packet,hehe */
    si4463_hw_tx(data, len, dev);

    return 0; 
}

/*
 * Deal with a transmit timeout.
 */
void si4463_tx_timeout (struct net_device *dev)
{
    struct si4463_priv *priv = (struct si4463_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_errors++;
    netif_wake_queue(dev);

    return;
}



/*
 * When we need some ioctls.
 */
int si4463_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	printk("si4463_ioctl\n");
    return 0;
}

/*
 * ifconfig to get the packet transmitting status.
 */

struct net_device_stats *si4463_stats(struct net_device *dev)
{
    struct si4463_priv *priv = (struct si4463_priv *) netdev_priv(dev);//dev->priv;
    return &priv->stats;
}

/*
 * TCP/IP handshake will call this function, if it need.
 */
int si4463_change_mtu(struct net_device *dev, int new_mtu)
{
    unsigned long flags;
    spinlock_t *lock = &((struct si4463_priv *) netdev_priv(dev)/*dev->priv*/)->lock;
    
    /* en, the mtu CANNOT LESS THEN 68 OR MORE THEN 1500. */
    if (new_mtu < 68)
        return -EINVAL;
   
    spin_lock_irqsave(lock, flags);
    dev->mtu = new_mtu;
    spin_unlock_irqrestore(lock, flags);

    return 0; 
}

static const struct net_device_ops my_netdev_ops = {

	.ndo_open            = si4463_open,
	.ndo_stop            = si4463_release,
	.ndo_start_xmit = si4463_tx,
	.ndo_do_ioctl        = si4463_ioctl,
	.ndo_get_stats       = si4463_stats,
	.ndo_change_mtu      = si4463_change_mtu,  
	.ndo_tx_timeout      = si4463_tx_timeout,
};
void si4463_init(struct net_device *dev)
{

	struct si4463_priv *priv;
	ether_setup(dev); /* assign some of the fields */

	dev->netdev_ops = &my_netdev_ops;

	dev->dev_addr[0] = 0x18;//(0x01 & addr[0])为multicast
	dev->dev_addr[1] = 0x02;
	dev->dev_addr[2] = 0x03;
	dev->dev_addr[3] = 0x04;
	dev->dev_addr[4] = 0x05;
	dev->dev_addr[5] = 0x06;

	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;
	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct si4463_priv));
	spin_lock_init(&priv->lock);
}

void si4463_cleanup(void)
{
	if (si4463_devs) 
	{
		unregister_netdev(si4463_devs);
		free_netdev(si4463_devs);
	}
	return;
}

int si4463_init_module(void)
{
	int result,ret = -ENOMEM;

	/* Allocate the devices */
	si4463_devs=alloc_netdev(sizeof(struct si4463_priv), "rf%d",
			si4463_init);
	if (si4463_devs == NULL)
		goto out;


	ret = -ENODEV;
	if ((result = register_netdev(si4463_devs)))
		printk("demo: error %i registering device \"%s\"\n",result, si4463_devs->name);
	else
		ret = 0;
   out:
	if (ret) 
		si4463_cleanup();
	return ret;
}

module_init(si4463_init_module);
module_exit(si4463_cleanup);

