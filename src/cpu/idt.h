#ifndef _IDT_H
#define _IDT_H
#include <stdint.h>

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

// Defining the CPU state object. This object will be passed to the ISR with a
// lot of juicy stuff. We should make sure that since everything is passed to
// the stack manually in assembly and the stack grows backwards, everything is
// gonna be flipped.
typedef struct {
    // Pushed to the stack via the common function.
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

    // Push by us.
    uint64_t interrupt_number, errno;

    // Pushed by the CPU as per Figure 6-9. IA-32e Mode Stack Usage After
    // Privilege Level Change of the SDM.
    uint64_t rip, cs, rflags, rsp, ss;
} cpu_t;

void init_idt();
void int_handler();
void isr_stub();

// We know that an interrupt returns nothing and will get `cpu_t` as the type.
// Defining a typedef for interrupt handlers.
// We love function pointers in C.
typedef void (*isr_t)(cpu_t* frame);
#endif // #_IDT_H
