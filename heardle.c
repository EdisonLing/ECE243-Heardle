#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LEDR_BASE 0xFF200000

#define PIXEL_BUFFER_BASE 0xFF203020

volatile int pixel_buffer_start;  // global variable
short int Buffer1[240][512];      // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

#define CHARACTER_BUFFER_BASE 0x09000000  // base for drawing characters

#define PS2_BASE 0xFF200100  // PS2 base address
#define RVALID_BIT 0x8000    // RVALID bit of PS2_Data register
#define PS2_DATA_BITS 0xFF   // data bits of PS2_Data register

#define SCREEN_WIDTH_CHARS 80
#define SCREEN_HEIGHT_CHARS 60

#define ANSWERS_COL_1 10  // x coord 1
#define ANSWERS_COL_2 50  // x coord 2
#define ANSWERS_ROW_1 40  // y coord 1
#define ANSWERS_ROW_2 50  // y coord 2

#define TOTAL_ROUNDS 5  // total number of rounds
#define MAX_SONG_LENGTH 40

const char *Songs[] = {
    "Stronger",
    "Power",
    "Heartless",
    "Gold Digger",
    "Flashing Lights",
    "All of the Lights",
    "Runaway",
    "Bound 2",
    "Black Skinhead",
    "Famous",
    "Ultralight Beam",
    "Ghost Town",
    "I Wonder",
    "Can't Tell Me Nothing",
    "Touch the Sky",
    "Jesus Walks",
    "Homecoming",
    "Love Lockdown",
    "Good Morning",
    "Father Stretch My Hands Pt. 1"};

int numSongs = 20;

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

// valid keys for pollKeyboard
char validKeysArr[] = {
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_Q,
    KEY_SPACE,
    KEY_NULL,
    KEY_ENTER,
    KEY_RIGHT_ARROW,
    KEY_W};
int sizeOfValidKeysArr = sizeof(validKeysArr) / sizeof(validKeysArr[0]);  // dynamically calculates size from array

/*
GLOBAL VARIABLES
GLOBAL VARIABLES
GLOBAL VARIABLES
*/
int PlayerScore = 0;      // total score
int RoundDifficulty = 1;  // the number of seconds the music will play for, inversely proportional to amount of points gained on correct answer
int Round = 1;
int SelectedAnswer = -1;
char currentAnswers[4][MAX_SONG_LENGTH] = {

};
bool doneGame = false;

/*
GLOBAL VARIABLES
GLOBAL VARIABLES
GLOBAL VARIABLES
*/

/*
FUNCTION PROTOTYPES
FUNCTION PROTOTYPES
FUNCTION PROTOTYPES
*/
void clearScreen();
bool isValidKey(char key);
bool checkAnswer(char answer[MAX_SONG_LENGTH]);
bool submitAnswer(int answer_index);
void pollKeyboard();
void drawCharacter();
void clearCharacterBuffer();
void writeCharacter(char character, int x, int y);
void drawStartScreen();
void seedRandom();
int randomInRange(int min, int max);
void loadCurrentAnswers();
void wait_for_vsync();
void writeRoundAndScore(bool endScreen);
int calculateCenterText_Y(char str[]);
void drawEndScreen();

/*
FUNCTION PROTOTYPES
FUNCTION PROTOTYPES
FUNCTION PROTOTYPES
*/

typedef struct {
    char key, last_key, last_last_key;
} keyboard_keys_struct;

keyboard_keys_struct keyboard_keys = {KEY_NULL, KEY_NULL, KEY_NULL};

int main(void) {
    volatile int *pixel_ctrl_ptr = (int *)PIXEL_BUFFER_BASE;
    /* set front pixel buffer to Buffer 1 */
    *(pixel_ctrl_ptr + 1) = (int)&Buffer1;  // first store the address in the  back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clearScreen();  // pixel_buffer_start points to the pixel buffer

    /* set back pixel buffer to Buffer 2 */
    *(pixel_ctrl_ptr + 1) = (int)&Buffer2;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1);  // we draw on the back buffer
    clearScreen();                               // pixel_buffer_start points to the pixel buffer
    clearCharacterBuffer();

    /*
    DRAW THE START SCREEN
    */
    drawStartScreen();
    /*
    WAIT FOR USER INPUT TO PROCEED
    */
    while (!(keyboard_keys.key == KEY_SPACE && keyboard_keys.last_last_key == KEY_NULL)) {
        pollKeyboard();
    }
    clearScreen();
    clearCharacterBuffer();
    writeRoundAndScore(0);
    loadCurrentAnswers();
    writeAnswersAndSelected();
    while (!doneGame) {  // while game is going on
        pollKeyboard();

        wait_for_vsync();
        pixel_buffer_start = *(pixel_ctrl_ptr + 1);  // new back buffer
    }

    clearCharacterBuffer();
    drawEndScreen();
    while (1) {
    }

    // done game screen
    return 0;
}

// seed the RNG
void seedRandom() {
    srand(time(NULL));
}
// return random number in range inclusive
int randomInRange(int min, int max) {
    if (min > max) {
        // Swap if min > max
        int temp = min;
        min = max;
        max = temp;
    }
    return rand() % (max - min + 1) + min;
}
void loadCurrentAnswers() {  // take current correct answer from index of round, and take 3 random songs from after the index
    int a = 0, b = 0, c = 0, d = 0;
    int temp;
    for (int i = 0; i < 4; i++) {
        temp = randomInRange(Round, numSongs - 1);  // index between item after right answer and end of song array
        while (temp == a || temp == b || temp == c || temp == d) {
            temp = randomInRange(Round, numSongs - 1);
        }
        // there is 100% a better way to do this function
        if (i == 0) a = temp;
        else if (i == 1) b = temp;
        else if (i == 2) c = temp;
        else if (i == 3) d = temp;
        strcpy(currentAnswers[i], Songs[temp]);  // COPY "RANDOM" SONG INTO CURRENT ANSWERS
    }
    int i = randomInRange(0, 3);
    strcpy(currentAnswers[i], Songs[Round - 1]);
    printf("Answers: %s, %s, %s, %s\n", currentAnswers[0], currentAnswers[1], currentAnswers[2], currentAnswers[3]);
}

void writeCharacter(char character, int x, int y) {
    // pointer to character buffer
    volatile char *char_buffer = (char *)CHARACTER_BUFFER_BASE;

    // format word
    int offset = (y << 7) + x;

    // write word into memory
    char_buffer[offset] = character;
    return;
}

int getStringSize(char *word) {
    int size = 0;
    while (word[size] != '\0') {
        size++;
    }
    // printf("'%s' is %d characters long\n", word, size);
    return size;
}

void writeWord(char *word, int x0, int y0) {
    int size = getStringSize(word);
    // printf("writing word at %d, %d\n", x0, y0);
    for (int i = 0; i < size; i++) {
        writeCharacter(word[i], i + x0, y0);
    }
}

void clearCharacterBuffer() {
    for (int x = 0; x < 80; x++) {
        for (int y = 0; y < 60; y++) {
            writeCharacter(' ', x, y);
        }
    }
}

void plotPixel(int x, int y, short int line_color) {
    // Make sure we're within bounds
    if (x < 0 || x >= 320 || y < 0 || y >= 240)
        return;

    volatile short int *one_pixel_address;
    one_pixel_address = (volatile short int *)(pixel_buffer_start + (y << 10) + (x << 1));
    *one_pixel_address = line_color;
}

void clearScreen() {  // loops through each pixel and sets to black
    // printf("started clearing screen\n");
    for (int x = 0; x < 320; x++) {
        for (int y = 0; y < 240; y++) {
            plotPixel(x, y, 0x0);  // 0xFFFFFF
        }
    }
}

// WIP
// helpers to find x and y coords to center text
int calculateCenterText_X(char str[]) {
    int size = getStringSize(str);
    return (SCREEN_WIDTH_CHARS - size) / 2;
}
int calculateCenterText_Y(char str[]) {
    // int size = getStringSize(str);
    return SCREEN_HEIGHT_CHARS / 2;
}

void drawStartScreen() {  // clears screen then draws title text
    char str[] = "HEARDLE";
    writeWord(str, 20, 40);
}

void drawEndScreen() {
    char str[MAX_SONG_LENGTH];
    if (PlayerScore >= 10 * TOTAL_ROUNDS) {  // perfect score
        strcpy(str, "PERFECT!!!!!");

    } else if (PlayerScore >= 9 * TOTAL_ROUNDS) {  // almost perfect
        strcpy(str, "amazing job!");

    } else if (PlayerScore >= 7 * TOTAL_ROUNDS) {  // good
        strcpy(str, "you actually did pretty good");

    } else if (PlayerScore >= 4 * TOTAL_ROUNDS) {  // decent
        strcpy(str, "at least you got some right i guess...");

    } else if (PlayerScore >= 2 * TOTAL_ROUNDS) {  // bad
        strcpy(str, "ermm... do you even listen to music?");

    } else {  // horrible
        strcpy(str, "dude, you suck at this...");
    }
    writeWord(str, calculateCenterText_X(str), calculateCenterText_Y(str));
    writeRoundAndScore(1);
}

bool isValidKey(char key) {
    for (int i = 0; i < sizeOfValidKeysArr; i++) {
        if (key == validKeysArr[i]) return true;
    }
    return false;
}

bool checkAnswer(char answer[MAX_SONG_LENGTH]) {             // temp, will return whether or not user's answer is correct
    if (strcmp(answer, Songs[Round - 1]) == 0) return true;  // if argument is same as round's answer, return true
    return false;
}

bool submitAnswer(int answer_index) {  // returns true if answer is right, false if answer is wrong, resets roundDifficulty to 1
    bool correct = checkAnswer(currentAnswers[answer_index]);
    if (correct) {
        PlayerScore += 10 - 2 * (RoundDifficulty - 1);
    }
    RoundDifficulty = 1;
    SelectedAnswer = -1;
    if (Round >= TOTAL_ROUNDS) {
        doneGame = true;
    } else {
        Round++;
        loadCurrentAnswers();
    }
    return correct;
}
/*

pollKeyboard checks for PS2 keyboard input:
    if W, increase difficulty
    if enter, call submitAnswer -> needs parameter change
*/
void writeRoundAndScore(bool endScreen) {
    // display round number
    char CurrentRoundStr[20];
    strcpy(CurrentRoundStr, "Round: ");
    writeWord(CurrentRoundStr, 1, 1);
    sprintf(CurrentRoundStr, "%d", Round);
    writeWord(CurrentRoundStr, 8, 1);
    strcpy(CurrentRoundStr, "/");
    writeWord(CurrentRoundStr, 10, 1);
    sprintf(CurrentRoundStr, "%d", TOTAL_ROUNDS);
    if (TOTAL_ROUNDS < 10) writeWord(CurrentRoundStr, 12, 1);
    else writeWord(CurrentRoundStr, 11, 1);
    if (!endScreen) {  // normal
        // display score
        char PlayerScoreStr[10];
        sprintf(PlayerScoreStr, "%d", PlayerScore);
        char score_label_str[] = "Score: ";
        writeWord(score_label_str, 1, 2);
        writeWord(PlayerScoreStr, 8, 2);
    } else {  // end screen, move score
        // display score
        char PlayerScoreStr[10];
        sprintf(PlayerScoreStr, "%d", PlayerScore);
        char score_label_str[] = "Final Score: ";
        writeWord(score_label_str, calculateCenterText_X(score_label_str) - 4, 10);
        writeWord(PlayerScoreStr, calculateCenterText_X(score_label_str) + getStringSize(score_label_str), 10);
    }
}

void clearHighlightedAnswers() {
    char clear[2] = "  ";
    writeWord(clear, ANSWERS_COL_1 - 6, ANSWERS_ROW_1);
    writeWord(clear, ANSWERS_COL_1 + getStringSize(currentAnswers[0]), ANSWERS_ROW_1);

    writeWord(clear, ANSWERS_COL_2 - 6, ANSWERS_ROW_1);
    writeWord(clear, ANSWERS_COL_2 + getStringSize(currentAnswers[1]), ANSWERS_ROW_1);

    writeWord(clear, ANSWERS_COL_1 - 6, ANSWERS_ROW_2);
    writeWord(clear, ANSWERS_COL_1 + getStringSize(currentAnswers[2]), ANSWERS_ROW_2);

    writeWord(clear, ANSWERS_COL_2 - 6, ANSWERS_ROW_2);
    writeWord(clear, ANSWERS_COL_2 + getStringSize(currentAnswers[3]), ANSWERS_ROW_2);
}

void writeAnswersAndSelected() {
    // clear prev
    for (int i = 0; i < 80; i++) {
        writeCharacter(' ', i, ANSWERS_ROW_1);
    }
    for (int i = 0; i < 80; i++) {
        writeCharacter(' ', i, ANSWERS_ROW_2);
    }

    // write 1
    writeWord(currentAnswers[0], ANSWERS_COL_1, ANSWERS_ROW_1);
    // write 2
    writeWord(currentAnswers[1], ANSWERS_COL_2, ANSWERS_ROW_1);
    // write 3
    writeWord(currentAnswers[2], ANSWERS_COL_1, ANSWERS_ROW_2);
    // write 4
    writeWord(currentAnswers[3], ANSWERS_COL_2, ANSWERS_ROW_2);

    char label[3];
    // write label 1
    strcpy(label, "[1]");
    writeWord(label, ANSWERS_COL_1 - 4, ANSWERS_ROW_1);
    // write label 2
    strcpy(label, "[2]");
    writeWord(label, ANSWERS_COL_2 - 4, ANSWERS_ROW_1);
    // write label 3
    strcpy(label, "[3]");
    writeWord(label, ANSWERS_COL_1 - 4, ANSWERS_ROW_2);
    // write label 4
    strcpy(label, "[4]");
    writeWord(label, ANSWERS_COL_2 - 4, ANSWERS_ROW_2);

    // highlight selected
    char highlight_front[3] = "<<";
    char highlight_end[3] = ">>";
    if (SelectedAnswer == 1) {
        writeWord(highlight_front, ANSWERS_COL_1 - 6, ANSWERS_ROW_1);
        writeWord(highlight_end, ANSWERS_COL_1 + getStringSize(currentAnswers[0]), ANSWERS_ROW_1);
    } else if (SelectedAnswer == 2) {
        writeWord(highlight_front, ANSWERS_COL_2 - 6, ANSWERS_ROW_1);
        writeWord(highlight_end, ANSWERS_COL_2 + getStringSize(currentAnswers[1]), ANSWERS_ROW_1);
    } else if (SelectedAnswer == 3) {
        writeWord(highlight_front, ANSWERS_COL_1 - 6, ANSWERS_ROW_2);
        writeWord(highlight_end, ANSWERS_COL_1 + getStringSize(currentAnswers[2]), ANSWERS_ROW_2);
    } else if (SelectedAnswer == 4) {
        writeWord(highlight_front, ANSWERS_COL_2 - 6, ANSWERS_ROW_2);
        writeWord(highlight_end, ANSWERS_COL_2 + getStringSize(currentAnswers[3]), ANSWERS_ROW_2);
    }
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

        if ((keyboard_keys.key == KEY_W && keyboard_keys.last_last_key == KEY_NULL)) {  // IF W IS PRESSED (decrease current round difficulty)
            if (RoundDifficulty < 5) RoundDifficulty++;
            // else do nothing
            // printf("Round Difficulty: %d\n", RoundDifficulty);
        } else if (keyboard_keys.key == KEY_ENTER && keyboard_keys.last_last_key == KEY_NULL && SelectedAnswer != -1) {  // IF ENTER IS PRESSED (submit answer / next round)
            submitAnswer(SelectedAnswer - 1);
            writeAnswersAndSelected();
            // printf("New Score: %d\n", PlayerScore);

            writeRoundAndScore(0);
        } else if (keyboard_keys.key == KEY_1 && keyboard_keys.last_last_key == KEY_NULL) {
            SelectedAnswer = 1;
            writeAnswersAndSelected();
        } else if (keyboard_keys.key == KEY_2 && keyboard_keys.last_last_key == KEY_NULL) {
            SelectedAnswer = 2;
            writeAnswersAndSelected();
        } else if (keyboard_keys.key == KEY_3 && keyboard_keys.last_last_key == KEY_NULL) {
            SelectedAnswer = 3;
            writeAnswersAndSelected();
        } else if (keyboard_keys.key == KEY_4 && keyboard_keys.last_last_key == KEY_NULL) {
            SelectedAnswer = 4;
            writeAnswersAndSelected();
        }
    }
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

void drawLine(int x0, int y0, int x1, int y1, short int colour) {
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);

    int error = -(deltax / 2);
    int y = y0;

    int y_step;
    if (y0 < y1) y_step = 1;
    else y_step = -1;

    for (int x = x0; x < x1; x++) {
        if (is_steep) plotPixel(y, x, colour);
        else plotPixel(x, y, colour);
        error += deltay;
        if (error > 0) {
            y += y_step;
            error -= deltax;
        }
    }
}

void wait_for_vsync() {
    volatile int *fbuf = (int *)0xFF203020;  // base of VGA controller
    int status;
    *fbuf = 1;             // enable swap
    status = *(fbuf + 3);  // read status register
    while (status & 1) {   // wait until we poll status = 0
        status = *(fbuf + 3);
    }
}
