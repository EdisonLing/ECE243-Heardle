#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "system.h"
#include "sys/alt_stdio.h"

//gpt nios V test; danielzj.zheng acc

#define BUFFER_SIZE 256

void file_transfer_via_uart() {
    FILE *fp;
    char buffer[BUFFER_SIZE];
    
    // Open file for reading and writing
    fp = fopen("testfile.txt", "a+");
    if (!fp) {
        printf("Error: Unable to open file!\n");
        return;
    }
    
    printf("Waiting for data over JTAG UART...\n");
    while (1) {
        int data = alt_getchar();  // Read from JTAG UART
        if (data != EOF) {
            buffer[0] = (char)data;
            buffer[1] = '\0';
            printf("Received: %s", buffer);
            fprintf(fp, "%s", buffer); // Write received data to file
            fflush(fp);
            
            // Echo back to the host
            alt_putchar(data);
        }
    }
    
    fclose(fp);
}

int main() {
    printf("Starting JTAG UART File Transfer...\n");
    file_transfer_via_uart();
    return 0;
}