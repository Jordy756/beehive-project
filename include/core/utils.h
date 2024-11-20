#ifndef UTILS_H
#define UTILS_H

#include <json-c/json.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>

// Funciones de números aleatorios
int random_range(int min, int max);

// Funciones de tiempo
void delay_ms(int milliseconds);
char* format_time(time_t t);

// Funciones de sistema de archivos
bool directory_exists(const char* path);
int create_directory(const char* path);
bool file_exists(const char* filename);

// Funciones de lectura/escritura JSON
json_object* read_json_array_file(const char* filename);
void write_json_file(const char* filename, json_object* json);

#endif