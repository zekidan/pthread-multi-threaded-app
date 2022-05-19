#include <stdio.h>
#include <string.h>
#include <math.h>

#include "rt_thread.h"
#include "multimedia.h"

taskType rt_threads[MAX_THREADS];		// container for thread descriptors
unsigned int active_rt_threads = 0;		// number of currently active threads
pthread_mutex_t rt_threads_mtx = PTHREAD_MUTEX_INITIALIZER;	// mutex to protect rt_threads

static unsigned short int count=0;

static nsecType tot_min_exe = 0;
static nsecType tot_max_exe = 0;
static double tot_avg_exe = 0;


/* ======================================
*  ============== UTILITY ===============
*  ====================================== */

/* Copy time data structure */
void time_copy(struct timespec *const dst, struct timespec src) {
	
	dst->tv_sec  = src.tv_sec;
	dst->tv_nsec = src.tv_nsec;
}


/* Add an interval (in milliseconds) to a time data structure */
void time_add_ms(struct timespec *const t, int ms) {

	t->tv_sec += ms/1000;
	t->tv_nsec += (ms%1000)*1000000;
	if (t->tv_nsec > 1000000000) {
		t->tv_nsec -= 1000000000;
		t->tv_sec += 1;
	}
}


/* Compare two time data structures */
int time_cmp(struct timespec t1, struct timespec t2) {
	
	if (t1.tv_sec > t2.tv_sec) return 1;
	if (t1.tv_sec < t2.tv_sec) return -1;
	if (t1.tv_nsec > t2.tv_nsec) return 1;
	if (t1.tv_nsec < t2.tv_nsec) return -1;
	return 0;
}

void ts_normalize(struct timespec *ts)
{
        if (ts == NULL) {
                printf("ERROR: ts is NULL\n");
                return;
        }

        /* get the abs(nsec) < NS_PER_SEC */
        while (ts->tv_nsec > NS_PER_SEC) {
                ts->tv_sec++;
                ts->tv_nsec -= NS_PER_SEC;
        }
        while (ts->tv_nsec < -NS_PER_SEC) {
                ts->tv_sec--;
                ts->tv_nsec += NS_PER_SEC;
        }

        /* get the values to the same polarity */
        if (ts->tv_sec > 0 && ts->tv_nsec < 0) {
                ts->tv_sec--;
                ts->tv_nsec += NS_PER_SEC;
        }
        if (ts->tv_sec < 0 && ts->tv_nsec > 0) {
                ts->tv_sec++;
                ts->tv_nsec -= NS_PER_SEC;
        }
}

int ts_to_nsec(struct timespec *ts, nsecType * ns)
{
        struct timespec t;
        if (ts == NULL) {
                printf("ERROR: ts is NULL\n");
                return -1;
        }
        t.tv_sec = ts->tv_sec;
        t.tv_nsec = ts->tv_nsec;
        ts_normalize(&t);

        if (t.tv_sec <= 0 && t.tv_nsec < 0) {
                printf("ERROR: ts is negative\n");
                return -1;
        }

        *ns = (nsecType) ts->tv_sec * NS_PER_SEC + ts->tv_nsec;
        return 0;
}



nsecType rt_gettime(void)
{
        struct timespec ts;
        nsecType ns;
        int rc;

        rc = clock_gettime(CLOCK_MONOTONIC, &ts);
        if (rc != 0) {
                printf("ERROR: clock_gettime() returned %d\n", rc);
               // perror("clock_gettime() failed");
                return 0;
        }

        ts_to_nsec(&ts, &ns);// convert to nanosecond
        return ns;
}

/* ======================================
*  ============= THREADING ==============
*  ====================================== */


/* Initialize thread pool descriptors */
void init_rt_thread_manager(void) {

	pthread_mutex_lock(&rt_threads_mtx);

	active_rt_threads = 0;

	// All threads are initially marked as idle
	for (int i = 0; i < MAX_THREADS; ++i) {
		rt_threads[i].in_use = false;
	}

	pthread_mutex_unlock(&rt_threads_mtx);
}


/* Allocates a previously unused thread descriptor and returns its index.
*  If none is free, return MAX_THREADS. */
unsigned int allocate_task_id(void) {

	unsigned int i = 0;

	pthread_mutex_lock(&rt_threads_mtx);

	if (active_rt_threads == MAX_THREADS) {
		pthread_mutex_unlock(&rt_threads_mtx);
		return MAX_THREADS;
	}

	while (rt_threads[i].in_use)
		++i;

	rt_threads[i].in_use = true;
	pthread_mutex_init(&rt_threads[i].mtx, NULL);
	++active_rt_threads;

	pthread_mutex_unlock(&rt_threads_mtx);
	return i;
}


/* Frees the descriptor of a finished thread. */
void deallocate_task_id(unsigned int id) {

	pthread_mutex_lock(&rt_threads_mtx);

	--active_rt_threads;
	rt_threads[id].in_use = false;

	pthread_mutex_destroy(&rt_threads[id].mtx);
	pthread_mutex_unlock(&rt_threads_mtx);
}


/* Get the identifier (index) of a task. */
static inline int get_task_id(taskType *const task) {

	return task - rt_threads;
}


/* Initialize pthread_attr_t structure */
int init_sched_attr(pthread_attr_t *const attr, int policy, int prio) {

	struct sched_param sp;
	int err;

	err = pthread_attr_init(attr);
	if (err)
		return 1;

	err = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
	if (err)
		return 2;

	err = pthread_attr_setschedpolicy(attr, policy);
	if (err)
		return 3;

	sp.sched_priority = prio;
	err = pthread_attr_setschedparam(attr, &sp);
	if (err)
		return 4;

	return 0;
}


/* Set up the first activation time and deadline of the task */
void set_activation(taskType *const task) {

	struct timespec t;

	pthread_mutex_lock(&task->mtx);/*...may be unnecessary..*/

	clock_gettime(CLOCK_MONOTONIC, &t);
	time_copy(&(task->at), t);
	time_copy(&(task->dl), t);
	time_add_ms(&(task->at), task->period);
	time_add_ms(&(task->dl), task->deadline);

	pthread_mutex_unlock(&task->mtx);
}


/* Return true if the last deadline has been missed */
bool is_deadline_missed(taskType *const task) {

	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (time_cmp(now, task->dl) > 0) {
		++task->dl_missed;

		nsecType t_end,start;
		struct timespec t_start = (struct timespec)(task->dl);
		ts_to_nsec(&t_start,&start);
	    ts_to_nsec(&now,&t_end);
		nsecType delta = (t_end-start)/NS_PER_US ;
					printf("TH%d deadline missed!Missed deadline by:%lldµs\n ",get_task_id(task),delta);
		return true;
	}
	return false;
}


/* Return the total number of deadlines missed by this task */
int num_of_dl_misses(unsigned int id) {

	return rt_threads[id].dl_missed;
}

nsecType get_min_exec_time(unsigned int id){

	return rt_threads[id].min_exe_time;
}

nsecType get_max_exec_time(unsigned int id){

	return rt_threads[id].max_exe_time;
}

double get_avg_exec_time(unsigned int id){

	return rt_threads[id].avg_exe_time;
}
unsigned long int get_task_iteration(unsigned int id){
	return rt_threads[id].iterations;
}


int get_exec_phase(unsigned int id){

	return rt_threads[id].exe_phase;
}



/* Sleep until next activation of the task */
void wait_and_set_nextActivation(taskType *const task) {

	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(task->at), NULL);
	time_add_ms(&(task->at), task->period);
	time_add_ms(&(task->dl), task->period);
}


void *rt_thr_body(void *const arg) {

	struct timespec t;

	nsecType exe_start, exe_end, exe_time;
	nsecType avg_exe_time = 0, min_exe_time = -1ULL, max_exe_time = 0;

	taskType *task = (taskType *)arg;
	int tid = get_task_id(task);

	set_activation(task);

	while (true) {
		pthread_mutex_lock(&task->mtx);

		// Stop the thread if requested
		if (task->stopped) {
			pthread_mutex_unlock(&task->mtx);
			break;
		}
		/*Execution measurement code goes here */

		exe_start=rt_gettime();

		task->behaviour(task->data);// Execute the instance-specific code

		exe_end=rt_gettime();
		exe_time = exe_end-exe_start;// IF task is preempted this value is response time

		if(task->iterations > 10000 &&(tid!=graphics_tid ||
				tid!=keyboard_tid||tid!=mouse_tid)){
			task->avg_exe_time=(double)avg_exe_time/(10000);
			//print statistics in microseconds
			printf("trd%d\t\t %lld\t\t %lld\t\t %.2f \t\t%d\n",tid,task->min_exe_time/NS_PER_US,
					task->max_exe_time/NS_PER_US,task->avg_exe_time/NS_PER_US,task->dl_missed);

			//pthread_mutex_lock(&rt_threads_mtx);//shared Data Structure...
												//this code must be executed atomically
			tot_min_exe += task->min_exe_time;
			tot_max_exe += task->max_exe_time;
			tot_avg_exe += task->avg_exe_time;

			count++;
			if(count==active_rt_threads-3){
				printf("\tsum_min = %lld\tsum_max = %lld\tsum_avg = %.2f\n\n",
						tot_min_exe/NS_PER_US,tot_max_exe/NS_PER_US,tot_avg_exe/NS_PER_US);
				printf("\tEXECUTION STATISTICS FOR MULTIMEDIA THREADS\n\n");

				rt_threads[graphics_tid].avg_exe_time/=rt_threads[graphics_tid].iterations;
				rt_threads[keyboard_tid].avg_exe_time/=rt_threads[keyboard_tid].iterations;
				rt_threads[mouse_tid].avg_exe_time/=rt_threads[mouse_tid].iterations ;

				tot_min_exe = tot_min_exe + rt_threads[graphics_tid].min_exe_time +
						rt_threads[keyboard_tid].min_exe_time +
						rt_threads[mouse_tid].min_exe_time;
				tot_max_exe = tot_max_exe + rt_threads[graphics_tid].max_exe_time +
						rt_threads[keyboard_tid].max_exe_time +
						rt_threads[mouse_tid].max_exe_time;
				tot_avg_exe = tot_avg_exe + rt_threads[graphics_tid].avg_exe_time +
						rt_threads[keyboard_tid].avg_exe_time +
						rt_threads[mouse_tid].avg_exe_time;

				printf("g_trd%d\t\t %lld\t\t %lld\t\t %.2f \t%d\n",
						graphics_tid,rt_threads[graphics_tid].min_exe_time/NS_PER_US,
						rt_threads[graphics_tid].max_exe_time/NS_PER_US,
						rt_threads[graphics_tid].avg_exe_time/NS_PER_US,
						rt_threads[graphics_tid].dl_missed);
				printf("k_trd%d\t\t %lld\t\t %lld\t\t %.2f \t\t%d\n",
						keyboard_tid,rt_threads[keyboard_tid].min_exe_time/NS_PER_US,
						rt_threads[keyboard_tid].max_exe_time/NS_PER_US,
						rt_threads[keyboard_tid].avg_exe_time/NS_PER_US,
						rt_threads[keyboard_tid].dl_missed);
				printf("m_trd%d\t\t %lld\t\t %lld\t\t %.2f \t\t%d\n",
						mouse_tid,rt_threads[mouse_tid].min_exe_time/NS_PER_US,
						rt_threads[mouse_tid].max_exe_time/NS_PER_US,
						rt_threads[mouse_tid].avg_exe_time/NS_PER_US,
						rt_threads[mouse_tid].dl_missed);

				rt_threads[graphics_tid].avg_exe_time = 0;
				rt_threads[graphics_tid].dl_missed = 0;


				rt_threads[keyboard_tid].avg_exe_time = 0;
				rt_threads[keyboard_tid].dl_missed = 0;

				rt_threads[mouse_tid].avg_exe_time = 0;
				rt_threads[mouse_tid].dl_missed = 0;

				rt_threads[graphics_tid].iterations = 0;
				rt_threads[keyboard_tid].iterations = 0;
				rt_threads[mouse_tid].iterations = 0;

				printf("\tTot_min = %lld\tTot_max = %lld\tTot_avg = %.2f\n\n",
						tot_min_exe/NS_PER_US,tot_max_exe/NS_PER_US,tot_avg_exe/NS_PER_US);

				//pthread_mutex_unlock(&rt_threads_mtx);

				printf("--------------------------------------------------"
						"--------------------\n");
				count=0;
				tot_min_exe = 0;
				tot_max_exe = 0;
				tot_avg_exe = 0;
			}
			++task->exe_phase;

			min_exe_time = max_exe_time = avg_exe_time=exe_time;
			task->iterations=0;
			task->dl_missed=0;
		}

		else{
			if (exe_time < min_exe_time){
				min_exe_time = exe_time;
				task->min_exe_time=min_exe_time;

			}
			if (exe_time > max_exe_time){
				max_exe_time = exe_time;
				task->max_exe_time=max_exe_time;
			}

			if(tid==graphics_tid || tid==keyboard_tid || tid==mouse_tid)
			{
				//nsecType t_ns = exe_time;
				task->avg_exe_time += (double)exe_time;
			}
			else
				avg_exe_time += exe_time;

			++task->iterations;
		}

		// Check for deadline miss
		if (is_deadline_missed(task)) {
			// < Corrective actions (optional) shall be added here >

		}

		pthread_mutex_unlock(&task->mtx);

		// Sleep until next activation
		wait_and_set_nextActivation(task);
	}
	printf("Shutting down thread with id #%d\n", tid);
	return NULL;
}



/* Starts a new real-time thread. 
*  Returns a unique index identifying the thread, or -1 in case of error. */
int start_thread(void *(*func)(void *), void *args, int policy, int prd, int dl, int prio)
{
	pthread_attr_t attr;
	taskType *task;
	unsigned int id;
	int ret;

	// Allocate an id
	id = allocate_task_id();
	if (id == MAX_THREADS)
		return -1;

	task = &rt_threads[id];
	pthread_mutex_lock(&task->mtx);

	// Setup the thread parameters
	task->behaviour = func;
	task->data = args;
	task->period = prd;
	task->deadline = dl;
	task->priority = prio;
	task->stopped = false;
	task->dl_missed = 0;
	task->exe_phase = 0;
	task->min_exe_time= -1ULL;
	task->max_exe_time= 0;
	task->avg_exe_time= 0;

	// Initialize scheduling attributes
	ret = init_sched_attr(&attr, policy, prio);
	if (ret) {
		printf("Init of sched attributes failed (error: %d)\n", ret);
		goto error;
	}

	// Start the actual thread
	ret = pthread_create(&task->tid, &attr, rt_thr_body, (void*)task);
	if (ret) {
		printf("Thread creation failed (error: %s)\n", strerror(ret));
		goto error;
	}

	// Cleanup
	ret = pthread_attr_destroy(&attr);
	if (ret)
		printf("Destruction of sched attributes failed (error: %d)\n", ret);

	pthread_mutex_unlock(&task->mtx);
	return id;

error:
	pthread_mutex_unlock(&task->mtx);
	deallocate_task_id(id);
	return -1;
}


/* Gracefully stop a running thread */
int stop_thread(unsigned int id) {

	if (id >= MAX_THREADS)
		return -1;

	if (!rt_threads[id].in_use)
		return -1;

	pthread_mutex_lock(&rt_threads[id].mtx);
	rt_threads[id].stopped = true;
	pthread_mutex_unlock(&rt_threads[id].mtx);
	pthread_join(rt_threads[id].tid, NULL);

	deallocate_task_id(id);
	return 0;
}
