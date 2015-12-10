#include "slots.h"

//struct slot slots_table[DEVICES_NUM];
//int global_slots_count = 0;
//
//void request_BCH(void){
//
//}

void updata_slot_info(struct slot *slot, u8 fi_data, u8 idfromslot){
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
				if( global_myid != GET_ID(fi_data))
					slot->slot_status = collision;
				else
					printk(KERN_ALERT "updata_slot_info: error update myslot\n");
			} else {
				if( global_myid != GET_ID(fi_data))
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
	case default:
		printk(KERN_ALERT "updata_slot_info: error update\n");
	}
	mutex_unlock(&slot->lock);
}

