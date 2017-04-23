#ifndef __MTSP_KMP_HEADER
	#define __MTSP_KMP_HEADER 1

	#include <stddef.h>
	#include <vector>



	//===----------------------------------------------------------------------===//
	//
	// Start of global define declarations
	//
	//===----------------------------------------------------------------------===//


	/// NOTE: kmp_taskdata_t and kmp_task_t structures allocated in single block with taskdata first
	/// These defines are remanescent from IRTL
	#define KMP_TASK_TO_TASKDATA(task)     		(((kmp_taskdata *) task) - 1)
	#define KMP_TASKDATA_TO_TASK(taskdata) 		(kmp_task *) (taskdata + 1)


	#define MAX_COALESCING_SIZE					  20
	#define MIN_SAMPLES_FOR_COALESCING			  10
	#define MIN_SAMPLES_AVERAGE					   3




	//===----------------------------------------------------------------------===//
	//
	// Start of global type declarations
	//
	//===----------------------------------------------------------------------===//

	/// Type aliases; just trying to easy things for the future
	typedef char               	kmp_int8;
	typedef unsigned char      	kmp_uint8;
	typedef short              	kmp_int16;
	typedef unsigned short     	kmp_uint16;
	typedef int                	kmp_int32;
	typedef unsigned int       	kmp_uint32;
	typedef long long          	kmp_int64;
	typedef unsigned long long 	kmp_uint64;
	typedef long             	kmp_intptr;


	typedef void (*kmpc_micro) (kmp_int32 * global_tid, kmp_int32 * local_tid, ...);
	typedef kmp_int32 (* kmp_routine_entry)(kmp_int32, void *);




	/// This is used to describe the properties of a task parameter
	typedef struct {
		kmp_intptr			base_addr;			/*< Base address of the parameter */
		size_t              len;				/*< Declared size of the memory region accessed */
		struct {								/*< The fields below aren't exclusive! */
			bool          	in:1;				/*< Parameter is used for reading. */
			bool          	out:1;				/*< Parameter is used for writing. */
		} flags;
	} kmp_depend_info;



	/// Used for describing an annotated source code block.
	typedef struct {
	    kmp_int32 reserved_1;   	/**< Currently not used in MTSP. */
	    kmp_int32 reserved_2;   	/**< Currently not used in MTSP. */
	    kmp_int32 reserved_3;   	/**< Currently not used in MTSP. */
	    kmp_int32 flags;        	/**< Currently not used in MTSP. */
	    char const *psource;    	/**< String describing the source location.
	                            	The string is composed of semi-colon separated fields which describe the source file,
	                            	the function and a pair of line numbers that delimit the construct. */
	} ident;

	/// This structure is used by MTSP to store metadata information related to a task
	typedef struct _mtsp_task_metadata {
		kmp_int16 metadata_slot_id;
		kmp_int16 taskgraph_slot_id;
		kmp_int32 ndeps;
		kmp_depend_info* dep_list;
		kmp_uint32 globalTaskId;
		kmp_uint64 taskSize;
		kmp_uint64 coalesceSize;
		struct _kmp_task* coalesced[MAX_COALESCING_SIZE];
	} mtsp_task_metadata;


	/// Used for representing a task invocation. The shareds field points to a list of pointers to shared variables.
	/// The "routine" field point to a ptask routine.
	typedef struct _kmp_task {
	    void *              	shareds;            /**< pointer to block of pointers to shared vars   */
	    kmp_routine_entry 		routine;            /**< pointer to routine to call for executing task */
	    mtsp_task_metadata*		metadata;        	/**< [MTSP] Actually this point to a metadata structure used by MTSP.  */
	    kmp_int32           	_was_part_id;       /**< Currently not used in MTSP. */
	    /*  private vars  */
	} kmp_task;


	/// This one is actually not used by MTSP
	typedef struct {          					/* Total struct must be exactly 32 bits */
	    /* Compiler flags */                    /* Total compiler flags must be 16 bits */
	    unsigned tiedness    : 1;               /* task is either tied (1) or untied (0) */
	    unsigned final       : 1;               /* task is final(1) so execute immediately */
	    unsigned merged_if0  : 1;               /* no __kmpc_task_{begin/complete}_if0 calls in if0 code path */
	    unsigned destructors_thunk : 1;         /* set if the compiler creates a thunk to invoke destructors from the runtime */
	    unsigned reserved    : 12;              /* reserved for compiler use */

	    /* Library flags */                     /* Total library flags must be 16 bits */
	    unsigned tasktype    : 1;               /* task is either explicit(1) or implicit (0) */
	    unsigned task_serial : 1;               /* this task is executed immediately (1) or deferred (0) */
	    unsigned tasking_ser : 1;               /* all tasks in team are either executed immediately (1) or may be deferred (0) */
	    unsigned team_serial : 1;               /* entire team is serial (1) [1 thread] or parallel (0) [>= 2 threads] */
	                                            /* If either team_serial or tasking_ser is set, task team may be NULL */
	    /* Task State Flags: */
	    unsigned started     : 1;               /* 1==started, 0==not started     */
	    unsigned executing   : 1;               /* 1==executing, 0==not executing */
	    unsigned complete    : 1;               /* 1==complete, 0==not complete   */
	    unsigned freed       : 1;               /* 1==freed, 0==allocateed        */
	    unsigned native      : 1;               /* 1==gcc-compiled task, 0==intel */
	    unsigned reserved31  : 7;               /* reserved for library use */
	} kmp_tasking_flags;


	/// Used to store metadata related to task execution/context
	/// Currently not used in MTSP
	typedef struct _kmp_taskdata {
		kmp_int32               	td_task_id;               		/* id, assigned by debugger                */
		kmp_tasking_flags     		td_flags;                 		/* task flags                              */

		struct _kmp_taskdata* 		td_parent;                		/* parent task                             */
		kmp_int32               	td_level;                 		/* task nesting level                      */
		ident*               		td_ident;                 		/* task identifier                         */

		ident*               		td_taskwait_ident;
		kmp_uint32              	td_taskwait_counter;
		kmp_int32               	td_taskwait_thread;       		/* gtid + 1 of thread encountered taskwait */

		volatile kmp_uint32     	td_allocated_child_tasks;  		/* Child tasks (+ current task) not yet deallocated */
		volatile kmp_uint32     	td_incomplete_child_tasks; 		/* Child tasks not yet complete */
	} kmp_taskdata;


	extern "C" {
		void 		__kmpc_fork_call(ident *loc, kmp_int32 argc, kmpc_micro microtask, ...);
		kmp_task* 	__kmpc_omp_task_alloc(ident *loc, kmp_int32 gtid, kmp_int32 pflags, kmp_uint32 sizeof_kmp_task_t, kmp_uint32 sizeof_shareds, kmp_routine_entry task_entry);
		kmp_int32 	__kmpc_omp_task_with_deps(ident* loc, kmp_int32 gtid, kmp_task* new_task, kmp_int32 ndeps, kmp_depend_info* dep_list, kmp_int32 ndeps_noalias, kmp_depend_info* noalias_dep_list);
		kmp_int32 	__kmpc_omp_taskwait(ident* loc, kmp_int32 gtid);
		kmp_int32 	__kmpc_cancel_barrier(ident* loc, kmp_int32 gtid);
		kmp_int32 	__kmpc_single(ident* loc, kmp_int32 gtid);
		void 		__kmpc_end_single(ident* loc, kmp_int32 gtid);
		kmp_int32 	__kmpc_master(ident* loc, kmp_int32 gtid);
		void 		__kmpc_end_master(ident* loc, kmp_int32 gtid);
	}

#endif // if __MTSP_KMP_HEADER
