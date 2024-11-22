#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/core/utils.h"

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);// Generar un número aleatorio entre 0 y max-min
}

void delay_ms(int milliseconds) {// Retrasar el programa por un número de milisegundos
    usleep(milliseconds * 1000);
}

char* format_time(time_t t) {
    static char buffer[26];// Buffer de almacenamiento para la fecha y hora
    struct tm* tm_info = localtime(&t);// Obtener la información de la fecha y hora
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);// Formatear la fecha y hora
    return buffer;// Devolver la fecha y hora formateada
}

bool directory_exists(const char* path) {
    struct stat st = {0};// Estructura de estado de un archivo
    return stat(path, &st) != -1;// Comprobar si el archivo existe
}

int create_directory(const char* path) {
    #ifdef _WIN32// Si se está compilando para Windows
        return mkdir(path);// Crear el directorio
    #else
        return mkdir(path, 0700);// Crear el directorio con permisos de lectura y escritura para los propietarios
    #endif
}

bool file_exists(const char* filename) {
    return access(filename, F_OK) == 0;// Comprobar si el archivo existe
}

json_object* read_json_array_file(const char* filename) {
    json_object* root = json_object_from_file(filename);// Leer el archivo JSON
    if (!root) {// Si el archivo no existe o no es un archivo JSON
        root = json_object_new_array();// Crear un arreglo vacío
    } else if (!json_object_is_type(root, json_type_array)) {// Si el archivo JSON no es un arreglo
        json_object_put(root);// Liberar la memoria del objeto
        root = json_object_new_array();// Crear un arreglo vacío
    }
    return root;// Devolver el arreglo
}

void write_json_file(const char* filename, json_object* json) {
    if (!filename || !json) return;// Comprobar si se proporcionó un archivo y un objeto JSON
    
    FILE* fp = fopen(filename, "w");// Abrir el archivo para escritura
    if (fp) {// Si se pudo abrir el archivo
        fputs(json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY), fp);// Escribir el objeto JSON en el archivo
        fclose(fp);// Cerrar el archivo
    }
}