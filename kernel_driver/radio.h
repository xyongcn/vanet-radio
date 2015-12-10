/* this is the header of the ed_device*/

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

/* TEST SLOT */
#define SLOTIRQ			183
/* RF212 SLP */
#define SLP_TR_PIN 		183 //IO9

#endif

