#include <linux/slab.h>
#include "ringbuffer.h"

DEFINE_MUTEX(mutex_rbuf);
/* 初始化环形缓冲区 */
int rbuf_init(rbuf_t *rb)
{
	int i;
	spin_lock_init(&rb->lock);
	init_waitqueue_head(&rb->wait_isfull);
	init_waitqueue_head(&rb->wait_isempty);

	rb->size = 0;
	rb->next_in = 0;
	rb->next_out = 0;
	rb->capacity = RBUF_MAX;

	rb->data = kmalloc(RBUF_MAX*sizeof(struct cmd), GFP_KERNEL);
	for (i = 0; i < RBUF_MAX; i++)
	{
		rb->data[i].data = kmalloc(MAXSIZE_PER_CMD*sizeof(u8), GFP_KERNEL);
	}

	return 0;
}

/* 销毁环形缓冲区 */
void rbuf_destroy(rbuf_t *c)
{
	int i;
	for (i = 0; i < RBUF_MAX; i++)
	{
		kfree(c->data[i].data);
	}
}

void copy_cmd(struct cmd *dest, struct cmd *src){
//	int i;
//	for(i=0; i<src->len; i++)
//		dest->data[i] = src->data[i];
	memcpy(dest->data, src->data, src->len);
//	dest->data = src->data;
	dest->len = src->len;
	dest->type = src->type;
//	dest->next = src->next;
}

void copy_cmd_without_dataField(struct cmd *dest, struct cmd *src) {
//	dest->len = src->len;
	dest->type = src->type;
//	dest->next = src->next;
}

/* 压入数据 */
int rbuf_enqueue(rbuf_t *rb, struct cmd *cmd_)
{
//	printk(KERN_ALERT "rbuf_enqueue\n");
//	int ret;
	//ppp(cmd_->data, cmd_->len);

	if (rbuf_full(rb))
	{
		printk(KERN_ALERT "ringbuffer is FULL, enlarge the RBUF_MAX\n");
		wait_event_interruptible(rb->wait_isfull, !rbuf_full(rb));
	}

//	printk(KERN_ALERT "len: %d, datap: %x, next_in:%d, size: %d\n", cmd_->len, cmd_->data, rb->next_in, rb->size);
	spin_lock(&rb->lock);
//	mutex_lock(&mutex_rbuf);
	copy_cmd(&(rb->data[rb->next_in++]), cmd_);
	rb->size++;
	rb->next_in %= rb->capacity;

	//ppp(rb->data[rb->next_in-1].data, rb->data[rb->next_in-1].data)

	spin_unlock(&rb->lock);
//	mutex_unlock(&mutex_rbuf);
	wake_up_interruptible(&rb->wait_isempty);
	return 0;
}

/* 压入数据
 * read cmd must be done first!
 * */
int rbuf_insert_readcmd(rbuf_t *rb)
{
//	printk(KERN_ALERT "rbuf_insert_readcmd\n");
//	int ret;
//	int tmp;
	//ppp(cmd_->data, cmd_->len);

	if (rbuf_full(rb))
	{
		printk(KERN_ALERT "ringbuffer is FULL, enlarge the RBUF_MAX\n");
		wait_event_interruptible(rb->wait_isfull, !rbuf_full(rb));
	}

	if (rbuf_empty(rb))
	{
		spin_lock(&rb->lock);
//		mutex_lock(&mutex_rbuf);
		rb->data[rb->next_in].type = READFIFO_CMD;
//		rb->data[rb->next_in].len = 4;
		rb->next_in++;
		rb->size++;
		rb->next_in %= rb->capacity;

		//ppp(rb->data[rb->next_in-1].data, rb->data[rb->next_in-1].data)

		spin_unlock(&rb->lock);
//		mutex_unlock(&mutex_rbuf);
		wake_up_interruptible(&rb->wait_isempty);
		return 0;
	}

	spin_lock(&rb->lock);
//	mutex_lock(&mutex_rbuf);
	if(rb->next_out == 0)
		rb->next_out == rb->capacity - 1;
	else
		rb->next_out = (rb->next_out - 1) % rb->capacity;
//	copy_cmd(&(rb->data[rb->next_out]), cmd_);
//	copy_cmd_without_dataField(&(rb->data[rb->next_out]), cmd_);

	rb->data[rb->next_out].type = READFIFO_CMD;
//	rb->data[rb->next_out].len = 4;

	rb->size++;
	spin_unlock(&rb->lock);
//	mutex_unlock(&mutex_rbuf);

	wake_up_interruptible(&rb->wait_isempty);
	return 0;
}

/* 取出数据 */
struct cmd* rbuf_dequeue(rbuf_t *rb)
{
//	printk(KERN_ALERT "rbuf_dequeue\n");
	struct cmd *cmd_;
//	int ret = 0;

	if (rbuf_empty(rb))
	{
//		printk(KERN_ALERT "ringbuffer is EMPTY!\n");
		wait_event_interruptible(rb->wait_isempty, !rbuf_empty(rb));
	}

//	printk(KERN_ALERT "next_in:%d, size: %d\n", rb->next_in, rb->size);
	spin_lock(&rb->lock);
//	mutex_lock(&mutex_rbuf);
	cmd_ = &rb->data[rb->next_out++];
	rb->size--;
	rb->next_out %= rb->capacity;

	spin_unlock(&rb->lock);
//	mutex_unlock(&mutex_rbuf);
	wake_up_interruptible(&rb->wait_isfull);
	//return ret;
	return cmd_;
}

/* 判断缓冲区是否为满 */
bool rbuf_full(rbuf_t *c)
{
	return (c->size == c->capacity);
}

bool rbuf_almost_full(rbuf_t *c)
{
	return (c->size >= (c->capacity - 5));
}

/*  */
bool rbuf_almost_empty(rbuf_t *c){
	return (c->size < 10);
}

/* 判断缓冲区是否为空 */
bool rbuf_empty(rbuf_t *c)
{
	return (c->size == 0);
}

/* 获取缓冲区可存放的元素的总个数 */
int rbuf_capacity(rbuf_t *c)
{
	return c->capacity;
}

int rbuf_len(rbuf_t *c)
{
	return c->size;
}

bool rbuf_peep_first_isREADCMD(rbuf_t *rb)
{
	struct cmd *cmd_;
//	int ret = 0;

	if (rbuf_empty(rb))
	{
//		printk(KERN_ALERT "peep: ringbuffer is EMPTY!\n");
		return 0;
	}
//	spin_lock(&rb->lock);
	cmd_ = &rb->data[rb->next_out];

//	spin_unlock(&rb->lock);
	return (cmd_->type == READFIFO_CMD) ? 1 : 0;;
}

void rbuf_print_status(rbuf_t *rb) {
//	spin_lock(&rb->lock);
	printk(KERN_ALERT "size: %d\n", rb->size);
//	printk(KERN_ALERT "next_in: %d\n", rb->next_in);
//	printk(KERN_ALERT "next_out %d\n", rb->next_out);
//	spin_unlock(&rb->lock);
//	printk(KERN_ALERT "", );
}
