// Macros to get the header, next and previous block in a free block
#define NEXT(fb) ((sf_block*)fb)->body.links.next
#define PREV(fb) ((sf_block*)fb)->body.links.prev
// Macros to get the header and size of a block
#define HEADER(b) ((sf_block*)b)->header
#define SIZE(b) ((((sf_block*)b)->header)>>2<<2)
// Macros returning the allocation bits
#define ALLOCATED(b) (int)(HEADER(b)%2)
#define THIS_PREV_ALLOCATED(b) (int)((HEADER(b)>>1)%2)
#define PREV_ALLOCATED(b) (int)((*((sf_footer*)((char*)b-8)))%2)

 /*
  * Returns the index of the free list that is appropriate for
  * the given size of the block
  */
int listIndFromSize(int size);

/* Updates the wilderness block
 * If the wilderness block is nonexistent, set the
 * new page to be the new wilderness block
 *
 * Otherwise, coalesce the current wilderness block with the
 * new page.
 *
 * Return the wilderness block
 */
void *updateWilderness();

/*
 * Attempts to coalesce the first block with the second block
 *
 * Returns NULL if any of the blocks are NULL
 * Returns NULL if the second block does not come right after the
 * first block
 * Returns the first block if the second block is allocated
 * Returns the second block if the first block is allocated
 *
 * Else coalesce the second block with the first one, returning
 * the pointer to the new first block
 */
void *coalesce(void *pp1, void *pp2);