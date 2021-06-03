#include <stdio.h>
#include "sfmm.h"
#include "extra.h"

int main(int argc, char const *argv[]) {
    // double* ptr = sf_malloc(sizeof(double));
    // *ptr = 320320320e-320;
    // printf("%f\n", *ptr);
    // sf_free(ptr);

    // Testing series of mallocs and frees with coalescing
    sf_block *arr[7];
    arr[0] = sf_malloc(8);
    arr[1] = sf_malloc(50);
    arr[2] = sf_malloc(110);
    arr[3] = sf_malloc(225);
    arr[4] = sf_malloc(500);
    arr[5] = sf_malloc(1000);
    arr[6] = sf_malloc(2000);
    sf_free(arr[3]);
    sf_free(arr[5]);
    sf_free(arr[1]);
    sf_show_heap();
    sf_free(arr[0]);
    // sf_show_heap();
    sf_free(arr[2]);
    // sf_show_heap();
	sf_free(arr[4]);
    // sf_show_heap();
    sf_free(arr[6]);
    sf_show_heap();

	// Testing reallocing larger block size
	// void *x = sf_malloc(8);
	// sf_malloc(8);
	// x = sf_realloc(x, 8*20);

	// Testing memalign with offset
	// sf_malloc(40);
	// void *x = sf_memalign(50, 256);
	// printf("%d\n", ((int)(long int)x-(int)(long int)sf_mem_start())%256);
	// sf_show_heap();

	// Testing memalign with incorrect alignment
	// void* x = sf_memalign(50, 48);
	// printf("%d %d\n", (int)(long int)x, sf_errno);

	// Testing memalign without offset
	// sf_malloc(456);
	// void *x = sf_memalign(100, 512);
	// printf("%d\n", ((int)(long int)x-(int)(long int)sf_mem_start())%512);
	// sf_show_heap();

	// Malloc four pages
    // void *x = sf_malloc(32704);
    // printf("%d\n", *(int*)x);
    // sf_show_heap();

    return EXIT_SUCCESS;
}
