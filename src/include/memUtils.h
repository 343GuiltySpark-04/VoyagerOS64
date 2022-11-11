#pragma once
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include "heap.h"
#include "liballoc.h"

extern KHEAPBM kheap;

#define SIZE 0x500000
#define BSIZE 16

#define ALLOC(TYPE) (malloc(sizeof(TYPE)))

#endif