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


current_tid = 1;					// initialized to tid of the main thread, updated when main thread swaps to another thread

static char *all_memory;			// pointer to page 0 (1st byte of 8MB physical memory + 16MB swapfile)
void *buffer_page = NULL;			// pointer to page 0 (buffer page used for swapping pages)
static void *base_page = NULL;		// pointer to page 1 (1st byte of OS region)
static void *user_space = NULL;		// pointer to page 1000 (1st byte of user region)
static void *swap_file = NULL;		// pointer to page 2048 (1st byte of swap file)

static page_meta page_table[6144];	// page table: array of page_meta_data structs


/****************************************************************/
/*
// SIGSEGV Handler
*/
/****************************************************************/

static void handler(int sig, siginfo_t *si, void *unused) {
	void *swapout;
	int sniped_tid, sniped_page, page_fault_page;

	page_fault_page = 2048 - ((((char *)all_memory + MEMORY_SIZE) - ((char *)si->si_addr + PAGE_SIZE))/PAGE_SIZE);
	swapout = (char *)all_memory + (page_fault_page*PAGE_SIZE);

	sniped_tid = current_tid;
	sniped_page = page_table[page_fault_page].page_num;

	swapPage(sniped_tid, sniped_page, swapout);
}
    
/****************************************************************/
/*
// Paging Library
*/
/****************************************************************/

// allocate 24MB (physical mem + swapfile) with memalign
// set pointers to key regions
// initialize page table
void initAll(){
	all_memory = (char *) memalign(PAGE_SIZE , (MEMORY_SIZE+SWAPFILE_SIZE) * sizeof(char));			//Creates 8mb physical mem + 16mb swapfile
	buffer_page = (void *) all_memory;
	base_page = (char *)all_memory + PAGE_SIZE;
	user_space = (char *)all_memory + 1000*PAGE_SIZE;
	swap_file = (char*)all_memory + 2048*PAGE_SIZE;

	int x;
	
	// Initialize Page Table
	// OS region
	for (x = 1; x < 1000; x++){
		page_table[x].isOsRegion = 1;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	} 
	// Memory
	for (x = 1000; x < 2048; x++){
		page_table[x].isOsRegion = 0;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	}
	// Swap file
	for (x = 2048; x < 6144; x++){
		page_table[x].isOsRegion = 0;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	}	
	
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = handler;
	
	if (sigaction(SIGSEGV, &sa, NULL) == -1){
		printf("Fatal error setting up signal handler.\n");
		exit(EXIT_FAILURE);
	}
	
}

// returns void* pointer to 1st page of current thread
void* getHead(int req){
	void *the_head;

	int y;
	if (req == THREADREQ){
		for (y = 1000; y < 2048; y++){
			// find page that belongs to current thread and is the 1st page
			if ((page_table[y].tid == current_tid) && (page_table[y].page_num == 1)){
				// set head to point to 1st byte of that page
				the_head = (y * PAGE_SIZE) + (char *)all_memory;
				return the_head; 
			}
		}
		// if such a page is not found
		for (y = 1000; y < 2048; y++){
			// find page that is free
			if (page_table[y].inUse == 0){
				// set head to point to 1st byte of that page
				the_head = (y * PAGE_SIZE) + (char *)all_memory;
				page_table[y].tid = current_tid;
				page_table[y].page_num = 1;
				page_table[y].inUse = 1;

				return the_head;
			}
		}
	}
	else if (req == LIBRARYREQ){
		// set head to point to page 1 (1st byte of OS region)
		the_head = base_page;
		return the_head;
	}
}

// returns ptr to a page that is free
// returns NULL if no pages are free
void* requestPage(){

	void* new_page;
	int y;

	for (y = 1000; y < 2048; y++){
		// if found page that is free
		if (page_table[y].inUse == 0){
			page_table[y].inUse = 1;
			page_table[y].tid = current_tid;
			new_page = (y * PAGE_SIZE) + (char *)all_memory;
			return new_page; 
		}
	}

	// if reaches here, no free pages in physical memory
	// go look in swap file
	for (y = 2048; y < 6144; y++){
		// if found page that is free
		if (page_table[y].inUse == 0){
			page_table[y].inUse = 1;
			page_table[y].tid = current_tid;
			//swap this page in swap file with a page in physical memory
		}
	}

	// if reaches here, physical memory and swap file have no free pages
	return NULL;
}

//A thread requests a page too big for it's current pages to handle. This function assumes there is an empty page in memory to make that request possible but it needs to move the next page  to continue the illusion of contigious memory 
void swapEmptyPage(void *newPage, void *oldPage){

	// Copy Sniped Page to Buffer
	memcpy(buffer_page, oldPage, PAGE_SIZE);

	// Evict Page to Sniped Page
	memset(newPage, 0, PAGE_SIZE);

	// Copy Buffered Page into Target
	memcpy(newPage, buffer_page, PAGE_SIZE);

	// Reset Buffer
	freeBuffer();
}	

void swapPage(int sniped_tid, int sniped_page, void *evict){

	int y; 
	void *curr_page;

	for(y = 1000; y < 2048; y++){
		if ((page_table[y].tid == sniped_tid)) 
			break;
	}

	page_meta *tmp = &page_table[y];

	while(sniped_page != tmp->page_num){
		
		if(sniped_page >  tmp->page_num)
			tmp = tmp->next;
		else
			tmp = tmp->prev;
	}

	curr_page = tmp;

	// Copy Sniped Page to Buffer
	memcpy(buffer_page, curr_page, PAGE_SIZE);

	// Evict Page to Sniped Page
	memcpy(curr_page, evict, PAGE_SIZE);

	// Copy Buffered Page into Target
	memcpy(evict, buffer_page, PAGE_SIZE);

	// Reset Buffer
	freeBuffer();
}

void freeBuffer(){

	//set all memory in buffer to 0 
	//pass the address of the buffer, target value, and how long it is 

	memset(&buffer_page, 0, PAGE_SIZE);

	return 1;
}

void mprotect_setter(int current_tid, int prev_tid){
	int i;
	for(i = 0; i < (MEMORY_SIZE/PAGE_SIZE); i++){
	
		if(page_table[i].tid ==  NULL)
			break;
	
		if(page_table[i].tid == prev_tid){
			mprotect(&page_table[i] , PAGE_SIZE, PROT_NONE);
		}

		else if(page_table[i].tid == current_tid){
				mprotect(&page_table[i] , PAGE_SIZE, PROT_READ | PROT_WRITE);
		}
	}

}

void mprotect_setter_dead(int current_tid){
	int i;
	for(i = 0; i < MEMORY_SIZE / PAGE_SIZE; i += 4096){
	
		if(page_table[i].tid ==  NULL)
			break;
	
		if(page_table[i].tid == current_tid)
			mprotect(&page_table[i] , 4096, PROT_READ | PROT_WRITE);	
		else
			mprotect(&page_table[i] , 4096, PROT_NONE);
	}
}


/****************************************************************/
/*
// myallocate Library
*/
/****************************************************************/


// Initialize HEAD.
void initMem(int req){

	//raise(SIGSEGV);

	HEAD->next = NULL;
	HEAD->prev = NULL;
	HEAD->is_free = 1;
	if (req == 1){
		HEAD->size = PAGE_SIZE-sizeof(Meta);
	}
	else if (req == 2){
		HEAD->size = OS_SIZE-sizeof(Meta);
	}
}



/* function that take large piece of memory and allocates only the 
portion needed and creates a Meta component for the rest */
void organizeMem(Meta * curr, int size){

	curr->is_free = 0;
	curr->size = size;

	//we need to find the address at which the next Meta compnent will live
	Meta * new = (char *)curr + sizeof(Meta) + size;
	new->size = curr->size - size - sizeof(Meta);
	new->is_free = 1;
	new->next = curr->next;
	new->prev = curr;

	curr->next = new;
}

void *myallocate(size_t size,int isThread){

	// initialize memory on 1st call
	if (base_page == NULL){
		initAll();
	}

	// Zero space requested
	if (size <= 0){
		fprintf(stderr,"ERROR: zero/negative request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
/*
	// >5000 space requested
	if (size > (PAGE_SIZE-sizeof(Meta))){
		fprintf(stderr,"ERROR: overload request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
*/
	Meta *curr, *prev;				// For Traversal
	void * memptr;					// Pointer to Return to User

	// get pointer to 1st page of current thread
	HEAD = (Meta *) getHead(isThread);
	
/*
	if (isThread == 1){
		HEAD = user_space;
	}
	else if (isThread == 2){
		HEAD = base_page;
	}
*/

	// if 1st page of current thread has no metadata, create metadata
	if (!(HEAD->size)){
		initMem(isThread);
	}

	curr = HEAD;
	// first traverse and find empty spacex
	if (curr->next != NULL){
		while ((curr != NULL) && (curr->next != NULL)){

			if (curr->is_free == 1 && curr->size >= size){
				break;
			}
			
			//iterate through array
			prev = curr;
			if (curr->next != NULL){
				curr = curr->next;
			}
		}
	}
	// curr now points to either free block of sufficient size or the last block

	if (curr == NULL){
		fprintf(stderr,"ERROR: unexpected null curr. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	// if curr points to free block of sufficient size, but not enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size >= size && curr->size <= size + sizeof(Meta)){
		//initialize Metadata 
		curr->is_free = 0;
		memptr = (char *)curr + sizeof(Meta);
		return memptr;
	}

	// if curr points to free block of sufficient size, with enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size > size + sizeof(Meta)){
		organizeMem(curr, size);
		memptr = (char *)curr + sizeof(Meta);
		return memptr;
	}
	
	// Last chunk is not free, or not large enough for request
	if ((curr->is_free == 0) || (curr->size < size)){
		// get a free page, nextPage points at free page
		void *nextPage = requestPage();
		// there are no free pages
		if (!nextPage){
			fprintf(stderr,"ERROR: insufficient pages. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
		if (((char *)curr + PAGE_SIZE) < ((char *)all_memory + MEMORY_SIZE)){
			// Code Mod Function here

			// the page number of the page that curr points at
			int currPageEntry = 2048 - (((char *)all_memory + MEMORY_SIZE - (char *)curr)/PAGE_SIZE);
			// the page number of the page that nextPage points at
			int newPageEntry = 2048 - (((char *)all_memory + MEMORY_SIZE - (char *)nextPage)/PAGE_SIZE);
			// the page number of the page after the one curr points at
			int nextPageEntry = currPageEntry + 1;
			// page counter of the page that curr points at
			int getPageNum = page_table[currPageEntry].page_num;
			
			// if the new free page is not directly after the current one
			if (newPageEntry != nextPageEntry){
				// if page after current one is not thread's 1st page
				if (page_table[nextPageEntry].page_num > 1){
					page_table[nextPageEntry].prev->next = &page_table[newPageEntry];
				}
				page_table[newPageEntry] = page_table[nextPageEntry];
				
				page_table[currPageEntry].next = &page_table[nextPageEntry];
				page_table[nextPageEntry].prev = &page_table[currPageEntry];
				page_table[nextPageEntry].next = NULL;
				page_table[nextPageEntry].tid = current_tid;
				page_table[nextPageEntry].inUse = 1;
				page_table[nextPageEntry].page_num = getPageNum + 1;
				
				swapEmptyPage(nextPage, (char *)curr + sizeof(Meta) + curr->size + 1);
			}
			else{
				page_table[currPageEntry].next = &page_table[nextPageEntry];
				page_table[nextPageEntry].prev = &page_table[currPageEntry];
				page_table[nextPageEntry].next = NULL;
				page_table[nextPageEntry].tid = current_tid;
				page_table[nextPageEntry].inUse = 1;
				page_table[nextPageEntry].page_num = getPageNum + 1;
			}
		}
		else {
			return NULL;
		}
		
		if (curr->is_free == 0){
			organizeMem(nextPage, size);
			memptr = (char *)nextPage + sizeof(Meta);
		}
		else if (curr->size < size){
			curr->size = curr->size + PAGE_SIZE;
			organizeMem(curr, size);
			memptr = (char *)curr + sizeof(Meta);
		}
		return memptr;
	}

	else{
		// NSF error
		fprintf(stderr,"ERROR: insufficient space. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
}

void mydeallocate(void *ptr, int isThread){

	HEAD = getHead(isThread);
	int uLimit;

	if (isThread == 1){
		uLimit = PAGE_SIZE;
	}
	else if (isThread == 2){
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


