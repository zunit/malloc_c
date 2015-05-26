#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mymemory.h"
#include <pthread.h>

// ---------------------- function prototype ---------------------------
// this find if we can get a best space
header* findBestSpace(header* begSpace, unsigned int intSpace);

// this split and ensures we created a new header 
void splitSpace(header* theBestOne, unsigned int size);

// this is extending the HEAP
header* extendHeap();

// this is merging
void coalescing(header* begSpace);

// alignment 
unsigned int alignBit(int size);

// -------------------------- LOCK -------------------------------------
pthread_mutex_t lock;

// ------------------------ MACROS -------------------------------------

#define PAGE 4096 // This is a page which we can use for initMalloc
// OPTIMIZE VERSION this is align
#define align(x) ((((x-1)>>3)<<3)+8) //make sure we align the right 8 bits
 
 
// ---------------------------------------------------------------------
/* mymalloc_init: initialize any data structures that your malloc needs in
                  order to keep track of allocated and free blocks of 
                  memory.  Get an initial chunk of memory for the heap from
                  the OS using sbrk() and mark it as free so that it can be
                  used in future calls to mymalloc()
*/

static header* heapSpace; // this keeps track head of the list
static header* tailSpace; //

// TODO - add semophore
// MINVALUE assumptions think about this
int mymalloc_init() {

    // placeholder so that the program will compile
    
    
    heapSpace = extendHeap(); 
    tailSpace = heapSpace;
    
    printf("heap starts at %lld \n", (unsigned long long) heapSpace);
    // QUESTION: (bellow) is this indicating an error?
    return 0; //success *DOUBLE CHECK ON THIS
}

// testing function code
void print_heap(){
    
    header *curr = heapSpace;	
    while(curr){
        printf("ptr: %p | size: %d | next: %p | free: %d\n", curr, curr->size, curr->next, curr->free);
        curr= curr->next;
    }
    printf("-------------------------------------Done\n");
}

// QUESTION: how to use align
/* mymalloc: allocates memory on the heap of the requested size. The block
             of memory returned should always be padded so that it begins
             and ends on a word boundary.
     unsigned int size: the number of bytes to allocate.
     retval: a pointer to the block of memory allocated or NULL if the 
             memory could not be allocated. 
             (NOTE: the system also sets errno, but we are not the system, 
                    so you are not required to do so.)
*/
void *mymalloc(unsigned int size) {

	// placeholder so that the program will compile
    // check if the size is less than the space all the nodes in the list.
	header* firstSpace = heapSpace; // first 
	unsigned int alignedSize = align(size);
	
    pthread_mutex_lock(&lock);
	// have the bestSpace to use
	header* bestSpace = findBestSpace(firstSpace, alignedSize);
	// printf("bestSPACE: %p\n", bestSpace);
    
    if (bestSpace == NULL){

		bestSpace = extendHeap();
		tailSpace->next = bestSpace;
		bestSpace->previous = tailSpace;
		tailSpace = bestSpace;
		coalescing(bestSpace); // merge
	}
    //Splitting the space;
    
    if (bestSpace->size  > alignedSize + sizeof(header) + 8){ 
    	splitSpace(bestSpace, alignedSize);
    }
    //Set block to 0 before passing it to malloc
    bestSpace->free = 0;
    pthread_mutex_unlock(&lock);
    return ((void*) bestSpace + sizeof(header));
}
/* 
 * Input: header* Beginner, space Asked
 * Output: header* goodSpace
 * return the bestHeader that can take the allocation
 * */
header* findBestSpace(header* begSpace, unsigned int intSpace){
	
	header* curr;
	curr = begSpace; // let it go from the beginning
	header* bestSpace = NULL;
	
	/*
	 * as long as we are NOT at end of list AND
	 * the curr chunk is a free flag chunk
	 * */
	while (curr != NULL){
		// condition 0: make sure that the curr Block is FREE!
		if ((curr->free == 1) && (curr->size > intSpace)){

			// condition 1: bestSpace is NULL and curr can take it
			if (bestSpace == NULL){

				bestSpace = curr;
			} 
			// condition 2: bestSpace has something
			else{ 
				// condition 2a: bestSpace > curr
				if (bestSpace->size > curr->size){
					bestSpace = curr;
				}
			}
		}  
		// Move On it's not FREE!
		curr = curr->next;	
	} 
	
	if (bestSpace == NULL){
		printf("BEST SPACE NULL!");
	}

	return bestSpace;
}

// helper for spliting space
// Input: theBestOne, Size
// Output: newHeader at Right Location 
void splitSpace(header* bestSpace, unsigned int size){
	
	// finding location of where to point 
	// printf("starting to split\n");

	header* newSpace;
	newSpace = (header*)((char*) bestSpace + sizeof(header) + size); 
	
	
	// assigning new space struct
	newSpace->size = bestSpace->size - size - sizeof(header);
	newSpace->free = 1;
	newSpace->magic = 88888;
	newSpace->previous = bestSpace;

	// updating the post bestSpace;
	// condition: bestSpace is the last one
	if (bestSpace->next == NULL){
		newSpace->next = NULL;
		tailSpace = newSpace;	 
	} else {
		(bestSpace->next)->previous = newSpace;
		newSpace->next = bestSpace->next;
	}
	// updating the bestSpace
	bestSpace->size = size;
	bestSpace->magic = 88888;
	bestSpace->next = newSpace;

}

unsigned int alignBit(int size){
	// this is divisor
	int num = size/8;
	int total = num * 8;
	if (total < size){
		return (num+1)*8;
	}
	return total;
}

// helper function that extend heap.
header* extendHeap(){
	header* extend;
	 // initialize the first space using sbrk
    extend = sbrk(PAGE);
	if (extend == (void *) -1){
        printf("Initialize Space Error");
        return NULL; // non-zero return value indicates an error
    } else { // initialize space success with sbrk
        // this is the header of the first block of memeory
        extend->size = PAGE - sizeof(header);
        extend->magic = 88888;
        extend->free = 1;
        extend->previous=NULL;
        extend->next=NULL;
    }
	return extend;
}


void coalescing(header* begSpace){
	
	header* curr = begSpace;
	// condition where we reach 
	
	// if we just need to coalescing with previous
	if (curr->free == 1){
		header* adjacentPREV = curr->previous;
		header* adjacentNEXT = curr->next;
		// 1 *1 NULL ===> EXTENDHEAP
		if (adjacentNEXT == NULL){
			adjacentPREV->size = adjacentPREV->size + sizeof(header) + curr->size;
			adjacentPREV->next = NULL;
			tailSpace = adjacentPREV;		
		}
		
		// NULL 1 1
		else if ((adjacentPREV == NULL) && (adjacentNEXT->free == 1)){
			curr->size = curr->size + sizeof(header) + adjacentNEXT->size;
			
			// NULL 1 1 ~
			if (adjacentNEXT->next){
				(adjacentNEXT->next)->previous = curr;
				curr->next = adjacentNEXT->next;
			}
			// NULL 1 1 NULL 
			else {
				curr->next = adjacentNEXT->next;
				tailSpace = curr;
			}
			
			
		}
		// 1 *1 1 ~ && 1 *1 1 NULL ===> FREE
		else if ((adjacentPREV->free == 1) && (adjacentNEXT->free == 1)){
			// 1 *1 1 ~
			if (adjacentNEXT->next != NULL){
				int sumSize = sizeof(header) + curr->size + sizeof(header) + adjacentNEXT->size;
				adjacentPREV->size = adjacentPREV->size + sumSize;
				adjacentPREV->next = adjacentNEXT;
				adjacentNEXT->previous = adjacentPREV;
			} 
			else{ // 1 *1 1 NULL
				int sumSize = sizeof(header) + curr->size + sizeof(header) + adjacentNEXT->size;
				adjacentPREV->size = adjacentPREV->size + sumSize;
				adjacentPREV->next = NULL;
				tailSpace = adjacentPREV;
			}
		} 
		
		// 1 *1 0 ===> FREE
		// if we just need to coalescing with previous
		else if ((adjacentPREV->free == 1) && (adjacentNEXT->free == 0)){
			adjacentPREV->size = adjacentPREV->size + sizeof(header) + curr->size;
			adjacentNEXT->previous = adjacentPREV;
			adjacentPREV->next = adjacentNEXT;
		} 
		
		// 0 *1 1 ~ && 0 *1 1 NULL ===> FREE
		else if ((adjacentPREV->free == 0) && (adjacentNEXT->free == 1)){
			// 0 *1 1 ~
			if (adjacentNEXT->next != NULL){
				curr->size = curr->size + sizeof(header) + adjacentNEXT->size;
				curr->next = adjacentNEXT;
				adjacentNEXT->previous = curr;
			} 
			else 
			{ // 0 *1 1 NULL
				curr->size = curr->size + sizeof(header) + adjacentNEXT->size;
				tailSpace = curr;
				curr->next = NULL;
			}
		}	
	}
}

/* myfree: unallocates memory that has been allocated with mymalloc.
     void *ptr: pointer to the first byte of a block of memory allocated by 
                mymalloc.
     retval: 0 if the memory was successfully freed and 1 otherwise.
             (NOTE: the system version of free returns no error.)
*/
unsigned int myfree(void *ptr) {
	// placeholder so that the program will compile
	header* cursor = ptr - sizeof(header);
	header* curr = heapSpace;
		// looping through all my chunks
        pthread_mutex_lock(&lock);	
	while (curr){
		if (curr==cursor){
			curr->free = 1;
	        pthread_mutex_unlock(&lock);
	        coalescing(cursor); //merging	
	        // print_heap();	
			return 0;
		}
		curr = curr->next;
	}
	pthread_mutex_unlock(&lock);
	return 1;
}



