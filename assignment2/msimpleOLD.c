#include <stdio.h>
#include "my_pthread_t.h"
#include "myallocate.h"

void* myThread(void* p){
    	 printf("Hello from thread %d\n", *(int*)p);
	char *test = malloc(1000);
	char *test2 = malloc(4000);
	char *test3 = malloc(5000);
	struct timeval thetime;
	gettimeofday(&thetime, NULL);
	long int nowtime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	long int thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	while (thentime - nowtime < 2000000){
		gettimeofday(&thetime, NULL);
		thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	}
    printf("Hello from thread %d I am ended\n", *(int*)p);
	my_pthread_exit(0);
	return 0;
}

int main(){
    my_pthread_t *thread = malloc(sizeof(my_pthread_t));
    int id, arg1, arg2;
    arg1 = 1;
    id = my_pthread_create(thread, NULL, myThread, (void*)&arg1);
    my_pthread_yield();
    arg2 = 2;
    my_pthread_join(*thread, NULL);
    myThread((void*)&arg2);
    
    return 0;
}

//#include<stdio.h>
//#include"my_pthread_t.h"
/*
void* myThread(void* p){
    printf("Hello from thread %d\n", *(int*)p);
	struct timeval thetime;
	gettimeofday(&thetime, NULL);
	long int nowtime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	long int thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	while (thentime - nowtime < 2000000){
		gettimeofday(&thetime, NULL);
		thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
	}
    printf("Hello from thread %d I am ended\n", *(int*)p);
	my_pthread_exit(0);
	return 0;
}

int main(){
    my_pthread_t *thread = malloc(sizeof(my_pthread_t));
    int id, arg1, arg2;
    arg1 = 1;
    id = my_pthread_create(thread, NULL, myThread, (void*)&arg1);
    my_pthread_yield();
    arg2 = 2;
    my_pthread_join(thread, NULL);
    myThread((void*)&arg2);
    
    return 0;
}
*/
