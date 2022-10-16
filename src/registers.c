#include <stdint.h>



void writeMSR(uint64_t msr, uint64_t value)
{
  uint32_t lo = (uint32_t)value;
  uint32_t hi = value >> 32;

  asm volatile ("wrmsr" : /* no output */ : "a"(lo), "d"(hi), "c"(msr));
}