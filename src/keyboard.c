// This file implements keyboard input handling for SimpleOS.
// It sets up the keyboard interrupt handler and translates scancodes to ASCII characters.

#include "ports.h"
#include "terminal.h"
#include "isr.h"

#define KEYBOARD_DATA_PORT 0x60
#define IRQ1 33  // Keyboard IRQ

// US keyboard layout scancodes (0-127)
static char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Keyboard interrupt handler
static void keyboard_callback(registers_t regs) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (!(scancode & 0x80)) {  // Key press
        char c = kbd_us[scancode];
        if (c) {
            char str[2] = {c, '\0'};
            terminal_writestring(str);
        }
    }
}

// Initialize keyboard and register interrupt handler
void init_keyboard() {
    register_interrupt_handler(IRQ1, &keyboard_callback);
}