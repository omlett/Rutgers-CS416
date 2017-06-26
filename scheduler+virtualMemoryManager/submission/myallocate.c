/****************************************************************/
// Created by Pranav Katkamwar, Ben De Brasi, Swapneel Chalageri
// Spring 2017 - CS416
/****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <sys/mman.h>
#include "myallocate.h"
#include "paging.h"
#include <errno.h>

					// initialized to tid of the main thread, updated when main thread swaps to another thread
char firstOS = '0';

 

/****************************************************************/
/*
// myallocate Library
*/
/****************************************************************/


// Initialize HEAD.
void initMem(int req, void * newHead){

	mprotect( (newHead), PAGE_SIZE, PROT_READ | PROT_WRITE );

	HEAD = (Meta *) newHead;
	

	if (req == THREADREQ){
		HEAD->size = PAGE_SIZE-sizeof(Meta);
	}

	else if (req == LIBRARYREQ){
		HEAD->size = OS_SIZE-sizeof(Meta);
	}

	HEAD->next = NULL;
	HEAD->is_free = 1;
}



/* function that take large piece of memory and allocates only the 
portion needed and creates a Meta component for the rest */
void organizeMem(Meta * curr, int size){

	// create next metadata component
	Meta* new = (Meta*)((char *)curr + sizeof(Meta) + size);
	new->size = curr->size - sizeof(Meta) - size;
	new->is_free = 1;
	new->next = curr->next;

	curr->is_free = 0;
	curr->size = size;
	curr->next = new;
}

void *myallocate(size_t size, int req){

	//printf("\nIn myallocate.\n req is %i\n", req);
	// initialize memory on 1st call
	if (base_page == NULL){
		initAll();
	}

	// Zero space requested
	if (size <= 0){
		fprintf(stderr,"ERROR: zero/negative request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	// >USER-SIzE space requested
	if (size > (USER_SIZE-sizeof(Meta))){
		fprintf(stderr,"ERROR: overload request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	Meta *curr;				// For Traversal
	void * memptr;					// Pointer to Return to User

	// get pointer to 1st page of current thread
	HEAD = (Meta *) getHead(req, MALLOC_FLAG);

	//printf("\n\nHeadId :%i\n\n", ((void *)HEAD - (void *)all_memory));

	// if thread does not own any pages any there are no free pages left in physical memory and swap file
	if (HEAD == NULL) {
		fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
	
	// curr points to 1st page of current thread
	curr = HEAD;

	// traverse through metadata of current thread
	while (curr != NULL && curr->next != NULL) {

		// if block is free and of sufficient size
		if (curr->is_free == 1 && curr->size >= size){
			break;
		}
			
		curr = curr->next;
	}

	// curr now points to either free block of sufficient size or the last block

	if (curr == NULL){
		fprintf(stderr,"ERROR: unexpected null curr. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	// if curr points to free block of sufficient size, but not enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size >= size && curr->size <= size + sizeof(Meta)){

		curr->is_free = 0;
		memptr = (char *)curr + sizeof(Meta);

		//printf("Case 1\n");
		return memptr;
	}

	// if curr points to free block of sufficient size, with enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size >= (size + sizeof(Meta))){

		organizeMem(curr, size);
		memptr = (char *)curr + sizeof(Meta);

		//printf("Case 2\n");
		return memptr;
	}
	
	// insufficient memory checks below

	// if last block is not free
	if (curr->is_free == 0) {
		// if not enough memory left to allocate size + new metadata
		if(((size + sizeof(Meta) > (numFreePagesMem * PAGE_SIZE))&& ((size + sizeof(Meta)) > (numFreePagesSF * PAGE_SIZE)))) {
			fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
	}

	// if last block is not large enough for request
	if (curr->size < size) {
		// if not enough memory left to allocate size
		if(size > curr->size + numFreePagesMem * PAGE_SIZE && size > curr->size + numFreePagesSF * PAGE_SIZE) {
			fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
	}

	// if curr is in page 2047 (last page in physical memory)
	if( (char*)curr + PAGE_SIZE >= all_memory + MEMORY_SIZE ) {
		fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	// Last block is not free, or not large enough for request
	if ((curr->is_free == 0) || (curr->size < size + sizeof(Meta))){

		// 1 if current thread needs more pages to satisfy request, 0 otherwise
		int needPages = 1;
		// guesses how many pages the current thread needs to satisfy request, increments each iteration
		int numPagesNeeded = 0;

		// each iteration gets a new free page
		// the nth iteration moves the data from the nth page after the current page to the free page
		// then the nth page after the current page belongs to the current thread		
		while (needPages) {

			numPagesNeeded++;

			// get a free page, freePage points at free page
			void *freePage = requestPage();
			//printf("Case 3\n");

			// there are no free pages
			if (!freePage){
				fprintf(stderr,"ERROR: insufficient pages. FILE: %s, LINE %d\n", __FILE__, __LINE__);
				return NULL;
			}

			// the page number of the page that curr points within
			int currPageNum = ((void *)curr - (void *)all_memory)/PAGE_SIZE;
			// the page number of the page that freePage points at
			int freePageNum = ((void *)freePage - (void *)all_memory)/PAGE_SIZE;
			// the page number of the next needed page
			// if current thread needs 5 pages to satsify request: curr+1 is next needed page, then curr+2 is next needed page, ..., curr+5 is next needed page
			int nextPageNum = currPageNum + numPagesNeeded;
			void* nextPage = (void*)(all_memory + nextPageNum * PAGE_SIZE);
			// page counter of the page before next needed page
			int pageCounter = page_table[nextPageNum - 1].page_num;
			
			// if the free page is not the next needed page (directly after: the current page or the last needed page)
			if (freePageNum != nextPageNum){

				// move data in next needed page to free page and update metadata, so next needed page can be given to current thread

				// set free page metadata equal to next needed page metadata
				page_table[freePageNum] = page_table[nextPageNum];
				page_table[freePageNum].entryNum = page_table[nextPageNum].entryNum;
				

				// set next needed page metadata
				page_table[nextPageNum].tid = current_tid;
				page_table[nextPageNum].inUse = 1;
				page_table[nextPageNum].page_num = pageCounter + 1;

				
				// move next needed page data to free page, then zero out next needed page data
				mprotect( (void*)(all_memory + freePageNum * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				mprotect( (void*)(all_memory + nextPageNum * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				swapEmptyPage(freePage, nextPage);
				mprotect( (void*)(all_memory + freePageNum * PAGE_SIZE), PAGE_SIZE, PROT_NONE );
			}
			// if the new free page is the next needed page (directly after: the current page or the last needed page)
			else{

				// set next page metadata
				page_table[nextPageNum].tid = current_tid;
				page_table[nextPageNum].inUse = 1;
				page_table[nextPageNum].page_num = pageCounter + 1;


				mprotect( (void*)(all_memory + nextPageNum * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
			}
			// now the next needed page belongs to the current thread

			// if last block is not free
			if (curr->is_free == 0){

				// iteration 1: the next needed page is right after the current page
				if (numPagesNeeded == 1) {
					// initialize 1st metadata component for the next needed page
					((Meta*)nextPage)->next = NULL;
					((Meta*)nextPage)->prev = curr;
					((Meta*)nextPage)->is_free = 1;
					((Meta*)nextPage)->size = PAGE_SIZE-sizeof(Meta);

					curr->next = ((Meta*)nextPage);
				}
				// iteration 2 or more
				else {
					// 1st metadata component of page right after the current page: increase block size by another page
					curr->next->size += PAGE_SIZE;
				}
			
				// if more pages are needed
				if (size > curr->next->size) {
					continue;
				}

				// have enough pages if reach here

				// if curr->next points to free block of sufficient size, with enough size to chunk off another metadata + free block
				if (curr->next->size > size + sizeof(Meta))	{
					organizeMem( curr->next, size );
				}
				
				// set memptr to the free block and return it
				curr->next->is_free = 0;
				memptr = (char*)curr->next + sizeof(Meta);
				return memptr;
			}
			// else last block is of insufficient size
			else {

				// increase block size by another page
				curr->size += PAGE_SIZE;

				// if more pages are needed
				if (size > curr->size) {
					continue;
				}

				// have enough pages if reach here

				// if curr points to free block of sufficient size, with enough size to chunk off another metadata + free block
				if (curr->size > size + sizeof(Meta)) {
					organizeMem( curr, size );
				}

				// set memptr to the free block and return it
				curr->is_free = 0;
				memptr = (char*)curr + sizeof(Meta);
				return memptr;
			}
		}
	}

	else{
		// Error
		fprintf(stderr,"ERROR. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
}


void mydeallocate(void *ptr, int req){

	HEAD = getHead(req, FREE_FLAG);
	if(HEAD == NULL) {
		fprintf(stderr,"ERROR: thread does not have any malloc'd memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return;
	}

	int uLimit;

	if (req == 1){
		uLimit = PAGE_SIZE;
	}
	else if (req == 2){
		uLimit = OS_SIZE;
	}

	// Basic Error Checks
	if (!HEAD){
		fprintf(stderr,"ERROR: invalid head. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return;
	}
	
	if (ptr == NULL){
		fprintf(stderr,"ERROR: null free request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return;
	}
	
	if ((ptr < HEAD) || (ptr > (HEAD + uLimit))){
		fprintf(stderr,"ERROR: out of bounds free request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return;
	}

	Meta *toFree;
	toFree = (Meta *)(ptr - sizeof(Meta));
	Meta *curr, *previous, *nxt;
	curr = HEAD;


	// Determine Pointer's Validity
	while((toFree != curr) && (curr != NULL)){

		if ((curr->next) == NULL){

			curr = NULL;

		}
		else{

			curr = curr->next;

		}

	}


	if(toFree == curr){
		if ((toFree->is_free) == 0){
			if (toFree->prev != NULL){
				previous = toFree->prev;
			}
			else {
				previous = NULL;
			}
			if (toFree->next != NULL){
				nxt = toFree->next;
			}
			else {
				nxt = NULL;
			}
			if (nxt != NULL){
				// Next Block is Free; Consolidate.
				if (nxt->is_free == 1){
					if (nxt->next != NULL){
						toFree->size += (sizeof(struct Meta) + nxt->size);
						toFree->next = nxt->next;
						nxt->next->prev = toFree;
						toFree->is_free = 1;
					}
					else{
						toFree->size += (sizeof(struct Meta) + nxt->size);
						toFree->next = nxt->next;
						toFree->is_free = 1;
					}
				}
			}
			else{
				toFree->is_free = 1;
			}
		
			// Previous Block is Free; Consolidate.
			if ((previous != NULL) && (previous->is_free == 1)){
				previous->size += (sizeof(struct Meta) + toFree->size);
				previous->next = toFree->next;
				toFree->next->prev = previous;
			}
		}
		else {
			fprintf(stderr,"ERROR: double free request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return;
		}
	}
	else{
		fprintf(stderr,"ERROR: unallocated free request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return;
	}
}


