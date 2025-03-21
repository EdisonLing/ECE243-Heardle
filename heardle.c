#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEDR_BASE 0xFF200000

#define PS2_BASE 0xFF200100  // PS2 base address
#define RVALID_BIT 0x8000    // RVALID bit of PS2_Data register
#define PS2_DATA_BITS 0xFF   // data bits of PS2_Data register

// constants for PS2 code to actual key
#define KEY_1 0x16
#define KEY_2 0x1E
#define KEY_3 0x26
#define KEY_4 0x25
#define KEY_Q 0x15
#define KEY_SPACE 0x29
#define KEY_NULL 0xF0
#define KEY_ENTER 0x5A
#define KEY_NOT_VALID 0x00
#define KEY_RIGHT_ARROW 0x74
#define KEY_W 0x1D

#define HEX_3to0_BASE 0xFF200020
#define HEX_5to4_BASE 0xFF200030

typedef struct {
    char key, last_key, last_last_key;
} keyboard_keys_struct;

keyboard_keys_struct keyboard_keys = {KEY_NULL, KEY_NULL, KEY_NULL};

char validKeysArr[] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_Q, KEY_SPACE, KEY_NULL, KEY_ENTER, KEY_RIGHT_ARROW, KEY_W};
int sizeOfValidKeysArr = sizeof(validKeysArr) / sizeof(validKeysArr[0]);

bool isValidKey(char key) {
    for (int i = 0; i < sizeOfValidKeysArr; i++) {
        if (key == validKeysArr[i]) return true;
    }
    return false;
}

int PlayerScore = 0;      // total score
int RoundDifficulty = 1;  // the number of seconds the music will play for, inversely proportional to amount of points gained on correct answer

bool checkAnswer(int answer_index, int round_index) {  // temp, will return whether or not user's answer is correct
    return true;
}

bool submitAnswer(int answer_index, int round_index) {  // returns true if answer is right, false if answer is wrong, resets roundDifficulty to 1
    int prev_round_difficulty = RoundDifficulty;
    RoundDifficulty = 1;
    if (checkAnswer(answer_index, round_index)) {
        PlayerScore += 10 - 2 * (prev_round_difficulty - 1);
        return true;
    } else return false;
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
        char temp_key = PS2_data & PS2_DATA_BITS;
        // if key is valid, add to structure, if not add "not valid" character instead
        // printf("%x\n", temp_key);
        if (isValidKey(temp_key)) keyboard_keys.key = temp_key;
        else keyboard_keys.key = KEY_NOT_VALID;

        if ((keyboard_keys.key == KEY_W && keyboard_keys.last_last_key == KEY_NULL)) {
            if (RoundDifficulty < 5) RoundDifficulty++;
            // else do nothing
            printf("Round Difficulty: %d\n", RoundDifficulty);
        } else if (keyboard_keys.key == KEY_ENTER && keyboard_keys.last_last_key == KEY_NULL) {
            submitAnswer(0, 0);
            printf("New Score: %d\n", PlayerScore);
        }

        printf("Keys: %x, %x, %x\n", keyboard_keys.key, keyboard_keys.last_key, keyboard_keys.last_last_key);
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
    volatile int *LEDR = LEDR_BASE;
    while (1) {
        pollKeyboard();
        LEDR = PlayerScore;
    }
    return 0;
}
