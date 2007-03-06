/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  03/03/2007: L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

struct queue *queue_new()
{
	struct queue *queue;
	if ((queue=malloc(sizeof *queue)) == NULL) {
		return NULL;
	}
	queue->head = NULL;
	queue->p_tail = &queue->head;

	if (pthread_mutex_init(&queue->mutex, NULL)) {
		free(queue);
		return NULL;
	}
	if (pthread_cond_init(&queue->cond, NULL)) {
		pthread_mutex_destroy(&queue->mutex);
		free(queue);
		return NULL;
	}
	if (pthread_mutex_init(&queue->cond_mutex, NULL)) {
		pthread_mutex_destroy(&queue->mutex);
		pthread_cond_destroy(&queue->cond);
		free(queue);
		return NULL;
	}

	return queue;
}

int queue_free(struct queue *queue, free_func_t free_func)
{
	int ret = 0;
	struct queue_node *cursor, *tmp;

	if (pthread_mutex_destroy(&queue->mutex)) {
		ret = -1;
	}
	if (pthread_cond_destroy(&queue->cond)) {
		ret = -1;
	}
	if (pthread_mutex_destroy(&queue->cond_mutex)) {
		ret = -1;
	}

	cursor = queue->head;
	while (cursor) {
		if (free_func) {
			free_func(cursor->data);
		}
		tmp = cursor->next;
		free(cursor);
		cursor = tmp;
	}

	free(queue);

	return ret;
}

int queue_queue(struct queue *queue, void *data)
{
	struct queue_node *node = NULL;
	int ret = 0;
	int canceltype;

	/* Allocate and populate new queue node */
	if ((node = malloc(sizeof *node)) == NULL) {
		return -1;
	}
	node->data = data;
	node->next = NULL;

	if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &canceltype)) {
		return -1;
	}
	/* Lock dispatch mutex, add node to dispatch queue, unlock mutex */
	if (pthread_mutex_lock(&queue->mutex)) {
		free(node);
		return -1;
	}
	*queue->p_tail = node;
	queue->p_tail = &node->next;
	if (pthread_mutex_unlock(&queue->mutex)) {
		return -1;
	}

	/* Signal dispatch condition */
	if (pthread_mutex_lock(&queue->cond_mutex)) {
		return -1;
	}
	if (pthread_cond_signal(&queue->cond)) {
		ret = -1;
	}
	if (pthread_mutex_unlock(&queue->cond_mutex)) {
		return -1;
	}
	if (pthread_setcanceltype(canceltype, &canceltype)) {
		ret = -1;
	}

	return ret;
}

int queue_dequeue(struct queue *queue, void **data)
{
	int ret = 0;
	struct queue_node *node;
	int canceltype;

	if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &canceltype)) {
		return -1;
	}

	/* Lock dispatch mutex, pop node, unlock mutex */
	if (pthread_mutex_lock(&queue->mutex)) {
		return -1;
	}
	if (queue->head) {
		node = queue->head;
		queue->head = node->next;
		if (!queue->head) {
			queue->p_tail = &queue->head;
		}
	}
	else {
		node = NULL;
		ret = -1;
	}
	if (pthread_mutex_unlock(&queue->mutex)) {
		ret = -1;
	}
	if (pthread_setcanceltype(canceltype, &canceltype)) {
		ret = -1;
	}

	if (node) {
		*data = node->data;
		free(node);
	}

	return ret;
}

int queue_wait(struct queue *queue)
{
	int wait_error = 0;

	if (pthread_mutex_lock(&queue->cond_mutex)) {
		return -1;
	}
	pthread_cleanup_push((void (*)(void *))pthread_mutex_unlock,
	                     (void *)&queue->cond_mutex);
	while (!wait_error && !queue->head) {
		if (pthread_cond_wait(&queue->cond, &queue->cond_mutex)) {
			wait_error = -1;
		}
	}
	pthread_cleanup_pop(1);

	return wait_error;
}

