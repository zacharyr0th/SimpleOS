// This is the main kernel file for SimpleOS, implementing core OS functionality.
// It includes memory management, process scheduling, interrupt handling, and basic I/O operations.

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "terminal.h"

// External assembly functions
extern void load_gdt(uintptr_t gdt_ptr);
extern void load_idt(uintptr_t idt_ptr);
extern void enable_paging(uintptr_t* pml4);

// Function prototypes
void init_gdt(void);
void init_idt(void);
void init_paging(void);
void isr_handler(void);

// Memory management
#define PAGE_SIZE 4096
#define ENTRIES_PER_TABLE 512

// Page tables
uintptr_t* pml4 = (uintptr_t*)0x1000000;
uintptr_t* pdpt = (uintptr_t*)0x1001000;
uintptr_t* pd = (uintptr_t*)0x1002000;
uintptr_t* pt = (uintptr_t*)0x1003000;

// Heap configuration
#define HEAP_START 0x2000000
#define HEAP_SIZE 0x1000000
uint8_t* heap_start = (uint8_t*)HEAP_START; 
uint8_t* heap_end = (uint8_t*)(HEAP_START + HEAP_SIZE);
uint8_t* heap_current = (uint8_t*)HEAP_START;

// Simple memory allocator
void* kmalloc(size_t size) {
    if (heap_current + size > heap_end) {
        return NULL;  // Out of memory
    }
    void* result = (void*)heap_current;
    heap_current += size;
    return result;
}

// Process management
#define MAX_PROCESSES 10
typedef struct { 
    uintptr_t rsp;
    uintptr_t cr3;
    bool active;
} Process;

Process processes[MAX_PROCESSES];
int current_process = -1;

// Initialize scheduler
void init_scheduler(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].active = false;
    }
}

// Create a new process
int create_process(void (*entry_point)(void)) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i].active) {
            processes[i].rsp = (uintptr_t)kmalloc(4096) + 4096;
            processes[i].cr3 = (uintptr_t)pml4;
            processes[i].active = true;
            
            uintptr_t* stack = (uintptr_t*)processes[i].rsp;
            *(--stack) = (uintptr_t)entry_point;
            *(--stack) = 0;  // Fake return address
            for (int j = 0; j < 7; j++) {  // Space for registers
                *(--stack) = 0;
            }
            processes[i].rsp = (uintptr_t)stack;
            
            return i;
        }
    }
    return -1;
}

// Schedule next process
void schedule(void) {
    if (current_process != -1) {
        asm volatile("mov %%rsp, %0" : "=r" (processes[current_process].rsp));
    }
    
    do {
        current_process = (current_process + 1) % MAX_PROCESSES;
    } while (!processes[current_process].active);
    
    asm volatile(
        "mov %0, %%cr3\n"
        "mov %1, %%rsp"
        : : "r" (processes[current_process].cr3), "r" (processes[current_process].rsp)
    );
}

// Example processes
void process1(void) {
    while(1) {
        terminal_writestring("Process 1 running\n");
        for (volatile int i = 0; i < 1000000; i++) {}
    }
}

void process2(void) {
    while(1) {
        terminal_writestring("Process 2 running\n");
        for (volatile int i = 0; i < 1000000; i++) {}
    }
}

// GDT structures
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

#define GDT_ENTRIES 5
struct gdt_entry gdt[GDT_ENTRIES];
struct gdt_ptr gp;

// IDT structures
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uintptr_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256
struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr ip;

// ISR function prototypes
extern void isr0(void);
extern void isr1(void);

// Set up a GDT entry
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access = access;
}

// Initialize GDT
void init_gdt(void) {
    gp.limit = (sizeof(struct gdt_entry) * GDT_ENTRIES) - 1;
    gp.base = (uintptr_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xAF); // Code segment
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xAF); // User mode code segment
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

    load_gdt((uintptr_t)&gp);
}

// Set up an IDT entry
void idt_set_gate(uint8_t num, uintptr_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_middle = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
    idt[num].ist = 0;
}

// Initialize IDT
void init_idt(void) {
    ip.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    ip.base = (uintptr_t)&idt;

    // Use memset from string.h or implement it separately
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_set_gate(0, (uintptr_t)isr0, 0x08, 0x8E); // Division by zero
    idt_set_gate(1, (uintptr_t)isr1, 0x08, 0x8E); // Debug exception

    load_idt((uintptr_t)&ip);
}

// Set up paging for 64-bit long mode
void init_paging(void) {
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        pt[i] = (i * PAGE_SIZE) | 3; // Present + Writable
    }
    pd[0] = (uintptr_t)pt | 3;
    pdpt[0] = (uintptr_t)pd | 3;
    pml4[0] = (uintptr_t)pdpt | 3;

    enable_paging(pml4);
}

// ISR handler
void isr_handler(void) {
    terminal_writestring("Interrupt received!\n");
}

// Kernel main function
void kernel_main(void) {
    init_vga();
    init_gdt();
    init_idt();
    init_paging();
    init_scheduler();
    
    terminal_writestring("Welcome to SimpleOS, there isn't much to do.\n");
    
    create_process(process1);
    create_process(process2);
    
    asm volatile("sti");
    
    while(1) {
        schedule();
        asm volatile("hlt");
    }
}