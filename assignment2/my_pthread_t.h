#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H


#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "myallocate.h"

//number of priority queue levels
#define RUN_QUEUES 5
//ucontext stack size
#define STACK_SIZE 4096
//number of scheduler cycles before maintenance
#define MAINTENANCE_THRESHOLD 20
//thread lifetime threshold (usec) for increasing priority
#define LIFETIME_THRESHOLD 1000000

//thread states
//QUEUED: thread is in running queue
//YIELD: thread called yield
//EXITED: thread has exited
//JOINING: thread called join
//BLOCKED: thread is in waiting queue (blocking for mutex)
//RUNNING: thread is running
#define QUEUED 1
#define YIELD 2
#define EXITED 3
#define JOINING 4
#define BLOCKED 5
#define RUNNING 6

//structs

union Data{

	int arr[2];
	void* arg;
};

typedef struct my_pthread_t{

	ucontext_t context;

	int id;
	struct my_pthread_t* next;
	int priority;	
	int state;
	struct my_pthread_t* address;
	void* retval;

	//timeEnd - timeStart (usec)
	unsigned long int timeRun;
	//start of thread's scheduled time slice (usec)
	unsigned long int timeStart;
	//end of thread's scheduled time slice (usec)
	unsigned long int timeEnd;
	//time stamp when thread was created (usec)
	unsigned long int timeLife;

} my_pthread_t;

typedef struct queue{

	my_pthread_t* front;
	my_pthread_t* rear;

} queue;

typedef struct scheduler{

	queue* runQueues;
	my_pthread_t* currentThread;
	int inScheduler;
	int swap;
	int maintenanceCycle;

} scheduler;

typedef struct my_pthread_mutex_t{

	volatile int flag;
	queue waitQueue;

} my_pthread_mutex_t;

//to integrate with pranav memory

int current_tid;
int prev_tid;

//global variables

scheduler* myScheduler;
//int threadID = 1;
//int firstCall = 1;

//functions

int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void* arg);
void my_pthread_yield();
void my_pthread_exit(void* value_ptr);
int my_pthread_join(my_pthread_t thread, void** value_ptr);

int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t* mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex);

void timer_handler(int signum);

void enqueue(my_pthread_t* thread, queue* q);
my_pthread_t* dequeue(queue* q);
my_pthread_t* getNextThread();
int checkNextThread();

void scheduler_init();
void maintenance();
void schedule();


#endif







