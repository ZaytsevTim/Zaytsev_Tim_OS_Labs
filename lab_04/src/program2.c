#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

typedef float (*area_func)(float, float);
typedef int* (*sort_func)(int*, size_t);

static float area_stub(float a, float b) {
    (void)a; (void)b;
    return 0.0f;
}

static int* sort_stub(int *arr, size_t n) {
    (void)arr; (void)n;
    return NULL;
}

static area_func area_ptr = area_stub;
static sort_func sort_ptr = sort_stub;
static void *area_lib = NULL;
static void *sort_lib = NULL;
static int version = 1;

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

static void load_libs(int ver) {
    const char *area_name = (ver == 1) ? "./area1.so" : "./area2.so";
    const char *sort_name = (ver == 1) ? "./sort1.so" : "./sort2.so";

    if (area_lib) dlclose(area_lib);
    if (sort_lib) dlclose(sort_lib);

    area_lib = dlopen(area_name, RTLD_LAZY);
    if (!area_lib) {
        write_str("Error loading area library\n");
        area_ptr = area_stub;
    } else {
        area_ptr = (area_func)dlsym(area_lib, "area");
        if (!area_ptr) {
            write_str("Warning: area function not found\n");
            area_ptr = area_stub;
        }
    }

    sort_lib = dlopen(sort_name, RTLD_LAZY);
    if (!sort_lib) {
        write_str("Error loading sort library\n");
        sort_ptr = sort_stub;
    } else {
        sort_ptr = (sort_func)dlsym(sort_lib, "sort");
        if (!sort_ptr) {
            write_str("Warning: sort function not found\n");
            sort_ptr = sort_stub;
        }
    }

    char msg[128];
    int len = snprintf(msg, sizeof(msg), "Loaded version %d\n", ver);
    write(STDOUT_FILENO, msg, len);
}

int main(void) {
    char input[256];

    write_str("Program 2: Dynamic loading\n");
    write_str("Commands:\n");
    write_str(" 0 - switch version (1 or 2)\n");
    write_str(" 1 a b - area\n");
    write_str(" 2 n a1 ... an - sort\n");
    write_str(" exit - exit\n\n");

    load_libs(version);

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

        if (input[0] == '0') {
            version = (version == 1) ? 2 : 1;
            load_libs(version);
        }
        else if (input[0] == '1') {
            float a, b;
            if (parse_two_floats(input + 2, &a, &b)) {
                float res = area_ptr(a, b);
                write_str("Area(");
                write_float(a);
                write_str(", ");
                write_float(b);
                write_str(") = ");
                write_float(res);
                write_str(" (v");
                write_int(version);
                write_str(version == 1 ? ": rectangle)\n" : ": triangle)\n");
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
                int *sorted = sort_ptr(arr, n);
                if (sorted) {
                    write_str("Sorted: ");
                    write_array(sorted, n);
                    write_str(" (v");
                    write_int(version);
                    write_str(version == 1 ? ": bubble)\n" : ": quick)\n");
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

    if (area_lib) dlclose(area_lib);
    if (sort_lib) dlclose(sort_lib);

    return 0;
}