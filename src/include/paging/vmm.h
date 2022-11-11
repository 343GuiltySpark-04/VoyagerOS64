#pragma once
#include <stdint.h>

void VMM_table_clone();

struct dummy_proc
{

    uint64_t id;
    uint64_t data;
};
