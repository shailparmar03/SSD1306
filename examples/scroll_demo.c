#include <stdio.h>
#include <unistd.h>
#include "ssd1306.h"

int main(void) {
    int fd = ssd1306_init("/dev/i2c-2", 0x3C);
    if (fd < 0) return 1;
    
    ssd1306_clear_display(fd);
    ssd1306_set_cursor(fd, 0, 0);
    ssd1306_draw_string(fd, "KL RAHUL ROCKS!");
    
    sleep(2);
    
    // Start left scroll on page 0 (0x00 fastest).
    ssd1306_start_scroll_left(fd, 0x00, 0x00, 0x00);
    sleep(10);
    ssd1306_stop_scroll(fd);
    
    sleep(2);
    ssd1306_clear_display(fd);
    ssd1306_close(fd);
    return 0;
}

