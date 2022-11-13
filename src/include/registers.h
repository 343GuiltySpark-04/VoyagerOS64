#pragma once
#include <stdint.h>

extern uint64_t readCRO();
extern uint64_t readCR3();
extern uint64_t readCR2();
extern uint64_t readRSP();
extern uint64_t readCR4();
extern void writeCR4(uint64_t rdi);
extern void writeCR0(uint64_t rdi);
extern void writeCR3(uint64_t rdi);
void writeMSR(uint64_t msr, uint64_t value);
uint64_t rdmsr(uint32_t msr);