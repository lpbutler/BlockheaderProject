///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2023 Deb Deppeler based on work by Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission SPRING 2023, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "p3Heap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;

    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * Start Heap: 
     *  The blockHeader for the first block of the heap is after skip 4 bytes.
     *  This ensures alignment requirements can be met.
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE NAME or TYPE. 
 * It must point to the first block in the heap and is set by init_heap()
 * i.e., the block at the lowest address.
 */
blockHeader *heap_start = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int alloc_size;

/*
 * Additional global variables may be added as needed below
 * TODO: add global variables needed by your function
 */
 //blockHeader *curr = NULL;
 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if size < 1 
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 *   Return if NULL unable to find and allocate block for required size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* balloc(int size) {     
    //TODO: Your code goes in here.

    //check size is greater than 1

	if(size < 1){
		return NULL;
	}

	//set variable curr to initiate a heap block, only do this once
		
		blockHeader *curr = heap_start; //void is from byte to byte and blockheader is from header to header

	//check if size + blockHeader is multiple of 8 and create remainder as padding
	//TODO: if state
	int padding = 0;
	int x = (size+sizeof(blockHeader))%8;
	if(x != 0){
		padding = 8 - x;
	}
	//find block size

	int blockSize = size + sizeof(blockHeader) + padding;
	
	//initialize temp variables and other variables

	int tempSize = alloc_size;
	blockHeader *tempBlock = NULL;
	
	//start while loop to traverse thru all blocks that are not the end of heap block 

	while(curr->size_status != 1){
	int sizeStatus = curr->size_status;
	int block = (sizeStatus / 8) * 8; //divide the size_status by 8 to drop the last bits and multiply by 8 to get how many bytes it takes up
	int Fstatus = sizeStatus % 2; //get last most bit 

		//if already allocated, go to next block

		if(Fstatus != 0){
			curr = (blockHeader*)((void*)curr + block);
		}

		//else if the size of the current block is free and is same size of needed mem, set temp variables and exit while loop

		else if(block == blockSize){
		tempSize = blockSize;
		tempBlock = curr;
		break;
		}

		//rest of the cases that come to this point are free blocks that could be too big for heap block or can fit with split

		else{

			//if required block size is less than current block size, try splitting

			if(block > blockSize){

				int split = block - blockSize;

					//check if divisible by 8

					if(split % 8 == 0){

						//if divisible AND less than the tempSize (more efficient and less of a split), assign temp and temp address

						if(tempBlock == NULL || block < tempSize){
							tempSize = block;
							tempBlock = curr;
						}	
					}	
			}
			//go to next block
			curr = (blockHeader*)((void*)curr + block);
		}
	}

	//out of while loop, now have temp variables of temp blockHeader and size
	
	//if temp is null, no valid block was found
	if(tempBlock == NULL){
		return NULL; 
	}

	//start allocating free block
	int ss = tempBlock->size_status;
	int blockLen = (ss/8) * 8;
	int splt = blockLen - blockSize;
	if(splt == 0){
		tempBlock->size_status += 1; //show it is now allocated 

		//get next blockHeader to show that previous block is allocated (add block size to tempBlock and save to a blockHeader pointer and add 2)

		blockHeader *next  = (blockHeader*)((void*)tempBlock + blockSize);

		//make next block size status represent previous block as allocated by adding two
		if(next->size_status != 1){
			next->size_status = (next->size_status)+2;
		}
		//return tempBlock
		return (void*)tempBlock+sizeof(blockHeader);
	
	}
	else{
		//split a block into two
				
		//first, get prev block status of unsplit/original free block. ss is going to always be even, if it is a multiple of 8 this means it is free AND previous block is free
		
		int prev = ss % 8;
		
		//make new blockHeaders
		blockHeader *split1 = tempBlock;
		blockHeader *split2 = (blockHeader*)((void*)tempBlock + blockSize);
	
		//set each blockHeader to appropriate memory bytes

		split1->size_status = blockSize;
		split2->size_status = splt;

		//add two if not a multiple of 8 - meaning the previous block is allocated
		
		if(prev != 0){
			split1->size_status = split1->size_status + 2;
		}
		//add one to show split block 1 is now allocated
		split1->size_status = split1->size_status + 1;
		 
		//add two to split block 2 because previous block is now allocated
			split2->size_status = split2->size_status + 2;
		//edt footer
		blockHeader *ftr = (blockHeader*)((void*)split2 + (splt-sizeof(blockHeader)));
		ftr->size_status = splt;
		//return split1
		return (void*)split1+sizeof(blockHeader);
	}

} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int bfree(void *ptr) {    
    //TODO: Your code goes in here.
    
	//if ptr is null
	if(ptr == NULL){
		return -1;
	}
	
	//if ptr is not a multiple of 8
	if((int)ptr % 8 != 0){
		return -1;
	}

	//TODO: if ptr is outside of heap space
	if(ptr < (void*)heap_start || ptr > (void*)heap_start + alloc_size){
		return -1;
	}

	//TODO: if ptr block is already freed
	blockHeader *block = ptr-sizeof(blockHeader);
	int stat = block->size_status % 2;
	if(stat == 0){
		return -1;
	}

	//subtract one to show status is free
	block->size_status = block->size_status - 1;

	//make footer
	int bSize = (block->size_status/8)*8;
	blockHeader *foot = (blockHeader*)((void*)block + (bSize-sizeof(blockHeader)));
	foot->size_status = bSize;
	//get next blockHeader to adjust p-bit 
	blockHeader *next = (blockHeader*)((void*)foot + sizeof(blockHeader));
	next->size_status = next->size_status - 2;
	return 0;	
}  

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for user-called coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    //TODO: Your code goes in here.
	//return 0 if there is no need to coalesce and any positive number if there was need to coalesce	
	int c = 0;
	blockHeader *prev = (blockHeader*)(void*)heap_start + sizeof(blockHeader);
	int s = (prev->size_status/8) * 8;
	blockHeader *curr = (blockHeader*)(void*)heap_start + sizeof(blockHeader) + s;
	
	while(curr->size_status != 1){
		if(curr->size_status % 8 == 0 && prev->size_status % 8 == 0){
			int next = curr->size_status;			
			blockHeader *foot = (blockHeader*)(void*)(((prev->size_status + curr->size_status)/8)*8)-1;
			foot->size_status = ((prev->size_status + curr->size_status)/8)*8;
			prev->size_status += curr->size_status;
			
			curr = (blockHeader*)(void*) next;
			

			c = 1;
		}
		else{
			prev = curr;
			curr = (void*)heap_start + sizeof(blockHeader) + ((curr->size_status/8)*8);
		}
	}
	return c;
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int   pagesize; // page size
    int   padsize;  // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int   fd;

    blockHeader* end_mark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize from O.S. 
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heap_start = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    end_mark = (blockHeader*)((void*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Set size in header
    heap_start->size_status = alloc_size;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heap_start->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
} 
                  
/* 
 * Function can be used for DEBUGGING to help you visualize your heap structure.
 * Traverses heap blocks and prints info about each block found.
 * 
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     
 
    int    counter;
    char   status[6];
    char   p_status[6];
    char * t_begin = NULL;
    char * t_end   = NULL;
    int    t_size;

    blockHeader *current = heap_start;
    counter = 1;

    int used_size =  0;
    int free_size =  0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** HEAP: Block List ****************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 


// end p3Heap.c (Spring 2023)                                         


