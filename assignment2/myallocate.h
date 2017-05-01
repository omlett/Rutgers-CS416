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
#include "paging.h"

#define malloc(x) 		myallocate(x, THREADREQ)
#define free(x)			mydeallocate(x, THREADREQ)
#define PAGE_SIZE		4096
#define OS_SIZE			4096000
#define MEMORY_SIZE		1024*1024*8
#define SWAPFILE_SIZE	1024*1024*16
#define THREADREQ		1
#define LIBRARYREQ		2
#define MALLOC_FLAG		1
#define	FREE_FLAG		2

/****************************************************************/
// Structure Definitions
/****************************************************************/
typedef struct Meta{
	int size;
	int is_free;
	struct Meta *prev;
	struct Meta *next;
} Meta;

/****************************************************************/
// Imported Variables
/****************************************************************/
int numFreePagesMem;			// counter that tracks the number of free pages in physical memory
int numFreePagesSF;			// counter that tracks the number of free pages in swap file

char *all_memory;			// pointer to page 0 (1st byte of 8MB physical memory + 16MB swapfile)
void *base_page;		// pointer to page 1 (1st byte of OS region)

page_meta page_table[6144];	// page table: array of page_meta_data structs
/****************************************************************/
// Global Variables
/****************************************************************/
Meta * HEAD;						// pointer to 1st page of current thread
char firstOS;

/****************************************************************/
// Function Headers
/****************************************************************/
void initMem(int req, void * newHead);
void organizeMem(Meta * curr, int size);
void *myallocate(size_t size, int isThread);
void mydeallocate(void *ptr, int isThread);

#endif
