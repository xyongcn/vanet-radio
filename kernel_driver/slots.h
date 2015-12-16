#include "radio.h"
#include "ringbuffer.h"
#include "si4463/si4463_api.h"
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
//#define DEVICES_NUM 		2
//#define SLOTS_NUM			5
#define MAX_SLOTS_NUM		120
#define TIMEMS_PERPACKET	10

#define DATA_PACKET			0
#define CONTROL_PACKET		1
#define FI					0x00
#define BCH_REQ				0x40
#define EXTRA_SLOTS_REQ		0x20

#define SLOT_TIMEOUT_TIME	(jiffies + 3*HZ)

#define GET_TYPE(data) 			((data)&0x80)
#define GET_CONTROL_TYPE(data) 	((data)&0x60)
#define GET_ID(data)			((data)&0x1f)

#define FI_GET_BUSY(data)		((data)&0x80)

#define IS_INSLOT(slot_start_jiffies, slot_size_ms) \
	((jiffies_to_msecs(jiffies) + TIMEMS_PERPACKET) < \
	(jiffies_to_msecs(slot_start_jiffies) + (slot_size_ms)))

enum device_status {
	REQUEST_BCH,
	WAITING_BCH,
	WORKING
};
enum slot_status {
	reserve,
	myslot,
	available,
	collision,
};
struct slot {
	enum slot_status slot_status;
	u8 owner_id;
	int slot_id;
	u8 priority;
	u8 p2p;
	struct mutex lock;
	struct timer_list timeout_timer;
};

void init_slot_table(struct slot *slot);
u8 isEmpty(struct slot *slot);

void request_BCH(void);

void updata_slot_info(struct slot *slot, u8 fi_data, u8 idfromslot, int mybchid);

void update_slot_table(u8* fi_data,
		struct slot *slot_table,
		int size,
		int current_slot,
		int idfromslot,
		int mybchid);

int get_first_available_slot(struct slot *slot_table, int size);

void send_bch_request(struct si4463 *devrec, int bch_candidate);

/**
 * broadcast slot information
 * slots arrangement: mybch is the last one!
 */
void send_fi(struct si4463 *devrec, int slots_perframe, struct slot *slot_table);

void send_remaining_data_inslot(struct si4463 *devrec, rbuf_t *txrb,
		unsigned long slot_start_jiffies,
		unsigned int slot_size_ms);

