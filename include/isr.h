// This header file defines structures and functions for handling interrupt service routines (ISRs).
// It provides a mechanism for registering custom interrupt handlers and a structure to represent CPU registers during an interrupt.

#ifndef ISR_H
#define ISR_H

#include <stdint.h>

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

void register_interrupt_handler(uint8_t n, void (*handler)(registers_t));

#endif 