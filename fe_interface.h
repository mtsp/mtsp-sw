/**
 * @file 	fe_interface.h
 * @brief 	This file contains the functions and variables that is used by the front-end of MTSP.
 *
 * In this file you will find the prototype of the functions that are used as an
 * interface between the serial application (i.e., the main application or the control thread)
 * and the runtime thread (or the MTSP module itself).
 */
#ifndef __MTSP_FE_INTERFACE_HEADER
	#define __MTSP_FE_INTERFACE_HEADER 1

	#include "kmp.h"
	#include "ThreadedQueue.h"


	extern kmp_uint64 tasksExecutedByCT;
	extern kmp_uint64 metadataRequestsNotServiced;
	extern volatile kmp_uint64 tasksExecutedByRT;

	extern SPSCQueue<kmp_task*, SUBMISSION_QUEUE_SIZE, SUBMISSION_QUEUE_BATCH_SIZE, SUBMISSION_QUEUE_CF> submissionQueue;


	/**
	* @brief	This function represents the initialization of a new OMP parallel region. 
	*
	* 			This is the first function called when a new OMP parallel region start
	* 			execution. Therefore, there are two main responsabilities of this function:
	* 			1) initialize the MTSP runtime module if this has not been done before, 
	* 			2) parse the \c var_arg list and forward the execution to the function 
	* 			pointed by \c kmpc_micro.
	*
	* @param 	loc  		Source location of the omp construct.
	* @param 	argc  		Total number of arguments in the ellipsis.
	* @param 	microtask  	Pointer to callback routine (usually .omp_microtask) consisting of outlined "omp parallel" construct
	* @param 	va_arg 		Pointers to shared variables that aren't global
	*/
	void __kmpc_fork_call(ident *loc, kmp_int32 argc, kmpc_micro microtask, ...);



	/**
	* @brief	This function is responsible for allocating memory for storing task parameters and metadata.
	*
	*			The task structure, the task metadata and the task private/shared members are allocated with
	*			just one call to malloc or taken from a pre-allocated memory region (i.e., it tries to reduce
	*			the number of calls to malloc). These structs are contiguously allocated in memory. Macros are
	*			used to cast pointers to these structures.
	*
	* @param 	loc					Source location of the omp construct.
	* @param 	gtid				Global id of the thread.
	* @param 	pflags				This always came as "1" from the compiler.
	* @param 	sizeof_kmp_task_t	This include the sizeof(kmp_task)+8 (i.e., 40bytes) + the space required by private vars.
	* @param 	sizeof_shareds		Size of shared memory region.
	* @param 	task_entry			Pointer to ptask wrapper routine.
	*
	* @return	Returns a pointer to the newly allocated task+metadata.
	*/
	kmp_task* __kmpc_omp_task_alloc(ident *loc, kmp_int32 gtid, kmp_int32 pflags, kmp_uint32 sizeof_kmp_task_t, kmp_uint32 sizeof_shareds, kmp_routine_entry task_entry);



	/**
	 * @brief	This function add a task creation request to the Submission Queue.
	 *
	 * 			This function forward the \c new_task structure returned by \c __kmpc_omp_task_alloc
	 *			again to the runtime module, but this time in the form of a request to add a new task
	 *			to the task graph. In other words, this function is used by the control thread
	 *			to ask the runtime to add a new task to the task graph.
	 *
	 * @param 	loc 				Location of the original task directive
	 * @param 	gtid 				Global thread ID of encountering thread
	 * @param 	new_task 			task thunk allocated by __kmp_omp_task_alloc() for the ''new task''
	 * @param 	ndeps 				Number of depend items with possible aliasing
	 * @param 	dep_list 			List of depend items with possible aliasing
	 * @param 	ndeps_noalias 		Number of depend items with no aliasing
	 * @param 	noalias_dep_list 	List of depend items with no aliasing
	 *
	 * @return 						Always returns zero.
	 */
	kmp_int32 __kmpc_omp_task_with_deps(ident* loc, kmp_int32 gtid, kmp_task* new_task, kmp_int32 ndeps, kmp_depend_info* dep_list, kmp_int32 ndeps_noalias, kmp_depend_info* noalias_dep_list);



	/**
	 * @brief	This make the current thread to pause execution until all its subtasks have finished execution. 
	 *
	 *			In the current implementation this function represents a barrier. The control thread will
	 *			pause execution until all worker threads have reached this barrier. The worker threads only
	 *			get inside this barrier after all in flight tasks in the system has been executed.
	 *
	 *  @param	loc 	Location of the original task directive
	 *  @param	gtid 	Global thread ID of encountering thread
	 *
	 *  @return			LLVM currently ignores the return of this function.
	 */
	kmp_int32 __kmpc_omp_taskwait(ident* loc, kmp_int32 gtid);



	/**
	 * @brief	Lock/Barrier related function. Currently not used in MTSP.
	 *
	 * @return 	LLVM currently ignores the return of this function.
	 */
	kmp_int32 __kmpc_cancel_barrier(ident* loc, kmp_int32 gtid);




	/**
	 * @brief	Test whether to execute a \c single construct.
	 *
	 * @return 	LLVM currently ignores the return of this function.
	 */
	kmp_int32 __kmpc_single(ident* loc, kmp_int32 gtid);




	/**
	 * @brief	Lock/Barrier related function. Currently not used in MTSP.
	 *
	 * @return 	LLVM currently ignores the return of this function.
	 */
	void __kmpc_end_single(ident* loc, kmp_int32 gtid);




	/**
	 * @brief	Should return 1 if the thread with ID \c gtid should execute the 
	 * 			\c master block, 0 otherwise.
	 *
	 * @return 	1 if the thread with ID \c gtid should execute the \c master block, 0 otherwise.
	 */
	kmp_int32 __kmpc_master(ident* loc, kmp_int32 gtid);




	/**
	 * @brief	Lock/Barrier related function. Currently not used in MTSP.
	 *
	 * @return 	LLVM currently ignores the return of this function.
	 */
	void __kmpc_end_master(ident* loc, kmp_int32 gtid);


	extern "C" {
		/**
		 * @brief 	This function is from the OMP C API and it returns the
		 * 			total number of threads currently under OMP (MTSP) control.
		 *
		 * @return 	The number of threads currently in the system.
		 */
		int omp_get_num_threads();
	}
#endif
