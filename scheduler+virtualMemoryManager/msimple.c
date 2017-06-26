#include<stdio.h>
#include "my_pthread_t.h"
#include "myallocate.h"
#include "paging.h"
#include <string.h>

void* myThread(void* p){
        printf("Hello from thread %d\n", *(int*)p);
        struct timeval thetime;
        gettimeofday(&thetime, NULL);
        long int nowtime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
        long int thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
        char * test;

        //my_pthread_yield();
        if (*(int*)p == 1){
            test = (char *) malloc(5000);
            strcpy(test, "Hello");
        }
        else if (*(int*)p == 2){
            test = (char *) malloc(6000);
            strcpy(test, "Hello22");
        }  

        printf("Hello from thread %d I am memoryspace %p\n", *(int*)p, test);
        //my_pthread_yield();
        while (thentime - nowtime < 8000000){
            gettimeofday(&thetime, NULL);
            thentime = (1000000 * thetime.tv_sec) + thetime.tv_usec;
            //test = "In Progress";
            
            //test2 = "In Progress 2";
            printf("%s from thread %d\n My memory address is: %p\n", test, *(int*)p, test);
            my_pthread_yield();
        }
        printf("Hello from thread %d I am ended\n", *(int*)p);

	my_pthread_exit(0);
	return 0;
}

int main(){
    my_pthread_t thread;
    int id, arg1, arg2;
    arg1 = 1;
    id = my_pthread_create(&thread, NULL, myThread, (void*)&arg1);
    my_pthread_yield();
    arg2 = 2;
    //my_pthread_join(thread, NULL);
    myThread((void*)&arg2);
    
    return 0;
}
