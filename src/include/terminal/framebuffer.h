/**
 * @file framebuffer.h
 * @brief Framebuffer-backed implementation of the Voyager terminal interface.
 * @ingroup terminal
 */
#ifndef _TERM_FRAMEBUFFER_H
#define _TERM_FRAMEBUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "term.h"

/** Number of glyph slots expected by the framebuffer terminal font table. */
#define FBTERM_FONT_GLYPHS 256

/** @brief One rendered terminal cell and its foreground/background colours. */
struct fbterm_char
{
    uint32_t c;
    uint32_t fg;
    uint32_t bg;
};

/** @brief Deferred framebuffer-cell update used by the renderer queue. */
struct fbterm_queue_item
{
    size_t             x, y;
    struct fbterm_char c;
};

/**
 * @brief Framebuffer terminal backend state.
 *
 * The generic term_context is embedded first so callers can use the returned
 * object through the backend-independent terminal API. The remaining fields
 * hold font geometry, framebuffer/canvas storage, cell state, queued updates,
 * colours, and cursor state.
 */
struct fbterm_context
{
    struct term_context term;

    size_t font_width;
    size_t font_height;
    size_t glyph_width;
    size_t glyph_height;

    size_t font_scale_x;
    size_t font_scale_y;

    size_t offset_x, offset_y;

    volatile uint32_t *framebuffer;
    size_t             pitch;
    size_t             width;
    size_t             height;
    size_t             bpp;

    size_t   font_bits_size;
    uint8_t *font_bits;
    size_t   font_bool_size;
    bool    *font_bool;

    uint32_t ansi_colours[8];
    uint32_t ansi_bright_colours[8];
    uint32_t default_fg, default_bg;

    size_t    canvas_size;
    uint32_t *canvas;

    size_t grid_size;
    size_t queue_size;
    size_t map_size;

    struct fbterm_char *grid;

    struct fbterm_queue_item *queue;
    size_t                    queue_i;

    struct fbterm_queue_item **map;

    uint32_t text_fg;
    uint32_t text_bg;
    bool     cursor_status;
    size_t   cursor_x;
    size_t   cursor_y;

    uint32_t saved_state_text_fg;
    uint32_t saved_state_text_bg;
    size_t   saved_state_cursor_x;
    size_t   saved_state_cursor_y;

    size_t old_cursor_x;
    size_t old_cursor_y;
};

/**
 * @brief Initialize a framebuffer terminal and return its generic context.
 * @param _malloc Allocation function used for terminal-owned dynamic storage.
 * @param framebuffer Kernel-accessible framebuffer base.
 * @param width Framebuffer width in pixels.
 * @param height Framebuffer height in pixels.
 * @param pitch Framebuffer pitch in bytes.
 * @param canvas Optional backing canvas supplied by the caller.
 * @param ansi_colours Optional normal ANSI palette.
 * @param ansi_bright_colours Optional bright ANSI palette.
 * @param default_bg Pointer to the default background colour.
 * @param default_fg Pointer to the default foreground colour.
 * @param font Optional caller-supplied font data.
 * @param font_width Caller-supplied font width when applicable.
 * @param font_height Caller-supplied font height when applicable.
 * @param font_spacing Horizontal font spacing.
 * @param font_scale_x Horizontal scale factor.
 * @param font_scale_y Vertical scale factor.
 * @param margin Terminal margin in pixels/cells as interpreted by the backend.
 * @return Initialized backend-independent terminal context, or NULL on failure.
 *
 * VoyagerOS64 initializes this only after its own page tables and heap are
 * live; the resulting context replaces Limine terminal callbacks for normal
 * kernel console output.
 */
struct term_context *fbterm_init(void *(*_malloc)(size_t),
                                 uint32_t *framebuffer,
                                 size_t    width,
                                 size_t    height,
                                 size_t    pitch,
                                 uint32_t *canvas,
                                 uint32_t *ansi_colours,
                                 uint32_t *ansi_bright_colours,
                                 uint32_t *default_bg,
                                 uint32_t *default_fg,
                                 void     *font,
                                 size_t    font_width,
                                 size_t    font_height,
                                 size_t    font_spacing,
                                 size_t    font_scale_x,
                                 size_t    font_scale_y,
                                 size_t    margin);

#endif