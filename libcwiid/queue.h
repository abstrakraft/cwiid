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
 *  2007-04-04 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Added queue_flush prototype
 *
 *  2007-03-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * Initial ChangeLog
 */

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
int queue_flush(struct queue *queue, free_func_t free_func);
int queue_free(struct queue *queue, free_func_t free_func);
int queue_queue(struct queue *queue, void *data);
int queue_dequeue(struct queue *queue, void **data);
int queue_wait(struct queue *queue);

#endif
