#include "include/sched.h"
#include "include/liballoc.h"
#include "include/printf.h"
#include "include/tss.h"
#include "include/registers.h"
#include "include/gdt.h"





void init_tasking(){

    static task_ctx_t ctx;

    ctx.stack.base = rsp0;

    



}