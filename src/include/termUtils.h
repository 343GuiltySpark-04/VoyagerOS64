#pragma once

#ifndef _TERM_UTILS_H

#define _TERM_UTILS_H

extern struct term_t term;

extern struct style_t style;

extern struct framebuffer_t fbr;

extern struct font_t font;

extern struct background_t back;

void setup_terminal();

#endif