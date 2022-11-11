#include "include/bitmap.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/paging/vmm.h"
#include "include/registers.h"
#include "include/string.h"
#include "include/printf.h"

static struct PageTable *vmm_page_table;
struct dummy_proc *test_proc;

uint64_t proc_page_size = 0;

void VMM_table_clone()
{

    static struct PageTable *table_frame;
    static struct PageTable *current_table;
    static struct PageTable *higher_frame;

    // test_proc->id = 0x0;
    // test_proc->data = 0x15000;

    table_frame = (struct PageTable *)frame_request();
    current_table = (struct PageTable *)readCR3();

    PagingMapMemory(current_table, TranslateToHighHalfMemoryAddress((uint64_t)table_frame), table_frame, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE);

    higher_frame = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)table_frame);

    memset(higher_frame, 0, sizeof(struct PageTable));

    auto proc_page_size = 0x8000 / 0x1000 + 1;

    printf_("%s\n", "pages used by test: ");
    printf_("%i\n", proc_page_size);

    test_proc = (struct dummy_proc *)frame_request_multiple(proc_page_size);
}