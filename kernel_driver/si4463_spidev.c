/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/gpio.h>
#include <linux/lnw_gpio.h>

#include <asm/uaccess.h>

/* For the si4463 module. If spidev module is required, then ... */
//#define SI4463

/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY)

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

/*-------------------------------------------------------------------------*/



static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*-------------------------------------------------------------------------*/
struct spi_device *spi_save;
struct spidev_data spidev_global;

#include "si4463.h"
#include "si4463_api.h"

#include <linux/delay.h>
#include <linux/kernel.h> /* printk() */
#include <linux/types.h>  /* size_t */
#include <linux/interrupt.h> /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>

#include <linux/in6.h>
#include <asm/checksum.h>



#define IRQ_NET_CHIP  20//需要根据硬件确定


/* GLOBAL */
static char  netbuffer[100];
struct net_device *si4463_devs;

#ifdef SI4463





void ppp(u8 * arr, int len){
	int i = 0;
	printk(KERN_ALERT "ppp: len=[%d]\n", len);
	for(;i<len;i++)
		printk(KERN_ALERT "%x ", arr[i]);
	printk(KERN_ALERT "\n");
}
/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void si4463_complete(void *arg)
{
	complete(arg);
}

static ssize_t
si4463_sync(struct spidev_data *spidev, struct spi_message *message)
{
	gpio_set_value(CS_SELF, 0);

	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = si4463_complete;
	message->context = &done;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
	spin_unlock_irq(&spidev->spi_lock);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}

	gpio_set_value(CS_SELF, 1);
	return status;
}

inline ssize_t
si4463_sync_transfer(struct spidev_data *spidev, u8 *in_buf, u8 *out_buf, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= in_buf,
			.rx_buf		= out_buf,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return si4463_sync(spidev, &m);
}

inline ssize_t
si4463_sync_write(struct spidev_data *spidev,  size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return si4463_sync(spidev, &m);
}

inline ssize_t
si4463_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return si4463_sync(spidev, &m);
}





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
	if (ret)
		return ret;
    printk("request_irq ok\n");

    //getCTS();

	netif_start_queue(dev);

	gpio_set_value(RADIO_SDN, 0);//enable

	getCTS();
	reset();

    return 0;
}

int si4463_release(struct net_device *dev)
{
	printk("si4463_release\n");
    netif_stop_queue(dev);
    gpio_set_value(RADIO_SDN, 1);//disable
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




static int spidev_probe(struct spi_device *spi)
{
	spi_save = spi;
	spidev_global.spi = spi;
	spin_lock_init(&spidev_global.spi_lock);
	mutex_init(&spidev_global.buf_lock);

	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;

	int saved_muxing = 0;
	int 		ret = 0;
	int			err = 0;
	int			retval = 0;
	u32			tmp;
	unsigned	n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_RET(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);


	/* spi setup :
	 * 		SPI_MODE_0
	 * 		MSBFIRST
	 * 		CS active low
	 * 		IRQ??????
	 */

	spin_lock_irq(&spidev->spi_lock);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	tmp = ~SPI_MODE_MASK;

	printk(KERN_ALERT "init: tmp= %x\n", tmp);
	tmp |= SPI_MODE_0;
	//tmp |= SPI_CS_HIGH;
	//tmp |= SPI_READY;
	printk(KERN_ALERT "midd: tmp= %x\n", tmp);
	tmp |= spi->mode & ~SPI_MODE_MASK;
	printk(KERN_ALERT "after: tmp= %x\n", tmp);
	spi->mode = (u8)tmp;
	spi->bits_per_word = BITS_PER_WORD;
	spi->max_speed_hz = SPI_SPEED;

	ret = spi_setup(spi);
	if (ret < 0)
		printk(KERN_ALERT "ERROR! spi_setup return: %d\n", ret);
	else
		printk(KERN_ALERT "spi_setup succeed\n");

	/* GPIO setup
	 *
	 */
	/* Request Chip SDN gpios */
	if (gpio_is_valid(RADIO_SDN)) {
		//saved_muxing = gpio_get_alt(RADIO_SDN);
		//lnw_gpio_set_alt(RADIO_SDN, LNW_GPIO);
		err = gpio_request_one(RADIO_SDN,
				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio SDN pin");
		if (err) {
			/*pr_err(
					"%s: unable to get Chip Select GPIO,\
						fallback to legacy CS mode \n",
					__func__);*/
			printk(KERN_ALERT "ERROR! RADIO_SDN request error: %d\n", err);
			//lnw_gpio_set_alt(RADIO_SDN, saved_muxing);
		}
		gpio_direction_output(RADIO_SDN, 1);

	} else {
		printk(KERN_ALERT "ERROR! invalid pin RADIO_SDN\n");
	}

	/* Request Chip Select gpios */
	if (gpio_is_valid(CS_SELF)) {
		//saved_muxing = gpio_get_alt(CS_SELF);
		//lnw_gpio_set_alt(CS_SELF, LNW_GPIO);
		err = gpio_request_one(CS_SELF,
				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio CS pin");
		if (err) {
			/*pr_err(
					"%s: unable to get Chip Select GPIO,\
						fallback to legacy CS mode \n",
					__func__);*/
			printk(KERN_ALERT "ERROR! CS_SELF request error: %d\n", err);
			//lnw_gpio_set_alt(CS_SELF, saved_muxing);
		}
		gpio_direction_output(CS_SELF, 1);
	} else {
		printk(KERN_ALERT "ERROR! invalid pin CS_SELF\n");
	}





	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{},
};

MODULE_DEVICE_TABLE(of, spidev_dt_ids);

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

void si4463_cleanup(void)
{
	gpio_free(RADIO_SDN);
	gpio_free(CS_SELF);
	if (si4463_devs)
	{
		unregister_netdev(si4463_devs);
		free_netdev(si4463_devs);
	}
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	return;
}

int si4463_init_module(void)
{

	int result,ret = -ENOMEM;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}

	ret = spi_register_driver(&spidev_spi_driver);
	if (ret < 0) {
		printk("spi_register_driver return %d\n",ret);
		goto out;
	}

	/* Allocate the devices */
	si4463_devs = alloc_netdev(sizeof(struct si4463_priv), "rf%d", si4463_init);
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

#else
/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
void ppp(u8 * arr, int len){
	int i = 0;
	printk(KERN_ALERT "ppp: len=[%d]\n", len);
	for(;i<len;i++)
		printk(KERN_ALERT "%x ", arr[i]);
	printk(KERN_ALERT "\n");
}
/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void spidev_complete(void *arg)
{
	complete(arg);
}

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
//	gpio_set_value(CS_SELF, 0);
	//ndelay(100);

	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = spidev_complete;
	message->context = &done;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
	spin_unlock_irq(&spidev->spi_lock);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}


//	gpio_set_value(CS_SELF, 1);
	return status;
}

inline ssize_t
spidev_sync_transfer(struct spidev_data *spidev, u8 *tx_buf, u8 *rx_buf, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= tx_buf,
			.rx_buf		= rx_buf,
			.len		= len,
			.cs_change	= 0,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

inline ssize_t
spidev_sync_write(struct spidev_data *spidev,  size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->buffer,
			.len		= len,
			.cs_change	= 1,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->buffer,
			.len		= len,
			.cs_change	= 1,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}


/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	status = spidev_sync_read(spidev, count);
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, spidev->buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;
	unsigned long		missing;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->buffer, buf, count);
	if (missing == 0) {
		status = spidev_sync_write(spidev, count);
	} else
		status = -EFAULT;
	mutex_unlock(&spidev->buf_lock);

	return status;
}



static int spidev_message(struct spidev_data *spidev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total;
	u8			*buf;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	buf = spidev->buffer;
	total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		if (total > bufsiz) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			k_tmp->rx_buf = buf;
			if (!access_ok(VERIFY_WRITE, (u8 __user *)
						(uintptr_t) u_tmp->rx_buf,
						u_tmp->len))
				goto done;
		}
		if (u_tmp->tx_buf) {
			k_tmp->tx_buf = buf;
			if (copy_from_user(buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					u_tmp->len))
				goto done;
		}
		buf += k_tmp->len;

		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
		dev_dbg(&spidev->spi->dev,
			"  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : spidev->spi->max_speed_hz);
#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	//printk(KERN_ALERT "I'm here!!!!\n");
/*	printk(KERN_ALERT "n_xfers: %d\n", n_xfers);
	printk(KERN_ALERT "spi_ioc_transfer:\n");
	printk(KERN_ALERT "len: %d\n",u_xfers->len);
	printk(KERN_ALERT "speed_hz: %d\n",u_xfers->speed_hz);
	printk(KERN_ALERT "delay_usecs: %d\n",u_xfers->delay_usecs);
	printk(KERN_ALERT "bits_per_word: %d\n",u_xfers->bits_per_word);
	printk(KERN_ALERT "cs_change: %d\n",u_xfers->cs_change);
*/
	status = spidev_sync(spidev, &msg);
	if (status < 0)
		goto done;

	/* copy any rx data out of bounce buffer */
	buf = spidev->buffer; //
	//printk(KERN_ALERT "BUF: %x\n", *buf);

	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, buf,
					u_tmp->len)) {
				status = -EFAULT;
				goto done;
			}
		}
		buf += u_tmp->len;
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
		retval = __put_user(spi->max_speed_hz, (__u32 __user *)arg);
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
		retval = __get_user(tmp, (u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			spi->mode = (u8)tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %02x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->max_speed_hz = save;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
		}
		break;

	default:
		/* segmented and/or full-duplex I/O request */
		if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
				|| _IOC_DIR(cmd) != _IOC_WRITE) {
			retval = -ENOTTY;
			break;
		}

		tmp = _IOC_SIZE(cmd);
		if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
			retval = -EINVAL;
			break;
		}
		n_ioc = tmp / sizeof(struct spi_ioc_transfer);
		if (n_ioc == 0)
			break;

		/* copy into scratch area */
		ioc = kmalloc(tmp, GFP_KERNEL);
		if (!ioc) {
			retval = -ENOMEM;
			break;
		}
		if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
			kfree(ioc);
			retval = -EFAULT;
			break;
		}

		/* translate to spi_message, execute */

		//printk(KERN_ALERT "spidev->spi: %x,  spi_save: %x\n", spidev->spi,spi_save);

		//spidev_global.buffer = kmalloc(5, GFP_KERNEL);
		retval = spidev_message(spidev, ioc, n_ioc);

		//getCTS();
		//printk(KERN_ALERT "RESET:\n");
		//reset();
		kfree(ioc);
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

#ifdef CONFIG_COMPAT
static long
spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (!spidev->buffer) {
			spidev->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (!spidev->buffer) {
				dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			spidev->users++;
			filp->private_data = spidev;
			nonseekable_open(inode, filp);
		}
	} else
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));

	mutex_unlock(&device_list_lock);
	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = 0;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;

	/* last close? */
	spidev->users--;
	if (!spidev->users) {
		int		dofree;

		kfree(spidev->buffer);
		spidev->buffer = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&spidev->spi_lock);
		dofree = (spidev->spi == NULL);
		spin_unlock_irq(&spidev->spi_lock);

		if (dofree)
			kfree(spidev);
	}
	mutex_unlock(&device_list_lock);

	return status;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

/*-------------------------------------------------------------------------*/

static int spidev_probe(struct spi_device *spi)
{
	spi_save = spi;
	spidev_global.spi = spi;
	spin_lock_init(&spidev_global.spi_lock);
	mutex_init(&spidev_global.buf_lock);

	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;

	int saved_muxing = 0;
	int 		ret = 0;
	int			err = 0;
	int			retval = 0;
	u32			tmp;
	unsigned	n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = PTR_RET(dev);
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);

	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);


	/* spi setup :
	 * 		SPI_MODE_0
	 * 		MSBFIRST
	 * 		CS active low
	 * 		IRQ??????
	 */

	spin_lock_irq(&spidev->spi_lock);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	tmp = ~SPI_MODE_MASK;

	printk(KERN_ALERT "init: tmp= %x\n", tmp);
	tmp |= SPI_MODE_0;
	//tmp |= SPI_NO_CS;
	//tmp |= SPI_CS_HIGH;
	//tmp |= SPI_READY;
	printk(KERN_ALERT "midd: tmp= %x\n", tmp);
	tmp |= spi->mode & ~SPI_MODE_MASK;
	printk(KERN_ALERT "after: tmp= %x\n", tmp);
	spi->mode = (u8)tmp;
	spi->bits_per_word = BITS_PER_WORD;
	spi->max_speed_hz = SPI_SPEED;

	ret = spi_setup(spi);
	if (ret < 0)
		printk(KERN_ALERT "ERROR! spi_setup return: %d\n", ret);
	else
		printk(KERN_ALERT "spi_setup succeed\n");

	/* GPIO setup
	 *
	 */
	/* Request Chip SDN gpios */
	if (gpio_is_valid(RADIO_SDN)) {
		saved_muxing = gpio_get_alt(RADIO_SDN);
		lnw_gpio_set_alt(RADIO_SDN, LNW_GPIO);
		err = gpio_request_one(RADIO_SDN,
				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio SDN pin");
		if (err) {
			/*pr_err(
					"%s: unable to get Chip Select GPIO,\
						fallback to legacy CS mode \n",
					__func__);*/
			printk(KERN_ALERT "ERROR! RADIO_SDN request error: %d\n", err);
			lnw_gpio_set_alt(RADIO_SDN, saved_muxing);
		}
		gpio_direction_output(RADIO_SDN, 1);

	} else {
		printk(KERN_ALERT "ERROR! invalid pin RADIO_SDN\n");
	}

	/* Request Chip Select gpios */

	if (gpio_is_valid(CS_SELF)) {
		saved_muxing = gpio_get_alt(CS_SELF);
		lnw_gpio_set_alt(CS_SELF, LNW_GPIO);
		err = gpio_request_one(CS_SELF,
				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio CS pin");
		if (err) {
			printk(KERN_ALERT "ERROR! CS_SELF request error: %d\n", err);
			lnw_gpio_set_alt(CS_SELF, saved_muxing);
		}
		gpio_direction_output(CS_SELF, 1);
	} else {
		printk(KERN_ALERT "ERROR! invalid pin CS_SELF\n");
	}

	return status;
}

static int spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{},
};

MODULE_DEVICE_TABLE(of, spidev_dt_ids);

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
		.of_match_table = of_match_ptr(spidev_dt_ids),
	},
	.probe =	spidev_probe,
	.remove =	spidev_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};


/*
 * si4463_rx,recieves a network packet and put the packet into TCP/IP up
 * layer,netif_rx() is the kernel API to do such thing. The recieving
 * procedure must alloc the sk_buff structure to store the data,
 * and the sk_buff will be freed in the up layer.
 */

void si4463_rx(struct net_device *dev, int len, unsigned char *buf)
{
	//printk("si4463_rx\n");

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
	if (ret)
		return ret;
    printk("request_irq ok\n");

    //getCTS();

	netif_start_queue(dev);

	gpio_set_value(RADIO_SDN, 0);//enable
	mdelay(10);
	//getCTS();
	reset();
	si4463_init();
	fifo_reset();
	enable_tx_interrupt();

	int tmp = 5;
//	u8 str[4] = {0xaa, 0xbb, 0xcc, 0xdd};
	const unsigned char tx_ph_data[19] = {'a','b','c','d','e',0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};
	printk(KERN_ALERT "Before: \n");
	get_fifo_info();
	mdelay(1000);
	for(;tmp>0; tmp--){
		mdelay(1000);

		spi_write_fifo(tx_ph_data, 19);
		printk(KERN_ALERT "Writing: \n");
		get_fifo_info();
		tx_start();
		printk(KERN_ALERT "Sending: \n");
		get_fifo_info();
	}


    return 0;
}

int si4463_release(struct net_device *dev)
{
	printk("si4463_release\n");
    netif_stop_queue(dev);
    gpio_set_value(RADIO_SDN, 1);//disable
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
	//printk("si4463_tx\n");
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

void si4463_net_init(struct net_device *dev)
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

/*-------------------------------------------------------------------------*/

static int __init spidev_init(void)
{
	int status, ret, result;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
		return PTR_ERR(spidev_class);
	}

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		class_destroy(spidev_class);
		unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	}

	/* Allocate the NET devices */
	si4463_devs = alloc_netdev(sizeof(struct si4463_priv), "rf%d", si4463_net_init);
	if (si4463_devs == NULL)
		goto out;


	ret = -ENODEV;
	if ((result = register_netdev(si4463_devs)))
		printk("demo: error %i registering device \"%s\"\n",result, si4463_devs->name);
	else
		ret = 0;
out:
	return status;
}
module_init(spidev_init);

static void __exit spidev_exit(void)
{
	gpio_set_value(RADIO_SDN, 1);//disable

	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);

	gpio_free(RADIO_SDN);
	gpio_free(CS_SELF);
	if (si4463_devs)
	{
		unregister_netdev(si4463_devs);
		free_netdev(si4463_devs);
	}
	return;
}
module_exit(spidev_exit);

#endif

MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
