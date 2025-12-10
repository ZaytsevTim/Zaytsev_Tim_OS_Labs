#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SHM_SIZE (BUFFER_SIZE + sizeof(uint32_t))

typedef struct {
    uint32_t length;
    char data[BUFFER_SIZE];
} shm_data;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        const char msg[] = "Usage: child1 <parent_pid>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    pid_t parent_pid = atoi(argv[1]);
    
    char shm1_name[64], shm2_name[64], sem1_name[64], sem2_name[64];
    char ready1_name[64];
    
    snprintf(shm1_name, sizeof(shm1_name), "/shm1-%d", parent_pid);
    snprintf(shm2_name, sizeof(shm2_name), "/shm2-%d", parent_pid);
    snprintf(sem1_name, sizeof(sem1_name), "/sem1-%d", parent_pid);
    snprintf(sem2_name, sizeof(sem2_name), "/sem2-%d", parent_pid);
    snprintf(ready1_name, sizeof(ready1_name), "/ready1-%d", parent_pid);
    
    int shm1_fd = shm_open(shm1_name, O_RDWR, 0);
    if (shm1_fd == -1) {
        const char msg[] = "error: failed to open SHM1 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    int shm2_fd = shm_open(shm2_name, O_RDWR, 0);
    if (shm2_fd == -1) {
        const char msg[] = "error: failed to open SHM2 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm1 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm1_fd, 0);
    if (shm1 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM1 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm2 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm2_fd, 0);
    if (shm2 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM2 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem1 = sem_open(sem1_name, 0);
    if (sem1 == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore1 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem2 = sem_open(sem2_name, 0);
    if (sem2 == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore2 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *ready_sem1 = sem_open(ready1_name, 0);
    if (ready_sem1 == SEM_FAILED) {
        const char msg[] = "error: failed to open ready semaphore1 in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    const char start_msg[] = "Child1 gotov!\n";
    if (write(STDOUT_FILENO, start_msg, sizeof(start_msg) - 1) == -1) {
        const char msg[] = "error: failed to write start message\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    sem_post(ready_sem1);
    sem_close(ready_sem1);
    
    int running = 1;
    
    while (running) {
        if (sem_wait(sem1) == -1) {
            const char msg[] = "error: sem_wait failed in child1\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        if (shm1->length > 0) {
            if (shm1->length == UINT32_MAX) {
                running = 0;
            } else {
                for (int i = 0; i < shm1->length && shm1->data[i]; i++) {
                    shm1->data[i] = toupper(shm1->data[i]);
                }
                
                if (sem_wait(sem2) == -1) {
                    const char msg[] = "error: sem_wait failed in child1\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    sem_post(sem1);
                    break;
                }
                
                shm2->length = shm1->length;
                memcpy(shm2->data, shm1->data, shm1->length + 1);
                
                if (sem_post(sem2) == -1) {
                    const char msg[] = "error: sem_post failed in child1\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    sem_post(sem1);
                    break;
                }
                
                shm1->length = 0;
            }
        }
        
        if (sem_post(sem1) == -1) {
            const char msg[] = "error: sem_post failed in child1\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
    }
    
    if (sem_wait(sem2) == -1) {
        const char msg[] = "error: sem_wait failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    } else {
        shm2->length = UINT32_MAX;
        if (sem_post(sem2) == -1) {
            const char msg[] = "error: sem_post failed in child1\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
        }
    }
    
    if (sem_close(sem1) == -1) {
        const char msg[] = "error: sem_close failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_close(sem2) == -1) {
        const char msg[] = "error: sem_close failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm1, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm2, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm1_fd) == -1) {
        const char msg[] = "error: close failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm2_fd) == -1) {
        const char msg[] = "error: close failed in child1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    const char end_msg[] = "Child1 finished.\n";
    if (write(STDOUT_FILENO, end_msg, sizeof(end_msg) - 1) == -1) {
        const char msg[] = "error: failed to write finish message\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    return 0;
}