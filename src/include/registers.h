#pragma once
#include <stdint.h>

extern uint64_t readCRO();
extern uint64_t readCR3();
extern uint64_t readCR2();
extern void writeCR0(uint64_t rdi);
extern void writeCR3(uint64_t rdi);