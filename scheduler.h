/**
 * @file 	scheduler.h
 * @brief 	This file contains functions and variables related to the scheduler module -- i.e., the worker threads.
 *
 */

#ifndef __MTSP_SCHEDULER_HEADER
	#define __MTSP_SCHEDULER_HEADER 1

	#include "mtsp.h"
	#include "task_graph.h"
	#include "ThreadedQueue.h"

	/// Used to set a flag indicating that worker threads must "barrier" synchronize
	extern bool			volatile	__mtsp_threadWait;

	/// Used to count how many threads have already reached to the barrier.
	extern kmp_uint32	volatile	__mtsp_threadWaitCounter;

	/// This is used only in \c DEBUG_MODE when we want to dump the whole task graph
	/// before any task is executed.
	extern bool 		volatile 	__mtsp_activate_workers;

	/// The <b>total</b> number of tasks anywhere in the system. This comprises the
	/// tasks in all queues and in the task graph. The same task is not counted twice.
	extern kmp_int32	volatile	__mtsp_inFlightTasks;

	/// The number of active worker threads in the backend
	extern kmp_uint32	volatile	__mtsp_numWorkerThreads;

	/// Total number of threads managed/related to OMP
	extern kmp_uint32	volatile 	__mtsp_numThreads;

	/// Pointer to the list of worker pthreads
	extern pthread_t* 	volatile	workerThreads;

	/// Pointer to the list of worker threads IDs (may not start at 0)
	extern kmp_uint32* 	volatile	workerThreadsIds;

	/// This is the famous Run Queue
	extern SimpleQueue<kmp_task*, RUN_QUEUE_SIZE, RUN_QUEUE_CF> RunQueue;

	/// And this is the famous Retirement Queue
	extern SimpleQueue<kmp_task*, RETIREMENT_QUEUE_SIZE> RetirementQueue;

	/**
	 * This function is used to initialize the runtime module. One of its
	 * main responsabilities is to create the worker threads.
	 */
	void __mtsp_initScheduler();
#endif
