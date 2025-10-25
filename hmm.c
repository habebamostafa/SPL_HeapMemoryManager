#include "hmm.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define HEAP_SIZE (100 * 1024 * 1024) // 100MB heap
#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

typedef struct block_header {
    size_t size;                   
    int is_free;                  
    struct block_header *next;      
    struct block_header *prev;     
    unsigned int magic;             
} block_header_t;

#define MAGIC_NUMBER 0xDEADBEEF
#define HEADER_SIZE ALIGN(sizeof(block_header_t))
#define MIN_ALLOC_SIZE 4096

static char heap[HEAP_SIZE];              
static void *program_break = heap;              
static block_header_t *free_list_head = NULL;
static size_t total_allocated = 0;
static size_t total_freed = 0;
static size_t num_allocs = 0;
static size_t num_frees = 0;
static size_t program_break_increments = 0;

static void *sbrk_simulate(intptr_t increment) {
    void *old_break = program_break;
    void *new_break = (char *)program_break + increment;
    if (new_break < (void *)heap || new_break > (void *)(heap + HEAP_SIZE)) {
        return (void *)-1;
    }
    program_break = new_break;
    if (increment > 0) {
        program_break_increments++;
    }
    return old_break;
}

static block_header_t *find_free_block(size_t size) {
    block_header_t *current = free_list_head;
    while (current != NULL) {
        assert(current->magic == MAGIC_NUMBER);
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void remove_from_free_list(block_header_t *block) {
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        free_list_head = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    block->next = NULL;
    block->prev = NULL;
}

static void add_to_free_list(block_header_t *block) {
    block->next = free_list_head;
    block->prev = NULL;
    if (free_list_head) {
        free_list_head->prev = block;
    }    
    free_list_head = block;
}

static void split_block(block_header_t *block, size_t size) {
    if (block->size >= size + HEADER_SIZE + ALIGNMENT) {
        block_header_t *new_block = (block_header_t *)((char *)block + HEADER_SIZE + size);
        new_block->size = block->size - size - HEADER_SIZE;
        new_block->is_free = 1;
        new_block->magic = MAGIC_NUMBER;
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        block->size = size;
    }
}

static void coalesce(block_header_t *block) {
    if (block->next && block->next->is_free) {
        char *block_end = (char *)block + HEADER_SIZE + block->size;
        if (block_end == (char *)block->next) {
            block->size += HEADER_SIZE + block->next->size;
            remove_from_free_list(block->next);
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
            }
        }
    }
    if (block->prev && block->prev->is_free) {
        char *prev_end = (char *)block->prev + HEADER_SIZE + block->prev->size;
        if (prev_end == (char *)block) {
            block->prev->size += HEADER_SIZE + block->size;
            remove_from_free_list(block);
            block->prev->next = block->next;
            if (block->next) {
                block->next->prev = block->prev;
            }
            block = block->prev;
        }
    }
}

void *HmmAlloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = ALIGN(size);
    block_header_t *block = find_free_block(size);

    if (block) {
        remove_from_free_list(block);
        split_block(block, size);
        block->is_free = 0;
    } else {
        size_t request_size = (size + HEADER_SIZE > MIN_ALLOC_SIZE) ? (size + HEADER_SIZE) : MIN_ALLOC_SIZE;
        void *new_mem = sbrk_simulate(request_size);
        if (new_mem == (void *)-1) {
            fprintf(stderr, "HmmAlloc: Out of memory\n");
            return NULL;
        }

        block = (block_header_t *)new_mem;
        block->size = request_size - HEADER_SIZE;
        block->is_free = 0;
        block->next = NULL;
        block->prev = NULL;
        block->magic = MAGIC_NUMBER;
        split_block(block, size);
        if (block->next && block->next->is_free) {
            add_to_free_list(block->next);
        }
    }
    total_allocated += size;
    num_allocs++;
    return (void *)((char *)block + HEADER_SIZE);
}

void HmmFree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    block_header_t *block = (block_header_t *)((char *)ptr - HEADER_SIZE);
    if (block->magic != MAGIC_NUMBER) {
        fprintf(stderr, "HmmFree: Invalid pointer or corrupted memory\n");
        return;
    }
    if (block->is_free) {
        fprintf(stderr, "HmmFree: Double free detected\n");
        return;
    }
    block->is_free = 1;
    total_freed += block->size;
    num_frees++;
    add_to_free_list(block);
    coalesce(block);
}

void HmmPrintStats(void) {
    size_t heap_used = (char *)program_break - heap;
    printf("\n========== Heap Memory Manager Statistics ==========\n");
    printf("Total allocations:        %zu\n", num_allocs);
    printf("Total frees:              %zu\n", num_frees);
    printf("Total allocated bytes:    %zu\n", total_allocated);
    printf("Total freed bytes:        %zu\n", total_freed);
    printf("Current heap usage:       %zu bytes (%.2f MB)\n",
           heap_used, heap_used / (1024.0 * 1024.0));
    printf("Program break increments: %zu\n", program_break_increments);
    printf("-----------------------------------------------------\n\n");
}

void HmmReset(void) {
    program_break = heap;
    free_list_head = NULL;
    total_allocated = 0;
    total_freed = 0;
    num_allocs = 0;
    num_frees = 0;
    program_break_increments = 0;
    memset(heap, 0, HEAP_SIZE);
}
