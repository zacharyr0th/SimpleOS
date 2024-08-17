// This file implements terminal output functions for SimpleOS.
// It provides low-level screen manipulation for text mode display.

#include "terminal.h"
#include <stddef.h>
#include <stdint.h>

// VGA text mode constants
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// VGA color codes
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
};

// VGA buffer and terminal state
volatile uint16_t* const VGA_BUFFER = (uint16_t*) VGA_MEMORY;
static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;

// Function to calculate string length
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    } else {
        const size_t index = terminal_row * VGA_WIDTH + terminal_column;
        VGA_BUFFER[index] = (uint16_t)c | (uint16_t)terminal_color << 8;
        if (++terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            if (++terminal_row == VGA_HEIGHT) {
                terminal_row = 0;
            }
        }
    }
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

void init_vga(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = VGA_COLOR_LIGHT_GREY | VGA_COLOR_BLACK << 4;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            VGA_BUFFER[index] = (uint16_t)' ' | (uint16_t)terminal_color << 8;
        }
    }
}