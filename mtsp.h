/**
 * @file 	mtsp.h
 * @brief 	This file contains functions and variables that are used all over the project.
 *
 * This file contains the prototypes of functions and variables used all over the 
 * system, but mostly by the runtime thread itself. In this file there is also a
 * set of preprocessor defines used to specify the size of the several queues and
 * the task graph.
 */

#ifndef __MTSP_HEADER
	#define __MTSP_HEADER 1

	#include "kmp.h"
////	#include "ittnotify.h"

	#include <pthread.h>
	#include <vector>
	#include <map>
	#include <set>
	#include <utility>
	#include <iostream>
	#include <iomanip>


	//===----------------------------------------------------------------------===//
	//
	// Start of MTSP configuration directives
	//
	//===----------------------------------------------------------------------===//

	/// This is used to when we want to see the sequence of task types that were 
	/// entered in the submission queue
///	#define SUBQUEUE_PATTERN				1	

	/// Enable or Disable security checks (i.e., overflow on queues, etc.)
//	#define DEBUG_MODE						1
// 	#define DEBUG_COAL_MODE					1

	/// Enable the exportation of the whole task graph to .dot
	/// remember to reserve a large space for task graph, submission queue, run queue, etc.
///	#define TG_DUMP_MODE					1

	/// Activate (when undefined) or deactivate (when defined) ITTNotify Events
///	#define	INTEL_NO_ITTNOFIFY_API			1

	/// Uncomment if you want the CT to steal work
//	#define MTSP_WORKSTEALING_CT			1

	/// Uncomment if you want the RT to steal work
//	#define MTSP_WORKSTEALING_RT			1

	/// Uncomment if you want to see some statistics at the end of
	#define MTSP_DUMP_STATS					1

	/// Represents the maximum number of tasks that can be stored in the task graph
	#define MAX_TASKS 					     		       512
	#define MAX_DEPENDENTS						  	  MAX_TASKS

	/// Represents the maximum number of tasks in the Submission Queue
	#define SUBMISSION_QUEUE_SIZE			        4*MAX_TASKS
	#define SUBMISSION_QUEUE_BATCH_SIZE						  4
	#define SUBMISSION_QUEUE_CF	  			   				 50

	/// Represents the maximum number of tasks in the Run Queue
	#define RUN_QUEUE_SIZE							  MAX_TASKS
	#define RUN_QUEUE_CF			    		 			 50

	/// Represents the maximum number of tasks in the Retirement Queue
	#define RETIREMENT_QUEUE_SIZE								4*MAX_TASKS

	/// Maximum size of one taskMetadata slot. Tasks that require a metadata region
	/// larger than this will use a memory region returned by a call to std malloc.
	#define TASK_METADATA_MAX_SIZE  								   1024

	/// Number of task metadata slots
	#define MAX_TASKMETADATA_SLOTS 		(MAX_TASKS + SUBMISSION_QUEUE_SIZE)

	/// Just global constants recognized as \c LOCKED and \c UNLOCKED
	#define LOCKED															1
	#define UNLOCKED														0

	/// When we enable thread pinning these defines are used to say in which
	/// core the runtime and the control thread will stay
	#define __MTSP_MAIN_THREAD_CORE__										2
	#define __MTSP_RUNTIME_THREAD_CORE__									3




	/**
	 * The defines below are used as wrappers to GCC intrinsic functions. These
	 * are all related to atomic operations supported in x86-64.
	 * Some of the defines below use intrinsics relative to GCC/G++:
	 * https://gcc.gnu.org/onlinedocs/gcc-5.1.0/gcc/_005f_005fsync-Builtins.html
	 */
	#define	TRY_ACQUIRE(ptr)		__sync_bool_compare_and_swap(ptr, UNLOCKED, LOCKED)
	#define	ACQUIRE(ptr)			while (__sync_bool_compare_and_swap(ptr, UNLOCKED, LOCKED) == false)
	#define RELEASE(ptr)			__sync_bool_compare_and_swap(ptr, LOCKED, UNLOCKED)
	#define CAS(ptr, val1, val2)	__sync_bool_compare_and_swap(ptr, val1, val2)


	#define ATOMIC_ADD(ptr, val)	__sync_add_and_fetch(ptr, val)
	#define ATOMIC_SUB(ptr, val)	__sync_sub_and_fetch(ptr, val)
	#define ATOMIC_AND(ptr, val)	__sync_and_and_fetch(ptr, val)
	#define ATOMIC_OR(ptr, val)		__sync_or_and_fetch(ptr, val)




	//===----------------------------------------------------------------------===//
	//
	// Global variables and their locks, etc.
	//
	//===----------------------------------------------------------------------===//

	/// Tells whether the MTSP runtime has already been initialized or not.
	extern bool volatile __mtsp_initialized;

	/// This variable is used as a lock. The lock is used when initializing the MTSP runtime.
	extern unsigned char volatile __mtsp_lock_initialized;

	/// This variable is used as a lock. The lock is used in the \c kmpc_omp_single 
	extern bool volatile __mtsp_Single;

	/// This is the thread variable for the MTSP runtime thread.
	extern pthread_t __mtsp_RuntimeThread;

	/// This is a global counter used to give an unique identifier to all tasks
	/// executed by the system.
	extern kmp_uint32 __mtsp_globalTaskCounter;

	/// This is used together with \c __mtsp_globalTaskCounter for assigning/dumping
	/// the global unique ID of each task in the system.
	extern kmp_uint32 lastPrintedTaskId;

	/// Memory region from where new tasks metadata will be allocated.
	extern char __mtsp_taskMetadataBuffer[MAX_TASKMETADATA_SLOTS][TASK_METADATA_MAX_SIZE];

	/// Status of each slot in the \c __mtsp_taskMetadataBuffer
	extern volatile bool __mtsp_taskMetadataStatus[MAX_TASKMETADATA_SLOTS];


	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	// Variables related to the coalescing framework
	
	/// Used to store the harmonic average execution time of tasks, runtime and coalescing
	/// routines. The key is any integer, usually the address of the task or routine.
	/// The value is pair [integer:double]. The first of these is the number of items that
	/// were averaged, the second is the value of the average.
	extern std::map<kmp_uint64, std::pair<kmp_uint64, double>> taskSize;
	extern std::map<kmp_uint64, bool> realTasks;

	// Tasks that have they address here will not be considered for coalescing anymore
	extern std::set<kmp_uint64> coalBlacklist;

	// This will store parameter addresses in order to compute how many
	// unique addresses there are inside a coalesce
	extern std::set<kmp_uint64> uniqueAddrs;
	extern double __coalHowManyAddrs;

	// This is the task that store the coalesced subtasks
	extern kmp_task* coalescedTask;

	/// This is the size of the current coalescing being constructed.
	extern kmp_int16 __curCoalesceSize;

	// This is the current linearity factor
	extern double __curCoalL;

	// This is the target linearity factor
	extern double __tgtCoalL;

	/// the number of times that coalescing was deemed necessary. I.e., the system can
	/// and should amortize the execution of tasks.
	extern kmp_int64 __coalNecessary;

	/// the number of times that coalescing was deemed unnecessary. I.e., the task size
	/// is sufficiently large to amortize the runtime overhead.
	extern kmp_int64 __coalUnnecessary;

	/// the number of times that coalescing was deemed impossible. I.e., the overhead
	/// of the runtime and the coalescing framework is too high for this target task
	/// size and number of worker threads.
	extern kmp_int64 __coalImpossible;

	/// the number of times that coalescing was deemed overflowed. I.e., the target
	/// size of the coalescing was greater than the allocated resources.
	extern kmp_int64 __coalOverflowed;

	/// Number of times that was possible to create a full coalescing. I.e., the
	/// \c __curCoalesceSize reached \c __curTargetCoalescingSize
	extern kmp_int64 __coalSuccess;

	/// Number of times that was not possible to complete the coalescing. I.e., 
	/// \c __curCoalesceSize did not reached \c __curTargetCoalescingSize because
	/// there was not sufficiently equal tasks in a row.
	extern kmp_int64 __coalFailed;


	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	// These vars are used to interact with VTune
	
	/// ITTNotify domain of events/tasks/frames
////	extern __itt_domain* 		volatile __itt_mtsp_domain;

	/// Labels for itt-events representing the duration of a fork-call
////	extern __itt_string_handle* volatile __itt_CT_Fork_Call;

	/// Labels for itt-events representing enqueue and dequeue from the ready tasks queue
////	extern __itt_string_handle* volatile __itt_Run_Queue_Dequeue;
////	extern __itt_string_handle* volatile __itt_Run_Queue_Enqueue;

	/// Labels for itt-events representing enqueue and dequeue from the new tasks queue
////	extern __itt_string_handle* volatile __itt_Submission_Queue_Dequeue;
////	extern __itt_string_handle* volatile __itt_Submission_Queue_Enqueue;
////	extern __itt_string_handle* volatile __itt_Submission_Queue_Copy;
////	extern __itt_string_handle* volatile __itt_Submission_Queue_Add;

	/// Labels for itt-events representing enqueue and dequeue from the finished tasks queue
////	extern __itt_string_handle* volatile __itt_Retirement_Queue_Dequeue;
////	extern __itt_string_handle* volatile __itt_Retirement_Queue_Enqueue;

	/// Labels for itt-events representing periods where a new task was being added/deleted to/from the task graph
////	extern __itt_string_handle* volatile __itt_TaskGraph_Add;
////	extern __itt_string_handle* volatile __itt_TaskGraph_Del;

	/// Labels for itt-events representing periods where the dependence checker was checking/releasing dependences
////	extern __itt_string_handle* volatile __itt_Checking_Dependences;
////	extern __itt_string_handle* volatile __itt_Releasing_Dependences;
////	extern __itt_string_handle* volatile __itt_Releasing_Dep_Reader;

	/// Labels for itt-events representing periods where the control thread was waiting in a taskwait barrier
////	extern __itt_string_handle* volatile __itt_CT_Barrier_Wait;

	/// Labels for itt-events representing periods where the control thread was executing task_alloc
////	extern __itt_string_handle* volatile __itt_CT_Task_Alloc;

	/// Labels for itt-events representing periods where the control thread was executing task_with_deps
////	extern __itt_string_handle* volatile __itt_CT_Task_With_Deps;

	/// Labels for itt-events representing periods where an worker thread was waiting in a taskwait barrier
////	extern __itt_string_handle* volatile __itt_WT_Barrier;
////	extern __itt_string_handle* volatile __itt_WT_Serving_Steal;

	/// Label for itt-events representing periods where an worker thread was waiting for tasks to execute
////	extern __itt_string_handle* volatile __itt_WT_Wait_For_Work;

	/// Label for itt-events representing periods where an worker thread was executing a task
////	extern __itt_string_handle* volatile __itt_Coalescing;
////	extern __itt_string_handle* volatile __itt_Coal_In_Execution;
////	extern __itt_string_handle* volatile __itt_Task_In_Execution;

	/// Label for itt-events representing periods where an thread stealing tasks
////	extern __itt_string_handle* volatile __itt_Task_Stealing;

	/// Label for itt-events representing the blocking of a access to the spsc queue
////	extern __itt_string_handle* volatile __itt_SPSC_Enq_Blocking;
////	extern __itt_string_handle* volatile __itt_SPSC_Deq_Blocking;

////	extern __itt_string_handle* volatile __itt_SPSC_Enq;
////	extern __itt_string_handle* volatile __itt_SPSC_Deq;

	/// Labels for the runtime events. Check for adding/removing tasks from the queues and the graph.
	/// and others stuffs
////	extern __itt_string_handle* volatile __itt_RT_Main_Loop;
////	extern __itt_string_handle* volatile __itt_RT_Check_Del;
////	extern __itt_string_handle* volatile __itt_RT_Check_Add;
////	extern __itt_string_handle* volatile __itt_RT_Check_Oth;



	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////
	


	// For reading the RDTSCP counter in the x86 architecture. You must call these
	// functions in the order "beg" (target_region_of_interest) "end".
	/// Used to read MTSP at the beggining of a region
	unsigned long long end_read_mtsp();

	/// Used to read MTSP at the beggining of a region
	unsigned long long beg_read_mtsp();

	/**
	 * When thread pinning is enabled this is the function that all threads call
	 * in order to be pinned to a desired core.
	 *
	 * @param core_id		ID of the core that the threads wants to execute on.
	 */
	int stick_this_thread_to_core(const char* const pref, int core_id);

	/**
	 * This function is executed every time that MTSP enters a new parallel region.
	 * It is responsible for re-initializing all global variables/data structures.
	 */
	void __mtsp_reInitialize();

	/**
	 * This function is the one responsible for initializing the MTSP runtime itself.
	 * It is called from inside the \c __kmpc_fork_call by the control thread the first
	 * time it encounters a parallel region. This function will call another functions
	 * that will initialize the Dependence Manager, Task Graph Manager, Scheduler and
	 * create the Worker Threads according to the environment variable OMP_NUM_THREADS.
	 */
	void __mtsp_initialize();

	/**
	 * This function is the one that actually enqueue a task in the submission queue.
	 * It is executed by the control thread.
	 *
	 * @param newTask		A pointer to a task structure returned by \c kmpc_omp_task_alloc
	 * @param ndeps			The number of memory addresses this task access
	 * @param depList		The list of memory addresses this task access
	 */
	void __mtsp_addNewTask(kmp_task* newTask, kmp_uint32 ndeps, kmp_depend_info* depList);

	/**
	 * This function is the one executed by the runtime thread itself. The parameter is
	 * just boiler plate code used by the pthread API and is not used for anything. The
	 * body of the function is mostly composed of a \c while loop with two parts: 1) removing
	 * tasks from the retirement queue and from the task graph; 2) removing task creation
	 * requests from the submission queue and adding to the task graph.
	 *
	 * @param	params		Used for nothing.
	 * @return 				nullptr.
	 */
	void* __mtsp_RuntimeThreadCode(void* params);

	/**
	 * This function is the one used for updating the average execution time of any piece of
	 * code. The function do not keep track of the history of execution times, it uses a
	 * simple algebra to update the average.
	 *
	 * @param taskAddr	Identification of the code snippet.
	 * @param size		Execute time (in cycles) of the code.
	 */
	void updateAverageTaskSize(kmp_uint64 taskAddr, double size);

	/**
	 * This function is an "artificial task" used to execute a group of tasks that have
	 * been coalesced together.
	 *
	 * @param notUsed	Currently useless. Just Clang boiler plate.
	 * @param param		Used to pass all parameters to the task.
	 */
	extern int executeCoalesced(int notUsed, void* param);

	/**
	 * This function is used when the runtime wants to add a new coalesced task to the
	 * task graph. That is, the coalescing framework decided to coalesce a sequence of
	 * tasks and subsequently created a group of tasks (i.e., a coalesce) and now wants
	 * to add this "macro task" to the task graph.
	 *
	 * @param coalescedTask		A pointer to the "artifical task" that will execute a group of subtasks that were coalesced together.
	 */
	void saveCoalesce();

#endif
