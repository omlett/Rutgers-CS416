/****************************************************************/
// Created by Pranav Katkamwar, Ben De Brasi, Swapneel Chalageri
// Spring 2017 - CS416
/****************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <ucontext.h>
#include "my_pthread_t.h"
//#include "myallocate.h"
#include <time.h>
/****************************************************************/
/*
// Global Variables
*/
/****************************************************************/
static scheduler *calendar;
static int initialized = 0;
static int initialized_c = 0;
static int tid_counter = 1;
static int lock_counter = 0;
static int scheduler_call_count = 0;
struct itimerval alarm_clock;
struct itimerval stop_clock;
/****************************************************************/
/*
// Running Queue Library
*/
/****************************************************************/
void enqueue(queue *qq, my_pthread_node *thread){
	if (qq->num_threads == 0){
		qq->head = thread;
		qq->tail = thread;
		qq->num_threads = qq->num_threads + 1;
	}
	else if (qq->num_threads == 1){
		qq->tail = thread;
		qq->head->next = thread;
		qq->num_threads = qq->num_threads + 1;
	}
	else{
		qq->tail->next = thread;
		qq->tail = thread;
		qq->num_threads = qq->num_threads + 1;
	}
}

my_pthread_node *dequeue(queue *qq){

	my_pthread_node *tmp ;

	if(qq->num_threads == 0){
		return NULL;
	}
	else if (qq->num_threads == 1){
		tmp = qq->head;
		tmp->next = NULL;
		qq->head = NULL;
		qq->tail = NULL;
		qq->num_threads = 0;
		return tmp;
	}
	else{
		tmp = qq->head;
		qq->head = tmp->next;
		qq->num_threads = qq->num_threads - 1;
		tmp->next = NULL;
		return tmp;
	}
	
}
/****************************************************************/
/*
// Wait Linked List Library
*/
/****************************************************************/
void waitLL_add(wait_node *hi){
	if (calendar->lock_list->num_locks == 0){
		calendar->lock_list->head = hi;
		calendar->lock_list->tail = hi;
		calendar->lock_list->num_locks = calendar->lock_list->num_locks + 1;
	}
	else if (calendar->lock_list->num_locks == 1){
		calendar->lock_list->tail = hi;
		calendar->lock_list->head->next = hi;
		calendar->lock_list->num_locks = calendar->lock_list->num_locks + 1;
	}
	else{
		calendar->lock_list->tail->next = hi;
		calendar->lock_list->tail = hi;
		calendar->lock_list->num_locks = calendar->lock_list->num_locks + 1;
	}

}
void * waitLL_delete(my_pthread_mutex_t *mutex){
	int id = mutex->lock_id;
	wait_node *tmp, *tmp2;

	if(calendar->lock_list->num_locks == 0){
		return NULL;
	}
	else if (calendar->lock_list->num_locks == 1){
		if (calendar->lock_list->head->lock_id == id){
			calendar->lock_list->head = NULL;
			calendar->lock_list->tail = NULL;
			calendar->lock_list->num_locks = 0;
		}
		else {
			printf("ERROR: Lock not found.\n");
		}
	}
	else{
		tmp2 = calendar->lock_list->head;
		while ((tmp2->lock_id != id) && (tmp2->next != NULL)){
			tmp = tmp2;
			tmp2 = tmp2->next;
		}
		if (tmp2->lock_id == id){
			if (!tmp2->next){
				mydeallocate(tmp2, LIBRARYREQ);
				calendar->lock_list->tail = tmp;
			}
			else{
				tmp->next = tmp2->next;
				mydeallocate(tmp2, LIBRARYREQ);
			}
		}
		else{
			printf("ERROR: Lock not found.\n");
		}
	}
}
/****************************************************************/
/*
// Scheduler Library
*/
/****************************************************************/
void init_scheduler(){
	calendar = (scheduler *) myallocate(sizeof(scheduler), LIBRARYREQ);
	calendar->run_queue = (mlpq *) myallocate(sizeof(mlpq), LIBRARYREQ);
	calendar->lock_list = (wait_linked_list *) myallocate(sizeof(wait_linked_list), LIBRARYREQ);
	calendar->lock_list->num_locks = 0;
	calendar->run_queue->low = (queue *) myallocate(sizeof(queue), LIBRARYREQ);
	calendar->run_queue->low->num_threads = 0;
	calendar->run_queue->normal = (queue *) myallocate(sizeof(queue), LIBRARYREQ);
	calendar->run_queue->normal->num_threads = 0;
	calendar->run_queue->high = (queue *) myallocate(sizeof(queue), LIBRARYREQ);
	calendar->run_queue->high->num_threads = 0;
	calendar->dead_threads = (queue *) myallocate(sizeof(queue), LIBRARYREQ);
	calendar->main_context = (my_pthread_node *) myallocate(sizeof(my_pthread_node), LIBRARYREQ);
	calendar->main_context->id = 0;
	calendar->main_context->next = calendar->main_context;
	calendar->running_context = (my_pthread_node *) myallocate(sizeof(my_pthread_node), LIBRARYREQ);

	signal(SIGVTALRM, signal_handler);

	initialized = 1;
}

void maintain(){
	my_pthread_node *tmp;
	
	tmp = dequeue(calendar->run_queue->low);
	while (tmp != NULL){
		enqueue(calendar->run_queue->high, tmp);
		tmp = dequeue(calendar->run_queue->low);
	}
}

my_pthread_node *getThread(){
	my_pthread_node *nextThread;
		nextThread = dequeue(calendar->run_queue->high);
		if(nextThread == NULL){
			nextThread = dequeue(calendar->run_queue->normal);
		}
		if(nextThread == NULL){
			nextThread = dequeue(calendar->run_queue->low);
		}
		if (nextThread == NULL){
			return NULL;
		}
			return nextThread;
}

void reschedule(my_pthread_node *thread){
	
	if (thread != NULL){
		if ((thread->status == YIELD)){
			if (thread->level == HIGH){
				enqueue(calendar->run_queue->high, thread);
			}
			if (thread->level == NORMAL){
				enqueue(calendar->run_queue->normal, thread);
			}
			if (thread->level == LOW){
				enqueue(calendar->run_queue->low, thread);
			}
		}
		else if (thread->status == INVERTED){
			enqueue(calendar->run_queue->high, thread);
		}
		else{
			if (thread->level == HIGH){
				if ((thread->running_total) >= HIGHRANGE){
					thread->level = NORMAL;
					thread->running_total = 0;
					enqueue(calendar->run_queue->normal, thread);
				}
				else{
					enqueue(calendar->run_queue->high, thread);
				}
			}
			else{
				if (thread->level == NORMAL){
					if ((thread->running_total) >= NORMALRANGE){
						thread->level = LOW;
						thread->running_total = 0;
						enqueue(calendar->run_queue->low, thread);
					}
					else {
						thread->level = NORMAL;
						enqueue(calendar->run_queue->normal, thread);
					}
				}
				else if (thread->level == LOW){
					enqueue(calendar->run_queue->low, thread);
				}
			}	
		}
	}
}
/****************************************************************/
void signal_handler(){

	// Reset itimerval
	alarm_clock.it_value.tv_sec = 0;
	alarm_clock.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);

	struct timeval thetime;
	gettimeofday(&thetime, NULL);
	
	my_pthread_node *tmp = calendar->running_context;
	
	// Check Maintenance Counter
	scheduler_call_count++;
	if(scheduler_call_count >= call_count_limit){
		maintain();
		scheduler_call_count = 0;
	}

	// Handle The Running_Context
	if (tmp->id != NULL){
		if(tmp->status == YIELD){
			tmp->stop_time =  (1000000*thetime.tv_sec) + thetime.tv_usec;
			tmp->running_total += (tmp->stop_time - tmp->start_time);
			my_pthread_yield();	
		}
		else if (tmp->status == RUNNING){
			tmp->status = READY;
			tmp->stop_time =  (1000000*thetime.tv_sec) + thetime.tv_usec;
			tmp->running_total += (tmp->stop_time - tmp->start_time);
			my_pthread_yield();
		}
		else if ((tmp->status == ENDED) || (tmp->status == WAITING)){	
			calendar->running_context->status = HELP;
			my_pthread_yield();
		}
	}
	else{
		my_pthread_yield();
	}


	return;
}
/****************************************************************/
void run_user_thread(my_pthread_node * thread, void *(*function)(void*), void * arg){
	// Get the current time
	struct timeval thetime;
	gettimeofday(&thetime, NULL);

	// Adjust flags / assignments
	thread->status = RUNNING;
	calendar->running_context = thread;
	current_tid = calendar->running_context->id;

	// Timestamp the thread
	thread->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
	thread->running_total = 0;

	// Run the thread's function
	thread->retval = function(arg);

	// pthread_exit was not called if it gets here.
	if (thread->status != ENDED){
		thread->status = ENDED;
		calendar->total_threads--;

		calendar->running_context = NULL;
		my_pthread_yield();
	}
}
/****************************************************************/
/*
// my_pthread_t Library
*/
/****************************************************************/
int my_pthread_create(my_pthread_t * thread, my_pthread_attr_t * attr, void *(*function)(void*), void * arg){

	if (initialized != 1){
		init_scheduler();
	}
	
	// myallocate a new node and assign it to the thread
	my_pthread_node *new_thread_node = (my_pthread_node *) myallocate(sizeof(my_pthread_node), LIBRARYREQ);
	thread->node = new_thread_node;

	int errc;
	errc = getcontext(&(new_thread_node->uctx));

	if (errc == -1){
		printf("ERROR: unable to getcontext\n");
	}
	else{
		tid_counter++;

		// Set up the thread's stack, ucontext_t
		thread->node->uctx.uc_stack.ss_sp = myallocate(TSTACK, LIBRARYREQ);
		thread->node->uctx.uc_stack.ss_size = TSTACK;
		thread->node->uctx.uc_link = &(calendar->main_context->uctx);

		// Initialize thread's flags/data
		thread->node->level = HIGH;
		thread->node->status = NEW;
		thread->node->id = tid_counter;

		// Make the context and enqueue the thread
		makecontext(&(thread->node->uctx), run_user_thread, 3, thread->node, function, arg);
		enqueue(calendar->run_queue->high, thread->node);
		calendar->total_threads++;
		
		// Set up the main context with a node and initialize it
		if (initialized_c != 1){
			my_pthread_node *main_thread_node = (my_pthread_node *) myallocate(sizeof(my_pthread_node), LIBRARYREQ);
			calendar->main_context = main_thread_node;
			calendar->main_context->level = HIGH;
			calendar->main_context->status = RUNNING;
			calendar->main_context->id = 1;

			calendar->running_context = calendar->main_context;
			current_tid = calendar->running_context->id;

			// Get Time
			struct timeval thetime;
			gettimeofday(&thetime, NULL);
			calendar->main_context->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;

			initialized_c = 1;
			getcontext(&(calendar->main_context->uctx));
		}

		return 0;
	}
}

void my_pthread_yield(){

	if(calendar == NULL){
	printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
	return NULL;

	}
	// Reset itimerval
	alarm_clock.it_value.tv_sec = 0;
	alarm_clock.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);

	// Get Time
	struct timeval thetime;
	gettimeofday(&thetime, NULL);

	// Variable Declarations
	my_pthread_node *old_thread, *new_thread;
	old_thread = calendar->running_context;
	
	if (old_thread != NULL){
		// Old thread exists and is running
		if (old_thread->status != ENDED){
			old_thread->status = YIELD;
			old_thread->stop_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
			old_thread->running_total += (old_thread->stop_time - old_thread->start_time);		
			new_thread = getThread();
			reschedule(old_thread);

			if (new_thread != NULL){
				alarm_clock.it_value.tv_sec = 0;
				if (new_thread->level == HIGH){	
					alarm_clock.it_value.tv_usec = 50000;
				}
				else if (new_thread->level == NORMAL){
					alarm_clock.it_value.tv_usec = 100000;
				}
				else if (new_thread->level == LOW){
					alarm_clock.it_value.tv_usec = 1500000;
				}
				new_thread->status = RUNNING;
				calendar->running_context = new_thread;
				current_tid = calendar->running_context->id;
				prev_tid = old_thread->id;
				mprotect_setter(current_tid, prev_tid);
				
				gettimeofday(&thetime, NULL);
				new_thread->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
				setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
				swapcontext(&(old_thread->uctx), &(new_thread->uctx));
			}
			// Just return to main if no other thread exists
		}
		// Old thread is marked as ended:
		else{
			new_thread = getThread();
			if (new_thread != NULL){
				alarm_clock.it_value.tv_sec = 0;
				if (new_thread->level == HIGH){	
					alarm_clock.it_value.tv_usec = 50000;
				}
				else if(new_thread->level == NORMAL){
					alarm_clock.it_value.tv_usec = 100000;
				}
				else if(new_thread->level == LOW){
					alarm_clock.it_value.tv_usec = 1500000;
				}
				new_thread->status = RUNNING;
				calendar->running_context = new_thread;
				current_tid = calendar->running_context->id;
				mprotect_setter(current_tid, 0);
				gettimeofday(&thetime, NULL);
				new_thread->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
				setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
				setcontext(&(new_thread->uctx));
			}
		}
	}
	// Old thread is marked as NULL
	else{
		new_thread = getThread();
		if (new_thread != NULL){
			alarm_clock.it_value.tv_sec = 0;
			if (new_thread->level == HIGH){	
				alarm_clock.it_value.tv_usec = 50000;
			}
			else if(new_thread->level == NORMAL){
				alarm_clock.it_value.tv_usec = 100000;
			}
			else if(new_thread->level == LOW){
				alarm_clock.it_value.tv_usec = 1500000;
			}
			new_thread->status = RUNNING;
			calendar->running_context = new_thread;
			current_tid = calendar->running_context->id;
			gettimeofday(&thetime, NULL);
			new_thread->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
			
			prev_tid = old_thread->id;
			mprotect_setter(current_tid, prev_tid);

			setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
			swapcontext(&(old_thread->uctx), &(new_thread->uctx));
		}
	}
}

void my_pthread_exit(void *value_ptr){


	if(calendar == NULL){
		printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
		return NULL;

}	
	// Reset itimerval
	alarm_clock.it_value.tv_sec = 0;
	alarm_clock.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);

	// Get Time
	struct timeval thetime;
	gettimeofday(&thetime, NULL);

	// Variable Declarations
	my_pthread_node *dead_thread, *new_thread;

	dead_thread = calendar->running_context;
	if (dead_thread != NULL){
		calendar->total_threads--;
		dead_thread->status = ENDED;
		dead_thread->retval = value_ptr;
		dead_thread->stop_time =  (1000000*thetime.tv_sec) + thetime.tv_usec;
		dead_thread->running_total += (dead_thread->stop_time - dead_thread->start_time);
		enqueue(calendar->dead_threads, dead_thread);
	
		
		new_thread = getThread();
		// Swap contexts
		if (new_thread != NULL){
			alarm_clock.it_value.tv_sec = 0;
			if (new_thread->level == HIGH){
				alarm_clock.it_value.tv_usec = 50000;
			}
			else if (new_thread->level == NORMAL){
				alarm_clock.it_value.tv_usec = 100000;
			}
			else if (new_thread->level == LOW){
				alarm_clock.it_value.tv_usec = 1500000;
			}
			new_thread->status = RUNNING;
			calendar->running_context = new_thread;
			current_tid = calendar->running_context->id;
			gettimeofday(&thetime, NULL);
			new_thread->start_time = (1000000*thetime.tv_sec) + thetime.tv_usec;
			prev_tid = dead_thread->id;
			mprotect_setter(current_tid, prev_tid);

			
			setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
			swapcontext(&(dead_thread->uctx), &(new_thread->uctx));
		}
		else{
			calendar->running_context = NULL;
			my_pthread_yield();
		}

	}

	return;
}

int my_pthread_join(my_pthread_t thread, void **value_ptr){


	if(calendar == NULL){
		printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
		return NULL;

	}
	while (thread.node->status != ENDED){
		my_pthread_yield();		
	}

	if (value_ptr == NULL){
		return;
	}
	else{
		thread.retval = thread.node->retval;
		value_ptr = thread.retval;
		//free
		return;
	}

	printf("ERROR: Reached end of Join");
	
	return;
}
/****************************************************************/
/*
// my_pthread_mutex Library
*/
/****************************************************************/
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, my_pthread_mutexattr_t *mutexattr){
/*
	if (mutex == NULL){
		return 1;				// Return 1 if unsuccessful?
	}
*/	
	if (initialized != 1){
		init_scheduler();
	}
	
	lock_counter++;
	mutex->lock_status = NOT_LOCKED;
	mutex->lock_id = lock_counter; 

	// Insert New Node into Wait List
	wait_node *new_wait_node = (wait_node*) myallocate(sizeof(wait_node), LIBRARYREQ);
	new_wait_node->lock_queue = (queue *) myallocate(sizeof(queue), LIBRARYREQ);
	new_wait_node->lock_id = mutex->lock_id;
	mutex->lock_node = new_wait_node;
	waitLL_add(new_wait_node);

	return 0;					// Return 0 if successful
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){



	if(calendar == NULL){
		printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
		return NULL;

	}

	if (mutex->lock_status == NOT_LOCKED){
		mutex->lock_status = LOCKED;
		mutex->lock_holder = calendar->running_context;
		//setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
		return 0;				// Return 0 if successful
	}
	else{


		if((mutex->lock_holder->level) < (calendar->running_context->level)){
			printf("Priority inversion!\n");
			mutex->lock_holder->status = INVERTED;
			mutex->lock_holder->running_total = 0;

			my_pthread_node *tmp;
			if (mutex->lock_holder->level == LOW){
				tmp = dequeue(calendar->run_queue->low);
				while (tmp->id != mutex->lock_holder->id){
					enqueue(calendar->run_queue->low, tmp);
					tmp = dequeue(calendar->run_queue->low);
				}
			}
			else if (mutex->lock_holder->level = NORMAL){
				tmp = dequeue(calendar->run_queue->normal);
				while (tmp->id != mutex->lock_holder->id){
					enqueue(calendar->run_queue->normal, tmp);
					tmp = dequeue(calendar->run_queue->normal);
				}
			}
			else if (mutex->lock_holder->level == HIGH){
				tmp = dequeue(calendar->run_queue->high);
				while (tmp->id != mutex->lock_holder->id){
					enqueue(calendar->run_queue->high, tmp);
					tmp = dequeue(calendar->run_queue->high);
				}
			}
			
			if ((tmp != NULL) && (tmp->id == mutex->lock_holder->id)){
				reschedule(tmp);
			}
			else {
				printf("ERROR: Inversion Acquisition Failed.\n");
			}
		}	


		// Add thread to wait queue and yield
		enqueue(mutex->lock_node->lock_queue, calendar->running_context);
		calendar->running_context->status = WAITING;
		calendar->running_context = NULL;
		my_pthread_yield();

		return 0;
	}
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){
	
	if(calendar == NULL){
		printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
		return NULL;

	}
	
	my_pthread_node * tmp;

	if (mutex->lock_status == LOCKED){
		mutex->lock_status = NOT_LOCKED;
		tmp = dequeue(mutex->lock_node->lock_queue);
		if (tmp != NULL){
			tmp->status = READY;
			reschedule(tmp);
		}
		setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
		return 0;				// Return 0 if successful
	}
	else{
		setitimer(ITIMER_VIRTUAL, &alarm_clock, NULL);
		return 0;
	}
}

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){

	if(calendar == NULL){
		printf("Error! Invalid function call unless pthread_create or mutex_init was called before\n");	
		return NULL;
	
	}
	
	if (mutex == NULL){
		return 1;				// Return 1 if unsuccessful?
	}

	if(mutex->lock_status == LOCKED	){
		return 1;
	}
	else{
		waitLL_delete(mutex);
	}
	return 0;					// Return 0 if successful
}
/****************************************************************/
/*
// Testing Purposes
*/
/****************************************************************/


/****************************************************************/
/*
// Pseudocode & Brainstorming
*/
/****************************************************************/
/*

*/
