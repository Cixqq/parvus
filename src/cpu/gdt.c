#include "gdt.h"
#include "cpu/cpu.h"
#include "terminal.h"

// Way cleaner than to type all of that manually.
// For some reason, the compiler keeps bitching if I don't cast this to an int.
#define GDT_ENTRY_COUNT (sizeof(gdt_entries) / sizeof(gdt_entries[0]))

// Defining the GDT object. This object will later on get encoded into something
// the CPU can actually deal with.
typedef struct {
    uint32_t base, limit;       // The base and length of the GDT.
    uint8_t access_byte, flags; // Some other stuff.
} gdt_t;

// Encoding GDT entries on the fly instead of hardcoding them.
// https://wiki.osdev.org/GDT_Tutorial.
uint64_t encode_gdt_entry(gdt_t source) {
    uint64_t descriptor = 0;

    // Check the limit to make sure that it can be encoded.
    if (source.limit > 0xFFFFF) {
        kerror("GDT cannot encode limits larger than 0xFFFFF.");
        hlt();
    }

    // Encode the limit.
    descriptor |= (source.limit & 0xFFFF);
    descriptor |= ((uint64_t)(source.limit >> 16) & 0x0F) << 48;

    // Encode the base.
    descriptor |= ((uint64_t)(source.base & 0xFFFFFF)) << 16;
    descriptor |= ((uint64_t)(source.base >> 24) & 0xFF) << 56;

    // Encode the access byte.
    descriptor |= ((uint64_t)source.access_byte) << 40;

    // Encode the flags.
    descriptor |= ((uint64_t)(source.flags & 0x0F)) << 52;

    // Write the complete descriptor.
    return descriptor;
}

// Initializing the GDT.
// Still not entirely sure why I gotta do this.
void init_gdt() {
    // Defining each GDT entry as per https://wiki.osdev.org/GDT_Tutorial.
    gdt_t gdt_entries[] = {
        // Null.
        {
            .limit = 0x00000,
            .base = 0x00000000,
            .access_byte = 0x00,
            .flags = 0x0,
        },

        // Kernel code.
        {
            .limit = 0xFFFFF,
            .base = 0x00000000,
            .access_byte = 0x9A,
            .flags = 0xA,
        },

        // Kernel data.
        {
            .limit = 0xFFFFF,
            .base = 0x00000000,
            .access_byte = 0x92,
            .flags = 0xC,
        },

        // User code.
        {
            .limit = 0xFFFFF,
            .base = 0x00000000,
            .access_byte = 0xFA,
            .flags = 0xA,
        },

        // User data.
        {
            .limit = 0xFFFFF,
            .base = 0x00000000,
            .access_byte = 0xF2,
            .flags = 0xC,
        },
    };

    // Initialize the GDT. We need it to have the same number of elements as
    // `gdt_entries`.
    static uint64_t gdt[GDT_ENTRY_COUNT] = {0};

    // Looping through `gdt_entries` and encoding everything.
    for (size_t i = 0; i < GDT_ENTRY_COUNT; ++i) {
        gdt[i] = encode_gdt_entry(gdt_entries[i]);
    }

    // "The size of the table in bytes subtracted by 1. This subtraction
    // occurs because the maximum value of Size is 65535, while the GDT can
    // be up to 65536 bytes in length (8192 entries). Further, no."
    // - https://wiki.osdev.org/Global_Descriptor_Table.
    gdtr_t gdtr = {.base = (uint64_t)gdt, .limit = sizeof(gdt) - 1};

    // Store the GDT info to GDTR via the LGDT instruction.
    // Refer to Intel's SDM Vol. 2A 3-599.
    __asm__ volatile("LGDT %0" : : "m"(gdtr));

    // Reload CS via far return and update data segments.
    // Pasted straight out of the osdev wiki.
    reload_segments();

    klog("GDT setup complete.\r\n");
}
