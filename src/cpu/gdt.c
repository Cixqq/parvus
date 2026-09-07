#include "gdt.h"
#include "terminal.h"

// Initializing the GDT.
// Still not entirely sure why I gotta do this.
void init_gdt() {
    // Shoutout to @pitust.
    static uint64_t gdt[] = {
        0x0000000000000000, 0x00af9b000000ffff, 0x00af93000000ffff,
        0x00affb000000ffff, 0x00aff3000000ffff,
    };

    // "The size of the table in bytes subtracted by 1. This subtraction occurs
    // because the maximum value of Size is 65535, while the GDT can be up to
    // 65536 bytes in length (8192 entries). Further, no."
    // - https://wiki.osdev.org/Global_Descriptor_Table.
    gdtr_t gdtr = {.base = (uint64_t)&gdt, .limit = sizeof(gdt) - 1};

    // Store the GDT info to GDTR via the LGDT instruction.
    // Refer to Intel's SDM Vol. 2A 3-599.
    __asm__ volatile("LGDT %0" : : "m"(gdtr));

    // Reload CS via far return and update data segments.
    // Pasted straight out of the osdev wiki.
    reload_segments();

    klog("GDT setup complete.\r\n");
}
