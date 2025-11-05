#include <stdint.h>

#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <ctype.h>


#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

typedef struct {
    long wins1;
    long wins2;
    sem_t mutex;
} shared_data_t;

typedef struct {
    int k;
    int player1_score;
    int player2_score;
    long experiments;
    int process_id;
    uint32_t rng_state;
} process_data_t;

long total_wins1 = 0;
long total_wins2 = 0;
long total_draws = 0;

int max_processes = 12;

int string_to_int(const char* str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    while (str[i] != '\0') {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
            i++;
        } else {
            break;
        }
    }
    
    return result * sign;
}

long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static int rock_n_roll(uint32_t* state) {
    return (xorshift32(state) % 6) + 1;
}

void child_process(process_data_t data, shared_data_t* shared) {
    
    long local_wins1 = 0, local_wins2 = 0, local_draws = 0;
    
    data.rng_state = (uint32_t)(time(NULL) + data.process_id + getpid());

    for (long exp = 0; exp < data.experiments; exp++) {
        int p1_total = data.player1_score;
        int p2_total = data.player2_score;
        
        for (int i = 0; i < data.k; i++) {
            p1_total += rock_n_roll(&data.rng_state) + rock_n_roll(&data.rng_state);
            p2_total += rock_n_roll(&data.rng_state) + rock_n_roll(&data.rng_state);
        }
        
        if (p1_total > p2_total) {
            local_wins1++;
        } else if (p2_total > p1_total) {
            local_wins2++;
        }
    }
    
    sem_wait(&shared->mutex);
    shared->wins1 += local_wins1;
    shared->wins2 += local_wins2;
    sem_post(&shared->mutex);
    
    exit(EXIT_SUCCESS);
}

void parallel_version(int k, int tour, int score1, int score2, long experiments, int num_processes) {
    if (num_processes > max_processes) {
        num_processes = max_processes;
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "Warning: limiting to %d processes (max allowed: -m %d)\n", max_processes, max_processes);
        write(STDERR_FILENO, msg, len);
    }
    shared_data_t* shared = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    
    if (shared == MAP_FAILED) {
        const char msg[] = "error: mmap failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shared->wins1 = 0;
    shared->wins2 = 0;
    
    char sem_name[32];
    snprintf(sem_name, sizeof(sem_name), "/dice_sem_%d", getpid());
    sem_t* mutex = sem_open(sem_name, O_CREAT | O_EXCL, 0644, 1);
    if (mutex == SEM_FAILED) {
        const char msg[] = "error: sem_open failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shared->mutex = *mutex;
    
    long experiments_per_process = experiments / num_processes;
    long remaining_experiments = experiments % num_processes;
    
    long long start_time = get_current_time_ms();
    
    pid_t pids[num_processes];
    for (int i = 0; i < num_processes; i++) {
        process_data_t data;
        data.k = k;
        data.player1_score = score1;
        data.player2_score = score2;
        data.experiments = experiments_per_process;
        data.process_id = i;
        
        if (i < remaining_experiments) {
            data.experiments++;
        }
        
        pid_t pid = fork();
        if (pid == 0) {
            child_process(data, shared);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            const char msg[] = "error: failed to fork(\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }
    
    for (int i = 0; i < num_processes; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    long long end_time = get_current_time_ms();
    double execution_time = (end_time - start_time) / 1000.0;
    char msg[1024];
    
    uint32_t len2 = snprintf(msg, sizeof(msg) - 1, "Player 1 wins: %ld (%.2f%%)\n",  shared->wins1, (double)shared->wins1 / experiments * 100);
    write(STDERR_FILENO, msg, len2);

    uint32_t len3 = snprintf(msg, sizeof(msg) - 1, "Player 2 wins: %ld (%.2f%%)\n", shared->wins2, (double)shared->wins2 / experiments * 100);
    write(STDERR_FILENO, msg, len3);

    uint32_t len5 = snprintf(msg, sizeof(msg) - 1, "Execution time: %.3f seconds\n", execution_time);
    write(STDERR_FILENO, msg, len5);

    sem_close(mutex);
    sem_unlink(sem_name);
    munmap(shared, sizeof(shared_data_t));
    
}

int main(int argc, char **argv) {
    int skip_args = 1; 

    int K, tour, pts1, pts2, prs;
    long exs;
    
    if (argc == 9 && strcmp(argv[1], "-m") == 0) {
        max_processes = string_to_int(argv[2]);
        K = string_to_int(argv[3]);
        tour = string_to_int(argv[4]);
        pts1 = string_to_int(argv[5]);
        pts2 = string_to_int(argv[6]);
        exs = (long)string_to_int(argv[7]);
        prs= string_to_int(argv[8]);
    }
    else if (argc == 7) {
        K = string_to_int(argv[1]);
        tour = string_to_int(argv[2]);
        pts1 = string_to_int(argv[3]);
        pts2 = string_to_int(argv[4]);
        exs = (long)string_to_int(argv[5]);
        prs = string_to_int(argv[6]);
    }
    else {
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s [-m MAX_PROCESSES] K tour p1_pts p2_pts experiments processes\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        return 1;
    }

    parallel_version(K, tour, pts1, pts2, exs, prs);
    return 0;
}