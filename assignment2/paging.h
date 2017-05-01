/****************************************************************/
// Created by Pranav Katkamwar, Andrew Khazonovic, Karl Xu
// Spring 2017 - CS416
/****************************************************************/

#ifndef PAGING_H
#define PAGING_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_pthread_t.h"
#include "myallocate.h"

#define FIRST_USER_PAGE	1000
#define FIRST_SWAP_PAGE	2048
#define MALLOC_FLAG		1
#define	FREE_FLAG		2

/****************************************************************/
// Extern Variables
/****************************************************************/
extern int current_tid;			
extern int prev_tid;

/****************************************************************/
// Structure Definitions
/****************************************************************/

typedef struct page_meta_data{
	int entryNum;
	int isOsRegion;					// is page in OS region
	int tid;						// thread id
	int page_num;					// page counter
	int inUse;						// being used
} page_meta;


/****************************************************************/
// Global Variables
/****************************************************************/
extern int numFreePagesMem;			// counter that tracks the number of free pages in physical memory
extern int numFreePagesSF;			// counter that tracks the number of free pages in swap file

extern char *all_memory;			// pointer to page 0 (1st byte of 8MB physical memory + 16MB swapfile)
static void *buffer_page;			// pointer to page 0 (buffer page used for swapping pages)
extern void *base_page;		// pointer to page 1 (1st byte of OS region)
static void *user_space;		// pointer to page FIRST_USER_PAGE (1st byte of user region)
static void *swap_file;		// pointer to page FIRST_SWAP_PAGE (1st byte of swap file)

extern page_meta page_table[6144];	// page table: array of page_meta_data structs


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
void *getHead(int req, int flag);
extern void mprotect_setter(int current_tid, int prev_tid);
static void handler(int sig, siginfo_t *si, void *unused);

#endif