#include <stdlib.h>
#include <string.h>

int* sort(int* array, size_t n) {
    if (n == 0) return NULL;
    int* result = (int*)malloc(n * sizeof(int));
    if (!result) return NULL;
    memcpy(result, array, n * sizeof(int));

    for (size_t i = 0; i < n - 1; i++) {
        for (size_t j = 0; j < n - i - 1; j++) {
            if (result[j] > result[j + 1]) {
                int temp = result[j];
                result[j] = result[j + 1];
                result[j + 1] = temp;
            }
        }
    }
    return result;
}