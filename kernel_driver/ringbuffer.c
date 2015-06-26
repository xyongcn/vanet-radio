#include <linux/slab.h>
#include "ringbuffer.h"

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

	for (i = 0; i < RBUF_MAX; i++)
	{
		rb->data[i].data = kmalloc(MAXSIZE_PER_CMD, GFP_KERNEL);
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
	dest->data = src->data;
	dest->len = src->len;
	dest->type = src->type;
	dest->next = src->next;
}

/* 压入数据 */
int rbuf_enqueue(rbuf_t *rb, struct cmd *cmd_)
{
	//printk(KERN_ALERT "rbuf_enqueue\n");
	int ret = 0;
	spin_lock(&rb->lock);

	if (rbuf_full(rb))
	{
		spin_unlock(&rb->lock);
		printk(KERN_ALERT "ringbuffer is FULL, enlarge the RBUF_MAX\n");
		wait_event_interruptible(rb->wait_isfull, !rbuf_full(rb));
		spin_lock(&rb->lock);
	}


	copy_cmd(&(rb->data[rb->next_in++]), cmd_);
	rb->size++;
	rb->next_in %= rb->capacity;
out:
	spin_unlock(&rb->lock);
	wake_up_interruptible(&rb->wait_isempty);
	return ret;
}

/* 取出数据 */
struct cmd* rbuf_dequeue(rbuf_t *rb)
{
	//printk(KERN_ALERT "rbuf_dequeue\n");
	struct cmd *cmd_ = NULL;
	int ret = 0;

	spin_lock(&rb->lock);

	if (rbuf_empty(rb))
	{
		spin_unlock(&rb->lock);
		//printk(KERN_ALERT "ringbuffer is EMPTY!\n");
		wait_event_interruptible(rb->wait_isempty, !rbuf_empty(rb));
		spin_lock(&rb->lock);
	}


	cmd_ = &rb->data[rb->next_out++];
	rb->size--;
	rb->next_out %= rb->capacity;
out:
	spin_unlock(&rb->lock);
	wake_up_interruptible(&rb->wait_isfull);
	//return ret;
	return cmd_;
}

/* 判断缓冲区是否为满 */
bool rbuf_full(rbuf_t *c)
{
	return (c->size == c->capacity);
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
