#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

typedef void free_func_t(void *);

struct queue_node {
	void *data;
	struct queue_node *next;
};

struct queue {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_mutex_t cond_mutex;
	struct queue_node *head;
	struct queue_node **p_tail;
};

struct queue *queue_new();
int queue_free(struct queue *queue, free_func_t free_func);
int queue_queue(struct queue *queue, void *data);
int queue_dequeue(struct queue *queue, void **data);
int queue_wait(struct queue *queue);

#endif
