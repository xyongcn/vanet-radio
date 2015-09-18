#include "si4463.h"

void insert_back(struct cmd_queue *cmd_queue_head, struct cmd *cmd_)
{
	struct cmd *p;
	if (cmd_queue_head->head == NULL)
	{
		cmd_queue_head->head = cmd_;
		return;
	}

	p = cmd_queue_head->head;
	while (p->next != NULL)
	{
		p = p->next;
	}
	p->next == cmd_;
	return;
}

struct cmd * get_front(struct cmd_queue *cmd_queue_head)
{
	struct cmd *p;
	if (cmd_queue_head->head == NULL) {
		printk(KERN_ALERT "cmd_queue_head is empty!!\n");
		return NULL;
	}
	p = cmd_queue_head->head;
	cmd_queue_head->head = p->next;
	return p;
}

