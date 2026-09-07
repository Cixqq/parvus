#ifndef _CPU_H
#define _CPU_H

// Halt macro.
// Basically instructs the CPU to do nothing.
// I figured this might be the best place to store this macro in.
#define hlt()                                                                  \
    do {                                                                       \
        while (1)                                                              \
            asm("hlt");                                                        \
    } while (0)

#define force_div_by_zero() __asm__("div %0" ::"r"(0))

#endif // _CPU_H
