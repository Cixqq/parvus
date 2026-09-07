#ifndef _TERMINAL_H
#define _TERMINAL_H
#include <flanterm/flanterm.h>
#include <limine.h>

extern struct flanterm_context* ft_ctx;
bool init_terminal(volatile struct limine_framebuffer* framebuffer);

#define kprint(msg) flanterm_write(ft_ctx, msg, sizeof(msg));
#define klog(msg) kprint("LOG: " msg);
#define kerror(msg) kprint("ERROR: " msg);

#endif // _TERMINAL_H
