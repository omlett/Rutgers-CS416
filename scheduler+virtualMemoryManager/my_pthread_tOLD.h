/****************************************************************/
// Created by Pranav Katkamwar, Ben De Brasi, Swapneel Chalageri
// Spring 2017 - CS416
/****************************************************************/
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H


#define pthread my_pthread
#define TSTACK 2048
#define LEVELS 3
#define call_count_limit 1
#define HIGHRANGE 500000
#define NORMALRANGE  1000000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <ucontext.h>
#include <time.h>
#include "myallocate.h"
/****************************************************************/
// Imported Variables
/****************************************************************/

int current_tid;
int prev_tid;

/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef enum status_flags{
	HELP,	
	ENDED,
	RUNNING,
	WAITING,
	YIELD,
	NEW,
	READY
} thread_status;

typedef enum queue_level{
	NONE,
	LOW,
	NORMAL,
	HIGH
} queue_level;

typedef enum mutex_lock_status{
	NOT_LOCKED,
	LOCKED
} mutex_lock_status;

typedef enum priority_flag{
	NOT_INVERTED,
	INVERTED
} priority_flag;

typedef struct my_pthread_node{
	long int running_total;
	long int start_time;
	long int stop_time;
	int runs;
	int id;
	ucontext_t uctx;
	thread_status status;
	queue_level level;
	priority_flag p_flag;
	//char thread_page_table[number_of_pages];
	void *retval;
	struct my_pthread_node *next;
} my_pthread_node;

typedef struct my_pthread_t{
	my_pthread_node *node;
	int id;
	void *retval;
} my_pthread_t;					// my_pthread_t

typedef struct my_pthread_attr_t{
	
} my_pthread_attr_t;				// my_pthread_attr_t

typedef struct the_queue{
	my_pthread_node *head;
	my_pthread_node *tail;
	int num_threads;
} queue;					// Base Queue for Running Queue

typedef struct wait_node{
	int lock_id;
	queue *lock_queue;
	struct wait_node *next;
} wait_node;					// Nodes for waiting queue

typedef struct wait_linked_list{
	wait_node *head;
	wait_node *tail;
	int num_locks;
} wait_linked_list;					// Waiting Queue

typedef struct multi_level_priority_queue{
	queue *low;
	queue *normal;
	queue *high;
} mlpq;						// Multi Level Priority Queue

typedef struct the_scheduler{
	mlpq *run_queue;
	queue *dead_threads;
	wait_linked_list *lock_list;
	my_pthread_node *main_context;
	my_pthread_node *running_context;
	int total_threads;
} scheduler;					// Scheduler

typedef struct my_pthread_mutexattr_t{

} my_pthread_mutexattr_t;				// my_pthread_mutexattr_t

typedef struct my_pthread_mutex_t{
	mutex_lock_status lock_status;
	int lock_id;
	my_pthread_node *lock_holder;		// When ever something successfully locks you point to this
	wait_node *lock_node;
} my_pthread_mutex_t;				// Mute Lock
/****************************************************************/
// Function Headers
/****************************************************************/
// Queue Library
/****************************************************************/
void enqueue(queue *qq, my_pthread_node *thread);
my_pthread_node *dequeue(queue *qq);
/****************************************************************/
// Wait Queue Library
/****************************************************************/
void wait_enqueue(wait_node *hi);
/****************************************************************/
// Scheduler Library
/****************************************************************/
void init_scheduler();
void run_user_thread(my_pthread_node * thread, void *(*function)(void*), void * arg);
void signal_handler();
my_pthread_node *getThread();
void reschedule(my_pthread_node *thread);
/****************************************************************/
// my_pthread_t Library
/****************************************************************/
int my_pthread_create(my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg);
void my_pthread_yield();
void my_pthread_exit(void *value_ptr);
int my_pthread_join(my_pthread_t thread, void **value_ptr);
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, my_pthread_mutexattr_t *mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#endif
