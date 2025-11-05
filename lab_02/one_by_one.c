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


long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int get_random_number() {
    unsigned int result;
    ssize_t bytes_read = syscall(SYS_getrandom, &result, sizeof(result), 0);
    if (bytes_read != sizeof(result)) {
        const char msg[] = "error: failed to get random bytes\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(EXIT_FAILURE);
    }
    return result;
}

static int rock_n_roll() {
    return (get_random_number() % 6) + 1;
}

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

void sequential_version(int k, int tour, int score1, int score2, long experiments) {
    long wins1 = 0, wins2 = 0, draws = 0;
    if (tour > k){
        char msg[1024];
	    uint32_t ln = snprintf(msg, sizeof(msg) - 1, "Tour number is bigger then amount of tours!");
	    write(STDERR_FILENO, msg, ln);  
        return;     
    }
    
    long long start_time = get_current_time_ms();
    
    for (long exp = 0; exp < experiments; exp++) {
        int p1_total = score1;
        int p2_total = score2;
        
        for (int i = 0; i < (k - (tour - 1)); i++) {
            p1_total += rock_n_roll()  + rock_n_roll();
            p2_total += rock_n_roll()  + rock_n_roll();
        }
        
        if (p1_total > p2_total) {
            wins1++;
        } else if (p2_total > p1_total) {
            wins2++;
        } 
    }
    
    long long end_time = get_current_time_ms();
    double execution_time = (end_time - start_time) / 1000.0;

	char msg[1024];
	uint32_t len1 = snprintf(msg, sizeof(msg) - 1, "Player 1 wins: %ld (%.2f%%)\n", wins1, (double)wins1 / experiments * 100);
	write(STDERR_FILENO, msg, len1);

    int32_t len2 = snprintf(msg, sizeof(msg) - 1, "Player 2 wins: %ld (%.2f%%)\n", wins2, (double)wins2 / experiments * 100);
	write(STDERR_FILENO, msg, len2);

    int32_t len3 = snprintf(msg, sizeof(msg) - 1, " Execution time: %.3f seconds\n", execution_time);
	write(STDERR_FILENO, msg, len3);

}

int main(int argc, char **argv){
    if (argc < 6){
        char msg[1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s K tour_number player1_pts player2_pts amount_of_experiments\n", argv[0]);
        write(STDERR_FILENO, msg, len);

    }
    int K = string_to_int(argv[1]);
    int tour = string_to_int(argv[2]);
    int pts1 = string_to_int(argv[3]);
    int pts2 = string_to_int(argv[4]);
    long exs = (long)string_to_int(argv[5]); 


    sequential_version(K, tour, pts1, pts2, exs);
}