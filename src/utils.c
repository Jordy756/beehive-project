#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

void init_random(void) {
    srand(time(NULL));
}

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void log_message(const char* message) {
    time_t now = time(NULL);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0'; // Remove newline
    printf("[%s] %s\n", timestamp, message);
}

void delay_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}