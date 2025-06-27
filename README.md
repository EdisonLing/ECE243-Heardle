# ECE243 Heardle
 
• Remade Heardle in C using the DE1-SoC FPGA board
• Interface with a PS/2 Keyboard; Built a raw C driver to read scancodes and buffer user guesses
• manage audio FIFOs, and play 16-bit PCM snippets</p>
• Connected the hardware timer to seed the C library’s RNG, ensuring a fresh a different experience each play</p>
• Drew polished title and menu screens pixel-by-pixel using the VGA pixel buffer, manage double-buffering for tear-free updates, and implement text-and-sprite routines in C.
