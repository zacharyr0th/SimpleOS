#ifndef DRIVERS_SERIAL_H
#define DRIVERS_SERIAL_H

// Minimal COM1 (16550 UART) driver. Used to mirror terminal output to the
// serial line so the OS can be observed headlessly (qemu -serial ...), which
// is what the CI smoke test asserts on.
void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char* s);

#endif
