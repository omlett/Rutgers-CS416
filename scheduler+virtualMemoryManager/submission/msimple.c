#include <stdio.h>
#include "my_pthread_t.h"
#include "myallocate.h"
#include "paging.h"
#include <string.h>

void* myThread(){
       
	printf("---Line 9 in msimple---\n");

	char* ptr1 = malloc(5000);
	printf("---Line 12 in msimple---\n");
	char* ptr2 = malloc(100000);

	printf("---Line 15 in msimple---\n");

	free(ptr1);
	free(ptr2);

	printf("---Line 20 in msimple---\n");

	my_pthread_exit(0);
	return 0;
}

int main(){


	my_pthread_t thread1;
	my_pthread_t thread2;

    int id1;
	int id2;
 
    id1 = my_pthread_create(&thread1, NULL, myThread, NULL);
	id2 = my_pthread_create(&thread2, NULL, myThread, NULL);

	char* ptr3 = malloc(5000);
	char* ptr4 = malloc(5000);

	free(ptr3);
	free(ptr4);

	printf("---Line 44 in main---\n");

    //pthread_yield();
    my_pthread_join(thread1, NULL);
	my_pthread_join(thread2, NULL);

	printf("---Line 50 in main---\n");

    myThread();
    
	printf("---Line 54 in main---\n");

    return 0;




/*
	int i;
	for (i=0; i<10; i++) {
		char* ptr1 = malloc(5000);
		free(ptr1);
	}
*/

/*
	char* ptr1 = malloc(5000);
	char* ptr2 = malloc(5000);
	char* ptr3 = malloc(5000);
	char* ptr4 = malloc(5000);
	char* ptr5 = malloc(5000);
	char* ptr6 = malloc(5000);
	char* ptr7 = malloc(5000);
	char* ptr8 = malloc(5000);
	char* ptr9 = malloc(5000);
	char* ptr10 = malloc(5000);


	free(ptr1);
	free(ptr2);
	free(ptr3);
	free(ptr4);
	free(ptr5);
	free(ptr6);
	free(ptr7);
	free(ptr8);
	free(ptr9);
	free(ptr10);
*/


}




/*
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
*/
