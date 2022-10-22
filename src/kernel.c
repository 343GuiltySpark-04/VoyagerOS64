#include <stdint.h>
#include <stddef.h>
#include "include/gdt.h"
#include "include/idt.h"
#include "include/KernelUtils.h"
#include "include/serial.h"
#include "include/limine.h"
#include "include/printf.h"
#include "include/heap.h"
#include "include/kernel.h"
#include "include/paging/frameallocator.h"
#include "include/registers.h"
#include "include/memUtils.h"
#include "include/termUtils.h"
#include "include/term.h"
#include "include/vgafont.h"
#include "include/pic.h"

#define White "\033[1;00m"
#define Red "\033[1;31m"
#define Green "\033[1;32m"
#define Yellow "\033[1;33m"
#define Blue "\033[1;34m"
#define Purple "\033[1;35m"
#define Cyan "\033[1;36m"
#define Black "\033[1;37m"

/// @attention Limine requests can be placed anywhere, but it is important that
/// the compiler does not optimise them away, so, usually, they should
/// be made volatile or equivalent.

volatile struct limine_kernel_address_request Kaddress_req = {

    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0

};

struct term_t term;

/// @brief breakpoint() Provides a magic breakpoint for debugging in Bochs
extern void breakpoint();
extern void stop_interrupts();
extern void start_interrupts();
extern void halt();

struct framebuffer_t fbr;

struct font_t font;

struct background_t back = {

    .background = NULL

};

struct style_t style = {

    .ansi_colours = DEFAULT_ANSI_COLOURS,
    .ansi_bright_colours = DEFAULT_ANSI_BRIGHT_COLOURS,
    .background = DEFAULT_BACKGROUND,
    .foreground = DEFAULT_FOREGROUND,
    .margin = DEFAULT_MARGIN,
    .margin_gradient = DEFAULT_MARGIN_GRADIENT

};

void setup_terminal()
{

    fbr.address = fbr_req.response->framebuffers[0]->address;
    fbr.width = fbr_req.response->framebuffers[0]->width;
    fbr.height = fbr_req.response->framebuffers[0]->height;
    fbr.pitch = fbr_req.response->framebuffers[0]->pitch;

    font.address = (uintptr_t)&vgafont;
    font.width = 8;
    font.height = 16;
    font.spacing = 1;
    font.scale_x = 0;
    font.scale_y = 0;
}

/// @var allows me to enable or dissable or alter behavoir according to wether the kernel
/// is fully loaded yet.
uint32_t bootspace = 1;

/// \fn  following will be our kernel's entry point.
void _start(void)
{

    breakpoint();

    stop_interrupts();

    LoadGDT_Stage1();

    printf_("%s\n", "Loaded GDT");

    breakpoint();

    idt_init();
    

   pic_enable();

    printf_("%s\n", "Loaded IDT");

    print_memmap();

    // @brief Kernel Addresses
    if (Kaddress_req.response == NULL)
    {
        printf_("%s\n", "!!!Error While Fetching Kernel Addresses!!!");
    }
    else
    {
        printf_("%s\n", "Kernel Base Addresses Are As Follows: ");
        printf_("%s", "Physical Address: ");
        printf_("0x%llx\n", Kaddress_req.response->physical_base);
        printf_("%s", "Virtual Address: ");
        printf_("0x%llx\n", Kaddress_req.response->virtual_base);
        printf_("%s\n", "--------------------------------------");
    }

    breakpoint();

    read_memory_map();

    /// @brief print usable memory to log
    printf("total memory: %llu\nfree memory: %llu\nused memory: %llu\nreserved memory: %llu\n", get_memory_size(), free_ram(), used_ram(), reserved_ram());

    init_memory();

    printf_("%s", "CR3: ");
    printf_("0x%llx\n", readCR3());

    setup_terminal();

    term_init(&term, NULL, true);

    term_vbe(&term, fbr, font, style, back);

    term_set_text_fg_rgb(&term, 0x0055FF55);

    bootspace = 0;

    printf("total memory: %llu\nfree memory: %llu\nused memory: %llu\nreserved memory: %llu\n", get_memory_size(), free_ram(), used_ram(), reserved_ram());

    printf_("%s\n", "Kernel Loaded");

    term_print(&term, "VoyagerOS64 v0.0.2");

    // Just chill until needed
    while (1)
    {
    }
}