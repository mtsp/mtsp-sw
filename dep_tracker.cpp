/**
 * @file 	dep_tracker.h
 * @brief 	This file contains functions and variables related to the Dependence Manager.
 *
 */
#include "dep_tracker.h"
#include "task_graph.h"
#include <utility>


std::map<kmp_intptr, std::pair<kmp_int32, std::vector<kmp_int32>>> dependenceTable;


void releaseDependencies(kmp_int16 idOfFinishedTask, kmp_uint32 ndeps, kmp_depend_info* depList) {
////	__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_Releasing_Dependences);

	// Iterate over each dependence and check if the task ID of the last task accessing that address
	// is the ID of the finished task. If it is remove that entry, otherwise do nothing.
	for (kmp_uint32 depIdx=0; depIdx<ndeps; depIdx++) {
		auto baseAddr 	= depList[depIdx].base_addr;

		// Is someone already accessing this address?
		if (dependenceTable.find(baseAddr) != dependenceTable.end()) {
			// If the finished task was a writer we check to see if it is still the last writer
			//		if yes we clear the first field. If not we do nothing yet.
			// If the finished task was a reader we disable that bit in the second field.
			if (depList[depIdx].flags.out) {
				if (dependenceTable[baseAddr].first == idOfFinishedTask) {
					dependenceTable[baseAddr].first = -1;
				}
			}
			else {
////				__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_Releasing_Dep_Reader);

				for (int idxPos=0; idxPos<dependenceTable[baseAddr].second.size(); idxPos++) {
					if (dependenceTable[baseAddr].second[idxPos] == idOfFinishedTask) {
						dependenceTable[baseAddr].second.erase( dependenceTable[baseAddr].second.begin() + idxPos);
						break;
					}
				}

////				__itt_task_end(__itt_mtsp_domain);
			}

			// If that address does not have more producers/writers we remove it from the hash
			// otherwise we just save the updated information
			if (dependenceTable[baseAddr].first == -1 && dependenceTable[baseAddr].second.size() == 0) {
				dependenceTable.erase(baseAddr);
			}
		}
	}

////	__itt_task_end(__itt_mtsp_domain);
}

kmp_uint64 checkAndUpdateDependencies(kmp_uint16 newTaskId, kmp_uint32 ndeps, kmp_depend_info* depList) {
////	__itt_task_begin(__itt_mtsp_domain, __itt_null, __itt_null, __itt_Checking_Dependences);

	kmp_uint64 depCounter = 0;

	// Iterate over each dependence
	for (kmp_uint32 depIdx=0; depIdx<ndeps; depIdx++) {
		auto baseAddr 	= depList[depIdx].base_addr;
		auto hashEntry 	= dependenceTable.find(baseAddr);
		bool isInput	= depList[depIdx].flags.in;
		bool isOutput	= depList[depIdx].flags.out;

		// <ID_OF_LAST_WRITER, IDS_OF_CURRENT_READERS>
		std::pair<kmp_int32, std::vector<kmp_int32>> hashValue;

		// Is someone already accessing this address?
		if (hashEntry != dependenceTable.end()) {
			hashValue = hashEntry->second;

			// If the new task is writing it must:
			//		if there are previous readers
			//			become dependent of all previous readers
			//		if there is no previous reader
			//			become dependent of the last writer
			//		become the new "producer", i.e., the last task writing
			//		reset the readers from the last writing
			if (isOutput) {
				if (hashValue.second.size() != 0) {
					// The new task become dependent on all previous readers
					for (auto& idReader : hashValue.second) {
						// If the new task does not already depends on the reader, it now becomes dependent
						if (whoIDependOn[newTaskId][idReader] == false) {
							whoIDependOn[newTaskId][idReader] = true;
							depCounter++;

							dependents[idReader][0]++;
							dependents[idReader][ dependents[idReader][0] ] = newTaskId;

#ifdef DEBUG_MODE
							if (dependents[idReader][0] >= MAX_DEPENDENTS) {
								printf("********* CRITICAL: Trying add more dependents than possible. [%s, %d].\n", __FILE__, __LINE__);
								exit(-1);
							}
#endif
						}
					}
				}
				else {
					kmp_int32 lastWriterId = hashValue.first;

					// If the new task does not already depends on the writer, it now becomes dependent
					if (whoIDependOn[newTaskId][lastWriterId] == false) {
						whoIDependOn[newTaskId][lastWriterId] = true;

						dependents[lastWriterId][0]++;
						dependents[lastWriterId][ dependents[lastWriterId][0] ] = newTaskId;
						depCounter++;

#ifdef DEBUG_MODE
						if (dependents[lastWriterId][0] >= MAX_DEPENDENTS) {
							printf("********* CRITICAL: Trying add more dependents than possible. [%s, %d].\n", __FILE__, __LINE__);
							exit(-1);
						}
#endif
					}
				}

				hashValue.first  = newTaskId;
				hashValue.second.clear();
			}
			else {
				// If we are reading:
				//		- if there was a previous producer/writer the new task become dependent
				//		-		if not, the new task does not have dependences
				//		- is always added to the set of last readers
				if (hashValue.first != -1) {
					kmp_int32 lastWriterId = hashValue.first;

					// If the new task does not already depends on the writer, it now becomes dependent
					if (whoIDependOn[newTaskId][lastWriterId] == false) {
						whoIDependOn[newTaskId][lastWriterId] = true;

						dependents[lastWriterId][0]++;
						dependents[lastWriterId][ dependents[lastWriterId][0] ] = newTaskId;
						depCounter++;
#ifdef DEBUG_MODE
						if (dependents[lastWriterId][0] >= MAX_DEPENDENTS) {
							printf("********* CRITICAL: Trying add more dependents than possible. [%s, %d].\n", __FILE__, __LINE__);
							__mtsp_dumpTaskGraphToDot();
							exit(-1);
						}
#endif
					}
				}

				hashValue.second.push_back(newTaskId);
			}
		}
		else {// Since there were no previous task acessing the addr
			  // the newTask does not have any dependence regarding this param

			// New task is writing to this address
			if (isOutput) {
				hashValue.first  = newTaskId;
				hashValue.second.clear();
			}
			else {// The new task is just reading from this address
				hashValue.first  = -1;
				hashValue.second.push_back(newTaskId);
			}
		}

		dependenceTable[baseAddr] = hashValue;
	}

////	__itt_task_end(__itt_mtsp_domain);
	return depCounter;
}
