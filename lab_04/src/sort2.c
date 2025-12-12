#include <stdlib.h>
#include <string.h>

static void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

static int hoare_partition(int* arr, int low, int high) {
    int pivot = arr[(low + high) / 2];  
    int i = low - 1;
    int j = high + 1;
    
    while (1) {
        i++;
        while (i <= high && arr[i] < pivot) {
            i++;
        }
        
        j--;
        while (j >= low && arr[j] > pivot) {
            j--;
        }
        
        if (i >= j) return j;
        
        swap(&arr[i], &arr[j]);
    }
}

static void quick_sort_hoare(int* arr, int low, int high) {
    if (low < high) {
        int p = hoare_partition(arr, low, high);
        quick_sort_hoare(arr, low, p);
        quick_sort_hoare(arr, p + 1, high);
    }
}

int* sort(int* array, size_t n) {
    if (n == 0) return NULL;
    int* result = (int*)malloc(n * sizeof(int));
    if (!result) return NULL;
    memcpy(result, array, n * sizeof(int));
    quick_sort_hoare(result, 0, n - 1);
    return result;
}