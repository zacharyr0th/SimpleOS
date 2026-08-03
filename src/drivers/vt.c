#include "drivers/vt.h"
#include "drivers/terminal.h"
#include "lib/string.h"
#include "drivers/ports.h"

// Virtual terminals
static virtual_terminal_t terminals[NUM_VIRTUAL_TERMINALS];
static int current_terminal = 0;

// VGA memory
static uint16_t* vga_buffer = (uint16_t*)0xB8000;

void vt_init(void) {
    for (int i = 0; i < NUM_VIRTUAL_TERMINALS; i++) {
        // Clear buffer
        for (int j = 0; j < 80 * 25; j++) {
            terminals[i].buffer[j] = ' ' | (0x0F << 8);  // White on black
        }
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].color = 0x0F;  // White on black
        terminals[i].active = 0;
        terminals[i].shell_pid = -1;
        terminals[i].esc_state = 0;
        terminals[i].esc_nparams = 0;
    }

    // Activate first terminal
    terminals[0].active = 1;
    vt_refresh();
}

void vt_switch(int terminal) {
    if (terminal < 0 || terminal >= NUM_VIRTUAL_TERMINALS) return;
    if (terminal == current_terminal) return;

    // Save current terminal state is already in buffer

    // Switch to new terminal
    current_terminal = terminal;

    // Mark as active
    terminals[terminal].active = 1;

    // Refresh screen with new terminal's content
    vt_refresh();

    // Update cursor position
    terminal_set_cursor(terminals[terminal].cursor_x, terminals[terminal].cursor_y);
}

int vt_get_current(void) {
    return current_terminal;
}

// Map an ANSI color index (0-7) to the VGA color index.
static const uint8_t ansi_to_vga[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

// Execute a completed CSI sequence (ESC [ params final).
static void vt_apply_csi(virtual_terminal_t* vt, char final) {
    int p0 = vt->esc_nparams > 0 ? vt->esc_params[0] : 0;

    switch (final) {
        case 'H':   // cursor position (row;col, 1-based); no params = home
        case 'f': {
            int row = vt->esc_nparams > 0 ? vt->esc_params[0] : 1;
            int col = vt->esc_nparams > 1 ? vt->esc_params[1] : 1;
            if (row < 1) row = 1;
            if (col < 1) col = 1;
            if (row > 25) row = 25;
            if (col > 80) col = 80;
            vt->cursor_y = (uint16_t)(row - 1);
            vt->cursor_x = (uint16_t)(col - 1);
            break;
        }
        case 'J': {   // erase display: 0=cursor->end, 1=start->cursor, 2=all
            int from = 0, to = 80 * 25;
            int cur = vt->cursor_y * 80 + vt->cursor_x;
            if (p0 == 0) { from = cur; to = 80 * 25; }
            else if (p0 == 1) { from = 0; to = cur + 1; }
            for (int i = from; i < to && i < 80 * 25; i++) {
                vt->buffer[i] = ' ' | (vt->color << 8);
            }
            break;
        }
        case 'K': {   // erase line: 0=cursor->eol, 1=bol->cursor, 2=whole line
            int line = vt->cursor_y * 80;
            int from = line, to = line + 80;
            if (p0 == 0) { from = line + vt->cursor_x; }
            else if (p0 == 1) { to = line + vt->cursor_x + 1; }
            for (int i = from; i < to && i < 80 * 25; i++) {
                vt->buffer[i] = ' ' | (vt->color << 8);
            }
            break;
        }
        case 'm': {   // SGR: reset + basic 8-color fg/bg
            if (vt->esc_nparams == 0) { vt->color = 0x0F; break; }
            for (int i = 0; i < vt->esc_nparams; i++) {
                int p = vt->esc_params[i];
                if (p == 0) vt->color = 0x0F;
                else if (p >= 30 && p <= 37) vt->color = (vt->color & 0xF0) | ansi_to_vga[p - 30];
                else if (p >= 40 && p <= 47) vt->color = (vt->color & 0x0F) | (uint8_t)(ansi_to_vga[p - 40] << 4);
            }
            break;
        }
        default:
            break;  // unsupported final byte: ignore
    }
}

// Internal: write char to VT buffer without refreshing screen. Also drives the
// ANSI CSI escape-sequence parser so the shell's clear/cursor codes work.
static void vt_putchar_norefresh(char c) {
    virtual_terminal_t* vt = &terminals[current_terminal];

    // --- ANSI escape state machine ---
    if (vt->esc_state == 1) {            // just saw ESC
        if (c == '[') {
            vt->esc_state = 2;
            vt->esc_nparams = 0;
            vt->esc_params[0] = 0;
        } else {
            vt->esc_state = 0;           // not a CSI sequence: drop
        }
        return;
    }
    if (vt->esc_state == 2) {            // collecting CSI parameters
        if (c >= '0' && c <= '9') {
            if (vt->esc_nparams == 0) vt->esc_nparams = 1;
            vt->esc_params[vt->esc_nparams - 1] =
                vt->esc_params[vt->esc_nparams - 1] * 10 + (c - '0');
            return;
        }
        if (c == ';') {
            if (vt->esc_nparams < 8) {
                vt->esc_params[vt->esc_nparams] = 0;
                vt->esc_nparams++;
            }
            return;
        }
        vt_apply_csi(vt, c);             // final byte
        vt->esc_state = 0;
        return;
    }
    if (c == 27) {                       // ESC
        vt->esc_state = 1;
        return;
    }

    if (c == '\n') {
        vt->cursor_x = 0;
        vt->cursor_y++;
    } else if (c == '\r') {
        vt->cursor_x = 0;
    } else if (c == '\b') {
        if (vt->cursor_x > 0) {
            vt->cursor_x--;
            vt->buffer[vt->cursor_y * 80 + vt->cursor_x] = ' ' | (vt->color << 8);
        }
    } else if (c == '\t') {
        vt->cursor_x = (vt->cursor_x + 8) & ~7;
        if (vt->cursor_x >= 80) {
            vt->cursor_x = 0;
            vt->cursor_y++;
        }
    } else if (c >= ' ' && c < 127) {
        if (vt->cursor_x >= 80) {
            vt->cursor_x = 0;
            vt->cursor_y++;
        }

        if (vt->cursor_y >= 25) {
            for (int i = 0; i < 24 * 80; i++) {
                vt->buffer[i] = vt->buffer[i + 80];
            }
            for (int i = 24 * 80; i < 25 * 80; i++) {
                vt->buffer[i] = ' ' | (vt->color << 8);
            }
            vt->cursor_y = 24;
        }

        vt->buffer[vt->cursor_y * 80 + vt->cursor_x] = c | (vt->color << 8);
        vt->cursor_x++;
    }

    if (vt->cursor_y >= 25) {
        for (int i = 0; i < 24 * 80; i++) {
            vt->buffer[i] = vt->buffer[i + 80];
        }
        for (int i = 24 * 80; i < 25 * 80; i++) {
            vt->buffer[i] = ' ' | (vt->color << 8);
        }
        vt->cursor_y = 24;
    }
}

void vt_putchar(char c) {
    vt_putchar_norefresh(c);
    virtual_terminal_t* vt = &terminals[current_terminal];
    vt_refresh();
    terminal_set_cursor(vt->cursor_x, vt->cursor_y);
}

// Write string to current virtual terminal (single refresh at end)
void vt_writestring(const char* str) {
    while (*str) {
        vt_putchar_norefresh(*str++);
    }
    vt_refresh();
    virtual_terminal_t* vt = &terminals[current_terminal];
    terminal_set_cursor(vt->cursor_x, vt->cursor_y);
}

// Clear current virtual terminal
void vt_clear(void) {
    virtual_terminal_t* vt = &terminals[current_terminal];

    for (int i = 0; i < 80 * 25; i++) {
        vt->buffer[i] = ' ' | (vt->color << 8);
    }

    vt->cursor_x = 0;
    vt->cursor_y = 0;

    vt_refresh();
    terminal_set_cursor(0, 0);
}

// Update physical screen from current VT
void vt_refresh(void) {
    virtual_terminal_t* vt = &terminals[current_terminal];

    // Copy virtual terminal buffer to VGA memory
    memcpy(vga_buffer, vt->buffer, sizeof(vt->buffer));

    // Show terminal indicator in top-right corner
    const char* indicators[] = {"[F1]", "[F2]", "[F3]", "[F4]"};
    const char* indicator = indicators[current_terminal];
    int x = 76;  // Right side of screen

    for (int i = 0; indicator[i]; i++) {
        vga_buffer[x + i] = indicator[i] | (0x1F << 8);  // White on blue
    }
}

// Get VT for specific terminal
virtual_terminal_t* vt_get(int terminal) {
    if (terminal < 0 || terminal >= NUM_VIRTUAL_TERMINALS) return NULL;
    return &terminals[terminal];
}

#ifdef SELFTEST
// Headless check of the ANSI CSI parser: drive escape sequences through the
// normal write path and inspect the resulting VT buffer/cursor. Results are
// reported on COM1 via terminal_writestring using the "SELFTEST:" prefix so
// scripts/selftest.sh's pass/fail assertions cover them too.
void vt_ansi_selftest(void) {
    virtual_terminal_t* vt = &terminals[current_terminal];

    vt_writestring("\033[2J\033[H");
    int ok_clear = (vt->cursor_x == 0 && vt->cursor_y == 0 &&
                    (vt->buffer[100] & 0xFF) == ' ');

    vt_writestring("\033[5;10H");
    int ok_pos = (vt->cursor_y == 4 && vt->cursor_x == 9);

    vt_writestring("Z");
    int ok_char = ((vt->buffer[4 * 80 + 9] & 0xFF) == 'Z' && vt->cursor_x == 10);

    vt_writestring("\033[7;1HABCDEF\033[7;3H\033[K");
    int base = 6 * 80;
    int ok_erase = ((vt->buffer[base + 0] & 0xFF) == 'A' &&
                    (vt->buffer[base + 1] & 0xFF) == 'B' &&
                    (vt->buffer[base + 2] & 0xFF) == ' ' &&
                    (vt->buffer[base + 5] & 0xFF) == ' ');

    vt_writestring("\033[2J\033[H");  // tidy up before handing off

    terminal_writestring(ok_clear ? "SELFTEST: ansi_clear_home PASS\n"
                                   : "SELFTEST: ansi_clear_home FAIL\n");
    terminal_writestring(ok_pos ? "SELFTEST: ansi_cursor_pos PASS\n"
                                : "SELFTEST: ansi_cursor_pos FAIL\n");
    terminal_writestring(ok_char ? "SELFTEST: ansi_putchar PASS\n"
                                 : "SELFTEST: ansi_putchar FAIL\n");
    terminal_writestring(ok_erase ? "SELFTEST: ansi_erase_line PASS\n"
                                  : "SELFTEST: ansi_erase_line FAIL\n");
}
#endif
