#include "my_pthread_t.h"

int threadID = 1;
int firstCall = 1;


int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void*(*function)(void*), void* arg){

	//1st call to my_pthread_create
	if(firstCall == 1){
	
		//initialize scheduler
		scheduler_init();

		my_pthread_t* mainThread = (my_pthread_t*)myallocate(sizeof(my_pthread_t), LIBRARYREQ);

		//set current running thread to main thread
		myScheduler->currentThread = mainThread;

		//initialize main thread variables
		mainThread->id = threadID++;
		mainThread->next = NULL;
		mainThread->priority = 0;
		mainThread->state = RUNNING;
		mainThread->address = mainThread;
		mainThread->retval = NULL;

		//to integrate with pranav memory
		current_tid = mainThread->id;

		//set main thread time slice start time and creation start time
		struct timeval time;
		gettimeofday(&time, NULL);
		mainThread->timeStart = (time.tv_sec * 1000000) + time.tv_usec;
		mainThread->timeLife = (time.tv_sec * 1000000) + time.tv_usec;
		mainThread->timeRun = 0;

		//initialize ucontext for main thread
		int check = getcontext(&(mainThread->context));
		if(check == -1){
			printf("getcontext error\n");
		}
		mainThread->context.uc_link = NULL;
		mainThread->context.uc_stack.ss_sp = myallocate(STACK_SIZE, LIBRARYREQ);
		mainThread->context.uc_stack.ss_size = STACK_SIZE;
		mainThread->context.uc_stack.ss_flags = 0;
		
		//enqueue(mainThread, &(myScheduler->runQueues[mainThread->priority]));
	}

	//pass void* arg from my_pthread_create to makecontext as int array
	union Data data;
	data.arg = arg;

	//initialize created thread variables
	thread->id = threadID++;
	thread->next = NULL;
	thread->priority = 0;
	thread->state = QUEUED;
	thread->address = thread;
	thread->retval = NULL;

	//set created thread creation start time
	struct timeval time;
	gettimeofday(&time, NULL);
	thread->timeLife = (time.tv_sec * 1000000) + time.tv_usec;
	thread->timeRun = 0;

	//initialize ucontext for created thread
	int check = getcontext(&(thread->context));
	if(check == -1){
		printf("getcontext error\n");
	}
	thread->context.uc_link = NULL;
	thread->context.uc_stack.ss_sp = myallocate(STACK_SIZE, LIBRARYREQ);
	thread->context.uc_stack.ss_size = STACK_SIZE;
	thread->context.uc_stack.ss_flags = 0;
	makecontext(&(thread->context), (void*)function, 1, data.arg);

	//add created thread to run queue
	enqueue(thread, &(myScheduler->runQueues[thread->priority]));

	//1st call to my_pthread_create
	if(firstCall == 1){

		firstCall = 0;
		//start 50ms timer
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 50000;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
}

void my_pthread_yield(){

	myScheduler->currentThread->state = YIELD;

	schedule();
}

void my_pthread_exit(void* value_ptr){

	myScheduler->currentThread->state = EXITED;

	if(value_ptr != NULL){
		myScheduler->currentThread->retval = value_ptr;
	}

	schedule();
}

int my_pthread_join(my_pthread_t thread, void** value_ptr){
	
	myScheduler->currentThread->state = JOINING;

	//schedule another thread while the thread being joined has not exited yet
	while(thread.address->state != EXITED){
		schedule();
	}
	
	if(value_ptr != NULL){
		*value_ptr = thread.address->retval;
	}

	myScheduler->currentThread->state = RUNNING;
}

//signal handler for timer
void timer_handler(int signum){

	if(myScheduler->inScheduler == 1){
		return;
	}

	schedule();
}

//inserts thread into queue
void enqueue(my_pthread_t* thread, queue* q){
	//queue is empty
	if(q->rear == NULL){
		q->front = thread;
		q->rear = thread;
	}
	//queue is not empty
	else{
		q->rear->next = thread;
		q->rear = thread;
	}
}

//returns NULL if queue is empty
my_pthread_t* dequeue(queue* q){
	//queue is empty
	if(q->front == NULL){
		return NULL;
	}
	//queue is not empty
	my_pthread_t* temp = q->front;
	q->front = q->front->next;
	temp->next = NULL;

	if(q->front == NULL){
		q->rear = NULL;
	}

	return temp;
}

//gets next thread to be scheduled
//returns NULL if no threads in run queues
my_pthread_t* getNextThread(){

	my_pthread_t* ptr;

	int i;
	for(i=0; i<RUN_QUEUES; i++){

		ptr = dequeue(&(myScheduler->runQueues[i]));
		if(ptr != NULL){
			return ptr;
		}
	}

	printf("From getNextThread: no more threads in run queues.\n");

	return NULL;
}

//returns 1 if there is at least 1 thread in run queues
//returns 0 otherwise
int checkNextThread(){

	my_pthread_t* ptr;

	int i;
	for(i=0; i<RUN_QUEUES; i++){

		ptr = (myScheduler->runQueues[i]).front;
		if(ptr != NULL){
			return 1;
		}
	}

	printf("From checkNextThread: no more threads in run queues.\n");

	return 0;
}

//initializes scheduler
void scheduler_init(){

	//allocate memory for scheduler and its queues
	myScheduler = (scheduler*)myallocate(sizeof(scheduler), LIBRARYREQ);
	myScheduler->runQueues = (queue*)myallocate(RUN_QUEUES * sizeof(queue), LIBRARYREQ);

	//initialize queues
	int i;
	for(i=0; i<RUN_QUEUES; i++){
		myScheduler->runQueues[i].front = NULL;
		myScheduler->runQueues[i].rear = NULL;
	}

	//intialize scheduler variables
	myScheduler->currentThread = NULL;
	myScheduler->inScheduler = 0;
	myScheduler->maintenanceCycle = 0;

	//assign signal handler to catch 50 ms timer signal
	signal(SIGALRM, timer_handler);
}

//performs maintenance
void maintenance(){

	my_pthread_t* ptr;
	my_pthread_t* prev;

	struct timeval time;
	unsigned long int timeNow;

	int i;
	//iterate through all lower priority queues
	for(i=1; i<RUN_QUEUES; i++){
		
		ptr = myScheduler->runQueues[i].front;
		prev = NULL;

		//while ptr points to a thread
		while(ptr != NULL){

			gettimeofday(&time, NULL);
			timeNow = (time.tv_sec * 1000000) + time.tv_usec;

			//thread priority should be increased
			if(timeNow - ptr->timeLife >= LIFETIME_THRESHOLD){

				//remove thread from queue
				//thread is front
				if(ptr == myScheduler->runQueues[i].front){

					myScheduler->runQueues[i].front = myScheduler->runQueues[i].front->next;

					if(myScheduler->runQueues[i].front == NULL){
						myScheduler->runQueues[i].rear = NULL;
					}
				}
				//thread is not front
				else{

					prev->next = ptr->next;

					if(prev->next == NULL){
						myScheduler->runQueues[i].rear = prev;
					}
				}

				my_pthread_t* temp = ptr;

				//advance ptr before ptr->next is set to NULL
				ptr=ptr->next;

				//add thread to highest priority queue
				temp->priority = 0;
				temp->next = NULL;
				enqueue(temp, &(myScheduler->runQueues[temp->priority]));
			}

			if(ptr != NULL){
				prev = ptr;
				ptr = ptr->next;
			}
		}
	}
}

//schedule function
void schedule(){

	//set signal handler flag
	myScheduler->inScheduler = 1;

	//maintenance cycle
	myScheduler->maintenanceCycle++;
	if(myScheduler->maintenanceCycle >= MAINTENANCE_THRESHOLD){
		
		myScheduler->maintenanceCycle = 0;
		maintenance();
	}

	//set to no swap
	myScheduler->swap = 0;

	//get current thread and check if there are any more threads in run queues
	my_pthread_t* ptr = myScheduler->currentThread;
	int checkNext = checkNextThread();

	//increment current thread's time run
	struct timeval time;
	gettimeofday(&time, NULL);
	ptr->timeEnd = (time.tv_sec * 1000000) + time.tv_usec;
	ptr->timeRun += ptr->timeEnd - ptr->timeStart;

	//no other threads in run queues
	if(checkNext == 0){
		
		//if current thread has exited
		if(ptr->state == EXITED){
			printf("From scheduler: all threads have finished running.\n");
			myScheduler->inScheduler = 0;
			return;
		}

		//set current thread time slice start time
		gettimeofday(&time, NULL);
		ptr->timeStart = (time.tv_sec * 1000000) + time.tv_usec;

		//start 50ms timer
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 50000;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);

		//set signal handler flag
		myScheduler->inScheduler = 0;

		return;
	}

	switch(ptr->state){
		//should not happen
		case QUEUED:
			//printf("Case: QUEUED %d\n", myScheduler->currentThread->id);
			break;
		//current thread called yield, requeued at same priority
		case YIELD:
			//printf("Case: YIELD %d\n", myScheduler->currentThread->id);
			ptr->state = QUEUED;
			ptr->timeRun = 0;
			enqueue(ptr, &(myScheduler->runQueues[ptr->priority]));
			myScheduler->swap = 1;
			break;
		//current thread called exit
		case EXITED:
			//printf("Case: EXITED %d\n", myScheduler->currentThread->id);
			mydeallocate(ptr->context.uc_stack.ss_sp, LIBRARYREQ);
			myScheduler->swap = 1;
			break;
		//current thread called join
		case JOINING:
			//printf("Case: JOINING %d\n", myScheduler->currentThread->id);
			ptr->state = QUEUED;
			ptr->timeRun = 0;
			enqueue(ptr, &(myScheduler->runQueues[ptr->priority]));
			myScheduler->swap = 1;
			break;
		//current thread is blocking for mutex in waiting queue
		case BLOCKED:
			//printf("Case: BLOCKED %d\n", myScheduler->currentThread->id);
			myScheduler->swap = 1;
			break;			
		//current thread is running
		case RUNNING:
			//printf("Case: RUNNING %d\n", myScheduler->currentThread->id);
			//check if current thread has run for its entire allotted time slice
			if(ptr->timeRun >= (ptr->priority + 1) * 50000){
				ptr->priority++;
				if(ptr->priority >= RUN_QUEUES){
					ptr->priority = RUN_QUEUES - 1;
				}
				ptr->state = QUEUED;
				ptr->timeRun = 0;
				enqueue(ptr, &(myScheduler->runQueues[ptr->priority]));
				myScheduler->swap = 1;
			}
			break;
	}

	
	//current thread should keep running
	if(myScheduler->swap == 0){
		
		//set current thread time slice start time
		gettimeofday(&time, NULL);
		ptr->timeStart = (time.tv_sec * 1000000) + time.tv_usec;

		//start 50 ms timer
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 50000;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);

		//set signal handler flag
		myScheduler->inScheduler = 0;
	}
	//current thread swapped out for next thread
	else{

		//get next thread
		myScheduler->currentThread = getNextThread();

		//set next thread state to RUNNING
		myScheduler->currentThread->state = RUNNING;

		//set next thread time slice start time
		gettimeofday(&time, NULL);
		myScheduler->currentThread->timeStart = (time.tv_sec * 1000000) + time.tv_usec;

		//start 50 ms timer
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 50000;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);

		//set signal handler flag
		myScheduler->inScheduler = 0;
		
		//to integrate with pranav memory
		prev_tid = ptr->id;
		current_tid = myScheduler->currentThread->id;

		//swap threads
		int result = swapcontext(&(ptr->context), &(myScheduler->currentThread->context));
	}

	return;
}

int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){

	if(mutex == NULL){
		return -1;
	}

	mutex->waitQueue.front = NULL;
	mutex->waitQueue.rear = NULL;

	mutex->flag = 0;

	return 0;
}

int my_pthread_mutex_lock(my_pthread_mutex_t* mutex){

	while(__sync_lock_test_and_set(&(mutex->flag), 1) == 1){

		myScheduler->currentThread->state = BLOCKED;
		enqueue(myScheduler->currentThread, &(mutex->waitQueue));
		schedule();
	}

	return 0;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){

	__sync_synchronize();
	my_pthread_t* ptr;

	if(mutex->waitQueue.front != NULL){

		ptr = dequeue(&(mutex->waitQueue));
		ptr->state = QUEUED;
		enqueue(ptr, &(myScheduler->runQueues[ptr->priority]));
	}

	mutex->flag=0;

	return 0;	
}

int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex){

	if(mutex == NULL){
		return -1;
	}

	if(mutex->flag != 0){
		return -1;
	}
	
	return 0;
}

















