# SimpleOS: A Basic x86-64 Operating System Kernel

SimpleOS is an operating system kernel designed for x86-64 architecture. It was built as a basic exploration of kernel architecture to serve as a foundation for more complex OS development.

## Features

1. **x86-64 Architecture Support**
   - Designed for 64-bit x86 systems
   - Uses a custom linker script for memory layout

2. **Basic Kernel Structure**
   - Monolithic design. Future iterations will be modularized
   - Entry point defined as `kernel_main` in `kernel.c`

3. **Interrupt Handling**
   - Custom interrupt service routine (ISR) setup
   - Ability to register custom interrupt handlers

4. **Keyboard Input**
   - US keyboard layout support
   - Interrupt-driven keyboard input handling
   - Real-time character display as keys are pressed

5. **Low-level I/O Operations**
   - Direct port I/O using inline assembly
   - Functions for reading from and writing to I/O ports

6. **Simple Terminal Output**
   - Basic string output functionality
   - Foundation for more advanced text-mode display

7. **Memory Management**
   - Basic memory sections defined in linker script (.text, .rodata, .data, .bss)

8. **Modular Code Structure**
   - Separated header and implementation files
   - Clear separation of concerns (e.g., keyboard, ports, terminal)

## Considerations that were made

* Quick and easy testing: Rebuild your kernel, copy to the image, and rerun QEMU.
* Safe: No modifications to critical system files on your development machine.
* Isolated: Your OS runs in a separate environment from your host system.
* Flexible: QEMU provides many options for debugging and system configuration.
* Semi-ephemeral: The image file persists, but changes can be easily reverted.

## Creating and Running Your OS Image

1. Create a disk image:

   ```bash
   dd if=/dev/zero of=myos.img bs=1M count=64
   ```

2. Set up your bootloader and kernel on this image.

3. Use QEMU (Quick Emulator - a free and open-source emulator for x86 and x86_64) to run your OS:

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
