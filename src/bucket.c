/*             Author: Benjamin David Lunt
 *                     Forever Young Software
 *                     Copyright (c) 1984-2022
 *
 *  This code is donated to the Freeware communitee.  You have the
 *   right to use it for learning purposes only.  You may not modify it
 *   for redistribution for any other purpose unless you have written
 *   permission from the author.
 *
 *  You may modify and use it in your own projects as long as they are
 *   for non-profit only and if distributed, have the same requirements.
 *  Any project for profit that uses this code must have written
 *   permission from the author.
 *
 *  For more information:
 *    http://www.fysnet.net/osdesign_book_series.htm
 *  Contact:
 *    fys [at] fysnet [dot] net
 *
 * Last update:  5 May 2022
 *
 */

#include <stdint.h>
#include <stddef.h>
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"

#define MODNAME "bucket.c"

//  A few comments about the code:
// there are a few non-standard typedef's within this code.
// for example, the HANDLE typedef is simply a 'void *'.
// a few others in malloc.cpp are bit64u and bit32u.  These are simply 64-bit and 32-bit unsigned integers.
// in fact, the double-forwardslash found at the beginning of this line is considered non-standard.
//  therefore, you might wish to modify it to use /*  and   */
/*
// kernel's memory heap
HANDLE kernel_heap = NULL;

int main( ... ) {

  kernel_heap = malloc_init( some size in bytes );

  void *ptr = malloc(4096, 0 /* no alignment , MALLOC_FLAGS_VIRTUAL, MODNAME);
  if (ptr) {
    // do something with it

    mfree(ptr);
  }

  return 0;
}
 */
// this function allocates 'size' pages of virtual memory from the system
void *mmap(size_t size)
{

    void *ptr = frame_request_multiple(size);

    if (ptr == NULL)
    {

        return NULL;
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    return realPtr;

    ///// If you are a beginner starting your kernel:
    // if you don't have a virtual memory system, or any other memory allocator
    //  you can simply set-aside a region of physical memory for this.
    // For example, if you set-aside the physical memory range:
    //   0x00000000_01000000 -> 0x00000000_01FFFFFF  (16-meg to 32-meg)
    // and simply return:
    //  return (void *) 0x00000000_01000000;
    // this heap allocator will use a 16 meg Bucket.
    //
    // As long as you can guarantee that this will be enough,
    //  meaning that this call will never be called twice, you
    //  can use this allocator without the mmap() functionality.
    // i.e.: If 16 meg will be enough for all memory allocation for your
    //  kernel, simply return a value as above.
    // however, if it isn't and the call is called twice, the second
    //  call will overwrite anything the first call created...

    // TODO: call the system to get 'size' pages of virtual memory
}

// this function free's the allocated virtual memory
void mmap_free(void *ptr, size_t size)
{

    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);

    if (k_mode.addr_debug)
    {

        printf("%s", "INFO: liballoc_free ptr: ");
        printf("0x%llx\n", ptr);
        printf("%s", "INFO: realptr: ");
        printf("0x%llx\n", realPtr);
    }

    frame_free_multiple(realPtr, size);

    return 0;

    // free ptr from virtual memory
}
