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
int numFreePagesMem = 1048;			// counter that tracks the number of free pages in physical memory
int numFreePagesSF = 4096;			// counter that tracks the number of free pages in swap file

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
	printf("Handler\n");
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
		page_table[x].entryNum = x;
		page_table[x].isOsRegion = 1;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
		page_table[x].next = NULL;
		page_table[x].prev = NULL;
	} 
	// Memory
	for (x = 1000; x < 2048; x++){
		page_table[x].entryNum = x;
		page_table[x].isOsRegion = 0;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
		page_table[x].next = NULL;
		page_table[x].prev = NULL;
	}
	// Swap file
	for (x = 2048; x < 6144; x++){
		page_table[x].entryNum = x;
		page_table[x].isOsRegion = 0;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
		page_table[x].next = NULL;
		page_table[x].prev = NULL;
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

// req is THREADREQ:
// returns void* pointer to 1st page of current thread
// if thread does not have any pages yet, find a free page in physical memory and swap it with the 1st page
// if no free pages in physical memory, find a free page in swap file and swap it with the 1st page
// otherwise, return NULL
// req is LIBRARYREQ:
// returns void* pointer to OS region
void* getHead(int req){
	void *the_head;

	
	int y;
	if (req == THREADREQ){
		// first see if thread has any pages
		for (y = 1000; y < 2048; y++){
			// find page current thread's page 1
			if ((page_table[y].tid == current_tid) && (page_table[y].page_num == 1)){

				// if current thread's page 1 is the 1st page
				if(y == 1000) {

					the_head = (void*)(all_memory + 1000 * PAGE_SIZE);
					return the_head;					
				}

				// if reaches here, current thread's page 1 is not the 1st page
				// swap current thread's page 1 with the 1st page

				// if 1st page has a prev page owned by that thread
				if (page_table[1000].prev != NULL) {
					// update the prev page's metadata to point to current thread's page 1 (which is where the data in the 1st page is being moved to)
					page_table[1000].prev->next = &page_table[y];
				}

				// if 1st page has a next page owned by that thread
				if (page_table[1000].next != NULL) {
					// update the next page's metadata to point to current thread's page 1 (which is where the data in the 1st page is being moved to)
					page_table[1000].next->prev = &page_table[y];
				}

				// if current thread's page 1 has a prev page owned by that thread
				if (page_table[y].prev != NULL) {
					// update the prev page's metadata to point to the 1st page (which is where the data in current thread's page 1 is being moved to)
					page_table[y].prev->next = &page_table[1000];
				}

				// if current thread's page 1 has a next page owned by that thread
				if (page_table[y].next != NULL) {
					// update the next page's metadata to point to the 1st page (which is where the data in current thread's page 1 is being moved to)
					page_table[y].next->prev = &page_table[1000];
				}

				// swap current thread's page 1 metadata with the 1st page metadata
				page_meta temp = page_table[1000];
				page_table[1000] = page_table[y];
				page_table[y] = temp;

				// swap current thread's page 1 data with the 1st page data
				memcpy( buffer_page, (void*)(all_memory + 1000 * PAGE_SIZE), PAGE_SIZE );
				memcpy( (void*)(all_memory + 1000 * PAGE_SIZE), (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE );
				memcpy( (void*)(all_memory + y * PAGE_SIZE), buffer_page, PAGE_SIZE );
				memset( buffer_page, 0, PAGE_SIZE );

				// set head to point to 1st page
				the_head = (void*)(all_memory + 1000 * PAGE_SIZE);
				return the_head;
			}
		}
		// if thread does not have any pages yet, see if there is a free page in physical memory
		for (y = 1000; y < 2048; y++){
			// find page that is free
			if (page_table[y].inUse == 0){

				// if free page is the 1st page
				if (y == 1000) {

					page_table[1000].prev = NULL;
					page_table[1000].next = NULL;
					page_table[1000].tid = current_tid;
					page_table[1000].inUse = 1;
					page_table[1000].page_num = 1;
					numFreePagesSF--;

					the_head = (void*)(all_memory + 1000 * PAGE_SIZE);
					initMem(req, the_head);
					return the_head;
				}

				// if reaches here, free page is not the 1st page
				// move data in 1st page to free page and update metadata, so 1st page can be given to current thread

				// if 1st page has a prev page owned by that thread
				if (page_table[1000].prev != NULL) {
					// update the prev page's metadata to point to free page (which is where the data in 1st page is being moved to)
					page_table[1000].prev->next = &page_table[y];
				}

				// if 1st page has a next page owned by that thread
				if (page_table[1000].next != NULL) {
					// update the next page's metadata to point to free page (which is where the data in 1st page is being moved to)
					page_table[1000].next->prev = &page_table[y];
				}

				// set free page metadata equal to 1st page metadata
				page_table[y] = page_table[1000];
				numFreePagesSF--;
				
				// set 1st page metadata
				page_table[1000].prev = NULL;
				page_table[1000].next = NULL;
				page_table[1000].tid = current_tid;
				page_table[1000].inUse = 1;
				page_table[1000].page_num = 1;
				
				// move 1st page data to free page, then zero out 1st page data
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				mprotect( (void*)(all_memory + 1000 * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				swapEmptyPage( (void*)(all_memory + y * PAGE_SIZE), (void*)(all_memory + 1000 * PAGE_SIZE) );
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_NONE );

				// set head to point to 1st page
				the_head = (void*)(all_memory + 1000 * PAGE_SIZE);
				initMem(req, the_head);
				return the_head;
			}
		}
		// if no free pages in physical memory, see if there is a free page in swap file
		for (y = 2048; y < 6144; y++) {
			// find page that is free
			if (page_table[y].inUse == 0) {
				
				// move data in 1st page to free page and update metadata, so 1st page can be given to current thread

				// if 1st page has a prev page owned by that thread
				if (page_table[1000].prev != NULL) {
					// update the prev page's metadata to point to free page (which is where the data in 1st page is being moved to)
					page_table[1000].prev->next = &page_table[y];
				}

				// if 1st page has a next page owned by that thread
				if (page_table[1000].next != NULL) {
					// update the next page's metadata to point to free page (which is where the data in 1st page is being moved to)
					page_table[1000].next->prev = &page_table[y];
				}

				// set free page metadata equal to 1st page metadata
				page_table[y] = page_table[1000];
				numFreePagesSF--;
				
				// set 1st page metadata
				page_table[1000].prev = NULL;
				page_table[1000].next = NULL;
				page_table[1000].tid = current_tid;
				page_table[1000].inUse = 1;
				page_table[1000].page_num = 1;
				
				// move 1st page data to free page, then zero out 1st page data
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				mprotect( (void*)(all_memory + 1000 * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				swapEmptyPage( (void*)(all_memory + y * PAGE_SIZE), (void*)(all_memory + 1000 * PAGE_SIZE) );
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_NONE );

				// set head to point to 1st page
				the_head = (void*)(all_memory + 1000 * PAGE_SIZE);
				initMem(req, the_head);
				return the_head;
			}
		}
	}
	else if (req == LIBRARYREQ){
		// set head to point to page 1 (1st byte of OS region)
		the_head = base_page;
		return the_head;
	}

	return NULL;
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
			numFreePagesMem--;
			page_table[y].tid = current_tid;
			mprotect_setter(current_tid, 0);
			new_page = (void*)(all_memory + y * PAGE_SIZE);
			return new_page; 
		}
	}

	// if reaches here, no free pages in physical memory
	// go look in swap file
	for (y = 2048; y < 6144; y++){
		// if found page that is free
		if (page_table[y].inUse == 0){

			page_table[y].inUse = 1;
			numFreePagesSF--;
			page_table[y].tid = current_tid;

			new_page = (void*)(all_memory + y * PAGE_SIZE);
			return new_page;
		}
	}

	// if reaches here, physical memory and swap file have no free pages
	return NULL;
}

//A thread requests a page too big for it's current pages to handle. This function assumes there is an empty page in memory to make that request possible but it needs to move the next page  to continue the illusion of contigious memory 
void swapEmptyPage(void *newPage, void *oldPage){



	// copy old page data to new page
	memcpy(newPage, oldPage, PAGE_SIZE);

	// zero out old page data
	memset(oldPage, 0, PAGE_SIZE);

	printf("Case 4\n");
}	

void swapPage(int sniped_tid, int sniped_page, void *evict){

	int y; 
	void *curr_page;

	for(y = 1000; y < 2048; y++){
		if ((page_table[y].tid == sniped_tid)) 
			break;
	}
	if (page_table[y].tid == sniped_tid){
		page_meta *tmp = &page_table[y];

		while(sniped_page != tmp->page_num){
		
			if(sniped_page >  tmp->page_num)
				tmp = tmp->next;
			else
				tmp = tmp->prev;
		}
		curr_page = all_memory + tmp->entryNum * PAGE_SIZE;
	}
	else{
		fprintf(stderr,"ERROR: Could not fulfill SWAP request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		exit(0);
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

// zeroes out buffer page
void freeBuffer(){

	//set all memory in buffer to 0 
	//pass the address of the buffer, target value, and how long it is 

	memset(buffer_page, 0, PAGE_SIZE);

	return;
}

// protect all pages belonging to previous thread
// unprotect all pages belonging to current thread
void mprotect_setter(int current_tid, int prev_tid){

	int i;
	for (i = 1000; i < 2048; i++) {
	
		if (page_table[i].tid == current_tid){
			mprotect( (void*)(all_memory + i * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
		}
		else {
			mprotect( (void*)(all_memory + i * PAGE_SIZE), PAGE_SIZE, PROT_NONE );
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
void initMem(int req, void * newHead){

	//raise(SIGSEGV);
	mprotect_setter(current_tid, 0);

	HEAD = (Meta *) newHead;
	

	if (req == THREADREQ){
		HEAD->size = PAGE_SIZE-sizeof(Meta);
	}

	else if (req == LIBRARYREQ){
		HEAD->size = OS_SIZE-sizeof(Meta);
	}

	HEAD->next = NULL;
	HEAD->prev = NULL;
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
	new->prev = curr;

	curr->is_free = 0;
	curr->size = size;
	curr->next = new;
}

void *myallocate(size_t size, int req){

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
	HEAD = (Meta *) getHead(req);

	// if thread does not own any pages any there are no free pages left in physical memory and swap file
	if (HEAD == NULL) {
		fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}
	
/*
	if (req == 1){
		HEAD = user_space;
	}
	else if (req == 2){
		HEAD = base_page;
	}
*/

	// if 1st page of current thread has no metadata, create metadata
	if (!(HEAD->size)){
		//initMem(req);
	}

	// curr points to 1st page of current thread
	curr = HEAD;

	// traverse through metadata of current thread
	while (curr != NULL && curr->next != NULL) {

		// if block is free and of sufficient size
		if (curr->is_free == 1 && curr->size >= size){
			break;
		}
			
		prev = curr;
		curr = curr->next;
	}

	// curr now points to either free block of sufficient size or the last block

	if (curr == NULL){
		fprintf(stderr,"ERROR: unexpected null curr. FILE: %s, LINE %d\n", __FILE__, __LINE__);
		return NULL;
	}

	// if curr points to free block of sufficient size, but not enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size == size){

		curr->is_free = 0;
		memptr = (char *)curr + sizeof(Meta);

		printf("Case 1\n");
		return memptr;
	}

	// if curr points to free block of sufficient size, with enough size to chunk off another metadata + free block
	if (curr->is_free == 1 && curr->size > size + sizeof(Meta)){

		organizeMem(curr, size);
		memptr = (char *)curr + sizeof(Meta);

		printf("Case 2\n");
		return memptr;
	}
	
	// insufficient memory checks below

	// if last block is not free
	if (curr->is_free == 0) {
		// if not enough memory left to allocate size + new metadata
		if(size + sizeof(Meta) > numFreePagesMem * PAGE_SIZE) {
			fprintf(stderr,"ERROR: insufficient memory. FILE: %s, LINE %d\n", __FILE__, __LINE__);
			return NULL;
		}
	}

	// if last block is not large enough for request
	if (curr->size < size) {
		// if not enough memory left to allocate size
		if(size > curr->size + numFreePagesMem * PAGE_SIZE) {
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
			printf("Case 3\n");

			// there are no free pages
			if (!freePage){
				fprintf(stderr,"ERROR: insufficient pages. FILE: %s, LINE %d\n", __FILE__, __LINE__);
				return NULL;
			}

			// the page number of the page that curr points within
			int currPageNum = 2048 - ( (all_memory + MEMORY_SIZE - (char *)curr) / PAGE_SIZE );
			// the page number of the page that freePage points at
			int freePageNum = 2048 - ( (all_memory + MEMORY_SIZE - (char *)freePage) / PAGE_SIZE );
			// the page number of the next needed page
			// if current thread needs 5 pages to satsify request: curr+1 is next needed page, then curr+2 is next needed page, ..., curr+5 is next needed page
			int nextPageNum = currPageNum + numPagesNeeded;
			void* nextPage = (void*)(all_memory + nextPageNum * PAGE_SIZE);
			// page counter of the page before next needed page
			int pageCounter = page_table[nextPageNum - 1].page_num;
			
			// if the free page is not the next needed page (directly after: the current page or the last needed page)
			if (freePageNum != nextPageNum){

				// move data in next needed page to free page and update metadata, so next needed page can be given to current thread

				// if next needed page has a prev page owned by that thread
				if (page_table[nextPageNum].prev != NULL) {
					// update the prev page's metadata to point to free page (which is where the data in next needed page is being moved to)
					page_table[nextPageNum].prev->next = &page_table[freePageNum];
				}

				// if next needed page has a next page owned by that thread
				if (page_table[nextPageNum].next != NULL) {
					// update the next page's metadata to point to free page (which is where the data in next needed page is being moved to)
					page_table[nextPageNum].next-> prev = &page_table[freePageNum];
				}

				// set free page metadata equal to next needed page metadata
				page_table[freePageNum] = page_table[nextPageNum];
				
				// set page before next needed page/next needed page metadata to point to each other
				page_table[nextPageNum - 1].next = &page_table[nextPageNum];
				page_table[nextPageNum].prev = &page_table[nextPageNum - 1];

				// set next needed page metadata
				page_table[nextPageNum].next = NULL;
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
				// set current/next page metadata to point to each other
				page_table[nextPageNum - 1].next = &page_table[nextPageNum];
				page_table[nextPageNum].prev = &page_table[nextPageNum - 1];

				// set next page metadata
				page_table[nextPageNum].next = NULL;
				page_table[nextPageNum].tid = current_tid;
				page_table[nextPageNum].inUse = 1;
				page_table[nextPageNum].page_num = pageCounter + 1;
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

// TODO: implement mydeallocate
// TODO: update numFreePagesMem counter as needed in mydeallocate
// TODO: update numFreePagesSF counter as needed in mydeallocate
void mydeallocate(void *ptr, int req){

	HEAD = getHead(req);
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


