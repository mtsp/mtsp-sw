/**
 * @file 	task_graph.h
 * @brief 	This file contains functions and variables related to the Task Graph Manager.
 *
 */

#ifndef __MTSP_TASK_GRAPH_HEADER
	#define __MTSP_TASK_GRAPH_HEADER 1

	#include "mtsp.h"

	/// Tells whether we have already initialized the task graph data structures
	extern bool 							taskGraphInitialized;

	/// Each position stores a pointer to a task descriptor for a task present in the task graph.
	extern kmp_task* 			volatile 	tasks[MAX_TASKS];

	// depCounters[i] tells on how many other tasks task 'i' depends on
	extern kmp_uint16 			volatile	depCounters[MAX_TASKS];

	// dependents[i][0] tells how many depends task 'i' has, these depends are listed on the subsequent dependents[i][1...]
	extern kmp_uint16 			volatile	dependents[MAX_TASKS][MAX_DEPENDENTS+1];

	// If a position whoIDependOn[i][j] is true then task 'i' depends on task 'j'
	extern bool 				volatile	whoIDependOn[MAX_TASKS][MAX_DEPENDENTS+1];

	/**
	 * This store a list of the free entries (slots) of the taskGraph. The index zero (0)
	 * is used as a counter for the number of free slots. The remaining positions store 
	 * indices of free slots in the taskGraph.
	 */
	extern kmp_uint16 			volatile 	freeSlots[MAX_TASKS + 1];




	/**
	 * This is used just for debugging purposes. It dumps the current task 
	 * graph to a dot file.
	 */
	void __mtsp_dumpTaskGraphToDot();

	/**
	 * This is used to initialize the task graph data structures.
	 */
	void __mtsp_initializeTaskGraph();

	/**
	 * This is used to add a new task to the task graph. The pointer to
	 * the task was obtained from the submission queue.
	 */
	void addToTaskGraph(kmp_task*);

	/**
	 * This is used to remove a task from the task graph. The pointer
	 * to the task was obtained from a Retirement Queue entry.
	 */
	void removeFromTaskGraph(kmp_task*);
#endif
