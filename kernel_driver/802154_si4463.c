/*
 * Driver for Microchip MRF24J40 802.15.4 Wireless-PAN Networking controller
 *
 * Copyright (C) 2012 Alan Ott <alan@signal11.us>
 *                    Signal 11 Software
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

#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <net/wpan-phy.h>
#include <net/mac802154.h>
#include <net/ieee802154.h>
#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <linux/irq.h>

#include "802154-si4463.h"
#include "si4463/si4463_api.h"

//for mac_cb debug
#include <net/ieee802154_netdev.h>

/* GLOBAL */
struct {
	u8* data;
	u8 len;
	int position;
} data_sending;

u8 d[10];
u8 padding[MAX_FIFO_SIZE];

struct net_device *global_net_devs;
struct spi_device *spi_save;
struct spidev_data spidev_global;


/* PIN MUX */
#define pin_mux_num  31
const struct gpio pin_mux[pin_mux_num] = {
		/*{111, NULL, NULL},*/
		{115, NULL, NULL},
		{114, NULL, NULL},
		{109, NULL, NULL},
		{214, GPIOF_INIT_LOW, NULL},

		{263, GPIOF_INIT_HIGH, NULL},
		{240, GPIOF_INIT_HIGH, NULL},
		{262, GPIOF_INIT_HIGH, NULL},
		{241, GPIOF_INIT_HIGH, NULL},
		{242, GPIOF_INIT_HIGH, NULL},
		{243, GPIOF_INIT_HIGH, NULL},
		{258, GPIOF_INIT_HIGH, NULL},
		{259, GPIOF_INIT_HIGH, NULL},
		{260, GPIOF_INIT_LOW, NULL},
		{261, GPIOF_INIT_HIGH, NULL},
		{226, GPIOF_DIR_IN, NULL},
		{227, GPIOF_DIR_IN, NULL},
		{228, GPIOF_DIR_IN, NULL},
		{229, GPIOF_DIR_IN, NULL},
		/*IO9*/
		{257, GPIOF_INIT_LOW, NULL},
		{225, GPIOF_DIR_IN, NULL},
		{183, GPIOF_DIR_IN, NULL},

		/*IO7 and IO8*/
		{256, GPIOF_INIT_HIGH, NULL},
		{224, GPIOF_DIR_IN, NULL},
		{255, GPIOF_INIT_HIGH, NULL},
		{223, GPIOF_DIR_IN, NULL},

		/*IO6*/
		{254, GPIOF_INIT_LOW, NULL},
		{222, GPIOF_DIR_IN, NULL},
		{182, GPIOF_DIR_IN, NULL},

		/*IO5*/
		{253, GPIOF_INIT_LOW, NULL},
		{221, GPIOF_DIR_IN, NULL},
		{13, GPIOF_DIR_IN, NULL}
};

static void write2file(struct file *fp, const char *write_str, int len) {
    static char buf1[10];
    mm_segment_t fs;
    loff_t pos;

    if( IS_ERR(fp)) {
    	printk(KERN_ALERT "fp is NULL!!\n");
    	return;
    }
    if(write_str == NULL) {
    	printk(KERN_ALERT "write_str is NULL!!\n");
    	return;
    }
//    printk(KERN_ALERT "Len: %d, write_str is %s\n", len, write_str);
	fs =get_fs();
    set_fs(KERNEL_DS);
    pos = 0;
    vfs_write(fp, write_str, len, &pos);
//    pos = 0;
//    vfs_read(fp, buf1, sizeof(buf1), &pos);
//    printk(KERN_ALERT "%s\n",buf1);

    set_fs(fs);
}

int set_pinmux(){


    struct file *fp;
//    struct file *fp_214;

    const char s_mode0[] = "mode0";
    const char s_mode1[] = "mode1";
    const char s_low[] = "low";
    const char s_high[] = "high";
    const char s_in[] = "in";
    const char s_on[] = "on";

    int i,ret;

    ret = gpio_request_array(pin_mux, pin_mux_num);
    printk(KERN_ALERT "gpio_request_array return %d\n", ret);
//
	ret = gpio_export(109, 1);
	ret = gpio_export(111, 1);
	ret = gpio_export(114, 1);
	ret = gpio_export(115, 1);
	ret = gpio_export(263, 1);
	ret = gpio_export(240, 1);
	ret = gpio_export(262, 1);
	ret = gpio_export(241, 1);
	ret = gpio_export(242, 1);
	ret = gpio_export(243, 1);
	ret = gpio_export(258, 1);
	ret = gpio_export(259, 1);
	ret = gpio_export(260, 1);
	ret = gpio_export(261, 1);
	ret = gpio_export(226, 1);
	ret = gpio_export(227, 1);
	ret = gpio_export(228, 1);
	ret = gpio_export(229, 1);
	ret = gpio_export(214, 1);
	/* IO9 */
	ret = gpio_export(183, 1);
	ret = gpio_export(257, 1);
	ret = gpio_export(225, 1);
	/* IO7,8 */
	ret = gpio_export(256, 1);
	ret = gpio_export(224, 1);
	ret = gpio_export(255, 1);
	ret = gpio_export(223, 1);
	/* IO6 */
	ret = gpio_export(182, 1);
	ret = gpio_export(254, 1);
	ret = gpio_export(222, 1);

	/* IO5 */
	ret = gpio_export(13, 1);
	ret = gpio_export(253, 1);
	ret = gpio_export(221, 1);

    /* gpio export */
//    fp = filp_open("/sys/class/gpio/export", O_WRONLY, 0);
//    for (i=0; i<pin_mux_num; i++) {
////    	printk(KERN_ALERT "gpio_pin[%d] is %s\n", i, gpio_pin[i]);
//    	write2file(fp, gpio_pin[i], sizeof(gpio_pin[i]));
//    }

    /* disable spi PM */
    fp = filp_open("/sys/devices/pci0000:00/0000:00:07.1/power/control", O_RDWR, 0);
    write2file(fp, s_on, 2);
	/* TRI_STATE_ALL */
    fp = filp_open("/sys/class/gpio/gpio214/direction", O_RDWR, 0);
    write2file(fp, s_low, 3);

		/* IO9 Input for si4463(GPIO)*/
		fp = filp_open("/sys/kernel/debug/gpio_debug/gpio183/current_pinmux", O_RDWR, 0);
		write2file(fp, s_mode0, 5);
		fp = filp_open("/sys/class/gpio/gpio257/direction", O_RDWR, 0);
		write2file(fp, s_low, 3);
		fp = filp_open("/sys/class/gpio/gpio225/direction", O_RDWR, 0);
		write2file(fp, s_in, 2);
		fp = filp_open("/sys/class/gpio/gpio183/direction", O_RDWR, 0);
		write2file(fp, s_in, 2);


	/* IO7,8 */
    fp = filp_open("/sys/class/gpio/gpio256/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio255/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio223/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio224/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    /* IO6 */
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio182/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode0, 5);
    fp = filp_open("/sys/class/gpio/gpio254/direction", O_RDWR, 0);
    write2file(fp, s_low, 3);
    fp = filp_open("/sys/class/gpio/gpio222/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio182/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    /* IO5 */
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio13/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode0, 5);
    fp = filp_open("/sys/class/gpio/gpio253/direction", O_RDWR, 0);
    write2file(fp, s_low, 3);
    fp = filp_open("/sys/class/gpio/gpio221/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio13/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);

    /* SPI */
    fp = filp_open("/sys/class/gpio/gpio263/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio240/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio262/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio241/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio242/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio243/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio258/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio259/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio260/direction", O_RDWR, 0);
    write2file(fp, s_low, 3);
    fp = filp_open("/sys/class/gpio/gpio261/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio226/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio227/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio228/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio229/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio109/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode1, 5);
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio111/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode1, 5);
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio114/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode1, 5);
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio115/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode1, 5);
	/* TRI_STATE_ALL */
    fp = filp_open("/sys/class/gpio/gpio214/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);

    filp_close(fp,NULL);

	return 0;

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

ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
//	gpio_set_value(SSpin, 0);
	//ndelay(100);

	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = spidev_complete;
	message->context = &done;

//	spin_lock_irq(&spidev->spi_lock);
//	down(&sem_spi);
//	mutex_lock(&mutex_spi);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
//	spin_unlock_irq(&spidev->spi_lock);
//	up(&sem_spi);
//	mutex_unlock(&mutex_spi);

	if (status == 0) {
//		wait_for_completion(&done);
		/*
		* Return: 0 if timed out, and positive (at least 1, or number of jiffies left
		* till timeout) if completed.
		*/
		status = wait_for_completion_timeout(&done, HZ);
		if(status == 0){
			printk(KERN_ALERT "*********************SPI TIMEOUT ERROR*****************\n");
		}
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}


//	gpio_set_value(SSpin, 1);
	return status;
}

inline ssize_t
spidev_sync_transfer(struct spidev_data *spidev, u8 *tx_buf, u8 *rx_buf, size_t len)
{
//	mutex_lock(&mutex_spi);
	ssize_t ret;
	struct spi_transfer	t = {
			.tx_buf		= tx_buf,
			.rx_buf		= rx_buf,
			.len		= len,
			.cs_change	= 0,
			.bits_per_word = 0,
			.delay_usecs = 0,
			.speed_hz = 0
		};
	struct spi_message	m;
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

//	ret = spi_sync_locked(spidev, &m);
	ret = spidev_sync(spidev, &m);
//	spi_message_add_tail(&t, &spi_message_global);
//	ret = spidev_sync(spidev, &spi_message_global);
//	mutex_unlock(&mutex_spi);

	return ret;
}

inline ssize_t
spidev_sync_write(struct spidev_data *spidev,  size_t len)
{
//	mutex_lock(&mutex_spi);
	ssize_t ret;
	struct spi_transfer	t = {
			.tx_buf		= spidev->buffer,
			.len		= len,
			.cs_change	= 0,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spidev_sync(spidev, &m);
//	mutex_unlock(&mutex_spi);
	return ret;
}

inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
//	mutex_lock(&mutex_spi);
	ssize_t ret;
	struct spi_transfer	t = {
			.rx_buf		= spidev->buffer,
			.len		= len,
			.cs_change	= 0,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	ret = spidev_sync(spidev, &m);
//	mutex_unlock(&mutex_spi);
	return ret;
}


/*-------------------------------------------------------------------------*/
void ppp(u8 * arr, int len){
	int i = 0;
	printk(KERN_ALERT "ppp: len=[%d]\n", len);
	for(i = 0;i<len;i++)
		printk(KERN_ALERT "%x ", arr[i]);
	printk(KERN_ALERT "\n");
}

/* Device Private Data */
struct si4463 {
	struct spi_device *spi;
	struct ieee802154_dev *dev;

	struct mutex buffer_mutex; /* only used to protect buf */
	struct completion tx_complete;
	struct work_struct irqwork;
	u8 *buf; /* 3 bytes. Used for SPI single-register transfers. */
};

#define printdev(X) (&X->spi->dev)


inline static void irq_rx(/*void *dev_id*/){

}


static int si4463_tx(struct ieee802154_dev *dev, struct sk_buff *skb)
{
	struct si4463 *devrec = dev->priv;
	u8 val;
	int ret = 0;
	int i, rssi;

//	dev_dbg(printdev(devrec), "tx packet of %d bytes\n", skb->len);

again:
	for (i = 0; i < 5; i++)
	{
		rssi = get_CCA();
		if (rssi) {
			ndelay(1000);
			goto again;
		}
		ndelay(95);
	}


	change_state2tx_tune();
	data_sending.len = skb->len;
	data_sending.data = skb->data;
//	if (data_sending.len > 40)
//	{
//		data_sending.data[0] = 0x18;
//		data_sending.data[1] = 0x02;
//		data_sending.data[2] = 0x03;
//		data_sending.data[3] = 0x04;
//		data_sending.data[4] = 0x05;
//		data_sending.data[5] = 0x06;
//	}

	INIT_COMPLETION(devrec->tx_complete);

//	printk(KERN_ALERT "Len: %d\n", data_sending.len);
	fifo_reset();


	data_sending.len = data_sending.len > MAX_FIFO_SIZE ? MAX_FIFO_SIZE : data_sending.len;
	d[0] = data_sending.len;
	tx_set_packet_len(data_sending.len + 1);
	spi_write_fifo(d, 1);
	spi_write_fifo(data_sending.data, data_sending.len);	//tmp_len);
//	if ((data_sending.len + 1) < MAX_FIFO_SIZE) {
//		spi_write_fifo(padding, MAX_FIFO_SIZE - data_sending.len - 1);
//	}

//	int fp = filp_open("/home/root/log", O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
//	int time_s = jiffies_to_msecs(jiffies);

	while(gpio_get_value(NIRQ) <=0 ) {
		printk(KERN_ALERT "Before tx, NIRQ PIN: %d\n", gpio_get_value(NIRQ));
		clr_interrupt();
	}


	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 7 * HZ);
//	int time_e = jiffies_to_msecs(jiffies);
//	write2file(fp, time_s - time_e, sizeof(int));
//	write2file(fp, "\n", 1);
////			printk(KERN_ALERT "SEND_CMD:LEN: cmd_->len[%d]\n", tmp_len);
//			//semaphore
////			down(&sem_tx_complete);
//			ret = down_timeout(&sem_tx_complete, 2*HZ);
/* FREE THE SKB */
//			dev_kfree_skb(skb);

//	get_interrupt_status(d);
//	ppp(d, 9);
//	get_fifo_info(d);
//	ppp(d, 3);
out_normal:
//			if(ret == 0)
	if (ret > 0)	//wait_for_completion_interruptible_timeout
//		clr_packet_sent_pend();
		clr_interrupt();
	else
	{
		printk(KERN_ALERT "!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");
		printk(KERN_ALERT "NIRQ PIN: %d\n", gpio_get_value(NIRQ));
		reset();
	}
//	rx_start();
//	u8 status = get_device_state();
//	printk(KERN_ALERT "DEVICE STATUS AFTER TX: %x\n", status);
    return 0;
}

static int si4463_ed(struct ieee802154_dev *dev, u8 *level)
{
	/* TODO: */
	printk(KERN_WARNING "si4463: ed not implemented\n");
	*level = 0;
	return 0;
}

static int si4463_start(struct ieee802154_dev *dev)
{
	struct si4463 *devrec = dev->priv;
	u8 val;
	int ret;
	int saved_muxing = 0;
	int err,tmp;

	dev_dbg(printdev(devrec), "start\n");



	rx_start();
	printk(KERN_ALERT "rx_start\n");

	printk(KERN_ALERT "si4463_start\n");

	return 0;
}

static void si4463_stop(struct ieee802154_dev *dev)
{
	struct si4463 *devrec = dev->priv;
	u8 val;
	int ret;
	dev_dbg(printdev(devrec), "stop\n");

    gpio_set_value(RADIO_SDN, 1);//disable
    free_irq(gpio_to_irq(NIRQ),global_net_devs);
    gpio_free_array(pin_mux, pin_mux_num);
	gpio_unexport(109);
	gpio_unexport(111);
	gpio_unexport(114);
	gpio_unexport(115);
	return;
}

static int si4463_set_channel(struct ieee802154_dev *dev,
				int page, int channel)
{
	struct si4463 *devrec = dev->priv;
	u8 val;
	int ret;

	dev_dbg(printdev(devrec), "Set Channel %d\n", channel);

//	WARN_ON(page != 0);
//	WARN_ON(channel < MRF24J40_CHAN_MIN);
//	WARN_ON(channel > MRF24J40_CHAN_MAX);
//
//	/* Set Channel TODO */
//	val = (channel-11) << 4 | 0x03;
//	write_long_reg(devrec, REG_RFCON0, val);
//
//	/* RF Reset */
//	ret = read_short_reg(devrec, REG_RFCTL, &val);
//	if (ret)
//		return ret;
//	val |= 0x04;
//	write_short_reg(devrec, REG_RFCTL, val);
//	val &= ~0x04;
//	write_short_reg(devrec, REG_RFCTL, val);
//
//	udelay(SET_CHANNEL_DELAY_US); /* per datasheet */

	return 0;
}






static inline void mac802154_haddr_copy_swap(u8 *dest, const u8 *src)
{
	int i;
	for (i = 0; i < IEEE802154_ADDR_LEN; i++)
		dest[IEEE802154_ADDR_LEN - i - 1] = src[i];
}
static inline int mac802154_fetch_skb_u8(struct sk_buff *skb, u8 *val)
{
	if (unlikely(!pskb_may_pull(skb, 1)))
		return -EINVAL;

	*val = skb->data[0];
	skb_pull(skb, 1);

	return 0;
}

static inline int mac802154_fetch_skb_u16(struct sk_buff *skb, u16 *val)
{
	if (unlikely(!pskb_may_pull(skb, 2)))
		return -EINVAL;

	*val = skb->data[0] | (skb->data[1] << 8);
	skb_pull(skb, 2);

	return 0;
}
static int mac802154_parse_frame_start(struct sk_buff *skb)
{
	u8 *head = skb->data;
	u16 fc;

	if (mac802154_fetch_skb_u16(skb, &fc) ||
	    mac802154_fetch_skb_u8(skb, &(mac_cb(skb)->seq)))
		goto err;

	printk(KERN_ALERT "fc: %04x dsn: %02x\n", fc, head[2]);

	mac_cb(skb)->flags = IEEE802154_FC_TYPE(fc);
	mac_cb(skb)->sa.addr_type = IEEE802154_FC_SAMODE(fc);
	mac_cb(skb)->da.addr_type = IEEE802154_FC_DAMODE(fc);

	if (fc & IEEE802154_FC_INTRA_PAN)
		mac_cb(skb)->flags |= MAC_CB_FLAG_INTRAPAN;

	if (mac_cb(skb)->da.addr_type != IEEE802154_ADDR_NONE) {
		if (mac802154_fetch_skb_u16(skb, &(mac_cb(skb)->da.pan_id)))
			goto err;

		/* source PAN id compression */
		if (mac_cb_is_intrapan(skb))
			mac_cb(skb)->sa.pan_id = mac_cb(skb)->da.pan_id;

		printk(KERN_ALERT "dest PAN addr: %04x\n", mac_cb(skb)->da.pan_id);

		if (mac_cb(skb)->da.addr_type == IEEE802154_ADDR_SHORT) {
			u16 *da = &(mac_cb(skb)->da.short_addr);

			if (mac802154_fetch_skb_u16(skb, da))
				goto err;

			printk(KERN_ALERT "destination address is short: %04x\n",
				 mac_cb(skb)->da.short_addr);
		} else {
			if (!pskb_may_pull(skb, IEEE802154_ADDR_LEN))
				goto err;

			mac802154_haddr_copy_swap(mac_cb(skb)->da.hwaddr,
						  skb->data);
			skb_pull(skb, IEEE802154_ADDR_LEN);

			printk(KERN_ALERT "destination address is hardware\n");
		}
	}

	if (mac_cb(skb)->sa.addr_type != IEEE802154_ADDR_NONE) {
		/* non PAN-compression, fetch source address id */
		if (!(mac_cb_is_intrapan(skb))) {
			u16 *sa_pan = &(mac_cb(skb)->sa.pan_id);

			if (mac802154_fetch_skb_u16(skb, sa_pan))
				goto err;
		}

		printk(KERN_ALERT "source PAN addr: %04x\n", mac_cb(skb)->da.pan_id);

		if (mac_cb(skb)->sa.addr_type == IEEE802154_ADDR_SHORT) {
			u16 *sa = &(mac_cb(skb)->sa.short_addr);

			if (mac802154_fetch_skb_u16(skb, sa))
				goto err;

			printk(KERN_ALERT "source address is short: %04x\n",
				 mac_cb(skb)->sa.short_addr);
		} else {
			if (!pskb_may_pull(skb, IEEE802154_ADDR_LEN))
				goto err;

			mac802154_haddr_copy_swap(mac_cb(skb)->sa.hwaddr,
						  skb->data);
			skb_pull(skb, IEEE802154_ADDR_LEN);

			printk(KERN_ALERT "source address is hardware\n");
		}
	}

	return 0;
err:
	return -EINVAL;
}











static int si4463_handle_rx(struct si4463 *devrec)
{
	u8 len = 0;
	u8 lqi = 0; //lqi is rssi.
	u8 val;
	int ret = 0;
	struct sk_buff *skb;


	len = get_packet_info();
//	printk(KERN_ALERT "RECV LEN: %d\n", len);
//	get_fifo_info(d);
//	ppp(d, 3);
	skb = alloc_skb(len, GFP_KERNEL);
	if (!skb) {
		ret = -ENOMEM;
		goto out;
	}
	spi_read_fifo(skb_put(skb, len), len);
//	ppp(skb->data, len);
//    //get data from hardware register
//	printk(KERN_ALERT "head: %x, data: %x\n", skb->head, skb->data);
//	mac802154_parse_frame_start(skb);
//	printk(KERN_ALERT "da.addr_type: %x, da.pan_id: %x, da.short_addr: %x\n",
//			mac_cb(skb)->da.addr_type, mac_cb(skb)->da.pan_id, mac_cb(skb)->da.short_addr);


	fifo_reset();
	clr_interrupt();
	rx_start();

	ieee802154_rx_irqsafe(devrec->dev, skb, lqi);
//	clr_packet_rx_pend();


//	/* Turn off reception of packets off the air. This prevents the
//	 * device from overwriting the buffer while we're reading it. */
//	ret = read_short_reg(devrec, REG_BBREG1, &val);
//	if (ret)
//		goto out;
//	val |= 4; /* SET RXDECINV */
//	write_short_reg(devrec, REG_BBREG1, val);
//
//	skb = alloc_skb(len, GFP_KERNEL);
//	if (!skb) {
//		ret = -ENOMEM;
//		goto out;
//	}
//
//	ret = si4463_read_rx_buf(devrec, skb_put(skb, len), &len, &lqi);
//	if (ret < 0) {
//		dev_err(printdev(devrec), "Failure reading RX FIFO\n");
//		kfree_skb(skb);
//		ret = -EINVAL;
//		goto out;
//	}
//
//	/* Cut off the checksum */
//	skb_trim(skb, len-2);
//
//	/* TODO: Other drivers call ieee20154_rx_irqsafe() here (eg: cc2040,
//	 * also from a workqueue).  I think irqsafe is not necessary here.
//	 * Can someone confirm? */
//	ieee802154_rx_irqsafe(devrec->dev, skb, lqi);
//
//	dev_dbg(printdev(devrec), "RX Handled\n");
//
//out:
//	/* Turn back on reception of packets off the air. */
//	ret = read_short_reg(devrec, REG_BBREG1, &val);
//	if (ret)
//		return ret;
//	val &= ~0x4; /* Clear RXDECINV */
//	write_short_reg(devrec, REG_BBREG1, val);
out:
	return ret;
}

static struct ieee802154_ops si4463_ops = {
	.owner = THIS_MODULE,
	.xmit = si4463_tx,
	.ed = si4463_ed,
	.start = si4463_start,
	.stop = si4463_stop,
	.set_channel = si4463_set_channel,
};

static irqreturn_t si4463_isr(int irq, void *data)
{
	struct si4463 *devrec = data;
//	printk(KERN_ALERT "=====IRQ=====\n");
	disable_irq_nosync(irq);

	schedule_work(&devrec->irqwork);

	return IRQ_HANDLED;
}

static void si4463_isrwork(struct work_struct *work)
{
	struct si4463 *devrec = container_of(work, struct si4463, irqwork);
	u8 intstat;
	int ret;
	int preamble_detect;
	bool tx_complete_flag;
	bool rx_flag;
	bool tx_fifo_almost_empty_flag;
	u8 ph_pend;
	u8 md_status;

	preamble_detect = 0;
	tx_complete_flag = 0;
	tx_fifo_almost_empty_flag = 0;
	rx_flag = 0;

	/*
	 * read FRR_A for PH_STATUS_PEND (or status????)
	 * 1, tx complete
	 * 2, rx
	 */
	ph_pend = read_frr_a();
//	u8 status = get_device_state();
//	printk(KERN_ALERT "PH_PEND: %x\n", ph_pend);
//	printk(KERN_ALERT "DEVICE STATUS: %x\n", status);
//		if(ph_pend==0 || ph_pend==0xff)
//			goto error;

	/*
	 * read FRR_B for md_status
	 * this can be combined with frr_a
	 */
//		read_frr_b(&md_status);
//		printk(KERN_ALERT "md_status: %x\n", md_status);
//
//		if((md_status==0 || md_status==0xff) && (ph_pend==0 || ph_pend==0xff))
//			goto error;

	//Check CRC error
	//...
	//

	//Check preamble
//		if((md_status & 0x02) == 0x02) {
//			preamble_detect = 1;
//			clr_preamble_detect_pend();
//		}
	//Only TX_FIFO_ALMOST_EMPTY interrupt happen. And no PACKET_SENT interrupt happen.
	if((ph_pend & 0x22) == 0x02) {
//		irq_fifo_almost_empty();
		tx_fifo_almost_empty_flag = 1;
		goto out;
	}
	//Check Tx complete
	if((ph_pend & 0x22) == 0x22 || (ph_pend & 0x22) == 0x20) {
//		irq_tx_complete();
		complete(&devrec->tx_complete);
		tx_complete_flag = 1;
		goto out;
	}

	if((ph_pend & 0x10) == 0x10) {
		si4463_handle_rx(devrec);
		rx_flag = 1;
		preamble_detect = 0;
		goto out;
	}



//		printk(KERN_ALERT "tx_complete_flag: %d, isSending: %d\n",tx_complete_flag, isSending);
//	if (!tx_complete_flag && !tx_fifo_almost_empty_flag) {
//		/* *
//		 * recv a packet when a sending is wait for complete.
//		 * the tx send complete irq	will be vanished
//		 * */
//		/*
//		 * SEND ERROR!!!!/Clear IRQ, rx process will clear irq.
//	if(!rx_flag && !tx_complete_flag && !preamble_detect && !tx_fifo_almost_empty_flag){
//		printk(KERN_ALERT "ERROR PH_PEND: %x\n", ph_pend);
//		clr_interrupt();
//	}
//		 */
//		printk(KERN_ALERT "!!!!SEND ERROR!!!!\n");
//		tx_fifo_reset();
////		irq_tx_complete();
//		rx_start();
//	}
//		write_int_with_lock(&isHandlingIrq, 0);

error:
	//Clear IRQ, rx process will clear irq.
	if(!rx_flag && !tx_complete_flag && !preamble_detect && !tx_fifo_almost_empty_flag){
		printk(KERN_ALERT "ERROR PH_PEND: %x\n", ph_pend);
		u8* rx[10];
		get_ph_status(rx);
		ppp(rx, 3);
		get_interrupt_status(rx);
		ppp(rx, 9);
		clr_interrupt();
	}

out:
	enable_irq(devrec->spi->irq);
}

static int si4463_probe(struct spi_device *spi)
{
	int tmp;
	int irq;
	int ret = -ENOMEM;
	u8 val;
	int err;
	struct si4463 *devrec;
	struct pinctrl *pinctrl;
	printk(KERN_ALERT "si4463: probe(). IRQ: %d\n", spi->irq);

	devrec = kzalloc(sizeof(struct si4463), GFP_KERNEL);
	if (!devrec)
		goto err_devrec;
	devrec->buf = kzalloc(3, GFP_KERNEL);
	if (!devrec->buf)
		goto err_buf;

	spi_save = spi;
	spidev_global.spi = spi;
	spin_lock_init(&spidev_global.spi_lock);
//	spin_lock_init(&isHandlingIrq.lock);
//	isHandlingIrq.data = 0;
	mutex_init(&spidev_global.buf_lock);

	/* GPIO setup
	 *
	 */
	/* Request Chip SDN gpios */
	err = gpio_request(RADIO_SDN,  "Radio SDN pin");
	if (err) {
		printk(KERN_ALERT "ERROR! RADIO_SDN request error: %d\n", err);
	}

	/* Request Chip Select gpios */
	err = gpio_request(SSpin,  "Radio CS pin");
	if (err) {
		printk(KERN_ALERT "ERROR! SSpin request error: %d\n", err);
//			lnw_gpio_set_alt(SSpin, saved_muxing);
	}
	gpio_direction_output(SSpin, 1);

	/* PIN MUX */
	set_pinmux();

	/* spi setup :
	 * 		SPI_MODE_0
	 * 		MSBFIRST
	 * 		CS active low
	 * 		IRQ??????
	 */
	if (spi == NULL)
		return -ESHUTDOWN;

	tmp = ~SPI_MODE_MASK;

	printk(KERN_ALERT "init: tmp= %x\n", tmp);
	tmp |= SPI_MODE_0;
	tmp |= SPI_NO_CS;

	//tmp |= SPI_CS_HIGH;
	//tmp |= SPI_READY;
	printk(KERN_ALERT "midd: tmp= %x\n", tmp);
	tmp |= spi->mode & ~SPI_MODE_MASK;

	printk(KERN_ALERT "after: tmp= %x\n", tmp);
//	spi->mode = (u8)tmp;
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = BITS_PER_WORD;
	spi->max_speed_hz = SPI_SPEED;
	spi->mode &= ~SPI_LSB_FIRST;

	ret = spi_setup(spi);
	if (ret < 0)
		printk(KERN_ALERT "ERROR! spi_setup return: %d\n", ret);
	else
		printk(KERN_ALERT "spi_setup succeed, spi:%x, spidev_global.spi:%x\n", spi, spidev_global.spi);


	mutex_init(&devrec->buffer_mutex);
	init_completion(&devrec->tx_complete);
	INIT_WORK(&devrec->irqwork, si4463_isrwork);
	devrec->spi = spi;
	spi_set_drvdata(spi, devrec);

	/* Register with the 802154 subsystem */

	devrec->dev = ieee802154_alloc_device(0, &si4463_ops);
	if (!devrec->dev)
		goto err_alloc_dev;

	devrec->dev->priv = devrec;
	devrec->dev->parent = &devrec->spi->dev;
	devrec->dev->phy->channels_supported[0] = 915;
	devrec->dev->flags = IEEE802154_HW_OMIT_CKSUM;//|IEEE802154_HW_AACK;

	dev_dbg(printdev(devrec), "registered si4463\n");
	ret = ieee802154_register_device(devrec->dev);
	if (ret)
		goto err_register_device;


	reset();
	clr_interrupt();

	/* IRQ */
	irq = gpio_to_irq(NIRQ);
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);
    ret = request_irq(irq, si4463_isr, IRQF_TRIGGER_FALLING,
    		dev_name(&spi->dev), devrec);
	if (ret) {
		dev_err(printdev(devrec), "Unable to get IRQ");
		goto err_irq;
	}
	devrec->spi->irq = irq;

	memset(padding, 0, MAX_FIFO_SIZE);

	return 0;

err_irq:
err_read_reg:
	ieee802154_unregister_device(devrec->dev);
err_register_device:
	ieee802154_free_device(devrec->dev);
err_alloc_dev:
	kfree(devrec->buf);
err_buf:
	kfree(devrec);
err_devrec:
	return ret;
}

static int si4463_remove(struct spi_device *spi)
{
	struct si4463 *devrec = spi_get_drvdata(spi);

	dev_dbg(printdev(devrec), "remove\n");

	free_irq(spi->irq, devrec);
	flush_work(&devrec->irqwork); /* TODO: Is this the right call? */
	ieee802154_unregister_device(devrec->dev);
	ieee802154_free_device(devrec->dev);
	/* TODO: Will ieee802154_free_device() wait until ->xmit() is
	 * complete? */

	/* Clean up the SPI stuff. */
	spi_set_drvdata(spi, NULL);
	kfree(devrec->buf);
	kfree(devrec);
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
	.probe =	si4463_probe,
	.remove =	si4463_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};



module_spi_driver(spidev_spi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wu");
MODULE_DESCRIPTION("SI4463 SPI 802.15.4 Controller Driver");
