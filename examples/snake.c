#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include "ssd1306.h"

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define PAGE_COUNT 8

// Define the grid: each block is 4x4 pixels.
#define GRID_WIDTH 32    // 128/4
#define GRID_HEIGHT 16   // 64/4
#define BLOCK_SIZE 4

// Global framebuffer: one byte per column per page (8 pages for 64 pixels height)
uint8_t fb[PAGE_COUNT][OLED_WIDTH];

// Clear the framebuffer.
void clear_fb(void) {
    for (int p = 0; p < PAGE_COUNT; p++) {
        for (int col = 0; col < OLED_WIDTH; col++) {
            fb[p][col] = 0x00;
        }
    }
}

// Set a single pixel (x,y) in the framebuffer to value (0 or 1).
void set_pixel(int x, int y, int value) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
        return;
    int page = y / 8;
    int bit = y % 8;
    if (value)
        fb[page][x] |= (1 << bit);
    else
        fb[page][x] &= ~(1 << bit);
}

// Draw a block in grid coordinates; each block is BLOCK_SIZE x BLOCK_SIZE pixels.
void draw_block(int gridX, int gridY, int value) {
    int x = gridX * BLOCK_SIZE;
    int y = gridY * BLOCK_SIZE;
    for (int dy = 0; dy < BLOCK_SIZE; dy++) {
        for (int dx = 0; dx < BLOCK_SIZE; dx++) {
            set_pixel(x + dx, y + dy, value);
        }
    }
}

// Update the OLED display by writing the framebuffer.
// For each page (8-pixel high row), set the cursor and write 128 data bytes.
void update_display(int fd) {
    for (int page = 0; page < PAGE_COUNT; page++) {
        ssd1306_set_cursor(fd, page, 0);
        uint8_t buf[129];
        buf[0] = 0x40;  // Data mode indicator.
        for (int col = 0; col < OLED_WIDTH; col++) {
            buf[col + 1] = fb[page][col];
        }
        if (write(fd, buf, 129) != 129) {
            perror("update_display: write error");
        }
    }
}

// ==================== Snake Game Logic ====================

typedef struct {
    int x;
    int y;
} Point;

#define MAX_SNAKE 100
Point snake[MAX_SNAKE];
int snake_length;
int direction; // 0: up, 1: right, 2: down, 3: left
Point food;
int game_over = 0;

// Place food at a random position not occupied by the snake.
void place_food(void) {
    int valid = 0;
    while (!valid) {
        food.x = rand() % GRID_WIDTH;
        food.y = rand() % GRID_HEIGHT;
        valid = 1;
        for (int i = 0; i < snake_length; i++) {
            if (snake[i].x == food.x && snake[i].y == food.y) {
                valid = 0;
                break;
            }
        }
    }
}

// Check if a point collides with the boundaries or the snake body.
int check_collision(Point p) {
    if (p.x < 0 || p.x >= GRID_WIDTH || p.y < 0 || p.y >= GRID_HEIGHT)
        return 1;
    for (int i = 0; i < snake_length; i++) {
        if (snake[i].x == p.x && snake[i].y == p.y)
            return 1;
    }
    return 0;
}

// ============== Terminal Input Setup ==============

struct termios orig_termios;

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // disable echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Check if a key is available (non-blocking).
int kbhit(void) {
    int byteswaiting;
    ioctl(STDIN_FILENO, FIONREAD, &byteswaiting);
    return byteswaiting;
}

// ==================== Main Game Loop ====================

int main(void) {
    srand(time(NULL));
    
    // Initialize the OLED display.
    int fd = ssd1306_init("/dev/i2c-7", 0x3C);
    if (fd < 0) {
        return 1;
    }
    
    // Set terminal to raw non-blocking mode.
    enable_raw_mode();
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    // Initialize snake in the middle of the grid.
    snake_length = 3;
    snake[0].x = GRID_WIDTH / 2;
    snake[0].y = GRID_HEIGHT / 2;
    snake[1].x = snake[0].x - 1;
    snake[1].y = snake[0].y;
    snake[2].x = snake[0].x - 2;
    snake[2].y = snake[0].y;
    direction = 1; // moving right.
    place_food();
    
    // Game loop: update every 200 ms.
    while (!game_over) {
        // Handle user input.
        if (kbhit()) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) < 0) break;
            if (ch == 'w' && direction != 2) direction = 0;
            else if (ch == 'd' && direction != 3) direction = 1;
            else if (ch == 's' && direction != 0) direction = 2;
            else if (ch == 'a' && direction != 1) direction = 3;
            else if (ch == 'q') { game_over = 1; break; }
        }
        
        // Calculate new head position.
        Point new_head = snake[0];
        if (direction == 0) new_head.y -= 1;
        else if (direction == 1) new_head.x += 1;
        else if (direction == 2) new_head.y += 1;
        else if (direction == 3) new_head.x -= 1;
        
        // Check collision with walls or self.
        if (check_collision(new_head)) {
            game_over = 1;
            break;
        }
        
        // Move snake: shift the body.
        for (int i = snake_length - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0] = new_head;
        
        // Check if food is eaten.
        if (new_head.x == food.x && new_head.y == food.y) {
            if (snake_length < MAX_SNAKE) {
                snake[snake_length] = snake[snake_length - 1]; // duplicate tail.
                snake_length++;
            }
            place_food();
        }
        
        // Clear the framebuffer.
        clear_fb();
        
        // Draw snake.
        for (int i = 0; i < snake_length; i++) {
            draw_block(snake[i].x, snake[i].y, 1);
        }
        // Draw food.
        draw_block(food.x, food.y, 1);
        
        // Update OLED display.
        update_display(fd);
        
        // Wait 200 ms.
        usleep(200000);
    }
    
    // Game over: show "Game Over" message
    clear_fb();
    ssd1306_clear_display(fd);
    ssd1306_set_cursor(fd, 3, 10);
    ssd1306_draw_string(fd, "Game Over");
    sleep(5);
    update_display(fd);
    sleep(3);
    
    ssd1306_clear_display(fd);
    ssd1306_close(fd);
    disable_raw_mode();
    
    return 0;
}

