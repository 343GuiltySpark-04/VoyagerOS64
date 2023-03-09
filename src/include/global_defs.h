#pragma once
#include <stdint.h>
#include "printf.h"

#define PACKED __attribute__((packed))

#define ALIGN_4K __attribute__((aligned(0x1000)))

#define ALIGN_16BIT __attribute__((aligned(0x10)))

#define CAS __sync_bool_compare_and_swap

#define MIN(A, B)                      \
    ({                                 \
        __auto_type MIN_a = A;         \
        __auto_type MIN_b = B;         \
        MIN_a < MIN_b ? MIN_a : MIN_b; \
    })

#define MAX(A, B)                      \
    ({                                 \
        __auto_type MAX_a = A;         \
        __auto_type MAX_b = B;         \
        MAX_a > MAX_b ? MAX_a : MAX_b; \
    })

#define ASSERT(COND)                                     \
    do                                                   \
    {                                                    \
        if (!(COND))                                     \
        {                                                \
            printf_("%s\n", "!!!KERNEL PANIC!!!");       \
            printf_("%s\n", "Assertion failed: " #COND); \
            printf_("%s\n", "!!!KERNEL PANIC!!!");       \
            __asm__("hlt");                              \
        }                                                \
    } while (0)

typedef int32_t mode_t;
typedef signed long ssize_t;
typedef unsigned long long lsize_t;
typedef void *HANDLE;