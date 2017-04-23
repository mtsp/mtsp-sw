/**
 * @file 	task_graph.h
 * @brief 	This file contains the implementation of the queues used in the system.
 *
 */

#ifndef MCRINGBUFFER_H_
#define MCRINGBUFFER_H_

#include "mtsp.h"
#include <stdio.h>
#include <stdlib.h>

/// for padding purposes
#define L1D_LINE_SIZE 	   64
#define USED			 	1
#define EMPTY			 	0

#define Q_LOCKED			1
#define Q_UNLOCKED			0
#define	GET_LOCK(ptr)		while (__sync_bool_compare_and_swap(ptr, Q_UNLOCKED, Q_LOCKED) == false)
#define RLS_LOCK(ptr)		__sync_bool_compare_and_swap(ptr, Q_LOCKED, Q_UNLOCKED)


/**
 * This is an implementation of the MCRingBuffer Single-Producer Single-Consumer
 * multi threaded queue. So all this is to say that there is only one thread
 * producing items and only one thread consuming them. This queue (algorithm)
 * is optimized because it does not require the use of locks and also because
 * it use paddings and batches to prevent false sharing when accessing the
 * queue items.
 */
template <typename T, int QUEUE_SIZE, int BATCH_SIZE, int CONT_FACTOR=100>
class SPSCQueue {
private:
	// shared control variables
	volatile int read;
	volatile int write;
	char pad1[L1D_LINE_SIZE - 2*sizeof(int)];

	// consumer local control variables
	int localWrite;
	int nextRead;
	char pad2[L1D_LINE_SIZE - 2*sizeof(int)];

	// producer local control variables
	int localRead;
	int nextWrite;
	char pad3[L1D_LINE_SIZE - 2*sizeof(int)];

	// Circular buffer to insert elements
	// all position of the queue are already initialized
	T elems[QUEUE_SIZE];


public:
	SPSCQueue() {
		read			= 0;
		write			= 0;
		localRead		= 0;
		localWrite		= 0;
		nextRead		= 0;
		nextWrite		= 0;

		/// The size of the queue must be a power of 2
		if (QUEUE_SIZE == 0 || (QUEUE_SIZE & (QUEUE_SIZE - 1)) != 0) {
			printf("CRITICAL: The size of the queue must be a power of 2.\n");
			exit(-1);
		}
	}

	/**
	 * Adds one element to the back of the queue. If the queue is full this
	 * method will busy wait until a free entry is available.
	 *
	 * @param elem 		The element to be added to the queue.
	 */
	void enq(T elem) {
////		__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_SPSC_Enq);

		int afterNextWrite = nextWrite + 1;
			afterNextWrite &= (QUEUE_SIZE - 1);

		if (afterNextWrite == localRead) {
////			__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_SPSC_Enq_Blocking);

			while (afterNextWrite == read) ;
			localRead = read;

////			__itt_task_end(__itt_mtsp_domain);
		}

		elems[nextWrite] = elem;

		nextWrite = afterNextWrite;

		if ((nextWrite & (BATCH_SIZE-1)) == 0)
			write = nextWrite;

////		__itt_task_end(__itt_mtsp_domain);
	}

	/** 
	 * Returns true if we can enqueue at least one more item on the queue.
	 * Actually it check to see if after the next enqueue the queue will be full.
	 * It updates the \c localRead if that would be true.
	 */
	bool can_enq() {
		return ((nextWrite+1) != localRead || ((localRead = read) != (nextWrite + 1)));
	}

	/**
	 * Remove one element from the front of the queue. If the queue is empty this
	 * method will busy wait until one element is available.
	 *
	 * @return	The element from the front of the queue.
	 */
	T deq() {
////		__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_SPSC_Deq);

		if (nextRead == localWrite) {
////			__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_SPSC_Deq_Blocking);

			//printf("Blocked, but Will not steal: cur_load = %d, cont_load = %d\n", cur_load(), cont_load());

			while (nextRead == write) ;
			localWrite = write;

////			__itt_task_end(__itt_mtsp_domain);
		}

		T data	= elems[nextRead];

		nextRead += 1;
		nextRead &= (QUEUE_SIZE-1);

		if ((nextRead & (BATCH_SIZE-1)) == 0)
			read = nextRead;

////		__itt_task_end(__itt_mtsp_domain);
		return data;
	}

	/**
	 * Returns true if there is any item in the queue ready to be dequeued
	 * We check to see if there is something left in the current batch visible
	 * to the consumer. If there is not then we check if the producer has
	 * already produced new items.
	 *
	 * (nextRead != localWrite)	==> where I would read is where the producer
	 *									was about to write a next item?
	 *	(nextRead != write)			==> has the producer produced new items yet?
	 */
	bool can_deq() {
		return (nextRead != localWrite || ((localWrite = write) != nextRead));
	}

	/**
	 * If the queue is not empty returns in \c elem the element from the front of the 
	 * queue and the function returns true. If the queue is empty the parameter \c elem 
	 * is not changed and the function return false. 
	 */
	bool try_deq(T* elem) {
		if (can_deq()) {
			*elem = deq();
			return true;
		}
		else {
			return false;
		}
	}

	/**
	 * Returns the number of items currently in the queue
	 */
	int cur_load() {
		int w = write, r = read;

		if (w-r >= 0)
			return w-r;
		else
			return (QUEUE_SIZE - r) + w;
	}

	/**
	 * This returns the number of items that represents the
	 * contention factor of this queue.
	 */
	int cont_load() {
		double cf = ((double)CONT_FACTOR / 100.0);
		return (int)(QUEUE_SIZE * cf);
	}

	/**
	 * This is used to flush items pending in the current batch. It
	 * is required because this queue update the indexes in batch mode to 
	 * prevent false sharing. However, if for some reason (i.e., there
	 * is no more items) a batch needs to be prematurely finished this
	 * function can be called.
	 */
	void fsh() {
		write = nextWrite;
		read = nextRead;
	}
};



/**
 * This is an multi threaded inter core queue implementation that can handle
 * multiple producers and/or multiple consumers. To cope with that it uses locks,
 * but currently these locks are based on atomic directives.
 */
template <typename T, int QUEUE_SIZE, int CONT_FACTOR=100>
class SimpleQueue {
private:
	volatile unsigned int head;
	volatile unsigned int tail;

	volatile T* data;
	volatile bool* status;
	volatile bool rlock;
	volatile bool wlock;

public:
	SimpleQueue() {
		if (QUEUE_SIZE <= 0 || (QUEUE_SIZE & (QUEUE_SIZE-1)) != 0) {
			printf("Queue size is not a power of 2! [%s, %d]\n", __FILE__, __LINE__);
			exit(-1);
		}

		data   = new T[QUEUE_SIZE];
		status = new bool[QUEUE_SIZE];

		for (int i=0; i<QUEUE_SIZE; i++)
			status[i] = EMPTY;

		rlock = Q_UNLOCKED;
		wlock = Q_UNLOCKED;
		head = 0;
		tail = 0;
	}

	/**
	 * Adds one element to the back of the queue. If the queue is full this
	 * method will busy wait until a free entry is available.
	 *
	 * @param elem 		The element to be added to the queue.
	 */
	void enq(T elem) {
		GET_LOCK(&wlock);

		while (status[tail] != EMPTY);

		data[tail] = elem;
		status[tail] = USED;

		tail = ((tail+1) & (QUEUE_SIZE-1));

		RLS_LOCK(&wlock);
	}

	/**
	 * Remove one element from the front of the queue. If the queue is empty this
	 * method will busy wait until one element is available.
	 *
	 * @return	The element from the front of the queue.
	 */
	T deq() {
		GET_LOCK(&rlock);
		while (status[head] == EMPTY);

		T elem = data[head];
		status[head] = EMPTY;
		head = ((head+1) & (QUEUE_SIZE-1));
		RLS_LOCK(&rlock);

		return elem;
	}

	/**
	 * If the queue is not empty returns in \c elem the element from the front of the 
	 * queue and the function returns true. If the queue is empty the parameter \c elem 
	 * is not changed and the function return false. 
	 */
	bool try_deq(T* elem) {
		GET_LOCK(&rlock);

		if (status[head] != EMPTY) {
			//while (status[head] == EMPTY);

			*elem = data[head];
			status[head] = EMPTY;
			head = ((head+1) & (QUEUE_SIZE-1));
			RLS_LOCK(&rlock);
			return true;
		}

		RLS_LOCK(&rlock);
		return false;
	}

	/**
	 * Returns the number of items currently in the queue
	 */
	int cur_load() {
		if (tail-head >= 0)
			return tail-head;
		else
			return (QUEUE_SIZE - head) + tail;
	}

	/**
	 * This returns the number of items that represents the
	 * contention factor of this queue.
	 */
	int cont_load() {
		return QUEUE_SIZE * ((double)CONT_FACTOR / 100.0);
	}

	~SimpleQueue() {
		// For now this is the safest option
		//delete [] data;
		//delete [] status;
	}
};

#endif /* MCRINGBUFFER_H_ */
