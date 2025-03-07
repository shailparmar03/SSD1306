# SSD1306 OLED Library and Examples

This project provides a simple C library for interfacing with an SSD1306-based 128×64 OLED display over I2C, along with several example applications that demonstrate its functionality. The library supports basic display control, text rendering using a 5×8 font, and hardware scrolling.

---

## Project Structure

```
SSD1306/
├── examples
│   ├── cpu_usage.c      # Displays per-core CPU usage with horizontal bars.
│   ├── hello_world.c    # Prints "Hello World" on the OLED.
│   ├── scroll_demo.c    # Demonstrates SSD1306 hardware scrolling.
│   └── snake.c          # A basic Snake game using a pixel-level framebuffer.
├── include
│   └── ssd1306.h        # Public header for the SSD1306 library.
├── lib                  # (Optional) Precompiled libraries will be placed here.
├── Makefile             # Build script for compiling the library and examples.
└── src
    └── ssd1306.c        # SSD1306 library implementation.
```

---

## Library Overview

### Basic Display Functions

- **`int ssd1306_init(const char *i2c_dev, uint8_t address);`**  
  Initializes the OLED display over the specified I2C device (e.g., `/dev/i2c-2`) and address (typically `0x3C`).

- **`void ssd1306_close(int fd);`**  
  Closes the I2C file descriptor associated with the display.

- **`void ssd1306_clear_display(int fd);`**  
  Clears the entire display.

- **`void ssd1306_set_cursor(int fd, uint8_t page, uint8_t col);`**  
  Sets the drawing cursor to the specified page (row) and column.

- **`void ssd1306_draw_char(int fd, char c);`**  
  Draws a single character (using a built-in 5×8 font) at the current cursor position.

- **`void ssd1306_draw_string(int fd, const char *str);`**  
  Draws a null-terminated string starting at the current cursor position.

### Scrolling Functions

- **`void ssd1306_start_scroll_left(int fd, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed);`**  
  Initiates leftward horizontal scrolling on the display.  
  - **Parameters:**  
    - `start_page`: The starting page (0–7) for scrolling.  
    - `end_page`: The ending page (0–7) for scrolling.  
    - `scroll_speed`: The scroll interval (0x00 for fastest; higher values slow down scrolling).

- **`void ssd1306_start_scroll_right(int fd, uint8_t start_page, uint8_t end_page, uint8_t scroll_speed);`**  
  Initiates rightward horizontal scrolling with similar parameters as above.

- **`void ssd1306_stop_scroll(int fd);`**  
  Stops any active scrolling on the display.

---

## Building the Project

A `Makefile` is provided to simplify building the library and examples.

### To build all examples, run:

```bash
make all
```

This command compiles the SSD1306 library (`src/ssd1306.c`) along with each example in the `examples/` directory (such as `hello_world`, `cpu_usage`, `scroll_demo`, and `snake`).

### To clean the build, run:

```bash
make clean
```

---

## Running the Examples

Since the library communicates with the I2C bus, you might need root privileges to run the examples. For instance:

```bash
sudo ./hello_world
sudo ./cpu_usage
sudo ./scroll_demo
sudo ./snake
```

### Example Descriptions

- **Hello World:**  
  Displays the text "Hello World" on the OLED.

- **CPU Usage:**  
  Reads CPU statistics from `/proc/stat` and displays per-core CPU usage with horizontal bars. The demo updates every second.

- **Scroll Demo:**  
  Demonstrates the hardware scrolling feature by scrolling a sample string across the display.

- **Snake:**  
  Implements a basic Snake game using a pixel-level framebuffer. The game uses raw terminal input (WASD for movement and Q to quit).

---

## Using the SSD1306 Library in Your Own Project

To integrate the SSD1306 library:

1. Include the header in your source file:
   ```c
   #include "ssd1306.h"
   ```

2. Compile your application with the library source or precompiled library. For example:
   ```bash
   gcc -o my_app my_app.c src/ssd1306.c -Iinclude -Wall -O2
   ```
   Or, if you build a static/shared library in `lib/`:
   ```bash
   gcc -o my_app my_app.c -Llib -lssd1306 -Iinclude -Wall -O2
   ```
3. Run your application (with `sudo` if required).

---

## Author

shailparmar03 <shailparmar26@gmail.com>

