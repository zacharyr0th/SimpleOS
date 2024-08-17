# This Makefile automates the build process for SimpleOS.
# It defines compilation rules for C and assembly files, linking, and cleaning up built files.

# SimpleOS/Makefile

# Compiler and assembler
CC = x86_64-elf-gcc
AS = x86_64-elf-as
# Compiler flags
CFLAGS = -ffreestanding -c -m64 -O2 -Wall -Wextra -I./include

# Linker flags
LDFLAGS = -nostdlib -T linker.ld -Wl,--no-warn-rwx-segments -Wl,--no-warn-execstack -Wl,--verbose

# Source files
SRC = src/kernel.c src/terminal.c
ASM_SRC = src/asm_functions.s

# Object files
OBJ = $(SRC:.c=.o) $(ASM_SRC:.s=.o)

# Output file
OUTPUT = kernel.bin

# Default target
all: $(OUTPUT)

# Link object files to create the kernel binary
$(OUTPUT): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Assemble assembly files
%.o: %.s
	$(AS) $< -o $@

# Clean up built files
clean:
	rm -f $(OBJ) $(OUTPUT)

# Phony targets
.PHONY: all clean