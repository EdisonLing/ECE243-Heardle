#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PS2_BASE 0xFF200100  // PS2 base address
#define RVALID_BIT 0x8000    // RVALID bit of PS2_Data register
#define PS2_DATA_BITS 0xFF   // data bits of PS2_Data register

#define HEX_3to0_BASE 0xFF200020
#define HEX_5to4_BASE 0xFF200030

typedef struct {
    char key, last_key, last_last_key;
} keyboard_keys_struct;

keyboard_keys_struct keyboard_keys;

void pollKeyboard() {
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int PS2_data = *(PS2_ptr);           // get value of PS2 data register
    int RVALID = PS2_data & RVALID_BIT;  // get only the RVALID bit
    if (RVALID != 0) {
        // shift values queue back and add newest keypress to front
        keyboard_keys.last_last_key = keyboard_keys.last_key;
        keyboard_keys.last_key = keyboard_keys.key;
        keyboard_keys.key = PS2_data & PS2_DATA_BITS;
        char keyPrintStatement[100] = "Keys: ";
        strcat(keyPrintStatement, keyboard_keys.key);
        strcat(keyPrintStatement, ", ");
        strcat(keyPrintStatement, keyboard_keys.last_key);
        strcat(keyPrintStatement, ", ");
        strcat(keyPrintStatement, keyboard_keys.last_last_key);
        strcat(keyPrintStatement, '\n');
    }
}

// void PS2_HEX_DISPLAY(keyboard_keys_struct keyboard_struct) {
//     volatile int *HEX_3to0_ptr = (int *)HEX_3to0_BASE;
//     volatile int *HEX_5to4_ptr = (int *)HEX_5to4_BASE;
//     //
//     unsigned char seven_seg_decode_table[] = {
//         0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
//         0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
// }

int main(void) {
    while (1) {
        pollKeyboard();
    }
    return 0;
}
