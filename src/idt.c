#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"


ALIGN_16BIT static idt_entry_t idt[IDT_ENTRIES];

static idtr_t idtr;

void int_handler(){

    printf_("%s\n", "!!!KERNEL PANIC!!!");
    printf_("%s", "INTERRUPT HANDLING NOT AVIAL!");

}