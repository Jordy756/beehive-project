#ifndef UTILS_H
#define UTILS_H

#include <json-c/json.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h>

// Funciones de números aleatorios
int random_range(int min, int max);// Generar un número aleatorio entre min y max

// Funciones de tiempo
void delay_ms(int milliseconds);// Retrasar el programa por un número de milisegundos
char* format_time(time_t t);// Formatear una fecha y hora

// Funciones de sistema de archivos
bool directory_exists(const char* path);// Comprobar si un directorio existe
int create_directory(const char* path);// Crear un directorio
bool file_exists(const char* filename);// Comprobar si un archivo existe

// Funciones de lectura/escritura JSON
json_object* read_json_array_file(const char* filename);// Leer un archivo JSON de arreglo
void write_json_file(const char* filename, json_object* json);// Escribir un archivo JSON

#endif