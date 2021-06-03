/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"
#include "extra.h"

void *updateWilderness() {
	char *newPage = sf_mem_grow();
	// If no memory is available, return NULL
	if(newPage == NULL) {
		return NULL;
	}
	// If the wilderness block is nonexistent, set the
    // new page to be the new wilderness block
	else if(NEXT(sf_free_list_heads+7) == sf_free_list_heads+7) {
		// Start at the old epilogue
		newPage -= 8;
		// Initialize the free block
		int size = (long int)sf_mem_end()-8-(long int)newPage;
		// Sets the header and footer of the free block to size
		*((sf_footer*)(newPage+size-8)) = HEADER(newPage) = size | PREV_BLOCK_ALLOCATED;
		// Insert the free block as the wilderness block
		sf_block *wildernessList = sf_free_list_heads+7;
		NEXT(newPage) = PREV(newPage) = wildernessList;
		NEXT(wildernessList) = PREV(wildernessList) = ((sf_block*)newPage);
		// Epilogue
		newPage = sf_mem_end()-8;
		HEADER(newPage) = 0 | THIS_BLOCK_ALLOCATED;
		return NEXT(sf_free_list_heads+7);
	}
	// Otherwise, coalesce the current wilderness block with the
    // new page.
    else {
    	char *currWilderness = (void*)NEXT(sf_free_list_heads+7);
    	// Initialize the free block
		int size = (long int)sf_mem_end()-8-(long int)currWilderness;
		// Sets the header and footer of the new wilderness
		*((sf_footer*)(currWilderness+size-8)) = HEADER(currWilderness) = size | PREV_BLOCK_ALLOCATED;
		// Epilogue
		newPage = sf_mem_end()-8;
		HEADER(newPage) = 0 | THIS_BLOCK_ALLOCATED;
		return NEXT(sf_free_list_heads+7);
    }
}

/*
 * Returns the index of the free list that is appropriate for
 * the given size of the block
 */
int listIndFromSize(int size) {
	int temp = 32;
	int i = 0;
	while(size > temp && i < 6) {
		i++;
		temp *= 2;
	}
	return i;
}

// Called by sf_free
void *coalesce(void *pp1, void *pp2) {
	// Returns NULL if any of the blocks are NULL
	if(pp1 == NULL || pp2 == NULL) {
		return NULL;
	}
	char *p1 = pp1;
	char *p2 = pp2;
	// Returns NULL if the second block does not come right after the
 	// first block
	if(p1+SIZE(p1) != p2) {
		return NULL;
	}
	// Remove the first block from its free list if it was part
	// of a free list (could be that p1 is the block being freed,
	// so it wouldn't be part of a free list)
	if(!ALLOCATED(p1)) {
		NEXT(PREV(p1)) = NEXT(p1);
		PREV(NEXT(p1)) = PREV(p1);
	}
	// Coalesce the second block with the first one, returning
 	// the pointer to the new first block.
	size_t newSize = SIZE(p1) + SIZE(p2);
	// The previous block is either the prologue or another allocated block
	*((sf_footer*)(p1+newSize-8)) = HEADER(p1) = (newSize | PREV_BLOCK_ALLOCATED)+1;

	// Remove the second block from its free list if it was
	// part of a free list (could be that p2 is the block being freed,
	// so it wouldn't be part of a free list)
	if(!ALLOCATED(p2)) {
		NEXT(PREV(p2)) = NEXT(p2);
		PREV(NEXT(p2)) = PREV(p2);
	}

	return p1;
}

void *sf_malloc(size_t size) {
	if(size == 0) {
		return NULL;
	}
	// If this is the first malloc, allocate a new page of memory,
	// set up the prologue and epilogue, insert the free block as
	// the wilderness block
	if(sf_mem_start() == sf_mem_end()) {
		// Initialize sentinel nodes in the free list
		for(int i=0; i<NUM_FREE_LISTS; i++) {
			sf_block *currHead = sf_free_list_heads+i;
			NEXT(currHead) = PREV(currHead) = currHead;
		}
		char *pageStart = sf_mem_grow();
		// Prologue
		pageStart += 8;
		HEADER(pageStart) = 32 | THIS_BLOCK_ALLOCATED;
		pageStart += 32;
		// Initialize the free block
		int size = (long int)sf_mem_end()-8-(long int)pageStart;
		// Sets the header and footer of the free block to size
		*((sf_footer*)(pageStart+size-8)) = HEADER(pageStart) = size | PREV_BLOCK_ALLOCATED;
		// Insert the free block as the wilderness block
		sf_block *wildernessList = sf_free_list_heads+7;
		NEXT(pageStart) = PREV(pageStart) = wildernessList;
		NEXT(wildernessList) = PREV(wildernessList) = ((sf_block*)pageStart);
		// Epilogue
		pageStart = sf_mem_end()-8;
		HEADER(pageStart) = 0 | THIS_BLOCK_ALLOCATED;
	}

    // If size > 0, add 8 bytes (for header) and round up to the
    // closest multiple of 16 that is >= 32
	int totalSize = size+8;
	int temp = 32;
	while(temp < totalSize) {
		temp += 16;
	}
	totalSize = temp;


	// Calculate the index of the first free list that is large enough
	// for the calculated value
	int i = listIndFromSize(totalSize);

    // Search every free list at and after the free list at the calculated index
    // for the first block that is greater than or equal to the calculated value
    for(; i<NUM_FREE_LISTS; i++) {
    	// After the block is found, attempt to split it
    	// if it will not create any splinters, freeing the remainder block
    	// and returning the allocated block (All accomplished using realloc).
    	// Also, remove the allocated block from the free list.
    	sf_block *freeBlock = NEXT(sf_free_list_heads+i);
    	if(freeBlock != sf_free_list_heads+i && SIZE(freeBlock) >= totalSize) {
    		NEXT(PREV(freeBlock)) = NEXT(freeBlock);
    		PREV(NEXT(freeBlock)) = PREV(freeBlock);
    		HEADER(freeBlock) = HEADER(freeBlock) | THIS_BLOCK_ALLOCATED;
    		return sf_realloc((char*)freeBlock+8, size);
    	}
    }

    // If no available block is found, try to grow the wilderness block
    // and use that block for the requested allocation (remember to coalesce
    // the newly added page with the current wilderness block)
    sf_block *freeBlock = updateWilderness();
    while(freeBlock != NULL && SIZE(freeBlock) < totalSize) {
    	freeBlock = updateWilderness();
    }
    // If the heap cannot be grown, set sf_errno and return NULL
    // (sf_errno is set in sf_mem_grow())
    if(freeBlock == NULL) return NULL;
    // Attempt to split the wilderness block, allocate the free block
    // and update the free list
    // sf_show_free_list(7);
    NEXT(PREV(freeBlock)) = NEXT(freeBlock);
    PREV(NEXT(freeBlock)) = PREV(freeBlock);
    // sf_show_free_list(7);
    HEADER(freeBlock) = HEADER(freeBlock) | THIS_BLOCK_ALLOCATED;
    return sf_realloc((char*)freeBlock+8, size);
}

void sf_free(void *pp) {
	// Offset to get back to the header
	pp = (char*)pp-8;
	// Error handling (See Notes on sf_free in hw3_doc)
	if(pp == NULL) abort();
	else if(((long int)pp+8) % 16 != 0) abort();

	if(SIZE(pp) % 16 != 0 || SIZE(pp) < 32) abort();
	else if(!ALLOCATED(pp)) abort();
	else if((char*)pp+SIZE(pp) > (char*)sf_mem_end()-8) abort();
	else if(!THIS_PREV_ALLOCATED(pp) && PREV_ALLOCATED(pp)) abort();

	char* temp;
	int coalesced = 0;

	// Coalesce the block with any adjacent free blocks
	if(!THIS_PREV_ALLOCATED(pp)) {
		// Get to the header of the previous block
		char * prevBlock = ((char*)pp)-8;
		prevBlock -= SIZE(prevBlock)-8;
		temp = coalesce(prevBlock, pp);
		if(temp != NULL) pp = temp;
	}
	char *nextBlock = ((char*)pp+SIZE(pp));
	if(!ALLOCATED(nextBlock)) {
		coalesced = 1;
		temp = coalesce(pp, nextBlock);
		if(temp != NULL) pp = temp;
	}

	// Insert the newly freed block into the beginning of the
	// appropriate free list and updating the
	// PREV_ALLOC bit in the next block (inserting into wilderness block
	// if it is the last free block)
	if(((char*)pp)+SIZE(pp)+8 == sf_mem_end()) {
		HEADER(pp) = HEADER(pp)-1;
		*((sf_footer*)(((char*)pp)+SIZE(pp)-8)) = HEADER(pp);
		NEXT(sf_free_list_heads+7) = PREV(sf_free_list_heads+7) = pp;
		NEXT(pp) = PREV(pp) = sf_free_list_heads+7;
		char *epilogue = ((char*)pp)+SIZE(pp);
		if(THIS_PREV_ALLOCATED(epilogue)) HEADER(epilogue) = HEADER(epilogue)-2;
	}
	else {
		HEADER(pp) = HEADER(pp)-1;
		*((sf_footer*)(((char*)pp)+SIZE(pp)-8)) = HEADER(pp);
		int i = listIndFromSize(SIZE(pp));
		NEXT(pp) = NEXT(sf_free_list_heads+i);
		PREV(pp) = sf_free_list_heads+i;
		NEXT(sf_free_list_heads+i) = pp;
		// If pp is the only free block, set the previous block of the
		// sentinel node to pp as well
		if(PREV(sf_free_list_heads+i) == sf_free_list_heads+i)
			PREV(sf_free_list_heads+i) = pp;
		char *nextBlock = ((char*)pp)+SIZE(pp);
		// If no coalescing was performed with the block in front, update the
		// PREV_ALLOC bit of the next block
		if(!coalesced) {
			HEADER(nextBlock) = HEADER(nextBlock)-2;
		}
	}
}

void *sf_realloc(void *pp, size_t rsize) {
	// Offset to get back to the header
	pp = (char*)pp-8;
	// Same error handling as sf_free
	if(pp == NULL) {
		sf_errno = EINVAL;
		return NULL;
	}
	else if(((long int)pp+8) % 16 != 0) {
		sf_errno = EINVAL;
		return NULL;
	}

	if(SIZE(pp) % 16 != 0 || SIZE(pp) < 32) {
		sf_errno = EINVAL;
		return NULL;
	}
	else if(!ALLOCATED(pp)) {
		sf_errno = EINVAL;
		return NULL;
	}
	else if((char*)pp+SIZE(pp) > (char*)sf_mem_end()-8) {
		sf_errno = EINVAL;
		return NULL;
	}
	else if(!THIS_PREV_ALLOCATED(pp) && PREV_ALLOCATED(pp)) {
		sf_errno = EINVAL;
		return NULL;
	}

	// Free the block if rsize is 0
	if(rsize == 0) {
		sf_free((char*)pp+8);
		return NULL;
	}

	// If rsize > 0, add 8 bytes (for header) and round up to the
    // closest multiple of 16 that is >= 32
	int totalSize = rsize+8;
	int temp = 32;
	while(temp < totalSize) {
		temp += 16;
	}
	totalSize = temp;

	// Larger size:
	// sf_malloc a larger size block, memcpy contents of the payload
	// from pp to the larger block
	// Free the current block and return the larger block
	if(totalSize > SIZE(pp)) {
		char *largerBlock = sf_malloc(rsize);
		if(largerBlock == NULL) return NULL;
		memcpy(largerBlock, ((char*)pp)+8, SIZE(pp)-8);
		sf_free((char*)pp+8);
		return largerBlock;
	}

	// Smaller size:
	// Check if splitting the current block to a smaller size will cause a splinter
	// If so, or if the next size is equal to the old size,
	// make sure that the header is accurate and just return the current block as it is
	// Set the PREV_ALLOC bit in the next block as well
	// Else, split the block appropriately, making sure both headers of the two
	// blocks are accurate, free the remainder block and return the allocated block
	if(totalSize == SIZE(pp) || SIZE(pp)-totalSize < 32) {
		char* nextBlock = ((char*)pp)+SIZE(pp);
		HEADER(nextBlock) = HEADER(nextBlock) | PREV_BLOCK_ALLOCATED;
		// If the next block isn't the epilogue, set the footer as well
		if(nextBlock+8 != sf_mem_end()) {
			*((sf_footer*)(nextBlock+SIZE(nextBlock)-8)) = HEADER(nextBlock);
		}
		return ((char*)pp)+8;
	}
	// Saves the allocation status of the current and previous block
	int allocation = HEADER(pp)%4;
	int remainderSize = SIZE(pp)-totalSize;
	// Set the header of the current block to the totalSize
	HEADER(pp) = totalSize + allocation;
	// Sets the header and footer of the remainder block then frees it
	char *remainder = (char*)pp+totalSize;
	*((sf_footer*)(remainder+remainderSize-8)) = HEADER(remainder) = (remainderSize | PREV_BLOCK_ALLOCATED) | THIS_BLOCK_ALLOCATED;
	sf_free(remainder+8);
	return (char*)pp+8;
}

void *sf_memalign(size_t size, size_t align) {
	// align >= 32
	if(align < 32) {
		sf_errno = EINVAL;
		return NULL;
	}
	// align must be a power of 2
	int temp = 32;
	while(temp < align) temp *= 2;
	if(temp != align) {
		sf_errno = EINVAL;
		return NULL;
	}

	// Allocates a block of size + alignment size + minimum block size
	char *block = sf_malloc(size + align + 32);
	if(block == NULL) return NULL;
	int sizeOfBlock = SIZE((block-8));
	char *retBlock = block;

	// Travel to the first point in memory where this block's address satisfies the required alignment
	// while((int)(((long int)retBlock)-((long int)sf_mem_start())) % align != 0) {
	while((int)(((long int)retBlock)) % align != 0) {
		retBlock += 16;
	}
	int offset = (long int)retBlock-(long int)block;

	// If no offset was needed, try to split the block
	if(offset == 0) {
		return sf_realloc(retBlock, size);
	}
	// Update the header of the return and original block, and free the original block.
	else {
		int allocation = HEADER((block-8))%4;
		HEADER((block-8)) = offset+allocation;
		HEADER((retBlock-8)) = (sizeOfBlock-offset) | THIS_BLOCK_ALLOCATED | PREV_BLOCK_ALLOCATED;
		sf_free(block);
		// Split off any excess payload space and return the block (done using realloc)
    	return sf_realloc(retBlock, size);
	}
}