# SimpleOS: A Basic x86-64 Operating System Kernel

SimpleOS is a minimalistic operating system kernel designed for x86-64 architecture. It was built as a basic exploration of kernel architecture to serve as a foundation for more complex OS development.

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

## Ways to Use SimpleOS

1. **Educational Tool**
   - Learn about low-level OS development
   - Understand x86-64 architecture specifics

2. **Experimentation Platform**
   - Add new features to explore OS concepts
   - Test different scheduling algorithms or memory management techniques

3. **Customization**
   - Modify keyboard layouts or add support for different languages
   - Extend terminal capabilities for more advanced text-based interfaces

4. **Bootable Environment**
   - Create a minimalistic, bootable operating system
   - Use as a base for developing specialized, bare-metal applications

5. **Hardware Interaction**
   - Learn about direct hardware communication in a controlled environment
   - Experiment with different device drivers

6. **Performance Testing**
   - Benchmark low-level operations without OS overhead
   - Compare against other minimal OS implementations

7. **Security Research**
   - Explore low-level security concepts
   - Implement and test various protection mechanisms

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

2. Set up your bootloader and kernel on this image (refer to your bootloader documentation).

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

## Contributing

Contributions to SimpleOS are welcome! Please feel free to submit pull requests, report bugs, or suggest new features.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.