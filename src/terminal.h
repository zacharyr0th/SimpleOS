// This header file declares functions for terminal output in SimpleOS.
// It provides an interface for writing strings to the terminal display.

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

void init_vga(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);

#endif // TERMINAL_H