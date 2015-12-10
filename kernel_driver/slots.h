#include "radio.h"

/*
 * 1. 报文分为数据包与控制包，第一位标定
	1. 0xxx：数据包
	2. 1xxx：控制包
		1. 控制包又分为时隙信息（FI）（必要），基础时隙申请，额外时隙申请
		2. [1位数据或控制flag] [ 1位基础时隙申请FLAG  1位额外时隙申请FLAG ] [ 5位本节点id ] [ * 时隙N信息 （1位BUSY，1位优先级，1位点到点flag，5位节点id) ]
		3. 时隙信息包：000；基础时隙申请：100；额外时隙申请：010；
		4. 规则：
			1. 起始时先在监听中确定的的预选基础时隙发送基础时隙申请，此时不需要发送其他信息，即：1 100 [id]
			2. 时隙信息（FI）包：1 000 [id] [N条时隙信息]
			3. 额外时隙申请：在对应的时隙上发送额外时隙申请报文：1 010 [id]

		5. 128个字节包最多可以允许127个时隙
 */
#define DEVICES_NUM 		2
#define SLOTS_NUM			5

#define DATA_PACKET			0
#define CONTROL_PACKET		1
#define FI					0x00
#define BCH_REQ				0x40
#define EXTRA_SLOTS_REQ		0x20

#define GET_TYPE(data) 			(data)&0x80
#define GET_CONTROL_TYPE(data) 	(data)&0x60
#define GET_ID(data)			(data)&0x1f

#define FI_GET_BUSY(data)		(data)&0x80

enum status {
	reserve,
	myslot,
	available,
	collision,

	REQUEST_BCH,
	WAITING_BCH,
	WORKING
};
struct slot {
	enum status slot_status;
	u8 owner_id;
	int slot_id;
	struct mutex lock;
};

void init_slot_table(struct slot *slot, int size){
	int i;
	for(i=0; i<size; i++) {
		mutex_init(&slot[i].lock);
		slot[i].owner_id = 0;
		slot[i].slot_id = i;
		slot[i].slot_status = available;
	}
}

void request_BCH(void);

void updata_slot_info(struct slot *slot, u8 fi_data, u8 idfromslot);
void update_slot_table(u8* fi_data,
		struct slot *slot_table,
		int current_slot,
		int idfromslot){
	int i, j;

	for(i=0; i<SLOTS_NUM; i++){
//		j = (i - (mybch_relatednum - mybch_number) + SLOTS_NUM) % SLOTS_NUM;
//		updata_slot_info(slot_table[j], fi_data[i], idfromslot);

		j = (i + (SLOTS_NUM - current_slot)) % SLOTS_NUM;
		updata_slot_info(slot_table[j], fi_data[i], idfromslot);
	}
}

int get_first_available_slot(struct slot *slot_table) {
	int i;
	for(i=0; i<SLOTS_NUM; i++){
		mutex_lock(&slot_table[i].lock);
		if(slot_table[i].slot_status == available)
			 break;
	}
	mutex_unlock(&slot_table[i].lock);
	return i;
}

void send_bch_request(struct si4463 *devrec, struct mutex *slot_lock) {
	int ret;
	u8 data[2];
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

	/* time slot */
	mutex_lock(slot_lock);

	change_state2tx_tune();

	tx_set_packet_len(2);
	data[0] = 1;
	data[1] = 0xdf & (devrec->device_id | 0xe0);
	printk(KERN_ALERT "send_bch_request: data- %x\n", data);

	spi_write_fifo(data, 2);	//tmp_len);


	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 7 * HZ);

out_normal:
	if (ret > 0)	//wait_for_completion_interruptible_timeout
//		clr_packet_sent_pend();
		clr_interrupt();
	else
	{
		printk(KERN_ALERT "send_bch_request:!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");
		printk(KERN_ALERT "NIRQ PIN: %d\n", gpio_get_value(NIRQ));
		reset();
	}
	rx_start();

}

void send_fi(struct si4463 *devrec, struct mutex *slot_lock, struct slot *slot_table) {
	int ret;
	int i;
	u8 data[SLOTS_NUM + 2];
	int len;
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

	change_state2tx_tune();
	len = SLOTS_NUM + 1;// a slot fi consist 1byte; plus 1byte fi header;
	tx_set_packet_len(len + 1); //len plus 1byte data length header

	data[0] = len;
	data[1] = 0x9f & (devrec->device_id | 0xe0); //fi header: 1 00 [5bit id]
	for(i=1; i<= SLOTS_NUM; i++) {
		data[i+1] =
	}
	printk(KERN_ALERT "send_bch_request: data- %x\n", data);

	spi_write_fifo(data, 1);	//tmp_len);


	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 7 * HZ);

out_normal:
	if (ret > 0)	//wait_for_completion_interruptible_timeout
//		clr_packet_sent_pend();
		clr_interrupt();
	else
	{
		printk(KERN_ALERT "send_bch_request:!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");
		printk(KERN_ALERT "NIRQ PIN: %d\n", gpio_get_value(NIRQ));
		reset();
	}
	rx_start();

}
