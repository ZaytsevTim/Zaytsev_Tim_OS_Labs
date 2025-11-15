#include "dice.h"

int main(int argc, char **argv) {
    int K, tour, pts1, pts2, max_threads;
    long exs;
    
    if (argc == 8 && strcmp(argv[1], "-m") == 0) {
        max_threads = atoi(argv[2]);
        K = atoi(argv[3]);
        tour = atoi(argv[4]);
        pts1 = atoi(argv[5]);
        pts2 = atoi(argv[6]);
        exs = atol(argv[7]);
    }
    else if (argc == 6) {
        K = atoi(argv[1]);
        tour = atoi(argv[2]);
        pts1 = atoi(argv[3]);
        pts2 = atoi(argv[4]);
        exs = atol(argv[5]);
        max_threads = get_logical_cores();
    }
    else {
        char msg[256];
        snprintf(msg, sizeof(msg), "Usage: %s [-m MAX_THREADS] K tour p1_pts p2_pts experiments", argv[0]);
        write(STDERR_FILENO, msg, strlen(msg));
        return 1;
    }

    parallel_version(K, tour, pts1, pts2, exs, max_threads);
    return 0;
}