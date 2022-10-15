 #pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

inline uint8_t bitmap_get(uint8_t *bitmap, size_t bit)
{
    size_t bitmapIndex = bit / 8;
    size_t bitIndex = bit % 8;

    return bitmap[bitmapIndex] & (1 << bitIndex);
}

inline void bitmap_set(uint8_t *bitmap, size_t bit, uint8_t value)
{
    size_t bitmapIndex = bit / 8;
    size_t bitIndex = bit % 8;

    if(value)
    {
        bitmap[bitmapIndex] |= (1 << bitIndex);
    }
    else
    {
        bitmap[bitmapIndex] &= ~(1 << bitIndex);
    }
}
