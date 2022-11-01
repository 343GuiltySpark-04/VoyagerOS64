#include "include/sched.h"
#include "include/liballoc.h"
#include "include/printf.h"
#include "include/tss.h"
#include "include/registers.h"
#include "include/gdt.h"


uint8_t task_timer_count = 0;



void task_switch_handler(){

    printf_("%s\n", "Switched!");

    task_timer_count = 0;



}