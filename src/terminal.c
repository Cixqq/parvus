#include <flanterm/flanterm_backends/fb.h>
#include <terminal.h>

// I hate global variables but I guess this is the best
// way to do this.
struct flanterm_context* ft_ctx = {0};

bool init_terminal(volatile struct limine_framebuffer* framebuffer) {
    ft_ctx = flanterm_fb_init(
        NULL, NULL, framebuffer->address, framebuffer->width,
        framebuffer->height, framebuffer->pitch, framebuffer->red_mask_size,
        framebuffer->red_mask_shift, framebuffer->green_mask_size,
        framebuffer->green_mask_shift, framebuffer->blue_mask_size,
        framebuffer->blue_mask_shift, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, 0, 0, 1, 0, 0, 0, 0, true);

    return (ft_ctx != NULL);
}
