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
        const char msg[] = "Usage: child2 <parent_pid>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    pid_t parent_pid = atoi(argv[1]);
    
    char shm2_name[64], shm3_name[64], sem2_name[64], sem3_name[64];
    char ready2_name[64];
    
    snprintf(shm2_name, sizeof(shm2_name), "/shm2-%d", parent_pid);
    snprintf(shm3_name, sizeof(shm3_name), "/shm3-%d", parent_pid);
    snprintf(sem2_name, sizeof(sem2_name), "/sem2-%d", parent_pid);
    snprintf(sem3_name, sizeof(sem3_name), "/sem3-%d", parent_pid);
    snprintf(ready2_name, sizeof(ready2_name), "/ready2-%d", parent_pid);
    
    int shm2_fd = shm_open(shm2_name, O_RDWR, 0);
    if (shm2_fd == -1) {
        const char msg[] = "error: failed to open SHM2 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    int shm3_fd = shm_open(shm3_name, O_RDWR, 0);
    if (shm3_fd == -1) {
        const char msg[] = "error: failed to open SHM3 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm2 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm2_fd, 0);
    if (shm2 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM2 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    shm_data* shm3 = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm3_fd, 0);
    if (shm3 == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM3 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem2 = sem_open(sem2_name, 0);
    if (sem2 == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore2 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *sem3 = sem_open(sem3_name, 0);
    if (sem3 == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore3 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    sem_t *ready_sem2 = sem_open(ready2_name, 0);
    if (ready_sem2 == SEM_FAILED) {
        const char msg[] = "error: failed to open ready semaphore2 in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    
    const char start_msg[] = "Child2 gotov!\n";
    if (write(STDOUT_FILENO, start_msg, sizeof(start_msg) - 1) == -1) {
        const char msg[] = "error: failed to write start message\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    sem_post(ready_sem2);
    sem_close(ready_sem2);
    
    int running = 1;
    
    while (running) {
        if (sem_wait(sem2) == -1) {
            const char msg[] = "error: sem_wait failed in child2\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
        
        if (shm2->length > 0) {
            if (shm2->length == UINT32_MAX) {
                running = 0;
            } else {
                for (int i = 0; i < shm2->length && shm2->data[i]; i++) {
                    if (isspace(shm2->data[i])) {
                        shm2->data[i] = '_';
                    }
                }
                
                if (sem_wait(sem3) == -1) {
                    const char msg[] = "error: sem_wait failed in child2\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    sem_post(sem2);
                    break;
                }
                
                shm3->length = shm2->length;
                memcpy(shm3->data, shm2->data, shm2->length + 1);
                
                if (sem_post(sem3) == -1) {
                    const char msg[] = "error: sem_post failed in child2\n";
                    write(STDERR_FILENO, msg, sizeof(msg) - 1);
                    sem_post(sem2);
                    break;
                }
                
                shm2->length = 0;
            }
        }
        
        if (sem_post(sem2) == -1) {
            const char msg[] = "error: sem_post failed in child2\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            break;
        }
    }
    
    if (sem_close(sem2) == -1) {
        const char msg[] = "error: sem_close failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (sem_close(sem3) == -1) {
        const char msg[] = "error: sem_close failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm2, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (munmap(shm3, SHM_SIZE) == -1) {
        const char msg[] = "error: munmap failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm2_fd) == -1) {
        const char msg[] = "error: close failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    if (close(shm3_fd) == -1) {
        const char msg[] = "error: close failed in child2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    const char end_msg[] = "Child2 finished.\n";
    if (write(STDOUT_FILENO, end_msg, sizeof(end_msg) - 1) == -1) {
        const char msg[] = "error: failed to write finish message\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    
    return 0;
}