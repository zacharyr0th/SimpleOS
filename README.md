# SimpleOS: A Basic x86-64 Operating System Kernel

SimpleOS is an operating system kernel designed for x86-64 architecture. 

It serves as a foundation for exploring kernel architecture and OS development.

## Table of Contents

1. [Features](#features)
2. [Kernel Architecture](#kernel-architecture)
   - [Memory Management](#1-memory-management)
   - [Process Management](#2-process-management)
   - [Global Descriptor Table (GDT)](#3-global-descriptor-table-gdt)
   - [Interrupt Descriptor Table (IDT)](#4-interrupt-descriptor-table-idt)
   - [Paging Initialization](#5-paging-initialization)
   - [Interrupt Service Routine (ISR) Handler](#6-interrupt-service-routine-isr-handler)
   - [Kernel Main Function](#7-kernel-main-function)
3. [Design Considerations](#design-considerations)
4. [Creating and Running Your OS Image](#creating-and-running-your-os-image)
5. [Advanced Testing Options](#advanced-testing-options)
6. [License](#license)

## Features

- **x86-64 Architecture Support**: Designed for 64-bit x86 systems with a custom linker script for memory layout.
- **Basic Kernel Structure**: Monolithic design with a defined entry point (`kernel_main` in `kernel.c`).
- **Interrupt Handling**: Custom ISR setup with the ability to register custom handlers.
- **Memory Management**: Implements paging and a simple heap allocator.
- **Process Management**: Supports basic multitasking with a round-robin scheduler.
- **Modular Code Structure**: Separated header and implementation files for clear organization.

### Interrupt Handling Example

```c
void init_keyboard() {
    // Example of registering an interrupt handler
    idt_set_gate(33, (uintptr_t)keyboard_handler, 0x08, 0x8E);
}
```

### Memory Management Example

```c
#define PAGE_SIZE 4096
#define ENTRIES_PER_TABLE 512

// Page tables
uintptr_t* pml4 = (uintptr_t*)0x1000000;
uintptr_t* pdpt = (uintptr_t*)0x1001000;
uintptr_t* pd = (uintptr_t*)0x1002000;
uintptr_t* pt = (uintptr_t*)0x1003000;
```

### Process Management Example

```c
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
```

## Kernel Architecture

### 1. Memory Management

#### Paging

- 4-level paging structure (PML4, PDPT, PD, PT)
- Page size: 4096 bytes
- Page tables initialized at specific addresses:
  - PML4: `0x1000000`
  - PDPT: `0x1001000`
  - PD: `0x1002000`
  - PT: `0x1003000`

#### Heap Management

- Heap start: `0x2000000`
- Heap size: `0x1000000` (16 MB)
- Simple bump allocator implemented:

```c
void* kmalloc(size_t size) {
    if (heap_current + size > heap_end) {
        return NULL;  // Out of memory
    }
    void* result = (void*)heap_current;
    heap_current += size;
    return result;
}
```

### 2. Process Management

- Supports up to 10 concurrent processes
- Process structure:

```c
typedef struct { 
    uintptr_t rsp;
    uintptr_t cr3;
    bool active;
} Process;
```

- Includes functions for:
  - Scheduler initialization
  - Process creation (`create_process`)
  - Simple round-robin scheduling (`schedule`)

### 3. Global Descriptor Table (GDT)

- 5 GDT entries:
  1. Null segment
  2. Code segment
  3. Data segment
  4. User mode code segment
  5. User mode data segment

- GDT structure:

```c
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));
```

- Initialization function: `init_gdt()`

### 4. Interrupt Descriptor Table (IDT)

- 256 IDT entries
- IDT entry structure:

```c
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));
```

- Initialization function: `init_idt()`
- Currently sets up handlers for:
  - Division by zero (ISR 0)
  - Debug exception (ISR 1)

### 5. Paging Initialization

- Function: `init_paging()`
- Sets up 4-level paging structure
- Maps first 2MB of physical memory to virtual memory
- Enables paging by loading PML4 address into CR3 register

```c
void init_paging(void) {
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        pt[i] = (i * PAGE_SIZE) | 3; // Present + Writable
    }
    pd[0] = (uintptr_t)pt | 3;
    pdpt[0] = (uintptr_t)pd | 3;
    pml4[0] = (uintptr_t)pdpt | 3;

    enable_paging(pml4);
}
```

### 6. Interrupt Service Routine (ISR) Handler

- Basic ISR handler that prints a message when an interrupt is received

```c
void isr_handler(void) {
    terminal_writestring("Interrupt received!\n");
}
```

### 7. Kernel Main Function

```c
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
```

- Initializes core kernel components
- Creates two example processes
- Enables interrupts
- Enters an infinite loop, scheduling processes and halting the CPU when idle

## Design Considerations

- **Quick and easy testing**: Rebuild your kernel, copy to the image, and rerun QEMU.
- **Safe**: No modifications to critical system files on your development machine.
- **Isolated**: Your OS runs in a separate environment from your host system.
- **Flexible**: QEMU provides many options for debugging and system configuration.
- **Semi-ephemeral**: The image file persists, but changes can be easily reverted.

## Creating and Running Your OS Image

1. Create a disk image:

   ```bash
   dd if=/dev/zero of=myos.img bs=1M count=64
   ```

2. Set up your bootloader and kernel on this image.

3. Use QEMU to run your OS:

   ```bash
   qemu-system-x86_64 -drive format=raw,file=myos.img
   ```

## Advanced Testing Options

1. Use snapshot mode to discard changes after each run:

   ```bash
   qemu-system-x86_64 -drive format=raw,file=myos.img -snapshot
   ```

2. Enable GDB support for debugging:

   ```bash
   qemu-system-x86_64 -s -S -drive format=raw,file=myos.img
   ```

   Connect with GDB in another terminal:

   ```bash
   
   gdb
   (gdb) target remote localhost:1234
   ```

3. Redirect QEMU's serial output for clear console output:

   ```bash
   qemu-system-x86_64 -drive format=raw,file=myos.img -serial stdio
   ```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
