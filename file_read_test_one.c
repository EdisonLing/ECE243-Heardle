#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_jtag_uart_regs.h"
// pre sure this is nios II specific

// Define the JTAG UART base address
#define JTAG_UART_BASE JTAG_UART_0_BASE

// Define maximum number of highscores to store
#define MAX_SCORES 10
#define MAX_NAME_LENGTH 20

// Structure to hold a single highscore entry
typedef struct {
    char name[MAX_NAME_LENGTH];
    int score;
} HighscoreEntry;

// Function prototypes
void init_jtag_uart();
int jtag_uart_read_char();
void jtag_uart_write_char(char c);
void jtag_uart_write_string(const char* str);
int read_highscores(HighscoreEntry scores[], int max_entries);
void write_highscores(HighscoreEntry scores[], int num_entries);
void add_highscore(HighscoreEntry scores[], int* num_entries, const char* name, int score);
void display_highscores(HighscoreEntry scores[], int num_entries);
void wait_for_connection();

int main() {
    HighscoreEntry highscores[MAX_SCORES];
    int num_scores = 0;
    char name[MAX_NAME_LENGTH];
    int score, i;
    char input[100];
    char c;
    
    // Initialize JTAG UART
    init_jtag_uart();
    
    // Wait for host connection
    wait_for_connection();
    
    // Read existing highscores from file
    num_scores = read_highscores(highscores, MAX_SCORES);
    
    while (1) {
        // Display menu
        jtag_uart_write_string("\r\n==== Highscore Management System ====\r\n");
        jtag_uart_write_string("1. View Highscores\r\n");
        jtag_uart_write_string("2. Add New Highscore\r\n");
        jtag_uart_write_string("3. Reset Highscores\r\n");
        jtag_uart_write_string("4. Exit\r\n");
        jtag_uart_write_string("Enter your choice: ");
        
        // Read choice
        i = 0;
        while ((c = jtag_uart_read_char()) != '\r' && c != '\n') {
            input[i++] = c;
            jtag_uart_write_char(c); // Echo character back to host
        }
        input[i] = '\0';
        jtag_uart_write_string("\r\n");
        
        switch (input[0]) {
            case '1':
                // View highscores
                display_highscores(highscores, num_scores);
                break;
                
            case '2':
                // Add new highscore
                jtag_uart_write_string("Enter player name: ");
                i = 0;
                while ((c = jtag_uart_read_char()) != '\r' && c != '\n') {
                    name[i++] = c;
                    jtag_uart_write_char(c); // Echo character back to host
                    if (i >= MAX_NAME_LENGTH - 1) break;
                }
                name[i] = '\0';
                jtag_uart_write_string("\r\n");
                
                jtag_uart_write_string("Enter score: ");
                i = 0;
                while ((c = jtag_uart_read_char()) != '\r' && c != '\n') {
                    input[i++] = c;
                    jtag_uart_write_char(c); // Echo character back to host
                }
                input[i] = '\0';
                jtag_uart_write_string("\r\n");
                
                score = atoi(input);
                add_highscore(highscores, &num_scores, name, score);
                write_highscores(highscores, num_scores);
                jtag_uart_write_string("Highscore added successfully!\r\n");
                break;
                
            case '3':
                // Reset highscores
                num_scores = 0;
                write_highscores(highscores, num_scores);
                jtag_uart_write_string("Highscores reset successfully!\r\n");
                break;
                
            case '4':
                // Exit
                jtag_uart_write_string("Exiting...\r\n");
                return 0;
                
            default:
                jtag_uart_write_string("Invalid choice. Please try again.\r\n");
        }
    }
    
    return 0;
}

// Initialize the JTAG UART
void init_jtag_uart() {
    // Enable both read and write interrupts
    IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_BASE, 
        ALTERA_AVALON_JTAG_UART_CONTROL_RE_MSK | 
        ALTERA_AVALON_JTAG_UART_CONTROL_WE_MSK);
}

// Wait until we have a connection to the host
void wait_for_connection() {
    int connected = 0;
    
    jtag_uart_write_string("Waiting for host connection...\r\n");
    
    while (!connected) {
        // Check if AC bit is set, indicating host connection
        if (IORD_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_BASE) & 
            ALTERA_AVALON_JTAG_UART_CONTROL_AC_MSK) {
            connected = 1;
            
            // Clear the AC bit by writing 1 to it
            IOWR_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_BASE, 
                IORD_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_BASE) | 
                ALTERA_AVALON_JTAG_UART_CONTROL_AC_MSK);
        }
    }
    
    jtag_uart_write_string("Host connected!\r\n");
}

// Read a character from the JTAG UART
int jtag_uart_read_char() {
    unsigned int data;
    
    // Wait until there's data available
    while (1) {
        data = IORD_ALTERA_AVALON_JTAG_UART_DATA(JTAG_UART_BASE);
        if (data & ALTERA_AVALON_JTAG_UART_DATA_RVALID_MSK) {
            // Return the character
            return data & 0xFF;
        }
    }
}

// Write a character to the JTAG UART
void jtag_uart_write_char(char c) {
    unsigned int status;
    
    // Wait until there's space in the transmit FIFO
    while (1) {
        status = IORD_ALTERA_AVALON_JTAG_UART_CONTROL(JTAG_UART_BASE);
        if ((status & ALTERA_AVALON_JTAG_UART_CONTROL_WSPACE_MSK) > 0) {
            break;
        }
    }
    
    // Write the character
    IOWR_ALTERA_AVALON_JTAG_UART_DATA(JTAG_UART_BASE, c);
}

// Write a string to the JTAG UART
void jtag_uart_write_string(const char* str) {
    while (*str) {
        jtag_uart_write_char(*str++);
    }
}

// Read highscores from a file
int read_highscores(HighscoreEntry scores[], int max_entries) {
    int num_entries = 0;
    char filename[] = "highscores.txt";
    char buffer[100];
    int i = 0;
    
    // Notify user that we're attempting to read highscores
    jtag_uart_write_string("Reading highscores...\r\n");
    
    // Send command to host to read the file
    jtag_uart_write_string("FILE_READ:");
    jtag_uart_write_string(filename);
    jtag_uart_write_string("\r\n");
    
    // Wait for response
    i = 0;
    while (i < 99) {
        char c = jtag_uart_read_char();
        if (c == '\r' || c == '\n') {
            if (i > 0) {
                buffer[i] = '\0';
                
                // Check if we've reached the end of file marker
                if (strcmp(buffer, "END_OF_FILE") == 0) {
                    break;
                }
                
                // Parse the line: format should be "name,score"
                char* token = strtok(buffer, ",");
                if (token != NULL) {
                    strcpy(scores[num_entries].name, token);
                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        scores[num_entries].score = atoi(token);
                        num_entries++;
                        if (num_entries >= max_entries) break;
                    }
                }
                
                i = 0;
            }
        } else {
            buffer[i++] = c;
        }
    }
    
    jtag_uart_write_string("Loaded ");
    sprintf(buffer, "%d", num_entries);
    jtag_uart_write_string(buffer);
    jtag_uart_write_string(" highscores\r\n");
    
    return num_entries;
}

// Write highscores to a file
void write_highscores(HighscoreEntry scores[], int num_entries) {
    char filename[] = "highscores.txt";
    char buffer[100];
    int i;
    
    // Notify user that we're writing highscores
    jtag_uart_write_string("Writing highscores...\r\n");
    
    // Send command to host to write the file
    jtag_uart_write_string("FILE_WRITE:");
    jtag_uart_write_string(filename);
    jtag_uart_write_string("\r\n");
    
    // Send each highscore entry
    for (i = 0; i < num_entries; i++) {
        sprintf(buffer, "%s,%d\r\n", scores[i].name, scores[i].score);
        jtag_uart_write_string(buffer);
    }
    
    // Send end of file marker
    jtag_uart_write_string("END_OF_FILE\r\n");
    
    // Wait for acknowledgment
    while (1) {
        i = 0;
        while (i < 99) {
            char c = jtag_uart_read_char();
            if (c == '\r' || c == '\n') {
                if (i > 0) {
                    buffer[i] = '\0';
                    if (strcmp(buffer, "FILE_WRITE_OK") == 0) {
                        jtag_uart_write_string("Highscores saved successfully\r\n");
                        return;
                    } else if (strcmp(buffer, "FILE_WRITE_ERROR") == 0) {
                        jtag_uart_write_string("Error saving highscores\r\n");
                        return;
                    }
                }
                i = 0;
            } else {
                buffer[i++] = c;
            }
        }
    }
}

// Add a new highscore
void add_highscore(HighscoreEntry scores[], int* num_entries, const char* name, int score) {
    int i, j;
    
    // Find the position to insert the new score (scores are sorted in descending order)
    i = 0;
    while (i < *num_entries && scores[i].score > score) {
        i++;
    }
    
    // If the list is full and the new score is lower than all existing scores, do nothing
    if (*num_entries >= MAX_SCORES && i >= MAX_SCORES) {
        return;
    }
    
    // Make room for the new score
    if (*num_entries < MAX_SCORES) {
        (*num_entries)++;
    }
    
    for (j = *num_entries - 1; j > i; j--) {
        strcpy(scores[j].name, scores[j-1].name);
        scores[j].score = scores[j-1].score;
    }
    
    // Insert the new score
    strcpy(scores[i].name, name);
    scores[i].score = score;
}

// Display highscores
void display_highscores(HighscoreEntry scores[], int num_entries) {
    int i;
    char buffer[100];
    
    jtag_uart_write_string("\r\n===== HIGH SCORES =====\r\n");
    jtag_uart_write_string("Rank  Name                 Score\r\n");
    jtag_uart_write_string("----------------------------------\r\n");
    
    for (i = 0; i < num_entries; i++) {
        sprintf(buffer, "%-5d %-20s %d\r\n", i+1, scores[i].name, scores[i].score);
        jtag_uart_write_string(buffer);
    }
    
    if (num_entries == 0) {
        jtag_uart_write_string("No highscores yet!\r\n");
    }
    
    jtag_uart_write_string("=======================\r\n");
}