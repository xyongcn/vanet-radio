#ifndef __rbuf_H__
#define __rbuf_H__

/* Define to prevent recursive inclusion
 -------------------------------------*/
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define RBUF_MAX 100
#define MAXSIZE_PER_CMD 3000


/* === TEMPPPPP~ ==============================================================*/
typedef struct buffer_tag
{
    uint8_t *body;              /* Pointer to the buffer body */
    struct buffer_tag *next;    /* Pointer to next free buffer */
} buffer_t;
/**
 * Queue structure. The application should declare the queue of type queue_t
 * and call qmm_queue_init before invoking any other functionality of qmm
 */
typedef struct queue_tag
{
    buffer_t *head;
    buffer_t *tail;

    /* Maximum number of buffers that can be accomodated in the current queue */
    uint8_t capacity;

    /* Number of buffers present in the current queue */
    uint8_t size;
} queue_t;
/* === TEMPPPPP~end ==============================================================*/

struct cmd {
#define READFIFO_CMD	1
#define SEND_CMD		2
#define OTHER_CMD		55
	int type;
	u8 * data;
//	struct sk_buff *skb;
	int len;
//	struct cmd *next;
};

typedef struct _rbuf
{
	int size; /* 当前缓冲区中存放的数据的个数 */
	int next_in; /* 缓冲区中下一个保存数据的位置 */
	int next_out; /* 从缓冲区中取出下一个数据的位置 */
	int capacity; /* 这个缓冲区的可保存的数据的总个数 */
//	mutex_t        mutex;            /* Lock the structure */
	spinlock_t lock;
//    cond_t        	not_full;        /* Full -> not full condition */
//   cond_t        	not_empty;        /* Empty -> not empty condition */
	wait_queue_head_t wait_isfull;
	wait_queue_head_t wait_isempty;
	struct cmd *data;/* 缓冲区中保存的数据指针 */
} rbuf_t;

/* 初始化环形缓冲区 */
int rbuf_init(rbuf_t *rb);

/* 销毁环形缓冲区 */
void rbuf_destroy(rbuf_t *rb);

/* 压入数据 */
int rbuf_enqueue(rbuf_t *rb, struct cmd *cmd_);

int rbuf_insert_readcmd(rbuf_t *rb);

/* 取出数据 */
struct cmd * rbuf_dequeue(rbuf_t *rb);

/* 判断缓冲区是否为满 */
bool rbuf_full(rbuf_t *rb);

bool rbuf_almost_full(rbuf_t *c);

bool rbuf_almost_empty(rbuf_t *c);

/* 判断缓冲区是否为空 */
bool rbuf_empty(rbuf_t *rb);

/* 获取缓冲区可存放的元素的总个数 */
int rbuf_capacity(rbuf_t *rb);

int rbuf_len(rbuf_t *rb);

bool rbuf_peep_first_isREADCMD(rbuf_t *rb);

void rbuf_print_status(rbuf_t *rb);
#endif
