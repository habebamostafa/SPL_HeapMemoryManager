#ifndef HMM_H
#define HMM_H
#include <stddef.h>

void *HmmAlloc (size_t size);
void HmmFree (void *ptr);
void HmmPrintStats (void);
void HmmReset(void);

#endif
