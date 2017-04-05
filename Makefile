CC      =       gcc
FLAGS	=	-pedantic -Wall -g -w
COMPILE =       ${CC} ${FLAGS}

all:    test
test: test.o myallocate.o
	${COMPILE} -o test test.o myallocate.o
test.o: test.c
	${COMPILE} -c test.c
simple:		simple.c
	${COMPILE} -o simple simple.c my_pthread.c myallocate.c
mcounter:	mcounter.c
	${COMPILE} -o mcounter mcounter.c my_pthread.c myallocate.c
mtsort:		mtsort.c
	${COMPILE} -o mtsort mtsort.c my_pthread.c myallocate.c
mmatrix:	mmatrix.c
	${COMPILE} -o mmatrix mmatrix.c my_pthread.c myallocate.c
testj:	test_johnny.c
	${COMPILE} -o testj test_johnny.c my_pthread.c myallocate.c
clean:
	rm -rf *.o test simple mcounter mmatrix mtsort
