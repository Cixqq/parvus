#include <cpu/gdt.h>
#include <flanterm/flanterm.h>
#include <flanterm/flanterm_backends/fb.h>
#include <lib/memory.h>
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "terminal.h"

// Set base Limine's revision to 6 (latest).
__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The protocol is centered around the concept of request/response -
// collectively named "features" - where the executable requests some action or
// information from the bootloader, and the bootloader responds accordingly, if
// it is capable of doing so.
// Requests can be placed anywhere, but it's important that the compiler doesn't
// optimize them away, so, usually, they should be made volatile and they should
// be accessed at least once or marked as used with the "used" attribute.
__attribute__((
    used,
    section(
        ".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

// Defining the start and end markers for the Limine requests.
__attribute((used, section(".limine_requests_start"))) static volatile uint64_t
    limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute((used, section(".limine_requests_end"))) static volatile uint64_t
    limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// Pretty much like GDTR.
typedef struct {
    uint16_t limit; // The size of the IDT in bytes.
    uint64_t base;  // The base address of the IDT object.
} __attribute__((packed)) idtr_t;

// Yoinked from the osdev wiki.
// https://wiki.osdev.org/Inline_Assembly/Examples#LIDT.
void load_ldt(idtr_t* idtr) {
    asm("lidt %0" : : "m"(idtr)); // let the compiler choose an addressing mode.
}

// Halt function.
// Basically instructs the CPU to do nothing.
static void hlt() {
    while (1)
        asm("hlt");
}

// Initializing Limine's framebuffer so we can use it
// for Flanterm later on and possibly even drawing
// some cool stuff with it.
volatile struct limine_framebuffer* init_framebuffer() {
    // Check if the response of our Limine request
    // was actually successfull.
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1)
        hlt();

    // Fetch the first framebuffer.
    volatile struct limine_framebuffer* framebuffer =
        framebuffer_request.response->framebuffers[0];

    return framebuffer;
}

void kmain() {
    // If the revision isn't supported by the bootloader then
    // we dip.
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false)
        goto halt;

    // Get a framebuffer and initialize Flanterm.
    volatile struct limine_framebuffer* framebuffer = init_framebuffer();
    if (!init_terminal(framebuffer))
        goto halt;
    klog("Flanterm successfully initialized.\r\n");

    // Initialize the GDT.
    init_gdt();

    klog("System fully initialized!\r\n");

    // Nothing else to do. Just hang.
halt:
    hlt();
}
