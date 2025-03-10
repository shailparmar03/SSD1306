#include "ssd1306.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <string.h>

// Private helper: send a command byte to the display.
static int ssd1306_send_cmd(int fd, uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // 0x00 indicates command mode
    if (write(fd, buf, 2) != 2) {
        perror("ssd1306: Failed to write command");
        return -1;
    }
    return 0;
}

int ssd1306_init(const char *i2c_dev, uint8_t address) {
    int fd = open(i2c_dev, O_RDWR);
    if (fd < 0) {
        perror("ssd1306: Failed to open I2C device");
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, address) < 0) {
        perror("ssd1306: Failed to acquire bus access");
        close(fd);
        return -1;
    }

    // Initialization sequence for SSD1306 (commands from the datasheet)
    ssd1306_send_cmd(fd, 0xAE); // Display OFF
    ssd1306_send_cmd(fd, 0xD5); // Set display clock divide ratio/oscillator frequency
    ssd1306_send_cmd(fd, 0x80); // Suggested ratio
    ssd1306_send_cmd(fd, 0xA8); // Set multiplex ratio
    ssd1306_send_cmd(fd, 0x3F); // 64MUX for 128x64 display
    ssd1306_send_cmd(fd, 0xD3); // Set display offset
    ssd1306_send_cmd(fd, 0x00); // No offset
    ssd1306_send_cmd(fd, 0x40); // Set start line address
    ssd1306_send_cmd(fd, 0x8D); // Charge pump setting
    ssd1306_send_cmd(fd, 0x14); // Enable charge pump
    ssd1306_send_cmd(fd, 0x20); // Memory addressing mode
    ssd1306_send_cmd(fd, 0x00); // Horizontal addressing mode
    ssd1306_send_cmd(fd, 0xA1); // Set segment re-map (mirror horizontally)
    ssd1306_send_cmd(fd, 0xC8); // Set COM output scan direction (remapped mode)
    ssd1306_send_cmd(fd, 0xDA); // Set COM pins hardware configuration
    ssd1306_send_cmd(fd, 0x12);
    ssd1306_send_cmd(fd, 0x81); // Set contrast control
    ssd1306_send_cmd(fd, 0xCF);
    ssd1306_send_cmd(fd, 0xD9); // Set pre-charge period
    ssd1306_send_cmd(fd, 0xF1);
    ssd1306_send_cmd(fd, 0xDB); // Set VCOMH deselect level
    ssd1306_send_cmd(fd, 0x40);
    ssd1306_send_cmd(fd, 0xA4); // Entire display follows RAM content
    ssd1306_send_cmd(fd, 0xA6); // Normal display (not inverted)
    ssd1306_send_cmd(fd, 0xAF); // Display ON

    return fd;
}

void ssd1306_close(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

void ssd1306_set_cursor(int fd, uint8_t page, uint8_t col) {
    ssd1306_send_cmd(fd, 0xB0 | (page & 0x07));         // Set page address (0xB0 to 0xB7)
    ssd1306_send_cmd(fd, 0x00 | (col & 0x0F));            // Set lower column start address
    ssd1306_send_cmd(fd, 0x10 | ((col >> 4) & 0x0F));     // Set higher column start address
}

void ssd1306_clear_display(int fd) {
    uint8_t zeros[129];
    zeros[0] = 0x40; // Data mode indicator
    memset(&zeros[1], 0x00, 128);
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_send_cmd(fd, 0xB0 + page); // Set page address
        ssd1306_send_cmd(fd, 0x00);        // Set lower column address
        ssd1306_send_cmd(fd, 0x10);        // Set higher column address
        if (write(fd, zeros, sizeof(zeros)) != sizeof(zeros)) {
            perror("ssd1306: Failed to clear page");
        }
    }
}

// -------------------------------------------------
// A simple 5x8 font for ASCII characters 32 to 126.
// Each character is represented by 5 bytes.
#define FONT_START 32
#define FONT_END   126

static const uint8_t font5x8[95][5] = {
    /* Space (32) */  {0x00,0x00,0x00,0x00,0x00},
    /* '!' (33)   */  {0x00,0x00,0x5F,0x00,0x00},
    /* '"' (34)   */  {0x00,0x07,0x00,0x07,0x00},
    /* '#' (35)   */  {0x14,0x7F,0x14,0x7F,0x14},
    /* '$' (36)   */  {0x24,0x2A,0x7F,0x2A,0x12},
    /* '%' (37)   */  {0x23,0x13,0x08,0x64,0x62},
    /* '&' (38)   */  {0x36,0x49,0x55,0x22,0x50},
    /* ''' (39)   */  {0x00,0x05,0x03,0x00,0x00},
    /* '(' (40)   */  {0x00,0x1C,0x22,0x41,0x00},
    /* ')' (41)   */  {0x00,0x41,0x22,0x1C,0x00},
    /* '*' (42)   */  {0x14,0x08,0x3E,0x08,0x14},
    /* '+' (43)   */  {0x08,0x08,0x3E,0x08,0x08},
    /* ',' (44)   */  {0x00,0x50,0x30,0x00,0x00},
    /* '-' (45)   */  {0x08,0x08,0x08,0x08,0x08},
    /* '.' (46)   */  {0x00,0x60,0x60,0x00,0x00},
    /* '/' (47)   */  {0x20,0x10,0x08,0x04,0x02},
    /* '0' (48)   */  {0x3E,0x51,0x49,0x45,0x3E},
    /* '1' (49)   */  {0x00,0x42,0x7F,0x40,0x00},
    /* '2' (50)   */  {0x42,0x61,0x51,0x49,0x46},
    /* '3' (51)   */  {0x21,0x41,0x45,0x4B,0x31},
    /* '4' (52)   */  {0x18,0x14,0x12,0x7F,0x10},
    /* '5' (53)   */  {0x27,0x45,0x45,0x45,0x39},
    /* '6' (54)   */  {0x3C,0x4A,0x49,0x49,0x30},
    /* '7' (55)   */  {0x01,0x71,0x09,0x05,0x03},
    /* '8' (56)   */  {0x36,0x49,0x49,0x49,0x36},
    /* '9' (57)   */  {0x06,0x49,0x49,0x29,0x1E},
    /* ':' (58)   */  {0x00,0x36,0x36,0x00,0x00},
    /* ';' (59)   */  {0x00,0x56,0x36,0x00,0x00},
    /* '<' (60)   */  {0x08,0x14,0x22,0x41,0x00},
    /* '=' (61)   */  {0x14,0x14,0x14,0x14,0x14},
    /* '>' (62)   */  {0x00,0x41,0x22,0x14,0x08},
    /* '?' (63)   */  {0x02,0x01,0x51,0x09,0x06},
    /* '@' (64)   */  {0x32,0x49,0x79,0x41,0x3E},
    /* 'A' (65)   */  {0x7E,0x11,0x11,0x11,0x7E},
    /* 'B' (66)   */  {0x7F,0x49,0x49,0x49,0x36},
    /* 'C' (67)   */  {0x3E,0x41,0x41,0x41,0x22},
    /* 'D' (68)   */  {0x7F,0x41,0x41,0x22,0x1C},
    /* 'E' (69)   */  {0x7F,0x49,0x49,0x49,0x41},
    /* 'F' (70)   */  {0x7F,0x09,0x09,0x09,0x01},
    /* 'G' (71)   */  {0x3E,0x41,0x49,0x49,0x7A},
    /* 'H' (72)   */  {0x7F,0x08,0x08,0x08,0x7F},
    /* 'I' (73)   */  {0x00,0x41,0x7F,0x41,0x00},
    /* 'J' (74)   */  {0x20,0x40,0x41,0x3F,0x01},
    /* 'K' (75)   */  {0x7F,0x08,0x14,0x22,0x41},
    /* 'L' (76)   */  {0x7F,0x40,0x40,0x40,0x40},
    /* 'M' (77)   */  {0x7F,0x02,0x04,0x02,0x7F},
    /* 'N' (78)   */  {0x7F,0x04,0x08,0x10,0x7F},
    /* 'O' (79)   */  {0x3E,0x41,0x41,0x41,0x3E},
    /* 'P' (80)   */  {0x7F,0x09,0x09,0x09,0x06},
    /* 'Q' (81)   */  {0x3E,0x41,0x51,0x21,0x5E},
    /* 'R' (82)   */  {0x7F,0x09,0x19,0x29,0x46},
    /* 'S' (83)   */  {0x46,0x49,0x49,0x49,0x31},
    /* 'T' (84)   */  {0x01,0x01,0x7F,0x01,0x01},
    /* 'U' (85)   */  {0x3F,0x40,0x40,0x40,0x3F},
    /* 'V' (86)   */  {0x1F,0x20,0x40,0x20,0x1F},
    /* 'W' (87)   */  {0x3F,0x40,0x38,0x40,0x3F},
    /* 'X' (88)   */  {0x63,0x14,0x08,0x14,0x63},
    /* 'Y' (89)   */  {0x07,0x08,0x70,0x08,0x07},
    /* 'Z' (90)   */  {0x61,0x51,0x49,0x45,0x43},
    /* '[' (91)   */  {0x00,0x7F,0x41,0x41,0x00},
    /* '\' (92)   */  {0x02,0x04,0x08,0x10,0x20},
    /* ']' (93)   */  {0x00,0x41,0x41,0x7F,0x00},
    /* '^' (94)   */  {0x04,0x02,0x01,0x02,0x04},
    /* '_' (95)   */  {0x40,0x40,0x40,0x40,0x40},
    /* '`' (96)   */  {0x00,0x03,0x07,0x00,0x00},
    /* 'a' (97)   */  {0x20,0x54,0x54,0x54,0x78},
    /* 'b' (98)   */  {0x7F,0x48,0x44,0x44,0x38},
    /* 'c' (99)   */  {0x38,0x44,0x44,0x44,0x20},
    /* 'd' (100)  */  {0x38,0x44,0x44,0x48,0x7F},
    /* 'e' (101)  */  {0x38,0x54,0x54,0x54,0x18},
    /* 'f' (102)  */  {0x08,0x7E,0x09,0x01,0x02},
    /* 'g' (103)  */  {0x0C,0x52,0x52,0x52,0x3E},
    /* 'h' (104)  */  {0x7F,0x08,0x04,0x04,0x78},
    /* 'i' (105)  */  {0x00,0x44,0x7D,0x40,0x00},
    /* 'j' (106)  */  {0x20,0x40,0x44,0x3D,0x00},
    /* 'k' (107)  */  {0x7F,0x10,0x28,0x44,0x00},
    /* 'l' (108)  */  {0x00,0x41,0x7F,0x40,0x00},
    /* 'm' (109)  */  {0x7C,0x04,0x18,0x04,0x78},
    /* 'n' (110)  */  {0x7C,0x08,0x04,0x04,0x78},
    /* 'o' (111)  */  {0x38,0x44,0x44,0x44,0x38},
    /* 'p' (112)  */  {0x7C,0x14,0x14,0x14,0x08},
    /* 'q' (113)  */  {0x08,0x14,0x14,0x18,0x7C},
    /* 'r' (114)  */  {0x7C,0x08,0x04,0x04,0x08},
    /* 's' (115)  */  {0x48,0x54,0x54,0x54,0x20},
    /* 't' (116)  */  {0x04,0x3F,0x44,0x40,0x20},
    /* 'u' (117)  */  {0x3C,0x40,0x40,0x20,0x7C},
    /* 'v' (118)  */  {0x1C,0x20,0x40,0x20,0x1C},
    /* 'w' (119)  */  {0x3C,0x40,0x30,0x40,0x3C},
    /* 'x' (120)  */  {0x44,0x28,0x10,0x28,0x44},
    /* 'y' (121)  */  {0x0C,0x50,0x50,0x50,0x3C},
    /* 'z' (122)  */  {0x44,0x64,0x54,0x4C,0x44},
    /* '{' (123)  */  {0x00,0x08,0x36,0x41,0x00},
    /* '|' (124)  */  {0x00,0x00,0x7F,0x00,0x00},
    /* '}' (125)  */  {0x00,0x41,0x36,0x08,0x00},
    /* '~' (126)  */  {0x08,0x04,0x08,0x10,0x08}
};

void ssd1306_draw_char(int fd, char c) {
    if (c < FONT_START || c > FONT_END) {
        c = ' '; // Replace unsupported characters with space
    }
    uint8_t idx = c - FONT_START;
    // Prepare a 6-byte array: 1 byte for data mode + 5 font bytes.
    uint8_t data[6];
    data[0] = 0x40; // Data mode indicator
    for (int i = 0; i < 5; i++) {
        data[i + 1] = font5x8[idx][i];
    }
    // Write the 6 bytes (5 columns of the character + 1 blank column for spacing)
    if (write(fd, data, 6) != 6) {
        perror("ssd1306: Failed to draw character");
    }
}

void ssd1306_draw_string(int fd, const char *str) {
    while (*str) {
        ssd1306_draw_char(fd, *str++);
    }
}

// Scrolling functions.
void ssd1306_start_scroll_left(int fd, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed) {
    ssd1306_send_cmd(fd, 0x2E); // Deactivate scrolling.
    ssd1306_send_cmd(fd, 0x27); // Left horizontal scroll command.
    ssd1306_send_cmd(fd, 0x00); // Dummy byte.
    ssd1306_send_cmd(fd, start_page); // Start page.
    ssd1306_send_cmd(fd, scroll_speed); // Time interval.
    ssd1306_send_cmd(fd, end_page);   // End page.
    ssd1306_send_cmd(fd, 0x00); // Dummy byte.
    ssd1306_send_cmd(fd, 0xFF); // Dummy byte.
    ssd1306_send_cmd(fd, 0x2F); // Activate scroll.
}

void ssd1306_start_scroll_right(int fd, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed) {
    ssd1306_send_cmd(fd, 0x2E); // Deactivate scrolling.
    ssd1306_send_cmd(fd, 0x26); // Right horizontal scroll command.
    ssd1306_send_cmd(fd, 0x00); // Dummy byte.
    ssd1306_send_cmd(fd, start_page); // Start page.
    ssd1306_send_cmd(fd, scroll_speed); // Time interval.
    ssd1306_send_cmd(fd, end_page);   // End page.
    ssd1306_send_cmd(fd, 0x00); // Dummy byte.
    ssd1306_send_cmd(fd, 0xFF); // Dummy byte.
    ssd1306_send_cmd(fd, 0x2F); // Activate scroll.
}

void ssd1306_stop_scroll(int fd) {
    ssd1306_send_cmd(fd, 0x2E); // Deactivate scrolling.
}
