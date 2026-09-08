/*
 * Legacy PagingGetFreeFrame() lived here and routed page-table allocations
 * directly to frame_request() in the old PMM.
 *
 * Stage 2 now routes page-table frame requests through
 * paging/frame_supplier.h. During bootstrap g_boot_phys_page points at the
 * early allocator; after frame_init() it points at neo_framealloc.
 *
 * Keep this translation unit temporarily so the existing build file does not
 * need to change in the same migration step. It can be removed once the old
 * memory objects are pruned from the build.
 */
