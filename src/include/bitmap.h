#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "printf.h"
#include "kernel.h"
#include "KernelUtils.h"

/**
 * @brief Gets the value of a bit in a bitmap.
 * @param * bitmap
 * @param bit The bit to read.
 * @return True if the bit is set
 */
inline bool bitmap_get(uint8_t *bitmap, size_t bit)
{
    uint64_t byteIndex = bit / 8;
    uint8_t bitIndex = bit % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;
    uint8_t wtf = (bitmap[byteIndex] & bitIndexer) > 0;

    if (temp == 1)
    {

        //printf_("0x%llx\n", bit);
    }

    return (bitmap[byteIndex] & bitIndexer) > 0;
}

/**
 * @brief Sets or clears a bit in a bitmap.
 * @param * bitmap
 * @param bit The bit to set or clear.
 * @param value The value to set the bit to
 */
inline void bitmap_set(uint8_t *bitmap, size_t bit, uint8_t value)
{
    uint64_t byteIndex = bit / 8;
    uint8_t bitIndex = bit % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    bitmap[byteIndex] &= ~bitIndexer;

    if (value)
    {
        bitmap[byteIndex] |= bitIndexer;
    }
}