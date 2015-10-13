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
#include <linux/irq.h>

#include <asm/uaccess.h>

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

/**
 * Module installed.
 * Support si4463 or rf212
 */
#define SI4463	1
#define RF212	2

static int module_install_global = 0;
module_param(module_install_global, int, S_IRUGO);
MODULE_PARM_DESC(module_install_global, "module installed");

static int mtu_global = 110;
module_param(mtu_global, int, S_IRUGO);
MODULE_PARM_DESC(mtu_global, "MAC layer MTU");

/*-------------------------------------------------------------------------*/


#include "radio.h"
#include "si4463/si4463_api.h"

#include "rf212/at86rf212.h"
#include "rf212/rf212_api.h"
#include "rf212/return_val.h"
#include "rf212/tal_internal.h"

//#include "cmd_queue.h"
#include "ringbuffer.h"

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

//#include <linux/wait.h>
#include <linux/kthread.h>

#include <linux/fs.h>

#include <linux/semaphore.h> /* semphone for the TX */
#include <linux/mutex.h>
#include <linux/timer.h>
/* semphone for the TX */
//DEFINE_SEMAPHORE(sem_tx_complete);
struct semaphore sem_tx_complete;
//DEFINE_MUTEX(mutex_tx_complete);
/* semphone for the SPI */
//DEFINE_SEMAPHORE(sem_spi);
//DEFINE_MUTEX(mutex_spi);
/* semphone for the irq */
//DEFINE_SEMAPHORE(sem_irq);
struct semaphore sem_irq;
DEFINE_MUTEX(mutex_irq_handling);
DEFINE_MUTEX(mutex_spi);

#define IRQ_NET_CHIP  20//需要根据硬件确定


/* GLOBAL */
static char  netbuffer[100];
struct net_device *global_net_devs;
//struct net_device *tmp_devs;

static bool isSending = 0;
struct int_with_lock{
	spinlock_t lock;
	int data;
};

inline static int read_int_with_lock(struct int_with_lock *t){
	int tmp;
	spin_lock(&t->lock);
	tmp = t->data;
	spin_unlock(&t->lock);
	return tmp;
}
inline static void write_int_with_lock(struct int_with_lock *t, int data){
	spin_lock(&t->lock);
	t->data = data;
	spin_unlock(&t->lock);
}

struct int_with_lock isHandlingIrq;
static bool preamble_detect = 0;

wait_queue_head_t wait_irq;
//struct {
//	bool flag;
//	mutex_irq_handling
//};

struct spi_device *spi_save;
struct spidev_data spidev_global;
struct spidev_data *spidev_test_global;
struct spi_message spi_message_global;

//wait_queue_head_t wait_irq;
//struct cmd_queue *cmd_queue_head;
rbuf_t cmd_ringbuffer;
struct task_struct *cmd_handler_tsk;
struct task_struct *irq_handler_tsk;

/* chip status for rf212 */
extern tal_trx_status_t tal_trx_status;
extern tal_state_t tal_state;


/* TX withdraw timer */
struct timer_list tx_withdraw_timer;
static bool tx_approved = 1;
static void withdraw(unsigned long data)
{
	tx_approved = 1;
    printk(KERN_ALERT "tx is now approved\n");
}

/* netif queue restart timer */
struct timer_list netq_restart_timer;
static void netq_restart(unsigned long data)
{
	netif_wake_queue(global_net_devs);
    printk(KERN_ALERT "Netif Queue is opening\n");
    rbuf_print_status(&cmd_ringbuffer);
}

/**
 * Function points to the threads
 */
int (*cmd_queue_handler)(void*);
int (*irq_handler)(void*);

/* FOR DEBUG */
static u8 tx_ph_data[19] = {'a','b','c','d','e',0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55};


/* PIN MUX */
#define pin_mux_num  31
//const char gpio_pin[pin_mux_num][4] = {
//		/*"111 ",*/
//		"115 ",
//		"114 ",
//		"109 ",
//		"214 ",
//
//		"263 ",
//		"240 ",
//		"262 ",
//		"241 ",
//		"242 ",
//		"243 ",
//		"258 ",
//		"259 ",
//		"260 ",
//		"261 ",
//		"226 ",
//		"227 ",
//		"228 ",
//		"229 ",
//
//		/*IO7 and IO8*/
//		"256 ",
//		"224 ",
//		"255 ",
//		"223 ",
//
//		/*IO6*/
//		"254 ",
//		"222 ",
//		"182 "
//};
//#define pin_mux_num  26
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


/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
void ppp(u8 * arr, int len){
	int i = 0;
	printk(KERN_ALERT "ppp: len=[%d]\n", len);
	for(i = 0;i<len;i++)
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
//			printk(KERN_ALERT "Reset SPI!\n");
//			if (spi_save == NULL)
//				printk(KERN_ALERT "Reset SPI ERROR!!!!\n");
//			int tmp = ~SPI_MODE_MASK;
//			tmp |= SPI_MODE_0;
//			//tmp |= SPI_NO_CS;
//			//tmp |= SPI_CS_HIGH;
//			//tmp |= SPI_READY;
//			spi_save->mode = (u8)tmp;
//			spi_save->bits_per_word = BITS_PER_WORD;
//			spi_save->max_speed_hz = SPI_SPEED;
//			int ret = spi_setup(spi_save);
//			if (ret < 0)
//				printk(KERN_ALERT "ERROR! spi_setup return: %d\n", ret);
//			else
//				printk(KERN_ALERT "spi_setup succeed, spi:%x, spidev_global.spi:%x\n", spi_save, spidev_global.spi);
//			spidev_complete(&done);
//			DECLARE_COMPLETION_ONSTACK(done_retry);
//			message->complete = spidev_complete;
//			message->context = &done_retry;
//			status = spi_async(spidev->spi, message);
//			status = wait_for_completion_timeout(&done_retry, HZ);
//			if(status == 0){
//				printk(KERN_ALERT "*********************RETRY SPI TIMEOUT ERROR*****************\n");
//			}
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

//inline ssize_t
//spidev_async_write(struct spidev_data *spidev,  size_t len)
//{
//	struct spi_transfer	t = {
//			.tx_buf		= spidev->buffer,
//			.len		= len,
//			.cs_change	= 1,
//		};
//	struct spi_message	m;
//
//	spi_message_init(&m);
//	spi_message_add_tail(&t, &m);
//	return spi_async(spidev->spi, &m);
//}

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

		printk(KERN_ALERT "  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
				u_tmp->len,
				u_tmp->rx_buf ? "rx " : "",
				u_tmp->tx_buf ? "tx " : "",
				u_tmp->cs_change ? "cs " : "",
				u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
				u_tmp->delay_usecs,
				u_tmp->speed_hz ? : spidev->spi->max_speed_hz);

		spi_message_add_tail(k_tmp, &msg);
	}


//	printk(KERN_ALERT "n_xfers: %d\n", n_xfers);//1
//	printk(KERN_ALERT "spi_ioc_transfer:\n");
//	printk(KERN_ALERT "len: %d\n",u_xfers->len);//1
//	printk(KERN_ALERT "speed_hz: %d\n",u_xfers->speed_hz);//0
//	printk(KERN_ALERT "delay_usecs: %d\n",u_xfers->delay_usecs);//0
//	printk(KERN_ALERT "bits_per_word: %d\n",u_xfers->bits_per_word);//0
//	printk(KERN_ALERT "cs_change: %d\n",u_xfers->cs_change);//0

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
	spin_lock_init(&isHandlingIrq.lock);
	isHandlingIrq.data = 0;
	mutex_init(&spidev_global.buf_lock);

	printk(KERN_ALERT "spidev_probe!\n");

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

	/* Allocate driver dsi4463_devsata */
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


	/* GPIO setup
	 *
	 */
	/* Request Chip SDN gpios */
//	if (gpio_is_valid(RADIO_SDN)) {
//		saved_muxing = gpio_get_alt(RADIO_SDN);
//		lnw_gpio_set_alt(RADIO_SDN, LNW_GPIO);
//		err = gpio_request_one(RADIO_SDN,
//				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio SDN pin");
		err = gpio_request(RADIO_SDN,  "Radio SDN pin");
		if (err) {
			/*pr_err(
					"%s: unable to get Chip Select GPIO,\
						fallback to legacy CS mode \n",
					__func__);*/
			printk(KERN_ALERT "ERROR! RADIO_SDN request error: %d\n", err);
//			lnw_gpio_set_alt(RADIO_SDN, saved_muxing);
		}
		if (module_install_global == SI4463)
			gpio_direction_output(RADIO_SDN, 1);
		else if (module_install_global == RF212)
			gpio_direction_output(RST_PIN, 0);

//	} else {
//		printk(KERN_ALERT "ERROR! invalid pin RADIO_SDN\n");
//	}

	/* Request Chip Select gpios */

//	if (gpio_is_valid(SSpin)) {
//		saved_muxing = gpio_get_alt(SSpin);
//		lnw_gpio_set_alt(SSpin, LNW_GPIO);
//		err = gpio_request_one(SSpin,
//				GPIOF_DIR_OUT | GPIOF_INIT_HIGH, "Radio CS pin");
		err = gpio_request(SSpin,  "Radio CS pin");
		if (err) {
			printk(KERN_ALERT "ERROR! SSpin request error: %d\n", err);
//			lnw_gpio_set_alt(SSpin, saved_muxing);
		}
		gpio_direction_output(SSpin, 1);


//	} else {
//		printk(KERN_ALERT "ERROR! invalid pin SSpin\n");
//	}
//
//	/*
//	 *
//	 */
//	//gpio_direction_output(SCKpin, 0);
//	//gpio_direction_output(MOSIpin, 0);
//	//gpio_direction_input(MISOpin);
//
	/* PIN MUX */
	set_pinmux();
//	mdelay(1000);
//	set_pinmux();


	/* spi setup :
	 * 		SPI_MODE_0
	 * 		MSBFIRST
	 * 		CS active low
	 * 		IRQ??????
	 */

//	spin_lock_irq(&spidev->spi_lock);
//	spin_unlock_irq(&spidev->spi_lock);
	spi_message_init(&spi_message_global);

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


//
//	/* Waiting Queue */
//	//init_waitqueue_head(&spi_wait_queue);
//	//cmd_queue_head = kmalloc(sizeof(struct cmd_queue), GFP_ATOMIC);
//	//cmd_queue_head->count = 0;
	init_waitqueue_head(&wait_irq);

	/* setup timer */
	setup_timer(&tx_withdraw_timer, withdraw, 0);
	tx_withdraw_timer.expires = jiffies + 4 * HZ/1000; //jiffies + HZ/2 are 0.5s
	tx_withdraw_timer.function = withdraw;
	tx_withdraw_timer.data = 0;

	setup_timer(&netq_restart_timer, netq_restart, 0);
	netq_restart_timer.expires = jiffies + HZ * 3; //jiffies + HZ/2 are 0.5s
	netq_restart_timer.function = netq_restart;
	netq_restart_timer.data = 0;


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

void netstack_rx(struct net_device *dev, int len, unsigned char *buf)
{
//	printk(KERN_ALERT "si4463_rx\n");
//	ppp(buf, len);
    struct sk_buff *skb;
    struct module_priv *priv = netdev_priv(dev);//(struct module_priv *) dev->priv;

    skb = dev_alloc_skb(len+2);
    if (!skb) {
        printk(KERN_ALERT "si4463_rx can not allocate more memory to store the packet. drop the packet\n");
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
static void irq_tx_complete() {
//
	up(&sem_tx_complete);
	isSending = 0;
	if(module_install_global == RF212) {
		tal_state = TAL_IDLE;
		tal_csma_state = CSMA_IDLE;
	}

//	printk(KERN_ALERT "TX_COMPLETE\n");
//	mutex_unlock(&mutex_tx_complete);
}
static void irq_rx(/*void *dev_id*/){
//	printk(KERN_ALERT "RECV\n");
//	tmp_devs = (struct net_device *) dev_id;
	//cmd_queue_head->count++;
//	struct cmd cmd_;
//	cmd_.type = READFIFO_CMD;
//	//using the data field to store the net_devices point;
//	cmd_.data = 0x00;
//	cmd_.len = 4;
	//insert_back(cmd_queue_head, &cmd_);
//	rbuf_enqueue(&cmd_ringbuffer, &cmd_);
	rbuf_insert_readcmd(&cmd_ringbuffer);
	//wake_up_interruptible(&spi_wait_queue);
}
static irqreturn_t rf212_interrupt (int irq, void *dev_id)
{
	write_int_with_lock(&isHandlingIrq, 1);

//	printk(KERN_ALERT "=====IRQ=====\n");
//	printk(KERN_ALERT "sen_irq before: %d\n", sem_irq.count);
	if(sem_irq.count > 0)
		printk(KERN_ALERT "!!!!!!!!!sen_irq > 0\n");
	up(&sem_irq);
//	printk(KERN_ALERT "sen_irq after: %d\n", sem_irq.count);
//	mutex_unlock(&mutex_irq);
	return IRQ_HANDLED;
}


static int rf212_irq_handler(void* data){
	bool tx_complete_flag;
	bool rx_flag;
	u8 ph_pend;
	u8 md_status;
	trx_irq_reason_t trx_irq_cause;

	while(1/*!kthread_should_stop()*/) {
		tx_complete_flag = 0;
		rx_flag = 0;
		write_int_with_lock(&isHandlingIrq, 0);

		down(&sem_irq);
	    trx_irq_cause = (trx_irq_reason_t)pal_trx_reg_read(RG_IRQ_STATUS);

//	    printk(KERN_ALERT "trx_irq_cause: %x", trx_irq_cause);

	    if (trx_irq_cause & TRX_IRQ_TRX_END)
	    {
	        /*
	         * TRX_END reason depends on if the trx is currently used for
	         * transmission or reception.
	         */
//	#if ((!defined RFD) && (!defined NOBEACON))
//	        if ((tal_state == TAL_TX_AUTO) || tal_beacon_transmission)
//	#else
//	        if (tal_state == TAL_TX_AUTO)
//	#endif
	    	if (tal_state == TAL_TX_AUTO)
	        {
	            /* Get the result and push it to the queue. */
//	            handle_tx_end_irq();
	        	irq_tx_complete();
	        	tx_complete_flag = 1;
	        }
	        else   /* Other tal_state than TAL_TX_... */
	        {
	            /* Handle rx done interrupt. */
//	            handle_received_frame_irq();
	        	irq_rx();
	        	rx_flag = 1;
	        	preamble_detect = 0;
	        }
	    }


//		printk(KERN_ALERT "tx_complete_flag: %d, isSending: %d\n",tx_complete_flag, isSending);
	 	if (!tx_complete_flag && isSending) {
	 		/* *
	 		 * recv a packet when a sending is wait for complete.
	 		 * the tx send complete irq	will be vanished
	 		 * */
	 		/*
	 		 * SEND ERROR!!!!
	 		 */
	 		printk(KERN_ALERT "!!!!SEND ERROR!!!!\n");
	 		tx_fifo_reset();
	 		irq_tx_complete();
	 		rx_start();
	 	}
//error:
		//Clear IRQ, rx process will clear irq.
		if(!rx_flag && !tx_complete_flag && !preamble_detect){

		}
		write_int_with_lock(&isHandlingIrq, 0);
//		wake_up_interruptible(&wait_irq);
	}
	return -1;
}
static int rf212_cmd_queue_handler(void *data){
	struct cmd *cmd_;
	int i;
	int ret;
	u8 tmp_len;
	u8 tmp[130];
//	u8 rx[64];
//	u8 padding[64];
	struct sk_buff *skb;
	u8* data_ptr;
//	u8 rssi;
//	memset(padding, 0, 64);
	memset(tmp, 0, 130);
	while(1/*!kthread_should_stop()*/){
//		printk(KERN_ALERT "cmd_queue_handler:1\n");
		if(read_int_with_lock(&isHandlingIrq)) {
//			printk(KERN_ALERT "isHandlingIrq: %d\n", isHandlingIrq);

//			wait_event_interruptible(wait_irq, !isHandlingIrq);
			udelay(10);
			continue;
//			mutex_lock(&mutex_irq_handling);
		}

		if(rbuf_almost_empty(&cmd_ringbuffer) && netif_queue_stopped(global_net_devs)) {

//			printk(KERN_ALERT "%d\n",netif_xmit_frozen_or_stopped(global_net_devs));
//			printk(KERN_ALERT "%d\n",netif_xmit_stopped(global_net_devs));
//			printk(KERN_ALERT "%d\n",netif_queue_stopped(global_net_devs));


			netif_wake_queue(global_net_devs);
			printk(KERN_ALERT "Netif Queue is awaaaaaak!!\n");
			rbuf_print_status(&cmd_ringbuffer);
		}
		cmd_ = rbuf_dequeue(&cmd_ringbuffer);
//		mutex_unlock(&mutex_irq_handling);
		switch(cmd_->type){
		case READFIFO_CMD:
//			printk(KERN_ALERT "+++++++++++++++++++++++++++READFIFO_CMD+++++++++++++++++++++++++++\n");

	        pal_trx_frame_read(&tmp_len, 1);
	        //len += 2;
//	        Serial.print("len:");
//	        Serial.println(len);
	        pal_trx_frame_read(tmp, tmp_len);
//	        ppp(tmp+1, tmp[0]-2);
	        netstack_rx(global_net_devs,tmp[0]-2,tmp+1);
//			printk(KERN_ALERT "rf212: Len: %d\n", tmp[0]-2);
//			clr_packet_rx_pend();
//			rx_start();
			rf212_clr_irq();

			break;
		case SEND_CMD:
//			printk(KERN_ALERT "-----------------------SEND_CMD, spi:%x, spi_save:%x\n", spidev_global.spi, spi_save);

			if(/*preamble_detect ||*/  rbuf_peep_first_isREADCMD(&cmd_ringbuffer)){
				/*
				 * we cannot send when receiving.
				 */
				rbuf_enqueue(&cmd_ringbuffer, cmd_);
				printk(KERN_ALERT "BREAK!!!\n");
//				tx_approved = 0;
//				add_timer(&tx_withdraw_timer);
				break;
			}

			isSending = 1;

			skb = (struct sk_buff*)cmd_->data;
			tmp_len = skb->len;
			data_ptr = skb->data;
//			/* THIS MAY TRIGGER ERROR! */
//			*(data_ptr--) = tmp_len + 2;
			memcpy(tmp+1, data_ptr, tmp_len);
			tmp[0] = tmp_len + 2;
			if(tmp_len > 40) {
				tmp[1] = 0x18;
				tmp[2] = 0x02;
				tmp[3] = 0x03;
				tmp[4] = 0x04;
				tmp[5] = 0x05;
				tmp[6] = 0x06;
			}

//			ppp(tmp, tmp_len+1);

//			printk(KERN_ALERT "SEND_CMD:LEN: cmd_->len[%d]\n", tmp_len);
			send_frame(tmp, 1, 0);
			//semaphore
//			printk(KERN_ALERT "waiting begin\n");
			ret = down_timeout(&sem_tx_complete, 2*HZ);
//			printk(KERN_ALERT "waiting end\n");
			/* FREE THE SKB */
			dev_kfree_skb(skb);
//			printk(KERN_ALERT "free skb\n");
			if(ret == 0) {
				//clear irq
				rf212_clr_irq();
//				printk(KERN_ALERT "clr irq\n");
			}else {
				printk(KERN_ALERT "!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");

				trx_reset();
				break;
			}

			rf212_rx_begin();
//			printk(KERN_ALERT "rf212_rx_begin\n");
			break;
		default:
			printk(KERN_ALERT "!!!!!!!!!!!!!!!!!!!CMD ERROR!!!!!!!!!!!!!!!!!\n");
			rbuf_print_status(&cmd_ringbuffer);
		}
	}
	return -1;

}

int rf212_open(struct net_device *dev)
{
	int ret=0;
	int saved_muxing = 0;
	int err,tmp;


	printk(KERN_ALERT "RF212_o2pen\n");

	if (spi_save == NULL)
		return -ESHUTDOWN;

	tmp = ~SPI_MODE_MASK;
	netif_start_queue(dev);

	/**
	 * INIT RF212
	 */
	if (FAILURE == tal_init())
		printk(KERN_ALERT "tal_init FAILED!!! \n");

	mdelay(1000);

	/* IRQ */
	int irq = gpio_to_irq(NIRQ);
	gpio_set_value(NIRQ, 0);
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_RISING);

    ret = request_irq(irq, rf212_interrupt, IRQF_NO_SUSPEND,
			  dev->name, dev);
	if (ret){
		printk(KERN_ALERT "request_irq failed!\n");
		return ret;
	}

	/*RING BUFFER */
	rbuf_init(&cmd_ringbuffer);

	/*CMD HANDLER*/
	cmd_handler_tsk = kthread_run(cmd_queue_handler, NULL, "cmd_queue_handler");
	if (IS_ERR(cmd_handler_tsk)) {
		printk(KERN_ALERT "create kthread failed!\n");
	} else {
		printk("create ktrhead ok!\n");
	}

	/* IRQ HANDLER */
	irq_handler_tsk = kthread_run(irq_handler, NULL, "irq_handler");
	if (IS_ERR(irq_handler_tsk)) {
		printk(KERN_ALERT "create kthread irq_handler failed!\n");
	} else {
		printk("create ktrhead irq_handler ok!\n");
	}

	rf212_rx_begin();
	printk(KERN_ALERT "rx_start\n");
    return ret;
}

//void rf212_hw_tx(char *buf, u16 len, struct net_device *dev)
//{
//    struct module_priv *priv;
//
//    /* check the ip packet length,it must more then 34 octets */
////    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
////	{
////        printk("Bad packet! It's size is less then 34!\n");
////        return;
////    }
//    /* record the transmitted packet status */
//    priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
//    priv->stats.tx_packets++;
//    priv->stats.rx_bytes += len;
//
////    ppp(buf, len);
//
//	struct cmd cmd_;
//	cmd_.type = SEND_CMD;
//	cmd_.data =
//
////	if (len >= 64)
////		goto out;
////	cmd_.len =  len;//19;
//
//	rbuf_enqueue(&cmd_ringbuffer, &cmd_);//must before the kfree
//out:
//    /* remember to free the sk_buffer allocated in upper layer. */
//	//commemt 20150915: move the kfree_skb to the cmd_queue_handler.
//    //dev_kfree_skb(priv->skb);
//
//
//}

int rf212_tx(struct sk_buff *skb, struct net_device *dev){
//    int len;
//    u8 *data;
	if (rbuf_full(&cmd_ringbuffer))
	{
		netif_stop_queue(global_net_devs);
		printk(KERN_ALERT "Netif Queue is closing\n");
		rbuf_print_status(&cmd_ringbuffer);
//		add_timer(&netq_restart_timer);
		return -1;
	}
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;

//    len = skb->len;// < ETH_ZLEN ? ETH_ZLEN : skb->len;
//    data = skb->data;


    /* stamp the time stamp */
    dev->trans_start = jiffies;

    /* check the ip packet length,it must more then 34 octets */
//    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
//	{
//        printk("Bad packet! It's size is less then 34!\n");
//        return;
//    }ndo_start_xmit
    /* record the transmitted packet status */
    priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_packets++;
    priv->stats.rx_bytes += skb->len;

	struct cmd cmd_;
	cmd_.type = SEND_CMD;
	cmd_.data = (void*)skb;

	rbuf_enqueue(&cmd_ringbuffer, &cmd_);//must before the kfree

    /* remember the skb and free it in si4463_hw_tx */
//    priv->skb = skb;

    /* pseudo transmit the packet,hehe */
//    rf212_hw_tx(data, (u16)len, dev);

    return 0;
}

int rf212_release(struct net_device *dev)
{
	printk("rf212_release\n");
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
void rf212_tx_timeout (struct net_device *dev)
{
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_errors++;
    netif_wake_queue(dev);

    return;
}



/*
 * When we need some ioctls.
 */
int rf212_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	printk("rf212_ioctl\n");
    return 0;
}

/*
 * ifconfig to get the packet transmitting status.
 */

struct net_device_stats *rf212_stats(struct net_device *dev)
{
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    return &priv->stats;
}

/*
 * TCP/IP handshake will call this function, if it need.
 */
int rf212_change_mtu(struct net_device *dev, int new_mtu)
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

static irqreturn_t si4463_interrupt (int irq, void *dev_id)
{
	write_int_with_lock(&isHandlingIrq, 1);
	printk(KERN_ALERT "=====IRQ=====\n");
//	printk(KERN_ALERT "sen_irq before: %d\n", sem_irq.count);
	if(sem_irq.count > 0)
		printk(KERN_ALERT "!!!!!!!!!sen_irq > 0\n");
	up(&sem_irq);
//	printk(KERN_ALERT "sen_irq after: %d\n", sem_irq.count);
//	mutex_unlock(&mutex_irq);
	return IRQ_HANDLED;
}

static int si4463_irq_handler(void* data){
	bool tx_complete_flag;
	bool rx_flag;
	u8 ph_pend;
	u8 md_status;

	while(1/*!kthread_should_stop()*/) {
		tx_complete_flag = 0;
		rx_flag = 0;
		write_int_with_lock(&isHandlingIrq, 0);

		down(&sem_irq);
//		mutex_lock(&mutex_irq_handling);
//		printk(KERN_ALERT "===irq_handler===\n");


		/*
		 * read FRR_A for PH_STATUS_PEND (or status????)
		 * 1, tx complete
		 * 2, rx
		 */
		read_frr_a(&ph_pend);
		printk(KERN_ALERT "PH_PEND: %x\n", ph_pend);
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
		//Check Tx complete
		if((ph_pend & 0x20) == 0x20) {
			irq_tx_complete();
			tx_complete_flag = 1;
		}
		//Check packet received
		if((ph_pend & 0x10) == 0x10) {
			irq_rx();
			rx_flag = 1;
			preamble_detect = 0;
		}



//		printk(KERN_ALERT "tx_complete_flag: %d, isSending: %d\n",tx_complete_flag, isSending);
	 	if (!tx_complete_flag && isSending) {
	 		/* *
	 		 * recv a packet when a sending is wait for complete.
	 		 * the tx send complete irq	will be vanished
	 		 * */
	 		/*
	 		 * SEND ERROR!!!!
	 		 */
	 		printk(KERN_ALERT "!!!!SEND ERROR!!!!\n");
	 		tx_fifo_reset();
	 		irq_tx_complete();
	 		rx_start();
	 	}
		write_int_with_lock(&isHandlingIrq, 0);

error:
		//Clear IRQ, rx process will clear irq.
		if(!rx_flag && !tx_complete_flag && !preamble_detect){
//			printk(KERN_ALERT "(((((((((((((((1)))))))))))))\n");
//			get_interrupt_status();
//			printk(KERN_ALERT "(((((((((((((((2)))))))))))))\n");
//			clr_packet_sent_pend();
//			printk(KERN_ALERT "(((((((((((((((3)))))))))))))\n");
//			get_interrupt_status();
//			printk(KERN_ALERT "(((((((((((((((4)))))))))))))\n");
			printk(KERN_ALERT "interrupt_status:::\n");
			get_interrupt_status();
//			printk(KERN_ALERT "PH_status:::\n");
//			read_frr_a(&ph_pend);
//			printk(KERN_ALERT "PH_PEND: %x\n", ph_pend);
//			set_frr_ctl();
//			read_frr_a(&ph_pend);
//			printk(KERN_ALERT "PH_PEND: %x\n", ph_pend);
			clr_interrupt();
		}
		write_int_with_lock(&isHandlingIrq, 0);
//		mutex_unlock(&mutex_irq_handling);
	}
	return -1;
}

static int si4463_cmd_queue_handler(void *data){
	struct cmd *cmd_;
	int i;
	int ret;
	u8 tmp_len;
	u8 tmp[10];
	u8 rx[64];
	u8 padding[64];
	u8 rssi;
	struct sk_buff *skb;
	u8* data_ptr;
	memset(padding, 0, 64);
	while(1/*!kthread_should_stop()*/){
//		printk(KERN_ALERT "cmd_queue_handler:1\n");
		if(read_int_with_lock(&isHandlingIrq)) {
			udelay(5);
//			printk(KERN_ALERT "isHandlingIrq: %d\n", isHandlingIrq);
			continue;
//			mutex_lock(&mutex_irq_handling);
		}
		cmd_ = rbuf_dequeue(&cmd_ringbuffer);
//		mutex_unlock(&mutex_irq_handling);
		switch(cmd_->type){
		case READFIFO_CMD:
			printk(KERN_ALERT "+++++++++++++++++++++++++++READFIFO_CMD+++++++++++++++++++++++++++\n");

			get_fifo_info(tmp);
			printk(KERN_ALERT "FIFO: %d, %d\n", tmp[1], tmp[2]);

			if(tmp[1]!=64){
				/*
				 * recv error
				 */
				fifo_reset();
				clr_packet_rx_pend();
				break;
			}

			spi_read_fifo(rx, 64);
		    //get data from hardware register
			tmp_len = rx[0];
			printk(KERN_ALERT "Len: %d\n", tmp_len);
			clr_packet_rx_pend();
			rx_start();
			netstack_rx(global_net_devs,rx[0],rx+1);

			break;
		case SEND_CMD:
			if(!tx_approved){
				rbuf_enqueue(&cmd_ringbuffer, cmd_);
				printk(KERN_ALERT "tx isn't approved!\n");
				break;
			}
			printk(KERN_ALERT "-----------------------SEND_CMD, spi:%x, spi_save:%x\n", spidev_global.spi, spi_save);
			/*
			 * can use rssi int: first close other int, then open rssi int.
			 */
						get_modem_status(tmp);
						printk(KERN_ALERT "When Sending, rssi value: %x\n", tmp[3]);
			for(i = 0; i<10; i++){
//				get_modem_status(tmp);
//				//rssi = tmp[3];// & 0x08;
//				printk(KERN_ALERT "rssi: %x\n", tmp[3]);

				rssi = get_CCA();
				if(rssi)
					break;
				ndelay(95);
			}

//			get_modem_status(tmp);
//			printk(KERN_ALERT "When Sending, rssi value: %x\n", tmp[3]);
//			printk(KERN_ALERT "rssi: %d\n", rssi);
//			printk(KERN_ALERT "preamble_detect: %d\n", preamble_detect);
			if(/*preamble_detect ||*/ read_int_with_lock(&isHandlingIrq) || rssi/*rssi!=0*/ || rbuf_peep_first_isREADCMD(&cmd_ringbuffer)){
				/*
				 * we cannot send when receiving.
				 */
				rbuf_enqueue(&cmd_ringbuffer, cmd_);
				printk(KERN_ALERT "BREAK!!!\n");
				tx_approved = 0;
				add_timer(&tx_withdraw_timer);
				break;
			}


			change_state2tx_tune();
			isSending = 1;

//			printk(KERN_ALERT "Writing: \n");
//			get_fifo_info();
//			get_interrupt_status();
//			printk(KERN_ALERT "NIRQ before: %d\n", gpio_get_value(NIRQ));

//			tx_change_variable_len(cmd_->len);
			skb = (struct sk_buff*)cmd_->data;
			tmp_len = skb->len;
			data_ptr = skb->data;
			if(tmp_len > 40) {
				data_ptr[0] = 0x18;
				data_ptr[1] = 0x02;
				data_ptr[2] = 0x03;
				data_ptr[3] = 0x04;
				data_ptr[4] = 0x05;
				data_ptr[5] = 0x06;
			}
			spi_write_fifo(&tmp_len, 1);
			spi_write_fifo(data_ptr, tmp_len);
			if((tmp_len + 1) < 64)
				spi_write_fifo(padding, 64-tmp_len-1);
			tx_start();
			printk(KERN_ALERT "SEND_CMD:LEN: cmd_->len[%d]\n", tmp_len);
			//semaphore
//			down(&sem_tx_complete);
			ret = down_timeout(&sem_tx_complete, 2*HZ);
			/* FREE THE SKB */
			dev_kfree_skb(skb);
			if(ret == 0)
				clr_packet_sent_pend();
			else {
				printk(KERN_ALERT "!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");

				reset();
				break;
			}

			rx_start();

			break;
		default:
			printk(KERN_ALERT "!!!!!!!!!!!!!!!!!!!CMD ERROR!!!!!!!!!!!!!!!!!\n");
			rbuf_print_status(&cmd_ringbuffer);
		}
	}
	return -1;

}

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
//int set_pinmux_test(){
//	/* PIN MUX */
//	#define pin_mux_num  26
//	const struct gpio pin_mux[pin_mux_num] = {
//			{111, NULL, NULL},
//			{115, NULL, NULL},
//			{114, NULL, NULL},
//			{109, NULL, NULL},
//			{214, GPIOF_INIT_LOW, NULL},
//
//			{263, GPIOF_INIT_HIGH, NULL},
//			{240, GPIOF_INIT_HIGH, NULL},
//			{262, GPIOF_INIT_HIGH, NULL},
//			{241, GPIOF_INIT_HIGH, NULL},
//			{242, GPIOF_INIT_HIGH, NULL},
//			{243, GPIOF_INIT_HIGH, NULL},
//			{258, GPIOF_INIT_HIGH, NULL},
//			{259, GPIOF_INIT_HIGH, NULL},
//			{260, GPIOF_INIT_LOW, NULL},
//			{261, GPIOF_INIT_HIGH, NULL},
//			{226, GPIOF_DIR_IN, NULL},
//			{227, GPIOF_DIR_IN, NULL},
//			{228, GPIOF_DIR_IN, NULL},
//			{229, GPIOF_DIR_IN, NULL},
//
//			/*IO7 and IO8*/
//			{256, GPIOF_INIT_HIGH, NULL},
//			{224, GPIOF_DIR_IN, NULL},
//			{255, GPIOF_INIT_HIGH, NULL},
//			{223, GPIOF_DIR_IN, NULL},
//
//			/*IO6*/
//			{254, GPIOF_INIT_LOW, NULL},
//			{222, GPIOF_DIR_IN, NULL},
//			{182, GPIOF_DIR_IN, NULL}
//	};
//
//}
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

	if (module_install_global == SI4463) {
		/* IO9 Input for si4463(GPIO)*/
		fp = filp_open("/sys/kernel/debug/gpio_debug/gpio183/current_pinmux", O_RDWR, 0);
		write2file(fp, s_mode0, 5);
		fp = filp_open("/sys/class/gpio/gpio257/direction", O_RDWR, 0);
		write2file(fp, s_low, 3);
		fp = filp_open("/sys/class/gpio/gpio225/direction", O_RDWR, 0);
		write2file(fp, s_in, 2);
		fp = filp_open("/sys/class/gpio/gpio183/direction", O_RDWR, 0);
		write2file(fp, s_in, 2);
	} else if (module_install_global == RF212) {
	    /* IO9 OUTPUT for RF212(SLP)*/
	    fp = filp_open("/sys/kernel/debug/gpio_debug/gpio183/current_pinmux", O_RDWR, 0);
	    write2file(fp, s_mode0, 5);
	    fp = filp_open("/sys/class/gpio/gpio257/direction", O_RDWR, 0);
	    write2file(fp, s_high, 4);
	    fp = filp_open("/sys/class/gpio/gpio225/direction", O_RDWR, 0);
	    write2file(fp, s_in, 2);
	    fp = filp_open("/sys/class/gpio/gpio183/direction", O_RDWR, 0);
	    write2file(fp, s_high, 4);
	}

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

int si4463_open(struct net_device *dev)
{
	int ret=0;
	int saved_muxing = 0;
	int err,tmp;


	printk(KERN_ALERT "si4463_o2pen\n");
	/* spi setup :
	 * 		SPI_MODE_0
	 * 		MSBFIRST
	 * 		CS active low
	 * 		IRQ??????
	 */

//	spin_lock_irq(&spidev->spi_lock);
//	spin_unlock_irq(&spidev->spi_lock);

	if (spi_save == NULL)
		return -ESHUTDOWN;

	tmp = ~SPI_MODE_MASK;
	netif_start_queue(dev);
	reset();

//	si4463_register_init();
//	setRFParameters();
//	Function_set_tran_property();
//	fifo_reset();

	/*RX*/
	//printk(KERN_ALERT "RXXXXXXXXXXX\n");
//	enable_rx_interrupt();
//	enable_chip_irq();
	//printk(KERN_ALERT "enable_rx_interrupt\n");
	clr_interrupt();
	//printk(KERN_ALERT "clr_interrupt\n");


	mdelay(1000);

	/* IRQ */
	int irq = gpio_to_irq(NIRQ);
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);

    ret = request_irq(irq, si4463_interrupt, IRQF_NO_SUSPEND,
			  dev->name, dev);
	if (ret){
		printk(KERN_ALERT "request_irq failed!\n");
		return ret;
	}

	/*RING BUFFER */
	rbuf_init(&cmd_ringbuffer);

	/*CMD HANDLER*/
	cmd_handler_tsk = kthread_run(cmd_queue_handler, NULL, "cmd_queue_handler");
	if (IS_ERR(cmd_handler_tsk)) {
		printk(KERN_ALERT "create kthread failed!\n");
	} else {
		printk("create ktrhead ok!\n");
	}

	/* IRQ HANDLER */
	irq_handler_tsk = kthread_run(irq_handler, NULL, "irq_handler");
	if (IS_ERR(irq_handler_tsk)) {
		printk(KERN_ALERT "create kthread irq_handler failed!\n");
	} else {
		printk("create ktrhead irq_handler ok!\n");
	}

/*
	int tmp = 5;
	//	u8 str[4] = {0xaa, 0xbb, 0xcc, 0xdd};
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
*/

	rx_start();
	printk(KERN_ALERT "rx_start\n");
    return ret;
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
 * pseudo network hareware transmit,it just put the data into the
 * ed_tx device.
 */

//void si4463_hw_tx(char *buf, u16 len, struct net_device *dev)
//{
////	int i;
//	//printk("si4463_hw_tx\n");
//    struct module_priv *priv;
//
//    /* check the ip packet length,it must more then 34 octets */
//    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
//	{
//        printk("Bad packet! It's size is less then 34!\n");
//        return;
//    }
//    /* record the transmitted packet status */
//    priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
//    priv->stats.tx_packets++;
//    priv->stats.rx_bytes += len;
//
////    ppp(buf, len);
//
//	struct cmd cmd_;
//	cmd_.type = SEND_CMD;
//	cmd_.data = buf;//the memcpy will be ran in the enqueue.
//
//	if (len >= 64)
//		goto out;
//	cmd_.len =  len;//19;
//
//	rbuf_enqueue(&cmd_ringbuffer, &cmd_);//must before the kfree
//out:
//    /* remember to free the sk_buffer allocated in upper layer. */
//    dev_kfree_skb(priv->skb);
//
//
//}

/*
 * Transmit the packet,called by the kernel when there is an
 * application wants to transmit a packet.
 */

int si4463_tx(struct sk_buff *skb, struct net_device *dev)
{
//	printk(KERN_ALERT "si4463_tx\n");
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;

//    len = skb->len;// < ETH_ZLEN ? ETH_ZLEN : skb->len;
//    data = skb->data;


    /* stamp the time stamp */
    dev->trans_start = jiffies;

    /* check the ip packet length,it must more then 34 octets */
//    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr))
//	{
//        printk("Bad packet! It's size is less then 34!\n");
//        return;
//    }
    /* record the transmitted packet status */
    priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
    priv->stats.tx_packets++;
    priv->stats.rx_bytes += skb->len;

	struct cmd cmd_;
	cmd_.type = SEND_CMD;
	cmd_.data = (void*)skb;

	rbuf_enqueue(&cmd_ringbuffer, &cmd_);//must before the kfree

    /* remember the skb and free it in si4463_hw_tx */
//    priv->skb = skb;

    /* pseudo transmit the packet,hehe */
//    rf212_hw_tx(data, (u16)len, dev);

    return 0;
}

/*
 * Deal with a transmit timeout.
 */
void si4463_tx_timeout (struct net_device *dev)
{
    struct module_priv *priv = (struct module_priv *) netdev_priv(dev);//dev->priv;
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

static const struct net_device_ops si4463_netdev_ops = {

	.ndo_open            = si4463_open,
	.ndo_stop            = si4463_release,
	.ndo_start_xmit		 = si4463_tx,
	.ndo_do_ioctl        = si4463_ioctl,
	.ndo_get_stats       = si4463_stats,
	.ndo_change_mtu      = si4463_change_mtu,
	.ndo_tx_timeout      = si4463_tx_timeout,
};

/**
 * RF212 File Operation
 */
//=========================================================================================

static const struct net_device_ops rf212_netdev_ops = {

	.ndo_open            = rf212_open,
	.ndo_stop            = rf212_release,
	.ndo_start_xmit		 = rf212_tx,
	.ndo_do_ioctl        = rf212_ioctl,
	.ndo_get_stats       = rf212_stats,
	.ndo_change_mtu      = rf212_change_mtu,
	.ndo_tx_timeout      = rf212_tx_timeout,
};


//static void si4463_net_dev_setup(struct net_device *dev)
//{
//	dev->header_ops		= &eth_header_ops;
//	dev->type		= ARPHRD_ETHER;
//	dev->hard_header_len 	= ETH_HLEN;
//	dev->mtu		= 64;
//	dev->addr_len		= ETH_ALEN;
//	dev->tx_queue_len	= 1000;	/* Ethernet wants good queues */
//	dev->flags		= IFF_BROADCAST|IFF_MULTICAST;
//	dev->priv_flags		|= IFF_TX_SKB_SHARING;
//
//	memset(dev->broadcast, 0xFF, ETH_ALEN);
//
//}

void module_net_init(struct net_device *dev)
{

	struct module_priv *priv;
	ether_setup(dev); /* assign some of the fields */
//	si4463_net_dev_setup(dev);

	if (module_install_global == SI4463)
		dev->netdev_ops = &si4463_netdev_ops;
	else if(module_install_global == RF212)
		dev->netdev_ops = &rf212_netdev_ops;

	//MTU
	dev->mtu		= mtu_global;

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
	memset(priv, 0, sizeof(struct module_priv));
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
	global_net_devs = alloc_netdev(sizeof(struct module_priv), "rf%d", module_net_init);
	if (global_net_devs == NULL)
		goto out;


	ret = -ENODEV;
	if ((result = register_netdev(global_net_devs)))
		printk(KERN_ALERT "demo: error %i registering device \"%s\"\n",result, global_net_devs->name);
	else
		ret = 0;
	/* init sem_irq , locked */
	sema_init(&sem_irq, 0);
	sema_init(&sem_tx_complete, 0);

	/**
	 * Func points
	 */
	if(module_install_global == SI4463) {
		cmd_queue_handler = &si4463_cmd_queue_handler;
		irq_handler = &si4463_irq_handler;
	} else if(module_install_global == RF212) {
		cmd_queue_handler = &rf212_cmd_queue_handler;
		irq_handler = &rf212_irq_handler;
	}
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
	gpio_free(SSpin);
	if (global_net_devs)
	{
		unregister_netdev(global_net_devs);
		free_netdev(global_net_devs);
	}
	del_timer(&tx_withdraw_timer);
	rbuf_destroy(&cmd_ringbuffer);
	return;
}
module_exit(spidev_exit);


MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
