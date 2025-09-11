#include <stdint.h>

/**
* @brief Write a 64 - bit value to an MSR.
* @param msr The MSR to write to.
* @param value The value to write
*/
void writeMSR(uint64_t msr, uint64_t value)
{
  uint32_t lo = (uint32_t)value;
  uint32_t hi = value >> 32;

  asm volatile("wrmsr"
               : /* no output */
               : "a"(lo), "d"(hi), "c"(msr));
}

/**
* @brief Read an MSR and return the value.
* @param msr Address of the MSR to read.
* @return Value read from the MSR
*/
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