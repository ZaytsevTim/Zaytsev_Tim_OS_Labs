#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    long wins1;
    long wins2;
    pthread_mutex_t mutex;
} shared_data_t;

typedef struct {
    int k;
    int player1_score;
    int player2_score;
    long experiments;
    int thread_id;
    shared_data_t* shared;
} thread_data_t;

int get_logical_cores();
static void init_prng();
static int rock_n_roll();
static unsigned int rand_xorshift();
void* thread_worker(void* arg);
void parallel_version(int k, int tour, int score1, int score2, long experiment, int max_threads);