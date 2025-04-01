//last resort; this interacts with spi (idk what that stands for but the sd card read def doesnt have a dedicated controller..)
//claude generated code
/**
 * DE1-SoC SD Card File Reader via SPI
 * Implements SD card access using SPI mode through JP1 expansion port
 * 
 * This code interacts directly with the hardware, no OS required
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <stdbool.h>
 #include <string.h>
 
 // DE1-SoC memory-mapped I/O addresses for JP1 expansion port
 #define JP1_BASE          0xFF200060  // Base address for JP1 Expansion
 #define JP1_DATA          (JP1_BASE + 0x00)  // Data register
 #define JP1_DIR           (JP1_BASE + 0x04)  // Direction register (1=output, 0=input)
 
 // SPI pin assignments on JP1 expansion port
 // Adjust these based on your actual wiring
 #define SPI_CS_PIN        0x01    // Chip Select (CS) - Pin 0
 #define SPI_MOSI_PIN      0x02    // Master Out Slave In (MOSI) - Pin 1
 #define SPI_MISO_PIN      0x04    // Master In Slave Out (MISO) - Pin 2
 #define SPI_SCK_PIN       0x08    // Serial Clock (SCK) - Pin 3
 
 // SPI control bits
 #define SPI_OUTPUT_PINS   (SPI_CS_PIN | SPI_MOSI_PIN | SPI_SCK_PIN)  // Pins configured as outputs
 #define SPI_INPUT_PINS    (SPI_MISO_PIN)                            // Pins configured as inputs
 
 // SD card commands
 #define CMD0              0       // GO_IDLE_STATE
 #define CMD1              1       // SEND_OP_COND
 #define CMD8              8       // SEND_IF_COND
 #define CMD9              9       // SEND_CSD
 #define CMD10             10      // SEND_CID
 #define CMD12             12      // STOP_TRANSMISSION
 #define CMD13             13      // SEND_STATUS
 #define CMD16             16      // SET_BLOCKLEN
 #define CMD17             17      // READ_SINGLE_BLOCK
 #define CMD18             18      // READ_MULTIPLE_BLOCK
 #define CMD24             24      // WRITE_BLOCK
 #define CMD25             25      // WRITE_MULTIPLE_BLOCK
 #define CMD32             32      // ERASE_WR_BLK_START
 #define CMD33             33      // ERASE_WR_BLK_END
 #define CMD38             38      // ERASE
 #define CMD55             55      // APP_CMD
 #define CMD58             58      // READ_OCR
 #define ACMD41            41      // SD_SEND_OP_COND
 
 // R1 response bit flags
 #define R1_IDLE_STATE     (1 << 0)
 #define R1_ERASE_RESET    (1 << 1)
 #define R1_ILLEGAL_CMD    (1 << 2)
 #define R1_CRC_ERROR      (1 << 3)
 #define R1_ERASE_SEQ_ERR  (1 << 4)
 #define R1_ADDR_ERROR     (1 << 5)
 #define R1_PARAM_ERROR    (1 << 6)
 #define R1_READY          0x00
 
 // Data tokens
 #define DATA_START_BLOCK  0xFE    // Start block token for single block read/write
 #define DATA_RES_MASK     0x1F    // Mask for data response token
 #define DATA_RES_ACCEPTED 0x05    // Data accepted
 
 // Memory-mapped I/O access functions
 static inline void write_reg(uint32_t addr, uint32_t value) {
     *((volatile uint32_t *)addr) = value;
 }
 
 static inline uint32_t read_reg(uint32_t addr) {
     return *((volatile uint32_t *)addr);
 }
 
 // Function prototypes
 void spi_init(void);
 void spi_cs_assert(void);
 void spi_cs_deassert(void);
 uint8_t spi_transfer(uint8_t data);
 bool sd_init(void);
 uint8_t sd_send_command(uint8_t cmd, uint32_t arg);
 bool sd_read_block(uint32_t block_addr, uint8_t *buffer);
 bool sd_read_file(const char *filename, uint8_t *buffer, uint32_t *size);
 
 // FAT32 structures
 typedef struct {
     uint8_t boot_jump[3];
     uint8_t oem_name[8];
     uint16_t bytes_per_sector;
     uint8_t sectors_per_cluster;
     uint16_t reserved_sectors;
     uint8_t fat_count;
     uint16_t root_dir_entries;
     uint16_t total_sectors_16;
     uint8_t media_type;
     uint16_t fat_size_16;
     uint16_t sectors_per_track;
     uint16_t head_count;
     uint32_t hidden_sectors;
     uint32_t total_sectors_32;
     uint32_t fat_size_32;
     uint16_t ext_flags;
     uint16_t fs_version;
     uint32_t root_cluster;
     uint16_t fs_info;
     uint16_t backup_boot;
     uint8_t reserved[12];
     uint8_t drive_number;
     uint8_t reserved1;
     uint8_t boot_signature;
     uint32_t volume_id;
     uint8_t volume_label[11];
     uint8_t fs_type[8];
 } __attribute__((packed)) fat32_boot_sector_t;
 
 typedef struct {
     uint8_t name[8];
     uint8_t ext[3];
     uint8_t attributes;
     uint8_t reserved;
     uint8_t create_time_tenths;
     uint16_t create_time;
     uint16_t create_date;
     uint16_t access_date;
     uint16_t first_cluster_high;
     uint16_t write_time;
     uint16_t write_date;
     uint16_t first_cluster_low;
     uint32_t file_size;
 } __attribute__((packed)) directory_entry_t;
 
 // Global variables
 static fat32_boot_sector_t boot_sector;
 static uint32_t fat_start_sector;
 static uint32_t data_start_sector;
 static uint32_t root_dir_cluster;
 static uint32_t sectors_per_cluster;
 static uint32_t bytes_per_cluster;
 static bool card_type_sdhc = false;
 
 // Simple delay function
 void delay_us(uint32_t us) {
     // Approximate delay - adjust based on your clock frequency
     volatile uint32_t i;
     for (i = 0; i < us * 10; i++) {
         __asm__("nop");
     }
 }
 
 // Initialize SPI interface
 void spi_init(void) {
     // Configure pin directions: MOSI, SCK, CS as outputs, MISO as input
     uint32_t dir = read_reg(JP1_DIR);
     dir |= SPI_OUTPUT_PINS;  // Set output pins
     dir &= ~SPI_INPUT_PINS;  // Clear input pins
     write_reg(JP1_DIR, dir);
     
     // Initial state: CS high (deselected), SCK low
     uint32_t data = read_reg(JP1_DATA);
     data |= SPI_CS_PIN;     // CS high
     data &= ~SPI_SCK_PIN;   // SCK low
     write_reg(JP1_DATA, data);
 }
 
 // Assert chip select (active low)
 void spi_cs_assert(void) {
     uint32_t data = read_reg(JP1_DATA);
     data &= ~SPI_CS_PIN;   // CS low
     write_reg(JP1_DATA, data);
     delay_us(1);          // Small delay
 }
 
 // Deassert chip select
 void spi_cs_deassert(void) {
     uint32_t data = read_reg(JP1_DATA);
     data |= SPI_CS_PIN;    // CS high
     write_reg(JP1_DATA, data);
     delay_us(1);          // Small delay
 }
 
 // Transfer one byte over SPI
 uint8_t spi_transfer(uint8_t tx_data) {
     uint8_t rx_data = 0;
     uint32_t data;
     
     // Send/receive bit by bit (MSB first)
     for (int i = 7; i >= 0; i--) {
         // Prepare MOSI data bit
         data = read_reg(JP1_DATA);
         if (tx_data & (1 << i))
             data |= SPI_MOSI_PIN;
         else
             data &= ~SPI_MOSI_PIN;
         
         // Clock low
         data &= ~SPI_SCK_PIN;
         write_reg(JP1_DATA, data);
         delay_us(1);
         
         // Clock high and sample MISO
         data |= SPI_SCK_PIN;
         write_reg(JP1_DATA, data);
         delay_us(1);
         
         // Read MISO bit
         if (read_reg(JP1_DATA) & SPI_MISO_PIN)
             rx_data |= (1 << i);
         
         // Clock low
         data &= ~SPI_SCK_PIN;
         write_reg(JP1_DATA, data);
         delay_us(1);
     }
     
     return rx_data;
 }
 
 // Wait for SD card to be ready (not busy)
 bool sd_wait_ready(void) {
     uint8_t response;
     uint32_t timeout = 100000;  // Adjust based on your system speed
     
     // Wait for card to be ready (0xFF response)
     do {
         response = spi_transfer(0xFF);
         if (response == 0xFF)
             return true;
         delay_us(10);
     } while (--timeout);
     
     return false;
 }
 
 // Send a command to SD card
 uint8_t sd_send_command(uint8_t cmd, uint32_t arg) {
     uint8_t response, retry = 0;
     
     // Wait if busy
     if (!sd_wait_ready())
         return 0xFF;
     
     // Send command
     spi_transfer(0x40 | cmd);        // Command (bit 6 always set)
     spi_transfer((arg >> 24) & 0xFF); // Argument [31:24]
     spi_transfer((arg >> 16) & 0xFF); // Argument [23:16]
     spi_transfer((arg >> 8) & 0xFF);  // Argument [15:8]
     spi_transfer(arg & 0xFF);         // Argument [7:0]
     
     // Send CRC (only matters for CMD0 and CMD8)
     if (cmd == CMD0)
         spi_transfer(0x95);  // Valid CRC for CMD0(0)
     else if (cmd == CMD8)
         spi_transfer(0x87);  // Valid CRC for CMD8(0x1AA)
     else
         spi_transfer(0x01);  // Dummy CRC + Stop bit
     
     // Wait for response (up to 8 bytes delay)
     for (retry = 0; retry < 8; retry++) {
         response = spi_transfer(0xFF);
         if (!(response & 0x80))  // Response received (bit 7 cleared)
             break;
     }
     
     return response;
 }
 
 // Initialize SD card
 bool sd_init(void) {
     uint8_t r1;
     uint32_t retry;
     bool v2_card = false;
     
     // Initialize SPI interface
     spi_init();
     
     // Deselect card and send at least 74 clock cycles
     spi_cs_deassert();
     for (int i = 0; i < 10; i++)
         spi_transfer(0xFF);
     
     // Select card
     spi_cs_assert();
     
     // Send CMD0 (GO_IDLE_STATE) to reset and enter SPI mode
     retry = 100;
     do {
         r1 = sd_send_command(CMD0, 0);
         retry--;
     } while ((r1 != R1_IDLE_STATE) && retry);
     
     if (r1 != R1_IDLE_STATE) {
         spi_cs_deassert();
         return false;  // Card did not enter idle state
     }
     
     // Send CMD8 (SEND_IF_COND) to check card version
     r1 = sd_send_command(CMD8, 0x1AA); // VHS = 3.3V, check pattern = 0xAA
     
     if (r1 == R1_IDLE_STATE) {
         // Card is V2 - read the remaining R7 response
         uint8_t resp[4];
         for (int i = 0; i < 4; i++)
             resp[i] = spi_transfer(0xFF);
         
         // Check voltage acceptance and echo-back pattern
         if (resp[2] == 0x01 && resp[3] == 0xAA)
             v2_card = true;
         else {
             spi_cs_deassert();
             return false;  // Card doesn't support our voltage
         }
     }
     
     // Initialize card
     retry = 1000;
     do {
         // Send CMD55 (APP_CMD) followed by ACMD41 (SD_SEND_OP_COND)
         sd_send_command(CMD55, 0);
         r1 = sd_send_command(ACMD41, v2_card ? 0x40000000 : 0); // HCS bit set for V2 card
         
         retry--;
         delay_us(1000);
     } while (r1 && retry);
     
     if (!retry) {
         spi_cs_deassert();
         return false;  // Initialization failed
     }
     
     // Check if it's an SDHC card (for block addressing)
     if (v2_card) {
         // Send CMD58 (READ_OCR)
         if (sd_send_command(CMD58, 0) == R1_READY) {
             // Read OCR register
             uint8_t ocr[4];
             for (int i = 0; i < 4; i++)
                 ocr[i] = spi_transfer(0xFF);
             
             // Check CCS bit
             if (ocr[0] & 0x40)
                 card_type_sdhc = true;  // SDHC or SDXC card
         }
     }
     
     // Set block size to 512 bytes (not needed for SDHC/SDXC)
     if (!card_type_sdhc)
         sd_send_command(CMD16, 512);
     
     // Deselect card
     spi_cs_deassert();
     spi_transfer(0xFF);  // One more clock cycle
     
     return true;
 }
 
 // Read a single block from SD card
 bool sd_read_block(uint32_t block_addr, uint8_t *buffer) {
     uint8_t response;
     uint16_t i;
     uint32_t addr = block_addr;
     
     // Convert to byte address if not SDHC
     if (!card_type_sdhc)
         addr *= 512;
     
     // Select card
     spi_cs_assert();
     
     // Send READ_SINGLE_BLOCK command
     response = sd_send_command(CMD17, addr);
     if (response != R1_READY) {
         spi_cs_deassert();
         return false;
     }
     
     // Wait for data token
     do {
         response = spi_transfer(0xFF);
     } while (response == 0xFF);
     
     // Check if we got the data token
     if (response != DATA_START_BLOCK) {
         spi_cs_deassert();
         return false;
     }
     
     // Read the block data
     for (i = 0; i < 512; i++)
         buffer[i] = spi_transfer(0xFF);
     
     // Read and discard CRC
     spi_transfer(0xFF);
     spi_transfer(0xFF);
     
     // Deselect card
     spi_cs_deassert();
     spi_transfer(0xFF);  // One more clock cycle
     
     return true;
 }
 
 // Parse FAT32 boot sector
 bool parse_boot_sector(void) {
     uint8_t buffer[512];
     
     // Read boot sector (sector 0)
     if (!sd_read_block(0, buffer))
         return false;
     
     // Copy to boot sector structure
     memcpy(&boot_sector, buffer, sizeof(boot_sector));
     
     // Validate boot sector
     if (buffer[510] != 0x55 || buffer[511] != 0xAA)
         return false;
     
     // Calculate important values
     fat_start_sector = boot_sector.reserved_sectors;
     sectors_per_cluster = boot_sector.sectors_per_cluster;
     bytes_per_cluster = sectors_per_cluster * boot_sector.bytes_per_sector;
     data_start_sector = fat_start_sector + 
                         (boot_sector.fat_count * boot_sector.fat_size_32);
     root_dir_cluster = boot_sector.root_cluster;
     
     return true;
 }
 
 // Get next cluster in chain
 uint32_t get_next_cluster(uint32_t current_cluster) {
     uint8_t buffer[512];
     uint32_t fat_offset = current_cluster * 4;
     uint32_t fat_sector = fat_start_sector + (fat_offset / 512);
     uint32_t fat_index = (fat_offset % 512) / 4;
     
     // Read FAT sector
     if (!sd_read_block(fat_sector, buffer))
         return 0;
     
     // Extract next cluster
     uint32_t next_cluster = *(uint32_t*)(&buffer[fat_index * 4]);
     next_cluster &= 0x0FFFFFFF;  // Mask out upper 4 bits
     
     return next_cluster;
 }
 
 // Convert cluster number to sector number
 uint32_t cluster_to_sector(uint32_t cluster) {
     return data_start_sector + (cluster - 2) * sectors_per_cluster;
 }
 
 // Compare file names
 bool compare_filename(const char *filename, const directory_entry_t *entry) {
     char short_name[13];
     int i, j;
     
     // Convert directory entry to short filename
     for (i = 0; i < 8; i++) {
         if (entry->name[i] == ' ')
             break;
         short_name[i] = entry->name[i];
     }
     
     if (entry->ext[0] != ' ') {
         short_name[i++] = '.';
         for (j = 0; j < 3; j++) {
             if (entry->ext[j] == ' ')
                 break;
             short_name[i++] = entry->ext[j];
         }
     }
     short_name[i] = '\0';
     
     // Compare names
     i = 0;
     while (filename[i] && short_name[i]) {
         if (filename[i] != short_name[i])
             return false;
         i++;
     }
     
     return filename[i] == short_name[i];
 }
 
 // Read a file from SD card
 bool sd_read_file(const char *filename, uint8_t *buffer, uint32_t *size) {
     uint8_t cluster_buffer[512];
     uint32_t cluster = root_dir_cluster;
     uint32_t sector;
     uint32_t read_size = 0;
     bool found = false;
     directory_entry_t *entry;
     
     // Initialize SD card
     if (!sd_init())
         return false;
     
     // Parse boot sector
     if (!parse_boot_sector())
         return false;
     
     // Traverse root directory
     while (cluster >= 2 && cluster < 0x0FFFFFF8) {
         sector = cluster_to_sector(cluster);
         
         // Read all sectors in this cluster
         for (int i = 0; i < sectors_per_cluster; i++) {
             if (!sd_read_block(sector + i, cluster_buffer))
                 return false;
             
             // Check entries in this sector
             for (int j = 0; j < 512 / sizeof(directory_entry_t); j++) {
                 entry = (directory_entry_t*)&cluster_buffer[j * sizeof(directory_entry_t)];
                 
                 // Skip empty entries
                 if (entry->name[0] == 0)
                     break;
                 if (entry->name[0] == 0xE5)
                     continue;
                 
                 // Skip volume label and subdirectories
                 if (entry->attributes & 0x08 || entry->attributes & 0x10)
                     continue;
                 
                 // Compare filename
                 if (compare_filename(filename, entry)) {
                     found = true;
                     
                     // Get file size
                     *size = entry->file_size;
                     
                     // Get first cluster
                     uint32_t file_cluster = entry->first_cluster_low | 
                                            (entry->first_cluster_high << 16);
                     
                     // Read file data
                     while (file_cluster >= 2 && file_cluster < 0x0FFFFFF8) {
                         sector = cluster_to_sector(file_cluster);
                         
                         // Read all sectors in this cluster
                         for (int k = 0; k < sectors_per_cluster; k++) {
                             if (!sd_read_block(sector + k, &buffer[read_size]))
                                 return false;
                             
                             read_size += 512;
                             if (read_size >= *size) {
                                 // File read complete
                                 return true;
                             }
                         }
                         
                         // Get next cluster
                         file_cluster = get_next_cluster(file_cluster);
                     }
                     
                     return true;
                 }
             }
         }
         
         // Get next cluster
         cluster = get_next_cluster(cluster);
     }
     
     return found;
 }
 
 // Example usage
 int main() {
     uint8_t buffer[65536];  // Buffer for file data
     uint32_t size;
     
     if (sd_read_file("TEST.TXT", buffer, &size)) {
         // File read successful
         buffer[size] = '\0';  // Null-terminate if text file
         
         // Here you can process the file data
         // For example, if it's a text file, you could print it:
         // printf("File contents: %s\n", buffer);
         
         return 0;
     } else {
         // File read failed
         return -1;
     }
 }