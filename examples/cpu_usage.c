#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ssd1306.h"

#define MAX_CORES 8  // maximum cores to monitor

// Structure to store CPU statistics per core.
typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long total;
} CPUStat;

// Reads per-core CPU stats from /proc/stat.
// Returns the number of cores read (up to max_cores).
int get_cpu_stats(CPUStat stats[], int max_cores) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return 0;
    }
    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Look for lines that start with "cpu" followed by a digit.
        if (strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9') {
            if (count >= max_cores)
                break;
            CPUStat cs = {0};
            // Read first 8 numbers (ignore guest fields)
            sscanf(line, "cpu%*d %llu %llu %llu %llu %llu %llu %llu %llu",
                   &cs.user, &cs.nice, &cs.system, &cs.idle,
                   &cs.iowait, &cs.irq, &cs.softirq, &cs.steal);
            cs.total = cs.user + cs.nice + cs.system + cs.idle +
                       cs.iowait + cs.irq + cs.softirq + cs.steal;
            stats[count++] = cs;
        }
    }
    fclose(fp);
    return count;
}

// Draw a horizontal bar on the given page starting at start_col.
// bar_length is the number of columns (each column is 8 vertical pixels) to fill.
void draw_bar(int fd, uint8_t page, uint8_t start_col, uint8_t bar_length) {
    // Set cursor to the specified page and column.
    ssd1306_set_cursor(fd, page, start_col);
    // Prepare a buffer: first byte is data mode indicator, then bar_length bytes of 0xFF.
    uint8_t buf[129]; // maximum 128 columns + 1
    buf[0] = 0x40; // Data mode
    for (int i = 1; i <= bar_length; i++) {
        buf[i] = 0xFF;
    }
    // Write the bar.
    if (write(fd, buf, bar_length + 1)<0) perror("error drawing bar");
}

int main(void) {
    // Initialize the display.
    int fd = ssd1306_init("/dev/i2c-7", 0x3C);
    if (fd < 0) {
        return 1;
    }
    
    // Get initial CPU stats.
    CPUStat prev_stats[MAX_CORES] = {0}, curr_stats[MAX_CORES] = {0};
    int num_cores = get_cpu_stats(prev_stats, MAX_CORES);
    if (num_cores <= 0) {
        printf("No CPU cores found.\n");
        ssd1306_close(fd);
        return 1;
    }
    
    // Main update loop (update every second).
    while (1) {
        sleep(1);
        num_cores = get_cpu_stats(curr_stats, MAX_CORES);
        
        // Clear the display.
        // ssd1306_clear_display(fd);
        
        // For each core, compute usage and draw a label and a bar.
        for (int i = 0; i < num_cores; i++) {
            unsigned long long prev_total = prev_stats[i].total;
            unsigned long long curr_total = curr_stats[i].total;
            unsigned long long total_diff = curr_total - prev_total;
            // Idle includes idle + iowait.
            unsigned long long prev_idle = prev_stats[i].idle + prev_stats[i].iowait;
            unsigned long long curr_idle = curr_stats[i].idle + curr_stats[i].iowait;
            unsigned long long idle_diff = curr_idle - prev_idle;
            
            int usage = 0;
            if (total_diff != 0) {
                usage = (int)(((total_diff - idle_diff) * 100) / total_diff);
            }
            
            // Build a label string like "C0: 72%"
            char label[16];
            snprintf(label, sizeof(label), "C%d:%3d%%", i, usage);
            // Set cursor on page i, column 0 and draw the label.
            ssd1306_set_cursor(fd, i, 0);
            ssd1306_draw_string(fd, label);
            
            // Draw the usage bar starting at column 40.
            // For example, let the maximum bar width be 80 columns.
            uint8_t max_bar_width = 80;
            uint8_t bar_width = (usage * max_bar_width) / 100;
            draw_bar(fd, i, 40, bar_width);
            
            // Update previous stats for next iteration.
            prev_stats[i] = curr_stats[i];
        }
    }
    
    // Clear display and close.
    ssd1306_clear_display(fd);
    ssd1306_close(fd);
    
    return 0;
}

