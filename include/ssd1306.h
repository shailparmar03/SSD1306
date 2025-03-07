#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

// Initialize the SSD1306 display.
int ssd1306_init(const char *i2c_dev, uint8_t address);

// Close the I2C file descriptor.
void ssd1306_close(int fd);

// Clear the entire display.
void ssd1306_clear_display(int fd);

// Set the cursor to a given page (row) and column.
void ssd1306_set_cursor(int fd, uint8_t page, uint8_t col);

// Draw a single character at the current cursor position.
void ssd1306_draw_char(int fd, char c);

// Draw a null-terminated string.
void ssd1306_draw_string(int fd, const char *str);

#endif // SSD1306_H

