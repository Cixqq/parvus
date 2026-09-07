#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>

// No idea what's that for but apparently it's required for defining interrupt
// handlers.
struct interrupt_frame {
    uintptr_t ip;
    uintptr_t cs;
    uintptr_t flags;
    uintptr_t sp;
    uintptr_t ss;
};

// Pretty much like GDTR.
typedef struct {
    uint16_t limit; // The size of the IDT in bytes.
    uint64_t base;  // The base address of the IDT object.
} __attribute__((packed)) idtr_t;

// Defining the IDT entry as per "Figure 6-8. 64-Bit IDT Gate Descriptors" in
// the SDM.
typedef struct {
    uint16_t offset1; // A 64-bit value split into 3 parts. It respresents the
                      // entry point of an ISR. This is part 1.
    uint16_t segment_selector; // A segment selector that points to a valid code
                               // segment in the GDT.
    uint8_t ist;  // A 3-bit value which is an offset into the "Interrupt stack
                  // table". (No idea what does this do yet)
    uint8_t type; // A gate type is a 4-byte value which defines the type of the
                  // gate this Interrupt descriptor represents.
                  // - 0b1110 or 0xE: 64-bit Interrupt Gate
                  // - 0b1111 or 0xF: 64-bit Trap Gate
    uint16_t offset2;  // Part 2 of the offset.
    uint32_t offset3;  // Part 3 of the offset.
    uint32_t reserved; // For some reason, there's an entire 32 bit region
                       // reserved in the IDT descriptor object.
} __attribute__((packed)) idt_t;

void init_idt();
#endif // #_IDT_H
