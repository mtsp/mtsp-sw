/**
 * @file 	dep_tracker.h
 * @brief 	This file contains the functions and variables used by the Dependence Manager module.
 */
#ifndef __MTSP_DEP_TRACKER_HEADER
	#define __MTSP_DEP_TRACKER_HEADER 1

	#include "kmp.h"
	#include "mtsp.h"
	#include <map>
	#include <utility>




	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//===-------------------------------------------------- Start of Variable Declaration --===//
	
	/**
	 * This is a map structure used to to keep track of which task is accessing
	 * each memory address. Therefore, dependenceTable maps from kmp_intptr
	 * (i.e., a memory address) to a pair \c LastWriter + \c vector<kmp_int32>
	 * where \c LastWriter is the ID of the last in flight task that write to
	 * this address and the vector contain the task IDs of tasks accessing this
	 * address. In other words, this function check if the last task accessing an
	 * address has finished execution.
	 */
	extern std::map<kmp_intptr, std::pair<kmp_int32, std::vector<kmp_int32>>> dependenceTable;






	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//==------------------------------------------------------------------------------------===//
	//===--------------------------------------------------- Start of Function Prototypes --===//

	/**
	 *
	 * This function is called whenever a task is being removed from the task graph. Its job is
	 * to check if the task with id \c idOfFinishedTask is the last task accessing any memory
	 * parameter and if that is true it will stale that information, otherwise do nothing.
	 *
	 * @param idOfFinishedTask		ID of the task that finished execution.
	 * @param ndeps					Number of possible memory addresses that this task access.
	 * @param depList				List of possible memory addresse that this task access.
	 */
	void releaseDependencies(kmp_int16 idOfFinishedTask, kmp_uint32 ndeps, kmp_depend_info* depList);

	/**
	 * This function is called whenever a new task is being added to the task graph. Its job is to
	 * check if any memory address accessed by the new task (stored in \c depList) is already being 
	 * accessed by a previous in flight task. In other words, this function check if any address in
	 * \c depList has already an entry in \c dependenceTable.
	 *
	 * @param taskId	ID of the task that is going to be added to the graph.
	 * @param ndeps					Number of possible memory addresses that this task access.
	 * @param depList				List of possible memory addresse that this task access.
	 */
	kmp_uint64 checkAndUpdateDependencies(kmp_uint16 taskId, kmp_uint32 ndeps, kmp_depend_info* depList);

#endif
