#pragma once
#ifndef _PANIC_H
#define _PANIC_H

__attribute__((noreturn)) void panic(const char *fmt, ...);

#endif
