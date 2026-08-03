#include "drivers/serial.h"
#include "drivers/ports.h"
#include <stdint.h>

#define COM1 0x3F8

// Register offsets from the base port
#define UART_THR 0  // Transmit holding (write, DLAB=0)
#define UART_IER 1  // Interrupt enable (DLAB=0)
#define UART_DLL 0  // Divisor latch low (DLAB=1)
#define UART_DLM 1  // Divisor latch high (DLAB=1)
#define UART_FCR 2  // FIFO control (write)
#define UART_LCR 3  // Line control
#define UART_MCR 4  // Modem control
#define UART_LSR 5  // Line status

#define LSR_THR_EMPTY 0x20

void serial_init(void) {
    outb(COM1 + UART_IER, 0x00);  // Disable interrupts
    outb(COM1 + UART_LCR, 0x80);  // Enable DLAB to set baud divisor
    outb(COM1 + UART_DLL, 0x01);  // Divisor 1 => 115200 baud
    outb(COM1 + UART_DLM, 0x00);
    outb(COM1 + UART_LCR, 0x03);  // 8 bits, no parity, 1 stop; clears DLAB
    outb(COM1 + UART_FCR, 0xC7);  // Enable+clear FIFOs, 14-byte threshold
    outb(COM1 + UART_MCR, 0x0B);  // DTR, RTS, OUT2
}

void serial_write_char(char c) {
    // Bounded wait so a misbehaving UART can never hang the kernel.
    for (int i = 0; i < 100000; i++) {
        if (inb(COM1 + UART_LSR) & LSR_THR_EMPTY) break;
    }
    outb(COM1 + UART_THR, (uint8_t)c);
}

void serial_write_string(const char* s) {
    while (*s) serial_write_char(*s++);
}
