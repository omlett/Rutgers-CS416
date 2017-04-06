/****************************************************************/
// Created by Pranav Katkamwar, Ben De Brasi, Swapneel Chalageri
// Spring 2017 - CS416
/****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _MYALLOCATE_H
#define _MYALLOCATE_H

#define malloc(x)	myallocate(x, THREADREQ)
#define free(x)	mydeallocate(x, THREADREQ)
#define PAGE_SIZE	4096
#define OS_SIZE	4096000
#define MEMORY_SIZE	1024*1024*8
#define THREADREQ	1
#define LIBRARYREQ	2

/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef struct Meta{
	int size;
	int is_free;
	struct Meta *prev;
	struct Meta *next;
} Meta;

typedef struct page_meta_data page_meta;
typedef page_meta page_ptr;
/****************************************************************/
// Extern Variables
/****************************************************************/
extern int current_tid;
extern int prev_tid;
/****************************************************************/
// Global Variables
/****************************************************************/
Meta * HEAD;
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
void mprotect_setter(int current_tid, int prev_tid);
mprotect_setter_dead(int current_tid);
/****************************************************************/
// myallocate Library
/****************************************************************/
void initMem();
void organizeMem(Meta * curr, int size);
void *myallocate(size_t size, int isThread);
void mydeallocate(void *ptr, int isThread);

#endif
