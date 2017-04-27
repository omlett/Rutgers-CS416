#include<stdio.h>
#include"my_pthread_t.h"

void* myThread(void* p){
    	printf("Hello from thread %d\n", *(int*)p);
	my_pthread_exit(0);
	return 0;
}

int main(){
    my_pthread_t thread;
    int id, arg1, arg2;
    arg1 = 1;
    id = my_pthread_create(&thread, NULL, myThread, (void*)&arg1);
    //pthread_yield();
    arg2 = 2;
    my_pthread_join(thread, NULL);
    myThread((void*)&arg2);
    
    return 0;
}
