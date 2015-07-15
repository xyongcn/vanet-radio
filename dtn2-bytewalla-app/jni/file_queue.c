#include "file_queue.h"

//find and remove target queue,there is the node return 1,otherwise return 0;
int file_queue_find_and_remove(queue_head *q,char *target_chars)
{
	if(q->first==NULL || q->end==NULL)
	{
		return 0;
	}

	queue_node *temp=q->end;
	// char *target_chars=(char *)node->content;
	do
	{
		char *thisnode=(char *)temp->content;
		if(thisnode!=NULL)
		{
			if(strcmp(target_chars,thisnode)==0)
			{
				//remove node
				temp->prior->next=temp->next;
				temp->next->prior=temp->prior;
				free(temp);
				return 1;
			}
		}
		temp=temp->next;
	}while(temp!=q->first && temp!=NULL);
	return 0;
}