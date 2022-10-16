 #pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

inline bool bitmap_get(uint8_t *bitmap, size_t bit)
{
    uint64_t byteIndex = bit / 8;
    uint8_t bitIndex = bit % 8;
    uint8_t bitIndexer = 0b10000000 >> bitIndex;

    return (bitmap[byteIndex] & bitIndexer) > 0;
}

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
