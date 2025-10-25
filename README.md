# SPL_HeapMemoryManager
Objective:
Heap Memory Manager
Implement a heap memory manager to provide the dynamic memory allocation service to the user space programs. 
The user shall allocate the memory in bytes through {void *HmmAlloc(size_t size)}. The allocated memory can be freed through {void HmmFree(void *ptr)} with the same pointer returned from HmmAlloc().

In this phase, we are not going to use sbrk() to increase or shrink the heap. Instead, we are going to implement our HMM in user space without any kernel dependency for simplicity/debuggability (we will implement a fake heap manager that you are the only one who is using it for now). This means:
- Use a very large statically allocated array to simulate the heap area.
- Use a variable to simulate the program break in your HMM (Initially it will be pointing to the beginning of the static array)

Overview
This project implements a Heap Memory Manager (HMM) that provides dynamic memory allocation services similar to malloc() and free() in C. The implementation operates in user space without kernel dependencies by using a simulated heap.
Key Features:

- Dynamic memory allocation (HmmAlloc)
- Memory deallocation with automatic coalescing (HmmFree)
- Free list management with first-fit strategy
- Memory alignment (16-byte aligned)
- Minimal program break increments
- Memory corruption detection
- Double-free protection
- Comprehensive testing suite

all flow chart and explain in canva link : 
https://www.canva.com/design/DAG2x4XZuWA/dEGa2IZN24JtK54vbtT22A/edit?utm_content=DAG2x4XZuWA&utm_campaign=designshare&utm_medium=link2&utm_source=sharebutton 
