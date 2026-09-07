#include <cpu/cpu.h>
#include <cpu/idt.h>
#include <terminal.h>

// START: Pasted blindly from the wiki.
// TODO: Understand whatever is written below.
// "Division Error" Interrupt handler.
// This is our first handler and it's job is very easy; if we catch a
// division-by-zero error, we panic.
__attribute__((interrupt)) void de_handler(struct interrupt_frame* frame) {
    (void)frame;
    kerror("PANIK.\r\n");
    hlt();
}
// END.

// Adding IDT entries to an IDT object. This will be very helpful when we start
// to scale up the number of interrupts.
void add_idt_entry(idt_t* source, idt_t* entry, uint8_t index) {
    source[index] = *entry;
}

// Helper to create a standard 64-bit interrupt gate descriptor.
idt_t create_idt_entry(uint64_t offset, uint16_t segment_selector, uint8_t ist,
                       uint8_t type) {
    idt_t entry;

    entry.offset1 = (uint16_t)(offset & 0xFFFF); // First 2 bytes of the offset.
    entry.segment_selector = segment_selector;
    entry.ist = ist & 0x7;
    entry.type = type;
    entry.offset2 =
        (uint16_t)((offset >> 16) & 0xFFFF); // Second 2 bytes of the offset.
    entry.offset3 =
        (uint32_t)(offset >> 32); // The rest of the offset (4 bytes).
    entry.reserved = 0;           // Must be zero.

    return entry;
}

// Add the IDT entries and execute LIDT.
void init_idt() {
    // Initialize the IDT object. The Intel SDM mandates that the IDT has 256
    // entries.
    static idt_t idt[256];

    // Create the descriptor that'll be added to the IDT.
    // I would probably want to make the kernel code segment a constant instead
    // of hardcoding it.
    idt_t de_descriptor = create_idt_entry((uint64_t)de_handler, 0x08, 0, 0x8E);

    // Add the entries to the IDT.
    add_idt_entry(idt, &de_descriptor, 0x0);

    // Initialize the IDTR object and execute LIDT.
    idtr_t idtr = {.base = (uint64_t)&idt,
                   .limit = sizeof(idt) -
                            1};   // I'm assuming you have to decrement by 1 as
                                  // well just like the GDTR limit.
    asm("LIDT %0" : : "m"(idtr)); // Let the compiler choose an addressing mode.

    klog("IDT setup complete.\r\n");
}
