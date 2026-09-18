#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H
#include <stddef.h>
// Each pool shall be 1 kilobyte in size.
#define POOL_SIZE 1024
// Each block of this pool shall be 16 bytes. That's 64 blocks in total.
#define BLOCK_SIZE 16
#define NUM_BLOCKS (POOL_SIZE / BLOCK_SIZE)

void init_mem();
void* kmalloc(size_t size);
void kfree(void* p);
void test_allocator();

#endif // _ALLOCATOR_H
