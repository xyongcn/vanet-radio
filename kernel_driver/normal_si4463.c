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


#include "radio.h"
#include "si4463/si4463_api.h"
#include "slots.h"

//for mac_cb debug
//#include <net/ieee802154_netdev.h>

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

static unsigned int global_device_id = -1;
module_param(global_device_id, uint, S_IRUGO);
MODULE_PARM_DESC(global_device_id, "device_id");
/************************ GLOBAL *******************************/
/* for time slots */
struct mutex global_mutex_slots[SLOTS_NUM];
struct slot global_slots_table[SLOTS_NUM];
static int global_slots_count = -1;
static enum status global_device_status = REQUEST_BCH;

/******************/
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
struct si4463 * global_devrec;

DEFINE_MUTEX(mutex_spi);
wait_queue_head_t wait_withdraw;
/* TX withdraw timer */
struct timer_list tx_withdraw_timer;
u8 tx_approved = 1;
static void withdraw(unsigned long data)
{
	tx_approved = 1;
//    printk(KERN_ALERT "tx is now approved\n");
	wake_up_interruptible(&wait_withdraw);
}

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
    /* IO5 OUTPUT*/
    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio13/current_pinmux", O_RDWR, 0);
    write2file(fp, s_mode0, 5);
    fp = filp_open("/sys/class/gpio/gpio253/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);
    fp = filp_open("/sys/class/gpio/gpio221/direction", O_RDWR, 0);
    write2file(fp, s_in, 2);
    fp = filp_open("/sys/class/gpio/gpio13/direction", O_RDWR, 0);
    write2file(fp, s_high, 4);

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
//	mutex_lock(&devrec->mutex_spi);
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
//	struct ieee802154_dev *dev;
	struct net_device *dev;
	struct mutex buffer_mutex; /* only used to protect buf */
	struct completion tx_complete;
	struct work_struct irqwork;
	struct work_struct slotirqwork;

	u8 *buf; /* 3 bytes. Used for SPI single-register transfers. */

	struct mutex mutex_spi;

	/**
	 *  for tdma
	 */
	struct mutex *mutex_myslot;
	int mybch_number;
	u8 device_id;

	bool irq_busy;
	spinlock_t lock;
};

#define printdev(X) (&X->spi->dev)



struct xmit_work {
	struct sk_buff *skb;
	struct work_struct work;
	struct module_priv *priv;
//	u8 chan;
//	u8 page;
};

static int si4463_tx_worker(struct work_struct *work)
{

	struct sk_buff *skb;
	struct xmit_work *xw = container_of(work, struct xmit_work, work);
	struct si4463 *devrec;
	devrec = xw->priv->spi_priv;
	skb = xw->skb;

	mutex_lock(&xw->priv->pib_lock);
	u8 val;
	int ret = 0;
	int i, rssi;

//	dev_dbg(printdev(devrec), "tx packet of %d bytes\n", skb->len);

//again:
//	for (i = 0; i < 5; i++)
//	{
//		rssi = get_CCA();
//		if (rssi) {
////			ndelay(1000);
//			tx_approved = 0;
//			add_timer(&tx_withdraw_timer);
//			wait_event_interruptible(wait_withdraw, tx_approved);
//			goto again;
//		}
//		ndelay(95);
//	}
again_irqbusy:
	spin_lock(&devrec->lock);
	if  (devrec->irq_busy) {
		printk(KERN_ALERT "TX: irq_busy!\n");
		spin_unlock(&devrec->lock);
//		return -EBUSY;
		tx_approved = 0;
		add_timer(&tx_withdraw_timer);
		wait_event_interruptible(wait_withdraw, tx_approved);
		goto again_irqbusy;
	}
	spin_unlock(&devrec->lock);
//	printk(KERN_ALERT "tx: %d\n", skb->len);

	if(skb->len > MAX_FIFO_SIZE)
		printk(KERN_ALERT "tx > MAX_FIFO_SIZE!!!! %d\n", skb->len);

	/* time slot */
	mutex_lock(devrec->mutex_myslot);

	change_state2tx_tune();
	while(gpio_get_value(NIRQ) <=0 ) {
		printk(KERN_ALERT "Before tx, NIRQ PIN: %d\n", gpio_get_value(NIRQ));
		spin_lock(&devrec->lock);
		if  (devrec->irq_busy) {
			printk(KERN_ALERT "TX222: irq_busy!\n");
			rx_start();
		}
		spin_unlock(&devrec->lock);
		clr_interrupt();
	}

	data_sending.len = skb->len;
	data_sending.data = skb->data;

	INIT_COMPLETION(devrec->tx_complete);

//	printk(KERN_ALERT "Len: %d\n", data_sending.len);
	fifo_reset();


	data_sending.len = data_sending.len > MAX_FIFO_SIZE ? MAX_FIFO_SIZE : data_sending.len;
	d[0] = data_sending.len;
	/* for tdma */
	d[1] = 0x00;
//	tx_set_packet_len(data_sending.len + 1);
	tx_set_packet_len(data_sending.len + 2);
	spi_write_fifo(d, 2);
	spi_write_fifo(data_sending.data, data_sending.len);	//tmp_len);
//	if ((data_sending.len + 1) < MAX_FIFO_SIZE) {
//		spi_write_fifo(padding, MAX_FIFO_SIZE - data_sending.len - 1);
//	}

//	int fp = filp_open("/home/root/log", O_CREAT|O_APPEND|O_RDWR, S_IRWXU);
//	int time_s = jiffies_to_msecs(jiffies);

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
	rx_start();
//	u8 status = get_device_state();
//	printk(KERN_ALERT "DEVICE STATUS AFTER TX: %x\n", status);

	mutex_unlock(&xw->priv->pib_lock);
	netif_wake_queue(global_net_devs);
	dev_kfree_skb(skb);
	kfree(xw);
//	printk(KERN_ALERT "si4463_tx_worker out!\n");
    return 0;
}


static int si4463_tx(struct sk_buff *skb, struct net_device *dev)
{
//	printk(KERN_ALERT "si4463_tx! dev:%x glo:%x\n", dev, global_net_devs);
	struct xmit_work *work;
	struct module_priv *priv;
	priv = netdev_priv(dev);

	work = kzalloc(sizeof(struct xmit_work), GFP_ATOMIC);
	if (!work) {
		kfree_skb(skb);
		return NETDEV_TX_BUSY;
	}
//	if (global_device_status != WORKING) {
//		kfree_skb(skb);
//		return NETDEV_TX_BUSY;
//	}

	netif_stop_queue(dev);
//
//
	INIT_WORK(&work->work, si4463_tx_worker);
	work->skb = skb;
	work->priv = priv;
	queue_work(priv->dev_workqueue, &work->work);

//	printk(KERN_ALERT "si4463_tx out!\n");
	return NETDEV_TX_OK;
}


struct rx_work {
	struct sk_buff *skb;
	struct work_struct work;
	struct net_device *dev;
};

static void si4463_handle_rx(struct work_struct *work)
{
	struct rx_work *rw = container_of(work, struct rx_work, work);
	struct sk_buff *skb = rw->skb;

    netif_rx(skb);

//	printk(KERN_ALERT "si4463_handle_rx out 2 \n");
	kfree(rw);

}

u8 tmprx[128];
void si4463_rx_irqsafe(struct net_device *dev)
{
	struct module_priv *priv;
	struct rx_work *work;

//	printk(KERN_ALERT "si4463_handle_rx in \n");
	u8 len = 0;
	u8 type;
	u8 val;
	int i;
	int mybch_relatednum;
	int ret = 0;
	struct sk_buff *skb;

	priv = netdev_priv(dev);
	len = get_packet_info();
//	printk(KERN_ALERT "RECV LEN: %d\n", len);
//	get_fifo_info(d);
//	ppp(d, 3);
//	skb = alloc_skb(len, GFP_KERNEL);


	/* for tdma */
	spi_read_fifo(tmprx, len);

//	spi_read_fifo(skb_put(skb, len), len);
//	ppp(skb->data, len);

	fifo_reset();
	clr_interrupt();
	rx_start();

	switch(GET_TYPE(tmprx[0])) {
	case DATA_PACKET://data packet
	    skb = dev_alloc_skb(len+2);
	    if (!skb) {
	        printk(KERN_ALERT "si4463_rx can not allocate more memory to store the packet. drop the packet\n");
	        priv->stats.rx_dropped++;
	        return -1;
	    }
	    skb_reserve(skb, 2);
		if (!skb) {
			ret = -ENOMEM;
			return;
		}

		memcpy(skb_put(skb, len-1), &tmprx[1], len-1);
	    skb->dev = dev;
	    skb->protocol = eth_type_trans(skb, dev);
	    /* We need not check the checksum */
	    skb->ip_summed = CHECKSUM_UNNECESSARY;
	    priv->stats.rx_packets++;

		work = kzalloc(sizeof(struct rx_work), GFP_ATOMIC);
		if (!work)
			return;

		INIT_WORK(&work->work, si4463_handle_rx);
		work->skb = skb;
		work->dev = dev;

		queue_work(priv->dev_workqueue, &work->work);
		break;
	case CONTROL_PACKET:
		switch(GET_CONTROL_TYPE(tmprx[0])){
		case FI:
			switch(global_device_status) {
			case REQUEST_BCH:
				/* listen */

				/* bch ==> priv->spi_priv->mybch_number */
//				break;

			case WORKING:
//				/* step 1: find my bch, if none, error handler.
//				 * tmprx[1] ==> tmprx[slots_number]
//				 * */
//				for (i=1, mybch_relatednum=-1; i<=SLOTS_NUM; i++) {
//					if(global_myid == GET_ID(tmprx[i])){
//						mybch_relatednum = i-1;
//						break;
//					}
//				}
//				if(mybch_relatednum == -1){
//					/* error handler */
//				}
//
//				/* step 2: update slots table */
				update_slot_table(tmprx+1, global_slots_table,
						global_slots_count,
						GET_ID(tmprx[0]));
			}

			break;
		case BCH_REQ:
			/* step 1: finding the slot on which we have received the packet. */
			switch(global_device_status) {
			case REQUEST_BCH:
			case WORKING:
				if(global_slots_table[global_slots_count].slot_status == available){
					global_slots_table[global_slots_count].owner_id = GET_ID(tmprx[0]);
					global_slots_table[global_slots_count].slot_status = reserve;
				} else {
					global_slots_table[global_slots_count].slot_status = collision;
				}

				break;

			case default:
				break;
			}
			break;
		case EXTRA_SLOTS_REQ:

			break;
		case default:
			printk(KERN_ALERT "CONTROL TYPE ERROR!!\n");

		}

		break;
	case default:
		printk(KERN_ALERT "TYPE ERROR!!\n");
	}

}

int si4463_release(struct net_device *dev)
{
	printk("si4463_release\n");
    netif_stop_queue(dev);
    gpio_set_value(RADIO_SDN, 1);//disable
    free_irq(gpio_to_irq(NIRQ),global_net_devs);
    gpio_free_array(pin_mux, pin_mux_num);
	gpio_unexport(109);
	gpio_unexport(111);
	gpio_unexport(114);
	gpio_unexport(115);

//	kthread_stop(cmd_handler_tsk);
//	kthread_stop(irq_handler_tsk);
    return 0;
}

/*
 * Deal with a transmit timeout.
 */
void si4463_tx_timeout (struct net_device *dev)
{
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_errors++;

    printk(KERN_ALERT "si4463_tx_timeout\n");
//    netif_wake_queue(dev);

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
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    return &priv->stats;
}

/*
 * TCP/IP handshake will call this function, if it need.
 */
int si4463_change_mtu(struct net_device *dev, int new_mtu)
{
    unsigned long flags;
    spinlock_t *lock = &((struct module_priv *) netdev_priv(dev)/*dev->priv*/)->lock;

    /* en, the mtu CANNOT LESS THEN 68 OR MORE THEN 1500. */
    if (new_mtu < 68)
        return -EINVAL;

    spin_lock_irqsave(lock, flags);
    dev->mtu = new_mtu;
    spin_unlock_irqrestore(lock, flags);

    return 0;
}
/*
static int sned_control_packet()
{

	struct si4463 *devrec;
	devrec = global_devrec;

	u8 val;
	int ret = 0;
	int i, rssi;

	INIT_COMPLETION(devrec->tx_complete);

	fifo_reset();


	tx_set_packet_len(data_sending.len + 1);
	spi_write_fifo(d, 1);
	spi_write_fifo(data_sending.data, data_sending.len);	//tmp_len);

	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 7 * HZ);

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
	rx_start();

	mutex_unlock(&xw->priv->pib_lock);
	netif_wake_queue(global_net_devs);
	dev_kfree_skb(skb);
	kfree(xw);
}
*/
static irqreturn_t slot_isr(int irq, void *data)
{
//	printk(KERN_ALERT "=====IRQ=====\n");
//	struct si4463 *devrec = data;

//	gpio_set_value(13, 0);

	schedule_work(&devrec->slotirqwork);

	return IRQ_HANDLED;
}

static void slot_isrwork(struct work_struct *work)
{
	struct si4463 *devrec;
	devrec = container_of(work, struct si4463, slotirqwork);
//	if( global_slots_count == (DEVICES_NUM - 1))

	/* initial with -1 */
	global_slots_count = (global_slots_count + 1) % SLOTS_NUM;
	if(mutex_is_locked(&global_mutex_slots[global_slots_count])) {
		mutex_unlock(&global_mutex_slots[global_slots_count]);
	}

	if (global_slots_count == devrec->mybch_number) {
		switch(global_device_status){
		/* This surely be the second round! */
		case WAITING_BCH:
			/* todo: check slot_table to see if we have the BCH */
			if(isEmpty(global_slots_table)) {
				/* there has no neighbor! */
				global_device_status = WORKING;
			} else if(global_slots_table[devrec->mybch_number].owner_id == devrec->device_id) {
				global_device_status = WORKING;
			} else {
				printk(KERN_ALERT "slot_isrwork: GET BCH ERROR!\n");
			}
			break;
		case WORKING:
			/* todo: send FI packet */
			/* todo: request extra slots */

		}
	}
//	gpio_set_value(13, 1);
//	printk(KERN_ALERT "=====IRQ=====\n");
}

static irqreturn_t si4463_isr(int irq, void *data)
{
//	printk(KERN_ALERT "=====IRQ=====\n");
	struct si4463 *devrec = data;


	disable_irq_nosync(irq);

	spin_lock(&devrec->lock);
	devrec->irq_busy = 1;
	spin_unlock(&devrec->lock);
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
	unsigned long flags;
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
		si4463_rx_irqsafe(global_net_devs);
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
	spin_lock_irqsave(&devrec->lock, flags);
	devrec->irq_busy = 0;
	spin_unlock_irqrestore(&devrec->lock, flags);
	enable_irq(devrec->spi->irq);
}

static int si4463_start(struct net_device *dev)
{


	int tmp_bch;
	/* GET MY SLOT! */
//	global_devrec->mutex_myslot = &mutex_slots[0];//test.
//	request_BCH();

	/* previously done in probe: listening for fi */
	/* step 1: find first available in slot table */
	tmp_bch = get_first_available_slot(global_slots_table);
	/* step 2: send bch request */
	send_bch_request(global_devrec, &global_mutex_slots[tmp_bch]);
	global_device_status = WAITING_BCH;
	global_devrec->mybch_number = tmp_bch;
	mutex_lock(&global_slots_table[global_devrec->mybch_number].lock);
	global_slots_table[global_devrec->mybch_number].slot_status = myslot;
	global_slots_table[global_devrec->mybch_number].owner_id = global_devrec->device_id;
	mutex_unlock(&global_slots_table[global_devrec->mybch_number].lock);

	netif_start_queue(dev);
	printk(KERN_ALERT "si4463_start\n");

	return 0;
}

static const struct net_device_ops si4463_netdev_ops = {

	.ndo_open            = si4463_start,
	.ndo_stop            = si4463_release,
	.ndo_start_xmit		 = si4463_tx,
	.ndo_do_ioctl        = si4463_ioctl,
	.ndo_get_stats       = si4463_stats,
	.ndo_change_mtu      = si4463_change_mtu,
	.ndo_tx_timeout      = si4463_tx_timeout,
};

static int si4463_probe(struct spi_device *spi)
{
	int tmp;
	int irq;
	int ret = -ENOMEM;
	u8 val;
	int err;
	int i;
	int irq, slot_irq;
	struct si4463 *devrec;

	struct pinctrl *pinctrl;
	printk(KERN_ALERT "si4463: probe(). IRQ: %d\n", spi->irq);

	devrec = kzalloc(sizeof(struct si4463), GFP_KERNEL);
	if (!devrec)
		goto err_devrec;

	global_devrec = devrec;

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
	mutex_init(&devrec->mutex_spi);
	spin_lock_init(&devrec->lock);

	for(i=0; i<=DEVICES_NUM; i++) {
		mutex_init(&global_mutex_slots[i]);
	}

	init_completion(&devrec->tx_complete);
	INIT_WORK(&devrec->irqwork, si4463_isrwork);
	INIT_WORK(&devrec->slotirqwork, slot_isrwork);
	devrec->spi = spi;
	spi_set_drvdata(spi, devrec);
	devrec->device_id = (u8)global_device_id & 0xff;
	devrec->mybch_number = -1;

	memset(padding, 0, MAX_FIFO_SIZE);

	/* setup timer */
	setup_timer(&tx_withdraw_timer, withdraw, 0);
	tx_withdraw_timer.expires = jiffies + HZ/1000; //jiffies + HZ/2 are 0.5s
	tx_withdraw_timer.function = withdraw;
	tx_withdraw_timer.data = 0;

	/* Waiting Queue */
	init_waitqueue_head(&wait_withdraw);

	/* IRQ */
	irq = gpio_to_irq(NIRQ);
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);
    ret = request_irq(irq, si4463_isr, IRQF_TRIGGER_FALLING,
//    		dev->name, dev);
    		"si4463", global_devrec);
	if (ret) {
//		dev_err(printdev(global_devrec), "Unable to get IRQ");
		printk("IRQ REQUEST ERROR!\n");
	}
	devrec->spi->irq = irq;

	/* TIME SLOTS */
//	slots_manager_tsk = kthread_run(irq_handler, NULL, "slots manager");
//	if (IS_ERR(slots_manager_tsk)) {
//		printk(KERN_ALERT "create kthread slots_manager_tsk failed!\n");
//	} else {
//		printk("create ktrhead irq_handler ok!\n");
//	}

	slot_irq = gpio_to_irq(SLOTIRQ);
	irq_set_irq_type(slot_irq, IRQ_TYPE_EDGE_RISING);
    ret = request_irq(slot_irq, slot_isr, IRQF_TRIGGER_RISING,
//    		dev->name, dev);
    		"si4463", global_devrec);
	if (ret) {
//		dev_err(printdev(global_devrec), "Unable to get IRQ");
		printk("slot_irq REQUEST ERROR!\n");
	}

	reset();
	clr_interrupt();

	rx_start();
	printk(KERN_ALERT "rx_start\n");

	return 0;

err_irq:
err_read_reg:
//	ieee802154_unregister_device(devrec->dev);
err_register_device:
//	ieee802154_free_device(devrec->dev);
err_alloc_dev:
	kfree(devrec->buf);
err_buf:
	kfree(devrec);
err_devrec:
	return ret;
}

//static int si4463_remove(struct spi_device *spi)
//{
//	struct si4463 *devrec = spi_get_drvdata(spi);
//
//	dev_dbg(printdev(devrec), "remove\n");
//
//	free_irq(spi->irq, devrec);
//	flush_work(&devrec->irqwork); /* TODO: Is this the right call? */
//	ieee802154_unregister_device(devrec->dev);
//	ieee802154_free_device(devrec->dev);
//	/* TODO: Will ieee802154_free_device() wait until ->xmit() is
//	 * complete? */
//
//	/* Clean up the SPI stuff. */
//	spi_set_drvdata(spi, NULL);
//	kfree(devrec->buf);
//	kfree(devrec);
//	return 0;
//}


static int spi_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
//	mutex_lock(&device_list_lock);
//	list_del(&spidev->device_entry);
//	device_destroy(spidev_class, spidev->devt);
//	clear_bit(MINOR(spidev->devt), minors);
//	if (spidev->users == 0)
//		kfree(spidev);
//	mutex_unlock(&device_list_lock);

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
	.remove =	spi_remove,

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

void module_net_init(struct net_device *dev)
{
	printk(KERN_ALERT "module_net_init!\n");
	struct module_priv *priv;
	ether_setup(dev); /* assign some of the fields */
//	si4463_net_dev_setup(dev);

	dev->netdev_ops = &si4463_netdev_ops;

	//MTU
	dev->mtu		= 113;

	dev->dev_addr[0] = 0x18;//(0x01 & addr[0])为multicast
	dev->dev_addr[1] = 0x02;
	dev->dev_addr[2] = 0x03;
	dev->dev_addr[3] = 0x04;
	dev->dev_addr[4] = 0x05;
	dev->dev_addr[5] = 0x06;

	/* keep the default flags, just add NOARP */
	dev->flags           |= IFF_NOARP;
	dev->features        |= NETIF_F_HW_CSUM;
//	dev->features		 |= NETIF_F_LLTX;

	dev->tx_queue_len = 200;
	/*
	 * Then, initialize the priv field. This encloses the statistics
	 * and a few private fields.
	 */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct module_priv));
	priv->spi_priv = global_devrec;
	global_devrec->dev = global_net_devs;
	priv->dev_workqueue = create_singlethread_workqueue("tx_queue");
	mutex_init(&priv->pib_lock);
	printk(KERN_ALERT "module_net_init! dev:%x glo:%x\n", dev, global_net_devs);

	spin_lock_init(&priv->lock);
}

static int __init si4463_init(void)
{
	int status, ret, result;

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0) {
		goto out;
	}

	/* Allocate the NET devices */
	global_net_devs = alloc_netdev(sizeof(struct module_priv), "si%d", module_net_init);
	if (global_net_devs == NULL)
		goto out;


	ret = -ENODEV;
	if ((result = register_netdev(global_net_devs)))
		printk(KERN_ALERT "demo: error %i registering device \"%s\"\n",result, global_net_devs->name);
	else
		ret = 0;

	printk(KERN_ALERT "AFTER register_netdev, global_net_devs:%x\n", global_net_devs);
	/* init sem_irq , locked */
//	sema_init(&sem_irq, 0);
//	sema_init(&sem_tx_complete, 0);
//	sema_init(&sem_tx_wait_fifo, 0);
//	sema_init(&sem_rx_wait_fifo, 0);

out:
	return status;
}





//module_spi_driver(spidev_spi_driver);

module_init(si4463_init);

static void __exit si4463_exit(void)
{
	gpio_set_value(RADIO_SDN, 1);//disable

	spi_unregister_driver(&spidev_spi_driver);

	gpio_free(RADIO_SDN);
	gpio_free(SSpin);
	if (global_net_devs)
	{
		unregister_netdev(global_net_devs);
		free_netdev(global_net_devs);
	}
	del_timer(&tx_withdraw_timer);

	return;
}
module_exit(si4463_exit);


MODULE_AUTHOR("wu");
MODULE_DESCRIPTION("SI4463 SPI 802.15.4 Controller Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
