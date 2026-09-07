#ifndef _GDT_H
#define _GDT_H
#include <stdint.h>

// Defining the object that GDTR accepts.
// Refer to Intel's SDM Vol. 2A 3-599.
typedef struct {
    uint16_t limit; // The size of the GDT in bytes.
    uint64_t base;  // The base address of the GDT object.
} __attribute__((packed)) gdtr_t;

extern void reload_segments(void);
void init_gdt();
#endif // _GDT_H
