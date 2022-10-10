#pragma once

#define PACKED __attribute__((packed))
#define ALIGN_4K __attribute__((aligned(0x1000)))
#define ALIGN_16BIT __attribute__((aligned(0x10)))