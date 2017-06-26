/****************************************************************/
// Created by Pranav Katkamwar, 
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

/****************************************************************/
/*
// SIGSEGV Handler
*/
/****************************************************************/

static void handler(int sig, siginfo_t *si, void *unused) {
	//printf("Handler\n");
	void *swapout;
	int sniped_tid, sniped_page, page_fault_page;

	page_fault_page = (si->si_addr - (void *) all_memory)/PAGE_SIZE;
	swapout = (char *)all_memory + (page_fault_page*PAGE_SIZE);

	sniped_tid = current_tid;
	sniped_page = page_fault_page - FIRST_USER_PAGE;
	//printf("Swapping: sniped_tid = %i, snipe_page = %i\n", sniped_tid, sniped_page);
	mprotect( (void*)(all_memory + page_fault_page ), PAGE_SIZE, PROT_READ | PROT_WRITE );
	swapPage(sniped_tid, sniped_page, swapout);
	return;
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
	user_space = (char *)all_memory + FIRST_USER_PAGE*PAGE_SIZE;
	swap_file = (char*)all_memory + FIRST_SWAP_PAGE*PAGE_SIZE;
	numFreePagesMem = 1048;			// counter that tracks the number of free pages in physical memory
	numFreePagesSF = 4096;
	int x;
	
	// Initialize Page Table
	// OS region
	for (x = 1; x < FIRST_USER_PAGE; x++){
		page_table[x].entryNum = x;
		page_table[x].isOsRegion = 1;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	}
	// Memory
	for (x = FIRST_USER_PAGE; x < FIRST_SWAP_PAGE; x++){
		page_table[x].entryNum = x;
		page_table[x].isOsRegion = 0;
		page_table[x].tid = -69;
		page_table[x].page_num = 0;
		page_table[x].inUse = 0;
	}
	// Swap file
	for (x = FIRST_SWAP_PAGE; x < 6144; x++){
		page_table[x].entryNum = x;
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


// returns ptr to a page that is free
// returns NULL if no pages are free
void* requestPage(){


	void* new_page;
	int y;

	for (y = FIRST_USER_PAGE; y < FIRST_SWAP_PAGE; y++){
		// if found page that is free
		if (page_table[y].inUse == 0){

			page_table[y].inUse = 1;
			numFreePagesMem--;
			page_table[y].tid = current_tid;
			mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
			new_page = (void*)(all_memory + y * PAGE_SIZE);
			return new_page; 
		}
	}

	// if reaches here, no free pages in physical memory
	// go look in swap file
	for (y = FIRST_SWAP_PAGE; y < 6144; y++){
		// if found page that is free
		if (page_table[y].inUse == 0){

			page_table[y].inUse = 1;
			numFreePagesSF--;
			page_table[y].tid = current_tid;
			mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
			new_page = (void*)(all_memory + y * PAGE_SIZE);
			return new_page;
		}
	}

	// if reaches here, physical memory and swap file have no free pages
	return NULL;
}

//A thread requests a page too big for it's current pages to handle. This function assumes there is an empty page in memory to make that request possible but it needs to move the next page  to continue the illusion of contigious memory 
void swapEmptyPage(void *newPage, void *oldPage){


	//printf("Swapping Old Page: %i with Empty Page: %i\n", (oldPage-(void*)all_memory)/PAGE_SIZE, (newPage-(void*)all_memory)/PAGE_SIZE);
	// copy old page data to new page
	memcpy(newPage, oldPage, PAGE_SIZE);

	// zero out old page data
	memset(oldPage, 0, PAGE_SIZE);

	//printf("Case 4\n");
}	

void swapPage(int sniped_tid, int sniped_page, void *evict){

	int y; 
	void *curr_page;

	for(y = FIRST_USER_PAGE; y < LAST_SWAP_PAGE; y++){
		if ((page_table[y].tid == sniped_tid) && (page_table[y].page_num == sniped_page))
		{
			curr_page = all_memory + y * PAGE_SIZE;
			mprotect( (void*)(curr_page), PAGE_SIZE, PROT_READ | PROT_WRITE );
			mprotect( (void*)(buffer_page), PAGE_SIZE, PROT_READ | PROT_WRITE );
			mprotect( (void*)(evict), PAGE_SIZE, PROT_READ | PROT_WRITE );
			int curr_page_num = (curr_page - (void *)all_memory)/PAGE_SIZE;
			int evict_page_num = (evict - (void *)all_memory)/PAGE_SIZE;
			page_meta temp = page_table[curr_page_num];
			page_table[curr_page_num] = page_table[y];
			page_table[curr_page_num].entryNum = curr_page_num;
			page_table[y] = temp;
			page_table[y].entryNum = y;


			//printf("\ncurr_page: %p\nevict_page: %p page_table[y].tid: %i page_table[y].page_num: %i\nsniped_page: %i\ny: %i\n", curr_page, evict, page_table[y].tid, sniped_tid, page_table[y].page_num, sniped_page, y);

			//printf("Swapping Curr_Page: %i '%s' ....... with evictPage : %i '%s'\n\n", y, (char *)(curr_page + sizeof(Meta)), (evict - (void *)all_memory)/PAGE_SIZE, (char *)(evict + sizeof(Meta))) ;
			// Copy Sniped Page to Buffer
			memcpy(buffer_page, curr_page, PAGE_SIZE);

			// Evict Page to Sniped Page
			memcpy(curr_page, evict, PAGE_SIZE);

			// Copy Buffered Page into Target

			memcpy(evict, buffer_page, PAGE_SIZE);

			//printf("evictPage : %i '%s' Curr_Page: %i '%s' \n\n", (evict - (void *)all_memory)/PAGE_SIZE, (char *)(evict + sizeof(Meta))), y, (char *)(curr_page + sizeof(Meta)) ;
			// Copy Sniped Page to Buffer
			mprotect( (void*)(curr_page), PAGE_SIZE, PROT_NONE );
			//printf("Protecting Curr_Page: %i\n", (curr_page - (void *)all_memory)/PAGE_SIZE);
			
			// Reset Buffer
			freeBuffer();
			return;
		}
	}


	fprintf(stderr,"ERROR: Could not fulfill SWAP request. FILE: %s, LINE %d\n", __FILE__, __LINE__);
	exit(0);
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

	//printf("\nmprotect_setter: \n current_tid: %i\n prev_tid: %i\n\n", current_tid, prev_tid);
	int i;
	for (i = FIRST_USER_PAGE; i < FIRST_SWAP_PAGE; i++) {
	
		if (page_table[i].tid == current_tid){
			if(mprotect( (void*)(all_memory + i * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE) == -1)
				printf("Could not unprotect: %s \n", strerror(errno));
		}
		if(page_table[i].tid == prev_tid){
			if(mprotect( (void*)(all_memory + i * PAGE_SIZE), PAGE_SIZE, PROT_NONE ) == -1)
				printf("Could not sucessfully protect: %s\n", strerror(errno));
			
			//printf("Protecting Page %i\n for Current Thread %i PrevThread: %i\n", i, current_tid, prev_tid);
		}
	}
}


/*void mprotect_setter_dead(int current_tid){
	int i;
	for(i = 0; i < (MEMORY_SIZE / PAGE_SIZE); i += 4096){
	
		if(page_table[i].tid ==  NULL)
			break;
	
		if(page_table[i].tid == current_tid)
			mprotect(&page_table[i] , 4096, PROT_READ | PROT_WRITE);	
		else
			mprotect(&page_table[i] , 4096, PROT_NONE);
	}
}*/


// if flag = 1, getHead called from myallocate
// if flag = 0, getHead called from mydeallocate
// req is THREADREQ:
// returns void* pointer to 1st page of current thread
// if thread does not have any pages yet, find a free page in physical memory and swap it with the 1st page
// if no free pages in physical memory, find a free page in swap file and swap it with the 1st page
// otherwise, return NULL
// req is LIBRARYREQ:
// returns void* pointer to OS region
void* getHead(int req, int flag){
	void *the_head;

	
	int y;
	if (req == THREADREQ){
		// first see if thread has any pages
		for (y = FIRST_USER_PAGE; y < FIRST_SWAP_PAGE; y++){
			// find page current thread's page 1
			if ((page_table[y].tid == current_tid) && (page_table[y].page_num == 0)){

				// if current thread's page 1 is the 1st page
				if(y == FIRST_USER_PAGE) {
					//printf("1st page is the page of thread: %i\n", current_tid);
					the_head = (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE);
					return the_head;					
				}

				// if reaches here, current thread's page 1 is not the 1st page
				// swap current thread's page 1 with the 1st page

				// swap current thread's page 1 metadata with the 1st page metadata
				page_meta temp = page_table[FIRST_USER_PAGE];
				page_table[FIRST_USER_PAGE] = page_table[y];
				page_table[FIRST_USER_PAGE].entryNum = FIRST_USER_PAGE;
				page_table[y] = temp;
				page_table[y].entryNum = y;

				//printf("PageTable[FIRST_USER_PAGE].tid %i pagetable[y].tid %i y: %i\n", page_table[FIRST_USER_PAGE].tid, page_table[y].tid, y);

				// swap current thread's page 1 data with the 1st page data
				mprotect( (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				memcpy( buffer_page, (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), PAGE_SIZE );
				//printf("---Line 271 in getHead---\n");	
				memcpy( (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE );
				//printf("---Line 273 in getHead---\n");	
				memcpy( (void*)(all_memory + y * PAGE_SIZE), buffer_page, PAGE_SIZE );
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_NONE );
				//printf("---Line 275 in getHead---\n");	
				memset( buffer_page, 0, PAGE_SIZE );
				// swap current thread's page 1 data with the 1st page data
				/*memcpy( buffer_page, (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), PAGE_SIZE );
				memcpy( (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE );
				memcpy( (void*)(all_memory + y * PAGE_SIZE), buffer_page, PAGE_SIZE );
				memset( buffer_page, 0, PAGE_SIZE );*/

				// set head to point to 1st page
				the_head = (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE);
				return the_head;
			}
		}

		// if getHead called from mydeallocate, return NULL since thread does not own any pages
		if (flag == 0) {
			return NULL;
		}

		// if thread does not have any pages yet, see if there is a free page in physical memory
		for (y = FIRST_USER_PAGE; y < FIRST_SWAP_PAGE; y++){
			// find page that is free
			if (page_table[y].inUse == 0){

				// if free page is the 1st page
				if (y == FIRST_USER_PAGE) {

					page_table[FIRST_USER_PAGE].tid = current_tid;
					//printf("Allocating page FIRST_USER_PAGE for Thread: %i\n", current_tid);
					page_table[FIRST_USER_PAGE].inUse = 1;
					page_table[FIRST_USER_PAGE].page_num = 0;
					numFreePagesMem--;

					the_head = (void*)(all_memory + (FIRST_USER_PAGE * PAGE_SIZE));
					initMem(req, the_head);
					return the_head;
				}

				// if reaches here, free page is not the 1st page
				// move data in 1st page to free page and update metadata, so 1st page can be given to current thread


				// set free page metadata equal to 1st page metadata
				page_table[y] = page_table[FIRST_USER_PAGE];
				page_table[y].entryNum = FIRST_USER_PAGE;
				numFreePagesMem--;
				
				// set 1st page metadata
				page_table[FIRST_USER_PAGE].tid = current_tid;
				page_table[FIRST_USER_PAGE].inUse = 1;
				page_table[FIRST_USER_PAGE].page_num = 0;
				
				// move 1st page data to free page, then zero out 1st page data
				//printf("\n\n---------------------Swapping free Page  %ito first page\n\n", y);
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				mprotect( (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				swapEmptyPage( (void*)(all_memory + y * PAGE_SIZE), (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE) );
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_NONE );

				// set head to point to 1st page
				the_head = (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE);
				initMem(req, the_head);
				return the_head;
			}
		}
		// if no free pages in physical memory, see if there is a free page in swap file
		for (y = FIRST_SWAP_PAGE; y < 6144; y++) {
			// find page that is free
			if (page_table[y].inUse == 0) {
				
				// move data in 1st page to free page and update metadata, so 1st page can be given to current thread

				// set free page metadata equal to 1st page metadata
				page_table[y] = page_table[FIRST_USER_PAGE];
				page_table[y].entryNum = FIRST_USER_PAGE;
				numFreePagesSF--;
				
				// set 1st page metadata
				page_table[FIRST_USER_PAGE].tid = current_tid;
				page_table[FIRST_USER_PAGE].inUse = 1;
				page_table[FIRST_USER_PAGE].page_num = 0;
				
				// move 1st page data to free page, then zero out 1st page data
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				mprotect( (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE), PAGE_SIZE, PROT_READ | PROT_WRITE );
				swapEmptyPage( (void*)(all_memory + y * PAGE_SIZE), (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE) );
				mprotect( (void*)(all_memory + y * PAGE_SIZE), PAGE_SIZE, PROT_NONE );

				// set head to point to 1st page
				the_head = (void*)(all_memory + FIRST_USER_PAGE * PAGE_SIZE);
				initMem(req, the_head);
				return the_head;
			}
		}
	}
	else if (req == LIBRARYREQ){
		// set head to point to page 1 (1st byte of OS region)
		the_head = base_page;
		if (firstOS == '0'){
			initMem(req, the_head);
			firstOS = '1';
		}
		return the_head;
	}

	return NULL;
}
