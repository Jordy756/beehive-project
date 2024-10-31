#ifndef UTILS_H
#define UTILS_H

#include "beehive_types.h"

int random_range(int min, int max);
void init_random(void);
void log_message(const char* message);
void delay_ms(int milliseconds);

#endif