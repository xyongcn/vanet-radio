#include "queue.h"

//init queue
queue_head *queue_init()
{
	queue_head *q=(queue_head*)malloc(sizeof(queue_head));
	return q;
}

//add to queue()
int queue_add(queue_head *q,queue_node *node)
{
	//error argument
	if(q==NULL || node==NULL)
	{
		return 0;
	}

	//queue is null
	if(q->first==NULL && q->end==NULL)
	{
		q->first=node;
		q->end=node;
		node->prior=node;
		node->next=node;
	}
	else
	{
		queue_node *temp=q->end;
		temp->next=node;
		q->end=node;
		node->prior=temp;
		node->next=q->first;
		q->first->prior=node;
	}

	return 1;
}

queue_node *queue_get(queue_head *q)
{
	if(q->first==q->end)
	{
		if(q->first==NULL)
		{
			return NULL;
		}
		else
		{
			queue_node *result=q->first;
			q->first=NULL;
			q->end=NULL;
			return result;
		}
	}
	else
	{
		queue_node *result=q->first;
		q->first=result->next;
		q->end->next=q->first;
		q->first->prior=q->end;
		return result;
	}

}
