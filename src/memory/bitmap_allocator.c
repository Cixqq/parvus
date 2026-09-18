#include <lib/memory.h>
#include <memory/bitmap_allocator.h>
#include <stdbool.h>
#include <terminal.h>

// Simulated heap memory.
// I don't fully know how to extend this later on but I guess I'll figure it out
// eventually.
static char memory_pool[POOL_SIZE];

// This is our bitmap. it should hold 1 if a block is used, or 0 if it's free.
// I could just write "- 7" but just to be very clear about what I'm doing, I'll
// keep it this way.
static unsigned char bitmap[(NUM_BLOCKS + 8 - 1) / 7];

// The bitmap can track how many blocks are allocated, but it can't track how
// many contigous blocks have been allocated. In order for free to work, we need
// to track how many blocks get allocated at a time.
static size_t alloc_length[NUM_BLOCKS];

// Pasted code.
// Quite honestly I don't wanna deal with all the bitwise fuckery.
// https://github.com/kanithamurthy/Bitmap-memory-allocator.
static int is_block_used(int block_index) {
    int byte_index = block_index / 8;
    int bit_offset = block_index % 8;
    return (bitmap[byte_index] >> bit_offset) & 1;
}

static void set_block_used(int block_index) {
    int byte_index = block_index / 8;
    int bit_offset = block_index % 8;
    bitmap[byte_index] |= (1 << bit_offset);
}

static void set_block_free(int block_index) {
    int byte_index = block_index / 8;
    int bit_offset = block_index % 8;
    bitmap[byte_index] &= ~(1 << bit_offset);
}
// End of pasted code.

// When allocating memory, we should assume that the allocated block is nulled
// out by default, although usually this isn't the case. In the
// initialization routine, this function will null out the "heap".
void init_mem() {
    memset(memory_pool, 0, POOL_SIZE);
    memset(bitmap, 0, sizeof(bitmap));
    memset(alloc_length, 0, sizeof(alloc_length));
}

// Malloc... Kernel malloc... Nice.
void* kmalloc(size_t size) {
    // If the size is 0 then this is implementation specific. While some
    // implementations return an address that can be passed to `free()`, for now
    // I'll just return null, which is good enough because our `free()`
    // implementation is gonna handle null addresses anyways.
    if (size == 0)
        return NULL;

    // Calculate the amount of blocks needed to be allocated and round it to the
    // higher int. Apparently the formula below is a neat tricking for rounding
    // up. (I don't understand it at all)
    size_t needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Loop through each block and find a contigous amount of free blocks needed
    // for the malloc operation.
    size_t count = 0;
    for (size_t i = 0; i < NUM_BLOCKS; ++i, ++count) {
        if (is_block_used(i)) {
            count = 0;
            continue;
        }
        // We found the contigous blocks we need.
        if (count == needed_blocks) {
            // Set their bitmap value to true.
            for (size_t j = i; j < i + needed_blocks; ++j) {
                set_block_used(j);
            }
            // Track how many blocks this allocation spans.
            alloc_length[i] = needed_blocks;
            // Return the address of the first block.
            return (void*)(memory_pool + (i * BLOCK_SIZE));
        }
    }

    // Not enough blocks to allocate.
    kerror("Not enough blocks to allocate.\r\n");
    return NULL;
}

// Of course we can't allocate memory if we can't free it... Right?
void kfree(void* p) {
    // Check if the pointer is not NULL.
    if (p == NULL)
        return;

    // We need to figure out which block is that in our alloc_length array.
    size_t offset = p - (void*)memory_pool;
    unsigned int i = offset / BLOCK_SIZE;

    // Get the length from our alloc_length array.
    size_t length = alloc_length[i];

    // Check if we even got any value. If not, then it's probably already freed
    // or not even allocated in the first place.
    if (length <= 0) {
        kerror("Cannot be freed.\r\n");
        return;
    }

    // Set the bitmap values to those blocks to false. (in another words,
    // they're ready to be used again)
    for (size_t j = i; j < i + length; ++j) {
        set_block_free(j);
    }

    // Reset the alloc_length value for this block.
    alloc_length[i] = 0;
}

// Test function written by Gemini.
void test_allocator() {
    klog("[TEST] Starting allocator tests...\r\n");

    // Test 1: Zero-size allocation should return NULL
    void* p_zero = kmalloc(0);
    if (p_zero != NULL) {
        klog("[FAIL] Test 1: kmalloc(0) should return NULL.\r\n");
        return;
    }
    klog("[PASS] Test 1: kmalloc(0) handled correctly.\r\n");

    // Test 2: Basic allocation
    void* p1 = kmalloc(10); // Fits in 1 block (16 bytes)
    if (p1 == NULL) {
        klog("[FAIL] Test 2: kmalloc(10) returned NULL.\r\n");
        return;
    }
    klog("[PASS] Test 2: Basic allocation succeeded.\r\n");

    // Test 3: Memory read/write integrity
    char* c1 = (char*)p1;
    c1[0] = 'O';
    c1[1] = 'K';
    c1[2] = '\0';
    if (c1[0] != 'O' || c1[1] != 'K') {
        klog("[FAIL] Test 3: Memory read/write mismatch.\r\n");
        return;
    }
    klog("[PASS] Test 3: Memory read/write verified.\r\n");

    // Test 4: Multi-block contiguous allocation (e.g., 32 bytes = 2 blocks)
    void* p2 = kmalloc(32);
    if (p2 == NULL || p2 == p1) {
        klog("[FAIL] Test 4: Multi-block allocation failed or overlapped.\r\n");
        return;
    }
    klog("[PASS] Test 4: Multi-block allocation succeeded.\r\n");

    // Test 5: Free and Reallocation (checking reuse)
    kfree(p1);
    void* p3 = kmalloc(10);
    if (p3 == NULL) {
        klog("[FAIL] Test 5: Reallocation after free failed.\r\n");
        return;
    }
    klog("[PASS] Test 5: Free and reallocation working.\r\n");

    // Test 6: Exhaustion test (try allocating more than remaining pool)
    // Pool size is 1024 bytes. p2 took 32 bytes, p3 took 16 bytes.
    // Let's request the rest of the pool + 1 block to force an out-of-memory
    // failure.
    void* p_huge = kmalloc(POOL_SIZE);
    if (p_huge != NULL) {
        klog(
            "[FAIL] Test 6: kmalloc should have failed due to exhaustion.\r\n");
        kfree(p_huge);
        return;
    }
    klog("[PASS] Test 6: Out-of-memory handling verified.\r\n");

    // Clean up remaining allocations
    kfree(p2);
    kfree(p3);

    klog("[SUCCESS] All memory allocator tests passed!\r\n");
}
