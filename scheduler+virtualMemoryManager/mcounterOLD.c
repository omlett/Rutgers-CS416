//#include <pthread.h>
#include "my_pthread_t.h"
#include <stdio.h>
#include <stdlib.h>
#include "myallocate.h"

#define ITER 1000000

my_pthread_mutex_t lock;
int c=0;

void* counter(void* a){
	int i,temp;
	for(i=0; i<ITER; i++){
		my_pthread_mutex_lock(&lock);		
		temp = c;
		temp++;
		c = temp;
		my_pthread_mutex_unlock(&lock);
	}
	my_pthread_exit( 0 );
	//never happens
	return 0;
}

int main(int argc, char* argv[]){
    int numThreads=2;
    int tmp,i;
    my_pthread_t* threads;
    

    if (argc==2){
        tmp = atoi(argv[1]);
        if (tmp>0) numThreads = tmp;
    }
    threads = (my_pthread_t*) malloc(numThreads * sizeof(my_pthread_t));
    //initialize lock
    if (my_pthread_mutex_init(&lock, NULL) !=0)
    {
        printf("mutex init failed\n");
        exit(1);
    }
    //create threads
    for(i=0; i<numThreads; i++){
        my_pthread_create(&threads[i], NULL, counter, NULL);
    }
	//my_pthread_create(&t1, NULL, counter, NULL);
	//my_pthread_create(&t2, NULL, counter, NULL);
    
    for(i=0; i<numThreads; i++){
	    my_pthread_join(threads[i], NULL);
    }
	//my_pthread_join(t2, NULL);
	printf("counter final val: %d, expecting %d\n", c, ITER*numThreads);
	return 0;
}
