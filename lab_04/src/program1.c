#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "contract.h"

static void write_str(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

static void write_float(float x) {
    char buf[64];
    int len = snprintf(buf, sizeof(buf), "%.6f", x);
    write(STDOUT_FILENO, buf, len);
}

static void write_int(int x) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d", x);
    write(STDOUT_FILENO, buf, len);
}

static void write_array(int *arr, size_t n) {
    for (size_t i = 0; i < n; i++) {
        write_int(arr[i]);
        write_str(" ");
    }
    write_str("\n");
}

static int read_line(char *buf, size_t size) {
    ssize_t bytes = read(STDIN_FILENO, buf, size - 1);
    if (bytes <= 0) {
        return 0;
    }
    buf[bytes] = '\0';
    
    char *newline = strchr(buf, '\n');
    if (newline) {
        *newline = '\0';
    }
    return 1;
}

static int parse_two_floats(const char *str, float *a, float *b) {
    char *end;
    *a = strtof(str, &end);
    if (end == str) return 0;
    
    while (*end == ' ') end++;
    *b = strtof(end, &end);
    if (end == str) return 0;
    
    return 1;
}

static int parse_array(const char *str, int *arr, int n) {
    char *ptr = (char *)str;
    for (int i = 0; i < n; i++) {
        while (*ptr == ' ') ptr++;
        if (*ptr == '\0') return 0;
        
        arr[i] = strtol(ptr, &ptr, 10);
    }
    return 1;
}

int main(void) {
    char input[256];

    write_str("Program 1: Static linking\n");
    write_str("Commands:\n");
    write_str(" 1 a b - area \n");
    write_str(" 2 n a1 a2 ... an - sort\n");
    write_str(" exit - exit\n\n");

    while (1) {
        write_str("> ");
        if (!read_line(input, sizeof(input))) {
            break;
        }

        if (strcmp(input, "exit") == 0) {
            write_str("Exiting\n");
            break;
        }

        if (strlen(input) == 0) {
            continue;
        }

        else if (input[0] == '1') {
            float a, b;
            if (parse_two_floats(input + 2, &a, &b)) {
                float res = area(a, b);
                write_str("Area(");
                write_float(a);
                write_str(", ");
                write_float(b);
                write_str(") = ");
                write_float(res);
                write_str("\n");
            } else {
                write_str("Error: need two numbers\n");
            }
        }
        else if (input[0] == '2') {
            char *space = strchr(input, ' ');
            if (!space) {
                write_str("Error: array size missing\n");
                continue;
            }
            
            int n = strtol(space + 1, &space, 10);
            if (n <= 0 || n > 100) {
                write_str("Error: size 1..100\n");
                continue;
            }

            int *arr = malloc(n * sizeof(int));
            if (!arr) {
                write_str("Memory error\n");
                continue;
            }

            if (parse_array(space, arr, n)) {
                write_str("Array: ");
                write_array(arr, n);
                int *sorted = sort(arr, n);
                if (sorted) {
                    write_str("Sorted: ");
                    write_array(sorted, n);
                    free(sorted);
                } else {
                    write_str("Sort failed\n");
                }
            } else {
                write_str("Error: not enough elements\n");
            }
            free(arr);
        }
        else {
            write_str("Unknown command\n");
        }
    }

    return 0;
}