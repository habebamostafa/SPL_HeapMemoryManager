#include "hmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_ALLOCS 10000
#define MIN_SIZE 16
#define MAX_SIZE 8192

typedef struct {
    void *ptr;
    size_t size;
    int is_allocated;
} alloc_info_t;

static size_t random_size(void) {
    return MIN_SIZE + (rand() % (MAX_SIZE - MIN_SIZE + 1));
}

static int verify_memory(alloc_info_t *info) {
    if (!info->ptr || !info->is_allocated) {
        return 1;
    }
    
    unsigned char *mem = (unsigned char *)info->ptr;
    unsigned char pattern = (unsigned char)(info->size & 0xFF);
    
    for (size_t i = 0; i < info->size; i++) {
        if (mem[i] != pattern) {
            fprintf(stderr, "Memory corruption detected at offset %zu\n", i);
            return 0;
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]) {
    alloc_info_t allocs[MAX_ALLOCS];
    int num_iterations = 5000;
    int current_allocs = 0;
    int total_allocs = 0;
    int total_frees = 0;
    int errors = 0;
    
    printf("  HMM Test: Random Allocation/Free\n");
    if (argc > 1) {
        num_iterations = atoi(argv[1]);
    }
    
    printf("Configuration:\n");
    printf("  Number of iterations: %d\n", num_iterations);
    printf("  Size range:          %d to %d bytes\n", MIN_SIZE, MAX_SIZE);
    printf("  Max concurrent:      %d allocations\n\n", MAX_ALLOCS);
    
    srand(time(NULL));
    memset(allocs, 0, sizeof(allocs));
    printf("Initial state:\n");
    HmmPrintStats();
    printf("\nRunning random allocation/free test...\n");
    for (int i = 0; i < num_iterations; i++) {
        int action = rand() % 100;
        if (action < 70 || current_allocs == 0) {
            if (current_allocs < MAX_ALLOCS) {
                size_t size = random_size();
                void *ptr = HmmAlloc(size);
                
                if (ptr == NULL) {
                    fprintf(stderr, "Allocation failed at iteration %d (size: %zu)\n", 
                            i, size);
                    errors++;
                    continue;
                }
                int slot = -1;
                for (int j = 0; j < MAX_ALLOCS; j++) {
                    if (!allocs[j].is_allocated) {
                        slot = j;
                        break;
                    }
                }
                if (slot == -1) {
                    fprintf(stderr, "No free slot available\n");
                    HmmFree(ptr);
                    continue;
                }
                allocs[slot].ptr = ptr;
                allocs[slot].size = size;
                allocs[slot].is_allocated = 1;
                memset(ptr, (unsigned char)(size & 0xFF), size);
                current_allocs++;
                total_allocs++;
                
                if (total_allocs % 1000 == 0) {
                    printf("  Progress: %d allocations, %d active\n", 
                           total_allocs, current_allocs);
                }
            }
        } else {
            if (current_allocs > 0) {
                int attempts = 0;
                int slot;
                do {
                    slot = rand() % MAX_ALLOCS;
                    attempts++;
                } while (!allocs[slot].is_allocated && attempts < MAX_ALLOCS);
                if (allocs[slot].is_allocated) {
                    if (!verify_memory(&allocs[slot])) {
                        errors++;
                    }
                    
                    HmmFree(allocs[slot].ptr);
                    allocs[slot].is_allocated = 0;
                    current_allocs--;
                    total_frees++;
                }
            }
        }
    }
    
    printf("\nVerifying remaining allocations...\n");
    int verified = 0;
    for (int i = 0; i < MAX_ALLOCS; i++) {
        if (allocs[i].is_allocated) {
            if (!verify_memory(&allocs[i])) {
                errors++;
            }
            verified++;
        }
    }
    printf("  Verified %d remaining allocations\n", verified);
    
    printf("\nFreeing all remaining allocations...\n");
    for (int i = 0; i < MAX_ALLOCS; i++) {
        if (allocs[i].is_allocated) {
            HmmFree(allocs[i].ptr);
            total_frees++;
        }
    }
    
    printf("\nTest Results:\n");
    printf("  Total allocations:    %d\n", total_allocs);
    printf("  Total frees:          %d\n", total_frees);
    printf("  Errors detected:      %d\n", errors);
    printf("\nFinal heap state:\n");
    HmmPrintStats();
    
    if (errors == 0) {
        printf("  Test completed successfully!\n");
        return 0;
    } else {
        printf(" Test completed with %d errors\n", errors);
        return 1;
    }
}
