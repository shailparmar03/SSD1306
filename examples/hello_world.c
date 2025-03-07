#include <stdio.h>
#include <unistd.h>
#include "ssd1306.h"

int main(void) {

    int fd = ssd1306_init("/dev/i2c-2", 0x3C);
    if (fd < 0) {
        return 1;
    }

    ssd1306_clear_display(fd);
    ssd1306_set_cursor(fd, 0, 0);
    ssd1306_draw_string(fd, "Hello World");
    
    sleep(5);
    
    ssd1306_clear_display(fd);
    ssd1306_close(fd);
    
    return 0;
}

