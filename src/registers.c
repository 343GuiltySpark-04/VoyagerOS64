#include <stdint.h>

void writeMSR(uint64_t msr, uint64_t value)
{
  uint32_t lo = (uint32_t)value;
  uint32_t hi = value >> 32;

  asm volatile("wrmsr"
               : /* no output */
               : "a"(lo), "d"(hi), "c"(msr));
}

uint64_t rdmsr(uint32_t msr)
{
  uint32_t edx = 0, eax = 0;
  asm volatile(
      "rdmsr\n\t"
      : "=a"(eax), "=d"(edx)
      : "c"(msr)
      : "memory");
  return ((uint64_t)edx << 32) | eax;
}