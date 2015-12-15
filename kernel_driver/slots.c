#include "slots.h"


extern unsigned int global_device_id;

void init_slot_table(struct slot *slot){
	int i;
	for(i=0; i<MAX_SLOTS_NUM; i++) {
		mutex_init(&slot[i].lock);
		slot[i].owner_id = 0;
		slot[i].slot_id = i;
		slot[i].slot_status = available;
	}
}
u8 isEmpty(struct slot *slot){
	int i;
	for(i=0; i<MAX_SLOTS_NUM; i++) {
		if(slot[i].slot_status != available)
			return 0;
	}
	return 1;
}

void updata_slot_info(struct slot *slot, u8 fi_data, u8 idfromslot, int mybchid){
	mutex_lock(&slot->lock);
	switch(slot->slot_status){
	case available:
		switch(FI_GET_BUSY(fi_data)){
		case 0:
			slot->slot_status = available;
			break;
		case 1:
			if(idfromslot != GET_ID(fi_data))
				slot->slot_status = available;
			else if(idfromslot == GET_ID(fi_data)) {
				slot->slot_status = reserve;
				slot->owner_id = idfromslot;
			}
			break;
		}
		break;
	case reserve:
		switch(FI_GET_BUSY(fi_data)){
		case 0:
			slot->slot_status = available;
			break;
		case 1:
			if(GET_ID(fi_data) == idfromslot) {
				if( slot->owner_id != GET_ID(fi_data))
					slot->slot_status = collision;
			} else {
				slot->slot_status = reserve;
			}
			break;
		}
		break;
	case myslot:
		switch(FI_GET_BUSY(fi_data)){
		case 0:
			slot->slot_status = collision;
			break;
		case 1:
			if(GET_ID(fi_data) == idfromslot) {
				if( global_device_id != GET_ID(fi_data))
					slot->slot_status = collision;
				else
					printk(KERN_ALERT "updata_slot_info: error update myslot\n");
			} else {
				if( global_device_id != GET_ID(fi_data))
					slot->slot_status = collision;
				else {
					slot->slot_status = myslot;
				}
			}
			break;
		}
		break;
	case collision:
		slot->slot_status = collision;
		break;
	default:
		printk(KERN_ALERT "updata_slot_info: error update\n");
	}
	mutex_unlock(&slot->lock);
}
void update_slot_table(u8* fi_data,
		struct slot *slot_table,
		int size,
		int current_slot,
		int idfromslot,
		int mybchid){
	int i, j;

	for(i=0; i<size; i++){
//		j = (i - (mybch_relatednum - mybch_number) + SLOTS_NUM) % SLOTS_NUM;
//		updata_slot_info(slot_table[j], fi_data[i], idfromslot);

		j = (i + (size - current_slot)) % size;
		updata_slot_info(&slot_table[j], fi_data[i], idfromslot, mybchid);
	}
}

int get_first_available_slot(struct slot *slot_table, int size) {
	int i;
	for(i=0; i<size; i++){
		mutex_lock(&slot_table[i].lock);
		if(slot_table[i].slot_status == available)
			 break;
		mutex_unlock(&slot_table[i].lock);
	}
	mutex_unlock(&slot_table[i].lock);
	return i;
}

void send_bch_request(struct si4463 *devrec, int bch_candidate) {
	int ret;
	u8 data[2];
again_irqbusy:
	spin_lock(&devrec->lock);
	if  (devrec->irq_busy) {
		printk(KERN_ALERT "send_bch_request: irq_busy!\n");
		spin_unlock(&devrec->lock);
//		return -EBUSY;
		goto again_irqbusy;
	}
	spin_unlock(&devrec->lock);

	change_state2tx_tune();

	tx_set_packet_len(2);
	data[0] = 1;
	data[1] = 0xdf & (devrec->device_id | 0xe0);
	printk(KERN_ALERT "send_bch_request: data- %x\n", data[1]);

	spi_write_fifo(data, 2);	//tmp_len);


	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 5 * HZ);

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

/**
 * broadcast slot information
 * slots arrangement: mybch is the last one!
 */
void send_fi(struct si4463 *devrec, int slots_perframe, struct slot *slot_table) {
	int ret;
	int i,j;
	u8 data[slots_perframe + 2];
	int len;
again_irqbusy:
	spin_lock(&devrec->lock);
	if  (devrec->irq_busy) {
		printk(KERN_ALERT "send_fi: irq_busy!\n");
		spin_unlock(&devrec->lock);
		goto again_irqbusy;
	}
	spin_unlock(&devrec->lock);

	change_state2tx_tune();
	len = slots_perframe + 1;// a slot fi consist 1byte; plus 1byte fi header;
	tx_set_packet_len(len + 1); //len plus 1byte data length header

	memset(data, slots_perframe + 2, 0);
	data[0] = len;
	data[1] = 0x9f & (devrec->device_id | 0xe0); //fi header: 1 00 [5bit id]
	for(i=0; i< slots_perframe; i++) {
		j = (i+(slots_perframe - 1 - devrec->mybch_number)) % slots_perframe;
		/* set BUSY bit*/
		switch(slot_table[j].slot_status){
		case available:
		case collision:
			data[i+2] |= 0x00;
			break;
		case reserve:
		case myslot:
			data[i+2] |= 0x80;
			break;
		default:
			printk(KERN_ALERT "send_bch_request: BUSY bit failed!\n");
			return;
		}
		/* set Priority bit */
		data[i+2] |= (slot_table[j].priority << 6);
		/* set p2p bit */
		data[i+2] |= (slot_table[j].p2p << 5);
		/* set id bit */
		data[i+2] |= slot_table[j].owner_id;
	}
	printk(KERN_ALERT "send_bch_request:\n");

	spi_write_fifo(data, 1);	//tmp_len);


	tx_start();
	ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 7 * HZ);

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

void send_remaining_data_inslot(struct si4463 *devrec, rbuf_t *txrb,
		unsigned long slot_start_jiffies,
		unsigned int slot_size_ms)
{
	int ret;
	u8 data[2];
	struct cmd *cmd_;

	if(unlikely(time_is_before_eq_jiffies(slot_start_jiffies))){
		printk(KERN_ALERT "send_remain_data_inslot: time error!\n");
		return;
	}

	while(IS_INSLOT(slot_start_jiffies, slot_size_ms) && !rbuf_empty(txrb)) {
		/* todo: send remaining data */
		cmd_ = rbuf_dequeue(txrb);
		data[0] = cmd_->len + 1;

		tx_set_packet_len(cmd_->len + 1);

		spi_write_fifo(data, 1);	//tmp_len);
		spi_write_fifo(cmd_->data, cmd_->len);

		tx_start2txtune();
		ret = wait_for_completion_interruptible_timeout(&devrec->tx_complete, 5 * HZ);

		if (ret > 0)	//wait_for_completion_interruptible_timeout
	//		clr_packet_sent_pend();
			clr_interrupt();
		else
		{
			printk(KERN_ALERT "send_bch_request:!!!!!!!!!!!!!!!!!!!SEND ERROR, RESETING!!!!!!!!!!!!!!!!!\n");
			printk(KERN_ALERT "NIRQ PIN: %d\n", gpio_get_value(NIRQ));
			reset();
		}
	}

	rx_start();
}

