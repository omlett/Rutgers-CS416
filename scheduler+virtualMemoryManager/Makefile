CC      =       gcc
FLAGS	=	    -g
COMPILE =       ${CC} ${FLAGS}

all:    test
test: test.o myallocate.o
	${COMPILE} -o test test.o myallocate.o
test.o: test.c
	${COMPILE} -c test.c
msimple:		msimple.c my_pthread.c my_pthread_t.h myallocate.c myallocate.h paging.c paging.h
	${COMPILE} -o msimple msimple.c my_pthread.c myallocate.c paging.c
mcounter:	mcounter.c my_pthread.c my_pthread_t.h myallocate.c myallocate.h paging.c paging.h
	${COMPILE} -o mcounter mcounter.c my_pthread.c myallocate.c paging.c
mtsort:		mtsort.c my_pthread.c my_pthread_t.h myallocate.c myallocate.h paging.c paging.h
	${COMPILE} -o mtsort mtsort.c my_pthread.c myallocate.c paging.c
mmatrix:	mmatrix.c my_pthread.c my_pthread_t.h myallocate.c myallocate.h paging.c paging.h
	${COMPILE} -o mmatrix mmatrix.c my_pthread.c myallocate.c paging.c
testj:	test_johnny.c
	${COMPILE} -o testj test_johnny.c my_pthread.c myallocate.c
clean:
	rm -rf *.o test msimple mcounter mmatrix mtsort
