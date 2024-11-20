#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../include/core/utils.h"

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void delay_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

char* format_time(time_t t) {
    static char buffer[26];
    struct tm* tm_info = localtime(&t);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

bool directory_exists(const char* path) {
    struct stat st = {0};
    return stat(path, &st) != -1;
}

int create_directory(const char* path) {
    #ifdef _WIN32
        return mkdir(path);
    #else
        return mkdir(path, 0700);
    #endif
}

bool file_exists(const char* filename) {
    return access(filename, F_OK) == 0;
}

json_object* read_json_array_file(const char* filename) {
    json_object* root = json_object_from_file(filename);
    if (!root) {
        root = json_object_new_array();
    } else if (!json_object_is_type(root, json_type_array)) {
        json_object_put(root);
        root = json_object_new_array();
    }
    return root;
}

void write_json_file(const char* filename, json_object* json) {
    if (!filename || !json) return;
    
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fputs(json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY), fp);
        fclose(fp);
    }
}