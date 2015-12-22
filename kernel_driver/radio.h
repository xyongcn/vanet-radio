#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
//#include <net/wpan-phy.h>
//#include <net/mac802154.h>
//#include <net/ieee802154.h>
#include <linux/gpio.h>
#include <linux/lnw_gpio.h>
#include <linux/irq.h>

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
#include <linux/jiffies.h>

#ifndef _ED_DEVICE_H
#define _ED_DEVICE_H

#define ED_REC_DEVICE      0
#define ED_TX_DEVICE       1
/* define the charactor device name */
#define ED_REC_DEVICE_NAME  "ed_rec"
#define ED_TX_DEVICE_NAME   "ed_tx"

#define ED_MTU              192
#define ED_MAGIC	     0x999
#define BUFFER_SIZE	     2048

#define MAJOR_NUM_REC 200
#define MAJOR_NUM_TX  201
#define IOCTL_SET_BUSY _IOWR(MAJOR_NUM_TX,1,int)
#include <linux/netdevice.h>

#define SPI_SPEED 7000000//
#define BITS_PER_WORD 8

//#define DEBUG

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
//	struct mutex *mutex_myslot;
	int mybch_number;
	int bch_candidate;
	u8 device_id;

	bool irq_busy;
	spinlock_t lock;
};

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* buffer is NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*buffer;
};

//inline void D

struct cmd_queue {
	int count;
	struct cmd *head;
};

struct ed_device{
	int magic;
	char name[8]; 	
	int busy;
	unsigned char *buffer;
    wait_queue_head_t rwait;
	int mtu;
	spinlock_t lock;
	int tx_len;
    int rx_len;
    int buffer_size;
	struct file *file;
    ssize_t (*kernel_write)(const char *buffer,size_t length,int buffer_size);
};

/* this is the private data struct of ednet */
struct module_priv
{
    struct net_device_stats stats;
    struct sk_buff *skb;
	struct workqueue_struct	*dev_workqueue;
	struct si4463 * spi_priv;

	struct mutex pib_lock;
    spinlock_t lock;
};

#define ED_TIMEOUT 5

//inline ssize_t si4463_sync_transfer(struct spidev_data *spidev, size_t len);
inline ssize_t spidev_sync_transfer(struct spidev_data *spidev, u8 *tx_buf, u8 *rx_buf, size_t len);
ssize_t spidev_sync_transfer_nosleep(struct spidev_data *spidev, u8 *tx_buf, u8 *rx_buf, size_t len);
inline ssize_t spidev_sync_read(struct spidev_data *spidev, size_t len);
inline ssize_t spidev_sync_write(struct spidev_data *spidev,  size_t len);
//inline ssize_t spidev_sync_write_nosleep(struct spidev_data *spidev,  size_t len);
inline ssize_t spidev_async_write(struct spidev_data *spidev,  size_t len);
ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message);

void ppp(u8 * arr, int len);

int set_pinmux(void);

// PIN Definitions:



/* common */
#define SCKpin  		109//IO13   // SCK
#define MOSIpin 		115//IO11   // MOSI
#define MISOpin 		114//IO12   // MISO
#define SSpin   		49	//IO8 //10    // SS
#define RST_PIN   		48	//IO7 //182
#define RADIO_SDN		RST_PIN
#define NIRQ 			182 //IO6  //interrpt
/* SI4463 GPIO*/
#define GPIO0			183 //IO9
//#define GPIO1			13 //IO5
#define TESTPIN			13 //IO5

/* TEST SLOT */
#define SLOTIRQ			183
/* RF212 SLP */
#define SLP_TR_PIN 		183 //IO9

#endif

