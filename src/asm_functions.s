# This assembly file contains low-level functions for x86_64 system initialization and interrupt handling.
# It includes routines for loading GDT and IDT, enabling paging, and handling interrupt service routines (ISRs).

.global load_gdt
.global load_idt
.global enable_paging
.global isr0
.global isr1

load_gdt:
    lgdt (%rdi)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    pushq $0x08
    pushq $.reload_cs
    retfq
.reload_cs:
    ret

load_idt:
    lidt (%rdi)
    ret

enable_paging:
    mov %rdi, %cr3
    mov %cr0, %rax
    mov $0x80000001, %rbx
    or %rbx, %rax
    mov %rax, %cr0
    ret

isr0:
    cli
    pushq $0
    pushq $0
    jmp isr_common_stub

isr1:
    cli
    pushq $0
    pushq $1
    jmp isr_common_stub

isr_common_stub:
    # Save registers
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi

    # Call C handler (you need to implement this)
    call isr_handler

    # Restore registers
    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax

    # Clean up the stack
    addq $16, %rsp
    sti
    iretq
