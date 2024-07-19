#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

void mkdir_safe(char *dir_name, int exist_ok) {
    struct stat st = {0};
    if(stat(dir_name, &st) != -1) {
        if(!exist_ok) {
            printf("ERROR -- Directory %s exists and exist_ok=%d\n", dir_name, exist_ok);
            exit(1);
        }
        //printf("INFO -- Directory %s exists and exist_ok=%d\n", dir_name, exist_ok);
        return;
    }
    if(mkdir(dir_name, 0777) == -1) {
        printf("ERROR -- Error creating directory %s\n", dir_name);
        exit(1);
    }
    //printf("INFO -- Created directory %s\n", dir_name);
}

FILE *open_safe(char *filename, char *mode) {
    errno = 0;
    FILE *f = fopen(filename, mode);
    if(f == NULL) {
        printf("ERROR -- Error opening file %s %s\n", filename, strerror(errno));
        exit(1);
    }
    //printf("INFO -- Opened file %s\n", filename);
    return f;
}

void close_safe(FILE *file) {
    if(fclose(file) == -1) {
        printf("ERROR -- Error closing file\n");
        exit(1);
    }
}

void prompt_input(char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    if(fgets(buffer, size, stdin) == NULL) {
        printf("ERROR -- Error reading input\n");
        exit(1);
    }
    buffer[strcspn(buffer, "\n")] = '\0';
}

DIR *opendir_safe(char *dirname) {
    DIR *dir = opendir(dirname);
    if(dir == NULL) {
        printf("ERROR -- Error opening directory %s\n", dirname);
    }
    //printf("INFO -- Opened directory %s\n", dirname);
    return dir;
}

char *read_to_buffer(char *filepath) {
    FILE *file = open_safe(filepath, "r");
    struct stat statbuf;
    if(stat(filepath, &statbuf) == -1) {
        printf("ERROR -- Error getting file status\n");
        exit(1);
    }
    char *buffer = (char *)malloc(statbuf.st_size + 1);
    if(buffer == NULL) {
        printf("ERROR -- Error allocating memory for file %s\n", filepath);
        close_safe(file);
        exit(1);
    };
    size_t bytes = fread(buffer, 1, statbuf.st_size, file);
    if(bytes != statbuf.st_size) {
        printf("Error reading to buffer file %s\n", filepath);
        free(buffer);
        close_safe(file);
        exit(1);
    }
    buffer[statbuf.st_size] = '\0';
    close_safe(file);
    return buffer;
}

char **read_to_lines(char *path, int *num_lines) {
    char *content = read_to_buffer(path);
    *num_lines = 0;
    for (char *ptr = content; *ptr; ptr++) {
        if (*ptr == '\n') {
            (*num_lines)++;
        }
    }
    char **lines = malloc((*num_lines + 1) * sizeof(char *));
    if (!lines) {
        perror("malloc");
        free(content);
        exit(EXIT_FAILURE);
    }
    int line_index = 0;
    char *line = strtok(content, "\n");
    while (line) {
        lines[line_index] = strdup(line);
        line = strtok(NULL, "\n");
        line_index++;
    }
    *num_lines = line_index;
    lines[line_index] = NULL;  
    free(content);
    return lines;
}

void free_lines(char **lines, int num_lines) {
    for(int i = 0; i < num_lines; i++) {
        free(lines[i]);
    }
    free(lines);
}

void parse_file_from_prefix(char *filepath, char *prefix, char *value) {
    FILE *f = open_safe(filepath, "r");
    char line[128];
    size_t prefix_len = strlen(prefix);
    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, prefix, prefix_len) == 0) {
            strcpy(value, line + prefix_len);
            value[strcspn(value, "\n")] = '\0';
            close_safe(f);
            return;
        }
    }
    printf("ERROR -- Unable to read %s from %s", prefix, filepath);
    exit(1);
}


void generate_timestamp(char *buffer, size_t size) {
    time_t current_time;
    struct tm *time_info;
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", time_info);
    //printf("INFO -- Generated timestamp %s\n", buffer);
}

