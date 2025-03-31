#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// this is claude; no sd card -> breakdown on danielzj05

// JTAG UART base address from the documentation
#define JTAG_UART_BASE 0xFF201000

// Define offsets for the JTAG UART registers
#define DATA_REG_OFFSET 0x0        // Data register offset (0xFF201000)
#define CONTROL_REG_OFFSET 0x4     // Control register offset (0xFF201004)

// Bit masks for the DATA register
#define RVALID_BIT 0x00008000      // Bit 15 (RVALID)
#define DATA_MASK 0x000000FF       // Bits 7-0 (DATA)

// Bit masks for the CONTROL register
#define RAVAIL_MASK 0xFFFF0000     // Bits 31-16 (RAVAIL)

// Define a structure for the JTAG UART registers
typedef struct {
    volatile unsigned int data;     // Data register at base address
    volatile unsigned int control;  // Control register at base + 4
} jtag_uart_t;

// Maximum buffer size for our file read operations
#define MAX_BUFFER_SIZE 1024

/**
 * Initialize the JTAG UART interface
 */
jtag_uart_t* jtag_uart_init() {
    return (jtag_uart_t*)JTAG_UART_BASE;
}

/**
 * Check if there's data available to read from the JTAG UART
 * @param uart Pointer to the JTAG UART structure
 * @return true if data is available, false otherwise
 */
bool jtag_uart_data_available(jtag_uart_t* uart) {
    return (uart->data & RVALID_BIT);
}

/**
 * Read a single character from the JTAG UART
 * @param uart Pointer to the JTAG UART structure
 * @return The character read, or -1 if no data available
 */
int jtag_uart_read_char(jtag_uart_t* uart) {
    // Check if data is available
    if (jtag_uart_data_available(uart)) {
        // Return just the data bits (0-7)
        return uart->data & DATA_MASK;
    }
    return -1; // No data available
}

/**
 * Get the number of characters available in the FIFO
 * @param uart Pointer to the JTAG UART structure
 * @return Number of characters available
 */
int jtag_uart_chars_available(jtag_uart_t* uart) {
    return (uart->control & RAVAIL_MASK) >> 16;
}

/**
 * Read data from the JTAG UART into a buffer
 * @param uart Pointer to the JTAG UART structure
 * @param buffer Buffer to store the read data
 * @param max_size Maximum number of bytes to read
 * @return Number of bytes actually read
 */
int jtag_uart_read(jtag_uart_t* uart, char* buffer, int max_size) {
    int count = 0;
    int char_data;
    
    // Determine how many characters we can read
    int chars_available = jtag_uart_chars_available(uart);
    int to_read = (chars_available < max_size) ? chars_available : max_size;
    
    // Read characters until we've read enough or there's no more data
    while (count < to_read) {
        char_data = jtag_uart_read_char(uart);
        if (char_data == -1) break; // No more data available
        
        buffer[count++] = (char)char_data;
    }
    
    return count;
}

/**
 * Read a file from the JTAG UART interface
 * @param filename Destination filename to save the data
 * @param max_size Maximum file size to read
 * @return 0 on success, negative value on error
 */
int read_file_from_jtag(const char* filename, int max_size) {
    jtag_uart_t* uart = jtag_uart_init();
    FILE* file = NULL;
    char buffer[MAX_BUFFER_SIZE];
    int total_bytes = 0;
    int bytes_read;
    
    // First, expect to receive the file size
    while (!jtag_uart_data_available(uart)) {
        // Wait for file size data
    }
    
    // Read file size (assuming it's sent as a 4-byte integer)
    int expected_size = 0;
    for (int i = 0; i < 4; i++) {
        while (!jtag_uart_data_available(uart)) {
            // Wait for next byte
        }
        int byte = jtag_uart_read_char(uart);
        expected_size |= (byte << (i * 8));
    }
    
    // Cap file size to our maximum
    if (expected_size > max_size) {
        expected_size = max_size;
    }
    
    // Open the destination file
    file = fopen(filename, "wb");
    if (!file) {
        printf("Error: Could not open file %s for writing\n", filename);
        return -1;
    }
    
    // Read data in chunks until we've received the entire file
    while (total_bytes < expected_size) {
        // Wait until there's data available
        while (jtag_uart_chars_available(uart) == 0) {
            // Simple polling loop
        }
        
        // Read a chunk of data
        int remaining = expected_size - total_bytes;
        int max_chunk = (remaining < MAX_BUFFER_SIZE) ? remaining : MAX_BUFFER_SIZE;
        
        bytes_read = jtag_uart_read(uart, buffer, max_chunk);
        if (bytes_read <= 0) break; // Error or no more data
        
        // Write the data to the file
        fwrite(buffer, 1, bytes_read, file);
        total_bytes += bytes_read;
        
        // Print progress
        printf("Read %d of %d bytes\r", total_bytes, expected_size);
    }
    
    printf("\nFile transfer complete: %d bytes received\n", total_bytes);
    fclose(file);
    
    return 0;
}

/**
 * Main function
 */
int main(void) {
    printf("JTAG UART File Reader\n");
    printf("Waiting for file transfer...\n");
    
    // Read a file with a maximum size of 10MB
    int result = read_file_from_jtag("received_file.bin", 10 * 1024 * 1024);
    
    if (result == 0) {
        printf("File successfully received and saved as 'received_file.bin'\n");
    } else {
        printf("Error receiving file\n");
    }
    
    return 0;
}