#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PS2_BASE 0xFF200100  // PS2 base address
#define RVALID_BIT 0x8000    // RVALID bit of PS2_Data register
#define PS2_DATA_BITS 0xFF   // data bits of PS2_Data register

// constants for PS2 code to actual key
#define KEY_1 0x16
#define KEY_2 0x1e
#define KEY_3 0x26
#define KEY_4 0x25
#define KEY_Q 0x15
#define KEY_SPACE 0x29
#define KEY_NULL 0xF0

#define HEX_3to0_BASE 0xFF200020
#define HEX_5to4_BASE 0xFF200030

typedef struct {
    char key, last_key, last_last_key;
} keyboard_keys_struct;

keyboard_keys_struct keyboard_keys;

char validKeysArr[] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_Q, KEY_SPACE, KEY_NULL};
int sizeOfValidKeysArr = sizeof(validKeysArr) / sizeof(validKeysArr[0]);

bool isValidKey(char key) {
    for (int i = 0; i < sizeOfValidKeysArr; i++) {
        if (key == validKeysArr[i]) return true;
    }
    return false;
}

void pollKeyboard() {
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int PS2_data = *(PS2_ptr);  // get value of PS2 data register
    // printf("%d\n", PS2_data);
    int RVALID = PS2_data & RVALID_BIT;  // get only the RVALID bit
    if (RVALID != 0) {
        // shift values queue back and add newest keypress to front
        keyboard_keys.last_last_key = keyboard_keys.last_key;
        keyboard_keys.last_key = keyboard_keys.key;
        keyboard_keys.key = PS2_data & PS2_DATA_BITS;
        // printf("Keys: %x, %x, %x\n", keyboard_keys.key, keyboard_keys.last_key, keyboard_keys.last_last_key);
        char key = 0x00, last_key = 0x00, last_last_key = 0x00;  // let FF be none
        if (isValidKey(keyboard_keys.key)) key = keyboard_keys.key;
        if (isValidKey(keyboard_keys.last_key)) last_key = keyboard_keys.last_key;
        if (isValidKey(keyboard_keys.last_last_key)) last_last_key = keyboard_keys.last_last_key;
        printf("Valid Keys: %x, %x, %x\n", key, last_key, last_last_key);
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
