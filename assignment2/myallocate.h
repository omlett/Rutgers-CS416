/****************************************************************/
// Created by Pranav Katkamwar, Ben De Brasi, Swapneel Chalageri
// Spring 2017 - CS416
/****************************************************************/

#ifndef MYALLOCATE_H
#define MYALLOCATE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread_t.h"

#define malloc(x) 		myallocate(x, THREADREQ)
#define free(x)			mydeallocate(x, THREADREQ)
#define PAGE_SIZE		4096
#define OS_SIZE			4096000
#define MEMORY_SIZE		1024*1024*8
#define SWAPFILE_SIZE	1024*1024*16
#define THREADREQ		1
#define LIBRARYREQ		2

/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef struct Meta{
	int size;
	int is_free;
	struct Meta *prev;
	struct Meta *next;
} Meta;

typedef struct page_meta_data{
	int isOsRegion;					// is page in OS region
	int tid;						// thread id
	int page_num;					// page counter
	int inUse;						// being used
	struct page_meta_data *next;	// next page owned by thread
	struct page_meta_data *prev;	// previous page owned by thread
} page_meta;

/****************************************************************/
// Extern Variables
/****************************************************************/
extern int current_tid;			
extern int prev_tid;
/****************************************************************/
// Global Variables
/****************************************************************/
Meta * HEAD;						// pointer to 1st page of current thread

/****************************************************************/
// Function Headers
/****************************************************************/
// Paging Library
/****************************************************************/
void initAll();
void *requestPage();
void swapEmptyPage(void *newPage, void *oldPage);
void swapPage(int sniped_tid, int sniped_page, void *evict);
void freeBuffer();
void *getHead();
extern void mprotect_setter(int current_tid, int prev_tid);
extern void mprotect_setter_dead(int current_tid);
/****************************************************************/
// myallocate Library
/****************************************************************/
void initMem();
void organizeMem(Meta * curr, int size);
void *myallocate(size_t size, int isThread);
void mydeallocate(void *ptr, int isThread);

#endif
