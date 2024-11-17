#ifndef UTILS_H
#define UTILS_H

#include <time.h>

int random_range(int min, int max);
void init_random(void);
void delay_ms(int milliseconds);
char* format_time(time_t t);  // Nueva función

#endif