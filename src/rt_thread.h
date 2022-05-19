#ifndef RT_THREAD_H
#define RT_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#define NS_PER_MS 1000000
#define NS_PER_US 1000
#define NS_PER_SEC 1000000000
#define US_PER_MS 1000
#define US_PER_SEC 1000000
#define MS_PER_SEC 1000

#define MAX_THREADS		20
typedef unsigned long long nsecType;

typedef struct taskType {

	bool 		in_use;			// structure is free or in use
	pthread_mutex_t mtx; 		// lock on the structure
	pthread_t 	tid;			// thread id
	void *		(*behaviour)(void *);	// behavior code
	void *		data;			// behavior-specific data
	int 		period; 		// period (ms)
	int 		deadline; 		// relative deadline (ms)
	int 		priority; 		// priority [0,99]
	int 		dl_missed;		// no. of deadline misses
	//unsigned long exe_time;		// Task Execution Time in us
	unsigned long iterations;	// # of iterations (inside execution phase)
	nsecType min_exe_time;	// Min Task Execution Time in us
	nsecType max_exe_time;
	double avg_exe_time;
	int 		exe_phase;		// How many 10-thousand executions have passed
	bool 		stopped;		// thread was ordered to stop
	struct timespec at; 		// next activ. time
	struct timespec dl; 		// abs. deadline
	} taskType;


void init_rt_thread_manager(void);

int start_thread(
		void *(*func)(void *),  // routine code
		void *args,		// routine args
		int policy,		// scheduling policy
		int prd,		// period (ms)
		int dl,			// relative deadline (ms)
		int prio 		// priority [0,99]
);

int stop_thread(unsigned int id);
int num_of_dl_misses(unsigned int id);
nsecType get_min_exec_time(unsigned int id);
nsecType get_max_exec_time(unsigned int id);
double get_avg_exec_time(unsigned int id);
int get_exec_phase(unsigned int id);
unsigned long int get_task_iteration(unsigned int id);

//me:
int ts_to_nsec(struct timespec *ts, nsecType *ns);
void ts_normalize(struct timespec *ts);
nsecType rt_gettime();

#endif
