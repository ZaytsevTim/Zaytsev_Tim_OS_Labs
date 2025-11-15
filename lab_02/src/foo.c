#include "dice.h"

int get_logical_cores() {
    long cores = sysconf(_SC_NPROCESSORS_ONLN) - 1;
    if (cores <= 0) {
        return 4;
    }
    return (int)cores;
}

static __thread unsigned int seed = 0;

static void init_prng() {
    if (seed == 0) {
        if (syscall(SYS_getrandom, &seed, sizeof(seed), 0) != sizeof(seed)) {
            seed = (unsigned int)time(NULL) * (unsigned int)getpid();
        }
        if (seed == 0) seed = 1;
    }
}

static unsigned int rand_xorshift() {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static int rock_n_roll() {
    init_prng();
    return (rand_xorshift() % 6) + 1;
}


void* thread_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long local_wins1 = 0, local_wins2 = 0;
    
    for (long exp = 0; exp < data->experiments; exp++) {
        int p1_total = data->player1_score;
        int p2_total = data->player2_score;
        
        for (int i = 0; i < data->k; i++) {
            p1_total += rock_n_roll() + rock_n_roll();
            p2_total += rock_n_roll() + rock_n_roll();
        }
        
        if (p1_total > p2_total) {
            local_wins1++;
        } else if (p2_total > p1_total) {
            local_wins2++;
        }
    }
    
    pthread_mutex_lock(&data->shared->mutex);
    data->shared->wins1 += local_wins1;
    data->shared->wins2 += local_wins2;
    pthread_mutex_unlock(&data->shared->mutex);
    
    return NULL;
}

void parallel_version(int k, int tour, int score1, int score2, long experiments, int max_threads){
    shared_data_t shared;
    shared.wins1 = 0;
    shared.wins2 = 0;

    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        const char msg[] = "error: pthread_mutex_init failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }
    
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    int num_threads = (experiments < max_threads) ? experiments : max_threads;
    long experiments_per_thread = experiments / num_threads;
    long remaining_experiments = experiments % num_threads;
    
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    char info_msg[128];
    int info_len = snprintf(info_msg, sizeof(info_msg), 
                           "Using %d threads (logical cores: %d)\n", 
                           num_threads, get_logical_cores());
    write(STDERR_FILENO, info_msg, info_len);
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].k = k;
        thread_data[i].player1_score = score1;
        thread_data[i].player2_score = score2;
        thread_data[i].experiments = experiments_per_thread;
        thread_data[i].thread_id = i;
        thread_data[i].shared = &shared;
        
        if (i < remaining_experiments) {
            thread_data[i].experiments++;
        }
        
        int result = pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
        if (result != 0) {
            const char msg[] = "error: pthread_create failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            
            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
            }
            pthread_mutex_destroy(&shared.mutex);
            return;
        }
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    gettimeofday(&end_time, NULL);
    double execution_time = (end_time.tv_sec - start_time.tv_sec) + 
                           (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    char msg[1024];
    int len;
    
    len = snprintf(msg, sizeof(msg), "Player 1 wins: %ld (%.2f%%)\n", shared.wins1, (double)shared.wins1 / experiments * 100);
    write(STDOUT_FILENO, msg, len);
    
    len = snprintf(msg, sizeof(msg), "Player 2 wins: %ld (%.2f%%)\n", shared.wins2, (double)shared.wins2 / experiments * 100);
    write(STDOUT_FILENO, msg, len);
    
    len = snprintf(msg, sizeof(msg), "Execution time: %.3f seconds\n", execution_time);
    write(STDOUT_FILENO, msg, len);
    
    len = snprintf(msg, sizeof(msg), "Threads used: %d\n", num_threads);
    write(STDOUT_FILENO, msg, len);
    
    pthread_mutex_destroy(&shared.mutex);
}