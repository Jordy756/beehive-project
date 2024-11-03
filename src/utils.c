#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "../include/core/utils.h"

void init_random(void) {
    srand(time(NULL));
}

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void delay_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}