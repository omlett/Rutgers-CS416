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
#include "my_pthread_t.h"


/****************************************************************/
/*
// Global Declarations
*/
/****************************************************************/

// Structures
struct page_meta_data{
	int isOsRegion;
	int tid;					// Thread Id
	int page_num;					// Page Counter
	int inUse;					// Is Free
	page_ptr *next;					// Next Page
	page_ptr *prev;					// Previous Page
};

current_tid = 1;
static char *all_memory;
static void *base_page = NULL;
void *buffer_page = NULL;
static void *user_space = NULL;
static page_ptr page_table[2048];

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

// Initialize All Memory
void initAll(){
	all_memory = (char *) memalign(PAGE_SIZE , MEMORY_SIZE * sizeof(char));			//Creates 8mb physical mem
	buffer_page = (void *) all_memory;
	base_page = (char *)all_memory + PAGE_SIZE;
	user_space = (char *)all_memory + 1000*PAGE_SIZE;

	int x;
	
	for (x = 1; x < 1000; x++){
		page_table[x].isOsRegion = 1;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	} 
	for (x = 1000; x < 2048; x++){
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

void *getHead(int req){
	void *the_head;

	int y;
	if (req == 1){
		for (y = 1000; y < 2048; y++){
			if ((page_table[y].tid == current_tid) && (page_table[y].page_num == 1)){
				the_head = (y * PAGE_SIZE) + (char *)all_memory;
				return the_head; 
			}
		}
		for (y = 1000; y < 2048; y++){
			if (page_table[y].inUse == 0){
				the_head = (y * PAGE_SIZE) + (char *)all_memory;
				page_table[y].tid = current_tid;
				page_table[y].page_num = 1;
				page_table[y].inUse = 1;
				return the_head;
			}
		}
	}
	else if (req == 2){
		the_head = base_page;
		return the_head;
	}
}

void *requestPage(){

	void *new_page;
	int y;

	for (y = 1000; y < 2048; y++){
		if (page_table[y].inUse == 0){
			page_table[y].inUse = 1;
			page_table[y].tid = current_tid;
			new_page = (y * PAGE_SIZE) + (char *)all_memory;
			return new_page; 
		}
	}

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

	page_ptr *tmp = &page_table[y];

	while(sniped_page != tmp->page_num){
		
		if(sniped_page >  tmp->page_num)
			tmp = tmp->next;
		else
			tmp = tmp->prev;

	}

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

mprotect_setter_dead(int current_tid){
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
	//we need to find the address at which the next Meta compnent will live
	Meta * new = ((char *)curr + sizeof(Meta) + size);
	new->size = (curr->size) - size - sizeof(Meta);
	new->is_free = 1;
	curr->size = size;
	new->next = curr->next;
	new->prev = curr;
	curr->next = new;
}

void *myallocate(size_t size,int isThread){

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

	HEAD = (Meta *) getHead(isThread);
	
/*
	if (isThread == 1){
		HEAD = user_space;
	}
	else if (isThread == 2){
		HEAD = base_page;
	}
*/
	if (!(HEAD->size)){
		initMem(isThread);
	}

	curr = HEAD;
	// first traverse and find empty space
	if (curr->next != NULL){
		while ((curr != NULL) && (curr->next != NULL)){

			if (curr->is_free == 1){
				if (curr->size >= size){
					break;
				}
			}
			
			//iterate through array
			prev = curr;
			if (curr->next != NULL){
				curr = curr->next;
			}
		}
	}

	if (curr == NULL){
		fprintf(stderr,"ERROR: unexpected null curr. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	if ((curr->size) == size){
		//initialize Metadata 
		curr->is_free = 0;
		curr->size = size;
		memptr = (char *)curr + sizeof(Meta);
		return memptr;
	}
	
	// Last chunk is not free, or not large enough for request
	if ((curr->is_free == 0) || (curr->size < size)){
		void *nextPage = requestPage();
		if (!nextPage){
			fprintf(stderr,"ERROR: insufficient pages. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
		if (((char *)curr + PAGE_SIZE) < ((char *)all_memory + MEMORY_SIZE)){
			// Code Mod Function here
			int currPageEntry = 2048 - ((((char *)all_memory + MEMORY_SIZE) - ((char *)curr)+PAGE_SIZE)/PAGE_SIZE);
			int newPageEntry = 2048 - ((((char *)all_memory + MEMORY_SIZE) - (char *)nextPage)/PAGE_SIZE);
			int nextPageEntry = currPageEntry + 1;
			int getPageNum = page_table[currPageEntry].page_num;
			
			if (newPageEntry != nextPageEntry){
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

	// Sufficient Space Found.
	else if((curr->size) > (size + sizeof(Meta))){
		organizeMem(curr, size);
		memptr = (char *)curr + sizeof(Meta);
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


