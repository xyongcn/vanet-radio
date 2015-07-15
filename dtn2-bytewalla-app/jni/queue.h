#include <stdlib.h>
// #include <stdio.h>
//queue's head
typedef struct queueNode queue_node;

typedef struct queueHead
{
	queue_node *first;
	queue_node *end;
}queue_head;

//queue node
typedef struct queueNode
{
	queue_node *prior;
	queue_node *next;
	void *content;
}queue_node;

queue_head *queue_init();
int queue_add(queue_head *q,queue_node *node);
queue_node *queue_get(queue_head *q);

//if queue is empty ,then return 1
int queue_empty(queue_head *q);


//file_queue function
int file_queue_find_and_remove(queue_head *q,char *target_chars);